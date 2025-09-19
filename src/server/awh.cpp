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
 * @copyright: Copyright © 2025
 */

/**
 * Подключаем заголовочный файл
 */
#include <server/awh.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * @brief Метод извлечения поддерживаемого протокола подключения
 *
 * @param bid идентификатор брокера
 * @return    поддерживаемый протокол подключения (HTTP1_1, HTTP2)
 */
awh::engine_t::proto_t awh::server::AWH::proto(const uint64_t bid) const noexcept {
	// Выполняем извлечения поддерживаемого протокола подключения
	return this->_http.proto(bid);
}
/**
 * @brief Метод извлечения объекта HTTP-парсера
 *
 * @param sid идентификатор потока
 * @param bid идентификатор брокера
 * @return    объект HTTP-парсера
 */
const awh::http_t * awh::server::AWH::parser(const int32_t sid, const uint64_t bid) const noexcept {
	// Выполняем извлечение объекта HTTP-парсера
	return this->_http.parser(sid, bid);
}
/**
 * @brief Метод получения запроса на передачу трейлеров
 *
 * @param bid идентификатор брокера
 * @param sid идентификатор потока
 * @return    флаг запроса клиентом передачи трейлеров
 */
bool awh::server::AWH::trailers(const int32_t sid, const uint64_t bid) const noexcept {
	// Выводим флаг запроса клиентом на передачу трейлеров
	return this->_http.trailers(sid, bid);
}
/**
 * @brief Метод установки трейлера
 *
 * @param sid идентификатор потока
 * @param bid идентификатор брокера
 * @param key ключ заголовка
 * @param val значение заголовка
 */
void awh::server::AWH::trailer(const int32_t sid, const uint64_t bid, const string & key, const string & val) noexcept {
	// Выполняем установку трейлера
	this->_http.trailer(sid, bid, key, val);
}
/**
 * @brief Метод инициализации WEB-сервера
 *
 * @param socket      unix-сокет для биндинга
 * @param compressors список поддерживаемых компрессоров
 */
void awh::server::AWH::init(const string & socket, const vector <http_t::compressor_t> & compressors) noexcept {
	// Выполняем инициализацию WEB-сервера
	this->_http.init(socket, compressors);
}
/**
 * @brief Метод инициализации WEB-сервера
 *
 * @param port        порт сервера
 * @param host        хост сервера
 * @param compressors список поддерживаемых компрессоров
 */
void awh::server::AWH::init(const uint32_t port, const string & host, const vector <http_t::compressor_t> & compressors) noexcept {
	// Выполняем инициализацию WEB-сервера
	this->_http.init(port, host, compressors);
}
/**
 * @brief Метод отправки сообщения об ошибке
 *
 * @param bid  идентификатор брокера
 * @param mess отправляемое сообщение об ошибке
 */
void awh::server::AWH::sendError(const uint64_t bid, const ws::mess_t & mess) noexcept {
	// Выполняем отправку сообещния об ошибке клиенту
	this->_http.sendError(bid, mess);
}
/**
 * @brief Метод отправки сообщения клиенту
 *
 * @param bid     идентификатор брокера
 * @param message передаваемое сообщения в бинарном виде
 * @param text    данные передаются в текстовом виде
 * @return        результат отправки сообщения
 */
bool awh::server::AWH::sendMessage(const uint64_t bid, const vector <char> & message, const bool text) noexcept {
	// Выполняем отправку сообщения клиенту
	return this->_http.sendMessage(bid, message, text);
}
/**
 * @brief Метод отправки сообщения на сервер
 *
 * @param bid     идентификатор брокера
 * @param message передаваемое сообщения в бинарном виде
 * @param size    размер передаваемого сообещния
 * @param text    данные передаются в текстовом виде
 * @return        результат отправки сообщения
 */
