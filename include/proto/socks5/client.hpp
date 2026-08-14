/**
 * @file client.hpp
 * @date 2026-05-24
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
 * @brief Заголовочный файл клиентской стороны протокола SOCKS5 — класс Client_Socks5,
 *        формирующий запросы приветствия, авторизации и команд подключения и разбирающий ответы прокси-сервера
 *
 * \~english
 * @brief Header file of the client side of the SOCKS5 protocol — the Client_Socks5 class,
 *        which forms the requests of the greeting, of the authorization and of the connection commands and parses the answers of the proxy server
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_PROTO_SOCKS5_CLIENT__
#define __AWH_PROTO_SOCKS5_CLIENT__

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
		 * @brief Класс клиента для работы с socks5 прокси
		 *
		 * \~english
		 * @brief Client class for working with a socks5 proxy
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Client_Socks5 : public socks5_t {
			private:
				// Имя пользователя для аутентификации
				string _username;
				// Пароль пользователя для аутентификации
				string _password;
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
				 * @brief Метод установки параметров авторизации
				 *
				 * @param username имя пользователя для авторизации на сервере
				 * @param password пароль пользователя для авторизации на сервере
				 *
				 * \~english
				 * @brief Method of setting the parameters of the authorization
				 * @param username user name for the authorization on the server
				 * @param password user password for the authorization on the server
				 *
				 * \~
				 */
				void setUser(const string & username, const string & password) noexcept;
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
				explicit Client_Socks5(const fmk_t * fmk, const log_t * log) noexcept;
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
				virtual ~Client_Socks5() noexcept;
		} client_socks5_t;
	};
};

#endif // __AWH_PROTO_SOCKS5_CLIENT__
