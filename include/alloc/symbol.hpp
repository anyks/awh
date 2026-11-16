/**
 * @file symbol.hpp
 * @date 2026-11-16
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
 * @brief Заголовочный файл сведений об адресе стека вызовов — образ, функция и смещение, какие отдаёт разбор адреса
 *
 * \~english
 * @brief Header file of the call stack address information — image, function and offset returned by address resolution
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#pragma once

/**
 * Стандартные заголовочные файлы
 */
#include <cstddef>
#include <cstdint>

/**
 * Подключаем заголовочный файл проекта
 */
#include "../sys/macro/global.hpp"

/**
 * \~russian
 * @brief Пространство имён фреймворка
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * \~russian
	 * @brief Пространство имён распределителя памяти
	 *
	 * \~english
	 * @brief Namespace of the memory allocator
	 *
	 * \~
	 */
	namespace alloc {
		/**
		 * \~russian
		 * @brief Сведения об адресе стека вызовов
		 *
		 * @note Строки указывают на память образа и на память средств системы: годны
		 *       они до следующего обращения к разбору В ТОМ ЖЕ ПОТОКЕ, а копировать их
		 *       обязан тот, кому они нужны дольше. Оговорка о потоке не лишняя: у
		 *       MS Windows название образа хранится за нами, и буфер под него свой у
		 *       каждого потока - разбор в чужом потоке строку не трогает
		 *
		 * \~english
		 * @brief Information about a call stack address
		 *
		 * @note The strings point to image memory and system facility memory: they
		 *       are valid until the next resolution call IN THE SAME THREAD, and
		 *       whoever needs them longer must copy them. The thread caveat is not
		 *       superfluous: on MS Windows the image name is stored by us, and the
		 *       buffer for it is per-thread - resolution in a foreign thread does
		 *       not touch the string
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Symbol {
			// Смещение разбираемого адреса от начала функции
			ptrdiff_t offset;
			// Название функции, которой принадлежит адрес
			const char * name;
			// Название образа, которому принадлежит адрес
			const char * image;
			// Адрес начала функции
			const void * begin;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * \~
			 */
			explicit Symbol() noexcept;
		} symbol_t;
	};
};
