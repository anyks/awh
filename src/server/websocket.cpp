/**
 * @file: websocket.cpp
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
#include <server/websocket.hpp>

/**
 * openCallback Функция обратного вызова при запуске работы
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::WebSocket::openCallback(const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((sid > 0) && (core != nullptr)){
		// Устанавливаем хост сервера
		reinterpret_cast <server::core_t *> (core)->init(sid, this->_port, this->_host);
		// Выполняем запуск сервера
		reinterpret_cast <server::core_t *> (core)->run(sid);
	}
}
/**
 * persistCallback Функция персистентного вызова
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::WebSocket::persistCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Получаем параметры подключения адъютанта
		websocket_scheme_t::coffer_t * adj = const_cast <websocket_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Получаем текущий штамп времени
			const time_t stamp = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
			// Если адъютант не ответил на пинг больше двух интервалов, отключаем его
			if(adj->close || ((stamp - adj->checkPoint) >= (PERSIST_INTERVAL * 5)))
				// Завершаем работу
				reinterpret_cast <server::core_t *> (core)->close(aid);
			// Отправляем запрос адъютанту
			else this->ping(aid, core, to_string(aid));
		}
	}
}
/**
 * connectCallback Функция обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::WebSocket::connectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Создаём адъютанта
		this->_scheme.set(aid);
		// Получаем параметры подключения адъютанта
		websocket_scheme_t::coffer_t * adj = const_cast <websocket_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Если нужно активировать многопоточность и она не активирована
			if(this->_threadsEnabled && !this->_thr.is())
				// Выполняем активацию тредпула
				this->_thr.init(this->_threadsCount);
			// Устанавливаем экшен выполнения
			adj->action = websocket_scheme_t::action_t::CONNECT;
			// Выполняем запуск обработчика событий
			this->handler(aid);
		}
	}
}
/**
 * disconnectCallback Функция обратного вызова при отключении от сервера
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::WebSocket::disconnectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Получаем параметры подключения адъютанта
		websocket_scheme_t::coffer_t * adj = const_cast <websocket_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Устанавливаем экшен выполнения
			adj->action = websocket_scheme_t::action_t::DISCONNECT;
			// Выполняем запуск обработчика событий
			this->handler(aid);
		}
	}
}
/**
 * readCallback Функция обратного вызова при чтении сообщений адъютанта
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::server::WebSocket::readCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (aid > 0) && (sid > 0)){
		// Получаем параметры подключения адъютанта
		websocket_scheme_t::coffer_t * adj = const_cast <websocket_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Если дисконнекта ещё не произошло
			if((adj->action == websocket_scheme_t::action_t::NONE) || (adj->action == websocket_scheme_t::action_t::READ)){
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
					adj->action = websocket_scheme_t::action_t::READ;
					// Добавляем полученные данные в буфер
					adj->buffer.payload.insert(adj->buffer.payload.end(), buffer, buffer + size);
					// Выполняем запуск обработчика событий
					this->handler(aid);
				}
			}
		}
	}
}
/**
 * writeCallback Функция обратного вызова при записи сообщений адъютанту
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::server::WebSocket::writeCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Получаем параметры подключения адъютанта
		websocket_scheme_t::coffer_t * adj = const_cast <websocket_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если параметры подключения адъютанта получены
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
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 * @return     результат разрешения к подключению адъютанта
 */
bool awh::server::WebSocket::acceptCallback(const string & ip, const string & mac, const u_int port, const size_t sid, awh::core_t * core) noexcept {
	// Результат работы функции
	bool result = true;
	// Если данные существуют
	if(!ip.empty() && !mac.empty() && (sid > 0) && (core != nullptr)){
		// Если функция обратного вызова установлена, проверяем
		if(this->_callback.accept != nullptr) result = this->_callback.accept(ip, mac, port, this);
	}
	// Разрешаем подключение адъютанту
	return result;
}
/**
 * handler Метод управления входящими методами
 * @param aid идентификатор адъютанта
 */
