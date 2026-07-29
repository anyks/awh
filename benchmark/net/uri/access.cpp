/**
 * @file: access.cpp
 * @date: 2026-07-29
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Сценарии измерения обращений к разобранному URI — извлечение хоста доменного
 *        имени, извлечение хоста IPv6-адреса и сравнение двух идентификаторов
 *
 * @copyright: Copyright © 2026
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
 * @brief Внутренние параметры и сценарии бенчмарков обращений к разобранному URI
 *
 */
namespace {
	/**
	 * @brief Пороги количества обращений в секунду
	 *
	 * @details Пороги откалиброваны по отладочной сборке репозитория с двукратным
	 *          запасом: они ловят регрессию в разы, а не колебания планировщика
	 *          операционной системы
	 *
	 */
	/**
	 * @note Порог извлечения доменного имени отстоит от измеренного вчетверо, а не
	 *       вдвое: операция обходится в двадцать наносекунд, то есть в копию короткой
	 *       строки, и на таком времени разброс между прогонами доходит до двух раз.
	 *       Регрессию, которая вернёт в этот геттер работу, порог поймает и с таким
	 *       запасом - она стоила бы порядка величины
	 *
	 */
	static constexpr double HOST_FQDN_THRESHOLD = 20000000.0;
	static constexpr double HOST_IPV6_THRESHOLD = 1500000.0;
	static constexpr double HOST_SET_THRESHOLD = 600000.0;
	static constexpr double COMPARE_THRESHOLD = 1000000.0;
	/**
	 * @brief Пороги количества выделений памяти на одно обращение
	 *
	 * @details Показатель от машины и режима сборки не зависит, поэтому пороги заданы
	 *          вплотную к измеренным значениям. Извлечению хоста положено не более
	 *          одного выделения - под возвращаемую наружу строку, и то лишь когда она
	 *          длиннее порога размещения внутри самого объекта: доменное имя образца
	 *          короче него и не выделяет ничего, а запись IPv6-адреса длиннее и
	 *          выделяет однажды. Сравнению не положено ни одного: оно ничего наружу
	 *          не отдаёт
	 *
	 */
	static constexpr double HOST_FQDN_ALLOCATIONS = 0.0;
	static constexpr double HOST_IPV6_ALLOCATIONS = 1.0;
	static constexpr double HOST_SET_ALLOCATIONS = 0.0;
	static constexpr double COMPARE_ALLOCATIONS = 0.0;

	/**
	 * @brief Образец адреса с хостом доменного имени
	 *
	 * @details Извлечение хоста выполняется на каждом подключении: по нему строится
	 *          запрос к службе доменных имён и заголовок Host запроса. Доменное имя
	 *          хранится готовой строкой, поэтому извлечение его - это копия строки, и
	 *          показатель сценария высок не от быстроты работы, а от её отсутствия:
	 *          сценарий стоит здесь, чтобы поймать регрессию, которая работу сюда
	 *          вернёт
	 *
	 */
	static constexpr const char * SAMPLE_FQDN = "http://www.example.com:8080/path/to/resource?query=1&id=123";
	/**
	 * @brief Образец адреса с хостом IPv6-адреса
	 *
	 * @details Хост хранится двоичным представлением, поэтому его извлечение
	 *          выполняет обратный перевод адреса в строку - работу заметно большую,
	 *          чем выдача уже готового доменного имени
	 *
	 */
	static constexpr const char * SAMPLE_IPV6 = "http://[2001:db8:85a3::8a2e:370:7334]:8080/api/v1/items?page=2";
	/**
	 * @brief Набор образцов хоста сценария повторной установки
	 *
	 * @details Образцы одной разновидности и перебираются по кругу: установка хоста
	 *          той же разновидности, что уже заведена, обязана обойтись без выделения
	 *          памяти - объект под адрес уже есть, а содержимое его составляют
	 *          шестнадцать октетов. Один и тот же образец на каждом круге измерял бы
	 *          при этом не установку, а её отсутствие
	 *
	 */
	static constexpr const char * SAMPLES_SET[] = {
		"[2001:db8:85a3::8a2e:370:7334]", "[2606:4700:4700::1111]"
	};

