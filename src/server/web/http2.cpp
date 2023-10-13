/**
 * @file: http2.cpp
 * @date: 2022-10-12
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
#include <server/web/http2.hpp>

/**
 * connectCallback Метод обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::Http2::connectCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {

}
/**
 * disconnectCallback Метод обратного вызова при отключении клиента
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::Http2::disconnectCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {

}
/**
 * readCallback Метод обратного вызова при чтении сообщения с клиента
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::server::Http2::readCallback(const char * buffer, const size_t size, const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {

}
/**
 * writeCallback Функция обратного вызова при записи сообщение адъютанту
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер записанных в сокет байт
 * @param aid    идентификатор адъютанта
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::server::Http2::writeCallback(const char * buffer, const size_t size, const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если подключение выполнено
	if(this->_core->working()){
		// Выполняем поиск агента которому соответствует клиент
		auto it = this->_agents.find(aid);
		// Если агент соответствует WebSocket-у
		if((it != this->_agents.end()) && (it->second == agent_t::WEBSOCKET))
			// Выполняем переброс вызова записи клиенту WebSocket
			this->_ws2.writeCallback(buffer, size, aid, sid, core);
	}
}
/**
 * persistCallback Функция персистентного вызова
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::Http2::persistCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {

}
/**
 * chunkSignal Метод обратного вызова при получении чанка HTTP/2
 * @param sid    идентификатор потока
 * @param aid    идентификатор адъютанта
 * @param buffer буфер данных который содержит полученный чанк
 * @param size   размер полученного буфера данных чанка
 * @return       статус полученных данных
 */
int awh::server::Http2::chunkSignal(const int32_t sid, const uint64_t aid, const uint8_t * buffer, const size_t size) noexcept {
	// Получаем параметры подключения адъютанта
	web_scheme_t::coffer_t * adj = const_cast <web_scheme_t::coffer_t *> (this->_scheme.get(aid));
	// Если параметры подключения адъютанта получены
	if(adj != nullptr){
		// Если функция обратного вызова на перехват входящих чанков установлена
		if(this->_callback.is("chunking"))
			// Выводим функцию обратного вызова
			this->_callback.call <const vector <char> &, const awh::http_t *> ("chunking", vector <char> (buffer, buffer + size), &adj->http);
		// Если функция перехвата полученных чанков не установлена
		else {
			// Если подключение закрыто
			if(adj->close){
				// Принудительно выполняем отключение лкиента
				const_cast <server::core_t *> (this->_core)->close(aid);
				// Выходим из функции
				return 0;
			}
			// Выполняем поиск агента которому соответствует клиент
			auto it = this->_agents.find(aid);
			// Если активный агент клиента установлен
			if(it != this->_agents.end()){
				// Определяем тип активного протокола
				switch(static_cast <uint8_t> (it->second)){
					// Если протокол соответствует HTTP-протоколу
					case static_cast <uint8_t> (agent_t::HTTP):
						// Добавляем полученный чанк в тело данных
						adj->http.body(vector <char> (buffer, buffer + size));
					break;
					// Если протокол соответствует протоколу WebSocket
					case static_cast <uint8_t> (agent_t::WEBSOCKET):
						// Выполняем передачу сигнала полученных чанков в модуль WebSocket
						this->_ws2.chunkSignal(sid, aid, buffer, size);
					// Выводим результат
					return 0;
				}
			// Добавляем полученный чанк в тело данных
			} else adj->http.body(vector <char> (buffer, buffer + size));
			// Если функция обратного вызова на вывода полученного чанка бинарных данных с сервера установлена
			if(this->_callback.is("chunks"))
				// Выводим функцию обратного вызова
				this->_callback.call <const int32_t, const uint64_t, const vector <char> &> ("chunks", sid, aid, vector <char> (buffer, buffer + size));
		}
	}
	// Выводим результат
	return 0;
}
/**
 * frameSignal Метод обратного вызова при получении фрейма заголовков HTTP/2
 * @param sid    идентификатор потока
 * @param aid    идентификатор адъютанта
 * @param direct направление передачи фрейма
 * @param type   тип полученного фрейма
 * @param flags  флаг полученного фрейма
 * @return       статус полученных данных
 */
int awh::server::Http2::frameSignal(const int32_t sid, const uint64_t aid, const nghttp2_t::direct_t direct, const uint8_t type, const uint8_t flags) noexcept {

}
/**
 * beginSignal Метод начала получения фрейма заголовков HTTP/2
 * @param sid идентификатор потока
 * @param aid идентификатор адъютанта
 * @return    статус полученных данных
 */
int awh::server::Http2::beginSignal(const int32_t sid, const uint64_t aid) noexcept {
	// Получаем параметры подключения адъютанта
	web_scheme_t::coffer_t * adj = const_cast <web_scheme_t::coffer_t *> (this->_scheme.get(aid));
	// Если параметры подключения адъютанта получены
	if(adj != nullptr){
		// Устанавливаем новый идентификатор потока
		adj->sid = sid;
		// Выполняем очистку параметров HTTP запроса
		adj->http.clear();
		// Очищаем буфер собранных данных
		adj->buffer.clear();
	}
	// Выводим результат
	return 0;
}
/**
 * closedSignal Метод завершения работы потока
 * @param sid   идентификатор потока
 * @param aid   идентификатор адъютанта
 * @param error флаг ошибки HTTP/2 если присутствует
 * @return      статус полученных данных
 */
