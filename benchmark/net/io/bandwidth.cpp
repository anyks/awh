/**
 * @file: bandwidth.cpp
 * @date: 2026-07-28
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Сценарий измерения ограничения пропускной способности —
 *        точность удержания заданного предела на отправке и на приёме,
 *        и цена самого механизма учёта, когда предел задан, но не достигается
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <chrono>
#include <vector>
#include <string>

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
 * @brief Внутренние параметры и сценарии бенчмарков ограничения пропускной способности
 *
 * @details Ограничение полосы в движке устроено как ведро токенов на каждое
 *          направление события: токены доливаются пропорционально прошедшему
 *          времени, а чтение и запись расходуют их по числу переданных октетов.
 *          Не хватило токенов - операция откладывается таймером и повторяется,
 *          когда ведро дольётся. Сценарии ниже проверяют именно это: держится ли
 *          заданный предел и во что обходится сам учёт
 *
 */
namespace {
	/**
	 * @brief Задаваемый предел пропускной способности сценариев точности
	 *
	 * @details Предел выбран заведомо ниже того, что петлевой интерфейс выдаёт
	 *          без ограничения: на петле передача идёт гигабайтами в секунду,
	 *          и предел в восемь мебибит держится с огромным запасом. Если бы
	 *          предел был близок к достижимому, сценарий измерял бы не точность
	 *          ограничителя, а способность машины его достичь
	 *
	 */
	static constexpr const char * BANDWIDTH_LIMIT = "8Mbps";
	/**
	 * @brief Задаваемый предел пропускной способности в октетах в секунду
	 *
	 * @details Величина обязана совпадать с разбором строки предела: движок
	 *          принимает биты в секунду и делит на восемь
	 *
	 */
	static constexpr double BANDWIDTH_LIMIT_BYTES = (8000000.0 / 8.0);
	/**
	 * @brief Продолжительность замера точности удержания предела в секундах
	 *
	 * @details Ведро токенов доливается непрерывно, а расходуется порциями по
	 *          размеру полезной нагрузки кадра, поэтому на коротком окне
	 *          измеряется в основном начальное наполнение ведра. Секунды хватает,
	 *          чтобы начальное наполнение стало пренебрежимым: за неё через
	 *          ограничитель проходит около мебибайта данных
	 *
	 */
	static constexpr double BANDWIDTH_SECONDS = 1.0;
	/**
	 * @brief Размер блока постановки в очередь сценариев ограничения полосы
	 *
	 * @details Блок заведомо больше полезной нагрузки кадра: очередь отправки
	 *          обязана оставаться непустой всё время замера, иначе ограничитель
	 *          мерил бы не себя, а темп подачи данных сценарием
	 *
	 */
	static constexpr size_t BANDWIDTH_CHUNK = 65536;
	/**
	 * @brief Допустимое отклонение достигнутой скорости от заданного предела
	 *
	 * @details Показатель выражается отношением достигнутой скорости к заданной,
	 *          и порог задан сверху: ограничитель обязан не превышать предел.
	 *          Запас в четверть оставлен на начальное наполнение ведра и на
	 *          разрешение таймера, которым откладывается операция
	 *
	 */
	static constexpr double ACCURACY_THRESHOLD = 1.25;
	/**
	 * @brief Порог доли достигнутой скорости от заданного предела
	 *
	 * @details Обратная сторона того же показателя: ограничитель обязан не только
	 *          не превышать предел, но и не душить передачу существенно ниже него.
	 *          Скорость ниже трёх четвертей заданной означает, что операции
	 *          откладываются дольше, чем нужно на долив ведра
	 *
	 */
	static constexpr double UTILIZATION_THRESHOLD = 0.75;
	/**
	 * @brief Порог пропускной способности с незадействованным пределом
	 *
	 * @details Заданный предел петлевой интерфейс не выдаёт ни на одной машине,
	 *          поэтому показатель измеряет не предел, а цену самого учёта: данные
	 *          идут на полной скорости, но через весь код ведра токенов.
	 *
	 *          Прежде показатель ловил здесь два дефекта разом. Поле предела было
	 *          тридцатидвухразрядным, и всё выше 34.4 Гбит/с бралось по модулю;
	 *          а размер операции обрезался до одного кадра MTU, отчего
	 *          ограничитель упирался в собственный потолок около 85 МБ/с
	 *          независимо от заданного предела. Оба исправлены: поле расширено
	 *          до разрядности разборщика, а размер операции задаётся окном
	 *          всплеска. После этого отладочная сборка даёт 1979-2046 МБ/с,
	 *          оптимизированная 1916-2538; порог задан с двукратным запасом от
	 *          нижней границы отладочной. Прежнее его значение в 250 МБ/с
	 *          соответствовало непочиненному состоянию и регрессию поймать
	 *          уже не могло
	 *
	 */
	static constexpr double OVERHEAD_THRESHOLD = 1000.0;
	/**
	 * @brief Объём передачи сценария цены учёта в октетах
	 *
	 */
	static constexpr size_t OVERHEAD_VOLUME = (64 * 1024 * 1024);
	/**
	 * @brief Предел пропускной способности сценария цены учёта
	 *
	 * @details Тридцать четыре гигабита в секунду петлевой интерфейс не выдаёт
	 *          ни на одной машине, поэтому предел заведомо не достигается и
	 *          передача идёт на полной скорости - но через весь код учёта
	 *          токенов. Предел выбран не круглым, а наибольшим, который прежде
	 *          выражался точно: поле предела было тридцатидвухразрядным, и
	 *          сотня гигабит в нём заворачивалась, обесценивая сценарий
	 *
	 */
	static constexpr const char * OVERHEAD_LIMIT = "34Gbps";
	/**
	 * @brief Глубина запаса очереди отправки сценариев ограничения полосы
	 *
	 * @details Очередь обязана оставаться непустой всё время замера, иначе
	 *          измерялся бы темп подачи данных сценарием, а не работа
	 *          ограничителя. Запас при этом ограничен: неограниченная подача
	 *          выбрала бы всю память процесса, потому что ограничитель отдаёт
	 *          в сокет медленнее, чем сценарий ставит в очередь
	 *
	 */
	static constexpr size_t BANDWIDTH_BACKLOG = (BANDWIDTH_CHUNK * 8);

