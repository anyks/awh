/**
 * @file: cluster.hpp
 * @date: 2023-07-01
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

#ifndef __AWH_CORE_CLUSTER__
#define __AWH_CORE_CLUSTER__

/**
 * Наши модули
 */
#include "core.hpp"
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
	 * @brief пространство имён
	 *
	 */
	namespace cluster {
		/**
		 * @brief Класс клиентского ядра биндинга TCP/IP
		 *
		 */
		typedef class AWHSHARED_EXPORT Core : public awh::core_t {
			public:
				/**
				 * Коды ошибок клиента
				 */
				enum class error_t : uint8_t {
					NONE     = 0x00, // Ошибка не установлена
					START    = 0x01, // Ошибка запуска приложения
					CLUSTER  = 0x02, // Ошибка работы кластера
					OSBROKEN = 0x03  // Ошибка неподдерживаемой ОС
				};
			private:
				// Размер кластера
				uint16_t _size;
			private:
				// Объект кластера
				cluster_t _cluster;
			private:
				/**
				 * @brief Метод вывода статуса работы сетевого ядра
				 *
				 * @param status флаг запуска сетевого ядра
				 */
				void activeCallback(const status_t status) noexcept;
				/**
				 * @brief Метод получения события подключения дочерних процессов
				 *
				 * @param wid  идентификатор воркера
				 * @param pid идентификатор процесса
				 */
				void readyCallback(const uint16_t wid, const pid_t pid) noexcept;
				/**
				 * @brief Метод события пересоздании процесса
				 *
				 * @param wid  идентификатор воркера
				 * @param pid  идентификатор процесса
				 * @param opid идентификатор старого процесса
				 */
				void rebaseCallback(const uint16_t wid, const pid_t pid, const pid_t opid) const noexcept;
				/**
				 * @brief Метод события завершения работы процесса
				 *
				 * @param wid    идентификатор воркера
				 * @param pid    идентификатор процесса
				 * @param status статус остановки работы процесса
				 */
				void exitCallback(const uint16_t wid, const pid_t pid, const int32_t status) const noexcept;
				/**
				 * @brief Метод информирования о статусе кластера
				 *
				 * @param wid   идентификатор воркера
				 * @param pid   идентификатор процесса
				 * @param event идентификатор события
				 */
				void eventsCallback(const uint16_t wid, const pid_t pid, const cluster_t::event_t event) const noexcept;
				/**
				 * @brief Метод получения сообщения
				 *
				 * @param wid    идентификатор воркера
				 * @param pid    идентификатор процесса
				 * @param buffer буфер бинарных данных
				 * @param size   размер буфера бинарных данных
				 */
				void messageCallback(const uint16_t wid, const pid_t pid, const char * buffer, const size_t size) const noexcept;
			public:
				/**
				 * @brief Меод получения семейства кластера
				 *
				 * @return семейство к которому принадлежит кластер (MASTER или CHILDREN)
				 */
				cluster_t::family_t family() const noexcept;
			public:
				/**
				 * @brief Метод получения списка дочерних процессов
				 *
				 * @return список дочерних процессов
				 */
				std::set <pid_t> pids() const noexcept;
			public:
				/**
				 * @brief Метод размещения нового воркера
				 *
				 */
				void emplace() noexcept;
				/**
				 * @brief Метод удаления активного процесса
				 *
				 * @param pid идентификатор процесса
				 */
				void erase(const pid_t pid) noexcept;
			public:
				/**
				 * @brief Метод отправки сообщение родительскому процессу
				 *
				 */
				void send() const noexcept;
				/**
				 * @brief Метод отправки сообщение родительскому процессу
				 *
				 * @param buffer буфер бинарных данных
				 * @param size   размер буфера бинарных данных
				 */
				void send(const char * buffer, const size_t size) const noexcept;
			public:
				/**
				 * @brief Метод отправки сообщение процессу
				 *
				 * @param pid идентификатор процесса для отправки
				 */
				void send(const pid_t pid) const noexcept;
				/**
				 * @brief Метод отправки сообщение процессу
				 *
				 * @param pid    идентификатор процесса для отправки
				 * @param buffer буфер бинарных данных
				 * @param size   размер буфера бинарных данных
				 */
				void send(const pid_t pid, const char * buffer, const size_t size) const noexcept;
			public:
				/**
				 * @brief Метод отправки сообщения всем дочерним процессам
				 *
				 */
				void broadcast() const noexcept;
				/**
				 * @brief Метод отправки сообщения всем дочерним процессам
				 *
				 * @param buffer бинарный буфер для отправки сообщения
				 * @param size   размер бинарного буфера для отправки сообщения
				 */
				void broadcast(const char * buffer, const size_t size) const noexcept;
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
				/**
				 * @brief Метод закрытия всех подключений
				 *
				 */
				void close() noexcept;
			public:
				/**
				 * @brief Метод установки названия кластера
				 *
				 * @param name название кластера для установки
				 */
				void name(const string & name) noexcept;
			public:
				/**
				 * @brief Метод установки функций обратного вызова
				 *
				 * @param callback функции обратного вызова
				 */
				void callback(const callback_t & callback) noexcept;
			public:
				/**
				 * @brief Метод установки количества процессов кластера
				 *
				 * @param size количество рабочих процессов
				 */
				void size(const uint16_t size = 0) noexcept;
				/**
				 * @brief Метод установки флага перезапуска процессов
				 *
				 * @param mode флаг перезапуска процессов
				 */
				void autoRestart(const bool mode) noexcept;
			public:
				/**
				 * @brief Метод установки соли шифрования
				 *
				 * @param salt соль для шифрования
				 */
				void salt(const string & salt) noexcept;
				/**
				 * @brief Метод установки пароля шифрования
				 *
				 * @param password пароль шифрования
				 */
				void password(const string & password) noexcept;
			public:
				/**
				 * @brief Метод установки размера шифрования
				 *
				 * @param cipher размер шифрования
				 */
				void cipher(const hash_t::cipher_t cipher) noexcept;
				/**
				 * @brief Метод установки метода компрессии
				 *
				 * @param compressor метод компрессии для установки
				 */
				void compressor(const hash_t::method_t compressor) noexcept;
			public:
				/**
				 * @brief Метод установки режима передачи данных
				 *
				 * @param transfer режим передачи данных
				 */
				void transfer(const cluster_t::transfer_t transfer) noexcept;
			public:
				/**
				 * @brief Метод установки пропускной способности сети
				 *
				 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
				 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
				 */
				void bandwidth(const string & read = "", const string & write = "") noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Core(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~Core() noexcept {}
		} core_t;
	};
};

#endif // __AWH_CORE_CLUSTER__
