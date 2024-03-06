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
 * Executor Класс объекта исполнителя
 */
class Executor {
	private:
		// Замеряем время начала работы для таймера
		chrono::time_point <chrono::system_clock> ts;
		// Замеряем время начала работы для интервала времени
		chrono::time_point <chrono::system_clock> is;
	private:
		// Идентификаторы таймеров
		u_short count;
	private:
		// Объект логирования
		log_t * _log;
	public:
		/**
		 * interval Метод интервала
		 * @param id   идентификатор таймера
		 * @param core объект сетевого ядра
		 */
		void interval(const u_short id, core_t * core){
			// Замеряем время начала работы для интервала времени
			auto shift = chrono::system_clock::now();
			// Выводим информацию в лог
			this->_log->print("Interval: %u seconds", log_t::flag_t::INFO, chrono::duration_cast <chrono::seconds> (shift - this->is).count());
			// Замеряем время начала работы для интервала времени
			this->is = shift;
			// Если таймер отработал 10 раз, выходим
			if((this->count++) >= 10){
				// Останавливаем работу таймера
				core->clearTimer(id);
				// Останавливаем работу модуля
				core->stop();
			}
		}
		/**
		 * timeout Метод таймаута
		 * @param id идентификатор таймера
		 */
		void timeout(const u_short id){
			// Выводим информацию в лог
			this->_log->print("Timeout: %u seconds", log_t::flag_t::INFO, chrono::duration_cast <chrono::seconds> (chrono::system_clock::now() - this->ts).count());
		}
		/**
		 * run Метод запуска сетевого ядра
		 * @param status флаг запуска сетевого ядра
		 * @param core   объект сетевого ядра
		 */
		void run(const awh::core_t::status_t status, core_t * core){
			// Определяем статус активности сетевого ядра
			switch(static_cast <uint8_t> (status)){
				// Если система запущена
				case static_cast <uint8_t> (awh::core_t::status_t::START): {
					// Замеряем время начала работы для таймера
					this->ts = chrono::system_clock::now();
					// Замеряем время начала работы для интервала времени
					this->is = this->ts;
					// Выводим информацию в лог
					this->_log->print("%s", log_t::flag_t::INFO, "Start timer");
					// Устанавливаем задержку времени на 10 секунд
					core->setTimeout(10000, std::bind(&Executor::timeout, this, _1));
					// Устанавливаем интервал времени времени на 5 секунд
					core->setInterval(5000, std::bind(&Executor::interval, this, _1, core));
				} break;
				// Если система остановлена
				case static_cast <uint8_t> (awh::core_t::status_t::STOP):
					// Выводим информацию в лог
					this->_log->print("%s", log_t::flag_t::INFO, "Stop timer");
				break;
			}
		}
	public:
		/**
		 * Executor Конструктор
		 * @param log объект логирования
		 */
		Executor(log_t * log) : ts(chrono::system_clock::now()), is(chrono::system_clock::now()), count(0), _log(log) {}
};

/**
 * main Главная функция приложения
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 */
int main(int argc, char * argv[]){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Создаём объект исполнителя
	Executor executor(&log);
	// Создаём биндинг
	core_t core(&fmk, &log);
	// Устанавливаем название сервиса
	log.name("Timer");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");
	// Устанавливаем функцию обратного вызова на запуск системы
	core.callback <void (const awh::core_t::status_t, core_t *)> ("status", std::bind(&Executor::run, &executor, _1, &core));
	// Выполняем запуск таймера
	core.start();
	// Выводим результат
	return 0;
}