bool awh::server::AWH::sendMessage(const uint64_t bid, const char * message, const size_t size, const bool text) noexcept {
	// Выполняем отправку сообщения клиенту
	return this->_http.sendMessage(bid, message, size, text);
}
/**
 * @brief Метод отправки данных в бинарном виде клиенту
 *
 * @param bid    идентификатор брокера
 * @param buffer буфер бинарных данных передаваемых клиенту
 * @param size   размер сообщения в байтах
 * @return       результат отправки сообщения
 */
bool awh::server::AWH::send(const uint64_t bid, const char * buffer, const size_t size) noexcept {
	// Выполняем отправку данных в бинарном виде клиенту
	return this->_http.send(bid, buffer, size);
}
/**
 * @brief Метод отправки тела сообщения на клиенту
 *
 * @param sid    идентификатор потока HTTP
 * @param bid    идентификатор брокера
 * @param buffer буфер бинарных данных передаваемых клиенту
 * @param size   размер сообщения в байтах
 * @param end    флаг последнего сообщения после которого поток закрывается
 * @return       результат отправки данных указанному клиенту
 */
bool awh::server::AWH::send(const int32_t sid, const uint64_t bid, const char * buffer, const size_t size, const bool end) noexcept {
	// Выполняем отправку тела сообщения клиенту
	return this->_http.send(sid, bid, buffer, size, end);
}
/**
 * @brief Метод отправки заголовков на клиенту
 *
 * @param sid     идентификатор потока HTTP
 * @param bid     идентификатор брокера
 * @param code    код сообщения для брокера
 * @param mess    отправляемое сообщение об ошибке
 * @param headers заголовки отправляемые клиенту
 * @param end     размер сообщения в байтах
 * @return        идентификатор нового запроса
 */
int32_t awh::server::AWH::send(const int32_t sid, const uint64_t bid, const uint32_t code, const string & mess, const std::unordered_multimap <string, string> & headers, const bool end) noexcept {
	// Выполняем отправку заголовков сообщения клиенту
	return this->_http.send(sid, bid, code, mess, headers, end);
}
/**
 * @brief Метод отправки сообщения брокеру
 *
 * @param sid     идентификатор потока HTTP
 * @param bid     идентификатор брокера
 * @param code    код сообщения для брокера
 * @param mess    отправляемое сообщение об ошибке
 * @param buffer  данные полезной нагрузки (тело сообщения)
 * @param size    размер данных полезной нагрузки (размер тела сообщения)
 * @param headers HTTP заголовки сообщения
 */
void awh::server::AWH::send(const int32_t sid, const uint64_t bid, const uint32_t code, const string & mess, const char * buffer, const size_t size, const std::unordered_multimap <string, string> & headers) noexcept {
	// Выполняем отправку сообщения клиенту
	this->_http.send(sid, bid, code, mess, buffer, size, headers);
}
/**
 * @brief Метод отправки сообщения брокеру
 *
 * @param sid     идентификатор потока HTTP
 * @param bid     идентификатор брокера
 * @param code    код сообщения для брокера
 * @param mess    отправляемое сообщение об ошибке
 * @param entity  данные полезной нагрузки (тело сообщения)
 * @param headers HTTP заголовки сообщения
 */
void awh::server::AWH::send(const int32_t sid, const uint64_t bid, const uint32_t code, const string & mess, const vector <char> & entity, const std::unordered_multimap <string, string> & headers) noexcept {
	// Выполняем отправку сообщения клиенту
	this->_http.send(sid, bid, code, mess, entity, headers);
}
/**
 * @brief Метод HTTP/2 отправки клиенту сообщения корректного завершения
 *
 * @param bid идентификатор брокера
 * @return    результат выполнения операции
 */
bool awh::server::AWH::shutdown2(const uint64_t bid) noexcept {
	// Выполняем отправку клиенту сообщения корректного завершения
	return this->_http.shutdown2(bid);
}
/**
 * @brief Метод HTTP/2 выполнения сброса подключения
 *
 * @param sid   идентификатор потока
 * @param bid   идентификатор брокера
 * @param error код отправляемой ошибки
 * @return      результат отправки сообщения
 */
