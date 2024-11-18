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
#include <sys/investigator.hpp>
#include <sys/fmk.hpp>
#include <sys/log.hpp>

// Подключаем пространство имён
using namespace std;
using namespace awh;

/**
 * main Главная функция приложения
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 */
int32_t main(int32_t argc, char * argv[]){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект дознавателя
	igtr_t igtr;
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Устанавливаем название сервиса
	log.name("Investigator");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");
	// Если количество параметров больше одного
	if(argc > 1){
		// Выполняем получение пида
		const pid_t pid = static_cast <pid_t> (::stoi(argv[1]));
		// Выводим в лог название приложения
		log.print("Investigator: NAME=%s", log_t::flag_t::INFO, igtr.inquiry(pid).c_str());
	// Выводим в лог название текущего проекта
	} else log.print("Investigator: NAME=%s", log_t::flag_t::INFO, igtr.inquiry().c_str());
	// Выводим результат
	return EXIT_SUCCESS;
}
