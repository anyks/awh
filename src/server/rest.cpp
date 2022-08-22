/**
 * @file: rest.cpp
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
#include <server/rest.hpp>

/**
 * chunking Метод обработки получения чанков
 * @param chunk бинарный буфер чанка
 * @param http  объект модуля HTTP
 */
void awh::server::Rest::chunking(const vector <char> & chunk, const awh::http_t * http) noexcept {
	// Если данные получены, формируем тело сообщения
	if(!chunk.empty()) const_cast <awh::http_t *> (http)->body(chunk);
}
/**
 * openCallback Функция обратного вызова при запуске работы
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 */
void awh::server::Rest::openCallback(const size_t wid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((wid > 0) && (core != nullptr)){
		// Устанавливаем хост сервера
		reinterpret_cast <server::core_t *> (core)->init(wid, this->_port, this->_host);
		// Выполняем запуск сервера
		reinterpret_cast <server::core_t *> (core)->run(wid);
	}
}
/**
 * persistCallback Функция персистентного вызова
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 */
void awh::server::Rest::persistCallback(const size_t aid, const size_t wid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((aid > 0) && (wid > 0) && (core != nullptr)){
		// Получаем параметры подключения адъютанта
		rest_worker_t::coffer_t * adj = const_cast <rest_worker_t::coffer_t *> (this->_worker.get(aid));
		// Если параметры подключения адъютанта получены
		if((adj != nullptr) && ((!adj->alive && !this->_alive) || adj->close)){
			// Если адъютант давно должен был быть отключён, отключаем его
			if(adj->close || !adj->http.isAlive()) reinterpret_cast <server::core_t *> (core)->close(aid);
			// Иначе проверяем прошедшее время
			else {
				// Получаем текущий штамп времени
				const time_t stamp = this->_fmk->unixTimestamp();
				// Если адъютант не ответил на пинг больше двух интервалов, отключаем его
				if((stamp - adj->checkPoint) >= this->_timeAlive)
					// Завершаем работу
					reinterpret_cast <server::core_t *> (core)->close(aid);
			}
		}
	}
}
/**
 * connectCallback Функция обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 */
void awh::server::Rest::connectCallback(const size_t aid, const size_t wid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((aid > 0) && (wid > 0) && (core != nullptr)){
		// Создаём адъютанта
		this->_worker.set(aid);
		// Получаем параметры подключения адъютанта
		rest_worker_t::coffer_t * adj = const_cast <rest_worker_t::coffer_t *> (this->_worker.get(aid));
		// Если объект адъютанта получен
		if(adj != nullptr){
			// Если нужно активировать многопоточность и она не активирована
			if(this->_threadsEnabled && !this->_thr.is())
				// Выполняем активацию тредпула
				this->_thr.init(this->_threadsCount);
			// Устанавливаем экшен выполнения
			adj->action = rest_worker_t::action_t::CONNECT;
			// Выполняем запуск обработчика событий
			this->handler(aid);
		}
	}
}
/**
 * disconnectCallback Функция обратного вызова при отключении от сервера
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 */
void awh::server::Rest::disconnectCallback(const size_t aid, const size_t wid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((aid > 0) && (wid > 0) && (core != nullptr)){
		// Получаем параметры подключения адъютанта
		rest_worker_t::coffer_t * adj = const_cast <rest_worker_t::coffer_t *> (this->_worker.get(aid));
		// Если объект адъютанта получен
		if(adj != nullptr){
			// Устанавливаем экшен выполнения
			adj->action = rest_worker_t::action_t::DISCONNECT;
			// Выполняем запуск обработчика событий
			this->handler(aid);
		}
	}
}
/**
 * readCallback Функция обратного вызова при чтении сообщения с адъютанта
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param wid    идентификатор воркера
 * @param core   объект биндинга TCP/IP
 */
void awh::server::Rest::readCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (aid > 0) && (wid > 0)){
		// Получаем параметры подключения адъютанта
		rest_worker_t::coffer_t * adj = const_cast <rest_worker_t::coffer_t *> (this->_worker.get(aid));
		// Если объект адъютанта получен
		if(adj != nullptr){
			// Если дисконнекта ещё не произошло
			if((adj->action == rest_worker_t::action_t::NONE) || (adj->action == rest_worker_t::action_t::READ)){
				// Если подключение закрыто
				if(adj->close){
					// Принудительно выполняем отключение лкиента
					reinterpret_cast <server::core_t *> (core)->close(aid);
					// Выходим из функции
					return;
				}
				// Если разрешено получение данных
				if(adj->allow.receive){
					// Устанавливаем экшен выполнения
					adj->action = rest_worker_t::action_t::READ;
					// Добавляем полученные данные в буфер
					adj->buffer.insert(adj->buffer.end(), buffer, buffer + size);
					// Выполняем запуск обработчика событий
					this->handler(aid);
				}
			}
		}
	}
}
/**
 * writeCallback Функция обратного вызова при записи сообщение адъютанту
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер записанных в сокет байт
 * @param aid    идентификатор адъютанта
 * @param wid    идентификатор воркера
 * @param core   объект биндинга TCP/IP
 */
void awh::server::Rest::writeCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((aid > 0) && (wid > 0) && (core != nullptr)){
		// Получаем параметры подключения адъютанта
		rest_worker_t::coffer_t * adj = const_cast <rest_worker_t::coffer_t *> (this->_worker.get(aid));
		// Если объект адъютанта получен
		if(adj != nullptr){
			// Если необходимо выполнить закрыть подключение
			if(!adj->close && adj->stopped){
				// Устанавливаем флаг закрытия подключения
				adj->close = !adj->close;
				// Принудительно выполняем отключение лкиента
				const_cast <server::core_t *> (this->_core)->close(aid);
			}
		}
	}
}
/**
 * acceptCallback Функция обратного вызова при проверке подключения адъютанта
 * @param ip   адрес интернет подключения адъютанта
 * @param mac  мак-адрес подключившегося адъютанта
 * @param port порт подключившегося адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @return     результат разрешения к подключению адъютанта
 */
