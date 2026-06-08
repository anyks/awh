/**
 * @file: client.cpp
 * @date: 2026-03-15
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
#include <units/client.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён плейсхолдеров
 */
using namespace placeholders;

/**
 * @brief Метод запуска/остановки работы клиента
 *
 * @param status статус запуска/остановки клиента
 */
void awh::unit::Client::launch(const event::status_t status) noexcept {
	/**
	 * Определяем статус работы клиента
	 */
	switch(static_cast <uint8_t> (status)){
		// Если работа клиента запущена
		case static_cast <uint8_t> (event::status_t::LAUNCHED): {
			// Если функция обратного вызова установлена
			if(this->_callback.is("clientStatus"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const event::status_t)> ("clientStatus", status);
		} break;
		// Если работа клиента подлежит уничтожению
		case static_cast <uint8_t> (event::status_t::DESTROYED): {
			// Если в списке событий клиента есть события
			if(!this->_events.empty()){
				// Выполняем удаление всех событий клиента
				for(const auto & eid : this->_events)
					// Удаляем событие клиента
					this->_io->destroy(eid);
			}
			// Если функция обратного вызова установлена
			if(this->_callback.is("clientStatus")){
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const event::status_t)> ("clientStatus", status);
				// Выполняем получение функции обратного вызова
				this->_callback.set("clientStatus", "status", this->_callback);
			}
		} break;
	}
}
/**
 * @brief Метод обработки событий подключения клиента к удалённому серверу
 *
 * @param eid идентификатор события
 * @param ok  результат подключения
 */
void awh::unit::Client::connect(const event::id_t eid, const bool ok) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("connect"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::id_t, const bool)> ("connect", eid, ok);
}
/**
 * @brief Метод обработки событий записи данных клиентом
 *
 * @param eid  идентификатор события
 * @param size размер данных для записи
 */
void awh::unit::Client::write(const event::id_t eid, const size_t size) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("write"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::id_t, const size_t)> ("write", eid, size);
}
/**
 * @brief Метод обработки действий клиента
 *
 * @param eid    идентификатор события
 * @param action действие клиента
 */
void awh::unit::Client::action(const event::id_t eid, const event::action_t action) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("action"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::id_t, const event::action_t)> ("action", eid, action);
}
/**
 * @brief Метод обработки событий изменения статуса клиента
 *
 * @param eid    идентификатор события
 * @param status новый статус клиента
 */
void awh::unit::Client::status(const event::id_t eid, const event::status_t status) noexcept {
	// Если статус сервера представляет из себя уничтожение
	if(status == event::status_t::DESTROYED){
		// Выполняем поиск идентификатора события клиента в списке событий
		auto i = this->_events.find(eid);
		// Если идентификатор события клиента найден
		if(i != this->_events.end())
			// Удаляем идентификатор события клиента
			this->_events.erase(i);
	}
	// Если функция обратного вызова установлена
	if(this->_callback.is("state"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::id_t, const event::status_t)> ("state", eid, status);
}
/**
 * @brief Метод обработки событий получения данных клиентом
 *
 * @param eid  идентификатор события
 * @param data данные события получения данных клиентом
 * @param size размер данных события получения данных клиентом
 */
void awh::unit::Client::read(const event::id_t eid, const uint8_t * data, const size_t size) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("read"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::id_t, const uint8_t *, const size_t)> ("read", eid, data, size);
}
/**
 * @brief Метод обработки события доступности/недоступности очереди исходящих данных клиента
 *
 * @param eid    идентификатор события
 * @param status статус доступности очереди
 * @param size   размер доступных данных очереди
 */
void awh::unit::Client::available(const event::id_t eid, const event::status_t status, const size_t size) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("available"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::id_t, const event::status_t, const size_t)> ("available", eid, status, size);
}
/**
 * @brief Метод обработки событий истечения таймаута клиента
 *
 * @param eid    идентификатор клиента
 * @param action тип действия для истекшего таймаута
 * @param delay  задержка таймаута в миллисекундах
 * @return       нужно ли завершить клиента после истечения таймаута
 */
bool awh::unit::Client::timeout(const event::id_t eid, const event::action_t action, const uint32_t delay) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("timeout"))
		// Выполняем функцию обратного вызова
		return this->_callback.call <bool (const event::id_t, const event::action_t, const uint32_t)> ("timeout", eid, action, delay);
	// Возвращаем значение, указывающее на то, что клиента нужно завершить после истечения таймаута
	return true;
}
/**
 * @brief Метод обработки событий ошибок клиента
 *
 * @param eid         идентификатор события
 * @param error       тип ошибки
 * @param description описание ошибки
 */