void awh::server::WebSocket::handler(const size_t aid) noexcept {
	// Получаем параметры подключения адъютанта
	websocket_scheme_t::coffer_t * adj = const_cast <websocket_scheme_t::coffer_t *> (this->_scheme.get(aid));
	// Если параметры подключения адъютанта получены
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
			while(loop && (adj->action != websocket_scheme_t::action_t::NONE)){
				// Определяем обрабатываемый экшен
				switch(static_cast <uint8_t> (adj->action)){
					// Если необходимо запустить экшен обработки данных поступающих с сервера
					case static_cast <uint8_t> (websocket_scheme_t::action_t::READ): this->actionRead(aid); break;
					// Если необходимо запустить экшен обработки подключения к серверу
					case static_cast <uint8_t> (websocket_scheme_t::action_t::CONNECT): this->actionConnect(aid); break;
					// Если необходимо запустить экшен обработки отключения от сервера
					case static_cast <uint8_t> (websocket_scheme_t::action_t::DISCONNECT): this->actionDisconnect(aid); break;
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
void awh::server::WebSocket::actionRead(const size_t aid) noexcept {
	// Если данные существуют
	if(aid > 0){
		// Получаем параметры подключения адъютанта
		websocket_scheme_t::coffer_t * adj = const_cast <websocket_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Объект сообщения
			mess_t mess;
			// Получаем объект биндинга ядра TCP/IP
			server::core_t * core = const_cast <server::core_t *> (this->_core);
			// Если рукопожатие не выполнено
			if(!reinterpret_cast <http_t *> (&adj->ws)->isHandshake()){
				// Выполняем парсинг полученных данных
				const size_t bytes = adj->ws.parse(adj->buffer.payload.data(), adj->buffer.payload.size());
				// Если все данные получены
				if(adj->ws.isEnd()){
					// Буфер данных для записи в сокет
					vector <char> buffer;
					// Метод компрессии данных
					http_t::compress_t compress = http_t::compress_t::NONE;
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Получаем данные запроса
						const auto & request = reinterpret_cast <http_t *> (&adj->ws)->request(true);
						// Если параметры запроса получены
						if(!request.empty()){
							// Выводим заголовок запроса
							cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << endl;
							// Выводим параметры запроса
							cout << string(request.begin(), request.end()) << endl;
							// Если тело запроса существует
							if(!adj->ws.body().empty())
								// Выводим сообщение о выводе чанка тела
								cout << this->_fmk->format("<body %u>", adj->ws.body().size()) << endl << endl;
							// Иначе устанавливаем перенос строки
							else cout << endl;
						}
					#endif
					// Выполняем проверку авторизации
					switch(static_cast <uint8_t> (adj->ws.getAuth())){
						// Если запрос выполнен удачно
						case static_cast <uint8_t> (http_t::stath_t::GOOD): {
							// Если рукопожатие выполнено
							if(adj->ws.isHandshake()){
								// Получаем метод компрессии HTML данных
								compress = adj->ws.compression();
								// Устанавливаем параметры шифрования
								if(this->_crypt) adj->ws.crypto(this->_pass, this->_salt, this->_cipher);
								// Проверяем версию протокола
								if(!adj->ws.checkVer()){
									// Выполняем сброс состояния HTTP парсера
									adj->ws.clear();
									// Получаем бинарные данные REST запроса
									buffer = adj->ws.reject(400, "Unsupported protocol version");
									// Если бинарные данные запроса получены, отправляем на сервер
									adj->stopped = !buffer.empty();
									// Завершаем работу
									break;
								}
								// Проверяем ключ адъютанта
								if(!adj->ws.checkKey()){
									// Выполняем сброс состояния HTTP парсера
									adj->ws.clear();
									// Получаем бинарные данные REST запроса
									buffer = adj->ws.reject(400, "Wrong client key");
									// Если бинарные данные запроса получены, отправляем на сервер
									adj->stopped = !buffer.empty();
									// Завершаем работу
									break;
								}
								// Выполняем сброс состояния HTTP парсера
								adj->ws.clear();
								// Получаем флаг шифрованных данных
								adj->crypt = adj->ws.isCrypt();
								// Получаем поддерживаемый метод компрессии
								adj->compress = adj->ws.compress();
								// Получаем размер скользящего окна сервера
								adj->wbitServer = adj->ws.wbitServer();
								// Получаем размер скользящего окна клиента
								adj->wbitClient = adj->ws.wbitClient();
								// Если разрешено выполнять перехват контекста компрессии для сервера
								if(adj->ws.serverTakeover())
									// Разрешаем перехватывать контекст компрессии для клиента
									adj->hash.takeoverCompress(true);
								// Если разрешено выполнять перехват контекста компрессии для клиента
								if(adj->ws.clientTakeover())
									// Разрешаем перехватывать контекст компрессии для сервера
									adj->hash.takeoverDecompress(true);
								// Получаем бинарные данные REST запроса
								buffer = adj->ws.response();
								// Если бинарные данные ответа получены
								if(!buffer.empty()){
									/**
									 * Если включён режим отладки
									 */
									#if defined(DEBUG_MODE)
										// Выводим заголовок ответа
										cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
										// Выводим параметры ответа
										cout << string(buffer.begin(), buffer.end()) << endl << endl;
									#endif
									// Выполняем отправку данных адъютанту
									core->write(buffer.data(), buffer.size(), aid);
									// Если функция обратного вызова установлена, выполняем
									if(this->_callback.active != nullptr) this->_callback.active(aid, mode_t::CONNECT, this);
									// Есла данных передано больше чем обработано
									if(adj->buffer.payload.size() > bytes)
										// Удаляем количество обработанных байт
										adj->buffer.payload.assign(adj->buffer.payload.begin() + bytes, adj->buffer.payload.end());
										// vector <decltype(adj->buffer.payload)::value_type> (adj->buffer.payload.begin() + bytes, adj->buffer.payload.end()).swap(adj->buffer.payload);
									// Если данных в буфере больше нет
									else {
										// Очищаем буфер собранных данных
										adj->buffer.payload.clear();
										// Если экшен соответствует, выполняем его сброс
										if(adj->action == websocket_scheme_t::action_t::READ)
											// Выполняем сброс экшена
											adj->action = websocket_scheme_t::action_t::NONE;
									}
									// Завершаем работу
									return;
								// Выполняем реджект
								} else {
									// Выполняем сброс состояния HTTP парсера
									adj->ws.clear();
									// Выполняем сброс состояния HTTP парсера
									adj->ws.reset();
									// Выполняем очистку буфера данных
									adj->buffer.payload.clear();
									// Формируем ответ, что страница не доступна
									buffer = adj->ws.reject(500);
									// Если бинарные данные запроса получены, отправляем на сервер
									adj->stopped = !buffer.empty();
								}
							// Сообщаем, что рукопожатие не выполнено
							} else {
								// Выполняем сброс состояния HTTP парсера
								adj->ws.clear();
								// Выполняем сброс состояния HTTP парсера
								adj->ws.reset();
								// Выполняем очистку буфера данных
								adj->buffer.payload.clear();
								// Формируем ответ, что страница не доступна
								buffer = adj->ws.reject(403);
								// Если бинарные данные запроса получены, отправляем на сервер
								adj->stopped = !buffer.empty();
							}
						} break;
						// Если запрос неудачный
						case static_cast <uint8_t> (http_t::stath_t::FAULT): {
							// Выполняем сброс состояния HTTP парсера
							adj->ws.clear();
							// Выполняем сброс состояния HTTP парсера
							adj->ws.reset();
							// Выполняем очистку буфера данных
							adj->buffer.payload.clear();
							// Формируем запрос авторизации
							buffer = adj->ws.reject(401);
						} break;
					}
					// Если бинарные данные запроса получены, отправляем на сервер
					if(!buffer.empty()){
						// Тело полезной нагрузки
						vector <char> payload;
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим заголовок ответа
							cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
							// Выводим параметры ответа
							cout << string(buffer.begin(), buffer.end()) << endl << endl;
						#endif
						// Устанавливаем метод компрессии данных ответа
						adj->ws.compress(compress);
						// Выполняем отправку заголовков сообщения
						core->write(buffer.data(), buffer.size(), aid);
						// Получаем данные тела запроса
						while(!(payload = adj->ws.payload()).empty()){
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								// Выводим сообщение о выводе чанка полезной нагрузки
								cout << this->_fmk->format("<chunk %u>", payload.size()) << endl;
							#endif
							// Выполняем отправку чанков
							core->write(payload.data(), payload.size(), aid);
						}
						// Если получение данных нужно остановить
						if(adj->stopped)
							// Выполняем запрет на получение входящих данных
							core->disabled(engine_t::method_t::READ, aid);
						// Если экшен соответствует, выполняем его сброс
						if(adj->action == websocket_scheme_t::action_t::READ)
							// Выполняем сброс экшена
							adj->action = websocket_scheme_t::action_t::NONE;
						// Завершаем работу
						return;
					}
					// Если экшен соответствует, выполняем его сброс
					if(adj->action == websocket_scheme_t::action_t::READ)
						// Выполняем сброс экшена
						adj->action = websocket_scheme_t::action_t::NONE;
					// Завершаем работу
					core->close(aid);
				// Если экшен соответствует, выполняем его сброс
				} else if(adj->action == websocket_scheme_t::action_t::READ)
					// Выполняем сброс экшена
					adj->action = websocket_scheme_t::action_t::NONE;
				// Завершаем работу
				return;
			// Если рукопожатие выполнено
			} else if(adj->allow.receive) {
				// Флаг удачного получения данных
				bool receive = false;
				// Создаём буфер сообщения
				vector <char> buffer;
				// Создаём объект шапки фрейма
				frame_t::head_t head;
				// Выполняем обработку полученных данных
				while(!adj->close && adj->allow.receive){
					// Выполняем чтение фрейма WebSocket
					const auto & data = this->_frame.get(head, adj->buffer.payload.data(), adj->buffer.payload.size());
					// Если буфер данных получен
					if(!data.empty()){
						// Проверяем состояние флагов RSV2 и RSV3
						if(head.rsv[1] || head.rsv[2]){
							// Создаём сообщение
							mess = mess_t(1002, "RSV2 and RSV3 must be clear");
							// Выполняем отключение адъютанта
							goto Stop;
						}
						// Если флаг компресси включён а данные пришли не сжатые
						if(head.rsv[0] && ((adj->compress == http_t::compress_t::NONE) ||
						  (head.optcode == frame_t::opcode_t::CONTINUATION) ||
						  ((static_cast <uint8_t> (head.optcode) > 0x07) && (static_cast <uint8_t> (head.optcode) < 0x0b)))){
							// Создаём сообщение
							mess = mess_t(1002, "RSV1 must be clear");
							// Выполняем отключение адъютанта
							goto Stop;
						}
						// Если опкоды требуют финального фрейма
						if(!head.fin && (static_cast <uint8_t> (head.optcode) > 0x07) && (static_cast <uint8_t> (head.optcode) < 0x0b)){
							// Создаём сообщение
							mess = mess_t(1002, "FIN must be set");
							// Выполняем отключение адъютанта
							goto Stop;
						}
						// Определяем тип ответа
						switch(static_cast <uint8_t> (head.optcode)){
							// Если ответом является PING
							case static_cast <uint8_t> (frame_t::opcode_t::PING):
								// Отправляем ответ адъютанту
								this->pong(aid, core, string(data.begin(), data.end()));
							break;
							// Если ответом является PONG
							case static_cast <uint8_t> (frame_t::opcode_t::PONG): {
								// Если идентификатор адъютанта совпадает
								if(memcmp(to_string(aid).c_str(), data.data(), data.size()) == 0)
									// Обновляем контрольную точку
									adj->checkPoint = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
							} break;
							// Если ответом является TEXT
							case static_cast <uint8_t> (frame_t::opcode_t::TEXT):
							// Если ответом является BINARY
							case static_cast <uint8_t> (frame_t::opcode_t::BINARY): {
								// Запоминаем полученный опкод
								adj->opcode = head.optcode;
								// Запоминаем, что данные пришли сжатыми
								adj->compressed = (head.rsv[0] && (adj->compress != http_t::compress_t::NONE));
								// Если сообщение не замаскированно
								if(!head.mask){
									// Создаём сообщение
									mess = mess_t(1002, "not masked frame from client");
									// Выполняем отключение адъютанта
									goto Stop;
								// Если список фрагментированных сообщений существует
								} else if(!adj->buffer.fragmes.empty()) {
									// Очищаем список фрагментированных сообщений
									adj->buffer.fragmes.clear();
									// Создаём сообщение
									mess = mess_t(1002, "opcode for subsequent fragmented messages should not be set");
									// Выполняем отключение адъютанта
									goto Stop;
								// Если сообщение является не последнем
								} else if(!head.fin)
									// Заполняем фрагментированное сообщение
									adj->buffer.fragmes.insert(adj->buffer.fragmes.end(), data.begin(), data.end());
								// Если сообщение является последним
								else buffer = std::forward <const vector <char>> (data);
							} break;
							// Если ответом является CONTINUATION
							case static_cast <uint8_t> (frame_t::opcode_t::CONTINUATION): {
								// Заполняем фрагментированное сообщение
								adj->buffer.fragmes.insert(adj->buffer.fragmes.end(), data.begin(), data.end());
								// Если сообщение является последним
								if(head.fin){
									// Выполняем копирование всех собранных сегментов
									buffer = std::forward <const vector <char>> (adj->buffer.fragmes);
									// Очищаем список фрагментированных сообщений
									adj->buffer.fragmes.clear();
								}
							} break;
							// Если ответом является CLOSE
							case static_cast <uint8_t> (frame_t::opcode_t::CLOSE): {
								// Создаём сообщение
								mess = this->_frame.message(data);
								// Выводим сообщение об ошибке
								this->error(aid, mess);
								// Если экшен соответствует, выполняем его сброс
								if(adj->action == websocket_scheme_t::action_t::READ)
									// Выполняем сброс экшена
									adj->action = websocket_scheme_t::action_t::NONE;
								// Завершаем работу
								core->close(aid);
								// Выходим из функции
								return;
							} break;
						}
					}
					// Если парсер обработал какое-то количество байт
					if((receive = ((head.frame > 0) && !adj->buffer.payload.empty()))){
						// Если размер буфера больше количества удаляемых байт
						if((receive = (adj->buffer.payload.size() >= head.frame)))
							// Удаляем количество обработанных байт
							adj->buffer.payload.assign(adj->buffer.payload.begin() + head.frame, adj->buffer.payload.end());
							// vector <decltype(adj->buffer.payload)::value_type> (adj->buffer.payload.begin() + head.frame, adj->buffer.payload.end()).swap(adj->buffer.payload);
					}
					// Если сообщения получены
					if(!buffer.empty()){
						// Если тредпул активирован
						if(this->_thr.is())
							// Добавляем в тредпул новую задачу на извлечение полученных сообщений
							this->_thr.push(std::bind(&websocket_t::extraction, this, aid, buffer, (adj->opcode == frame_t::opcode_t::TEXT)));
						// Если тредпул не активирован, выполняем извлечение полученных сообщений
						else this->extraction(aid, buffer, (adj->opcode == frame_t::opcode_t::TEXT));
						// Очищаем буфер полученного сообщения
						buffer.clear();
					}
					// Если данные мы все получили, выходим
					if(!receive || adj->buffer.payload.empty()) break;
				}
				// Если экшен соответствует, выполняем его сброс
				if(adj->action == websocket_scheme_t::action_t::READ)
					// Выполняем сброс экшена
					adj->action = websocket_scheme_t::action_t::NONE;
				// Выходим из функции
				return;
			}
			// Устанавливаем метку остановки адъютанта
			Stop:
			// Отправляем серверу сообщение
			this->sendError(aid, mess);
			// Если экшен соответствует, выполняем его сброс
			if(adj->action == websocket_scheme_t::action_t::READ)
				// Выполняем сброс экшена
				adj->action = websocket_scheme_t::action_t::NONE;
		}
	}
}
/**
 * actionConnect Метод обработки экшена подключения к серверу
 * @param aid идентификатор адъютанта
 */
void awh::server::WebSocket::actionConnect(const size_t aid) noexcept {
	// Если данные существуют
	if(aid > 0){
		// Получаем параметры подключения адъютанта
		websocket_scheme_t::coffer_t * adj = const_cast <websocket_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Если данные необходимо зашифровать
			if(this->_crypt){
				// Устанавливаем соль шифрования
				adj->hash.salt(this->_salt);
				// Устанавливаем пароль шифрования
				adj->hash.pass(this->_pass);
				// Устанавливаем размер шифрования
				adj->hash.cipher(this->_cipher);
			}
			// Разрешаем перехватывать контекст для клиента
			adj->ws.clientTakeover(this->_takeOverCli);
			// Разрешаем перехватывать контекст для сервера
			adj->ws.serverTakeover(this->_takeOverSrv);
			// Разрешаем перехватывать контекст компрессии
			adj->hash.takeoverCompress(this->_takeOverSrv);
			// Разрешаем перехватывать контекст декомпрессии
			adj->hash.takeoverDecompress(this->_takeOverCli);
			// Устанавливаем данные сервиса
			adj->ws.serv(this->_sid, this->_name, this->_ver);
			// Устанавливаем поддерживаемые сабпротоколы
			if(!this->_subs.empty()) adj->ws.subs(this->_subs);
			// Устанавливаем метод компрессии поддерживаемый сервером
			adj->ws.compress(this->_scheme.compress);
			// Если сервер требует авторизацию
			if(this->_authType != auth_t::type_t::NONE){
				// Определяем тип авторизации
				switch(static_cast <uint8_t> (this->_authType)){
					// Если тип авторизации Basic
					case static_cast <uint8_t> (auth_t::type_t::BASIC): {
						// Устанавливаем параметры авторизации
						adj->ws.authType(this->_authType);
						// Устанавливаем функцию проверки авторизации
						adj->ws.authCallback(this->_callback.checkAuth);
					} break;
					// Если тип авторизации Digest
					case static_cast <uint8_t> (auth_t::type_t::DIGEST): {
						// Устанавливаем название сервера
						adj->ws.realm(this->_realm);
						// Устанавливаем временный ключ сессии сервера
						adj->ws.opaque(this->_opaque);
						// Устанавливаем параметры авторизации
						adj->ws.authType(this->_authType, this->_authHash);
						// Устанавливаем функцию извлечения пароля
						adj->ws.extractPassCallback(this->_callback.extractPass);
					} break;
				}
			}
			// Если экшен соответствует, выполняем его сброс
			if(adj->action == websocket_scheme_t::action_t::CONNECT)
				// Выполняем сброс экшена
				adj->action = websocket_scheme_t::action_t::NONE;
		}
	}
}
/**
 * actionDisconnect Метод обработки экшена отключения от сервера
 * @param aid идентификатор адъютанта
 */
void awh::server::WebSocket::actionDisconnect(const size_t aid) noexcept {
	// Если идентификатор адъютанта существует
	if(aid > 0){
		// Получаем параметры подключения адъютанта
		websocket_scheme_t::coffer_t * adj = const_cast <websocket_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Если экшен соответствует, выполняем его сброс
			if(adj->action == websocket_scheme_t::action_t::DISCONNECT)
				// Выполняем сброс экшена
				adj->action = websocket_scheme_t::action_t::NONE;
		}
		// Добавляем в очередь список мусорных адъютантов
		this->_garbage.emplace(aid, this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS));
		// Если функция обратного вызова установлена, выполняем
		if(this->_callback.active != nullptr)
			// Выполняем функцию обратного вызова
			this->_callback.active(aid, mode_t::DISCONNECT, this);
	}
}
/**
 * garbage Метод удаления мусорных адъютантов
 * @param tid  идентификатор таймера
 * @param core объект сетевого ядра
 */
