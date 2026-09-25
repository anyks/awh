/**
 * @file: base.cpp
 * @date: 2024-06-26
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

/**
 * Если максимальное количество файловых дескрипторов не передано
 */
#ifndef AWH_MAX_COUNT_FDS
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		/**
		 * Устанавливаем максимальное количество доступных файловых дескрипторов 16384
		 */
		#define AWH_MAX_COUNT_FDS 0x4000
	/**
	 * Для всех остальных операционных систем
	 */
	#else
		/**
		 * Устанавливаем максимальное количество доступных файловых дескрипторов 131072
		 */
		#define AWH_MAX_COUNT_FDS 0x20000
	#endif
#endif

/**
 * Подключаем заголовочный файл
 */
#include <events/base.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * @brief Страж флага блокировки опроса базы событий
 *
 * Флаг снимается на любом пути выхода из метода, включая досрочный возврат
 * и исключение. Иначе опрос базы событий останавливается навсегда.
 */
namespace {
	class LockerGuard {
		private:
			// Флаг блокировки опроса базы событий
			std::atomic_bool & _locker;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param locker флаг блокировки опроса базы событий
			 */
			explicit LockerGuard(std::atomic_bool & locker) noexcept : _locker(locker) {
				// Выполняем блокировку чтения базы событий
				this->_locker = true;
			}
			/**
			 * @brief Деструктор
			 *
			 */
			~LockerGuard() noexcept {
				// Выполняем разблокировку чтения базы событий
				this->_locker = false;
			}
	};
};

/**
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
	/**
	 * @brief Метод проверки на инициализацию WinSocksAPI
	 *
	 * @return результат проверки
	 */
	static bool winsockInitialized() noexcept {
		// Выполняем создание сокета
		SOCKET sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		// Если сокет не создан
		if(sock == INVALID_SOCKET)
			// Сообщаем, что сокет не создан а значит WinSocksAPI не инициализирован
			return false;
		// Выполняем закрытие открытого сокета
		::closesocket(sock);
		// Сообщаем, что WinSocksAPI уже инициализирован
		return true;
	}
#endif
/**
 * @brief Метод получения идентификатора потока
 *
 * @return идентификатор потока для получения
 */
uint64_t awh::Base::wid() const noexcept {
	// Создаём объект хэширования
	std::hash <std::thread::id> hasher;
	// Устанавливаем идентификатор потока
	return hasher(std::this_thread::get_id());
}
/**
 * @brief Метод проверки запущен ли модуль в дочернем потоке
 *
 * @return результат проверки
 */
bool awh::Base::isChildThread() const noexcept {
	// Выполняем проверку
	return (this->_wid != this->wid());
}
/**
 * @brief Метод применение сетевой оптимизации операционной системы
 *
 * @return результат работы
 */
void awh::Base::boostingNetwork() const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#if !_WIN32 && !_WIN64
				// Структура лимитов дампов
				struct rlimit limit;
				// Устанавливаем текущий лимит равный бесконечности
				limit.rlim_cur = RLIM_INFINITY;
				// Устанавливаем максимальный лимит равный бесконечности
				limit.rlim_max = RLIM_INFINITY;
				// Выводим результат установки лимита дампов ядра
				if(::setrlimit(RLIMIT_CORE, &limit) != 0){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, ::strerror(errno));
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
					#endif
				}
			#endif
		#endif
		/**
		 * Выполняем установку нужного нам количества файловых дескрипторов
		 */
		if(!this->_fds.limit(AWH_MAX_COUNT_FDS)){
			// Получаем лимиты файловых дескрипторов
			const auto & limits = this->_fds.limit();
			// Если текущий лимит меньше желаемого
			if(limits.first < AWH_MAX_COUNT_FDS)
				// Выводим сообщение подсказки
				this->_fds.help(limits.first, AWH_MAX_COUNT_FDS);
		}
		/**
		 * Если необходимо выполнить тюннинг операционной системы
		 */
		#if AWH_BOOSTING_NET
			/**
			 * Для операционной системы MS Windows
			 */
			#if _WIN32 || _WIN64
				// Если эффективный идентификатор пользователя принадлежит Administrator
				if(this->_os.isAdmin()){
					// Vista/7 также включает «Compound TCP (CTCP)», который похож на CUBIC в Linux
					this->_os.exec("netsh interface tcp set global congestionprovider=ctcp");
					// Если вам вообще нужно включить автонастройку, вот команды
					this->_os.exec("netsh interface tcp set global autotuninglevel=normal");
				// Если пользователь не является суперпользователем
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, "Administrator privileges are required to apply network optimizations");
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::WARNING, "Administrator privileges are required to apply network optimizations");
					#endif
				}
			/**
			 * Реализация под Sun Solaris
			 */
			#elif __sun__
				// Если эффективный идентификатор пользователя принадлежит ROOT
				if(this->_os.isAdmin()){
					// Эмпирическое правило: max_buf = 2 x cwnd_max (окно перегрузки)
					this->_os.exec("ndd -set /dev/tcp tcp_max_buf 4194304");
					this->_os.exec("ndd -set /dev/tcp tcp_cwnd_max 2097152");
					// Увеличиваем размер окна TCP по умолчанию
					this->_os.exec("ndd -set /dev/tcp tcp_xmit_hiwat 65536");
					this->_os.exec("ndd -set /dev/tcp tcp_recv_hiwat 65536");
				// Если пользователь не является суперпользователем
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, "Root privileges are required to apply network optimizations");
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::WARNING, "Root privileges are required to apply network optimizations");
					#endif
				}
			/**
			 * Для операционной системы MacOS X
			 */
			#elif __APPLE__ || __MACH__
				// Если эффективный идентификатор пользователя принадлежит ROOT
				if(this->_os.isAdmin()){
					// Устанавливаем максимальное количество подключений
					this->_os.sysctl("kern.ipc.somaxconn", 49152);
					/**
					 * Для хостов 10G было бы неплохо увеличить это значение,
					 * т.к. 4G, похоже, является пределом для некоторых установок MacOS X
					 */
					this->_os.sysctl("kern.ipc.maxsockbuf", 6291456);
					// Увеличиваем максимальный размер буферов для отправки
					this->_os.sysctl("net.inet.tcp.sendspace", 1042560);
					// Увеличиваем максимальный размер буферов для чтения
					this->_os.sysctl("net.inet.tcp.recvspace", 1042560);
					// В MacOS X значение по умолчанию 3, что очень мало
					this->_os.sysctl("net.inet.tcp.r", 8);
					// Увеличиваем максимумы автонастройки MacOS X TCP
					this->_os.sysctl("net.inet.tcp.autorcvbufmax", 33554432);
					this->_os.sysctl("net.inet.tcp.autosndbufmax", 33554432);
					// Устанавливаем прочие настройки
					this->_os.sysctl("net.inet.tcp.slowstart_flightsize", 20);
					this->_os.sysctl("net.inet.tcp.local_slowstart_flightsize", 20);
				// Если пользователь не является суперпользователем
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, "Root privileges are required to apply network optimizations");
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::WARNING, "Root privileges are required to apply network optimizations");
					#endif
				}
			/**
			 * Для операционной системы Linux
			 */
			#elif __linux__
				// Если эффективный идентификатор пользователя принадлежит ROOT
				if(this->_os.isAdmin()){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Для отладки активируем создание дампов ядра
						this->_os.sysctl("kernel.core_uses_pid", 1);
						this->_os.sysctl("kernel.core_pattern", "/tmp/%e-%p.core");
					#endif
					// Разрешаем выборочные подтверждения (Selective Acknowledgements, SACK)
					this->_os.sysctl("net.ipv4.tcp_sack", 1);
					// Активируем параметр помогающий в борье за ресурсы
					this->_os.sysctl("net.ipv4.tcp_tw_reuse", 1);
					// Разрешаем использование временных меток (timestamps) в протоколах TCP
					this->_os.sysctl("net.ipv4.tcp_timestamps", 1);
					// Устанавливаем максимальное количество подключений
					this->_os.sysctl("net.core.somaxconn", 49152);
					// Увеличиваем максимальный размер буферов для чтения
					this->_os.sysctl("net.core.rmem_max", 16777216);
					// Увеличиваем максимальный размер буферов для отправки
					this->_os.sysctl("net.core.wmem_max", 16777216);
					// Разрешаем масштабирование TCP-окна
					this->_os.sysctl("net.ipv4.tcp_window_scaling", 1);
					// Запрещаем сохранять результаты измерений TCP-соединения в кэше при его закрытии
					this->_os.sysctl("net.ipv4.tcp_no_metrics_save", 1);
					// Включаем автоматическую настройку размера приёмного буфера TCP
					this->_os.sysctl("net.ipv4.tcp_moderate_rcvbuf", 1);
					// Определяем максимальное количество входящих пакетов
					this->_os.sysctl("net.core.netdev_max_backlog", 2500);
					// Увеличиваем лимит автонастройки TCP-буфера Linux до 64 МБ
					this->_os.sysctl("net.ipv4.tcp_rmem", "\"4096 87380 16777216\"");
					this->_os.sysctl("net.ipv4.tcp_wmem", "\"4096 65536 16777216\"");
					// Рекомендуется для хостов с включенными большими фреймами
					this->_os.sysctl("net.ipv4.tcp_mtu_probing", 1);
					// Рекомендуется для хостов CentOS 7/Debian 8
					this->_os.sysctl("net.core.default_qdisc", "fq");
					/**
					 * Рекомендуемый контроль перегрузки по умолчанию — htcp.
					 * Вы можете проверить, какие доступны алгоритмы получения доступных сообщений, используя net.ipv4.tcp_available_congestion_control
					 */
					const string & algorithm = this->_os.sysctl <string> ("net.ipv4.tcp_available_congestion_control");
					// Если выбран лучший доступны алгоритм
					if(!algorithm.empty()){
						// Если найден алгоритм cubic
						if(this->_fmk->exists("cubic", algorithm))
							// Активируем выбранный нами алгоритм
							this->_os.sysctl("net.ipv4.tcp_congestion_control", "cubic");
						// Если же найден алгоритм htcp
						else if(this->_fmk->exists("htcp", algorithm))
							// Активируем выбранный нами алгоритм
							this->_os.sysctl("net.ipv4.tcp_congestion_control", "htcp");
					}
				// Если пользователь не является суперпользователем
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, "Root privileges are required to apply network optimizations");
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::WARNING, "Root privileges are required to apply network optimizations");
					#endif
				}
			/**
			 * Для операционной системы FreeBSD, NetBSD или OpenBSD
			 */
			#elif __FreeBSD__ || __NetBSD__ || __OpenBSD__
				// Если эффективный идентификатор пользователя принадлежит ROOT
				if(this->_os.isAdmin()){
					/**
					 * Данные оптимизаций операционной системы берет от сюда: http://fasterdata.es.net/host-tuning/freebsd
					 */
					// Активируем контроль работы временной марки и масштабируемого окна
					this->_os.sysctl("net.inet.tcp.rfc1323", 1);
					// Устанавливаем максимальное количество подключений
					this->_os.sysctl("kern.ipc.somaxconn", 49152);
					// Активируем автоматическую отправку и получение
					this->_os.sysctl("net.inet.tcp.sendbuf_auto", 1);
					this->_os.sysctl("net.inet.tcp.recvbuf_auto", 1);
					// Увеличиваем размер шага автонастройки
					this->_os.sysctl("net.inet.tcp.sendbuf_inc", 8192);
					this->_os.sysctl("net.inet.tcp.recvbuf_inc", 16384);
					// Активируем нормальное нормальное TCP Reno
					this->_os.sysctl("net.inet.tcp.inflight.enable", 0);
					// Активируем на хостах тестирования/измерений
					this->_os.sysctl("net.inet.tcp.hostcache.expire", 1);
					/**
					 * Для хостов 10G было бы неплохо увеличить это значение,
					 * т.к. 4G, похоже, является пределом для некоторых установок FreeBSD
					 */
					this->_os.sysctl("kern.ipc.maxsockbuf", 16777216);
					// Увеличиваем максимальный размер буферов для отправки
					this->_os.sysctl("net.inet.tcp.sendspace", 1042560);
					// Увеличиваем максимальный размер буферов для чтения
					this->_os.sysctl("net.inet.tcp.recvspace", 1042560);
					// Увеличиваем максимальный размер буферов для отправки
					this->_os.sysctl("net.inet.tcp.sendbuf_max", 16777216);
					// Увеличиваем максимальный размер буферов для чтения
					this->_os.sysctl("net.inet.tcp.recvbuf_max", 16777216);
					/**
					 * Вы можете проверить, какие доступны алгоритмы получения доступных сообщений, используя net.inet.tcp.cc.available
					 */
					const string & algorithm = this->_os.sysctl <string> ("net.inet.tcp.cc.available");
					// Если выбран лучший доступны алгоритм
					if(!algorithm.empty()){
						// Если найден алгоритм cubic
						if(this->_fmk->exists("cubic", algorithm))
							// Активируем выбранный нами алгоритм
							this->_os.sysctl("net.inet.tcp.cc.algorithm", "cubic");
						// Если же найден алгоритм htcp
						else if(this->_fmk->exists("htcp", algorithm))
							// Активируем выбранный нами алгоритм
							this->_os.sysctl("net.inet.tcp.cc.algorithm", "htcp");
					}
				// Если пользователь не является суперпользователем
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, "Root privileges are required to apply network optimizations");
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::WARNING, "Root privileges are required to apply network optimizations");
					#endif
				}
			#endif
		#endif
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод инициализации базы событий
 *
 * @param mode флаг инициализации
 */
void awh::Base::init(const event_mode_t mode) noexcept {
	/**
	 * Определяем флаг инициализации
	 */
	switch(static_cast <uint8_t> (mode)){
		// Если необходимо активировать сетевые методы
		case static_cast <uint8_t> (event_mode_t::ENABLED): {
			/**
			 * Для операционной системы MS Windows
			 */
			#if _WIN32 || _WIN64
				// Если WinSocksAPI ещё не инициализирована
				if(!(this->_winSockInit = ::winsockInitialized())){
					// Идентификатор ошибки
					int32_t error = 0;
					// Выполняем инициализацию сетевого контекста
					if((error = ::WSAStartup(MAKEWORD(2, 2), &this->_wsaData)) != 0){
						// Создаём буфер сообщения ошибки
						wchar_t message[256] = {0};
						// Выполняем формирование текста ошибки
						::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 256, 0);
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug(L"%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, message);
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print(L"%s", log_t::flag_t::CRITICAL, message);
						#endif
						// Очищаем сетевой контекст
						::WSACleanup();
						// Выходим из приложения
						::exit(EXIT_FAILURE);
					}
					// Выполняем проверку версии WinSocket
					if((2 != LOBYTE(this->_wsaData.wVersion)) || (2 != HIBYTE(this->_wsaData.wVersion))){
						// Выводим сообщение об ошибке
						this->_log->print("Events base is not init", log_t::flag_t::CRITICAL);
						// Очищаем сетевой контекст
						::WSACleanup();
						// Выходим из приложения
						::exit(EXIT_FAILURE);
					}
				}
			/**
			 * Для операционной системы Sun Solaris
			 */
			#elif __sun__
				// Выполняем инициализацию /dev/poll
				if((this->_wfd = ::open("/dev/poll", O_RDWR, 0)) == INVALID_SOCKET){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
					#endif
					// Выходим принудительно из приложения
					::exit(EXIT_FAILURE);
				}
				// Выполняем открытие файлового дескриптора
				::fcntl(this->_wfd, F_SETFD, FD_CLOEXEC);
			/**
			 * Для операционной системы Linux
			 */
			#elif __linux__
				// Выполняем инициализацию EPoll
				if((this->_efd = ::epoll_create(AWH_MAX_COUNT_FDS)) == INVALID_SOCKET){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
					#endif
					// Выходим принудительно из приложения
					::exit(EXIT_FAILURE);
				}
				// Выполняем открытие файлового дескриптора
				::fcntl(this->_efd, F_SETFD, FD_CLOEXEC);
				// Если уведомитель пробуждения опроса инициализирован
				if(this->_wakeup != INVALID_SOCKET){
					// Создаём объект события пробуждения
					struct epoll_event event = {};
					// Устанавливаем флаги ожидания готовности на чтение
					event.events = (EPOLLIN | EPOLLET);
					// Помечаем событие указателем на уведомитель пробуждения
					event.data.ptr = &this->_wake;
					// Выполняем регистрацию уведомителя пробуждения в EPoll
					if(::epoll_ctl(this->_efd, EPOLL_CTL_ADD, this->_wakeup, &event) != 0)
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				}
			/**
			 * Для операционной системы FreeBSD, NetBSD, OpenBSD или MacOS X
			 */
			#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
				// Выполняем инициализацию Kqueue
				if((this->_kq = kqueue()) == INVALID_SOCKET){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
					#endif
					// Выходим принудительно из приложения
					::exit(EXIT_FAILURE);
				}
				// Выполняем открытие файлового дескриптора
				::fcntl(this->_kq, F_SETFD, FD_CLOEXEC);
				// Если уведомитель пробуждения опроса инициализирован
				if(this->_wakeup != INVALID_SOCKET){
					// Создаём объект события пробуждения
					struct kevent event;
					// Выполняем заполнение нулями всю структуру события
					::memset(&event, 0, sizeof(event));
					// Устанавливаем событие чтения уведомителя пробуждения
					EV_SET(&event, this->_wakeup, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, nullptr);
					/**
					 * Регистрируем отдельным вызовом, а не через постоянный список изменений:
					 * уведомитель не является участником базы событий
					 */
					if(::kevent(this->_kq, &event, 1, nullptr, 0, nullptr) == INVALID_SOCKET)
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				}
			#endif
		} break;
		// Если необходимо деактивировать сетевые методы
		case static_cast <uint8_t> (event_mode_t::DISABLED): {
			/**
			 * Для операционной системы MS Windows
			 */
			#if _WIN32 || _WIN64
				// Если WinSocksAPI была инициализированна в этой базе событий
				if(!this->_winSockInit)
					// Очищаем сетевой контекст
					::WSACleanup();
			/**
			 * Для операционной системы Sun Solaris
			 */
			#elif __sun__
				// Выполняем закрытие подключения
				::close(this->_wfd);
			/**
			 * Для операционной системы Linux
			 */
			#elif __linux__
				// Выполняем закрытие подключения
				::close(this->_efd);
			/**
			 * Для операционной системы FreeBSD, NetBSD, OpenBSD или MacOS X
			 */
			#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
				// Выполняем закрытие подключения
				::close(this->_kq);
			#endif
		} break;
	}
}
/**
 * @brief Метод пересоздания объекта ядра базы событий с повторной регистрацией всех участников
 *
 * Участники, их функции обратного вызова и уведомители таймеров сохраняются как есть.
 * Раньше пинок очищал базу событий и добавлял участников заново: на Linux очистка не
 * удаляла участников, add() находил их и не регистрировал в новом EPoll, а таймеры
 * уничтожались, поэтому все события умирали. На kqueue таймеры пересоздавались с новыми
 * дескрипторами, о которых объекты событий не знали. Метод может быть вызван из обработчика,
 * поэтому участники не удаляются (удаление уничтожило бы выполняющуюся функцию обратного вызова)
 */
