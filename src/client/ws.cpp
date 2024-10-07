/**
 * @file: ws.cpp
 * @date: 2023-09-25
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2023
 */

// Подключаем заголовочный файл
#include <client/ws.hpp>

/**
 * proto Метод извлечения поддерживаемого протокола подключения
 * @return поддерживаемый протокол подключения (HTTP1_1, HTTP2)
 */
awh::engine_t::proto_t awh::client::Websocket::proto() const noexcept {
	// Выполняем определение активного HTTP-протокола
	return this->_ws.proto();
}
/**
 * sendError Метод отправки сообщения об ошибке на сервер
 * @param mess отправляемое сообщение об ошибке
 */
void awh::client::Websocket::sendError(const ws::mess_t & mess) noexcept {
	// Выполняем отправку сообщения об ошибке
	this->_ws.sendError(mess);
}
/**
 * sendMessage Метод отправки сообщения на сервер
 * @param message передаваемое сообщения в бинарном виде
 * @param utf8    данные передаются в текстовом виде
 * @return        результат отправки сообщения
 */
bool awh::client::Websocket::sendMessage(const vector <char> & message, const bool utf8) noexcept {
	// Выполняем отправку сообщения на сервер
	return this->_ws.sendMessage(message, utf8);
}
/**
 * pause Метод установки на паузу клиента
 */
void awh::client::Websocket::pause() noexcept {
	// Выполняем постановку клиента на паузу
	this->_ws.pause();
}
/**
 * init Метод инициализации клиента
 * @param dest        адрес назначения удалённого сервера
 * @param compressors список поддерживаемых компрессоров
 */
void awh::client::Websocket::init(const string & dest, const vector <awh::http_t::compressor_t> & compressors) noexcept {
	// Выполняем инициализацию клиента
	this->_ws.init(dest, compressors);
}
/**
 * open Метод открытия подключения
 */
void awh::client::Websocket::open() noexcept {
	// Выполняем открытие подключения
	this->_ws.open();
}
/**
 * reset Метод принудительного сброса подключения
 */
void awh::client::Websocket::reset() noexcept {
	// Выполняем отправку сигнала таймаута
	this->_ws.reset();
}
/**
 * stop Метод остановки клиента
 */
void awh::client::Websocket::stop() noexcept {
	// Выполняем остановку работы модуля
	this->_ws.stop();
}
/**
 * start Метод запуска клиента
 */
void awh::client::Websocket::start() noexcept {
	// Выполняем запуск работы модуля
	this->_ws.start();
}
/**
 * waitPong Метод установки времени ожидания ответа WebSocket-сервера
 * @param sec время ожидания в секундах
 */
void awh::client::Websocket::waitPong(const time_t sec) noexcept {
	// Выполняем установку времени ожидания
	this->_ws.waitPong(sec);
}
/**
 * pingInterval Метод установки интервала времени выполнения пингов
 * @param sec интервал времени выполнения пингов в секундах
 */
void awh::client::Websocket::pingInterval(const time_t sec) noexcept {
	// Выполняем установку интервала времени выполнения пингов в секундах
	this->_ws.pingInterval(sec);
}
/**
 * callbacks Метод установки функций обратного вызова
 * @param callbacks функции обратного вызова
 */
void awh::client::Websocket::callbacks(const fn_t & callbacks) noexcept {
	// Выполняем установку функций обратного вызова
	this->_ws.callbacks(callbacks);
}
/**
 * subprotocol Метод установки поддерживаемого сабпротокола
 * @param subprotocol сабпротокол для установки
 */
void awh::client::Websocket::subprotocol(const string & subprotocol) noexcept {
	// Выполняем установку поддерживаемого сабпротокола
	this->_ws.subprotocol(subprotocol);
}
/**
 * subprotocol Метод получения списка выбранных сабпротоколов
 * @return список выбранных сабпротоколов
 */
const set <string> & awh::client::Websocket::subprotocols() const noexcept {
	// Выполняем извлечение списка выбранных сабпротоколов
	return this->_ws.subprotocols();
}
/**
 * subprotocols Метод установки списка поддерживаемых сабпротоколов
 * @param subprotocols сабпротоколы для установки
 */
