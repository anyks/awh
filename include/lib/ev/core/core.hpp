/**
 * @file: core.hpp
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

#ifndef __AWH_CORE__
#define __AWH_CORE__

/**
 * Стандартная библиотека
 */
#include <map>
#include <ctime>
#include <mutex>
#include <future>
#include <chrono>
#include <thread>
#include <string>
#include <cerrno>
#include <cstdlib>
#include <functional>
#include <unordered_map>
#include <libev/ev++.h>

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
			 * Флаги перехвата сигналов
			 */
			enum class signals_t : uint8_t {
				ENABLED  = 0x01, // Включено
				DISABLED = 0x00  // Отключено
			};
		private:
			/**
			 * Timer Класс таймера
			 */
			typedef class Timer {
				public:
					// Идентификатор таймера
					u_short id;
				public:
					// Флаг персистентной работы
					bool persist;
				public:
					// Задержка времени в секундах
					float delay;
				public:
					// Объект события таймера
					ev::timer io;
				public:
					// Объект сетевого ядра
					Core * core;
				public:
					// Внешняя функция обратного вызова
					function <void (const u_short, Core *)> fn;
				public:
					/**
					 * callback Метод обратного вызова
					 * @param timer   объект события таймера
					 * @param revents идентификатор события
					 */
					void callback(ev::timer & timer, int revents) noexcept;
				public:
					/**
					 * Timer Конструктор
					 */
					Timer() noexcept : id(0), persist(false), delay(0.f), core(nullptr), fn(nullptr) {}
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
				// Адрес файла unix-сокета
				string filename;
				// Тип сокета подключения (TCP / UDP)
				scheme_t::sonet_t sonet;
				// Тип протокола интернета (IPV4 / IPV6 / NIX)
				scheme_t::family_t family;
				// Параметры для сети IPv4
				pair <vector <string>, vector <string>> v4;
				// Параметры для сети IPv6
				pair <vector <string>, vector <string>> v6;
				/**
				 * Settings Конструктор
				 */
				Settings() noexcept :
				 filename(""),
				 sonet(scheme_t::sonet_t::TCP),
				 family(scheme_t::family_t::IPV4),
				 v4({{"0.0.0.0"}, {}}), v6({{"[::0]"}, {}}) {}
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
				public:
					// База событий
					ev::loop_ref base;
				private:
					// Мютекс для блокировки потока
					recursive_mutex _mtx;
				private:
					// Частота обновления базы событий
					chrono::milliseconds _freq;
				private:
					// Функция обратного вызова при запуске модуля
					function <void (void)> _launching;
					// Функция обратного вызова при остановки модуля
					function <void (void)> _closedown;
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
					 * freeze Метод заморозки чтения данных
					 * @param mode флаг активации
					 */
					void freeze(const bool mode) noexcept;
					/**
					 * easily Метод активации простого режима чтения базы событий
					 * @param mode флаг активации
					 */
					void easily(const bool mode) noexcept;
					/**
					 * rebase Метод пересоздания базы событий
					 * @param clear флаг очистки предыдущей базы событий
					 */
					void rebase(const bool clear = true) noexcept;
				public:
					/**
					 * setBase Метод установки базы событий
					 * @param base база событий
					 */
					void setBase(struct ev_loop * base) noexcept;
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
			pid_t pid;
		protected:
			// Создаем объект сети
			net_t net;
		protected:
			// Создаём объект работы с URI
			uri_t uri;
			// Создаём объект DNS-резолвера
			dns_t dns;
			// Создаём объект для работы с актуатором
			engine_t engine;
			// Сетевые параметры
			settings_t settings;
			// Объект для работы с чтением базы событий
			dispatch_t dispatch;
		protected:
			// Объект работы с файловой системой
			fs_t _fs;
			// Объект работы с сигналами
			sig_t _sig;
			// Объект события таймера
			timer_t _timer;
		protected:
			// Мютекс для блокировки основного потока
			mutable mtx_t _mtx;
		protected:
			// Статус сетевого ядра
			status_t status;
			// Тип запускаемого ядра
			engine_t::type_t type;
		protected:
			// Флаг обработки сигналов
			signals_t _signals;
		private:
			// Список активных таймеров
			map <u_short, unique_ptr <timer_t>> _timers;
		protected:
			// Список активных схем сети
			map <size_t, const scheme_t *> schemes;
			// Список подключённых клиентов
			map <size_t, const scheme_t::adj_t *> adjutants;
		protected:
			// Флаг разрешения работы
			bool mode;
			// Флаг запрета вывода информационных сообщений
			bool noinfo;
			// Флаг персистентного запуска каллбека
			bool persist;
			// Флаг вывода запуска функции активности в отдельном потоке
			bool activeOnTrhead;
		protected:
			// Количество подключённых внешних ядер
			u_int cores;
		protected:
			// Название сервера по умолчанию
			string servName;
		private:
			// Интервал персистентного таймера в миллисекундах
			time_t _persIntvl;
		protected:
			// Создаём объект фреймворка
			const fmk_t * fmk;
			// Создаём объект работы с логами
			const log_t * log;
		protected:
			// Функция обратного вызова при краше приложения
			function <void (const int)> _crashFn;
			// Функция обратного вызова при запуске/остановке модуля
			function <void (const status_t, Core *)> _activeFn;
		private:
			/**
			 * launching Метод вызова при активации базы событий
			 */
			void launching() noexcept;
			/**
			 * closedown Метод вызова при деакцтивации базы событий
			 */
			void closedown() noexcept;
		private:
			/**
			 * persistent Метод персистентного вызова по таймеру
			 * @param timer   объект события таймера
			 * @param revents идентификатор события
			 */
			void persistent(ev::timer & timer, int revents) noexcept;
		private:
			/**
			 * signal Метод вывода полученного сигнала
			 */
			void signal(const int signal) noexcept;
		protected:
			/**
			 * clean Метод буфера событий
			 * @param aid идентификатор адъютанта
			 */
			void clean(const size_t aid) const noexcept;
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
			 * crash Метод установки функции обратного вызова при краше приложения
			 * @param callback функция обратного вызова для установки
			 */
			void crash(function <void (const int)> callback) noexcept;
			/**
			 * callback Метод установки функции обратного вызова при запуске/остановки работы модуля
			 * @param callback функция обратного вызова для установки
			 */
			void callback(function <void (const status_t, Core *)> callback) noexcept;
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
			size_t add(const scheme_t * scheme) noexcept;
		public:
			/**
			 * close Метод отключения всех адъютантов
			 */
			virtual void close() noexcept;
			/**
			 * remove Метод удаления всех схем сети
			 */
			virtual void remove() noexcept;
		public:
			/**
			 * close Метод закрытия подключения адъютанта
			 * @param aid идентификатор адъютанта
			 */
			virtual void close(const size_t aid) noexcept;
			/**
			 * remove Метод удаления схемы сети
			 * @param sid идентификатор схемы сети
			 */
			virtual void remove(const size_t sid) noexcept;
		private:
			/**
			 * timeout Метод вызова при срабатывании таймаута
			 * @param aid идентификатор адъютанта
			 */
			virtual void timeout(const size_t aid) noexcept;
			/**
			 * connected Метод вызова при удачном подключении к серверу
			 * @param aid идентификатор адъютанта
			 */
			virtual void connected(const size_t aid) noexcept;
			/**
			 * transfer Метед передачи данных между клиентом и сервером
			 * @param method метод режима работы
			 * @param aid    идентификатор адъютанта
			 */
			virtual void transfer(const engine_t::method_t method, const size_t aid) noexcept;
		public:
			/**
			 * bandWidth Метод установки пропускной способности сети
			 * @param aid   идентификатор адъютанта
			 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
			 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
			 */
			virtual void bandWidth(const size_t aid, const string & read, const string & write) noexcept;
		public:
			/**
			 * rebase Метод пересоздания базы событий
			 */
			void rebase() noexcept;
		public:
			/**
			 * method Метод получения текущего метода работы
			 * @param aid идентификатор адъютанта
			 * @return    результат работы функции
			 */
			engine_t::method_t method(const size_t aid) const noexcept;
		public:
			/**
			 * enabled Метод активации метода события сокета
			 * @param method метод события сокета
			 * @param aid    идентификатор адъютанта
			 */
			void enabled(const engine_t::method_t method, const size_t aid) noexcept;
			/**
			 * disabled Метод деактивации метода события сокета
			 * @param method метод события сокета
			 * @param aid    идентификатор адъютанта
			 */
			void disabled(const engine_t::method_t method, const size_t aid) noexcept;
		public:
			/**
			 * write Метод записи буфера данных в сокет
			 * @param buffer буфер для записи данных
			 * @param size   размер записываемых данных
			 * @param aid    идентификатор адъютанта
			 */
			void write(const char * buffer, const size_t size, const size_t aid) noexcept;
		public:
			/**
			 * lockMethod Метод блокировки метода режима работы
			 * @param method метод режима работы
			 * @param mode   флаг блокировки метода
			 * @param aid    идентификатор адъютанта
			 */
			void lockMethod(const engine_t::method_t method, const bool mode, const size_t aid) noexcept;
			/**
			 * dataTimeout Метод установки таймаута ожидания появления данных
			 * @param method  метод режима работы
			 * @param seconds время ожидания в секундах
			 * @param aid     идентификатор адъютанта
			 */
			void dataTimeout(const engine_t::method_t method, const time_t seconds, const size_t aid) noexcept;
			/**
			 * marker Метод установки маркера на размер детектируемых байт
			 * @param method метод режима работы
			 * @param min    минимальный размер детектируемых байт
			 * @param min    максимальный размер детектируемых байт
			 * @param aid    идентификатор адъютанта
			 */
			void marker(const engine_t::method_t method, const size_t min, const size_t max, const size_t aid) noexcept;
		public:
			/**
			 * clearTimers Метод очистки всех таймеров
			 */
			void clearTimers() noexcept;
			/**
			 * clearTimer Метод очистки таймера
			 * @param id идентификатор таймера для очистки
			 */
			void clearTimer(const u_short id) noexcept;
		public:
			/**
			 * setTimeout Метод установки таймаута
			 * @param delay    задержка времени в миллисекундах
			 * @param callback функция обратного вызова
			 * @return         идентификатор созданного таймера
			 */
			u_short setTimeout(const time_t delay, function <void (const u_short, Core *)> callback) noexcept;
			/**
			 * setInterval Метод установки интервала времени
			 * @param delay    задержка времени в миллисекундах
			 * @param callback функция обратного вызова
			 * @return         идентификатор созданного таймера
			 */
			u_short setInterval(const time_t delay, function <void (const u_short, Core *)> callback) noexcept;
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
			 * sonet Метод извлечения типа сокета подключения
			 * @return тип сокета подключения (TCP / UDP / SCTP)
			 */
			scheme_t::sonet_t sonet() const noexcept;
			/**
			 * sonet Метод установки типа сокета подключения
			 * @param sonet тип сокета подключения (TCP / UDP / SCTP)
			 */
			void sonet(const scheme_t::sonet_t sonet = scheme_t::sonet_t::TCP) noexcept;
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
			void family(const scheme_t::family_t family = scheme_t::family_t::IPV4) noexcept;
		public:
			/**
			 * clearDNS Метод сброса кэша резолвера
			 * @return результат работы функции
			 */
			bool clearDNS() noexcept;
			/**
			 * flushDNS Метод сброса кэша DNS-резолвера
			 * @return результат работы функции
			 */
			bool flushDNS() noexcept;
		public:
			/**
			 * timeoutDNS Метод установки времени ожидания выполнения DNS запроса
			 * @param sec интервал времени ожидания в секундах
			 */
			void timeoutDNS(const uint8_t sec) noexcept;
		public:
			/**
			 * cashTimeToLiveDNS Время жизни кэша DNS
			 * @param msec время жизни в миллисекундах
			 */
			void cashTimeToLiveDNS(const time_t msec) noexcept;
		public:
			/**
			 * serverDNS Метод установки серверов имён DNS
			 * @param ns список серверов имён
			 */
			void serverDNS(const vector <string> & ns) noexcept;
			/**
			 * serverDNS Метод установки серверов имён DNS
			 * @param ns     список серверов имён
			 * @param family тип протокола интернета (IPV4 / IPV6)
			 */
			void serverDNS(const vector <string> & ns, const scheme_t::family_t family) noexcept;
		public:
			/**
			 * clearBlackListDNS Метод очистки чёрного списка
			 * @param domain доменное имя соответствующее IP-адресу
			 */
			void clearBlackListDNS(const string & domain) noexcept;
			/**
			 * clearBlackListDNS Метод очистки чёрного списка
			 * @param family тип протокола интернета (IPV4 / IPV6)
			 * @param domain доменное имя соответствующее IP-адресу
			 */
			void clearBlackListDNS(const scheme_t::family_t family, const string & domain) noexcept;
		public:
			/**
			 * delInBlackListDNS Метод удаления IP-адреса из чёрного списока
			 * @param domain доменное имя соответствующее IP-адресу
			 * @param ip     адрес для удаления из чёрного списка
			 */
			void delInBlackListDNS(const string & domain, const string & ip) noexcept;
			/**
			 * delInBlackListDNS Метод удаления IP-адреса из чёрного списока
			 * @param family тип протокола интернета (IPV4 / IPV6)
			 * @param domain доменное имя соответствующее IP-адресу
			 * @param ip     адрес для удаления из чёрного списка
			 */
			void delInBlackListDNS(const scheme_t::family_t family, const string & domain, const string & ip) noexcept;
		public:
			/**
			 * setToBlackListDNS Метод добавления IP-адреса в чёрный список
			 * @param domain доменное имя соответствующее IP-адресу
			 * @param ip     адрес для добавления в чёрный список
			 */
			void setToBlackListDNS(const string & domain, const string & ip) noexcept;
			/**
			 * setToBlackListDNS Метод добавления IP-адреса в чёрный список
			 * @param family тип протокола интернета (IPV4 / IPV6)
			 * @param domain доменное имя соответствующее IP-адресу
			 * @param ip     адрес для добавления в чёрный список
			 */
			void setToBlackListDNS(const scheme_t::family_t family, const string & domain, const string & ip) noexcept;
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
			 * persistEnable Метод установки персистентного флага
			 * @param mode флаг персистентного запуска каллбека
			 */
			void persistEnable(const bool mode) noexcept;
			/**
			 * persistInterval Метод установки персистентного таймера
			 * @param itv интервал персистентного таймера в миллисекундах
			 */
			void persistInterval(const time_t itv) noexcept;
			/**
			 * frequency Метод установки частоты обновления базы событий
			 * @param msec частота обновления базы событий в миллисекундах
			 */
			void frequency(const uint8_t msec = 10) noexcept;
			/**
			 * serverName Метод добавления названия сервера
			 * @param name название сервера для добавления
			 */
			void serverName(const string & name = "") noexcept;
			/**
			 * signalInterception Метод активации перехвата сигналов
			 * @param mode флаг активации
			 */
			void signalInterception(const signals_t mode) noexcept;
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
			 * ~Core Деструктор
			 */
			virtual ~Core() noexcept;
	} core_t;
};

#endif // __AWH_CORE__
