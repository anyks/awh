/**
 * @file: http1.hpp
 * @date: 2026-07-18
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл тестовой фикстуры парсера протокола HTTP/1.x — объявление класса фикстуры Google Test,
 *        подготавливающего и освобождающего тестовое окружение набора тестов
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_HTTP_PARSER_HTTP1_TESTS__
#define __AWH_HTTP_PARSER_HTTP1_TESTS__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <cstdint>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "../../../../main.hpp"
#include "../../../../../include/proto/http/parser/http1/http.hpp"

/**
 * @brief Класс фикстуры для тестов подмодуля парсера HTTP/1.0 и HTTP/1.1
 *
 */
class ParserFixture : public testing::Test {
	public:
		/**
		 * @brief Структура сборщика событий парсера
		 *
		 */
		typedef struct Events {
			// Флаг вызова функции обратного вызова обработки провайдера
			bool providerFired = false;
			// Собранное тело сообщения
			std::string body;
			// Собранные заголовки сообщения
			std::vector <std::pair <std::string, std::string>> headers;
			// Собранные трейлеры сообщения
			std::vector <std::pair <std::string, std::string>> trailers;
			// Собранные фазовые события (фаза, часть сообщения)
			std::vector <std::pair <awh::http::parser_t::phase_t, awh::http::parser_t::part_t>> phases;
			// Собранные события границ чанков (фаза, размер, расширения)
			std::vector <std::tuple <awh::http::parser_t::phase_t, uint64_t, std::string>> chunks;
		} events_t;
	protected:
		// Объект фреймворка
		std::unique_ptr <awh::fmk_t> _fmk;
		// Объект логов
		std::unique_ptr <awh::log_t> _log;
	public:
		/**
		 * @brief Метод настройки тестового окружения
		 *
		 */
		void SetUp();
		/**
		 * @brief Метод очистки тестового окружения
		 *
		 */
		void TearDown();
	protected:
		/**
		 * @brief Фабричный метод создания объекта парсера
		 *
		 * @param direct направление трафика (запрос/ответ)
		 * @return       сформированный объект парсера
		 *
		 */
		std::unique_ptr <awh::http::parser_http_t> make(const awh::http::direct_t direct) const noexcept;
		/**
		 * @brief Метод подписки сборщика событий на все функции обратного вызова парсера
		 *
		 * @param parser объект парсера
		 * @param events объект сборщика событий парсера
		 *
		 */
		void attach(awh::http::parser_http_t & parser, events_t & events) const noexcept;
};

#endif // __AWH_HTTP_PARSER_HTTP1_TESTS__
