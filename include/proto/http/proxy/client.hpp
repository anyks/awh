/**
 * @file client.hpp
 * @date 2026-11-23
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
 * @brief Заголовочный файл клиентской стороны протокола HTTP-прокси — класс Client,
 *        формирующий запросы CONNECT и absolute-form, разбирающий ответы прокси-сервера
 *        и выполняющий авторизацию на прокси (Basic/Digest/Bearer) поверх парсера HTTP/1.x
 *
 * \~english
 * @brief Header file of the client side of the HTTP proxy protocol — the Client class,
 *        which forms the requests of CONNECT and absolute-form, parses the answers of the proxy server
 *        and performs the authorization on the proxy (Basic/Digest/Bearer) on top of the parser of HTTP/1.x
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#pragma once

/**
 * Стандартные заголовочные файлы
 */
#include <deque>
#include <string>
#include <vector>
#include <utility>
#include <cstddef>
#include <cstdint>
#include <functional>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../http.hpp"
#include "../headers.hpp"
#include "../auth/auth.hpp"
#include "../parser/http1/http.hpp"
#include "../../../net/net.hpp"
#include "../../../net/addr.hpp"
#include "../../../sys/macro/global.hpp"

/**
 * \~russian
 * @brief основное пространство имён
 *
 * \~english
 * @brief main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Пространство имён HTTP-протокола
	 *
	 * \~english
	 * @brief HTTP protocol namespace
	 *
	 * \~
	 */
	namespace http {
		/**
		 * \~russian
		 * @brief Пространство имён прокси-клиента
		 *
		 * \~english
		 * @brief Proxy client namespace
		 *
		 * \~
		 */
		namespace proxy {
			/**
			 * \~russian
			 * @brief Класс клиента для работы с HTTP-прокси
			 *
			 * @details Инкапсулирует обмен с прокси-сервером поверх HTTP/1.x: установку
			 *          туннеля методом CONNECT (RFC 9110 §9.3.6) и пересылку обычных
			 *          HTTP-запросов в absolute-form. Ответы прокси разбираются парсером
			 *          HTTP/1.x, авторизация на прокси выполняется модулем авторизации
			 *          в режиме прокси (Proxy-Authorization/Proxy-Authenticate).
			 *
			 * \~english
			 * @brief Client class for working with an HTTP proxy
			 *
			 * @details Encapsulates the exchange with a proxy server over HTTP/1.x: the establishment
			 *          of a tunnel by the method CONNECT (RFC 9110 §9.3.6) and the forwarding of ordinary
			 *          HTTP requests in absolute-form. The answers of the proxy are parsed by the parser
			 *          of HTTP/1.x, the authorization on the proxy is performed by the module of the authorization
			 *          in the mode of the proxy (Proxy-Authorization/Proxy-Authenticate).
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Client {
				public:
					/**
					 * \~russian
					 * @brief Максимальное число повторов авторизации по ответу 407
					 *
					 * \~english
					 * @brief Maximum number of the retries of the authorization on an answer 407
					 *
					 * \~
					 */
					static constexpr uint8_t HTTP_PROXY_AUTH_RETRIES = 1;
					/**
					 * \~russian
					 * @brief Максимальный суммарный размер заголовков ответа прокси
					 *
					 * \~english
					 * @brief Maximum total size of the headers of an answer of the proxy
					 *
					 * \~
					 */
					static constexpr size_t HTTP_PROXY_RX_MAX_HEADERS = (16 * 1024);
					/**
					 * \~russian
					 * @brief Ёмкость выходного буфера отправки запроса прокси
					 *
					 * @details Запрос пользователя в режиме FORWARD вместе с телом
					 *          должен помещаться в этот буфер целиком.
					 *
					 * \~english
					 * @brief Capacity of the output buffer of the sending of a request to the proxy
					 *
					 * @details A request of the user in the mode FORWARD together with the body
					 *          must fit into this buffer wholly.
					 *
					 * \~
					 */
					static constexpr size_t HTTP_PROXY_TX_HIGH_WATER = (1024 * 1024);
				public:
					/**
					 * \~russian
					 * @brief Режимы работы клиента через прокси
					 *
					 * \~english
					 * @brief Modes of the work of the client through a proxy
					 *
					 * \~
					 */
					enum class mode_t : uint8_t {
						TUNNEL  = 0x00, // Туннель методом CONNECT (RFC 9110 §9.3.6)
						FORWARD = 0x01  // Пересылка обычных HTTP-запросов в absolute-form
					};
					/**
					 * \~russian
					 * @brief Стейты работы модуля
					 *
					 * \~english
					 * @brief States of the work of the module
					 *
					 * \~
					 */
					enum class state_t : uint8_t {
						NONE      = 0x00, // Состояние не определено
						REQUEST   = 0x01, // Запрос отправлен, ожидается ответ прокси
						AUTH      = 0x02, // Получен ответ 407, требуется авторизация на прокси
						HANDSHAKE = 0x03, // Получен успешный ответ, туннель установлен
						COMPLETED = 0x04, // Обмен завершён (туннель активен либо ответ доставлен)
						BROKEN    = 0x05  // Бракованные данные либо ошибка обмена
					};
				public:
					/**
					 * \~russian
					 * @brief Функция обратного вызова готовности трейлеров ответа прокси
					 *
					 * @details Вызывается по завершении разбора финального ответа прокси,
					 *          если ответ содержит трейлеры: собранные трейлеры выдаются
					 *          после тела ответа. Ответы без трейлеров вызова не порождают.
					 *
					 * \~english
					 * @brief Callback function of the readiness of the trailers of an answer of the proxy
					 *
					 * @details Called on the completion of the parsing of the final answer of the proxy
					 *          if the answer contains trailers: the collected trailers are issued
					 *          after the body of the answer. The answers without trailers do not generate a call.
					 *
					 * \~
					 */
					using trailer_callback_t = function <void (const vector <pair <string, string>> &)>;
					/**
					 * \~russian
					 * @brief Функция обратного вызова готовности головы ответа прокси
					 *
					 * @details Вызывается по завершении разбора заголовков финального
					 *          ответа прокси: код состояния и собранные заголовки.
					 *          Промежуточные ответы 1xx и ответ 407, по которому выполняется
					 *          повторная авторизация, не выдаются.
					 *
					 * \~english
					 * @brief Callback function of the readiness of the head of an answer of the proxy
					 *
					 * @details Called on the completion of the parsing of the headers of the final
					 *          answer of the proxy: the status code and the collected headers.
					 *          The intermediate answers 1xx and an answer 407 on which the repeated
					 *          authorization is performed are not issued.
					 *
					 * \~
					 */
					using response_callback_t = function <void (const uint16_t, const vector <pair <string, string>> &)>;
				public:
					/**
					 * \~russian
					 * @brief Структура промежуточного контекста
					 *
					 * \~english
					 * @brief Structure of the intermediate context
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Context {
						// Режим работы клиента через прокси
						mode_t mode;
						// Текущее состояние
						state_t state;
						// Код состояния HTTP-ответа прокси
						uint16_t code;
						// Число выполненных повторов авторизации
						uint8_t retries;
						// Параметры адреса целевого хоста
						unique_ptr <net::attr_t> host;
						// Заголовки ответа прокси
						vector <pair <string, string>> headers;
						// Трейлеры ответа прокси
						vector <pair <string, string>> trailers;
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * \~english
						 * @brief Constructor
						 *
						 * \~
						 */
						explicit Context() noexcept;
					} ctx_t;
				private:
					/**
					 * \~russian
					 * @brief Структура пересылаемого запроса в очереди конвейера
					 *
					 * @details Хранит параметры запроса для повторной сборки при
					 *          авторизации по ответу 407 и сформированные байты запроса.
					 *          Ответы прокси сопоставляются запросам очереди в порядке FIFO.
					 *
					 * \~english
					 * @brief Structure of a forwarded request in the pipeline queue
					 *
					 * @details Stores the parameters of the request for the repeated assembly on
					 *          the authorization by an answer 407 and the formed octets of the request.
					 *          The answers of the proxy are matched to the requests of the queue in the order FIFO.
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Forward_Request {
						// Флаг отправки запроса прокси-серверу
						bool sent;
						// Число выполненных повторов авторизации запроса
						uint8_t retries;
						// Метод запроса (для кадрирования тела ответа)
						method_t method;
						// Цель URI запроса в absolute-form
						string uri;
						// Тело запроса
						string body;
						// Сформированные байты запроса к прокси
						string wire;
						// Оригинальное написание метода запроса
						string methodName;
						// Заголовки запроса
						vector <pair <string, string>> fields;
						// Трейлеры запроса
						vector <pair <string, string>> trailers;
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * \~english
						 * @brief Constructor
						 *
						 * \~
						 */
						explicit Forward_Request() noexcept;
					} forward_request_t;
				private:
					// Флаг подавления выдачи тела ответа вызывающей стороне
					bool _suppress;
					// Флаг превентивной отправки учётных данных авторизации
					bool _preemptive;
					// Флаг состоявшегося вызова авторизации прокси в соединении
					bool _challenged;
				private:
					// Объект авторизации в режиме прокси
					auth_t _auth;
					// Метод пересылаемого запроса
					method_t _method;
				private:
					// Объект сетевых адресов для преобразования адресов
					net_addr_t _addr;
				private:
					// Объект парсера ответов прокси-сервера
					parser_http_t _rx;
					// Объект парсера и отправителя запросов
					parser_http_t _tx;
				private:
					// Цель URI пересылаемого запроса
					string _uri;
					// Тело пересылаемого запроса
					string _body;
					// Токен доступа для BEARER-аутентификации
					string _token;
					// Имя пользователя для аутентификации
					string _username;
					// Пароль пользователя для аутентификации
					string _password;
					// Оригинальное написание метода пересылаемого запроса
					string _methodName;
				private:
					// Активный контекст на время разбора ответа
					ctx_t * _ctx;
				private:
					// Функция обратного вызова готовности трейлеров ответа прокси
					trailer_callback_t _trailer;
					// Функция обратного вызова готовности головы ответа прокси
					response_callback_t _response;
					// Функция обратного вызова обработки тела пересылаемого ответа
					parser_http_t::data_callback_t _data;
				private:
					// Очередь пересылаемых запросов конвейера (FIFO)
					deque <forward_request_t> _queue;
				private:
					// Заголовки пересылаемого запроса
					vector <pair <string, string>> _fields;
					// Трейлеры пересылаемого запроса
					vector <pair <string, string>> _trailers;
				private:
					/**
					 * \~russian
					 * @brief Метод настройки парсеров и подписки на их события
					 *
					 * @details Применяет протокол PROXY1, лимиты заголовков и функции
					 *          обратного вызова разбора. Вызывается конструктором
					 *          и после полной очистки парсеров.
					 *
					 * \~english
					 * @brief Method of the configuration of the parsers and of the subscription to their events
					 *
					 * @details Applies the protocol PROXY1, the limits of the headers and the callback
					 *          functions of the parsing. Called by the constructor
					 *          and after a full clearing of the parsers.
					 *
					 * \~
					 */
					void setup() noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод постановки разобранного запроса в очередь конвейера
					 *
					 * @details Формирует цель в absolute-form из разобранного запроса
					 *          пользователя, собирает байты запроса и ставит его в очередь.
					 *
					 * @param ctx объект контекста для установки параметров сообщения
					 * @return    результат постановки запроса в очередь
					 *
					 * \~english
					 * @brief Method of the placing of a parsed request into the queue of the pipeline
					 *
					 * @details Forms the target in absolute-form from the parsed request
					 *          of the user, assembles the octets of the request and places it into the queue.
					 *
					 * @param ctx object of the context for the setting of the parameters of the message
					 * @return    result of the placing of the request into the queue
					 *
					 * \~
					 */
					bool enqueue(ctx_t & ctx) noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки запроса CONNECT
					 *
					 * @param ctx объект контекста для установки параметров сообщения
					 * @return    результат сборки запроса
					 *
					 * \~english
					 * @brief Method of the assembly of a request CONNECT
					 *
					 * @param ctx object of the context for the setting of the parameters of the message
					 * @return    result of the assembly of the request
					 *
					 * \~
					 */
					bool connect(ctx_t & ctx) noexcept;
					/**
					 * \~russian
					 * @brief Метод обработки полностью разобранного ответа прокси
					 *
					 * @details Выбирает итоговое состояние обмена по коду ответа:
					 *          повторная авторизация по 407, установка туннеля по 2xx
					 *          либо ошибка. Вызывается из разбора данных и из уведомления
					 *          о закрытии соединения для ответов без Content-Length.
					 *
					 * @param ctx объект контекста для установки параметров сообщения
					 *
					 * \~english
					 * @brief Method of the processing of the fully parsed answer of the proxy
					 *
					 * @details Chooses the final state of the exchange by the code of the answer:
					 *          the repeated authorization on 407, the establishment of the tunnel on 2xx
					 *          or an error. Called from the parsing of the data and from the notification
					 *          about the closing of the connection for the answers without Content-Length.
					 *
					 * @param ctx object of the context for the setting of the parameters of the message
					 *
					 * \~
					 */
					void complete(ctx_t & ctx) noexcept;
					/**
					 * \~russian
					 * @brief Метод повторной авторизации по ответу 407
					 *
					 * @details Выбирает схему авторизации по вызову прокси и переводит
					 *          контекст в состояние повторного запроса. Если повторы
					 *          исчерпаны либо подходящая схема не найдена, выдаёт голову
					 *          ответа 407 вызывающей стороне и фиксирует ошибку обмена.
					 *
					 * @param ctx объект контекста для установки параметров сообщения
					 * @return    результат возможности повторной авторизации
					 *
					 * \~english
					 * @brief Method of the repeated authorization on an answer 407
					 *
					 * @details Chooses the scheme of the authorization on the challenge of the proxy and transfers
					 *          the context into the state of the repeated request. If the retries are exhausted
					 *          or a suitable scheme is not found, issues the head of the answer 407
					 *          to the calling side and fixes the error of the exchange.
					 *
					 * @param ctx object of the context for the setting of the parameters of the message
					 * @return    result of the possibility of the repeated authorization
					 *
					 * \~
					 */
					bool reauthorize(ctx_t & ctx) noexcept;
					/**
					 * \~russian
					 * @brief Метод выбора схемы авторизации по вызову прокси
					 *
					 * @param ctx объект контекста с заголовками ответа прокси
					 * @return    результат выбора и разбора схемы авторизации
					 *
					 * \~english
					 * @brief Method of the choice of the scheme of the authorization on the challenge of the proxy
					 *
					 * @param ctx object of the context with the headers of the answer of the proxy
					 * @return    result of the choice and of the parsing of the scheme of the authorization
					 *
					 * \~
					 */
					bool challenge(const ctx_t & ctx) noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод формирования строки authority целевого хоста
					 *
					 * @param ctx объект контекста с параметрами целевого хоста
					 * @return    строка authority (host:port, IPv6 в скобках)
					 *
					 * \~english
					 * @brief Method of the forming of the line authority of the target host
					 *
					 * @param ctx object of the context with the parameters of the target host
					 * @return    line authority (host:port, IPv6 in brackets)
					 *
					 * \~
					 */
					string authority(const ctx_t & ctx) noexcept;
					/**
					 * \~russian
					 * @brief Метод добавления заголовка авторизации в контейнер запроса
					 *
					 * @param headers контейнер заголовков запроса
					 *
					 * \~english
					 * @brief Method of the addition of the header of the authorization into the container of the request
					 *
					 * @param headers container of the headers of the request
					 *
					 * \~
					 */
					void authorize(headers_t & headers) noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод сборки байтов пересылаемого запроса в absolute-form
					 *
					 * @details Собирает заголовки и тело запроса в выходной буфер
					 *          отправителя и сохраняет сформированные байты в запросе.
					 *          При установленном флаге учётных данных добавляет заголовок
					 *          авторизации, иначе действует режим превентивной авторизации.
					 *
					 * @param request     объект пересылаемого запроса
					 * @param credentials флаг принудительного добавления учётных данных
					 * @return            результат сборки запроса
					 *
					 * \~english
					 * @brief Method of the assembly of the octets of a forwarded request in absolute-form
					 *
					 * @details Assembles the headers and the body of the request into the output buffer
					 *          of the sender and stores the formed octets in the request.
					 *          At the set flag of the credentials adds the header of the authorization,
					 *          otherwise the mode of the preemptive authorization acts.
					 *
					 * @param request     object of the forwarded request
					 * @param credentials flag of the forced addition of the credentials
					 * @return            result of the assembly of the request
					 *
					 * \~
					 */
					bool assemble(forward_request_t & request, const bool credentials) noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод обработки провайдера разобранного ответа прокси
					 *
					 * @param id       идентификатор потока
					 * @param provider объект провайдера стартовой строки ответа
					 * @param end      флаг завершения сообщения
					 * @return         результат обработки
					 *
					 * \~english
					 * @brief Method of the processing of the provider of a parsed answer of the proxy
					 *
					 * @param id       stream identifier
					 * @param provider object of the provider of the starting line of the answer
					 * @param end      flag of the completion of the message
					 * @return         result of the processing
					 *
					 * \~
					 */
					bool responseProvider(const uint32_t id, const provider_t * provider, const bool end) noexcept;
					/**
					 * \~russian
					 * @brief Метод обработки фрагмента тела разобранного ответа прокси
					 *
					 * @param id     идентификатор потока
					 * @param buffer буфер фрагмента тела
					 * @param size   размер фрагмента тела
					 * @param end    флаг завершения тела
					 * @return       результат обработки
					 *
					 * \~english
					 * @brief Method of the processing of a fragment of the body of a parsed answer of the proxy
					 *
					 * @param id     stream identifier
					 * @param buffer buffer of the fragment of the body
					 * @param size   size of the fragment of the body
					 * @param end    flag of the completion of the body
					 * @return       result of the processing
					 *
					 * \~
					 */
					bool responseData(const uint32_t id, const void * buffer, const size_t size, const bool end) noexcept;
					/**
					 * \~russian
					 * @brief Метод обработки заголовков разобранного ответа прокси
					 *
					 * @param id    идентификатор потока
					 * @param name  имя заголовка
					 * @param value значение заголовка
					 * @param part  часть сообщения (заголовки/трейлеры)
					 * @return      результат обработки
					 *
					 * \~english
					 * @brief Method of the processing of the headers of a parsed answer of the proxy
					 *
					 * @param id    stream identifier
					 * @param name  name of the header
					 * @param value value of the header
					 * @param part  part of the message (headers/trailers)
					 * @return      result of the processing
					 *
					 * \~
					 */
					bool responseHeader(const uint32_t id, const string_view name, const string_view value, const parser_t::part_t part) noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод обработки провайдера разобранного запроса пользователя
					 *
					 * @param id       идентификатор потока
					 * @param provider объект провайдера стартовой строки запроса
					 * @param end      флаг завершения сообщения
					 * @return         результат обработки
					 *
					 * \~english
					 * @brief Method of the processing of the provider of a parsed request of the user
					 *
					 * @param id       stream identifier
					 * @param provider object of the provider of the starting line of the request
					 * @param end      flag of the completion of the message
					 * @return         result of the processing
					 *
					 * \~
					 */
					bool requestProvider(const uint32_t id, const provider_t * provider, const bool end) noexcept;
					/**
					 * \~russian
					 * @brief Метод обработки фрагмента тела разобранного запроса пользователя
					 *
					 * @param id     идентификатор потока
					 * @param buffer буфер фрагмента тела
					 * @param size   размер фрагмента тела
					 * @param end    флаг завершения тела
					 * @return       результат обработки
					 *
					 * \~english
					 * @brief Method of the processing of a fragment of the body of a parsed request of the user
					 *
					 * @param id     stream identifier
					 * @param buffer buffer of the fragment of the body
					 * @param size   size of the fragment of the body
					 * @param end    flag of the completion of the body
					 * @return       result of the processing
					 *
					 * \~
					 */
					bool requestBody(const uint32_t id, const void * buffer, const size_t size, const bool end) noexcept;
					/**
					 * \~russian
					 * @brief Метод обработки заголовков разобранного запроса пользователя
					 *
					 * @param id    идентификатор потока
					 * @param name  имя заголовка
					 * @param value значение заголовка
					 * @param part  часть сообщения (заголовки/трейлеры)
					 * @return      результат обработки
					 *
					 * \~english
					 * @brief Method of the processing of the headers of a parsed request of the user
					 *
					 * @param id    stream identifier
					 * @param name  name of the header
					 * @param value value of the header
					 * @param part  part of the message (headers/trailers)
					 * @return      result of the processing
					 *
					 * \~
					 */
					bool requestHeader(const uint32_t id, const string_view name, const string_view value, const parser_t::part_t part) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод уведомления о завершении потока данных от прокси
					 *
					 * @details Требуется для ответов, тело которых кадрируется закрытием
					 *          соединения (HTTP/1.0 либо без Content-Length и chunked):
					 *          вызывается при закрытии соединения прокси-сервером.
					 *
					 * \~english
					 * @brief Method of the notification about the completion of the stream of the data from the proxy
					 *
					 * @details Required for the answers the body of which is framed by the closing
					 *          of the connection (HTTP/1.0 or without Content-Length and chunked):
					 *          called at the closing of the connection by the proxy server.
					 *
					 * \~
					 */
					void eof() noexcept;
					/**
					 * \~russian
					 * @brief Метод уведомления о завершении потока данных от прокси с обработкой ответа
					 *
					 * @details Дополнительно к уведомлению парсера обрабатывает полностью
					 *          разобранный ответ: для ответов, тело которых кадрируется
					 *          закрытием соединения, переводит контекст в итоговое состояние
					 *          обмена (повторная авторизация по 407, туннель по 2xx либо ошибка).
					 *
					 * @param ctx объект контекста для установки параметров сообщения
					 *
					 * \~english
					 * @brief Method of the notification about the completion of the stream of the data from the proxy with the processing of the answer
					 *
					 * @details In addition to the notification of the parser, processes the fully
					 *          parsed answer: for the answers the body of which is framed by the closing
					 *          of the connection, transfers the context into the final state
					 *          of the exchange (the repeated authorization on 407, the tunnel on 2xx or an error).
					 *
					 * @param ctx object of the context for the setting of the parameters of the message
					 *
					 * \~
					 */
					void eof(ctx_t & ctx) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод полной очистки для нового соединения
					 *
					 * @param ctx объект контекста для сброса параметров сообщения
					 *
					 * \~english
					 * @brief Method of the full clearing for a new connection
					 *
					 * @param ctx object of the context for the reset of the parameters of the message
					 *
					 * \~
					 */
					void clear(ctx_t & ctx) noexcept;
					/**
					 * \~russian
					 * @brief Метод подготовки к следующему обмену в том же соединении
					 *
					 * @details Дешёвый сброс между сообщениями (keep-alive): сохраняет
					 *          настройки и учётные данные, очищает состояние разбора
					 *          и накопленные параметры запроса.
					 *
					 * @param ctx объект контекста для сброса параметров сообщения
					 *
					 * \~english
					 * @brief Method of the preparation for the next exchange in the same connection
					 *
					 * @details A cheap reset between the messages (keep-alive): preserves
					 *          the settings and the credentials, clears the state of the parsing
					 *          and the accumulated parameters of the request.
					 *
					 * @param ctx object of the context for the reset of the parameters of the message
					 *
					 * \~
					 */
					void reset(ctx_t & ctx) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения названия текущей ошибки разбора
					 *
					 * @return название текущей ошибки разбора ответа прокси
					 *
					 * \~english
					 * @brief Method of the getting of the name of the current error of the parsing
					 *
					 * @return name of the current error of the parsing of an answer of the proxy
					 *
					 * \~
					 */
					string_view error() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод освобождения отправленных байтов запроса
					 *
					 * @param size число отправленных байтов
					 *
					 * \~english
					 * @brief Method of the release of the sent octets of the request
					 *
					 * @param size number of the sent octets
					 *
					 * \~
					 */
					void consume(const size_t size) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод установки режима превентивной авторизации
					 *
					 * @details При включённом режиме учётные данные Basic/Bearer отправляются
					 *          в первом же запросе, не дожидаясь ответа 407. Схема Digest
					 *          превентивно не отправляется: она требует вызова сервера.
					 *
					 * @param mode флаг превентивной отправки учётных данных
					 *
					 * \~english
					 * @brief Method of the setting of the mode of the preemptive authorization
					 *
					 * @details At the enabled mode the credentials of Basic/Bearer are sent
					 *          in the very first request, without waiting for an answer 407. The scheme Digest
					 *          is not sent preemptively: it requires a challenge of the server.
					 *
					 * @param mode flag of the preemptive sending of the credentials
					 *
					 * \~
					 */
					void preemptive(const bool mode) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод установки токена доступа BEARER
					 *
					 * @param token токен доступа для авторизации на прокси
					 *
					 * \~english
					 * @brief Method of the setting of the token of the access BEARER
					 *
					 * @param token token of the access for the authorization on the proxy
					 *
					 * \~
					 */
					void setToken(const string & token) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки параметров авторизации
					 *
					 * @param username имя пользователя для авторизации на прокси
					 * @param password пароль пользователя для авторизации на прокси
					 *
					 * \~english
					 * @brief Method of the setting of the parameters of the authorization
					 *
					 * @param username user name for the authorization on the proxy
					 * @param password user password for the authorization on the proxy
					 *
					 * \~
					 */
					void setUser(const string & username, const string & password) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения буфера запроса к прокси
					 *
					 * @details Собирает очередной запрос по состоянию контекста: CONNECT
					 *          в режиме TUNNEL либо пересылаемый запрос в режиме FORWARD.
					 *          Буфер принадлежит отправителю и действителен до следующего
					 *          вызова методов модуля; после отправки вызывается consume().
					 *
					 * @param buffer указатель на буфер для извлечения данных
					 * @param size   ссылка на размер буфера для извлечения данных
					 * @param ctx    объект контекста для установки параметров сообщения
					 * @return       результат извлечения данных в буфер
					 *
					 * \~english
					 * @brief Method of the extraction of the buffer of a request to the proxy
					 *
					 * @details Assembles the next request on the state of the context: CONNECT
					 *          in the mode TUNNEL or a forwarded request in the mode FORWARD.
					 *          The buffer belongs to the sender and is valid until the next
					 *          call of the methods of the module; after the sending consume() is called.
					 *
					 * @param buffer pointer to the buffer for the extraction of the data
					 * @param size   reference to the size of the buffer for the extraction of the data
					 * @param ctx    object of the context for the setting of the parameters of the message
					 * @return       result of the extraction of the data into the buffer
					 *
					 * \~
					 */
					bool buffer(uint8_t ** buffer, size_t & size, ctx_t & ctx) noexcept;
					/**
					 * \~russian
					 * @brief Метод парсинга входящих данных от прокси
					 *
					 * @details Потребляет байты ответа прокси и двигает состояние контекста.
					 *          Возвращает число потреблённых байтов: хвост буфера за ними
					 *          принадлежит туннелю (early data успешного ответа CONNECT)
					 *          и должен быть передан прикладному обмену, а не парсеру.
					 *
					 * @param buffer бинарный буфер входящих данных
					 * @param size   размер бинарного буфера входящих данных
					 * @param ctx    объект для извлечения параметров сообщения
					 * @return       количество потреблённых байт данных
					 *
					 * \~english
					 * @brief Method of the parsing of the incoming data from the proxy
					 *
					 * @details Consumes the octets of an answer of the proxy and moves the state of the context.
					 *          Returns the number of the consumed octets: the tail of the buffer behind them
					 *          belongs to the tunnel (early data of a successful answer CONNECT)
					 *          and must be passed to the application exchange, not to the parser.
					 *
					 * @param buffer binary buffer of the incoming data
					 * @param size   size of the binary buffer of the incoming data
					 * @param ctx    object for the extraction of the parameters of the message
					 * @return       number of the consumed octets of the data
					 *
					 * \~
					 */
					size_t parse(const void * buffer, const size_t size, ctx_t & ctx) noexcept;
					/**
					 * \~russian
					 * @brief Метод приёма запроса пользователя для пересылки через прокси
					 *
					 * @details Принимает части HTTP-запросов пользователя в режиме FORWARD,
					 *          накапливает их парсером и по завершении каждого сообщения
					 *          собирает запрос в absolute-form с заголовком Host и авторизацией
					 *          и ставит его в очередь конвейера. Поддерживается конвейер
					 *          запросов (RFC 9112 §9.3.2): несколько сообщений в одном буфере
					 *          разбираются по порядку и ставятся в очередь без потерь.
					 *
					 * @param buffer буфер данных запроса пользователя
					 * @param size   размер данных запроса пользователя
					 * @param ctx    объект контекста для установки параметров сообщения
					 * @return       результат приёма данных запроса
					 *
					 * \~english
					 * @brief Method of the acceptance of a request of the user for the forwarding through a proxy
					 *
					 * @details Accepts the parts of HTTP requests of the user in the mode FORWARD,
					 *          accumulates them by the parser and on the completion of each message
					 *          assembles the request in absolute-form with the header Host and the authorization
					 *          and places it into the queue of the pipeline. The pipeline of the requests
					 *          is supported (RFC 9112 §9.3.2): several messages in one buffer
					 *          are parsed in order and placed into the queue without losses.
					 *
					 * @param buffer buffer of the data of the request of the user
					 * @param size   size of the data of the request of the user
					 * @param ctx    object of the context for the setting of the parameters of the message
					 * @return       result of the acceptance of the data of the request
					 *
					 * \~
					 */
					bool forward(const void * buffer, const size_t size, ctx_t & ctx) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод установки функции обратного вызова готовности трейлеров ответа
					 *
					 * @param callback функция обратного вызова готовности трейлеров ответа прокси
					 *
					 * \~english
					 * @brief Method of the setting of the callback function of the readiness of the trailers of an answer
					 *
					 * @param callback callback function of the readiness of the trailers of an answer of the proxy
					 *
					 * \~
					 */
					void on(trailer_callback_t callback) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки функции обратного вызова готовности головы ответа
					 *
					 * @param callback функция обратного вызова готовности головы ответа прокси
					 *
					 * \~english
					 * @brief Method of the setting of the callback function of the readiness of the head of an answer
					 *
					 * @param callback callback function of the readiness of the head of an answer of the proxy
					 *
					 * \~
					 */
					void on(response_callback_t callback) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки функции обратного вызова обработки тела ответа
					 *
					 * @details Тело ответа прокси в режиме FORWARD выдаётся вызывающей стороне
					 *          этой функцией. Тело ответа 407 и тела ответов режима TUNNEL
					 *          не выдаются: они не принадлежат прикладному обмену.
					 *
					 * @param callback функция обратного вызова обработки фрагмента тела ответа
					 *
					 * \~english
					 * @brief Method of the setting of the callback function of the processing of the body of an answer
					 *
					 * @details The body of an answer of the proxy in the mode FORWARD is issued to the calling side
					 *          by this function. The body of an answer 407 and the bodies of the answers of the mode TUNNEL
					 *          are not issued: they do not belong to the application exchange.
					 *
					 * @param callback callback function of the processing of a fragment of the body of an answer
					 *
					 * \~
					 */
					void on(parser_http_t::data_callback_t callback) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Client() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					~Client() noexcept;
			} client_t;
		};
	};
};