void awh::client::Websocket::subprotocols(const set <string> & subprotocols) noexcept {
	// Выполняем установку поддерживаемых сабпротоколов
	this->_ws.subprotocols(subprotocols);
}
/**
 * extensions Метод извлечения списка расширений Websocket
 * @return список поддерживаемых расширений
 */
const vector <vector <string>> & awh::client::Websocket::extensions() const noexcept {
	// Выполняем извлечение списка расширений
	return this->_ws.extensions();
}
/**
 * extensions Метод установки списка расширений Websocket
 * @param extensions список поддерживаемых расширений
 */
void awh::client::Websocket::extensions(const vector <vector <string>> & extensions) noexcept {
	// Выполняем установку списка расширений
	this->_ws.extensions(extensions);
}
/**
 * bandwidth Метод установки пропускной способности сети
 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
 */
void awh::client::Websocket::bandwidth(const string & read, const string & write) noexcept {
	// Выполняем установку пропускной способности сети
	this->_ws.bandwidth(read, write);
}
/**
 * chunk Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::client::Websocket::chunk(const size_t size) noexcept {
	// Выполняем установку размера чанка
	this->_ws.chunk(size);
}
/**
 * segmentSize Метод установки размеров сегментов фрейма Websocket
 * @param size минимальный размер сегмента
 */
void awh::client::Websocket::segmentSize(const size_t size) noexcept {
	// Выполняем установку размера сегмента фрейма Websocket
	this->_ws.segmentSize(size);
}
/**
 * attempts Метод установки общего количества попыток
 * @param attempts общее количество попыток
 */
void awh::client::Websocket::attempts(const uint8_t attempts) noexcept {
	// Выполняем установку количества попыток редиректа
	this->_ws.attempts(attempts);
}
/**
 * hosts Метод загрузки файла со списком хостов
 * @param filename адрес файла для загрузки
 */
void awh::client::Websocket::hosts(const string & filename) noexcept {
	// Если адрес файла с хостами в операционной системе передан
	if(!filename.empty())
		// Выполняем установку адреса файла хостов в операционной системе
		this->_dns.hosts(filename);
}
/**
 * mode Метод установки флагов настроек модуля
 * @param flags список флагов настроек модуля для установки
 */
void awh::client::Websocket::mode(const set <web_t::flag_t> & flags) noexcept {
	// Выполняем установку флагов настроек модуля
	this->_ws.mode(flags);
}
/**
 * user Метод установки параметров авторизации
 * @param login    логин пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 */
void awh::client::Websocket::user(const string & login, const string & password) noexcept {
	// Выполняем установку логина и пароля пользователя
	this->_ws.user(login, password);
}
/**
 * compressors Метод установки списка поддерживаемых компрессоров
 * @param compressors список поддерживаемых компрессоров
 */
void awh::client::Websocket::compressors(const vector <awh::http_t::compressor_t> & compressors) noexcept {
	// Выполняем установку списка поддерживаемых компрессоров
	this->_ws.compressors(compressors);
}
/**
 * keepAlive Метод установки жизни подключения
 * @param cnt   максимальное количество попыток
 * @param idle  интервал времени в секундах через которое происходит проверка подключения
 * @param intvl интервал времени в секундах между попытками
 */
