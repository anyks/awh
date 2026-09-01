/**
 * @file blocking.cpp
 * @date 2026-08-31
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
 * @brief Проверки обмена по БЛОКИРУЮЩЕМУ сокету
 *
 * @details Блокирующий режим - умолчание движка: неблокирующим сокет делает
 *          признак `NO_IO_BLOCK`, который пользователь может и не ставить.
 *          Прочие проверки набора ставят его все до одной, отчего непокрытым
 *          оставалось ровно то поведение, какое получает пользователь,
 *          ничего не настроивший.
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "io.hpp"

/**
 * Стандартные заголовочные файлы
 */
#include <thread>
#include <atomic>
#include <chrono>
#include <cstdlib>
/**
 * Средства пробуждения вставшего обмена разнятся по системам
 *
 * @details У систем POSIX вставший приём прерывается сигналом без SA_RESTART, у
 *          MS Windows сигналами он не прерывается вовсе - там для того заведено
 *          своё обращение, снимающее синхронный обмен в УКАЗАННОМ потоке
 *
 */
#if defined(_WIN32) || defined(_WIN64)
	#include <sys/win32.hpp>
#else
	#include <csignal>
	#include <pthread.h>
	#include <unistd.h>
#endif

/**
 * Пространство имён проверок блокирующего обмена
 *
 * @note Имена держатся в безымянной области намеренно: набор `net` собирается
 *       единой программой, и вынос их наружу столкнул бы одинаковые помощники
 *       разных единиц трансляции
 */
namespace {
	// Признак того, что сторож будил поток цикла
	static std::atomic <bool> __awh_stalled__(false);
	// Счётчик оборотов опроса, по которому судят о живости цикла
	static std::atomic <uint64_t> __awh_rounds__(0);
	/**
	 * @brief Обработчик пробуждающего сигнала
	 *
	 * @param signal номер полученного сигнала
	 *
	 */
#if !defined(_WIN32) && !defined(_WIN64)
	static void __awh_wake__(int32_t signal) noexcept {
		// Отмечаем, что сторож вмешался
		(void) signal;
		__awh_stalled__.store(true);
	}
#endif
	/**
	 * @brief Выдача порта проверкам блокирующего обмена
	 *
	 * @return порт из разряда, отведённого проверкам
	 *
	 */
	static uint16_t blockingPort() noexcept {
		// Границы разряда, отводимого проверкам
		uint16_t begin = 0, end = 0;
		// Выполняем выбор разряда портов для проверок
		__awh_test_port_range__(begin, end);
		// Счётчик, разводящий проверки этого файла по разным портам
		static uint16_t offset = 0;
		// Выводим очередной порт разряда
		return (begin + ((offset++ + 211) % ((end > begin) ? (end - begin) : 1)));
	}
}

/**
 * @brief Обмен в обе стороны по блокирующему сокету
 *
 * @details Утверждается не только доставка октетов, но и то, что оборот опроса
 *          ВЕРНУЛСЯ. Разница существенна: приём по блокирующему сокету, уйдя в
 *          ожидание внутри оборота, не отменяется ничем - ни сроком в условии
 *          цикла, ибо условие проверяется МЕЖДУ оборотами, ни закрытием узла,
 *          ибо закрывать его некому. Проверка без этого утверждения при дефекте
 *          не провалилась бы, а повисла, утянув за собой весь набор.
 *
 *          Пробуждение делается сигналом: обработчик ставится БЕЗ `SA_RESTART`,
 *          отчего застрявший приём возвращает EINTR, движок доводит оборот до
 *          конца, а проверка кончается внятным отказом.
 */