bool awh::server::AWH::reject2(const int32_t sid, const uint64_t bid, const awh::http2_t::error_t error) noexcept {
	// Выполняем сброс подключения
	return this->_http.reject2(sid, bid, error);
}
/**
 * @brief Метод HTTP/2 отправки сообщения закрытия всех потоков
 *
 * @param last   идентификатор последнего потока
 * @param bid    идентификатор брокера
 * @param error  код отправляемой ошибки
 * @param buffer буфер отправляемых данных если требуется
 * @param size   размер отправляемого буфера данных
 * @return       результат отправки данных фрейма
 */
bool awh::server::AWH::goaway2(const int32_t last, const uint64_t bid, const awh::http2_t::error_t error, const uint8_t * buffer, const size_t size) noexcept {
	// Выполняем отправку сообщения закрытия всех потоков
	return this->_http.goaway2(last, bid, error, buffer, size);
}
/**
 * @brief HTTP/2 Метод отправки трейлеров
 *
 * @param sid     идентификатор потока
 * @param bid     идентификатор брокера
 * @param headers заголовки отправляемые
 * @return        результат отправки данных указанному клиенту
 */
bool awh::server::AWH::send2(const int32_t sid, const uint64_t bid, const vector <std::pair <string, string>> & headers) noexcept {
	// Выполняем отправку трейлеров
	return this->_http.send2(sid, bid, headers);
}
/**
 * @brief HTTP/2 Метод отправки сообщения клиенту
 *
 * @param sid    идентификатор потока
 * @param bid    идентификатор брокера
 * @param buffer буфер бинарных данных передаваемых
 * @param size   размер сообщения в байтах
 * @param flag   флаг передаваемого потока по сети
 * @return       результат отправки данных указанному клиенту
 */
bool awh::server::AWH::send2(const int32_t sid, const uint64_t bid, const char * buffer, const size_t size, const awh::http2_t::flag_t flag) noexcept {
	// Выполняем отправку сообщения клиенту
	return this->_http.send2(sid, bid, buffer, size, flag);
}
/**
 * @brief HTTP/2 Метод отправки заголовков
 *
 * @param sid     идентификатор потока
 * @param bid     идентификатор брокера
 * @param headers заголовки отправляемые
 * @param flag    флаг передаваемого потока по сети
 * @return        флаг последнего сообщения после которого поток закрывается
 */
int32_t awh::server::AWH::send2(const int32_t sid, const uint64_t bid, const vector <std::pair <string, string>> & headers, const awh::http2_t::flag_t flag) noexcept {
	// Выполняем отправку заголовков
	return this->_http.send2(sid, bid, headers, flag);
}
/**
 * @brief HTTP/2 Метод отправки пуш-уведомлений
 *
 * @param sid     идентификатор потока
 * @param bid     идентификатор брокера
 * @param headers заголовки отправляемые
 * @param flag    флаг передаваемого потока по сети
 * @return        флаг последнего сообщения после которого поток закрывается
 */
int32_t awh::server::AWH::push2(const int32_t sid, const uint64_t bid, const vector <std::pair <string, string>> & headers, const awh::http2_t::flag_t flag) noexcept {
	// Выполняем отправку пуш-уведомлений
	return this->_http.push2(sid, bid, headers, flag);
}
/**
 * @brief Метод установки функций обратного вызова
 *
 * @param callback функции обратного вызова
 */
void awh::server::AWH::callback(const callback_t & callback) noexcept {
	// Выполняем установку функций обратного вызова
	this->_http.callback(callback);
}
/**
 * @brief Метод получения порта подключения брокера
 *
 * @param bid идентификатор брокера
 * @return    порт подключения брокера
 */
uint32_t awh::server::AWH::port(const uint64_t bid) const noexcept {
	// Выполняем извлечение порта брокера
	return this->_http.port(bid);
}
/**
 * @brief Метод извлечения агента клиента
 *
 * @param bid идентификатор брокера
 * @return    агент к которому относится подключённый клиент
 */
