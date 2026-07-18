/**
 * @file: http.cpp
 * @date: 2026-07-18
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

/**
 * Стандартные заголовочные файлы
 */
#include <array>
#include <cstring>

/**
 * Подключаем заголовочный файл проекта
 */
#include <proto/http/parser/http1/http.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Внутренние вспомогательные функции и таблицы (внутренняя компоновка)
 */
namespace {
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Внутренние состояния конечного автомата
	 *
	 */
	enum state_t : uint8_t {
		// Общий старт (диспетчеризация по направлению трафика)
		S_START = 0x00,
		/**
		 * Стартовая строка запроса клиента (request-line)
		 */
		S_REQ_METHOD           = 0x01, // Метод запроса
		S_REQ_TARGET_START     = 0x02, // Пробелы перед request-target
		S_REQ_TARGET           = 0x03, // Разбор request-target
		S_REQ_HTTP_START       = 0x04, // Пробелы перед "HTTP/"
		S_REQ_HTTP_H           = 0x05, // Литерал "H"
		S_REQ_HTTP_HT          = 0x06, // Литерал "HT"
		S_REQ_HTTP_HTT         = 0x07, // Литерал "HTT"
		S_REQ_HTTP_HTTP        = 0x08, // Литерал "HTTP"
		S_REQ_HTTP_SLASH       = 0x09, // Литерал "HTTP/"
		S_REQ_HTTP_DOT         = 0x0A, // Точка между major и minor
		S_REQ_HTTP_MINOR       = 0x0B, // Минорная цифра версии
		S_REQ_LINE_ALMOST_DONE = 0x0C, // Ожидание CR/LF после версии
		S_REQ_LINE_LF          = 0x0D, // Ожидание LF после CR
		/**
		 * Стартовая строка ответа сервера (status-line)
		 */
		S_RES_HTTP_H       = 0x0E, // Литерал "H"
		S_RES_HTTP_HT      = 0x0F, // Литерал "HT"
		S_RES_HTTP_HTT     = 0x10, // Литерал "HTT"
		S_RES_HTTP_HTTP    = 0x11, // Литерал "HTTP"
		S_RES_HTTP_SLASH   = 0x12, // Литерал "HTTP/"
		S_RES_HTTP_DOT     = 0x13, // Точка между major и minor
		S_RES_HTTP_MINOR   = 0x14, // Минорная цифра версии
		S_RES_FIRST_SPACE  = 0x15, // Обязательный пробел после версии
		S_RES_STATUS_START = 0x16, // Пробелы перед статус-кодом
		S_RES_STATUS_CODE  = 0x17, // Разбор статус-кода
		S_RES_REASON_START = 0x18, // Пробел перед reason-phrase
		S_RES_REASON       = 0x19, // Разбор reason-phrase
		S_RES_LINE_LF      = 0x1A, // Ожидание LF после CR
		/**
		 * Заголовки сообщения
		 */
		S_HEADER_START       = 0x1B, // Начало строки заголовка
		S_HEADER_NAME        = 0x1C, // Разбор имени заголовка
		S_HEADER_VALUE_OWS   = 0x1D, // Пропуск ведущих OWS значения
		S_HEADER_VALUE       = 0x1E, // Разбор значения заголовка
		S_HEADER_ALMOST_DONE = 0x1F, // Ожидание LF после CR
		S_HEADERS_LF         = 0x20, // Ожидание LF финальной пустой строки
		/**
		 * Тело сообщения
		 */
		S_BODY_IDENTITY    = 0x21, // Тело фиксированного размера (Content-Length)
		S_BODY_UNTIL_CLOSE = 0x22, // Тело до закрытия соединения
		/**
		 * Кодирование chunked
		 */
		S_CHUNK_SIZE             = 0x23, // Разбор hex-размера чанка
		S_CHUNK_EXT              = 0x24, // Пропуск расширений чанка (chunk-ext)
		S_CHUNK_SIZE_LF          = 0x25, // Ожидание LF после CR строки размера
		S_CHUNK_DATA             = 0x26, // Данные чанка
		S_CHUNK_DATA_ALMOST_DONE = 0x27, // Ожидание CR/LF после данных чанка
		S_CHUNK_DATA_LF          = 0x28, // Ожидание LF после CR
		/**
		 * Трейлеры сообщения
		 */
		S_TRAILER_START       = 0x29, // Начало строки трейлера
		S_TRAILER_NAME        = 0x2A, // Разбор имени трейлера
		S_TRAILER_VALUE_OWS   = 0x2B, // Пропуск ведущих OWS значения
		S_TRAILER_VALUE       = 0x2C, // Разбор значения трейлера
		S_TRAILER_ALMOST_DONE = 0x2D, // Ожидание LF после CR
		S_TRAILERS_LF         = 0x2E, // Ожидание LF финальной пустой строки
		/**
		 * Финал разбора сообщения
		 */
		S_MESSAGE_DONE = 0x2F // Сообщение полностью разобрано
	};

	/**
	 * @brief Функция генерации таблицы токенов (RFC 7230): ALPHA / DIGIT / "!#$%&'*+-.^_`|~"
	 *
	 * @return таблица токенов
	 */
	constexpr array <bool, 256> makeTokenTable() noexcept {
		// Результат работы функции
		array <bool, 256> result{};
		/**
		 * Выполняем перебор всех символов таблицы
		 */
		for(uint16_t letter = 0; letter < 256; ++letter)
			// Помечаем буквы и цифры как допустимые
			result[letter] = (
				((letter >= 'a') && (letter <= 'z')) ||
				((letter >= 'A') && (letter <= 'Z')) ||
				((letter >= '0') && (letter <= '9'))
			);
		// Список специальных символов допустимых в токенах
		const char specials[] = "!#$%&'*+-.^_`|~";
		/**
		 * Выполняем перебор всех специальных символов
		 */
		for(size_t i = 0; specials[i] != '\0'; ++i)
			// Помечаем специальный символ как допустимый
			result[static_cast <uint8_t> (specials[i])] = true;
		// Выводим результат
		return result;
	}
	/**
	 * @brief Функция генерации таблицы допустимых символов значения заголовка:
	 *        HTAB / SP / VCHAR(0x21..0x7E) / obs-text(0x80..0xFF)
	 *
	 * @return таблица допустимых символов значения заголовка
	 */
	constexpr array <bool, 256> makeValueTable() noexcept {
		// Результат работы функции
		array <bool, 256> result{};
		/**
		 * Выполняем перебор всех символов таблицы
		 */
		for(uint16_t letter = 0; letter < 256; ++letter)
			// Помечаем допустимые символы значения заголовка
			result[letter] = (
				(letter == 0x09) || (letter == 0x20) ||
				((letter >= 0x21) && (letter <= 0x7E)) ||
				((letter >= 0x80) && (letter <= 0xFF))
			);
		// Выводим результат
		return result;
	}
	/**
	 * @brief Функция генерации таблицы допустимых символов request-target: VCHAR без пробела (0x21..0x7E)
	 *
	 * @return таблица допустимых символов request-target
	 */
	constexpr array <bool, 256> makeTargetTable() noexcept {
		// Результат работы функции
		array <bool, 256> result{};
		/**
		 * Выполняем перебор всех видимых символов
		 */
		for(uint16_t letter = 0x21; letter <= 0x7E; ++letter)
			// Помечаем видимый символ как допустимый
			result[letter] = true;
		// Выводим результат
		return result;
	}

	/**
	 * @brief Таблица токенов (считается на этапе компиляции)
	 *
	 */
	constexpr auto tokenTable = makeTokenTable();
	/**
	 * @brief Таблица допустимых символов значения заголовка (считается на этапе компиляции)
	 *
	 */
	constexpr auto valueTable = makeValueTable();
	/**
	 * @brief Таблица допустимых символов request-target (считается на этапе компиляции)
	 *
	 */
	constexpr auto targetTable = makeTargetTable();

