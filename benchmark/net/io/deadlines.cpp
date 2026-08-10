/**
 * @file: deadlines.cpp
 * @date: 2026-07-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Сценарии измерения структуры дедлайнов — постановка, отмена и перевзведение таймера
 *        в отрыве от создания и освобождения узла события
 *
 * @details Сценарий полного жизненного цикла таймера (`timers`) измеряет всё
 *          сразу: создание узла события, постановку, срабатывание и
 *          освобождение. Для сравнения с чужими библиотеками этого мало по двум
 *          причинам. Во-первых, у них наблюдатель таймера живёт в памяти
 *          пользователя и не создаётся вовсе, поэтому сравнивать полный цикл с
 *          постановкой - значит сравнивать разный объём работы. Во-вторых, из
 *          одного числа не видно, какая из двух структур дедлайнов движка чего
 *          стоит и на какой операции.
 *
 *          Здесь узлы событий создаются заранее, вне окна замера, и измеряется
 *          ровно одна операция структуры дедлайнов. Дедлайны отнесены далеко в
 *          будущее, поэтому за время замера не срабатывает ни один таймер.
 *
 *          Отдельно снимается отношение стоимости операции при десятикратном
 *          росте количества таймеров: оно показывает сложность структуры прямо,
 *          не требуя знания её устройства
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <chrono>
#include <vector>

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
 * @brief Внутренние параметры и сценарии бенчмарков структуры дедлайнов
 *
 */
namespace {
	/**
	 * @brief Порог скорости постановки таймеров в структуру дедлайнов
	 *
	 * @details Пороги откалиброваны по отладочной сборке репозитория с запасом:
	 *          операции структуры дедлайнов от режима сборки зависят слабо, потому
	 *          что состоят из работы с памятью, а не из обращений к ядру
	 *
	 */
	static constexpr double ARM_THRESHOLD = 300000.0;
	/**
	 * @brief Порог скорости отмены таймеров в структуре дедлайнов
	 *
	 */
	static constexpr double CANCEL_THRESHOLD = 100000.0;
	/**
	 * @brief Порог скорости перевзведения таймеров в структуре дедлайнов
	 *
	 */
	static constexpr double REARM_THRESHOLD = 100000.0;
	/**
	 * @brief Порог отношения стоимости операции при десятикратном росте количества таймеров
	 *
	 * @details Показатель прямо выражает сложность структуры: у логарифмической
	 *          отношение близко к единице, у линейной - к десяти. Порог в двойке
	 *          отделяет первое от второго с запасом и не зависит ни от машины, ни
	 *          от режима сборки, потому что является отношением двух замеров,
	 *          снятых подряд в одинаковых условиях
	 *
	 */
	static constexpr double SCALING_THRESHOLD = 2.0;
	/**
	 * @brief Количество проходов замера одной операции
	 *
	 * @details Один проход по набору таймеров занимает доли миллисекунды, а на
	 *          таком окне замер меряет шум наравне с работой: у самых быстрых
	 *          структур показатель гулял вдвое между прогонами. Проходов делается
	 *          несколько, и в зачёт идёт самый быстрый из них - обычная защита
	 *          короткого замера от постороннего вмешательства операционной
	 *          системы, потому что помешать проходу она может, а помочь - нет.
	 *
	 *          Подготовка набора к очередному проходу выполняется вне окна замера
	 *
	 */
	static constexpr size_t DEADLINE_PASSES = 50;

