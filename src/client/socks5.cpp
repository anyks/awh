/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <client/socks5.hpp>

/**
 * reqCmd Метод получения бинарного буфера запроса
 * @return бинарный буфер запроса
 */
vector <char> awh::Socks5Client::reqCmd() const noexcept {
	// Результат работы функции
	vector <char> result;
	// Если IP адрес или доменное имя установлены
	if(!this->url.ip.empty() || !this->url.domain.empty()){
		// Бинарные данные буфера
		const char * data = nullptr;
		// Получаем значение порта
		const uint16_t port = this->url.port;
		// Увеличиваем память на 4 октета
		result.resize(sizeof(uint8_t) * 4, 0x0);
		// Устанавливаем версию протокола
		u_short offset = this->setOctet(result, VER);
		// Устанавливаем комманду запроса
		offset = this->setOctet(result, (uint8_t) cmd_t::CONNECT, offset);
		// Устанавливаем RSV октет
		offset = this->setOctet(result, 0x00, offset);
		// Если IP адрес получен
		if(!this->url.ip.empty()){
			// Получаем бинарные буфер IP адреса
			const auto & ip = this->ipToHex(this->url.ip, this->url.family);
			// Если буфер IP адреса получен
			if(!ip.empty()){
				// Определяем тип подключения
				switch(this->url.family){
					// Устанавливаем тип адреса [IPv4]
					case AF_INET: offset = this->setOctet(result, (uint8_t) atyp_t::IPv4, offset); break;
					// Устанавливаем тип адреса [IPv6]
					case AF_INET6: offset = this->setOctet(result, (uint8_t) atyp_t::IPv6, offset); break;
				}
				// Устанавливаем полученный буфер IP адреса
				result.insert(result.end(), ip.begin(), ip.end());
			}
		// Устанавливаем доменное имя
		} else {
			// Устанавливаем тип адреса [Доменное имя]
			offset = this->setOctet(result, (uint8_t) atyp_t::DMNAME, offset);
			// Получаем размер доменного имени
			const uint8_t size = this->url.domain.size();
			// Получаем бинарные данные размера домена
			data = (const char *) &size;
			// Устанавливаем размер доменного имени
			result.insert(result.end(), data, data + sizeof(size));
			// Записываем бинарные данные домена
			result.insert(result.end(), this->url.domain.begin(), this->url.domain.end());
		}
		// Получаем бинарные данные порта
		data = (const char *) &port;
		// Устанавливаем порт запроса
		result.insert(result.end(), data, data + sizeof(port));
	}
	// Выводим результат
	return result;
}
/**
 * reqAuth Метод получения бинарного буфера авторизации на сервере
 * @return бинарный буфер запроса
 */
vector <char> awh::Socks5Client::reqAuth() const noexcept {
	// Результат работы функции
	vector <char> result;
	// Если логин и пароль переданы
	if(!this->login.empty() && !this->password.empty()){
		// Бинарные данные буфера
		const char * data = nullptr;
		// Увеличиваем память на 4 октета
		result.resize(sizeof(uint8_t), 0x0);
		// Устанавливаем версию протокола
		this->setOctet(result, VER);
		// Получаем размер логина пользователя
		const uint8_t userSize = this->login.size();
		// Получаем бинарные данные размера логина пользователя
		data = (const char *) &userSize;
		// Устанавливаем размер логина пользователя
		result.insert(result.end(), data, data + sizeof(userSize));
		// Устанавливаем логин пользователя
		result.insert(result.end(), this->login.begin(), this->login.end());
		// Получаем размер пароля пользователя
		const uint8_t passwordSize = this->password.size();
		// Получаем бинарные данные размера пароля пользователя
		data = (const char *) &passwordSize;
		// Устанавливаем размер пароля пользователя
		result.insert(result.end(), data, data + sizeof(passwordSize));
		// Устанавливаем пароль пользователя
		result.insert(result.end(), this->password.begin(), this->password.end());
	}
	// Выводим результат
	return result;
}
/**
 * reqMethods Метод получения бинарного буфера опроса методов подключения
 * @return бинарный буфер запроса
 */
vector <char> awh::Socks5Client::reqMethods() const noexcept {
	// Результат работы функции
	vector <char> result;
	// Увеличиваем память на 4 октета
	result.resize(sizeof(uint8_t) * 4, 0x0);
	// Устанавливаем версию протокола
	u_short offset = this->setOctet(result, VER);
	// Устанавливаем количество методов авторизации
	offset = this->setOctet(result, 2, offset);
	// Устанавливаем первый метод авторизации (без авторизации)
	offset = this->setOctet(result, (uint8_t) method_t::NOAUTH, offset);
	// Устанавливаем второй метод авторизации (по паролю)
	offset = this->setOctet(result, (uint8_t) method_t::PASSWD, offset);
	// Выводим результат
	return result;
}
/**
 * parse Метод парсинга входящих данных
 * @param buffer бинарный буфер входящих данных
 * @param size   размер бинарного буфера входящих данных
 */
void awh::Socks5Client::parse(const char * buffer, const size_t size) noexcept {

}
/**
 * reset Метод сброса собранных данных
 */
void awh::Socks5Client::reset() noexcept {

}
/**
 * clearUsers Метод очистки списка пользователей
 */
void awh::Socks5Client::clearUsers() noexcept {
	// Выполняем очистку пользователя
	this->login.clear();
	// Выполняем очистку пароля пользователя
	this->password.clear();
}
/**
 * setUser Метод установки параметров авторизации
 * @param login    логин пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 */
void awh::Socks5Client::setUser(const string & login, const string & password) noexcept {
	// Устанавливаем данные пользователя
	this->login = login;
	// Устанавливаем пароль пользователя
	this->password = password;
}
