/**
 * @file: reactor.cpp
 * @date: 2025-10-14
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
 * Если максимальное количество опрашиваемых событий за одну итерацию (64, 128, 256, 512, 1024)
 */
#ifndef AWH_MAX_POLL_EVENTS_COUNT
	/**
	 * Устанавливаем максимальное количество опрашиваемых событий за одну итерацию (64)
	 */
	#define AWH_MAX_POLL_EVENTS_COUNT 0x40
#endif

/**
 * Для операционной системы Linux
 */
#if __linux__
	/**
	 * Подключаем системные заголовки
	 */
	#include <sys/epoll.h>
/**
 * Для операционной системы Sun Solaris
 */
#elif __sun__
	/**
	 * Подключаем системные заголовки
	 */
	#include "/usr/include/port.h"
	#include "/usr/include/sys/devpoll.h"
/**
 * Для операционной системы MacOS X, FreeBSD, NetBSD или OpenBSD
 */
#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
	/**
	 * Подключаем системные заголовки
	 */
	#include <sys/event.h>
#endif

/**
 * Стандартные модули
 */
#include <mutex>
#include <atomic>
#include <chrono>
#include <thread>
#include <memory>
#include <string>
#include <vector>
#include <cstdlib>
#include <iostream>
#include <unordered_map>
#include <fcntl.h>

/**
 * Наши модули
 */
#include <events/fds.hpp>

/**
 * Подключаем заголовочный файл
 */
#include <events/reactor.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Идентификатор потока
 */
static uint64_t __awh_wid__;
/**
 * @brief Мютекс блокировки потока для основных операций
 *
 */
static mutex __awh_main_mtx__;
/**
 * @brief Мютекс блокировки потока для уведомителя
 *
 */
static mutex __awh_stream_mtx__;
/**
 * Сокет активного рабочего таймера
 */
static SOCKET __awh_timer__ = INVALID_SOCKET;
/**
 * Список идентификаторов активных сокетов
 */
static unordered_map <SOCKET, uint32_t> __awh_ids__;
/**
 * Список активных уведомителей
 */
static unordered_map <uint32_t, std::unique_ptr <awh::notifier_t>> __awh_notifiers__;

/**
 * @brief Функция получения идентификатора потока
 *
 * @return идентификатор потока для получения
 */
static uint64_t wid() noexcept {
	// Создаём объект хэширования
	hash <thread::id> hasher;
	// Устанавливаем идентификатор потока
	return hasher(this_thread::get_id());
}
/**
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
	/**
	 * Параметры сетевой инициализации
	 */
	static WSADATA __awh_wsa__;
	/**
	 * Переменная инициализации WinSocksAPI
	 */
	static bool __awh_winsock__ = false;
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
/**
 * Для операционной системы Sun Solaris
 */
#elif __sun__
	/**
	 * Флаг активации Event Ports
	 */
	static bool __awh_event_ports__ = false;
	/**
	 * @brief Функция определения поддержки портов в Sun Solaris
	 *
	 * @return результат проверки доступности портов
	 */
	static bool tryEventPorts() noexcept {
		/**
		 * Если нам необходимо использовать порты
		 */
		#if AWH_USING_EVENTS_PORTS
			// Выполняем создание порта
			const SOCKET port = ::port_create();
			// Если порт удачно создан
			if(port != INVALID_SOCKET){
				// Выполняем закрытие порта
				::close(port);
				// Выводим положительный результат
				return true;
			}
		#endif
		// Выводим отрицательный результат
		return false;
	}
#endif

/**
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
	/**
	 * @brief Структура событийной модели
	 *
	 */
	struct EventLoop {
		// Список активных сокетов
		SOCKET sockets[AWH_MAX_POLL_EVENTS_COUNT];
		// Список активных событий
		WSAEVENT events[AWH_MAX_POLL_EVENTS_COUNT];
		// Список активных поллеров
		vector <awh::react_t::poller_t> pollers;
		// Список активных таймеров
		unordered_map <uint32_t, pair <uint32_t, bool>> timers;
		/**
		 * @brief Конструктор
		 *
		 */
		EventLoop() noexcept {}
	};
/**
 * Для операционной системы Sun Solaris
 */
#elif __sun__
	/**
	 * @brief Структура событийной модели
	 *
	 */
	struct EventLoop {
		// Сокет связи с ядром операционной системы
		int32_t wfd;
		/**
		 * @brief Конструктор
		 *
		 */
		EventLoop() noexcept : wfd(0) {}
	};
	/**
	 * @brief Структура событийной модели для Event Ports
	 *
	 */
	struct EventLoop1 : public EventLoop {
		// Количество добавленных сокетов
		size_t count;
		// Список активных событий
		port_event_t events[AWH_MAX_POLL_EVENTS_COUNT];
		// Список активных поллеров
		unordered_map <uint32_t, awh::react_t::poller_t> pollers;
		/**
		 * @brief Конструктор
		 *
		 */
		EventLoop1() noexcept : count(0) {}
	};
	/**
	 * @brief Структура событийной модели для /dev/poll
	 *
	 */
	struct EventLoop2 : public EventLoop {
		// Создаем объект опроса
		struct dvpoll dop;
		// Флаг фиксации изменений
		std::atomic_bool commit;
		// Список активных событий
		vector <struct pollfd> events;
		// Список активных поллеров
		vector <awh::react_t::poller_t> pollers;
		// Список активных таймеров
		unordered_map <uint32_t, pair <uint32_t, bool>> timers;
		/**
		 * @brief Конструктор
		 *
		 */
		EventLoop2() noexcept : commit(false) {}
	};
/**
 * Для операционной системы Linux
 */
#elif __linux__
	/**
	 * @brief Структура событийной модели
	 *
	 */
	struct EventLoop {
		// Сокет связи с ядром операционной системы
		int32_t efd;
		// Список активных событий
		struct epoll_event events[AWH_MAX_POLL_EVENTS_COUNT];
		// Список активных поллеров
		unordered_map <uint32_t, awh::react_t::poller_t> pollers;
		/**
		 * @brief Конструктор
		 *
		 */
		EventLoop() noexcept : efd(0) {}
	};
/**
 * Для операционной системы MacOS X, FreeBSD, NetBSD или OpenBSD
 */
#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
	/**
	 * @brief Структура расширенного поллера
	 *
	 */
	typedef struct ExtendPoller : awh::react_t::poller_t {
		// Статус активированных событий
		uint8_t active;
		/**
		 * Конструктор
		 */
		ExtendPoller() noexcept : active(0) {}
	} epoller_t;
	/**
	 * @brief Структура событийной модели
	 *
	 */
	struct EventLoop {
		// Сокет связи с ядром операционной системы
		int32_t kq;
		// Список активных событий
		struct kevent events[AWH_MAX_POLL_EVENTS_COUNT];
		// Список временных событий ожидающих активации
		vector <struct kevent> change;
		// Список активных поллеров
		unordered_map <uint32_t, epoller_t> pollers;
		/**
		 * @brief Конструктор
		 *
		 */
		EventLoop() noexcept : kq(0) {}
	};
#endif

/**
 * Объект событийной модели
 */
static unique_ptr <EventLoop> __awh_loop__(nullptr);

