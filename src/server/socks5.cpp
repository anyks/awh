/**
 * @file: socks5.cpp
 * @date: 2022-09-03
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
#include <server/socks5.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * crash Метод обработки вызова крашей в приложении
 * @param sig номер сигнала операционной системы
 */
void awh::server::ProxySocks5::crash(const int32_t sig) noexcept {
	// Если функция обратного вызова при получении крашей установлена
	if(this->_callback.is("crash"))
		// Выводим функцию обратного вызова
		this->_callback.call <void (const int32_t)> ("crash", sig);
	// Если функция обратного вызова не установлена
	else {
		// Если мы получили сигнал завершения работы
		if(sig == 2)
			// Выводим сообщение о заверщении работы
			this->_log->print("%s finishing work, goodbye!", log_t::flag_t::INFO, AWH_NAME);
		// Выводим сообщение об ошибке
		else this->_log->print("%s cannot be continued, signal: [%u]. Finishing work, goodbye!", log_t::flag_t::CRITICAL, AWH_NAME, sig);
		// Выполняем отключение вывода лога
		const_cast <awh::log_t *> (this->_log)->level(awh::log_t::level_t::NONE);
		// Выполняем остановку работы сервера
		this->stop();
		// Завершаем работу приложения
		::exit(sig);
	}
}
/**
 * openEvents Метод обратного вызова при запуске работы
 * @param sid идентификатор схемы сети
 */
void awh::server::ProxySocks5::openEvents(const uint16_t sid) noexcept {
	// Если данные существуют
	if(sid > 0){
		// Устанавливаем хост сервера
		this->_core.init(sid, this->_port, this->_host);
		// Если хост передан пустой
		if(this->_host.empty())
			// Выполняем компенсацию хоста
			this->_host = this->_core.host(sid);
		// Выполняем запуск сервера
		this->_core.launch(sid);
	}
}
/**
 * launchedEvents Метод получения события запуска сервера
 * @param host хост запущенного сервера
 * @param port порт запущенного сервера
 */
