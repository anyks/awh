/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <server/socks5.hpp>

/**
 * resCmd Метод получения бинарного буфера ответа
 */
void awh::Socks5Server::resCmd() const noexcept {

}
/**
 * resAuth Метод получения бинарного буфера ответа на авторизацию клиента
 */
void awh::Socks5Server::resAuth() const noexcept {

}
/**
 * resMethod Метод получения бинарного буфера выбора метода подключения
 */
void awh::Socks5Server::resMethod() const noexcept {

}
/**
 * parse Метод парсинга входящих данных
 * @param buffer бинарный буфер входящих данных
 * @param size   размер бинарного буфера входящих данных
 */
void awh::Socks5Server::parse(const char * buffer, const size_t size) noexcept {

}
/**
 * reset Метод сброса собранных данных
 */
void awh::Socks5Server::reset() noexcept {

}
/**
 * clearUsers Метод очистки списка пользователей
 */
void awh::Socks5Server::clearUsers() noexcept {

}
/**
 * setUsers Метод добавления списка пользователей
 * @param users список пользователей для добавления
 */
void awh::Socks5Server::setUsers(const unordered_map <string, string> & users) noexcept {

}
