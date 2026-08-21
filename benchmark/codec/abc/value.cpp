/**
 * @file value.cpp
 * @date 2026-08-21
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
 * @brief Замеры владеющего значения бинарного контейнера ABC
 *
 * @details Дерево документа держит содержимое одним хранилищем и раздаёт виды в него, а
 * владеющее значение заводит память у всякого узла и годится к передаче наружу.
 * Замеряется здесь именно эта плата за владение и работа с готовым значением
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
 * @brief Внутренние параметры сценариев владеющего значения
 *
 */
namespace {
	/**
	 * @brief Количество обрабатываемых крупных записей
	 *
	 */
	static constexpr size_t LARGE_ROUNDS = 8;
	/**
	 * @brief Количество обрабатываемых записей ответа службы
	 *
	 */
	static constexpr size_t SMALL_ROUNDS = 20000;
	/**
	 * @brief Количество устанавливаемых значений при сборке вызовами
	 *
	 */
	static constexpr size_t ASSEMBLY_COUNT = 20000;

	/**
	 * @warning ПОРОГИ ЗДЕСЬ ВРЕМЕННЫЕ. Сняты они по одной рабочей машине и назначены с
	 *          запасом, взятым на глаз. Разметить их надлежит по дну КАЖДОГО СЦЕНАРИЯ,
	 *          снятому раскладкой по двенадцати стендам тремя прогонами отдельными
	 *          процессами: дно берут разные машины, и порог по рабочей машине сторожил
	 *          бы лишь её
	 *
	 */
	/**
	 * @brief Порог пропускной способности снятия значения записи ответа службы
	 *
	 * @note Порог этот отдельный от порога крупной записи, стоящего в наборе дерева:
	 *       мелкая запись снимается быстрее крупной, и общий порог на двоих сторожил бы
	 *       лишь того, кто медленнее
	 *
	 */
	static constexpr double TAKE_SERVICE_THRESHOLD = 4.0;
	/**
	 * @brief Порог пропускной способности обхода владеющего значения
	 *
	 */
	static constexpr double WALK_LARGE_THRESHOLD = 50.0;
	/**
	 * @brief Порог пропускной способности записи значения в запись
	 *
	 */
	static constexpr double COMPOSE_LARGE_THRESHOLD = 30.0;
	/**
	 * @brief Порог скорости сборки значения вызовами
	 *
	 * @details Показатель этот стережёт устройство отображения: поиск поля попарным
	 *          сличением имён обращает сборку в квадратичную, и на перечне из тысяч
	 *          полей падение видно сразу
	 *
	 * @warning Сборка эта КВАДРАТИЧНА ныне, и порог поставлен по измеренному, а не по
	 *          должному. Замерено на рабочей машине 21.08.2026: 3.09 мкс на установку
	 *          при 2500 полях, 5.34 при 5000, 10.26 при 10000, 18.36 при 20000 - плата
	 *          на установку удваивается с удвоением числа полей. Держит это `operator[]`
	 *          владеющего значения, разыскивающий поле перебором имён, и свойство это
	 *          общее у владеющих значений ВСЕХ кодеков, а не своё у ABC. Порог сторожит
	 *          лишь то, чтобы хуже не стало, и привязан к ASSEMBLY_COUNT: изменив его,
	 *          пороговое число надлежит переснять
	 *
	 */
	static constexpr double ASSEMBLY_THRESHOLD = 27000.0;
	/**
	 * @brief Порог расхода выделений памяти на снятие одного значения
	 *
	 * @details Расход этот **на порядки выше** расхода дерева, и так и должно быть:
	 *          дерево хранит узлы сплошным перечнем, а владеющее значение - россыпью,
	 *          где всякий узел заводит свою память. Порог сторожит не малость расхода,
	 *          а устройство: рост его сверх числа узлов означал бы, что память заводится
	 *          не по узлу, а по нескольку раз на узел
	 *
	 */
	static constexpr double TAKE_ALLOCATIONS_THRESHOLD = 128.0;