awh::server::web_t::agent_t awh::server::AWH::agent(const uint64_t bid) const noexcept {
	// Выводим идентификатор агента к которому относится клиент
	return this->_http.agent(bid);
}
/**
 * @brief Метод получения IP-адреса брокера
 *
 * @param bid идентификатор брокера
 * @return    адрес интернет подключения брокера
 */
const string & awh::server::AWH::ip(const uint64_t bid) const noexcept {
	// Выполняем извлечение IP-адреса брокера
	return this->_http.ip(bid);
}
/**
 * @brief Метод получения MAC-адреса брокера
 *
 * @param bid идентификатор брокера
 * @return    адрес устройства брокера
 */
const string & awh::server::AWH::mac(const uint64_t bid) const noexcept {
	// Выполняем извлечение MAC-адреса брокера
	return this->_http.mac(bid);
}
/**
 * @brief Метод остановки сервера
 *
 */
void awh::server::AWH::stop() noexcept {
	// Выполняем остановку сервера
	this->_http.stop();
}
/**
 * @brief Метод запуска сервера
 *
 */
void awh::server::AWH::start() noexcept {
	// Выполняем запуск сервера
	this->_http.start();
}
/**
 * @brief Метод закрытия подключения брокера
 *
 * @param bid идентификатор брокера
 */
void awh::server::AWH::close(const uint64_t bid) noexcept {
	// Выполняем закрытие подключения брокера
	this->_http.close(bid);
}
/**
 * @brief Метод установки времени ожидания ответа WebSocket-клиента
 *
 * @param sec время ожидания в секундах
 */
void awh::server::AWH::waitPong(const uint16_t sec) noexcept {
	// Выполняем установку времени ожидания
	this->_http.waitPong(sec);
}
/**
 * @brief Метод установки интервала времени выполнения пингов
 *
 * @param sec интервал времени выполнения пингов в секундах
 */
void awh::server::AWH::pingInterval(const uint16_t sec) noexcept {
	// Выполняем установку интервала времени выполнения пингов в секундах
	this->_http.pingInterval(sec);
}
/**
 * @brief Метод установки поддерживаемого сабпротокола
 *
 * @param subprotocol сабпротокол для установки
 */
void awh::server::AWH::subprotocol(const string & subprotocol) noexcept {
	// Выполняем установку поддерживаемого сабпротокола
	this->_http.subprotocol(subprotocol);
}
/**
 * @brief Метод установки списка поддерживаемых сабпротоколов
 *
 * @param subprotocols сабпротоколы для установки
 */
void awh::server::AWH::subprotocols(const std::unordered_set <string> & subprotocols) noexcept {
	// Выполняем установку списка поддерживаемых сабпротоколов
	this->_http.subprotocols(subprotocols);
}
/**
 * @brief Метод получения списка выбранных сабпротоколов
 *
 * @param bid идентификатор брокера
 * @return    список выбранных сабпротоколов
 */
const std::unordered_set <string> & awh::server::AWH::subprotocols(const uint64_t bid) const noexcept {
	// Выполняем извлечение списка выбранных сабпротоколов
	return this->_http.subprotocols(bid);
}
/**
 * @brief Метод установки списка расширений
 *
 * @param extensions список поддерживаемых расширений
 */
void awh::server::AWH::extensions(const vector <vector <string>> & extensions) noexcept {
	// Выполняем установку списка поддерживаемых расширений
	this->_http.extensions(extensions);
}
/**
 * @brief Метод извлечения списка поддерживаемых расширений
 *
 * @param bid идентификатор брокера
 * @return    список поддерживаемых расширений
 */
const vector <vector <string>> & awh::server::AWH::extensions(const uint64_t bid) const noexcept {
	// Выполняем извлечение списка поддерживаемых расширений
	return this->_http.extensions(bid);
}
/**
 * @brief Метод установки максимального количества одновременных подключений
 *
 * @param total максимальное количество одновременных подключений
 */
