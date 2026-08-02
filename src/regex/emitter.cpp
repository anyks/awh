/**
 * @file: emitter.cpp
 * @date: 2026-08-02
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация порождения машинного кода ARM64 — сборка команд процессора
 *        из полей их двоичного представления и отложенное разрешение переходов
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <regex/emitter.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;

/**
 * @brief Пространство имён двоичного представления команд ARM64
 *
 * @details Команда ARM64 занимает ровно четыре байта и собирается из полей
 *          с постоянными положениями. Значения основ взяты из описания набора
 *          команд и проверены сличением с выводом ассемблера.
 *
 */
namespace {
	/**
	 * @brief Наибольшее число, размещаемое в поле команды
	 *
	 * @details Команды сложения, вычитания и сравнения несут двенадцать разрядов
	 *          числа, поэтому больший довод в команду не помещается.
	 *
	 */
	constexpr uint32_t MAX_IMMEDIATE = 0xFFF;

	/**
	 * @brief Наибольшее смещение перехода по условию в командах
	 *
	 * @details Условный переход несёт девятнадцать разрядов смещения со знаком,
	 *          отчего достижимы восемнадцать разрядов в каждую сторону.
	 *
	 */
	constexpr int64_t MAX_BRANCH = 0x3FFFF;

	/**
	 * @brief Наибольшее смещение обычного перехода в командах
	 *
	 * @details Обычный переход несёт двадцать шесть разрядов смещения со знаком,
	 *          отчего достижимы двадцать пять разрядов в каждую сторону.
	 *
	 */
	constexpr int64_t MAX_JUMP = 0x1FFFFFF;

	/**
	 * @brief Наибольший номер значения таблицы адресов обстановки
	 *
	 * @details Чтение восьмибайтового значения по смещению несёт двенадцать
	 *          разрядов номера значения.
	 *
	 */
	constexpr uint32_t MAX_INDEX = 0xFFF;

