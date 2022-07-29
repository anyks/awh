/**
 * @file: server.hpp
 * @date: 2021-12-19
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2021
 */

#ifndef __AWH_CORE_SERVER__
#define __AWH_CORE_SERVER__

/**
 * Стандартная библиотека
 */
#include <set>
#include <vector>

/**
 * Наши модули
 */
#include <core/core.hpp>
#include <worker/server.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * server серверное пространство имён
	 */
	namespace server {
		/**
		 * Core Класс клиентского ядра биндинга TCP/IP
		 */
		typedef class Core : public awh::core_t {
			private:
				/**
				 * Worker Устанавливаем дружбу с классом сетевого ядра
				 */
				friend class Worker;
			private:
				/**
				 * Jack Структура работника
				 */
				typedef struct Jack {
					pid_t pid;    // Пид родительского процесса
					int mfds[2];  // Список файловых дескрипторов родительского процесса
					int cfds[2];  // Список файловых дескрипторов дочернего процесса
					ev::io read;  // Объект события на чтение
					ev::io write; // Объект события на запись
					size_t index; // Индекс работника в списке
					size_t count; // Количество подключений
					/**
					 * Jack Конструктор
					 */
					Jack() noexcept : pid(0), index(0), count(0) {}
				} jack_t;
			private:
				/**
				 * Mutex Объект основных мютексов
				 */
				typedef struct Mutex {
					recursive_mutex close;  // Для закрытия подключения
					recursive_mutex accept; // Для одобрения подключения
					recursive_mutex system; // Для установки системных параметров
				} mtx_t;
			private:
				// Идентификатор процесса
				pid_t pid;
				// Индекс работника в списке
				size_t index;
			private:
				// Мютекс для блокировки основного потока
				mtx_t mtx;
			private:
				// Объект для работы с сетевым интерфейсом
				ifnet_t ifnet;
			private:
				// Количество доступных потоков
				size_t threads;
			private:
				// Список блокированных объектов
				set <size_t> locking;
			private:
				// Список дочерних работников
				vector <unique_ptr <jack_t>> jacks;
			private:
				/**
				 * readJack Функция обратного вызова при чтении данных с сокета
				 * @param watcher объект события чтения
				 * @param revents идентификатор события
				 */
				void readJack(ev::io & watcher, int revents) noexcept;
				/**
				 * writeJack Функция обратного вызова при записи данных в сокет
				 * @param watcher объект события записи
				 * @param revents идентификатор события
				 */
				void writeJack(ev::io & watcher, int revents) noexcept;
			private:
				/**
				 * explain Метод разъяснения (создание дочерних процессов)
				 * @param wid   wid идентификатор воркера
				 * @param index индекс инициализированного процесса
				 */
				void explain(const size_t wid, const size_t index = 0) noexcept;
			private:
				/**
				 * resolver Функция выполнения резолвинга домена
				 * @param ip  полученный IP адрес
				 * @param wrk объект воркера
				 */
				void resolver(const string & ip, worker_t * wrk) noexcept;
			private:
				/**
				 * close Метод закрытия сокета
				 * @param fd файловый дескриптор (сокет) для закрытия
				 */
				void close(const int fd) noexcept;
				/**
				 * accept Функция подключения к серверу
				 * @param fd  файловый дескриптор (сокет) подключившегося клиента
				 * @param wid идентификатор воркера
				 */
				void accept(const int fd, const size_t wid) noexcept;
			public:
				/**
				 * close Метод отключения всех воркеров
				 */
				void close() noexcept;
				/**
				 * remove Метод удаления всех воркеров
				 */
				void remove() noexcept;
			public:
				/**
				 * run Метод запуска сервера воркером
				 * @param wid идентификатор воркера
				 */
				void run(const size_t wid) noexcept;
				/**
				 * remove Метод удаления воркера
				 * @param wid идентификатор воркера
				 */
				void remove(const size_t wid) noexcept;
			public:
				/**
				 * close Метод закрытия подключения воркера
				 * @param aid идентификатор адъютанта
				 */
				void close(const size_t aid) noexcept;
			private:
				/**
				 * timeout Функция обратного вызова при срабатывании таймаута
				 * @param aid идентификатор адъютанта
				 */
				void timeout(const size_t aid) noexcept;
				/**
				 * write Функция обратного вызова при записи данных в сокет
				 * @param method метод режима работы
				 * @param aid    идентификатор адъютанта
				 */
				void transfer(const method_t method, const size_t aid) noexcept;
			public:
				/**
				 * setBandwidth Метод установки пропускной способности сети
				 * @param aid   идентификатор адъютанта
				 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
				 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
				 */
				void setBandwidth(const size_t aid, const string & read, const string & write) noexcept;
			public:
				/**
				 * setIpV6only Метод установки флага использования только сети IPv6
				 * @param mode флаг для установки
				 */
				void setIpV6only(const bool mode) noexcept;
				/**
				 * setTotal Метод установки максимального количества одновременных подключений
				 * @param wid   идентификатор воркера
				 * @param total максимальное количество одновременных подключений
				 */
				void setTotal(const size_t wid, const u_short total) noexcept;
				/**
				 * init Метод инициализации сервера
				 * @param wid  идентификатор воркера
				 * @param port порт сервера
				 * @param host хост сервера
				 */
				void init(const size_t wid, const u_int port, const string & host = "") noexcept;
				/**
				 * setCert Метод установки файлов сертификата
				 * @param cert  корневой сертификат
				 * @param key   приватный ключ сертификата
				 * @param chain файл цепочки сертификатов
				 */
				void setCert(const string & cert, const string & key, const string & chain = "") noexcept;
			public:
				/**
				 * Core Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Core(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~Core Деструктор
				 */
				~Core() noexcept;
		} core_t;
	};
};

#endif // __AWH_CORE_SERVER__
