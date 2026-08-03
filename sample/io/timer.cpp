/**
 * @file: timer.cpp
 * @date: 2025-11-18
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Пример работы с таймерами движка ввода-вывода — демонстрация создания одиночных и периодических таймеров,
 *        их перезапуска и остановки внутри цикла событий
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Стандартные модули
 */
#include <chrono>
#include <iostream>
#include <cinttypes>

/**
 * Подключаем заголовочный файл проекта
 */
#include <net/io.hpp>

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
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 *
 */
int32_t main(int32_t argc, char * argv[]){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект логирования
	log_t log(&fmk);
	// Создаём объект асинхронного движка ввода-вывода
	engine::io_t io(&fmk, &log);
	// Устанавливаем тип таймера как простой
	io.setInternalTimer(event::timer_t::SIMPLE);
	// Устанавливаем тип таймера как сложный
	// io.setInternalTimer(event::timer_t::DIFFICULT);
	// Добавляем новое событие таймера
	event::id_t eid1 = io.event(event::node_t::TIMEOUT, event::family_t::TIMER);
	// Добавляем новое событие интервала
	event::id_t eid2 = io.event(event::node_t::INTERVAL, event::family_t::TIMER);
	// Устанавливаем таймаут таймера
	io.setTimeout(eid1, event::action_t::NONE, 12000);
	// Устанавливаем таймаут интервала
	io.setTimeout(eid2, event::action_t::NONE, 5000);
	// Инициализируем асинхронный движок ввода-вывода
	if(io.initialize()){
		// Выполняем фиксацию настроек события таймера
		io.commit(eid1);
		// Выполняем фиксацию настроек события интервала
		io.commit(eid2);
		// Замеряем время начала работы для таймера
		chrono::time_point <chrono::system_clock> ts = chrono::system_clock::now();
		// Замеряем время начала работы для интервала времени
		chrono::time_point <chrono::system_clock> is = chrono::system_clock::now();
		// Устанавливаем функцию обратного вызова на событие таймера
		io.on(eid1, [&ts, &log](const event::id_t eid, const event::status_t status) noexcept -> void {
			// Замеряем время начала работы для интервала времени
			auto shift = chrono::system_clock::now();
			// Если статус события успешен
			if(status == event::status_t::SUCCESS)
				// Записываем в лог сообщение о срабатывании таймера
				log.print("Таймер сработал: ID=%u, %u seconds", log_t::flag_t::INFO, eid, chrono::duration_cast <chrono::seconds> (shift - ts).count());
		});
		// Количество срабатываний интервала
		uint8_t count = 0;
		// Устанавливаем функцию обратного вызова на событие интервала
		io.on(eid2, [&count, &is, &log](const event::id_t eid, const event::status_t status) noexcept -> void {
			// Замеряем время начала работы для интервала времени
			auto shift = chrono::system_clock::now();
			// Если статус события успешен
			if(status == event::status_t::SUCCESS){
				// Записываем в лог сообщение о срабатывании интервала
				log.print("Интервал сработал: ID=%u, %u seconds", log_t::flag_t::INFO, eid, chrono::duration_cast <chrono::seconds> (shift - is).count());
				// Замеряем время начала работы для интервала времени
				is = ::move(shift);
				// Если таймер отработал 10 раз, выходим
				if((count++) >= 10)
					// Завершаем работу приложения
					::exit(EXIT_SUCCESS);
			}
		});
		// Выполняем запуск события
		if(io.launch(eid1) && io.launch(eid2)){
			// Записываем в лог сообщение об успешном запуске события
			cout << " Событие успешно запущено!" << endl;
			/**
			 * Запускаем опрос событий
			 */
			while(io.poll());
		// Записываем ошибку в лог запуска события
		} else cout << " Ошибка запуска события!" << endl;
	}
	// Возвращаем результат
	return EXIT_SUCCESS;
}
