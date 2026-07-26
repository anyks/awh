/**
 * @file: quic.cpp
 * @date: 2026-07-22
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация модулей QUIC — связывание конечного автомата соединения QUIC с движком ввода-вывода:
 *        приём и отправка датаграмм, управление сессиями, миграция клиентского адреса,
 *        кластеризация сервера и возобновление сессии по 0-RTT
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include <units/quic.hpp>
#include <net/addr.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Используем пространство имён placeholders
 */
using namespace placeholders;

/**
 * @brief Инкапсулируем внутренние типы модуля в анонимное пространство имён
 *
 */
namespace {
	/**
	 * @brief Тип внутреннего сообщения между процессами кластера QUIC
	 *
	 */
	enum class quic_cluster_message_t : uint8_t {
		NONE = 0x00, // Сообщение не определено
		PORT = 0x01  // Сообщение несёт выделенный дочернему процессу порт прослушивания
	};
};

/**
 * @brief Конструктор сессии соединения
 *
 */
awh::unit::QuicServer::Session::Session() noexcept :
 connection(nullptr), connected(false), dest(0), tunnel(quic::connection_t::INVALID_STREAM) {}
/**
 * @brief Конструктор параметров кластера
 *
 */
awh::unit::QuicServer::ClusterParams::ClusterParams() noexcept :
 name{""}, rebirth(false), count(0),
 restartLimit(10), restartWindow(30000),
 mode(event::mode_t::DISABLED) {}

/**
 * @brief Метод получения текущего времени в миллисекундах
 *
 * @return текущее время в миллисекундах
 *
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
 *
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
 * @brief Метод установки адреса удалённого эндпоинта соединению из движка
 *
 * @param oid        идентификатор события сессии
 * @param connection соединение, которому устанавливается адрес эндпоинта
 *
 */
void awh::unit::QuicServer::endpoint(const event::id_t oid, quic::connection_t * connection) const noexcept {
	// Структура сетевого адреса удалённого эндпоинта сессии
	unique_ptr <net::addr_t> addr = nullptr;
	// Если структура адреса удалённого эндпоинта в семействе IPv4 не получена
	if(!this->_io->getAddress(oid, event::address_t::IPV4, addr))
		// Извлекаем структуру адреса удалённого эндпоинта в семействе IPv6
		this->_io->getAddress(oid, event::address_t::IPV6, addr);
	// Устанавливаем адрес удалённого эндпоинта соединению штатной структурой адреса и портом источника
	connection->address(addr.get(), this->_io->getSourcePort(oid));
}
/**
 * @brief Метод определения сессии принятой датаграммы (RFC 9000 §17.2)
 *
 * @param eid  идентификатор события сервера
 * @param data данные датаграммы
 * @param size размер датаграммы
 * @param key  выводимый ключ сессии
 * @return     результат определения сессии
 *
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
 *
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
	// Устанавливаем адрес удалённого эндпоинта соединения из движка
	this->endpoint(oid, session.connection.get());
	// Устанавливаем режим проверки адреса клиента через пакет Retry
	session.connection->retry(this->_retry);
	// Устанавливаем режим маркировки исходящих датаграмм соединения
	session.connection->ecn(this->_ecn);
	// Устанавливаем общий ключ вывода токенов сброса без сохранения состояния
	session.connection->resetKey(this->_resetKey);
	// Устанавливаем функцию обратного вызова на чтение датаграмм сессии
	this->_io->on(oid, static_cast <engine::callback::read_t> (std::bind(&QuicServer::read, this, _1, _2, _3)));
	/**
	 * Устанавливаем функцию обратного вызова на инъекцию объединённых данных сессии:
	 * перенаправленные из события-источника байты отправляются туннельным потоком
	 * сессии с шифрованием на уровне соединения, а не пишутся сырьём в сокет (splice)
	 */
	this->_io->on(oid, static_cast <engine::callback::inject_t> (std::bind(&QuicServer::inject, this, _1, _2, _3)));
}
/**
 * @brief Метод обработки принятой датаграммы сессии
 *
 * @param oid  идентификатор события сессии
 * @param data данные датаграммы
 * @param size размер датаграммы
 *
 */
void awh::unit::QuicServer::read(const event::id_t oid, const uint8_t * data, const size_t size) noexcept {
	// Выполняем поиск сессии соединения
	auto i = this->_sessions.find(oid);
	// Если сессия соединения не найдена
	if(i == this->_sessions.end())
		// Выходим из метода
		return;
	/**
	 * Сессию ведём указателем, а не удерживаемой ссылкой: ниже вызываются функции
	 * обратного вызова приложения ("open", а также "read"/"datagram" внутри process),
	 * и приложение вправе реентрантно завершить соединение прямо в них. Тогда узел
	 * сессии освобождается, и удерживаемая ссылка становится висячей. Поэтому после
	 * каждого колбэка сессию перечитываем из карты и прекращаем обработку, если она снята
	 */
	session_t * session = &i->second;
	/**
	 * Обновляем адрес удалённого эндпоинта перед разбором датаграммы: смена
	 * адреса при установленном соединении означает миграцию на новый путь,
	 * которую соединение отслеживает самостоятельно (RFC 9000 §9)
	 */
	this->endpoint(oid, session->connection.get());
	// Если уведомление о перегрузке пути включено
	if(this->_ecn){
		/**
		 * Извлекаем метаданные принятой датаграммы с события сервера: маркировка
		 * снимается с заголовка IP-пакета при приёме, а сессии её не хранят
		 */
		const net::dgram_info_t info = this->_io->getTrafficInfo(this->_eid);
		// Выполняем обработку входящей датаграммы с маркировкой перегрузки
		session->connection->read(data, size, this->date(), info.congestion);
	// Выполняем обработку входящей датаграммы
	} else session->connection->read(data, size, this->date());
	// Синхронизируем маршрутизацию по идентификаторам соединения
	this->reroute(oid, * session);
	// Если соединение установлено и приложение об этом ещё не оповещено
	if(!session->connected && (session->connection->state() == quic::connection_t::state_t::CONNECTED)){
		// Устанавливаем флаг оповещения приложения об установленном соединении
		session->connected = true;
		// Выполняем функцию обратного вызова об установленном соединении
		this->_callback.call <void (const event::id_t)> ("open", oid);
		// Перечитываем сессию: колбэк "open" мог реентрантно снять её
		i = this->_sessions.find(oid);
		// Если сессия соединения снята приложением из функции обратного вызова
		if(i == this->_sessions.end())
			// Выходим из метода
			return;
		// Обновляем указатель на сессию соединения
		session = &i->second;
	}
	// Выполняем выдачу собранных данных потоков приложения
	this->process(oid);
	/**
	 * Перечитываем сессию после выдачи данных: process() вызывает колбэки
	 * "read"/"datagram", в которых приложение могло реентрантно снять сессию
	 */
	i = this->_sessions.find(oid);
	// Если сессия соединения снята приложением из функции обратного вызова
	if(i == this->_sessions.end())
		// Выходим из метода
		return;
	// Обновляем указатель на сессию соединения
	session = &i->second;
	// Отправляем все готовые исходящие датаграммы
	const bool sent = this->flush(oid, * session);
	/**
	 * Если соединение не начато и отправлять в ответ нечего: датаграмма адресована
	 * соединению, о котором сервер ничего не помнит. Отвечаем сбросом без сохранения
	 * состояния и снимаем сессию - иначе удалённый узел будет слать датаграммы
	 * до самого таймаута простоя (RFC 9000 §10.3)
	 */
	if(!sent && (session->connection->state() == quic::connection_t::state_t::NONE)){
		// Отправляем сброс без сохранения состояния
		this->drop(oid, data, size);
		// Выполняем завершение сессии соединения
		this->erase(oid);
		// Выходим из метода
		return;
	}
	// Если удалённый эндпоинт завершил соединение
	if(session->connection->state() == quic::connection_t::state_t::DRAINING)
		// Выполняем завершение сессии соединения
		this->erase(oid);
}
/**
 * @brief Метод обработки просроченных таймеров соединений
 *
 * @param eid    идентификатор события интервала
 * @param status статус события интервала
 *
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
 *
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
 *
 */
void awh::unit::QuicServer::process(const event::id_t oid) noexcept {
	/**
	 * Сессию берём по идентификатору, а не по удерживаемой ссылке: выдача данных
	 * вызывает функции обратного вызова приложения ("read"/"datagram"), а приложение
	 * вправе реентрантно завершить соединение прямо в них (destroy/erase). Тогда узел
	 * сессии освобождается, и любая ранее взятая ссылка session_t& становится висячей.
	 * Поэтому сессию перечитываем из карты после каждого колбэка и прекращаем выдачу,
	 * если она была снята
	 */
	auto i = this->_sessions.find(oid);
	// Если сессия соединения не найдена
	if(i == this->_sessions.end())
		// Выходим из метода
		return;
	/**
	 * Если для сессии установлено объединение данных (splice), собранные данные
	 * потоков и датаграмм перенаправляются в событие-приёмник вместо выдачи
	 * приложению - строится прозрачный канал между сессией и другим событием
	 */
	const bool spliced = (i->second.dest != 0);
	// Если объединение установлено либо функция обратного вызова на собранные данные потока установлена
	if(spliced || this->_callback.is("read")){
		// Список потоков с собранными данными
		vector <uint64_t> streams;
		// Получаем список потоков с собранными данными
		i->second.connection->readable(streams);
		/**
		 * Перебираем потоки с собранными данными
		 */
		for(auto & sid : streams){
			// Перечитываем сессию: предыдущий колбэк мог реентрантно снять её
			i = this->_sessions.find(oid);
			// Если сессия соединения снята приложением из функции обратного вызова
			if(i == this->_sessions.end())
				// Выходим из метода
				return;
			// Флаг завершения потока удалённым эндпоинтом
			bool fin = false;
			// Собранные данные потока приложения
			string data = "";
			// Если выдача собранных данных потока не выполнена
			if(i->second.connection->receive(sid, data, fin) != quic::status_t::OK)
				// Пропускаем поток с ошибкой выдачи
				continue;
			// Если данные получены либо поток завершён
			if(!data.empty() || fin){
				// Если для сессии установлено объединение данных - перенаправляем их в событие-приёмник
				if(spliced)
					// Перенаправляем собранные данные потока в событие-приёмник объединения
					this->forward(i->second.dest, data);
				// Иначе выдаём собранные данные потока приложению
				else this->_callback.call <void (const event::id_t, const uint64_t, const string &, const bool)> ("read", oid, sid, data, fin);
			}
		}
	}
	// Перечитываем сессию перед выдачей датаграмм: цикл потоков мог реентрантно снять её
	i = this->_sessions.find(oid);
	// Если сессия соединения снята приложением из функции обратного вызова
	if(i == this->_sessions.end())
		// Выходим из метода
		return;
	// Если объединение установлено либо функция обратного вызова на принятую датаграмму приложения установлена
	if(spliced || this->_callback.is("datagram")){
		// Буфер принятой датаграммы приложения
		string datagram = "";
		/**
		 * Выдаём принятые датаграммы приложения: они доставляются вне потоков
		 * и порядка доставки не имеют (RFC 9221)
		 */
		while(i->second.connection->datagram(datagram)){
			// Если для сессии установлено объединение данных - перенаправляем датаграмму в событие-приёмник
			if(spliced)
				// Перенаправляем принятую датаграмму в событие-приёмник объединения
				this->forward(i->second.dest, datagram);
			// Иначе выдаём принятую датаграмму приложению
			else this->_callback.call <void (const event::id_t, const string &)> ("datagram", oid, datagram);
			// Перечитываем сессию: колбэк датаграммы мог реентрантно снять её
			i = this->_sessions.find(oid);
			// Если сессия соединения снята приложением из функции обратного вызова
			if(i == this->_sessions.end())
				// Выходим из метода
				return;
		}
	}
}
/**
 * @brief Метод отправки готовых исходящих датаграмм соединения
 *
 * @param oid     идентификатор события сессии
 * @param session сессия соединения
 *
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
 * @brief Метод отправки объединённых данных в туннельный поток сессии
 *
 * @param oid  идентификатор события сессии-приёмника
 * @param data данные для отправки в туннельный поток
 * @param size размер данных для отправки
 * @return     результат постановки данных в очередь отправки
 *
 */
bool awh::unit::QuicServer::inject(const event::id_t oid, const uint8_t * data, const size_t size) noexcept {
	// Если данные для отправки отсутствуют
	if((data == nullptr) || (size == 0))
		// Возвращаем значение по умолчанию
		return false;
	// Выполняем поиск сессии-приёмника объединения данных
	auto i = this->_sessions.find(oid);
	// Если сессия-приёмник не найдена либо соединение не создано
	if((i == this->_sessions.end()) || (i->second.connection == nullptr))
		// Возвращаем значение по умолчанию
		return false;
	// Если туннельный поток входящих объединённых данных ещё не открыт
	if(i->second.tunnel == quic::connection_t::INVALID_STREAM)
		// Открываем двунаправленный туннельный поток приложения (RFC 9000 §2.1)
		i->second.tunnel = i->second.connection->open(false);
	// Если туннельный поток открыть не удалось
	if(i->second.tunnel == quic::connection_t::INVALID_STREAM)
		// Возвращаем значение по умолчанию
		return false;
	// Если постановка байт в очередь отправки туннельного потока не выполнена
	if(i->second.connection->send(i->second.tunnel, string_view(reinterpret_cast <const char *> (data), size), false) != quic::status_t::OK)
		// Возвращаем значение по умолчанию
		return false;
	// Отправляем готовые исходящие датаграммы удалённому эндпоинту
	this->flush(oid, i->second);
	// Возвращаем положительный результат
	return true;
}
/**
 * @brief Метод перенаправления собранных данных сессии в событие-приёмник объединения
 *
 * @param dest идентификатор события-приёмника объединения данных
 * @param data данные для перенаправления
 *
 */
