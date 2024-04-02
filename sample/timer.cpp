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
#include <core/timer.hpp>

// Подключаем пространство имён
using namespace std;
using namespace awh;

/**
 * Executor Класс объекта исполнителя
 */
class Executor {
	private:
		// Замеряем время начала работы для таймера
		chrono::time_point <chrono::system_clock> _ts;
		// Замеряем время начала работы для интервала времени
		chrono::time_point <chrono::system_clock> _is;
	private:
		// Идентификаторы таймеров
		u_short _count;
	private:
		// Объект логирования
		log_t * _log;
	public:
		/**
		 * interval Метод интервала
		 * @param tid   идентификатор таймера
		 * @param timer объект таймера
		 */
		void interval(const u_short tid, awh::timer_t * timer){
			// Замеряем время начала работы для интервала времени
			auto shift = chrono::system_clock::now();
			// Выводим информацию в лог
			this->_log->print("Interval: %u seconds", log_t::flag_t::INFO, chrono::duration_cast <chrono::seconds> (shift - this->_is).count());
			// Замеряем время начала работы для интервала времени
			this->_is = shift;
			// Если таймер отработал 10 раз, выходим
			if((this->_count++) >= 10){
				// Останавливаем работу таймера
				timer->clear(tid);
				// Останавливаем работу модуля
				timer->stop();
			}
		}
		/**
		 * timeout Метод таймаута
		 * @param id идентификатор таймера
		 */
		void timeout(const u_short id){
			// Выводим информацию в лог
			this->_log->print("Timeout: %u seconds", log_t::flag_t::INFO, chrono::duration_cast <chrono::seconds> (chrono::system_clock::now() - this->_ts).count());
		}
		/**
		 * status Метод запуска сетевого ядра
		 * @param status флаг запуска сетевого ядра
		 * @param timer  объект таймера
		 */
		void status(const awh::core_t::status_t status, awh::timer_t * timer){
			// Определяем статус активности сетевого ядра
			switch(static_cast <uint8_t> (status)){
				// Если система запущена
				case static_cast <uint8_t> (awh::core_t::status_t::START): {
					// Замеряем время начала работы для таймера
					this->_ts = chrono::system_clock::now();
					// Замеряем время начала работы для интервала времени
					this->_is = this->_ts;
					// Выводим информацию в лог
					this->_log->print("%s", log_t::flag_t::INFO, "Start timer");
					// Устанавливаем задержку времени на 10 секунд
					uint16_t tid = timer->timeout(10000);
					// Выполняем добавление функции обратного вызова
					timer->set <void (const uint16_t)> (tid, std::bind(&Executor::timeout, this, tid));
					// Устанавливаем задержку времени на 5 секунд
					tid = timer->interval(5000);
					// Устанавливаем интервал времени времени на 5 секунд
					timer->set <void (const uint16_t, awh::timer_t *)> (tid, std::bind(&Executor::interval, this, tid, timer));
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
		Executor(log_t * log) : _ts(chrono::system_clock::now()), _is(chrono::system_clock::now()), _count(0), _log(log) {}
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
	
	os_t os;

	cout << " ----1 kern.ipc.maxsockbuf= " << os.sysctl <uint32_t> ("kern.ipc.maxsockbuf") << endl;

	os.sysctl("kern.ipc.maxsockbuf", 6291456);

	cout << " ----2 kern.ipc.maxsockbuf= " << os.sysctl <uint32_t> ("kern.ipc.maxsockbuf") << endl;

	os.boost();
	
	return 0;
	
	// Создаём объект исполнителя
	Executor executor(&log);
	// Создаём объект таймера
	awh::timer_t timer(&fmk, &log);
	// Устанавливаем название сервиса
	log.name("Timer");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");
	// Устанавливаем функцию обратного вызова на запуск системы
	timer.callback <void (const awh::core_t::status_t, core_t *)> ("status", std::bind(&Executor::status, &executor, _1, &timer));
	// Выполняем запуск таймера
	timer.start();
	// Выводим результат
	return 0;
}