	/**
	 * @brief Функция проверки принадлежности символа к токенам
	 *
	 * @param c проверяемый символ
	 * @return  результат проверки
	 */
	inline bool isToken(const uint8_t letter) noexcept {
		// Выполняем проверку по таблице токенов
		return tokenTable[letter];
	}
	/**
	 * @brief Функция проверки допустимости символа в значении заголовка (и reason-phrase)
	 *
	 * @param letter проверяемый символ
	 * @return       результат проверки
	 */
	inline bool isValueCh(const uint8_t letter) noexcept {
		// Выполняем проверку по таблице допустимых символов значения заголовка
		return valueTable[letter];
	}
	/**
	 * @brief Функция проверки допустимости символа в request-target
	 *
	 * @param letter проверяемый символ
	 * @return       результат проверки
	 */
	inline bool isTargetCh(const uint8_t letter) noexcept {
		// Выполняем проверку по таблице допустимых символов request-target
		return targetTable[letter];
	}
	/**
	 * @brief Функция проверки принадлежности символа к десятичным цифрам
	 *
	 * @param letter проверяемый символ
	 * @return       результат проверки
	 */
	inline bool isDigit(const uint8_t letter) noexcept {
		// Выполняем проверку принадлежности символа к десятичным цифрам
		return ((letter >= '0') && (letter <= '9'));
	}
	/**
	 * @brief Функция получения числового значения шестнадцатеричной цифры
	 *
	 * @param letter проверяемый символ
	 * @return       числовое значение цифры либо -1, если символ не является hex-цифрой
	 */
	inline int8_t hexVal(const uint8_t letter) noexcept {
		// Если символ является десятичной цифрой
		if((letter >= '0') && (letter <= '9'))
			// Выводим числовое значение цифры
			return static_cast <int8_t> (letter - '0');
		// Если символ является строчной hex-буквой
		if((letter >= 'a') && (letter <= 'f'))
			// Выводим числовое значение цифры
			return static_cast <int8_t> (letter - 'a' + 10);
		// Если символ является прописной hex-буквой
		if((letter >= 'A') && (letter <= 'F'))
			// Выводим числовое значение цифры
			return static_cast <int8_t> (letter - 'A' + 10);
		// Символ не является hex-цифрой
		return -1;
	}
	/**
	 * @brief Функция приведения символа к нижнему регистру
	 *
	 * @param letter приводимый символ
	 * @return       символ в нижнем регистре
	 */
	inline char lower(const char letter) noexcept {
		// Выполняем приведение символа к нижнему регистру
		return (((letter >= 'A') && (letter <= 'Z')) ? static_cast <char> (letter - 'A' + 'a') : letter);
	}
	/**
	 * @brief Функция сравнения строки с литералом без учёта регистра (литерал в нижнем регистре)
	 *
	 * @param str      сравниваемая строка
	 * @param size     размер сравниваемой строки
	 * @param litLower литерал в нижнем регистре
	 * @return         результат сравнения
	 */
	bool iequalsLit(const char * str, const size_t size, const char * litLower) noexcept {
		/**
		 * Выполняем перебор всех символов сравниваемой строки
		 */
		for(size_t i = 0; i < size; ++i){
			// Если литерал закончился раньше строки - строки не равны
			if(litLower[i] == '\0')
				// Строки не равны
				return false;
			// Если символы не совпадают - строки не равны
			if(lower(str[i]) != litLower[i])
				// Строки не равны
				return false;
		}
		// Строки равны, если литерал закончилась одновременно со строкой
		return (litLower[size] == '\0');
	}
	/**
	 * @brief Функция сравнения строки с литералом без учёта регистра (литерал в нижнем регистре)
	 *
	 * @param str      сравниваемая строка
	 * @param litLower литерал в нижнем регистре
	 * @return         результат сравнения
	 */
	bool iequalsLit(const string & str, const char * litLower) noexcept {
		// Выполняем сравнение строки с литералом
		return iequalsLit(str.c_str(), str.length(), litLower);
	}
	/**
	 * @brief Функция разбора десятичного числа с контролем переполнения
	 *
	 * @param str  указатель на строку с числом
	 * @param size размер строки с числом
	 * @param out  результирующее число
	 * @return     результат разбора
	 */
	bool parseDecimal(const char * str, const size_t size, uint64_t & out) noexcept {
		// Пустая строка числом не является
		if(size == 0)
			// Разбор не удался
			return false;
		// Результирующее значение числа
		uint64_t value = 0;
		/**
		 * Выполняем перебор всех символов строки
		 */
		for(size_t i = 0; i < size; ++i){
			// Если символ не является десятичной цифрой
			if(!isDigit(static_cast <uint8_t> (str[i])))
				// Разбор не удался
				return false;
			// Получаем числовое значение цифры
			const uint64_t digit = static_cast <uint64_t> (str[i] - '0');
			// Если добавление цифры приведёт к переполнению
			if(value > ((UINT64_MAX - digit) / 10ull))
				// Разбор не удался
				return false;
			// Добавляем цифру к результирующему значению
			value = ((value * 10ull) + digit);
		}
		// Устанавливаем результирующее значение
		out = value;
		// Разбор выполнен успешно
		return true;
	}
	/**
	 * @brief Функция триминга OWS (SP/HTAB) по краям подстроки [begin, end)
	 *
	 * @param begin начало подстроки
	 * @param end   конец подстроки
	 */
	void trimOWS(const char *& begin, const char *& end) noexcept {
		/**
		 * Пропускаем ведущие пробелы и табуляции
		 */
		while((begin < end) && ((* begin == ' ') || (* begin == '\t')))
			// Смещаем начало подстроки
			++begin;
		/**
		 * Пропускаем хвостовые пробелы и табуляции
		 */
		while((end > begin) && ((end[-1] == ' ') || (end[-1] == '\t')))
			// Смещаем конец подстроки
			--end;
	}
	/**
	 * @brief Функция классификации метода запроса по его имени
	 *
	 * @param method имя метода запроса
	 * @param fmk    объект фреймворка
	 * @return       распознанный метод запроса либо method_t::NONE
	 */
	http::method_t classifyMethod(const string & method, const fmk_t * fmk) noexcept {
		/**
		 * Диспетчеризация по длине имени метода (operator== сравнивает размер первым)
		 */
		switch(method.size()){
			// Методы с длиной имени 3 символа
			case 3: {
				// Если метод совпадает с известным методом запроса GET
				if(fmk->compare("GET", method))
					// Выводим распознанный метод запроса
					return http::method_t::GET;
				// Если метод совпадает с известным методом запроса PUT
				if(fmk->compare("PUT", method))
					// Выводим распознанный метод запроса
					return http::method_t::PUT;
				// Если метод совпадает с известным методом запроса ACL
				if(fmk->compare("ACL", method))
					// Выводим распознанный метод запроса
					return http::method_t::ACL;
				// Если метод совпадает с известным методом запроса PRI
				if(fmk->compare("PRI", method))
					// Выводим распознанный метод запроса
					return http::method_t::PRI;
			} break;
			// Методы с длиной имени 4 символа
			case 4: {
				// Если метод совпадает с известным методом запроса HEAD
				if(fmk->compare("HEAD", method))
					// Выводим распознанный метод запроса
					return http::method_t::HEAD;
				// Если метод совпадает с известным методом запроса POST
				if(fmk->compare("POST", method))
					// Выводим распознанный метод запроса
					return http::method_t::POST;
				// Если метод совпадает с известным методом запроса COPY
				if(fmk->compare("COPY", method))
					// Выводим распознанный метод запроса
					return http::method_t::COPY;
				// Если метод совпадает с известным методом запроса LOCK
				if(fmk->compare("LOCK", method))
					// Выводим распознанный метод запроса
					return http::method_t::LOCK;
				// Если метод совпадает с известным методом запроса MOVE
				if(fmk->compare("MOVE", method))
					// Выводим распознанный метод запроса
					return http::method_t::MOVE;
				// Если метод совпадает с известным методом запроса BIND
				if(fmk->compare("BIND", method))
					// Выводим распознанный метод запроса
					return http::method_t::BIND;
				// Если метод совпадает с известным методом запроса LINK
				if(fmk->compare("LINK", method))
					// Выводим распознанный метод запроса
					return http::method_t::LINK;
			} break;
			// Методы с длиной имени 5 символов
			case 5: {
				// Если метод совпадает с известным методом запроса TRACE
				if(fmk->compare("TRACE", method))
					// Выводим распознанный метод запроса
					return http::method_t::TRACE;
				// Если метод совпадает с известным методом запроса PATCH
				if(fmk->compare("PATCH", method))
					// Выводим распознанный метод запроса
					return http::method_t::PATCH;
				// Если метод совпадает с известным методом запроса MKCOL
				if(fmk->compare("MKCOL", method))
					// Выводим распознанный метод запроса
					return http::method_t::MKCOL;
				// Если метод совпадает с известным методом запроса MERGE
				if(fmk->compare("MERGE", method))
					// Выводим распознанный метод запроса
					return http::method_t::MERGE;
				// Если метод совпадает с известным методом запроса PURGE
				if(fmk->compare("PURGE", method))
					// Выводим распознанный метод запроса
					return http::method_t::PURGE;
			} break;
			// Методы с длиной имени 6 символов
			case 6: {
				// Если метод совпадает с известным методом запроса DELETE
				if(fmk->compare("DELETE", method))
					// Выводим распознанный метод запроса
					return http::method_t::DEL;
				// Если метод совпадает с известным методом запроса SEARCH
				if(fmk->compare("SEARCH", method))
					// Выводим распознанный метод запроса
					return http::method_t::SEARCH;
				// Если метод совпадает с известным методом запроса UNLOCK
				if(fmk->compare("UNLOCK", method))
					// Выводим распознанный метод запроса
					return http::method_t::UNLOCK;
				// Если метод совпадает с известным методом запроса REBIND
				if(fmk->compare("REBIND", method))
					// Выводим распознанный метод запроса
					return http::method_t::REBIND;
				// Если метод совпадает с известным методом запроса UNBIND
				if(fmk->compare("UNBIND", method))
					// Выводим распознанный метод запроса
					return http::method_t::UNBIND;
				// Если метод совпадает с известным методом запроса REPORT
				if(fmk->compare("REPORT", method))
					// Выводим распознанный метод запроса
					return http::method_t::REPORT;
				// Если метод совпадает с известным методом запроса NOTIFY
				if(fmk->compare("NOTIFY", method))
					// Выводим распознанный метод запроса
					return http::method_t::NOTIFY;
				// Если метод совпадает с известным методом запроса SOURCE
				if(fmk->compare("SOURCE", method))
					// Выводим распознанный метод запроса
					return http::method_t::SOURCE;
				// Если метод совпадает с известным методом запроса UNLINK
				if(fmk->compare("UNLINK", method))
					// Выводим распознанный метод запроса
					return http::method_t::UNLINK;
			} break;
			// Методы с длиной имени 7 символов
			case 7: {
				// Если метод совпадает с известным методом запроса CONNECT
				if(fmk->compare("CONNECT", method))
					// Выводим распознанный метод запроса
					return http::method_t::CONNECT;
				// Если метод совпадает с известным методом запроса OPTIONS
				if(fmk->compare("OPTIONS", method))
					// Выводим распознанный метод запроса
					return http::method_t::OPTIONS;
			} break;
			// Методы с длиной имени 8 символов
			case 8: {
				// Если метод совпадает с известным методом запроса PROPFIND
				if(fmk->compare("PROPFIND", method))
					// Выводим распознанный метод запроса
					return http::method_t::PROPFIND;
				// Если метод совпадает с известным методом запроса CHECKOUT
				if(fmk->compare("CHECKOUT", method))
					// Выводим распознанный метод запроса
					return http::method_t::CHECKOUT;
				// Если метод совпадает с известным методом запроса M-SEARCH
				if(fmk->compare("M-SEARCH", method))
					// Выводим распознанный метод запроса
					return http::method_t::MSEARCH;
			} break;
			// Методы с длиной имени 9 символов
			case 9: {
				// Если метод совпадает с известным методом запроса PROPPATCH
				if(fmk->compare("PROPPATCH", method))
					// Выводим распознанный метод запроса
					return http::method_t::PROPPATCH;
				// Если метод совпадает с известным методом запроса SUBSCRIBE
				if(fmk->compare("SUBSCRIBE", method))
					// Выводим распознанный метод запроса
					return http::method_t::SUBSCRIBE;
			} break;
			// Методы с длиной имени 10 символов
			case 10: {
				// Если метод совпадает с известным методом запроса MKACTIVITY
				if(fmk->compare("MKACTIVITY", method))
					// Выводим распознанный метод запроса
					return http::method_t::MKACTIVITY;
				// Если метод совпадает с известным методом запроса MKCALENDAR
				if(fmk->compare("MKCALENDAR", method))
					// Выводим распознанный метод запроса
					return http::method_t::MKCALENDAR;
			} break;
			// Методы с длиной имени 11 символов
			case 11: {
				// Если метод совпадает с известным методом запроса UNSUBSCRIBE
				if(fmk->compare("UNSUBSCRIBE", method))
					// Выводим распознанный метод запроса
					return http::method_t::UNSUBSCRIBE;
			} break;
		}
		// Метод не распознан
		return http::method_t::NONE;
	}
};

/**
 * @brief Метод вызова функции обратного вызова обработки фазы разбора
 *
 * @param phase фаза разбора HTTP-сообщения
 * @param part  часть сообщения
 * @return      результат обработки (false - разбор прерван)
 */
bool awh::http::Parser_HTTP::firePhase(const phase_t phase, const part_t part) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callbacks.phase != nullptr){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если функция обратного вызова потребовала прервать разбор
			if(!this->_callbacks.phase(phase, part)){
				// Фиксируем ошибку прерывания разбора
				this->_error = error_t::ABORTED;
				// Разбор прерван
				return false;
			}
		// Если функция обратного вызова бросила исключение
		} catch(const exception &) {
			// Фиксируем внутреннюю ошибку
			this->_error = error_t::INTERNAL;
			// Разбор прерван
			return false;
		}
	}
	// Продолжаем разбор
	return true;
}
/**
 * @brief Метод вызова функции обратного вызова обработки границ чанков
 *
 * @param phase фаза разбора чанка
 * @param size  размер данных чанка
 * @return      результат обработки (false - разбор прерван)
 */
bool awh::http::Parser_HTTP::fireChunk(const phase_t phase, const uint64_t size) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callbacks.chunk != nullptr){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если функция обратного вызова потребовала прервать разбор
			if(!this->_callbacks.chunk(phase, size, string_view(this->_header.chunkExt.data(), this->_header.chunkExt.size()))){
				// Фиксируем ошибку прерывания разбора
				this->_error = error_t::ABORTED;
				// Разбор прерван
				return false;
			}
		// Если функция обратного вызова бросила исключение
		} catch(const exception &) {
			// Фиксируем внутреннюю ошибку
			this->_error = error_t::INTERNAL;
			// Разбор прерван
			return false;
		}
	}
	// Продолжаем разбор
	return true;
}
/**
 * @brief Метод завершения разбора стартовой строки (request-line/status-line)
 *
 * @return результат обработки (false - разбор прерван)
 */
bool awh::http::Parser_HTTP::commitStartLine() noexcept {
	// Переходим к разбору заголовков
	this->_state = static_cast <uint8_t> (S_HEADER_START);
	// Если функция обратного вызова установлена
	if(this->_callbacks.provider != nullptr){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если функция обратного вызова потребовала прервать разбор
			if(!this->_callbacks.provider(this->_message.provider.get())){
				// Фиксируем ошибку прерывания разбора
				this->_error = error_t::ABORTED;
				// Разбор прерван
				return false;
			}
		// Если функция обратного вызова бросила исключение
		} catch(const exception &) {
			// Фиксируем внутреннюю ошибку
			this->_error = error_t::INTERNAL;
			// Разбор прерван
			return false;
		}
	}
	// Продолжаем разбор
	return true;
}
/**
 * @brief Метод завершения разбора текущего заголовка/трейлера
 *
 * @return результат обработки (false - разбор прерван)
 */
bool awh::http::Parser_HTTP::commitHeader() noexcept {
	// Выполняем триминг хвостовых OWS у значения (ведущие уже пропущены состоянием OWS)
	while(!this->_header.value.empty() && ((this->_header.value.back() == ' ') || (this->_header.value.back() == '\t')))
		// Удаляем хвостовой пробел или табуляцию
		this->_header.value.pop_back();
	// Если превышено максимальное число заголовков
	if(++this->_statsHeaders.count > this->_limits.maxHeaderCount){
		// Фиксируем ошибку превышения числа заголовков
		this->_error = error_t::TOO_MANY_HEADERS;
		// Разбор прерван
		return false;
	}
	// Наращиваем суммарный размер разобранных заголовков
	this->_statsHeaders.bytes += (this->_header.name.size() + this->_header.value.size());
	// Если превышен суммарный размер всех заголовков
	if(this->_statsHeaders.bytes > this->_limits.maxHeadersTotal){
		// Фиксируем ошибку превышения размера заголовков
		this->_error = error_t::HEADER_OVERFLOW;
		// Разбор прерван
		return false;
	}
	// Если выполняется разбор основных заголовков (трейлеры не влияют на кадрирование)
	if(!this->_flags.inTrailers){
		// Получаем указатель на начало значения заголовка
		const char * begin = this->_header.value.data();
		// Получаем указатель на конец значения заголовка
		const char * end = (begin + this->_header.value.size());
		/**
		 * Интерпретация специальных заголовков (диспетчер по первой букве - дёшево)
		 */
		switch(this->_header.name.empty() ? '\0' : lower(this->_header.name.front())){
			// Заголовки начинающиеся на "C"
			case 'c': {
				// Если получен заголовок Content-Length
				if(iequalsLit(this->_header.name, "content-length")){
					// Если интерпретация заголовка Content-Length не удалась
					if(!this->applyContentLength(begin, end))
						// Разбор прерван (код ошибки уже установлен)
						return false;
				// Если получен заголовок Connection
				} else if(iequalsLit(this->_header.name, "connection"))
					// Выполняем интерпретацию заголовка Connection
					this->applyConnection(begin, end);
			} break;
			// Заголовки начинающиеся на "T"
			case 't': {
				// Если получен заголовок Transfer-Encoding
				if(iequalsLit(this->_header.name, "transfer-encoding"))
					// Выполняем интерпретацию заголовка Transfer-Encoding
					this->applyTransferEncoding(begin, end);
			} break;
			// Заголовки начинающиеся на "U"
			case 'u': {
				// Если получен заголовок Upgrade
				if(iequalsLit(this->_header.name, "upgrade"))
					// Помечаем что заголовок Upgrade получен
					this->_flags.upgradeSeen = true;
			} break;
			// Заголовки начинающиеся на "E"
			case 'e': {
				// Если получен заголовок Expect со значением 100-continue (только для запросов)
				if((this->_direct == direct_t::REQUEST) && iequalsLit(this->_header.name, "expect") && iequalsLit(this->_header.value, "100-continue"))
					// Помечаем что клиент ожидает промежуточный ответ [100 Continue]
					this->_message.flags.expectContinue = true;
			} break;
		}
	}
	// Если функция обратного вызова установлена
	if(this->_callbacks.header != nullptr){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если функция обратного вызова потребовала прервать разбор
			if(!this->_callbacks.header(
				string_view(this->_header.name.data(), this->_header.name.size()),
				string_view(this->_header.value.data(), this->_header.value.size()),
				(this->_flags.inTrailers ? part_t::TRAILER : part_t::HEADERS)
			)){
				// Фиксируем ошибку прерывания разбора
				this->_error = error_t::ABORTED;
				// Разбор прерван
				return false;
			}
		// Если функция обратного вызова бросила исключение
		} catch(const exception &) {
			// Фиксируем внутреннюю ошибку
			this->_error = error_t::INTERNAL;
			// Разбор прерван
			return false;
		}
	}
	// Очищаем накопитель имени текущего заголовка
	this->_header.name.clear();
	// Очищаем накопитель значения текущего заголовка
	this->_header.value.clear();
	// Продолжаем разбор
	return true;
}
/**
 * @brief Метод проверки отсутствия тела у ответа сервера (по статус-коду и методу запроса)
 *
 * @return результат проверки
 */
