/**
 * @file: server.cpp
 * @date: 2026-03-22
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

/**
 * Подключаем заголовочный файл модуля
 */
#include <units/server.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён плейсхолдеров
 */
using namespace placeholders;

/**
 * @brief Метод запуска/остановки работы сервера
 *
 * @param status статус запуска/остановки сервера
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
				// Если функция обратного вызова установлена
				if(this->_callback.is("server_status"))
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const event::status_t)> ("server_status", status);
			/**
			 * Для операционной системы Linux или FreeBSD
			 */
			#elif __linux__ || __FreeBSD__
				// Если кластер в работе не используется или если процесс является дочерним
				if((this->_clusterParams.mode == event::mode_t::DISABLED) || !this->_cluster->master()){
					// Если функция обратного вызова установлена
					if(this->_callback.is("server_status"))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const event::status_t)> ("server_status", status);
				}
			/**
			 * Для операционной системы OpenBSD, NetBSD, Sun Solaris или MacOS X
			 */
			#elif __OpenBSD__ || ___NetBSD__ || __sun__ || __APPLE__ || __MACH__
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
					case static_cast <uint8_t> (event::mode_t::DISABLED): {
						// Если функция обратного вызова установлена
						if(this->_callback.is("server_status"))
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const event::status_t)> ("server_status", status);
					} break;
				}
			#endif
		} break;
		// Если работа кластера подлежит уничтожению
		case static_cast <uint8_t> (event::status_t::DESTROYED): {
			{
				// Выполняем блокировку потока для работы временным списком событий сервера
				const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
				// Если в списке событий сервера есть события
				if(!this->_events.empty()){
					// Выполняем удаление всех событий сервера
					for(const auto & eid : this->_events){
						// Выполняем блокировку потока для уничтожения событий
						const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
						// Удаляем событие сервера
						this->_io->destroy(eid);
					}
				}
			}
			// Если функция обратного вызова установлена
			if(this->_callback.is("server_status")){
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const event::status_t)> ("server_status", status);
				// Выполняем получение функции обратного вызова
				this->_callback.set("server_status", "status", this->_callback);
			}
			/**
			 * Для операционной системы OpenBSD, NetBSD, Sun Solaris или MacOS X
			 */
			#if __OpenBSD__ || ___NetBSD__ || __sun__ || __APPLE__ || __MACH__
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
 */
void awh::unit::Server::rebase(const pid_t old, const pid_t pid) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("cluster_rebase"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const pid_t, const pid_t)> ("cluster_rebase", old, pid);
}
/**
 * @brief Метод получения события завершения работы процесса
 *
 * @param pid    идентификатор процесса
 * @param signal сигнал с которым завершился процесс
 */
void awh::unit::Server::exit(const pid_t pid, const int32_t signal) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("cluster_exit"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const pid_t, const int32_t)> ("cluster_exit", pid, signal);
}
/**
 * @brief Метод обработки события отправки сообщения процессу кластера
 *
 * @param pid  идентификатор процесса
 * @param size размер отправленного сообщения
 */
void awh::unit::Server::sending(const pid_t pid, const size_t size) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("cluster_sending"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const pid_t, const size_t)> ("cluster_sending", pid, size);
}
/**
 * @brief Метод обработки событий записи данных сервером
 *
 * @param eid  идентификатор события
 * @param size размер данных для записи
 */
void awh::unit::Server::write(const event::id_t eid, const size_t size) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("write"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::id_t, const size_t)> ("write", eid, size);
}
/**
 * @brief Метод обработки события разрешения подключения
 *
 * @param eid идентификатор сервера
 * @param cid идентификатор клиента
 */
void awh::unit::Server::accept(const event::id_t eid, const event::id_t cid) noexcept {
	{
		// Выполняем блокировку потока для работы с событием сервера
		const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
		// Устанавливаем функцию обратного вызова на событие истечения таймаута клиента
		this->_io->on(cid, static_cast <engine::callback::timeout_t> (std::bind(&server_t::timeout, this, _1, _2, _3)));
		// Устанавливаем функцию обратного вызова на событие изменения действий клиента
		this->_io->on(cid, static_cast <engine::callback::event_t> (std::bind(&server_t::action, this, _1, _2)));
		// Устанавливаем функцию обратного вызова на событие записи данных клиенту
		this->_io->on(cid, static_cast <engine::callback::write_t> (std::bind(&server_t::write, this, _1, _2)));
		// Устанавливаем функцию обратного вызова на событие чтения данных клиента
		this->_io->on(cid, static_cast <engine::callback::read_t> (std::bind(&server_t::read, this, _1, _2, _3)));
		// Устанавливаем функцию обратного вызова на событие неотправленных данных клиенту
		this->_io->on(cid, static_cast <engine::callback::spool_t> (std::bind(&server_t::spool, this, _1, _2, _3, _4)));
		// Устанавливаем функцию обратного вызова на событие изменения статуса клиента
		this->_io->on(cid, static_cast <engine::callback::status_t> (std::bind(static_cast <void (server_t::*)(const event::id_t, const event::status_t)> (&server_t::status), this, _1, _2)));
		// Устанавливаем функцию обратного вызова на событие получения ошибок клиента
		this->_io->on(cid, static_cast <engine::callback::error_t> (std::bind(static_cast <void (server_t::*)(const event::id_t, const event::error_t, const string &)> (&server_t::error), this, _1, _2, _3)));
		// Устанавливаем функцию обратного вызова на событие доступности/недоступности очереди исходящих данных клиента
		this->_io->on(cid, static_cast <engine::callback::available_t> (std::bind(static_cast <void (server_t::*)(const event::id_t, const event::status_t, const size_t)> (&server_t::available), this, _1, _2, _3)));
	}{
		// Выполняем блокировку потока для работы временным списком событий сервера
		const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
		// Добавляем идентификатор события однорангового узла в список событий сервера
		this->_events.emplace(cid);
	}
	// Если функция обратного вызова установлена
	if(this->_callback.is("accept"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::id_t, const event::id_t)> ("accept", eid, cid);
}
/**
 * @brief Метод обработки действий сервера
 *
 * @param eid    идентификатор события
 * @param action действие сервера
 */
void awh::unit::Server::action(const event::id_t eid, const event::action_t action) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("action"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::id_t, const event::action_t)> ("action", eid, action);
}
/**
 * @brief Метод обработки событий изменения статуса кластера
 *
 * @param pid    идентификатор события
 * @param status новый статус кластера
 */
