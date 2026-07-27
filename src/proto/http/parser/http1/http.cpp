/**
 * @file: http.cpp
 * @date: 2026-07-18
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация парсера протокола HTTP/1.x — разбор стартовой строки,
 *        заголовков и тела с кадрированием chunked и Content-Length, контроль лимитов и версий,
 *        сбор статистики и сборка исходящих сообщений
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <array>
#include <cstdio>
#include <cstdlib>
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
 * @brief Внутренние вспомогательные функции и таблицы (внутренняя компоновка)
 *
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
		S_CHUNK_SIZE_BWS         = 0x31, // Пропуск BWS между размером чанка и расширениями
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
		S_MESSAGE_DONE = 0x2F, // Сообщение полностью разобрано
		/**
		 * Точка входа быстрого пути стартовой строки ответа сервера
		 *
		 * @details Отдельное состояние нужно, чтобы крупноблочный путь получил
		 *          стартовую строку с первого её октета: посимвольный разбор
		 *          литерала версии начинается уже после потреблённого "H"
		 *
		 */
		S_RES_STATUS_LINE = 0x30
	};

	/**
	 * @brief Функция генерации таблицы токенов (RFC 7230): ALPHA / DIGIT / "!#$%&'*+-.^_`|~"
	 *
	 * @return таблица токенов
	 *
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
	 *
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
	 *
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
	 * @brief Класс крупноблочной обработки состояния конечного автомата
	 *
	 * @details Часть состояний обрабатывается не побайтово, а целыми участками:
	 *          тело сообщения копируется блоками, а токены сканируются по
	 *          lookup-таблице до первого недопустимого символа. Классификация
	 *          вынесена в таблицу, чтобы на горячем пути посимвольного разбора
	 *          выполнялось одно чтение вместо цепочки сравнений с каждым
	 *          крупноблочным состоянием
	 *
	 */
	enum scan_t : uint8_t {
		SCAN_NONE         = 0x00, // Состояние обрабатывается посимвольно
		SCAN_TARGET       = 0x01, // Сканирование участка request-target
		SCAN_TOKEN        = 0x02, // Сканирование участка имени заголовка/трейлера
		SCAN_VALUE        = 0x03, // Сканирование участка значения заголовка/трейлера
		SCAN_BODY_CHUNK   = 0x04, // Крупноблочное чтение данных чанка
		SCAN_BODY_IDENTITY= 0x05, // Крупноблочное чтение тела фиксированного размера
		SCAN_BODY_CLOSE   = 0x06, // Крупноблочное чтение тела до закрытия соединения
		SCAN_HEADER_LINE  = 0x07, // Разбор целой строки заголовка или трейлера
		SCAN_METHOD       = 0x08, // Сканирование участка метода запроса
		SCAN_VERSION      = 0x09, // Разбор литерала версии протокола запроса
		SCAN_STATUS       = 0x0A  // Разбор целой стартовой строки ответа сервера
	};

	/**
	 * @brief Размер литерала версии протокола вместе с окончанием строки в октетах
	 *
	 * @details Допустимых написаний версии в стартовой строке запроса всего два, и
	 *          оба имеют одинаковую длину: "HTTP/1.1" либо "HTTP/1.0", а следом
	 *          обязательное для быстрого пути окончание строки CRLF
	 *
	 */
	static constexpr size_t VERSION_LINE = 10;
	/**
	 * @brief Размер литерала версии протокола ответа вместе с обязательным пробелом в октетах
	 *
	 * @details Допустимых написаний версии всего два, и оба имеют одинаковую длину:
	 *          "HTTP/1.1" либо "HTTP/1.0", а следом обязательный для быстрого пути
	 *          одиночный пробел перед кодом состояния
	 *
	 */
	static constexpr size_t STATUS_VERSION = 9;
	/**
	 * @brief Размер обязательной части стартовой строки ответа в октетах
	 *
	 * @details Литерал версии с пробелом и три цифры кода состояния - минимум,
	 *          который обязана содержать стартовая строка ответа
	 *
	 */
	static constexpr size_t STATUS_MINIMUM = 12;
	/**
	 * @brief Предел числа октетов пустых строк, пропускаемых перед стартовой строкой запроса
	 *
	 * @details RFC 9112 §2.2 требует игнорировать хотя бы одну пустую строку, то есть два
	 *          октета. Предел взят с запасом на нескольких подряд, но остаётся малым:
	 *          пустые строки не несут содержимого, и поток из них не должен удерживать
	 *          соединение без продвижения разбора
	 *
	 */
	static constexpr uint8_t MAX_LEADING_BLANKS = 16;

	/**
	 * @brief Функция генерации таблицы классов крупноблочной обработки состояний
	 *
	 * @return таблица классов крупноблочной обработки состояний
	 *
	 */
	constexpr array <uint8_t, 256> makeScanTable() noexcept {
		// Результат работы функции
		array <uint8_t, 256> result{};
		// Помечаем состояние разбора метода запроса
		result[static_cast <uint8_t> (state_t::S_REQ_METHOD)] = SCAN_METHOD;
		// Помечаем состояние разбора request-target
		result[static_cast <uint8_t> (state_t::S_REQ_TARGET)] = SCAN_TARGET;
		// Помечаем состояние разбора литерала версии протокола запроса
		result[static_cast <uint8_t> (state_t::S_REQ_HTTP_START)] = SCAN_VERSION;
		// Помечаем состояние разбора имени заголовка
		result[static_cast <uint8_t> (state_t::S_HEADER_NAME)] = SCAN_TOKEN;
		// Помечаем состояние разбора имени трейлера
		result[static_cast <uint8_t> (state_t::S_TRAILER_NAME)] = SCAN_TOKEN;
		// Помечаем состояние разбора значения заголовка
		result[static_cast <uint8_t> (state_t::S_HEADER_VALUE)] = SCAN_VALUE;
		// Помечаем состояние разбора значения трейлера
		result[static_cast <uint8_t> (state_t::S_TRAILER_VALUE)] = SCAN_VALUE;
		// Помечаем состояние чтения данных чанка
		result[static_cast <uint8_t> (state_t::S_CHUNK_DATA)] = SCAN_BODY_CHUNK;
		// Помечаем состояние чтения тела фиксированного размера
		result[static_cast <uint8_t> (state_t::S_BODY_IDENTITY)] = SCAN_BODY_IDENTITY;
		// Помечаем состояние чтения тела до закрытия соединения
		result[static_cast <uint8_t> (state_t::S_BODY_UNTIL_CLOSE)] = SCAN_BODY_CLOSE;
		// Помечаем состояние начала строки заголовка
		result[static_cast <uint8_t> (state_t::S_HEADER_START)] = SCAN_HEADER_LINE;
		// Помечаем состояние начала строки трейлера
		result[static_cast <uint8_t> (state_t::S_TRAILER_START)] = SCAN_HEADER_LINE;
		// Помечаем состояние начала стартовой строки ответа сервера
		result[static_cast <uint8_t> (state_t::S_RES_STATUS_LINE)] = SCAN_STATUS;
		// Выводим результат
		return result;
	}

	/**
	 * @brief Таблица классов крупноблочной обработки состояний (считается на этапе компиляции)
	 *
	 */
	constexpr auto scanTable = makeScanTable();

	/**
	 * @brief Функция проверки принадлежности символа к токенам
	 *
	 * @param c проверяемый символ
	 * @return  результат проверки
	 *
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
	 *
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
	 *
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
	 *
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
	 *
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
	 * @brief Функция записи размера чанка в шестнадцатеричном виде с завершающим CRLF
	 *
	 * @details Заменяет snprintf на горячем пути кадрирования: разбор форматной строки
	 *          стоит дороже самой записи, а формат здесь фиксирован
	 *
	 * @param output буфер записи (не менее 18 байт)
	 * @param value  записываемый размер чанка
	 * @return       число записанных байт
	 *
	 */
	inline size_t writeChunkSize(char * output, const uint64_t value) noexcept {
		// Таблица шестнадцатеричных цифр в верхнем регистре
		static constexpr char DIGITS[] = "0123456789ABCDEF";
		// Временный буфер цифр в обратном порядке (максимум 16 цифр у uint64_t)
		char digits[16];
		// Количество записанных цифр
		size_t count = 0;
		// Остаток записываемого значения
		uint64_t rest = value;
		/**
		 * Выполняем выделение шестнадцатеричных цифр начиная с младшей
		 */
		do {
			// Записываем очередную младшую цифру во временный буфер
			digits[count++] = DIGITS[rest & 0x0F];
			// Сдвигаем остаток значения на одну шестнадцатеричную цифру
			rest >>= 4;
		// Продолжаем пока остаток значения не исчерпан
		} while(rest != 0);
		// Позиция записи в результирующем буфере
		size_t result = 0;
		/**
		 * Выполняем перенос цифр в результирующий буфер в прямом порядке
		 */
		while(count > 0)
			// Переносим очередную цифру в результирующий буфер
			output[result++] = digits[--count];
		// Дописываем возврат каретки
		output[result++] = '\r';
		// Дописываем перевод строки
		output[result++] = '\n';
		// Выводим результат
		return result;
	}
	/**
	 * @brief Функция приведения символа к нижнему регистру
	 *
	 * @param letter приводимый символ
	 * @return       символ в нижнем регистре
	 *
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
	 *
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
	 *
	 */
	bool iequalsLit(const string & str, const char * litLower) noexcept {
		// Выполняем сравнение строки с литералом
		return iequalsLit(str.c_str(), str.length(), litLower);
	}
	/**
	 * @brief Функция точного сравнения имени метода запроса с литералом
	 *
	 * @details Метод запроса регистрозависим (RFC 9110 §9.1): трактовка "get" как GET
	 *          позволяет обойти ограничения по методу на промежуточном узле, поэтому
	 *          сравнение выполняется побайтово. Длина имени уже совпала при
	 *          диспетчеризации по размеру, поэтому достаточно memcmp
	 *
	 * @param method  имя метода запроса
	 * @param literal литерал канонического написания метода
	 * @return        результат сравнения
	 *
	 */
	inline bool equalsMethod(const string_view method, const char * literal) noexcept {
		// Выполняем побайтовое сравнение имени метода с литералом
		return (::memcmp(method.data(), literal, method.size()) == 0);
	}
	/**
	 * @brief Функция разбора десятичного числа с контролем переполнения
	 *
	 * @param str  указатель на строку с числом
	 * @param size размер строки с числом
	 * @param out  результирующее число
	 * @return     результат разбора
	 *
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
	 *
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
	 * @brief Функция отсечения параметров транспортного кодирования от его имени
	 *
	 * @details По RFC 9112 §7 элемент Transfer-Encoding имеет вид
	 *          token *( OWS ";" OWS transfer-parameter ), поэтому запись
	 *          "chunked;foo=bar" является корректным указанием кодировки chunked
	 *          и обязана распознаваться наравне с голым "chunked"
	 *
	 * @param begin начало элемента списка
	 * @param end   конец элемента списка
	 *
	 */
	void trimParameters(const char * begin, const char *& end) noexcept {
		// Выполняем поиск начала параметров транспортного кодирования
		const char * separator = static_cast <const char *> (::memchr(begin, ';', static_cast <size_t> (end - begin)));
		// Если параметры транспортного кодирования обнаружены
		if(separator != nullptr){
			// Отсекаем параметры транспортного кодирования от имени
			end = separator;
			// Указатель на начало имени транспортного кодирования
			const char * name = begin;
			// Выполняем триминг хвостовых OWS оставшегося имени
			trimOWS(name, end);
		}
	}
	/**
	 * @brief Функция проверки завершения списка транспортных кодирований токеном chunked
	 *
	 * @details Кадрирование chunked действует, только если это последнее кодирование
	 *          в списке (RFC 9112 §6.1) - именно так значение трактует принимающая сторона
	 *
	 * @param value значение заголовка Transfer-Encoding
	 * @return      результат проверки
	 *
	 */
	bool endsWithChunked(const string & value) noexcept {
		// Получаем указатель на начало последнего элемента списка
		const char * begin = value.data();
		// Получаем указатель на конец последнего элемента списка
		const char * end = (begin + value.size());
		// Выполняем поиск последнего разделителя списка (memrchr - расширение, недоступное переносимо)
		const size_t comma = value.find_last_of(',');
		// Если разделитель обнаружен - последним является элемент после него
		if(comma != string::npos)
			// Смещаем начало последнего элемента списка
			begin = (value.data() + comma + 1);
		// Выполняем триминг OWS по краям последнего элемента списка
		trimOWS(begin, end);
		// Отсекаем параметры транспортного кодирования от его имени
		trimParameters(begin, end);
		// Выводим результат сравнения последнего кодирования с chunked
		return iequalsLit(begin, static_cast <size_t> (end - begin), "chunked");
	}
	/**
	 * @brief Функция проверки наличия токена chunked в списке транспортных кодирований
	 *
	 * @details Отличается от проверки последнего кодирования: нужна, чтобы отличить
	 *          список без chunked, который допустимо дополнить, от списка, где chunked
	 *          уже присутствует, но не последним. Второй случай неисправим дописыванием -
	 *          кодирование chunked нельзя применять к телу дважды (RFC 9112 §6.1)
	 *
	 * @param value значение заголовка транспортного кодирования
	 * @return      результат проверки
	 *
	 */
	bool containsChunked(const string & value) noexcept {
		// Указатель на текущую позицию разбора
		const char * current = value.data();
		// Указатель на конец значения заголовка
		const char * finish = (current + value.size());
		/**
		 * Выполняем разбор списка кодирований
		 */
		while(current <= finish){
			// Выполняем поиск разделителя списка
			const char * comma = static_cast <const char *> (memchr(current, ',', static_cast <size_t> (finish - current)));
			// Определяем конец текущего элемента списка
			const char * tokEnd = (comma != nullptr ? comma : finish);
			// Указатель на начало элемента списка
			const char * begin = current;
			// Указатель на конец элемента списка
			const char * end = tokEnd;
			// Выполняем триминг OWS по краям элемента списка
			trimOWS(begin, end);
			// Отсекаем параметры транспортного кодирования от его имени
			trimParameters(begin, end);
			// Если элемент списка является кодированием chunked
			if(iequalsLit(begin, static_cast <size_t> (end - begin), "chunked"))
				// Выводим признак присутствия кодирования chunked
				return true;
			// Если разделителей больше нет - разбор списка завершён
			if(comma == nullptr)
				// Выходим из цикла
				break;
			// Переходим к следующему элементу списка
			current = (comma + 1);
		}
		// Кодирование chunked в списке отсутствует
		return false;
	}
	/**
	 * @brief Функция проверки запрета поля в блоке трейлеров
	 *
	 * @details По RFC 9110 §6.5.1 отправитель не вправе формировать трейлер, если
	 *          определение соответствующего заголовка не разрешает его передачу
	 *          в трейлерах: заголовок, вычисляемый до получения тела, принятый
	 *          задним числом переопределил бы уже разобранное сообщение. Проверить
	 *          такое разрешение по определению поля библиотека не может, поэтому
	 *          применяется отбраковка по категориям, которые стандарт называет
	 *          непригодными для трейлеров: кадрирование сообщения, маршрутизация
	 *          и управление соединением, модификаторы запроса (управляющие поля
	 *          и условные), аутентификация, управляющие данные ответа и поля,
	 *          определяющие способ обработки содержимого. Поимённый состав
	 *          категорий взят из RFC 7230 §4.1.2, где он был перечислен явно
	 *
	 * @note Список общий для приёма и отправки: принимающая сторона такие поля
	 *       отбрасывает (RFC 9112 §7.1.2 разрешает получателю выборочно отбрасывать
	 *       трейлеры), а отправляющая их не формирует
	 *
	 * @param name название поля трейлера
	 * @return     результат проверки
	 *
	 */
	bool forbiddenTrailer(const string_view name) noexcept {
		/**
		 * Выполняем проверку названия по списку запрещённых (диспетчер по первой букве)
		 */
		switch(name.empty() ? '\0' : lower(name.front())){
			// Поля начинающиеся на "C"
			case 'c':
				// Проверяем принадлежность к полям кадрирования, обработки содержимого, управления соединением, кэшированием и аутентификацией
				return (
					iequalsLit(name.data(), name.size(), "content-length") ||
					iequalsLit(name.data(), name.size(), "content-type") ||
					iequalsLit(name.data(), name.size(), "content-encoding") ||
					iequalsLit(name.data(), name.size(), "content-range") ||
					iequalsLit(name.data(), name.size(), "connection") ||
					iequalsLit(name.data(), name.size(), "cache-control") ||
					iequalsLit(name.data(), name.size(), "cookie")
				);
			// Поля начинающиеся на "T"
			case 't':
				// Проверяем принадлежность к полям транспортного кодирования
				return (
					iequalsLit(name.data(), name.size(), "transfer-encoding") ||
					iequalsLit(name.data(), name.size(), "trailer") ||
					iequalsLit(name.data(), name.size(), "te")
				);
			// Поля начинающиеся на "H"
			case 'h':
				// Проверяем принадлежность к полю целевого узла
				return iequalsLit(name.data(), name.size(), "host");
			// Поля начинающиеся на "E"
			case 'e':
				// Проверяем принадлежность к полям ожидания промежуточного ответа и срока годности ответа
				return (iequalsLit(name.data(), name.size(), "expect") || iequalsLit(name.data(), name.size(), "expires"));
			// Поля начинающиеся на "U"
			case 'u':
				// Проверяем принадлежность к полю переключения протокола
				return iequalsLit(name.data(), name.size(), "upgrade");
			// Поля начинающиеся на "K"
			case 'k':
				// Проверяем принадлежность к полю управления удержанием соединения
				return iequalsLit(name.data(), name.size(), "keep-alive");
			// Поля начинающиеся на "P"
			case 'p':
				// Проверяем принадлежность к полям управления соединением с прокси, аутентификации на прокси и совместимости с HTTP/1.0
				return (
					iequalsLit(name.data(), name.size(), "proxy-connection") ||
					iequalsLit(name.data(), name.size(), "proxy-authorization") ||
					iequalsLit(name.data(), name.size(), "proxy-authenticate") ||
					iequalsLit(name.data(), name.size(), "proxy-authentication-info") ||
					iequalsLit(name.data(), name.size(), "pragma")
				);
			// Поля начинающиеся на "A"
			case 'a':
				// Проверяем принадлежность к полям аутентификации и возраста ответа
				return (
					iequalsLit(name.data(), name.size(), "authorization") ||
					iequalsLit(name.data(), name.size(), "authentication-info") ||
					iequalsLit(name.data(), name.size(), "age")
				);
			// Поля начинающиеся на "W"
			case 'w':
				// Проверяем принадлежность к полям запроса аутентификации и предупреждения о содержимом
				return (iequalsLit(name.data(), name.size(), "www-authenticate") || iequalsLit(name.data(), name.size(), "warning"));
			// Поля начинающиеся на "S"
			case 's':
				// Проверяем принадлежность к полю установки сессионных данных
				return iequalsLit(name.data(), name.size(), "set-cookie");
			// Поля начинающиеся на "I"
			case 'i':
				// Проверяем принадлежность к условным полям запроса
				return (
					iequalsLit(name.data(), name.size(), "if-match") ||
					iequalsLit(name.data(), name.size(), "if-none-match") ||
					iequalsLit(name.data(), name.size(), "if-modified-since") ||
					iequalsLit(name.data(), name.size(), "if-unmodified-since") ||
					iequalsLit(name.data(), name.size(), "if-range")
				);
			// Поля начинающиеся на "R"
			case 'r':
				// Проверяем принадлежность к полям запроса диапазона и указания времени повтора
				return (iequalsLit(name.data(), name.size(), "range") || iequalsLit(name.data(), name.size(), "retry-after"));
			// Поля начинающиеся на "M"
			case 'm':
				// Проверяем принадлежность к полю ограничения числа промежуточных узлов
				return iequalsLit(name.data(), name.size(), "max-forwards");
			// Поля начинающиеся на "L"
			case 'l':
				// Проверяем принадлежность к полю перенаправления
				return iequalsLit(name.data(), name.size(), "location");
			// Поля начинающиеся на "D"
			case 'd':
				// Проверяем принадлежность к полю отметки времени сообщения
				return iequalsLit(name.data(), name.size(), "date");
			// Поля начинающиеся на "V"
			case 'v':
				// Проверяем принадлежность к полю ключа вариативности кэша
				return iequalsLit(name.data(), name.size(), "vary");
		}
		// Поле в блоке трейлеров разрешено
		return false;
	}
	/**
	 * @brief Функция удаления строк заголовка с указанным названием из сериализованного блока
	 *
	 * @details Работает по уже напечатанному блоку, чтобы не копировать контейнер
	 *          заголовков ради аварийного пути. Строки заголовка ищутся с начала
	 *          строки, поэтому совпадение с телом чужого значения исключено
	 *
	 * @param block    сериализованный блок заголовков
	 * @param nameLower название удаляемого заголовка в нижнем регистре
	 *
	 */
	void dropHeaderLine(string & block, const char * nameLower) noexcept {
		// Позиция начала очередной строки блока
		size_t position = 0;
		/**
		 * Выполняем перебор всех строк сериализованного блока
		 */
		while(position < block.size()){
			// Выполняем поиск конца текущей строки блока
			const size_t eol = block.find("\r\n", position);
			// Если конец строки не обнаружен - блок повреждён
			if(eol == string::npos)
				// Выходим из цикла
				break;
			// Если достигнута завершающая пустая строка блока
			if(eol == position)
				// Выходим из цикла
				break;
			// Выполняем поиск разделителя имени и значения заголовка
			const size_t colon = block.find(':', position);
			// Если название текущей строки совпадает с удаляемым заголовком
			if((colon != string::npos) && (colon < eol) && iequalsLit(block.data() + position, (colon - position), nameLower)){
				// Удаляем строку заголовка вместе с её окончанием
				block.erase(position, ((eol + 2) - position));
				// Продолжаем перебор с той же позиции
				continue;
			}
			// Переходим к следующей строке блока
			position = (eol + 2);
		}
	}
	/**
	 * @brief Функция удаления запрещённых полей из сериализованного блока трейлеров
	 *
	 * @details Отправлять поля, непригодные для трейлеров, запрещено RFC 9110 §6.5.1
	 *          и вдобавок опасно: промежуточный узел с иной трактовкой способен принять
	 *          их к сведению и переопределить кадрирование либо трактовку уже переданного
	 *          тела. Такое поле вычищается из блока до его выдачи на провод
	 *
	 * @param block сериализованный блок трейлеров
	 * @return      количество удалённых полей
	 *
	 */
	size_t dropForbiddenTrailers(string & block) noexcept {
		// Количество удалённых полей
		size_t result = 0;
		// Позиция начала очередной строки блока
		size_t position = 0;
		/**
		 * Выполняем перебор всех строк сериализованного блока
		 */
		while(position < block.size()){
			// Выполняем поиск конца текущей строки блока
			const size_t eol = block.find("\r\n", position);
			// Если конец строки не обнаружен - блок повреждён
			if(eol == string::npos)
				// Выходим из цикла
				break;
			// Если достигнута завершающая пустая строка блока
			if(eol == position)
				// Выходим из цикла
				break;
			// Выполняем поиск разделителя имени и значения поля
			const size_t colon = block.find(':', position);
			// Если поле запрещено в блоке трейлеров
			if((colon != string::npos) && (colon < eol) && forbiddenTrailer(string_view(block.data() + position, (colon - position)))){
				// Удаляем строку поля вместе с её окончанием
				block.erase(position, ((eol + 2) - position));
				// Учитываем удалённое поле
				++result;
				// Продолжаем перебор с той же позиции
				continue;
			}
			// Переходим к следующей строке блока
			position = (eol + 2);
		}
		// Выводим результат
		return result;
	}
	/**
	 * @brief Функция классификации метода запроса по его имени
	 *
	 * @details Сравнение выполняется с учётом регистра - метод запроса регистрозависим
	 *          по RFC 9110 §9.1, а нераспознанное написание уходит в method_t::UNKNOWN
	 *          с сохранением оригинала
	 *
	 * @note Имя метода принимается представлением: на быстром пути разбора
	 *       стартовой строки оно указывает прямо во входной буфер, и накопитель
	 *       при распознанном методе не задействуется вовсе
	 *
	 * @param method имя метода запроса
	 * @return       распознанный метод запроса либо method_t::NONE
	 *
	 */
	http::method_t classifyMethod(const string_view method) noexcept {
		/**
		 * Диспетчеризация по длине имени метода (несовпадение размера отсекается без сравнения)
		 */
		switch(method.size()){
			// Методы с длиной имени 3 символа
			case 3: {
				// Если метод совпадает с известным методом запроса GET
				if(::equalsMethod(method, "GET"))
					// Выводим распознанный метод запроса
					return http::method_t::GET;
				// Если метод совпадает с известным методом запроса PUT
				if(::equalsMethod(method, "PUT"))
					// Выводим распознанный метод запроса
					return http::method_t::PUT;
				// Если метод совпадает с известным методом запроса ACL
				if(::equalsMethod(method, "ACL"))
					// Выводим распознанный метод запроса
					return http::method_t::ACL;
				// Если метод совпадает с известным методом запроса PRI
				if(::equalsMethod(method, "PRI"))
					// Выводим распознанный метод запроса
					return http::method_t::PRI;
			} break;
			// Методы с длиной имени 4 символа
			case 4: {
				// Если метод совпадает с известным методом запроса HEAD
				if(::equalsMethod(method, "HEAD"))
					// Выводим распознанный метод запроса
					return http::method_t::HEAD;
				// Если метод совпадает с известным методом запроса POST
				if(::equalsMethod(method, "POST"))
					// Выводим распознанный метод запроса
					return http::method_t::POST;
				// Если метод совпадает с известным методом запроса COPY
				if(::equalsMethod(method, "COPY"))
					// Выводим распознанный метод запроса
					return http::method_t::COPY;
				// Если метод совпадает с известным методом запроса LOCK
				if(::equalsMethod(method, "LOCK"))
					// Выводим распознанный метод запроса
					return http::method_t::LOCK;
				// Если метод совпадает с известным методом запроса MOVE
				if(::equalsMethod(method, "MOVE"))
					// Выводим распознанный метод запроса
					return http::method_t::MOVE;
				// Если метод совпадает с известным методом запроса BIND
				if(::equalsMethod(method, "BIND"))
					// Выводим распознанный метод запроса
					return http::method_t::BIND;
				// Если метод совпадает с известным методом запроса LINK
				if(::equalsMethod(method, "LINK"))
					// Выводим распознанный метод запроса
					return http::method_t::LINK;
			} break;
			// Методы с длиной имени 5 символов
			case 5: {
				// Если метод совпадает с известным методом запроса TRACE
				if(::equalsMethod(method, "TRACE"))
					// Выводим распознанный метод запроса
					return http::method_t::TRACE;
				// Если метод совпадает с известным методом запроса PATCH
				if(::equalsMethod(method, "PATCH"))
					// Выводим распознанный метод запроса
					return http::method_t::PATCH;
				// Если метод совпадает с известным методом запроса MKCOL
				if(::equalsMethod(method, "MKCOL"))
					// Выводим распознанный метод запроса
					return http::method_t::MKCOL;
				// Если метод совпадает с известным методом запроса MERGE
				if(::equalsMethod(method, "MERGE"))
					// Выводим распознанный метод запроса
					return http::method_t::MERGE;
				// Если метод совпадает с известным методом запроса PURGE
				if(::equalsMethod(method, "PURGE"))
					// Выводим распознанный метод запроса
					return http::method_t::PURGE;
			} break;
			// Методы с длиной имени 6 символов
			case 6: {
				// Если метод совпадает с известным методом запроса DELETE
				if(::equalsMethod(method, "DELETE"))
					// Выводим распознанный метод запроса
					return http::method_t::DEL;
				// Если метод совпадает с известным методом запроса SEARCH
				if(::equalsMethod(method, "SEARCH"))
					// Выводим распознанный метод запроса
					return http::method_t::SEARCH;
				// Если метод совпадает с известным методом запроса UNLOCK
				if(::equalsMethod(method, "UNLOCK"))
					// Выводим распознанный метод запроса
					return http::method_t::UNLOCK;
				// Если метод совпадает с известным методом запроса REBIND
				if(::equalsMethod(method, "REBIND"))
					// Выводим распознанный метод запроса
					return http::method_t::REBIND;
				// Если метод совпадает с известным методом запроса UNBIND
				if(::equalsMethod(method, "UNBIND"))
					// Выводим распознанный метод запроса
					return http::method_t::UNBIND;
				// Если метод совпадает с известным методом запроса REPORT
				if(::equalsMethod(method, "REPORT"))
					// Выводим распознанный метод запроса
					return http::method_t::REPORT;
				// Если метод совпадает с известным методом запроса NOTIFY
				if(::equalsMethod(method, "NOTIFY"))
					// Выводим распознанный метод запроса
					return http::method_t::NOTIFY;
				// Если метод совпадает с известным методом запроса SOURCE
				if(::equalsMethod(method, "SOURCE"))
					// Выводим распознанный метод запроса
					return http::method_t::SOURCE;
				// Если метод совпадает с известным методом запроса UNLINK
				if(::equalsMethod(method, "UNLINK"))
					// Выводим распознанный метод запроса
					return http::method_t::UNLINK;
			} break;
			// Методы с длиной имени 7 символов
			case 7: {
				// Если метод совпадает с известным методом запроса CONNECT
				if(::equalsMethod(method, "CONNECT"))
					// Выводим распознанный метод запроса
					return http::method_t::CONNECT;
				// Если метод совпадает с известным методом запроса OPTIONS
				if(::equalsMethod(method, "OPTIONS"))
					// Выводим распознанный метод запроса
					return http::method_t::OPTIONS;
			} break;
			// Методы с длиной имени 8 символов
			case 8: {
				// Если метод совпадает с известным методом запроса PROPFIND
				if(::equalsMethod(method, "PROPFIND"))
					// Выводим распознанный метод запроса
					return http::method_t::PROPFIND;
				// Если метод совпадает с известным методом запроса CHECKOUT
				if(::equalsMethod(method, "CHECKOUT"))
					// Выводим распознанный метод запроса
					return http::method_t::CHECKOUT;
				// Если метод совпадает с известным методом запроса M-SEARCH
				if(::equalsMethod(method, "M-SEARCH"))
					// Выводим распознанный метод запроса
					return http::method_t::MSEARCH;
			} break;
			// Методы с длиной имени 9 символов
			case 9: {
				// Если метод совпадает с известным методом запроса PROPPATCH
				if(::equalsMethod(method, "PROPPATCH"))
					// Выводим распознанный метод запроса
					return http::method_t::PROPPATCH;
				// Если метод совпадает с известным методом запроса SUBSCRIBE
				if(::equalsMethod(method, "SUBSCRIBE"))
					// Выводим распознанный метод запроса
					return http::method_t::SUBSCRIBE;
			} break;
			// Методы с длиной имени 10 символов
			case 10: {
				// Если метод совпадает с известным методом запроса MKACTIVITY
				if(::equalsMethod(method, "MKACTIVITY"))
					// Выводим распознанный метод запроса
					return http::method_t::MKACTIVITY;
				// Если метод совпадает с известным методом запроса MKCALENDAR
				if(::equalsMethod(method, "MKCALENDAR"))
					// Выводим распознанный метод запроса
					return http::method_t::MKCALENDAR;
			} break;
			// Методы с длиной имени 11 символов
			case 11: {
				// Если метод совпадает с известным методом запроса UNSUBSCRIBE
				if(::equalsMethod(method, "UNSUBSCRIBE"))
					// Выводим распознанный метод запроса
					return http::method_t::UNSUBSCRIBE;
			} break;
		}
		// Метод не распознан
		return http::method_t::NONE;
	}
};

