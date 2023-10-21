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
 * init Метод инициализации WebSocket-сервера
 * @param socket   unix-сокет для биндинга
 * @param compress метод сжатия передаваемых сообщений
 */
void awh::server::WebSocket::init(const string & socket, const http_t::compress_t compress) noexcept {
	// Выполняем инициализацию WebSocket-сервера
	this->_ws.init(socket, compress);
}
/**
 * init Метод инициализации WebSocket-сервера
 * @param port     порт сервера
 * @param host     хост сервера
 * @param compress метод сжатия передаваемых сообщений
 */
void awh::server::WebSocket::init(const u_int port, const string & host, const http_t::compress_t compress) noexcept {
	// Выполняем инициализацию WebSocket-сервера
	this->_ws.init(port, host, compress);
}
/**
 * sendError Метод отправки сообщения об ошибке
 * @param bid  идентификатор брокера
 * @param mess отправляемое сообщение об ошибке
 */
void awh::server::WebSocket::sendError(const uint64_t bid, const ws::mess_t & mess) noexcept {
	// Выполняем отправку сообщения об ошибке
	this->_ws.sendError(bid, mess);
}
/**
 * sendMessage Метод отправки сообщения клиенту
 * @param bid     идентификатор брокера
 * @param message передаваемое сообщения в бинарном виде
 * @param text    данные передаются в текстовом виде
 */
void awh::server::WebSocket::sendMessage(const uint64_t bid, const vector <char> & message, const bool text) noexcept {
	// Выполняем отправку сообщения клиенту
	this->_ws.sendMessage(bid, message, text);
}
/**
 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket::on(function <void (const uint64_t, const web_t::mode_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_ws.on(callback);
}
/**
 * on Метод установки функции обратного вызова для извлечения пароля
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket::on(function <string (const uint64_t, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_ws.on(callback);
}
/**
 * on Метод установки функции обратного вызова для обработки авторизации
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket::on(function <bool (const uint64_t, const string &, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_ws.on(callback);
}
/**
 * on Метод установки функции обратного вызова получения событий запуска и остановки сетевого ядра
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket::on(function <void (const awh::core_t::status_t, awh::core_t *)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_ws.on(callback);
}
/**
 * on Метод установки функции обратного вызова для перехвата полученных чанков
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket::on(function <void (const uint64_t, const vector <char> &, const awh::http_t *)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_ws.on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие активации брокера на сервере
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket::on(function <bool (const string &, const string &, const u_int)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_ws.on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибок
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket::on(function <void (const uint64_t, const u_int, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_ws.on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket::on(function <void (const uint64_t, const vector <char> &, const bool)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_ws.on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибки
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket::on(function <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_ws.on(callback);
}
/**
 * on Метод установки функция обратного вызова активности потока
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket::on(function <void (const int32_t, const uint64_t, const web_t::mode_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_ws.on(callback);
}
/**
 * on Метод установки функция обратного вызова при выполнении рукопожатия
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket::on(function <void (const int32_t, const uint64_t, const web_t::agent_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_ws.on(callback);
}
/**
 * on Метод установки функции обратного вызова при завершении запроса
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket::on(function <void (const int32_t, const uint64_t, const web_t::direct_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_ws.on(callback);
}
/**
 * on Метод установки функции вывода полученного чанка бинарных данных с клиента
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket::on(function <void (const int32_t, const uint64_t, const vector <char> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_ws.on(callback);
}
/**
 * on Метод установки функции вывода полученного заголовка с клиента
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket::on(function <void (const int32_t, const uint64_t, const string &, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_ws.on(callback);
}
/**
 * on Метод установки функции вывода запроса клиента к серверу
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket::on(function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_ws.on(callback);
}
/**
 * on Метод установки функции вывода полученного тела данных с клиента
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket::on(function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_ws.on(callback);
}
/**
 * on Метод установки функции вывода полученных заголовков с клиента
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket::on(function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_ws.on(callback);
}
/**
 * port Метод получения порта подключения брокера
 * @param bid идентификатор брокера
 * @return    порт подключения брокера
 */
u_int awh::server::WebSocket::port(const uint64_t bid) const noexcept {
	// Выполняем извлечение порта брокера
	return this->_ws.port(bid);
}
/**
 * ip Метод получения IP-адреса брокера
 * @param bid идентификатор брокера
 * @return    адрес интернет подключения брокера
 */
const string & awh::server::WebSocket::ip(const uint64_t bid) const noexcept {
	// Выполняем извлечение IP-адреса брокера
	return this->_ws.ip(bid);
}
/**
 * mac Метод получения MAC-адреса брокера
 * @param bid идентификатор брокера
 * @return    адрес устройства брокера
 */
