/**
 * @file: ws.cpp
 * @date: 2022-09-03
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
#include <client/ws.hpp>

/**
 * openCallback Метод обратного вызова при запуске работы
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::WebSocket::openCallback(const size_t sid, awh::core_t * core) noexcept {
	// Если дисконнекта ещё не произошло
	if(this->_action == action_t::NONE){
		// Устанавливаем экшен выполнения
		this->_action = action_t::OPEN;
		// Выполняем запуск обработчика событий
		this->handler();
	}
}
/**
 * persistCallback Метод персистентного вызова
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::WebSocket::persistCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Получаем текущий штамп времени
		const time_t stamp = this->_fmk->unixTimestamp();
		// Если адъютант не ответил на пинг больше двух интервалов, отключаем его
		if(this->_close || ((stamp - this->_checkPoint) >= (PERSIST_INTERVAL * 5)))
			// Завершаем работу
			reinterpret_cast <client::core_t *> (core)->close(aid);
		// Отправляем запрос адъютанту
		else this->ping(to_string(aid));
	}
}
/**
 * connectCallback Метод обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::WebSocket::connectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Запоминаем идентификатор адъютанта
		this->_aid = aid;
		// Устанавливаем экшен выполнения
		this->_action = action_t::CONNECT;
		// Выполняем запуск обработчика событий
		this->handler();
	}
}
/**
 * disconnectCallback Метод обратного вызова при отключении от сервера
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::WebSocket::disconnectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((sid > 0) && (core != nullptr)){
		// Устанавливаем экшен выполнения
		this->_action = action_t::DISCONNECT;
		// Выполняем запуск обработчика событий
		this->handler();
	}
}
/**
 * readCallback Метод обратного вызова при чтении сообщения с сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::client::WebSocket::readCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (aid > 0) && (sid > 0)){
		// Если дисконнекта ещё не произошло
		if((this->_action == action_t::NONE) || (this->_action == action_t::READ)){
			// Если подключение закрыто
			if(this->_close){
				// Принудительно выполняем отключение лкиента
				reinterpret_cast <client::core_t *> (core)->close(aid);
				// Выходим из функции
				return;
			}
			// Если разрешено получение данных
			if(this->_allow.receive){
				// Устанавливаем экшен выполнения
				this->_action = action_t::READ;
				// Добавляем полученные данные в буфер
				this->_buffer.payload.insert(this->_buffer.payload.end(), buffer, buffer + size);
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
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::client::WebSocket::writeCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Если необходимо выполнить закрыть подключение
		if(!this->_close && this->_stopped){
			// Устанавливаем флаг закрытия подключения
			this->_close = !this->_close;
			// Принудительно выполняем отключение лкиента
			const_cast <client::core_t *> (this->_core)->close(aid);
		}
	}
}
/**
 * enableTLSCallback Метод активации зашифрованного канала TLS
 * @param url  адрес сервера для которого выполняется активация зашифрованного канала TLS
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 * @return     результат активации зашифрованного канала TLS
 */
bool awh::client::WebSocket::enableTLSCallback(const uri_t::url_t & url, const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Блокируем переменные которые не используем
	(void) aid;
	(void) sid;
	(void) core;
	// Выводим результат активации
	return (!url.empty() && (url.schema.compare("wss") == 0));
}
/**
 * proxyConnectCallback Метод обратного вызова при подключении к прокси-серверу
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::WebSocket::proxyConnectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Запоминаем идентификатор адъютанта
		this->_aid = aid;
		// Устанавливаем экшен выполнения
		this->_action = action_t::PROXY_CONNECT;
		// Выполняем запуск обработчика событий
		this->handler();
	}
}
/**
 * proxyReadCallback Метод обратного вызова при чтении сообщения с прокси-сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::client::WebSocket::proxyReadCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (aid > 0) && (sid > 0)){
		// Если дисконнекта ещё не произошло
		if((this->_action == action_t::NONE) || (this->_action == action_t::PROXY_READ)){
			// Устанавливаем экшен выполнения
			this->_action = action_t::PROXY_READ;
			// Добавляем полученные данные в буфер
			this->_buffer.payload.insert(this->_buffer.payload.end(), buffer, buffer + size);
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
	if(!this->_locker.mode){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_locker.mtx);
		// Флаг разрешающий циклический перебор экшенов
		bool loop = true;
		// Выполняем блокировку обработчика
		this->_locker.mode = true;
		// Выполняем обработку всех экшенов
		while(loop && (this->_action != action_t::NONE)){
			// Определяем обрабатываемый экшен
			switch((uint8_t) this->_action){
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
				// Если сработал неизвестный экшен, выходим
				default: loop = false;
			}
		}
		// Выполняем разблокировку обработчика
		this->_locker.mode = false;
	}
}
/**
 * actionOpen Метод обработки экшена открытия подключения
 */
void awh::client::WebSocket::actionOpen() noexcept {
	// Выполняем подключение
	const_cast <client::core_t *> (this->_core)->open(this->_scheme.sid);
	// Если экшен соответствует, выполняем его сброс
	if(this->_action == action_t::OPEN)
		// Выполняем сброс экшена
		this->_action = action_t::NONE;
}
/**
 * actionRead Метод обработки экшена чтения с сервера
 */