void awh::server::ProxySocks5::launchedEvents(const string & host, const uint32_t port) noexcept {
	// Если параметра активации сервера переданы
	if(!host.empty()){
		// Если функция обратного вызова установлена
		if(this->_callback.is("launched"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const string &, const uint32_t)> ("launched", host, port);
	}
}
/**
 * acceptEvents Метод обратного вызова при проверке подключения клиента
 * @param ip   адрес интернет подключения клиента
 * @param mac  мак-адрес подключившегося клиента
 * @param port порт подключившегося брокера
 * @param sid  идентификатор схемы сети
 * @return     результат разрешения к подключению клиента
 */
bool awh::server::ProxySocks5::acceptEvents(const string & ip, const string & mac, const uint32_t port, const uint16_t sid) noexcept {
	// Если данные существуют
	if(!ip.empty() && !mac.empty() && (sid > 0)){
		// Если функция обратного вызова установлена
		if(this->_callback.is("accept"))
			// Выводим функцию обратного вызова
			return this->_callback.call <bool (const string &, const string &, const uint32_t)> ("accept", ip, mac, port);
	}
	// Разрешаем подключение клиенту
	return false;
}
/**
 * connectClientEvents Метод обратного вызова при подключении
 * @param broker брокер вызвавший событие
 * @param bid1   идентификатор брокера сервера
 * @param bid2   идентификатор брокера клиента
 * @param sid    идентификатор схемы сети
 */
void awh::server::ProxySocks5::connectEvents(const broker_t broker, const uint64_t bid1, const uint64_t bid2, const uint16_t sid) noexcept {
	// Если данные существуют
	if((bid1 > 0) && (sid > 0)){
		// Определяем тип активного брокера
		switch(static_cast <uint8_t> (broker)){
			// Если брокер является клиентом
			case static_cast <uint8_t> (broker_t::CLIENT): {
				// Получаем параметры активного клиента
				scheme::socks5_t::options_t * options = const_cast <scheme::socks5_t::options_t *> (this->_scheme.get(bid1));
				// Если подключение не выполнено
				if((options != nullptr) && !options->connect){
					// Устанавливаем идентификатор клиента
					options->id = bid2;
					// Разрешаем обработки данных
					options->locked = false;
					// Запоминаем, что подключение выполнено
					options->connect = !options->connect;
					// Устанавливаем флаг разрешающий подключение
					options->socks5.cmd(socks5_t::rep_t::SUCCESS);
					// Получаем данные запроса
					const auto & buffer = options->socks5.get();
					// Если данные получены
					if(!buffer.empty())
						// Выполняем запись полученных данных на сервер
						this->_core.send(buffer.data(), buffer.size(), bid1);
				}
			} break;
			// Если брокер является сервером
			case static_cast <uint8_t> (broker_t::SERVER): {
				// Создаём брокера
				this->_scheme.set(bid1);
				// Получаем параметры активного клиента
				scheme::socks5_t::options_t * options = const_cast <scheme::socks5_t::options_t *> (this->_scheme.get(bid1));
				// Если параметры активного клиента получены
				if(options != nullptr){
					// Устанавливаем количество секунд на чтение
					options->scheme.timeouts.read = this->_scheme.timeouts.read;
					// Устанавливаем количество секунд на запись
					options->scheme.timeouts.write = this->_scheme.timeouts.write;
					// Выполняем установку максимального количества попыток
					options->scheme.keepAlive.cnt = this->_scheme.keepAlive.cnt;
					// Выполняем установку интервала времени в секундах через которое происходит проверка подключения
					options->scheme.keepAlive.idle = this->_scheme.keepAlive.idle;
					// Выполняем установку интервала времени в секундах между попытками
					options->scheme.keepAlive.intvl = this->_scheme.keepAlive.intvl;
					// Если функция обратного вызова при выполнении авторизации установлена
					if(this->_callback.is("checkPassword"))
						// Устанавливаем функцию проверки авторизации
						options->socks5.authCallback(std::bind(this->_callback.get <bool (const uint64_t, const string &, const string &)> ("checkPassword"), bid1, _1, _2));
					// Если unix-сокет установлен
					if(!this->_socket.empty()){
						// Устанавливаем тип сети
						options->scheme.url.family = AF_UNIX;
						// Устанавливаем хост сервера
						options->scheme.url.host = this->_socket;
					// Если сервер слушает порт
					} else {
						// Определяем тип хоста сервера
						switch(static_cast <uint8_t> (this->_net.host(this->_host))){
							// Если хост является адресом IPv4
							case static_cast <uint8_t> (net_t::type_t::IPV4): {
								// Устанавливаем тип сети
								options->scheme.url.family = AF_INET;
								// Устанавливаем хост сервера
								options->scheme.url.ip = this->_host;
							} break;
							// Если хост является адресом IPv6
							case static_cast <uint8_t> (net_t::type_t::IPV6): {
								// Устанавливаем тип сети
								options->scheme.url.family = AF_INET6;
								// Устанавливаем хост сервера
								options->scheme.url.ip = this->_host;
							} break;
							// Если хост является доменной зоной
							case static_cast <uint8_t> (net_t::type_t::FQDN):
								// Устанавливаем хост сервера
								options->scheme.url.domain = this->_host;
							break;
						}
						// Устанавливаем хост сервера
						options->scheme.url.host = this->_host;
						// Устанавливаем порт сервера
						options->scheme.url.port = this->_port;
					}
					// Устанавливаем URL адрес запроса
					options->socks5.url(options->scheme.url);
					// Если функция обратного вызова при подключении/отключении установлена
					if(this->_callback.is("active"))
						// Выводим функцию обратного вызова
						this->_callback.call <void (const uint64_t, const mode_t)> ("active", bid1, mode_t::CONNECT);
					// Выполняем создание нового подключённого клиента
					auto ret = this->_clients.emplace(bid1, std::make_unique <client::core_t> (&this->_dns, this->_fmk, this->_log));
					// Выполняем отключение информационных сообщений сетевого ядра клиента
					ret.first->second->verbose(false);
					// Устанавливаем параметры SSL-шифрвоания
					ret.first->second->ssl(this->_settings.ssl);
					// Добавляем схему сети в сетевое ядро
					ret.first->second->scheme(&options->scheme);
					// Устанавливаем параметры сети клиента
					ret.first->second->network(this->_settings.ips);
					// Активируем правило асинхронной работы передачи данных
					ret.first->second->transferRule(client::core_t::transfer_t::ASYNC);
					// Выполняем установку размера памяти для хранения полезной нагрузки всех брокеров
					ret.first->second->memoryAvailableSize(this->_memoryAvailableSize);
					// Выполняем установку размера хранимой полезной нагрузки для одного брокера
					ret.first->second->brokerAvailableSize(this->_brokerAvailableSize);
					// Устанавливаем событие подключения
					ret.first->second->on <void (const uint64_t, const uint16_t)> ("connect", &proxy_socks5_t::connectEvents, this, broker_t::CLIENT, bid1, _1, _2);
					// Устанавливаем событие отключения
					ret.first->second->on <void (const uint64_t, const uint16_t)> ("disconnect", &proxy_socks5_t::disconnectEvents, this, broker_t::CLIENT, bid1, _1, _2);
					// Устанавливаем функцию чтения данных
					ret.first->second->on <void (const char *, const size_t, const uint64_t, const uint16_t)> ("read", &proxy_socks5_t::readEvents, this, broker_t::CLIENT, _1, _2, bid1, _4);
					// Устанавливаем функцию обратного вызова на получение событий очистки буферов полезной нагрузки
					ret.first->second->on <void (const uint64_t, const size_t)> ("available", &proxy_socks5_t::available, this, broker_t::CLIENT, _1, _2, ret.first->second.get());
					// Устанавливаем функцию обратного вызова на получение событий очистки буферов полезной нагрузки
					ret.first->second->on <void (const uint64_t, const char *, const size_t)> ("unavailable", &proxy_socks5_t::unavailable, this, broker_t::CLIENT, _1, _2, _3);
					// Выполняем подключение клиента к сетевому ядру
					this->_core.bind(ret.first->second.get());
					// Выполняем запуск работы клиента
					ret.first->second->start();
				}
			} break;
		}
	}
}
/**
 * disconnectClientEvents Метод обратного вызова при отключении
 * @param broker брокер вызвавший событие
 * @param bid1   идентификатор брокера сервера
 * @param bid2   идентификатор брокера клиента
 * @param sid    идентификатор схемы сети
 */
void awh::server::ProxySocks5::disconnectEvents(const broker_t broker, const uint64_t bid1, const uint64_t bid2, const uint16_t sid) noexcept {
	// Если данные существуют
	if((sid > 0) && (bid1 > 0)){
		// Определяем тип активного брокера
		switch(static_cast <uint8_t> (broker)){
			// Если брокер является клиентом
			case static_cast <uint8_t> (broker_t::CLIENT): {
				// Получаем параметры активного клиента
				scheme::socks5_t::options_t * options = const_cast <scheme::socks5_t::options_t *> (this->_scheme.get(bid1));
				// Если подключение не выполнено, отправляем ответ клиенту
				if((options != nullptr) && !options->connect){
					// Устанавливаем флаг запрещающий подключение
					options->socks5.cmd(socks5_t::rep_t::DENIED);
					// Получаем данные запроса
					const auto & buffer = options->socks5.get();
					// Если данные получены
					if((options->stopped = !buffer.empty()))
						// Отправляем сообщение клиенту
						this->_core.send(buffer.data(), buffer.size(), bid1);
				}
				// Выполняем закрытие подключения
				this->close(bid1);
			} break;
			// Если брокер является сервером
			case static_cast <uint8_t> (broker_t::SERVER): {
				// Выполняем поиск активного клиента
				auto i = this->_clients.find(bid1);
				// Если активный клиент найден
				if(i != this->_clients.end()){
					// Устанавливаем интервал времени на удаление отключившихся клиентов раз в 3 секунды
					const uint16_t tid = this->_timer.timeout(3000);
					// Выполняем добавление функции обратного вызова
					this->_timer.on(tid, &proxy_socks5_t::erase, this, tid, bid1);
				}
			} break;
		}
	}
}
/**
 * readClientEvents Метод обратного вызова при чтении сообщения
 * @param broker брокер вызвавший событие
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param bid    идентификатор брокера
 * @param sid    идентификатор схемы сети
 */
void awh::server::ProxySocks5::readEvents(const broker_t broker, const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept {
	// Если данные переданы правильно
	if((size > 0) && (bid > 0) && (sid > 0) && (buffer != nullptr)){
		// Определяем тип активного брокера
		switch(static_cast <uint8_t> (broker)){
			// Если брокер является клиентом
			case static_cast <uint8_t> (broker_t::CLIENT): {
				// Получаем параметры активного клиента
				scheme::socks5_t::options_t * options = const_cast <scheme::socks5_t::options_t *> (this->_scheme.get(bid));
				// Если подключение выполнено, отправляем ответ клиенту
				if((options != nullptr) && options->connect){
					// Если функция обратного вызова при получении входящих сообщений установлена
					if(this->_callback.is("message")){
						// Выводим данные полученного сообщения
						if(this->_callback.call <bool (const uint64_t, const event_t, const char *, const size_t)> ("message", bid, event_t::RESPONSE, buffer, size))
							// Отправляем ответ клиенту
							this->_core.send(buffer, size, bid);
					// Отправляем ответ клиенту
					} else this->_core.send(buffer, size, bid);
				}
			} break;
			// Если брокер является сервером
			case static_cast <uint8_t> (broker_t::SERVER): {
				// Получаем параметры активного клиента
				scheme::socks5_t::options_t * options = const_cast <scheme::socks5_t::options_t *> (this->_scheme.get(bid));
				// Если параметры активного клиента получены
				if((options != nullptr) && !options->locked){
					// Если данные не получены
					if(!options->connect && !options->socks5.is(socks5_t::state_t::END)){
						// Выполняем парсинг входящих данных
						options->socks5.parse(buffer, size);
						// Получаем данные запроса
						const auto & buffer = options->socks5.get();
						// Если данные получены
						if(!buffer.empty())
							// Выполняем запись полученных данных на сервер
							this->_core.send(buffer.data(), buffer.size(), bid);
						// Если данные все получены
						else if(options->socks5.is(socks5_t::state_t::END)) {
							// Если рукопожатие выполнено
							if((options->locked = options->socks5.is(socks5_t::state_t::CONNECT))){
								// Получаем данные запрашиваемого сервера
								const auto & server = options->socks5.server();
								// Устанавливаем порт сервера
								options->scheme.url.port = server.port;
								// Устанавливаем хост сервера
								options->scheme.url.host = server.host;
								// Определяем тип передаваемого сервера
								switch(static_cast <uint8_t> (this->_net.host(server.host))){
									// Если хост является доменом или IPv4 адресом
									case static_cast <uint8_t> (net_t::type_t::IPV4):
										// Устанавливаем IP-адрес
										options->scheme.url.ip = server.host;
									break;
									// Если хост является IPv6 адресом, переводим ip адрес в полную форму
									case static_cast <uint8_t> (net_t::type_t::IPV6):
										// Устанавливаем IP-адрес
										options->scheme.url.ip = this->_net = server.host;
									break;
									// Если хост является доменной зоной
									case static_cast <uint8_t> (net_t::type_t::FQDN): {
										// Выполняем очистку IP-адреса
										options->scheme.url.ip.clear();
										// Устанавливаем доменное имя
										options->scheme.url.domain = server.host;
									} break;
								}
								// Выполняем поиск сетевое ядро клиента
								auto i = this->_clients.find(bid);
								// Если сетевое ядро клиента найдено
								if(i != this->_clients.end())
									// Выполняем запрос на сервер
									i->second->open(options->scheme.id);
								// Выходим из функции
								return;
							// Если рукопожатие не выполнено
							} else {
								// Устанавливаем флаг запрещающий подключение
								options->socks5.cmd(awh::socks5_t::rep_t::FORBIDDEN);
								// Получаем данные запроса
								const auto & buffer = options->socks5.get();
								// Если данные получены
								if((options->stopped = !buffer.empty()))
									// Отправляем сообщение клиенту
									this->_core.send(buffer.data(), buffer.size(), bid);
								// Принудительно выполняем отключение лкиента
								else this->close(bid);
							}
						}
					// Если подключение выполнено
					} else {
						// Выполняем поиск сетевое ядро клиента
						auto i = this->_clients.find(bid);
						// Если сетевое ядро клиента найдено
						if(i != this->_clients.end()){
							// Если функция обратного вызова при получении входящих сообщений установлена
							if(this->_callback.is("message")){
								// Выводим данные полученного сообщения
								if(this->_callback.call <bool (const uint64_t, const event_t, const char *, const size_t)> ("message", bid, event_t::REQUEST, buffer, size))
									// Отправляем запрос на внешний сервер
									i->second->send(buffer, size, options->id);
							// Отправляем запрос на внешний сервер
							} else i->second->send(buffer, size, options->id);
						}
					}
				}
			} break;
		}
	}
}
/**
 * writeServerEvents Метод обратного вызова при записи сообщения на клиенте
 * @param broker брокер вызвавший событие
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер записанных в сокет байт
 * @param bid    идентификатор брокера
 * @param sid    идентификатор схемы сети
 */
void awh::server::ProxySocks5::writeEvents(const broker_t broker, const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept {
	// Если данные переданы правильно
	if((size > 0) && (bid > 0) && (sid > 0) && (buffer != nullptr)){
		// Если брокер является сервером
		if(broker == broker_t::SERVER){
			// Получаем параметры активного клиента
			scheme::socks5_t::options_t * options = const_cast <scheme::socks5_t::options_t *> (this->_scheme.get(bid));
			// Если объект брокера получен
			if((options != nullptr) && options->stopped)
				// Выполняем закрытие подключения
				this->close(bid);
		}
	}
}
/**
 * available Метод получения событий освобождения памяти буфера полезной нагрузки
 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
 * @param bid    идентификатор брокера
 * @param size   размер буфера полезной нагрузки
 * @param core   объект сетевого ядра
 */
void awh::server::ProxySocks5::available(const broker_t broker, const uint64_t bid, const size_t size, awh::core_t * core) noexcept {
	// Ещем для указанного потока очередь полезной нагрузки
	auto i = this->_payloads.find(bid);
	// Если для потока очередь полезной нагрузки получена
	if((i != this->_payloads.end()) && !i->second->empty()){
		// Если места достаточно в буфере данных для отправки
		if(i->second->size() <= size){
			// Если сетевое ядро уже инициализированно
			if(core != nullptr){
				// Флаг разрешения отправки ранее неотправленных данных из временного буфера полезной нагрузки
				bool allow = true;
				// Если функция обратного вызова установлена
				if(this->_callback.is("available"))
					// Выполняем функцию обратного вызова
					allow = this->_callback.call <bool (const broker_t, const uint64_t, const size_t)> ("available", broker, bid, size);
				// Если разрешено добавить неотправленную запись во временный буфер полезной нагрузки
				if(allow){
					// Определяем переданного брокера
					switch(static_cast <uint8_t> (broker)){
						// Если брокером является клиент
						case static_cast <uint8_t> (broker_t::CLIENT): {
							// Выполняем отправку заголовков запроса на сервер
							if(dynamic_cast <client::core_t *> (core)->send(reinterpret_cast <const char *> (i->second->get()), i->second->size(), bid))
								// Выполняем удаление буфера полезной нагрузки
								i->second->pop();
						} break;
						// Если брокером является сервер
						case static_cast <uint8_t> (broker_t::SERVER): {
							// Выполняем отправку заголовков запроса на сервер
							if(dynamic_cast <server::core_t *> (core)->send(reinterpret_cast <const char *> (i->second->get()), i->second->size(), bid))
								// Выполняем удаление буфера полезной нагрузки
								i->second->pop();
						} break;
					}
				}
			// Выполняем удаление буфера полезной нагрузки
			} else i->second->pop();
		}
	}
}
/**
 * unavailable Метод получения событий недоступности памяти буфера полезной нагрузки
 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
 * @param bid    идентификатор брокера
 * @param buffer буфер полезной нагрузки которую не получилось отправить
 * @param size   размер буфера полезной нагрузки
 */
void awh::server::ProxySocks5::unavailable(const broker_t broker, const uint64_t bid, const char * buffer, const size_t size) noexcept {
	// Флаг разрешения добавления неотправленных данных во временный буфер полезной нагрузки
	bool allow = true;
	// Если функция обратного вызова установлена
	if(this->_callback.is("unavailable"))
		// Выполняем функцию обратного вызова
		allow = this->_callback.call <bool (const broker_t, const uint64_t, const char *, const size_t)> ("unavailable", broker, bid, buffer, size);
	// Если разрешено добавить неотправленную запись во временный буфер полезной нагрузки
	if(allow){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Ещем для указанного потока очередь полезной нагрузки
			auto i = this->_payloads.find(bid);
			// Если для потока очередь полезной нагрузки получена
			if(i != this->_payloads.end())
				// Добавляем в очередь полезной нагрузки наш буфер полезной нагрузки
				i->second->push(buffer, size);
			// Если для потока почередь полезной нагрузки ещё не сформированна
			else {
				// Создаём новую очередь полезной нагрузки
				auto ret = this->_payloads.emplace(bid, std::make_unique <queue_t> (this->_log));
				// Добавляем в очередь полезной нагрузки наш буфер полезной нагрузки
				ret.first->second->push(buffer, size);
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const bad_alloc &) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (broker), bid, buffer, size), log_t::flag_t::CRITICAL, "Memory allocation error");
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
			#endif
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		}
	}
}
/**
 * erase Метод удаления отключённых клиентов
 * @param tid идентификатор таймера
 * @param bid идентификатор брокера
 */