void awh::server::WebSocket::garbage(const u_short tid, awh::core_t * core) noexcept {
	// Если список мусорных адъютантов не пустой
	if(!this->_garbage.empty()){
		// Получаем текущее значение времени
		const time_t date = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
		// Выполняем переход по всему списку мусорных адъютантов
		for(auto it = this->_garbage.begin(); it != this->_garbage.end();){
			// Если адъютант уже давно удалился
			if((date - it->second) >= 10000){
				// Получаем параметры подключения адъютанта
				websocket_scheme_t::coffer_t * adj = const_cast <websocket_scheme_t::coffer_t *> (this->_scheme.get(it->first));
				// Если параметры подключения адъютанта получены
				if(adj != nullptr)
					// Устанавливаем флаг отключения
					adj->close = true;
				// Выполняем удаление параметров адъютанта
				this->_scheme.rm(it->first);
				// Выполняем удаление объекта адъютантов из списка мусора
				it = this->_garbage.erase(it);
			// Выполняем пропуск адъютанта
			} else ++it;
		}
	}
	// Устанавливаем таймаут времени на удаление мусорных адъютантов раз в 10 секунд
	this->_timer.setTimeout(10000, (function <void (const u_short, awh::core_t *)>) std::bind(&websocket_t::garbage, this, _1, _2));
}
/**
 * error Метод вывода сообщений об ошибках работы адъютанта
 * @param aid     идентификатор адъютанта
 * @param message сообщение с описанием ошибки
 */
