/**
 * @file: icmp.hpp
 * @date: 2026-03-06
 * @license: LicenseRef-AWH-1.0
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
 * Защита от повторного включения заголовочного файла
 */
#ifndef __AWH_UNIT_ICMP__
#define __AWH_UNIT_ICMP__

/**
 * Подключаем заголовочный файл проекта
 */
#include "unit.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * @brief Пространство имён модулей
	 *
	 */
	namespace unit {
		/**
		 * Используем стандартное пространство имён
		 */
		using namespace std;

		/**
		 * @brief Класс ICMP-клиента
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ ICMP : public unit_t {
			public:
				/**
				 * @brief Идентификатор ICMP-клиента
				 *
				 */
				using id_t = uint16_t;
			public:
				/**
				 * @brief Режим работы ICMP-клиента
				 *
				 */
				enum class mode_t : uint8_t {
					SYNC  = 0x00, // Синхронный режим работы
					ASYNC = 0x01  // Асинхронный режим работы
				};
			public:
				/**
				 * @brief Структура ответа от удалённого сервера на запрос ICMP-клиента
				 *
				 * @details Содержит информацию о размере ответа, времени выполнения запроса,
				 *          индексе последовательности запроса, времени жизни пакета (TTL) и адресе удалённого сервера,
				 *          от которого пришёл ответ на запрос ICMP-клиента.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Response {
					// Размер полученного ответа от удалённого сервера на запрос ICMP-клиента
					size_t size;
					// Время выполнения запроса в миллисекундах
					uint64_t elapsed;
					// Индекс последовательности запроса
					uint16_t sequence;
					// Время жизни пакета (TTL) в хопах
					uint32_t timeToLive;
					// Адрес удалённого сервера, от которого пришёл ответ на запрос ICMP-клиента
					net::addr_t * address;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Response() noexcept;
				} response_t;
			private:
				/**
				 * @brief Структура для управления состоянием ICMP-клиента
				 *
				 * @details Содержит информацию о времени ожидания ответа от удалённого сервера, идентификаторе события ICMP-клиента,
				 *          адресе удалённого сервера для выполнения запросов и локальном адресе, с которого выполняется запрос.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Client {
					// Время ожидания ответа от удалённого сервера (в миллисекундах, по умолчанию 5000 мс)
					uint32_t delay;
					// Идентификатор события для ICMP-клиента
					event::id_t eid;
					// Адрес удалённого сервера для выполнения запросов
					unique_ptr <net::addr_t> target;
					// Локальный адрес, с которого выполняется запрос
					unique_ptr <net::addr_t> source;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Client() noexcept;
				} client_t;
				/**
				 * @brief Структура для управления передачей данных при выполнении запросов ICMP
				 *
				 * @details Содержит информацию о текущем идентификаторе запроса, флаге ожидания ответа от удалённого сервера,
				 *          количестве повторений запросов, номере последовательности последнего отправленного запроса
				 *          и штампе времени начала запроса.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Transfer {
					// Активный идентификатор запроса
					id_t id;
					// Флаг ожидания ответа от сервера
					bool waiting;
					// Количество повторений запросов
					uint16_t count;
					// Номер последовательности последнего отправленного запроса
					uint16_t sequence;
					// Штамп времени начала запроса
					uint64_t timestamp;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Transfer() noexcept;
				} transfer_t;
			private:
				// Объект работы с сетевыми адресами
				net_addr_t _addr;
			private:
				// Состояние ICMP-клиента
				client_t _client;
			private:
				// Объект управления передачей данных при выполнении запросов ICMP
				transfer_t _transfer;
			private:
				// Адрес последнего ответа для передачи в callback ping
				unique_ptr <net::addr_t> _replyAddress;
			private:
				/**
				 * @brief Метод уничтожения события ICMP-клиента
				 *
				 */
				void destroyClient() noexcept;
				/**
				 * @brief Метод отправки ICMP Echo-запроса
				 *
				 * @param eid      идентификатор события ICMP-клиента
				 * @param id       идентификатор ICMP-запроса
				 * @param sequence номер последовательности запроса
				 * @return         количество отправленных байт
				 */
				size_t sendEcho(const event::id_t eid, const id_t id, const uint16_t sequence) noexcept;
			private:
				/**
				 * @brief Метод обработки ошибок событий ICMP-клиента
				 *
				 * @param eid         идентификатор события ICMP-клиента
				 * @param error       код ошибки события ICMP-клиента
				 * @param description описание ошибки события ICMP-клиента
				 */
				void error(const event::id_t eid, const event::error_t error, const string & description) noexcept;
			private:
				/**
				 * @brief Метод обработки таймаута ожидания ответа ICMP-сервера
				 *
				 * @param eid    идентификатор события ICMP-клиента
				 * @param action действие события таймера ICMP-клиента
				 * @param delay  задержка таймера ICMP-клиента
				 * @return       нужно ли завершить клиента после истечения таймаута
				 */
				bool timeout(const event::id_t eid, const event::action_t action, const uint32_t delay) noexcept;
				/**
				 * @brief Метод обработки ответов удалённого сервера на ICMP-запросы
				 *
				 * @param eid  идентификатор события чтения ICMP-ответа
				 * @param mode режим обработки события чтения ICMP-ответа
				 * @param data данные события чтения ICMP-ответа
				 * @param size размер данных события чтения ICMP-ответа
				 */
				void response(const event::id_t eid, const mode_t mode, const uint8_t * data, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод инициализации события ICMP-клиента
				 *
				 * @param family семейство протоколов (например: IPv4 или IPv6)
				 * @return       результат инициализации события ICMP-клиента
				 */
				bool init(const event::family_t family) noexcept;
			public:
				/**
				 * @brief Метод установки функций обратного вызова
				 *
				 * @param callback функции обратного вызова (ping, timeout, datagram)
				 */
				void callback(const callback_t & callback) noexcept;
			public:
				/**
				 * @brief Метод установки таймаута для ожидания ответа от сервера
				 *
				 * @param delay время ожидания ответа от сервера (в миллисекундах)
				 */
				void setTimeout(const uint32_t delay) noexcept;
			public:
				/**
				 * @brief Метод получения типа события
				 *
				 * @return тип события
				 */
				event::type_t type() const noexcept;
				/**
				 * @brief Метод получения типа узла события
				 *
				 * @return тип узла события
				 */
				event::node_t node() const noexcept;
				/**
				 * @brief Метод получения семейства события
				 *
				 * @return семейство адресов
				 */
				event::family_t family() const noexcept;
				/**
				 * @brief Метод получения статуса события
				 *
				 * @return статус события
				 */
				event::status_t status() const noexcept;
			public:
				/**
				 * @brief Метод установки адреса хоста целевой машины
				 *
				 * @param target адрес хоста целевой машины
				 * @return       результат выполнения установки
				 */
				bool setTarget(string_view target) noexcept;
				/**
				 * @brief Метод установки адреса хоста целевой машины
				 *
				 * @param target адрес хоста целевой машины
				 * @return       результат выполнения установки
				 */
				bool setTarget(const net::addr_t * target) noexcept;
				/**
				 * @brief Метод установки адреса хоста целевой машины
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param target адрес хоста целевой машины
				 * @return       результат выполнения установки
				 */
				bool setTarget(const event::family_t family, string_view target) noexcept;
			public:
				/**
				 * @brief Метод установки адреса сети, с которого будет выполняться запрос
				 *
				 * @param source адрес сети для выполнения запроса
				 * @return       результат выполнения установки
				 */
				bool setSource(string_view source) noexcept;
				/**
				 * @brief Метод установки адреса сети, с которого будет выполняться запрос
				 *
				 * @param source адрес сети для выполнения запроса
				 * @return       результат выполнения установки
				 */
				bool setSource(const net::addr_t * source) noexcept;
				/**
				 * @brief Метод установки адреса сети, с которого будет выполняться запрос
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param source адрес сети для выполнения запроса
				 * @return       результат выполнения установки
				 */
				bool setSource(const event::family_t family, string_view source) noexcept;
			public:
				/**
				 * @brief Метод получения идентификатора ICMP-клиента для выполнения запроса к удалённому серверу
				 *
				 * @return идентификатор ICMP-клиента для выполнения запроса к удалённому серверу
				 */
				id_t issue() const noexcept;
			public:
				/**
				 * @brief Метод выполнения пингов удалённого сервера
				 *
				 * @param id    идентификатор ICMP-клиента для выполнения запроса к удалённому серверу
				 * @param count количество выполняемых запросов
				 * @param mode  режим выполнения запросов
				 * @return      результат выполнения запроса
				 */
				bool ping(const id_t id, const uint16_t count, const mode_t mode) noexcept;
			private:
				/**
				 * @brief Конструктор копирования (запрещаем)
				 *
				 */
				ICMP(const ICMP &) = delete;
				/**
				 * @brief Оператор копирования (запрещаем)
				 *
				 * @return текущее значение объекта
				 */
				ICMP & operator = (const ICMP &) = delete;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				explicit ICMP(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~ICMP() noexcept;
		} icmp_t;
	};
};

#endif // __AWH_UNIT_ICMP__
