/**
 * @file: reg.cpp
 * @date: 2025-10-25
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

/**
 * Стандартные модули
 */
#include <memory>
#include <cstring>
#include <iostream>

/**
 * Подключаем модуль PCRE2
 */
#include <pcre2/pcre2posix.h>

/**
 * Подключаем заголовочные файлы регулярых выражений и логера
 */
#include <sys/reg.hpp>
#include <sys/log.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * @brief Класс регулярного выражения
 *
 */
class awh::Regular_Expressions::Expression {
	private:
		// Флаг инициализации
		bool _mode;
	public:
		// Объект контекста регулярного выражения
		regex_t reg;
	public:
		/**
		 * @brief Оператор проверки на инициализацию регулярного выражения
		 *
		 * @return результат проверки
		 */
		operator bool() const noexcept;
	public:
		/**
		 * @brief Оператор установки флага инициализации
		 *
		 * @param mode флаг инициализации для установки
		 * @return     текущий объект регулярного выражения
		 */
		Expression & operator = (const bool mode) noexcept;
	public:
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Expression() noexcept;
		/**
		 * @brief Деструктор
		 *
		 */
		~Expression() noexcept;
};

/**
 * @brief Оператор проверки на инициализацию регулярного выражения
 *
 * @return результат проверки
 */
awh::Regular_Expressions::Expression::operator bool() const noexcept {
	// Выводим результ проверки инициализации
	return this->_mode;
}
/**
 * @brief Оператор установки флага инициализации
 *
 * @param mode флаг инициализации для установки
 * @return     текущий объект регулярного выражения
 */
awh::Regular_Expressions::Expression & awh::Regular_Expressions::Expression::operator = (const bool mode) noexcept {
	// Выполняем установку флага инициализации
	this->_mode = mode;
	// Выводим текущее значение объекта
	return (* this);
}
/**
 * @brief Конструктор
 *
 */
awh::Regular_Expressions::Expression::Expression() noexcept : _mode(false) {}
/**
 * @brief Деструктор
 *
 */
awh::Regular_Expressions::Expression::~Expression() noexcept {
	// Если уже модуль проинициализированны
	if(this->_mode){
		// Запрещаем повторное удаление регулярного выражения
		this->_mode = !this->_mode;
		// Удаляем контекст регулярного выражения
		::pcre2_regfree(&this->reg);
	}
}
/**
 * @brief Метод извлечения текста ошибки регулярного выражения
 *
 * @return текст ошибки регулярного выражения
 */
const string & awh::Regular_Expressions::error() const noexcept {
	// Выполняем извлечение текста ошибки регулярного выражения
	return this->_error;
}
/**
 * @brief Метод установки безопасности работы потоков
 *
 * @param mode флаг режима безопасности потоков
 */
void awh::Regular_Expressions::threadSafety(const bool mode) noexcept {
	/** 
	 * Активируем или деактивируем мьютексы в зависимости от переданного флага
	 */
	this->_mtx.match.enabled = mode;
	this->_mtx.cache.enabled = mode;
}
/**
 * @brief Метод проверки регулярного выражения
 *
 * @param text текст для обработки
 * @param exp  объект регулярного выражения
 * @return     результат проверки регулярного выражения
 */
bool awh::Regular_Expressions::test(string_view text, const exp_t & exp) const noexcept {
	// Если данные переданы верные
	if(!text.empty() && static_cast <bool> (exp))
		// Выполняем проверку регулярного выражения
		return this->test(text.data(), text.size(), exp);
	// Выводим результат
	return false;
}
/**
 * @brief Метод проверки регулярного выражения
 *
 * @param text текст для обработки
 * @param size размер текста для обработки
 * @param exp  объект регулярного выражения
 * @return     результат проверки регулярного выражения
 */
