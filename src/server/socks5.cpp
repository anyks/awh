/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <server/socks5.hpp>

/**
 * runCallback Функция обратного вызова при активации ядра сервера
 * @param mode флаг запуска/остановки
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::ProxySocks5Server::runCallback(const bool mode, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxySocks5Srv_t * proxy = reinterpret_cast <proxySocks5Srv_t *> (ctx);
		// Выполняем биндинг базы событий для клиента
		if(mode) proxy->coreSrv.bind(reinterpret_cast <core_t *> (&proxy->coreCli));
		// Выполняем анбиндинг базы событий клиента
		else proxy->coreSrv.unbind(reinterpret_cast <core_t *> (&proxy->coreCli));
	}
}
/**
 * openServerCallback Функция обратного вызова при запуске работы
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::ProxySocks5Server::openServerCallback(const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxySocks5Srv_t * proxy = reinterpret_cast <proxySocks5Srv_t *> (ctx);
		// Устанавливаем хост сервера
		reinterpret_cast <coreSrv_t *> (core)->init(wid, proxy->port, proxy->host);
		// Выполняем запуск сервера
		core->run(wid);
	}
}
/**
 * persistServerCallback Функция персистентного вызова
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::ProxySocks5Server::persistServerCallback(const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxySocks5Srv_t * proxy = reinterpret_cast <proxySocks5Srv_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workSrvSocks5_t::adjp_t * adj = const_cast <workSrvSocks5_t::adjp_t *> (proxy->worker.getAdj(aid));
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
void awh::ProxySocks5Server::connectClientCallback(const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxySocks5Srv_t * proxy = reinterpret_cast <proxySocks5Srv_t *> (ctx);
		// Ищем идентификатор адъютанта пары
		auto it = proxy->worker.pairs.find(wid);
		// Если адъютант получен
		if(it != proxy->worker.pairs.end()){
			// Получаем параметры подключения адъютанта
			workSrvSocks5_t::adjp_t * adj = const_cast <workSrvSocks5_t::adjp_t *> (proxy->worker.getAdj(it->second));
			// Если подключение не выполнено
			if(!adj->connect){
				// Разрешаем обработки данных
				adj->locked = false;
				// Запоминаем, что подключение выполнено
				adj->connect = true;
				// Устанавливаем флаг разрешающий подключение
				adj->socks5.resCmd(socks5_t::rep_t::SUCCESS);
				// Получаем данные запроса
				const auto & socks5 = adj->socks5.get();
				// Если данные получены
				if(!socks5.empty()) reinterpret_cast <core_t *> (&proxy->coreSrv)->write(socks5.data(), socks5.size(), it->second);
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
void awh::ProxySocks5Server::connectServerCallback(const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxySocks5Srv_t * proxy = reinterpret_cast <proxySocks5Srv_t *> (ctx);
		// Создаём адъютанта
		proxy->worker.createAdj(aid);
		// Получаем параметры подключения адъютанта
		workSrvSocks5_t::adjp_t * adj = const_cast <workSrvSocks5_t::adjp_t *> (proxy->worker.getAdj(aid));
		// Устанавливаем контекст сообщения
		adj->worker.ctx = proxy;
		// Устанавливаем флаг ожидания входящих сообщений
		adj->worker.wait = proxy->worker.wait;
		// Устанавливаем количество секунд на чтение
		adj->worker.timeRead = proxy->worker.timeRead;
		// Устанавливаем количество секунд на запись
		adj->worker.timeWrite = proxy->worker.timeWrite;
		// Устанавливаем функцию чтения данных
		adj->worker.readFn = readClientCallback;
		// Устанавливаем событие подключения
		adj->worker.connectFn = connectClientCallback;
		// Устанавливаем событие отключения
		adj->worker.disconnectFn = disconnectClientCallback;
		// Добавляем воркер в биндер TCP/IP
		proxy->coreCli.add(&adj->worker);
		// Создаём пару клиента и сервера
		proxy->worker.pairs.emplace(adj->worker.wid, aid);
		// Устанавливаем функцию проверки авторизации
		adj->socks5.setAuthCallback(proxy->ctx.at(2), proxy->checkAuthFn);
		// Определяем тип хоста сервера
		switch((uint8_t) proxy->worker.nwk.parseHost(proxy->host)){
			// Если хост является адресом IPv4
			case (uint8_t) network_t::type_t::IPV4: {
				// Устанавливаем тип сети
				adj->worker.url.family = AF_INET;
				// Устанавливаем хост сервера
				adj->worker.url.ip = proxy->host;
			} break;
			// Если хост является адресом IPv6
			case (uint8_t) network_t::type_t::IPV6: {
				// Устанавливаем тип сети
				adj->worker.url.family = AF_INET6;
				// Устанавливаем хост сервера
				adj->worker.url.ip = proxy->host;
			} break;
			// Если хост является доменным именем
			case (uint8_t) network_t::type_t::DOMNAME:
				// Устанавливаем хост сервера
				adj->worker.url.domain = proxy->host;
			break;
		}
		// Устанавливаем порт сервера
		adj->worker.url.port = proxy->port;
		// Устанавливаем URL адрес запроса
		adj->socks5.setUrl(adj->worker.url);
		// Если функция обратного вызова установлена, выполняем
		if(proxy->openStopFn != nullptr) proxy->openStopFn(aid, mode_t::CONNECT, proxy, proxy->ctx.at(0));
	}
}
/**
 * disconnectClientCallback Функция обратного вызова при отключении от сервера
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::ProxySocks5Server::disconnectClientCallback(const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxySocks5Srv_t * proxy = reinterpret_cast <proxySocks5Srv_t *> (ctx);
		// Ищем идентификатор адъютанта пары
		auto it = proxy->worker.pairs.find(wid);
		// Если адъютант получен
		if(it != proxy->worker.pairs.end()){
			// Получаем идентификатор адъютанта
			const size_t aid = it->second;
			// Удаляем пару клиента и сервера
			proxy->worker.pairs.erase(it);
			// Получаем параметры подключения адъютанта
			workSrvSocks5_t::adjp_t * adj = const_cast <workSrvSocks5_t::adjp_t *> (proxy->worker.getAdj(aid));
			// Если подключение не выполнено, отправляем ответ клиенту
			if(!adj->connect){
				// Устанавливаем флаг запрещающий подключение
				adj->socks5.resCmd(socks5_t::rep_t::DENIED);
				// Получаем данные запроса
				const auto & socks5 = adj->socks5.get();
				// Если данные получены
				if(!socks5.empty()){
					// Получаем размер сообщения
					adj->stopBytes = socks5.size();
					// Отправляем сообщение клиенту
					reinterpret_cast <core_t *> (&proxy->coreSrv)->write(socks5.data(), socks5.size(), aid);
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
void awh::ProxySocks5Server::disconnectServerCallback(const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxySocks5Srv_t * proxy = reinterpret_cast <proxySocks5Srv_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workSrvSocks5_t::adjp_t * adj = const_cast <workSrvSocks5_t::adjp_t *> (proxy->worker.getAdj(aid));
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
bool awh::ProxySocks5Server::acceptServerCallback(const string & ip, const string & mac, const size_t wid, core_t * core, void * ctx) noexcept {
	// Результат работы функции
	bool result = true;
	// Если данные существуют
	if(!ip.empty() && !mac.empty() && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxySocks5Srv_t * proxy = reinterpret_cast <proxySocks5Srv_t *> (ctx);
		// Если функция обратного вызова установлена, проверяем
		if(proxy->acceptFn != nullptr) result = proxy->acceptFn(ip, mac, proxy, proxy->ctx.at(3));
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
void awh::ProxySocks5Server::readClientCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((size > 0) && (aid > 0) && (wid > 0) && (buffer != nullptr) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxySocks5Srv_t * proxy = reinterpret_cast <proxySocks5Srv_t *> (ctx);
		// Ищем идентификатор адъютанта пары
		auto it = proxy->worker.pairs.find(wid);
		// Если адъютант получен
		if(it != proxy->worker.pairs.end()){
			// Получаем параметры подключения адъютанта
			workSrvSocks5_t::adjp_t * adj = const_cast <workSrvSocks5_t::adjp_t *> (proxy->worker.getAdj(it->second));
			// Если подключение выполнено, отправляем ответ клиенту
			if(adj->connect){
				// Если функция обратного вызова установлена, выполняем
				if(proxy->binaryFn != nullptr){
					// Выводим сообщение
					if(proxy->binaryFn(it->second, event_t::RESPONSE, buffer, size, proxy, proxy->ctx.at(1)))
						// Отправляем ответ клиенту
						reinterpret_cast <core_t *> (&proxy->coreSrv)->write(buffer, size, it->second);
				// Отправляем ответ клиенту
				} else reinterpret_cast <core_t *> (&proxy->coreSrv)->write(buffer, size, it->second);
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
void awh::ProxySocks5Server::readServerCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((size > 0) && (aid > 0) && (wid > 0) && (buffer != nullptr) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxySocks5Srv_t * proxy = reinterpret_cast <proxySocks5Srv_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workSrvSocks5_t::adjp_t * adj = const_cast <workSrvSocks5_t::adjp_t *> (proxy->worker.getAdj(aid));
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
						adj->worker.url = proxy->worker.uri.parseUrl(proxy->fmk->format("http://%s:%u", server.host.c_str(), server.port));
						// Выполняем запрос на сервер
						proxy->coreCli.open(adj->worker.wid);
						// Выходим из функции
						return;
					// Если рукопожатие не выполнено
					} else {
						// Устанавливаем флаг запрещающий подключение
						adj->socks5.resCmd(socks5_t::rep_t::FORBIDDEN);
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
				const size_t aid = adj->worker.getAid();
				// Отправляем запрос на внешний сервер
				if(aid > 0){
					// Если функция обратного вызова установлена, выполняем
					if(proxy->binaryFn != nullptr){
						// Выводим сообщение
						if(proxy->binaryFn(aid, event_t::REQUEST, buffer, size, proxy, proxy->ctx.at(1)))
							// Отправляем запрос на внешний сервер
							reinterpret_cast <core_t *> (&proxy->coreCli)->write(buffer, size, aid);
					// Отправляем запрос на внешний сервер
					} else reinterpret_cast <core_t *> (&proxy->coreCli)->write(buffer, size, aid);
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
void awh::ProxySocks5Server::writeServerCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((size > 0) && (aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxySocks5Srv_t * proxy = reinterpret_cast <proxySocks5Srv_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workSrvSocks5_t::adjp_t * adj = const_cast <workSrvSocks5_t::adjp_t *> (proxy->worker.getAdj(aid));
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
void awh::ProxySocks5Server::init(const u_int port, const string & host) noexcept {
	// Устанавливаем порт сервера
	this->port = port;
	// Устанавливаем хост сервера
	this->host = host;
}
/**
 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова
 */