void awh::Base::recreate() noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем блокировку чтения базы событий (страж снимет блокировку на любом пути выхода)
		const LockerGuard locker(this->_locker);
		/**
		 * Для операционной системы Linux
		 */
		#if __linux__
			/**
			 * Результаты последнего опроса принадлежат старому объекту ядра. Буфер не сокращаем
			 * (он может обходиться прямо сейчас), а помечаем все его записи пустыми
			 */
			for(auto & event : this->_events){
				// Сбрасываем флаги события
				event.events = 0;
				// Помечаем запись пустой
				event.data.ptr = nullptr;
			}
		/**
		 * Для операционной системы FreeBSD, NetBSD, OpenBSD или MacOS X
		 */
		#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
			/**
			 * Результаты последнего опроса принадлежат старому объекту ядра. Буфер не сокращаем
			 * (он может обходиться прямо сейчас), а помечаем все его записи недействительными
			 */
			for(auto & event : this->_events){
				// Сбрасываем флаги события
				event.flags = 0;
				// Помечаем фильтр события недействительным
				event.filter = 0;
				// Помечаем идентификатор события недействительным
				event.ident = static_cast <uintptr_t> (INVALID_SOCKET);
			}
		#endif
		/**
		 * Для всех операционных систем кроме MS Windows. У WSAPoll нет объекта ядра, а
		 * деинициализация WinSocksAPI сделала бы недействительными все сокеты
		 */
		#if !_WIN32 && !_WIN64
			// Выполняем деинициализацию базы событий
			this->init(event_mode_t::DISABLED);
			// Выполняем инициализацию базы событий
			this->init(event_mode_t::ENABLED);
		#endif
		/**
		 * Для операционной системы Sun Solaris
		 */
		#if __sun__
			// Если список отслеживаемых файловых дескрипторов не пустой
			if(!this->_events.empty()){
				// Выполняем перебор всего списка отслеживаемых файловых дескрипторов
				for(auto & event : this->_events)
					// Очищаем полученное событие
					event.revents = 0;
				// Выполняем добавление списка файловых дескрипторов для отслеживания
				if(::write(this->_wfd, this->_events.data(), sizeof(struct pollfd) * this->_events.size()) <= 0){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
					#endif
				}
			}
		/**
		 * Для операционной системы Linux
		 */
		#elif __linux__
			// Выполняем перебор всего списка изменений (в нём текущие флаги событий каждого участника)
			for(auto & item : this->_change){
				// Если участник события установлен
				if(item.data.ptr != nullptr){
					// Выполняем регистрацию участника в новом EPoll
					if(::epoll_ctl(this->_efd, EPOLL_CTL_ADD, reinterpret_cast <peer_t *> (item.data.ptr)->sock, &item) != 0){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
						#endif
					}
				}
			}
		/**
		 * Для операционной системы FreeBSD, NetBSD, OpenBSD или MacOS X
		 */
		#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
			/**
			 * Список изменений постоянный (все записи с EV_ADD) и подаётся в каждый вызов kevent,
			 * поэтому все участники регистрируются в новой очереди при следующем опросе
			 */
		#endif
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод получения событий верхнеуровневых потоков
 *
 * @param sock  сокет межпотокового передатчика
 * @param event входящее событие от межпотокового передатчика
 */
void awh::Base::stream(const SOCKET sock, const uint64_t event) noexcept {
	// Выполняем поиск указанного межпотокового передатчика
	auto i = this->_upstream.find(sock);
	// Если межпотоковый передатчик обнаружен
	if(i != this->_upstream.end()){
		// Если функция обратного вызова установлена
		if(i->second->callback != nullptr)
			// Выполняем функцию обратного вызова
			std::apply(i->second->callback, std::make_tuple(event));
	}
}
/**
 * @brief Метод извлечения всех накопившихся событий межпотокового передатчика
 *
 * Уведомитель отслеживается по фронту (EPOLLET / EV_CLEAR): одно пробуждение может
 * нести несколько уведомлений, и оставшиеся в очереди ждали бы следующего уведомления.
 * Поэтому события извлекаются до исчерпания, каждое доставляется ровно один раз.
 * Передатчик и участник ищутся заново на каждом шаге: обработчик может их удалить
 *
 * @param id   идентификатор записи
 * @param sock сокет межпотокового передатчика
 */
void awh::Base::streams(const uint64_t id, const SOCKET sock) noexcept {
	// Идентификатор межпотокового события
	uint64_t event = 0;
	// Флаг извлечения события
	bool extracted = false;
	/**
	 * Выполняем извлечение событий до исчерпания
	 */
	do {
		// Выполняем поиск верхнеуровневого потока
		auto i = this->_upstream.find(sock);
		// Если верхнеуровневый поток не найден
		if(i == this->_upstream.end())
			// Выходим из цикла
			break;
		// Выполняем блокировку потока
		i->second->mtx.lock();
		// Выполняем извлечение входящего события
		extracted = i->second->notifier.event(event);
		// Выполняем разблокировку потока
		i->second->mtx.unlock();
		// Если событие извлечено
		if(extracted){
			// Выполняем поиск участника в базе событий
			auto j = this->_peers.find(sock);
			// Если участник удалён или заменён
			if((j == this->_peers.end()) || (j->second.id != id))
				// Выходим из цикла
				break;
			// Выполняем поиск события межпотоковое присутствует в базе событий
			auto k = j->second.mode.find(event_type_t::STREAM);
			// Если событие найдено и оно активированно
			if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
				// Выполняем функцию обратного вызова
				this->stream(sock, event);
		}
	} while(extracted);
}
/**
 * Для операционной системы Linux
 */
#if __linux__
	/**
	 * @brief Метод исключения участника из результатов последнего опроса базы событий
	 *
	 * Буфер результатов опроса нельзя сокращать во время его обхода: записи сдвигаются,
	 * часть событий пропускается, а в обход попадают устаревшие записи прошлого опроса.
	 * Поэтому запись удаляемого участника только помечается пустой (data.ptr = nullptr),
	 * и обход её пропускает. Без этого указатель на удалённого участника разыменовывается.
	 *
	 * @param peer участник для исключения
	 */
	void awh::Base::forget(const peer_t * peer) noexcept {
		// Количество действительных записей в буфере результатов
		const size_t count = std::min(this->_ready, this->_events.size());
		// Выполняем перебор всех действительных записей
		for(size_t i = 0; i < count; i++){
			// Если запись принадлежит участнику
			if(this->_events[i].data.ptr == peer){
				// Сбрасываем флаги события
				this->_events[i].events = 0;
				// Помечаем запись пустой
				this->_events[i].data.ptr = nullptr;
			}
		}
	}
/**
 * Для операционной системы MacOS X, FreeBSD, NetBSD или OpenBSD
 */
#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
	/**
	 * @brief Метод исключения сокета из результатов последнего опроса базы событий
	 *
	 * Буфер результатов опроса нельзя сокращать во время его обхода: записи сдвигаются,
	 * часть событий пропускается, а в обход попадают устаревшие записи прошлого опроса
	 * (например чужой EV_EOF закрывает здоровое подключение с тем же номером сокета).
	 * Поэтому запись только помечается недействительной, и обход её пропускает.
	 *
	 * @param sock сокет для исключения
	 */
	void awh::Base::forget(const SOCKET sock) noexcept {
		// Количество действительных записей в буфере результатов
		const size_t count = std::min(this->_ready, this->_events.size());
		// Выполняем перебор всех действительных записей
		for(size_t i = 0; i < count; i++){
			// Если запись принадлежит сокету
			if(this->_events[i].ident == static_cast <uintptr_t> (sock)){
				// Сбрасываем флаги события
				this->_events[i].flags = 0;
				// Помечаем фильтр события недействительным
				this->_events[i].filter = 0;
				// Помечаем идентификатор события недействительным
				this->_events[i].ident = static_cast <uintptr_t> (INVALID_SOCKET);
			}
		}
	}
	/**
	 * @brief Метод удаления всех изменений событий сокета
	 *
	 * @param sock сокет изменения которого удаляются
	 */
	void awh::Base::unslot(const SOCKET sock) noexcept {
		// Выполняем перебор всего списка изменений
		for(auto i = this->_change.begin(); i != this->_change.end();){
			// Если изменение принадлежит сокету
			if(i->ident == static_cast <uintptr_t> (sock))
				// Выполняем удаление изменения
				i = this->_change.erase(i);
			// Продолжаем перебор дальше
			else ++i;
		}
	}
	/**
	 * @brief Метод поиска изменения события сокета для указанного фильтра
	 *
	 * Фильтры kqueue являются малыми отрицательными числами, а не битами, поэтому
	 * у сокета отдельное изменение для чтения и отдельное для записи. Одно общее
	 * изменение затирало ожидающее включение записи при включении чтения.
	 *
	 * @param sock   сокет для поиска
	 * @param filter фильтр события (EVFILT_READ / EVFILT_WRITE)
	 * @return       найденное изменение события или nullptr
	 */
	struct kevent * awh::Base::slot(const SOCKET sock, const int16_t filter) noexcept {
		// Выполняем перебор всего списка изменений
		for(auto & item : this->_change){
			// Если изменение принадлежит сокету и фильтру
			if((item.ident == static_cast <uintptr_t> (sock)) && (item.filter == filter))
				// Выводим найденное изменение
				return &item;
		}
		// Сообщаем, что изменение не найдено
		return nullptr;
	}
#endif
/**
 * @brief Метод удаления файлового дескриптора из базы событий
 *
 * @param sock сокет для удаления
 * @return     результат работы функции
 */
bool awh::Base::del(const SOCKET sock) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Выполняем блокировку чтения базы событий
			this->_locker = true;
			// Выполняем поиск файлового дескриптора из списка событий
			for(auto i = this->_events.begin(); i != this->_events.end(); ++i){
				// Если сокет найден
				if(i->fd == sock){
					// Очищаем полученное событие
					i->revents = 0;
					// Выполняем сброс файлового дескриптора
					i->fd = INVALID_SOCKET;
					// Выполняем удаление события из списка отслеживания
					this->_events.erase(i);
					// Выполняем поиск файлового дескриптора в базе событий
					auto j = this->_peers.find(sock);
					// Если сокет есть в базе событий
					if(j != this->_peers.end()){
						// Если событие принадлежит к таймеру
						if(j->second.type == event_type_t::TIMER)
							// Выполняем удаление таймера
							this->_watch.away(j->second.sock);
					}
					// Выходим из цикла
					break;
				}
			}
			// Выполняем разблокировку чтения базы событий
			this->_locker = false;
		/**
		 * Для операционной системы Sun Solaris
		 */
		#elif __sun__
			// Выполняем блокировку чтения базы событий
			this->_locker = true;
			// Выполняем поиск файлового дескриптора из списка событий
			for(auto i = this->_events.begin(); i != this->_events.end(); ++i){
				// Если сокет найден
				if(i->fd == sock){
					// Очищаем полученное событие
					i->revents = 0;
					// Выполняем сброс файлового дескриптора
					i->fd = INVALID_SOCKET;
					// Выполняем удаление события из списка отслеживания
					this->_events.erase(i);
					// Если в списке ещё есть что отслеживать
					if(!this->_events.empty()){
						// Выполняем добавление списка файловых дескрипторов для отслеживания
						if(::write(this->_wfd, this->_events.data(), sizeof(struct pollfd) * this->_events.size()) <= 0){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock), log_t::flag_t::CRITICAL, ::strerror(errno));
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
							#endif
						}
					// Если в списке больше нет файловых дескрипторов
					} else {
						// Выполняем деинициализацию базы событий
						this->init(event_mode_t::DISABLED);
						// Выполняем инициализацию базы событий
						this->init(event_mode_t::ENABLED);
					}
					// Выполняем поиск файлового дескриптора в базе событий
					auto j = this->_peers.find(sock);
					// Если сокет есть в базе событий
					if(j != this->_peers.end()){
						// Если событие принадлежит к таймеру
						if(j->second.type == event_type_t::TIMER)
							// Выполняем удаление таймера
							this->_watch.away(j->second.sock);
					}
					// Выходим из цикла
					break;
				}
			}
			// Выполняем разблокировку чтения базы событий
			this->_locker = false;
		/**
		 * Для операционной системы Linux
		 */
		#elif __linux__
			// Выполняем блокировку чтения базы событий
			this->_locker = true;
			/**
			 * Буфер результатов опроса (_events) здесь не трогаем: он может обходиться
			 * прямо сейчас, а регистрацию в ядре снимаем по списку изменений
			 */
			// Выполняем поиск файлового дескриптора из списка изменений
			for(auto i = this->_change.begin(); i != this->_change.end(); ++i){
				// Если сокет найден
				if((i->data.ptr != nullptr) && (reinterpret_cast <peer_t *> (i->data.ptr)->sock == sock)){
					// Выполняем изменение параметров события
					result = (::epoll_ctl(this->_efd, EPOLL_CTL_DEL, sock, &(* i)) == 0);
					// Если событие принадлежит к таймеру
					if(reinterpret_cast <peer_t *> (i->data.ptr)->type == event_type_t::TIMER)
						// Выполняем удаление таймера
						this->_watch.away(reinterpret_cast <peer_t *> (i->data.ptr)->sock);
					// Выполняем удаление события из списка изменений
					this->_change.erase(i);
					// Выходим из цикла
					break;
				}
			}
			// Если удаление не выполненно
			if(!result)
				// Выполняем изменение параметров события
				result = (::epoll_ctl(this->_efd, EPOLL_CTL_DEL, sock, nullptr) == 0);
			// Выполняем разблокировку чтения базы событий
			this->_locker = false;
		/**
		 * Для операционной системы FreeBSD, NetBSD, OpenBSD или MacOS X
		 */
		#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
			// Выполняем блокировку чтения базы событий
			this->_locker = true;
			// Исключаем сокет из результатов текущего опроса (буфер не сокращаем, он может обходиться)
			this->forget(sock);
			// Выполняем удаление всех изменений событий сокета (чтения и записи)
			this->unslot(sock);
			// Выполняем поиск файлового дескриптора в базе событий
			auto i = this->_peers.find(sock);
			// Если сокет есть в базе событий
			if(i != this->_peers.end()){
				// Если событие принадлежит к таймеру
				if(i->second.type == event_type_t::TIMER)
					// Выполняем удаление таймера
					this->_watch.away(i->second.sock);
			}
			// Выполняем разблокировку чтения базы событий
			this->_locker = false;
		#endif
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод удаления файлового дескриптора из базы событий для всех событий
 *
 * @param id идентификатор записи
 * @param sock сокет для удаления
 * @return   результат работы функции
 */
bool awh::Base::del(const uint64_t id, const SOCKET sock) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Выполняем поиск файлового дескриптора в базе событий
			auto i = this->_peers.find(sock);
			// Если сокет есть в базе событий
			if((result = (i != this->_peers.end()) && (i->second.id == id))){
				// Выполняем блокировку чтения базы событий
				this->_locker = true;
				// Выполняем поиск файлового дескриптора из списка событий
				for(auto j = this->_events.begin(); j != this->_events.end(); ++j){
					// Если сокет найден
					if(j->fd == sock){
						// Очищаем полученное событие
						j->revents = 0;
						// Выполняем сброс файлового дескриптора
						j->fd = INVALID_SOCKET;
						// Выполняем удаление события из списка отслеживания
						this->_events.erase(j);
						// Если событие принадлежит к таймеру
						if(i->second.type == event_type_t::TIMER)
							// Выполняем удаление таймера
							this->_watch.away(i->second.sock);
						// Выходим из цикла
						break;
					}
				}
				// Выполняем удаление всего события
				this->_peers.erase(i);
				// Выполняем разблокировку чтения базы событий
				this->_locker = false;
			}
		/**
		 * Для операционной системы Sun Solaris
		 */
		#elif __sun__
			// Выполняем поиск файлового дескриптора в базе событий
			auto i = this->_peers.find(sock);
			// Если сокет есть в базе событий
			if((result = (i != this->_peers.end()) && (i->second.id == id))){
				// Выполняем блокировку чтения базы событий
				this->_locker = true;
				// Выполняем поиск файлового дескриптора из списка событий
				for(auto j = this->_events.begin(); j != this->_events.end(); ++j){
					// Если сокет найден
					if(j->fd == sock){
						// Очищаем полученное событие
						j->revents = 0;
						// Выполняем сброс файлового дескриптора
						j->fd = INVALID_SOCKET;
						// Выполняем удаление события из списка отслеживания
						this->_events.erase(j);
						// Если в списке ещё есть что отслеживать
						if(!this->_events.empty()){
							// Выполняем добавление списка файловых дескрипторов для отслеживания
							if(::write(this->_wfd, this->_events.data(), sizeof(struct pollfd) * this->_events.size()) <= 0){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock), log_t::flag_t::CRITICAL, ::strerror(errno));
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об ошибке
									this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
								#endif
							}
						// Если в списке больше нет файловых дескрипторов
						} else {
							// Выполняем деинициализацию базы событий
							this->init(event_mode_t::DISABLED);
							// Выполняем инициализацию базы событий
							this->init(event_mode_t::ENABLED);
						}
						// Если событие принадлежит к таймеру
						if(i->second.type == event_type_t::TIMER)
							// Выполняем удаление таймера
							this->_watch.away(i->second.sock);
						// Выходим из цикла
						break;
					}
				}
				// Выполняем удаление всего события
				this->_peers.erase(i);
				// Выполняем разблокировку чтения базы событий
				this->_locker = false;
			}
		/**
		 * Для операционной системы Linux
		 */
		#elif __linux__
			// Выполняем поиск файлового дескриптора в базе событий
			auto i = this->_peers.find(sock);
			// Если сокет есть в базе событий
			if((result = (i != this->_peers.end()) && (i->second.id == id))){
				// Выполняем блокировку чтения базы событий
				this->_locker = true;
				// Исключаем участника из результатов текущего опроса (буфер не сокращаем, он может обходиться)
				this->forget(&i->second);
				// Выполняем поиск файлового дескриптора из списка изменений
				for(auto j = this->_change.begin(); j != this->_change.end(); ++j){
					// Если сокет найден
					if((reinterpret_cast <peer_t *> (j->data.ptr) == &i->second) &&
					   (reinterpret_cast <peer_t *> (j->data.ptr)->id == id)){
						// Выполняем изменение параметров события
						result = (::epoll_ctl(this->_efd, EPOLL_CTL_DEL, i->second.sock, &(* j)) == 0);
						// Выполняем удаление события из списка изменений
						this->_change.erase(j);
						// Выходим из цикла
						break;
					}
				}
				// Если событие принадлежит к таймеру
				if(i->second.type == event_type_t::TIMER)
					// Выполняем удаление таймера
					this->_watch.away(i->second.sock);
				// Выполняем удаление всего события
				this->_peers.erase(i);
				// Выполняем разблокировку чтения базы событий
				this->_locker = false;
			}
		/**
		 * Для операционной системы FreeBSD, NetBSD, OpenBSD или MacOS X
		 */
		#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
			// Выполняем поиск файлового дескриптора в базе событий
			auto i = this->_peers.find(sock);
			// Если сокет есть в базе событий
			if((result = (i != this->_peers.end()) && (i->second.id == id))){
				// Выполняем блокировку чтения базы событий
				this->_locker = true;
				// Исключаем сокет из результатов текущего опроса (буфер не сокращаем, он может обходиться)
				this->forget(sock);
				/**
				 * Выполняем удаление всех изменений событий сокета (чтения и записи).
				 * Регистрация в ядре снимается закрытием сокета, как и раньше
				 */
				this->unslot(sock);
				// Если событие принадлежит к таймеру
				if(i->second.type == event_type_t::TIMER)
					// Выполняем удаление таймера
					this->_watch.away(i->second.sock);
				// Выполняем удаление всего события
				this->_peers.erase(i);
				// Выполняем разблокировку чтения базы событий
				this->_locker = false;
			}
		#endif
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод удаления файлового дескриптора из базы событий для указанного события
 *
 * @param id   идентификатор записи
 * @param sock   сокет для удаления
 * @param type тип отслеживаемого события
 * @return     результат работы функции
 */
