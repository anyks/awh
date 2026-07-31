/**
 * @file: beast.cpp
 * @date: 2026-07-31
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Стенд сравнения контейнера HTTP-заголовков — реализация Boost.Beast
 *
 * @details Boost.Beast выбран эталоном как прямой ровесник по задаче: его
 *          [http::fields] хранит поля с сохранением порядка и кратности,
 *          ищет их без учёта регистра и умеет отдавать сообщение в виде,
 *          пригодном для передачи. Это тот же набор обязанностей, что и у
 *          нашего контейнера, а не выборка из него
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы стендов
 */
#include "driver.hpp"

/**
 * Стандартные заголовочные файлы
 */
#include <sstream>

/**
 * Подключаем заголовочный файлы сравниваемой реализации
 */
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

/**
 * Используем пространство имён сравниваемой реализации
 */
namespace http = boost::beast::http;

/**
 * @brief Инкапсулируем сценарии стенда в пространство имён
 *
 */
namespace {
	/**
	 * @brief Шаблон функции наполнения контейнера полями образца
	 *
	 * @tparam Fields тип наполняемого контейнера полей
	 *
	 */
	template <typename Fields>
	/**
	 * @brief Функция наполнения контейнера полями образца
	 *
	 * @param fields наполняемый контейнер полей
	 *
	 */
	static void fill(Fields & fields) noexcept {
		/**
		 * Проходим по всем полям образца
		 */
		for(size_t i = 0; i < scenarios::FIELDS; i++)
			// Добавляем очередное поле образца, сохраняя одноимённые
			fields.insert(scenarios::NAMES[i], scenarios::VALUES[i]);
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
		// Создаём контейнер полей
		http::fields fields;
		// Наполняем контейнер полями образца
		::fill(fields);
		// Выводим итог наполнения контейнера
		return static_cast <uint64_t> (std::distance(fields.begin(), fields.end()));
	});
	/**
	 * Выполняем сценарий поиска поля по названию без учёта регистра
	 */
	{
		// Создаём контейнер полей сценария
		static http::fields fields;
		// Наполняем контейнер полями образца
		::fill(fields);
		// Выполняем сценарий поиска поля по названию
		driver::execute("headers/lookup/hit", "поисков/с", scenarios::LIGHT_ROUNDS, mask, [](const size_t index) noexcept -> uint64_t {
			// Выводим длину найденного значения поля
			return static_cast <uint64_t> (fields[scenarios::LOOKUPS[index & 1]].size());
		});
	}
	/**
	 * Выполняем сценарий сборки сообщения в виде, пригодном для передачи
	 */
	{
		// Создаём объект запроса клиента
		static http::request <http::empty_body> request;
		// Устанавливаем метод запроса
		request.method(http::verb::get);
		// Устанавливаем цель запроса
		request.target("/index.html");
		// Устанавливаем версию протокола
		request.version(11);
		// Наполняем запрос полями образца
		::fill(request);
		// Выполняем сценарий сборки сообщения
		driver::execute("headers/serialize/request", "сборок/с", scenarios::ROUNDS, mask, []([[maybe_unused]] const size_t index) noexcept -> uint64_t {
			// Создаём поток сборки сообщения
			std::ostringstream stream;
			// Собираем сообщение в поток
			stream << request;
			// Выводим длину собранного сообщения
			return static_cast <uint64_t> (stream.str().size());
		});
	}
	/**
	 * Выполняем сценарий замены значения существующего поля
	 */
	{
		// Создаём контейнер полей сценария
		static http::fields fields;
		// Наполняем контейнер полями образца
		::fill(fields);
		// Выполняем сценарий замены значения поля
		driver::execute("headers/replace/existing", "замен/с", scenarios::LIGHT_ROUNDS, mask, [](const size_t index) noexcept -> uint64_t {
			// Выполняем замену значения поля очередным образцом
			fields.set(scenarios::REPLACE, scenarios::REPLACEMENTS[index & 1]);
			// Выводим итог замены значения поля
			return static_cast <uint64_t> (fields[scenarios::REPLACE].size());
		});
	}
	// Выводим накопленные итоги работы сценариев
	driver::digest(argc, argv);
	// Выводим успешный код выхода
	return 0;
}
