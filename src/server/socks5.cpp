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
 * getServer Метод извлечения параметров запрашиваемого сервера
 * @return параметры запрашиваемого сервера
 */
const awh::Socks5Server::serv_t & awh::Socks5Server::getServer() const noexcept {
	// Выводим запрашиваемый сервер
	return this->server;
}
/**
 * resCmd Метод получения бинарного буфера ответа
 * @param rep код ответа сервера
 */
void awh::Socks5Server::resCmd(const uint8_t rep) const noexcept {
	// Очищаем бинарный буфер данных
	this->buffer.clear();
	// Если IP адрес или доменное имя установлены
	if(!this->url.ip.empty() || !this->url.domain.empty()){
		// Бинарные данные буфера
		const char * data = nullptr;
		// Получаем значение порта
		const uint16_t port = htons(this->url.port);
		// Увеличиваем память на 4 октета
		this->buffer.resize(sizeof(uint8_t) * 4, 0x0);
		// Устанавливаем версию протокола
		u_short offset = this->setOctet(VER);
		// Устанавливаем комманду ответа
		offset = this->setOctet(rep, offset);
		// Устанавливаем RSV октет
		offset = this->setOctet(0x00, offset);
		// Если IP адрес получен
		if(!this->url.ip.empty()){
			// Получаем бинарные буфер IP адреса
			const auto & ip = this->ipToHex(this->url.ip, this->url.family);
			// Если буфер IP адреса получен
			if(!ip.empty()){
				// Определяем тип подключения
				switch(this->url.family){
					// Устанавливаем тип адреса [IPv4]
					case AF_INET: offset = this->setOctet((uint8_t) atyp_t::IPv4, offset); break;
					// Устанавливаем тип адреса [IPv6]
					case AF_INET6: offset = this->setOctet((uint8_t) atyp_t::IPv6, offset); break;
				}
				// Устанавливаем полученный буфер IP адреса
				this->buffer.insert(this->buffer.end(), ip.begin(), ip.end());
			}
		// Устанавливаем доменное имя
		} else {
			// Устанавливаем тип адреса [Доменное имя]
			offset = this->setOctet((uint8_t) atyp_t::DMNAME, offset);
			// Добавляем в буфер доменное имя
			this->setText(this->url.domain);
		}
		// Получаем бинарные данные порта
		data = (const char *) &port;
		// Устанавливаем порт ответа
		this->buffer.insert(this->buffer.end(), data, data + sizeof(port));
	}
}
/**
 * resMethod Метод получения бинарного буфера выбора метода подключения
 * @param methods методы авторизаций выбранныйе пользователем
 */
void awh::Socks5Server::resMethod(const vector <uint8_t> & methods) const noexcept {
	// Создаём объект ответа
	resMet_t response;
	// Устанавливаем версию прокси-протокола
	response.ver = VER;
	// Устанавливаем запрещённый метод авторизации
	response.method = (uint8_t) method_t::NOMETHOD;
	// Если пользователь выбрал список методов
	if(!methods.empty()){
		// Переходим по всем методам авторизаций
		for(auto & method : methods){
			// Если метод авторизации выбран логин/пароль пользователя
			if(method == (uint8_t) method_t::PASSWD){
				// Если пользователи установлены
				if(this->authFn != nullptr){
					// Устанавливаем метод прокси-сервера
					response.method = (uint8_t) method;
					// Выходим из цикла
					break;
				}
			// Если пользователь выбрал метод без авторизации
			} else if(method == (uint8_t) method_t::NOAUTH) {
				// Если пользователи не установлены
				if(this->authFn == nullptr){
					// Устанавливаем метод прокси-сервера
					response.method = (uint8_t) method;
					// Выходим из цикла
					break;
				}
			}
		}
	}
	// Очищаем бинарный буфер данных
	this->buffer.clear();
	// Увеличиваем память на 4 октета
	this->buffer.resize(sizeof(uint8_t) * 2, 0x0);
	// Копируем в буфер нашу структуру ответа
	memcpy(&response, this->buffer.data(), sizeof(response));
}
/**
 * resAuth Метод получения бинарного буфера ответа на авторизацию клиента
 * @param login    логин пользователя
 * @param password пароль пользователя
 */
void awh::Socks5Server::resAuth(const string & login, const string & password) const noexcept {
	// Создаём объект ответа
	resAuth_t response;
	// Устанавливаем версию соглашения авторизации
	response.ver = AVER;
	// Устанавливаем ответ отказа об авторизации
	response.status = (uint8_t) rep_t::FORBIDDEN;
	// Если пользователи установлены
	if(!login.empty() && !password.empty() && (this->authFn != nullptr)){
		// Если авторизация выполнена
		if(this->authFn(login, password))
			// Разрешаем авторизацию пользователя
			response.status = (uint8_t) rep_t::SUCCESS;
	}
	// Очищаем бинарный буфер данных
	this->buffer.clear();
	// Увеличиваем память на 4 октета
	this->buffer.resize(sizeof(uint8_t) * 2, 0x0);
	// Копируем в буфер нашу структуру ответа
	memcpy(&response, this->buffer.data(), sizeof(response));
}
/**
 * parse Метод парсинга входящих данных
 * @param buffer бинарный буфер входящих данных
 * @param size   размер бинарного буфера входящих данных
 */
