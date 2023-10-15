/**
 * @file: client.hpp
 * @date: 2022-09-03
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2022
 */

#ifndef __AWH_CORE_CLIENT__
#define __AWH_CORE_CLIENT__

/**
 * Стандартная библиотека
 */
#include <set>

/**
 * Наши модули
 */
#include <core/core.hpp>
#include <scheme/client.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * client клиентское пространство имён
	 */
	namespace client {
		/**
		 * Core Класс клиентского ядра биндинга TCP/IP
		 */
		typedef class Core : public awh::core_t {
			private:
				/**
				 * Scheme Устанавливаем дружбу с схемой сети
				 */
				friend class Scheme;
			public:
				/**
				 * Режим работы клиента
				 */
				enum class mode_t : uint8_t {
					SYNC  = 0x00, // Синхронный режим работы
					ASYNC = 0x01  // Асинхронный режим работы
				};
			private:
				/**
				 * Mutex Структура основных мютексов
				 */
				typedef struct Mutex {
					recursive_mutex main;    // Для установки системных параметров
					recursive_mutex close;   // Для закрытия подключения
					recursive_mutex reset;   // Для сброса параметров таймаута
					recursive_mutex proxy;   // Для работы с прокси-сервером
					recursive_mutex connect; // Для выполнения подключения
					recursive_mutex timeout; // Для установки таймаутов
				} mtx_t;
				/**
				 * Timeout Класс работы таймаута
				 */
				typedef class Timeout {
					public:
						uint16_t sid;          // Идентификатор схемы сети
						Core * core;           // Объект ядра клиента
						ev::timer timer;       // Объект события таймера
						scheme_t::mode_t mode; // Режим работы клиента
					public:
						/**
						 * callback Метод обратного вызова
						 * @param timer   объект события таймера
						 * @param revents идентификатор события
						 */
						void callback(ev::timer & timer, int revents) noexcept;
					public:
						/**
						 * Timeout Конструктор
						 */
						Timeout() noexcept : sid(0), core(nullptr), mode(scheme_t::mode_t::DISCONNECT) {}
				} timeout_t;
			private:
				// Мютекс для блокировки основного потока
				mtx_t _mtx;
			private:
				// Список блокированных объектов
				set <uint64_t> _locking;
				// Список таймеров
				map <uint64_t, unique_ptr <timeout_t>> _timeouts;
			private:
				/**
				 * connect Метод создания подключения к удаленному серверу
				 * @param sid идентификатор схемы сети
				 */
				void connect(const uint16_t sid) noexcept;
				/**
				 * reconnect Метод восстановления подключения
				 * @param sid идентификатор схемы сети
				 */
				void reconnect(const uint16_t sid) noexcept;
			private:
				/**
				 * createTimeout Метод создания таймаута
				 * @param sid  идентификатор схемы сети
				 * @param mode режим работы клиента
				 */
				void createTimeout(const uint16_t sid, const scheme_t::mode_t mode) noexcept;
			public:
				/**
				 * sendTimeout Метод отправки принудительного таймаута
				 * @param aid идентификатор адъютанта
				 */
				void sendTimeout(const uint64_t aid) noexcept;
				/**
				 * clearTimeout Метод удаления установленного таймаута
				 * @param sid идентификатор схемы сети
				 */
				void clearTimeout(const uint16_t sid) noexcept;
			public:
				/**
				 * close Метод отключения всех адъютантов
				 */
				void close() noexcept;
				/**
				 * remove Метод удаления всех схем сети
				 */
				void remove() noexcept;
			public:
				/**
				 * open Метод открытия подключения
				 * @param sid идентификатор схемы сети
				 */
				void open(const uint16_t sid) noexcept;
				/**
				 * remove Метод удаления схемы сети
				 * @param sid идентификатор схемы сети
				 */
				void remove(const uint16_t sid) noexcept;
			public:
				/**
				 * close Метод закрытия подключения
				 * @param aid идентификатор адъютанта
				 */
				void close(const uint64_t aid) noexcept;
				/**
				 * switchProxy Метод переключения с прокси-сервера
				 * @param aid идентификатор адъютанта
				 */
				void switchProxy(const uint64_t aid) noexcept;
			private:
				/**
				 * timeout Метод вызова при срабатывании таймаута
				 * @param aid идентификатор адъютанта
				 */
				void timeout(const uint64_t aid) noexcept;
				/**
				 * connected Метод вызова при удачном подключении к серверу
				 * @param aid идентификатор адъютанта
				 */
				void connected(const uint64_t aid) noexcept;
				/**
				 * transfer Метед передачи данных между клиентом и сервером
				 * @param method метод режима работы
				 * @param aid    идентификатор адъютанта
				 */
				void transfer(const engine_t::method_t method, const uint64_t aid) noexcept;
				/**
				 * resolving Метод получения IP адреса доменного имени
				 * @param sid    идентификатор схемы сети
				 * @param ip     адрес интернет-подключения
				 * @param family тип интернет-протокола AF_INET, AF_INET6
				 */
				void resolving(const uint16_t sid, const string & ip, const int family) noexcept;
			public:
				/**
				 * bandWidth Метод установки пропускной способности сети
				 * @param aid   идентификатор адъютанта
				 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
				 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
				 */
				void bandWidth(const uint64_t aid, const string & read, const string & write) noexcept;
			public:
				/**
				 * Core Конструктор
				 * @param fmk    объект фреймворка
				 * @param log    объект для работы с логами
				 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
				 * @param sonet  тип сокета подключения (TCP / UDP)
				 */
				Core(const fmk_t * fmk, const log_t * log, const scheme_t::family_t family = scheme_t::family_t::IPV4, const scheme_t::sonet_t sonet = scheme_t::sonet_t::TCP) noexcept;
				/**
				 * ~Core Деструктор
				 */
				~Core() noexcept;
		} core_t;
	};
};

#endif // __AWH_CORE_CLIENT__
