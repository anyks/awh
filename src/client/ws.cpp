/**
 * @file: ws.cpp
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
#include <client/ws.hpp>

/**
 * openCallback Метод обратного вызова при запуске работы
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 */
void awh::client::WebSocket::openCallback(const size_t wid, awh::core_t * core) noexcept {
	// Если дисконнекта ещё не произошло
	if(this->action == action_t::NONE){
		// Устанавливаем экшен выполнения
		this->action = action_t::OPEN;
		// Выполняем запуск обработчика событий
		this->handler();
	}
}
/**
 * persistCallback Метод персистентного вызова
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 */
void awh::client::WebSocket::persistCallback(const size_t aid, const size_t wid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((aid > 0) && (wid > 0) && (core != nullptr)){
		// Получаем текущий штамп времени
		const time_t stamp = this->fmk->unixTimestamp();
		// Если адъютант не ответил на пинг больше двух интервалов, отключаем его
		if(this->close || ((stamp - this->checkPoint) >= (PERSIST_INTERVAL * 5)))
			// Завершаем работу
			reinterpret_cast <client::core_t *> (core)->close(aid);
		// Отправляем запрос адъютанту
		else this->ping(to_string(aid));
	}
}
/**
 * connectCallback Метод обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 */
void awh::client::WebSocket::connectCallback(const size_t aid, const size_t wid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((aid > 0) && (wid > 0) && (core != nullptr)){
		// Запоминаем идентификатор адъютанта
		this->aid = aid;
		// Устанавливаем экшен выполнения
		this->action = action_t::CONNECT;
		// Выполняем запуск обработчика событий
		this->handler();
	}
}
/**
 * disconnectCallback Метод обратного вызова при отключении от сервера
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 */
void awh::client::WebSocket::disconnectCallback(const size_t aid, const size_t wid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((wid > 0) && (core != nullptr)){
		// Устанавливаем экшен выполнения
		this->action = action_t::DISCONNECT;
		// Выполняем запуск обработчика событий
		this->handler();
	}
}
/**
 * readCallback Метод обратного вызова при чтении сообщения с сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param wid    идентификатор воркера
 * @param core   объект биндинга TCP/IP
 */
void awh::client::WebSocket::readCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (aid > 0) && (wid > 0)){
		// Если дисконнекта ещё не произошло
		if((this->action == action_t::NONE) || (this->action == action_t::READ)){
			// Если подключение закрыто
			if(this->close){
				// Принудительно выполняем отключение лкиента
				reinterpret_cast <client::core_t *> (core)->close(aid);
				// Выходим из функции
				return;
			}
			// Если разрешено получение данных
			if(this->allow.receive){
				// Устанавливаем экшен выполнения
				this->action = action_t::READ;
				// Добавляем полученные данные в буфер
				this->buffer.payload.insert(this->buffer.payload.end(), buffer, buffer + size);
				// Выполняем запуск обработчика событий
				this->handler();
			}
		}
	}
}
/**
 * writeCallback Метод обратного вызова при записи сообщения на клиенте
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param wid    идентификатор воркера
 * @param core   объект биндинга TCP/IP
 */
void awh::client::WebSocket::writeCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((aid > 0) && (wid > 0) && (core != nullptr)){
		// Если необходимо выполнить закрыть подключение
		if(!this->close && this->stopped){
			// Устанавливаем флаг закрытия подключения
			this->close = !this->close;
			// Принудительно выполняем отключение лкиента
			const_cast <client::core_t *> (this->core)->close(aid);
		}
	}
}
/**
 * proxyConnectCallback Метод обратного вызова при подключении к прокси-серверу
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 */
void awh::client::WebSocket::proxyConnectCallback(const size_t aid, const size_t wid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((aid > 0) && (wid > 0) && (core != nullptr)){
		// Запоминаем идентификатор адъютанта
		this->aid = aid;
		// Устанавливаем экшен выполнения
		this->action = action_t::PROXY_CONNECT;
		// Выполняем запуск обработчика событий
		this->handler();
	}
}
/**
 * proxyReadCallback Метод обратного вызова при чтении сообщения с прокси-сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param wid    идентификатор воркера
 * @param core   объект биндинга TCP/IP
 */
void awh::client::WebSocket::proxyReadCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (aid > 0) && (wid > 0)){
		// Если дисконнекта ещё не произошло
		if((this->action == action_t::NONE) || (this->action == action_t::PROXY_READ)){
			// Устанавливаем экшен выполнения
			this->action = action_t::PROXY_READ;
			// Добавляем полученные данные в буфер
			this->buffer.payload.insert(this->buffer.payload.end(), buffer, buffer + size);
			// Выполняем запуск обработчика событий
			this->handler();
		}
	}
}
/**
 * handler Метод управления входящими методами
 */
void awh::client::WebSocket::handler() noexcept {
	// Если управляющий блокировщик не заблокирован
	if(!this->locker.mode){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->locker.mtx);
		// Выполняем блокировку обработчика
		this->locker.mode = true;
		// Выполняем обработку всех экшенов
		while(this->action != action_t::NONE){
			// Определяем обрабатываемый экшен
			switch((uint8_t) this->action){
				// Если необходимо запустить экшен открытия подключения
				case (uint8_t) action_t::OPEN: this->actionOpen(); break;
				// Если необходимо запустить экшен обработки данных поступающих с сервера
				case (uint8_t) action_t::READ: this->actionRead(); break;
				// Если необходимо запустить экшен обработки подключения к серверу
				case (uint8_t) action_t::CONNECT: this->actionConnect(); break;
				// Если необходимо запустить экшен обработки отключения от сервера
				case (uint8_t) action_t::DISCONNECT: this->actionDisconnect(); break;
				// Если необходимо запустить экшен обработки данных поступающих с прокси-сервера
				case (uint8_t) action_t::PROXY_READ: this->actionProxyRead(); break;
				// Если необходимо запустить экшен обработки подключения к прокси-серверу
				case (uint8_t) action_t::PROXY_CONNECT: this->actionProxyConnect(); break;
			}
		}
		// Выполняем разблокировку обработчика
		this->locker.mode = false;
	}
}
/**
 * actionOpen Метод обработки экшена открытия подключения
 */
void awh::client::WebSocket::actionOpen() noexcept {
	// Выполняем подключение
	const_cast <client::core_t *> (this->core)->open(this->worker.wid);
	// Если экшен соответствует, выполняем его сброс
	if(this->action == action_t::OPEN)
		// Выполняем сброс экшена
		this->action = action_t::NONE;
}
/**
 * actionRead Метод обработки экшена чтения с сервера
 */
