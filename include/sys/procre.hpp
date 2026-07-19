/**
 * @file: procre.hpp
 * @date: 2026-01-26
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
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
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Класс работы с резольвером процессов
	 *
	 */
	typedef class __AWH_SHARED_EXPORT__ Process_Resolver {
		public:
			/**
			 * @brief Структура портов
			 *
			 */
			typedef struct Ports {
				// Порт источника
				uint16_t src;
				// Порт назначения
				uint16_t dst;
				/**
				 * @brief Конструктор
				 *
				 */
				explicit Ports() noexcept : src(0), dst(0) {}
			} ports_t;
			/**
			 * @brief Структура IP-адресов процесса
			 *
			 */
			typedef struct Addresses {
				// Адрес источника процесса
				unique_ptr <net::addr_t> src;
				// Адрес назначения процесса
				unique_ptr <net::addr_t> dst;
				/**
				 * @brief Конструктор
				 *
				 */
				explicit Addresses() noexcept : src(nullptr), dst(nullptr) {}
			} addrs_t;
			/**
			 * @brief Структура информационных метаданных процесса
			 *
			 */
			typedef struct Info {
				// Порты процесса
				ports_t ports;
				// IP-адреса процесса
				addrs_t addresses;
				// Семейство протокола процесса
				event::family_t family;
				// Протокол процесса
				event::protocol_t protocol;
				/**
				 * @brief Конструктор
				 *
				 */
				explicit Info() noexcept :
				 family(event::family_t::NONE),
				 protocol(event::protocol_t::NONE) {}
			} info_t;
		private:
			/**
			 * @brief Функция обратного вызова для получения информации о процессе
			 *
			 * @details Вызывается для каждого процесса, найденного в ходе сканирования
			 *          активных процессов. Если не установлена - информация о процессах
			 *          не возвращается.
			 *
			 * @param pid  идентификатор процесса
			 * @param info объект информационных метаданных процесса
			 */
			function <void (const pid_t, const info_t &)> _callback;
		private:
			// Объект работы с логами
			const log_t * _log;
		public:
			/**
			 * @brief Метод запуска процесса сканирования активных процессов и получения информации о них
			 *
			 */
			void scanning() noexcept;
		public:
			/**
			 * @brief Метод получения названия приложения по идентификатору процесса
			 *
			 * @param pid идентификатор процесса
			 * @return    название приложения которому принадлежит процесс
			 */
			string name(const pid_t pid = ::getpid()) const noexcept;
		public:
			/**
			 * @brief Метод установки функции обратного вызова для получения информации о процессе
			 *
			 * @param callback функция обратного вызова
			 */
			void on(function <void (const pid_t, const info_t &)> callback) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param log объект для работы с логами
			 */
			explicit Process_Resolver(const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Process_Resolver() noexcept;
	} procre_t;
};

#endif // __AWH_PROCRE__
