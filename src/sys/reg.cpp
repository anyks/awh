/**
 * @file: reg.cpp
 * @date: 2023-12-14
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

// Подключаем заголовочный файл
#include <sys/reg.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Оператор проверки на инициализацию регулярного выражения
 * @return результат проверки
 */
awh::RegExp::Expression::operator bool() const noexcept {
	// Выводим результ проверки инициализации
	return this->_mode;
}
/**
 * operator Оператор установки флага инициализации
 * @param mode флаг инициализации для установки
 * @return     текущий объект регулярного выражения
 */
awh::RegExp::Expression & awh::RegExp::Expression::operator = (const bool mode) noexcept {
	// Выполняем установку флага инициализации
	this->_mode = mode;
	// Выводим текущее значение объекта
	return (* this);
}
/**
 * Expression Конструктор
 */
awh::RegExp::Expression::Expression() noexcept : _mode(false) {}
/**
 * ~Expression Деструктор
 */
awh::RegExp::Expression::~Expression() noexcept {
	// Если уже модуль проинициализированны
	if(this->_mode){
		// Запрещаем повторное удаление регулярного выражения
		this->_mode = !this->_mode;
		// Удаляем контекст регулярного выражения
		::pcre2_regfree(&this->reg);
	}
}
/**
 * error Метод извлечения текста ошибки регулярного выражения
 * @return текст ошибки регулярного выражения
 */
const string & awh::RegExp::error() const noexcept {
	// Выполняем извлечение текста ошибки регулярного выражения
	return this->_error;
}
/**
 * test Метод проверки регулярного выражения
 * @param text текст для обработки
 * @param exp  объект регулярного выражения
 * @return     результат проверки регулярного выражения
 */
bool awh::RegExp::test(const string & text, const exp_t & exp) const noexcept {
	// Если данные переданы верные
	if(!text.empty() && static_cast <bool> (exp))
		// Выполняем проверку регулярного выражения
		return this->test(text.c_str(), text.length(), exp);
	// Выводим результат
	return false;
}
/**
 * test Метод проверки регулярного выражения
 * @param text текст для обработки
 * @param size размер текста для обработки
 * @param exp  объект регулярного выражения
 * @return     результат проверки регулярного выражения
 */