void awh::unit::Server::status(const pid_t pid, const event::status_t status) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("cluster_state"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const pid_t, const event::status_t)> ("cluster_state", pid, status);
}
/**
 * @brief Метод обработки событий изменения статуса сервера
 *
 * @param eid    идентификатор события
 * @param status новый статус сервера
 */
void awh::unit::Server::status(const event::id_t eid, const event::status_t status) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("state"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::id_t, const event::status_t)> ("state", eid, status);
}
/**
 * @brief Метод получения событий активации/деактивации кластера
 *
 * @param pid   идентификатор процесса
 * @param event флаг события кластера
 */
void awh::unit::Server::cluster(const pid_t pid, const unit::cluster_t::event_t event) noexcept {
	/**
	 * Определяем полученное событие кластера
	 */
	switch(static_cast <uint8_t> (event)){
		// Если событие представляет из себя запуск процесса
		case static_cast <uint8_t> (unit::cluster_t::event_t::START): {
			/**
			 * Для операционной системы Linux или FreeBSD
			 */
			#if __linux__ || __FreeBSD__
				// Если работа юнита ещё не запущена
				if(!this->working()){
					// Если функция обратного вызова установлена
					if(this->_callback.is("status"))
						// Выполняем получение функции обратного вызова
						this->_callback.set("status", "server_status", this->_callback);
					// Устанавливаем функцию обратного вызова на запуск системы
					this->_callback.on <void (const event::status_t)> ("status", static_cast <void (server_t::*)(const event::status_t)> (&server_t::launch), this, _1);
					// Выполняем запуск работы основного юнита
					unit_t::start();
				}
			/**
			 * Для операционной системы OpenBSD, NetBSD, Sun Solaris или MacOS X
			 */
			#elif __OpenBSD__ || ___NetBSD__ || __sun__ || __APPLE__ || __MACH__
				// Если процесс является дочерним
				if(!this->_cluster->master()){
					// Если функция обратного вызова установлена
					if(this->_callback.is("server_status"))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const event::status_t)> ("server_status", event::status_t::LAUNCHED);
				}
			#endif
		} break;
		// Если событие представляет из себя остановку процесса
		case static_cast <uint8_t> (unit::cluster_t::event_t::STOP): {
			/**
			 * Для операционной системы Linux или FreeBSD
			 */
			#if __linux__ || __FreeBSD__
				// Если работа юнита запущена
				if(this->working())
					// Останавливаем работу основного юнита
					unit_t::stop();
			#endif
		} break;
	}
	// Если функция получения событий кластера установлена
	if(this->_callback.is("cluster_events"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const pid_t, const unit::cluster_t::event_t)>  ("cluster_events", pid, event);
}
/**
 * @brief Метод обработки события получения сообщения от процесса кластера
 *
 * @param pid  идентификатор процесса
 * @param data данные полученного сообщения
 * @param size размер данных полученного сообщения
 */
void awh::unit::Server::message(const pid_t pid, const uint8_t * data, const size_t size) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("cluster_message"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const pid_t, const uint8_t *, const size_t)> ("cluster_message", pid, data, size);
}
/**
 * @brief Метод обработки событий получения данных сервером
 *
 * @param eid  идентификатор события
 * @param data данные события получения данных сервером
 * @param size размер данных события получения данных сервером
 */
void awh::unit::Server::read(const event::id_t eid, const uint8_t * data, const size_t size) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("read"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::id_t, const uint8_t *, const size_t)> ("read", eid, data, size);
}
/**
 * @brief Метод обработки события доступности/недоступности очереди исходящих сообщений кластера
 *
 * @param pid    идентификатор процесса
 * @param status статус доступности очереди
 * @param size   размер доступных данных очереди
 */
void awh::unit::Server::available(const pid_t pid, const event::status_t status, const size_t size) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("cluster_available"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const pid_t, const event::status_t, const size_t)> ("cluster_available", pid, status, size);
}
/**
 * @brief Метод обработки события доступности/недоступности очереди исходящих данных сервера
 *
 * @param eid    идентификатор события
 * @param status статус доступности очереди
 * @param size   размер доступных данных очереди
 */
void awh::unit::Server::available(const event::id_t eid, const event::status_t status, const size_t size) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("available"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::id_t, const event::status_t, const size_t)> ("available", eid, status, size);
}
/**
 * @brief Метод обработки событий истечения таймаута подключённого клиента
 *
 * @param eid    идентификатор подключённого клиента
 * @param action тип действия для истекшего таймаута
 * @param delay  задержка таймаута в миллисекундах
 * @return       нужно ли завершить клиента после истечения таймаута
 */
bool awh::unit::Server::timeout(const event::id_t eid, const event::action_t action, const uint32_t delay) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("timeout"))
		// Выполняем функцию обратного вызова
		return this->_callback.call <bool (const event::id_t, const event::action_t, const uint32_t)> ("timeout", eid, action, delay);
	// Возвращаем значение, указывающее на то, что клиента нужно завершить после истечения таймаута
	return true;
}
/**
 * @brief Метод обработки событий ошибок кластера
 *
 * @param pid         идентификатор процесса
 * @param error       тип ошибки
 * @param description описание ошибки
 */
void awh::unit::Server::error(const pid_t pid, const event::error_t error, const string & description) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("cluster_error"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const pid_t, const event::error_t, const string &)> ("cluster_error", pid, error, description);
}
/**
 * @brief Метод обработки событий ошибок сервера
 *
 * @param eid         идентификатор события
 * @param error       тип ошибки
 * @param description описание ошибки
 */
