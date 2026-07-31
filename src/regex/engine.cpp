/**
 * @file: engine.cpp
 * @date: 2026-07-31
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация движка регулярных выражений — сборка прямой и развёрнутой программ,
 *        выбор способа сопоставления по свойствам выражения и длине участка текста,
 *        поиск позиции начала совпадения проходом в обратном направлении
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <regex/engine.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;

/**
 * @brief Конструктор
 *
 */
awh::regex::Engine::Engine() noexcept :
 _backtracking(false), _ready(false), _reversible(false), _error(error_t::NONE) {}
/**
 * @brief Метод извлечения кода ошибки последней операции
 *
 * @return код ошибки последней операции движка
 *
 */
awh::regex::error_t awh::regex::Engine::error() const noexcept {
	// Выводим код ошибки последней операции движка
	return this->_error;
}
/**
 * @brief Метод извлечения текста ошибки последней операции
 *
 * @return текст ошибки последней операции движка
 *
 */
string awh::regex::Engine::message() const noexcept {
	// Выводим текст ошибки последней операции движка
	return this->_parser.message();
}
/**
 * @brief Метод извлечения количества захватывающих групп
 *
 * @return количество захватывающих групп регулярного выражения
 *
 */
uint32_t awh::regex::Engine::captures() const noexcept {
	// Выводим количество захватывающих групп регулярного выражения
	return this->_forward.captures;
}
/**
 * @brief Метод сборки регулярного выражения
 *
 * @param pattern текст регулярного выражения
 * @param flags   набор режимов компиляции регулярного выражения
 * @return        результат выполнения сборки
 *
 */
bool awh::regex::Engine::build(string_view pattern, const uint32_t flags) noexcept {
	// Выполняем сброс флага готовности движка
	this->_ready = false;
	// Выполняем сброс флага исполнения выражения с возвратом
	this->_backtracking = false;
	// Выполняем сброс флага применимости поиска позиции начала
	this->_reversible = false;
	// Выполняем сброс кода ошибки последней операции
	this->_error = error_t::NONE;
	/**
	 * Если разбор регулярного выражения не выполнен
	 */
	if(!this->_parser.parse(pattern, flags)) {
		// Выполняем установку кода ошибки разбора выражения
		this->_error = this->_parser.error();
		// Выводим результат выполнения сборки
		return false;
	}
	/**
	 * Если компиляция регулярного выражения не выполнена
	 */
	if(!this->_compiler.compile(this->_parser, this->_forward)) {
		// Выполняем установку кода ошибки компиляции выражения
		this->_error = this->_compiler.error();
		/**
		 * Если выражение регулярному подмножеству синтаксиса не принадлежит
		 *
		 * @details Выражение вне регулярного подмножества исполняется с возвратом,
		 *          что требует времени, растущего с длиной текста показательно,
		 *          поэтому такое исполнение применяется исключительно при
		 *          недостижимости исполнения без возврата.
		 *
		 */
		if(this->_error == error_t::UNSUPPORTED) {
			/**
			 * Если компиляция регулярного выражения целиком не выполнена
			 */
			if(!this->_compiler.compileFull(this->_parser, this->_forward)) {
				// Выполняем установку кода ошибки компиляции выражения
				this->_error = this->_compiler.error();
				// Выводим результат выполнения сборки
				return false;
			}
			// Выполняем сброс кода ошибки последней операции
			this->_error = error_t::NONE;
			// Выполняем установку флага исполнения выражения с возвратом
			this->_backtracking = true;
			// Выполняем установку флага готовности движка
			this->_ready = true;
			// Выводим результат выполнения сборки
			return true;
		}
		// Выводим результат выполнения сборки
		return false;
	}
	// Выполняем установку флага готовности движка
	this->_ready = true;
	/**
	 * Если выражение привязано к началу попытки сопоставления
	 *
	 * @details Поиск позиции начала совпадения при привязке выражения не требуется,
	 *          поскольку позиция начала совпадения известна заранее.
	 *
	 */
	if((this->_forward.flags & static_cast <uint32_t> (flag_t::ANCHORED)) != 0)
		// Выводим результат выполнения сборки
		return true;
	/**
	 * Выполняем поиск привязок, зависящих от начала попытки сопоставления
	 *
	 * @details Привязка «\G» соответствует началу попытки сопоставления, тогда как
	 *          проход в обратном направлении начинается с конца текста, поэтому
	 *          для выражений с такой привязкой поиск позиции начала неприменим.
	 *
	 */
	for(auto & instruction : this->_forward.instructions) {
		/**
		 * Если инструкция проверяет привязку к началу попытки сопоставления
		 */
		if((instruction.type == opcode_t::ANCHOR) && (instruction.assertion.type == anchor_t::SEARCH_HEAD))
			// Выводим результат выполнения сборки
			return true;
	}
	/**
	 * Если компиляция развёрнутого регулярного выражения выполнена
	 */
	if(this->_compiler.compileReverse(this->_parser, this->_backward))
		// Выполняем установку флага применимости поиска позиции начала
		this->_reversible = true;
	// Выводим результат выполнения сборки
	return true;
}
/**
 * @brief Метод проверки наличия совпадения в тексте
 *
 * @param text  текст для сопоставления
 * @param start позиция начала поиска совпадения
 * @return      результат проверки наличия совпадения
 *
 */
