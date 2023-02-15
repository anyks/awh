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

#include <net/net.hpp>

// Подключаем пространство имён
using namespace std;
using namespace awh;

/**
 * Timer Класс объекта исполнителя
 */
class Timer {
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
		 * @param id   идентификатор таймера
		 * @param core объект сетевого ядра
		 */
		void timeout(const u_short id, core_t * core){
			// Выводим информацию в лог
			this->_log->print("Timeout: %u seconds", log_t::flag_t::INFO, chrono::duration_cast <chrono::seconds> (chrono::system_clock::now() - this->ts).count());
		}
		/**
		 * run Метод запуска сетевого ядра
		 * @param mode флаг запуска сетевого ядра
		 * @param core объект сетевого ядра
		 */
		void run(const bool mode, Core * core){
			// Если система запущена
			if(mode){
				// Замеряем время начала работы для таймера
				this->ts = chrono::system_clock::now();
				// Замеряем время начала работы для интервала времени
				this->is = chrono::system_clock::now();
				// Выводим информацию в лог
				this->_log->print("%s", log_t::flag_t::INFO, "Start timer");
				// Устанавливаем задержку времени на 10 секунд
				core->setTimeout(10000, (function <void (const u_short, core_t *)>) bind(&Timer::timeout, this, _1, _2));
				// Устанавливаем интервал времени времени на 5 секунд
				core->setInterval(5000, (function <void (const u_short, core_t *)>) bind(&Timer::interval, this, _1, _2));
			// Выводим информацию в лог
			} else this->_log->print("%s", log_t::flag_t::INFO, "Stop timer");
		}
	public:
		/**
		 * Timer Конструктор
		 * @param log объект логирования
		 */
		Timer(log_t * log) : ts(chrono::system_clock::now()), is(chrono::system_clock::now()), count(0), _log(log) {}
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

	net_t net(&fmk, &log);

	bool status = net.parse("192.168.0.1");

	cout << " ################ STATUS " << status << " == " << (u_short) net.type() << " || " << net.get(net_t::format_t::SHORT) << " || " << net.v4() << endl;

	status = net.parse("2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d");

	cout << " ################ STATUS " << status << " == " << (u_short) net.type() << " || " << net.get(net_t::format_t::SHORT) << " || " << net.v6()[0] << " == " << net.v6()[1] << endl;

	status = net.parse("2001:0db8:0000:0000:0000:0000:ae21:ad12");

	cout << " ################ STATUS " << status << " == " << (u_short) net.type() << " || " << net.get(net_t::format_t::SHORT) << " || " << net.v6()[0] << " == " << net.v6()[1] << endl;

	status = net.parse("2001:db8::ae21:ad12");

	cout << " ################ STATUS " << status << " == " << (u_short) net.type() << " || " << net.get(net_t::format_t::SHORT) << " || " << net.v6()[0] << " == " << net.v6()[1] << endl;

	status = net.parse("0000:0000:0000:0000:0000:0000:ae21:ad12");

	cout << " ################ STATUS " << status << " == " << (u_short) net.type() << " || " << net.get(net_t::format_t::SHORT) << " || " << net.v6()[0] << " == " << net.v6()[1] << endl;

	status = net.parse("::ae21:ad12");

	cout << " ################ STATUS " << status << " == " << (u_short) net.type() << " || " << net.get(net_t::format_t::SHORT) << " || " << net.v6()[0] << " == " << net.v6()[1] << endl;

	status = net.parse("2001:0db8:11a3:09d7:1f34::");

	cout << " ################ STATUS " << status << " == " << (u_short) net.type() << " || " << net.get(net_t::format_t::SHORT) << " || " << net.v6()[0] << " == " << net.v6()[1] << endl;

	status = net.parse("::ffff:192.0.2.1");

	cout << " ################ STATUS " << status << " == " << (u_short) net.type() << " || " << net.get(net_t::format_t::SHORT) << " || " << net.v4() << endl;

	status = net.parse("[2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d]");

	cout << " ################ STATUS " << status << " == " << (u_short) net.type() << " || " << net.get(net_t::format_t::SHORT) << " || " << net.v6()[0] << " == " << net.v6()[1] << endl;

	status = net.parse("::0");

	cout << " ################ STATUS " << status << " == " << (u_short) net.type() << " || " << net.get(net_t::format_t::SHORT) << " || " << net.v6()[0] << " == " << net.v6()[1] << endl;

	status = net.parse("2001:4860:4860::8844");

	cout << " ################ STATUS " << status << " == " << (u_short) net.type() << " || " << net.get(net_t::format_t::SHORT) << " || " << net.v6()[0] << " == " << net.v6()[1] << endl;

	net = "::ae21:ad12";

	cout << " ++++++++++++++++++++1 " << net.get(net_t::format_t::LONG) << endl;

	cout << " ====================1 " << net << endl;

	net = "46.39.230.51";

	cout << " ++++++++++++++++++++2 " << net.get(net_t::format_t::LONG) << endl;

	cout << " ====================2 " << net << endl;

	string k = net;

	cout << " ====================3 " << k << endl;

	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";

	net.impose(53);

	cout << " ++++++++++++++++++3 " << net << endl;

	net = "192.168.3.192";

	net.impose(9);

	cout << " ++++++++++++++++++4 " << net << endl;

	net = "192.168.3.192";

	net.impose("255.255.255.0");

	cout << " ++++++++++++++++++5 " << net << endl;

	cout << " =================== PREFIX1 " << net.prefix2Mask(24) << endl;

	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";

	cout << " =================== PREFIX2 " << net.prefix2Mask(53) << endl;

	net.impose("FFFF:FFFF:FFFF:F800::");

	cout << " ++++++++++++++++++6 " << net << endl;

	net = "192.168.3.192";

	net.dempose(9);

	cout << " ++++++++++++++++++7 " << net << endl;

	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";

	net.dempose(53);

	cout << " ++++++++++++++++++8 " << net << endl;


	/*
	// Создаём объект исполнителя
	Timer executor(&log);
	// Создаём биндинг
	core_t core(&fmk, &log);
	// Устанавливаем название сервиса
	log.name("Timer");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");
	// Устанавливаем функцию обратного вызова на запуск системы
	core.callback((function <void (const bool, core_t *)>) bind(&Timer::run, &executor, _1, _2));
	// Выполняем запуск таймера
	core.start();
	*/
	// Выводим результат
	return 0;
}