void awh::server::WebSocket::error(const size_t aid, const mess_t & message) const noexcept {
	// Получаем параметры подключения адъютанта
	websocket_scheme_t::coffer_t * adj = const_cast <websocket_scheme_t::coffer_t *> (this->_scheme.get(aid));
	// Если параметры подключения адъютанта получены
	if(adj != nullptr){
		// Очищаем список буффер бинарных данных
		adj->buffer.payload.clear();
		// Очищаем список фрагментированных сообщений
		adj->buffer.fragmes.clear();
	}
	// Если идентификатор адъютанта передан и код ошибки указан
	if((aid > 0) && (message.code > 0)){
		// Если сообщение об ошибке пришло
		if(!message.text.empty()){
			// Если тип сообщения получен
			if(!message.type.empty())
				// Выводим в лог сообщение
				this->_log->print("%s - %s [%u]", log_t::flag_t::WARNING, message.type.c_str(), message.text.c_str(), message.code);
			// Иначе выводим сообщение в упрощёном виде
			else this->_log->print("%s [%u]", log_t::flag_t::WARNING, message.text.c_str(), message.code);
			// Если функция обратного вызова установлена, выводим полученное сообщение
			if(this->_callback.error != nullptr) this->_callback.error(aid, message.code, message.text, const_cast <WebSocket *> (this));
		}
	}
}
/**
 * extraction Метод извлечения полученных данных
 * @param aid    идентификатор адъютанта
 * @param buffer данные в чистом виде полученные с сервера
 * @param utf8   данные передаются в текстовом виде
 */