void awh::client::WebSocket::actionRead() noexcept {
	// Объект сообщения
	mess_t mess;
	// Получаем объект биндинга ядра TCP/IP
	client::core_t * core = const_cast <client::core_t *> (this->_core);
	// Если рукопожатие не выполнено
	if(!reinterpret_cast <http_t *> (&this->_http)->isHandshake()){
		// Выполняем парсинг полученных данных
		const size_t bytes = this->_http.parse(this->_buffer.payload.data(), this->_buffer.payload.size());
		// Если все данные получены
		if(this->_http.isEnd()){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Получаем данные ответа
				const auto & response = reinterpret_cast <http_t *> (&this->_http)->response(true);
				// Если параметры ответа получены
				if(!response.empty()){
					// Выводим заголовок ответа
					cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
					// Выводим параметры ответа
					cout << string(response.begin(), response.end()) << endl;
					// Если тело ответа существует
					if(!this->_http.body().empty())
						// Выводим сообщение о выводе чанка тела
						cout << this->_fmk->format("<body %u>", this->_http.body().size()) << endl << endl;
					// Иначе устанавливаем перенос строки
					else cout << endl;
				}
			#endif
			// Выполняем проверку авторизации
			switch((uint8_t) this->_http.getAuth()){
				// Если нужно попытаться ещё раз
				case (uint8_t) http_t::stath_t::RETRY: {
					// Если попытка повторить авторизацию ещё не проводилась
					if(this->_attempt < this->_attempts){
						// Получаем новый адрес запроса
						this->_scheme.url = this->_http.getUrl();
						// Если адрес запроса получен
						if(!this->_scheme.url.empty()){
							// Увеличиваем количество попыток
							this->_attempt++;
							// Выполняем очистку оставшихся данных
							this->_buffer.payload.clear();
							// Если соединение является постоянным
							if(this->_http.isAlive())
								// Устанавливаем новый экшен выполнения
								this->_action = action_t::CONNECT;
							// Завершаем работу
							else {
								// Если экшен соответствует, выполняем его сброс
								if(this->_action == action_t::READ)
									// Выполняем сброс экшена
									this->_action = action_t::NONE;
								// Завершаем работу
								core->close(this->_aid);
							}
							// Завершаем работу
							return;
						}
					}
					// Устанавливаем флаг принудительной остановки
					this->_stopped = true;
					// Создаём сообщение
					mess = mess_t(this->_code, this->_http.message(this->_code));
					// Выводим сообщение
					this->error(mess);
				} break;
				// Если запрос выполнен удачно
				case (uint8_t) http_t::stath_t::GOOD: {
					// Если рукопожатие выполнено
					if(this->_http.isHandshake()){
						// Выполняем сброс количества попыток
						this->_attempt = 0;
						// Очищаем список фрагментированных сообщений
						this->_buffer.fragmes.clear();
						// Получаем флаг шифрованных данных
						this->_crypt = this->_http.isCrypt();
						// Получаем поддерживаемый метод компрессии
						this->_compress = this->_http.compress();
						// Получаем размер скользящего окна сервера
						this->_wbitServer = this->_http.wbitServer();
						// Получаем размер скользящего окна клиента
						this->_wbitClient = this->_http.wbitClient();
						// Обновляем контрольную точку времени получения данных
						this->_checkPoint = this->_fmk->unixTimestamp();
						// Разрешаем перехватывать контекст компрессии для клиента
						this->_hash.takeoverCompress(this->_http.clientTakeover());
						// Разрешаем перехватывать контекст компрессии для сервера
						this->_hash.takeoverDecompress(this->_http.serverTakeover());
						// Выводим в лог сообщение
						if(!this->_noinfo) this->_log->print("authorization on the WebSocket server was successful", log_t::flag_t::INFO);
						// Если функция обратного вызова установлена, выполняем
						if(this->_callback.active != nullptr) this->_callback.active(mode_t::CONNECT, this);
						// Есла данных передано больше чем обработано
						if(this->_buffer.payload.size() > bytes)
							// Удаляем количество обработанных байт
							this->_buffer.payload.assign(this->_buffer.payload.begin() + bytes, this->_buffer.payload.end());
							// vector <decltype(this->_buffer.payload)::value_type> (this->_buffer.payload.begin() + bytes, this->_buffer.payload.end()).swap(this->_buffer.payload);
						// Если данных в буфере больше нет
						else {
							// Очищаем буфер собранных данных
							this->_buffer.payload.clear();
							// Если экшен соответствует, выполняем его сброс
							if(this->_action == action_t::READ)
								// Выполняем сброс экшена
								this->_action = action_t::NONE;
						}
						// Завершаем работу
						return;
					// Сообщаем, что рукопожатие не выполнено
					} else {
						// Устанавливаем код ответа
						this->_code = 404;
						// Создаём сообщение
						mess = mess_t(this->_code, this->_http.message(this->_code));
						// Выводим сообщение
						this->error(mess);
					}
				} break;
				// Если запрос неудачный
				case (uint8_t) http_t::stath_t::FAULT: {
					// Получаем параметры запроса
					const auto & query = this->_http.query();
					// Устанавливаем флаг принудительной остановки
					this->_stopped = true;
					// Устанавливаем код ответа
					this->_code = query.code;
					// Создаём сообщение
					mess = mess_t(this->_code, query.message);
					// Выводим сообщение
					this->error(mess);
				} break;
			}
			// Если экшен соответствует, выполняем его сброс
			if(this->_action == action_t::READ)
				// Выполняем сброс экшена
				this->_action = action_t::NONE;
			// Выполняем сброс количества попыток
			this->_attempt = 0;
			// Завершаем работу
			core->close(this->_aid);
		// Если экшен соответствует, выполняем его сброс
		} else if(this->_action == action_t::READ)
			// Выполняем сброс экшена
			this->_action = action_t::NONE;
		// Завершаем работу
		return;
	// Если рукопожатие выполнено
	} else if(this->_allow.receive) {
		// Флаг удачного получения данных
		bool receive = false;
		// Создаём буфер сообщения
		vector <char> buffer;
		// Создаём объект шапки фрейма
		frame_t::head_t head;
		// Выполняем обработку полученных данных
		while(!this->_close && this->_allow.receive){
			// Выполняем чтение фрейма WebSocket
			const auto & data = this->_frame.get(head, this->_buffer.payload.data(), this->_buffer.payload.size());
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
				if(head.rsv[0] && ((this->_compress == http_t::compress_t::NONE) ||
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
						if(memcmp(to_string(this->_aid).c_str(), data.data(), data.size()) == 0)
							// Обновляем контрольную точку
							this->_checkPoint = this->_fmk->unixTimestamp();
					break;
					// Если ответом является TEXT
					case (uint8_t) frame_t::opcode_t::TEXT:
					// Если ответом является BINARY
					case (uint8_t) frame_t::opcode_t::BINARY: {
						// Запоминаем полученный опкод
						this->_opcode = head.optcode;
						// Запоминаем, что данные пришли сжатыми
						this->_compressed = (head.rsv[0] && (this->_compress != http_t::compress_t::NONE));
						// Если сообщение замаскированно
						if(head.mask){
							// Создаём сообщение
							mess = mess_t(1002, "masked frame from server");
							// Выводим сообщение
							this->error(mess);
							// Выполняем реконнект
							goto Reconnect;
						// Если список фрагментированных сообщений существует
						} else if(!this->_buffer.fragmes.empty()) {
							// Очищаем список фрагментированных сообщений
							this->_buffer.fragmes.clear();
							// Создаём сообщение
							mess = mess_t(1002, "opcode for subsequent fragmented messages should not be set");
							// Выводим сообщение
							this->error(mess);
							// Выполняем реконнект
							goto Reconnect;
						// Если сообщение является не последнем
						} else if(!head.fin)
							// Заполняем фрагментированное сообщение
							this->_buffer.fragmes.insert(this->_buffer.fragmes.end(), data.begin(), data.end());
						// Если сообщение является последним
						else buffer = std::forward <const vector <char>> (data);
					} break;
					// Если ответом является CONTINUATION
					case (uint8_t) frame_t::opcode_t::CONTINUATION: {
						// Заполняем фрагментированное сообщение
						this->_buffer.fragmes.insert(this->_buffer.fragmes.end(), data.begin(), data.end());
						// Если сообщение является последним
						if(head.fin){
							// Выполняем копирование всех собранных сегментов
							buffer = std::forward <const vector <char>> (this->_buffer.fragmes);
							// Очищаем список фрагментированных сообщений
							this->_buffer.fragmes.clear();
						}
					} break;
					// Если ответом является CLOSE
					case (uint8_t) frame_t::opcode_t::CLOSE: {
						// Извлекаем сообщение
						mess = this->_frame.message(data);
						// Выводим сообщение
						this->error(mess);
						// Выполняем реконнект
						goto Reconnect;
					} break;
				}
			}
			// Если парсер обработал какое-то количество байт
			if((receive = ((head.frame > 0) && !this->_buffer.payload.empty()))){
				// Если размер буфера больше количества удаляемых байт
				if((receive = (this->_buffer.payload.size() >= head.frame)))
					// Удаляем количество обработанных байт
					this->_buffer.payload.assign(this->_buffer.payload.begin() + head.frame, this->_buffer.payload.end());
					// vector <decltype(this->_buffer.payload)::value_type> (this->_buffer.payload.begin() + head.frame, this->_buffer.payload.end()).swap(this->_buffer.payload);
			}
			// Если сообщения получены
			if(!buffer.empty()){
				// Если тредпул активирован
				if(this->_thr.is())
					// Добавляем в тредпул новую задачу на извлечение полученных сообщений
					this->_thr.push(std::bind(&ws_t::extraction, this, buffer, (this->_opcode == frame_t::opcode_t::TEXT)));
				// Если тредпул не активирован, выполняем извлечение полученных сообщений
				else this->extraction(buffer, (this->_opcode == frame_t::opcode_t::TEXT));
				// Очищаем буфер полученного сообщения
				buffer.clear();
			}
			// Если данные мы все получили, выходим
			if(!receive || this->_buffer.payload.empty()) break;
		}
		// Если экшен соответствует, выполняем его сброс
		if(this->_action == action_t::READ)
			// Выполняем сброс экшена
			this->_action = action_t::NONE;
		// Выходим из функции
		return;
	}
	// Устанавливаем метку реконнекта
	Reconnect:
	// Выполняем отправку сообщения об ошибке
	this->sendError(mess);
	// Если экшен соответствует, выполняем его сброс
	if(this->_action == action_t::READ)
		// Выполняем сброс экшена
		this->_action = action_t::NONE;
}
/**
 * actionConnect Метод обработки экшена подключения к серверу
 */