bool awh::Base::del(const uint64_t id, const SOCKET sock, const event_type_t type) noexcept {
	// Результат работы функции
	bool result = false;
	// Если сокет передан верный
	if((sock != INVALID_SOCKET) || (type == event_type_t::TIMER)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * Для операционной системы MS Windows
			 */
			#if _WIN32 || _WIN64
				// Выполняем поиск файлового дескриптора в базе событий
				auto i = this->_peers.find(sock);
				// Если сокет есть в базе событий
				if((result = (i != this->_peers.end()) && (i->second.id == id))){
					// Выполняем блокировку чтения базы событий
					this->_locker = true;
					/**
					 * Определяем тип переданного события
					 */
					switch(static_cast <uint8_t> (type)){
						// Если событие установлено как отслеживание закрытия подключения
						case static_cast <uint8_t> (event_type_t::CLOSE): {
							// Выполняем поиск типа события и его режим работы
							auto j = i->second.mode.find(type);
							// Если режим работы события получен
							if((result = (j != i->second.mode.end()))){
								// Выполняем отключение работы события
								j->second = event_mode_t::DISABLED;
								// Выполняем удаление типа события
								i->second.mode.erase(j);
							}
						} break;
						// Если событие установлено как таймер
						case static_cast <uint8_t> (event_type_t::TIMER): {
							// Флаг удалённого события из базы событий
							bool erased = false;
							// Выполняем поиск типа события и его режим работы
							auto j = i->second.mode.find(type);
							// Если режим работы события получен
							if((result = (j != i->second.mode.end()))){
								// Выполняем отключение работы события
								j->second = event_mode_t::DISABLED;
								// Выполняем поиск файлового дескриптора из списка событий
								for(auto k = this->_events.begin(); k != this->_events.end(); ++k){
									// Если сокет найден
									if((erased = (k->fd == sock))){
										// Очищаем полученное событие
										k->revents = 0;
										// Удаляем флаг ожидания готовности файлового дескриптора на чтение
										k->events ^= POLLIN;
										// Выполняем удаление события из списка отслеживания
										this->_events.erase(k);
										// Выполняем удаление типа события
										i->second.mode.erase(j);
										// Выполняем удаление таймера
										this->_watch.away(i->second.sock);
										// Выходим из цикла
										break;
									}
								}
								// Если удаление события небыло произведено
								if(!erased)
									// Выполняем удаление типа события
									i->second.mode.erase(j);
							}
						} break;
						// Если событие принадлежит к потоку
						case static_cast <uint8_t> (event_type_t::STREAM): {
							// Флаг удалённого события из базы событий
							bool erased = false;
							// Выполняем поиск типа события и его режим работы
							auto j = i->second.mode.find(type);
							// Если режим работы события получен
							if((result = (j != i->second.mode.end()))){
								// Выполняем отключение работы события
								j->second = event_mode_t::DISABLED;
								// Выполняем поиск файлового дескриптора из списка событий
								for(auto k = this->_events.begin(); k != this->_events.end(); ++k){
									// Если сокет найден
									if((erased = (k->fd == sock))){
										// Очищаем полученное событие
										k->revents = 0;
										// Удаляем флаг ожидания готовности файлового дескриптора на чтение
										k->events ^= POLLIN;
										// Выполняем удаление типа события
										i->second.mode.erase(j);
										// Выполняем удаление события из списка отслеживания
										this->_events.erase(k);
										// Выходим из цикла
										break;
									}
								}
								// Если удаление события небыло произведено
								if(!erased)
									// Выполняем удаление типа события
									i->second.mode.erase(j);
							}
						} break;
						// Если событие установлено как отслеживание события чтения из сокета
						case static_cast <uint8_t> (event_type_t::READ): {
							// Флаг удалённого события из базы событий
							bool erased = false;
							// Выполняем поиск типа события и его режим работы
							auto j = i->second.mode.find(type);
							// Если режим работы события получен
							if((result = (j != i->second.mode.end()))){
								// Выполняем отключение работы события
								j->second = event_mode_t::DISABLED;
								// Выполняем поиск файлового дескриптора из списка событий
								for(auto k = this->_events.begin(); k != this->_events.end(); ++k){
									// Если сокет найден
									if((erased = (k->fd == sock))){
										// Очищаем полученное событие
										k->revents = 0;
										// Удаляем флаг ожидания готовности файлового дескриптора на чтение
										k->events ^= POLLIN;
										// Выполняем удаление типа события
										i->second.mode.erase(j);
										// Если список режимов событий пустой
										if(i->second.mode.empty() || (i->second.mode.find(event_type_t::WRITE) == i->second.mode.end()))
											// Выполняем удаление события из списка отслеживания
											this->_events.erase(k);
										// Выходим из цикла
										break;
									}
								}
								// Если удаление события небыло произведено
								if(!erased)
									// Выполняем удаление типа события
									i->second.mode.erase(j);
							}
						} break;
						// Если событие установлено как отслеживание события записи в сокет
						case static_cast <uint8_t> (event_type_t::WRITE): {
							// Флаг удалённого события из базы событий
							bool erased = false;
							// Выполняем поиск типа события и его режим работы
							auto j = i->second.mode.find(type);
							// Если режим работы события получен
							if((result = (j != i->second.mode.end()))){
								// Выполняем отключение работы события
								j->second = event_mode_t::DISABLED;
								// Выполняем поиск файлового дескриптора из списка событий
								for(auto k = this->_events.begin(); k != this->_events.end(); ++k){
									// Если сокет найден
									if((erased = (k->fd == sock))){
										// Очищаем полученное событие
										k->revents = 0;
										// Удаляем флаг ожидания готовности файлового дескриптора на запись
										k->events ^= POLLOUT;
										// Выполняем удаление типа события
										i->second.mode.erase(j);
										// Если список режимов событий пустой
										if(i->second.mode.empty() || (i->second.mode.find(event_type_t::READ) == i->second.mode.end()))
											// Выполняем удаление события из списка отслеживания
											this->_events.erase(k);
										// Выходим из цикла
										break;
									}
								}
								// Если удаление события небыло произведено
								if(!erased)
									// Выполняем удаление типа события
									i->second.mode.erase(j);
							}
						}
					}
					// Если список режимов событий пустой
					if(i->second.mode.empty() || ((i->second.mode.size() == 1) && (i->second.mode.find(event_type_t::CLOSE) != i->second.mode.end())))
						// Выполняем удаление всего события
						this->_peers.erase(i);
					// Выполняем разблокировку чтения базы событий
					this->_locker = false;
				}
			/**
			 * Для операционной системы Sun Solaris
			 */
			#elif __sun__
				// Выполняем поиск файлового дескриптора в базе событий
				auto i = this->_peers.find(sock);
				// Если сокет есть в базе событий
				if((result = (i != this->_peers.end()) && (i->second.id == id))){
					// Выполняем блокировку чтения базы событий
					this->_locker = true;
					/**
					 * Определяем тип переданного события
					 */
					switch(static_cast <uint8_t> (type)){
						// Если событие установлено как отслеживание закрытия подключения
						case static_cast <uint8_t> (event_type_t::CLOSE): {
							// Выполняем поиск типа события и его режим работы
							auto j = i->second.mode.find(type);
							// Если режим работы события получен
							if((result = (j != i->second.mode.end()))){
								// Выполняем отключение работы события
								j->second = event_mode_t::DISABLED;
								// Выполняем удаление типа события
								i->second.mode.erase(j);
							}
						} break;
						// Если событие установлено как таймер
						case static_cast <uint8_t> (event_type_t::TIMER): {
							// Флаг удалённого события из базы событий
							bool erased = false;
							// Выполняем поиск типа события и его режим работы
							auto j = i->second.mode.find(type);
							// Если режим работы события получен
							if((result = (j != i->second.mode.end()))){
								// Выполняем отключение работы события
								j->second = event_mode_t::DISABLED;
								// Выполняем поиск файлового дескриптора из списка событий
								for(auto k = this->_events.begin(); k != this->_events.end(); ++k){
									// Если сокет найден
									if((erased = (k->fd == sock))){
										// Очищаем полученное событие
										k->revents = 0;
										// Удаляем флаг ожидания готовности файлового дескриптора на чтение
										k->events ^= POLLIN;
										// Выполняем удаление типа события
										i->second.mode.erase(j);
										// Выполняем удаление события из списка отслеживания
										this->_events.erase(k);
										// Если в списке ещё есть что отслеживать
										if(!this->_events.empty()){
											// Выполняем добавление списка файловых дескрипторов для отслеживания
											if(::write(this->_wfd, this->_events.data(), sizeof(struct pollfd) * this->_events.size()) <= 0){
												/**
												 * Если включён режим отладки
												 */
												#if DEBUG_MODE
													// Выводим сообщение об ошибке
													this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type)), log_t::flag_t::CRITICAL, ::strerror(errno));
												/**
												 * Если режим отладки не включён
												 */
												#else
													// Выводим сообщение об ошибке
													this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
												#endif
											}
										// Если в списке больше нет файловых дескрипторов
										} else {
											// Выполняем деинициализацию базы событий
											this->init(event_mode_t::DISABLED);
											// Выполняем инициализацию базы событий
											this->init(event_mode_t::ENABLED);
										}
										// Выполняем удаление таймера
										this->_watch.away(i->second.sock);
										// Выходим из цикла
										break;
									}
								}
								// Если удаление события небыло произведено
								if(!erased)
									// Выполняем удаление типа события
									i->second.mode.erase(j);
							}
						} break;
						// Если событие принадлежит к потоку
						case static_cast <uint8_t> (event_type_t::STREAM): {
							// Флаг удалённого события из базы событий
							bool erased = false;
							// Выполняем поиск типа события и его режим работы
							auto j = i->second.mode.find(type);
							// Если режим работы события получен
							if((result = (j != i->second.mode.end()))){
								// Выполняем отключение работы события
								j->second = event_mode_t::DISABLED;
								// Выполняем поиск файлового дескриптора из списка событий
								for(auto k = this->_events.begin(); k != this->_events.end(); ++k){
									// Если сокет найден
									if((erased = (k->fd == sock))){
										// Очищаем полученное событие
										k->revents = 0;
										// Удаляем флаг ожидания готовности файлового дескриптора на чтение
										k->events ^= POLLIN;
										// Выполняем удаление типа события
										i->second.mode.erase(j);
										// Выполняем удаление события из списка отслеживания
										this->_events.erase(k);
										// Если в списке ещё есть что отслеживать
										if(!this->_events.empty()){
											// Выполняем добавление списка файловых дескрипторов для отслеживания
											if(::write(this->_wfd, this->_events.data(), sizeof(struct pollfd) * this->_events.size()) <= 0){
												/**
												 * Если включён режим отладки
												 */
												#if DEBUG_MODE
													// Выводим сообщение об ошибке
													this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type)), log_t::flag_t::CRITICAL, ::strerror(errno));
												/**
												 * Если режим отладки не включён
												 */
												#else
													// Выводим сообщение об ошибке
													this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
												#endif
											}
										// Если в списке больше нет файловых дескрипторов
										} else {
											// Выполняем деинициализацию базы событий
											this->init(event_mode_t::DISABLED);
											// Выполняем инициализацию базы событий
											this->init(event_mode_t::ENABLED);
										}
										// Выходим из цикла
										break;
									}
								}
								// Если удаление события небыло произведено
								if(!erased)
									// Выполняем удаление типа события
									i->second.mode.erase(j);
							}
						} break;
						// Если событие установлено как отслеживание события чтения из сокета
						case static_cast <uint8_t> (event_type_t::READ): {
							// Флаг удалённого события из базы событий
							bool erased = false;
							// Выполняем поиск типа события и его режим работы
							auto j = i->second.mode.find(type);
							// Если режим работы события получен
							if((result = (j != i->second.mode.end()))){
								// Выполняем отключение работы события
								j->second = event_mode_t::DISABLED;
								// Выполняем поиск файлового дескриптора из списка событий
								for(auto k = this->_events.begin(); k != this->_events.end(); ++k){
									// Если сокет найден
									if((erased = (k->fd == sock))){
										// Очищаем полученное событие
										k->revents = 0;
										// Удаляем флаг ожидания готовности файлового дескриптора на чтение
										k->events ^= POLLIN;
										// Выполняем удаление типа события
										i->second.mode.erase(j);
										// Если список режимов событий пустой
										if(i->second.mode.empty() || (i->second.mode.find(event_type_t::WRITE) == i->second.mode.end())){
											// Выполняем удаление события из списка отслеживания
											this->_events.erase(k);
											// Если в списке ещё есть что отслеживать
											if(!this->_events.empty()){
												// Выполняем добавление списка файловых дескрипторов для отслеживания
												if(::write(this->_wfd, this->_events.data(), sizeof(struct pollfd) * this->_events.size()) <= 0){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type)), log_t::flag_t::CRITICAL, ::strerror(errno));
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
													#endif
												}
											// Если в списке больше нет файловых дескрипторов
											} else {
												// Выполняем деинициализацию базы событий
												this->init(event_mode_t::DISABLED);
												// Выполняем инициализацию базы событий
												this->init(event_mode_t::ENABLED);
											}
										}
										// Выходим из цикла
										break;
									}
								}
								// Если удаление события небыло произведено
								if(!erased)
									// Выполняем удаление типа события
									i->second.mode.erase(j);
							}
						} break;
						// Если событие установлено как отслеживание события записи в сокет
						case static_cast <uint8_t> (event_type_t::WRITE): {
							// Флаг удалённого события из базы событий
							bool erased = false;
							// Выполняем поиск типа события и его режим работы
							auto j = i->second.mode.find(type);
							// Если режим работы события получен
							if((result = (j != i->second.mode.end()))){
								// Выполняем отключение работы события
								j->second = event_mode_t::DISABLED;
								// Выполняем поиск файлового дескриптора из списка событий
								for(auto k = this->_events.begin(); k != this->_events.end(); ++k){
									// Если сокет найден
									if((erased = (k->fd == sock))){
										// Очищаем полученное событие
										k->revents = 0;
										// Удаляем флаг ожидания готовности файлового дескриптора на запись
										k->events ^= POLLOUT;
										// Выполняем удаление типа события
										i->second.mode.erase(j);
										// Если список режимов событий пустой
										if(i->second.mode.empty() || (i->second.mode.find(event_type_t::READ) == i->second.mode.end())){
											// Выполняем удаление события из списка отслеживания
											this->_events.erase(k);
											// Если в списке ещё есть что отслеживать
											if(!this->_events.empty()){
												// Выполняем добавление списка файловых дескрипторов для отслеживания
												if(::write(this->_wfd, this->_events.data(), sizeof(struct pollfd) * this->_events.size()) <= 0){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type)), log_t::flag_t::CRITICAL, ::strerror(errno));
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
													#endif
												}
											// Если в списке больше нет файловых дескрипторов
											} else {
												// Выполняем деинициализацию базы событий
												this->init(event_mode_t::DISABLED);
												// Выполняем инициализацию базы событий
												this->init(event_mode_t::ENABLED);
											}
										}
										// Выходим из цикла
										break;
									}
								}
								// Если удаление события небыло произведено
								if(!erased)
									// Выполняем удаление типа события
									i->second.mode.erase(j);
							}
						}
					}
					// Если список режимов событий пустой
					if(i->second.mode.empty() || ((i->second.mode.size() == 1) && (i->second.mode.find(event_type_t::CLOSE) != i->second.mode.end())))
						// Выполняем удаление всего события
						this->_peers.erase(i);
					// Выполняем разблокировку чтения базы событий
					this->_locker = false;
				}
			/**
			 * Для операционной системы Linux
			 */
			#elif __linux__
				// Выполняем поиск файлового дескриптора в базе событий
				auto i = this->_peers.find(sock);
				// Если сокет есть в базе событий
				if((result = (i != this->_peers.end()) && (i->second.id == id))){
					// Выполняем блокировку чтения базы событий
					this->_locker = true;
					// Выполняем поиск типа события и его режим работы
					auto j = i->second.mode.find(type);
					// Если режим работы события получен
					if((result = (j != i->second.mode.end()))){
						// Флаг удалённого события из базы событий
						bool erased = false;
						// Выполняем отключение работы события
						j->second = event_mode_t::DISABLED;
						// Выполняем поиск файлового дескриптора из списка событий
						for(auto k = this->_change.begin(); k != this->_change.end(); ++k){
							// Если сокет найден
							if((erased = (reinterpret_cast <peer_t *> (k->data.ptr) == &i->second))){
								/**
								 * Определяем тип переданного события
								 */
								switch(static_cast <uint8_t> (type)){
									// Если событие установлено как таймер
									case static_cast <uint8_t> (event_type_t::TIMER):
									// Если событие принадлежит к потоку
									case static_cast <uint8_t> (event_type_t::STREAM):
										// Выполняем удаление флагов отслеживания таймера
										k->events ^= (EPOLLIN | EPOLLET);
									break;
									// Если событие установлено как отслеживание закрытия подключения
									case static_cast <uint8_t> (event_type_t::CLOSE):
										// Выполняем удаление флагов отслеживания закрытия подключения
										k->events ^= (EPOLLRDHUP | EPOLLHUP);
									break;
									// Если событие установлено как отслеживание события чтения из сокета
									case static_cast <uint8_t> (event_type_t::READ):
										// Удаляем флаг ожидания готовности файлового дескриптора на чтение
										k->events ^= EPOLLIN;
									break;
									// Если событие установлено как отслеживание события записи в сокет
									case static_cast <uint8_t> (event_type_t::WRITE):
										// Выполняем удаление флагов отслеживания записи данных в сокет
										k->events ^= EPOLLOUT;
									break;
								}
								// Выполняем удаление типа события
								i->second.mode.erase(j);
								// Если список режимов событий пустой
								if(i->second.mode.empty()){
									// Выполняем изменение параметров события
									result = (::epoll_ctl(this->_efd, EPOLL_CTL_DEL, i->second.sock, &(* k)) == 0);
									// Выполняем удаление события из списка изменений
									this->_change.erase(k);
								// Выполняем изменение параметров события
								} else result = (::epoll_ctl(this->_efd, EPOLL_CTL_MOD, i->second.sock, &(* k)) == 0);
								// Если событие принадлежит к таймеру
								if(i->second.type == event_type_t::TIMER)
									// Выполняем удаление таймера
									this->_watch.away(i->second.sock);
								// Выходим из цикла
								break;
							}
						}
						// Если удаление события небыло произведено
						if(!erased)
							// Выполняем удаление типа события
							i->second.mode.erase(j);
					}
					// Если список режимов событий пустой
					if(i->second.mode.empty()){
						// Исключаем участника из результатов текущего опроса (буфер не сокращаем, он может обходиться)
						this->forget(&i->second);
						// Если событие принадлежит к таймеру
						if(i->second.type == event_type_t::TIMER)
							// Выполняем удаление таймера
							this->_watch.away(i->second.sock);
						// Выполняем удаление всего события
						this->_peers.erase(i);
					}
					// Выполняем разблокировку чтения базы событий
					this->_locker = false;
				}
			/**
			 * Для операционной системы FreeBSD, NetBSD, OpenBSD или MacOS X
			 */
			#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
				// Выполняем поиск файлового дескриптора в базе событий
				auto i = this->_peers.find(sock);
				// Если сокет есть в базе событий
				if((result = (i != this->_peers.end()) && (i->second.id == id))){
					// Выполняем блокировку чтения базы событий
					this->_locker = true;
					/**
					 * Определяем тип переданного события
					 */
					switch(static_cast <uint8_t> (type)){
						// Если событие установлено как отслеживание закрытия подключения
						case static_cast <uint8_t> (event_type_t::CLOSE): {
							// Выполняем поиск типа события и его режим работы
							auto j = i->second.mode.find(type);
							// Если режим работы события получен
							if((result = (j != i->second.mode.end()))){
								// Выполняем отключение работы события
								j->second = event_mode_t::DISABLED;
								// Выполняем удаление типа события
								i->second.mode.erase(j);
							}
						} break;
						// Если событие установлено как таймер
						case static_cast <uint8_t> (event_type_t::TIMER):
						// Если событие принадлежит к потоку
						case static_cast <uint8_t> (event_type_t::STREAM): {
							// Выполняем поиск типа события и его режим работы
							auto j = i->second.mode.find(type);
							// Если режим работы события получен
							if((result = (j != i->second.mode.end()))){
								// Выполняем отключение работы события
								j->second = event_mode_t::DISABLED;
								// Выполняем удаление типа события
								i->second.mode.erase(j);
								// Если изменение события чтения найдено
								if(this->slot(sock, EVFILT_READ) != nullptr){
									// Исключаем сокет из результатов текущего опроса (буфер не сокращаем, он может обходиться)
									this->forget(sock);
									// Выполняем удаление всех изменений событий сокета
									this->unslot(sock);
									// Если удаляется таймер
									if(type == event_type_t::TIMER)
										// Выполняем удаление таймера
										this->_watch.away(i->second.sock);
								}
							}
						} break;
						// Если событие установлено как отслеживание события чтения из сокета
						case static_cast <uint8_t> (event_type_t::READ):
						// Если событие установлено как отслеживание события записи в сокет
						case static_cast <uint8_t> (event_type_t::WRITE): {
							// Выполняем поиск типа события и его режим работы
							auto j = i->second.mode.find(type);
							// Если режим работы события получен
							if((result = (j != i->second.mode.end()))){
								// Выполняем отключение работы события
								j->second = event_mode_t::DISABLED;
								// Выполняем поиск изменения события для нужного фильтра
								struct kevent * k = this->slot(sock, (type == event_type_t::READ ? EVFILT_READ : EVFILT_WRITE));
								/**
								 * Если изменение найдено, выключаем фильтр в ядре.
								 * EV_DELETE в постоянном списке изменений оставлять нельзя: список подаётся в
								 * каждый вызов kevent, повторное удаление вернёт ENOENT с флагом EV_ERROR,
								 * и обход примет это за ошибку сокета и закроет подключение
								 */
								if(k != nullptr)
									// Выполняем выключение фильтра события
									EV_SET(k, k->ident, k->filter, EV_ADD | EV_CLEAR | EV_DISABLE, 0, 0, &i->second);
								// Выполняем удаление типа события
								i->second.mode.erase(j);
							}
						} break;
					}
					// Если список режимов событий пустой
					if(i->second.mode.empty() || ((i->second.mode.size() == 1) && (i->second.mode.find(event_type_t::CLOSE) != i->second.mode.end()))){
						// Исключаем сокет из результатов текущего опроса (буфер не сокращаем, он может обходиться)
						this->forget(sock);
						// Выполняем удаление всех изменений событий сокета
						this->unslot(sock);
						// Если событие принадлежит к таймеру
						if(i->second.type == event_type_t::TIMER)
							// Выполняем удаление таймера
							this->_watch.away(i->second.sock);
						// Выполняем удаление всего события
						this->_peers.erase(i);
					}
					// Выполняем разблокировку чтения базы событий
					this->_locker = false;
				}
			#endif
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type)), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод добавления файлового дескриптора в базу событий
 *
 * @param id       идентификатор записи
 * @param sock     сокет для добавления
 * @param callback функция обратного вызова при получении события
 * @param delay    задержка времени для создания таймеров
 * @param persist  флаг персистентного таймера
 * @return         результат работы функции
 */