int awh::server::Http2::closedSignal(const int32_t sid, const uint64_t aid, const uint32_t error) noexcept {
	// Определяем тип получаемой ошибки
	switch(error){
		// Если получена ошибка протокола
		case 0x1: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%zu] closed with error=%s", log_t::flag_t::CRITICAL, sid, aid, "PROTOCOL_ERROR");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_PROTOCOL, this->_fmk->format("Stream %d closed with error=%s", sid, "PROTOCOL_ERROR"));
		} break;
		// Если получена ошибка реализации
		case 0x2: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%zu] closed with error=%s", log_t::flag_t::CRITICAL, sid, aid, "INTERNAL_ERROR");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_INTERNAL, this->_fmk->format("Stream %d closed with error=%s", sid, "INTERNAL_ERROR"));
		} break;
		// Если получена ошибка превышения предела управления потоком
		case 0x3: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%zu] closed with error=%s", log_t::flag_t::CRITICAL, sid, aid, "FLOW_CONTROL_ERROR");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_FLOW_CONTROL, this->_fmk->format("Stream %d closed with error=%s", sid, "FLOW_CONTROL_ERROR"));
		} break;
		// Если установка не подтверждённа
		case 0x4: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%zu] closed with error=%s", log_t::flag_t::CRITICAL, sid, aid, "SETTINGS_TIMEOUT");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_SETTINGS_TIMEOUT, this->_fmk->format("Stream %d closed with error=%s", sid, "SETTINGS_TIMEOUT"));
		} break;
		// Если получен кадр для завершения потока
		case 0x5: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%zu] closed with error=%s", log_t::flag_t::CRITICAL, sid, aid, "STREAM_CLOSED");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_STREAM_CLOSED, this->_fmk->format("Stream %d closed with error=%s", sid, "STREAM_CLOSED"));
		} break;
		// Если размер кадра некорректен
		case 0x6: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%zu] closed with error=%s", log_t::flag_t::CRITICAL, sid, aid, "FRAME_SIZE_ERROR");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_FRAME_SIZE, this->_fmk->format("Stream %d closed with error=%s", sid, "FRAME_SIZE_ERROR"));
		} break;
		// Если поток не обработан
		case 0x7: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%zu] closed with error=%s", log_t::flag_t::CRITICAL, sid, aid, "REFUSED_STREAM");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_REFUSED_STREAM, this->_fmk->format("Stream %d closed with error=%s", sid, "REFUSED_STREAM"));
		} break;
		// Если поток аннулирован
		case 0x8: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%zu] closed with error=%s", log_t::flag_t::CRITICAL, sid, aid, "CANCEL");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_CANCEL, this->_fmk->format("Stream %d closed with error=%s", sid, "CANCEL"));
		} break;
		// Если состояние компрессии не обновлено
		case 0x9: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%zu] closed with error=%s", log_t::flag_t::CRITICAL, sid, aid, "COMPRESSION_ERROR");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_COMPRESSION, this->_fmk->format("Stream %d closed with error=%s", sid, "COMPRESSION_ERROR"));
		} break;
		// Если получена ошибка TCP-соединения для метода CONNECT
		case 0xA: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%zu] closed with error=%s", log_t::flag_t::CRITICAL, sid, aid, "CONNECT_ERROR");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_CONNECT, this->_fmk->format("Stream %d closed with error=%s", sid, "CONNECT_ERROR"));
		} break;
		// Если превышена емкость для обработки
		case 0xB: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%zu] closed with error=%s", log_t::flag_t::CRITICAL, sid, aid, "ENHANCE_YOUR_CALM");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_ENHANCE_YOUR_CALM, this->_fmk->format("Stream %d closed with error=%s", sid, "ENHANCE_YOUR_CALM"));
		} break;
		// Если согласованные параметры TLS не приемлемы
		case 0xC: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%zu] closed with error=%s", log_t::flag_t::CRITICAL, sid, aid, "INADEQUATE_SECURITY");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_INADEQUATE_SECURITY, this->_fmk->format("Stream %d closed with error=%s", sid, "INADEQUATE_SECURITY"));
		} break;
		// Если для запроса используется HTTP/1.1
		case 0xD: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%zu] closed with error=%s", log_t::flag_t::CRITICAL, sid, aid, "HTTP_1_1_REQUIRED");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_HTTP_1_1_REQUIRED, this->_fmk->format("Stream %d closed with error=%s", sid, "HTTP_1_1_REQUIRED"));
		} break;
	}
	// Если функция обратного вызова активности потока установлена
	if(this->_callback.is("stream"))
		// Выполняем функцию обратного вызова
		this->_callback.call <const int32_t, const uint64_t, const mode_t> ("stream", sid, aid, mode_t::CLOSE);
	// Если разрешено выполнить остановку
	if(error > 0x00){
		// Выполняем поиск адъютанта в списке активных сессий
		auto it = this->_sessions.find(aid);
		// Если активная сессия найдена
		if(it != this->_sessions.end()){
			// Если закрытие подключения не выполнено
			if(!it->second->close()){
				// Выполняем установку функции обратного вызова триггера, для закрытия соединения после завершения всех процессов
				it->second->on((function <void (void)>) std::bind(static_cast <void (server::core_t::*)(const uint64_t)> (&server::core_t::close), const_cast <server::core_t *> (this->_core), aid));
				// Выводим сообщение об ошибке
				return NGHTTP2_ERR_CALLBACK_FAILURE;
			// Выполняем установку функции обратного вызова триггера, для закрытия соединения после завершения всех процессов
			} else it->second->on((function <void (void)>) std::bind(static_cast <void (server::core_t::*)(const uint64_t)> (&server::core_t::close), const_cast <server::core_t *> (this->_core), aid));
		}
	}
	// Выводим результат
	return 0;
}
/**
 * headerSignal Метод обратного вызова при получении заголовка HTTP/2
 * @param sid идентификатор потока
 * @param aid идентификатор адъютанта
 * @param key данные ключа заголовка
 * @param val данные значения заголовка
 * @return    статус полученных данных
 */
