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
 * openServerCallback Функция обратного вызова при запуске работы
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::ProxySocks5::openServerCallback(const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((sid > 0) && (core != nullptr)){
		// Устанавливаем хост сервера
		dynamic_cast <server::core_t *> (core)->init(sid, this->_port, this->_host);
		// Выполняем запуск сервера
		dynamic_cast <server::core_t *> (core)->run(sid);
	}
}
/**
 * eventsCallback Функция обратного вызова при активации ядра сервера
 * @param status флаг запуска/остановки
 * @param core   объект сетевого ядра
 */
void awh::server::ProxySocks5::eventsCallback(const awh::core_t::status_t status, awh::core_t * core) noexcept {
	// Если данные существуют
	if(core != nullptr){
		// Определяем статус активности сетевого ядра
		switch(static_cast <uint8_t> (status)){
			// Если система запущена
			case static_cast <uint8_t> (awh::core_t::status_t::START): {
				// Выполняем биндинг базы событий для таймера
				this->_core.server.bind(dynamic_cast <awh::core_t *> (&this->_core.timer));
				// Выполняем биндинг базы событий для клиента
				this->_core.server.bind(dynamic_cast <awh::core_t *> (&this->_core.client));
				// Устанавливаем интервал времени на удаление отключившихся клиентов раз в 5 секунд
				this->_core.timer.setInterval(5000, std::bind(&proxy_socks5_t::erase, this, _1, _2));
			} break;
			// Если система остановлена
			case static_cast <uint8_t> (awh::core_t::status_t::STOP): {
				// Выполняем анбиндинг базы событий таймера
				this->_core.server.unbind(dynamic_cast <awh::core_t *> (&this->_core.timer));
				// Выполняем анбиндинг базы событий клиента
				this->_core.server.unbind(dynamic_cast <awh::core_t *> (&this->_core.client));
			} break;
		}
		// Если функция получения событий запуска и остановки сетевого ядра установлена
		if(this->_callback.is("events"))
			// Выводим функцию обратного вызова
			this->_callback.call <const awh::core_t::status_t, awh::core_t *> ("events", status, core);
	}
}
/**
 * connectClientCallback Функция обратного вызова при подключении к серверу
 * @param bid  идентификатор брокера
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::ProxySocks5::connectClientCallback(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((bid > 0) && (sid > 0) && (core != nullptr)){
		// Ищем идентификатор брокера пары
		auto it = this->_scheme.pairs.find(sid);
		// Если брокер получен
		if(it != this->_scheme.pairs.end()){
			// Получаем параметры активного клиента
			socks5_scheme_t::options_t * options = const_cast <socks5_scheme_t::options_t *> (this->_scheme.get(it->second));
			// Если подключение не выполнено
			if((options != nullptr) && !options->connect){
				// Разрешаем обработки данных
				options->locked = false;
				// Запоминаем, что подключение выполнено
				options->connect = true;
				// Устанавливаем флаг разрешающий подключение
				options->socks5.cmd(socks5_t::rep_t::SUCCESS);
				// Получаем данные запроса
				const auto & buffer = options->socks5.get();
				// Если данные получены
				if(!buffer.empty())
					// Выполняем запись полученных данных на сервер
					this->_core.server.write(buffer.data(), buffer.size(), it->second);
			}
		}
	}
}
/**
 * connectServerCallback Функция обратного вызова при подключении к серверу
 * @param bid  идентификатор брокера
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::ProxySocks5::connectServerCallback(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((bid > 0) && (sid > 0) && (core != nullptr)){
		// Создаём брокера
		this->_scheme.set(bid);
		// Получаем параметры активного клиента
		socks5_scheme_t::options_t * options = const_cast <socks5_scheme_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Устанавливаем количество секунд на чтение
			options->scheme.timeouts.read = 30;
			// Устанавливаем количество секунд на запись
			options->scheme.timeouts.write = 10;
			// Устанавливаем флаг ожидания входящих сообщений
			options->scheme.wait = this->_scheme.wait;
			// Выполняем установку максимального количества попыток
			options->scheme.keepAlive.cnt = this->_scheme.keepAlive.cnt;
			// Выполняем установку интервала времени в секундах через которое происходит проверка подключения
			options->scheme.keepAlive.idle = this->_scheme.keepAlive.idle;
			// Выполняем установку интервала времени в секундах между попытками
			options->scheme.keepAlive.intvl = this->_scheme.keepAlive.intvl;
			// Устанавливаем событие подключения
			options->scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("connect", std::bind(&proxy_socks5_t::connectClientCallback, this, _1, _2, _3));
			// Устанавливаем событие отключения
			options->scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("disconnect", std::bind(&proxy_socks5_t::disconnectClientCallback, this, _1, _2, _3));
			// Устанавливаем функцию чтения данных
			options->scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("read", std::bind(&proxy_socks5_t::readClientCallback, this, _1, _2, _3, _4, _5));
			// Добавляем схему сети в сетевое ядро
			this->_core.client.add(&options->scheme);
			// Создаём пару клиента и сервера
			this->_scheme.pairs.emplace(options->scheme.sid, bid);
			// Если функция обратного вызова при выполнении авторизации установлена
			if(this->_callback.is("checkPassword"))
				// Устанавливаем функцию проверки авторизации
				options->socks5.authCallback(std::bind(this->_callback.get <bool (const uint64_t, const string &, const string &)> ("checkPassword"), bid, _1, _2));
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
					// Если хост является доменным именем
					case static_cast <uint8_t> (net_t::type_t::DOMN):
						// Устанавливаем хост сервера
						options->scheme.url.domain = this->_host;
					break;
				}
				// Устанавливаем порт сервера
				options->scheme.url.port = this->_port;
			}
			// Устанавливаем URL адрес запроса
			options->socks5.url(options->scheme.url);
			// Если функция обратного вызова при подключении/отключении установлена
			if(this->_callback.is("active"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const mode_t> ("active", bid, mode_t::CONNECT);
		}
	}
}
/**
 * disconnectClientCallback Функция обратного вызова при отключении от сервера
 * @param bid  идентификатор брокера
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::ProxySocks5::disconnectClientCallback(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((sid > 0) && (core != nullptr)){
		// Ищем идентификатор брокера пары
		auto it = this->_scheme.pairs.find(sid);
		// Если брокер получен
		if(it != this->_scheme.pairs.end()){
			// Получаем идентификатор брокера
			const uint64_t bid = it->second;
			// Удаляем пару клиента и сервера
			this->_scheme.pairs.erase(it);
			// Получаем параметры активного клиента
			socks5_scheme_t::options_t * options = const_cast <socks5_scheme_t::options_t *> (this->_scheme.get(bid));
			// Если подключение не выполнено, отправляем ответ клиенту
			if((options != nullptr) && !options->connect){
				// Устанавливаем флаг запрещающий подключение
				options->socks5.cmd(socks5_t::rep_t::DENIED);
				// Получаем данные запроса
				const auto & buffer = options->socks5.get();
				// Если данные получены
				if((options->stopped = !buffer.empty()))
					// Отправляем сообщение клиенту
					this->_core.server.write(buffer.data(), buffer.size(), bid);
			}
			// Выполняем отключение клиента
			this->close(bid);
			// Выходим из функции
			return;
		}
		// Выполняем отключение клиента
		this->_core.client.close(bid);
	}
}
/**
 * disconnectServerCallback Функция обратного вызова при отключении от сервера
 * @param bid  идентификатор брокера
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::ProxySocks5::disconnectServerCallback(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {	
	// Выполняем закрытие подключения
	this->close(bid);
}
/**
 * acceptServerCallback Функция обратного вызова при проверке подключения клиента
 * @param ip   адрес интернет подключения клиента
 * @param mac  мак-адрес подключившегося клиента
 * @param port порт подключившегося брокера
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 * @return     результат разрешения к подключению клиента
 */
