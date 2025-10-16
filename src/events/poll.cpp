/**
 * @file: poll.cpp
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
#include <events/poll.hpp>

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
 * @brief Мютекс блокировки потока для таймера
 *
 */
static mutex __awh_timer_mtx__;
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
	 * @return true 
	 * @return false 
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
		vector <SOCKET> sockets;
		// Список активных событий
		vector <WSAEVENT> events;
		// Список активных пиров
		vector <awh::poll_t::event_t> peers;
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
		// Список активных событий
		vector <port_event_t> events;
		// Список активных пиров
		unordered_map <uint32_t, awh::poll_t::event_t> peers;
		/**
		 * @brief Конструктор
		 *
		 */
		EventLoop1() noexcept {}
	};
	/**
	 * @brief Структура событийной модели для /dev/poll
	 *
	 */
	struct EventLoop2 : public EventLoop {
		// Флаг фиксации изменений
		std::atomic_bool commit;
		// Список активных событий
		vector <struct pollfd> events;
		// Список активных пиров
		vector <awh::poll_t::event_t> peers;
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
		vector <struct epoll_event> events;
		// Список активных пиров
		unordered_map <uint32_t, awh::poll_t::event_t> peers;
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
	 * @brief Структура событийной модели
	 *
	 */
	struct EventLoop {
		// Сокет связи с ядром операционной системы
		int32_t kq;
		// Список активных событий
		vector <struct kevent> events;
		// Список активных пиров
		unordered_map <uint32_t, awh::poll_t::event_t> peers;
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
bool awh::Poll::init() noexcept {
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
		// Выполняем установку пустого значения пира
		::__awh_loop__->peers.push_back(poll_t::event_t{});
	/**
	 * Для операционной системы Sun Solaris
	 */
	#elif __sun__
		// Если активирована поддержка Event Ports
		if(::__awh_event_ports__){
			// Выполняем получение объекта Event Loop
			EventLoop1 * loop = dynamic_cast <EventLoop2 *> (::__awh_loop__.get());
			// Выполняем активацию работы таймера
			if(::port_associate(loop->wfd, PORT_SOURCE_FD, ::__awh_timer__, POLLIN, nullptr) != 0){
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
			// Увеличиваем количество событий
			loop->events.push_back(port_event_t{});
		// Если активированна поддержка /dev/poll
		} else {
			// Выполняем создание объекта события
			struct pollfd event = {0};
			// Устанавливаем файловый дескриптор таймера
			event.fd = ::__awh_timer__;
			// Выполняем сброс всех событий
			event.events = 0;
			// Активируем событие на чтение
			event.events |= POLLIN;
			// Активируем событие на получение ошибки
			event.events |= POLLERR;
			// Активируем событие на получение дисконнекта
			event.events |= POLLHUP;
			// Выполняем получение объекта Event Loop
			EventLoop2 * loop = dynamic_cast <EventLoop2 *> (::__awh_loop__.get());
			// Устанавливаем наше новое событие в список событий
			loop->events.push_back(::move(event));
			// Выполняем установку пустого значения пира
			loop->peers.push_back(poll_t::event_t{});
			// Получаем размер записываемых данных
			const size_t size = (sizeof(struct pollfd) * loop->events.size());
			// Отправляем ядру операционной системы только активные сокеты
			if(::write(loop->wfd, &loop->events[0], size) != size){
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
		}
	/**
	 * Для операционной системы Linux
	 */
	#elif __linux__
		// Выполняем создание объекта события
		struct epoll_event event = {0};
		// Выполняем сброс всех событий
		event.events = 0;
		// Активируем событие на чтение
		event.events |= EPOLLIN;
		// Активируем событие на получение ошибки
		event.events |= EPOLLERR;
		// Активируем событие на получение дисконнекта
		event.events |= EPOLLHUP;
		// Устанавливаем файловый дескриптор таймера
		event.data.fd = ::__awh_timer__;
		// Устанавливаем наше новое событие в список событий
		::__awh_loop__->events.push_back((struct epoll_event){});
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
		// Устанавливаем наше новое событие в список событий
		::__awh_loop__->events.push_back((struct kevent){});
		// Выполняем активацию работы таймера
		EV_SET(&::__awh_loop__->events.back(), ::__awh_timer__, EVFILT_READ, EV_ADD | EV_CLEAR | EV_ENABLE, 0, 0, nullptr);
		// Выполняем активацию события таймера
		if(::kevent(::__awh_loop__->kq, &::__awh_loop__->events.back(), 1, nullptr, 0, nullptr) == INVALID_SOCKET){
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
	#endif
	// Запускаем работу таймера
	this->_watch.start();
	// Выводим результат
	return true;
}
/**
 * @brief Метод разрушения событийной модели
 *
 * @return результат разрушения
 */
bool awh::Poll::destroy() noexcept {
	// Выполняем блокировку потока
	const lock_guard lock(::__awh_main_mtx__);
	// Если событийная модель ктивированна
	if(::__awh_loop__ != nullptr){
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
uint32_t awh::Poll::notifications(const uint32_t id) noexcept {
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
bool awh::Poll::notify(const uint32_t id, const uint32_t data) noexcept {
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
 * @brief Метод удаления событий
 *
 * @param id   идентификатор события
 * @param sock сетевой сокет для удаления
 * @return     результат удаления
 */
bool awh::Poll::del(const uint32_t id, const SOCKET sock) noexcept {
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
				// Объект найденного пира
				poll_t::event_t * peer = nullptr;
				// Выполняем перебор всего списка пиров
				for(size_t i = 0; i < ::__awh_loop__->peers.size(); i++){
					// Получаем текущее значение пира
					peer = &::__awh_loop__->peers[i];
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
									this->_log->debug("Event loop model is corrupted, SOCKET=%d does not match SOCKET=%d", __PRETTY_FUNCTION__, std::make_tuple(sock, id), log_t::flag_t::WARNING, sock, ::__awh_loop__->sockets[i]);
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
						// Выполняем удаление пира
						::__awh_loop__->peers.erase(::__awh_loop__->peers.begin() + i);
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
				// Выполняем поиск нужного нам пира
				auto i = loop->peers.find(id);
				// Если нужный нам пир найден
				if((result = (i != loop->peers.end()))){
					// Выполняем проверку является ли событие таймером
					if((i->second.events & AWH_TIMER) || (i->second.events &  AWH_INTERVAL)){
						// Выполняем остановку работы таймера
						this->_watch.away(id);
						// Выполняем удаление пира
						loop->peers.erase(i);
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
									this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, id), log_t::flag_t::CRITICAL, ::strerror(errno));
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
							// Выполняем удаление пира
							loop->peers.erase(i);
							// Удаляем лишнее событие
							loop->events.pop_back();
							// Выполняем заполнение нулями всю структуру событий
							::memset(&loop->events[0], 0, loop->events.size() * sizeof(port_event_t));
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
								this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, id), log_t::flag_t::CRITICAL, ::strerror(errno));
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
						// Выполняем удаление пира
						loop->peers.erase(i);
						// Удаляем лишнее событие
						loop->events.pop_back();
						// Выполняем заполнение нулями всю структуру событий
						::memset(&loop->events[0], 0, loop->events.size() * sizeof(port_event_t));
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
					// Объект найденного пира
					poll_t::event_t * peer = nullptr;
					// Выполняем перебор всего списка пиров
					for(size_t i = 0; i < loop->peers.size(); i++){
						// Получаем текущее значение пира
						peer = &loop->peers[i];
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
							// Выполняем удаление пира
							loop->peers.erase(loop->peers.begin() + i);
							// Выполняем инициализацию /dev/poll
							if((loop->wfd = ::open("/dev/poll", O_RDWR, 0)) == INVALID_SOCKET){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, id), log_t::flag_t::CRITICAL, ::strerror(errno));
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
			// Выполняем поиск нужного нам пира
			auto i = ::__awh_loop__->peers.find(id);
			// Если нужный нам пир найден
			if((result = (i != ::__awh_loop__->peers.end()))){
				// Выполняем проверку является ли событие таймером
				if((i->second.events & AWH_TIMER) || (i->second.events &  AWH_INTERVAL)){
					// Выполняем остановку работы таймера
					this->_watch.away(id);
					// Выполняем удаление пира
					::__awh_loop__->peers.erase(i);
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
								this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, id), log_t::flag_t::CRITICAL, ::strerror(errno));
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
						// Выполняем удаление пира
						::__awh_loop__->peers.erase(i);
						// Удаляем лишнее событие
						::__awh_loop__->events.pop_back();
						// Выполняем заполнение нулями всю структуру событий
						::memset(&::__awh_loop__->events[0], 0, ::__awh_loop__->events.size() * sizeof(struct epoll_event));
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
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, id), log_t::flag_t::CRITICAL, ::strerror(errno));
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
					// Выполняем удаление пира
					::__awh_loop__->peers.erase(i);
					// Удаляем лишнее событие
					::__awh_loop__->events.pop_back();
					// Выполняем заполнение нулями всю структуру событий
					::memset(&::__awh_loop__->events[0], 0, ::__awh_loop__->events.size() * sizeof(struct epoll_event));
				}
			}
		/**
		 * Для операционной системы MacOS X, FreeBSD, NetBSD или OpenBSD
		 */
		#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
			// Выполняем поиск нужного нам пира
			auto i = ::__awh_loop__->peers.find(id);
			// Если нужный нам пир найден
			if((result = (i != ::__awh_loop__->peers.end()))){
				// Выполняем проверку является ли событие таймером
				if((i->second.events & AWH_TIMER) || (i->second.events &  AWH_INTERVAL)){
					// Выполняем остановку работы таймера
					this->_watch.away(id);
					// Выполняем удаление пира
					::__awh_loop__->peers.erase(i);
				// Если идентификатор принадлежит потоковому событию
				} else if(i->second.events & AWH_STREAM) {
					// Выполняем блокировку потока
					const lock_guard lock(::__awh_stream_mtx__);
					// Выполняем поиск нужного нам уведомителя
					auto j = ::__awh_notifiers__.find(i->first);
					// Если нужный нам уведомитель найден
					if(j != ::__awh_notifiers__.end()){
						// Создаём объект события
						struct kevent event;
						// Извлекаем сокет события
						const_cast <SOCKET &> (sock) = j->second->init();
						// Снимаем событие ожидания сокета готовности на чтение
						EV_SET(&event, sock, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
						// Выполняем изменение параметров события
						if(::kevent(::__awh_loop__->kq, &event, 1, nullptr, 0, nullptr) == INVALID_SOCKET){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, id), log_t::flag_t::CRITICAL, ::strerror(errno));
							/**
							* Если режим отладки не включён
							*/
							#else
								// Выводим сообщение об ошибке
								this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
							#endif
							// Выполняем сброс результата
							result = false;
						}
						// Снимаем событие ожидания сокета готовности на запись
						EV_SET(&event, sock, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
						// Выполняем изменение параметров события
						if(::kevent(::__awh_loop__->kq, &event, 1, nullptr, 0, nullptr) == INVALID_SOCKET){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, id), log_t::flag_t::CRITICAL, ::strerror(errno));
							/**
							* Если режим отладки не включён
							*/
							#else
								// Выводим сообщение об ошибке
								this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
							#endif
							// Выполняем сброс результата
							result = false;
						}
						// Выполняем удаление активного сокета
						::__awh_ids__.erase(sock);
						// Выполняем удаление уведомителя
						::__awh_notifiers__.erase(j);
						// Выполняем удаление пира
						::__awh_loop__->peers.erase(i);
						// Удаляем лишнее событие
						::__awh_loop__->events.pop_back();
						// Выполняем заполнение нулями всю структуру событий
						::memset(&::__awh_loop__->events[0], 0, ::__awh_loop__->events.size() * sizeof(struct kevent));
					}
				// Если сокет передан верно
				} else if(sock != INVALID_SOCKET) {
					// Создаём объект события
					struct kevent event;
					// Снимаем событие ожидания сокета готовности на чтение
					EV_SET(&event, sock, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
					// Выполняем изменение параметров события
					if(::kevent(::__awh_loop__->kq, &event, 1, nullptr, 0, nullptr) == INVALID_SOCKET){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, id), log_t::flag_t::CRITICAL, ::strerror(errno));
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
						#endif
						// Выполняем сброс результата
						result = false;
					}
					// Снимаем событие ожидания сокета готовности на запись
					EV_SET(&event, sock, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
					// Выполняем изменение параметров события
					if(::kevent(::__awh_loop__->kq, &event, 1, nullptr, 0, nullptr) == INVALID_SOCKET){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, id), log_t::flag_t::CRITICAL, ::strerror(errno));
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
						#endif
						// Выполняем сброс результата
						result = false;
					}
					// Выполняем удаление активного сокета
					::__awh_ids__.erase(sock);
					// Выполняем удаление пира
					::__awh_loop__->peers.erase(i);
					// Удаляем лишнее событие
					::__awh_loop__->events.pop_back();
					// Выполняем заполнение нулями всю структуру событий
					::memset(&::__awh_loop__->events[0], 0, ::__awh_loop__->events.size() * sizeof(struct kevent));
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
			this->_log->debug("Event with ID=%d could not be delete from the event database loop", __PRETTY_FUNCTION__, std::make_tuple(sock, id), log_t::flag_t::WARNING, id);
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
 * @brief Метод добавления сетевого события
 *
 * @param id     идентификатор сетевого события
 * @param sock   сетевой сокет для добавления
 * @param events поддерживаемые типы событий
 * @return       результат добавления
 */
bool awh::Poll::add(const uint32_t id, const SOCKET sock, const uint8_t events) noexcept {
	// Если данные для установки переданы и база событий инициализированна
	if((sock != INVALID_SOCKET) && (id > 0) && (events != AWH_NONE) && (::__awh_loop__ != nullptr)){
		// Если сокет ещё не добавлен в список для отслеживания
		if(::__awh_ids__.find(sock) == ::__awh_ids__.end()){
			// Если событие для отслеживания не передано
			if(events == AWH_NONE){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("For socket %d with ID=%d, no event was passed for tracking", __PRETTY_FUNCTION__, std::make_tuple(sock, id, events), log_t::flag_t::WARNING, sock, id);
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("For socket %d with ID=%d, no event was passed for tracking", log_t::flag_t::WARNING, sock, id);
				#endif
				// Выходим из функции
				return false;
			}
			// Если переданы события таймеров
			if((events & AWH_TIMER) || (events & AWH_INTERVAL)){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Timer tracking event cannot be activated for a network socket", __PRETTY_FUNCTION__, std::make_tuple(sock, id, events), log_t::flag_t::WARNING);
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
				this->_log->debug("Socket %d with ID=%d can no longer be added to the event loop database because it is already there", __PRETTY_FUNCTION__, std::make_tuple(sock, id, events), log_t::flag_t::WARNING, sock, id);
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
					this->_log->debug(L"%s", __PRETTY_FUNCTION__, std::make_tuple(sock, id, events), log_t::flag_t::CRITICAL, message);
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
			long options = 0;
			// Если установлен флаг ожидания закрытия подключения
			if(events & AWH_CLOSE)
				// Ставим флаг ожидания закрытия подключения
				options |= FD_CLOSE;
			// Если установлен флаг ожидания получения данных
			if(events & AWH_READ)
				// Ставим флаги ожидания готовности на чтение
				options |= (FD_READ | FD_ACCEPT);
			// Если установлен флаг ожидания сокета на запись
			if(events & AWH_WRITE)
				// Ставим флаги ожидания готовности на запись
				options |= (FD_WRITE | FD_CONNECT);
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
					this->_log->debug(L"%s", __PRETTY_FUNCTION__, std::make_tuplesock, id, events), log_t::flag_t::CRITICAL, message);
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
			// Устанавливаем наш таймер в список активных сокетов
			::__awh_loop__->sockets.push_back(sock);
			// Устанавливаем наше новое событие в список событий
			::__awh_loop__->events.push_back(::move(event));
			// Выполняем установку пустого значения пира
			::__awh_loop__->peers.push_back(poll_t::event_t{});
			// Устанавливаем идентификатор события
			::__awh_loop__->peers.back().id = id;
			// Если событие является потоком
			if(events & AWH_STREAM)
				// Устанавливаем тип события
				::__awh_loop__->peers.back().events = AWH_STREAM;
			// Устанавливаем соответствие идентификатора сокету
			return ::__awh_ids__.emplace(sock, id).second;
		/**
		 * Для операционной системы Sun Solaris
		 */
		#elif __sun__
			// Если активирована поддержка Event Ports
			if(::__awh_event_ports__){
				// Опции событий для установки
				int32_t options = 0;
				// Если установлен флаг ожидания закрытия подключения
				if(events & AWH_CLOSE)
					// Ставим флаг ожидания закрытия подключения
					options |= POLLHUP;
				// Если установлен флаг ожидания появления ошибки
				if(events & AWH_CLOSE)
					// Ставим флаг ожидания появления ошибки
					options |= POLLERR;
				// Если установлен флаг ожидания получения данных
				if(events & AWH_READ)
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
				// Выполняем добавление пира
				auto ret = loop->peers.emplace(id, poll_t::event_t{});
				// Если событие является потоком
				if(events & AWH_STREAM)
					// Устанавливаем тип события
					ret.first->second.events = AWH_STREAM;
				// Выполняем активацию отслеживания событий для сокета
				if(::port_associate(loop->wfd, PORT_SOURCE_FD, sock, options, &ret.first->second) != 0){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuplesock, id, events), log_t::flag_t::CRITICAL, ::strerror(errno));
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
					#endif
					// Удаляем добавленного пира
					loop->peers.erase(id);
					// Выходим из функции
					return false;
				}
				// Увеличиваем количество событий
				loop->events.push_back(port_event_t{});
			// Если активированна поддержка /dev/poll
			} else {
				// Выполняем создание объекта события
				struct pollfd event = {0};
				// Устанавливаем файловый дескриптор таймера
				event.fd = sock;
				// Выполняем сброс всех событий
				event.events = 0;
				// Если установлен флаг ожидания закрытия подключения
				if(events & AWH_CLOSE)
					// Ставим флаг ожидания закрытия подключения
					event.events |= POLLHUP;
				// Если установлен флаг ожидания появления ошибки
				if(events & AWH_CLOSE)
					// Ставим флаг ожидания появления ошибки
					event.events |= POLLERR;
				// Если установлен флаг ожидания получения данных
				if(events & AWH_READ)
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
				// Выполняем установку пустого значения пира
				loop->peers.push_back(poll_t::event_t{});
				// Устанавливаем идентификатор события
				loop->peers.back().id = id;
				// Если событие является потоком
				if(events & AWH_STREAM)
					// Устанавливаем тип события
					loop->peers.back().events = AWH_STREAM;
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
			event.events = 0;
			// Если установлен флаг ожидания закрытия подключения
			if(events & AWH_CLOSE)
				// Ставим флаг ожидания закрытия подключения
				event.events |= EPOLLHUP;
			// Если установлен флаг ожидания появления ошибки
			if(events & AWH_CLOSE)
				// Ставим флаг ожидания появления ошибки
				event.events |= EPOLLERR;
			// Если установлен флаг ожидания получения данных
			if(events & AWH_READ)
				// Ставим флаг ожидания готовности на чтение
				event.events |= EPOLLIN;
			// Если сокет не активирован на прослушку
			if(!this->_socket.listen(sock)){
				// Если установлен флаг ожидания сокета на запись
				if(events & AWH_WRITE)
					// Ставим флаг ожидания готовности на запись
					event.events |= EPOLLOUT;
			}
			// Выполняем добавление пира
			auto ret = loop->peers.emplace(id, poll_t::event_t{});
			// Если событие является потоком
			if(events & AWH_STREAM)
				// Устанавливаем тип события
				ret.first->second.events = AWH_STREAM;
			// Выполняем установку указателя на основное событие
			event.data.ptr = &ret.first->second;
			// Выполняем изменение параметров события
			if(::epoll_ctl(::__awh_loop__->efd, EPOLL_CTL_ADD, sock, &event) != 0){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuplesock, id, events), log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
				// Удаляем добавленного пира
				::__awh_loop__->peers.erase(id);
				// Выходим из функции
				return false;
			}
			// Увеличиваем количество событий
			::__awh_loop__->events.push_back((struct epoll_event){});
			// Устанавливаем соответствие идентификатора сокету
			return ::__awh_ids__.emplace(sock, id).second;
		/**
		 * Для операционной системы MacOS X, FreeBSD, NetBSD или OpenBSD
		 */
		#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
			// Создаём объект события
			struct kevent event;
			// Выполняем добавление пира
			auto ret = ::__awh_loop__->peers.emplace(id, poll_t::event_t{});
			// Если событие является потоком
			if(events & AWH_STREAM)
				// Устанавливаем тип события
				ret.first->second.events = AWH_STREAM;
			// Если установлен флаг ожидания получения данных
			if(events & AWH_READ)
				// Ставим флаг ожидания готовности на чтение
				EV_SET(&event, sock, EVFILT_READ, EV_ADD | EV_CLEAR | EV_ENABLE, 0, 0, &ret.first->second);
			// Если сокет не активирован на прослушку
			if(!this->_socket.listen(sock)){
				// Если установлен флаг ожидания сокета на запись
				if(events & AWH_WRITE)
					// Ставим флаг ожидания готовности на запись
					EV_SET(&event, sock, EVFILT_WRITE, EV_ADD | EV_CLEAR | EV_ENABLE, 0, 0, &ret.first->second);
			}
			// Устанавливаем наше новое событие в список событий
			::__awh_loop__->events.push_back(::move(event));
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
			this->_log->debug("Socket %d with ID=%d could not be added to the event database loop", __PRETTY_FUNCTION__, std::make_tuple(sock, id, events), log_t::flag_t::WARNING, sock, id);
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
 * @brief Метод добавления несетевых событий
 *
 * @param id     идентификатор несетевого события
 * @param events поддерживаемые типы событий
 * @param msec   время ожидания срабатывания в миллисекундах
 * @return       результат добавления
 */
bool awh::Poll::add(const uint32_t id, const uint8_t events, const uint32_t msec) noexcept {
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
							// Выполняем добавление пира
							auto ret = loop->peers.emplace(id, poll_t::event_t{});
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
						// Выполняем добавление пира
						auto ret = ::__awh_loop__->peers.emplace(id, poll_t::event_t{});
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
						this->_log->debug("You cannot initialize a stream with ID=%d in a child thread", __PRETTY_FUNCTION__, std::make_tuple(id, msec, events), log_t::flag_t::WARNING, id);
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
					return this->add(id, sock, AWH_READ | AWH_STREAM);
			}
		// Если нам передали для установки событие для сетевого сокета
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("There are events for a network socket being sent for tracking, but you want to set a timer or thread", __PRETTY_FUNCTION__, std::make_tuple(id, msec, events), log_t::flag_t::WARNING);
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
			this->_log->debug("Event with ID=%d could not be added to the event database loop", __PRETTY_FUNCTION__, std::make_tuple(id, msec, events), log_t::flag_t::WARNING, id);
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
 * @param events список сработавших событий
 * @param max    максимальное количество ожидаемых событий
 * @param msec   время ожидания события в миллисекундах
 * @return       количество полученных событий
 */
uint32_t awh::Poll::wait(event_t * events, const uint16_t max, const int32_t msec) noexcept {
	/**
	 * Для операционной системы Sun Solaris
	 */
	#if __sun__
		// Если активированна поддержка /dev/poll
		if(!::__awh_event_ports__){
			// Выполняем получение объекта Event Loop
			EventLoop2 * loop = dynamic_cast <EventLoop2 *> (::__awh_loop__.get());
			// Если перед началом работы необходимо зафиксировать изменения
			if(!loop->events.empty() && loop->commit){
				// Снимаем флаг фиксации изменений
				loop->commit = false;
				// Выполняем блокировку потока
				const lock_guard lock(::__awh_main_mtx__);
				// Получаем размер записываемых данных
				const size_t size = (sizeof(struct pollfd) * loop->events.size());
				// Отправляем ядру операционной системы только активные сокеты
				if(::write(loop->wfd, &loop->events[0], size) != size){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(max, msec), log_t::flag_t::CRITICAL, ::strerror(errno));
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
	 * Для операционной системы MacOS X, FreeBSD, NetBSD или OpenBSD
	 */
	#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
		// Если в базе событий есть добавленные сокеты
		if(!::__awh_loop__->events.empty()){
			// Выполняем блокировку потока
			const lock_guard lock(::__awh_main_mtx__);
			// Выполняем изменение параметров события
			if(::kevent(::__awh_loop__->kq, &::__awh_loop__->events[0], ::__awh_loop__->events.size(), nullptr, 0, nullptr) == INVALID_SOCKET){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(max, msec), log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
			}
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
				events[0].events = AWH_TIMEOUT;
				// Выводим событие таймаута
				return 1;
			}
			// Если мы получили ошибку сокета
			if(index == WSA_WAIT_FAILED){
				// Устанавливаем событие ошибки
				events[0].events = AWH_ERROR;
				// Выводим значение ошибки
				return 1;
			}
			// Получаем смещение в списоке событий
			const uint32_t offset = (index - WSA_WAIT_EVENT_0);
			// Выполняем получение сокета
			const SOCKET sock = ::__awh_loop__->sockets[offset];
			// Если сокет соответствует таймеру
			if(sock == ::__awh_timer__){
				// Получаем идентификатор таймера
				events[0].id = this->_watch.event();
				// Устанавливаем событие Таймера
				events[0].events = AWH_TIMER;
				// Выполняем блокировку потока
				const lock_guard lock(::__awh_main_mtx__);
				// Выполняем проверку является ли таймер персистентным
				auto i = ::__awh_loop__->timers.find(events[0].id);
				// Если мы нашли нужный нам таймер
				if(i != ::__awh_loop__->timers.end()){
					// Если таймер является персистентным
					if(i->second.second){
						// Устанавливаем событие таймера как Интервал
						events[0].events = AWH_INTERVAL;
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
				poll_t::event_t & event = ::__awh_loop__->peers[offset];
				// Устанавливаем идентификатор события
				events[0].id = event.id;
				// Если событие является потоком
				if(event.events & AWH_STREAM)
					// Заменяем флаг события
					events[0].events = AWH_STREAM;
				// Если мы получили обычное событие
				else {
					// Объект статуса события сокета
					WSANETWORKEVENTS status;
					// Если мы статус событий не смогли прочитать
					if(::WSAEnumNetworkEvents(sock, ::__awh_loop__->events[offset], &status) == SOCKET_ERROR)
						// Устанавливаем событие ошибки
						events[0].events = AWH_ERROR;
					// Если мы удачно извлекли события
					else {
						// Выполняем сброс событий
						events[0].events = AWH_NONE;
						// Если мы детектировали закрытие подключения
						if(status.lNetworkEvents & FD_CLOSE)
							// Устанавливаем флаг события
							events[0].events |= AWH_CLOSE;
						// Если мы детектировали событие готовности сокета на чтение данных
						if(status.lNetworkEvents & (FD_READ | FD_ACCEPT))
							// Устанавливаем флаг события
							events[0].events |= AWH_READ;
						// Если мы детектировали событие готовности сокета на запись данных
						if(status.lNetworkEvents & (FD_WRITE | FD_CONNECT))
							// Устанавливаем флаг события
							events[0].events |= AWH_WRITE;
						// Если мы детектировали наличие ошибки
						if(status.iErrorCode[FD_CLOSE_BIT] != 0)
							// Устанавливаем флаг события
							events[0].events |= AWH_ERROR;
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

	/**
	 * Для операционной системы Linux
	 */
	#elif __linux__

	/**
	 * Для операционной системы MacOS X, FreeBSD, NetBSD или OpenBSD
	 */
	#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__

	#endif
	// Если время таймаута передано
	if(msec > 0)
		// Замораживаем поток на период времени частоты обновления базы событий
		this_thread::sleep_for(chrono::milliseconds(msec));
	// Если время таймаута не передано, замораживаем на 100 миллисекунд
	else this_thread::sleep_for(100ms);
	// Устанавливаем событие таймаута
	events[0].events = AWH_TIMEOUT;
	// Выводим событие таймаута
	return 1;
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Poll::Poll(const fmk_t * fmk, const log_t * log) noexcept :
 _watch(fmk, log), _socket(fmk, log), _fmk(fmk), _log(log) {
	// Устанавливаем текущий идентификатор потока
	::__awh_wid__ = ::wid();
}
/**
 * @brief Деструктор
 *
 */
awh::Poll::~Poll() noexcept {
	// Выполняем разрушение событийной модели
	this->destroy();
}