void awh::ProxySocks5Server::on(void * ctx, function <void (const size_t, const mode_t, ProxySocks5Server *, void *)> callback) noexcept {
	// Устанавливаем контекст передаваемого объекта
	this->ctx.at(0) = ctx;
	// Устанавливаем функцию запуска и остановки
	this->openStopFn = callback;
}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений в бинарном виде
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова
 */
void awh::ProxySocks5Server::on(void * ctx, function <bool (const size_t, const event_t, const char *, const size_t, ProxySocks5Server *, void *)> callback) noexcept {
	// Устанавливаем контекст передаваемого объекта
	this->ctx.at(1) = ctx;
	// Устанавливаем функцию получения сообщений в бинарном виде с сервера
	this->binaryFn = callback;
}
/**
 * on Метод добавления функции обработки авторизации
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова для обработки авторизации
 */
void awh::ProxySocks5Server::on(void * ctx, function <bool (const string &, const string &, void *)> callback) noexcept {
	// Устанавливаем контекст передаваемого объекта
	this->ctx.at(2) = ctx;
	// Устанавливаем функцию обратного вызова для обработки авторизации
	this->checkAuthFn = callback;
}
/**
 * on Метод установки функции обратного вызова на событие активации клиента на сервере
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова
 */
void awh::ProxySocks5Server::on(void * ctx, function <bool (const string &, const string &, ProxySocks5Server *, void *)> callback) noexcept {
	// Устанавливаем контекст передаваемого объекта
	this->ctx.at(3) = ctx;
	// Устанавливаем функцию запуска и остановки
	this->acceptFn = callback;
}
/**
 * ip Метод получения IP адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес интернет подключения адъютанта
 */