bool awh::http::Parser_HTTP::responseHasNoBody() const noexcept {
	// Получаем статус-код ответа сервера
	const uint16_t code = static_cast <const response_t *> (this->_message.provider.get())->code;
	// Ответ на запрос методом HEAD тела не имеет (даже при наличии Content-Length)
	if(this->_method == method_t::HEAD)
		// Тело отсутствует
		return true;
	// Успешный (2xx) ответ на CONNECT открывает туннель - тела нет
	if((this->_method == method_t::CONNECT) && (code >= 200) && (code < 300))
		// Тело отсутствует
		return true;
	// Информационные (1xx) ответы тела не имеют
	if((code >= 100) && (code < 200))
		// Тело отсутствует
		return true;
	// Ответы [204 No Content] и [304 Not Modified] тела не имеют
	if((code == 204) || (code == 304))
		// Тело отсутствует
		return true;
	// Тело присутствует
	return false;
}
/**
 * @brief Метод интерпретации заголовка Content-Length
 *
 * @param begin начало значения заголовка
 * @param end   конец значения заголовка
 * @return      результат интерпретации
 */
bool awh::http::Parser_HTTP::applyContentLength(const char * begin, const char * end) noexcept {
	// Указатель на текущую позицию разбора
	const char * cur = begin;
	// Флаг получения первого значения
	bool firstSet = false;
	// Первое полученное значение
	uint64_t first = 0;
	/**
	 * Значение может быть списком "5, 5" - все элементы обязаны совпадать
	 */
	while(cur <= end){
		// Выполняем поиск разделителя списка
		const char * comma = static_cast <const char *> (::memchr(cur, ',', static_cast <size_t> (end - cur)));
		// Определяем конец текущего элемента списка
		const char * tokEnd = (comma != nullptr ? comma : end);
		// Указатель на начало элемента списка
		const char * tb = cur;
		// Указатель на конец элемента списка
		const char * te = tokEnd;
		// Выполняем триминг OWS по краям элемента списка
		trimOWS(tb, te);
		// Значение текущего элемента списка
		uint64_t value = 0;
		// Если разбор десятичного числа не удался
		if(!parseDecimal(tb, static_cast <size_t> (te - tb), value)){
			// Фиксируем ошибку некорректного Content-Length
			this->_error = error_t::INVALID_CONTENT_LENGTH;
			// Интерпретация не удалась
			return false;
		}
		// Если это первое полученное значение - запоминаем его
		if(!firstSet){
			// Запоминаем первое значение
			first = value;
			// Помечаем что первое значение получено
			firstSet = true;
		// Если значения в списке различаются
		} else if(value != first) {
			// Фиксируем ошибку конфликта Content-Length (request smuggling)
			this->_error = error_t::CONTENT_LENGTH_CONFLICT;
			// Интерпретация не удалась
			return false;
		}
		// Если разделителей больше нет - разбор списка завершён
		if(comma == nullptr)
			// Выходим из цикла
			break;
		// Переходим к следующему элементу списка
		cur = (comma + 1);
	}
	// Если ни одного значения не получено
	if(!firstSet){
		// Фиксируем ошибку некорректного Content-Length
		this->_error = error_t::INVALID_CONTENT_LENGTH;
		// Интерпретация не удалась
		return false;
	}
	// Если Content-Length уже был получен ранее и значения различаются
	if(this->_flags.contentLengthSeen && (this->_statsBody.contentLength != first)){
		// Фиксируем ошибку конфликта Content-Length (request smuggling)
		this->_error = error_t::CONTENT_LENGTH_CONFLICT;
		// Интерпретация не удалась
		return false;
	}
	// Помечаем что заголовок Content-Length получен
	this->_flags.contentLengthSeen = true;
	// Запоминаем значение заголовка Content-Length
	this->_statsBody.contentLength = first;
	// Интерпретация выполнена успешно
	return true;
}
/**
 * @brief Метод интерпретации заголовка Transfer-Encoding (накопительно по нескольким заголовкам)
 *
 * @param begin начало значения заголовка
 * @param end   конец значения заголовка
 */
void awh::http::Parser_HTTP::applyTransferEncoding(const char * begin, const char * end) noexcept {
	// Помечаем что заголовок Transfer-Encoding получен
	this->_flags.transferEncodingSeen = true;
	// Если предыдущий Transfer-Encoding уже заканчивался на chunked,
	// любой новый Transfer-Encoding делает chunked не последним - ошибка кадрирования
	if(this->_flags.transferEncodingChunkedFinal)
		// Помечаем Transfer-Encoding некорректным
		this->_flags.transferEncodingInvalid = true;
	// Указатель на текущую позицию разбора
	const char * cur = begin;
	// Флаг того, что последнее кодирование в списке - chunked
	bool lastChunked = false;
	/**
	 * Выполняем разбор списка кодирований
	 */
	while(cur <= end){
		// Выполняем поиск разделителя списка
		const char * comma = static_cast <const char *> (::memchr(cur, ',', static_cast <size_t> (end - cur)));
		// Определяем конец текущего элемента списка
		const char * tokEnd = (comma != nullptr ? comma : end);
		// Указатель на начало элемента списка
		const char * tb = cur;
		// Указатель на конец элемента списка
		const char * te = tokEnd;
		// Выполняем триминг OWS по краям элемента списка
		trimOWS(tb, te);
		// Если элемент списка не пустой
		if(te > tb){
			// Проверяем является ли кодирование chunked
			const bool isChunked = iequalsLit(tb, static_cast <size_t> (te - tb), "chunked");
			// Если chunked оказался не последним внутри строки - ошибка кадрирования
			if(lastChunked)
				// Помечаем Transfer-Encoding некорректным
				this->_flags.transferEncodingInvalid = true;
			// Запоминаем является ли текущее кодирование chunked
			lastChunked = isChunked;
		}
		// Если разделителей больше нет - разбор списка завершён
		if(comma == nullptr)
			// Выходим из цикла
			break;
		// Переходим к следующему элементу списка
		cur = (comma + 1);
	}
	// Запоминаем что последнее кодирование - chunked (если ошибок не выявлено)
	this->_flags.transferEncodingChunkedFinal = (lastChunked && !this->_flags.transferEncodingInvalid);
}
/**
 * @brief Метод интерпретации заголовка Connection
 *
 * @param begin начало значения заголовка
 * @param end   конец значения заголовка
 */
void awh::http::Parser_HTTP::applyConnection(const char * begin, const char * end) noexcept {
	// Указатель на текущую позицию разбора
	const char * cur = begin;
	/**
	 * Выполняем разбор списка токенов заголовка Connection
	 */
	while(cur <= end){
		// Выполняем поиск разделителя списка
		const char * comma = static_cast <const char *> (::memchr(cur, ',', static_cast <size_t> (end - cur)));
		// Определяем конец текущего элемента списка
		const char * tokEnd = (comma != nullptr ? comma : end);
		// Указатель на начало элемента списка
		const char * tb = cur;
		// Указатель на конец элемента списка
		const char * te = tokEnd;
		// Выполняем триминг OWS по краям элемента списка
		trimOWS(tb, te);
		// Получаем размер элемента списка
		const size_t n = static_cast <size_t> (te - tb);
		// Если элемент списка не пустой
		if(n > 0){
			// Если получен токен close
			if(iequalsLit(tb, n, "close"))
				// Помечаем что соединение должно быть закрыто
				this->_flags.connectionClose = true;
			// Если получен токен keep-alive
			else if(iequalsLit(tb, n, "keep-alive"))
				// Помечаем что соединение переиспользуемое
				this->_flags.connectionKeepAlive = true;
			// Если получен токен upgrade
			else if(iequalsLit(tb, n, "upgrade"))
				// Помечаем что запрошено переключение протокола
				this->_flags.connectionUpgrade = true;
		}
		// Если разделителей больше нет - разбор списка завершён
		if(comma == nullptr)
			// Выходим из цикла
			break;
		// Переходим к следующему элементу списка
		cur = (comma + 1);
	}
}
/**
 * @brief Метод завершения разбора всего сообщения
 *
 */
void awh::http::Parser_HTTP::completeMessage() noexcept {
	// Помечаем что сообщение полностью разобрано
	this->_message.flags.complete = true;
	// Устанавливаем финальное состояние конечного автомата
	this->_state = static_cast <uint8_t> (S_MESSAGE_DONE);
	// Устанавливаем фазу окончания разбора сообщения
	this->_message.phase = phase_t::END;
	// Сбрасываем партицию текущего состояния парсера
	this->_message.part = part_t::NONE;
	// Уведомляем о завершении разбора всего сообщения
	this->firePhase(phase_t::END, part_t::NONE);
}
/**
 * @brief Метод выбора способа кадрирования тела после завершения заголовков
 *
 */
