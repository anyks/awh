/**
 * @file holding.hpp
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
 * @brief Заголовочный файл сведений об удерживаемом блоке — запись, какую отдаёт перебор удерживаемых блоков
 *
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
 * @brief Пространство имён фреймворка
 *
 */
namespace awh {
	/**
	 * @brief Пространство имён распределителя памяти
	 *
	 */
	namespace alloc {
		/**
		 * @brief Сведения об удерживаемом прикладным кодом блоке
		 *
		 * @note Массив стека указывает на память самой записи: годен он лишь внутри
		 *       отклика перебора, а нужен дольше - копировать
		 *
		 */
		typedef struct __AWH_SHARED_EXPORT__ Holding {
			// Затребованный размер блока в байтах
			size_t size;
			// Глубина снятого стека вызовов
			size_t depth;
			// Отметка времени выдачи в миллисекундах
			uint64_t stamp;
			// Адрес удерживаемого блока
			const void * block;
			// Стек вызовов места выдачи
			const void * const * frames;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Holding() noexcept;
		} holding_t;
	};
};