bool awh::server::Rest::acceptCallback(const string & ip, const string & mac, const u_int port, const size_t wid, awh::core_t * core) noexcept {
	// Результат работы функции
	bool result = true;
	// Если данные существуют
	if(!ip.empty() && !mac.empty() && (wid > 0) && (core != nullptr)){
		// Если функция обратного вызова установлена, проверяем
		if(this->_callback.accept != nullptr) result = this->_callback.accept(ip, mac, port, this);
	}
	// Разрешаем подключение адъютанту
	return result;
}
/**
 * messageCallback Функция обратного вызова при получении сообщений сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param wid    идентификатор воркера
 * @param aid    идентификатор адъютанта
 * @param pid    идентификатор дочернего процесса
 * @param core   объект биндинга TCP/IP
 */
void awh::server::Rest::messageCallback(const char * buffer, const size_t size, const size_t wid, const size_t aid, const pid_t pid, awh::core_t * core) noexcept {
	// Если активирована система игольного ушка
	if(this->_needleEye){
		// Если процесс является родительским
		if(this->_pid == getpid()){
			// Определяем тип сигнала
			switch((uint8_t) buffer[0]){
				// Если сигнал получения сообщения
				case (uint8_t) needle_t::MESSAGE: {
					// Получаем объект подключения
					auto it = this->_adjutants.find(aid);
					// Если объект подключения получен
					if(it != this->_adjutants.end()){
						// Выполняем установку дампа HTTP данных
						it->second->http.dump(vector <char> (buffer + 1, (buffer + 1) + (size - 1)));
						// Если функция обратного вызова, установлена
						if(this->_callback.message != nullptr)
							// Отправляем полученный результат
							this->_callback.message(aid, &it->second->http, const_cast <Rest *> (this));
					}
				} break;
				// Если сигнал подключения
				case (uint8_t) needle_t::CONNECT: {
					// Создаём бъект адъютанта
					unique_ptr <adjutant_t> adjutant(new adjutant_t(this->_fmk, this->_log, &this->_uri));
					// Размер строковой записи
					size_t length = 0, offset = 1;
					// Выполняем извлечение порта пользователя
					memcpy((void *) &adjutant->port, buffer + offset, sizeof(adjutant->port));
					// Увеличиваем смещение в буфере данных
					offset += sizeof(adjutant->port);
					// Выполняем извлечение длины IP адреса пользователя
					memcpy(&length, buffer + offset, sizeof(length));
					// Увеличиваем смещение в буфере данных
					offset += sizeof(length);
					// Выделяем память для IP адреса
					adjutant->ip.resize(length, 0);
					// Копируем полученные данные IP адреса
					memcpy((void *) adjutant->ip.data(), buffer + offset, length);
					// Увеличиваем смещение в буфере данных
					offset += length;
					// Выполняем извлечение длины MAC адреса пользователя
					memcpy(&length, buffer + offset, sizeof(length));
					// Увеличиваем смещение в буфере данных
					offset += sizeof(length);
					// Выделяем память для MAC адреса
					adjutant->mac.resize(length, 0);
					// Копируем полученные данные MAC адреса
					memcpy((void *) adjutant->mac.data(), buffer + offset, length);
					// Увеличиваем смещение в буфере данных
					offset += length;
					// Выполняем добавление адъютанта в список адъютантов
					this->_adjutants.emplace(aid, move(adjutant));
					// Если функция обратного вызова установлена, выполняем
					if(this->_callback.active != nullptr) this->_callback.active(aid, mode_t::CONNECT, this);
				} break;
				// Если сигнал отключения
				case (uint8_t) needle_t::DISCONNECT: {
					// Выполняем удаление адъютанта из списка адъютантов
					this->_adjutants.erase(aid);
					// Если функция обратного вызова установлена, выполняем
					if(this->_callback.active != nullptr) this->_callback.active(aid, mode_t::DISCONNECT, this);
				} break;
			}
		// Если процесс является дочерним, отправляем сообщение
		} else {
			// Определяем тип сигнала
			switch((uint8_t) buffer[0]){
				// Если сигнал получения сообщения
				case (uint8_t) needle_t::MESSAGE: {
					// Получаем параметры подключения адъютанта
					rest_worker_t::coffer_t * adj = const_cast <rest_worker_t::coffer_t *> (this->_worker.get(aid));
					// Если объект адъютанта получен
					if(adj != nullptr)
						// Устанавливаем флаг закрытия подключения
						adj->stopped = true;
					// Отправляем тело на сервер
					((awh::core_t *) const_cast <server::core_t *> (this->_core))->write(buffer + 1, size - 1, aid);
				} break;
				// Если сигнал отключения
				case (uint8_t) needle_t::DISCONNECT: {
					// Получаем параметры подключения адъютанта
					rest_worker_t::coffer_t * adj = const_cast <rest_worker_t::coffer_t *> (this->_worker.get(aid));
					// Если параметры подключения адъютанта получены, устанавливаем флаг закрытия подключения
					if(adj != nullptr){
						// Устанавливаем флаг закрытия подключения адъютанта
						adj->close = true;
						// Выполняем отключение адъютанта
						const_cast <server::core_t *> (this->_core)->close(aid);
					}
				} break;
			}
		}
	}
}
/**
 * handler Метод управления входящими методами
 * @param aid идентификатор адъютанта
 */
void awh::server::Rest::handler(const size_t aid) noexcept {
	// Получаем параметры подключения адъютанта
	rest_worker_t::coffer_t * adj = const_cast <rest_worker_t::coffer_t *> (this->_worker.get(aid));
	// Если объект адъютанта получен
	if(adj != nullptr){
		// Если управляющий блокировщик не заблокирован
		if(!adj->locker.mode){
			// Выполняем блокировку потока
			const lock_guard <recursive_mutex> lock(adj->locker.mtx);
			// Флаг разрешающий циклический перебор экшенов
			bool loop = true;
			// Выполняем блокировку обработчика
			adj->locker.mode = true;
			// Выполняем обработку всех экшенов
			while(loop && (adj->action != rest_worker_t::action_t::NONE)){
				// Определяем обрабатываемый экшен
				switch((uint8_t) adj->action){
					// Если необходимо запустить экшен обработки данных поступающих с сервера
					case (uint8_t) rest_worker_t::action_t::READ: this->actionRead(aid); break;
					// Если необходимо запустить экшен обработки подключения к серверу
					case (uint8_t) rest_worker_t::action_t::CONNECT: this->actionConnect(aid); break;
					// Если необходимо запустить экшен обработки отключения от сервера
					case (uint8_t) rest_worker_t::action_t::DISCONNECT: this->actionDisconnect(aid); break;
					// Если сработал неизвестный экшен, выходим
					default: loop = false;
				}
			}
			// Выполняем разблокировку обработчика
			adj->locker.mode = false;
		}
	}
}
/**
 * actionRead Метод обработки экшена чтения с сервера
 * @param aid идентификатор адъютанта
 */