	/**
	 * @brief Функция сборки команды сравнения значений регистров
	 *
	 * @param first  номер регистра уменьшаемого значения
	 * @param second номер регистра вычитаемого значения
	 * @return       собранная команда процессора
	 *
	 */
	inline uint32_t compare(const uint32_t first, const uint32_t second) noexcept {
		// Выводим собранную команду «subs xzr, first, second»
		return (0xEB000000u | (second << 16) | (first << 5) | 31u);
	}
	/**
	 * @brief Функция сборки команды сравнения значения регистра с числом
	 *
	 * @param reg   номер регистра сравниваемого значения
	 * @param value сравниваемое число
	 * @return      собранная команда процессора
	 *
	 */
	inline uint32_t compare(const uint32_t reg, const uint32_t value, const bool) noexcept {
		/**
		 * Выводим собранную команду «subs xzr, xreg, #value»
		 *
		 * @details Сравнение выполняется над всеми разрядами регистра, а не над
		 *          младшими тридцатью двумя: регистры соглашения о вызове несут
		 *          положения в тексте, длина какого тридцатью двумя разрядами
		 *          не ограничена.
		 *
		 */
		return (0xF1000000u | (value << 10) | (reg << 5) | 31u);
	}
	/**
	 * @brief Функция сборки команды сложения значения регистра с числом
	 *
	 * @param target номер регистра итога сложения
	 * @param source номер регистра слагаемого значения
	 * @param value  прибавляемое число
	 * @return       собранная команда процессора
	 *
	 */
	inline uint32_t add(const uint32_t target, const uint32_t source, const uint32_t value) noexcept {
		// Выводим собранную команду «add xtarget, xsource, #value»
		return (0x91000000u | (value << 10) | (source << 5) | target);
	}
	/**
	 * @brief Функция сборки команды вычитания числа из значения регистра
	 *
	 * @param target номер регистра итога вычитания
	 * @param source номер регистра уменьшаемого значения
	 * @param value  вычитаемое число
	 * @return       собранная команда процессора
	 *
	 */
	inline uint32_t sub(const uint32_t target, const uint32_t source, const uint32_t value) noexcept {
		/**
		 * Выводим собранную команду «sub xtarget, xsource, #value»
		 *
		 * @details Вычитание выполняется над всеми разрядами регистра: над младшими
		 *          тридцатью двумя оно обнуляло бы старшие, а вычитание из указателя
		 *          стека сделало бы его непригодным вовсе.
		 *
		 */
		return (0xD1000000u | (value << 10) | (source << 5) | target);
	}
	/**
	 * @brief Функция сборки команды переноса значения регистра
	 *
	 * @param target номер регистра назначения переноса
	 * @param source номер регистра источника переноса
	 * @return       собранная команда процессора
	 *
	 */
	inline uint32_t move(const uint32_t target, const uint32_t source) noexcept {
		// Выводим собранную команду «orr xtarget, xzr, xsource»
		return (0xAA0003E0u | (source << 16) | target);
	}
	/**
	 * @brief Функция сборки команды записи части числа в регистр
	 *
	 * @param target номер регистра назначения записи
	 * @param value  записываемая часть числа
	 * @param shift  номер шестнадцатиразрядной части числа
	 * @param keep   флаг сохранения прочих частей значения регистра
	 * @return       собранная команда процессора
	 *
	 */
	inline uint32_t write(const uint32_t target, const uint32_t value, const uint32_t shift, const bool keep) noexcept {
		// Выводим собранную команду «movz xtarget, #value, lsl #shift» либо «movk»
		return ((keep ? 0xF2800000u : 0xD2800000u) | (shift << 21) | (value << 5) | target);
	}
	/**
	 * @brief Функция сборки команды чтения байта по сумме регистров
	 *
	 * @param target номер регистра прочитанного значения байта
	 * @param base   номер регистра адреса начала области чтения
	 * @param offset номер регистра смещения читаемого байта
	 * @return       собранная команда процессора
	 *
	 */
	inline uint32_t load(const uint32_t target, const uint32_t base, const uint32_t offset) noexcept {
		// Выводим собранную команду «ldrb wtarget, [xbase, xoffset]»
		return (0x38606800u | (offset << 16) | (base << 5) | target);
	}
	/**
	 * @brief Функция сборки команды чтения восьмибайтового значения по смещению
	 *
	 * @param target номер регистра прочитанного значения
	 * @param base   номер регистра адреса начала области чтения
	 * @param index  номер читаемого значения в области
	 * @return       собранная команда процессора
	 *
	 */
	inline uint32_t fetch(const uint32_t target, const uint32_t base, const uint32_t index) noexcept {
		// Выводим собранную команду «ldr xtarget, [xbase, #(index * 8)]»
		return (0xF9400000u | (index << 10) | (base << 5) | target);
	}
	/**
	 * @brief Функция сборки команды записи восьмибайтового значения по смещению
	 *
	 * @param source номер регистра записываемого значения
	 * @param base   номер регистра адреса начала области записи
	 * @param index  номер записываемого значения в области
	 * @return       собранная команда процессора
	 *
	 */
	inline uint32_t store(const uint32_t source, const uint32_t base, const uint32_t index) noexcept {
		// Выводим собранную команду «str xsource, [xbase, #(index * 8)]»
		return (0xF9000000u | (index << 10) | (base << 5) | source);
	}
};

/**
 * @brief Метод проверки поддержки порождения машинного кода сборкой
 *
 * @return результат проверки поддержки порождения машинного кода
 *
 */
bool awh::regex::Emitter::available() noexcept {
	/**
	 * Если сборка выполняется для процессора с набором команд ARM64
	 */
	#if defined(__aarch64__) || defined(_M_ARM64)
		// Выводим поддержку порождения машинного кода сборкой
		return true;
	/**
	 * Если сборка выполняется для прочих наборов команд
	 */
	#else
		// Выводим отсутствие поддержки порождения машинного кода сборкой
		return false;
	#endif
}
/**
 * @brief Метод очистки порождаемой последовательности команд
 *
 */
void awh::regex::Emitter::clear() noexcept {
	// Выполняем очистку порождаемой последовательности команд
	this->_code.clear();
	// Выполняем очистку положений меток перехода
	this->_labels.clear();
	// Выполняем очистку набора отложенных переходов
	this->_fixups.clear();
	// Выполняем сброс флага отказа порождения машинного кода
	this->_failed = false;
}
/**
 * @brief Метод заведения метки перехода
 *
 * @return номер заведённой метки перехода
 *
 */
size_t awh::regex::Emitter::label() noexcept {
	// Получаем номер заводимой метки перехода
	const size_t result = this->_labels.size();
	// Выполняем заведение метки, положения не имеющей
	this->_labels.push_back(INVALID_LABEL);
	// Выводим номер заведённой метки перехода
	return result;
}
/**
 * @brief Метод расстановки метки перехода
 *
 * @param label номер расставляемой метки перехода
 *
 */
