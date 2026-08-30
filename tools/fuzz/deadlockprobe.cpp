/**
 * @file: deadlockprobe.cpp
 * @brief Щуп: спасает ли срок подключения блокирующий узел от самоблокировки
 *
 * @details Воспроизводится замкнутый круг: клиент и сервер живут в ОДНОМ движке,
 *          очередь принятия у сервера переполнена, принять её может только оборот
 *          опроса, а оборот стоит внутри блокирующего `::connect`
 *
 * @note Вопрос щупа один: назначенный `setTimeout()` с действием `CONNECT` этот круг
 *       размыкает или нет. Срок этот отсчитывается таймером ВНУТРИ цикла опроса, и
 *       если оборота нет, то и отсчитывать его некому - но утверждать это без замера
 *       нельзя, чтением сегодня уже трижды ошибались
 *
 * @copyright Copyright © 2026
 *
 */

#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <cstdint>
#include <unistd.h>
#include <sys/time.h>

#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <net/io.hpp>

/**
 * @brief Обработчик сторожевого срока
 *
 * @param signal номер полученного сигнала
 *
 */
static void watchdog(int signal) noexcept {
	// Отмечаем полученный сигнал использованным
	static_cast <void> (signal);
	// Сообщаем о находке напрямую
	static const char message[] = "ВСТАЛ: цикл опроса не вернул управление\n";
	// Выполняем запись сообщения в поток ошибок
	static_cast <void> (::write(STDERR_FILENO, message, sizeof(message) - 1));
	// Выходим с кодом ошибки
	::_exit(EXIT_FAILURE);
}
/**
 * @brief Функция взведения сторожевого срока
 *
 * @param ms срок в миллисекундах, нуль снимает сторож
 *
 */
static void guard(const uint32_t ms) noexcept {
	// Объект настройки таймера
	struct itimerval timer;
	// Обнуляем повтор
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 0;
	// Устанавливаем целые секунды срока
	timer.it_value.tv_sec = static_cast <time_t> (ms / 1000);
	// Устанавливаем остаток срока в микросекундах
	timer.it_value.tv_usec = static_cast <suseconds_t> ((ms % 1000) * 1000);
	// Выполняем взведение таймера
	static_cast <void> (::setitimer(ITIMER_REAL, &timer, nullptr));
}
/**
 * @brief Функция запуска приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 *
 */
