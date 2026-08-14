/**
 * @file resolve.cpp
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
 * @brief Сценарии измерения работы с адресом при переходе по перенаправлению —
 *        разрешение относительной ссылки относительно основы и сличение происхождений
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
 * @brief Внутренние параметры и сценарии бенчмарков перехода по перенаправлению
 *
 */
namespace {
	/**
	 * @brief Пороги количества операций в секунду
	 *
	 * @details Пороги откалиброваны по отладочной сборке репозитория с двукратным
	 *          запасом: они ловят регрессию в разы, а не колебания планировщика
	 *          операционной системы
	 *
	 */
	static constexpr double RESOLVE_THRESHOLD = 650000.0;
	static constexpr double SAME_ORIGIN_THRESHOLD = 3400000.0;
	/**
	 * @brief Пороги количества выделений памяти на одну операцию
	 *
	 * @details Показатель от машины и режима сборки не зависит, поэтому пороги заданы
	 *          вплотную к измеренным значениям. Разрешению ссылки положено одно
	 *          выделение: набор сегментов пути объект держит при себе и заново под
	 *          него памяти не берёт, а платит лишь за сам новый сегмент. Сличению
	 *          же происхождений не положено ни одного - наружу оно ничего не
	 *          отдаёт, хост сличается по атрибутам, и строка нигде не собирается
	 *
	 */
	static constexpr double RESOLVE_ALLOCATIONS = 1.0;
	static constexpr double SAME_ORIGIN_ALLOCATIONS = 0.0;

	/**
	 * @brief Образец основы, относительно которой разрешается ссылка
	 *
	 * @details Основой служит адрес запроса, на который пришёл ответ с
	 *          перенаправлением: путь его ведёт от корня и оканчивается именем
	 *          ресурса, а не косой чертой, - последний сегмент такого пути при
	 *          разрешении относительной ссылки снимается (RFC 3986 5.3)
	 *
	 */
	static constexpr const char * SAMPLE_BASE = "https://example.com/a/b/c/page.html?x=1#f";
	/**
	 * @brief Образцы относительных ссылок, разрешаемых относительно основы
	 *
	 * @details Ссылки подобраны взаимно обратными: разрешение первой уводит с
	 *          "page.html" на "other.html", разрешение второй возвращает обратно, и
	 *          через два прохода объект приходит в то же состояние. Иначе путь на
	 *          каждом проходе рос бы или укорачивался, и замер измерял бы работу с
	 *          путём переменной длины - показатель, от прогона к прогону разный.
	 *
	 *          Параметры заданы у обеих: ссылка, параметров не несущая, снимает их с
	 *          основы (RFC 3986 5.2.2), и первый же проход оставил бы объект без них
	 *
	 */
	static constexpr const char * SAMPLES_REFERENCE[2] = {
		"other.html?y=2",
		"page.html?x=1"
	};
	/**
	 * @brief Образцы адресов одного происхождения
	 *
	 * @details Сличаются заведомо совпадающие происхождения: несовпадение
	 *          обнаруживается первым же разошедшимся полем и обходится тем дешевле,
	 *          чем раньше это поле стоит в порядке сличения. Замер на совпадающих
	 *          происхождениях измеряет полный проход - наихудший случай, он же
	 *          единственный воспроизводимый.
	 *
	 *          Записи различаются всем, что в происхождение не входит: учётными
	 *          данными, портом по умолчанию, путём, параметрами и якорем
	 *
	 */
	static constexpr const char * SAMPLE_ORIGIN_FIRST = "https://example.com/a/b/c/page.html?x=1#f";
	static constexpr const char * SAMPLE_ORIGIN_SECOND = "https://user:secret@example.com:443/other?y=2#g";

	/**
	 * @brief Функция прогона сценария разрешения относительной ссылки
	 *
	 * @details Разрешение ведётся присваиванием строки ссылки объекту, уже несущему
	 *          основу: именно так обрабатывается заголовок Location ответа с
	 *          перенаправлением - ссылка в нём бывает и полным адресом, и одним лишь
	 *          путём, и даже одними параметрами
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t resolving() noexcept {
		// Объект работы с идентификаторами ресурсов
		awh::uri_t object(framework(), logger());
		// Выполняем наполнение объекта разбором образца основы
		object.parse(SAMPLE_BASE);
		// Накопитель результатов разрешения ссылки
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(URI_ROUNDS, [&](const size_t index) noexcept {
			// Выполняем разрешение очередной ссылки относительно установленной основы
			object.parse(SAMPLES_REFERENCE[index & 1]);
			// Накапливаем результат разрешения ссылки
			summary += object.path().size();
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария сличения происхождений
	 *
	 * @details Сличение происхождений выполняется на каждом переходе по
	 *          перенаправлению: по его ответу решается, нести ли дальше учётные
	 *          данные и заголовок проверки подлинности (RFC 9110 15.4)
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t origins() noexcept {
		// Первый объект работы с идентификаторами ресурсов
		awh::uri_t first(framework(), logger());
		// Второй объект работы с идентификаторами ресурсов
		awh::uri_t second(framework(), logger());
		// Выполняем наполнение первого объекта разбором образца строки URI
		first.parse(SAMPLE_ORIGIN_FIRST);
		// Выполняем наполнение второго объекта разбором образца строки URI
		second.parse(SAMPLE_ORIGIN_SECOND);
		// Накопитель результатов сличения происхождений
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(LIGHT_ROUNDS, [&]([[maybe_unused]] const size_t index) noexcept {
			// Выполняем сличение происхождений с накоплением его результата
			summary += static_cast <uint64_t> (first.sameOrigin(second));
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}

	/**
	 * @brief Функция получения итогов прогона сценария разрешения ссылки
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & resolved() noexcept {
		// Итоги прогона сценария разрешения ссылки
		static const outcome_t result = ::resolving();
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона сценария сличения происхождений
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & originated() noexcept {
		// Итоги прогона сценария сличения происхождений
		static const outcome_t result = ::origins();
		// Выводим итоги прогона сценария
		return result;
	}

	// Объявляем сценарии разрешения относительной ссылки
	AWH_URI_SCENARIO(Resolve, ::resolved)
	// Объявляем сценарии сличения происхождений
	AWH_URI_SCENARIO(SameOrigin, ::originated)

	// Регистрируем сценарий скорости разрешения относительной ссылки
	static const bool gResolve = awh::benchmark::add(
		"net/uri/resolve", "разрешений/с", RESOLVE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedResolve
	);
	// Регистрируем сценарий выделений памяти на разрешение относительной ссылки
	static const bool gMemoryResolve = awh::benchmark::add(
		"net/uri/resolve/allocations", "выделений", RESOLVE_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryResolve
	);
	// Регистрируем сценарий скорости сличения происхождений
	static const bool gSameOrigin = awh::benchmark::add(
		"net/uri/same-origin", "сличений/с", SAME_ORIGIN_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedSameOrigin
	);
	// Регистрируем сценарий выделений памяти на сличение происхождений
	static const bool gMemorySameOrigin = awh::benchmark::add(
		"net/uri/same-origin/allocations", "выделений", SAME_ORIGIN_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memorySameOrigin
	);
};
