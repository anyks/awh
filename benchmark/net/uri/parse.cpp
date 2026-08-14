/**
 * @file parse.cpp
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
 * @brief Сценарии измерения разбора строки URI — происхождение, полный адрес запроса,
 *        адрес с IPv6-хостом, адрес с параметрами пользователя и адрес с процент-кодированием
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
 * @brief Внутренние параметры и сценарии бенчмарков разбора строки URI
 *
 */
namespace {
	/**
	 * @brief Пороги количества разборов в секунду
	 *
	 * @details Пороги откалиброваны по отладочной сборке репозитория с двукратным
	 *          запасом: они ловят регрессию в разы, а не колебания планировщика
	 *          операционной системы
	 *
	 */
	static constexpr double PARSE_ORIGIN_THRESHOLD = 1000000.0;
	static constexpr double PARSE_REQUEST_THRESHOLD = 250000.0;
	static constexpr double PARSE_IPV6_THRESHOLD = 200000.0;
	static constexpr double PARSE_USERINFO_THRESHOLD = 700000.0;
	static constexpr double PARSE_ENCODED_THRESHOLD = 240000.0;
	/**
	 * @brief Пороги количества выделений памяти на один разбор
	 *
	 * @details Показатель от машины и режима сборки не зависит, поэтому пороги заданы
	 *          вплотную к измеренным значениям. Они и есть главный показатель набора:
	 *          разбор строки URI обязан выделять память лишь под те её части, которые
	 *          модуль сохраняет у себя, - схему, сегменты пути, пары параметров и
	 *          якорь. Всякое выделение сверх этого есть работа, которой можно не делать.
	 *
	 *          Порог разбора происхождения ресурса приподнят над измеренным на
	 *          сотую долю: оптимизированная сборка выполняет за прогон одно
	 *          постороннее выделение сверх ровно одного на операцию, и порог,
	 *          выставленный вплотную, отвергал бы её из-за единственного события на
	 *          сто тысяч операций
	 *
	 */
	static constexpr double PARSE_ORIGIN_ALLOCATIONS = 1.01;
	static constexpr double PARSE_REQUEST_ALLOCATIONS = 3.0;
	static constexpr double PARSE_IPV6_ALLOCATIONS = 4.0;
	static constexpr double PARSE_USERINFO_ALLOCATIONS = 1.0;
	static constexpr double PARSE_ENCODED_ALLOCATIONS = 5.0;

	/**
	 * @brief Разбираемая строка происхождения ресурса
	 *
	 * @details Кратчайший из встречающихся на практике URI: ни пути, ни параметров,
	 *          ни якоря. Такой вид имеет заголовок Origin, такой же приходит адресом
	 *          назначения при работе через посредника. Сценарий измеряет постоянные
	 *          накладные расходы разбора в отрыве от длины строки
	 *
	 */
	static constexpr const char * SAMPLE_ORIGIN = "https://example.com";
	/**
	 * @brief Разбираемая строка полного адреса запроса
	 *
	 * @details Обыкновенный адрес запроса протокола HTTP: схема, доменное имя, путь
	 *          из нескольких сегментов, пара параметров запроса и якорь. Именно такой
	 *          разбор выполняется на каждом принятом запросе
	 *
	 */
	static constexpr const char * SAMPLE_REQUEST = "http://www.example.com/path/to/resource?query=1&id=123#frag";
	/**
	 * @brief Разбираемая строка адреса с IPv6-хостом
	 *
	 * @details Хост в квадратных скобках вынуждает разбор различать двоеточия адреса
	 *          и двоеточие порта, а это единственное место разбора, где он смотрит
	 *          вперёд по строке. Сценарий ловит регрессию именно в нём
	 *
	 */
	static constexpr const char * SAMPLE_IPV6 = "http://[2001:db8:85a3::8a2e:370:7334]:8080/api/v1/items?page=2";
	/**
	 * @brief Разбираемая строка адреса с параметрами пользователя
	 *
	 * @details Вид, в котором задаётся посредник Socks5: логин и пароль стоят до
	 *          символа "@", и разбор обязан отличить двоеточие между ними от
	 *          двоеточия перед портом
	 *
	 */
	static constexpr const char * SAMPLE_USERINFO = "socks5://user:password@proxy.example.com:1080";
	/**
	 * @brief Разбираемая строка адреса с процент-кодированием
	 *
	 * @details Кириллица в пути, в параметрах и в якоре: разбор обязан раскодировать
	 *          каждую процент-последовательность. Сценарий измеряет раскодирование,
	 *          доступа к которому снаружи модуля нет - оно вызывается только разбором
	 *
	 */
	static constexpr const char * SAMPLE_ENCODED = "http://example.com/%D0%BF%D1%83%D1%82%D1%8C/to%20file?q=%D0%B0%D0%B1%D0%B2&x=%41%42%43#%D1%8F%D0%BA%D0%BE%D1%80%D1%8C";