const string & awh::ProxySocks5Server::ip(const size_t aid) const noexcept {
	// Выводим результат
	return this->worker.ip(aid);
}
/**
 * mac Метод получения MAC адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес устройства адъютанта
 */
const string & awh::ProxySocks5Server::mac(const size_t aid) const noexcept {
	// Выводим результат
	return this->worker.mac(aid);
}
/**
 * start Метод запуска клиента
 */
void awh::ProxySocks5Server::start() noexcept {
	// Если биндинг не запущен, выполняем запуск биндинга
	if(!this->coreSrv.working())
		// Выполняем запуск биндинга
		this->coreSrv.start();
}
/**
 * stop Метод остановки клиента
 */
void awh::ProxySocks5Server::stop() noexcept {
	// Если подключение выполнено
	if(this->coreSrv.working())
		// Завершаем работу, если разрешено остановить
		this->coreSrv.stop();
}
/**
 * close Метод закрытия подключения клиента
 * @param aid идентификатор адъютанта
 */
void awh::ProxySocks5Server::close(const size_t aid) noexcept {
	// Получаем параметры подключения адъютанта
	workSrvSocks5_t::adjp_t * adj = const_cast <workSrvSocks5_t::adjp_t *> (this->worker.getAdj(aid));
	// Если параметры подключения адъютанта получены, устанавливаем флаг закрытия подключения
	if(adj != nullptr){
		// Выполняем отключение всех дочерних клиентов
		reinterpret_cast <core_t *> (&this->coreCli)->close(adj->worker.getAid());
		// Выполняем удаление параметров адъютанта
		this->worker.removeAdj(aid);
	}
	// Отключаем клиента от сервера
	reinterpret_cast <core_t *> (&this->coreSrv)->close(aid);
	// Если функция обратного вызова установлена, выполняем
	if(this->openStopFn != nullptr) this->openStopFn(aid, mode_t::DISCONNECT, this, this->ctx.at(0));
}
/**
 * setMode Метод установки флага модуля
 * @param flag флаг модуля для установки
 */