bool awh::Regular_Expressions::test(const char * text, const size_t size, const exp_t & exp) const noexcept {
	// Результат работы функции
	bool result = false;
	// Выполняем блокировку потока
	const locker_t <> lock(this->_mtx.match);
	// Если данные переданы верные
	if((text != nullptr) && (size > 0) && static_cast <bool> (exp)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём объект матчинга
			regmatch_t match[1];
			// Выполняем разбор регулярного выражения
			const int32_t error = ::pcre2_regexec(&exp->reg, text, 1, match, REG_NOTEMPTY);
			// Если возникла ошибка
			if(!(result = (error == 0))){
				// Создаём буфер данных для извлечения данных ошибки
				char buffer[0xFF];
				// Выполняем заполнение нулями буфер данных
				::memset(buffer, '\0', sizeof(buffer));
				// Выполняем извлечение текста ошибки
				const size_t size = ::pcre2_regerror(error, &exp->reg, buffer, sizeof(buffer) - 1);
				// Если текст ошибки получен
				if(size > 0)
					// Выполняем установку кода ошибки
					const_cast <regexp_t *> (this)->_error.assign(buffer, size);
			// Если ошибок не получено
			} else result = (match[0].rm_eo > 0);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(text, size), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод запуска регулярного выражения
 *
 * @param text текст для обработки
 * @param exp  объект регулярного выражения
 * @return     результат обработки регулярного выражения
 */
vector <string> awh::Regular_Expressions::exec(string_view text, const exp_t & exp) const noexcept {
	// Если данные переданы верные
	if(!text.empty() && static_cast <bool> (exp))
		// Выполняем запуск регулярного выражения
		return this->exec(text.data(), text.size(), exp);
	// Выводим результат
	return vector <string> ();
}
/**
 * @brief Метод запуска регулярного выражения
 *
 * @param text текст для обработки
 * @param size размер текста для обработки
 * @param exp  объект регулярного выражения
 * @return     результат обработки регулярного выражения
 */
vector <string> awh::Regular_Expressions::exec(const char * text, const size_t size, const exp_t & exp) const noexcept {
	// Результат работы функции
	vector <string> result;
	// Выполняем блокировку потока
	const locker_t <> lock(this->_mtx.match);
	// Если данные переданы верные
	if((text != nullptr) && (size > 0) && static_cast <bool> (exp)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём объект матчинга
			std::unique_ptr <regmatch_t []> match(new regmatch_t [exp->reg.re_nsub + 1]);
			// Выполняем разбор регулярного выражения
			const int32_t error = ::pcre2_regexec(&exp->reg, text, exp->reg.re_nsub + 1, match.get(), REG_NOTEMPTY);
			// Если возникла ошибка
			if(error > 0){
				// Создаём буфер данных для извлечения данных ошибки
				char buffer[0xFF];
				// Выполняем заполнение нулями буфер данных
				::memset(buffer, '\0', sizeof(buffer));
				// Выполняем извлечение текста ошибки
				const size_t size = ::pcre2_regerror(error, &exp->reg, buffer, sizeof(buffer) - 1);
				// Если текст ошибки получен
				if(size > 0)
					// Выполняем установку кода ошибки
					const_cast <regexp_t *> (this)->_error.assign(buffer, size);
			// Если ошибок не получено
			} else {
				// Выполняем создание результата
				result.resize(exp->reg.re_nsub + 1);
				// Выполняем перебор всех полученных вариантов
				for(uint8_t i = 0; i < static_cast <uint8_t> (exp->reg.re_nsub + 1); i++){
					// Если результат получен
					if((match[i].rm_eo > 0) && (static_cast <size_t> (match[i].rm_eo) <= size) && (match[i].rm_so >= 0))
						// Добавляем полученный результат в список результатов
						result[i].assign(text + match[i].rm_so, match[i].rm_eo - match[i].rm_so);
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const bad_alloc &) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(text, size), log_t::flag_t::CRITICAL, "Memory allocation error");
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, "Memory allocation error");
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! %s\n", "Memory allocation error");
				#endif
			}
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(text, size), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод выполнения регулярного выражения
 *
 * @param text текст для обработки
 * @param exp  объект регулярного выражения
 * @return     результат обработки регулярного выражения
 */
vector <std::pair <size_t, size_t>> awh::Regular_Expressions::match(string_view text, const exp_t & exp) const noexcept {
	// Если данные переданы верные
	if(!text.empty() && static_cast <bool> (exp))
		// Выполняем выполнение регулярного выражения
		return this->match(text.data(), text.size(), exp);
	// Выводим результат
	return vector <std::pair <size_t, size_t>> ();
}
/**
 * @brief Метод выполнения регулярного выражения
 *
 * @param text текст для обработки
 * @param size размер текста для обработки
 * @param exp  объект регулярного выражения
 * @return     результат обработки регулярного выражения
 */
vector <std::pair <size_t, size_t>> awh::Regular_Expressions::match(const char * text, const size_t size, const exp_t & exp) const noexcept {
	// Результат работы функции
	vector <std::pair <size_t, size_t>> result;
	// Выполняем блокировку потока
	const locker_t <> lock(this->_mtx.match);
	// Если данные переданы верные
	if((text != nullptr) && (size > 0) && static_cast <bool> (exp)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём объект матчинга
			std::unique_ptr <regmatch_t []> match(new regmatch_t [exp->reg.re_nsub + 1]);
			// Выполняем разбор регулярного выражения
			const int32_t error = ::pcre2_regexec(&exp->reg, text, exp->reg.re_nsub + 1, match.get(), REG_NOTEMPTY);
			// Если возникла ошибка
			if(error > 0){
				// Создаём буфер данных для извлечения данных ошибки
				char buffer[0xFF];
				// Выполняем заполнение нулями буфер данных
				::memset(buffer, '\0', sizeof(buffer));
				// Выполняем извлечение текста ошибки
				const size_t size = ::pcre2_regerror(error, &exp->reg, buffer, sizeof(buffer) - 1);
				// Если текст ошибки получен
				if(size > 0)
					// Выполняем установку кода ошибки
					const_cast <regexp_t *> (this)->_error.assign(buffer, size);
			// Если ошибок не получено
			} else {
				// Выполняем создание результата
				result.resize(exp->reg.re_nsub + 1);
				// Выполняем перебор всех полученных вариантов
				for(uint8_t i = 0; i < static_cast <uint8_t> (exp->reg.re_nsub + 1); i++){
					// Если результат получен
					if((match[i].rm_eo > 0) && (static_cast <size_t> (match[i].rm_eo) <= size) && (match[i].rm_so >= 0))
						// Добавляем полученный результат в список результатов
						result[i] = std::make_pair(static_cast <size_t> (match[i].rm_so), static_cast <size_t> (match[i].rm_eo - match[i].rm_so));
					// Добавляем пустое значение
					else result[i] = std::make_pair(static_cast <size_t> (0), static_cast <size_t> (0));
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const bad_alloc &) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(text, size), log_t::flag_t::CRITICAL, "Memory allocation error");
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, "Memory allocation error");
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! %s\n", "Memory allocation error");
				#endif
			}
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(text, size), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод сборки регулярного выражения
 *
 * @param pattern регулярное выражение для сборки
 * @param options список опций для сборки регулярного выражения
 * @return        результат собранного регулярного выражения
 */
awh::Regular_Expressions::exp_t awh::Regular_Expressions::build(string_view pattern, const vector <option_t> & options) const noexcept {
	// Результат работы функции
	exp_t result = nullptr;
	// Если регулярное выражение передано
	if(!pattern.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Список основных опций
			int32_t option = 0;
			// Если опции переданы
			if(!options.empty()){
				// Выполняем перебор всех переданных опций
				for(auto & item : options){
					/**
					 * Определяем тип переданной опции
					 */
					switch(static_cast <uint8_t> (item)){
						// Если передан флаг запуска в режиме UTF-8
						case static_cast <uint8_t> (option_t::UTF8):
							// Выполняем установку флага
							option |= REG_UTF;
						break;
						// Если передан флаг запреда вывода сопоставления
						case static_cast <uint8_t> (option_t::NOSUB):
							// Выполняем установку флага
							option |= REG_NOSUB;
						break;
						// Если передан флаг точки соответствующей чему угодно, включая NL
						case static_cast <uint8_t> (option_t::DOTALL):
							// Выполняем установку флага
							option |= REG_DOTALL;
						break;
						// Если передан флаг инвертирования жадности кванторов
						case static_cast <uint8_t> (option_t::UNGREEDY):
							// Выполняем установку флага
							option |= REG_UNGREEDY;
						break;
						// Если нужно блокировать пустые строки
						case static_cast <uint8_t> (option_t::NOTEMPTY):
							// Выполняем установку флага
							option |= REG_NOTEMPTY;
						break;
						// Если передан флаг работы без учёта регистра
						case static_cast <uint8_t> (option_t::CASELESS):
							// Выполняем установку флага
							option |= REG_ICASE;
						break;
						// Если передан флаг то (^ и $) будут соответствовать новым строкам в тексте
						case static_cast <uint8_t> (option_t::MULTILINE):
							// Выполняем установку флага
							option |= REG_NEWLINE;
						break;
					}
				}
			}
			// Создаём ключ регулярного выражения
			const auto & key = std::make_pair(option, string{pattern});
			// Выполняем поиск уже ранее созданного регулярного выражения
			auto i = this->_cache.find(key);
			// Если регулярное выражение уже созданно
			if(i != this->_cache.end()){
				// Выполняем получение скомпилированного регулярного выражения
				result = i->second.lock();
				// Если регулярное выражение уже устарело и удалено
				if(result == nullptr){
					// Выполняем блокировку потока
					const locker_t <> lock(this->_mtx.cache);
					// Удаляем запись
					this->_cache.erase(key);
				}
			}
			// Выполняем генерацию нового регулярного выражения
			if(result == nullptr){
				// Выполняем создание нового блока результата
				result = exp_t(new Expression);
				// Выполняем инициализацию объекта регулярного выражения
				result->reg = regex_t();
				// Выполняем компиляцию регулярного выражения
				const int32_t error = ::pcre2_regcomp(&result->reg, pattern.data(), option);
				// Если возникла ошибка компиляции
				if(!((* result.get()) = static_cast <bool> (error == 0))){
					// Создаём буфер данных для извлечения данных ошибки
					char buffer[0xFF];
					// Выполняем заполнение нулями буфер данных
					::memset(buffer, '\0', sizeof(buffer));
					// Выполняем извлечение текста ошибки
					const size_t size = ::pcre2_regerror(error, &result->reg, buffer, sizeof(buffer) - 1);
					// Если текст ошибки получен
					if(size > 0)
						// Выполняем установку кода ошибки
						const_cast <regexp_t *> (this)->_error.assign(buffer, size);
					// Выполняем удаление скомпилированного регулярного выражения
					::pcre2_regfree(&result->reg);
					// Выполняем сброс блока результата
					result.reset();
				// Если регулярное выражение удачно созданно
				} else {
					// Выполняем блокировку потока
					const locker_t <> lock(this->_mtx.cache);
					// Добавляем регулярное выражение в список
					this->_cache.emplace(key, result);
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const bad_alloc &) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(pattern, options.size()), log_t::flag_t::CRITICAL, "Memory allocation error");
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, "Memory allocation error");
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! %s\n", "Memory allocation error");
				#endif
			}
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(pattern, options.size()), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки объекта логирования
 *
 * @param log объект работы с логами
 */
void awh::Regular_Expressions::setLogger(const log_t * log) noexcept {
	// Выполняем установку объекта логирования
	this->_log = log;
}
/**
 * @brief Конструктор
 *
 */
awh::Regular_Expressions::Regular_Expressions() noexcept : _error{""}, _log(nullptr) {
	/**
	 * Деактивируем мьютексы на время инициализации
	 */
	this->_mtx.match.enabled = false;
	this->_mtx.cache.enabled = false;
}
/**
 * @brief Конструктор
 *
 * @param log объект работы с логами
 */
awh::Regular_Expressions::Regular_Expressions(const log_t * log) noexcept : _error{""}, _log(log) {
	/**
	 * Деактивируем мьютексы на время инициализации
	 */
	this->_mtx.match.enabled = false;
	this->_mtx.cache.enabled = false;
}
/**
 * @brief Деструктор
 *
 */
awh::Regular_Expressions::~Regular_Expressions() noexcept {}