bool awh::server::ProxySocks5::acceptServerCallback(const string & ip, const string & mac, const u_int port, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if(!ip.empty() && !mac.empty() && (sid > 0) && (core != nullptr)){
		// Если функция обратного вызова установлена
		if(this->_callback.is("accept"))
			// Выводим функцию обратного вызова
			return this->_callback.apply <bool, const string &, const string &, const u_int> ("accept", ip, mac, port);
	}
	// Разрешаем подключение клиенту
	return false;
}
/**
 * readClientCallback Функция обратного вызова при чтении сообщения с сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param bid    идентификатор брокера
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::server::ProxySocks5::readClientCallback(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((size > 0) && (bid > 0) && (sid > 0) && (buffer != nullptr) && (core != nullptr)){
		// Ищем идентификатор брокера пары
		auto it = this->_scheme.pairs.find(sid);
		// Если брокер получен
		if(it != this->_scheme.pairs.end()){
			// Получаем параметры активного клиента
			socks5_scheme_t::options_t * options = const_cast <socks5_scheme_t::options_t *> (this->_scheme.get(it->second));
			// Если подключение выполнено, отправляем ответ клиенту
			if((options != nullptr) && options->connect){
				// Если функция обратного вызова при получении входящих сообщений установлена
				if(this->_callback.is("message")){
					// Выводим данные полученного сообщения
					if(this->_callback.apply <bool, const uint64_t, const event_t, const char *, const size_t> ("message", it->second, event_t::RESPONSE, buffer, size))
						// Отправляем ответ клиенту
						this->_core.server.write(buffer, size, it->second);
				// Отправляем ответ клиенту
				} else this->_core.server.write(buffer, size, it->second);
			}
		}
	}
}
/**
 * readServerCallback Функция обратного вызова при чтении сообщения с клиента
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param bid    идентификатор брокера
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::server::ProxySocks5::readServerCallback(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (bid > 0) && (sid > 0)){
		// Получаем параметры активного клиента
		socks5_scheme_t::options_t * options = const_cast <socks5_scheme_t::options_t *> (this->_scheme.get(bid));
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
					this->_core.server.write(buffer.data(), buffer.size(), bid);
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
							// Если хост является доменным именем
							case static_cast <uint8_t> (net_t::type_t::DOMN): {
								// Выполняем очистку IP-адреса
								options->scheme.url.ip.clear();
								// Устанавливаем доменное имя
								options->scheme.url.domain = server.host;
							} break;
						}
						// Выполняем запрос на сервер
						this->_core.client.open(options->scheme.sid);
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
							this->_core.server.write(buffer.data(), buffer.size(), bid);
						// Принудительно выполняем отключение лкиента
						else this->close(bid);
					}
				}
			// Если подключение выполнено
			} else {
				// Получаем идентификатор брокера
				const uint64_t bid = options->scheme.bid();
				// Отправляем запрос на внешний сервер
				if(bid > 0){
					// Если функция обратного вызова при получении входящих сообщений установлена
					if(this->_callback.is("message")){
						// Выводим данные полученного сообщения
						if(this->_callback.apply <bool, const uint64_t, const event_t, const char *, const size_t> ("message", bid, event_t::REQUEST, buffer, size))
							// Отправляем запрос на внешний сервер
							this->_core.client.write(buffer, size, bid);
					// Отправляем запрос на внешний сервер
					} else this->_core.client.write(buffer, size, bid);
				}
			}
		}
	}
}
/**
 * writeServerCallback Функция обратного вызова при записи сообщения на клиенте
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер записанных в сокет байт
 * @param bid    идентификатор брокера
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::server::ProxySocks5::writeServerCallback(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((size > 0) && (bid > 0) && (sid > 0) && (core != nullptr)){
		// Получаем параметры активного клиента
		socks5_scheme_t::options_t * options = const_cast <socks5_scheme_t::options_t *> (this->_scheme.get(bid));
		// Если объект брокера получен
		if((options != nullptr) && options->stopped)
			// Выполняем закрытие подключения
			this->close(bid);
	}
}
/**
 * erase Метод удаления отключённых клиентов
 * @param tid  идентификатор таймера
 * @param core объект сетевого ядра
 */
