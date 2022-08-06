/**
 * @file: client.cpp
 * @date: 2021-12-19
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2021
 */

// Подключаем заголовочный файл
#include <socks5/client.hpp>

/**
 * reqCmd Метод получения бинарного буфера запроса
 */
void awh::client::Socks5::reqCmd() const noexcept {
	// Очищаем бинарный буфер данных
	this->_buffer.clear();
	// Если IP адрес или доменное имя установлены
	if(!this->_url.ip.empty() || !this->_url.domain.empty()){
		// Бинарные данные буфера
		const char * data = nullptr;
		// Получаем значение порта
		const uint16_t port = htons(this->_url.port);
		// Увеличиваем память на 4 октета
		this->_buffer.resize(sizeof(uint8_t) * 4, 0x0);
		// Устанавливаем версию протокола
		u_short offset = this->octet(VER);
		// Устанавливаем комманду запроса
		offset = this->octet((uint8_t) cmd_t::CONNECT, offset);
		// Устанавливаем RSV октет
		offset = this->octet(0x00, offset);
		// Если IP адрес получен
		if(!this->_url.ip.empty()){
			// Получаем бинарные буфер IP адреса
			const auto & ip = this->ipToHex(this->_url.ip, this->_url.family);
			// Если буфер IP адреса получен
			if(!ip.empty()){
				// Определяем тип подключения
				switch(this->_url.family){
					// Устанавливаем тип адреса [IPv4]
					case AF_INET: offset = this->octet((uint8_t) atyp_t::IPv4, offset); break;
					// Устанавливаем тип адреса [IPv6]
					case AF_INET6: offset = this->octet((uint8_t) atyp_t::IPv6, offset); break;
				}
				// Устанавливаем полученный буфер IP адреса
				this->_buffer.insert(this->_buffer.end(), ip.begin(), ip.end());
			}
		// Устанавливаем доменное имя
		} else {
			// Устанавливаем тип адреса [Доменное имя]
			offset = this->octet((uint8_t) atyp_t::DMNAME, offset);
			// Добавляем в буфер доменное имя
			this->text(this->_url.domain);
		}
		// Получаем бинарные данные порта
		data = (const char *) &port;
		// Устанавливаем порт запроса
		this->_buffer.insert(this->_buffer.end(), data, data + sizeof(port));
	}
}
/**
 * reqAuth Метод получения бинарного буфера авторизации на сервере
 */
void awh::client::Socks5::reqAuth() const noexcept {
	// Очищаем бинарный буфер данных
	this->_buffer.clear();
	// Если логин и пароль переданы
	if(!this->_login.empty() && !this->_pass.empty()){
		// Увеличиваем память на 4 октета
		this->_buffer.resize(sizeof(uint8_t), 0x0);
		// Устанавливаем версию протокола
		this->octet(AVER);
		// Добавляем в буфер логин пользователя
		this->text(this->_login);
		// Добавляем в буфер пароль пользователя
		this->text(this->_pass);
	}
}
/**
 * reqMethods Метод получения бинарного буфера опроса методов подключения
 */
void awh::client::Socks5::reqMethods() const noexcept {
	// Очищаем бинарный буфер данных
	this->_buffer.clear();
	// Увеличиваем память на 4 октета
	this->_buffer.resize(sizeof(uint8_t) * 4, 0x0);
	// Устанавливаем версию протокола
	u_short offset = this->octet(VER);
	// Устанавливаем количество методов авторизации
	offset = this->octet(2, offset);
	// Устанавливаем первый метод авторизации (без авторизации)
	offset = this->octet((uint8_t) method_t::NOAUTH, offset);
	// Устанавливаем второй метод авторизации (по паролю)
	offset = this->octet((uint8_t) method_t::PASSWD, offset);
}
/**
 * parse Метод парсинга входящих данных
 * @param buffer бинарный буфер входящих данных
 * @param size   размер бинарного буфера входящих данных
 */