void awh::server::ProxySocks5::erase([[maybe_unused]] const uint16_t tid, const uint64_t bid) noexcept {
	// Выполняем поиск активного клиента
	auto i = this->_clients.find(bid);
	// Если активный клиент найден
	if(i != this->_clients.end()){
		// Выполняем остановку работы клиента
		i->second->stop();
		// Выполняем отключение клиента от сетевого ядра
		this->_core.unbind(i->second.get());
		// Удаляем активного клиента
		this->_clients.erase(i);
		// Выполняем удаление параметров брокера
		this->_scheme.rm(bid);
		// Если функция обратного вызова при подключении/отключении установлена
		if(this->_callback.is("active"))
			// Выводим функцию обратного вызова
			this->_callback.call <void (const uint64_t, const mode_t)> ("active", bid, mode_t::DISCONNECT);
	}
	// Выполняем поиск неотправленных буферов полезной нагрузки
	auto j = this->_payloads.find(bid);
	// Если неотправленные буферы полезной нагрузки найдены
	if(j != this->_payloads.end())
		// Выполняем удаление неотправленные буферы полезной нагрузки
		this->_payloads.erase(j);
}
/**
 * init Метод инициализации брокера
 * @param socket unix-сокет для биндинга
 */
void awh::server::ProxySocks5::init(const string & socket) noexcept {
	/**
	 * Для операционной системы не являющейся OS Windows
	 */
	#if !_WIN32 && !_WIN64
		// Устанавливаем unix-сокет сервера
		this->_socket = socket;
		// Выполняем установку unix-сокет
		this->_core.sockname(socket);
		// Устанавливаем тип сокета unix-сокет
		this->_core.family(scheme_t::family_t::IPC);
	#endif
}
/**
 * init Метод инициализации брокера
 * @param port порт сервера
 * @param host хост сервера
 */