void awh::unit::Server::error(const event::id_t eid, const event::error_t error, const string & description) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("error"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", eid, error, description);
}
/**
 * @brief Метод обработки события неотправленных данных сервера
 *
 * @param eid   идентификатор события
 * @param error тип ошибки отправки данных
 * @param data  данные, которые не получилось отправить
 * @param size  размер данных, которые не получилось отправить
 */
void awh::unit::Server::spool(const event::id_t eid, const event::send_error_t error, const uint8_t * data, const size_t size) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("spool"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::id_t, const event::send_error_t, const uint8_t *, const size_t)> ("spool", eid, error, data, size);
}
/**
 * @brief Метод проверки актуальности события
 *
 * @param eid идентификатор события
 * @return    результат проверки актуальности события
 */
bool awh::unit::Server::isActual(const event::id_t eid) const noexcept {
	// Выполняем блокировку потока для работы временным списком событий сервера
	const locker_t <std::shared_mutex> lock(const_cast <server_t *> (this)->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
	// Проверяем есть ли событие сервера в списке событий сервера
	return this->_events.find(eid) != this->_events.end();
}
/**
 * @brief Метод очистки чёрного списка события
 *
 * @param eid идентификатор события
 * @return    результат выполнения очистки
 */
bool awh::unit::Server::clearBlacklist(const event::id_t eid) noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Выполняем очистку чёрного списка события
	return this->_io->blacklist.clear(eid);
}
/**
 * @brief Метод очистки белого списка события
 *
 * @param eid идентификатор события
 * @return    результат выполнения очистки
 */
bool awh::unit::Server::clearWhitelist(const event::id_t eid) noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Выполняем очистку белого списка события
	return this->_io->whitelist.clear(eid);
}
/**
 * @brief Метод добавления адреса в чёрный список события
 *
 * @param eid   идентификатор события
 * @param value значение адреса события
 * @return      результат выполнения установки
 */
bool awh::unit::Server::addToBlacklist(const event::id_t eid, string_view value) noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Выполняем добавление адреса в чёрный список события
	return this->_io->blacklist.add(eid, value);
}
/**
 * @brief Метод добавления адреса в белый список события
 *
 * @param eid   идентификатор события
 * @param value значение адреса события
 * @return      результат выполнения установки
 */
bool awh::unit::Server::addToWhitelist(const event::id_t eid, string_view value) noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Выполняем добавление адреса в белый список события
	return this->_io->whitelist.add(eid, value);
}
/**
 * @brief Метод удаления адреса из чёрного списка события
 *
 * @param eid   идентификатор события
 * @param value адрес для удаления из чёрного списка
 * @return      результат выполнения удаления
 */
bool awh::unit::Server::removeFromBlacklist(const event::id_t eid, string_view value) noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Выполняем удаление адреса из чёрного списка события
	return this->_io->blacklist.remove(eid, value);
}
/**
 * @brief Метод удаления адреса из белого списка события
 *
 * @param eid   идентификатор события
 * @param value адрес для удаления из белого списка
 * @return      результат выполнения удаления
 */
bool awh::unit::Server::removeFromWhitelist(const event::id_t eid, string_view value) noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Выполняем удаление адреса из белого списка события
	return this->_io->whitelist.remove(eid, value);
}
/**
 * @brief Метод получения чёрного списка события
 *
 * @param eid идентификатор события
 * @return    чёрный список события
 */
const std::unordered_map <string, awh::event::address_t> & awh::unit::Server::getFromBlacklist(const event::id_t eid) const noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::SHARED);
	// Возвращаем чёрный список события
	return this->_io->blacklist.get(eid);
}
/**
 * @brief Метод получения белого списка события
 *
 * @param eid идентификатор события
 * @return    белый список события
 */
const std::unordered_map <string, awh::event::address_t> & awh::unit::Server::getFromWhitelist(const event::id_t eid) const noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::SHARED);
	// Возвращаем белый список события
	return this->_io->whitelist.get(eid);
}
/**
 * @brief Метод установки безопасности работы потоков
 *
 * @param mode флаг режима безопасности потоков
 */
void awh::unit::Server::threadSafety(const bool mode) noexcept {
	// Устанавливаем режим безопасности работы потоков для родительского юнита
	unit_t::threadSafety(mode);
	// Устанавливаем режим безопасности работы потоков для объекта блокировки
	this->_mtx.enabled = mode;
}
/**
 * @brief Метод фиксации настроек сервера
 *
 * @param eid идентификатор события сервера
 * @return    результат выполнения фиксации
 */
bool awh::unit::Server::commit(const event::id_t eid) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		{
			// Выполняем блокировку потока для работы с событием сервера
			const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			/**
			 * Для операционной системы Linux или FreeBSD
			 */
			#if __linux__ || __FreeBSD__
				// Если кластер активен, значит нам необходимо проверить опции сервера
				if(this->_clusterParams.mode == event::mode_t::ENABLED){
					// Если не установлена опция переиспользования портов
					if(!(this->_io->getOptions(eid) & event::options::REUSE_PORT))
						// Выполняем установку опции для события сервера
						this->_io->setOption(eid, event::options::REUSE_PORT, true);
				}
			#endif
			// Выполняем фиксацию параметров события
			result = this->_io->commit(eid);
		}
		// Если фиксация параметров события не удалась
		if(!result){
			{
				// Выполняем блокировку потока для работы с событием сервера
				const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
				// Удаляем событие сервера
				this->_io->destroy(eid);
			}{
				// Выполняем блокировку потока для работы временным списком событий сервера
				const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
				// Выполняем поиск идентификатора события сервера в списке событий сервера
				auto i = this->_events.find(eid);
				// Если идентификатор события сервера найден в списке событий сервера
				if(i != this->_events.end())
					// Удаляем идентификатор события сервера из списка событий сервера
					this->_events.erase(i);
			}
			// Если функция обратного вызова не установлена
			if(!this->_callback.is("error")){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Failed to commit server", __PRETTY_FUNCTION__, std::make_tuple(eid), log_t::flag_t::CRITICAL);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Failed to commit server", log_t::flag_t::CRITICAL);
				#endif
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(eid), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод запуска работы сервера
 *
 * @param eid идентификатор события сервера
 * @return    результат выполнения запуска
 */
bool awh::unit::Server::launch(const event::id_t eid) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		{
			// Выполняем блокировку потока для работы с событием сервера
			const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Выполняем запуск работы сервера
			result = this->_io->launch(eid);
		}
		// Если запуск работы сервера не удался
		if(!result){
			// Если функция обратного вызова не установлена
			if(!this->_callback.is("error")){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Failed to launch server", __PRETTY_FUNCTION__, std::make_tuple(eid), log_t::flag_t::CRITICAL);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Failed to launch server", log_t::flag_t::CRITICAL);
				#endif
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(eid), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод приостановки работы клиента
 *
 * @param eid идентификатор события клиента
 * @return    результат выполнения приостановки работы
 */
bool awh::unit::Server::pause(const event::id_t eid) noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Выполняем приостановку работы сервера
	return this->_io->pause(eid);
}
/**
 * @brief Метод возобновления работы клиента
 *
 * @param eid идентификатор события клиента
 * @return    результат выполнения возобновления работы
 */
bool awh::unit::Server::resume(const event::id_t eid) noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Выполняем возобновление работы сервера
	return this->_io->resume(eid);
}
/**
 * @brief Метод перевода события в режим прослушивания входящих соединений
 *
 * @param eid идентификатор события сервера
 * @param max максимальное количество входящих соединений
 * @return    результат выполнения перевода в режим прослушивания
 */
