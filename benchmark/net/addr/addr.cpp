/**
 * @file: addr.cpp
 * @date: 2026-07-29
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Сценарии измерения модуля работы с адресами — разбор, печать,
 *        определение разновидности адреса и принадлежности сети
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <array>
#include <chrono>
#include <string>

/**
 * Подключаем заголовочный файл бенчмарков модуля работы с адресами
 */
#include "addr.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён бенчмарков модуля работы с адресами
 */
using namespace awh::benchmark::addr;

/**
 * @brief Внутренние параметры и сценарии бенчмарков модуля работы с адресами
 *
 */
namespace {
	/**
	 * @brief Пороги количества операций в секунду
	 *
	 * @details Пороги откалиброваны по отладочной сборке репозитория с
	 *          двукратным запасом: они ловят регрессию в разы, а не колебания
	 *          планировщика операционной системы
	 *
	 */
	static constexpr double PARSE_IPV4_THRESHOLD = 1800000.0;
	static constexpr double PARSE_IPV6_THRESHOLD = 800000.0;
	static constexpr double PRINT_IPV4_THRESHOLD = 3400000.0;
	static constexpr double PRINT_IPV6_THRESHOLD = 1700000.0;
	static constexpr double HOST_THRESHOLD = 1400000.0;
	static constexpr double MAPPING_THRESHOLD = 1600000.0;
	/**
	 * @brief Пороги количества выделений памяти на одну операцию
	 *
	 * @details Показатель от машины и режима сборки не зависит, поэтому пороги
	 *          заданы вплотную к измеренным значениям. Они и есть главный
	 *          показатель набора: разбор адреса в буфер известного размера
	 *          выделять память не обязан вовсе, и всякое выделение здесь -
	 *          работа, которой можно не делать.
	 *
	 *          Первый замер показал расхождение по разновидностям адреса
	 *          вчетверо, и расхождение не в пользу IPv6: разбор IPv4 не
	 *          выделял памяти вовсе, разбор IPv6 выделял трижды на адрес,
	 *          определение разновидности - четырежды с четвертью, проверка
	 *          принадлежности сети - единожды. Источников оказалось три:
	 *          сборка IPv6-адреса заводила три динамических массива слов,
	 *          проверка адреса заводила динамический массив под буфер, ничего
	 *          наружу не отдающий, а бинарный буфер самого адреса был
	 *          динамическим при заведомо известной предельной длине. После их
	 *          устранения ни один из перечисленных сценариев памяти не
	 *          выделяет, и пороги стоят на нуле - обратно они подниматься не
	 *          должны.
	 *
	 *          Печать IPv4 не выделяет памяти по случайности, а не по
	 *          устройству: строка "192.168.1.100" короче порога размещения
	 *          строки внутри самого объекта, а строка IPv6 длиннее его, и
	 *          выделение у неё - это возвращаемое значение. Убрать его можно
	 *          лишь сменой сигнатуры, отдающей строку наружу, поэтому порог
	 *          печати IPv6 единственный оставлен ненулевым
	 *
	 */
	static constexpr double PARSE_IPV4_ALLOCATIONS = 0.0;
	static constexpr double PARSE_IPV6_ALLOCATIONS = 0.0;
	static constexpr double PRINT_IPV4_ALLOCATIONS = 0.0;
	static constexpr double PRINT_IPV6_ALLOCATIONS = 1.0;
	static constexpr double HOST_ALLOCATIONS = 0.0;
	static constexpr double MAPPING_ALLOCATIONS = 0.0;

	/**
	 * @brief Набор образцов адресов IPv4 сценариев разбора и печати
	 *
	 * @details Образцы перебираются по кругу, а не повторяется один и тот же:
	 *          повторение одного адреса измеряло бы работу распределителя
	 *          памяти на прогретом кэше, а не разбор
	 *
	 */
	static constexpr const char * SAMPLES_IPV4[] = {
		"192.168.1.100", "10.0.0.1", "172.16.254.1", "8.8.8.8",
		"255.255.255.255", "0.0.0.0", "127.0.0.1", "203.0.113.42"
	};
	/**
	 * @brief Набор образцов адресов IPv6 сценариев разбора и печати
	 *
	 */
	static constexpr const char * SAMPLES_IPV6[] = {
		"2001:db8:85a3::8a2e:370:7334", "::1", "fe80::1%lo0",
		"2001:0db8:0000:0000:0000:ff00:0042:8329", "::ffff:192.168.1.1",
		"fd00::1", "2606:4700:4700::1111", "::"
	};
	/**
	 * @brief Набор образцов сценария определения разновидности адреса
	 *
	 * @details Набор намеренно разнороден: разновидность определяется перебором
	 *          проверок, и замер на одной разновидности измерял бы длину пути
	 *          до неё, а не работу метода
	 *
	 */
	static constexpr const char * SAMPLES_HOST[] = {
		"192.168.1.100", "2001:db8::1", "02:42:9b:9e:f9:ae",
		"example.com", "192.168.0.0/24", "/var/run/socket.sock",
		"https://anyks.com/path", "8.8.8.8"
	};
	/**
	 * @brief Количество образцов в каждом наборе
	 *
	 */
	static constexpr size_t SAMPLES_COUNT = 8;

