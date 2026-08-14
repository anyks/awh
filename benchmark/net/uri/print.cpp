/**
 * @file print.cpp
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
 * @brief Сценарии измерения сборки строки URI — полный адрес, относительный адрес запроса,
 *        происхождение ресурса и адрес, требующий процентного кодирования
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
 * @brief Внутренние параметры и сценарии бенчмарков сборки строки URI
 *
 */
namespace {
	/**
	 * @brief Пороги количества сборок в секунду
	 *
	 * @details Пороги откалиброваны по отладочной сборке репозитория с двукратным
	 *          запасом: они ловят регрессию в разы, а не колебания планировщика
	 *          операционной системы
	 *
	 */
	static constexpr double PRINT_URI_THRESHOLD = 1100000.0;
	static constexpr double PRINT_REQUEST_THRESHOLD = 1400000.0;
	static constexpr double PRINT_ORIGIN_THRESHOLD = 9000000.0;
	static constexpr double PRINT_ENCODED_THRESHOLD = 900000.0;
	static constexpr double PRINT_IPV6_THRESHOLD = 650000.0;
	static constexpr double PRINT_HOST_THRESHOLD = 10000000.0;
	/**
	 * @brief Пороги количества выделений памяти на одну сборку
	 *
	 * @details Показатель от машины и режима сборки не зависит, поэтому пороги заданы
	 *          вплотную к измеренным значениям. Сборка строки URI обязана выделять
	 *          память ровно под возвращаемую наружу строку - одно выделение, а при
	 *          длине сверх исходной ёмкости накопителя ещё несколько на его рост.
	 *          Всякое выделение сверх этого приходится на промежуточные строки,
	 *          наружу не отдаваемые, и есть работа, которой можно не делать
	 *
	 */
	static constexpr double PRINT_URI_ALLOCATIONS = 2.0;
	static constexpr double PRINT_REQUEST_ALLOCATIONS = 1.0;
	static constexpr double PRINT_ORIGIN_ALLOCATIONS = 0.0;
	static constexpr double PRINT_ENCODED_ALLOCATIONS = 3.0;
	static constexpr double PRINT_IPV6_ALLOCATIONS = 2.0;
	static constexpr double PRINT_HOST_ALLOCATIONS = 0.0;

	/**
	 * @brief Собираемый полный адрес запроса
	 *
	 * @details Тот же образец, что и у сценария разбора: набор составляющих у него
	 *          полный, и сборка проходит по всем ветвям - схема, хост, порт, путь,
	 *          параметры и якорь
	 *
	 */
	static constexpr const char * SAMPLE_REQUEST = "http://www.example.com:8080/path/to/resource?query=1&id=123#frag";
	/**
	 * @brief Собираемое происхождение ресурса
	 *
	 * @details Ни пути, ни параметров, ни якоря: сценарий измеряет постоянные
	 *          накладные расходы сборки в отрыве от длины строки
	 *
	 */
	static constexpr const char * SAMPLE_ORIGIN = "https://example.com";
	/**
	 * @brief Собираемый адрес, требующий процентного кодирования
	 *
	 * @details Кириллица в пути, в параметрах и в якоре: сборка обязана закодировать
	 *          каждый символ, не входящий в незарезервированный набор. Сценарий
	 *          измеряет кодирование, доступа к которому снаружи модуля нет - оно
	 *          вызывается только сборкой
	 *
	 */
	static constexpr const char * SAMPLE_ENCODED = "http://example.com/%D0%BF%D1%83%D1%82%D1%8C/to%20file?q=%D0%B0%D0%B1%D0%B2&x=%41%42%43#%D1%8F%D0%BA%D0%BE%D1%80%D1%8C";
	/**
	 * @brief Собираемый адрес с IPv6-хостом
	 *
	 * @details Хост хранится двоичным представлением, поэтому сборка выполняет обратный
	 *          перевод адреса в строку средствами модуля работы с адресами. Остальные
	 *          сценарии сборки построены на доменном имени, которое хранится готовой
	 *          строкой, и эту ветвь не задевают вовсе.
	 *
	 *          Путь, параметры и якорь совпадают с образцом полного адреса запроса, а
	 *          отличается только хост: разность показателей двух сценариев тогда
	 *          относится к хосту и ни к чему иному
	 *
	 */
	static constexpr const char * SAMPLE_IPV6 = "http://[2001:db8:85a3::8a2e:370:7334]:8080/path/to/resource?query=1&id=123#frag";