void awh::unit::QuicServer::forward(const event::id_t dest, string_view data) noexcept {
	// Если данные для перенаправления присутствуют и приёмник задан
	if(!data.empty() && (dest != 0))
		/**
		 * Перенаправляем данные в событие-приёмник через сетевой движок: если приёмник
		 * шифрует данные на уровне соединения (QUIC-сессия, в т.ч. другого юнита) - они
		 * отправляются его туннельным потоком через функцию инъекции, иначе (TCP/UDP) -
		 * пишутся сырыми байтами в сокет
		 */
		this->_io->relay(dest, data.data(), data.size());
}
/**
 * @brief Метод отправки сброса без сохранения состояния (RFC 9000 §10.3)
 *
 * @param oid  идентификатор события сессии
 * @param data данные вызвавшей сброс датаграммы
 * @param size размер вызвавшей сброс датаграммы
 * @return     результат отправки
 *
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
 *
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
 *
 */
void awh::unit::QuicServer::erase(const event::id_t oid) noexcept {
	// Выполняем поиск сессии соединения
	auto i = this->_sessions.find(oid);
	// Если сессия соединения не найдена
	if(i == this->_sessions.end())
		// Выходим из метода
		return;
	// Признак оповещения приложения об установленном соединении
	const bool connected = i->second.connected;
	// Код ошибки завершения соединения для оповещения приложения
	const quic::error_t error = (connected ? i->second.connection->error() : quic::error_t::NO_ERROR);
	/**
	 * Удаляем сессию из карты ДО функции обратного вызова "close": приложение вправе
	 * реентрантно из неё вызвать destroy(oid) на этой же сессии, а раннее удаление
	 * делает вложенный erase(oid) no-op'ом и снимает риск двойного удаления итератора
	 */
	this->_sessions.erase(i);
	// Если приложение было оповещено об установленном соединении
	if(connected)
		// Выполняем функцию обратного вызова о завершённом соединении
		this->_callback.call <void (const event::id_t, const quic::error_t)> ("close", oid, error);
	// Уничтожаем событие сессии вместе с его ключами маршрутизации
	this->_io->destroy(oid);
}
/**
 * @brief Метод установки шаблона контекста безопасности соединений
 *
 * @param coder объект кодера транспортной безопасности
 * @param ctx   идентификатор шаблона контекста безопасности
 *
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
 *
 */
void awh::unit::QuicServer::params(const quic::params::params_t & params) noexcept {
	// Устанавливаем локальные транспортные параметры соединений
	this->_params = params;
}
/**
 * @brief Метод установки проверки адреса клиента через пакет Retry (RFC 9000 §8.1.2)
 *
 * @param mode режим проверки адреса клиента
 *
 */
void awh::unit::QuicServer::retry(const bool mode) noexcept {
	// Устанавливаем режим проверки адреса клиента
	this->_retry = mode;
}
/**
 * @brief Метод установки уведомления о перегрузке пути (RFC 9000 §13.4)
 *
 * @param mode режим уведомления о перегрузке пути
 *
 */
void awh::unit::QuicServer::ecn(const bool mode) noexcept {
	// Устанавливаем режим уведомления о перегрузке пути
	this->_ecn = mode;
}
/**
 * @brief Метод установки общего ключа вывода токенов сброса (RFC 9000 §10.3.2)
 *
 * @param key общий ключ вывода токенов сброса
 *
 */
void awh::unit::QuicServer::resetKey(string_view key) noexcept {
	// Устанавливаем общий ключ вывода токенов сброса
	this->_resetKey.assign(key.begin(), key.end());
}
/**
 * @brief Метод проверки актуальности события сервера
 *
 * @param eid идентификатор события
 * @return    результат проверки актуальности события
 *
 */
bool awh::unit::QuicServer::isActual(const event::id_t eid) const noexcept {
	// Событие актуально, если это событие сервера (в т.ч. унаследованное фасадом) либо активная сессия соединения
	return (((this->_eid != 0) && (this->actual(eid) == this->_eid)) || (this->_sessions.find(eid) != this->_sessions.end()));
}
/**
 * @brief Метод приведения переданного фасадом идентификатора события сервера к актуальному
 *
 * @param eid идентификатор события, переданный фасадом
 * @return    актуальный идентификатор события сервера
 *
 */
awh::event::id_t awh::unit::QuicServer::actual(const event::id_t eid) const noexcept {
	/**
	 * В режиме кластера дочерний процесс пересоздаёт собственное событие сервера с
	 * новым идентификатором, а фасад продолжает обращаться к унаследованному до fork
	 * идентификатору: подменяем унаследованный идентификатор на собственный. Активная
	 * сессия с таким же идентификатором (при повторном выделении) имеет приоритет
	 */
	if((this->_eid != 0) && (eid != 0) && (eid == this->_inheritedEid) && (this->_sessions.find(eid) == this->_sessions.end()))
		// Возвращаем собственный идентификатор события сервера дочернего процесса
		return this->_eid;
	// Возвращаем переданный фасадом идентификатор события без изменения
	return eid;
}
/**
 * @brief Метод получения семейства адресов события сервера
 *
 * @param eid идентификатор события сервера
 * @return    семейство адресов события сервера
 *
 */
awh::event::family_t awh::unit::QuicServer::family(const event::id_t eid) const noexcept {
	// Приводим унаследованный дочерним процессом кластера идентификатор к собственному
	return this->unit_t::family(this->actual(eid));
}
/**
 * @brief Метод запуска/остановки работы сервера
 *
 * @param status статус запуска/остановки сервера
 *
 */
void awh::unit::QuicServer::launch(const event::status_t status) noexcept {
	/**
	 * Определяем статус работы сервера
	 */
	switch(static_cast <uint8_t> (status)){
		// Если работа сервера запущена
		case static_cast <uint8_t> (event::status_t::LAUNCHED):
			/**
			 * В режиме кластера дочерний процесс запускает цикл событий раньше, чем
			 * получает выделенный родительским процессом порт: пока сокет не привязан,
			 * приложение о запуске не оповещается - оповещение выполнит message()
			 * после фактической привязки сокета
			 */
			if(!this->_awaitingPort)
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const event::status_t)> ("server_status", status);
		break;
		// Если работа сервера подлежит уничтожению
		case static_cast <uint8_t> (event::status_t::DESTROYED): {
			// Завершаем все сессии соединений сервера
			if(!this->_sessions.empty()){
				// Копируем список идентификаторов сессий для безопасного удаления во время итерации
				vector <event::id_t> sessions;
				// Резервируем память под список идентификаторов сессий
				sessions.reserve(this->_sessions.size());
				// Формируем список идентификаторов сессий соединений
				for(const auto & item : this->_sessions)
					// Добавляем идентификатор сессии соединения в список
					sessions.push_back(item.first);
				// Завершаем все сессии соединений
				for(const auto & oid : sessions)
					// Завершаем сессию соединения
					this->erase(oid);
			}
			// Если событие интервала таймеров соединений создано
			if(this->_tid != 0){
				// Уничтожаем событие интервала таймеров соединений
				this->_io->destroy(this->_tid);
				// Сбрасываем идентификатор события интервала таймеров соединений
				this->_tid = 0;
			}
			// Если событие сервера создано
			if(this->_eid != 0){
				// Снимаем функцию обратного вызова определения сессии
				this->_io->on(this->_eid, static_cast <engine::callback::origin_t> (nullptr));
				// Снимаем функцию обратного вызова на создание сессии соединения
				this->_io->on(this->_eid, static_cast <engine::callback::accept_t> (nullptr));
				// Уничтожаем событие сервера
				this->_io->destroy(this->_eid);
				// Сбрасываем идентификатор события сервера
				this->_eid = 0;
			}
			// Выполняем получение идентификатора функции обратного вызова
			const callback_t::id_t fid = this->_callback.id("server_status");
			// Если функция обратного вызова установлена
			if(this->_callback.is(fid)){
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const event::status_t)> (fid, status);
				// Выполняем получение функции обратного вызова
				this->_callback.set(fid, this->_callback.id("status"), this->_callback);
			}
		} break;
	}
}
/**
 * @brief Метод создания события сервера QUIC поверх UDP
 *
 * @param family   семейство адресов события сервера
 * @param type     тип события (игнорируется, приводится к DATAGRAM)
 * @param protocol протокол события (игнорируется, приводится к UDP)
 * @return         идентификатор созданного события сервера
 *
 */
awh::event::id_t awh::unit::QuicServer::issue(const event::family_t family, [[maybe_unused]] const event::type_t type, [[maybe_unused]] const event::protocol_t protocol) noexcept {
	// Если событие сервера уже создано
	if(this->_eid != 0)
		// Выводим идентификатор созданного ранее события сервера
		return this->_eid;
	// Добавляем событие сервера поверх UDP (транспорт QUIC работает поверх дейтаграммного UDP-сокета)
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
	// Устанавливаем интервал проверки таймеров соединений
	this->_io->setTimeout(this->_tid, event::action_t::NONE, 25);
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
	// Выводим идентификатор созданного события сервера
	return this->_eid;
}
/**
 * @brief Метод фиксации настроек события сервера
 *
 * @param eid идентификатор события сервера
 * @return    результат выполнения фиксации
 *
 */
