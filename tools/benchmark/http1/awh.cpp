/**
 * @file awh.cpp
 * @date 2026-07-26
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
 * @brief Эталонный стенд сравнения парсера протокола HTTP/1.x библиотеки AWH —
 *        та же нагрузка и тот же прогон, что и у сравниваемых реализаций
 *
 * @details Показатели библиотеки снимает её собственный набор бенчмарков, но в
 *          сравнении участвует именно этот стенд: он проводит парсер через тот же
 *          драйвер прогона, что и остальных, и снимает с них ту же контрольную
 *          сумму. Иначе разница в обвязке замера осталась бы неотделима от
 *          разницы в самих парсерах
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * Подключаем заголовочный файл сравниваемой реализации
 */
#include <proto/http/parser/http1/http.hpp>
#include <sys/log.hpp>

/**
 * Подписываемся на пространство имён HTTP-протокола
 */
using namespace awh::http;

/**
 * @brief Обвязка парсера библиотеки AWH под интерфейс прогона сценариев
 *
 */
namespace {
	/**
	 * @brief Класс разбора сообщений парсером библиотеки AWH
	 *
	 */
	class Engine {
		private:
			// Объект фреймворка окружения парсера
			awh::fmk_t _fmk;
			// Объект логирования окружения парсера
			awh::log_t _log;
			// Объект парсера сравниваемой реализации
			parser_http_t _parser;
		public:
			/**
			 * @brief Метод сброса состояния для разбора следующего сообщения
			 *
			 */
			void reset() noexcept {
				// Выполняем сброс состояния парсера
				this->_parser.reset();
			}
			/**
			 * @brief Метод подачи фрагмента сообщения
			 *
			 * @note Состояние разбора здесь не опрашивается намеренно: опрос
			 *       обошёлся бы вызовом функции библиотеки на каждый поданный
			 *       фрагмент, а сценарий побайтовой подачи измеряет как раз
			 *       стоимость одного вызова. Ошибку разбора ловит проверка
			 *       завершённости, которую драйвер прогона выполняет на прогреве
			 *
			 * @param data данные фрагмента сообщения
			 * @param size размер фрагмента сообщения
			 * @return     результат разбора (false - фрагмент разобран с ошибкой)
			 *
			 */
			bool feed(const char * data, const size_t size) noexcept {
				// Выполняем разбор фрагмента сообщения
				this->_parser.parse(data, size);
				// Выводим положительный результат
				return true;
			}
			/**
			 * @brief Метод проверки завершённости разбора сообщения
			 *
			 * @return результат проверки (true - сообщение разобрано целиком)
			 *
			 */
			bool complete() const noexcept {
				// Выводим результат проверки завершённости разбора сообщения
				return (this->_parser.status() == parser_t::status_t::COMPLETE);
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Engine() noexcept : _fmk(), _log(&_fmk), _parser(direct_t::REQUEST, &_fmk, &_log) {
				// Устанавливаем функцию обратного вызова обработки фрагмента тела сообщения
				this->_parser.on(parser_http_t::data_callback_t([](const uint32_t, const void * buffer, const size_t size, const bool) noexcept -> bool {
					// Выполняем потребление фрагмента тела сообщения
					rival::consume(buffer, size);
					// Продолжаем разбор
					return true;
				}));
				// Устанавливаем функцию обратного вызова обработки заголовков сообщения
				this->_parser.on(parser_http_t::header_callback_t([](const uint32_t, const std::string_view name, const std::string_view value, const parser_t::part_t) noexcept -> bool {
					// Учитываем разобранный заголовок
					rival::account(name.size(), value.size());
					// Продолжаем разбор
					return true;
				}));
			}
	};
};

/**
 * @brief Функция входа в стенд
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из стенда
 *
 */
int32_t main(int32_t argc, char ** argv) noexcept {
	// Получаем фильтр названий выполняемых сценариев
	const char * mask = rival::filter(argc, argv);
	// Создаём объект разбора сообщений сравниваемой реализацией
	Engine engine;
	// Выполняем сценарий разбора запроса без заголовков
	rival::execute("http1/parse/tiny-request", rival::metric_t::MESSAGES, engine, rival::tiny(), rival::TINY_ROUNDS, 0, mask);
	// Выполняем сценарий разбора запроса браузера
	rival::execute("http1/parse/typical-request", rival::metric_t::THROUGHPUT, engine, rival::typical(), rival::TYPICAL_ROUNDS, 0, mask);
	// Выполняем сценарий разбора запроса браузера при побайтовой подаче
	rival::execute("http1/parse/fragmented-request", rival::metric_t::THROUGHPUT, engine, rival::typical(), rival::FRAGMENT_ROUNDS, 1, mask);
	// Выполняем сценарий разбора тела фиксированного размера
	rival::execute("http1/parse/identity-body", rival::metric_t::THROUGHPUT, engine, rival::identity(rival::BODY_SIZE), rival::BODY_ROUNDS, 0, mask);
	// Выполняем сценарий разбора тела в кодировке chunked
	rival::execute("http1/parse/chunked-body", rival::metric_t::THROUGHPUT, engine, rival::chunked(rival::BODY_SIZE, rival::CHUNK_SIZE), rival::BODY_ROUNDS, 0, mask);
	// Выполняем сценарий учёта выделений памяти на одно разобранное сообщение
	rival::execute("http1/allocations/per-parsed-message", rival::metric_t::ALLOCATIONS, engine, rival::typical(), rival::TYPICAL_ROUNDS, 0, mask);
	// Выводим контрольную сумму обработанных данных
	rival::digest(argc, argv);
	// Выводим успешный код выхода
	return 0;
}
