/**
 * @file: server.cpp
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
#include <socks5/server.hpp>

/**
 * server Метод извлечения параметров запрашиваемого сервера
 * @return параметры запрашиваемого сервера
 */
const awh::server::Socks5::serv_t & awh::server::Socks5::server() const noexcept {
	// Выводим запрашиваемый сервер
	return this->_server;
}
/**
 * cmd Метод получения бинарного буфера ответа
 * @param rep код ответа сервера
 */
void awh::server::Socks5::cmd(const rep_t rep) const noexcept {
	// Очищаем бинарный буфер данных
	this->_buffer.clear();
	// Если IP адрес или доменное имя установлены
	if(!this->_url.ip.empty() || !this->_url.domain.empty() || !this->_url.host.empty()){
		// Бинарные данные буфера
		const char * data = nullptr;
		// Получаем значение порта
		const uint16_t port = htons(this->_url.port);
		// Увеличиваем память на 4 октета
		this->_buffer.resize(sizeof(uint8_t) * 4, 0x0);
		// Устанавливаем версию протокола
		uint16_t offset = this->octet(VER);
		// Устанавливаем комманду ответа
		offset = this->octet(static_cast <uint8_t> (rep), offset);
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
					case AF_INET: offset = this->octet(static_cast <uint8_t> (atyp_t::IPv4), offset); break;
					// Устанавливаем тип адреса [IPv6]
					case AF_INET6: offset = this->octet(static_cast <uint8_t> (atyp_t::IPv6), offset); break;
				}
				// Устанавливаем полученный буфер IP адреса
				this->_buffer.insert(this->_buffer.end(), ip.begin(), ip.end());
			}
		// Устанавливаем доменное имя
		} else {
			// Устанавливаем тип адреса [Доменное имя]
			offset = this->octet(static_cast <uint8_t> (atyp_t::DMNAME), offset);
			// Добавляем в буфер доменное имя
			this->text(!this->_url.domain.empty() ? this->_url.domain : this->_url.host);
		}
		// Получаем бинарные данные порта
		data = (const char *) &port;
		// Устанавливаем порт ответа
		this->_buffer.insert(this->_buffer.end(), data, data + sizeof(port));
	}
}
/**
 * method Метод получения бинарного буфера выбора метода подключения
 * @param methods методы авторизаций выбранныйе пользователем
 */
void awh::server::Socks5::method(const vector <uint8_t> & methods) const noexcept {
	// Создаём объект ответа
	res_method_t response;
	// Устанавливаем версию прокси-протокола
	response.ver = VER;
	// Устанавливаем запрещённый метод авторизации
	response.method = static_cast <uint8_t> (method_t::NOMETHOD);
	// Если пользователь выбрал список методов
	if(!methods.empty()){
		// Переходим по всем методам авторизаций
		for(auto & method : methods){
			// Если метод авторизации выбран логин/пароль пользователя
			if(method == static_cast <uint8_t> (method_t::PASSWD)){
				// Если пользователи установлены
				if(this->_auth != nullptr){
					// Устанавливаем метод прокси-сервера
					response.method = static_cast <uint8_t> (method);
					// Выходим из цикла
					break;
				}
			// Если пользователь выбрал метод без авторизации
			} else if(method == static_cast <uint8_t> (method_t::NOAUTH)) {
				// Если пользователи не установлены
				if(this->_auth == nullptr){
					// Устанавливаем метод прокси-сервера
					response.method = static_cast <uint8_t> (method);
					// Выходим из цикла
					break;
				}
			}
		}
	}
	// Очищаем бинарный буфер данных
	this->_buffer.clear();
	// Увеличиваем память на 4 октета
	this->_buffer.resize(sizeof(uint8_t) * 2, 0x0);
	// Копируем в буфер нашу структуру ответа
	::memcpy(this->_buffer.data(), &response, sizeof(response));
}
/**
 * auth Метод получения бинарного буфера ответа на авторизацию клиента
 * @param login    логин пользователя
 * @param password пароль пользователя
 */
void awh::server::Socks5::auth(const string & login, const string & password) const noexcept {
	// Создаём объект ответа
	auth_t response;
	// Устанавливаем версию соглашения авторизации
	response.ver = AVER;
	// Устанавливаем ответ отказа об авторизации
	response.status = static_cast <uint8_t> (rep_t::FORBIDDEN);
	// Если пользователи установлены
	if(!login.empty() && !password.empty() && (this->_auth != nullptr)){
		// Если авторизация выполнена
		if(this->_auth(login, password))
			// Разрешаем авторизацию пользователя
			response.status = static_cast <uint8_t> (rep_t::SUCCESS);
	}
	// Очищаем бинарный буфер данных
	this->_buffer.clear();
	// Увеличиваем память на 4 октета
	this->_buffer.resize(sizeof(uint8_t) * 2, 0x0);
	// Копируем в буфер нашу структуру ответа
	::memcpy(this->_buffer.data(), &response, sizeof(response));
}
/**
 * parse Метод парсинга входящих данных
 * @param buffer бинарный буфер входящих данных
 * @param size   размер бинарного буфера входящих данных
 */