bool awh::Base::add(const uint64_t id, SOCKET & sock, callback_t callback, const uint32_t delay, const bool persist) noexcept {
	// Результат работы функции
	bool result = false;
	// Если сокет передан верный
	if((sock != INVALID_SOCKET) || (delay > 0)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Если количество добавленных файловых дескрипторов для отслеживания не достигло предела
			if(this->_peers.size() < AWH_MAX_COUNT_FDS){
				/**
				 * Выполняем блокировку чтения базы событий. Страж снимает блокировку на любом
				 * пути выхода: при неудачном создании таймера метод выходит досрочно, и раньше
				 * флаг оставался взведённым, а база событий больше никогда не опрашивалась
				 */
				const LockerGuard locker(this->_locker);
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					// Выполняем поиск файлового дескриптора в базе событий
					auto i = this->_peers.find(sock);
					// Если сокет есть в базе событий
					if((result = (i != this->_peers.end()) && (i->second.id == id))){
						// Если функция обратного вызова передана
						if(callback != nullptr)
							// Выполняем установку функции обратного вызова
							i->second.callback = callback;
					// Если файлового дескриптора в базе событий нет
					} else {
						// Объект текущего события
						peer_t * item = nullptr;
						// Если нам необходимо создать таймер
						if(delay > 0){
							// Выполняем создание сокетов
							sock = this->_watch.create();
							// Выполняем инициализацию таймера
							if(sock == INVALID_SOCKET)
								// Выходим из функции
								return result;
							// Выполняем добавление в список параметров для отслеживания
							auto ret = this->_peers.emplace(sock, peer_t());
							// Выполняем установку задержки времени таймера
							ret.first->second.delay = delay;
							// Выполняем установку флага персистентного таймера
							ret.first->second.persist = persist;
							// Выполняем установку типа таймера
							ret.first->second.type = event_type_t::TIMER;
							// Выполняем установку событий таймера
							ret.first->second.mode.emplace(event_type_t::TIMER, event_mode_t::DISABLED);
							// Выполняем получение объекта текущего события
							item = &ret.first->second;
						// Если нам необходимо создать обычное событие
						} else {
							// Выполняем добавление в список параметров для отслеживания
							auto ret = this->_peers.emplace(sock, peer_t());
							// Выключаем установку событий модуля
							ret.first->second.mode = {
								{event_type_t::READ, event_mode_t::DISABLED},
								{event_type_t::WRITE, event_mode_t::DISABLED},
								{event_type_t::CLOSE, event_mode_t::DISABLED},
								{event_type_t::STREAM, event_mode_t::DISABLED}
							};
							// Выполняем получение объекта текущего события
							item = &ret.first->second;
						}
						// Если объект текущего события получен
						if((result = (item != nullptr))){
							// Устанавливаем идентификатор записи
							item->id = id;
							// Выполняем установку файлового дескриптора события
							item->sock = sock;
							// Если функция обратного вызова передана
							if(callback != nullptr)
								// Выполняем установку функции обратного вызова
								item->callback = callback;
							// Устанавливаем сокет в список для отслеживания
							this->_events.push_back((WSAPOLLFD){});
							// Выполняем установку файлового дескриптора
							this->_events.back().fd = sock;
							// Сбрасываем состояние события
							this->_events.back().revents = 0;
						}
					}
				/**
				 * Для операционной системы Sun Solaris
				 */
				#elif __sun__
					// Выполняем поиск файлового дескриптора в базе событий
					auto i = this->_peers.find(sock);
					// Если сокет есть в базе событий
					if((result = (i != this->_peers.end()) && (i->second.id == id))){
						// Если функция обратного вызова передана
						if(callback != nullptr)
							// Выполняем установку функции обратного вызова
							i->second.callback = callback;
					// Если файлового дескриптора в базе событий нет
					} else {
						// Объект текущего события
						peer_t * item = nullptr;
						// Если нам необходимо создать таймер
						if(delay > 0){
							// Выполняем создание сокетов
							sock = this->_watch.create();
							// Выполняем инициализацию таймера
							if(sock == INVALID_SOCKET)
								// Выходим из функции
								return result;
							// Выполняем добавление в список параметров для отслеживания
							auto ret = this->_peers.emplace(sock, peer_t());
							// Выполняем установку задержки времени таймера
							ret.first->second.delay = delay;
							// Выполняем установку флага персистентного таймера
							ret.first->second.persist = persist;
							// Выполняем установку типа таймера
							ret.first->second.type = event_type_t::TIMER;
							// Выполняем установку событий таймера
							ret.first->second.mode.emplace(event_type_t::TIMER, event_mode_t::DISABLED);
							// Выполняем получение объекта текущего события
							item = &ret.first->second;
						// Если нам необходимо создать обычное событие
						} else {
							// Выполняем добавление в список параметров для отслеживания
							auto ret = this->_peers.emplace(sock, peer_t());
							// Выключаем установку событий модуля
							ret.first->second.mode = {
								{event_type_t::READ, event_mode_t::DISABLED},
								{event_type_t::WRITE, event_mode_t::DISABLED},
								{event_type_t::CLOSE, event_mode_t::DISABLED},
								{event_type_t::STREAM, event_mode_t::DISABLED}
							};
							// Выполняем получение объекта текущего события
							item = &ret.first->second;
						}
						// Если объект текущего события получен
						if((result = (item != nullptr))){
							// Устанавливаем идентификатор записи
							item->id = id;
							// Выполняем установку файлового дескриптора события
							item->sock = sock;
							// Если функция обратного вызова передана
							if(callback != nullptr)
								// Выполняем установку функции обратного вызова
								item->callback = callback;
							// Устанавливаем сокет в список для отслеживания
							this->_events.push_back((struct pollfd){});
							// Выполняем установку файлового дескриптора
							this->_events.back().fd = sock;
							// Сбрасываем состояние события
							this->_events.back().revents = 0;
						}
					}
				/**
				 * Для операционной системы Linux
				 */
				#elif __linux__
					// Выполняем поиск файлового дескриптора в базе событий
					auto i = this->_peers.find(sock);
					// Если сокет есть в базе событий
					if((result = (i != this->_peers.end()) && (i->second.id == id))){
						// Если функция обратного вызова передана
						if(callback != nullptr)
							// Выполняем установку функции обратного вызова
							i->second.callback = callback;
					// Если файлового дескриптора в базе событий нет
					} else {
						// Объект текущего события
						peer_t * item = nullptr;
						// Если нам необходимо создать таймер
						if(delay > 0){
							// Выполняем создание сокетов
							sock = this->_watch.create();
							// Выполняем инициализацию таймера
							if(sock == INVALID_SOCKET)
								// Выходим из функции
								return result;
							// Выполняем добавление в список параметров для отслеживания
							auto ret = this->_peers.emplace(sock, peer_t());
							// Выполняем установку задержки времени таймера
							ret.first->second.delay = delay;
							// Выполняем установку флага персистентного таймера
							ret.first->second.persist = persist;
							// Выполняем установку типа таймера
							ret.first->second.type = event_type_t::TIMER;
							// Выполняем установку событий таймера
							ret.first->second.mode.emplace(event_type_t::TIMER, event_mode_t::DISABLED);
							// Выполняем получение объекта текущего события
							item = &ret.first->second;
						// Если нам необходимо создать обычное событие
						} else {
							// Выполняем добавление в список параметров для отслеживания
							auto ret = this->_peers.emplace(sock, peer_t());
							// Выключаем установку событий модуля
							ret.first->second.mode = {
								{event_type_t::READ, event_mode_t::DISABLED},
								{event_type_t::WRITE, event_mode_t::DISABLED},
								{event_type_t::CLOSE, event_mode_t::DISABLED},
								{event_type_t::STREAM, event_mode_t::DISABLED}
							};
							// Выполняем получение объекта текущего события
							item = &ret.first->second;
						}
						// Если объект текущего события получен
						if((result = (item != nullptr))){
							// Устанавливаем идентификатор записи
							item->id = id;
							// Выполняем установку файлового дескриптора события
							item->sock = sock;
							// Если функция обратного вызова передана
							if(callback != nullptr)
								// Выполняем установку функции обратного вызова
								item->callback = callback;
							// Устанавливаем новый объект для изменений события
							this->_change.push_back((struct epoll_event){});
							/**
							 * Буфер результатов опроса (_events) здесь не расширяем: метод может быть вызван
							 * из обработчика во время обхода буфера, и перераспределение памяти оставит обход
							 * с висячей ссылкой. Буфер подгоняется под размер списка изменений перед опросом
							 */
							// Выполняем установку указателя на основное событие
							this->_change.back().data.ptr = item;
							// Устанавливаем флаг ожидания отключения сокета
							this->_change.back().events = EPOLLERR;
							// Выполняем изменение параметров события
							if(!(result = (::epoll_ctl(this->_efd, EPOLL_CTL_ADD, sock, &this->_change.back()) == 0))){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, delay, persist), log_t::flag_t::CRITICAL, ::strerror(errno));
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
								#endif
							}
						}
					}
				/**
				 * Для операционной системы FreeBSD, NetBSD, OpenBSD или MacOS X
				 */
				#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
					// Выполняем поиск файлового дескриптора в базе событий
					auto i = this->_peers.find(sock);
					// Если сокет есть в базе событий
					if((result = (i != this->_peers.end()) && (i->second.id == id))){
						// Если функция обратного вызова передана
						if(callback != nullptr)
							// Выполняем установку функции обратного вызова
							i->second.callback = callback;
					// Если файлового дескриптора в базе событий нет
					} else {
						// Объект текущего события
						peer_t * item = nullptr;
						// Если нам необходимо создать таймер
						if(delay > 0){
							// Выполняем создание сокетов
							sock = this->_watch.create();
							// Выполняем инициализацию таймера
							if(sock == INVALID_SOCKET)
								// Выходим из функции
								return result;
							// Выполняем добавление в список параметров для отслеживания
							auto ret = this->_peers.emplace(sock, peer_t());
							// Выполняем установку задержки времени таймера
							ret.first->second.delay = delay;
							// Выполняем установку флага персистентного таймера
							ret.first->second.persist = persist;
							// Выполняем установку типа таймера
							ret.first->second.type = event_type_t::TIMER;
							// Выполняем установку событий таймера
							ret.first->second.mode.emplace(event_type_t::TIMER, event_mode_t::DISABLED);
							// Выполняем получение объекта текущего события
							item = &ret.first->second;
						// Если нам необходимо создать обычное событие
						} else {
							// Выполняем добавление в список параметров для отслеживания
							auto ret = this->_peers.emplace(sock, peer_t());
							// Выключаем установку событий модуля
							ret.first->second.mode = {
								{event_type_t::READ, event_mode_t::DISABLED},
								{event_type_t::WRITE, event_mode_t::DISABLED},
								{event_type_t::CLOSE, event_mode_t::DISABLED},
								{event_type_t::STREAM, event_mode_t::DISABLED}
							};
							// Выполняем получение объекта текущего события
							item = &ret.first->second;
						}
						// Если объект текущего события получен
						if((result = (item != nullptr))){
							// Устанавливаем идентификатор записи
							item->id = id;
							// Выполняем установку файлового дескриптора события
							item->sock = sock;
							// Если функция обратного вызова передана
							if(callback != nullptr)
								// Выполняем установку функции обратного вызова
								item->callback = callback;
							// Устанавливаем новый объект для изменений события
							this->_change.push_back((struct kevent){});
							/**
							 * Буфер результатов опроса (_events) здесь не расширяем: метод может быть вызван
							 * из обработчика во время обхода буфера, и перераспределение памяти оставит обход
							 * с висячей ссылкой. Буфер подгоняется под размер списка изменений перед опросом
							 */
							// Выполняем заполнение нулями всю структуру изменений
							::memset(&this->_change.back(), 0, sizeof(this->_change.back()));
							// Устанавливаем идентификатор файлового дескриптора
							this->_change.back().ident = sock;
							/**
							 * Создаём изменение только для фильтра чтения. Фильтры kqueue не битовые маски:
							 * прежнее «EVFILT_READ | EVFILT_WRITE» и так давало EVFILT_READ. Изменение для
							 * фильтра записи создаётся отдельно при первом включении записи
							 */
							EV_SET(&this->_change.back(), this->_change.back().ident, EVFILT_READ, EV_ADD | EV_CLEAR | EV_DISABLE, 0, 0, item);
						}
					}
				#endif
			// Выводим сообщение об ошибке
			} else this->_log->print("SOCKET=%d cannot be added because the number of events being monitored has already reached the limit of %llu", log_t::flag_t::WARNING, sock, AWH_MAX_COUNT_FDS);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, delay, persist), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки режима работы модуля
 *
 * @param id   идентификатор записи
 * @param sock сокет для установки режима работы
 * @param type тип событий модуля для которого требуется сменить режим работы
 * @param mode флаг режима работы модуля
 * @return     результат работы функции
 */
