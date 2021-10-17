/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <server/ws.hpp>

/**
 * openCallback Функция обратного вызова при запуске работы
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::WebSocketServer::openCallback(const size_t wid, core_t * core, void * ctx) noexcept {
	// Устанавливаем хост сервера
	reinterpret_cast <coreSrv_t *> (core)->init(wid, 2222, "127.0.0.1");
	// Выполняем запуск сервера
	core->run(wid);
}
/**
 * pingCallback Метод пинга адъютанта
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::WebSocketServer::pingCallback(const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		wsSrv_t * ws = reinterpret_cast <wsSrv_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workSrvWss_t::adjp_t * adj = const_cast <workSrvWss_t::adjp_t *> (ws->worker.getAdj(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Получаем текущий штамп времени
			const time_t stamp = ws->fmk->unixTimestamp();
			// Если адъютант не ответил на пинг больше двух интервалов, отключаем его
			if((stamp - adj->checkPoint) >= (PING_INTERVAL * 2))
				// Завершаем работу
				core->close(aid);
			// Отправляем запрос адъютанту
			else ws->ping(aid, core, to_string(aid));
		}
	}
}
/**
 * connectCallback Функция обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::WebSocketServer::connectCallback(const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		wsSrv_t * ws = reinterpret_cast <wsSrv_t *> (ctx);
		// Создаём адъютанта
		ws->worker.createAdj(aid);
	}
}
/**
 * disconnectCallback Функция обратного вызова при отключении от сервера
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::WebSocketServer::disconnectCallback(const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		wsSrv_t * ws = reinterpret_cast <wsSrv_t *> (ctx);
		// Выполняем удаление параметров адъютанта
		ws->worker.removeAdj(aid);

		cout << " ^^^^^^^^^^^ STOP ADJ " << aid << endl;
	}
}
/**
 * writeCallback Функция обратного вызова при записи сообщения на клиенте
 * @param size размер записанных в сокет байт
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::WebSocketServer::writeCallback(const size_t size, const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept {

	cout << " ********** WRITE ********** " << size << endl;

	// Если данные существуют
	if((size > 0) && (aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		wsSrv_t * ws = reinterpret_cast <wsSrv_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workSrvWss_t::adjp_t * adj = const_cast <workSrvWss_t::adjp_t *> (ws->worker.getAdj(aid));
		// Если параметры подключения адъютанта получены
		if((adj != nullptr) && (adj->stopBytes > 0)){
			// Если размер полученных байт соответствует
			if(adj->stopBytes == size) core->close(aid);
		}
	}
}
/**
 * acceptCallback Функция обратного вызова при проверке подключения клиента
 * @param ip   адрес интернет подключения клиента
 * @param mac  мак-адрес подключившегося клиента
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 * @return     результат разрешения к подключению клиента
 */
bool awh::WebSocketServer::acceptCallback(const string & ip, const string & mac, const size_t wid, core_t * core, void * ctx) noexcept {
	// Разрешаем подключение клиенту
	return true;
}
/**
 * readCallback Функция обратного вызова при чтении сообщения с клиента
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param wid    идентификатор воркера
 * @param core   объект биндинга TCP/IP
 * @param ctx    передаваемый контекст модуля
 */