const string & awh::server::WebSocket::mac(const uint64_t bid) const noexcept {
	// Выполняем извлечение MAC-адреса брокера
	return this->_ws.mac(bid);
}
/**
 * stop Метод остановки сервера
 */
void awh::server::WebSocket::stop() noexcept {
	// Выполняем остановку сервера
	this->_ws.stop();
}
/**
 * start Метод запуска сервера
 */
void awh::server::WebSocket::start() noexcept {
	// Выполняем запуск сервера
	this->_ws.start();
}
/**
 * close Метод закрытия подключения брокера
 * @param bid идентификатор брокера
 */
void awh::server::WebSocket::close(const uint64_t bid) noexcept {
	// Выполняем закрытие подключения брокера
	this->_ws.close(bid);
}
/**
 * subprotocol Метод установки поддерживаемого сабпротокола
 * @param subprotocol сабпротокол для установки
 */
void awh::server::WebSocket::subprotocol(const string & subprotocol) noexcept {
	// Выполняем установку сабпротокола поддерживаемого сервером
	this->_ws.subprotocol(subprotocol);
}
/**
 * subprotocols Метод установки списка поддерживаемых сабпротоколов
 * @param subprotocols сабпротоколы для установки
 */
void awh::server::WebSocket::subprotocols(const set <string> & subprotocols) noexcept {
	// Выполняем установку списка сабпротоколов поддерживаемых сервером
	this->_ws.subprotocols(subprotocols);
}
/**
 * subprotocol Метод получения списка выбранных сабпротоколов
 * @param bid идентификатор брокера
 * @return    список выбранных сабпротоколов
 */
const set <string> & awh::server::WebSocket::subprotocols(const uint64_t bid) const noexcept {
	// Выводим извлечение списка выбранных сабпротоколов
	return this->_ws.subprotocols(bid);
}
/**
 * extensions Метод установки списка расширений
 * @param extensions список поддерживаемых расширений
 */
void awh::server::WebSocket::extensions(const vector <vector <string>> & extensions) noexcept {
	// Выполняем установку списка поддерживаемых расширений
	this->_ws.extensions(extensions);
}
/**
 * extensions Метод извлечения списка расширений
 * @param bid идентификатор брокера
 * @return    список поддерживаемых расширений
 */
const vector <vector <string>> & awh::server::WebSocket::extensions(const uint64_t bid) const noexcept {
	// Выполняем извлечение списка расширений
	return this->_ws.extensions(bid);
}
/**
 * multiThreads Метод активации многопоточности
 * @param count количество потоков для активации
 * @param mode  флаг активации/деактивации мультипоточности
 */
void awh::server::WebSocket::multiThreads(const uint16_t count, const bool mode) noexcept {
	// Выполняем активацию многопоточности
	this->_ws.multiThreads(count, mode);
}
/**
 * total Метод установки максимального количества одновременных подключений
 * @param total максимальное количество одновременных подключений
 */
void awh::server::WebSocket::total(const u_short total) noexcept {
	// Выполняем установку максимального количества одновременных подключений
	this->_ws.total(total);
}
/**
 * segmentSize Метод установки размеров сегментов фрейма
 * @param size минимальный размер сегмента
 */
void awh::server::WebSocket::segmentSize(const size_t size) noexcept {
	// Выполняем установку размера сегментов фрейма
	this->_ws.segmentSize(size);
}
/**
 * clusterAutoRestart Метод установки флага перезапуска процессов
 * @param mode флаг перезапуска процессов
 */
void awh::server::WebSocket::clusterAutoRestart(const bool mode) noexcept {
	// Выполняем установку флага перезапуска процессов
	this->_ws.clusterAutoRestart(mode);
}
/**
 * compress Метод установки метода сжатия
 * @param метод сжатия сообщений
 */
void awh::server::WebSocket::compress(const http_t::compress_t compress) noexcept {
	// Выполняем установку метода сжатия
	this->_ws.compress(compress);
}
/**
 * keepAlive Метод установки жизни подключения
 * @param cnt   максимальное количество попыток
 * @param idle  интервал времени в секундах через которое происходит проверка подключения
 * @param intvl интервал времени в секундах между попытками
 */
void awh::server::WebSocket::keepAlive(const int cnt, const int idle, const int intvl) noexcept {
	// Выполняем установку жизни подключения
	this->_ws.keepAlive(cnt, idle, intvl);
}
/**
 * mode Метод установки флагов настроек модуля
 * @param flags список флагов настроек модуля для установки
 */
void awh::server::WebSocket::mode(const set <web_t::flag_t> & flags) noexcept {
	// Выполняем установку флагов настроек модуля
	this->_ws.mode(flags);
}
/**
 * realm Метод установки название сервера
 * @param realm название сервера
 */
