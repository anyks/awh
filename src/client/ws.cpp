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
 * send Метод отправки сообщения на сервер
 * @param message буфер сообщения в бинарном виде
 * @param size    размер сообщения в байтах
 * @param utf8    данные передаются в текстовом виде
 */
void awh::client::WebSocket::send(const char * message, const size_t size, const bool utf8) noexcept {
	// Выполняем отправку сообщения на сервер
	this->_ws.send(message, size, utf8);
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
 * @param dest     адрес назначения удалённого сервера
 * @param compress метод компрессии передаваемых сообщений
 */
void awh::client::WebSocket::init(const string & dest, const awh::http_t::compress_t compress) noexcept {
	// Выполняем инициализацию клиента
	this->_ws.init(dest, compress);
}
/**
 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket::on(function <void (const web_t::mode_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_ws.on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибок
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket::on(function <void (const u_int, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_ws.on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket::on(function <void (const vector <char> &, const bool)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_ws.on(callback);
}
/**
 * on Метод установки функции обратного вызова получения событий запуска и остановки сетевого ядра
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket::on(function <void (const awh::core_t::status_t, awh::core_t *)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_ws.on(callback);
}
/**
 * on Метод установки функции обратного вызова для перехвата полученных чанков
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket::on(function <void (const vector <char> &, const awh::http_t *)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_ws.on(callback);
}
/**
 * on Метод установки функции вывода полученного чанка бинарных данных с сервера
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket::on(function <void (const int32_t, const vector <char> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_ws.on(callback);
}
/**
 * on Метод установки функция обратного вызова активности потока
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket::on(function <void (const int32_t, const web_t::mode_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_ws.on(callback);
}
/**
 * on Метод установки функции вывода ответа сервера на ранее выполненный запрос
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket::on(function <void (const int32_t, const u_int, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_ws.on(callback);
}
/**
 * on Метод установки функции вывода полученного заголовка с сервера
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket::on(function <void (const int32_t, const string &, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_ws.on(callback);
}
/**
 * on Метод установки функции вывода полученного тела данных с сервера
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket::on(function <void (const int32_t, const u_int, const string &, const vector <char> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_ws.on(callback);
}
/**
 * on Метод установки функции вывода полученных заголовков с сервера
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket::on(function <void (const int32_t, const u_int, const string &, const unordered_multimap <string, string> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_ws.on(callback);
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
 * sub Метод получения выбранного сабпротокола WebSocket
 * @return выбранный сабпротокол
 */
const string & awh::client::WebSocket::sub() const noexcept {
	// Выполняем извлечение выбранного сабпротокола
	return this->_ws.sub();
}
/**
 * sub Метод установки сабпротокола поддерживаемого сервером WebSocket
 * @param sub сабпротокол для установки
 */
void awh::client::WebSocket::sub(const string & sub) noexcept {
	// Выполняем установку сабпротокола
	this->_ws.sub(sub);
}
/**
 * subs Метод установки списка сабпротоколов поддерживаемых сервером WebSocket
 * @param subs сабпротоколы для установки
 */
void awh::client::WebSocket::subs(const vector <string> & subs) noexcept {
	// Выполняем установку списка сабпротоколов
	this->_ws.subs(subs);
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
 * mode Метод установки флагов настроек модуля
 * @param flags список флагов настроек модуля для установки
 */
void awh::client::WebSocket::mode(const set <web_t::flag_t> & flags) noexcept {
	// Выполняем установку флагов настроек модуля
	this->_ws.mode(flags);
}
/**
 * compress Метод установки метода компрессии
 * @param compress метод компрессии сообщений
 */
void awh::client::WebSocket::compress(const awh::http_t::compress_t compress) noexcept {
	// Выполняем установку метода компрессии
	this->_ws.compress(compress);
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
 * multiThreads Метод активации многопоточности в WebSocket
 * @param threads количество потоков для активации
 * @param mode    флаг активации/деактивации мультипоточности
 */
void awh::client::WebSocket::multiThreads(const size_t threads, const bool mode) noexcept {
	// Выполняем активацию многопоточности при получения данных в WebSocket
	this->_ws.multiThreads(threads, mode);
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
 * userAgent Метод установки User-Agent для HTTP запроса
 * @param userAgent агент пользователя для HTTP запроса
 */
void awh::client::WebSocket::userAgent(const string & userAgent) noexcept {
	// Выполняем установку User-Agent для HTTP запроса
	this->_ws.userAgent(userAgent);
}
/**
 * serv Метод установки данных сервиса
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::client::WebSocket::serv(const string & id, const string & name, const string & ver) noexcept {
	// Выполняем установку данных сервиса
	this->_ws.serv(id, name, ver);
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
 * crypto Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::client::WebSocket::crypto(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Выполняем установку параметров шифрования
	this->_ws.crypto(pass, salt, cipher);
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
