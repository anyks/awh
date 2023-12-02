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
 * sendTimeout Метод отправки сигнала таймаута
 */
void awh::client::WebSocket::sendTimeout() noexcept {
	// Выполняем отправку сигнала таймаута
	this->_ws.sendTimeout();
}
/**
 * sendError Метод отправки сообщения об ошибке на сервер
 * @param mess отправляемое сообщение об ошибке
 */
void awh::client::WebSocket::sendError(const ws::mess_t & mess) noexcept {
	// Выполняем отправку сообщения об ошибке
	this->_ws.sendError(mess);
}
/**
 * sendMessage Метод отправки сообщения на сервер
 * @param message передаваемое сообщения в бинарном виде
 * @param utf8    данные передаются в текстовом виде
 */
void awh::client::WebSocket::sendMessage(const vector <char> & message, const bool utf8) noexcept {
	// Выполняем отправку сообщения на сервер
	this->_ws.sendMessage(message, utf8);
}
/**
 * pause Метод установки на паузу клиента
 */
void awh::client::WebSocket::pause() noexcept {
	// Выполняем постановку клиента на паузу
	this->_ws.pause();
}
/**
 * init Метод инициализации клиента
 * @param dest        адрес назначения удалённого сервера
 * @param compressors список поддерживаемых компрессоров
 */
void awh::client::WebSocket::init(const string & dest, const vector <awh::http_t::compress_t> & compressors) noexcept {
	// Выполняем инициализацию клиента
	this->_ws.init(dest, compressors);
}
/**
 * open Метод открытия подключения
 */
void awh::client::WebSocket::open() noexcept {
	// Выполняем открытие подключения
	this->_ws.open();
}
/**
 * stop Метод остановки клиента
 */
void awh::client::WebSocket::stop() noexcept {
	// Выполняем остановку работы модуля
	this->_ws.stop();
}
/**
 * start Метод запуска клиента
 */
void awh::client::WebSocket::start() noexcept {
	// Выполняем запуск работы модуля
	this->_ws.start();
}
/**
 * callback Метод установки функций обратного вызова
 * @param callback функции обратного вызова
 */
void awh::client::WebSocket::callback(const fn_t & callback) noexcept {
	// Выполняем установку функций обратного вызова
	this->_ws.callback(callback);
}
/**
 * subprotocol Метод установки поддерживаемого сабпротокола
 * @param subprotocol сабпротокол для установки
 */
void awh::client::WebSocket::subprotocol(const string & subprotocol) noexcept {
	// Выполняем установку поддерживаемого сабпротокола
	this->_ws.subprotocol(subprotocol);
}
/**
 * subprotocol Метод получения списка выбранных сабпротоколов
 * @return список выбранных сабпротоколов
 */
const set <string> & awh::client::WebSocket::subprotocols() const noexcept {
	// Выполняем извлечение списка выбранных сабпротоколов
	return this->_ws.subprotocols();
}
/**
 * subprotocols Метод установки списка поддерживаемых сабпротоколов
 * @param subprotocols сабпротоколы для установки
 */
void awh::client::WebSocket::subprotocols(const set <string> & subprotocols) noexcept {
	// Выполняем установку поддерживаемых сабпротоколов
	this->_ws.subprotocols(subprotocols);
}
/**
 * extensions Метод извлечения списка расширений WebSocket
 * @return список поддерживаемых расширений
 */
const vector <vector <string>> & awh::client::WebSocket::extensions() const noexcept {
	// Выполняем извлечение списка расширений
	return this->_ws.extensions();
}
/**
 * extensions Метод установки списка расширений WebSocket
 * @param extensions список поддерживаемых расширений
 */
void awh::client::WebSocket::extensions(const vector <vector <string>> & extensions) noexcept {
	// Выполняем установку списка расширений
	this->_ws.extensions(extensions);
}
/**
 * chunk Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::client::WebSocket::chunk(const size_t size) noexcept {
	// Выполняем установку размера чанка
	this->_ws.chunk(size);
}
/**
 * segmentSize Метод установки размеров сегментов фрейма WebSocket
 * @param size минимальный размер сегмента
 */