int awh::server::Http2::headerSignal(const int32_t sid, const uint64_t aid, const string & key, const string & val) noexcept {
	// Получаем параметры подключения адъютанта
	web_scheme_t::coffer_t * adj = const_cast <web_scheme_t::coffer_t *> (this->_scheme.get(aid));
	// Если параметры подключения адъютанта получены
	if(adj != nullptr){
		// Устанавливаем полученные заголовки
		adj->http.header2(key, val);
		// Если функция обратного вызова на полученного заголовка с сервера установлена
		if(this->_callback.is("header"))
			// Выводим функцию обратного вызова
			this->_callback.call <const int32_t, const uint64_t, const string &, const string &> ("header", sid, aid, key, val);
	}
	// Выводим результат
	return 0;
}
/**
 * disconected Метод удаления отключившихся адъютантов
 * @param tid  идентификатор таймера
 * @param core объект сетевого ядра
 */
void awh::server::Http2::disconected(const u_short tid, awh::core_t * core) noexcept {

}
/**
 * init Метод инициализации WebSocket-сервера
 * @param socket   unix-сокет для биндинга
 * @param compress метод сжатия передаваемых сообщений
 */
void awh::server::Http2::init(const string & socket, const http_t::compress_t compress) noexcept {
	// Устанавливаем тип компрессии
	this->_scheme.compress = compress;
	// Выполняем инициализацию родительского объекта
	web2_t::init(socket, compress);
}
/**
 * init Метод инициализации WebSocket-сервера
 * @param port     порт сервера
 * @param host     хост сервера
 * @param compress метод сжатия передаваемых сообщений
 */
void awh::server::Http2::init(const u_int port, const string & host, const http_t::compress_t compress) noexcept {
	// Устанавливаем тип компрессии
	this->_scheme.compress = compress;
	// Выполняем инициализацию родительского объекта
	web2_t::init(port, host, compress);
}
/**
 * sendError Метод отправки сообщения об ошибке
 * @param aid  идентификатор адъютанта
 * @param mess отправляемое сообщение об ошибке
 */
void awh::server::Http2::sendError(const uint64_t aid, const ws::mess_t & mess) noexcept {
	// Если подключение выполнено
	if(this->_core->working()){
		// Выполняем поиск агента которому соответствует клиент
		auto it = this->_agents.find(aid);
		// Если агент соответствует WebSocket-у
		if((it != this->_agents.end()) && (it->second == agent_t::WEBSOCKET))
			// Выполняем отправку ошибки клиенту WebSocket
			this->_ws2.sendError(aid, mess);
	}
}
/**
 * send Метод отправки сообщения клиенту
 * @param aid     идентификатор адъютанта
 * @param message буфер сообщения в бинарном виде
 * @param size    размер сообщения в байтах
 * @param text    данные передаются в текстовом виде
 */
void awh::server::Http2::send(const uint64_t aid, const char * message, const size_t size, const bool text) noexcept {
	// Если подключение выполнено
	if(this->_core->working()){
		// Выполняем поиск агента которому соответствует клиент
		auto it = this->_agents.find(aid);
		// Если агент соответствует WebSocket-у
		if((it != this->_agents.end()) && (it->second == agent_t::WEBSOCKET))
			// Выполняем передачу данных клиенту WebSocket
			this->_ws2.send(aid, message, size, text);
	}
}
/**
 * send Метод отправки тела сообщения клиенту
 * @param id     идентификатор потока HTTP
 * @param aid    идентификатор адъютанта
 * @param buffer буфер бинарных данных передаваемых клиенту
 * @param size   размер сообщения в байтах
 * @param end    флаг последнего сообщения после которого поток закрывается
 * @return       результат отправки данных указанному клиенту
 */
bool awh::server::Http2::send(const int32_t id, const uint64_t aid, const char * buffer, const size_t size, const bool end) noexcept {

}
/**
 * send Метод отправки заголовков клиенту
 * @param id      идентификатор потока HTTP
 * @param aid     идентификатор адъютанта
 * @param code    код сообщения для адъютанта
 * @param mess    отправляемое сообщение об ошибке
 * @param headers заголовки отправляемые клиенту
 * @param end     размер сообщения в байтах
 * @return        идентификатор нового запроса
 */
int32_t awh::server::Http2::send(const int32_t id, const uint64_t aid, const u_int code, const string & mess, const unordered_multimap <string, string> & headers, const bool end) noexcept {

}
/**
 * send Метод отправки сообщения адъютанту
 * @param aid     идентификатор адъютанта
 * @param code    код сообщения для адъютанта
 * @param mess    отправляемое сообщение об ошибке
 * @param entity  данные полезной нагрузки (тело сообщения)
 * @param headers HTTP заголовки сообщения
 */