bool awh::unit::Server::listen(const event::id_t eid, const uint16_t max) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		{
			// Выполняем блокировку потока для работы с событием сервера
			const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Выполняем прослушивание порта сервера для получения входящих подключений
			result = this->_io->listen(eid, max);
		}
		// Если прослушивание порта сервера для получения входящих подключений не удалось
		if(!result){
			{
				// Выполняем блокировку потока для работы с событием сервера
				const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
				// Удаляем событие сервера
				this->_io->destroy(eid);
			}{
				// Выполняем блокировку потока для работы временным списком событий сервера
				const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
				// Выполняем поиск идентификатора события сервера в списке событий сервера
				auto i = this->_events.find(eid);
				// Если идентификатор события сервера найден в списке событий сервера
				if(i != this->_events.end())
					// Удаляем идентификатор события сервера из списка событий сервера
					this->_events.erase(i);
			}
			// Если функция обратного вызова не установлена
			if(!this->_callback.is("error")){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Failed to launch server", __PRETTY_FUNCTION__, std::make_tuple(eid, max), log_t::flag_t::CRITICAL);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Failed to launch server", log_t::flag_t::CRITICAL);
				#endif
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(eid, max), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод получения данных от клиента
 *
 * @param eid идентификатор события клиента
 * @return    результат получения данных
 */
bool awh::unit::Server::recv(const event::id_t eid) noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Выполняем получение данных от клиента
	return this->_io->recv(eid);
}
/**
 * @brief Метод отправки данных клиенту
 *
 * @param eid    идентификатор события клиента
 * @param buffer буфер данных для отправки
 * @param size   размер данных для отправки
 * @return       количество байт данных, отправленных клиенту
 */
size_t awh::unit::Server::send(const event::id_t eid, const void * buffer, const size_t size) noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Выполняем отправку данных клиенту
	return this->_io->send(eid, buffer, size);
}
/**
 * @brief Метод объединения данных между сервером и другим событием
 *
 * @param eid  идентификатор события-источника
 * @param dest идентификатор события-приёмника
 * @return     результат выполнения объединения
 */
bool awh::unit::Server::splice(const event::id_t eid, const event::id_t dest) noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Выполняем объединение данных между событием сервера и другим событием
	return this->_io->splice(eid, dest);
}
/**
 * @brief Метод получения опций сервера
 *
 * @param eid идентификатор события сервера
 * @return    опции сервера
 */
uint16_t awh::unit::Server::getOptions(const event::id_t eid) const noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::SHARED);
	// Выполняем получение опций для события сервера
	return this->_io->getOptions(eid);
}
/**
 * @brief Метод установки опций сервера
 *
 * @param eid     идентификатор события сервера
 * @param options опции сервера для установки
 * @return        результат выполнения установки
 */
bool awh::unit::Server::setOptions(const event::id_t eid, const uint16_t options) noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Выполняем установку опций для события сервера
	return this->_io->setOptions(eid, options);
}
/**
 * @brief Метод установки опции сервера
 *
 * @param eid    идентификатор события сервера
 * @param option опция сервера для установки
 * @param mode   режим установки опции сервера
 * @return       результат выполнения установки
 */
bool awh::unit::Server::setOption(const event::id_t eid, const uint16_t option, const bool mode) noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Выполняем установку опции для события сервера
	return this->_io->setOption(eid, option, mode);
}
/**
 * @brief Метод получения максимального количества хопов, через которые может пройти пакет
 *
 * @param eid идентификатор события сервера
 * @return    максимальное количество хопов
 */
awh::event::hops_t awh::unit::Server::getHops(const event::id_t eid) const noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::SHARED);
	// Выполняем получение максимального количества хопов для события сервера
	return this->_io->getHops(eid);
}
/**
 * @brief Метод установки максимального количества хопов, через которые может пройти пакет
 *
 * @param eid  идентификатор события сервера
 * @param hops максимальное количество хопов
 * @return     результат работы функции
 */
bool awh::unit::Server::setHops(const event::id_t eid, const event::hops_t hops) noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Выполняем установку максимального количества хопов для события сервера
	return this->_io->setHops(eid, this->_io->family(eid), hops);
}
/**
 * @brief Метод получения сетевого интерфейса сервера
 *
 * @param eid идентификатор события сервера
 * @return    сетевой интерфейс сервера
 */
string awh::unit::Server::getIface(const event::id_t eid) const noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::SHARED);
	// Выполняем получение сетевого интерфейса для события сервера
	return this->_io->getIface(eid);
}
/**
 * @brief Метод установки сетевого интерфейса сервера
 *
 * @param eid  идентификатор события сервера
 * @param name имя сетевого интерфейса для установки
 * @return     результат выполнения установки
 */
