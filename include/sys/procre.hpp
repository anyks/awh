/**
 * @file procre.hpp
 * @date 2026-01-26
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
 * \~russian
 * @brief Заголовочный файл модуля резольвера процессов — класс Process_Resolver,
 *        сопоставляющий сетевое соединение (адреса, порты,
 *        семейство и протокол) с владеющим им процессом операционной системы
 *
 * \~english
 * @brief Header file of the process resolver module — the Process_Resolver class,
 *        which matches a network connection (addresses, ports,
 *        family and protocol) with the operating system process owning it
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_PROCRE__
#define __AWH_PROCRE__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <unistd.h>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "log.hpp"
#include "../net/net.hpp"
#include "../net/event.hpp"

/**
 * \~russian
 * @brief Основное пространство имён
 *
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Класс работы с резольвером процессов
	 *
	 * \~english
	 * @brief Class for working with the process resolver
	 *
	 * \~
	 */
	typedef class __AWH_SHARED_EXPORT__ Process_Resolver {
		public:
			/**
			 * \~russian
			 * @brief Структура портов
			 *
			 * @details Порт источника и порт назначения процесса.
			 *
			 * \~english
			 * @brief Structure of the ports
			 * @details The source port and the destination port of the process.
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Ports {
				// Порт источника
				uint16_t src;
				// Порт назначения
				uint16_t dst;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * \~
				 */
				explicit Ports() noexcept;
			} ports_t;
			/**
			 * \~russian
			 * @brief Структура IP-адресов процесса
			 *
			 * @details IP-адрес источника и IP-адрес назначения процесса.
			 *
			 * \~english
			 * @brief Structure of the IP addresses of the process
			 * @details The source IP address and the destination IP address of the process.
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Addresses {
				// Адрес источника процесса
				unique_ptr <net::addr_t> src;
				// Адрес назначения процесса
				unique_ptr <net::addr_t> dst;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * \~
				 */
				explicit Addresses() noexcept;
			} addrs_t;
			/**
			 * \~russian
			 * @brief Структура информационных метаданных процесса
			 *
			 * @details Содержит информацию о портах, IP-адресах, семействе протокола и протоколе процесса.
			 *
			 * \~english
			 * @brief Structure of the informational metadata of the process
			 * @details Holds information about the ports, the IP addresses, the protocol family and the protocol of the process.
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Info {
				// Порты процесса
				ports_t ports;
				// IP-адреса процесса
				addrs_t addresses;
				// Семейство протокола процесса
				event::family_t family;
				// Протокол процесса
				event::protocol_t protocol;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * \~
				 */
				explicit Info() noexcept;
			} info_t;
		private:
			/**
			 * \~russian
			 * @brief Функция обратного вызова для получения информации о процессе
			 *
			 * @details Вызывается для каждого процесса, найденного в ходе сканирования
			 *          активных процессов. Если не установлена - информация о процессах
			 *          не возвращается.
			 *
			 * @param pid  идентификатор процесса
			 * @param info объект информационных метаданных процесса
			 *
			 * \~english
			 * @brief Callback function for getting information about a process
			 * @details It is called for every process found in the course of the scan of
			 *          the active processes. If it is not set — the information about the processes
			 *          is not returned.
			 * @param pid  identifier of the process
			 * @param info object of the informational metadata of the process
			 *
			 * \~
			 */
			function <void (const pid_t, const info_t &)> _callback;
		private:
			// Объект работы с логами
			const log_t * _log;
		public:
			/**
			 * \~russian
			 * @brief Метод запуска процесса сканирования активных процессов и получения информации о них
			 *
			 * \~english
			 * @brief Method of starting the scan of the active processes and of getting information about them
			 *
			 * \~
			 */
			void scanning() noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения названия приложения по идентификатору процесса
			 *
			 * @param pid идентификатор процесса
			 * @return    название приложения которому принадлежит процесс
			 *
			 * \~english
			 * @brief Method of getting the name of the application by the identifier of the process
			 * @param pid identifier of the process
			 * @return    name of the application the process belongs to
			 *
			 * \~
			 */
			string name(const pid_t pid = ::getpid()) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки функции обратного вызова для получения информации о процессе
			 *
			 * @param callback функция обратного вызова
			 *
			 * \~english
			 * @brief Method of setting the callback function for getting information about a process
			 * @param callback callback function
			 *
			 * \~
			 */
			void on(function <void (const pid_t, const info_t &)> callback) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param log объект для работы с логами
			 *
			 * \~english
			 * @brief Constructor
			 * @param log object for working with logs
			 *
			 * \~
			 */
			explicit Process_Resolver(const log_t * log) noexcept;
			/**
			 * \~russian
			 * @brief Деструктор
			 *
			 *
			 * \~english
			 * @brief Destructor
			 *
			 * \~
			 */
			~Process_Resolver() noexcept;
	} procre_t;
};

#endif // __AWH_PROCRE__
