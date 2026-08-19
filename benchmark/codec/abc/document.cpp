/**
 * @file document.cpp
 * @date 2026-08-19
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
 * @brief Замеры дерева документа и владеющего значения бинарного контейнера ABC
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл
 */
#include "abc.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh::benchmark::binary;

/**
 * @brief Внутренние параметры сценариев дерева документа
 *
 */
namespace {
	/**
	 * @brief Количество собираемых деревьев крупной записи
	 *
	 */
	static constexpr size_t LARGE_ROUNDS = 8;
	/**
	 * @brief Количество собираемых деревьев записи ответа службы
	 *
	 */
	static constexpr size_t SMALL_ROUNDS = 20000;

	/**
	 * @brief Порог пропускной способности сборки дерева крупной записи
	 *
	 */
	static constexpr double TREE_LARGE_THRESHOLD = 7.0;
	/**
	 * @brief Порог пропускной способности обхода собранного дерева
	 *
	 * @details Дерево собирается до замера, чтобы сборка и обход не скрывали друг друга
	 *
	 */
	static constexpr double TREE_WALK_THRESHOLD = 60.0;
	/**
	 * @brief Порог пропускной способности перезаписи дерева в запись
	 *
	 */
	static constexpr double TREE_REWRITE_THRESHOLD = 6.0;
	/**
	 * @brief Порог количества выделений памяти на сборку дерева
	 *
	 * @details Дерево хранится сплошным перечнем узлов, содержимое - одним хранилищем:
	 *          расход обязан оставаться считанными единицами на запись, а рост его
	 *          означал бы выделение памяти на всякий узел
	 *
	 */
	static constexpr double TREE_ALLOCATIONS_THRESHOLD = 80.0;
	/**
	 * @brief Порог пропускной способности снятия владеющего значения
	 *
	 */
	static constexpr double VALUE_TAKE_THRESHOLD = 1.5;
	/**
	 * @brief Порог задержки сборки дерева записи ответа службы в микросекундах
	 *
	 */
	static constexpr double TREE_SERVICE_LATENCY_THRESHOLD = 20.0;