void awh::client::WebSocket::actionRead() noexcept {
	// Объект сообщения
	mess_t mess;
	// Получаем объект биндинга ядра TCP/IP
	client::core_t * core = const_cast <client::core_t *> (this->core);
	// Если рукопожатие не выполнено
	if(!reinterpret_cast <http_t *> (&this->http)->isHandshake()){
		// Выполняем парсинг полученных данных
		const size_t bytes = this->http.parse(this->buffer.payload.data(), this->buffer.payload.size());
		// Если все данные получены
		if(this->http.isEnd()){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Получаем данные ответа
				const auto & response = reinterpret_cast <http_t *> (&this->http)->response(true);
				// Если параметры ответа получены
				if(!response.empty()){
					// Выводим заголовок ответа
					cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
					// Выводим параметры ответа
					cout << string(response.begin(), response.end()) << endl;
					// Если тело ответа существует
					if(!this->http.getBody().empty())
						// Выводим сообщение о выводе чанка тела
						cout << this->fmk->format("<body %u>", this->http.getBody().size()) << endl << endl;
					// Иначе устанавливаем перенос строки
					else cout << endl;
				}
			#endif
			// Выполняем проверку авторизации
			switch((uint8_t) this->http.getAuth()){
				// Если нужно попытаться ещё раз
				case (uint8_t) http_t::stath_t::RETRY: {
					// Если попытка повторить авторизацию ещё не проводилась
					if(this->attempts < this->totalAttempts){
						// Получаем новый адрес запроса
						this->worker.url = this->http.getUrl();
						// Если адрес запроса получен
						if(!this->worker.url.empty()){
							// Увеличиваем количество попыток
							this->attempts++;
							// Выполняем очистку оставшихся данных
							this->buffer.payload.clear();
							// Если соединение является постоянным
							if(this->http.isAlive())
								// Устанавливаем новый экшен выполнения
								this->action = action_t::CONNECT;
							// Завершаем работу
							else {
								// Если экшен соответствует, выполняем его сброс
								if(this->action == action_t::READ)
									// Выполняем сброс экшена
									this->action = action_t::NONE;
								// Завершаем работу
								core->close(this->aid);
							}
							// Завершаем работу
							return;
						}
					}
					// Устанавливаем код ответа
					this->code = 403;
					// Создаём сообщение
					mess = mess_t(this->code, this->http.getMessage(this->code));
					// Выводим сообщение
					this->error(mess);
				} break;
				// Если запрос выполнен удачно
				case (uint8_t) http_t::stath_t::GOOD: {
					// Если рукопожатие выполнено
					if(this->http.isHandshake()){
						// Выполняем сброс количества попыток
						this->attempts = 0;
						// Очищаем список фрагментированных сообщений
						this->buffer.fragmes.clear();
						// Получаем флаг шифрованных данных
						this->crypt = this->http.isCrypt();
						// Получаем поддерживаемый метод компрессии
						this->compress = this->http.getCompress();
						// Обновляем контрольную точку
						this->checkPoint = this->fmk->unixTimestamp();
						// Устанавливаем размер скользящего окна
						this->hash.setWbit(this->http.getWbitServer());
						// Если разрешено выполнять перехват контекста компрессии для клиента
						if(this->http.getClientTakeover())
							// Разрешаем перехватывать контекст компрессии для клиента
							this->hash.setTakeoverCompress(true);
						// Если разрешено выполнять перехват контекста компрессии для сервера
						if(this->http.getServerTakeover())
							// Разрешаем перехватывать контекст компрессии для сервера
							this->hash.setTakeoverDecompress(true);
						// Выводим в лог сообщение
						if(!this->noinfo) this->log->print("authorization on the WebSocket server was successful", log_t::flag_t::INFO);
						// Если функция обратного вызова установлена, выполняем
						if(this->activeFn != nullptr) this->activeFn(mode_t::CONNECT, this);
						// Есла данных передано больше чем обработано
						if(this->buffer.payload.size() > bytes)
							// Удаляем количество обработанных байт
							this->buffer.payload.assign(this->buffer.payload.begin() + bytes, this->buffer.payload.end());
							// vector <decltype(this->buffer.payload)::value_type> (this->buffer.payload.begin() + bytes, this->buffer.payload.end()).swap(this->buffer.payload);
						// Если данных в буфере больше нет
						else {
							// Очищаем буфер собранных данных
							this->buffer.payload.clear();
							// Если экшен соответствует, выполняем его сброс
							if(this->action == action_t::READ)
								// Выполняем сброс экшена
								this->action = action_t::NONE;
						}
						// Завершаем работу
						return;
					// Сообщаем, что рукопожатие не выполнено
					} else {
						// Устанавливаем код ответа
						this->code = 404;
						// Создаём сообщение
						mess = mess_t(this->code, this->http.getMessage(this->code));
						// Выводим сообщение
						this->error(mess);
					}
				} break;
				// Если запрос неудачный
				case (uint8_t) http_t::stath_t::FAULT: {
					// Получаем параметры запроса
					const auto & query = this->http.getQuery();
					// Устанавливаем код ответа
					this->code = query.code;
					// Создаём сообщение
					mess = mess_t(this->code, query.message);
					// Запрещаем бесконечный редирект при запросе авторизации
					if((this->code == 401) || (this->code == 407)){
						// Устанавливаем код ответа
						this->code = 403;
						// Присваиваем сообщению, новое значение кода
						mess = this->code;
					}
					// Выводим сообщение
					this->error(mess);
				} break;
			}
			// Если экшен соответствует, выполняем его сброс
			if(this->action == action_t::READ)
				// Выполняем сброс экшена
				this->action = action_t::NONE;
			// Выполняем сброс количества попыток
			this->attempts = 0;
			// Завершаем работу
			core->close(this->aid);
		// Если экшен соответствует, выполняем его сброс
		} else if(this->action == action_t::READ)
			// Выполняем сброс экшена
			this->action = action_t::NONE;
		// Завершаем работу
		return;
	// Если рукопожатие выполнено
	} else if(this->allow.receive) {
		// Флаг удачного получения данных
		bool receive = false;
		// Создаём буфер сообщения
		vector <char> buffer;
		// Создаём объект шапки фрейма
		frame_t::head_t head;
		// Выполняем обработку полученных данных
		while(!this->close && this->allow.receive){
			// Выполняем чтение фрейма WebSocket
			const auto & data = this->frame.get(head, this->buffer.payload.data(), this->buffer.payload.size());
			// Если буфер данных получен
			if(!data.empty()){
				// Проверяем состояние флагов RSV2 и RSV3
				if(head.rsv[1] || head.rsv[2]){
					// Создаём сообщение
					mess = mess_t(1002, "RSV2 and RSV3 must be clear");
					// Выводим сообщение
					this->error(mess);
					// Выполняем реконнект
					goto Reconnect;
				}
				// Если флаг компресси включён а данные пришли не сжатые
				if(head.rsv[0] && ((this->compress == http_t::compress_t::NONE) ||
				  (head.optcode == frame_t::opcode_t::CONTINUATION) ||
				  (((uint8_t) head.optcode > 0x07) && ((uint8_t) head.optcode < 0x0b)))){
					// Создаём сообщение
					mess = mess_t(1002, "RSV1 must be clear");
					// Выводим сообщение
					this->error(mess);
					// Выполняем реконнект
					goto Reconnect;
				}
				// Если опкоды требуют финального фрейма
				if(!head.fin && ((uint8_t) head.optcode > 0x07) && ((uint8_t) head.optcode < 0x0b)){
					// Создаём сообщение
					mess = mess_t(1002, "FIN must be set");
					// Выводим сообщение
					this->error(mess);
					// Выполняем реконнект
					goto Reconnect;
				}
				// Определяем тип ответа
				switch((uint8_t) head.optcode){
					// Если ответом является PING
					case (uint8_t) frame_t::opcode_t::PING:
						// Отправляем ответ серверу
						this->pong(string(data.begin(), data.end()));
					break;
					// Если ответом является PONG
					case (uint8_t) frame_t::opcode_t::PONG:
						// Если идентификатор адъютанта совпадает
						if(memcmp(to_string(aid).c_str(), data.data(), data.size()) == 0)
							// Обновляем контрольную точку
							this->checkPoint = this->fmk->unixTimestamp();
					break;
					// Если ответом является TEXT
					case (uint8_t) frame_t::opcode_t::TEXT:
					// Если ответом является BINARY
					case (uint8_t) frame_t::opcode_t::BINARY: {
						// Запоминаем полученный опкод
						this->opcode = head.optcode;
						// Запоминаем, что данные пришли сжатыми
						this->compressed = (head.rsv[0] && (this->compress != http_t::compress_t::NONE));
						// Если сообщение замаскированно
						if(head.mask){
							// Создаём сообщение
							mess = mess_t(1002, "masked frame from server");
							// Выводим сообщение
							this->error(mess);
							// Выполняем реконнект
							goto Reconnect;
						// Если список фрагментированных сообщений существует
						} else if(!this->buffer.fragmes.empty()) {
							// Очищаем список фрагментированных сообщений
							this->buffer.fragmes.clear();
							// Создаём сообщение
							mess = mess_t(1002, "opcode for subsequent fragmented messages should not be set");
							// Выводим сообщение
							this->error(mess);
							// Выполняем реконнект
							goto Reconnect;
						// Если сообщение является не последнем
						} else if(!head.fin)
							// Заполняем фрагментированное сообщение
							this->buffer.fragmes.insert(this->buffer.fragmes.end(), data.begin(), data.end());
						// Если сообщение является последним
						else buffer = move(* const_cast <vector <char> *> (&data));
					} break;
					// Если ответом является CONTINUATION
					case (uint8_t) frame_t::opcode_t::CONTINUATION: {
						// Заполняем фрагментированное сообщение
						this->buffer.fragmes.insert(this->buffer.fragmes.end(), data.begin(), data.end());
						// Если сообщение является последним
						if(head.fin){
							// Выполняем копирование всех собранных сегментов
							buffer = move(* const_cast <vector <char> *> (&this->buffer.fragmes));
							// Очищаем список фрагментированных сообщений
							this->buffer.fragmes.clear();
						}
					} break;
					// Если ответом является CLOSE
					case (uint8_t) frame_t::opcode_t::CLOSE: {
						// Извлекаем сообщение
						mess = this->frame.message(data);
						// Выводим сообщение
						this->error(mess);
						// Выполняем реконнект
						goto Reconnect;
					} break;
				}
			}
			// Если парсер обработал какое-то количество байт
			if((receive = ((head.frame > 0) && !this->buffer.payload.empty()))){
				// Если размер буфера больше количества удаляемых байт
				if((receive = (this->buffer.payload.size() >= head.frame)))
					// Удаляем количество обработанных байт
					this->buffer.payload.assign(this->buffer.payload.begin() + head.frame, this->buffer.payload.end());
					// vector <decltype(this->buffer.payload)::value_type> (this->buffer.payload.begin() + head.frame, this->buffer.payload.end()).swap(this->buffer.payload);
			}
			// Если сообщения получены
			if(!buffer.empty()){
				// Если тредпул активирован
				if(this->thr.is())
					// Добавляем в тредпул новую задачу на извлечение полученных сообщений
					this->thr.push(std::bind(&ws_t::extraction, this, buffer, (this->opcode == frame_t::opcode_t::TEXT)));
				// Если тредпул не активирован, выполняем извлечение полученных сообщений
				else this->extraction(buffer, (this->opcode == frame_t::opcode_t::TEXT));
				// Очищаем буфер полученного сообщения
				buffer.clear();
			}
			// Если данные мы все получили, выходим
			if(!receive || this->buffer.payload.empty()) break;
		}
		// Если экшен соответствует, выполняем его сброс
		if(this->action == action_t::READ)
			// Выполняем сброс экшена
			this->action = action_t::NONE;
		// Выходим из функции
		return;
	}
	// Устанавливаем метку реконнекта
	Reconnect:
	// Выполняем отправку сообщения об ошибке
	this->sendError(mess);
	// Если экшен соответствует, выполняем его сброс
	if(this->action == action_t::READ)
		// Выполняем сброс экшена
		this->action = action_t::NONE;
}
/**
 * actionConnect Метод обработки экшена подключения к серверу
 */