void awh::server::ProxySocks5::init(const uint32_t port, const string & host) noexcept {
	// Устанавливаем порт сервера
	this->_port = port;
	// Устанавливаем хост сервера
	this->_host = host;
	/**
	 * Для операционной системы не являющейся OS Windows
	 */
	#if !_WIN32 && !_WIN64
		// Удаляем unix-сокет сервера
		this->_socket.clear();
	#endif
}
/**
 * callback Метод установки функций обратного вызова
 * @param callback функции обратного вызова
 */
void awh::server::ProxySocks5::callback(const callback_t & callback) noexcept {
	// Выполняем установку функции обратного вызова на событие запуска или остановки подключения
	this->_callback.set("active", callback);
	// Выполняем установку функции обратного вызова на получения событий запуска и остановки сетевого ядра
	this->_callback.set("status", callback);
	// Выполняем установку функции обратного вызова на событие активации клиента на сервере
	this->_callback.set("accept", callback);
	// Выполняем установку функции обратного вызова на событие получения сообщений в бинарном виде
	this->_callback.set("message", callback);
	// Выполняем установку функции обратного вызова при освобождении буфера хранения полезной нагрузки
	this->_callback.set("available", callback);
	// Выполняем установку функции обратного вызова при заполнении буфера хранения полезной нагрузки
	this->_callback.set("unavailable", callback);
	// Выполняем установку функции обратного вызова для обработки авторизации
	this->_callback.set("checkPassword", callback);
	// Выполняем установку функции обратного вызова для получения события завершения работы процесса
	this->_callback.set("clusterExit", callback);
	// Выполняем установку функции обратного вызова для получения события подключения дочерних процессов
	this->_callback.set("clusterReady", callback);
	// Выполняем установку функции обратного вызова для получения события пересоздании процесса
	this->_callback.set("clusterRebase", callback);
	// Выполняем установку функции обратного вызова для получения события ЗАПУСКА/ОСТАНОВКИ кластера
	this->_callback.set("clusterEvents", callback);
	// Выполняем установку функции обратного вызова для получения сообщений от дочерних процессоров кластера
	this->_callback.set("clusterMessage", callback);
}
/**
 * port Метод получения порта подключения брокера
 * @param bid идентификатор брокера
 * @return    порт подключения брокера
 */
