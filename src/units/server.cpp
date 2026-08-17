/**
 * @file server.cpp
 * @date 2026-03-22
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
 * @brief Реализация модуля сервера — приём и обслуживание входящих подключений поверх движка ввода-вывода,
 *        управление жизненным циклом клиентов и работа в кластерном режиме
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include <units/server.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Используем пространство имён placeholders
 */
using namespace placeholders;

/**
 * @brief Конструктор
 *
 */
awh::unit::Server::ClusterParams::ClusterParams() noexcept :
 name{""}, rebirth(false), count(0),
 restartLimit(10), restartWindow(30000),
 mode(event::mode_t::DISABLED) {}

/**
 * @brief Метод удаления связи клиента с сервером
 *
 * @param cid идентификатор клиентского события
 *
 */
void awh::unit::Server::unlinkClient(const event::id_t cid) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск клиентского события
		auto i = this->_events.find(cid);
		// Если клиентское событие найдено
		if(i != this->_events.end()){
			// Извлекаем идентификатор серверного события
			const event::id_t sid = i->second;
			// Если событие является клиентским
			if(sid > 0){
				// Выполняем поиск списка клиентов сервера
				auto j = this->_serverClients.find(sid);
				// Выполняем поиск позиции клиента в списке
				auto k = this->_clientPositions.find(cid);
				// Если список сервера и позиция клиента найдены
				if((j != this->_serverClients.end()) && (k != this->_clientPositions.end())){
					// Удаляем клиента из списка сервера за O(1)
					j->second.erase(k->second);
					// Если список клиентов сервера пуст
					if(j->second.empty())
						// Удаляем список клиентов сервера
						this->_serverClients.erase(j);
				}
				// Удаляем позицию клиента в индексе
				if(k != this->_clientPositions.end())
					// Удаляем позицию клиента из индекса
					this->_clientPositions.erase(k);
			}
			// Удаляем клиентское событие из общего индекса
			this->_events.erase(i);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(cid), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод удаления всех клиентов серверного события
 *
 * @param sid идентификатор серверного события
 *
 */
void awh::unit::Server::unlinkServerClients(const event::id_t sid) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск списка клиентов сервера
		auto i = this->_serverClients.find(sid);
		// Если список найден
		if(i != this->_serverClients.end()){
			/**
			 * Удаляем только клиентов текущего сервера
			 */
			for(const event::id_t cid : i->second){
				// Удаляем клиент из общего индекса событий
				auto j = this->_events.find(cid);
				// Если клиент найден в общем индексе событий
				if(j != this->_events.end())
					// Удаляем клиентское событие из общего индекса событий
					this->_events.erase(j);
				// Удаляем позицию клиента
				auto k = this->_clientPositions.find(cid);
				// Если позиция клиента найдена
				if(k != this->_clientPositions.end())
					// Удаляем позицию клиента из индекса
					this->_clientPositions.erase(k);
				// Удаляем клиентское событие из IO-движка
				this->_io->destroy(cid);
			}
			// Удаляем список клиентов сервера
			this->_serverClients.erase(i);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sid), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод регистрации связи клиента с сервером
 *
 * @param sid идентификатор серверного события
 * @param cid идентификатор клиентского события
 *
 */
void awh::unit::Server::linkClient(const event::id_t sid, const event::id_t cid) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если клиент уже связан, удаляем предыдущую связь
		this->unlinkClient(cid);
		// Добавляем связь клиентского события с серверным
		this->_events.emplace(cid, sid);
		// Регистрируем клиента в списке клиентов сервера
		auto & clients = this->_serverClients[sid];
		// Добавляем клиента в список сервера
		clients.emplace_back(cid);
		// Получаем итератор на добавленного клиента
		auto i = clients.end();
		// Двигаем итератор на добавленного клиента
		--i;
		// Сохраняем позицию клиента для удаления за O(1)
		this->_clientPositions.emplace(cid, i);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sid, cid), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод запуска/остановки работы сервера
 *
 * @param status статус запуска/остановки сервера
 *
 */
void awh::unit::Server::launch(const event::status_t status) noexcept {
	/**
	 * Определяем статус работы сервера
	 */
	switch(static_cast <uint8_t> (status)){
		// Если работа кластера запущена
		case static_cast <uint8_t> (event::status_t::LAUNCHED): {
			/**
			 * Для операционной системы MS Windows
			 */
			#if _WIN32 || _WIN64
				/**
				 * Проверяем требуется ли активировать кластер
				 */
				switch(static_cast <uint8_t> (this->_clusterParams.mode)){
					// Если необходимо активировать кластер
					case static_cast <uint8_t> (event::mode_t::ENABLED): {
						// Если кластер не инициализирован
						if(this->_cluster == nullptr){
							// Создаём объект кластера для управления процессами сервера
							this->_cluster = make_unique <cluster_t> (this->_fmk, this->_log);
							// Если имя кластера установлено
							if(!this->_clusterParams.name.empty())
								// Устанавливаем название кластера
								this->_cluster->name(this->_clusterParams.name);
							// Устанавливаем максимальное количество процессов кластера
							this->_cluster->count(this->_clusterParams.count);
							// Устанавливаем флаг автоматического возрождения процессов
							this->_cluster->rebirth(this->_clusterParams.rebirth);
							// Устанавливаем параметры защиты от цикла перезапусков процессов
							this->_cluster->rebirthLimit(this->_clusterParams.restartLimit, this->_clusterParams.restartWindow);
						}
						// Устанавливаем функцию обратного вызова на событие получения сигнала
						this->_cluster->on <void (const pid_t, const int32_t)> ("exit", &server_t::exit, this, _1, _2);
						// Устанавливаем функцию обратного вызова на событие пересоздания процесса
						this->_cluster->on <void (const pid_t, const pid_t)> ("rebase", &server_t::rebase, this, _1, _2);
						// Устанавливаем функцию обратного вызова на событие отправки сообщений
						this->_cluster->on <void (const pid_t, const size_t)> ("sending", &server_t::sending, this, _1, _2);
						// Устанавливаем функцию обратного вызова на получение событий кластера
						this->_cluster->on <void (const pid_t, const unit::cluster_t::event_t)> ("events", &server_t::cluster, this, _1, _2);
						// Устанавливаем функцию обратного вызова на событие получения сообщений
						this->_cluster->on <void (const pid_t, const uint8_t *, const size_t)> ("message", &server_t::message, this, _1, _2, _3);
						// Устанавливаем функцию обратного вызова на событие изменения статуса кластера
						this->_cluster->on <void (const pid_t, const event::status_t)> ("state", static_cast <void (server_t::*)(const pid_t, const event::status_t)> (&server_t::status), this, _1, _2);
						// Устанавливаем функцию обратного вызова на событие получения ошибок кластера
						this->_cluster->on <void (const pid_t, const event::error_t, const string &)> ("error", static_cast <void (server_t::*)(const pid_t, const event::error_t, const string &)> (&server_t::error), this, _1, _2, _3);
						// Устанавливаем функцию обратного вызова на событие доступности/недоступности очереди исходящих данных кластера
						this->_cluster->on <void (const pid_t, const event::status_t, const size_t)> ("available", static_cast <void (server_t::*)(const pid_t, const event::status_t, const size_t)> (&server_t::available), this, _1, _2, _3);
						/**
						 * Мастер поднимает свои слушающие события ПРЕЖДЕ порождения работников
						 *
						 * @details Ветвления у этой системы нет, и слушающего сокета работник
						 *          не наследует. Принимает подключения мастер сам, а работнику
						 *          отдаёт снимком уже принятое подключение - оттого к мигу
						 *          порождения работников слушающий сокет обязан уже работать
						 *
						 * @note У работника отклик потребителя зовётся ПОСЛЕ того, как он
						 *       связался с мастером: своих слушающих событий он не заводит
						 *       вовсе, а заводит их отсрочкой - подключения приходят ему
						 *       готовыми
						 */
						if(!this->_cluster->worker())
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const event::status_t)> ("server_status", status);
						// Запускаем работу кластера
						this->_cluster->start();
					} break;
					// Если активировать кластер не требуется
					case static_cast <uint8_t> (event::mode_t::DISABLED):
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const event::status_t)> ("server_status", status);
					break;
				}
			/**
			 * Для систем, где ядро само разводит подключения между процессами кластера
			 */
			#elif __AWH_CLUSTER_BALANCE__
				// Если кластер в работе не используется или если процесс является дочерним
				if((this->_clusterParams.mode == event::mode_t::DISABLED) || !this->_cluster->master())
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const event::status_t)> ("server_status", status);
			/**
			 * Для операционной системы OpenBSD, NetBSD, Sun Solaris или macOS
			 */
			#elif __OpenBSD__ || __NetBSD__ || __sun__ || __APPLE__ || __MACH__
				/**
				 * Проверяем требуется ли активировать кластер
				 */
				switch(static_cast <uint8_t> (this->_clusterParams.mode)){
					// Если необходимо активировать кластер
					case static_cast <uint8_t> (event::mode_t::ENABLED): {
						// Если кластер не инициализирован
						if(this->_cluster == nullptr){
							// Создаём объект кластера для управления процессами сервера
							this->_cluster = make_unique <cluster_t> (this->_fmk, this->_log);
							// Если имя кластера установлено
							if(!this->_clusterParams.name.empty())
								// Устанавливаем название кластера
								this->_cluster->name(this->_clusterParams.name);
							// Устанавливаем максимальное количество процессов кластера
							this->_cluster->count(this->_clusterParams.count);
							// Устанавливаем флаг автоматического возрождения процессов
							this->_cluster->rebirth(this->_clusterParams.rebirth);
							// Устанавливаем параметры защиты от цикла перезапусков процессов
							this->_cluster->rebirthLimit(this->_clusterParams.restartLimit, this->_clusterParams.restartWindow);
						}
						// Устанавливаем функцию обратного вызова на событие получения сигнала
						this->_cluster->on <void (const pid_t, const int32_t)> ("exit", &server_t::exit, this, _1, _2);
						// Устанавливаем функцию обратного вызова на событие пересоздания процесса
						this->_cluster->on <void (const pid_t, const pid_t)> ("rebase", &server_t::rebase, this, _1, _2);
						// Устанавливаем функцию обратного вызова на событие отправки сообщений
						this->_cluster->on <void (const pid_t, const size_t)> ("sending", &server_t::sending, this, _1, _2);
						// Устанавливаем функцию обратного вызова на получение событий кластера
						this->_cluster->on <void (const pid_t, const unit::cluster_t::event_t)> ("events", &server_t::cluster, this, _1, _2);
						// Устанавливаем функцию обратного вызова на событие получения сообщений
						this->_cluster->on <void (const pid_t, const uint8_t *, const size_t)> ("message", &server_t::message, this, _1, _2, _3);
						// Устанавливаем функцию обратного вызова на событие изменения статуса кластера
						this->_cluster->on <void (const pid_t, const event::status_t)> ("state", static_cast <void (server_t::*)(const pid_t, const event::status_t)> (&server_t::status), this, _1, _2);
						// Устанавливаем функцию обратного вызова на событие получения ошибок кластера
						this->_cluster->on <void (const pid_t, const event::error_t, const string &)> ("error", static_cast <void (server_t::*)(const pid_t, const event::error_t, const string &)> (&server_t::error), this, _1, _2, _3);
						// Устанавливаем функцию обратного вызова на событие доступности/недоступности очереди исходящих данных кластера
						this->_cluster->on <void (const pid_t, const event::status_t, const size_t)> ("available", static_cast <void (server_t::*)(const pid_t, const event::status_t, const size_t)> (&server_t::available), this, _1, _2, _3);
						// Запускаем работу кластера
						this->_cluster->start();
					} break;
					// Если активировать кластер не требуется
					case static_cast <uint8_t> (event::mode_t::DISABLED):
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const event::status_t)> ("server_status", status);
					break;
				}
			#endif
		} break;
		// Если сервер завершает работу
		case static_cast <uint8_t> (event::status_t::DESTROYED): {
			// Если в списке событий сервера есть события
			if(!this->_events.empty()){
				/**
				 * Выполняем удаление всех событий сервера
				 */
				for(const auto & event : this->_events)
					// Удаляем событие сервера
					this->_io->destroy(event.first);
				// Очищаем внутренние индексы событий
				this->_events.clear();
				// Очищаем список клиентов сервера
				this->_serverClients.clear();
				// Очищаем позиции клиентов в индексе
				this->_clientPositions.clear();
			}
			// Выполняем получение идентификатора функции обратного вызова
			const callback_t::id_t fid = this->_callback.id("server_status");
			// Если функция обратного вызова установлена
			if(this->_callback.is(fid)){
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const event::status_t)> (fid, status);
				// Восстанавливаем callback status
				this->_callback.set(fid, this->_callback.id("status"), this->_callback);
			}
			/**
			 * Для операционной системы OpenBSD, NetBSD, Sun Solaris или macOS
			 */
			#if __OpenBSD__ || __NetBSD__ || __sun__ || __APPLE__ || __MACH__
				// Если необходимо деактивировать кластер
				if(this->_clusterParams.mode == event::mode_t::ENABLED){
					// Если кластер инициализирован
					if(this->_cluster != nullptr)
						// Останавливаем работу кластера
						this->_cluster->stop();
				}
			#endif
		} break;
	}
}
/**
 * @brief Метод обработки события пересоздания процесса
 *
 * @param old старый идентификатор процесса
 * @param pid текущий идентификатор процесса
 *
 */