void awh::server::WebSocket::extraction(const size_t aid, const vector <char> & buffer, const bool utf8) const noexcept {
	// Если буфер данных передан
	if((aid > 0) && !buffer.empty() && (this->_callback.message != nullptr)){
		// Получаем параметры подключения адъютанта
		websocket_scheme_t::coffer_t * adj = const_cast <websocket_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Выполняем блокировку потока	
			const lock_guard <recursive_mutex> lock(adj->mtx);
			// Если данные пришли в сжатом виде
			if(adj->compressed && (adj->compress != http_t::compress_t::NONE)){
				// Декомпрессионные данные
				vector <char> data;
				// Определяем метод компрессии
				switch(static_cast <uint8_t> (adj->compress)){
					// Если метод компрессии выбран Deflate
					case static_cast <uint8_t> (http_t::compress_t::DEFLATE): {
						// Устанавливаем размер скользящего окна
						adj->hash.wbit(adj->wbitClient);
						// Добавляем хвост в полученные данные
						adj->hash.setTail(* const_cast <vector <char> *> (&buffer));
						// Выполняем декомпрессию полученных данных
						data = adj->hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::DEFLATE);
					} break;
					// Если метод компрессии выбран GZip
					case static_cast <uint8_t> (http_t::compress_t::GZIP):
						// Выполняем декомпрессию полученных данных
						data = adj->hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::GZIP);
					break;
					// Если метод компрессии выбран Brotli
					case static_cast <uint8_t> (http_t::compress_t::BROTLI):
						// Выполняем декомпрессию полученных данных
						data = adj->hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::BROTLI);
					break;
				}
				// Если данные получены
				if(!data.empty()){
					// Если нужно производить дешифрование
					if(adj->crypt){
						// Выполняем шифрование переданных данных
						const auto & res = adj->hash.decrypt(data.data(), data.size());
						// Отправляем полученный результат
						if(!res.empty()){
							// Отправляем полученный результат
							this->_callback.message(aid, res, utf8, const_cast <WebSocket *> (this));
							// Выходим из функции
							return;
						}
					}
					// Выводим сообщение так - как оно пришло
					this->_callback.message(aid, data, utf8, const_cast <WebSocket *> (this));
				// Выводим сообщение об ошибке
				} else {
					// Создаём сообщение
					mess_t mess(1007, "received data decompression error");
					// Получаем буфер сообщения
					data = this->_frame.message(mess);
					// Если данные сообщения получены
					if((adj->stopped = !data.empty())){
						// Выполняем запрет на получение входящих данных
						const_cast <server::core_t *> (this->_core)->disabled(engine_t::method_t::READ, aid);
						// Выполняем отправку сообщения адъютанту
						const_cast <server::core_t *> (this->_core)->write(data.data(), data.size(), aid);
					// Завершаем работу
					} else const_cast <server::core_t *> (this->_core)->close(aid);
				}
			// Если функция обратного вызова установлена, выводим полученное сообщение
			} else {
				// Если нужно производить дешифрование
				if(adj->crypt){
					// Выполняем шифрование переданных данных
					const auto & res = adj->hash.decrypt(buffer.data(), buffer.size());
					// Отправляем полученный результат
					if(!res.empty()){
						// Отправляем полученный результат
						this->_callback.message(aid, res, utf8, const_cast <WebSocket *> (this));
						// Выходим из функции
						return;
					}
				}
				// Выводим сообщение так - как оно пришло
				this->_callback.message(aid, buffer, utf8, const_cast <WebSocket *> (this));
			}
		}
	}
}
/**
 * pong Метод ответа на проверку о доступности сервера
 * @param aid  идентификатор адъютанта
 * @param core объект сетевого ядра
 * @param      message сообщение для отправки
 */