bool awh::unit::QuicServer::commit(const event::id_t eid) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid)){
		/**
		 * В режиме кластера фиксация настроек (привязка сокета) выполняется в каждом
		 * дочернем процессе после fork на собственном сокете, поэтому на событии
		 * родительского процесса привязка не производится (см. cluster())
		 */
		if((this->_clusterParams.mode == event::mode_t::ENABLED) && (eid == this->_eid))
			// Возвращаем положительный результат - фиксация отложена в дочерние процессы
			return true;
		// Выполняем фиксацию настроек события сервера и события интервала таймеров соединений
		return (this->_io->commit(this->_eid) && this->_io->commit(this->_tid));
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод запуска работы события сервера
 *
 * @param eid идентификатор события сервера
 * @return    результат выполнения запуска
 *
 */
bool awh::unit::QuicServer::launch(const event::id_t eid) noexcept {
	/**
	 * В режиме кластера дочерний процесс пересоздаёт собственное событие сервера с
	 * новым идентификатором (см. message()), который не совпадает с переданным фасадом
	 * идентификатором родительского процесса, поэтому запуск ведётся по собственному
	 * событию сервера без проверки актуальности переданного идентификатора
	 */
	if(this->_clusterParams.mode == event::mode_t::ENABLED){
		// Если собственное событие сервера дочернего процесса не создано
		if(this->_eid == 0)
			// Возвращаем значение по умолчанию
			return false;
	// Если событие сервера не актуально
	} else if(!this->isActual(eid))
		// Возвращаем значение по умолчанию
		return false;
	// Если запуск события сервера и события интервала таймеров соединений не выполнен
	if(!this->_io->launch(this->_eid) || !this->_io->launch(this->_tid))
		// Возвращаем значение по умолчанию
		return false;
	// Если уведомление о перегрузке пути включено
	if(this->_ecn)
		// Помечаем исходящие датаграммы поддержкой ECN (RFC 9000 §13.4.1)
		this->mark(event::ecn_t::ECT0);
	// Возвращаем положительный результат
	return true;
}
/**
 * @brief Метод прослушивания порта сервера для приёма входящих соединений
 *
 * @param eid идентификатор события сервера
 * @param max максимальный размер очереди ожидания соединений
 * @return    результат выполнения прослушивания
 *
 */
bool awh::unit::QuicServer::listen(const event::id_t eid, const uint32_t max) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid)){
		// Кэшируем размер очереди ожидания для пересоздания сокета в дочернем процессе кластера
		if(eid == this->_eid)
			// Сохраняем максимальный размер очереди ожидания соединений
			this->_backlog = max;
		/**
		 * Транспорт QUIC работает поверх дейтаграммного UDP-сокета: приём входящих
		 * соединений ведётся приёмом датаграмм по событию сервера и демультиплексацией
		 * по идентификатору соединения, поэтому отдельного перевода сокета в режим
		 * прослушивания (доступного лишь потоковым транспортам STREAM/SEQPACKET) не
		 * требуется - размер очереди лишь сохраняется
		 */
		return true;
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод приостановки работы события сервера
 *
 * @param eid идентификатор события сервера
 * @return    результат выполнения приостановки работы
 *
 */
bool awh::unit::QuicServer::pause(const event::id_t eid) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем приостановку работы события сервера
		return this->_io->pause(this->_eid);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод возобновления работы события сервера
 *
 * @param eid идентификатор события сервера
 * @return    результат выполнения возобновления работы
 *
 */
bool awh::unit::QuicServer::resume(const event::id_t eid) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем возобновление работы события сервера
		return this->_io->resume(this->_eid);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения данных от клиента
 *
 * @param eid идентификатор события клиента
 * @return    результат получения данных
 *
 */
bool awh::unit::QuicServer::recv([[maybe_unused]] const event::id_t eid) noexcept {
	// Для транспорта QUIC приём датаграмм сессий выполняется самим модулем по событию сервера
	return false;
}
/**
 * @brief Метод отправки данных клиенту
 *
 * @param eid    идентификатор события сессии
 * @param buffer буфер данных для отправки
 * @param size   размер данных для отправки
 * @return       количество байт, поставленных в очередь отправки
 *
 */
size_t awh::unit::QuicServer::send(const event::id_t eid, const void * buffer, const size_t size) noexcept {
	// Если данные для отправки отсутствуют
	if((buffer == nullptr) || (size == 0))
		// Возвращаем значение по умолчанию
		return 0;
	// Выполняем поиск сессии соединения по идентификатору события сессии
	auto i = this->_sessions.find(eid);
	// Если сессия соединения не найдена либо соединение не создано
	if((i == this->_sessions.end()) || (i->second.connection == nullptr))
		// Возвращаем значение по умолчанию
		return 0;
	// Открываем двунаправленный поток приложения по умолчанию (RFC 9000 §2.1)
	const uint64_t sid = i->second.connection->open(false);
	// Если поток приложения не открыт
	if(sid == quic::connection_t::INVALID_STREAM)
		// Возвращаем значение по умолчанию
		return 0;
	// Если постановка данных в очередь отправки потока выполнена
	if(this->send(eid, sid, string_view(reinterpret_cast <const char *> (buffer), size), false))
		// Выводим количество поставленных в очередь отправки байт
		return size;
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод объединения потоков данных между двумя событиями
 *
 * @param eid  идентификатор события-источника
 * @param dest идентификатор события-приёмника
 * @return     результат объединения
 *
 */
bool awh::unit::QuicServer::splice(const event::id_t eid, const event::id_t dest) noexcept {
	/**
	 * Объединение данных QUIC ведётся на уровне сессии, а не сокета: собранные
	 * (расшифрованные) данные потоков сессии-источника перенаправляются в
	 * событие-приёмник, а входящие в сессию объединённые байты отправляются
	 * туннельным потоком с шифрованием на уровне соединения. Направление
	 * задаётся отдельным вызовом на каждую сторону (как для прочих транспортов)
	 */
	// Выполняем поиск сессии-источника объединения данных
	auto i = this->_sessions.find(eid);
	// Если сессия-источник найдена (прямое направление: данные сессии -> приёмнику)
	if(i != this->_sessions.end()){
		// Устанавливаем событие-приёмник объединения данных сессии
		i->second.dest = dest;
		/**
		 * Сразу выдаём уже собранные данные потоков, накопленные до установки
		 * приёмника: без нового входящего события метод process() иначе не будет
		 * вызван и буферизованные данные не пойдут в событие-приёмник
		 */
		this->process(eid);
		// Возвращаем положительный результат
		return true;
	}
	/**
	 * Обратное направление: приёмник - сессия, а источник (eid) - внешнее событие
	 * (напр. TCP-бэкенд). Привязываем источник к сессии на уровне сетевого движка:
	 * входящие в источник байты будут инъецированы в туннельный поток сессии, минуя
	 * выдачу приложению (см. inject). Так обеспечивается симметрия с QuicClient::splice
	 */
	if(this->_sessions.find(dest) != this->_sessions.end())
		// Объединяем внешнее событие-источник с сессией-приёмником на уровне движка
		return this->_io->splice(eid, dest);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки контекста события клиента
 *
 * @param eid идентификатор события клиента
 * @param ctx контекст события клиента
 * @return    результат выполнения установки
 *
 */
bool awh::unit::QuicServer::setContext([[maybe_unused]] const event::id_t eid, [[maybe_unused]] void * ctx) noexcept {
	// Для транспорта QUIC контекст сессии ведётся самим модулем по идентификатору события сессии
	return false;
}
/**
 * @brief Метод уничтожения события сервера
 *
 * @param eid идентификатор события для уничтожения
 *
 */
void awh::unit::QuicServer::destroy(const event::id_t eid) noexcept {
	// Если событие сервера не актуально
	if(!this->isActual(eid))
		// Выходим из метода
		return;
	/**
	 * Если идентификатор адресует отдельную сессию, а не событие сервера: завершаем
	 * только её. Иначе уничтожение одной сессии (в т.ч. реентрантно из функции
	 * обратного вызова) сносило бы весь сервер со всеми прочими сессиями, событием
	 * приёма и таймером - для отдельной сессии это неверно (RFC 9000 §10)
	 */
	if(this->_sessions.find(eid) != this->_sessions.end()){
		// Завершаем сессию соединения
		this->erase(eid);
		// Выходим из метода
		return;
	}
	// Завершаем все сессии соединений сервера
	if(!this->_sessions.empty()){
		// Копируем список идентификаторов сессий для безопасного удаления во время итерации
		vector <event::id_t> sessions;
		// Резервируем память под список идентификаторов сессий
		sessions.reserve(this->_sessions.size());
		// Формируем список идентификаторов сессий соединений
		for(const auto & item : this->_sessions)
			// Добавляем идентификатор сессии соединения в список
			sessions.push_back(item.first);
		// Завершаем все сессии соединений
		for(const auto & oid : sessions)
			// Завершаем сессию соединения
			this->erase(oid);
	}
	/**
	 * Если событие сервера уже уничтожено реентрантно: приложение вправе из функции
	 * обратного вызова "close" (вызванной erase выше) уничтожить сервер целиком, тогда
	 * _eid уже обнулён вложенным вызовом, и дальнейший снос по нулевому идентификатору
	 * лишь повторял бы работу вхолостую
	 */
	if(this->_eid == 0)
		// Выходим из метода
		return;
	// Если событие интервала таймеров соединений создано
	if(this->_tid != 0){
		// Уничтожаем событие интервала таймеров соединений
		this->_io->destroy(this->_tid);
		// Сбрасываем идентификатор события интервала таймеров соединений
		this->_tid = 0;
	}
	// Снимаем функцию обратного вызова определения сессии
	this->_io->on(this->_eid, static_cast <engine::callback::origin_t> (nullptr));
	// Снимаем функцию обратного вызова на создание сессии соединения
	this->_io->on(this->_eid, static_cast <engine::callback::accept_t> (nullptr));
	// Уничтожаем событие сервера
	this->_io->destroy(this->_eid);
	// Сбрасываем идентификатор события сервера
	this->_eid = 0;
}
/**
 * @brief Метод получения опций события сервера
 *
 * @param eid идентификатор события сервера
 * @return    опции события сервера
 *
 */
uint16_t awh::unit::QuicServer::getOptions(const event::id_t eid) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение опций для события сервера
		return this->_io->getOptions(this->_eid);
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки опций события сервера
 *
 * @param eid     идентификатор события сервера
 * @param options опции события сервера для установки
 * @return        результат выполнения установки
 *
 */
bool awh::unit::QuicServer::setOptions(const event::id_t eid, const uint16_t options) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку опций для события сервера
		return this->_io->setOptions(this->_eid, options);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки опции события сервера
 *
 * @param eid    идентификатор события сервера
 * @param option опция события сервера для установки
 * @param mode   режим установки опции события сервера
 * @return       результат выполнения установки
 *
 */
bool awh::unit::QuicServer::setOption(const event::id_t eid, const uint16_t option, const bool mode) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку опции для события сервера
		return this->_io->setOption(this->_eid, option, mode);
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
awh::net::dgram_info_t awh::unit::QuicServer::getTrafficInfo(const event::id_t eid) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение метаданных последнего принятого дейтаграммного пакета для события сервера
		return this->_io->getTrafficInfo(this->_eid);
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
uint8_t awh::unit::QuicServer::getCountHops(const event::id_t eid) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение количества хопов последнего принятого пакета для события сервера
		return this->_io->getCountHops(this->_eid);
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
bool awh::unit::QuicServer::setCountHops(const event::id_t eid, const uint8_t hops) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку количества хопов последнего принятого пакета для события сервера
		return this->_io->setCountHops(this->_eid, hops);
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
awh::event::hops_t awh::unit::QuicServer::getHops(const event::id_t eid) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение максимального количества хопов для события сервера
		return this->_io->getHops(this->_eid);
	// Возвращаем значение по умолчанию
	return event::hops_t::LOOPBACK;
}
/**
 * @brief Метод установки максимального количества хопов, через которые может пройти пакет
 *
 * @param eid  идентификатор события сервера
 * @param hops максимальное количество хопов
 * @return     результат работы функции
 *
 */
bool awh::unit::QuicServer::setHops(const event::id_t eid, const event::hops_t hops) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку максимального количества хопов для события сервера
		return this->_io->setHops(this->_eid, hops);
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
string awh::unit::QuicServer::getIface(const event::id_t eid) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение сетевого интерфейса для события сервера
		return this->_io->getIface(this->_eid);
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
bool awh::unit::QuicServer::setIface(const event::id_t eid, string_view name) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку сетевого интерфейса для события сервера
		return this->_io->setIface(this->_eid, name);
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
uint16_t awh::unit::QuicServer::getPort(const event::id_t eid) const noexcept {
	// Если событие актуально (для события сервера - порт прослушивания, для сессии - порт пира)
	if(this->isActual(eid))
		// Выполняем получение порта для актуального события (унаследованный кластером идентификатор приводится к собственному)
		return this->_io->getSourcePort(this->actual(eid));
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
bool awh::unit::QuicServer::setPort(const event::id_t eid, const uint16_t port) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid)){
		// Кэшируем порт прослушивания для пересоздания сокета в дочернем процессе кластера
		if(eid == this->_eid)
			// Сохраняем порт прослушивания сервера
			this->_listenPort = port;
		// Выполняем установку порта сервера для события сервера
		return this->_io->setSourcePort(this->_eid, port);
	}
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
string awh::unit::QuicServer::getAddress(const event::id_t eid, const event::address_t address) const noexcept {
	// Если событие актуально (для события сервера - адрес прослушивания, для сессии - адрес пира)
	if(this->isActual(eid))
		// Выполняем получение адреса для актуального события (унаследованный кластером идентификатор приводится к собственному)
		return this->_io->getAddress(this->actual(eid), address);
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
bool awh::unit::QuicServer::setAddress(const event::id_t eid, const event::address_t address, string_view value) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid)){
		// Кэшируем адрес прослушивания для пересоздания сокета в дочернем процессе кластера
		if(eid == this->_eid){
			// Сохраняем тип адреса прослушивания сервера
			this->_listenType = address;
			// Сохраняем хост прослушивания сервера
			this->_listenHost = value;
		}
		// Выполняем установку адреса сервера для события сервера
		return this->_io->setAddress(this->_eid, address, value);
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки адреса сервера
 *
 * @param eid     идентификатор события сервера
 * @param address тип адреса сервера
 * @param value   структура сетевого адреса сервера
 * @return        результат выполнения установки
 *
 */
bool awh::unit::QuicServer::setAddress(const event::id_t eid, const event::address_t address, const net::addr_t * value) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку адреса сервера для события сервера
		return this->_io->setAddress(this->_eid, address, value);
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
bool awh::unit::QuicServer::getAddress(const event::id_t eid, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept {
	// Если событие актуально (для события сервера - адрес прослушивания, для сессии - адрес пира)
	if(this->isActual(eid))
		// Выполняем получение адреса для актуального события (унаследованный кластером идентификатор приводится к собственному)
		return this->_io->getAddress(this->actual(eid), address, value);
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
uint16_t awh::unit::QuicServer::getMaximumTransmissionUnit(const event::id_t eid) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение MTU сетевого интерфейса для события сервера
		return this->_io->getMaximumTransmissionUnit(this->_eid);
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
bool awh::unit::QuicServer::setMaximumTransmissionUnit(const event::id_t eid, const uint16_t mtu) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку MTU сетевого интерфейса для события сервера
		return this->_io->setMaximumTransmissionUnit(this->_eid, mtu);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения режима обнаружения максимального размера пакета (MTU)
 *
 * @param eid идентификатор события сервера
 * @return    текущий режим обнаружения MTU
 *
 */
awh::event::mtu_discover_t awh::unit::QuicServer::getMaximumTransmissionUnitDiscover(const event::id_t eid) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение режима обнаружения MTU для события сервера
		return this->_io->getMaximumTransmissionUnitDiscover(this->_eid, this->_io->family(this->_eid));
	// Возвращаем значение по умолчанию
	return event::mtu_discover_t::NONE;
}
/**
 * @brief Метод установки режима обнаружения максимального размера пакета (MTU)
 *
 * @param eid  идентификатор события сервера
 * @param mode режим обнаружения максимального размера пакета (MTU)
 * @return     результат работы функции
 *
 */
bool awh::unit::QuicServer::setMaximumTransmissionUnitDiscover(const event::id_t eid, const event::mtu_discover_t mode) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку режима обнаружения MTU для события сервера
		return this->_io->setMaximumTransmissionUnitDiscover(this->_eid, this->_io->family(this->_eid), mode);
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
awh::event::delivery_mode_t awh::unit::QuicServer::getDelivery(const event::id_t eid) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение режима трансляции пакетов сервера для события сервера
		return this->_io->getDelivery(this->_eid);
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
bool awh::unit::QuicServer::setDelivery(const event::id_t eid, const event::delivery_mode_t delivery) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку режима трансляции пакетов сервера для события сервера
		return this->_io->setDelivery(this->_eid, delivery);
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
size_t awh::unit::QuicServer::getBufferSize(const event::id_t eid, const event::action_t action) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение размера буфера сервера для события сервера
		return this->_io->getBufferSize(this->_eid, action);
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
bool awh::unit::QuicServer::setBufferSize(const event::id_t eid, const event::action_t action, const size_t size) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку размера буфера сервера для события сервера
		return this->_io->setBufferSize(this->_eid, action, size);
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
awh::event::usage_t awh::unit::QuicServer::getUsageReadTimeout(const event::id_t eid) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение режима использования таймаута на чтение события для события сервера
		return this->_io->getUsageReadTimeout(this->_eid);
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
void awh::unit::QuicServer::setUsageReadTimeout(const event::id_t eid, const event::usage_t usage) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку режима использования таймаута на чтение события для события сервера
		this->_io->setUsageReadTimeout(this->_eid, usage);
}
/**
 * @brief Метод получения таймаута сервера
 *
 * @param eid    идентификатор события сервера
 * @param action тип действия сервера
 * @return       значение таймаута в миллисекундах
 *
 */
uint32_t awh::unit::QuicServer::getTimeout(const event::id_t eid, const event::action_t action) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение параметров таймаута для сервера
		return this->_io->getTimeout(this->_eid, action);
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
void awh::unit::QuicServer::setTimeout(const event::id_t eid, const event::action_t action, const uint32_t timeout) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку параметров таймаута для сервера
		this->_io->setTimeout(this->_eid, action, timeout);
}
/**
 * @brief Метод установки пропускной способности сервера
 *
 * @param eid       идентификатор события сервера
 * @param limiting  режим ограничения пропускной способности сервера (egress или ingress)
 * @param bandwidth пропускная способность сервера для установки
 * @return          результат выполнения установки
 *
 */