void awh::unit::Server::rebase(const pid_t old, const pid_t pid) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const pid_t, const pid_t)> ("cluster_rebase", old, pid);
}
/**
 * @brief Метод получения события завершения работы процесса
 *
 * @param pid    идентификатор процесса
 * @param status состояние, с которым завершился процесс
 *
 */
void awh::unit::Server::exit(const pid_t pid, const int32_t status) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const pid_t, const int32_t)> ("cluster_exit", pid, status);
}
/**
 * @brief Метод обработки события отправки сообщения процессу кластера
 *
 * @param pid  идентификатор процесса
 * @param size размер отправленного сообщения
 *
 */
void awh::unit::Server::sending(const pid_t pid, const size_t size) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const pid_t, const size_t)> ("cluster_sending", pid, size);
}
/**
 * @brief Метод обработки событий записи данных сервером
 *
 * @param eid  идентификатор события
 * @param size размер данных для записи
 * @param ctx  промежуточный контекст для передачи в функцию обратного вызова
 *
 */
void awh::unit::Server::write(const event::id_t eid, const size_t size, void * ctx) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const event::id_t, const size_t, void *)> ("write", eid, size, ctx);
}
/**
 * @brief Метод обработки события разрешения подключения
 *
 * @param eid идентификатор сервера
 * @param cid идентификатор клиента
 *
 */
void awh::unit::Server::accept(const event::id_t eid, const event::id_t cid) noexcept {
	/**
	 * Методы только операционной системы MS Windows
	 *
	 * @note Мастер кластера подключений не обслуживает: приняв подключение, он отдаёт
	 *       его работнику снимком, а сам о нём забывает. Не досталось работника -
	 *       обслуживает сам, обычным путём ниже
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Если процесс является мастером кластера и подключение отдано работнику
		if((this->_cluster != nullptr) && !this->_cluster->worker() && this->handover(cid))
			// Прерываем выполнение: подключение обслуживает работник
			return;
	#endif
	// Устанавливаем функцию обратного вызова на событие изменения действий клиента
	this->_io->on(cid, static_cast <engine::callback::event_t> (std::bind(&server_t::action, this, _1, _2, nullptr)));
	// Устанавливаем функцию обратного вызова на событие записи данных клиенту
	this->_io->on(cid, static_cast <engine::callback::write_t> (std::bind(&server_t::write, this, _1, _2, nullptr)));
	// Устанавливаем функцию обратного вызова на событие чтения данных клиента
	this->_io->on(cid, static_cast <engine::callback::read_t> (std::bind(&server_t::read, this, _1, _2, _3, nullptr)));
	// Устанавливаем функцию обратного вызова на событие неотправленных данных клиенту
	this->_io->on(cid, static_cast <engine::callback::spool_t> (std::bind(&server_t::spool, this, _1, _2, _3, _4, nullptr)));
	// Устанавливаем функцию обратного вызова на событие истечения таймаута клиента
	this->_io->on(cid, static_cast <engine::callback::timeout_t> (std::bind(&server_t::timeout, this, _1, _2, _3, nullptr)));
	// Устанавливаем функцию обратного вызова на событие изменения статуса клиента
	this->_io->on(cid, static_cast <engine::callback::status_t> (std::bind(static_cast <void (server_t::*)(const event::id_t, const event::status_t, void *)> (&server_t::status), this, _1, _2, nullptr)));
	// Устанавливаем функцию обратного вызова на событие получения ошибок клиента
	this->_io->on(cid, static_cast <engine::callback::error_t> (std::bind(static_cast <void (server_t::*)(const event::id_t, const event::error_t, const string &, void *)> (&server_t::error), this, _1, _2, _3, nullptr)));
	// Устанавливаем функцию обратного вызова на событие доступности/недоступности очереди исходящих данных клиента
	this->_io->on(cid, static_cast <engine::callback::available_t> (std::bind(static_cast <void (server_t::*)(const event::id_t, const event::status_t, const size_t, void *)> (&server_t::available), this, _1, _2, _3, nullptr)));
	// Регистрируем связь клиентского события с сервером
	this->linkClient(eid, cid);
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const event::id_t, const event::id_t)> ("accept", eid, cid);
}
/**
 * @brief Метод обработки действий сервера
 *
 * @param eid    идентификатор события
 * @param action действие сервера
 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
 *
 */
void awh::unit::Server::action(const event::id_t eid, const event::action_t action, void * ctx) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const event::id_t, const event::action_t, void *)> ("action", eid, action, ctx);
}
/**
 * @brief Метод обработки событий изменения статуса кластера
 *
 * @param pid    идентификатор события
 * @param status новый статус кластера
 *
 */
void awh::unit::Server::status(const pid_t pid, const event::status_t status) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const pid_t, const event::status_t)> ("cluster_state", pid, status);
}
/**
 * @brief Метод обработки событий изменения статуса сервера
 *
 * @param eid    идентификатор события
 * @param status новый статус сервера
 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
 *
 */
void awh::unit::Server::status(const event::id_t eid, const event::status_t status, void * ctx) noexcept {
	// Если статус сервера представляет из себя уничтожение
	if(status == event::status_t::DESTROYED){
		// Выполняем поиск идентификатора события сервера в списке событий
		auto i = this->_events.find(eid);
		// Если идентификатор события сервера найден
		if(i != this->_events.end()){
			// Если идентификатор события является сервером
			if(i->second == 0){
				// Удаляем все клиентские события данного сервера
				this->unlinkServerClients(i->first);
				// Удаляем идентификатор события сервера
				this->_events.erase(i);
			// Если идентификатор относится к клиентскому событию, удаляем его
			} else this->unlinkClient(i->first);
		}
	}
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const event::id_t, const event::status_t, void *)> ("state", eid, status, ctx);
}
/**
 * @brief Метод обработки информационных метаданных о дейтаграммном пакете
 *
 * @param eid  идентификатор события
 * @param info информационные метаданные о дейтаграммном пакете
 *
 */
void awh::unit::Server::traffic(const event::id_t eid, const net::dgram_info_t & info) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const event::id_t, const net::dgram_info_t &)> ("traffic", eid, info);
}
/**
 * @brief Метод получения событий активации/деактивации кластера
 *
 * @param pid   идентификатор процесса
 * @param event флаг события кластера
 *
 */
void awh::unit::Server::cluster(const pid_t pid, const unit::cluster_t::event_t event) noexcept {
	/**
	 * Определяем полученное событие кластера
	 */
	switch(static_cast <uint8_t> (event)){
		// Если событие представляет из себя запуск процесса
		case static_cast <uint8_t> (unit::cluster_t::event_t::START): {
			/**
			 * Для операционной системы MS Windows
			 */
			#if _WIN32 || _WIN64
				/**
				 * Если процесс является родительским
				 *
				 * @note Событие запуска приходит мастеру ОДНО на весь кластер и несёт его
				 *       собственный номер процесса, а не номер работника: работники к
				 *       этому мигу порождены все, и передача идёт каждому из них
				 */
				if(this->_cluster->master()){
					/**
					 * Мастер здесь не делает ничего
					 *
					 * @note Передача слушающих событий идёт не отсюда: порождение работника
					 *       ещё не значит, что канал с ним связан, а отдать снимок в
					 *       несвязанный канал нельзя. Отдаётся он по просьбе самого
					 *       работника - она и служит признаком связанности
					 */
				// Если процесс является дочерним
				} else {
					/**
					 * Работник просит передачу событий сам
					 *
					 * @details Просьба эта и задаёт порядок: к мигу её отправки канал
					 *          заведомо связан - работник только что по нему написал, -
					 *          тогда как мастер о связанности канала узнать не может
					 *          вовсе. Порождение работника связи ещё не означает
					 *
					 * @warning Просьба уходит ПЕРВЫМ сообщением работника, а передача
					 *          приходит ПЕРВЫМ сообщением мастера, и оба конца этот
					 *          порядок знают. Потребитель к своим сообщениям приступает
					 *          лишь из отклика ниже - опередить их он не может
					 */
					const uint8_t request = server_t::HANDOVER_REQUEST;
					// Объявляем мастеру о своей готовности обслуживать подключения
					this->_cluster->send(&request, sizeof(request));
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const event::status_t)> ("server_status", event::status_t::LAUNCHED);
				}
			/**
			 * Для систем, где ядро само разводит подключения между процессами кластера
			 */
			#elif __AWH_CLUSTER_BALANCE__
				// Если работа юнита ещё не запущена
				if(!this->working()){
					// Выполняем получение идентификатора функции обратного вызова
					const callback_t::id_t fid = this->_callback.id("status");
					// Если функция обратного вызова установлена
					if(this->_callback.is(fid))
						// Переименовываем callback status в server_status
						this->_callback.set(fid, this->_callback.id("server_status"), this->_callback);
					// Устанавливаем функцию обратного вызова на запуск системы
					this->_callback.on <void (const event::status_t)> (fid, static_cast <void (server_t::*)(const event::status_t)> (&server_t::launch), this, _1);
					// Выполняем запуск работы основного юнита
					unit_t::start();
				}
			/**
			 * Для операционной системы OpenBSD, NetBSD, Sun Solaris или macOS
			 */
			#elif __OpenBSD__ || __NetBSD__ || __sun__ || __APPLE__ || __MACH__
				// Если процесс является дочерним
				if(!this->_cluster->master())
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const event::status_t)> ("server_status", event::status_t::LAUNCHED);
			#endif
		} break;
		// Если событие представляет из себя остановку процесса
		case static_cast <uint8_t> (unit::cluster_t::event_t::STOP): {
			/**
			 * Для систем, где ядро само разводит подключения между процессами кластера
			 */
			#if __AWH_CLUSTER_BALANCE__
				// Если работа юнита запущена
				if(this->working())
					// Останавливаем работу основного юнита
					unit_t::stop();
			#endif
		} break;
	}
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const pid_t, const unit::cluster_t::event_t)>  ("cluster_events", pid, event);
}
/**
 * @brief Метод обработки события получения сообщения от процесса кластера
 *
 * @param pid  идентификатор процесса
 * @param data данные полученного сообщения
 * @param size размер данных полученного сообщения
 *
 */
