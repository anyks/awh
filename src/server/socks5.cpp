/**
 * @file: socks5.cpp
 * @date: 2021-12-19
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2021
 */

// Подключаем заголовочный файл
#include <server/socks5.hpp>

/**
 * runCallback Функция обратного вызова при активации ядра сервера
 * @param mode флаг запуска/остановки
 * @param core объект биндинга TCP/IP
 */
void awh::server::ProxySocks5::runCallback(const bool mode, awh::core_t * core) noexcept {
	// Если данные существуют
	if(core != nullptr){
		// Выполняем биндинг базы событий для клиента
		if(mode) this->_core.server.bind(reinterpret_cast <awh::core_t *> (&this->_core.client));
		// Выполняем анбиндинг базы событий клиента
		else this->_core.server.unbind(reinterpret_cast <awh::core_t *> (&this->_core.client));
	}
}
/**
 * openServerCallback Функция обратного вызова при запуске работы
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 */
void awh::server::ProxySocks5::openServerCallback(const size_t wid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((wid > 0) && (core != nullptr)){
		// Устанавливаем хост сервера
		reinterpret_cast <server::core_t *> (core)->init(wid, this->_port, this->_host);
		// Выполняем запуск сервера
		reinterpret_cast <server::core_t *> (core)->run(wid);
	}
}
/**
 * connectClientCallback Функция обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 */
void awh::server::ProxySocks5::connectClientCallback(const size_t aid, const size_t wid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((aid > 0) && (wid > 0) && (core != nullptr)){
		// Ищем идентификатор адъютанта пары
		auto it = this->_worker.pairs.find(wid);
		// Если адъютант получен
		if(it != this->_worker.pairs.end()){
			// Получаем параметры подключения адъютанта
			socks5_worker_t::coffer_t * adj = const_cast <socks5_worker_t::coffer_t *> (this->_worker.get(it->second));
			// Если подключение не выполнено
			if((adj != nullptr) && !adj->connect){
				// Разрешаем обработки данных
				adj->locked = false;
				// Запоминаем, что подключение выполнено
				adj->connect = true;
				// Устанавливаем флаг разрешающий подключение
				adj->socks5.resCmd(socks5_t::rep_t::SUCCESS);
				// Получаем данные запроса
				const auto & socks5 = adj->socks5.get();
				// Если данные получены
				if(!socks5.empty()) reinterpret_cast <awh::core_t *> (&this->_core.server)->write(socks5.data(), socks5.size(), it->second);
			}
		}
	}
}
/**
 * connectServerCallback Функция обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 */