void awh::client::Websocket::keepAlive(const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept {
	// Выполняем установку жизни подключения
	this->_ws.keepAlive(cnt, idle, intvl);
}
/**
 * multiThreads Метод активации многопоточности в Websocket
 * @param count количество потоков для активации
 * @param mode  флаг активации/деактивации мультипоточности
 */
void awh::client::Websocket::multiThreads(const uint16_t count, const bool mode) noexcept {
	// Выполняем активацию многопоточности при получения данных в Websocket
	this->_ws.multiThreads(count, mode);
}
/**
 * setHeaders Метод установки списка заголовков
 * @param headers список заголовков для установки
 */
void awh::client::Websocket::setHeaders(const unordered_multimap <string, string> & headers) noexcept {
	// Выполняем установку заголовков необходимых при передаче на сервер во время рукопожатия
	this->_ws.setHeaders(headers);
}
/**
 * userAgent Метод установки User-Agent для HTTP-запроса
 * @param userAgent агент пользователя для HTTP-запроса
 */
void awh::client::Websocket::userAgent(const string & userAgent) noexcept {
	// Выполняем установку User-Agent для HTTP-запроса
	this->_ws.userAgent(userAgent);
}
/**
 * ident Метод установки идентификации клиента
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::client::Websocket::ident(const string & id, const string & name, const string & ver) noexcept {
	// Выполняем установку данных сервиса
	this->_ws.ident(id, name, ver);
}
/**
 * proxy Метод активации/деактивации прокси-склиента
 * @param work флаг активации/деактивации прокси-клиента
 */
void awh::client::Websocket::proxy(const client::scheme_t::work_t work) noexcept {
	// Выполняем установку флага активации/деактивации прокси-склиента
	this->_ws.proxy(work);
}
/**
 * proxy Метод установки прокси-сервера
 * @param uri    параметры прокси-сервера
 * @param family семейстово интернет протоколов (IPV4 / IPV6 / NIX)
 */
void awh::client::Websocket::proxy(const string & uri, const scheme_t::family_t family) noexcept {
	// Выполняем установку прокси-сервера
	this->_ws.proxy(uri, family);
}
/**
 * flushDNS Метод сброса кэша DNS-резолвера
 * @return результат работы функции
 */
bool awh::client::Websocket::flushDNS() noexcept {
	// Выполняем сброс кэша DNS-резолвера
	return this->_dns.flush();
}
/**
 * timeoutDNS Метод установки времени ожидания выполнения запроса
 * @param sec интервал времени выполнения запроса в секундах
 */
void awh::client::Websocket::timeoutDNS(const uint8_t sec) noexcept {
	// Если время ожидания выполнения DNS-запроса передано
	if(sec > 0)
		// Выполняем установку времени ожидания получения данных с DNS-сервера
		this->_dns.timeout(sec);
}
/**
 * prefixDNS Метод установки префикса переменной окружения для извлечения серверов имён
 * @param prefix префикс переменной окружения для установки
 */
void awh::client::Websocket::prefixDNS(const string & prefix) noexcept {
	// Если префикс переменной окружения для извлечения серверов имён передан
	if(!prefix.empty())
		// Выполняем установку префикса переменной окружения
		this->_dns.prefix(prefix);
}
/**
 * clearDNSBlackList Метод очистки чёрного списка
 * @param domain доменное имя для которого очищается чёрный список
 */
void awh::client::Websocket::clearDNSBlackList(const string & domain) noexcept {
	// Если доменное имя для удаления из чёрного списока передано
	if(!domain.empty())
		// Выполняем удаление доменного имени из чёрного списока
		this->_dns.clearBlackList(domain);
}
/**
 * delInDNSBlackList Метод удаления IP-адреса из чёрного списока
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для удаления из чёрного списка
 */
void awh::client::Websocket::delInDNSBlackList(const string & domain, const string & ip) noexcept {
	// Если доменное имя для удаления из чёрного списока и соответствующий ему IP-адрес переданы
	if(!domain.empty() && !ip.empty())
		// Выполняем удаление доменного имени из чёрного списока
		this->_dns.delInBlackList(domain, ip);
}
/**
 * setToDNSBlackList Метод добавления IP-адреса в чёрный список
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для добавления в чёрный список
 */
void awh::client::Websocket::setToDNSBlackList(const string & domain, const string & ip) noexcept {
	// Если доменное имя для добавление в чёрный список и соответствующий ему IP-адрес переданы
	if(!domain.empty() && !ip.empty())
		// Выполняем установку доменного имени в чёрный список
		this->_dns.setToBlackList(domain, ip);
}
/**
 * waitMessage Метод ожидания входящих сообщений
 * @param sec интервал времени в секундах
 */
void awh::client::Websocket::waitMessage(const time_t sec) noexcept {
	// Выполняем установку интервала времени ожидания входящих сообщений
	this->_ws.waitMessage(sec);
}
/**
 * cork Метод отключения/включения алгоритма TCP/CORK
 * @param mode режим применимой операции
 * @return     результат выполенния операции
 */
bool awh::client::Websocket::cork(const engine_t::mode_t mode) noexcept {
	// Выполняем отключение/включение алгоритма TCP/CORK
	return this->_ws.cork(mode);
}
/**
 * nodelay Метод отключения/включения алгоритма Нейгла
 * @param mode режим применимой операции
 * @return     результат выполенния операции
 */
bool awh::client::Websocket::nodelay(const engine_t::mode_t mode) noexcept {
	// Выполняем отключение/включение алгоритма TCP/CORK
	return this->_ws.nodelay(mode);
}
/**
 * encryption Метод активации шифрования
 * @param mode флаг активации шифрования
 */
void awh::client::Websocket::encryption(const bool mode) noexcept {
	// Выполняем установку флага шифрования
	this->_ws.encryption(mode);
}
/**
 * encryption Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::client::Websocket::encryption(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Выполняем установку параметров шифрования
	this->_ws.encryption(pass, salt, cipher);
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest-авторизации
 */
void awh::client::Websocket::authType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Выполняем установку типа авторизации
	this->_ws.authType(type, hash);
}
/**
 * authTypeProxy Метод установки типа авторизации прокси-сервера
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest-авторизации
 */