void awh::unit::Client::error(const event::id_t eid, const event::error_t error, const string & description) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("error"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", eid, error, description);
}
/**
 * @brief Метод обработки события неотправленных данных клиента
 *
 * @param eid   идентификатор события
 * @param error тип ошибки отправки данных
 * @param data  данные, которые не получилось отправить
 * @param size  размер данных, которые не получилось отправить
 */
void awh::unit::Client::spool(const event::id_t eid, const event::send_error_t error, const uint8_t * data, const size_t size) noexcept {
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
bool awh::unit::Client::isActual(const event::id_t eid) const noexcept {
	// Проверяем есть ли событие клиента в списке событий клиента
	return this->_events.find(eid) != this->_events.end();
}
/**
 * @brief Метод фиксации настроек клиента
 *
 * @param eid идентификатор события клиента
 * @return    результат выполнения фиксации
 */
bool awh::unit::Client::commit(const event::id_t eid) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если событие клиента является актуальным
		if(this->isActual(eid)){
			// Выполняем фиксацию параметров события
			if(!(result = this->_io->commit(eid))){
				// Удаляем событие клиента
				this->_io->destroy(eid);
				// Выполняем поиск идентификатора события клиента в списке событий клиента
				auto i = this->_events.find(eid);
				// Если идентификатор события клиента найден в списке событий клиента
				if(i != this->_events.end())
					// Удаляем идентификатор события клиента из списка событий клиента
					this->_events.erase(i);
				// Если функция обратного вызова не установлена
				if(!this->_callback.is("error")){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Failed to commit client", __PRETTY_FUNCTION__, std::make_tuple(eid), log_t::flag_t::CRITICAL);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Failed to commit client", log_t::flag_t::CRITICAL);
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
 * @brief Метод запуска работы клиента
 *
 * @param eid идентификатор события клиента
 * @return    результат выполнения запуска
 */
bool awh::unit::Client::launch(const event::id_t eid) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если событие клиента является актуальным
		if(this->isActual(eid)){
			// Выполняем запуск работы клиента
			if(!(result = this->_io->launch(eid))){
				// Если функция обратного вызова не установлена
				if(!this->_callback.is("error")){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Failed to launch client", __PRETTY_FUNCTION__, std::make_tuple(eid), log_t::flag_t::CRITICAL);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Failed to launch client", log_t::flag_t::CRITICAL);
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
bool awh::unit::Client::pause(const event::id_t eid) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем приостановку работы клиента
		return this->_io->pause(eid);
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод возобновления работы клиента
 *
 * @param eid идентификатор события клиента
 * @return    результат выполнения возобновления работы
 */
bool awh::unit::Client::resume(const event::id_t eid) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем возобновление работы клиента
		return this->_io->resume(eid);
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод отключения клиента от удалённого сервера
 *
 * @param eid идентификатор события клиента
 * @return    результат выполнения отключения
 */
bool awh::unit::Client::disconnect(const event::id_t eid) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем отключение клиента от сервера
		return this->_io->disconnect(eid);
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод мультиподключения клиентов к удалённым хостам
 *
 * @param ids список идентификаторов событий для подключения
 * @return    результат выполнения подключения
 */
bool awh::unit::Client::connect(const vector <event::id_t> & ids) noexcept {
	// Если количество идентификаторов событий для подключения больше количества событий клиента
	if(ids.size() > this->_events.size())
		// Выводим результат по умолчанию
		return false;
	/**
	 * Выполняем проверку каждого идентификатора события для подключения
	 */
	for(awh::event::id_t eid : ids)
		// Если событие клиента не является актуальным
		if(!this->isActual(eid))
			// Выводим результат по умолчанию
			return false;
	// Выполняем подключение клиента к серверу
	return this->_io->connect(ids);
}
/**
 * @brief Метод получения данных от сервера
 *
 * @param eid идентификатор события клиента
 * @return    результат получения данных
 */
bool awh::unit::Client::recv(const event::id_t eid) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение данных от сервера
		return this->_io->recv(eid);
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод отправки данных серверу
 *
 * @param eid    идентификатор события клиента
 * @param buffer буфер данных для отправки
 * @param size   размер данных для отправки
 * @return       количество байт данных, отправленных серверу
 */
size_t awh::unit::Client::send(const event::id_t eid, const void * buffer, const size_t size) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем отправку данных серверу
		return this->_io->send(eid, buffer, size);
	// Выводим результат по умолчанию
	return 0;
}
/**
 * @brief Метод объединения данных между клиентом и другим событием
 *
 * @param eid  идентификатор события-источника
 * @param dest идентификатор события-приёмника
 * @return     результат выполнения объединения
 */
bool awh::unit::Client::splice(const event::id_t eid, const event::id_t dest) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid) || this->isActual(dest))
		// Выполняем объединение данных между событием клиента и другим событием
		return this->_io->splice(eid, dest);
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод получения опций клиента
 *
 * @param eid идентификатор события клиента
 * @return    опции клиента
 */
uint16_t awh::unit::Client::getOptions(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение опций для события клиента
		return this->_io->getOptions(eid);
	// Выводим результат по умолчанию
	return 0;
}
/**
 * @brief Метод установки опций клиента
 *
 * @param eid     идентификатор события клиента
 * @param options опции клиента для установки
 * @return        результат выполнения установки
 */
bool awh::unit::Client::setOptions(const event::id_t eid, const uint16_t options) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку опций для события клиента
		return this->_io->setOptions(eid, options);
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод установки опции клиента
 *
 * @param eid    идентификатор события клиента
 * @param option опция клиента для установки
 * @param mode   режим установки опции клиента
 * @return       результат выполнения установки
 */
bool awh::unit::Client::setOption(const event::id_t eid, const uint16_t option, const bool mode) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку опции для события клиента
		return this->_io->setOption(eid, option, mode);
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод получения максимального количества хопов, через которые может пройти пакет
 *
 * @param eid идентификатор события клиента
 * @return    максимальное количество хопов
 */
awh::event::hops_t awh::unit::Client::getHops(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение максимального количества хопов для события клиента
		return this->_io->getHops(eid);
	// Выводим результат по умолчанию
	return event::hops_t::LOOPBACK;
}
/**
 * @brief Метод установки максимального количества хопов, через которые может пройти пакет
 *
 * @param eid  идентификатор события клиента
 * @param hops максимальное количество хопов
 * @return     результат работы функции
 */
bool awh::unit::Client::setHops(const event::id_t eid, const event::hops_t hops) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку максимального количества хопов для события клиента
		return this->_io->setHops(eid, this->_io->family(eid), hops);
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод получения сетевого интерфейса клиента
 *
 * @param eid идентификатор события клиента
 * @return    сетевой интерфейс клиента
 */
string awh::unit::Client::getIface(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение сетевого интерфейса для события клиента
		return this->_io->getIface(eid);
	// Выводим результат по умолчанию
	return "";
}
/**
 * @brief Метод установки сетевого интерфейса клиента
 *
 * @param eid  идентификатор события клиента
 * @param name имя сетевого интерфейса для установки
 * @return     результат выполнения установки
 */
bool awh::unit::Client::setIface(const event::id_t eid, string_view name) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку сетевого интерфейса для события клиента
		return this->_io->setIface(eid, name);
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод получения внутреннего порта события
 *
 * @param eid идентификатор события
 * @return    внутренний порт события
 */
uint16_t awh::unit::Client::getSourcePort(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение внутреннего порта для события клиента
		return this->_io->getSourcePort(eid);
	// Выводим результат по умолчанию
	return 0;
}
/**
 * @brief Метод установки внутреннего порта события
 *
 * @param eid  идентификатор события
 * @param port внутренний порт события
 * @return     результат выполнения установки
 */
bool awh::unit::Client::setSourcePort(const event::id_t eid, const uint16_t port) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку внутреннего порта для события клиента
		return this->_io->setSourcePort(eid, port);
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод получения порта удаленного сервера
 *
 * @param eid идентификатор события клиента
 * @return    порт удаленного сервера
 */
uint16_t awh::unit::Client::getDestinationPort(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение порта удаленного сервера для события клиента
		return this->_io->getDestinationPort(eid);
	// Выводим результат по умолчанию
	return 0;
}
/**
 * @brief Метод установки порта удаленного сервера
 *
 * @param eid  идентификатор события клиента
 * @param port порт удаленного сервера для установки
 * @return     результат выполнения установки
 */
bool awh::unit::Client::setDestinationPort(const event::id_t eid, const uint16_t port) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку порта удаленного сервера для события клиента
		return this->_io->setDestinationPort(eid, port);
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод получения адреса хоста целевой машины
 *
 * @param eid идентификатор события клиента
 * @return    адрес хоста целевой машины
 */
string awh::unit::Client::getTarget(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение адреса хоста целевой машины для события клиента
		return this->_io->getTarget(eid);
	// Выводим результат по умолчанию
	return "";
}
/**
 * @brief Метод установки адреса хоста целевой машины
 *
 * @param eid    идентификатор события клиента
 * @param target адрес хоста целевой машины
 * @return       результат выполнения установки
 */
bool awh::unit::Client::setTarget(const event::id_t eid, string_view target) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку адреса хоста целевой машины для события клиента
		return this->_io->setTarget(eid, target);
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод установки адреса хоста целевой машины
 *
 * @param eid    идентификатор события клиента
 * @param target адрес хоста целевой машины
 * @return       результат выполнения установки
 */
bool awh::unit::Client::setTarget(const event::id_t eid, const net::addr_t * target) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку адреса хоста целевой машины для события клиента
		return this->_io->setTarget(eid, target);
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод получения адреса хоста целевой машины
 *
 * @param eid    идентификатор события клиента
 * @param target объект для извлечения адреса хоста целевой машины
 * @return       результат выполнения извлечения адреса хоста целевой машины
 */
bool awh::unit::Client::getTarget(const event::id_t eid, unique_ptr <net::addr_t> & target) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение адреса хоста целевой машины для события клиента
		return this->_io->getTarget(eid, target);
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод получения адреса клиента
 *
 * @param eid     идентификатор события клиента
 * @param address тип адреса клиента
 * @return        значение адреса клиента
 */
string awh::unit::Client::getAddress(const event::id_t eid, const event::address_t address) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение адреса клиента для события клиента
		return this->_io->getAddress(eid, address);
	// Выводим результат по умолчанию
	return "";
}
/**
 * @brief Метод установки адреса клиента
 *
 * @param eid     идентификатор события клиента
 * @param address тип адреса клиента
 * @param value   значение адреса клиента
 * @return        результат выполнения установки
 */
bool awh::unit::Client::setAddress(const event::id_t eid, const event::address_t address, string_view value) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку адреса клиента для события клиента
		return this->_io->setAddress(eid, address, value);
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод установки адреса клиента
 *
 * @param eid     идентификатор события клиента
 * @param address тип адреса клиента
 * @param value   значение адреса клиента
 * @return        результат выполнения установки
 */
bool awh::unit::Client::setAddress(const event::id_t eid, const event::address_t address, const net::addr_t * value) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку адреса клиента для события клиента
		return this->_io->setAddress(eid, address, value);
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод получения адреса клиента
 *
 * @param eid     идентификатор события клиента
 * @param address тип адреса клиента
 * @param value   объект для извлечения адреса клиента
 * @return        результат выполнения извлечения адреса клиента
 */
bool awh::unit::Client::getAddress(const event::id_t eid, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение адреса клиента для события клиента
		return this->_io->getAddress(eid, address, value);
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод получения MTU сетевого интерфейса
 *
 * @param eid идентификатор события клиента
 * @return    MTU сетевого интерфейса
 */
uint16_t awh::unit::Client::getMaximumTransmissionUnit(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение MTU сетевого интерфейса для события клиента
		return this->_io->getMaximumTransmissionUnit(eid);
	// Выводим результат по умолчанию
	return 0;
}
/**
 * @brief Метод установки MTU сетевого интерфейса
 *
 * @param eid идентификатор события клиента
 * @param mtu размер MTU интерфейса
 * @return    результат установки MTU сетевого интерфейса
 */
bool awh::unit::Client::setMaximumTransmissionUnit(const event::id_t eid, const uint16_t mtu) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку MTU сетевого интерфейса для события клиента
		return this->_io->setMaximumTransmissionUnit(eid, mtu);
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод получения режима трансляции пакетов клиента
 *
 * @param eid идентификатор события клиента
 * @return    режим трансляции пакетов (unicast, multicast, broadcast)
 */
awh::event::delivery_mode_t awh::unit::Client::getDelivery(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение режима трансляции пакетов клиента для события клиента
		return this->_io->getDelivery(eid);
	// Выводим результат по умолчанию
	return event::delivery_mode_t::NONE;
}
/**
 * @brief Метод установки режима трансляции пакетов клиента
 *
 * @param eid      идентификатор события клиента
 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
 * @return         результат выполнения установки
 */
bool awh::unit::Client::setDelivery(const event::id_t eid, const event::delivery_mode_t delivery) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку режима трансляции пакетов клиента для события клиента
		return this->_io->setDelivery(eid, delivery);
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод получения размера буфера клиента
 *
 * @param eid    идентификатор события клиента
 * @param action тип действия клиента
 * @return       размер буфера клиента
 */
size_t awh::unit::Client::getBufferSize(const event::id_t eid, const event::action_t action) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение размера буфера клиента для события клиента
		return this->_io->getBufferSize(eid, action);
	// Выводим результат по умолчанию
	return 0;
}
/**
 * @brief Метод установки размера буфера клиента
 *
 * @param eid    идентификатор события клиента
 * @param action тип действия клиента
 * @param size   размер буфера клиента
 * @return       результат выполнения установки
 */
bool awh::unit::Client::setBufferSize(const event::id_t eid, const event::action_t action, const size_t size) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку размера буфера клиента для события клиента
		return this->_io->setBufferSize(eid, action, size);
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод получения режима использования таймаута на чтение события
 *
 * @param eid идентификатор события
 * @return    режим использования таймаута на чтение события
 */
awh::event::usage_t awh::unit::Client::getUsageReadTimeout(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение режима использования таймаута на чтение события для события клиента
		return this->_io->getUsageReadTimeout(eid);
	// Выводим результат по умолчанию
	return event::usage_t::NONE;
}
/**
 * @brief Метод установки режима использования таймаута на чтение события
 *
 * @param eid   идентификатор события
 * @param usage режим использования таймаута на чтение события (reusable или disposable)
 */
void awh::unit::Client::setUsageReadTimeout(const event::id_t eid, const event::usage_t usage) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку режима использования таймаута на чтение события для события клиента
		this->_io->setUsageReadTimeout(eid, usage);
}
/**
 * @brief Метод получения таймаута клиента
 *
 * @param eid    идентификатор события клиента
 * @param action тип действия клиента
 * @return       значение таймаута в миллисекундах
 */