/**
 * @brief Конструктор
 *
 */
awh::http::Parser_HTTP::Limits awh::http::Parser_HTTP::Limits::strict() noexcept {
	// Результат работы функции - строгий набор ограничений
	Limits result;
	// Требуем строгого окончания строк CRLF
	result.strictEOL = true;
	// Запрещаем лишние пробелы внутри стартовой строки
	result.strictSpaces = true;
	// Требуем обязательного заголовка Host у запросов HTTP/1.1
	result.requireHost = true;
	// Выводим результат
	return result;
}

/**
 * @brief Конструктор
 *
 */
awh::http::Parser_HTTP::Limits::Limits() noexcept :
 parser_t::limits_t(),
 strictEOL(STRICT_EOL),
 strictSpaces(STRICT_SPACES),
 requireHost(REQUIRE_HOST),
 maxChunkLine(MAX_CHUNK_LINE),
 maxRequestLine(MAX_REQUEST_LINE),
 maxChunkSize(MAX_CHUNK_SIZE) {}

/**
 * @brief Конструктор
 *
 */
awh::http::Parser_HTTP::Message::Flags::Flags() noexcept :
 chunked(false), upgrade(false),
 complete(false), keepAlive(true),
 expectContinue(false) {}

/**
 * @brief Оператор перемещающего присваивания параметров сообщения
 *
 * @param message объект сообщения для перемещения
 * @return        текущее сообщение
 *
 */
awh::http::Parser_HTTP::Message & awh::http::Parser_HTTP::Message::operator = (Message && message) noexcept {
	// Если перемещаемое сообщение не является текущим объектом
	if(this != &message){
		// Выполняем копирование партиции текущего состояния парсера
		this->part = message.part;
		// Выполняем копирование фазы разбора HTTP-сообщения
		this->phase = message.phase;
		// Выполняем копирование флагов состояния сообщения
		this->flags = message.flags;
		// Выполняем перемещение провайдера заголовков сообщения
		this->provider = ::move(message.provider);
		// Выполняем копирование размера тела сообщения
		this->bodySize = message.bodySize;
		// Сбрасываем размер тела сообщения
		message.bodySize = -1;
		// Сбрасываем партицию текущего состояния парсера
		message.part = part_t::NONE;
		// Сбрасываем фазу разбора HTTP-сообщения
		message.phase = phase_t::NONE;
		// Сбрасываем флаги состояния сообщения
		message.flags = flags_t();
	}
	// Выводим текущий объект
	return * this;
}
/**
 * @brief Оператор присваивания параметров сообщения
 *
 * @param message объект сообщения для копирования
 * @return        текущее сообщение
 *
 */
awh::http::Parser_HTTP::Message & awh::http::Parser_HTTP::Message::operator = (const Message & message) noexcept {
	// Если копируемое сообщение не является текущим объектом
	if(this != &message){
		// Выполняем копирование партиции текущего состояния парсера
		this->part = message.part;
		// Выполняем копирование фазы разбора HTTP-сообщения
		this->phase = message.phase;
		// Выполняем копирование флагов состояния сообщения
		this->flags = message.flags;
		// Выполняем копирование провайдера заголовков сообщения (если он установлен)
		this->provider = (message.provider != nullptr ? message.provider->clone() : nullptr);
		// Выполняем копирование размера тела сообщения
		this->bodySize = message.bodySize;
	}
	// Выводим текущий объект
	return * this;
}
/**
 * @brief Оператор сравнения
 *
 * @param message объект сообщения для сравнения
 * @return        результат сравнения
 *
 */
bool awh::http::Parser_HTTP::Message::operator == (const Message & message) noexcept {
	// Выполняем сравнение всех параметров сообщения кроме провайдера
	const bool result = (
		(this->part == message.part) &&
		(this->phase == message.phase) &&
		(this->flags.chunked == message.flags.chunked) &&
		(this->flags.complete == message.flags.complete) &&
		(this->flags.keepAlive == message.flags.keepAlive) &&
		(this->flags.upgrade == message.flags.upgrade) &&
		(this->flags.expectContinue == message.flags.expectContinue) &&
		(this->bodySize == message.bodySize)
	);
	// Если базовые параметры сообщения не совпадают - дальше сравнивать нет смысла
	if(!result)
		// Сообщения не эквивалентны
		return false;
	// Если оба провайдера отсутствуют - сообщения эквивалентны
	if((this->provider == nullptr) && (message.provider == nullptr))
		// Сообщения эквивалентны
		return true;
	// Если провайдер присутствует только у одного из сообщений
	if((this->provider == nullptr) || (message.provider == nullptr))
		// Сообщения не эквивалентны
		return false;
	// Если направления трафика провайдеров не совпадают
	if(this->provider->direct != message.provider->direct)
		// Сообщения не эквивалентны
		return false;
	/**
	 * В зависимости от направления трафика провайдера, сравниваем содержимое провайдеров
	 */
	switch(static_cast <uint8_t> (this->provider->direct)){
		// Если провайдер содержит запрос клиента
		case static_cast <uint8_t> (direct_t::REQUEST):
			// Сравниваем содержимое запросов клиента
			return ((* static_cast <const request_t *> (this->provider.get())) == (* static_cast <const request_t *> (message.provider.get())));
		// Если провайдер содержит ответ сервера
		case static_cast <uint8_t> (direct_t::RESPONSE):
			// Сравниваем содержимое ответов сервера
			return ((* static_cast <const response_t *> (this->provider.get())) == (* static_cast <const response_t *> (message.provider.get())));
	}
	// Сообщения эквивалентны
	return true;
}
/**
 * @brief Оператор сравнения
 *
 * @param message объект сообщения для сравнения
 * @return        результат сравнения
 *
 */
bool awh::http::Parser_HTTP::Message::operator != (const Message & message) noexcept {
	// Выполняем сравнение всех параметров сообщения
	return !((* this) == message);
}
/**
 * @brief Конструктор перемещения
 *
 * @param message объект сообщения для перемещения
 *
 */
awh::http::Parser_HTTP::Message::Message(Message && message) noexcept :
 part(message.part),
 phase(message.phase),
 flags(message.flags),
 bodySize(message.bodySize),
 provider(::move(message.provider)) {}
/**
 * @brief Конструктор копирования
 *
 * @param message объект сообщения для копирования
 *
 */
awh::http::Parser_HTTP::Message::Message(const Message & message) noexcept :
 part(message.part),
 phase(message.phase),
 flags(message.flags),
 bodySize(message.bodySize),
 provider(message.provider != nullptr ? message.provider->clone() : nullptr) {}
/**
 * @brief Конструктор
 *
 */
awh::http::Parser_HTTP::Message::Message() noexcept :
 part(part_t::NONE), phase(phase_t::NONE), bodySize(-1), provider(nullptr) {}

/**
 * @brief Конструктор
 *
 */
awh::http::Parser_HTTP::Header::Header() noexcept :
 name{""}, value{""}, chunkExt{""} {}

/**
 * @brief Конструктор
 *
 */
awh::http::Parser_HTTP::Statistics_Body::Statistics_Body() noexcept :
 bytes(0), digits(0), chunkSize(0),
 contentLength(0), bytesRemaining(0) {}

/**
 * @brief Конструктор
 *
 */
awh::http::Parser_HTTP::Statistics_Headers::Statistics_Headers() noexcept :
 count(0), bytes(0), lineBytes(0), chunkLineBytes(0) {}

/**
 * @brief Конструктор
 *
 */
awh::http::Parser_HTTP::Flags::Flags() noexcept :
 inTrailers(false), upgradeSeen(false), hostCount(0), leadingBlanks(0),
 connectionClose(false), connectionUpgrade(false),
 contentLengthSeen(false), connectionKeepAlive(false),
 transferEncodingSeen(false), transferEncodingInvalid(false),
 transferEncodingChunkedFinal(false) {}

/**
 * @brief Конструктор
 *
 */
awh::http::Parser_HTTP::Sender::Sender() noexcept :
 endSent(false),
 sourceEof(false),
 headersSent(false),
 writableNotified(false),
 framing(framing_t::NONE),
 lowWater(SEND_LOW_WATER),
 highWater(SEND_HIGH_WATER),
 remaining(0),
 pumpLimit(SOURCE_PUMP_LIMIT),
 source(nullptr) {}

/**
 * @brief Конструктор
 *
 */
awh::http::Parser_HTTP::Callbacks::Callbacks() noexcept :
 data(nullptr), phase(nullptr),
 chunk(nullptr), write(nullptr),
 header(nullptr), provider(nullptr),
 writable(nullptr) {}

/**
 * @brief Метод выбора способа кадрирования тела после завершения заголовков
 *
 */