bool awh::unit::QuicServer::bandwidth(const event::id_t eid, const event::limiting_t limiting, string_view bandwidth) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку параметров пропускной способности для сервера
		return this->_io->bandwidth(this->_eid, limiting, bandwidth);
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
bool awh::unit::QuicServer::keepAlive(const event::id_t eid, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку параметров keep-alive для сервера
		return this->_io->keepAlive(this->_eid, cnt, idle, intvl);
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
awh::event::dscp_t awh::unit::QuicServer::getDifferentiatedServicesCodePoint(const event::id_t eid) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение значения поля Differentiated Services Code Point (DSCP) для сервера
		return this->_io->getDifferentiatedServicesCodePoint(this->_eid, this->_io->family(this->_eid));
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
bool awh::unit::QuicServer::setDifferentiatedServicesCodePoint(const event::id_t eid, const event::dscp_t dscp) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку значения поля Differentiated Services Code Point (DSCP) для сервера
		return this->_io->setDifferentiatedServicesCodePoint(this->_eid, this->_io->family(this->_eid), dscp);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод активации/деактивации мультикаст-группы
 *
 * @param eid    идентификатор события сервера
 * @param mode   режим активации/деактивации
 * @param group  мультикаст-группа для активации/деактивации
 * @param source адрес сетевого интерфейса с которого выполняется подписка
 * @param port   порт мультикаст-группы с которого выполняется подписка
 * @return       результат выполнения установки
 *
 */
bool awh::unit::QuicServer::membership(const event::id_t eid, const event::mode_t mode, string_view group, string_view source, const uint16_t port) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем активацию/деактивацию мультикаст-группы для сервера
		return this->_io->membership(this->_eid, mode, group, source, port);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод активации/деактивации мультикаст-группы
 *
 * @param eid    идентификатор события сервера
 * @param mode   режим активации/деактивации
 * @param group  мультикаст-группа для активации/деактивации
 * @param source адрес сетевого интерфейса с которого выполняется подписка
 * @param port   порт мультикаст-группы с которого выполняется подписка
 * @return       результат выполнения установки
 *
 */
bool awh::unit::QuicServer::membership(const event::id_t eid, const event::mode_t mode, const net::addr_t * group, const net::addr_t * source, const uint16_t port) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем активацию/деактивацию мультикаст-группы для сервера
		return this->_io->membership(this->_eid, mode, group, source, port);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод остановки сервера
 *
 */
void awh::unit::QuicServer::stop() noexcept {
	/**
	 * Для операционных систем, отличных от MS Windows
	 */
	#if !_WIN32 && !_WIN64
		/**
		 * В режиме кластера работу останавливает родительский процесс: он не запускает
		 * собственный цикл событий (working() остаётся ложным), а лишь супервизирует
		 * дочерние процессы, поэтому остановка кластера не гейтится на working()
		 */
		if(this->_clusterParams.mode == event::mode_t::ENABLED){
			// Если кластер инициализирован
			if(this->_cluster != nullptr)
				// Останавливаем работу кластера (каждый дочерний процесс остановит свой сокет)
				this->_cluster->stop();
			// Выходим из метода
			return;
		}
	#endif
	// Если работа юнита запущена
	if(this->working())
		// Выполняем остановку работы основного юнита
		unit_t::stop();
}
/**
 * @brief Метод запуска сервера
 *
 */
void awh::unit::QuicServer::start() noexcept {
	// Если работа юнита ещё не запущена
	if(!this->working()){
		/**
		 * Для операционных систем, отличных от MS Windows
		 */
		#if !_WIN32 && !_WIN64
			/**
			 * В режиме кластера родительский процесс выступает супервизором: он не
			 * поднимает собственный сокет, а раздаёт порты дочерним процессам (см.
			 * cluster()), каждый из которых поднимает собственный сокет сервера после
			 * fork. Схема единая для всех систем, различается лишь политика выделения
			 * портов дочерним процессам
			 */
			if(this->_clusterParams.mode == event::mode_t::ENABLED){
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
				this->_cluster->on <void (const pid_t, const int32_t)> ("exit", &quic_server_t::exit, this, _1, _2);
				// Устанавливаем функцию обратного вызова на событие пересоздания процесса
				this->_cluster->on <void (const pid_t, const pid_t)> ("rebase", &quic_server_t::rebase, this, _1, _2);
				// Устанавливаем функцию обратного вызова на событие отправки сообщений
				this->_cluster->on <void (const pid_t, const size_t)> ("sending", &quic_server_t::sending, this, _1, _2);
				// Устанавливаем функцию обратного вызова на получение событий кластера
				this->_cluster->on <void (const pid_t, const unit::cluster_t::event_t)> ("events", &quic_server_t::cluster, this, _1, _2);
				// Устанавливаем функцию обратного вызова на событие получения сообщений
				this->_cluster->on <void (const pid_t, const uint8_t *, const size_t)> ("message", &quic_server_t::message, this, _1, _2, _3);
				// Устанавливаем функцию обратного вызова на событие изменения статуса кластера
				this->_cluster->on <void (const pid_t, const event::status_t)> ("state", static_cast <void (quic_server_t::*)(const pid_t, const event::status_t)> (&quic_server_t::status), this, _1, _2);
				// Устанавливаем функцию обратного вызова на событие получения ошибок кластера
				this->_cluster->on <void (const pid_t, const event::error_t, const string &)> ("error", static_cast <void (quic_server_t::*)(const pid_t, const event::error_t, const string &)> (&quic_server_t::error), this, _1, _2, _3);
				// Устанавливаем функцию обратного вызова на событие доступности/недоступности очереди исходящих данных кластера
				this->_cluster->on <void (const pid_t, const event::status_t, const size_t)> ("available", static_cast <void (quic_server_t::*)(const pid_t, const event::status_t, const size_t)> (&quic_server_t::available), this, _1, _2, _3);
				// Запускаем работу кластера (сокеты серверов поднимаются в дочерних процессах после fork)
				this->_cluster->start();
				// Выходим из метода
				return;
			}
		#endif
		/**
		 * Одиночный процесс (MS Windows либо режим без кластера): сервер поднимается
		 * штатным образом на собственном сокете
		 */
		// Выполняем получение идентификатора функции обратного вызова
		const callback_t::id_t fid = this->_callback.id("status");
		// Если функция обратного вызова установлена
		if(this->_callback.is(fid))
			// Переименовываем callback status в server_status
			this->_callback.set(fid, this->_callback.id("server_status"), this->_callback);
		// Устанавливаем функцию обратного вызова на запуск системы
		this->_callback.on <void (const event::status_t)> (fid, static_cast <void (quic_server_t::*)(const event::status_t)> (&quic_server_t::launch), this, _1);
		// Выполняем запуск работы основного юнита
		unit_t::start();
	}
}
/**
 * @brief Метод открытия потока приложения соединения
 *
 * @param oid  идентификатор события сессии
 * @param mode режим однонаправленного потока
 * @return     идентификатор открытого потока
 *
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
 *
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
 *
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
 *
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
 *
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
 *
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
 *
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
 *
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
 * @brief Метод обработки события пересоздания процесса кластера
 *
 * @param old старый идентификатор процесса
 * @param pid текущий идентификатор процесса
 *
 */
void awh::unit::QuicServer::rebase(const pid_t old, const pid_t pid) noexcept {
	/**
	 * Возрождённому после падения дочернему процессу автоматически возвращаем тот же
	 * порт прослушивания, что был у него до гибели: без этого возрождённый процесс
	 * остался бы работать в холостую, так как повторная раздача портов не выполняется
	 */
	// Выполняем поиск порта, выделенного погибшему дочернему процессу
	auto i = this->_ports.find(old);
	// Если погибшему дочернему процессу был выделен порт прослушивания
	if(i != this->_ports.end()){
		// Запоминаем порт прослушивания погибшего дочернего процесса
		const uint16_t port = i->second;
		// Удаляем привязку порта к идентификатору погибшего дочернего процесса
		this->_ports.erase(i);
		// Отправляем возрождённому дочернему процессу его прежний порт прослушивания
		this->sendPort(pid, port);
	// Если погибший дочерний процесс работал в холостую - возрождённый тоже остаётся в холостую
	} else if(this->_idle.erase(old) > 0)
		// Переносим признак работы в холостую на идентификатор возрождённого процесса
		this->_idle.insert(pid);
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const pid_t, const pid_t)> ("cluster_rebase", old, pid);
}
/**
 * @brief Метод получения события завершения работы процесса кластера
 *
 * @param pid    идентификатор процесса
 * @param signal сигнал с которым завершился процесс
 *
 */
void awh::unit::QuicServer::exit(const pid_t pid, const int32_t signal) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const pid_t, const int32_t)> ("cluster_exit", pid, signal);
}
/**
 * @brief Метод обработки события отправки сообщения процессу кластера
 *
 * @param pid  идентификатор процесса
 * @param size размер отправленного сообщения
 *
 */
void awh::unit::QuicServer::sending(const pid_t pid, const size_t size) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const pid_t, const size_t)> ("cluster_sending", pid, size);
}
/**
 * @brief Метод получения событий активации/деактивации кластера
 *
 * @param pid   идентификатор процесса
 * @param event флаг события кластера
 *
 */
