/**
 * @file: cluster.cpp
 * @date: 2025-03-02
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include <core/cluster.hpp>

/**
 * Подписываемся на пространство имён AWH
 */
using namespace awh;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * Executor Класс объекта исполнителя
 */
class Executor {
	private:
		// Объект логирования
		log_t * _log;
	public:
		/**
		 * ready Метод готовности дочерних процессов к работе
		 * @param pid  идентификатор процесса
		 * @param core объект сетевого ядра
		 */
		void ready(const pid_t pid, cluster::core_t * core){
			// Формируем сообщение приветствия
			const string message = "Hi!";
			// Выполняем отправку текста
			core->send(pid, message.data(), message.size());
		}
		/**
		 * events Метод вывода события активации процесса
		 * @param worker тип активного процесса
		 * @param pid    идентификатор процесса
		 * @param event  событие процесса
		 * @param core   объект сетевого ядра
		 */
		void events(const cluster_t::family_t worker, [[maybe_unused]] const pid_t pid, const cluster_t::event_t event, cluster::core_t * core){
			// Если производится запуск сервиса
			if(event == cluster_t::event_t::START){
				// Определяем тип воркера
				switch(static_cast <uint8_t> (worker)){
					// Если событие пришло от родительского процесса
					case static_cast <uint8_t> (cluster_t::family_t::MASTER):
						// Выполняем создание нового процесса
						core->emplace();
					break;
					// Если событие пришло от дочернего процесса
					case static_cast <uint8_t> (cluster_t::family_t::CHILDREN): {
						// Формируем сообщение приветствия
						const string message = "Hello";
						// Отправляем проиветствие родительскому процессу
						core->send(message.data(), message.size());
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
		void message(const cluster_t::family_t worker, const pid_t pid, const char * buffer, const size_t size, cluster::core_t * core){
			// Определяем тип воркера
			switch(static_cast <uint8_t> (worker)){
				// Если событие пришло от родительского процесса
				case static_cast <uint8_t> (cluster_t::family_t::MASTER): {
					// Формируем новое ответное сообщение
					const string message = "ANYKS!!!";
					// Отправляем проиветствие родительскому процессу
					core->send(pid, message.data(), message.size());
					// Выводим полученное сообщение в лог
					this->_log->print("Message from children [%u]: %s", log_t::flag_t::INFO, pid, string(buffer, size).c_str());
				} break;
				// Если событие пришло от дочернего процесса
				case static_cast <uint8_t> (cluster_t::family_t::CHILDREN):
					// Выводим полученное сообщение в лог
					this->_log->print("Message from master: %s [%u]", log_t::flag_t::INFO, string(buffer, size).c_str(), ::getpid());
				break;
			}
		}
		/**
		 * status Метод запуска сетевого ядра
		 * @param status флаг запуска сетевого ядра
		 */
		void status(const awh::core_t::status_t status){
			// Определяем статус активности сетевого ядра
			switch(static_cast <uint8_t> (status)){
				// Если система запущена
				case static_cast <uint8_t> (awh::core_t::status_t::START):
					// Выводим информацию в лог
					this->_log->print("%s", log_t::flag_t::INFO, "Start cluster");
				break;
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
		Executor(log_t * log) : _log(log) {}
};

/**
 * main Главная функция приложения
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 */
int32_t main(int32_t argc, char * argv[]){
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
	core.size();
	// Устанавливаем название кластера
	core.name("ANYKS");
	// Разрешаем выполнять автоматический перезапуск упавшего процесса
	core.autoRestart(true);
	// Переключаем режим передачи данных
	// core.transfer(cluster_t::transfer_t::IPC);
	// Устанавливаем пароль шифрования
	core.password("Password");
	// Устанавливаем размера шифрования
	core.cipher(hash_t::cipher_t::AES256);
	// Устанавливаем компрессора сообщений
	core.compressor(hash_t::method_t::ZSTD);
	// Устанавливаем функцию обратного вызова готовность дочерних процессов
	core.on <void (const pid_t)> ("ready", &Executor::ready, &executor, _1, &core);
	// Устанавливаем функцию обратного вызова на запуск системы
	core.on <void (const awh::core_t::status_t)> ("status", &Executor::status, &executor, _1);
	// Устанавливаем функцию обратного вызова при получении событий
	core.on <void (const cluster_t::family_t, const pid_t, const cluster_t::event_t)> ("events", &Executor::events, &executor, _1, _2, _3, &core);
	// Устанавливаем функцию обработки входящих сообщений
	core.on <void (const cluster_t::family_t, const pid_t, const char *, const size_t)> ("message", &Executor::message, &executor, _1, _2, _3, _4, &core);
	// Выполняем запуск таймера
	core.start();
	// Выводим результат
	return EXIT_SUCCESS;
}
