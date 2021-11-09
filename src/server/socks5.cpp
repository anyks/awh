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
		proxySocks5Srv_t * socks5 = reinterpret_cast <proxySocks5Srv_t *> (ctx);
		// Выполняем биндинг базы событий для клиента
		if(mode) socks5->coreSrv.bind(reinterpret_cast <core_t *> (&socks5->coreCli));
		// Выполняем анбиндинг базы событий клиента
		else socks5->coreSrv.unbind(reinterpret_cast <core_t *> (&socks5->coreCli));
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
		proxySocks5Srv_t * socks5 = reinterpret_cast <proxySocks5Srv_t *> (ctx);
		// Устанавливаем хост сервера
		reinterpret_cast <coreSrv_t *> (core)->init(wid, socks5->port, socks5->host);
		// Выполняем запуск сервера
		core->run(wid);
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
		proxySocks5Srv_t * socks5 = reinterpret_cast <proxySocks5Srv_t *> (ctx);
		// Создаём адъютанта
		socks5->worker.createAdj(aid);
		// Получаем параметры подключения адъютанта
		workSrvSocks5_t::adjp_t * adj = const_cast <workSrvSocks5_t::adjp_t *> (socks5->worker.getAdj(aid));
		// Устанавливаем контекст сообщения
		adj->worker.ctx = socks5;
		// Устанавливаем функцию чтения данных
		adj->worker.readFn = readClientCallback;
		// Устанавливаем событие подключения
		adj->worker.connectFn = connectClientCallback;
		// Устанавливаем событие отключения
		adj->worker.disconnectFn = disconnectClientCallback;
		// Добавляем воркер в биндер TCP/IP
		socks5->coreCli.add(&adj->worker);
		// Создаём пару клиента и сервера
		socks5->worker.pairs.emplace(adj->worker.wid, aid);
		// Устанавливаем функцию проверки авторизации
		adj->socks5.setAuthCallback(socks5->ctx.at(2), socks5->checkAuthFn);


		// Устанавливаем URL адрес запроса
		// adj->socks5.setUrl(ws->worker.url);

		// Если функция обратного вызова установлена, выполняем
		if(socks5->openStopFn != nullptr) socks5->openStopFn(aid, mode_t::CONNECT, socks5, socks5->ctx.at(0));
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
		proxySocks5Srv_t * socks5 = reinterpret_cast <proxySocks5Srv_t *> (ctx);
		// Ищем идентификатор адъютанта пары
		auto it = socks5->worker.pairs.find(wid);
		// Если адъютант получен
		if(it != socks5->worker.pairs.end()){
			// Получаем идентификатор адъютанта
			const size_t aid = it->second;
			// Удаляем пару клиента и сервера
			socks5->worker.pairs.erase(it);
			// Получаем параметры подключения адъютанта
			workSrvSocks5_t::adjp_t * adj = const_cast <workSrvSocks5_t::adjp_t *> (socks5->worker.getAdj(aid));
			// Устанавливаем флаг отключения клиента
			adj->close = true;
			// Выполняем отключение клиента
			reinterpret_cast <core_t *> (&socks5->coreSrv)->close(aid);
			// Выполняем удаление параметров адъютанта
			socks5->worker.removeAdj(aid);
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
		proxySocks5Srv_t * socks5 = reinterpret_cast <proxySocks5Srv_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workSrvSocks5_t::adjp_t * adj = const_cast <workSrvSocks5_t::adjp_t *> (socks5->worker.getAdj(aid));
		// Выполняем отключение клиента от стороннего сервера
		if(adj != nullptr){
			// Устанавливаем флаг отключения клиента
			adj->close = true;
			// Выполняем отключение всех дочерних клиентов
			reinterpret_cast <core_t *> (&socks5->coreCli)->close(adj->worker.getAid());
		}
		// Если функция обратного вызова установлена, выполняем
		if(socks5->openStopFn != nullptr) socks5->openStopFn(aid, mode_t::DISCONNECT, socks5, socks5->ctx.at(0));
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
		proxySocks5Srv_t * socks5 = reinterpret_cast <proxySocks5Srv_t *> (ctx);
		// Если функция обратного вызова установлена, проверяем
		if(socks5->acceptFn != nullptr) result = socks5->acceptFn(ip, mac, socks5, socks5->ctx.at(3));
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
		proxySocks5Srv_t * socks5 = reinterpret_cast <proxySocks5Srv_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workSrvSocks5_t::adjp_t * adj = const_cast <workSrvSocks5_t::adjp_t *> (socks5->worker.getAdj(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Если данные не получены
			if(!adj->socks5.isEnd()){
				// Выполняем парсинг входящих данных
				adj->socks5.parse(buffer, size);
				// Получаем данные запроса
				const auto & socks5 = adj->socks5.get();
				// Если данные получены
				if(!socks5.empty()) core->write(socks5.data(), socks5.size(), aid);
				// Если данные все получены
				else if(adj->socks5.isEnd()) {
					// Если рукопожатие выполнено
					if(adj->socks5.isConnected()){
						// adj->socks5.resCmd(socks5_t::rep_t::SUCCESS);
						// const auto & server = adj->socks5.getServer();
					// Если рукопожатие не выполнено
					} else {
						/*
						// Устанавливаем код ответа
						ws->code = ws->worker.proxy.socks5.getCode();
						// Создаём сообщение
						mess_t mess(ws->code);
						// Устанавливаем сообщение ответа
						mess = ws->worker.proxy.socks5.getMessage(ws->code);
						// Если включён режим отладки
						#if defined(DEBUG_MODE)
							// Если заголовки получены
							if(!mess.text.empty()){
								// Данные REST ответа
								const string & response = ws->fmk->format("SOCKS5 %u %s\r\n", ws->code, mess.text.c_str());
								// Выводим заголовок ответа
								cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE PROXY ^^^^^^^^^\x1B[0m" << endl;
								// Выводим параметры ответа
								cout << string(response.begin(), response.end()) << endl;
							}
						#endif
						// Выводим сообщение
						ws->error(mess);
						// Завершаем работу
						core->close(aid);
						 */
					}
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
		proxySocks5Srv_t * socks5 = reinterpret_cast <proxySocks5Srv_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workSrvSocks5_t::adjp_t * adj = const_cast <workSrvSocks5_t::adjp_t *> (socks5->worker.getAdj(aid));
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
 * setAlive Метод установки долгоживущего подключения
 * @param mode флаг долгоживущего подключения
 */
void awh::ProxySocks5Server::setAlive(const bool mode) noexcept {
	// Устанавливаем флаг долгоживущего подключения
	this->alive = mode;
}
/**
 * setAlive Метод установки долгоживущего подключения
 * @param aid  идентификатор адъютанта
 * @param mode флаг долгоживущего подключения
 */
void awh::ProxySocks5Server::setAlive(const size_t aid, const bool mode) noexcept {
	// Получаем параметры подключения адъютанта
	workSrvSocks5_t::adjp_t * adj = const_cast <workSrvSocks5_t::adjp_t *> (this->worker.getAdj(aid));
	// Если параметры подключения адъютанта получены, устанавливаем флаг пдолгоживущего подключения
	if(adj != nullptr) adj->alive = mode;
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
	if(adj != nullptr) adj->close = true;
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
 * setMode Метод установки флага модуля
 * @param flag флаг модуля для установки
 */
void awh::ProxySocks5Server::setMode(const u_short flag) noexcept {
	// Устанавливаем флаг запрещающий метод CONNECT
	this->noConnect = (flag & (uint8_t) flag_t::NOCONNECT);
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
 * setChunkSize Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::ProxySocks5Server::setChunkSize(const size_t size) noexcept {
	// Устанавливаем размер чанка
	this->chunkSize = (size > 0 ? size : BUFFER_CHUNK);
}
/**
 * setKeepAlive Метод установки времени жизни подключения
 * @param time время жизни подключения
 */
void awh::ProxySocks5Server::setKeepAlive(const size_t time) noexcept {
	// Устанавливаем время жизни подключения
	this->keepAlive = time;
}
/**
 * setMaxRequests Метод установки максимального количества запросов
 * @param max максимальное количество запросов
 */
void awh::ProxySocks5Server::setMaxRequests(const size_t max) noexcept {
	// Устанавливаем максимальное количество запросов
	this->maxRequests = max;
}
/**
 * setConnectTimeouts Метод установки таймаутов для метода CONNECT
 * @param read  таймаут в секундах на чтение
 * @param write таймаут в секундах на запись
 */
void awh::ProxySocks5Server::setConnectTimeouts(const time_t read, const time_t write) noexcept {
	// Устанавливаем таймаут на чтение
	this->readTimeout = read;
	// Устанавливаем таймаут на запись
	this->writeTimeout = write;
}
/**
 * setCrypt Метод установки параметров шифрования
 * @param pass пароль шифрования передаваемых данных
 * @param salt соль шифрования передаваемых данных
 * @param aes  размер шифрования передаваемых данных
 */
void awh::ProxySocks5Server::setCrypt(const string & pass, const string & salt, const hash_t::aes_t aes) noexcept {
	// Устанавливаем флаг шифрования
	if((this->crypt = !pass.empty())){
		// Размер шифрования передаваемых данных
		this->aes = aes;
		// Пароль шифрования передаваемых данных
		this->pass = pass;
		// Соль шифрования передаваемых данных
		this->salt = salt;
	}
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
	// Устанавливаем событие подключения
	this->worker.connectFn = connectServerCallback;
	// Устанавливаем событие отключения
	this->worker.disconnectFn = disconnectServerCallback;
	// Добавляем воркер в биндер TCP/IP
	this->coreSrv.add(&this->worker);
	// Устанавливаем функцию активации ядра сервера
	this->coreSrv.setCallback(this, &runCallback);
}
