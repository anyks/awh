/**
 * @file: base.cpp
 * @date: 2025-10-17
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
 * Подключаем заголовочный файл
 */
#include <events/base.hpp>

#include <iostream>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * @brief Метод проверки запущена ли в данный момент база событий
 *
 * @return результат проверки запущена ли база событий
 */
bool awh::Base::launched() const noexcept {
	// Выполняем проверку запущена ли работа базы событий
	return this->_launched;
}
/**
 * @brief Метод удаления события из базы событий
 *
 * @param id идентификатор события
 * @return   результат работы функции
 */
bool awh::Base::erase(const uint32_t id) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем обработку ошибки
	 */
	try {
		// Выполняем поиск нужного нам участника
		auto i = this->_peers.find(id);
		// Если нужный нам участник найден
		if(i != this->_peers.end()){
			// Выполняем удаление события
			result = this->_react.del(i->first, i->second.sock);
			// Выполняем удаление участника
			this->_peers.erase(i);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат по умолчанию
	return result;
}
/**
 * @brief Метод установки режима работы сокета
 *
 * @param id     идентификатор события
 * @param events устанавливаемые события
 * @return       результат установки события
 */
bool awh::Base::mode(const uint32_t id, const uint8_t events) noexcept {
	/**
	 * Выполняем обработку ошибки
	 */
	try {
		// Выполняем поиск идентификатора участника
		auto i = this->_peers.find(id);
		// Если участник найден
		if(i != this->_peers.end()){
			// Устанавливаем события
			i->second.events = events;
			// Если участник является таймером или интервалом
			if(i->second.delay > 0)
				// Выполняем активацию таймера
				return this->_react.add(i->first, i->second.events, i->second.delay);
			// Выполняем модификацию события участника
			return this->_react.modify(i->first, i->second.sock, i->second.events);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, events), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод добавления сокета в базу событий
 *
 * @param sock     сокет для добавления
 * @param callback функция обратного вызова при получении события
 * @param delay    задержка времени таймера
 * @return         идентификатор добавленного события
 */
uint32_t awh::Base::emplace(const SOCKET sock, const events_t::callback_t & callback, const uint32_t delay) noexcept {
	// Если сокет для установки передан
	if((callback != nullptr) && ((sock != INVALID_SOCKET) || (delay > 0))){
		/**
		 * Выполняем обработку ошибки
		 */
		try {
			// Выполняем блокировку потока
			const lock_guard lock(this->_mtx);
			// Выполняем регистрацию нового участника
			auto ret = this->_peers.emplace(const_cast <fmk_t *> (this->_fmk)->identifier(), peer_t(callback));
			// Устанавливаем сокет участника
			ret.first->second.sock = sock;
			// Устанавливаем задрежку времени
			ret.first->second.delay = delay;
			// Если мы добавляем не таймер
			if(ret.first->second.delay == 0)
				// Если участник является обычным событием
				this->_react.add(ret.first->first, ret.first->second.sock);
			// Выводим созданный идентификатор пира
			return ret.first->first;
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, delay), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return 0;
}
/**
 * @brief Метод очистки списка событий
 *
 */
void awh::Base::clear() noexcept {
	// Если работа базы событий ещё запущена
	if(!this->_launched){
		/**
		 * Выполняем обработку ошибки
		 */
		try {
			// Выполняем очистку активных участников
			this->_peers.clear();
			// Выполняем очистку списка событий
			this->_events.clear();
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод остановки чтения базы событий
 *
 */
void awh::Base::stop() noexcept {
	// Если работа базы событий запущена
	if(this->_launched && this->_works)
		// Выполняем отключение флага цикла опроса событий сокетов
		this->_works = !this->_works;
}
/**
 * @brief Метод запуска чтения базы событий
 *
 */
void awh::Base::start() noexcept {
	// Если работа базы событий не запущена
	if(!this->_launched){
		// Устанавливаем флаг запуска
		this->_launched = !this->_launched;
		/**
		 * Количество активных сокетов
		 * Идентификатор полученного события
		 * Идентификатор трансферной передачи
		 */
		uint32_t count = 0, id = 0, tid = 0;
		/**
		 * Функция обратного вызова
		 */
		events_t::callback_t callback = nullptr;
		/**
		 * Устанавливаем флаг разрешение запуска базы событий
		 */
		this->_works = static_cast <bool> (this->_launched);
		/**
		 * Выполняем запуск базы события
		 */
		while(this->_works){
			/**
			 * Выполняем обработку ошибки
			 */
			try {
				// Выполняем очистку списка активных событий
				this->_pollers.clear();
				// Выполняем инициализацию списка активных событий
				this->_pollers.resize(this->_peers.size());
				// Выполняем опрос событий для активных сокетов
				count = this->_react.wait(&this->_pollers[0], (!this->_easily ? static_cast <int32_t> (this->_rate) : 0));
				// Переходим по всему списку активных событий
				for(uint32_t i = 0; i < count; i++){
					// Выполняем получение объекта поллера
					react_t::poller_t & poller = this->_pollers[i];
					// Выполняем поиск участника
					auto j = this->_peers.find(poller.id);
					// Если участник найден
					if(j != this->_peers.end()){
						// Устанавливаем идентификатор события
						id = j->first;
						// Если событие принадлежит потоку
						if(poller.events & react_t::AWH_STREAM){
							// Выполняем извлечение данных проброшенных между потоками
							tid = this->_react.notifications(id);
							// Выполняем поиска функции обратного вызова
							auto i = this->_events.find(id);
							// Если функцию обратного вызова мы получили
							if(i != this->_events.end()){
								// Выполняем копирование функции обратного вызова
								auto callback = i->second;
								// Выполняем функцию обратного вызова
								callback(tid);
							}
						// Если событие принадлежит таймеру
						} else if(poller.events & react_t::AWH_TIMER)
							// Выполняем функцию обратного вызова
							j->second.callback(j->second.sock, events_t::type_t::TIMER);
						// Если событие принадлежит интервалу
						else if(poller.events & react_t::AWH_INTERVAL)
							// Выполняем функцию обратного вызова
							j->second.callback(j->second.sock, events_t::type_t::INTERVAL);
						// Если мы получаем событие таймаута
						else if(poller.events & react_t::AWH_TIMEOUT) {
							/**
							 * Пропускаем данное событие так-как в нём нет смысла
							 */
						// Если мы получили любое другое событие
						} else {
							// Если мы получили событие готовности сокета на чтение
							if(poller.events & react_t::AWH_READ){
								// Выполняем копирование функции обратного вызова
								callback = j->second.callback;
								// Выполняем функцию обратного вызова
								callback(j->second.sock, events_t::type_t::READ);
							}
							// Если мы получили событие готовности сокета на запись
							if(poller.events & react_t::AWH_WRITE){
								// Выполняем поиск активного участника
								auto i = this->_peers.find(id);
								// Если искомый нами участник найден
								if(i != this->_peers.end()){
									// Выполняем копирование функции обратного вызова
									callback = i->second.callback;
									// Выполняем функцию обратного вызова
									callback(i->second.sock, events_t::type_t::WRITE);
								}
							}
							// Если мы получили событие закрытия сокета или ошибку
							if((poller.events & react_t::AWH_CLOSE) || (poller.events & react_t::AWH_ERROR)){
								// Выполняем поиск активного участника
								auto i = this->_peers.find(id);
								// Если искомый нами участник найден
								if(i != this->_peers.end()){
									// Выполняем копирование функции обратного вызова
									callback = i->second.callback;
									// Выполняем функцию обратного вызова
									callback(i->second.sock, events_t::type_t::CLOSE);
								}
							}
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
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			}
		}
		// Устанавливаем флаг запуска
		this->_launched = !this->_launched;
	}
}
/**
 * @brief Метод пересоздания базы событий
 *
 */
void awh::Base::rebase() noexcept {
	/**
	 * Выполняем обработку ошибки
	 */
	try {
		// Если работа базы событий запущена
		if(this->_launched){
			// Выполняем остановку работы базы событий
			this->stop();
			// Выполняем запуск работы базы событий
			this->start();
			// Если список активных участников не пустой
			if(!this->_peers.empty()){
				// Выполняем перебор всего списка активных участников
				for(auto & peer : this->_peers){
					// Если участник является таймером или интервалом
					if(peer.second.delay > 0)
						// Выполняем активацию таймера
						this->_react.add(peer.first, peer.second.events, peer.second.delay);
					// Если участник является обычным событием
					else this->_react.add(peer.first, peer.second.sock, peer.second.events);
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод активации простого режима чтения базы событий
 *
 * @param mode флаг активации
 */
void awh::Base::easily(const bool mode) noexcept {
	// Выполняем установку флага активации простого режима чтения базы событий
	this->_easily = mode;
	// Если активирован простой режим работы чтения базы событий
	if(!this->_easily)
		// Выполняем сброс времени ожидания
		this->_rate = -1;
}
/**
 * @brief Метод установки времени блокировки базы событий в ожидании событий
 *
 * @param msec время ожидания событий в миллисекундах
 */
void awh::Base::rate(const uint32_t msec) noexcept {
	// Если количество миллисекунд передано верно
	if(msec > 0)
		// Выполняем установку времени ожидания
		this->_rate = static_cast <int32_t> (msec);
	// Выполняем сброс времени ожидания
	else this->_rate = -1;
}
/**
 * @brief Метод отправки события через потоки
 *
 * @param id  идентификатор события для отправки
 * @param tid идентификатор трансферной передачи
 * @return    результат отправки события
 */
bool awh::Base::trigger(const uint32_t id, const uint32_t tid) noexcept {
	// Выполняем отправки события
	return this->_react.notify(id, tid);
}
/**
 * @brief Метод отмены регистрации события
 *
 * @param id идентификатор события
 * @return   результат отмены регистрации события
 */
bool awh::Base::detach(const uint32_t id) noexcept {
	// Результат работы функции
	bool result = false;
	// Выполняем удаление события из реактора
	if((id > 0) && this->_react.del(id)){
		/**
		 * Выполняем обработку ошибки
		 */
		try {
			// Выполняем блокировку потока
			const lock_guard lock(this->_mtx);
			// Выполняем поиск функции обратного вызова
			auto i = this->_events.find(id);
			// Если функция обратного вызова найдена
			if((result = (i != this->_events.end())))
				// Выполняем удаление функции обратного вызова
				this->_events.erase(i);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод регистрации нового события
 *
 * @param callback функция обратного вызова
 * @return         идентификатор события
 */
uint32_t awh::Base::attach(function <void (const uint32_t)> callback) noexcept {
	// Если функция обратного вызова передана
	if(callback != nullptr){
		/**
		 * Выполняем обработку ошибки
		 */
		try {
			// Выполняем блокировку потока
			const lock_guard lock(this->_mtx);
			// Выполняем регистрацию функции обратного вызова
			auto ret = this->_events.emplace(const_cast <fmk_t *> (this->_fmk)->identifier(), ::move(callback));
			// Если событие удачно зарегистрированно в реакторе
			if(this->_react.add(ret.first->first, react_t::AWH_STREAM))
				// Выводим идентификатор зарегистрированного события
				return ret.first->first;
			// Удаляем зарегистрированное событие
			else this->_events.erase(ret.first->first);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат по умолчанию
	return 0;
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Base::Base(const fmk_t * fmk, const log_t * log) noexcept :
 _react(fmk, log), _rate(-1), _works(false),
 _easily(false), _launched(false), _fmk(fmk), _log(log) {
	// Выполняем инициализацию реактора
	this->_react.init();
}
/**
 * @brief Деструктор
 *
 */
awh::Base::~Base() noexcept {
	// Выполняем остановку работы базы событий
	this->stop();
	// Выполняем уничтожения реактора
	this->_react.destroy();
}