	/**
	 * @brief Функция прогона сценария извлечения хоста URI
	 *
	 * @param sample образец строки URI для наполнения объекта
	 * @param rounds количество выполняемых извлечений хоста
	 * @return       итоги прогона сценария
	 *
	 */
	static outcome_t hosting(const char * sample, const size_t rounds) noexcept {
		// Объект работы с идентификаторами ресурсов
		awh::uri_t object(framework(), logger());
		// Выполняем наполнение объекта разбором образца строки URI
		object.parse(sample);
		// Накопитель результатов извлечения хоста
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(rounds, [&]([[maybe_unused]] const size_t index) noexcept {
			// Выполняем извлечение хоста с накоплением его длины
			summary += object.host().size();
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария повторной установки хоста URI
	 *
	 * @details Установка хоста выполняется на одном и том же объекте: именно так
	 *          работает клиент, перенаправляемый с адреса на адрес, и именно здесь
	 *          видно, заводит ли модуль объект под адрес заново или заполняет
	 *          заведённый
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t setting() noexcept {
		// Объект работы с идентификаторами ресурсов
		awh::uri_t object(framework(), logger());
		// Накопитель результатов установки хоста
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(URI_ROUNDS, [&](const size_t index) noexcept {
			// Выполняем установку очередного образца хоста
			object.host(SAMPLES_SET[index & 1]);
			// Накапливаем результат установки хоста
			summary += static_cast <uint64_t> (object.attr() != nullptr);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария сравнения двух идентификаторов
	 *
	 * @details Сравниваются заведомо равные идентификаторы: неравенство обнаруживается
	 *          первым же несовпавшим полем и обходится тем дешевле, чем раньше это
	 *          поле стоит в порядке сравнения. Замер на равных объектах измеряет
	 *          полный проход по всем полям - наихудший случай, он же единственный
	 *          воспроизводимый
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t comparison() noexcept {
		// Первый объект работы с идентификаторами ресурсов
		awh::uri_t first(framework(), logger());
		// Второй объект работы с идентификаторами ресурсов
		awh::uri_t second(framework(), logger());
		// Выполняем наполнение первого объекта разбором образца строки URI
		first.parse(SAMPLE_FQDN);
		// Выполняем наполнение второго объекта разбором того же образца
		second.parse(SAMPLE_FQDN);
		// Накопитель результатов сравнения
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(LIGHT_ROUNDS, [&]([[maybe_unused]] const size_t index) noexcept {
			// Выполняем сравнение идентификаторов с накоплением его результата
			summary += static_cast <uint64_t> (first == second);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}

	/**
	 * @brief Функция получения итогов прогона сценария извлечения доменного имени
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & hostedFQDN() noexcept {
		// Итоги прогона сценария извлечения доменного имени
		static const outcome_t result = ::hosting(SAMPLE_FQDN, LIGHT_ROUNDS);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона сценария извлечения IPv6-адреса
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & hostedIPv6() noexcept {
		// Итоги прогона сценария извлечения IPv6-адреса
		static const outcome_t result = ::hosting(SAMPLE_IPV6, URI_ROUNDS);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона сценария повторной установки хоста
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & settled() noexcept {
		// Итоги прогона сценария повторной установки хоста
		static const outcome_t result = ::setting();
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона сценария сравнения идентификаторов
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & compared() noexcept {
		// Итоги прогона сценария сравнения идентификаторов
		static const outcome_t result = ::comparison();
		// Выводим итоги прогона сценария
		return result;
	}

	// Объявляем сценарии извлечения доменного имени
	AWH_URI_SCENARIO(HostFQDN, ::hostedFQDN)
	// Объявляем сценарии извлечения IPv6-адреса
	AWH_URI_SCENARIO(HostIPv6, ::hostedIPv6)
	// Объявляем сценарии повторной установки хоста
	AWH_URI_SCENARIO(HostSet, ::settled)
	// Объявляем сценарии сравнения идентификаторов
	AWH_URI_SCENARIO(Compare, ::compared)

	// Регистрируем сценарий скорости извлечения доменного имени
	static const bool gHostFQDN = awh::benchmark::add(
		"net/uri/host-fqdn", "извлечений/с", HOST_FQDN_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedHostFQDN
	);
	// Регистрируем сценарий выделений памяти на извлечение доменного имени
	static const bool gMemoryHostFQDN = awh::benchmark::add(
		"net/uri/host-fqdn/allocations", "выделений", HOST_FQDN_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryHostFQDN
	);
	// Регистрируем сценарий скорости извлечения IPv6-адреса
	static const bool gHostIPv6 = awh::benchmark::add(
		"net/uri/host-ipv6", "извлечений/с", HOST_IPV6_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedHostIPv6
	);
	// Регистрируем сценарий выделений памяти на извлечение IPv6-адреса
	static const bool gMemoryHostIPv6 = awh::benchmark::add(
		"net/uri/host-ipv6/allocations", "выделений", HOST_IPV6_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryHostIPv6
	);
	// Регистрируем сценарий скорости повторной установки хоста
	static const bool gHostSet = awh::benchmark::add(
		"net/uri/host-set", "установок/с", HOST_SET_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedHostSet
	);
	// Регистрируем сценарий выделений памяти на повторную установку хоста
	static const bool gMemoryHostSet = awh::benchmark::add(
		"net/uri/host-set/allocations", "выделений", HOST_SET_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryHostSet
	);
	// Регистрируем сценарий скорости сравнения идентификаторов
	static const bool gCompare = awh::benchmark::add(
		"net/uri/compare", "сравнений/с", COMPARE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedCompare
	);
	// Регистрируем сценарий выделений памяти на сравнение идентификаторов
	static const bool gMemoryCompare = awh::benchmark::add(
		"net/uri/compare/allocations", "выделений", COMPARE_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryCompare
	);
};