bool awh::unit::Server::setIface(const event::id_t eid, string_view name) noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Выполняем установку сетевого интерфейса для события сервера
	return this->_io->setIface(eid, name);
}
/**
 * @brief Метод получения порта удаленного сервера
 *
 * @param eid идентификатор события сервера
 * @return    порт удаленного сервера
 */
uint16_t awh::unit::Server::getPort(const event::id_t eid) const noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::SHARED);
	// Выполняем получение порта удаленного сервера для события сервера
	return this->_io->getPort(eid);
}
/**
 * @brief Метод установки порта удаленного сервера
 *
 * @param eid  идентификатор события сервера
 * @param port порт удаленного сервера для установки
 * @return     результат выполнения установки
 */
bool awh::unit::Server::setPort(const event::id_t eid, const uint16_t port) noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Выполняем установку порта удаленного сервера для события сервера
	return this->_io->setPort(eid, port);
}
/**
 * @brief Метод получения адреса сервера
 *
 * @param eid     идентификатор события сервера
 * @param address тип адреса сервера
 * @return        значение адреса сервера
 */
string awh::unit::Server::getAddress(const event::id_t eid, const event::address_t address) const noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::SHARED);
	// Выполняем получение адреса сервера для события сервера
	return this->_io->getAddress(eid, address);
}
/**
 * @brief Метод установки адреса сервера
 *
 * @param eid     идентификатор события сервера
 * @param address тип адреса сервера
 * @param value   значение адреса сервера
 * @return        результат выполнения установки
 */
bool awh::unit::Server::setAddress(const event::id_t eid, const event::address_t address, string_view value) noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Выполняем установку адреса сервера для события сервера
	return this->_io->setAddress(eid, address, value);
}
/**
 * @brief Метод установки адреса сервера
 *
 * @param eid     идентификатор события сервера
 * @param address тип адреса сервера
 * @param value   значение адреса сервера
 * @return        результат выполнения установки
 */
bool awh::unit::Server::setAddress(const event::id_t eid, const event::address_t address, const net::addr_t * value) noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Выполняем установку адреса сервера для события сервера
	return this->_io->setAddress(eid, address, value);
}
/**
 * @brief Метод получения адреса сервера
 *
 * @param eid     идентификатор события сервера
 * @param address тип адреса сервера
 * @param value   объект для извлечения адреса сервера
 * @return        результат выполнения извлечения адреса сервера
 */
bool awh::unit::Server::getAddress(const event::id_t eid, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::SHARED);
	// Выполняем получение адреса сервера для события сервера
	return this->_io->getAddress(eid, address, value);
}
/**
 * @brief Метод получения MTU сетевого интерфейса
 *
 * @param eid идентификатор события сервера
 * @return    MTU сетевого интерфейса
 */
uint16_t awh::unit::Server::getMaximumTransmissionUnit(const event::id_t eid) const noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::SHARED);
	// Выполняем получение MTU сетевого интерфейса для события сервера
	return this->_io->getMaximumTransmissionUnit(eid);
}
/**
 * @brief Метод установки MTU сетевого интерфейса
 *
 * @param eid идентификатор события сервера
 * @param mtu размер MTU интерфейса
 * @return    результат установки MTU сетевого интерфейса
 */
bool awh::unit::Server::setMaximumTransmissionUnit(const event::id_t eid, const uint16_t mtu) const noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Выполняем установку MTU сетевого интерфейса для события сервера
	return this->_io->setMaximumTransmissionUnit(eid, mtu);
}
/**
 * @brief Метод получения режима трансляции пакетов сервера
 *
 * @param eid идентификатор события сервера
 * @return    режим трансляции пакетов (unicast, multicast, broadcast)
 */
awh::event::delivery_mode_t awh::unit::Server::getDelivery(const event::id_t eid) const noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::SHARED);
	// Выполняем получение режима трансляции пакетов сервера для события сервера
	return this->_io->getDelivery(eid);
}
/**
 * @brief Метод установки режима трансляции пакетов сервера
 *
 * @param eid      идентификатор события сервера
 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
 * @return         результат выполнения установки
 */
bool awh::unit::Server::setDelivery(const event::id_t eid, const event::delivery_mode_t delivery) noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Выполняем установку режима трансляции пакетов сервера для события сервера
	return this->_io->setDelivery(eid, delivery);
}
/**
 * @brief Метод получения размера буфера сервера
 *
 * @param eid    идентификатор события сервера
 * @param action тип действия сервера
 * @return       размер буфера сервера
 */
size_t awh::unit::Server::getBufferSize(const event::id_t eid, const event::action_t action) const noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::SHARED);
	// Выполняем получение размера буфера сервера для события сервера
	return this->_io->getBufferSize(eid, action);
}
/**
 * @brief Метод установки размера буфера сервера
 *
 * @param eid    идентификатор события сервера
 * @param action тип действия сервера
 * @param size   размер буфера сервера
 * @return       результат выполнения установки
 */
bool awh::unit::Server::setBufferSize(const event::id_t eid, const event::action_t action, const size_t size) noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Выполняем установку размера буфера сервера для события сервера
	return this->_io->setBufferSize(eid, action, size);
}
/**
 * @brief Метод получения режима использования таймаута на чтение события
 *
 * @param eid идентификатор события
 * @return    режим использования таймаута на чтение события
 */
awh::event::usage_t awh::unit::Server::getUsageReadTimeout(const event::id_t eid) const noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::SHARED);
	// Выполняем получение режима использования таймаута на чтение события для события сервера
	return this->_io->getUsageReadTimeout(eid);
}
/**
 * @brief Метод установки режима использования таймаута на чтение события
 *
 * @param eid   идентификатор события
 * @param usage режим использования таймаута на чтение события (reusable или disposable)
 */
void awh::unit::Server::setUsageReadTimeout(const event::id_t eid, const event::usage_t usage) noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Выполняем установку режима использования таймаута на чтение события для события сервера
	this->_io->setUsageReadTimeout(eid, usage);
}
/**
 * @brief Метод получения таймаута сервера
 *
 * @param eid    идентификатор события сервера
 * @param action тип действия сервера
 * @return       значение таймаута в миллисекундах
 */
