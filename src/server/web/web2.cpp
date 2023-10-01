/**
 * @file: web2.cpp
 * @date: 2022-10-01
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
#include <server/web/web.hpp>

/**
 * sendSignal Метод обратного вызова при отправки данных HTTP/2
 * @param buffer буфер бинарных данных
 * @param size  размер буфера данных для отправки
 */
void awh::server::Web2::sendSignal(const uint8_t * buffer, const size_t size) noexcept {

}
/**
 * eventsCallback Функция обратного вызова при активации ядра сервера
 * @param status флаг запуска/остановки
 * @param core   объект сетевого ядра
 */
void awh::server::Web2::eventsCallback(const awh::core_t::status_t status, awh::core_t * core) noexcept {

}
/**
 * connectCallback Метод обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::Web2::connectCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {

}
/**
 * persistCallback Функция персистентного вызова
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::Web2::persistCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {

}
/**
 * implementation Метод выполнения активации сессии HTTP/2
 * @param aid  идентификатор адъютанта
 * @param core объект сетевого ядра
 */
void awh::server::Web2::implementation(const uint64_t aid, server::core_t * core) noexcept {

}
/**
 * ping Метод выполнения пинга клиента
 * @return результат работы пинга
 */
bool awh::server::Web2::ping() noexcept {

}
/**
 * send Метод отправки сообщения на сервер
 * @param id      идентификатор потока HTTP/2
 * @param message сообщение передаваемое на сервер
 * @param size    размер сообщения в байтах
 * @param end     флаг последнего сообщения после которого поток закрывается
 */
void awh::server::Web2::send(const int32_t id, const char * message, const size_t size, const bool end) noexcept {

}
/**
 * mode Метод установки флагов настроек модуля
 * @param flags список флагов настроек модуля для установки
 */
void awh::server::Web2::mode(const set <flag_t> & flags) noexcept {

}
/**
 * setOrigin Метод установки списка разрешенных источников
 * @param origins список разрешённых источников
 */
void awh::server::Web2::setOrigin(const vector <string> & origins) noexcept {

}
/**
 * sendOrigin Метод отправки списка разрешенных источников для
 * @param origins список разрешённых источников
 */
void awh::server::Web2::sendOrigin(const vector <string> & origins) noexcept {

}
/**
 * settings Модуль установки настроек протокола HTTP/2
 * @param settings список настроек протокола HTTP/2
 */
void awh::server::Web2::settings(const map <settings_t, uint32_t> & settings) noexcept {

}
/**
 * Web2 Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::server::Web2::Web2(const fmk_t * fmk, const log_t * log) noexcept {

}
/**
 * Web2 Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::server::Web2::Web2(const server::core_t * core, const fmk_t * fmk, const log_t * log) noexcept {

}