void awh::server::Rest::actionRead(const size_t aid) noexcept {
	// Если данные существуют
	if(aid > 0){
		// Получаем параметры подключения адъютанта
		rest_worker_t::coffer_t * adj = const_cast <rest_worker_t::coffer_t *> (this->_worker.get(aid));
		// Если объект адъютанта получен
		if(adj != nullptr){
			// Выполняем обработку полученных данных
			while(!adj->close){
				// Выполняем парсинг полученных данных
				size_t bytes = adj->http.parse(adj->buffer.data(), adj->buffer.size());
				// Если все данные получены
				if(adj->http.isEnd()){
					// Если включён режим отладки
					#if defined(DEBUG_MODE)
						// Получаем данные запроса
						const auto & request = adj->http.request(true);
						// Если параметры запроса получены
						if(!request.empty()){
							// Выводим заголовок запроса
							cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << endl;
							// Выводим параметры запроса
							cout << string(request.begin(), request.end()) << endl;
							// Если тело запроса существует
							if(!adj->http.body().empty())
								// Выводим сообщение о выводе чанка тела
								cout << this->_fmk->format("<body %u>", adj->http.body().size()) << endl << endl;
							// Иначе устанавливаем перенос строки
							else cout << endl;
						}
					#endif
					// Если подключение не установлено как постоянное
					if(!this->_alive && !adj->alive){
						// Увеличиваем количество выполненных запросов
						adj->requests++;
						// Если количество выполненных запросов превышает максимальный
						if(adj->requests >= this->_maxRequests)
							// Устанавливаем флаг закрытия подключения
							adj->close = true;
						// Получаем текущий штамп времени
						else adj->checkPoint = this->_fmk->unixTimestamp();
					// Выполняем сброс количества выполненных запросов
					} else adj->requests = 0;
					// Получаем объект сетевого ядра
					core_t * core = const_cast <core_t *> (this->_core);
					// Выполняем проверку авторизации
					switch((uint8_t) adj->http.getAuth()){
						// Если запрос выполнен удачно
						case (uint8_t) http_t::stath_t::GOOD: {
							// Получаем флаг шифрованных данных
							adj->crypt = adj->http.isCrypt();
							// Получаем поддерживаемый метод компрессии
							adj->compress = adj->http.compress();
							// Если активирована система игольного ушка
							if(this->_needleEye && (this->_pid != getpid())){
								// Получаем дамп HTTP объекта
								vector <char> buffer = adj->http.dump();
								// Получаем тип сообщения
								const uint8_t type = (uint8_t) needle_t::MESSAGE;
								// Добавляем тип сообщения
								buffer.insert(buffer.begin(), (const char *) &type, (const char *) &type + sizeof(type));
								// Отправляем сообщение родительскому процессу
								const_cast <server::core_t *> (this->_core)->sendMessage(this->_worker.wid, aid, getpid(), buffer.data(), buffer.size());
							// Если функция обратного вызова, установлена
							} else if(this->_callback.message != nullptr)
								// Отправляем полученный результат
								this->_callback.message(aid, &adj->http, const_cast <Rest *> (this));
							// Выполняем сброс состояния HTTP парсера
							adj->http.clear();
							// Выполняем сброс состояния HTTP парсера
							adj->http.reset();
							// Завершаем обработку
							goto Next;
						} break;
						// Если запрос неудачный
						case (uint8_t) http_t::stath_t::FAULT: {
							// Выполняем сброс состояния HTTP парсера
							adj->http.clear();
							// Выполняем сброс состояния HTTP парсера
							adj->http.reset();
							// Выполняем очистку буфера полученных данных
							adj->buffer.clear();
							// Формируем запрос авторизации
							const auto & response = adj->http.reject(401);
							// Если ответ получен
							if(!response.empty()){
								// Тело полезной нагрузки
								vector <char> payload;
								// Отправляем ответ адъютанту
								core->write(response.data(), response.size(), aid);
								// Получаем данные тела запроса
								while(!(payload = adj->http.payload()).empty())
									// Отправляем тело на сервер
									core->write(payload.data(), payload.size(), aid);
								// Устанавливаем флаг закрытия подключения
								adj->stopped = true;
							// Выполняем отключение адъютанта
							} else core->close(aid);
							// Если экшен соответствует, выполняем его сброс
							if(adj->action == rest_worker_t::action_t::READ)
								// Выполняем сброс экшена
								adj->action = rest_worker_t::action_t::NONE;
							// Выходим из функции
							return;
						}
					}
				}
				// Устанавливаем метку продолжения обработки пайплайна
				Next:
				// Если парсер обработал какое-то количество байт
				if((bytes > 0) && !adj->buffer.empty()){
					// Если размер буфера больше количества удаляемых байт
					if(adj->buffer.size() >= bytes)
						// Удаляем количество обработанных байт
						adj->buffer.assign(adj->buffer.begin() + bytes, adj->buffer.end());
						// Удаляем количество обработанных байт
						// vector <decltype(adj->buffer)::value_type> (adj->buffer.begin() + bytes, adj->buffer.end()).swap(adj->buffer);
					// Если байт в буфере меньше, просто очищаем буфер
					else adj->buffer.clear();
					// Если данных для обработки не осталось, выходим
					if(adj->buffer.empty()) break;
				// Если данных для обработки недостаточно, выходим
				} else break;
			}
			// Если экшен соответствует, выполняем его сброс
			if(adj->action == rest_worker_t::action_t::READ)
				// Выполняем сброс экшена
				adj->action = rest_worker_t::action_t::NONE;
		}
	}
}
/**
 * actionConnect Метод обработки экшена подключения к серверу
 * @param aid идентификатор адъютанта
 */