void awh::server::WebSocket::realm(const string & realm) noexcept {
	// Выполняем установку названия сервера
	this->_ws.realm(realm);
}
/**
 * opaque Метод установки временного ключа сессии сервера
 * @param opaque временный ключ сессии сервера
 */
void awh::server::WebSocket::opaque(const string & opaque) noexcept {
	// Выполняем установку временного ключа сессии сервера
	this->_ws.opaque(opaque);
}
/**
 * chunk Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::server::WebSocket::chunk(const size_t size) noexcept {
	// Выполняем установку размера чанка
	this->_ws.chunk(size);
}
/**
 * maxRequests Метод установки максимального количества запросов
 * @param max максимальное количество запросов
 */
void awh::server::WebSocket::maxRequests(const size_t max) noexcept {
	// Выполняем установку максимального количества запросов
	this->_ws.maxRequests(max);
}
/**
 * alive Метод установки долгоживущего подключения
 * @param mode флаг долгоживущего подключения
 */
void awh::server::WebSocket::alive(const bool mode) noexcept {
	// Устанавливаем долгоживущее подключение
	this->_ws.alive(mode);
}
/**
 * alive Метод установки времени жизни подключения
 * @param time время жизни подключения
 */
void awh::server::WebSocket::alive(const time_t time) noexcept {
	// Устанавливаем время жизни подключения
	this->_ws.alive(time);
}
/**
 * setHeaders Метод установки списка заголовков
 * @param headers список заголовков для установки
 */
void awh::server::WebSocket::setHeaders(const unordered_multimap <string, string> & headers) noexcept {
	// Выполняем установку списка заголовков
	this->_ws.setHeaders(headers);
}
/**
 * waitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read  количество секунд для детекции по чтению
 * @param write количество секунд для детекции по записи
 */
void awh::server::WebSocket::waitTimeDetect(const time_t read, const time_t write) noexcept {
	// Выполняем установку детекции сообщений по количеству секунд
	this->_ws.waitTimeDetect(read, write);
}
/**
 * bytesDetect Метод детекции сообщений по количеству байт
 * @param read  количество байт для детекции по чтению
 * @param write количество байт для детекции по записи
 */
void awh::server::WebSocket::bytesDetect(const scheme_t::mark_t read, const scheme_t::mark_t write) noexcept {
	// Выполняем установку детекции сообщений по количеству байт
	this->_ws.bytesDetect(read, write);
}
/**
 * setOrigin Метод установки списка разрешенных источников
 * @param origins список разрешённых источников
 */
void awh::server::WebSocket::setOrigin(const vector <string> & origins) noexcept {
	// Выполняем установку списка разрешенных источников
	this->_ws.setOrigin(origins);
}
/**
 * sendOrigin Метод отправки списка разрешенных источников
 * @param bid     идентификатор брокера
 * @param origins список разрешённых источников
 */
void awh::server::WebSocket::sendOrigin(const uint64_t bid, const vector <string> & origins) noexcept {
	// Выполняем отправку списка разрешенных источников
	this->_ws.sendOrigin(bid, origins);
}
/**
 * settings Модуль установки настроек протокола HTTP/2
 * @param settings список настроек протокола HTTP/2
 */
void awh::server::WebSocket::settings(const map <web2_t::settings_t, uint32_t> & settings) noexcept {
	// Выполняем установку настроек протокола HTTP/2
	this->_ws.settings(settings);
}
/**
 * ident Метод установки идентификации сервера
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::server::WebSocket::ident(const string & id, const string & name, const string & ver) noexcept {
	// Выполняем установку идентификатора сервера
	this->_ws.ident(id, name, ver);
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::server::WebSocket::authType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Выполняем установку типа авторизации
	this->_ws.authType(type, hash);
}
/**
 * crypto Метод получения флага шифрования
 * @param bid идентификатор брокера
 * @return    результат проверки
 */
bool awh::server::WebSocket::crypto(const uint64_t bid) const noexcept {
	// Выводим установленный флаг шифрования
	return this->_ws.crypto(bid);
}
/**
 * crypto Метод активации шифрования для клиента
 * @param bid  идентификатор брокера
 * @param mode флаг активации шифрования
 */
void awh::server::WebSocket::crypto(const uint64_t bid, const bool mode) noexcept {
	// Выполняем установку флага шифрования для клиента
	this->_ws.crypto(bid, mode);
}
/**
 * crypto Метод активации шифрования
 * @param mode флаг активации шифрования
 */
void awh::server::WebSocket::crypto(const bool mode) noexcept {
	// Выполняем установку флага шифрования
	this->_ws.crypto(mode);
}
/**
 * crypto Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::server::WebSocket::crypto(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Выполняем установку параметров шифрования
	this->_ws.crypto(pass, salt, cipher);
}
