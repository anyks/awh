/**
 * @file headers.cpp
 * @date 2026-07-31
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
 * @brief Общее окружение бенчмарков контейнера HTTP-заголовков
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл бенчмарков
 */
#include "headers.hpp"

/**
 * @brief Инкапсулируем образцы полей в пространство имён
 *
 */
namespace {
	/**
	 * @brief Названия полей образца обычного запроса клиента
	 *
	 * @details Набор снят с обычного запроса обозревателя: десять полей, два из них
	 *          одноимённых. Кратность в образце намеренная - контейнер обязан её
	 *          сохранять, и сценарии обязаны мерить его на той работе, которую он
	 *          выполняет в обмене, а не на упрощённом наборе без дубликатов
	 *
	 * @note Образцы совпадают с образцами стендов сравнения в
	 *       tools/benchmark/headers/scenarios.hpp. Разойдутся они - и сравнение
	 *       с соперниками начнёт мерить разницу образцов, а не реализаций
	 *
	 */
	static constexpr const char * NAMES[awh::benchmark::headers::FIELDS] = {
		"Host", "User-Agent", "Accept", "Accept-Encoding", "Accept-Language",
		"Connection", "Cache-Control", "Set-Cookie", "Set-Cookie", "Content-Length"
	};
	/**
	 * @brief Значения полей образца обычного запроса клиента
	 *
	 */
	static constexpr const char * VALUES[awh::benchmark::headers::FIELDS] = {
		"example.com",
		"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36",
		"text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8",
		"gzip, deflate, br",
		"ru-RU,ru;q=0.9,en-US;q=0.8,en;q=0.7",
		"keep-alive",
		"no-cache",
		"session=abcdef0123456789; Path=/; HttpOnly",
		"theme=dark; Path=/; Max-Age=31536000",
		"1024"
	};
};

/**
 * @brief Функция получения объекта фреймворка сценариев
 *
 * @return объект фреймворка сценариев
 *
 */
const awh::fmk_t * awh::benchmark::headers::framework() noexcept {
	// Объект фреймворка сценариев
	static fmk_t result;
	// Выводим объект фреймворка сценариев
	return &result;
}
/**
 * @brief Функция получения объекта логирования сценариев
 *
 * @return объект логирования сценариев
 *
 */
const awh::log_t * awh::benchmark::headers::logger() noexcept {
	// Объект логирования сценариев
	static log_t result(framework());
	/**
	 * Отключаем вывод логов: контейнер записывает предупреждением понижение
	 * ограничений, а сценарии выводят только показатели
	 */
	result.level(log_t::level_t::NONE);
	// Выводим объект логирования сценариев
	return &result;
}
/**
 * @brief Функция получения накопителя итогов работы сценариев
 *
 * @return ссылка на накопитель итогов
 *
 */
uint64_t & awh::benchmark::headers::checksum() noexcept {
	// Накопитель итогов работы сценариев
	static uint64_t result = 0;
	// Выводим ссылку на накопитель итогов
	return result;
}
/**
 * @brief Функция получения названия поля образца
 *
 * @param index порядковый номер поля образца
 * @return      название поля образца
 *
 */
const char * awh::benchmark::headers::name(const size_t index) noexcept {
	// Выводим название поля образца
	return NAMES[index % FIELDS];
}
/**
 * @brief Функция получения значения поля образца
 *
 * @param index порядковый номер поля образца
 * @return      значение поля образца
 *
 */
const char * awh::benchmark::headers::value(const size_t index) noexcept {
	// Выводим значение поля образца
	return VALUES[index % FIELDS];
}
/**
 * @brief Функция наполнения контейнера полями образца
 *
 * @param object наполняемый контейнер заголовков
 *
 */
void awh::benchmark::headers::fill(http::headers_t & object) noexcept {
	/**
	 * Проходим по всем полям образца
	 */
	for(size_t i = 0; i < FIELDS; i++)
		// Добавляем очередное поле образца, сохраняя одноимённые
		object.emplace(NAMES[i], VALUES[i], http::headers_t::mode_t::APPEND);
}
