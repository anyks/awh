/**
 * @file: http2.cpp
 * @date: 2026-07-19
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
 * Подключаем заголовочный файлы проекта
 */
#include "http2.hpp"

/**
 * @brief Метод настройки тестового окружения
 *
 */
void ParserHttp2Fixture::SetUp(){
	// Создаём объект фреймворка
	this->_fmk = std::make_unique <awh::fmk_t> ();
	// Создаём объект логгера
	this->_log = std::make_unique <awh::log_t> (this->_fmk.get());
}

/**
 * @brief Метод очистки тестового окружения
 *
 */
void ParserHttp2Fixture::TearDown() {}

/**
 * @brief Фабричный метод создания объекта парсера
 *
 * @param direct направление трафика (REQUEST - мы сервер, RESPONSE - мы клиент)
 * @return       сформированный объект парсера
 */
std::unique_ptr <awh::http::parser_http2_t> ParserHttp2Fixture::make(const awh::http::direct_t direct) const noexcept {
	// Создаём и возвращаем объект парсера
	return std::make_unique <awh::http::parser_http2_t> (direct, this->_fmk.get(), this->_log.get());
}

/**
 * @brief Метод подписки сборщика событий на все функции обратного вызова парсера
 *
 * @param parser объект парсера
 * @param events объект сборщика событий парсера
 */
void ParserHttp2Fixture::attach(awh::http::parser_http2_t & parser, events_t & events) const noexcept {
	// Устанавливаем функцию обратного вызова для обработки открытия нового потока
	parser.on(awh::http::parser_http2_t::begin_callback_t([&events](const uint32_t sid) noexcept -> bool {
		// Собираем идентификатор открытого пиром потока
		events.begins.push_back(sid);
		// Продолжаем разбор
		return true;
	}));
	// Устанавливаем функцию обратного вызова для обработки заголовков или трейлеров потока
	parser.on(awh::http::parser_http2_t::header_callback_t([&events](const uint32_t sid, const std::string_view name, const std::string_view value, const awh::http::parser_t::part_t part) noexcept -> bool {
		// Собираем заголовок или трейлер потока
		events.headers.emplace_back(sid, std::string(name), std::string(value), part);
		// Продолжаем разбор
		return true;
	}));
	// Устанавливаем функцию обратного вызова для обработки провайдера заголовков потока
	parser.on(awh::http::parser_http2_t::provider_callback_t([&events](const uint32_t sid, const awh::http::provider_t * provider, const bool endStream) noexcept -> bool {
		// Собираем событие провайдера заголовков потока
		events.providers.emplace_back(sid, (provider == nullptr), endStream);
		// Если провайдер заголовков передан
		if(provider != nullptr){
			// Если провайдер является запросом клиента
			if(provider->direct == awh::http::direct_t::REQUEST){
				// Собираем метод запроса клиента
				events.method = static_cast <const awh::http::request_t *> (provider)->method;
				// Собираем параметры URI-запроса
				events.uri = static_cast <const awh::http::request_t *> (provider)->uri;
			// Если провайдер является ответом сервера - собираем статус-код
			} else events.code = static_cast <const awh::http::response_t *> (provider)->code;
		}
		// Продолжаем разбор
		return true;
	}));
	// Устанавливаем функцию обратного вызова для обработки фрагмента тела потока
	parser.on(awh::http::parser_http2_t::data_callback_t([&events](const uint32_t sid, const void * buffer, const size_t size, const bool endStream) noexcept -> bool {
		// Собираем фрагмент тела потока
		events.bodies[sid].append(static_cast <const char *> (buffer), size);
		// Не используемый параметр
		(void) endStream;
		// Продолжаем разбор
		return true;
	}));
	// Устанавливаем функцию обратного вызова для обработки закрытия потока
	parser.on(awh::http::parser_http2_t::close_callback_t([&events](const uint32_t sid, const awh::http::parser_http2_t::error_t code) noexcept {
		// Собираем событие закрытия потока
		events.closes.emplace_back(sid, code);
	}));
	// Устанавливаем функцию обратного вызова для обработки анонса server push
	parser.on(awh::http::parser_http2_t::push_callback_t([&events](const uint32_t sid, const uint32_t promisedSid) noexcept -> bool {
		// Собираем событие анонса server push
		events.pushes.emplace_back(sid, promisedSid);
		// Принимаем push
		return true;
	}));
	// Устанавливаем функцию обратного вызова о готовности потока принимать данные тела
	parser.on(awh::http::parser_http2_t::writable_callback_t([&events](const uint32_t sid) noexcept {
		// Собираем событие готовности потока принимать данные тела
		events.writables.push_back(sid);
	}));
	// Устанавливаем функцию обратного вызова для обработки применённого SETTINGS пира
	parser.on(awh::http::parser_http2_t::settings_callback_t([&events]() noexcept {
		// Считаем применённые SETTINGS пира
		events.settingsApplied++;
	}));
	// Устанавливаем функцию обратного вызова для обработки полученного GOAWAY
	parser.on(awh::http::parser_http2_t::goaway_callback_t([&events](const uint32_t last, const awh::http::parser_http2_t::error_t code, const std::string_view debug) noexcept {
		// Помечаем что GOAWAY получен
		events.goawayFired = true;
		// Собираем наибольший идентификатор обработанного пиром потока
		events.goawayLast = last;
		// Собираем код ошибки завершения соединения
		events.goawayCode = code;
		// Собираем отладочные данные
		events.goawayDebug = std::string(debug);
	}));
	// Устанавливаем функцию обратного вызова для обработки ошибки уровня соединения
	parser.on(awh::http::parser_http2_t::error_callback_t([&events](const awh::http::parser_http2_t::error_t code, const std::string_view message) noexcept {
		// Помечаем что ошибка уровня соединения зафиксирована
		events.errorFired = true;
		// Собираем код ошибки уровня соединения
		events.errorCode = code;
		// Собираем текстовое описание ошибки
		events.errorMessage = std::string(message);
	}));
}

/**
 * @brief Метод соединения двух парсеров каналами записи (эмуляция сети)
 *
 * @param client объект парсера клиента
 * @param server объект парсера сервера
 */
void ParserHttp2Fixture::connect(awh::http::parser_http2_t & client, awh::http::parser_http2_t & server) const noexcept {
	// Исходящие байты клиента подаём на разбор серверу
	client.on(awh::http::parser_http2_t::write_callback_t([&server](const void * buffer, const size_t size) noexcept {
		// Выполняем разбор исходящих байтов клиента на сервере
		server.parse(buffer, size);
	}));
	// Исходящие байты сервера подаём на разбор клиенту
	server.on(awh::http::parser_http2_t::write_callback_t([&client](const void * buffer, const size_t size) noexcept {
		// Выполняем разбор исходящих байтов сервера на клиенте
		client.parse(buffer, size);
	}));
}

/**
 * @brief Метод выполнения рукопожатия соединения (preface + обмен SETTINGS)
 *
 * @param client объект парсера клиента
 * @param server объект парсера сервера
 */
void ParserHttp2Fixture::handshake(awh::http::parser_http2_t & client, awh::http::parser_http2_t & server) const noexcept {
	// Клиент отправляет magic-строку и свой SETTINGS
	client.sendPreface();
	// Сервер отправляет свой SETTINGS
	server.sendPreface();
}
