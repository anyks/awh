/**
 * @file: core.hpp
 * @date: 2022-09-08
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

#ifndef __AWH_CORE__
#define __AWH_CORE__

/**
 * Стандартные модули
 */
#include <map>
#include <ctime>
#include <mutex>
#include <chrono>
#include <thread>
#include <string>
#include <cerrno>
#include <cstdlib>
#include <functional>
#include <unordered_map>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/event_struct.h>

/**
 * Если операционной системой является Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	// Подключаем заголовочный файл
	#include <tchar.h>
	#include <synchapi.h>
/**
 * Для всех остальных операционных систем
 */
#else
	// Подключаем заголовочный файл
	#include <unistd.h>
#endif

/**
 * Наши модули
 */
#include <sys/fn.hpp>
#include <net/dns.hpp>
#include <net/engine.hpp>
#include <scheme/core.hpp>
#include <sys/signals.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Core Класс ядра биндинга TCP/IP
	 */
	typedef class Core {
		private:
			/**
			 * Scheme Устанавливаем дружбу с схемой сети
			 */
			friend class Scheme;
		public:
			/**
			 * Статус работы сетевого ядра
			 */
			enum class status_t : uint8_t {
				STOP  = 0x02, // Статус остановки
				START = 0x01  // Статус запуска
			};
			/**
			 * Флаги активации/деактивации
			 */
			enum class mode_t : uint8_t {
				ENABLED  = 0x01, // Включено
				DISABLED = 0x00  // Отключено
			};
			/**
			 * Коды ошибок клиента
			 */
			enum class error_t : uint8_t {
				NONE      = 0x00, // Ошибка не установлена
				START     = 0x01, // Ошибка запуска приложения
				ACCEPT    = 0x02, // Ошибка разрешения подключения
				TIMEOUT   = 0x03, // Подключение завершено по таймауту
				CONNECT   = 0x04, // Ошибка подключения
				PROTOCOL  = 0x05, // Ошибка активации протокола
				OS_BROKEN = 0x06  // Ошибка неподдерживаемой ОС
			};
		private:
			/**
			 * Timer Класс таймера
			 */
			typedef class Timer {
				public:
					// Идентификатор таймера
					uint16_t id;
				public:
					// Флаг персистентной работы
					bool persist;
				public:
					// Задержка времени в секундах
					time_t delay;
				public:
					// Объект события таймера
					event_t event;
				public:
					// Объект сетевого ядра
					Core * core;
				public:
					// Внешняя функция обратного вызова
					function <void (const uint16_t)> fn;
				public:
					/**
					 * callback Метод обратного вызова
					 * @param fd    файловый дескриптор (сокет)
					 * @param event произошедшее событие
					 */
					void callback(const evutil_socket_t fd, const short event) noexcept;
				public:
					/**
					 * Timer Конструктор
					 * @param log объект для работы с логами
					 */
					Timer(const log_t * log) noexcept :
					 id(0), persist(false), delay(0),
					 event(event_t::type_t::TIMER, log),
					 core(nullptr), fn(nullptr) {}
					/**
					 * ~Timer Деструктор
					 */
					~Timer() noexcept;
			} timer_t;
		protected:
			/**
			 * Mutex Объект основных мютексов
			 */
			typedef struct Mutex {
				recursive_mutex bind;   // Для работы с биндингом ядра
				recursive_mutex main;   // Для работы с параметрами модуля
				recursive_mutex timer;  // Для работы с таймерами
				recursive_mutex status; // Для контроля запуска модуля
				recursive_mutex scheme; // Для работы с схемой сети
			} mtx_t;
			/**
			 * Settings Структура текущих параметров сети
			 */
			typedef struct Settings {
				// Протокол активного подключения (RAW, HTTP1, HTTP1_1, HTTP2, HTTP3)
				engine_t::proto_t proto;
				// Тип сокета подключения (TCP / UDP)
				scheme_t::sonet_t sonet;
				// Тип протокола интернета (IPV4 / IPV6 / NIX)
				scheme_t::family_t family;
				// Адрес файла unix-сокета
				string filename;
				// Параметры для сети
				vector <string> network;
				/**
				 * Settings Конструктор
				 */
				Settings() noexcept :
				 proto(engine_t::proto_t::RAW),
				 sonet(scheme_t::sonet_t::TCP),
				 family(scheme_t::family_t::IPV4),
				 filename{""}, network{"0.0.0.0","[::]"} {}
			} settings_t;
		private:
			/**
			 * Dispatch Класс работы с событиями
			 */
			typedef class Dispatch {
				private:
					// Core Устанавливаем дружбу с классом ядра
					friend class Core;
				private:
					// Объект ядра
					Core * _core;
				private:
					// Флаг простого чтения базы событий
					bool _easy;
					// Флаг работы модуля
					bool _work;
					// Флаг инициализации базы событий
					bool _init;
					// Флаг виртуальной базы данных
					bool _virt;
					// Флаг заморозки получения данных
					bool _freeze;
				private:
					// Мютекс для блокировки потока
					recursive_mutex _mtx;
				private:
					// Частота обновления базы событий
					chrono::milliseconds _freq;
				public:
					// База данных событий
					struct event_base * base;
				private:
					// Функция обратного вызова при запуске модуля
					function <void (const bool, const bool)> _launching;
					// Функция обратного вызова при остановки модуля
					function <void (const bool, const bool)> _closedown;
				private:
					/**
					 * Если операционной системой является Windows
					 */
					#if defined(_WIN32) || defined(_WIN64)
						// Объект данных запроса
						WSADATA _wsaData;
					#endif
				public:
					/**
					 * kick Метод отправки пинка
					 */
					void kick() noexcept;
					/**
					 * stop Метод остановки чтения базы событий
					 */
					void stop() noexcept;
					/**
					 * start Метод запуска чтения базы событий
					 */
					void start() noexcept;
				public:
					/**
					 * virt Метод активации работы базы событий как виртуальной
					 * @param mode флаг активации
					 */
					void virt(const bool mode) noexcept;
				public:
					/**
					 * rebase Метод пересоздания базы событий
					 */
					void rebase() noexcept;
					/**
					 * freeze Метод заморозки чтения данных
					 * @param mode флаг активации
					 */
					void freeze(const bool mode) noexcept;
					/**
					 * easily Метод активации простого режима чтения базы событий
					 * @param mode флаг активации
					 */
					void easily(const bool mode) noexcept;
				public:
					/**
					 * frequency Метод установки частоты обновления базы событий
					 * @param msec частота обновления базы событий в миллисекундах
					 */
					void frequency(const uint8_t msec = 10) noexcept;
				public:
					/**
					 * Dispatch Конструктор
					 * @param core объект сетевого ядра
					 */
					Dispatch(Core * core) noexcept;
					/**
					 * ~Dispatch Деструктор
					 */
					~Dispatch() noexcept;
			} dispatch_t;
		protected:
			// Идентификатор процесса
			pid_t _pid;
		protected:
			// Флаг разрешения работы
			bool _mode;
			// Флаг запрета вывода информационных сообщений
			bool _noinfo;
		private:
			// Количество подключённых внешних ядер
			uint32_t _cores;
		protected:
			// Объект работы с файловой системой
			fs_t _fs;
			// Хранилище функций обратного вызова
			fn_t _callbacks;
		protected:
			// Создаем объект сети
			net_t _net;
			// Создаём объект работы с URI
			uri_t _uri;
			// Создаём объект для работы с актуатором
			engine_t _engine;
			// Сетевые параметры
			settings_t _settings;
			// Объект для работы с чтением базы событий
			dispatch_t _dispatch;
		private:
			// Объект работы с сигналами
			sig_t _sig;
		protected:
			// Мютекс для блокировки основного потока
			mtx_t _mtx;
		protected:
			// Флаг обработки сигналов
			mode_t _signals;
			// Статус сетевого ядра
			status_t _status;
			// Тип запускаемого ядра
			engine_t::type_t _type;
		private:
			// Список активных таймеров
			map <uint16_t, unique_ptr <timer_t>> _timers;
		protected:
			// Список активных схем сети
			map <uint16_t, const scheme_t *> _schemes;
			// Список активных брокеров
			map <uint64_t, const scheme_t::broker_t *> _brokers;
		protected:
			// Создаём объект DNS-резолвера
			dns_t * _dns;
		protected:
			// Создаём объект фреймворка
			const fmk_t * _fmk;
			// Создаём объект работы с логами
			const log_t * _log;
		private:
			/**
			 * signal Метод вывода полученного сигнала
			 */
			void signal(const int signal) noexcept;
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
		protected:
			/**
			 * clean Метод буфера событий
			 * @param bid идентификатор брокера
			 */
			void clean(const uint64_t bid) const noexcept;
		public:
			/**
			 * rebase Метод пересоздания базы событий
			 */
			void rebase() noexcept;
		public:
			/**
			 * bind Метод подключения модуля ядра к текущей базе событий
			 * @param core модуль ядра для подключения
			 */
			void bind(Core * core) noexcept;
			/**
			 * unbind Метод отключения модуля ядра от текущей базы событий
			 * @param core модуль ядра для отключения
			 */
			void unbind(Core * core) noexcept;
		public:
			/**
			 * stop Метод остановки клиента
			 */
			virtual void stop() noexcept;
			/**
			 * start Метод запуска клиента
			 */
			virtual void start() noexcept;
		public:
			/**
			 * working Метод проверки на запуск работы
			 * @return результат проверки
			 */
			bool working() const noexcept;
		public:
			/**
			 * add Метод добавления схемы сети
			 * @param scheme схема рабочей сети
			 * @return       идентификатор схемы сети
			 */
			uint16_t add(const scheme_t * scheme) noexcept;
		public:
			/**
			 * close Метод отключения всех брокеров
			 */
			virtual void close() noexcept;
			/**
			 * remove Метод удаления всех схем сети
			 */
			virtual void remove() noexcept;
		public:
			/**
			 * close Метод закрытия подключения брокера
			 * @param bid идентификатор брокера
			 */
			virtual void close(const uint64_t bid) noexcept;
			/**
			 * remove Метод удаления схемы сети
			 * @param sid идентификатор схемы сети
			 */
			virtual void remove(const uint16_t sid) noexcept;
		private:
			/**
			 * timeout Метод вызова при срабатывании таймаута
			 * @param bid идентификатор брокера
			 */
			virtual void timeout(const uint64_t bid) noexcept;
			/**
			 * connected Метод вызова при удачном подключении к серверу
			 * @param bid идентификатор брокера
			 */
			virtual void connected(const uint64_t bid) noexcept;
		public:
			/**
			 * read Метод чтения данных для брокера
			 * @param bid идентификатор брокера
			 */
			virtual void read(const uint64_t bid) noexcept;
			/**
			 * write Метод записи буфера данных в сокет
			 * @param buffer буфер для записи данных
			 * @param size   размер записываемых данных
			 * @param bid    идентификатор брокера
			 */
			virtual void write(const char * buffer, const size_t size, const uint64_t bid) noexcept;
		public:
			/**
			 * bandWidth Метод установки пропускной способности сети
			 * @param bid   идентификатор брокера
			 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
			 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
			 */
			virtual void bandWidth(const uint64_t bid, const string & read, const string & write) noexcept;
		public:
			/**
			 * lockup Метод блокировки метода режима работы
			 * @param method метод режима работы
			 * @param mode   флаг блокировки метода
			 * @param bid    идентификатор брокера
			 */
			void lockup(const engine_t::method_t method, const bool mode, const uint64_t bid) noexcept;
			/**
			 * events Метод активации/деактивации метода события сокета
			 * @param mode   сигнал активации сокета
			 * @param method метод события сокета
			 * @param bid    идентификатор брокера
			 */
			void events(const mode_t mode, const engine_t::method_t method, const uint64_t bid) noexcept;
			/**
			 * dataTimeout Метод установки таймаута ожидания появления данных
			 * @param method  метод режима работы
			 * @param seconds время ожидания в секундах
			 * @param bid     идентификатор брокера
			 */
			void dataTimeout(const engine_t::method_t method, const time_t seconds, const uint64_t bid) noexcept;
			/**
			 * marker Метод установки маркера на размер детектируемых байт
			 * @param method метод режима работы
			 * @param min    минимальный размер детектируемых байт
			 * @param min    максимальный размер детектируемых байт
			 * @param bid    идентификатор брокера
			 */
			void marker(const engine_t::method_t method, const size_t min, const size_t max, const uint64_t bid) noexcept;
		public:
			/**
			 * clearTimers Метод очистки всех таймеров
			 */
			void clearTimers() noexcept;
			/**
			 * clearTimer Метод очистки таймера
			 * @param id идентификатор таймера для очистки
			 */
			void clearTimer(const uint16_t id) noexcept;
		public:
			/**
			 * setTimeout Метод установки таймаута
			 * @param delay    задержка времени в миллисекундах
			 * @param callback функция обратного вызова
			 * @return         идентификатор созданного таймера
			 */
			uint16_t setTimeout(const time_t delay, function <void (const uint16_t)> callback) noexcept;
			/**
			 * setInterval Метод установки интервала времени
			 * @param delay    задержка времени в миллисекундах
			 * @param callback функция обратного вызова
			 * @return         идентификатор созданного таймера
			 */
			uint16_t setInterval(const time_t delay, function <void (const uint16_t)> callback) noexcept;
		public:
			/**
			 * callbacks Метод установки функций обратного вызова
			 * @param callbacks функции обратного вызова
			 */
			virtual void callbacks(const fn_t & callbacks) noexcept;
		public:
			/**
			 * callback Шаблон метода установки финкции обратного вызова
			 * @tparam A тип функции обратного вызова
			 */
			template <typename A>
			/**
			 * callback Метод установки функции обратного вызова
			 * @param idw идентификатор функции обратного вызова
			 * @param fn  функция обратного вызова для установки
			 */
			void callback(const uint64_t idw, function <A> fn) noexcept {
				// Если функция обратного вызова передана
				if((idw > 0) && (fn != nullptr))
					// Выполняем установку функции обратного вызова
					this->_callbacks.set <A> (idw, fn);
			}
			/**
			 * callback Шаблон метода установки финкции обратного вызова
			 * @tparam A тип функции обратного вызова
			 */
			template <typename A>
			/**
			 * callback Метод установки функции обратного вызова
			 * @param name название функции обратного вызова
			 * @param fn   функция обратного вызова для установки
			 */
			void callback(const string & name, function <A> fn) noexcept {
				// Если функция обратного вызова передана
				if(!name.empty() && (fn != nullptr))
					// Выполняем установку функции обратного вызова
					this->_callbacks.set <A> (name, fn);
			}
		public:
			/**
			 * easily Метод активации простого режима чтения базы событий
			 * @param mode флаг активации простого чтения базы событий
			 */
			void easily(const bool mode) noexcept;
			/**
			 * freeze Метод заморозки чтения данных
			 * @param mode флаг активации заморозки чтения данных
			 */
			void freeze(const bool mode) noexcept;
		public:
			/**
			 * removeUnixSocket Метод удаления unix-сокета
			 * @return результат выполнения операции
			 */
			bool removeUnixSocket() noexcept;
			/**
			 * unixSocket Метод установки адреса файла unix-сокета
			 * @param socket адрес файла unix-сокета
			 * @return       результат установки unix-сокета
			 */
			bool unixSocket(const string & socket = "") noexcept;
		public:
			/**
			 * proto Метод извлечения поддерживаемого протокола подключения
			 * @return поддерживаемый протокол подключения (RAW, HTTP1, HTTP1_1, HTTP2, HTTP3)
			 */
			engine_t::proto_t proto() const noexcept;
			/**
			 * proto Метод извлечения активного протокола подключения
			 * @param bid идентификатор брокера
			 * @return активный протокол подключения (RAW, HTTP1, HTTP1_1, HTTP2, HTTP3)
			 */
			engine_t::proto_t proto(const uint64_t bid) const noexcept;
			/**
			 * proto Метод установки поддерживаемого протокола подключения
			 * @param proto устанавливаемый протокол (RAW, HTTP1, HTTP1_1, HTTP2, HTTP3)
			 */
			void proto(const engine_t::proto_t proto) noexcept;
		public:
			/**
			 * sonet Метод извлечения типа сокета подключения
			 * @return тип сокета подключения (TCP / UDP / SCTP)
			 */
			scheme_t::sonet_t sonet() const noexcept;
			/**
			 * sonet Метод установки типа сокета подключения
			 * @param sonet тип сокета подключения (TCP / UDP / SCTP)
			 */
			void sonet(const scheme_t::sonet_t sonet) noexcept;
		public:
			/**
			 * family Метод извлечения типа протокола интернета
			 * @return тип протокола интернета (IPV4 / IPV6 / NIX)
			 */
			scheme_t::family_t family() const noexcept;
			/**
			 * family Метод установки типа протокола интернета
			 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
			 */
			void family(const scheme_t::family_t family) noexcept;
		public:
			/**
			 * resolver Метод установки объекта DNS-резолвера
			 * @param dns объект DNS-резолвер
			 */
			void resolver(const dns_t * dns) noexcept;
		public:
			/**
			 * noInfo Метод установки флага запрета вывода информационных сообщений
			 * @param mode флаг запрета вывода информационных сообщений
			 */
			void noInfo(const bool mode) noexcept;
			/**
			 * verifySSL Метод разрешающий или запрещающий, выполнять проверку соответствия, сертификата домену
			 * @param mode флаг состояния разрешения проверки
			 */
			void verifySSL(const bool mode) noexcept;
			/**
			 * frequency Метод установки частоты обновления базы событий
			 * @param msec частота обновления базы событий в миллисекундах
			 */
			void frequency(const uint8_t msec = 10) noexcept;
			/**
			 * signalInterception Метод активации перехвата сигналов
			 * @param mode флаг активации
			 */
			void signalInterception(const mode_t mode) noexcept;
			/**
			 * ciphers Метод установки алгоритмов шифрования
			 * @param ciphers список алгоритмов шифрования для установки
			 */
			void ciphers(const vector <string> & ciphers) noexcept;
			/**
			 * ca Метод установки доверенного сертификата (CA-файла)
			 * @param trusted адрес доверенного сертификата (CA-файла)
			 * @param path    адрес каталога где находится сертификат (CA-файл)
			 */
			void ca(const string & trusted, const string & path = "") noexcept;
			/**
			 * certificate Метод установки файлов сертификата
			 * @param chain файл цепочки сертификатов
			 * @param key   приватный ключ сертификата
			 */
			void certificate(const string & chain, const string & key) noexcept;
		public:
			/**
			 * network Метод установки параметров сети
			 * @param ips    список IP-адресов компьютера с которых разрешено выходить в интернет
			 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
			 * @param sonet  тип сокета подключения (TCP / UDP)
			 */
			void network(const vector <string> & ips = {}, const scheme_t::family_t family = scheme_t::family_t::IPV4, const scheme_t::sonet_t sonet = scheme_t::sonet_t::TCP) noexcept;
		public:
			/**
			 * Core Конструктор
			 * @param fmk    объект фреймворка
			 * @param log    объект для работы с логами
			 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
			 * @param sonet  тип сокета подключения (TCP / UDP / TLS / DTLS)
			 */
			Core(const fmk_t * fmk, const log_t * log, const scheme_t::family_t family = scheme_t::family_t::IPV4, const scheme_t::sonet_t sonet = scheme_t::sonet_t::TCP) noexcept;
			/**
			 * Core Конструктор
			 * @param dns    объект DNS-резолвера
			 * @param fmk    объект фреймворка
			 * @param log    объект для работы с логами
			 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
			 * @param sonet  тип сокета подключения (TCP / UDP / TLS / DTLS)
			 */
			Core(const dns_t * dns, const fmk_t * fmk, const log_t * log, const scheme_t::family_t family = scheme_t::family_t::IPV4, const scheme_t::sonet_t sonet = scheme_t::sonet_t::TCP) noexcept;
			/**
			 * ~Core Деструктор
			 */
			virtual ~Core() noexcept;
	} core_t;
};

#endif // __AWH_CORE__
