/**
 * @file: storage.cpp
 * @date: 2026-08-04
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Файл реализации хранилища собранных регулярных выражений —
 *        запись собранных выражений последовательностью байтов и восстановление
 *        их из записи без повторного разбора и компиляции
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы модуля
 */
#include <regex/storage.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;

/**
 * @brief Наибольший допустимый размер записи хранилища
 *
 * @details Размер ограничен затем, чтобы запись испорченная, объявляющая
 *          количество несообразное, не привела к попытке размещения памяти
 *          объёмом непомерным до чтения самого содержимого.
 *
 */
static constexpr size_t MAX_STORAGE = (1024ull * 1024ull * 1024ull);

/**
 * @brief Наибольшее допустимое количество выражений записи
 *
 */
static constexpr uint32_t MAX_EXPRESSIONS = 0x400000;

/**
 * @brief Пространство имён вспомогательных функций записи и чтения
 *
 */
namespace {
	/**
	 * @brief Функция записи восьмиразрядного числа
	 *
	 * @param value  записываемое число
	 * @param result запись хранилища
	 *
	 */
	inline void write8(const uint8_t value, string & result) noexcept {
		// Выполняем запись числа
		result.push_back(static_cast <char> (value));
	}
	/**
	 * @brief Функция записи шестнадцатиразрядного числа
	 *
	 * @param value  записываемое число
	 * @param result запись хранилища
	 *
	 */
	inline void write16(const uint16_t value, string & result) noexcept {
		/**
		 * Выполняем запись числа байтами от младшего
		 */
		for(uint8_t shift = 0; shift < 16; shift += 8)
			// Выполняем запись очередного байта числа
			result.push_back(static_cast <char> ((value >> shift) & 0xFF));
	}
	/**
	 * @brief Функция записи тридцатидвухразрядного числа
	 *
	 * @param value  записываемое число
	 * @param result запись хранилища
	 *
	 */
	inline void write32(const uint32_t value, string & result) noexcept {
		/**
		 * Выполняем запись числа байтами от младшего
		 */
		for(uint8_t shift = 0; shift < 32; shift += 8)
			// Выполняем запись очередного байта числа
			result.push_back(static_cast <char> ((value >> shift) & 0xFF));
	}
	/**
	 * @brief Функция записи шестидесятичетырёхразрядного числа
	 *
	 * @param value  записываемое число
	 * @param result запись хранилища
	 *
	 */
	inline void write64(const uint64_t value, string & result) noexcept {
		/**
		 * Выполняем запись числа байтами от младшего
		 */
		for(uint8_t shift = 0; shift < 64; shift += 8)
			// Выполняем запись очередного байта числа
			result.push_back(static_cast <char> ((value >> shift) & 0xFF));
	}
	/**
	 * @brief Функция записи числа переменной длины
	 *
	 * @param value  записываемое число
	 * @param result запись хранилища
	 *
	 * @details Число записывается семью разрядами на байт, старший разряд байта
	 *          отмечает продолжение. Значения малые - а таковы почти все адреса,
	 *          номера и границы диапазонов - занимают один-два байта взамен
	 *          четырёх, отчего запись сокращается втрое, а чтение ускоряется
	 *          соразмерно объёму прочитанного.
	 *
	 */
	inline void writeVar(uint32_t value, string & result) noexcept {
		/**
		 * Выполняем запись числа семиразрядными долями
		 */
		while(value >= 0x80) {
			// Выполняем запись очередной доли числа с отметкой продолжения
			result.push_back(static_cast <char> ((value & 0x7F) | 0x80));
			// Выполняем сдвиг числа к следующей доле
			value >>= 7;
		}
		// Выполняем запись последней доли числа
		result.push_back(static_cast <char> (value));
	}
	/**
	 * @brief Функция чтения числа переменной длины
	 *
	 * @param data   запись хранилища
	 * @param offset позиция чтения записи
	 * @param value  прочитанное число
	 * @return       результат чтения числа
	 *
	 */
	inline bool readVar(string_view data, size_t & offset, uint32_t & value) noexcept {
		// Выполняем сброс прочитанного числа
		value = 0;
		/**
		 * Выполняем чтение числа семиразрядными долями
		 */
		for(uint8_t shift = 0; shift < 35; shift += 7) {
			/**
			 * Если запись оборвана до завершения числа
			 */
			if(offset >= data.size())
				// Выводим результат чтения числа
				return false;
			// Получаем очередной байт числа
			const uint8_t letter = static_cast <uint8_t> (data[offset++]);
			/**
			 * Если доля числа разрядность его превышает
			 */
			if((shift == 28) && ((letter & 0x7F) > 0x0F))
				// Выводим результат чтения числа
				return false;
			// Выполняем добавление доли числа
			value |= (static_cast <uint32_t> (letter & 0x7F) << shift);
			/**
			 * Если байт продолжения не отмечает
			 */
			if((letter & 0x80) == 0)
				// Выводим результат чтения числа
				return true;
		}
		// Выводим результат чтения числа
		return false;
	}
	/**
	 * @brief Функция чтения числа переменной длины без проверки границ
	 *
	 * @param data   запись хранилища
	 * @param offset позиция чтения записи
	 * @return       прочитанное число
	 *
	 * @details Проверка границ вынесена наружу и выполняется единожды на блок
	 *          записей: она обходится дороже самого чтения, а блок известной
	 *          длины проверяется одним сравнением. Вызывать эту функцию
	 *          дозволено лишь после проверки, что в записи довольно байтов на
	 *          весь блок с запасом наибольшей длины числа.
	 *
	 */
	inline uint32_t readFast(string_view data, size_t & offset) noexcept {
		// Получаем первый байт числа
		uint8_t letter = static_cast <uint8_t> (data[offset++]);
		/**
		 * Если число уместилось в один байт
		 */
		if((letter & 0x80) == 0)
			// Выводим прочитанное число
			return static_cast <uint32_t> (letter);
		// Накопленное значение числа
		uint32_t result = static_cast <uint32_t> (letter & 0x7F);
		/**
		 * Выполняем чтение оставшихся долей числа
		 */
		for(uint8_t shift = 7; shift < 35; shift += 7) {
			// Получаем очередной байт числа
			letter = static_cast <uint8_t> (data[offset++]);
			// Выполняем добавление доли числа
			result |= (static_cast <uint32_t> (letter & 0x7F) << shift);
			/**
			 * Если байт продолжения не отмечает
			 */
			if((letter & 0x80) == 0)
				// Выводим прочитанное число
				return result;
		}
		// Выводим прочитанное число
		return result;
	}
	/**
	 * @brief Функция записи последовательности символов
	 *
	 * @param value  записываемая последовательность
	 * @param result запись хранилища
	 *
	 */
	inline void writeText(const string & value, string & result) noexcept {
		// Выполняем запись длины последовательности
		writeVar(static_cast <uint32_t> (value.size()), result);
		// Выполняем запись содержимого последовательности
		result.append(value);
	}
	/**
	 * @brief Функция чтения восьмиразрядного числа
	 *
	 * @param data   запись хранилища
	 * @param offset позиция чтения записи
	 * @param value  прочитанное число
	 * @return       результат чтения числа
	 *
	 */
	inline bool read8(string_view data, size_t & offset, uint8_t & value) noexcept {
		/**
		 * Если запись оборвана до завершения числа
		 */
		if((offset + 1) > data.size())
			// Выводим результат чтения числа
			return false;
		// Выполняем чтение числа
		value = static_cast <uint8_t> (data[offset++]);
		// Выводим результат чтения числа
		return true;
	}
	/**
	 * @brief Функция чтения шестнадцатиразрядного числа
	 *
	 * @param data   запись хранилища
	 * @param offset позиция чтения записи
	 * @param value  прочитанное число
	 * @return       результат чтения числа
	 *
	 */
	inline bool read16(string_view data, size_t & offset, uint16_t & value) noexcept {
		/**
		 * Если запись оборвана до завершения числа
		 */
		if((offset + 2) > data.size())
			// Выводим результат чтения числа
			return false;
		// Выполняем сброс прочитанного числа
		value = 0;
		/**
		 * Выполняем чтение числа байтами от младшего
		 */
		for(uint8_t shift = 0; shift < 16; shift += 8)
			// Выполняем чтение очередного байта числа
			value |= (static_cast <uint16_t> (static_cast <uint8_t> (data[offset++])) << shift);
		// Выводим результат чтения числа
		return true;
	}
	/**
	 * @brief Функция чтения тридцатидвухразрядного числа
	 *
	 * @param data   запись хранилища
	 * @param offset позиция чтения записи
	 * @param value  прочитанное число
	 * @return       результат чтения числа
	 *
	 */
	inline bool read32(string_view data, size_t & offset, uint32_t & value) noexcept {
		/**
		 * Если запись оборвана до завершения числа
		 */
		if((offset + 4) > data.size())
			// Выводим результат чтения числа
			return false;
		// Выполняем сброс прочитанного числа
		value = 0;
		/**
		 * Выполняем чтение числа байтами от младшего
		 */
		for(uint8_t shift = 0; shift < 32; shift += 8)
			// Выполняем чтение очередного байта числа
			value |= (static_cast <uint32_t> (static_cast <uint8_t> (data[offset++])) << shift);
		// Выводим результат чтения числа
		return true;
	}
	/**
	 * @brief Функция чтения шестидесятичетырёхразрядного числа
	 *
	 * @param data   запись хранилища
	 * @param offset позиция чтения записи
	 * @param value  прочитанное число
	 * @return       результат чтения числа
	 *
	 */
	inline bool read64(string_view data, size_t & offset, uint64_t & value) noexcept {
		/**
		 * Если запись оборвана до завершения числа
		 */
		if((offset + 8) > data.size())
			// Выводим результат чтения числа
			return false;
		// Выполняем сброс прочитанного числа
		value = 0;
		/**
		 * Выполняем чтение числа байтами от младшего
		 */
		for(uint8_t shift = 0; shift < 64; shift += 8)
			// Выполняем чтение очередного байта числа
			value |= (static_cast <uint64_t> (static_cast <uint8_t> (data[offset++])) << shift);
		// Выводим результат чтения числа
		return true;
	}
	/**
	 * @brief Функция чтения последовательности символов
	 *
	 * @param data   запись хранилища
	 * @param offset позиция чтения записи
	 * @param value  прочитанная последовательность
	 * @return       результат чтения последовательности
	 *
	 */
	inline bool readText(string_view data, size_t & offset, string & value) noexcept {
		// Длина прочитанной последовательности
		uint32_t length = 0;
		/**
		 * Если чтение длины последовательности не выполнено
		 */
		if(!readVar(data, offset, length))
			// Выводим результат чтения последовательности
			return false;
		/**
		 * Если запись оборвана до завершения последовательности
		 */
		if((offset + static_cast <size_t> (length)) > data.size())
			// Выводим результат чтения последовательности
			return false;
		// Выполняем чтение содержимого последовательности
		value.assign(data.substr(offset, length));
		// Выполняем перенос позиции чтения записи
		offset += static_cast <size_t> (length);
		// Выводим результат чтения последовательности
		return true;
	}
	/**
	 * @brief Функция вычисления контрольной суммы записи
	 *
	 * @param data запись хранилища
	 * @return     вычисленная контрольная сумма записи
	 *
	 * @details Сумма служит обнаружению порчи записи, а не защите от подмены
	 *          намеренной: восстановление проверяет содержимое поля за полем и
	 *          на сумму в этом не полагается.
	 *
	 */
	inline uint64_t checksum(string_view data) noexcept {
		// Накопленная контрольная сумма записи
		uint64_t result = 0xCBF29CE484222325ull;
		// Получаем позицию чтения записи
		size_t offset = 0;
		/**
		 * Выполняем перебор записи восьмибайтовыми долями
		 *
		 * @details Сумма считается словами, а не байтами: запись достигает
		 *          мегабайтов, и побайтовый обход её обходился дороже разбора
		 *          всего содержимого. Доли собираются из байтов явно, а не
		 *          переносом памяти, дабы значение суммы не зависело от порядка
		 *          байтов машины и запись оставалась переносимой.
		 */
		for(; (offset + 8) <= data.size(); offset += 8) {
			// Собираемая доля записи
			uint64_t block = 0;
			/**
			 * Выполняем сборку доли записи байтами от младшего
			 */
			for(uint8_t shift = 0; shift < 64; shift += 8)
				// Выполняем добавление очередного байта доли
				block |= (static_cast <uint64_t> (static_cast <uint8_t> (data[offset + (shift >> 3)])) << shift);
			// Выполняем смешивание доли записи
			result ^= block;
			// Выполняем умножение накопленной суммы
			result *= 0x100000001B3ull;
			// Выполняем перемешивание накопленной суммы
			result ^= (result >> 29);
		}
		/**
		 * Выполняем перебор остатка записи побайтно
		 */
		for(; offset < data.size(); offset++) {
			// Выполняем смешивание очередного байта записи
			result ^= static_cast <uint64_t> (static_cast <uint8_t> (data[offset]));
			// Выполняем умножение накопленной суммы
			result *= 0x100000001B3ull;
		}
		// Выводим вычисленную контрольную сумму записи
		return result;
	}
};