	/**
	 * @brief Структура набора подготовленных таймеров
	 *
	 */
	typedef struct Fleet {
		// Объект асинхронного движка ввода-вывода
		awh::engine::io_t io;
		// Список идентификаторов созданных событий таймеров
		vector <awh::event::id_t> ids;
		/**
		 * @brief Конструктор
		 *
		 * @param mode  тип внутренних таймеров движка
		 * @param count количество создаваемых таймеров
		 *
		 */
		explicit Fleet(const awh::event::timer_t mode, const size_t count) noexcept :
		 io(framework(), logger()) {
			// Устанавливаем тип внутренних таймеров движка
			this->io.setInternalTimer(mode);
			// Если инициализация движка не выполнена
			if(!this->io.initialize())
				// Прекращаем подготовку набора
				return;
			// Резервируем память под список идентификаторов событий
			this->ids.reserve(count);
			/**
			 * Выполняем создание требуемого количества таймеров
			 */
			for(size_t i = 0; i < count; i++){
				// Добавляем новое событие таймера
				const awh::event::id_t id = this->io.event(awh::event::node_t::TIMEOUT, awh::event::family_t::TIMER);
				// Устанавливаем дедлайн таймера с разбросом по диапазону
				this->io.setTimeout(id, awh::event::action_t::NONE, deadline(i));
				// Выполняем фиксацию настроек события таймера
				this->io.commit(id);
				// Запоминаем идентификатор созданного события
				this->ids.push_back(id);
			}
		}
		/**
		 * @brief Деструктор
		 *
		 */
		~Fleet() noexcept {
			// Выполняем деинициализацию движка
			this->io.deinitialize();
		}
		/**
		 * @brief Функция вычисления дедлайна таймера
		 *
		 * @param index порядковый номер таймера
		 * @return      дедлайн таймера в миллисекундах
		 *
		 */
		static uint32_t deadline(const size_t index) noexcept {
			// Выводим дедлайн таймера, отнесённый далеко в будущее
			return (DEADLINE_OFFSET + static_cast <uint32_t> (index % DEADLINE_SPREAD));
		}
		/**
		 * @brief Функция постановки всех таймеров набора
		 *
		 */
		void arm() noexcept {
			/**
			 * Выполняем постановку всех таймеров набора
			 */
			for(size_t i = 0; i < this->ids.size(); i++)
				// Ставим таймер в структуру дедлайнов
				this->io.launch(this->ids[i]);
		}
		/**
		 * @brief Функция снятия всех таймеров набора
		 *
		 * @details Снятие выражается нулевой задержкой, поэтому вместе с таймером
		 *          обнуляется и заданная ему задержка
		 *
		 */
		void disarm() noexcept {
			/**
			 * Выполняем снятие всех таймеров набора
			 */
			for(size_t i = 0; i < this->ids.size(); i++)
				// Снимаем таймер со структуры дедлайнов
				this->io.setTimeout(this->ids[i], awh::event::action_t::NONE, 0);
		}
		/**
		 * @brief Функция восстановления задержек всех таймеров набора
		 *
		 * @details Возвращает набор к состоянию, в котором он вышел из
		 *          конструктора: таймеры сняты, а задержки заданы. Нужна между
		 *          проходами замера, потому что снятие задержки обнуляет
		 *
		 */
		void restore() noexcept {
			/**
			 * Выполняем восстановление задержек всех таймеров набора
			 */
			for(size_t i = 0; i < this->ids.size(); i++)
				// Возвращаем таймеру заданную ему задержку
				this->io.setTimeout(this->ids[i], awh::event::action_t::NONE, deadline(i));
		}
	} fleet_t;

