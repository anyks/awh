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
 * @copyright: Copyright © 2025
 */

#ifndef __AWH_CORE_CLIENT__
#define __AWH_CORE_CLIENT__

/**
 * Наши модули
 */
#include <core/node.hpp>
#include <core/timer.hpp>
#include <scheme/client.hpp>

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * client клиентское пространство имён
	 */
	namespace client {
		/**
		 * Core Класс клиентского сетевого ядра
		 */
		typedef class AWHSHARED_EXPORT Core : public awh::node_t {
			public:
				/**
				 * Правила передачи данных
				 */
				enum class transfer_t : uint8_t {
					SYNC  = 0x00, // Данные передаются синхронно
					ASYNC = 0x01  // Данные передаются асинхронно
				};
				/**
				 * Коды ошибок клиента
				 */
				enum class error_t : uint8_t {
					NONE     = 0x00, // Ошибка не установлена
					START    = 0x01, // Ошибка запуска приложения
					CONNECT  = 0x02, // Ошибка подключения
					PROTOCOL = 0x03  // Ошибка активации протокола
				};
			private:
				/**
				 * Mutex Структура основных мютексов
				 */
				typedef struct Mutex {
					std::recursive_mutex close;   // Для закрытия подключения
					std::recursive_mutex reset;   // Для сброса параметров таймаута
					std::recursive_mutex proxy;   // Для работы с прокси-сервером
					std::recursive_mutex connect; // Для выполнения подключения
					std::recursive_mutex receive; // Для работы с таймаутами ожидания получения данных
					std::recursive_mutex timeout; // Для создания нового таймаута
				} mtx_t;
			private:
				// Мютекс для блокировки основного потока
				mtx_t _mtx;
			private:
				// Объект работы таймера
				timer_t _timer;
			private:
				// Правило передачи данных
				transfer_t _transfer;
			private:
				// Список таймаутов на получение данных
				std::map <uint64_t, uint16_t> _receive;
				// Список активных таймаутов
				std::map <uint16_t, uint16_t> _timeouts;
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
			private:
				/**
				 * clearTimeout Метод удаления таймера ожидания получения данных
				 * @param bid идентификатор брокера
				 */
				void clearTimeout(const uint64_t bid) noexcept;
				/**
				 * clearTimeout Метод удаления таймера подключения или переподключения
				 * @param sid идентификатор схемы сети
				 */
				void clearTimeout(const uint16_t sid) noexcept;
			private:
				/**
				 * createTimeout Метод создания таймаута ожидания получения данных
				 * @param bid  идентификатор брокера
				 * @param msec время ожидания получения данных в миллисекундах
				 */
				void createTimeout(const uint64_t bid, const uint32_t msec) noexcept;
				/**
				 * createTimeout Метод создания таймаута подключения или переподключения
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
				 * reset Метод принудительного сброса подключения
				 * @param bid идентификатор брокера
				 */
				void reset(const uint64_t bid) noexcept;
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
			public:
				/**
				 * send Метод асинхронной отправки буфера данных в сокет
				 * @param buffer буфер для записи данных
				 * @param size   размер записываемых данных
				 * @param bid    идентификатор брокера
				 * @return       результат отправки сообщения
				 */
				bool send(const char * buffer, const size_t size, const uint64_t bid) noexcept;
			private:
				/**
				 * read Метод чтения данных для брокера
				 * @param bid идентификатор брокера
				 */
				void read(const uint64_t bid) noexcept;
				/**
				 * write Метод записи данных в брокер
				 * @param bid идентификатор брокера
				 */
				void write(const uint64_t bid) noexcept;
			public:
				/**
				 * write Метод записи буфера данных в сокет
				 * @param buffer буфер для записи данных
				 * @param size   размер записываемых данных
				 * @param bid    идентификатор брокера
				 * @return       количество отправленных байт
				 */
				size_t write(const char * buffer, const size_t size, const uint64_t bid) noexcept;
			private:
				/**
				 * work Метод запуска работы подключения клиента
				 * @param sid    идентификатор схемы сети
				 * @param ip     адрес интернет-подключения
				 * @param family тип интернет-протокола AF_INET, AF_INET6
				 */
				void work(const uint16_t sid, const string & ip, const int32_t family) noexcept;
			public:
				/**
				 * transferRule Метод установки правила передачи данных
				 * @param transfer правило передачи данных
				 */
				void transferRule(const transfer_t transfer) noexcept;
			public:
				/**
				 * waitMessage Метод ожидания входящих сообщений
				 * @param bid идентификатор брокера
				 * @param sec интервал времени в секундах
				 */
				void waitMessage(const uint64_t bid, const uint16_t sec) noexcept;
				/**
				 * waitTimeDetect Метод детекции сообщений по количеству секунд
				 * @param bid     идентификатор брокера
				 * @param read    количество секунд для детекции по чтению
				 * @param write   количество секунд для детекции по записи
				 * @param connect количество секунд для детекции по подключению
				 */
				void waitTimeDetect(const uint64_t bid, const uint16_t read = READ_TIMEOUT, const uint16_t write = WRITE_TIMEOUT, const uint16_t connect = CONNECT_TIMEOUT) noexcept;
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
