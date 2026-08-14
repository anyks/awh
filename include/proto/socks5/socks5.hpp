/**
 * @file socks5.hpp
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
 * @brief Заголовочный файл базового класса протокола SOCKS5 (RFC 1928) — общий конечный автомат обмена,
 *        коды команд и статусов, структура UDP-заголовка и разбор адресов,
 *        разделяемые клиентской и серверной реализациями
 *
 * \~english
 * @brief Header file of the base class of the SOCKS5 protocol (RFC 1928) — the common state machine of the exchange,
 *        the codes of the commands and of the statuses, the structure of the UDP header and the parsing of the addresses,
 *        shared by the client and the server implementations
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_PROTO_SOCKS5__
#define __AWH_PROTO_SOCKS5__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <cstddef>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../net/net.hpp"
#include "../../sys/fmk.hpp"
#include "../../sys/log.hpp"

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
		 * @brief Класс для работы с протоколом SOCKS5
		 *
		 * \~english
		 * @brief Class for working with the SOCKS5 protocol
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Socks5 {
			public:
				/**
				 * \~russian
				 * @brief Максимальный размер входящего SOCKS5-кадра по TCP
				 *
				 * \~english
				 * @brief Maximum size of an incoming SOCKS5 frame over TCP
				 *
				 * \~
				 */
				static constexpr size_t SOCKS5_RX_MAX_FRAME = 0x202;
				/**
				 * \~russian
				 * @brief Максимальный размер исходящего SOCKS5-кадра (RFC 1929 USER/PASS)
				 *
				 * \~english
				 * @brief Maximum size of an outgoing SOCKS5 frame (RFC 1929 USER/PASS)
				 *
				 * \~
				 */
				static constexpr size_t SOCKS5_TX_BUFFER_SIZE = 0x202;
			public:
				/**
				 * \~russian
				 * @brief Комманды запроса клиента
				 *
				 * \~english
				 * @brief Commands of a request of the client
				 *
				 * \~
				 */
				enum class command_t : uint8_t {
					NONE    = 0x00, // Команда не определена
					CONNECT = 0x01, // Метод подключения
					BIND    = 0x02, // Метод обратного подключения (сервера к клиенту)
					UDP     = 0x03  // Работа с UDP протоколом
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
					AUTH      = 0x01, // Состояние ожидания получения метода аутентификации
					BROKEN    = 0x02, // Состояние бракованных данных
					CONNECT   = 0x03, // Состояние ожидания получения команды подключения
					SUCCESS   = 0x04, // Состояние успешного получения запроса
					REQUEST   = 0x05, // Состояние ожидания получения запроса
					RESPONSE  = 0x06, // Состояние ожидания получения ответа
					HANDSHAKE = 0x07, // Состояние выполненного рукопожатия
					COMPLETED = 0x08  // Состояние завершённого обмена данными
				};
				/**
				 * \~russian
				 * @brief Коды статусов ответа сервера
				 *
				 * \~english
				 * @brief Status codes of an answer of the server
				 *
				 * \~
				 */
				enum class status_t : uint8_t {
					SUCCESS   = 0x00, // Подключение успешное
					SOCKSERR  = 0x01, // Ошибка SOCKS-сервера
					FORBIDDEN = 0x02, // Соединение запрещено набором правил
					UNAVNET   = 0x03, // Сеть недоступна
					UNAVHOST  = 0x04, // Хост недоступен
					DENIED    = 0x05, // Отказ в соединении
					TIMETTL   = 0x06, // Истечение TTL
					NOCOMMAND = 0x07, // Команда не поддерживается
					NOADDR    = 0x08, // Тип адреса не поддерживается
					NOSUPPORT = 0x09, // До X'FF' не определены
					NOSTATUS  = 0xFF, // Статус не определён
				};
			public:
				/**
				 * \~russian
				 * @brief Структура UDP заголовка
				 *
				 * \~english
				 * @brief Structure of the UDP header
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ UDP_Header {
					// Номер фрагмента (0x00 = нет фрагментации)
					uint8_t frag;
					// Размер данных UDP пакета
					size_t size;
					// Хост конечного получателя
					unique_ptr <net::attr_t> host;
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
					explicit UDP_Header() noexcept;
				} udp_head_t;
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
					// Текущее состояние
					state_t state;
					// Текущее значение статуса
					status_t status;
					// Запрошенная команда подключения
					command_t command;
					// Параметры адреса хоста
					unique_ptr <net::attr_t> host;
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
					explicit Context() noexcept;
				} ctx_t;
			protected:
				// Объект фреймворка
				const fmk_t * _fmk;
				// Объект для работы с логами
				const log_t * _log;
			public:
				/**
				 * \~russian
				 * @brief Метод получения сообщения
				 *
				 * @param code код статуса
				 * @return     текстовое значение кода статуса
				 *
				 * \~english
				 * @brief Method of getting a message
				 * @param code status code
				 * @return     text value of the status code
				 *
				 * \~
				 */
				static string statusMessage(const status_t code) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод определения полного размера SOCKS5-кадра
				 *
				 * @param state текущее состояние протокола
				 * @param data  буфер входящих данных
				 * @param size  размер буфера входящих данных
				 * @return      0 — кадр неполный; SIZE_MAX — кадр некорректный; иначе размер кадра
				 *
				 * \~english
				 * @brief Method of determining the full size of a SOCKS5 frame
				 * @param state current state of the protocol
				 * @param data  buffer of the incoming data
				 * @param size  size of the buffer of the incoming data
				 * @return      0 — the frame is incomplete; SIZE_MAX — the frame is incorrect; otherwise the size of the frame
				 *
				 * \~
				 */
				static size_t frameSize(const state_t state, const uint8_t * data, const size_t size) noexcept;
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
				virtual bool parse(const void * buffer, const size_t size, ctx_t & ctx) noexcept = 0;
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
				virtual bool parse(const void * buffer, const size_t size, udp_head_t & udp) noexcept = 0;
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
				virtual bool buffer(uint8_t ** buffer, size_t & size, ctx_t & ctx) const noexcept = 0;
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
				virtual bool buffer(uint8_t ** buffer, size_t & size, const udp_head_t & udp) const noexcept = 0;
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
				explicit Socks5(const fmk_t * fmk, const log_t * log) noexcept;
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
				virtual ~Socks5() noexcept;
		} socks5_t;
	};
};

#endif // __AWH_PROTO_SOCKS5__