void awh::client::WebSocket::actionConnect() noexcept {
	// Выполняем сброс параметров запроса
	this->flush();
	// Выполняем сброс состояния HTTP парсера
	this->http.reset();
	// Выполняем очистку параметров HTTP запроса
	this->http.clear();
	// Устанавливаем метод сжатия
	this->http.setCompress(this->compress);
	// Разрешаем перехватывать контекст для клиента
	this->http.setClientTakeover(this->takeOverCli);
	// Разрешаем перехватывать контекст для сервера
	this->http.setServerTakeover(this->takeOverSrv);
	// Разрешаем перехватывать контекст компрессии
	this->hash.setTakeoverCompress(this->takeOverCli);
	// Разрешаем перехватывать контекст декомпрессии
	this->hash.setTakeoverDecompress(this->takeOverSrv);
	// Получаем бинарные данные REST запроса
	const auto & buffer = this->http.request(this->worker.url);
	// Если бинарные данные запроса получены
	if(!buffer.empty()){
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим заголовок запроса
			cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << endl;
			// Выводим параметры запроса
			cout << string(buffer.begin(), buffer.end()) << endl << endl;
		#endif
		// Выполняем отправку сообщения на сервер
		const_cast <client::core_t *> (this->core)->write(buffer.data(), buffer.size(), this->aid);
	}
	// Если экшен соответствует, выполняем его сброс
	if(this->action == action_t::CONNECT)
		// Выполняем сброс экшена
		this->action = action_t::NONE;
}
/**
 * actionDisconnect Метод обработки экшена отключения от сервера
 */
void awh::client::WebSocket::actionDisconnect() noexcept {
	// Если нужно произвести запрос заново
	if((this->code == 301) || (this->code == 308) ||
	   (this->code == 401) || (this->code == 407)){
		// Если статус ответа требует произвести авторизацию или заголовок перенаправления указан
		if((this->code == 401) || (this->code == 407) || this->http.isHeader("location")){
			// Выполняем установку следующего экшена на открытие подключения
			this->action = action_t::OPEN;
			// Выходим из функции
			return;
		}
	}
	// Выполняем сброс параметров запроса
	this->flush();
	// Очищаем код ответа
	this->code = 0;
	// Завершаем работу
	if(this->unbind) const_cast <client::core_t *> (this->core)->stop();
	// Если экшен соответствует, выполняем его сброс
	if(this->action == action_t::DISCONNECT)
		// Выполняем сброс экшена
		this->action = action_t::NONE;
}
/**
 * actionProxyRead Метод обработки экшена чтения с прокси-сервера
 */
