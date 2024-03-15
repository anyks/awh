/**
 * @file: client.hpp
 * @date: 2024-03-09
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2024
 */

#ifndef __AWH_CORE_CLIENT__
#define __AWH_CORE_CLIENT__

/**
 * Наши модули
 */
#include <core/node.hpp>
#include <core/timer.hpp>
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
		 * Core Класс клиентского сетевого ядра
		 */
		typedef class Core : public awh::node_t {
			private:
				/**
				 * Mutex Структура основных мютексов
				 */
				typedef struct Mutex {
					recursive_mutex timer;   // Для создания нового таймера
					recursive_mutex close;   // Для закрытия подключения
					recursive_mutex reset;   // Для сброса параметров таймаута
					recursive_mutex proxy;   // Для работы с прокси-сервером
					recursive_mutex connect; // Для выполнения подключения
				} mtx_t;
			private:
				// Мютекс для блокировки основного потока
				mtx_t _mtx;
			private:
				// Список активных таймеров для сетевых схем
				map <uint16_t, unique_ptr <timer_t>> _timers;
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
				 * launching Метод вызова при активации базы событий
				 * @param mode   флаг работы с сетевым протоколом
				 * @param status флаг вывода события статуса
				 */
				void launching(const bool mode, const bool status) noexcept;
				/**
				 * closedown Метод вызова при деакцтивации базы событий
				 * @param mode   флаг работы с сетевым протоколом
				 * @param status флаг вывода события статуса
				 */
				void closedown(const bool mode, const bool status) noexcept;
			private:
				/**
				 * timeout Метод вызова при срабатывании локального таймаута
				 * @param sid  идентификатор схемы сети
				 * @param mode режим работы клиента
				 */
				void timeout(const uint16_t sid, const scheme_t::mode_t mode) noexcept;
				/**
				 * createTimeout Метод создания таймаута
				 * @param sid  идентификатор схемы сети
				 * @param mode режим работы клиента
				 */
				void createTimeout(const uint16_t sid, const scheme_t::mode_t mode) noexcept;
			public:
				/**
				 * stop Метод остановки клиента
				 */
				void stop() noexcept;
				/**
				 * start Метод запуска клиента
				 */
				void start() noexcept;
			public:
				/**
				 * sendTimeout Метод отправки принудительного таймаута
				 * @param bid идентификатор брокера
				 */
				void sendTimeout(const uint64_t bid) noexcept;
				/**
				 * clearTimeout Метод удаления установленного таймаута
				 * @param sid идентификатор схемы сети
				 */
				void clearTimeout(const uint16_t sid) noexcept;
			private:
				/**
				 * disable Метод остановки активности брокера подключения
				 * @param bid идентификатор брокера
				 */
				void disable(const uint64_t bid) noexcept;
			public:
				/**
				 * close Метод отключения всех брокеров
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
			public:
				/**
				 * close Метод закрытия подключения
				 * @param bid идентификатор брокера
				 */
				void close(const uint64_t bid) noexcept;
				/**
				 * remove Метод удаления схемы сети
				 * @param sid идентификатор схемы сети
				 */
				void remove(const uint16_t sid) noexcept;
			public:
				/**
				 * switchProxy Метод переключения с прокси-сервера
				 * @param bid идентификатор брокера
				 */
				void switchProxy(const uint64_t bid) noexcept;
			private:
				/**
				 * connected Метод вызова при удачном подключении к серверу
				 * @param bid идентификатор брокера
				 */
				void connected(const uint64_t bid) noexcept;
				/**
				 * timeout Метод вызова при срабатывании таймаута
				 * @param bid    идентификатор брокера
				 * @param method метод режима работы
				 */
				void timeout(const uint64_t bid, const engine_t::method_t method) noexcept;
			private:
				/**
				 * read Метод чтения данных для брокера
				 * @param bid идентификатор брокера
				 */
				void read(const uint64_t bid) noexcept;
			public:
				/**
				 * write Метод записи буфера данных в сокет
				 * @param buffer буфер для записи данных
				 * @param size   размер записываемых данных
				 * @param bid    идентификатор брокера
				 */
				void write(const char * buffer, const size_t size, const uint64_t bid) noexcept;
			private:
				/**
				 * work Метод запуска работы подключения клиента
				 * @param sid    идентификатор схемы сети
				 * @param ip     адрес интернет-подключения
				 * @param family тип интернет-протокола AF_INET, AF_INET6
				 */
				void work(const uint16_t sid, const string & ip, const int family) noexcept;
			public:
				/**
				 * Core Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Core(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * Core Конструктор
				 * @param dns объект DNS-резолвера
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Core(const dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~Core Деструктор
				 */
				~Core() noexcept {}
		} core_t;
	};
};

#endif // __AWH_CORE_CLIENT__
