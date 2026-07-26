/**
 * @file: reg.cpp
 * @date: 2025-10-25
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация модуля регулярных выражений — компиляция и кеширование шаблонов PCRE2,
 *        выполнение поиска и проверки соответствия, извлечение групп совпадений
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <memory>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <functional>

/**
 * Подключаем заголовочный файл для работы с PCRE2
 */
#include <pcre2/pcre2posix.h>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/reg.hpp>
#include <sys/log.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Оператор вычисления хэша ключа кэша
 *
 * @param key ключ кэша (пара из набора опций и текста регулярного выражения)
 * @return    вычисленное значение хэша
 *
 */
size_t awh::Regular_Expressions::CacheHash::operator () (const pair <int32_t, string> & key) const noexcept {
	// Вычисляем хэш для набора опций
	const size_t h1 = hash <int32_t> {}(key.first);
	// Вычисляем хэш для текста регулярного выражения
	const size_t h2 = hash <string> {}(key.second);
	// Выполняем смешивание хэшей и возвращаем результат
	return (h1 ^ (h2 + 0x9E3779B9 + (h1 << 6) + (h1 >> 2)));
}

/**
 * @brief Класс регулярного выражения
 *
 * @details Класс представляет собой обёртку над объектом регулярного выражения, предоставляя методы для его инициализации, выполнения и освобождения ресурсов.
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
		 *
		 */
		operator bool() const noexcept;
	public:
		/**
		 * @brief Оператор установки флага инициализации
		 *
		 * @param mode флаг инициализации для установки
		 * @return     текущий объект регулярного выражения
		 *
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
 *
 */
awh::Regular_Expressions::Expression::operator bool() const noexcept {
	// Возвращаем результат проверки инициализации
	return this->_mode;
}
/**
 * @brief Оператор установки флага инициализации
 *
 * @param mode флаг инициализации для установки
 * @return     текущий объект регулярного выражения
 *
 */