	/**
	 * @brief Функция прогона сценария разбора адреса
	 *
	 * @param samples набор образцов адресов
	 * @return        итоги прогона сценария
	 *
	 */
	static outcome_t parsing(const char * const * samples) noexcept {
		// Итоги прогона сценария
		outcome_t result;
		// Объект работы с адресами
		awh::net_addr_t addr(framework(), logger());
		/**
		 * Выполняем прогрев: первые обращения выводят модуль на установившийся режим
		 */
		for(size_t i = 0; i < ADDR_WARMUP; i++)
			// Выполняем разбор очередного образца адреса
			addr.parse(samples[i % SAMPLES_COUNT]);
		// Включаем учёт выделений памяти
		awh::benchmark::counting(true);
		// Включаем учёт системных вызовов
		awh::benchmark::syscall::counting(true);
		// Запоминаем момент начала замера
		const auto start = std::chrono::steady_clock::now();
		/**
		 * Выполняем требуемое количество разборов адреса
		 */
		for(size_t i = 0; i < ADDR_ROUNDS; i++)
			// Выполняем разбор очередного образца адреса
			addr.parse(samples[i % SAMPLES_COUNT]);
		// Запоминаем момент окончания замера
		const auto finish = std::chrono::steady_clock::now();
		// Закрываем окно замера: счётчики обязаны остановиться там же, где часы
		awh::benchmark::counting(false);
		awh::benchmark::syscall::counting(false);
		// Устанавливаем количество выполненных операций
		result.operations = ADDR_ROUNDS;
		// Устанавливаем затраченное время
		result.seconds = std::chrono::duration <double> (finish - start).count();
		// Снимаем показатели окружения по итогам замера
		collect(result);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария печати адреса
	 *
	 * @param sample образец адреса
	 * @return       итоги прогона сценария
	 *
	 */
	static outcome_t printing(const char * sample) noexcept {
		// Итоги прогона сценария
		outcome_t result;
		// Объект работы с адресами
		awh::net_addr_t addr(framework(), logger());
		// Выполняем разбор образца адреса
		addr.parse(sample);
		/**
		 * Выполняем прогрев: первые обращения выводят модуль на установившийся режим
		 */
		for(size_t i = 0; i < ADDR_WARMUP; i++)
			// Выполняем печать адреса
			addr.print();
		// Включаем учёт выделений памяти
		awh::benchmark::counting(true);
		// Включаем учёт системных вызовов
		awh::benchmark::syscall::counting(true);
		// Запоминаем момент начала замера
		const auto start = std::chrono::steady_clock::now();
		/**
		 * Выполняем требуемое количество печатей адреса
		 */
		for(size_t i = 0; i < ADDR_ROUNDS; i++)
			// Выполняем печать адреса
			addr.print();
		// Запоминаем момент окончания замера
		const auto finish = std::chrono::steady_clock::now();
		// Закрываем окно замера: счётчики обязаны остановиться там же, где часы
		awh::benchmark::counting(false);
		awh::benchmark::syscall::counting(false);
		// Устанавливаем количество выполненных операций
		result.operations = ADDR_ROUNDS;
		// Устанавливаем затраченное время
		result.seconds = std::chrono::duration <double> (finish - start).count();
		// Снимаем показатели окружения по итогам замера
		collect(result);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария определения разновидности адреса
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t hosting() noexcept {
		// Итоги прогона сценария
		outcome_t result;
		// Объект работы с адресами
		awh::net_addr_t addr(framework(), logger());
		/**
		 * Выполняем прогрев: первые обращения выводят модуль на установившийся режим
		 */
		for(size_t i = 0; i < ADDR_WARMUP; i++)
			// Выполняем определение разновидности очередного образца
			addr.host(SAMPLES_HOST[i % SAMPLES_COUNT]);
		// Включаем учёт выделений памяти
		awh::benchmark::counting(true);
		// Включаем учёт системных вызовов
		awh::benchmark::syscall::counting(true);
		// Запоминаем момент начала замера
		const auto start = std::chrono::steady_clock::now();
		/**
		 * Выполняем требуемое количество определений разновидности адреса
		 */
		for(size_t i = 0; i < ADDR_ROUNDS; i++)
			// Выполняем определение разновидности очередного образца
			addr.host(SAMPLES_HOST[i % SAMPLES_COUNT]);
		// Запоминаем момент окончания замера
		const auto finish = std::chrono::steady_clock::now();
		// Закрываем окно замера: счётчики обязаны остановиться там же, где часы
		awh::benchmark::counting(false);
		awh::benchmark::syscall::counting(false);
		// Устанавливаем количество выполненных операций
		result.operations = ADDR_ROUNDS;
		// Устанавливаем затраченное время
		result.seconds = std::chrono::duration <double> (finish - start).count();
		// Снимаем показатели окружения по итогам замера
		collect(result);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария принадлежности адреса сети
	 *
	 * @details Проверка принадлежности сети выполняется движком на каждом
	 *          принятом подключении, когда задан список разрешённых сетей,
	 *          поэтому её стоимость входит в стоимость приёма подключения
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t membership() noexcept {
		// Итоги прогона сценария
		outcome_t result;
		// Объект работы с адресами
		awh::net_addr_t addr(framework(), logger());
		// Выполняем разбор проверяемого адреса
		addr.parse("192.168.1.100");
		/**
		 * Выполняем прогрев: первые обращения выводят модуль на установившийся режим
		 */
		for(size_t i = 0; i < ADDR_WARMUP; i++)
			// Выполняем проверку принадлежности адреса сети
			addr.mapping("192.168.0.0", static_cast <uint8_t> (16), awh::net_addr_t::addr_t::HOST);
		// Включаем учёт выделений памяти
		awh::benchmark::counting(true);
		// Включаем учёт системных вызовов
		awh::benchmark::syscall::counting(true);
		// Запоминаем момент начала замера
		const auto start = std::chrono::steady_clock::now();
		/**
		 * Выполняем требуемое количество проверок принадлежности сети
		 */
		for(size_t i = 0; i < NETWORK_ROUNDS; i++)
			// Выполняем проверку принадлежности адреса сети
			addr.mapping("192.168.0.0", static_cast <uint8_t> (16), awh::net_addr_t::addr_t::HOST);
		// Запоминаем момент окончания замера
		const auto finish = std::chrono::steady_clock::now();
		// Закрываем окно замера: счётчики обязаны остановиться там же, где часы
		awh::benchmark::counting(false);
		awh::benchmark::syscall::counting(false);
		// Устанавливаем количество выполненных операций
		result.operations = NETWORK_ROUNDS;
		// Устанавливаем затраченное время
		result.seconds = std::chrono::duration <double> (finish - start).count();
		// Снимаем показатели окружения по итогам замера
		collect(result);
		// Выводим итоги прогона сценария
		return result;
	}

	/**
	 * @brief Функция получения итогов прогона сценария разбора адреса IPv4
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & parsedIPv4() noexcept {
		// Итоги прогона сценария разбора адреса IPv4
		static const outcome_t result = ::parsing(SAMPLES_IPV4);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона сценария разбора адреса IPv6
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & parsedIPv6() noexcept {
		// Итоги прогона сценария разбора адреса IPv6
		static const outcome_t result = ::parsing(SAMPLES_IPV6);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона сценария печати адреса IPv4
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & printedIPv4() noexcept {
		// Итоги прогона сценария печати адреса IPv4
		static const outcome_t result = ::printing("192.168.1.100");
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона сценария печати адреса IPv6
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & printedIPv6() noexcept {
		// Итоги прогона сценария печати адреса IPv6
		static const outcome_t result = ::printing("2001:db8:85a3::8a2e:370:7334");
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона сценария определения разновидности адреса
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & hosted() noexcept {
		// Итоги прогона сценария определения разновидности адреса
		static const outcome_t result = ::hosting();
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона сценария принадлежности адреса сети
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & mapped() noexcept {
		// Итоги прогона сценария принадлежности адреса сети
		static const outcome_t result = ::membership();
		// Выводим итоги прогона сценария
		return result;
	}

	/**
	 * @brief Макрос объявления пары сценариев скорости и выделений памяти
	 *
	 * @details Каждая измеряемая операция даёт два показателя: сколько операций
	 *          в секунду и сколько выделений памяти на операцию. Объявляются они
	 *          одинаково для всех шести операций, и выписывать шесть пар функций
	 *          вручную значило бы шесть раз повторить один и тот же текст
	 *
	 * @param SUFFIX  окончание имён объявляемых функций
	 * @param OUTCOME функция получения итогов прогона сценария
	 *
	 */
	#define AWH_ADDR_SCENARIO(SUFFIX, OUTCOME)                                  \
		static awh::benchmark::result_t speed ## SUFFIX() noexcept {            \
			awh::benchmark::result_t result;                                    \
			const outcome_t & outcome = OUTCOME();                              \
			result.value = perSecond(outcome);                                  \
			result.details = details(outcome);                                  \
			return result;                                                      \
		}                                                                       \
		static awh::benchmark::result_t memory ## SUFFIX() noexcept {           \
			awh::benchmark::result_t result;                                    \
			const outcome_t & outcome = OUTCOME();                              \
			result.value = perOperation(outcome);                               \
			result.details = details(outcome);                                  \
			return result;                                                      \
		}

	// Объявляем сценарии разбора адреса IPv4
	AWH_ADDR_SCENARIO(ParseIPv4, ::parsedIPv4)
	// Объявляем сценарии разбора адреса IPv6
	AWH_ADDR_SCENARIO(ParseIPv6, ::parsedIPv6)
	// Объявляем сценарии печати адреса IPv4
	AWH_ADDR_SCENARIO(PrintIPv4, ::printedIPv4)
	// Объявляем сценарии печати адреса IPv6
	AWH_ADDR_SCENARIO(PrintIPv6, ::printedIPv6)
	// Объявляем сценарии определения разновидности адреса
	AWH_ADDR_SCENARIO(Host, ::hosted)
	// Объявляем сценарии принадлежности адреса сети
	AWH_ADDR_SCENARIO(Mapping, ::mapped)

	#undef AWH_ADDR_SCENARIO

	// Регистрируем сценарий скорости разбора адреса IPv4
	static const bool gParseIPv4 = awh::benchmark::add(
		"net/addr/parse-ipv4", "разборов/с", PARSE_IPV4_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedParseIPv4
	);
	// Регистрируем сценарий выделений памяти на разбор адреса IPv4
	static const bool gMemoryParseIPv4 = awh::benchmark::add(
		"net/addr/parse-ipv4/allocations", "выделений", PARSE_IPV4_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryParseIPv4
	);
	// Регистрируем сценарий скорости разбора адреса IPv6
	static const bool gParseIPv6 = awh::benchmark::add(
		"net/addr/parse-ipv6", "разборов/с", PARSE_IPV6_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedParseIPv6
	);
	// Регистрируем сценарий выделений памяти на разбор адреса IPv6
	static const bool gMemoryParseIPv6 = awh::benchmark::add(
		"net/addr/parse-ipv6/allocations", "выделений", PARSE_IPV6_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryParseIPv6
	);
	// Регистрируем сценарий скорости печати адреса IPv4
	static const bool gPrintIPv4 = awh::benchmark::add(
		"net/addr/print-ipv4", "печатей/с", PRINT_IPV4_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedPrintIPv4
	);
	// Регистрируем сценарий выделений памяти на печать адреса IPv4
	static const bool gMemoryPrintIPv4 = awh::benchmark::add(
		"net/addr/print-ipv4/allocations", "выделений", PRINT_IPV4_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryPrintIPv4
	);
	// Регистрируем сценарий скорости печати адреса IPv6
	static const bool gPrintIPv6 = awh::benchmark::add(
		"net/addr/print-ipv6", "печатей/с", PRINT_IPV6_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedPrintIPv6
	);
	// Регистрируем сценарий выделений памяти на печать адреса IPv6
	static const bool gMemoryPrintIPv6 = awh::benchmark::add(
		"net/addr/print-ipv6/allocations", "выделений", PRINT_IPV6_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryPrintIPv6
	);
	// Регистрируем сценарий скорости определения разновидности адреса
	static const bool gHost = awh::benchmark::add(
		"net/addr/host", "определений/с", HOST_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedHost
	);
	// Регистрируем сценарий выделений памяти на определение разновидности адреса
	static const bool gMemoryHost = awh::benchmark::add(
		"net/addr/host/allocations", "выделений", HOST_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryHost
	);
	// Регистрируем сценарий скорости проверки принадлежности адреса сети
	static const bool gMapping = awh::benchmark::add(
		"net/addr/mapping", "проверок/с", MAPPING_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedMapping
	);
	// Регистрируем сценарий выделений памяти на проверку принадлежности адреса сети
	static const bool gMemoryMapping = awh::benchmark::add(
		"net/addr/mapping/allocations", "выделений", MAPPING_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryMapping
	);
};
