/**
 * @file: timers.cpp
 * @date: 2026-07-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Сценарий измерения таймеров цикла событий — постановка множества одноразовых таймеров
 *        с разнесёнными дедлайнами и их срабатывание
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <chrono>

/**
 * Подключаем заголовочный файл бенчмарков сетевого движка
 */
#include "io.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён бенчмарков сетевого движка
 */
using namespace awh::benchmark::io;

/**
 * @brief Внутренние параметры и сценарии бенчмарков таймеров
 *
 */
namespace {
	/**
	 * @brief Порог скорости обслуживания таймеров простой структурой хранения
	 *
	 * @details Порог откалиброван по отладочной сборке репозитория с запасом в
	 *          два с половиной раза: она даёт около 54 тысяч таймеров в секунду
	 *          против 190 тысяч у оптимизированной. Разрыв между сборками здесь
	 *          перестал быть десятикратным после перевода пользовательских
	 *          таймеров на внутреннюю структуру дедлайнов: постановка больше не
	 *          проходит через список изменений ядра, а значит отладочная сборка
	 *          больше не печатает трассировку на каждый таймер
	 *
	 */
	static constexpr double SIMPLE_THRESHOLD = 20000.0;
	/**
	 * @brief Порог скорости обслуживания таймеров сложной структурой хранения
	 *
	 * @details Задан равным порогу простой структуры: обе укладываются в один
	 *          порядок, и какая из них быстрее, зависит от нагрузки. На постановке
	 *          десятков тысяч таймеров разом простая идёт быстрее, зато выполняет
	 *          вдвое больше выделений памяти на таймер
	 *
	 */
	static constexpr double DIFFICULT_THRESHOLD = 20000.0;
	/**
	 * @brief Порог количества выделений памяти на один таймер
	 *
	 * @details Два выделения приходятся на сам узел события: структура таймера и
	 *          запись реестра событий. Ещё два добавляет простая структура
	 *          дедлайнов - запись упорядоченного множества и запись поиска по
	 *          ключу, - и от режима сборки это не зависит. Сложная структура
	 *          дедлайнов обходится без них вовсе: она хранит дедлайны в
	 *          непрерывной куче и таблице слотов, поэтому на ней показатель
	 *          остаётся равным двум. Порог задан по простой структуре как по
	 *          худшей из двух
	 *
	 */
	static constexpr double ALLOCATIONS_THRESHOLD = 4.5;
	/**
	 * @brief Порог количества изменений подписки на один таймер
	 *
	 * @details Сам таймер изменений подписки больше не порождает: он живёт во
	 *          внутренней структуре дедлайнов, а ядру достаётся один таймер на весь
	 *          цикл событий. Оставшиеся две записи принадлежат не таймеру, а общему
	 *          для всех типов событий механизму освобождения узла: он ставит
	 *          пользовательское событие и дёргает его, чтобы удалить узел вне
	 *          функции обратного вызова. Порог сторожит именно это - появление
	 *          третьей записи означает, что таймер снова что-то регистрирует в ядре
	 *
	 */
	static constexpr double CHANGES_THRESHOLD = 2.5;
	/**
	 * @brief Порог размера наибольшего пакета изменений подписки на один таймер
	 *
	 * @details Показатель нормирован на количество таймеров нарочно: смысл его не в
	 *          величине, а в том, что пакет изменений не должен расти вместе с
	 *          количеством событий. Список изменений сбрасывается ядру только при
	 *          опросе, поиск по нему линеен, и пакет в сто тысяч записей означает
	 *          квадратичное поведение постановки. Суммарное количество изменений
	 *          этого не поймает: сброс списка частями оставит его тем же, а
	 *          квадратичность уберёт. Целевое значение - величина, не зависящая от
	 *          количества таймеров, то есть ноль после нормировки
	 *
	 */
	static constexpr double BATCH_THRESHOLD = 0.05;

