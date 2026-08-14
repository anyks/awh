/**
 * @file: server.hpp
 * @date: 2026-05-24
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл серверной стороны протокола SOCKS5 — класс Server_Socks5, разбирающий запросы клиента,
 *        выполняющий согласование метода авторизации и формирующий ответы на команды CONNECT, BIND и UDP ASSOCIATE
 *
 * \~english
 * @brief Header file of the server side of the SOCKS5 protocol — the Server_Socks5 class, which parses the requests of the client,
 *        performs the negotiation of the authorization method and forms the answers to the CONNECT, BIND and UDP ASSOCIATE commands
 *
 * \~
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_PROTO_SOCKS5_SERVER__
#define __AWH_PROTO_SOCKS5_SERVER__

/**
 * Подключаем заголовочный файл проекта
 */
#include "socks5.hpp"

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
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Пространство имён протоколов
	 *
	 *
	 * \~english
	 * @brief Protocols namespace
	 *
	 * \~
	 */
	namespace proto {
		/**
		 * \~russian
		 * @brief Класс сервера для работы с socks5 прокси
		 *
		 * \~english
		 * @brief Server class for working with a socks5 proxy
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Server_Socks5 : public socks5_t {
			private:
				/**
				 * \~russian
				 * @brief Внешняя функция проверки авторизации
				 *
				 * @details Если установлена - парсер сам проверяет авторизацию и отдаёт
				 *          результат проверки через функцию обратного вызова. Если не установлена -
				 *          авторизация не проверяется (по умолчанию).
				 *
				 * @param login    логин пользователя
				 * @param password пароль пользователя
				 * @return         результат проверки авторизации (true - авторизация успешна, false - авторизация неуспешна)
				 *
				 * \~english
				 * @brief External function of the check of the authorization
				 * @details If it is set — the parser checks the authorization itself and gives
				 *          the result of the check through a callback function. If it is not set —
				 *          the authorization is not checked (by default).
				 * @param login    login of the user
				 * @param password password of the user
				 * @return         result of the check of the authorization (true — the authorization is successful, false — the authorization is unsuccessful)
				 *
				 * \~
				 */
				function <bool (const string &, const string &)> _callback;
			public:
				/**
				 * \~russian
				 * @brief Метод парсинга входящих данных
				 *
				 * @param buffer бинарный буфер входящих данных
				 * @param size   размер бинарного буфера входящих данных
				 * @param ctx    объект для извлечения параметров сообщения
				 * @return       результат парсинга входящих данных
				 *
				 *
				 * \~english
				 * @brief Method of parsing the incoming data
				 * @param buffer binary buffer of the incoming data
				 * @param size   size of the binary buffer of the incoming data
				 * @param ctx    object for extracting the parameters of the message
				 * @return       result of parsing the incoming data
				 *
				 * \~
				 */
				bool parse(const void * buffer, const size_t size, ctx_t & ctx) noexcept;
				/**
				 * \~russian
				 * @brief Метод парсинга входящих данных
				 *
				 * @param buffer бинарный буфер входящих данных
				 * @param size   размер бинарного буфера входящих данных
				 * @param udp    объект для извлечения параметров UDP заголовка
				 * @return       результат парсинга входящих данных
				 *
				 *
				 * \~english
				 * @brief Method of parsing the incoming data
				 * @param buffer binary buffer of the incoming data
				 * @param size   size of the binary buffer of the incoming data
				 * @param udp    object for extracting the parameters of the UDP header
				 * @return       result of parsing the incoming data
				 *
				 * \~
				 */
				bool parse(const void * buffer, const size_t size, udp_head_t & udp) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения буфера запроса/ответа
				 *
				 * @param buffer указатель на буфер для извлечения данных
				 * @param size   ссылка на размер буфера для извлечения данных
				 * @param ctx    объект для установки параметров сообщения
				 * @return 	     результат извлечения данных в буфер
				 *
				 *
				 * \~english
				 * @brief Method of extracting the buffer of a request/response
				 * @param buffer pointer to the buffer for extracting the data
				 * @param size   reference to the size of the buffer for extracting the data
				 * @param ctx    object for setting the parameters of the message
				 * @return 	     result of extracting the data into the buffer
				 *
				 * \~
				 */
				bool buffer(uint8_t ** buffer, size_t & size, ctx_t & ctx) const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения буфера запроса/ответа
				 *
				 * @param buffer указатель на буфер для извлечения данных
				 * @param size   ссылка на размер буфера для извлечения данных
				 * @param udp    объект для установки параметров UDP заголовка
				 * @return 	     результат извлечения данных в буфер
				 *
				 *
				 * \~english
				 * @brief Method of extracting the buffer of a request/response
				 * @param buffer pointer to the buffer for extracting the data
				 * @param size   reference to the size of the buffer for extracting the data
				 * @param udp    object for setting the parameters of the UDP header
				 * @return 	     result of extracting the data into the buffer
				 *
				 * \~
				 */
				bool buffer(uint8_t ** buffer, size_t & size, const udp_head_t & udp) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод добавления функции обработки авторизации
				 *
				 * @param callback функция обратного вызова для обработки авторизации
				 *
				 * \~english
				 * @brief Method of adding a function of the processing of the authorization
				 * @param callback callback function for the processing of the authorization
				 *
				 * \~
				 */
				void on(function <bool (const string &, const string &)> callback) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 *
				 *
				 * \~english
				 * @brief Constructor
				 * @param fmk framework object
				 * @param log object for working with logs
				 *
				 * \~
				 */
				explicit Server_Socks5(const fmk_t * fmk, const log_t * log) noexcept;
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
				virtual ~Server_Socks5() noexcept;
		} server_socks5_t;
	};
};

#endif // __AWH_PROTO_SOCKS5_SERVER__