void awh::client::WebSocket::actionConnect() noexcept {
	// Выполняем сброс параметров запроса
	this->flush();
	// Выполняем сброс состояния HTTP парсера
	this->_http.reset();
	// Выполняем очистку параметров HTTP запроса
	this->_http.clear();
	// Устанавливаем метод сжатия
	this->_http.compress(this->_compress);
	// Разрешаем перехватывать контекст для клиента
	this->_http.clientTakeover(this->_takeOverCli);
	// Разрешаем перехватывать контекст для сервера
	this->_http.serverTakeover(this->_takeOverSrv);
	// Разрешаем перехватывать контекст компрессии
	this->_hash.takeoverCompress(this->_takeOverCli);
	// Разрешаем перехватывать контекст декомпрессии
	this->_hash.takeoverDecompress(this->_takeOverSrv);
	// Получаем бинарные данные REST запроса
	const auto & buffer = this->_http.request(this->_scheme.url);
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
		const_cast <client::core_t *> (this->_core)->write(buffer.data(), buffer.size(), this->_aid);
	}
	// Если экшен соответствует, выполняем его сброс
	if(this->_action == action_t::CONNECT)
		// Выполняем сброс экшена
		this->_action = action_t::NONE;
}
/**
 * actionDisconnect Метод обработки экшена отключения от сервера
 */
void awh::client::WebSocket::actionDisconnect() noexcept {
	// Если подключение является постоянным
	if(this->_scheme.alive){
		// Если экшен соответствует, выполняем его сброс
		if(this->_action == action_t::DISCONNECT)
			// Выполняем сброс экшена
			this->_action = action_t::NONE;
		// Если функция обратного вызова установлена
		if(this->_callback.active != nullptr)
			// Выполняем функцию обратного вызова
			this->_callback.active(mode_t::DISCONNECT, this);
	// Если подключение не является постоянным
	} else {
		// Если нужно произвести запрос заново
		if(!this->_stopped && ((this->_code == 301) || (this->_code == 308) || (this->_code == 401) || (this->_code == 407))){
			// Если статус ответа требует произвести авторизацию или заголовок перенаправления указан
			if((this->_code == 401) || (this->_code == 407) || this->_http.isHeader("location")){
				// Выполняем установку следующего экшена на открытие подключения
				this->_action = action_t::OPEN;
				// Выходим из функции
				return;
			}
		}
		// Выполняем сброс параметров запроса
		this->flush();
		// Очищаем код ответа
		this->_code = 0;
		// Завершаем работу
		if(this->_unbind) const_cast <client::core_t *> (this->_core)->stop();
		// Если экшен соответствует, выполняем его сброс
		if(this->_action == action_t::DISCONNECT)
			// Выполняем сброс экшена
			this->_action = action_t::NONE;
		// Если функция обратного вызова установлена, выполняем
		if(this->_callback.active != nullptr)
			// Выполняем функцию обратного вызова
			this->_callback.active(mode_t::DISCONNECT, this);
	}
}
/**
 * actionProxyRead Метод обработки экшена чтения с прокси-сервера
 */