void awh::client::WebSocket::actionProxyRead() noexcept {
	// Получаем объект биндинга ядра TCP/IP
	client::core_t * core = const_cast <client::core_t *> (this->core);
	// Определяем тип прокси-сервера
	switch((uint8_t) this->worker.proxy.type){
		// Если прокси-сервер является Socks5
		case (uint8_t) proxy_t::type_t::SOCKS5: {
			// Если данные не получены
			if(!this->worker.proxy.socks5.isEnd()){
				// Выполняем парсинг входящих данных
				this->worker.proxy.socks5.parse(this->buffer.payload.data(), this->buffer.payload.size());
				// Получаем данные запроса
				const auto & buffer = this->worker.proxy.socks5.get();
				// Если данные получены
				if(!buffer.empty()){
					// Выполняем очистку буфера данных
					this->buffer.payload.clear();
					// Выполняем отправку сообщения на сервер
					core->write(buffer.data(), buffer.size(), this->aid);
					// Если экшен соответствует, выполняем его сброс
					if(this->action == action_t::PROXY_READ)
						// Выполняем сброс экшена
						this->action = action_t::NONE;
					// Завершаем работу
					return;
				// Если данные все получены
				} else if(this->worker.proxy.socks5.isEnd()) {
					// Выполняем очистку буфера данных
					this->buffer.payload.clear();
					// Если рукопожатие выполнено
					if(this->worker.proxy.socks5.isHandshake()){
						// Выполняем переключение на работу с сервером
						core->switchProxy(this->aid);
						// Если экшен соответствует, выполняем его сброс
						if(this->action == action_t::PROXY_READ)
							// Выполняем сброс экшена
							this->action = action_t::NONE;
						// Завершаем работу
						return;
					// Если рукопожатие не выполнено
					} else {
						// Устанавливаем код ответа
						this->code = this->worker.proxy.socks5.getCode();
						// Создаём сообщение
						mess_t mess(this->code);
						// Устанавливаем сообщение ответа
						mess = this->worker.proxy.socks5.getMessage(this->code);
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Если заголовки получены
							if(!mess.text.empty()){
								// Данные REST ответа
								const string & response = this->fmk->format("SOCKS5 %u %s\r\n", this->code, mess.text.c_str());
								// Выводим заголовок ответа
								cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE PROXY ^^^^^^^^^\x1B[0m" << endl;
								// Выводим параметры ответа
								cout << string(response.begin(), response.end()) << endl;
							}
						#endif
						// Выводим сообщение
						this->error(mess);
						// Если экшен соответствует, выполняем его сброс
						if(this->action == action_t::PROXY_READ)
							// Выполняем сброс экшена
							this->action = action_t::NONE;
						// Завершаем работу
						core->close(this->aid);
						// Завершаем работу
						return;
					}
				}
			}
		} break;
		// Если прокси-сервер является HTTP
		case (uint8_t) proxy_t::type_t::HTTP: {
			// Выполняем парсинг полученных данных
			this->worker.proxy.http.parse(this->buffer.payload.data(), this->buffer.payload.size());
			// Если все данные получены
			if(this->worker.proxy.http.isEnd()){
				// Получаем параметры запроса
				const auto & query = this->worker.proxy.http.getQuery();
				// Устанавливаем код ответа
				this->code = query.code;
				// Создаём сообщение
				mess_t mess(this->code, query.message);
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Получаем данные ответа
					const auto & response = this->worker.proxy.http.response(true);
					// Если параметры ответа получены
					if(!response.empty()){
						// Выводим заголовок ответа
						cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE PROXY ^^^^^^^^^\x1B[0m" << endl;
						// Выводим параметры ответа
						cout << string(response.begin(), response.end()) << endl;
						// Если тело ответа существует
						if(!this->worker.proxy.http.getBody().empty())
							// Выводим сообщение о выводе чанка тела
							cout << this->fmk->format("<body %u>", this->worker.proxy.http.getBody().size())  << endl;
					}
				#endif
				// Выполняем проверку авторизации
				switch((uint8_t) this->worker.proxy.http.getAuth()){
					// Если нужно попытаться ещё раз
					case (uint8_t) http_t::stath_t::RETRY: {
						// Если попытка повторить авторизацию ещё не проводилась
						if(this->attempts < this->totalAttempts){
							// Получаем новый адрес запроса
							this->worker.proxy.url = this->worker.proxy.http.getUrl();
							// Если адрес запроса получен
							if(!this->worker.proxy.url.empty()){
								// Увеличиваем количество попыток
								this->attempts++;
								// Если соединение является постоянным
								if(this->worker.proxy.http.isAlive())
									// Устанавливаем новый экшен выполнения
									this->action = action_t::PROXY_CONNECT;
								// Если соединение должно быть закрыто
								else {
									// Если экшен соответствует, выполняем его сброс
									if(this->action == action_t::PROXY_READ)
										// Выполняем сброс экшена
										this->action = action_t::NONE;
									// Завершаем работу
									core->close(this->aid);
								}
								// Завершаем работу
								return;
							}
						}
						// Устанавливаем код ответа
						this->code = 403;
						// Присваиваем сообщению, новое значение кода
						mess = this->code;
					} break;
					// Если запрос выполнен удачно
					case (uint8_t) http_t::stath_t::GOOD: {
						// Выполняем сброс количества попыток
						this->attempts = 0;
						// Выполняем переключение на работу с сервером
						core->switchProxy(this->aid);
						// Если экшен соответствует, выполняем его сброс
						if(this->action == action_t::PROXY_READ)
							// Выполняем сброс экшена
							this->action = action_t::NONE;
						// Завершаем работу
						return;
					} break;
					// Если запрос неудачный
					case (uint8_t) http_t::stath_t::FAULT: {
						// Запрещаем бесконечный редирект при запросе авторизации
						if((this->code == 401) || (this->code == 407)){
							// Устанавливаем код ответа
							this->code = 403;
							// Присваиваем сообщению, новое значение кода
							mess = this->code;
						}
					} break;
				}
				// Выводим сообщение
				this->error(mess);
				// Если экшен соответствует, выполняем его сброс
				if(this->action == action_t::PROXY_READ)
					// Выполняем сброс экшена
					this->action = action_t::NONE;
				// Выполняем сброс количества попыток
				this->attempts = 0;
				// Завершаем работу
				core->close(this->aid);
				// Завершаем работу
				return;
			}
		} break;
		// Иначе завершаем работу
		default: {
			// Если экшен соответствует, выполняем его сброс
			if(this->action == action_t::PROXY_READ)
				// Выполняем сброс экшена
				this->action = action_t::NONE;
			// Завершаем работу
			core->close(this->aid);
		}
	}
}
/**
 * actionProxyConnect Метод обработки экшена подключения к прокси-серверу
 */
void awh::client::WebSocket::actionProxyConnect() noexcept {
	// Получаем объект биндинга ядра TCP/IP
	client::core_t * core = const_cast <client::core_t *> (this->core);
	// Определяем тип прокси-сервера
	switch((uint8_t) this->worker.proxy.type){
		// Если прокси-сервер является Socks5
		case (uint8_t) proxy_t::type_t::SOCKS5: {
			// Выполняем сброс состояния Socks5 парсера
			this->worker.proxy.socks5.reset();
			// Устанавливаем URL адрес запроса
			this->worker.proxy.socks5.setUrl(this->worker.url);
			// Выполняем создание буфера запроса
			this->worker.proxy.socks5.parse();
			// Получаем данные запроса
			const auto & buffer = this->worker.proxy.socks5.get();
			// Если данные получены
			if(!buffer.empty())
				// Выполняем отправку сообщения на сервер
				core->write(buffer.data(), buffer.size(), this->aid);
		} break;
		// Если прокси-сервер является HTTP
		case (uint8_t) proxy_t::type_t::HTTP: {
			// Выполняем сброс состояния HTTP парсера
			this->worker.proxy.http.reset();
			// Выполняем очистку параметров HTTP запроса
			this->worker.proxy.http.clear();
			// Получаем бинарные данные REST запроса
			const auto & buffer = this->worker.proxy.http.proxy(this->worker.url);
			// Если бинарные данные запроса получены
			if(!buffer.empty()){
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Выводим заголовок запроса
					cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST PROXY ^^^^^^^^^\x1B[0m" << endl;
					// Выводим параметры запроса
					cout << string(buffer.begin(), buffer.end()) << endl;
				#endif
				// Выполняем отправку сообщения на сервер
				core->write(buffer.data(), buffer.size(), this->aid);
			}
		} break;
		// Иначе завершаем работу
		default: core->close(this->aid);
	}
	// Если экшен соответствует, выполняем его сброс
	if(this->action == action_t::PROXY_CONNECT)
		// Выполняем сброс экшена
		this->action = action_t::NONE;
}
/**
 * error Метод вывода сообщений об ошибках работы клиента
 * @param message сообщение с описанием ошибки
 */
void awh::client::WebSocket::error(const mess_t & message) const noexcept {
	// Очищаем список буффер бинарных данных
	const_cast <ws_t *> (this)->buffer.payload.clear();
	// Очищаем список фрагментированных сообщений
	const_cast <ws_t *> (this)->buffer.fragmes.clear();
	// Если код ошибки указан
	if(message.code > 0){
		// Если сообщение об ошибке пришло
		if(!message.text.empty()){
			// Если тип сообщения получен
			if(!message.type.empty())
				// Выводим в лог сообщение
				this->log->print("%s - %s [%u]", log_t::flag_t::WARNING, message.type.c_str(), message.text.c_str(), message.code);
			// Иначе выводим сообщение в упрощёном виде
			else this->log->print("%s [%u]", log_t::flag_t::WARNING, message.text.c_str(), message.code);
			// Если функция обратного вызова установлена, выводим полученное сообщение
			if(this->errorFn != nullptr) this->errorFn(message.code, message.text, const_cast <WebSocket *> (this));
		}
	}
}
/**
 * extraction Метод извлечения полученных данных
 * @param buffer данные в чистом виде полученные с сервера
 * @param utf8   данные передаются в текстовом виде
 */
void awh::client::WebSocket::extraction(const vector <char> & buffer, const bool utf8) noexcept {
	// Выполняем блокировку потока	
	const lock_guard <recursive_mutex> lock(this->mtx);
	// Если буфер данных передан
	if(!buffer.empty() && !this->freeze && (this->messageFn != nullptr)){
		// Если данные пришли в сжатом виде
		if(this->compressed && (this->compress != http_t::compress_t::NONE)){
			// Декомпрессионные данные
			vector <char> data;
			// Определяем метод компрессии
			switch((uint8_t) this->compress){
				// Если метод компрессии выбран Deflate
				case (uint8_t) http_t::compress_t::DEFLATE: {
					// Добавляем хвост в полученные данные
					this->hash.setTail(* const_cast <vector <char> *> (&buffer));
					// Выполняем декомпрессию полученных данных
					data = this->hash.decompress(buffer.data(), buffer.size());
				} break;
				// Если метод компрессии выбран GZip
				case (uint8_t) http_t::compress_t::GZIP:
					// Выполняем декомпрессию полученных данных
					data = this->hash.decompressGzip(buffer.data(), buffer.size());
				break;
				// Если метод компрессии выбран Brotli
				case (uint8_t) http_t::compress_t::BROTLI:
					// Выполняем декомпрессию полученных данных
					data = this->hash.decompressBrotli(buffer.data(), buffer.size());
				break;
			}
			// Если данные получены
			if(!data.empty()){
				// Если нужно производить дешифрование
				if(this->crypt){
					// Выполняем шифрование переданных данных
					const auto & res = this->hash.decrypt(data.data(), data.size());
					// Отправляем полученный результат
					if(!res.empty()) this->messageFn(res, utf8, const_cast <WebSocket *> (this));
					// Иначе выводим сообщение так - как оно пришло
					else this->messageFn(data, utf8, const_cast <WebSocket *> (this));
				// Отправляем полученный результат
				} else this->messageFn(data, utf8, const_cast <WebSocket *> (this));
			// Выводим сообщение об ошибке
			} else {
				// Создаём сообщение
				mess_t mess(1007, "received data decompression error");
				// Выводим сообщение
				this->error(mess);
				// Иначе выводим сообщение так - как оно пришло
				this->messageFn(buffer, utf8, const_cast <WebSocket *> (this));
				// Выполняем отправку сообщения об ошибке
				this->sendError(mess);
			}
		// Если функция обратного вызова установлена, выводим полученное сообщение
		} else {
			// Если нужно производить дешифрование
			if(this->crypt){
				// Выполняем шифрование переданных данных
				const auto & res = this->hash.decrypt(buffer.data(), buffer.size());
				// Отправляем полученный результат
				if(!res.empty()) this->messageFn(res, utf8, const_cast <WebSocket *> (this));
				// Иначе выводим сообщение так - как оно пришло
				else this->messageFn(buffer, utf8, const_cast <WebSocket *> (this));
			// Отправляем полученный результат
			} else this->messageFn(buffer, utf8, const_cast <WebSocket *> (this));
		}
	}
}
/**
 * flush Метод сброса параметров запроса
 */
void awh::client::WebSocket::flush() noexcept {
	// Снимаем флаг отключения
	this->close = false;
	// Снимаем флаг принудительной остановки
	this->stopped = false;
	// Устанавливаем флаг разрешающий обмен данных
	this->allow = allow_t();
	// Очищаем буфер данных
	this->buffer = buffer_t();
}
/**
 * pong Метод ответа на проверку о доступности сервера
 * @param message сообщение для отправки
 */
void awh::client::WebSocket::pong(const string & message) noexcept {
	// Если подключение выполнено
	if(this->core->working() && this->allow.send){
		// Если рукопожатие выполнено
		if(this->http.isHandshake() && (this->aid > 0)){
			// Создаём буфер для отправки
			const auto & buffer = this->frame.pong(message, true);
			// Если бинарный буфер получен
			if(!buffer.empty())
				// Выполняем отправку сообщения на сервер
				const_cast <client::core_t *> (this->core)->write(buffer.data(), buffer.size(), this->aid);
		}
	}
}
/**
 * ping Метод проверки доступности сервера
 * @param message сообщение для отправки
 */
void awh::client::WebSocket::ping(const string & message) noexcept {
	// Если подключение выполнено
	if(this->core->working() && this->allow.send){
		// Если рукопожатие выполнено
		if(this->http.isHandshake() && (this->aid > 0)){
			// Создаём буфер для отправки
			const auto & buffer = this->frame.ping(message, true);
			// Если бинарный буфер получен
			if(!buffer.empty())
				// Выполняем отправку сообщения на сервер
				const_cast <client::core_t *> (this->core)->write(buffer.data(), buffer.size(), this->aid);
		}
	}
}
/**
 * init Метод инициализации WebSocket клиента
 * @param url      адрес WebSocket сервера
 * @param compress метод компрессии передаваемых сообщений
 */
void awh::client::WebSocket::init(const string & url, const http_t::compress_t compress) noexcept {
	// Если unix-сокет установлен
	if(this->core->family() == core_t::family_t::NIX){
		// Выполняем очистку воркера
		this->worker.clear();
		// Устанавливаем метод компрессии сообщений
		this->compress = compress;
		// Устанавливаем URL адрес запроса (как заглушка)
		this->worker.url = this->uri.parseUrl("ws://unixsocket");
		/**
		 * Если операционной системой не является Windows
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Выполняем установку unix-сокета 
			const_cast <client::core_t *> (this->core)->setUnixSocket(url);
		#endif
	// Выполняем установку unix-сокет
	} else {
		// Если адрес сервера передан
		if(!url.empty()){
			// Выполняем очистку воркера
			this->worker.clear();
			// Устанавливаем URL адрес запроса
			this->worker.url = this->uri.parseUrl(url);
			/**
			 * Если операционной системой не является Windows
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
				// Удаляем unix-сокет ранее установленный
				const_cast <client::core_t *> (this->core)->unsetUnixSocket();
			#endif
		}
	}
	// Устанавливаем метод компрессии сообщений
	this->compress = compress;
}
/**
 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket::on(function <void (const mode_t, WebSocket *)> callback) noexcept {
	// Устанавливаем функцию запуска и остановки
	this->activeFn = callback;
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибок
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket::on(function <void (const u_int, const string &, WebSocket *)> callback) noexcept {
	// Устанавливаем функцию получения ошибок
	this->errorFn = callback;
}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket::on(function <void (const vector <char> &, const bool, WebSocket *)> callback) noexcept {
	// Устанавливаем функцию получения сообщений с сервера
	this->messageFn = callback;
}
/**
 * sendTimeout Метод отправки сигнала таймаута
 */
void awh::client::WebSocket::sendTimeout() noexcept {
	// Если подключение выполнено
	if(this->core->working() && this->allow.send)
		// Отправляем сигнал принудительного таймаута
		const_cast <client::core_t *> (this->core)->sendTimeout(this->aid);
}
/**
 * sendError Метод отправки сообщения об ошибке
 * @param mess отправляемое сообщение об ошибке
 */
void awh::client::WebSocket::sendError(const mess_t & mess) noexcept {
	// Если подключение выполнено
	if(this->core->working() && this->allow.send && (this->aid > 0)){
		// Запрещаем получение данных
		this->allow.receive = false;
		// Получаем объект биндинга ядра TCP/IP
		client::core_t * core = const_cast <client::core_t *> (this->core);
		// Выполняем остановку получения данных
		core->disabled(core_t::method_t::READ, this->aid);
		// Если код ошибки относится к WebSocket
		if(mess.code >= 1000){
			// Получаем буфер сообщения
			const auto & buffer = this->frame.message(mess);
			// Если данные сообщения получены
			if(!buffer.empty()){
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Выводим заголовок ответа
					cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
					// Выводим отправляемое сообщение
					cout << this->fmk->format("%s [%u]", mess.text.c_str(), mess.code) << endl << endl;
				#endif
				// Устанавливаем флаг принудительной остановки
				this->stopped = true;
				// Выполняем отправку сообщения на сервер
				core->write(buffer.data(), buffer.size(), this->aid);
				// Выходим из функции
				return;
			}
		}
		// Завершаем работу
		const_cast <client::core_t *> (this->core)->close(this->aid);
	}
}
/**
 * send Метод отправки сообщения на сервер
 * @param message буфер сообщения в бинарном виде
 * @param size    размер сообщения в байтах
 * @param utf8    данные передаются в текстовом виде
 */
void awh::client::WebSocket::send(const char * message, const size_t size, const bool utf8) noexcept {
	// Если подключение выполнено
	if(this->core->working() && this->allow.send){
		// Выполняем блокировку отправки сообщения
		this->allow.send = !this->allow.send;
		// Если рукопожатие выполнено
		if((message != nullptr) && (size > 0) && this->http.isHandshake() && (this->aid > 0)){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим заголовок ответа
				cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
				// Если отправляемое сообщение является текстом
				if(utf8)
					// Выводим параметры ответа
					cout << string(message, size) << endl << endl;
				// Выводим сообщение о выводе чанка полезной нагрузки
				else cout << this->fmk->format("<bytes %u>", size) << endl << endl;
			#endif
			// Буфер сжатых данных
			vector <char> buffer;
			// Создаём объект заголовка для отправки
			frame_t::head_t head(true, true);
			// Если нужно производить шифрование
			if(this->crypt){
				// Выполняем шифрование переданных данных
				buffer = this->hash.encrypt(message, size);
				// Если данные зашифрованны
				if(!buffer.empty()){
					// Заменяем сообщение для передачи
					message = buffer.data();
					// Заменяем размер сообщения
					(* const_cast <size_t *> (&size)) = buffer.size();
				}
			}
			// Устанавливаем опкод сообщения
			head.optcode = (utf8 ? frame_t::opcode_t::TEXT : frame_t::opcode_t::BINARY);
			// Указываем, что сообщение передаётся в сжатом виде
			head.rsv[0] = ((size >= 1024) && (this->compress != http_t::compress_t::NONE));
			// Если необходимо сжимать сообщение перед отправкой
			if(head.rsv[0]){
				// Компрессионные данные
				vector <char> data;
				// Определяем метод компрессии
				switch((uint8_t) this->compress){
					// Если метод компрессии выбран Deflate
					case (uint8_t) http_t::compress_t::DEFLATE: {
						// Выполняем компрессию полученных данных
						data = this->hash.compress(message, size);
						// Удаляем хвост в полученных данных
						this->hash.rmTail(data);
					} break;
					// Если метод компрессии выбран GZip
					case (uint8_t) http_t::compress_t::GZIP:
						// Выполняем компрессию полученных данных
						data = this->hash.compressGzip(message, size);
					break;
					// Если метод компрессии выбран Brotli
					case (uint8_t) http_t::compress_t::BROTLI:
						// Выполняем компрессию полученных данных
						data = this->hash.compressBrotli(message, size);
					break;
				}
				// Если сжатие данных прошло удачно
				if(!data.empty()){
					// Выполняем перемещение данных
					buffer = move(data);
					// Заменяем сообщение для передачи
					message = buffer.data();
					// Заменяем размер сообщения
					(* const_cast <size_t *> (&size)) = buffer.size();
				// Снимаем флаг сжатых данных
				} else head.rsv[0] = false;
			}
			// Если требуется фрагментация сообщения
			if(size > this->frameSize){
				// Бинарный буфер чанка данных
				vector <char> chunk(this->frameSize);
				// Смещение в бинарном буфере
				size_t start = 0, stop = this->frameSize;
				// Выполняем разбивку полезной нагрузки на сегменты
				while(stop < size){
					// Увеличиваем длину чанка
					stop += this->frameSize;
					// Если длина чанка слишком большая, компенсируем
					stop = (stop > size ? size : stop);
					// Устанавливаем флаг финального сообщения
					head.fin = (stop == size);
					// Формируем чанк бинарных данных
					chunk.assign(message + start, message + stop);
					// Создаём буфер для отправки
					const auto & buffer = this->frame.set(head, chunk.data(), chunk.size());
					// Если бинарный буфер для отправки данных получен
					if(!buffer.empty())
						// Отправляем серверу сообщение
						const_cast <client::core_t *> (this->core)->write(buffer.data(), buffer.size(), aid);
					// Выполняем сброс RSV1
					head.rsv[0] = false;
					// Устанавливаем опкод сообщения
					head.optcode = frame_t::opcode_t::CONTINUATION;
					// Увеличиваем смещение в буфере
					start = stop;
				}
			// Если фрагментация сообщения не требуется
			} else {
				// Создаём буфер для отправки
				const auto & buffer = this->frame.set(head, message, size);
				// Если бинарный буфер для отправки данных получен
				if(!buffer.empty())
					// Отправляем серверу сообщение
					const_cast <client::core_t *> (this->core)->write(buffer.data(), buffer.size(), aid);
			}
		}
		// Выполняем разблокировку отправки сообщения
		this->allow.send = !this->allow.send;
	}
}
/**
 * stop Метод остановки клиента
 */
void awh::client::WebSocket::stop() noexcept {
	// Очищаем код ответа
	this->code = 0;
	// Если подключение выполнено
	if(this->core->working()){
		// Завершаем работу, если разрешено остановить
		if(this->unbind) const_cast <client::core_t *> (this->core)->stop();
		// Если завершать работу запрещено, просто отключаемся
		else {
			/**
			 * Если установлено постоянное подключение
			 * нам нужно заблокировать автоматический реконнект.
			 */
			// Считываем значение флага
			const bool alive = this->worker.alive;
			// Выполняем отключение флага постоянного подключения
			this->worker.alive = false;
			// Выполняем отключение клиента
			const_cast <client::core_t *> (this->core)->close(this->aid);
			// Восстанавливаем предыдущее значение флага
			this->worker.alive = alive;
		}
		// Если функция обратного вызова установлена, выполняем
		if(this->activeFn != nullptr) this->activeFn(mode_t::DISCONNECT, this);
	}
}
/**
 * start Метод запуска клиента
 */
void awh::client::WebSocket::start() noexcept {
	// Если адрес URL запроса передан
	if(!this->freeze && !this->worker.url.empty()){
		// Если биндинг не запущен, выполняем запуск биндинга
		if(!this->core->working())
			// Выполняем запуск биндинга
			const_cast <client::core_t *> (this->core)->start();
		// Выполняем запрос на сервер
		else const_cast <client::core_t *> (this->core)->open(this->worker.wid);
	}
	// Снимаем с паузы клиент
	this->freeze = false;
}
/**
 * pause Метод установки на паузу клиента
 */
void awh::client::WebSocket::pause() noexcept {
	// Ставим работу клиента на паузу
	this->freeze = true;
}
/**
 * getSub Метод получения выбранного сабпротокола
 * @return выбранный сабпротокол
 */
const string & awh::client::WebSocket::getSub() const noexcept {
	// Выводим выбранный сабпротокол
	return this->http.getSub();
}
/**
 * setSub Метод установки подпротокола поддерживаемого сервером
 * @param sub подпротокол для установки
 */
void awh::client::WebSocket::setSub(const string & sub) noexcept {
	// Устанавливаем подпротокол
	if(!sub.empty()) this->http.setSub(sub);
}
/**
 * setSubs Метод установки списка подпротоколов поддерживаемых сервером
 * @param subs подпротоколы для установки
 */
void awh::client::WebSocket::setSubs(const vector <string> & subs) noexcept {
	// Если список подпротоколов получен
	if(!subs.empty()) this->http.setSubs(subs);
}
/**
 * setBytesDetect Метод детекции сообщений по количеству байт
 * @param read  количество байт для детекции по чтению
 * @param write количество байт для детекции по записи
 */
void awh::client::WebSocket::setBytesDetect(const worker_t::mark_t read, const worker_t::mark_t write) noexcept {
	// Устанавливаем количество байт на чтение
	this->worker.marker.read = read;
	// Устанавливаем количество байт на запись
	this->worker.marker.write = write;
	// Если минимальный размер данных для чтения, не установлен
	if(this->worker.marker.read.min == 0)
		// Устанавливаем размер минимальных для чтения данных по умолчанию
		this->worker.marker.read.min = BUFFER_READ_MIN;
	// Если максимальный размер данных для записи не установлен, устанавливаем по умолчанию
	if(this->worker.marker.write.max == 0)
		// Устанавливаем размер максимальных записываемых данных по умолчанию
		this->worker.marker.write.max = BUFFER_WRITE_MAX;
}
/**
 * setWaitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read    количество секунд для детекции по чтению
 * @param write   количество секунд для детекции по записи
 * @param connect количество секунд для детекции по подключению
 */
void awh::client::WebSocket::setWaitTimeDetect(const time_t read, const time_t write, const time_t connect) noexcept {
	// Устанавливаем количество секунд на чтение
	this->worker.timeouts.read = read;
	// Устанавливаем количество секунд на запись
	this->worker.timeouts.write = write;
	// Устанавливаем количество секунд на подключение
	this->worker.timeouts.connect = connect;
}
/**
 * multiThreads Метод активации многопоточности
 * @param threads количество потоков для активации
 * @param mode    флаг активации/деактивации мультипоточности
 */
void awh::client::WebSocket::multiThreads(const size_t threads, const bool mode) noexcept {
	// Если нужно активировать многопоточность
	if(mode){
		// Если многопоточность ещё не активированна
		if(!this->thr.is()) this->thr.init(threads);
		// Если многопоточность уже активированна
		else {
			// Выполняем завершение всех активных потоков
			this->thr.wait();
			// Выполняем инициализацию нового тредпула
			this->thr.init(threads);
		}
		// Устанавливаем простое чтение базы событий
		const_cast <client::core_t *> (this->core)->easily(true);
	// Выполняем завершение всех потоков
	} else this->thr.wait();
}
/**
 * setMode Метод установки флага модуля
 * @param flag флаг модуля для установки
 */
void awh::client::WebSocket::setMode(const u_short flag) noexcept {
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	this->noinfo = (flag & (uint8_t) flag_t::NOINFO);
	// Устанавливаем флаг анбиндинга ядра сетевого модуля
	this->unbind = !(flag & (uint8_t) flag_t::NOTSTOP);
	// Устанавливаем флаг ожидания входящих сообщений
	this->worker.wait = (flag & (uint8_t) flag_t::WAITMESS);
	// Устанавливаем флаг поддержания автоматического подключения
	this->worker.alive = (flag & (uint8_t) flag_t::KEEPALIVE);
	// Устанавливаем флаг перехвата контекста компрессии для клиента
	this->takeOverCli = (flag & (uint8_t) flag_t::TAKEOVERCLI);
	// Устанавливаем флаг перехвата контекста компрессии для сервера
	this->takeOverSrv = (flag & (uint8_t) flag_t::TAKEOVERSRV);
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	const_cast <client::core_t *> (this->core)->setNoInfo(flag & (uint8_t) flag_t::NOINFO);
	// Выполняем установку флага проверки домена
	const_cast <client::core_t *> (this->core)->setVerifySSL(flag & (uint8_t) flag_t::VERIFYSSL);
}
/**
 * setProxy Метод установки прокси-сервера
 * @param uri параметры прокси-сервера
 */
void awh::client::WebSocket::setProxy(const string & uri) noexcept {
	// Если URI параметры переданы
	if(!uri.empty()){
		// Устанавливаем параметры прокси-сервера
		this->worker.proxy.url = this->uri.parseUrl(uri);
		// Если данные параметров прокси-сервера получены
		if(!this->worker.proxy.url.empty()){
			// Если протокол подключения SOCKS5
			if(this->worker.proxy.url.schema.compare("socks5") == 0){
				// Устанавливаем тип прокси-сервера
				this->worker.proxy.type = proxy_t::type_t::SOCKS5;
				// Если требуется авторизация на прокси-сервере
				if(!this->worker.proxy.url.user.empty() && !this->worker.proxy.url.pass.empty())
					// Устанавливаем данные пользователя
					this->worker.proxy.socks5.setUser(this->worker.proxy.url.user, this->worker.proxy.url.pass);
			// Если протокол подключения HTTP
			} else if((this->worker.proxy.url.schema.compare("http") == 0) || (this->worker.proxy.url.schema.compare("https") == 0)) {
				// Устанавливаем тип прокси-сервера
				this->worker.proxy.type = proxy_t::type_t::HTTP;
				// Если требуется авторизация на прокси-сервере
				if(!this->worker.proxy.url.user.empty() && !this->worker.proxy.url.pass.empty())
					// Устанавливаем данные пользователя
					this->worker.proxy.http.setUser(this->worker.proxy.url.user, this->worker.proxy.url.pass);
			}
		}
	}
}
/**
 * setChunkSize Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::client::WebSocket::setChunkSize(const size_t size) noexcept {
	// Устанавливаем размер чанка
	this->http.setChunkSize(size);
}
/**
 * setFrameSize Метод установки размеров сегментов фрейма
 * @param size минимальный размер сегмента
 */
void awh::client::WebSocket::setFrameSize(const size_t size) noexcept {
	// Если размер передан, устанавливаем
	if(size > 0) this->frameSize = size;
}
/**
 * setAttempts Метод установки общего количества попыток
 * @param attempts общее количество попыток
 */
void awh::client::WebSocket::setAttempts(const uint8_t attempts) noexcept {
	// Если количество попыток передано, устанавливаем его
	if(attempts > 0) this->totalAttempts = attempts;
}
/**
 * setUserAgent Метод установки User-Agent для HTTP запроса
 * @param userAgent агент пользователя для HTTP запроса
 */
void awh::client::WebSocket::setUserAgent(const string & userAgent) noexcept {
	// Устанавливаем UserAgent
	if(!userAgent.empty()){
		// Устанавливаем пользовательского агента
		this->http.setUserAgent(userAgent);
		// Устанавливаем пользовательского агента для прокси-сервера
		this->worker.proxy.http.setUserAgent(userAgent);
	}
}
/**
 * setCompress Метод установки метода компрессии
 * @param compress метод компрессии сообщений
 */
void awh::client::WebSocket::setCompress(const http_t::compress_t compress) noexcept {
	// Устанавливаем метод компрессии
	this->compress = compress;
}
/**
 * setUser Метод установки параметров авторизации
 * @param login    логин пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 */
void awh::client::WebSocket::setUser(const string & login, const string & password) noexcept {
	// Если пользователь и пароль переданы
	if(!login.empty() && !password.empty())
		// Устанавливаем логин и пароль пользователя
		this->http.setUser(login, password);
}
/**
 * setServ Метод установки данных сервиса
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::client::WebSocket::setServ(const string & id, const string & name, const string & ver) noexcept {
	// Устанавливаем данные сервиса
	this->http.setServ(id, name, ver);
	// Устанавливаем данные сервиса для прокси-сервера
	this->worker.proxy.http.setServ(id, name, ver);
}
/**
 * setCrypt Метод установки параметров шифрования
 * @param pass пароль шифрования передаваемых данных
 * @param salt соль шифрования передаваемых данных
 * @param aes  размер шифрования передаваемых данных
 */
void awh::client::WebSocket::setCrypt(const string & pass, const string & salt, const hash_t::aes_t aes) noexcept {
	// Устанавливаем флаг шифрования
	this->crypt = !pass.empty();
	// Устанавливаем размер шифрования
	this->hash.setAES(aes);
	// Устанавливаем соль шифрования
	this->hash.setSalt(salt);
	// Устанавливаем пароль шифрования
	this->hash.setPassword(pass);
	// Устанавливаем параметры шифрования
	if(this->crypt) this->http.setCrypt(pass, salt, aes);
}
/**
 * setAuthType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::client::WebSocket::setAuthType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Если объект авторизации создан
	this->http.setAuthType(type, hash);
}
/**
 * setAuthTypeProxy Метод установки типа авторизации прокси-сервера
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::client::WebSocket::setAuthTypeProxy(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Если объект авторизации создан
	this->worker.proxy.http.setAuthType(type, hash);
}
/**
 * WebSocket Конструктор
 * @param core объект биндинга TCP/IP
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::client::WebSocket::WebSocket(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept : nwk(fmk), hash(fmk, log), uri(fmk, &nwk), http(fmk, log, &uri), frame(fmk, log), worker(fmk, log), action(action_t::NONE), opcode(frame_t::opcode_t::TEXT), compress(http_t::compress_t::NONE), fmk(fmk), log(log), core(core) {
	// Устанавливаем событие на запуск системы
	this->worker.openFn = std::bind(&awh::client::WebSocket::openCallback, this, _1, _2);
	// Устанавливаем функцию персистентного вызова
	this->worker.persistFn = std::bind(&awh::client::WebSocket::persistCallback, this, _1, _2, _3);
	// Устанавливаем событие подключения
	this->worker.connectFn = std::bind(&awh::client::WebSocket::connectCallback, this, _1, _2, _3);
	// Устанавливаем функцию чтения данных
	this->worker.readFn = std::bind(&awh::client::WebSocket::readCallback, this, _1, _2, _3, _4, _5);
	// Устанавливаем функцию записи данных
	this->worker.writeFn = std::bind(&awh::client::WebSocket::writeCallback, this, _1, _2, _3, _4, _5);
	// Устанавливаем событие отключения
	this->worker.disconnectFn = std::bind(&awh::client::WebSocket::disconnectCallback, this, _1, _2, _3);
	// Устанавливаем событие на подключение к прокси-серверу
	this->worker.connectProxyFn = std::bind(&awh::client::WebSocket::proxyConnectCallback, this, _1, _2, _3);
	// Устанавливаем событие на чтение данных с прокси-сервера
	this->worker.readProxyFn = std::bind(&awh::client::WebSocket::proxyReadCallback, this, _1, _2, _3, _4, _5);
	// Активируем персистентный запуск для работы пингов
	const_cast <client::core_t *> (this->core)->setPersist(true);
	// Добавляем воркер в биндер TCP/IP
	const_cast <client::core_t *> (this->core)->add(&this->worker);
}