void awh::regex::Emitter::place(const size_t label) noexcept {
	/**
	 * Если метка перехода не заведена
	 */
	if(label >= this->_labels.size()) {
		// Выполняем установку флага отказа порождения машинного кода
		this->_failed = true;
		// Выходим из метода расстановки метки перехода
		return;
	}
	// Выполняем установку положения метки перехода
	this->_labels.at(label) = this->_code.size();
}
/**
 * @brief Метод размещения перехода к метке
 *
 * @param label номер метки, к какой выполняется переход
 *
 */
void awh::regex::Emitter::jump(const size_t label) noexcept {
	/**
	 * Если метка перехода не заведена
	 */
	if(label >= this->_labels.size()) {
		// Выполняем установку флага отказа порождения машинного кода
		this->_failed = true;
		// Выходим из метода размещения перехода к метке
		return;
	}
	// Выполняем размещение отложенного перехода
	this->_fixups.emplace_back();
	// Выполняем установку положения команды перехода
	this->_fixups.back().position = this->_code.size();
	// Выполняем установку номера метки перехода
	this->_fixups.back().label = label;
	// Выполняем установку признака безусловного перехода
	this->_fixups.back().conditional = false;
	// Выполняем размещение команды перехода, смещения ещё не несущей
	this->_code.push_back(0x14000000u);
}
/**
 * @brief Метод размещения перехода к метке по условию
 *
 * @param cond  условие выполнения перехода
 * @param label номер метки, к какой выполняется переход
 *
 */
void awh::regex::Emitter::branch(const cond_t cond, const size_t label) noexcept {
	/**
	 * Если метка перехода не заведена
	 */
	if(label >= this->_labels.size()) {
		// Выполняем установку флага отказа порождения машинного кода
		this->_failed = true;
		// Выходим из метода размещения перехода к метке
		return;
	}
	// Выполняем размещение отложенного перехода
	this->_fixups.emplace_back();
	// Выполняем установку положения команды перехода
	this->_fixups.back().position = this->_code.size();
	// Выполняем установку номера метки перехода
	this->_fixups.back().label = label;
	// Выполняем установку признака перехода по условию
	this->_fixups.back().conditional = true;
	// Выполняем размещение команды перехода, смещения ещё не несущей
	this->_code.push_back(0x54000000u | static_cast <uint32_t> (cond));
}
/**
 * @brief Метод размещения сравнения значений регистров
 *
 * @param first  регистр уменьшаемого значения
 * @param second регистр вычитаемого значения
 *
 */
void awh::regex::Emitter::compare(const reg_t first, const reg_t second) noexcept {
	// Выполняем размещение команды сравнения значений регистров
	this->_code.push_back(::compare(static_cast <uint32_t> (first), static_cast <uint32_t> (second)));
}
/**
 * @brief Метод размещения сравнения значения регистра с числом
 *
 * @param reg   регистр сравниваемого значения
 * @param value сравниваемое число
 *
 */
void awh::regex::Emitter::compare(const reg_t reg, const uint32_t value) noexcept {
	/**
	 * Если сравниваемое число в поле команды не помещается
	 */
	if(value > MAX_IMMEDIATE) {
		// Выполняем установку флага отказа порождения машинного кода
		this->_failed = true;
		// Выходим из метода размещения сравнения
		return;
	}
	// Выполняем размещение команды сравнения значения регистра с числом
	this->_code.push_back(::compare(static_cast <uint32_t> (reg), value, true));
}
/**
 * @brief Метод размещения сложения значения регистра с числом
 *
 * @param target регистр итога сложения
 * @param source регистр слагаемого значения
 * @param value  прибавляемое число
 *
 */
void awh::regex::Emitter::add(const reg_t target, const reg_t source, const uint32_t value) noexcept {
	/**
	 * Если прибавляемое число в поле команды не помещается
	 */
	if(value > MAX_IMMEDIATE) {
		// Выполняем установку флага отказа порождения машинного кода
		this->_failed = true;
		// Выходим из метода размещения сложения
		return;
	}
	// Выполняем размещение команды сложения значения регистра с числом
	this->_code.push_back(::add(static_cast <uint32_t> (target), static_cast <uint32_t> (source), value));
}
/**
 * @brief Метод размещения вычитания числа из значения регистра
 *
 * @param target регистр итога вычитания
 * @param source регистр уменьшаемого значения
 * @param value  вычитаемое число
 *
 */