void awh::unit::QuicServer::cluster(const pid_t pid, const unit::cluster_t::event_t event) noexcept {
	/**
	 * Определяем полученное событие кластера
	 */
	switch(static_cast <uint8_t> (event)){
		// Если событие представляет из себя запуск процесса
		case static_cast <uint8_t> (unit::cluster_t::event_t::START): {
			/**
			 * Родительский процесс раздаёт порты прослушивания дочерним процессам, а
			 * каждый дочерний процесс поднимает собственный сокет на выделенном ему
			 * порту после fork (см. message()). Единая для всех систем схема, различие
			 * только в политике выделения портов
			 */
			// Если процесс является родительским процессом кластера
			if(this->clusterFamily() == cluster_t::family_t::MASTER){
				/**
				 * Определяем диапазон портов: явно заданный диапазон либо единственный
				 * порт прослушивания, если диапазон не задавался
				 */
				const uint16_t begin = ((this->_portBegin > 0) ? this->_portBegin : this->_listenPort);
				uint16_t end = ((this->_portEnd > 0) ? this->_portEnd : begin);
				// Если конечный порт меньше начального - выравниваем к единственному порту
				if(end < begin)
					// Устанавливаем конечный порт равным начальному
					end = begin;
				// Если порт прослушивания определён
				if(begin > 0){
					// Формируем пул доступных портов диапазона
					vector <uint16_t> ports;
					// Резервируем память под пул портов
					ports.reserve((end - begin) + 1);
					// Заполняем пул доступных портов диапазона
					for(uint32_t port = begin; port <= end; ++port)
						// Добавляем порт в пул доступных портов
						ports.push_back(static_cast <uint16_t> (port));
					// Получаем список идентификаторов дочерних процессов кластера
					const auto workers = this->clusterWorkers();
					// Очищаем список дочерних процессов, работающих в холостую
					this->_idle.clear();
					// Очищаем карту выделенных дочерним процессам портов
					this->_ports.clear();
					// Индекс выделяемого из пула порта
					size_t index = 0;
					/**
					 * Раздаём порты прослушивания дочерним процессам: уникальные порты
					 * диапазона выделяются по порядку, пока не закончатся
					 */
					for(const auto & worker : workers){
						// Если в диапазоне остаётся неиспользованный уникальный порт
						if(index < ports.size())
							// Отправляем дочернему процессу очередной уникальный порт диапазона
							this->sendPort(worker, ports[index]);
						// Если уникальные порты диапазона исчерпаны
						else {
							/**
							 * Для операционной системы Linux или FreeBSD
							 *
							 * @note Оставшимся дочерним процессам достаётся последний порт
							 *       диапазона: несколько процессов делят его через SO_REUSEPORT,
							 *       а ядро распределяет между ними входящие датаграммы. Частный
							 *       случай единственного порта диапазона - он достаётся всем
							 *
							 */
							#if __linux__ || __FreeBSD__
								// Отправляем дочернему процессу последний порт диапазона
								this->sendPort(worker, ports.back());
							/**
							 * Для операционной системы OpenBSD, NetBSD, Sun Solaris или macOS
							 *
							 * @note Порт выделяется дочернему процессу монопольно (SO_REUSEPORT
							 *       в этих системах не даёт нескольким процессам полноценно
							 *       работать на общем порту), поэтому оставшийся без порта
							 *       дочерний процесс работает в холостую: его идентификатор
							 *       сохраняется для ручной доотправки порта методом clusterAssign()
							 *
							 */
							#else
								// Сохраняем дочерний процесс в списке работающих в холостую
								this->_idle.insert(worker);
							#endif
						}
						// Переходим к следующему порту диапазона
						++index;
					}
				}
			// Если процесс является дочерним процессом кластера
			} else {
				// Если работа юнита ещё не запущена
				if(!this->working()){
					/**
					 * Дочерний процесс поднимает собственный сокет только после получения
					 * выделенного родительским процессом порта (см. message()), поэтому
					 * унаследованное несвязанное событие сервера уничтожается, а оповещение
					 * приложения о запуске откладывается флагом ожидания порта
					 */
					// Если унаследованное событие сервера создано
					if(this->_eid != 0){
						// Снимаем функцию обратного вызова определения сессии
						this->_io->on(this->_eid, static_cast <engine::callback::origin_t> (nullptr));
						// Снимаем функцию обратного вызова на создание сессии соединения
						this->_io->on(this->_eid, static_cast <engine::callback::accept_t> (nullptr));
						// Уничтожаем унаследованное событие сервера
						this->_io->destroy(this->_eid);
						// Запоминаем унаследованный идентификатор для приведения обращений фасада к собственному событию (см. actual())
						this->_inheritedEid = this->_eid;
						// Сбрасываем идентификатор события сервера
						this->_eid = 0;
					}
					// Если унаследованное событие интервала таймеров соединений создано
					if(this->_tid != 0){
						// Уничтожаем унаследованное событие интервала таймеров соединений
						this->_io->destroy(this->_tid);
						// Сбрасываем идентификатор события интервала таймеров соединений
						this->_tid = 0;
					}
					// Взводим флаг ожидания выделенного родительским процессом порта прослушивания
					this->_awaitingPort = true;
					// Выполняем получение идентификатора функции обратного вызова
					const callback_t::id_t fid = this->_callback.id("status");
					// Если функция обратного вызова установлена
					if(this->_callback.is(fid))
						// Переименовываем callback status в server_status
						this->_callback.set(fid, this->_callback.id("server_status"), this->_callback);
					// Устанавливаем функцию обратного вызова на запуск системы
					this->_callback.on <void (const event::status_t)> (fid, static_cast <void (quic_server_t::*)(const event::status_t)> (&quic_server_t::launch), this, _1);
					// Выполняем запуск работы основного юнита (сокет будет привязан после получения порта)
					unit_t::start();
				}
			}
		} break;
		// Если событие представляет из себя остановку процесса
		case static_cast <uint8_t> (unit::cluster_t::event_t::STOP): {
			// Если процесс является дочерним процессом кластера и его работа запущена
			if((this->clusterFamily() == cluster_t::family_t::CHILDREN) && this->working())
				// Останавливаем работу основного юнита
				unit_t::stop();
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
void awh::unit::QuicServer::message(const pid_t pid, const uint8_t * data, const size_t size) noexcept {
	/**
	 * Если дочерний процесс ожидает выделенный родительским процессом порт прослушивания
	 * и получено корректное сообщение
	 */
	if(this->_awaitingPort && (data != nullptr) && (size >= sizeof(quic_cluster_message_t))){
		// Тип полученного сообщения
		quic_cluster_message_t message = quic_cluster_message_t::NONE;
		// Извлекаем тип сообщения из полученных данных
		::memcpy(&message, data, sizeof(message));
		// Если сообщение несёт выделенный порт прослушивания и его размер корректен
		if((message == quic_cluster_message_t::PORT) && ((size - sizeof(message)) == sizeof(uint16_t))){
			// Выделенный родительским процессом порт прослушивания
			uint16_t port = 0;
			// Извлекаем выделенный порт прослушивания из полученных данных
			::memcpy(&port, data + sizeof(message), sizeof(port));
			// Создаём собственный сокет сервера и таймер соединений в дочернем процессе
			if(this->issue(this->_family) != 0){
				// Восстанавливаем адрес прослушивания на собственном сокете
				if(!this->_listenHost.empty())
					// Устанавливаем сохранённый адрес прослушивания сервера
					this->_io->setAddress(this->_eid, this->_listenType, this->_listenHost);
				// Устанавливаем выделенный родительским процессом порт прослушивания
				this->_io->setSourcePort(this->_eid, port);
				// Привязываем собственный сокет и таймер дочернего процесса (SO_REUSEPORT установлен в issue())
				if(this->_io->commit(this->_eid) && this->_io->commit(this->_tid)){
					// Сбрасываем флаг ожидания выделенного порта прослушивания
					this->_awaitingPort = false;
					/**
					 * Оповещаем приложение о запуске сервера дочернего процесса: это
					 * переводит фасад к launch()/listen() на уже привязанном собственном
					 * сокете дочернего процесса
					 */
					this->_callback.call <void (const event::status_t)> ("server_status", event::status_t::LAUNCHED);
				}
			}
		}
	}
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const pid_t, const uint8_t *, const size_t)> ("cluster_message", pid, data, size);
}
/**
 * @brief Метод обработки событий изменения статуса процесса кластера
 *
 * @param pid    идентификатор процесса
 * @param status новый статус процесса кластера
 *
 */
void awh::unit::QuicServer::status(const pid_t pid, const event::status_t status) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const pid_t, const event::status_t)> ("cluster_state", pid, status);
}
/**
 * @brief Метод обработки событий ошибок кластера
 *
 * @param pid         идентификатор процесса
 * @param error       тип ошибки
 * @param description описание ошибки
 *
 */
void awh::unit::QuicServer::error(const pid_t pid, const event::error_t error, const string & description) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const pid_t, const event::error_t, const string &)> ("cluster_error", pid, error, description);
}
/**
 * @brief Метод обработки события доступности/недоступности очереди исходящих сообщений кластера
 *
 * @param pid    идентификатор процесса
 * @param status статус доступности очереди
 * @param size   размер доступных данных очереди
 *
 */
void awh::unit::QuicServer::available(const pid_t pid, const event::status_t status, const size_t size) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const pid_t, const event::status_t, const size_t)> ("cluster_available", pid, status, size);
}
/**
 * @brief Метод установки названия кластера
 *
 * @param name название кластера для установки
 *
 */