bool awh::RegExp::test(const char * text, const size_t size, const exp_t & exp) const noexcept {
	// Результат работы функции
	bool result = false;
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(this->_mtx.match);
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
				char buffer[256];
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
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * exec Метод запуска регулярного выражения
 * @param text текст для обработки
 * @param exp  объект регулярного выражения
 * @return     результат обработки регулярного выражения
 */
vector <string> awh::RegExp::exec(const string & text, const exp_t & exp) const noexcept {
	// Если данные переданы верные
	if(!text.empty() && static_cast <bool> (exp))
		// Выполняем запуск регулярного выражения
		return this->exec(text.c_str(), text.length(), exp);
	// Выводим результат
	return vector <string> ();
}
/**
 * exec Метод запуска регулярного выражения
 * @param text текст для обработки
 * @param size размер текста для обработки
 * @param exp  объект регулярного выражения
 * @return     результат обработки регулярного выражения
 */
vector <string> awh::RegExp::exec(const char * text, const size_t size, const exp_t & exp) const noexcept {
	// Результат работы функции
	vector <string> result;
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(this->_mtx.match);
	// Если данные переданы верные
	if((text != nullptr) && (size > 0) && static_cast <bool> (exp)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём объект матчинга
			unique_ptr <regmatch_t []> match(new regmatch_t [exp->reg.re_nsub + 1]);
			// Выполняем разбор регулярного выражения
			const int32_t error = ::pcre2_regexec(&exp->reg, text, exp->reg.re_nsub + 1, match.get(), REG_NOTEMPTY);
			// Если возникла ошибка
			if(error > 0){
				// Создаём буфер данных для извлечения данных ошибки
				char buffer[256];
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
						result.at(i).assign(text + match[i].rm_so, match[i].rm_eo - match[i].rm_so);
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const bad_alloc &) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, "Memory allocation error");
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", "Memory allocation error");
			#endif
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * match Метод выполнения регулярного выражения
 * @param text текст для обработки
 * @param exp  объект регулярного выражения
 * @return     результат обработки регулярного выражения
 */
vector <pair <size_t, size_t>> awh::RegExp::match(const string & text, const exp_t & exp) const noexcept {
	// Если данные переданы верные
	if(!text.empty() && static_cast <bool> (exp))
		// Выполняем выполнение регулярного выражения
		return this->match(text.c_str(), text.length(), exp);
	// Выводим результат
	return vector <pair <size_t, size_t>> ();
}
/**
 * match Метод выполнения регулярного выражения
 * @param text текст для обработки
 * @param size размер текста для обработки
 * @param exp  объект регулярного выражения
 * @return     результат обработки регулярного выражения
 */
vector <pair <size_t, size_t>> awh::RegExp::match(const char * text, const size_t size, const exp_t & exp) const noexcept {
	// Результат работы функции
	vector <pair <size_t, size_t>> result;
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(this->_mtx.match);
	// Если данные переданы верные
	if((text != nullptr) && (size > 0) && static_cast <bool> (exp)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём объект матчинга
			unique_ptr <regmatch_t []> match(new regmatch_t [exp->reg.re_nsub + 1]);
			// Выполняем разбор регулярного выражения
			const int32_t error = ::pcre2_regexec(&exp->reg, text, exp->reg.re_nsub + 1, match.get(), REG_NOTEMPTY);
			// Если возникла ошибка
			if(error > 0){
				// Создаём буфер данных для извлечения данных ошибки
				char buffer[256];
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
						result.at(i) = make_pair(static_cast <size_t> (match[i].rm_so), static_cast <size_t> (match[i].rm_eo - match[i].rm_so));
					// Добавляем пустое значение
					else result.at(i) = make_pair(static_cast <size_t> (0), static_cast <size_t> (0));
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const bad_alloc &) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, "Memory allocation error");
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", "Memory allocation error");
			#endif
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * build Метод сборки регулярного выражения
 * @param pattern регулярное выражение для сборки
 * @param options список опций для сборки регулярного выражения
 * @return        результат собранного регулярного выражения
 */
awh::RegExp::exp_t awh::RegExp::build(const string & pattern, const vector <option_t> & options) const noexcept {
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
					// Определяем тип переданной опции
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
			const auto & key = make_pair(option, pattern);
			// Выполняем поиск уже ранее созданного регулярного выражения
			auto i = this->_cache.find(key);
			// Если регулярное выражение уже созданно
			if(i != this->_cache.end()){
				// Выполняем получение скомпилированного регулярного выражения
				result = i->second.lock();
				// Если регулярное выражение уже устарело и удалено
				if(result == nullptr){
					// Выполняем блокировку потока
					const lock_guard <mutex> lock(this->_mtx.cache);
					// Удаляем запись
					this->_cache.erase(key);
				}
			}
			// Выполняем генерацию нового регулярного выражения
			if(result == nullptr){
				// Выполняем создание нового блока результата
				result = exp_t(new Expression);
				// Выполняем компиляцию регулярного выражения
				const int32_t error = ::pcre2_regcomp(&result->reg, pattern.c_str(), option);
				// Если возникла ошибка компиляции
				if(!((* result.get()) = static_cast <bool> (error == 0))){
					// Создаём буфер данных для извлечения данных ошибки
					char buffer[256];
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
					const lock_guard <mutex> lock(this->_mtx.cache);
					// Добавляем регулярное выражение в список
					this->_cache.emplace(key, result);
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const bad_alloc &) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, "Memory allocation error");
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", "Memory allocation error");
			#endif
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