awh::Regular_Expressions::Expression & awh::Regular_Expressions::Expression::operator = (const bool mode) noexcept {
	// Выполняем установку флага инициализации
	this->_mode = mode;
	// Возвращаем текущее значение объекта
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
 *
 */
const string & awh::Regular_Expressions::error() const noexcept {
	// Выполняем извлечение текста ошибки регулярного выражения
	return this->_error;
}
/**
 * @brief Метод установки безопасности работы потоков
 *
 * @param mode флаг режима безопасности потоков
 *
 */
void awh::Regular_Expressions::threadSafety(const bool mode) noexcept {
	/**
	 * Активируем или деактивируем мьютексы в зависимости от переданного флага
	 */
	this->_mtx.enabled = mode;
}
/**
 * @brief Метод проверки регулярного выражения
 *
 * @param text текст для обработки
 * @param exp  объект регулярного выражения
 * @return     результат проверки регулярного выражения
 *
 */
bool awh::Regular_Expressions::test(string_view text, const exp_t & exp) const noexcept {
	// Если данные переданы верные
	if(!text.empty() && static_cast <bool> (exp))
		// Выполняем проверку регулярного выражения
		return this->test(text.data(), text.size(), exp);
	// Возвращаем результат
	return false;
}
/**
 * @brief Метод проверки регулярного выражения
 *
 * @param text текст для обработки
 * @param size размер текста для обработки
 * @param exp  объект регулярного выражения
 * @return     результат проверки регулярного выражения
 *
 */
bool awh::Regular_Expressions::test(const char * text, const size_t size, const exp_t & exp) const noexcept {
	// Переменная результата
	bool result = false;
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
			// Если регулярное выражение успешно сопоставлено
			if(error == 0)
				// Запоминаем результат сопоставления
				result = (match[0].rm_eo > 0);
			// Если получена реальная ошибка (отсутствие совпадения ошибкой не считаем)
			else if(error != REG_NOMATCH) {
				// Создаём буфер данных для извлечения данных ошибки
				char buffer[0xFF];
				// Выполняем заполнение нулями буфер данных
				::memset(buffer, '\0', sizeof(buffer));
				// Выполняем извлечение текста ошибки
				const size_t length = ::pcre2_regerror(error, &exp->reg, buffer, sizeof(buffer));
				// Если текст ошибки получен
				if(length > 0){
					// Выполняем блокировку потока на время записи текста ошибки
					const locker_t <> lock(this->_mtx);
					// Выполняем установку кода ошибки (длина включает завершающий ноль)
					const_cast <regexp_t *> (this)->_error.assign(buffer, ::min(length - 1, sizeof(buffer) - 1));
				}
			}
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
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(text, size), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод запуска регулярного выражения
 *
 * @param text текст для обработки
 * @param exp  объект регулярного выражения
 * @return     результат обработки регулярного выражения
 *
 */
vector <string> awh::Regular_Expressions::exec(string_view text, const exp_t & exp) const noexcept {
	// Если данные переданы верные
	if(!text.empty() && static_cast <bool> (exp))
		// Выполняем запуск регулярного выражения
		return this->exec(text.data(), text.size(), exp);
	// Возвращаем результат
	return vector <string> ();
}
/**
 * @brief Метод запуска регулярного выражения
 *
 * @param text текст для обработки
 * @param size размер текста для обработки
 * @param exp  объект регулярного выражения
 * @return     результат обработки регулярного выражения
 *
 */
vector <string> awh::Regular_Expressions::exec(const char * text, const size_t size, const exp_t & exp) const noexcept {
	// Переменная результата
	vector <string> result;
	// Если данные переданы верные
	if((text != nullptr) && (size > 0) && static_cast <bool> (exp)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Небольшой буфер на стеке для типичных случаев, чтобы избежать аллокации в куче
			regmatch_t stack[16];
			// По умолчанию используем буфер на стеке
			regmatch_t * match = stack;
			// Указатель на динамический буфер совпадений (используется только при большом числе групп)
			unique_ptr <regmatch_t []> heap;
			// Получаем количество ожидаемых групп совпадений
			const size_t count = (exp->reg.re_nsub + 1);
			// Если число групп превышает размер стекового буфера
			if(count > (sizeof(stack) / sizeof(stack[0]))){
				// Выделяем буфер совпадений в куче
				heap.reset(new regmatch_t [count]);
				// Используем буфер в куче
				match = heap.get();
			}
			// Выполняем разбор регулярного выражения
			const int32_t error = ::pcre2_regexec(&exp->reg, text, count, match, REG_NOTEMPTY);
			// Если регулярное выражение успешно сопоставлено
			if(error == 0){
				// Выполняем создание результата
				result.resize(count);
				/**
				 * Выполняем перебор всех полученных вариантов
				 */
				for(size_t i = 0; i < count; i++){
					// Если результат получен
					if((match[i].rm_so >= 0) && (match[i].rm_eo > match[i].rm_so) && (static_cast <size_t> (match[i].rm_eo) <= size))
						// Добавляем полученный результат в список результатов
						result[i].assign(text + match[i].rm_so, match[i].rm_eo - match[i].rm_so);
				}
			// Если получена реальная ошибка (отсутствие совпадения ошибкой не считаем)
			} else if(error != REG_NOMATCH) {
				// Создаём буфер данных для извлечения данных ошибки
				char buffer[0xFF];
				// Выполняем заполнение нулями буфер данных
				::memset(buffer, '\0', sizeof(buffer));
				// Выполняем извлечение текста ошибки
				const size_t length = ::pcre2_regerror(error, &exp->reg, buffer, sizeof(buffer));
				// Если текст ошибки получен
				if(length > 0){
					// Выполняем блокировку потока на время записи текста ошибки
					const locker_t <> lock(this->_mtx);
					// Выполняем установку кода ошибки (длина включает завершающий ноль)
					const_cast <regexp_t *> (this)->_error.assign(buffer, ::min(length - 1, sizeof(buffer) - 1));
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
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(text, size), log_t::flag_t::CRITICAL, "Memory allocation error");
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, "Memory allocation error");
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n", "Memory allocation error");
				#endif
			}
			// Выходим из приложения
			::_exit(EXIT_FAILURE);
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
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(text, size), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод выполнения регулярного выражения
 *
 * @param text текст для обработки
 * @param exp  объект регулярного выражения
 * @return     результат обработки регулярного выражения
 *
 */
vector <pair <size_t, size_t>> awh::Regular_Expressions::match(string_view text, const exp_t & exp) const noexcept {
	// Если данные переданы верные
	if(!text.empty() && static_cast <bool> (exp))
		// Выполняем выполнение регулярного выражения
		return this->match(text.data(), text.size(), exp);
	// Возвращаем результат
	return vector <pair <size_t, size_t>> ();
}
/**
 * @brief Метод выполнения регулярного выражения
 *
 * @param text текст для обработки
 * @param size размер текста для обработки
 * @param exp  объект регулярного выражения
 * @return     результат обработки регулярного выражения
 *
 */
vector <pair <size_t, size_t>> awh::Regular_Expressions::match(const char * text, const size_t size, const exp_t & exp) const noexcept {
	// Переменная результата
	vector <pair <size_t, size_t>> result;
	// Если данные переданы верные
	if((text != nullptr) && (size > 0) && static_cast <bool> (exp)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Небольшой буфер на стеке для типичных случаев, чтобы избежать аллокации в куче
			regmatch_t stack[16];
			// По умолчанию используем буфер на стеке
			regmatch_t * match = stack;
			// Указатель на динамический буфер совпадений (используется только при большом числе групп)
			unique_ptr <regmatch_t []> heap;
			// Получаем количество ожидаемых групп совпадений
			const size_t count = (exp->reg.re_nsub + 1);
			// Если число групп превышает размер стекового буфера
			if(count > (sizeof(stack) / sizeof(stack[0]))){
				// Выделяем буфер совпадений в куче
				heap.reset(new regmatch_t [count]);
				// Используем буфер в куче
				match = heap.get();
			}
			// Выполняем разбор регулярного выражения
			const int32_t error = ::pcre2_regexec(&exp->reg, text, count, match, REG_NOTEMPTY);
			// Если регулярное выражение успешно сопоставлено
			if(error == 0){
				// Выполняем создание результата
				result.resize(count);
				/**
				 * Выполняем перебор всех полученных вариантов
				 */
				for(size_t i = 0; i < count; i++){
					// Если результат получен
					if((match[i].rm_so >= 0) && (match[i].rm_eo > match[i].rm_so) && (static_cast <size_t> (match[i].rm_eo) <= size))
						// Добавляем полученный результат в список результатов
						result[i] = make_pair(static_cast <size_t> (match[i].rm_so), static_cast <size_t> (match[i].rm_eo - match[i].rm_so));
					// Добавляем пустое значение
					else result[i] = make_pair(static_cast <size_t> (0), static_cast <size_t> (0));
				}
			// Если получена реальная ошибка (отсутствие совпадения ошибкой не считаем)
			} else if(error != REG_NOMATCH) {
				// Создаём буфер данных для извлечения данных ошибки
				char buffer[0xFF];
				// Выполняем заполнение нулями буфер данных
				::memset(buffer, '\0', sizeof(buffer));
				// Выполняем извлечение текста ошибки
				const size_t length = ::pcre2_regerror(error, &exp->reg, buffer, sizeof(buffer));
				// Если текст ошибки получен
				if(length > 0){
					// Выполняем блокировку потока на время записи текста ошибки
					const locker_t <> lock(this->_mtx);
					// Выполняем установку кода ошибки (длина включает завершающий ноль)
					const_cast <regexp_t *> (this)->_error.assign(buffer, ::min(length - 1, sizeof(buffer) - 1));
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
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(text, size), log_t::flag_t::CRITICAL, "Memory allocation error");
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, "Memory allocation error");
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n", "Memory allocation error");
				#endif
			}
			// Выходим из приложения
			::_exit(EXIT_FAILURE);
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
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(text, size), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод сборки регулярного выражения
 *
 * @param pattern регулярное выражение для сборки
 * @param options список опций для сборки регулярного выражения
 * @return        результат собранного регулярного выражения
 *
 */
awh::Regular_Expressions::exp_t awh::Regular_Expressions::build(string_view pattern, const vector <option_t> & options) const noexcept {
	// Переменная результата
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
				/**
				 * Выполняем перебор всех переданных опций
				 */
				for(auto & item : options){
					/**
					 * Определяем тип переданной опции
					 */
					switch(static_cast <uint8_t> (item)){
						// Если передан флаг поддержки свойств Юникода
						case static_cast <uint8_t> (option_t::UCP):
							// Выполняем установку флага
							option |= REG_UCP;
						break;
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
			const auto key = make_pair(option, string{pattern});
			/**
			 * Выполняем поиск уже ранее созданного регулярного выражения под блокировкой,
			 * чтобы исключить гонку данных с конкурентными вставками и удалениями
			 */
			{
				// Выполняем блокировку потока
				const locker_t <> lock(this->_mtx);
				// Выполняем поиск уже ранее созданного регулярного выражения
				auto i = this->_cache.find(key);
				// Если регулярное выражение уже созданно
				if(i != this->_cache.end()){
					// Выполняем получение скомпилированного регулярного выражения
					result = i->second.lock();
					// Если регулярное выражение уже устарело и удалено
					if(result == nullptr)
						// Удаляем устаревшую запись по итератору
						this->_cache.erase(i);
				}
			}
			// Выполняем генерацию нового регулярного выражения
			if(result == nullptr){
				// Выполняем создание нового блока результата
				result = exp_t(new Expression);
				// Выполняем инициализацию объекта регулярного выражения
				result->reg = regex_t();
				/**
				 * Выполняем компиляцию регулярного выражения из null-терминированной строки
				 * (string_view может не иметь завершающего нуля, что недопустимо для POSIX-API)
				 */
				const int32_t error = ::pcre2_regcomp(&result->reg, key.second.c_str(), option);
				// Если возникла ошибка компиляции
				if(!((* result.get()) = static_cast <bool> (error == 0))){
					// Создаём буфер данных для извлечения данных ошибки
					char buffer[0xFF];
					// Выполняем заполнение нулями буфер данных
					::memset(buffer, '\0', sizeof(buffer));
					// Выполняем извлечение текста ошибки
					const size_t length = ::pcre2_regerror(error, &result->reg, buffer, sizeof(buffer));
					// Если текст ошибки получен
					if(length > 0){
						// Выполняем блокировку потока на время записи текста ошибки
						const locker_t <> lock(this->_mtx);
						// Выполняем установку кода ошибки (длина включает завершающий ноль)
						const_cast <regexp_t *> (this)->_error.assign(buffer, ::min(length - 1, sizeof(buffer) - 1));
					}
					// Выполняем удаление скомпилированного регулярного выражения
					::pcre2_regfree(&result->reg);
					// Выполняем сброс блока результата
					result.reset();
				// Если регулярное выражение удачно созданно
				} else {
					// Выполняем блокировку потока
					const locker_t <> lock(this->_mtx);
					/**
					 * Выполняем повторную проверку: другой поток мог успеть скомпилировать
					 * и закэшировать такой же паттерн, пока шла компиляция
					 */
					auto res = this->_cache.emplace(key, result);
					// Если запись с таким ключом уже существует
					if(!res.second){
						// Пытаемся получить уже закэшированное регулярное выражение
						if(auto existing = res.first->second.lock())
							// Используем уже закэшированный объект
							result = existing;
						// Если закэшированная запись устарела, заменяем её на новую
						else res.first->second = result;
					}
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
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(pattern, options.size()), log_t::flag_t::CRITICAL, "Memory allocation error");
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, "Memory allocation error");
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n", "Memory allocation error");
				#endif
			}
			// Выходим из приложения
			::_exit(EXIT_FAILURE);
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
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(pattern, options.size()), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод установки объекта логирования
 *
 * @param log объект работы с логами
 *
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
	this->_mtx.enabled = false;
}
/**
 * @brief Конструктор
 *
 * @param log объект работы с логами
 *
 */
awh::Regular_Expressions::Regular_Expressions(const log_t * log) noexcept : _error{""}, _log(log) {
	/**
	 * Деактивируем мьютексы на время инициализации
	 */
	this->_mtx.enabled = false;
}
/**
 * @brief Деструктор
 *
 */
awh::Regular_Expressions::~Regular_Expressions() noexcept {}