void awh::unit::QuicServer::clusterName(string_view name) noexcept {
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
awh::unit::cluster_t::family_t awh::unit::QuicServer::clusterFamily() const noexcept {
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
awh::event::mode_t awh::unit::QuicServer::clusterMode() const noexcept {
	// Извлекаем режим активации кластера
	return this->_clusterParams.mode;
}
/**
 * @brief Метод установки режима работы кластера
 *
 * @param mode режим активации/деактивации кластера
 *
 */
void awh::unit::QuicServer::clusterMode(const event::mode_t mode) noexcept {
	// Устанавливаем режим активации кластера
	this->_clusterParams.mode = mode;
}
/**
 * @brief Метод получения максимального количества процессов
 *
 * @return максимальное количество процессов
 *
 */
uint16_t awh::unit::QuicServer::clusterCount() const noexcept {
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
void awh::unit::QuicServer::clusterCount(const uint16_t count) noexcept {
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
unordered_set <pid_t> awh::unit::QuicServer::clusterWorkers() const noexcept {
	// Если кластер инициализирован
	if(this->_cluster != nullptr)
		// Возвращаем количество активных воркеров кластера
		return this->_cluster->workers();
	// Возвращаем значение по умолчанию
	return {};
}
/**
 * @brief Метод установки диапазона портов для выделения дочерним процессам кластера
 *
 * @param begin начальный порт диапазона (0 - использовать порт прослушивания)
 * @param end   конечный порт диапазона (0 - использовать порт прослушивания)
 *
 */
void awh::unit::QuicServer::clusterRange(const uint16_t begin, const uint16_t end) noexcept {
	// Устанавливаем начальный порт диапазона выделения портов дочерним процессам
	this->_portBegin = begin;
	// Устанавливаем конечный порт диапазона выделения портов дочерним процессам
	this->_portEnd = end;
}
/**
 * @brief Метод получения списка дочерних процессов, не получивших порт прослушивания
 *
 * @return список идентификаторов дочерних процессов, работающих в холостую
 *
 */
unordered_set <pid_t> awh::unit::QuicServer::clusterIdle() const noexcept {
	// Возвращаем список дочерних процессов, не получивших порт прослушивания
	return this->_idle;
}
/**
 * @brief Метод отправки выделенного порта прослушивания дочернему процессу кластера
 *
 * @param pid  идентификатор дочернего процесса
 * @param port выделяемый порт прослушивания
 * @return     результат отправки
 *
 */
bool awh::unit::QuicServer::sendPort(const pid_t pid, const uint16_t port) noexcept {
	// Буфер IPC-сообщения: тег типа сообщения и выделенный порт
	uint8_t buffer[sizeof(quic_cluster_message_t) + sizeof(uint16_t)] = {0};
	// Устанавливаем тип сообщения - выделенный порт прослушивания
	const quic_cluster_message_t message = quic_cluster_message_t::PORT;
	// Записываем тип сообщения в буфер
	::memcpy(buffer, &message, sizeof(message));
	// Записываем выделенный порт в буфер сообщения
	::memcpy(&buffer[sizeof(message)], &port, sizeof(port));
	// Если отправка выделенного порта дочернему процессу выполнена
	if(this->clusterSend(pid, buffer, sizeof(buffer)) > 0){
		// Запоминаем выделенный дочернему процессу порт для восстановления после возрождения
		this->_ports[pid] = port;
		// Возвращаем положительный результат
		return true;
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод отправки порта прослушивания конкретному дочернему процессу кластера
 *
 * @param pid  идентификатор дочернего процесса
 * @param port порт прослушивания для дочернего процесса
 * @return     результат отправки порта дочернему процессу
 *
 */
bool awh::unit::QuicServer::clusterAssign(const pid_t pid, const uint16_t port) noexcept {
	// Отправляем порт прослушивания дочернему процессу
	if(this->sendPort(pid, port)){
		// Исключаем дочерний процесс из списка работающих в холостую
		this->_idle.erase(pid);
		// Возвращаем положительный результат
		return true;
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод отправки сообщения родительскому процессу
 *
 * @param buffer бинарный буфер для отправки сообщения
 * @param size   размер бинарного буфера для отправки сообщения
 * @return       количество байт отправленного сообщения
 *
 */
size_t awh::unit::QuicServer::clusterSend(const void * buffer, const size_t size) noexcept {
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
size_t awh::unit::QuicServer::clusterSend(const pid_t pid, const void * buffer, const size_t size) noexcept {
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
size_t awh::unit::QuicServer::clusterBroadcast(const void * buffer, const size_t size) noexcept {
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
void awh::unit::QuicServer::clusterRebirth(const bool mode) noexcept {
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
void awh::unit::QuicServer::clusterRebirthLimit(const uint16_t limit, const uint64_t window) noexcept {
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
awh::event::type_t awh::unit::QuicServer::clusterGetTypeEventMessage() const noexcept {
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
void awh::unit::QuicServer::clusterSetTypeEventMessage(const event::type_t type) noexcept {
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
size_t awh::unit::QuicServer::clusterGetBufferSize(const pid_t pid, const event::action_t action) const noexcept {
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
bool awh::unit::QuicServer::clusterSetBufferSize(const pid_t pid, const event::action_t action, const size_t size) noexcept {
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
awh::unit::QuicServer::QuicServer(const fmk_t * fmk, const log_t * log) noexcept :
 unit_t(fmk, log), _eid(0), _tid(0), _inheritedEid(0), _retry(false), _ecn(false), _family(event::family_t::IPV4),
 _marking(event::ecn_t::NOT_ECT), _resetKey{""}, _ctx(0), _coder(nullptr), _cluster(nullptr),
 _listenType(event::address_t::IPV4), _awaitingPort(false), _backlog(0), _listenPort(0),
 _portBegin(0), _portEnd(0), _listenHost{""} {}
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
 *
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
 *
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
	 * Билет возобновления сессии соединение сохраняет самостоятельно в кэш кодера
	 * по ключу сервера (RFC 9001 §4.6), поэтому шаттлинг билета юнитом не требуется
	 */
	/**
	 * Сохраняем присланный сервером токен проверки адреса: он приходит фреймом
	 * NEW_TOKEN уже после установления соединения, поэтому проверяется на каждой
	 * датаграмме, пока не будет получен
	 */
	if((this->_connection != nullptr) && this->_token.empty())
		// Сохраняем токен проверки адреса для следующего подключения
		this->_token = this->_connection->token();
	// Выполняем оповещение приложения о завершённом соединении
	this->complete();
}
/**
 * @brief Метод оповещения приложения о завершённом соединении
 *
 */
void awh::unit::QuicClient::complete() noexcept {
	// Если соединение не создано либо ещё не завершено
	if((this->_connection == nullptr) || (this->_connection->state() != quic::connection_t::state_t::DRAINING))
		// Выходим из метода
		return;
	/**
	 * Оповещение выполняется однократно и независимо от того, было ли соединение
	 * установлено: сорванное рукопожатие приложению требуется отличать от
	 * успешно завершённого обмена, а без оповещения попытка подключения
	 * заканчивается молча и причина разрыва снаружи неразличима
	 */
	if(!this->_notified){
		// Устанавливаем флаг выполненного оповещения о завершённом соединении
		this->_notified = true;
		// Сбрасываем флаг оповещения приложения об установленном соединении
		this->_connected = false;
		// Выполняем функцию обратного вызова о завершённом соединении
		this->_callback.call <void (const event::id_t, const quic::error_t)> ("close", this->_eid, this->_connection->error());
	}
}
/**
 * @brief Метод обработки просроченных таймеров соединения
 *
 * @param eid    идентификатор события интервала
 * @param status статус события интервала
 *
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
		/**
		 * Выполняем оповещение приложения о завершённом соединении: завершение
		 * наступает и по таймеру - истечением периода завершения либо таймаута
		 * простоя, когда приходить от удалённого эндпоинта уже нечему
		 */
		this->complete();
	}
}
/**
 * @brief Метод обработки завершения подключения к серверу
 *
 * @param eid идентификатор события клиента
 * @param ok  результат подключения к серверу
 *
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
	/**
	 * Оповещаем приложение о готовности к отправке ранних данных: ключи их защиты
	 * выданы возобновлённой сессией, а лимиты взяты из прошлого соединения, так
	 * что поставленное сейчас содержимое уйдёт вместе с первым пакетом хендшейка,
	 * не дожидаясь его завершения (RFC 9001 §4.6)
	 */
	if(this->_connection->writable())
		// Выполняем функцию обратного вызова о готовности к отправке ранних данных
		this->_callback.call <void (const event::id_t)> ("early", this->_eid);
	// Отправляем первый пакет хендшейка серверу
	this->flush();
}
/**
 * @brief Метод выдачи собранных данных потоков приложения
 *
 */
void awh::unit::QuicClient::process() noexcept {
	/**
	 * Соединение может быть уничтожено приложением реентрантно - из функции
	 * обратного вызова, вызываемой этим же методом (напр. "read" открывает путь
	 * к destroy()). После этого объект соединения освобождён, и продолжать выдачу
	 * нельзя (RFC здесь ни при чём - это защита от повторного входа)
	 */
	if(this->_connection == nullptr)
		// Выходим из метода
		return;
	/**
	 * Если для соединения установлено объединение данных (splice), собранные данные
	 * потоков и датаграмм перенаправляются в событие-приёмник вместо выдачи
	 * приложению - строится прозрачный канал между соединением и другим событием
	 */
	const bool spliced = (this->_dest != 0);
	// Если объединение установлено либо функция обратного вызова на собранные данные потока установлена
	if(spliced || this->_callback.is("read")){
		// Список потоков с собранными данными
		vector <uint64_t> streams;
		// Получаем список потоков с собранными данными
		this->_connection->readable(streams);
		/**
		 * Перебираем потоки с собранными данными
		 */
		for(auto & sid : streams){
			// Если соединение уничтожено приложением реентрантно из функции обратного вызова - прекращаем выдачу
			if(this->_connection == nullptr)
				// Выходим из метода
				return;
			// Флаг завершения потока удалённым эндпоинтом
			bool fin = false;
			// Собранные данные потока приложения
			string data = "";
			// Если выдача собранных данных потока не выполнена
			if(this->_connection->receive(sid, data, fin) != quic::status_t::OK)
				// Пропускаем поток с ошибкой выдачи
				continue;
			// Если данные получены либо поток завершён
			if(!data.empty() || fin){
				// Если для соединения установлено объединение данных - перенаправляем их в событие-приёмник
				if(spliced)
					// Перенаправляем собранные данные потока в событие-приёмник объединения
					this->forward(data);
				// Иначе выдаём собранные данные потока приложению
				else this->_callback.call <void (const event::id_t, const uint64_t, const string &, const bool)> ("read", this->_eid, sid, data, fin);
			}
		}
	}
	// Если соединение уничтожено приложением реентрантно из функции обратного вызова на собранные данные потока
	if(this->_connection == nullptr)
		// Выходим из метода
		return;
	// Если объединение установлено либо функция обратного вызова на принятую датаграмму приложения установлена
	if(spliced || this->_callback.is("datagram")){
		// Буфер принятой датаграммы приложения
		string datagram = "";
		/**
		 * Выдаём принятые датаграммы приложения: они доставляются вне потоков
		 * и порядка доставки не имеют (RFC 9221)
		 */
		while(this->_connection->datagram(datagram)){
			// Если для соединения установлено объединение данных - перенаправляем датаграмму в событие-приёмник
			if(spliced)
				// Перенаправляем принятую датаграмму в событие-приёмник объединения
				this->forward(datagram);
			// Иначе выдаём принятую датаграмму приложению
			else this->_callback.call <void (const event::id_t, const string &)> ("datagram", this->_eid, datagram);
			// Если соединение уничтожено приложением реентрантно из функции обратного вызова - прекращаем выдачу
			if(this->_connection == nullptr)
				// Выходим из метода
				return;
		}
	}
}
/**
 * @brief Метод отправки готовых исходящих датаграмм соединения
 *
 */
void awh::unit::QuicClient::flush() noexcept {
	// Если соединение уничтожено приложением реентрантно из функции обратного вызова - отправлять нечего
	if(this->_connection == nullptr)
		// Выходим из метода
		return;
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
 * @brief Метод отправки объединённых данных в туннельный поток соединения
 *
 * @param data данные для отправки в туннельный поток
 * @param size размер данных для отправки
 * @return     результат постановки данных в очередь отправки
 *
 */
bool awh::unit::QuicClient::inject(const uint8_t * data, const size_t size) noexcept {
	// Если данные для отправки отсутствуют либо соединение не создано
	if((data == nullptr) || (size == 0) || (this->_connection == nullptr))
		// Возвращаем значение по умолчанию
		return false;
	// Если туннельный поток входящих объединённых данных ещё не открыт
	if(this->_tunnel == quic::connection_t::INVALID_STREAM)
		// Открываем двунаправленный туннельный поток приложения (RFC 9000 §2.1)
		this->_tunnel = this->_connection->open(false);
	// Если туннельный поток открыть не удалось
	if(this->_tunnel == quic::connection_t::INVALID_STREAM)
		// Возвращаем значение по умолчанию
		return false;
	// Если постановка байт в очередь отправки туннельного потока не выполнена
	if(this->_connection->send(this->_tunnel, string_view(reinterpret_cast <const char *> (data), size), false) != quic::status_t::OK)
		// Возвращаем значение по умолчанию
		return false;
	// Отправляем готовые исходящие датаграммы удалённому серверу
	this->flush();
	// Возвращаем положительный результат
	return true;
}
/**
 * @brief Метод перенаправления собранных данных соединения в событие-приёмник объединения
 *
 * @param data данные для перенаправления
 *
 */
void awh::unit::QuicClient::forward(string_view data) noexcept {
	// Если данные для перенаправления присутствуют и приёмник задан
	if(!data.empty() && (this->_dest != 0))
		/**
		 * Перенаправляем собранные данные соединения в событие-приёмник через сетевой
		 * движок: если приёмник шифрует данные на уровне соединения (QUIC-сессия) - они
		 * отправляются его туннельным потоком через функцию инъекции, иначе (TCP/UDP) -
		 * пишутся сырыми байтами в сокет
		 */
		this->_io->relay(this->_dest, data.data(), data.size());
}
/**
 * @brief Метод применения маркировки соединения к сокету события клиента
 *
 * @param marking требуемая маркировка исходящих датаграмм
 *
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
 *
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
 *
 */
void awh::unit::QuicClient::ecn(const bool mode) noexcept {
	// Устанавливаем режим уведомления о перегрузке пути
	this->_ecn = mode;
}
/**
 * @brief Метод установки локальных транспортных параметров соединения (RFC 9000 §7.4)
 *
 * @param params локальные транспортные параметры
 *
 */
void awh::unit::QuicClient::params(const quic::params::params_t & params) noexcept {
	// Устанавливаем локальные транспортные параметры соединения
	this->_params = params;
}
/**
 * @brief Метод извлечения сохранённого токена проверки адреса (RFC 9000 §8.1.3)
 *
 * @return токен проверки адреса (пусто - токен не получен)
 *
 */
const string & awh::unit::QuicClient::token() const noexcept {
	// Выводим сохранённый токен проверки адреса
	return this->_token;
}
/**
 * @brief Метод установки сохранённого токена проверки адреса (RFC 9000 §8.1.3)
 *
 * @param token токен проверки адреса
 *
 */
void awh::unit::QuicClient::token(string_view token) noexcept {
	// Устанавливаем сохранённый токен проверки адреса
	this->_token.assign(token.begin(), token.end());
}
/**
 * @brief Метод проверки актуальности события клиента
 *
 * @param eid идентификатор события
 * @return    результат проверки актуальности события
 *
 */
bool awh::unit::QuicClient::isActual(const event::id_t eid) const noexcept {
	// Событие клиента актуально, если оно создано и совпадает с идентификатором события клиента
	return ((this->_eid != 0) && (eid == this->_eid));
}
/**
 * @brief Метод запуска/остановки работы клиента
 *
 * @param status статус запуска/остановки клиента
 *
 */
void awh::unit::QuicClient::launch(const event::status_t status) noexcept {
	/**
	 * Определяем статус работы клиента
	 */
	switch(static_cast <uint8_t> (status)){
		// Если работа клиента запущена
		case static_cast <uint8_t> (event::status_t::LAUNCHED):
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const event::status_t)> ("client_status", status);
		break;
		// Если работа клиента подлежит уничтожению
		case static_cast <uint8_t> (event::status_t::DESTROYED): {
			// Освобождаем объект соединения QUIC
			this->_connection.reset(nullptr);
			// Если событие интервала таймеров соединения создано
			if(this->_tid != 0){
				// Уничтожаем событие интервала таймеров соединения
				this->_io->destroy(this->_tid);
				// Сбрасываем идентификатор события интервала таймеров соединения
				this->_tid = 0;
			}
			// Если событие клиента создано
			if(this->_eid != 0){
				// Снимаем функцию обратного вызова на чтение датаграмм соединения
				this->_io->on(this->_eid, static_cast <engine::callback::read_t> (nullptr));
				// Снимаем функцию обратного вызова на завершение подключения к серверу
				this->_io->on(this->_eid, static_cast <engine::callback::connect_t> (nullptr));
				// Уничтожаем событие клиента
				this->_io->destroy(this->_eid);
				// Сбрасываем идентификатор события клиента
				this->_eid = 0;
			}
			// Выполняем получение идентификатора функции обратного вызова
			const callback_t::id_t fid = this->_callback.id("client_status");
			// Если функция обратного вызова установлена
			if(this->_callback.is(fid)){
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const event::status_t)> (fid, status);
				// Выполняем получение функции обратного вызова
				this->_callback.set(fid, this->_callback.id("status"), this->_callback);
			}
		} break;
	}
}
/**
 * @brief Метод создания события клиента QUIC поверх UDP
 *
 * @param family   семейство адресов события клиента
 * @param type     тип события (игнорируется, приводится к DATAGRAM)
 * @param protocol протокол события (игнорируется, приводится к UDP)
 * @return         идентификатор созданного события клиента
 *
 */
awh::event::id_t awh::unit::QuicClient::issue(const event::family_t family, [[maybe_unused]] const event::type_t type, [[maybe_unused]] const event::protocol_t protocol) noexcept {
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
	// Добавляем событие клиента поверх UDP (транспорт QUIC работает поверх дейтаграммного UDP-сокета)
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
	// Сбрасываем флаг выполненного оповещения о завершённом соединении
	this->_notified = false;
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
	if(!this->_io->setAddress(this->_eid, ((this->_family == event::family_t::IPV6) ? event::address_t::IPV6 : event::address_t::IPV4), ((this->_family == event::family_t::IPV6) ? "::" : "0.0.0.0"))){
		// Записываем ошибку в лог
		this->_log->print("QUIC client address is not set", log_t::flag_t::CRITICAL);
		// Выводим отрицательный результат
		return 0;
	}
	// Устанавливаем функцию обратного вызова на чтение датаграмм соединения
	this->_io->on(this->_eid, static_cast <engine::callback::read_t> (std::bind(&QuicClient::read, this, _1, _2, _3)));
	/**
	 * Устанавливаем функцию обратного вызова на инъекцию объединённых данных соединения:
	 * перенаправленные из события-источника байты отправляются туннельным потоком
	 * соединения с шифрованием на уровне соединения, а не пишутся сырьём в сокет (splice)
	 */
	this->_io->on(this->_eid, static_cast <engine::callback::inject_t> (std::bind(&QuicClient::inject, this, _2, _3)));
	// Устанавливаем функцию обратного вызова на завершение подключения к серверу
	this->_io->on(this->_eid, static_cast <engine::callback::connect_t> (std::bind(&QuicClient::connected, this, _1, _2)));
	// Устанавливаем функцию обратного вызова на событие интервала таймеров соединения
	this->_io->on(this->_tid, static_cast <engine::callback::status_t> (std::bind(&QuicClient::tick, this, _1, _2)));
	// Выводим идентификатор созданного события клиента
	return this->_eid;
}
/**
 * @brief Метод фиксации настроек события клиента
 *
 * @param eid идентификатор события клиента
 * @return    результат выполнения фиксации
 *
 */
bool awh::unit::QuicClient::commit(const event::id_t eid) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем фиксацию настроек события клиента и события интервала таймеров соединения
		return (this->_io->commit(this->_eid) && this->_io->commit(this->_tid));
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод запуска работы события клиента
 *
 * @param eid идентификатор события клиента
 * @return    результат выполнения запуска
 *
 */
bool awh::unit::QuicClient::launch(const event::id_t eid) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем запуск события клиента и события интервала таймеров соединения
		return (this->_io->launch(this->_eid) && this->_io->launch(this->_tid));
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод приостановки работы события клиента
 *
 * @param eid идентификатор события клиента
 * @return    результат выполнения приостановки работы
 *
 */
bool awh::unit::QuicClient::pause(const event::id_t eid) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем приостановку работы события клиента
		return this->_io->pause(this->_eid);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод возобновления работы события клиента
 *
 * @param eid идентификатор события клиента
 * @return    результат выполнения возобновления работы
 *
 */
bool awh::unit::QuicClient::resume(const event::id_t eid) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем возобновление работы события клиента
		return this->_io->resume(this->_eid);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод отключения клиента от удалённого сервера
 *
 * @param eid идентификатор события клиента
 * @return    результат выполнения отключения
 *
 */
bool awh::unit::QuicClient::disconnect(const event::id_t eid) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем отключение клиента от удалённого сервера
		return this->_io->disconnect(this->_eid);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод подключения клиента к удалённому серверу
 *
 * @param eid идентификатор события клиента
 * @return    результат выполнения подключения
 *
 */
bool awh::unit::QuicClient::connect(const event::id_t eid) noexcept {
	// Если событие клиента не актуально
	if(!this->isActual(eid))
		// Выводим отрицательный результат
		return false;
	// Если шаблон контекста безопасности не установлен
	if((this->_coder == nullptr) || (this->_ctx == 0)){
		// Записываем ошибку в лог
		this->_log->print("QUIC security context is not set", log_t::flag_t::CRITICAL);
		// Выводим отрицательный результат
		return false;
	}
	// Сбрасываем флаг выполненного оповещения о завершённом соединении
	this->_notified = false;
	/**
	 * Сбрасываем идентификатор туннельного потока сплайса: нумерация потоков
	 * ведётся в пределах соединения, поэтому на новом соединении прежний поток
	 * недействителен и должен быть открыт заново при следующей передаче (см. inject())
	 */
	this->_tunnel = quic::connection_t::INVALID_STREAM;
	// Создаём соединение QUIC на шаблоне контекста безопасности
	this->_connection = make_unique <quic::connection_t> (
		quic::endpoint_t::CLIENT, this->_ctx,
		* this->_coder, this->_log
	);
	// Устанавливаем локальные транспортные параметры соединения
	this->_connection->params(this->_params);
	// Устанавливаем адрес удалённого эндпоинта соединению из ранее установленной структуры адреса и порта
	this->_connection->address(this->_target.get(), this->_targetPort);
	// Устанавливаем режим маркировки исходящих датаграмм соединения
	this->_connection->ecn(this->_ecn);
	/**
	 * Билет возобновления сессии соединение подставляет самостоятельно из кэша
	 * кодера по ключу сервера при начале хендшейка (RFC 9001 §4.6)
	 */
	/**
	 * Подставляем сохранённый токен проверки адреса: предъявление токена в первом
	 * пакете подтверждает адрес клиента сразу, и обмен пакетом Retry не выполняется
	 * (RFC 9000 §8.1.3)
	 */
	if(!this->_token.empty())
		// Устанавливаем сохранённый токен проверки адреса соединению
		this->_connection->token(this->_token);
	// Выполняем подключение к удалённому серверу
	return this->_io->connect(this->_eid);
}
/**
 * @brief Метод подключения клиента к удалённому серверу
 *
 * @param ids список идентификаторов событий для подключения
 * @return    результат выполнения подключения
 *
 */
bool awh::unit::QuicClient::connect(const vector <event::id_t> & ids) noexcept {
	// Для транспорта QUIC поддерживается единственное соединение на событие клиента
	if((ids.size() == 1) && this->isActual(ids.front()))
		// Выполняем подключение к удалённому серверу
		return this->connect(ids.front());
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения данных от удалённого сервера
 *
 * @param eid идентификатор события клиента
 * @return    результат получения данных
 *
 */
bool awh::unit::QuicClient::recv(const event::id_t eid) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение данных от удалённого сервера
		return this->_io->recv(this->_eid);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод отправки данных удалённому серверу
 *
 * @param eid    идентификатор события клиента
 * @param buffer буфер данных для отправки
 * @param size   размер данных для отправки
 * @return       количество байт, поставленных в очередь отправки
 *
 */
size_t awh::unit::QuicClient::send(const event::id_t eid, const void * buffer, const size_t size) noexcept {
	// Если событие не актуально либо соединение не создано либо данные отсутствуют
	if(!this->isActual(eid) || (this->_connection == nullptr) || (buffer == nullptr) || (size == 0))
		// Возвращаем значение по умолчанию
		return 0;
	// Открываем двунаправленный поток приложения по умолчанию (RFC 9000 §2.1)
	const uint64_t sid = this->_connection->open(false);
	// Если поток приложения не открыт
	if(sid == quic::connection_t::INVALID_STREAM)
		// Возвращаем значение по умолчанию
		return 0;
	// Если постановка данных в очередь отправки потока не выполнена
	if(this->_connection->send(sid, string_view(reinterpret_cast <const char *> (buffer), size), false) != quic::status_t::OK)
		// Возвращаем значение по умолчанию
		return 0;
	// Отправляем все готовые исходящие датаграммы
	this->flush();
	// Выводим количество поставленных в очередь отправки байт
	return size;
}
/**
 * @brief Метод объединения потоков данных между двумя событиями
 *
 * @param eid  идентификатор события-источника
 * @param dest идентификатор события-приёмника
 * @return     результат объединения
 *
 */
bool awh::unit::QuicClient::splice(const event::id_t eid, const event::id_t dest) noexcept {
	/**
	 * Объединение данных QUIC ведётся на уровне сессии, а не сокета: собранные
	 * (расшифрованные) данные потоков соединения перенаправляются в событие-приёмник,
	 * а входящие в соединение объединённые байты отправляются туннельным потоком
	 * с шифрованием на уровне соединения. Направление задаётся отдельным вызовом
	 * на каждую сторону (как для прочих транспортов)
	 */
	// Если событие клиента является источником объединения данных
	if(this->isActual(eid)){
		// Устанавливаем событие-приёмник объединения данных соединения
		this->_dest = dest;
		/**
		 * Сразу выдаём уже собранные данные потоков, накопленные до установки
		 * приёмника: без нового входящего события метод process() иначе не будет
		 * вызван и буферизованные данные не пойдут в событие-приёмник
		 */
		this->process();
		// Возвращаем положительный результат
		return true;
	// Если событие клиента является приёмником объединения данных
	} else if(this->isActual(dest))
		/**
		 * Входящее направление (событие-источник -> клиент) обслуживается инъекцией
		 * байт в туннельный поток соединения через сетевой движок: привязываем
		 * внешнее событие-источник к клиенту на уровне движка, тогда входящие в
		 * источник байты будут инъецированы в туннельный поток соединения, минуя
		 * выдачу приложению (см. inject). Так обеспечивается симметрия с сервером
		 */
		return this->_io->splice(eid, dest);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод уничтожения события клиента
 *
 * @param eid идентификатор события для уничтожения
 *
 */
void awh::unit::QuicClient::destroy(const event::id_t eid) noexcept {
	// Если событие клиента не актуально
	if(!this->isActual(eid))
		// Выходим из метода
		return;
	// Освобождаем объект соединения QUIC
	this->_connection.reset(nullptr);
	// Сбрасываем состояние сплайса: туннельный поток и цель форвардинга недействительны после уничтожения события
	this->_tunnel = quic::connection_t::INVALID_STREAM;
	// Сбрасываем цель форвардинга сплайса
	this->_dest = 0;
	// Если событие интервала таймеров соединения создано
	if(this->_tid != 0){
		// Уничтожаем событие интервала таймеров соединения
		this->_io->destroy(this->_tid);
		// Сбрасываем идентификатор события интервала таймеров соединения
		this->_tid = 0;
	}
	// Снимаем функцию обратного вызова на чтение датаграмм соединения
	this->_io->on(this->_eid, static_cast <engine::callback::read_t> (nullptr));
	// Снимаем функцию обратного вызова на завершение подключения к серверу
	this->_io->on(this->_eid, static_cast <engine::callback::connect_t> (nullptr));
	// Уничтожаем событие клиента
	this->_io->destroy(this->_eid);
	// Сбрасываем идентификатор события клиента
	this->_eid = 0;
}
/**
 * @brief Метод получения опций события клиента
 *
 * @param eid идентификатор события клиента
 * @return    опции события клиента
 *
 */
uint16_t awh::unit::QuicClient::getOptions(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение опций для события клиента
		return this->_io->getOptions(this->_eid);
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки опций события клиента
 *
 * @param eid     идентификатор события клиента
 * @param options опции события клиента для установки
 * @return        результат выполнения установки
 *
 */
bool awh::unit::QuicClient::setOptions(const event::id_t eid, const uint16_t options) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку опций для события клиента
		return this->_io->setOptions(this->_eid, options);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки опции события клиента
 *
 * @param eid    идентификатор события клиента
 * @param option опция события клиента для установки
 * @param mode   режим установки опции события клиента
 * @return       результат выполнения установки
 *
 */
bool awh::unit::QuicClient::setOption(const event::id_t eid, const uint16_t option, const bool mode) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку опции для события клиента
		return this->_io->setOption(this->_eid, option, mode);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения метаданных последнего принятого дейтаграммного пакета
 *
 * @param eid идентификатор события клиента
 * @return    метаданные последнего принятого дейтаграммного пакета
 *
 */
awh::net::dgram_info_t awh::unit::QuicClient::getTrafficInfo(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение метаданных последнего принятого дейтаграммного пакета для события клиента
		return this->_io->getTrafficInfo(this->_eid);
	// Возвращаем значение по умолчанию
	return net::dgram_info_t();
}
/**
 * @brief Метод получения количества хопов последнего принятого пакета
 *
 * @param eid идентификатор события клиента
 * @return    количество хопов последнего принятого пакета
 *
 */
uint8_t awh::unit::QuicClient::getCountHops(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение количества хопов последнего принятого пакета для события клиента
		return this->_io->getCountHops(this->_eid);
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки количества хопов последнего принятого пакета
 *
 * @param eid  идентификатор события клиента
 * @param hops количество хопов последнего принятого пакета
 * @return     результат выполнения установки
 *
 */
bool awh::unit::QuicClient::setCountHops(const event::id_t eid, const uint8_t hops) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку количества хопов последнего принятого пакета для события клиента
		return this->_io->setCountHops(this->_eid, hops);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения максимального количества хопов, через которые может пройти пакет
 *
 * @param eid идентификатор события клиента
 * @return    максимальное количество хопов
 *
 */
awh::event::hops_t awh::unit::QuicClient::getHops(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение максимального количества хопов для события клиента
		return this->_io->getHops(this->_eid);
	// Возвращаем значение по умолчанию
	return event::hops_t::LOOPBACK;
}
/**
 * @brief Метод установки максимального количества хопов, через которые может пройти пакет
 *
 * @param eid  идентификатор события клиента
 * @param hops максимальное количество хопов
 * @return     результат работы функции
 *
 */
bool awh::unit::QuicClient::setHops(const event::id_t eid, const event::hops_t hops) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку максимального количества хопов для события клиента
		return this->_io->setHops(this->_eid, hops);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения сетевого интерфейса клиента
 *
 * @param eid идентификатор события клиента
 * @return    сетевой интерфейс клиента
 *
 */
string awh::unit::QuicClient::getIface(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение сетевого интерфейса для события клиента
		return this->_io->getIface(this->_eid);
	// Возвращаем значение по умолчанию
	return "";
}
/**
 * @brief Метод установки сетевого интерфейса клиента
 *
 * @param eid  идентификатор события клиента
 * @param name имя сетевого интерфейса для установки
 * @return     результат выполнения установки
 *
 */
bool awh::unit::QuicClient::setIface(const event::id_t eid, string_view name) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку сетевого интерфейса для события клиента
		return this->_io->setIface(this->_eid, name);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения внутреннего порта события
 *
 * @param eid идентификатор события
 * @return    внутренний порт события
 *
 */
uint16_t awh::unit::QuicClient::getSourcePort(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение внутреннего порта для события клиента
		return this->_io->getSourcePort(this->_eid);
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки внутреннего порта события
 *
 * @param eid  идентификатор события
 * @param port внутренний порт события
 * @return     результат выполнения установки
 *
 */
bool awh::unit::QuicClient::setSourcePort(const event::id_t eid, const uint16_t port) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку внутреннего порта для события клиента
		return this->_io->setSourcePort(this->_eid, port);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения порта удалённого сервера
 *
 * @param eid идентификатор события клиента
 * @return    порт удалённого сервера
 *
 */
uint16_t awh::unit::QuicClient::getTargetPort(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение порта удалённого сервера для события клиента
		return this->_io->getTargetPort(this->_eid);
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки порта удалённого сервера
 *
 * @param eid  идентификатор события клиента
 * @param port порт удалённого сервера для установки
 * @return     результат выполнения установки
 *
 */
bool awh::unit::QuicClient::setTargetPort(const event::id_t eid, const uint16_t port) noexcept {
	// Если событие клиента не актуально
	if(!this->isActual(eid))
		// Возвращаем значение по умолчанию
		return false;
	// Запоминаем порт удалённого сервера для установки адреса эндпоинта соединению
	this->_targetPort = port;
	// Выполняем установку порта удалённого сервера для события клиента
	return this->_io->setTargetPort(this->_eid, port);
}
/**
 * @brief Метод получения адреса хоста целевой машины
 *
 * @param eid идентификатор события клиента
 * @return    адрес хоста целевой машины
 *
 */
string awh::unit::QuicClient::getTarget(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение адреса хоста целевой машины для события клиента
		return this->_io->getTarget(this->_eid);
	// Возвращаем значение по умолчанию
	return "";
}
/**
 * @brief Метод установки адреса хоста целевой машины
 *
 * @param eid    идентификатор события клиента
 * @param target адрес хоста целевой машины
 * @return       результат выполнения установки
 *
 */
bool awh::unit::QuicClient::setTarget(const event::id_t eid, string_view target) noexcept {
	// Если событие клиента не актуально
	if(!this->isActual(eid))
		// Возвращаем значение по умолчанию
		return false;
	// Разбираем адрес удалённого сервера в штатную структуру сетевого адреса фреймворка
	net_addr_t resolver(this->_fmk, this->_log);
	// Если разбор адреса удалённого сервера выполнен успешно
	if(resolver.parse(target))
		// Запоминаем структуру сетевого адреса удалённого сервера для установки адреса эндпоинта соединению
		this->_target = resolver.source();
	// Выполняем установку адреса хоста целевой машины для события клиента
	return this->_io->setTarget(this->_eid, target);
}
/**
 * @brief Метод установки адреса хоста целевой машины
 *
 * @param eid    идентификатор события клиента
 * @param target структура сетевого адреса хоста целевой машины
 * @return       результат выполнения установки
 *
 */
bool awh::unit::QuicClient::setTarget(const event::id_t eid, const net::addr_t * target) noexcept {
	// Если событие клиента не актуально
	if(!this->isActual(eid))
		// Возвращаем значение по умолчанию
		return false;
	// Запоминаем структуру сетевого адреса удалённого сервера для установки адреса эндпоинта соединению
	if(target != nullptr){
		/**
		 * Выполняем глубокое копирование штатной структуры сетевого адреса
		 */
		switch(target->size){
			// Для сетевого адреса IPv4
			case 4: {
				// Выделяем память под структуру адреса IPv4
				auto addr = make_unique <net::addr_net_ipv4_t> ();
				// Копируем IPv4-адрес в чистом виде
				addr->address = static_cast <const net::addr_net_ipv4_t *> (target)->address;
				// Запоминаем структуру сетевого адреса удалённого сервера
				this->_target = ::move(addr);
			} break;
			// Для сетевого адреса IPv6
			case 16: {
				// Выделяем память под структуру адреса IPv6
				auto addr = make_unique <net::addr_net_ipv6_t> ();
				// Копируем IPv6-адрес в чистом виде
				addr->address = static_cast <const net::addr_net_ipv6_t *> (target)->address;
				// Запоминаем структуру сетевого адреса удалённого сервера
				this->_target = ::move(addr);
			} break;
		}
	}
	// Выполняем установку адреса хоста целевой машины для события клиента
	return this->_io->setTarget(this->_eid, target);
}
/**
 * @brief Метод получения адреса хоста целевой машины
 *
 * @param eid    идентификатор события клиента
 * @param target объект для извлечения адреса хоста целевой машины
 * @return       результат выполнения извлечения адреса хоста целевой машины
 *
 */
bool awh::unit::QuicClient::getTarget(const event::id_t eid, unique_ptr <net::addr_t> & target) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение адреса хоста целевой машины для события клиента
		return this->_io->getTarget(this->_eid, target);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения адреса клиента
 *
 * @param eid     идентификатор события клиента
 * @param address тип адреса клиента
 * @return        значение адреса клиента
 *
 */
string awh::unit::QuicClient::getAddress(const event::id_t eid, const event::address_t address) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение адреса клиента для события клиента
		return this->_io->getAddress(this->_eid, address);
	// Возвращаем значение по умолчанию
	return "";
}
/**
 * @brief Метод установки адреса клиента
 *
 * @param eid     идентификатор события клиента
 * @param address тип адреса клиента
 * @param value   значение адреса клиента
 * @return        результат выполнения установки
 *
 */
bool awh::unit::QuicClient::setAddress(const event::id_t eid, const event::address_t address, string_view value) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку адреса клиента для события клиента
		return this->_io->setAddress(this->_eid, address, value);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки адреса клиента
 *
 * @param eid     идентификатор события клиента
 * @param address тип адреса клиента
 * @param value   структура сетевого адреса клиента
 * @return        результат выполнения установки
 *
 */
bool awh::unit::QuicClient::setAddress(const event::id_t eid, const event::address_t address, const net::addr_t * value) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку адреса клиента для события клиента
		return this->_io->setAddress(this->_eid, address, value);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения адреса клиента
 *
 * @param eid     идентификатор события клиента
 * @param address тип адреса клиента
 * @param value   объект для извлечения адреса клиента
 * @return        результат выполнения извлечения адреса клиента
 *
 */
bool awh::unit::QuicClient::getAddress(const event::id_t eid, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение адреса клиента для события клиента
		return this->_io->getAddress(this->_eid, address, value);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения MTU сетевого интерфейса
 *
 * @param eid идентификатор события клиента
 * @return    MTU сетевого интерфейса
 *
 */
uint16_t awh::unit::QuicClient::getMaximumTransmissionUnit(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение MTU сетевого интерфейса для события клиента
		return this->_io->getMaximumTransmissionUnit(this->_eid);
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки MTU сетевого интерфейса
 *
 * @param eid идентификатор события клиента
 * @param mtu размер MTU интерфейса
 * @return    результат установки MTU сетевого интерфейса
 *
 */
bool awh::unit::QuicClient::setMaximumTransmissionUnit(const event::id_t eid, const uint16_t mtu) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку MTU сетевого интерфейса для события клиента
		return this->_io->setMaximumTransmissionUnit(this->_eid, mtu);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения режима обнаружения максимального размера пакета (MTU)
 *
 * @param eid идентификатор события клиента
 * @return    текущий режим обнаружения MTU
 *
 */
awh::event::mtu_discover_t awh::unit::QuicClient::getMaximumTransmissionUnitDiscover(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение режима обнаружения MTU для события клиента
		return this->_io->getMaximumTransmissionUnitDiscover(this->_eid, this->_io->family(this->_eid));
	// Возвращаем значение по умолчанию
	return event::mtu_discover_t::NONE;
}
/**
 * @brief Метод установки режима обнаружения максимального размера пакета (MTU)
 *
 * @param eid  идентификатор события клиента
 * @param mode режим обнаружения максимального размера пакета (MTU)
 * @return     результат работы функции
 *
 */
bool awh::unit::QuicClient::setMaximumTransmissionUnitDiscover(const event::id_t eid, const event::mtu_discover_t mode) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку режима обнаружения MTU для события клиента
		return this->_io->setMaximumTransmissionUnitDiscover(this->_eid, this->_io->family(this->_eid), mode);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения режима трансляции пакетов клиента
 *
 * @param eid идентификатор события клиента
 * @return    режим трансляции пакетов (unicast, multicast, broadcast)
 *
 */
awh::event::delivery_mode_t awh::unit::QuicClient::getDelivery(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение режима трансляции пакетов клиента для события клиента
		return this->_io->getDelivery(this->_eid);
	// Возвращаем значение по умолчанию
	return event::delivery_mode_t::NONE;
}
/**
 * @brief Метод установки режима трансляции пакетов клиента
 *
 * @param eid      идентификатор события клиента
 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
 * @return         результат выполнения установки
 *
 */
bool awh::unit::QuicClient::setDelivery(const event::id_t eid, const event::delivery_mode_t delivery) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку режима трансляции пакетов клиента для события клиента
		return this->_io->setDelivery(this->_eid, delivery);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения размера буфера клиента
 *
 * @param eid    идентификатор события клиента
 * @param action тип действия клиента
 * @return       размер буфера клиента
 *
 */
size_t awh::unit::QuicClient::getBufferSize(const event::id_t eid, const event::action_t action) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение размера буфера клиента для события клиента
		return this->_io->getBufferSize(this->_eid, action);
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки размера буфера клиента
 *
 * @param eid    идентификатор события клиента
 * @param action тип действия клиента
 * @param size   размер буфера клиента
 * @return       результат выполнения установки
 *
 */
bool awh::unit::QuicClient::setBufferSize(const event::id_t eid, const event::action_t action, const size_t size) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку размера буфера клиента для события клиента
		return this->_io->setBufferSize(this->_eid, action, size);
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
awh::event::usage_t awh::unit::QuicClient::getUsageReadTimeout(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение режима использования таймаута на чтение события для события клиента
		return this->_io->getUsageReadTimeout(this->_eid);
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
void awh::unit::QuicClient::setUsageReadTimeout(const event::id_t eid, const event::usage_t usage) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку режима использования таймаута на чтение события для события клиента
		this->_io->setUsageReadTimeout(this->_eid, usage);
}
/**
 * @brief Метод получения таймаута клиента
 *
 * @param eid    идентификатор события клиента
 * @param action тип действия клиента
 * @return       значение таймаута в миллисекундах
 *
 */
uint32_t awh::unit::QuicClient::getTimeout(const event::id_t eid, const event::action_t action) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение параметров таймаута для клиента
		return this->_io->getTimeout(this->_eid, action);
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки таймаута клиента
 *
 * @param eid     идентификатор события клиента
 * @param action  тип действия клиента
 * @param timeout значение таймаута в миллисекундах
 *
 */
void awh::unit::QuicClient::setTimeout(const event::id_t eid, const event::action_t action, const uint32_t timeout) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку параметров таймаута для клиента
		this->_io->setTimeout(this->_eid, action, timeout);
}
/**
 * @brief Метод установки пропускной способности клиента
 *
 * @param eid       идентификатор события клиента
 * @param limiting  режим ограничения пропускной способности клиента (egress или ingress)
 * @param bandwidth пропускная способность клиента для установки
 * @return          результат выполнения установки
 *
 */
bool awh::unit::QuicClient::bandwidth(const event::id_t eid, const event::limiting_t limiting, string_view bandwidth) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку параметров пропускной способности для клиента
		return this->_io->bandwidth(this->_eid, limiting, bandwidth);
	// Возвращаем значение по умолчанию
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
 *
 */
bool awh::unit::QuicClient::keepAlive(const event::id_t eid, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку параметров keep-alive для клиента
		return this->_io->keepAlive(this->_eid, cnt, idle, intvl);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
 *
 * @param eid идентификатор события клиента
 * @return    значение DSCP
 *
 */
awh::event::dscp_t awh::unit::QuicClient::getDifferentiatedServicesCodePoint(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение значения поля Differentiated Services Code Point (DSCP) для клиента
		return this->_io->getDifferentiatedServicesCodePoint(this->_eid, this->_io->family(this->_eid));
	// Возвращаем значение по умолчанию
	return event::dscp_t::CS0;
}
/**
 * @brief Метод установки значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
 *
 * @param eid  идентификатор события клиента
 * @param dscp значение DSCP
 * @return     результат работы функции
 *
 */
bool awh::unit::QuicClient::setDifferentiatedServicesCodePoint(const event::id_t eid, const event::dscp_t dscp) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку значения поля Differentiated Services Code Point (DSCP) для клиента
		return this->_io->setDifferentiatedServicesCodePoint(this->_eid, this->_io->family(this->_eid), dscp);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод активации/деактивации мультикаст-группы
 *
 * @param eid    идентификатор события клиента
 * @param mode   режим активации/деактивации
 * @param group  мультикаст-группа для активации/деактивации
 * @param source адрес сетевого интерфейса с которого выполняется подписка
 * @param port   порт мультикаст-группы с которого выполняется подписка
 * @return       результат выполнения установки
 *
 */
bool awh::unit::QuicClient::membership(const event::id_t eid, const event::mode_t mode, string_view group, string_view source, const uint16_t port) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем активацию/деактивацию мультикаст-группы для клиента
		return this->_io->membership(this->_eid, mode, group, source, port);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод активации/деактивации мультикаст-группы
 *
 * @param eid    идентификатор события клиента
 * @param mode   режим активации/деактивации
 * @param group  мультикаст-группа для активации/деактивации
 * @param source адрес сетевого интерфейса с которого выполняется подписка
 * @param port   порт мультикаст-группы с которого выполняется подписка
 * @return       результат выполнения установки
 *
 */
bool awh::unit::QuicClient::membership(const event::id_t eid, const event::mode_t mode, const net::addr_t * group, const net::addr_t * source, const uint16_t port) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем активацию/деактивацию мультикаст-группы для клиента
		return this->_io->membership(this->_eid, mode, group, source, port);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод остановки клиента
 *
 */
void awh::unit::QuicClient::stop() noexcept {
	// Если работа юнита запущена
	if(this->working())
		// Выполняем остановку работы основного юнита
		unit_t::stop();
}
/**
 * @brief Метод запуска клиента
 *
 */
void awh::unit::QuicClient::start() noexcept {
	// Если работа юнита ещё не запущена
	if(!this->working()){
		// Выполняем получение идентификатора функции обратного вызова
		const callback_t::id_t fid = this->_callback.id("status");
		// Если функция обратного вызова установлена
		if(this->_callback.is(fid))
			// Выполняем получение функции обратного вызова
			this->_callback.set(fid, this->_callback.id("client_status"), this->_callback);
		// Устанавливаем функцию обратного вызова на запуск системы
		this->_callback.on <void (const event::status_t)> (fid, static_cast <void (quic_client_t::*)(const event::status_t)> (&quic_client_t::launch), this, _1);
		// Выполняем запуск работы основного юнита
		unit_t::start();
	}
}
/**
 * @brief Метод открытия потока приложения соединения
 *
 * @param mode режим однонаправленного потока
 * @return     идентификатор открытого потока
 *
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
 * @brief Метод проверки принятия ранних данных удалённым сервером (RFC 9001 §4.6.2)
 *
 * @return результат проверки
 *
 */
bool awh::unit::QuicClient::early() const noexcept {
	// Если соединение не создано
	if(this->_connection == nullptr)
		// Выводим отрицательный результат
		return false;
	// Выводим результат принятия ранних данных соединением
	return this->_connection->early();
}
/**
 * @brief Метод отправки данных в поток приложения соединения
 *
 * @param sid  идентификатор потока приложения
 * @param data отправляемые данные
 * @param fin  флаг завершения потока
 * @return     результат постановки данных в очередь отправки
 *
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
 *
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
 *
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
 *
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
 *
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
 *
 */
void awh::unit::QuicClient::callback(const callback_t & callback) noexcept {
	// Выполняем установку функции обратного вызова на готовность отправки ранних данных
	this->_callback.set("early", callback);
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
 *
 */
awh::unit::QuicClient::QuicClient(const fmk_t * fmk, const log_t * log) noexcept :
 unit_t(fmk, log), _eid(0), _tid(0), _connected(false), _notified(false), _ecn(false),
 _family(event::family_t::IPV4), _marking(event::ecn_t::NOT_ECT), _ctx(0), _coder(nullptr),
 _token{""}, _targetPort(0), _target(nullptr), _connection(nullptr),
 _dest(0), _tunnel(quic::connection_t::INVALID_STREAM) {}
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
