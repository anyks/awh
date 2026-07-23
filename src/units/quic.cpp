/**
 * @file: quic.cpp
 * @date: 2026-07-22
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
 * Подключаем заголовочный файл проекта
 */
#include <units/quic.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Используем пространство имён placeholders
 */
using namespace placeholders;

/**
 * @brief Конструктор сессии соединения
 *
 */
awh::unit::QuicServer::Session::Session() noexcept : connection(nullptr), connected(false) {}

/**
 * @brief Метод получения текущего времени в миллисекундах
 *
 * @return текущее время в миллисекундах
 */
uint64_t awh::unit::QuicServer::date() const noexcept {
	// Выводим текущий штамп времени в миллисекундах
	return this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
}
/**
 * @brief Метод формирования адреса удалённого эндпоинта сессии
 *
 * @param oid идентификатор события сессии
 * @return    адрес удалённого эндпоинта в виде "адрес:порт"
 */
string awh::unit::QuicServer::peer(const event::id_t oid) const noexcept {
	// Извлекаем адрес удалённого эндпоинта сессии
	string result = this->_io->getAddress(oid, event::address_t::IPV4);
	// Если адрес IPv4 не получен
	if(result.empty())
		// Извлекаем адрес удалённого эндпоинта сессии в семействе IPv6
		result = this->_io->getAddress(oid, event::address_t::IPV6);
	// Выводим адрес удалённого эндпоинта вместе с портом
	return (result + ":" + std::to_string(this->_io->getSourcePort(oid)));
}
/**
 * @brief Метод определения сессии принятой датаграммы (RFC 9000 §17.2)
 *
 * @param eid  идентификатор события сервера
 * @param data данные датаграммы
 * @param size размер датаграммы
 * @param key  выводимый ключ сессии
 * @return     результат определения сессии
 */
bool awh::unit::QuicServer::origin([[maybe_unused]] const event::id_t eid, const uint8_t * data, const size_t size, net::origin_key_t & key) noexcept {
	// Выводим идентификатор соединения получателя в качестве ключа сессии
	return quic::route(data, size, quic::connection_t::LOCAL_CID_SIZE, key);
}
/**
 * @brief Метод создания сессии нового соединения
 *
 * @param eid идентификатор события сервера
 * @param oid идентификатор события сессии
 */
void awh::unit::QuicServer::accept([[maybe_unused]] const event::id_t eid, const event::id_t oid) noexcept {
	// Если шаблон контекста безопасности не установлен
	if((this->_coder == nullptr) || (this->_ctx == 0)){
		// Записываем ошибку в лог
		this->_log->print("QUIC security context is not set", log_t::flag_t::CRITICAL);
		// Уничтожаем сессию соединения
		this->_io->destroy(oid);
		// Выходим из метода
		return;
	}
	// Выполняем создание сессии соединения
	auto ret = this->_sessions.emplace(oid, session_t());
	// Если сессия соединения не создана
	if(!ret.second)
		// Выходим из метода
		return;
	// Получаем созданную сессию соединения
	session_t & session = ret.first->second;
	// Создаём соединение QUIC на шаблоне контекста безопасности
	session.connection = make_unique <quic::connection_t> (
		quic::endpoint_t::SERVER, this->_ctx,
		* this->_coder, this->_log
	);
	// Устанавливаем локальные транспортные параметры соединения
	session.connection->params(this->_params);
	// Устанавливаем адрес удалённого эндпоинта соединения
	session.connection->address(this->peer(oid));
	// Устанавливаем режим проверки адреса клиента через пакет Retry
	session.connection->retry(this->_retry);
	// Устанавливаем режим маркировки исходящих датаграмм соединения
	session.connection->ecn(this->_ecn);
	// Устанавливаем общий ключ вывода токенов сброса без сохранения состояния
	session.connection->resetKey(this->_resetKey);
	// Устанавливаем функцию обратного вызова на чтение датаграмм сессии
	this->_io->on(oid, static_cast <engine::callback::read_t> (std::bind(&QuicServer::read, this, _1, _2, _3)));
}
/**
 * @brief Метод обработки принятой датаграммы сессии
 *
 * @param oid  идентификатор события сессии
 * @param data данные датаграммы
 * @param size размер датаграммы
 */
void awh::unit::QuicServer::read(const event::id_t oid, const uint8_t * data, const size_t size) noexcept {
	// Выполняем поиск сессии соединения
	auto i = this->_sessions.find(oid);
	// Если сессия соединения не найдена
	if(i == this->_sessions.end())
		// Выходим из метода
		return;
	// Получаем сессию соединения
	session_t & session = i->second;
	/**
	 * Обновляем адрес удалённого эндпоинта перед разбором датаграммы: смена
	 * адреса при установленном соединении означает миграцию на новый путь,
	 * которую соединение отслеживает самостоятельно (RFC 9000 §9)
	 */
	session.connection->address(this->peer(oid));
	// Если уведомление о перегрузке пути включено
	if(this->_ecn){
		/**
		 * Извлекаем метаданные принятой датаграммы с события сервера: маркировка
		 * снимается с заголовка IP-пакета при приёме, а сессии её не хранят
		 */
		const net::dgram_info_t info = this->_io->getTrafficInfo(this->_eid);
		// Выполняем обработку входящей датаграммы с маркировкой перегрузки
		session.connection->read(data, size, this->date(), info.congestion);
	// Выполняем обработку входящей датаграммы
	} else session.connection->read(data, size, this->date());
	// Синхронизируем маршрутизацию по идентификаторам соединения
	this->reroute(oid, session);
	// Если соединение установлено и приложение об этом ещё не оповещено
	if(!session.connected && (session.connection->state() == quic::connection_t::state_t::CONNECTED)){
		// Устанавливаем флаг оповещения приложения об установленном соединении
		session.connected = true;
		// Выполняем функцию обратного вызова об установленном соединении
		this->_callback.call <void (const event::id_t)> ("open", oid);
	}
	// Выполняем выдачу собранных данных потоков приложения
	this->process(oid, session);
	// Отправляем все готовые исходящие датаграммы
	const bool sent = this->flush(oid, session);
	/**
	 * Если соединение не начато и отправлять в ответ нечего: датаграмма адресована
	 * соединению, о котором сервер ничего не помнит. Отвечаем сбросом без сохранения
	 * состояния и снимаем сессию - иначе удалённый узел будет слать датаграммы
	 * до самого таймаута простоя (RFC 9000 §10.3)
	 */
	if(!sent && (session.connection->state() == quic::connection_t::state_t::NONE)){
		// Отправляем сброс без сохранения состояния
		this->drop(oid, data, size);
		// Выполняем завершение сессии соединения
		this->erase(oid);
		// Выходим из метода
		return;
	}
	// Если удалённый эндпоинт завершил соединение
	if(session.connection->state() == quic::connection_t::state_t::DRAINING)
		// Выполняем завершение сессии соединения
		this->erase(oid);
}
/**
 * @brief Метод обработки просроченных таймеров соединений
 *
 * @param eid    идентификатор события интервала
 * @param status статус события интервала
 */