void awh::http::Parser_HTTP::beginBody() noexcept {
	// Если одновременно получены Content-Length и Transfer-Encoding
	if(this->_flags.transferEncodingSeen && this->_flags.contentLengthSeen){
		// Фиксируем ошибку конфликта кадрирования (защита от request smuggling)
		this->_error = error_t::CONTENT_LENGTH_CONFLICT;
		// Выходим из метода
		return;
	}
	// Если заголовок Transfer-Encoding некорректен
	if(this->_flags.transferEncodingInvalid){
		// Фиксируем ошибку некорректного Transfer-Encoding
		this->_error = error_t::INVALID_TRANSFER_ENCODING;
		// Выходим из метода
		return;
	}
	// Устанавливаем флаг передачи тела chunked
	this->_message.flags.chunked = (this->_flags.transferEncodingSeen && this->_flags.transferEncodingChunkedFinal);
	// Устанавливаем ожидаемый размер тела сообщения (-1 если Content-Length не получен)
	this->_message.bodySize = (this->_flags.contentLengthSeen ? static_cast <int64_t> (this->_statsBody.contentLength) : -1);
	// Получаем версию протокола из провайдера заголовков сообщения
	const version_t version = this->_message.provider->version;
	// Если версия протокола HTTP/1.1 - соединение переиспользуемое, если не указан Connection: close
	if(version == version_t::HTTP1_1)
		// Устанавливаем флаг переиспользования соединения
		this->_message.flags.keepAlive = !this->_flags.connectionClose;
	// Если версия протокола HTTP/1.0 - соединение переиспользуемое только при явном Connection: keep-alive
	else if(version == version_t::HTTP1_0)
		// Устанавливаем флаг переиспользования соединения
		this->_message.flags.keepAlive = (this->_flags.connectionKeepAlive && !this->_flags.connectionClose);
	// Для всех остальных версий соединение не переиспользуемое
	else this->_message.flags.keepAlive = false;
	// Если выполняется разбор запроса клиента
	if(this->_direct == direct_t::REQUEST)
		// Переключение протокола запрошено при наличии заголовка Upgrade и токена upgrade в Connection
		this->_message.flags.upgrade = (this->_flags.upgradeSeen && this->_flags.connectionUpgrade);
	// Если выполняется разбор ответа сервера
	else {
		// Получаем статус-код ответа сервера
		const uint16_t code = static_cast <const response_t *> (this->_message.provider.get())->code;
		// Переключение протокола выполнено при ответе [101 Switching Protocols] или успешном ответе на CONNECT
		this->_message.flags.upgrade = ((code == 101) || ((this->_method == method_t::CONNECT) && (code >= 200) && (code < 300)));
	}
	// Уведомляем о завершении разбора всех заголовков (заголовки разобраны и осмыслены)
	if(!this->firePhase(phase_t::END, part_t::HEADERS))
		// Выходим из метода (разбор прерван)
		return;
	// Если ответ сервера не имеет тела по правилам RFC
	if((this->_direct == direct_t::RESPONSE) && this->responseHasNoBody()){
		// Завершаем разбор всего сообщения
		this->completeMessage();
		// Выходим из метода
		return;
	}
	// Если тело передаётся chunked
	if(this->_message.flags.chunked){
		// Сбрасываем размер текущего чанка
		this->_statsBody.chunkSize = 0;
		// Сбрасываем счётчик hex-цифр размера чанка
		this->_statsBody.digits = 0;
		// Сбрасываем длину строки заголовка чанка
		this->_statsHeaders.chunkLineBytes = 0;
		// Устанавливаем партицию тела сообщения
		this->_message.part = part_t::BODY;
		// Переходим к разбору размера первого чанка
		this->_state = static_cast <uint8_t> (S_CHUNK_SIZE);
		// Уведомляем о начале приёма тела сообщения
		this->firePhase(phase_t::BEGIN, part_t::BODY);
		// Выходим из метода
		return;
	}
	// Если получен заголовок Content-Length
	if(this->_flags.contentLengthSeen){
		// Если тело имеет нулевой размер
		if(this->_statsBody.contentLength == 0){
			// Завершаем разбор всего сообщения
			this->completeMessage();
			// Выходим из метода
			return;
		}
		// Если анонсированный размер тела превышает лимит
		if(this->_statsBody.contentLength > this->_limits.maxBodySize){
			// Фиксируем ошибку превышения размера тела
			this->_error = error_t::BODY_OVERFLOW;
			// Выходим из метода
			return;
		}
		// Устанавливаем остаток непрочитанных данных тела
		this->_statsBody.bytesRemaining = this->_statsBody.contentLength;
		// Устанавливаем партицию тела сообщения
		this->_message.part = part_t::BODY;
		// Переходим к чтению тела фиксированного размера
		this->_state = static_cast <uint8_t> (S_BODY_IDENTITY);
		// Уведомляем о начале приёма тела сообщения
		this->firePhase(phase_t::BEGIN, part_t::BODY);
		// Выходим из метода
		return;
	}
	// Если Transfer-Encoding получен, но chunked не является последним кодированием
	if(this->_flags.transferEncodingSeen){
		// Если выполняется разбор запроса клиента - такой запрос кадрировать невозможно
		if(this->_direct == direct_t::REQUEST){
			// Фиксируем ошибку некорректного Transfer-Encoding
			this->_error = error_t::INVALID_TRANSFER_ENCODING;
			// Выходим из метода
			return;
		}
		// Тело кадрируется закрытием соединения - оно не переиспользуемо
		this->_message.flags.keepAlive = false;
		// Устанавливаем партицию тела сообщения
		this->_message.part = part_t::BODY;
		// Переходим к чтению тела до закрытия соединения
		this->_state = static_cast <uint8_t> (S_BODY_UNTIL_CLOSE);
		// Уведомляем о начале приёма тела сообщения
		this->firePhase(phase_t::BEGIN, part_t::BODY);
		// Выходим из метода
		return;
	}
	// Если нет ни Content-Length ни Transfer-Encoding, а выполняется разбор запроса клиента
	if(this->_direct == direct_t::REQUEST){
		// У запроса по умолчанию тела нет - завершаем разбор всего сообщения
		this->completeMessage();
		// Выходим из метода
		return;
	}
	// Ответ сервера: тело до закрытия соединения - keep-alive невозможен
	this->_message.flags.keepAlive = false;
	// Устанавливаем партицию тела сообщения
	this->_message.part = part_t::BODY;
	// Переходим к чтению тела до закрытия соединения
	this->_state = static_cast <uint8_t> (S_BODY_UNTIL_CLOSE);
	// Уведомляем о начале приёма тела сообщения
	this->firePhase(phase_t::BEGIN, part_t::BODY);
}
/**
 * @brief Метод завершения разбора строки размера чанка
 *
 */
void awh::http::Parser_HTTP::chunkSizeComplete() noexcept {
	// Если в размере чанка не было ни одной hex-цифры
	if(this->_statsBody.digits == 0){
		// Фиксируем ошибку некорректного размера чанка
		this->_error = error_t::INVALID_CHUNK_SIZE;
		// Выходим из метода
		return;
	}
	// Если получен последний чанк (нулевого размера) - переходим к трейлерам
	if(this->_statsBody.chunkSize == 0){
		// Уведомляем о разборе заголовка последнего чанка (для END-события чанк данных не имеет)
		if(!this->fireChunk(phase_t::BEGIN, 0))
			// Выходим из метода (разбор прерван)
			return;
		// Очищаем накопитель расширений текущего чанка
		this->_header.chunkExt.clear();
		// Уведомляем о завершении приёма тела сообщения
		if(!this->firePhase(phase_t::END, part_t::BODY))
			// Выходим из метода (разбор прерван)
			return;
		// Даём трейлерам собственный бюджет лимитов (число/суммарный размер) - защита от DoS
		this->_statsHeaders.count = 0;
		// Сбрасываем суммарный размер разобранных заголовков
		this->_statsHeaders.bytes = 0;
		// Помечаем что выполняется разбор трейлеров
		this->_flags.inTrailers = true;
		// Устанавливаем партицию трейлеров сообщения
		this->_message.part = part_t::TRAILER;
		// Переходим к разбору трейлеров
		this->_state = static_cast <uint8_t> (S_TRAILER_START);
		// Уведомляем о начале разбора трейлеров
		this->firePhase(phase_t::BEGIN, part_t::TRAILER);
		// Выходим из метода
		return;
	}
	// Если размер чанка превышает лимит
	if(this->_statsBody.chunkSize > this->_limits.maxChunkSize){
		// Фиксируем ошибку превышения размера чанка
		this->_error = error_t::CHUNK_OVERFLOW;
		// Выходим из метода
		return;
	}
	// Уведомляем о разборе заголовка очередного чанка
	if(!this->fireChunk(phase_t::BEGIN, this->_statsBody.chunkSize))
		// Выходим из метода (разбор прерван)
		return;
	// Очищаем накопитель расширений текущего чанка
	this->_header.chunkExt.clear();
	// Устанавливаем остаток непрочитанных данных чанка
	this->_statsBody.bytesRemaining = this->_statsBody.chunkSize;
	// Переходим к чтению данных чанка
	this->_state = static_cast <uint8_t> (S_CHUNK_DATA);
}
/**
 * @brief Метод полной очистки всех данных парсера
 *
 * @details Помимо сброса состояния разбора возвращает лимиты безопасности
 *          к значениям по умолчанию и удаляет установленные функции обратного вызова
 */
void awh::http::Parser_HTTP::clear() noexcept {
	// Выполняем полную очистку данных базового парсера (сброс состояния + лимиты по умолчанию)
	parser_t::clear();
	// Удаляем все установленные функции обратного вызова
	this->_callbacks = callbacks_t();
}
/**
 * @brief Метод сброса парсера для разбора следующего сообщения в том же соединении
 *
 * @details Дешёвый сброс между сообщениями (keep-alive/pipelining): сохраняет лимиты
 *          безопасности и установленные функции обратного вызова, провайдер заголовков
 *          не пересоздаётся, а очищается (переиспользуется выделенная память)
 */
void awh::http::Parser_HTTP::reset() noexcept {
	// Выполняем сброс состояния базового парсера
	parser_t::reset();
	// Сбрасываем состояние конечного автомата
	this->_state = static_cast <uint8_t> (S_START);
	// Сбрасываем метод запроса, которому соответствует ожидаемый ответ
	this->_method = method_t::NONE;
	// Очищаем накопитель имени текущего заголовка (выделенная память строки сохраняется)
	this->_header.name.clear();
	// Очищаем накопитель значения текущего заголовка (выделенная память строки сохраняется)
	this->_header.value.clear();
	// Очищаем накопитель расширений текущего чанка (выделенная память строки сохраняется)
	this->_header.chunkExt.clear();
	// Сбрасываем количество разобранных заголовков
	this->_statsHeaders.count = 0;
	// Сбрасываем суммарный размер разобранных заголовков
	this->_statsHeaders.bytes = 0;
	// Сбрасываем длину текущей стартовой строки
	this->_statsHeaders.lineBytes = 0;
	// Сбрасываем длину текущей строки заголовка чанка
	this->_statsHeaders.chunkLineBytes = 0;
	// Сбрасываем общий размер принятого тела сообщения
	this->_statsBody.bytes = 0;
	// Сбрасываем остаток непрочитанных данных тела/чанка
	this->_statsBody.bytesRemaining = 0;
	// Сбрасываем размер текущего чанка
	this->_statsBody.chunkSize = 0;
	// Сбрасываем счётчик цифр
	this->_statsBody.digits = 0;
	// Сбрасываем флаг получения заголовка Content-Length
	this->_flags.contentLengthSeen = false;
	// Сбрасываем флаг получения заголовка Transfer-Encoding
	this->_flags.transferEncodingSeen = false;
	// Сбрасываем флаг последнего кодирования chunked
	this->_flags.transferEncodingChunkedFinal = false;
	// Сбрасываем флаг некорректности заголовка Transfer-Encoding
	this->_flags.transferEncodingInvalid = false;
	// Сбрасываем флаг наличия close в заголовке Connection
	this->_flags.connectionClose = false;
	// Сбрасываем флаг наличия keep-alive в заголовке Connection
	this->_flags.connectionKeepAlive = false;
	// Сбрасываем флаг наличия upgrade в заголовке Connection
	this->_flags.connectionUpgrade = false;
	// Сбрасываем флаг получения заголовка Upgrade
	this->_flags.upgradeSeen = false;
	// Сбрасываем флаг разбора трейлеров
	this->_flags.inTrailers = false;
	// Сбрасываем значение заголовка Content-Length
	this->_statsBody.contentLength = 0;
}
/**
 * @brief Метод клонирования объекта парсера
 *
 * @details Клон получает те же направление трафика, лимиты безопасности и функции
 *          обратного вызова, но чистое состояние разбора ("фабрика с теми же настройками")
 *
 * @return копия объекта парсера
 */
unique_ptr <awh::http::parser_t> awh::http::Parser_HTTP::clone() const noexcept {
	// Результат работы функции - копия объекта парсера
	unique_ptr <parser_t> result = nullptr;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Создаём новый объект парсера с теми же направлением трафика и объектами фреймворка
		unique_ptr <parser_http_t> parser = make_unique <parser_http_t> (this->_direct, this->_fmk, this->_log);
		// Копируем настроенные лимиты безопасности
		parser->_limits = this->_limits;
		// Копируем установленные функции обратного вызова
		parser->_callbacks = this->_callbacks;
		// Копируем метод запроса, которому соответствует ожидаемый ответ
		parser->_method = this->_method;
		// Перемещаем созданный объект парсера в результат
		result = ::move(parser);
	// Если возникает ошибка выделения памяти
	} catch(const exception &) {}
	// Выводим результат
	return result;
}
/**
 * @brief Метод разбора данных
 *
 * @details Потребляет столько байтов, сколько смог, и возвращает их число.
 *          Итоговый статус необходимо контролировать методом status():
 *          - PARTIAL:  данные приняты, сообщение не завершено - нужно ещё байтов;
 *          - COMPLETE: сообщение полностью разобрано, разбор остановлен ровно на границе
 *                      сообщения - для конвейерных (pipelined) сообщений вызовите reset()
 *                      и затем parse() на оставшемся хвосте буфера;
 *          - ERROR:    ошибка разбора/безопасности - причина в методе error().
 *
 * @param buffer буфер данных для разбора
 * @param size   размер данных для разбора
 * @return       количество обработанных байт данных
 */
