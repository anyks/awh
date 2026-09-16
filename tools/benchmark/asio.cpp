/**
 * @file asio.cpp
 * @date 2026-09-16
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
 * @brief Эталонный стенд сравнения на Boost.Asio — те же сценарии нагрузки, что и у
 *        бенчмарков сетевого движка AWH, выполненные средствами её цикла событий
 *
 * @details Библиотека работает по договору завершения операций: наружу отдаётся не
 *          готовность описателя, а уже выполненный обмен. Под MS Windows она опирается
 *          на порт завершения — тот же механизм, каким пользуется движок AWH, и это
 *          делает сличение осмысленным: сравниваются не механизмы ожидания, а их
 *          применение
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <memory>
#include <array>

/**
 * Подключаем заголовочные файлы библиотеки Boost.Asio
 *
 * @note Библиотека заголовочная: связывания с ней не требуется вовсе, у MS Windows
 *       нужны лишь средства сокетов самой системы
 */
#include <boost/asio.hpp>

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Используем пространство имён общего окружения эталонных стендов
 */
using namespace rival;

/**
 * Сокращаем обращение к пространству имён библиотеки
 */
namespace asio = boost::asio;

/**
 * @brief Внутреннее окружение стенда
 *
 */
namespace {
	/**
	 * @brief Состояние прогона сценария обмена короткими сообщениями
	 *
	 */
	struct echo_t {
		// Количество выполненных обменов
		size_t done;
		// Количество требуемых обменов
		size_t rounds;
		/**
		 * @brief Конструктор
		 *
		 * @param rounds количество требуемых обменов
		 *
		 */
		explicit echo_t(const size_t rounds) noexcept : done(0), rounds(rounds) {}
	};
	/**
	 * @brief Состояние обслуживаемого подключения
	 *
	 * @note Буфер отведён под нагрузку сценария и переиспользуется: отведение памяти
	 *       на каждом обмене мерило бы распределитель памяти, а не цикл событий
	 *
	 */
	struct session_t {
		// Сокет подключения
		asio::ip::tcp::socket socket;
		// Буфер обмена данными подключения
		array <char, ECHO_PAYLOAD> buffer;
		/**
		 * @brief Конструктор
		 *
		 * @param context цикл событий стенда
		 *
		 */
		explicit session_t(asio::io_context & context) noexcept : socket(context), buffer{} {}
	};
	/**
	 * @brief Функция обслуживания стороны сервера
	 *
	 * @details Сервер читает сообщение целиком и возвращает его обратно, после чего
	 *          снова встаёт на чтение. Обмен считается выполненным стороной клиента,
	 *          а не сервера: так его считает и набор замеров движка
	 *
	 * @param session обслуживаемое подключение
	 *
	 */
	void serve(const shared_ptr <session_t> & session) noexcept {
		// Выполняем чтение сообщения целиком
		asio::async_read(session->socket, asio::buffer(session->buffer),
		[session](const boost::system::error_code & error, const size_t size) noexcept -> void {
			// Если чтение сообщения не удалось, обслуживание прекращается
			if(error) return;
			// Выполняем возврат прочитанного сообщения отправителю
			asio::async_write(session->socket, asio::buffer(session->buffer.data(), size),
			[session](const boost::system::error_code & error, const size_t) noexcept -> void {
				// Если запись сообщения не удалась, обслуживание прекращается
				if(error) return;
				// Встаём на чтение следующего сообщения
				::serve(session);
			});
		});
	}
	/**
	 * @brief Функция приёма подключений стороной сервера
	 *
	 * @param acceptor слушающий сокет стенда
	 * @param context  цикл событий стенда
	 *
	 */
	void accepting(asio::ip::tcp::acceptor & acceptor, asio::io_context & context) noexcept {
		// Создаём состояние принимаемого подключения
		shared_ptr <session_t> session = make_shared <session_t> (context);
		// Выполняем приём входящего подключения
		acceptor.async_accept(session->socket,
		[&acceptor, &context, session](const boost::system::error_code & error) noexcept -> void {
			// Если подключение принято
			if(!error){
				// Отключаем алгоритм Нейгла принятого подключения
				session->socket.set_option(asio::ip::tcp::no_delay(true));
				// Запускаем обслуживание принятого подключения
				::serve(session);
				// Встаём на приём следующего подключения
				::accepting(acceptor, context);
			}
		});
	}
	/**
	 * @brief Функция выполнения обмена стороной клиента
	 *
	 * @param session обслуживаемое подключение
	 * @param state   состояние прогона сценария
	 * @param context цикл событий стенда
	 *
	 */
	void exchanging(const shared_ptr <session_t> & session, echo_t * state, asio::io_context & context) noexcept {
		// Если требуемое количество обменов уже выполнено
		if(state->done >= state->rounds){
			// Останавливаем цикл событий стенда
			context.stop();
			// Выходим: обмениваться больше не нужно
			return;
		}
		// Выполняем отправку сообщения стороне сервера
		asio::async_write(session->socket, asio::buffer(session->buffer),
		[session, state, &context](const boost::system::error_code & error, const size_t) noexcept -> void {
			// Если отправка сообщения не удалась, обмен прекращается
			if(error) return;
			// Выполняем чтение возвращённого сообщения целиком
			asio::async_read(session->socket, asio::buffer(session->buffer),
			[session, state, &context](const boost::system::error_code & error, const size_t) noexcept -> void {
				// Если чтение возвращённого сообщения не удалось, обмен прекращается
				if(error) return;
				// Увеличиваем количество выполненных обменов
				state->done++;
				// Выполняем следующий обмен
				::exchanging(session, state, context);
			});
		});
	}
	/**
	 * @brief Функция прогона сценария обмена короткими сообщениями
	 *
	 * @param connections количество одновременных подключений
	 * @param rounds      количество требуемых обменов
	 * @return            итоги прогона сценария
	 *
	 */
	outcome_t exchange(const size_t connections, const size_t rounds) noexcept {
		// Итоги прогона сценария
		outcome_t result;
		// Состояние прогона сценария
		echo_t state(rounds);
		// Цикл событий стенда
		asio::io_context context(1);
		// Слушающий сокет стенда
		asio::ip::tcp::acceptor acceptor(context, asio::ip::tcp::endpoint(asio::ip::make_address("127.0.0.1"), 0));
		// Встаём на приём входящих подключений
		::accepting(acceptor, context);
		// Список состояний клиентских подключений
		vector <shared_ptr <session_t>> clients;
		// Резервируем память под состояния клиентских подключений
		clients.reserve(connections);
		/**
		 * Выполняем создание требуемого количества клиентских подключений
		 */
		for(size_t i = 0; i < connections; i++){
			// Создаём состояние клиентского подключения
			clients.push_back(make_shared <session_t> (context));
			// Получаем состояние созданного подключения
			shared_ptr <session_t> session = clients.back();
			// Выполняем подключение к слушающему сокету
			session->socket.async_connect(acceptor.local_endpoint(),
			[session, &state, &context](const boost::system::error_code & error) noexcept -> void {
				// Если подключение не удалось, обмен не начинается
				if(error) return;
				// Отключаем алгоритм Нейгла подключения
				session->socket.set_option(asio::ip::tcp::no_delay(true));
				// Запускаем обмен сообщениями
				::exchanging(session, &state, context);
			});
		}
		// Момент начала замера
		const auto start = now();
		// Запускаем цикл событий до выполнения требуемого количества обменов
		context.run();
		// Устанавливаем затраченное время
		result.seconds = elapsed(start, now());
		// Устанавливаем количество выполненных операций
		result.operations = state.done;
		// Устанавливаем объём переданных данных с учётом обоих направлений обмена
		result.bytes = (state.done * ECHO_PAYLOAD * 2);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Состояние прогона сценария датаграммного обмена
	 *
	 */
	struct datagram_t {
		// Количество выполненных обменов
		size_t done;
		// Количество требуемых обменов
		size_t rounds;
		// Сокет приёмника датаграмм
		asio::ip::udp::socket receiver;
		// Адрес стороны, приславшей датаграмму
		asio::ip::udp::endpoint source;
		// Буфер обмена данными приёмника
		array <char, ECHO_PAYLOAD> buffer;
		/**
		 * @brief Конструктор
		 *
		 * @param context цикл событий стенда
		 * @param rounds  количество требуемых обменов
		 *
		 */
		explicit datagram_t(asio::io_context & context, const size_t rounds) noexcept :
		 done(0), rounds(rounds),
		 receiver(context, asio::ip::udp::endpoint(asio::ip::make_address("127.0.0.1"), 0)),
		 buffer{} {}
	};
	/**
	 * @brief Состояние отправителя датаграмм
	 *
	 */
	struct emitter_t {
		// Сокет отправителя датаграмм
		asio::ip::udp::socket socket;
		// Буфер обмена данными отправителя
		array <char, ECHO_PAYLOAD> buffer;
		/**
		 * @brief Конструктор
		 *
		 * @param context цикл событий стенда
		 *
		 */
		explicit emitter_t(asio::io_context & context) noexcept :
		 socket(context, asio::ip::udp::endpoint(asio::ip::make_address("127.0.0.1"), 0)), buffer{} {}
	};
	/**
	 * @brief Функция обслуживания приёмника датаграмм
	 *
	 * @details Приёмник возвращает датаграмму приславшей стороне: обмен считается
	 *          выполненным, когда отправитель получил её обратно
	 *
	 * @param state состояние прогона сценария
	 *
	 */
	void receiving(datagram_t * state) noexcept {
		// Выполняем приём датаграммы
		state->receiver.async_receive_from(asio::buffer(state->buffer), state->source,
		[state](const boost::system::error_code & error, const size_t size) noexcept -> void {
			// Если приём датаграммы не удался, обслуживание прекращается
			if(error) return;
			// Выполняем возврат датаграммы приславшей стороне
			state->receiver.async_send_to(asio::buffer(state->buffer.data(), size), state->source,
			[state](const boost::system::error_code & error, const size_t) noexcept -> void {
				// Если возврат датаграммы не удался, обслуживание прекращается
				if(error) return;
				// Встаём на приём следующей датаграммы
				::receiving(state);
			});
		});
	}
	/**
	 * @brief Функция выполнения датаграммного обмена стороной отправителя
	 *
	 * @param emitter  состояние отправителя датаграмм
	 * @param state    состояние прогона сценария
	 * @param endpoint адрес приёмника датаграмм
	 * @param context  цикл событий стенда
	 *
	 */
	void emitting(const shared_ptr <emitter_t> & emitter, datagram_t * state, const asio::ip::udp::endpoint & endpoint, asio::io_context & context) noexcept {
		// Если требуемое количество обменов уже выполнено
		if(state->done >= state->rounds){
			// Останавливаем цикл событий стенда
			context.stop();
			// Выходим: обмениваться больше не нужно
			return;
		}
		// Выполняем отправку датаграммы приёмнику
		emitter->socket.async_send_to(asio::buffer(emitter->buffer), endpoint,
		[emitter, state, endpoint, &context](const boost::system::error_code & error, const size_t) noexcept -> void {
			// Если отправка датаграммы не удалась, обмен прекращается
			if(error) return;
			// Выполняем приём возвращённой датаграммы
			emitter->socket.async_receive(asio::buffer(emitter->buffer),
			[emitter, state, endpoint, &context](const boost::system::error_code & error, const size_t) noexcept -> void {
				// Если приём возвращённой датаграммы не удался, обмен прекращается
				if(error) return;
				// Увеличиваем количество выполненных обменов
				state->done++;
				// Выполняем следующий обмен
				::emitting(emitter, state, endpoint, context);
			});
		});
	}
	/**
	 * @brief Функция прогона сценария датаграммного обмена
	 *
	 * @note Приёмник у сценария ОДИН на всех отправителей: принятого сокета на
	 *       каждого отправителя у датаграммного сервера не возникает. Тем сценарий и
	 *       отличается от потокового, и числа их прямо сопоставлять нельзя
	 *
	 * @param emitters количество одновременных отправителей
	 * @param rounds   количество требуемых обменов
	 * @return         итоги прогона сценария
	 *
	 */
	outcome_t datagram(const size_t emitters, const size_t rounds) noexcept {
		// Итоги прогона сценария
		outcome_t result;
		// Цикл событий стенда
		asio::io_context context(1);
		// Состояние прогона сценария
		datagram_t state(context, rounds);
		// Адрес приёмника датаграмм
		const asio::ip::udp::endpoint endpoint = state.receiver.local_endpoint();
		// Встаём на приём датаграмм
		::receiving(&state);
		// Список состояний отправителей датаграмм
		vector <shared_ptr <emitter_t>> senders;
		// Резервируем память под состояния отправителей датаграмм
		senders.reserve(emitters);
		/**
		 * Выполняем создание требуемого количества отправителей датаграмм
		 */
		for(size_t i = 0; i < emitters; i++){
			// Создаём состояние отправителя датаграмм
			senders.push_back(make_shared <emitter_t> (context));
			// Запускаем датаграммный обмен
			::emitting(senders.back(), &state, endpoint, context);
		}
		// Момент начала замера
		const auto start = now();
		// Запускаем цикл событий до выполнения требуемого количества обменов
		context.run();
		// Устанавливаем затраченное время
		result.seconds = elapsed(start, now());
		// Устанавливаем количество выполненных операций
		result.operations = state.done;
		// Устанавливаем объём переданных данных с учётом обоих направлений обмена
		result.bytes = (state.done * ECHO_PAYLOAD * 2);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария приёма подключений
	 *
	 * @details Меряется полный круг короткоживущего соединения: заведение сокета,
	 *          подключение, приём сервером и освобождение обеих сторон. Круги идут
	 *          последовательно, по одному за раз
	 *
	 * @return итоги прогона сценария
	 *
	 */
	outcome_t accepting() noexcept {
		// Итоги прогона сценария
		outcome_t result;
		// Цикл событий стенда
		asio::io_context context(1);
		// Слушающий сокет стенда
		asio::ip::tcp::acceptor acceptor(context, asio::ip::tcp::endpoint(asio::ip::make_address("127.0.0.1"), 0));
		// Адрес слушающего сокета
		const asio::ip::tcp::endpoint endpoint = acceptor.local_endpoint();
		// Количество выполненных кругов
		size_t done = 0;
		// Момент начала замера
		auto start = now();
		/**
		 * Выполняем требуемое количество кругов подключения
		 */
		for(size_t i = 0; i < (ACCEPT_WARMUP + ACCEPT_ROUNDS); i++){
			// Сокет принимаемого подключения
			asio::ip::tcp::socket accepted(context);
			// Сокет исходящего подключения
			asio::ip::tcp::socket connected(context);
			// Признак принятого подключения
			bool taken = false;
			// Выполняем приём входящего подключения
			acceptor.async_accept(accepted, [&taken](const boost::system::error_code & error) noexcept -> void {
				// Устанавливаем признак принятого подключения
				taken = !error;
			});
			// Признак выполненного подключения
			bool joined = false;
			// Выполняем подключение к слушающему сокету
			connected.async_connect(endpoint, [&joined](const boost::system::error_code & error) noexcept -> void {
				// Устанавливаем признак выполненного подключения
				joined = !error;
			});
			/**
			 * Прокручиваем цикл событий до завершения обеих сторон круга
			 */
			while(!taken || !joined){
				// Если событий больше нет, круг считается неудавшимся
				if(context.run_one() == 0) break;
			}
			// Возвращаем цикл событий в рабочее состояние для следующего круга
			context.restart();
			/**
			 * Если круг выполнен полностью
			 */
			if(taken && joined){
				// Включаем немедленный обрыв соединения при закрытии сокета
				hardClose(static_cast <socket_t> (connected.native_handle()));
				// Включаем немедленный обрыв соединения принятой стороны
				hardClose(static_cast <socket_t> (accepted.native_handle()));
				// Если разогрев окончен, увеличиваем количество выполненных кругов
				if(i >= ACCEPT_WARMUP) done++;
			}
			// Если разогрев только что окончен, замер начинается заново
			if(i == (ACCEPT_WARMUP - 1)) start = now();
		}
		// Устанавливаем затраченное время
		result.seconds = elapsed(start, now());
		// Устанавливаем количество выполненных операций
		result.operations = done;
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария постановки и срабатывания таймеров
	 *
	 * @note Дедлайны разнесены так же, как у набора замеров движка: иначе мерилась бы
	 *       не постановка таймеров, а ожидание одного и того же мгновения
	 *
	 * @return итоги прогона сценария
	 *
	 */
	outcome_t timers() noexcept {
		// Итоги прогона сценария
		outcome_t result;
		// Цикл событий стенда
		asio::io_context context(1);
		// Количество сработавших таймеров
		size_t fired = 0;
		// Список таймеров стенда
		vector <unique_ptr <asio::steady_timer>> list;
		// Резервируем память под таймеры стенда
		list.reserve(DEADLINE_COUNT);
		// Момент начала замера
		const auto start = now();
		/**
		 * Выполняем постановку требуемого количества таймеров
		 */
		for(size_t i = 0; i < DEADLINE_COUNT; i++){
			// Создаём таймер стенда
			list.push_back(unique_ptr <asio::steady_timer> (new asio::steady_timer(context)));
			// Устанавливаем срок срабатывания таймера
			list.back()->expires_after(std::chrono::milliseconds(static_cast <int64_t> (i % DEADLINE_SPREAD)));
			// Выполняем постановку таймера
			list.back()->async_wait([&fired](const boost::system::error_code & error) noexcept -> void {
				// Если таймер сработал, увеличиваем количество сработавших
				if(!error) fired++;
			});
		}
		// Запускаем цикл событий до срабатывания всех таймеров
		context.run();
		// Устанавливаем затраченное время
		result.seconds = elapsed(start, now());
		// Устанавливаем количество выполненных операций
		result.operations = fired;
		// Выводим итоги прогона сценария
		return result;
	}
}

/**
 * @brief Точка входа эталонного стенда
 *
 * @param argc количество доводов запуска
 * @param argv список доводов запуска
 * @return     код завершения стенда
 *
 */
int32_t main(int32_t argc, char * argv[]) noexcept {
	// Поднимаем предел открытых описателей
	limits();
	// Фильтр названий сценариев
	const char * name = filter(argc, argv);
	/**
	 * Если сценарий обмена на одном подключении выполняется
	 */
	if(selected("net/io/echo/single", name)){
		// Выполняем прогон сценария обмена на одном подключении
		const outcome_t outcome = ::exchange(1, ECHO_SINGLE_ROUNDS);
		// Выводим результат прогона сценария
		report("net/io/echo/single", "обменов/с", perSecond(outcome), outcome);
	}
	/**
	 * Если сценарий обмена на множестве подключений выполняется
	 */
	if(selected("net/io/echo/multi", name)){
		// Выполняем прогон сценария обмена на множестве подключений
		const outcome_t outcome = ::exchange(ECHO_MULTI_CONNECTIONS, ECHO_MULTI_ROUNDS);
		// Выводим результат прогона сценария
		report("net/io/echo/multi", "обменов/с", perSecond(outcome), outcome);
	}
	/**
	 * Если сценарий датаграммного обмена одним отправителем выполняется
	 */
	if(selected("net/io/datagram/single", name)){
		// Выполняем прогон сценария датаграммного обмена одним отправителем
		const outcome_t outcome = ::datagram(1, ECHO_SINGLE_ROUNDS);
		// Выводим результат прогона сценария
		report("net/io/datagram/single", "обменов/с", perSecond(outcome), outcome);
	}
	/**
	 * Если сценарий датаграммного обмена множеством отправителей выполняется
	 */
	if(selected("net/io/datagram/multi", name)){
		// Выполняем прогон сценария датаграммного обмена множеством отправителей
		const outcome_t outcome = ::datagram(ECHO_MULTI_CONNECTIONS, ECHO_MULTI_ROUNDS);
		// Выводим результат прогона сценария
		report("net/io/datagram/multi", "обменов/с", perSecond(outcome), outcome);
	}
	/**
	 * Если сценарий приёма подключений выполняется
	 */
	if(selected("net/io/accept/connections", name)){
		// Выполняем прогон сценария приёма подключений
		const outcome_t outcome = ::accepting();
		// Выводим результат прогона сценария
		report("net/io/accept/connections", "подключений/с", perSecond(outcome), outcome);
	}
	/**
	 * Если сценарий постановки и срабатывания таймеров выполняется
	 */
	if(selected("net/io/timers", name)){
		// Выполняем прогон сценария постановки и срабатывания таймеров
		const outcome_t outcome = ::timers();
		// Выводим результат прогона сценария
		report("net/io/timers", "таймеров/с", perSecond(outcome), outcome);
	}
	// Выводим успешный код завершения стенда
	return EXIT_SUCCESS;
}