	/**
	 * @brief Функция прогона сценария разбора строки URI
	 *
	 * @param sample разбираемая строка URI
	 * @param rounds количество выполняемых разборов
	 * @return       итоги прогона сценария
	 *
	 */
	static outcome_t parsing(const char * sample, const size_t rounds) noexcept {
		// Объект работы с идентификаторами ресурсов
		awh::uri_t object(framework(), logger());
		// Накопитель результатов разбора строки URI
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(rounds, [&]([[maybe_unused]] const size_t index) noexcept {
			// Выполняем разбор строки URI с накоплением её типа
			summary += static_cast <uint64_t> (object.parse(sample));
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}

	/**
	 * @brief Функция получения итогов прогона сценария разбора происхождения ресурса
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & parsedOrigin() noexcept {
		// Итоги прогона сценария разбора происхождения ресурса
		static const outcome_t result = ::parsing(SAMPLE_ORIGIN, URI_ROUNDS);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона сценария разбора адреса запроса
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & parsedRequest() noexcept {
		// Итоги прогона сценария разбора адреса запроса
		static const outcome_t result = ::parsing(SAMPLE_REQUEST, URI_ROUNDS);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона сценария разбора адреса с IPv6-хостом
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & parsedIPv6() noexcept {
		// Итоги прогона сценария разбора адреса с IPv6-хостом
		static const outcome_t result = ::parsing(SAMPLE_IPV6, URI_ROUNDS);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона сценария разбора параметров пользователя
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & parsedUserinfo() noexcept {
		// Итоги прогона сценария разбора параметров пользователя
		static const outcome_t result = ::parsing(SAMPLE_USERINFO, URI_ROUNDS);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона сценария разбора процент-кодирования
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & parsedEncoded() noexcept {
		// Итоги прогона сценария разбора процент-кодирования
		static const outcome_t result = ::parsing(SAMPLE_ENCODED, URI_ROUNDS);
		// Выводим итоги прогона сценария
		return result;
	}

	// Объявляем сценарии разбора происхождения ресурса
	AWH_URI_SCENARIO(ParseOrigin, ::parsedOrigin)
	// Объявляем сценарии разбора адреса запроса
	AWH_URI_SCENARIO(ParseRequest, ::parsedRequest)
	// Объявляем сценарии разбора адреса с IPv6-хостом
	AWH_URI_SCENARIO(ParseIPv6, ::parsedIPv6)
	// Объявляем сценарии разбора параметров пользователя
	AWH_URI_SCENARIO(ParseUserinfo, ::parsedUserinfo)
	// Объявляем сценарии разбора процент-кодирования
	AWH_URI_SCENARIO(ParseEncoded, ::parsedEncoded)

	// Регистрируем сценарий скорости разбора происхождения ресурса
	static const bool gParseOrigin = awh::benchmark::add(
		"net/uri/parse-origin", "разборов/с", PARSE_ORIGIN_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedParseOrigin
	);
	// Регистрируем сценарий выделений памяти на разбор происхождения ресурса
	static const bool gMemoryParseOrigin = awh::benchmark::add(
		"net/uri/parse-origin/allocations", "выделений", PARSE_ORIGIN_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryParseOrigin
	);
	// Регистрируем сценарий скорости разбора адреса запроса
	static const bool gParseRequest = awh::benchmark::add(
		"net/uri/parse-request", "разборов/с", PARSE_REQUEST_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedParseRequest
	);
	// Регистрируем сценарий выделений памяти на разбор адреса запроса
	static const bool gMemoryParseRequest = awh::benchmark::add(
		"net/uri/parse-request/allocations", "выделений", PARSE_REQUEST_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryParseRequest
	);
	// Регистрируем сценарий скорости разбора адреса с IPv6-хостом
	static const bool gParseIPv6 = awh::benchmark::add(
		"net/uri/parse-ipv6", "разборов/с", PARSE_IPV6_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedParseIPv6
	);
	// Регистрируем сценарий выделений памяти на разбор адреса с IPv6-хостом
	static const bool gMemoryParseIPv6 = awh::benchmark::add(
		"net/uri/parse-ipv6/allocations", "выделений", PARSE_IPV6_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryParseIPv6
	);
	// Регистрируем сценарий скорости разбора параметров пользователя
	static const bool gParseUserinfo = awh::benchmark::add(
		"net/uri/parse-userinfo", "разборов/с", PARSE_USERINFO_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedParseUserinfo
	);
	// Регистрируем сценарий выделений памяти на разбор параметров пользователя
	static const bool gMemoryParseUserinfo = awh::benchmark::add(
		"net/uri/parse-userinfo/allocations", "выделений", PARSE_USERINFO_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryParseUserinfo
	);
	// Регистрируем сценарий скорости разбора процент-кодирования
	static const bool gParseEncoded = awh::benchmark::add(
		"net/uri/parse-encoded", "разборов/с", PARSE_ENCODED_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedParseEncoded
	);
	// Регистрируем сценарий выделений памяти на разбор процент-кодирования
	static const bool gMemoryParseEncoded = awh::benchmark::add(
		"net/uri/parse-encoded/allocations", "выделений", PARSE_ENCODED_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryParseEncoded
	);
};