size_t awh::http::Parser_HTTP::parse(const void * buffer, const size_t size) noexcept {
	// Если ранее зафиксирована ошибка разбора
	if(this->_error != error_t::NONE){
		// Устанавливаем итоговый статус разбора
		this->_status = status_t::ERROR;
		// Данные не обработаны
		return 0;
	}
	// Если сообщение уже полностью разобрано (требуется вызов reset)
	if(this->_state == static_cast <uint8_t> (S_MESSAGE_DONE)){
		// Устанавливаем итоговый статус разбора
		this->_status = status_t::COMPLETE;
		// Данные не обработаны
		return 0;
	}
	// Если данных для разбора нет
	if((buffer == nullptr) || (size == 0))
		// Данные не обработаны
		return 0;
	// Получаем указатель на данные для разбора
	const char * data = static_cast <const char *> (buffer);
	// Количество обработанных байт данных
	size_t i = 0;
	/**
	 * Парсер копит токены в std::string, поэтому теоретически возможен std::bad_alloc.
	 * Чтобы не нарушать noexcept-контракт, перехватываем всё и отдаём error_t::INTERNAL
	 */
	try {
		// Получаем объект провайдера заголовков запроса клиента (если разбирается запрос)
		request_t * req = (this->_direct == direct_t::REQUEST ? static_cast <request_t *> (this->_message.provider.get()) : nullptr);
		// Получаем объект провайдера заголовков ответа сервера (если разбирается ответ)
		response_t * res = (this->_direct == direct_t::RESPONSE ? static_cast <response_t *> (this->_message.provider.get()) : nullptr);
		/**
		 * Выполняем перебор всех байт входного буфера
		 */
		while(i < size){
			// Получаем текущий байт данных
			const uint8_t ch = static_cast <uint8_t> (data[i]);
			/**
			 * Bulk-состояния (тело/чанк): обрабатываем большими блоками без посимвольного разбора
			 */
			// Если читается тело фиксированного размера (Content-Length)
			if(this->_state == static_cast <uint8_t> (S_BODY_IDENTITY)){
				// Определяем количество доступных байт данных
				const uint64_t avail = static_cast <uint64_t> (size - i);
				// Определяем сколько байт данных можно забрать
				const uint64_t take = (avail < this->_statsBody.bytesRemaining ? avail : this->_statsBody.bytesRemaining);
				// Если данные для передачи есть
				if(take > 0){
					// Если функция обратного вызова установлена и потребовала прервать разбор
					if((this->_callbacks.body != nullptr) && !this->_callbacks.body(data + i, static_cast <size_t> (take))){
						// Фиксируем ошибку прерывания разбора
						this->_error = error_t::ABORTED;
						// Устанавливаем итоговый статус разбора
						this->_status = status_t::ERROR;
						// Выводим количество обработанных байт данных
						return i;
					}
					// Наращиваем общий размер принятого тела сообщения
					this->_statsBody.bytes += take;
				}
				// Смещаем позицию разбора
				i += static_cast <size_t> (take);
				// Уменьшаем остаток непрочитанных данных тела
				this->_statsBody.bytesRemaining -= take;
				// Если тело полностью принято
				if(this->_statsBody.bytesRemaining == 0){
					// Уведомляем о завершении приёма тела сообщения
					if(this->firePhase(phase_t::END, part_t::BODY))
						// Завершаем разбор всего сообщения
						this->completeMessage();
					// Устанавливаем итоговый статус разбора
					this->_status = (this->_error == error_t::NONE ? status_t::COMPLETE : status_t::ERROR);
					// Выводим количество обработанных байт данных
					return i;
				}
				// Продолжаем разбор
				continue;
			}
			// Если читается тело до закрытия соединения
			if(this->_state == static_cast <uint8_t> (S_BODY_UNTIL_CLOSE)){
				// Определяем количество доступных байт данных
				const size_t avail = (size - i);
				// Если общий размер тела превышает лимит
				if((this->_statsBody.bytes + avail) > this->_limits.maxBodySize){
					// Фиксируем ошибку превышения размера тела
					this->_error = error_t::BODY_OVERFLOW;
					// Устанавливаем итоговый статус разбора
					this->_status = status_t::ERROR;
					// Выводим количество обработанных байт данных
					return i;
				}
				// Если данные для передачи есть
				if(avail > 0){
					// Если функция обратного вызова установлена и потребовала прервать разбор
					if((this->_callbacks.body != nullptr) && !this->_callbacks.body(data + i, avail)){
						// Фиксируем ошибку прерывания разбора
						this->_error = error_t::ABORTED;
						// Устанавливаем итоговый статус разбора
						this->_status = status_t::ERROR;
						// Выводим количество обработанных байт данных
						return i;
					}
					// Наращиваем общий размер принятого тела сообщения
					this->_statsBody.bytes += avail;
				}
				// Смещаем позицию разбора
				i += avail;
				// Продолжаем разбор (завершение - только по вызову eof)
				continue;
			}
			// Если читаются данные чанка
			if(this->_state == static_cast <uint8_t> (S_CHUNK_DATA)){
				// Определяем количество доступных байт данных
				const uint64_t avail = static_cast <uint64_t> (size - i);
				// Определяем сколько байт данных можно забрать
				const uint64_t take = (avail < this->_statsBody.bytesRemaining ? avail : this->_statsBody.bytesRemaining);
				// Если общий размер тела превышает лимит
				if((this->_statsBody.bytes + take) > this->_limits.maxBodySize){
					// Фиксируем ошибку превышения размера тела
					this->_error = error_t::BODY_OVERFLOW;
					// Устанавливаем итоговый статус разбора
					this->_status = status_t::ERROR;
					// Выводим количество обработанных байт данных
					return i;
				}
				// Если данные для передачи есть
				if(take > 0){
					// Если функция обратного вызова установлена и потребовала прервать разбор
					if((this->_callbacks.body != nullptr) && !this->_callbacks.body(data + i, static_cast <size_t> (take))){
						// Фиксируем ошибку прерывания разбора
						this->_error = error_t::ABORTED;
						// Устанавливаем итоговый статус разбора
						this->_status = status_t::ERROR;
						// Выводим количество обработанных байт данных
						return i;
					}
					// Наращиваем общий размер принятого тела сообщения
					this->_statsBody.bytes += take;
				}
				// Смещаем позицию разбора
				i += static_cast <size_t> (take);
				// Уменьшаем остаток непрочитанных данных чанка
				this->_statsBody.bytesRemaining -= take;
				// Если данные чанка полностью приняты
				if(this->_statsBody.bytesRemaining == 0)
					// Переходим к ожиданию CRLF после данных чанка
					this->_state = static_cast <uint8_t> (S_CHUNK_DATA_ALMOST_DONE);
				// Продолжаем разбор
				continue;
			}
			/**
			 * Крупноблочное сканирование токенов (переносимый "SIMD"): непрерывный участок
			 * допустимых символов сканируется по lookup-таблице и копируется одним append
			 */
			// Если разбирается request-target
			if(this->_state == static_cast <uint8_t> (S_REQ_TARGET)){
				// Позиция конца непрерывного участка допустимых символов
				size_t j = i;
				// Сканируем непрерывный участок допустимых символов request-target
				while((j < size) && isTargetCh(static_cast <uint8_t> (data[j])))
					// Смещаем позицию конца участка
					++j;
				// Если непрерывный участок найден
				if(j > i){
					// Определяем размер непрерывного участка
					const size_t run = (j - i);
					// Если длина стартовой строки превышает лимит
					if((this->_statsHeaders.lineBytes + run) > this->_limits.maxRequestLine){
						// Фиксируем ошибку превышения длины request-line
						this->_error = error_t::URL_OVERFLOW;
						// Устанавливаем итоговый статус разбора
						this->_status = status_t::ERROR;
						// Выводим количество обработанных байт данных
						return i;
					}
					// Наращиваем длину текущей стартовой строки
					this->_statsHeaders.lineBytes += run;
					// Добавляем непрерывный участок к параметрам URI-запроса
					req->uri.append(data + i, run);
					// Смещаем позицию разбора
					i = j;
					// Продолжаем разбор
					continue;
				}
			// Если разбирается имя заголовка или трейлера
			} else if((this->_state == static_cast <uint8_t> (S_HEADER_NAME)) || (this->_state == static_cast <uint8_t> (S_TRAILER_NAME))) {
				// Позиция конца непрерывного участка допустимых символов
				size_t j = i;
				// Сканируем непрерывный участок символов токена
				while((j < size) && isToken(static_cast <uint8_t> (data[j])))
					// Смещаем позицию конца участка
					++j;
				// Если непрерывный участок найден
				if(j > i){
					// Определяем размер непрерывного участка
					const size_t run = (j - i);
					// Если длина имени заголовка превышает лимит
					if((this->_header.name.size() + run) > this->_limits.maxHeaderName){
						// Фиксируем ошибку превышения размера заголовков
						this->_error = error_t::HEADER_OVERFLOW;
						// Устанавливаем итоговый статус разбора
						this->_status = status_t::ERROR;
						// Выводим количество обработанных байт данных
						return i;
					}
					// Добавляем непрерывный участок к имени заголовка
					this->_header.name.append(data + i, run);
					// Смещаем позицию разбора
					i = j;
					// Продолжаем разбор
					continue;
				}
			// Если разбирается значение заголовка или трейлера
			} else if((this->_state == static_cast <uint8_t> (S_HEADER_VALUE)) || (this->_state == static_cast <uint8_t> (S_TRAILER_VALUE))) {
				// Позиция конца непрерывного участка допустимых символов
				size_t j = i;
				// Сканируем непрерывный участок допустимых символов значения заголовка
				while((j < size) && isValueCh(static_cast <uint8_t> (data[j])))
					// Смещаем позицию конца участка
					++j;
				// Если непрерывный участок найден
				if(j > i){
					// Определяем размер непрерывного участка
					const size_t run = (j - i);
					// Если длина значения заголовка превышает лимит
					if((this->_header.value.size() + run) > this->_limits.maxHeaderValue){
						// Фиксируем ошибку превышения размера заголовков
						this->_error = error_t::HEADER_OVERFLOW;
						// Устанавливаем итоговый статус разбора
						this->_status = status_t::ERROR;
						// Выводим количество обработанных байт данных
						return i;
					}
					// Добавляем непрерывный участок к значению заголовка
					this->_header.value.append(data + i, run);
					// Смещаем позицию разбора
					i = j;
					// Продолжаем разбор
					continue;
				}
			}
			/**
			 * Посимвольные состояния конечного автомата
			 */
			switch(this->_state){
				// Общий старт (диспетчеризация по направлению трафика)
				case static_cast <uint8_t> (S_START): {
					// Устанавливаем фазу начала разбора сообщения
					this->_message.phase = phase_t::BEGIN;
					// Устанавливаем партицию заголовков сообщения
					this->_message.part = part_t::HEADERS;
					// Уведомляем о начале разбора нового сообщения
					if(!this->firePhase(phase_t::BEGIN, part_t::NONE))
						// Выходим из состояния (ошибка уже установлена)
						break;
					// Если выполняется разбор запроса клиента
					if(this->_direct == direct_t::REQUEST){
						// Если первый символ не является символом токена
						if(!isToken(ch)){
							// Фиксируем ошибку некорректного метода
							this->_error = error_t::INVALID_METHOD;
							// Выходим из состояния
							break;
						}
						// Добавляем символ к накопителю имени метода
						this->_header.name.push_back(static_cast <char> (ch));
						// Начинаем отсчёт длины стартовой строки
						this->_statsHeaders.lineBytes = 1;
						// Переходим к разбору метода запроса
						this->_state = static_cast <uint8_t> (S_REQ_METHOD);
					// Если выполняется разбор ответа сервера
					} else {
						// Если первый символ не является началом литерала "HTTP/"
						if(ch != 'H'){
							// Фиксируем ошибку некорректной версии протокола
							this->_error = error_t::INVALID_VERSION;
							// Выходим из состояния
							break;
						}
						// Начинаем отсчёт длины стартовой строки
						this->_statsHeaders.lineBytes = 1;
						// Переходим к разбору литерала "HTTP/"
						this->_state = static_cast <uint8_t> (S_RES_HTTP_H);
					}
				} break;
				/**
				 * Стартовая строка запроса клиента (request-line)
				 */
				// Разбор метода запроса
				case static_cast <uint8_t> (S_REQ_METHOD): {
					// Если получен пробел - метод разобран полностью
					if(ch == ' '){
						// Выполняем классификацию метода запроса по его имени
						req->method = classifyMethod(this->_header.name, this->_fmk);
						// Если метод запроса синтаксически корректен, но не распознан
						if(req->method == method_t::NONE){
							// Помечаем метод запроса как нераспознанный
							req->method = method_t::UNKNOWN;
							// Сохраняем оригинальное написание метода (прозрачное проксирование экзотических методов)
							req->methodName = this->_header.name;
						}
						// Очищаем накопитель имени (метод уже классифицирован)
						this->_header.name.clear();
						// Переходим к пропуску пробелов перед request-target
						this->_state = static_cast <uint8_t> (S_REQ_TARGET_START);
					// Если получен символ токена
					} else if(isToken(ch)) {
						// Если длина стартовой строки превышает лимит
						if(++this->_statsHeaders.lineBytes > this->_limits.maxRequestLine){
							// Фиксируем ошибку превышения длины request-line
							this->_error = error_t::URL_OVERFLOW;
							// Выходим из состояния
							break;
						}
						// Добавляем символ к накопителю имени метода
						this->_header.name.push_back(static_cast <char> (ch));
					// Любой другой символ в методе недопустим
					} else this->_error = error_t::INVALID_METHOD;
				} break;
				// Пропуск пробелов перед request-target
				case static_cast <uint8_t> (S_REQ_TARGET_START): {
					// Толерантно пропускаем лишние пробелы
					if(ch == ' ')
						// Выходим из состояния
						break;
					// Если символ недопустим в request-target
					if(!isTargetCh(ch)){
						// Фиксируем ошибку некорректного request-target
						this->_error = error_t::INVALID_TARGET;
						// Выходим из состояния
						break;
					}
					// Если длина стартовой строки превышает лимит
					if(++this->_statsHeaders.lineBytes > this->_limits.maxRequestLine){
						// Фиксируем ошибку превышения длины request-line
						this->_error = error_t::URL_OVERFLOW;
						// Выходим из состояния
						break;
					}
					// Добавляем символ к параметрам URI-запроса
					req->uri.push_back(static_cast <char> (ch));
					// Переходим к разбору request-target
					this->_state = static_cast <uint8_t> (S_REQ_TARGET);
				} break;
				// Разбор request-target
				case static_cast <uint8_t> (S_REQ_TARGET): {
					// Если получен пробел - request-target разобран полностью
					if(ch == ' '){
						// Переходим к пропуску пробелов перед "HTTP/"
						this->_state = static_cast <uint8_t> (S_REQ_HTTP_START);
						// Выходим из состояния
						break;
					}
					// Если символ недопустим в request-target
					if(!isTargetCh(ch)){
						// Фиксируем ошибку некорректного request-target
						this->_error = error_t::INVALID_TARGET;
						// Выходим из состояния
						break;
					}
					// Если длина стартовой строки превышает лимит
					if(++this->_statsHeaders.lineBytes > this->_limits.maxRequestLine){
						// Фиксируем ошибку превышения длины request-line
						this->_error = error_t::URL_OVERFLOW;
						// Выходим из состояния
						break;
					}
					// Добавляем символ к параметрам URI-запроса
					req->uri.push_back(static_cast <char> (ch));
				} break;
				// Пропуск пробелов перед "HTTP/"
				case static_cast <uint8_t> (S_REQ_HTTP_START): {
					// Толерантно пропускаем лишние пробелы
					if(ch == ' ')
						// Выходим из состояния
						break;
					// Если получен не литерал "H"
					if(ch != 'H'){
						// Фиксируем ошибку некорректной версии протокола
						this->_error = error_t::INVALID_VERSION;
						// Выходим из состояния
						break;
					}
					// Переходим к разбору литерала "HTTP/"
					this->_state = static_cast <uint8_t> (S_REQ_HTTP_H);
				} break;
				// Разбор литерала "HT"
				case static_cast <uint8_t> (S_REQ_HTTP_H): {
					// Если получен не литерал "T"
					if(ch != 'T'){
						// Фиксируем ошибку некорректной версии протокола
						this->_error = error_t::INVALID_VERSION;
						// Выходим из состояния
						break;
					}
					// Переходим к следующему литералу
					this->_state = static_cast <uint8_t> (S_REQ_HTTP_HT);
				} break;
				// Разбор литерала "HTT"
				case static_cast <uint8_t> (S_REQ_HTTP_HT): {
					// Если получен не литерал "T"
					if(ch != 'T'){
						// Фиксируем ошибку некорректной версии протокола
						this->_error = error_t::INVALID_VERSION;
						// Выходим из состояния
						break;
					}
					// Переходим к следующему литералу
					this->_state = static_cast <uint8_t> (S_REQ_HTTP_HTT);
				} break;
				// Разбор литерала "HTTP"
				case static_cast <uint8_t> (S_REQ_HTTP_HTT): {
					// Если получен не литерал "P"
					if(ch != 'P'){
						// Фиксируем ошибку некорректной версии протокола
						this->_error = error_t::INVALID_VERSION;
						// Выходим из состояния
						break;
					}
					// Переходим к следующему литералу
					this->_state = static_cast <uint8_t> (S_REQ_HTTP_HTTP);
				} break;
				// Разбор литерала "HTTP/"
				case static_cast <uint8_t> (S_REQ_HTTP_HTTP): {
					// Если получен не литерал "/"
					if(ch != '/'){
						// Фиксируем ошибку некорректной версии протокола
						this->_error = error_t::INVALID_VERSION;
						// Выходим из состояния
						break;
					}
					// Переходим к разбору мажорной цифры версии
					this->_state = static_cast <uint8_t> (S_REQ_HTTP_SLASH);
				} break;
				// Разбор мажорной цифры версии (принимается только HTTP/1.x)
				case static_cast <uint8_t> (S_REQ_HTTP_SLASH): {
					// Если мажорная цифра версии не равна 1
					if(ch != '1'){
						// Фиксируем ошибку некорректной версии протокола
						this->_error = error_t::INVALID_VERSION;
						// Выходим из состояния
						break;
					}
					// Переходим к разбору точки между major и minor
					this->_state = static_cast <uint8_t> (S_REQ_HTTP_DOT);
				} break;
				// Разбор точки между major и minor
				case static_cast <uint8_t> (S_REQ_HTTP_DOT): {
					// Если получена не точка
					if(ch != '.'){
						// Фиксируем ошибку некорректной версии протокола
						this->_error = error_t::INVALID_VERSION;
						// Выходим из состояния
						break;
					}
					// Переходим к разбору минорной цифры версии
					this->_state = static_cast <uint8_t> (S_REQ_HTTP_MINOR);
				} break;
				// Разбор минорной цифры версии (принимаются только HTTP/1.0 и HTTP/1.1)
				case static_cast <uint8_t> (S_REQ_HTTP_MINOR): {
					// Если минорная цифра версии равна 0
					if(ch == '0')
						// Устанавливаем версию протокола HTTP/1.0
						req->version = version_t::HTTP1_0;
					// Если минорная цифра версии равна 1
					else if(ch == '1')
						// Устанавливаем версию протокола HTTP/1.1
						req->version = version_t::HTTP1_1;
					// Любая другая версия отвергается по контракту парсера
					else {
						// Фиксируем ошибку некорректной версии протокола
						this->_error = error_t::INVALID_VERSION;
						// Выходим из состояния
						break;
					}
					// Переходим к ожиданию CR/LF после версии
					this->_state = static_cast <uint8_t> (S_REQ_LINE_ALMOST_DONE);
				} break;
				// Ожидание CR/LF после версии
				case static_cast <uint8_t> (S_REQ_LINE_ALMOST_DONE): {
					// Если получен CR - ожидаем LF
					if(ch == '\r'){
						// Переходим к ожиданию LF
						this->_state = static_cast <uint8_t> (S_REQ_LINE_LF);
						// Выходим из состояния
						break;
					}
					// Если получен LF - стартовая строка разобрана полностью (толерантность к голому LF)
					if(ch == '\n'){
						// Завершаем разбор стартовой строки
						this->commitStartLine();
						// Выходим из состояния
						break;
					}
					// Любой другой символ после версии недопустим
					this->_error = error_t::INVALID_VERSION;
				} break;
				// Ожидание LF после CR
				case static_cast <uint8_t> (S_REQ_LINE_LF): {
					// Если получен не LF
					if(ch != '\n'){
						// Фиксируем ошибку окончания строки
						this->_error = error_t::INVALID_EOL;
						// Выходим из состояния
						break;
					}
					// Завершаем разбор стартовой строки
					this->commitStartLine();
				} break;
				/**
				 * Стартовая строка ответа сервера (status-line)
				 */
				// Разбор литерала "HT"
				case static_cast <uint8_t> (S_RES_HTTP_H): {
					// Если получен не литерал "T"
					if(ch != 'T'){
						// Фиксируем ошибку некорректной версии протокола
						this->_error = error_t::INVALID_VERSION;
						// Выходим из состояния
						break;
					}
					// Переходим к следующему литералу
					this->_state = static_cast <uint8_t> (S_RES_HTTP_HT);
				} break;
				// Разбор литерала "HTT"
				case static_cast <uint8_t> (S_RES_HTTP_HT): {
					// Если получен не литерал "T"
					if(ch != 'T'){
						// Фиксируем ошибку некорректной версии протокола
						this->_error = error_t::INVALID_VERSION;
						// Выходим из состояния
						break;
					}
					// Переходим к следующему литералу
					this->_state = static_cast <uint8_t> (S_RES_HTTP_HTT);
				} break;
				// Разбор литерала "HTTP"
				case static_cast <uint8_t> (S_RES_HTTP_HTT): {
					// Если получен не литерал "P"
					if(ch != 'P'){
						// Фиксируем ошибку некорректной версии протокола
						this->_error = error_t::INVALID_VERSION;
						// Выходим из состояния
						break;
					}
					// Переходим к следующему литералу
					this->_state = static_cast <uint8_t> (S_RES_HTTP_HTTP);
				} break;
				// Разбор литерала "HTTP/"
				case static_cast <uint8_t> (S_RES_HTTP_HTTP): {
					// Если получен не литерал "/"
					if(ch != '/'){
						// Фиксируем ошибку некорректной версии протокола
						this->_error = error_t::INVALID_VERSION;
						// Выходим из состояния
						break;
					}
					// Переходим к разбору мажорной цифры версии
					this->_state = static_cast <uint8_t> (S_RES_HTTP_SLASH);
				} break;
				// Разбор мажорной цифры версии (принимается только HTTP/1.x)
				case static_cast <uint8_t> (S_RES_HTTP_SLASH): {
					// Если мажорная цифра версии не равна 1
					if(ch != '1'){
						// Фиксируем ошибку некорректной версии протокола
						this->_error = error_t::INVALID_VERSION;
						// Выходим из состояния
						break;
					}
					// Переходим к разбору точки между major и minor
					this->_state = static_cast <uint8_t> (S_RES_HTTP_DOT);
				} break;
				// Разбор точки между major и minor
				case static_cast <uint8_t> (S_RES_HTTP_DOT): {
					// Если получена не точка
					if(ch != '.'){
						// Фиксируем ошибку некорректной версии протокола
						this->_error = error_t::INVALID_VERSION;
						// Выходим из состояния
						break;
					}
					// Переходим к разбору минорной цифры версии
					this->_state = static_cast <uint8_t> (S_RES_HTTP_MINOR);
				} break;
				// Разбор минорной цифры версии (принимаются только HTTP/1.0 и HTTP/1.1)
				case static_cast <uint8_t> (S_RES_HTTP_MINOR): {
					// Если минорная цифра версии равна 0
					if(ch == '0')
						// Устанавливаем версию протокола HTTP/1.0
						res->version = version_t::HTTP1_0;
					// Если минорная цифра версии равна 1
					else if(ch == '1')
						// Устанавливаем версию протокола HTTP/1.1
						res->version = version_t::HTTP1_1;
					// Любая другая версия отвергается по контракту парсера
					else {
						// Фиксируем ошибку некорректной версии протокола
						this->_error = error_t::INVALID_VERSION;
						// Выходим из состояния
						break;
					}
					// Переходим к обязательному пробелу после версии
					this->_state = static_cast <uint8_t> (S_RES_FIRST_SPACE);
				} break;
				// Обязательный пробел после версии (RFC 7230 §3.1.2)
				case static_cast <uint8_t> (S_RES_FIRST_SPACE): {
					// Если получен не пробел
					if(ch != ' '){
						// Фиксируем ошибку некорректной версии протокола
						this->_error = error_t::INVALID_VERSION;
						// Выходим из состояния
						break;
					}
					// Переходим к пропуску пробелов перед статус-кодом
					this->_state = static_cast <uint8_t> (S_RES_STATUS_START);
				} break;
				// Пропуск пробелов перед статус-кодом
				case static_cast <uint8_t> (S_RES_STATUS_START): {
					// Толерантно пропускаем дополнительные пробелы перед кодом
					if(ch == ' ')
						// Выходим из состояния
						break;
					// Если символ не является десятичной цифрой
					if(!isDigit(ch)){
						// Фиксируем ошибку некорректного статус-кода
						this->_error = error_t::INVALID_STATUS;
						// Выходим из состояния
						break;
					}
					// Устанавливаем первую цифру статус-кода
					res->code = static_cast <uint8_t> (ch - '0');
					// Начинаем отсчёт цифр статус-кода
					this->_statsBody.digits = 1;
					// Переходим к разбору статус-кода
					this->_state = static_cast <uint8_t> (S_RES_STATUS_CODE);
				} break;
				// Разбор статус-кода
				case static_cast <uint8_t> (S_RES_STATUS_CODE): {
					// Если получена десятичная цифра
					if(isDigit(ch)){
						// Если статус-код уже содержит три цифры
						if(this->_statsBody.digits >= 3){
							// Фиксируем ошибку некорректного статус-кода
							this->_error = error_t::INVALID_STATUS;
							// Выходим из состояния
							break;
						}
						// Добавляем цифру к статус-коду
						res->code = static_cast <uint8_t> ((res->code * 10) + (ch - '0'));
						// Наращиваем счётчик цифр статус-кода
						++this->_statsBody.digits;
						// Выходим из состояния
						break;
					}
					// Если статус-код содержит не три цифры
					if(this->_statsBody.digits != 3){
						// Фиксируем ошибку некорректного статус-кода
						this->_error = error_t::INVALID_STATUS;
						// Выходим из состояния
						break;
					}
					// Если получен пробел - переходим к reason-phrase
					if(ch == ' '){
						// Переходим к разбору reason-phrase
						this->_state = static_cast <uint8_t> (S_RES_REASON_START);
						// Выходим из состояния
						break;
					}
					// Если получен CR - ожидаем LF
					if(ch == '\r'){
						// Переходим к ожиданию LF
						this->_state = static_cast <uint8_t> (S_RES_LINE_LF);
						// Выходим из состояния
						break;
					}
					// Если получен LF - стартовая строка разобрана полностью (толерантность к голому LF)
					if(ch == '\n'){
						// Завершаем разбор стартовой строки
						this->commitStartLine();
						// Выходим из состояния
						break;
					}
					// Любой другой символ после статус-кода недопустим
					this->_error = error_t::INVALID_STATUS;
				} break;
				// Пробел перед reason-phrase
				case static_cast <uint8_t> (S_RES_REASON_START): {
					// Если получен CR - reason-phrase пустая, ожидаем LF
					if(ch == '\r'){
						// Переходим к ожиданию LF
						this->_state = static_cast <uint8_t> (S_RES_LINE_LF);
						// Выходим из состояния
						break;
					}
					// Если получен LF - стартовая строка разобрана полностью (толерантность к голому LF)
					if(ch == '\n'){
						// Завершаем разбор стартовой строки
						this->commitStartLine();
						// Выходим из состояния
						break;
					}
					// Если символ недопустим в reason-phrase
					if(!isValueCh(ch)){
						// Фиксируем ошибку некорректного статус-кода
						this->_error = error_t::INVALID_STATUS;
						// Выходим из состояния
						break;
					}
					// Добавляем символ к сообщению сервера
					res->message.push_back(static_cast <char> (ch));
					// Переходим к разбору reason-phrase
					this->_state = static_cast <uint8_t> (S_RES_REASON);
				} break;
				// Разбор reason-phrase
				case static_cast <uint8_t> (S_RES_REASON): {
					// Если получен CR - ожидаем LF
					if(ch == '\r'){
						// Переходим к ожиданию LF
						this->_state = static_cast <uint8_t> (S_RES_LINE_LF);
						// Выходим из состояния
						break;
					}
					// Если получен LF - стартовая строка разобрана полностью (толерантность к голому LF)
					if(ch == '\n'){
						// Завершаем разбор стартовой строки
						this->commitStartLine();
						// Выходим из состояния
						break;
					}
					// Если символ недопустим в reason-phrase
					if(!isValueCh(ch)){
						// Фиксируем ошибку некорректного статус-кода
						this->_error = error_t::INVALID_STATUS;
						// Выходим из состояния
						break;
					}
					// Если длина reason-phrase превышает лимит стартовой строки
					if(res->message.size() >= this->_limits.maxRequestLine){
						// Фиксируем ошибку превышения длины стартовой строки
						this->_error = error_t::URL_OVERFLOW;
						// Выходим из состояния
						break;
					}
					// Добавляем символ к сообщению сервера
					res->message.push_back(static_cast <char> (ch));
				} break;
				// Ожидание LF после CR
				case static_cast <uint8_t> (S_RES_LINE_LF): {
					// Если получен не LF
					if(ch != '\n'){
						// Фиксируем ошибку окончания строки
						this->_error = error_t::INVALID_EOL;
						// Выходим из состояния
						break;
					}
					// Завершаем разбор стартовой строки
					this->commitStartLine();
				} break;
				/**
				 * Заголовки сообщения
				 */
				// Начало строки заголовка
				case static_cast <uint8_t> (S_HEADER_START): {
					// Если получен CR - ожидаем LF финальной пустой строки
					if(ch == '\r'){
						// Переходим к ожиданию LF финальной пустой строки
						this->_state = static_cast <uint8_t> (S_HEADERS_LF);
						// Выходим из состояния
						break;
					}
					// Если получен LF - заголовки разобраны полностью (толерантность к голому LF)
					if(ch == '\n'){
						// Выбираем способ кадрирования тела сообщения
						this->beginBody();
						// Выходим из состояния
						break;
					}
					// Если получен пробел или табуляция - это obs-fold (запрещён RFC 7230)
					if((ch == ' ') || (ch == '\t')){
						// Фиксируем ошибку некорректного имени заголовка
						this->_error = error_t::INVALID_HEADER_TOKEN;
						// Выходим из состояния
						break;
					}
					// Если символ не является символом токена
					if(!isToken(ch)){
						// Фиксируем ошибку некорректного имени заголовка
						this->_error = error_t::INVALID_HEADER_TOKEN;
						// Выходим из состояния
						break;
					}
					// Очищаем накопитель имени текущего заголовка
					this->_header.name.clear();
					// Добавляем символ к имени заголовка
					this->_header.name.push_back(static_cast <char> (ch));
					// Переходим к разбору имени заголовка
					this->_state = static_cast <uint8_t> (S_HEADER_NAME);
				} break;
				// Разбор имени заголовка
				case static_cast <uint8_t> (S_HEADER_NAME): {
					// Если получено двоеточие - имя заголовка разобрано полностью
					if(ch == ':'){
						// Очищаем накопитель значения текущего заголовка
						this->_header.value.clear();
						// Переходим к пропуску ведущих OWS значения
						this->_state = static_cast <uint8_t> (S_HEADER_VALUE_OWS);
						// Выходим из состояния
						break;
					}
					// Если символ не является символом токена
					if(!isToken(ch)){
						// Фиксируем ошибку некорректного имени заголовка
						this->_error = error_t::INVALID_HEADER_TOKEN;
						// Выходим из состояния
						break;
					}
					// Если длина имени заголовка превышает лимит
					if(this->_header.name.size() >= this->_limits.maxHeaderName){
						// Фиксируем ошибку превышения размера заголовков
						this->_error = error_t::HEADER_OVERFLOW;
						// Выходим из состояния
						break;
					}
					// Добавляем символ к имени заголовка
					this->_header.name.push_back(static_cast <char> (ch));
				} break;
				// Пропуск ведущих OWS значения
				case static_cast <uint8_t> (S_HEADER_VALUE_OWS): {
					// Пропускаем ведущие пробелы и табуляции
					if((ch == ' ') || (ch == '\t'))
						// Выходим из состояния
						break;
					// Если получен CR - значение пустое, ожидаем LF
					if(ch == '\r'){
						// Переходим к ожиданию LF
						this->_state = static_cast <uint8_t> (S_HEADER_ALMOST_DONE);
						// Выходим из состояния
						break;
					}
					// Если получен LF - заголовок разобран полностью (толерантность к голому LF)
					if(ch == '\n'){
						// Если завершение разбора заголовка не удалось
						if(!this->commitHeader())
							// Выходим из состояния (ошибка уже установлена)
							break;
						// Переходим к разбору следующего заголовка
						this->_state = static_cast <uint8_t> (S_HEADER_START);
						// Выходим из состояния
						break;
					}
					// Если символ недопустим в значении заголовка
					if(!isValueCh(ch)){
						// Фиксируем ошибку некорректного значения заголовка
						this->_error = error_t::INVALID_HEADER_VALUE;
						// Выходим из состояния
						break;
					}
					// Добавляем символ к значению заголовка
					this->_header.value.push_back(static_cast <char> (ch));
					// Переходим к разбору значения заголовка
					this->_state = static_cast <uint8_t> (S_HEADER_VALUE);
				} break;
				// Разбор значения заголовка
				case static_cast <uint8_t> (S_HEADER_VALUE): {
					// Если получен CR - ожидаем LF
					if(ch == '\r'){
						// Переходим к ожиданию LF
						this->_state = static_cast <uint8_t> (S_HEADER_ALMOST_DONE);
						// Выходим из состояния
						break;
					}
					// Если получен LF - заголовок разобран полностью (толерантность к голому LF)
					if(ch == '\n'){
						// Если завершение разбора заголовка не удалось
						if(!this->commitHeader())
							// Выходим из состояния (ошибка уже установлена)
							break;
						// Переходим к разбору следующего заголовка
						this->_state = static_cast <uint8_t> (S_HEADER_START);
						// Выходим из состояния
						break;
					}
					// Если символ недопустим в значении заголовка
					if(!isValueCh(ch)){
						// Фиксируем ошибку некорректного значения заголовка
						this->_error = error_t::INVALID_HEADER_VALUE;
						// Выходим из состояния
						break;
					}
					// Если длина значения заголовка превышает лимит
					if(this->_header.value.size() >= this->_limits.maxHeaderValue){
						// Фиксируем ошибку превышения размера заголовков
						this->_error = error_t::HEADER_OVERFLOW;
						// Выходим из состояния
						break;
					}
					// Добавляем символ к значению заголовка
					this->_header.value.push_back(static_cast <char> (ch));
				} break;
				// Ожидание LF после CR
				case static_cast <uint8_t> (S_HEADER_ALMOST_DONE): {
					// Если получен не LF
					if(ch != '\n'){
						// Фиксируем ошибку окончания строки
						this->_error = error_t::INVALID_EOL;
						// Выходим из состояния
						break;
					}
					// Если завершение разбора заголовка не удалось
					if(!this->commitHeader())
						// Выходим из состояния (ошибка уже установлена)
						break;
					// Переходим к разбору следующего заголовка
					this->_state = static_cast <uint8_t> (S_HEADER_START);
				} break;
				// Ожидание LF финальной пустой строки
				case static_cast <uint8_t> (S_HEADERS_LF): {
					// Если получен не LF
					if(ch != '\n'){
						// Фиксируем ошибку окончания строки
						this->_error = error_t::INVALID_EOL;
						// Выходим из состояния
						break;
					}
					// Выбираем способ кадрирования тела сообщения
					this->beginBody();
				} break;
				/**
				 * Кодирование chunked
				 */
				// Разбор hex-размера чанка
				case static_cast <uint8_t> (S_CHUNK_SIZE): {
					// Если длина строки заголовка чанка превышает лимит
					if(++this->_statsHeaders.chunkLineBytes > this->_limits.maxChunkLine){
						// Фиксируем ошибку превышения размера чанка
						this->_error = error_t::CHUNK_OVERFLOW;
						// Выходим из состояния
						break;
					}
					// Получаем числовое значение hex-цифры
					const int8_t hv = hexVal(ch);
					// Если получена hex-цифра
					if(hv >= 0){
						// Если добавление цифры приведёт к переполнению
						if(this->_statsBody.chunkSize > (UINT64_MAX >> 4)){
							// Фиксируем ошибку превышения размера чанка
							this->_error = error_t::CHUNK_OVERFLOW;
							// Выходим из состояния
							break;
						}
						// Добавляем цифру к размеру чанка
						this->_statsBody.chunkSize = ((this->_statsBody.chunkSize << 4) | static_cast <uint64_t> (hv));
						// Наращиваем счётчик hex-цифр размера чанка
						++this->_statsBody.digits;
						// Выходим из состояния
						break;
					}
					// Если получена точка с запятой - начинаются расширения чанка
					if(ch == ';'){
						// Переходим к пропуску расширений чанка
						this->_state = static_cast <uint8_t> (S_CHUNK_EXT);
						// Выходим из состояния
						break;
					}
					// Если получен CR - ожидаем LF
					if(ch == '\r'){
						// Переходим к ожиданию LF
						this->_state = static_cast <uint8_t> (S_CHUNK_SIZE_LF);
						// Выходим из состояния
						break;
					}
					// Если получен LF - строка размера чанка разобрана полностью (толерантность к голому LF)
					if(ch == '\n'){
						// Завершаем разбор строки размера чанка
						this->chunkSizeComplete();
						// Выходим из состояния
						break;
					}
					// Любой другой символ в размере чанка недопустим
					this->_error = error_t::INVALID_CHUNK_SIZE;
				} break;
				// Пропуск расширений чанка (chunk-ext)
				case static_cast <uint8_t> (S_CHUNK_EXT): {
					// Если длина строки заголовка чанка превышает лимит
					if(++this->_statsHeaders.chunkLineBytes > this->_limits.maxChunkLine){
						// Фиксируем ошибку превышения размера чанка
						this->_error = error_t::CHUNK_OVERFLOW;
						// Выходим из состояния
						break;
					}
					// Если получен CR - ожидаем LF
					if(ch == '\r'){
						// Переходим к ожиданию LF
						this->_state = static_cast <uint8_t> (S_CHUNK_SIZE_LF);
						// Выходим из состояния
						break;
					}
					// Если получен LF - строка размера чанка разобрана полностью (толерантность к голому LF)
					if(ch == '\n'){
						// Завершаем разбор строки размера чанка
						this->chunkSizeComplete();
						// Выходим из состояния
						break;
					}
					// Валидируем печатаемость символа расширений чанка
					if((ch < 0x20) && (ch != '\t')){
						// Фиксируем ошибку некорректного размера чанка
						this->_error = error_t::INVALID_CHUNK_SIZE;
						// Выходим из состояния
						break;
					}
					// Расширения чанка накапливаются только при установленном chunk-callback'е
					if(this->_callbacks.chunk != nullptr)
						// Добавляем символ к накопителю расширений текущего чанка
						this->_header.chunkExt.push_back(static_cast <char> (ch));
				} break;
				// Ожидание LF после CR строки размера чанка
				case static_cast <uint8_t> (S_CHUNK_SIZE_LF): {
					// Если получен не LF
					if(ch != '\n'){
						// Фиксируем ошибку окончания строки
						this->_error = error_t::INVALID_EOL;
						// Выходим из состояния
						break;
					}
					// Завершаем разбор строки размера чанка
					this->chunkSizeComplete();
				} break;
				// Ожидание CR/LF после данных чанка
				case static_cast <uint8_t> (S_CHUNK_DATA_ALMOST_DONE): {
					// Если получен CR - ожидаем LF
					if(ch == '\r'){
						// Переходим к ожиданию LF
						this->_state = static_cast <uint8_t> (S_CHUNK_DATA_LF);
						// Выходим из состояния
						break;
					}
					// Если получен LF - чанк дочитан полностью (толерантность к голому LF)
					if(ch == '\n'){
						// Уведомляем о завершении приёма данных чанка
						if(!this->fireChunk(phase_t::END, this->_statsBody.chunkSize))
							// Выходим из состояния (разбор прерван)
							break;
						// Сбрасываем размер текущего чанка
						this->_statsBody.chunkSize = 0;
						// Сбрасываем счётчик hex-цифр размера чанка
						this->_statsBody.digits = 0;
						// Сбрасываем длину строки заголовка чанка
						this->_statsHeaders.chunkLineBytes = 0;
						// Переходим к разбору размера следующего чанка
						this->_state = static_cast <uint8_t> (S_CHUNK_SIZE);
						// Выходим из состояния
						break;
					}
					// Любой другой символ после данных чанка недопустим
					this->_error = error_t::INVALID_CHUNK_TERMINATOR;
				} break;
				// Ожидание LF после CR данных чанка
				case static_cast <uint8_t> (S_CHUNK_DATA_LF): {
					// Если получен не LF
					if(ch != '\n'){
						// Фиксируем ошибку терминатора чанка
						this->_error = error_t::INVALID_CHUNK_TERMINATOR;
						// Выходим из состояния
						break;
					}
					// Уведомляем о завершении приёма данных чанка
					if(!this->fireChunk(phase_t::END, this->_statsBody.chunkSize))
						// Выходим из состояния (разбор прерван)
						break;
					// Сбрасываем размер текущего чанка
					this->_statsBody.chunkSize = 0;
					// Сбрасываем счётчик hex-цифр размера чанка
					this->_statsBody.digits = 0;
					// Сбрасываем длину строки заголовка чанка
					this->_statsHeaders.chunkLineBytes = 0;
					// Переходим к разбору размера следующего чанка
					this->_state = static_cast <uint8_t> (S_CHUNK_SIZE);
				} break;
				/**
				 * Трейлеры сообщения
				 */
				// Начало строки трейлера
				case static_cast <uint8_t> (S_TRAILER_START): {
					// Если получен CR - ожидаем LF финальной пустой строки
					if(ch == '\r'){
						// Переходим к ожиданию LF финальной пустой строки
						this->_state = static_cast <uint8_t> (S_TRAILERS_LF);
						// Выходим из состояния
						break;
					}
					// Если получен LF - трейлеры разобраны полностью (толерантность к голому LF)
					if(ch == '\n'){
						// Уведомляем о завершении разбора трейлеров
						if(this->firePhase(phase_t::END, part_t::TRAILER))
							// Завершаем разбор всего сообщения
							this->completeMessage();
						// Выходим из состояния
						break;
					}
					// Если получен пробел или табуляция - это obs-fold (запрещён RFC 7230)
					if((ch == ' ') || (ch == '\t')){
						// Фиксируем ошибку некорректного имени заголовка
						this->_error = error_t::INVALID_HEADER_TOKEN;
						// Выходим из состояния
						break;
					}
					// Если символ не является символом токена
					if(!isToken(ch)){
						// Фиксируем ошибку некорректного имени заголовка
						this->_error = error_t::INVALID_HEADER_TOKEN;
						// Выходим из состояния
						break;
					}
					// Очищаем накопитель имени текущего трейлера
					this->_header.name.clear();
					// Добавляем символ к имени трейлера
					this->_header.name.push_back(static_cast <char> (ch));
					// Переходим к разбору имени трейлера
					this->_state = static_cast <uint8_t> (S_TRAILER_NAME);
				} break;
				// Разбор имени трейлера
				case static_cast <uint8_t> (S_TRAILER_NAME): {
					// Если получено двоеточие - имя трейлера разобрано полностью
					if(ch == ':'){
						// Очищаем накопитель значения текущего трейлера
						this->_header.value.clear();
						// Переходим к пропуску ведущих OWS значения
						this->_state = static_cast <uint8_t> (S_TRAILER_VALUE_OWS);
						// Выходим из состояния
						break;
					}
					// Если символ не является символом токена
					if(!isToken(ch)){
						// Фиксируем ошибку некорректного имени заголовка
						this->_error = error_t::INVALID_HEADER_TOKEN;
						// Выходим из состояния
						break;
					}
					// Если длина имени трейлера превышает лимит
					if(this->_header.name.size() >= this->_limits.maxHeaderName){
						// Фиксируем ошибку превышения размера заголовков
						this->_error = error_t::HEADER_OVERFLOW;
						// Выходим из состояния
						break;
					}
					// Добавляем символ к имени трейлера
					this->_header.name.push_back(static_cast <char> (ch));
				} break;
				// Пропуск ведущих OWS значения трейлера
				case static_cast <uint8_t> (S_TRAILER_VALUE_OWS): {
					// Пропускаем ведущие пробелы и табуляции
					if((ch == ' ') || (ch == '\t'))
						// Выходим из состояния
						break;
					// Если получен CR - значение пустое, ожидаем LF
					if(ch == '\r'){
						// Переходим к ожиданию LF
						this->_state = static_cast <uint8_t> (S_TRAILER_ALMOST_DONE);
						// Выходим из состояния
						break;
					}
					// Если получен LF - трейлер разобран полностью (толерантность к голому LF)
					if(ch == '\n'){
						// Если завершение разбора трейлера не удалось
						if(!this->commitHeader())
							// Выходим из состояния (ошибка уже установлена)
							break;
						// Переходим к разбору следующего трейлера
						this->_state = static_cast <uint8_t> (S_TRAILER_START);
						// Выходим из состояния
						break;
					}
					// Если символ недопустим в значении трейлера
					if(!isValueCh(ch)){
						// Фиксируем ошибку некорректного значения заголовка
						this->_error = error_t::INVALID_HEADER_VALUE;
						// Выходим из состояния
						break;
					}
					// Добавляем символ к значению трейлера
					this->_header.value.push_back(static_cast <char> (ch));
					// Переходим к разбору значения трейлера
					this->_state = static_cast <uint8_t> (S_TRAILER_VALUE);
				} break;
				// Разбор значения трейлера
				case static_cast <uint8_t> (S_TRAILER_VALUE): {
					// Если получен CR - ожидаем LF
					if(ch == '\r'){
						// Переходим к ожиданию LF
						this->_state = static_cast <uint8_t> (S_TRAILER_ALMOST_DONE);
						// Выходим из состояния
						break;
					}
					// Если получен LF - трейлер разобран полностью (толерантность к голому LF)
					if(ch == '\n'){
						// Если завершение разбора трейлера не удалось
						if(!this->commitHeader())
							// Выходим из состояния (ошибка уже установлена)
							break;
						// Переходим к разбору следующего трейлера
						this->_state = static_cast <uint8_t> (S_TRAILER_START);
						// Выходим из состояния
						break;
					}
					// Если символ недопустим в значении трейлера
					if(!isValueCh(ch)){
						// Фиксируем ошибку некорректного значения заголовка
						this->_error = error_t::INVALID_HEADER_VALUE;
						// Выходим из состояния
						break;
					}
					// Если длина значения трейлера превышает лимит
					if(this->_header.value.size() >= this->_limits.maxHeaderValue){
						// Фиксируем ошибку превышения размера заголовков
						this->_error = error_t::HEADER_OVERFLOW;
						// Выходим из состояния
						break;
					}
					// Добавляем символ к значению трейлера
					this->_header.value.push_back(static_cast <char> (ch));
				} break;
				// Ожидание LF после CR трейлера
				case static_cast <uint8_t> (S_TRAILER_ALMOST_DONE): {
					// Если получен не LF
					if(ch != '\n'){
						// Фиксируем ошибку окончания строки
						this->_error = error_t::INVALID_EOL;
						// Выходим из состояния
						break;
					}
					// Если завершение разбора трейлера не удалось
					if(!this->commitHeader())
						// Выходим из состояния (ошибка уже установлена)
						break;
					// Переходим к разбору следующего трейлера
					this->_state = static_cast <uint8_t> (S_TRAILER_START);
				} break;
				// Ожидание LF финальной пустой строки трейлеров
				case static_cast <uint8_t> (S_TRAILERS_LF): {
					// Если получен не LF
					if(ch != '\n'){
						// Фиксируем ошибку окончания строки
						this->_error = error_t::INVALID_EOL;
						// Выходим из состояния
						break;
					}
					// Уведомляем о завершении разбора трейлеров
					if(this->firePhase(phase_t::END, part_t::TRAILER))
						// Завершаем разбор всего сообщения
						this->completeMessage();
				} break;
				// Неизвестное состояние конечного автомата
				default:
					// Фиксируем внутреннюю ошибку
					this->_error = error_t::INTERNAL;
				break;
			}
			// Если зафиксирована ошибка разбора
			if(this->_error != error_t::NONE){
				// Устанавливаем итоговый статус разбора
				this->_status = status_t::ERROR;
				// Выводим количество обработанных байт данных
				return i;
			}
			// Если сообщение полностью разобрано
			if(this->_state == static_cast <uint8_t> (S_MESSAGE_DONE)){
				// Устанавливаем итоговый статус разбора
				this->_status = status_t::COMPLETE;
				// Выводим количество обработанных байт данных (включая текущий байт)
				return (i + 1);
			}
			// Переходим к следующему байту данных
			++i;
		}
		// Данные приняты, но сообщение ещё не завершено - нужно ещё байтов
		this->_status = status_t::PARTIAL;
		// Выводим количество обработанных байт данных
		return i;
	/**
	 * Сбой аллокации или неперехваченное исключение
	 */
	} catch(const exception &) {
		// Фиксируем внутреннюю ошибку
		this->_error = error_t::INTERNAL;
		// Устанавливаем итоговый статус разбора
		this->_status = status_t::ERROR;
		// Выводим количество обработанных байт данных
		return i;
	}
}
/**
 * @brief Метод уведомления парсера о завершении потока данных (закрытии соединения)
 *
 * @details Требуется для сообщений, у которых тело кадрируется закрытием соединения:
 *          ответы HTTP/1.0 и ответы без Content-Length и без Transfer-Encoding: chunked.
 *          У таких сообщений в протоколе нет маркера конца тела - конец определяется
 *          только закрытием соединения удалённой стороной. Сетевой слой обязан вызвать
 *          этот метод, когда соединение закрыто (получен FIN/EOF сокета)
 */
