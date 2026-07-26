/**
 * @file: llhttp.cpp
 * @date: 2026-07-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Эталонный стенд сравнения парсера протокола HTTP/1.x с парсером llhttp
 *        проекта Node.js — потоковым конечным автоматом, порождаемым генератором llparse
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * Подключаем заголовочный файл сравниваемой реализации
 */
#include <llhttp.h>

/**
 * @brief Обвязка парсера llhttp под интерфейс прогона сценариев
 *
 */
namespace {
	/**
	 * @brief Класс разбора сообщений парсером llhttp
	 *
	 * @details Модель разбора совпадает с моделью парсера AWH: вход подаётся
	 *          произвольными фрагментами, разобранные части сообщения отдаются
	 *          функциями обратного вызова представлениями во входной буфер,
	 *          собственного хранилища разобранного сообщения парсер не заводит
	 *
	 */
	class Engine {
		private:
			// Набор функций обратного вызова парсера
			llhttp_settings_t _settings;
			// Объект парсера сравниваемой реализации
			llhttp_t _parser;
			// Флаг завершённости разбора сообщения
			bool _complete;
		private:
			/**
			 * @brief Функция обратного вызова получения фрагмента названия заголовка
			 *
			 * @param parser объект парсера
			 * @param at     фрагмент названия заголовка
			 * @param length размер фрагмента названия заголовка
			 * @return       результат обработки (0 - разбор продолжается)
			 *
			 */
			static int32_t field([[maybe_unused]] llhttp_t * parser, [[maybe_unused]] const char * at, size_t length) noexcept {
				// Учитываем разобранный фрагмент названия заголовка
				rival::account(length, 0);
				// Продолжаем разбор
				return 0;
			}
			/**
			 * @brief Функция обратного вызова получения фрагмента значения заголовка
			 *
			 * @param parser объект парсера
			 * @param at     фрагмент значения заголовка
			 * @param length размер фрагмента значения заголовка
			 * @return       результат обработки (0 - разбор продолжается)
			 *
			 */
			static int32_t value([[maybe_unused]] llhttp_t * parser, [[maybe_unused]] const char * at, size_t length) noexcept {
				// Учитываем разобранный фрагмент значения заголовка
				rival::account(0, length);
				// Продолжаем разбор
				return 0;
			}
			/**
			 * @brief Функция обратного вызова получения фрагмента тела сообщения
			 *
			 * @param parser объект парсера
			 * @param at     фрагмент тела сообщения
			 * @param length размер фрагмента тела сообщения
			 * @return       результат обработки (0 - разбор продолжается)
			 *
			 */
			static int32_t body([[maybe_unused]] llhttp_t * parser, const char * at, size_t length) noexcept {
				// Выполняем потребление фрагмента тела сообщения
				rival::consume(at, length);
				// Продолжаем разбор
				return 0;
			}
			/**
			 * @brief Функция обратного вызова завершения разбора сообщения
			 *
			 * @param parser объект парсера
			 * @return       результат обработки (0 - разбор продолжается)
			 *
			 */
			static int32_t finish(llhttp_t * parser) noexcept {
				// Отмечаем завершённость разбора сообщения
				reinterpret_cast <Engine *> (parser->data)->_complete = true;
				// Продолжаем разбор
				return 0;
			}
		public:
			/**
			 * @brief Метод сброса состояния для разбора следующего сообщения
			 *
			 */
			void reset() noexcept {
				// Сбрасываем флаг завершённости разбора сообщения
				this->_complete = false;
				// Выполняем сброс состояния парсера
				::llhttp_reset(&this->_parser);
			}
			/**
			 * @brief Метод подачи фрагмента сообщения
			 *
			 * @param data данные фрагмента сообщения
			 * @param size размер фрагмента сообщения
			 * @return     результат разбора (false - фрагмент разобран с ошибкой)
			 *
			 */
			bool feed(const char * data, const size_t size) noexcept {
				// Выполняем разбор фрагмента сообщения
				return (::llhttp_execute(&this->_parser, data, size) == HPE_OK);
			}
			/**
			 * @brief Метод проверки завершённости разбора сообщения
			 *
			 * @return результат проверки (true - сообщение разобрано целиком)
			 *
			 */
			bool complete() const noexcept {
				// Выводим флаг завершённости разбора сообщения
				return this->_complete;
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Engine() noexcept : _settings{}, _parser{}, _complete(false) {
				// Выполняем инициализацию набора функций обратного вызова
				::llhttp_settings_init(&this->_settings);
				// Устанавливаем функцию обратного вызова получения названия заголовка
				this->_settings.on_header_field = &Engine::field;
				// Устанавливаем функцию обратного вызова получения значения заголовка
				this->_settings.on_header_value = &Engine::value;
				// Устанавливаем функцию обратного вызова получения тела сообщения
				this->_settings.on_body = &Engine::body;
				// Устанавливаем функцию обратного вызова завершения разбора сообщения
				this->_settings.on_message_complete = &Engine::finish;
				// Выполняем инициализацию парсера запросов
				::llhttp_init(&this->_parser, HTTP_REQUEST, &this->_settings);
				// Устанавливаем объект обвязки пользовательскими данными парсера
				this->_parser.data = this;
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
