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
 * send Метод отправки сообщения адъютанту
 * @param aid     идентификатор адъютанта
 * @param code    код сообщения для адъютанта
 * @param mess    отправляемое сообщение об ошибке
 * @param entity  данные полезной нагрузки (тело сообщения)
 * @param headers HTTP заголовки сообщения
 */
void awh::server::AWH::send(const uint64_t aid, const u_int code, const string & mess, const vector <char> & entity, const unordered_multimap <string, string> & headers) const noexcept {
	// Выполняем отправку сообщения клиенту
	this->_http.send(aid, code, mess, entity, headers);
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
void awh::server::AWH::on(function <string (const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * on Метод установки функции обратного вызова для обработки авторизации
 * @param callback функция обратного вызова
 */
void awh::server::AWH::on(function <bool (const string &, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * on Метод установки функции обратного вызова для перехвата полученных чанков
 * @param callback функция обратного вызова
 */
void awh::server::AWH::on(function <void (const vector <char> &, const awh::http_t *)> callback) noexcept {
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
 * on Метод установки функции обратного вызова на событие активации адъютанта на сервере
 * @param callback функция обратного вызова
 */
void awh::server::AWH::on(function <bool (const string &, const string &, const u_int)> callback) noexcept {
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
 * on Метод установки функция обратного вызова при полном получении запроса клиента
 * @param callback функция обратного вызова
 */
void awh::server::AWH::on(function <void (const int32_t, const uint64_t)> callback) noexcept {
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
 * port Метод получения порта подключения адъютанта
 * @param aid идентификатор адъютанта
 * @return    порт подключения адъютанта
 */
u_int awh::server::AWH::port(const uint64_t aid) const noexcept {
	// Выполняем извлечение порта адъютанта
	return this->_http.port(aid);
}
/**
 * ip Метод получения IP-адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес интернет подключения адъютанта
 */
const string & awh::server::AWH::ip(const uint64_t aid) const noexcept {
	// Выполняем извлечение IP-адреса адъютанта
	return this->_http.ip(aid);
}
/**
 * mac Метод получения MAC-адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес устройства адъютанта
 */
const string & awh::server::AWH::mac(const uint64_t aid) const noexcept {
	// Выполняем извлечение MAC-адреса адъютанта
	return this->_http.mac(aid);
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
 * close Метод закрытия подключения адъютанта
 * @param aid идентификатор адъютанта
 */
void awh::server::AWH::close(const uint64_t aid) noexcept {
	// Выполняем закрытие подключения адъютанта
	this->_http.close(aid);
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
 * @param aid  идентификатор адъютанта
 * @param mode флаг долгоживущего подключения
 */
void awh::server::AWH::alive(const uint64_t aid, const bool mode) noexcept {
	// Выполняем установку долгоживущего подключения
	this->_http.alive(aid, mode);
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