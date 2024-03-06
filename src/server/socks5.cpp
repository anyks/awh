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
 * @copyright: Copyright © 2022
 */

// Подключаем заголовочный файл
#include <server/socks5.hpp>

/**
 * openEvents Метод обратного вызова при запуске работы
 * @param sid идентификатор схемы сети
 */
void awh::server::ProxySocks5::openEvents(const uint16_t sid) noexcept {
	// Если данные существуют
	if(sid > 0){
		// Устанавливаем хост сервера
		this->_core.init(sid, this->_port, this->_host);
		// Выполняем запуск сервера
		this->_core.run(sid);
	}
}
/**
 * statusEvents Метод обратного вызова при активации ядра
 * @param status флаг запуска/остановки
 */
void awh::server::ProxySocks5::statusEvents(const awh::core_t::status_t status) noexcept {
	// Определяем статус активности сетевого ядра
	switch(static_cast <uint8_t> (status)){
		// Если система запущена
		case static_cast <uint8_t> (awh::core_t::status_t::START): {
			// Выполняем биндинг базы событий для таймера
			this->_core.bind(dynamic_cast <awh::core_t *> (&this->_timer));
			// Устанавливаем интервал времени на удаление отключившихся клиентов раз в 3 секунды
			this->_timer.setInterval(3000, std::bind(&proxy_socks5_t::erase, this, _1));
		} break;
		// Если система остановлена
		case static_cast <uint8_t> (awh::core_t::status_t::STOP):
			// Выполняем анбиндинг базы событий таймера
			this->_core.unbind(dynamic_cast <awh::core_t *> (&this->_timer));
		break;
	}
	// Если функция получения событий запуска и остановки сетевого ядра установлена
	if(this->_callbacks.is("status"))
		// Выводим функцию обратного вызова
		this->_callbacks.call <void (const awh::core_t::status_t)> ("status", status);
}
/**
 * acceptEvents Метод обратного вызова при проверке подключения клиента
 * @param ip   адрес интернет подключения клиента
 * @param mac  мак-адрес подключившегося клиента
 * @param port порт подключившегося брокера
 * @param sid  идентификатор схемы сети
 * @return     результат разрешения к подключению клиента
 */