void awh::client::Websocket::authTypeProxy(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Выполняем установку типа авторизации на прокси-сервере
	this->_ws.authTypeProxy(type, hash);
}
/**
 * waitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read    количество секунд для детекции по чтению
 * @param write   количество секунд для детекции по записи
 * @param connect количество секунд для детекции по подключению
 */
void awh::client::Websocket::waitTimeDetect(const time_t read, const time_t write, const time_t connect) noexcept {
	// Выполняем установку детекции сообщений по количеству секунд
	this->_ws.waitTimeDetect(read, write, connect);
}
/**
 * network Метод установки параметров сети
 * @param ips    список IP-адресов компьютера с которых разрешено выходить в интернет
 * @param ns     список серверов имён, через которые необходимо производить резолвинг доменов
 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
 */
void awh::client::Websocket::network(const vector <string> & ips, const vector <string> & ns, const scheme_t::family_t family) noexcept {
	// Если список IP-адресов передан
	if(!ips.empty()){
		// Определяем тип протокола интернета
		switch(static_cast <uint8_t> (family)){
			// Если протокол интернета соответствует IPv4
			case static_cast <uint8_t> (scheme_t::family_t::IPV4):
				// Добавляем список IP-адресов через которые нужно выходить в интернет
				this->_dns.network(AF_INET, ips);
			break;
			// Если протокол интернета соответствует IPv6
			case static_cast <uint8_t> (scheme_t::family_t::IPV6):
				// Добавляем список IP-адресов через которые нужно выходить в интернет
				this->_dns.network(AF_INET6, ips);
			break;
		}
		// Устанавливаем параметры сети для сетевого ядра
		const_cast <client::core_t *> (this->_core)->network(ips, family);
	}
	// Если список DNS-серверов передан
	if(!ns.empty()){
		// Определяем тип протокола интернета
		switch(static_cast <uint8_t> (family)){
			// Если протокол интернета соответствует IPv4
			case static_cast <uint8_t> (scheme_t::family_t::IPV4):
				// Выполняем установку списка DNS-серверов
				this->_dns.replace(AF_INET, ns);
			break;
			// Если протокол интернета соответствует IPv6
			case static_cast <uint8_t> (scheme_t::family_t::IPV6):
				// Выполняем установку списка DNS-серверов
				this->_dns.replace(AF_INET6, ns);
			break;
		}
	}
}
/**
 * Websocket Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::client::Websocket::Websocket(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 _dns(fmk, log), _ws(core, fmk, log), _fmk(fmk), _log(log), _core(core) {
	// Выполняем установку DNS-резолвера
	const_cast <client::core_t *> (core)->resolver(&this->_dns);
}