void awh::regex::Emitter::sub(const reg_t target, const reg_t source, const uint32_t value) noexcept {
	/**
	 * Если вычитаемое число в поле команды не помещается
	 */
	if(value > MAX_IMMEDIATE) {
		// Выполняем установку флага отказа порождения машинного кода
		this->_failed = true;
		// Выходим из метода размещения вычитания
		return;
	}
	// Выполняем размещение команды вычитания числа из значения регистра
	this->_code.push_back(::sub(static_cast <uint32_t> (target), static_cast <uint32_t> (source), value));
}
/**
 * @brief Метод размещения переноса значения регистра
 *
 * @param target регистр назначения переноса
 * @param source регистр источника переноса
 *
 */
void awh::regex::Emitter::move(const reg_t target, const reg_t source) noexcept {
	// Выполняем размещение команды переноса значения регистра
	this->_code.push_back(::move(static_cast <uint32_t> (target), static_cast <uint32_t> (source)));
}
/**
 * @brief Метод размещения записи числа в регистр
 *
 * @details Число записывается по шестнадцать разрядов: первая записанная часть
 *          обнуляет прочие разряды регистра, последующие их сохраняют. Части,
 *          равные нулю, пропускаются, отчего малое число записывается одной командой.
 *
 * @param target регистр назначения записи
 * @param value  записываемое число
 *
 */
void awh::regex::Emitter::move(const reg_t target, const uint64_t value) noexcept {
	// Получаем номер регистра назначения записи
	const uint32_t reg = static_cast <uint32_t> (target);
	// Флаг размещения первой части записываемого числа
	bool written = false;
	/**
	 * Выполняем обход шестнадцатиразрядных частей записываемого числа
	 */
	for(uint32_t shift = 0; shift < 4; shift++) {
		// Получаем очередную часть записываемого числа
		const uint32_t part = static_cast <uint32_t> ((value >> (shift * 16)) & 0xFFFFull);
		/**
		 * Если очередная часть числа равна нулю
		 *
		 * @details Обнуление прочих разрядов выполняет первая размещённая команда,
		 *          поэтому нулевая часть размещения не требует.
		 *
		 */
		if((part == 0) && written)
			// Переходим к следующей части записываемого числа
			continue;
		/**
		 * Если очередная часть числа равна нулю до размещения первой части
		 */
		if((part == 0) && (shift < 3)) {
			// Получаем остаток записываемого числа
			const uint64_t remains = (value >> ((shift + 1) * 16));
			/**
			 * Если остаток записываемого числа не пуст
			 */
			if(remains != 0)
				// Переходим к следующей части записываемого числа
				continue;
		}
		// Выполняем размещение команды записи части числа в регистр
		this->_code.push_back(::write(reg, part, shift, written));
		// Выполняем установку флага размещения первой части числа
		written = true;
	}
}
/**
 * @brief Метод размещения чтения байта текста
 *
 * @param target регистр прочитанного значения байта
 * @param base   регистр адреса начала области чтения
 * @param offset регистр смещения читаемого байта
 *
 */
void awh::regex::Emitter::load(const reg_t target, const reg_t base, const reg_t offset) noexcept {
	// Выполняем размещение команды чтения байта по сумме регистров
	this->_code.push_back(::load(static_cast <uint32_t> (target), static_cast <uint32_t> (base), static_cast <uint32_t> (offset)));
}
/**
 * @brief Метод размещения чтения значения обстановки исполнения
 *
 * @param target регистр прочитанного значения
 * @param index  номер значения в таблице адресов обстановки
 *
 */
void awh::regex::Emitter::context(const reg_t target, const uint32_t index) noexcept {
	/**
	 * Если номер значения в поле команды не помещается
	 */
	if(index > MAX_INDEX) {
		// Выполняем установку флага отказа порождения машинного кода
		this->_failed = true;
		// Выходим из метода размещения чтения
		return;
	}
	// Выполняем размещение команды чтения значения обстановки исполнения
	this->_code.push_back(::fetch(static_cast <uint32_t> (target), static_cast <uint32_t> (reg_t::CONTEXT), index));
}
/**
 * @brief Метод размещения чтения значения из памяти
 *
 * @param target регистр прочитанного значения
 * @param base   регистр адреса начала области чтения
 * @param index  номер читаемого значения в области
 *
 */
