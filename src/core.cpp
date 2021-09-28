/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <core.hpp>

// Если - это Windows
#if defined(_WIN32) || defined(_WIN64)
	/**
	 * winSocketInit Метод инициализации WinSock
	 */
	void awh::Core::winSocketInit() const noexcept {
		// Если winSock ещё не инициализирован
		if(!this->winSock){
			// Идентификатор ошибки
			int error = 0;
			// Объект данных запроса
			WSADATA wsaData;
			// Выполняем инициализацию сетевого контекста
			if((error = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0){ // 0x202
				// Сообщаем, что сетевой контекст не поднят
				this->log->print("WSAStartup failed with error: %d", log_t::flag_t::CRITICAL, error);
				// Выходим из приложения
				exit(EXIT_FAILURE);
			}
			// Выполняем проверку версии WinSocket
			if((2 != LOBYTE(wsaData.wVersion)) || (2 != HIBYTE(wsaData.wVersion))){
				// Сообщаем, что версия WinSocket не подходит
				this->log->print("%s", log_t::flag_t::CRITICAL, "WSADATA version is not correct");
				// Очищаем сетевой контекст
				this->winSocketClean();
				// Выходим из приложения
				exit(EXIT_FAILURE);
			}
			// Запоминаем, что winSock уже инициализирован
			this->winSock = true;
		}
	}
	/**
	 * winSocketClean Метод очистки WinSock
	 */
	void awh::Core::winSocketClean() const noexcept {
		// Очищаем сетевой контекст
		WSACleanup();
		// Запоминаем, что winSock не инициализирован
		this->winSock = false;
	}
#endif
/**
 * delay Метод фриза потока на указанное количество секунд
 * @param seconds количество секунд для фриза потока
 */
void awh::Core::delay(const size_t seconds) const noexcept {
	// Если количество секунд передано
	if(seconds){
		// Если операционной системой является Windows
		#if defined(_WIN32) || defined(_WIN64)
			// Выполняем фриз потока
			::Sleep(seconds * 1000);
		// Для всех остальных операционных систем
		#else
			// Выполняем фриз потока
			::sleep(seconds);
		#endif
	}
}
/**
 * socket Метод создания сокета
 * @param ip     адрес для которого нужно создать сокет
 * @param port   порт сервера для которого нужно создать сокет
 * @param family тип протокола интернета AF_INET или AF_INET6
 * @return       параметры подключения к серверу
 */
const awh::Core::socket_t awh::Core::socket(const string & ip, const u_int port, const int family) const noexcept {
	// Результат работы функции
	socket_t result;
	// Если IP адрес передан
	if(!ip.empty() && (port > 0) && (port <= 65535)){
		// Адрес сервера для биндинга
		string host = "";
		// Размер структуры подключения
		socklen_t size = 0;
		// Объект подключения
		struct sockaddr * sin = nullptr;
		// Определяем тип подключения
		switch(family){
			// Для протокола IPv4
			case AF_INET: {
				// Получаем список ip адресов
				auto ips = this->net.v4.first;
				// Если количество элементов больше 1
				if(ips.size() > 1){
					// рандомизация генератора случайных чисел
					srand(time(0));
					// Получаем ip адрес
					host = ips.at(rand() % ips.size());
				// Выводим только первый элемент
				} else host = ips.front();
				// Очищаем всю структуру для клиента
				memset(&result.client, 0, sizeof(result.client));
				// Очищаем всю структуру для сервера
				memset(&result.server, 0, sizeof(result.server));
				// Устанавливаем протокол интернета
				result.client.sin_family = AF_INET;
				result.server.sin_family = AF_INET;
				// Устанавливаем произвольный порт для локального подключения
				result.client.sin_port = htons(0);
				// Устанавливаем порт для локального подключения
				result.server.sin_port = htons(port);
				// Устанавливаем адрес для локальго подключения
				result.client.sin_addr.s_addr = inet_addr(host.c_str());
				// Устанавливаем адрес для удаленного подключения
				result.server.sin_addr.s_addr = inet_addr(ip.c_str());
				// Обнуляем серверную структуру
				memset(&result.server.sin_zero, 0, sizeof(result.server.sin_zero));
				// Запоминаем размер структуры
				size = sizeof(result.client);
				// Запоминаем полученную структуру
				sin = reinterpret_cast <struct sockaddr *> (&result.client);
				// Создаем сокет подключения
				result.fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			} break;
			// Для протокола IPv6
			case AF_INET6: {
				// Получаем список ip адресов
				auto ips = this->net.v6.first;
				// Если количество элементов больше 1
				if(ips.size() > 1){
					// рандомизация генератора случайных чисел
					srand(time(0));
					// Получаем ip адрес
					host = ips.at(rand() % ips.size());
				// Выводим только первый элемент
				} else host = ips.front();
				// Переводим ip адрес в полноценный вид
				host = move(this->nwk->setLowIp6(host));
				// Буфер содержащий адрес IPv6
				// char hostClient[INET6_ADDRSTRLEN], hostServer[INET6_ADDRSTRLEN];
				// Очищаем всю структуру для клиента
				memset(&result.client6, 0, sizeof(result.client6));
				// Очищаем всю структуру для сервера
				memset(&result.server6, 0, sizeof(result.server6));
				// Неважно, IPv4 или IPv6
				result.client6.sin6_family = AF_INET6;
				result.server6.sin6_family = AF_INET6;
				// Устанавливаем произвольный порт для локального подключения
				result.client6.sin6_port = htons(0);
				// Устанавливаем порт для локального подключения
				result.server6.sin6_port = htons(port);
				// Указываем адреса
				inet_pton(AF_INET6, host.c_str(), &result.client6.sin6_addr);
				inet_pton(AF_INET6, ip.c_str(), &result.server6.sin6_addr);
				// Устанавливаем адреса
				// inet_ntop(AF_INET6, &result.client6.sin6_addr, hostClient, sizeof(hostClient));
				// inet_ntop(AF_INET6, &result.server6.sin6_addr, hostServer, sizeof(hostServer));
				// Запоминаем размер структуры
				size = sizeof(result.client6);
				// Запоминаем полученную структуру
				sin = reinterpret_cast <struct sockaddr *> (&result.client6);
				// Создаем сокет подключения
				result.fd = ::socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
			} break;
			// Если тип сети не определен
			default: {
				// Выводим сообщение в консоль
				this->log->print("network not allow from server = %s, port = %u", log_t::flag_t::CRITICAL, ip.c_str(), port);
				// Выходим
				return result;
			}
		}
		// Если сокет не создан то выходим
		if(result.fd < 0){
			// Выводим сообщение в консоль
			this->log->print("creating socket to server = %s, port = %u", log_t::flag_t::CRITICAL, ip.c_str(), port);
			// Выходим
			return result;
		}
		// Если - это Unix
		#if !defined(_WIN32) && !defined(_WIN64)
			// Выполняем игнорирование сигнала неверной инструкции процессора
			sockets_t::noSigill(this->log);
			// Устанавливаем разрешение на повторное использование сокета
			sockets_t::reuseable(result.fd, this->log);
			// Отключаем сигнал записи в оборванное подключение
			sockets_t::noSigpipe(result.fd, this->log);
			// Отключаем алгоритм Нейгла для сервера и клиента
			sockets_t::tcpNodelay(result.fd, this->log);
			// Разблокируем сокет
			sockets_t::nonBlocking(result.fd, this->log);
			// Активируем keepalive
			sockets_t::keepAlive(result.fd, this->alive.keepcnt, this->alive.keepidle, this->alive.keepintvl, this->log);
		// Если - это Windows
		#else
			// Выполняем инициализацию WinSock
			this->winSocketInit();
			// Переводим сокет в блокирующий режим
			// sockets_t::blocking(result.fd);
			evutil_make_socket_nonblocking(result.fd);
			// evutil_make_socket_closeonexec(result.fd);
			evutil_make_listen_socket_reuseable(result.fd);
		#endif
		// Выполняем бинд на сокет
		if(::bind(result.fd, sin, size) < 0){
			// Выводим в лог сообщение
			this->log->print("bind local network [%s] error", log_t::flag_t::CRITICAL, host.c_str());
			// Выходим
			return result;
		}
	}
	// Выводим результат
	return result;
}
/**
 * bind Метод подключения модуля ядра к текущей базе событий
 * @param core модуль ядра для подключения
 */
void awh::Core::bind(Core * core) noexcept {
	// Если модуль ядра передан
	if(core != nullptr){
		// Выполняем блокировку потока
		this->bloking.lock();
		// Устанавливаем базу событий
		core->base = this->base;
		// Выполняем блокировку инициализации базы событий
		core->locker = (core->base != nullptr);
		// Если блокировка базы событий выполнена
		if(core->locker){
			try {
				// Резолвер IPv4, создаём резолвер
				if(core->dns4 != nullptr) core->dns4 = new dns_t(core->fmk, core->log, core->nwk, core->base, core->net.v4.second);
				// Резолвер IPv6, создаём резолвер
				if(core->dns6 != nullptr) core->dns6 = new dns_t(core->fmk, core->log, core->nwk, core->base, core->net.v6.second);
				// Если список воркеров существует
				if(!core->workers.empty()){
					// Переходим по всему списку воркеров
					for(auto & worker : core->workers){
						// Если функция обратного вызова установлена
						if(worker.second->startFn != nullptr)
							// Выполняем функцию обратного вызова
							worker.second->startFn(worker.first, core, worker.second->context);
					}
				}
				// Если функция обратного вызова установлена, выполняем
				if(core->startFn != nullptr) core->startFn(core->base, core, core->ctx);
			// Если происходит ошибка то игнорируем её
			} catch(const bad_alloc&) {
				// Выводим сообщение об ошибке
				core->log->print("%s", log_t::flag_t::CRITICAL, "memory could not be allocated");
			}
		}
		// Выполняем разблокировку потока
		this->bloking.unlock();
	}
}
/**
 * unbind Метод отключения модуля ядра от текущей базы событий
 * @param core модуль ядра для отключения
 */
void awh::Core::unbind(Core * core) noexcept {
	// Если модуль ядра передан
	if(core != nullptr){
		// Выполняем блокировку потока
		this->bloking.lock();
		// Отключаем всех клиентов
		if(core->base != nullptr) core->closeAll();
		// Удаляем DNS сервера IPv4
		if(core->dns4 != nullptr) delete core->dns4;
		// Удаляем DNS сервера IPv6
		if(core->dns6 != nullptr) delete core->dns6;
		// Зануляем базу событий
		core->base = nullptr;
		// Если функция обратного вызова установлена, выполняем
		if(core->stopFn != nullptr) core->stopFn(core, core->ctx);
		// Выполняем сброс блокировки базы событий
		core->locker = false;
		// Выполняем разблокировку потока
		this->bloking.unlock();
	}
}
/**
 * setStopCallback Метод установки функции обратного вызова при завершении работы модуля
 * @param callback функция обратного вызова для установки
 */
void awh::Core::setStopCallback(function <void (Core * core, void *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->stopFn = callback;
}
/**
 * setStartCallback Метод установки функции обратного вызова при запуске работы модуля
 * @param callback функция обратного вызова для установки
 */
void awh::Core::setStartCallback(function <void (struct event_base *, Core * core, void *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->startFn = callback;
}
/**
 * isStart Метод проверки на запуск бинда TCP/IP
 * @return результат проверки
 */
bool awh::Core::isStart() const noexcept {
	// Выводим результат проверки
	return this->mode;
}
/**
 * closeAll Метод отключения всех воркеров
 */
void awh::Core::closeAll() noexcept {
	// Если список воркеров активен
	if(!this->workers.empty()){
		// Переходим по всему списку воркеров
		for(auto & worker : this->workers)
			// Выполняем закрытие подключения
			this->close(worker.second);
	}
}
/**
 * removeAll Метод удаления всех воркеров
 */
void awh::Core::removeAll() noexcept {
	// Выполняем удаление всех воркеров
	this->workers.clear();
}
/**
 * close Метод закрытия подключения воркером
 * @param wid идентификатор воркера
 */
void awh::Core::close(const size_t wid) noexcept {
	// Если идентификатор воркера передан
	if(wid > 0){
		// Выполняем поиск воркера
		auto it = this->workers.find(wid);
		// Если воркер найден
		if(it != this->workers.end()) this->close(it->second);
	}
}
/**
 * remove Метод удаления воркера из биндинга
 * @param wid идентификатор воркера
 */
void awh::Core::remove(const size_t wid) noexcept {
	// Если идентификатор воркера передан
	if(wid > 0){
		// Выполняем поиск воркера
		auto it = this->workers.find(wid);
		// Если воркер найден
		if(it != this->workers.end()) this->workers.erase(it);
	}
}
/**
 * setVerifySSL Метод разрешающий или запрещающий, выполнять проверку соответствия, сертификата домену
 * @param mode флаг состояния разрешения проверки
 */
void awh::Core::setVerifySSL(const bool mode) noexcept {
	// Выполняем установку флага проверки домена
	this->ssl->setVerify(mode);
}
/**
 * setFamily Метод установки тип протокола интернета
 * @param family тип протокола интернета AF_INET или AF_INET6
 */
void awh::Core::setFamily(const int family) noexcept {
	// Устанавливаем тип активного интернет-подключения
	this->net.family = family;
}
/**
 * setCA Метод установки CA-файла корневого SSL сертификата
 * @param cafile адрес CA-файла
 * @param capath адрес каталога где находится CA-файл
 */
void awh::Core::setCA(const string & cafile, const string & capath) noexcept {
	// Устанавливаем адрес CA-файла
	this->ssl->setCA(cafile, capath);
}
/**
 * setNet Метод установки параметров сети
 * @param ip     список IP адресов компьютера с которых разрешено выходить в интернет
 * @param ns     список серверов имён, через которые необходимо производить резолвинг доменов
 * @param family тип протокола интернета AF_INET или AF_INET6
 */
void awh::Core::setNet(const vector <string> & ip, const vector <string> & ns, const int family) noexcept {
	// Устанавливаем тип активного интернет-подключения
	this->net.family = family;
	// Определяем тип интернет-протокола
	switch(this->net.family){
		// Если - это интернет-протокол IPv4
		case AF_INET: {
			// Если IP адреса переданы, устанавливаем их
			if(!ip.empty()) this->net.v4.first.assign(ip.cbegin(), ip.cend());
			// Если сервера имён переданы, устанавливаем их
			if(!ns.empty()) this->net.v4.second.assign(ns.cbegin(), ns.cend());
		} break;
		// Если - это интернет-протокол IPv6
		case AF_INET6: {
			// Если IP адреса переданы, устанавливаем их
			if(!ip.empty()) this->net.v6.first.assign(ip.cbegin(), ip.cend());
			// Если сервера имён переданы, устанавливаем их
			if(!ns.empty()) this->net.v6.second.assign(ns.cbegin(), ns.cend());
		} break;
	}
}
/**
 * Core Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Core::Core(const fmk_t * fmk, const log_t * log) noexcept {
	try {
		// Устанавливаем зависимые модули
		this->fmk = fmk;
		this->log = log;
		// Создаём объект для работы с сетью
		this->nwk = new network_t(this->fmk);
		// Создаём объект URI
		this->uri = new uri_t(this->fmk, this->nwk);
		// Создаём объект для работы с SSL
		this->ssl = new ssl_t(this->fmk, this->log, this->uri);
	// Если происходит ошибка то игнорируем её
	} catch(const bad_alloc&) {
		// Выводим сообщение об ошибке
		log->print("%s", log_t::flag_t::CRITICAL, "memory could not be allocated");
		// Выходим из приложения
		exit(EXIT_FAILURE);
	}
}
/**
 * ~Core Деструктор
 */
awh::Core::~Core() noexcept {
	// Если объект для работы с SSL создан
	if(this->ssl != nullptr) delete this->ssl;
	// Удаляем объект для работы с URI
	if(this->uri != nullptr) delete this->uri;
	// Удаляем объект для работы с сетью
	if(this->nwk != nullptr) delete this->nwk;
	// Если объект DNS IPv4 резолвера создан
	if(this->dns4 != nullptr) delete this->dns4;
	// Если объект DNS IPv6 резолвера создан
	if(this->dns6 != nullptr) delete this->dns6;
	// Если - это Windows
	#if defined(_WIN32) || defined(_WIN64)
		// Очищаем сетевой контекст
		this->winSocketClean();
	#endif
}