void awh::server::ProxySocks5::connectServerCallback(const size_t aid, const size_t wid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((aid > 0) && (wid > 0) && (core != nullptr)){
		// Создаём адъютанта
		this->_worker.set(aid);
		// Получаем параметры подключения адъютанта
		socks5_worker_t::coffer_t * adj = const_cast <socks5_worker_t::coffer_t *> (this->_worker.get(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Устанавливаем флаг ожидания входящих сообщений
			adj->worker.wait = this->_worker.wait;
			// Устанавливаем количество секунд на чтение
			adj->worker.timeouts.read = this->_worker.timeouts.read;
			// Устанавливаем количество секунд на запись
			adj->worker.timeouts.write = this->_worker.timeouts.write;
			// Выполняем установку максимального количества попыток
			adj->worker.keepAlive.cnt = this->_worker.keepAlive.cnt;
			// Выполняем установку интервала времени в секундах через которое происходит проверка подключения
			adj->worker.keepAlive.idle = this->_worker.keepAlive.idle;
			// Выполняем установку интервала времени в секундах между попытками
			adj->worker.keepAlive.intvl = this->_worker.keepAlive.intvl;
			// Устанавливаем функцию чтения данных
			adj->worker.callback.read = std::bind(&ProxySocks5::readClientCallback, this, _1, _2, _3, _4, _5);
			// Устанавливаем событие подключения
			adj->worker.callback.connect = std::bind(&ProxySocks5::connectClientCallback, this, _1, _2, _3);
			// Устанавливаем событие отключения
			adj->worker.callback.disconnect = std::bind(&ProxySocks5::disconnectClientCallback, this, _1, _2, _3);
			// Добавляем воркер в сетевое ядро
			this->_core.client.add(&adj->worker);
			// Создаём пару клиента и сервера
			this->_worker.pairs.emplace(adj->worker.wid, aid);
			// Устанавливаем функцию проверки авторизации
			adj->socks5.authCallback(this->_callback.checkAuth);
			// Если unix-сокет установлен
			if(!this->_usock.empty()){
				// Устанавливаем хост сервера
				adj->worker.url.host = this->_usock;
				// Устанавливаем тип сети
				adj->worker.url.family = AF_UNIX;
			// Если сервер слушает порт
			} else {
				// Определяем тип хоста сервера
				switch((uint8_t) this->_worker.nwk.parseHost(this->_host)){
					// Если хост является адресом IPv4
					case (uint8_t) network_t::type_t::IPV4: {
						// Устанавливаем хост сервера
						adj->worker.url.ip = this->_host;
						// Устанавливаем тип сети
						adj->worker.url.family = AF_INET;
					} break;
					// Если хост является адресом IPv6
					case (uint8_t) network_t::type_t::IPV6: {
						// Устанавливаем хост сервера
						adj->worker.url.ip = this->_host;
						// Устанавливаем тип сети
						adj->worker.url.family = AF_INET6;
					} break;
					// Если хост является доменным именем
					case (uint8_t) network_t::type_t::DOMNAME:
						// Устанавливаем хост сервера
						adj->worker.url.domain = this->_host;
					break;
				}
				// Устанавливаем порт сервера
				adj->worker.url.port = this->_port;
			}
			// Устанавливаем URL адрес запроса
			adj->socks5.url(adj->worker.url);
			// Если функция обратного вызова установлена
			if(this->_callback.active != nullptr)
				// Выполняем функцию обратного вызова
				this->_callback.active(aid, mode_t::CONNECT, this);
		}
	}
}
/**
 * disconnectClientCallback Функция обратного вызова при отключении от сервера
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 */
void awh::server::ProxySocks5::disconnectClientCallback(const size_t aid, const size_t wid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((wid > 0) && (core != nullptr)){
		// Ищем идентификатор адъютанта пары
		auto it = this->_worker.pairs.find(wid);
		// Если адъютант получен
		if(it != this->_worker.pairs.end()){
			// Получаем идентификатор адъютанта
			const size_t aid = it->second;
			// Удаляем пару клиента и сервера
			this->_worker.pairs.erase(it);
			// Получаем параметры подключения адъютанта
			socks5_worker_t::coffer_t * adj = const_cast <socks5_worker_t::coffer_t *> (this->_worker.get(aid));
			// Если подключение не выполнено, отправляем ответ клиенту
			if((adj != nullptr) && !adj->connect){
				// Устанавливаем флаг запрещающий подключение
				adj->socks5.resCmd(socks5_t::rep_t::DENIED);
				// Получаем данные запроса
				const auto & socks5 = adj->socks5.get();
				// Если данные получены
				if((adj->stopped = !socks5.empty()))
					// Отправляем сообщение клиенту
					this->_core.server.write(socks5.data(), socks5.size(), aid);
			}
			// Выполняем отключение клиента
			this->close(aid);
			// Выходим из функции
			return;
		}
		// Выполняем отключение клиента
		this->_core.client.close(aid);
	}
}
/**
 * disconnectServerCallback Функция обратного вызова при отключении от сервера
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 */
void awh::server::ProxySocks5::disconnectServerCallback(const size_t aid, const size_t wid, awh::core_t * core) noexcept {	
	// Принудительно выполняем отключение лкиента
	if(aid > 0) this->close(aid);
}
/**
 * acceptServerCallback Функция обратного вызова при проверке подключения клиента
 * @param ip   адрес интернет подключения клиента
 * @param mac  мак-адрес подключившегося клиента
 * @param port порт подключившегося адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @return     результат разрешения к подключению клиента
 */
bool awh::server::ProxySocks5::acceptServerCallback(const string & ip, const string & mac, const u_int port, const size_t wid, awh::core_t * core) noexcept {
	// Результат работы функции
	bool result = true;
	// Если данные существуют
	if(!ip.empty() && !mac.empty() && (wid > 0) && (core != nullptr)){
		// Если функция обратного вызова установлена
		if(this->_callback.accept != nullptr)
			// Выполняем проверку клиента на разрешение подключения
			result = this->_callback.accept(ip, mac, port, this);
	}
	// Разрешаем подключение клиенту
	return result;
}
/**
 * readClientCallback Функция обратного вызова при чтении сообщения с сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param wid    идентификатор воркера
 * @param core   объект биндинга TCP/IP
 */
void awh::server::ProxySocks5::readClientCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((size > 0) && (aid > 0) && (wid > 0) && (buffer != nullptr) && (core != nullptr)){
		// Ищем идентификатор адъютанта пары
		auto it = this->_worker.pairs.find(wid);
		// Если адъютант получен
		if(it != this->_worker.pairs.end()){
			// Получаем параметры подключения адъютанта
			socks5_worker_t::coffer_t * adj = const_cast <socks5_worker_t::coffer_t *> (this->_worker.get(it->second));
			// Если подключение выполнено, отправляем ответ клиенту
			if((adj != nullptr) && adj->connect){
				// Если функция обратного вызова установлена, выполняем
				if(this->_callback.message != nullptr){
					// Выводим сообщение
					if(this->_callback.message(it->second, event_t::RESPONSE, buffer, size, this))
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
 * @param aid    идентификатор адъютанта
 * @param wid    идентификатор воркера
 * @param core   объект биндинга TCP/IP
 */
void awh::server::ProxySocks5::readServerCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core) noexcept {	
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (aid > 0) && (wid > 0)){
		// Получаем параметры подключения адъютанта
		socks5_worker_t::coffer_t * adj = const_cast <socks5_worker_t::coffer_t *> (this->_worker.get(aid));
		// Если параметры подключения адъютанта получены
		if((adj != nullptr) && !adj->locked){			
			// Если данные не получены
			if(!adj->connect && !adj->socks5.isEnd()){
				// Выполняем парсинг входящих данных
				adj->socks5.parse(buffer, size);
				// Получаем данные запроса
				const auto & socks5 = adj->socks5.get();
				// Если данные получены
				if(!socks5.empty()) this->_core.server.write(socks5.data(), socks5.size(), aid);
				// Если данные все получены
				else if(adj->socks5.isEnd()) {
					// Если рукопожатие выполнено
					if((adj->locked = adj->socks5.isConnected())){
						// Получаем данные запрашиваемого сервера
						const auto & server = adj->socks5.server();
						// Формируем адрес подключения
						adj->worker.url = this->_worker.uri.parse(this->_fmk->format("http://%s:%u", server.host.c_str(), server.port));
						// Выполняем запрос на сервер
						this->_core.client.open(adj->worker.wid);
						// Выходим из функции
						return;
					// Если рукопожатие не выполнено
					} else {
						// Устанавливаем флаг запрещающий подключение
						adj->socks5.resCmd(awh::socks5_t::rep_t::FORBIDDEN);
						// Получаем данные запроса
						const auto & socks5 = adj->socks5.get();
						// Если данные получены
						if((adj->stopped = !socks5.empty()))
							// Отправляем сообщение клиенту
							this->_core.server.write(socks5.data(), socks5.size(), aid);
						// Принудительно выполняем отключение лкиента
						else this->close(aid);
					}
				}
			// Если подключение выполнено
			} else {
				// Получаем идентификатор адъютанта
				const size_t aid = adj->worker.getAid();
				// Отправляем запрос на внешний сервер
				if(aid > 0){
					// Если функция обратного вызова установлена, выполняем
					if(this->_callback.message != nullptr){
						// Выводим сообщение
						if(this->_callback.message(aid, event_t::REQUEST, buffer, size, this))
							// Отправляем запрос на внешний сервер
							this->_core.client.write(buffer, size, aid);
					// Отправляем запрос на внешний сервер
					} else this->_core.client.write(buffer, size, aid);
				}
			}
		}
	}
}
/**
 * writeServerCallback Функция обратного вызова при записи сообщения на клиенте
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер записанных в сокет байт
 * @param aid    идентификатор адъютанта
 * @param wid    идентификатор воркера
 * @param core   объект биндинга TCP/IP
 */
void awh::server::ProxySocks5::writeServerCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((size > 0) && (aid > 0) && (wid > 0) && (core != nullptr)){
		// Получаем параметры подключения адъютанта
		socks5_worker_t::coffer_t * adj = const_cast <socks5_worker_t::coffer_t *> (this->_worker.get(aid));
		// Если объект адъютанта получен
		if((adj != nullptr) && adj->stopped)
			// Выполняем закрытие подключения
			this->close(aid);
	}
}
/**
 * init Метод инициализации WebSocket адъютанта
 * @param socket unix socket для биндинга
 */
void awh::server::ProxySocks5::init(const string & socket) noexcept {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Устанавливаем unix-сокет сервера
		this->_usock = socket;
		// Выполняем установку unix-сокет
		this->_core.server.unixSocket(socket);
		// Устанавливаем тип сокета unix-сокет
		this->_core.server.family(worker_t::family_t::NIX);
	#endif
}
/**
 * init Метод инициализации WebSocket клиента
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
		this->_usock.clear();
		// Удаляем unix-сокет ранее установленный
		this->_core.server.removeUnixSocket();
	#endif
}
/**
 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
 * @param callback функция обратного вызова
 */
void awh::server::ProxySocks5::on(function <void (const size_t, const mode_t, ProxySocks5 *)> callback) noexcept {
	// Устанавливаем функцию запуска и остановки
	this->_callback.active = callback;
}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений в бинарном виде
 * @param callback функция обратного вызова
 */
void awh::server::ProxySocks5::on(function <bool (const size_t, const event_t, const char *, const size_t, ProxySocks5 *)> callback) noexcept {
	// Устанавливаем функцию запуска и остановки
	this->_callback.message = callback;
}
/**
 * on Метод добавления функции обработки авторизации
 * @param callback функция обратного вызова для обработки авторизации
 */
void awh::server::ProxySocks5::on(function <bool (const string &, const string &)> callback) noexcept {
	// Устанавливаем функцию запуска и остановки
	this->_callback.checkAuth = callback;
}
/**
 * on Метод установки функции обратного вызова на событие активации клиента на сервере
 * @param callback функция обратного вызова
 */
void awh::server::ProxySocks5::on(function <bool (const string &, const string &, const u_int, ProxySocks5 *)> callback) noexcept {
	// Устанавливаем функцию запуска и остановки
	this->_callback.accept = callback;
}
/**
 * serverName Метод добавления названия сервера
 * @param name название сервера для добавления
 */
void awh::server::ProxySocks5::serverName(const string & name) noexcept {
	// Устанавливаем названия сервера
	this->_core.server.serverName(name);
}
/**
 * port Метод получения порта подключения адъютанта
 * @param aid идентификатор адъютанта
 * @return    порт подключения адъютанта
 */
u_int awh::server::ProxySocks5::port(const size_t aid) const noexcept {
	// Выводим результат
	return this->_worker.getPort(aid);
}
/**
 * ip Метод получения IP адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес интернет подключения адъютанта
 */
const string & awh::server::ProxySocks5::ip(const size_t aid) const noexcept {
	// Выводим результат
	return this->_worker.getIp(aid);
}
/**
 * mac Метод получения MAC адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес устройства адъютанта
 */
const string & awh::server::ProxySocks5::mac(const size_t aid) const noexcept {
	// Выводим результат
	return this->_worker.getMac(aid);
}
/**
 * start Метод запуска клиента
 */
void awh::server::ProxySocks5::start() noexcept {
	// Если биндинг не запущен, выполняем запуск биндинга
	if(!this->_core.server.working())
		// Выполняем запуск биндинга
		this->_core.server.start();
}
/**
 * stop Метод остановки клиента
 */
void awh::server::ProxySocks5::stop() noexcept {
	// Если подключение выполнено
	if(this->_core.server.working())
		// Завершаем работу, если разрешено остановить
		this->_core.server.stop();
}
/**
 * close Метод закрытия подключения клиента
 * @param aid идентификатор адъютанта
 */
void awh::server::ProxySocks5::close(const size_t aid) noexcept {
	// Получаем параметры подключения адъютанта
	socks5_worker_t::coffer_t * adj = const_cast <socks5_worker_t::coffer_t *> (this->_worker.get(aid));
	// Если параметры подключения адъютанта получены, устанавливаем флаг закрытия подключения
	if(adj != nullptr){
		// Выполняем отключение всех дочерних клиентов
		this->_core.client.close(adj->worker.getAid());
		// Удаляем воркер из сетевого ядра
		this->_core.client.remove(adj->worker.wid);
	}
	// Отключаем клиента от сервера
	this->_core.server.close(aid);
	// Если функция обратного вызова установлена
	if(this->_callback.active != nullptr)
		// Выполняем функцию обратного вызова
		this->_callback.active(aid, mode_t::DISCONNECT, this);
	// Выполняем удаление параметров адъютанта
	if(adj != nullptr) this->_worker.rm(aid);
}
/**
 * waitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read  количество секунд для детекции по чтению
 * @param write количество секунд для детекции по записи
 */
void awh::server::ProxySocks5::waitTimeDetect(const time_t read, const time_t write) noexcept {
	// Устанавливаем количество секунд на чтение
	this->_worker.timeouts.read = read;
	// Устанавливаем количество секунд на запись
	this->_worker.timeouts.write = write;
}
/**
 * bytesDetect Метод детекции сообщений по количеству байт
 * @param read  количество байт для детекции по чтению
 * @param write количество байт для детекции по записи
 */
void awh::server::ProxySocks5::bytesDetect(const worker_t::mark_t read, const worker_t::mark_t write) noexcept {
	// Устанавливаем количество байт на чтение
	this->_worker.marker.read = read;
	// Устанавливаем количество байт на запись
	this->_worker.marker.write = write;
	// Если минимальный размер данных для чтения, не установлен
	if(this->_worker.marker.read.min == 0)
		// Устанавливаем размер минимальных для чтения данных по умолчанию
		this->_worker.marker.read.min = BUFFER_READ_MIN;
	// Если максимальный размер данных для записи не установлен, устанавливаем по умолчанию
	if(this->_worker.marker.write.max == 0)
		// Устанавливаем размер максимальных записываемых данных по умолчанию
		this->_worker.marker.write.max = BUFFER_WRITE_MAX;
}
/**
 * mode Метод установки флага модуля
 * @param flag флаг модуля для установки
 */
void awh::server::ProxySocks5::mode(const u_short flag) noexcept {
	// Устанавливаем флаг ожидания входящих сообщений
	this->_worker.wait = (flag & (uint8_t) flag_t::WAITMESS);
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	this->_core.server.noInfo(flag & (uint8_t) flag_t::NOINFO);
}
/**
 * total Метод установки максимального количества одновременных подключений
 * @param total максимальное количество одновременных подключений
 */
void awh::server::ProxySocks5::total(const u_short total) noexcept {
	// Устанавливаем максимальное количество одновременных подключений
	this->_core.server.total(this->_worker.wid, total);
}
/**
 * clusterSize Метод установки количества процессов кластера
 * @param size количество рабочих процессов
 */
void awh::server::ProxySocks5::clusterSize(const size_t size) noexcept {
	// Устанавливаем количество процессов кластера
	this->_core.server.clusterSize(size);
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
	this->_worker.keepAlive.cnt = cnt;
	// Выполняем установку интервала времени в секундах через которое происходит проверка подключения
	this->_worker.keepAlive.idle = idle;
	// Выполняем установку интервала времени в секундах между попытками
	this->_worker.keepAlive.intvl = intvl;
}
/**
 * sonet Метод установки типа сокета подключения
 * @param sonet тип сокета подключения (TCP / UDP / SCTP)
 */
void awh::server::ProxySocks5::sonet(const worker_t::sonet_t sonet) noexcept {
	// Устанавливаем тип сокета подключения
	this->_core.server.sonet(sonet);
}
/**
 * family Метод установки типа протокола интернета
 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
 */
void awh::server::ProxySocks5::family(const worker_t::family_t family) noexcept {
	// Устанавливаем тип протокола интернета
	this->_core.server.family(family);
}
/**
 * bandWidth Метод установки пропускной способности сети
 * @param aid   идентификатор адъютанта
 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
 */
void awh::server::ProxySocks5::bandWidth(const size_t aid, const string & read, const string & write) noexcept {
	// Устанавливаем пропускную способность сети
	this->_core.server.bandWidth(aid, read, write);
}
/**
 * network Метод установки параметров сети
 * @param ip     список IP адресов компьютера с которых разрешено выходить в интернет
 * @param ns     список серверов имён, через которые необходимо производить резолвинг доменов
 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
 * @param sonet  тип сокета подключения (TCP / UDP)
 */
void awh::server::ProxySocks5::network(const vector <string> & ip, const vector <string> & ns, const worker_t::family_t family, const worker_t::sonet_t sonet) noexcept {
	// Устанавливаем параметры сети клиента
	this->_core.client.network(ip, ns);
	// Устанавливаем параметры сети сервера
	this->_core.server.network(ip, ns, family, sonet);
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
 * ciphers Метод установки алгоритмов шифрования
 * @param ciphers список алгоритмов шифрования для установки
 */
void awh::server::ProxySocks5::ciphers(const vector <string> & ciphers) noexcept {
	// Устанавливаем установки алгоритмов шифрования
	this->_core.server.ciphers(ciphers);
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
awh::server::ProxySocks5::ProxySocks5(const fmk_t * fmk, const log_t * log) noexcept : _port(SERVER_PORT), _host(""), _usock(""), _core(fmk, log), _worker(fmk, log), _fmk(fmk), _log(log) {
	// Устанавливаем флаг запрещающий вывод информационных сообщений для клиента
	this->_core.client.noInfo(true);
	// Устанавливаем протокол интернет-подключения
	this->_core.server.sonet(worker_t::sonet_t::TCP);
	// Устанавливаем событие на запуск системы
	this->_worker.callback.open = std::bind(&ProxySocks5::openServerCallback, this, _1, _2);
	// Устанавливаем событие подключения
	this->_worker.callback.connect = std::bind(&ProxySocks5::connectServerCallback, this, _1, _2, _3);
	// Устанавливаем функцию чтения данных
	this->_worker.callback.read = std::bind(&ProxySocks5::readServerCallback, this, _1, _2, _3, _4, _5);
	// Устанавливаем функцию записи данных
	this->_worker.callback.write = std::bind(&ProxySocks5::writeServerCallback, this, _1, _2, _3, _4, _5);
	// Добавляем событие аццепта адъютанта
	this->_worker.callback.accept = std::bind(&ProxySocks5::acceptServerCallback, this, _1, _2, _3, _4, _5);
	// Устанавливаем событие отключения
	this->_worker.callback.disconnect = std::bind(&ProxySocks5::disconnectServerCallback, this, _1, _2, _3);
	// Добавляем воркер в сетевое ядро
	this->_core.server.add(&this->_worker);
	// Разрешаем автоматический перезапуск упавших процессов
	this->_core.server.clusterAutoRestart(this->_worker.wid, true);
	// Устанавливаем функцию активации ядра сервера
	this->_core.server.callback(std::bind(&ProxySocks5::runCallback, this, _1, _2));
}
