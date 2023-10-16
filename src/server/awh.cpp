/**
 * @file: awh.cpp
 * @date: 2022-10-02
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
#include <server/awh.hpp>

/**
 * init Метод инициализации WEB-сервера
 * @param socket   unix-сокет для биндинга
 * @param compress метод сжатия передаваемых сообщений
 */
void awh::server::AWH::init(const string & socket, const http_t::compress_t compress) noexcept {
	// Выполняем инициализацию WEB-сервера
	this->_http.init(socket, compress);
}
/**
 * init Метод инициализации WEB-сервера
 * @param port     порт сервера
 * @param host     хост сервера
 * @param compress метод сжатия передаваемых сообщений
 */
void awh::server::AWH::init(const u_int port, const string & host, const http_t::compress_t compress) noexcept {
	// Выполняем инициализацию WEB-сервера
	this->_http.init(port, host, compress);
}
/**
 * sendError Метод отправки сообщения об ошибке
 * @param bid  идентификатор брокера
 * @param mess отправляемое сообщение об ошибке
 */
void awh::server::AWH::sendError(const uint64_t bid, const ws::mess_t & mess) noexcept {
	// Выполняем отправку сообещния об ошибке клиенту
	this->_http.sendError(bid, mess);
}
/**
 * sendMessage Метод отправки сообщения клиенту
 * @param bid     идентификатор брокера
 * @param message передаваемое сообщения в бинарном виде
 * @param text    данные передаются в текстовом виде
 */
void awh::server::AWH::sendMessage(const uint64_t bid, const vector <char> & message, const bool text) noexcept {
	// Выполняем отправку сообщения клиенту
	this->_http.sendMessage(bid, message, text);
}
/**
 * send Метод отправки тела сообщения на клиенту
 * @param id     идентификатор потока HTTP
 * @param bid    идентификатор брокера
 * @param buffer буфер бинарных данных передаваемых клиенту
 * @param size   размер сообщения в байтах
 * @param end    флаг последнего сообщения после которого поток закрывается
 * @return       результат отправки данных указанному клиенту
 */
bool awh::server::AWH::send(const int32_t id, const uint64_t bid, const char * buffer, const size_t size, const bool end) noexcept {
	// Выполняем отправку тела сообщения клиенту
	return this->_http.send(id, bid, buffer, size, end);
}
/**
 * send Метод отправки заголовков на клиенту
 * @param id      идентификатор потока HTTP
 * @param bid     идентификатор брокера
 * @param code    код сообщения для брокера
 * @param mess    отправляемое сообщение об ошибке
 * @param headers заголовки отправляемые клиенту
 * @param end     размер сообщения в байтах
 * @return        идентификатор нового запроса
 */
int32_t awh::server::AWH::send(const int32_t id, const uint64_t bid, const u_int code, const string & mess, const unordered_multimap <string, string> & headers, const bool end) noexcept {
	// Выполняем отправку заголовков сообщения клиенту
	return this->_http.send(id, bid, code, mess, headers, end);
}
/**
 * send Метод отправки сообщения брокеру
 * @param bid     идентификатор брокера
 * @param code    код сообщения для брокера
 * @param mess    отправляемое сообщение об ошибке
 * @param entity  данные полезной нагрузки (тело сообщения)
 * @param headers HTTP заголовки сообщения
 */
void awh::server::AWH::send(const uint64_t bid, const u_int code, const string & mess, const vector <char> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Выполняем отправку сообщения клиенту
	this->_http.send(bid, code, mess, entity, headers);
}
/**
 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
 * @param callback функция обратного вызова
 */