void awh::unit::Server::message(const pid_t pid, const uint8_t * data, const size_t size) noexcept {
	/**
	 * Для операционной системы MS Windows
	 *
	 * @note Первым сообщением работник получает передачу слушающих событий, и наружу
	 *       она не отдаётся: это разговор движка с движком, а не сообщение потребителя
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Если кластер в работе
		if(this->_cluster != nullptr){
			/**
			 * Если мастер получил объявление работника о готовности к работе
			 *
			 * @note Объявление это потребителю не предназначалось: это разговор движка
			 *       с движком, и наружу он не выходит
			 */
			if(!this->_cluster->worker()){
				// Если сообщение оказалось объявлением о готовности к работе
				if((size == sizeof(uint8_t)) && (data[0] == server_t::HANDOVER_REQUEST)){
					// Учитываем работника, готового обслуживать подключения
					this->handover(pid);
					// Прерываем выполнение: сообщение потребителю не предназначалось
					return;
				}
			// Если работник принял переданное мастером подключение
			} else if(this->handover(data, size))
				// Прерываем выполнение: сообщение потребителю не предназначалось
				return;
		}
	#endif
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const pid_t, const uint8_t *, const size_t)> ("cluster_message", pid, data, size);
}
/**
 * Для операционной системы MS Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	/**
	 * @brief Устройство записи передачи слушающего события
	 *
	 * @details Складывается из устройства события и снимка его сокета. Устройство
	 *          нужно работнику затем, чтобы опознать, которому из своих событий
	 *          достаётся снимок: событий у сервера бывает несколько, и опознание по
	 *          порядку следования было бы хрупким
	 *
	 * @note Адрес узла в записи не едет: работник задал его сам, тем же кодом, каким
	 *       задавал мастер, - у обоих процессов он один и тот же. Едет лишь порт,
	 *       и едет он опознавателем, а не настройкой
	 *
	 */
	typedef struct Handover {
		// Семейство адресов слушающего события
		uint8_t family;
		// Вид устройства обмена слушающего события
		uint8_t type;
		// Протокол слушающего события
		uint8_t protocol;
		// Порт передаваемого события
		uint16_t port;
		// Длина адреса встречной стороны передаваемого события
		uint8_t length;
		// Размер снимка передаваемого события
		uint32_t size;
	} __attribute__((packed)) handover_t;
	/**
	 * @brief Метод заведения объекта кластера
	 *
	 * @note Зовётся отовсюду, где нужна роль процесса: заводится объект единожды, а
	 *       повторные обращения ничего не делают
	 *
	 */
	void awh::unit::Server::clusterCreate() noexcept {
		// Если кластер не заказан либо уже заведён - делать нечего
		if((this->_clusterParams.mode != event::mode_t::ENABLED) || (this->_cluster != nullptr))
			// Выходим из функции
			return;
		// Создаём объект кластера для управления процессами сервера
		this->_cluster = make_unique <cluster_t> (this->_fmk, this->_log);
		// Если имя кластера установлено
		if(!this->_clusterParams.name.empty())
			// Устанавливаем название кластера
			this->_cluster->name(this->_clusterParams.name);
		// Устанавливаем максимальное количество процессов кластера
		this->_cluster->count(this->_clusterParams.count);
		// Устанавливаем флаг автоматического возрождения процессов
		this->_cluster->rebirth(this->_clusterParams.rebirth);
		// Устанавливаем параметры защиты от цикла перезапусков процессов
		this->_cluster->rebirthLimit(this->_clusterParams.restartLimit, this->_clusterParams.restartWindow);
	}
	/**
	 * @brief Метод учёта работника, готового обслуживать подключения
	 *
	 * @details Готовность работник объявляет сам, первым же своим сообщением: к мигу
	 *          его отправки канал заведомо связан, тогда как порождение работника
	 *          связи ещё не означает
	 *
	 * @param pid идентификатор процесса работника
	 *
	 */
	void awh::unit::Server::handover(const pid_t pid) noexcept {
		// Если работник уже учтён - учитывать его снова незачем
		if(std::find(this->_handoverWorkers.begin(), this->_handoverWorkers.end(), pid) != this->_handoverWorkers.end())
			// Выходим из функции
			return;
		// Запоминаем работника, готового обслуживать подключения
		this->_handoverWorkers.push_back(pid);
		// Записываем в лог сообщение о готовности работника обслуживать подключения
		this->_log->print("Cluster worker process [%d] is ready to serve connections", log_t::flag_t::INFO, pid);
	}
	/**
	 * @brief Метод передачи принятого подключения работнику кластера
	 *
	 * @details Приём подключений у этой системы делится между процессами лишь так:
	 *          принимает мастер, а обслуживает работник. Разделить сам слушающий
	 *          сокет нельзя - привязка к порту завершений живёт у объекта сокета, и
	 *          второй процесс её не получает: система отвечает отказом 87, а
	 *          завершения его приёмов уходят в порт первого процесса
	 *
	 * @note Мастер при этом не бездельничает: не досталось работника - обслуживает
	 *       подключение сам, обычным путём
	 *
	 * @param cid идентификатор принятого подключения
	 * @return    признак того, что подключение отдано работнику
	 *
	 */
	bool awh::unit::Server::handover(const event::id_t cid) noexcept {
		// Если работников, готовых обслуживать подключения, нет вовсе
		if((this->_cluster == nullptr) || this->_handoverWorkers.empty())
			// Выводим результат отказа: обслуживать подключение придётся самому
			return false;
		/**
		 * Выполняем перебор всех учтённых работников по кругу
		 *
		 * @note Круг проходится целиком: работник вправе пасть между учётом и
		 *       передачей, и подключение из-за этого терять нельзя
		 */
		for(size_t i = 0; i < this->_handoverWorkers.size(); i++){
			// Получаем работника, которому подключение достаётся по очереди
			const pid_t pid = this->_handoverWorkers.at(this->_handoverNext % this->_handoverWorkers.size());
			// Передвигаем очередь на работника следующего
			this->_handoverNext++;
			// Получаем событие обмена, ведущее к работнику
			const event::id_t dest = this->_cluster->channel(pid);
			// Если события обмена с работником нет - работник пал
			if(dest == 0)
				// Переходим к работнику следующему
				continue;
			// Буфер снимка принятого подключения
			vector <uint8_t> snapshot;
			// Выполняем снятие снимка принятого подключения для процесса работника
			if(!this->_io->snapshot(cid, dest, snapshot))
				// Переходим к работнику следующему
				continue;
			// Устройство передаваемого подключения
			handover_t record{};
			// Запоминаем семейство адресов подключения
			record.family = static_cast <uint8_t> (this->_io->family(cid));
			// Запоминаем вид устройства обмена подключения
			record.type = static_cast <uint8_t> (this->_io->type(cid));
			// Запоминаем протокол подключения
			record.protocol = static_cast <uint8_t> (this->_io->protocol(cid));
			/**
			 * Запоминаем порт встречной стороны подключения
			 *
			 * @note Спрашивается он у движка напрямую: обращения юнита о порте и
			 *       адресе спрашивают событие сервера, а подключение событием сервера
			 *       не является - им они отвечают пустотой
			 */
			record.port = this->_io->getTargetPort(cid);
			// Запоминаем размер снимка подключения
			record.size = static_cast <uint32_t> (snapshot.size());
			/**
			 * Получаем адрес встречной стороны подключения
			 *
			 * @details Едет он затем, что событие подключения у работника заводится
			 *          узлом клиента, а тому адрес встречной стороны обязателен: без
			 *          него фиксация настроек отвечает отказом. Заодно адрес этот
			 *          отдаётся и потребителю - тем же обращением, каким он берётся
			 *          у приёма своего
			 */
			const string target = this->_io->getAddress(cid, ((record.family == static_cast <uint8_t> (event::family_t::IPV6)) ? event::address_t::IPV6 : event::address_t::IPV4));
			// Запоминаем длину адреса встречной стороны подключения
			record.length = static_cast <uint8_t> (target.size());
			// Буфер сообщения передачи принятого подключения
			vector <uint8_t> message;
			// Оставляем в начале сообщения место под опознаватель передачи
			message.push_back(server_t::HANDOVER_CONNECTION);
			// Добавляем в сообщение устройство передаваемого подключения
			message.insert(message.end(), reinterpret_cast <const uint8_t *> (&record), reinterpret_cast <const uint8_t *> (&record) + sizeof(record));
			// Добавляем в сообщение адрес встречной стороны подключения
			message.insert(message.end(), target.begin(), target.end());
			// Добавляем в сообщение снимок передаваемого подключения
			message.insert(message.end(), snapshot.begin(), snapshot.end());
			// Если отправить передачу работнику не удалось
			if(this->_cluster->send(pid, message.data(), message.size()) == 0)
				// Переходим к работнику следующему
				continue;
			/**
			 * Расстаёмся со своим подключением: обслуживает его теперь работник
			 *
			 * @note Снимок уже снят, и объект сокета живёт у работника. Держи мастер
			 *       свой описатель дальше, подключение не закрылось бы и после того,
			 *       как работник его закроет
			 */
			this->_io->destroy(cid);
			// Выводим успешный результат: подключение отдано работнику
			return true;
		}
		// Выводим результат отказа: обслуживать подключение придётся самому
		return false;
	}
	/**
	 * @brief Метод подъёма принятого подключения из снимка, присланного мастером
	 *
	 * @param data данные полученного сообщения
	 * @param size размер данных полученного сообщения
	 * @return     признак того, что сообщение было передачей подключения
	 *
	 */
	bool awh::unit::Server::handover(const uint8_t * data, const size_t size) noexcept {
		// Если сообщение короче опознавателя с устройством - передачей оно не является
		if((data == nullptr) || (size < (sizeof(uint8_t) + sizeof(handover_t))) || (data[0] != server_t::HANDOVER_CONNECTION))
			// Выводим результат
			return false;
		// Устройство переданного подключения
		handover_t record{};
		// Извлекаем устройство переданного подключения
		::memcpy(&record, data + sizeof(uint8_t), sizeof(record));
		// Если остатка сообщения не хватает на адрес встречной стороны со снимком
		if((size - sizeof(uint8_t) - sizeof(record)) < (static_cast <size_t> (record.length) + static_cast <size_t> (record.size)))
			// Выводим результат
			return false;
		// Получаем адрес встречной стороны переданного подключения
		const string target(reinterpret_cast <const char *> (data + sizeof(uint8_t) + sizeof(record)), static_cast <size_t> (record.length));
		// Получаем снимок переданного подключения
		const uint8_t * snapshot = (data + sizeof(uint8_t) + sizeof(record) + static_cast <size_t> (record.length));
		// Идентификатор своего события сервера, которому подключение принадлежит
		event::id_t eid = 0;
		/**
		 * Выполняем перебор всех своих событий сервера
		 *
		 * @note Событие ищется по устройству: сервер вправе держать их несколько, и
		 *       подключение обязано достаться тому же событию, каким его принял мастер
		 */
		for(const auto & event : this->_events){
			// Если устройство своего события совпадает с устройством подключения
			if((static_cast <uint8_t> (this->_io->family(event.first)) == record.family) &&
			 (static_cast <uint8_t> (this->_io->type(event.first)) == record.type) &&
			 (static_cast <uint8_t> (this->_io->protocol(event.first)) == record.protocol)){
				// Запоминаем своё событие сервера
				eid = event.first;
				// Прерываем поиск
				break;
			}
		}
		// Если своего события сервера не нашлось
		if(eid == 0){
			// Записываем ошибку в лог
			this->_log->print("Cluster worker process [%d] has no server event for the connection received from the master", log_t::flag_t::CRITICAL, ::getpid());
			// Выводим признак того, что сообщение было передачей подключения
			return true;
		}
		/**
		 * Заводим событие подключения узлом клиента
		 *
		 * @details Принятое подключение у работника заводится именно клиентом, а не
		 *          одноранговым узлом: одноранговый узел заводит себе сам приём
		 *          подключения, и завести его снаружи нечем. Работа же у обоих одна -
		 *          связанный обмен, - и потребителю событие отдаётся принятым
		 *          подключением обычным путём
		 */
		const event::id_t cid = this->_io->event(
			event::node_t::CLIENT, static_cast <event::family_t> (record.family),
			static_cast <event::type_t> (record.type), static_cast <event::protocol_t> (record.protocol)
		);
		// Если событие подключения завести не удалось
		if(cid == 0){
			// Записываем ошибку в лог
			this->_log->print("Cluster worker process [%d] cannot create an event for the connection received from the master", log_t::flag_t::CRITICAL, ::getpid());
			// Выводим признак того, что сообщение было передачей подключения
			return true;
		}
		/**
		 * Задаём событию подключения адрес встречной стороны
		 *
		 * @note Узлу клиента адрес этот обязателен: фиксация настроек без него
		 *       отвечает отказом. Подключение при этом уже связано, и к установке
		 *       связи адрес отношения не имеет - он лишь описывает встречную сторону
		 */
		// ВРЕМЕННЫЙ ЩУП: установка адреса встречной стороны
		const bool portSet = this->_io->setTargetPort(cid, record.port);
		const bool targetSet = this->_io->setTarget(cid, target);
		this->_log->print("ЩУП работник [%d]: цель [%s]:%u, семейство=%u, порт=%u, адрес=%u", log_t::flag_t::INFO, ::getpid(), target.c_str(), static_cast <uint32_t> (record.port), static_cast <uint32_t> (record.family), static_cast <uint32_t> (portSet), static_cast <uint32_t> (targetSet));
		if(!portSet || !targetSet){
			// Удаляем заведённое событие подключения
			this->_io->destroy(cid);
			// Записываем ошибку в лог
			this->_log->print("Cluster worker process [%d] cannot set the target of the connection received from the master", log_t::flag_t::CRITICAL, ::getpid());
			// Выводим признак того, что сообщение было передачей подключения
			return true;
		}
		// Выполняем подъём подключения из присланного снимка
		if(!this->_io->restore(cid, snapshot, static_cast <size_t> (record.size))){
			// Удаляем заведённое событие подключения
			this->_io->destroy(cid);
			// Записываем ошибку в лог
			this->_log->print("Cluster worker process [%d] cannot restore the connection received from the master", log_t::flag_t::CRITICAL, ::getpid());
			// Выводим признак того, что сообщение было передачей подключения
			return true;
		}
		// ВРЕМЕННЫЙ ЩУП: поднятое подключение
		this->_log->print("ЩУП работник [%d]: поднято подключение [%llu] у события [%llu]", log_t::flag_t::INFO, ::getpid(), static_cast <uint64_t> (cid), static_cast <uint64_t> (eid));
		// Отдаём подключение потребителю обычным путём принятого подключения
		this->accept(eid, cid);
		// Выполняем запуск работы подключения
		if(!this->_io->launch(cid)){
			// Удаляем заведённое событие подключения
			this->_io->destroy(cid);
			// Записываем ошибку в лог
			this->_log->print("Cluster worker process [%d] cannot start the connection received from the master", log_t::flag_t::CRITICAL, ::getpid());
		}
		// Выводим признак того, что сообщение было передачей подключения
		return true;
	}
