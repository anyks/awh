/**
 * @file http3.hpp
 * @date 2026-07-27
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
 * @brief Заголовочный файл тестовой фикстуры парсера протокола HTTP/3 — объявление класса фикстуры Google Test
 *        и эмулятора транспорта QUIC, соединяющего два парсера потоками
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_HTTP_PARSER_HTTP3_TESTS__
#define __AWH_HTTP_PARSER_HTTP3_TESTS__

/**
 * Стандартные заголовочные файлы
 */
#include <map>
#include <deque>
#include <tuple>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "../../../../main.hpp"
#include "../../../../../include/proto/http/parser/http3/http.hpp"

/**
 * @brief Класс фикстуры для тестов подмодуля парсера HTTP/3
 *
 */
class ParserHttp3Fixture : public testing::Test {
	public:
		/**
		 * @brief Структура сборщика событий парсера
		 *
		 */
		typedef struct Events {
			// Идентификаторы открытых пиром потоков
			std::vector <uint64_t> begins;
			// Собранные поля секций (поток, название, значение, часть сообщения)
			std::vector <std::tuple <uint64_t, std::string, std::string, awh::http::parser_t::part_t>> headers;
			// События провайдеров полей (поток, трейлеры (провайдер = nullptr), завершение потока)
			std::vector <std::tuple <uint64_t, bool, bool>> providers;
			// Собранные фазовые события приёма сообщений (поток, фаза, часть сообщения)
			std::vector <std::tuple <uint64_t, awh::http::parser_t::phase_t, awh::http::parser_t::part_t>> phases;
			// Собранные тела потоков
			std::map <uint64_t, std::string> bodies;
			// События закрытия потоков (поток, код ошибки закрытия)
			std::vector <std::pair <uint64_t, awh::http::Parser_HTTP3::error_t>> closes;
			// События обрыва потоков (поток, код ошибки, признак остановки приёма)
			std::vector <std::tuple <uint64_t, awh::http::Parser_HTTP3::error_t, bool>> aborts;
			// События ошибок уровня соединения (код ошибки, описание)
			std::vector <std::pair <awh::http::Parser_HTTP3::error_t, std::string>> errors;
			// Анонсы server push (ассоциированный поток, идентификатор push)
			std::vector <std::pair <uint64_t, uint64_t>> pushes;
			// Идентификаторы, объявленные пиром в кадре GOAWAY
			std::vector <uint64_t> goaways;
			// Количество применённых наборов параметров пира
			size_t settings = 0;
			// Разобранный из провайдера метод запроса клиента
			std::string method;
			// Разобранный из провайдера URI-запрос клиента
			std::string uri;
			// Разобранный из провайдера статус-код ответа сервера
			uint16_t code = 0;
		} events_t;
		/**
		 * @brief Класс эмулятора транспорта QUIC для одной стороны соединения
		 *
		 * @details Парсер HTTP/3 не умеет ни открывать потоки, ни писать в них: этим
		 *          распоряжается транспорт. Эмулятор выдаёт идентификаторы потоков
		 *          по правилам RFC 9000 §2.1 и складывает исходящие байты в очередь,
		 *          откуда их забирает прокачка
		 *
		 */
		typedef class Endpoint {
			public:
				// Объект парсера стороны соединения
				std::unique_ptr <awh::http::parser_http3_t> parser;
				// Сборщик событий парсера стороны соединения
				events_t events;
			public:
				// Очередь исходящих данных (поток, данные, признак завершения потока)
				std::deque <std::tuple <uint64_t, std::string, bool>> queue;
				// Идентификатор следующего выдаваемого однонаправленного потока
				int64_t unistream;
				// Идентификатор следующего выдаваемого двунаправленного потока
				uint64_t bistream;
			public:
				// Признак отказа транспорта открывать однонаправленные потоки
				bool refuse;
			public:
				/**
				 * @brief Конструктор
				 *
				 */
				explicit Endpoint() noexcept : unistream(0), bistream(0), refuse(false) {}
		} endpoint_t;
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
		 * @brief Метод подготовки стороны соединения
		 *
		 * @details Создаёт парсер, подписывает сборщик событий и подключает эмулятор
		 *          транспорта. Идентификаторы потоков выдаются по правилам RFC 9000 §2.1:
		 *          младший бит различает инициатора, второй - направленность
		 *
		 * @param endpoint сторона соединения
		 * @param direct   направление трафика (REQUEST - мы сервер, RESPONSE - мы клиент)
		 *
		 */
		void setup(endpoint_t & endpoint, const awh::http::direct_t direct) const noexcept;
		/**
		 * @brief Метод прокачки очередей исходящих данных между сторонами соединения
		 *
		 * @details Байты каждой стороны подаются на разбор другой стороне, как если бы
		 *          они прошли через соединение QUIC без потерь и переупорядочивания
		 *
		 * @param client сторона клиента
		 * @param server сторона сервера
		 *
		 */
		void pump(endpoint_t & client, endpoint_t & server) const noexcept;
		/**
		 * @brief Метод выполнения рукопожатия соединения (обмен SETTINGS)
		 *
		 * @param client сторона клиента
		 * @param server сторона сервера
		 *
		 */
		void handshake(endpoint_t & client, endpoint_t & server) const noexcept;
		/**
		 * @brief Метод подачи произвольных байтов в поток одной стороны
		 *
		 * @details Нужен для проверок реакции на некорректные потоки, которые
		 *          исправный парсер противоположной стороны сформировать не может
		 *
		 * @param endpoint сторона соединения
		 * @param sid      идентификатор потока
		 * @param data     подаваемые байты
		 * @param fin      признак завершения потока
		 * @return         результат разбора
		 *
		 */
		awh::http::h3::status_t feed(endpoint_t & endpoint, const uint64_t sid, const std::string & data, const bool fin = false) const noexcept;
		/**
		 * @brief Метод сборки набора полей запроса клиента
		 *
		 * @param method метод запроса
		 * @param path   путь запроса
		 * @return       собранный набор полей
		 *
		 */
		std::vector <awh::http::h3::qpack::field_t> request(const std::string & method, const std::string & path) const noexcept;
		/**
		 * @brief Метод сборки набора полей ответа сервера
		 *
		 * @param status статус-код ответа
		 * @return       собранный набор полей
		 *
		 */
		std::vector <awh::http::h3::qpack::field_t> response(const std::string & status) const noexcept;
		/**
		 * @brief Метод поиска значения поля в собранных событиях
		 *
		 * @param events сборщик событий парсера
		 * @param sid    идентификатор потока
		 * @param name   название искомого поля
		 * @return       значение поля либо пустая строка
		 *
		 */
		std::string field(const events_t & events, const uint64_t sid, const std::string & name) const noexcept;
		/**
		 * @brief Метод сборки последовательности фазовых событий потока в строку
		 *
		 * @param events сборщик событий парсера
		 * @param sid    идентификатор потока
		 * @return       последовательность фазовых событий
		 *
		 */
		std::string phases(const events_t & events, const uint64_t sid) const noexcept;
};

#endif // __AWH_HTTP_PARSER_HTTP3_TESTS__