/**
 * @brief Метод записи программы регулярного выражения
 *
 * @param program записываемая программа
 * @param result  запись хранилища
 *
 */
void awh::regex::Storage::save(const program_t & program, string & result) const noexcept {
	// Выполняем запись опознания программы
	write64(program.id, result);
	// Выполняем запись количества захватывающих групп
	writeVar(program.captures, result);
	// Выполняем запись количества ячеек состояния
	writeVar(program.cells, result);
	// Выполняем запись набора режимов компиляции
	writeVar(program.flags, result);
	// Выполняем запись признаков программы
	write8(static_cast <uint8_t> (program.plain ? 1 : 0), result);
	write8(static_cast <uint8_t> (program.sweeping ? 1 : 0), result);
	write8(static_cast <uint8_t> (program.anchored ? 1 : 0), result);
	// Выполняем запись последовательности символов выражения
	writeText(program.text, result);
	// Выполняем запись количества инструкций программы
	writeVar(static_cast <uint32_t> (program.instructions.size()), result);
	/**
	 * Выполняем перебор набора инструкций программы
	 *
	 * @details Операнды инструкции хранятся объединением, поэтому записываются
	 *          наибольшим из его составов: набор полей одинаков для всех кодов
	 *          операций, отчего запись не зависит от размера объединения и от
	 *          размещения полей в нём.
	 */
	for(const auto & instruction : program.instructions) {
		/**
		 * Выполняем запись кода операции инструкции
		 *
		 * @details Набор режимов у подавляющего большинства инструкций совпадает
		 *          с набором режимов самой программы, поэтому записывается он
		 *          лишь при расхождении, а признак расхождения несёт старший
		 *          разряд кода операции: коды не превышают 0x14, и разряд этот
		 *          свободен. Экономия - байт на инструкцию и чтение числа на
		 *          каждую из них.
		 */
		if(instruction.flags == program.flags)
			// Выполняем запись кода операции инструкции
			write8(static_cast <uint8_t> (instruction.type), result);
		/**
		 * Если набор режимов инструкции набору режимов программы не отвечает
		 */
		else {
			// Выполняем запись кода операции инструкции с признаком расхождения
			write8(static_cast <uint8_t> (static_cast <uint8_t> (instruction.type) | 0x80), result);
			// Выполняем запись набора режимов инструкции
			writeVar(instruction.flags, result);
		}
		/**
		 * Определяем код операции инструкции программы
		 */
		switch(static_cast <uint8_t> (instruction.type)) {
			// Выполняем запись операндов сопоставления одиночного символа
			case static_cast <uint8_t> (opcode_t::CHAR):
				writeVar(instruction.letter.code, result);
			break;
			// Выполняем запись операндов сопоставления символа из класса
			case static_cast <uint8_t> (opcode_t::CLASS):
				writeVar(instruction.charclass.index, result);
			break;
			// Выполняем запись операндов перехода по двум ветвям
			case static_cast <uint8_t> (opcode_t::SPLIT):
				writeVar(instruction.split.first, result);
				writeVar(instruction.split.second, result);
			break;
			// Выполняем запись операндов безусловного перехода
			case static_cast <uint8_t> (opcode_t::JUMP):
				writeVar(instruction.jump.target, result);
			break;
			// Выполняем запись операндов запоминания границы захвата
			case static_cast <uint8_t> (opcode_t::SAVE):
				writeVar(instruction.save.slot, result);
			break;
			// Выполняем запись операндов привязки к позиции в тексте
			case static_cast <uint8_t> (opcode_t::ANCHOR):
				write8(static_cast <uint8_t> (instruction.assertion.type), result);
			break;
			// Выполняем запись операндов отметки и отката точек возврата
			case static_cast <uint8_t> (opcode_t::MARK):
			case static_cast <uint8_t> (opcode_t::CUT):
				writeVar(instruction.atomic.cell, result);
			break;
			// Выполняем запись операндов сопоставления захваченного текста
			case static_cast <uint8_t> (opcode_t::BACKREF):
				writeVar(instruction.backref.number, result);
			break;
			// Выполняем запись операндов проверки продвижения по тексту
			case static_cast <uint8_t> (opcode_t::PROGRESS):
				writeVar(instruction.progress.cell, result);
				writeVar(instruction.progress.target, result);
			break;
			// Выполняем запись операндов проверки окружения
			case static_cast <uint8_t> (opcode_t::LOOK):
				writeVar(instruction.look.body, result);
				writeVar(instruction.look.target, result);
				writeVar(instruction.look.least, result);
				writeVar(instruction.look.most, result);
				writeVar(instruction.look.alternate, result);
				write8(static_cast <uint8_t> (instruction.look.negative ? 1 : 0), result);
				write8(static_cast <uint8_t> (instruction.look.backward ? 1 : 0), result);
			break;
			// Выполняем запись операндов рекурсивного вызова подвыражения
			case static_cast <uint8_t> (opcode_t::CALL):
				writeVar(instruction.call.body, result);
				writeVar(instruction.call.number, result);
			break;
			// Выполняем запись операндов условного выражения
			case static_cast <uint8_t> (opcode_t::CONDITION):
				write8(static_cast <uint8_t> (instruction.condition.type), result);
				writeVar(instruction.condition.number, result);
				writeVar(instruction.condition.positive, result);
				writeVar(instruction.condition.negative, result);
			break;
			/**
			 * Если код операции операндов не несёт
			 *
			 * @details Операнды записываются полем неиспользуемым, дабы запись
			 *          оставалась одинаковой длины при всяком коде операции и
			 *          чтение её не зависело от полноты перечисления кодов.
			 */
			default: writeVar(instruction.letter.code, result);
		}
	}
	// Выполняем запись количества ссылок на классы символов
	writeVar(static_cast <uint32_t> (program.classes.size()), result);
	/**
	 * Выполняем перебор хранилища ссылок на классы символов
	 *
	 * @details Ссылка несёт лишь количества: номера первых записей выводятся
	 *          при восстановлении накоплением, поскольку участки следуют в
	 *          наборах подряд и в том же порядке, что и ссылки на них.
	 */
	for(const auto & item : program.classes) {
		// Выполняем запись признака отрицания класса символов
		write8(static_cast <uint8_t> (item.negative ? 1 : 0), result);
		// Выполняем запись количества диапазонов класса символов
		writeVar(item.rangeCount, result);
		// Выполняем запись количества свойств Юникода класса символов
		writeVar(item.propertyCount, result);
	}
	// Выполняем запись количества диапазонов кодовых значений
	writeVar(static_cast <uint32_t> (program.ranges.size()), result);
	/**
	 * Выполняем перебор сплошного набора диапазонов кодовых значений
	 */
	for(const auto & range : program.ranges) {
		// Выполняем запись нижней границы диапазона
		writeVar(range.begin, result);
		// Выполняем запись верхней границы диапазона
		writeVar(range.end, result);
	}
	// Выполняем запись количества свойств Юникода
	writeVar(static_cast <uint32_t> (program.properties.size()), result);
	/**
	 * Выполняем перебор сплошного набора свойств Юникода
	 */
	for(const auto & property : program.properties) {
		// Выполняем запись идентификатора свойства Юникода
		write16(property.id, result);
		// Выполняем запись признака отрицания свойства Юникода
		write8(static_cast <uint8_t> (property.negative ? 1 : 0), result);
	}
	// Выполняем запись количества последовательностей символов
	writeVar(static_cast <uint32_t> (program.strings.size()), result);
	/**
	 * Выполняем перебор хранилища последовательностей символов
	 */
	for(const auto & code : program.strings)
		// Выполняем запись кодового значения символа
		writeVar(code, result);
	// Выполняем запись количества адресов тел повторений
	writeVar(static_cast <uint32_t> (program.runs.size()), result);
	/**
	 * Выполняем перебор набора адресов тел повторений
	 */
	for(const auto & address : program.runs)
		// Выполняем запись адреса тела повторения
		writeVar(address, result);
	// Выполняем запись количества адресов тел ленивых повторений
	writeVar(static_cast <uint32_t> (program.lazy.size()), result);
	/**
	 * Выполняем перебор набора адресов тел ленивых повторений
	 */
	for(const auto & address : program.lazy)
		// Выполняем запись адреса тела ленивого повторения
		writeVar(address, result);
	// Выполняем запись признаков предварительного отбора позиций
	write8(static_cast <uint8_t> (program.prefilter.active ? 1 : 0), result);
	write8(static_cast <uint8_t> (program.prefilter.utf ? 1 : 0), result);
	write8(static_cast <uint8_t> (program.prefilter.unique ? 1 : 0), result);
	write8(static_cast <uint8_t> (program.prefilter.letter), result);
	/**
	 * Выполняем перебор набора допустимых начальных байтов
	 *
	 * @details Набор пишется битовой картой, а не байтом на признак: признаков
	 *          в нём двести пятьдесят шесть на каждую программу, и запись их
	 *          порознь обходилась восьмикратно дороже как местом, так и
	 *          временем чтения - на каждый признак приходилась своя проверка
	 *          границ записи.
	 */
	for(size_t i = 0; i < 256; i += 8) {
		// Собираемая доля битовой карты
		uint8_t block = 0;
		/**
		 * Выполняем сборку доли битовой карты
		 */
		for(uint8_t bit = 0; bit < 8; bit++)
			// Выполняем установку признака допустимости байта
			block |= (program.prefilter.bytes[i + bit] ? static_cast <uint8_t> (1 << bit) : 0);
		// Выполняем запись доли битовой карты
		write8(block, result);
	}
	// Выполняем запись литерала, присутствующего в любом совпадении
	writeText(program.prefilter.literal, result);
	// Выполняем запись последовательности начала любого совпадения
	writeText(program.prefilter.leading, result);
}
/**
 * @brief Метод восстановления программы регулярного выражения
 *
 * @param data    запись хранилища
 * @param offset  позиция чтения записи
 * @param program восстанавливаемая программа
 * @return        результат восстановления программы
 *
 */
