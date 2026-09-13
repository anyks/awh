/**
 * @file timer.cpp
 * @date 2026-02-22
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
 * @brief Пример работы с модулем таймера — демонстрация создания одиночных и периодических таймеров цикла событий,
 *        их перезапуска и остановки
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные модули
 */
#include <chrono>

/**
 * Подключаем заголовочный файл проекта
 */
#include <unit/timer.hpp>
#include <sys/log.hpp>
#include <sys/fmk.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * Используем пространство имён placeholders
 */
using namespace placeholders;

/**
 * @brief Главная функция приложения
 *
 * @return код выхода из приложения
 *
 */
int32_t main(){
	/**
	 * Выполняем заведение модуля ядра первым делом
	 *
	 * @note Заведение захватывает выдачу памяти процесса и обязано идти
	 *       ДО всякой выдачи и ДО порождения потоков
	 */
	awh::fmk::initialize();
	// Создаём объект узла таймера
	unit::timer_t timer;
	// Добавляем новое событие таймера
	event::id_t eid1 = timer.timeout(12000);
	// Добавляем новое событие интервала
	event::id_t eid2 = timer.interval(5000);
	// Замеряем время начала работы для таймера
	chrono::time_point <chrono::system_clock> ts = chrono::system_clock::now();
	// Замеряем время начала работы для интервала времени
	chrono::time_point <chrono::system_clock> is = chrono::system_clock::now();
	// Устанавливаем функцию обратного вызова на событие таймера
	timer.on <void (const event::id_t)> (eid1, [&ts](const event::id_t eid) noexcept -> void {
		// Замеряем время начала работы для интервала времени
		auto shift = chrono::system_clock::now();
		// Записываем в лог сообщение о срабатывании таймера
		awh::log::print("Таймер сработал: ID=%u, %u seconds", awh::log::flag_t::INFO, eid, chrono::duration_cast <chrono::seconds> (shift - ts).count());
	}, placeholders::_1);
	// Количество срабатываний интервала
	uint8_t count = 0;
	// Устанавливаем функцию обратного вызова на событие интервала
	timer.on <void (const event::id_t)> (eid2, [&count, &is, &timer](const event::id_t eid) noexcept -> void {
		// Замеряем время начала работы для интервала времени
		auto shift = chrono::system_clock::now();
		// Записываем в лог сообщение о срабатывании интервала
		awh::log::print("Интервал сработал: ID=%u, %u seconds", awh::log::flag_t::INFO, eid, chrono::duration_cast <chrono::seconds> (shift - is).count());
		// Замеряем время начала работы для интервала времени
		is = ::move(shift);
		// Если таймер отработал 10 раз, выходим
		if((count++) >= 10)
			// Завершаем работу таймера
			timer.stop();
	}, placeholders::_1);
	// Запускаем таймер
	timer.start();
	// Возвращаем результат
	return EXIT_SUCCESS;
}
