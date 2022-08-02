/**
 * @file: core.hpp
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

#ifndef __AWH_CORE__
#define __AWH_CORE__

/**
 * Стандартная библиотека
 */
#include <map>
#include <mutex>
#include <chrono>
#include <thread>
#include <string>
#include <functional>
#include <unordered_map>
#include <errno.h>
#include <libev/ev++.h>

/**
 * Если операционной системой является Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	// Подключаем заголовочный файл
	#include <time.h>
	#include <tchar.h>
	#include <stdlib.h>
	#include <signal.h>
	#include <synchapi.h>
/**
 * Для всех остальных операционных систем
 */
#else
	// Подключаем заголовочный файл
	#include <ctime>
	#include <unistd.h>
#endif

/**
 * Наши модули
 */
#include <net/engine.hpp>
// #include <net/dns.hpp>
#include <worker/core.hpp>

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
			 * Worker Устанавливаем дружбу с классом воркера
			 */
			friend class Worker;
		protected:
			/**
			 * Статус работы сетевого ядра
			 */
			enum class status_t : uint8_t {STOP, START};
		public:
			/**
			 * Тип сокета подключения
			 */
			enum class sonet_t : uint8_t {TCP_SOCK, UDP_SOCK};
			/**
			 * Тип интернет-подключения
			 */
			enum class family_t : uint8_t {IPV4, IPV6, SONIX};
			/**
			 * Основные методы режимов работы
			 */
			enum class method_t : uint8_t {READ, WRITE, CONNECT};
		private:
			/**
			 * Методы только для OS Windows
			 */
			#if defined(_WIN32) || defined(_WIN64)
				// Устанавливаем прототип функции обработчика сигнала
				typedef void (* SignalHandlerPointer)(int);
				/**
				 * Signals Структура событий сигналов
				 */
				typedef struct Signals {
					SignalHandlerPointer sint;  // Перехватчик сигнала SIGINT
					SignalHandlerPointer sfpe;  // Перехватчик сигнала SIGFPE
					SignalHandlerPointer sill;  // Перехватчик сигнала SIGILL
					SignalHandlerPointer sabrt; // Перехватчик сигнала SIGABRT
					SignalHandlerPointer sterm; // Перехватчик сигнала SIGTERM
					SignalHandlerPointer ssegv; // Перехватчик сигнала SIGSEGV
				} sig_t;
			#endif
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
					// Штамп времени исполнения
					time_t stamp;
				public:
					// Объект события таймера
					ev::timer io;
				public:
					// Родительский объект
					Core * core;
				public:
					// Внешняя функция обратного вызова
					function <void (const u_short, Core *)> fn;
				public:
					/**
					 * callback Функция обратного вызова
					 * @param timer   объект события таймера
					 * @param revents идентификатор события
					 */
					void callback(ev::timer & timer, int revents) noexcept;
				public:
					/**
					 * Timer Конструктор
					 */
					Timer() noexcept : id(0), persist(false), delay(0.f), stamp(0), core(nullptr), fn(nullptr) {}
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
				recursive_mutex worker; // Для работы с воркерами
			} mtx_t;
			/**
			 * Network Структура текущих параметров сети
			 */
			typedef struct Network {
				// Тип сокета подключения (TCP_SOCK / UDP_SOCK)
				sonet_t sonet;
				// Тип протокола интернета (IPV4 / IPV6 / SONIX)
				family_t family;
				// Адрес файла unix-сокета
				string filename;
				// Параметры для сети IPv4
				pair <vector <string>, vector <string>> v4;
				// Параметры для сети IPv6
				pair <vector <string>, vector <string>> v6;
				/**
				 * Network Конструктор
				 */
				Network() noexcept :
				 sonet(sonet_t::TCP_SOCK), family(family_t::IPV4), filename(""),
				 v4({{"0.0.0.0"}, IPV4_RESOLVER}), v6({{"[::0]"}, IPV6_RESOLVER}) {}
			} net_t;
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
					Core * core;
				private:
					// Флаг простого чтения базы событий
					bool easy;
					// Флаг разрешения работы
					bool mode;
					// Флаг работы модуля
					bool work;
					// Флаг инициализации базы событий
					bool init;
				public:
					// База событий
					ev::loop_ref base;
				private:
					// Мютекс для блокировки потока
					recursive_mutex mtx;
				private:
					// Частота обновления базы событий
					chrono::milliseconds freq;
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
					 * setFrequency Метод установки частоты обновления базы событий
					 * @param msec частота обновления базы событий в миллисекундах
					 */
					void setFrequency(const uint8_t msec = 10) noexcept;
				public:
					/**
					 * Dispatch Конструктор
					 */
					Dispatch(Core * core) noexcept;
					/**
					 * ~Dispatch Деструктор
					 */
					~Dispatch() noexcept;
			} dispatch_t;
		protected:
			// Создаем объект сети
			network_t nwk;
		protected:
			// Создаём объект работы с URI
			uri_t uri;
			// Сетевые параметры
			net_t net;
			// Создаём объект для работы с актуатором
			engine_t engine;
			/*
			// Создаём объект DNS IPv4 резолвера
			dns_t dns4;
			// Создаём объект DNS IPv6 резолвера
			dns_t dns6;
			*/
			// Объект для работы с чтением базы событий
			dispatch_t dispatch;
		private:
			// Объект события таймера
			timer_t timer;
		protected:
			// Мютекс для блокировки основного потока
			mutable mtx_t mtx;
		private:
			/**
			 * Методы только для OS Windows
			 */
			#if defined(_WIN32) || defined(_WIN64)
				// Объект работы с сигналами
				sig_t sig;
			#endif
		protected:
			// Статус сетевого ядра
			status_t status = status_t::STOP;
			// Тип запускаемого ядра
			engine_t::type_t type = engine_t::type_t::CLIENT;
		private:
			// Список активных таймеров
			map <u_short, unique_ptr <timer_t>> timers;
		protected:
			// Список активных воркеров
			map <size_t, const worker_t *> workers;
			// Список подключённых клиентов
			map <size_t, const worker_t::adj_t *> adjutants;
		protected:
			// Флаг разрешения работы
			bool mode = false;
			// Флаг запрета вывода информационных сообщений
			bool noinfo = false;
			// Флаг персистентного запуска каллбека
			bool persist = false;
		protected:
			// Количество подключённых внешних ядер
			u_int cores = 0;
		protected:
			// Название сервера по умолчанию
			string serverName = AWH_SHORT_NAME;
		private:
			// Интервал персистентного таймера в миллисекундах
			time_t persistInterval = PERSIST_INTERVAL;
		protected:
			// Создаём объект фреймворка
			const fmk_t * fmk = nullptr;
			// Создаём объект работы с логами
			const log_t * log = nullptr;
		protected:
			// Функция обратного вызова при запуске/остановке модуля
			function <void (const bool, Core * core)> callbackFn = nullptr;
		private:
			/**
			 * launching Метод вызова при активации базы событий
			 */
			void launching() noexcept;
			/**
			 * closedown Метод вызова при деакцтивации базы событий
			 */
			void closedown() noexcept;
		protected:
			/**
			 * executeTimers Метод принудительного исполнения работы таймеров
			 */
			void executeTimers() noexcept;
		private:
			/**
			 * persistent Функция персистентного вызова по таймеру
			 * @param timer   объект события таймера
			 * @param revents идентификатор события
			 */
			void persistent(ev::timer & timer, int revents) noexcept;
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
			 * setCallback Метод установки функции обратного вызова при запуске/остановки работы модуля
			 * @param callback функция обратного вызова для установки
			 */
			void setCallback(function <void (const bool, Core * core)> callback) noexcept;
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
			 * add Метод добавления воркера в биндинг
			 * @param worker воркер для добавления
			 * @return       идентификатор воркера в биндинге
			 */
			size_t add(const worker_t * worker) noexcept;
		public:
			/**
			 * close Метод отключения всех воркеров
			 */
			virtual void close() noexcept;
			/**
			 * remove Метод удаления всех воркеров
			 */
			virtual void remove() noexcept;
		public:
			/**
			 * close Метод закрытия подключения воркера
			 * @param aid идентификатор адъютанта
			 */
			virtual void close(const size_t aid) noexcept;
			/**
			 * remove Метод удаления воркера из биндинга
			 * @param wid идентификатор воркера
			 */
			virtual void remove(const size_t wid) noexcept;
		private:
			/**
			 * timeout Функция обратного вызова при срабатывании таймаута
			 * @param aid идентификатор адъютанта
			 */
			virtual void timeout(const size_t aid) noexcept;
			/**
			 * connected Функция обратного вызова при удачном подключении к серверу
			 * @param aid идентификатор адъютанта
			 */
			virtual void connected(const size_t aid) noexcept;
			/**
			 * write Функция обратного вызова при записи данных в сокет
			 * @param method метод режима работы
			 * @param aid    идентификатор адъютанта
			 */
			virtual void transfer(const method_t method, const size_t aid) noexcept;
		public:
			/**
			 * setBandwidth Метод установки пропускной способности сети
			 * @param aid   идентификатор адъютанта
			 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
			 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
			 */
			virtual void setBandwidth(const size_t aid, const string & read, const string & write) noexcept;
		public:
			/**
			 * rebase Метод пересоздания базы событий
			 */
			void rebase() noexcept;
		public:
			/**
			 * enabled Метод активации метода события сокета
			 * @param method метод события сокета
			 * @param aid    идентификатор адъютанта
			 */
			void enabled(const method_t method, const size_t aid) noexcept;
			/**
			 * disabled Метод деактивации метода события сокета
			 * @param method метод события сокета
			 * @param aid    идентификатор адъютанта
			 */
			void disabled(const method_t method, const size_t aid) noexcept;
		public:
			/**
			 * write Метод записи буфера данных воркером
			 * @param buffer буфер для записи данных
			 * @param size   размер записываемых данных
			 * @param aid    идентификатор адъютанта
			 */
			void write(const char * buffer, const size_t size, const size_t aid) noexcept;
		public:
			/**
			 * setLockMethod Метод блокировки метода режима работы
			 * @param method метод режима работы
			 * @param mode   флаг блокировки метода
			 * @param aid    идентификатор адъютанта
			 */
			void setLockMethod(const method_t method, const bool mode, const size_t aid) noexcept;
			/**
			 * setDataTimeout Метод установки таймаута ожидания появления данных
			 * @param method  метод режима работы
			 * @param seconds время ожидания в секундах
			 * @param aid     идентификатор адъютанта
			 */
			void setDataTimeout(const method_t method, const time_t seconds, const size_t aid) noexcept;
			/**
			 * setMark Метод установки маркера на размер детектируемых байт
			 * @param method метод режима работы
			 * @param min    минимальный размер детектируемых байт
			 * @param min    максимальный размер детектируемых байт
			 * @param aid    идентификатор адъютанта
			 */
			void setMark(const method_t method, const size_t min, const size_t max, const size_t aid) noexcept;
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
			 * unsetUnixSocket Метод удаления unix-сокета
			 * @return результат выполнения операции
			 */
			bool unsetUnixSocket() noexcept;
			/**
			 * setUnixSocket Метод установки адреса файла unix-сокета
			 * @param socket адрес файла unix-сокета
			 * @return       результат установки unix-сокета
			 */
			bool setUnixSocket(const string & socket = "") noexcept;
		public:
			/**
			 * sonet Метод извлечения типа сокета подключения
			 * @return тип сокета подключения (TCP_SOCK / UDP_SOCK)
			 */
			sonet_t sonet() const noexcept;
			/**
			 * family Метод извлечения типа протокола интернета
			 * @return тип протокола интернета (IPV4 / IPV6 / SONIX)
			 */
			family_t family() const noexcept;
		public:
			/**
			 * setNoInfo Метод установки флага запрета вывода информационных сообщений
			 * @param mode флаг запрета вывода информационных сообщений
			 */
			void setNoInfo(const bool mode) noexcept;
			/**
			 * setPersist Метод установки персистентного флага
			 * @param mode флаг персистентного запуска каллбека
			 */
			void setPersist(const bool mode) noexcept;
			/**
			 * setVerifySSL Метод разрешающий или запрещающий, выполнять проверку соответствия, сертификата домену
			 * @param mode флаг состояния разрешения проверки
			 */
			void setVerifySSL(const bool mode) noexcept;
			/**
			 * setPersistInterval Метод установки персистентного таймера
			 * @param itv интервал персистентного таймера в миллисекундах
			 */
			void setPersistInterval(const time_t itv) noexcept;
			/**
			 * setFrequency Метод установки частоты обновления базы событий
			 * @param msec частота обновления базы событий в миллисекундах
			 */
			void setFrequency(const uint8_t msec = 10) noexcept;
			/**
			 * setServerName Метод добавления названия сервера
			 * @param name название сервера для добавления
			 */
			void setServerName(const string & name = "") noexcept;
			/**
			 * setCipher Метод установки алгоритмов шифрования
			 * @param cipher список алгоритмов шифрования для установки
			 */
			void setCipher(const vector <string> & cipher) noexcept;
			/**
			 * setFamily Метод установки тип протокола интернета
			 * @param family тип протокола интернета (IPV4 / IPV6 / SONIX)
			 */
			void setFamily(const family_t family = family_t::IPV4) noexcept;
			/**
			 * setSockType Метод установки типа сокета подключения
			 * @param sonet тип сокета подключения (TCP_SOCK / UDP_SOCK)
			 */
			void setSockType(const sonet_t sonet = sonet_t::TCP_SOCK) noexcept;
			/**
			 * setTrusted Метод установки доверенного сертификата (CA-файла)
			 * @param trusted адрес доверенного сертификата (CA-файла)
			 * @param path    адрес каталога где находится сертификат (CA-файл)
			 */
			void setTrusted(const string & trusted, const string & path = "") noexcept;
			/**
			 * setNet Метод установки параметров сети
			 * @param ip     список IP адресов компьютера с которых разрешено выходить в интернет
			 * @param ns     список серверов имён, через которые необходимо производить резолвинг доменов
			 * @param family тип протокола интернета (IPV4 / IPV6 / SONIX)
			 * @param sonet  тип сокета подключения (TCP_SOCK / UDP_SOCK)
			 */
			void setNet(const vector <string> & ip = {}, const vector <string> & ns = {}, const family_t family = family_t::IPV4, const sonet_t sonet = sonet_t::TCP_SOCK) noexcept;
		public:
			/**
			 * Core Конструктор
			 * @param fmk    объект фреймворка
			 * @param log    объект для работы с логами
			 * @param family тип протокола интернета (IPV4 / IPV6 / SONIX)
			 * @param sonet  тип сокета подключения (TCP_SOCK / UDP_SOCK)
			 */
			Core(const fmk_t * fmk, const log_t * log, const family_t family = family_t::IPV4, const sonet_t sonet = sonet_t::TCP_SOCK) noexcept;
			/**
			 * ~Core Деструктор
			 */
			virtual ~Core() noexcept;
	} core_t;
};

#endif // __AWH_CORE__