void awh::server::Rest::actionConnect(const size_t aid) noexcept {
	// Если данные существуют
	if(aid > 0){
		// Получаем параметры подключения адъютанта
		rest_worker_t::coffer_t * adj = const_cast <rest_worker_t::coffer_t *> (this->_worker.get(aid));
		// Если объект адъютанта получен
		if(adj != nullptr){
			// Устанавливаем размер чанка
			adj->http.chunk(this->_chunkSize);
			// Устанавливаем данные сервиса
			adj->http.serv(this->_sid, this->_name, this->_ver);
			// Если функция обратного вызова для обработки чанков установлена
			if(this->_callback.chunking != nullptr)
				// Устанавливаем функцию обработки вызова для получения чанков
				adj->http.chunking(this->_callback.chunking);
			// Устанавливаем функцию обработки вызова для получения чанков
			else adj->http.chunking(std::bind(&Rest::chunking, this, _1, _2));
			// Устанавливаем метод компрессии поддерживаемый сервером
			adj->http.compress(this->_worker.compress);
			// Устанавливаем параметры шифрования
			if(this->_crypt) adj->http.crypto(this->_pass, this->_salt, this->_cipher);
			// Если сервер требует авторизацию
			if(this->_authType != auth_t::type_t::NONE){
				// Определяем тип авторизации
				switch((uint8_t) this->_authType){
					// Если тип авторизации Basic
					case (uint8_t) auth_t::type_t::BASIC: {
						// Устанавливаем параметры авторизации
						adj->http.authType(this->_authType);
						// Устанавливаем функцию проверки авторизации
						adj->http.authCallback(this->_callback.checkAuth);
					} break;
					// Если тип авторизации Digest
					case (uint8_t) auth_t::type_t::DIGEST: {
						// Устанавливаем название сервера
						adj->http.realm(this->_realm);
						// Устанавливаем временный ключ сессии сервера
						adj->http.opaque(this->_opaque);
						// Устанавливаем параметры авторизации
						adj->http.authType(this->_authType, this->_authHash);
						// Устанавливаем функцию извлечения пароля
						adj->http.extractPassCallback(this->_callback.extractPass);
					} break;
				}
			}
			// Если экшен соответствует, выполняем его сброс
			if(adj->action == rest_worker_t::action_t::CONNECT)
				// Выполняем сброс экшена
				adj->action = rest_worker_t::action_t::NONE;
			// Если активирована система игольного ушка
			if(this->_needleEye && (this->_pid != getpid())){
				// Размер строковой записи
				size_t length = 0;
				// Получаем порт адъютанта
				const u_int port = this->port(aid);
				// Получаем IP адрес адъютанта
				const string & ip = this->ip(aid);
				// Получаем MAC адрес адъютанта
				const string & mac = this->mac(aid);
				// Создаём буфер отправляемого сообщения
				vector <char> buffer(1, (char) needle_t::CONNECT);
				// Добавляем порт адъютанта в буфер сообщения
				buffer.insert(buffer.end(), (const char *) &port, (const char *) &port + sizeof(port));
				// Получаем размер записи IP адреса адъютанта
				length = ip.size();
				// Добавляем размер IP адреса адъютанта
				buffer.insert(buffer.end(), (const char *) &length, (const char *) &length + sizeof(length));
				// Добавляем сами данные IP адреса адъютанта
				buffer.insert(buffer.end(), ip.begin(), ip.end());
				// Получаем размер записи MAC адреса адъютанта
				length = mac.size();
				// Добавляем размер MAC адреса адъютанта
				buffer.insert(buffer.end(), (const char *) &length, (const char *) &length + sizeof(length));
				// Добавляем сами данные MAC адреса адъютанта
				buffer.insert(buffer.end(), mac.begin(), mac.end());
				// Отправляем сообщение родительскому процессу
				const_cast <server::core_t *> (this->_core)->sendMessage(this->_worker.wid, aid, getpid(), buffer.data(), buffer.size());
			// Если функция обратного вызова установлена, выполняем
			} else if(this->_callback.active != nullptr) this->_callback.active(aid, mode_t::CONNECT, this);
		}
	}
}
/**
 * actionDisconnect Метод обработки экшена отключения от сервера
 * @param aid идентификатор адъютанта
 */
void awh::server::Rest::actionDisconnect(const size_t aid) noexcept {
	// Если данные существуют
	if(aid > 0){
		// Получаем параметры подключения адъютанта
		rest_worker_t::coffer_t * adj = const_cast <rest_worker_t::coffer_t *> (this->_worker.get(aid));
		// Если объект адъютанта получен
		if(adj != nullptr){
			// Устанавливаем флаг отключения
			adj->close = true;
			// Если активирована система игольного ушка
			if(this->_needleEye && (this->_pid != getpid())){
				// Создаём буфер отправляемого сообщения
				vector <char> buffer(1, (char) needle_t::DISCONNECT);
				// Отправляем сообщение родительскому процессу
				const_cast <server::core_t *> (this->_core)->sendMessage(this->_worker.wid, aid, getpid(), buffer.data(), buffer.size());
			// Если функция обратного вызова установлена, выполняем
			} else if(this->_callback.active != nullptr) this->_callback.active(aid, mode_t::DISCONNECT, this);
			// Если экшен соответствует, выполняем его сброс
			if(adj->action == rest_worker_t::action_t::DISCONNECT)
				// Выполняем сброс экшена
				adj->action = rest_worker_t::action_t::NONE;
			// Выполняем удаление параметров адъютанта
			this->_worker.rm(aid);
		}
	}
}
/**
 * init Метод инициализации Rest адъютанта
 * @param socket   unix socket для биндинга
 * @param compress метод сжатия передаваемых сообщений
 */
void awh::server::Rest::init(const string & socket, const http_t::compress_t compress) noexcept {
	// Устанавливаем тип компрессии
	this->_worker.compress = compress;
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Выполняем установку unix-сокет
		const_cast <server::core_t *> (this->_core)->unixSocket(socket);
	#endif
}
/**
 * init Метод инициализации Rest адъютанта
 * @param port     порт сервера
 * @param host     хост сервера
 * @param compress метод сжатия передаваемых сообщений
 */