#endif
/**
 * @brief Метод обработки событий получения данных сервером
 *
 * @param eid  идентификатор события
 * @param data данные события получения данных сервером
 * @param size размер данных события получения данных сервером
 * @param ctx  промежуточный контекст для передачи в функцию обратного вызова
 *
 */
void awh::unit::Server::read(const event::id_t eid, const uint8_t * data, const size_t size, void * ctx) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const event::id_t, const uint8_t *, const size_t, void *)> ("read", eid, data, size, ctx);
}
/**
 * @brief Метод обработки события доступности/недоступности очереди исходящих сообщений кластера
 *
 * @param pid    идентификатор процесса
 * @param status статус доступности очереди
 * @param size   размер доступных данных очереди
 *
 */
void awh::unit::Server::available(const pid_t pid, const event::status_t status, const size_t size) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const pid_t, const event::status_t, const size_t)> ("cluster_available", pid, status, size);
}
/**
 * @brief Метод обработки события доступности/недоступности очереди исходящих данных сервера
 *
 * @param eid    идентификатор события
 * @param status статус доступности очереди
 * @param size   размер доступных данных очереди
 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
 *
 */
void awh::unit::Server::available(const event::id_t eid, const event::status_t status, const size_t size, void * ctx) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const event::id_t, const event::status_t, const size_t, void *)> ("available", eid, status, size, ctx);
}
/**
 * @brief Метод обработки событий истечения таймаута подключённого клиента
 *
 * @param eid    идентификатор подключённого клиента
 * @param action тип действия для истекшего таймаута
 * @param delay  задержка таймаута в миллисекундах
 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
 * @return       нужно ли завершить клиента после истечения таймаута
 *
 */
bool awh::unit::Server::timeout(const event::id_t eid, const event::action_t action, const uint32_t delay, void * ctx) noexcept {
	// Выполняем получение идентификатора функции обратного вызова
	const callback_t::id_t fid = this->_callback.id("timeout");
	// Если функция обратного вызова установлена
	if(this->_callback.is(fid))
		// Выполняем функцию обратного вызова
		return this->_callback.call <bool (const event::id_t, const event::action_t, const uint32_t, void *)> (fid, eid, action, delay, ctx);
	// Возвращаем значение, указывающее на то, что клиента нужно завершить после истечения таймаута
	return true;
}
/**
 * @brief Метод обработки событий ошибок кластера
 *
 * @param pid         идентификатор процесса
 * @param error       тип ошибки
 * @param description описание ошибки
 *
 */
void awh::unit::Server::error(const pid_t pid, const event::error_t error, const string & description) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const pid_t, const event::error_t, const string &)> ("cluster_error", pid, error, description);
}
/**
 * @brief Метод обработки событий ошибок сервера
 *
 * @param eid         идентификатор события
 * @param error       тип ошибки
 * @param description описание ошибки
 * @param ctx         промежуточный контекст для передачи в функцию обратного вызова
 *
 */
void awh::unit::Server::error(const event::id_t eid, const event::error_t error, const string & description, void * ctx) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const event::id_t, const event::error_t, const string &, void *)> ("error", eid, error, description, ctx);
}
/**
 * @brief Метод обработки события неотправленных данных сервера
 *
 * @param eid   идентификатор события
 * @param error тип ошибки отправки данных
 * @param data  данные, которые не получилось отправить
 * @param size  размер данных, которые не получилось отправить
 * @param ctx   промежуточный контекст для передачи в функцию обратного вызова
 *
 */
void awh::unit::Server::spool(const event::id_t eid, const event::send_error_t error, const uint8_t * data, const size_t size, void * ctx) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const event::id_t, const event::send_error_t, const uint8_t *, const size_t, void *)> ("spool", eid, error, data, size, ctx);
}
/**
 * @brief Метод проверки актуальности события
 *
 * @param eid идентификатор события
 * @return    результат проверки актуальности события
 *
 */
bool awh::unit::Server::isActual(const event::id_t eid) const noexcept {
	// Проверяем есть ли событие сервера в списке событий сервера
	return this->_events.find(eid) != this->_events.end();
}
/**
 * @brief Метод очистки чёрного списка события
 *
 * @param eid идентификатор события
 * @return    результат выполнения очистки
 *
 */
bool awh::unit::Server::clearBlacklist(const event::id_t eid) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем очистку чёрного списка события
		return this->_io->blacklist.clear(eid);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод очистки белого списка события
 *
 * @param eid идентификатор события
 * @return    результат выполнения очистки
 *
 */
bool awh::unit::Server::clearWhitelist(const event::id_t eid) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем очистку белого списка события
		return this->_io->whitelist.clear(eid);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод добавления адреса в чёрный список события
 *
 * @param eid   идентификатор события
 * @param value значение адреса события
 * @return      результат выполнения установки
 *
 */
bool awh::unit::Server::addToBlacklist(const event::id_t eid, string_view value) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем добавление адреса в чёрный список события
		return this->_io->blacklist.add(eid, value);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод добавления адреса в белый список события
 *
 * @param eid   идентификатор события
 * @param value значение адреса события
 * @return      результат выполнения установки
 *
 */
bool awh::unit::Server::addToWhitelist(const event::id_t eid, string_view value) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем добавление адреса в белый список события
		return this->_io->whitelist.add(eid, value);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод удаления адреса из чёрного списка события
 *
 * @param eid   идентификатор события
 * @param value адрес для удаления из чёрного списка
 * @return      результат выполнения удаления
 *
 */
bool awh::unit::Server::removeFromBlacklist(const event::id_t eid, string_view value) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем удаление адреса из чёрного списка события
		return this->_io->blacklist.remove(eid, value);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод удаления адреса из белого списка события
 *
 * @param eid   идентификатор события
 * @param value адрес для удаления из белого списка
 * @return      результат выполнения удаления
 *
 */
bool awh::unit::Server::removeFromWhitelist(const event::id_t eid, string_view value) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем удаление адреса из белого списка события
		return this->_io->whitelist.remove(eid, value);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения чёрного списка события
 *
 * @param eid идентификатор события
 * @return    чёрный список события
 *
 */
