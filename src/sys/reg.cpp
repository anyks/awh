/**
 * @file: reg.cpp
 * @date: 2023-12-14
 * @license{LicenseRef-AWH-1.0}
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
 * Подключаем заголовочный файл
 */
#include <sys/reg.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * @brief Оператор проверки на инициализацию регулярного выражения
 *
 * @return результат проверки
 */
awh::RegExp::Expression::operator bool() const noexcept {
	// Выводим результ проверки инициализации
	return this->_mode;
}
/**
 * @brief Оператор установки флага инициализации
 *
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
 * @brief Конструктор
 *
 */
awh::RegExp::Expression::Expression() noexcept : _mode(false), _engine(nullptr) {}
/**
 * @brief Деструктор
 *
 */
awh::RegExp::Expression::~Expression() noexcept = default;
/**
 * @brief Метод извлечения текста ошибки регулярного выражения
 *
 * @return текст ошибки регулярного выражения
 */
const string & awh::RegExp::error() const noexcept {
	// Выводим текст ошибки
	return this->_error;
}
/**
 * @brief Метод проверки регулярного выражения
 *
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
 * @brief Метод проверки регулярного выражения
 *
 * @param text текст для обработки
 * @param size размер текста для обработки
 * @param exp  объект регулярного выражения
 * @return     результат проверки регулярного выражения
 */
