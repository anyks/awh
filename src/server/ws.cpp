/**
 * @file: ws.cpp
 * @date: 2022-10-04
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
#include <server/ws.hpp>

/**
 * proto Метод извлечения поддерживаемого протокола подключения
 * @param bid идентификатор брокера
 * @return    поддерживаемый протокол подключения (HTTP1_1, HTTP2)
 */
awh::engine_t::proto_t awh::server::Websocket::proto(const uint64_t bid) const noexcept {
	// Выполняем извлечения поддерживаемого протокола подключения
	return this->_ws.proto(bid);
}
/**
 * init Метод инициализации Websocket-сервера
 * @param socket      unix-сокет для биндинга
 * @param compressors список поддерживаемых компрессоров
 */
void awh::server::Websocket::init(const string & socket, const vector <http_t::compressor_t> & compressors) noexcept {
	// Выполняем инициализацию Websocket-сервера
	this->_ws.init(socket, compressors);
}
/**
 * init Метод инициализации Websocket-сервера
 * @param port        порт сервера
 * @param host        хост сервера
 * @param compressors список поддерживаемых компрессоров
 */
void awh::server::Websocket::init(const uint32_t port, const string & host, const vector <http_t::compressor_t> & compressors) noexcept {
	// Выполняем инициализацию Websocket-сервера
	this->_ws.init(port, host, compressors);
}
/**
 * sendError Метод отправки сообщения об ошибке
 * @param bid  идентификатор брокера
 * @param mess отправляемое сообщение об ошибке
 */
void awh::server::Websocket::sendError(const uint64_t bid, const ws::mess_t & mess) noexcept {
	// Выполняем отправку сообщения об ошибке
	this->_ws.sendError(bid, mess);
}
/**
 * sendMessage Метод отправки сообщения клиенту
 * @param bid     идентификатор брокера
 * @param message передаваемое сообщения в бинарном виде
 * @param text    данные передаются в текстовом виде
 * @return        результат отправки сообщения
 */
bool awh::server::Websocket::sendMessage(const uint64_t bid, const vector <char> & message, const bool text) noexcept {
	// Выполняем отправку сообщения клиенту
	return this->_ws.sendMessage(bid, message, text);
}
/**
 * callbacks Метод установки функций обратного вызова
 * @param callbacks функции обратного вызова
 */
void awh::server::Websocket::callbacks(const fn_t & callbacks) noexcept {
	// Выполняем установку функций обратного вызова
	this->_ws.callbacks(callbacks);
}
/**
 * port Метод получения порта подключения брокера
 * @param bid идентификатор брокера
 * @return    порт подключения брокера
 */
uint32_t awh::server::Websocket::port(const uint64_t bid) const noexcept {
	// Выполняем извлечение порта брокера
	return this->_ws.port(bid);
}
/**
 * ip Метод получения IP-адреса брокера
 * @param bid идентификатор брокера
 * @return    адрес интернет подключения брокера
 */
const string & awh::server::Websocket::ip(const uint64_t bid) const noexcept {
	// Выполняем извлечение IP-адреса брокера
	return this->_ws.ip(bid);
}
/**
 * mac Метод получения MAC-адреса брокера
 * @param bid идентификатор брокера
 * @return    адрес устройства брокера
 */
const string & awh::server::Websocket::mac(const uint64_t bid) const noexcept {
	// Выполняем извлечение MAC-адреса брокера
	return this->_ws.mac(bid);
}
/**
 * stop Метод остановки сервера
 */
void awh::server::Websocket::stop() noexcept {
	// Выполняем остановку сервера
	this->_ws.stop();
}
/**
 * start Метод запуска сервера
 */
void awh::server::Websocket::start() noexcept {
	// Выполняем запуск сервера
	this->_ws.start();
}
/**
 * close Метод закрытия подключения брокера
 * @param bid идентификатор брокера
 */
void awh::server::Websocket::close(const uint64_t bid) noexcept {
	// Выполняем закрытие подключения брокера
	this->_ws.close(bid);
}
/**
 * waitPong Метод установки времени ожидания ответа WebSocket-клиента
 * @param sec время ожидания в секундах
 */
void awh::server::Websocket::waitPong(const time_t sec) noexcept {
	// Выполняем установку времени ожидания
	this->_ws.waitPong(sec);
}
/**
 * pingInterval Метод установки интервала времени выполнения пингов
 * @param sec интервал времени выполнения пингов в секундах
 */
void awh::server::Websocket::pingInterval(const time_t sec) noexcept {
	// Выполняем установку интервала времени выполнения пингов в секундах
	this->_ws.pingInterval(sec);
}
/**
 * subprotocol Метод установки поддерживаемого сабпротокола
 * @param subprotocol сабпротокол для установки
 */
