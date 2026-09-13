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
 *        обмена сообщениями между процессами, прямой связи работников между собой,
 *        пересылки через мастера и обработки событий падения и перезапуска воркеров
 *
 * @details Устройство прямой связи работников и служебного канала разобрано в
 *          `src/unit/CLUSTER-LINK.md`
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <unit/cluster.hpp>
#include <sys/log.hpp>
#include <sys/fmk.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * @brief Главная функция приложения
 *
 * @return код выхода из приложения
 *
 */
int32_t main(){
	/**
	 * Выполняем заведение модуля ядра первым делом
	 *
	 * @note Заведение захватывает выдачу памяти процесса и обязано идти
	 *       ДО всякой выдачи и ДО порождения потоков
	 */
	awh::fmk::initialize();
	// Создаём объект для работы с логами
	// Создаём объект кластера
	unit::cluster_t cluster;
	// Устанавливаем количество дочерних процессов в кластере
	cluster.count(4);
	// Устанавливаем функцию обратного вызова на изменение статуса кластера
	cluster.on <void (const event::status_t)> ("status", [](const event::status_t status) noexcept -> void {
		// Возвращаем статус работы кластера
		awh::log::print("Cluster status: %s", awh::log::flag_t::INFO, (status == event::status_t::LAUNCHED) ? "launched" : "destroyed");
		/**
		// Если статус работы кластера - запущен
		if(status == event::status_t::LAUNCHED)
			// Выполняем остановку кластера
			cluster.stop();
		 */
	}, placeholders::_1);
	// Устанавливаем функцию обратного вызова на события работы кластера
	cluster.on <void (const pid_t, const unit::cluster_t::event_t)> ("events", [&cluster](const pid_t pid, const unit::cluster_t::event_t event) noexcept -> void {
		// Возвращаем событие работы кластера
		awh::log::print("Cluster event: %s (pid: %u)", awh::log::flag_t::INFO, (event == unit::cluster_t::event_t::START) ? "started" : "stopped", pid);
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
	cluster.on <void (const pid_t, const pid_t)> ("rebase", [](const pid_t old_pid, const pid_t new_pid) noexcept -> void {
		// Возвращаем событие перезапуска процесса
		awh::log::print("Cluster process [%u] has been reborn as process [%u]", awh::log::flag_t::INFO, old_pid, new_pid);
	}, placeholders::_1, placeholders::_2);
	// Устанавливаем функцию обратного вызова на событие получения ошибок
	cluster.on <void (const pid_t, const event::error_t, const string &)> ("error", [](const pid_t pid, const event::error_t error, const string & message) noexcept -> void {
		// Возвращаем событие получения ошибки
		awh::log::print("Cluster process [%u] has received error [%d]: %s", awh::log::flag_t::CRITICAL, pid, static_cast <uint16_t >(error), message.c_str());
	}, placeholders::_1, placeholders::_2, placeholders::_3);
	// Устанавливаем функцию обратного вызова на событие отправки сообщений
	cluster.on <void (const pid_t, const size_t)> ("sending", [](const pid_t pid, const size_t size) noexcept -> void {
		// Возвращаем событие записи сообщения
		awh::log::print("Cluster process [%u] has sent message: %zu bytes, from PID=%u,", awh::log::flag_t::INFO, ::getpid(), size, pid);
	}, placeholders::_1, placeholders::_2);
	// Устанавливаем функцию обратного вызова на событие получения сообщений
	cluster.on <void (const pid_t, const uint8_t *, const size_t)> ("message", [](const pid_t pid, const uint8_t * data, const size_t size) noexcept -> void {
		// Текст входящего сообщения
		const string message(reinterpret_cast <const char *> (data), size);
		// Возвращаем событие получения сообщения
		awh::log::print("Cluster process [%u] has received message: %zu bytes, from PID=%u, message: %s", awh::log::flag_t::INFO, ::getpid(), size, pid, message.c_str());
		/**
		// Если процесс является мастер-процессом
		if(cluster.master())
			// Удаляем процесс приславший сообщение из кластера
			cluster.erase(pid, unit::cluster_t::shutdown_t::FORCEFUL);
			// cluster.erase(pid, unit::cluster_t::shutdown_t::GRACEFUL);
		 */
	}, placeholders::_1, placeholders::_2, placeholders::_3);
	// Устанавливаем функцию обратного вызова на событие доступности очереди сообщений
	cluster.on <void (const pid_t, const event::status_t, const size_t)> ("available", [](const pid_t pid, const event::status_t status, const size_t size) noexcept -> void {
		// Возвращаем событие доступности очереди сообщений
		awh::log::print("Cluster process [%u] has message queue availability: %zu bytes, status: %s", awh::log::flag_t::INFO, pid, size, (status == event::status_t::QUEUE_OVERFLOW) ? "overflow" : "available");
	}, placeholders::_1, placeholders::_2, placeholders::_3);
	// Устанавливаем функцию обратного вызова на событие изменения статуса процесса
	cluster.on <void (const pid_t, const event::status_t)> ("state", [](const pid_t pid, const event::status_t status) noexcept -> void {
		// Возвращаем событие изменения статуса
		awh::log::print("Cluster process [%u] state: %d", awh::log::flag_t::INFO, pid, static_cast <uint16_t> (status));
	}, placeholders::_1, placeholders::_2);
	// Устанавливаем функцию обратного вызова на событие завершения процесса
	cluster.on <void (const pid_t, const int32_t)> ("exit", [](const pid_t pid, const int32_t status) noexcept -> void {
		/**
		 * Состояние завершения приходит в том виде, в каком его отдаёт система, и к
		 * общему виду между системами не приводится: у POSIX это упакованное состояние
		 * ожидания, у MS Windows - код завершения. Переносимые вопросы задаются
		 * разборными методами кластера, платформу при этом разбирать не требуется
		 */
		if(unit::cluster_t::crashed(status))
			// Возвращаем событие падения процесса
			awh::log::print("Cluster process [%u] has crashed, signal: %d", awh::log::flag_t::CRITICAL, pid, unit::cluster_t::termsig(status));
		// Если процесс был снят с клавиатуры
		else if(unit::cluster_t::manual(status))
			// Возвращаем событие ручной остановки процесса
			awh::log::print("Cluster process [%u] has been interrupted", awh::log::flag_t::WARNING, pid);
		// Если процесс завершился сам
		else if(unit::cluster_t::exited(status))
			// Возвращаем событие завершения процесса
			awh::log::print("Cluster process [%u] has exited with code: %d", awh::log::flag_t::INFO, pid, unit::cluster_t::exitcode(status));
		// Если процесс был остановлен мастером
		else awh::log::print("Cluster process [%u] has been stopped by master", awh::log::flag_t::INFO, pid);
	}, placeholders::_1, placeholders::_2);
	/**
	 * Устанавливаем функцию обратного вызова на вход узла в кластер
	 *
	 * @details Своего способа узнать соседей у работника нет вовсе: он знает свой номер
	 *          и номер мастера. Извещения эти - единственный путь к связи, и приходят
	 *          они лишь о тех узлах, которые ПОДНЯЛИСЬ и дошли до цикла событий
	 */
	cluster.on <void (const pid_t)> ("join", [&cluster](const pid_t pid) noexcept -> void {
		/**
		 * Возвращаем событие входа узла в кластер
		 *
		 * @note Список узлов спрашивается по-разному у двух ролей: мастер ведёт список
		 *       своих работников, а работник - список соседей, о которых его известили.
		 *       У мастера `nodes` пустует всегда: список соседей заводится извещениями,
		 *       а извещает как раз он сам
		 */
		awh::log::print("Cluster node [%u] has joined (known to [%u]: %zu)", awh::log::flag_t::INFO, pid, ::getpid(),
		 (cluster.master() ? cluster.workers().size() : cluster.nodes().size()));
		/**
		 * Заказываем прямую связь с соседом
		 *
		 * @note Заказ ведёт МЛАДШИЙ по номеру процесса: узнают друг о друге оба, и
		 *       закажи связь оба - второй заказ получил бы отказ EXISTS. Правило это
		 *       произвольно, но однозначно, и оттого годится обеим сторонам без уговора
		 */
		if(!cluster.master() && (static_cast <pid_t> (::getpid()) < pid))
			// Заказываем прямую связь с соседом
			cluster.link(pid);
	}, placeholders::_1);
	// Устанавливаем функцию обратного вызова на выбытие узла из кластера
	cluster.on <void (const pid_t)> ("leave", [](const pid_t pid) noexcept -> void {
		// Возвращаем событие выбытия узла из кластера
		awh::log::print("Cluster node [%u] has left", awh::log::flag_t::INFO, pid);
	}, placeholders::_1);
	/**
	 * Устанавливаем функцию обратного вызова на запрос разрешения связи
	 *
	 * @details Сводит работников мастер, и право отказать принадлежит ему: связь
	 *          заводится лишь тогда, когда отклик этот отвечает согласием. Не установлен
	 *          отклик - разрешены все связи
	 */
	cluster.on <bool (const pid_t, const pid_t)> ("linking", [](const pid_t initiator, const pid_t peer) noexcept -> bool {
		// Возвращаем событие запроса разрешения связи
		awh::log::print("Cluster link [%u] <-> [%u] is permitted", awh::log::flag_t::INFO, initiator, peer);
		// Разрешаем заведение связи
		return true;
	}, placeholders::_1, placeholders::_2);
	// Устанавливаем функцию обратного вызова на заведение прямой связи
	cluster.on <void (const pid_t, const event::id_t)> ("linked", [&cluster](const pid_t pid, const event::id_t eid) noexcept -> void {
		// Возвращаем событие заведения прямой связи
		awh::log::print("Cluster process [%u] is linked to [%u], event: %llu", awh::log::flag_t::INFO, ::getpid(), pid, static_cast <uint64_t> (eid));
		/**
		 * Отправляем посылку соседу НАПРЯМУЮ
		 *
		 * @note Мастер в этом обмене не участвует вовсе: пара заведена им, но концы её
		 *       принадлежат работникам, и байты идут между ними
		 */
		if(static_cast <pid_t> (::getpid()) < pid){
			// Текст посылки по прямой связи
			const string message = "Hello from a neighbour worker!";
			// Отправляем посылку соседу напрямую
			cluster.transmit(pid, message.c_str(), message.length());
		}
	}, placeholders::_1, placeholders::_2);
	// Устанавливаем функцию обратного вызова на разрыв прямой связи
	cluster.on <void (const pid_t, const unit::cluster_t::reason_t)> ("unlinked", [](const pid_t pid, const unit::cluster_t::reason_t reason) noexcept -> void {
		// Возвращаем событие разрыва прямой связи
		awh::log::print("Cluster process [%u] is unlinked from [%u], reason: %u", awh::log::flag_t::WARNING, ::getpid(), pid, static_cast <uint16_t> (reason));
	}, placeholders::_1, placeholders::_2);
	// Устанавливаем функцию обратного вызова на получение сообщения по прямой связи
	cluster.on <void (const pid_t, const uint8_t *, const size_t)> ("peer", [&cluster](const pid_t pid, const uint8_t * data, const size_t size) noexcept -> void {
		// Текст входящего сообщения
		const string message(reinterpret_cast <const char *> (data), size);
		// Возвращаем событие получения сообщения по прямой связи
		awh::log::print("Cluster process [%u] has received a direct message from [%u]: %s", awh::log::flag_t::INFO, ::getpid(), pid, message.c_str());
		/**
		 * Пересылаем соседу подтверждение ЧЕРЕЗ МАСТЕРА
		 *
		 * @details Пересылка связи не требует и заводится ради коротких сообщений,
		 *          какие случаются изредка: заводить ради одного такого пару дороже
		 *          самого сообщения. Платой идут две передачи вместо одной и предел
		 *          размера - maximumRelay
		 */
		if(static_cast <pid_t> (::getpid()) > pid){
			// Текст подтверждения, пересылаемого через мастера
			const string relay = "Acknowledged through the master";
			// Если подтверждение вмещается в предел пересылки
			if(relay.length() <= cluster.maximumRelay())
				// Пересылаем подтверждение соседу через мастера
				cluster.relay(pid, relay.c_str(), relay.length());
		}
	}, placeholders::_1, placeholders::_2, placeholders::_3);
	// Устанавливаем функцию обратного вызова на получение пересылки через мастера
	cluster.on <void (const pid_t, const uint8_t *, const size_t)> ("relay", [](const pid_t pid, const uint8_t * data, const size_t size) noexcept -> void {
		// Текст входящей пересылки
		const string message(reinterpret_cast <const char *> (data), size);
		// Возвращаем событие получения пересылки
		awh::log::print("Cluster process [%u] has received a relayed message from [%u]: %s", awh::log::flag_t::INFO, ::getpid(), pid, message.c_str());
	}, placeholders::_1, placeholders::_2, placeholders::_3);
	/**
	 * Устанавливаем функцию обратного вызова на приказ завершить работу
	 *
	 * @details Приказ этот - НЕ сигнал: работник вправе довести своё дело до конца и
	 *          уйти сам, вызовом leave. Установлен отклик - уход за работником; не
	 *          установлен - кластер уходит немедленно названным кодом
	 *
	 * @note Ушедший по приказу работник возрождению не подлежит, и счётчик быстрых
	 *       падений его уход не задевает: мастер отличает уход от падения
	 */
	cluster.on <void (const int32_t)> ("shutdown", [&cluster](const int32_t code) noexcept -> void {
		// Возвращаем событие получения приказа завершить работу
		awh::log::print("Cluster process [%u] has been ordered to terminate with code %d", awh::log::flag_t::WARNING, ::getpid(), code);
		// Уходим из кластера названным кодом, доведя своё дело до конца
		cluster.leave(code);
	}, placeholders::_1);
	/**
	 * Мастер вправе велеть работнику уйти, а работник - уйти по своей воле
	 *
	 * // Мастер велит работнику завершить работу условленным кодом
	 * cluster.shutdown(pid, 42);
	 *
	 * // Работник уходит сам, известив мастера
	 * cluster.leave(EXIT_SUCCESS);
	 */
	// Запускаем работу кластера
	cluster.start();
	// Возвращаем результат
	return EXIT_SUCCESS;
}