/**
 * @brief Функция применения сетевой оптимизации операционной системы
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
static void boostingNetwork([[maybe_unused]] const awh::fmk_t * fmk, const awh::log_t * log) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем инициализацию объекта работы с операционноы системы
		awh::os_t os;
		// Выполняем инициализацию объекта работы с файловыми дескрипторами
		awh::fds_t fds(log);
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
						log->debug("%s", __PRETTY_FUNCTION__, {}, awh::log_t::flag_t::WARNING, ::strerror(errno));
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
					#endif
				}
			#endif
		#endif
		/**
		 * Выполняем установку нужного нам количества файловых дескрипторов
		 */
		if(!fds.limit(AWH_MAX_COUNT_FDS)){
			// Получаем лимиты файловых дескрипторов
			const auto & limits = fds.limit();
			// Если текущий лимит меньше желаемого
			if(limits.first < AWH_MAX_COUNT_FDS)
				// Выводим сообщение подсказки
				fds.help(limits.first, AWH_MAX_COUNT_FDS);
		}
		/**
		 * Если необходимо выполнить тюннинг операционной системы
		 */
		#if AWH_BOOSTING_NET
			/**
			 * Для операционных систем не относящихся к MS Windows
			 */
			#if !_WIN32 && !_WIN64
				// Устанавливаем правила освобождения памяти
				::mallopt(M_MMAP_THRESHOLD, 64 * 1024);
				::mallopt(M_TRIM_THRESHOLD, 128 * 1024);
			#endif
			/**
			 * Для операционной системы MS Windows
			 */
			#if _WIN32 || _WIN64
				// Если эффективный идентификатор пользователя принадлежит Administrator
				if(os.isAdmin()){
					// Vista/7 также включает «Compound TCP (CTCP)», который похож на CUBIC в Linux
					os.exec("netsh interface tcp set global congestionprovider=ctcp");
					// Если вам вообще нужно включить автонастройку, вот команды
					os.exec("netsh interface tcp set global autotuninglevel=normal");
				// Если пользователь не является суперпользователем
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						log->debug("%s", __PRETTY_FUNCTION__, {}, awh::log_t::flag_t::WARNING, "Administrator privileges are required to apply network optimizations");
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						log->print("%s", awh::log_t::flag_t::WARNING, "Administrator privileges are required to apply network optimizations");
					#endif
				}
			/**
			 * Реализация под Sun Solaris
			 */
			#elif __sun__
				// Если эффективный идентификатор пользователя принадлежит ROOT
				if(os.isAdmin()){
					// Эмпирическое правило: max_buf = 2 x cwnd_max (окно перегрузки)
					os.exec("ndd -set /dev/tcp tcp_max_buf 4194304");
					os.exec("ndd -set /dev/tcp tcp_cwnd_max 2097152");
					// Увеличиваем размер окна TCP по умолчанию
					os.exec("ndd -set /dev/tcp tcp_xmit_hiwat 65536");
					os.exec("ndd -set /dev/tcp tcp_recv_hiwat 65536");
				// Если пользователь не является суперпользователем
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						log->debug("%s", __PRETTY_FUNCTION__, {}, awh::log_t::flag_t::WARNING, "Root privileges are required to apply network optimizations");
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						log->print("%s", awh::log_t::flag_t::WARNING, "Root privileges are required to apply network optimizations");
					#endif
				}
			/**
			 * Для операционной системы MacOS X
			 */
			#elif __APPLE__ || __MACH__
				// Если эффективный идентификатор пользователя принадлежит ROOT
				if(os.isAdmin()){
					// Устанавливаем максимальное количество подключений
					os.sysctl("kern.ipc.somaxconn", 49152);
					/**
					 * Для хостов 10G было бы неплохо увеличить это значение,
					 * т.к. 4G, похоже, является пределом для некоторых установок MacOS X
					 */
					os.sysctl("kern.ipc.maxsockbuf", 6291456);
					// Увеличиваем максимальный размер буферов для отправки
					os.sysctl("net.inet.tcp.sendspace", 1042560);
					// Увеличиваем максимальный размер буферов для чтения
					os.sysctl("net.inet.tcp.recvspace", 1042560);
					// В MacOS X значение по умолчанию 3, что очень мало
					os.sysctl("net.inet.tcp.r", 8);
					// Увеличиваем максимумы автонастройки MacOS X TCP
					os.sysctl("net.inet.tcp.autorcvbufmax", 33554432);
					os.sysctl("net.inet.tcp.autosndbufmax", 33554432);
					// Устанавливаем прочие настройки
					os.sysctl("net.inet.tcp.slowstart_flightsize", 20);
					os.sysctl("net.inet.tcp.local_slowstart_flightsize", 20);
				// Если пользователь не является суперпользователем
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						log->debug("%s", __PRETTY_FUNCTION__, {}, awh::log_t::flag_t::WARNING, "Root privileges are required to apply network optimizations");
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						log->print("%s", awh::log_t::flag_t::WARNING, "Root privileges are required to apply network optimizations");
					#endif
				}
			/**
			 * Для операционной системы Linux
			 */
			#elif __linux__
				// Если эффективный идентификатор пользователя принадлежит ROOT
				if(os.isAdmin()){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Для отладки активируем создание дампов ядра
						os.sysctl("kernel.core_uses_pid", 1);
						os.sysctl("kernel.core_pattern", "/tmp/%e-%p.core");
					#endif
					// Разрешаем выборочные подтверждения (Selective Acknowledgements, SACK)
					os.sysctl("net.ipv4.tcp_sack", 1);
					// Активируем параметр помогающий в борье за ресурсы
					os.sysctl("net.ipv4.tcp_tw_reuse", 1);
					// Разрешаем использование временных меток (timestamps) в протоколах TCP
					os.sysctl("net.ipv4.tcp_timestamps", 1);
					// Устанавливаем максимальное количество подключений
					os.sysctl("net.core.somaxconn", 49152);
					// Увеличиваем максимальный размер буферов для чтения
					os.sysctl("net.core.rmem_max", 16777216);
					// Увеличиваем максимальный размер буферов для отправки
					os.sysctl("net.core.wmem_max", 16777216);
					// Разрешаем масштабирование TCP-окна
					os.sysctl("net.ipv4.tcp_window_scaling", 1);
					// Запрещаем сохранять результаты измерений TCP-соединения в кэше при его закрытии
					os.sysctl("net.ipv4.tcp_no_metrics_save", 1);
					// Включаем автоматическую настройку размера приёмного буфера TCP
					os.sysctl("net.ipv4.tcp_moderate_rcvbuf", 1);
					// Определяем максимальное количество входящих пакетов
					os.sysctl("net.core.netdev_max_backlog", 2500);
					// Увеличиваем лимит автонастройки TCP-буфера Linux до 64 МБ
					os.sysctl("net.ipv4.tcp_rmem", "\"4096 87380 16777216\"");
					os.sysctl("net.ipv4.tcp_wmem", "\"4096 65536 16777216\"");
					// Рекомендуется для хостов с включенными большими фреймами
					os.sysctl("net.ipv4.tcp_mtu_probing", 1);
					// Рекомендуется для хостов CentOS 7/Debian 8
					os.sysctl("net.core.default_qdisc", "fq");
					/**
					 * Рекомендуемый контроль перегрузки по умолчанию — htcp.
					 * Вы можете проверить, какие доступны алгоритмы получения доступных сообщений, используя net.ipv4.tcp_available_congestion_control
					 */
					const string & algorithm = os.sysctl <string> ("net.ipv4.tcp_available_congestion_control");
					// Если выбран лучший доступны алгоритм
					if(!algorithm.empty()){
						// Если найден алгоритм cubic
						if(fmk->exists("cubic", algorithm))
							// Активируем выбранный нами алгоритм
							os.sysctl("net.ipv4.tcp_congestion_control", "cubic");
						// Если же найден алгоритм htcp
						else if(fmk->exists("htcp", algorithm))
							// Активируем выбранный нами алгоритм
							os.sysctl("net.ipv4.tcp_congestion_control", "htcp");
					}
				// Если пользователь не является суперпользователем
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						log->debug("%s", __PRETTY_FUNCTION__, {}, awh::log_t::flag_t::WARNING, "Root privileges are required to apply network optimizations");
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						log->print("%s", awh::log_t::flag_t::WARNING, "Root privileges are required to apply network optimizations");
					#endif
				}
			/**
			 * Для операционной системы FreeBSD, NetBSD или OpenBSD
			 */
			#elif __FreeBSD__ || __NetBSD__ || __OpenBSD__
				// Если эффективный идентификатор пользователя принадлежит ROOT
				if(os.isAdmin()){
					/**
					 * Данные оптимизаций операционной системы берет от сюда: http://fasterdata.es.net/host-tuning/freebsd
					 */
					// Активируем контроль работы временной марки и масштабируемого окна
					os.sysctl("net.inet.tcp.rfc1323", 1);
					// Устанавливаем максимальное количество подключений
					os.sysctl("kern.ipc.somaxconn", 49152);
					// Активируем автоматическую отправку и получение
					os.sysctl("net.inet.tcp.sendbuf_auto", 1);
					os.sysctl("net.inet.tcp.recvbuf_auto", 1);
					// Увеличиваем размер шага автонастройки
					os.sysctl("net.inet.tcp.sendbuf_inc", 8192);
					os.sysctl("net.inet.tcp.recvbuf_inc", 16384);
					// Активируем нормальное нормальное TCP Reno
					os.sysctl("net.inet.tcp.inflight.enable", 0);
					// Активируем на хостах тестирования/измерений
					os.sysctl("net.inet.tcp.hostcache.expire", 1);
					/**
					 * Для хостов 10G было бы неплохо увеличить это значение,
					 * т.к. 4G, похоже, является пределом для некоторых установок FreeBSD
					 */
					os.sysctl("kern.ipc.maxsockbuf", 16777216);
					// Увеличиваем максимальный размер буферов для отправки
					os.sysctl("net.inet.tcp.sendspace", 1042560);
					// Увеличиваем максимальный размер буферов для чтения
					os.sysctl("net.inet.tcp.recvspace", 1042560);
					// Увеличиваем максимальный размер буферов для отправки
					os.sysctl("net.inet.tcp.sendbuf_max", 16777216);
					// Увеличиваем максимальный размер буферов для чтения
					os.sysctl("net.inet.tcp.recvbuf_max", 16777216);
					/**
					 * Вы можете проверить, какие доступны алгоритмы получения доступных сообщений, используя net.inet.tcp.cc.available
					 */
					const string & algorithm = os.sysctl <string> ("net.inet.tcp.cc.available");
					// Если выбран лучший доступны алгоритм
					if(!algorithm.empty()){
						// Если найден алгоритм cubic
						if(fmk->exists("cubic", algorithm))
							// Активируем выбранный нами алгоритм
							os.sysctl("net.inet.tcp.cc.algorithm", "cubic");
						// Если же найден алгоритм htcp
						else if(fmk->exists("htcp", algorithm))
							// Активируем выбранный нами алгоритм
							os.sysctl("net.inet.tcp.cc.algorithm", "htcp");
					}
				// Если пользователь не является суперпользователем
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						log->debug("%s", __PRETTY_FUNCTION__, {}, awh::log_t::flag_t::WARNING, "Root privileges are required to apply network optimizations");
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						log->print("%s", awh::log_t::flag_t::WARNING, "Root privileges are required to apply network optimizations");
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
			log->debug("%s", __PRETTY_FUNCTION__, {}, awh::log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			log->print("%s", awh::log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}

/**
 * @brief Метод инициализации событийной модели
 *
 * @return результат инициализации
 */
bool awh::Reactor::init() noexcept {
	// Если событийная модель не активированна
	if(::__awh_loop__ == nullptr){
		// Выполняем блокировку потока
		const lock_guard lock(::__awh_main_mtx__);
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Если WinSocksAPI ещё не инициализирована
			if(!::winsockInitialized()){
				// Идентификатор ошибки
				int32_t error = 0;
				// Выполняем инициализацию сетевого контекста
				if((error = ::WSAStartup(MAKEWORD(2, 2), &::__awh_wsa__)) != 0){
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
				if((2 != LOBYTE(::__awh_wsa__.wVersion)) || (2 != HIBYTE(::__awh_wsa__.wVersion))){
					// Выводим сообщение об ошибке
					this->_log->print("Events loop is not init", log_t::flag_t::CRITICAL);
					// Очищаем сетевой контекст
					::WSACleanup();
					// Выходим из приложения
					::exit(EXIT_FAILURE);
				}
				// Устанавливаем флаг инициализации
				::__awh_winsock__ = true;
			}
			/**
			 * Выполняем настройку сетевых параметров
			 */
			::boostingNetwork(this->_fmk, this->_log);
			/**
			 * Выполняем инициализацию событийной модели
			 */
			::__awh_loop__ = std::make_unique <EventLoop> ();
		/**
		 * Для операционной системы Sun Solaris
		 */
		#elif __sun__
			/**
			 * Выполняем настройку сетевых параметров
			 */
			::boostingNetwork(this->_fmk, this->_log);
			// Если активированна поддержка Event Ports
			if((::__awh_event_ports__ = ::tryEventPorts())){
				/**
				 * Выполняем инициализацию событийной модели
				 */
				::__awh_loop__ = unique_ptr <EventLoop> (new EventLoop1);
				// Выполняем инициализацию Event Ports
				if((::__awh_loop__->wfd = ::port_create()) == INVALID_SOCKET){
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
					// Очищаем объект порта
					::__awh_loop__.reset(nullptr);
					// Выходим из приложения
					::exit(EXIT_FAILURE);
				}
			// Если активированна поддержка /dev/poll
			} else {
				/**
				 * Выполняем инициализацию событийной модели
				 */
				::__awh_loop__ = unique_ptr <EventLoop> (new EventLoop2);
				// Выполняем инициализацию /dev/poll
				if((::__awh_loop__->wfd = ::open("/dev/poll", O_RDWR, 0)) == INVALID_SOCKET){
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
					// Очищаем объект порта
					::__awh_loop__.reset(nullptr);
					// Выходим из приложения
					::exit(EXIT_FAILURE);
				}
			}
			// Устанавливаем флаг автозакрытия файлового дескриптора
			::fcntl(::__awh_loop__->wfd, F_SETFD, FD_CLOEXEC);
		/**
		 * Для операционной системы Linux
		 */
		#elif __linux__
			/**
			 * Выполняем настройку сетевых параметров
			 */
			::boostingNetwork(this->_fmk, this->_log);
			/**
			 * Выполняем инициализацию событийной модели
			 */
			::__awh_loop__ = std::make_unique <EventLoop> ();
			// Выполняем инициализацию EPoll
			if((::__awh_loop__->efd = ::epoll_create(AWH_MAX_EVENTS_LOOP)) == INVALID_SOCKET){
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
				// Очищаем объект порта
				::__awh_loop__.reset(nullptr);
				// Выходим из приложения
				::exit(EXIT_FAILURE);
			}
			// Устанавливаем флаг автозакрытия файлового дескриптора
			::fcntl(::__awh_loop__->efd, F_SETFD, FD_CLOEXEC);
		/**
		 * Для операционной системы MacOS X, FreeBSD, NetBSD или OpenBSD
		 */
		#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
			/**
			 * Выполняем настройку сетевых параметров
			 */
			::boostingNetwork(this->_fmk, this->_log);
			/**
			 * Выполняем инициализацию событийной модели
			 */
			::__awh_loop__ = std::make_unique <EventLoop> ();
			// Выполняем инициализацию Kqueue
			if((::__awh_loop__->kq = ::kqueue()) == INVALID_SOCKET){
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
				// Очищаем объект порта
				::__awh_loop__.reset(nullptr);
				// Выходим из приложения
				::exit(EXIT_FAILURE);
			}
			// Устанавливаем флаг автозакрытия файлового дескриптора
			::fcntl(::__awh_loop__->kq, F_SETFD, FD_CLOEXEC);
		#endif
		// Выполняем создание таймера
		::__awh_timer__ = this->_watch.create();
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Выполняем создание объекта события
			WSAEVENT event = ::WSACreateEvent();
			// Если событие не может быть создано
			if(event == WSA_INVALID_EVENT){
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
				// Выполняем закрытие сокета
				::closesocket(::__awh_timer__);
				// Очищаем объект порта
				::__awh_loop__.reset(nullptr);
				// Если WinSocksAPI была инициализированна в этой базе событий
				if(::__awh_winsock__)
					// Очищаем сетевой контекст
					::WSACleanup();
				// Выходим из приложения
				::exit(EXIT_FAILURE);
			}
			// Выполняем активацию работы таймера
			if(::WSAEventSelect(::__awh_timer__, event, FD_READ | FD_CLOSE) == SOCKET_ERROR){
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
				// Очищаем инициализированный объект события
				::WSACloseEvent(event);
				// Выполняем закрытие сокета
				::closesocket(::__awh_timer__);
				// Очищаем объект порта
				::__awh_loop__.reset(nullptr);
				// Если WinSocksAPI была инициализированна в этой базе событий
				if(::__awh_winsock__)
					// Очищаем сетевой контекст
					::WSACleanup();
				// Выходим из приложения
				::exit(EXIT_FAILURE);
			}
			// Устанавливаем наше новое событие в список событий
			::__awh_loop__->events.push_back(::move(event));
			// Выполняем установку сокета таймера
			::__awh_loop__->sockets.push_back(::__awh_timer__);
			// Выполняем установку пустого значения поллера
			::__awh_loop__->pollers.push_back(react_t::poller_t{});
			// Устанавливаем тип события
			::__awh_loop__->pollers.back().events = AWH_TIMER;
		/**
		 * Для операционной системы Sun Solaris
		 */
		#elif __sun__
			// Если активирована поддержка Event Ports
			if(::__awh_event_ports__){
				// Выполняем получение объекта Event Loop
				EventLoop1 * loop = dynamic_cast <EventLoop2 *> (::__awh_loop__.get());
				// Выполняем добавление поллера
				auto ret = loop->pollers.emplace(0, react_t::poller_t{});
				// Устанавливаем тип события
				ret.first->second.events = AWH_TIMER;
				// Выполняем активацию работы таймера
				if(::port_associate(loop->wfd, PORT_SOURCE_FD, ::__awh_timer__, POLLIN | POLLERR | POLLHUP, &ret.first->second) != 0){
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
					// Выполняем закрытие сокета
					::close(::__awh_timer__);
					// Очищаем объект порта
					::__awh_loop__.reset(nullptr);
					// Выходим из приложения
					::exit(EXIT_FAILURE);
				}
				// Устанавливаем количество активных сокетов
				loop->count++;
			// Если активированна поддержка /dev/poll
			} else {
				// Выполняем создание объекта события
				struct pollfd event = {0};
				// Устанавливаем файловый дескриптор таймера
				event.fd = ::__awh_timer__;
				// Выполняем сброс всех событий
				event.events = AWH_NONE;
				// Активируем событие на чтение
				event.events |= POLLIN;
				// Активируем событие на получение ошибки
				event.events |= POLLERR;
				// Активируем событие на получение дисконнекта
				event.events |= POLLHUP;
				// Выполняем получение объекта Event Loop
				EventLoop2 * loop = dynamic_cast <EventLoop2 *> (::__awh_loop__.get());
				// Устанавливаем флаг фиксации изменений
				loop->commit = true;
				// Устанавливаем наше новое событие в список событий
				loop->events.push_back(::move(event));
				// Выполняем установку пустого значения поллера
				loop->pollers.push_back(react_t::poller_t{});
				// Устанавливаем тип события
				loop->pollers.back().events = AWH_TIMER;
			}
		/**
		 * Для операционной системы Linux
		 */
		#elif __linux__
			// Выполняем создание объекта события
			struct epoll_event event = {0};
			// Выполняем сброс всех событий
			event.events = AWH_NONE;
			// Активируем событие на чтение
			event.events |= EPOLLIN;
			// Активируем событие на получение ошибки
			event.events |= EPOLLERR;
			// Активируем событие на получение дисконнекта
			event.events |= EPOLLHUP;
			// Устанавливаем файловый дескриптор таймера
			event.data.fd = ::__awh_timer__;
			// Выполняем добавление поллера
			auto ret = ::__awh_loop__->pollers.emplace(0, react_t::poller_t{});
			// Устанавливаем тип события
			ret.first->second.events = AWH_TIMER;
			// Выполняем установку указателя на основное событие
			event.data.ptr = &ret.first->second;
			// Выполняем изменение параметров события
			if(::epoll_ctl(::__awh_loop__->efd, EPOLL_CTL_ADD, ::__awh_timer__, &event) != 0){
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
				// Выполняем закрытие сокета
				::close(::__awh_timer__);
				// Очищаем объект порта
				::__awh_loop__.reset(nullptr);
				// Выходим из приложения
				::exit(EXIT_FAILURE);
			}
		/**
		 * Для операционной системы MacOS X, FreeBSD, NetBSD или OpenBSD
		 */
		#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
			// Выполняем добавление поллера
			auto ret = ::__awh_loop__->pollers.emplace(0, epoller_t{});
			// Устанавливаем тип события
			ret.first->second.events = AWH_TIMER;
			// Устанавливаем наше новое событие в список событий
			::__awh_loop__->change.push_back((struct kevent){});
			// Выполняем активацию работы таймера
			EV_SET(&::__awh_loop__->change.back(), ::__awh_timer__, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, &ret.first->second);
		#endif
		// Запускаем работу таймера
		this->_watch.start();
		// Выводим результат
		return true;
	}
	// Выводим значение по умолчанию
	return false;
}
/**
 * @brief Метод разрушения событийной модели
 *
 * @return результат разрушения
 */
bool awh::Reactor::destroy() noexcept {
	// Если событийная модель активированна
	if(::__awh_loop__ != nullptr){
		// Выполняем блокировку потока
		const lock_guard lock(::__awh_main_mtx__);
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Выполняем переход по всем открытым событиям
			for(auto & event : ::__awh_loop__->events)
				// Выполняем закрытие всех событий
				::WSACloseEvent(event);
		#endif
		// Если сокет таймера открыт
		if(::__awh_timer__ != INVALID_SOCKET){
			// Останавливаем работу таймера
			this->_watch.stop();
			/**
			 * Для операционной системы MS Windows
			 */
			#if _WIN32 || _WIN64
				// Выполняем закрытие сокета
				::closesocket(::__awh_timer__);
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#else
				// Выполняем закрытие сокета
				::close(::__awh_timer__);
			#endif
		}
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Если WinSocksAPI была инициализированна в этой базе событий
			if(::__awh_winsock__)
				// Очищаем сетевой контекст
				::WSACleanup();
		/**
		 * Для операционной системы Sun Solaris
		 */
		#elif __sun__
			// Выполняем закрытие подключения
			::close(::__awh_loop__->wfd);
		/**
		 * Для операционной системы Linux
		 */
		#elif __linux__
			// Выполняем закрытие подключения
			::close(::__awh_loop__->efd);
		/**
		 * Для операционной системы MacOS Xб FreeBSD, NetBSD или OpenBSD
		 */
		#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
			// Выполняем закрытие подключения
			::close(::__awh_loop__->kq);
		#endif
		// Очищаем объект порта
		::__awh_loop__.reset(nullptr);
		// Выходим из функции
		return true;
	}
	// Выводим значение по умолчанию
	return false;
}
/**
 * @brief Метод извлечения входящих уведомлений
 *
 * @param id идентификатор события
 * @return   данные уведомления
 */
uint32_t awh::Reactor::notifications(const uint32_t id) noexcept {
	// Если данные для установки переданы и база событий инициализированна
	if((id > 0) && (::__awh_loop__ != nullptr)){
		// Если метод запущен в дочернем потоке
		if(::__awh_wid__ != ::wid()){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("You cannot initialize a stream with ID=%d in a child thread", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING, id);
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("You cannot initialize a stream with ID=%d in a child thread", log_t::flag_t::WARNING, id);
			#endif
			// Выходим из функции
			return false;
		}
		// Выполняем поиск нужного нам уведомителя
		auto i = ::__awh_notifiers__.find(id);
		// Если уведомитель найден
		if(i != ::__awh_notifiers__.end()){
			// Выполняем блокировку потока
			const lock_guard lock(::__awh_stream_mtx__);
			// Выполняем извлечение уведомления
			return i->second->event();
		}
	// Если произошла ошибка добавления сокета для отслеживания
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Event with ID=%d could not be retrieved from the event database loop", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING, id);
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Event with ID=%d could not be retrieved from the event database loop", log_t::flag_t::WARNING, id);
		#endif
	}
	// Выходим из функции
	return 0;
}
/**
 * @brief Метод отправки уведомления о событии
 *
 * @param id   идентификатор события
 * @param data данные уведомления для отправки
 * @return     результат выполнения уведомления
 */
bool awh::Reactor::notify(const uint32_t id, const uint32_t data) noexcept {
	// Если данные для установки переданы и база событий инициализированна
	if((id > 0) && (::__awh_loop__ != nullptr)){
		// Если метод запущен в основном потоке
		if(::__awh_wid__ == ::wid()){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("You cannot send a message to stream with ID=%d from the main thread", __PRETTY_FUNCTION__, std::make_tuple(id, data), log_t::flag_t::WARNING, id);
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("You cannot send a message to stream with ID=%d from the main thread", log_t::flag_t::WARNING, id);
			#endif
			// Выходим из функции
			return false;
		}
		// Выполняем поиск нужного нам уведомителя
		auto i = ::__awh_notifiers__.find(id);
		// Если уведомитель найден
		if(i != ::__awh_notifiers__.end()){
			// Выполняем блокировку потока
			const lock_guard lock(::__awh_stream_mtx__);
			// Выполняем отправку родительскому потоку сообщение
			i->second->notify(data);
			// Выводим удачный результат
			return true;
		}
	// Если произошла ошибка добавления сокета для отслеживания
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Event with ID=%d could not be added to the event database loop", __PRETTY_FUNCTION__, std::make_tuple(id, data), log_t::flag_t::WARNING, id);
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Event with ID=%d could not be added to the event database loop", log_t::flag_t::WARNING, id);
		#endif
	}
	// Выходим из функции
	return false;
}
/**
 * @brief Метод ожидания получения событий
 *
 * @param pollers список сработавших событий
 * @param msec    время ожидания события в миллисекундах
 * @return        количество полученных событий
 */
uint32_t awh::Reactor::wait(poller_t * pollers, const int32_t msec) noexcept {
	/**
	 * Для операционной системы Sun Solaris
	 */
	#if __sun__
		// Если активированна поддержка /dev/poll
		if(!::__awh_event_ports__){
			// Выполняем получение объекта Event Loop
			EventLoop2 * loop = dynamic_cast <EventLoop2 *> (::__awh_loop__.get());
			// Если необходимо выполнить переинициализацию списка событий
			if(loop->commit){
				// Снимаем флаг фиксации изменений
				loop->commit = false;
				// Если перед началом работы необходимо зафиксировать изменения
				if(!loop->events.empty()){
					// Получаем размер записываемых данных
					const size_t size = (sizeof(struct pollfd) * loop->events.size());
					// Отправляем ядру операционной системы только активные сокеты
					if(::write(loop->wfd, &loop->events[0], size) != size){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(msec), log_t::flag_t::CRITICAL, ::strerror(errno));
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
		}
	/**
	 * Для операционной системы MacOS X, FreeBSD, NetBSD или OpenBSD
	 */
	#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
		// Если в базе событий есть добавленные сокеты
		if(!::__awh_loop__->change.empty()){
			// Выполняем блокировку потока
			const lock_guard lock(::__awh_main_mtx__);
			// Выполняем изменение параметров события
			if(::kevent(::__awh_loop__->kq, &::__awh_loop__->change[0], ::__awh_loop__->change.size(), nullptr, 0, nullptr) == INVALID_SOCKET){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(msec), log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
			}
			// Выполняем очистку временного списка событий
			::__awh_loop__->change.clear();
		}
	#endif
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		// Если в базе событий есть добавленные сокеты
		if(!::__awh_loop__->events.empty()){
			// Получаем индекс текущего элемента
			const DWORD index = ::WSAWaitForMultipleEvents(
				::__awh_loop__->events.size(), &::__awh_loop__->events[0],
				FALSE, (msec < 0 ? WSA_INFINITE : static_cast <DWORD> (msec)), FALSE
			);
			// Если мы получили таймаут
			if(index == WSA_WAIT_TIMEOUT){
				// Устанавливаем событие таймаута
				pollers[0].events = AWH_TIMEOUT;
				// Выводим событие таймаута
				return 1;
			}
			// Если мы получили ошибку сокета
			if(index == WSA_WAIT_FAILED){
				// Устанавливаем событие ошибки
				pollers[0].events = AWH_ERROR;
				// Выводим значение ошибки
				return 1;
			}
			// Получаем смещение в списоке событий
			const uint32_t offset = (index - WSA_WAIT_EVENT_0);
			// Выполняем получение сокета
			const SOCKET sock = ::__awh_loop__->sockets[offset];
			// Если сокет соответствует таймеру
			if(sock == ::__awh_timer__){
				// Устанавливаем событие Таймера
				pollers[0].events = AWH_TIMER;
				// Получаем идентификатор таймера
				pollers[0].id = this->_watch.event();
				// Выполняем блокировку потока
				const lock_guard lock(::__awh_main_mtx__);
				// Выполняем проверку является ли таймер персистентным
				auto i = ::__awh_loop__->timers.find(pollers[0].id);
				// Если мы нашли нужный нам таймер
				if(i != ::__awh_loop__->timers.end()){
					// Если таймер является персистентным
					if(i->second.second){
						// Устанавливаем событие таймера как Интервал
						pollers[0].events = AWH_INTERVAL;
						// Выполняем активацию таймера на указанное время
						this->_watch.wait(i->first, i->second.first);
					// Если таймер является обычным
					} else {
						// Выполняем остановку работы таймера
						this->_watch.away(i->first);
						// Выполняем удаление таймера
						::__awh_loop__->timers.erase(i);
					}
				}
			// Если событие не является таймером
			} else {
				// Получаем объект события
				react_t::poller_t & event = ::__awh_loop__->pollers[offset];
				// Устанавливаем идентификатор события
				pollers[0].id = event.id;
				// Если событие является потоком
				if(event.events & AWH_STREAM)
					// Заменяем флаг события
					pollers[0].events = AWH_STREAM;
				// Если мы получили обычное событие
				else {
					// Объект статуса события сокета
					WSANETWORKEVENTS status;
					// Если мы статус событий не смогли прочитать
					if(::WSAEnumNetworkEvents(sock, ::__awh_loop__->events[offset], &status) == SOCKET_ERROR)
						// Устанавливаем событие ошибки
						pollers[0].events = AWH_ERROR;
					// Если мы удачно извлекли события
					else {
						// Выполняем сброс событий
						pollers[0].events = AWH_NONE;
						// Если мы детектировали закрытие подключения
						if(status.lNetworkEvents & FD_CLOSE)
							// Устанавливаем флаг события
							pollers[0].events |= AWH_CLOSE;
						// Если мы детектировали событие готовности сокета на чтение данных
						if(status.lNetworkEvents & (FD_READ | FD_ACCEPT))
							// Устанавливаем флаг события
							pollers[0].events |= AWH_READ;
						// Если мы детектировали событие готовности сокета на запись данных
						if(status.lNetworkEvents & (FD_WRITE | FD_CONNECT))
							// Устанавливаем флаг события
							pollers[0].events |= AWH_WRITE;
						// Если мы детектировали наличие ошибки
						if(status.iErrorCode[FD_CLOSE_BIT] != 0)
							// Устанавливаем флаг события
							pollers[0].events |= AWH_ERROR;
					}
				}
			}
			// Завершаем обработку события
			::WSAResetEvent(::__awh_loop__->events[offset]);
			// Выводим значение ошибки
			return 1;
		}
	/**
	 * Для операционной системы Sun Solaris
	 */
	#elif __sun__
		// Если активирована поддержка Event Ports
		if(::__awh_event_ports__){
			// Создаём объект таймаута
			timespec_t ts, * tsp = nullptr;
			// Если установлено время ожидания
			if(msec >= 0){
				// Устанавливаем количество секунд
				ts.tv_sec = (msec / 1000);
				// Устанавливаем количество наносекунд
				ts.tv_nsec = ((msec % 1000) * 1000000);
				// Активируем параметры таймера
				tsp = &ts;
			}
			// Выполняем получение объекта Event Loop
			EventLoop1 * loop = dynamic_cast <EventLoop1 *> (::__awh_loop__.get());
			// Текущее количество активных сокетов
			uint32_t count = loop->count;
			// Выполняем опрос ядра на наличие событий сокетов
			if((::port_getn(loop->wfd, loop->events, AWH_MAX_POLL_EVENTS_COUNT, &count, tsp) == INVALID_SOCKET) && (count == 0)){
				// Если у нас просто вышло время
				if(errno == ETIME)
					// Устанавливаем событие таймаута
					pollers[0].events = AWH_TIMEOUT;
				// Если мы получили другую ошибку
				else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(msec), log_t::flag_t::WARNING, ::strerror(errno));
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
					#endif
					// Устанавливаем событие ошибки
					pollers[0].events = AWH_ERROR;
				}
				// Выводим значение ошибки
				return 1;
			}
			// Сетевой сокет событие которого мы поймали
			SOCKET sock = INVALID_SOCKET;
			// Смещение в буфере событий и флаги текущих событий
			uint32_t offset = 0, revents = 0;
			// Объект поллера который относится к событию
			react_t::poller_t * event = nullptr;
			// Выполняем перебор всех полученных событий
			for(uint32_t i = 0; i < count; i++){
				// Если мы получили событие сокета
				if(loop->events[i].portev_source == PORT_SOURCE_FD){
					// Получаем сетевой сокет
					sock = loop->events[i].portev_object;
					// Если сокет соответствует таймеру
					if(sock == ::__awh_timer__){
						// Устанавливаем событие Таймера
						pollers[offset].events = AWH_TIMER;
						// Получаем идентификатор таймера
						pollers[offset].id = this->_watch.event();
						// Выполняем блокировку потока
						const lock_guard lock(::__awh_main_mtx__);
						// Выполняем проверку является ли таймер персистентным
						auto i = loop->pollers.find(pollers[offset].id);
						// Если мы нашли нужный нам таймер
						if(i != loop->pollers.end()){
							// Если таймер является персистентным
							if(i->second.events & AWH_INTERVAL){
								// Устанавливаем событие таймера как Интервал
								pollers[offset].events = AWH_INTERVAL;
								// Выполняем активацию таймера на указанное время
								this->_watch.wait(i->first, i->second.id);
							// Если таймер является обычным
							} else {
								// Выполняем остановку работы таймера
								this->_watch.away(i->first);
								// Выполняем удаление таймера
								loop->pollers.erase(i);
							}
						}
						// Увеличиваем количество полученных событий
						offset++;
					// Если событие не является таймером
					} else {
						// Выполняем получение поллера к которому относится событие
						event = reinterpret_cast <react_t::poller_t *> (loop->events[i].portev_user);
						// Если поллер события не получен, значит поллер уже удалён
						if(event == nullptr)
							// Пропускаем событие
							continue;
						// Устанавливаем идентификатор события
						pollers[offset].id = event->id;
						// Если событие является потоком
						if(event->events & AWH_STREAM)
							// Заменяем флаг события
							pollers[offset].events = AWH_STREAM;
						// Если мы получили обычное событие
						else {
							// Получаем события сетевого сокета
							revents = loop->events[i].portev_events;
							// Выполняем сброс событий
							pollers[offset].events = AWH_NONE;
							// Если мы детектировали закрытие подключения
							if((revents & POLLHUP) || (revents & POLLNVAL))
								// Устанавливаем флаг события
								pollers[offset].events |= AWH_CLOSE;
							// Если мы детектировали событие готовности сокета на чтение данных
							if(revents & POLLIN)
								// Устанавливаем флаг события
								pollers[offset].events |= AWH_READ;
							// Если мы детектировали событие готовности сокета на запись данных
							if(revents & POLLOUT)
								// Устанавливаем флаг события
								pollers[offset].events |= AWH_WRITE;
							// Если мы детектировали наличие ошибки
							if(revents & POLLERR)
								// Устанавливаем флаг события
								pollers[offset].events |= AWH_ERROR;
						}
						// Увеличиваем количество полученных событий
						offset++;
					}
				}
			}
			// Выводим результат
			return offset;
		// Если активированна поддержка /dev/poll
		} else {
			// Выполняем получение объекта Event Loop
			EventLoop2 * loop = dynamic_cast <EventLoop2 *> (::__awh_loop__.get());
			// Если перед началом работы необходимо зафиксировать изменения
			if(!loop->events.empty()){
				// Устанавливаем время ожидания событий в миллисекундах
				loop->dop.dp_timeout = msec;
				// Устанавливаем список опрашиваемых событий
				loop->dop.dp_fds = &loop->events[0];
				// Устанавливаем количество опрашиваемых событий
				loop->dop.dp_nfds = loop->events.size();
				// Выполняем ожидание получения событий сокета
				int32_t count = ::ioctl(loop->wfd, DP_POLL, &loop->dop);
				// Если события не получены
				if(count <= 0){
					// Если у нас просто вышло время
					if(count == 0)
						// Устанавливаем событие таймаута
						pollers[0].events = AWH_TIMEOUT;
					// Если мы получили другую ошибку
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(msec), log_t::flag_t::WARNING, ::strerror(errno));
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
						#endif
						// Устанавливаем событие ошибки
						pollers[0].events = AWH_ERROR;
					}
					// Выводим значение ошибки
					return 1;
				}
				// Сетевой сокет событие которого мы поймали
				SOCKET sock = INVALID_SOCKET;
				// Смещение в буфере событий и флаги текущих событий
				uint32_t offset = 0, revents = 0;
				// Выполняем перебор всех полученных событий
				for(int32_t i = 0; i < count; i++){
					// Получаем сетевой сокет
					sock = loop->dop.dp_fds[i].fd;
					// Если сокет соответствует таймеру
					if(sock == ::__awh_timer__){
						// Устанавливаем событие Таймера
						pollers[offset].events = AWH_TIMER;
						// Получаем идентификатор таймера
						pollers[offset].id = this->_watch.event();
						// Выполняем блокировку потока
						const lock_guard lock(::__awh_main_mtx__);
						// Выполняем проверку является ли таймер персистентным
						auto i = loop->timers.find(pollers[0].id);
						// Если мы нашли нужный нам таймер
						if(i != loop->timers.end()){
							// Если таймер является персистентным
							if(i->second.second){
								// Устанавливаем событие таймера как Интервал
								pollers[0].events = AWH_INTERVAL;
								// Выполняем активацию таймера на указанное время
								this->_watch.wait(i->first, i->second.first);
							// Если таймер является обычным
							} else {
								// Выполняем остановку работы таймера
								this->_watch.away(i->first);
								// Выполняем удаление таймера
								loop->timers.erase(i);
							}
						}
						// Увеличиваем количество полученных событий
						offset++;
					// Если событие не является таймером
					} else {
						// Получаем события сетевого сокета
						revents = loop->dop.dp_fds[i].revents;
						// Если сокет давным-давно уже закрыт
						if(revents == 0)
							// Не добавляем в результат
							continue;
						// Получаем объект события
						react_t::poller_t & event = loop->pollers[i];
						// Устанавливаем идентификатор события
						pollers[offset].id = event->id;
						// Если событие является потоком
						if(event->events & AWH_STREAM)
							// Заменяем флаг события
							pollers[offset].events = AWH_STREAM;
						// Если мы получили обычное событие
						else {
							// Выполняем сброс событий
							pollers[offset].events = AWH_NONE;
							// Если мы детектировали закрытие подключения
							if((revents & POLLHUP) || (revents & POLLNVAL))
								// Устанавливаем флаг события
								pollers[offset].events |= AWH_CLOSE;
							// Если мы детектировали событие готовности сокета на чтение данных
							if(revents & POLLIN)
								// Устанавливаем флаг события
								pollers[offset].events |= AWH_READ;
							// Если мы детектировали событие готовности сокета на запись данных
							if(revents & POLLOUT)
								// Устанавливаем флаг события
								pollers[offset].events |= AWH_WRITE;
							// Если мы детектировали наличие ошибки
							if(revents & POLLERR)
								// Устанавливаем флаг события
								pollers[offset].events |= AWH_ERROR;
						}
						// Увеличиваем количество полученных событий
						offset++;
					}
				}
				// Выводим результат
				return offset;
			}
		}
	/**
	 * Для операционной системы Linux
	 */
	#elif __linux__
		// Текущее количество активных сокетов
		int32_t count = 0;
		/**
		 * Выполняем проверку событий для сокетов
		 */
		do {
			// Выполняем опрос ядра на наличие событий сокетов
			count = ::epoll_wait(::__awh_loop__->efd, ::__awh_loop__->events, AWH_MAX_POLL_EVENTS_COUNT, msec);
		/**
		 * Если необходимо повторить проверку
		 */
		} while((count == INVALID_SOCKET) && (errno == EINTR));
		// Если мы не получили событий
		if(count <= 0){
			// Если у нас просто вышло время
			if(count == 0)
				// Устанавливаем событие таймаута
				pollers[0].events = AWH_TIMEOUT;
			// Если мы получили другую ошибку
			else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(msec), log_t::flag_t::WARNING, ::strerror(errno));
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
				#endif
				// Устанавливаем событие ошибки
				pollers[0].events = AWH_ERROR;
			}
			// Выводим значение ошибки
			return 1;
		}
		// Сетевой сокет событие которого мы поймали
		SOCKET sock = INVALID_SOCKET;
		// Смещение в буфере событий и флаги текущих событий
		uint32_t offset = 0, revents = 0;
		// Объект поллера который относится к событию
		react_t::poller_t * event = nullptr;
		// Выполняем перебор всех полученных событий
		for(int32_t i = 0; i < count; i++){
			// Получаем сетевой сокет
			sock = ::__awh_loop__->events[i].data.fd;
			// Если сокет соответствует таймеру
			if(sock == ::__awh_timer__){
				// Устанавливаем событие Таймера
				pollers[offset].events = AWH_TIMER;
				// Получаем идентификатор таймера
				pollers[offset].id = this->_watch.event();
				// Выполняем блокировку потока
				const lock_guard lock(::__awh_main_mtx__);
				// Выполняем проверку является ли таймер персистентным
				auto i = ::__awh_loop__->pollers.find(pollers[offset].id);
				// Если мы нашли нужный нам таймер
				if(i != ::__awh_loop__->pollers.end()){
					// Если таймер является персистентным
					if(i->second.events & AWH_INTERVAL){
						// Устанавливаем событие таймера как Интервал
						pollers[offset].events = AWH_INTERVAL;
						// Выполняем активацию таймера на указанное время
						this->_watch.wait(i->first, i->second.id);
					// Если таймер является обычным
					} else {
						// Выполняем остановку работы таймера
						this->_watch.away(i->first);
						// Выполняем удаление таймера
						::__awh_loop__->pollers.erase(i);
					}
				}
				// Увеличиваем количество полученных событий
				offset++;
			// Если событие не является таймером
			} else {
				// Выполняем получение поллера к которому относится событие
				event = reinterpret_cast <react_t::poller_t *> (::__awh_loop__->events[i].data.ptr);
				// Если поллер события не получен, значит поллер уже удалён
				if(event == nullptr)
					// Пропускаем событие
					continue;
				// Устанавливаем идентификатор события
				pollers[offset].id = event->id;
				// Если событие является потоком
				if(event->events & AWH_STREAM)
					// Заменяем флаг события
					pollers[offset].events = AWH_STREAM;
				// Если мы получили обычное событие
				else {
					// Получаем события сетевого сокета
					revents = ::__awh_loop__->events[i].events;
					// Выполняем сброс событий
					pollers[offset].events = AWH_NONE;
					// Если мы детектировали закрытие подключения
					if((revents & EPOLLHUP) || (revents & EPOLLNVAL))
						// Устанавливаем флаг события
						pollers[offset].events |= AWH_CLOSE;
					// Если мы детектировали событие готовности сокета на чтение данных
					if(revents & EPOLLIN)
						// Устанавливаем флаг события
						pollers[offset].events |= AWH_READ;
					// Если мы детектировали событие готовности сокета на запись данных
					if(revents & EPOLLOUT)
						// Устанавливаем флаг события
						pollers[offset].events |= AWH_WRITE;
					// Если мы детектировали наличие ошибки
					if(revents & EPOLLERR)
						// Устанавливаем флаг события
						pollers[offset].events |= AWH_ERROR;
				}
				// Увеличиваем количество полученных событий
				offset++;
			}
		}
		// Выводим результат
		return offset;
	/**
	 * Для операционной системы MacOS X, FreeBSD, NetBSD или OpenBSD
	 */
	#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
		// Создаём объект таймаута
		struct timespec ts, * tsp = nullptr;
		// Если установлено время ожидания
		if(msec >= 0){
			// Устанавливаем количество секунд
			ts.tv_sec = (msec / 1000);
			// Устанавливаем количество наносекунд
			ts.tv_nsec = ((msec % 1000) * 1000000);
			// Активируем параметры таймера
			tsp = &ts;
		}
		// Выполняем опрос ядра на наличие событий сокетов
		int32_t count = ::kevent(::__awh_loop__->kq, nullptr, 0, ::__awh_loop__->events, AWH_MAX_POLL_EVENTS_COUNT, tsp);
		// Если мы не получили событий
		if(count <= 0){
			// Если у нас просто вышло время
			if(count == 0)
				// Устанавливаем событие таймаута
				pollers[0].events = AWH_TIMEOUT;
			// Если мы получили другую ошибку
			else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(msec), log_t::flag_t::WARNING, ::strerror(errno));
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
				#endif
				// Устанавливаем событие ошибки
				pollers[0].events = AWH_ERROR;
			}
			// Выводим значение ошибки
			return 1;
		}
		// Смещение в буфере событий
		uint32_t offset = 0;
		// Объект поллера который относится к событию
		react_t::poller_t * event = nullptr;
		// Выполняем перебор всех полученных событий
		for(int32_t i = 0; i < count; i++){
			// Получаем текущее значение события
			struct kevent & ev = ::__awh_loop__->events[i];
			// Выполняем получение поллера к которому относится событие
			event = reinterpret_cast <react_t::poller_t *> (ev.udata);
			// Если поллер события не получен, значит поллер уже удалён
			if(event == nullptr)
				// Пропускаем событие
				continue;
			// Если сокет соответствует таймеру
			if(event->events & AWH_TIMER){
				// Устанавливаем событие Таймера
				pollers[offset].events = AWH_TIMER;
				// Получаем идентификатор таймера
				pollers[offset].id = this->_watch.event();
				// Выполняем блокировку потока
				const lock_guard lock(::__awh_main_mtx__);
				// Выполняем проверку является ли таймер персистентным
				auto i = ::__awh_loop__->pollers.find(pollers[offset].id);
				// Если мы нашли нужный нам таймер
				if(i != ::__awh_loop__->pollers.end()){
					// Если таймер является персистентным
					if(i->second.events & AWH_INTERVAL){
						// Устанавливаем событие таймера как Интервал
						pollers[offset].events = AWH_INTERVAL;
						// Выполняем активацию таймера на указанное время
						this->_watch.wait(i->first, i->second.id);
					// Если таймер является обычным
					} else {
						// Выполняем остановку работы таймера
						this->_watch.away(i->first);
						// Выполняем удаление таймера
						::__awh_loop__->pollers.erase(i);
					}
				}
				// Увеличиваем количество полученных событий
				offset++;
			// Если событие не является таймером
			} else {
				// Устанавливаем идентификатор события
				pollers[offset].id = event->id;
				// Если событие является потоком
				if(event->events & AWH_STREAM)
					// Заменяем флаг события
					pollers[offset].events = AWH_STREAM;
				// Если мы получили обычное событие
				else {
					// Выполняем сброс событий
					pollers[offset].events = AWH_NONE;
					// Если мы детектировали закрытие подключения
					if(ev.flags & EV_EOF)
						// Устанавливаем флаг события
						pollers[offset].events |= AWH_CLOSE;
					// Если мы детектировали событие готовности сокета на чтение данных
					if(ev.filter == EVFILT_READ)
						// Устанавливаем флаг события
						pollers[offset].events |= AWH_READ;
					// Если мы детектировали событие готовности сокета на запись данных
					if(ev.filter == EVFILT_WRITE)
						// Устанавливаем флаг события
						pollers[offset].events |= AWH_WRITE;
					// Если мы детектировали наличие ошибки
					if(ev.flags & EV_ERROR)
						// Устанавливаем флаг события
						pollers[offset].events |= AWH_ERROR;
				}
				// Увеличиваем количество полученных событий
				offset++;
			}
		}
		// Выводим результат
		return offset;
	#endif
	// Если время таймаута передано
	if(msec > 0)
		// Замораживаем поток на период времени частоты обновления базы событий
		this_thread::sleep_for(chrono::milliseconds(msec));
	// Если время таймаута не передано, замораживаем на 100 миллисекунд
	else this_thread::sleep_for(100ms);
	// Устанавливаем событие таймаута
	pollers[0].events = AWH_TIMEOUT;
	// Выводим событие таймаута
	return 1;
}
/**
 * @brief Метод удаления событий
 *
 * @param id   идентификатор события
 * @param sock сетевой сокет для удаления
 * @return     результат удаления
 */