	/**
	 * @brief Функция обхода владеющего значения
	 *
	 * @param value обходимое значение
	 * @return      количество обойдённых значений
	 *
	 */
	static uint64_t walk(const awh::codec::abc::value_t & value) noexcept {
		// Количество обойдённых значений
		uint64_t result = 1;
		/**
		 * Если значение является перечнем либо отображением
		 */
		if(value.is(awh::codec::abc::type_t::ARRAY) || value.is(awh::codec::abc::type_t::MAP)){
			/**
			 * Выполняем перебор всех вложенных значений
			 */
			for(size_t i = 0; i < value.size(); i++)
				// Выполняем учёт обойдённых вложенных значений
				result += ::walk(value[i]);
		}
		// Выводим количество обойдённых значений
		return result;
	}
	/**
	 * @brief Функция прогона сценария снятия значения записи ответа службы
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t takeService() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемая запись
		const vector <uint8_t> & record = service();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(record.size(), SMALL_ROUNDS, [&record]() noexcept {
			// Владеющее значение записи
			awh::codec::abc::value_t value;
			/**
			 * Если разобрать запись во владеющее значение не удалось
			 */
			if(!value.parse(record.data(), record.size()))
				// Выводим нулевое количество значений
				return static_cast <uint64_t> (0);
			// Выводим количество значений владеющего значения
			return static_cast <uint64_t> (value.size() + 1);
		});
		// Если сценарий работы не выполнил
		if(!worked(outcome, result))
			// Выводим результат измерения
			return result;
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария обхода владеющего значения
	 *
	 * @details Значение снимается до замера, чтобы снятие и обход не скрывали друг друга
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t walkLarge() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемая запись
		const vector <uint8_t> & record = large();
		// Владеющее значение записи
		awh::codec::abc::value_t value;
		/**
		 * Если разобрать запись во владеющее значение не удалось
		 */
		if(!value.parse(record.data(), record.size())){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "разбор записи во владеющее значение отвечен отказом";
			// Выводим результат измерения
			return result;
		}
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(record.size(), LARGE_ROUNDS, [&value]() noexcept {
			// Выполняем обход владеющего значения
			return ::walk(value);
		});
		// Если сценарий работы не выполнил
		if(!worked(outcome, result))
			// Выводим результат измерения
			return result;
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария записи владеющего значения в запись
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t composeLarge() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемая запись
		const vector <uint8_t> & record = large();
		// Владеющее значение записи
		awh::codec::abc::value_t value;
		/**
		 * Если разобрать запись во владеющее значение не удалось
		 */
		if(!value.parse(record.data(), record.size())){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "разбор записи во владеющее значение отвечен отказом";
			// Выводим результат измерения
			return result;
		}
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(record.size(), LARGE_ROUNDS, [&value]() noexcept {
			// Сборка собираемой записи
			awh::codec::abc::writer_t writer;
			/**
			 * Если записать владеющее значение не удалось
			 */
			if(!value.compose(writer))
				// Выводим нулевой размер собранной записи
				return static_cast <uint64_t> (0);
			// Выводим размер собранной записи
			return static_cast <uint64_t> (writer.record().size());
		});
		// Если сценарий работы не выполнил
		if(!worked(outcome, result))
			// Выводим результат измерения
			return result;
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария сборки значения вызовами
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t assembly() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(0, 1, []() noexcept {
			// Собираемое владеющее значение
			awh::codec::abc::value_t value;
			// Количество выполненных установок
			uint64_t count = 0;
			/**
			 * Выполняем установку всех значений отображения
			 */
			for(size_t i = 0; i < ASSEMBLY_COUNT; i++){
				// Выполняем установку очередного значения отображения
				value["поле" + to_string(i)] = awh::codec::abc::value_t(static_cast <uint64_t> (i));
				// Выполняем учёт выполненной установки
				count++;
			}
			// Выводим количество выполненных установок
			return count;
		});
		// Если сценарий работы не выполнил
		if(!worked(outcome, result))
			// Выводим результат измерения
			return result;
		// Устанавливаем измеренную скорость выполнения установок
		result.value = ((outcome.seconds > 0.0) ? (static_cast <double> (outcome.operations * ASSEMBLY_COUNT) / outcome.seconds) : 0.0);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария расхода выделений памяти на снятие значения
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t takeAllocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемая запись
		const vector <uint8_t> & record = service();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(record.size(), SMALL_ROUNDS, [&record]() noexcept {
			// Владеющее значение записи
			awh::codec::abc::value_t value;
			/**
			 * Если разобрать запись во владеющее значение не удалось
			 */
			if(!value.parse(record.data(), record.size()))
				// Выводим нулевое количество значений
				return static_cast <uint64_t> (0);
			// Выводим количество значений владеющего значения
			return static_cast <uint64_t> (value.size() + 1);
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
	 * Выполняем регистрацию сценария снятия значения записи ответа службы
	 */
	static const bool TAKE_SERVICE_REGISTERED = awh::benchmark::add(
		"codec/abc: снятие значения ответа службы", "МБ/с", TAKE_SERVICE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, takeService
	);
	/**
	 * Выполняем регистрацию сценария обхода владеющего значения
	 */
	static const bool WALK_REGISTERED = awh::benchmark::add(
		"codec/abc: обход владеющего значения", "МБ/с", WALK_LARGE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, walkLarge
	);
	/**
	 * Выполняем регистрацию сценария записи владеющего значения в запись
	 */
	static const bool COMPOSE_REGISTERED = awh::benchmark::add(
		"codec/abc: запись значения в запись", "МБ/с", COMPOSE_LARGE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, composeLarge
	);
	/**
	 * Выполняем регистрацию сценария сборки значения вызовами
	 */
	static const bool ASSEMBLY_REGISTERED = awh::benchmark::add(
		"codec/abc: сборка значения вызовами", "уст./с", ASSEMBLY_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, assembly
	);
	/**
	 * Выполняем регистрацию сценария расхода выделений памяти на снятие значения
	 */
	static const bool TAKE_ALLOCATIONS_REGISTERED = awh::benchmark::add(
		"codec/abc: выделения на снятие значения", "выд./зап.", TAKE_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, takeAllocations
	);
};