void awh::unit::QuicServer::tick([[maybe_unused]] const event::id_t eid, const event::status_t status) noexcept {
	// Если статус события интервала не успешен
	if(status != event::status_t::SUCCESS)
		// Выходим из метода
		return;
	// Текущее время в миллисекундах
	const uint64_t date = this->date();
	// Список сессий с завершёнными соединениями
	vector <event::id_t> completed;
	/**
	 * Перебираем список сессий соединений
	 */
	for(auto & item : this->_sessions){
		// Дедлайн ближайшего события таймера соединения
		const uint64_t timeout = item.second.connection->timeout();
		// Если дедлайн таймера соединения наступил
		if((timeout > 0) && (date >= timeout)){
			// Выполняем обработку просроченных таймеров соединения
			item.second.connection->tick(date);
			// Синхронизируем маршрутизацию по идентификаторам соединения
			this->reroute(item.first, item.second);
			// Отправляем все готовые исходящие датаграммы
			this->flush(item.first, item.second);
		}
		// Если соединение завершено
		if(item.second.connection->state() == quic::connection_t::state_t::DRAINING)
			// Запоминаем сессию с завершённым соединением
			completed.push_back(item.first);
	}
	/**
	 * Перебираем список сессий с завершёнными соединениями
	 */
	for(auto & oid : completed)
		// Выполняем завершение сессии соединения
		this->erase(oid);
}
/**
 * @brief Метод синхронизации маршрутизации соединения
 *
 * @param oid     идентификатор события сессии
 * @param session сессия соединения
 */
void awh::unit::QuicServer::reroute(const event::id_t oid, session_t & session) noexcept {
	// Идентификаторы, введённые в обращение
	vector <quic::cid_t> added;
	// Идентификаторы, выведенные из обращения
	vector <quic::cid_t> removed;
	// Извлекаем изменения набора идентификаторов соединения
	session.connection->issued(added, removed);
	/**
	 * Перебираем введённые в обращение идентификаторы соединения
	 */
	for(auto & cid : added)
		// Привязываем идентификатор соединения к сессии
		this->_io->bind(oid, net::origin_key_t(cid.data, static_cast <uint8_t> (cid.size)));
	/**
	 * Перебираем выведенные из обращения идентификаторы соединения
	 */
	for(auto & cid : removed)
		// Снимаем идентификатор соединения с сессии
		this->_io->unbind(oid, net::origin_key_t(cid.data, static_cast <uint8_t> (cid.size)));
}
/**
 * @brief Метод выдачи собранных данных потоков приложения
 *
 * @param oid     идентификатор события сессии
 * @param session сессия соединения
 */
void awh::unit::QuicServer::process(const event::id_t oid, session_t & session) noexcept {
	// Если функция обратного вызова на собранные данные потока установлена
	if(this->_callback.is("read")){
		// Список потоков с собранными данными
		vector <uint64_t> streams;
		// Получаем список потоков с собранными данными
		session.connection->readable(streams);
		/**
		 * Перебираем потоки с собранными данными
		 */
		for(auto & sid : streams){
			// Флаг завершения потока удалённым эндпоинтом
			bool fin = false;
			// Собранные данные потока приложения
			string data = "";
			// Если выдача собранных данных потока не выполнена
			if(session.connection->receive(sid, data, fin) != quic::status_t::OK)
				// Пропускаем поток с ошибкой выдачи
				continue;
			// Если данные получены либо поток завершён
			if(!data.empty() || fin)
				// Выполняем функцию обратного вызова на собранные данные потока
				this->_callback.call <void (const event::id_t, const uint64_t, const string &, const bool)> ("read", oid, sid, data, fin);
		}
	}
	// Если функция обратного вызова на принятую датаграмму приложения установлена
	if(this->_callback.is("datagram")){
		// Буфер принятой датаграммы приложения
		string datagram = "";
		/**
		 * Выдаём принятые датаграммы приложения: они доставляются вне потоков
		 * и порядка доставки не имеют (RFC 9221)
		 */
		while(session.connection->datagram(datagram))
			// Выполняем функцию обратного вызова на принятую датаграмму приложения
			this->_callback.call <void (const event::id_t, const string &)> ("datagram", oid, datagram);
	}
}
/**
 * @brief Метод отправки готовых исходящих датаграмм соединения
 *
 * @param oid     идентификатор события сессии
 * @param session сессия соединения
 */
bool awh::unit::QuicServer::flush(const event::id_t oid, session_t & session) noexcept {
	// Флаг отправки хотя бы одной датаграммы
	bool result = false;
	// Буфер исходящей датаграммы
	string datagram = "";
	/**
	 * Извлекаем исходящие датаграммы соединения
	 */
	while(session.connection->write(datagram, this->date())){
		/**
		 * Применяем маркировку соединения к сокету: путь соединения мог проверку
		 * поддержки ECN не пройти, и маркировать его датаграммы далее нельзя,
		 * тогда как остальным соединениям сокета маркировка сохраняется
		 */
		this->mark(session.connection->marking());
		// Если отправка датаграммы удалённому эндпоинту не выполнена
		if(this->_io->send(oid, datagram.data(), datagram.size()) == 0)
			// Записываем ошибку в лог
			this->_log->print("QUIC datagram is not sent: ID=%u", log_t::flag_t::CRITICAL, oid);
		// Устанавливаем флаг отправки датаграммы
		result = true;
	}
	// Выводим результат отправки исходящих датаграмм
	return result;
}
/**
 * @brief Метод отправки сброса без сохранения состояния (RFC 9000 §10.3)
 *
 * @param oid  идентификатор события сессии
 * @param data данные вызвавшей сброс датаграммы
 * @param size размер вызвавшей сброс датаграммы
 * @return     результат отправки
 */