void awh::regex::Emitter::fetch(const reg_t target, const reg_t base, const uint32_t index) noexcept {
	/**
	 * Если номер значения в поле команды не помещается
	 */
	if(index > MAX_INDEX) {
		// Выполняем установку флага отказа порождения машинного кода
		this->_failed = true;
		// Выходим из метода размещения чтения
		return;
	}
	// Выполняем размещение команды чтения значения из памяти
	this->_code.push_back(::fetch(static_cast <uint32_t> (target), static_cast <uint32_t> (base), index));
}
/**
 * @brief Метод размещения записи значения регистра в память
 *
 * @param source регистр записываемого значения
 * @param base   регистр адреса начала области записи
 * @param index  номер записываемого значения в области
 *
 */
void awh::regex::Emitter::store(const reg_t source, const reg_t base, const uint32_t index) noexcept {
	/**
	 * Если номер значения в поле команды не помещается
	 */
	if(index > MAX_INDEX) {
		// Выполняем установку флага отказа порождения машинного кода
		this->_failed = true;
		// Выходим из метода размещения записи
		return;
	}
	// Выполняем размещение команды записи значения регистра в память
	this->_code.push_back(::store(static_cast <uint32_t> (source), static_cast <uint32_t> (base), index));
}
/**
 * @brief Метод размещения завершения вызова
 *
 */
void awh::regex::Emitter::ret() noexcept {
	// Выполняем размещение команды «ret»
	this->_code.push_back(0xD65F03C0u);
}
/**
 * @brief Метод разрешения отложенных переходов
 *
 * @return результат разрешения отложенных переходов
 *
 */
bool awh::regex::Emitter::resolve() noexcept {
	/**
	 * Если порождение машинного кода отказано
	 */
	if(this->_failed)
		// Выводим результат разрешения отложенных переходов
		return false;
	/**
	 * Выполняем обход набора отложенных переходов
	 */
	for(auto & fixup : this->_fixups) {
		// Получаем положение метки, к какой выполняется переход
		const size_t target = this->_labels.at(fixup.label);
		/**
		 * Если метка перехода положения не получила
		 */
		if(target == INVALID_LABEL) {
			// Выполняем установку флага отказа порождения машинного кода
			this->_failed = true;
			// Выводим результат разрешения отложенных переходов
			return false;
		}
		/**
		 * Получаем смещение перехода в командах
		 *
		 * @details Смещение отсчитывается от самой команды перехода, а не от
		 *          следующей за нею: так устроен набор команд ARM64.
		 *
		 */
		const int64_t delta = (static_cast <int64_t> (target) - static_cast <int64_t> (fixup.position));
		/**
		 * Если смещение перехода за пределы поля команды выходит
		 */
		if(fixup.conditional ? ((delta > MAX_BRANCH) || (delta < -MAX_BRANCH)) : ((delta > MAX_JUMP) || (delta < -MAX_JUMP))) {
			// Выполняем установку флага отказа порождения машинного кода
			this->_failed = true;
			// Выводим результат разрешения отложенных переходов
			return false;
		}
		/**
		 * Если переход выполняется по условию
		 */
		if(fixup.conditional)
			// Выполняем вписывание смещения в команду перехода по условию
			this->_code.at(fixup.position) |= ((static_cast <uint32_t> (delta) & 0x7FFFFu) << 5);
		/**
		 * Если переход выполняется безусловно
		 */
		else this->_code.at(fixup.position) |= (static_cast <uint32_t> (delta) & 0x3FFFFFFu);
	}
	// Выполняем очистку набора разрешённых переходов
	this->_fixups.clear();
	// Выводим результат разрешения отложенных переходов
	return true;
}
/**
 * @brief Метод извлечения порождённой последовательности команд
 *
 * @return порождённая последовательность команд процессора
 *
 */
const vector <uint32_t> & awh::regex::Emitter::code() const noexcept {
	// Выводим порождённую последовательность команд процессора
	return this->_code;
}
/**
 * @brief Метод извлечения размера порождённого машинного кода
 *
 * @return размер порождённого машинного кода в байтах
 *
 */
size_t awh::regex::Emitter::length() const noexcept {
	// Выводим размер порождённого машинного кода
	return (this->_code.size() * sizeof(uint32_t));
}
/**
 * @brief Метод проверки отказа порождения машинного кода
 *
 * @return результат проверки отказа порождения машинного кода
 *
 */
bool awh::regex::Emitter::failed() const noexcept {
	// Выводим результат проверки отказа порождения машинного кода
	return this->_failed;
}
/**
 * @brief Конструктор
 *
 */
awh::regex::Emitter::Emitter() noexcept : _failed(false) {}