bool awh::Reactor::del(const uint32_t id, const SOCKET sock) noexcept {
	// Результат работы функции
	bool result = false;
	// Если данные для установки переданы и база событий инициализированна
	if((id > 0) && (::__awh_loop__ != nullptr)){
		// Выполняем блокировку потока
		const lock_guard lock(::__awh_main_mtx__);
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Выполняем проверку является ли событие таймером
			auto i = ::__awh_loop__->timers.find(id);
			// Если событие является таймером
			if((result = (i != ::__awh_loop__->timers.end()))){
				// Выполняем остановку работы таймера
				this->_watch.away(id);
				// Удаляем таймер из списка таймеров
				::__awh_loop__->timers.erase(i);
			// Если событие не является таймером
			} else {
				// Объект найденного поллера
				react_t::poller_t * peer = nullptr;
				// Выполняем перебор всего списка поллеров
				for(size_t i = 0; i < ::__awh_loop__->pollers.size(); i++){
					// Получаем текущее значение поллера
					peer = &::__awh_loop__->pollers[i];
					// Если мы нашли нужный нам идентификатор
					if((result = (id == peer->id))){
						// Выполняем очистку события сокета
						::WSACloseEvent(::__awh_loop__->events[i]);
						// Выполняем удаление событие сокета
						::__awh_loop__->events.erase(::__awh_loop__->events.begin() + i);
						// Если идентификатор принадлежит потоковому событию
						if(peer->events & AWH_STREAM){
							// Выполняем блокировку потока
							const lock_guard lock(::__awh_stream_mtx__);
							// Выполняем поиск нужного нам уведомителя
							auto i = ::__awh_notifiers__.find(id);
							// Если нужный нам уведомитель найден
							if(i != ::__awh_notifiers__.end())
								// Выполняем удаление уведомителя
								::__awh_notifiers__.erase(i);
							// Выполняем удаление активного сокета
							::__awh_ids__.erase(::__awh_loop__->sockets[i]);
							// Выполняем удаление сокета
							::__awh_loop__->sockets.erase(::__awh_loop__->sockets.begin() + i);
						// Если сокет передан, значит это стандартное событие
						} else if(sock != INVALID_SOCKET) {
							// Если сокет точно соответствует
							if(sock == ::__awh_loop__->sockets[i]){
								// Выполняем удаление активного сокета
								::__awh_ids__.erase(sock);
								// Выполняем удаление сокета
								::__awh_loop__->sockets.erase(::__awh_loop__->sockets.begin() + i);
							// Если сокет не соответствует
							} else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("Event loop model is corrupted, SOCKET=%d does not match SOCKET=%d", __PRETTY_FUNCTION__, std::make_tuple(id, sock), log_t::flag_t::WARNING, sock, ::__awh_loop__->sockets[i]);
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("Event loop model is corrupted, SOCKET=%d does not match SOCKET=%d", log_t::flag_t::WARNING, sock, ::__awh_loop__->sockets[i]);
								#endif
								// Выполняем поиск нужного нам сокета
								for(size_t i = 0; i < ::__awh_loop__->sockets.size(); i++){
									// Если мы нашли нужный нам сокет
									if(sock == ::__awh_loop__->sockets[i]){
										// Выполняем удаление активного сокета
										::__awh_ids__.erase(sock);
										// Выполняем удаление сокета
										::__awh_loop__->sockets.erase(::__awh_loop__->sockets.begin() + i);
										// Выходим из цикла
										break;
									}
								}
							}
						}
						// Выполняем удаление поллера
						::__awh_loop__->pollers.erase(::__awh_loop__->pollers.begin() + i);
						// Выходим из цикла
						break;
					}
				}
			}
		/**
		 * Для операционной системы Sun Solaris
		 */
		#elif __sun__
			// Если активирована поддержка Event Ports
			if(::__awh_event_ports__){
				// Выполняем получение объекта Event Loop
				EventLoop1 * loop = dynamic_cast <EventLoop2 *> (::__awh_loop__.get());
				// Выполняем поиск нужного нам поллера
				auto i = loop->pollers.find(id);
				// Если нужный нам поллер найден
				if((result = (i != loop->pollers.end()))){
					// Выполняем проверку является ли событие таймером
					if((i->second.events & AWH_TIMER) || (i->second.events &  AWH_INTERVAL)){
						// Выполняем остановку работы таймера
						this->_watch.away(id);
						// Выполняем удаление поллера
						loop->pollers.erase(i);
					// Если идентификатор принадлежит потоковому событию
					} else if(i->second.events & AWH_STREAM) {
						// Выполняем блокировку потока
						const lock_guard lock(::__awh_stream_mtx__);
						// Выполняем поиск нужного нам уведомителя
						auto j = ::__awh_notifiers__.find(i->first);
						// Если нужный нам уведомитель найден
						if(j != ::__awh_notifiers__.end()){
							// Извлекаем сокет события
							const_cast <SOCKET &> (sock) = j->second->init();
							// Выполняем деактивацию события сокета
							if(::port_dissociate(loop->wfd, PORT_SOURCE_FD, sock) != 0){
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
								// Выполняем сброс результата
								result = !result;
							}
							// Выполняем удаление активного сокета
							::__awh_ids__.erase(sock);
							// Выполняем удаление уведомителя
							::__awh_notifiers__.erase(j);
							// Уменьшаем количество активных сокетов
							loop->count--;
							// Выполняем удаление поллера
							loop->pollers.erase(i);
						}
					// Если сокет передан верно
					} else if(sock != INVALID_SOCKET) {
						// Выполняем деактивацию события сокета
						if(::port_dissociate(loop->wfd, PORT_SOURCE_FD, sock) != 0){
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
							// Выполняем сброс результата
							result = !result;
						}
						// Выполняем удаление активного сокета
						::__awh_ids__.erase(sock);
						// Уменьшаем количество активных сокетов
						loop->count--;
						// Выполняем удаление поллера
						loop->pollers.erase(i);
					}
				}
			// Если активированна поддержка /dev/poll
			} else {
				// Выполняем получение объекта Event Loop
				EventLoop2 * loop = dynamic_cast <EventLoop2 *> (::__awh_loop__.get());
				// Выполняем проверку является ли событие таймером
				auto i = loop->timers.find(id);
				// Если событие является таймером
				if((result = (i != loop->timers.end()))){
					// Выполняем остановку работы таймера
					this->_watch.away(id);
					// Удаляем таймер из списка таймеров
					loop->timers.erase(i);
				// Если событие не является таймером
				} else {
					// Объект найденного поллера
					react_t::poller_t * peer = nullptr;
					// Выполняем перебор всего списка поллеров
					for(size_t i = 0; i < loop->pollers.size(); i++){
						// Получаем текущее значение поллера
						peer = &loop->pollers[i];
						// Если мы нашли нужный нам идентификатор
						if((result = (id == peer->id))){
							// Закрываем старый дескриптор
							::close(loop->wfd);
							// Если идентификатор принадлежит потоковому событию
							if(peer->events & AWH_STREAM){
								// Выполняем блокировку потока
								const lock_guard lock(::__awh_stream_mtx__);
								// Выполняем поиск нужного нам уведомителя
								auto i = ::__awh_notifiers__.find(id);
								// Если нужный нам уведомитель найден
								if(i != ::__awh_notifiers__.end()){
									// Выполняем удаление активного сокета
									::__awh_ids__.erase(i->second->init());
									// Выполняем удаление уведомителя
									::__awh_notifiers__.erase(i);
								}
							// Если сокет передан верно
							} else if(sock != INVALID_SOCKET)
								// Выполняем удаление активного сокета
								::__awh_ids__.erase(sock);
							// Выполняем удаление поллера
							loop->pollers.erase(loop->pollers.begin() + i);
							// Выполняем инициализацию /dev/poll
							if((loop->wfd = ::open("/dev/poll", O_RDWR, 0)) == INVALID_SOCKET){
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
								// Очищаем объект порта
								::__awh_loop__.reset(nullptr);
								// Выходим из приложения
								::exit(EXIT_FAILURE);
							}
							// Устанавливаем флаг автозакрытия файлового дескриптора
							::fcntl(loop->wfd, F_SETFD, FD_CLOEXEC);
							// Выполняем удаление событие сокета
							loop->events.erase(loop->events.begin() + i);
							// Устанавливаем флаг фиксации изменений
							loop->commit = true;
							// Выходим из цикла
							break;
						}
					}
				}
			}
		/**
		 * Для операционной системы Linux
		 */
		#elif __linux__
			// Выполняем поиск нужного нам поллера
			auto i = ::__awh_loop__->pollers.find(id);
			// Если нужный нам поллер найден
			if((result = (i != ::__awh_loop__->pollers.end()))){
				// Выполняем проверку является ли событие таймером
				if((i->second.events & AWH_TIMER) || (i->second.events &  AWH_INTERVAL)){
					// Выполняем остановку работы таймера
					this->_watch.away(id);
					// Выполняем удаление поллера
					::__awh_loop__->pollers.erase(i);
				// Если идентификатор принадлежит потоковому событию
				} else if(i->second.events & AWH_STREAM) {
					// Выполняем блокировку потока
					const lock_guard lock(::__awh_stream_mtx__);
					// Выполняем поиск нужного нам уведомителя
					auto j = ::__awh_notifiers__.find(i->first);
					// Если нужный нам уведомитель найден
					if(j != ::__awh_notifiers__.end()){
						// Извлекаем сокет события
						const_cast <SOCKET &> (sock) = j->second->init();
						// Выполняем изменение параметров события
						if(::epoll_ctl(::__awh_loop__->efd, EPOLL_CTL_DEL, sock, nullptr) != 0){
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
							// Выполняем сброс результата
							result = !result;
						}
						// Выполняем удаление активного сокета
						::__awh_ids__.erase(sock);
						// Выполняем удаление уведомителя
						::__awh_notifiers__.erase(j);
						// Выполняем удаление поллера
						::__awh_loop__->pollers.erase(i);
					}
				// Если сокет передан верно
				} else if(sock != INVALID_SOCKET) {
					// Выполняем изменение параметров события
					if(::epoll_ctl(::__awh_loop__->efd, EPOLL_CTL_DEL, sock, nullptr) != 0){
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
						// Выполняем сброс результата
						result = !result;
					}
					// Выполняем удаление активного сокета
					::__awh_ids__.erase(sock);
					// Выполняем удаление поллера
					::__awh_loop__->pollers.erase(i);
				}
			}
		/**
		 * Для операционной системы MacOS X, FreeBSD, NetBSD или OpenBSD
		 */
		#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
			// Выполняем поиск нужного нам поллера
			auto i = ::__awh_loop__->pollers.find(id);
			// Если нужный нам поллер найден
			if((result = (i != ::__awh_loop__->pollers.end()))){
				// Выполняем проверку является ли событие таймером
				if((i->second.events & AWH_TIMER) || (i->second.events &  AWH_INTERVAL)){
					// Выполняем остановку работы таймера
					this->_watch.away(id);
					// Выполняем удаление поллера
					::__awh_loop__->pollers.erase(i);
				// Если идентификатор принадлежит потоковому событию
				} else if(i->second.events & AWH_STREAM) {
					// Выполняем блокировку потока
					const lock_guard lock(::__awh_stream_mtx__);
					// Выполняем поиск нужного нам уведомителя
					auto j = ::__awh_notifiers__.find(i->first);
					// Если нужный нам уведомитель найден
					if(j != ::__awh_notifiers__.end()){
						// Добавляем новое событие в список событий
						struct kevent garbage;
						// Извлекаем сокет события
						const_cast <SOCKET &> (sock) = j->second->init();
						// Снимаем событие ожидания сокета готовности на чтение
						EV_SET(&garbage, sock, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
						// Выполняем удаление ненужных нам событий
						::kevent(::__awh_loop__->kq, &garbage, 1, nullptr, 0, nullptr);
						// Выполняем удаление активного сокета
						::__awh_ids__.erase(sock);
						// Выполняем удаление уведомителя
						::__awh_notifiers__.erase(j);
						// Выполняем удаление поллера
						::__awh_loop__->pollers.erase(i);
					}
				// Если сокет передан верно
				} else if(sock != INVALID_SOCKET) {
					// Добавляем новое событие в список событий
					vector <struct kevent> garbage;
					// Если событие ожидания готовности сокета на чтение установлено
					if(i->second.active & AWH_READ){
						// Выполняем добавление нового события
						garbage.push_back((struct kevent){});
						// Снимаем событие ожидания сокета готовности на чтение
						EV_SET(&garbage.back(), sock, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
					}
					// Если событие ожидания готовности сокета на запись установлено
					if(i->second.active & AWH_WRITE){
						// Выполняем добавление нового события
						garbage.push_back((struct kevent){});
						// Снимаем событие ожидания сокета готовности на чтение
						EV_SET(&garbage.back(), sock, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
					}
					// Если нам есть чего удалять
					if(!garbage.empty())
						// Выполняем удаление ненужных нам событий
						::kevent(::__awh_loop__->kq, &garbage[0], garbage.size(), nullptr, 0, nullptr);
					// Выполняем перебор списка готовых идентификаторов
					for(auto i = ::__awh_loop__->change.begin(); i != ::__awh_loop__->change.end();){
						// Если событие соответствует нашему
						if(id == reinterpret_cast <react_t::poller_t *> (i->udata)->id)
							// Выполняем удаление лишнего события
							i = ::__awh_loop__->change.erase(i);
						// Продолжаем перебор
						else ++i;
					}
					// Выполняем удаление активного сокета
					::__awh_ids__.erase(sock);
					// Выполняем удаление поллера
					::__awh_loop__->pollers.erase(i);
				}
			}
		#endif
		// Выводим удачный результат
		return true;
	// Если произошла ошибка добавления сокета для отслеживания
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Event with ID=%d could not be delete from the event database loop", __PRETTY_FUNCTION__, std::make_tuple(id, sock), log_t::flag_t::WARNING, id);
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Event with ID=%d could not be delete from the event database loop", log_t::flag_t::WARNING, id);
		#endif
	}
	// Выходим из функции
	return false;
}
/**
 * @brief Метод модификации события сокета
 *
 * @param id     идентификатор события
 * @param sock   сетевой сокет для модификации
 * @param events модифицированные типы событий
 * @return       результат модификации
 */
bool awh::Reactor::modify(const uint32_t id, const SOCKET sock, const uint8_t events) noexcept {
	// Результат работы функции
	bool result = false;
	// Если данные для установки переданы и база событий инициализированна
	if((sock != INVALID_SOCKET) && (id > 0) && (::__awh_loop__ != nullptr)){
		// Если сокет уже добавлен в список для отслеживания
		if(::__awh_ids__.find(sock) != ::__awh_ids__.end()){
			// Если передано событие потока
			if(events & AWH_STREAM){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Stream tracking event cannot be activated for a network socket", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (events)), log_t::flag_t::WARNING);
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Stream tracking event cannot be activated for a network socket", log_t::flag_t::WARNING);
				#endif
				// Выходим из функции
				return false;
			// Если переданы события таймеров
			} else if((events & AWH_TIMER) || (events & AWH_INTERVAL)) {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Timer tracking event cannot be activated for a network socket", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (events)), log_t::flag_t::WARNING);
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Timer tracking event cannot be activated for a network socket", log_t::flag_t::WARNING);
				#endif
				// Выходим из функции
				return false;
			}
			// Выполняем блокировку потока
			const lock_guard lock(::__awh_main_mtx__);
			/**
			 * Для операционной системы MS Windows
			 */
			#if _WIN32 || _WIN64
				// Выполняем поск необходимого нам поллера
				for(uint32_t i = 0; i < ::__awh_loop__->pollers.size(); i++){
					// Если нужный нам поллер найден
					if((result = (id == ::__awh_loop__->pollers[i].id))){
						// Сонимаем регистрацию для события
						::WSACloseEvent(::__awh_loop__->events[i]);
						// Получаем текущее значение поллера
						react_t::poller_t & poller = ::__awh_loop__->pollers[i];
						// Если было установлено событие готовности на чтение, а сейчас его необходимо отключить
						if(static_cast <bool> (poller.events & AWH_READ) && !static_cast <bool> (events & AWH_READ))
							// Снимаем флаг ожидания готовности на чтение
							poller.events ^= AWH_READ;
						// Если события готовности на чтения небыло установлено, а сейчас требуется установить
						else if(!static_cast <bool> (poller.events & AWH_READ) && static_cast <bool> (events & AWH_READ))
							// Устанавливаем флаг ожидания готовности на чтение
							poller.events |= AWH_READ;
						// Если было установлено событие готовности на запись, а сейчас его необходимо отключить
						if(static_cast <bool> (poller.events & AWH_WRITE) && !static_cast <bool> (events & AWH_WRITE))
							// Снимаем флаг ожидания готовности на запись
							poller.events ^= AWH_WRITE;
						// Если события готовности на запись небыло установлено, а сейчас требуется установить
						else if(!static_cast <bool> (poller.events & AWH_WRITE) && static_cast <bool> (events & AWH_WRITE))
							// Устанавливаем флаг готовности на запись
							poller.events |= AWH_WRITE;
						// Опции событий для установки
						long options = AWH_NONE;
						// Если установлен флаг ожидания получения данных
						if(poller.events & AWH_READ)
							// Ставим флаг ожидания получения данных, появления ошибки или закрытия подключения
							options |= (FD_READ | FD_CLOSE | FD_ACCEPT);
						// Если сокет не активирован на прослушку
						if(!this->_socket.listen(sock)){
							// Если установлен флаг ожидания сокета на запись
							if(poller.events & AWH_WRITE)
								// Ставим флаги ожидания готовности на запись
								options |= (FD_WRITE | FD_CONNECT);
						}
						// Если события необходимо установить
						if(options != AWH_NONE){
							// Выполняем активацию работы таймера
							if(::WSAEventSelect(sock, ::__awh_loop__->events[i], options) == SOCKET_ERROR){
								// Создаём буфер сообщения ошибки
								wchar_t message[256] = {0};
								// Выполняем формирование текста ошибки
								::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 256, 0);
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug(L"%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (events)), log_t::flag_t::CRITICAL, message);
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print(L"%s", log_t::flag_t::CRITICAL, message);
								#endif
								// Выходим из функции
								return false;
							}
						}
						// Выходим из цикла
						break;
					}
				}
			/**
			 * Для операционной системы Sun Solaris
			 */
			#elif __sun__
				// Если активирована поддержка Event Ports
				if(::__awh_event_ports__){
					// Выполняем получение объекта Event Loop
					EventLoop1 * loop = dynamic_cast <EventLoop2 *> (::__awh_loop__.get());
					// Выполняем поск необходимого нам поллера
					auto i = loop->pollers.find(id);
					// Если нужный нам поллер найден
					if((result = (i != loop->pollers.end()))){
						// Выполняем деактивацию события сокета
						::port_dissociate(loop->wfd, PORT_SOURCE_FD, sock);
						// Если было установлено событие готовности на чтение, а сейчас его необходимо отключить
						if(static_cast <bool> (i->second.events & AWH_READ) && !static_cast <bool> (events & AWH_READ))
							// Снимаем флаг ожидания готовности на чтение
							i->second.events ^= AWH_READ;
						// Если события готовности на чтения небыло установлено, а сейчас требуется установить
						else if(!static_cast <bool> (i->second.events & AWH_READ) && static_cast <bool> (events & AWH_READ))
							// Устанавливаем флаг ожидания готовности на чтение
							i->second.events |= AWH_READ;
						// Если было установлено событие готовности на запись, а сейчас его необходимо отключить
						if(static_cast <bool> (i->second.events & AWH_WRITE) && !static_cast <bool> (events & AWH_WRITE))
							// Снимаем флаг ожидания готовности на запись
							i->second.events ^= AWH_WRITE;
						// Если события готовности на запись небыло установлено, а сейчас требуется установить
						else if(!static_cast <bool> (i->second.events & AWH_WRITE) && static_cast <bool> (events & AWH_WRITE))
							// Устанавливаем флаг готовности на запись
							i->second.events |= AWH_WRITE;
						// Опции событий для установки
						int32_t options = AWH_NONE;
						// Если установлен флаг ожидания получения данных
						if(i->second.events & AWH_READ)
							// Ставим флаг ожидания получения данных, появления ошибки или закрытия подключения
							options |= (POLLIN | POLLERR | POLLHUP);
						// Если сокет не активирован на прослушку
						if(!this->_socket.listen(sock)){
							// Если установлен флаг ожидания сокета на запись
							if(i->second.events & AWH_WRITE)
								// Ставим флаг ожидания готовности на запись
								options |= POLLOUT;
						}
						// Если события необходимо установить
						if(options != AWH_NONE){
							// Выполняем активацию отслеживания событий для сокета
							if(::port_associate(loop->wfd, PORT_SOURCE_FD, sock, options, &i->second) != 0){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (events)), log_t::flag_t::CRITICAL, ::strerror(errno));
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
								#endif
								// Выходим из функции
								return false;
							}
							// Устанавливаем флаг фиксации изменений
							loop->commit = true;
						}
					}
				// Если активированна поддержка /dev/poll
				} else {
					// Выполняем получение объекта Event Loop
					EventLoop2 * loop = dynamic_cast <EventLoop2 *> (::__awh_loop__.get());
					// Выполняем поск необходимого нам поллера
					for(uint32_t i = 0; i < loop->pollers.size(); i++){
						// Если нужный нам поллер найден
						if((result = (id == loop->pollers[i].id))){
							// Закрываем старый дескриптор
							::close(loop->wfd);
							// Получаем текущее значение поллера
							react_t::poller_t & poller = loop->pollers[i];
							// Если было установлено событие готовности на чтение, а сейчас его необходимо отключить
							if(static_cast <bool> (poller.events & AWH_READ) && !static_cast <bool> (events & AWH_READ))
								// Снимаем флаг ожидания готовности на чтение
								poller.events ^= AWH_READ;
							// Если события готовности на чтения небыло установлено, а сейчас требуется установить
							else if(!static_cast <bool> (poller.events & AWH_READ) && static_cast <bool> (events & AWH_READ))
								// Устанавливаем флаг ожидания готовности на чтение
								poller.events |= AWH_READ;
							// Если было установлено событие готовности на запись, а сейчас его необходимо отключить
							if(static_cast <bool> (poller.events & AWH_WRITE) && !static_cast <bool> (events & AWH_WRITE))
								// Снимаем флаг ожидания готовности на запись
								poller.events ^= AWH_WRITE;
							// Если события готовности на запись небыло установлено, а сейчас требуется установить
							else if(!static_cast <bool> (poller.events & AWH_WRITE) && static_cast <bool> (events & AWH_WRITE))
								// Устанавливаем флаг готовности на запись
								poller.events |= AWH_WRITE;
							// Получаем параметры события
							struct epoll_event & event = loop->events[i];
							// Выполняем сброс всех событий
							event.events = AWH_NONE;
							// Если установлен флаг ожидания получения данных
							if(poller.events & AWH_READ)
								// Ставим флаг ожидания получения данных, появления ошибки или закрытия подключения
								event.events |= (POLLIN | POLLERR | POLLHUP);
							// Если сокет не активирован на прослушку
							if(!this->_socket.listen(sock)){
								// Если установлен флаг ожидания сокета на запись
								if(poller.events & AWH_WRITE)
									// Ставим флаг ожидания готовности на запись
									event.events |= POLLOUT;
							}
							// Устанавливаем флаг фиксации изменений
							loop->commit = true;
							// Выполняем инициализацию /dev/poll
							if((loop->wfd = ::open("/dev/poll", O_RDWR, 0)) == INVALID_SOCKET){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (events)), log_t::flag_t::CRITICAL, ::strerror(errno));
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
								#endif
								// Выходим из приложения
								::exit(EXIT_FAILURE);
							}
							// Устанавливаем флаг автозакрытия файлового дескриптора
							::fcntl(loop->wfd, F_SETFD, FD_CLOEXEC);
							// Выходим из цикла
							break;
						}
					}
				}
			/**
			 * Для операционной системы Linux
			 */
			#elif __linux__
				// Выполняем поск необходимого нам поллера
				auto i = ::__awh_loop__->pollers.find(id);
				// Если нужный нам поллер найден
				if((result = (i != ::__awh_loop__->pollers.end()))){
					// Выполняем деактивацию события сокета
					::epoll_ctl(::__awh_loop__->efd, EPOLL_CTL_DEL, sock, nullptr);
					// Если было установлено событие готовности на чтение, а сейчас его необходимо отключить
					if(static_cast <bool> (i->second.events & AWH_READ) && !static_cast <bool> (events & AWH_READ))
						// Снимаем флаг ожидания готовности на чтение
						i->second.events ^= AWH_READ;
					// Если события готовности на чтения небыло установлено, а сейчас требуется установить
					else if(!static_cast <bool> (i->second.events & AWH_READ) && static_cast <bool> (events & AWH_READ))
						// Устанавливаем флаг ожидания готовности на чтение
						i->second.events |= AWH_READ;
					// Если было установлено событие готовности на запись, а сейчас его необходимо отключить
					if(static_cast <bool> (i->second.events & AWH_WRITE) && !static_cast <bool> (events & AWH_WRITE))
						// Снимаем флаг ожидания готовности на запись
						i->second.events ^= AWH_WRITE;
					// Если события готовности на запись небыло установлено, а сейчас требуется установить
					else if(!static_cast <bool> (i->second.events & AWH_WRITE) && static_cast <bool> (events & AWH_WRITE))
						// Устанавливаем флаг готовности на запись
						i->second.events |= AWH_WRITE;
					// Выполняем создание объекта события
					struct epoll_event event = {0};
					// Выполняем сброс всех событий
					event.events = AWH_NONE;
					// Если установлен флаг ожидания получения данных
					if(i->second.events & AWH_READ)
						// Ставим флаг ожидания получения данных, появления ошибки или закрытия подключения
						event.events |= (EPOLLIN | EPOLLERR | EPOLLHUP);
					// Если сокет не активирован на прослушку
					if(!this->_socket.listen(sock)){
						// Если установлен флаг ожидания сокета на запись
						if(i->second.events & AWH_WRITE)
							// Ставим флаг ожидания готовности на запись
							event.events |= EPOLLOUT;
					}
					// Если события необходимо установить
					if(event.events != AWH_NONE){
						// Выполняем установку указателя на основное событие
						event.data.ptr = &i->second;
						// Выполняем изменение параметров события
						if(::epoll_ctl(::__awh_loop__->efd, EPOLL_CTL_ADD, sock, &event) != 0){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (events)), log_t::flag_t::CRITICAL, ::strerror(errno));
							/**
							* Если режим отладки не включён
							*/
							#else
								// Выводим сообщение об ошибке
								this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
							#endif
							// Выходим из функции
							return false;
						}
						// Устанавливаем флаг фиксации изменений
						::__awh_loop__->commit = true;
					}
				}
			/**
			 * Для операционной системы MacOS X, FreeBSD, NetBSD или OpenBSD
			 */
			#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
				// Выполняем поск необходимого нам поллера
				auto i = ::__awh_loop__->pollers.find(id);
				// Если нужный нам поллер найден
				if((result = (i != ::__awh_loop__->pollers.end()))){
					// Если было установлено событие готовности на чтение, а сейчас его необходимо отключить
					if(static_cast <bool> (i->second.events & AWH_READ) && !static_cast <bool> (events & AWH_READ)){
						// Выполняем добавление нового события
						::__awh_loop__->change.push_back((struct kevent){});
						// Если событие чтения никогда небыли добавлены
						if(!(i->second.active & AWH_READ)){
							// Устанавливаем флаг события
							i->second.active |= AWH_READ;
							// Устанавливаем событие но отключаем его
							EV_SET(&::__awh_loop__->change.back(), sock, EVFILT_READ, EV_ADD | EV_CLEAR | EV_DISABLE, 0, 0, &i->second);
						// Выполняем отключение события готовности на чтение
						} else EV_SET(&::__awh_loop__->change.back(), sock, EVFILT_READ, EV_DISABLE, 0, 0, &i->second);
					// Если события готовности на чтения небыло установлено, а сейчас требуется установить
					} else if(!static_cast <bool> (i->second.events & AWH_READ) && static_cast <bool> (events & AWH_READ)) {
						// Выполняем добавление нового события
						::__awh_loop__->change.push_back((struct kevent){});
						// Если событие чтения никогда небыли добавлены
						if(!(i->second.active & AWH_READ)){
							// Устанавливаем флаг события
							i->second.active |= AWH_READ;
							// Устанавливаем событие но подключаем его
							EV_SET(&::__awh_loop__->change.back(), sock, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, &i->second);
						// Выполняем активацию события готовности на чтение
						} else EV_SET(&::__awh_loop__->change.back(), sock, EVFILT_READ, EV_ENABLE, 0, 0, &i->second);
					}
					// Если сокет не активирован на прослушку
					if(!this->_socket.listen(sock)){
						// Если было установлено событие готовности на запись, а сейчас его необходимо отключить
						if(static_cast <bool> (i->second.events & AWH_WRITE) && !static_cast <bool> (events & AWH_WRITE)){
							// Выполняем добавление нового события
							::__awh_loop__->change.push_back((struct kevent){});
							// Если событие записи никогда небыли добавлены
							if(!(i->second.active & AWH_WRITE)){
								// Устанавливаем флаг события
								i->second.active |= AWH_WRITE;
								// Устанавливаем событие но отключаем его
								EV_SET(&::__awh_loop__->change.back(), sock, EVFILT_WRITE, EV_ADD | EV_CLEAR | EV_DISABLE, 0, 0, &i->second);
							// Выполняем отключение события готовности на запись
							} else EV_SET(&::__awh_loop__->change.back(), sock, EVFILT_WRITE, EV_DISABLE, 0, 0, &i->second);
						// Если события готовности на запись небыло установлено, а сейчас требуется установить
						} else if(!static_cast <bool> (i->second.events & AWH_WRITE) && static_cast <bool> (events & AWH_WRITE)) {
							// Выполняем добавление нового события
							::__awh_loop__->change.push_back((struct kevent){});
							// Если событие записи никогда небыли добавлены
							if(!(i->second.active & AWH_WRITE)){
								// Устанавливаем флаг события
								i->second.active |= AWH_WRITE;
								// Устанавливаем событие но подключаем его
								EV_SET(&::__awh_loop__->change.back(), sock, EVFILT_WRITE, EV_ADD | EV_CLEAR, 0, 0, &i->second);
							// Выполняем активацию события готовности на запись
							} else EV_SET(&::__awh_loop__->change.back(), sock, EVFILT_WRITE, EV_ENABLE, 0, 0, &i->second);
						}
					}
					// Устанавливаем тип события
					i->second.events = events;
				}
			#endif
		}
	// Если произошла ошибка добавления сокета для отслеживания
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Socket %d with ID=%d could not be added to the event database loop", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (events)), log_t::flag_t::WARNING, sock, id);
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Socket %d with ID=%d could not be added to the event database loop", log_t::flag_t::WARNING, sock, id);
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод добавления несетевых событий
 *
 * @param id     идентификатор несетевого события
 * @param events поддерживаемые типы событий
 * @param msec   время ожидания срабатывания в миллисекундах
 * @return       результат добавления
 */