void awh::server::Socks5::parse(const char * buffer, const size_t size) noexcept {
	// Очищаем буфер данных
	this->_buffer.clear();
	// Если данные буфера переданы
	if((buffer != nullptr) && (size > 0)){
		// Определяем текущий стейт
		switch(static_cast <uint8_t> (this->_state)){
			// Если установлен стейт, выбора метода
			case static_cast <uint8_t> (state_t::METHOD): {
				// Если данных достаточно для получения ответа
				if(size > sizeof(uint16_t)){
					// Версия прокси-протокола
					uint8_t version = 0x0;
					// Выполняем чтение версии протокола
					::memcpy(&version, buffer, sizeof(version));
					// Если версия протокола соответствует
					if(version == VER){
						// Количество методов авторизации
						uint8_t count = 0x0;
						// Выполняем чтение количество методов авторизации
						::memcpy(&count, buffer + sizeof(uint8_t), sizeof(count));
						// Если количество методов авторизации получено
						if((count > 0) && (size >= (sizeof(uint16_t) + (sizeof(uint8_t) * count)))){
							// Полученный метод авторизации
							uint8_t method = 0x0;
							// Список методов авторизации
							vector <uint8_t> methods(count);
							// Переходим по всем методам авторизации
							for(uint8_t i = 0; i < count; i++){
								// Получаем метод авторизации
								::memcpy(&method, buffer + (sizeof(uint16_t) + (sizeof(uint8_t) * i)), sizeof(method));
								// Добавляем полученный метод авторизации в список методов
								methods.at(i) = method;
							}
							// Выполняем формирование ответа клиенту
							this->method(methods);
							// Проверяем метод сформированного овтета
							switch(static_cast <uint8_t> (this->_buffer.back())){
								// Если метод авторизации не выбран
								case static_cast <uint8_t> (method_t::NOMETHOD): {
									// Устанавливаем код сообщения
									this->_code = 0x01;
									// Устанавливаем статус ошибки
									this->_state = state_t::BROKEN;
								} break;
								// Если метод авторизации выбран
								case static_cast <uint8_t> (method_t::PASSWD):
									// Устанавливаем статус авторизации
									this->_state = state_t::AUTH;
								break;
								// Если авторизация не требуется
								case static_cast <uint8_t> (method_t::NOAUTH):
									// Устанавливаем статус запроса
									this->_state = state_t::REQUEST;
								break;
							}
						}
					// Если версия протокола не соответствует
					} else {
						// Устанавливаем код сообщения
						this->_code = 0x11;
						// Устанавливаем статус ошибки
						this->_state = state_t::BROKEN;
					}
				}
			} break;
			// Если установлен стейт, ожидания ответа на требования авторизации
			case static_cast <uint8_t> (state_t::AUTH): {
				// Если данных достаточно для получения ответа
				if(size > sizeof(uint32_t)){
					// Версия прокси-протокола
					uint8_t version = 0x0;
					// Выполняем чтение версии соглашения авторизации
					::memcpy(&version, buffer, sizeof(version));
					// Получаем смещение в буфере
					size_t offset = sizeof(version);
					// Если версия соглашения авторизации соответствует
					if(version == AVER){
						// Размер логина пользователя
						uint8_t length = 0x0;
						// Выполняем получение длины логина пользователя
						::memcpy(&length, buffer + offset, sizeof(length));
						// Если количество байт достаточно, чтобы получить логин пользователя
						if(size >= (offset + sizeof(length) + length)){
							// Получаем логин пользователя
							const string & login = this->text(buffer + offset, length);
							// Увеличиваем смещение в буфере
							offset += (sizeof(length) + length);
							// Если логин пользователя получен
							if(!login.empty() && (size >= (sizeof(uint16_t) + offset))){
								// Выполняем получение длины пароля пользователя
								::memcpy(&length, buffer + offset, sizeof(length));
								// Если количество байт достаточно, чтобы получить пароль пользователя
								if(size >= (offset + sizeof(length) + length)){
									// Получаем пароль пользователя
									const string & password = this->text(buffer + offset, length);
									// Если пароль получен
									if(!password.empty()){
										// Выполняем проверку авторизации
										this->auth(login, password);
										// Проверяем авторизацию пользователя
										switch(static_cast <uint8_t> (this->_buffer.back())){
											// Если авторизация не пройдена
											case static_cast <uint8_t> (rep_t::FORBIDDEN): {
												// Устанавливаем код сообщения
												this->_code = 0x12;
												// Устанавливаем статус ошибки
												this->_state = state_t::BROKEN;
											} break;
											// Если авторизация выполнена
											case static_cast <uint8_t> (rep_t::SUCCESS):
												// Устанавливаем статус запроса
												this->_state = state_t::REQUEST;
											break;
										}
									// Если пароль пользователя не получен
									} else {
										// Устанавливаем код сообщения
										this->_code = 0x10;
										// Устанавливаем статус ошибки
										this->_state = state_t::BROKEN;
									}
								}
							// Если логин пользователя не получен
							} else {
								// Устанавливаем код сообщения
								this->_code = 0x10;
								// Устанавливаем статус ошибки
								this->_state = state_t::BROKEN;
							}
						}
					// Если версия протокола не соответствует
					} else {
						// Устанавливаем код сообщения
						this->_code = 0x11;
						// Устанавливаем статус ошибки
						this->_state = state_t::BROKEN;
					}
				}
			} break;
			// Если установлен стейт, ожидания запроса
			case static_cast <uint8_t> (state_t::REQUEST): {				
				// Если данных достаточно для получения запроса
				if(size > sizeof(req_t)){					
					// Создаём объект данных запроса
					req_t req;
					// Выполняем чтение данных
					::memcpy(&req, buffer, sizeof(req));
					// Если версия протокола соответствует
					if(req.ver == static_cast <uint8_t> (VER)){
						// Если команда запрошена поддерживаемая сервером
						if(req.cmd == static_cast <uint8_t> (cmd_t::CONNECT)){
							// Определяем тип адреса
							switch(req.atyp){
								// Получаем адрес IPv4
								case static_cast <uint8_t> (atyp_t::IPv4): {
									// Если буфер пришел достаточного размера
									if(size >= (sizeof(req_t) + sizeof(ip_t))){
										// Создаём объект данных сервера
										ip_t server;
										// Устанавливаем тип хоста сервера
										this->_server.family = AF_INET;
										// Копируем в буфер наши данные IP адреса
										::memcpy(&server, buffer + sizeof(req_t), sizeof(server));
										// Выполняем получение IP адреса
										this->_server.host = this->hexToIp((const char *) &server.host, sizeof(server.host), AF_INET);
										// Если IP адрес получен
										if(!this->_server.host.empty()){
											// Заменяем порт сервера
											this->_server.port = ntohs(server.port);
											// Устанавливаем стейт выполнения проверки
											this->_state = state_t::CONNECT;
										}
									}
								} break;
								// Получаем адрес IPv6
								case static_cast <uint8_t> (atyp_t::IPv6): {
									// Если буфер пришел достаточного размера
									if(size >= (sizeof(req_t) + sizeof(ip_t))){
										// Создаём объект данных сервера
										ip_t server;
										// Устанавливаем тип хоста сервера
										this->_server.family = AF_INET6;
										// Копируем в буфер наши данные IP адреса
										::memcpy(&server, buffer + sizeof(req_t), sizeof(server));
										// Выполняем получение IP адреса
										this->_server.host = this->hexToIp((const char *) &server.host, sizeof(server.host), AF_INET6);
										// Если IP адрес получен
										if(!this->_server.host.empty()){
											// Заменяем порт сервера
											this->_server.port = ntohs(server.port);
											// Устанавливаем стейт выполнения проверки
											this->_state = state_t::CONNECT;
										}
									}
								} break;
								// Получаем адрес DMNAME
								case static_cast <uint8_t> (atyp_t::DMNAME): {
									// Если буфер пришел достаточного размера
									if(size >= (sizeof(req_t) + sizeof(uint16_t))){
										// Извлекаем доменное имя
										this->_server.host = this->text(buffer + sizeof(req_t), size);
										// Получаем размер смещения
										uint16_t offset = (sizeof(req_t) + sizeof(uint8_t) + this->_server.host.size());
										// Если доменное имя получено
										if(!this->_server.host.empty() && (size >= (offset + sizeof(uint16_t)))){
											// Создаём порт сервера
											uint16_t port = 0;
											// Выполняем извлечение порта сервера
											::memcpy(&port, buffer + offset, sizeof(uint16_t));
											// Заменяем порт сервера
											this->_server.port = ntohs(port);
											// Устанавливаем стейт выполнения проверки
											this->_state = state_t::CONNECT;
										}
									}
								} break;
							}
						// Если авторизация не пройдена
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
		}
	// Если данные не переданы
	} else {
		// Устанавливаем код сообщения
		this->_code = 0x01;
		// Устанавливаем статус ошибки
		this->_state = state_t::BROKEN;
	}
}
/**
 * reset Метод сброса собранных данных
 */
void awh::server::Socks5::reset() noexcept {
	// Выполняем сброс статуса ошибки
	this->_code = 0x00;
	// Выполняем очистку буфера данных
	this->_buffer.clear();
	// Выполняем сброс стейта
	this->_state = state_t::METHOD;
}
/**
 * authCallback Метод добавления функции обработки авторизации
 * @param callback функция обратного вызова для обработки авторизации
 */
void awh::server::Socks5::authCallback(function <bool (const string &, const string &)> callback) noexcept {
	// Устанавливаем функцию проверки авторизации
	this->_auth = callback;
}