bool awh::unit::QuicServer::drop(const event::id_t oid, const uint8_t * data, const size_t size) noexcept {
	// Ключ маршрутизации вызвавшей сброс датаграммы
	net::origin_key_t key;
	// Если идентификатор соединения получателя извлечь не удалось
	if(!quic::route(data, size, quic::connection_t::LOCAL_CID_SIZE, key))
		// Выводим отрицательный результат
		return false;
	// Идентификатор соединения получателя вызвавшей сброс датаграммы
	quic::cid_t cid;
	// Если размер извлечённого идентификатора превышает предел QUIC
	if(key.size > quic::proto::MAX_CID_SIZE)
		// Выводим отрицательный результат
		return false;
	// Устанавливаем длину идентификатора соединения получателя
	cid.size = key.size;
	// Копируем данные идентификатора соединения получателя
	::memcpy(cid.data, key.data, key.size);
	// Буфер пакета сброса без сохранения состояния
	string reset = "";
	// Если сборка пакета сброса не выполнена
	if(!quic::reset(reset, this->_resetKey, cid, size))
		// Выводим отрицательный результат
		return false;
	// Если отправка пакета сброса удалённому эндпоинту не выполнена
	if(this->_io->send(oid, reset.data(), reset.size()) == 0)
		// Выводим отрицательный результат
		return false;
	// Записываем в лог сообщение об отправке сброса без сохранения состояния
	this->_log->print("QUIC stateless reset is sent: ID=%u", log_t::flag_t::INFO, oid);
	// Выводим положительный результат
	return true;
}
/**
 * @brief Метод применения маркировки соединения к сокету события сервера
 *
 * @param marking требуемая маркировка исходящих датаграмм
 */
void awh::unit::QuicServer::mark(const event::ecn_t marking) noexcept {
	// Если уведомление о перегрузке пути отключено либо маркировка уже установлена
	if(!this->_ecn || (marking == this->_marking))
		// Выходим из метода - смена маркировки не требуется
		return;
	// Если смена маркировки исходящих датаграмм не выполнена
	if(!this->_io->setExplicitCongestionNotification(this->_eid, this->_family, marking)){
		// Записываем предупреждение в лог
		this->_log->print("QUIC outgoing datagrams marking is not applied", log_t::flag_t::WARNING);
		// Выходим из метода - установленная на сокете маркировка неизвестна
		return;
	}
	// Запоминаем установленную на сокете маркировку
	this->_marking = marking;
}
/**
 * @brief Метод завершения сессии соединения
 *
 * @param oid идентификатор события сессии
 */
void awh::unit::QuicServer::erase(const event::id_t oid) noexcept {
	// Выполняем поиск сессии соединения
	auto i = this->_sessions.find(oid);
	// Если сессия соединения не найдена
	if(i == this->_sessions.end())
		// Выходим из метода
		return;
	// Если приложение было оповещено об установленном соединении
	if(i->second.connected)
		// Выполняем функцию обратного вызова о завершённом соединении
		this->_callback.call <void (const event::id_t, const quic::error_t)> ("close", oid, i->second.connection->error());
	// Удаляем сессию из списка сессий соединений
	this->_sessions.erase(i);
	// Уничтожаем событие сессии вместе с его ключами маршрутизации
	this->_io->destroy(oid);
}
/**
 * @brief Метод установки шаблона контекста безопасности соединений
 *
 * @param coder объект кодера транспортной безопасности
 * @param ctx   идентификатор шаблона контекста безопасности
 */
void awh::unit::QuicServer::context(const tls::coder_t & coder, const tls::coder_t::id_t ctx) noexcept {
	// Устанавливаем объект кодера транспортной безопасности
	this->_coder = &coder;
	// Устанавливаем идентификатор шаблона контекста безопасности
	this->_ctx = ctx;
}
/**
 * @brief Метод установки локальных транспортных параметров соединений (RFC 9000 §7.4)
 *
 * @param params локальные транспортные параметры
 */
void awh::unit::QuicServer::params(const quic::params::params_t & params) noexcept {
	// Устанавливаем локальные транспортные параметры соединений
	this->_params = params;
}
/**
 * @brief Метод установки проверки адреса клиента через пакет Retry (RFC 9000 §8.1.2)
 *
 * @param mode режим проверки адреса клиента
 */
void awh::unit::QuicServer::retry(const bool mode) noexcept {
	// Устанавливаем режим проверки адреса клиента
	this->_retry = mode;
}
/**
 * @brief Метод установки уведомления о перегрузке пути (RFC 9000 §13.4)
 *
 * @param mode режим уведомления о перегрузке пути
 */
void awh::unit::QuicServer::ecn(const bool mode) noexcept {
	// Устанавливаем режим уведомления о перегрузке пути
	this->_ecn = mode;
}
/**
 * @brief Метод установки общего ключа вывода токенов сброса (RFC 9000 §10.3.2)
 *
 * @param key общий ключ вывода токенов сброса
 */
void awh::unit::QuicServer::resetKey(string_view key) noexcept {
	// Устанавливаем общий ключ вывода токенов сброса
	this->_resetKey.assign(key.begin(), key.end());
}
/**
 * @brief Метод запуска сервера соединений
 *
 * @param family семейство адресов события сервера
 * @param ip     адрес прослушивания события сервера
 * @param port   порт прослушивания события сервера
 * @return       идентификатор созданного события сервера
 */