int main(int argc, char * argv[]) noexcept {
	// Вид назначаемого срока: 0 - без срока, 1 - действием CONNECT, 2 - действием WRITE
	const int32_t deadline = ((argc > 1) ? ::atoi(argv[1]) : 0);
	// Объект фреймворка
	static awh::fmk_t fmk;
	// Объект работы с логами
	static awh::log_t log(&fmk);
	/**
	 * Журнал НЕ глушим намеренно
	 *
	 * @note Заглушенный журнал дважды оставлял отказ без причины: движок её называет,
	 *       а щуп её прятал
	 */
	log.level(awh::log_t::level_t::ALL);
	// Объект сетевого движка
	awh::engine::io_t io(&fmk, &log);
	// Выполняем заведение сетевого движка
	if(!io.initialize()){
		// Сообщаем об отказе заведения движка
		::fprintf(stderr, "Движок завести не удалось\n");
		// Выходим с кодом ошибки
		return EXIT_FAILURE;
	}
	/**
	 * Порт берётся от номера процесса
	 *
	 * @note Постоянный порт занимался сокетами предыдущих прогонов в состоянии
	 *       `TIME_WAIT`, и фиксация сервера отвергалась с «Address already in use» -
	 *       отказ этот к самоблокировке отношения не имеет вовсе
	 */
	const uint16_t port = static_cast <uint16_t> (20000 + (::getpid() % 30000));
	// Взводим сторожевой срок на ВЕСЬ прогон: встать он может где угодно, не только в подключении
	::guard(20000);
	// Заводим узел сервера потоковой передачи
	const awh::event::id_t srv = io.event(
		awh::event::node_t::SERVER, awh::event::family_t::IPV4,
		awh::event::type_t::STREAM, awh::event::protocol_t::TCP
	);
	// Назначаем серверу адрес привязки
	static_cast <void> (io.setAddress(srv, awh::event::address_t::IPV4, "127.0.0.1"));
	// Назначаем серверу порт привязки
	static_cast <void> (io.setSourcePort(srv, port));
	// Ограничиваем очередь принятия одним подключением
	static_cast <void> (io.setMaxConnections(srv, 1));
	// Если фиксация настроек сервера отвергнута
	if(!io.commit(srv)){
		// Сообщаем об отказе фиксации сервера
		::fprintf(stderr, "Фиксация сервера ОТВЕРГНУТА\n");
		// Выходим с кодом ошибки
		return EXIT_FAILURE;
	}
	// Выполняем запуск сервера
	static_cast <void> (io.launch(srv));
	// Сообщаем о поднятом сервере
	::fprintf(stderr, "Сервер поднят на 127.0.0.1:%u, очередь 1, принимать не будем\n", port);
	/**
	 * Заводим клиентов, пока один из них не упрётся в переполненную очередь
	 *
	 * @note Обороты опроса НЕ делаются намеренно: принять подключения некому, и
	 *       очередь обязана переполниться
	 */
	for(uint16_t i = 1; i <= 64; i++){
		// Заводим узел клиента потоковой передачи
		const awh::event::id_t cli = io.event(
			awh::event::node_t::CLIENT, awh::event::family_t::IPV4,
			awh::event::type_t::STREAM, awh::event::protocol_t::TCP
		);
		// Назначаем клиенту адрес назначения
		static_cast <void> (io.setTarget(cli, "127.0.0.1"));
		// Назначаем клиенту порт назначения
		static_cast <void> (io.setTargetPort(cli, port));
		/**
		 * Назначаем узлу набор опций ЦЕЛИКОМ, а не поштучно
		 *
		 * @warning Поштучное `setOption(NO_IO_BLOCK, false)` блокирующим узел НЕ делает:
		 *          это «не ставить опцию», а движок неблокирующий режим назначает сам.
		 *          Блокирующим узел выходит лишь при присвоении всего набора числом,
		 *          затирающего умолчание, - именно так его и получил ворошитель. Число
		 *          взято из пойманного случая дословно (зерно 5029)
		 */
		static_cast <void> (io.setOptions(cli, 41424));
		/**
		 * Если срок требуется назначить действием подключения
		 *
		 * @note Довод 1: `CONNECT` разветвления по режиму НЕ имеет и лишь запоминает
		 *       задержку, отсчитываемую таймером цикла
		 */
		if(deadline == 1)
			// Назначаем узлу срок установления подключения
			io.setTimeout(cli, awh::event::action_t::CONNECT, 2000);
		/**
		 * Если срок требуется назначить действием записи
		 *
		 * @note Довод 2: `WRITE` у блокирующего узла кладёт `SO_SNDTIMEO` прямо на
		 *       сокет, а он ограничивает и подключение
		 */
		else if(deadline == 2)
			// Назначаем узлу срок записи
			io.setTimeout(cli, awh::event::action_t::WRITE, 2000);
		// Фиксируем настройки клиента
		static_cast <void> (io.commit(cli));
		// Взводим сторожевой срок на подключение
		::guard(8000);
		// Выполняем подключение клиента
		const bool started = io.connect(cli);
		// Снимаем сторожевой срок
		::guard(0);
		// Сообщаем об итоге подключения
		::fprintf(stderr, "клиент %u: подключение %s\n", i, (started ? "начато" : "ОТКАЗ"));
		// Сбрасываем поток ошибок
		::fflush(stderr);
	}
	// Сообщаем, что круг замкнуть не удалось
	::fprintf(stderr, "64 подключения прошли, цикл не встал\n");
	// Уничтожаем все узлы движка
	static_cast <void> (io.deinitialize());
	// Выходим с успешным кодом
	return EXIT_SUCCESS;
}
