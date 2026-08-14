/**
 * @file nwt.hpp
 * @date 2025-10-25
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл модуля определения типов сетевых адресов — класс Network_Types,
 *        распознающий во входной строке URL, домен, IP-адрес, MAC-адрес,
 *        e-mail или путь файловой системы и выполняющий разбор URL-адреса на составные части
 *
 * \~english
 * @brief Header file of the module of the determination of the types of the network addresses — the Network_Types class,
 *        recognizing in an input string a URL, a domain, an IP address, a MAC address,
 *        an e-mail or a path of the file system and performing the parsing of a URL address into its constituent parts
 *
 * \~
 *
 * @copyright Copyright © 2025
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_NWT__
#define __AWH_NWT__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <cstdint>
#include <string_view>
#include <unordered_set>

/**
 * Подключаем заголовочный файл проекта
 */
#include "../sys/global.hpp"

/**
 * \~russian
 * @brief Основное пространство имён
 *
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * \~russian
	 * @brief Прототип класса работы с логами
	 *
	 * \~english
	 * @brief Prototype of the class for working with logs
	 *
	 * \~
	 */
	class Logging;

	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Структура списка параметров URL
	 *
	 * @details Отвечает на вопрос «что это такое», а не «разбери мне вот это».
	 *          На вход подаётся произвольная строка, на выходе - её тип и разобранные
	 *          части. Нужно это там, где вид записи заранее неизвестен: пользователь
	 *          ввёл строку в поле, и она может оказаться адресом сайта, голым доменом,
	 *          адресом IPv4 или IPv6, MAC-адресом или электронной почтой.
	 *
	 *          Этим модуль и отличается от `uri_t`: тот разбирает то, что уже признано
	 *          идентификатором ресурса, и делает это по правилам RFC 3986, а здесь
	 *          сперва определяется сам вид записи. Если разбирается заведомо известный
	 *          URI, обращаться следует к `uri_t` - он строже и полнее
	 *
	 * @note Распознавание домена опирается на список доменных зон, потому что отличить
	 *       домен от чего угодно другого можно только по зоне. Список встроенный, а
	 *       собственные зоны добавляются методами `zone()` и `zones()`: без них строка
	 *       вида `host.local` доменом признана не будет
	 *
	 * @par Пример: определение вида записи
	 *
	 * \~english
	 * @brief Structure of the list of the parameters of a URL
	 * @details Answers the question «what is this», and not «parse this for me».
	 *          An arbitrary string is fed to the input, at the output there is its type and the parsed
	 *          parts. This is needed where the kind of the record is unknown in advance: the user
	 *          has entered a string into a field, and it may turn out to be an address of a site, a bare domain,
	 *          an IPv4 or IPv6 address, a MAC address or an electronic mail.
	 *          By this the module differs from `uri_t`: that one parses what is already recognized as
	 *          an identifier of a resource, and does it by the rules of RFC 3986, and here
	 *          the very kind of the record is determined first. If a knowingly known
	 *          URI is being parsed, one should address `uri_t` — it is stricter and fuller
	 * @note The recognition of a domain relies on the list of the domain zones, because telling
	 *       a domain from anything else is possible only by the zone. The list is a built-in one, and
	 *       one's own zones are added by the `zone()` and `zones()` methods: without them a string
	 *       of the kind `host.local` will not be recognized as a domain
	 * @par Example: the determination of the kind of a record
	 *
	 * \~
	 *
	 * @code{.cpp}
	 * awh::nwt_t nwt(&log);
	 * // Добавляем собственную доменную зону, иначе она останется неизвестной
	 * nwt.zone("local");
	 * const awh::nwt_t::url_t url = nwt.parse("https://user:pass@anyks.com:443/path?query=1#anchor");
	 * // Здесь url.type равен URL, url.host - "anyks.com", url.port - 443,
	 * // url.domain - "com", а url.schema - "https"
	 * if(url.type == awh::nwt_t::types_t::URL)
	 *     connect(url.host, url.port);
	 * @endcode
	 *
	 */
	typedef class __AWH_SHARED_EXPORT__ Network_Types {
		public:
			/**
			 * \~russian
			 * @brief Типы URL-адреса
			 *
			 * @note Отдельного значения для голого домена здесь нет: доменное имя -
			 *       частный случай адреса, и распознаётся оно как `URL`, а зона
			 *       верхнего уровня кладётся в поле `domain`. Так же поступает и адрес
			 *       со схемой: `URL` возвращается и для `anyks.com`, и для
			 *       `https://anyks.com/path`, различаются они заполненностью полей
			 *
			 * @note Значение `NONE` означает, что строку не удалось отнести ни к
			 *       одному из видов, и проверять его следует всегда: разбор в этом
			 *       случае полей не заполняет
			 *
			 * \~english
			 * @brief Types of a URL address
			 * @note There is no separate value for a bare domain here: a domain name is
			 *       a particular case of an address, and it is recognized as a `URL`, and the zone
			 *       of the top level is put into the `domain` field. So does an address
			 *       with a schema as well: `URL` is returned both for `anyks.com`, and for
			 *       `https://anyks.com/path`, they differ by the filledness of the fields
			 * @note The `NONE` value means that the string could not be attributed to
			 *       any of the kinds, and it should always be checked: the parsing in this
			 *       case fills no fields
			 *
			 * \~
			 */
			enum class types_t : uint8_t {
				NONE  = 0x00, // Тип не определён
				MAC   = 0x01, // MAC-адрес
				URL   = 0x02, // URL-адрес
				IPV4  = 0x03, // IPv4-адрес
				IPV6  = 0x04, // IPv6-адрес
				EMAIL = 0x05  // Электронная почта
			};
		public:
			/**
			 * \~russian
			 * @brief Класс URL-адреса
			 *
			 * @details Итог разбора. Какие поля заполнены, зависит от распознанного
			 *          вида записи, и полагаться на заполненность без проверки `type`
			 *          нельзя:
			 *
			 *          | Вид | Что заполняется |
			 *          |---|---|
			 *          | `URL` | `host`, `domain`, а также `schema`, `port`, `path`, `params`, `anchor`, `user`, `pass` - по мере наличия в записи |
			 *          | `IPV4`, `IPV6` | `host` |
			 *          | `MAC` | `host` |
			 *          | `EMAIL` | `user`, `host`, `domain` |
			 *          | `NONE` | ничего |
			 *
			 * @note Нулевой `port` означает, что порт в записи отсутствовал, а не что
			 *       он равен нулю: подстановкой порта по умолчанию для схемы модуль не
			 *       занимается - этим ведает `uri_t`
			 *
			 * \~english
			 * @brief Class of a URL address
			 * @details The result of the parsing. Which fields are filled depends on the recognized
			 *          kind of the record, and relying on the filledness without a check of `type`
			 *          is not allowed:
			 *          | Kind | What is filled |
			 *          |---|---|
			 *          | `URL` | `host`, `domain`, as well as `schema`, `port`, `path`, `params`, `anchor`, `user`, `pass` — as far as they are present in the record |
			 *          | `IPV4`, `IPV6` | `host` |
			 *          | `MAC` | `host` |
			 *          | `EMAIL` | `user`, `host`, `domain` |
			 *          | `NONE` | nothing |
			 * @note A zero `port` means that the port was absent in the record, and not that
			 *       it equals zero: the module does not occupy itself with the substitution of the port by default for
			 *       a schema — `uri_t` is in charge of that
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ URL {
				public:
					types_t type;  // Тип URL-адреса
					uint32_t port; // Порт URL-адреса
					string uri;    // Полный URI-параметры
					string host;   // Хост URL-адреса
					string path;   // Путь URL-адреса
					string user;   // Ник пользователя (для электронной почты)
					string pass;   // Пароль пользователя
					string anchor; // Якорь URL-адреса
					string domain; // Домен верхнего уровня
					string params; // Параметры URL-адреса
					string schema; // Протокол URL-адреса
				public:
					/**
					 * \~russian
					 * @brief Оператор перемещения
					 *
					 * @param url параметры адреса
					 * @return    параметры URL-запроса
					 *
					 * \~english
					 * @brief Move operator
					 * @param url parameters of the address
					 * @return    parameters of the URL request
					 *
					 * \~
					 */
					URL & operator = (URL && url) noexcept;
					/**
					 * \~russian
					 * @brief Оператор присванивания
					 *
					 * @param url параметры адреса
					 * @return    параметры URL-запроса
					 *
					 * \~english
					 * @brief Assignment operator
					 * @param url parameters of the address
					 * @return    parameters of the URL request
					 *
					 * \~
					 */
					URL & operator = (const URL & url) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Оператор сравнения
					 *
					 * @param url параметры адреса
					 * @return    результат сравнения
					 *
					 * \~english
					 * @brief Comparison operator
					 * @param url parameters of the address
					 * @return    result of the comparison
					 *
					 * \~
					 */
					bool operator == (const URL & url) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Конструктор перемещения
					 *
					 * @param url параметры адреса
					 *
					 * \~english
					 * @brief Move constructor
					 * @param url parameters of the address
					 *
					 * \~
					 */
					URL(URL && url) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор копирования
					 *
					 * @param url параметры адреса
					 *
					 * \~english
					 * @brief Copy constructor
					 * @param url parameters of the address
					 *
					 * \~
					 */
					URL(const URL & url) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit URL() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					~URL() noexcept = default;
			} url_t;
		private:
			// Список пользовательских доменных зон интернета
			unordered_set <string> _user;
		private:
			// Объект логера
			const Logging * _log;
		private:
			/**
			 * \~russian
			 * @brief Метод проверки, является ли домен верхнего уровня известной доменной зоной
			 *
			 * @param domain домен верхнего уровня для проверки
			 * @return       результат проверки (true, если зона известна)
			 *
			 * \~english
			 * @brief Method of checking whether a top level domain is a known domain zone
			 * @param domain top level domain to check
			 * @return       result of the check (true, if the zone is known)
			 *
			 * \~
			 */
			bool isZone(const string & domain) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод очистки результатов парсинга
			 *
			 * \~english
			 * @brief Method of clearing the results of the parsing
			 *
			 * \~
			 */
			void clear() noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки пользовательской зоны
			 *
			 * @details Добавляет доменную зону к встроенному списку. Отличить домен от
			 *          произвольной строки можно только по зоне верхнего уровня,
			 *          поэтому запись с неизвестной зоной доменом признана не будет:
			 *          внутрисетевые имена вроде `host.local` или `service.internal`
			 *          требуют явного добавления
			 *
			 * @note Зоны накапливаются, а не заменяют друг друга: каждый вызов
			 *       добавляет одну. Заменить весь список целиком позволяет перегрузка
			 *       `zones()`
			 *
			 * @param zone пользовательская зона
			 *
			 * \~english
			 * @brief Method of setting a user zone
			 * @details Adds a domain zone to the built-in list. Telling a domain from
			 *          an arbitrary string is possible only by the zone of the top level,
			 *          and therefore a record with an unknown zone will not be recognized as a domain:
			 *          the intranet names of the kind `host.local` or `service.internal`
			 *          require an explicit addition
			 * @note The zones accumulate, and do not replace each other: every call
			 *       adds one. Replacing the whole list entirely is made possible by the
			 *       `zones()` overload
			 * @param zone user zone
			 *
			 * \~
			 */
			void zone(string_view zone) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод извлечения списка пользовательских зон интернета
			 *
			 * \~english
			 * @brief Method of extracting the list of the user zones of the internet
			 *
			 * \~
			 */
			const unordered_set <string> & zones() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки списка пользовательских зон
			 *
			 * @param zones список доменных зон интернета
			 *
			 * \~english
			 * @brief Method of setting the list of the user zones
			 * @param zones list of the domain zones of the internet
			 *
			 * \~
			 */
			void zones(const unordered_set <string> & zones) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод парсинга URI-строки
			 *
			 * @details Определяет вид записи и разбирает её на части. Метод постоянный
			 *          и состояния объекта не меняет, поэтому один разборщик можно
			 *          держать на всё приложение и обращаться к нему откуда угодно -
			 *          при условии, что список пользовательских зон уже задан
			 *
			 * @note Итог возвращается значением, а не ссылкой на внутреннее состояние:
			 *          результаты разных разборов друг друга не затирают
			 *
			 * @note Вид записи следует проверять всегда. Строка, не отнесённая ни к
			 *       одному виду, вернётся с типом `NONE` и пустыми полями, и обращение
			 *       к ним без проверки даст не ошибку, а тихо неверное поведение
			 *
			 * @param text текст для парсинга
			 * @return     параметры полученные в результате парсинга
			 *
			 * \~english
			 * @brief Method of parsing a URI string
			 * @details Determines the kind of the record and parses it into the parts. The method is a constant one
			 *          and does not change the state of the object, and therefore one parser can be
			 *          held for the whole application and addressed from anywhere —
			 *          provided that the list of the user zones is already set
			 * @note The result is returned by value, and not by a reference to the internal state:
			 *          the results of different parsings do not overwrite each other
			 * @note The kind of the record should always be checked. A string not attributed to
			 *       any kind will be returned with the type `NONE` and with the empty fields, and an address
			 *       to them without a check will give not an error, but a quietly wrong behaviour
			 * @param text text to parse
			 * @return     parameters obtained as the result of the parsing
			 *
			 * \~
			 */
			url_t parse(string_view text) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки объекта логирования
			 *
			 * @param log объект работы с логами
			 *
			 * \~english
			 * @brief Method of setting the logging object
			 * @param log object for working with logs
			 *
			 * \~
			 */
			void setLogger(const Logging * log) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * \~
			 */
			explicit Network_Types() noexcept;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param log объект для работы с логами
			 *
			 * \~english
			 * @brief Constructor
			 * @param log object for working with logs
			 *
			 * \~
			 */
			explicit Network_Types(const Logging * log) noexcept;
			/**
			 * \~russian
			 * @brief Деструктор
			 *
			 *
			 * \~english
			 * @brief Destructor
			 *
			 * \~
			 */
			~Network_Types() noexcept = default;
	} nwt_t;
};

#endif // __AWH_NWT__