bool awh::Reactor::add(const uint32_t id, const uint8_t events, const uint32_t msec) noexcept {
	// Если данные для установки переданы и база событий инициализированна
	if((id > 0) && (events != AWH_NONE) && (::__awh_loop__ != nullptr)){
		// Если переданы события таймеров или потока
		if((events == AWH_STREAM) || (events & AWH_TIMER) || (events & AWH_INTERVAL)){
			// Если необходимо установить таймер или интервал
			if(events != AWH_STREAM){
				// Результат работы функции
				bool result = false;
				// Если время задержки таймера установлено
				if(msec > 0){
					// Выполняем блокировку потока
					const lock_guard lock(::__awh_main_mtx__);
					/**
					 * Для операционной системы MS Windows
					 */
					#if _WIN32 || _WIN64
						// Выполняем создание таймера
						result = ::__awh_loop__->timers.emplace(id, std::make_pair(msec, static_cast <bool> (events & AWH_INTERVAL))).second;
					/**
					 * Для операционной системы Sun Solaris
					 */
					#elif __sun__
						// Если активирована поддержка Event Ports
						if(::__awh_event_ports__){
							// Выполняем получение объекта Event Loop
							EventLoop1 * loop = dynamic_cast <EventLoop2 *> (::__awh_loop__.get());
							// Выполняем добавление поллера
							auto ret = loop->pollers.emplace(id, react_t::poller_t{});
							// Устанавливаем результат
							result = ret.second;
							// Устанавливаем интервал времени таймера
							ret.first->second.id = msec;
							// Если событие является интервалом
							if(events & AWH_INTERVAL)
								// Устанавливаем тип события
								ret.first->second.events = AWH_INTERVAL;
							// Если событие является таймером
							else if(events & AWH_TIMER)
								// Устанавливаем тип события
								ret.first->second.events = AWH_TIMER;
						// Если активированна поддержка /dev/poll
						} else {
							// Выполняем получение объекта Event Loop
							EventLoop2 * loop = dynamic_cast <EventLoop2 *> (::__awh_loop__.get());
							// Выполняем создание таймера
							result = loop->timers.emplace(id, std::make_pair(msec, static_cast <bool> (events & AWH_INTERVAL))).second;
						}
					/**
					 * Для операционной системы Linux, MacOS X, FreeBSD, NetBSD или OpenBSD
					 */
					#elif __linux__ || __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
						// Выполняем добавление поллера
						auto ret = ::__awh_loop__->pollers.emplace(id, epoller_t{});
						// Устанавливаем интервал времени таймера
						ret.first->second.id = msec;
						// Если событие является интервалом
						if(events & AWH_INTERVAL)
							// Устанавливаем тип события
							ret.first->second.events = AWH_INTERVAL;
						// Если событие является таймером
						else if(events & AWH_TIMER)
							// Устанавливаем тип события
							ret.first->second.events = AWH_TIMER;
					#endif
					// Выполняем активацию таймера на указанное время
					this->_watch.wait(id, msec);
				}
				// Выводим удачный результат
				return result;
			// Если необходимо установить поток
			} else {
				// Если метод запущен в дочернем потоке
				if(::__awh_wid__ != ::wid()){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("You cannot initialize a stream with ID=%d in a child thread", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (events), msec), log_t::flag_t::WARNING, id);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("You cannot initialize a stream with ID=%d in a child thread", log_t::flag_t::WARNING, id);
					#endif
					// Выходим из функции
					return false;
				}
				// Выполняем блокировку потока
				const lock_guard lock(::__awh_stream_mtx__);
				// Выполняем добавление нового нотификатора
				auto ret = ::__awh_notifiers__.emplace(id, std::make_unique <notifier_t> (this->_fmk, this->_log));
				// Выполняем инициализацию уведомителя
				const SOCKET sock = ret.first->second->init();
				// Если уведомитель инициализирован правильно
				if(sock != INVALID_SOCKET)
					// Выполняем отслеживание сокета на чтение
					return this->add(id, sock, AWH_STREAM);
			}
		// Если нам передали для установки событие для сетевого сокета
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("There are events for a network socket being sent for tracking, but you want to set a timer or thread", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (events), msec), log_t::flag_t::WARNING);
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("There are events for a network socket being sent for tracking, but you want to set a timer or thread", log_t::flag_t::WARNING);
			#endif
		}
	// Если произошла ошибка добавления сокета для отслеживания
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Event with ID=%d could not be added to the event database loop", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (events), msec), log_t::flag_t::WARNING, id);
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Event with ID=%d could not be added to the event database loop", log_t::flag_t::WARNING, id);
		#endif
	}
	// Выходим из функции
	return false;
}
/**
 * @brief Метод добавления сетевого события
 *
 * @param id     идентификатор сетевого события
 * @param sock   сетевой сокет для добавления
 * @param events поддерживаемые типы событий
 * @return       результат добавления
 */
