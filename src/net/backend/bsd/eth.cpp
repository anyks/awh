/**
 * @file: eth.cpp
 * @date: 2025-11-06
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация бэкенда сетевого уровня Ethernet — объединение работы с интерфейсами, шлюзами, маршрутами,
 *        пробросом портов и сокетами канального уровня с ветвлением под конкретную операционную систему
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Если максимальное количество файловых дескрипторов не передано
 */
#ifndef AWH_MAX_COUNT_FDS
	/**
	 * Устанавливаем максимальное количество доступных файловых дескрипторов 131072
	 */
	#define AWH_MAX_COUNT_FDS 0x20000
#endif

/**
 * Стандартные заголовочные файлы
 */
#include <cerrno>
#include <cstring>
#include <cstdlib>

/**
 * Системный заголовочный файл
 */
#include <sys/resource.h>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/os.hpp>
#include <net/fds.hpp>
#include <net/eth/eth.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Инкапсулируем статические прототипы функций в пространство имён работы с сетевыми параметрами
 *
 */
namespace options {
	/**
	 * @brief Метод применения сетевой оптимизации операционной системы
	 *
	 * @param fmk объект фреймворка
	 * @param log объект работы с логами
	 *
	 */
	static void netboost(const awh::fmk_t * fmk, const awh::log_t * log) noexcept {
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем инициализацию объекта работы с операционной системы
			awh::os_t os(log);
			// Выполняем инициализацию объекта работы с файловыми дескрипторами
			awh::fds_t fds(log);
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Структура лимитов дампов
				struct rlimit limit;
				// Устанавливаем текущий лимит равный бесконечности
				limit.rlim_cur = RLIM_INFINITY;
				// Устанавливаем максимальный лимит равный бесконечности
				limit.rlim_max = RLIM_INFINITY;
				// Возвращаем результат установки лимита дампов ядра
				if(::setrlimit(RLIMIT_CORE, &limit) != 0){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						log->debug("%s", __PRETTY_FUNCTION__, {}, awh::log_t::flag_t::WARNING, ::strerror(errno));
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
					#endif
				}
			#endif
			/**
			 * Выполняем установку нужного нам количества файловых дескрипторов
			 */
			if(!fds.limit(AWH_MAX_COUNT_FDS)){
				// Получаем лимиты файловых дескрипторов
				const auto & limits = fds.limit();
				// Если текущий лимит меньше желаемого
				if(limits.first < AWH_MAX_COUNT_FDS)
					// Записываем в лог сообщение подсказки
					fds.help(limits.first, AWH_MAX_COUNT_FDS);
			}
			/**
			 * Если необходимо выполнить тюннинг операционной системы
			 */
			#if AWH_BOOSTING_NET
				// Если эффективный идентификатор пользователя не принадлежит ROOT
				if(!os.isAdmin()){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						log->debug("Root privileges are required to apply network optimizations", __PRETTY_FUNCTION__, {}, awh::log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						log->print("Root privileges are required to apply network optimizations", awh::log_t::flag_t::WARNING);
					#endif
				// Если права суперпользователя получены
				} else {
					/**
					 * Для операционной системы macOS
					 *
					 * Названия проверены на macOS 26.5
					 */
					#if __APPLE__ || __MACH__
						// Устанавливаем максимальное количество подключений
						os.sysctl("kern.ipc.somaxconn", 49152);
						/**
						 * Для хостов 10G было бы неплохо увеличить это значение,
						 * т.к. 4G, похоже, является пределом для некоторых установок macOS
						 */
						os.sysctl("kern.ipc.maxsockbuf", 16777216);
						// Увеличиваем максимальный размер буферов для отправки
						os.sysctl("net.inet.tcp.sendspace", 1042560);
						// Увеличиваем максимальный размер буферов для чтения
						os.sysctl("net.inet.tcp.recvspace", 1042560);
						/**
						 * Увеличиваем множитель масштабирования окна: у macOS он равен 3, что
						 * держит окно в пределах четверти мегабайта и режет пропускную
						 * способность на путях с большой задержкой
						 *
						 * @warning Прежде здесь стояло имя "net.inet.tcp.r", которого не
						 *          существует: замер на macOS 26.5 отвечает unknown oid. Судя
						 *          по пояснению рядом - "значение по умолчанию 3, что очень
						 *          мало", - имелось в виду именно это имя, усечённое при переносе
						 */
						os.sysctl("net.inet.tcp.win_scale_factor", 8);
						// Увеличиваем максимумы автонастройки macOS TCP
						os.sysctl("net.inet.tcp.autorcvbufmax", 33554432);
						os.sysctl("net.inet.tcp.autosndbufmax", 33554432);
						/**
						 * Увеличиваем начальное окно для подключений внутри машины
						 *
						 * @warning Соседнее "net.inet.tcp.slowstart_flightsize" убрано: замер
						 *          на macOS 26.5 отвечает unknown oid
						 */
						os.sysctl("net.inet.tcp.local_slowstart_flightsize", 20);
					/**
					 * Для операционной системы FreeBSD
					 *
					 * Данные оптимизаций берутся отсюда: http://fasterdata.es.net/host-tuning/freebsd
					 * Названия проверены на FreeBSD 14.1-RELEASE
					 */
					#elif __FreeBSD__
						// Активируем контроль работы временной марки и масштабируемого окна
						os.sysctl("net.inet.tcp.rfc1323", 1);
						// Устанавливаем максимальное количество подключений
						os.sysctl("kern.ipc.somaxconn", 49152);
						// Активируем автоматическую отправку и получение
						os.sysctl("net.inet.tcp.sendbuf_auto", 1);
						os.sysctl("net.inet.tcp.recvbuf_auto", 1);
						/**
						 * Увеличиваем размер шага автонастройки
						 *
						 * @warning Соседнее "net.inet.tcp.recvbuf_inc" убрано: замер на
						 *          FreeBSD 14.1 отвечает unknown oid, шаг приёма отдельной
						 *          настройкой там больше не задаётся
						 */
						os.sysctl("net.inet.tcp.sendbuf_inc", 8192);
						/**
						 * Активируем на хостах тестирования/измерений
						 *
						 * @warning Соседнее "net.inet.tcp.inflight.enable" убрано: замер на
						 *          FreeBSD 14.1 отвечает unknown oid, ограничение объёма данных
						 *          в пути снято из ядра вместе с настройкой
						 */
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
						/**
						 * Выполняем выбор источника времени
						 *
						 * @details Под гипервизором ядро выбирает паравиртуальные часы, а с
						 *          ними быстрый путь чтения времени из пространства пользователя
						 *          не работает: всякое обращение к часам уходит в ядро. Замер на
						 *          стенде FreeBSD 14 в виртуальной машине KVM даёт при kvmclock
						 *          613.8 наносекунды на обращение и 536.9 у дешёвых часов, а при
						 *          TSC-low - 32.5 и 7.9 соответственно. Разница шестидесятивосьмикратная,
						 *          и берётся она не из удешевления перехода, а из его исчезновения
						 *
						 *          Задето это всё, где время берётся на операцию: сроки ожидания,
						 *          таймеры, отметки времени в журнале
						 *
						 * @warning Выбор этот отменяет собственную оценку ядра: TSC помечен им
						 *          низким качеством намеренно, ибо под гипервизором счётчик не
						 *          всегда единообразен между ядрами. Оттого источник и не
						 *          назначается вслепую, а берётся лишь тогда, когда ядро само
						 *          выносит его в перечень доступных - отсутствие в перечне
						 *          означает, что ядро сочло его негодным вовсе
						 */
						const string & timecounter = os.sysctl <string> ("kern.timecounter.choice");
						// Если перечень доступных источников времени получен
						if(!timecounter.empty()){
							// Если доступен счётчик тактов с пониженной частотой
							if(fmk->exists("TSC-low", timecounter))
								// Выполняем выбор счётчика тактов с пониженной частотой
								os.sysctl("kern.timecounter.hardware", "TSC-low");
							// Если же доступен счётчик тактов
							else if(fmk->exists("TSC", timecounter))
								// Выполняем выбор счётчика тактов
								os.sysctl("kern.timecounter.hardware", "TSC");
						}
					/**
					 * Для операционной системы NetBSD
					 *
					 * Названия проверены на NetBSD 10. Ветвь настроек здесь своя: общего
					 * потолка "kern.ipc.maxsockbuf" у NetBSD нет вовсе, его место занимает
					 * "kern.sbmax", и поднимать его следует **первым** - пределы буферов TCP
					 * выше него не встанут
					 */
					#elif __NetBSD__
						// Поднимаем общий потолок буферов сокета
						os.sysctl("kern.sbmax", 16777216);
						// Активируем контроль работы временной марки и масштабируемого окна
						os.sysctl("net.inet.tcp.rfc1323", 1);
						// Разрешаем использование временных меток
						os.sysctl("net.inet.tcp.timestamps", 1);
						// Разрешаем выборочные подтверждения
						os.sysctl("net.inet.tcp.sack.enable", 1);
						// Активируем автоматическую отправку и получение
						os.sysctl("net.inet.tcp.sendbuf_auto", 1);
						os.sysctl("net.inet.tcp.recvbuf_auto", 1);
						// Увеличиваем размер шага автонастройки
						os.sysctl("net.inet.tcp.sendbuf_inc", 8192);
						os.sysctl("net.inet.tcp.recvbuf_inc", 16384);
						// Увеличиваем максимальный размер буферов для отправки
						os.sysctl("net.inet.tcp.sendbuf_max", 16777216);
						// Увеличиваем максимальный размер буферов для чтения
						os.sysctl("net.inet.tcp.recvbuf_max", 16777216);
						// Увеличиваем размеры буферов, устанавливаемых по умолчанию
						os.sysctl("net.inet.tcp.sendspace", 1042560);
						os.sysctl("net.inet.tcp.recvspace", 1042560);
						// Увеличиваем начальное окно до предписанного RFC 6928
						os.sysctl("net.inet.tcp.init_win", 10);
						/**
						 * Алгоритмы у NetBSD перечисляются в net.inet.tcp.congctl.available,
						 * а выбор ведётся через net.inet.tcp.congctl.selected
						 */
						const string & algorithm = os.sysctl <string> ("net.inet.tcp.congctl.available");
						// Если выбран лучший доступны алгоритм
						if(!algorithm.empty()){
							// Если найден алгоритм cubic
							if(fmk->exists("cubic", algorithm))
								// Активируем выбранный нами алгоритм
								os.sysctl("net.inet.tcp.congctl.selected", "cubic");
							// Если же найден алгоритм newreno
							else if(fmk->exists("newreno", algorithm))
								// Активируем выбранный нами алгоритм
								os.sysctl("net.inet.tcp.congctl.selected", "newreno");
						}
						/**
						 * Выполняем выбор источника времени
						 *
						 * @details Под гипервизором ядро выбирает hpet0, а этот источник читается
						 *          обращением к устройству, и всякое такое чтение проваливается в
						 *          гипервизор. Замер на стенде NetBSD 10 в виртуальной машине даёт
						 *          при hpet0 13161 наносекунду на обращение к часам против 725 при
						 *          TSC - в восемнадцать раз. Проверено переключением туда и обратно
						 *
						 *          Быстрого пути чтения времени из пространства пользователя у
						 *          NetBSD нет и с TSC: обращение к ядру остаётся, и оставшиеся 725
						 *          наносекунд - его цена. Выбор источника снимает лишь надбавку
						 *          сверх неё, но надбавка эта и составляет девяносто пять сотых
						 *          всей стоимости
						 *
						 * @warning Выбор этот отменяет собственную оценку ядра: TSC помечен им
						 *          низким качеством намеренно, ибо под гипервизором счётчик не
						 *          всегда единообразен между ядрами. Оттого источник и не
						 *          назначается вслепую, а берётся лишь тогда, когда ядро само
						 *          выносит его в перечень доступных - отсутствие в перечне
						 *          означает, что ядро сочло его негодным вовсе
						 */
						const string & timecounter = os.sysctl <string> ("kern.timecounter.choice");
						// Если перечень доступных источников времени получен и счётчик тактов доступен
						if(!timecounter.empty() && fmk->exists("TSC", timecounter))
							// Выполняем выбор счётчика тактов
							os.sysctl("kern.timecounter.hardware", "TSC");
					/**
					 * Для операционной системы OpenBSD
					 *
					 * Названия проверены на OpenBSD 7.9. Настроек здесь намеренно мало:
					 * размеры буферов сокета OpenBSD подбирает сам и наружу этого не выносит,
					 * а потому ни "sendspace", ни "recvspace", ни пределов автонастройки, ни
					 * выбора алгоритма перегрузки там нет вовсе. Прежде сюда прилагалась
					 * ветвь FreeBSD целиком, и из десяти её настроек ложилась ровно одна -
					 * прочие отвергались молча
					 */
					#elif __OpenBSD__
						/**
						 * Устанавливаем максимальное количество подключений
						 *
						 * @warning Предел здесь 32767, и значение выше отвергается целиком:
						 *          замер на OpenBSD 7.9 отвечает Invalid argument уже на 65535.
						 *          Принятое у прочих систем 49152 сюда не годится
						 */
						os.sysctl("kern.somaxconn", 32767);
						// Активируем контроль работы временной марки и масштабируемого окна
						os.sysctl("net.inet.tcp.rfc1323", 1);
						// Разрешаем выборочные подтверждения
						os.sysctl("net.inet.tcp.sack", 1);
						// Активируем увеличенное начальное окно по RFC 3390
						os.sysctl("net.inet.tcp.rfc3390", 2);
						/**
						 * Источник времени здесь не выбирается намеренно
						 *
						 * @note Счётчика тактов OpenBSD в перечень доступных источников не
						 *       выносит вовсе: замер на стенде под гипервизором даёт
						 *       "i8254(0) pvclock0(1500) acpihpet0(1000) acpitimer0(1000)",
						 *       и лучший из них - паравиртуальные часы - ядром уже и выбран.
						 *       Выбирать нечего, а обращение к часам стоит там 711 наносекунд
						 *       против 725 у NetBSD с TSC - то есть настройкой это не поднять
						 */
					#endif
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
				// Записываем ошибку в лог
				log->debug("%s", __PRETTY_FUNCTION__, {}, awh::log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log->print("%s", awh::log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
};

/**
 * Если операционной системой является FreeBSD
 */
#if __FreeBSD__
	/**
	 * @brief Конструктор
	 *
	 * @param fmk объект фреймворка
	 * @param log объект работы с логами
	 *
	 */
	awh::Ethernet::Ethernet(const fmk_t * fmk, const log_t * log) noexcept :
	 addr(fmk, log), iface(fmk, log), sctp(fmk, log), socket(fmk, log),
	 gateway(fmk, log), _fmk(fmk), _log(log) {
		/**
		 * Связываем объект работы с адресами с объектом управления шлюзами: исходящий
		 * адрес определяется подбором маршрута, а подбор ведёт объект шлюзов
		 */
		this->addr.gateway(&this->gateway);
		/**
		 * Выполняем настройку сетевых параметров
		 */
		::options::netboost(fmk, log);
	}
/**
 * Для остальных операционных систем
 */
#else
	/**
	 * @brief Конструктор
	 *
	 * @param fmk объект фреймворка
	 * @param log объект работы с логами
	 *
	 */
	awh::Ethernet::Ethernet(const fmk_t * fmk, const log_t * log) noexcept :
	 addr(fmk, log), iface(fmk, log), socket(fmk, log),
	 gateway(fmk, log), _fmk(fmk), _log(log) {
		/**
		 * Связываем объект работы с адресами с объектом управления шлюзами: исходящий
		 * адрес определяется подбором маршрута, а подбор ведёт объект шлюзов
		 */
		this->addr.gateway(&this->gateway);
		/**
		 * Выполняем настройку сетевых параметров
		 */
		::options::netboost(fmk, log);
	}
#endif
/**
 * @brief Деструктор
 *
 */
awh::Ethernet::~Ethernet() noexcept {}