void awh::client::Socks5::parse(const char * buffer, const size_t size) noexcept {
	// Очищаем буфер данных
	this->_buffer.clear();
	// Если данные буфера переданы
	if((buffer != nullptr) && (size > 0)){
		// Определяем текущий стейт
		switch((uint8_t) this->_state){
			// Если установлен стейт, выбора метода
			case (uint8_t) state_t::METHOD: {
				// Если данных достаточно для получения ответа
				if(size >= sizeof(resMet_t)){
					// Создаём объект данных метода
					resMet_t resp;
					// Выполняем чтение данных
					memcpy(&resp, buffer, sizeof(resp));
					// Если версия протокола соответствует
					if(resp.ver == (uint8_t) VER){
						// Если авторизацию производить не нужно
						if(resp.method == (uint8_t) method_t::NOAUTH){
							// Устанавливаем статус ожидания ответа
							this->_state = state_t::RESPONSE;
							// Формируем запрос для доступа к сайту
							this->reqCmd();
						// Если сервер требует авторизацию
						} else if(resp.method == (uint8_t) method_t::PASSWD) {
							// Проверяем установлен ли логин с паролем
							if(!this->_login.empty() && !this->_pass.empty()){
								// Устанавливаем статус ожидания ответа на авторизацию
								this->_state = state_t::AUTH;
								// Формируем запрос авторизации
								this->reqAuth();
							// Если логин и пароль не установлены
							} else {
								// Устанавливаем код сообщения
								this->_code = 0x10;
								// Устанавливаем статус ошибки
								this->_state = state_t::BROKEN;
							}
						// Если пришёл не поддерживаемый метод
						} else {
							// Устанавливаем код сообщения
							this->_code = 0x07;
							// Устанавливаем статус ошибки
							this->_state = state_t::BROKEN;
						}
					// Если версия прокси-сервера не соответствует
					} else {
						// Устанавливаем код сообщения
						this->_code = 0x11;
						// Устанавливаем статус ошибки
						this->_state = state_t::BROKEN;
					}
				}
			} break;
			// Если установлен стейт, ожидания ответа на авторизацию
			case (uint8_t) state_t::AUTH: {
				// Если данных достаточно для получения ответа
				if(size >= sizeof(resAuth_t)){
					// Создаём объект данных ответа
					resAuth_t resp;
					// Выполняем чтение данных
					memcpy(&resp, buffer, sizeof(resp));
					// Если версия протокола соответствует
					if(resp.ver == (uint8_t) AVER){
						// Если авторизация пройдера нуспешно
						if(resp.status == (uint8_t) rep_t::SUCCESS){
							// Устанавливаем статус ожидания ответа
							this->_state = state_t::RESPONSE;
							// Формируем запрос для доступа к сайту
							this->reqCmd();
						// Если авторизация не пройдена
						} else {
							// Устанавливаем код сообщения
							this->_code = 0x12;
							// Устанавливаем статус ошибки
							this->_state = state_t::BROKEN;
						}
					// Если версия прокси-сервера не соответствует
					} else {
						// Устанавливаем код сообщения
						this->_code = 0x13;
						// Устанавливаем статус ошибки
						this->_state = state_t::BROKEN;
					}
				}
			} break;
			// Если установлен стейт, ожидания ответа на запрос
			case (uint8_t) state_t::RESPONSE: {
				// Если данных достаточно для получения ответа
				if(size > sizeof(res_t)){
					// Создаём объект данных ответа
					res_t res;
					// Выполняем чтение данных
					memcpy(&res, buffer, sizeof(res));
					// Если версия протокола соответствует
					if(res.ver == (uint8_t) VER){
						// Если рукопожатие выполнено
						if(res.rep == (uint8_t) rep_t::SUCCESS){
							// Определяем тип адреса
							switch(res.atyp){
								// Получаем адрес IPv4
								case (uint8_t) atyp_t::IPv4: {
									// Если буфер пришел достаточного размера
									if(size >= (sizeof(res_t) + sizeof(ip_t))){
										// Создаём объект данных сервера
										ip_t server;
										// Копируем в буфер наши данные IP адреса
										memcpy(&server, buffer + sizeof(res_t), sizeof(server));
										// Выполняем получение IP адреса
										const string & ip = this->hexToIp((const char *) &server.host, sizeof(server.host), AF_INET);
										// Если IP адрес получен
										if(!ip.empty()){
											// Заменяем порт сервера
											server.port = ntohs(server.port);
											// Если включён режим отладки
											#if defined(DEBUG_MODE)
												// Выводим сообщение о данных сервера
												this->_log->print("%s %s:%u %s", log_t::flag_t::INFO, "socks5 proxy server", ip.c_str(), server.port, "is accepted");
											#endif
											// Устанавливаем стейт рукопожатия
											this->_state = state_t::HANDSHAKE;
										}
									}
								} break;
								// Получаем адрес IPv6
								case (uint8_t) atyp_t::IPv6: {
									// Если буфер пришел достаточного размера
									if(size >= (sizeof(res_t) + sizeof(ip_t))){
										// Создаём объект данных сервера
										ip_t server;
										// Копируем в буфер наши данные IP адреса
										memcpy(&server, buffer + sizeof(res_t), sizeof(server));
										// Выполняем получение IP адреса
										const string & ip = this->hexToIp((const char *) &server.host, sizeof(server.host), AF_INET6);
										// Если IP адрес получен
										if(!ip.empty()){
											// Заменяем порт сервера
											server.port = ntohs(server.port);
											// Если включён режим отладки
											#if defined(DEBUG_MODE)
												// Выводим сообщение о данных сервера
												this->_log->print("%s [%s]:%u %s", log_t::flag_t::INFO, "socks5 proxy server", ip.c_str(), server.port, "is accepted");
											#endif
											// Устанавливаем стейт рукопожатия
											this->_state = state_t::HANDSHAKE;
										}
									}
								} break;
								// Получаем адрес DMNAME
								case (uint8_t) atyp_t::DMNAME: {
									// Если буфер пришел достаточного размера
									if(size >= (sizeof(res_t) + sizeof(uint16_t))){
										// Извлекаем доменное имя
										const string & domain = this->text(buffer + sizeof(res_t), size);
										// Получаем размер смещения
										u_short offset = (sizeof(res_t) + sizeof(uint8_t) + domain.size());
										// Если доменное имя получено
										if(!domain.empty() && (size >= (offset + sizeof(uint16_t)))){
											// Создаём порт сервера
											uint16_t port = 0;
											// Выполняем извлечение порта сервера
											memcpy(&port, buffer + offset, sizeof(uint16_t));
											// Заменяем порт сервера
											port = ntohs(port);
											// Если включён режим отладки
											#if defined(DEBUG_MODE)
												// Выводим сообщение о данных сервера
												this->_log->print("%s %s:%u %s", log_t::flag_t::INFO, "socks5 proxy server", domain.c_str(), port, "is accepted");
											#endif
											// Устанавливаем стейт рукопожатия
											this->_state = state_t::HANDSHAKE;
										}
									}
								} break;
							}
						// Если авторизация не пройдена
						} else {
							// Устанавливаем код сообщения
							this->_code = res.rep;
							// Устанавливаем статус ошибки
							this->_state = state_t::BROKEN;
						}
					// Если версия прокси-сервера не соответствует
					} else {
						// Устанавливаем код сообщения
						this->_code = 0x11;
						// Устанавливаем статус ошибки
						this->_state = state_t::BROKEN;
					}
				}
			} break;
		}
	// Иначе подготавливаем буфер для запроса доступных методов
	} else if(this->_state == state_t::METHOD) this->reqMethods();
}
/**
 * reset Метод сброса собранных данных
 */
void awh::client::Socks5::reset() noexcept {
	// Выполняем сброс статуса ошибки
	this->_code = 0x00;
	// Выполняем очистку буфера данных
	this->_buffer.clear();
	// Выполняем сброс стейта
	this->_state = state_t::METHOD;
}
/**
 * clearUser Метод очистки списка пользователей
 */
void awh::client::Socks5::clearUser() noexcept {
	// Выполняем очистку пароля пользователя
	this->_pass.clear();
	// Выполняем очистку пользователя
	this->_login.clear();
}
/**
 * user Метод установки параметров авторизации
 * @param login логин пользователя для авторизации на сервере
 * @param pass  пароль пользователя для авторизации на сервере
 */
void awh::client::Socks5::user(const string & login, const string & pass) noexcept {
	// Устанавливаем пароль пользователя
	this->_pass = pass;
	// Устанавливаем данные пользователя
	this->_login = login;
}