	/**
	 * @brief Структура состояния прогона сценариев ограничения полосы
	 *
	 */
	typedef struct State {
		// Флаг завершения прогона сценария
		bool stop;
		// Флаг начала замера
		bool measuring;
		// Идентификатор события отправителя
		awh::event::id_t sender;
		// Количество принятых октетов
		size_t received;
		// Количество принятых октетов на момент начала замера
		size_t baseline;
		// Количество октетов, поставленных в очередь и ещё не записанных
		size_t backlog;
		// Флаг готовности отправителя к подаче данных
		bool ready;
		// Момент начала замера
		std::chrono::steady_clock::time_point start;
		// Момент окончания замера
		std::chrono::steady_clock::time_point finish;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit State() noexcept :
		 stop(false), measuring(false), sender(0),
		 received(0), baseline(0), backlog(0), ready(false) {}
	} state_t;

	/**
	 * @brief Функция прогона сценария удержания заданного предела
	 *
	 * @details Отправитель непрерывно держит очередь непустой, приёмник считает
	 *          принятые октеты. Ограничение ставится либо на отправку у клиента,
	 *          либо на приём у принятого сервером подключения - в обоих случаях
	 *          через ограничитель проходит весь поток, и достигнутая скорость
	 *          измеряется одинаково, по счётчику приёмника
	 *
	 * @param limiting режим ограничения пропускной способности
	 * @return         итоги прогона сценария
	 *
	 */
	static outcome_t sustain(const awh::event::limiting_t limiting) noexcept {
		// Итоги прогона сценария
		outcome_t result;
		// Состояние прогона сценария
		state_t state;
		// Блок передаваемых данных
		static vector <uint8_t> chunk(BANDWIDTH_CHUNK, 0x5A);
		// Создаём объект асинхронного движка ввода-вывода
		awh::engine::io_t io(framework(), logger());
		// Получаем свободный порт петлевого интерфейса
		const uint16_t number = port();
		// Добавляем новое событие сервера
		const awh::event::id_t server = io.event(awh::event::node_t::SERVER, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
		// Устанавливаем порт события сервера
		io.setSourcePort(server, number);
		// Если инициализация движка не выполнена
		if(!io.initialize())
			// Выводим пустые итоги прогона
			return result;
		// Устанавливаем опции события сервера
		io.setOptions(server, options());
		// Устанавливаем адрес события сервера
		io.setAddress(server, awh::event::address_t::IPV4, "127.0.0.1");
		// Устанавливаем функцию обратного вызова на принятие входящего подключения
		io.on(server, static_cast <awh::engine::callback::accept_t> ([&io, &state, limiting](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
			// Устанавливаем опции принятого подключения
			io.setOptions(cid, options());
			// Если ограничивается приём, ставим предел принятому подключению
			if(limiting == awh::event::limiting_t::INGRESS)
				// Устанавливаем предел пропускной способности на приём данных
				io.bandwidth(cid, awh::event::limiting_t::INGRESS, BANDWIDTH_LIMIT);
			/**
			 * Устанавливаем функцию обратного вызова на чтение из принятого
			 * подключения: приёмник только считает принятые октеты
			 */
			io.on(cid, [&state](const awh::event::id_t eid, const uint8_t *, const size_t size) noexcept -> void {
				// Накапливаем количество принятых октетов
				state.received += size;
				// Если замер выполняется и отведённое ему время истекло
				if(state.measuring && (std::chrono::duration <double> (std::chrono::steady_clock::now() - state.start).count() >= BANDWIDTH_SECONDS)){
					// Запоминаем момент окончания замера
					state.finish = std::chrono::steady_clock::now();
					// Закрываем окно замера: счётчики обязаны остановиться там же, где часы
					awh::benchmark::counting(false);
					awh::benchmark::syscall::counting(false);
					// Останавливаем цикл событий
					state.stop = true;
				}
			});
		}));
		// Выполняем фиксацию настроек события сервера
		io.commit(server);
		// Переводим событие сервера в режим прослушивания
		io.listen(server, BACKLOG);
		// Запускаем событие сервера
		io.launch(server);
		// Добавляем новое событие клиента
		const awh::event::id_t client = io.event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
		// Устанавливаем порт назначения события клиента
		io.setTargetPort(client, number);
		// Устанавливаем опции события клиента
		io.setOptions(client, options());
		// Устанавливаем адрес привязки события клиента
		io.setAddress(client, awh::event::address_t::IPV4, "0.0.0.0");
		// Устанавливаем адрес назначения события клиента
		io.setTarget(client, "127.0.0.1");
		// Запоминаем идентификатор события отправителя
		state.sender = client;
		// Устанавливаем функцию обратного вызова на подключение клиента
		io.on(client, static_cast <awh::engine::callback::connect_t> ([&io, &state, limiting](const awh::event::id_t eid, const bool ok) noexcept -> void {
			// Если подключение выполнено успешно
			if(ok){
				// Если ограничивается отправка, ставим предел отправителю
				if(limiting == awh::event::limiting_t::EGRESS)
					// Устанавливаем предел пропускной способности на отправку данных
					io.bandwidth(eid, awh::event::limiting_t::EGRESS, BANDWIDTH_LIMIT);
				// Включаем учёт выделений памяти
				awh::benchmark::counting(true);
				// Включаем учёт системных вызовов
				awh::benchmark::syscall::counting(true);
				// Отмечаем начало замера
				state.measuring = true;
				// Запоминаем момент начала замера
				state.start = std::chrono::steady_clock::now();
				// Запоминаем количество принятых октетов на момент начала замера
				state.baseline = state.received;
				// Отмечаем готовность отправителя к подаче данных
				state.ready = true;
			}
		}));
		/**
		 * Устанавливаем функцию обратного вызова на запись в событие клиента:
		 * записанный объём освобождает место в очереди, а подача идёт из цикла
		 * опроса - вызов отправки внутри этого обработчика возвращался бы в
		 * него же через саму отправку и уводил стек вглубь
		 */
		io.on(client, static_cast <awh::engine::callback::write_t> ([&state]([[maybe_unused]] const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Уменьшаем объём очереди, ожидающий записи
			state.backlog -= ((size < state.backlog) ? size : state.backlog);
		}));
		// Выполняем фиксацию настроек события клиента
		io.commit(client);
		// Выполняем подключение клиента к серверу
		io.connect(client);
		// Запускаем событие клиента
		io.launch(client);
		/**
		 * Запускаем опрос событий до истечения времени замера
		 */
		while(!state.stop && io.poll()){
			/**
			 * Держим очередь отправки непустой, не выбирая под неё всю память
			 */
			while(state.ready && !state.stop && (state.backlog < BANDWIDTH_BACKLOG)){
				// Ставим в очередь очередной блок передачи
				const size_t accepted = io.send(state.sender, chunk.data(), BANDWIDTH_CHUNK);
				// Если блок в очередь не принят
				if(accepted == 0)
					// Прекращаем подачу
					break;
				// Накапливаем объём очереди, ожидающий записи
				state.backlog += accepted;
			}
			// Если замер выполняется и отведённое ему время истекло
			if(state.measuring && (std::chrono::duration <double> (std::chrono::steady_clock::now() - state.start).count() >= BANDWIDTH_SECONDS)){
				// Запоминаем момент окончания замера
				state.finish = std::chrono::steady_clock::now();
				// Закрываем окно замера: счётчики обязаны остановиться там же, где часы
				awh::benchmark::counting(false);
				awh::benchmark::syscall::counting(false);
				// Останавливаем цикл событий
				state.stop = true;
			}
		}
		// Устанавливаем объём переданных за время замера данных
		result.bytes = (state.received - state.baseline);
		// Устанавливаем количество выполненных операций
		result.operations = (result.bytes / BANDWIDTH_CHUNK);
		// Устанавливаем затраченное время
		result.seconds = std::chrono::duration <double> (state.finish - state.start).count();
		// Снимаем показатели окружения по итогам замера
		collect(result);
		// Выполняем деинициализацию движка
		io.deinitialize();
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария цены учёта токенов
	 *
	 * @details Предел задан заведомо выше достижимого, поэтому передача идёт на
	 *          полной скорости, но через весь код учёта. Сравнение с показателем
	 *          потоковой передачи без ограничения даёт цену механизма
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t overhead() noexcept {
		// Итоги прогона сценария
		outcome_t result;
		// Состояние прогона сценария
		state_t state;
		// Блок передаваемых данных
		static vector <uint8_t> chunk(BANDWIDTH_CHUNK, 0x5A);
		// Создаём объект асинхронного движка ввода-вывода
		awh::engine::io_t io(framework(), logger());
		// Получаем свободный порт петлевого интерфейса
		const uint16_t number = port();
		// Добавляем новое событие сервера
		const awh::event::id_t server = io.event(awh::event::node_t::SERVER, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
		// Устанавливаем порт события сервера
		io.setSourcePort(server, number);
		// Если инициализация движка не выполнена
		if(!io.initialize())
			// Выводим пустые итоги прогона
			return result;
		// Устанавливаем опции события сервера
		io.setOptions(server, options());
		// Устанавливаем адрес события сервера
		io.setAddress(server, awh::event::address_t::IPV4, "127.0.0.1");
		// Устанавливаем функцию обратного вызова на принятие входящего подключения
		io.on(server, static_cast <awh::engine::callback::accept_t> ([&io, &state](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
			// Устанавливаем опции принятого подключения
			io.setOptions(cid, options());
			// Устанавливаем недостижимый предел пропускной способности на приём данных
			io.bandwidth(cid, awh::event::limiting_t::INGRESS, OVERHEAD_LIMIT);
			// Устанавливаем функцию обратного вызова на чтение из принятого подключения
			io.on(cid, [&state](const awh::event::id_t eid, const uint8_t *, const size_t size) noexcept -> void {
				// Накапливаем количество принятых октетов
				state.received += size;
				// Если весь объём передачи принят
				if(state.received >= OVERHEAD_VOLUME){
					// Запоминаем момент окончания замера
					state.finish = std::chrono::steady_clock::now();
					// Закрываем окно замера: счётчики обязаны остановиться там же, где часы
					awh::benchmark::counting(false);
					awh::benchmark::syscall::counting(false);
					// Останавливаем цикл событий
					state.stop = true;
				}
			});
		}));
		// Выполняем фиксацию настроек события сервера
		io.commit(server);
		// Переводим событие сервера в режим прослушивания
		io.listen(server, BACKLOG);
		// Запускаем событие сервера
		io.launch(server);
		// Добавляем новое событие клиента
		const awh::event::id_t client = io.event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
		// Устанавливаем порт назначения события клиента
		io.setTargetPort(client, number);
		// Устанавливаем опции события клиента
		io.setOptions(client, options());
		// Устанавливаем адрес привязки события клиента
		io.setAddress(client, awh::event::address_t::IPV4, "0.0.0.0");
		// Устанавливаем адрес назначения события клиента
		io.setTarget(client, "127.0.0.1");
		// Запоминаем идентификатор события отправителя
		state.sender = client;
		// Устанавливаем функцию обратного вызова на подключение клиента
		io.on(client, static_cast <awh::engine::callback::connect_t> ([&io, &state](const awh::event::id_t eid, const bool ok) noexcept -> void {
			// Если подключение выполнено успешно
			if(ok){
				// Устанавливаем недостижимый предел пропускной способности на отправку данных
				io.bandwidth(eid, awh::event::limiting_t::EGRESS, OVERHEAD_LIMIT);
				// Включаем учёт выделений памяти
				awh::benchmark::counting(true);
				// Включаем учёт системных вызовов
				awh::benchmark::syscall::counting(true);
				// Запоминаем момент начала замера
				state.start = std::chrono::steady_clock::now();
				// Отмечаем готовность отправителя к подаче данных
				state.ready = true;
			}
		}));
		// Устанавливаем функцию обратного вызова на запись в событие клиента
		io.on(client, static_cast <awh::engine::callback::write_t> ([&state]([[maybe_unused]] const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Уменьшаем объём очереди, ожидающий записи
			state.backlog -= ((size < state.backlog) ? size : state.backlog);
		}));
		// Выполняем фиксацию настроек события клиента
		io.commit(client);
		// Выполняем подключение клиента к серверу
		io.connect(client);
		// Запускаем событие клиента
		io.launch(client);
		/**
		 * Запускаем опрос событий до передачи всего объёма
		 */
		while(!state.stop && io.poll()){
			/**
			 * Держим очередь отправки непустой, не выбирая под неё всю память
			 */
			while(state.ready && !state.stop && (state.backlog < BANDWIDTH_BACKLOG)){
				// Ставим в очередь очередной блок передачи
				const size_t accepted = io.send(state.sender, chunk.data(), BANDWIDTH_CHUNK);
				// Если блок в очередь не принят
				if(accepted == 0)
					// Прекращаем подачу
					break;
				// Накапливаем объём очереди, ожидающий записи
				state.backlog += accepted;
			}
		}
		// Устанавливаем объём переданных данных
		result.bytes = state.received;
		// Устанавливаем количество выполненных операций
		result.operations = (state.received / BANDWIDTH_CHUNK);
		// Устанавливаем затраченное время
		result.seconds = std::chrono::duration <double> (state.finish - state.start).count();
		// Снимаем показатели окружения по итогам замера
		collect(result);
		// Выполняем деинициализацию движка
		io.deinitialize();
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона удержания предела на отправке
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & egress() noexcept {
		// Итоги прогона сценария удержания предела на отправке
		static const outcome_t result = ::sustain(awh::event::limiting_t::EGRESS);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона удержания предела на приёме
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & ingress() noexcept {
		// Итоги прогона сценария удержания предела на приёме
		static const outcome_t result = ::sustain(awh::event::limiting_t::INGRESS);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона цены учёта токенов
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & measured() noexcept {
		// Итоги прогона сценария цены учёта токенов
		static const outcome_t result = ::overhead();
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция вычисления отношения достигнутой скорости к заданному пределу
	 *
	 * @param output итоги прогона сценария
	 * @return       отношение достигнутой скорости к заданному пределу
	 *
	 */
	static double attained(const outcome_t & output) noexcept {
		// Если время замера не снято, отношение вычислить не из чего
		if(output.seconds <= 0.0)
			// Выводим нулевое отношение
			return 0.0;
		// Выводим отношение достигнутой скорости к заданному пределу
		return ((static_cast <double> (output.bytes) / output.seconds) / BANDWIDTH_LIMIT_BYTES);
	}
	/**
	 * @brief Функция прогона сценария удержания предела на отправке
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t egressAccuracy() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем итоги прогона сценария
		const outcome_t & outcome = ::egress();
		// Устанавливаем измеренное значение
		result.value = ::attained(outcome);
		// Убеждаемся, что сценарий выполнил хоть одну операцию
		validate(result, outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария использования предела на отправке
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t egressUtilization() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем итоги прогона сценария
		const outcome_t & outcome = ::egress();
		// Устанавливаем измеренное значение
		result.value = ::attained(outcome);
		// Убеждаемся, что сценарий выполнил хоть одну операцию
		validate(result, outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария удержания предела на приёме
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t ingressAccuracy() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем итоги прогона сценария
		const outcome_t & outcome = ::ingress();
		// Устанавливаем измеренное значение
		result.value = ::attained(outcome);
		// Убеждаемся, что сценарий выполнил хоть одну операцию
		validate(result, outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария использования предела на приёме
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t ingressUtilization() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем итоги прогона сценария
		const outcome_t & outcome = ::ingress();
		// Устанавливаем измеренное значение
		result.value = ::attained(outcome);
		// Убеждаемся, что сценарий выполнил хоть одну операцию
		validate(result, outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария цены учёта токенов
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t throughput() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем итоги прогона сценария
		const outcome_t & outcome = ::measured();
		// Устанавливаем измеренное значение
		result.value = megabytes(outcome);
		// Убеждаемся, что сценарий выполнил хоть одну операцию
		validate(result, outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	// Регистрируем сценарий непревышения заданного предела на отправке
	static const bool gEgressAccuracy = awh::benchmark::add(
		"net/io/bandwidth/egress-accuracy", "доля предела", ACCURACY_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, &::egressAccuracy
	);
	// Регистрируем сценарий использования заданного предела на отправке
	static const bool gEgressUtilization = awh::benchmark::add(
		"net/io/bandwidth/egress-utilization", "доля предела", UTILIZATION_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::egressUtilization
	);
	// Регистрируем сценарий непревышения заданного предела на приёме
	static const bool gIngressAccuracy = awh::benchmark::add(
		"net/io/bandwidth/ingress-accuracy", "доля предела", ACCURACY_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, &::ingressAccuracy
	);
	// Регистрируем сценарий использования заданного предела на приёме
	static const bool gIngressUtilization = awh::benchmark::add(
		"net/io/bandwidth/ingress-utilization", "доля предела", UTILIZATION_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::ingressUtilization
	);
	// Регистрируем сценарий цены учёта токенов
	static const bool gThroughput = awh::benchmark::add(
		"net/io/bandwidth/unreached-throughput", "МБ/с", OVERHEAD_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::throughput
	);
};
