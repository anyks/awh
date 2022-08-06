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
 * @param ctx  передаваемый контекст модуля
 */
void awh::server::ProxySocks5::runCallback(const bool mode, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxySocks5_t * proxy = reinterpret_cast <proxySocks5_t *> (ctx);
		// Выполняем биндинг базы событий для клиента
		if(mode) proxy->_core.server.bind(reinterpret_cast <awh::core_t *> (&proxy->_core.client));
		// Выполняем анбиндинг базы событий клиента
		else proxy->_core.server.unbind(reinterpret_cast <awh::core_t *> (&proxy->_core.client));
	}
}
/**
 * openServerCallback Функция обратного вызова при запуске работы
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::server::ProxySocks5::openServerCallback(const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxySocks5_t * proxy = reinterpret_cast <proxySocks5_t *> (ctx);
		// Устанавливаем хост сервера
		reinterpret_cast <server::core_t *> (core)->init(wid, proxy->_port, proxy->_host);
		// Выполняем запуск сервера
		reinterpret_cast <server::core_t *> (core)->run(wid);
	}
}
/**
 * persistServerCallback Функция персистентного вызова
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::server::ProxySocks5::persistServerCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxySocks5_t * proxy = reinterpret_cast <proxySocks5_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workerSocks5_t::adjp_t * adj = const_cast <workerSocks5_t::adjp_t *> (proxy->_worker.getAdj(aid));
		// Если параметры подключения адъютанта получены
		if((adj != nullptr) && adj->close) proxy->close(aid);
	}
}
/**
 * connectClientCallback Функция обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::server::ProxySocks5::connectClientCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxySocks5_t * proxy = reinterpret_cast <proxySocks5_t *> (ctx);
		// Ищем идентификатор адъютанта пары
		auto it = proxy->_worker.pairs.find(wid);
		// Если адъютант получен
		if(it != proxy->_worker.pairs.end()){
			// Получаем параметры подключения адъютанта
			workerSocks5_t::adjp_t * adj = const_cast <workerSocks5_t::adjp_t *> (proxy->_worker.getAdj(it->second));
			// Если подключение не выполнено
			if(!adj->connect){
				// Разрешаем обработки данных
				adj->locked = false;
				// Запоминаем, что подключение выполнено
				adj->connect = true;
				// Устанавливаем флаг разрешающий подключение
				adj->socks5.resCmd(awh::socks5_t::rep_t::SUCCESS);
				// Получаем данные запроса
				const auto & socks5 = adj->socks5.get();
				// Если данные получены
				if(!socks5.empty()) reinterpret_cast <awh::core_t *> (&proxy->_core.server)->write(socks5.data(), socks5.size(), it->second);
			}
		}
	}
}
/**
 * connectServerCallback Функция обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::server::ProxySocks5::connectServerCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxySocks5_t * proxy = reinterpret_cast <proxySocks5_t *> (ctx);
		// Создаём адъютанта
		proxy->_worker.createAdj(aid);
		// Получаем параметры подключения адъютанта
		workerSocks5_t::adjp_t * adj = const_cast <workerSocks5_t::adjp_t *> (proxy->_worker.getAdj(aid));
		// Устанавливаем контекст сообщения
		adj->_worker.ctx = proxy;
		// Устанавливаем флаг ожидания входящих сообщений
		adj->_worker.wait = proxy->_worker.wait;
		// Устанавливаем количество секунд на чтение
		adj->_worker.timeRead = proxy->_worker.timeRead;
		// Устанавливаем количество секунд на запись
		adj->_worker.timeWrite = proxy->_worker.timeWrite;
		// Устанавливаем функцию чтения данных
		adj->_worker.readFn = readClientCallback;
		// Устанавливаем событие подключения
		adj->_worker.connectFn = connectClientCallback;
		// Устанавливаем событие отключения
		adj->_worker.disconnectFn = disconnectClientCallback;
		// Добавляем воркер в биндер TCP/IP
		proxy->_core.client.add(&adj->_worker);
		// Создаём пару клиента и сервера
		proxy->_worker.pairs.emplace(adj->_worker.wid, aid);
		// Устанавливаем функцию проверки авторизации
		adj->socks5.setAuthCallback(proxy->ctx.at(2), proxy->_checkAuthFn);
		// Определяем тип хоста сервера
		switch((uint8_t) proxy->_worker.nwk.parseHost(proxy->_host)){
			// Если хост является адресом IPv4
			case (uint8_t) network_t::type_t::IPV4: {
				// Устанавливаем тип сети
				adj->_worker.url.family = AF_INET;
				// Устанавливаем хост сервера
				adj->_worker.url.ip = proxy->_host;
			} break;
			// Если хост является адресом IPv6
			case (uint8_t) network_t::type_t::IPV6: {
				// Устанавливаем тип сети
				adj->_worker.url.family = AF_INET6;
				// Устанавливаем хост сервера
				adj->_worker.url.ip = proxy->_host;
			} break;
			// Если хост является доменным именем
			case (uint8_t) network_t::type_t::DOMNAME:
				// Устанавливаем хост сервера
				adj->_worker.url.domain = proxy->_host;
			break;
		}
		// Устанавливаем порт сервера
		adj->_worker.url.port = proxy->_port;
		// Устанавливаем URL адрес запроса
		adj->socks5.setUrl(adj->_worker.url);
		// Если функция обратного вызова установлена, выполняем
		if(proxy->_activeFn != nullptr) proxy->_activeFn(aid, mode_t::CONNECT, proxy, proxy->ctx.at(0));
	}
}
/**
 * disconnectClientCallback Функция обратного вызова при отключении от сервера
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::server::ProxySocks5::disconnectClientCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxySocks5_t * proxy = reinterpret_cast <proxySocks5_t *> (ctx);
		// Ищем идентификатор адъютанта пары
		auto it = proxy->_worker.pairs.find(wid);
		// Если адъютант получен
		if(it != proxy->_worker.pairs.end()){
			// Получаем идентификатор адъютанта
			const size_t aid = it->second;
			// Удаляем пару клиента и сервера
			proxy->_worker.pairs.erase(it);
			// Получаем параметры подключения адъютанта
			workerSocks5_t::adjp_t * adj = const_cast <workerSocks5_t::adjp_t *> (proxy->_worker.getAdj(aid));
			// Если подключение не выполнено, отправляем ответ клиенту
			if(!adj->connect){
				// Устанавливаем флаг запрещающий подключение
				adj->socks5.resCmd(awh::socks5_t::rep_t::DENIED);
				// Получаем данные запроса
				const auto & socks5 = adj->socks5.get();
				// Если данные получены
				if(!socks5.empty()){
					// Получаем размер сообщения
					adj->stopBytes = socks5.size();
					// Отправляем сообщение клиенту
					reinterpret_cast <awh::core_t *> (&proxy->_core.server)->write(socks5.data(), socks5.size(), aid);
				// Устанавливаем флаг отключения клиента
				} else adj->close = true;
			// Устанавливаем флаг отключения клиента
			} else adj->close = true;
		}
	}
}
/**
 * disconnectServerCallback Функция обратного вызова при отключении от сервера
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::server::ProxySocks5::disconnectServerCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxySocks5_t * proxy = reinterpret_cast <proxySocks5_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workerSocks5_t::adjp_t * adj = const_cast <workerSocks5_t::adjp_t *> (proxy->_worker.getAdj(aid));
		// Выполняем отключение клиента от стороннего сервера
		if(adj != nullptr) adj->close = true;
	}
}
/**
 * acceptServerCallback Функция обратного вызова при проверке подключения клиента
 * @param ip   адрес интернет подключения клиента
 * @param mac  мак-адрес подключившегося клиента
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 * @return     результат разрешения к подключению клиента
 */
