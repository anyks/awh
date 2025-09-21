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
		#if AWH_BOOSTING_OS
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
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, "Operating system kernel settings, accessible only by the superuser");
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, "Operating system kernel settings, accessible only by the superuser");
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
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, "Operating system kernel settings, accessible only by the superuser");
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, "Operating system kernel settings, accessible only by the superuser");
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
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, "Operating system kernel settings, accessible only by the superuser");
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, "Operating system kernel settings, accessible only by the superuser");
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
						this->_os.sysctl("kernel.core_pattern", string{"/tmp/%e-%p.core"});
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

					cout << " **************** " << net.core.wmem_max << " == " << 16777216 << endl;

					// Разрешаем масштабирование TCP-окна
					this->_os.sysctl("net.ipv4.tcp_window_scaling", 1);
					// Запрещаем сохранять результаты измерений TCP-соединения в кэше при его закрытии
					this->_os.sysctl("net.ipv4.tcp_no_metrics_save", 1);
					// Включаем автоматическую настройку размера приёмного буфера TCP
					this->_os.sysctl("net.ipv4.tcp_moderate_rcvbuf", 1);
					// Определяем максимальное количество входящих пакетов
					this->_os.sysctl("net.core.netdev_max_backlog", 2500);
					// Увеличиваем лимит автонастройки TCP-буфера Linux до 64 МБ
					this->_os.sysctl("net.ipv4.tcp_rmem", string{"\"4096 87380 16777216\""});
					this->_os.sysctl("net.ipv4.tcp_wmem", string{"\"4096 65536 16777216\""});
					// Рекомендуется для хостов с включенными большими фреймами
					this->_os.sysctl("net.ipv4.tcp_mtu_probing", 1);
					// Рекомендуется для хостов CentOS 7/Debian 8
					this->_os.sysctl("net.core.default_qdisc", string{"fq"});
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
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, "Operating system kernel settings, accessible only by the superuser");
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, "Operating system kernel settings, accessible only by the superuser");
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
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, "Operating system kernel settings, accessible only by the superuser");
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, "Operating system kernel settings, accessible only by the superuser");
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
			// Флаг удалённого события из базы событий
			bool erased = false;
			// Выполняем блокировку чтения базы событий
			this->_locker = true;
			// Выполняем поиск файлового дескриптора из списка событий
			for(auto i = this->_events.begin(); i != this->_events.end(); ++i){
				// Если сокет найден
				if((i->data.ptr != nullptr) && (reinterpret_cast <peer_t *> (i->data.ptr)->sock == sock)){
					// Выполняем изменение параметров события
					result = erased = (::epoll_ctl(this->_efd, EPOLL_CTL_DEL, sock, &(* i)) == 0);
					// Если событие принадлежит к таймеру
					if(reinterpret_cast <peer_t *> (i->data.ptr)->type == event_type_t::TIMER)
						// Выполняем удаление таймера
						this->_watch.away(reinterpret_cast <peer_t *> (i->data.ptr)->sock);
					// Выполняем удаление события из списка отслеживания
					this->_events.erase(i);
					// Выходим из цикла
					break;
				}
			}
			// Выполняем поиск файлового дескриптора из списка изменений
			for(auto i = this->_change.begin(); i != this->_change.end(); ++i){
				// Если сокет найден
				if((i->data.ptr != nullptr) && (reinterpret_cast <peer_t *> (i->data.ptr)->sock == sock)){
					// Если событие ещё не удалено из базы событий
					if(!erased){
						// Выполняем изменение параметров события
						result = (::epoll_ctl(this->_efd, EPOLL_CTL_DEL, sock, &(* i)) == 0);
						// Если событие принадлежит к таймеру
						if(reinterpret_cast <peer_t *> (i->data.ptr)->type == event_type_t::TIMER)
							// Выполняем удаление таймера
							this->_watch.away(reinterpret_cast <peer_t *> (i->data.ptr)->sock);
					}
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
			// Флаг удалённого события из базы событий
			bool erased = false;
			// Выполняем блокировку чтения базы событий
			this->_locker = true;
			// Выполняем поиск файлового дескриптора из списка событий
			for(auto i = this->_events.begin(); i != this->_events.end(); ++i){
				// Если сокет найден
				if((erased = (i->ident == sock))){
					// Выполняем удаление объекта события
					EV_SET(&(* i), i->ident, EVFILT_READ | EVFILT_WRITE, EV_DELETE, 0, 0, 0);
					// Выполняем удаление события из списка отслеживания
					this->_events.erase(i);
					// Выходим из цикла
					break;
				}
			}
			// Выполняем поиск файлового дескриптора из списка изменений
			for(auto i = this->_change.begin(); i != this->_change.end(); ++i){
				// Если сокет найден
				if(i->ident == sock){
					// Если событие ещё не удалено из базы событий
					if(!erased)
						// Выполняем удаление объекта события
						EV_SET(&(* i), i->ident, EVFILT_READ | EVFILT_WRITE, EV_DELETE, 0, 0, 0);
					// Выполняем удаление события из списка изменений
					this->_change.erase(i);
					// Выходим из цикла
					break;
				}
			}
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
				// Флаг удалённого события из базы событий
				bool erased = false;
				// Выполняем поиск файлового дескриптора из списка событий
				for(auto j = this->_events.begin(); j != this->_events.end(); ++j){
					// Если сокет найден
					if((reinterpret_cast <peer_t *> (j->data.ptr) == &i->second) &&
					   (reinterpret_cast <peer_t *> (j->data.ptr)->id == id)){
						// Выполняем изменение параметров события
						result = erased = (::epoll_ctl(this->_efd, EPOLL_CTL_DEL, i->second.sock, &(* j)) == 0);
						// Выполняем удаление события из списка отслеживания
						this->_events.erase(j);
						// Выходим из цикла
						break;
					}
				}
				// Выполняем поиск файлового дескриптора из списка изменений
				for(auto j = this->_change.begin(); j != this->_change.end(); ++j){
					// Если сокет найден
					if((reinterpret_cast <peer_t *> (j->data.ptr) == &i->second) &&
					   (reinterpret_cast <peer_t *> (j->data.ptr)->id == id)){
						// Если событие ещё не удалено из базы событий
						if(!erased)
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
				// Флаг удалённого события из базы событий
				bool erased = false;
				// Выполняем блокировку чтения базы событий
				this->_locker = true;
				// Выполняем поиск файлового дескриптора из списка событий
				for(auto j = this->_events.begin(); j != this->_events.end(); ++j){
					// Если сокет найден
					if((erased = (j->ident == sock))){
						/**
						 * Определяем тип события к которому принадлежит сокет
						 */
						switch(static_cast <uint8_t> (i->second.type)){
							// Если событие принадлежит к таймеру
							case static_cast <uint8_t> (event_type_t::TIMER):
							// Если событие принадлежит к потоку
							case static_cast <uint8_t> (event_type_t::STREAM):
								// Выполняем удаление события таймера
								EV_SET(&(* j), j->ident, EVFILT_READ, EV_DELETE, 0, 0, 0);
							break;
							// Если это другие события
							default:
								// Выполняем удаление объекта события
								EV_SET(&(* j), j->ident, EVFILT_READ | EVFILT_WRITE, EV_DELETE, 0, 0, 0);
						}
						// Выполняем удаление события из списка отслеживания
						this->_events.erase(j);
						// Выходим из цикла
						break;
					}
				}
				// Выполняем поиск файлового дескриптора из списка изменений
				for(auto j = this->_change.begin(); j != this->_change.end(); ++j){
					// Если сокет найден
					if(j->ident == sock){
						// Если событие ещё не удалено из базы событий
						if(!erased){
							/**
							 * Определяем тип события к которому принадлежит сокет
							 */
							switch(static_cast <uint8_t> (i->second.type)){
								// Если событие принадлежит к таймеру
								case static_cast <uint8_t> (event_type_t::TIMER):
								// Если событие принадлежит к потоку
								case static_cast <uint8_t> (event_type_t::STREAM):
									// Выполняем удаление события таймера
									EV_SET(&(* j), j->ident, EVFILT_READ, EV_DELETE, 0, 0, 0);
								break;
								// Если это другие события
								default:
									// Выполняем удаление объекта события
									EV_SET(&(* j), j->ident, EVFILT_READ | EVFILT_WRITE, EV_DELETE, 0, 0, 0);
							}
						}
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
						// Выполняем поиск файлового дескриптора из списка событий
						for(auto k = this->_events.begin(); k != this->_events.end(); ++k){
							// Если сокет найден
							if((reinterpret_cast <peer_t *> (k->data.ptr) == &i->second) &&
							   (reinterpret_cast <peer_t *> (k->data.ptr)->id == id)){
								// Выполняем удаление события из списка событий
								this->_events.erase(k);
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
								for(auto k = this->_change.begin(); k != this->_change.end(); ++k){
									// Если сокет найден
									if((erased = (k->ident == sock))){
										// Выполняем удаление работы события
										EV_SET(&(* k), k->ident, EVFILT_READ, EV_DELETE, 0, 0, 0);
										// Выполняем удаление типа события
										i->second.mode.erase(j);
										// Выполняем удаление события из списка изменений
										this->_change.erase(k);
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
								for(auto k = this->_change.begin(); k != this->_change.end(); ++k){
									// Если сокет найден
									if((erased = (k->ident == sock))){
										// Выполняем удаление работы события
										EV_SET(&(* k), k->ident, EVFILT_READ, EV_DELETE, 0, 0, 0);
										// Выполняем удаление типа события
										i->second.mode.erase(j);
										// Выполняем удаление события из списка изменений
										this->_change.erase(k);
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
								for(auto k = this->_change.begin(); k != this->_change.end(); ++k){
									// Если сокет найден
									if((erased = (k->ident == sock))){
										// Выполняем удаление работы события
										EV_SET(&(* k), k->ident, EVFILT_READ | EVFILT_WRITE, EV_DELETE, 0, 0, &i->second);
										// Выполняем поиск типа события
										auto l = i->second.mode.find(event_type_t::WRITE);
										// Если режим записи данных найден и он активирован
										if((l != i->second.mode.end()) && (l->second == event_mode_t::ENABLED))
											// Выполняем активацию события на запись
											EV_SET(&(* k), k->ident, EVFILT_WRITE, EV_ADD | EV_CLEAR | EV_ENABLE, 0, 0, &i->second);
										// Выполняем удаление типа события
										i->second.mode.erase(j);
										// Если список режимов событий пустой
										if(i->second.mode.empty() || (i->second.mode.find(event_type_t::WRITE) == i->second.mode.end()))
											// Выполняем удаление события из списка изменений
											this->_change.erase(k);
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
								for(auto k = this->_change.begin(); k != this->_change.end(); ++k){
									// Если сокет найден
									if((erased = (k->ident == sock))){
										// Выполняем удаление работы события
										EV_SET(&(* k), k->ident, EVFILT_READ | EVFILT_WRITE, EV_DELETE, 0, 0, &i->second);
										// Выполняем поиск типа события
										auto l = i->second.mode.find(event_type_t::READ);
										// Если режим чтения данных найден и он активирован
										if((l != i->second.mode.end()) && (l->second == event_mode_t::ENABLED))
											// Выполняем активацию события на чтение
											EV_SET(&(* k), k->ident, EVFILT_READ, EV_ADD | EV_CLEAR | EV_ENABLE, 0, 0, &i->second);
										// Выполняем удаление типа события
										i->second.mode.erase(j);
										// Если список режимов событий пустой
										if(i->second.mode.empty() || (i->second.mode.find(event_type_t::READ) == i->second.mode.end()))
											// Выполняем удаление события из списка изменений
											this->_change.erase(k);
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
					}
					// Если список режимов событий пустой
					if(i->second.mode.empty() || ((i->second.mode.size() == 1) && (i->second.mode.find(event_type_t::CLOSE) != i->second.mode.end()))){
						// Выполняем поиск файлового дескриптора из списка событий
						for(auto k = this->_events.begin(); k != this->_events.end(); ++k){
							// Если сокет найден
							if(k->ident == sock){
								/**
								 * Определяем тип события к которому принадлежит сокет
								 */
								switch(static_cast <uint8_t> (i->second.type)){
									// Если событие принадлежит к таймеру
									case static_cast <uint8_t> (event_type_t::TIMER):
									// Если событие принадлежит к потоку
									case static_cast <uint8_t> (event_type_t::STREAM):
										// Выполняем удаление события таймера
										EV_SET(&(* k), k->ident, EVFILT_READ, EV_DELETE, 0, 0, 0);
									break;
									// Если это другие события
									default:
										// Выполняем полное удаление события из базы событий
										EV_SET(&(* k), k->ident, EVFILT_READ | EVFILT_WRITE, EV_DELETE, 0, 0, 0);
								}
								// Выполняем удаление события из списка событий
								this->_events.erase(k);
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
				// Выполняем блокировку чтения базы событий
				this->_locker = true;
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
							// Устанавливаем новый объект для отслеживания события
							this->_events.push_back((struct epoll_event){});
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
							// Устанавливаем новый объект для отслеживания события
							this->_events.push_back((struct kevent){});
							// Выполняем заполнение нулями всю структуру изменений
							::memset(&this->_change.back(), 0, sizeof(this->_change.back()));
							// Выполняем заполнение нулями всю структуру событий
							::memset(&this->_events.back(), 0, sizeof(this->_events.back()));
							// Устанавливаем идентификатор файлового дескриптора
							this->_change.back().ident = sock;
							// Выполняем смену режима работы отлова события
							EV_SET(&this->_change.back(), this->_change.back().ident, EVFILT_READ | EVFILT_WRITE, EV_ADD | EV_CLEAR | EV_DISABLE, 0, 0, item);
						}
					}
				#endif
				// Выполняем разблокировку чтения базы событий
				this->_locker = false;
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
													// Выполняем деактивацию таймера
													this->_watch.away(k->fd);
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
												// Выполняем деактивацию таймера
												} else this->_watch.away(sock);
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
												// Выполняем деактивацию таймера
												} else this->_watch.away(sock);
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
							// Выполняем поиск файлового дескриптора из списка событий
							for(auto k = this->_change.begin(); k != this->_change.end(); ++k){
								// Если сокет найден
								if(k->ident == sock){
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
													EV_SET(&(* k), k->ident, EVFILT_READ, EV_ADD | EV_CLEAR | EV_ENABLE, 0, 0, &i->second);
													// Выполняем активацию таймера на указанное время
													this->_watch.wait(k->ident, i->second.delay);
												} break;
												// Если нужно деактивировать событие работы таймера
												case static_cast <uint8_t> (event_mode_t::DISABLED): {
													// Выполняем смену режима работы отлова события
													EV_SET(&(* k), k->ident, EVFILT_READ, EV_ADD | EV_CLEAR | EV_DISABLE, 0, 0, &i->second);
													// Выполняем деактивацию таймера
													this->_watch.away(k->ident);
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
													// Выполняем смену режима работы отлова события
													EV_SET(&(* k), k->ident, EVFILT_READ, EV_ADD | EV_CLEAR | EV_ENABLE, 0, 0, &i->second);
												break;
												// Если нужно деактивировать событие чтения из сокета
												case static_cast <uint8_t> (event_mode_t::DISABLED):
													// Выполняем смену режима работы отлова события
													EV_SET(&(* k), k->ident, EVFILT_READ, EV_ADD | EV_CLEAR | EV_DISABLE, 0, 0, &i->second);
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
													// Выполняем смену режима работы отлова события
													EV_SET(&(* k), k->ident, EVFILT_READ, EV_ADD | EV_CLEAR | EV_ENABLE, 0, 0, &i->second);
												break;
												// Если нужно деактивировать событие чтения из сокета
												case static_cast <uint8_t> (event_mode_t::DISABLED): {
													// Выполняем смену режима работы отлова события
													EV_SET(&(* k), k->ident, EVFILT_READ | EVFILT_WRITE, EV_ADD | EV_CLEAR | EV_DISABLE, 0, 0, &i->second);
													// Если событие на запись включено
													if(i->second.mode.at(event_type_t::WRITE) == event_mode_t::ENABLED)
														// Выполняем активацию события на запись
														EV_SET(&(* k), k->ident, EVFILT_WRITE, EV_ADD | EV_CLEAR | EV_ENABLE, 0, 0, &i->second);
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
												case static_cast <uint8_t> (event_mode_t::ENABLED):
													// Выполняем смену режима работы отлова события
													EV_SET(&(* k), k->ident, EVFILT_WRITE, EV_ADD | EV_CLEAR | EV_ENABLE, 0, 0, &i->second);
												break;
												// Если нужно деактивировать событие записи в сокет
												case static_cast <uint8_t> (event_mode_t::DISABLED): {
													// Выполняем смену режима работы отлова события
													EV_SET(&(* k), k->ident, EVFILT_READ | EVFILT_WRITE, EV_ADD | EV_CLEAR | EV_DISABLE, 0, 0, &i->second);
													// Если событие на чтение включено
													if(i->second.mode.at(event_type_t::READ) == event_mode_t::ENABLED)
														// Выполняем активацию события на чтение
														EV_SET(&(* k), k->ident, EVFILT_READ, EV_ADD | EV_CLEAR | EV_ENABLE, 0, 0, &i->second);
												} break;
											}
										} break;
									}
									// Выходим из цикла
									break;
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
			// Выполняем поиск файлового дескриптора из списка событий
			for(auto i = this->_events.begin(); i != this->_events.end();)
				// Выполняем удаление события из списка отслеживания
				i = this->_events.erase(i);
		/**
		 * Для операционной системы FreeBSD, NetBSD, OpenBSD или MacOS X
		 */
		#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
			// Выполняем поиск файлового дескриптора из списка событий
			for(auto i = this->_events.begin(); i != this->_events.end();){
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
				// Выполняем удаление события из списка отслеживания
				i = this->_events.erase(i);
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
		if(this->_works){
			// Выполняем активацию блокировки
			this->_locker = static_cast <bool> (this->_works);
			// Запоминаем список активных событий
			std::map <SOCKET, peer_t> items = this->_peers;
			// Выполняем очистку всех параметров
			this->clear();
			// Выполняем деинициализацию базы событий
			this->init(event_mode_t::DISABLED);
			// Выполняем инициализацию базы событий
			this->init(event_mode_t::ENABLED);
			// Если список активных событий не пустой
			if(!items.empty()){
				// Выполняем перебор всего списка активных событий
				for(auto & item : items){
					// Выполняем добавление события в базу событий
					if(!this->add(item.second.id, item.second.sock, item.second.callback, item.second.delay, item.second.persist))
						// Выводим сообщение что событие не вышло активировать
						this->_log->print("Failed activate event for SOCKET=%d", log_t::flag_t::WARNING, item.second.sock);
					// Если событие добавленно удачно
					else {
						// Выполняем перебор всех разрешений на запуск событий
						for(auto & mode : item.second.mode)
							// Выполняем активацию события на запуск
							this->mode(item.second.id, item.second.sock, mode.first, mode.second);
					}
				}
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
							// Выполняем запуск ожидания входящих событий сокетов
							poll = ::epoll_wait(this->_efd, this->_events.data(), AWH_MAX_COUNT_FDS, (!this->_easily ? static_cast <int32_t> (this->_rate) : 0));
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
														// Выполняем чтение входящего события
														const uint64_t event = this->_watch.event(sock);
														// Если чтение выполнено удачно
														if(event > 0){
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
														// Удаляем сокет из базы событий
														} else this->del(item->id, item->sock);
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
															auto j = item->mode.find(event_type_t::STREAM);
															// Если событие найдено и оно активированно
															if((j != item->mode.end()) && (j->second == event_mode_t::ENABLED))
																// Выполняем функцию обратного вызова
																this->stream(i->first, event);
														}
													} break;
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
							// Выполняем запуск ожидания входящих событий сокетов
							poll = ::kevent(this->_kq, this->_change.data(), this->_change.size(), this->_events.data(), this->_events.size(), ((this->_rate > -1) || this->_easily ? &baseDelay : nullptr));
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
										// Получаем код ошибки переданный ядром
										code = event.data;
										// Получаем флаг закрытия подключения
										isClose = (event.flags & EV_EOF);
										// Получаем флаг получения ошибки сокета
										isError = (event.flags & EV_ERROR);
										// Получаем флаг достуности чтения из сокета
										isRead = (event.filter & EVFILT_READ);
										// Получаем флаг доступности сокета на запись
										isWrite = (event.filter & EVFILT_WRITE);
										// Получаем флаг нашего кастомного события
										isEvent = (event.filter & EVFILT_USER);
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
														// Выполняем чтение входящего события
														const uint64_t event = this->_watch.event(sock);
														// Если чтение выполнено удачно
														if(event > 0){
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
														// Удаляем сокет из базы событий
														} else this->del(item->id, sock);
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
															auto j = item->mode.find(event_type_t::STREAM);
															// Если событие найдено и оно активированно
															if((j != item->mode.end()) && (j->second == event_mode_t::ENABLED))
																// Выполняем функцию обратного вызова
																this->stream(i->first, event);
														}
													} break;
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
				// Запоминаем список активных событий
				std::map <SOCKET, peer_t> items = this->_peers;
				// Выполняем остановку работы базы событий
				this->stop();
				// Если список активных событий не пустой
				if(!items.empty()){
					// Выполняем перебор всего списка активных событий
					for(auto & item : items){
						// Выполняем добавление события в базу событий
						if(!this->add(item.second.id, item.second.sock, item.second.callback))
							// Выводим сообщение что событие не вышло активировать
							this->_log->print("Failed activate event for SOCKET=%d", log_t::flag_t::WARNING, item.second.sock);
						// Если событие добавленно удачно
						else {
							// Выполняем перебор всех разрешений на запуск событий
							for(auto & mode : item.second.mode)
								// Выполняем активацию события на запуск
								this->mode(item.second.id, item.second.sock, mode.first, mode.second);
						}
					}
				}
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
 _fds(log), _watch(fmk, log), _fmk(fmk), _log(log) {
	// Получаем идентификатор потока
	this->_wid = this->wid();
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
