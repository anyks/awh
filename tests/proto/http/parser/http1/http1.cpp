/**
 * @file: http1.cpp
 * @date: 2026-07-18
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация тестовой фикстуры парсера протокола HTTP/1.x —
 *        создание объектов тестового окружения перед каждым тестом и их освобождение после его завершения
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "http1.hpp"

/**
 * @brief Метод настройки тестового окружения
 *
 */
void ParserFixture::SetUp(){
	// Создаём объект фреймворка
	this->_fmk = std::make_unique <awh::fmk_t> ();
	// Создаём объект логгера
	this->_log = std::make_unique <awh::log_t> (this->_fmk.get());
}

/**
 * @brief Метод очистки тестового окружения
 *
 */
void ParserFixture::TearDown() {}

/**
 * @brief Фабричный метод создания объекта парсера
 *
 * @param direct направление трафика (запрос/ответ)
 * @return       сформированный объект парсера
 *
 */
std::unique_ptr <awh::http::parser_http_t> ParserFixture::make(const awh::http::direct_t direct) const noexcept {
	// Создаём и возвращаем объект парсера
	return std::make_unique <awh::http::parser_http_t> (direct, this->_fmk.get(), this->_log.get());
}

/**
 * @brief Метод подписки сборщика событий на все функции обратного вызова парсера
 *
 * @param parser объект парсера
 * @param events объект сборщика событий парсера
 *
 */
void ParserFixture::attach(awh::http::parser_http_t & parser, events_t & events) const noexcept {
	// Устанавливаем функцию обратного вызова для обработки фрагмента тела сообщения
	parser.on(awh::http::parser_http_t::data_callback_t([&events](const uint32_t, const void * buffer, const size_t size, const bool) noexcept -> bool {
		// Собираем фрагмент тела сообщения
		events.body.append(static_cast <const char *> (buffer), size);
		// Продолжаем разбор
		return true;
	}));
	// Устанавливаем функцию обратного вызова для обработки фазы разбора HTTP-сообщения
	parser.on(awh::http::parser_http_t::phase_callback_t([&events](const uint32_t, const awh::http::parser_t::phase_t phase, const awh::http::parser_t::part_t part) noexcept -> bool {
		// Собираем фазовое событие
		events.phases.emplace_back(phase, part);
		// Продолжаем разбор
		return true;
	}));
	// Устанавливаем функцию обратного вызова для обработки границ чанков
	parser.on(awh::http::parser_http_t::chunk_callback_t([&events](const awh::http::parser_t::phase_t phase, const uint64_t size, const std::string_view extension) noexcept -> bool {
		// Собираем событие границы чанка
		events.chunks.emplace_back(phase, size, std::string(extension));
		// Продолжаем разбор
		return true;
	}));
	// Устанавливаем функцию обратного вызова для обработки заголовков или трейлеров сообщения
	parser.on(awh::http::parser_http_t::header_callback_t([&events](const uint32_t, const std::string_view name, const std::string_view value, const awh::http::parser_t::part_t part) noexcept -> bool {
		// Если получен трейлер сообщения
		if(part == awh::http::parser_t::part_t::TRAILER)
			// Собираем трейлер сообщения
			events.trailers.emplace_back(std::string(name), std::string(value));
		// Если получен заголовок сообщения
		else events.headers.emplace_back(std::string(name), std::string(value));
		// Продолжаем разбор
		return true;
	}));
	// Устанавливаем функцию обратного вызова для обработки провайдера заголовков сообщения
	parser.on(awh::http::parser_http_t::provider_callback_t([&events](const uint32_t, const awh::http::provider_t * provider, const bool) noexcept -> bool {
		// Помечаем что функция обратного вызова обработки провайдера вызвана (для трейлеров провайдер - nullptr)
		if(provider != nullptr)
			// Помечаем что функция обратного вызова обработки провайдера вызвана
			events.providerFired = true;
		// Продолжаем разбор
		return true;
	}));
}
