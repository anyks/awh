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
#include <cluster/core.hpp>

// Подключаем пространство имён
using namespace std;
using namespace awh;

/**
 * Executor Класс объекта исполнителя
 */
class Executor {
	private:
		// Объект логирования
		log_t * _log;
	private:
		// Сетевое ядро
		cluster::core_t * _core;
	public:
		/**
		 * events Метод вывода события активации процесса
		 * @param worker тип активного процесса
		 * @param pid    идентификатор процесса
		 * @param event  событие процесса
		 */
		void events(const cluster::core_t::worker_t worker, const pid_t pid, const cluster_t::event_t event){
			// Если производится запуск сервиса
			if(event == cluster_t::event_t::START){
				// Определяем тип воркера
				switch(static_cast <uint8_t> (worker)){
					// Если событие пришло от родительского процесса
					case static_cast <uint8_t> (cluster::core_t::worker_t::MASTER): {
						// Формируем сообщение приветствия
						const char * message = "Hi!";
						// Отправляем проиветствие всем дочерним процессам
						this->_core->broadcast(message, strlen(message));
					} break;
					// Если событие пришло от дочернего процесса
					case static_cast <uint8_t> (cluster::core_t::worker_t::CHILDREN): {
						// Формируем сообщение приветствия
						const char * message = "Hello";
						// Отправляем проиветствие родительскому процессу
						this->_core->send(message, strlen(message));
					} break;
				}
			}
		}
		/**
		 * message Метод получения сообщения
		 * @param worker тип активного процесса
		 * @param pid    идентификатор процесса
		 * @param buffer буфер данных сообщения
		 * @param size   размер полученных данных
		 */
		void message(const cluster::core_t::worker_t worker, const pid_t pid, const char * buffer, const size_t size){
			// Определяем тип воркера
			switch(static_cast <uint8_t> (worker)){
				// Если событие пришло от родительского процесса
				case static_cast <uint8_t> (cluster::core_t::worker_t::MASTER):
					// Выводим полученное сообщение в лог
					this->_log->print("Message from children [%u]: %s", log_t::flag_t::INFO, pid, string(buffer, size).c_str());
				break;
				// Если событие пришло от дочернего процесса
				case static_cast <uint8_t> (cluster::core_t::worker_t::CHILDREN):
					// Выводим полученное сообщение в лог
					this->_log->print("Message from master: %s [%u]", log_t::flag_t::INFO, string(buffer, size).c_str(), getpid());
				break;
			}
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
					// Выполняем установку сетевого ядра
					this->_core = dynamic_cast <cluster::core_t *> (core);
					// Выводим информацию в лог
					this->_log->print("%s", log_t::flag_t::INFO, "Start cluster");
				} break;
				// Если система остановлена
				case static_cast <uint8_t> (awh::core_t::status_t::STOP):
					// Выводим информацию в лог
					this->_log->print("%s", log_t::flag_t::INFO, "Stop cluster");
				break;
			}
		}
	public:
		/**
		 * Executor Конструктор
		 * @param log объект логирования
		 */
		Executor(log_t * log) : _log(log), _core(nullptr) {}
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
	cluster::core_t core(&fmk, &log);
	// Устанавливаем название сервиса
	log.name("Cluster");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");
	// Выделяем для кластера все доступные ядра
	core.clusterSize();
	// Разрешаем выполнять автоматический перезапуск упавшего процесса
	core.clusterAutoRestart(true);
	// Устанавливаем функцию обратного вызова на запуск системы
	core.callback((function <void (const awh::core_t::status_t, core_t *)>) bind(&Executor::run, &executor, _1, _2));
	// Устанавливаем функцию обратного вызова при получении событий
	core.on((function <void (const cluster::core_t::worker_t, const pid_t, const cluster_t::event_t)>) bind(&Executor::events, &executor, _1, _2, _3));
	// Устанавливаем функцию обработки входящих сообщений
	core.on((function <void (const cluster::core_t::worker_t, const pid_t, const char *, const size_t)>) bind(&Executor::message, &executor, _1, _2, _3, _4));
	// Выполняем запуск таймера
	core.start();
	// Выводим результат
	return 0;
}