const unordered_map <string, awh::event::address_t> & awh::unit::Server::getFromBlacklist(const event::id_t eid) const noexcept {
	// Переменная результата
	static const unordered_map <string, awh::event::address_t> result;
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Возвращаем чёрный список события
		return this->_io->blacklist.get(eid);
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод получения белого списка события
 *
 * @param eid идентификатор события
 * @return    белый список события
 *
 */
const unordered_map <string, awh::event::address_t> & awh::unit::Server::getFromWhitelist(const event::id_t eid) const noexcept {
	// Переменная результата
	static const unordered_map <string, awh::event::address_t> result;
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Возвращаем белый список события
		return this->_io->whitelist.get(eid);
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод фиксации настроек сервера
 *
 * @param eid идентификатор события сервера
 * @return    результат выполнения фиксации
 *
 */
bool awh::unit::Server::commit(const event::id_t eid) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если событие сервера является актуальным
		if(this->isActual(eid)){
			/**
			 * Для систем, где ядро само разводит подключения между процессами кластера
			 */
			#if __AWH_CLUSTER_BALANCE__
				// Если кластер активен, значит нам необходимо проверить опции сервера
				if(this->_clusterParams.mode == event::mode_t::ENABLED){
					// Если не установлена опция переиспользования портов
					if(!(this->_io->getOptions(eid) & event::options::REUSE_PORT))
						// Выполняем установку опции для события сервера
						this->_io->setOption(eid, event::options::REUSE_PORT, true);
				}
			#endif
			/**
			 * Для операционной системы MS Windows
			 *
			 * @details Работник кластера своего сокета не заводит вовсе - он получает
			 *          его снимком от мастера, - оттого фиксация у него откладывается
			 *          целиком. Отложить её позже, у прослушивания, нельзя: к тому мигу
			 *          сокет уже был бы заведён и привязан, и снимку достался бы узел с
			 *          чужим описателем
			 *
			 * @note Отложенная фиксация отвечает успехом: событие заведено и принято к
			 *       работе, а поднимет его приход снимка - обычной же фиксацией
			 */
			#if defined(_WIN32) || defined(_WIN64)
				/**
				 * Заводим объект кластера, если он ещё не заведён
				 *
				 * @note Фиксация вправе случиться прежде запуска юнита: потребитель
				 *       заводит свои события своим порядком. Роль же процесса нужна
				 *       именно здесь, оттого объект и заводится по первому требованию
				 */
				this->clusterCreate();
				// Если процесс является работником кластера
				if((this->_cluster != nullptr) && this->_cluster->worker()){
					// Запоминаем событие как оставленное без своего сокета
					this->_handover.emplace(eid);
					// Выводим успешный результат: фиксация событию не нужна
					return true;
				}
			#endif
			// Выполняем фиксацию параметров события
			if(!(result = this->_io->commit(eid))){
				// Удаляем событие сервера
				this->_io->destroy(eid);
				// Выполняем поиск идентификатора события сервера в списке событий сервера
				auto i = this->_events.find(eid);
				// Если идентификатор события сервера найден в списке событий сервера
				if(i != this->_events.end())
					// Удаляем идентификатор события сервера из списка событий сервера
					this->_events.erase(i);
				// Если функция обратного вызова не установлена
				if(!this->_callback.is("error")){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Failed to commit server", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::CRITICAL);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Failed to commit server", log_t::flag_t::CRITICAL);
					#endif
				}
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод запуска работы сервера
 *
 * @param eid идентификатор события сервера
 * @return    результат выполнения запуска
 *
 */
bool awh::unit::Server::launch(const event::id_t eid) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если событие сервера является актуальным
		if(this->isActual(eid)){
			/**
			 * Методы только операционной системы MS Windows
			 *
			 * @note Запуск работы события у работника кластера откладывается до прихода
			 *       снимка от мастера: отклик о запуске юнита приходит задолго до него,
			 *       а событие к тому мигу стоит отсроченным - сокета у него ещё нет, и
			 *       взводить подписку не на чем
			 *
			 * @note Отложенный запуск отвечает успехом: событие принято к работе, а
			 *       случится запуск следом за прослушиванием, приходом снимка
			 */
			#if defined(_WIN32) || defined(_WIN64)
				// Если процесс является работником кластера и событие оставлено без сокета
				if((this->_cluster != nullptr) && this->_cluster->worker() &&
				 (this->_handover.find(eid) != this->_handover.end()))
					// Выводим успешный результат: запускать работнику нечего
					return true;
			#endif
			// Выполняем запуск работы сервера
			if(!(result = this->_io->launch(eid))){
				// Если функция обратного вызова не установлена
				if(!this->_callback.is("error")){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Failed to launch server", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::CRITICAL);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Failed to launch server", log_t::flag_t::CRITICAL);
					#endif
				}
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод приостановки обработки события
 *
 * @param eid идентификатор события
 * @return    результат выполнения приостановки
 *
 */
bool awh::unit::Server::pause(const event::id_t eid) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем приостановку работы сервера
		return this->_io->pause(eid);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод возобновления обработки события
 *
 * @param eid идентификатор события
 * @return    результат выполнения возобновления
 *
 */
bool awh::unit::Server::resume(const event::id_t eid) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем возобновление работы сервера
		return this->_io->resume(eid);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки промежуточного контекста события подключённого клиента
 *
 * @param eid идентификатор события сервера
 * @param ctx указатель на контекст события
 * @return    результат выполнения установки
 *
 */
bool awh::unit::Server::setContext(const event::id_t eid, void * ctx) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если событие сервера является актуальным
		if(this->isActual(eid)){
			// Если событие принадлежит к подключённому клиенту
			if((result = (this->_io->node(eid) == event::node_t::PEER))){
				// Устанавливаем функцию обратного вызова на событие изменения действий клиента
				this->_io->on(eid, static_cast <engine::callback::event_t> (std::bind(&server_t::action, this, _1, _2, ctx)));
				// Устанавливаем функцию обратного вызова на событие записи данных клиенту
				this->_io->on(eid, static_cast <engine::callback::write_t> (std::bind(&server_t::write, this, _1, _2, ctx)));
				// Устанавливаем функцию обратного вызова на событие чтения данных клиента
				this->_io->on(eid, static_cast <engine::callback::read_t> (std::bind(&server_t::read, this, _1, _2, _3, ctx)));
				// Устанавливаем функцию обратного вызова на событие неотправленных данных клиенту
				this->_io->on(eid, static_cast <engine::callback::spool_t> (std::bind(&server_t::spool, this, _1, _2, _3, _4, ctx)));
				// Устанавливаем функцию обратного вызова на событие истечения таймаута клиента
				this->_io->on(eid, static_cast <engine::callback::timeout_t> (std::bind(&server_t::timeout, this, _1, _2, _3, ctx)));
				// Устанавливаем функцию обратного вызова на событие изменения статуса клиента
				this->_io->on(eid, static_cast <engine::callback::status_t> (std::bind(static_cast <void (server_t::*)(const event::id_t, const event::status_t, void *)> (&server_t::status), this, _1, _2, ctx)));
				// Устанавливаем функцию обратного вызова на событие получения ошибок клиента
				this->_io->on(eid, static_cast <engine::callback::error_t> (std::bind(static_cast <void (server_t::*)(const event::id_t, const event::error_t, const string &, void *)> (&server_t::error), this, _1, _2, _3, ctx)));
				// Устанавливаем функцию обратного вызова на событие доступности/недоступности очереди исходящих данных клиента
				this->_io->on(eid, static_cast <engine::callback::available_t> (std::bind(static_cast <void (server_t::*)(const event::id_t, const event::status_t, const size_t, void *)> (&server_t::available), this, _1, _2, _3, ctx)));
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, ctx), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод перевода события в режим прослушивания входящих соединений
 *
 * @param eid идентификатор события сервера
 * @param max максимальное количество входящих соединений
 * @return    результат выполнения перевода в режим прослушивания
 *
 */
bool awh::unit::Server::listen(const event::id_t eid, const uint32_t max) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если событие сервера является актуальным
		if(this->isActual(eid)){
			/**
			 * Для операционной системы MS Windows
			 *
			 * @details Работник кластера своего слушающего сокета не заводит вовсе: у
			 *          этой системы нет ни `SO_REUSEPORT`, ни годного `SO_REUSEADDR`, и
			 *          привязка работника к тому же порту была бы не разделением работы,
			 *          а перехватом. Сокет приходит снимком от мастера, оттого
			 *          прослушивание здесь откладывается до его прихода
			 *
			 * @note Отложенное прослушивание отвечает успехом: событие заведено и
			 *       принято к работе, а о готовности его потребитель узнаёт обычным
			 *       путём - откликом о запуске
			 */
			#if defined(_WIN32) || defined(_WIN64)
				// Если процесс является работником кластера и событие оставлено без сокета
				if((this->_cluster != nullptr) && this->_cluster->worker() &&
				 (this->_handover.find(eid) != this->_handover.end()))
					// Выводим успешный результат: прослушивать работнику нечего
					return true;
			#endif
			// Выполняем прослушивание порта сервера для получения входящих подключений
			if(!(result = this->_io->listen(eid, max))){
				// Удаляем событие сервера
				this->_io->destroy(eid);
				// Выполняем поиск идентификатора события сервера в списке событий сервера
				auto i = this->_events.find(eid);
				// Если идентификатор события сервера найден в списке событий сервера
				if(i != this->_events.end())
					// Удаляем идентификатор события сервера из списка событий сервера
					this->_events.erase(i);
				// Если функция обратного вызова не установлена
				if(!this->_callback.is("error")){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Failed to launch server", __PRETTY_FUNCTION__, make_tuple(eid, max), log_t::flag_t::CRITICAL);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Failed to launch server", log_t::flag_t::CRITICAL);
					#endif
				}
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, max), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод получения данных от клиента
 *
 * @param eid идентификатор события клиента
 * @return    результат получения данных
 *
 */
bool awh::unit::Server::recv(const event::id_t eid) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение данных от клиента
		return this->_io->recv(eid);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод отправки данных клиенту
 *
 * @param eid    идентификатор события клиента
 * @param buffer буфер данных для отправки
 * @param size   размер данных для отправки
 * @return       количество байт данных, отправленных клиенту
 *
 */
size_t awh::unit::Server::send(const event::id_t eid, const void * buffer, const size_t size) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем отправку данных клиенту
		return this->_io->send(eid, buffer, size);
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод назначения источника данных для вытягивающей модели отправки
 *
 * @param eid    идентификатор события клиента
 * @param source функция обратного вызова источника данных
 *
 */
void awh::unit::Server::source(const event::id_t eid, engine::callback::source_t source) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Устанавливаем функцию обратного вызова источника данных
		this->_io->on(eid, source);
}
/**
 * @brief Метод объединения данных между сервером и другим событием
 *
 * @param eid  идентификатор события-источника
 * @param dest идентификатор события-приёмника
 * @return     результат выполнения объединения
 *
 */
bool awh::unit::Server::splice(const event::id_t eid, const event::id_t dest) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid) || this->isActual(dest))
		// Выполняем объединение данных между событием сервера и другим событием
		return this->_io->splice(eid, dest);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения опций сервера
 *
 * @param eid идентификатор события сервера
 * @return    опции сервера
 *
 */
uint16_t awh::unit::Server::getOptions(const event::id_t eid) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение опций для события сервера
		return this->_io->getOptions(eid);
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки опций сервера
 *
 * @param eid     идентификатор события сервера
 * @param options опции сервера для установки
 * @return        результат выполнения установки
 *
 */
bool awh::unit::Server::setOptions(const event::id_t eid, const uint16_t options) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку опций для события сервера
		return this->_io->setOptions(eid, options);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки опции сервера
 *
 * @param eid    идентификатор события сервера
 * @param option опция сервера для установки
 * @param mode   режим установки опции сервера
 * @return       результат выполнения установки
 *
 */
bool awh::unit::Server::setOption(const event::id_t eid, const uint16_t option, const bool mode) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку опции для события сервера
		return this->_io->setOption(eid, option, mode);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения метаданных последнего принятого дейтаграммного пакета
 *
 * @param eid идентификатор события сервера
 * @return    метаданные последнего принятого дейтаграммного пакета
 *
 */
awh::net::dgram_info_t awh::unit::Server::getTrafficInfo(const event::id_t eid) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение метаданных последнего принятого дейтаграммного пакета для события сервера
		return this->_io->getTrafficInfo(eid);
	// Возвращаем значение по умолчанию
	return net::dgram_info_t();
}
/**
 * @brief Метод получения количества хопов последнего принятого пакета
 *
 * @param eid идентификатор события сервера
 * @return    количество хопов последнего принятого пакета
 *
 */
uint8_t awh::unit::Server::getCountHops(const event::id_t eid) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение количества хопов последнего принятого пакета для события сервера
		return this->_io->getCountHops(eid);
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки количества хопов последнего принятого пакета
 *
 * @param eid  идентификатор события сервера
 * @param hops количество хопов последнего принятого пакета
 * @return     результат выполнения установки
 *
 */
bool awh::unit::Server::setCountHops(const event::id_t eid, const uint8_t hops) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку количества хопов последнего принятого пакета для события сервера
		return this->_io->setCountHops(eid, hops);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения максимального количества хопов, через которые может пройти пакет
 *
 * @param eid идентификатор события сервера
 * @return    максимальное количество хопов
 *
 */
awh::event::hops_t awh::unit::Server::getHops(const event::id_t eid) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение максимального количества хопов для события сервера
		return this->_io->getHops(eid);
	// Возвращаем значение по умолчанию
	return awh::event::hops_t::LOOPBACK;
}
/**
 * @brief Метод установки максимального количества хопов, через которые может пройти пакет
 *
 * @param eid  идентификатор события сервера
 * @param hops максимальное количество хопов
 * @return     результат работы функции
 *
 */
bool awh::unit::Server::setHops(const event::id_t eid, const event::hops_t hops) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку максимального количества хопов для события сервера
		return this->_io->setHops(eid, hops);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения сетевого интерфейса сервера
 *
 * @param eid идентификатор события сервера
 * @return    сетевой интерфейс сервера
 *
 */
string awh::unit::Server::getIface(const event::id_t eid) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение сетевого интерфейса для события сервера
		return this->_io->getIface(eid);
	// Возвращаем значение по умолчанию
	return "";
}
/**
 * @brief Метод установки сетевого интерфейса сервера
 *
 * @param eid  идентификатор события сервера
 * @param name имя сетевого интерфейса для установки
 * @return     результат выполнения установки
 *
 */
