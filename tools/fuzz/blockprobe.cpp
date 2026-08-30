/**
 * @file: blockprobe.cpp
 * @brief Щуп: встаёт ли цикл опроса на подключении узла, заведённого по умолчанию
 *
 * @details Договор `connect()` обещает немедленный возврат и предусматривает срок
 *          установления через `setTimeout()` с действием `CONNECT`. Щуп проверяет,
 *          ДОСТИЖИМ ли этот срок: применяет его движок, получив управление, а
 *          блокирующий `::connect` управления не возвращает
 *
 * @note Щуп доказывает ОБЫЧНОЕ положение, а не редкую гонку: опция зовётся
 *       `NO_IO_BLOCK`, то есть узел без опций блокирующий, и совпадения условий
 *       тут никакого не требуется
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
	// Сообщаем о находке напрямую: печать через stdio в обработчике недопустима
	static const char message[] = "НАХОДКА: опрос не вернул управление - цикл встал\n";
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
	// Обнуляем повтор: сторож одноразовый
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
	// Признак снятия блокировки у узла
	const bool unblock = ((argc > 1) && (::atoi(argv[1]) != 0));
	// Объект фреймворка
	static awh::fmk_t fmk;
	// Объект работы с логами
	static awh::log_t log(&fmk);
	// Снимаем вывод журнала
	log.level(awh::log_t::level_t::NONE);
	// Объект сетевого движка
	awh::engine::io_t io(&fmk, &log);
	// Выполняем заведение сетевого движка
	if(!io.initialize()){
		// Сообщаем, что движок завести не удалось
		::fprintf(stderr, "Сетевой движок завести не удалось\n");
		// Выходим с кодом ошибки
		return EXIT_FAILURE;
	}
	// Заводим клиентский узел потоковой передачи
	const awh::event::id_t id = io.event(
		awh::event::node_t::CLIENT,
		awh::event::family_t::IPV4,
		awh::event::type_t::STREAM,
		awh::event::protocol_t::TCP
	);
	// Если узел завести не удалось
	if(id == 0){
		// Сообщаем, что узел завести не удалось
		::fprintf(stderr, "Узел завести не удалось\n");
		// Выходим с кодом ошибки
		return EXIT_FAILURE;
	}
	// Если блокировку у узла требуется снять
	if(unblock)
		// Снимаем блокировку ввода-вывода у узла
		static_cast <void> (io.setOption(id, awh::event::options::NO_IO_BLOCK, true));
	/**
	 * Адрес заведомо недостижимый: он назначен документом RFC 5737 под примеры и
	 * в сети не маршрутизируется, оттого подключение к нему не завершится никогда
	 */
	static_cast <void> (io.setTarget(id, "192.0.2.1"));
	// Назначаем порт удалённой стороны
	static_cast <void> (io.setTargetPort(id, 80));
	// Итог фиксации настроек узла
	const bool committed = io.commit(id);
	// Сообщаем об итоге фиксации
	::fprintf(stderr, "Фиксация: %s\n", (committed ? "принята" : "ОТВЕРГНУТА"));
	// Сообщаем о начале опыта
	::fprintf(stderr, "Узел заведён%s, подключаемся\n", (unblock ? " и переведён в неблокирующий режим" : " с настройками по умолчанию"));
	// Сбрасываем поток ошибок: буфер теряет вывод при выходе из обработчика
	::fflush(stderr);
	// Взводим сторожевой срок на подключение и опрос
	::guard(5000);
	// Итог подключения узла
	const bool connected = io.connect(id);
	// Снимаем сторожевой срок на время печати
	::guard(0);
	// Сообщаем об итоге подключения
	::fprintf(stderr, "Подключение: %s\n", (connected ? "начато" : "ОТКАЗ"));
	// Сбрасываем поток ошибок
	::fflush(stderr);
	// Взводим сторожевой срок заново на обороты опроса
	::guard(5000);
	// Выполняем обороты опроса
	for(uint8_t i = 0; i < 8; i++)
		// Выполняем оборот опроса
		static_cast <void> (io.poll(100));
	// Снимаем сторожевой срок
	::guard(0);
	// Сообщаем, что управление вернулось
	::fprintf(stderr, "Управление вернулось: цикл не встал\n");
	// Уничтожаем все узлы движка
	static_cast <void> (io.deinitialize());
	// Выходим с успешным кодом
	return EXIT_SUCCESS;
}
