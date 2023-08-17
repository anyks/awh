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
#include <net/ntp.hpp>
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
int main(int argc, char * argv[]){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Создаём биндинг
	ntp_t ntp(&fmk, &log);
	// Создаём объект сетевого ядра
	core_t core(&fmk, &log);
	// Устанавливаем название сервиса
	log.name("NTP");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");
	// Выполняем установку серверов имён
	ntp.ns({"77.88.8.88", "77.88.8.2"});
	// Выполняем установку списка сервером времени
	ntp.servers({"0.ru.pool.ntp.org", "1.ru.pool.ntp.org", "2.ru.pool.ntp.org", "3.ru.pool.ntp.org"});
	// Выполняем получение текущего UnitxTimeStamp
	const time_t timestamp = (ntp.request() / 1000);
	// Выполняем запрос на получение первого времени
	log.print("Time: %s", log_t::flag_t::INFO, ctime(&timestamp));
	// Выводим результат
	return 0;
}