void awh::http::Parser_HTTP::beginBody() noexcept {
	// Получаем версию протокола из провайдера заголовков сообщения
	const version_t version = this->_message.provider->version;
	/**
	 * Заголовок Transfer-Encoding появился в HTTP/1.1, и сообщение HTTP/1.0 с этим
	 * заголовком получатель обязан считать сообщением с неисправным кадрированием -
	 * даже при наличии Content-Length - и закрыть соединение после его обработки
	 * (RFC 9112 §6.1). Причина в том, что такое сообщение почти наверняка прошло
	 * через звено, не обработавшее кодирование chunked, и часть тела осталась в его
	 * буфере: продолжение работы по соединению интерпретировало бы этот остаток как
	 * следующее сообщение. Проверка выполняется прежде остальных - стандарт отдаёт
	 * ей приоритет над конфликтом Content-Length и Transfer-Encoding
	 */
	if(this->_flags.transferEncodingSeen && (version == version_t::HTTP1_0)){
		// Фиксируем ошибку некорректного Transfer-Encoding
		this->_error = error_t::INVALID_TRANSFER_ENCODING;
		// Выходим из метода
		return;
	}
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
	/**
	 * Валидация заголовка Host выполняется до любых уведомлений: запрос HTTP/1.1
	 * обязан нести ровно один заголовок Host (RFC 9112 §3.2), а расхождение в его
	 * трактовке между звеньями цепочки - вектор подмены целевого узла
	 */
	if(this->_limits.requireHost && (this->_direct == direct_t::REQUEST) && (version == version_t::HTTP1_1) && (this->_flags.hostCount != 1)){
		// Фиксируем ошибку отсутствия либо дублирования заголовка Host
		this->_error = error_t::MISSING_HOST;
		// Выходим из метода
		return;
	}
	/**
	 * Запрос с Transfer-Encoding, не заканчивающимся токеном chunked, кадрировать
	 * невозможно: длина тела не объявлена, а закрытием соединения тело запроса не
	 * ограничивается. Проверка выполняется до любых уведомлений - иначе потребитель
	 * успевает получить блок заголовков и завести приём тела, которого не будет
	 */
	if((this->_direct == direct_t::REQUEST) && this->_flags.transferEncodingSeen && !this->_flags.transferEncodingChunkedFinal){
		// Фиксируем ошибку некорректного Transfer-Encoding
		this->_error = error_t::INVALID_TRANSFER_ENCODING;
		// Выходим из метода
		return;
	}
	// Признак отсутствия тела у ответа сервера по правилам RFC
	const bool noBody = ((this->_direct == direct_t::RESPONSE) && this->responseHasNoBody());
	/**
	 * Лимиты кадрирования проверяются до уведомления потребителя: иначе потребитель
	 * успевает получить готовый блок заголовков и завести приём тела, а следом
	 * получить ошибку по уже начатому сообщению
	 */
	if(this->_flags.contentLengthSeen){
		/**
		 * Размер тела отдаётся наружу знаковым типом, поэтому значение выше INT64_MAX
		 * недопустимо: молчаливое приведение превратило бы его в отрицательное
		 */
		if(this->_statsBody.contentLength > static_cast <uint64_t> (INT64_MAX)){
			// Фиксируем ошибку некорректного Content-Length
			this->_error = error_t::INVALID_CONTENT_LENGTH;
			// Выходим из метода
			return;
		}
		/**
		 * Лимит размера тела применяется только к сообщениям, тело которых
		 * действительно передаётся: у ответа на HEAD и у ответов [204] и [304]
		 * заголовок Content-Length описывает гипотетическое тело, которого
		 * в сообщении нет, и отвергать такой ответ по лимиту недопустимо
		 */
		if(!noBody && (this->_statsBody.contentLength > this->_limits.maxBodySize)){
			// Фиксируем ошибку превышения размера тела
			this->_error = error_t::BODY_OVERFLOW;
			// Выходим из метода
			return;
		}
	}
	/**
	 * Устанавливаем флаг передачи тела chunked: у ответа без тела кадрирование
	 * игнорируется целиком, поэтому флаг обязан остаться сброшенным - иначе
	 * потребитель будет ждать тело, которого в сообщении нет
	 */
	this->_message.flags.chunked = (!noBody && this->_flags.transferEncodingSeen && this->_flags.transferEncodingChunkedFinal);
	// Устанавливаем ожидаемый размер тела сообщения (-1 если Content-Length не получен)
	this->_message.bodySize = (this->_flags.contentLengthSeen ? static_cast <int64_t> (this->_statsBody.contentLength) : -1);
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
	// Флаг завершения сообщения (тела не будет)
	bool endStream = false;
	// Если ответ сервера не имеет тела по правилам RFC - тела не будет
	if(noBody)
		// Устанавливаем флаг завершения сообщения
		endStream = true;
	// Если тело не кадрируется chunked
	else if(!this->_message.flags.chunked) {
		// Если получен заголовок Content-Length - тела не будет только при нулевом размере
		if(this->_flags.contentLengthSeen)
			// Устанавливаем флаг завершения сообщения
			endStream = (this->_statsBody.contentLength == 0);
		// Если кадрирование не определено - у запроса по умолчанию тела нет
		else endStream = (!this->_flags.transferEncodingSeen && (this->_direct == direct_t::REQUEST));
	}
	// Уведомляем о готовности провайдера заголовков сообщения (заголовки разобраны и осмыслены)
	if(!this->fireProvider(this->_message.provider.get(), endStream))
		// Выходим из метода (разбор прерван)
		return;
	// Уведомляем о завершении разбора всех заголовков
	if(!this->firePhase(phase_t::END, part_t::HEADERS))
		// Выходим из метода (разбор прерван)
		return;
	// Если ответ сервера не имеет тела по правилам RFC
	if(noBody){
		// Завершаем разбор всего сообщения
		this->completeMessage();
		// Выходим из метода
		return;
	}
	// Если тело передаётся chunked
	if(this->_message.flags.chunked){
		// Сбрасываем счётчик hex-цифр размера чанка
		this->_statsBody.digits = 0;
		// Сбрасываем размер текущего чанка
		this->_statsBody.chunkSize = 0;
		// Сбрасываем длину строки заголовка чанка
		this->_statsHeaders.chunkLineBytes = 0;
		// Устанавливаем партицию тела сообщения
		this->_message.part = part_t::BODY;
		// Переходим к разбору размера первого чанка
		this->_state = static_cast <uint8_t> (state_t::S_CHUNK_SIZE);
		// Уведомляем о начале приёма тела сообщения (разбор может быть прерван потребителем)
		if(!this->firePhase(phase_t::BEGIN, part_t::BODY))
			// Выходим из метода (код ошибки уже установлен)
			return;
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
		// Устанавливаем партицию тела сообщения
		this->_message.part = part_t::BODY;
		// Переходим к чтению тела фиксированного размера
		this->_state = static_cast <uint8_t> (state_t::S_BODY_IDENTITY);
		// Устанавливаем остаток непрочитанных данных тела
		this->_statsBody.bytesRemaining = this->_statsBody.contentLength;
		// Уведомляем о начале приёма тела сообщения (разбор может быть прерван потребителем)
		if(!this->firePhase(phase_t::BEGIN, part_t::BODY))
			// Выходим из метода (код ошибки уже установлен)
			return;
		// Выходим из метода
		return;
	}
	/**
	 * Если Transfer-Encoding получен, но chunked не является последним кодированием:
	 * у запроса такое кадрирование отвергнуто выше, у ответа тело ограничивается
	 * закрытием соединения
	 */
	if(this->_flags.transferEncodingSeen){
		// Устанавливаем партицию тела сообщения
		this->_message.part = part_t::BODY;
		// Тело кадрируется закрытием соединения - оно не переиспользуемо
		this->_message.flags.keepAlive = false;
		// Переходим к чтению тела до закрытия соединения
		this->_state = static_cast <uint8_t> (state_t::S_BODY_UNTIL_CLOSE);
		// Уведомляем о начале приёма тела сообщения (разбор может быть прерван потребителем)
		if(!this->firePhase(phase_t::BEGIN, part_t::BODY))
			// Выходим из метода (код ошибки уже установлен)
			return;
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
	// Устанавливаем партицию тела сообщения
	this->_message.part = part_t::BODY;
	// Ответ сервера: тело до закрытия соединения - keep-alive невозможен
	this->_message.flags.keepAlive = false;
	// Переходим к чтению тела до закрытия соединения
	this->_state = static_cast <uint8_t> (state_t::S_BODY_UNTIL_CLOSE);
	// Уведомляем о начале приёма тела сообщения (разбор может быть прерван потребителем)
	if(!this->firePhase(phase_t::BEGIN, part_t::BODY))
		// Выходим из метода (код ошибки уже установлен)
		return;
}
/**
 * @brief Метод завершения разбора текущего заголовка/трейлера
 *
 * @return результат обработки (false - разбор прерван)
 *
 */
bool awh::http::Parser_HTTP::commitHeader(const string_view name, string_view value) noexcept {
	/**
	 * Выполняем триминг хвостовых OWS у значения (ведущие уже пропущены вызывающей стороной)
	 */
	while(!value.empty() && ((value.back() == ' ') || (value.back() == '\t')))
		// Отсекаем хвостовой пробел или табуляцию
		value.remove_suffix(1);
	// Если превышено максимальное число заголовков
	if(++this->_statsHeaders.count > this->_limits.maxHeaderCount){
		// Фиксируем ошибку превышения числа заголовков
		this->_error = error_t::TOO_MANY_HEADERS;
		// Разбор прерван
		return false;
	}
	/**
	 * Наращиваем суммарный размер разобранных заголовков: помимо имени и значения
	 * учитываются служебные байты строки (": " и CRLF) - иначе поток заголовков
	 * с пустыми значениями расходует бюджет лимита медленнее, чем занимает канал
	 */
	this->_statsHeaders.bytes += (name.size() + value.size() + 4);
	// Если превышен суммарный размер всех заголовков
	if(this->_statsHeaders.bytes > this->_limits.maxHeadersTotal){
		// Фиксируем ошибку превышения размера заголовков
		this->_error = error_t::HEADER_OVERFLOW;
		// Разбор прерван
		return false;
	}
	/**
	 * Заголовки, непригодные для передачи в трейлерах (RFC 9110 §6.5.1), до
	 * потребителя не доходят: RFC 9112 §7.1.2 разрешает получателю выборочно
	 * отбрасывать полученные трейлеры, и такое поле отбрасывается - иначе
	 * трейлер способен задним числом переопределить кадрирование, маршрутизацию
	 * либо трактовку уже принятого тела в обход внешних фильтров безопасности
	 */
	if(this->_flags.inTrailers){
		// Проверяем запрет полученного поля в блоке трейлеров
		const bool forbidden = ::forbiddenTrailer(name);
		// Если трейлер запрещён в блоке трейлеров
		if(forbidden){
			// Записываем сообщение об отброшенном трейлере в лог
			/**
			 * Название поля печатается представлением с явной длиной: копия в string
			 * ради одного сообщения означала бы аллокацию на каждый отброшенный трейлер,
			 * то есть на пути, который атакующая сторона наполняет по своему желанию
			 */
			this->_log->print(
				"HTTP/1.x trailer field is not allowed and has been dropped: %.*s",
				log_t::flag_t::WARNING, static_cast <int32_t> (name.size()), name.data()
			);
			// Продолжаем разбор
			return true;
		}
	}
	// Если выполняется разбор основных заголовков (трейлеры не влияют на кадрирование)
	if(!this->_flags.inTrailers){
		// Получаем указатель на начало значения заголовка
		const char * begin = value.data();
		// Получаем указатель на конец значения заголовка
		const char * end = (begin + value.size());
		/**
		 * Интерпретация специальных заголовков (диспетчер по первой букве - дёшево)
		 */
		switch(name.empty() ? '\0' : ::lower(name.front())){
			// Заголовки начинающиеся на "C"
			case 'c': {
				// Если получен заголовок Content-Length
				if(::iequalsLit(name.data(), name.size(), "content-length")){
					// Если интерпретация заголовка Content-Length не удалась
					if(!this->applyContentLength(begin, end))
						// Разбор прерван (код ошибки уже установлен)
						return false;
				// Если получен заголовок Connection
				} else if(::iequalsLit(name.data(), name.size(), "connection"))
					// Выполняем интерпретацию заголовка Connection
					this->applyConnection(begin, end);
			} break;
			// Заголовки начинающиеся на "T"
			case 't': {
				// Если получен заголовок Transfer-Encoding
				if(::iequalsLit(name.data(), name.size(), "transfer-encoding"))
					// Выполняем интерпретацию заголовка Transfer-Encoding
					this->applyTransferEncoding(begin, end);
			} break;
			// Заголовки начинающиеся на "U"
			case 'u': {
				// Если получен заголовок Upgrade
				if(::iequalsLit(name.data(), name.size(), "upgrade"))
					// Помечаем что заголовок Upgrade получен
					this->_flags.upgradeSeen = true;
			} break;
			// Заголовки начинающиеся на "H"
			case 'h': {
				// Если получен заголовок Host - учитываем его в счётчике (запрос HTTP/1.1 обязан нести ровно один)
				if((this->_direct == direct_t::REQUEST) && ::iequalsLit(name.data(), name.size(), "host") && (this->_flags.hostCount < 255))
					// Наращиваем счётчик полученных заголовков Host
					++this->_flags.hostCount;
			} break;
			// Заголовки начинающиеся на "E"
			case 'e': {
				// Если получен заголовок Expect (только для запросов)
				if((this->_direct == direct_t::REQUEST) && ::iequalsLit(name.data(), name.size(), "expect"))
					// Выполняем интерпретацию заголовка Expect
					this->applyExpect(begin, end);
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
				STREAM_ID,
				name,
				value,
				(this->_flags.inTrailers ? part_t::TRAILER : part_t::HEADERS)
			)){
				// Фиксируем ошибку прерывания разбора
				this->_error = error_t::ABORTED;
				// Разбор прерван
				return false;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
			// Фиксируем внутреннюю ошибку
			this->_error = error_t::INTERNAL;
			// Разбор прерван
			return false;
		}
		/**
		 * Если функция обратного вызова сбросила парсер - разбор прерывается немедленно
		 *
		 * Проверки в начале цикла разбора для этого мало: она срабатывает лишь на
		 * следующей итерации, а до неё вызывающий код успевает записать в уже
		 * обнулённый автомат - выбрать кадрирование тела, взвести состояние
		 * трейлеров, пометить сообщение завершённым. Отказ здесь отсекает все
		 * такие продолжения через уже имеющиеся проверки результата. Код ошибки
		 * при этом не выставляется: сброс - это не прерывание по воле потребителя,
		 * а обнуление состояния, и ошибкой разбора он не является
		 */
		if(this->_recycled)
			// Разбор прерван
			return false;
	}
	// Продолжаем разбор
	return true;
}
/**
 * @brief Метод разбора целой стартовой строки ответа сервера из входного буфера
 *
 * @param data указатель на входные данные
 * @param size размер входных данных
 * @return     число потреблённых байт либо 0, если быстрый путь неприменим
 *
 */
size_t awh::http::Parser_HTTP::parseStatusLine(const char * data, const size_t size) noexcept {
	// Выполняем поиск окончания стартовой строки
	const void * found = ::memchr(data, '\n', size);
	// Если стартовая строка присутствует во входном буфере не целиком
	if(found == nullptr)
		// Передаём управление посимвольному разбору
		return 0;
	// Определяем длину стартовой строки вместе с её окончанием
	const size_t consumed = (static_cast <size_t> (static_cast <const char *> (found) - data) + 1);
	/**
	 * Быстрый путь берёт на себя только строку с окончанием CRLF: одиночный LF либо
	 * принимается толерантным режимом, либо становится ошибкой в строгом, и оба
	 * решения уже приняты посимвольным разбором
	 */
	if((consumed < 2) || (data[consumed - 2] != '\r'))
		// Передаём управление посимвольному разбору
		return 0;
	// Определяем длину содержимого стартовой строки без её окончания
	const size_t length = (consumed - 2);
	// Если строка короче обязательной части - разбирать крупноблочно нечего
	if(length < STATUS_MINIMUM)
		// Передаём управление посимвольному разбору
		return 0;
	// Определяем версию протокола ответа
	version_t version = version_t::NONE;
	// Если получен литерал версии HTTP/1.1 с обязательным пробелом
	if(::memcmp(data, "HTTP/1.1 ", STATUS_VERSION) == 0)
		// Запоминаем версию протокола ответа
		version = version_t::HTTP1_1;
	// Если получен литерал версии HTTP/1.0 с обязательным пробелом
	else if(::memcmp(data, "HTTP/1.0 ", STATUS_VERSION) == 0)
		// Запоминаем версию протокола ответа
		version = version_t::HTTP1_0;
	/**
	 * Любое иное написание версии, как и лишние пробелы после неё, разбирает
	 * посимвольный путь: он и зафиксирует ошибку в положенном месте, и примет
	 * решение по лишним пробелам согласно режиму строгости
	 */
	else return 0;
	/**
	 * Выполняем проверку кода состояния: ровно три десятичные цифры
	 */
	for(size_t i = STATUS_VERSION; i < STATUS_MINIMUM; ++i){
		// Если символ не является десятичной цифрой
		if(!::isDigit(static_cast <uint8_t> (data[i])))
			// Передаём управление посимвольному разбору
			return 0;
	}
	/**
	 * За кодом состояния следует либо конец строки, либо одиночный пробел и
	 * пояснение к коду. Всё остальное разбирает посимвольный путь
	 */
	if((length > STATUS_MINIMUM) && (data[STATUS_MINIMUM] != ' '))
		// Передаём управление посимвольному разбору
		return 0;
	// Определяем позицию начала пояснения к коду состояния
	const size_t position = ((length > STATUS_MINIMUM) ? (STATUS_MINIMUM + 1) : length);
	/**
	 * Выполняем проверку допустимости всех символов пояснения к коду состояния
	 */
	for(size_t i = position; i < length; ++i){
		// Если символ недопустим в пояснении к коду состояния
		if(!::isValueCh(static_cast <uint8_t> (data[i])))
			// Передаём управление посимвольному разбору
			return 0;
	}
	// Если длина стартовой строки превышает лимит
	if(length > this->_limits.maxRequestLine)
		// Передаём управление посимвольному разбору
		return 0;
	// Получаем объект провайдера заголовков ответа сервера
	response_t * res = static_cast <response_t *> (this->_message.provider.get());
	// Устанавливаем версию протокола ответа
	res->version = version;
	// Устанавливаем код состояния ответа
	res->code = static_cast <uint16_t> (((data[STATUS_VERSION] - '0') * 100) +
	 ((data[STATUS_VERSION + 1] - '0') * 10) + (data[STATUS_VERSION + 2] - '0'));
	/**
	 * Учитываем разобранные цифры кода состояния: на кадрирование счётчик не влияет -
	 * перед разбором чанков он обнуляется, - но оба пути обязаны оставлять парсер в
	 * одинаковом состоянии. Это инвариант, на котором держится сверка путей, и
	 * расхождение в нём стоит закрывать сразу, а не когда оно на что-нибудь повлияет
	 */
	this->_statsBody.digits = 3;
	// Устанавливаем пояснение к коду состояния
	res->message.assign(data + position, (length - position));
	// Учитываем разобранную стартовую строку в её длине
	this->_statsHeaders.lineBytes = length;
	// Завершаем разбор стартовой строки
	this->commitStartLine();
	// Выводим число потреблённых байт
	return consumed;
}
/**
 * @brief Метод разбора целой строки заголовка из входного буфера
 *
 * @param data указатель на входные данные
 * @param size размер входных данных
 * @return     число потреблённых байт либо 0, если быстрый путь неприменим
 *
 */
size_t awh::http::Parser_HTTP::parseHeaderLine(const char * data, const size_t size) noexcept {
	// Выполняем поиск окончания строки заголовка
	const void * found = ::memchr(data, '\n', size);
	// Если строка заголовка присутствует во входном буфере не целиком
	if(found == nullptr)
		// Передаём управление посимвольному разбору
		return 0;
	// Определяем длину строки заголовка вместе с её окончанием
	const size_t consumed = (static_cast <size_t> (static_cast <const char *> (found) - data) + 1);
	// Определяем длину содержимого строки заголовка без её окончания
	size_t length = (consumed - 1);
	// Если содержимое строки заголовка завершается возвратом каретки
	if((length > 0) && (data[length - 1] == '\r'))
		// Отсекаем возврат каретки от содержимого строки заголовка
		--length;
	/**
	 * Одиночный LF в роли окончания строки разбирается посимвольным путём: он
	 * либо принимается толерантным режимом, либо становится ошибкой в строгом,
	 * и оба решения уже приняты там
	 */
	else if(length == (consumed - 1))
		// Передаём управление посимвольному разбору
		return 0;
	/**
	 * Пустая строка завершает блок заголовков и переводит разбор к телу
	 * сообщения - решение принимает посимвольный путь
	 */
	if(length == 0)
		// Передаём управление посимвольному разбору
		return 0;
	// Позиция конца имени заголовка
	size_t position = 0;
	/**
	 * Сканируем имя заголовка по таблице символов токена
	 */
	while((position < length) && ::isToken(static_cast <uint8_t> (data[position])))
		// Смещаем позицию конца имени заголовка
		++position;
	/**
	 * Имя обязано быть непустым и завершаться двоеточием: пустое имя, obs-fold
	 * и пробел перед двоеточием - ошибки, которые фиксирует посимвольный путь
	 */
	if((position == 0) || (position >= length) || (data[position] != ':'))
		// Передаём управление посимвольному разбору
		return 0;
	// Запоминаем длину имени заголовка
	const size_t nameLength = position;
	// Пропускаем двоеточие после имени заголовка
	++position;
	/**
	 * Пропускаем ведущие OWS значения заголовка
	 */
	while((position < length) && ((data[position] == ' ') || (data[position] == '\t')))
		// Смещаем позицию начала значения заголовка
		++position;
	// Определяем число пропущенных октетов ведущих OWS значения заголовка
	const size_t spaces = (position - (nameLength + 1));
	/**
	 * Если пропущенные OWS не укладываются в бюджет блока заголовков
	 *
	 * Отброшенные при разборе октеты входят в бюджет наравне с сохранёнными: канал
	 * и процессорное время они занимают такие же, а без их учёта одна строка вида
	 * "X:" с потоком пробелов растягивалась бы неограниченно, не приближая разбор
	 * к завершению и не расходуя ни одного лимита
	 */
	if((this->_statsHeaders.bytes + spaces) > this->_limits.maxHeadersTotal)
		// Передаём управление посимвольному разбору
		return 0;
	/**
	 * Выполняем проверку допустимости всех символов значения заголовка
	 */
	for(size_t i = position; i < length; ++i){
		// Если символ недопустим в значении заголовка
		if(!::isValueCh(static_cast <uint8_t> (data[i])))
			// Передаём управление посимвольному разбору
			return 0;
	}
	// Если длина имени заголовка превышает лимит
	if(nameLength > this->_limits.maxHeaderName)
		// Передаём управление посимвольному разбору
		return 0;
	// Если длина значения заголовка превышает лимит
	if((length - position) > this->_limits.maxHeaderValue)
		// Передаём управление посимвольному разбору
		return 0;
	// Учитываем пропущенные октеты ведущих OWS в бюджете блока заголовков
	this->_statsHeaders.bytes += spaces;
	/**
	 * Завершаем разбор заголовка представлениями во входной буфер
	 *
	 * Результат намеренно не проверяется: метод завершается следующей же строкой,
	 * и проверка породила бы мёртвую ветку. Прерывание разбора потребителем
	 * фиксируется кодом ошибки внутри самого уведомления и поднимается проверкой
	 * в ветке крупноблочной обработки, а сброс парсера - проверкой в начале
	 * очередного шага цикла разбора
	 */
	this->commitHeader(string_view(data, nameLength), string_view(data + position, (length - position)));
	// Выводим число потреблённых байт
	return consumed;
}
/**
 * @brief Метод завершения разбора стартовой строки (request-line/status-line)
 *
 * @return результат обработки (false - разбор прерван)
 *
 */
bool awh::http::Parser_HTTP::commitStartLine() noexcept {
	// Переходим к разбору заголовков
	this->_state = static_cast <uint8_t> (state_t::S_HEADER_START);
	// Продолжаем разбор (провайдер отдаётся по завершению блока заголовков, как END_HEADERS у HTTP/2)
	return true;
}
/**
 * @brief Метод завершения разбора всего сообщения
 *
 */
void awh::http::Parser_HTTP::completeMessage() noexcept {
	// Сбрасываем партицию текущего состояния парсера
	this->_message.part = part_t::NONE;
	// Устанавливаем фазу окончания разбора сообщения
	this->_message.phase = phase_t::END;
	// Устанавливаем финальное состояние конечного автомата
	this->_state = static_cast <uint8_t> (state_t::S_MESSAGE_DONE);
	/**
	 * Признак полноты сообщения выставляется только после успешного уведомления:
	 * прерывание разбора потребителем переводит парсер в состояние ошибки, и
	 * полностью разобранным такое сообщение считаться не может
	 */
	if(this->firePhase(phase_t::END, part_t::NONE))
		// Помечаем что сообщение полностью разобрано
		this->_message.flags.complete = true;
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
		this->_state = static_cast <uint8_t> (state_t::S_TRAILER_START);
		/**
		 * Уведомляем о начале разбора трейлеров
		 *
		 * Результат намеренно не проверяется: метод завершается следующей же
		 * строкой, и проверка породила бы мёртвую ветку. Прерывание разбора
		 * потребителем фиксируется в коде ошибки внутри самого уведомления и
		 * поднимается внешним контролем на следующем шаге цикла разбора
		 */
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
	this->_state = static_cast <uint8_t> (state_t::S_CHUNK_DATA);
}
/**
 * @brief Метод проверки отсутствия тела у ответа сервера (по статус-коду и методу запроса)
 *
 * @return результат проверки
 *
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
 * @brief Метод фиксации ошибки разбора (код ошибки, итоговый статус и запись в лог)
 *
 * @param error код ошибки разбора
 *
 */
void awh::http::Parser_HTTP::fail(const error_t error) noexcept {
	// Фиксируем код ошибки разбора
	this->_error = error;
	// Устанавливаем итоговый статус разбора
	this->_status = status_t::ERROR;
	// Получаем название кода ошибки разбора
	const string_view name = this->errorName(error);
	/**
	 * Название печатается представлением с явной длиной: копия в string ради одного
	 * сообщения означала бы аллокацию на каждую ошибку разбора, то есть на пути,
	 * который атакующая сторона наполняет по своему желанию
	 */
	this->_log->print(
		"HTTP/1.x %s parsing failed: %.*s",
		log_t::flag_t::WARNING,
		(this->_direct == direct_t::REQUEST ? "request" : "response"),
		static_cast <int32_t> (name.size()), name.data()
	);
}
/**
 * @brief Метод передачи исходящих байтов сетевому слою через функцию обратного вызова записи
 *
 * @details Если функция записи не установлена - байты остаются во внутреннем
 *          буфере до выборки через pending()/consumePending().
 *
 */
void awh::http::Parser_HTTP::flush() noexcept {
	// Если функция обратного вызова записи не установлена - работаем в pull-модели
	if(this->_callbacks.write == nullptr)
		// Выходим из метода
		return;
	/**
	 * Отдаём исходящие байты, пока они есть: функция обратного вызова могла
	 * реентрантно породить новые исходящие данные (например, через sendData)
	 */
	while(!this->_sender.output.empty()){
		/**
		 * Переносим отдаваемые байты в отдельный буфер обменом (O(1)): функция
		 * обратного вызова вправе реентрантно дописать новые исходящие данные,
		 * а дозапись способна переместить содержимое буфера в памяти и оставить
		 * отдаваемую область висящей. Выделенная память при обмене сохраняется
		 * в обоих буферах и между передачами не переаллоцируется
		 */
		this->_sender.flushing.swap(this->_sender.output);
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Отдаём исходящие байты сетевому слою
			this->_callbacks.write(this->_sender.flushing.data(), this->_sender.flushing.size());
			// Освобождаем отданные байты
			this->_sender.flushing.clear();
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
			/**
			 * Возвращаем неотданные байты обратно в выходной буфер: они ещё не попали
			 * в сеть, а их потеря разорвала бы кадрирование у принимающей стороны
			 */
			if(!this->_sender.output.empty())
				// Дописываем реентрантно порождённые байты после неотданного остатка
				this->_sender.flushing.push(this->_sender.output);
			// Возвращаем неотданный остаток в выходной буфер
			this->_sender.output.swap(this->_sender.flushing);
			// Освобождаем буфер передачи
			this->_sender.flushing.clear();
			// Прерываем передачу исходящих байтов
			return;
		}
		/**
		 * Если реентрантной дозаписи не было - возвращаем выделенную память
		 * выходному буферу, чтобы следующая запись не начинала рост заново
		 */
		if(this->_sender.output.empty())
			// Возвращаем выделенную память в выходной буфер
			this->_sender.output.swap(this->_sender.flushing);
	}
}
/**
 * @brief Метод получения логического объёма ещё не отправленных исходящих байтов
 *
 * @return объём не отправленных исходящих байтов
 *
 */
size_t awh::http::Parser_HTTP::outputPending() const noexcept {
	// Выводим объём ещё не отданных исходящих байтов
	return this->_sender.output.size();
}
/**
 * @brief Метод дозагрузки выходного буфера из pull-источника данных (если он задан)
 *
 * @details Источник пишет напрямую в выходной буфер (без промежуточной копии),
 *          кадрирование тела применяется к каждой полученной порции.
 *
 * @return число полученных от источника байт тела
 *
 */
size_t awh::http::Parser_HTTP::refillFromSource() noexcept {
	// Результат работы функции - число полученных от источника байт тела
	size_t result = 0;
	// Если источник данных не задан либо его тело уже закончилось - дозагружать нечего
	if((this->_sender.source == nullptr) || this->_sender.sourceEof)
		// Выводим результат
		return result;
	/**
	 * Страховка инварианта: сообщение без выбранного кадрирования тела не принимает,
	 * и некадрированные байты на провод уйти не должны - получатель прочитал бы их
	 * как начало следующего сообщения. Сейчас сюда попасть нельзя: единственный
	 * способ получить кадрирование NONE - завершить сообщение заголовками либо
	 * отказаться кадрировать тело, а оба случая помечают сообщение завершённым,
	 * и прокачка отсекается ещё в pumpSource. Проверка оставлена намеренно - она
	 * удерживает инвариант, если появится новый путь к кадрированию NONE при
	 * незавершённом сообщении. Источник при срабатывании удаляется, чтобы не
	 * удерживать захваченные им ресурсы до сброса отправителя: холостой прокачки
	 * это уже не касается - её отсекает сам признак sourcePending, требующий
	 * выбранного кадрирования
	 */
	if(this->_sender.framing == sender_t::framing_t::NONE){
		// Помечаем что дозагружать из источника нечего
		this->_sender.sourceEof = true;
		// Удаляем источник данных (сообщение осталось незавершённым - соединение следует закрыть)
		this->_sender.source = nullptr;
		// Записываем сообщение об отброшенном источнике тела в лог
		this->_log->print(
			"HTTP/1.x outgoing message accepts no body: the pull data source has been dropped",
			log_t::flag_t::CRITICAL
		);
		// Выводим результат
		return result;
	}
	// Ширина фиксированного hex-заголовка чанка (четыре hex-цифры размера порции и CRLF)
	static constexpr size_t CHUNK_HEADER = (4 + 2);
	/**
	 * Заголовок чанка резервируется фиксированной ширины, поэтому гранулярность порции
	 * обязана укладываться в четыре шестнадцатеричные цифры: при её увеличении заголовок
	 * молча не поместился бы в зарезервированное место и разорвал бы кадрирование
	 */
	static_assert((SOURCE_CHUNK_SIZE <= 0xFFFF), "Chunk header width does not fit the source chunk size");
	/**
	 * Держим буфер наполненным до high-water, запрашивая источник данных порциями
	 */
	while((this->outputPending() < this->_sender.highWater) && !this->_sender.sourceEof){
		// Признак кадрирования тела кодировкой chunked
		const bool chunked = (this->_sender.framing == sender_t::framing_t::CHUNKED);
		// Вычисляем ёмкость запрашиваемой порции
		size_t cap = ::min(SOURCE_CHUNK_SIZE, this->_sender.highWater - this->outputPending());
		// Для кадрирования фиксированного размера ограничиваем порцию остатком Content-Length
		if(this->_sender.framing == sender_t::framing_t::IDENTITY)
			// Ограничиваем порцию остатком тела до полного Content-Length
			cap = ::min(cap, static_cast <size_t> (this->_sender.remaining));
		// Если тело фиксированного размера полностью получено
		if(cap == 0){
			// Помечаем что конец тела источника достигнут
			this->_sender.sourceEof = true;
			// Завершаем тело исходящего сообщения
			this->finishBody();
			// Выходим из цикла дозагрузки
			break;
		}
		/**
		 * Резервируем место под порцию прямо в хвосте выходного буфера: смартбуфер
		 * отдаёт указатель на свободную область без её инициализации и без копии,
		 * а незафиксированный хвост просто не попадает в буфер - откат бесплатен
		 */
		void * area = this->_sender.output.prepare((chunked ? (CHUNK_HEADER + cap + 2) : cap));
		// Если зарезервировать место не удалось - дозагружать некуда
		if(area == nullptr)
			// Выходим из цикла дозагрузки
			break;
		// Получаем байтовый указатель на зарезервированную область
		uint8_t * region = reinterpret_cast <uint8_t *> (area);
		// Флаг достижения конца тела
		bool eof = false;
		// Результат запроса данных у источника
		int64_t bytes = -1;
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Запрашиваем порцию данных у источника (источник пишет напрямую в выходной буфер)
			bytes = this->_sender.source(STREAM_ID, (region + (chunked ? CHUNK_HEADER : 0)), cap, eof);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
		// Если источник сообщил об ошибке данных либо нарушил контракт (записал больше ёмкости)
		if((bytes < 0) || (bytes > static_cast <int64_t> (cap))){
			// Помечаем что конец тела источника достигнут (отправка прервана)
			this->_sender.sourceEof = true;
			// Удаляем источник данных (сообщение осталось незавершённым - соединение следует закрыть)
			this->_sender.source = nullptr;
			// Выходим из цикла дозагрузки (зарезервированное место не фиксируется)
			break;
		}
		// Если источник выдал данные
		if(bytes > 0){
			// Если тело кадрируется chunked
			if(chunked){
				// Формируем hex-заголовок чанка фиксированной ширины (ведущие нули допустимы по RFC 9112)
				region[0] = static_cast <uint8_t> ("0123456789ABCDEF"[(bytes >> 12) & 0x0F]);
				// Формируем вторую цифру hex-заголовка чанка
				region[1] = static_cast <uint8_t> ("0123456789ABCDEF"[(bytes >> 8) & 0x0F]);
				// Формируем третью цифру hex-заголовка чанка
				region[2] = static_cast <uint8_t> ("0123456789ABCDEF"[(bytes >> 4) & 0x0F]);
				// Формируем четвёртую цифру hex-заголовка чанка
				region[3] = static_cast <uint8_t> ("0123456789ABCDEF"[bytes & 0x0F]);
				// Дописываем возврат каретки заголовка чанка
				region[4] = static_cast <uint8_t> ('\r');
				// Дописываем перевод строки заголовка чанка
				region[5] = static_cast <uint8_t> ('\n');
				// Дописываем возврат каретки завершающего CRLF чанка
				region[CHUNK_HEADER + static_cast <size_t> (bytes)] = static_cast <uint8_t> ('\r');
				// Дописываем перевод строки завершающего CRLF чанка
				region[CHUNK_HEADER + static_cast <size_t> (bytes) + 1] = static_cast <uint8_t> ('\n');
				// Фиксируем в буфере заголовок чанка, его данные и завершающий CRLF
				this->_sender.output.commit(CHUNK_HEADER + static_cast <size_t> (bytes) + 2);
			// Для остальных способов кадрирования фиксируем только сами данные
			} else this->_sender.output.commit(static_cast <size_t> (bytes));
			// Для тела фиксированного размера списываем порцию из остатка Content-Length
			if(this->_sender.framing == sender_t::framing_t::IDENTITY)
				// Списываем порцию из остатка тела
				this->_sender.remaining -= static_cast <uint64_t> (bytes);
			// Учитываем полученные байты тела в результате
			result += static_cast <size_t> (bytes);
		}
		// Если достигнут конец тела источника
		if(eof){
			// Помечаем что конец тела источника достигнут
			this->_sender.sourceEof = true;
			/**
			 * Тело фиксированного размера завершается строго по исчерпании анонсированного
			 * Content-Length - той же политикой, что и на пути sendData. Источник, объявивший
			 * конец тела раньше, выдал бы в сеть усечённое тело: получатель дочитывал бы
			 * недостающие байты до таймаута, а в конвейере принял бы за них начало следующего
			 * сообщения. Сообщение остаётся незавершённым - вызывающая сторона либо дошлёт
			 * остаток методом sendData, либо закроет соединение, - а источник удаляется:
			 * конец тела он уже объявил, и держать захваченные им ресурсы незачем
			 */
			if((this->_sender.framing == sender_t::framing_t::IDENTITY) && (this->_sender.remaining > 0)){
				// Удаляем источник данных тела (сообщение осталось незавершённым)
				this->_sender.source = nullptr;
				// Записываем сообщение о преждевременном завершении тела источником в лог
				this->_log->print(
					"HTTP/1.x pull data source ended the body shorter than the announced Content-Length: %llu byte(s) left, the message is left unfinished",
					log_t::flag_t::CRITICAL,
					static_cast <unsigned long long> (this->_sender.remaining)
				);
			// Для остальных способов кадрирования конец тела источника завершает тело
			} else this->finishBody();
		}
		// Если источник временно без данных - прерываем дозагрузку
		if((bytes == 0) && !eof)
			// Прерываем дозагрузку до следующей прокачки
			break;
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод прокачки pull-источника данных в сеть
 *
 * @details В push-режиме (установлена функция обратного вызова записи) качает
 *          источник до конца тела либо до временного отсутствия данных.
 *          В pull-режиме наполняет выходной буфер до high-water однократно -
 *          досылка происходит по мере выборки consumePending().
 *
 */
void awh::http::Parser_HTTP::pumpSource() noexcept {
	// Если сообщение не находится в фазе отправки тела - качать нечего
	if(!this->_sender.headersSent || this->_sender.endSent)
		// Выходим из метода
		return;
	// Если установлена функция обратного вызова записи - качаем источник порциями
	if(this->_callbacks.write != nullptr){
		// Объём тела, выкачанный из источника за текущую прокачку
		uint64_t pumped = 0;
		/**
		 * Качаем источник, пока он выдаёт данные: flush() опустошает буфер, поэтому
		 * дозагрузка продолжается до конца тела, до паузы источника либо до исчерпания
		 * лимита одной прокачки. Лимит не даёт телу произвольного размера удержать
		 * управление внутри одного вызова - остаток дозагружается через resumeSource()
		 */
		while(!this->_sender.sourceEof && (this->_sender.source != nullptr) && (pumped < this->_sender.pumpLimit)){
			// Дозагружаем выходной буфер из источника данных
			const size_t bytes = this->refillFromSource();
			// Передаём исходящие байты сетевому слою
			this->flush();
			// Если источник временно без данных - прерываем прокачку
			if(bytes == 0)
				// Прерываем прокачку до следующего вызова
				break;
			// Наращиваем объём тела, выкачанный за текущую прокачку
			pumped += static_cast <uint64_t> (bytes);
		}
		// Отдаём сформированный финал тела (последний чанк) сетевому слою
		this->flush();
	// В pull-модели наполняем выходной буфер до high-water однократно
	} else this->refillFromSource();
}
/**
 * @brief Метод получения признака незавершённой отправки тела из pull-источника
 *
 * @details Истинно, пока источник данных назначен, его тело не исчерпано и очередная
 *          прокачка способна продвинуться: заголовки отправлены, сообщение не завершено
 *          и его тело есть чем кадрировать. Ровно этот признак управляет циклом
 *          дозагрузки: пока он истинен, сетевому слою следует вызывать resumeSource()
 *          по готовности сокета к записи. Учёт способности продвинуться обязателен -
 *          иначе источник, назначенный до отправки заголовков, либо источник сообщения,
 *          тело которого кадрировать нечем, удерживали бы признак истинным навсегда
 *          и цикл дозагрузки крутился бы вхолостую
 *
 * @return признак незавершённой отправки тела
 *
 */
bool awh::http::Parser_HTTP::sourcePending() const noexcept {
	// Если источник данных не назначен либо его тело исчерпано - отправка завершена
	if((this->_sender.source == nullptr) || this->_sender.sourceEof)
		// Выводим признак завершённой отправки тела
		return false;
	/**
	 * Назначенного источника недостаточно: признак управляет циклом дозагрузки, и
	 * истинным он обязан быть только тогда, когда очередная прокачка способна
	 * продвинуться. Прокачка отсекается в pumpSource теми же условиями, поэтому
	 * без их учёта признак остался бы истинным навсегда, а сетевой слой крутил бы
	 * resumeSource вхолостую. Так происходит с источником, назначенным до отправки
	 * заголовков, и с источником сообщения, тело которого кадрировать нечем
	 */
	return (this->_sender.headersSent && !this->_sender.endSent && (this->_sender.framing != sender_t::framing_t::NONE));
}
/**
 * @brief Метод продолжения отправки тела из pull-источника данных
 *
 * @details За одну прокачку из источника выкачивается не более лимита,
 *          заданного методом pumpLimit - тело произвольного размера не
 *          удерживает управление внутри одного вызова. Сетевой слой обязан
 *          вызывать метод по готовности сокета к записи, пока он возвращает
 *          истину. В pull-модели дозагрузка выполняется автоматически при
 *          выборке consumePending, и вызывать метод не требуется.
 *
 * @return признак того, что тело источника ещё не исчерпано
 *
 */
bool awh::http::Parser_HTTP::resumeSource() noexcept {
	// Если отправка тела из источника уже завершена - продолжать нечего
	if(!this->sourcePending())
		// Выводим признак завершения отправки тела
		return false;
	// Прокачиваем очередную порцию тела из источника данных
	this->pumpSource();
	// Выводим признак незавершённой отправки тела
	return this->sourcePending();
}
/**
 * @brief Метод настройки объёма одной прокачки pull-источника данных
 *
 * @param size максимальный объём тела, выкачиваемый за одну прокачку
 *
 */
void awh::http::Parser_HTTP::pumpLimit(const uint64_t size) noexcept {
	// Устанавливаем объём одной прокачки (нулевое значение сняло бы ограничение целиком)
	this->_sender.pumpLimit = ((size > 0) ? size : SOURCE_PUMP_LIMIT);
}
/**
 * @brief Метод сигнализации о готовности принимать данные (один раз на провал буфера)
 *
 */
void awh::http::Parser_HTTP::maybeNotifyWritable() noexcept {
	// Сигнал отдаём только для push-модели (sendData), не для pull-источника данных
	if((this->_sender.source != nullptr) || (this->_callbacks.writable == nullptr))
		// Выходим из метода
		return;
	// Если сообщение не находится в фазе отправки тела - сигналить не о чем
	if(!this->_sender.headersSent || this->_sender.endSent)
		// Выходим из метода
		return;
	// Если сигнал ещё не подан и выходной буфер опустился ниже low-water
	if(!this->_sender.writableNotified && (this->outputPending() <= this->_sender.lowWater)){
		// Помечаем что сигнал для текущего провала буфера подан
		this->_sender.writableNotified = true;
		// Уведомляем о готовности принимать данные
		this->_callbacks.writable(STREAM_ID);
	}
}
/**
 * @brief Метод завершения тела исходящего сообщения (финализация кадрирования)
 *
 */
void awh::http::Parser_HTTP::finishBody() noexcept {
	// Если тело кадрируется chunked - завершаем его последним (нулевым) чанком
	if(this->_sender.framing == sender_t::framing_t::CHUNKED)
		// Дописываем последний чанк и пустой блок трейлеров
		this->_sender.output.push("0\r\n\r\n", 5);
	// Помечаем что исходящее сообщение завершено
	this->_sender.endSent = true;
}
/**
 * @brief Метод кадрирования и записи порции тела в выходной буфер
 *
 * @param buffer буфер данных тела
 * @param size   размер данных тела
 *
 */
void awh::http::Parser_HTTP::frameBody(const void * buffer, const size_t size) noexcept {
	// Если данных для кадрирования нет
	if(size == 0)
		// Выходим из метода
		return;
	/**
	 * Определяем способ кадрирования тела исходящего сообщения
	 */
	switch(static_cast <uint8_t> (this->_sender.framing)){
		// Кодировка chunked - каждая порция оборачивается в отдельный чанк
		case static_cast <uint8_t> (sender_t::framing_t::CHUNKED): {
			// Текстовый буфер hex-представления размера чанка (16 цифр uint64_t и CRLF)
			char line[18];
			// Формируем строку размера чанка (hex + CRLF)
			const size_t length = ::writeChunkSize(line, static_cast <uint64_t> (size));
			// Дописываем строку размера чанка в выходной буфер
			this->_sender.output.push(line, length);
			// Дописываем данные чанка в выходной буфер
			this->_sender.output.push(buffer, size);
			// Дописываем завершающий CRLF чанка
			this->_sender.output.push("\r\n", 2);
		} break;
		// Фиксированный размер (Content-Length) и сырое тело - данные пишутся как есть
		case static_cast <uint8_t> (sender_t::framing_t::RAW):
		case static_cast <uint8_t> (sender_t::framing_t::IDENTITY):
			// Дописываем данные тела в выходной буфер
			this->_sender.output.push(buffer, size);
		break;
	}
}
/**
 * @brief Метод вызова функции обратного вызова обработки фазы разбора
 *
 * @param phase фаза разбора HTTP-сообщения
 * @param part  часть сообщения
 * @return      результат обработки (false - разбор прерван)
 *
 */
bool awh::http::Parser_HTTP::firePhase(const phase_t phase, const part_t part) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callbacks.phase != nullptr){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если функция обратного вызова потребовала прервать разбор
			if(!this->_callbacks.phase(STREAM_ID, phase, part)){
				// Фиксируем ошибку прерывания разбора
				this->_error = error_t::ABORTED;
				// Разбор прерван
				return false;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (phase), static_cast <uint16_t> (part)), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
			// Фиксируем внутреннюю ошибку
			this->_error = error_t::INTERNAL;
			// Разбор прерван
			return false;
		}
		/**
		 * Если функция обратного вызова сбросила парсер - разбор прерывается немедленно
		 *
		 * Проверки в начале цикла разбора для этого мало: она срабатывает лишь на
		 * следующей итерации, а до неё вызывающий код успевает записать в уже
		 * обнулённый автомат - выбрать кадрирование тела, взвести состояние
		 * трейлеров, пометить сообщение завершённым. Отказ здесь отсекает все
		 * такие продолжения через уже имеющиеся проверки результата. Код ошибки
		 * при этом не выставляется: сброс - это не прерывание по воле потребителя,
		 * а обнуление состояния, и ошибкой разбора он не является
		 */
		if(this->_recycled)
			// Разбор прерван
			return false;
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
 *
 */
bool awh::http::Parser_HTTP::fireChunk(const phase_t phase, const uint64_t size) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callbacks.chunk != nullptr){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если функция обратного вызова потребовала прервать разбор
			if(!this->_callbacks.chunk(phase, size, this->_header.chunkExt)){
				// Фиксируем ошибку прерывания разбора
				this->_error = error_t::ABORTED;
				// Разбор прерван
				return false;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (phase), size), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
			// Фиксируем внутреннюю ошибку
			this->_error = error_t::INTERNAL;
			// Разбор прерван
			return false;
		}
		/**
		 * Если функция обратного вызова сбросила парсер - разбор прерывается немедленно
		 *
		 * Проверки в начале цикла разбора для этого мало: она срабатывает лишь на
		 * следующей итерации, а до неё вызывающий код успевает записать в уже
		 * обнулённый автомат - выбрать кадрирование тела, взвести состояние
		 * трейлеров, пометить сообщение завершённым. Отказ здесь отсекает все
		 * такие продолжения через уже имеющиеся проверки результата. Код ошибки
		 * при этом не выставляется: сброс - это не прерывание по воле потребителя,
		 * а обнуление состояния, и ошибкой разбора он не является
		 */
		if(this->_recycled)
			// Разбор прерван
			return false;
	}
	// Продолжаем разбор
	return true;
}
/**
 * @brief Метод вызова функции обратного вызова обработки провайдера заголовков сообщения
 *
 * @param provider  объект провайдера заголовков сообщения (nullptr для трейлеров)
 * @param endStream флаг завершения сообщения (тела не будет)
 * @return          результат обработки (false - разбор прерван с ошибкой ABORTED)
 *
 */
