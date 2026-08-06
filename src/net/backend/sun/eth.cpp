/**
 * @file: eth.cpp
 * @date: 2026-08-06
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация бэкенда сетевого уровня Ethernet под операционные системы Solaris
 *        и illumos — объединение работы с интерфейсами, шлюзами, маршрутами, пробросом
 *        портов и сокетами канального уровня
 *
 * @copyright: Copyright © 2026
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
	 * @details Настройки сетевого уровня у Solaris и illumos задаются не через
	 *          `sysctl`, которого там нет вовсе, а через средство `ndd`, обращающееся
	 *          прямо к устройству протокола: `/dev/tcp`
	 *
	 * @par Намеренные решения
	 *
	 *      **Выбрано `ndd`, а не `ipadm`.** У Solaris 11.4 средство `ndd` считается
	 *      устаревшим, и рекомендуется `ipadm set-prop`. Однако замер на стендах
	 *      показал, что названия свойств у `ipadm` **расходятся**: Solaris 11.4 зовёт
	 *      его `max-buf`, illumos - `max_buf`. У `ndd` же имя одно на обе системы -
	 *      `tcp_max_buf`, - и обе отвечают на установку успехом
	 *
	 *      Одно средство на две системы против двух наборов имён - выбор в пользу
	 *      `ndd`. Когда Solaris уберёт его совсем, ветвь придётся разделить, но
	 *      делать это заранее, ради средства, которое работает, нет причины
	 *
	 * @note Проверено на Solaris 11.4.90 и illumos (OpenIndiana)
	 *
	 * @param fmk объект фреймворка
	 * @param log объект работы с логами
	 *
	 */
	static void netboost([[maybe_unused]] const awh::fmk_t * fmk, const awh::log_t * log) noexcept {
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
					 * Поднимаем общий потолок буферов сокета: он ограничивает всё
					 * остальное, и задавать его следует первым
					 */
					os.exec("ndd -set /dev/tcp tcp_max_buf 16777216");
					/**
					 * Эмпирическое правило: наибольшее окно перегрузки берут вдвое
					 * меньше общего потолка буферов
					 */
					os.exec("ndd -set /dev/tcp tcp_cwnd_max 8388608");
					// Увеличиваем размер буфера отправки, устанавливаемый по умолчанию
					os.exec("ndd -set /dev/tcp tcp_xmit_hiwat 1042560");
					// Увеличиваем размер буфера приёма, устанавливаемый по умолчанию
					os.exec("ndd -set /dev/tcp tcp_recv_hiwat 1042560");
					/**
					 * Устанавливаем максимальное количество подключений: у Solaris и
					 * illumos очередь разделена надвое - установленные подключения
					 * ждут в одной, полуоткрытые в другой
					 */
					os.exec("ndd -set /dev/tcp tcp_conn_req_max_q 49152");
					os.exec("ndd -set /dev/tcp tcp_conn_req_max_q0 49152");
					// Разрешаем масштабирование окна независимо от запроса другой стороны
					os.exec("ndd -set /dev/tcp tcp_wscale_always 1");
					// Разрешаем выборочные подтверждения независимо от запроса другой стороны
					os.exec("ndd -set /dev/tcp tcp_sack_permitted 2");
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
 * @note Протокол с управлением потоком у Solaris и illumos есть, и объект работы с
 *       ним создаётся здесь наравне с прочими
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
