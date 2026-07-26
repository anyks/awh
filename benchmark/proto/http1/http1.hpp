/**
 * @file: http1.hpp
 * @date: 2026-07-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл бенчмарков протокола HTTP/1.x — общее окружение сценариев
 *        и набор эталонных сообщений, на которых измеряется разбор и сборка
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_BENCHMARK_HTTP1__
#define __AWH_BENCHMARK_HTTP1__

/**
 * Стандартные заголовочные файлы
 */
#include <string>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../main.hpp"
#include "../../../include/proto/http/parser/http1/http.hpp"

/**
 * @brief основное пространство имён
 *
 */
namespace awh {
	/**
	 * @brief Пространство имён бенчмарков
	 *
	 */
	namespace benchmark {
		/**
		 * @brief Пространство имён бенчмарков протокола HTTP/1.x
		 *
		 */
		namespace http1 {
			/**
			 * @brief Функция получения объекта фреймворка окружения бенчмарка
			 *
			 * @note Объекты окружения создаются при первом обращении: порядок
			 *       статической инициализации между единицами трансляции не
			 *       определён, а фреймворк зависит от таблиц чужих модулей
			 *
			 * @return объект фреймворка
			 *
			 */
			const fmk_t * fmk() noexcept;
			/**
			 * @brief Функция получения объекта логирования окружения бенчмарка
			 *
			 * @return объект логирования
			 *
			 */
			const log_t * log() noexcept;
			/**
			 * @brief Функция получения эталонного запроса без заголовков
			 *
			 * @details Нагрузка, на которой доминируют постоянные накладные расходы
			 *          на сообщение: сброс состояния, стартовая строка и фазовые события
			 *
			 * @return эталонный запрос
			 *
			 */
			const std::string & tiny() noexcept;
			/**
			 * @brief Функция получения эталонного запроса браузера
			 *
			 * @details Нагрузка, на которой доминируют заголовки: длинные значения
			 *          сканируются крупноблочно, а имена и значения копятся в накопители
			 *
			 * @return эталонный запрос
			 *
			 */
			const std::string & typical() noexcept;
			/**
			 * @brief Функция получения эталонного запроса с телом фиксированного размера
			 *
			 * @param size размер тела сообщения
			 * @return     эталонный запрос
			 *
			 */
			const std::string & identity(const size_t size) noexcept;
			/**
			 * @brief Функция получения эталонного запроса с телом в кодировке chunked
			 *
			 * @param size    размер тела сообщения
			 * @param portion размер одного чанка
			 * @return        эталонный запрос
			 *
			 */
			const std::string & chunked(const size_t size, const size_t portion) noexcept;
		};
	};
};

#endif // __AWH_BENCHMARK_HTTP1__