bool awh::server::ProxySocks5::acceptServerCallback(const string & ip, const string & mac, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Результат работы функции
	bool result = true;
	// Если данные существуют
	if(!ip.empty() && !mac.empty() && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxySocks5_t * proxy = reinterpret_cast <proxySocks5_t *> (ctx);
		// Если функция обратного вызова установлена, проверяем
		if(proxy->_acceptFn != nullptr) result = proxy->_acceptFn(ip, mac, proxy, proxy->ctx.at(3));
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
 * @param ctx    передаваемый контекст модуля
 */
void awh::server::ProxySocks5::readClientCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((size > 0) && (aid > 0) && (wid > 0) && (buffer != nullptr) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxySocks5_t * proxy = reinterpret_cast <proxySocks5_t *> (ctx);
		// Ищем идентификатор адъютанта пары
		auto it = proxy->_worker.pairs.find(wid);
		// Если адъютант получен
		if(it != proxy->_worker.pairs.end()){
			// Получаем параметры подключения адъютанта
			workerSocks5_t::adjp_t * adj = const_cast <workerSocks5_t::adjp_t *> (proxy->_worker.getAdj(it->second));
			// Если подключение выполнено, отправляем ответ клиенту
			if(adj->connect){
				// Если функция обратного вызова установлена, выполняем
				if(proxy->_binaryFn != nullptr){
					// Выводим сообщение
					if(proxy->_binaryFn(it->second, event_t::RESPONSE, buffer, size, proxy, proxy->ctx.at(1)))
						// Отправляем ответ клиенту
						reinterpret_cast <awh::core_t *> (&proxy->_core.server)->write(buffer, size, it->second);
				// Отправляем ответ клиенту
				} else reinterpret_cast <awh::core_t *> (&proxy->_core.server)->write(buffer, size, it->second);
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
 * @param ctx    передаваемый контекст модуля
 */
void awh::server::ProxySocks5::readServerCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((size > 0) && (aid > 0) && (wid > 0) && (buffer != nullptr) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxySocks5_t * proxy = reinterpret_cast <proxySocks5_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workerSocks5_t::adjp_t * adj = const_cast <workerSocks5_t::adjp_t *> (proxy->_worker.getAdj(aid));
		// Если параметры подключения адъютанта получены
		if((adj != nullptr) && !adj->locked){
			// Если данные не получены
			if(!adj->connect && !adj->socks5.isEnd()){
				// Выполняем парсинг входящих данных
				adj->socks5.parse(buffer, size);
				// Получаем данные запроса
				const auto & socks5 = adj->socks5.get();
				// Если данные получены
				if(!socks5.empty()) core->write(socks5.data(), socks5.size(), aid);
				// Если данные все получены
				else if(adj->socks5.isEnd()) {
					// Если рукопожатие выполнено
					if((adj->locked = adj->socks5.isConnected())){
						// Получаем данные запрашиваемого сервера
						const auto & server = adj->socks5.getServer();
						// Формируем адрес подключения
						adj->_worker.url = proxy->_worker.uri.parseUrl(proxy->fmk->format("http://%s:%u", server.host.c_str(), server.port));
						// Выполняем запрос на сервер
						proxy->_core.client.open(adj->_worker.wid);
						// Выходим из функции
						return;
					// Если рукопожатие не выполнено
					} else {
						// Устанавливаем флаг запрещающий подключение
						adj->socks5.resCmd(awh::socks5_t::rep_t::FORBIDDEN);
						// Получаем данные запроса
						const auto & socks5 = adj->socks5.get();
						// Если данные получены
						if(!socks5.empty()){
							// Получаем размер сообщения
							adj->stopBytes = socks5.size();
							// Отправляем сообщение клиенту
							core->write(socks5.data(), socks5.size(), aid);
						// Устанавливаем флаг отключения клиента
						} else adj->close = true;
					}
				}
			// Если подключение выполнено
			} else {
				// Получаем идентификатор адъютанта
				const size_t aid = adj->_worker.getAid();
				// Отправляем запрос на внешний сервер
				if(aid > 0){
					// Если функция обратного вызова установлена, выполняем
					if(proxy->_binaryFn != nullptr){
						// Выводим сообщение
						if(proxy->_binaryFn(aid, event_t::REQUEST, buffer, size, proxy, proxy->ctx.at(1)))
							// Отправляем запрос на внешний сервер
							reinterpret_cast <awh::core_t *> (&proxy->_core.client)->write(buffer, size, aid);
					// Отправляем запрос на внешний сервер
					} else reinterpret_cast <awh::core_t *> (&proxy->_core.client)->write(buffer, size, aid);
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
 * @param ctx    передаваемый контекст модуля
 */
void awh::server::ProxySocks5::writeServerCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((size > 0) && (aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxySocks5_t * proxy = reinterpret_cast <proxySocks5_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workerSocks5_t::adjp_t * adj = const_cast <workerSocks5_t::adjp_t *> (proxy->_worker.getAdj(aid));
		// Если параметры подключения адъютанта получены
		if((adj != nullptr) && (adj->stopBytes > 0)){
			// Запоминаем количество прочитанных байт
			adj->readBytes += size;
			// Если размер полученных байт соответствует
			adj->close = (adj->stopBytes >= adj->readBytes);
		}
	}
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
}
/**
 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
 * @param callback функция обратного вызова
 */
void awh::server::ProxySocks5::on(function <void (const size_t, const mode_t, ProxySocks5 *)> callback) noexcept {
	// Устанавливаем функцию запуска и остановки
	this->_activeFn = callback;
}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений в бинарном виде
 * @param callback функция обратного вызова
 */
void awh::server::ProxySocks5::on(function <bool (const size_t, const event_t, const char *, const size_t, ProxySocks5 *)> callback) noexcept {
	// Устанавливаем функцию получения сообщений в бинарном виде с сервера
	this->_binaryFn = callback;
}
/**
 * on Метод добавления функции обработки авторизации
 * @param callback функция обратного вызова для обработки авторизации
 */
void awh::server::ProxySocks5::on(function <bool (const string &, const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова для обработки авторизации
	this->_checkAuthFn = callback;
}
/**
 * on Метод установки функции обратного вызова на событие активации клиента на сервере
 * @param callback функция обратного вызова
 */
void awh::server::ProxySocks5::on(function <bool (const string &, const string &, ProxySocks5 *)> callback) noexcept {
	// Устанавливаем функцию запуска и остановки
	this->_acceptFn = callback;
}
/**
 * ip Метод получения IP адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес интернет подключения адъютанта
 */
const string & awh::server::ProxySocks5::ip(const size_t aid) const noexcept {
	// Выводим результат
	return this->_worker.ip(aid);
}
/**
 * mac Метод получения MAC адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес устройства адъютанта
 */
const string & awh::server::ProxySocks5::mac(const size_t aid) const noexcept {
	// Выводим результат
	return this->_worker.mac(aid);
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
	workerSocks5_t::adjp_t * adj = const_cast <workerSocks5_t::adjp_t *> (this->_worker.getAdj(aid));
	// Если параметры подключения адъютанта получены, устанавливаем флаг закрытия подключения
	if(adj != nullptr){
		// Выполняем отключение всех дочерних клиентов
		reinterpret_cast <awh::core_t *> (&this->_core.client)->close(adj->_worker.getAid());
		// Выполняем удаление параметров адъютанта
		this->_worker.removeAdj(aid);
	}
	// Отключаем клиента от сервера
	reinterpret_cast <awh::core_t *> (&this->_core.server)->close(aid);
	// Если функция обратного вызова установлена, выполняем
	if(this->_activeFn != nullptr) this->_activeFn(aid, mode_t::DISCONNECT, this, this->ctx.at(0));
}
/**
 * setMode Метод установки флага модуля
 * @param flag флаг модуля для установки
 */
void awh::server::ProxySocks5::setMode(const u_short flag) noexcept {
	// Устанавливаем флаг ожидания входящих сообщений
	this->_worker.wait = (flag & (uint8_t) flag_t::WAITMESS);
	// Устанавливаем флаг отложенных вызовов событий сокета
	this->_core.client.setDefer(flag & (uint8_t) flag_t::DEFER);
	this->_core.server.setDefer(flag & (uint8_t) flag_t::DEFER);
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	this->_core.client.setNoInfo(flag & (uint8_t) flag_t::NOINFO);
	this->_core.server.setNoInfo(flag & (uint8_t) flag_t::NOINFO);
}
/**
 * setTotal Метод установки максимального количества одновременных подключений
 * @param total максимальное количество одновременных подключений
 */
void awh::server::ProxySocks5::setTotal(const u_short total) noexcept {
	// Устанавливаем максимальное количество одновременных подключений
	this->_core.server.setTotal(this->_worker.wid, total);
}
/**
 * setWaitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read  количество секунд для детекции по чтению
 * @param write количество секунд для детекции по записи
 */
void awh::server::ProxySocks5::setWaitTimeDetect(const time_t read, const time_t write) noexcept {
	// Устанавливаем количество секунд на чтение
	this->_worker.timeRead = read;
	// Устанавливаем количество секунд на запись
	this->_worker.timeWrite = write;
}
/**
 * setBytesDetect Метод детекции сообщений по количеству байт
 * @param read  количество байт для детекции по чтению
 * @param write количество байт для детекции по записи
 */
void awh::server::ProxySocks5::setBytesDetect(const worker_t::mark_t read, const worker_t::mark_t write) noexcept {
	// Устанавливаем количество байт на чтение
	this->_worker.markRead = read;
	// Устанавливаем количество байт на запись
	this->_worker.markWrite = write;
}
/**
 * ProxySocks5 Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::server::ProxySocks5::ProxySocks5(const fmk_t * fmk, const log_t * log) noexcept : core(fmk, log), worker(fmk, log), fmk(fmk), log(log) {
	// Устанавливаем контекст сообщения
	this->_worker.ctx = this;
	// Устанавливаем событие на запуск системы
	this->_worker.openFn = openServerCallback;
	// Устанавливаем функцию чтения данных
	this->_worker.readFn = readServerCallback;
	// Устанавливаем функцию записи данных
	this->_worker.writeFn = writeServerCallback;
	// Добавляем событие аццепта клиента
	this->_worker.acceptFn = acceptServerCallback;
	// Устанавливаем функцию персистентного вызова
	this->_worker.persistFn = persistServerCallback;
	// Устанавливаем событие подключения
	this->_worker.connectFn = connectServerCallback;
	// Устанавливаем событие отключения
	this->_worker.disconnectFn = disconnectServerCallback;
	// Активируем персистентный запуск для работы пингов
	this->_core.server.setPersist(true);
	// Добавляем воркер в биндер TCP/IP
	this->_core.server.add(&this->_worker);
	// Устанавливаем функцию активации ядра сервера
	this->_core.server.setCallback(this, &runCallback);
	// Устанавливаем интервал персистентного таймера для работы пингов
	this->_core.server.setPersistInterval(KEEPALIVE_TIMEOUT / 2);
}