bool awh::regex::Engine::test(string_view text, const size_t start) noexcept {
	/**
	 * Если движок к сопоставлению не готов
	 */
	if(!this->_ready)
		// Выводим результат проверки наличия совпадения
		return false;
	/**
	 * Если выражение исполняется с возвратом
	 */
	if(this->_backtracking) {
		// Создаём набор границ совпадения и захваченных групп
		vector <pair <size_t, size_t>> captures;
		// Выполняем сопоставление регулярного выражения исполнением с возвратом
		const bool result = this->_backtrack.exec(this->_forward, text, start, captures);
		// Выполняем установку кода ошибки исполнения с возвратом
		this->_error = this->_backtrack.error();
		// Выводим результат проверки наличия совпадения
		return result;
	}
	/**
	 * Если обязательный литерал совпадения в тексте отсутствует
	 */
	if(!this->_forward.prefilter.possible(text, start))
		// Выводим результат проверки наличия совпадения
		return false;
	/**
	 * Если детерминированное исполнение применимо
	 */
	if(this->_dfa.available(this->_forward))
		// Выводим результат проверки наличия совпадения детерминированным исполнением
		return this->_dfa.test(this->_forward, text, start);
	// Создаём набор границ совпадения и захваченных групп
	vector <pair <size_t, size_t>> captures;
	/**
	 * Выводим результат сопоставления исполнением без возврата
	 *
	 * @details Наличие совпадения установлено детерминированным исполнением выше,
	 *          поэтому повторный проход по тексту для его проверки не требуется.
	 *
	 */
	return this->_pike.exec(this->_forward, text, start, captures, mode_t::VERIFIED);
}
/**
 * @brief Метод сопоставления регулярного выражения с текстом
 *
 * @param text     текст для сопоставления
 * @param start    позиция начала поиска совпадения
 * @param captures набор границ совпадения и захваченных групп
 * @return         результат поиска совпадения
 *
 */
bool awh::regex::Engine::exec(string_view text, const size_t start, vector <pair <size_t, size_t>> & captures) noexcept {
	// Выполняем очистку набора границ совпадения
	captures.clear();
	/**
	 * Если движок к сопоставлению не готов
	 */
	if(!this->_ready)
		// Выводим результат поиска совпадения
		return false;
	/**
	 * Если выражение исполняется с возвратом
	 */
	if(this->_backtracking) {
		// Выполняем сопоставление регулярного выражения исполнением с возвратом
		const bool result = this->_backtrack.exec(this->_forward, text, start, captures);
		// Выполняем установку кода ошибки исполнения с возвратом
		this->_error = this->_backtrack.error();
		// Выводим результат поиска совпадения
		return result;
	}
	/**
	 * Если обязательный литерал совпадения в тексте отсутствует
	 */
	if(!this->_forward.prefilter.possible(text, start))
		// Выводим результат поиска совпадения
		return false;
	/**
	 * Если детерминированное исполнение неприменимо
	 */
	if(!this->_dfa.available(this->_forward))
		// Выводим результат сопоставления исполнением без возврата
		return this->_pike.exec(this->_forward, text, start, captures);
	// Позиция завершения обнаруженного совпадения
	size_t finish = string_view::npos;
	/**
	 * Если совпадение в тексте отсутствует
	 *
	 * @details Отсутствие совпадения устанавливается детерминированным исполнением,
	 *          проходящим текст многократно быстрее исполнения без возврата.
	 *
	 */
	if(!this->_dfa.search(this->_forward, text, start, finish))
		// Выводим результат поиска совпадения
		return false;
	/**
	 * Если поиск позиции начала совпадения применим
	 */
	if(this->_reversible && (start == 0)) {
		// Получаем длину участка текста, пройденного до завершения совпадения
		const size_t scanned = (finish - start);
		// Получаем длину участка текста, подлежащего проходу в обратном направлении
		const size_t remains = (text.size() - start);
		/**
		 * Если проход в обратном направлении обходится дешевле исполнения без возврата
		 *
		 * @details Исполнение без возврата проходит текст до завершения совпадения,
		 *          тогда как проход в обратном направлении проходит текст целиком,
		 *          но многократно быстрее, поэтому выбор определяется их отношением.
		 *
		 */
		if((scanned > MIN_REVERSE) && (remains < (REVERSE_RATIO * scanned))) {
			// Позиция начала обнаруженного совпадения
			size_t begin = string_view::npos;
			/**
			 * Если поиск позиции начала совпадения выполнен
			 */
			if(this->_reverse.reverse(this->_backward, text, text.size(), begin)) {
				/**
				 * Если исполнение без возврата с позиции начала совпадения выполнено
				 */
				if(this->_pike.exec(this->_forward, text, begin, captures, mode_t::ANCHORED))
					// Выводим результат поиска совпадения
					return true;
			}
		}
	}
	// Выводим результат сопоставления исполнением без возврата
	return this->_pike.exec(this->_forward, text, start, captures);
}
