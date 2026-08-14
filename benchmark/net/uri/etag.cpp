/**
 * @file etag.cpp
 * @date 2026-07-29
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
 * @brief Сценарии измерения выработки хэша ETag — короткая и длинная строки
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл бенчмарков модуля работы с идентификаторами ресурсов
 */
#include "uri.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён бенчмарков модуля работы с идентификаторами ресурсов
 */
using namespace awh::benchmark::uri;

/**
 * @brief Внутренние параметры и сценарии бенчмарков выработки хэша ETag
 *
 */
namespace {
	/**
	 * @brief Пороги количества выработок хэша в секунду
	 *
	 * @details Пороги откалиброваны по отладочной сборке репозитория с двукратным
	 *          запасом: они ловят регрессию в разы, а не колебания планировщика
	 *          операционной системы
	 *
	 */
	static constexpr double ETAG_SHORT_THRESHOLD = 7000000.0;
	static constexpr double ETAG_LONG_THRESHOLD = 1800000.0;
	/**
	 * @brief Пороги количества выделений памяти на одну выработку хэша
	 *
	 * @details Показатель от машины и режима сборки не зависит, поэтому пороги заданы
	 *          вплотную к измеренным значениям. Выработка хэша - это обход строки с
	 *          накоплением 64-разрядного числа и запись его шестнадцатеричной записи
	 *          в строку из восемнадцати символов. Строка эта короче порога размещения
	 *          внутри самого объекта, поэтому выделять память выработка не обязана
	 *          вовсе, и всякое выделение здесь есть работа, которой можно не делать
	 *
	 */
	static constexpr double ETAG_SHORT_ALLOCATIONS = 0.0;
	static constexpr double ETAG_LONG_ALLOCATIONS = 0.0;

	/**
	 * @brief Хэшируемая короткая строка
	 *
	 * @details Строка параметров запроса - именно такой длины строку модуль хэширует
	 *          при выработке контрольной суммы адреса
	 *
	 */
	static constexpr const char * SAMPLE_SHORT = "id=123&query=1";
	/**
	 * @brief Хэшируемая длинная строка
	 *
	 * @details Заголовок ответа или тело небольшого документа: сценарий отделяет
	 *          постоянные накладные расходы выработки от стоимости обхода строки
	 *
	 */
	static constexpr const char * SAMPLE_LONG =
		"HTTP/1.1 200 OK\r\nServer: anyks\r\nContent-Type: text/html; charset=utf-8\r\n"
		"Content-Length: 4096\r\nCache-Control: no-cache, no-store, must-revalidate\r\n"
		"Date: Tue, 29 Jul 2026 11:22:33 GMT\r\nConnection: keep-alive\r\n\r\n";

	/**
	 * @brief Функция прогона сценария выработки хэша ETag
	 *
	 * @param sample хэшируемая строка
	 * @param rounds количество выполняемых выработок хэша
	 * @return       итоги прогона сценария
	 *
	 */
	static outcome_t hashing(const char * sample, const size_t rounds) noexcept {
		// Объект работы с идентификаторами ресурсов
		awh::uri_t object(framework(), logger());
		// Накопитель результатов выработки хэша
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(rounds, [&]([[maybe_unused]] const size_t index) noexcept {
			// Выполняем выработку хэша с накоплением его длины
			summary += object.etag(sample).size();
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}

	/**
	 * @brief Функция получения итогов прогона сценария выработки хэша короткой строки
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & hashedShort() noexcept {
		// Итоги прогона сценария выработки хэша короткой строки
		static const outcome_t result = ::hashing(SAMPLE_SHORT, LIGHT_ROUNDS);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона сценария выработки хэша длинной строки
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & hashedLong() noexcept {
		// Итоги прогона сценария выработки хэша длинной строки
		static const outcome_t result = ::hashing(SAMPLE_LONG, URI_ROUNDS);
		// Выводим итоги прогона сценария
		return result;
	}

	// Объявляем сценарии выработки хэша короткой строки
	AWH_URI_SCENARIO(ETagShort, ::hashedShort)
	// Объявляем сценарии выработки хэша длинной строки
	AWH_URI_SCENARIO(ETagLong, ::hashedLong)

	// Регистрируем сценарий скорости выработки хэша короткой строки
	static const bool gETagShort = awh::benchmark::add(
		"net/uri/etag-short", "хэшей/с", ETAG_SHORT_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedETagShort
	);
	// Регистрируем сценарий выделений памяти на выработку хэша короткой строки
	static const bool gMemoryETagShort = awh::benchmark::add(
		"net/uri/etag-short/allocations", "выделений", ETAG_SHORT_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryETagShort
	);
	// Регистрируем сценарий скорости выработки хэша длинной строки
	static const bool gETagLong = awh::benchmark::add(
		"net/uri/etag-long", "хэшей/с", ETAG_LONG_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedETagLong
	);
	// Регистрируем сценарий выделений памяти на выработку хэша длинной строки
	static const bool gMemoryETagLong = awh::benchmark::add(
		"net/uri/etag-long/allocations", "выделений", ETAG_LONG_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryETagLong
	);
};