	/**
	 * @brief Функция прогона сценария сборки строки URI
	 *
	 * @param sample образец строки URI для наполнения объекта
	 * @param item   режим элемента URI для сборки
	 * @param rounds количество выполняемых сборок
	 * @return       итоги прогона сценария
	 *
	 */
	static outcome_t printing(const char * sample, const awh::uri_t::item_t item, const size_t rounds) noexcept {
		// Объект работы с идентификаторами ресурсов
		awh::uri_t object(framework(), logger());
		// Выполняем наполнение объекта разбором образца строки URI
		object.parse(sample);
		// Накопитель результатов сборки строки URI
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(rounds, [&]([[maybe_unused]] const size_t index) noexcept {
			// Выполняем сборку строки URI с накоплением её длины
			summary += object.print(item).size();
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}

	/**
	 * @brief Функция получения итогов прогона сценария сборки полного адреса
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & printedURI() noexcept {
		// Итоги прогона сценария сборки полного адреса
		static const outcome_t result = ::printing(SAMPLE_REQUEST, awh::uri_t::item_t::URI, URI_ROUNDS);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона сценария сборки адреса запроса
	 *
	 * @details Относительный адрес запроса собирается на каждом исходящем запросе
	 *          протокола HTTP: именно он ставится в стартовую строку
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & printedRequest() noexcept {
		// Итоги прогона сценария сборки адреса запроса
		static const outcome_t result = ::printing(SAMPLE_REQUEST, awh::uri_t::item_t::REQUEST, URI_ROUNDS);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона сценария сборки происхождения ресурса
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & printedOrigin() noexcept {
		// Итоги прогона сценария сборки происхождения ресурса
		static const outcome_t result = ::printing(SAMPLE_ORIGIN, awh::uri_t::item_t::ORIGIN, URI_ROUNDS);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона сценария сборки с процент-кодированием
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & printedEncoded() noexcept {
		// Итоги прогона сценария сборки с процент-кодированием
		static const outcome_t result = ::printing(SAMPLE_ENCODED, awh::uri_t::item_t::URI, URI_ROUNDS);
		// Выводим итоги прогона сценария
		return result;
	}

	/**
	 * @brief Функция получения итогов прогона сценария сборки с IPv6-хостом
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & printedIPv6() noexcept {
		// Итоги прогона сценария сборки с IPv6-хостом
		static const outcome_t result = ::printing(SAMPLE_IPV6, awh::uri_t::item_t::URI, URI_ROUNDS);
		// Выводим итоги прогона сценария
		return result;
	}

	// Объявляем сценарии сборки полного адреса
	AWH_URI_SCENARIO(PrintURI, ::printedURI)
	// Объявляем сценарии сборки адреса запроса
	AWH_URI_SCENARIO(PrintRequest, ::printedRequest)
	// Объявляем сценарии сборки происхождения ресурса
	AWH_URI_SCENARIO(PrintOrigin, ::printedOrigin)
	// Объявляем сценарии сборки с процент-кодированием
	AWH_URI_SCENARIO(PrintEncoded, ::printedEncoded)
	// Объявляем сценарии сборки с IPv6-хостом
	/**
	 * @brief Функция получения итогов прогона сценария сборки значения заголовка Host
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & printedHost() noexcept {
		// Итоги прогона сценария сборки значения заголовка Host
		static const outcome_t result = ::printing(SAMPLE_REQUEST, awh::uri_t::item_t::HOST, URI_ROUNDS);
		// Выводим итоги прогона сценария
		return result;
	}

	AWH_URI_SCENARIO(PrintIPv6, ::printedIPv6)
	// Объявляем сценарии сборки значения заголовка Host
	AWH_URI_SCENARIO(PrintHost, ::printedHost)

	// Регистрируем сценарий скорости сборки полного адреса
	static const bool gPrintURI = awh::benchmark::add(
		"net/uri/print-uri", "сборок/с", PRINT_URI_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedPrintURI
	);
	// Регистрируем сценарий выделений памяти на сборку полного адреса
	static const bool gMemoryPrintURI = awh::benchmark::add(
		"net/uri/print-uri/allocations", "выделений", PRINT_URI_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryPrintURI
	);
	// Регистрируем сценарий скорости сборки адреса запроса
	static const bool gPrintRequest = awh::benchmark::add(
		"net/uri/print-request", "сборок/с", PRINT_REQUEST_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedPrintRequest
	);
	// Регистрируем сценарий выделений памяти на сборку адреса запроса
	static const bool gMemoryPrintRequest = awh::benchmark::add(
		"net/uri/print-request/allocations", "выделений", PRINT_REQUEST_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryPrintRequest
	);
	// Регистрируем сценарий скорости сборки происхождения ресурса
	static const bool gPrintOrigin = awh::benchmark::add(
		"net/uri/print-origin", "сборок/с", PRINT_ORIGIN_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedPrintOrigin
	);
	// Регистрируем сценарий выделений памяти на сборку происхождения ресурса
	static const bool gMemoryPrintOrigin = awh::benchmark::add(
		"net/uri/print-origin/allocations", "выделений", PRINT_ORIGIN_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryPrintOrigin
	);
	// Регистрируем сценарий скорости сборки с процент-кодированием
	static const bool gPrintEncoded = awh::benchmark::add(
		"net/uri/print-encoded", "сборок/с", PRINT_ENCODED_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedPrintEncoded
	);
	// Регистрируем сценарий выделений памяти на сборку с процент-кодированием
	static const bool gMemoryPrintEncoded = awh::benchmark::add(
		"net/uri/print-encoded/allocations", "выделений", PRINT_ENCODED_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryPrintEncoded
	);
	// Регистрируем сценарий скорости сборки с IPv6-хостом
	static const bool gPrintIPv6 = awh::benchmark::add(
		"net/uri/print-ipv6", "сборок/с", PRINT_IPV6_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedPrintIPv6
	);
	// Регистрируем сценарий выделений памяти на сборку с IPv6-хостом
	static const bool gMemoryPrintIPv6 = awh::benchmark::add(
		"net/uri/print-ipv6/allocations", "выделений", PRINT_IPV6_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryPrintIPv6
	);
	// Регистрируем сценарий скорости сборки значения заголовка Host
	static const bool gPrintHost = awh::benchmark::add(
		"net/uri/print-host", "сборок/с", PRINT_HOST_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedPrintHost
	);
	// Регистрируем сценарий выделений памяти на сборку значения заголовка Host
	static const bool gMemoryPrintHost = awh::benchmark::add(
		"net/uri/print-host/allocations", "выделений", PRINT_HOST_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryPrintHost
	);
};