bool awh::regex::Storage::load(string_view data, size_t & offset, program_t & program) const noexcept {
	// Выполняем очистку восстанавливаемой программы
	program.clear();
	// Признак программы, читаемый записью
	uint8_t flag = 0;
	/**
	 * Если чтение полей программы не выполнено
	 */
	if(!read64(data, offset, program.id) ||
	 !readVar(data, offset, program.captures) ||
	 !readVar(data, offset, program.cells) ||
	 !readVar(data, offset, program.flags)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления программы
		return false;
	}
	/**
	 * Если чтение признаков программы не выполнено
	 */
	if(!read8(data, offset, flag)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления программы
		return false;
	}
	// Выполняем установку признака выражения, сопоставляемого литералом
	program.plain = (flag != 0);
	/**
	 * Если чтение признаков программы не выполнено
	 */
	if(!read8(data, offset, flag)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления программы
		return false;
	}
	// Выполняем установку признака выражения, проходящего текст одной попыткой
	program.sweeping = (flag != 0);
	/**
	 * Если чтение признаков программы не выполнено
	 */
	if(!read8(data, offset, flag)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления программы
		return false;
	}
	// Выполняем установку признака выражения, привязанного к началу поиска
	program.anchored = (flag != 0);
	/**
	 * Если чтение последовательности символов выражения не выполнено
	 */
	if(!readText(data, offset, program.text)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления программы
		return false;
	}
	// Количество читаемых записей
	uint32_t count = 0;
	/**
	 * Если чтение количества инструкций программы не выполнено
	 */
	if(!readVar(data, offset, count)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления программы
		return false;
	}
	/**
	 * Если количество инструкций программы превышает допустимое
	 */
	if(static_cast <size_t> (count) > MAX_PROGRAM) {
		// Устанавливаем ошибку несообразного содержимого записи
		this->_error = storage_error_t::BAD_CONTENT;
		// Выводим результат восстановления программы
		return false;
	}
	// Выполняем размещение набора инструкций программы
	program.instructions.resize(static_cast <size_t> (count));
	/**
	 * Выполняем перебор набора инструкций программы
	 */
	for(auto & instruction : program.instructions) {
		// Код операции инструкции программы
		uint8_t type = 0;
		/**
		 * Если чтение кода операции инструкции не выполнено
		 */
		if(!read8(data, offset, type)) {
			// Устанавливаем ошибку обрыва записи
			this->_error = storage_error_t::TRUNCATED;
			// Выводим результат восстановления программы
			return false;
		}
		/**
		 * Если набор режимов инструкции набору режимов программы отвечает
		 */
		if((type & 0x80) == 0)
			// Выполняем установку набора режимов инструкции
			instruction.flags = program.flags;
		/**
		 * Если набор режимов инструкции записан отдельно
		 */
		else {
			// Снимаем признак расхождения набора режимов
			type &= 0x7F;
			/**
			 * Если чтение набора режимов инструкции не выполнено
			 */
			if(!readVar(data, offset, instruction.flags)) {
				// Устанавливаем ошибку обрыва записи
				this->_error = storage_error_t::TRUNCATED;
				// Выводим результат восстановления программы
				return false;
			}
		}
		/**
		 * Если код операции инструкции модулю неизвестен
		 *
		 * @details Значение 0x01 набору кодов операций не принадлежит: место
		 *          это в перечислении пустует, и запись, его несущая, испорчена.
		 */
		if((type > static_cast <uint8_t> (opcode_t::GRAPHEME)) || (type == 0x01)) {
			// Устанавливаем ошибку несообразного содержимого записи
			this->_error = storage_error_t::BAD_CONTENT;
			// Выводим результат восстановления программы
			return false;
		}
		// Выполняем установку кода операции инструкции
		instruction.type = static_cast <opcode_t> (type);
		// Признак успешности чтения операндов инструкции
		bool read = true;
		/**
		 * Определяем код операции инструкции программы
		 */
		switch(type) {
			// Выполняем чтение операндов сопоставления одиночного символа
			case static_cast <uint8_t> (opcode_t::CHAR):
				read = readVar(data, offset, instruction.letter.code);
			break;
			// Выполняем чтение операндов сопоставления символа из класса
			case static_cast <uint8_t> (opcode_t::CLASS):
				read = readVar(data, offset, instruction.charclass.index);
			break;
			// Выполняем чтение операндов перехода по двум ветвям
			case static_cast <uint8_t> (opcode_t::SPLIT):
				read = (readVar(data, offset, instruction.split.first) &&
				 readVar(data, offset, instruction.split.second));
			break;
			// Выполняем чтение операндов безусловного перехода
			case static_cast <uint8_t> (opcode_t::JUMP):
				read = readVar(data, offset, instruction.jump.target);
			break;
			// Выполняем чтение операндов запоминания границы захвата
			case static_cast <uint8_t> (opcode_t::SAVE):
				read = readVar(data, offset, instruction.save.slot);
			break;
			/**
			 * Выполняем чтение операндов привязки к позиции в тексте
			 */
			case static_cast <uint8_t> (opcode_t::ANCHOR): {
				// Тип привязки к позиции в тексте
				uint8_t anchor = 0;
				// Выполняем чтение типа привязки к позиции в тексте
				read = read8(data, offset, anchor);
				/**
				 * Если тип привязки модулю неизвестен
				 */
				if(read && (anchor > static_cast <uint8_t> (anchor_t::KEEP_OUT))) {
					// Устанавливаем ошибку несообразного содержимого записи
					this->_error = storage_error_t::BAD_CONTENT;
					// Выводим результат восстановления программы
					return false;
				}
				// Выполняем установку типа привязки к позиции в тексте
				instruction.assertion.type = static_cast <anchor_t> (anchor);
			} break;
			// Выполняем чтение операндов отметки и отката точек возврата
			case static_cast <uint8_t> (opcode_t::MARK):
			case static_cast <uint8_t> (opcode_t::CUT):
				read = readVar(data, offset, instruction.atomic.cell);
			break;
			// Выполняем чтение операндов сопоставления захваченного текста
			case static_cast <uint8_t> (opcode_t::BACKREF):
				read = readVar(data, offset, instruction.backref.number);
			break;
			// Выполняем чтение операндов проверки продвижения по тексту
			case static_cast <uint8_t> (opcode_t::PROGRESS):
				read = (readVar(data, offset, instruction.progress.cell) &&
				 readVar(data, offset, instruction.progress.target));
			break;
			/**
			 * Выполняем чтение операндов проверки окружения
			 */
			case static_cast <uint8_t> (opcode_t::LOOK): {
				// Признаки проверки окружения
				uint8_t negative = 0, backward = 0;
				// Выполняем чтение операндов проверки окружения
				read = (readVar(data, offset, instruction.look.body) &&
				 readVar(data, offset, instruction.look.target) &&
				 readVar(data, offset, instruction.look.least) &&
				 readVar(data, offset, instruction.look.most) &&
				 readVar(data, offset, instruction.look.alternate) &&
				 read8(data, offset, negative) && read8(data, offset, backward));
				// Выполняем установку признака отрицания проверки окружения
				instruction.look.negative = (negative != 0);
				// Выполняем установку признака проверки предшествующего текста
				instruction.look.backward = (backward != 0);
			} break;
			// Выполняем чтение операндов рекурсивного вызова подвыражения
			case static_cast <uint8_t> (opcode_t::CALL):
				read = (readVar(data, offset, instruction.call.body) &&
				 readVar(data, offset, instruction.call.number));
			break;
			/**
			 * Выполняем чтение операндов условного выражения
			 */
			case static_cast <uint8_t> (opcode_t::CONDITION): {
				// Тип условия условного выражения
				uint8_t test = 0;
				// Выполняем чтение операндов условного выражения
				read = (read8(data, offset, test) &&
				 readVar(data, offset, instruction.condition.number) &&
				 readVar(data, offset, instruction.condition.positive) &&
				 readVar(data, offset, instruction.condition.negative));
				/**
				 * Если тип условия модулю неизвестен
				 */
				if(read && (test > static_cast <uint8_t> (test_t::ALWAYS))) {
					// Устанавливаем ошибку несообразного содержимого записи
					this->_error = storage_error_t::BAD_CONTENT;
					// Выводим результат восстановления программы
					return false;
				}
				// Выполняем установку типа условия условного выражения
				instruction.condition.type = static_cast <test_t> (test);
			} break;
			// Выполняем чтение операндов неиспользуемых
			default: read = readVar(data, offset, instruction.letter.code);
		}
		/**
		 * Если чтение операндов инструкции не выполнено
		 */
		if(!read) {
			// Устанавливаем ошибку обрыва записи
			this->_error = storage_error_t::TRUNCATED;
			// Выводим результат восстановления программы
			return false;
		}
	}
	/**
	 * Если чтение количества классов символов не выполнено
	 */
	if(!readVar(data, offset, count)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления программы
		return false;
	}
	/**
	 * Если количество классов символов превышает допустимое
	 */
	if(static_cast <size_t> (count) > MAX_PROGRAM) {
		// Устанавливаем ошибку несообразного содержимого записи
		this->_error = storage_error_t::BAD_CONTENT;
		// Выводим результат восстановления программы
		return false;
	}
	// Выполняем размещение хранилища ссылок на классы символов
	program.classes.resize(static_cast <size_t> (count));
	// Накопленные номера первых записей участков наборов
	uint32_t rangeOffset = 0, propertyOffset = 0;
	/**
	 * Выполняем перебор хранилища ссылок на классы символов
	 */
	for(auto & item : program.classes) {
		/**
		 * Если чтение ссылки на класс символов не выполнено
		 */
		if(!read8(data, offset, flag) || !readVar(data, offset, item.rangeCount) ||
		 !readVar(data, offset, item.propertyCount)) {
			// Устанавливаем ошибку обрыва записи
			this->_error = storage_error_t::TRUNCATED;
			// Выводим результат восстановления программы
			return false;
		}
		// Выполняем установку признака отрицания класса символов
		item.negative = (flag != 0);
		/**
		 * Если количества записей участков несообразны
		 *
		 * @details Числа пишутся переменной длиной, и наименьшая запись
		 *          занимает байт, поэтому количество, в остаток записи не
		 *          помещающееся, размещения не заслуживает.
		 */
		if((static_cast <size_t> (item.rangeCount) > (data.size() - offset)) ||
		 (static_cast <size_t> (item.propertyCount) > (data.size() - offset))) {
			// Устанавливаем ошибку несообразного содержимого записи
			this->_error = storage_error_t::BAD_CONTENT;
			// Выводим результат восстановления программы
			return false;
		}
		// Выполняем установку номера первого диапазона класса
		item.ranges = rangeOffset;
		// Выполняем установку номера первого свойства класса
		item.properties = propertyOffset;
		// Выполняем накопление номера первого диапазона следующего класса
		rangeOffset += item.rangeCount;
		// Выполняем накопление номера первого свойства следующего класса
		propertyOffset += item.propertyCount;
	}
	/**
	 * Если чтение количества диапазонов кодовых значений не выполнено
	 */
	if(!readVar(data, offset, count)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления программы
		return false;
	}
	/**
	 * Если количество диапазонов ссылкам на классы не отвечает
	 */
	if(count != rangeOffset) {
		// Устанавливаем ошибку несообразного содержимого записи
		this->_error = storage_error_t::BAD_CONTENT;
		// Выводим результат восстановления программы
		return false;
	}
	/**
	 * Если запись оборвана до завершения набора диапазонов
	 *
	 * @details Проверка выполняется единожды на весь набор, по запасу
	 *          наибольшей длины числа, после чего чтение идёт без проверки
	 *          границ на каждом числе: проверка эта обходится дороже самого
	 *          чтения, а набор диапазонов - место, где чисел больше всего.
	 */
	if((static_cast <size_t> (count) * 10) > (data.size() - offset)) {
		// Выполняем размещение сплошного набора диапазонов
		program.ranges.reserve(static_cast <size_t> (count));
		/**
		 * Выполняем чтение набора диапазонов с проверкой границ
		 */
		for(uint32_t i = 0; i < count; i++) {
			// Границы диапазона кодовых значений символов
			uint32_t begin = 0, end = 0;
			/**
			 * Если чтение границ диапазона не выполнено
			 */
			if(!readVar(data, offset, begin) || !readVar(data, offset, end)) {
				// Устанавливаем ошибку обрыва записи
				this->_error = storage_error_t::TRUNCATED;
				// Выводим результат восстановления программы
				return false;
			}
			// Выполняем добавление диапазона в сплошной набор
			program.ranges.emplace_back(begin, end);
		}
	/**
	 * Если запись несёт весь набор диапазонов целиком
	 */
	} else {
		// Выполняем размещение сплошного набора диапазонов
		program.ranges.resize(static_cast <size_t> (count));
		/**
		 * Выполняем перебор сплошного набора диапазонов
		 */
		for(auto & range : program.ranges) {
			// Выполняем чтение нижней границы диапазона
			range.begin = readFast(data, offset);
			// Выполняем чтение верхней границы диапазона
			range.end = readFast(data, offset);
		}
	}
	/**
	 * Если чтение количества свойств Юникода не выполнено
	 */
	if(!readVar(data, offset, count)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления программы
		return false;
	}
	/**
	 * Если количество свойств ссылкам на классы не отвечает
	 */
	if(count != propertyOffset) {
		// Устанавливаем ошибку несообразного содержимого записи
		this->_error = storage_error_t::BAD_CONTENT;
		// Выводим результат восстановления программы
		return false;
	}
	/**
	 * Если количество свойств превышает размер оставшейся записи
	 */
	if((static_cast <size_t> (count) * 3) > (data.size() - offset)) {
		// Устанавливаем ошибку несообразного содержимого записи
		this->_error = storage_error_t::BAD_CONTENT;
		// Выводим результат восстановления программы
		return false;
	}
	// Выполняем размещение сплошного набора свойств Юникода
	program.properties.reserve(static_cast <size_t> (count));
	/**
	 * Выполняем перебор сплошного набора свойств Юникода
	 */
	for(uint32_t i = 0; i < count; i++) {
		// Идентификатор свойства Юникода
		uint16_t id = 0;
		/**
		 * Если чтение свойства Юникода не выполнено
		 */
		if(!read16(data, offset, id) || !read8(data, offset, flag)) {
			// Устанавливаем ошибку обрыва записи
			this->_error = storage_error_t::TRUNCATED;
			// Выводим результат восстановления программы
			return false;
		}
		// Выполняем добавление свойства Юникода в сплошной набор
		program.properties.emplace_back(id, (flag != 0));
	}
	/**
	 * Если чтение количества последовательностей символов не выполнено
	 */
	if(!readVar(data, offset, count)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления программы
		return false;
	}
	/**
	 * Если количество кодовых значений превышает размер оставшейся записи
	 */
	if(static_cast <size_t> (count) > (data.size() - offset)) {
		// Устанавливаем ошибку несообразного содержимого записи
		this->_error = storage_error_t::BAD_CONTENT;
		// Выводим результат восстановления программы
		return false;
	}
	// Выполняем размещение хранилища последовательностей символов
	program.strings.resize(static_cast <size_t> (count));
	/**
	 * Выполняем перебор хранилища последовательностей символов
	 */
	for(auto & code : program.strings) {
		/**
		 * Если чтение кодового значения символа не выполнено
		 */
		if(!readVar(data, offset, code)) {
			// Устанавливаем ошибку обрыва записи
			this->_error = storage_error_t::TRUNCATED;
			// Выводим результат восстановления программы
			return false;
		}
	}
	/**
	 * Выполняем перебор наборов адресов тел повторений
	 */
	for(uint8_t pass = 0; pass < 2; pass++) {
		/**
		 * Если чтение количества адресов не выполнено
		 */
		if(!readVar(data, offset, count)) {
			// Устанавливаем ошибку обрыва записи
			this->_error = storage_error_t::TRUNCATED;
			// Выводим результат восстановления программы
			return false;
		}
		/**
		 * Если количество адресов размеру программы не отвечает
		 *
		 * @details Наборы помечают инструкции программы и потому либо пусты,
		 *          либо размером с набор инструкций.
		 */
		if((count != 0) && (static_cast <size_t> (count) != program.instructions.size())) {
			// Устанавливаем ошибку несообразного содержимого записи
			this->_error = storage_error_t::BAD_CONTENT;
			// Выводим результат восстановления программы
			return false;
		}
		// Получаем набор адресов тел повторений
		auto & addresses = ((pass == 0) ? program.runs : program.lazy);
		// Выполняем размещение набора адресов тел повторений
		addresses.resize(static_cast <size_t> (count));
		/**
		 * Выполняем перебор набора адресов тел повторений
		 */
		for(auto & address : addresses) {
			/**
			 * Если чтение адреса тела повторения не выполнено
			 */
			if(!readVar(data, offset, address)) {
				// Устанавливаем ошибку обрыва записи
				this->_error = storage_error_t::TRUNCATED;
				// Выводим результат восстановления программы
				return false;
			}
		}
	}
	// Признаки предварительного отбора позиций
	uint8_t active = 0, utf = 0, unique = 0, letter = 0;
	/**
	 * Если чтение признаков предварительного отбора не выполнено
	 */
	if(!read8(data, offset, active) || !read8(data, offset, utf) ||
	 !read8(data, offset, unique) || !read8(data, offset, letter)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления программы
		return false;
	}
	// Выполняем установку признаков предварительного отбора позиций
	program.prefilter.active = (active != 0);
	program.prefilter.utf = (utf != 0);
	program.prefilter.unique = (unique != 0);
	program.prefilter.letter = static_cast <char> (letter);
	/**
	 * Если запись оборвана до завершения битовой карты отбора
	 */
	if((offset + 32) > data.size()) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления программы
		return false;
	}
	/**
	 * Выполняем перебор набора допустимых начальных байтов
	 */
	for(size_t i = 0; i < 256; i += 8) {
		// Получаем очередную долю битовой карты
		const uint8_t block = static_cast <uint8_t> (data[offset++]);
		/**
		 * Выполняем разбор доли битовой карты
		 */
		for(uint8_t bit = 0; bit < 8; bit++)
			// Выполняем установку признака допустимости байта
			program.prefilter.bytes[i + bit] = ((block & (1 << bit)) != 0);
	}
	/**
	 * Если чтение последовательностей предварительного отбора не выполнено
	 */
	if(!readText(data, offset, program.prefilter.literal) ||
	 !readText(data, offset, program.prefilter.leading)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления программы
		return false;
	}
	/**
	 * Если правильность восстановленной программы не подтверждена
	 */
	if(!this->verify(program)) {
		// Устанавливаем ошибку несообразного содержимого записи
		this->_error = storage_error_t::BAD_CONTENT;
		// Выводим результат восстановления программы
		return false;
	}
	// Выводим результат восстановления программы
	return true;
}
/**
 * @brief Метод проверки правильности программы регулярного выражения
 *
 * @param program проверяемая программа
 * @return        результат проверки правильности программы
 *
 */