bool awh::unit::Server::setIface(const event::id_t eid, string_view name) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку сетевого интерфейса для события сервера
		return this->_io->setIface(eid, name);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения порта сервера
 *
 * @param eid идентификатор события сервера
 * @return    порт сервера
 *
 */
uint16_t awh::unit::Server::getPort(const event::id_t eid) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение порта сервера для события сервера
		return this->_io->getSourcePort(eid);
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки порта сервера
 *
 * @param eid  идентификатор события сервера
 * @param port порт сервера для установки
 * @return     результат выполнения установки
 *
 */
bool awh::unit::Server::setPort(const event::id_t eid, const uint16_t port) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку порта сервера для события сервера
		return this->_io->setSourcePort(eid, port);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения адреса сервера
 *
 * @param eid     идентификатор события сервера
 * @param address тип адреса сервера
 * @return        значение адреса сервера
 *
 */
string awh::unit::Server::getAddress(const event::id_t eid, const event::address_t address) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение адреса сервера для события сервера
		return this->_io->getAddress(eid, address);
	// Возвращаем значение по умолчанию
	return "";
}
/**
 * @brief Метод установки адреса сервера
 *
 * @param eid     идентификатор события сервера
 * @param address тип адреса сервера
 * @param value   значение адреса сервера
 * @return        результат выполнения установки
 *
 */
bool awh::unit::Server::setAddress(const event::id_t eid, const event::address_t address, string_view value) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку адреса сервера для события сервера
		return this->_io->setAddress(eid, address, value);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки адреса сервера
 *
 * @param eid     идентификатор события сервера
 * @param address тип адреса сервера
 * @param value   значение адреса сервера
 * @return        результат выполнения установки
 *
 */
bool awh::unit::Server::setAddress(const event::id_t eid, const event::address_t address, const net::addr_t * value) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку адреса сервера для события сервера
		return this->_io->setAddress(eid, address, value);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения адреса сервера
 *
 * @param eid     идентификатор события сервера
 * @param address тип адреса сервера
 * @param value   объект для извлечения адреса сервера
 * @return        результат выполнения извлечения адреса сервера
 *
 */
bool awh::unit::Server::getAddress(const event::id_t eid, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение адреса сервера для события сервера
		return this->_io->getAddress(eid, address, value);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения MTU сетевого интерфейса
 *
 * @param eid идентификатор события сервера
 * @return    MTU сетевого интерфейса
 *
 */
uint16_t awh::unit::Server::getMaximumTransmissionUnit(const event::id_t eid) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение MTU сетевого интерфейса для события сервера
		return this->_io->getMaximumTransmissionUnit(eid);
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки MTU сетевого интерфейса
 *
 * @param eid идентификатор события сервера
 * @param mtu размер MTU интерфейса
 * @return    результат установки MTU сетевого интерфейса
 *
 */
bool awh::unit::Server::setMaximumTransmissionUnit(const event::id_t eid, const uint32_t mtu) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку MTU сетевого интерфейса для события сервера
		return this->_io->setMaximumTransmissionUnit(eid, mtu);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения режима трансляции пакетов сервера
 *
 * @param eid идентификатор события сервера
 * @return    режим трансляции пакетов (unicast, multicast, broadcast)
 *
 */
awh::event::delivery_mode_t awh::unit::Server::getDelivery(const event::id_t eid) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение режима трансляции пакетов сервера для события сервера
		return this->_io->getDelivery(eid);
	// Возвращаем значение по умолчанию
	return event::delivery_mode_t::NONE;
}
/**
 * @brief Метод установки режима трансляции пакетов сервера
 *
 * @param eid      идентификатор события сервера
 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
 * @return         результат выполнения установки
 *
 */
bool awh::unit::Server::setDelivery(const event::id_t eid, const event::delivery_mode_t delivery) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку режима трансляции пакетов сервера для события сервера
		return this->_io->setDelivery(eid, delivery);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения размера буфера сервера
 *
 * @param eid    идентификатор события сервера
 * @param action тип действия сервера
 * @return       размер буфера сервера
 *
 */
size_t awh::unit::Server::getBufferSize(const event::id_t eid, const event::action_t action) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение размера буфера сервера для события сервера
		return this->_io->getBufferSize(eid, action);
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки размера буфера сервера
 *
 * @param eid    идентификатор события сервера
 * @param action тип действия сервера
 * @param size   размер буфера сервера
 * @return       результат выполнения установки
 *
 */
bool awh::unit::Server::setBufferSize(const event::id_t eid, const event::action_t action, const size_t size) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку размера буфера сервера для события сервера
		return this->_io->setBufferSize(eid, action, size);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения режима использования таймаута на чтение события
 *
 * @param eid идентификатор события
 * @return    режим использования таймаута на чтение события
 *
 */
awh::event::usage_t awh::unit::Server::getUsageReadTimeout(const event::id_t eid) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение режима использования таймаута на чтение события для события сервера
		return this->_io->getUsageReadTimeout(eid);
	// Возвращаем значение по умолчанию
	return event::usage_t::NONE;
}
/**
 * @brief Метод установки режима использования таймаута на чтение события
 *
 * @param eid   идентификатор события
 * @param usage режим использования таймаута на чтение события (reusable или disposable)
 *
 */
void awh::unit::Server::setUsageReadTimeout(const event::id_t eid, const event::usage_t usage) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку режима использования таймаута на чтение события для события сервера
		this->_io->setUsageReadTimeout(eid, usage);
}
/**
 * @brief Метод получения таймаута сервера
 *
 * @param eid    идентификатор события сервера
 * @param action тип действия сервера
 * @return       значение таймаута в миллисекундах
 *
 */
uint32_t awh::unit::Server::getTimeout(const event::id_t eid, const event::action_t action) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение параметров таймаута для сервера
		return this->_io->getTimeout(eid, action);
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки таймаута сервера
 *
 * @param eid     идентификатор события сервера
 * @param action  тип действия сервера
 * @param timeout значение таймаута в миллисекундах
 *
 */
void awh::unit::Server::setTimeout(const event::id_t eid, const event::action_t action, const uint32_t timeout) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку параметров таймаута для сервера
		this->_io->setTimeout(eid, action, timeout);
}
/**
 * @brief Метод установки пропускной способности сервера
 *
 * @param eid       идентификатор события сервера
 * @param limiting  режим ограничения пропускной способности сервера (egress или ingress)
 * @param bandwidth пропускная способность сервера для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
 * @return          результат выполнения установки
 *
 */
bool awh::unit::Server::bandwidth(const event::id_t eid, const event::limiting_t limiting, string_view bandwidth) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку параметров пропускной способности для сервера
		return this->_io->bandwidth(eid, limiting, bandwidth);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки параметров keep-alive для сервера
 *
 * @param eid   идентификатор события сервера
 * @param cnt   количество пакетов keep-alive
 * @param idle  время простоя перед отправкой первого пакета keep-alive в секундах
 * @param intvl интервал между пакетами keep-alive в секундах
 * @return      результат выполнения установки
 *
 */
bool awh::unit::Server::keepAlive(const event::id_t eid, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку параметров keep-alive для сервера
		return this->_io->keepAlive(eid, cnt, idle, intvl);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
 *
 * @param eid идентификатор события сервера
 * @return    значение DSCP
 *
 */
awh::event::dscp_t awh::unit::Server::getDifferentiatedServicesCodePoint(const event::id_t eid) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение значения поля Differentiated Services Code Point (DSCP) для сервера
		return this->_io->getDifferentiatedServicesCodePoint(eid, this->_io->family(eid));
	// Возвращаем значение по умолчанию
	return event::dscp_t::CS0;
}
/**
 * @brief Метод установки значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
 *
 * @param eid  идентификатор события сервера
 * @param dscp значение DSCP
 * @return     результат работы функции
 *
 */
bool awh::unit::Server::setDifferentiatedServicesCodePoint(const event::id_t eid, const event::dscp_t dscp) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку значения поля Differentiated Services Code Point (DSCP) для сервера
		return this->_io->setDifferentiatedServicesCodePoint(eid, this->_io->family(eid), dscp);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения обнаружения максимального размера пакета (MTU)
 *
 * @param eid идентификатор события сервера
 * @return    режим обнаружения максимального размера пакета (MTU)
 *
 */
awh::event::mtu_discover_t awh::unit::Server::getMaximumTransmissionUnitDiscover(const event::id_t eid) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение обнаружения максимального размера пакета (MTU) для сервера
		return this->_io->getMaximumTransmissionUnitDiscover(eid, this->_io->family(eid));
	// Возвращаем значение по умолчанию
	return event::mtu_discover_t::NONE;
}
/**
 * @brief Метод установки обнаружения максимального размера пакета (MTU)
 *
 * @param eid  идентификатор события сервера
 * @param mode режим обнаружения максимального размера пакета (MTU)
 * @return     результат работы функции
 *
 */
bool awh::unit::Server::setMaximumTransmissionUnitDiscover(const event::id_t eid, const event::mtu_discover_t mode) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку обнаружения максимального размера пакета (MTU) для сервера
		return this->_io->setMaximumTransmissionUnitDiscover(eid, this->_io->family(eid), mode);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод активации/деактивации мультикаст группы
 *
 * @param eid    идентификатор события сервера
 * @param mode   режим активации/деактивации
 * @param group  мультикаст-группа для активации/деактивации
 * @param source адрес сетевого интерфейса с которого выполняется подписка
 * @param port   порт мультикаст-группы с которого выполняется подписка
 * @return       результат выполнения установки
 *
 */
bool awh::unit::Server::membership(const event::id_t eid, const event::mode_t mode, string_view group, string_view source, const uint16_t port) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем активацию/деактивацию мультикаст группы для сервера
		return this->_io->membership(eid, mode, group, source, port);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод активации/деактивации мультикаст группы
 *
 * @param eid    идентификатор события сервера
 * @param mode   режим активации/деактивации
 * @param group  мультикаст-группа для активации/деактивации
 * @param source адрес сетевого интерфейса с которого выполняется подписка
 * @param port   порт мультикаст-группы с которого выполняется подписка
 * @return       результат выполнения установки
 *
 */
bool awh::unit::Server::membership(const event::id_t eid, const event::mode_t mode, const net::addr_t * group, const net::addr_t * source, const uint16_t port) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем активацию/деактивацию мультикаст группы для сервера
		return this->_io->membership(eid, mode, group, source, port);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод очистки событий сервера
 *
 */
void awh::unit::Server::clear() noexcept {
	// Если в списке событий сервера есть события
	if(!this->_events.empty()){
		/**
		 * Удаляем все события сервера
		 */
		for(const auto & event : this->_events)
			// Удаляем событие сервера
			this->_io->destroy(event.first);
		// Очищаем внутренние индексы событий
		this->_events.clear();
		// Очищаем список клиентов сервера
		this->_serverClients.clear();
		// Очищаем позиции клиентов в индексе
		this->_clientPositions.clear();
	}
}
/**
 * @brief Метод остановки сервера
 *
 */
