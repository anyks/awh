/**
 * @file: server.hpp
 * @date: 2022-09-08
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

#ifndef __AWH_CORE_SERVER__
#define __AWH_CORE_SERVER__

/**
 * Наши модули
 */
#include "node.hpp"
#include "timer.hpp"
#include "../scheme/server.hpp"
#include "../cluster/cluster.hpp"

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
	 * @brief серверное пространство имён
	 *
	 */
	namespace server {
		/**
		 * @brief Класс серверного сетевого ядра
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
					ACCEPT   = 0x02, // Ошибка разрешения подключения
					CLUSTER  = 0x03, // Ошибка работы кластера
					PROTOCOL = 0x04, // Ошибка активации протокола
					OSBROKEN = 0x05  // Ошибка неподдерживаемой ОС
				};
			private:
				/**
				 * Режим создания таймера DTLS
				 */
				enum class mode_t : uint8_t {
					NONE    = 0x00, // Режим не установлен
					READ    = 0x01, // Режим чтения данных
					ACCEPT  = 0x02, // Режим разрешения подключения
					RECEIVE = 0x03  // Режим ожидания получения данных
				};
			private:
				/**
				 * @brief Объект основных мютексов
				 *
				 */
				typedef struct Mutex {
					std::recursive_mutex main;    // Для установки системных параметров
					std::recursive_mutex close;   // Для закрытия подключения
					std::recursive_mutex accept;  // Для одобрения подключения
					std::recursive_mutex receive; // Для работы с таймаутами ожидания получения данных
					std::recursive_mutex timeout; // Для создания нового таймаута
				} mtx_t;
			private:
				// Мютекс для блокировки основного потока
				mtx_t _mtx;
			private:
				// Объект работы с сокетами
				socket_t _socket;
				// Объект кластера
				cluster_t _cluster;
			private:
				// Правило передачи данных
				transfer_t _transfer;
			private:
				// Размер кластера
				int16_t _clusterSize;
				// Флаг автоматического перезапуска упавших процессов
				bool _clusterAutoRestart;
			private:
				// Флаг активации/деактивации кластера
				awh::scheme_t::mode_t _clusterMode;
			private:
				// Таймер для работы DTLS
				std::unique_ptr <timer_t> _timer;
			private:
				// Список активных дочерних процессов
				std::multimap <uint16_t, pid_t> _workers;
			private:
				// Список таймаутов на получение данных
				std::map <uint64_t, uint16_t> _receive;
				// Список активных таймаутов
				std::map <uint16_t, uint16_t> _timeouts;
			private:
				// Список подключённых брокеров
				std::map <uint16_t, std::unique_ptr <awh::scheme_t::broker_t>> _brokers;
			private:
				/**
				 * @brief Метод вызова при подключении к серверу
				 *
				 * @param sock сетевой сокет подключившегося клиента
				 * @param sid  идентификатор схемы сети
				 */
				void accept(const SOCKET sock, const uint16_t sid) noexcept;
				/**
				 * @brief Метод вызова при активации DTLS-подключения
				 *
				 * @param sid идентификатор схемы сети
				 * @param bid идентификатор брокера
				 */
				void accept(const uint16_t sid, const uint64_t bid) noexcept;
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
				 * @brief Метод создания таймаута подключения или переподключения
				 *
				 * @param sid  идентификатор схемы сети
				 * @param bid  идентификатор брокера
				 * @param msec время ожидания получения данных в миллисекундах
				 * @param mode режим создания таймера
				 */
				void createTimeout(const uint16_t sid, const uint64_t bid, const uint32_t msec, const mode_t mode) noexcept;
			private:
				/**
				 * @brief Метод получения события подключения дочерних процессов
				 *
				 * @param sid идентификатор схемы сети
				 * @param pid идентификатор процесса
				 */
				void clusterReadyCallback(const uint16_t sid, const pid_t pid) noexcept;
				/**
				 * @brief Метод события пересоздании процесса
				 *
				 * @param sid  идентификатор схемы сети
				 * @param pid  идентификатор процесса
				 * @param opid идентификатор старого процесса
				 */
				void clusterRebaseCallback(const uint16_t sid, const pid_t pid, const pid_t opid) const noexcept;
				/**
				 * @brief Метод события завершения работы процесса
				 *
				 * @param sid    идентификатор схемы сети
				 * @param pid    идентификатор процесса
				 * @param status статус остановки работы процесса
				 */
				void clusterExitCallback(const uint16_t sid, const pid_t pid, const int32_t status) const noexcept;
				/**
				 * @brief Метод события ЗАПУСКА/ОСТАНОВКИ кластера
				 *
				 * @param sid   идентификатор схемы сети
				 * @param pid   идентификатор процесса
				 * @param event идентификатор события
				 */
				void clusterEventsCallback(const uint16_t sid, const pid_t pid, const cluster_t::event_t event) noexcept;
				/**
				 * @brief Метод получения сообщений от дочерних процессоров кластера
				 *
				 * @param sid    идентификатор схемы сети
				 * @param pid    идентификатор процесса
				 * @param buffer буфер бинарных данных
				 * @param size   размер буфера бинарных данных
				 */
				void clusterMessageCallback(const uint16_t sid, const pid_t pid, const char * buffer, const size_t size) noexcept;
			private:
				/**
				 * @brief Метод инициализации DTLS-брокера
				 *
				 * @param sid идентификатор схемы сети
				 */
				void initDTLS(const uint16_t sid) noexcept;
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
				 * @brief Метод отключения всех брокеров
				 *
				 */
				void close() noexcept;
				/**
				 * @brief Метод удаления всех активных схем сети
				 *
				 */
				void remove() noexcept;
			public:
				/**
				 * @brief Метод закрытия подключения брокера
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
				 * @brief Метод закрытия подключения брокера по протоколу UDP
				 *
				 * @param sid идентификатор схемы сети
				 * @param bid идентификатор брокера
				 */
				void close(const uint16_t sid, const uint64_t bid) noexcept;
			public:
				/**
				 * @brief Метод запуска сервера
				 *
				 * @param sid идентификатор схемы сети
				 */
				void launch(const uint16_t sid) noexcept;
			private:
				/**
				 * @brief Метод создания сервера
				 *
				 * @param sid идентификатор схемы сети
				 * @return    результат создания сервера
				 */
				bool create(const uint16_t sid) noexcept;
			public:
				/**
				 * @brief Метод получения хоста сервера
				 *
				 * @param sid идентификатор схемы сети
				 * @return    хост на котором висит сервер
				 */
				string host(const uint16_t sid) const noexcept;
				/**
				 * @brief Метод получения порта сервера
				 *
				 * @param sid идентификатор схемы сети
				 * @return    порт сервера который он прослушивает
				 */
				uint32_t port(const uint16_t sid) const noexcept;
			public:
				/**
				 * @brief Метод получения списка доступных воркеров
				 *
				 * @param sid идентификатор схемы сети
				 * @return    список доступных воркеров
				 */
				std::set <pid_t> workers(const uint16_t sid) const noexcept;
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
			public:
				/**
				 * @brief Метод отправки сообщения родительскому процессу
				 *
				 * @param sid идентификатор схемы сети
				 */
				void sendToProcess(const uint16_t sid) noexcept;
				/**
				 * @brief Метод отправки сообщения родительскому процессу
				 *
				 * @param sid    идентификатор схемы сети
				 * @param buffer бинарный буфер для отправки сообщения
				 * @param size   размер бинарного буфера для отправки сообщения
				 */
				void sendToProcess(const uint16_t sid, const char * buffer, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод отправки сообщения дочернему процессу
				 *
				 * @param sid идентификатор схемы сети
				 * @param pid идентификатор процесса для получения сообщения
				 */
				void sendToProcess(const uint16_t sid, const pid_t pid) noexcept;
				/**
				 * @brief Метод отправки сообщения дочернему процессу
				 *
				 * @param sid    идентификатор схемы сети
				 * @param pid    идентификатор процесса для получения сообщения
				 * @param buffer бинарный буфер для отправки сообщения
				 * @param size   размер бинарного буфера для отправки сообщения
				 */
				void sendToProcess(const uint16_t sid, const pid_t pid, const char * buffer, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод отправки сообщения всем дочерним процессам
				 *
				 * @param sid идентификатор схемы сети
				 */
				void broadcastToProcess(const uint16_t sid) noexcept;
				/**
				 * @brief Метод отправки сообщения всем дочерним процессам
				 *
				 * @param sid    идентификатор схемы сети
				 * @param buffer бинарный буфер для отправки сообщения
				 * @param size   размер бинарного буфера для отправки сообщения
				 */
				void broadcastToProcess(const uint16_t sid, const char * buffer, const size_t size) noexcept;
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
				 * @brief Метод активации параметров запуска сервера
				 *
				 * @param sid    идентификатор схемы сети
				 * @param ip     адрес интернет-подключения
				 * @param family тип интернет-протокола AF_INET, AF_INET6
				 */
				void work(const uint16_t sid, const string & ip, const int32_t family) noexcept;
			public:
				/**
				 * @brief Метод установки флага использования только сети IPv6
				 *
				 * @param mode флаг для установки
				 */
				void ipV6only(const bool mode) noexcept;
			public:
				/**
				 * @brief Метод установки функций обратного вызова
				 *
				 * @param callback функции обратного вызова
				 */
				void callback(const callback_t & callback) noexcept;
			public:
				/**
				 * @brief Метод установки правила передачи данных
				 *
				 * @param transfer правило передачи данных
				 */
				void transferRule(const transfer_t transfer) noexcept;
			public:
				/**
				 * @brief Метод установки максимального количества одновременных подключений
				 *
				 * @param sid   идентификатор схемы сети
				 * @param total максимальное количество одновременных подключений
				 */
				void total(const uint16_t sid, const uint16_t total) noexcept;
			public:
				/**
				 * @brief Метод установки названия кластера
				 *
				 * @param name название кластера для установки
				 */
				void clusterName(const string & name) noexcept;
			public:
				/**
				 * @brief Метод установки флага перезапуска процессов
				 *
				 * @param mode флаг перезапуска процессов
				 */
				void clusterAutoRestart(const bool mode) noexcept;
			public:
				/**
				 * @brief Метод установки соли шифрования
				 *
				 * @param salt соль для шифрования
				 */
				void clusterSalt(const string & salt) noexcept;
				/**
				 * @brief Метод установки пароля шифрования
				 *
				 * @param password пароль шифрования
				 */
				void clusterPassword(const string & password) noexcept;
			public:
				/**
				 * @brief Метод установки размера шифрования
				 *
				 * @param cipher размер шифрования
				 */
				void clusterCipher(const hash_t::cipher_t cipher) noexcept;
				/**
				 * @brief Метод установки метода компрессии
				 *
				 * @param compressor метод компрессии для установки
				 */
				void clusterCompressor(const hash_t::method_t compressor) noexcept;
			public:
				/**
				 * @brief Метод установки режима передачи данных
				 *
				 * @param transfer режим передачи данных
				 */
				void clusterTransfer(const cluster_t::transfer_t transfer) noexcept;
			public:
				/**
				 * @brief Метод установки пропускной способности сети кластера
				 *
				 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
				 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
				 */
				void clusterBandwidth(const string & read = "", const string & write = "") noexcept;
			public:
				/**
				 * @brief Меод получения семейства кластера
				 *
				 * @return семейство к которому принадлежит кластер (MASTER или CHILDREN)
				 */
				cluster_t::family_t clusterFamily() const noexcept;
			public:
				/**
				 * @brief Метод проверки активации кластера
				 *
				 * @return режим активации кластера
				 */
				awh::scheme_t::mode_t cluster() const noexcept;
				/**
				 * @brief Метод установки количества процессов кластера
				 *
				 * @param mode флаг активации/деактивации кластера
				 * @param size количество рабочих процессов
				 */
				void cluster(const awh::scheme_t::mode_t mode, const int16_t size = 0) noexcept;
			public:
				/**
				 * @brief Метод инициализации сервера
				 *
				 * @param sid  идентификатор схемы сети
				 * @param port порт сервера
				 * @param host хост сервера
				 */
				void init(const uint16_t sid, const uint32_t port, const string & host = "") noexcept;
			public:
				/**
				 * @brief Метод установки пропускной способности сети
				 *
				 * @param bid   идентификатор брокера
				 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
				 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
				 */
				void bandwidth(const uint64_t bid, const string & read = "", const string & write = "") noexcept;
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

#endif // __AWH_CORE_SERVER__
