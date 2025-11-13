/**
 * @file: events.hpp
 * @date: 2025-10-26
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

#ifndef __AWH_EVENTS__
#define __AWH_EVENTS__

/**
 * Стандартные модули
 */
#include <cstdint>
#include <functional>

/**
 * @brief основное пространство имён
 *
 */
namespace awh {
	/**
	 * @brief пространство имён работы с событием
	 *
	 */
	namespace event {
		/**
		 * @brief Тип идентификатора события
		 *
		 */
		using id_t = uint32_t;
		/**
		 * @brief Действия событий
		 *
		 */
		enum class action_t : uint8_t {
			NONE       = 0x00, // Экшен события отсутствует
			READ       = 0x01, // Чтение объекта
			WRITE      = 0x02, // Запись объекта
			CLOSE      = 0x03, // Закрытие объекта
			CHANGE     = 0x04, // Изменение файла или каталога
			DELETE     = 0x05, // Удаление файла или каталога
			RENAME     = 0x06, // Переименование файла или каталога
			ATTRIB     = 0x07, // Атрибуты файла изменены (chmod, chown, utime)
			REVOKE     = 0x08, // Доступ к файлу отозван (например, файл был удалён или размонтирована ФС)
			HDLINK     = 0x09, // Счётчик жёстких ссылок на файл изменился
			CONNECT    = 0x0A, // Подключение к серверу установлено
			RECONNECT  = 0x0B, // Переподключение к серверу после обрыва подключения
			DISCONNECT = 0x0C  // Отключение от сервера выполнено
		};
		/**
		 * @brief Типы узлов
		 *
		 */
		enum class node_t : uint8_t {
			NONE   = 0x00, // Тип узла не определён
			IPC    = 0x01, // Межпроцессное взаимодействие
			DIR    = 0x02, // Директория
			FILE   = 0x03, // Файл в файловой системе
			PEER   = 0x04, // Одноранговый узел
			USER   = 0x05, // Пользовательский узел
			TIMER  = 0x06, // Таймерный узел
			CLIENT = 0x07, // Клиентский узел
			SERVER = 0x08  // Серверный узел
		};
		/**
		 * @brief Типы адресов событий
		 *
		 */
		enum class address_t : uint8_t {
			NONE    = 0x00, // Адрес не определён
			MAC     = 0x01, // MAC-адрес
			UDS     = 0x02, // Адрес Unix Domain Socket
			DIR     = 0x03, // Адрес директории
			FILE    = 0x04, // Файловый адрес
			IPV4    = 0x05, // Адрес IPv4
			IPV6    = 0x06, // Адрес IPv6
			NETWORK = 0x07  // Сетевой адрес
		};
		/**
		 * @brief Статусы событий
		 *
		 */
		enum class status_t : uint8_t {
			NONE        = 0x00, // Статус не определён
			INITIAL     = 0x01, // Статус инициализации
			DESTROYED   = 0x02, // Статус удаления
			RUNNING     = 0x03, // Статус выполнения
			STOPPED     = 0x04, // Статус остановки
			PAUSED      = 0x05, // Статус паузы
			RESUMED     = 0x06, // Статус возобновления
			SUCCESS     = 0x07, // Операция выполнена успешно
			FAILURE     = 0x08, // Операция завершилась неудачей
			PENDING     = 0x09, // Операция в ожидании
			CONNECTED   = 0x0A, // Статус подключено
			CANCELLED   = 0x0B, // Операция отменена
			RECONNECTED = 0x0C  // Статус переподключения
		};
		/**
		 * @brief Происхождение событий
		 *
		 */
		enum class origin_t : uint8_t {
			LOCAL  = 0x00, // Локальное событие
			REMOTE = 0x01  // Удалённое событие
		};
		/**
		 * @brief Режимы событий
		 *
		 */
		enum class mode_t : uint8_t {
			ENABLED  = 0x00, // Режим включён
			DISABLED = 0x01  // Режим отключён
		};
		/**
		 * @brief Типы протоколов сокетов
		 *
		 */
		enum class protocol_t : uint8_t {
			NONE = 0x00, // Сокет не определён
			RAW  = 0x01, // Сокет RAW
			UDP  = 0x02, // Сокет UDP
			TCP  = 0x03, // Сокет TCP
			ICMP = 0x04, // Сокет ICMP
			IGMP = 0x05, // Сокет IGMP
			SCTP = 0x06, // Сокет SCTP
			DTLS = 0x07, // Сокет DTLS
			FILE = 0x08, // Файловый сокет
			DIR  = 0x09  // Сокет каталога
		};
		/**
		 * @brief Семейства сокетов
		 *
		 */
		enum class family_t : uint8_t {
			NONE     = 0x00, // Семейство не определено
			UDS      = 0x01, // Семейство Unix Domain Sockets
			IPC      = 0x02, // Семейство межпроцессного взаимодействия
			DIR      = 0x03, // Семейство директорий
			FILE     = 0x04, // Файловое семейство сокетов
			IPV4     = 0x05, // Семейство сокетов IPv4
			IPV6     = 0x06, // Семейство сокетов IPv6
			UDPV4    = 0x07, // Семейство сокетов UDPv4
			UDPV6    = 0x08, // Семейство сокетов UDPv6
			TIMER    = 0x09, // Семейство таймеров
			INTERVAL = 0x0A  // Семейство интервалов
		};
		/**
		 * @brief Флаги сокетов
		 *
		 */
		enum class flag_t : uint8_t {
			NONE        = 0x00, // Флаг не определён
			NONBLOCKING = 0x01, // Флаг неблокирующего сокета
			BLOCKING    = 0x02, // Флаг блокирующего сокета
			LISTENING   = 0x03, // Флаг слушающего сокета
			CONNECTED   = 0x04, // Флаг подключённого сокета
			CLOSED      = 0x05  // Флаг закрытого сокета
		};
		/**
		 * @brief Типы сокетов
		 *
		 */
		enum class type_t : uint8_t {
			NONE      = 0x00, // Тип сокета не определён
			RAW       = 0x01, // Сырой сокет
			STREAM    = 0x02, // Потоковый сокет
			DATAGRAM  = 0x03, // Дейтаграммный сокет
			SEQPACKET = 0x04  // Сокет с последовательными пакетами
		};
		/**
		 * @brief пространство имён опций событий
		 *
		 */
		namespace options {
			/**
			 * Опция не определена
			 */
			static constexpr uint16_t NONE = 0x00;
			/**
			 * Опция отложенной отправки TCP пакетов
			 */
			static constexpr uint16_t TCPCORK = 0x01;
			/**
			 * Опция только IPv6 для сокета
			 */
			static constexpr uint16_t IPV6ONLY = 0x02;
			/**
			 * Опция отключения сигнала SIGILL
			 */
			static constexpr uint16_t NOSIGILL = 0x04;
			/**
			 * Опция отключения сигнала SIGPIPE
			 */
			static constexpr uint16_t NOSIGPIPE = 0x08;
			/**
			 * Опция неблокирующего ввода-вывода
			 */
			static constexpr uint16_t NOIOBLOCK = 0x10;
			/**
			 * Опция повторного использования адреса
			 */
			static constexpr uint16_t REUSEADDR = 0x20;
			/**
			 * Опция повторного использования порта
			 */
			static constexpr uint16_t REUSEPORT = 0x40;
			/**
			 * Опция отключения алгоритма Нейгла
			 */
			static constexpr uint16_t TCPNODELAY = 0x80;
			/**
			 * Опция включения TCP keepalive
			 */
			static constexpr uint16_t KEEPALIVE = 0x100;
			/**
			 * Опция закрытия сокета при выполнении exec
			 */
			static constexpr uint16_t CLOSEONEXEC = 0x200;
		};
		/**
		 * @brief пространство имён работы с обратными вызовами
		 *
		 */
		namespace callback {
			/**
			 * Обратный вызов при подключении события
			 */
			using connect_t = std::function <void (const event::id_t, const bool)>;
			/**
			 * Обратный вызов при записи в событие
			 */
			using write_t = std::function <void (const event::id_t, const size_t)>;
			/**
			 * Обратный вызов при принятии события
			 */
			using accept_t = std::function <void (const event::id_t, const event::id_t)>;
			/**
			 * Обратный вызов при ошибке события
			 */
			using error_t = std::function <void (const event::id_t, const std::string &)>;
			/**
			 * Обратный вызов общего события
			 */
			using event_t = std::function <void (const event::id_t, const event::action_t)>;
			/**
			 * Обратный вызов при изменении статуса события
			 */
			using status_t = std::function <void (const event::id_t, const event::status_t)>;
			/**
			 * Обратный вызов при чтении из события
			 */
			using read_t = std::function <void (const event::id_t, const uint8_t *, const size_t)>;
		};
	};
};

#endif // __AWH_EVENTS__