uint32_t awh::server::ProxySocks5::port(const uint64_t bid) const noexcept {
	// Выводим результат
	return this->_scheme.port(bid);
}
/**
 * ip Метод получения IP-адреса брокера
 * @param bid идентификатор брокера
 * @return    адрес интернет подключения брокера
 */
const string & awh::server::ProxySocks5::ip(const uint64_t bid) const noexcept {
	// Выводим результат
	return this->_scheme.ip(bid);
}
/**
 * mac Метод получения MAC-адреса брокера
 * @param bid идентификатор брокера
 * @return    адрес устройства брокера
 */
const string & awh::server::ProxySocks5::mac(const uint64_t bid) const noexcept {
	// Выводим результат
	return this->_scheme.mac(bid);
}
/**
 * port Метод получения порта сервера
 * @param sid идентификатор схемы сети
 * @return    порт подключения сервера
 */
uint32_t awh::server::ProxySocks5::port(const uint16_t sid) const noexcept {
	// Выполняем получение порта сервера
	return this->_core.port(sid);
}
/**
 * host Метод получения хоста сервера
 * @param sid идентификатор схемы сети
 * @return    адрес интернет подключения сервера
 */
const string & awh::server::ProxySocks5::host(const uint16_t sid) const noexcept {
	// Выполняем получение хост сервера
	return this->_core.host(sid);
}
/**
 * stop Метод остановки сервера
 */
