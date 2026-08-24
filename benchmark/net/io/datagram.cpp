/**
 * @file datagram.cpp
 * @date 2026-08-24
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
 * @brief Сценарии измерения датаграммного обмена — стоимость одного оборота цикла событий
 *        на одном сокете UDP и на множестве одновременных отправителей
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <chrono>
#include <memory>
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
 * @brief Внутренние параметры и сценарии бенчмарков датаграммного обмена
 *
 */
namespace {
	/**
	 * @brief Порог скорости датаграммного обмена на одном сокете в обменах в секунду
	 *
	 * @details Порог откалиброван с тем же запасом, что у потокового обмена, чьи
	 *          параметры нагрузки этот сценарий повторяет дословно
	 *
	 * @note Ожидание, что датаграммный обмен окажется ДЕШЕВЛЕ потокового - сборки
	 *       границ сообщения на принимающей стороне ведь нет, - замером НЕ
	 *       подтвердилось: 58 500 обменов в секунду против 67 800 у потокового в
	 *       одном прогоне, то есть датаграммы медленнее на 14 %. Обращений к ядру
	 *       при этом поровну (по шесть) и выделений памяти нет ни там, ни там,
	 *       значит разница лежит в стоимости самих обращений, а не в их числе
	 *
	 */
	static constexpr double SINGLE_THRESHOLD = 25000.0;
	/**
	 * @brief Порог скорости датаграммного обмена на множестве отправителей в обменах в секунду
	 *
	 * @warning Порог этот НЕ равен пороговому значению потокового обмена, хотя
	 *          параметры нагрузки совпадают. Сначала он был скопирован оттуда, и
	 *          это оказалось ошибкой: у потокового обмена запас до порога
	 *          трёхкратный, а у датаграммного вышел бы полуторным - такой порог
	 *          срывался бы на колебаниях планировщика, а не на регрессии
	 *
	 * @note Датаграммный обмен на множестве отправителей идёт примерно вдвое
	 *       медленнее потокового (замер: 122 000 против 236 000 обменов в секунду
	 *       в одном прогоне), и это устройство сценария, а не изъян движка: все
	 *       отправители сходятся на ОДНОМ сокете сервера, тогда как у потокового
	 *       обмена на каждого приходится свой принятый сокет. Оттого сценарий
	 *       меряет разбор сессий на одном сокете, а не выборку готовых из многих, -
	 *       чего прочие сценарии набора не меряют вовсе
	 *
	 */
	static constexpr double MULTI_THRESHOLD = 40000.0;
	/**
	 * @brief Порог количества выделений памяти на один датаграммный обмен
	 *
	 * @details Как и у потокового обмена, показатель от машины и режима сборки не
	 *          зависит, поэтому порог задан вплотную к измеренному: установившийся
	 *          обмен выделений памяти не выполняет вовсе
	 *
	 */
	static constexpr double ALLOCATIONS_THRESHOLD = 0.5;
	/**
	 * @brief Порог количества системных вызовов на один датаграммный обмен
	 *
	 * @details Замер даёт РОВНО ШЕСТЬ обращений - столько же, сколько у потокового
	 *          обмена: по приёму и передаче на каждой стороне плюс возврат за
	 *          готовностью. Порог задан вплотную, как и там
	 *
	 * @warning Прежде здесь стоял просторный порог 12.5 - по догадке, что
	 *          датаграммам обращений нужно БОЛЬШЕ. Догадка неверна, и замер её
	 *          отверг: обращений поровну. Разница же движков лежит в другую
	 *          сторону - у колец `io_uring` родной приём убирает `recvfrom` вовсе
	 *          (замер: 92 обращения на десять датаграмм против 51), то есть счёт
	 *          там МЕНЬШЕ шести и в порог с границей MAXIMUM укладывается тем более.
	 *          Просторный порог ловил бы лишь то, чего не бывает, пропуская
	 *          настоящую регрессию - появление седьмого обращения
	 *
	 */
	static constexpr double SYSCALLS_THRESHOLD = 6.5;
	/**
	 * @brief Предельный срок прогона сценария в миллисекундах
	 *
	 * @warning Срок этот - НЕ мера ожидания, а страховка от зависания. Датаграмма
	 *          доставки не гарантирует, и потерянная остановила бы обмен навсегда:
	 *          сторона ждала бы ответа, которого не будет. Прогон, упёршийся в
	 *          срок, объявляется НЕ выполнившимся, а не медленным, - итоги его
	 *          пусты, и `validate()` отмечает это отказом
	 *
	 */
	static constexpr uint32_t DEADLINE = 120000;