void awh::server::Websocket::subprotocol(const string & subprotocol) noexcept {
	// Выполняем установку сабпротокола поддерживаемого сервером
	this->_ws.subprotocol(subprotocol);
}
/**
 * subprotocols Метод установки списка поддерживаемых сабпротоколов
 * @param subprotocols сабпротоколы для установки
 */
void awh::server::Websocket::subprotocols(const set <string> & subprotocols) noexcept {
	// Выполняем установку списка сабпротоколов поддерживаемых сервером
	this->_ws.subprotocols(subprotocols);
}
/**
 * subprotocol Метод получения списка выбранных сабпротоколов
 * @param bid идентификатор брокера
 * @return    список выбранных сабпротоколов
 */
const set <string> & awh::server::Websocket::subprotocols(const uint64_t bid) const noexcept {
	// Выводим извлечение списка выбранных сабпротоколов
	return this->_ws.subprotocols(bid);
}
/**
 * extensions Метод установки списка расширений
 * @param extensions список поддерживаемых расширений
 */
void awh::server::Websocket::extensions(const vector <vector <string>> & extensions) noexcept {
	// Выполняем установку списка поддерживаемых расширений
	this->_ws.extensions(extensions);
}
/**
 * extensions Метод извлечения списка расширений
 * @param bid идентификатор брокера
 * @return    список поддерживаемых расширений
 */
const vector <vector <string>> & awh::server::Websocket::extensions(const uint64_t bid) const noexcept {
	// Выполняем извлечение списка расширений
	return this->_ws.extensions(bid);
}
/**
 * multiThreads Метод активации многопоточности
 * @param count количество потоков для активации
 * @param mode  флаг активации/деактивации мультипоточности
 */
void awh::server::Websocket::multiThreads(const uint16_t count, const bool mode) noexcept {
	// Выполняем активацию многопоточности
	this->_ws.multiThreads(count, mode);
}
/**
 * total Метод установки максимального количества одновременных подключений
 * @param total максимальное количество одновременных подключений
 */
void awh::server::Websocket::total(const uint16_t total) noexcept {
	// Выполняем установку максимального количества одновременных подключений
	this->_ws.total(total);
}
/**
 * segmentSize Метод установки размеров сегментов фрейма
 * @param size минимальный размер сегмента
 */
void awh::server::Websocket::segmentSize(const size_t size) noexcept {
	// Выполняем установку размера сегментов фрейма
	this->_ws.segmentSize(size);
}
/**
 * hosts Метод загрузки файла со списком хостов
 * @param filename адрес файла для загрузки
 */
void awh::server::Websocket::hosts(const string & filename) noexcept {
	// Если адрес файла с хостами в операционной системе передан
	if(!filename.empty())
		// Выполняем установку адреса файла хостов в операционной системе
		this->_dns.hosts(filename);
}
/**
 * compressors Метод установки списка поддерживаемых компрессоров
 * @param compressors список поддерживаемых компрессоров
 */
void awh::server::Websocket::compressors(const vector <http_t::compressor_t> & compressors) noexcept {
	// Выполняем установку список поддерживаемых компрессоров
	this->_ws.compressors(compressors);
}
/**
 * keepAlive Метод установки жизни подключения
 * @param cnt   максимальное количество попыток
 * @param idle  интервал времени в секундах через которое происходит проверка подключения
 * @param intvl интервал времени в секундах между попытками
 */