bool awh::http::Parser_HTTP::fireProvider(const provider_t * provider, const bool endStream) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callbacks.provider != nullptr){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если функция обратного вызова потребовала прервать разбор
			if(!this->_callbacks.provider(STREAM_ID, provider, endStream)){
				// Фиксируем ошибку прерывания разбора
				this->_error = error_t::ABORTED;
				// Разбор прерван
				return false;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(endStream), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
			// Фиксируем внутреннюю ошибку
			this->_error = error_t::INTERNAL;
			// Разбор прерван
			return false;
		}
		/**
		 * Если функция обратного вызова сбросила парсер - разбор прерывается немедленно
		 *
		 * Проверки в начале цикла разбора для этого мало: она срабатывает лишь на
		 * следующей итерации, а до неё вызывающий код успевает записать в уже
		 * обнулённый автомат - выбрать кадрирование тела, взвести состояние
		 * трейлеров, пометить сообщение завершённым. Отказ здесь отсекает все
		 * такие продолжения через уже имеющиеся проверки результата. Код ошибки
		 * при этом не выставляется: сброс - это не прерывание по воле потребителя,
		 * а обнуление состояния, и ошибкой разбора он не является
		 */
		if(this->_recycled)
			// Разбор прерван
			return false;
	}
	// Продолжаем разбор
	return true;
}
/**
 * @brief Метод интерпретации заголовка Connection
 *
 * @param begin начало значения заголовка
 * @param end   конец значения заголовка
 *
 */
void awh::http::Parser_HTTP::applyConnection(const char * begin, const char * end) noexcept {
	// Указатель на текущую позицию разбора
	const char * current = begin;
	/**
	 * Выполняем разбор списка токенов заголовка Connection
	 */
	while(current <= end){
		// Выполняем поиск разделителя списка
		const char * comma = static_cast <const char *> (::memchr(current, ',', static_cast <size_t> (end - current)));
		// Определяем конец текущего элемента списка
		const char * tokEnd = (comma != nullptr ? comma : end);
		// Указатель на начало элемента списка
		const char * tb = current;
		// Указатель на конец элемента списка
		const char * te = tokEnd;
		// Выполняем триминг OWS по краям элемента списка
		::trimOWS(tb, te);
		/**
		 * Отсекаем параметры от имени токена: элементом списка Connection обязан быть
		 * голый токен (RFC 9110 §7.6.1), и точка с запятой в нём недопустима. Значение
		 * вида "close;foo" толкуется как close - расхождение в трактовке с соседним
		 * звеном цепочки решается в пользу закрытия соединения: удержать открытым то,
		 * что пир считает закрытым, опаснее обратного
		 */
		::trimParameters(tb, te);
		// Получаем размер элемента списка
		const size_t size = static_cast <size_t> (te - tb);
		// Если элемент списка не пустой
		if(size > 0){
			// Если получен токен close
			if(::iequalsLit(tb, size, "close"))
				// Помечаем что соединение должно быть закрыто
				this->_flags.connectionClose = true;
			// Если получен токен keep-alive
			else if(::iequalsLit(tb, size, "keep-alive"))
				// Помечаем что соединение переиспользуемое
				this->_flags.connectionKeepAlive = true;
			// Если получен токен upgrade
			else if(::iequalsLit(tb, size, "upgrade"))
				// Помечаем что запрошено переключение протокола
				this->_flags.connectionUpgrade = true;
		}
		// Если разделителей больше нет - разбор списка завершён
		if(comma == nullptr)
			// Выходим из цикла
			break;
		// Переходим к следующему элементу списка
		current = (comma + 1);
	}
}
/**
 * @brief Метод интерпретации заголовка Expect
 *
 * @details Значение разбирается как список: клиент вправе прислать
 *          "100-continue" в любом регистре и в сопровождении других
 *          ожиданий, а также с параметрами после ";"
 *
 * @param begin начало значения заголовка
 * @param end   конец значения заголовка
 *
 */
void awh::http::Parser_HTTP::applyExpect(const char * begin, const char * end) noexcept {
	// Указатель на текущую позицию разбора
	const char * current = begin;
	/**
	 * Выполняем разбор списка ожиданий клиента
	 */
	while(current <= end){
		// Выполняем поиск разделителя списка
		const char * comma = static_cast <const char *> (::memchr(current, ',', static_cast <size_t> (end - current)));
		// Определяем конец текущего элемента списка
		const char * tokEnd = (comma != nullptr ? comma : end);
		// Указатель на начало элемента списка
		const char * tb = current;
		// Указатель на конец элемента списка
		const char * te = tokEnd;
		// Выполняем триминг OWS по краям элемента списка
		::trimOWS(tb, te);
		// Отсекаем параметры ожидания от его имени
		::trimParameters(tb, te);
		// Если получено ожидание промежуточного ответа [100 Continue]
		if(::iequalsLit(tb, static_cast <size_t> (te - tb), "100-continue"))
			// Помечаем что клиент ожидает промежуточный ответ [100 Continue]
			this->_message.flags.expectContinue = true;
		// Если разделителей больше нет - разбор списка завершён
		if(comma == nullptr)
			// Выходим из цикла
			break;
		// Переходим к следующему элементу списка
		current = (comma + 1);
	}
}
/**
 * @brief Метод интерпретации заголовка Content-Length
 *
 * @param begin начало значения заголовка
 * @param end   конец значения заголовка
 * @return      результат интерпретации
 *
 */