void awh::server::ProxySocks5::stop() noexcept {
	// Если подключение выполнено
	if(this->_core.working()){
		// Запрещаем перехват сигналов
		this->_core.signalInterception(awh::scheme_t::mode_t::DISABLED);
		// Завершаем работу, если разрешено остановить
		this->_core.stop();
	}
}
/**
 * start Метод запуска сервера
 */
void awh::server::ProxySocks5::start() noexcept {
	// Если биндинг не запущен, выполняем запуск биндинга
	if(!this->_core.working()){
		// Разрешаем перехват сигналов
		this->_core.signalInterception(awh::scheme_t::mode_t::ENABLED);
		// Устанавливаем функцию обработки сигналов завершения работы приложения
		this->_core.on <void (const int32_t)> ("crash", &server::proxy_socks5_t::crash, this, _1);
		// Если функция обратного вызова для получения события подключения дочерних процессов
		if(this->_callback.is("clusterReady"))
			// Выполняем установку функции обратного вызова для получения события подключения дочерних процессов
			this->_core.on <void (const uint16_t, const pid_t)> ("clusterReady", this->_callback.get <void (const uint16_t, const pid_t)> ("clusterReady"), _1, _2);
		// Если функция обратного вызова для получения события завершения работы процесса установлена
		if(this->_callback.is("clusterExit"))
			// Выполняем установку функции обратного вызова для получения события завершения работы процесса
			this->_core.on <void (const uint16_t, const pid_t, const int32_t)> ("clusterExit", this->_callback.get <void (const uint16_t, const pid_t, const int32_t)> ("clusterExit"), _1, _2, _3);
		// Если функция обратного вызова для получения события пересоздании процесса
		if(this->_callback.is("clusterRebase"))
			// Выполняем установку функции обратного вызова для получения события пересоздании процесса
			this->_core.on <void (const uint16_t, const pid_t, const pid_t)> ("clusterRebase", this->_callback.get <void (const uint16_t, const pid_t, const pid_t)> ("clusterRebase"), _1, _2, _3);
		// Если функция обратного вызова для получения события ЗАПУСКА/ОСТАНОВКИ кластера
		if(this->_callback.is("clusterEvents"))
			// Выполняем установку функции обратного вызова для получения события ЗАПУСКА/ОСТАНОВКИ кластера
			this->_core.on <void (const cluster_t::family_t, const uint16_t, const pid_t, const cluster_t::event_t)> ("clusterEvents", this->_callback.get <void (const cluster_t::family_t, const uint16_t, const pid_t, const cluster_t::event_t)> ("clusterEvents"), _1, _2, _3, _4);
		// Если функция обратного вызова для получения сообщений от дочерних процессоров кластера
		if(this->_callback.is("clusterMessage"))
			// Выполняем установку функции обратного вызова для получения сообщений от дочерних процессоров кластера
			this->_core.on <void (const cluster_t::family_t, const uint16_t, const pid_t, const char *, const size_t)> ("clusterMessage", this->_callback.get <void (const cluster_t::family_t, const uint16_t, const pid_t, const char *, const size_t)> ("clusterMessage"), _1, _2, _3, _4, _5);
		// Выполняем запуск биндинга
		this->_core.start();
	}
}
/**
 * bind Метод подключения модуля ядра к текущей базе событий
 * @param core модуль ядра для подключения
 */
void awh::server::ProxySocks5::bind(awh::core_t * core) noexcept {
	// Выполняем подключение модуля ядра
	this->_core.bind(core);
}
/**
 * unbind Метод отключения модуля ядра от текущей базы событий
 * @param core модуль ядра для отключения
 */
void awh::server::ProxySocks5::unbind(awh::core_t * core) noexcept {
	// Выполняем отключение модуля ядра
	this->_core.unbind(core);
}
/**
 * close Метод закрытия подключения
 * @param bid идентификатор брокера
 */
void awh::server::ProxySocks5::close(const uint64_t bid) noexcept {
	// Отключаем клиента от сервера
	this->_core.close(bid);
}
/**
 * memoryAvailableSize Метод получения максимального рамзера памяти для хранения полезной нагрузки всех брокеров
 * @return размер памяти для хранения полезной нагрузки всех брокеров
 */
size_t awh::server::ProxySocks5::memoryAvailableSize() const noexcept {
	// Выводим размер памяти для хранения полезной нагрузки всех брокеров
	return this->_core.memoryAvailableSize();
}
/**
 * memoryAvailableSize Метод установки максимального рамзера памяти для хранения полезной нагрузки всех брокеров
 * @param size размер памяти для хранения полезной нагрузки всех брокеров
 */