	/**
	 * @brief Функция обхода собранного дерева документа
	 *
	 * @param value обходимое значение документа
	 * @return      количество обойдённых узлов
	 *
	 */
	static uint64_t walk(const awh::codec::abc::document_t::value_t & value) noexcept {
		// Количество обойдённых узлов
		uint64_t result = 1;
		/**
		 * Если значение является вместимым
		 */
		if(value.is(awh::codec::abc::type_t::CONTAINER)){
			/**
			 * Выполняем обход всех значений вместимого
			 */
			for(auto item = value.begin(); item.valid(); item = item.next())
				// Выполняем учёт обойдённых узлов вложенного значения
				result += ::walk(item);
		}
		// Выводим количество обойдённых узлов
		return result;
	}
	/**
	 * @brief Функция сборки дерева документа
	 *
	 * @param record разбираемая запись
	 * @return       количество узлов собранного дерева
	 *
	 */
	static uint64_t build(const vector <uint8_t> & record) noexcept {
		// Дерево документа
		awh::codec::abc::document_t document;
		// Если разобрать запись в дерево документа не удалось
		if(!document.parse(record.data(), record.size()))
			// Выводим нулевое количество узлов дерева
			return 0;
		// Выводим количество узлов собранного дерева
		return static_cast <uint64_t> (document.nodes());
	}
	/**
	 * @brief Функция прогона сценария сборки дерева заданной записи
	 *
	 * @param record разбираемая запись
	 * @param rounds количество собираемых деревьев
	 * @return       результат измерения
	 *
	 */
	static awh::benchmark::result_t building(const vector <uint8_t> & record, const size_t rounds) noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(record.size(), rounds, [&record]() noexcept {
			// Выполняем сборку дерева документа
			return ::build(record);
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария сборки дерева крупной записи
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t treeLarge() noexcept {
		// Выполняем прогон сценария сборки дерева крупной записи
		return ::building(large(), LARGE_ROUNDS);
	}
	/**
	 * @brief Функция прогона сценария обхода собранного дерева
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t treeWalk() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемая запись
		const vector <uint8_t> & record = large();
		// Дерево документа
		awh::codec::abc::document_t document;
		/**
		 * Если разобрать запись в дерево документа не удалось
		 */
		if(!document.parse(record.data(), record.size())){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "разбор записи в дерево документа отвечен отказом";
			// Выводим результат измерения
			return result;
		}
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(record.size(), LARGE_ROUNDS, [&document]() noexcept {
			// Выполняем обход собранного дерева документа
			return ::walk(document.root());
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария перезаписи дерева в запись
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t treeRewrite() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемая запись
		const vector <uint8_t> & record = large();
		// Дерево документа
		awh::codec::abc::document_t document;
		/**
		 * Если разобрать запись в дерево документа не удалось
		 */
		if(!document.parse(record.data(), record.size())){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "разбор записи в дерево документа отвечен отказом";
			// Выводим результат измерения
			return result;
		}
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(record.size(), LARGE_ROUNDS, [&document]() noexcept {
			// Сборка перезаписываемой записи
			awh::codec::abc::writer_t writer;
			// Если перезаписать дерево документа не удалось
			if(!document.build(writer))
				// Выводим нулевой размер перезаписанной записи
				return static_cast <uint64_t> (0);
			// Выводим размер перезаписанной записи
			return static_cast <uint64_t> (writer.record().size());
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария расхода выделений памяти на сборку дерева
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t treeAllocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемая запись
		const vector <uint8_t> & record = large();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(record.size(), LARGE_ROUNDS, [&record]() noexcept {
			// Выполняем сборку дерева документа
			return ::build(record);
		});
		// Если сценарий работы не выполнил
		if(!worked(outcome, result))
			// Выводим результат измерения
			return result;
		// Если учёт выделений памяти не работает
		if(!counted(outcome, result))
			// Выводим результат измерения
			return result;
		// Устанавливаем измеренное количество выделений памяти на одну запись
		result.value = perDocument(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария снятия владеющего значения
	 *
	 * @details Мост от разбора к владению: дерево обходится одним выделением на запись,
	 * а владеющее значение держит память у всякого узла
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t valueTake() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемая запись
		const vector <uint8_t> & record = large();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(record.size(), LARGE_ROUNDS, [&record]() noexcept {
			// Владеющее значение документа
			awh::codec::abc::value_t value;
			// Если разобрать запись во владеющее значение не удалось
			if(!value.parse(record.data(), record.size()))
				// Выводим нулевое количество значений
				return static_cast <uint64_t> (0);
			// Выводим количество значений владеющего значения
			return static_cast <uint64_t> (value.size() + 1);
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария задержки сборки дерева ответа службы
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t latencyService() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемая запись
		const vector <uint8_t> & record = service();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(record.size(), SMALL_ROUNDS, [&record]() noexcept {
			// Выполняем сборку дерева документа
			return ::build(record);
		});
		// Если сценарий работы не выполнил
		if(!worked(outcome, result))
			// Выводим результат измерения
			return result;
		// Устанавливаем измеренную задержку сборки дерева
		result.value = perLatency(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	/**
	 * Выполняем регистрацию сценария сборки дерева крупной записи
	 */
	static const bool LARGE_REGISTERED = awh::benchmark::add(
		"codec/abc: сборка дерева крупной записи", "МБ/с", TREE_LARGE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, treeLarge
	);
	/**
	 * Выполняем регистрацию сценария обхода собранного дерева
	 */
	static const bool WALK_REGISTERED = awh::benchmark::add(
		"codec/abc: обход собранного дерева", "МБ/с", TREE_WALK_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, treeWalk
	);
	/**
	 * Выполняем регистрацию сценария перезаписи дерева в запись
	 */
	static const bool REWRITE_REGISTERED = awh::benchmark::add(
		"codec/abc: перезапись дерева", "МБ/с", TREE_REWRITE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, treeRewrite
	);
	/**
	 * Выполняем регистрацию сценария расхода выделений памяти на сборку дерева
	 */
	static const bool ALLOCATIONS_REGISTERED = awh::benchmark::add(
		"codec/abc: выделения на сборку дерева", "выд./зап.", TREE_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, treeAllocations
	);
	/**
	 * Выполняем регистрацию сценария снятия владеющего значения
	 */
	static const bool VALUE_REGISTERED = awh::benchmark::add(
		"codec/abc: снятие владеющего значения", "МБ/с", VALUE_TAKE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, valueTake
	);
	/**
	 * Выполняем регистрацию сценария задержки сборки дерева ответа службы
	 */
	static const bool SERVICE_LATENCY_REGISTERED = awh::benchmark::add(
		"codec/abc: задержка сборки дерева ответа службы", "мкс/зап.", TREE_SERVICE_LATENCY_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, latencyService
	);
};