bool awh::regex::Storage::verify(const program_t & program) const noexcept {
	// Получаем количество инструкций программы
	const size_t count = program.instructions.size();
	/**
	 * @brief Проверка принадлежности адреса программе
	 *
	 * @param address проверяемый адрес инструкции
	 * @return        результат проверки принадлежности адреса программе
	 *
	 */
	auto inside = [&](const address_t address) noexcept -> bool {
		// Выводим результат проверки принадлежности адреса программе
		return ((address == INVALID_ADDRESS) || (static_cast <size_t> (address) < count));
	};
	/**
	 * Выполняем перебор набора инструкций программы
	 */
	for(const auto & instruction : program.instructions) {
		/**
		 * Определяем код операции инструкции программы
		 */
		switch(static_cast <uint8_t> (instruction.type)) {
			/**
			 * Если инструкция сопоставляет символ из класса символов
			 */
			case static_cast <uint8_t> (opcode_t::CLASS): {
				/**
				 * Если указание на класс символов хранилищу не принадлежит
				 */
				if(static_cast <size_t> (instruction.charclass.index) >= program.classes.size())
					// Выводим результат проверки правильности программы
					return false;
			} break;
			/**
			 * Если инструкция выполняет переход по двум ветвям
			 */
			case static_cast <uint8_t> (opcode_t::SPLIT): {
				/**
				 * Если адреса ветвей программе не принадлежат
				 */
				if(!inside(instruction.split.first) || !inside(instruction.split.second))
					// Выводим результат проверки правильности программы
					return false;
			} break;
			/**
			 * Если инструкция выполняет безусловный переход
			 */
			case static_cast <uint8_t> (opcode_t::JUMP): {
				/**
				 * Если адрес перехода программе не принадлежит
				 */
				if(!inside(instruction.jump.target))
					// Выводим результат проверки правильности программы
					return false;
			} break;
			/**
			 * Если инструкция запоминает границу захвата
			 */
			case static_cast <uint8_t> (opcode_t::SAVE): {
				/**
				 * Если номер места границы захвата выражению не принадлежит
				 *
				 * @details Набор мест несёт границы захвата и ячейки состояния
				 *          подряд, а инструкция сохранения позиции пишет и в те,
				 *          и в другие: атомарная группа сохраняет позицию начала
				 *          в ячейке, а не в границе захвата. Поэтому проверяется
				 *          принадлежность всему набору мест, а не одним лишь
				 *          границам захвата.
				 */
				if(static_cast <size_t> (instruction.save.slot) >=
				 (((static_cast <size_t> (program.captures) + 1) * 2) + static_cast <size_t> (program.cells)))
					// Выводим результат проверки правильности программы
					return false;
			} break;
			/**
			 * Если инструкция отмечает либо откатывает точки возврата
			 */
			case static_cast <uint8_t> (opcode_t::MARK):
			case static_cast <uint8_t> (opcode_t::CUT): {
				/**
				 * Если номер ячейки отметки несообразен
				 *
				 * @details Ячейки отметок атомарных групп ведутся счётчиком своим,
				 *          отдельным от ячеек состояния повторений, и в программе
				 *          количество их не хранится: исполнение с возвратом
				 *          наращивает набор отметок по мере надобности. Поэтому
				 *          проверяется лишь сообразность номера - отметок в
				 *          выражении не более, чем инструкций.
				 */
				if(static_cast <size_t> (instruction.atomic.cell) >= MAX_PROGRAM)
					// Выводим результат проверки правильности программы
					return false;
			} break;
			/**
			 * Если инструкция сопоставляет захваченный текст
			 */
			case static_cast <uint8_t> (opcode_t::BACKREF): {
				/**
				 * Если номер группы выражению не принадлежит
				 */
				if(instruction.backref.number > program.captures)
					// Выводим результат проверки правильности программы
					return false;
			} break;
			/**
			 * Если инструкция проверяет продвижение по тексту
			 */
			case static_cast <uint8_t> (opcode_t::PROGRESS): {
				/**
				 * Если номер ячейки либо адрес завершения не принадлежат выражению
				 *
				 * @details Ячейки состояния повторений размещаются в наборе мест
				 *          следом за границами захвата, поэтому номер ячейки
				 *          отсчитывается от размера набора границ.
				 */
				if((static_cast <size_t> (instruction.progress.cell) >=
				 (((static_cast <size_t> (program.captures) + 1) * 2) + static_cast <size_t> (program.cells))) ||
				 !inside(instruction.progress.target))
					// Выводим результат проверки правильности программы
					return false;
			} break;
			/**
			 * Если инструкция выполняет проверку окружения
			 */
			case static_cast <uint8_t> (opcode_t::LOOK): {
				/**
				 * Если адреса проверки окружения программе не принадлежат
				 */
				if(!inside(instruction.look.body) || !inside(instruction.look.target) ||
				 !inside(instruction.look.alternate))
					// Выводим результат проверки правильности программы
					return false;
				/**
				 * Если границы длины сопоставляемого текста несообразны
				 */
				if(instruction.look.least > instruction.look.most)
					// Выводим результат проверки правильности программы
					return false;
			} break;
			/**
			 * Если инструкция выполняет рекурсивный вызов подвыражения
			 */
			case static_cast <uint8_t> (opcode_t::CALL): {
				/**
				 * Если адрес тела либо номер группы выражению не принадлежат
				 */
				if(!inside(instruction.call.body) || (instruction.call.number > program.captures))
					// Выводим результат проверки правильности программы
					return false;
			} break;
			/**
			 * Если инструкция выполняет условное выражение
			 */
			case static_cast <uint8_t> (opcode_t::CONDITION): {
				/**
				 * Если адреса ветвей условия программе не принадлежат
				 */
				if(!inside(instruction.condition.positive) || !inside(instruction.condition.negative))
					// Выводим результат проверки правильности программы
					return false;
				/**
				 * Если номер группы условия выражению не принадлежит
				 */
				if((instruction.condition.type == test_t::CAPTURED) &&
				 (instruction.condition.number > program.captures))
					// Выводим результат проверки правильности программы
					return false;
			} break;
		}
	}
	/**
	 * Выполняем перебор наборов адресов тел повторений
	 */
	for(uint8_t pass = 0; pass < 2; pass++) {
		// Получаем набор адресов тел повторений
		const auto & addresses = ((pass == 0) ? program.runs : program.lazy);
		/**
		 * Выполняем перебор набора адресов тел повторений
		 */
		for(const auto & address : addresses) {
			/**
			 * Если адрес тела повторения программе не принадлежит
			 */
			if(!inside(address))
				// Выводим результат проверки правильности программы
				return false;
		}
	}
	/**
	 * Если количество ячеек состояния превышает допустимое
	 */
	if(static_cast <size_t> (program.cells) > MAX_PROGRAM)
		// Выводим результат проверки правильности программы
		return false;
	/**
	 * Если количество захватывающих групп превышает допустимое
	 */
	if(static_cast <size_t> (program.captures) > MAX_PROGRAM)
		// Выводим результат проверки правильности программы
		return false;
	// Выводим результат проверки правильности программы
	return true;
}
/**
 * @brief Метод записи собранных выражений
 *
 * @param expressions набор собранных выражений
 * @param result      запись хранилища
 * @return            результат записи собранных выражений
 *
 */