awh::event::id_t awh::unit::QuicServer::listen(const event::family_t family, string_view ip, const uint16_t port) noexcept {
	// Если событие сервера уже создано
	if(this->_eid != 0)
		// Выводим идентификатор созданного ранее события сервера
		return this->_eid;
	// Добавляем событие сервера поверх UDP
	this->_eid = this->_io->event(event::node_t::SERVER, family, event::type_t::DATAGRAM, event::protocol_t::UDP);
	// Добавляем событие интервала таймеров соединений
	this->_tid = this->_io->event(event::node_t::INTERVAL, event::family_t::TIMER);
	// Если события не созданы
	if((this->_eid == 0) || (this->_tid == 0)){
		// Записываем ошибку в лог
		this->_log->print("QUIC server events are not created", log_t::flag_t::CRITICAL);
		// Выводим отрицательный результат
		return 0;
	}
	// Запоминаем семейство адресов события сервера
	this->_family = ((family == event::family_t::IPV6) ? event::family_t::IPV6 : event::family_t::IPV4);
	/**
	 * Если общий ключ вывода токенов сброса не задан приложением: генерируем его
	 * случайно - сброс будет работать в пределах жизни процесса
	 */
	if(this->_resetKey.empty() && !quic::resetKey(this->_resetKey))
		// Записываем предупреждение в лог - сброс без сохранения состояния недоступен
		this->_log->print("QUIC stateless reset key is not generated", log_t::flag_t::WARNING);
	// Устанавливаем порт прослушивания события сервера
	this->_io->setSourcePort(this->_eid, port);
	// Устанавливаем интервал проверки таймеров соединений
	this->_io->setTimeout(this->_tid, event::action_t::NONE, 25);
	/**
	 * База событий инициализируется базовым модулем при его создании, поэтому
	 * повторная инициализация здесь не выполняется
	 */
	/**
	 * Устанавливаем опции события сервера: извлечение метаданных датаграмм
	 * включается только при уведомлении о перегрузке - оно переводит приём на
	 * разбор служебных сообщений и обходится дороже
	 */
	this->_io->setOptions(this->_eid, static_cast <uint16_t> (
		event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR |
		event::options::REUSE_PORT | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC |
		(this->_ecn ? event::options::DGRAM_INFO : 0)
	));
	// Если адрес прослушивания события сервера не установлен
	if(!this->_io->setAddress(this->_eid, ((family == event::family_t::IPV6) ? event::address_t::IPV6 : event::address_t::IPV4), ip)){
		// Записываем ошибку в лог
		this->_log->print("QUIC server address is not set", log_t::flag_t::CRITICAL);
		// Выводим отрицательный результат
		return 0;
	}
	/**
	 * Устанавливаем функцию обратного вызова определения сессии: соединение
	 * адресуется идентификатором соединения, а не четвёркой сокета, поэтому
	 * смена адреса клиента его не разрывает
	 */
	this->_io->on(this->_eid, static_cast <engine::callback::origin_t> (std::bind(&QuicServer::origin, this, _1, _2, _3, _4)));
	// Устанавливаем функцию обратного вызова на создание сессии соединения
	this->_io->on(this->_eid, static_cast <engine::callback::accept_t> (std::bind(&QuicServer::accept, this, _1, _2)));
	// Устанавливаем функцию обратного вызова на событие интервала таймеров соединений
	this->_io->on(this->_tid, static_cast <engine::callback::status_t> (std::bind(&QuicServer::tick, this, _1, _2)));
	// Если фиксация настроек событий не выполнена
	if(!this->_io->commit(this->_eid) || !this->_io->commit(this->_tid)){
		// Записываем ошибку в лог
		this->_log->print("QUIC server events are not committed", log_t::flag_t::CRITICAL);
		// Выводим отрицательный результат
		return 0;
	}
	// Если запуск событий не выполнен
	if(!this->_io->launch(this->_eid) || !this->_io->launch(this->_tid)){
		// Записываем ошибку в лог
		this->_log->print("QUIC server events are not launched", log_t::flag_t::CRITICAL);
		// Выводим отрицательный результат
		return 0;
	}
	// Если уведомление о перегрузке пути включено
	if(this->_ecn)
		// Помечаем исходящие датаграммы поддержкой ECN (RFC 9000 §13.4.1)
		this->mark(event::ecn_t::ECT0);
	// Выводим идентификатор созданного события сервера
	return this->_eid;
}
/**
 * @brief Метод открытия потока приложения соединения
 *
 * @param oid  идентификатор события сессии
 * @param mode режим однонаправленного потока
 * @return     идентификатор открытого потока
 */
uint64_t awh::unit::QuicServer::open(const event::id_t oid, const bool mode) noexcept {
	// Выполняем поиск сессии соединения
	auto i = this->_sessions.find(oid);
	// Если сессия соединения не найдена
	if(i == this->_sessions.end())
		// Выводим отрицательный результат
		return quic::connection_t::INVALID_STREAM;
	// Выводим идентификатор открытого потока приложения
	return i->second.connection->open(mode);
}
/**
 * @brief Метод отправки данных в поток приложения соединения
 *
 * @param oid  идентификатор события сессии
 * @param sid  идентификатор потока приложения
 * @param data отправляемые данные
 * @param fin  флаг завершения потока
 * @return     результат постановки данных в очередь отправки
 */
bool awh::unit::QuicServer::send(const event::id_t oid, const uint64_t sid, string_view data, const bool fin) noexcept {
	// Выполняем поиск сессии соединения
	auto i = this->_sessions.find(oid);
	// Если сессия соединения не найдена
	if(i == this->_sessions.end())
		// Выводим отрицательный результат
		return false;
	// Если постановка данных в очередь отправки не выполнена
	if(i->second.connection->send(sid, data, fin) != quic::status_t::OK)
		// Выводим отрицательный результат
		return false;
	// Отправляем все готовые исходящие датаграммы
	this->flush(oid, i->second);
	// Выводим положительный результат
	return true;
}
/**
 * @brief Метод отправки датаграммы приложения соединению (RFC 9221)
 *
 * @param oid  идентификатор события сессии
 * @param data данные датаграммы приложения
 * @return     результат отправки
 */
bool awh::unit::QuicServer::datagram(const event::id_t oid, string_view data) noexcept {
	// Выполняем поиск сессии соединения
	auto i = this->_sessions.find(oid);
	// Если сессия соединения не найдена
	if(i == this->_sessions.end())
		// Выводим отрицательный результат
		return false;
	// Если постановка датаграммы в очередь отправки не выполнена
	if(i->second.connection->datagram(data) != quic::status_t::OK)
		// Выводим отрицательный результат
		return false;
	// Отправляем все готовые исходящие датаграммы
	this->flush(oid, i->second);
	// Выводим положительный результат
	return true;
}
/**
 * @brief Метод получения предельного размера отправляемой датаграммы (RFC 9221 §3)
 *
 * @param oid идентификатор события сессии
 * @return    предельный размер данных датаграммы в октетах (0 - датаграммы не поддерживаются)
 */
size_t awh::unit::QuicServer::datagrams(const event::id_t oid) const noexcept {
	// Выполняем поиск сессии соединения
	auto i = this->_sessions.find(oid);
	// Если сессия соединения не найдена
	if(i == this->_sessions.end())
		// Выводим нулевой предел - отправка датаграмм невозможна
		return 0;
	// Выводим предельный размер данных отправляемой датаграммы
	return i->second.connection->datagrams();
}
/**
 * @brief Метод завершения соединения приложением (RFC 9000 §10.2)
 *
 * @param oid    идентификатор события сессии
 * @param code   код ошибки приложения
 * @param reason человекочитаемая причина завершения
 */
