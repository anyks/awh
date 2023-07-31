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
	// Устанавливаем название сервиса
	log.name("DNS");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");
	// Флаг получения данных
	bool success = false;
	// Выполняем резолвинг для доменного имени
	core.resolve("google.com", scheme_t::family_t::IPV4, [&log, &success](const string & ip, const scheme_t::family_t family, core_t * core){
		// Запоминаем, что результат получен
		success = true;
		// Выводим результат получения IP адреса
		log.print("IP: %s", log_t::flag_t::INFO, ip.c_str());
		// Завершаем работу
		core->stop();
	});
	// Если результат ещё не получен
	if(!success)
		// Выполняем запуск таймера
		core.start();
	// Выводим результат
	return 0;
}