bool awh::regex::Storage::save(const vector <exp_t> & expressions, string & result) const noexcept {
	// Выполняем очистку записи хранилища
	result.clear();
	// Выполняем сброс кода ошибки хранилища
	this->_error = storage_error_t::NONE;
	/**
	 * Если количество записываемых выражений превышает допустимое
	 */
	if(expressions.size() > static_cast <size_t> (MAX_EXPRESSIONS)) {
		// Устанавливаем ошибку превышения размера записи
		this->_error = storage_error_t::TOO_LARGE;
		// Выводим результат записи собранных выражений
		return false;
	}
	// Содержимое записи хранилища
	string payload;
	// Выполняем запись количества собранных выражений
	writeVar(static_cast <uint32_t> (expressions.size()), payload);
	/**
	 * Выполняем перебор набора собранных выражений
	 */
	for(const auto & expression : expressions) {
		/**
		 * Если собранное выражение не установлено
		 */
		if(!expression) {
			// Устанавливаем ошибку несообразного содержимого записи
			this->_error = storage_error_t::BAD_CONTENT;
			// Выводим результат записи собранных выражений
			return false;
		}
		// Выполняем запись признаков собранного выражения
		write8(static_cast <uint8_t> (expression->backtracking ? 1 : 0), payload);
		write8(static_cast <uint8_t> (expression->ready ? 1 : 0), payload);
		write8(static_cast <uint8_t> (expression->reversible ? 1 : 0), payload);
		// Выполняем запись программы сопоставления в прямом направлении
		this->save(expression->forward, payload);
		// Выполняем запись программы сопоставления в обратном направлении
		this->save(expression->backward, payload);
		// Выполняем запись количества именованных групп выражения
		writeVar(static_cast <uint32_t> (expression->names.size()), payload);
		/**
		 * Выполняем перебор соответствия имён именованных групп
		 */
		for(const auto & item : expression->names) {
			// Выполняем запись имени именованной группы
			writeText(item.first, payload);
			// Выполняем запись количества номеров именованной группы
			writeVar(static_cast <uint32_t> (item.second.size()), payload);
			/**
			 * Выполняем перебор набора номеров именованной группы
			 */
			for(const auto & number : item.second)
				// Выполняем запись номера именованной группы
				writeVar(number, payload);
		}
	}
	/**
	 * Если размер записи хранилища превышает допустимый
	 */
	if(payload.size() > MAX_STORAGE) {
		// Устанавливаем ошибку превышения размера записи
		this->_error = storage_error_t::TOO_LARGE;
		// Выводим результат записи собранных выражений
		return false;
	}
	// Выполняем размещение записи хранилища
	result.reserve(payload.size() + 32);
	// Выполняем запись опознания записи хранилища
	write64(STORAGE_MAGIC, result);
	// Выполняем запись версии устройства записи
	write16(STORAGE_VERSION, result);
	// Выполняем запись размера содержимого записи
	write64(static_cast <uint64_t> (payload.size()), result);
	// Выполняем запись контрольной суммы содержимого
	write64(checksum(payload), result);
	// Выполняем запись содержимого записи хранилища
	result.append(payload);
	// Выводим результат записи собранных выражений
	return true;
}
/**
 * @brief Метод восстановления собранных выражений
 *
 * @param data   запись хранилища
 * @param result набор восстановленных выражений
 * @return       результат восстановления собранных выражений
 *
 */