void awh::server::ProxySocks5::memoryAvailableSize(const size_t size) noexcept {
	// Запоминаем размер памяти для хранения полезной нагрузки всех брокеров
	this->_memoryAvailableSize = size;
	// Выполняем установку размера памяти для хранения полезной нагрузки всех брокеров
	this->_core.memoryAvailableSize(size);
}
/**
 * brokerAvailableSize Метод получения максимального размера хранимой полезной нагрузки для одного брокера
 * @return размер хранимой полезной нагрузки для одного брокера
 */
size_t awh::server::ProxySocks5::brokerAvailableSize() const noexcept {
	// Выводим размер хранимой полезной нагрузки для одного брокера
	return this->_core.brokerAvailableSize();
}
/**
 * brokerAvailableSize Метод установки максимального размера хранимой полезной нагрузки для одного брокера
 * @param size размер хранимой полезной нагрузки для одного брокера
 */
void awh::server::ProxySocks5::brokerAvailableSize(const size_t size) noexcept {
	// Запоминаем размер хранимой полезной нагрузки для одного брокера
	this->_brokerAvailableSize = size;
	// Выполняем установку размера хранимой полезной нагрузки для одного брокера
	this->_core.brokerAvailableSize(static_cast <size_t> (size));
}
/**
 * waitMessage Метод ожидания входящих сообщений
 * @param sec интервал времени в секундах
 */
void awh::server::ProxySocks5::waitMessage(const uint16_t sec) noexcept {
	// Выполняем установку времени ожидания входящих сообщений
	this->_scheme.timeouts.wait = sec;
}
/**
 * waitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read  количество секунд для детекции по чтению
 * @param write количество секунд для детекции по записи
 */
void awh::server::ProxySocks5::waitTimeDetect(const uint16_t read, const uint16_t write) noexcept {
	// Устанавливаем количество секунд на чтение
	this->_scheme.timeouts.read = read;
	// Устанавливаем количество секунд на запись
	this->_scheme.timeouts.write = write;
}
/**
 * total Метод установки максимального количества одновременных подключений
 * @param total максимальное количество одновременных подключений
 */
void awh::server::ProxySocks5::total(const uint16_t total) noexcept {
	// Устанавливаем максимальное количество одновременных подключений
	this->_core.total(this->_scheme.id, total);
}
/**
 * clusterAutoRestart Метод установки флага перезапуска процессов
 * @param mode флаг перезапуска процессов
 */
void awh::server::ProxySocks5::clusterAutoRestart(const bool mode) noexcept {
	// Выполняем установку флага автоматического перезапуска
	this->_core.clusterAutoRestart(mode);
}
/**
 * cluster Метод установки количества процессов кластера
 * @param mode флаг активации/деактивации кластера
 * @param size количество рабочих процессов
 */
void awh::server::ProxySocks5::cluster(const awh::scheme_t::mode_t mode, const uint16_t size) noexcept {
	// Устанавливаем количество процессов кластера
	this->_core.cluster(mode, size);
}
/**
 * mode Метод установки флагов модуля
 * @param flags список флагов модуля для установки
 */
void awh::server::ProxySocks5::mode(const std::set <flag_t> & flags) noexcept {
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	this->_core.verbose(flags.find(flag_t::NOT_INFO) == flags.end());
}
/**
 * ipV6only Метод установки флага использования только сети IPv6
 * @param mode флаг для установки
 */
void awh::server::ProxySocks5::ipV6only(const bool mode) noexcept {
	// Устанавливаем количество процессов кластера
	this->_core.ipV6only(mode);
}
/**
 * sonet Метод установки типа сокета подключения
 * @param sonet тип сокета подключения (TCP / UDP / SCTP)
 */
void awh::server::ProxySocks5::sonet(const scheme_t::sonet_t sonet) noexcept {
	// Устанавливаем тип сокета подключения
	this->_core.sonet(sonet);
}
/**
 * family Метод установки типа протокола интернета
 * @param family тип протокола интернета (IPV4 / IPV6 / IPC)
 */
void awh::server::ProxySocks5::family(const scheme_t::family_t family) noexcept {
	// Устанавливаем тип протокола интернета
	this->_core.family(family);
}
/**
 * keepAlive Метод установки жизни подключения
 * @param cnt   максимальное количество попыток
 * @param idle  интервал времени в секундах через которое происходит проверка подключения
 * @param intvl интервал времени в секундах между попытками
 */