bool awh::Reactor::add(const uint32_t id, const SOCKET sock, const uint8_t events) noexcept {
	// Если данные для установки переданы и база событий инициализированна
	if((sock != INVALID_SOCKET) && (id > 0) && (::__awh_loop__ != nullptr)){
		// Если сокет ещё не добавлен в список для отслеживания
		if(::__awh_ids__.find(sock) == ::__awh_ids__.end()){
			// Если переданы события таймеров
			if((events & AWH_TIMER) || (events & AWH_INTERVAL)){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Timer tracking event cannot be activated for a network socket", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (events)), log_t::flag_t::WARNING);
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Timer tracking event cannot be activated for a network socket", log_t::flag_t::WARNING);
				#endif
				// Выходим из функции
				return false;
			}
		// Сокет уде находится в базе событий, предварительно удаляем его
		} else {
			// Выполняем удаление сокета из базы событий
			if(this->del(id, sock))
				// Выполняем добавление сокета заново
				return this->add(id, sock, events);
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("Socket %d with ID=%d can no longer be added to the event loop database because it is already there", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (events)), log_t::flag_t::WARNING, sock, id);
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("Socket %d with ID=%d can no longer be added to the event loop database because it is already there", log_t::flag_t::WARNING, sock, id);
			#endif
			// Выходим из функции
			return false;
		}
		// Выполняем блокировку потока
		const lock_guard lock(::__awh_main_mtx__);
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Выполняем создание объекта события
			WSAEVENT event = ::WSACreateEvent();
			// Если событие не может быть создано
			if(event == WSA_INVALID_EVENT){
				// Создаём буфер сообщения ошибки
				wchar_t message[256] = {0};
				// Выполняем формирование текста ошибки
				::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 256, 0);
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug(L"%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (events)), log_t::flag_t::CRITICAL, message);
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print(L"%s", log_t::flag_t::CRITICAL, message);
				#endif
				// Выходим из функции
				return false;
			}
			// Опции событий для установки
			long options = AWH_NONE;
			// Если установлен флаг ожидания получения данных
			if(events & AWH_READ)
				// Ставим флаг ожидания закрытия подключения
				options |= (FD_CLOSE | FD_ACCEPT);
			// Если установлен флаг ожидания получения данных или событие потока
			if((events & AWH_READ) || (events & AWH_STREAM))
				// Ставим флаги ожидания готовности на чтение
				options |= FD_READ;
			// Если сокет не активирован на прослушку
			if(!this->_socket.listen(sock)){
				// Если установлен флаг ожидания сокета на запись
				if(events & AWH_WRITE)
					// Ставим флаги ожидания готовности на запись
					options |= (FD_WRITE | FD_CONNECT);
			}
			// Если события необходимо установить
			if(options != AWH_NONE){
				// Выполняем активацию работы таймера
				if(::WSAEventSelect(sock, event, options) == SOCKET_ERROR){
					// Создаём буфер сообщения ошибки
					wchar_t message[256] = {0};
					// Выполняем формирование текста ошибки
					::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 256, 0);
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug(L"%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (events)), log_t::flag_t::CRITICAL, message);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print(L"%s", log_t::flag_t::CRITICAL, message);
					#endif
					// Очищаем инициализированный объект события
					::WSACloseEvent(event);
					// Выходим из функции
					return false;
				}
			}
			// Устанавливаем наш таймер в список активных сокетов
			::__awh_loop__->sockets.push_back(sock);
			// Устанавливаем наше новое событие в список событий
			::__awh_loop__->events.push_back(::move(event));
			// Выполняем установку пустого значения поллера
			::__awh_loop__->pollers.push_back(react_t::poller_t{});
			// Устанавливаем идентификатор события
			::__awh_loop__->pollers.back().id = id;
			// Устанавливаем тип события
			::__awh_loop__->pollers.back().events = events;
			// Устанавливаем соответствие идентификатора сокету
			return ::__awh_ids__.emplace(sock, id).second;
		/**
		 * Для операционной системы Sun Solaris
		 */
		#elif __sun__
			// Если активирована поддержка Event Ports
			if(::__awh_event_ports__){
				// Опции событий для установки
				int32_t options = AWH_NONE;
				// Если установлен флаг ожидания получения данных
				if(events & AWH_READ)
					// Ставим флаг ожидания появления ошибки или закрытия подключения
					options |= (POLLERR | POLLHUP);
				// Если установлен флаг ожидания получения данных или событие потока
				if((events & AWH_READ) || (events & AWH_STREAM))
					// Ставим флаг ожидания готовности на чтение
					options |= POLLIN;
				// Если сокет не активирован на прослушку
				if(!this->_socket.listen(sock)){
					// Если установлен флаг ожидания сокета на запись
					if(events & AWH_WRITE)
						// Ставим флаг ожидания готовности на запись
						options |= POLLOUT;
				}
				// Выполняем получение объекта Event Loop
				EventLoop1 * loop = dynamic_cast <EventLoop2 *> (::__awh_loop__.get());
				// Выполняем добавление поллера
				auto ret = loop->pollers.emplace(id, react_t::poller_t{});
				// Устанавливаем идентификатор события
				ret.first->second.id = id;
				// Устанавливаем тип события
				ret.first->second.events = events;
				// Если события необходимо установить
				if(options != AWH_NONE){
					// Выполняем активацию отслеживания событий для сокета
					if(::port_associate(loop->wfd, PORT_SOURCE_FD, sock, options, &ret.first->second) != 0){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (events)), log_t::flag_t::CRITICAL, ::strerror(errno));
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
						#endif
						// Удаляем добавленного поллера
						loop->pollers.erase(id);
						// Выходим из функции
						return false;
					}
				}
				// Устанавливаем количество активных сокетов
				loop->count++;
			// Если активированна поддержка /dev/poll
			} else {
				// Выполняем создание объекта события
				struct pollfd event = {0};
				// Устанавливаем файловый дескриптор таймера
				event.fd = sock;
				// Выполняем сброс всех событий
				event.events = AWH_NONE;
				// Если установлен флаг ожидания получения данных
				if(events & AWH_READ)
					// Ставим флаг ожидания появления ошибки или закрытия подключения
					event.events |= (POLLERR | POLLHUP);
				// Если установлен флаг ожидания получения данных или событие потока
				if((events & AWH_READ) || (events & AWH_STREAM))
					// Ставим флаг ожидания готовности на чтение
					event.events |= POLLIN;
				// Если сокет не активирован на прослушку
				if(!this->_socket.listen(sock)){
					// Если установлен флаг ожидания сокета на запись
					if(events & AWH_WRITE)
						// Ставим флаг ожидания готовности на запись
						event.events |= POLLOUT;
				}
				// Выполняем получение объекта Event Loop
				EventLoop2 * loop = dynamic_cast <EventLoop2 *> (::__awh_loop__.get());
				// Устанавливаем флаг фиксации изменений
				loop->commit = true;
				// Устанавливаем наше новое событие в список событий
				loop->events.push_back(::move(event));
				// Выполняем установку пустого значения поллера
				loop->pollers.push_back(react_t::poller_t{});
				// Устанавливаем идентификатор события
				loop->pollers.back().id = id;
				// Устанавливаем тип события
				loop->pollers.back().events = events;
			}
			// Устанавливаем соответствие идентификатора сокету
			return ::__awh_ids__.emplace(sock, id).second;
		/**
		 * Для операционной системы Linux
		 */
		#elif __linux__
			// Выполняем создание объекта события
			struct epoll_event event = {0};
			// Выполняем сброс всех событий
			event.events = AWH_NONE;
			// Если установлен флаг ожидания получения данных
			if(events & AWH_READ)
				// Ставим флаг ожидания появления ошибки или закрытия подключения
				event.events |= (EPOLLERR | EPOLLHUP);
			// Если установлен флаг ожидания получения данных или событие потока
			if((events & AWH_READ) || (events & AWH_STREAM))
				// Ставим флаг ожидания готовности на чтение
				event.events |= EPOLLIN;
			// Если сокет не активирован на прослушку
			if(!this->_socket.listen(sock)){
				// Если установлен флаг ожидания сокета на запись
				if(events & AWH_WRITE)
					// Ставим флаг ожидания готовности на запись
					event.events |= EPOLLOUT;
			}
			// Выполняем добавление поллера
			auto ret = loop->pollers.emplace(id, react_t::poller_t{});
			// Устанавливаем идентификатор события
			ret.first->second.id = id;
			// Устанавливаем тип события
			ret.first->second.events = events;
			// Если события необходимо установить
			if(event.events != AWH_NONE){
				// Выполняем установку указателя на основное событие
				event.data.ptr = &ret.first->second;
				// Выполняем изменение параметров события
				if(::epoll_ctl(::__awh_loop__->efd, EPOLL_CTL_ADD, sock, &event) != 0){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (events)), log_t::flag_t::CRITICAL, ::strerror(errno));
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
					#endif
					// Удаляем добавленного поллера
					::__awh_loop__->pollers.erase(id);
					// Выходим из функции
					return false;
				}
			}
			// Устанавливаем соответствие идентификатора сокету
			return ::__awh_ids__.emplace(sock, id).second;
		/**
		 * Для операционной системы MacOS X, FreeBSD, NetBSD или OpenBSD
		 */
		#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
			// Выполняем добавление поллера
			auto ret = ::__awh_loop__->pollers.emplace(id, epoller_t{});
			// Устанавливаем идентификатор события
			ret.first->second.id = id;
			// Устанавливаем событие по умолчанию
			ret.first->second.events = AWH_NONE;
			// Если установлен флаг ожидания получения данных
			if((events & AWH_READ) || (events & AWH_STREAM)){
				// Если установлен флаг на чтение
				if(events & AWH_READ)
					// Устанавливаем тип события
					ret.first->second.events |= AWH_READ;
				// Если установлен флаг потока
				if(events & AWH_STREAM)
					// Устанавливаем тип события
					ret.first->second.events |= (AWH_READ | AWH_STREAM);
				// Выделяем новое событие
				::__awh_loop__->change.push_back((struct kevent){});
				// Ставим флаг ожидания готовности на чтение
				EV_SET(&::__awh_loop__->change.back(), sock, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, &ret.first->second);
			}
			// Если сокет не активирован на прослушку
			if(!this->_socket.listen(sock)){
				// Если установлен флаг ожидания готовности на запись
				if(events & AWH_WRITE){
					// Устанавливаем тип события
					ret.first->second.events |= AWH_WRITE;
					// Выделяем новое событие
					::__awh_loop__->change.push_back((struct kevent){});
					// Ставим флаг ожидания готовности на чтение
					EV_SET(&::__awh_loop__->change.back(), sock, EVFILT_WRITE, EV_ADD | EV_CLEAR, 0, 0, &ret.first->second);
				}
			}
			// Устанавливаем соответствие идентификатора сокету
			return ::__awh_ids__.emplace(sock, id).second;
		#endif
	// Если произошла ошибка добавления сокета для отслеживания
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Socket %d with ID=%d could not be added to the event database loop", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (events)), log_t::flag_t::WARNING, sock, id);
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Socket %d with ID=%d could not be added to the event database loop", log_t::flag_t::WARNING, sock, id);
		#endif
	}
	// Выходим из функции
	return false;
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Reactor::Reactor(const fmk_t * fmk, const log_t * log) noexcept :
 _watch(fmk, log), _socket(fmk, log), _fmk(fmk), _log(log) {
	// Устанавливаем текущий идентификатор потока
	::__awh_wid__ = ::wid();
}
/**
 * @brief Деструктор
 *
 */
awh::Reactor::~Reactor() noexcept {
	// Выполняем разрушение событийной модели
	this->destroy();
}