void awh::server::WebSocket::pong(const size_t aid, awh::core_t * core, const string & message) noexcept {
	// Если необходимые данные переданы
	if((aid > 0) && (core != nullptr)){
		// Получаем параметры подключения адъютанта
		websocket_scheme_t::coffer_t * adj = const_cast <websocket_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если отправка сообщений разблокированна
		if((adj != nullptr) && adj->allow.send){
			// Создаём буфер для отправки
			const auto & buffer = this->_frame.pong(message);
			// Если буфер данных получен
			if(!buffer.empty())
				// Выполняем отправку сообщения адъютанту
				core->write(buffer.data(), buffer.size(), aid);
		}
	}
}
/**
 * ping Метод проверки доступности сервера
 * @param aid  идентификатор адъютанта
 * @param core объект сетевого ядра
 * @param      message сообщение для отправки
 */
void awh::server::WebSocket::ping(const size_t aid, awh::core_t * core, const string & message) noexcept {
	// Если необходимые данные переданы
	if((aid > 0) && (core != nullptr) && core->working()){
		// Получаем параметры подключения адъютанта
		websocket_scheme_t::coffer_t * adj = const_cast <websocket_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если отправка сообщений разблокированна
		if((adj != nullptr) && adj->allow.send){
			// Создаём буфер для отправки
			const auto & buffer = this->_frame.ping(message);
			// Если буфер данных получен
			if(!buffer.empty())
				// Выполняем отправку сообщения адъютанту
				core->write(buffer.data(), buffer.size(), aid);
		}
	}
}
/**
 * init Метод инициализации WebSocket адъютанта
 * @param socket   unix-сокет для биндинга
 * @param compress метод сжатия передаваемых сообщений
 */
void awh::server::WebSocket::init(const string & socket, const http_t::compress_t compress) noexcept {
	// Устанавливаем тип компрессии
	this->_scheme.compress = compress;
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Выполняем установку unix-сокет
		const_cast <server::core_t *> (this->_core)->unixSocket(socket);
	#endif
}
/**
 * init Метод инициализации WebSocket адъютанта
 * @param port     порт сервера
 * @param host     хост сервера
 * @param compress метод сжатия передаваемых сообщений
 */