bool awh::regex::Storage::load(string_view data, vector <exp_t> & result) const noexcept {
	// Выполняем очистку набора восстановленных выражений
	result.clear();
	// Выполняем сброс кода ошибки хранилища
	this->_error = storage_error_t::NONE;
	/**
	 * Если запись хранилища пуста
	 */
	if(data.empty()) {
		// Устанавливаем ошибку пустой записи
		this->_error = storage_error_t::EMPTY;
		// Выводим результат восстановления собранных выражений
		return false;
	}
	// Позиция чтения записи хранилища
	size_t offset = 0;
	// Опознание записи хранилища
	uint64_t magic = 0;
	/**
	 * Если чтение опознания записи не выполнено
	 */
	if(!read64(data, offset, magic)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления собранных выражений
		return false;
	}
	/**
	 * Если опознание записи не совпадает
	 */
	if(magic != STORAGE_MAGIC) {
		// Устанавливаем ошибку несовпадения опознания записи
		this->_error = storage_error_t::BAD_MAGIC;
		// Выводим результат восстановления собранных выражений
		return false;
	}
	// Версия устройства записи хранилища
	uint16_t version = 0;
	/**
	 * Если чтение версии устройства записи не выполнено
	 */
	if(!read16(data, offset, version)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления собранных выражений
		return false;
	}
	/**
	 * Если версия устройства записи не поддерживается
	 */
	if(version != STORAGE_VERSION) {
		// Устанавливаем ошибку неподдерживаемой версии записи
		this->_error = storage_error_t::BAD_VERSION;
		// Выводим результат восстановления собранных выражений
		return false;
	}
	// Размер содержимого записи и контрольная сумма
	uint64_t length = 0, sum = 0;
	/**
	 * Если чтение размера содержимого либо суммы не выполнено
	 */
	if(!read64(data, offset, length) || !read64(data, offset, sum)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления собранных выражений
		return false;
	}
	/**
	 * Если объявленный размер содержимого записи не отвечает её длине
	 */
	if(length != static_cast <uint64_t> (data.size() - offset)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления собранных выражений
		return false;
	}
	// Получаем содержимое записи хранилища
	const string_view payload = data.substr(offset);
	/**
	 * Если контрольная сумма содержимого записи не совпадает
	 */
	if(checksum(payload) != sum) {
		// Устанавливаем ошибку несовпадения контрольной суммы
		this->_error = storage_error_t::BAD_CHECKSUM;
		// Выводим результат восстановления собранных выражений
		return false;
	}
	// Выполняем сброс позиции чтения содержимого записи
	offset = 0;
	// Количество восстанавливаемых выражений
	uint32_t count = 0;
	/**
	 * Если чтение количества выражений не выполнено
	 */
	if(!readVar(payload, offset, count)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления собранных выражений
		return false;
	}
	/**
	 * Если количество выражений превышает допустимое
	 */
	if(count > MAX_EXPRESSIONS) {
		// Устанавливаем ошибку несообразного содержимого записи
		this->_error = storage_error_t::BAD_CONTENT;
		// Выводим результат восстановления собранных выражений
		return false;
	}
	// Выполняем размещение набора восстановленных выражений
	result.reserve(static_cast <size_t> (count));
	/**
	 * Выполняем восстановление набора выражений
	 */
	for(uint32_t i = 0; i < count; i++) {
		// Создаём восстанавливаемое выражение
		auto expression = make_shared <expression_t> ();
		// Признак выражения, читаемый записью
		uint8_t flag = 0;
		/**
		 * Если чтение признаков выражения не выполнено
		 */
		if(!read8(payload, offset, flag)) {
			// Устанавливаем ошибку обрыва записи
			this->_error = storage_error_t::TRUNCATED;
			// Выводим результат восстановления собранных выражений
			return false;
		}
		// Выполняем установку признака исполнения выражения с возвратом
		expression->backtracking = (flag != 0);
		/**
		 * Если чтение признаков выражения не выполнено
		 */
		if(!read8(payload, offset, flag)) {
			// Устанавливаем ошибку обрыва записи
			this->_error = storage_error_t::TRUNCATED;
			// Выводим результат восстановления собранных выражений
			return false;
		}
		// Выполняем установку признака готовности выражения
		expression->ready = (flag != 0);
		/**
		 * Если чтение признаков выражения не выполнено
		 */
		if(!read8(payload, offset, flag)) {
			// Устанавливаем ошибку обрыва записи
			this->_error = storage_error_t::TRUNCATED;
			// Выводим результат восстановления собранных выражений
			return false;
		}
		// Выполняем установку признака применимости поиска начала совпадения
		expression->reversible = (flag != 0);
		/**
		 * Если восстановление программ выражения не выполнено
		 */
		if(!this->load(payload, offset, expression->forward) ||
		 !this->load(payload, offset, expression->backward))
			// Выводим результат восстановления собранных выражений
			return false;
		// Количество именованных групп выражения
		uint32_t names = 0;
		/**
		 * Если чтение количества именованных групп не выполнено
		 */
		if(!readVar(payload, offset, names)) {
			// Устанавливаем ошибку обрыва записи
			this->_error = storage_error_t::TRUNCATED;
			// Выводим результат восстановления собранных выражений
			return false;
		}
		/**
		 * Если количество именованных групп превышает размер оставшейся записи
		 */
		if((static_cast <size_t> (names) * 3) > (payload.size() - offset)) {
			// Устанавливаем ошибку несообразного содержимого записи
			this->_error = storage_error_t::BAD_CONTENT;
			// Выводим результат восстановления собранных выражений
			return false;
		}
		/**
		 * Выполняем восстановление соответствия имён именованных групп
		 */
		for(uint32_t j = 0; j < names; j++) {
			// Имя именованной группы выражения
			string name;
			// Количество номеров именованной группы
			uint32_t numbers = 0;
			/**
			 * Если чтение имени именованной группы не выполнено
			 */
			if(!readText(payload, offset, name) || !readVar(payload, offset, numbers)) {
				// Устанавливаем ошибку обрыва записи
				this->_error = storage_error_t::TRUNCATED;
				// Выводим результат восстановления собранных выражений
				return false;
			}
			/**
			 * Если количество номеров превышает размер оставшейся записи
			 */
			if(static_cast <size_t> (numbers) > (payload.size() - offset)) {
				// Устанавливаем ошибку несообразного содержимого записи
				this->_error = storage_error_t::BAD_CONTENT;
				// Выводим результат восстановления собранных выражений
				return false;
			}
			// Создаём набор номеров именованной группы
			vector <uint32_t> records(static_cast <size_t> (numbers));
			/**
			 * Выполняем перебор набора номеров именованной группы
			 */
			for(auto & number : records) {
				/**
				 * Если чтение номера именованной группы не выполнено
				 */
				if(!readVar(payload, offset, number)) {
					// Устанавливаем ошибку обрыва записи
					this->_error = storage_error_t::TRUNCATED;
					// Выводим результат восстановления собранных выражений
					return false;
				}
				/**
				 * Если номер именованной группы выражению не принадлежит
				 */
				if(number > expression->forward.captures) {
					// Устанавливаем ошибку несообразного содержимого записи
					this->_error = storage_error_t::BAD_CONTENT;
					// Выводим результат восстановления собранных выражений
					return false;
				}
			}
			// Выполняем добавление именованной группы в соответствие
			expression->names.emplace(::move(name), ::move(records));
		}
		/**
		 * Если выражение собрано с режимом порождения машинного кода
		 *
		 * @details Порождённый код в записи не хранится, поэтому порождается
		 *          заново. Отказ порождения отказом восстановления не является:
		 *          выражение сопоставляется исполнением программы, как и без
		 *          режима.
		 */
		if(((expression->forward.flags & static_cast <uint32_t> (flag_t::JIT)) != 0) &&
		 ((expression->forward.flags & (static_cast <uint32_t> (flag_t::ANCHORED) |
		  static_cast <uint32_t> (flag_t::NOTEMPTY))) == 0) &&
		 !expression->backtracking && !expression->forward.plain) {
			// Создаём сопоставитель выражения в виде порождённого машинного кода
			expression->machine = make_shared <codegen_t> ();
			/**
			 * Если порождение сопоставителя выражения не выполнено
			 */
			if(!expression->machine->compile(expression->forward))
				// Выполняем сброс сопоставителя выражения
				expression->machine.reset();
		}
		// Выполняем добавление восстановленного выражения в набор
		result.push_back(::move(expression));
	}
	/**
	 * Если запись хранилища прочитана не до конца
	 */
	if(offset != payload.size()) {
		// Устанавливаем ошибку несообразного содержимого записи
		this->_error = storage_error_t::BAD_CONTENT;
		// Выполняем очистку набора восстановленных выражений
		result.clear();
		// Выводим результат восстановления собранных выражений
		return false;
	}
	// Выводим результат восстановления собранных выражений
	return true;
}
/**
 * @brief Метод извлечения кода ошибки хранилища
 *
 * @return код ошибки хранилища собранных выражений
 *
 */
awh::regex::storage_error_t awh::regex::Storage::error() const noexcept {
	// Выводим код ошибки хранилища собранных выражений
	return this->_error;
}
