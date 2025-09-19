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
#include "node.hpp"
#include "timer.hpp"
#include "../scheme/client.hpp"

/**
 * @brief пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief клиентское пространство имён
	 *
	 */
	namespace client {
		/**
		 * @brief Класс клиентского сетевого ядра
		 *
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
				 * @brief Структура основных мютексов
				 *
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
				 * @brief Метод создания подключения к удаленному серверу
				 *
				 * @param sid идентификатор схемы сети
				 */
				void connect(const uint16_t sid) noexcept;
				/**
				 * @brief Метод восстановления подключения
				 *
				 * @param sid идентификатор схемы сети
				 */
				void reconnect(const uint16_t sid) noexcept;
			private:
				/**
				 * @brief Метод вызова при активации базы событий
				 *
				 * @param mode   флаг работы с сетевым протоколом
				 * @param status флаг вывода события статуса
				 */
				void launching(const bool mode, const bool status) noexcept;
				/**
				 * @brief Метод вызова при деакцтивации базы событий
				 *
				 * @param mode   флаг работы с сетевым протоколом
				 * @param status флаг вывода события статуса
				 */
				void closedown(const bool mode, const bool status) noexcept;
			private:
				/**
				 * @brief Метод вызова при срабатывании локального таймаута
				 *
				 * @param sid  идентификатор схемы сети
				 * @param mode режим работы клиента
				 */
				void timeout(const uint16_t sid, const scheme_t::mode_t mode) noexcept;
			private:
				/**
				 * @brief Метод удаления таймера ожидания получения данных
				 *
				 * @param bid идентификатор брокера
				 */
				void clearTimeout(const uint64_t bid) noexcept;
				/**
				 * @brief Метод удаления таймера подключения или переподключения
				 *
				 * @param sid идентификатор схемы сети
				 */
				void clearTimeout(const uint16_t sid) noexcept;
			private:
				/**
				 * @brief Метод создания таймаута ожидания получения данных
				 *
				 * @param bid  идентификатор брокера
				 * @param msec время ожидания получения данных в миллисекундах
				 */
				void createTimeout(const uint64_t bid, const uint32_t msec) noexcept;
				/**
				 * @brief Метод создания таймаута подключения или переподключения
				 *
				 * @param sid  идентификатор схемы сети
				 * @param mode режим работы клиента
				 */
				void createTimeout(const uint16_t sid, const scheme_t::mode_t mode) noexcept;
			public:
				/**
				 * @brief Метод остановки клиента
				 *
				 */
				void stop() noexcept;
				/**
				 * @brief Метод запуска клиента
				 *
				 */
				void start() noexcept;
			public:
				/**
				 * @brief Метод принудительного сброса подключения
				 *
				 * @param bid идентификатор брокера
				 */
				void reset(const uint64_t bid) noexcept;
			public:
				/**
				 * @brief Метод отключения всех брокеров
				 *
				 */
				void close() noexcept;
				/**
				 * @brief Метод удаления всех схем сети
				 *
				 */
				void remove() noexcept;
			public:
				/**
				 * @brief Метод открытия подключения
				 *
				 * @param sid идентификатор схемы сети
				 */
				void open(const uint16_t sid) noexcept;
			public:
				/**
				 * @brief Метод закрытия подключения
				 *
				 * @param bid идентификатор брокера
				 */
				void close(const uint64_t bid) noexcept;
				/**
				 * @brief Метод удаления схемы сети
				 *
				 * @param sid идентификатор схемы сети
				 */
				void remove(const uint16_t sid) noexcept;
			public:
				/**
				 * @brief Метод переключения с прокси-сервера
				 *
				 * @param bid идентификатор брокера
				 */
				void switchProxy(const uint64_t bid) noexcept;
			private:
				/**
				 * @brief Метод вызова при удачном подключении к серверу
				 *
				 * @param bid идентификатор брокера
				 */
				void connected(const uint64_t bid) noexcept;
			public:
				/**
				 * @brief Метод асинхронной отправки буфера данных в сокет
				 *
				 * @param buffer буфер для записи данных
				 * @param size   размер записываемых данных
				 * @param bid    идентификатор брокера
				 * @return       результат отправки сообщения
				 */
				bool send(const char * buffer, const size_t size, const uint64_t bid) noexcept;
			private:
				/**
				 * @brief Метод чтения данных для брокера
				 *
				 * @param bid идентификатор брокера
				 */
				void read(const uint64_t bid) noexcept;
				/**
				 * @brief Метод записи данных в брокер
				 *
				 * @param bid идентификатор брокера
				 */
				void write(const uint64_t bid) noexcept;
			public:
				/**
				 * @brief Метод записи буфера данных в сокет
				 *
				 * @param buffer буфер для записи данных
				 * @param size   размер записываемых данных
				 * @param bid    идентификатор брокера
				 * @return       количество отправленных байт
				 */
				size_t write(const char * buffer, const size_t size, const uint64_t bid) noexcept;
			private:
				/**
				 * @brief Метод запуска работы подключения клиента
				 *
				 * @param sid    идентификатор схемы сети
				 * @param ip     адрес интернет-подключения
				 * @param family тип интернет-протокола AF_INET, AF_INET6
				 */
				void work(const uint16_t sid, const string & ip, const int32_t family) noexcept;
			public:
				/**
				 * @brief Метод установки правила передачи данных
				 *
				 * @param transfer правило передачи данных
				 */
				void transferRule(const transfer_t transfer) noexcept;
			public:
				/**
				 * @brief Метод ожидания входящих сообщений
				 *
				 * @param bid идентификатор брокера
				 * @param sec интервал времени в секундах
				 */
				void waitMessage(const uint64_t bid, const uint16_t sec) noexcept;
				/**
				 * @brief Метод детекции сообщений по количеству секунд
				 *
				 * @param bid     идентификатор брокера
				 * @param read    количество секунд для детекции по чтению
				 * @param write   количество секунд для детекции по записи
				 * @param connect количество секунд для детекции по подключению
				 */
				void waitTimeDetect(const uint64_t bid, const uint16_t read = READ_TIMEOUT, const uint16_t write = WRITE_TIMEOUT, const uint16_t connect = CONNECT_TIMEOUT) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Core(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param dns объект DNS-резолвера
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Core(const dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~Core() noexcept {}
		} core_t;
	};
};

#endif // __AWH_CORE_CLIENT__