void awh::http::Parser_HTTP::eof() noexcept {
	// Если ранее зафиксирована ошибка разбора - ничего не делаем
	if(this->_error != error_t::NONE)
		// Выходим из метода
		return;
	// Если сообщение уже полностью разобрано - нормальное закрытие соединения
	if(this->_state == static_cast <uint8_t> (S_MESSAGE_DONE))
		// Выходим из метода
		return;
	// Если парсер читает тело "до закрытия соединения" - сообщение завершено
	if(this->_state == static_cast <uint8_t> (S_BODY_UNTIL_CLOSE)){
		// Уведомляем о завершении приёма тела сообщения
		if(this->firePhase(phase_t::END, part_t::BODY))
			// Завершаем разбор всего сообщения
			this->completeMessage();
		// Устанавливаем итоговый статус разбора
		this->_status = (this->_error == error_t::NONE ? status_t::COMPLETE : status_t::ERROR);
		// Выходим из метода
		return;
	}
	// Если парсер находится между сообщениями - нормальное закрытие keep-alive соединения
	if((this->_state == static_cast <uint8_t> (S_START)) && (this->_statsHeaders.lineBytes == 0))
		// Выходим из метода
		return;
	// Сообщение разобрано частично - фиксируем обрыв соединения
	this->_error = error_t::PREMATURE_EOF;
	// Устанавливаем итоговый статус разбора
	this->_status = status_t::ERROR;
}
/**
 * @brief Метод установки метода запроса, которому соответствует ожидаемый ответ
 *
 * @details Используется ТОЛЬКО для направления RESPONSE: парсер ответа сам не может
 *          узнать, на какой запрос пришёл ответ, а метод запроса влияет на кадрирование
 *          тела (ответ на HEAD содержит Content-Length, но тела не имеет; успешный 2xx
 *          ответ на CONNECT открывает туннель и тела не имеет).
 *          Сбрасывается в NONE при reset() - выставляйте заново перед каждым ответом
 *          в keep-alive/конвейере
 *
 * @param method метод запроса клиента
 */