void awh::server::Http2::send(const uint64_t aid, const u_int code, const string & mess, const vector <char> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Если подключение выполнено
	if(this->_core->working()){
		// Выполняем поиск агента которому соответствует клиент
		auto it = this->_agents.find(aid);
		// Если агент соответствует HTTP-протоколу
		if((it == this->_agents.end()) || (it->second == agent_t::HTTP)){
			// Получаем параметры подключения адъютанта
			web_scheme_t::coffer_t * adj = const_cast <web_scheme_t::coffer_t *> (this->_scheme.get(aid));
			// Если параметры подключения адъютанта получены
			if(adj != nullptr){
				// Устанавливаем полезную нагрузку
				adj->http.body(entity);
				// Устанавливаем заголовки ответа
				adj->http.headers(headers);
				// Если сообщение ответа не установлено
				if(mess.empty())
					// Выполняем установку сообщения по умолчанию
					const_cast <string &> (mess) = adj->http.message(code);
				{
					// Формируем ответ на запрос клиента
					awh::web_t::res_t response(2.0f, code, mess);
					// Получаем заголовки ответа удалённому клиенту
					const auto & headers = adj->http.process2(http_t::process_t::RESPONSE, response);
					// Если бинарные данные ответа получены
					if(!headers.empty()){
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							{
								// Выводим заголовок ответа
								cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
								// Получаем объект работы с HTTP-запросами
								const http_t & http = reinterpret_cast <http_t &> (adj->http);
								// Получаем бинарные данные REST-ответа
								const auto & buffer = http.process(http_t::process_t::RESPONSE, response);
								// Если бинарные данные ответа получены
								if(!buffer.empty())
									// Выводим параметры ответа
									cout << string(buffer.begin(), buffer.end()) << endl << endl;
							}
						#endif
						// Если тело данных не установлено для отправки
						if(adj->http.body().empty())
							// Если подключение не установлено как постоянное, устанавливаем флаг завершения работы
							adj->stopped = (!this->_service.alive && !adj->alive);
						// Выполняем ответ подключившемуся клиенту
						int32_t sid = web2_t::send(adj->sid, aid, headers, adj->stopped);
						// Если запрос не получилось отправить, выходим из функции
						if(sid < 0) return;
						// Если тело запроса существует
						if(!adj->http.body().empty()){
							// Тело WEB запроса
							vector <char> entity;
							// Получаем данные тела запроса
							while(!(entity = adj->http.payload()).empty()){
								/**
								 * Если включён режим отладки
								 */
								#if defined(DEBUG_MODE)
									// Выводим сообщение о выводе чанка тела
									cout << this->_fmk->format("<chunk %u>", entity.size()) << endl << endl;
								#endif
								// Если тело данных не установлено для отправки
								if(adj->http.body().empty())
									// Если подключение не установлено как постоянное, устанавливаем флаг завершения работы
									adj->stopped = (!this->_service.alive && !adj->alive);
								// Выполняем отправку тела запроса на сервер
								if(!web2_t::send(adj->sid, aid, entity.data(), entity.size(), adj->http.body().empty()))
									// Выходим из функции
									return;
							}
						}
					}
				}
			}
		}
	}
}
/**
 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
 * @param callback функция обратного вызова
 */
