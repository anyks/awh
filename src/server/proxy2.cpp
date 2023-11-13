/**
 * @file: proxy.cpp
 * @date: 2023-11-12
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
#include <server/proxy2.hpp>

/**
 * passwordCallback Метод извлечения пароля (для авторизации методом Digest)
 * @param bid   идентификатор брокера (клиента)
 * @param login логин пользователя
 * @return      пароль пользователя хранящийся в базе данных
 */
string awh::server::Proxy::passwordCallback(const uint64_t bid, const string & login) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("extractPassword"))
		// Выполняем функцию обратного вызова
		return this->_callback.apply <string, const uint64_t, const string &> ("extractPassword", bid, login);
	// Сообщаем, что пароль для пользователя не найден
	return "";
}
/**
 * authCallback Метод проверки авторизации пользователя (для авторизации методом Basic)
 * @param bid      идентификатор брокера (клиента)
 * @param login    логин пользователя (от клиента)
 * @param password пароль пользователя (от клиента)
 * @return         результат авторизации
 */
bool awh::server::Proxy::authCallback(const uint64_t bid, const string & login, const string & password) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("checkPassword"))
		// Выполняем функцию обратного вызова
		return this->_callback.apply <bool, const uint64_t, const string &, const string &> ("checkPassword", bid, login, password);
	// Сообщаем, что пользователь не прошёл валидацию
	return false;
}
/**
 * acceptCallback Метод активации клиента на сервере
 * @param ip   адрес интернет подключения
 * @param mac  аппаратный адрес подключения
 * @param port порт подключения
 * @return     результат проверки
 */
bool awh::server::Proxy::acceptCallback(const string & ip, const string & mac, const u_int port) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("accept"))
		// Выполняем функцию обратного вызова
		return this->_callback.apply <bool, const string &, const string &, const u_int> ("accept", ip, mac, port);
	// Запрещаем выполнение подключения
	return false;
}
/**
 * activeCallback Метод идентификации активности на Web сервере
 * @param bid  идентификатор брокера (клиента)
 * @param mode режим события подключения
 */
void awh::server::Proxy::activeCallback(const uint64_t bid, const server::web_t::mode_t mode) noexcept {

}
/**
 * handshakeCallback Метод получения удачного запроса
 * @param sid   идентификатор потока
 * @param bid   идентификатор брокера
 * @param agent идентификатор агента клиента
 */
void awh::server::Proxy::handshakeCallback(const int32_t sid, const uint64_t bid, const server::web_t::agent_t agent) noexcept {

}
/**
 * requestCallback Метод запроса клиента
 * @param sid    идентификатор потока
 * @param bid    идентификатор брокера
 * @param method метод запроса
 * @param url    url-адрес запроса
 */
void awh::server::Proxy::requestCallback(const int32_t sid, const uint64_t bid, const awh::web_t::method_t method, const uri_t::url_t & url) noexcept {

}
/**
 * entityCallback Метод получения тела запроса
 * @param sid    идентификатор потока
 * @param bid    идентификатор брокера
 * @param method метод запроса
 * @param url    url-адрес запроса
 * @param entity тело запроса
 */
void awh::server::Proxy::entityCallback(const int32_t sid, const uint64_t bid, const awh::web_t::method_t method, const uri_t::url_t & url, const vector <char> & entity) noexcept {

}
/**
 * headersCallback Метод получения заголовков запроса
 * @param sid     идентификатор потока
 * @param bid     идентификатор брокера
 * @param method  метод запроса
 * @param url     url-адрес запроса
 * @param headers заголовки запроса
 */
