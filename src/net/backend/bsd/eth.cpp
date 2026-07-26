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
 * @copyright: Copyright © 2025
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
				/**
				 * Для операционной системы macOS
				 */
				#if __APPLE__ || __MACH__
					// Если эффективный идентификатор пользователя принадлежит ROOT
					if(os.isAdmin()){
						// Устанавливаем максимальное количество подключений
						os.sysctl("kern.ipc.somaxconn", 49152);
						/**
						 * Для хостов 10G было бы неплохо увеличить это значение,
						 * т.к. 4G, похоже, является пределом для некоторых установок macOS
						 */
						os.sysctl("kern.ipc.maxsockbuf", 6291456);
						// Увеличиваем максимальный размер буферов для отправки
						os.sysctl("net.inet.tcp.sendspace", 1042560);
						// Увеличиваем максимальный размер буферов для чтения
						os.sysctl("net.inet.tcp.recvspace", 1042560);
						// В macOS значение по умолчанию 3, что очень мало
						os.sysctl("net.inet.tcp.r", 8);
						// Увеличиваем максимумы автонастройки macOS TCP
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
							// Записываем ошибку в лог
							log->debug("Root privileges are required to apply network optimizations", __PRETTY_FUNCTION__, {}, awh::log_t::flag_t::WARNING);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log->print("Root privileges are required to apply network optimizations", awh::log_t::flag_t::WARNING);
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
						// Активируем нормальное TCP Reno
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
							// Записываем ошибку в лог
							log->debug("Root privileges are required to apply network optimizations", __PRETTY_FUNCTION__, {}, awh::log_t::flag_t::WARNING);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log->print("Root privileges are required to apply network optimizations", awh::log_t::flag_t::WARNING);
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
	 */
	awh::Ethernet::Ethernet(const fmk_t * fmk, const log_t * log) noexcept :
	 addr(fmk, log), iface(fmk, log), sctp(fmk, log), socket(fmk, log),
	 gateway(fmk, log), portmap(fmk, log), _fmk(fmk), _log(log) {
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
	 */
	awh::Ethernet::Ethernet(const fmk_t * fmk, const log_t * log) noexcept :
	 addr(fmk, log), iface(fmk, log), socket(fmk, log),
	 gateway(fmk, log), portmap(fmk, log), _fmk(fmk), _log(log) {
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