void awh::server::AWH::total(const uint16_t total) noexcept {
	// Выполняем установку максимального количества одновременных подключений
	this->_http.total(total);
}
/**
 * @brief Метод установки размеров сегментов фрейма
 *
 * @param size минимальный размер сегмента
 */
void awh::server::AWH::segmentSize(const size_t size) noexcept {
	// Выполняем установку размеров сегментов фрейма
	this->_http.segmentSize(size);
}
/**
 * @brief Метод загрузки файла со списком хостов
 *
 * @param filename адрес файла для загрузки
 */
void awh::server::AWH::hosts(const string & filename) noexcept {
	// Если адрес файла с хостами в операционной системе передан
	if(!filename.empty())
		// Выполняем установку адреса файла хостов в операционной системе
		this->_dns.hosts(filename);
}
/**
 * @brief Метод установки списка поддерживаемых компрессоров
 *
 * @param compressors список поддерживаемых компрессоров
 */
void awh::server::AWH::compressors(const vector <http_t::compressor_t> & compressors) noexcept {
	// Выполняем установку списка поддерживаемых компрессоров
	this->_http.compressors(compressors);
}
/**
 * @brief Метод установки жизни подключения
 *
 * @param cnt   максимальное количество попыток
 * @param idle  интервал времени в секундах через которое происходит проверка подключения
 * @param intvl интервал времени в секундах между попытками
 */
