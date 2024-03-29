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
#include <net/ping.hpp>
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
	core_t core(&fmk, &log);
	// Создаём объект PING запросов
	ping_t ping(&fmk, &log);
	// Устанавливаем название сервиса
	log.name("PING");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");
	// Выполняем инициализацию подключения
	const double result = ping.ping("api.telegram.org", 10);
	// Выводим результат пинга
	log.print("PING result=%f", log_t::flag_t::INFO, result);
	// Выводим результат
	return 0;
}