void awh::ProxySocks5Server::setMode(const u_short flag) noexcept {
	// Устанавливаем флаг ожидания входящих сообщений
	this->worker.wait = (flag & (uint8_t) flag_t::WAITMESS);
	// Устанавливаем флаг отложенных вызовов событий сокета
	this->coreCli.setDefer(flag & (uint8_t) flag_t::DEFER);
	this->coreSrv.setDefer(flag & (uint8_t) flag_t::DEFER);
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	this->coreCli.setNoInfo(flag & (uint8_t) flag_t::NOINFO);
	this->coreSrv.setNoInfo(flag & (uint8_t) flag_t::NOINFO);
}
/**
 * setWaitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read  количество секунд для детекции по чтению
 * @param write количество секунд для детекции по записи
 */
void awh::ProxySocks5Server::setWaitTimeDetect(const time_t read, const time_t write) noexcept {
	// Устанавливаем количество секунд на чтение
	this->worker.timeRead = read;
	// Устанавливаем количество секунд на запись
	this->worker.timeWrite = write;
}
/**
 * setBytesDetect Метод детекции сообщений по количеству байт
 * @param read  количество байт для детекции по чтению
 * @param write количество байт для детекции по записи
 */
void awh::ProxySocks5Server::setBytesDetect(const worker_t::mark_t read, const worker_t::mark_t write) noexcept {
	// Устанавливаем количество байт на чтение
	this->worker.markRead = read;
	// Устанавливаем количество байт на запись
	this->worker.markWrite = write;
}
/**
 * ProxySocks5Server Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::ProxySocks5Server::ProxySocks5Server(const fmk_t * fmk, const log_t * log) noexcept : coreCli(fmk, log), coreSrv(fmk, log), worker(fmk, log), fmk(fmk), log(log) {
	// Устанавливаем контекст сообщения
	this->worker.ctx = this;
	// Устанавливаем событие на запуск системы
	this->worker.openFn = openServerCallback;
	// Устанавливаем функцию чтения данных
	this->worker.readFn = readServerCallback;
	// Устанавливаем функцию записи данных
	this->worker.writeFn = writeServerCallback;
	// Добавляем событие аццепта клиента
	this->worker.acceptFn = acceptServerCallback;
	// Устанавливаем функцию персистентного вызова
	this->worker.persistFn = persistServerCallback;
	// Устанавливаем событие подключения
	this->worker.connectFn = connectServerCallback;
	// Устанавливаем событие отключения
	this->worker.disconnectFn = disconnectServerCallback;
	// Активируем персистентный запуск для работы пингов
	this->coreSrv.setPersist(true);
	// Добавляем воркер в биндер TCP/IP
	this->coreSrv.add(&this->worker);
	// Устанавливаем функцию активации ядра сервера
	this->coreSrv.setCallback(this, &runCallback);
	// Устанавливаем интервал персистентного таймера для работы пингов
	this->coreSrv.setPersistInterval(KEEPALIVE_TIMEOUT / 2);
}