bool awh::Base::mode(const uint64_t id, const SOCKET sock, const event_type_t type, const event_mode_t mode) noexcept {
	// Результат работы функции
	bool result = false;
	// Если сокет и его тип переданы правильно
	if((sock != INVALID_SOCKET) && (type != event_type_t::NONE)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем поиск файлового дескриптора в базе событий
			auto i = this->_peers.find(sock);
			// Если сокет есть в базе событий
			if((i != this->_peers.end()) && (i->second.id == id)){
				// Выполняем поиск события модуля
				auto j = i->second.mode.find(type);
				// Если событие для изменения режима работы модуля найдено
				if((result = ((j != i->second.mode.end()) && (j->second != mode)))){
					// Выполняем установку режима работы модуля
					j->second = mode;
					/**
					 * Для операционной системы MS Windows
					 */
					#if _WIN32 || _WIN64
						// Если тип установлен как не закрытие подключения
						if(type != event_type_t::CLOSE){
							// Выполняем поиск файлового дескриптора из списка событий
							for(auto k = this->_events.begin(); k != this->_events.end(); ++k){
								// Если сокет найден
								if(k->fd == sock){
									// Очищаем полученное событие
									k->revents = 0;
									/**
									 * Определяем тип события
									 */
									switch(static_cast <uint8_t> (type)){
										// Если событие установлено как таймер
										case static_cast <uint8_t> (event_type_t::TIMER): {
											/**
											 * Определяем режим работы модуля
											 */
											switch(static_cast <uint8_t> (mode)){
												// Если нужно активировать событие работы таймера
												case static_cast <uint8_t> (event_mode_t::ENABLED): {
													// Устанавливаем флаг ожидания готовности файлового дескриптора на чтение
													k->events |= POLLIN;
													// Выполняем активацию таймера на указанное время
													this->_watch.wait(k->fd, i->second.delay);
												} break;
												// Если нужно деактивировать событие работы таймера
												case static_cast <uint8_t> (event_mode_t::DISABLED): {
													// Снимаем флаг ожидания готовности файлового дескриптора на чтение
													k->events ^= POLLIN;
													// Выполняем отмену ожидания таймера (уведомитель остаётся, таймер можно включить снова)
													this->_watch.cancel(k->fd);
												} break;
											}
										} break;
										// Если событие принадлежит к потоку
										case static_cast <uint8_t> (event_type_t::STREAM): {
											// Устанавливаем тип события сокета
											i->second.type = type;
											/**
											 * Определяем режим работы модуля
											 */
											switch(static_cast <uint8_t> (mode)){
												// Если нужно активировать событие чтения из сокета
												case static_cast <uint8_t> (event_mode_t::ENABLED):
													// Устанавливаем флаг ожидания готовности файлового дескриптора на чтение
													k->events |= POLLIN;
												break;
												// Если нужно деактивировать событие чтения из сокета
												case static_cast <uint8_t> (event_mode_t::DISABLED):
													// Снимаем флаг ожидания готовности файлового дескриптора на чтение
													k->events ^= POLLIN;
												break;
											}
										} break;
										// Если событие является чтением данных из сокета
										case static_cast <uint8_t> (event_type_t::READ): {
											/**
											 * Определяем режим работы модуля
											 */
											switch(static_cast <uint8_t> (mode)){
												// Если нужно активировать событие чтения из сокета
												case static_cast <uint8_t> (event_mode_t::ENABLED):
													// Устанавливаем флаг ожидания готовности файлового дескриптора на чтение
													k->events |= POLLIN;
												break;
												// Если нужно деактивировать событие чтения из сокета
												case static_cast <uint8_t> (event_mode_t::DISABLED):
													// Снимаем флаг ожидания готовности файлового дескриптора на чтение
													k->events ^= POLLIN;
												break;
											}
										} break;
										// Если событие является записи данных в сокет
										case static_cast <uint8_t> (event_type_t::WRITE): {
											/**
											 * Определяем режим работы модуля
											 */
											switch(static_cast <uint8_t> (mode)){
												// Если нужно активировать событие записи в сокет
												case static_cast <uint8_t> (event_mode_t::ENABLED):
													// Устанавливаем флаг отслеживания записи данных в сокет
													k->events |= POLLOUT;
												break;
												// Если нужно деактивировать событие записи в сокет
												case static_cast <uint8_t> (event_mode_t::DISABLED):
													// Снимаем флаг ожидания готовности файлового дескриптора на запись
													k->events ^= POLLOUT;
												break;
											}
										} break;
									}
									// Выходим из цикла
									break;
								}
							}
						}
					/**
					 * Для операционной системы Sun Solaris
					 */
					#elif __sun__
						// Выполняем поиск файлового дескриптора из списка событий
						for(auto k = this->_events.begin(); k != this->_events.end(); ++k){
							// Если сокет найден
							if(k->fd == sock){
								// Очищаем полученное событие
								k->revents = 0;
								/**
								 * Определяем тип события
								 */
								switch(static_cast <uint8_t> (type)){
									// Если событие установлено как таймер
									case static_cast <uint8_t> (event_type_t::TIMER): {
										/**
										 * Определяем режим работы модуля
										 */
										switch(static_cast <uint8_t> (mode)){
											// Если нужно активировать событие работы таймера
											case static_cast <uint8_t> (event_mode_t::ENABLED): {
												// Устанавливаем флаг ожидания готовности файлового дескриптора на чтение
												k->events |= POLLIN;
												// Выполняем добавление списка файловых дескрипторов для отслеживания
												if(::write(this->_wfd, this->_events.data(), sizeof(struct pollfd) * this->_events.size()) <= 0){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
													#endif
												// Выполняем активацию таймера на указанное время
												} else this->_watch.wait(sock, i->second.delay);
											} break;
											// Если нужно деактивировать событие работы таймера
											case static_cast <uint8_t> (event_mode_t::DISABLED): {
												// Снимаем флаг ожидания готовности файлового дескриптора на чтение
												k->events ^= POLLIN;
												// Выполняем добавление списка файловых дескрипторов для отслеживания
												if(::write(this->_wfd, this->_events.data(), sizeof(struct pollfd) * this->_events.size()) <= 0){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
													#endif
												// Выполняем отмену ожидания таймера (уведомитель остаётся, таймер можно включить снова)
												} else this->_watch.cancel(sock);
											} break;
										}
									} break;
									// Если событие принадлежит к потоку
									case static_cast <uint8_t> (event_type_t::STREAM): {
										// Устанавливаем тип события сокета
										i->second.type = type;
										/**
										 * Определяем режим работы модуля
										 */
										switch(static_cast <uint8_t> (mode)){
											// Если нужно активировать событие работы таймера
											case static_cast <uint8_t> (event_mode_t::ENABLED): {
												// Устанавливаем флаг ожидания готовности файлового дескриптора на чтение
												k->events |= POLLIN;
												// Выполняем добавление списка файловых дескрипторов для отслеживания
												if(::write(this->_wfd, this->_events.data(), sizeof(struct pollfd) * this->_events.size()) <= 0){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
													#endif
												}
											} break;
											// Если нужно деактивировать событие работы таймера
											case static_cast <uint8_t> (event_mode_t::DISABLED): {
												// Снимаем флаг ожидания готовности файлового дескриптора на чтение
												k->events ^= POLLIN;
												// Выполняем добавление списка файловых дескрипторов для отслеживания
												if(::write(this->_wfd, this->_events.data(), sizeof(struct pollfd) * this->_events.size()) <= 0){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
													#endif
												}
											} break;
										}
									} break;
									// Если событие установлено как отслеживание закрытия подключения
									case static_cast <uint8_t> (event_type_t::CLOSE): {
										/**
										 * Определяем режим работы модуля
										 */
										switch(static_cast <uint8_t> (mode)){
											// Если нужно активировать событие чтения из сокета
											case static_cast <uint8_t> (event_mode_t::ENABLED): {
												// Устанавливаем флаг ожидания готовности файлового дескриптора на чтение
												k->events |= POLLHUP;
												// Выполняем добавление списка файловых дескрипторов для отслеживания
												if(::write(this->_wfd, this->_events.data(), sizeof(struct pollfd) * this->_events.size()) <= 0){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
													#endif
												}
											} break;
											// Если нужно деактивировать событие чтения из сокета
											case static_cast <uint8_t> (event_mode_t::DISABLED): {
												// Выполняем удаление флагов отслеживания закрытия подключения
												k->events ^= POLLHUP;
												// Выполняем добавление списка файловых дескрипторов для отслеживания
												if(::write(this->_wfd, this->_events.data(), sizeof(struct pollfd) * this->_events.size()) <= 0){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
													#endif
												}
											} break;
										}
									} break;
									// Если событие является чтением данных из сокета
									case static_cast <uint8_t> (event_type_t::READ): {
										/**
										 * Определяем режим работы модуля
										 */
										switch(static_cast <uint8_t> (mode)){
											// Если нужно активировать событие чтения из сокета
											case static_cast <uint8_t> (event_mode_t::ENABLED): {
												// Устанавливаем флаг ожидания готовности файлового дескриптора на чтение
												k->events |= POLLIN;
												// Выполняем добавление списка файловых дескрипторов для отслеживания
												if(::write(this->_wfd, this->_events.data(), sizeof(struct pollfd) * this->_events.size()) <= 0){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
													#endif
												}
											} break;
											// Если нужно деактивировать событие чтения из сокета
											case static_cast <uint8_t> (event_mode_t::DISABLED): {
												// Снимаем флаг ожидания готовности файлового дескриптора на чтение
												k->events ^= POLLIN;
												// Выполняем добавление списка файловых дескрипторов для отслеживания
												if(::write(this->_wfd, this->_events.data(), sizeof(struct pollfd) * this->_events.size()) <= 0){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
													#endif
												}
											} break;
										}
									} break;
									// Если событие является записи данных в сокет
									case static_cast <uint8_t> (event_type_t::WRITE): {
										/**
										 * Определяем режим работы модуля
										 */
										switch(static_cast <uint8_t> (mode)){
											// Если нужно активировать событие записи в сокет
											case static_cast <uint8_t> (event_mode_t::ENABLED): {
												// Устанавливаем флаг отслеживания записи данных в сокет
												k->events |= POLLOUT;
												// Выполняем добавление списка файловых дескрипторов для отслеживания
												if(::write(this->_wfd, this->_events.data(), sizeof(struct pollfd) * this->_events.size()) <= 0){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
													#endif
												}
											} break;
											// Если нужно деактивировать событие записи в сокет
											case static_cast <uint8_t> (event_mode_t::DISABLED): {
												// Снимаем флаг ожидания готовности файлового дескриптора на запись
												k->events ^= POLLOUT;
												// Выполняем добавление списка файловых дескрипторов для отслеживания
												if(::write(this->_wfd, this->_events.data(), sizeof(struct pollfd) * this->_events.size()) <= 0){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
													#endif
												}
											} break;
										}
									} break;
								}
								// Выходим из цикла
								break;
							}
						}
					/**
					 * Для операционной системы Linux
					 */
					#elif __linux__
						// Выполняем поиск файлового дескриптора из списка событий
						for(auto k = this->_change.begin(); k != this->_change.end(); ++k){
							// Если сокет найден
							if(reinterpret_cast <peer_t *> (k->data.ptr)->sock == sock){
								/**
								 * Определяем тип события
								 */
								switch(static_cast <uint8_t> (type)){
									// Если событие установлено как таймер
									case static_cast <uint8_t> (event_type_t::TIMER): {
										/**
										 * Определяем режим работы модуля
										 */
										switch(static_cast <uint8_t> (mode)){
											// Если нужно активировать событие таймера
											case static_cast <uint8_t> (event_mode_t::ENABLED): {
												// Устанавливаем флаг ожидания готовности файлового дескриптора на чтение
												k->events |= (EPOLLIN | EPOLLET);
												// Выполняем изменение параметров события
												if(::epoll_ctl(this->_efd, EPOLL_CTL_MOD, sock, &(* k)) != 0){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
													#endif
												// Выполняем активацию таймера на указанное время
												} else this->_watch.wait(sock, i->second.delay);
											} break;
											// Если нужно деактивировать событие таймера
											case static_cast <uint8_t> (event_mode_t::DISABLED): {
												// Снимаем флаг ожидания готовности файлового дескриптора на чтение
												k->events ^= (EPOLLIN | EPOLLET);
												// Выполняем изменение параметров события
												if(::epoll_ctl(this->_efd, EPOLL_CTL_MOD, sock, &(* k)) != 0){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
													#endif
												// Выполняем отмену ожидания таймера (уведомитель остаётся, таймер можно включить снова)
												} else this->_watch.cancel(sock);
											} break;
										}
									} break;
									// Если событие принадлежит к потоку
									case static_cast <uint8_t> (event_type_t::STREAM): {
										// Устанавливаем тип события сокета
										i->second.type = type;
										/**
										 * Определяем режим работы модуля
										 */
										switch(static_cast <uint8_t> (mode)){
											// Если нужно активировать событие чтения из сокета
											case static_cast <uint8_t> (event_mode_t::ENABLED): {
												// Устанавливаем флаг ожидания готовности файлового дескриптора на чтение
												k->events |= (EPOLLIN | EPOLLET);
												// Выполняем изменение параметров события
												if(::epoll_ctl(this->_efd, EPOLL_CTL_MOD, sock, &(* k)) != 0){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
													#endif
												}
											} break;
											// Если нужно деактивировать событие чтения из сокета
											case static_cast <uint8_t> (event_mode_t::DISABLED): {
												// Снимаем флаг ожидания готовности файлового дескриптора на чтение
												k->events ^= (EPOLLIN | EPOLLET);
												// Выполняем изменение параметров события
												if(::epoll_ctl(this->_efd, EPOLL_CTL_MOD, sock, &(* k)) != 0){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
													#endif
												}
											} break;
										}
									} break;
									// Если событие установлено как отслеживание закрытия подключения
									case static_cast <uint8_t> (event_type_t::CLOSE): {
										/**
										 * Определяем режим работы модуля
										 */
										switch(static_cast <uint8_t> (mode)){
											// Если нужно активировать событие чтения из сокета
											case static_cast <uint8_t> (event_mode_t::ENABLED): {
												// Выполняем установку флагов отслеживания закрытия подключения
												k->events |= (EPOLLRDHUP | EPOLLHUP);
												// Выполняем изменение параметров события
												if(::epoll_ctl(this->_efd, EPOLL_CTL_MOD, sock, &(* k)) != 0){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
													#endif
												}
											} break;
											// Если нужно деактивировать событие чтения из сокета
											case static_cast <uint8_t> (event_mode_t::DISABLED): {
												// Выполняем удаление флагов отслеживания закрытия подключения
												k->events ^= (EPOLLRDHUP | EPOLLHUP);
												// Выполняем изменение параметров события
												if(::epoll_ctl(this->_efd, EPOLL_CTL_MOD, sock, &(* k)) != 0){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
													#endif
												}
											} break;
										}
									} break;
									// Если событие является чтением данных из сокета
									case static_cast <uint8_t> (event_type_t::READ): {
										/**
										 * Определяем режим работы модуля
										 */
										switch(static_cast <uint8_t> (mode)){
											// Если нужно активировать событие чтения из сокета
											case static_cast <uint8_t> (event_mode_t::ENABLED): {
												// Устанавливаем флаг ожидания готовности файлового дескриптора на чтение
												k->events |= EPOLLIN;
												// Выполняем изменение параметров события
												if(::epoll_ctl(this->_efd, EPOLL_CTL_MOD, sock, &(* k)) != 0){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
													#endif
												}
											} break;
											// Если нужно деактивировать событие чтения из сокета
											case static_cast <uint8_t> (event_mode_t::DISABLED): {
												// Снимаем флаг ожидания готовности файлового дескриптора на чтение
												k->events ^= EPOLLIN;
												// Выполняем изменение параметров события
												if(::epoll_ctl(this->_efd, EPOLL_CTL_MOD, sock, &(* k)) != 0){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
													#endif
												}
											} break;
										}
									} break;
									// Если событие является записи данных в сокет
									case static_cast <uint8_t> (event_type_t::WRITE): {
										/**
										 * Определяем режим работы модуля
										 */
										switch(static_cast <uint8_t> (mode)){
											// Если нужно активировать событие записи в сокет
											case static_cast <uint8_t> (event_mode_t::ENABLED): {
												// Устанавливаем флаг отслеживания записи данных в сокет
												k->events |= EPOLLOUT;
												// Выполняем изменение параметров события
												if(::epoll_ctl(this->_efd, EPOLL_CTL_MOD, sock, &(* k)) != 0){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
													#endif
												}
											} break;
											// Если нужно деактивировать событие записи в сокет
											case static_cast <uint8_t> (event_mode_t::DISABLED): {
												// Снимаем флаг ожидания готовности файлового дескриптора на запись
												k->events ^= EPOLLOUT;
												// Выполняем изменение параметров события
												if(::epoll_ctl(this->_efd, EPOLL_CTL_MOD, sock, &(* k)) != 0){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
													#endif
												}
											} break;
										}
									} break;
								}
								// Выходим из цикла
								break;
							}
						}
					/**
					 * Для операционной системы FreeBSD, NetBSD, OpenBSD или MacOS X
					 */
					#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
						// Если тип установлен как не закрытие подключения
						if(type != event_type_t::CLOSE){
							/**
							 * Фильтры kqueue являются малыми отрицательными числами, а не битами, поэтому у сокета
							 * отдельное изменение для чтения и отдельное для записи. Раньше одно общее изменение
							 * перезаписывалось: включение чтения затирало ещё не применённое включение записи,
							 * и очередь исходящих данных вставала
							 */
							const int16_t filter = (type == event_type_t::WRITE ? EVFILT_WRITE : EVFILT_READ);
							// Выполняем поиск изменения события для нужного фильтра
							struct kevent * k = this->slot(sock, filter);
							// Если изменения для фильтра записи ещё нет, а запись включается
							if((k == nullptr) && (filter == EVFILT_WRITE) && (mode == event_mode_t::ENABLED) && (this->slot(sock, EVFILT_READ) != nullptr)){
								// Устанавливаем новый объект для изменений события
								this->_change.push_back((struct kevent){});
								// Выполняем заполнение нулями всю структуру изменений
								::memset(&this->_change.back(), 0, sizeof(this->_change.back()));
								// Получаем добавленное изменение события
								k = &this->_change.back();
							}
							// Если изменение события найдено
							if(k != nullptr){
								/**
								 * Определяем тип события
								 */
								switch(static_cast <uint8_t> (type)){
									// Если событие установлено как таймер
									case static_cast <uint8_t> (event_type_t::TIMER): {
										/**
										 * Определяем режим работы модуля
										 */
										switch(static_cast <uint8_t> (mode)){
											// Если нужно активировать событие работы таймера
											case static_cast <uint8_t> (event_mode_t::ENABLED): {
												// Выполняем смену режима работы отлова события
												EV_SET(k, sock, EVFILT_READ, EV_ADD | EV_CLEAR | EV_ENABLE, 0, 0, &i->second);
												// Выполняем активацию таймера на указанное время
												this->_watch.wait(sock, i->second.delay);
											} break;
											// Если нужно деактивировать событие работы таймера
											case static_cast <uint8_t> (event_mode_t::DISABLED): {
												// Выполняем смену режима работы отлова события
												EV_SET(k, sock, EVFILT_READ, EV_ADD | EV_CLEAR | EV_DISABLE, 0, 0, &i->second);
												// Выполняем отмену ожидания таймера (уведомитель остаётся, таймер можно включить снова)
												this->_watch.cancel(sock);
											} break;
										}
									} break;
									// Если событие принадлежит к потоку
									case static_cast <uint8_t> (event_type_t::STREAM):
										// Устанавливаем тип события сокета
										i->second.type = type;
									// Если событие является чтением данных из сокета
									case static_cast <uint8_t> (event_type_t::READ):
									// Если событие является записи данных в сокет
									case static_cast <uint8_t> (event_type_t::WRITE): {
										/**
										 * Определяем режим работы модуля
										 */
										switch(static_cast <uint8_t> (mode)){
											// Если нужно активировать событие
											case static_cast <uint8_t> (event_mode_t::ENABLED):
												// Выполняем смену режима работы отлова события
												EV_SET(k, sock, filter, EV_ADD | EV_CLEAR | EV_ENABLE, 0, 0, &i->second);
											break;
											// Если нужно деактивировать событие
											case static_cast <uint8_t> (event_mode_t::DISABLED):
												// Выполняем смену режима работы отлова события
												EV_SET(k, sock, filter, EV_ADD | EV_CLEAR | EV_DISABLE, 0, 0, &i->second);
											break;
										}
									} break;
								}
							}
						}
					#endif
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод проверки запущена ли в данный момент база событий
 *
 * @return результат проверки запущена ли база событий
 */
bool awh::Base::launched() const noexcept {
	// Выполняем проверку запущена ли работа базы событий
	return this->_launched;
}
/**
 * @brief Метод очистки списка событий
 *
 */
void awh::Base::clear() noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем блокировку чтения базы событий
		this->_locker = true;
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Сокет найденный сокет для удаления
			SOCKET sock = INVALID_SOCKET;
			// Выполняем поиск файлового дескриптора из списка событий
			for(auto i = this->_events.begin(); i != this->_events.end();){
				// Получаем текущий сокет
				sock = i->fd;
				// Очищаем полученное событие
				i->revents = 0;
				// Выполняем сброс файлового дескриптора
				i->fd = INVALID_SOCKET;
				// Выполняем удаление события из списка отслеживания
				i = this->_events.erase(i);
				// Выполняем поиск файлового дескриптора в базе событий
				auto j = this->_peers.find(sock);
				// Если сокет есть в базе событий
				if(j != this->_peers.end()){
					// Если событие принадлежит к таймеру
					if(j->second.type == event_type_t::TIMER)
						// Выполняем удаление таймера
						this->_watch.away(j->second.sock);
				}
			}
		/**
		 * Для операционной системы Sun Solaris
		 */
		#elif __sun__
			// Сокет найденный сокет для удаления
			SOCKET sock = INVALID_SOCKET;
			// Выполняем поиск файлового дескриптора из списка событий
			for(auto i = this->_events.begin(); i != this->_events.end();){
				// Получаем текущий сокет
				sock = i->fd;
				// Очищаем полученное событие
				i->revents = 0;
				// Выполняем сброс файлового дескриптора
				i->fd = INVALID_SOCKET;
				// Выполняем удаление события из списка отслеживания
				i = this->_events.erase(i);
				// Выполняем поиск файлового дескриптора в базе событий
				auto j = this->_peers.find(sock);
				// Если сокет есть в базе событий
				if(j != this->_peers.end()){
					// Если событие принадлежит к таймеру
					if(j->second.type == event_type_t::TIMER)
						// Выполняем удаление таймера
						this->_watch.away(j->second.sock);
				}
			}
		/**
		 * Для операционной системы Linux
		 */
		#elif __linux__
			// Выполняем поиск файлового дескриптора из списка изменений
			for(auto i = this->_change.begin(); i != this->_change.end();){
				// Выполняем изменение параметров события
				::epoll_ctl(this->_efd, EPOLL_CTL_DEL, reinterpret_cast <peer_t *> (i->data.ptr)->sock, &(* i));
				// Если событие принадлежит к таймеру
				if(reinterpret_cast <peer_t *> (i->data.ptr)->type == event_type_t::TIMER)
					// Выполняем удаление таймера
					this->_watch.away(reinterpret_cast <peer_t *> (i->data.ptr)->sock);
				// Выполняем удаление события из списка изменений
				i = this->_change.erase(i);
			}
			/**
			 * Буфер результатов опроса не сокращаем (он может обходиться прямо сейчас,
			 * если метод вызван из обработчика), а помечаем все его записи пустыми.
			 * Помечаем весь буфер: вложенный опрос мог изменить количество записей
			 */
			for(size_t i = 0; i < this->_events.size(); i++){
				// Сбрасываем флаги события
				this->_events[i].events = 0;
				// Помечаем запись пустой
				this->_events[i].data.ptr = nullptr;
			}
			/**
			 * Удаляем участников, как и для kqueue. Раньше участники оставались: регистрация
			 * в ядре снята, а повторное добавление того же сокета находило старого участника
			 * с устаревшими режимами и не регистрировало событие в новом EPoll
			 */
			this->_peers.clear();
		/**
		 * Для операционной системы FreeBSD, NetBSD, OpenBSD или MacOS X
		 */
		#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
			/**
			 * Буфер результатов опроса не сокращаем (он может обходиться прямо сейчас,
			 * если метод вызван из обработчика), а помечаем все его записи недействительными.
			 * Все участники есть в списке изменений и удаляются ниже
			 */
			for(size_t i = 0; i < this->_events.size(); i++){
				// Сбрасываем флаги события
				this->_events[i].flags = 0;
				// Помечаем фильтр события недействительным
				this->_events[i].filter = 0;
				// Помечаем идентификатор события недействительным
				this->_events[i].ident = static_cast <uintptr_t> (INVALID_SOCKET);
			}
			// Выполняем поиск файлового дескриптора из списка изменений
			for(auto i = this->_change.begin(); i != this->_change.end();){
				// Выполняем поиск файлового дескриптора в базе событий
				auto j = this->_peers.find(i->ident);
				// Если сокет есть в базе событий
				if(j != this->_peers.end()){
					/**
					 * Определяем тип события к которому принадлежит сокет
					 */
					switch(static_cast <uint8_t> (j->second.type)){
						// Если событие принадлежит к таймеру
						case static_cast <uint8_t> (event_type_t::TIMER): {
							// Выполняем удаление события таймера
							EV_SET(&(* i), i->ident, EVFILT_READ, EV_DELETE, 0, 0, 0);
							// Выполняем удаление таймера
							this->_watch.away(j->second.sock);
						} break;
						// Если событие принадлежит к потоку
						case static_cast <uint8_t> (event_type_t::STREAM):
							// Выполняем удаление события таймера
							EV_SET(&(* i), i->ident, EVFILT_READ, EV_DELETE, 0, 0, 0);
						break;
						// Если это другое событие
						default:
							// Выполняем удаление объекта события
							EV_SET(&(* i), i->ident, EVFILT_READ | EVFILT_WRITE, EV_DELETE, 0, 0, 0);
					}
					// Выполняем удаление события
					this->_peers.erase(j);
				}
				// Выполняем удаление события из списка изменений
				i = this->_change.erase(i);
			}
		#endif
		// Выполняем разблокировку чтения базы событий
		this->_locker = false;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Выполняем разблокировку чтения базы событий
		this->_locker = false;
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод отправки пинка
 *
 */
void awh::Base::kick() noexcept {
	/**
	 * Выполняем обработку ошибки
	 */
	try {
		// Если работа базы событий запущена
		if(this->_works)
			/**
			 * Пересоздаём объект ядра и регистрируем в нём тех же участников. Пинок вызывается
			 * при переподключении и смене режима опроса: все события, таймеры и межпотоковые
			 * передатчики обязаны его пережить
			 */
			this->recreate();
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод остановки чтения базы событий
 *
 */
void awh::Base::stop() noexcept {
	/**
	 * Выполняем обработку ошибки
	 */
	try {
		// Если работа базы событий запущена
		if(this->_works){
			/**
			 * Для операционной системы Linux, MacOS X, FreeBSD, NetBSD или OpenBSD
			 */
			#if __linux__ || __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
				/**
				 * Если опрос работает в другом потоке (остановка по сигналу), то очищать базу событий
				 * здесь нельзя: поток опроса обходит её прямо сейчас, а ожидание событий без таймаута
				 * не прерывается пересозданием объекта ядра (Linux, FreeBSD), и start() не возвращался.
				 * Поэтому помечаем очистку отложенной, будим поток опроса, и он очищает базу сам
				 */
				if(this->_launched && this->isChildThread() && (this->_wakeup != INVALID_SOCKET)){
					// Помечаем очистку отложенной (до снятия флага работы, чтобы поток опроса её увидел)
					this->_defer = true;
					// Снимаем флаг работы базы событий
					this->_works = !this->_works;
					// Выполняем пробуждение потока опроса
					this->_wake.notify(0);
					// Выходим из функции
					return;
				}
			#endif
			// Снимаем флаг работы базы событий
			this->_works = !this->_works;
			// Выполняем очистку списка событий
			this->clear();
			// Выполняем деинициализацию базы событий
			this->init(event_mode_t::DISABLED);
			// Выполняем инициализацию базы событий
			this->init(event_mode_t::ENABLED);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод запуска чтения базы событий
 *
 */
void awh::Base::start() noexcept {
	// Если работа базы событий не запущена
	if(!this->_works){
		// Устанавливаем флаг работы базы событий
		this->_works = !this->_works;
		/**
		 * Если это  MacOS X, FreeBSD, NetBSD, OpenBSD или Sun Solaris
		 */
		#if __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__ || __sun__
			// Создаём объект временного таймаута
			struct timespec baseDelay = {0, 0};
			// Если установлен конкретный таймаут
			if((this->_rate > 0) && !this->_easily){
				// Устанавливаем время в секундах
				baseDelay.tv_sec = (this->_rate / 1000);
				// Устанавливаем время счётчика (наносекунды)
				baseDelay.tv_nsec = ((this->_rate % 1000) * 1000000L);
			}
		#endif
		// Запускаем работу часов
		this->_watch.start();
		// Получаем идентификатор потока
		this->_wid = this->wid();
		// Устанавливаем флаг запущенного опроса базы событий
		this->_launched = static_cast <bool> (this->_works);
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * Для операционной системы MS Windows
			 */
			#if _WIN32 || _WIN64
				// Количество событий для опроса
				size_t count = 0;
			#endif
			// Переменная опроса события
			int32_t poll = 0;
			/**
			 * Выполняем запуск базы события
			 */
			while(this->_works){
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					// Если опрос базы событий не заблокирован
					if(!this->_locker){
						// Если в списке достаточно событий для опроса
						if(!this->_events.empty()){
							// Выполняем запуск ожидания входящих событий сокетов
							poll = ::WSAPoll(this->_events.data(), this->_events.size(), (!this->_easily ? static_cast <int32_t> (this->_rate) : 0));
							// Если мы получили ошибку
							if(poll == SOCKET_ERROR){
								// Создаём буфер сообщения ошибки
								wchar_t message[256] = {0};
								// Выполняем формирование текста ошибки
								::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 256, 0);
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug(L"%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, message);
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print(L"%s", log_t::flag_t::CRITICAL, message);
								#endif
							// Если сработал таймаут
							} else if(poll == 0)
								// Компенсируем условие
								poll = 0;
							// Если опрос прошёл успешно
							else {
								// Получаем количество файловых дескрипторов для проверки
								count = this->_events.size();
								// Идентификатор события
								uint64_t id = 0;
								// Файловый дескриптор события
								SOCKET sock = INVALID_SOCKET;
								// Флаги статусов полученного сокета
								bool isRead = false, isWrite = false, isClose = false, isError = false;
								// Выполняем перебор всех файловых дескрипторов
								for(size_t i = 0; i < count; i++){
									// Если записей достаточно в списке
									if(i < this->_events.size()){
										// Зануляем идентификатор события
										id = 0;
										// Получаем объект файлового дескриптора
										auto & event = this->_events.at(i);
										// Получаем сокет
										sock = event.fd;
										// Получаем флаг достуности чтения из сокета
										isRead = (event.revents & POLLIN);
										// Получаем флаг доступности сокета на запись
										isWrite = (event.revents & POLLOUT);
										// Получаем флаг получения ошибки сокета
										isError = (event.revents & POLLERR);
										// Получаем флаг закрытия подключения
										isClose = (event.revents & POLLHUP);
										// Обнуляем количество событий
										event.revents = 0;
										// Если флаг на чтение данных из сокета установлен
										if(isRead){
											// Выполняем поиск указанной записи
											auto j = this->_peers.find(sock);
											// Если сокет в списке найден
											if(j != this->_peers.end()){
												// Получаем идентификатор события
												id = j->second.id;
												/**
												 * Определяем тип события к которому принадлежит сокет
												 */
												switch(static_cast <uint8_t> (j->second.type)){
													// Если событие принадлежит к таймеру
													case static_cast <uint8_t> (event_type_t::TIMER): {
														// Выполняем чтение входящего события
														const uint64_t event = this->_watch.event(sock);
														// Если чтение выполнено удачно
														if(event > 0){
															// Если функция обратного вызова установлена
															if(j->second.callback != nullptr){
																// Выполняем поиск события таймера присутствует в базе событий
																auto k = j->second.mode.find(event_type_t::TIMER);
																// Если событие найдено и оно активированно
																if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
																	// Выполняем функцию обратного вызова
																	std::apply(j->second.callback, std::make_tuple(sock, event_type_t::TIMER));
															}
															// Выполняем поиск указанной записи
															j = this->_peers.find(sock);
															// Если сокет в списке найден
															if((j != this->_peers.end()) && (id == j->second.id)){
																// Если таймер установлен как персистентный
																if(j->second.persist){
																	// Выполняем поиск события таймера присутствует в базе событий
																	auto k = j->second.mode.find(event_type_t::TIMER);
																	// Если событие найдено и оно активированно
																	if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
																		// Выполняем активацию таймера на указанное время
																		this->_watch.wait(j->second.sock, j->second.delay);
																}
															}
														// Удаляем сокет из базы событий
														} else this->del(j->second.id, j->second.sock);
													} break;
													// Если событие принадлежит к потоку
													case static_cast <uint8_t> (event_type_t::STREAM): {
														// Выполняем поиск верхнеуровневого потока
														auto i = this->_upstream.find(sock);
														// Если верхнеуровневый поток найден
														if(i != this->_upstream.end()){
															// Выполняем блокировку потока
															i->second->mtx.lock();
															// Выполняем чтение входящего события
															const uint64_t event = i->second->notifier.event();
															// Выполняем разблокировку потока
															i->second->mtx.unlock();
															// Выполняем поиск события межпотоковое присутствует в базе событий
															auto k = j->second.mode.find(event_type_t::STREAM);
															// Если событие найдено и оно активированно
															if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
																// Выполняем функцию обратного вызова
																this->stream(i->first, event);
														}
													} break;
													// Если это другое событие
													default: {
														// Если функция обратного вызова установлена
														if(j->second.callback != nullptr){
															// Выполняем поиск события на получение данных присутствует в базе событий
															auto k = j->second.mode.find(event_type_t::READ);
															// Если событие найдено и оно активированно
															if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
																// Выполняем функцию обратного вызова
																std::apply(j->second.callback, std::make_tuple(sock, event_type_t::READ));
														}
													}
												}
											}
										}
										// Если сокет доступен для записи
										if(isWrite){
											// Выполняем поиск указанной записи
											auto j = this->_peers.find(sock);
											// Если сокет в списке найден
											if((j != this->_peers.end()) && ((id == j->second.id) || (id == 0))){
												// Получаем идентификатор события
												id = j->second.id;
												// Если функция обратного вызова установлена
												if(j->second.callback != nullptr){
													// Выполняем поиск события на запись данных присутствует в базе событий
													auto k = j->second.mode.find(event_type_t::WRITE);
													// Если событие найдено и оно активированно
													if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
														// Выполняем функцию обратного вызова
														std::apply(j->second.callback, std::make_tuple(sock, event_type_t::WRITE));
												}
											}
										}
										// Если сокет отключился или произошла ошибка
										if(isClose || isError){
											// Если мы реально получили ошибку
											if(::WSAGetLastError() > 0){
												// Создаём буфер сообщения ошибки
												wchar_t message[256] = {0};
												// Выполняем формирование текста ошибки
												::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 256, 0);
												/**
												 * Если включён режим отладки
												 */
												#if DEBUG_MODE
													// Выводим сообщение об ошибке
													this->_log->debug(L"%s", __PRETTY_FUNCTION__, std::make_tuple(sock), log_t::flag_t::CRITICAL, message);
												/**
												* Если режим отладки не включён
												*/
												#else
													// Выводим сообщение об ошибке
													this->_log->print(L"%s", log_t::flag_t::CRITICAL, message);
												#endif
											}
											// Выполняем поиск указанной записи
											auto j = this->_peers.find(sock);
											// Если сокет в списке найден
											if(j != this->_peers.end()){
												// Если идентификаторы соответствуют
												if((id == j->second.id) || (id == 0)){
													// Получаем идентификатор события
													id = j->second.id;
													// Если функция обратного вызова установлена
													if(j->second.callback != nullptr){
														// Получаем функцию обратного вызова
														auto callback = std::bind(j->second.callback, sock, event_type_t::CLOSE);
														// Выполняем поиск события на отключение присутствует в базе событий
														auto k = j->second.mode.find(event_type_t::CLOSE);
														// Если событие найдено и оно активированно
														if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED)){
															// Удаляем сокет из базы событий
															this->del(j->second.id, sock);
															// Выполняем функцию обратного вызова
															std::apply(callback, std::make_tuple());
															// Продолжаем обход дальше
															continue;
														}
													}
													// Удаляем сокет из базы событий
													this->del(j->second.id, sock);
												}
											// Выполняем удаление фантомного файлового дескриптора
											} else this->del(sock);
										}
									// Выходим из цикла
									} else break;
								}
							}
							// Если активирован простой режим работы чтения базы событий
							if(this->_easily){
								// Если время установленно
								if(this->_rate > 0)
									// Выполняем задержку времени на указанное количество времени
									std::this_thread::sleep_for(chrono::milliseconds(this->_rate));
								// Устанавливаем задержку времени по умолчанию
								else std::this_thread::sleep_for(10ms);
								// Продолжаем опрос дальше
								continue;
							// Если опрос базы событий не заблокирован
							} else if(!this->_locker)
								// Продолжаем опрос дальше
								continue;
						}
					}
					// Замораживаем поток на период времени частоты обновления базы событий
					std::this_thread::sleep_for(100ms);
				/**
				 * Для операционной системы Sun Solaris
				 */
				#elif __sun__
					// Если опрос базы событий не заблокирован
					if(!this->_locker){
						// Если в списке достаточно событий для опроса
						if(!this->_events.empty()){
							// Устанавливаем список опрашиваемых сокетов
							this->_dopoll.dp_fds = this->_events.data();
							// Устанавливаем количество сокетов для опроса
							this->_dopoll.dp_nfds = this->_events.size();
							// Устанавливаем таймаут ожидания получения события
							this->_dopoll.dp_timeout = (!this->_easily ? static_cast <int32_t> (this->_rate) : -1);
							// Выполняем запуск ожидания входящих событий сокетов
							poll = ::ioctl(this->_wfd, DP_POLL, &this->_dopoll);
							// Если мы получили ошибку
							if(poll < 0){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
								#endif
							// Если сработал таймаут
							} else if(poll == 0)
								// Компенсируем условие
								poll = 0;
							// Если опрос прошёл успешно
							else {
								// Идентификатор события
								uint64_t id = 0;
								// Файловый дескриптор события
								SOCKET sock = INVALID_SOCKET;
								// Флаги статусов полученного сокета
								bool isRead = false, isWrite = false, isClose = false, isError = false;
								// Выполняем перебор всех файловых дескрипторов
								for(int32_t i = 0; i < poll; i++){
									// Если записей достаточно в списке
									if(i < this->_dopoll.dp_nfds){
										// Зануляем идентификатор события
										id = 0;
										// Получаем объект файлового дескриптора
										auto & event = this->_dopoll.dp_fds[i];
										// Получаем сокет
										sock = event.fd;
										// Получаем флаг достуности чтения из сокета
										isRead = (event.revents & POLLIN);
										// Получаем флаг доступности сокета на запись
										isWrite = (event.revents & POLLOUT);
										// Получаем флаг получения ошибки сокета
										isError = (event.revents & POLLERR);
										// Получаем флаг закрытия подключения
										isClose = (event.revents & POLLHUP);
										// Обнуляем количество событий
										event.revents = 0;
										// Если флаг на чтение данных из сокета установлен
										if(isRead){
											// Выполняем поиск указанной записи
											auto j = this->_peers.find(sock);
											// Если сокет в списке найден
											if(j != this->_peers.end()){
												// Получаем идентификатор события
												id = j->second.id;
												/**
												 * Определяем тип события к которому принадлежит сокет
												 */
												switch(static_cast <uint8_t> (j->second.type)){
													// Если событие принадлежит к таймеру
													case static_cast <uint8_t> (event_type_t::TIMER): {
														// Выполняем чтение входящего события
														const uint64_t event = this->_watch.event(sock);
														// Если чтение выполнено удачно
														if(event > 0){
															// Если функция обратного вызова установлена
															if(j->second.callback != nullptr){
																// Выполняем поиск события таймера присутствует в базе событий
																auto k = j->second.mode.find(event_type_t::TIMER);
																// Если событие найдено и оно активированно
																if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
																	// Выполняем функцию обратного вызова
																	std::apply(j->second.callback, std::make_tuple(sock, event_type_t::TIMER));
															}
															// Выполняем поиск указанной записи
															j = this->_peers.find(sock);
															// Если сокет в списке найден
															if((j != this->_peers.end()) && (id == j->second.id)){
																// Если таймер установлен как персистентный
																if(j->second.persist){
																	// Выполняем поиск события таймера присутствует в базе событий
																	auto k = j->second.mode.find(event_type_t::TIMER);
																	// Если событие найдено и оно активированно
																	if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
																		// Выполняем активацию таймера на указанное время
																		this->_watch.wait(j->second.sock, j->second.delay);
																}
															}
														// Удаляем сокет из базы событий
														} else this->del(j->second.id, j->second.sock);
													} break;
													// Если событие принадлежит к потоку
													case static_cast <uint8_t> (event_type_t::STREAM): {
														// Выполняем поиск верхнеуровневого потока
														auto i = this->_upstream.find(sock);
														// Если верхнеуровневый поток найден
														if(i != this->_upstream.end()){
															// Выполняем блокировку потока
															i->second->mtx.lock();
															// Выполняем чтение входящего события
															const uint64_t event = i->second->notifier.event();
															// Выполняем разблокировку потока
															i->second->mtx.unlock();
															// Выполняем поиск события межпотоковое присутствует в базе событий
															auto k = j->second.mode.find(event_type_t::STREAM);
															// Если событие найдено и оно активированно
															if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
																// Выполняем функцию обратного вызова
																this->stream(i->first, event);
														}
													} break;
													// Если это другое событие
													default: {
														// Если функция обратного вызова установлена
														if(j->second.callback != nullptr){
															// Выполняем поиск события на получение данных присутствует в базе событий
															auto k = j->second.mode.find(event_type_t::READ);
															// Если событие найдено и оно активированно
															if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
																// Выполняем функцию обратного вызова
																std::apply(j->second.callback, std::make_tuple(sock, event_type_t::READ));
														}
													}
												}
											}
										}
										// Если сокет доступен для записи
										if(isWrite){
											// Выполняем поиск указанной записи
											auto j = this->_peers.find(sock);
											// Если сокет в списке найден
											if((j != this->_peers.end()) && ((id == j->second.id) || (id == 0))){
												// Получаем идентификатор события
												id = j->second.id;
												// Если функция обратного вызова установлена
												if(j->second.callback != nullptr){
													// Выполняем поиск события на запись данных присутствует в базе событий
													auto k = j->second.mode.find(event_type_t::WRITE);
													// Если событие найдено и оно активированно
													if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
														// Выполняем функцию обратного вызова
														std::apply(j->second.callback, std::make_tuple(sock, event_type_t::WRITE));
												}
											}
										}
										// Если сокет отключился или произошла ошибка
										if(isClose || isError){
											// Если была вызвана ошибка
											if(isError){
												/**
												 * Если включён режим отладки
												 */
												#if DEBUG_MODE
													// Выводим сообщение об ошибке
													this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock), log_t::flag_t::CRITICAL, ::strerror(errno));
												/**
												* Если режим отладки не включён
												*/
												#else
													// Выводим сообщение об ошибке
													this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
												#endif
											}
											// Выполняем поиск указанной записи
											auto j = this->_peers.find(sock);
											// Если сокет в списке найден
											if(j != this->_peers.end()){
												// Если идентификаторы соответствуют
												if((id == j->second.id) || (id == 0)){
													// Получаем идентификатор события
													id = j->second.id;
													// Если функция обратного вызова установлена
													if(j->second.callback != nullptr){
														// Получаем функцию обратного вызова
														auto callback = std::bind(j->second.callback, sock, event_type_t::CLOSE);
														// Выполняем поиск события на отключение присутствует в базе событий
														auto k = j->second.mode.find(event_type_t::CLOSE);
														// Если событие найдено и оно активированно
														if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED)){
															// Удаляем сокет из базы событий
															this->del(j->second.id, sock);
															// Выполняем функцию обратного вызова
															std::apply(callback, std::make_tuple());
															// Продолжаем обход дальше
															continue;
														}
													}
													// Удаляем сокет из базы событий
													this->del(j->second.id, sock);
												}
											// Выполняем удаление фантомного файлового дескриптора
											} else this->del(sock);
										}
									// Выходим из цикла
									} else break;
								}
							}
							// Если активирован простой режим работы чтения базы событий
							if(this->_easily){
								// Если время установленно
								if(this->_rate > 0)
									// Выполняем задержку времени на указанное количество времени
									std::this_thread::sleep_for(chrono::milliseconds(this->_rate));
								// Устанавливаем задержку времени по умолчанию
								else std::this_thread::sleep_for(10ms);
								// Продолжаем опрос дальше
								continue;
							// Если опрос базы событий не заблокирован
							} else if(!this->_locker)
								// Продолжаем опрос дальше
								continue;
						}
					}
					// Замораживаем поток на период времени частоты обновления базы событий
					std::this_thread::sleep_for(100ms);
				/**
				 * Для операционной системы Linux
				 */
				#elif __linux__
					// Если опрос базы событий не заблокирован
					if(!this->_locker){
						// Если в списке достаточно событий для опроса
						if(!this->_change.empty()){
							// Если буфер результатов опроса меньше списка отслеживаемых событий
							if(this->_events.size() < this->_change.size())
								// Выполняем увеличение буфера результатов опроса (вне обхода буфера это безопасно)
								this->_events.resize(this->_change.size());
							// Сбрасываем количество действительных записей в буфере результатов
							this->_ready = 0;
							// Выполняем запуск ожидания входящих событий сокетов (размер буфера передаём как предел событий)
							poll = ::epoll_wait(this->_efd, this->_events.data(), static_cast <int32_t> (this->_events.size()), (!this->_easily ? static_cast <int32_t> (this->_rate) : 0));
							// Запоминаем количество действительных записей в буфере результатов
							this->_ready = (poll > 0 ? static_cast <size_t> (poll) : 0);
							// Если мы получили ошибку
							if(poll == INVALID_SOCKET){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
								#endif
							// Если сработал таймаут
							} else if(poll == 0)
								// Компенсируем условие
								poll = 0;
							// Если опрос прошёл успешно
							else {
								// Идентификатор события
								uint64_t id = 0;
								// Файловый дескриптор события
								SOCKET sock = INVALID_SOCKET;
								// Флаги статусов полученного сокета
								bool isRead = false, isWrite = false, isClose = false, isError = false;
								// Выполняем перебор всех событий в которых мы получили изменения
								for(int32_t i = 0; i < poll; i++){
									// Если записей достаточно в списке
									if(static_cast <size_t> (i) < this->_events.size()){
										// Получаем объект файлового дескриптора
										const auto & event = this->_events.at(i);
										// Получаем флаг достуности чтения из сокета
										isRead = (event.events & EPOLLIN);
										// Получаем флаг доступности сокета на запись
										isWrite = (event.events & EPOLLOUT);
										// Получаем флаг получения ошибки сокета
										isError = (event.events & EPOLLERR);
										// Получаем флаг закрытия подключения
										isClose = (event.events & (EPOLLRDHUP | EPOLLHUP));
										// Если сработал уведомитель пробуждения опроса
										if(event.data.ptr == &this->_wake){
											// Идентификатор события пробуждения
											uint64_t wake = 0;
											// Извлекаем все события пробуждения
											while(this->_wake.event(wake));
											// Продолжаем обход дальше
											continue;
										}
										// Получаем объект текущего события
										peer_t * item = reinterpret_cast <peer_t *> (event.data.ptr);
										// Если объект текущего события получен
										if(item != nullptr){
											// Получаем идентификатор события
											id = item->id;
											// Получаем значение текущего идентификатора
											sock = item->sock;
											// Если в сокете появились данные для чтения
											if(isRead){
												/**
												 * Определяем тип события к которому принадлежит сокет
												 */
												switch(static_cast <uint8_t> (item->type)){
													// Если событие принадлежит к таймеру
													case static_cast <uint8_t> (event_type_t::TIMER): {
														// Идентификатор события таймера
														uint64_t event = 0;
														// Флаг срабатывания таймера
														bool fired = false;
														/**
														 * Извлекаем все накопившиеся срабатывания: уведомитель отслеживается по фронту,
														 * и оставшееся срабатывание ждало бы следующего. Несколько срабатываний
														 * сливаются в одно. Пробуждение без срабатывания таймер больше не удаляет:
														 * раньше ложное пробуждение (чтение вернуло 0) уничтожало рабочий таймер
														 */
														while(this->_watch.event(sock, event))
															// Запоминаем, что таймер сработал
															fired = true;
														// Если таймер сработал
														if(fired){
															// Если функция обратного вызова установлена
															if(item->callback != nullptr){
																// Выполняем поиск события таймера присутствует в базе событий
																auto j = item->mode.find(event_type_t::TIMER);
																// Если событие найдено и оно активированно
																if((j != item->mode.end()) && (j->second == event_mode_t::ENABLED))
																	// Выполняем функцию обратного вызова
																	std::apply(item->callback, std::make_tuple(item->sock, event_type_t::TIMER));
															}
															// Выполняем поиск файлового дескриптора в базе событий
															auto j = this->_peers.find(sock);
															// Если сокет есть в базе событий
															if((j != this->_peers.end()) && (id == j->second.id)){
																// Если таймер установлен как персистентный
																if(j->second.persist){
																	// Выполняем поиск события таймера присутствует в базе событий
																	auto k = j->second.mode.find(event_type_t::TIMER);
																	// Если событие найдено и оно активированно
																	if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
																		// Выполняем активацию таймера на указанное время
																		this->_watch.wait(j->second.sock, j->second.delay);
																}
															}
														}
													} break;
													// Если событие принадлежит к потоку
													case static_cast <uint8_t> (event_type_t::STREAM):
														// Выполняем извлечение всех накопившихся межпотоковых событий
														this->streams(id, sock);
													break;
													// Если это другое событие
													default: {
														// Если функция обратного вызова установлена
														if(item->callback != nullptr){
															// Выполняем поиск события на получение данных присутствует в базе событий
															auto j = item->mode.find(event_type_t::READ);
															// Если событие найдено и оно активированно
															if((j != item->mode.end()) && (j->second == event_mode_t::ENABLED))
																// Выполняем функцию обратного вызова
																std::apply(item->callback, std::make_tuple(item->sock, event_type_t::READ));
														}
													}
												}
											}
											// Если сокет доступен для записи
											if(isWrite){
												// Выполняем поиск файлового дескриптора в базе событий
												auto i = this->_peers.find(sock);
												// Если сокет есть в базе событий
												if((i != this->_peers.end()) && (id == i->second.id)){
													// Если функция обратного вызова установлена
													if(i->second.callback != nullptr){
														// Выполняем поиск события на запись данных присутствует в базе событий
														auto j = i->second.mode.find(event_type_t::WRITE);
														// Если событие найдено и оно активированно
														if((j != i->second.mode.end()) && (j->second == event_mode_t::ENABLED))
															// Выполняем функцию обратного вызова
															std::apply(i->second.callback, std::make_tuple(i->second.sock, event_type_t::WRITE));
													}
												}
											}
											// Если сокет отключился или произошла ошибка
											if(isClose || isError){
												// Если была вызвана ошибка
												if(isError){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock), log_t::flag_t::CRITICAL, ::strerror(errno));
													/**
													* Если режим отладки не включён
													*/
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
													#endif
												}
												// Выполняем поиск файлового дескриптора в базе событий
												auto i = this->_peers.find(sock);
												// Если сокет есть в базе событий
												if(i != this->_peers.end()){
													// Если идентификаторы соответствуют
													if(id == i->second.id){
														// Если функция обратного вызова установлена
														if(i->second.callback != nullptr){
															// Получаем функцию обратного вызова
															auto callback = std::bind(i->second.callback, i->second.sock, event_type_t::CLOSE);
															// Выполняем поиск события на отключение присутствует в базе событий
															auto j = i->second.mode.find(event_type_t::CLOSE);
															// Если событие найдено и оно активированно
															if((j != i->second.mode.end()) && (j->second == event_mode_t::ENABLED)){
																// Удаляем сокет из базы событий
																this->del(i->second.id, i->second.sock);
																// Выполняем функцию обратного вызова
																std::apply(callback, std::make_tuple());
																// Продолжаем обход дальше
																continue;
															}
														}
														// Удаляем сокет из базы событий
														this->del(i->second.id, i->second.sock);
													}
												// Выполняем удаление фантомного файлового дескриптора
												} else this->del(sock);
											}
										}
									// Выходим из цикла
									} else break;
								}
							}
							// Если активирован простой режим работы чтения базы событий
							if(this->_easily){
								// Если время установленно
								if(this->_rate > 0)
									// Выполняем задержку времени на указанное количество времени
									std::this_thread::sleep_for(chrono::milliseconds(this->_rate));
								// Устанавливаем задержку времени по умолчанию
								else std::this_thread::sleep_for(10ms);
								// Продолжаем опрос дальше
								continue;
							// Если опрос базы событий не заблокирован
							} else if(!this->_locker)
								// Продолжаем опрос дальше
								continue;
						}
					}
					// Замораживаем поток на период времени частоты обновления базы событий
					std::this_thread::sleep_for(100ms);
				/**
				 * Для операционной системы FreeBSD, NetBSD, OpenBSD или MacOS X
				 */
				#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
					// Если опрос базы событий не заблокирован
					if(!this->_locker){
						// Если в списке достаточно событий для опроса
						if(!this->_change.empty()){
							/**
							 * Буфер результатов не меньше списка изменений: тогда в нём хватает места и для
							 * ошибок применения изменений (EV_ERROR), и kevent не отказывает целиком
							 */
							if(this->_events.size() < this->_change.size())
								// Выполняем увеличение буфера результатов опроса (вне обхода буфера это безопасно)
								this->_events.resize(this->_change.size());
							// Сбрасываем количество действительных записей в буфере результатов
							this->_ready = 0;
							// Выполняем запуск ожидания входящих событий сокетов
							poll = ::kevent(this->_kq, this->_change.data(), this->_change.size(), this->_events.data(), this->_events.size(), ((this->_rate > -1) || this->_easily ? &baseDelay : nullptr));
							// Запоминаем количество действительных записей в буфере результатов
							this->_ready = (poll > 0 ? static_cast <size_t> (poll) : 0);
							// Если мы получили ошибку
							if(poll == INVALID_SOCKET){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
								#endif
							// Если сработал таймаут
							} else if(poll == 0)
								// Компенсируем условие
								poll = 0;
							// Если опрос прошёл успешно
							else {
								// Идентификатор события
								uint64_t id = 0;
								// Код ошибки полученный от ядра
								int32_t code = 0;
								// Файловый дескриптор события
								SOCKET sock = INVALID_SOCKET;
								// Флаги статусов полученного сокета
								bool isRead = false, isWrite = false,
								     isClose = false, isError = false, isEvent = false;
								// Выполняем перебор всех событий в которых мы получили изменения
								for(int32_t i = 0; i < poll; i++){
									// Если записей достаточно в списке
									if(static_cast <size_t> (i) < this->_events.size()){
										// Получаем объект файлового дескриптора
										const auto & event = this->_events.at(i);
										// Если запись исключена из обхода (её сокет удалён во время обхода)
										if(event.filter == 0)
											// Пропускаем запись
											continue;
										// Если сработал уведомитель пробуждения опроса
										if(event.ident == static_cast <uintptr_t> (this->_wakeup)){
											// Идентификатор события пробуждения
											uint64_t wake = 0;
											// Извлекаем все события пробуждения
											while(this->_wake.event(wake));
											// Продолжаем обход дальше
											continue;
										}
										// Получаем код ошибки переданный ядром
										code = static_cast <int32_t> (event.data);
										// Получаем флаг закрытия подключения
										isClose = (event.flags & EV_EOF);
										// Получаем флаг получения ошибки сокета
										isError = (event.flags & EV_ERROR);
										/**
										 * Фильтры kqueue являются малыми отрицательными числами, а не битами,
										 * поэтому сравниваем на равенство. Проверка по маске давала истину
										 * для любого фильтра, и чтение с записью срабатывали одновременно
										 */
										isRead = (event.filter == EVFILT_READ);
										// Получаем флаг доступности сокета на запись
										isWrite = (event.filter == EVFILT_WRITE);
										// Получаем флаг нашего кастомного события
										isEvent = (event.filter == EVFILT_USER);
										// Выполняем поиск файлового дескриптора в базе событий
										auto j = this->_peers.find(event.ident);
										// Если сокет есть в базе событий
										if(j != this->_peers.end()){
											// Получаем объект текущего события
											peer_t * item = &j->second;
											// Получаем идентификатор события
											id = item->id;
											// Получаем значение текущего идентификатора
											sock = item->sock;
											// Если в сокете появились данные для чтения или пользовательское событие
											if(isRead || isEvent){
												/**
												 * Определяем тип события к которому принадлежит сокет
												 */
												switch(static_cast <uint8_t> (item->type)){
													// Если событие принадлежит к таймеру
													case static_cast <uint8_t> (event_type_t::TIMER): {
														// Идентификатор события таймера
														uint64_t event = 0;
														// Флаг срабатывания таймера
														bool fired = false;
														/**
														 * Извлекаем все накопившиеся срабатывания: уведомитель отслеживается по фронту,
														 * и оставшееся срабатывание ждало бы следующего. Несколько срабатываний
														 * сливаются в одно. Пробуждение без срабатывания таймер больше не удаляет:
														 * раньше ложное пробуждение (чтение вернуло 0) уничтожало рабочий таймер
														 */
														while(this->_watch.event(sock, event))
															// Запоминаем, что таймер сработал
															fired = true;
														// Если таймер сработал
														if(fired){
															// Если функция обратного вызова установлена
															if(item->callback != nullptr){
																// Выполняем поиск события таймера присутствует в базе событий
																auto k = item->mode.find(event_type_t::TIMER);
																// Если событие найдено и оно активированно
																if((k != item->mode.end()) && (k->second == event_mode_t::ENABLED))
																	// Выполняем функцию обратного вызова
																	std::apply(item->callback, std::make_tuple(item->sock, event_type_t::TIMER));
															}
															// Выполняем поиск файлового дескриптора в базе событий
															j = this->_peers.find(sock);
															// Если сокет есть в базе событий
															if((j != this->_peers.end()) && (id == j->second.id)){
																// Если таймер установлен как персистентный
																if(j->second.persist){
																	// Выполняем поиск события таймера присутствует в базе событий
																	auto k = j->second.mode.find(event_type_t::TIMER);
																	// Если событие найдено и оно активированно
																	if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
																		// Выполняем активацию таймера на указанное время
																		this->_watch.wait(j->second.sock, j->second.delay);
																}
															}
														}
													} break;
													// Если событие принадлежит к потоку
													case static_cast <uint8_t> (event_type_t::STREAM):
														// Выполняем извлечение всех накопившихся межпотоковых событий
														this->streams(id, sock);
													break;
													// Если это другое событие
													default: {
														// Если функция обратного вызова установлена
														if(item->callback != nullptr){
															// Выполняем поиск события на получение данных присутствует в базе событий
															auto k = item->mode.find(event_type_t::READ);
															// Если событие найдено и оно активированно
															if((k != item->mode.end()) && (k->second == event_mode_t::ENABLED))
																// Выполняем функцию обратного вызова
																std::apply(item->callback, std::make_tuple(sock, event_type_t::READ));
														}
													}
												}
											}
											// Если сокет доступен для записи
											if(isWrite){
												// Выполняем поиск файлового дескриптора в базе событий
												j = this->_peers.find(sock);
												// Если сокет есть в базе событий
												if((j != this->_peers.end()) && (id == j->second.id)){
													// Если функция обратного вызова установлена
													if(j->second.callback != nullptr){
														// Выполняем поиск события на запись данных присутствует в базе событий
														auto k = j->second.mode.find(event_type_t::WRITE);
														// Если событие найдено и оно активированно
														if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
															// Выполняем функцию обратного вызова
															std::apply(j->second.callback, std::make_tuple(j->second.sock, event_type_t::WRITE));
													}
												}
											}
											// Если сокет отключился или произошла ошибка
											if(isClose || isError){
												// Если была вызвана ошибка
												if(isError){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock), log_t::flag_t::CRITICAL, ::strerror(code));
													/**
													* Если режим отладки не включён
													*/
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(code));
													#endif
												}
												// Выполняем поиск файлового дескриптора в базе событий
												j = this->_peers.find(sock);
												// Если сокет есть в базе событий
												if(j != this->_peers.end()){
													// Если идентификаторы соответствуют
													if(id == j->second.id){
														// Если функция обратного вызова установлена
														if(j->second.callback != nullptr){
															// Получаем функцию обратного вызова
															auto callback = std::bind(j->second.callback, j->second.sock, event_type_t::CLOSE);
															// Выполняем поиск события на отключение присутствует в базе событий
															auto k = j->second.mode.find(event_type_t::CLOSE);
															// Если событие найдено и оно активированно
															if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED)){
																// Удаляем сокет из базы событий
																this->del(j->second.id, j->second.sock);
																// Выполняем функцию обратного вызова
																std::apply(callback, std::make_tuple());
																// Продолжаем обход дальше
																continue;
															}
														}
														// Удаляем сокет из базы событий
														this->del(j->second.id, j->second.sock);
													}
												// Выполняем удаление фантомного файлового дескриптора
												} else this->del(sock);
											}
										}
									// Выходим из цикла
									} else break;
								}
							}
							// Если активирован простой режим работы чтения базы событий
							if(this->_easily){
								// Если время установленно
								if(this->_rate > 0)
									// Выполняем задержку времени на указанное количество времени
									std::this_thread::sleep_for(chrono::milliseconds(this->_rate));
								// Устанавливаем задержку времени по умолчанию
								else std::this_thread::sleep_for(10ms);
								// Продолжаем опрос дальше
								continue;
							// Если опрос базы событий не заблокирован
							} else if(!this->_locker)
								// Продолжаем опрос дальше
								continue;
						}
					}
					// Замораживаем поток на период времени частоты обновления базы событий
					std::this_thread::sleep_for(100ms);
				#endif
			}
			// Если очистка базы событий отложена остановкой из другого потока
			if(this->_defer.exchange(false)){
				// Выполняем очистку списка событий
				this->clear();
				// Выполняем деинициализацию базы событий
				this->init(event_mode_t::DISABLED);
				// Выполняем инициализацию базы событий
				this->init(event_mode_t::ENABLED);
				// Идентификатор события пробуждения
				uint64_t wake = 0;
				// Извлекаем оставшиеся события пробуждения
				while(this->_wake.event(wake));
			}
			// Останавливаем работу таймеров скрина
			this->_watch.stop();
			// Снимаем флаг запущенного опроса базы событий
			this->_launched = static_cast <bool> (this->_works);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если не происходит отключение работы базы событий
			if(this->_works){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Снимаем флаг запущенного опроса базы событий
			} else this->_launched = static_cast <bool> (this->_works);
		}
	}
}
/**
 * @brief Метод пересоздания базы событий
 *
 */