void awh::server::WebSocket::init(const u_int port, const string & host, const http_t::compress_t compress) noexcept {
	// Устанавливаем порт сервера
	this->_port = port;
	// Устанавливаем хост сервера
	this->_host = host;
	// Устанавливаем тип компрессии
	this->_scheme.compress = compress;
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
void awh::server::WebSocket::on(function <void (const size_t, const mode_t, WebSocket *)> callback) noexcept {
	// Устанавливаем функцию запуска и остановки
	this->_callback.active = callback;
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибок
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket::on(function <void (const size_t, const u_int, const string &, WebSocket *)> callback) noexcept {
	// Устанавливаем функцию получения ошибок
	this->_callback.error = callback;
}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket::on(function <void (const size_t, const vector <char> &, const bool, WebSocket *)> callback) noexcept {
	// Устанавливаем функцию получения сообщений с сервера
	this->_callback.message = callback;
}
/**
 * on Метод добавления функции извлечения пароля
 * @param callback функция обратного вызова для извлечения пароля
 */
void awh::server::WebSocket::on(function <string (const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова для извлечения пароля
	this->_callback.extractPass = callback;
}
/**
 * on Метод добавления функции обработки авторизации
 * @param callback функция обратного вызова для обработки авторизации
 */
void awh::server::WebSocket::on(function <bool (const string &, const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова для обработки авторизации
	this->_callback.checkAuth = callback;
}
/**
 * on Метод установки функции обратного вызова на событие активации адъютанта на сервере
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket::on(function <bool (const string &, const string &, const u_int, WebSocket *)> callback) noexcept {
	// Устанавливаем функцию запуска и остановки
	this->_callback.accept = callback;
}
/**
 * sendError Метод отправки сообщения об ошибке
 * @param aid  идентификатор адъютанта
 * @param mess отправляемое сообщение об ошибке
 */
void awh::server::WebSocket::sendError(const size_t aid, const mess_t & mess) const noexcept {
	// Если подключение выполнено
	if(this->_core->working()){
		// Если код ошибки относится к WebSocket
		if(mess.code >= 1000){
			// Получаем параметры подключения адъютанта
			websocket_scheme_t::coffer_t * adj = const_cast <websocket_scheme_t::coffer_t *> (this->_scheme.get(aid));
			// Получаем объект биндинга ядра TCP/IP
			server::core_t * core = const_cast <server::core_t *> (this->_core);
			// Если параметры подключения адъютанта получены
			if(adj != nullptr){
				// Запрещаем получение данных
				adj->allow.receive = false;
				// Выполняем остановку получения данных
				core->disabled(engine_t::method_t::READ, aid);
			}
			// Если отправка сообщений разблокированна
			if((adj != nullptr) && adj->allow.send){
				// Получаем буфер сообщения
				const auto & buffer = this->_frame.message(mess);
				// Если данные сообщения получены
				if((adj->stopped = !buffer.empty())){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим заголовок ответа
						cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
						// Выводим отправляемое сообщение
						cout << this->_fmk->format("%s [%u]", mess.text.c_str(), mess.code) << endl << endl;
					#endif
					// Выполняем отправку сообщения адъютанту
					core->write(buffer.data(), buffer.size(), aid);
					// Выходим из функции
					return;
				}
			}
		}
		// Завершаем работу
		const_cast <server::core_t *> (this->_core)->close(aid);
	}
}
/**
 * send Метод отправки сообщения на сервер
 * @param aid     идентификатор адъютанта
 * @param message буфер сообщения в бинарном виде
 * @param size    размер сообщения в байтах
 * @param utf8    данные передаются в текстовом виде
 */
void awh::server::WebSocket::send(const size_t aid, const char * message, const size_t size, const bool utf8) noexcept {
	// Если подключение выполнено
	if(this->_core->working() && (aid > 0) && (size > 0) && (message != nullptr)){
		// Получаем параметры подключения адъютанта
		websocket_scheme_t::coffer_t * adj = const_cast <websocket_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если отправка сообщений разблокированна
		if((adj != nullptr) && adj->allow.send){
			// Выполняем блокировку отправки сообщения
			adj->allow.send = !adj->allow.send;
			// Если рукопожатие выполнено
			if(adj->ws.isHandshake()){
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
				frame_t::head_t head(true, false);
				// Если нужно производить шифрование
				if(this->_crypt){
					// Выполняем шифрование переданных данных
					buffer = adj->hash.encrypt(message, size);
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
				head.rsv[0] = ((size >= 1024) && (adj->compress != http_t::compress_t::NONE));
				// Если необходимо сжимать сообщение перед отправкой
				if(head.rsv[0]){
					// Компрессионные данные
					vector <char> data;
					// Определяем метод компрессии
					switch(static_cast <uint8_t> (adj->compress)){
						// Если метод компрессии выбран Deflate
						case static_cast <uint8_t> (http_t::compress_t::DEFLATE): {
							// Устанавливаем размер скользящего окна
							adj->hash.wbit(adj->wbitServer);
							// Выполняем компрессию полученных данных
							data = adj->hash.compress(message, size, hash_t::method_t::DEFLATE);
							// Удаляем хвост в полученных данных
							adj->hash.rmTail(data);
						} break;
						// Если метод компрессии выбран GZip
						case static_cast <uint8_t> (http_t::compress_t::GZIP):
							// Выполняем компрессию полученных данных
							data = adj->hash.compress(message, size, hash_t::method_t::GZIP);
						break;
						// Если метод компрессии выбран Brotli
						case static_cast <uint8_t> (http_t::compress_t::BROTLI):
							// Выполняем компрессию полученных данных
							data = adj->hash.compress(message, size, hash_t::method_t::BROTLI);
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
							const_cast <server::core_t *> (this->_core)->write(buffer.data(), buffer.size(), aid);
						// Иначе просто выходим
						else break;
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
						const_cast <server::core_t *> (this->_core)->write(buffer.data(), buffer.size(), aid);
				}
			}
			// Выполняем разблокировку отправки сообщения
			adj->allow.send = !adj->allow.send;
		}
	}
}
/**
 * port Метод получения порта подключения адъютанта
 * @param aid идентификатор адъютанта
 * @return    порт подключения адъютанта
 */
u_int awh::server::WebSocket::port(const size_t aid) const noexcept {
	// Выводим результат
	return this->_scheme.getPort(aid);
}
/**
 * ip Метод получения IP адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес интернет подключения адъютанта
 */
const string & awh::server::WebSocket::ip(const size_t aid) const noexcept {
	// Выводим результат
	return this->_scheme.getIp(aid);
}
/**
 * mac Метод получения MAC адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес устройства адъютанта
 */
const string & awh::server::WebSocket::mac(const size_t aid) const noexcept {
	// Выводим результат
	return this->_scheme.getMac(aid);
}
/**
 * stop Метод остановки сервера
 */
void awh::server::WebSocket::stop() noexcept {
	// Если подключение выполнено
	if(this->_core->working())
		// Завершаем работу, если разрешено остановить
		const_cast <server::core_t *> (this->_core)->stop();
}
/**
 * start Метод запуска сервера
 */
void awh::server::WebSocket::start() noexcept {
	// Если биндинг не запущен, выполняем запуск биндинга
	if(!this->_core->working())
		// Выполняем запуск биндинга
		const_cast <server::core_t *> (this->_core)->start();
}
/**
 * multiThreads Метод активации многопоточности
 * @param threads количество потоков для активации
 * @param mode    флаг активации/деактивации мультипоточности
 */
void awh::server::WebSocket::multiThreads(const size_t threads, const bool mode) noexcept {
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
 * sub Метод установки подпротокола поддерживаемого сервером
 * @param sub подпротокол для установки
 */
void awh::server::WebSocket::sub(const string & sub) noexcept {
	// Устанавливаем подпротокол
	if(!sub.empty()) this->_subs.push_back(sub);
}
/**
 * subs Метод установки списка подпротоколов поддерживаемых сервером
 * @param subs подпротоколы для установки
 */
void awh::server::WebSocket::subs(const vector <string> & subs) noexcept {
	// Если список подпротоколов получен
	if(!subs.empty()) this->_subs = subs;
}
/**
 * sub Метод получения выбранного сабпротокола
 * @param aid идентификатор адъютанта
 * @return    название поддерживаемого сабпротокола
 */
const string awh::server::WebSocket::sub(const size_t aid) const noexcept {
	// Результат работы функции
	string result = "";
	// Если идентификатор адъютанта передан
	if(aid > 0){
		// Получаем параметры подключения адъютанта
		websocket_scheme_t::coffer_t * adj = const_cast <websocket_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr) result = adj->ws.sub();
	}
	// Выводим результат
	return result;
}
/**
 * waitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read  количество секунд для детекции по чтению
 * @param write количество секунд для детекции по записи
 */
void awh::server::WebSocket::waitTimeDetect(const time_t read, const time_t write) noexcept {
	// Устанавливаем количество секунд на чтение
	this->_scheme.timeouts.read = read;
	// Устанавливаем количество секунд на запись
	this->_scheme.timeouts.write = write;
}
/**
 * bytesDetect Метод детекции сообщений по количеству байт
 * @param read  количество байт для детекции по чтению
 * @param write количество байт для детекции по записи
 */
void awh::server::WebSocket::bytesDetect(const scheme_t::mark_t read, const scheme_t::mark_t write) noexcept {
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
 * realm Метод установки название сервера
 * @param realm название сервера
 */
void awh::server::WebSocket::realm(const string & realm) noexcept {
	// Устанавливаем название сервера
	this->_realm = realm;
}
/**
 * opaque Метод установки временного ключа сессии сервера
 * @param opaque временный ключ сессии сервера
 */
void awh::server::WebSocket::opaque(const string & opaque) noexcept {
	// Устанавливаем временный ключ сессии сервера
	this->_opaque = opaque;
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::server::WebSocket::authType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Устанавливаем алгоритм шифрования для Digest авторизации
	this->_authHash = hash;
	// Устанавливаем тип авторизации
	this->_authType = type;
}
/**
 * mode Метод установки флага модуля
 * @param flag флаг модуля для установки
 */
void awh::server::WebSocket::mode(const u_short flag) noexcept {
	// Устанавливаем флаг ожидания входящих сообщений
	this->_scheme.wait = (flag & static_cast <uint8_t> (flag_t::WAIT_MESS));
	// Устанавливаем флаг перехвата контекста компрессии для клиента
	this->_takeOverCli = (flag & static_cast <uint8_t> (flag_t::TAKEOVER_CLIENT));
	// Устанавливаем флаг перехвата контекста компрессии для сервера
	this->_takeOverSrv = (flag & static_cast <uint8_t> (flag_t::TAKEOVER_SERVER));
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	const_cast <server::core_t *> (this->_core)->noInfo(flag & static_cast <uint8_t> (flag_t::NOT_INFO));
}
/**
 * total Метод установки максимального количества одновременных подключений
 * @param total максимальное количество одновременных подключений
 */
void awh::server::WebSocket::total(const u_short total) noexcept {
	// Устанавливаем максимальное количество одновременных подключений
	const_cast <server::core_t *> (this->_core)->total(this->_scheme.sid, total);
}
/**
 * segmentSize Метод установки размеров сегментов фрейма
 * @param size минимальный размер сегмента
 */
void awh::server::WebSocket::segmentSize(const size_t size) noexcept {
	// Если размер передан, устанавливаем
	if(size > 0) this->_frameSize = size;
}
/**
 * clusterAutoRestart Метод установки флага перезапуска процессов
 * @param mode флаг перезапуска процессов
 */
void awh::server::WebSocket::clusterAutoRestart(const bool mode) noexcept {
	// Выполняем установку флага автоматического перезапуска
	const_cast <server::core_t *> (this->_core)->clusterAutoRestart(this->_scheme.sid, mode);
}
/**
 * compress Метод установки метода сжатия
 * @param метод сжатия сообщений
 */
void awh::server::WebSocket::compress(const http_t::compress_t compress) noexcept {
	// Устанавливаем метод компрессии
	this->_scheme.compress = compress;
}
/**
 * keepAlive Метод установки жизни подключения
 * @param cnt   максимальное количество попыток
 * @param idle  интервал времени в секундах через которое происходит проверка подключения
 * @param intvl интервал времени в секундах между попытками
 */
void awh::server::WebSocket::keepAlive(const int cnt, const int idle, const int intvl) noexcept {
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
void awh::server::WebSocket::serv(const string & id, const string & name, const string & ver) noexcept {
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
void awh::server::WebSocket::crypto(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
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
 * WebSocket Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::server::WebSocket::WebSocket(const server::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 _pid(getpid()), _port(SERVER_PORT), _host(""), _frame(fmk, log), _timer(fmk, log), _scheme(fmk, log),
 _sid(AWH_SHORT_NAME), _ver(AWH_VERSION), _name(AWH_NAME), _realm(""), _opaque(""), _pass(""), _salt(""),
 _cipher(hash_t::cipher_t::AES128), _authHash(auth_t::hash_t::MD5), _authType(auth_t::type_t::NONE),
 _crypt(false), _takeOverCli(false), _takeOverSrv(false), _frameSize(0xFA000),
 _threadsCount(0), _threadsEnabled(false), _fmk(fmk), _log(log), _core(core) {
	// Устанавливаем событие на запуск системы
	this->_scheme.callback.set <void (const size_t, awh::core_t *)> ("open", std::bind(&WebSocket::openCallback, this, _1, _2));
	// Устанавливаем функцию персистентного вызова
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("persist", std::bind(&WebSocket::persistCallback, this, _1, _2, _3));
	// Устанавливаем событие подключения
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("connect", std::bind(&WebSocket::connectCallback, this, _1, _2, _3));
	// Устанавливаем событие отключения
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("disconnect", std::bind(&WebSocket::disconnectCallback, this, _1, _2, _3));
	// Устанавливаем функцию чтения данных
	this->_scheme.callback.set <void (const char *, const size_t, const size_t, const size_t, awh::core_t *)> ("read", std::bind(&WebSocket::readCallback, this, _1, _2, _3, _4, _5));
	// Устанавливаем функцию записи данных
	this->_scheme.callback.set <void (const char *, const size_t, const size_t, const size_t, awh::core_t *)> ("write", std::bind(&WebSocket::writeCallback, this, _1, _2, _3, _4, _5));
	// Добавляем событие аццепта адъютанта
	this->_scheme.callback.set <bool (const string &, const string &, const u_int, const size_t, awh::core_t *)> ("accept", std::bind(&WebSocket::acceptCallback, this, _1, _2, _3, _4, _5));
	// Выполняем биндинг ядра локальных таймеров
	const_cast <server::core_t *> (this->_core)->bind(&this->_timer);
	// Активируем персистентный запуск для работы пингов
	const_cast <server::core_t *> (this->_core)->persistEnable(true);
	// Добавляем схему сети в сетевое ядро
	const_cast <server::core_t *> (this->_core)->add(&this->_scheme);
	// Устанавливаем таймаут времени на удаление мусорных адъютантов раз в 10 секунд
	this->_timer.setTimeout(10000, (function <void (const u_short, awh::core_t *)>) std::bind(&WebSocket::garbage, this, _1, _2));
}
/**
 * ~WebSocket Деструктор
 */
awh::server::WebSocket::~WebSocket() noexcept {
	// Если многопоточность активированна
	if(this->_threadsEnabled && this->_thr.is())
		// Выполняем завершение всех потоков
		this->_thr.wait();
}