TEST_F(IoFixture, IoBlockingExchangeTest){
	// Признаки прохождения обмена в обе стороны
	std::atomic <bool> delivered(false), echoed(false), finished(false);
	// Сбрасываем признак вмешательства сторожа и счётчик оборотов
	__awh_stalled__.store(false);
	__awh_rounds__.store(0);
	// Получаем порт, на котором пойдёт обмен
	const uint16_t port = blockingPort();
	// Сообщение, гоняемое в обе стороны
	const std::string message = "AWH blocking socket exchange";
	// Получаем пару событий для обмена по протоколу TCP
	const auto events = this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
	// Выполняем проверку, что события созданы
	for(uint8_t i = 0; i < 2; i++)
		ASSERT_GT(events[i], 0);
	// Устанавливаем порт назначения клиенту и порт источника серверу
	ASSERT_TRUE(this->_io->setTargetPort(events[0], port));
	ASSERT_TRUE(this->_io->setSourcePort(events[1], port));
	// Выполняем инициализацию движка
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Признаки узлов НАМЕРЕННО без `NO_IO_BLOCK`: сокеты остаются блокирующими,
	 * как и задумано умолчанием движка. Ради этого проверка и заведена
	 */
	const uint16_t options = (
		awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE |
		awh::event::options::REUSE_ADDR | awh::event::options::CLOSE_ON_EXEC |
		awh::event::options::TCP_NO_DELAY
	);
	for(uint8_t i = 0; i < 2; i++)
		ASSERT_TRUE(this->_io->setOptions(events[i], options));
	// Устанавливаем адрес прослушивания серверу
	ASSERT_TRUE(this->_io->setAddress(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
	// Устанавливаем функцию обратного вызова принятия подключения
	this->_io->on(events[1], static_cast <awh::engine::callback::accept_t> ([&delivered, this](const awh::event::id_t, const awh::event::id_t cid) noexcept -> void {
		// Принятому узлу признаки ставятся те же, блокирующие
		EXPECT_TRUE(this->_io->setOptions(cid, options));
		// Устанавливаем функцию обратного вызова получения данных
		this->_io->on(cid, [&delivered, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Отмечаем доставку данных серверу
			delivered.store(true);
			/**
			 * Задержка отклика переменной среды заведена НАМЕРЕННО: отклик
			 * зовётся на стеке оборота опроса, и застряв в нём, оборот встаёт
			 * в точности так, как встал бы внутри приёма. Ею проверяется, что
			 * сторож ЗАМЕЧАЕТ вставший цикл, а не только отсутствие данных
			 */
			if(::getenv("AWH_TEST_STALL_READ") != nullptr)
				::sleep(30);
			// Отправляем полученные данные обратно клиенту
			this->_io->send(eid, reinterpret_cast <const char *> (data), size);
		});
	}));
	// Выполняем закрепление настроек сервера и его запуск
	ASSERT_TRUE(this->_io->commit(events[1]));
	ASSERT_TRUE(this->_io->listen(events[1], 100));
	ASSERT_TRUE(this->_io->launch(events[1]));
	// Устанавливаем адрес и цель клиенту
	ASSERT_TRUE(this->_io->setAddress(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
	ASSERT_TRUE(this->_io->setTarget(events[0], "127.0.0.1"));
	// Устанавливаем функцию обратного вызова подключения клиента
	this->_io->on(events[0], static_cast <awh::engine::callback::connect_t> ([&message, this](const awh::event::id_t eid, const bool ok) noexcept -> void {
		// Если подключение удалось, отправляем сообщение серверу
		/**
		 * Глушение отправки переменной среды заведено НАМЕРЕННО и оставлено:
		 * им проверяется, что сама проверка умеет провалиться, а не проходит
		 * по недосмотру. Без такой пробы утверждение о невставшем цикле
		 * осталось бы украшением - оно ведь ни разу не срабатывало
		 */
		if(ok && (::getenv("AWH_TEST_MUTE_SEND") == nullptr))
			this->_io->send(eid, message.c_str(), message.size());
	}));
	// Устанавливаем функцию обратного вызова получения ответа сервера
	this->_io->on(events[0], [&echoed, &message](const awh::event::id_t, const uint8_t * data, const size_t size) noexcept -> void {
		// Отмечаем возврат сообщения, сличая его с отправленным
		echoed.store(message.compare(0, message.size(), reinterpret_cast <const char *> (data), size) == 0);
	});
	// Выполняем закрепление настроек клиента, подключение и запуск
	ASSERT_TRUE(this->_io->commit(events[0]));
	ASSERT_TRUE(this->_io->connect(events[0]));
	ASSERT_TRUE(this->_io->launch(events[0]));
	/**
	 * Ставим обработчик пробуждающего сигнала БЕЗ `SA_RESTART`: только так
	 * застрявший в ядре приём вернёт EINTR, а не продолжится после сигнала
	 */
#if defined(_WIN32) || defined(_WIN64)
	/**
	 * Описатель потока, в котором крутится цикл событий
	 *
	 * @note Обращение снятия синхронного обмена требует ОПИСАТЕЛЯ потока, а
	 *       описатель текущего потока - величина подставная, годная лишь этому же
	 *       потоку. Оттого он и удваивается: сторожевому потоку нужен настоящий
	 */
	HANDLE worker = nullptr;
	// Выполняем удвоение описателя текущего потока
	ASSERT_TRUE(::DuplicateHandle(::GetCurrentProcess(), ::GetCurrentThread(), ::GetCurrentProcess(), &worker, 0, FALSE, DUPLICATE_SAME_ACCESS) != 0);
#else
	struct sigaction action, previous;
	::memset(&action, 0, sizeof(action));
	action.sa_handler = &__awh_wake__;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	ASSERT_EQ(0, ::sigaction(SIGUSR1, &action, &previous));
	// Запоминаем поток, в котором крутится цикл событий
	const pthread_t worker = ::pthread_self();
#endif
	/**
	 * Сторож проверки: если оборот опроса не вернётся, срок в условии цикла не
	 * сработает, ибо условие проверяется МЕЖДУ оборотами. Разбудить поток
	 * можно лишь извне, сигналом
	 */
	std::thread guard([&finished, worker]() noexcept -> void {
		// Срок, отведённый обмену
		const auto limit = std::chrono::steady_clock::now() + std::chrono::seconds(10);
		// Ожидаем завершения обмена или истечения срока
		while(!finished.load() && (std::chrono::steady_clock::now() < limit))
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
		// Если обмен успел завершиться, вмешиваться не в чем
		if(finished.load())
			return;
		/**
		 * Обмен не уложился в срок, но причин тому две, и путать их нельзя:
		 * цикл мог ВСТАТЬ внутри приёма, а мог исправно крутиться, просто не
		 * получая данных. Различаются они живостью счётчика оборотов: у
		 * вставшего цикла он замирает
		 */
		const uint64_t before = __awh_rounds__.load();
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		// Если счётчик сдвинулся, цикл жив и будить его незачем
		if(__awh_rounds__.load() != before)
			return;
		/**
		 * Счётчик замер - цикл встал. Разбудить его можно лишь извне: срок в
		 * условии цикла проверяется МЕЖДУ оборотами, а оборот не кончается
		 */
		__awh_stalled__.store(true);
#if defined(_WIN32) || defined(_WIN64)
		/**
		 * Снимаем синхронный обмен, вставший в потоке цикла
		 *
		 * @note Это родной ключ MS Windows и точный сверстник EINTR: обмен
		 *       возвращает управление, движок доводит оборот до конца, цикл выходит
		 *       по своему сроку. Сигналами вставший приём там не будится вовсе
		 */
		::CancelSynchronousIo(worker);
#else
		::pthread_kill(worker, SIGUSR1);
#endif
	});
	// Срок на весь обмен, проверяемый между оборотами опроса
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(12);
	// Крутим цикл событий до получения ответа сервера либо истечения срока
	while(!echoed.load() && (std::chrono::steady_clock::now() < deadline) && this->_io->poll(__AWH_TEST_POLL_SLICE__))
		// Отмечаем оборот опроса: по этому счётчику сторож судит о живости цикла
		__awh_rounds__.fetch_add(1);
	// Отмечаем завершение обмена и дожидаемся сторожа
	finished.store(true);
	guard.join();
	// Возвращаем прежний обработчик сигнала
#if defined(_WIN32) || defined(_WIN64)
	// Закрываем удвоенный описатель потока
	if(worker != nullptr)
		// Выполняем закрытие описателя
		::CloseHandle(worker);
#else
	::sigaction(SIGUSR1, &previous, nullptr);
#endif
	/**
	 * Утверждение о невставшем цикле идёт ПЕРВЫМ: вставший цикл объясняет и
	 * недоставку данных, и отсутствие ответа, а потому доложить о нём надо
	 * прежде следствий
	 */
	EXPECT_FALSE(__awh_stalled__.load()) << "оборот опроса не вернулся: приём по блокирующему сокету ушёл в неотменяемое ожидание";
	// Выполняем проверку доставки данных серверу и возврата их клиенту
	EXPECT_TRUE(delivered.load()) << "данные не дошли до сервера по блокирующему сокету";
	EXPECT_TRUE(echoed.load()) << "ответ сервера не вернулся клиенту по блокирующему сокету";
}
