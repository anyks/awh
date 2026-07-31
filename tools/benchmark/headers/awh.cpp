/**
 * @file: awh.cpp
 * @date: 2026-07-31
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Стенд сравнения контейнера HTTP-заголовков — реализация AWH
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы стендов
 */
#include "driver.hpp"

/**
 * Подключаем заголовочный файлы проекта
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <proto/http/headers.hpp>

/**
 * @brief Инкапсулируем сценарии стенда в пространство имён
 *
 */
namespace {
	/**
	 * @brief Функция получения объекта фреймворка стенда
	 *
	 * @return объект фреймворка стенда
	 *
	 */
	static awh::fmk_t * framework() noexcept {
		// Объект фреймворка стенда
		static awh::fmk_t result;
		// Выводим объект фреймворка стенда
		return &result;
	}
	/**
	 * @brief Функция получения объекта логирования стенда
	 *
	 * @return объект логирования стенда
	 *
	 */
	static awh::log_t * logger() noexcept {
		// Объект логирования стенда
		static awh::log_t result(framework());
		// Отключаем вывод логов: стенд выводит только показатели
		result.level(awh::log_t::level_t::NONE);
		// Выводим объект логирования стенда
		return &result;
	}
	/**
	 * @brief Функция наполнения контейнера полями образца
	 *
	 * @param headers наполняемый контейнер заголовков
	 *
	 */
	static void fill(awh::http::headers_t & headers) noexcept {
		/**
		 * Проходим по всем полям образца
		 */
		for(size_t i = 0; i < scenarios::FIELDS; i++)
			// Добавляем очередное поле образца, сохраняя одноимённые
			headers.emplace(scenarios::NAMES[i], scenarios::VALUES[i], awh::http::headers_t::mode_t::APPEND);
	}
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
	// Выполняем сценарий наполнения контейнера полями запроса
	driver::execute("headers/build/request", "наборов/с", scenarios::ROUNDS, mask, []([[maybe_unused]] const size_t index) noexcept -> uint64_t {
		// Создаём контейнер заголовков
		awh::http::headers_t headers(awh::http::proto_t::HTTP1, framework(), logger());
		// Наполняем контейнер полями образца
		::fill(headers);
		// Выводим итог наполнения контейнера
		return static_cast <uint64_t> (headers.size());
	});
	/**
	 * Выполняем сценарий поиска поля по названию без учёта регистра
	 */
	{
		// Создаём контейнер заголовков сценария
		static awh::http::headers_t headers(awh::http::proto_t::HTTP1, framework(), logger());
		// Наполняем контейнер полями образца
		::fill(headers);
		// Выполняем сценарий поиска поля по названию
		driver::execute("headers/lookup/hit", "поисков/с", scenarios::LIGHT_ROUNDS, mask, [](const size_t index) noexcept -> uint64_t {
			// Выводим длину найденного значения поля
			return static_cast <uint64_t> (headers.at(scenarios::LOOKUPS[index & 1]).size());
		});
	}
	/**
	 * Выполняем сценарий сборки сообщения в виде, пригодном для передачи
	 */
	{
		// Создаём контейнер заголовков сценария
		static awh::http::headers_t headers(awh::http::proto_t::HTTP1, framework(), logger());
		// Создаём объект запроса клиента
		static awh::http::request_t request(awh::http::version_t::HTTP1_1, awh::http::method_t::GET, std::string("/index.html"));
		// Устанавливаем провайдер запроса
		headers.provider(&request);
		// Наполняем контейнер полями образца
		::fill(headers);
		// Выполняем сценарий сборки сообщения
		driver::execute("headers/serialize/request", "сборок/с", scenarios::ROUNDS, mask, []([[maybe_unused]] const size_t index) noexcept -> uint64_t {
			// Выводим длину собранного сообщения
			return static_cast <uint64_t> (headers.print(awh::http::proto_t::HTTP1).size());
		});
	}
	/**
	 * Выполняем сценарий замены значения существующего поля
	 */
	{
		// Создаём контейнер заголовков сценария
		static awh::http::headers_t headers(awh::http::proto_t::HTTP1, framework(), logger());
		// Наполняем контейнер полями образца
		::fill(headers);
		// Выполняем сценарий замены значения поля
		driver::execute("headers/replace/existing", "замен/с", scenarios::LIGHT_ROUNDS, mask, [](const size_t index) noexcept -> uint64_t {
			// Выполняем замену значения поля очередным образцом
			return static_cast <uint64_t> (headers.emplace(
				scenarios::REPLACE, scenarios::REPLACEMENTS[index & 1],
				awh::http::headers_t::mode_t::REPLACE
			));
		});
	}
	// Выводим накопленные итоги работы сценариев
	driver::digest(argc, argv);
	// Выводим успешный код выхода
	return 0;
}