void awh::unit::QuicServer::close(const event::id_t oid, const uint64_t code, string_view reason) noexcept {
	// Выполняем поиск сессии соединения
	auto i = this->_sessions.find(oid);
	// Если сессия соединения не найдена
	if(i == this->_sessions.end())
		// Выходим из метода
		return;
	// Выполняем завершение соединения приложением
	i->second.connection->close(code, reason);
	// Отправляем фрейм завершения соединения удалённому эндпоинту
	this->flush(oid, i->second);
}
/**
 * @brief Метод получения согласованного ALPN-протокола соединения
 *
 * @param oid идентификатор события сессии
 * @return    согласованный ALPN-протокол
 */
awh::tls::coder_t::alpn_t awh::unit::QuicServer::alpn(const event::id_t oid) const noexcept {
	// Выполняем поиск сессии соединения
	auto i = this->_sessions.find(oid);
	// Если сессия соединения найдена
	if(i != this->_sessions.end())
		// Выводим согласованный ALPN-протокол соединения
		return i->second.connection->alpn();
	// Выводим пустой результат
	return tls::coder_t::alpn_t();
}
/**
 * @brief Метод получения адреса удалённого эндпоинта соединения
 *
 * @param oid идентификатор события сессии
 * @return    адрес удалённого эндпоинта в виде "адрес:порт"
 */
string awh::unit::QuicServer::address(const event::id_t oid) const noexcept {
	// Если сессия соединения не найдена
	if(this->_sessions.find(oid) == this->_sessions.end())
		// Выводим пустой результат
		return "";
	// Выводим адрес удалённого эндпоинта соединения
	return this->peer(oid);
}
/**
 * @brief Метод установки функций обратного вызова
 *
 * @param callback функции обратного вызова
 */