void awh::Base::rebase() noexcept {
	// Если метод запущен в дочернем потоке
	if(this->isChildThread())
		// Выводим сообщение об ошибке
		this->_log->print("Method \"rebase\" cannot be called in a child thread", log_t::flag_t::WARNING);
	// Если запуск производится в основном потоке
	else {
		/**
		 * Выполняем обработку ошибки
		 */
		try {
			// Если работа базы событий запущена
			if(this->_works){
				// Снимаем флаг работы базы событий
				this->_works = !this->_works;
				/**
				 * Пересоздаём объект ядра и регистрируем в нём тех же участников. Раньше база
				 * событий останавливалась с очисткой, а участники добавлялись заново без задержки
				 * таймера: таймеры превращались в обычные события на закрытых дескрипторах
				 */
				this->recreate();
				// Выполняем запуск работы базы событий
				this->start();
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод заморозки чтения данных
 *
 * @param mode флаг активации
 */
void awh::Base::freeze(const bool mode) noexcept {
	// Выполняем активацию блокировки
	this->_locker = mode;
}
/**
 * @brief Метод активации простого режима чтения базы событий
 *
 * @param mode флаг активации
 */
void awh::Base::easily(const bool mode) noexcept {
	// Выполняем установку флага активации простого режима чтения базы событий
	this->_easily = mode;
	// Если активирован простой режим работы чтения базы событий
	if(!this->_easily)
		// Выполняем сброс времени ожидания
		this->_rate = -1;
}
/**
 * @brief Метод установки времени блокировки базы событий в ожидании событий
 *
 * @param msec время ожидания событий в миллисекундах
 */
void awh::Base::rate(const uint32_t msec) noexcept {
	// Если количество миллисекунд передано верно
	if(msec > 0)
		// Выполняем установку времени ожидания
		this->_rate = static_cast <int32_t> (msec);
	// Выполняем сброс времени ожидания
	else this->_rate = -1;
}
/**
 * @brief Метод отправки сообщения между потоками
 *
 * @param sock сокет межпотокового передатчика
 * @param tid  идентификатор трансферной передачи
 */
void awh::Base::upstream(const SOCKET sock, const uint64_t tid) noexcept {
	// Если метод запущен в основном потоке
	if(!this->isChildThread())
		// Выводим сообщение об ошибке
		this->_log->print("Method \"%s\" cannot be called in a main thread", log_t::flag_t::WARNING, __FUNCTION__);
	// Если запуск производится в основном потоке
	else {
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * Выполняем блокировку списка передатчиков: метод вызывается из дочернего потока,
			 * а активация и деактивация меняют список под этим же мютексом
			 */
			const lock_guard <std::recursive_mutex> guard(this->_mtx);
			// Выполняем поиск указанного межпотокового передатчика
			auto i = this->_upstream.find(sock);
			// Если межпотоковый передатчик обнаружен
			if(i != this->_upstream.end()){
				// Выполняем блокировку потока
				const lock_guard <std::mutex> lock(i->second->mtx);
				// Выполняем отправку родительскому потоку сообщение
				i->second->notifier.notify(tid);
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, tid), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод деактивации межпотокового передатчика
 *
 * @param sock сокет межпотокового передатчика
 */
void awh::Base::deactivationUpstream(const SOCKET sock) noexcept {
	// Если метод запущен в дочернем потоке
	if(this->isChildThread())
		// Выводим сообщение об ошибке
		this->_log->print("Method \"%s\" cannot be called in a child thread", log_t::flag_t::WARNING, __FUNCTION__);
	// Если запуск производится в основном потоке
	else {
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем поиск указанного межпотокового передатчика
			auto i = this->_upstream.find(sock);
			// Если межпотоковый передатчик обнаружен
			if(i != this->_upstream.end()){
				// Выполняем блокировку потока
				const lock_guard <std::recursive_mutex> lock(this->_mtx);
				// Выполняем удаление события сокета из базы событий
				if(!this->del(static_cast <uint64_t> (i->first), i->first))
					// Выводим сообщение что событие не вышло активировать
					this->_log->print("Failed remove upstream event for SOCKET=%d", log_t::flag_t::WARNING, i->first);
				// Выполняем удаление верхнеуровневого потока из списка
				this->_upstream.erase(i);
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод активации межпотокового передатчика
 *
 * @param callback функция обратного вызова
 * @return         сокет межпотокового передатчика
 */
SOCKET awh::Base::activationUpstream(function <void (const uint64_t)> callback) noexcept {
	// Результат работы функции
	SOCKET result = INVALID_SOCKET;
	// Если метод запущен в дочернем потоке
	if(this->isChildThread())
		// Выводим сообщение об ошибке
		this->_log->print("Method \"%s\" cannot be called in a child thread", log_t::flag_t::WARNING, __FUNCTION__);
	// Если запуск производится в основном потоке
	else {
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Создаём объект межпотокового передатчика
			auto upstream = std::make_unique <upstream_t> (this->_fmk, this->_log);
			// Выполняем установку функции обратного вызова
			upstream->callback = callback;
			// Выполняем инициализацию уведомителя
			const SOCKET sock = upstream->notifier.init();
			// Если уведомитель инициализирован правильно
			if(sock != INVALID_SOCKET){
				// Выполняем блокировку потока
				const lock_guard <std::recursive_mutex> lock(this->_mtx);
				// Устанавливаем результат
				result = sock;
				// Выполняем перенос нашего уведомителя в список уведомителей
				if(this->_upstream.emplace(result, ::move(upstream)).first->first){
					// Выполняем добавление события в базу событий
					if(!this->add(static_cast <uint64_t> (result), result))
						// Выводим сообщение что событие не вышло активировать
						this->_log->print("Failed activate upstream event for SOCKET=%d", log_t::flag_t::WARNING, result);
					// Если событие в базу событий успешно добавленно, активируем событие верхнеуровневого потока
					else if(!this->mode(static_cast <uint64_t> (result), result, event_type_t::STREAM, event_mode_t::ENABLED))
						// Выводим сообщение что событие не вышло активировать
						this->_log->print("Failed enabled read upstream event for SOCKET=%d", log_t::flag_t::WARNING, result);
				// Выводим сообщение, что такой сокет уже существует
				} else this->_log->print("Failed create upstream event for SOCKET=%d", log_t::flag_t::WARNING, result);
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const bad_alloc &) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, "Memory allocation error");
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
			#endif
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Base::Base(const fmk_t * fmk, const log_t * log) noexcept :
 _wid(0), _rate(-1),
 _works(false), _easily(false),
 _locker(false), _launched(false),
 _fds(log), _watch(fmk, log), _defer(false),
 _wakeup(INVALID_SOCKET), _wake(fmk, log), _fmk(fmk), _log(log) {
	// Получаем идентификатор потока
	this->_wid = this->wid();
	/**
	 * Для операционной системы Linux, MacOS X, FreeBSD, NetBSD или OpenBSD
	 */
	#if __linux__ || __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
		// Сбрасываем количество действительных записей в буфере результатов
		this->_ready = 0;
	#endif
	/**
	 * Для операционной системы Linux, MacOS X, FreeBSD, NetBSD или OpenBSD
	 */
	#if __linux__ || __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
		// Выполняем инициализацию уведомителя пробуждения опроса
		this->_wakeup = this->_wake.init();
	#endif
	// Выполняем инициализацию базы событий
	this->init(event_mode_t::ENABLED);
	// Выполняем настройку сетевых параметров
	this->boostingNetwork();
}
/**
 * @brief Деструктор
 *
 */
awh::Base::~Base() noexcept {
	// Выполняем деинициализацию базы событий
	this->init(event_mode_t::DISABLED);
}