bool awh::RegExp::test(const char * text, const size_t size, const exp_t & exp) const noexcept {
	// Результат работы функции
	bool result = false;
	// Выполняем блокировку потока
	const lock_guard <std::mutex> lock(this->_mtx.match);
	// Если данные переданы верные
	if((text != nullptr) && (size > 0) && static_cast <bool> (exp) && (exp->_engine != nullptr)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем блокировку исполнения
			const lock_guard <std::mutex> locker(exp->_locker);
			// Список полученных захватов
			vector <std::pair <size_t, size_t>> captures;
			// Выполняем сопоставление с началом текста
			if(exp->_engine->exec(string_view(text, size), 0, captures))
				// Сопоставление найдено, если совпадение не пустое
				result = (!captures.empty() && (captures.front().second > 0));
			// Если сопоставление не выполнено
			else const_cast <regexp_t *> (this)->_error = exp->_engine->message();
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
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
vector <string> awh::RegExp::exec(const string & text, const exp_t & exp) const noexcept {
	// Если данные переданы верные
	if(!text.empty() && static_cast <bool> (exp))
		// Выполняем запуск регулярного выражения
		return this->exec(text.c_str(), text.length(), exp);
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
vector <string> awh::RegExp::exec(const char * text, const size_t size, const exp_t & exp) const noexcept {
	// Результат работы функции
	vector <string> result;
	// Выполняем блокировку потока
	const lock_guard <std::mutex> lock(this->_mtx.match);
	// Если данные переданы верные
	if((text != nullptr) && (size > 0) && static_cast <bool> (exp) && (exp->_engine != nullptr)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем блокировку исполнения
			const lock_guard <std::mutex> locker(exp->_locker);
			// Список полученных захватов
			vector <std::pair <size_t, size_t>> captures;
			// Выполняем сопоставление с началом текста
			if(exp->_engine->exec(string_view(text, size), 0, captures)){
				// Выполняем перебор всех полученных захватов
				for(auto & capture : captures){
					// Границы захвата: движок отдаёт начало и конец
					if((capture.second > capture.first) && (capture.second <= size))
						// Добавляем полученный результат в список
						result.push_back(string(text + capture.first, capture.second - capture.first));
				}
			// Если сопоставление не выполнено
			} else const_cast <regexp_t *> (this)->_error = exp->_engine->message();
		/**
		 * Если возникает ошибка
		 */
		} catch(const bad_alloc &) {
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
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
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
vector <std::pair <size_t, size_t>> awh::RegExp::match(const string & text, const exp_t & exp) const noexcept {
	// Если данные переданы верные
	if(!text.empty() && static_cast <bool> (exp))
		// Выполняем выполнение регулярного выражения
		return this->match(text.c_str(), text.length(), exp);
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
vector <std::pair <size_t, size_t>> awh::RegExp::match(const char * text, const size_t size, const exp_t & exp) const noexcept {
	// Результат работы функции
	vector <std::pair <size_t, size_t>> result;
	// Выполняем блокировку потока
	const lock_guard <std::mutex> lock(this->_mtx.match);
	// Если данные переданы верные
	if((text != nullptr) && (size > 0) && static_cast <bool> (exp) && (exp->_engine != nullptr)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем блокировку исполнения
			const lock_guard <std::mutex> locker(exp->_locker);
			// Выполняем сопоставление с началом текста
			if(exp->_engine->exec(string_view(text, size), 0, result)){
				/**
				 * Движок отдаёт границы захвата парой «начало - конец», а обёртка
				 * v4 отвечает парой «начало - длина»: разность и есть длина
				 */
				for(auto & capture : result){
					// Если захват получен и лежит в пределах текста
					if((capture.second >= capture.first) && (capture.second <= size))
						// Переводим конец захвата в его длину
						capture.second -= capture.first;
					// Если захват выходит за пределы текста
					else capture = std::make_pair(static_cast <size_t> (0), static_cast <size_t> (0));
				}
			// Если сопоставление не выполнено
			} else {
				// Выполняем установку текста ошибки
				const_cast <regexp_t *> (this)->_error = exp->_engine->message();
				// Выполняем очистку результата
				result.clear();
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const bad_alloc &) {
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
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
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
			uint32_t option = 0;
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
							option |= static_cast <uint32_t> (regex::flag_t::UTF);
						break;
						// Если передан флаг поддержки свойств Юникода
						case static_cast <uint8_t> (option_t::UCP):
							// Выполняем установку флага
							option |= static_cast <uint32_t> (regex::flag_t::UCP);
						break;
						// Если передан флаг точки соответствующей чему угодно, включая NL
						case static_cast <uint8_t> (option_t::DOTALL):
							// Выполняем установку флага
							option |= static_cast <uint32_t> (regex::flag_t::DOTALL);
						break;
						// Если передан флаг инвертирования жадности кванторов
						case static_cast <uint8_t> (option_t::UNGREEDY):
							// Выполняем установку флага
							option |= static_cast <uint32_t> (regex::flag_t::UNGREEDY);
						break;
						// Если нужно блокировать пустые строки
						case static_cast <uint8_t> (option_t::NOTEMPTY):
							// Выполняем установку флага
							option |= static_cast <uint32_t> (regex::flag_t::NOTEMPTY);
						break;
						// Если передан флаг работы без учёта регистра
						case static_cast <uint8_t> (option_t::CASELESS):
							// Выполняем установку флага
							option |= static_cast <uint32_t> (regex::flag_t::CASELESS);
						break;
						// Если передан флаг то (^ и $) будут соответствовать новым строкам в тексте
						case static_cast <uint8_t> (option_t::MULTILINE):
							// Выполняем установку флага
							option |= static_cast <uint32_t> (regex::flag_t::MULTILINE);
						break;
						/**
						 * Отказ от сообщения о сопоставлении (NOSUB) движку v5 незнаком:
						 * захваты он отдаёт всегда, и опция остаётся без действия
						 */
						default: break;
					}
				}
			}
			// Создаём ключ регулярного выражения
			const auto & key = std::make_pair(static_cast <int32_t> (option), pattern);
			// Выполняем поиск уже ранее созданного регулярного выражения
			auto i = this->_cache.find(key);
			// Если регулярное выражение уже созданно
			if(i != this->_cache.end()){
				// Выполняем получение скомпилированного регулярного выражения
				result = i->second.lock();
				// Если регулярное выражение уже устарело и удалено
				if(result == nullptr){
					// Выполняем блокировку потока
					const lock_guard <std::mutex> lock(this->_mtx.cache);
					// Удаляем запись
					this->_cache.erase(key);
				}
			}
			// Выполняем генерацию нового регулярного выражения
			if(result == nullptr){
				// Выполняем создание нового блока результата
				result = exp_t(new Expression);
				// Выполняем создание движка регулярного выражения
				result->_engine = make_shared <regex::Engine> ();
				// Если выражение собрано
				if(result->_engine->build(pattern, option)){
					// Помечаем выражение инициализированным
					(* result.get()) = true;
					// Выполняем блокировку потока
					const lock_guard <std::mutex> lock(this->_mtx.cache);
					// Добавляем регулярное выражение в список
					this->_cache.emplace(key, result);
				// Если выражение собрать не удалось
				} else {
					// Выполняем установку кода ошибки
					const_cast <regexp_t *> (this)->_error = result->_engine->message();
					// Выполняем сброс блока результата
					result.reset();
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const bad_alloc &) {
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
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
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
	// Выводим результат
	return result;
}