void awh::server::ProxySocks5::erase(const uint16_t tid, awh::core_t * core) noexcept {
	// Если список отключившихся клиентов не пустой
	if(!this->_disconnected.empty()){
		// Получаем текущее значение времени
		const time_t date = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
		// Выполняем переход по всему списку отключившихся клиентов
		for(auto it = this->_disconnected.begin(); it != this->_disconnected.end();){
			// Если брокер уже давно удалился
			if((date - it->second) >= 5000){
				// Выполняем удаление параметров брокера
				this->_scheme.rm(it->first);
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
		this->_core.server.unixSocket(socket);
		// Устанавливаем тип сокета unix-сокет
		this->_core.server.family(scheme_t::family_t::NIX);
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
		this->_core.server.removeUnixSocket();
	#endif
}
/**
 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
 * @param callback функция обратного вызова
 */
void awh::server::ProxySocks5::on(function <void (const uint64_t, const mode_t)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const uint64_t, const mode_t)> ("active", callback);
}
/**
 * on Метод установки функции обратного вызова получения событий запуска и остановки сетевого ядра
 * @param callback функция обратного вызова
 */
void awh::server::ProxySocks5::on(function <void (const awh::core_t::status_t, awh::core_t *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const awh::core_t::status_t, awh::core_t *)> ("events", callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений в бинарном виде
 * @param callback функция обратного вызова
 */
void awh::server::ProxySocks5::on(function <bool (const uint64_t, const event_t, const char *, const size_t)> callback) noexcept {
	// Устанавливаем функцию обратного вызова для получения входящих сообщений
	this->_callback.set <bool (const uint64_t, const event_t, const char *, const size_t)> ("message", callback);
}
/**
 * on Метод установки функции обратного вызова на событие активации клиента на сервере
 * @param callback функция обратного вызова
 */
void awh::server::ProxySocks5::on(function <bool (const string &, const string &, const u_int)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <bool (const string &, const string &, const u_int)> ("accept", callback);
}
/**
 * on Метод установки функции обратного вызова для обработки авторизации
 * @param callback функция обратного вызова
 */
void awh::server::ProxySocks5::on(function <bool (const uint64_t, const string &, const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <bool (const uint64_t, const string &, const string &)> ("checkPassword", callback);
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
	if(this->_core.server.working())
		// Завершаем работу, если разрешено остановить
		this->_core.server.stop();
}
/**
 * start Метод запуска сервера
 */
void awh::server::ProxySocks5::start() noexcept {
	// Если биндинг не запущен, выполняем запуск биндинга
	if(!this->_core.server.working())
		// Выполняем запуск биндинга
		this->_core.server.start();
}
/**
 * close Метод закрытия подключения
 * @param bid идентификатор брокера
 */
void awh::server::ProxySocks5::close(const uint64_t bid) noexcept {
	// Если идентификатор брокера существует
	if(bid > 0){
		// Получаем параметры активного клиента
		socks5_scheme_t::options_t * options = const_cast <socks5_scheme_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены, устанавливаем флаг закрытия подключения
		if(options != nullptr){
			// Выполняем отключение всех дочерних клиентов
			this->_core.client.close(options->scheme.bid());
			// Удаляем схему сети из сетевого ядра
			this->_core.client.remove(options->scheme.sid);
		}
		// Отключаем клиента от сервера
		this->_core.server.close(bid);
		// Добавляем в очередь список отключившихся клиентов
		this->_disconnected.emplace(bid, this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS));
		// Если функция обратного вызова при подключении/отключении установлена
		if(this->_callback.is("active"))
			// Выводим функцию обратного вызова
			this->_callback.call <const uint64_t, const mode_t> ("active", bid, mode_t::DISCONNECT);
	}
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
	this->_core.server.total(this->_scheme.sid, total);
}
/**
 * clusterAutoRestart Метод установки флага перезапуска процессов
 * @param mode флаг перезапуска процессов
 */
void awh::server::ProxySocks5::clusterAutoRestart(const bool mode) noexcept {
	// Выполняем установку флага автоматического перезапуска
	this->_core.server.clusterAutoRestart(this->_scheme.sid, mode);
}
/**
 * clusterSize Метод установки количества процессов кластера
 * @param size количество рабочих процессов
 */
void awh::server::ProxySocks5::clusterSize(const uint16_t size) noexcept {
	// Устанавливаем количество процессов кластера
	this->_core.server.clusterSize(size);
}
/**
 * mode Метод установки флагов модуля
 * @param flags список флагов модуля для установки
 */
void awh::server::ProxySocks5::mode(const set <flag_t> & flags) noexcept {
	// Устанавливаем флаг ожидания входящих сообщений
	this->_scheme.wait = (flags.count(flag_t::WAIT_MESS) > 0);
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	this->_core.server.noInfo(flags.count(flag_t::NOT_INFO) > 0);
}
/**
 * ciphers Метод установки алгоритмов шифрования
 * @param ciphers список алгоритмов шифрования для установки
 */
void awh::server::ProxySocks5::ciphers(const vector <string> & ciphers) noexcept {
	// Устанавливаем установки алгоритмов шифрования
	this->_core.server.ciphers(ciphers);
}
/**
 * ipV6only Метод установки флага использования только сети IPv6
 * @param mode флаг для установки
 */
void awh::server::ProxySocks5::ipV6only(const bool mode) noexcept {
	// Устанавливаем количество процессов кластера
	this->_core.server.ipV6only(mode);
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
	this->_core.server.sonet(sonet);
}
/**
 * family Метод установки типа протокола интернета
 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
 */
void awh::server::ProxySocks5::family(const scheme_t::family_t family) noexcept {
	// Устанавливаем тип протокола интернета
	this->_core.server.family(family);
}
/**
 * bandWidth Метод установки пропускной способности сети
 * @param bid   идентификатор брокера
 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
 */
void awh::server::ProxySocks5::bandWidth(const uint64_t bid, const string & read, const string & write) noexcept {
	// Устанавливаем пропускную способность сети
	this->_core.server.bandWidth(bid, read, write);
}
/**
 * network Метод установки параметров сети
 * @param ips    список IP адресов компьютера с которых разрешено выходить в интернет
 * @param ns     список серверов имён, через которые необходимо производить резолвинг доменов
 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
 * @param sonet  тип сокета подключения (TCP / UDP)
 */
void awh::server::ProxySocks5::network(const vector <string> & ips, const vector <string> & ns, const scheme_t::family_t family, const scheme_t::sonet_t sonet) noexcept {
	// Устанавливаем DNS адреса клиента
	this->_core.client.serversDNS(ns);
	// Устанавливаем DNS адреса сервера
	this->_core.server.serversDNS(ns);
	// Устанавливаем параметры сети клиента
	this->_core.client.network(ips);
	// Устанавливаем параметры сети сервера
	this->_core.server.network(ips, family, sonet);
}
/**
 * signalInterception Метод активации перехвата сигналов
 * @param mode флаг активации
 */
void awh::server::ProxySocks5::signalInterception(const awh::core_t::mode_t mode) noexcept {
	// Выполняем активацию перехвата сигналов
	this->_core.server.signalInterception(mode);
}
/**
 * verifySSL Метод разрешающий или запрещающий, выполнять проверку соответствия, сертификата домену
 * @param mode флаг состояния разрешения проверки
 */
void awh::server::ProxySocks5::verifySSL(const bool mode) noexcept {
	// Разрешаем проверку сертификата для клиента
	this->_core.client.verifySSL(mode);
	// Разрешаем проверку сертификата для сервера
	this->_core.server.verifySSL(mode);
}
/**
 * ca Метод установки доверенного сертификата (CA-файла)
 * @param trusted адрес доверенного сертификата (CA-файла)
 * @param path    адрес каталога где находится сертификат (CA-файл)
 */
void awh::server::ProxySocks5::ca(const string & trusted, const string & path) noexcept {
	// Устанавливаем доверенный сертификат
	this->_core.client.ca(trusted, path);
}
/**
 * certificate Метод установки файлов сертификата
 * @param chain файл цепочки сертификатов
 * @param key   приватный ключ сертификата
 */
void awh::server::ProxySocks5::certificate(const string & chain, const string & key) noexcept {
	// Устанавливаем установки файлов сертификата
	this->_core.server.certificate(chain, key);
}
/**
 * ProxySocks5 Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::server::ProxySocks5::ProxySocks5(const fmk_t * fmk, const log_t * log) noexcept :
 _port(SERVER_PORT), _host{""}, _socket{""}, _core(fmk, log), _callback(log), _scheme(fmk, log), _fmk(fmk), _log(log) {
	// Выполняем отключение информационных сообщений сетевого ядра таймера
	this->_core.timer.noInfo(true);
	// Выполняем отключение информационных сообщений сетевого ядра клиента
	this->_core.client.noInfo(true);
	// Устанавливаем протокол интернет-подключения
	this->_core.server.sonet(scheme_t::sonet_t::TCP);
	// Устанавливаем событие на запуск системы
	this->_scheme.callback.set <void (const uint16_t, awh::core_t *)> ("open", std::bind(&proxy_socks5_t::openServerCallback, this, _1, _2));
	// Устанавливаем событие подключения
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("connect", std::bind(&proxy_socks5_t::connectServerCallback, this, _1, _2, _3));
	// Устанавливаем событие отключения
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("disconnect", std::bind(&proxy_socks5_t::disconnectServerCallback, this, _1, _2, _3));
	// Устанавливаем функцию чтения данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("read", std::bind(&proxy_socks5_t::readServerCallback, this, _1, _2, _3, _4, _5));
	// Устанавливаем функцию записи данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("write", std::bind(&proxy_socks5_t::writeServerCallback, this, _1, _2, _3, _4, _5));
	// Добавляем событие аццепта брокера
	this->_scheme.callback.set <bool (const string &, const string &, const u_int, const uint16_t, awh::core_t *)> ("accept", std::bind(&proxy_socks5_t::acceptServerCallback, this, _1, _2, _3, _4, _5));
	// Добавляем схему сети в сетевое ядро
	this->_core.server.add(&this->_scheme);
	// Разрешаем автоматический перезапуск упавших процессов
	this->_core.server.clusterAutoRestart(this->_scheme.sid, true);
	// Устанавливаем функцию активации ядра сервера
	this->_core.server.on(std::bind(&proxy_socks5_t::eventsCallback, this, _1, _2));
}
