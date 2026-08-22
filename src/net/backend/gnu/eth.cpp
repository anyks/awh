/**
 * @file eth.cpp
 * @date 2026-08-06
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * @brief Реализация бэкенда сетевого уровня Ethernet под операционную систему Linux —
 *        объединение работы с интерфейсами, шлюзами, маршрутами, пробросом портов и
 *        сокетами канального уровня
 *
 * @copyright Copyright © 2026
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
#include <memory>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <iostream>

/**
 * Системный заголовочный файл
 *
 * @note Нужен ради `mallopt`, а тот зовётся лишь при снятом своём распределителе
 */
#if defined(AWH_ALLOC_DISABLED)
	#include <malloc.h>
#endif

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
	 * @details Настройки берутся отсюда: http://fasterdata.es.net/host-tuning/linux
	 *
	 * @note Названия у Linux свои и с BSD не совпадают ни одним: там ветвь
	 *       `net.inet.tcp`, здесь `net.ipv4.tcp` и `net.core`. Пределы буферов
	 *       задаются порознь - общий потолок в `net.core`, а пределы самого TCP
	 *       набором из трёх чисел в `net.ipv4.tcp_rmem` и `net.ipv4.tcp_wmem`
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
				/**
				 * Задаём правила освобождения памяти: обмен ведётся буферами, и переход на
				 * отображение памяти при каждом крупном запросе обходится дороже удержания
				 * уже взятого
				 *
				 * @note Средство это принадлежит glibc, системным вызовом оно не является:
				 *       ни macOS, ни BSD его не предоставляют - там нет и заголовка malloc.h
				 *
				 * @note Зовётся, лишь когда выдачу памяти ведёт glibc. У систем ELF её ведём
				 *       МЫ - наш файл определяет знак `malloc`, и распределитель встаёт на
				 *       место системного самим связыванием. Настраивать тогда нечего:
				 *       `mallopt` правит внутренности той кучи glibc, к какой в процессе
				 *       никто не обращается, и работы от него ноль. Своего же
				 *       распределителя здесь НЕ настраиваем намеренно: настройки его -
				 *       одни на процесс, и движок, задавая их за приложение, затёр бы
				 *       заданное им раньше. Правила возврата памяти задаются через
				 *       `Allocator::options` тем, кто заводит приложение
				 */
				#if defined(AWH_ALLOC_DISABLED)
					::mallopt(M_MMAP_THRESHOLD, 64 * 1024);
					::mallopt(M_TRIM_THRESHOLD, 128 * 1024);
				#endif
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
					// Разрешаем масштабирование окна
					os.sysctl("net.ipv4.tcp_window_scaling", 1);
					// Разрешаем выборочные подтверждения
					os.sysctl("net.ipv4.tcp_sack", 1);
					// Разрешаем использование временных меток
					os.sysctl("net.ipv4.tcp_timestamps", 1);
					// Разрешаем повторное использование подключений в состоянии ожидания
					os.sysctl("net.ipv4.tcp_tw_reuse", 1);
					// Включаем автоматическую настройку размера приёмного буфера
					os.sysctl("net.ipv4.tcp_moderate_rcvbuf", 1);
					/**
					 * Запрещаем сохранять итоги измерений подключения при его закрытии: иначе
					 * одно неудачное подключение портит оценку всем последующим, идущим к той
					 * же сети
					 */
					os.sysctl("net.ipv4.tcp_no_metrics_save", 1);
					// Устанавливаем максимальное количество подключений
					os.sysctl("net.core.somaxconn", 49152);
					// Устанавливаем максимальное количество полуоткрытых подключений
					os.sysctl("net.ipv4.tcp_max_syn_backlog", 49152);
					// Увеличиваем очередь пакетов, не разобранных обработчиком протокола
					os.sysctl("net.core.netdev_max_backlog", 30000);
					/**
					 * Для хостов 10G было бы неплохо увеличить это значение,
					 * т.к. 4G, похоже, является пределом для некоторых установок Linux
					 */
					os.sysctl("net.core.rmem_max", 16777216);
					os.sysctl("net.core.wmem_max", 16777216);
					// Увеличиваем размеры буферов, устанавливаемых по умолчанию
					os.sysctl("net.core.rmem_default", 1042560);
					os.sysctl("net.core.wmem_default", 1042560);
					/**
					 * Пределы буферов самого TCP задаются набором из трёх чисел: наименьший
					 * размер, размер по умолчанию и наибольший
					 *
					 * @warning Передавать их строкой в кавычках нельзя: кавычки уходят в
					 *          /proc/sys дословно, и ядро отвергает запись целиком. Замер на
					 *          Fedora 44: в кавычках отказ и прежнее значение, без кавычек
					 *          значение встаёт. Здесь набор передаётся вектором, а разделение
					 *          пробелом берёт на себя объект работы с операционной системой
					 */
					os.sysctl("net.ipv4.tcp_rmem", vector <int32_t> {4096, 1042560, 16777216});
					os.sysctl("net.ipv4.tcp_wmem", vector <int32_t> {4096, 1042560, 16777216});
					// Активируем подбор наибольшего размера пакета, проходящего без дробления
					os.sysctl("net.ipv4.tcp_mtu_probing", 1);
					/**
					 * Выставляем справедливую очередь отправки: алгоритм bbr рассчитан именно
					 * на неё и без неё работает вполсилы
					 */
					os.sysctl("net.core.default_qdisc", "fq");
					/**
					 * Вы можете проверить, какие доступны алгоритмы получения доступных сообщений,
					 * используя net.ipv4.tcp_available_congestion_control
					 */
					const string & algorithm = os.sysctl <string> ("net.ipv4.tcp_available_congestion_control");
					// Если выбран лучший доступны алгоритм
					if(!algorithm.empty()){
						// Если найден алгоритм bbr
						if(fmk->exists("bbr", algorithm))
							// Активируем выбранный нами алгоритм
							os.sysctl("net.ipv4.tcp_congestion_control", "bbr");
						// Если же найден алгоритм cubic
						else if(fmk->exists("cubic", algorithm))
							// Активируем выбранный нами алгоритм
							os.sysctl("net.ipv4.tcp_congestion_control", "cubic");
						// Если же найден алгоритм htcp
						else if(fmk->exists("htcp", algorithm))
							// Активируем выбранный нами алгоритм
							os.sysctl("net.ipv4.tcp_congestion_control", "htcp");
					}
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
 * @brief Конструктор
 *
 * @note Протокол с управлением потоком у Linux есть, и объект работы с ним
 *       создаётся здесь наравне с прочими
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
 * @brief Деструктор
 *
 */
awh::Ethernet::~Ethernet() noexcept {}