void awh::WebSocketServer::readCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (aid > 0) && (wid > 0)){
		// Получаем контекст модуля
		wsSrv_t * ws = reinterpret_cast <wsSrv_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workSrvWss_t::adjp_t * adj = const_cast <workSrvWss_t::adjp_t *> (ws->worker.getAdj(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Объект сообщения
			mess_t mess;
			// Если рукопожатие не выполнено
			if(!reinterpret_cast <http_t *> (&adj->http)->isHandshake()){
				// Выполняем парсинг полученных данных
				adj->http.parse(buffer, size);
				// Если все данные получены
				if(adj->http.isEnd()){
					// Бинарный буфер ответа сервера
					vector <char> response;
					// Выполняем проверку авторизации
					switch((uint8_t) adj->http.getAuth()){
						// Если запрос выполнен удачно
						case (uint8_t) http_t::stath_t::GOOD: {
							// Если рукопожатие выполнено
							if(adj->http.isHandshake()){
								// Проверяем версию протокола
								if(!adj->http.checkVer()){
									// Выполняем сброс состояния HTTP парсера
									adj->http.clear();
									// Получаем бинарные данные REST запроса
									response = adj->http.reject(400, "Unsupported protocol version");
									// Завершаем работу
									break;
								}
								// Проверяем ключ клиента
								if(!adj->http.checkKey()){
									// Выполняем сброс состояния HTTP парсера
									adj->http.clear();
									// Получаем бинарные данные REST запроса
									response = adj->http.reject(400, "Wrong client key");
									// Завершаем работу
									break;
								}
								// Выполняем сброс состояния HTTP парсера
								adj->http.clear();
								// Получаем флаг шифрованных данных
								adj->crypt = adj->http.isCrypt();
								// Получаем поддерживаемый метод компрессии
								adj->compress = adj->http.getCompress();
								// Получаем бинарные данные REST запроса
								response = adj->http.response();

								cout << " =========== RESPONSE " << response.size() << endl;

								// Если бинарные данные запроса получены, отправляем на сервер
								if(!response.empty()) core->write(response.data(), response.size(), aid);
								// Завершаем работу
								return;
							// Сообщаем, что рукопожатие не выполнено
							} else {
								// Выполняем сброс состояния HTTP парсера
								adj->http.clear();
								// Формируем ответ, что страница не найдена
								response = adj->http.reject(404);
							}
						} break;
						// Если запрос неудачный
						case (uint8_t) http_t::stath_t::FAULT: {
							// Выполняем сброс состояния HTTP парсера
							adj->http.clear();
							// Формируем запрос авторизации
							response = adj->http.reject(401);
						} break;
					}
					// Если бинарные данные запроса получены, отправляем на сервер
					if(!response.empty()){
						// Устанавливаем размер стопбайт
						adj->stopBytes = response.size();
						// Отправляем ответ клиенту
						core->write(response.data(), response.size(), aid);
					// Завершаем работу
					} else core->close(aid);
				}
				// Завершаем работу
				return;
			// Если рукопожатие выполнено
			} else {
				// Смещение в буфере данных
				size_t offset = 0;
				// Создаём объект шапки фрейма
				frame_t::head_t head;
				// Выполняем перебор полученных данных
				while((size - offset) > 0){
					// Выполняем чтение фрейма WebSocket
					const auto & data = ws->frame.get(head, buffer + offset, size - offset);
					// Если буфер данных получен
					if(!data.empty()){
						// Проверяем состояние флагов RSV2 и RSV3
						if(head.rsv[1] || head.rsv[2]){
							// Создаём сообщение
							mess = mess_t(1002, "RSV2 and RSV3 must be clear");
							// Выполняем отключение клиента
							goto Stop;
						}
						// Если флаг компресси включён а данные пришли не сжатые
						if(head.rsv[0] && ((adj->compress == http_t::compress_t::NONE) ||
						(head.optcode == frame_t::opcode_t::CONTINUATION) ||
						(((uint8_t) head.optcode > 0x07) && ((uint8_t) head.optcode < 0x0b)))){
							// Создаём сообщение
							mess = mess_t(1002, "RSV1 must be clear");
							// Выполняем отключение клиента
							goto Stop;
						}
						// Если опкоды требуют финального фрейма
						if(!head.fin && ((uint8_t) head.optcode > 0x07) && ((uint8_t) head.optcode < 0x0b)){
							// Создаём сообщение
							mess = mess_t(1002, "FIN must be set");
							// Выполняем отключение клиента
							goto Stop;
						}
						// Определяем тип ответа
						switch((uint8_t) head.optcode){
							// Если ответом является PING
							case (uint8_t) frame_t::opcode_t::PING:
								// Отправляем ответ клиенту
								ws->pong(aid, core, string(data.begin(), data.end()));
							break;
							// Если ответом является PONG
							case (uint8_t) frame_t::opcode_t::PONG: {
								// Если идентификатор адъютанта совпадает
								if(stoull(string(data.begin(), data.end())) == aid)
									// Обновляем контрольную точку
									adj->checkPoint = ws->fmk->unixTimestamp();
							} break;
							// Если ответом является TEXT
							case (uint8_t) frame_t::opcode_t::TEXT:
							// Если ответом является BINARY
							case (uint8_t) frame_t::opcode_t::BINARY: {
								// Запоминаем полученный опкод
								adj->opcode = head.optcode;
								// Запоминаем, что данные пришли сжатыми
								adj->compressed = (head.rsv[0] && (adj->compress != http_t::compress_t::NONE));
								// Если список фрагментированных сообщений существует
								if(!adj->fragmes.empty()){
									// Создаём сообщение
									mess = mess_t(1002, "opcode for subsequent fragmented messages should not be set");
									// Выполняем отключение клиента
									goto Stop;
								// Если сообщение является не последнем
								} else if(!head.fin)
									// Заполняем фрагментированное сообщение
									adj->fragmes.insert(adj->fragmes.end(), data.begin(), data.end());
								// Если сообщение является последним
								else ws->extraction(adj, aid, core, data, (adj->opcode == frame_t::opcode_t::TEXT));
							} break;
							// Если ответом является CONTINUATION
							case (uint8_t) frame_t::opcode_t::CONTINUATION: {
								// Заполняем фрагментированное сообщение
								adj->fragmes.insert(adj->fragmes.end(), data.begin(), data.end());
								// Если сообщение является последним
								if(head.fin){
									// Выполняем извлечение данных
									ws->extraction(adj, aid, core, adj->fragmes, (adj->opcode == frame_t::opcode_t::TEXT));
									// Очищаем список фрагментированных сообщений
									adj->fragmes.clear();
								}
							} break;
							// Если ответом является CLOSE
							case (uint8_t) frame_t::opcode_t::CLOSE: {
								// Создаём сообщение
								mess = ws->frame.message(data);

								cout << " ++++++++++++++++++= " << mess.text << endl;

								// Завершаем работу
								core->close(aid);
							} break;
						}
						// Увеличиваем смещение в буфере
						offset += (head.payload + head.size);
					// Выходим из цикла, данных в буфере не достаточно
					} else break;
				}
				// Выходим из функции
				return;
			}
			// Устанавливаем метку остановки клиента
			Stop:
			// Получаем буфер сообщения
			const auto & buffer = ws->frame.message(mess);
			// Если данные сообщения получены
			if(!buffer.empty()){
				// Устанавливаем размер стопбайт
				adj->stopBytes = buffer.size();
				// Отправляем серверу сообщение
				core->write(buffer.data(), buffer.size(), aid);
			// Завершаем работу
			} else core->close(aid);
		}
	}
}
/**
 * extraction Метод извлечения полученных данных
 * @param adj    параметры подключения адъютанта
 * @param aid    идентификатор адъютанта
 * @param core   объект биндинга TCP/IP
 * @param buffer данные в чистом виде полученные с сервера
 * @param utf8   данные передаются в текстовом виде
 */