void awh::server::Proxy::headersCallback(const int32_t sid, const uint64_t bid, const awh::web_t::method_t method, const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {

}
/**
 * proto Метод извлечения поддерживаемого протокола подключения
 * @param bid идентификатор брокера
 * @return    поддерживаемый протокол подключения (HTTP1_1, HTTP2)
 */
awh::engine_t::proto_t awh::server::Proxy::proto(const uint64_t bid) const noexcept {
	// Выполняем извлечение поддерживаемого протокола подключения
	return this->_server.proto(bid);
}
/**
 * parser Метод извлечения объекта HTTP-парсера
 * @param bid идентификатор брокера
 * @return    объект HTTP-парсера
 */
const awh::http_t * awh::server::Proxy::parser(const uint64_t bid) const noexcept {
	// Выполняем извлечение объекта HTTP-парсера
	return this->_server.parser(bid);
}
/**
 * init Метод инициализации PROXY-сервера
 * @param socket     unix-сокет для биндинга
 * @param compressor поддерживаемый компрессор
 */
void awh::server::Proxy::init(const string & socket, const http_t::compress_t compressor) noexcept {
	// Выполняем инициализацию PROXY-сервера
	this->_server.init(socket);
}
/**
 * init Метод инициализации PROXY-сервера
 * @param port       порт сервера
 * @param host       хост сервера
 * @param compressor поддерживаемый компрессор
 */
void awh::server::Proxy::init(const u_int port, const string & host, const http_t::compress_t compressor) noexcept {
	// Выполняем инициализацию PROXY-сервера
	this->_server.init(port, host);
}
/**
 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const uint64_t, const web_t::mode_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_server.on(callback);
}
/**
 * on Метод установки функции обратного вызова для извлечения пароля
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <string (const uint64_t, const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <string (const uint64_t, const string &)> ("extractPassword", callback);
}
/**
 * on Метод установки функции обратного вызова для обработки авторизации
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <bool (const uint64_t, const string &, const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <bool (const uint64_t, const string &, const string &)> ("checkPassword", callback);
}
/**
 * on Метод установки функции обратного вызова получения событий запуска и остановки сетевого ядра
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const awh::core_t::status_t, awh::core_t *)> callback) noexcept {

}
/**
 * on Метод установки функции обратного вызова для перехвата полученных чанков
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const uint64_t, const vector <char> &, const awh::http_t *)> callback) noexcept {

}
/**
 * on Метод установки функции обратного вызова на событие активации брокера на сервере
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <bool (const string &, const string &, const u_int)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <bool (const string &, const string &, const u_int)> ("accept", callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибок
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const uint64_t, const u_int, const string &)> callback) noexcept {

}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const uint64_t, const vector <char> &, const bool)> callback) noexcept {

}
/**
 * on Метод установки функции обратного вызова на событие получения ошибки
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> callback) noexcept {

}
/**
 * port Метод получения порта подключения брокера
 * @param bid идентификатор брокера
 * @return    порт подключения брокера
 */
u_int awh::server::Proxy::port(const uint64_t bid) const noexcept {
	// Выполняем получение порта подключения брокера
	return this->_server.port(bid);
}
/**
 * ip Метод получения IP-адреса брокера
 * @param bid идентификатор брокера
 * @return    адрес интернет подключения брокера
 */
const string & awh::server::Proxy::ip(const uint64_t bid) const noexcept {
	// Выполняем получение IP-адреса брокера
	return this->_server.ip(bid);
}
/**
 * mac Метод получения MAC-адреса брокера
 * @param bid идентификатор брокера
 * @return    адрес устройства брокера
 */
const string & awh::server::Proxy::mac(const uint64_t bid) const noexcept {
	// Выполняем получение MAC-адреса брокера
	return this->_server.mac(bid);
}
/**
 * stop Метод остановки сервера
 */
void awh::server::Proxy::stop() noexcept {
	// Выполняем остановку сервера
	this->_server.stop();
}
/**
 * start Метод запуска сервера
 */
void awh::server::Proxy::start() noexcept {
	// Выполняем запуск сервера
	this->_server.start();
}
/**
 * close Метод закрытия подключения брокера
 * @param bid идентификатор брокера
 */
void awh::server::Proxy::close(const uint64_t bid) noexcept {
	// Выполняем закрытие подключения брокера
	this->_server.close(bid);
}
/**
 * total Метод установки максимального количества одновременных подключений
 * @param total максимальное количество одновременных подключений
 */
void awh::server::Proxy::total(const u_short total) noexcept {
	// Выполняем установку максимального количества одновременных подключений
	this->_server.total(total);
}
/**
 * hosts Метод загрузки файла со списком хостов
 * @param filename адрес файла для загрузки
 */
void awh::server::Proxy::hosts(const string & filename) noexcept {
	// Выполняем загрузку файла со списком хостов
	this->_server.hosts(filename);
}
/**
 * clusterAutoRestart Метод установки флага перезапуска процессов
 * @param mode флаг перезапуска процессов
 */
void awh::server::Proxy::clusterAutoRestart(const bool mode) noexcept {
	// Выполняем установку флага перезапуска процессов
	this->_server.clusterAutoRestart(mode);
}
/**
 * user Метод установки параметров авторизации
 * @param login    логин пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 */
void awh::server::Proxy::user(const string & login, const string & password) noexcept {

}
/**
 * keepAlive Метод установки жизни подключения
 * @param cnt   максимальное количество попыток
 * @param idle  интервал времени в секундах через которое происходит проверка подключения
 * @param intvl интервал времени в секундах между попытками
 */
void awh::server::Proxy::keepAlive(const int cnt, const int idle, const int intvl) noexcept {
	// Выполняем установку жизни подключения
	this->_server.keepAlive(cnt, idle, intvl);
}
/**
 * compressors Метод установки списка поддерживаемых компрессоров
 * @param compressors список поддерживаемых компрессоров
 */
void awh::server::Proxy::compressors(const vector <http_t::compress_t> & compressors) noexcept {
	// Выполняем установку списка поддерживаемых компрессоров
	this->_server.compressors(compressors);
}
/**
 * mode Метод установки флагов настроек модуля
 * @param flags список флагов настроек модуля для установки
 */
void awh::server::Proxy::mode(const set <web_t::flag_t> & flags) noexcept {

}
/**
 * addOrigin Метод добавления разрешённого источника
 * @param origin разрешённый источнико
 */
void awh::server::Proxy::addOrigin(const string & origin) noexcept {
	// Выполняем добавление разрешённого источника
	this->_server.addOrigin(origin);
}
/**
 * setOrigin Метод установки списка разрешённых источников
 * @param origins список разрешённых источников
 */
void awh::server::Proxy::setOrigin(const vector <string> & origins) noexcept {
	// Выполняем установку списка разрешённых источников
	this->_server.setOrigin(origins);
}
/**
 * addAltSvc Метод добавления альтернативного сервиса
 * @param origin название альтернативного сервиса
 * @param field  поле альтернативного сервиса
 */
void awh::server::Proxy::addAltSvc(const string & origin, const string & field) noexcept {
	// Выполняем добавление альтернативного сервиса
	this->_server.addAltSvc(origin, field);
}
/**
 * setAltSvc Метод установки списка разрешённых источников
 * @param origins список альтернативных сервисов
 */
void awh::server::Proxy::setAltSvc(const unordered_multimap <string, string> & origins) noexcept {
	// Выполняем установку списка разрешённых источников
	this->_server.setAltSvc(origins);
}
/**
 * settings Модуль установки настроек протокола HTTP/2
 * @param settings список настроек протокола HTTP/2
 */
void awh::server::Proxy::settings(const map <web2_t::settings_t, uint32_t> & settings) noexcept {
	// Выполняем установку настроек протокола HTTP/2
	this->_server.settings(settings);
}
/**
 * realm Метод установки название сервера
 * @param realm название сервера
 */
void awh::server::Proxy::realm(const string & realm) noexcept {
	// Выполняем установку названия сервера
	this->_server.realm(realm);
}
/**
 * opaque Метод установки временного ключа сессии сервера
 * @param opaque временный ключ сессии сервера
 */
void awh::server::Proxy::opaque(const string & opaque) noexcept {
	// Выполняем установку временного ключа сессии сервера
	this->_server.opaque(opaque);
}
/**
 * chunk Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::server::Proxy::chunk(const size_t size) noexcept {
	// Выполняем установку размера чанка
	this->_server.chunk(size);
}
/**
 * maxRequests Метод установки максимального количества запросов
 * @param max максимальное количество запросов
 */
void awh::server::Proxy::maxRequests(const size_t max) noexcept {
	// Выполняем установку максимального количества запросов
	this->_server.maxRequests(max);
}
/**
 * verifySSL Метод разрешающий или запрещающий, выполнять проверку соответствия, сертификата домену
 * @param mode флаг состояния разрешения проверки
 */
void awh::server::Proxy::verifySSL(const bool mode) noexcept {
	// Выполняем установку  разрешения выполнения проверки соответствия, сертификата домену
	this->_core.verifySSL(mode);
}
/**
 * ciphers Метод установки алгоритмов шифрования
 * @param ciphers список алгоритмов шифрования для установки
 */
void awh::server::Proxy::ciphers(const vector <string> & ciphers) noexcept {
	// Выполняем установку алгоритмов шифрования
	this->_core.ciphers(ciphers);
}
/**
 * ca Метод установки доверенного сертификата (CA-файла)
 * @param trusted адрес доверенного сертификата (CA-файла)
 * @param path    адрес каталога где находится сертификат (CA-файл)
 */
void awh::server::Proxy::ca(const string & trusted, const string & path) noexcept {
	// Выполняем установку доверенного сертификата (CA-файла)
	this->_core.ca(trusted, path);
}
/**
 * certificate Метод установки файлов сертификата
 * @param chain файл цепочки сертификатов
 * @param key   приватный ключ сертификата
 */
void awh::server::Proxy::certificate(const string & chain, const string & key) noexcept {
	// Если адрес файла сертификата и ключа передан
	if(!chain.empty() && !key.empty()){
		// Устанавливаем тип сокета TLS
		this->_core.sonet(awh::scheme_t::sonet_t::TLS);
		// Устанавливаем активный протокол подключения
		this->_core.proto(awh::engine_t::proto_t::HTTP2);
		// Устанавливаем SSL сертификаты сервера
		this->_core.certificate(chain, key);
	}
}
/**
 * alive Метод установки долгоживущего подключения
 * @param mode флаг долгоживущего подключения
 */
void awh::server::Proxy::alive(const bool mode) noexcept {
	// Устанавливаем долгоживущее подключения
	this->_server.alive(mode);
}
/**
 * alive Метод установки времени жизни подключения
 * @param time время жизни подключения
 */
void awh::server::Proxy::alive(const time_t time) noexcept {
	// Устанавливаем время жизни подключения
	this->_server.alive(time);
}
/**
 * alive Метод установки долгоживущего подключения
 * @param bid  идентификатор брокера
 * @param mode флаг долгоживущего подключения
 */
void awh::server::Proxy::alive(const uint64_t bid, const bool mode) noexcept {
	// Устанавливаем долгоживущее подключение
	this->_server.alive(bid, mode);
}
/**
 * identity Метод установки идентичности протокола модуля
 * @param identity идентичность протокола модуля
 */
void awh::server::Proxy::identity(const http_t::identity_t identity) noexcept {
	// Устанавливаем идентичности протокола модуля
	this->_server.identity(identity);
}
/**
 * waitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read  количество секунд для детекции по чтению
 * @param write количество секунд для детекции по записи
 */
void awh::server::Proxy::waitTimeDetect(const time_t read, const time_t write) noexcept {
	// Выполняем установку детекцию сообщений по количеству секунд
	this->_server.waitTimeDetect(read, write);
}
/**
 * bytesDetect Метод детекции сообщений по количеству байт
 * @param read  количество байт для детекции по чтению
 * @param write количество байт для детекции по записи
 */
void awh::server::Proxy::bytesDetect(const scheme_t::mark_t read, const scheme_t::mark_t write) noexcept {
	// Выполняем установку детекцию сообщений по количеству байт
	this->_server.bytesDetect(read, write);
}
/**
 * ipV6only Метод установки флага использования только сети IPv6
 * @param mode флаг для установки
 */
void awh::server::Proxy::ipV6only(const bool mode) noexcept {
	// Выполняем установку флага использования только сети IPv6
	this->_core.ipV6only(mode);
}
/**
 * sonet Метод установки типа сокета подключения
 * @param sonet тип сокета подключения (TCP / UDP / SCTP)
 */
void awh::server::Proxy::sonet(const scheme_t::sonet_t sonet) noexcept {
	// Выполняем установку типа сокета подключения
	this->_core.sonet(sonet);
}
/**
 * family Метод установки типа протокола интернета
 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
 */
void awh::server::Proxy::family(const scheme_t::family_t family) noexcept {
	// Выполняем установку типа протокола интернета
	this->_core.family(family);
}
/**
 * bandWidth Метод установки пропускной способности сети
 * @param bid   идентификатор брокера
 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
 */
void awh::server::Proxy::bandWidth(const size_t bid, const string & read, const string & write) noexcept {
	// Выполняем установку пропускной способности сети
	this->_core.bandWidth(bid, read, write);
}
/**
 * network Метод установки параметров сети
 * @param ips    список IP адресов компьютера с которых разрешено выходить в интернет
 * @param ns     список серверов имён, через которые необходимо производить резолвинг доменов
 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
 * @param sonet  тип сокета подключения (TCP / UDP)
 */
void awh::server::Proxy::network(const vector <string> & ips, const vector <string> & ns, const scheme_t::family_t family, const scheme_t::sonet_t sonet) noexcept {
	// Выполняем установку параметров сети
	this->_core.network(ips, family, sonet);
}
/**
 * userAgent Метод установки User-Agent для HTTP-запроса
 * @param userAgent агент пользователя для HTTP-запроса
 */
void awh::server::Proxy::userAgent(const string & userAgent) noexcept {

}
/**
 * ident Метод установки идентификации клиента
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::server::Proxy::ident(const string & id, const string & name, const string & ver) noexcept {
	// Выполняем установку идентификации клиента
	this->_server.ident(id, name, ver);
}
/**
 * proxy Метод установки прокси-сервера
 * @param uri    параметры прокси-сервера
 * @param family семейстово интернет протоколов (IPV4 / IPV6 / NIX)
 */
void awh::server::Proxy::proxy(const string & uri, const scheme_t::family_t family) noexcept {

}
/**
 * flushDNS Метод сброса кэша DNS-резолвера
 * @return результат работы функции
 */
bool awh::server::Proxy::flushDNS() noexcept {

}
/**
 * timeoutDNS Метод установки времени ожидания выполнения запроса
 * @param sec интервал времени выполнения запроса в секундах
 */
void awh::server::Proxy::timeoutDNS(const uint8_t sec) noexcept {

}
/**
 * timeToLiveDNS Метод установки времени жизни DNS-кэша
 * @param ttl время жизни DNS-кэша в миллисекундах
 */
void awh::server::Proxy::timeToLiveDNS(const time_t ttl) noexcept {

}
/**
 * prefixDNS Метод установки префикса переменной окружения для извлечения серверов имён
 * @param prefix префикс переменной окружения для установки
 */
void awh::server::Proxy::prefixDNS(const string & prefix) noexcept {

}
/**
 * clearDNSBlackList Метод очистки чёрного списка
 * @param domain доменное имя для которого очищается чёрный список
 */
void awh::server::Proxy::clearDNSBlackList(const string & domain) noexcept {

}
/**
 * delInDNSBlackList Метод удаления IP-адреса из чёрного списока
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для удаления из чёрного списка
 */
void awh::server::Proxy::delInDNSBlackList(const string & domain, const string & ip) noexcept {

}
/**
 * setToDNSBlackList Метод добавления IP-адреса в чёрный список
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для добавления в чёрный список
 */
void awh::server::Proxy::setToDNSBlackList(const string & domain, const string & ip) noexcept {

}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::server::Proxy::authType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Выполняем установку типа авторизации
	this->_server.authType(type, hash);
}
/**
 * crypted Метод получения флага шифрования
 * @param bid идентификатор брокера
 * @return    результат проверки
 */