void awh::server::AWH::keepAlive(const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept {
	// Выполняем установку жизни подключения
	this->_http.keepAlive(cnt, idle, intvl);
}
/**
 * @brief Метод установки флагов настроек модуля
 *
 * @param flags список флагов настроек модуля для установки
 */
void awh::server::AWH::mode(const std::set <web_t::flag_t> & flags) noexcept {
	// Выполняем установку флагов настроек модуля
	this->_http.mode(flags);
}
/**
 * @brief Метод добавления разрешённого источника
 *
 * @param origin разрешённый источнико
 */
void awh::server::AWH::addOrigin(const string & origin) noexcept {
	// Выполняем добавление разрешённого источника
	this->_http.addOrigin(origin);
}
/**
 * @brief Метод установки списка разрешённых источников
 *
 * @param origins список разрешённых источников
 */
void awh::server::AWH::setOrigin(const vector <string> & origins) noexcept {
	// Выполняем установку разрешённых источников
	this->_http.setOrigin(origins);
}
/**
 * @brief Метод добавления альтернативного сервиса
 *
 * @param origin название альтернативного сервиса
 * @param field  поле альтернативного сервиса
 */
void awh::server::AWH::addAltSvc(const string & origin, const string & field) noexcept {
	// Выполняем добавление альтернативного сервиса
	this->_http.addAltSvc(origin, field);
}
/**
 * @brief Метод установки списка разрешённых источников
 *
 * @param origins список альтернативных сервисов
 */
void awh::server::AWH::setAltSvc(const std::unordered_multimap <string, string> & origins) noexcept {
	// Выполняем установку списка разрешённых источников
	this->_http.setAltSvc(origins);
}
/**
 * @brief Модуль установки настроек протокола HTTP/2
 *
 * @param settings список настроек протокола HTTP/2
 */
void awh::server::AWH::settings(const std::map <awh::http2_t::settings_t, uint32_t> & settings) noexcept {
	// Выполняем установку списка настроек протокола HTTP/2
	this->_http.settings(settings);
}
/**
 * @brief Метод установки название сервера
 *
 * @param realm название сервера
 */
void awh::server::AWH::realm(const string & realm) noexcept {
	// Выполняем установку названия сервера
	this->_http.realm(realm);
}
/**
 * @brief Метод установки временного ключа сессии сервера
 *
 * @param opaque временный ключ сессии сервера
 */
void awh::server::AWH::opaque(const string & opaque) noexcept {
	// Выполняем установку временного ключа сессии сервера
	this->_http.opaque(opaque);
}
/**
 * @brief Метод установки размера чанка
 *
 * @param size размер чанка для установки
 */
void awh::server::AWH::chunk(const size_t size) noexcept {
	// Выполняем установку размера чанка
	this->_http.chunk(size);
}
/**
 * @brief Метод установки максимального количества запросов
 *
 * @param max максимальное количество запросов
 */
void awh::server::AWH::maxRequests(const uint32_t max) noexcept {
	// Выполняем установку максимального количества запросов
	this->_http.maxRequests(max);
}
/**
 * @brief Метод установки долгоживущего подключения
 *
 * @param mode флаг долгоживущего подключения
 */
void awh::server::AWH::alive(const bool mode) noexcept {
	// Устанавливаем долгоживущее подключение
	this->_http.alive(mode);
}
/**
 * @brief Метод установки долгоживущего подключения
 *
 * @param bid  идентификатор брокера
 * @param mode флаг долгоживущего подключения
 */
void awh::server::AWH::alive(const uint64_t bid, const bool mode) noexcept {
	// Выполняем установку долгоживущего подключения
	this->_http.alive(bid, mode);
}
/**
 * @brief Метод установки идентичности протокола модуля
 *
 * @param identity идентичность протокола модуля
 */
void awh::server::AWH::identity(const http_t::identity_t identity) noexcept {
	// Выполняем установку идентичности протокола модуля
	this->_http.identity(identity);
}
/**
 * @brief Метод ожидания входящих сообщений
 *
 * @param sec интервал времени в секундах
 */
void awh::server::AWH::waitMessage(const uint16_t sec) noexcept {
	// Выполняем установку времени ожидания входящих сообщений
	this->_http.waitMessage(sec);
}
/**
 * @brief Метод детекции сообщений по количеству секунд
 *
 * @param read  количество секунд для детекции по чтению
 * @param write количество секунд для детекции по записи
 */
void awh::server::AWH::waitTimeDetect(const uint16_t read, const uint16_t write) noexcept {
	// Выполняем установку детекции сообщений по количеству секунд
	this->_http.waitTimeDetect(read, write);
}
/**
 * @brief Метод установки идентификации сервера
 *
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::server::AWH::ident(const string & id, const string & name, const string & ver) noexcept {
	// Выполняем установку идентификатора сервера
	this->_http.ident(id, name, ver);
}
/**
 * @brief Метод установки типа авторизации
 *
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::server::AWH::authType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Выполняем установку типа авторизации
	this->_http.authType(type, hash);
}
/**
 * @brief Метод получения флага шифрования
 *
 * @param sid идентификатор потока HTTP
 * @param bid идентификатор брокера
 * @return    результат проверки
 */
bool awh::server::AWH::crypted(const int32_t sid, const uint64_t bid) const noexcept {
	// Выводим установленный флаг шифрования
	return this->_http.crypted(sid, bid);
}
/**
 * @brief Метод активации шифрования для клиента
 *
 * @param sid  идентификатор потока HTTP
 * @param bid  идентификатор брокера
 * @param mode флаг активации шифрования
 */
void awh::server::AWH::encrypt(const int32_t sid, const uint64_t bid, const bool mode) noexcept {
	// Выполняем установку флага шифрования для клиента
	this->_http.encrypt(sid, bid, mode);
}
/**
 * @brief Метод активации шифрования
 *
 * @param mode флаг активации шифрования
 */
void awh::server::AWH::encryption(const bool mode) noexcept {
	// Выполняем установку флага шифрования
	this->_http.encryption(mode);
}
/**
 * @brief Метод установки параметров шифрования
 *
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::server::AWH::encryption(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Выполняем установку параметров шифрования
	this->_http.encryption(pass, salt, cipher);
}
/**
 * @brief Конструктор
 *
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::server::AWH::AWH(const server::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 _dns(fmk, log), _http(core, fmk, log), _fmk(fmk), _log(log) {
	// Выполняем установку DNS-резолвера
	const_cast <server::core_t *> (core)->resolver(&this->_dns);
}
