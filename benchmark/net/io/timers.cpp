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
	 * @details Порог откалиброван по отладочной сборке репозитория с двукратным
	 *          запасом. Разброс между сборками здесь наибольший среди сценариев
	 *          набора: отладочная сборка печатает трассировку изменений ядра на
	 *          каждую постановку таймера и медленнее оптимизированной на порядок
	 *
	 */
	static constexpr double SIMPLE_THRESHOLD = 1000.0;
	/**
	 * @brief Порог скорости обслуживания таймеров сложной структурой хранения
	 *
	 */
	static constexpr double DIFFICULT_THRESHOLD = 1000.0;
	/**
	 * @brief Порог количества выделений памяти на один таймер
	 *
	 * @details Постановка таймера выполняет два выделения памяти и от режима
	 *          сборки не зависит, поэтому порог задан вплотную к измеренному
	 *          значению
	 *
	 */
	static constexpr double ALLOCATIONS_THRESHOLD = 4.0;

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
		// Отключаем учёт выделений памяти
		awh::benchmark::counting(false);
		// Устанавливаем количество выполненных операций
		result.operations = fired;
		// Устанавливаем затраченное время
		result.seconds = std::chrono::duration <double> (finish - start).count();
		// Получаем статистику выделений памяти
		awh::benchmark::allocations(result.allocations, result.allocated);
		// Получаем пиковый объём занятой процессом памяти
		result.footprint = footprint();
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
	// Регистрируем сценарий учёта выделений памяти на один таймер
	static const bool gAllocations = awh::benchmark::add(
		"net/io/timers/allocations-per-timer", "выделений", ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, &::allocations
	);
};
