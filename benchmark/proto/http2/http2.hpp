/**
 * @file: http2.hpp
 * @date: 2026-07-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл бенчмарков протокола HTTP/2 — общее окружение сценариев,
 *        эталонные наборы заголовков и средства сборки потока кадров соединения
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_BENCHMARK_HTTP2__
#define __AWH_BENCHMARK_HTTP2__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../main.hpp"
#include "../../../include/proto/http/parser/http2/http.hpp"

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
		 * @brief Пространство имён бенчмарков протокола HTTP/2
		 *
		 * @details В отличие от HTTP/1.x протокол работает на уровне соединения, а не
		 *          отдельного сообщения, поэтому измеряется не только разбор байт:
		 *          сжатие заголовков HPACK, мультиплексирование потоков и учёт окон
		 *          управления потоком дают собственные характеристики, которых
		 *          у текстового протокола нет
		 *
		 */
		namespace http2 {
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
			 * @brief Функция получения эталонного набора заголовков запроса браузера
			 *
			 * @details Набор повторяет запрос настоящего браузера: псевдо-заголовки,
			 *          длинный user-agent, cookie и переменная часть в виде пути.
			 *          Именно на таком наборе имеет смысл измерять сжатие HPACK -
			 *          на постоянной части выигрыш даёт статическая таблица,
			 *          на повторяющейся переменной динамическая
			 *
			 * @param index номер запроса в последовательности
			 * @return      набор заголовков запроса
			 *
			 */
			std::vector <awh::http::h2::hpack::field_t> request(const size_t index) noexcept;
			/**
			 * @brief Функция получения эталонного набора заголовков ответа сервера
			 *
			 * @param index номер ответа в последовательности
			 * @return      набор заголовков ответа
			 *
			 */
			std::vector <awh::http::h2::hpack::field_t> response(const size_t index) noexcept;
			/**
			 * @brief Функция получения эталонного тела ответа
			 *
			 * @note Объявленная в заголовках длина обязана совпадать с фактической:
			 *       расхождение - малформированное сообщение (RFC 9113 §8.1.1),
			 *       и парсер справедливо оборвёт поток
			 *
			 * @return тело ответа
			 *
			 */
			const std::string & payload() noexcept;
			/**
			 * @brief Функция подсчёта размера набора заголовков до сжатия
			 *
			 * @note Считается по правилу RFC 9113 §6.5.2 без надбавки в 32 октета
			 *       на запись: сравнивать сжатый блок нужно с объёмом самих данных,
			 *       а не с оценкой памяти под них
			 *
			 * @param fields набор заголовков
			 * @return       размер набора заголовков в октетах
			 *
			 */
			size_t length(const std::vector <awh::http::h2::hpack::field_t> & fields) noexcept;
			/**
			 * @brief Функция сборки кадра HTTP/2
			 *
			 * @param type    тип кадра
			 * @param flags   флаги кадра
			 * @param sid     идентификатор потока
			 * @param payload полезная нагрузка кадра
			 * @return        собранный кадр
			 *
			 */
			std::string frame(const uint8_t type, const uint8_t flags, const uint32_t sid, const std::string & payload) noexcept;
			/**
			 * @brief Функция сборки преамбулы соединения клиента
			 *
			 * @return преамбула соединения: magic-строка и кадр SETTINGS
			 *
			 */
			std::string preface() noexcept;
		};
	};
};

#endif // __AWH_BENCHMARK_HTTP2__