uint32_t awh::unit::Client::getTimeout(const event::id_t eid, const event::action_t action) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение параметров таймаута для клиента
		return this->_io->getTimeout(eid, action);
	// Выводим результат по умолчанию
	return 0;
}
/**
 * @brief Метод установки таймаута клиента
 *
 * @param eid     идентификатор события клиента
 * @param action  тип действия клиента
 * @param timeout значение таймаута в миллисекундах
 */
void awh::unit::Client::setTimeout(const event::id_t eid, const event::action_t action, const uint32_t timeout) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку параметров таймаута для клиента
		this->_io->setTimeout(eid, action, timeout);
}
/**
 * @brief Метод установки пропускной способности клиента
 *
 * @param eid       идентификатор события клиента
 * @param limiting  режим ограничения пропускной способности клиента (egress или ingress)
 * @param bandwidth пропускная способность клиента для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
 * @return          результат выполнения установки
 */
bool awh::unit::Client::bandwidth(const event::id_t eid, const event::limiting_t limiting, string_view bandwidth) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку параметров пропускной способности для клиента
		return this->_io->bandwidth(eid, limiting, bandwidth);
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод установки параметров keep-alive для клиента
 *
 * @param eid   идентификатор события клиента
 * @param cnt   количество пакетов keep-alive
 * @param idle  время простоя перед отправкой первого пакета keep-alive в секундах
 * @param intvl интервал между пакетами keep-alive в секундах
 * @return      результат выполнения установки
 */