	/**
	 * @brief Функция прогона сценария обслуживания таймеров
	 *
	 * @details Замер охватывает полный жизненный цикл таймера: создание события,
	 *          постановку с дедлайном, срабатывание и освобождение. Дедлайны
	 *          разнесены по диапазону, поэтому структура хранения таймеров
	 *          работает на всю глубину, а не вырождается в единственный ключ.
	 *          Ожидание дедлайнов входит в замер и одинаково для всех
	 *          сравниваемых реализаций
	 *
	 * @param mode тип внутренних таймеров движка
	 * @return     итоги прогона сценария
	 *
	 */
	static outcome_t schedule(const awh::event::timer_t mode) noexcept {
		// Итоги прогона сценария
		outcome_t result;
		// Количество сработавших таймеров
		size_t fired = 0;
		// Создаём объект асинхронного движка ввода-вывода
		awh::engine::io_t io(framework(), logger());
		// Устанавливаем тип внутренних таймеров движка
		io.setInternalTimer(mode);
		// Если инициализация движка не выполнена
		if(!io.initialize())
			// Выводим пустые итоги прогона
			return result;
		// Включаем учёт выделений памяти
		awh::benchmark::counting(true);
		// Включаем учёт системных вызовов
		awh::benchmark::syscall::counting(true);
		// Запоминаем момент начала замера
		const auto start = std::chrono::steady_clock::now();
		/**
		 * Выполняем постановку требуемого количества одноразовых таймеров
		 */
		for(size_t i = 0; i < TIMER_COUNT; i++){
			// Добавляем новое событие таймера
			const awh::event::id_t id = io.event(awh::event::node_t::TIMEOUT, awh::event::family_t::TIMER);
			// Устанавливаем дедлайн таймера с разбросом по диапазону
			io.setTimeout(id, awh::event::action_t::NONE, (1 + static_cast <uint32_t> (i % TIMER_SPREAD)));
			// Устанавливаем функцию обратного вызова на срабатывание таймера
			io.on(id, static_cast <awh::engine::callback::status_t> ([&fired](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				// Если таймер сработал
				if(status == awh::event::status_t::SUCCESS)
					// Считаем сработавший таймер
					fired++;
			}));
			// Выполняем фиксацию настроек события таймера
			io.commit(id);
			// Запускаем событие таймера
			io.launch(id);
		}
		/**
		 * Запускаем опрос событий до срабатывания всех таймеров
		 */
		while((fired < TIMER_COUNT) && io.poll());
		// Запоминаем момент окончания замера
		const auto finish = std::chrono::steady_clock::now();
		// Закрываем окно замера: счётчики обязаны остановиться там же, где часы
		awh::benchmark::counting(false);
		awh::benchmark::syscall::counting(false);
		// Устанавливаем количество выполненных операций
		result.operations = fired;
		// Устанавливаем затраченное время
		result.seconds = std::chrono::duration <double> (finish - start).count();
		// Снимаем показатели окружения по итогам замера
		collect(result);
		// Выполняем деинициализацию движка
		io.deinitialize();
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона на простой структуре хранения
	 *
	 * @note Итоги снимаются с единственного прогона: показатели скорости и
	 *       выделений памяти описывают одну и ту же постановку таймеров
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & measured() noexcept {
		// Итоги прогона сценария обслуживания таймеров
		static const outcome_t result = ::schedule(awh::event::timer_t::SIMPLE);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария таймеров на простой структуре хранения
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t simple() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем итоги прогона сценария обслуживания таймеров
		const outcome_t & outcome = ::measured();
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Убеждаемся, что сценарий выполнил хоть одну операцию
		validate(result, outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария таймеров на сложной структуре хранения
	 *
	 * @details Сложная структура хранения включается методом `setInternalTimer`
	 *          и отличается от простой устройством очереди дедлайнов. Сценарий
	 *          снимает обе: показатель прямо отвечает на вопрос, какую из них
	 *          выбирать под нагрузку из десятков тысяч одновременных таймеров
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t difficult() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем прогон сценария обслуживания таймеров
		const outcome_t outcome = ::schedule(awh::event::timer_t::DIFFICULT);
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Убеждаемся, что сценарий выполнил хоть одну операцию
		validate(result, outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария учёта выделений памяти на один таймер
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t allocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем итоги прогона сценария обслуживания таймеров
		const outcome_t & outcome = ::measured();
		// Устанавливаем измеренное значение
		result.value = perOperation(outcome);
		// Убеждаемся, что сценарий выполнил хоть одну операцию
		validate(result, outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	// Регистрируем сценарий таймеров на простой структуре хранения
	static const bool gSimple = awh::benchmark::add(
		"net/io/timers/simple", "таймеров/с", SIMPLE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::simple
	);
	// Регистрируем сценарий таймеров на сложной структуре хранения
	static const bool gDifficult = awh::benchmark::add(
		"net/io/timers/difficult", "таймеров/с", DIFFICULT_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::difficult
	);
	/**
	 * @brief Функция прогона сценария учёта изменений подписки на один таймер
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t changes() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Если учёт системных вызовов недоступен
		if(!awh::benchmark::syscall::available()){
			// Отмечаем измерение как не выполнявшееся
			result.skipped = true;
			// Устанавливаем причину, по которой измерение не выполнялось
			result.reason = awh::benchmark::syscall::reason();
			// Выводим результат измерения
			return result;
		}
		// Получаем итоги прогона сценария
		const outcome_t & outcome = ::measured();
		// Устанавливаем измеренное значение
		result.value = perChange(outcome);
		// Убеждаемся, что сценарий выполнил хоть одну операцию
		validate(result, outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	// Регистрируем сценарий учёта выделений памяти на один таймер
	static const bool gAllocations = awh::benchmark::add(
		"net/io/timers/allocations-per-timer", "выделений", ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, &::allocations
	);
	/**
	 * @brief Функция прогона сценария размера пакета изменений подписки
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t batch() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Если учёт системных вызовов недоступен
		if(!awh::benchmark::syscall::available()){
			// Отмечаем измерение как не выполнявшееся
			result.skipped = true;
			// Устанавливаем причину, по которой измерение не выполнялось
			result.reason = awh::benchmark::syscall::reason();
			// Выводим результат измерения
			return result;
		}
		// Получаем итоги прогона сценария
		const outcome_t & outcome = ::measured();
		// Устанавливаем измеренное значение
		result.value = ((outcome.operations > 0)
		 ? (static_cast <double> (outcome.batch) / static_cast <double> (outcome.operations)) : 0.0);
		// Убеждаемся, что сценарий выполнил хоть одну операцию
		validate(result, outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	// Регистрируем сценарий учёта изменений подписки на один таймер
	static const bool gChanges = awh::benchmark::add(
		"net/io/timers/changes-per-timer", "изменений", CHANGES_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, &::changes
	);
	// Регистрируем сценарий размера пакета изменений подписки
	static const bool gBatch = awh::benchmark::add(
		"net/io/timers/peak-batch-per-timer", "записей", BATCH_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, &::batch
	);
};