void awh::client::WebSocket::actionProxyRead() noexcept {
	// Получаем объект биндинга ядра TCP/IP
	client::core_t * core = const_cast <client::core_t *> (this->_core);
	// Определяем тип прокси-сервера
	switch((uint8_t) this->_scheme.proxy.type){
		// Если прокси-сервер является Socks5
		case (uint8_t) proxy_t::type_t::SOCKS5: {
			// Если данные не получены
			if(!this->_scheme.proxy.socks5.isEnd()){
				// Выполняем парсинг входящих данных
				this->_scheme.proxy.socks5.parse(this->_buffer.payload.data(), this->_buffer.payload.size());
				// Получаем данные запроса
				const auto & buffer = this->_scheme.proxy.socks5.get();
				// Если данные получены
				if(!buffer.empty()){
					// Выполняем очистку буфера данных
					this->_buffer.payload.clear();
					// Выполняем отправку сообщения на сервер
					core->write(buffer.data(), buffer.size(), this->_aid);
					// Если экшен соответствует, выполняем его сброс
					if(this->_action == action_t::PROXY_READ)
						// Выполняем сброс экшена
						this->_action = action_t::NONE;
					// Завершаем работу
					return;
				// Если данные все получены
				} else if(this->_scheme.proxy.socks5.isEnd()) {
					// Выполняем очистку буфера данных
					this->_buffer.payload.clear();
					// Если рукопожатие выполнено
					if(this->_scheme.proxy.socks5.isHandshake()){
						// Выполняем переключение на работу с сервером
						core->switchProxy(this->_aid);
						// Если экшен соответствует, выполняем его сброс
						if(this->_action == action_t::PROXY_READ)
							// Выполняем сброс экшена
							this->_action = action_t::NONE;
						// Завершаем работу
						return;
					// Если рукопожатие не выполнено
					} else {
						// Устанавливаем код ответа
						this->_code = this->_scheme.proxy.socks5.code();
						// Создаём сообщение
						mess_t mess(this->_code);
						// Устанавливаем сообщение ответа
						mess = this->_scheme.proxy.socks5.message(this->_code);
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Если заголовки получены
							if(!mess.text.empty()){
								// Данные REST ответа
								const string & response = this->_fmk->format("SOCKS5 %u %s\r\n", this->_code, mess.text.c_str());
								// Выводим заголовок ответа
								cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE PROXY ^^^^^^^^^\x1B[0m" << endl;
								// Выводим параметры ответа
								cout << string(response.begin(), response.end()) << endl;
							}
						#endif
						// Выводим сообщение
						this->error(mess);
						// Если экшен соответствует, выполняем его сброс
						if(this->_action == action_t::PROXY_READ)
							// Выполняем сброс экшена
							this->_action = action_t::NONE;
						// Завершаем работу
						core->close(this->_aid);
						// Завершаем работу
						return;
					}
				}
			}
		} break;
		// Если прокси-сервер является HTTP
		case (uint8_t) proxy_t::type_t::HTTP: {
			// Выполняем парсинг полученных данных
			this->_scheme.proxy.http.parse(this->_buffer.payload.data(), this->_buffer.payload.size());
			// Если все данные получены
			if(this->_scheme.proxy.http.isEnd()){
				// Получаем параметры запроса
				const auto & query = this->_scheme.proxy.http.query();
				// Устанавливаем код ответа
				this->_code = query.code;
				// Создаём сообщение
				mess_t mess(this->_code, query.message);
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Получаем данные ответа
					const auto & response = this->_scheme.proxy.http.response(true);
					// Если параметры ответа получены
					if(!response.empty()){
						// Выводим заголовок ответа
						cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE PROXY ^^^^^^^^^\x1B[0m" << endl;
						// Выводим параметры ответа
						cout << string(response.begin(), response.end()) << endl;
						// Если тело ответа существует
						if(!this->_scheme.proxy.http.body().empty())
							// Выводим сообщение о выводе чанка тела
							cout << this->_fmk->format("<body %u>", this->_scheme.proxy.http.body().size()) << endl;
					}
				#endif
				// Выполняем проверку авторизации
				switch((uint8_t) this->_scheme.proxy.http.getAuth()){
					// Если нужно попытаться ещё раз
					case (uint8_t) http_t::stath_t::RETRY: {
						// Если попытка повторить авторизацию ещё не проводилась
						if(this->_attempt < this->_attempts){
							// Если адрес запроса получен
							if(!this->_scheme.proxy.url.empty()){
								// Увеличиваем количество попыток
								this->_attempt++;
								// Если соединение является постоянным
								if(this->_scheme.proxy.http.isAlive())
									// Устанавливаем новый экшен выполнения
									this->_action = action_t::PROXY_CONNECT;
								// Если соединение должно быть закрыто
								else {
									// Если экшен соответствует, выполняем его сброс
									if(this->_action == action_t::PROXY_READ)
										// Выполняем сброс экшена
										this->_action = action_t::NONE;
									// Завершаем работу
									core->close(this->_aid);
								}
								// Завершаем работу
								return;
							}
						}
						// Устанавливаем флаг принудительной остановки
						this->_stopped = true;
					} break;
					// Если запрос выполнен удачно
					case (uint8_t) http_t::stath_t::GOOD: {
						// Выполняем сброс количества попыток
						this->_attempt = 0;
						// Выполняем переключение на работу с сервером
						core->switchProxy(this->_aid);
						// Если экшен соответствует, выполняем его сброс
						if(this->_action == action_t::PROXY_READ)
							// Выполняем сброс экшена
							this->_action = action_t::NONE;
						// Завершаем работу
						return;
					} break;
					// Если запрос неудачный
					case (uint8_t) http_t::stath_t::FAULT:
						// Устанавливаем флаг принудительной остановки
						this->_stopped = true;
					break;
				}
				// Выводим сообщение
				this->error(mess);
				// Если экшен соответствует, выполняем его сброс
				if(this->_action == action_t::PROXY_READ)
					// Выполняем сброс экшена
					this->_action = action_t::NONE;
				// Выполняем сброс количества попыток
				this->_attempt = 0;
				// Завершаем работу
				core->close(this->_aid);
				// Завершаем работу
				return;
			}
		} break;
		// Иначе завершаем работу
		default: {
			// Если экшен соответствует, выполняем его сброс
			if(this->_action == action_t::PROXY_READ)
				// Выполняем сброс экшена
				this->_action = action_t::NONE;
			// Завершаем работу
			core->close(this->_aid);
		}
	}
}
/**
 * actionProxyConnect Метод обработки экшена подключения к прокси-серверу
 */
void awh::client::WebSocket::actionProxyConnect() noexcept {
	// Получаем объект биндинга ядра TCP/IP
	client::core_t * core = const_cast <client::core_t *> (this->_core);
	// Определяем тип прокси-сервера
	switch((uint8_t) this->_scheme.proxy.type){
		// Если прокси-сервер является Socks5
		case (uint8_t) proxy_t::type_t::SOCKS5: {
			// Выполняем сброс состояния Socks5 парсера
			this->_scheme.proxy.socks5.reset();
			// Устанавливаем URL адрес запроса
			this->_scheme.proxy.socks5.url(this->_scheme.url);
			// Выполняем создание буфера запроса
			this->_scheme.proxy.socks5.parse();
			// Получаем данные запроса
			const auto & buffer = this->_scheme.proxy.socks5.get();
			// Если данные получены
			if(!buffer.empty())
				// Выполняем отправку сообщения на сервер
				core->write(buffer.data(), buffer.size(), this->_aid);
		} break;
		// Если прокси-сервер является HTTP
		case (uint8_t) proxy_t::type_t::HTTP: {
			// Выполняем сброс состояния HTTP парсера
			this->_scheme.proxy.http.reset();
			// Выполняем очистку параметров HTTP запроса
			this->_scheme.proxy.http.clear();
			// Получаем бинарные данные REST запроса
			const auto & buffer = this->_scheme.proxy.http.proxy(this->_scheme.url);
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
				core->write(buffer.data(), buffer.size(), this->_aid);
			}
		} break;
		// Иначе завершаем работу
		default: core->close(this->_aid);
	}
	// Если экшен соответствует, выполняем его сброс
	if(this->_action == action_t::PROXY_CONNECT)
		// Выполняем сброс экшена
		this->_action = action_t::NONE;
}
/**
 * error Метод вывода сообщений об ошибках работы клиента
 * @param message сообщение с описанием ошибки
 */
void awh::client::WebSocket::error(const mess_t & message) const noexcept {
	// Очищаем список буффер бинарных данных
	const_cast <ws_t *> (this)->_buffer.payload.clear();
	// Очищаем список фрагментированных сообщений
	const_cast <ws_t *> (this)->_buffer.fragmes.clear();
	// Если код ошибки указан
	if(message.code > 0){
		// Если сообщение об ошибке пришло
		if(!message.text.empty()){
			// Если тип сообщения получен
			if(!message.type.empty())
				// Выводим в лог сообщение
				this->_log->print("%s - %s [%u]", log_t::flag_t::WARNING, message.type.c_str(), message.text.c_str(), message.code);
			// Иначе выводим сообщение в упрощёном виде
			else this->_log->print("%s [%u]", log_t::flag_t::WARNING, message.text.c_str(), message.code);
			// Если функция обратного вызова установлена, выводим полученное сообщение
			if(this->_callback.error != nullptr) this->_callback.error(message.code, message.text, const_cast <WebSocket *> (this));
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
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Если буфер данных передан
	if(!buffer.empty() && !this->_freeze && (this->_callback.message != nullptr)){
		// Если данные пришли в сжатом виде
		if(this->_compressed && (this->_compress != http_t::compress_t::NONE)){
			// Декомпрессионные данные
			vector <char> data;
			// Определяем метод компрессии
			switch((uint8_t) this->_compress){
				// Если метод компрессии выбран Deflate
				case (uint8_t) http_t::compress_t::DEFLATE: {
					// Устанавливаем размер скользящего окна
					this->_hash.wbit(this->_wbitServer);
					// Добавляем хвост в полученные данные
					this->_hash.setTail(* const_cast <vector <char> *> (&buffer));
					// Выполняем декомпрессию полученных данных
					data = this->_hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::DEFLATE);
				} break;
				// Если метод компрессии выбран GZip
				case (uint8_t) http_t::compress_t::GZIP:
					// Выполняем декомпрессию полученных данных
					data = this->_hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::GZIP);
				break;
				// Если метод компрессии выбран Brotli
				case (uint8_t) http_t::compress_t::BROTLI:
					// Выполняем декомпрессию полученных данных
					data = this->_hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::BROTLI);
				break;
			}
			// Если данные получены
			if(!data.empty()){
				// Если нужно производить дешифрование
				if(this->_crypt){
					// Выполняем шифрование переданных данных
					const auto & res = this->_hash.decrypt(data.data(), data.size());
					// Отправляем полученный результат
					if(!res.empty()) this->_callback.message(res, utf8, const_cast <WebSocket *> (this));
					// Иначе выводим сообщение так - как оно пришло
					else this->_callback.message(data, utf8, const_cast <WebSocket *> (this));
				// Отправляем полученный результат
				} else this->_callback.message(data, utf8, const_cast <WebSocket *> (this));
			// Выводим сообщение об ошибке
			} else {
				// Создаём сообщение
				mess_t mess(1007, "received data decompression error");
				// Выводим сообщение
				this->error(mess);
				// Иначе выводим сообщение так - как оно пришло
				this->_callback.message(buffer, utf8, const_cast <WebSocket *> (this));
				// Выполняем отправку сообщения об ошибке
				this->sendError(mess);
			}
		// Если функция обратного вызова установлена, выводим полученное сообщение
		} else {
			// Если нужно производить дешифрование
			if(this->_crypt){
				// Выполняем шифрование переданных данных
				const auto & res = this->_hash.decrypt(buffer.data(), buffer.size());
				// Отправляем полученный результат
				if(!res.empty()) this->_callback.message(res, utf8, const_cast <WebSocket *> (this));
				// Иначе выводим сообщение так - как оно пришло
				else this->_callback.message(buffer, utf8, const_cast <WebSocket *> (this));
			// Отправляем полученный результат
			} else this->_callback.message(buffer, utf8, const_cast <WebSocket *> (this));
		}
	}
}
/**
 * flush Метод сброса параметров запроса
 */