void awh::Socks5Server::parse(const char * buffer, const size_t size) noexcept {
	// Очищаем буфер данных
	this->buffer.clear();
	// Если данные буфера переданы
	if((buffer != nullptr) && (size > 0)){
		// Определяем текущий стейт
		switch((uint8_t) this->state){
			// Если установлен стейт, выбора метода
			case (uint8_t) state_t::METHOD: {
				// Если данных достаточно для получения ответа
				if(size > sizeof(uint16_t)){
					// Версия прокси-протокола
					uint8_t version = 0x0;
					// Выполняем чтение версии протокола
					memcpy(&version, buffer, sizeof(version));
					// Если версия протокола соответствует
					if(version == VER){
						// Количество методов авторизации
						uint8_t count = 0x0;
						// Выполняем чтение количество методов авторизации
						memcpy(&count, buffer + sizeof(uint8_t), sizeof(count));
						// Если количество методов авторизации получено
						if((count > 0) && (size >= (sizeof(uint16_t) + (sizeof(uint8_t) * count)))){
							// Полученный метод авторизации
							uint8_t method = 0x0;
							// Список методов авторизации
							vector <uint8_t> methods(count);
							// Переходим по всем методам авторизации
							for(uint8_t i = 0; i < count; i++){
								// Получаем метод авторизации
								memcpy(&method, buffer + (sizeof(uint16_t) + (sizeof(uint8_t) * i)), sizeof(method));
								// Добавляем полученный метод авторизации в список методов
								methods.at(i) = method;
							}
							// Выполняем формирование ответа клиенту
							this->resMethod(methods);
							// Проверяем метод сформированного овтета
							switch((uint8_t) this->buffer.back()){
								// Если метод авторизации не выбран
								case (uint8_t) method_t::NOMETHOD: {
									// Устанавливаем код сообщения
									this->code = 0x01;
									// Устанавливаем статус ошибки
									this->state = state_t::BROKEN;
								} break;
								// Если метод авторизации выбран
								case (uint8_t) method_t::PASSWD: this->state = state_t::AUTH; break;
								// Если авторизация не требуется
								case (uint8_t) method_t::NOAUTH: this->state = state_t::REQUEST; break;
							}
						}
					// Если версия протокола не соответствует
					} else {
						// Устанавливаем код сообщения
						this->code = 0x11;
						// Устанавливаем статус ошибки
						this->state = state_t::BROKEN;
					}
				}
			} break;
			// Если установлен стейт, ожидания ответа на требования авторизации
			case (uint8_t) state_t::AUTH: {
				// Если данных достаточно для получения ответа
				if(size > sizeof(uint32_t)){
					// Версия прокси-протокола
					uint8_t version = 0x0;
					// Выполняем чтение версии соглашения авторизации
					memcpy(&version, buffer, sizeof(version));
					// Получаем смещение в буфере
					size_t offset = sizeof(version);
					// Если версия соглашения авторизации соответствует
					if(version == AVER){
						// Размер логина пользователя
						uint8_t length = 0x0;
						// Выполняем получение длины логина пользователя
						memcpy(&length, buffer + offset, sizeof(length));
						// Если количество байт достаточно, чтобы получить логин пользователя
						if(size >= (offset + sizeof(length) + length)){
							// Получаем логин пользователя
							const string & login = this->getText(buffer + offset, length);
							// Увеличиваем смещение в буфере
							offset += (sizeof(length) + length);
							// Если логин пользователя получен
							if(!login.empty() && (size >= (sizeof(uint16_t) + offset))){
								// Выполняем получение длины пароля пользователя
								memcpy(&length, buffer + offset, sizeof(length));
								// Если количество байт достаточно, чтобы получить пароль пользователя
								if(size >= (offset + sizeof(length) + length)){
									// Получаем пароль пользователя
									const string & password = this->getText(buffer + offset, length);
									// Если пароль получен
									if(!password.empty()){
										// Выполняем проверку авторизации
										this->resAuth(login, password);
										// Проверяем авторизацию пользователя
										switch((uint8_t) this->buffer.back()){
											// Если авторизация не пройдена
											case (uint8_t) rep_t::FORBIDDEN: {
												// Устанавливаем код сообщения
												this->code = 0x12;
												// Устанавливаем статус ошибки
												this->state = state_t::BROKEN;
											} break;
											// Если авторизация выполнена
											case (uint8_t) rep_t::SUCCESS: this->state = state_t::REQUEST; break;
										}
									// Если пароль пользователя не получен
									} else {
										// Устанавливаем код сообщения
										this->code = 0x10;
										// Устанавливаем статус ошибки
										this->state = state_t::BROKEN;
									}
								}
							// Если логин пользователя не получен
							} else {
								// Устанавливаем код сообщения
								this->code = 0x10;
								// Устанавливаем статус ошибки
								this->state = state_t::BROKEN;
							}
						}
					// Если версия протокола не соответствует
					} else {
						// Устанавливаем код сообщения
						this->code = 0x11;
						// Устанавливаем статус ошибки
						this->state = state_t::BROKEN;
					}
				}
			} break;
			// Если установлен стейт, ожидания запроса
			case (uint8_t) state_t::REQUEST: {
				// Если данных достаточно для получения запроса
				if(size > sizeof(req_t)){
					// Создаём объект данных запроса
					req_t req;
					// Выполняем чтение данных
					memcpy(&req, buffer, sizeof(req));
					// Если версия протокола соответствует
					if(req.ver == (uint8_t) VER){
						// Если команда запрошена поддерживаемая сервером
						if(req.cmd == (uint8_t) cmd_t::CONNECT){
							// Устанавливаем тип хоста сервера
							this->server.family = req.atyp;
							// Определяем тип адреса
							switch(req.atyp){
								// Получаем адрес IPv4
								case (uint8_t) atyp_t::IPv4: {
									// Если буфер пришел достаточного размера
									if(size >= (sizeof(req_t) + sizeof(ip_t))){
										// Создаём объект данных сервера
										ip_t server;
										// Копируем в буфер наши данные IP адреса
										memcpy(&server, buffer + sizeof(req_t), sizeof(server));
										// Выполняем получение IP адреса
										this->server.host = this->hexToIp((const char *) &server.host, sizeof(server.host), this->server.family);
										// Если IP адрес получен
										if(!this->server.host.empty()){
											// Заменяем порт сервера
											this->server.port = ntohs(server.port);
											// Устанавливаем стейт выполнения проверки
											this->state = state_t::CONNECT;
										}
									}
								} break;
								// Получаем адрес IPv6
								case (uint8_t) atyp_t::IPv6: {
									// Если буфер пришел достаточного размера
									if(size >= (sizeof(req_t) + sizeof(ip_t))){
										// Создаём объект данных сервера
										ip_t server;
										// Копируем в буфер наши данные IP адреса
										memcpy(&server, buffer + sizeof(req_t), sizeof(server));
										// Выполняем получение IP адреса
										this->server.host = this->hexToIp((const char *) &server.host, sizeof(server.host), this->server.family);
										// Если IP адрес получен
										if(!this->server.host.empty()){
											// Заменяем порт сервера
											this->server.port = ntohs(server.port);
											// Устанавливаем стейт выполнения проверки
											this->state = state_t::CONNECT;
										}
									}
								} break;
								// Получаем адрес DMNAME
								case (uint8_t) atyp_t::DMNAME: {
									// Если буфер пришел достаточного размера
									if(size >= (sizeof(req_t) + sizeof(uint16_t))){
										// Извлекаем доменное имя
										this->server.host = this->getText(buffer + sizeof(req_t), size);
										// Получаем размер смещения
										u_short offset = (sizeof(req_t) + sizeof(uint8_t) + this->server.host.size());
										// Если доменное имя получено
										if(!this->server.host.empty() && (size >= (offset + sizeof(uint16_t)))){
											// Создаём порт сервера
											uint16_t port = 0;
											// Выполняем извлечение порта сервера
											memcpy(&port, buffer + offset, sizeof(uint16_t));
											// Заменяем порт сервера
											this->server.port = ntohs(port);
											// Устанавливаем стейт выполнения проверки
											this->state = state_t::CONNECT;
										}
									}
								} break;
							}
						// Если авторизация не пройдена
						} else {
							// Устанавливаем код сообщения
							this->code = 0x07;
							// Устанавливаем статус ошибки
							this->state = state_t::BROKEN;
						}
					// Если версия прокси-сервера не соответствует
					} else {
						// Устанавливаем код сообщения
						this->code = 0x11;
						// Устанавливаем статус ошибки
						this->state = state_t::BROKEN;
					}
				}
			} break;
		}
	// Если данные не переданы
	} else {
		// Устанавливаем код сообщения
		this->code = 0x01;
		// Устанавливаем статус ошибки
		this->state = state_t::BROKEN;
	}
}
/**
 * reset Метод сброса собранных данных
 */
void awh::Socks5Server::reset() noexcept {
	// Выполняем сброс статуса ошибки
	this->code = 0x00;
	// Выполняем очистку буфера данных
	this->buffer.clear();
	// Выполняем сброс стейта
	this->state = state_t::METHOD;
}
/**
 * setAuthCallback Метод добавления функции обработки авторизации
 * @param callback функция обратного вызова для обработки авторизации
 */
void awh::Socks5Server::setAuthCallback(function <bool (const string &, const string &)> callback) noexcept {
	// Устанавливаем функцию проверки авторизации
	this->authFn = callback;
}