void awh::server::ProxySocks5::keepAlive(const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept {
	// Выполняем установку максимального количества попыток
	this->_scheme.keepAlive.cnt = cnt;
	// Выполняем установку интервала времени в секундах через которое происходит проверка подключения
	this->_scheme.keepAlive.idle = idle;
	// Выполняем установку интервала времени в секундах между попытками
	this->_scheme.keepAlive.intvl = intvl;
}
/**
 * bandwidth Метод установки пропускной способности сети
 * @param bid   идентификатор брокера
 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
 */
void awh::server::ProxySocks5::bandwidth(const uint64_t bid, const string & read, const string & write) noexcept {
	// Устанавливаем пропускную способность сети
	this->_core.bandwidth(bid, read, write);
}
/**
 * network Метод установки параметров сети
 * @param ips    список IP-адресов компьютера с которых разрешено выходить в интернет
 * @param ns     список серверов имён, через которые необходимо производить резолвинг доменов
 * @param family тип протокола интернета (IPV4 / IPV6 / IPC)
 * @param sonet  тип сокета подключения (TCP / UDP)
 */
void awh::server::ProxySocks5::network(const vector <string> & ips, const vector <string> & ns, const scheme_t::family_t family, const scheme_t::sonet_t sonet) noexcept {
	// Устанавливаем DNS адреса клиента
	this->_dns.servers(ns);
	// Устанавливаем параметры сети сервера
	this->_core.network(ips, family, sonet);
	// Устанавливаем список IP-адресов компьютера с которых разрешено выходить в интернет
	this->_settings.ips.assign(ips.begin(), ips.end());
}
/**
 * signalInterception Метод активации перехвата сигналов
 * @param mode флаг активации
 */
void awh::server::ProxySocks5::signalInterception(const scheme_t::mode_t mode) noexcept {
	// Выполняем активацию перехвата сигналов
	this->_core.signalInterception(mode);
}
/**
 * ssl Метод установки параметров SSL-шифрования
 * @param ssl объект параметров SSL-шифрования
 */
void awh::server::ProxySocks5::ssl(const node_t::ssl_t & ssl) noexcept {
	// Выполняем установку параметров SSL-шифрования
	this->_core.ssl(ssl);
	// Запоминаем параметры SSL-шифрования
	this->_settings.ssl = ssl;
	// Если адрес файла сертификата и ключа передан
	if(!ssl.cert.empty() && !ssl.key.empty()){
		// Устанавливаем тип сокета TLS
		this->_core.sonet(awh::scheme_t::sonet_t::TLS);
		// Устанавливаем активный протокол подключения
		this->_core.proto(awh::engine_t::proto_t::HTTP2);
	}
}
/**
 * ProxySocks5 Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::server::ProxySocks5::ProxySocks5(const fmk_t * fmk, const log_t * log) noexcept :
 _port(SERVER_PORT), _host{""}, _socket{""}, _dns(fmk, log), _net(log),
 _callback(log), _core(&_dns, fmk, log), _timer(fmk, log), _scheme(fmk, log),
 _memoryAvailableSize(AWH_WINDOW_SIZE), _brokerAvailableSize(AWH_PAYLOAD_SIZE), _fmk(fmk), _log(log) {
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	this->_timer.verbose(false);
	// Устанавливаем протокол интернет-подключения
	this->_core.sonet(scheme_t::sonet_t::TCP);
	// Активируем правило асинхронной работы передачи данных
	this->_core.transferRule(server::core_t::transfer_t::ASYNC);
	// Устанавливаем функцию получения события запуска сервера
	this->_core.on <void (const string &, const uint32_t)> ("launched", &proxy_socks5_t::launchedEvents, this, _1, _2);
	// Устанавливаем функцию обратного вызова на получение событий очистки буферов полезной нагрузки
	this->_core.on <void (const uint64_t, const size_t)> ("available", &proxy_socks5_t::available, this, broker_t::SERVER, _1, _2, &this->_core);
	// Устанавливаем функцию обратного вызова на получение событий очистки буферов полезной нагрузки
	this->_core.on <void (const uint64_t, const char *, const size_t)> ("unavailable", &proxy_socks5_t::unavailable, this, broker_t::SERVER, _1, _2, _3);
	// Устанавливаем событие на запуск системы
	this->_core.on <void (const uint16_t)> ("open", &proxy_socks5_t::openEvents, this, _1);
	// Устанавливаем событие подключения
	this->_core.on <void (const uint64_t, const uint16_t)> ("connect", &proxy_socks5_t::connectEvents, this, broker_t::SERVER, _1, 0, _2);
	// Устанавливаем событие отключения
	this->_core.on <void (const uint64_t, const uint16_t)> ("disconnect", &proxy_socks5_t::disconnectEvents, this, broker_t::SERVER, _1, 0, _2);
	// Добавляем событие аццепта брокера
	this->_core.on <bool (const string &, const string &, const uint32_t, const uint16_t)> ("accept", &proxy_socks5_t::acceptEvents, this, _1, _2, _3, _4);
	// Устанавливаем функцию чтения данных
	this->_core.on <void (const char *, const size_t, const uint64_t, const uint16_t)> ("read", &proxy_socks5_t::readEvents, this, broker_t::SERVER, _1, _2, _3, _4);
	// Устанавливаем функцию записи данных
	this->_core.on <void (const char *, const size_t, const uint64_t, const uint16_t)> ("write", &proxy_socks5_t::writeEvents, this, broker_t::SERVER, _1, _2, _3, _4);
	// Добавляем схему сети в сетевое ядро
	this->_core.scheme(&this->_scheme);
	// Разрешаем автоматический перезапуск упавших процессов
	this->_core.clusterAutoRestart(true);
	// Выполняем биндинг сетевого ядра таймера
	this->_core.bind(dynamic_cast <awh::core_t *> (&this->_timer));
}
/**
 * ~ProxySocks5 Деструктор
 */
awh::server::ProxySocks5::~ProxySocks5() noexcept {
	// Если подключение выполнено
	if(this->_core.working()){
		// Запрещаем перехват сигналов
		this->_core.signalInterception(awh::scheme_t::mode_t::DISABLED);
		// Завершаем работу, если разрешено остановить
		this->_core.stop();
	}
}