bool awh::server::Proxy::crypted(const uint64_t bid) const noexcept {
	// Выполняем получение флага шифрования
	return this->_server.crypted(bid);
}
/**
 * encrypt Метод активации шифрования для клиента
 * @param bid  идентификатор брокера
 * @param mode флаг активации шифрования
 */
void awh::server::Proxy::encrypt(const uint64_t bid, const bool mode) noexcept {
	// Выполняем активацию шифрования для клиента
	this->_server.encrypt(bid, mode);
}
/**
 * encryption Метод активации шифрования
 * @param mode флаг активации шифрования
 */
void awh::server::Proxy::encryption(const bool mode) noexcept {
	// Выполянем активацию шифрования
	this->_server.encryption(mode);
}
/**
 * encryption Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::server::Proxy::encryption(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Выполняем установку параметров шифрования
	this->_server.encryption(pass, salt, cipher);
}
/**
 * Proxy Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::server::Proxy::Proxy(const fmk_t * fmk, const log_t * log) noexcept : _callback(log), _core(fmk, log), _server(&_core, fmk, log), _fmk(fmk), _log(log) {
	// Устанавливаем тип сокета TCP
	this->_core.sonet(awh::scheme_t::sonet_t::TCP);
	// Устанавливаем активный протокол подключения
	this->_core.proto(awh::engine_t::proto_t::HTTP1_1);
	// Выполняем установку идентичности протокола модуля
	this->_server.identity(awh::http_t::identity_t::PROXY);
	// Устанавливаем функцию извлечения пароля
	this->_server.on((function <string (const uint64_t, const string &)>) std::bind(&proxy_t::passwordCallback, this, _1, _2));
	// Устанавливаем функцию проверки авторизации
	this->_server.on((function <bool (const uint64_t, const string &, const string &)>) std::bind(&proxy_t::authCallback, this, _1, _2, _3));
	// Установливаем функцию обратного вызова на событие запуска или остановки подключения
	this->_server.on((function <void (const uint64_t, const server::web_t::mode_t)>) std::bind(&proxy_t::activeCallback, this, _1, _2));
	// Установливаем функцию обратного вызова на событие активации клиента на сервере
	this->_server.on((function <bool (const string &, const string &, const u_int)>) std::bind(&proxy_t::acceptCallback, this, _1, _2, _3));
	// Устанавливаем функцию обратного вызова при выполнении удачного рукопожатия
	this->_server.on((function <void (const int32_t, const uint64_t, const server::web_t::agent_t)>) std::bind(&proxy_t::handshakeCallback, this, _1, _2, _3));
	// Установливаем функцию обратного вызова на событие получения запроса
	this->_server.on((function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &)>) std::bind(&proxy_t::requestCallback, this, _1, _2, _3, _4));
	// Установливаем функцию обратного вызова на событие получения тела сообщения
	this->_server.on((function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &)>) std::bind(&proxy_t::entityCallback, this, _1, _2, _3, _4, _5));
	// Установливаем функцию обратного вызова на событие получения заголовки сообщения
	this->_server.on((function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)>) std::bind(&proxy_t::headersCallback, this, _1, _2, _3, _4, _5));
}
