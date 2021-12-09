/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <chrono>
#include <core/core.hpp>

// Подключаем пространство имён
using namespace std;
using namespace awh;

/**
 * main Главная функция приложения
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 */
int main(int argc, char * argv[]) noexcept {
	// Создаём объект фреймворка
	fmk_t fmk(true);
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Создаём биндинг
	core_t core(&fmk, &log);
	// Устанавливаем название сервиса
	log.setLogName("Timer");
	// Устанавливаем формат времени
	log.setLogFormat("%H:%M:%S %d.%m.%Y");
	// Устанавливаем функцию обратного вызова на запуск системы
	core.setCallback(&log, [](const bool mode, core_t * core, void * ctx) noexcept {
		// Получаем объект логирования
		log_t * log = reinterpret_cast <log_t *> (ctx);
		// Если система запущена
		if(mode){
			// Идентификаторы таймеров
			u_short id1 = 0, id2 = 0, count = 0;
			// Замеряем время начала работы для таймера
			auto timeShift = chrono::system_clock::now();
			// Замеряем время начала работы для интервала времени
			auto intervalShift = chrono::system_clock::now();
			// Выводим информацию в лог
			log->print("%s", log_t::flag_t::INFO, "Start timer");
			// Устанавливаем задержку времени на 10 секунд
			id1 = core->setTimeout(log, 10000, [&timeShift](void * ctx) noexcept {
				// Получаем объект логирования
				log_t * log = reinterpret_cast <log_t *> (ctx);
				// Выводим информацию в лог
				log->print("Timeout: %u seconds", log_t::flag_t::INFO, chrono::duration_cast <chrono::seconds> (chrono::system_clock::now() - timeShift).count());
			});
			// Устанавливаем интервал времени времени на 5 секунд
			id2 = core->setInterval(log, 5000, [&id2, &count, &intervalShift, core](void * ctx) noexcept {
				// Получаем объект логирования
				log_t * log = reinterpret_cast <log_t *> (ctx);
				// Выводим информацию в лог
				log->print("Interval: %u seconds", log_t::flag_t::INFO, chrono::duration_cast <chrono::seconds> (chrono::system_clock::now() - intervalShift).count());
				// Замеряем время начала работы для интервала времени
				intervalShift = chrono::system_clock::now();
				// Если таймер отработал 10 раз, выходим
				if((count++) >= 10){
					// Останавливаем работу таймера
					core->clearTimer(id2);
					// Останавливаем работу модуля
					core->stop();
				}
			});
		// Выводим информацию в лог
		} else log->print("%s", log_t::flag_t::INFO, "Stop timer");
	});
	// Выполняем запуск таймера
	core.start();
	// Выводим результат
	return 0;
}