uint32_t awh::unit::Server::getTimeout(const event::id_t eid, const event::action_t action) const noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::SHARED);
	// Выполняем получение параметров таймаута для сервера
	return this->_io->getTimeout(eid, action);
}
/**
 * @brief Метод установки таймаута сервера
 *
 * @param eid     идентификатор события сервера
 * @param action  тип действия сервера
 * @param timeout значение таймаута в миллисекундах
 */
void awh::unit::Server::setTimeout(const event::id_t eid, const event::action_t action, const uint32_t timeout) noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
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
 */
bool awh::unit::Server::bandwidth(const event::id_t eid, const event::limiting_t limiting, string_view bandwidth) noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Выполняем установку параметров пропускной способности для сервера
	return this->_io->bandwidth(eid, limiting, bandwidth);
}
/**
 * @brief Метод установки параметров keep-alive для сервера
 *
 * @param eid   идентификатор события сервера
 * @param cnt   количество пакетов keep-alive
 * @param idle  время простоя перед отправкой первого пакета keep-alive в секундах
 * @param intvl интервал между пакетами keep-alive в секундах
 * @return      результат выполнения установки
 */
bool awh::unit::Server::keepAlive(const event::id_t eid, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Выполняем установку параметров keep-alive для сервера
	return this->_io->keepAlive(eid, cnt, idle, intvl);
}
/**
 * @brief Метод получения значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
 *
 * @param eid идентификатор события сервера
 * @return    значение DSCP
 */
awh::event::dscp_t awh::unit::Server::getDifferentiatedServicesCodePoint(const event::id_t eid) const noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::SHARED);
	// Выполняем получение значения поля Differentiated Services Code Point (DSCP) для сервера
	return this->_io->getDifferentiatedServicesCodePoint(eid, this->_io->family(eid));
}
/**
 * @brief Метод установки значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
 *
 * @param eid  идентификатор события сервера
 * @param dscp значение DSCP
 * @return     результат работы функции
 */
bool awh::unit::Server::setDifferentiatedServicesCodePoint(const event::id_t eid, const event::dscp_t dscp) const noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Выполняем установку значения поля Differentiated Services Code Point (DSCP) для сервера
	return this->_io->setDifferentiatedServicesCodePoint(eid, this->_io->family(eid), dscp);
}
/**
 * @brief Метод получения обнаружения максимального размера пакета (MTU)
 *
 * @param eid идентификатор события сервера
 * @return    режим обнаружения максимального размера пакета (MTU)
 */
awh::event::mtu_discover_t awh::unit::Server::getMaximumTransmissionUnitDiscover(const event::id_t eid) const noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::SHARED);
	// Выполняем получение обнаружения максимального размера пакета (MTU) для сервера
	return this->_io->getMaximumTransmissionUnitDiscover(eid, this->_io->family(eid));
}
/**
 * @brief Метод установки обнаружения максимального размера пакета (MTU)
 *
 * @param eid  идентификатор события сервера
 * @param mode режим обнаружения максимального размера пакета (MTU)
 * @return     результат работы функции
 */
bool awh::unit::Server::setMaximumTransmissionUnitDiscover(const event::id_t eid, const event::mtu_discover_t mode) const noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Выполняем установку обнаружения максимального размера пакета (MTU) для сервера
	return this->_io->setMaximumTransmissionUnitDiscover(eid, this->_io->family(eid), mode);
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
 */
bool awh::unit::Server::membership(const event::id_t eid, const event::mode_t mode, string_view group, string_view source, const uint16_t port) noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Выполняем активацию/деактивацию мультикаст группы для сервера
	return this->_io->membership(eid, mode, group, source, port);
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
 */
bool awh::unit::Server::membership(const event::id_t eid, const event::mode_t mode, const net::addr_t * group, const net::addr_t * source, const uint16_t port) noexcept {
	// Выполняем блокировку потока для работы с событием сервера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Выполняем активацию/деактивацию мультикаст группы для сервера
	return this->_io->membership(eid, mode, group, source, port);
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
			// Выполняем остановку работы основного юнита
			unit_t::stop();
		/**
		 * Для операционной системы Linux или FreeBSD
		 */
		#elif __linux__ || __FreeBSD__
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
				// Если необходимо кластер в работе не используется
				case static_cast <uint8_t> (event::mode_t::DISABLED):
					// Выполняем остановку работы основного юнита
					unit_t::stop();
				break;
			}
		/**
		 * Для операционной системы OpenBSD, NetBSD, Sun Solaris или MacOS X
		 */
		#elif __OpenBSD__ || ___NetBSD__ || __sun__ || __APPLE__ || __MACH__
			// Выполняем остановку работы основного юнита
			unit_t::stop();
		/**
		 * Операционной системой не распознана
		 */
		#else
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке запуска события
				this->_log->debug("This operating system is not supported", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке запуска события
				this->_log->print("This operating system is not supported", log_t::flag_t::CRITICAL);
			#endif
			// Выходим из приложения
			::exit(EXIT_FAILURE);
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
			// Если функция обратного вызова установлена
			if(this->_callback.is("status"))
				// Выполняем получение функции обратного вызова
				this->_callback.set("status", "server_status", this->_callback);
			// Устанавливаем функцию обратного вызова на запуск системы
			this->_callback.on <void (const event::status_t)> ("status", static_cast <void (server_t::*)(const event::status_t)> (&server_t::launch), this, _1);
			// Выполняем запуск работы основного юнита
			unit_t::start();
		/**
		 * Для операционной системы Linux или FreeBSD
		 */
		#elif __linux__ || __FreeBSD__
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
				// Если необходимо кластер в работе не используется
				case static_cast <uint8_t> (event::mode_t::DISABLED): {
					// Если функция обратного вызова установлена
					if(this->_callback.is("status"))
						// Выполняем получение функции обратного вызова
						this->_callback.set("status", "server_status", this->_callback);
					// Устанавливаем функцию обратного вызова на запуск системы
					this->_callback.on <void (const event::status_t)> ("status", static_cast <void (server_t::*)(const event::status_t)> (&server_t::launch), this, _1);
					// Выполняем запуск работы основного юнита
					unit_t::start();
				} break;
			}
		/**
		 * Для операционной системы OpenBSD, NetBSD, Sun Solaris или MacOS X
		 */
		#elif __OpenBSD__ || ___NetBSD__ || __sun__ || __APPLE__ || __MACH__
			// Если функция обратного вызова установлена
			if(this->_callback.is("status"))
				// Выполняем получение функции обратного вызова
				this->_callback.set("status", "server_status", this->_callback);
			// Устанавливаем функцию обратного вызова на запуск системы
			this->_callback.on <void (const event::status_t)> ("status", static_cast <void (server_t::*)(const event::status_t)> (&server_t::launch), this, _1);
			// Выполняем запуск работы основного юнита
			unit_t::start();
		/**
		 * Операционной системой не распознана
		 */
		#else
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке запуска события
				this->_log->debug("This operating system is not supported", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке запуска события
				this->_log->print("This operating system is not supported", log_t::flag_t::CRITICAL);
			#endif
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		#endif
	}
}
/**
 * @brief Метод установки функций обратного вызова
 *
 * @param callback функции обратного вызова
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
	// Выполняем установку функции обратного вызова при ЗАПУСКЕ/ОСТАНОВКЕ процесса кластера
	this->_callback.set("cluster_events", callback);
	// Выполняем установку функции обратного вызова при отправке сообщения кластера
	this->_callback.set("cluster_sending", callback);
	// Выполняем установку функции обратного вызова при получении сообщения кластера
	this->_callback.set("cluster_message", callback);
	// Выполняем установку функции обратного вызова при получении доступности размера очереди сообщений кластера
	this->_callback.set("cluster_available", callback);
}
/**
 * @brief Метод уничтожения события сервера
 *
 * @param eid идентификатор события для уничтожения
 */
