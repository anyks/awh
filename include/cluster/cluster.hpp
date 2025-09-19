/**
 * @file: cluster.hpp
 * @date: 2024-07-14
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

#ifndef __AWH_CLUSTER__
#define __AWH_CLUSTER__

/**
 * Стандартные библиотеки
 */
#include <map>
#include <vector>
#include <thread>
#include <string>
#include <cstring>
#include <csignal>
#include <functional>
#include <sys/types.h>

/**
 * Для операционной системы не являющейся OS Windows
 */
#if !_WIN32 && !_WIN64
	/**
	 * Стандартные библиотеки
	 */
	#include <sys/wait.h>
#endif

/**
 * Наши модули
 */
#include "cmp.hpp"
#include "../sys/fmk.hpp"
#include "../sys/log.hpp"
#include "../sys/callback.hpp"
#include "../net/socket.hpp"
#include "../core/core.hpp"
#include "../events/event.hpp"

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
	 * @brief Класс работы с кластером
	 *
	 */
	typedef class AWHSHARED_EXPORT Cluster {
		public:
			/**
			 * Режим обмена сообщениям
			 */
			enum class transfer_t : uint8_t {
				PIPE = 0x00, // Передача сообщений через Shared Memory
				IPC  = 0x01  // Передача сообщений через Unix-socket
			};
			/**
			 * События работы кластера
			 */
			enum class event_t : uint8_t {
				NONE  = 0x00, // Событие не установлено
				STOP  = 0x02, // Событие остановки процесса
				START = 0x01  // Событие запуска процесса
			};
			/**
			 * Семейство кластера
			 */
			enum class family_t : uint8_t {
				NONE     = 0x00, // Воркер не установлено
				MASTER   = 0x02, // Воркер является мастером
				CHILDREN = 0x01  // Воркер является ребёнком
			};
		private:
			/**
			 * Тип сообщения
			 */
			enum class message_t : uint8_t {
				NONE    = 0x00, // Тип сообщения не установлен
				HELLO   = 0x01, // Тип сообщения рукопожатия
				GENERAL = 0x02  // Тип сообщения общего назначения
			};
		public:
			/**
			 * @brief Структура буфера
			 *
			 */
			typedef struct Buffer {
				// Размер бинарного буфера данных
				size_t size;
				// Бинарный буфер полученных данных
				std::unique_ptr <uint8_t []> data;
				/**
				 * @brief Конструктор
				 *
				 */
				Buffer() noexcept : size(0), data(nullptr) {}
			} buffer_t;
			/**
			 * @brief Класс воркера
			 *
			 */
			typedef class AWHSHARED_EXPORT Worker {
				private:
					// Устанавливаем дружбу с родительским классом
					friend class Cluster;
				private:
					// Флаг запуска работы
					bool _working;
					// Флаг автоматического перезапуска
					bool _autoRestart;
				private:
					// Идентификатор воркера
					uint16_t _wid;
				private:
					// Количество рабочих процессов
					uint16_t _count;
				private:
					// Бинарный буфер полученных данных
					buffer_t _buffer;
				private:
					// Список декодеров для декодирования сообщений
					std::map <SOCKET, std::unique_ptr <cmp::decoder_t>> _decoders;
				private:
					// Объект для работы с логами
					const log_t * _log;
					// Родительский объект кластера
					const Cluster * _ctx;
				public:
					/**
					 * Для операционной системы не являющейся OS Windows
					 */
					#if !_WIN32 && !_WIN64
						/**
						 * @brief Метод обратного вызова получении сообщений
						 *
						 * @param sock  сетевой сокет
						 * @param event произошедшее событие
						 */
						void message(const SOCKET sock, const base_t::event_type_t event) noexcept;
					#endif
				public:
					/**
					 * @brief Конструктор
					 *
					 * @param wid идентификатор воркера
					 * @param ctx родительский объект кластера
					 * @param log объект для работы с логами
					 */
					Worker(const uint16_t wid, const Cluster * ctx, const log_t * log) noexcept :
					 _working(false), _autoRestart(false),
					 _wid(wid), _count(1), _log(log), _ctx(ctx) {}
					/**
					 * @brief Деструктор
					 *
					 */
					~Worker() noexcept {}
			} worker_t;
		private:
			/**
			 * @brief Структура пропускной способности
			 *
			 */
			typedef struct Bandwidth {
				// Размер буфера на чтение
				size_t read;
				// Размер буфера на запись
				size_t write;
				/**
				 * @brief Конструктор
				 *
				 */
				Bandwidth() noexcept :
				 read(AWH_BUFFER_SIZE_RCV),
				 write(AWH_BUFFER_SIZE_SND) {}
			} __attribute__((packed)) bandwidth_t;
			/**
			 * @brief Структура подключения
			 *
			 */
			typedef struct Peer {
				// Размер объекта подключения
				socklen_t size;
				// Параметры подключения
				struct sockaddr_storage addr;
				/**
				 * @brief Конструктор
				 *
				 */
				Peer() noexcept : size(0), addr{} {}
			} peer_t;
			/**
			 * @brief Структура клиента
			 *
			 */
			typedef struct Client {
				// Сетевой сокет
				SOCKET sock;
				// Идентификатор воркера
				uint16_t wid;
				// Объект подключения
				peer_t peer;
				// Объект события
				awh::event_t ev;
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Client(const fmk_t * fmk, const log_t * log) noexcept :
				 sock(INVALID_SOCKET), wid(0), ev(awh::event_t::type_t::EVENT, fmk, log) {}
			} client_t;
			/**
			 * @brief Структура сервера
			 *
			 */
			typedef struct Server {
				// Сетевой сокет
				SOCKET sock;
				// Адрес unix-сокета
				string ipc;
				// Объект работы с файловой системой
				fs_t fs;
				// Объект подключения
				peer_t peer;
				// Объект работы с сокетами
				socket_t socket;
				// Объект события на получения сообщений
				awh::event_t ev;
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Server(const fmk_t * fmk, const log_t * log) noexcept :
				 sock(INVALID_SOCKET), ipc{""}, fs(fmk, log),
				 socket(fmk, log), ev(awh::event_t::type_t::EVENT, fmk, log) {}
			} server_t;
		private:
			/**
			 * Для операционной системы не являющейся OS Windows
			 */
			#if !_WIN32 && !_WIN64
				/**
				 * @brief Структура брокера
				 *
				 */
				typedef struct Broker {
					bool stop;          // Флаг завершения работы процессом
					pid_t pid;          // Идентификатор активного процесса
					uint64_t date;      // Время начала жизни процесса
					SOCKET mfds[2];     // Список файловых дескрипторов родительского процесса
					SOCKET cfds[2];     // Список файловых дескрипторов дочернего процесса
					awh::event_t read;  // Объект события на получения сообщений
					awh::event_t write; // Объект события на запись сообщений
					/**
					 * @brief Конструктор
					 *
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					Broker(const fmk_t * fmk, const log_t * log) noexcept :
					 stop(false), pid(::getpid()), date(0),
					 mfds{INVALID_SOCKET, INVALID_SOCKET},
					 cfds{INVALID_SOCKET, INVALID_SOCKET},
					 read(awh::event_t::type_t::EVENT, fmk, log),
					 write(awh::event_t::type_t::EVENT, fmk, log) {}
					/**
					 * @brief Деструктор
					 *
					 */
					~Broker() noexcept {}
				} broker_t;
			/**
			 * Для операционной системы OS Windows
			 */
			#else
				/**
				 * @brief Структура брокера
				 *
				 */
				typedef struct Broker {
					pid_t pid;     // Идентификатор активного процесса
					uint64_t date; // Время начала жизни процесса
					/**
					 * @brief Конструктор
					 *
					 */
					Broker() noexcept : pid(0), date(0) {}
				} broker_t;
			#endif
		private:
			// Идентификатор родительского процесса
			pid_t _pid;
		private:
			// Название кластера
			string _name;
		private:
			// Соль шифрования сообщений
			string _salt;
			// Пароль шифрования сообщений
			string _pass;
		private:
			// Объект параметров сервера
			server_t _server;
		private:
			// Хранилище функций обратного вызова
			callback_t _callback;
		private:
			// Режим передачи данных
			transfer_t _transfer;
		private:
			// Параметры пропускной способности
			bandwidth_t _bandwidth;
		private:
			// Размер шифрования
			hash_t::cipher_t _cipher;
			// Метод компрессии
			hash_t::method_t _method;
		private:
			/**
			 * Для операционной системы не являющейся OS Windows
			 */
			#if !_WIN32 && !_WIN64
				// Объект перехвата сигнала
				struct sigaction _sa;
			#endif
		private:
			// Список активных дочерних процессов
			std::map <pid_t, uint16_t> _pids;
			// Список активных сокетов привязанных к процессам
			std::map <SOCKET, pid_t> _sockets;
			// Список активных клиентов дочерних процессов
			std::map <SOCKET, std::unique_ptr <client_t>> _clients;
			// Список активных воркеров
			std::map <uint16_t, std::unique_ptr <worker_t>> _workers;
			// Список энкодеров для кодирования сообщений
			std::map <pid_t, std::unique_ptr <cmp::encoder_t>> _encoders;
			// Список дочерних брокеров
			std::map <uint16_t, vector <std::unique_ptr <broker_t>>> _brokers;
		private:
			// Объект сетевого ядра
			core_t * _core;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		private:
			/**
			 * Для операционной системы не являющейся OS Windows
			 */
			#if !_WIN32 && !_WIN64
				/**
				 * @brief Метод инициализации unix-сокета для обмены данными
				 *
				 * @param family семейстов кластера
				 */
				void ipc(const family_t family) noexcept;
				/**
				 * @brief Метод перезапуска упавшего процесса
				 *
				 * @param pid    идентификатор упавшего процесса
				 * @param status статус остановившегося процесса
				 */
				void process(const pid_t pid, const int32_t status) noexcept;
				/**
				 * @brief Функция фильтр перехватчика сигналов
				 *
				 * @param signal номер сигнала полученного системой
				 * @param info   объект информации полученный системой
				 * @param ctx    передаваемый внутренний контекст
				 */
				static void child(int32_t signal, siginfo_t * info, void * ctx) noexcept;
			#endif
		private:
			/**
			 * @brief Метод активации прослушивания сокета
			 *
			 * @return результат выполнения операции
			 */
			bool list() noexcept;
			/**
			 * @brief Метод выполнения подключения
			 *
			 * @return результат выполнения операции
			 */
			bool connect() noexcept;
			/**
			 * @brief Метод обратного вызова получении запроса на подключение
			 *
			 * @param wid  идентификатор воркера
			 * @param sock сетевой сокет
			 */
			void accept(const uint16_t wid, const SOCKET sock, const base_t::event_type_t) noexcept;
		private:
			/**
			 * @brief Метод записи буфера данных в сокет
			 *
			 * @param wid  идентификатор воркера
			 * @param pid  идентификатор процесса для получения сообщения
			 * @param sock идентификатор сетевого сокета
			 */
			void write(const uint16_t wid, const pid_t pid, const SOCKET sock) noexcept;
		private:
			/**
			 * @brief Метод обратного вызова получении сообщений готовности сокета на запись
			 *
			 * @param wid  идентификатор воркера
			 * @param pid  идентификатор процесса для отправки сообщения
			 * @param sock сетевой сокет
			 */
			void sending(const uint16_t wid, const pid_t pid, const SOCKET sock) noexcept;
		private:
			/**
			 * @brief Метод размещения нового дочернего процесса
			 *
			 * @param wid идентификатор воркера
			 * @param pid идентификатор предыдущего процесса
			 */
			void emplace(const uint16_t wid, const pid_t pid) noexcept;
			/**
			 * @brief Метод создания дочерних процессов при запуске кластера
			 *
			 * @param wid   идентификатор воркера
			 * @param index индекс инициализированного процесса
			 */
			void create(const uint16_t wid, const uint16_t index = 0) noexcept;
		public:
			/**
			 * @brief Метод проверки является ли процесс родительским
			 *
			 * @return результат проверки
			 */
			bool master() const noexcept;
		public:
			/**
			 * @brief Метод проверки на запуск работы кластера
			 *
			 * @param wid идентификатор воркера
			 * @return    результат работы проверки
			 */
			bool working(const uint16_t wid) const noexcept;
		public:
			/**
			 * @brief Метод получения списка дочерних процессов
			 *
			 * @param wid идентификатор воркера
			 * @return    список дочерних процессов
			 */
			std::set <pid_t> pids(const uint16_t wid) const noexcept;
		public:
			/**
			 * @brief Метод отправки сообщения родительскому процессу
			 *
			 * @param wid идентификатор воркера
			 */
			void send(const uint16_t wid) noexcept;
			/**
			 * @brief Метод отправки сообщения родительскому процессу
			 *
			 * @param wid    идентификатор воркера
			 * @param buffer бинарный буфер для отправки сообщения
			 * @param size   размер бинарного буфера для отправки сообщения
			 */
			void send(const uint16_t wid, const char * buffer, const size_t size) noexcept;
		public:
			/**
			 * @brief Метод отправки сообщения дочернему процессу
			 *
			 * @param wid идентификатор воркера
			 * @param pid идентификатор процесса для получения сообщения
			 */
			void send(const uint16_t wid, const pid_t pid) noexcept;
			/**
			 * @brief Метод отправки сообщения дочернему процессу
			 *
			 * @param wid    идентификатор воркера
			 * @param pid    идентификатор процесса для получения сообщения
			 * @param buffer бинарный буфер для отправки сообщения
			 * @param size   размер бинарного буфера для отправки сообщения
			 */
			void send(const uint16_t wid, const pid_t pid, const char * buffer, const size_t size) noexcept;
		public:
			/**
			 * @brief Метод отправки сообщения всем дочерним процессам
			 *
			 * @param wid идентификатор воркера
			 */
			void broadcast(const uint16_t wid) noexcept;
			/**
			 * @brief Метод отправки сообщения всем дочерним процессам
			 *
			 * @param wid    идентификатор воркера
			 * @param buffer бинарный буфер для отправки сообщения
			 * @param size   размер бинарного буфера для отправки сообщения
			 */
			void broadcast(const uint16_t wid, const char * buffer, const size_t size) noexcept;
		public:
			/**
			 * @brief Метод очистки всех выделенных ресурсов
			 *
			 */
			void clear() noexcept;
		public:
			/**
			 * @brief Метод закрытия всех подключений
			 *
			 */
			void close() noexcept;
			/**
			 * @brief Метод закрытия всех подключений
			 *
			 * @param wid идентификатор воркера
			 */
			void close(const uint16_t wid) noexcept;
		private:
			/**
			 * @brief Метод закрытия сетевого сокета
			 *
			 * @param wid  идентификатор воркера
			 * @param sock сетевой сокет для закрытия
			 */
			void close(const uint16_t wid, const SOCKET sock) noexcept;
		public:
			/**
			 * @brief Метод остановки кластера
			 *
			 * @param wid идентификатор воркера
			 */
			void stop(const uint16_t wid) noexcept;
			/**
			 * @brief Метод запуска кластера
			 *
			 * @param wid идентификатор воркера
			 */
			void start(const uint16_t wid) noexcept;
		public:
			/**
			 * @brief Метод установки флага перезапуска процессов
			 *
			 * @param wid  идентификатор воркера
			 * @param mode флаг перезапуска процессов
			 */
			void restart(const uint16_t wid, const bool mode) noexcept;
		public:
			/**
			 * @brief Метод установки сетевого ядра
			 *
			 * @param core сетевое ядро для установки
			 */
			void core(core_t * core) noexcept;
		public:
			/**
			 * @brief Метод установки названия кластера
			 *
			 * @param name название кластера для установки
			 */
			void name(const string & name) noexcept;
		public:
			/**
			 * @brief Метод установки режима передачи данных
			 *
			 * @param transfer режим передачи данных
			 */
			void transfer(const transfer_t transfer) noexcept;
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
			 * @brief Метод размещения нового дочернего процесса
			 *
			 * @param wid идентификатор воркера
			 */
			void emplace(const uint16_t wid) noexcept;
			/**
			 * @brief Метод удаления активного процесса
			 *
			 * @param wid идентификатор воркера
			 * @param pid идентификатор процесса
			 */
			void erase(const uint16_t wid, const pid_t pid) noexcept;
		public:
			/**
			 * @brief Метод получения максимального количества процессов
			 *
			 * @param wid идентификатор воркера
			 * @return    максимальное количество процессов
			 */
			uint16_t count(const uint16_t wid) const noexcept;
			/**
			 * @brief Метод установки максимального количества процессов
			 *
			 * @param wid   идентификатор воркера
			 * @param count максимальное количество процессов
			 */
			void count(const uint16_t wid, const uint16_t count) noexcept;
		public:
			/**
			 * @brief Метод установки флага разрешения перезапуска процессов
			 *
			 * @param wid  идентификатор воркера
			 * @param mode флаг перезапуска процессов
			 */
			void autoRestart(const uint16_t wid, const bool mode) noexcept;
		public:
			/**
			 * @brief Метод инициализации воркера
			 *
			 * @param wid   идентификатор воркера
			 * @param count максимальное количество процессов
			 */
			void init(const uint16_t wid, const uint16_t count = 1) noexcept;
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
			 * @brief Метод установки функций обратного вызова
			 *
			 * @param callback функции обратного вызова
			 */
			void callback(const callback_t & callback) noexcept;
		public:
			/**
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 */
			template <typename T, class... Args>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param name  идентификатор функкции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 */
			auto on(const char * name, Args... args) noexcept -> uint64_t {
				// Если мы получили название функции обратного вызова
				if(name != nullptr)
					// Выполняем установку функции обратного вызова
					return this->_callback.on <T> (name, args...);
				// Выводим результат по умолчанию
				return 0;
			}
			/**
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 */
			template <typename T, class... Args>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param name  идентификатор функкции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 */
			auto on(const string & name, Args... args) noexcept -> uint64_t {
				// Если мы получили название функции обратного вызова
				if(!name.empty())
					// Выполняем установку функции обратного вызова
					return this->_callback.on <T> (name, args...);
				// Выводим результат по умолчанию
				return 0;
			}
			/**
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 */
			template <typename T, class... Args>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param fid  идентификатор функкции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 */
			auto on(const uint64_t fid, Args... args) noexcept -> uint64_t {
				// Если мы получили название функции обратного вызова
				if(fid > 0)
					// Выполняем установку функции обратного вызова
					return this->_callback.on <T> (fid, args...);
				// Выводим результат по умолчанию
				return 0;
			}
			/**
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam A    тип идентификатора функции
			 * @tparam B    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 */
			template <typename A, typename B, class... Args>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param fid  идентификатор функкции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 */
			auto on(const A fid, Args... args) noexcept -> uint64_t {
				// Если мы получили на вход число
				if(is_integral_v <A> || is_enum_v <A> || is_floating_point_v <A>)
					// Выполняем установку функции обратного вызова
					return this->_callback.on <B> (static_cast <uint64_t> (fid), args...);
				// Выводим результат по умолчанию
				return 0;
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Cluster(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param core объект сетевого ядра
			 * @param fmk  объект фреймворка
			 * @param log  объект для работы с логами
			 */
			Cluster(core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Cluster() noexcept;
	} cluster_t;
};

#endif // __AWH_CLUSTER__
