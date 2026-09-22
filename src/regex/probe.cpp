/**
 * @file probe.cpp
 * @date 2026-08-29
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
 * @brief Исходный файл учёта путей исполнения сопоставления
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <regex/probe.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <atomic>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

namespace {
	/**
	 * @brief Счётчики путей исполнения сопоставления
	 *
	 * @details Счётчики заводятся всегда, а приращаются лишь у библиотеки, признаком
	 *          сборки «AWH_REGEX_PROBING» собранной: место под семь величин ничего
	 *          не стоит, а условная сборка самого хранилища развела бы устройство
	 *          библиотеки и её заголовка.
	 *
	 */
	atomic <uint64_t> COUNTERS[static_cast <size_t> (awh::regex::path_t::COUNT)];

	/**
	 * @brief Набор счётчиков мер работы сопоставления
	 *
	 * @details Хранилище разведено с хранилищем путей намеренно: перечисления
	 *          их несут значения, с нуля начинающиеся, и один набор смешал бы
	 *          шаги цикла с порождением машинного кода
	 *
	 */
	atomic <uint64_t> AMOUNTS[static_cast <size_t> (awh::regex::work_t::COUNT)];
};
/**
 * @brief Метод проверки заведения учёта путей исполнения
 *
 * @return признак заведения учёта путей исполнения
 *
 */
bool awh::regex::Probe::enabled() noexcept {
	/**
	 * Если учёт путей исполнения сопоставления заведён
	 */
	#if defined(AWH_REGEX_PROBING)
		// Выводим признак заведения учёта путей исполнения
		return true;
	/**
	 * Если учёт путей исполнения сопоставления не заведён
	 */
	#else
		// Выводим отсутствие учёта путей исполнения
		return false;
	#endif
}
/**
 * @brief Метод сброса счётчиков путей исполнения
 *
 */
void awh::regex::Probe::reset() noexcept {
	/**
	 * Выполняем обход счётчиков путей исполнения сопоставления
	 */
	for(size_t i = 0; i < static_cast <size_t> (path_t::COUNT); i++)
		// Выполняем сброс очередного счётчика пути исполнения
		COUNTERS[i].store(0, memory_order_relaxed);
	/**
	 * Выполняем обход всех счётчиков мер работы сопоставления
	 */
	for(size_t i = 0; i < static_cast <size_t> (work_t::COUNT); i++)
		// Выполняем сброс очередного счётчика меры работы
		AMOUNTS[i].store(0, memory_order_relaxed);
}
/**
 * @brief Метод извлечения счётчика пути исполнения
 *
 * @param path учитываемый путь исполнения сопоставления
 * @return     количество прохождений пути исполнения
 *
 */
uint64_t awh::regex::Probe::count(const path_t path) noexcept {
	/**
	 * Если учитываемый путь исполнения за пределы набора выходит
	 */
	if(static_cast <size_t> (path) >= static_cast <size_t> (path_t::COUNT))
		// Выводим отсутствие прохождений пути исполнения
		return 0;
	// Выводим количество прохождений пути исполнения
	return COUNTERS[static_cast <size_t> (path)].load(memory_order_relaxed);
}
/**
 * @brief Метод учёта прохождения пути исполнения
 *
 * @param path пройденный путь исполнения сопоставления
 *
 */
void awh::regex::Probe::tick(const path_t path) noexcept {
	/**
	 * Если пройденный путь исполнения за пределы набора выходит
	 */
	if(static_cast <size_t> (path) >= static_cast <size_t> (path_t::COUNT))
		// Выходим из метода учёта прохождения пути исполнения
		return;
	// Выполняем учёт прохождения пути исполнения сопоставления
	COUNTERS[static_cast <size_t> (path)].fetch_add(1, memory_order_relaxed);
}
/**
 * @brief Метод извлечения счётчика меры работы
 *
 * @param work учитываемая мера работы сопоставления
 * @return     количество операций учитываемой меры
 *
 */
uint64_t awh::regex::Probe::amount(const work_t work) noexcept {
	/**
	 * Если мера работы за пределами набора счётчиков
	 */
	if(static_cast <size_t> (work) >= static_cast <size_t> (work_t::COUNT))
		// Выводим отсутствие операций учитываемой меры
		return 0;
	// Выводим количество операций учитываемой меры работы
	return AMOUNTS[static_cast <size_t> (work)].load(memory_order_relaxed);
}
/**
 * @brief Метод учёта выполненных операций меры работы
 *
 * @param work  выполненная мера работы сопоставления
 * @param count количество выполненных операций
 *
 */
void awh::regex::Probe::spend(const work_t work, const uint64_t count) noexcept {
	/**
	 * Если мера работы за пределами набора счётчиков
	 */
	if(static_cast <size_t> (work) >= static_cast <size_t> (work_t::COUNT))
		// Выходим из метода учёта выполненных операций
		return;
	// Выполняем учёт выполненных операций меры работы сопоставления
	AMOUNTS[static_cast <size_t> (work)].fetch_add(count, memory_order_relaxed);
}