void awh::unit::QuicServer::callback(const callback_t & callback) noexcept {
	// Выполняем установку функции обратного вызова на установленное соединение
	this->_callback.set("open", callback);
	// Выполняем установку функции обратного вызова на собранные данные потока
	this->_callback.set("read", callback);
	// Выполняем установку функции обратного вызова на принятую датаграмму приложения
	this->_callback.set("datagram", callback);
	// Выполняем установку функции обратного вызова на завершённое соединение
	this->_callback.set("close", callback);
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::unit::QuicServer::QuicServer(const fmk_t * fmk, const log_t * log) noexcept :
 unit_t(fmk, log), _eid(0), _tid(0), _retry(false), _ecn(false), _family(event::family_t::IPV4),
 _marking(event::ecn_t::NOT_ECT), _resetKey{""}, _ctx(0), _coder(nullptr) {}
/**
 * @brief Деструктор
 *
 */
awh::unit::QuicServer::~QuicServer() noexcept {
	/**
	 * Перебираем список сессий соединений
	 */
	for(auto & item : this->_sessions){
		// Снимаем функцию обратного вызова на чтение датаграмм сессии
		this->_io->on(item.first, static_cast <engine::callback::read_t> (nullptr));
		// Уничтожаем событие сессии вместе с его ключами маршрутизации
		this->_io->destroy(item.first);
	}
	// Очищаем список сессий соединений
	this->_sessions.clear();
	// Если событие интервала таймеров соединений создано
	if(this->_tid != 0)
		// Уничтожаем событие интервала таймеров соединений
		this->_io->destroy(this->_tid);
	// Если событие сервера создано
	if(this->_eid != 0)
		// Уничтожаем событие сервера
		this->_io->destroy(this->_eid);
}

/**
 * @brief Метод получения текущего времени в миллисекундах
 *
 * @return текущее время в миллисекундах
 */
uint64_t awh::unit::QuicClient::date() const noexcept {
	// Выводим текущий штамп времени в миллисекундах
	return this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
}
/**
 * @brief Метод обработки принятой датаграммы соединения
 *
 * @param eid  идентификатор события клиента
 * @param data данные датаграммы
 * @param size размер датаграммы
 */
void awh::unit::QuicClient::read([[maybe_unused]] const event::id_t eid, const uint8_t * data, const size_t size) noexcept {
	// Если соединение не создано
	if(this->_connection == nullptr)
		// Выходим из метода
		return;
	// Если уведомление о перегрузке пути включено
	if(this->_ecn){
		/**
		 * Извлекаем метаданные принятой датаграммы с события клиента: маркировка
		 * снимается с заголовка IP-пакета при приёме
		 */
		const net::dgram_info_t info = this->_io->getTrafficInfo(this->_eid);
		// Выполняем обработку входящей датаграммы с маркировкой перегрузки
		this->_connection->read(data, size, this->date(), info.congestion);
	// Выполняем обработку входящей датаграммы
	} else this->_connection->read(data, size, this->date());
	// Если соединение установлено и приложение об этом ещё не оповещено
	if(!this->_connected && (this->_connection->state() == quic::connection_t::state_t::CONNECTED)){
		// Устанавливаем флаг оповещения приложения об установленном соединении
		this->_connected = true;
		// Выполняем функцию обратного вызова об установленном соединении
		this->_callback.call <void (const event::id_t)> ("open", this->_eid);
	}
	// Выполняем выдачу собранных данных потоков приложения
	this->process();
	// Отправляем все готовые исходящие датаграммы
	this->flush();
	/**
	 * Сохраняем присланный сервером билет возобновления: он приходит уже после
	 * установления соединения, поэтому проверяется на каждой датаграмме, пока
	 * не будет получен
	 */
	if(this->_ticket.empty()){
		// Извлекаем билет возобновления сессии соединения
		string ticket = this->_connection->session();
		// Если билет возобновления получен от сервера
		if(!ticket.empty())
			// Сохраняем билет возобновления для следующего подключения
			this->_ticket = ::move(ticket);
	}
	/**
	 * Сохраняем присланный сервером токен проверки адреса: он приходит фреймом
	 * NEW_TOKEN уже после установления соединения, поэтому проверяется на каждой
	 * датаграмме, пока не будет получен
	 */
	if(this->_token.empty())
		// Сохраняем токен проверки адреса для следующего подключения
		this->_token = this->_connection->token();
	// Если удалённый эндпоинт завершил соединение
	if(this->_connection->state() == quic::connection_t::state_t::DRAINING){
		// Если приложение было оповещено об установленном соединении
		if(this->_connected)
			// Выполняем функцию обратного вызова о завершённом соединении
			this->_callback.call <void (const event::id_t, const quic::error_t)> ("close", this->_eid, this->_connection->error());
		// Сбрасываем флаг оповещения приложения об установленном соединении
		this->_connected = false;
	}
}
/**
 * @brief Метод обработки просроченных таймеров соединения
 *
 * @param eid    идентификатор события интервала
 * @param status статус события интервала
 */
void awh::unit::QuicClient::tick([[maybe_unused]] const event::id_t eid, const event::status_t status) noexcept {
	// Если статус события интервала не успешен либо соединение не создано
	if((status != event::status_t::SUCCESS) || (this->_connection == nullptr))
		// Выходим из метода
		return;
	// Текущее время в миллисекундах
	const uint64_t date = this->date();
	// Дедлайн ближайшего события таймера соединения
	const uint64_t timeout = this->_connection->timeout();
	// Если дедлайн таймера соединения наступил
	if((timeout > 0) && (date >= timeout)){
		// Выполняем обработку просроченных таймеров соединения
		this->_connection->tick(date);
		// Отправляем все готовые исходящие датаграммы
		this->flush();
	}
}
/**
 * @brief Метод обработки завершения подключения к серверу
 *
 * @param eid идентификатор события клиента
 * @param ok  результат подключения к серверу
 */
void awh::unit::QuicClient::connected([[maybe_unused]] const event::id_t eid, const bool ok) noexcept {
	// Если подключение к серверу не выполнено либо соединение не создано
	if(!ok || (this->_connection == nullptr))
		// Выходим из метода
		return;
	/**
	 * Начинаем соединение: до завершения подключения событие к отправке не
	 * готово, поэтому хендшейк запускается именно здесь
	 */
	if(this->_connection->connect() != quic::status_t::OK){
		// Записываем ошибку в лог
		this->_log->print("QUIC connection is not started", log_t::flag_t::CRITICAL);
		// Выходим из метода
		return;
	}
	// Отправляем первый пакет хендшейка серверу
	this->flush();
}
/**
 * @brief Метод выдачи собранных данных потоков приложения
 *
 */
void awh::unit::QuicClient::process() noexcept {
	// Если функция обратного вызова на собранные данные потока установлена
	if(this->_callback.is("read")){
		// Список потоков с собранными данными
		vector <uint64_t> streams;
		// Получаем список потоков с собранными данными
		this->_connection->readable(streams);
		/**
		 * Перебираем потоки с собранными данными
		 */
		for(auto & sid : streams){
			// Флаг завершения потока удалённым эндпоинтом
			bool fin = false;
			// Собранные данные потока приложения
			string data = "";
			// Если выдача собранных данных потока не выполнена
			if(this->_connection->receive(sid, data, fin) != quic::status_t::OK)
				// Пропускаем поток с ошибкой выдачи
				continue;
			// Если данные получены либо поток завершён
			if(!data.empty() || fin)
				// Выполняем функцию обратного вызова на собранные данные потока
				this->_callback.call <void (const event::id_t, const uint64_t, const string &, const bool)> ("read", this->_eid, sid, data, fin);
		}
	}
	// Если функция обратного вызова на принятую датаграмму приложения установлена
	if(this->_callback.is("datagram")){
		// Буфер принятой датаграммы приложения
		string datagram = "";
		/**
		 * Выдаём принятые датаграммы приложения: они доставляются вне потоков
		 * и порядка доставки не имеют (RFC 9221)
		 */
		while(this->_connection->datagram(datagram))
			// Выполняем функцию обратного вызова на принятую датаграмму приложения
			this->_callback.call <void (const event::id_t, const string &)> ("datagram", this->_eid, datagram);
	}
}
/**
 * @brief Метод отправки готовых исходящих датаграмм соединения
 *
 */
void awh::unit::QuicClient::flush() noexcept {
	// Буфер исходящей датаграммы
	string datagram = "";
	/**
	 * Извлекаем исходящие датаграммы соединения
	 */
	while(this->_connection->write(datagram, this->date())){
		/**
		 * Применяем маркировку соединения к сокету: путь мог проверку поддержки
		 * ECN не пройти, и маркировать датаграммы далее нельзя
		 */
		this->mark(this->_connection->marking());
		// Если отправка датаграммы серверу не выполнена
		if(this->_io->send(this->_eid, datagram.data(), datagram.size()) == 0)
			// Записываем ошибку в лог
			this->_log->print("QUIC datagram is not sent: ID=%u", log_t::flag_t::CRITICAL, this->_eid);
	}
}
/**
 * @brief Метод применения маркировки соединения к сокету события клиента
 *
 * @param marking требуемая маркировка исходящих датаграмм
 */
void awh::unit::QuicClient::mark(const event::ecn_t marking) noexcept {
	// Если уведомление о перегрузке пути отключено либо маркировка уже установлена
	if(!this->_ecn || (marking == this->_marking))
		// Выходим из метода - смена маркировки не требуется
		return;
	// Если смена маркировки исходящих датаграмм не выполнена
	if(!this->_io->setExplicitCongestionNotification(this->_eid, this->_family, marking)){
		// Записываем предупреждение в лог
		this->_log->print("QUIC outgoing datagrams marking is not applied", log_t::flag_t::WARNING);
		// Выходим из метода - установленная на сокете маркировка неизвестна
		return;
	}
	// Запоминаем установленную на сокете маркировку
	this->_marking = marking;
}
/**
 * @brief Метод установки шаблона контекста безопасности соединения
 *
 * @param coder объект кодера транспортной безопасности
 * @param ctx   идентификатор шаблона контекста безопасности
 */
void awh::unit::QuicClient::context(const tls::coder_t & coder, const tls::coder_t::id_t ctx) noexcept {
	// Устанавливаем объект кодера транспортной безопасности
	this->_coder = &coder;
	// Устанавливаем идентификатор шаблона контекста безопасности
	this->_ctx = ctx;
}
/**
 * @brief Метод установки уведомления о перегрузке пути (RFC 9000 §13.4)
 *
 * @param mode режим уведомления о перегрузке пути
 */
void awh::unit::QuicClient::ecn(const bool mode) noexcept {
	// Устанавливаем режим уведомления о перегрузке пути
	this->_ecn = mode;
}
/**
 * @brief Метод установки локальных транспортных параметров соединения (RFC 9000 §7.4)
 *
 * @param params локальные транспортные параметры
 */
void awh::unit::QuicClient::params(const quic::params::params_t & params) noexcept {
	// Устанавливаем локальные транспортные параметры соединения
	this->_params = params;
}
/**
 * @brief Метод установки возобновления сессии сохранённым билетом (RFC 9001 §4.6)
 *
 * @param mode режим возобновления сессии
 */
void awh::unit::QuicClient::resume(const bool mode) noexcept {
	// Устанавливаем режим возобновления сессии
	this->_resume = mode;
}
/**
 * @brief Метод извлечения сохранённого билета возобновления сессии
 *
 * @return сериализованный билет возобновления (пусто - билет не получен)
 */
const string & awh::unit::QuicClient::session() const noexcept {
	// Выводим сохранённый билет возобновления сессии
	return this->_ticket;
}
/**
 * @brief Метод установки сохранённого билета возобновления сессии
 *
 * @param session сериализованный билет возобновления
 */
void awh::unit::QuicClient::session(string_view session) noexcept {
	// Устанавливаем сохранённый билет возобновления сессии
	this->_ticket.assign(session.begin(), session.end());
}
/**
 * @brief Метод извлечения сохранённого токена проверки адреса (RFC 9000 §8.1.3)
 *
 * @return токен проверки адреса (пусто - токен не получен)
 */
const string & awh::unit::QuicClient::token() const noexcept {
	// Выводим сохранённый токен проверки адреса
	return this->_token;
}
/**
 * @brief Метод установки сохранённого токена проверки адреса (RFC 9000 §8.1.3)
 *
 * @param token токен проверки адреса
 */
void awh::unit::QuicClient::token(string_view token) noexcept {
	// Устанавливаем сохранённый токен проверки адреса
	this->_token.assign(token.begin(), token.end());
}
/**
 * @brief Метод подключения к удалённому серверу
 *
 * @param family семейство адресов события клиента
 * @param ip     адрес удалённого сервера
 * @param port   порт удалённого сервера
 * @return       идентификатор созданного события клиента
 */
awh::event::id_t awh::unit::QuicClient::connect(const event::family_t family, string_view ip, const uint16_t port) noexcept {
	// Если шаблон контекста безопасности не установлен
	if((this->_coder == nullptr) || (this->_ctx == 0)){
		// Записываем ошибку в лог
		this->_log->print("QUIC security context is not set", log_t::flag_t::CRITICAL);
		// Выводим отрицательный результат
		return 0;
	}
	// Если событие клиента уже создано
	if(this->_eid != 0)
		// Выводим идентификатор созданного ранее события клиента
		return this->_eid;
	// Добавляем событие клиента поверх UDP
	this->_eid = this->_io->event(event::node_t::CLIENT, family, event::type_t::DATAGRAM, event::protocol_t::UDP);
	// Добавляем событие интервала таймеров соединения
	this->_tid = this->_io->event(event::node_t::INTERVAL, event::family_t::TIMER);
	// Если события не созданы
	if((this->_eid == 0) || (this->_tid == 0)){
		// Записываем ошибку в лог
		this->_log->print("QUIC client events are not created", log_t::flag_t::CRITICAL);
		// Выводим отрицательный результат
		return 0;
	}
	// Запоминаем семейство адресов события клиента
	this->_family = ((family == event::family_t::IPV6) ? event::family_t::IPV6 : event::family_t::IPV4);
	// Формируем адрес удалённого сервера
	this->_address = (string(ip) + ":" + std::to_string(port));
	// Создаём соединение QUIC на шаблоне контекста безопасности
	this->_connection = make_unique <quic::connection_t> (
		quic::endpoint_t::CLIENT, this->_ctx,
		* this->_coder, this->_log
	);
	// Устанавливаем локальные транспортные параметры соединения
	this->_connection->params(this->_params);
	// Устанавливаем адрес удалённого эндпоинта соединения
	this->_connection->address(this->_address);
	// Устанавливаем режим маркировки исходящих датаграмм соединения
	this->_connection->ecn(this->_ecn);
	/**
	 * Подставляем сохранённый билет возобновления: повторное соединение с тем же
	 * сервером обходится без полного хендшейка (RFC 9001 §4.6)
	 */
	if(this->_resume && !this->_ticket.empty())
		// Устанавливаем сохранённый билет возобновления соединению
		this->_connection->session(this->_ticket);
	/**
	 * Подставляем сохранённый токен проверки адреса: предъявление токена в первом
	 * пакете подтверждает адрес клиента сразу, и обмен пакетом Retry не выполняется
	 * (RFC 9000 §8.1.3)
	 */
	if(this->_resume && !this->_token.empty())
		// Устанавливаем сохранённый токен проверки адреса соединению
		this->_connection->token(this->_token);
	// Устанавливаем порт удалённого сервера
	this->_io->setTargetPort(this->_eid, port);
	// Устанавливаем интервал проверки таймеров соединения
	this->_io->setTimeout(this->_tid, event::action_t::NONE, 25);
	/**
	 * Устанавливаем опции события клиента: извлечение метаданных датаграмм
	 * включается только при уведомлении о перегрузке - оно переводит приём на
	 * разбор служебных сообщений и обходится дороже
	 */
	this->_io->setOptions(this->_eid, static_cast <uint16_t> (
		event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR |
		event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC |
		(this->_ecn ? event::options::DGRAM_INFO : 0)
	));
	// Если локальный адрес события клиента не установлен
	if(!this->_io->setAddress(this->_eid, ((family == event::family_t::IPV6) ? event::address_t::IPV6 : event::address_t::IPV4), ((family == event::family_t::IPV6) ? "::" : "0.0.0.0"))){
		// Записываем ошибку в лог
		this->_log->print("QUIC client address is not set", log_t::flag_t::CRITICAL);
		// Выводим отрицательный результат
		return 0;
	}
	// Если адрес удалённого сервера не установлен
	if(!this->_io->setTarget(this->_eid, ip)){
		// Записываем ошибку в лог
		this->_log->print("QUIC server address is not set", log_t::flag_t::CRITICAL);
		// Выводим отрицательный результат
		return 0;
	}
	// Устанавливаем функцию обратного вызова на чтение датаграмм соединения
	this->_io->on(this->_eid, static_cast <engine::callback::read_t> (std::bind(&QuicClient::read, this, _1, _2, _3)));
	// Устанавливаем функцию обратного вызова на завершение подключения к серверу
	this->_io->on(this->_eid, static_cast <engine::callback::connect_t> (std::bind(&QuicClient::connected, this, _1, _2)));
	// Устанавливаем функцию обратного вызова на событие интервала таймеров соединения
	this->_io->on(this->_tid, static_cast <engine::callback::status_t> (std::bind(&QuicClient::tick, this, _1, _2)));
	// Если фиксация настроек событий не выполнена
	if(!this->_io->commit(this->_eid) || !this->_io->commit(this->_tid)){
		// Записываем ошибку в лог
		this->_log->print("QUIC client events are not committed", log_t::flag_t::CRITICAL);
		// Выводим отрицательный результат
		return 0;
	}
	// Если подключение к удалённому серверу не выполнено
	if(!this->_io->connect(this->_eid)){
		// Записываем ошибку в лог
		this->_log->print("QUIC client is not connected", log_t::flag_t::CRITICAL);
		// Выводим отрицательный результат
		return 0;
	}
	// Если запуск событий не выполнен
	if(!this->_io->launch(this->_eid) || !this->_io->launch(this->_tid)){
		// Записываем ошибку в лог
		this->_log->print("QUIC client events are not launched", log_t::flag_t::CRITICAL);
		// Выводим отрицательный результат
		return 0;
	}
	// Выводим идентификатор созданного события клиента
	return this->_eid;
}
/**
 * @brief Метод открытия потока приложения соединения
 *
 * @param mode режим однонаправленного потока
 * @return     идентификатор открытого потока
 */
uint64_t awh::unit::QuicClient::open(const bool mode) noexcept {
	// Если соединение не создано
	if(this->_connection == nullptr)
		// Выводим отрицательный результат
		return quic::connection_t::INVALID_STREAM;
	// Выводим идентификатор открытого потока приложения
	return this->_connection->open(mode);
}
/**
 * @brief Метод отправки данных в поток приложения соединения
 *
 * @param sid  идентификатор потока приложения
 * @param data отправляемые данные
 * @param fin  флаг завершения потока
 * @return     результат постановки данных в очередь отправки
 */
bool awh::unit::QuicClient::send(const uint64_t sid, string_view data, const bool fin) noexcept {
	// Если соединение не создано
	if(this->_connection == nullptr)
		// Выводим отрицательный результат
		return false;
	// Если постановка данных в очередь отправки не выполнена
	if(this->_connection->send(sid, data, fin) != quic::status_t::OK)
		// Выводим отрицательный результат
		return false;
	// Отправляем все готовые исходящие датаграммы
	this->flush();
	// Выводим положительный результат
	return true;
}
/**
 * @brief Метод отправки датаграммы приложения серверу (RFC 9221)
 *
 * @param data данные датаграммы приложения
 * @return     результат отправки
 */
bool awh::unit::QuicClient::datagram(string_view data) noexcept {
	// Если соединение не создано
	if(this->_connection == nullptr)
		// Выводим отрицательный результат
		return false;
	// Если постановка датаграммы в очередь отправки не выполнена
	if(this->_connection->datagram(data) != quic::status_t::OK)
		// Выводим отрицательный результат
		return false;
	// Отправляем все готовые исходящие датаграммы
	this->flush();
	// Выводим положительный результат
	return true;
}
/**
 * @brief Метод получения предельного размера отправляемой датаграммы (RFC 9221 §3)
 *
 * @return предельный размер данных датаграммы в октетах (0 - датаграммы не поддерживаются)
 */
size_t awh::unit::QuicClient::datagrams() const noexcept {
	// Если соединение не создано
	if(this->_connection == nullptr)
		// Выводим нулевой предел - отправка датаграмм невозможна
		return 0;
	// Выводим предельный размер данных отправляемой датаграммы
	return this->_connection->datagrams();
}
/**
 * @brief Метод завершения соединения приложением (RFC 9000 §10.2)
 *
 * @param code   код ошибки приложения
 * @param reason человекочитаемая причина завершения
 */
void awh::unit::QuicClient::close(const uint64_t code, string_view reason) noexcept {
	// Если соединение не создано
	if(this->_connection == nullptr)
		// Выходим из метода
		return;
	// Выполняем завершение соединения приложением
	this->_connection->close(code, reason);
	// Отправляем фрейм завершения соединения удалённому серверу
	this->flush();
}
/**
 * @brief Метод получения согласованного ALPN-протокола соединения
 *
 * @return согласованный ALPN-протокол
 */
awh::tls::coder_t::alpn_t awh::unit::QuicClient::alpn() const noexcept {
	// Если соединение не создано
	if(this->_connection == nullptr)
		// Выводим пустой результат
		return tls::coder_t::alpn_t();
	// Выводим согласованный ALPN-протокол соединения
	return this->_connection->alpn();
}
/**
 * @brief Метод установки функций обратного вызова
 *
 * @param callback функции обратного вызова
 */
void awh::unit::QuicClient::callback(const callback_t & callback) noexcept {
	// Выполняем установку функции обратного вызова на установленное соединение
	this->_callback.set("open", callback);
	// Выполняем установку функции обратного вызова на собранные данные потока
	this->_callback.set("read", callback);
	// Выполняем установку функции обратного вызова на принятую датаграмму приложения
	this->_callback.set("datagram", callback);
	// Выполняем установку функции обратного вызова на завершённое соединение
	this->_callback.set("close", callback);
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::unit::QuicClient::QuicClient(const fmk_t * fmk, const log_t * log) noexcept :
 unit_t(fmk, log), _eid(0), _tid(0), _connected(false), _resume(false), _ecn(false),
 _family(event::family_t::IPV4), _marking(event::ecn_t::NOT_ECT), _ctx(0), _coder(nullptr),
 _ticket{""}, _token{""}, _address{""}, _connection(nullptr) {}
/**
 * @brief Деструктор
 *
 */
awh::unit::QuicClient::~QuicClient() noexcept {
	// Если событие интервала таймеров соединения создано
	if(this->_tid != 0)
		// Уничтожаем событие интервала таймеров соединения
		this->_io->destroy(this->_tid);
	// Если событие клиента создано
	if(this->_eid != 0){
		// Снимаем функцию обратного вызова на чтение датаграмм соединения
		this->_io->on(this->_eid, static_cast <engine::callback::read_t> (nullptr));
		// Уничтожаем событие клиента
		this->_io->destroy(this->_eid);
	}
	// Освобождаем объект соединения
	this->_connection.reset(nullptr);
}