void awh::unit::Server::destroy(const event::id_t eid) noexcept {
	{
		// Выполняем блокировку потока для уничтожения события сервера
		const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
		// Удаляем событие сервера
		this->_io->destroy(eid);
	}
	// Выполняем блокировку потока для работы временным списком событий сервера
	const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Если в списке событий сервера есть события
	if(!this->_events.empty()){
		// Выполняем поиск идентификатора события сервера в списке событий сервера
		auto i = this->_events.find(eid);
		// Если идентификатор события сервера найден в списке событий сервера
		if(i != this->_events.end())
			// Удаляем идентификатор события сервера из списка событий сервера
			this->_events.erase(i);
	}
}
/**
 * @brief Метод получения идентификатора сервера для выполнения запросов к серверу
 *
 * @param family   семейство адресов
 * @param type     тип события
 * @param protocol протокол события
 * @return         идентификатор созданного сервера
 */
awh::event::id_t awh::unit::Server::issue(const event::family_t family, const event::type_t type, const event::protocol_t protocol) noexcept {
	// Результат работы функции
	event::id_t result = 0;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		{
			// Выполняем блокировку потока для работы с событием сервера
			const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Добавляем новое событие сервера
			result = this->_io->event(event::node_t::SERVER, family, type, protocol);
			// Устанавливаем функцию обратного вызова на событие изменения действий сервера
			this->_io->on(result, static_cast <engine::callback::event_t> (std::bind(&server_t::action, this, _1, _2)));
			// Устанавливаем функцию обратного вызова на событие записи данных
			this->_io->on(result, static_cast <engine::callback::write_t> (std::bind(&server_t::write, this, _1, _2)));
			// Устанавливаем функцию обратного вызова на событие разрешения подключения
			this->_io->on(result, static_cast <engine::callback::accept_t> (std::bind(&server_t::accept, this, _1, _2)));
			// Устанавливаем функцию обратного вызова на событие неотправленных данных сервера
			this->_io->on(result, static_cast <engine::callback::spool_t> (std::bind(&server_t::spool, this, _1, _2, _3, _4)));
			// Устанавливаем функцию обратного вызова на событие изменения статуса сервера
			this->_io->on(result, static_cast <engine::callback::status_t> (std::bind(static_cast <void (server_t::*)(const event::id_t, const event::status_t)> (&server_t::status), this, _1, _2)));
			// Устанавливаем функцию обратного вызова на событие получения ошибок
			this->_io->on(result, static_cast <engine::callback::error_t> (std::bind(static_cast <void (server_t::*)(const event::id_t, const event::error_t, const string &)> (&server_t::error), this, _1, _2, _3)));
		}
		// Выполняем блокировку потока для работы временным списком событий сервера
		const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
		// Добавляем идентификатор события сервера в список событий сервера
		this->_events.emplace(result);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (family), static_cast <uint16_t> (type), static_cast <uint16_t> (protocol)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки флага автоматического возрождения процессов
 *
 * @param mode флаг возрождения процессов
 */
void awh::unit::Server::clusterRebirth(const bool mode) noexcept {
	// Выполняем блокировку потока для работы с событием кластера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Устанавливаем флаг автоматического возрождения процессов
	this->_clusterParams.rebirth = mode;
	// Если кластер инициализирован
	if(this->_cluster != nullptr)
		// Устанавливаем флаг автоматического возрождения процессов
		this->_cluster->rebirth(this->_clusterParams.rebirth);
}
/**
 * @brief Метод установки названия кластера
 *
 * @param name название кластера для установки
 */
void awh::unit::Server::clusterName(string_view name) noexcept {
	// Выполняем блокировку потока для работы с событием кластера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
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
 */
awh::unit::cluster_t::family_t awh::unit::Server::clusterFamily() const noexcept {
	// Если кластер инициализирован
	if(this->_cluster != nullptr){
		// Выполняем блокировку потока для работы с событием кластера
		const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::SHARED);
		// Если текущий процесс кластера является родительским
		if(this->_cluster->master())
			// Устанавливаем результат работы функции
			return cluster_t::family_t::MASTER;
		// Если текущий процесс кластера является дочерним
		return cluster_t::family_t::CHILDREN;
	}
	// Выводим результат по умолчанию
	return cluster_t::family_t::NONE;
}
/**
 * @brief Метод получения режима активации кластера
 *
 * @return режим активации кластера
 */
awh::event::mode_t awh::unit::Server::clusterMode() const noexcept {
	// Выполняем блокировку потока для работы с событием кластера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::SHARED);
	// Извлекаем режим активации кластера
	return this->_clusterParams.mode;
}
/**
 * @brief Метод установки количества процессов кластера
 *
 * @param mode флаг активации/деактивации кластера
 * @param size количество рабочих процессов
 */
void awh::unit::Server::clusterMode(const event::mode_t mode) noexcept {
	// Выполняем блокировку потока для работы с событием кластера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Устанавливаем режим активации кластера
	this->_clusterParams.mode = mode;
}
/**
 * @brief Метод получения максимального количества процессов
 *
 * @return максимальное количество процессов
 */
uint16_t awh::unit::Server::clusterCount() const noexcept {
	// Если кластер инициализирован
	if(this->_cluster != nullptr){
		// Выполняем блокировку потока для работы с событием кластера
		const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::SHARED);
		// Выводим максимальное количество процессов кластера
		return this->_cluster->count();
	}
	// Выводим результат по умолчанию
	return 0;
}
/**
 * @brief Метод установки максимального количества процессов
 *
 * @param count максимальное количество процессов
 */