void awh::client::WebSocket::segmentSize(const size_t size) noexcept {
	// Выполняем установку размера сегмента фрейма WebSocket
	this->_ws.segmentSize(size);
}
/**
 * attempts Метод установки общего количества попыток
 * @param attempts общее количество попыток
 */
void awh::client::WebSocket::attempts(const uint8_t attempts) noexcept {
	// Выполняем установку количества попыток редиректа
	this->_ws.attempts(attempts);
}
/**
 * hosts Метод загрузки файла со списком хостов
 * @param filename адрес файла для загрузки
 */
void awh::client::WebSocket::hosts(const string & filename) noexcept {
	// Если адрес файла с хостами в операционной системе передан
	if(!filename.empty())
		// Выполняем установку адреса файла хостов в операционной системе
		this->_dns.hosts(filename);
}
/**
 * mode Метод установки флагов настроек модуля
 * @param flags список флагов настроек модуля для установки
 */
void awh::client::WebSocket::mode(const set <web_t::flag_t> & flags) noexcept {
	// Выполняем установку флагов настроек модуля
	this->_ws.mode(flags);
}
/**
 * user Метод установки параметров авторизации
 * @param login    логин пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 */
void awh::client::WebSocket::user(const string & login, const string & password) noexcept {
	// Выполняем установку логина и пароля пользователя
	this->_ws.user(login, password);
}
/**
 * keepAlive Метод установки жизни подключения
 * @param cnt   максимальное количество попыток
 * @param idle  интервал времени в секундах через которое происходит проверка подключения
 * @param intvl интервал времени в секундах между попытками
 */
void awh::client::WebSocket::keepAlive(const int cnt, const int idle, const int intvl) noexcept {
	// Выполняем установку жизни подключения
	this->_ws.keepAlive(cnt, idle, intvl);
}
/**
 * compressors Метод установки списка поддерживаемых компрессоров
 * @param compressors список поддерживаемых компрессоров
 */
void awh::client::WebSocket::compressors(const vector <awh::http_t::compress_t> & compressors) noexcept {
	// Выполняем установку списка поддерживаемых компрессоров
	this->_ws.compressors(compressors);
}
/**
 * multiThreads Метод активации многопоточности в WebSocket
 * @param count количество потоков для активации
 * @param mode  флаг активации/деактивации мультипоточности
 */
void awh::client::WebSocket::multiThreads(const uint16_t count, const bool mode) noexcept {
	// Выполняем активацию многопоточности при получения данных в WebSocket
	this->_ws.multiThreads(count, mode);
}
/**
 * setHeaders Метод установки списка заголовков
 * @param headers список заголовков для установки
 */
void awh::client::WebSocket::setHeaders(const unordered_multimap <string, string> & headers) noexcept {
	// Выполняем установку заголовков необходимых при передаче на сервер во время рукопожатия
	this->_ws.setHeaders(headers);
}
/**
 * userAgent Метод установки User-Agent для HTTP-запроса
 * @param userAgent агент пользователя для HTTP-запроса
 */
void awh::client::WebSocket::userAgent(const string & userAgent) noexcept {
	// Выполняем установку User-Agent для HTTP-запроса
	this->_ws.userAgent(userAgent);
}
/**
 * ident Метод установки идентификации клиента
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::client::WebSocket::ident(const string & id, const string & name, const string & ver) noexcept {
	// Выполняем установку данных сервиса
	this->_ws.ident(id, name, ver);
}
/**
 * proxy Метод установки прокси-сервера
 * @param uri    параметры прокси-сервера
 * @param family семейстово интернет протоколов (IPV4 / IPV6 / NIX)
 */
void awh::client::WebSocket::proxy(const string & uri, const scheme_t::family_t family) noexcept {
	// Выполняем установку прокси-сервера
	this->_ws.proxy(uri, family);
}
/**
 * flushDNS Метод сброса кэша DNS-резолвера
 * @return результат работы функции
 */
bool awh::client::WebSocket::flushDNS() noexcept {
	// Выполняем сброс кэша DNS-резолвера
	return this->_dns.flush();
}
/**
 * timeoutDNS Метод установки времени ожидания выполнения запроса
 * @param sec интервал времени выполнения запроса в секундах
 */
