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
			ACCEPT     = 0x0A, // Входящее соединение принято
			CONNECT    = 0x0B, // Подключение к серверу установлено
			RECONNECT  = 0x0C, // Переподключение к серверу после обрыва подключения
			DISCONNECT = 0x0D  // Отключение от сервера выполнено
		};
		/**
		 * @brief Типы узлов
		 *
		 */
		enum class node_t : uint8_t {
			NONE     = 0x00, // Узел не определён
			IPC      = 0x01, // Узел межпроцессного взаимодействия
			DIR      = 0x02, // Узел директории в файловой системе
			FILE     = 0x03, // Узел файла в файловой системе
			PEER     = 0x04, // Одноранговый узел
			NOTIFY   = 0x05, // Узел уведомления
			CLIENT   = 0x06, // Узел клиента
			SERVER   = 0x07, // Узел сервера
			TIMEOUT  = 0x08, // Узел таймаута времени
			INTERVAL = 0x09  // Узел интервала времени
		};
		/**
		 * @brief Типы виртуальных узлов
		 *
		 */
		enum class vnode_t : uint8_t {
			NONE = 0x00, // Узел не определён
			CHR  = 0x01, // Виртуальный символьный узел устройства
			BLK  = 0x02, // Виртуальный блочный узел устройства
			DIR  = 0x03, // Виртуальный узел каталога
			FILE = 0x04, // Виртуальный узел файла
			FIFO = 0x05, // Виртуальный узел канала FIFO
			SOCK = 0x06, // Виртуальный сокет узел
			LINK = 0x07  // Виртуальный узел символической ссылки
		};
		/**
		 * @brief Типы адресов событий
		 *
		 */
		enum class address_t : uint8_t {
			NONE    = 0x00, // Адрес не определён
			FS      = 0x01, // Адрес файловой системы
			MAC     = 0x02, // MAC-адрес
			UDS     = 0x03, // Адрес Unix Domain Socket
			IPV4    = 0x04, // Адрес IPv4
			IPV6    = 0x05, // Адрес IPv6
			NETWORK = 0x06  // Сетевой адрес
		};
		/**
		 * @brief Типичные значения TTL для multicast
		 * 
		 */
		enum class multicast_ttl_t : uint8_t {
			LOOPBACK  = 0x00, // Только в хосте (loopback)
			NETWORK   = 0x01, // Только локальная сеть (подсеть) — самое частое значение для локального multicast
			COMPANY   = 0x20, // Внутри организации
			REGION    = 0x40, // Внутри региона
			CONTINENT = 0x80, // Внутри континента
			WORLD     = 0xff  // Глобально (максимум)
		};
		/**
		 * @brief Статусы событий
		 *
		 */
		enum class status_t : uint8_t {
			NONE        = 0x00, // Статус не определён
			INITIAL     = 0x01, // Статус инициализации
			DESTROYED   = 0x02, // Статус удаления
			ACCEPTED    = 0x03, // Статус принятия
			RUNNING     = 0x04, // Статус выполнения
			STOPPED     = 0x05, // Статус остановки
			PAUSED      = 0x06, // Статус паузы
			RESUMED     = 0x07, // Статус возобновления
			SUCCESS     = 0x08, // Операция выполнена успешно
			FAILURE     = 0x09, // Операция завершилась неудачей
			PENDING     = 0x0A, // Операция в ожидании
			CONNECTED   = 0x0B, // Статус подключено
			CANCELLED   = 0x0C, // Операция отменена
			RECONNECTED = 0x0D  // Статус переподключения
		};
		/**
		 * @brief Типы ошибок событий
		 *
		 */
		enum class error_t : uint8_t {
			NONE             = 0x00, // Ошибка не определена
			UNKNOWN          = 0x01, // Неизвестная ошибка
			INVALID          = 0x02, // Недопустимая операция
			NOT_FOUND        = 0x03, // Объект не найден
			EVENT_FAIL       = 0x04, // Ошибка события
			ACCESS_DENIED    = 0x05, // Доступ запрещён
			ALREADY_EXISTS   = 0x06, // Объект уже существует
			INVALID_SOCKET   = 0x07, // Ошибка доступа к сокету
			INVALID_ADDRESS  = 0x08, // Некорректный адрес
			CONNECTION_FAIL  = 0x09, // Ошибка подключения
			INSUFFICIENT_RES = 0x0A  // Недостаточно ресурсов
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
		 * @brief Типы смещений в файле
		 *
		 */
		enum class seek_t : uint8_t {
			BEGIN   = 0x00, // Смещение от начала файла
			CURRENT = 0x01, // Смещение от текущей позиции
			END     = 0x02  // Смещение от конца файла
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
			FILE = 0x07, // Файловый сокет
			DIR  = 0x08  // Сокет каталога
		};
		/**
		 * @brief Семейства сокетов
		 *
		 */
		enum class family_t : uint8_t {
			NONE  = 0x00, // Семейство событий не определено
			UDS   = 0x01, // Семейство событий Unix Domain Sockets
			PIPE  = 0x02, // Семейство событий пайпов
			FSYS  = 0x03, // Файловое семейство событий
			USER  = 0x04, // Пользовательское семейство событий
			IPV4  = 0x05, // Семейство событий IPv4
			IPV6  = 0x06, // Семейство событий IPv6
			TIMER = 0x07  // Семейство таймеров
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
			 * Опция одиночного использования сокета
			 */
			static constexpr uint16_t ONSHOT = 0x01;
			/**
			 * Опция отложенной отправки TCP пакетов
			 */
			static constexpr uint16_t TCPCORK = 0x02;
			/**
			 * Опция ручной установки заголовков IP пакетов
			 */
			static constexpr uint16_t HDRINCL = 0x04;
			/**
			 * Опция только IPv6 для сокета
			 */
			static constexpr uint16_t IPV6ONLY = 0x08;
			/**
			 * Опция отключения сигнала SIGILL
			 */
			static constexpr uint16_t NOSIGILL = 0x10;
			/**
			 * Опция отключения сигнала SIGPIPE
			 */
			static constexpr uint16_t NOSIGPIPE = 0x20;
			/**
			 * Опция неблокирующего ввода-вывода
			 */
			static constexpr uint16_t NOIOBLOCK = 0x40;
			/**
			 * Опция умного неблокирующего ввода-вывода
			 */
			static constexpr uint16_t SMIOBLOCK = 0x80;
			/**
			 * Опция широковещательного адреса
			 */
			static constexpr uint16_t BROADCAST = 0x100;
			/**
			 * Опция повторного использования адреса
			 */
			static constexpr uint16_t REUSEADDR = 0x200;
			/**
			 * Опция повторного использования порта
			 */
			static constexpr uint16_t REUSEPORT = 0x400;
			/**
			 * Опция отключения алгоритма Нейгла
			 */
			static constexpr uint16_t TCPNODELAY = 0x800;
			/**
			 * Опция включения TCP keepalive
			 */
			static constexpr uint16_t KEEPALIVE = 0x1000;
			/**
			 * Опция закрытия сокета при выполнении exec
			 */
			static constexpr uint16_t CLOSEONEXEC = 0x2000;
		};
		/**
		 * @brief пространство имён работы с обратными вызовами
		 *
		 */
		namespace callback {
			/**
			 * Функция обратного вызова срабатывающая при подключении события
			 */
			using connect_t = std::function <void (const event::id_t, const bool)>;
			/**
			 * Функция обратного вызова срабатывающая при записи в событие
			 */
			using write_t = std::function <void (const event::id_t, const size_t)>;
			/**
			 * Функция обратного вызова срабатывающая при принятии события
			 */
			using accept_t = std::function <void (const event::id_t, const event::id_t)>;
			/**
			 * Функция обратного вызова срабатывающая при общем событии
			 */
			using event_t = std::function <void (const event::id_t, const event::action_t)>;
			/**
			 * Функция обратного вызова срабатывающая при изменении статуса события
			 */
			using status_t = std::function <void (const event::id_t, const event::status_t)>;
			/**
			 * Функция обратного вызова срабатывающая при чтении из события
			 */
			using read_t = std::function <void (const event::id_t, const uint8_t *, const size_t)>;
			/**
			 * Функция обратного вызова срабатывающая при ошибке события
			 */
			using error_t = std::function <void (const event::id_t, const event::error_t, const std::string &)>;
			/**
			 * Функция обратного вызова срабатывающая при изменении каталога
			 */
			using change_t = std::function <void (const event::id_t, const action_t, const vnode_t, const std::string &)>;
		};
	};
};

#endif // __AWH_EVENTS__