void awh::server::Rest::init(const u_int port, const string & host, const http_t::compress_t compress) noexcept {
	// Устанавливаем порт сервера
	this->_port = port;
	// Устанавливаем хост сервера
	this->_host = host;
	// Устанавливаем тип компрессии
	this->_worker.compress = compress;
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Удаляем unix-сокет ранее установленный
		const_cast <server::core_t *> (this->_core)->removeUnixSocket();
	#endif
}
/**
 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
 * @param callback функция обратного вызова
 */
void awh::server::Rest::on(function <void (const size_t, const mode_t, Rest *)> callback) noexcept {
	// Устанавливаем функцию запуска и остановки
	this->_callback.active = callback;
}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений
 * @param callback функция обратного вызова
 */
void awh::server::Rest::on(function <void (const size_t, const awh::http_t *, Rest *)> callback) noexcept {
	// Устанавливаем функцию получения сообщений с сервера
	this->_callback.message = callback;
}
/**
 * on Метод добавления функции извлечения пароля
 * @param callback функция обратного вызова для извлечения пароля
 */
void awh::server::Rest::on(function <string (const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова для извлечения пароля
	this->_callback.extractPass = callback;
}
/**
 * on Метод добавления функции обработки авторизации
 * @param callback функция обратного вызова для обработки авторизации
 */
void awh::server::Rest::on(function <bool (const string &, const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова для обработки авторизации
	this->_callback.checkAuth = callback;
}
/**
 * on Метод установки функции обратного вызова для получения чанков
 * @param callback функция обратного вызова
 */
void awh::server::Rest::on(function <void (const vector <char> &, const awh::http_t *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова для получения чанков
	this->_callback.chunking = callback;
}
/**
 * on Метод установки функции обратного вызова на событие активации адъютанта на сервере
 * @param callback функция обратного вызова
 */
void awh::server::Rest::on(function <bool (const string &, const string &, const u_int, Rest *)> callback) noexcept {
	// Устанавливаем функцию запуска и остановки
	this->_callback.accept = callback;
}
/**
 * reject Метод отправки сообщения об ошибке
 * @param aid     идентификатор адъютанта
 * @param code    код сообщения для адъютанта
 * @param mess    отправляемое сообщение об ошибке
 * @param entity  данные полезной нагрузки (тело сообщения)
 * @param headers HTTP заголовки сообщения
 */
void awh::server::Rest::reject(const size_t aid, const u_int code, const string & mess, const vector <char> & entity, const unordered_multimap <string, string> & headers) const noexcept {
	// Если подключение выполнено
	if(this->_core->working()){
		// Если активирована система игольного ушка
		if(this->_needleEye && (this->_pid == getpid())){
			// Получаем объект подключения
			auto it = this->_adjutants.find(aid);
			// Если объект подключения получен
			if(it != this->_adjutants.end()){
				// Тело полезной нагрузки
				vector <char> payload;
				// Создаём буфер данных для отправки сообщения
				vector <char> buffer(1, (char) needle_t::MESSAGE);
				// Устанавливаем полезную нагрузку
				it->second->http.body(entity);
				// Устанавливаем заголовки ответа
				it->second->http.headers(headers);
				// Если подключение не установлено как постоянное, но подключение долгоживущее
				if(!this->_alive && it->second->http.isAlive())
					// Указываем сколько запросов разрешено выполнить за указанный интервал времени
					it->second->http.header("Keep-Alive", this->_fmk->format("timeout=%d, max=%d", this->_timeAlive / 1000, this->_maxRequests));
				// Формируем запрос авторизации
				const auto & response = it->second->http.reject(code, mess);
				// Если включён режим отладки
				#if defined(DEBUG_MODE)
					// Выводим заголовок ответа
					cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
					// Выводим параметры ответа
					cout << string(response.begin(), response.end()) << endl;
				#endif
				// Добавляем в буфер бинарные данные заголовков
				buffer.insert(buffer.end(), response.begin(), response.end());
				// Получаем данные полезной нагрузки ответа
				while(!(payload = it->second->http.payload()).empty()){
					// Если включён режим отладки
					#if defined(DEBUG_MODE)
						// Выводим сообщение о выводе чанка полезной нагрузки
						cout << this->_fmk->format("<chunk %u>", payload.size()) << endl;
					#endif
					// Добавляем в буфер бинарные данные тела сообщения
					buffer.insert(buffer.end(), payload.begin(), payload.end());
				}
				// Отправляем сообщение дочернему процессу
				const_cast <server::core_t *> (this->_core)->sendMessage(this->_worker.wid, aid, getpid(), buffer.data(), buffer.size());
			}
		// Иначе просто отправляем сообщение адъютанту
		} else {
			// Получаем параметры подключения адъютанта
			rest_worker_t::coffer_t * adj = const_cast <rest_worker_t::coffer_t *> (this->_worker.get(aid));
			// Если отправка сообщений разблокированна
			if(adj != nullptr){
				// Тело полезной нагрузки
				vector <char> payload;
				// Устанавливаем полезную нагрузку
				adj->http.body(entity);
				// Устанавливаем заголовки ответа
				adj->http.headers(headers);
				// Если подключение не установлено как постоянное, но подключение долгоживущее
				if(!this->_alive && !adj->alive && adj->http.isAlive())
					// Указываем сколько запросов разрешено выполнить за указанный интервал времени
					adj->http.header("Keep-Alive", this->_fmk->format("timeout=%d, max=%d", this->_timeAlive / 1000, this->_maxRequests));
				// Формируем запрос авторизации
				const auto & response = adj->http.reject(code, mess);
				// Если включён режим отладки
				#if defined(DEBUG_MODE)
					// Выводим заголовок ответа
					cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
					// Выводим параметры ответа
					cout << string(response.begin(), response.end()) << endl;
				#endif
				// Отправляем серверу сообщение
				((awh::core_t *) const_cast <server::core_t *> (this->_core))->write(response.data(), response.size(), aid);
				// Получаем данные полезной нагрузки ответа
				while(!(payload = adj->http.payload()).empty()){
					// Если включён режим отладки
					#if defined(DEBUG_MODE)
						// Выводим сообщение о выводе чанка полезной нагрузки
						cout << this->_fmk->format("<chunk %u>", payload.size()) << endl;
					#endif
					// Отправляем тело на сервер
					((awh::core_t *) const_cast <server::core_t *> (this->_core))->write(payload.data(), payload.size(), aid);
				}
				// Устанавливаем флаг завершения работы
				adj->stopped = true;
			}
		}
	}
}
/**
 * response Метод отправки сообщения адъютанту
 * @param aid     идентификатор адъютанта
 * @param code    код сообщения для адъютанта
 * @param mess    отправляемое сообщение об ошибке
 * @param entity  данные полезной нагрузки (тело сообщения)
 * @param headers HTTP заголовки сообщения
 */
void awh::server::Rest::response(const size_t aid, const u_int code, const string & mess, const vector <char> & entity, const unordered_multimap <string, string> & headers) const noexcept {
	// Если подключение выполнено
	if(this->_core->working()){
		// Если активирована система игольного ушка
		if(this->_needleEye && (this->_pid == getpid())){
			// Получаем объект подключения
			auto it = this->_adjutants.find(aid);
			// Если объект подключения получен
			if(it != this->_adjutants.end()){
				// Тело полезной нагрузки
				vector <char> payload;
				// Создаём буфер данных для отправки сообщения
				vector <char> buffer(1, (char) needle_t::MESSAGE);
				// Устанавливаем полезную нагрузку
				it->second->http.body(entity);
				// Устанавливаем заголовки ответа
				it->second->http.headers(headers);
				// Если подключение не установлено как постоянное, но подключение долгоживущее
				if(!this->_alive && it->second->http.isAlive())
					// Указываем сколько запросов разрешено выполнить за указанный интервал времени
					it->second->http.header("Keep-Alive", this->_fmk->format("timeout=%d, max=%d", this->_timeAlive / 1000, this->_maxRequests));
				// Формируем запрос авторизации
				const auto & response = it->second->http.response(code, mess);
				// Если включён режим отладки
				#if defined(DEBUG_MODE)
					// Выводим заголовок ответа
					cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
					// Выводим параметры ответа
					cout << string(response.begin(), response.end()) << endl;
				#endif
				// Добавляем в буфер бинарные данные заголовков
				buffer.insert(buffer.end(), response.begin(), response.end());
				// Получаем данные полезной нагрузки ответа
				while(!(payload = it->second->http.payload()).empty()){
					// Если включён режим отладки
					#if defined(DEBUG_MODE)
						// Выводим сообщение о выводе чанка полезной нагрузки
						cout << this->_fmk->format("<chunk %u>", payload.size()) << endl;
					#endif
					// Добавляем в буфер бинарные данные тела сообщения
					buffer.insert(buffer.end(), payload.begin(), payload.end());
				}
				// Отправляем сообщение дочернему процессу
				const_cast <server::core_t *> (this->_core)->sendMessage(this->_worker.wid, aid, getpid(), buffer.data(), buffer.size());
			}
		// Иначе просто отправляем сообщение адъютанту
		} else {
			// Получаем параметры подключения адъютанта
			rest_worker_t::coffer_t * adj = const_cast <rest_worker_t::coffer_t *> (this->_worker.get(aid));
			// Если отправка сообщений разблокированна
			if(adj != nullptr){
				// Тело полезной нагрузки
				vector <char> payload;
				// Устанавливаем полезную нагрузку
				adj->http.body(entity);
				// Устанавливаем заголовки ответа
				adj->http.headers(headers);
				// Если подключение не установлено как постоянное, но подключение долгоживущее
				if(!this->_alive && !adj->alive && adj->http.isAlive())
					// Указываем сколько запросов разрешено выполнить за указанный интервал времени
					adj->http.header("Keep-Alive", this->_fmk->format("timeout=%d, max=%d", this->_timeAlive / 1000, this->_maxRequests));
				// Формируем запрос авторизации
				const auto & response = adj->http.response(code, mess);
				// Если включён режим отладки
				#if defined(DEBUG_MODE)
					// Выводим заголовок ответа
					cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
					// Выводим параметры ответа
					cout << string(response.begin(), response.end()) << endl;
				#endif
				// Отправляем серверу сообщение
				((awh::core_t *) const_cast <server::core_t *> (this->_core))->write(response.data(), response.size(), aid);
				// Получаем данные полезной нагрузки ответа
				while(!(payload = adj->http.payload()).empty()){
					// Если включён режим отладки
					#if defined(DEBUG_MODE)
						// Выводим сообщение о выводе чанка полезной нагрузки
						cout << this->_fmk->format("<chunk %u>", payload.size()) << endl;
					#endif
					// Отправляем тело на сервер
					((awh::core_t *) const_cast <server::core_t *> (this->_core))->write(payload.data(), payload.size(), aid);
				}
				// Устанавливаем флаг завершения работы
				adj->stopped = true;
			}
		}
	}
}
/**
 * port Метод получения порта подключения адъютанта
 * @param aid идентификатор адъютанта
 * @return    порт подключения адъютанта
 */
u_int awh::server::Rest::port(const size_t aid) const noexcept {
	// Результат работы функции
	u_int result = 0;
	// Если активирована система игольного ушка
	if(this->_needleEye && (this->_pid == getpid())){
		// Выполняем поиск идентификатора адъютанта
		auto it = this->_adjutants.find(aid);
		// Если идентификатор адъютанта найден
		if(it != this->_adjutants.end()) result = it->second->port;
	// Запрашиваем порт адъютанта у рабочего
	} else result = this->_worker.getPort(aid);
	// Выводим результат
	return result;
}
/**
 * ip Метод получения IP адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес интернет подключения адъютанта
 */
const string & awh::server::Rest::ip(const size_t aid) const noexcept {
	// Результат работы функции
	const static string result = "";
	// Если активирована система игольного ушка
	if(this->_needleEye && (this->_pid == getpid())){
		// Выполняем поиск идентификатора адъютанта
		auto it = this->_adjutants.find(aid);
		// Если идентификатор адъютанта найден
		if(it != this->_adjutants.end()) return it->second->ip;
	// Запрашиваем IP адрес адъютанта у рабочего
	} else return this->_worker.getIp(aid);
	// Выводим результат
	return result;
}
/**
 * mac Метод получения MAC адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес устройства адъютанта
 */
const string & awh::server::Rest::mac(const size_t aid) const noexcept {
	// Результат работы функции
	const static string result = "";
	// Если активирована система игольного ушка
	if(this->_needleEye && (this->_pid == getpid())){
		// Выполняем поиск идентификатора адъютанта
		auto it = this->_adjutants.find(aid);
		// Если идентификатор адъютанта найден
		if(it != this->_adjutants.end()) return it->second->mac;
	// Запрашиваем MAC адрес адъютанта у рабочего
	} else return this->_worker.getMac(aid);
	// Выводим результат
	return result;
}
/**
 * alive Метод установки долгоживущего подключения
 * @param mode флаг долгоживущего подключения
 */
void awh::server::Rest::alive(const bool mode) noexcept {
	// Устанавливаем флаг долгоживущего подключения
	this->_alive = mode;
}
/**
 * alive Метод установки времени жизни подключения
 * @param time время жизни подключения
 */
void awh::server::Rest::alive(const size_t time) noexcept {
	// Устанавливаем время жизни подключения
	this->_alive = time;
}
/**
 * alive Метод установки долгоживущего подключения
 * @param aid  идентификатор адъютанта
 * @param mode флаг долгоживущего подключения
 */
void awh::server::Rest::alive(const size_t aid, const bool mode) noexcept {
	// Получаем параметры подключения адъютанта
	rest_worker_t::coffer_t * adj = const_cast <rest_worker_t::coffer_t *> (this->_worker.get(aid));
	// Если параметры подключения адъютанта получены, устанавливаем флаг пдолгоживущего подключения
	if(adj != nullptr) adj->alive = mode;
}
/**
 * stop Метод остановки сервера
 */
void awh::server::Rest::stop() noexcept {
	// Если подключение выполнено
	if(this->_core->working())
		// Завершаем работу, если разрешено остановить
		const_cast <server::core_t *> (this->_core)->stop();
}
/**
 * start Метод запуска сервера
 */
void awh::server::Rest::start() noexcept {
	// Если биндинг не запущен, выполняем запуск биндинга
	if(!this->_core->working())
		// Выполняем запуск биндинга
		const_cast <server::core_t *> (this->_core)->start();
}
/**
 * close Метод закрытия подключения адъютанта
 * @param aid идентификатор адъютанта
 */
void awh::server::Rest::close(const size_t aid) noexcept {
	// Если активирована система игольного ушка
	if(this->_needleEye && (this->_pid == getpid())){
		// Создаём буфер данных для отправки сообщения
		vector <char> buffer(1, (char) needle_t::DISCONNECT);
		// Отправляем сообщение дочернему процессу
		const_cast <server::core_t *> (this->_core)->sendMessage(this->_worker.wid, aid, getpid(), buffer.data(), buffer.size());
	// Иначе просто отправляем сообщение адъютанту
	} else {
		// Получаем параметры подключения адъютанта
		rest_worker_t::coffer_t * adj = const_cast <rest_worker_t::coffer_t *> (this->_worker.get(aid));
		// Если параметры подключения адъютанта получены, устанавливаем флаг закрытия подключения
		if(adj != nullptr){
			// Устанавливаем флаг закрытия подключения адъютанта
			adj->close = true;
			// Выполняем отключение адъютанта
			const_cast <server::core_t *> (this->_core)->close(aid);
		}
	}
}
/**
 * multiThreads Метод активации многопоточности
 * @param threads количество потоков для активации
 * @param mode    флаг активации/деактивации мультипоточности
 */
void awh::server::Rest::multiThreads(const size_t threads, const bool mode) noexcept {
	// Выполняем активацию тредпула
	this->_threadsEnabled = mode;
	// Устанавливаем количество активных потоков
	this->_threadsCount = threads;
	// Если нужно активировать многопоточность
	if(this->_threadsEnabled)
		// Устанавливаем простое чтение базы событий
		const_cast <server::core_t *> (this->_core)->easily(true);
	// Выполняем завершение всех потоков
	else this->_thr.wait();
}
/**
 * waitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read  количество секунд для детекции по чтению
 * @param write количество секунд для детекции по записи
 */
void awh::server::Rest::waitTimeDetect(const time_t read, const time_t write) noexcept {
	// Устанавливаем количество секунд на чтение
	this->_worker.timeouts.read = read;
	// Устанавливаем количество секунд на запись
	this->_worker.timeouts.write = write;
}
/**
 * bytesDetect Метод детекции сообщений по количеству байт
 * @param read  количество байт для детекции по чтению
 * @param write количество байт для детекции по записи
 */
void awh::server::Rest::bytesDetect(const worker_t::mark_t read, const worker_t::mark_t write) noexcept {
	// Устанавливаем количество байт на чтение
	this->_worker.marker.read = read;
	// Устанавливаем количество байт на запись
	this->_worker.marker.write = write;
	// Если минимальный размер данных для чтения, не установлен
	if(this->_worker.marker.read.min == 0)
		// Устанавливаем размер минимальных для чтения данных по умолчанию
		this->_worker.marker.read.min = BUFFER_READ_MIN;
	// Если максимальный размер данных для записи не установлен, устанавливаем по умолчанию
	if(this->_worker.marker.write.max == 0)
		// Устанавливаем размер максимальных записываемых данных по умолчанию
		this->_worker.marker.write.max = BUFFER_WRITE_MAX;
}
/**
 * realm Метод установки название сервера
 * @param realm название сервера
 */
void awh::server::Rest::realm(const string & realm) noexcept {
	// Устанавливаем название сервера
	this->_realm = realm;
}
/**
 * opaque Метод установки временного ключа сессии сервера
 * @param opaque временный ключ сессии сервера
 */
void awh::server::Rest::opaque(const string & opaque) noexcept {
	// Устанавливаем временный ключ сессии сервера
	this->_opaque = opaque;
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::server::Rest::authType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Устанавливаем алгоритм шифрования для Digest авторизации
	this->_authHash = hash;
	// Устанавливаем тип авторизации
	this->_authType = type;
}
/**
 * mode Метод установки флага модуля
 * @param flag флаг модуля для установки
 */
void awh::server::Rest::mode(const u_short flag) noexcept {
	// Устанавливаем флаг ожидания входящих сообщений
	this->_worker.wait = (flag & (uint8_t) flag_t::WAITMESS);
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	const_cast <server::core_t *> (this->_core)->noInfo(flag & (uint8_t) flag_t::NOINFO);
}
/**
 * total Метод установки максимального количества одновременных подключений
 * @param total максимальное количество одновременных подключений
 */
void awh::server::Rest::total(const u_short total) noexcept {
	// Устанавливаем максимальное количество одновременных подключений
	const_cast <server::core_t *> (this->_core)->total(this->_worker.wid, total);
}
/**
 * needleEye Метод установки флага использования игольного ушка
 * @param mode флаг активации
 */
void awh::server::Rest::needleEye(const bool mode) noexcept {
	/**
	 * Если операционной системой является Nix-подобная
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Если установка производится в родительском процессе
		if(this->_pid == getpid())
			// Выполняем активацию игольного ушка
			this->_needleEye = mode;
		// Иначе выводим предупреждение
		else this->_log->print("activation of the eye of the needle is only allowed to the parent process", log_t::flag_t::WARNING);
	/**
	 * Если операционной системой является MS Windows
	 */
	#else
		// Выводим предупреждение
		this->_log->print("activation of the eye of the needle is prohibited in the MS Windows", log_t::flag_t::WARNING);
	#endif
}
/**
 * chunkSize Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::server::Rest::chunkSize(const size_t size) noexcept {
	// Устанавливаем размер чанка
	this->_chunkSize = (size > 0 ? size : BUFFER_CHUNK);
}
/**
 * maxRequests Метод установки максимального количества запросов
 * @param max максимальное количество запросов
 */
void awh::server::Rest::maxRequests(const size_t max) noexcept {
	// Устанавливаем максимальное количество запросов
	this->_maxRequests = max;
}
/**
 * compress Метод установки метода сжатия
 * @param метод сжатия сообщений
 */
void awh::server::Rest::compress(const http_t::compress_t compress) noexcept {
	// Устанавливаем метод компрессии
	this->_worker.compress = compress;
}
/**
 * keepAlive Метод установки жизни подключения
 * @param cnt   максимальное количество попыток
 * @param idle  интервал времени в секундах через которое происходит проверка подключения
 * @param intvl интервал времени в секундах между попытками
 */
void awh::server::Rest::keepAlive(const int cnt, const int idle, const int intvl) noexcept {
	// Выполняем установку максимального количества попыток
	this->_worker.keepAlive.cnt = cnt;
	// Выполняем установку интервала времени в секундах через которое происходит проверка подключения
	this->_worker.keepAlive.idle = idle;
	// Выполняем установку интервала времени в секундах между попытками
	this->_worker.keepAlive.intvl = intvl;
}
/**
 * serv Метод установки данных сервиса
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::server::Rest::serv(const string & id, const string & name, const string & ver) noexcept {
	// Устанавливаем идентификатор сервера
	this->_sid = id;
	// Устанавливаем версию сервера
	this->_ver = ver;
	// Устанавливаем название сервера
	this->_name = name;
}
/**
 * crypto Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::server::Rest::crypto(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Устанавливаем флаг шифрования
	if((this->_crypt = !pass.empty())){
		// Пароль шифрования передаваемых данных
		this->_pass = pass;
		// Соль шифрования передаваемых данных
		this->_salt = salt;
		// Размер шифрования передаваемых данных
		this->_cipher = cipher;
	}
}
/**
 * Rest Конструктор
 * @param core объект биндинга TCP/IP
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::server::Rest::Rest(const server::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 _pid(getpid()), _port(SERVER_PORT), _host(""), _nwk(fmk), _uri(fmk, &_nwk),
 _worker(fmk, log), _sid(AWH_SHORT_NAME), _ver(AWH_VERSION), _name(AWH_NAME),
 _realm(""), _opaque(""), _pass(""), _salt(""), _cipher(hash_t::cipher_t::AES128),
 _authHash(auth_t::hash_t::MD5), _authType(auth_t::type_t::NONE),
 _crypt(false), _alive(false), _needleEye(false), _chunkSize(BUFFER_CHUNK),
 _timeAlive(KEEPALIVE_TIMEOUT), _maxRequests(SERVER_MAX_REQUESTS),
 _threadsCount(0), _threadsEnabled(false), _fmk(fmk), _log(log), _core(core) {
	// Устанавливаем событие на запуск системы
	this->_worker.callback.open = std::bind(&Rest::openCallback, this, _1, _2);
	// Устанавливаем функцию персистентного вызова
	this->_worker.callback.persist = std::bind(&Rest::persistCallback, this, _1, _2, _3);
	// Устанавливаем событие подключения
	this->_worker.callback.connect = std::bind(&Rest::connectCallback, this, _1, _2, _3);
	// Устанавливаем функцию чтения данных
	this->_worker.callback.read = std::bind(&Rest::readCallback, this, _1, _2, _3, _4, _5);
	// Устанавливаем функцию записи данных
	this->_worker.callback.write = std::bind(&Rest::writeCallback, this, _1, _2, _3, _4, _5);
	// Устанавливаем событие отключения
	this->_worker.callback.disconnect = std::bind(&Rest::disconnectCallback, this, _1, _2, _3);
	// Добавляем событие аццепта адъютанта
	this->_worker.callback.accept = std::bind(&Rest::acceptCallback, this, _1, _2, _3, _4, _5);
	// Добавляем событие на получение сообщений сервера
	this->_worker.callback.mess = std::bind(&Rest::messageCallback, this, _1, _2, _3, _4, _5, _6);
	// Активируем персистентный запуск для работы пингов
	const_cast <server::core_t *> (this->_core)->persistEnable(true);
	// Добавляем воркер в биндер TCP/IP
	const_cast <server::core_t *> (this->_core)->add(&this->_worker);
}