void awh::unit::Server::stop() noexcept {
	// Если работа юнита запущена
	if(this->working()){
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Если кластер в работе и процесс является родительским
			if((this->_cluster != nullptr) && this->_cluster->master())
				// Останавливаем работу кластера: работники уходят вместе с мастером
				this->_cluster->stop();
			// Выполняем остановку работы основного юнита
			unit_t::stop();
		/**
		 * Для систем, где ядро само разводит подключения между процессами кластера
		 */
		#elif __AWH_CLUSTER_BALANCE__
			/**
			 * Определяем режим запуска сервера
			 */
			switch(static_cast <uint8_t> (this->_clusterParams.mode)){
				// Если необходимо активировать кластер
				case static_cast <uint8_t> (event::mode_t::ENABLED): {
					// Если кластер инициализирован
					if(this->_cluster != nullptr)
						// Останавливаем работу кластера
						this->_cluster->stop();
				} break;
				// Если кластер отключён
				case static_cast <uint8_t> (event::mode_t::DISABLED):
					// Выполняем остановку работы основного юнита
					unit_t::stop();
				break;
			}
		/**
		 * Для операционной системы OpenBSD, NetBSD, Sun Solaris или macOS
		 */
		#elif __OpenBSD__ || __NetBSD__ || __sun__ || __APPLE__ || __MACH__
			// Выполняем остановку работы основного юнита
			unit_t::stop();
		/**
			 * Операционная система не поддерживается
		 */
		#else
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог запуска события
				this->_log->debug("This operating system is not supported", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог запуска события
				this->_log->print("This operating system is not supported", log_t::flag_t::CRITICAL);
			#endif
			// Выходим из приложения
			::_exit(EXIT_FAILURE);
		#endif
	}
}
/**
 * @brief Метод запуска сервера
 *
 */
void awh::unit::Server::start() noexcept {
	// Если работа юнита ещё не запущена
	if(!this->working()){
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			/**
			 * Кластер заводится ДО запуска юнита
			 *
			 * @details Работник обязан узнать свою роль прежде, чем потребитель заведёт
			 *          свои события: фиксация их у работника откладывается, а спросить
			 *          роль не у чего, покуда кластера нет. Установлено прогоном -
			 *          фиксация случалась раньше запуска кластера, отсрочка не
			 *          срабатывала, и работник заводил свой сокет вместо переданного
			 *
			 * @note Заводится здесь только объект: запускается кластер прежним порядком -
			 *       откликом о запуске юнита, когда слушающие события мастера уже подняты
			 */
			this->clusterCreate();
			// Выполняем получение идентификатора функции обратного вызова
			const callback_t::id_t fid = this->_callback.id("status");
			// Если функция обратного вызова установлена
			if(this->_callback.is(fid))
				// Переименовываем callback status в server_status
				this->_callback.set(fid, this->_callback.id("server_status"), this->_callback);
			// Устанавливаем функцию обратного вызова на запуск системы
			this->_callback.on <void (const event::status_t)> (fid, static_cast <void (server_t::*)(const event::status_t)> (&server_t::launch), this, _1);
			// Выполняем запуск работы основного юнита
			unit_t::start();
		/**
		 * Для систем, где ядро само разводит подключения между процессами кластера
		 */
		#elif __AWH_CLUSTER_BALANCE__
			/**
			 * Определяем режим запуска сервера
			 */
			switch(static_cast <uint8_t> (this->_clusterParams.mode)){
				// Если необходимо активировать кластер
				case static_cast <uint8_t> (event::mode_t::ENABLED): {
					// Если кластер не инициализирован
					if(this->_cluster == nullptr){
						// Создаём объект кластера для управления процессами сервера
						this->_cluster = make_unique <cluster_t> (this->_fmk, this->_log);
						// Если имя кластера установлено
						if(!this->_clusterParams.name.empty())
							// Устанавливаем название кластера
							this->_cluster->name(this->_clusterParams.name);
						// Устанавливаем максимальное количество процессов кластера
						this->_cluster->count(this->_clusterParams.count);
						// Устанавливаем флаг автоматического возрождения процессов
						this->_cluster->rebirth(this->_clusterParams.rebirth);
						// Устанавливаем параметры защиты от цикла перезапусков процессов
						this->_cluster->rebirthLimit(this->_clusterParams.restartLimit, this->_clusterParams.restartWindow);
					}
					// Устанавливаем функцию обратного вызова на событие получения сигнала
					this->_cluster->on <void (const pid_t, const int32_t)> ("exit", &server_t::exit, this, _1, _2);
					// Устанавливаем функцию обратного вызова на событие пересоздания процесса
					this->_cluster->on <void (const pid_t, const pid_t)> ("rebase", &server_t::rebase, this, _1, _2);
					// Устанавливаем функцию обратного вызова на событие отправки сообщений
					this->_cluster->on <void (const pid_t, const size_t)> ("sending", &server_t::sending, this, _1, _2);
					// Устанавливаем функцию обратного вызова на получение событий кластера
					this->_cluster->on <void (const pid_t, const unit::cluster_t::event_t)> ("events", &server_t::cluster, this, _1, _2);
					// Устанавливаем функцию обратного вызова на событие получения сообщений
					this->_cluster->on <void (const pid_t, const uint8_t *, const size_t)> ("message", &server_t::message, this, _1, _2, _3);
					// Устанавливаем функцию обратного вызова на событие изменения статуса кластера
					this->_cluster->on <void (const pid_t, const event::status_t)> ("state", static_cast <void (server_t::*)(const pid_t, const event::status_t)> (&server_t::status), this, _1, _2);
					// Устанавливаем функцию обратного вызова на событие получения ошибок кластера
					this->_cluster->on <void (const pid_t, const event::error_t, const string &)> ("error", static_cast <void (server_t::*)(const pid_t, const event::error_t, const string &)> (&server_t::error), this, _1, _2, _3);
					// Устанавливаем функцию обратного вызова на событие доступности/недоступности очереди исходящих данных кластера
					this->_cluster->on <void (const pid_t, const event::status_t, const size_t)> ("available", static_cast <void (server_t::*)(const pid_t, const event::status_t, const size_t)> (&server_t::available), this, _1, _2, _3);
					// Запускаем работу кластера
					this->_cluster->start();
				} break;
				// Если кластер отключён
				case static_cast <uint8_t> (event::mode_t::DISABLED): {
					// Выполняем получение идентификатора функции обратного вызова
					const callback_t::id_t fid = this->_callback.id("status");
					// Если функция обратного вызова установлена
					if(this->_callback.is(fid))
						// Переименовываем callback status в server_status
						this->_callback.set(fid, this->_callback.id("server_status"), this->_callback);
					// Устанавливаем функцию обратного вызова на запуск системы
					this->_callback.on <void (const event::status_t)> (fid, static_cast <void (server_t::*)(const event::status_t)> (&server_t::launch), this, _1);
					// Выполняем запуск работы основного юнита
					unit_t::start();
				} break;
			}
		/**
		 * Для операционной системы OpenBSD, NetBSD, Sun Solaris или macOS
		 */
		#elif __OpenBSD__ || __NetBSD__ || __sun__ || __APPLE__ || __MACH__
			// Выполняем получение идентификатора функции обратного вызова
			const callback_t::id_t fid = this->_callback.id("status");
			// Если функция обратного вызова установлена
			if(this->_callback.is(fid))
				// Переименовываем callback status в server_status
				this->_callback.set(fid, this->_callback.id("server_status"), this->_callback);
			// Устанавливаем функцию обратного вызова на запуск системы
			this->_callback.on <void (const event::status_t)> (fid, static_cast <void (server_t::*)(const event::status_t)> (&server_t::launch), this, _1);
			// Выполняем запуск работы основного юнита
			unit_t::start();
		/**
		 * Операционная система не поддерживается
		 */
		#else
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог запуска события
				this->_log->debug("This operating system is not supported", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог запуска события
				this->_log->print("This operating system is not supported", log_t::flag_t::CRITICAL);
			#endif
			// Выходим из приложения
			::_exit(EXIT_FAILURE);
		#endif
	}
}
/**
 * @brief Метод установки функций обратного вызова
 *
 * @param callback функции обратного вызова
 *
 */
void awh::unit::Server::callback(const callback_t & callback) noexcept {
	// Устанавливаем функцию обратного вызова для родительского юнита
	unit_t::callback(callback);
	// Выполняем установку функции обратного вызова при получении данных от клиента
	this->_callback.set("read", callback);
	// Выполняем установку функции обратного вызова при отправке данных клиенту
	this->_callback.set("write", callback);
	// Выполняем установку функции обратного вызова при получении состояния сервера
	this->_callback.set("state", callback);
	// Выполняем установку функции обратного вызова при получении события неотправленных данных
	this->_callback.set("spool", callback);
	// Выполняем установку функции обратного вызова при обработке действий сервера
	this->_callback.set("action", callback);
	// Выполняем установку функции обратного вызова при принятии нового подключения
	this->_callback.set("accept", callback);
	// Выполняем установку функции обратного вызова при получении информационных метаданных о дейтаграммном пакете
	this->_callback.set("traffic", callback);
	// Выполняем установку функции обратного вызова на событие истечения таймаута подключённого клиента
	this->_callback.set("timeout", callback);
	// Выполняем установку функции обратного вызова при получении событий доступности/недоступности очереди исходящих данных сервера
	this->_callback.set("available", callback);
	// Выполняем установку функции обратного вызова при завершении работы процесса кластера
	this->_callback.set("cluster_exit", callback);
	// Выполняем установку функции обратного вызова при получении ошибок кластера
	this->_callback.set("cluster_error", callback);
	// Выполняем установку функции обратного вызова при получении состояния процесса кластера
	this->_callback.set("cluster_state", callback);
	// Выполняем установку функции обратного вызова при пересоздании процесса кластера
	this->_callback.set("cluster_rebase", callback);
	// Выполняем установку функции обратного вызова при запуске/остановке процесса кластера
	this->_callback.set("cluster_events", callback);
	// Выполняем установку функции обратного вызова при отправке сообщения кластера
	this->_callback.set("cluster_sending", callback);
	// Выполняем установку функции обратного вызова при получении сообщения кластера
	this->_callback.set("cluster_message", callback);
	// Выполняем установку функции обратного вызова при изменении доступности очереди исходящих сообщений кластера
	this->_callback.set("cluster_available", callback);
}
/**
 * @brief Метод уничтожения события сервера
 *
 * @param eid идентификатор события для уничтожения
 *
 */
void awh::unit::Server::destroy(const event::id_t eid) noexcept {
	// Если в списке событий есть события
	if(!this->_events.empty()){
		// Выполняем поиск идентификатора события сервера в списке событий
		auto i = this->_events.find(eid);
		// Если идентификатор события сервера найден
		if(i != this->_events.end()){
			// Если идентификатор события является сервером
			if(i->second == 0){
				// Удаляем все клиентские события данного сервера
				this->unlinkServerClients(i->first);
				// Удаляем событие сервера
				this->_io->destroy(i->first);
				// Удаляем идентификатор события сервера из списка событий
				this->_events.erase(i);
			// Если идентификатор относится к клиентскому событию
			} else {
				// Удаляем событие сервера
				this->_io->destroy(i->first);
				// Удаляем идентификатор клиентского события из индексов
				this->unlinkClient(i->first);
			}
		}
	}
}
/**
 * @brief Метод создания серверного события
 *
 * @param family   семейство адресов
 * @param type     тип события
 * @param protocol протокол события
 * @return         идентификатор созданного серверного события
 *
 */
awh::event::id_t awh::unit::Server::issue(const event::family_t family, const event::type_t type, const event::protocol_t protocol) noexcept {
	// Переменная результата
	event::id_t result = 0;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Добавляем новое событие сервера
		result = this->_io->event(event::node_t::SERVER, family, type, protocol);
		// Если событие сервера успешно создано
		if(result > 0){
			// Устанавливаем функцию обратного вызова на событие изменения действий сервера
			this->_io->on(result, static_cast <engine::callback::event_t> (std::bind(&server_t::action, this, _1, _2, nullptr)));
			// Устанавливаем функцию обратного вызова на событие записи данных
			this->_io->on(result, static_cast <engine::callback::write_t> (std::bind(&server_t::write, this, _1, _2, nullptr)));
			// Устанавливаем функцию обратного вызова на событие разрешения подключения
			this->_io->on(result, static_cast <engine::callback::accept_t> (std::bind(&server_t::accept, this, _1, _2)));
			// Устанавливаем функцию обратного вызова на событие получения информационных метаданных о дейтаграммном пакете
			this->_io->on(result, static_cast <engine::callback::traffic_t> (std::bind(&server_t::traffic, this, _1, _2)));
			// Устанавливаем функцию обратного вызова на событие неотправленных данных сервера
			this->_io->on(result, static_cast <engine::callback::spool_t> (std::bind(&server_t::spool, this, _1, _2, _3, _4, nullptr)));
			// Устанавливаем функцию обратного вызова на событие изменения статуса сервера
			this->_io->on(result, static_cast <engine::callback::status_t> (std::bind(static_cast <void (server_t::*)(const event::id_t, const event::status_t, void *)> (&server_t::status), this, _1, _2, nullptr)));
			// Устанавливаем функцию обратного вызова на событие получения ошибок
			this->_io->on(result, static_cast <engine::callback::error_t> (std::bind(static_cast <void (server_t::*)(const event::id_t, const event::error_t, const string &, void *)> (&server_t::error), this, _1, _2, _3, nullptr)));
			// Добавляем идентификатор события сервера в список событий сервера
			this->_events.emplace(result, 0);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family), static_cast <uint16_t> (type), static_cast <uint16_t> (protocol)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод установки названия кластера
 *
 * @param name название кластера для установки
 *
 */
void awh::unit::Server::clusterName(string_view name) noexcept {
	// Устанавливаем название кластера
	this->_clusterParams.name = name;
	// Если кластер инициализирован
	if(this->_cluster != nullptr)
		// Устанавливаем название кластера
		this->_cluster->name(this->_clusterParams.name);
}
/**
 * @brief Метод получения семейства кластера
 *
 * @return семейство к которому принадлежит кластер (MASTER или CHILDREN)
 *
 */
awh::unit::cluster_t::family_t awh::unit::Server::clusterFamily() const noexcept {
	// Если кластер инициализирован
	if(this->_cluster != nullptr){
		// Если текущий процесс кластера является родительским
		if(this->_cluster->master())
			// Устанавливаем результат работы функции
			return cluster_t::family_t::MASTER;
		// Если текущий процесс кластера является дочерним
		return cluster_t::family_t::CHILDREN;
	}
	// Возвращаем значение по умолчанию
	return cluster_t::family_t::NONE;
}
/**
 * @brief Метод получения режима активации кластера
 *
 * @return режим активации кластера
 *
 */
awh::event::mode_t awh::unit::Server::clusterMode() const noexcept {
	// Извлекаем режим активации кластера
	return this->_clusterParams.mode;
}
/**
 * @brief Метод установки режима работы кластера
 *
 * @param mode режим активации/деактивации кластера
 *
 */
void awh::unit::Server::clusterMode(const event::mode_t mode) noexcept {
	// Устанавливаем режим активации кластера
	this->_clusterParams.mode = mode;
}
/**
 * @brief Метод получения максимального количества процессов
 *
 * @return максимальное количество процессов
 *
 */
uint16_t awh::unit::Server::clusterCount() const noexcept {
	// Если кластер инициализирован
	if(this->_cluster != nullptr)
		// Возвращаем максимальное количество процессов кластера
		return this->_cluster->count();
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки максимального количества процессов
 *
 * @param count максимальное количество процессов
 *
 */
void awh::unit::Server::clusterCount(const uint16_t count) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Локальная переменная для работы с количеством процессов
		this->_clusterParams.count = count;
		// Если количество процессов передано пустое
		if(this->_clusterParams.count == 0){
			// Устанавливаем количество доступных ядер в системе
			this->_clusterParams.count = static_cast <uint16_t> (thread::hardware_concurrency());
			// Если количество доступных воркеров больше одного, уменьшаем пополам
			if(this->_clusterParams.count > 1)
				// Уменьшаем количество воркеров в два раза
				this->_clusterParams.count /= 2;
		}
		// Если кластер инициализирован
		if(this->_cluster != nullptr)
			// Устанавливаем максимальное количество процессов кластера
			this->_cluster->count(this->_clusterParams.count);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(count), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод получения списка дочерних процессов
 *
 * @return список дочерних процессов
 *
 */
unordered_set <pid_t> awh::unit::Server::clusterWorkers() const noexcept {
	// Если кластер инициализирован
	if(this->_cluster != nullptr)
		// Возвращаем количество активных воркеров кластера
		return this->_cluster->workers();
	// Возвращаем значение по умолчанию
	return {};
}
/**
 * @brief Метод отправки сообщения родительскому процессу
 *
 * @param buffer бинарный буфер для отправки сообщения
 * @param size   размер бинарного буфера для отправки сообщения
 * @return       количество байт отправленного сообщения
 *
 */
size_t awh::unit::Server::clusterSend(const void * buffer, const size_t size) noexcept {
	// Если кластер инициализирован
	if(this->_cluster != nullptr)
		// Выполняем отправку сообщения родительскому процессу
		return this->_cluster->send(buffer, size);
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод отправки сообщения дочернему процессу
 *
 * @param pid    идентификатор процесса для получения сообщения
 * @param buffer бинарный буфер для отправки сообщения
 * @param size   размер бинарного буфера для отправки сообщения
 * @return       количество байт отправленного сообщения
 *
 */
size_t awh::unit::Server::clusterSend(const pid_t pid, const void * buffer, const size_t size) noexcept {
	// Если кластер инициализирован
	if(this->_cluster != nullptr)
		// Выполняем отправку сообщения дочернему процессу
		return this->_cluster->send(pid, buffer, size);
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод отправки сообщения всем дочерним процессам
 *
 * @param buffer бинарный буфер для отправки сообщения
 * @param size   размер бинарного буфера для отправки сообщения
 * @return       количество байт отправленного сообщения
 *
 */
size_t awh::unit::Server::clusterBroadcast(const void * buffer, const size_t size) noexcept {
	// Если кластер инициализирован
	if(this->_cluster != nullptr)
		// Выполняем отправку сообщения всем дочерним процессам
		return this->_cluster->broadcast(buffer, size);
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки флага автоматического возрождения процессов
 *
 * @param mode флаг возрождения процессов
 *
 */
void awh::unit::Server::clusterRebirth(const bool mode) noexcept {
	// Устанавливаем флаг автоматического возрождения процессов
	this->_clusterParams.rebirth = mode;
	// Если кластер инициализирован
	if(this->_cluster != nullptr)
		// Устанавливаем флаг автоматического возрождения процессов
		this->_cluster->rebirth(this->_clusterParams.rebirth);
}
/**
 * @brief Метод установки параметров защиты от цикла перезапусков процессов кластера
 *
 * @param limit  максимальное число подряд идущих быстрых падений до остановки кластера (0 — без ограничения)
 * @param window временное окно «быстрого» (раннего) падения процесса в миллисекундах
 *
 */
void awh::unit::Server::clusterRebirthLimit(const uint16_t limit, const uint64_t window) noexcept {
	// Устанавливаем максимальное число подряд идущих быстрых падений процессов
	this->_clusterParams.restartLimit = limit;
	// Устанавливаем временное окно «быстрого» падения процесса
	this->_clusterParams.restartWindow = window;
	// Если кластер инициализирован
	if(this->_cluster != nullptr)
		// Устанавливаем параметры защиты от цикла перезапусков процессов
		this->_cluster->rebirthLimit(this->_clusterParams.restartLimit, this->_clusterParams.restartWindow);
}
/**
 * @brief Метод получения типа протокола передачи данных между воркерами
 *
 * @return тип протокола передачи данных между воркерами
 *
 */
awh::event::type_t awh::unit::Server::clusterGetTypeEventMessage() const noexcept {
	// Если кластер инициализирован
	if(this->_cluster != nullptr)
		// Выполняем получение типа протокола передачи данных между воркерами
		return this->_cluster->getTypeEventMessage();
	// Возвращаем значение по умолчанию
	return event::type_t::NONE;
}
/**
 * @brief Метод установки типа протокола передачи данных между воркерами
 *
 * @param type тип протокола передачи данных между воркерами для установки
 *
 */
void awh::unit::Server::clusterSetTypeEventMessage(const event::type_t type) noexcept {
	// Если кластер инициализирован
	if(this->_cluster != nullptr)
		// Выполняем установку типа протокола передачи данных между воркерами
		this->_cluster->setTypeEventMessage(type);
}
/**
 * @brief Метод получения размера буфера события
 *
 * @param pid    идентификатор процесса
 * @param action тип действия события
 * @return       размер буфера события
 *
 */
size_t awh::unit::Server::clusterGetBufferSize(const pid_t pid, const event::action_t action) const noexcept {
	// Если кластер инициализирован
	if(this->_cluster != nullptr)
		// Выполняем получение размера буфера кластера для события кластера
		return this->_cluster->getBufferSize(pid, action);
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки размера буфера события
 *
 * @param pid    идентификатор процесса
 * @param action тип действия события
 * @param size   размер буфера события
 * @return       результат выполнения установки
 *
 */
bool awh::unit::Server::clusterSetBufferSize(const pid_t pid, const event::action_t action, const size_t size) noexcept {
	// Если кластер инициализирован
	if(this->_cluster != nullptr)
		// Выполняем установку размера буфера кластера для события кластера
		return this->_cluster->setBufferSize(pid, action, size);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
awh::unit::Server::Server(const fmk_t * fmk, const log_t * log) noexcept : unit_t(fmk, log), _cluster(nullptr)
/**
 * Для операционной системы MS Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	, _handoverNext(0)
#endif
{}
/**
 * @brief Деструктор
 *
 */
awh::unit::Server::~Server() noexcept {
	// Если в списке событий сервера есть события
	if(!this->_events.empty()){
		/**
		 * Выполняем удаление всех событий сервера
		 */
		for(const auto & event : this->_events){
			// Снимаем функцию обратного вызова на событие получения ошибок клиента
			this->_io->on(event.first, static_cast <engine::callback::error_t> (nullptr));
			// Снимаем функцию обратного вызова на событие изменения действий клиента
			this->_io->on(event.first, static_cast <engine::callback::event_t> (nullptr));
			// Снимаем функцию обратного вызова на событие записи данных клиенту
			this->_io->on(event.first, static_cast <engine::callback::write_t> (nullptr));
			// Снимаем функцию обратного вызова на событие неотправленных данных клиенту
			this->_io->on(event.first, static_cast <engine::callback::spool_t> (nullptr));
			// Снимаем функцию обратного вызова на событие изменения статуса клиента
			this->_io->on(event.first, static_cast <engine::callback::status_t> (nullptr));
			// Снимаем функцию обратного вызова на событие доступности/недоступности очереди исходящих данных клиента
			this->_io->on(event.first, static_cast <engine::callback::available_t> (nullptr));
			/**
			 * Определяем узел к которому относится событие
			 */
			switch(static_cast <uint8_t> (this->_io->node(event.first))){
				// Если узел события является одноразовым узлом
				case static_cast <uint8_t> (event::node_t::PEER): {
					// Снимаем функцию обратного вызова на событие чтения данных клиента
					this->_io->on(event.first, static_cast <engine::callback::read_t> (nullptr));
					// Снимаем функцию обратного вызова на событие истечения таймаута клиента
					this->_io->on(event.first, static_cast <engine::callback::timeout_t> (nullptr));
				} break;
				// Если узел события является серверным узлом
				case static_cast <uint8_t> (event::node_t::SERVER): {
					// Снимаем функцию обратного вызова на событие разрешения подключения
					this->_io->on(event.first, static_cast <engine::callback::accept_t> (nullptr));
					// Снимаем функцию обратного вызова на событие получения информационных метаданных о дейтаграммном пакете
					this->_io->on(event.first, static_cast <engine::callback::traffic_t> (nullptr));
				} break;
			}
			// Удаляем событие сервера
			this->_io->destroy(event.first);
		}
		// Очищаем внутренние индексы событий
		this->_events.clear();
		// Очищаем список клиентов сервера
		this->_serverClients.clear();
		// Очищаем позиции клиентов в индексе
		this->_clientPositions.clear();
	}
}