void awh::http::Parser_HTTP::method(const method_t method) noexcept {
	// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
	this->_method = method;
}
/**
 * @brief Метод установки функции обратного вызова для обработки тела сообщения
 *
 * @param callback функция обратного вызова для обработки тела сообщения
 */
void awh::http::Parser_HTTP::on(body_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова для обработки тела сообщения
	this->_callbacks.body = ::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова для обработки фазы разбора HTTP-сообщения
 *
 * @param callback функция обратного вызова для обработки фазы разбора HTTP-сообщения
 */
void awh::http::Parser_HTTP::on(phase_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова для обработки фазы разбора HTTP-сообщения
	this->_callbacks.phase = ::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова для обработки границ чанков
 *
 * @param callback функция обратного вызова для обработки границ чанков
 */
void awh::http::Parser_HTTP::on(chunk_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова для обработки границ чанков
	this->_callbacks.chunk = ::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова для обработки заголовков или трейлеров сообщения
 *
 * @param callback функция обратного вызова для обработки заголовков или трейлеров сообщения
 */
void awh::http::Parser_HTTP::on(header_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова для обработки заголовков или трейлеров сообщения
	this->_callbacks.header = ::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова для обработки провайдера заголовков сообщения
 *
 * @param callback функция обратного вызова для обработки провайдера заголовков сообщения
 */
void awh::http::Parser_HTTP::on(provider_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова для обработки провайдера заголовков сообщения
	this->_callbacks.provider = ::move(callback);
}
/**
 * @brief Конструктор
 *
 * @param direct направление трафика (запрос/ответ)
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 */
awh::http::Parser_HTTP::Parser_HTTP(const direct_t direct, const fmk_t * fmk, const log_t * log) noexcept :
 parser_t(direct, fmk, log), _state(static_cast <uint8_t> (S_START)), _method(method_t::NONE) {}
/**
 * @brief Деструктор
 *
 */
awh::http::Parser_HTTP::~Parser_HTTP() noexcept {}