void awh::client::WebSocket::timeoutDNS(const uint8_t sec) noexcept {
	// Если время ожидания выполнения DNS-запроса передано
	if(sec > 0)
		// Выполняем установку времени ожидания получения данных с DNS-сервера
		this->_dns.timeout(sec);
}
/**
 * timeToLiveDNS Метод установки времени жизни DNS-кэша
 * @param ttl время жизни DNS-кэша в миллисекундах
 */
void awh::client::WebSocket::timeToLiveDNS(const time_t ttl) noexcept {
	// Если значение времени жизни DNS-кэша передано
	if(ttl > 0)
		// Выполняем установку времени жизни DNS-кэша
		this->_dns.timeToLive(ttl);
}
/**
 * prefixDNS Метод установки префикса переменной окружения для извлечения серверов имён
 * @param prefix префикс переменной окружения для установки
 */
void awh::client::WebSocket::prefixDNS(const string & prefix) noexcept {
	// Если префикс переменной окружения для извлечения серверов имён передан
	if(!prefix.empty())
		// Выполняем установку префикса переменной окружения
		this->_dns.prefix(prefix);
}
/**
 * clearDNSBlackList Метод очистки чёрного списка
 * @param domain доменное имя для которого очищается чёрный список
 */
void awh::client::WebSocket::clearDNSBlackList(const string & domain) noexcept {
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
void awh::client::WebSocket::delInDNSBlackList(const string & domain, const string & ip) noexcept {
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
void awh::client::WebSocket::setToDNSBlackList(const string & domain, const string & ip) noexcept {
	// Если доменное имя для добавление в чёрный список и соответствующий ему IP-адрес переданы
	if(!domain.empty() && !ip.empty())
		// Выполняем установку доменного имени в чёрный список
		this->_dns.setToBlackList(domain, ip);
}
/**
 * encryption Метод активации шифрования
 * @param mode флаг активации шифрования
 */
void awh::client::WebSocket::encryption(const bool mode) noexcept {
	// Выполняем установку флага шифрования
	this->_ws.encryption(mode);
}
/**
 * encryption Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::client::WebSocket::encryption(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Выполняем установку параметров шифрования
	this->_ws.encryption(pass, salt, cipher);
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest-авторизации
 */
void awh::client::WebSocket::authType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Выполняем установку типа авторизации
	this->_ws.authType(type, hash);
}
/**
 * authTypeProxy Метод установки типа авторизации прокси-сервера
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest-авторизации
 */
void awh::client::WebSocket::authTypeProxy(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Выполняем установку типа авторизации на прокси-сервере
	this->_ws.authTypeProxy(type, hash);
}
/**
 * bytesDetect Метод детекции сообщений по количеству байт
 * @param read  количество байт для детекции по чтению
 * @param write количество байт для детекции по записи
 */
void awh::client::WebSocket::bytesDetect(const scheme_t::mark_t read, const scheme_t::mark_t write) noexcept {
	// Выполняем установку детекции сообщений по количеству байт
	this->_ws.bytesDetect(read, write);
}
/**
 * waitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read    количество секунд для детекции по чтению
 * @param write   количество секунд для детекции по записи
 * @param connect количество секунд для детекции по подключению
 */
void awh::client::WebSocket::waitTimeDetect(const time_t read, const time_t write, const time_t connect) noexcept {
	// Выполняем установку детекции сообщений по количеству секунд
	this->_ws.waitTimeDetect(read, write, connect);
}
/**
 * network Метод установки параметров сети
 * @param ips    список IP-адресов компьютера с которых разрешено выходить в интернет
 * @param ns     список серверов имён, через которые необходимо производить резолвинг доменов
 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
 */
void awh::client::WebSocket::network(const vector <string> & ips, const vector <string> & ns, const scheme_t::family_t family) noexcept {
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
 * WebSocket Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::client::WebSocket::WebSocket(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 _dns(fmk, log), _ws(core, fmk, log), _fmk(fmk), _log(log), _core(core) {
	// Выполняем установку DNS-резолвера
	const_cast <client::core_t *> (core)->resolver(&this->_dns);
}
