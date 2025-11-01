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
			NONE   = 0x00, // Экшен события отсутствует
			CREATE = 0x01, // Создание объекта
			UPDATE = 0x02, // Обновление объекта
			DELETE = 0x03, // Удаление объекта
			READ   = 0x04, // Чтение объекта
			WRITE  = 0x05, // Запись объекта
			CLOSE  = 0x06  // Закрытие объекта
		};
		/**
		 * @brief Типы узлов
		 *
		 */
		enum class node_t : uint8_t {
			NONE   = 0x00, // Тип узла не определён
			PEER   = 0x01, // Узел-сосед
			CLIENT = 0x02, // Клиентский узел
			SERVER = 0x03  // Серверный узел
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
			NONE      = 0x00, // Статус не определён
			INITIAL   = 0x01, // Статус инициализации
			DESTROYED = 0x02, // Статус удаления
			RUNNING   = 0x03, // Статус выполнения
			STOPPED   = 0x04, // Статус остановки
			PAUSED    = 0x05, // Статус паузы
			RESUMED   = 0x06, // Статус возобновления
			SUCCESS   = 0x07, // Операция выполнена успешно
			FAILURE   = 0x08, // Операция завершилась неудачей
			PENDING   = 0x09, // Операция в ожидании
			CANCELLED = 0x0A  // Операция отменена
		};
		/**
		 * @brief Категории событий
		 *
		 */
		enum class category_t : uint8_t {
			NONE           = 0x00, // Категория не определена
			AUTHENTICATION = 0x01, // События аутентификации
			AUTHORIZATION  = 0x02, // События авторизации
			SYSTEM         = 0x03, // Системные события
			APPLICATION    = 0x04, // События приложений
			SECURITY       = 0x05  // События безопасности
		};
		/**
		 * @brief Режимы обработки событий
		 *
		 */
		enum class mode_t : uint8_t {
			NONE         = 0x00, // Режим не определён
			ADAPTIVE     = 0x01, // Адаптивный режим обработки событий
			SYNCHRONOUS  = 0x02, // Синхронный режим обработки событий
			ASYNCHRONOUS = 0x03  // Асинхронный режим обработки событий
		};
		/**
		 * @brief Уведомления событий
		 *
		 */
		enum class notify_t : uint8_t {
			DISABLED = 0x00, // Уведомления отключены
			ENABLED  = 0x01  // Уведомления включены
		};
		/**
		 * @brief Разрешения событий
		 *
		 */
		enum class permission_t : uint8_t {
			READ    = 0x01, // Разрешение на чтение
			WRITE   = 0x02, // Разрешение на запись
			EXECUTE = 0x03, // Разрешение на выполнение
			DELETE  = 0x04, // Разрешение на удаление
			MODIFY  = 0x05, // Разрешение на изменение
			FULL    = 0x06  // Полные разрешения
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
		 * @brief Степени важности событий
		 *
		 */
		enum class severity_t : uint8_t {
			LOW      = 0x00, // Низкая степень важности события
			MEDIUM   = 0x01, // Средняя степень важности события
			HIGH     = 0x02, // Высокая степень важности события
			CRITICAL = 0x03  // Критическая степень важности события
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
		 * @brief Опции сокетов
		 *
		 */
		enum class option_t : uint8_t {
			NONE       = 0x00, // Опция не определена
			REUSEADDR  = 0x01, // Опция повторного использования адреса
			KEEPALIVE  = 0x02, // Опция поддержания активности соединения
			NOSIGPIPE  = 0x03, // Опция отключения сигнала SIGPIPE
			TCPNODELAY = 0x04, // Опция отключения алгоритма Нейгла
			BROADCAST  = 0x05, // Опция разрешения широковещательных сообщений
			MCASTJOIN  = 0x06, // Опция присоединения к мультикаст группе
			MCASTLEAVE = 0x07, // Опция выхода из мультикаст группы
			RCVTIMEO   = 0x08, // Опция таймаута приёма данных
			SNDTIMEO   = 0x09  // Опция таймаута отправки данных
		};
		/**
		 * @brief пространство имён работы с обратными вызовами
		 *
		 */
		namespace callback {
			// Обратный вызов события пользователя
			using user_t = std::function <void (const uint32_t)>;
			// Обратный вызов при записи в событие
			using write_t = std::function <void (const id_t, const size_t)>;
			// Обратный вызов при подключении события
			using connect_t = std::function <void (const id_t, const bool)>;
			// Обратный вызов при ошибке события
			using error_t = std::function <void (const id_t, const std::string &)>;
			// Обратный вызов при принятии события
			using accept_t = std::function <void (const id_t, const id_t)>;
			// Обратный вызов при изменении статуса события
			using status_t = std::function <void (const id_t, const status_t)>;
			// Обратный вызов при чтении из события
			using read_t = std::function <void (const id_t, const uint8_t *, const size_t)>;
		};
	};
};

#endif // __AWH_EVENTS__