void awh::WebSocketServer::extraction(workSrvWss_t::adjp_t * adj, const size_t aid, core_t * core, const vector <char> & buffer, const bool utf8) const noexcept {
	// Если буфер данных передан
	// if(!buffer.empty() && (this->messageFn != nullptr)){
		// Если данные пришли в сжатом виде
		if(adj->compressed && (adj->compress != http_t::compress_t::NONE)){
			// Декомпрессионные данные
			vector <char> data;
			// Определяем метод компрессии
			switch((uint8_t) adj->compress){
				// Если метод компрессии выбран Deflate
				case (uint8_t) http_t::compress_t::DEFLATE: {
					// Устанавливаем размер скользящего окна
					this->hash.setWbit(adj->http.getWbitServer());
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
				/*
				// Если нужно производить дешифрование
				if(adj->crypt){
					// Выполняем шифрование переданных данных
					const auto & res = this->hash.decrypt(data.data(), data.size());
					// Отправляем полученный результат
					if(!res.empty()) this->messageFn(res, utf8, const_cast <WebSocketClient *> (this), this->ctx.at(3));
					// Иначе выводим сообщение так - как оно пришло
					else this->messageFn(data, utf8, const_cast <WebSocketClient *> (this), this->ctx.at(3));
				// Отправляем полученный результат
				} else this->messageFn(data, utf8, const_cast <WebSocketClient *> (this), this->ctx.at(3));
				*/

				cout << " ^^^^^^^^^MESSAGE^^^^^^^^^1 " << string(data.begin(), data.end()) << endl;

			// Выводим сообщение об ошибке
			} else {
				// Создаём сообщение
				mess_t mess(1007, "received data decompression error");
				// Получаем буфер сообщения
				const auto & buffer = this->frame.message(mess);
				// Если данные сообщения получены
				if(!buffer.empty()){
					// Устанавливаем размер стопбайт
					adj->stopBytes = buffer.size();
					// Отправляем серверу сообщение
					core->write(buffer.data(), buffer.size(), aid);
				// Завершаем работу
				} else core->close(aid);
			}
		// Если функция обратного вызова установлена, выводим полученное сообщение
		} else {
			/*
			// Если нужно производить дешифрование
			if(adj->crypt){
				// Выполняем шифрование переданных данных
				const auto & res = this->hash.decrypt(buffer.data(), buffer.size());
				// Отправляем полученный результат
				if(!res.empty()) this->messageFn(res, utf8, const_cast <WebSocketClient *> (this), this->ctx.at(3));
				// Иначе выводим сообщение так - как оно пришло
				else this->messageFn(buffer, utf8, const_cast <WebSocketClient *> (this), this->ctx.at(3));
			// Отправляем полученный результат
			} else this->messageFn(buffer, utf8, const_cast <WebSocketClient *> (this), this->ctx.at(3));
			*/

			cout << " ^^^^^^^^^MESSAGE^^^^^^^^^2 " << string(buffer.begin(), buffer.end()) << endl;
		}
	// }
}
/**
 * pong Метод ответа на проверку о доступности сервера
 * @param aid  идентификатор адъютанта
 * @param core объект биндинга TCP/IP
 * @param      message сообщение для отправки
 */
void awh::WebSocketServer::pong(const size_t aid, core_t * core, const string & message) noexcept {
	// Если необходимые данные переданы
	if((aid > 0) && (core != nullptr)){
		// Создаём буфер для отправки
		const auto & buffer = this->frame.pong(message);
		// Отправляем серверу сообщение
		core->write(buffer.data(), buffer.size(), aid);
	}
}
/**
 * ping Метод проверки доступности сервера
 * @param aid  идентификатор адъютанта
 * @param core объект биндинга TCP/IP
 * @param      message сообщение для отправки
 */
void awh::WebSocketServer::ping(const size_t aid, core_t * core, const string & message) noexcept {
	// Если необходимые данные переданы
	if((aid > 0) && (core != nullptr) && core->working()){
		// Создаём буфер для отправки
		const auto & buffer = this->frame.ping(message);
		// Отправляем серверу сообщение
		core->write(buffer.data(), buffer.size(), aid);
	}
}
/**
 * stop Метод остановки клиента
 */
void awh::WebSocketServer::stop() noexcept {
	// Если подключение выполнено
	if(this->core->working()){
		// Завершаем работу, если разрешено остановить
		const_cast <coreSrv_t *> (this->core)->stop();
		// Если функция обратного вызова установлена, выполняем
		// if(this->openStopFn != nullptr) this->openStopFn(false, this, this->ctx.at(0));
	}
}
/**
 * start Метод запуска клиента
 */
void awh::WebSocketServer::start() noexcept {
	// Если биндинг не запущен, выполняем запуск биндинга
	if(!this->core->working())
		// Выполняем запуск биндинга
		const_cast <coreSrv_t *> (this->core)->start();
}
/**
 * setSub Метод установки подпротокола поддерживаемого сервером
 * @param sub подпротокол для установки
 */
void awh::WebSocketServer::setSub(const string & sub) noexcept {

}
/**
 * setSubs Метод установки списка подпротоколов поддерживаемых сервером
 * @param subs подпротоколы для установки
 */
void awh::WebSocketServer::setSubs(const vector <string> & subs) noexcept {

}
/**
 * setWaitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read  количество секунд для детекции по чтению
 * @param write количество секунд для детекции по записи
 */
void awh::WebSocketServer::setWaitTimeDetect(const time_t read, const time_t write) noexcept {
	// Устанавливаем количество секунд на чтение
	this->worker.timeRead = read;
	// Устанавливаем количество секунд на запись
	this->worker.timeWrite = write;
}
/**
 * setBytesDetect Метод детекции сообщений по количеству байт
 * @param read  количество байт для детекции по чтению
 * @param write количество байт для детекции по записи
 */
void awh::WebSocketServer::setBytesDetect(const worker_t::mark_t read, const worker_t::mark_t write) noexcept {
	// Устанавливаем количество байт на чтение
	this->worker.markRead = read;
	// Устанавливаем количество байт на запись
	this->worker.markWrite = write;
}
/**
 * setMode Метод установки флага модуля
 * @param flag флаг модуля для установки
 */
void awh::WebSocketServer::setMode(const u_short flag) noexcept {
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	this->noinfo = (flag & (uint8_t) flag_t::NOINFO);
	// Устанавливаем флаг ожидания входящих сообщений
	this->worker.wait = (flag & (uint8_t) flag_t::WAITMESS);
	// Устанавливаем флаг отложенных вызовов событий сокета
	const_cast <coreSrv_t *> (this->core)->setDefer(flag & (uint8_t) flag_t::DEFER);
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	const_cast <coreSrv_t *> (this->core)->setNoInfo(flag & (uint8_t) flag_t::NOINFO);
}
/**
 * setChunkSize Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::WebSocketServer::setChunkSize(const size_t size) noexcept {

}
/**
 * setFrameSize Метод установки размеров сегментов фрейма
 * @param size минимальный размер сегмента
 */
void awh::WebSocketServer::setFrameSize(const size_t size) noexcept {

}
/**
 * setCompress Метод установки метода сжатия
 * @param метод сжатия сообщений
 */
void awh::WebSocketServer::setCompress(const http_t::compress_t compress) noexcept {

}
/**
 * setServ Метод установки данных сервиса
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::WebSocketServer::setServ(const string & id, const string & name, const string & ver) noexcept {

}
/**
 * setCrypt Метод установки параметров шифрования
 * @param pass пароль шифрования передаваемых данных
 * @param salt соль шифрования передаваемых данных
 * @param aes  размер шифрования передаваемых данных
 */
void awh::WebSocketServer::setCrypt(const string & pass, const string & salt, const hash_t::aes_t aes) noexcept {

}
/**
 * WebSocketServer Конструктор
 * @param core объект биндинга TCP/IP
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::WebSocketServer::WebSocketServer(const coreSrv_t * core, const fmk_t * fmk, const log_t * log) noexcept : hash(fmk, log), frame(fmk, log), core(core), fmk(fmk), log(log), worker(fmk, log) {
	// Устанавливаем контекст сообщения
	this->worker.ctx = this;
	// Устанавливаем событие на запуск системы
	this->worker.openFn = openCallback;
	// Устанавливаем функцию пинга клиента
	this->worker.pingFn = pingCallback;
	// Устанавливаем функцию чтения данных
	this->worker.readFn = readCallback;
	// Устанавливаем функцию записи данных
	this->worker.writeFn = writeCallback;
	// Добавляем событие аццепта клиента
	this->worker.acceptFn = acceptCallback;
	// Устанавливаем событие подключения
	this->worker.connectFn = connectCallback;
	// Устанавливаем событие отключения
	this->worker.disconnectFn = disconnectCallback;
	// Добавляем воркер в биндер TCP/IP
	const_cast <coreSrv_t *> (this->core)->add(&this->worker);
}