void awh::server::AWH::on(function <void (const uint64_t, const web_t::mode_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * on Метод установки функции обратного вызова для извлечения пароля
 * @param callback функция обратного вызова
 */
void awh::server::AWH::on(function <string (const uint64_t, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * on Метод установки функции обратного вызова для обработки авторизации
 * @param callback функция обратного вызова
 */
void awh::server::AWH::on(function <bool (const uint64_t, const string &, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * on Метод установки функции обратного вызова получения событий запуска и остановки сетевого ядра
 * @param callback функция обратного вызова
 */
void awh::server::AWH::on(function <void (const awh::core_t::status_t, awh::core_t *)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * on Метод установки функции обратного вызова для перехвата полученных чанков
 * @param callback функция обратного вызова
 */
void awh::server::AWH::on(function <void (const uint64_t, const vector <char> &, const awh::http_t *)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие активации брокера на сервере
 * @param callback функция обратного вызова
 */
void awh::server::AWH::on(function <bool (const string &, const string &, const u_int)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибок
 * @param callback функция обратного вызова
 */
void awh::server::AWH::on(function <void (const uint64_t, const u_int, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений
 * @param callback функция обратного вызова
 */
void awh::server::AWH::on(function <void (const uint64_t, const vector <char> &, const bool)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибки
 * @param callback функция обратного вызова
 */
void awh::server::AWH::on(function <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * on Метод установки функция обратного вызова активности потока
 * @param callback функция обратного вызова
 */
void awh::server::AWH::on(function <void (const int32_t, const uint64_t, const web_t::mode_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * on Метод установки функция обратного вызова при полном получении запроса клиента
 * @param callback функция обратного вызова
 */
void awh::server::AWH::on(function <void (const int32_t, const uint64_t, const web_t::agent_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * on Метод установки функции обратного вызова при завершении запроса
 * @param callback функция обратного вызова
 */
void awh::server::AWH::on(function <void (const int32_t, const uint64_t, const web_t::direct_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * on Метод установки функции вывода полученного чанка бинарных данных с клиента
 * @param callback функция обратного вызова
 */
void awh::server::AWH::on(function <void (const int32_t, const uint64_t, const vector <char> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * on Метод установки функции вывода полученного заголовка с клиента
 * @param callback функция обратного вызова
 */
void awh::server::AWH::on(function <void (const int32_t, const uint64_t, const string &, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * on Метод установки функции вывода запроса клиента к серверу
 * @param callback функция обратного вызова
 */
void awh::server::AWH::on(function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * on Метод установки функции вывода полученного тела данных с клиента
 * @param callback функция обратного вызова
 */
void awh::server::AWH::on(function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * on Метод установки функции вывода полученных заголовков с клиента
 * @param callback функция обратного вызова
 */
void awh::server::AWH::on(function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * port Метод получения порта подключения брокера
 * @param bid идентификатор брокера
 * @return    порт подключения брокера
 */
u_int awh::server::AWH::port(const uint64_t bid) const noexcept {
	// Выполняем извлечение порта брокера
	return this->_http.port(bid);
}
/**
 * agent Метод извлечения агента клиента
 * @param bid идентификатор брокера
 * @return    агент к которому относится подключённый клиент
 */
awh::server::web_t::agent_t awh::server::AWH::agent(const uint64_t bid) const noexcept {
	// Выводим идентификатор агента к которому относится клиент
	return this->_http.agent(bid);
}
/**
 * ip Метод получения IP-адреса брокера
 * @param bid идентификатор брокера
 * @return    адрес интернет подключения брокера
 */
const string & awh::server::AWH::ip(const uint64_t bid) const noexcept {
	// Выполняем извлечение IP-адреса брокера
	return this->_http.ip(bid);
}
/**
 * mac Метод получения MAC-адреса брокера
 * @param bid идентификатор брокера
 * @return    адрес устройства брокера
 */
const string & awh::server::AWH::mac(const uint64_t bid) const noexcept {
	// Выполняем извлечение MAC-адреса брокера
	return this->_http.mac(bid);
}
/**
 * stop Метод остановки сервера
 */
void awh::server::AWH::stop() noexcept {
	// Выполняем остановку сервера
	this->_http.stop();
}
/**
 * start Метод запуска сервера
 */
void awh::server::AWH::start() noexcept {
	// Выполняем запуск сервера
	this->_http.start();
}
/**
 * close Метод закрытия подключения брокера
 * @param bid идентификатор брокера
 */
void awh::server::AWH::close(const uint64_t bid) noexcept {
	// Выполняем закрытие подключения брокера
	this->_http.close(bid);
}
/**
 * subprotocol Метод установки поддерживаемого сабпротокола
 * @param subprotocol сабпротокол для установки
 */
void awh::server::AWH::subprotocol(const string & subprotocol) noexcept {
	// Выполняем установку поддерживаемого сабпротокола
	this->_http.subprotocol(subprotocol);
}
/**
 * subprotocols Метод установки списка поддерживаемых сабпротоколов
 * @param subprotocols сабпротоколы для установки
 */
void awh::server::AWH::subprotocols(const set <string> & subprotocols) noexcept {
	// Выполняем установку списка поддерживаемых сабпротоколов
	this->_http.subprotocols(subprotocols);
}
/**
 * subprotocol Метод получения списка выбранных сабпротоколов
 * @param bid идентификатор брокера
 * @return    список выбранных сабпротоколов
 */
const set <string> & awh::server::AWH::subprotocols(const uint64_t bid) const noexcept {
	// Выполняем извлечение списка выбранных сабпротоколов
	return this->_http.subprotocols(bid);
}
/**
 * extensions Метод установки списка расширений
 * @param extensions список поддерживаемых расширений
 */
void awh::server::AWH::extensions(const vector <vector <string>> & extensions) noexcept {
	// Выполняем установку списка поддерживаемых расширений
	this->_http.extensions(extensions);
}
/**
 * extensions Метод извлечения списка поддерживаемых расширений
 * @param bid идентификатор брокера
 * @return    список поддерживаемых расширений
 */
const vector <vector <string>> & awh::server::AWH::extensions(const uint64_t bid) const noexcept {
	// Выполняем извлечение списка поддерживаемых расширений
	return this->_http.extensions(bid);
}
/**
 * total Метод установки максимального количества одновременных подключений
 * @param total максимальное количество одновременных подключений
 */
void awh::server::AWH::total(const u_short total) noexcept {
	// Выполняем установку максимального количества одновременных подключений
	this->_http.total(total);
}
/**
 * segmentSize Метод установки размеров сегментов фрейма
 * @param size минимальный размер сегмента
 */
void awh::server::AWH::segmentSize(const size_t size) noexcept {
	// Выполняем установку размеров сегментов фрейма
	this->_http.segmentSize(size);
}
/**
 * clusterAutoRestart Метод установки флага перезапуска процессов
 * @param mode флаг перезапуска процессов
 */
void awh::server::AWH::clusterAutoRestart(const bool mode) noexcept {
	// Выполняем установки флага перезапуска процессов
	this->_http.clusterAutoRestart(mode);
}
/**
 * compress Метод установки метода сжатия
 * @param метод сжатия сообщений
 */
void awh::server::AWH::compress(const http_t::compress_t compress) noexcept {
	// Выполняем установку метода сжатия
	this->_http.compress(compress);
}
/**
 * keepAlive Метод установки жизни подключения
 * @param cnt   максимальное количество попыток
 * @param idle  интервал времени в секундах через которое происходит проверка подключения
 * @param intvl интервал времени в секундах между попытками
 */
void awh::server::AWH::keepAlive(const int cnt, const int idle, const int intvl) noexcept {
	// Выполняем установку жизни подключения
	this->_http.keepAlive(cnt, idle, intvl);
}
/**
 * mode Метод установки флагов настроек модуля
 * @param flags список флагов настроек модуля для установки
 */
void awh::server::AWH::mode(const set <web_t::flag_t> & flags) noexcept {
	// Выполняем установку флагов настроек модуля
	this->_http.mode(flags);
}
/**
 * setOrigin Метод установки списка разрешённых источников
 * @param origins список разрешённых источников
 */
void awh::server::AWH::setOrigin(const vector <string> & origins) noexcept {
	// Выполняем установку списка разрешённых источников
	this->_http.setOrigin(origins);
}
/**
 * sendOrigin Метод отправки списка разрешённых источников
 * @param bid     идентификатор брокера
 * @param origins список разрешённых источников
 */
void awh::server::AWH::sendOrigin(const uint64_t bid, const vector <string> & origins) noexcept {
	// Выполняем отправку списка разрешённых источников
	this->_http.sendOrigin(bid, origins);
}
/**
 * settings Модуль установки настроек протокола HTTP/2
 * @param settings список настроек протокола HTTP/2
 */
void awh::server::AWH::settings(const map <web2_t::settings_t, uint32_t> & settings) noexcept {
	// Выполняем установку списка настроек протокола HTTP/2
	this->_http.settings(settings);
}
/**
 * realm Метод установки название сервера
 * @param realm название сервера
 */
void awh::server::AWH::realm(const string & realm) noexcept {
	// Выполняем установку названия сервера
	this->_http.realm(realm);
}
/**
 * opaque Метод установки временного ключа сессии сервера
 * @param opaque временный ключ сессии сервера
 */
void awh::server::AWH::opaque(const string & opaque) noexcept {
	// Выполняем установку временного ключа сессии сервера
	this->_http.opaque(opaque);
}
/**
 * chunk Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::server::AWH::chunk(const size_t size) noexcept {
	// Выполняем установку размера чанка
	this->_http.chunk(size);
}
/**
 * maxRequests Метод установки максимального количества запросов
 * @param max максимальное количество запросов
 */
void awh::server::AWH::maxRequests(const size_t max) noexcept {
	// Выполняем установку максимального количества запросов
	this->_http.maxRequests(max);
}
/**
 * alive Метод установки долгоживущего подключения
 * @param mode флаг долгоживущего подключения
 */
void awh::server::AWH::alive(const bool mode) noexcept {
	// Устанавливаем долгоживущее подключение
	this->_http.alive(mode);
}
/**
 * alive Метод установки времени жизни подключения
 * @param time время жизни подключения
 */
void awh::server::AWH::alive(const time_t time) noexcept {
	// Устанавливаем время жизни подключения
	this->_http.alive(time);
}
/**
 * alive Метод установки долгоживущего подключения
 * @param bid  идентификатор брокера
 * @param mode флаг долгоживущего подключения
 */
void awh::server::AWH::alive(const uint64_t bid, const bool mode) noexcept {
	// Выполняем установку долгоживущего подключения
	this->_http.alive(bid, mode);
}
/**
 * waitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read  количество секунд для детекции по чтению
 * @param write количество секунд для детекции по записи
 */
void awh::server::AWH::waitTimeDetect(const time_t read, const time_t write) noexcept {
	// Выполняем установку детекции сообщений по количеству секунд
	this->_http.waitTimeDetect(read, write);
}
/**
 * bytesDetect Метод детекции сообщений по количеству байт
 * @param read  количество байт для детекции по чтению
 * @param write количество байт для детекции по записи
 */
void awh::server::AWH::bytesDetect(const scheme_t::mark_t read, const scheme_t::mark_t write) noexcept {
	// Выполняем установку детекции сообщений по количеству байт
	this->_http.bytesDetect(read, write);
}
/**
 * ident Метод установки идентификации сервера
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::server::AWH::ident(const string & id, const string & name, const string & ver) noexcept {
	// Выполняем установку идентификатора сервера
	this->_http.ident(id, name, ver);
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::server::AWH::authType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Выполняем установку типа авторизации
	this->_http.authType(type, hash);
}
/**
 * crypto Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::server::AWH::crypto(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Выполняем установку параметров шифрования
	this->_http.crypto(pass, salt, cipher);
}
