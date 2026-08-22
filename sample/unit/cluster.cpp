/**
 * @file cluster.cpp
 * @date 2026-02-21
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * @brief Пример работы с модулем кластера — демонстрация запуска дочерних воркеров,
 *        обмена сообщениями между процессами и обработки событий падения и перезапуска воркеров
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <unit/cluster.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * @brief Главная функция приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 *
 */
int32_t main(int32_t argc, char * argv[]){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Создаём объект кластера
	unit::cluster_t cluster(&fmk, &log);
	// Устанавливаем количество дочерних процессов в кластере
	cluster.count(4);
	// Устанавливаем функцию обратного вызова на изменение статуса кластера
	cluster.on <void (const event::status_t)> ("status", [&cluster, &log](const event::status_t status) noexcept -> void {
		// Возвращаем статус работы кластера
		log.print("Cluster status: %s", log_t::flag_t::INFO, (status == event::status_t::LAUNCHED) ? "launched" : "destroyed");
		/**
		// Если статус работы кластера - запущен
		if(status == event::status_t::LAUNCHED)
			// Выполняем остановку кластера
			cluster.stop();
		 */
	}, placeholders::_1);
	// Устанавливаем функцию обратного вызова на события работы кластера
	cluster.on <void (const pid_t, const unit::cluster_t::event_t)> ("events", [&cluster, &log](const pid_t pid, const unit::cluster_t::event_t event) noexcept -> void {
		// Возвращаем событие работы кластера
		log.print("Cluster event: %s (pid: %u)", log_t::flag_t::INFO, (event == unit::cluster_t::event_t::START) ? "started" : "stopped", pid);
		// Если процесс является мастер-процессом
		if(cluster.master()){
			// Если событие - запуск процесса
			if(event == unit::cluster_t::event_t::START){
				// Создаём еще один процесс
				cluster.emplace();
				// Если процесс является мастер-процессом
				if(cluster.master()){
					// Текст сообщения для отправки
					const string message = "Hello from master process!";
					/**
					 * Переходим по всему списку дочерних процессов
					 */
					for(auto & pid : cluster.workers())
						// Отправляем сообщение всем дочерним процессам
						cluster.send(pid, message.c_str(), message.length());
				}
			}
		// Если процесс является дочерним
		} else {
			// Если событие - запуск процесса
			if(event == unit::cluster_t::event_t::START){
				// Текст сообщения для отправки
				const string message = "Hello from child process!";
				// Отправляем сообщение родительскому процессу
				cluster.send(message.c_str(), message.length());
			}
		}
	}, placeholders::_1, placeholders::_2);
	// Устанавливаем функцию обратного вызова на событие пересоздания процесса
	cluster.on <void (const pid_t, const pid_t)> ("rebase", [&cluster, &log](const pid_t old_pid, const pid_t new_pid) noexcept -> void {
		// Возвращаем событие перезапуска процесса
		log.print("Cluster process [%u] has been reborn as process [%u]", log_t::flag_t::INFO, old_pid, new_pid);
	}, placeholders::_1, placeholders::_2);
	// Устанавливаем функцию обратного вызова на событие получения ошибок
	cluster.on <void (const pid_t, const event::error_t, const string &)> ("error", [&cluster, &log](const pid_t pid, const event::error_t error, const string & message) noexcept -> void {
		// Возвращаем событие получения ошибки
		log.print("Cluster process [%u] has received error [%d]: %s", log_t::flag_t::CRITICAL, pid, static_cast <uint16_t >(error), message.c_str());
	}, placeholders::_1, placeholders::_2, placeholders::_3);
	// Устанавливаем функцию обратного вызова на событие отправки сообщений
	cluster.on <void (const pid_t, const size_t)> ("sending", [&cluster, &log](const pid_t pid, const size_t size) noexcept -> void {
		// Возвращаем событие записи сообщения
		log.print("Cluster process [%u] has sent message: %zu bytes, from PID=%u,", log_t::flag_t::INFO, ::getpid(), size, pid);
	}, placeholders::_1, placeholders::_2);
	// Устанавливаем функцию обратного вызова на событие получения сообщений
	cluster.on <void (const pid_t, const uint8_t *, const size_t)> ("message", [&cluster, &log](const pid_t pid, const uint8_t * data, const size_t size) noexcept -> void {
		// Текст входящего сообщения
		const string message(reinterpret_cast <const char *> (data), size);
		// Возвращаем событие получения сообщения
		log.print("Cluster process [%u] has received message: %zu bytes, from PID=%u, message: %s", log_t::flag_t::INFO, ::getpid(), size, pid, message.c_str());
		/**
		// Если процесс является мастер-процессом
		if(cluster.master())
			// Удаляем процесс приславший сообщение из кластера
			cluster.erase(pid, unit::cluster_t::shutdown_t::FORCEFUL);
			// cluster.erase(pid, unit::cluster_t::shutdown_t::GRACEFUL);
		 */
	}, placeholders::_1, placeholders::_2, placeholders::_3);
	// Устанавливаем функцию обратного вызова на событие доступности очереди сообщений
	cluster.on <void (const pid_t, const event::status_t, const size_t)> ("available", [&cluster, &log](const pid_t pid, const event::status_t status, const size_t size) noexcept -> void {
		// Возвращаем событие доступности очереди сообщений
		log.print("Cluster process [%u] has message queue availability: %zu bytes, status: %s", log_t::flag_t::INFO, pid, size, (status == event::status_t::QUEUE_OVERFLOW) ? "overflow" : "available");
	}, placeholders::_1, placeholders::_2, placeholders::_3);
	// Устанавливаем функцию обратного вызова на событие изменения статуса процесса
	cluster.on <void (const pid_t, const event::status_t)> ("state", [&cluster, &log](const pid_t pid, const event::status_t status) noexcept -> void {
		// Возвращаем событие изменения статуса
		log.print("Cluster process [%u] state: %d", log_t::flag_t::INFO, pid, static_cast <uint16_t> (status));
	}, placeholders::_1, placeholders::_2);
	// Устанавливаем функцию обратного вызова на событие завершения процесса
	cluster.on <void (const pid_t, const int32_t)> ("exit", [&cluster, &log](const pid_t pid, const int32_t status) noexcept -> void {
		/**
		 * Состояние завершения приходит в том виде, в каком его отдаёт система, и к
		 * общему виду между системами не приводится: у POSIX это упакованное состояние
		 * ожидания, у MS Windows - код завершения. Переносимые вопросы задаются
		 * разборными методами кластера, платформу при этом разбирать не требуется
		 */
		if(unit::cluster_t::crashed(status))
			// Возвращаем событие падения процесса
			log.print("Cluster process [%u] has crashed, signal: %d", log_t::flag_t::CRITICAL, pid, unit::cluster_t::termsig(status));
		// Если процесс был снят с клавиатуры
		else if(unit::cluster_t::manual(status))
			// Возвращаем событие ручной остановки процесса
			log.print("Cluster process [%u] has been interrupted", log_t::flag_t::WARNING, pid);
		// Если процесс завершился сам
		else if(unit::cluster_t::exited(status))
			// Возвращаем событие завершения процесса
			log.print("Cluster process [%u] has exited with code: %d", log_t::flag_t::INFO, pid, unit::cluster_t::exitcode(status));
		// Если процесс был остановлен мастером
		else log.print("Cluster process [%u] has been stopped by master", log_t::flag_t::INFO, pid);
	}, placeholders::_1, placeholders::_2);
	// Запускаем работу кластера
	cluster.start();
	// Возвращаем результат
	return EXIT_SUCCESS;
}