void awh::unit::Server::clusterCount(const uint16_t count) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем блокировку потока для работы с событием кластера
		const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(count), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод получения списка дочерних процессов
 *
 * @return список дочерних процессов
 */
unordered_set <pid_t> awh::unit::Server::clusterWorkers() const noexcept {
	// Если кластер инициализирован
	if(this->_cluster != nullptr){
		// Выполняем блокировку потока для работы с событием кластера
		const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::SHARED);
		// Выводим количество активных воркеров кластера
		return this->_cluster->workers();
	}
	// Выводим результат по умолчанию
	return {};
}
/**
 * @brief Метод отправки сообщения родительскому процессу
 *
 * @param buffer бинарный буфер для отправки сообщения
 * @param size   размер бинарного буфера для отправки сообщения
 * @return       количество байт отправленного сообщения
 */
size_t awh::unit::Server::clusterSend(const void * buffer, const size_t size) noexcept {
	// Если кластер инициализирован
	if(this->_cluster != nullptr){
		// Выполняем блокировку потока для работы с событием кластера
		const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
		// Выполняем отправку сообщения родительскому процессу
		return this->_cluster->send(buffer, size);
	}
	// Выводим результат по умолчанию
	return 0;
}
/**
 * @brief Метод отправки сообщения дочернему процессу
 *
 * @param pid    идентификатор процесса для получения сообщения
 * @param buffer бинарный буфер для отправки сообщения
 * @param size   размер бинарного буфера для отправки сообщения
 * @return       количество байт отправленного сообщения
 */
size_t awh::unit::Server::clusterSend(const pid_t pid, const void * buffer, const size_t size) noexcept {
	// Если кластер инициализирован
	if(this->_cluster != nullptr){
		// Выполняем блокировку потока для работы с событием кластера
		const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
		// Выполняем отправку сообщения дочернему процессу
		return this->_cluster->send(pid, buffer, size);
	}
	// Выводим результат по умолчанию
	return 0;
}
/**
 * @brief Метод отправки сообщения всем дочерним процессам
 *
 * @param buffer бинарный буфер для отправки сообщения
 * @param size   размер бинарного буфера для отправки сообщения
 * @return       количество байт отправленного сообщения
 */
size_t awh::unit::Server::clusterBroadcast(const void * buffer, const size_t size) noexcept {
	// Если кластер инициализирован
	if(this->_cluster != nullptr){
		// Выполняем блокировку потока для работы с событием кластера
		const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
		// Выполняем отправку сообщения всем дочерним процессам
		return this->_cluster->broadcast(buffer, size);
	}
	// Выводим результат по умолчанию
	return 0;
}
/**
 * @brief Метод получения размера буфера события
 *
 * @param pid    идентификатор процесса
 * @param action тип действия события
 * @return       размер буфера события
 */
size_t awh::unit::Server::clusterGetBufferSize(const pid_t pid, const event::action_t action) const noexcept {
	// Если кластер инициализирован
	if(this->_cluster != nullptr){
		// Выполняем блокировку потока для работы с событием кластера
		const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::SHARED);
		// Выполняем получение размера буфера кластера для события кластера
		return this->_cluster->getBufferSize(pid, action);
	}
	// Выводим результат по умолчанию
	return 0;
}
/**
 * @brief Метод установки размера буфера события
 *
 * @param pid    идентификатор процесса
 * @param action тип действия события
 * @param size   размер буфера события
 * @return       результат выполнения установки
 */
bool awh::unit::Server::clusterSetBufferSize(const pid_t pid, const event::action_t action, const size_t size) noexcept {
	// Если кластер инициализирован
	if(this->_cluster != nullptr){
		// Выполняем блокировку потока для работы с событием кластера
		const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
		// Выполняем установку размера буфера кластера для события кластера
		return this->_cluster->setBufferSize(pid, action, size);
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::unit::Server::Server(const fmk_t * fmk, const log_t * log) noexcept : unit_t(fmk, log), _cluster(nullptr) {
	// Деактивируем мьютекс на время инициализации
	this->_mtx.enabled = false;
}
/**
 * @brief Деструктор
 *
 */
awh::unit::Server::~Server() noexcept {
	// Выполняем блокировку потока для работы временным списком событий сервера
	const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
	// Если в списке событий сервера есть события
	if(!this->_events.empty()){
		// Выполняем удаление всех событий сервера
		for(const auto & eid : this->_events){
			// Выполняем блокировку потока для уничтожения событий
			const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Удаляем событие сервера
			this->_io->destroy(eid);
		}
	}
}