bool awh::server::ProxySocks5::acceptEvents(const string & ip, const string & mac, const u_int port, const uint16_t sid) noexcept {
	// Если данные существуют
	if(!ip.empty() && !mac.empty() && (sid > 0)){
		// Если функция обратного вызова установлена
		if(this->_callbacks.is("accept"))
			// Выводим функцию обратного вызова
			return this->_callbacks.call <bool (const string &, const string &, const u_int)> ("accept", ip, mac, port);
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
						this->_core.write(buffer.data(), buffer.size(), bid1);
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
					// Устанавливаем флаг ожидания входящих сообщений
					options->scheme.wait = this->_scheme.wait;
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
					// Устанавливаем событие подключения
					options->scheme.callbacks.set <void (const uint64_t, const uint16_t)> ("connect", std::bind(&proxy_socks5_t::connectEvents, this, broker_t::CLIENT, bid1, _1, _2));
					// Устанавливаем событие отключения
					options->scheme.callbacks.set <void (const uint64_t, const uint16_t)> ("disconnect", std::bind(&proxy_socks5_t::disconnectEvents, this, broker_t::CLIENT, bid1, _1, _2));
					// Устанавливаем функцию чтения данных
					options->scheme.callbacks.set <void (const char *, const size_t, const uint64_t, const uint16_t)> ("read", std::bind(&proxy_socks5_t::readEvents, this, broker_t::CLIENT, _1, _2, bid1, _4));
					// Если функция обратного вызова при выполнении авторизации установлена
					if(this->_callbacks.is("checkPassword"))
						// Устанавливаем функцию проверки авторизации
						options->socks5.authCallback(std::bind(this->_callbacks.get <bool (const uint64_t, const string &, const string &)> ("checkPassword"), bid1, _1, _2));
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
							case static_cast <uint8_t> (net_t::type_t::ZONE):
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
					if(this->_callbacks.is("active"))
						// Выводим функцию обратного вызова
						this->_callbacks.call <void (const uint64_t, const mode_t)> ("active", bid1, mode_t::CONNECT);
					// Выполняем создание нового подключённого клиента
					auto ret = this->_clients.emplace(bid1, unique_ptr <client::core_t> (new client::core_t(&this->_dns, this->_fmk, this->_log)));
					// Выполняем отключение информационных сообщений сетевого ядра клиента
					ret.first->second->noInfo(true);
					// Добавляем схему сети в сетевое ядро
					ret.first->second->add(&options->scheme);
					// Устанавливаем параметры сети клиента
					ret.first->second->network(this->_settings.ips);
					// Разрешаем проверку сертификата для клиента
					ret.first->second->verifySSL(this->_settings.verify);
					// Устанавливаем доверенный сертификат
					ret.first->second->ca(this->_settings.cacert.trusted, this->_settings.cacert.path);
					// Выполняем подключение клиента к сетевому ядру
					this->_core.bind(ret.first->second.get());
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
						this->_core.write(buffer.data(), buffer.size(), bid1);
				}
				// Выполняем закрытие подключения
				this->close(bid1);
			} break;
			// Если брокер является сервером
			case static_cast <uint8_t> (broker_t::SERVER): {
				// Выполняем поиск активного клиента
				auto it = this->_clients.find(bid1);
				// Если активный клиент найден
				if(it != this->_clients.end())
					// Выполняем отключение клиента от сетевого ядра
					this->_core.unbind(it->second.get());
				// Добавляем в очередь список отключившихся клиентов
				this->_disconnected.emplace(bid1, this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS));
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
					if(this->_callbacks.is("message")){
						// Выводим данные полученного сообщения
						if(this->_callbacks.call <bool (const uint64_t, const event_t, const char *, const size_t)> ("message", bid, event_t::RESPONSE, buffer, size))
							// Отправляем ответ клиенту
							this->_core.write(buffer, size, bid);
					// Отправляем ответ клиенту
					} else this->_core.write(buffer, size, bid);
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
							this->_core.write(buffer.data(), buffer.size(), bid);
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
									case static_cast <uint8_t> (net_t::type_t::IPV6): {
										// Создаём объкт для работы с адресами
										net_t net{};
										// Устанавливаем IP-адрес
										options->scheme.url.ip = net = server.host;
									} break;
									// Если хост является доменной зоной
									case static_cast <uint8_t> (net_t::type_t::ZONE): {
										// Выполняем очистку IP-адреса
										options->scheme.url.ip.clear();
										// Устанавливаем доменное имя
										options->scheme.url.domain = server.host;
									} break;
								}
								// Выполняем поиск сетевое ядро клиента
								auto it = this->_clients.find(bid);
								// Если сетевое ядро клиента найдено
								if(it != this->_clients.end())
									// Выполняем запрос на сервер
									it->second->open(options->scheme.sid);
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
									this->_core.write(buffer.data(), buffer.size(), bid);
								// Принудительно выполняем отключение лкиента
								else this->close(bid);
							}
						}
					// Если подключение выполнено
					} else {
						// Выполняем поиск сетевое ядро клиента
						auto it = this->_clients.find(bid);
						// Если сетевое ядро клиента найдено
						if(it != this->_clients.end()){
							// Если функция обратного вызова при получении входящих сообщений установлена
							if(this->_callbacks.is("message")){
								// Выводим данные полученного сообщения
								if(this->_callbacks.call <bool (const uint64_t, const event_t, const char *, const size_t)> ("message", bid, event_t::REQUEST, buffer, size))
									// Отправляем запрос на внешний сервер
									it->second->write(buffer, size, options->id);
							// Отправляем запрос на внешний сервер
							} else it->second->write(buffer, size, options->id);
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
 * erase Метод удаления отключённых клиентов
 * @param tid идентификатор таймера
 */
void awh::server::ProxySocks5::erase(const uint16_t tid) noexcept {
	// Если список отключившихся клиентов не пустой
	if(!this->_disconnected.empty()){
		// Получаем текущее значение времени
		const time_t date = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
		// Выполняем переход по всему списку отключившихся клиентов
		for(auto it = this->_disconnected.begin(); it != this->_disconnected.end();){
			// Если брокер уже давно удалился
			if((date - it->second) >= 5000){
				// Удаляем активного клиента
				this->_clients.erase(it->first);
				// Выполняем удаление параметров брокера
				this->_scheme.rm(it->first);
				// Если функция обратного вызова при подключении/отключении установлена
				if(this->_callbacks.is("active"))
					// Выводим функцию обратного вызова
					this->_callbacks.call <void (const uint64_t, const mode_t)> ("active", it->first, mode_t::DISCONNECT);
				// Выполняем удаление объекта брокеров из списка мусора
				it = this->_disconnected.erase(it);
			// Выполняем пропуск брокера
			} else ++it;
		}
	}
}
/**
 * init Метод инициализации брокера
 * @param socket unix-сокет для биндинга
 */
void awh::server::ProxySocks5::init(const string & socket) noexcept {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Устанавливаем unix-сокет сервера
		this->_socket = socket;
		// Выполняем установку unix-сокет
		this->_core.unixSocket(socket);
		// Устанавливаем тип сокета unix-сокет
		this->_core.family(scheme_t::family_t::NIX);
	#endif
}
/**
 * init Метод инициализации брокера
 * @param port порт сервера
 * @param host хост сервера
 */
void awh::server::ProxySocks5::init(const u_int port, const string & host) noexcept {
	// Устанавливаем порт сервера
	this->_port = port;
	// Устанавливаем хост сервера
	this->_host = host;
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Удаляем unix-сокет сервера
		this->_socket.clear();
		// Удаляем unix-сокет ранее установленный
		this->_core.removeUnixSocket();
	#endif
}
/**
 * callbacks Метод установки функций обратного вызова
 * @param callbacks функции обратного вызова
 */
void awh::server::ProxySocks5::callbacks(const fn_t & callbacks) noexcept {
	// Выполняем установку функции обратного вызова на событие запуска или остановки подключения
	this->_callbacks.set("active", callbacks);
	// Выполняем установку функции обратного вызова на получения событий запуска и остановки сетевого ядра
	this->_callbacks.set("status", callbacks);
	// Выполняем установку функции обратного вызова на событие активации клиента на сервере
	this->_callbacks.set("accept", callbacks);
	// Выполняем установку функции обратного вызова на событие получения сообщений в бинарном виде
	this->_callbacks.set("message", callbacks);
	// Выполняем установку функции обратного вызова для обработки авторизации
	this->_callbacks.set("checkPassword", callbacks);
}
/**
 * port Метод получения порта подключения брокера
 * @param bid идентификатор брокера
 * @return    порт подключения брокера
 */
u_int awh::server::ProxySocks5::port(const uint64_t bid) const noexcept {
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
 * stop Метод остановки сервера
 */
void awh::server::ProxySocks5::stop() noexcept {
	// Если подключение выполнено
	if(this->_core.working())
		// Завершаем работу, если разрешено остановить
		this->_core.stop();
}
/**
 * start Метод запуска сервера
 */
void awh::server::ProxySocks5::start() noexcept {
	// Если биндинг не запущен, выполняем запуск биндинга
	if(!this->_core.working())
		// Выполняем запуск биндинга
		this->_core.start();
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
 * waitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read  количество секунд для детекции по чтению
 * @param write количество секунд для детекции по записи
 */
void awh::server::ProxySocks5::waitTimeDetect(const time_t read, const time_t write) noexcept {
	// Устанавливаем количество секунд на чтение
	this->_scheme.timeouts.read = read;
	// Устанавливаем количество секунд на запись
	this->_scheme.timeouts.write = write;
}
/**
 * bytesDetect Метод детекции сообщений по количеству байт
 * @param read  количество байт для детекции по чтению
 * @param write количество байт для детекции по записи
 */
void awh::server::ProxySocks5::bytesDetect(const scheme_t::mark_t read, const scheme_t::mark_t write) noexcept {
	// Устанавливаем количество байт на чтение
	this->_scheme.marker.read = read;
	// Устанавливаем количество байт на запись
	this->_scheme.marker.write = write;
	// Если минимальный размер данных для чтения, не установлен
	if(this->_scheme.marker.read.min == 0)
		// Устанавливаем размер минимальных для чтения данных по умолчанию
		this->_scheme.marker.read.min = BUFFER_READ_MIN;
	// Если максимальный размер данных для записи не установлен, устанавливаем по умолчанию
	if(this->_scheme.marker.write.max == 0)
		// Устанавливаем размер максимальных записываемых данных по умолчанию
		this->_scheme.marker.write.max = BUFFER_WRITE_MAX;
}
/**
 * total Метод установки максимального количества одновременных подключений
 * @param total максимальное количество одновременных подключений
 */
void awh::server::ProxySocks5::total(const u_short total) noexcept {
	// Устанавливаем максимальное количество одновременных подключений
	this->_core.total(this->_scheme.sid, total);
}
/**
 * clusterAutoRestart Метод установки флага перезапуска процессов
 * @param mode флаг перезапуска процессов
 */
void awh::server::ProxySocks5::clusterAutoRestart(const bool mode) noexcept {
	// Выполняем установку флага автоматического перезапуска
	this->_core.clusterAutoRestart(this->_scheme.sid, mode);
}
/**
 * cluster Метод установки количества процессов кластера
 * @param size количество рабочих процессов
 */
void awh::server::ProxySocks5::cluster(const uint16_t size) noexcept {
	// Устанавливаем количество процессов кластера
	this->_core.cluster(size);
}
/**
 * mode Метод установки флагов модуля
 * @param flags список флагов модуля для установки
 */
void awh::server::ProxySocks5::mode(const set <flag_t> & flags) noexcept {
	// Устанавливаем флаг ожидания входящих сообщений
	this->_scheme.wait = (flags.count(flag_t::WAIT_MESS) > 0);
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	this->_core.noInfo(flags.count(flag_t::NOT_INFO) > 0);
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
 * keepAlive Метод установки жизни подключения
 * @param cnt   максимальное количество попыток
 * @param idle  интервал времени в секундах через которое происходит проверка подключения
 * @param intvl интервал времени в секундах между попытками
 */
void awh::server::ProxySocks5::keepAlive(const int cnt, const int idle, const int intvl) noexcept {
	// Выполняем установку максимального количества попыток
	this->_scheme.keepAlive.cnt = cnt;
	// Выполняем установку интервала времени в секундах через которое происходит проверка подключения
	this->_scheme.keepAlive.idle = idle;
	// Выполняем установку интервала времени в секундах между попытками
	this->_scheme.keepAlive.intvl = intvl;
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
 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
 */
void awh::server::ProxySocks5::family(const scheme_t::family_t family) noexcept {
	// Устанавливаем тип протокола интернета
	this->_core.family(family);
}
/**
 * bandWidth Метод установки пропускной способности сети
 * @param bid   идентификатор брокера
 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
 */
void awh::server::ProxySocks5::bandWidth(const uint64_t bid, const string & read, const string & write) noexcept {
	// Устанавливаем пропускную способность сети
	this->_core.bandWidth(bid, read, write);
}
/**
 * network Метод установки параметров сети
 * @param ips    список IP-адресов компьютера с которых разрешено выходить в интернет
 * @param ns     список серверов имён, через которые необходимо производить резолвинг доменов
 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
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
void awh::server::ProxySocks5::signalInterception(const awh::core_t::mode_t mode) noexcept {
	// Выполняем активацию перехвата сигналов
	this->_core.signalInterception(mode);
}
/**
 * verifySSL Метод разрешающий или запрещающий, выполнять проверку соответствия, сертификата домену
 * @param mode флаг состояния разрешения проверки
 */
void awh::server::ProxySocks5::verifySSL(const bool mode) noexcept {
	// Разрешаем проверку сертификата для сервера
	this->_core.verifySSL(mode);
	// Выполняем установку флага разрешающего выполнять валидацию доменного имени
	this->_settings.verify = mode;
}
/**
 * ca Метод установки доверенного сертификата (CA-файла)
 * @param trusted адрес доверенного сертификата (CA-файла)
 * @param path    адрес каталога где находится сертификат (CA-файл)
 */
void awh::server::ProxySocks5::ca(const string & trusted, const string & path) noexcept {
	// Устанавливаем путь до каталога сертификатов
	this->_settings.cacert.path = path;
	// Устанавливаем путь до файла сертификата
	this->_settings.cacert.trusted = trusted;
}
/**
 * certificate Метод установки файлов сертификата
 * @param chain файл цепочки сертификатов
 * @param key   приватный ключ сертификата
 */
void awh::server::ProxySocks5::certificate(const string & chain, const string & key) noexcept {
	// Устанавливаем установки файлов сертификата
	this->_core.certificate(chain, key);
}
/**
 * ProxySocks5 Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::server::ProxySocks5::ProxySocks5(const fmk_t * fmk, const log_t * log) noexcept :
 _port(SERVER_PORT), _host{""}, _socket{""}, _dns(fmk, log), _callbacks(log),
 _core(&_dns, fmk, log), _timer(fmk, log), _scheme(fmk, log), _fmk(fmk), _log(log) {
	// Выполняем отключение информационных сообщений сетевого ядра таймера
	this->_timer.noInfo(true);
	// Устанавливаем протокол интернет-подключения
	this->_core.sonet(scheme_t::sonet_t::TCP);
	// Устанавливаем функцию активации ядра сервера
	this->_core.callback <void (const awh::core_t::status_t)> ("status", std::bind(&proxy_socks5_t::statusEvents, this, _1));
	// Устанавливаем событие на запуск системы
	this->_scheme.callbacks.set <void (const uint16_t)> ("open", std::bind(&proxy_socks5_t::openEvents, this, _1));
	// Устанавливаем событие подключения
	this->_scheme.callbacks.set <void (const uint64_t, const uint16_t)> ("connect", std::bind(&proxy_socks5_t::connectEvents, this, broker_t::SERVER, _1, 0, _2));
	// Устанавливаем событие отключения
	this->_scheme.callbacks.set <void (const uint64_t, const uint16_t)> ("disconnect", std::bind(&proxy_socks5_t::disconnectEvents, this, broker_t::SERVER, _1, 0, _2));
	// Добавляем событие аццепта брокера
	this->_scheme.callbacks.set <bool (const string &, const string &, const u_int, const uint16_t)> ("accept", std::bind(&proxy_socks5_t::acceptEvents, this, _1, _2, _3, _4));
	// Устанавливаем функцию чтения данных
	this->_scheme.callbacks.set <void (const char *, const size_t, const uint64_t, const uint16_t)> ("read", std::bind(&proxy_socks5_t::readEvents, this, broker_t::SERVER, _1, _2, _3, _4));
	// Устанавливаем функцию записи данных
	this->_scheme.callbacks.set <void (const char *, const size_t, const uint64_t, const uint16_t)> ("write", std::bind(&proxy_socks5_t::writeEvents, this, broker_t::SERVER, _1, _2, _3, _4));
	// Добавляем схему сети в сетевое ядро
	this->_core.add(&this->_scheme);
	// Разрешаем автоматический перезапуск упавших процессов
	this->_core.clusterAutoRestart(this->_scheme.sid, true);
}