	/**
	 * @brief Структура состояния одного отправителя сценария
	 *
	 * @note Состояние захватывается функцией обратного вызова конкретного
	 *       отправителя по указателю, поэтому поиск состояния по идентификатору
	 *       события в горячем пути не выполняется
	 *
	 */
	typedef struct Sender {
		// Идентификатор события отправителя
		awh::event::id_t id;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Sender() noexcept : id(0) {}
	} sender_t;

	/**
	 * @brief Структура состояния прогона сценария
	 *
	 */
	typedef struct State {
		// Признак остановки цикла событий
		bool stop;
		// Признак того, что замер начат
		bool measuring;
		// Признак того, что прогон упёрся в предельный срок
		bool expired;
		// Количество выполненных обменов прогрева
		size_t warmed;
		// Количество выполненных обменов замера
		size_t done;
		// Требуемое количество обменов прогрева
		size_t warmup;
		// Требуемое количество обменов замера
		size_t rounds;
		// Момент начала замера
		std::chrono::steady_clock::time_point start;
		// Момент окончания замера
		std::chrono::steady_clock::time_point finish;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit State() noexcept :
		 stop(false), measuring(false), expired(false),
		 warmed(0), done(0), warmup(0), rounds(0) {}
	} state_t;

	/**
	 * @brief Функция учёта выполненного обмена
	 *
	 * @note Устроена дословно как у потокового обмена: окно замера открывается
	 *       после прогрева и закрывается вместе с часами, чтобы счётчики выделений
	 *       памяти и обращений к ядру описывали ровно тот же участок, что и время
	 *
	 * @param state состояние прогона сценария
	 * @return      результат учёта (true - обмен следует продолжать)
	 *
	 */
	static bool account(state_t & state) noexcept {
		// Если замер ещё не начат
		if(!state.measuring){
			// Считаем выполненный обмен прогрева
			state.warmed++;
			// Если прогрев ещё не завершён
			if(state.warmed < state.warmup)
				// Продолжаем обмен
				return true;
			// Включаем режим замера
			state.measuring = true;
			// Включаем учёт выделений памяти
			awh::benchmark::counting(true);
			// Включаем учёт системных вызовов
			awh::benchmark::syscall::counting(true);
			// Запоминаем момент начала замера
			state.start = std::chrono::steady_clock::now();
			// Продолжаем обмен
			return true;
		}
		// Считаем выполненный обмен замера
		state.done++;
		// Если требуемое количество обменов ещё не выполнено
		if(state.done < state.rounds)
			// Продолжаем обмен
			return true;
		// Запоминаем момент окончания замера
		state.finish = std::chrono::steady_clock::now();
		// Закрываем окно замера: счётчики обязаны остановиться там же, где часы
		awh::benchmark::counting(false);
		awh::benchmark::syscall::counting(false);
		// Останавливаем цикл событий
		state.stop = true;
		// Прекращаем обмен
		return false;
	}
	/**
	 * @brief Функция прогона сценария датаграммного обмена
	 *
	 * @details Обе стороны обмена обслуживаются одним циклом событий в одном
	 *          потоке: отправитель шлёт датаграмму, сервер возвращает её обратно,
	 *          и приём ответа отправителем считается одним обменом. Каждый
	 *          отправитель держит ровно одну датаграмму в полёте, поэтому
	 *          показатель отражает стоимость оборота цикла, а не глубину
	 *          конвейеризации
	 *
	 * @note Параметры нагрузки взяты у потокового обмена БЕЗ изменений - тот же
	 *       размер полезной нагрузки и то же количество обменов. Сделано это
	 *       намеренно: при совпадении всего прочего разница показателей выражает
	 *       ровно цену переноса, а собственные величины сделали бы сценарии
	 *       несравнимыми
	 *
	 * @note Сборки границ сообщения на принимающей стороне здесь нет, и это не
	 *       упрощение, а свойство переноса: датаграмма приходит целиком либо не
	 *       приходит вовсе, тогда как поток вправе доставить ответ по частям
	 *
	 * @param senders количество одновременных отправителей
	 * @param rounds  требуемое количество обменов замера
	 * @return        итоги прогона сценария
	 *
	 */
	static outcome_t exchange(const size_t senders, const size_t rounds) noexcept {
		// Итоги прогона сценария
		outcome_t result;
		// Состояние прогона сценария
		state_t state;
		// Устанавливаем требуемое количество обменов замера
		state.rounds = rounds;
		/**
		 * Устанавливаем требуемое количество обменов прогрева: буферы сокетов и
		 * накопители движка выходят на рабочий объём за первые обмены
		 */
		state.warmup = (senders * 8);
		// Полезная нагрузка одного обмена
		static uint8_t payload[ECHO_PAYLOAD] = {0};
		// Создаём объект асинхронного движка ввода-вывода
		awh::engine::io_t io(framework(), logger());
		// Получаем свободный порт петлевого интерфейса
		const uint16_t number = port();
		// Добавляем новое событие сервера
		const awh::event::id_t server = io.event(
			awh::event::node_t::SERVER, awh::event::family_t::IPV4,
			awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP
		);
		// Устанавливаем порт события сервера
		io.setSourcePort(server, number);
		// Добавляем событие предельного срока прогона
		const awh::event::id_t deadline = io.event(awh::event::node_t::TIMEOUT, awh::event::family_t::TIMER);
		// Если инициализация движка не выполнена
		if(!io.initialize())
			// Выводим пустые итоги прогона
			return result;
		// Устанавливаем опции события сервера
		io.setOptions(server, options());
		// Устанавливаем адрес события сервера
		io.setAddress(server, awh::event::address_t::IPV4, "127.0.0.1");
		// Список принятых сессий отправителей
		vector <unique_ptr <sender_t>> accepted;
		// Резервируем место под сессии отправителей
		accepted.reserve(senders);
		/**
		 * Устанавливаем функцию обратного вызова на создание сессии отправителя
		 *
		 * @note Слушающего сокета у датаграммного сервера нет: сессия заводится
		 *       первой пришедшей датаграммой, и сторона сервера узнаёт об
		 *       отправителе только отсюда
		 */
		io.on(server, static_cast <awh::engine::callback::accept_t> ([&io, &accepted](
			[[maybe_unused]] const awh::event::id_t sid, const awh::event::id_t cid
		) noexcept -> void {
			// Устанавливаем опции события сессии отправителя
			io.setOptions(cid, options());
			// Создаём состояние сессии отправителя
			accepted.push_back(unique_ptr <sender_t> (new sender_t));
			// Запоминаем идентификатор события сессии отправителя
			accepted.back()->id = cid;
			/**
			 * Устанавливаем функцию обратного вызова на чтение из сессии отправителя:
			 * сервер возвращает принятые октеты как есть
			 */
			io.on(cid, [&io](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Отправляем принятые октеты обратно отправителю
				io.send(eid, data, size);
			});
		}));
		// Выполняем фиксацию настроек события сервера
		io.commit(server);
		// Выполняем запуск события сервера
		io.launch(server);
		// Список отправителей
		vector <unique_ptr <sender_t>> clients;
		// Резервируем место под отправителей
		clients.reserve(senders);
		/**
		 * Выполняем создание требуемого количества отправителей
		 */
		for(size_t i = 0; i < senders; i++){
			// Создаём состояние отправителя
			clients.push_back(unique_ptr <sender_t> (new sender_t));
			// Получаем состояние отправителя
			sender_t * sender = clients.back().get();
			// Добавляем новое событие отправителя
			sender->id = io.event(
				awh::event::node_t::CLIENT, awh::event::family_t::IPV4,
				awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP
			);
			// Устанавливаем порт назначения события отправителя
			io.setTargetPort(sender->id, number);
			// Устанавливаем опции события отправителя
			io.setOptions(sender->id, options());
			// Устанавливаем локальный адрес события отправителя
			io.setAddress(sender->id, awh::event::address_t::IPV4, "0.0.0.0");
			// Устанавливаем адрес назначения события отправителя
			io.setTarget(sender->id, "127.0.0.1");
			/**
			 * Устанавливаем функцию обратного вызова на чтение из события отправителя
			 *
			 * @note Накопления принятых октетов здесь нет намеренно: датаграмма
			 *       приходит целиком, и всякий вызов отклика есть ровно один обмен
			 */
			io.on(sender->id, [&io, &state]([[maybe_unused]] const awh::event::id_t eid, [[maybe_unused]] const uint8_t * data, [[maybe_unused]] const size_t size) noexcept -> void {
				// Если обмен следует продолжать
				if(::account(state))
					// Отправляем следующую датаграмму
					io.send(eid, payload, ECHO_PAYLOAD);
			});
			// Выполняем фиксацию настроек события отправителя
			io.commit(sender->id);
			// Выполняем запуск события отправителя
			io.launch(sender->id);
			// Отправляем первую датаграмму, начиная обмен
			io.send(sender->id, payload, ECHO_PAYLOAD);
		}
		// Устанавливаем предельный срок прогона
		io.setTimeout(deadline, awh::event::action_t::NONE, DEADLINE);
		/**
		 * Устанавливаем функцию обратного вызова на событие предельного срока
		 */
		io.on(deadline, [&state]([[maybe_unused]] const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			// Если предельный срок прогона истёк
			if(status == awh::event::status_t::SUCCESS){
				// Отмечаем прогон упёршимся в предельный срок
				state.expired = true;
				// Останавливаем цикл событий
				state.stop = true;
			}
		});
		// Выполняем фиксацию настроек события предельного срока
		io.commit(deadline);
		// Выполняем запуск события предельного срока
		io.launch(deadline);
		/**
		 * Запускаем опрос событий до выполнения требуемого количества обменов
		 */
		while(!state.stop && io.poll());
		// Если прогон упёрся в предельный срок
		if(state.expired){
			// Закрываем окно замера, оставшееся открытым
			awh::benchmark::counting(false);
			awh::benchmark::syscall::counting(false);
			// Уничтожаем все события движка
			io.deinitialize();
			// Выводим пустые итоги прогона: обмен не состоялся
			return result;
		}
		// Устанавливаем количество выполненных обменов
		result.operations = state.done;
		// Устанавливаем объём переданных данных
		result.bytes = (state.done * ECHO_PAYLOAD * 2);
		// Устанавливаем длительность замера
		result.seconds = std::chrono::duration <double> (state.finish - state.start).count();
		// Собираем показатели учёта
		collect(result);
		// Уничтожаем все события движка
		io.deinitialize();
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона датаграммного обмена на одном сокете
	 *
	 * @note Итоги снимаются с единственного прогона: показатели скорости и
	 *       выделений памяти описывают один и тот же обмен
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & measured() noexcept {
		// Итоги прогона сценария датаграммного обмена на одном сокете
		static const outcome_t result = ::exchange(1, ECHO_SINGLE_ROUNDS);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария датаграммного обмена на одном сокете
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t single() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем итоги прогона сценария обмена
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
	 * @brief Функция прогона сценария датаграммного обмена на множестве отправителей
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t multi() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем прогон сценария обмена
		const outcome_t outcome = ::exchange(ECHO_MULTI_CONNECTIONS, ECHO_MULTI_ROUNDS);
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
	 * @brief Функция прогона сценария учёта выделений памяти на один обмен
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t allocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем итоги прогона сценария обмена
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
	/**
	 * @brief Функция прогона сценария учёта системных вызовов на один обмен
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t syscalls() noexcept {
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
		result.value = perSyscall(outcome);
		// Убеждаемся, что сценарий выполнил хоть одну операцию
		validate(result, outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	// Регистрируем сценарий датаграммного обмена на одном сокете
	static const bool gSingle = awh::benchmark::add(
		"net/io/datagram/single", "обменов/с", SINGLE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::single
	);
	// Регистрируем сценарий датаграммного обмена на множестве отправителей
	static const bool gMulti = awh::benchmark::add(
		"net/io/datagram/multi", "обменов/с", MULTI_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::multi
	);
	// Регистрируем сценарий учёта выделений памяти на один обмен
	static const bool gAllocations = awh::benchmark::add(
		"net/io/datagram/allocations-per-round", "выделений", ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, &::allocations
	);
	// Регистрируем сценарий учёта системных вызовов на один обмен
	static const bool gSyscalls = awh::benchmark::add(
		"net/io/datagram/syscalls-per-round", "вызовов", SYSCALLS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, &::syscalls
	);
};
