/**
 * @file: provider.hpp
 * @date: 2026-07-08
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл провайдеров HTTP-сообщений — классы Provider, Request и Response,
 *        формирующие и хранящие структуру HTTP-запроса клиента и HTTP-ответа сервера поверх контейнера заголовков
 *
 * \~english
 * @brief Header file of the providers of the HTTP messages — the classes Provider, Request and Response
 *        forming and storing the structure of an HTTP request of a client and of an HTTP answer of a server over the container of the headers
 *
 * \~
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_HTTP_PROVIDER__
#define __AWH_HTTP_PROVIDER__

/**
 * Стандартные заголовочные файлы
 */
#include <memory>
#include <string>
#include <string_view>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "http.hpp"
#include "../../sys/global.hpp"

/**
 * \~russian
 * @brief основное пространство имён
 *
 *
 * \~english
 * @brief main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Пространство имён HTTP-протокола
	 *
	 *
	 * \~english
	 * @brief HTTP protocol namespace
	 *
	 * \~
	 */
	namespace http {
		/**
		 * \~russian
		 * @brief Класс провайдера
		 *
		 * \~english
		 * @brief Class of the provider
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Provider {
			public:
				// Версия протокола
				version_t version;
				// Направление трафика (запрос/ответ)
				const direct_t direct;
			public:
				/**
				 * \~russian
				 * @brief Метод клонирования объекта провайдера
				 *
				 * @details Обеспечивает корректное копирование производного объекта (Request/Response)
				 *          через указатель на базовый класс без срезки (object slicing)
				 *
				 * @return копия объекта провайдера
				 *
				 * \~english
				 * @brief Method of cloning the object of the provider
				 * @details Ensures a correct copying of a derived object (Request/Response)
				 *          through a pointer to the base class without a slicing (object slicing)
				 * @return copy of the object of the provider
				 *
				 * \~
				 */
				virtual unique_ptr <Provider> clone() const noexcept = 0;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param direct направление трафика (запрос/ответ)
				 *
				 * \~english
				 * @brief Constructor
				 * @param direct direction of the traffic (a request/an answer)
				 *
				 * \~
				 */
				explicit Provider(const direct_t direct) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param direct  направление трафика (запрос/ответ)
				 * @param version версия протокола
				 *
				 * \~english
				 * @brief Constructor
				 * @param direct  direction of the traffic (a request/an answer)
				 * @param version version of the protocol
				 *
				 * \~
				 */
				explicit Provider(const direct_t direct, const version_t version) noexcept;
			public:
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
				virtual ~Provider() noexcept = default;
		} provider_t;

		/**
		 * \~russian
		 * @brief Класс HTTP-запроса клиента
		 *
		 * \~english
		 * @brief Class of an HTTP request of a client
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Request : public provider_t {
			public:
				// Параметры URI-запроса
				string uri;
				// Метод запроса клиента
				method_t method;
				// Оригинальное написание метода запроса (заполняется ТОЛЬКО для method_t::UNKNOWN - прозрачное проксирование экзотических методов)
				string methodName;
				/**
				 * Протокол расширенного метода CONNECT (псевдо-заголовок [:protocol], RFC 8441).
				 * Заполняется ТОЛЬКО для HTTP/2 и HTTP/3: именно так поверх CONNECT
				 * поднимается WebSocket. Для остальных запросов остаётся пустым
				 */
				string protocol;
			public:
				/**
				 * \~russian
				 * @brief Метод клонирования объекта запроса
				 *
				 * @return копия объекта запроса
				 *
				 * \~english
				 * @brief Method of cloning the object of the request
				 * @return copy of the object of the request
				 *
				 * \~
				 */
				unique_ptr <provider_t> clone() const noexcept override;
			public:
				/**
				 * \~russian
				 * @brief Оператор [=] перемещения параметров запроса клиента
				 *
				 * @param request объект параметров запроса клиента
				 * @return        текущие параметры запроса клиента
				 *
				 * \~english
				 * @brief Operator [=] of the moving of the parameters of a request of a client
				 * @param request object of the parameters of the request of the client
				 * @return        current parameters of the request of the client
				 *
				 * \~
				 */
				Request & operator = (Request && request) noexcept;
				/**
				 * \~russian
				 * @brief Оператор [=] присванивания параметров запроса клиента
				 *
				 * @param request объект параметров запроса клиента
				 * @return        текущие параметры запроса клиента
				 *
				 * \~english
				 * @brief Operator [=] of the assignment of the parameters of a request of a client
				 * @param request object of the parameters of the request of the client
				 * @return        current parameters of the request of the client
				 *
				 * \~
				 */
				Request & operator = (const Request & request) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Оператор [==] сравнения
				 *
				 * @param request объект параметров запроса клиента
				 * @return        результат сравнения
				 *
				 * \~english
				 * @brief Operator [==] of a comparison
				 * @param request object of the parameters of the request of the client
				 * @return        result of the comparison
				 *
				 * \~
				 */
				bool operator == (const Request & request) const noexcept;
				/**
				 * \~russian
				 * @brief Оператор [!=] сравнения
				 *
				 * @param request объект параметров запроса клиента
				 * @return        результат сравнения
				 *
				 * \~english
				 * @brief Operator [!=] of a comparison
				 * @param request object of the parameters of the request of the client
				 * @return        result of the comparison
				 *
				 * \~
				 */
				bool operator != (const Request & request) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Конструктор перемещения
				 *
				 * @param request объект параметров запроса клиента
				 *
				 * \~english
				 * @brief Constructor of the moving
				 * @param request object of the parameters of the request of the client
				 *
				 * \~
				 */
				Request(Request && request) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор копирования
				 *
				 * @param request объект параметров запроса клиента
				 *
				 * \~english
				 * @brief Constructor of the copying
				 * @param request object of the parameters of the request of the client
				 *
				 * \~
				 */
				Request(const Request & request) noexcept;
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
				explicit Request() noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param uri параметры URI-запроса
				 *
				 * \~english
				 * @brief Constructor
				 * @param uri parameters of the URI request
				 *
				 * \~
				 */
				explicit Request(const string & uri) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param method метод запроса клиента
				 *
				 * \~english
				 * @brief Constructor
				 * @param method method of the request of the client
				 *
				 * \~
				 */
				explicit Request(const method_t method) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param version версия протокола
				 *
				 * \~english
				 * @brief Constructor
				 * @param version version of the protocol
				 *
				 * \~
				 */
				explicit Request(const version_t version) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param method метод запроса клиента
				 * @param uri    параметры URI-запроса
				 *
				 * \~english
				 * @brief Constructor
				 * @param method method of the request of the client
				 * @param uri    parameters of the URI request
				 *
				 * \~
				 */
				explicit Request(const method_t method, const string & uri) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param version версия протокола
				 * @param uri     параметры URI-запроса
				 *
				 * \~english
				 * @brief Constructor
				 * @param version version of the protocol
				 * @param uri     parameters of the URI request
				 *
				 * \~
				 */
				explicit Request(const version_t version, const string & uri) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param version версия протокола
				 * @param method  метод запроса клиента
				 *
				 * \~english
				 * @brief Constructor
				 * @param version version of the protocol
				 * @param method  method of the request of the client
				 *
				 * \~
				 */
				explicit Request(const version_t version, const method_t method) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param version версия протокола
				 * @param method  метод запроса клиента
				 * @param uri     параметры URI-запроса
				 *
				 * \~english
				 * @brief Constructor
				 * @param version version of the protocol
				 * @param method  method of the request of the client
				 * @param uri     parameters of the URI request
				 *
				 * \~
				 */
				explicit Request(const version_t version, const method_t method, const string & uri) noexcept;
			public:
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
				~Request() noexcept = default;
		} request_t;

		/**
		 * \~russian
		 * @brief Класс HTTP-ответа сервера
		 *
		 * \~english
		 * @brief Class of an HTTP answer of a server
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Response : public provider_t {
			public:
				// Код ответа сервера
				uint16_t code;
				// Сообщение сервера
				string message;
			public:
				/**
				 * \~russian
				 * @brief Метод клонирования объекта ответа
				 *
				 * @return копия объекта ответа
				 *
				 * \~english
				 * @brief Method of cloning the object of the answer
				 * @return copy of the object of the answer
				 *
				 * \~
				 */
				unique_ptr <provider_t> clone() const noexcept override;
			public:
				/**
				 * \~russian
				 * @brief Оператор [=] перемещения параметров ответа сервера
				 *
				 * @param response объект параметров ответа сервера
				 * @return         текущие параметры ответа сервера
				 *
				 * \~english
				 * @brief Operator [=] of the moving of the parameters of an answer of a server
				 * @param response object of the parameters of the answer of the server
				 * @return         current parameters of the answer of the server
				 *
				 * \~
				 */
				Response & operator = (Response && response) noexcept;
				/**
				 * \~russian
				 * @brief Оператор [=] присванивания параметров ответа сервера
				 *
				 * @param response объект параметров ответа сервера
				 * @return         текущие параметры ответа сервера
				 *
				 * \~english
				 * @brief Operator [=] of the assignment of the parameters of an answer of a server
				 * @param response object of the parameters of the answer of the server
				 * @return         current parameters of the answer of the server
				 *
				 * \~
				 */
				Response & operator = (const Response & response) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Оператор [==] сравнения
				 *
				 * @param response объект параметров ответа сервера
				 * @return         результат сравнения
				 *
				 * \~english
				 * @brief Operator [==] of a comparison
				 * @param response object of the parameters of the answer of the server
				 * @return         result of the comparison
				 *
				 * \~
				 */
				bool operator == (const Response & response) const noexcept;
				/**
				 * \~russian
				 * @brief Оператор [!=] сравнения
				 *
				 * @param response объект параметров ответа сервера
				 * @return         результат сравнения
				 *
				 * \~english
				 * @brief Operator [!=] of a comparison
				 * @param response object of the parameters of the answer of the server
				 * @return         result of the comparison
				 *
				 * \~
				 */
				bool operator != (const Response & response) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Конструктор перемещения
				 *
				 * @param response объект параметров ответа сервера
				 *
				 * \~english
				 * @brief Constructor of the moving
				 * @param response object of the parameters of the answer of the server
				 *
				 * \~
				 */
				Response(Response && response) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор копирования
				 *
				 * @param response объект параметров ответа сервера
				 *
				 * \~english
				 * @brief Constructor of the copying
				 * @param response object of the parameters of the answer of the server
				 *
				 * \~
				 */
				Response(const Response & response) noexcept;
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
				explicit Response() noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param code код ответа сервера
				 *
				 * \~english
				 * @brief Constructor
				 * @param code code of the answer of the server
				 *
				 * \~
				 */
				explicit Response(const uint16_t code) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param message сообщение сервера
				 *
				 * \~english
				 * @brief Constructor
				 * @param message message of the server
				 *
				 * \~
				 */
				explicit Response(const string & message) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param version версия протокола
				 *
				 * \~english
				 * @brief Constructor
				 * @param version version of the protocol
				 *
				 * \~
				 */
				explicit Response(const version_t version) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param code    код ответа сервера
				 * @param message сообщение сервера
				 *
				 * \~english
				 * @brief Constructor
				 * @param code    code of the answer of the server
				 * @param message message of the server
				 *
				 * \~
				 */
				explicit Response(const uint16_t code, const string & message) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param version версия протокола
				 * @param code    код ответа сервера
				 *
				 * \~english
				 * @brief Constructor
				 * @param version version of the protocol
				 * @param code    code of the answer of the server
				 *
				 * \~
				 */
				explicit Response(const version_t version, const uint16_t code) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param version версия протокола
				 * @param message сообщение сервера
				 *
				 * \~english
				 * @brief Constructor
				 * @param version version of the protocol
				 * @param message message of the server
				 *
				 * \~
				 */
				explicit Response(const version_t version, const string & message) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param version версия протокола
				 * @param code    код ответа сервера
				 * @param message сообщение сервера
				 *
				 * \~english
				 * @brief Constructor
				 * @param version version of the protocol
				 * @param code    code of the answer of the server
				 * @param message message of the server
				 *
				 * \~
				 */
				explicit Response(const version_t version, const uint16_t code, const string & message) noexcept;
			public:
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
				~Response() noexcept = default;
		} response_t;

		/**
		 * \~russian
		 * @brief Функция получения стандартного сообщения HTTP-ответа по коду
		 *
		 * @details Таблица сообщений хранится как локальная static constexpr-таблица указателей на строковые
		 *          литералы: данные размещаются в .rodata, без конструкторов при старте программы и без
		 *          динамических аллокаций. Функция помечена inline, поэтому во всей программе существует
		 *          единственный экземпляр таблицы (без inline-переменных, требующих C++17).
		 *
		 * @param code код ответа сервера
		 * @return     стандартное сообщение либо пустое представление, если код неизвестен
		 *
		 * \~english
		 * @brief Function of getting the standard message of an HTTP answer by a code
		 * @details The table of the messages is stored as a local static constexpr table of the pointers to the string
		 *          literals: the data is placed in .rodata, without the constructors at the start of the program and without
		 *          the dynamic allocations. The function is marked inline, therefore in the whole program there exists
		 *          a single copy of the table (without the inline variables requiring C++17).
		 * @param code code of the answer of the server
		 * @return     standard message or an empty representation, if the code is unknown
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ string_view statusMessage(const uint16_t code) noexcept;
		/**
		 * \~russian
		 * @brief Функция классификации метода запроса по значению псевдо-заголовка [:method]
		 *
		 * @note В отличие от HTTP/1.x сравнение выполняется с учётом регистра:
		 *       методы HTTP - регистрозависимые токены (RFC 9110 §9.1).
		 *
		 * @param method значение псевдо-заголовка [:method]
		 * @return       распознанный метод запроса либо method_t::NONE
		 *
		 * \~english
		 * @brief Function of the classification of a method of a request by the value of the pseudo header [:method]
		 * @note Unlike HTTP/1.x the comparison is performed with the account of the case:
		 *       the methods of HTTP are case-dependent tokens (RFC 9110 §9.1).
		 * @param method value of the pseudo header [:method]
		 * @return       recognized method of the request or method_t::NONE
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ http::method_t classifyMethod(string_view method) noexcept;
		/**
		 * \~russian
		 * @brief Функция получения имени метода запроса для псевдо-заголовка [:method]
		 *
		 * @param request провайдер запроса клиента
		 * @return        имя метода запроса (для UNKNOWN - оригинальное написание метода)
		 *
		 * \~english
		 * @brief Function of getting the name of a method of a request for the pseudo header [:method]
		 * @param request provider of the request of the client
		 * @return        name of the method of the request (for UNKNOWN - the original spelling of the method)
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ string_view methodName(const http::request_t * request) noexcept;
		/**
		 * \~russian
		 * @brief Функция приведения пути цели запроса к виду псевдо-заголовка [:path]
		 *
		 * @details Псевдо-заголовок пути не бывает пустым и обязан начинаться с '/'
		 *          (RFC 9113 §8.3.1, RFC 9114 §4.3.1). Буфер задействуется только
		 *          когда путь требует дополнения, то есть в редком случае цели вида
		 *          [https://example.com?q=1]; в остальных выдаётся представление
		 *          на исходную строку без копирования.
		 *
		 *          Звёздочная форма цели выдаётся без изменений: ею метод OPTIONS
		 *          обращается к серверу целиком, а не к его ресурсу (RFC 9112 §3.2.4),
		 *          и псевдо-заголовок пути в таком запросе обязан нести именно её.
		 *
		 * @param path   путь цели запроса
		 * @param buffer буфер под дополненный путь
		 * @return       путь, пригодный для псевдо-заголовка
		 *
		 * \~english
		 * @brief Function of bringing the path of the target of a request to the form of the pseudo header [:path]
		 * @details The pseudo header of the path is never empty and is obliged to begin with '/'
		 *          (RFC 9113 §8.3.1, RFC 9114 §4.3.1). The buffer is engaged only
		 *          when the path requires a supplementation, that is in the rare case of a target of the form
		 *          [https://example.com?q=1]; in the rest a representation
		 *          onto the source string is issued without a copying.
		 *          The asterisk form of a target is issued without changes: by it the method OPTIONS
		 *          addresses the server as a whole rather than its resource (RFC 9112 §3.2.4),
		 *          and the pseudo header of the path in such a request is obliged to carry exactly it.
		 * @param path   path of the target of the request
		 * @param buffer buffer for the supplemented path
		 * @return       path suitable for the pseudo header
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ string_view targetPath(const string_view path, string & buffer) noexcept;
		/**
		 * \~russian
		 * @brief Функция разбора цели запроса, заданной в абсолютной форме (RFC 9112 §3.2.2)
		 *
		 * @details Абсолютная форма цели законна в HTTP/1 и обязательна в запросе
		 *          к прокси, а провайдер запроса общий для всех протоколов. Протоколы
		 *          же, работающие псевдо-заголовками, требуют разнесённых схемы,
		 *          авторитета и пути (RFC 9113 §8.3.1, RFC 9114 §4.3.1), поэтому цель
		 *          перед отправкой обязана быть разобрана.
		 *
		 *          Конец компонента авторитета - первый из символов '/', '?' и '#'
		 *          (RFC 3986 §3.2), а не один только '/': иначе строка запроса цели
		 *          без пути уехала бы внутрь авторитета.
		 *
		 *          Выданный путь может оказаться пустым либо начинаться со строки
		 *          запроса - привести его к виду псевдо-заголовка позволяет targetPath().
		 *
		 * @param target    цель запроса
		 * @param scheme    схема цели (представление в target)
		 * @param authority авторитет цели (представление в target)
		 * @param path      путь цели вместе со строкой запроса (представление в target)
		 * @return          признак того, что цель задана в абсолютной форме
		 *
		 * \~english
		 * @brief Function of parsing the target of a request given in the absolute form (RFC 9112 §3.2.2)
		 * @details The absolute form of a target is lawful in HTTP/1 and is obligatory in a request
		 *          to a proxy, while the provider of a request is common for all the protocols. And the protocols
		 *          working by the pseudo headers require a separated scheme,
		 *          authority and path (RFC 9113 §8.3.1, RFC 9114 §4.3.1), therefore the target
		 *          before the sending is obliged to be parsed.
		 *          The end of the component of the authority is the first of the characters '/', '?' and '#'
		 *          (RFC 3986 §3.2) rather than '/' alone: otherwise the query string of a target
		 *          without a path would go away inside the authority.
		 *          The issued path may turn out to be empty or begin with the query
		 *          string - to bring it to the form of the pseudo header is allowed by targetPath().
		 * @param target    target of the request
		 * @param scheme    scheme of the target (a representation in target)
		 * @param authority authority of the target (a representation in target)
		 * @param path      path of the target together with the query string (a representation in target)
		 * @return          flag of the target being given in the absolute form
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool splitTarget(const string_view target, string_view & scheme, string_view & authority, string_view & path) noexcept;
    };
};

#endif // __AWH_HTTP_PROVIDER__
