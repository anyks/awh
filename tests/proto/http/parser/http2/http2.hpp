/**
 * @file: http2.hpp
 * @date: 2026-07-19
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

#ifndef __AWH_HTTP_PARSER_HTTP2_TESTS__
#define __AWH_HTTP_PARSER_HTTP2_TESTS__

/**
 * Стандартные заголовочные файлы
 */
#include <map>
#include <tuple>
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <cstdint>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "../../../../main.hpp"
#include "../../../../../include/proto/http/parser/http2/http.hpp"

/**
 * @brief Класс фикстуры для тестов подмодуля парсера HTTP/2
 *
 */
class ParserHttp2Fixture : public testing::Test {
	public:
		/**
		 * @brief Структура сборщика событий парсера
		 *
		 */
		typedef struct Events {
			// Идентификаторы открытых пиром потоков
			std::vector <uint32_t> begins;
			// Собранные заголовки и трейлеры (поток, название, значение, часть сообщения)
			std::vector <std::tuple <uint32_t, std::string, std::string, awh::http::parser_t::part_t>> headers;
			// События провайдеров заголовков (поток, трейлеры (провайдер = nullptr), завершение потока)
			std::vector <std::tuple <uint32_t, bool, bool>> providers;
			// Собранные фазовые события приёма сообщений (поток, фаза, часть сообщения)
			std::vector <std::tuple <uint32_t, awh::http::parser_t::phase_t, awh::http::parser_t::part_t>> phases;
			// Собранные тела потоков
			std::map <uint32_t, std::string> bodies;
			// События закрытия потоков (поток, код ошибки закрытия)
			std::vector <std::pair <uint32_t, awh::http::parser_http2_t::error_t>> closes;
			// События анонса server push (поток клиента, обещанный поток)
			std::vector <std::pair <uint32_t, uint32_t>> pushes;
			// События готовности потоков принимать данные тела
			std::vector <uint32_t> writables;
			// Количество применённых SETTINGS пира
			size_t settingsApplied = 0;
			// Флаг получения GOAWAY
			bool goawayFired = false;
			// Наибольший идентификатор обработанного пиром потока из GOAWAY
			uint32_t goawayLast = 0;
			// Код ошибки завершения соединения из GOAWAY
			awh::http::parser_http2_t::error_t goawayCode = awh::http::parser_http2_t::error_t::NO_ERROR;
			// Отладочные данные из GOAWAY
			std::string goawayDebug;
			// Флаг ошибки уровня соединения
			bool errorFired = false;
			// Код ошибки уровня соединения
			awh::http::parser_http2_t::error_t errorCode = awh::http::parser_http2_t::error_t::NO_ERROR;
			// Текстовое описание ошибки уровня соединения
			std::string errorMessage;
			// Разобранный из провайдера метод запроса клиента
			awh::http::method_t method = awh::http::method_t::NONE;
			// Разобранные из провайдера параметры URI-запроса
			std::string uri;
			// Разобранный из провайдера статус-код ответа сервера
			uint16_t code = 0;
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
		 * @param direct направление трафика (REQUEST - мы сервер, RESPONSE - мы клиент)
		 * @return       сформированный объект парсера
		 */
		std::unique_ptr <awh::http::parser_http2_t> make(const awh::http::direct_t direct) const noexcept;
		/**
		 * @brief Метод подписки сборщика событий на все функции обратного вызова парсера
		 *
		 * @param parser объект парсера
		 * @param events объект сборщика событий парсера
		 */
		void attach(awh::http::parser_http2_t & parser, events_t & events) const noexcept;
		/**
		 * @brief Метод соединения двух парсеров каналами записи (эмуляция сети)
		 *
		 * @details Исходящие байты каждого парсера напрямую подаются на разбор другому,
		 *          как если бы они прошли через TCP-соединение без задержек
		 *
		 * @param client объект парсера клиента
		 * @param server объект парсера сервера
		 */
		void connect(awh::http::parser_http2_t & client, awh::http::parser_http2_t & server) const noexcept;
		/**
		 * @brief Метод выполнения рукопожатия соединения (preface + обмен SETTINGS)
		 *
		 * @param client объект парсера клиента
		 * @param server объект парсера сервера
		 */
		void handshake(awh::http::parser_http2_t & client, awh::http::parser_http2_t & server) const noexcept;
};

#endif // __AWH_HTTP_PARSER_HTTP2_TESTS__