void awh::server::Websocket::keepAlive(const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept {
	// Выполняем установку жизни подключения
	this->_ws.keepAlive(cnt, idle, intvl);
}
/**
 * mode Метод установки флагов настроек модуля
 * @param flags список флагов настроек модуля для установки
 */
void awh::server::Websocket::mode(const set <web_t::flag_t> & flags) noexcept {
	// Выполняем установку флагов настроек модуля
	this->_ws.mode(flags);
}
/**
 * realm Метод установки название сервера
 * @param realm название сервера
 */
void awh::server::Websocket::realm(const string & realm) noexcept {
	// Выполняем установку названия сервера
	this->_ws.realm(realm);
}
/**
 * opaque Метод установки временного ключа сессии сервера
 * @param opaque временный ключ сессии сервера
 */
void awh::server::Websocket::opaque(const string & opaque) noexcept {
	// Выполняем установку временного ключа сессии сервера
	this->_ws.opaque(opaque);
}
/**
 * chunk Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::server::Websocket::chunk(const size_t size) noexcept {
	// Выполняем установку размера чанка
	this->_ws.chunk(size);
}
/**
 * maxRequests Метод установки максимального количества запросов
 * @param max максимальное количество запросов
 */
void awh::server::Websocket::maxRequests(const size_t max) noexcept {
	// Выполняем установку максимального количества запросов
	this->_ws.maxRequests(max);
}
/**
 * alive Метод установки долгоживущего подключения
 * @param mode флаг долгоживущего подключения
 */
void awh::server::Websocket::alive(const bool mode) noexcept {
	// Устанавливаем долгоживущее подключение
	this->_ws.alive(mode);
}
/**
 * alive Метод установки времени жизни подключения
 * @param sec время жизни подключения
 */
void awh::server::Websocket::alive(const time_t sec) noexcept {
	// Устанавливаем время жизни подключения
	this->_ws.alive(sec);
}
/**
 * waitMessage Метод ожидания входящих сообщений
 * @param sec интервал времени в секундах
 */
void awh::server::Websocket::waitMessage(const time_t sec) noexcept {
	// Выполняем установку времени ожидания входящих сообщений
	this->_ws.waitMessage(sec);
}
/**
 * setHeaders Метод установки списка заголовков
 * @param headers список заголовков для установки
 */
void awh::server::Websocket::setHeaders(const unordered_multimap <string, string> & headers) noexcept {
	// Выполняем установку списка заголовков
	this->_ws.setHeaders(headers);
}
/**
 * waitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read  количество секунд для детекции по чтению
 * @param write количество секунд для детекции по записи
 */
void awh::server::Websocket::waitTimeDetect(const time_t read, const time_t write) noexcept {
	// Выполняем установку детекции сообщений по количеству секунд
	this->_ws.waitTimeDetect(read, write);
}
/**
 * addOrigin Метод добавления разрешённого источника
 * @param origin разрешённый источнико
 */
void awh::server::Websocket::addOrigin(const string & origin) noexcept {
	// Выполняем добавление разрешённого источника
	this->_ws.addOrigin(origin);
}
/**
 * setOrigin Метод установки списка разрешённых источников
 * @param origins список разрешённых источников
 */
void awh::server::Websocket::setOrigin(const vector <string> & origins) noexcept {
	// Выполняем установку разрешённых источников
	this->_ws.setOrigin(origins);
}
/**
 * addAltSvc Метод добавления альтернативного сервиса
 * @param origin название альтернативного сервиса
 * @param field  поле альтернативного сервиса
 */
void awh::server::Websocket::addAltSvc(const string & origin, const string & field) noexcept {
	// Выполняем добавление альтернативного сервиса
	this->_ws.addAltSvc(origin, field);
}
/**
 * setAltSvc Метод установки списка разрешённых источников
 * @param origins список альтернативных сервисов
 */
void awh::server::Websocket::setAltSvc(const unordered_multimap <string, string> & origins) noexcept {
	// Выполняем установку списка разрешённых источников
	this->_ws.setAltSvc(origins);
}
/**
 * settings Модуль установки настроек протокола HTTP/2
 * @param settings список настроек протокола HTTP/2
 */
void awh::server::Websocket::settings(const map <awh::http2_t::settings_t, uint32_t> & settings) noexcept {
	// Выполняем установку настроек протокола HTTP/2
	this->_ws.settings(settings);
}
/**
 * ident Метод установки идентификации сервера
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::server::Websocket::ident(const string & id, const string & name, const string & ver) noexcept {
	// Выполняем установку идентификатора сервера
	this->_ws.ident(id, name, ver);
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::server::Websocket::authType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Выполняем установку типа авторизации
	this->_ws.authType(type, hash);
}
/**
 * crypted Метод получения флага шифрования
 * @param bid идентификатор брокера
 * @return    результат проверки
 */
bool awh::server::Websocket::crypted(const uint64_t bid) const noexcept {
	// Выводим установленный флаг шифрования
	return this->_ws.crypted(bid);
}
/**
 * encrypt Метод активации шифрования для клиента
 * @param bid  идентификатор брокера
 * @param mode флаг активации шифрования
 */
void awh::server::Websocket::encrypt(const uint64_t bid, const bool mode) noexcept {
	// Выполняем установку флага шифрования для клиента
	this->_ws.encrypt(bid, mode);
}
/**
 * encryption Метод активации шифрования
 * @param mode флаг активации шифрования
 */
void awh::server::Websocket::encryption(const bool mode) noexcept {
	// Выполняем установку флага шифрования
	this->_ws.encryption(mode);
}
/**
 * encryption Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::server::Websocket::encryption(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Выполняем установку параметров шифрования
	this->_ws.encryption(pass, salt, cipher);
}
/**
 * Websocket Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::server::Websocket::Websocket(const server::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 _dns(fmk, log), _ws(core, fmk, log), _fmk(fmk), _log(log) {
	// Выполняем установку DNS-резолвера
	const_cast <server::core_t *> (core)->resolver(&this->_dns);
}