bool awh::http::Parser_HTTP::applyContentLength(const char * begin, const char * end) noexcept {
	// Первое полученное значение
	uint64_t first = 0;
	// Флаг получения первого значения
	bool firstSet = false;
	// Указатель на текущую позицию разбора
	const char * current = begin;
	/**
	 * Значение может быть списком "5, 5" - все элементы обязаны совпадать
	 */
	while(current <= end){
		// Выполняем поиск разделителя списка
		const char * comma = static_cast <const char *> (::memchr(current, ',', static_cast <size_t> (end - current)));
		// Определяем конец текущего элемента списка
		const char * tokEnd = (comma != nullptr ? comma : end);
		// Указатель на начало элемента списка
		const char * tb = current;
		// Указатель на конец элемента списка
		const char * te = tokEnd;
		// Выполняем триминг OWS по краям элемента списка
		::trimOWS(tb, te);
		// Значение текущего элемента списка
		uint64_t value = 0;
		// Если разбор десятичного числа не удался
		if(!::parseDecimal(tb, static_cast <size_t> (te - tb), value)){
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
		current = (comma + 1);
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
 *
 */
void awh::http::Parser_HTTP::applyTransferEncoding(const char * begin, const char * end) noexcept {
	// Помечаем что заголовок Transfer-Encoding получен
	this->_flags.transferEncodingSeen = true;
	/**
	 * Если предыдущий Transfer-Encoding уже заканчивался на chunked,
	 * любой новый Transfer-Encoding делает chunked не последним - ошибка кадрирования
	 */
	if(this->_flags.transferEncodingChunkedFinal)
		// Помечаем Transfer-Encoding некорректным
		this->_flags.transferEncodingInvalid = true;
	// Флаг того, что последнее кодирование в списке - chunked
	bool lastChunked = false;
	// Указатель на текущую позицию разбора
	const char * current = begin;
	/**
	 * Выполняем разбор списка кодирований
	 */
	while(current <= end){
		// Выполняем поиск разделителя списка
		const char * comma = static_cast <const char *> (::memchr(current, ',', static_cast <size_t> (end - current)));
		// Определяем конец текущего элемента списка
		const char * tokEnd = (comma != nullptr ? comma : end);
		// Указатель на начало элемента списка
		const char * tb = current;
		// Указатель на конец элемента списка
		const char * te = tokEnd;
		// Выполняем триминг OWS по краям элемента списка
		::trimOWS(tb, te);
		// Отсекаем параметры транспортного кодирования от его имени
		::trimParameters(tb, te);
		// Если элемент списка не пустой
		if(te > tb){
			// Проверяем является ли кодирование chunked
			const bool isChunked = ::iequalsLit(tb, static_cast <size_t> (te - tb), "chunked");
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
		current = (comma + 1);
	}
	// Запоминаем что последнее кодирование - chunked (если ошибок не выявлено)
	this->_flags.transferEncodingChunkedFinal = (lastChunked && !this->_flags.transferEncodingInvalid);
}
/**
 * @brief Метод полной очистки всех данных парсера
 *
 * @details Помимо сброса состояния разбора возвращает лимиты безопасности
 *          к значениям по умолчанию и удаляет установленные функции обратного вызова.
 *
 */
void awh::http::Parser_HTTP::clear() noexcept {
	// Выполняем сброс состояния разбора
	this->reset();
	// Возвращаем лимиты безопасности к значениям по умолчанию
	this->_limits = limits_t();
	// Удаляем все установленные функции обратного вызова
	this->_callbacks = callbacks_t();
	// Полностью сбрасываем состояние отправки (включая выходной буфер и пороги)
	this->_sender = sender_t();
	// Восстанавливаем объект логирования буфера исходящих байтов
	this->_sender.output.setLogger(this->_log);
	// Восстанавливаем объект логирования буфера передачи в сетевой слой
	this->_sender.flushing.setLogger(this->_log);
	// Восстанавливаем согласованность лимита памяти смартбуферов с порогами по умолчанию
	this->sendWaterMarks(SEND_HIGH_WATER, SEND_LOW_WATER);
}
/**
 * @brief Метод сброса парсера для разбора следующего сообщения в том же соединении
 *
 * @details Дешёвый сброс между сообщениями (keep-alive/pipelining): сохраняет лимиты
 *          безопасности и установленные функции обратного вызова, провайдер заголовков
 *          не пересоздаётся, а очищается (переиспользуется выделенная память).
 *
 */
void awh::http::Parser_HTTP::reset() noexcept {
	// Выполняем сброс состояния базового парсера (итоговый статус разбора)
	parser_t::reset();
	// Сбрасываем размер тела сообщения
	this->_message.bodySize = -1;
	// Сбрасываем код ошибки разбора
	this->_error = error_t::NONE;
	// Сбрасываем партицию текущего состояния парсера
	this->_message.part = part_t::NONE;
	// Сбрасываем фазу разбора HTTP-сообщения
	this->_message.phase = phase_t::NONE;
	// Сбрасываем флаги состояния сообщения
	this->_message.flags = message_t::flags_t();
	// Если провайдер заголовков сообщения существует
	if(this->_message.provider != nullptr){
		/**
		 * В зависимости от направления потока данных, очищаем содержимое провайдера
		 */
		switch(static_cast <uint8_t> (this->_direct)){
			// Если выполняется разбор запроса клиента
			case static_cast <uint8_t> (direct_t::REQUEST): {
				// Получаем объект провайдера заголовков запроса клиента
				request_t * provider = static_cast <request_t *> (this->_message.provider.get());
				// Очищаем параметры URI-запроса (выделенная память строки сохраняется)
				provider->uri.clear();
				// Очищаем оригинальное написание метода запроса (выделенная память строки сохраняется)
				provider->methodName.clear();
				// Сбрасываем метод запроса клиента
				provider->method = method_t::NONE;
				// Сбрасываем версию протокола
				provider->version = version_t::NONE;
			} break;
			// Если выполняется разбор ответа сервера
			case static_cast <uint8_t> (direct_t::RESPONSE): {
				// Получаем объект провайдера заголовков ответа сервера
				response_t * provider = static_cast <response_t *> (this->_message.provider.get());
				// Сбрасываем код ответа сервера
				provider->code = 0;
				// Очищаем сообщение сервера (выделенная память строки сохраняется)
				provider->message.clear();
				// Сбрасываем версию протокола
				provider->version = version_t::NONE;
			} break;
		}
	}
	// Сбрасываем состояние конечного автомата
	this->_state = static_cast <uint8_t> (state_t::S_START);
	// Сбрасываем метод запроса, которому соответствует ожидаемый ответ
	this->_method = method_t::NONE;
	// Очищаем накопитель имени текущего заголовка (выделенная память строки сохраняется)
	this->_header.name.clear();
	// Очищаем накопитель значения текущего заголовка (выделенная память строки сохраняется)
	this->_header.value.clear();
	// Очищаем накопитель расширений текущего чанка (выделенная память строки сохраняется)
	this->_header.chunkExt.clear();
	// Сбрасываем общий размер принятого тела сообщения
	this->_statsBody.bytes = 0;
	// Сбрасываем счётчик цифр
	this->_statsBody.digits = 0;
	// Сбрасываем количество разобранных заголовков
	this->_statsHeaders.count = 0;
	// Сбрасываем суммарный размер разобранных заголовков
	this->_statsHeaders.bytes = 0;
	// Сбрасываем размер текущего чанка
	this->_statsBody.chunkSize = 0;
	// Сбрасываем длину текущей стартовой строки
	this->_statsHeaders.lineBytes = 0;
	// Сбрасываем значение заголовка Content-Length
	this->_statsBody.contentLength = 0;
	// Сбрасываем остаток непрочитанных данных тела/чанка
	this->_statsBody.bytesRemaining = 0;
	// Сбрасываем длину текущей строки заголовка чанка
	this->_statsHeaders.chunkLineBytes = 0;
	// Сбрасываем флаг разбора трейлеров
	this->_flags.inTrailers = false;
	// Сбрасываем флаг получения заголовка Upgrade
	this->_flags.upgradeSeen = false;
	// Сбрасываем счётчик полученных заголовков Host
	this->_flags.hostCount = 0;
	// Сбрасываем счётчик пропущенных пустых строк перед стартовой строкой запроса
	this->_flags.leadingBlanks = 0;
	// Сбрасываем флаг наличия close в заголовке Connection
	this->_flags.connectionClose = false;
	// Сбрасываем флаг наличия upgrade в заголовке Connection
	this->_flags.connectionUpgrade = false;
	// Сбрасываем флаг получения заголовка Content-Length
	this->_flags.contentLengthSeen = false;
	// Сбрасываем флаг наличия keep-alive в заголовке Connection
	this->_flags.connectionKeepAlive = false;
	// Сбрасываем флаг получения заголовка Transfer-Encoding
	this->_flags.transferEncodingSeen = false;
	// Сбрасываем флаг некорректности заголовка Transfer-Encoding
	this->_flags.transferEncodingInvalid = false;
	// Сбрасываем флаг последнего кодирования chunked
	this->_flags.transferEncodingChunkedFinal = false;
	/**
	 * Взводим признак сброса последним действием: если сброс выполнен из функции
	 * обратного вызова, активный разбор обязан немедленно вернуть управление -
	 * продолжать его обнулённым автоматом означало бы принять середину текущего
	 * сообщения за начало следующего. Между вызовами parse() признак безвреден:
	 * разбор снимает его на входе
	 */
	this->_recycled = true;
}
/**
 * @brief Метод установки метода запроса, которому соответствует ожидаемый ответ
 *
 * @details Используется ТОЛЬКО для направления RESPONSE: парсер ответа сам не может
 *          узнать, на какой запрос пришёл ответ, а метод запроса влияет на кадрирование
 *          тела (ответ на HEAD содержит Content-Length, но тела не имеет; успешный 2xx
 *          ответ на CONNECT открывает туннель и тела не имеет).
 *          Сбрасывается в NONE при reset() - выставляйте заново перед каждым ответом
 *          в keep-alive/конвейере.
 *
 * @param method метод запроса клиента
 *
 */
void awh::http::Parser_HTTP::method(const method_t method) noexcept {
	// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
	this->_method = method;
}
/**
 * @brief Метод клонирования объекта парсера
 *
 * @details Клон получает те же направление трафика, лимиты безопасности и функции
 *          обратного вызова, но чистое состояние разбора ("фабрика с теми же настройками").
 *
 * @return копия объекта парсера
 *
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
		// Копируем метод запроса, которому соответствует ожидаемый ответ
		parser->_method = this->_method;
		// Копируем установленные функции обратного вызова
		parser->_callbacks = this->_callbacks;
		/**
		 * Копируем пороги выходного буфера через штатный метод: он же согласует
		 * лимит памяти смартбуферов с ними, а прямое присваивание полей оставило
		 * бы клону лимит по умолчанию
		 */
		parser->sendWaterMarks(this->_sender.highWater, this->_sender.lowWater);
		// Копируем объём одной прокачки pull-источника данных
		parser->_sender.pumpLimit = this->_sender.pumpLimit;
		// Перемещаем созданный объект парсера в результат
		result = ::move(parser);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод уведомления парсера о завершении потока данных (закрытии соединения)
 *
 * @details Требуется для сообщений, у которых тело кадрируется закрытием соединения:
 *          ответы HTTP/1.0 и ответы без Content-Length и без Transfer-Encoding: chunked.
 *          У таких сообщений в протоколе нет маркера конца тела - конец определяется
 *          только закрытием соединения удалённой стороной. Сетевой слой обязан вызвать
 *          этот метод, когда соединение закрыто (получен FIN/EOF сокета):
 *          - если парсер читает тело "до закрытия соединения" - сообщение помечается
 *            завершённым (status() == COMPLETE);
 *          - если парсер находится между сообщениями - ничего не происходит
 *            (нормальное закрытие keep-alive соединения);
 *          - если сообщение разобрано частично (заголовки или недочитанное тело
 *            с Content-Length) - фиксируется ошибка PREMATURE_EOF (обрыв соединения).
 *
 */
void awh::http::Parser_HTTP::eof() noexcept {
	// Если ранее зафиксирована ошибка разбора - ничего не делаем
	if(this->_error != error_t::NONE)
		// Выходим из метода
		return;
	// Если сообщение уже полностью разобрано - нормальное закрытие соединения
	if(this->_state == static_cast <uint8_t> (state_t::S_MESSAGE_DONE))
		// Выходим из метода
		return;
	// Если парсер читает тело "до закрытия соединения" - сообщение завершено
	if(this->_state == static_cast <uint8_t> (state_t::S_BODY_UNTIL_CLOSE)){
		// Уведомляем о завершении приёма тела сообщения
		if(this->firePhase(phase_t::END, part_t::BODY))
			// Завершаем разбор всего сообщения
			this->completeMessage();
		// Если разбор прерван сбросом парсера - завершённым сообщение не считается
		if(this->_recycled)
			// Устанавливаем итоговый статус разбора
			this->_status = status_t::PARTIAL;
		// Если ошибок разбора нет - сообщение полностью разобрано
		else if(this->_error == error_t::NONE)
			// Устанавливаем итоговый статус разбора
			this->_status = status_t::COMPLETE;
		// Если зафиксирована ошибка разбора - фиксируем её с записью в лог
		else this->fail(this->_error);
		// Выходим из метода
		return;
	}
	// Если парсер находится между сообщениями - нормальное закрытие keep-alive соединения
	if((this->_state == static_cast <uint8_t> (state_t::S_START)) && (this->_statsHeaders.lineBytes == 0))
		// Выходим из метода
		return;
	// Сообщение разобрано частично - фиксируем обрыв соединения с записью в лог
	this->fail(error_t::PREMATURE_EOF);
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
 *          На статусе ERROR возвращаемое значение - позиция, на которой разбор
 *          остановлен: побайтово одинаковой при разной нарезке входа она не
 *          гарантируется, одинаковы лишь статус и код ошибки.
 *
 * @param buffer буфер данных для разбора
 * @param size   размер данных для разбора
 * @return       количество обработанных байт данных
 *
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
	if(this->_state == static_cast <uint8_t> (state_t::S_MESSAGE_DONE)){
		// Устанавливаем итоговый статус разбора
		this->_status = status_t::COMPLETE;
		// Данные не обработаны
		return 0;
	}
	// Если данных для разбора нет
	if((buffer == nullptr) || (size == 0))
		// Данные не обработаны
		return 0;
	// Снимаем признак сброса: значение имеет только сброс, выполненный по ходу этого разбора
	this->_recycled = false;
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
			/**
			 * Если парсер сброшен функцией обратного вызова - разбор прерывается
			 *
			 * Проверка стоит в начале цикла, а не в его конце: крупноблочные пути -
			 * тело, данные чанка, целая строка заголовка - возвращаются к началу
			 * через continue и хвоста цикла не проходят. Расставлять проверку по
			 * каждому такому пути значило бы держать инвариант на полноте перечня,
			 * а пропуск одного пути молча возвращает рассинхронизацию потока
			 */
			if(this->_recycled){
				// Устанавливаем итоговый статус разбора
				this->_status = status_t::PARTIAL;
				// Выводим количество обработанных байт данных
				return i;
			}
			/**
			 * Крупноблочная обработка: тело копируется блоками, а токены сканируются
			 * по lookup-таблице до первого недопустимого символа (переносимый "SIMD").
			 * Класс состояния определяется одним чтением таблицы - на посимвольном
			 * пути это одна проверка вместо цепочки сравнений с каждым из состояний
			 */
			switch(::scanTable[this->_state]){
				// Крупноблочное чтение тела фиксированного размера (Content-Length)
				case static_cast <uint8_t> (scan_t::SCAN_BODY_IDENTITY): {
					// Определяем количество доступных байт данных
					const uint64_t avail = static_cast <uint64_t> (size - i);
					// Определяем сколько байт данных можно забрать
					const uint64_t take = (avail < this->_statsBody.bytesRemaining ? avail : this->_statsBody.bytesRemaining);
					// Если данные для передачи есть
					if(take > 0){
						// Если функция обратного вызова установлена и потребовала прервать разбор
						if((this->_callbacks.data != nullptr) &&
						   !this->_callbacks.data(STREAM_ID, data + i, static_cast <size_t> (take), (take == this->_statsBody.bytesRemaining))){
							// Фиксируем ошибку прерывания разбора с записью в лог
							this->fail(error_t::ABORTED);
							// Выводим количество обработанных байт данных
							return i;
						}
						/**
						 * Если функция обратного вызова сбросила парсер - разбор прерывается
						 * до учёта отданных байт: счётчики уже обнулены сбросом, и вычитание
						 * из них отданной порции увело бы остаток тела в переполнение
						 */
						if(this->_recycled){
							// Устанавливаем итоговый статус разбора
							this->_status = status_t::PARTIAL;
							// Выводим количество обработанных байт данных вместе с отданной порцией
							return (i + static_cast <size_t> (take));
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
						// Если разбор прерван сбросом парсера - завершённым сообщение не считается
						if(this->_recycled)
							// Устанавливаем итоговый статус разбора
							this->_status = status_t::PARTIAL;
						// Если ошибок разбора нет - сообщение полностью разобрано
						else if(this->_error == error_t::NONE)
							// Устанавливаем итоговый статус разбора
							this->_status = status_t::COMPLETE;
						// Если зафиксирована ошибка разбора - фиксируем её с записью в лог
						else this->fail(this->_error);
						// Выводим количество обработанных байт данных
						return i;
				}
					// Продолжаем разбор
					continue;
				}
				// Крупноблочное чтение тела до закрытия соединения
				case static_cast <uint8_t> (scan_t::SCAN_BODY_CLOSE): {
					// Определяем количество доступных байт данных
					const size_t avail = (size - i);
					// Если общий размер тела превышает лимит
					if((this->_statsBody.bytes + avail) > this->_limits.maxBodySize){
						// Фиксируем ошибку превышения размера тела с записью в лог
						this->fail(error_t::BODY_OVERFLOW);
						// Выводим количество обработанных байт данных
						return i;
				}
					// Если данные для передачи есть
					if(avail > 0){
						/**
						 * Если функция обратного вызова установлена и потребовала прервать разбор
						 * (конец тела "до закрытия соединения" неизвестен - endStream всегда false)
						 */
						if((this->_callbacks.data != nullptr) && !this->_callbacks.data(STREAM_ID, data + i, avail, false)){
							// Фиксируем ошибку прерывания разбора с записью в лог
							this->fail(error_t::ABORTED);
							// Выводим количество обработанных байт данных
							return i;
						}
						/**
						 * Если функция обратного вызова сбросила парсер - разбор прерывается
						 * до учёта отданных байт: счётчики уже обнулены сбросом, и вычитание
						 * из них отданной порции увело бы остаток тела в переполнение
						 */
						if(this->_recycled){
							// Устанавливаем итоговый статус разбора
							this->_status = status_t::PARTIAL;
							// Выводим количество обработанных байт данных вместе с отданной порцией
							return (i + static_cast <size_t> (avail));
						}
						// Наращиваем общий размер принятого тела сообщения
						this->_statsBody.bytes += avail;
				}
					// Смещаем позицию разбора
					i += avail;
					// Продолжаем разбор (завершение - только по вызову eof)
					continue;
				}
				// Крупноблочное чтение данных чанка
				case static_cast <uint8_t> (scan_t::SCAN_BODY_CHUNK): {
					// Определяем количество доступных байт данных
					const uint64_t avail = static_cast <uint64_t> (size - i);
					// Определяем сколько байт данных можно забрать
					const uint64_t take = (avail < this->_statsBody.bytesRemaining ? avail : this->_statsBody.bytesRemaining);
					// Если общий размер тела превышает лимит
					if((this->_statsBody.bytes + take) > this->_limits.maxBodySize){
						// Фиксируем ошибку превышения размера тела с записью в лог
						this->fail(error_t::BODY_OVERFLOW);
						// Выводим количество обработанных байт данных
						return i;
				}
					// Если данные для передачи есть
					if(take > 0){
						/**
						 * Если функция обратного вызова установлена и потребовала прервать разбор
						 * (конец тела chunked в момент фрагмента неизвестен - endStream всегда false)
						 */
						if((this->_callbacks.data != nullptr) && !this->_callbacks.data(STREAM_ID, data + i, static_cast <size_t> (take), false)){
							// Фиксируем ошибку прерывания разбора с записью в лог
							this->fail(error_t::ABORTED);
							// Выводим количество обработанных байт данных
							return i;
						}
						/**
						 * Если функция обратного вызова сбросила парсер - разбор прерывается
						 * до учёта отданных байт: счётчики уже обнулены сбросом, и вычитание
						 * из них отданной порции увело бы остаток тела в переполнение
						 */
						if(this->_recycled){
							// Устанавливаем итоговый статус разбора
							this->_status = status_t::PARTIAL;
							// Выводим количество обработанных байт данных вместе с отданной порцией
							return (i + static_cast <size_t> (take));
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
						this->_state = static_cast <uint8_t> (state_t::S_CHUNK_DATA_ALMOST_DONE);
					// Продолжаем разбор
					continue;
				}
				// Разбор целой строки заголовка или трейлера
				case static_cast <uint8_t> (scan_t::SCAN_HEADER_LINE): {
					// Выполняем разбор целой строки заголовка из входного буфера
					const size_t consumed = this->parseHeaderLine(data + i, (size - i));
					// Если быстрый путь неприменим - строку разбирает посимвольный путь
					if(consumed == 0)
						// Выходим из ветки крупноблочной обработки
						break;
					// Если разбор прерван потребителем либо превышен лимит
					if(this->_error != error_t::NONE){
						// Фиксируем ошибку разбора с записью в лог
						this->fail(this->_error);
						// Выводим количество обработанных байт данных
						return i;
					}
					// Смещаем позицию разбора за разобранную строку заголовка
					i += consumed;
					// Продолжаем разбор
					continue;
				}
				// Разбор целой стартовой строки ответа сервера
				case static_cast <uint8_t> (scan_t::SCAN_STATUS): {
					// Выполняем разбор целой стартовой строки ответа из входного буфера
					const size_t consumed = this->parseStatusLine(data + i, (size - i));
					// Если быстрый путь неприменим - строку разбирает посимвольный путь
					if(consumed == 0)
						// Выходим из ветки крупноблочной обработки
						break;
					// Смещаем позицию разбора за разобранную стартовую строку
					i += consumed;
					// Продолжаем разбор
					continue;
				}
				// Сканирование непрерывного участка метода запроса
				case static_cast <uint8_t> (scan_t::SCAN_METHOD): {
					// Позиция конца непрерывного участка допустимых символов
					size_t j = i;
					/**
					 * Сканируем непрерывный участок символов токена
					 */
					while((j < size) && ::isToken(static_cast <uint8_t> (data[j])))
						// Смещаем позицию конца участка
						++j;
					// Определяем размер непрерывного участка
					const size_t run = (j - i);
					/**
					 * Быстрый путь: метод присутствует во входном буфере целиком и
					 * завершён разделителем. Классификация выполняется представлением
					 * прямо во входные данные, и накопитель при распознанном методе
					 * не задействуется вовсе
					 */
					if((run > 0) && this->_header.name.empty() && (j < size) && (data[j] == ' ')){
						// Если длина стартовой строки превышает лимит (разделитель тоже учитывается)
						if((this->_statsHeaders.lineBytes + run + 1) > this->_limits.maxRequestLine){
							// Фиксируем ошибку превышения длины request-line с записью в лог
							this->fail(error_t::URL_OVERFLOW);
							// Выводим количество обработанных байт данных
							return i;
						}
						// Наращиваем длину текущей стартовой строки
						this->_statsHeaders.lineBytes += (run + 1);
						// Выполняем классификацию метода запроса по его имени
						req->method = ::classifyMethod(string_view(data + i, run));
						// Если метод запроса синтаксически корректен, но не распознан
						if(req->method == method_t::NONE){
							// Помечаем метод запроса как нераспознанный
							req->method = method_t::UNKNOWN;
							// Сохраняем оригинальное написание метода (прозрачное проксирование экзотических методов)
							req->methodName.assign(data + i, run);
						}
						// Переходим к пропуску пробелов перед request-target
						this->_state = static_cast <uint8_t> (state_t::S_REQ_TARGET_START);
						// Смещаем позицию разбора за разделитель
						i = (j + 1);
						// Продолжаем разбор
						continue;
					}
					/**
					 * Медленный путь: метод разорван между фрагментами - копим его в
					 * накопитель, а решение о завершении принимает посимвольный путь
					 */
					if(run > 0){
						// Если длина стартовой строки превышает лимит
						if((this->_statsHeaders.lineBytes + run) > this->_limits.maxRequestLine){
							// Фиксируем ошибку превышения длины request-line с записью в лог
							this->fail(error_t::URL_OVERFLOW);
							// Выводим количество обработанных байт данных
							return i;
						}
						// Наращиваем длину текущей стартовой строки
						this->_statsHeaders.lineBytes += run;
						// Добавляем непрерывный участок к накопителю имени метода
						this->_header.name.append(data + i, run);
						// Смещаем позицию разбора
						i = j;
						// Продолжаем разбор
						continue;
					}
				} break;
				// Разбор литерала версии протокола запроса
				case static_cast <uint8_t> (scan_t::SCAN_VERSION): {
					/**
					 * Быстрый путь: литерал версии присутствует во входном буфере целиком
					 * вместе с окончанием строки. Восемь его октетов разбираются иначе
					 * восемью отдельными состояниями конечного автомата, тогда как
					 * допустимых написаний всего два и оба сравниваются одним вызовом
					 */
					if((size - i) >= VERSION_LINE){
						// Версия протокола, распознанная по литералу стартовой строки
						version_t version = version_t::NONE;
						// Если литерал соответствует версии протокола HTTP/1.1
						if(::memcmp(data + i, "HTTP/1.1\r\n", VERSION_LINE) == 0)
							// Устанавливаем версию протокола HTTP/1.1
							version = version_t::HTTP1_1;
						// Если литерал соответствует версии протокола HTTP/1.0
						else if(::memcmp(data + i, "HTTP/1.0\r\n", VERSION_LINE) == 0)
							// Устанавливаем версию протокола HTTP/1.0
							version = version_t::HTTP1_0;
						/**
						 * Любое отклонение от двух допустимых написаний передаётся
						 * посимвольному пути: и толерантность к голому LF, и лишние
						 * пробелы перед литералом, и все ошибки разбираются там
						 */
						if(version != version_t::NONE){
							/**
							 * Разобранная версия протокола устанавливается до проверки лимита:
							 * посимвольный путь записывает её на минорной цифре и только затем
							 * учитывает литерал в длине стартовой строки, поэтому при отказе
							 * по лимиту версия у него уже установлена. Состояние после отказа
							 * обязано совпадать у обоих путей
							 */
							req->version = version;
							/**
							 * Учитываем в длине стартовой строки литерал версии протокола
							 * целиком, как это делает посимвольный путь: окончание строки
							 * в длине стартовой строки не учитывается
							 */
							if((this->_statsHeaders.lineBytes + (VERSION_LINE - 2)) > this->_limits.maxRequestLine){
								// Устанавливаем длину стартовой строки превысившей лимит
								this->_statsHeaders.lineBytes += (VERSION_LINE - 2);
								// Фиксируем ошибку превышения длины request-line с записью в лог
								this->fail(error_t::URL_OVERFLOW);
								// Выводим количество обработанных байт данных до минорной цифры версии
								return (i + (VERSION_LINE - 3));
							}
							// Наращиваем длину текущей стартовой строки
							this->_statsHeaders.lineBytes += (VERSION_LINE - 2);
							// Завершаем разбор стартовой строки
							this->commitStartLine();
							// Смещаем позицию разбора за литерал версии и окончание строки
							i += VERSION_LINE;
							// Продолжаем разбор
							continue;
						}
					}
				} break;
				// Сканирование непрерывного участка request-target
				case static_cast <uint8_t> (scan_t::SCAN_TARGET): {
					// Позиция конца непрерывного участка допустимых символов
					size_t j = i;
					/**
					 * Сканируем непрерывный участок допустимых символов request-target
					 * (URI-запроса) по lookup-таблице
					 */
					while((j < size) && ::isTargetCh(static_cast <uint8_t> (data[j])))
						// Смещаем позицию конца участка
						++j;
					// Если непрерывный участок найден
					if(j > i){
						// Определяем размер непрерывного участка
						const size_t run = (j - i);
						// Если длина стартовой строки превышает лимит
						if((this->_statsHeaders.lineBytes + run) > this->_limits.maxRequestLine){
							/**
							 * Дописываем ту часть участка, которая укладывается в лимит:
							 * посимвольный путь проверяет лимит после записи каждого октета
							 * и оставляет URI-адрес заполненным ровно до предела. Состояние
							 * после отказа обязано совпадать у обоих путей, иначе содержимое
							 * отвергнутого сообщения зависело бы от разбиения входа
							 */
							const size_t allowed = ((this->_statsHeaders.lineBytes < this->_limits.maxRequestLine)
							 ? (this->_limits.maxRequestLine - this->_statsHeaders.lineBytes) : 0);
							// Добавляем укладывающуюся в лимит часть участка к параметрам URI-запроса
							req->uri.append(data + i, allowed);
							// Устанавливаем длину стартовой строки превысившей лимит
							this->_statsHeaders.lineBytes = (this->_limits.maxRequestLine + 1);
							// Фиксируем ошибку превышения длины request-line с записью в лог
							this->fail(error_t::URL_OVERFLOW);
							// Выводим количество обработанных байт данных до недопустимого октета
							return (i + allowed);
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
				} break;
				// Сканирование непрерывного участка имени заголовка или трейлера
				case static_cast <uint8_t> (scan_t::SCAN_TOKEN): {
					// Позиция конца непрерывного участка допустимых символов
					size_t j = i;
					/**
					 * Сканируем непрерывный участок символов токена
					 */
					while((j < size) && ::isToken(static_cast <uint8_t> (data[j])))
						// Смещаем позицию конца участка
						++j;
					// Если непрерывный участок найден
					if(j > i){
						// Определяем размер непрерывного участка
						const size_t run = (j - i);
						// Если длина имени заголовка превышает лимит
						if((this->_header.name.size() + run) > this->_limits.maxHeaderName){
							// Фиксируем ошибку превышения размера заголовков с записью в лог
							this->fail(error_t::HEADER_OVERFLOW);
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
				} break;
				// Сканирование непрерывного участка значения заголовка или трейлера
				case static_cast <uint8_t> (scan_t::SCAN_VALUE): {
					// Позиция конца непрерывного участка допустимых символов
					size_t j = i;
					/**
					 * Сканируем непрерывный участок допустимых символов значения заголовка
					 */
					while((j < size) && ::isValueCh(static_cast <uint8_t> (data[j])))
						// Смещаем позицию конца участка
						++j;
					// Если непрерывный участок найден
					if(j > i){
						// Определяем размер непрерывного участка
						const size_t run = (j - i);
						// Если длина значения заголовка превышает лимит
						if((this->_header.value.size() + run) > this->_limits.maxHeaderValue){
							// Фиксируем ошибку превышения размера заголовков с записью в лог
							this->fail(error_t::HEADER_OVERFLOW);
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
				} break;
			}
			// Получаем текущий байт данных
			const uint8_t ch = static_cast <uint8_t> (data[i]);
			/**
			 * Посимвольные состояния конечного автомата
			 */
			switch(this->_state){
				// Общий старт (диспетчеризация по направлению трафика)
				case static_cast <uint8_t> (state_t::S_START): {
					/**
					 * Пустые строки перед стартовой строкой запроса пропускаются
					 *
					 * RFC 9112 §2.2 требует от сервера игнорировать хотя бы одну пустую
					 * строку перед request-line: устаревшие клиенты дописывают лишний CRLF
					 * после тела, и без пропуска соединение keep-alive обрывалось бы на
					 * ровном месте. Правило адресовано именно серверу, поэтому к ответам
					 * не применяется - там ведущий CRLF остаётся ошибкой версии.
					 *
					 * Пропуск выполняется до уведомления о начале сообщения: иначе
					 * потребитель получил бы фазу начала того, что ещё не началось.
					 * Пропущенные октеты не входят в бюджет стартовой строки - его
					 * нулевое значение служит признаком "между сообщениями" при закрытии
					 * соединения, и закрытие после случайного CRLF стало бы обрывом.
					 * Число пропускаемых октетов ограничено: без предела поток пустых
					 * строк удерживал бы соединение без всякого продвижения
					 */
					if((this->_direct == direct_t::REQUEST) && ((ch == '\r') || (ch == '\n'))){
						/**
						 * Пустая строка перед запросом принимается только в толерантном
						 * режиме: расхождение в её трактовке с соседним звеном цепочки
						 * смещает границы сообщений в конвейере - тот же вектор
						 * рассинхронизации, что и у одиночного LF
						 */
						if(this->_limits.strictEOL){
							// Фиксируем ошибку окончания строки
							this->_error = error_t::INVALID_EOL;
							// Выходим из состояния
							break;
						}
						// Если число пропускаемых октетов превысило предел
						if(this->_flags.leadingBlanks >= MAX_LEADING_BLANKS){
							// Фиксируем ошибку некорректного метода
							this->_error = error_t::INVALID_METHOD;
							// Выходим из состояния
							break;
						}
						// Учитываем пропущенный октет пустой строки
						++this->_flags.leadingBlanks;
						// Выходим из состояния, потребив октет и оставшись в исходном состоянии
						break;
					}
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
						if(!::isToken(ch)){
							// Фиксируем ошибку некорректного метода
							this->_error = error_t::INVALID_METHOD;
							// Выходим из состояния
							break;
						}
						// Начинаем отсчёт длины стартовой строки
						this->_statsHeaders.lineBytes = 0;
						// Переходим к разбору метода запроса
						this->_state = static_cast <uint8_t> (state_t::S_REQ_METHOD);
						/**
						 * Текущий байт намеренно не потребляется: метод разбирается
						 * крупноблочно и должен получить свой первый символ сам, иначе
						 * накопитель оказался бы непустым и быстрый путь не сработал
						 */
						continue;
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
						this->_statsHeaders.lineBytes = 0;
						// Переходим к разбору стартовой строки ответа
						this->_state = static_cast <uint8_t> (state_t::S_RES_STATUS_LINE);
						/**
						 * Текущий байт намеренно не потребляется: стартовая строка ответа
						 * разбирается крупноблочно и должна получить свой первый октет сама,
						 * иначе быстрый путь увидел бы строку без литерала версии
						 */
						continue;
					}
				} break;
				/**
				 * Стартовая строка запроса клиента (request-line)
				 */
				// Разбор метода запроса
				case static_cast <uint8_t> (state_t::S_REQ_METHOD): {
					// Если получен пробел - метод разобран полностью
					if(ch == ' '){
						// Если длина стартовой строки превышает лимит (разделитель тоже учитывается)
						if(++this->_statsHeaders.lineBytes > this->_limits.maxRequestLine){
							// Фиксируем ошибку превышения длины request-line
							this->_error = error_t::URL_OVERFLOW;
							// Выходим из состояния
							break;
						}
						// Выполняем классификацию метода запроса по его имени
						req->method = ::classifyMethod(this->_header.name);
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
						this->_state = static_cast <uint8_t> (state_t::S_REQ_TARGET_START);
					// Если получен символ токена
					} else if(::isToken(ch)) {
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
				case static_cast <uint8_t> (state_t::S_REQ_TARGET_START): {
					// Толерантно пропускаем лишние пробелы
					if(ch == ' '){
						/**
						 * Лишние пробелы внутри стартовой строки принимаются только в толерантном
						 * режиме: расхождение в их трактовке с соседним звеном цепочки позволяет
						 * протащить через фильтрующий узел не тот запрос, который увидит бэкенд
						 */
						if(this->_limits.strictSpaces){
							// Фиксируем ошибку лишнего пробела в стартовой строке
							this->_error = error_t::INVALID_TARGET;
							// Выходим из состояния
							break;
						}
						// Если длина стартовой строки превышает лимит (пробелы тоже учитываются)
						if(++this->_statsHeaders.lineBytes > this->_limits.maxRequestLine)
							// Фиксируем ошибку превышения длины request-line
							this->_error = error_t::URL_OVERFLOW;
						// Выходим из состояния
						break;
					}
					// Если символ недопустим в request-target
					if(!::isTargetCh(ch)){
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
					this->_state = static_cast <uint8_t> (state_t::S_REQ_TARGET);
				} break;
				// Разбор request-target
				case static_cast <uint8_t> (state_t::S_REQ_TARGET): {
					// Если получен пробел - request-target разобран полностью
					if(ch == ' '){
						// Если длина стартовой строки превышает лимит (разделитель тоже учитывается)
						if(++this->_statsHeaders.lineBytes > this->_limits.maxRequestLine){
							// Фиксируем ошибку превышения длины request-line
							this->_error = error_t::URL_OVERFLOW;
							// Выходим из состояния
							break;
						}
						// Переходим к пропуску пробелов перед "HTTP/"
						this->_state = static_cast <uint8_t> (state_t::S_REQ_HTTP_START);
						// Выходим из состояния
						break;
					}
					// Если символ недопустим в request-target
					if(!::isTargetCh(ch)){
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
				case static_cast <uint8_t> (state_t::S_REQ_HTTP_START): {
					// Толерантно пропускаем лишние пробелы
					if(ch == ' '){
						/**
						 * Лишние пробелы внутри стартовой строки принимаются только в толерантном
						 * режиме: расхождение в их трактовке с соседним звеном цепочки позволяет
						 * протащить через фильтрующий узел не тот запрос, который увидит бэкенд
						 */
						if(this->_limits.strictSpaces){
							// Фиксируем ошибку лишнего пробела в стартовой строке
							this->_error = error_t::INVALID_VERSION;
							// Выходим из состояния
							break;
						}
						// Если длина стартовой строки превышает лимит (пробелы тоже учитываются)
						if(++this->_statsHeaders.lineBytes > this->_limits.maxRequestLine)
							// Фиксируем ошибку превышения длины request-line
							this->_error = error_t::URL_OVERFLOW;
						// Выходим из состояния
						break;
					}
					// Если получен не литерал "H"
					if(ch != 'H'){
						// Фиксируем ошибку некорректной версии протокола
						this->_error = error_t::INVALID_VERSION;
						// Выходим из состояния
						break;
					}
					// Переходим к разбору литерала "HTTP/"
					this->_state = static_cast <uint8_t> (state_t::S_REQ_HTTP_H);
				} break;
				// Разбор литерала "HT"
				case static_cast <uint8_t> (state_t::S_REQ_HTTP_H): {
					// Если получен не литерал "T"
					if(ch != 'T'){
						// Фиксируем ошибку некорректной версии протокола
						this->_error = error_t::INVALID_VERSION;
						// Выходим из состояния
						break;
					}
					// Переходим к следующему литералу
					this->_state = static_cast <uint8_t> (state_t::S_REQ_HTTP_HT);
				} break;
				// Разбор литерала "HTT"
				case static_cast <uint8_t> (state_t::S_REQ_HTTP_HT): {
					// Если получен не литерал "T"
					if(ch != 'T'){
						// Фиксируем ошибку некорректной версии протокола
						this->_error = error_t::INVALID_VERSION;
						// Выходим из состояния
						break;
					}
					// Переходим к следующему литералу
					this->_state = static_cast <uint8_t> (state_t::S_REQ_HTTP_HTT);
				} break;
				// Разбор литерала "HTTP"
				case static_cast <uint8_t> (state_t::S_REQ_HTTP_HTT): {
					// Если получен не литерал "P"
					if(ch != 'P'){
						// Фиксируем ошибку некорректной версии протокола
						this->_error = error_t::INVALID_VERSION;
						// Выходим из состояния
						break;
					}
					// Переходим к следующему литералу
					this->_state = static_cast <uint8_t> (state_t::S_REQ_HTTP_HTTP);
				} break;
				// Разбор литерала "HTTP/"
				case static_cast <uint8_t> (state_t::S_REQ_HTTP_HTTP): {
					// Если получен не литерал "/"
					if(ch != '/'){
						// Фиксируем ошибку некорректной версии протокола
						this->_error = error_t::INVALID_VERSION;
						// Выходим из состояния
						break;
					}
					// Переходим к разбору мажорной цифры версии
					this->_state = static_cast <uint8_t> (state_t::S_REQ_HTTP_SLASH);
				} break;
				// Разбор мажорной цифры версии (принимается только HTTP/1.x)
				case static_cast <uint8_t> (state_t::S_REQ_HTTP_SLASH): {
					// Если мажорная цифра версии не равна 1
					if(ch != '1'){
						// Фиксируем ошибку некорректной версии протокола
						this->_error = error_t::INVALID_VERSION;
						// Выходим из состояния
						break;
					}
					// Переходим к разбору точки между major и minor
					this->_state = static_cast <uint8_t> (state_t::S_REQ_HTTP_DOT);
				} break;
				// Разбор точки между major и minor
				case static_cast <uint8_t> (state_t::S_REQ_HTTP_DOT): {
					// Если получена не точка
					if(ch != '.'){
						// Фиксируем ошибку некорректной версии протокола
						this->_error = error_t::INVALID_VERSION;
						// Выходим из состояния
						break;
					}
					// Переходим к разбору минорной цифры версии
					this->_state = static_cast <uint8_t> (state_t::S_REQ_HTTP_MINOR);
				} break;
				// Разбор минорной цифры версии (принимаются только HTTP/1.0 и HTTP/1.1)
				case static_cast <uint8_t> (state_t::S_REQ_HTTP_MINOR): {
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
					/**
					 * Учитываем в длине стартовой строки литерал версии протокола целиком:
					 * восемь его байт разбираются отдельными состояниями и иначе выпали бы
					 * из-под лимита
					 */
					this->_statsHeaders.lineBytes += 8;
					// Если длина стартовой строки превышает лимит
					if(this->_statsHeaders.lineBytes > this->_limits.maxRequestLine){
						// Фиксируем ошибку превышения длины request-line
						this->_error = error_t::URL_OVERFLOW;
						// Выходим из состояния
						break;
					}
					// Переходим к ожиданию CR/LF после версии
					this->_state = static_cast <uint8_t> (state_t::S_REQ_LINE_ALMOST_DONE);
				} break;
				// Ожидание CR/LF после версии
				case static_cast <uint8_t> (state_t::S_REQ_LINE_ALMOST_DONE): {
					// Если получен CR - ожидаем LF
					if(ch == '\r'){
						// Переходим к ожиданию LF
						this->_state = static_cast <uint8_t> (state_t::S_REQ_LINE_LF);
						// Выходим из состояния
						break;
					}
					// Если получен LF - стартовая строка разобрана полностью (толерантность к голому LF)
					if(ch == '\n'){
						/**
						 * Одиночный LF в роли окончания строки принимается только в толерантном
						 * режиме: расхождение в его трактовке с соседним звеном цепочки -
						 * классический вектор рассинхронизации кадрирования (request smuggling)
						 */
						if(this->_limits.strictEOL){
							// Фиксируем ошибку окончания строки
							this->_error = error_t::INVALID_EOL;
							// Выходим из состояния
							break;
						}
						// Завершаем разбор стартовой строки
						this->commitStartLine();
						// Выходим из состояния
						break;
					}
					// Любой другой символ после версии недопустим
					this->_error = error_t::INVALID_VERSION;
				} break;
				// Ожидание LF после CR
				case static_cast <uint8_t> (state_t::S_REQ_LINE_LF): {
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
				/**
				 * Точка входа быстрого пути стартовой строки ответа
				 *
				 * Сюда управление доходит только тогда, когда крупноблочный путь
				 * отказался разбирать строку целиком: октет потребляется и разбор
				 * передаётся прежней посимвольной цепочке состояний литерала версии
				 */
				case static_cast <uint8_t> (state_t::S_RES_STATUS_LINE): {
					/**
					 * Литерал "H" проверен начальным состоянием, которое передаёт сюда
					 * управление, не потребляя октет. Проверка повторяется потому, что
					 * инвариант держится на трёх разнесённых участках кода - начальном
					 * состоянии, крупноблочной диспетчеризации и этой ветке, - а его
					 * нарушение проявилось бы не ошибкой разбора, а молчаливым приёмом
					 * чужого литерала версии
					 */
					if(ch != 'H'){
						// Фиксируем ошибку некорректной версии протокола
						this->_error = error_t::INVALID_VERSION;
						// Выходим из состояния
						break;
					}
					// Начинаем отсчёт длины стартовой строки
					this->_statsHeaders.lineBytes = 1;
					// Переходим к разбору литерала "HTTP/"
					this->_state = static_cast <uint8_t> (state_t::S_RES_HTTP_H);
				} break;
				// Разбор литерала "HT"
				case static_cast <uint8_t> (state_t::S_RES_HTTP_H): {
					// Если получен не литерал "T"
					if(ch != 'T'){
						// Фиксируем ошибку некорректной версии протокола
						this->_error = error_t::INVALID_VERSION;
						// Выходим из состояния
						break;
					}
					// Переходим к следующему литералу
					this->_state = static_cast <uint8_t> (state_t::S_RES_HTTP_HT);
				} break;
				// Разбор литерала "HTT"
				case static_cast <uint8_t> (state_t::S_RES_HTTP_HT): {
					// Если получен не литерал "T"
					if(ch != 'T'){
						// Фиксируем ошибку некорректной версии протокола
						this->_error = error_t::INVALID_VERSION;
						// Выходим из состояния
						break;
					}
					// Переходим к следующему литералу
					this->_state = static_cast <uint8_t> (state_t::S_RES_HTTP_HTT);
				} break;
				// Разбор литерала "HTTP"
				case static_cast <uint8_t> (state_t::S_RES_HTTP_HTT): {
					// Если получен не литерал "P"
					if(ch != 'P'){
						// Фиксируем ошибку некорректной версии протокола
						this->_error = error_t::INVALID_VERSION;
						// Выходим из состояния
						break;
					}
					// Переходим к следующему литералу
					this->_state = static_cast <uint8_t> (state_t::S_RES_HTTP_HTTP);
				} break;
				// Разбор литерала "HTTP/"
				case static_cast <uint8_t> (state_t::S_RES_HTTP_HTTP): {
					// Если получен не литерал "/"
					if(ch != '/'){
						// Фиксируем ошибку некорректной версии протокола
						this->_error = error_t::INVALID_VERSION;
						// Выходим из состояния
						break;
					}
					// Переходим к разбору мажорной цифры версии
					this->_state = static_cast <uint8_t> (state_t::S_RES_HTTP_SLASH);
				} break;
				// Разбор мажорной цифры версии (принимается только HTTP/1.x)
				case static_cast <uint8_t> (state_t::S_RES_HTTP_SLASH): {
					// Если мажорная цифра версии не равна 1
					if(ch != '1'){
						// Фиксируем ошибку некорректной версии протокола
						this->_error = error_t::INVALID_VERSION;
						// Выходим из состояния
						break;
					}
					// Переходим к разбору точки между major и minor
					this->_state = static_cast <uint8_t> (state_t::S_RES_HTTP_DOT);
				} break;
				// Разбор точки между major и minor
				case static_cast <uint8_t> (state_t::S_RES_HTTP_DOT): {
					// Если получена не точка
					if(ch != '.'){
						// Фиксируем ошибку некорректной версии протокола
						this->_error = error_t::INVALID_VERSION;
						// Выходим из состояния
						break;
					}
					// Переходим к разбору минорной цифры версии
					this->_state = static_cast <uint8_t> (state_t::S_RES_HTTP_MINOR);
				} break;
				// Разбор минорной цифры версии (принимаются только HTTP/1.0 и HTTP/1.1)
				case static_cast <uint8_t> (state_t::S_RES_HTTP_MINOR): {
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
					/**
					 * Учитываем в длине стартовой строки литерал версии протокола:
					 * первый его байт уже учтён стартовым состоянием, остальные семь
					 * разбираются отдельными состояниями и иначе выпали бы из-под лимита
					 */
					this->_statsHeaders.lineBytes += 7;
					// Если длина стартовой строки превышает лимит
					if(this->_statsHeaders.lineBytes > this->_limits.maxRequestLine){
						// Фиксируем ошибку превышения длины стартовой строки
						this->_error = error_t::URL_OVERFLOW;
						// Выходим из состояния
						break;
					}
					// Переходим к обязательному пробелу после версии
					this->_state = static_cast <uint8_t> (state_t::S_RES_FIRST_SPACE);
				} break;
				// Обязательный пробел после версии (RFC 7230 §3.1.2)
				case static_cast <uint8_t> (state_t::S_RES_FIRST_SPACE): {
					// Если получен не пробел
					if(ch != ' '){
						// Фиксируем ошибку некорректной версии протокола
						this->_error = error_t::INVALID_VERSION;
						// Выходим из состояния
						break;
					}
					// Если длина стартовой строки превышает лимит (обязательный пробел тоже занимает канал)
					if(++this->_statsHeaders.lineBytes > this->_limits.maxRequestLine){
						// Фиксируем ошибку превышения длины стартовой строки
						this->_error = error_t::URL_OVERFLOW;
						// Выходим из состояния
						break;
					}
					// Переходим к пропуску пробелов перед статус-кодом
					this->_state = static_cast <uint8_t> (state_t::S_RES_STATUS_START);
				} break;
				// Пропуск пробелов перед статус-кодом
				case static_cast <uint8_t> (state_t::S_RES_STATUS_START): {
					// Толерантно пропускаем дополнительные пробелы перед кодом
					if(ch == ' '){
						/**
						 * Лишние пробелы внутри стартовой строки принимаются только в толерантном
						 * режиме: расхождение в их трактовке с соседним звеном цепочки позволяет
						 * протащить через фильтрующий узел не тот запрос, который увидит бэкенд
						 */
						if(this->_limits.strictSpaces){
							// Фиксируем ошибку лишнего пробела в стартовой строке
							this->_error = error_t::INVALID_STATUS;
							// Выходим из состояния
							break;
						}
						// Если длина стартовой строки превышает лимит (пробелы тоже учитываются)
						if(++this->_statsHeaders.lineBytes > this->_limits.maxRequestLine)
							// Фиксируем ошибку превышения длины стартовой строки
							this->_error = error_t::URL_OVERFLOW;
						// Выходим из состояния
						break;
					}
					// Если символ не является десятичной цифрой
					if(!::isDigit(ch)){
						// Фиксируем ошибку некорректного статус-кода
						this->_error = error_t::INVALID_STATUS;
						// Выходим из состояния
						break;
					}
					// Если длина стартовой строки превышает лимит (цифры кода тоже занимают канал)
					if(++this->_statsHeaders.lineBytes > this->_limits.maxRequestLine){
						// Фиксируем ошибку превышения длины стартовой строки
						this->_error = error_t::URL_OVERFLOW;
						// Выходим из состояния
						break;
					}
					// Устанавливаем первую цифру статус-кода
					res->code = static_cast <uint16_t> (ch - '0');
					// Начинаем отсчёт цифр статус-кода
					this->_statsBody.digits = 1;
					// Переходим к разбору статус-кода
					this->_state = static_cast <uint8_t> (state_t::S_RES_STATUS_CODE);
				} break;
				// Разбор статус-кода
				case static_cast <uint8_t> (state_t::S_RES_STATUS_CODE): {
					// Если получена десятичная цифра
					if(::isDigit(ch)){
						// Если статус-код уже содержит три цифры
						if(this->_statsBody.digits >= 3){
							// Фиксируем ошибку некорректного статус-кода
							this->_error = error_t::INVALID_STATUS;
							// Выходим из состояния
							break;
						}
						// Если длина стартовой строки превышает лимит (цифры кода тоже занимают канал)
						if(++this->_statsHeaders.lineBytes > this->_limits.maxRequestLine){
							// Фиксируем ошибку превышения длины стартовой строки
							this->_error = error_t::URL_OVERFLOW;
							// Выходим из состояния
							break;
						}
						// Добавляем цифру к статус-коду
						res->code = static_cast <uint16_t> ((res->code * 10) + (ch - '0'));
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
						// Если длина стартовой строки превышает лимит (разделитель тоже занимает канал)
						if(++this->_statsHeaders.lineBytes > this->_limits.maxRequestLine){
							// Фиксируем ошибку превышения длины стартовой строки
							this->_error = error_t::URL_OVERFLOW;
							// Выходим из состояния
							break;
						}
						// Переходим к разбору reason-phrase
						this->_state = static_cast <uint8_t> (state_t::S_RES_REASON_START);
						// Выходим из состояния
						break;
					}
					// Если получен CR - ожидаем LF
					if(ch == '\r'){
						// Переходим к ожиданию LF
						this->_state = static_cast <uint8_t> (state_t::S_RES_LINE_LF);
						// Выходим из состояния
						break;
					}
					// Если получен LF - стартовая строка разобрана полностью (толерантность к голому LF)
					if(ch == '\n'){
						/**
						 * Одиночный LF в роли окончания строки принимается только в толерантном
						 * режиме: расхождение в его трактовке с соседним звеном цепочки -
						 * классический вектор рассинхронизации кадрирования (request smuggling)
						 */
						if(this->_limits.strictEOL){
							// Фиксируем ошибку окончания строки
							this->_error = error_t::INVALID_EOL;
							// Выходим из состояния
							break;
						}
						// Завершаем разбор стартовой строки
						this->commitStartLine();
						// Выходим из состояния
						break;
					}
					// Любой другой символ после статус-кода недопустим
					this->_error = error_t::INVALID_STATUS;
				} break;
				// Пробел перед reason-phrase
				case static_cast <uint8_t> (state_t::S_RES_REASON_START): {
					// Если получен CR - reason-phrase пустая, ожидаем LF
					if(ch == '\r'){
						// Переходим к ожиданию LF
						this->_state = static_cast <uint8_t> (state_t::S_RES_LINE_LF);
						// Выходим из состояния
						break;
					}
					// Если получен LF - стартовая строка разобрана полностью (толерантность к голому LF)
					if(ch == '\n'){
						/**
						 * Одиночный LF в роли окончания строки принимается только в толерантном
						 * режиме: расхождение в его трактовке с соседним звеном цепочки -
						 * классический вектор рассинхронизации кадрирования (request smuggling)
						 */
						if(this->_limits.strictEOL){
							// Фиксируем ошибку окончания строки
							this->_error = error_t::INVALID_EOL;
							// Выходим из состояния
							break;
						}
						// Завершаем разбор стартовой строки
						this->commitStartLine();
						// Выходим из состояния
						break;
					}
					// Если символ недопустим в reason-phrase
					if(!::isValueCh(ch)){
						// Фиксируем ошибку некорректного статус-кода
						this->_error = error_t::INVALID_STATUS;
						// Выходим из состояния
						break;
					}
					// Если длина стартовой строки превышает лимит (reason-phrase тоже занимает канал)
					if(++this->_statsHeaders.lineBytes > this->_limits.maxRequestLine){
						// Фиксируем ошибку превышения длины стартовой строки
						this->_error = error_t::URL_OVERFLOW;
						// Выходим из состояния
						break;
					}
					// Добавляем символ к сообщению сервера
					res->message.push_back(static_cast <char> (ch));
					// Переходим к разбору reason-phrase
					this->_state = static_cast <uint8_t> (state_t::S_RES_REASON);
				} break;
				// Разбор reason-phrase
				case static_cast <uint8_t> (state_t::S_RES_REASON): {
					// Если получен CR - ожидаем LF
					if(ch == '\r'){
						// Переходим к ожиданию LF
						this->_state = static_cast <uint8_t> (state_t::S_RES_LINE_LF);
						// Выходим из состояния
						break;
					}
					// Если получен LF - стартовая строка разобрана полностью (толерантность к голому LF)
					if(ch == '\n'){
						/**
						 * Одиночный LF в роли окончания строки принимается только в толерантном
						 * режиме: расхождение в его трактовке с соседним звеном цепочки -
						 * классический вектор рассинхронизации кадрирования (request smuggling)
						 */
						if(this->_limits.strictEOL){
							// Фиксируем ошибку окончания строки
							this->_error = error_t::INVALID_EOL;
							// Выходим из состояния
							break;
						}
						// Завершаем разбор стартовой строки
						this->commitStartLine();
						// Выходим из состояния
						break;
					}
					// Если символ недопустим в reason-phrase
					if(!::isValueCh(ch)){
						// Фиксируем ошибку некорректного статус-кода
						this->_error = error_t::INVALID_STATUS;
						// Выходим из состояния
						break;
					}
					/**
					 * Длина reason-phrase учитывается в общем бюджете стартовой строки,
					 * а не отдельным лимитом: отдельный лимит позволял бы стартовой строке
					 * ответа занять вдвое больше канала, чем разрешено стартовой строке
					 * запроса при том же значении настройки
					 */
					if(++this->_statsHeaders.lineBytes > this->_limits.maxRequestLine){
						// Фиксируем ошибку превышения длины стартовой строки
						this->_error = error_t::URL_OVERFLOW;
						// Выходим из состояния
						break;
					}
					// Добавляем символ к сообщению сервера
					res->message.push_back(static_cast <char> (ch));
				} break;
				// Ожидание LF после CR
				case static_cast <uint8_t> (state_t::S_RES_LINE_LF): {
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
				case static_cast <uint8_t> (state_t::S_HEADER_START): {
					// Если получен CR - ожидаем LF финальной пустой строки
					if(ch == '\r'){
						// Переходим к ожиданию LF финальной пустой строки
						this->_state = static_cast <uint8_t> (state_t::S_HEADERS_LF);
						// Выходим из состояния
						break;
					}
					// Если получен LF - заголовки разобраны полностью (толерантность к голому LF)
					if(ch == '\n'){
						/**
						 * Одиночный LF в роли окончания строки принимается только в толерантном
						 * режиме: расхождение в его трактовке с соседним звеном цепочки -
						 * классический вектор рассинхронизации кадрирования (request smuggling)
						 */
						if(this->_limits.strictEOL){
							// Фиксируем ошибку окончания строки
							this->_error = error_t::INVALID_EOL;
							// Выходим из состояния
							break;
						}
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
					if(!::isToken(ch)){
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
					this->_state = static_cast <uint8_t> (state_t::S_HEADER_NAME);
				} break;
				// Разбор имени заголовка
				case static_cast <uint8_t> (state_t::S_HEADER_NAME): {
					// Если получено двоеточие - имя заголовка разобрано полностью
					if(ch == ':'){
						// Очищаем накопитель значения текущего заголовка
						this->_header.value.clear();
						// Переходим к пропуску ведущих OWS значения
						this->_state = static_cast <uint8_t> (state_t::S_HEADER_VALUE_OWS);
						// Выходим из состояния
						break;
					}
					// Если символ не является символом токена
					if(!::isToken(ch)){
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
				case static_cast <uint8_t> (state_t::S_HEADER_VALUE_OWS): {
					// Пропускаем ведущие пробелы и табуляции
					if((ch == ' ') || (ch == '\t')){
						/**
						 * Отброшенные при разборе октеты входят в бюджет блока заголовков
						 * наравне с сохранёнными: канал и процессорное время они занимают
						 * такие же, а без их учёта одна строка вида "X:" с потоком пробелов
						 * растягивалась бы неограниченно, не приближая разбор к завершению
						 * и не расходуя ни одного лимита
						 */
						if(++this->_statsHeaders.bytes > this->_limits.maxHeadersTotal)
							// Фиксируем ошибку превышения размера заголовков
							this->_error = error_t::HEADER_OVERFLOW;
						// Выходим из состояния
						break;
					}
					// Если получен CR - значение пустое, ожидаем LF
					if(ch == '\r'){
						// Переходим к ожиданию LF
						this->_state = static_cast <uint8_t> (state_t::S_HEADER_ALMOST_DONE);
						// Выходим из состояния
						break;
					}
					// Если получен LF - заголовок разобран полностью (толерантность к голому LF)
					if(ch == '\n'){
						/**
						 * Одиночный LF в роли окончания строки принимается только в толерантном
						 * режиме: расхождение в его трактовке с соседним звеном цепочки -
						 * классический вектор рассинхронизации кадрирования (request smuggling)
						 */
						if(this->_limits.strictEOL){
							// Фиксируем ошибку окончания строки
							this->_error = error_t::INVALID_EOL;
							// Выходим из состояния
							break;
						}
						// Если завершение разбора заголовка не удалось
						if(!this->commitHeader(this->_header.name, this->_header.value))
							// Выходим из состояния (ошибка уже установлена)
							break;
						// Очищаем накопитель имени (выделенная память строки сохраняется)
						this->_header.name.clear();
						// Очищаем накопитель значения (выделенная память строки сохраняется)
						this->_header.value.clear();
						// Переходим к разбору следующего заголовка
						this->_state = static_cast <uint8_t> (state_t::S_HEADER_START);
						// Выходим из состояния
						break;
					}
					// Если символ недопустим в значении заголовка
					if(!::isValueCh(ch)){
						// Фиксируем ошибку некорректного значения заголовка
						this->_error = error_t::INVALID_HEADER_VALUE;
						// Выходим из состояния
						break;
					}
					// Добавляем символ к значению заголовка
					this->_header.value.push_back(static_cast <char> (ch));
					// Переходим к разбору значения заголовка
					this->_state = static_cast <uint8_t> (state_t::S_HEADER_VALUE);
				} break;
				// Разбор значения заголовка
				case static_cast <uint8_t> (state_t::S_HEADER_VALUE): {
					// Если получен CR - ожидаем LF
					if(ch == '\r'){
						// Переходим к ожиданию LF
						this->_state = static_cast <uint8_t> (state_t::S_HEADER_ALMOST_DONE);
						// Выходим из состояния
						break;
					}
					// Если получен LF - заголовок разобран полностью (толерантность к голому LF)
					if(ch == '\n'){
						/**
						 * Одиночный LF в роли окончания строки принимается только в толерантном
						 * режиме: расхождение в его трактовке с соседним звеном цепочки -
						 * классический вектор рассинхронизации кадрирования (request smuggling)
						 */
						if(this->_limits.strictEOL){
							// Фиксируем ошибку окончания строки
							this->_error = error_t::INVALID_EOL;
							// Выходим из состояния
							break;
						}
						// Если завершение разбора заголовка не удалось
						if(!this->commitHeader(this->_header.name, this->_header.value))
							// Выходим из состояния (ошибка уже установлена)
							break;
						// Очищаем накопитель имени (выделенная память строки сохраняется)
						this->_header.name.clear();
						// Очищаем накопитель значения (выделенная память строки сохраняется)
						this->_header.value.clear();
						// Переходим к разбору следующего заголовка
						this->_state = static_cast <uint8_t> (state_t::S_HEADER_START);
						// Выходим из состояния
						break;
					}
					// Если символ недопустим в значении заголовка
					if(!::isValueCh(ch)){
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
				case static_cast <uint8_t> (state_t::S_HEADER_ALMOST_DONE): {
					// Если получен не LF
					if(ch != '\n'){
						// Фиксируем ошибку окончания строки
						this->_error = error_t::INVALID_EOL;
						// Выходим из состояния
						break;
					}
					// Если завершение разбора заголовка не удалось
					if(!this->commitHeader(this->_header.name, this->_header.value))
						// Выходим из состояния (ошибка уже установлена)
						break;
					// Очищаем накопитель имени (выделенная память строки сохраняется)
					this->_header.name.clear();
					// Очищаем накопитель значения (выделенная память строки сохраняется)
					this->_header.value.clear();
					// Переходим к разбору следующего заголовка
					this->_state = static_cast <uint8_t> (state_t::S_HEADER_START);
				} break;
				// Ожидание LF финальной пустой строки
				case static_cast <uint8_t> (state_t::S_HEADERS_LF): {
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
				case static_cast <uint8_t> (state_t::S_CHUNK_SIZE): {
					// Если длина строки заголовка чанка превышает лимит
					if(++this->_statsHeaders.chunkLineBytes > this->_limits.maxChunkLine){
						// Фиксируем ошибку превышения размера чанка
						this->_error = error_t::CHUNK_OVERFLOW;
						// Выходим из состояния
						break;
					}
					// Получаем числовое значение hex-цифры
					const int8_t hv = ::hexVal(ch);
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
						this->_state = static_cast <uint8_t> (state_t::S_CHUNK_EXT);
						// Выходим из состояния
						break;
					}
					/**
					 * Если получен пробел или табуляция - это BWS перед расширениями чанка
					 *
					 * По RFC 9112 §7.1 расширения имеют вид *( BWS ";" BWS ... ), поэтому
					 * запись "5 ;ext" грамматически допустима. Отдельное состояние нужно
					 * потому, что BWS входит в грамматику только вместе с точкой с запятой:
					 * запись "5 " без расширений остаётся ошибкой, и это принципиально -
					 * звено цепочки, читающее размер чанка до пробела, а не до конца строки,
					 * увидело бы иное кадрирование
					 */
					if((ch == ' ') || (ch == '\t')){
						// Переходим к пропуску BWS перед расширениями чанка
						this->_state = static_cast <uint8_t> (state_t::S_CHUNK_SIZE_BWS);
						// Выходим из состояния
						break;
					}
					// Если получен CR - ожидаем LF
					if(ch == '\r'){
						// Переходим к ожиданию LF
						this->_state = static_cast <uint8_t> (state_t::S_CHUNK_SIZE_LF);
						// Выходим из состояния
						break;
					}
					// Если получен LF - строка размера чанка разобрана полностью (толерантность к голому LF)
					if(ch == '\n'){
						/**
						 * Одиночный LF в роли окончания строки принимается только в толерантном
						 * режиме: расхождение в его трактовке с соседним звеном цепочки -
						 * классический вектор рассинхронизации кадрирования (request smuggling)
						 */
						if(this->_limits.strictEOL){
							// Фиксируем ошибку окончания строки
							this->_error = error_t::INVALID_EOL;
							// Выходим из состояния
							break;
						}
						// Завершаем разбор строки размера чанка
						this->chunkSizeComplete();
						// Выходим из состояния
						break;
					}
					// Любой другой символ в размере чанка недопустим
					this->_error = error_t::INVALID_CHUNK_SIZE;
				} break;
				// Пропуск BWS между размером чанка и его расширениями
				case static_cast <uint8_t> (state_t::S_CHUNK_SIZE_BWS): {
					// Если длина строки заголовка чанка превышает лимит
					if(++this->_statsHeaders.chunkLineBytes > this->_limits.maxChunkLine){
						// Фиксируем ошибку превышения размера чанка
						this->_error = error_t::CHUNK_OVERFLOW;
						// Выходим из состояния
						break;
					}
					// Пропускаем пробелы и табуляции
					if((ch == ' ') || (ch == '\t'))
						// Выходим из состояния
						break;
					// Если получена точка с запятой - начинаются расширения чанка
					if(ch == ';'){
						// Переходим к пропуску расширений чанка
						this->_state = static_cast <uint8_t> (state_t::S_CHUNK_EXT);
						// Выходим из состояния
						break;
					}
					/**
					 * Всё остальное после пробела недопустимо, включая окончание строки:
					 * BWS входит в грамматику только как часть расширения чанка, и запись
					 * "5 " без точки с запятой расширением не является
					 */
					this->_error = error_t::INVALID_CHUNK_SIZE;
				} break;
				// Пропуск расширений чанка (chunk-ext)
				case static_cast <uint8_t> (state_t::S_CHUNK_EXT): {
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
						this->_state = static_cast <uint8_t> (state_t::S_CHUNK_SIZE_LF);
						// Выходим из состояния
						break;
					}
					// Если получен LF - строка размера чанка разобрана полностью (толерантность к голому LF)
					if(ch == '\n'){
						/**
						 * Одиночный LF в роли окончания строки принимается только в толерантном
						 * режиме: расхождение в его трактовке с соседним звеном цепочки -
						 * классический вектор рассинхронизации кадрирования (request smuggling)
						 */
						if(this->_limits.strictEOL){
							// Фиксируем ошибку окончания строки
							this->_error = error_t::INVALID_EOL;
							// Выходим из состояния
							break;
						}
						// Завершаем разбор строки размера чанка
						this->chunkSizeComplete();
						// Выходим из состояния
						break;
					}
					/**
					 * Валидируем допустимость символа расширений чанка
					 *
					 * Структурно расширения не разбираются - они пропускаются и отдаются
					 * потребителю как есть, - но октеты, недопустимые в них ни при каком
					 * разборе, отвергаются. По RFC 9112 §7.1.1 расширение состоит из token
					 * и token либо quoted-string, а DEL не входит ни в token, ни в qdtext.
					 * Правило берётся то же, что и для значения заголовка: управляющие
					 * символы и DEL запрещены, obs-text допускается - он законен внутри
					 * quoted-string, а отличить её без структурного разбора нечем
					 */
					if(!::isValueCh(ch)){
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
				case static_cast <uint8_t> (state_t::S_CHUNK_SIZE_LF): {
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
				case static_cast <uint8_t> (state_t::S_CHUNK_DATA_ALMOST_DONE): {
					// Если получен CR - ожидаем LF
					if(ch == '\r'){
						// Переходим к ожиданию LF
						this->_state = static_cast <uint8_t> (state_t::S_CHUNK_DATA_LF);
						// Выходим из состояния
						break;
					}
					// Если получен LF - чанк дочитан полностью (толерантность к голому LF)
					if(ch == '\n'){
						/**
						 * Одиночный LF в роли окончания строки принимается только в толерантном
						 * режиме: расхождение в его трактовке с соседним звеном цепочки -
						 * классический вектор рассинхронизации кадрирования (request smuggling)
						 */
						if(this->_limits.strictEOL){
							// Фиксируем ошибку окончания строки
							this->_error = error_t::INVALID_EOL;
							// Выходим из состояния
							break;
						}
						// Уведомляем о завершении приёма данных чанка
						if(!this->fireChunk(phase_t::END, this->_statsBody.chunkSize))
							// Выходим из состояния (разбор прерван)
							break;
						// Сбрасываем счётчик hex-цифр размера чанка
						this->_statsBody.digits = 0;
						// Сбрасываем размер текущего чанка
						this->_statsBody.chunkSize = 0;
						// Сбрасываем длину строки заголовка чанка
						this->_statsHeaders.chunkLineBytes = 0;
						// Переходим к разбору размера следующего чанка
						this->_state = static_cast <uint8_t> (state_t::S_CHUNK_SIZE);
						// Выходим из состояния
						break;
					}
					// Любой другой символ после данных чанка недопустим
					this->_error = error_t::INVALID_CHUNK_TERMINATOR;
				} break;
				// Ожидание LF после CR данных чанка
				case static_cast <uint8_t> (state_t::S_CHUNK_DATA_LF): {
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
					// Сбрасываем счётчик hex-цифр размера чанка
					this->_statsBody.digits = 0;
					// Сбрасываем размер текущего чанка
					this->_statsBody.chunkSize = 0;
					// Сбрасываем длину строки заголовка чанка
					this->_statsHeaders.chunkLineBytes = 0;
					// Переходим к разбору размера следующего чанка
					this->_state = static_cast <uint8_t> (state_t::S_CHUNK_SIZE);
				} break;
				/**
				 * Трейлеры сообщения
				 */
				// Начало строки трейлера
				case static_cast <uint8_t> (state_t::S_TRAILER_START): {
					// Если получен CR - ожидаем LF финальной пустой строки
					if(ch == '\r'){
						// Переходим к ожиданию LF финальной пустой строки
						this->_state = static_cast <uint8_t> (state_t::S_TRAILERS_LF);
						// Выходим из состояния
						break;
					}
					// Если получен LF - трейлеры разобраны полностью (толерантность к голому LF)
					if(ch == '\n'){
						/**
						 * Одиночный LF в роли окончания строки принимается только в толерантном
						 * режиме: расхождение в его трактовке с соседним звеном цепочки -
						 * классический вектор рассинхронизации кадрирования (request smuggling)
						 */
						if(this->_limits.strictEOL){
							// Фиксируем ошибку окончания строки
							this->_error = error_t::INVALID_EOL;
							// Выходим из состояния
							break;
						}
						// Уведомляем о завершении блока трейлеров (провайдер для трейлеров - nullptr)
						if(this->fireProvider(nullptr, true) && this->firePhase(phase_t::END, part_t::TRAILER))
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
					if(!::isToken(ch)){
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
					this->_state = static_cast <uint8_t> (state_t::S_TRAILER_NAME);
				} break;
				// Разбор имени трейлера
				case static_cast <uint8_t> (state_t::S_TRAILER_NAME): {
					// Если получено двоеточие - имя трейлера разобрано полностью
					if(ch == ':'){
						// Очищаем накопитель значения текущего трейлера
						this->_header.value.clear();
						// Переходим к пропуску ведущих OWS значения
						this->_state = static_cast <uint8_t> (state_t::S_TRAILER_VALUE_OWS);
						// Выходим из состояния
						break;
					}
					// Если символ не является символом токена
					if(!::isToken(ch)){
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
				case static_cast <uint8_t> (state_t::S_TRAILER_VALUE_OWS): {
					// Пропускаем ведущие пробелы и табуляции
					if((ch == ' ') || (ch == '\t')){
						// Отброшенные октеты входят в собственный бюджет блока трейлеров
						if(++this->_statsHeaders.bytes > this->_limits.maxHeadersTotal)
							// Фиксируем ошибку превышения размера заголовков
							this->_error = error_t::HEADER_OVERFLOW;
						// Выходим из состояния
						break;
					}
					// Если получен CR - значение пустое, ожидаем LF
					if(ch == '\r'){
						// Переходим к ожиданию LF
						this->_state = static_cast <uint8_t> (state_t::S_TRAILER_ALMOST_DONE);
						// Выходим из состояния
						break;
					}
					// Если получен LF - трейлер разобран полностью (толерантность к голому LF)
					if(ch == '\n'){
						/**
						 * Одиночный LF в роли окончания строки принимается только в толерантном
						 * режиме: расхождение в его трактовке с соседним звеном цепочки -
						 * классический вектор рассинхронизации кадрирования (request smuggling)
						 */
						if(this->_limits.strictEOL){
							// Фиксируем ошибку окончания строки
							this->_error = error_t::INVALID_EOL;
							// Выходим из состояния
							break;
						}
						// Если завершение разбора трейлера не удалось
						if(!this->commitHeader(this->_header.name, this->_header.value))
							// Выходим из состояния (ошибка уже установлена)
							break;
						// Очищаем накопитель имени (выделенная память строки сохраняется)
						this->_header.name.clear();
						// Очищаем накопитель значения (выделенная память строки сохраняется)
						this->_header.value.clear();
						// Переходим к разбору следующего трейлера
						this->_state = static_cast <uint8_t> (state_t::S_TRAILER_START);
						// Выходим из состояния
						break;
					}
					// Если символ недопустим в значении трейлера
					if(!::isValueCh(ch)){
						// Фиксируем ошибку некорректного значения заголовка
						this->_error = error_t::INVALID_HEADER_VALUE;
						// Выходим из состояния
						break;
					}
					// Добавляем символ к значению трейлера
					this->_header.value.push_back(static_cast <char> (ch));
					// Переходим к разбору значения трейлера
					this->_state = static_cast <uint8_t> (state_t::S_TRAILER_VALUE);
				} break;
				// Разбор значения трейлера
				case static_cast <uint8_t> (state_t::S_TRAILER_VALUE): {
					// Если получен CR - ожидаем LF
					if(ch == '\r'){
						// Переходим к ожиданию LF
						this->_state = static_cast <uint8_t> (state_t::S_TRAILER_ALMOST_DONE);
						// Выходим из состояния
						break;
					}
					// Если получен LF - трейлер разобран полностью (толерантность к голому LF)
					if(ch == '\n'){
						/**
						 * Одиночный LF в роли окончания строки принимается только в толерантном
						 * режиме: расхождение в его трактовке с соседним звеном цепочки -
						 * классический вектор рассинхронизации кадрирования (request smuggling)
						 */
						if(this->_limits.strictEOL){
							// Фиксируем ошибку окончания строки
							this->_error = error_t::INVALID_EOL;
							// Выходим из состояния
							break;
						}
						// Если завершение разбора трейлера не удалось
						if(!this->commitHeader(this->_header.name, this->_header.value))
							// Выходим из состояния (ошибка уже установлена)
							break;
						// Очищаем накопитель имени (выделенная память строки сохраняется)
						this->_header.name.clear();
						// Очищаем накопитель значения (выделенная память строки сохраняется)
						this->_header.value.clear();
						// Переходим к разбору следующего трейлера
						this->_state = static_cast <uint8_t> (state_t::S_TRAILER_START);
						// Выходим из состояния
						break;
					}
					// Если символ недопустим в значении трейлера
					if(!::isValueCh(ch)){
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
				case static_cast <uint8_t> (state_t::S_TRAILER_ALMOST_DONE): {
					// Если получен не LF
					if(ch != '\n'){
						// Фиксируем ошибку окончания строки
						this->_error = error_t::INVALID_EOL;
						// Выходим из состояния
						break;
					}
					// Если завершение разбора трейлера не удалось
					if(!this->commitHeader(this->_header.name, this->_header.value))
						// Выходим из состояния (ошибка уже установлена)
						break;
					// Очищаем накопитель имени (выделенная память строки сохраняется)
					this->_header.name.clear();
					// Очищаем накопитель значения (выделенная память строки сохраняется)
					this->_header.value.clear();
					// Переходим к разбору следующего трейлера
					this->_state = static_cast <uint8_t> (state_t::S_TRAILER_START);
				} break;
				// Ожидание LF финальной пустой строки трейлеров
				case static_cast <uint8_t> (state_t::S_TRAILERS_LF): {
					// Если получен не LF
					if(ch != '\n'){
						// Фиксируем ошибку окончания строки
						this->_error = error_t::INVALID_EOL;
						// Выходим из состояния
						break;
					}
					// Уведомляем о завершении блока трейлеров (провайдер для трейлеров - nullptr)
					if(this->fireProvider(nullptr, true) && this->firePhase(phase_t::END, part_t::TRAILER))
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
				// Устанавливаем итоговый статус разбора и записываем ошибку в лог
				this->fail(this->_error);
				// Выводим количество обработанных байт данных
				return i;
			}
			// Если сообщение полностью разобрано
			if(this->_state == static_cast <uint8_t> (state_t::S_MESSAGE_DONE)){
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
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
		// Фиксируем внутреннюю ошибку разбора
		this->fail(error_t::INTERNAL);
		// Выводим количество обработанных байт данных
		return i;
	}
}
/**
 * @brief Метод получения кода ошибки разбора
 *
 * @return код ошибки
 *
 */
awh::http::Parser_HTTP::error_t awh::http::Parser_HTTP::error() const noexcept {
	// Выводим код ошибки разбора
	return this->_error;
}
/**
 * @brief Метод получения человекочитаемого названия текущей ошибки разбора
 *
 * @return название текущей ошибки разбора
 *
 */
string_view awh::http::Parser_HTTP::errorName() const noexcept {
	// Выводим название текущего кода ошибки разбора
	return errorName(this->_error);
}
/**
 * @brief Метод получения человекочитаемого названия кода ошибки
 *
 * @param error код ошибки разбора
 * @return      название кода ошибки
 *
 */
string_view awh::http::Parser_HTTP::errorName(const error_t error) noexcept {
	/**
	 * В зависимости от кода ошибки разбора, выводим соответствующее название
	 */
	switch(static_cast <uint8_t> (error)){
		// Ошибок нет
		case static_cast <uint8_t> (error_t::NONE):
			// Выводим название кода ошибки
			return "NONE";
		// Внутренняя ошибка состояния
		case static_cast <uint8_t> (error_t::INTERNAL):
			// Выводим название кода ошибки
			return "INTERNAL";
		// Ожидался LF после CR
		case static_cast <uint8_t> (error_t::INVALID_EOL):
			// Выводим название кода ошибки
			return "INVALID_EOL";
		// Недопустимый символ в методе
		case static_cast <uint8_t> (error_t::INVALID_METHOD):
			// Выводим название кода ошибки
			return "INVALID_METHOD";
		// Недопустимый символ в request-target
		case static_cast <uint8_t> (error_t::INVALID_TARGET):
			// Выводим название кода ошибки
			return "INVALID_TARGET";
		// Неверный статус-код ответа
		case static_cast <uint8_t> (error_t::INVALID_STATUS):
			// Выводим название кода ошибки
			return "INVALID_STATUS";
		// Неверная строка версии (HTTP/x.y)
		case static_cast <uint8_t> (error_t::INVALID_VERSION):
			// Выводим название кода ошибки
			return "INVALID_VERSION";
		// Неверный размер чанка
		case static_cast <uint8_t> (error_t::INVALID_CHUNK_SIZE):
			// Выводим название кода ошибки
			return "INVALID_CHUNK_SIZE";
		// Недопустимый символ в имени заголовка / obs-fold
		case static_cast <uint8_t> (error_t::INVALID_HEADER_TOKEN):
			// Выводим название кода ошибки
			return "INVALID_HEADER_TOKEN";
		// Недопустимый символ в значении заголовка
		case static_cast <uint8_t> (error_t::INVALID_HEADER_VALUE):
			// Выводим название кода ошибки
			return "INVALID_HEADER_VALUE";
		// Content-Length не число / некорректен
		case static_cast <uint8_t> (error_t::INVALID_CONTENT_LENGTH):
			// Выводим название кода ошибки
			return "INVALID_CONTENT_LENGTH";
		// Нет CRLF после данных чанка
		case static_cast <uint8_t> (error_t::INVALID_CHUNK_TERMINATOR):
			// Выводим название кода ошибки
			return "INVALID_CHUNK_TERMINATOR";
		// Некорректный Transfer-Encoding (chunked не последний и т.п.)
		case static_cast <uint8_t> (error_t::INVALID_TRANSFER_ENCODING):
			// Выводим название кода ошибки
			return "INVALID_TRANSFER_ENCODING";
		// Разбор прерван пользовательским callback'ом
		case static_cast <uint8_t> (error_t::ABORTED):
			// Выводим название кода ошибки
			return "ABORTED";
		// Превышен лимит длины request-line
		case static_cast <uint8_t> (error_t::URL_OVERFLOW):
			// Выводим название кода ошибки
			return "URL_OVERFLOW";
		// Превышен лимит размера тела
		case static_cast <uint8_t> (error_t::BODY_OVERFLOW):
			// Выводим название кода ошибки
			return "BODY_OVERFLOW";
		// Соединение закрыто посреди незавершённого сообщения
		case static_cast <uint8_t> (error_t::PREMATURE_EOF):
			// Выводим название кода ошибки
			return "PREMATURE_EOF";
		// У запроса HTTP/1.1 отсутствует либо продублирован заголовок Host
		case static_cast <uint8_t> (error_t::MISSING_HOST):
			// Выводим название кода ошибки
			return "MISSING_HOST";
		// Превышен лимит размера чанка
		case static_cast <uint8_t> (error_t::CHUNK_OVERFLOW):
			// Выводим название кода ошибки
			return "CHUNK_OVERFLOW";
		// Превышен лимит размера заголовков
		case static_cast <uint8_t> (error_t::HEADER_OVERFLOW):
			// Выводим название кода ошибки
			return "HEADER_OVERFLOW";
		// Превышено число заголовков
		case static_cast <uint8_t> (error_t::TOO_MANY_HEADERS):
			// Выводим название кода ошибки
			return "TOO_MANY_HEADERS";
		// CL+TE или несколько разных Content-Length (request smuggling)
		case static_cast <uint8_t> (error_t::CONTENT_LENGTH_CONFLICT):
			// Выводим название кода ошибки
			return "CONTENT_LENGTH_CONFLICT";
	}
	// Код ошибки неизвестен
	return "UNKNOWN_ERROR";
}
/**
 * @brief Метод получения лимитов безопасности
 *
 * @return лимиты безопасности
 *
 */
const awh::http::Parser_HTTP::limits_t & awh::http::Parser_HTTP::limits() const noexcept {
	// Выводим настроенные лимиты безопасности
	return this->_limits;
}
/**
 * @brief Метод установки лимитов безопасности
 *
 * @param limits лимиты безопасности
 *
 */
void awh::http::Parser_HTTP::limits(const limits_t & limits) noexcept {
	// Устанавливаем новые лимиты безопасности
	this->_limits = limits;
}
/**
 * @brief Метод получения разобранного сообщения
 *
 * @return разобранное сообщение
 *
 */
const awh::http::Parser_HTTP::message_t & awh::http::Parser_HTTP::message() const noexcept {
	// Выводим результат разбора сообщения
	return this->_message;
}
/**
 * @brief Метод сброса состояния отправки для следующего сообщения в том же соединении
 *
 * @details Готовит отправитель к следующему сообщению (keep-alive): сбрасывает
 *          кадрирование, источник данных и флаги, но НЕ трогает неотправленный
 *          остаток выходного буфера. Состояние разбора не затрагивается -
 *          для него используется reset().
 *
 */
void awh::http::Parser_HTTP::resetSender() noexcept {
	// Сбрасываем остаток тела до полного Content-Length
	this->_sender.remaining = 0;
	// Сбрасываем признак завершения исходящего сообщения
	this->_sender.endSent = false;
	// Сбрасываем признак достижения конца тела источника
	this->_sender.sourceEof = false;
	// Сбрасываем признак отправки заголовков сообщения
	this->_sender.headersSent = false;
	// Взводим сигнал writable
	this->_sender.writableNotified = false;
	// Сбрасываем способ кадрирования тела исходящего сообщения
	this->_sender.framing = sender_t::framing_t::NONE;
	// Удаляем pull-источник данных тела
	this->_sender.source = nullptr;
}
/**
 * @brief Метод назначения pull-источника данных тела сообщения
 *
 * @param source pull-источник данных тела
 *
 */
void awh::http::Parser_HTTP::dataSource(data_source_callback_t source) noexcept {
	/**
	 * Если предыдущее сообщение уже завершено - готовим отправитель к следующему:
	 * иначе назначенный до sendHeaders источник был бы затёрт сбросом состояния
	 * отправителя внутри самого sendHeaders
	 */
	if(this->_sender.headersSent && this->_sender.endSent)
		// Готовим отправитель к следующему сообщению
		this->resetSender();
	// Сбрасываем признак достижения конца тела источника
	this->_sender.sourceEof = false;
	// Устанавливаем pull-источник данных тела
	this->_sender.source = ::move(source);
	// Запускаем прокачку тела из источника данных
	this->pumpSource();
}
/**
 * @brief Метод настройки порогов выходного буфера отправки
 *
 * @param high ёмкость выходного буфера отправки (high-water)
 * @param low  порог сигнала writable (low-water)
 *
 */
void awh::http::Parser_HTTP::sendWaterMarks(const size_t high, const size_t low) noexcept {
	// Устанавливаем порог сигнала writable
	this->_sender.lowWater = low;
	// Устанавливаем ёмкость выходного буфера отправки (не меньше порога сигнала writable)
	this->_sender.highWater = ::max(high, low);
	/**
	 * Согласуем лимит памяти смартбуферов с настроенными порогами: помимо полезных
	 * данных буфер держит блок заголовков и порцию pull-источника с кадрированием,
	 * а исчерпание лимита остановило бы отправку. Значение по умолчанию сохраняется,
	 * пока пороги его не перерастают
	 */
	const size_t memory = ::max(static_cast <size_t> (AWH_MAX_MEMORY_BUFFER), (this->_sender.highWater * 4));
	// Устанавливаем лимит памяти буфера исходящих байтов
	this->_sender.output.setMaxMemory(memory);
	// Устанавливаем лимит памяти буфера передачи в сетевой слой
	this->_sender.flushing.setMaxMemory(memory);
}
/**
 * @brief Метод отправки блока заголовков (запрос/ответ/трейлеры) исходящего сообщения
 *
 * @details Способ кадрирования тела выбирается по заголовкам контейнера:
 *          - установлен Content-Length - тело фиксированного размера (IDENTITY);
 *          - Content-Length отсутствует и endStream == false - добавляется
 *            Transfer-Encoding: chunked. В HTTP/1.0 кодирования chunked не
 *            существует: объявление снимается с провода, тело ответа кадрируется
 *            закрытием соединения, а тело запроса кадрировать нечем - оно не
 *            принимается вовсе (ни sendData, ни pull-источником), и сообщение
 *            помечается завершённым, поскольку на проводе блок заголовков без
 *            Content-Length и без Transfer-Encoding уже является законченным
 *            запросом без тела (RFC 9112 §6.3);
 *          - endStream == true - тела нет, заголовки отправляются как есть;
 *            если контейнер при этом объявляет Transfer-Encoding, оканчивающийся
 *            токеном chunked, пустое тело завершается нулевым чанком - блок
 *            заголовков сам по себе конца сообщения не обозначает, и получатель
 *            иначе ждал бы тело до закрытия соединения.
 *          Контейнер без провайдера в режиме chunked интерпретируется как
 *          трейлеры (завершает тело последним чанком) - та же семантика,
 *          что у HTTP/2. Во всех остальных случаях контейнер без провайдера
 *          отбрасывается с записью в лог: стартовую строку формировать не из
 *          чего, и блок ушёл бы на провод голыми полями.
 *
 * @param headers   контейнер заголовков (провайдер контейнера задаёт стартовую строку)
 * @param endStream флаг завершения сообщения (тела не будет)
 *
 */
void awh::http::Parser_HTTP::sendHeaders(const headers_t & headers, const bool endStream) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если заголовки сообщения уже отправлены
		if(this->_sender.headersSent){
			// Если тело кадрируется chunked и контейнер без провайдера - это трейлеры
			if(!this->_sender.endSent && (this->_sender.framing == sender_t::framing_t::CHUNKED) && (headers.provider() == nullptr)){
				/**
				 * Если тело сообщения ещё формирует pull-источник - трейлеры несовместимы
				 *
				 * Блок трейлеров завершает тело нулевым чанком, а источник свою выдачу
				 * ещё не закончил: недочитанный остаток тела оказался бы отрезан, и
				 * получатель принял бы усечённое сообщение за полное. Источник завершает
				 * тело сам по достижении конца данных - дописывать трейлеры можно только
				 * к телу, выданному методом sendData, что и зафиксировано в описании
				 * dataSource
				 */
				if((this->_sender.source != nullptr) && !this->_sender.sourceEof){
					// Записываем сообщение об отброшенном блоке трейлеров в лог
					this->_log->print(
						"HTTP/1.x trailers are incompatible with an unfinished pull data source: block dropped",
						log_t::flag_t::CRITICAL
					);
					// Выходим из метода
					return;
				}
				// Сериализуем блок трейлеров сообщения
				string block = headers.print(http::proto_t::HTTP1);
				// Вычищаем из блока поля, запрещённые в трейлерах
				const size_t dropped = ::dropForbiddenTrailers(block);
				// Если запрещённые поля обнаружены
				if(dropped > 0)
					// Записываем сообщение об отброшенных полях трейлеров в лог
					this->_log->print(
						"HTTP/1.x outgoing trailer fields are not allowed and have been dropped: %zu field(s)",
						log_t::flag_t::WARNING, dropped
					);
				// Завершаем тело последним (нулевым) чанком без пустой строки
				this->_sender.output.push("0\r\n", 3);
				// Дописываем блок трейлеров с завершающей пустой строкой
				this->_sender.output.push(block);
				// Помечаем что исходящее сообщение завершено
				this->_sender.endSent = true;
				// Передаём исходящие байты сетевому слою
				this->flush();
				// Выходим из метода
				return;
			}
			/**
			 * Если предыдущее сообщение не завершено - отправка нового недопустима:
			 * его блок заголовков ушёл бы на провод посреди чужого тела, и получатель
			 * прочитал бы его как продолжение тела либо рассинхронизировал кадрирование.
			 * Отказ фиксируется в логе: без него вызывающая сторона видит лишь то, что
			 * сообщение молча не ушло, и причину искать негде
			 */
			if(!this->_sender.endSent){
				// Записываем сообщение об отказе отправить блок заголовков в лог
				this->_log->print(
					"HTTP/1.x previous outgoing message is not finished: the headers block has been dropped",
					log_t::flag_t::CRITICAL
				);
				// Выходим из метода
				return;
			}
			// Предыдущее сообщение завершено - готовим отправитель к следующему сообщению
			this->resetSender();
		}
		/**
		 * Контейнер без провайдера является блоком трейлеров, и единственное место,
		 * где он допустим - незавершённое сообщение с кадрированием chunked, которое
		 * обработано выше. Во всех остальных случаях стартовую строку формировать
		 * не из чего, и блок ушёл бы на провод голыми полями: получатель прочитал бы
		 * его как начало следующего сообщения и рассинхронизировал кадрирование.
		 * Такое сочетание возникает, например, при попытке дослать трейлеры после
		 * тела, отданного pull-источником: источник завершает сообщение сам по
		 * достижении конца тела, и дописывать к нему уже нечего
		 */
		if(headers.provider() == nullptr){
			// Записываем сообщение об отброшенном блоке трейлеров в лог
			this->_log->print(
				"HTTP/1.x trailers are only allowed inside an unfinished chunked message: block dropped",
				log_t::flag_t::CRITICAL
			);
			// Выходим из метода
			return;
		}
		// Версия протокола исходящего сообщения (по умолчанию HTTP/1.1)
		version_t version = version_t::HTTP1_1;
		// Если провайдер контейнера установлен и версия протокола определена
		if((headers.provider() != nullptr) && (headers.provider()->version != version_t::NONE))
			// Получаем версию протокола из провайдера контейнера
			version = headers.provider()->version;
		// Признак необходимости дописать заголовок Transfer-Encoding: chunked
		bool injectChunked = false;
		// Признак необходимости завершить объявленное кадрирование chunked нулевым чанком
		bool terminateChunked = false;
		// Признак необходимости вычистить конфликтующий заголовок Transfer-Encoding
		bool dropEncoding = false;
		// Признак необходимости вычистить заголовок Transfer-Encoding, недопустимый в HTTP/1.0
		bool dropLegacyEncoding = false;
		// Признак отказа кадрировать тело исходящего сообщения (сообщение завершается заголовками)
		bool bodyRefused = false;
		// Признак объявленного нулевого размера тела (сообщение завершается заголовками)
		bool bodyEmpty = false;
		// Признак необходимости вычистить некорректный заголовок Content-Length
		bool dropLength = false;
		// Признак пригодного к кадрированию заголовка Content-Length
		bool usableLength = false;
		// Признак наличия заголовка Content-Length в контейнере
		const bool hasLength = headers.has("Content-Length");
		// Признак наличия заголовка Transfer-Encoding в контейнере
		const bool hasEncoding = headers.has("Transfer-Encoding");
		/**
		 * Если сообщение завершается заголовками, а объявлен ненулевой Content-Length:
		 * у запроса это всегда ошибка вызывающей стороны - тела не будет, а получатель
		 * останется ждать объявленный объём. У ответа такое сочетание законно: ответ на
		 * HEAD и ответ [304 Not Modified] несут размер гипотетического тела
		 */
		if(hasLength && endStream && (this->_direct == direct_t::REQUEST)){
			// Значение объявленного размера тела
			uint64_t announced = 0;
			// Получаем значение заголовка Content-Length
			const string & length = headers.at("Content-Length");
			// Если объявлен ненулевой размер тела
			if(!::parseDecimal(length.data(), length.size(), announced) || (announced > 0)){
				// Помечаем заголовок к вычистке из блока
				dropLength = true;
				// Записываем сообщение о несовместимом заголовке Content-Length в лог
				this->_log->print(
					"HTTP/1.x outgoing request declares Content-Length without a body and it has been dropped: %s",
					log_t::flag_t::CRITICAL, length.c_str()
				);
			}
		}
		// Если заголовок Content-Length установлен и сообщение предполагает тело
		if(hasLength && !endStream){
			// Получаем значение заголовка Content-Length
			const string & length = headers.at("Content-Length");
			// Проверяем пригодность значения заголовка Content-Length к кадрированию
			usableLength = ::parseDecimal(length.data(), length.size(), this->_sender.remaining);
			// Если значение заголовка Content-Length не является корректным числом
			if(!usableLength){
				// Сбрасываем остаток тела до полного Content-Length
				this->_sender.remaining = 0;
				/**
				 * Некорректный Content-Length вычищается из блока, а тело кадрируется
				 * способом, не требующим заранее известной длины: отправка заголовка,
				 * который принимающая сторона обязана отвергнуть, оставила бы соединение
				 * в состоянии, из которого нет корректного продолжения
				 */
				dropLength = true;
				// Записываем сообщение о некорректном заголовке Content-Length в лог
				this->_log->print(
					"HTTP/1.x outgoing Content-Length is not a valid number and has been dropped: %s",
					log_t::flag_t::CRITICAL, length.c_str()
				);
			}
		}
		// Если сообщение завершается заголовками - тела не будет
		if(endStream){
			// Тело исходящего сообщения отсутствует
			this->_sender.framing = sender_t::framing_t::NONE;
			/**
			 * Если заголовок транспортного кодирования установлен
			 */
			if(hasEncoding){
				/**
				 * Одновременная отправка Content-Length и Transfer-Encoding запрещена
				 * (RFC 9112 §6.1) независимо от наличия тела: получатель обязан
				 * отвергнуть такой кадр как попытку request smuggling. Конфликт
				 * разрешается в пользу Content-Length - той же политикой, что и на
				 * пути с телом. Если же Content-Length вычищен выше как несовместимый
				 * с завершением сообщения, конфликта на проводе не остаётся
				 */
				dropEncoding = (hasLength && !dropLength);
				/**
				 * Объявленное вызывающей стороной кадрирование chunked обязано быть
				 * завершено нулевым чанком даже при пустом теле: блок заголовков сам по
				 * себе конца сообщения не обозначает, и получатель ждал бы тело до
				 * закрытия соединения. Проверяется последнее значение заголовка -
				 * значения нескольких заголовков Transfer-Encoding склеиваются по
				 * порядку следования, и кадрирование определяет именно оно.
				 * В HTTP/1.0 кодирования chunked не существует, и нулевой чанк там
				 * оказался бы для получателя частью тела
				 */
				if(!dropEncoding && (version != version_t::HTTP1_0)){
					// Получаем все значения заголовка транспортного кодирования
					const vector <string> values = headers.range("Transfer-Encoding");
					// Определяем необходимость завершить тело нулевым чанком
					terminateChunked = (!values.empty() && ::endsWithChunked(values.back()));
				}
			}
		}
		// Если установлен пригодный заголовок Content-Length - тело фиксированного размера
		else if(usableLength) {
			// Устанавливаем кадрирование тела фиксированного размера
			this->_sender.framing = sender_t::framing_t::IDENTITY;
			/**
			 * Объявленный нулевой размер тела завершает сообщение: на проводе блок
			 * заголовков с Content-Length: 0 уже является законченным сообщением без
			 * тела, и состояние отправителя обязано этому соответствовать. Иначе
			 * отправитель ждал бы тела, которого по объявленному размеру быть не может,
			 * а следующее сообщение отбрасывалось бы как поданное поверх незавершённого
			 */
			bodyEmpty = (this->_sender.remaining == 0);
			/**
			 * Одновременная отправка Content-Length и Transfer-Encoding запрещена
			 * (RFC 9112 §6.1): такой кадр принимающая сторона обязана отвергнуть как
			 * попытку request smuggling. Кадрирование уже выбрано по Content-Length,
			 * поэтому конфликтующий заголовок вычищается из блока
			 */
			dropEncoding = hasEncoding;
		// Если версия протокола HTTP/1.0 - кодирования chunked не существует
		} else if(version == version_t::HTTP1_0){
			/**
			 * Без Content-Length тело в HTTP/1.0 ограничивается только закрытием
			 * соединения. Ответу сервера это подходит, а телу запроса - нет:
			 * запрос без Content-Length и без Transfer-Encoding получатель обязан
			 * считать запросом без тела (RFC 9112 §6.3), и отданные следом байты
			 * прочитает как начало следующего запроса. Кадрировать такое тело
			 * нечем, поэтому оно не принимается вовсе - sendData вернёт ноль,
			 * и вызывающая сторона узнает об отказе вместо того, чтобы выдать
			 * на провод кадр, разбираемый получателем как request smuggling
			 */
			if(this->_direct == direct_t::REQUEST){
				// Тело исходящего сообщения кадрировать нечем
				this->_sender.framing = sender_t::framing_t::NONE;
				/**
				 * Сообщение помечается завершённым: на проводе блок заголовков без
				 * Content-Length и без Transfer-Encoding уже является законченным
				 * запросом без тела (RFC 9112 §6.3), и состояние отправителя обязано
				 * этому соответствовать. Иначе отправитель считал бы сообщение
				 * незавершённым, а следующий sendHeaders молча выходил бы по проверке
				 * незавершённого предыдущего - соединение залипло бы без диагностики
				 */
				bodyRefused = true;
				/**
				 * Удаляем источник данных тела, назначенный до отправки заголовков:
				 * выдать его тело в этом сообщении невозможно, и держать захваченные
				 * им ресурсы до сброса отправителя незачем
				 */
				this->_sender.source = nullptr;
				// Записываем сообщение о невозможности кадрировать тело запроса в лог
				this->_log->print(
					"HTTP/1.x outgoing HTTP/1.0 request declares no valid Content-Length: the body cannot be framed and is not accepted",
					log_t::flag_t::CRITICAL
				);
			// Тело ответа сервера кадрируется закрытием соединения
			} else this->_sender.framing = sender_t::framing_t::RAW;
		}
		// Для HTTP/1.1 без пригодного Content-Length тело кадрируется кодировкой chunked
		else {
			// Устанавливаем кадрирование тела кодировкой chunked
			this->_sender.framing = sender_t::framing_t::CHUNKED;
			/**
			 * Заголовок Transfer-Encoding дописывается, если контейнер его не несёт либо
			 * несёт кодирование, не заканчивающееся токеном chunked: тело кадрируется
			 * chunked в любом случае, и объявление обязано этому соответствовать.
			 * Отдельный заголовок корректен - по RFC 9112 §6.1 значения нескольких
			 * заголовков Transfer-Encoding склеиваются по порядку следования
			 */
			injectChunked = !hasEncoding;
			// Если заголовок транспортного кодирования установлен - проверяем его последнее значение
			if(hasEncoding){
				/**
				 * Значения нескольких заголовков Transfer-Encoding склеиваются по порядку
				 * следования, поэтому кадрирование определяет последний из них - проверять
				 * только первое значение недостаточно
				 */
				const vector <string> values = headers.range("Transfer-Encoding");
				// Дописываем заголовок, если последнее кодирование не заканчивается токеном chunked
				injectChunked = (values.empty() || !::endsWithChunked(values.back()));
			}
		}
		/**
		 * Заголовок Transfer-Encoding появился в HTTP/1.1, и сообщение HTTP/1.0 с этим
		 * заголовком получатель обязан считать сообщением с неисправным кадрированием -
		 * даже при наличии Content-Length - и закрыть соединение после его обработки
		 * (RFC 9112 §6.1). Кадрирование для HTTP/1.0 уже выбрано выше: фиксированным
		 * размером по Content-Length либо сырым телом до закрытия соединения. Поэтому
		 * заголовок снимается с провода - иначе объявление противоречило бы тому, чем
		 * тело кадрируется на самом деле
		 */
		if(hasEncoding && !dropEncoding && (version == version_t::HTTP1_0))
			// Помечаем заголовок к вычистке из блока
			dropLegacyEncoding = true;
		/**
		 * Если объявление транспортного кодирования уходит на провод и содержит chunked
		 * не последним кодированием, сообщение не собирается вовсе. Дописать chunked
		 * нельзя: к телу он применялся бы дважды, что запрещено RFC 9112 §6.1. Изменить
		 * порядок кодирований библиотека тоже не вправе - ей неизвестно, какие из них
		 * вызывающая сторона к телу действительно применила. Собрать заведомо отвергаемый
		 * получателем кадр хуже, чем отказать явно. Проверка стоит здесь, а не в ветке
		 * выбора кадрирования: заголовок уходит на провод и при завершении сообщения
		 * заголовками, когда кадрирование тела не выбирается вовсе
		 */
		if(hasEncoding && !dropEncoding && !dropLegacyEncoding){
			// Получаем все значения заголовка транспортного кодирования
			const vector <string> values = headers.range("Transfer-Encoding");
			// Если последнее кодирование не заканчивается токеном chunked
			if(!values.empty() && !::endsWithChunked(values.back())){
				/**
				 * Выполняем перебор всех значений заголовка транспортного кодирования
				 */
				for(auto & value : values){
					// Если объявление всё же содержит кодирование chunked - оно не последнее
					if(::containsChunked(value)){
						// Возвращаем кадрирование тела в исходное состояние
						this->_sender.framing = sender_t::framing_t::NONE;
						// Записываем сообщение о неисправимом объявлении кодирования в лог
						this->_log->print(
							"HTTP/1.x outgoing Transfer-Encoding declares chunked not as the final coding: message dropped",
							log_t::flag_t::CRITICAL
						);
						// Выходим из метода
						return;
					}
				}
			}
		}
		// Сериализуем стартовую строку и заголовки сообщения
		string block = headers.print(http::proto_t::HTTP1);
		// Если необходимо вычистить некорректный заголовок Content-Length
		if(dropLength)
			// Удаляем некорректный заголовок из сериализованного блока
			::dropHeaderLine(block, "content-length");
		// Если необходимо вычистить конфликтующий заголовок Transfer-Encoding
		if(dropEncoding){
			// Записываем сообщение о конфликте кадрирования исходящего сообщения в лог
			this->_log->print("HTTP/1.x outgoing message declares both Content-Length and Transfer-Encoding: the latter has been dropped", log_t::flag_t::WARNING);
			// Удаляем конфликтующий заголовок из сериализованного блока
			::dropHeaderLine(block, "transfer-encoding");
		// Если необходимо вычистить заголовок Transfer-Encoding, недопустимый в HTTP/1.0
		} else if(dropLegacyEncoding) {
			// Записываем сообщение о недопустимом в HTTP/1.0 кадрировании исходящего сообщения в лог
			this->_log->print("HTTP/1.x outgoing HTTP/1.0 message declares Transfer-Encoding and it has been dropped", log_t::flag_t::WARNING);
			// Удаляем недопустимый в HTTP/1.0 заголовок из сериализованного блока
			::dropHeaderLine(block, "transfer-encoding");
		}
		/**
		 * Дописываем заголовок перед завершающей пустой строкой блока: сериализация
		 * всегда оканчивается ей, но при внутреннем сбое печати блок может прийти
		 * пустым - вставка по несуществующей позиции недопустима
		 */
		if(injectChunked && (block.size() >= 2))
			// Вставляем заголовок перед завершающей пустой строкой блока
			block.insert(block.size() - 2, "Transfer-Encoding: chunked\r\n");
		// Дописываем сериализованный блок заголовков в выходной буфер
		this->_sender.output.push(block);
		// Если объявленное кадрирование chunked требуется завершить нулевым чанком
		if(terminateChunked)
			// Завершаем пустое тело нулевым чанком без блока трейлеров
			this->_sender.output.push("0\r\n\r\n", 5);
		// Помечаем что заголовки сообщения отправлены
		this->_sender.headersSent = true;
		/**
		 * Помечаем сообщение завершённым, если тела не будет: по воле вызывающей
		 * стороны, из-за объявленного нулевого размера тела либо потому что кадрировать
		 * тело нечем и оно отвергнуто. Во всех случаях на проводе уже лежит
		 * законченное сообщение, и состояние отправителя
		 * обязано этому соответствовать - иначе следующий sendHeaders молча вышел бы
		 * по проверке незавершённого предыдущего сообщения
		 */
		this->_sender.endSent = (endStream || bodyRefused || bodyEmpty);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(endStream), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Передаём исходящие байты сетевому слою
	this->flush();
	// Если задан pull-источник данных - запускаем прокачку тела
	this->pumpSource();
}
/**
 * @brief Метод передачи части тела сообщения для отправки (push-модель, bounded buffer)
 *
 * @details Копирует в выходной буфер столько байт, сколько влезает до high-water,
 *          и возвращает это число (0..size). Если вернулось меньше size - буфер
 *          заполнен: приостановите выдачу и дождитесь функции обратного вызова
 *          writable. Кадрирование тела (chunked/identity) парсер применяет сам.
 *
 * @param buffer    буфер данных тела
 * @param size      размер данных тела
 * @param endStream флаг завершения сообщения
 * @return          число принятых байт (0..size)
 *
 */
size_t awh::http::Parser_HTTP::sendData(const void * buffer, const size_t size, const bool endStream) noexcept {
	// Результат работы функции - число принятых байт
	size_t result = 0;
	// Если заголовки ещё не отправлены либо сообщение уже завершено - тело не принимается
	if(!this->_sender.headersSent || this->_sender.endSent)
		// Выводим число принятых байт
		return result;
	// Если сообщение не предполагает тела - данные не принимаются
	if(this->_sender.framing == sender_t::framing_t::NONE)
		// Выводим число принятых байт
		return result;
	/**
	 * Пока pull-источник не исчерпан, тело сообщения формирует он один
	 *
	 * Источник и прямая выдача - взаимоисключающие способы подачи одного и того же
	 * тела, и это зафиксировано в описании dataSource. Вторая выдача вклинила бы
	 * между порциями источника чужие байты: у кадрирования фиксированного размера
	 * они вытеснили бы хвост тела из анонсированного Content-Length, у chunked
	 * встали бы отдельным чанком посреди чужих данных. Вызванная же из самого
	 * источника, она пишет внутрь участка выходного буфера, зарезервированного
	 * под текущую порцию, и разрывает кадрирование - на проводе оказывается
	 * оборванный заголовок чанка. Отказ фиксируется в логе: молчаливая потеря
	 * тела не оставила бы вызывающей стороне ни следа причины
	 */
	if((this->_sender.source != nullptr) && !this->_sender.sourceEof){
		// Записываем сообщение об отброшенной порции тела в лог
		this->_log->print(
			"HTTP/1.x outgoing body is produced by the pull data source: the data passed to sendData has been dropped",
			log_t::flag_t::CRITICAL
		);
		// Выводим число принятых байт
		return result;
	}
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Вычисляем свободное место в выходном буфере до high-water
		const size_t room = ((this->outputPending() < this->_sender.highWater) ? (this->_sender.highWater - this->outputPending()) : 0);
		// Принимаем столько байт, сколько влезает (частичный приём + счётчик)
		result = ::min(size, room);
		// Для тела фиксированного размера ограничиваем приём остатком Content-Length
		if(this->_sender.framing == sender_t::framing_t::IDENTITY)
			// Ограничиваем приём остатком тела до полного Content-Length
			result = ::min(result, static_cast <size_t> (this->_sender.remaining));
		// Если есть что принимать - кадрируем и дописываем данные в выходной буфер
		if(result > 0){
			// Кадрируем и дописываем порцию тела в выходной буфер
			this->frameBody(buffer, result);
			// Для тела фиксированного размера списываем порцию из остатка Content-Length
			if(this->_sender.framing == sender_t::framing_t::IDENTITY)
				// Списываем порцию из остатка тела
				this->_sender.remaining -= static_cast <uint64_t> (result);
		}
		// Признак кадрирования тела фиксированного размера
		const bool identity = (this->_sender.framing == sender_t::framing_t::IDENTITY);
		/**
		 * Тело фиксированного размера завершается строго по исчерпании анонсированного
		 * Content-Length: досрочный endStream отправил бы в сеть усечённое тело, а
		 * получатель остался бы ждать недостающие байты до таймаута
		 */
		if(identity){
			// Если анонсированный размер тела полностью исчерпан
			if(this->_sender.remaining == 0)
				// Завершаем тело исходящего сообщения
				this->finishBody();
			// Если потребитель объявил конец тела, не выдав анонсированный объём
			else if(endStream && (result == size))
				// Записываем сообщение о преждевременном завершении тела в лог
				this->_log->print(
					"HTTP/1.x outgoing body is shorter than the announced Content-Length: %llu byte(s) left, end of stream ignored",
					log_t::flag_t::WARNING,
					static_cast <unsigned long long> (this->_sender.remaining)
				);
		// Для остальных способов кадрирования тело завершается по принятому финальному фрагменту
		} else if(endStream && (result == size))
			// Завершаем тело исходящего сообщения
			this->finishBody();
		// Если выходной буфер поднялся выше low-water - взводим сигнал writable снова
		if(this->outputPending() > this->_sender.lowWater)
			// Взводим сигнал writable для следующего провала буфера
			this->_sender.writableNotified = false;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size, endStream), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Передаём исходящие байты сетевому слою
	this->flush();
	/**
	 * Сигнализируем о готовности принимать данные: в push-модели выходной буфер
	 * опустошается функцией обратного вызова записи, а не выборкой consumePending,
	 * поэтому без этого вызова сигнал writable не поступил бы вовсе и потребитель,
	 * дождавшийся частичного приёма, остался бы ждать его бесконечно
	 */
	this->maybeNotifyWritable();
	// Выводим число принятых байт
	return result;
}
/**
 * @brief Метод получения ещё не отправленных исходящих байтов (pull-модель)
 *
 * @details View действителен до следующего вызова любого метода парсера.
 *          После записи в сокет освободите отправленную часть методом
 *          consumePending(). При установленной функции обратного вызова
 *          записи буфер опустошается автоматически.
 *
 * @return ещё не отправленные исходящие байты (zero-copy view во внутренний буфер)
 *
 */
string_view awh::http::Parser_HTTP::pending() const noexcept {
	// Если ещё не отправленных исходящих байтов нет
	if(this->_sender.output.empty())
		// Выводим пустое представление
		return string_view();
	// Выводим ещё не отправленные исходящие байты
	return string_view(static_cast <const char *> (this->_sender.output.data()), this->_sender.output.size());
}
/**
 * @brief Метод освобождения отправленных байтов из исходящего буфера (амортизированно O(1))
 *
 * @param size число отправленных байт
 *
 */
void awh::http::Parser_HTTP::consumePending(const size_t size) noexcept {
	/**
	 * Освобождаем отданные байты: смартбуфер сдвигает начало полезных данных, а
	 * физическую компактизацию выполняет сам при нехватке места в хвосте
	 */
	this->_sender.output.erase(::min(size, this->_sender.output.size()));
	// Выходной буфер просел - дозагружаем его из pull-источника данных
	if(this->_sender.source != nullptr)
		// Прокачиваем тело из источника данных
		this->pumpSource();
	// В push-модели сигнализируем о готовности принимать данные
	else this->maybeNotifyWritable();
}
/**
 * @brief Метод установки функции обратного вызова для обработки фрагмента тела сообщения
 *
 * @param callback функция обратного вызова для обработки фрагмента тела сообщения
 *
 */
void awh::http::Parser_HTTP::on(data_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова для обработки фрагмента тела сообщения
	this->_callbacks.data = ::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова для обработки фазы разбора HTTP-сообщения
 *
 * @param callback функция обратного вызова для обработки фазы разбора HTTP-сообщения
 *
 */
void awh::http::Parser_HTTP::on(phase_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова для обработки фазы разбора HTTP-сообщения
	this->_callbacks.phase = ::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова для обработки границ чанков
 *
 * @param callback функция обратного вызова для обработки границ чанков
 *
 */
void awh::http::Parser_HTTP::on(chunk_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова для обработки границ чанков
	this->_callbacks.chunk = ::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова записи исходящих байтов в сеть
 *
 * @param callback функция обратного вызова записи исходящих байтов в сеть
 *
 */
void awh::http::Parser_HTTP::on(write_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова записи исходящих байтов в сеть
	this->_callbacks.write = ::move(callback);
	// Передаём накопленные исходящие байты сетевому слою
	this->flush();
}
/**
 * @brief Метод установки функции обратного вызова для обработки заголовков или трейлеров сообщения
 *
 * @param callback функция обратного вызова для обработки заголовков или трейлеров сообщения
 *
 */
void awh::http::Parser_HTTP::on(header_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова для обработки заголовков или трейлеров сообщения
	this->_callbacks.header = ::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова для обработки провайдера заголовков сообщения
 *
 * @param callback функция обратного вызова для обработки провайдера заголовков сообщения
 *
 */
void awh::http::Parser_HTTP::on(provider_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова для обработки провайдера заголовков сообщения
	this->_callbacks.provider = ::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова о готовности принимать данные тела
 *
 * @param callback функция обратного вызова о готовности принимать данные тела
 *
 */
void awh::http::Parser_HTTP::on(writable_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова о готовности принимать данные тела
	this->_callbacks.writable = ::move(callback);
}
/**
 * @brief Конструктор
 *
 * @param direct направление трафика (запрос/ответ)
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 *
 */
awh::http::Parser_HTTP::Parser_HTTP(const direct_t direct, const fmk_t * fmk, const log_t * log) noexcept :
 parser_t(direct, fmk, log), _error(error_t::NONE), _recycled(false),
 _state(static_cast <uint8_t> (S_START)), _method(method_t::NONE) {
	// Устанавливаем объект логирования буферу исходящих байтов
	this->_sender.output.setLogger(log);
	// Устанавливаем объект логирования буферу передачи в сетевой слой
	this->_sender.flushing.setLogger(log);
	// Согласуем лимит памяти смартбуферов с порогами выходного буфера по умолчанию
	this->sendWaterMarks(SEND_HIGH_WATER, SEND_LOW_WATER);
	/**
	 * В зависимости от направления потока данных, формируем объект провайдера заголовков сообщения
	 */
	switch(static_cast <uint8_t> (direct)){
		// Если передан запрос клиента
		case static_cast <uint8_t> (http::direct_t::REQUEST):
			// Формируем объект провайдера заголовков запроса клиента
			this->_message.provider = make_unique <request_t> ();
		break;
		// Если передан ответ сервера
		case static_cast <uint8_t> (http::direct_t::RESPONSE):
			// Формируем объект провайдера заголовков ответа сервера
			this->_message.provider = make_unique <response_t> ();
		break;
	}
}
/**
 * @brief Деструктор
 *
 */
awh::http::Parser_HTTP::~Parser_HTTP() noexcept {}