void awh::client::WebSocket::flush() noexcept {
	// Снимаем флаг отключения
	this->_close = false;
	// Снимаем флаг принудительной остановки
	this->_stopped = false;
	// Устанавливаем флаг разрешающий обмен данных
	this->_allow = allow_t();
	// Очищаем буфер данных
	this->_buffer = buffer_t();
}
/**
 * pong Метод ответа на проверку о доступности сервера
 * @param message сообщение для отправки
 */
void awh::client::WebSocket::pong(const string & message) noexcept {
	// Если подключение выполнено
	if(this->_core->working() && this->_allow.send){
		// Если рукопожатие выполнено
		if(this->_http.isHandshake() && (this->_aid > 0)){
			// Создаём буфер для отправки
			const auto & buffer = this->_frame.pong(message, true);
			// Если бинарный буфер получен
			if(!buffer.empty())
				// Выполняем отправку сообщения на сервер
				const_cast <client::core_t *> (this->_core)->write(buffer.data(), buffer.size(), this->_aid);
		}
	}
}
/**
 * ping Метод проверки доступности сервера
 * @param message сообщение для отправки
 */
void awh::client::WebSocket::ping(const string & message) noexcept {
	// Если подключение выполнено
	if(this->_core->working() && this->_allow.send){
		// Если рукопожатие выполнено
		if(this->_http.isHandshake() && (this->_aid > 0)){
			// Создаём буфер для отправки
			const auto & buffer = this->_frame.ping(message, true);
			// Если бинарный буфер получен
			if(!buffer.empty())
				// Выполняем отправку сообщения на сервер
				const_cast <client::core_t *> (this->_core)->write(buffer.data(), buffer.size(), this->_aid);
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
	if(this->_core->family() == scheme_t::family_t::NIX){
		// Выполняем очистку схемы сети
		this->_scheme.clear();
		// Устанавливаем метод компрессии сообщений
		this->_compress = compress;
		// Устанавливаем URL адрес запроса (как заглушка)
		this->_scheme.url = this->_uri.parse("ws://unixsocket");
		/**
		 * Если операционной системой не является Windows
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Выполняем установку unix-сокета 
			const_cast <client::core_t *> (this->_core)->unixSocket(url);
		#endif
	// Выполняем установку unix-сокет
	} else {
		// Если адрес сервера передан
		if(!url.empty()){
			// Выполняем очистку схемы сети
			this->_scheme.clear();
			// Устанавливаем URL адрес запроса
			this->_scheme.url = this->_uri.parse(url);
			/**
			 * Если операционной системой не является Windows
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
				// Удаляем unix-сокет ранее установленный
				const_cast <client::core_t *> (this->_core)->removeUnixSocket();
			#endif
		}
	}
	// Устанавливаем метод компрессии сообщений
	this->_compress = compress;
}
/**
 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket::on(function <void (const mode_t, WebSocket *)> callback) noexcept {
	// Устанавливаем функцию запуска и остановки
	this->_callback.active = callback;
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибок
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket::on(function <void (const u_int, const string &, WebSocket *)> callback) noexcept {
	// Устанавливаем функцию получения ошибок
	this->_callback.error = callback;
}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket::on(function <void (const vector <char> &, const bool, WebSocket *)> callback) noexcept {
	// Устанавливаем функцию получения сообщений с сервера
	this->_callback.message = callback;
}
/**
 * sendTimeout Метод отправки сигнала таймаута
 */
void awh::client::WebSocket::sendTimeout() noexcept {
	// Если подключение выполнено
	if(this->_core->working() && this->_allow.send)
		// Отправляем сигнал принудительного таймаута
		const_cast <client::core_t *> (this->_core)->sendTimeout(this->_aid);
}
/**
 * sendError Метод отправки сообщения об ошибке
 * @param mess отправляемое сообщение об ошибке
 */
void awh::client::WebSocket::sendError(const mess_t & mess) noexcept {
	// Если подключение выполнено
	if(this->_core->working() && this->_allow.send && (this->_aid > 0)){
		// Запрещаем получение данных
		this->_allow.receive = false;
		// Получаем объект биндинга ядра TCP/IP
		client::core_t * core = const_cast <client::core_t *> (this->_core);
		// Выполняем остановку получения данных
		core->disabled(engine_t::method_t::READ, this->_aid);
		// Если код ошибки относится к WebSocket
		if(mess.code >= 1000){
			// Получаем буфер сообщения
			const auto & buffer = this->_frame.message(mess);
			// Если данные сообщения получены
			if(!buffer.empty()){
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Выводим заголовок ответа
					cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
					// Выводим отправляемое сообщение
					cout << this->_fmk->format("%s [%u]", mess.text.c_str(), mess.code) << endl << endl;
				#endif
				// Устанавливаем флаг принудительной остановки
				this->_stopped = true;
				// Выполняем отправку сообщения на сервер
				core->write(buffer.data(), buffer.size(), this->_aid);
				// Выходим из функции
				return;
			}
		}
		// Завершаем работу
		const_cast <client::core_t *> (this->_core)->close(this->_aid);
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
	if(this->_core->working() && this->_allow.send){
		// Выполняем блокировку отправки сообщения
		this->_allow.send = !this->_allow.send;
		// Если рукопожатие выполнено
		if((message != nullptr) && (size > 0) && this->_http.isHandshake() && (this->_aid > 0)){
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
				else cout << this->_fmk->format("<bytes %u>", size) << endl << endl;
			#endif
			// Буфер сжатых данных
			vector <char> buffer;
			// Создаём объект заголовка для отправки
			frame_t::head_t head(true, true);
			// Если нужно производить шифрование
			if(this->_crypt){
				// Выполняем шифрование переданных данных
				buffer = this->_hash.encrypt(message, size);
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
			head.rsv[0] = ((size >= 1024) && (this->_compress != http_t::compress_t::NONE));
			// Если необходимо сжимать сообщение перед отправкой
			if(head.rsv[0]){
				// Компрессионные данные
				vector <char> data;
				// Определяем метод компрессии
				switch((uint8_t) this->_compress){
					// Если метод компрессии выбран Deflate
					case (uint8_t) http_t::compress_t::DEFLATE: {
						// Устанавливаем размер скользящего окна
						this->_hash.wbit(this->_wbitClient);
						// Выполняем компрессию полученных данных
						data = this->_hash.compress(message, size, hash_t::method_t::DEFLATE);
						// Удаляем хвост в полученных данных
						this->_hash.rmTail(data);
					} break;
					// Если метод компрессии выбран GZip
					case (uint8_t) http_t::compress_t::GZIP:
						// Выполняем компрессию полученных данных
						data = this->_hash.compress(message, size, hash_t::method_t::GZIP);
					break;
					// Если метод компрессии выбран Brotli
					case (uint8_t) http_t::compress_t::BROTLI:
						// Выполняем компрессию полученных данных
						data = this->_hash.compress(message, size, hash_t::method_t::BROTLI);
					break;
				}
				// Если сжатие данных прошло удачно
				if(!data.empty()){
					// Выполняем перемещение данных
					buffer = std::forward <vector <char>> (data);
					// Заменяем сообщение для передачи
					message = buffer.data();
					// Заменяем размер сообщения
					(* const_cast <size_t *> (&size)) = buffer.size();
				// Снимаем флаг сжатых данных
				} else head.rsv[0] = false;
			}
			// Если требуется фрагментация сообщения
			if(size > this->_frameSize){
				// Бинарный буфер чанка данных
				vector <char> chunk(this->_frameSize);
				// Смещение в бинарном буфере
				size_t start = 0, stop = this->_frameSize;
				// Выполняем разбивку полезной нагрузки на сегменты
				while(stop < size){
					// Увеличиваем длину чанка
					stop += this->_frameSize;
					// Если длина чанка слишком большая, компенсируем
					stop = (stop > size ? size : stop);
					// Устанавливаем флаг финального сообщения
					head.fin = (stop == size);
					// Формируем чанк бинарных данных
					chunk.assign(message + start, message + stop);
					// Создаём буфер для отправки
					const auto & buffer = this->_frame.set(head, chunk.data(), chunk.size());
					// Если бинарный буфер для отправки данных получен
					if(!buffer.empty())
						// Отправляем серверу сообщение
						const_cast <client::core_t *> (this->_core)->write(buffer.data(), buffer.size(), this->_aid);
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
				const auto & buffer = this->_frame.set(head, message, size);
				// Если бинарный буфер для отправки данных получен
				if(!buffer.empty())
					// Отправляем серверу сообщение
					const_cast <client::core_t *> (this->_core)->write(buffer.data(), buffer.size(), this->_aid);
			}
		}
		// Выполняем разблокировку отправки сообщения
		this->_allow.send = !this->_allow.send;
	}
}
/**
 * stop Метод остановки клиента
 */
void awh::client::WebSocket::stop() noexcept {
	// Очищаем код ответа
	this->_code = 0;
	// Если подключение выполнено
	if(this->_core->working()){
		// Завершаем работу, если разрешено остановить
		if(this->_unbind) const_cast <client::core_t *> (this->_core)->stop();
		// Если завершать работу запрещено, просто отключаемся
		else {
			/**
			 * Если установлено постоянное подключение
			 * нам нужно заблокировать автоматический реконнект.
			 */
			// Считываем значение флага
			const bool alive = this->_scheme.alive;
			// Выполняем отключение флага постоянного подключения
			this->_scheme.alive = false;
			// Выполняем отключение клиента
			const_cast <client::core_t *> (this->_core)->close(this->_aid);
			// Восстанавливаем предыдущее значение флага
			this->_scheme.alive = alive;
		}
	}
}
/**
 * start Метод запуска клиента
 */
void awh::client::WebSocket::start() noexcept {
	// Если адрес URL запроса передан
	if(!this->_freeze && !this->_scheme.url.empty()){
		// Если биндинг не запущен, выполняем запуск биндинга
		if(!this->_core->working())
			// Выполняем запуск биндинга
			const_cast <client::core_t *> (this->_core)->start();
		// Выполняем запрос на сервер
		else const_cast <client::core_t *> (this->_core)->open(this->_scheme.sid);
	}
	// Снимаем с паузы клиент
	this->_freeze = false;
}
/**
 * pause Метод установки на паузу клиента
 */
void awh::client::WebSocket::pause() noexcept {
	// Ставим работу клиента на паузу
	this->_freeze = true;
}
/**
 * sub Метод получения выбранного сабпротокола
 * @return выбранный сабпротокол
 */
const string & awh::client::WebSocket::sub() const noexcept {
	// Выводим выбранный сабпротокол
	return this->_http.sub();
}
/**
 * sub Метод установки подпротокола поддерживаемого сервером
 * @param sub подпротокол для установки
 */
void awh::client::WebSocket::sub(const string & sub) noexcept {
	// Устанавливаем подпротокол
	if(!sub.empty()) this->_http.sub(sub);
}
/**
 * subs Метод установки списка подпротоколов поддерживаемых сервером
 * @param subs подпротоколы для установки
 */
void awh::client::WebSocket::subs(const vector <string> & subs) noexcept {
	// Если список подпротоколов получен
	if(!subs.empty()) this->_http.subs(subs);
}
/**
 * bytesDetect Метод детекции сообщений по количеству байт
 * @param read  количество байт для детекции по чтению
 * @param write количество байт для детекции по записи
 */
void awh::client::WebSocket::bytesDetect(const scheme_t::mark_t read, const scheme_t::mark_t write) noexcept {
	// Устанавливаем количество байт на чтение
	this->_scheme.marker.read = read;
	// Устанавливаем количество байт на запись
	this->_scheme.marker.write = write;
	// Если минимальный размер данных для чтения, не установлен
	if(this->_scheme.marker.read.min == 0)
		// Устанавливаем размер минимальных для чтения данных по умолчанию
		this->_scheme.marker.read.min = BUFFER_READ_MIN;
	// Если максимальный размер данных для записи не установлен, устанавливаем по умолчанию
	if(this->_scheme.marker.write.max == 0)
		// Устанавливаем размер максимальных записываемых данных по умолчанию
		this->_scheme.marker.write.max = BUFFER_WRITE_MAX;
}
/**
 * waitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read    количество секунд для детекции по чтению
 * @param write   количество секунд для детекции по записи
 * @param connect количество секунд для детекции по подключению
 */
void awh::client::WebSocket::waitTimeDetect(const time_t read, const time_t write, const time_t connect) noexcept {
	// Устанавливаем количество секунд на чтение
	this->_scheme.timeouts.read = read;
	// Устанавливаем количество секунд на запись
	this->_scheme.timeouts.write = write;
	// Устанавливаем количество секунд на подключение
	this->_scheme.timeouts.connect = connect;
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
		if(!this->_thr.is()) this->_thr.init(threads);
		// Если многопоточность уже активированна
		else {
			// Выполняем завершение всех активных потоков
			this->_thr.wait();
			// Выполняем инициализацию нового тредпула
			this->_thr.init(threads);
		}
		// Устанавливаем простое чтение базы событий
		const_cast <client::core_t *> (this->_core)->easily(true);
	// Выполняем завершение всех потоков
	} else this->_thr.wait();
}
/**
 * proxy Метод установки прокси-сервера
 * @param uri    параметры прокси-сервера
 * @param family семейстово интернет протоколов (IPV4 / IPV6 / NIX)
 */
void awh::client::WebSocket::proxy(const string & uri, const scheme_t::family_t family) noexcept {
	// Если URI параметры переданы
	if(!uri.empty()){
		// Устанавливаем семейство интернет протоколов
		this->_scheme.proxy.family = family;
		// Устанавливаем параметры прокси-сервера
		this->_scheme.proxy.url = this->_uri.parse(uri);
		// Если данные параметров прокси-сервера получены
		if(!this->_scheme.proxy.url.empty()){
			// Если протокол подключения SOCKS5
			if(this->_scheme.proxy.url.schema.compare("socks5") == 0){
				// Устанавливаем тип прокси-сервера
				this->_scheme.proxy.type = proxy_t::type_t::SOCKS5;
				// Если требуется авторизация на прокси-сервере
				if(!this->_scheme.proxy.url.user.empty() && !this->_scheme.proxy.url.pass.empty())
					// Устанавливаем данные пользователя
					this->_scheme.proxy.socks5.user(this->_scheme.proxy.url.user, this->_scheme.proxy.url.pass);
			// Если протокол подключения HTTP
			} else if((this->_scheme.proxy.url.schema.compare("http") == 0) || (this->_scheme.proxy.url.schema.compare("https") == 0)) {
				// Устанавливаем тип прокси-сервера
				this->_scheme.proxy.type = proxy_t::type_t::HTTP;
				// Если требуется авторизация на прокси-сервере
				if(!this->_scheme.proxy.url.user.empty() && !this->_scheme.proxy.url.pass.empty())
					// Устанавливаем данные пользователя
					this->_scheme.proxy.http.user(this->_scheme.proxy.url.user, this->_scheme.proxy.url.pass);
			}
		}
	}
}
/**
 * mode Метод установки флага модуля
 * @param flag флаг модуля для установки
 */
void awh::client::WebSocket::mode(const u_short flag) noexcept {
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	this->_noinfo = (flag & (uint8_t) flag_t::NOINFO);
	// Устанавливаем флаг анбиндинга ядра сетевого модуля
	this->_unbind = !(flag & (uint8_t) flag_t::NOTSTOP);
	// Устанавливаем флаг поддержания автоматического подключения
	this->_scheme.alive = (flag & (uint8_t) flag_t::ALIVE);
	// Устанавливаем флаг ожидания входящих сообщений
	this->_scheme.wait = (flag & (uint8_t) flag_t::WAITMESS);
	// Устанавливаем флаг перехвата контекста компрессии для клиента
	this->_takeOverCli = (flag & (uint8_t) flag_t::TAKEOVERCLI);
	// Устанавливаем флаг перехвата контекста компрессии для сервера
	this->_takeOverSrv = (flag & (uint8_t) flag_t::TAKEOVERSRV);
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	const_cast <client::core_t *> (this->_core)->noInfo(flag & (uint8_t) flag_t::NOINFO);
	// Выполняем установку флага проверки домена
	const_cast <client::core_t *> (this->_core)->verifySSL(flag & (uint8_t) flag_t::VERIFYSSL);
}
/**
 * chunk Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::client::WebSocket::chunk(const size_t size) noexcept {
	// Устанавливаем размер чанка
	this->_http.chunk(size);
}
/**
 * segmentSize Метод установки размеров сегментов фрейма
 * @param size минимальный размер сегмента
 */
void awh::client::WebSocket::segmentSize(const size_t size) noexcept {
	// Если размер передан, устанавливаем
	if(size > 0) this->_frameSize = size;
}
/**
 * attempts Метод установки общего количества попыток
 * @param attempts общее количество попыток
 */
void awh::client::WebSocket::attempts(const uint8_t attempts) noexcept {
	// Если количество попыток передано, устанавливаем его
	if(attempts > 0) this->_attempts = attempts;
}
/**
 * userAgent Метод установки User-Agent для HTTP запроса
 * @param userAgent агент пользователя для HTTP запроса
 */
void awh::client::WebSocket::userAgent(const string & userAgent) noexcept {
	// Устанавливаем UserAgent
	if(!userAgent.empty()){
		// Устанавливаем пользовательского агента
		this->_http.userAgent(userAgent);
		// Устанавливаем пользовательского агента для прокси-сервера
		this->_scheme.proxy.http.userAgent(userAgent);
	}
}
/**
 * compress Метод установки метода компрессии
 * @param compress метод компрессии сообщений
 */
void awh::client::WebSocket::compress(const http_t::compress_t compress) noexcept {
	// Устанавливаем метод компрессии
	this->_compress = compress;
}
/**
 * user Метод установки параметров авторизации
 * @param login    логин пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 */
void awh::client::WebSocket::user(const string & login, const string & password) noexcept {
	// Если пользователь и пароль переданы
	if(!login.empty() && !password.empty())
		// Устанавливаем логин и пароль пользователя
		this->_http.user(login, password);
}
/**
 * keepAlive Метод установки жизни подключения
 * @param cnt   максимальное количество попыток
 * @param idle  интервал времени в секундах через которое происходит проверка подключения
 * @param intvl интервал времени в секундах между попытками
 */
void awh::client::WebSocket::keepAlive(const int cnt, const int idle, const int intvl) noexcept {
	// Выполняем установку максимального количества попыток
	this->_scheme.keepAlive.cnt = cnt;
	// Выполняем установку интервала времени в секундах через которое происходит проверка подключения
	this->_scheme.keepAlive.idle = idle;
	// Выполняем установку интервала времени в секундах между попытками
	this->_scheme.keepAlive.intvl = intvl;
}
/**
 * serv Метод установки данных сервиса
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::client::WebSocket::serv(const string & id, const string & name, const string & ver) noexcept {
	// Устанавливаем данные сервиса
	this->_http.serv(id, name, ver);
	// Устанавливаем данные сервиса для прокси-сервера
	this->_scheme.proxy.http.serv(id, name, ver);
}
/**
 * crypto Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::client::WebSocket::crypto(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Устанавливаем флаг шифрования
	this->_crypt = !pass.empty();
	// Устанавливаем соль шифрования
	this->_hash.salt(salt);
	// Устанавливаем пароль шифрования
	this->_hash.pass(pass);
	// Устанавливаем размер шифрования
	this->_hash.cipher(cipher);
	// Устанавливаем параметры шифрования
	if(this->_crypt) this->_http.crypto(pass, salt, cipher);
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::client::WebSocket::authType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Если объект авторизации создан
	this->_http.authType(type, hash);
}
/**
 * authTypeProxy Метод установки типа авторизации прокси-сервера
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::client::WebSocket::authTypeProxy(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Если объект авторизации создан
	this->_scheme.proxy.http.authType(type, hash);
}
/**
 * WebSocket Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::client::WebSocket::WebSocket(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 _nwk(fmk), _hash(log), _uri(fmk, &_nwk), _http(fmk, log, &_uri), _frame(fmk, log), _scheme(fmk, log),
 _action(action_t::NONE), _opcode(frame_t::opcode_t::TEXT), _compress(http_t::compress_t::NONE), _crypt(false),
 _close(false), _unbind(true), _freeze(false), _noinfo(false), _stopped(false), _compressed(false), _takeOverCli(false),
 _takeOverSrv(false), _attempt(0), _attempts(10), _wbitClient(0), _wbitServer(0), _aid(0), _code(0),
 _checkPoint(0), _frameSize(0xFA000), _fmk(fmk), _log(log), _core(core) {
	// Устанавливаем событие на запуск системы
	this->_scheme.callback.set <void (const size_t, awh::core_t *)> ("open", std::bind(&WebSocket::openCallback, this, _1, _2));
	// Устанавливаем функцию персистентного вызова
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("persist", std::bind(&WebSocket::persistCallback, this, _1, _2, _3));
	// Устанавливаем событие подключения
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("connect", std::bind(&WebSocket::connectCallback, this, _1, _2, _3));
	// Устанавливаем событие отключения
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("disconnect", std::bind(&WebSocket::disconnectCallback, this, _1, _2, _3));
	// Устанавливаем событие на подключение к прокси-серверу
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("connectProxy", std::bind(&WebSocket::proxyConnectCallback, this, _1, _2, _3));
	// Устанавливаем функцию чтения данных
	this->_scheme.callback.set <void (const char *, const size_t, const size_t, const size_t, awh::core_t *)> ("read", std::bind(&WebSocket::readCallback, this, _1, _2, _3, _4, _5));
	// Устанавливаем функцию записи данных
	this->_scheme.callback.set <void (const char *, const size_t, const size_t, const size_t, awh::core_t *)> ("write", std::bind(&WebSocket::writeCallback, this, _1, _2, _3, _4, _5));
	// Устанавливаем событие на чтение данных с прокси-сервера
	this->_scheme.callback.set <void (const char *, const size_t, const size_t, const size_t, awh::core_t *)> ("readProxy", std::bind(&WebSocket::proxyReadCallback, this, _1, _2, _3, _4, _5));
	// Устанавливаем событие на активацию шифрованного TLS канала
	this->_scheme.callback.set <bool (const uri_t::url_t &, const size_t, const size_t, awh::core_t *)> ("tls", std::bind(&WebSocket::enableTLSCallback, this, _1, _2, _3, _4));
	// Активируем персистентный запуск для работы пингов
	const_cast <client::core_t *> (this->_core)->persistEnable(true);
	// Добавляем схему сети в сетевое ядро
	const_cast <client::core_t *> (this->_core)->add(&this->_scheme);
	// Активируем асинхронный режим работы
	const_cast <client::core_t *> (this->_core)->mode(client::core_t::mode_t::ASYNC);
}