bool awh::unit::Client::keepAlive(const event::id_t eid, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку параметров keep-alive для клиента
		return this->_io->keepAlive(eid, cnt, idle, intvl);
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод получения значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
 *
 * @param eid идентификатор события клиента
 * @return    значение DSCP
 */
awh::event::dscp_t awh::unit::Client::getDifferentiatedServicesCodePoint(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение значения поля Differentiated Services Code Point (DSCP) для клиента
		return this->_io->getDifferentiatedServicesCodePoint(eid, this->_io->family(eid));
	// Выводим результат по умолчанию
	return event::dscp_t::CS0;
}
/**
 * @brief Метод установки значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
 *
 * @param eid  идентификатор события клиента
 * @param dscp значение DSCP
 * @return     результат работы функции
 */
bool awh::unit::Client::setDifferentiatedServicesCodePoint(const event::id_t eid, const event::dscp_t dscp) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку значения поля Differentiated Services Code Point (DSCP) для клиента
		return this->_io->setDifferentiatedServicesCodePoint(eid, this->_io->family(eid), dscp);
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод получения обнаружения максимального размера пакета (MTU)
 *
 * @param eid идентификатор события клиента
 * @return    режим обнаружения максимального размера пакета (MTU)
 */
awh::event::mtu_discover_t awh::unit::Client::getMaximumTransmissionUnitDiscover(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение обнаружения максимального размера пакета (MTU) для клиента
		return this->_io->getMaximumTransmissionUnitDiscover(eid, this->_io->family(eid));
	// Выводим результат по умолчанию
	return event::mtu_discover_t::NONE;
}
/**
 * @brief Метод установки обнаружения максимального размера пакета (MTU)
 *
 * @param eid  идентификатор события клиента
 * @param mode режим обнаружения максимального размера пакета (MTU)
 * @return     результат работы функции
 */
bool awh::unit::Client::setMaximumTransmissionUnitDiscover(const event::id_t eid, const event::mtu_discover_t mode) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку обнаружения максимального размера пакета (MTU) для клиента
		return this->_io->setMaximumTransmissionUnitDiscover(eid, this->_io->family(eid), mode);
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод активации/деактивации мультикаст группы
 *
 * @param eid    идентификатор события клиента
 * @param mode   режим активации/деактивации
 * @param group  мультикаст-группа для активации/деактивации
 * @param source адрес сетевого интерфейса с которого выполняется подписка
 * @param port   порт мультикаст-группы с которого выполняется подписка
 * @return       результат выполнения установки
 */
bool awh::unit::Client::membership(const event::id_t eid, const event::mode_t mode, string_view group, string_view source, const uint16_t port) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем активацию/деактивацию мультикаст группы для клиента
		return this->_io->membership(eid, mode, group, source, port);
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод активации/деактивации мультикаст группы
 *
 * @param eid    идентификатор события клиента
 * @param mode   режим активации/деактивации
 * @param group  мультикаст-группа для активации/деактивации
 * @param source адрес сетевого интерфейса с которого выполняется подписка
 * @param port   порт мультикаст-группы с которого выполняется подписка
 * @return       результат выполнения установки
 */
bool awh::unit::Client::membership(const event::id_t eid, const event::mode_t mode, const net::addr_t * group, const net::addr_t * source, const uint16_t port) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем активацию/деактивацию мультикаст группы для клиента
		return this->_io->membership(eid, mode, group, source, port);
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод остановки сервера
 *
 */
void awh::unit::Client::stop() noexcept {
	// Если работа юнита запущена
	if(this->working())
		// Выполняем остановку работы основного юнита
		unit_t::stop();
}
/**
 * @brief Метод запуска сервера
 *
 */
void awh::unit::Client::start() noexcept {
	// Если работа юнита ещё не запущена
	if(!this->working()){
		// Если функция обратного вызова установлена
		if(this->_callback.is("status"))
			// Выполняем получение функции обратного вызова
			this->_callback.set("status", "clientStatus", this->_callback);
		// Устанавливаем функцию обратного вызова на запуск системы
		this->_callback.on <void (const event::status_t)> ("status", static_cast <void (client_t::*)(const event::status_t)> (&client_t::launch), this, _1);
		// Выполняем запуск работы основного юнита
		unit_t::start();
	}
}
/**
 * @brief Метод установки функций обратного вызова
 *
 * @param callback функции обратного вызова
 */
void awh::unit::Client::callback(const callback_t & callback) noexcept {
	// Устанавливаем функцию обратного вызова для родительского юнита
	unit_t::callback(callback);
	// Выполняем установку функции обратного вызова при получении данных от сервера
	this->_callback.set("read", callback);
	// Выполняем установку функции обратного вызова при отправке данных на сервер
	this->_callback.set("write", callback);
	// Выполняем установку функции обратного вызова при получении состояния клиента
	this->_callback.set("state", callback);
	// Выполняем установку функции обратного вызова при получении события неотправленных данных
	this->_callback.set("spool", callback);
	// Выполняем установку функции обратного вызова на событие изменения статуса клиента
	this->_callback.set("status", callback);
	// Выполняем установку функции обратного вызова при обработке действий клиента
	this->_callback.set("action", callback);
	// Выполняем установку функции обратного вызова при подключении клиента к серверу
	this->_callback.set("connect", callback);
	// Выполняем установку функции обратного вызова на событие истечения таймаута клиента
	this->_callback.set("timeout", callback);
	// Выполняем установку функции обратного вызова при получении событий доступности/недоступности очереди исходящих данных клиента
	this->_callback.set("available", callback);
}
/**
 * @brief Метод уничтожения события клиента
 *
 * @param eid идентификатор события для уничтожения
 */
void awh::unit::Client::destroy(const event::id_t eid) noexcept {
	// Удаляем событие клиента
	this->_io->destroy(eid);
	// Если в списке событий клиента есть события
	if(!this->_events.empty()){
		// Выполняем поиск идентификатора события клиента в списке событий клиента
		auto i = this->_events.find(eid);
		// Если идентификатор события клиента найден в списке событий клиента
		if(i != this->_events.end())
			// Удаляем идентификатор события клиента из списка событий клиента
			this->_events.erase(i);
	}
}
/**
 * @brief Метод получения идентификатора клиента для выполнения запросов к серверу
 *
 * @param family   семейство адресов
 * @param type     тип события
 * @param protocol протокол события
 * @return         идентификатор созданного клиента
 */
awh::event::id_t awh::unit::Client::issue(const event::family_t family, const event::type_t type, const event::protocol_t protocol) noexcept {
	// Результат работы функции
	event::id_t result = 0;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Добавляем новое событие клиента
		result = this->_io->event(event::node_t::CLIENT, family, type, protocol);
		// Устанавливаем функцию обратного вызова на событие записи данных
		this->_io->on(result, static_cast <engine::callback::write_t> (std::bind(&client_t::write, this, _1, _2)));
		// Устанавливаем функцию обратного вызова на событие чтения данных
		this->_io->on(result, static_cast <engine::callback::read_t> (std::bind(&client_t::read, this, _1, _2, _3)));
		// Устанавливаем функцию обратного вызова на событие изменения действий клиента
		this->_io->on(result, static_cast <engine::callback::event_t> (std::bind(&client_t::action, this, _1, _2)));
		// Устанавливаем функцию обратного вызова на событие изменения статуса клиента
		this->_io->on(result, static_cast <engine::callback::status_t> (std::bind(&client_t::status, this, _1, _2)));
		// Устанавливаем функцию обратного вызова на событие получения ошибок
		this->_io->on(result, static_cast <engine::callback::error_t> (std::bind(&client_t::error, this, _1, _2, _3)));
		// Устанавливаем функцию обратного вызова на событие истечения таймаута клиента
		this->_io->on(result, static_cast <engine::callback::timeout_t> (std::bind(&client_t::timeout, this, _1, _2, _3)));
		// Устанавливаем функцию обратного вызова на событие неотправленных данных клиента
		this->_io->on(result, static_cast <engine::callback::spool_t> (std::bind(&client_t::spool, this, _1, _2, _3, _4)));
		// Устанавливаем функцию обратного вызова на событие доступности/недоступности очереди исходящих данных клиента
		this->_io->on(result, static_cast <engine::callback::available_t> (std::bind(&client_t::available, this, _1, _2, _3)));
		// Устанавливаем функцию обратного вызова на событие подключения клиента к удалённому серверу
		this->_io->on(result, static_cast <engine::callback::connect_t> (std::bind(static_cast <void (client_t::*)(const event::id_t, const bool)> (&client_t::connect), this, _1, _2)));
		// Добавляем идентификатор события клиента в список событий клиента
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
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::unit::Client::Client(const fmk_t * fmk, const log_t * log) noexcept : unit_t(fmk, log) {}
/**
 * @brief Деструктор
 *
 */
awh::unit::Client::~Client() noexcept {
	// Если в списке событий клиента есть события
	if(!this->_events.empty()){
		// Выполняем удаление всех событий клиента
		for(const auto & eid : this->_events)
			// Удаляем событие клиента
			this->_io->destroy(eid);
	}
}