void awh::server::Http2::on(function <void (const uint64_t, const mode_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
}
/**
 * on Метод установки функции обратного вызова для извлечения пароля
 * @param callback функция обратного вызова
 */
void awh::server::Http2::on(function <string (const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP-сервера
	this->_http1.on(callback);
}
/**
 * on Метод установки функции обратного вызова для обработки авторизации
 * @param callback функция обратного вызова
 */
void awh::server::Http2::on(function <bool (const string &, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP-сервера
	this->_http1.on(callback);
}
/**
 * on Метод установки функции обратного вызова для перехвата полученных чанков
 * @param callback функция обратного вызова
 */
void awh::server::Http2::on(function <void (const vector <char> &, const awh::http_t *)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP-сервера
	this->_http1.on(callback);
}
/**
 * on Метод установки функции обратного вызова получения событий запуска и остановки сетевого ядра
 * @param callback функция обратного вызова
 */
void awh::server::Http2::on(function <void (const awh::core_t::status_t, awh::core_t *)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие активации адъютанта на сервере
 * @param callback функция обратного вызова
 */
void awh::server::Http2::on(function <bool (const string &, const string &, const u_int)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP-сервера
	this->_http1.on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибок
 * @param callback функция обратного вызова
 */
void awh::server::Http2::on(function <void (const uint64_t, const u_int, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws2.on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений
 * @param callback функция обратного вызова
 */
void awh::server::Http2::on(function <void (const uint64_t, const vector <char> &, const bool)> callback) noexcept {
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws2.on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибки
 * @param callback функция обратного вызова
 */
void awh::server::Http2::on(function <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP-сервера
	this->_http1.on(callback);
}
/**
 * on Метод установки функция обратного вызова активности потока
 * @param callback функция обратного вызова
 */
void awh::server::Http2::on(function <void (const int32_t, const uint64_t, const mode_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP-сервера
	this->_http1.on(callback);
}
/**
 * on Метод установки функция обратного вызова при выполнении рукопожатия
 * @param callback функция обратного вызова
 */
void awh::server::Http2::on(function <void (const int32_t, const uint64_t, const agent_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP-сервера
	this->_http1.on(callback);
}
/**
 * on Метод установки функции обратного вызова при завершении запроса
 * @param callback функция обратного вызова
 */
void awh::server::Http2::on(function <void (const int32_t, const uint64_t, const direct_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP-сервера
	this->_http1.on(callback);
}
/**
 * on Метод установки функции вывода полученного чанка бинарных данных с клиента
 * @param callback функция обратного вызова
 */
void awh::server::Http2::on(function <void (const int32_t, const uint64_t, const vector <char> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP-сервера
	this->_http1.on(callback);
}
/**
 * on Метод установки функции вывода полученного заголовка с клиента
 * @param callback функция обратного вызова
 */
void awh::server::Http2::on(function <void (const int32_t, const uint64_t, const string &, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP-сервера
	this->_http1.on(callback);
}
/**
 * on Метод установки функции вывода запроса клиента к серверу
 * @param callback функция обратного вызова
 */
void awh::server::Http2::on(function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP-сервера
	this->_http1.on(callback);
}
/**
 * on Метод установки функции вывода полученного тела данных с клиента
 * @param callback функция обратного вызова
 */
void awh::server::Http2::on(function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP-сервера
	this->_http1.on(callback);
}
/**
 * on Метод установки функции вывода полученных заголовков с клиента
 * @param callback функция обратного вызова
 */
void awh::server::Http2::on(function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP-сервера
	this->_http1.on(callback);
}
/**
 * port Метод получения порта подключения адъютанта
 * @param aid идентификатор адъютанта
 * @return    порт подключения адъютанта
 */
u_int awh::server::Http2::port(const uint64_t aid) const noexcept {
	// Выводим результат
	return this->_scheme.getPort(aid);
}
/**
 * agent Метод извлечения агента клиента
 * @param aid идентификатор адъютанта
 * @return    агент к которому относится подключённый клиент
 */
awh::server::web_t::agent_t awh::server::Http2::agent(const uint64_t aid) const noexcept {
	// Выполняем поиск нужного нам агента
	auto it = this->_agents.find(aid);
	// Если агент клиента найден
	if(it != this->_agents.end())
		// Выводим идентификатор агента
		return it->second;
	// Выводим сообщение, что ничего не найдено
	return agent_t::HTTP;
}
/**
 * ip Метод получения IP-адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес интернет подключения адъютанта
 */
const string & awh::server::Http2::ip(const uint64_t aid) const noexcept {
	// Выводим результат
	return this->_scheme.getIp(aid);
}
/**
 * mac Метод получения MAC-адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес устройства адъютанта
 */
const string & awh::server::Http2::mac(const uint64_t aid) const noexcept {
	// Выводим результат
	return this->_scheme.getMac(aid);
}
/**
 * stop Метод остановки сервера
 */
void awh::server::Http2::stop() noexcept {
	// Выполняем остановку работы основного модуля
	web2_t::stop();
}
/**
 * start Метод запуска сервера
 */
void awh::server::Http2::start() noexcept {
	// Если биндинг не запущен
	if(!this->_core->working())
		// Выполняем запуск биндинга
		const_cast <server::core_t *> (this->_core)->start();
	// Если биндинг уже запущен, выполняем запуск
	else this->openCallback(this->_scheme.sid, dynamic_cast <awh::core_t *> (const_cast <server::core_t *> (this->_core)));
}
/**
 * close Метод закрытия подключения адъютанта
 * @param aid идентификатор адъютанта
 */
void awh::server::Http2::close(const uint64_t aid) noexcept {
	// Получаем параметры подключения адъютанта
	web_scheme_t::coffer_t * adj = const_cast <web_scheme_t::coffer_t *> (this->_scheme.get(aid));
	// Если параметры подключения адъютанта получены, устанавливаем флаг закрытия подключения
	if(adj != nullptr){
		// Выполняем поиск агента которому соответствует клиент
		auto it = this->_agents.find(aid);
		// Если активный агент клиента установлен
		if(it != this->_agents.end()){
			// Определяем тип активного протокола
			switch(static_cast <uint8_t> (it->second)){
				// Если протокол соответствует HTTP-протоколу
				case static_cast <uint8_t> (agent_t::HTTP): {
					// Определяем протокола подключения
					switch(static_cast <uint8_t> (adj->proto)){
						// Если протокол подключения соответствует HTTP/1.1
						case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1):
							// Выполняем закрытие подключения клиента HTTP/1.1
							this->_http1.close(aid);
						break;
						// Если протокол подключения соответствует HTTP/2
						case static_cast <uint8_t> (engine_t::proto_t::HTTP2): {
							// Устанавливаем флаг закрытия подключения адъютанта
							adj->close = true;
							// Выполняем поиск адъютанта в списке активных сессий
							auto it = this->_sessions.find(aid);
							// Если активная сессия найдена
							if(it != this->_sessions.end()){
								// Выполняем закрытие подключения
								it->second->close();
								// Выполняем установку функции обратного вызова триггера, для закрытия соединения после завершения всех процессов
								it->second->on((function <void (void)>) std::bind(static_cast <void (server::core_t::*)(const uint64_t)> (&server::core_t::close), const_cast <server::core_t *> (this->_core), aid));
							// Выполняем отключение адъютанта
							} else const_cast <server::core_t *> (this->_core)->close(aid);
						} break;
					}
				} break;
				// Если протокол соответствует протоколу WebSocket
				case static_cast <uint8_t> (agent_t::WEBSOCKET):
					// Выполняем закрытие подключения клиента WebSocket
					this->_ws2.close(aid);
				break;
			}
		// Иначе выполняем обработку входящих данных как Web-сервер
		} else {
			// Определяем протокола подключения
			switch(static_cast <uint8_t> (adj->proto)){
				// Если протокол подключения соответствует HTTP/1.1
				case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1):
					// Выполняем закрытие подключения клиента HTTP/1.1
					this->_http1.close(aid);
				break;
				// Если протокол подключения соответствует HTTP/2
				case static_cast <uint8_t> (engine_t::proto_t::HTTP2): {
					// Устанавливаем флаг закрытия подключения адъютанта
					adj->close = true;
					// Выполняем поиск адъютанта в списке активных сессий
					auto it = this->_sessions.find(aid);
					// Если активная сессия найдена
					if(it != this->_sessions.end()){
						// Выполняем закрытие подключения
						it->second->close();
						// Выполняем установку функции обратного вызова триггера, для закрытия соединения после завершения всех процессов
						it->second->on((function <void (void)>) std::bind(static_cast <void (server::core_t::*)(const uint64_t)> (&server::core_t::close), const_cast <server::core_t *> (this->_core), aid));
					// Выполняем отключение адъютанта
					} else const_cast <server::core_t *> (this->_core)->close(aid);
				} break;
			}
		}
	}
}
/**
 * subprotocol Метод установки поддерживаемого сабпротокола
 * @param subprotocol сабпротокол для установки
 */
void awh::server::Http2::subprotocol(const string & subprotocol) noexcept {
	// Выполняем установку списка поддерживаемого сабпротокола
	this->_ws2.subprotocol(subprotocol);
}
/**
 * subprotocols Метод установки списка поддерживаемых сабпротоколов
 * @param subprotocols сабпротоколы для установки
 */
void awh::server::Http2::subprotocols(const set <string> & subprotocols) noexcept {
	// Выполняем установку списка поддерживаемых сабпротоколов
	this->_ws2.subprotocols(subprotocols);
}
/**
 * subprotocol Метод получения списка выбранных сабпротоколов
 * @param aid идентификатор адъютанта
 * @return    список выбранных сабпротоколов
 */
const set <string> & awh::server::Http2::subprotocols(const uint64_t aid) const noexcept {
	// Результат работы функции
	static const set <string> result;
	// Получаем параметры подключения адъютанта
	web_scheme_t::coffer_t * adj = const_cast <web_scheme_t::coffer_t *> (this->_scheme.get(aid));
	// Если параметры подключения адъютанта получены
	if(adj != nullptr)
		// Выводим согласованный сабпротокол
		return this->_ws2.subprotocols(aid);
	// Выводим результат по умолчанию
	return result;
}
/**
 * extensions Метод установки списка расширений
 * @param extensions список поддерживаемых расширений
 */
void awh::server::Http2::extensions(const vector <vector <string>> & extensions) noexcept {
	// Выполняем установку списка поддерживаемых расширений
	this->_ws2.extensions(extensions);
}
/**
 * extensions Метод извлечения списка расширений
 * @param aid идентификатор адъютанта
 * @return    список поддерживаемых расширений
 */
const vector <vector <string>> & awh::server::Http2::extensions(const uint64_t aid) const noexcept {
	// Результат работы функции
	static const vector <vector <string>> result;
	// Получаем параметры подключения адъютанта
	web_scheme_t::coffer_t * adj = const_cast <web_scheme_t::coffer_t *> (this->_scheme.get(aid));
	// Если параметры подключения адъютанта получены
	if(adj != nullptr)
		// Выполняем извлечение списка поддерживаемых расширений WebSocket
		return this->_ws2.extensions(aid);
	// Выводим результат по умолчанию
	return result;
}
/**
 * multiThreads Метод активации многопоточности
 * @param count количество потоков для активации
 * @param mode  флаг активации/деактивации мультипоточности
 */
void awh::server::Http2::multiThreads(const uint16_t count, const bool mode) noexcept {
	// Если нужно активировать многопоточность
	if(mode){
		// Если многопоточность ещё не активированна
		if(!this->_ws2._thr.is())
			// Выполняем инициализацию пула потоков
			this->_ws2._thr.init(count);
		// Если многопоточность уже активированна
		else {
			// Выполняем завершение всех активных потоков
			this->_ws2._thr.wait();
			// Выполняем инициализацию нового тредпула
			this->_ws2._thr.init(count);
		}
		// Если сетевое ядро установлено
		if(this->_core != nullptr)
			// Устанавливаем простое чтение базы событий
			const_cast <server::core_t *> (this->_core)->easily(true);
	// Выполняем завершение всех потоков
	} else this->_ws2._thr.wait();
}
/**
 * total Метод установки максимального количества одновременных подключений
 * @param total максимальное количество одновременных подключений
 */
void awh::server::Http2::total(const u_short total) noexcept {
	// Устанавливаем максимальное количество одновременных подключений
	const_cast <server::core_t *> (this->_core)->total(this->_scheme.sid, total);
}
/**
 * segmentSize Метод установки размеров сегментов фрейма
 * @param size минимальный размер сегмента
 */
void awh::server::Http2::segmentSize(const size_t size) noexcept {
	// Если размер сегмента фрейма передан
	if(size > 0)
		// Устанавливаем размер одного сегмента фрейма
		this->_frameSize = size;
	// Выполняем установку размеров сегментов фрейма для WebSocket-сервера
	this->_ws2.segmentSize(size);
}
/**
 * clusterAutoRestart Метод установки флага перезапуска процессов
 * @param mode флаг перезапуска процессов
 */
void awh::server::Http2::clusterAutoRestart(const bool mode) noexcept {
	// Выполняем установку флага автоматического перезапуска
	const_cast <server::core_t *> (this->_core)->clusterAutoRestart(this->_scheme.sid, mode);
}
/**
 * compress Метод установки метода сжатия
 * @param метод сжатия сообщений
 */
void awh::server::Http2::compress(const http_t::compress_t compress) noexcept {
	// Устанавливаем метод компрессии
	this->_scheme.compress = compress;
}
/**
 * keepAlive Метод установки жизни подключения
 * @param cnt   максимальное количество попыток
 * @param idle  интервал времени в секундах через которое происходит проверка подключения
 * @param intvl интервал времени в секундах между попытками
 */
void awh::server::Http2::keepAlive(const int cnt, const int idle, const int intvl) noexcept {
	// Выполняем установку максимального количества попыток
	this->_scheme.keepAlive.cnt = cnt;
	// Выполняем установку интервала времени в секундах через которое происходит проверка подключения
	this->_scheme.keepAlive.idle = idle;
	// Выполняем установку интервала времени в секундах между попытками
	this->_scheme.keepAlive.intvl = intvl;
}
/**
 * mode Метод установки флагов настроек модуля
 * @param flags список флагов настроек модуля для установки
 */
void awh::server::Http2::mode(const set <flag_t> & flags) noexcept {
	// Устанавливаем флаги настроек модуля для WebSocket-сервера
	this->_ws2.mode(flags);
	// Устанавливаем флаги настроек модуля для HTTP-сервера
	this->_http1.mode(flags);
	// Устанавливаем флаг анбиндинга ядра сетевого модуля
	this->_unbind = (flags.count(flag_t::NOT_STOP) == 0);
	// Устанавливаем флаг поддержания автоматического подключения
	this->_scheme.alive = (flags.count(flag_t::ALIVE) > 0);
	// Устанавливаем флаг ожидания входящих сообщений
	this->_scheme.wait = (flags.count(flag_t::WAIT_MESS) > 0);
	// Устанавливаем флаг разрешающий выполнять подключение к протоколу WebSocket
	this->_webSocket = (flags.count(flag_t::WEBSOCKET_ENABLE) > 0);
	// Устанавливаем флаг перехвата контекста компрессии для клиента
	this->_client.takeover = (flags.count(flag_t::TAKEOVER_CLIENT) > 0);
	// Устанавливаем флаг перехвата контекста компрессии для сервера
	this->_server.takeover = (flags.count(flag_t::TAKEOVER_SERVER) > 0);
	// Если сетевое ядро установлено
	if(this->_core != nullptr){
		// Устанавливаем флаг запрещающий вывод информационных сообщений
		const_cast <server::core_t *> (this->_core)->noInfo(flags.count(flag_t::NOT_INFO) > 0);
		// Выполняем установку флага проверки домена
		const_cast <server::core_t *> (this->_core)->verifySSL(flags.count(flag_t::VERIFY_SSL) > 0);
	}
}
/**
 * alive Метод установки долгоживущего подключения
 * @param mode флаг долгоживущего подключения
 */
void awh::server::Http2::alive(const bool mode) noexcept {
	// Выполняем установку долгоживущего подключения
	web2_t::alive(mode);
	// Выполняем установку долгоживущего подключения для WebSocket-сервера
	this->_ws2.alive(mode);
	// Выполняем установку долгоживущего подключения для HTTP-сервера
	this->_http1.alive(mode);
}
/**
 * alive Метод установки времени жизни подключения
 * @param time время жизни подключения
 */
void awh::server::Http2::alive(const time_t time) noexcept {
	// Выполняем установку времени жизни подключения
	web2_t::alive(time);
	// Выполняем установку времени жизни подключения для WebSocket-сервера
	this->_ws2.alive(time);
	// Выполняем установку времени жизни подключения для HTTP-сервера
	this->_http1.alive(time);
}
/**
 * alive Метод установки долгоживущего подключения
 * @param aid  идентификатор адъютанта
 * @param mode флаг долгоживущего подключения
 */
void awh::server::Http2::alive(const uint64_t aid, const bool mode) noexcept {
	// Получаем параметры подключения адъютанта
	web_scheme_t::coffer_t * adj = const_cast <web_scheme_t::coffer_t *> (this->_scheme.get(aid));
	// Если параметры подключения адъютанта получены
	if(adj != nullptr)
		// Устанавливаем флаг пдолгоживущего подключения
		adj->alive = mode;
}
/**
 * core Метод установки сетевого ядра
 * @param core объект сетевого ядра
 */
void awh::server::Http2::core(const server::core_t * core) noexcept {
	// Если объект сетевого ядра передан
	if(core != nullptr){
		// Выполняем установку объекта сетевого ядра
		this->_core = core;
		// Активируем персистентный запуск для работы пингов
		const_cast <server::core_t *> (this->_core)->persistEnable(true);
		// Добавляем схемы сети в сетевое ядро
		const_cast <server::core_t *> (this->_core)->add(&this->_scheme);
		// Устанавливаем функцию активации ядра сервера
		const_cast <server::core_t *> (this->_core)->on(std::bind(&http2_t::eventsCallback, this, _1, _2));
		// Если многопоточность активированна
		if(this->_ws2._thr.is() || this->_ws2._ws1._thr.is())
			// Устанавливаем простое чтение базы событий
			const_cast <server::core_t *> (this->_core)->easily(true);
	// Если объект сетевого ядра не передан но ранее оно было добавлено
	} else if(this->_core != nullptr) {
		// Если многопоточность активированна
		if(this->_ws2._thr.is() || this->_ws2._ws1._thr.is()){
			// Выполняем завершение всех активных потоков
			this->_ws2._thr.wait();
			// Выполняем завершение всех активных потоков
			this->_ws2._ws1._thr.wait();
			// Снимаем режим простого чтения базы событий
			const_cast <server::core_t *> (this->_core)->easily(false);
		}
		// Деактивируем персистентный запуск для работы пингов
		const_cast <server::core_t *> (this->_core)->persistEnable(false);
		// Удаляем схему сети из сетевого ядра
		const_cast <server::core_t *> (this->_core)->remove(this->_scheme.sid);
		// Выполняем установку объекта сетевого ядра
		this->_core = core;
	}
}
/**
 * waitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read  количество секунд для детекции по чтению
 * @param write количество секунд для детекции по записи
 */
void awh::server::Http2::waitTimeDetect(const time_t read, const time_t write) noexcept {
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
void awh::server::Http2::bytesDetect(const scheme_t::mark_t read, const scheme_t::mark_t write) noexcept {
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
void awh::server::Http2::realm(const string & realm) noexcept {
	// Устанавливаем название сервера
	web2_t::realm(realm);
	// Устанавливаем название сервера для WebSocket-сервера
	this->_ws2.realm(realm);
	// Устанавливаем название сервера для HTTP-сервера
	this->_http1.realm(realm);
}
/**
 * opaque Метод установки временного ключа сессии сервера
 * @param opaque временный ключ сессии сервера
 */
void awh::server::Http2::opaque(const string & opaque) noexcept {
	// Устанавливаем временный ключ сессии сервера
	web2_t::opaque(opaque);
	// Устанавливаем временный ключ сессии сервера для WebSocket-сервера
	this->_ws2.opaque(opaque);
	// Устанавливаем временный ключ сессии сервера для HTTP-сервера
	this->_http1.opaque(opaque);
}
/**
 * chunk Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::server::Http2::chunk(const size_t size) noexcept {
	// Устанавливаем размера чанка
	web2_t::chunk(size);
	// Устанавливаем размера чанка для WebSocket-сервера
	this->_ws2.chunk(size);
	// Устанавливаем размера чанка для HTTP-сервера
	this->_http1.chunk(size);
}
/**
 * maxRequests Метод установки максимального количества запросов
 * @param max максимальное количество запросов
 */
void awh::server::Http2::maxRequests(const size_t max) noexcept {
	// Устанавливаем максимальное количество запросов
	web2_t::maxRequests(max);
	// Устанавливаем максимальное количество запросов для WebSocket-сервера
	this->_ws2.maxRequests(max);
	// Устанавливаем максимальное количество запросов для HTTP-сервера
	this->_http1.maxRequests(max);
}
/**
 * ident Метод установки идентификации сервера
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::server::Http2::ident(const string & id, const string & name, const string & ver) noexcept {
	// Устанавливаем идентификацию сервера
	web2_t::ident(id, name, ver);
	// Устанавливаем идентификацию сервера для WebSocket-сервера
	this->_ws2.ident(id, name, ver);
	// Устанавливаем идентификацию сервера для HTTP-сервера
	this->_http1.ident(id, name, ver);
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::server::Http2::authType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Устанавливаем тип авторизации
	web2_t::authType(type, hash);
	// Устанавливаем тип авторизации для WebSocket-сервера
	this->_ws2.authType(type, hash);
	// Устанавливаем тип авторизации для HTTP-сервера
	this->_http1.authType(type, hash);
}
/**
 * crypto Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::server::Http2::crypto(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Устанавливаем параметры шифрования
	web2_t::crypto(pass, salt, cipher);
	// Устанавливаем параметры шифрования для WebSocket-сервера
	this->_ws2.crypto(pass, salt, cipher);
	// Устанавливаем параметры шифрования для HTTP-сервера
	this->_http1.crypto(pass, salt, cipher);
}
/**
 * Http2 Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::server::Http2::Http2(const fmk_t * fmk, const log_t * log) noexcept : web2_t(fmk, log), _webSocket(false), _frameSize(0), _ws2(fmk, log), _http1(fmk, log), _scheme(fmk, log) {
	// Устанавливаем событие на запуск системы
	this->_scheme.callback.set <void (const uint16_t, awh::core_t *)> ("open", std::bind(&http2_t::openCallback, this, _1, _2));
	// Устанавливаем функцию персистентного вызова
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("persist", std::bind(&http2_t::persistCallback, this, _1, _2, _3));
	// Устанавливаем событие подключения
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("connect", std::bind(&http2_t::connectCallback, this, _1, _2, _3));
	// Устанавливаем событие отключения
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("disconnect", std::bind(&http2_t::disconnectCallback, this, _1, _2, _3));
	// Устанавливаем функцию чтения данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("read", std::bind(&http2_t::readCallback, this, _1, _2, _3, _4, _5));
	// Устанавливаем функцию записи данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("write", std::bind(&http2_t::writeCallback, this, _1, _2, _3, _4, _5));
	// Добавляем событие аццепта адъютанта
	this->_scheme.callback.set <bool (const string &, const string &, const u_int, const uint64_t, awh::core_t *)> ("accept", std::bind(&http2_t::acceptCallback, this, _1, _2, _3, _4, _5));
}
/**
 * Http2 Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::server::Http2::Http2(const server::core_t * core, const fmk_t * fmk, const log_t * log) noexcept : web2_t(core, fmk, log), _webSocket(false), _frameSize(0), _ws2(fmk, log), _http1(fmk, log), _scheme(fmk, log) {
	// Добавляем схему сети в сетевое ядро
	const_cast <server::core_t *> (this->_core)->add(&this->_scheme);
	// Устанавливаем событие на запуск системы
	this->_scheme.callback.set <void (const uint16_t, awh::core_t *)> ("open", std::bind(&http2_t::openCallback, this, _1, _2));
	// Устанавливаем функцию персистентного вызова
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("persist", std::bind(&http2_t::persistCallback, this, _1, _2, _3));
	// Устанавливаем событие подключения
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("connect", std::bind(&http2_t::connectCallback, this, _1, _2, _3));
	// Устанавливаем событие отключения
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("disconnect", std::bind(&http2_t::disconnectCallback, this, _1, _2, _3));
	// Устанавливаем функцию чтения данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("read", std::bind(&http2_t::readCallback, this, _1, _2, _3, _4, _5));
	// Устанавливаем функцию записи данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("write", std::bind(&http2_t::writeCallback, this, _1, _2, _3, _4, _5));
	// Добавляем событие аццепта адъютанта
	this->_scheme.callback.set <bool (const string &, const string &, const u_int, const uint64_t, awh::core_t *)> ("accept", std::bind(&http2_t::acceptCallback, this, _1, _2, _3, _4, _5));
}
/**
 * ~Http2 Деструктор
 */
awh::server::Http2::~Http2() noexcept {
	// Снимаем адрес сетевого ядра
	this->_ws2._core = nullptr;
}