	/**
	 * @brief Функция прогона сценария постановки таймеров
	 *
	 * @details Узлы событий созданы и зафиксированы заранее, поэтому в окно замера
	 *          попадает только вставка дедлайна в структуру
	 *
	 * @param mode  тип внутренних таймеров движка
	 * @param count количество таймеров
	 * @return      итоги прогона сценария
	 *
	 */
	static outcome_t arming(const awh::event::timer_t mode, const size_t count) noexcept {
		// Итоги прогона сценария
		outcome_t result;
		// Подготавливаем набор таймеров
		fleet_t fleet(mode, count);
		// Если набор таймеров не подготовлен
		if(fleet.ids.empty())
			// Выводим пустые итоги прогона
			return result;
		// Время самого быстрого прохода замера
		double best = 0.0;
		/**
		 * Выполняем требуемое количество проходов замера
		 */
		for(size_t pass = 0; pass < DEADLINE_PASSES; pass++){
			// Возвращаем набор к исходному состоянию вне окна замера
			if(pass > 0){
				// Снимаем все таймеры набора, поставленные прошлым проходом
				fleet.disarm();
				// Возвращаем таймерам заданные им задержки
				fleet.restore();
			}
			// Включаем учёт выделений памяти
			awh::benchmark::counting(true);
			// Включаем учёт системных вызовов
			awh::benchmark::syscall::counting(true);
			// Запоминаем момент начала замера
			const auto start = std::chrono::steady_clock::now();
			// Выполняем постановку всех таймеров набора
			fleet.arm();
			// Запоминаем момент окончания замера
			const auto finish = std::chrono::steady_clock::now();
			// Закрываем окно замера: счётчики обязаны остановиться там же, где часы
			awh::benchmark::counting(false);
			awh::benchmark::syscall::counting(false);
			// Получаем время текущего прохода замера
			const double seconds = std::chrono::duration <double> (finish - start).count();
			// Запоминаем время прохода, если он оказался самым быстрым
			if((best <= 0.0) || (seconds < best))
				// Запоминаем время самого быстрого прохода замера
				best = seconds;
		}
		// Устанавливаем количество выполненных операций
		result.operations = fleet.ids.size();
		// Устанавливаем затраченное время
		result.seconds = best;
		// Снимаем показатели окружения по итогам замера
		collect(result);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария отмены таймеров
	 *
	 * @details Отмена выражается нулевой задержкой: движок снимает таймер со
	 *          структуры дедлайнов и возвращает событие в состояние
	 *          инициализировано
	 *
	 * @param mode  тип внутренних таймеров движка
	 * @param count количество таймеров
	 * @return      итоги прогона сценария
	 *
	 */
	static outcome_t cancelling(const awh::event::timer_t mode, const size_t count) noexcept {
		// Итоги прогона сценария
		outcome_t result;
		// Подготавливаем набор таймеров
		fleet_t fleet(mode, count);
		// Если набор таймеров не подготовлен
		if(fleet.ids.empty())
			// Выводим пустые итоги прогона
			return result;
		// Время самого быстрого прохода замера
		double best = 0.0;
		/**
		 * Выполняем требуемое количество проходов замера
		 */
		for(size_t pass = 0; pass < DEADLINE_PASSES; pass++){
			// Возвращаем таймерам задержки, обнулённые снятием прошлого прохода
			if(pass > 0)
				// Возвращаем таймерам заданные им задержки
				fleet.restore();
			// Выполняем постановку всех таймеров набора вне окна замера
			fleet.arm();
			// Включаем учёт выделений памяти
			awh::benchmark::counting(true);
			// Включаем учёт системных вызовов
			awh::benchmark::syscall::counting(true);
			// Запоминаем момент начала замера
			const auto start = std::chrono::steady_clock::now();
			/**
			 * Выполняем отмену всех таймеров набора
			 */
			for(size_t i = 0; i < fleet.ids.size(); i++)
				// Снимаем таймер со структуры дедлайнов
				fleet.io.setTimeout(fleet.ids[i], awh::event::action_t::NONE, 0);
			// Запоминаем момент окончания замера
			const auto finish = std::chrono::steady_clock::now();
			// Закрываем окно замера: счётчики обязаны остановиться там же, где часы
			awh::benchmark::counting(false);
			awh::benchmark::syscall::counting(false);
			// Получаем время текущего прохода замера
			const double seconds = std::chrono::duration <double> (finish - start).count();
			// Запоминаем время прохода, если он оказался самым быстрым
			if((best <= 0.0) || (seconds < best))
				// Запоминаем время самого быстрого прохода замера
				best = seconds;
		}
		// Устанавливаем количество выполненных операций
		result.operations = fleet.ids.size();
		// Устанавливаем затраченное время
		result.seconds = best;
		// Снимаем показатели окружения по итогам замера
		collect(result);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария перевзведения таймеров
	 *
	 * @details Перевзведение - самая частая операция с таймером в сетевом сервере:
	 *          таймаут чтения сдвигается вперёд на каждом принятом пакете. В
	 *          структуре дедлайнов оно выражается снятием прежней записи и
	 *          вставкой новой, поэтому обходится дороже одной только постановки
	 *
	 * @param mode  тип внутренних таймеров движка
	 * @param count количество таймеров
	 * @return      итоги прогона сценария
	 *
	 */
	static outcome_t rearming(const awh::event::timer_t mode, const size_t count) noexcept {
		// Итоги прогона сценария
		outcome_t result;
		// Подготавливаем набор таймеров
		fleet_t fleet(mode, count);
		// Если набор таймеров не подготовлен
		if(fleet.ids.empty())
			// Выводим пустые итоги прогона
			return result;
		// Выполняем постановку всех таймеров набора
		fleet.arm();
		// Время самого быстрого прохода замера
		double best = 0.0;
		/**
		 * Выполняем требуемое количество проходов замера
		 *
		 * @note Подготовки между проходами перевзведение не требует: каждый проход
		 *       отодвигает дедлайны на один и тот же шаг, оставляя набор
		 *       поставленным и пригодным для следующего прохода
		 */
		for(size_t pass = 0; pass < DEADLINE_PASSES; pass++){
			// Величина, на которую текущий проход отодвигает дедлайны
			const uint32_t shift = (static_cast <uint32_t> (pass + 1) * DEADLINE_SPREAD);
			// Включаем учёт выделений памяти
			awh::benchmark::counting(true);
			// Включаем учёт системных вызовов
			awh::benchmark::syscall::counting(true);
			// Запоминаем момент начала замера
			const auto start = std::chrono::steady_clock::now();
			/**
			 * Выполняем перевзведение всех таймеров набора
			 */
			for(size_t i = 0; i < fleet.ids.size(); i++)
				// Сдвигаем дедлайн таймера вперёд
				fleet.io.setTimeout(fleet.ids[i], awh::event::action_t::NONE, (fleet_t::deadline(i) + shift));
			// Запоминаем момент окончания замера
			const auto finish = std::chrono::steady_clock::now();
			// Закрываем окно замера: счётчики обязаны остановиться там же, где часы
			awh::benchmark::counting(false);
			awh::benchmark::syscall::counting(false);
			// Получаем время текущего прохода замера
			const double seconds = std::chrono::duration <double> (finish - start).count();
			// Запоминаем время прохода, если он оказался самым быстрым
			if((best <= 0.0) || (seconds < best))
				// Запоминаем время самого быстрого прохода замера
				best = seconds;
		}
		// Устанавливаем количество выполненных операций
		result.operations = fleet.ids.size();
		// Устанавливаем затраченное время
		result.seconds = best;
		// Снимаем показатели окружения по итогам замера
		collect(result);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция вычисления стоимости одной операции в микросекундах
	 *
	 * @param output итоги прогона сценария
	 * @return       стоимость одной операции
	 *
	 */
	static double cost(const outcome_t & output) noexcept {
		// Если операции не выполнялись
		if(output.operations == 0)
			// Выводим нулевую стоимость
			return 0.0;
		// Выводим стоимость одной операции в микросекундах
		return ((output.seconds * 1e6) / static_cast <double> (output.operations));
	}
	/**
	 * @brief Функция получения итогов прогона постановки на простой структуре
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & armedSimple() noexcept {
		// Итоги прогона сценария постановки
		static const outcome_t result = ::arming(awh::event::timer_t::SIMPLE, DEADLINE_COUNT);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона постановки на сложной структуре
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & armedDifficult() noexcept {
		// Итоги прогона сценария постановки
		static const outcome_t result = ::arming(awh::event::timer_t::DIFFICULT, DEADLINE_COUNT);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция формирования результата измерения скорости
	 *
	 * @param output итоги прогона сценария
	 * @return       результат измерения
	 *
	 */
	static awh::benchmark::result_t speed(const outcome_t & output) noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Устанавливаем измеренное значение
		result.value = perSecond(output);
		// Устанавливаем сведения о прогоне
		// Убеждаемся, что сценарий выполнил хоть одну операцию
		validate(result, output);
		result.details = details(output);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция формирования результата оценки сложности структуры
	 *
	 * @details Отношение стоимости одной операции на полном прогоне к стоимости на
	 *          уменьшенном в десять раз. Оба замера снимаются подряд в одинаковых
	 *          условиях, поэтому отношение не зависит ни от машины, ни от режима
	 *          сборки - оно выражает свойство самой структуры
	 *
	 * @param mode тип внутренних таймеров движка
	 * @return     результат измерения
	 *
	 */
	static awh::benchmark::result_t scaling(const awh::event::timer_t mode) noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем прогон на уменьшенном количестве таймеров
		const outcome_t small = ::arming(mode, DEADLINE_SMALL_COUNT);
		// Выполняем прогон на полном количестве таймеров
		const outcome_t large = ::arming(mode, DEADLINE_COUNT);
		// Получаем стоимость одной операции на уменьшенном прогоне
		const double base = ::cost(small);
		// Если стоимость на уменьшенном прогоне не измерена
		if(base <= 0.0)
			// Выводим результат измерения
			return result;
		// Устанавливаем отношение стоимостей одной операции
		result.value = (::cost(large) / base);
		// Буфер формирования сведений о прогоне
		char buffer[192];
		// Выполняем формирование сведений о прогоне
		::snprintf(
			buffer, sizeof(buffer),
			"на %zu таймерах: %.3f мкс, на %zu таймерах: %.3f мкс, отношение: %.2f",
			small.operations, base, large.operations, ::cost(large), result.value
		);
		// Устанавливаем сведения о прогоне
		result.details = string(buffer);
		// Выводим результат измерения
		return result;
	}

	// Регистрируем сценарий постановки таймеров на простой структуре
	static const bool gArmSimple = awh::benchmark::add(
		"net/io/deadlines/arm-simple", "постановок/с", ARM_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, []() noexcept -> awh::benchmark::result_t {
			// Выводим результат измерения скорости постановки
			return ::speed(::armedSimple());
		}
	);
	// Регистрируем сценарий постановки таймеров на сложной структуре
	static const bool gArmDifficult = awh::benchmark::add(
		"net/io/deadlines/arm-difficult", "постановок/с", ARM_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, []() noexcept -> awh::benchmark::result_t {
			// Выводим результат измерения скорости постановки
			return ::speed(::armedDifficult());
		}
	);
	// Регистрируем сценарий отмены таймеров на простой структуре
	static const bool gCancelSimple = awh::benchmark::add(
		"net/io/deadlines/cancel-simple", "отмен/с", CANCEL_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, []() noexcept -> awh::benchmark::result_t {
			// Выводим результат измерения скорости отмены
			return ::speed(::cancelling(awh::event::timer_t::SIMPLE, DEADLINE_COUNT));
		}
	);
	// Регистрируем сценарий отмены таймеров на сложной структуре
	static const bool gCancelDifficult = awh::benchmark::add(
		"net/io/deadlines/cancel-difficult", "отмен/с", CANCEL_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, []() noexcept -> awh::benchmark::result_t {
			// Выводим результат измерения скорости отмены
			return ::speed(::cancelling(awh::event::timer_t::DIFFICULT, DEADLINE_COUNT));
		}
	);
	// Регистрируем сценарий перевзведения таймеров на простой структуре
	static const bool gRearmSimple = awh::benchmark::add(
		"net/io/deadlines/rearm-simple", "перевзведений/с", REARM_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, []() noexcept -> awh::benchmark::result_t {
			// Выводим результат измерения скорости перевзведения
			return ::speed(::rearming(awh::event::timer_t::SIMPLE, DEADLINE_COUNT));
		}
	);
	// Регистрируем сценарий перевзведения таймеров на сложной структуре
	static const bool gRearmDifficult = awh::benchmark::add(
		"net/io/deadlines/rearm-difficult", "перевзведений/с", REARM_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, []() noexcept -> awh::benchmark::result_t {
			// Выводим результат измерения скорости перевзведения
			return ::speed(::rearming(awh::event::timer_t::DIFFICULT, DEADLINE_COUNT));
		}
	);
	// Регистрируем сценарий оценки сложности простой структуры
	static const bool gScalingSimple = awh::benchmark::add(
		"net/io/deadlines/scaling-simple", "отношение", SCALING_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, []() noexcept -> awh::benchmark::result_t {
			// Выводим результат оценки сложности структуры
			return ::scaling(awh::event::timer_t::SIMPLE);
		}
	);
	// Регистрируем сценарий оценки сложности сложной структуры
	static const bool gScalingDifficult = awh::benchmark::add(
		"net/io/deadlines/scaling-difficult", "отношение", SCALING_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, []() noexcept -> awh::benchmark::result_t {
			// Выводим результат оценки сложности структуры
			return ::scaling(awh::event::timer_t::DIFFICULT);
		}
	);
};
