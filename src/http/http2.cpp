/**
 * @file: http2.cpp
 * @date: 2023-09-27
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

/**
 * Подключаем заголовочный файл
 */
#include <http/http2.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * debug Функция обратного вызова при получении отладочной информации
 * @param format формат вывода отладочной информации
 * @param args   список аргументов отладочной информации
 */
void awh::Http2::debug(const char * format, va_list args) noexcept {
	// Буфер данных для логирования
	string buffer(1024, '\0');
	// Выполняем перебор всех аргументов
	while(true){
		// Создаем список аргументов
		va_list args2;
		// Копируем список аргументов
		va_copy(args2, args);
		// Выполняем запись в буфер данных
		size_t res = vsnprintf(buffer.data(), buffer.size(), format, args2);
		// Если результат получен
		if((res >= 0) && (res < buffer.size())){
			// Завершаем список аргументов
			va_end(args);
			// Завершаем список локальных аргументов
			va_end(args2);
			// Если результат не получен
			if(res == 0)
				// Выполняем сброс результата
				buffer.clear();
			// Выводим результат
			else buffer.assign(buffer.begin(), buffer.begin() + res);
			// Выходим из цикла
			break;
		}
		// Размер буфера данных
		size_t size = 0;
		// Если данные не получены, увеличиваем буфер в два раза
		if(res < 0)
			// Увеличиваем размер буфера в два раза
			size = (buffer.size() * 2);
		// Увеличиваем размер буфера на один байт
		else size = (res + 1);
		// Очищаем буфер данных
		buffer.clear();
		// Выделяем память для буфера
		buffer.resize(size);
		// Завершаем список локальных аргументов
		va_end(args2);
	}
	// Выводим отладочную информацию
	std::cout << " \x1B[36m\x1B[1mDebug:\x1B[0m " << buffer << std::endl << std::flush;
}
/**
 * begin Функция обратного вызова активации получения фрейма заголовков
 * @param session объект сессии
 * @param frame   объект фрейма заголовков
 * @param ctx     передаваемый промежуточный контекст
 * @return        статус обработки полученных данных
 */
int32_t awh::Http2::begin([[maybe_unused]] nghttp2_session * session, const nghttp2_frame * frame, void * ctx) noexcept {
	// Получаем объект родительского объекта
	http2_t * self = reinterpret_cast <http2_t *> (ctx);
	// Если функция обратного вызова установлена
	if(self->_callback.is("begin")){
		// Выполняем определение типа фрейма
		switch(frame->hd.type){
			// Если мы получили входящие данные push-уведомления
			case NGHTTP2_PUSH_PROMISE: {
				// Если мы получили запрос клиента
				if(frame->headers.cat == NGHTTP2_HCAT_REQUEST){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим заголовок ответа
						std::cout << "\x1B[33m\x1B[1m^^^^^^^^^ PUSH ^^^^^^^^^\x1B[0m" << std::endl << std::flush;
						// Выводим информацию об ошибке
						std::cout << self->_fmk->format("Stream ID=%d", frame->hd.stream_id) << std::endl << std::endl << std::flush;
					#endif
					// Выполняем функцию обратного вызова
					return self->_callback.call <int32_t (const int32_t)> ("begin", frame->hd.stream_id);
				}
			} break;
			// Если мы получили входящие данные заголовков ответа
			case NGHTTP2_HEADERS: {
				// Получаем объект родительского объекта
				http2_t * self = reinterpret_cast <http2_t *> (ctx);
				// Определяем идентификатор сервиса
				switch(static_cast <uint8_t> (self->_mode)){
					// Если сервис идентифицирован как клиент
					case static_cast <uint8_t> (mode_t::CLIENT): {
						// Если мы получили ответ сервера
						if(frame->headers.cat == NGHTTP2_HCAT_RESPONSE){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим заголовок ответа
								std::cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << std::endl << std::flush;
								// Выводим информацию об ошибке
								std::cout << self->_fmk->format("Stream ID=%d", frame->hd.stream_id) << std::endl << std::endl << std::flush;
							#endif
							// Выполняем функцию обратного вызова
							return self->_callback.call <int32_t (const int32_t)> ("begin", frame->hd.stream_id);
						}
					} break;
					// Если сервис идентифицирован как сервер
					case static_cast <uint8_t> (mode_t::SERVER): {
						// Если мы получили запрос клиента
						if(frame->headers.cat == NGHTTP2_HCAT_REQUEST){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим заголовок ответа
								std::cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << std::endl << std::flush;
								// Выводим информацию об ошибке
								std::cout << self->_fmk->format("Stream ID=%d", frame->hd.stream_id) << std::endl << std::endl << std::flush;
							#endif
							// Выполняем функцию обратного вызова
							return self->_callback.call <int32_t (const int32_t)> ("begin", frame->hd.stream_id);
						}
					} break;
				}
			} break;
		}
	}
	// Выводим результат
	return 0;
}
/**
 * create Функция обратного вызова при создании фрейма
 * @param session объект сессии
 * @param hd      параметры фрейма
 * @param ctx     передаваемый промежуточный контекст
 * @return        статус обработки полученных данных
 */
int32_t awh::Http2::create([[maybe_unused]] nghttp2_session * session, const nghttp2_frame_hd * hd, void * ctx) noexcept {
	// Выполняем создание идентификатора фрейма по умолчанию
	frame_t type = frame_t::NONE;
	// Получаем объект родительского объекта
	http2_t * self = reinterpret_cast <http2_t *> (ctx);
	// Определяем тип фрейма
	switch(static_cast <uint8_t> (hd->type)){
		// Если мы получили фрейм данных
		case static_cast <uint8_t> (NGHTTP2_DATA):
			// Выполняем установку фрейма
			type = frame_t::DATA;
		break;
		// Если мы получили фрейм пингов
		case static_cast <uint8_t> (NGHTTP2_PING):
			// Выполняем установку фрейма
			type = frame_t::PING;
		break;
		// Если мы получили фрейм требования отключиться от сервера
		case static_cast <uint8_t> (NGHTTP2_GOAWAY):
			// Выполняем установку фрейма
			type = frame_t::GOAWAY;
		break;
		// Если мы получили фрейм передачи альтернативных желаемых протоколов
		case static_cast <uint8_t> (NGHTTP2_ALTSVC):
			// Выполняем установку фрейма
			type = frame_t::ALTSVC;
		break;
		// Если мы получили фрейм списка разрешённых ресурсов для подключения
		case static_cast <uint8_t> (NGHTTP2_ORIGIN):
			// Выполняем установку фрейма
			type = frame_t::ORIGIN;
		break;
		// Если мы получили фрейм заголовков
		case static_cast <uint8_t> (NGHTTP2_HEADERS): {
			// Выполняем установку фрейма
			type = frame_t::HEADERS;
			// Если сервис идентифицирован как сервер
			if(self->_mode == mode_t::SERVER){
				// Если поток является пользовательским
				if(hd->stream_id > 0)
					// Выполняем отправку списка источников клиенту
					self->sendOrigin();
				// Выполняем отправку список альтернативных сервисов клиенту
				self->sendAltSvc(hd->stream_id);
			}
		} break;
		// Если мы получили фрейм приоритетов
		case static_cast <uint8_t> (NGHTTP2_PRIORITY):
			// Выполняем установку фрейма
			type = frame_t::PRIORITY;
		break;
		// Если мы получили фрейм полученя настроек
		case static_cast <uint8_t> (NGHTTP2_SETTINGS): {
			// Выполняем установку фрейма
			type = frame_t::SETTINGS;
			// Если сервис идентифицирован как сервер
			if(self->_mode == mode_t::SERVER)
				// Выполняем отправку список альтернативных сервисов клиенту
				self->sendAltSvc(hd->stream_id);
		} break;
		// Если мы получили фрейм сброса подключения клиента
		case static_cast <uint8_t> (NGHTTP2_RST_STREAM):
			// Выполняем установку фрейма
			type = frame_t::RST_STREAM;
		break;
		// Если мы получили фрейм продолжения работы
		case static_cast <uint8_t> (NGHTTP2_CONTINUATION):
			// Выполняем установку фрейма
			type = frame_t::CONTINUATION;
		break;
		// Если мы получили фрейм отправки push-уведомления
		case static_cast <uint8_t> (NGHTTP2_PUSH_PROMISE):
			// Выполняем установку фрейма
			type = frame_t::PUSH_PROMISE;
		break;
		// Если мы получили фрейм обновления окна фрейма
		case static_cast <uint8_t> (NGHTTP2_WINDOW_UPDATE):
			// Выполняем установку фрейма
			type = frame_t::WINDOW_UPDATE;
		break;
		// Если мы получили фрейм обновления приоритетов
		case static_cast <uint8_t> (NGHTTP2_PRIORITY_UPDATE):
			// Выполняем установку фрейма
			type = frame_t::PRIORITY_UPDATE;
		break;
	}
	// Если функция обратного вызова установлена
	if(self->_callback.is("create"))
		// Выполняем функцию обратного вызова
		return self->_callback.call <int32_t (const int32_t, const frame_t)> ("create", hd->stream_id, type);
	// Выводим результат
	return 0;
}
/**
 * frameRecv Функция обратного вызова при получении фрейма
 * @param session объект сессии
 * @param frame   объект фрейма заголовков
 * @param ctx     передаваемый промежуточный контекст
 * @return        статус обработки полученных данных
 */
int32_t awh::Http2::frameRecv([[maybe_unused]] nghttp2_session * session, const nghttp2_frame * frame, void * ctx) noexcept {
	// Получаем объект родительского объекта
	http2_t * self = reinterpret_cast <http2_t *> (ctx);
	// Если функция обратного вызова установлена
	if(self->_callback.is("frame")){
		// Выполняем создание флага по умолчанию
		std::set <flag_t> flags;
		// Выполняем создание идентификатора фрейма по умолчанию
		frame_t type = frame_t::NONE;
		// Идентификатор активного потока
		const int32_t sid = frame->hd.stream_id;
		// Если мы получили флаг PADDED
		if(frame->hd.flags & NGHTTP2_FLAG_PADDED)
			// Выполняем установку флага
			flags.emplace(flag_t::PADDED);
		// Если мы получили флаг установки приоритетов
		if(frame->hd.flags & NGHTTP2_FLAG_PRIORITY)
			// Выполняем установку флага
			flags.emplace(flag_t::PRIORITY);
		// Если мы получили флаг завершения передачи потока
		if(frame->hd.flags & NGHTTP2_FLAG_END_STREAM)
			// Выполняем установку флага
			flags.emplace(flag_t::END_STREAM);
		// Если мы получили флаг завершения передачи заголовков
		if(frame->hd.flags & NGHTTP2_FLAG_END_HEADERS)
			// Выполняем установку флага
			flags.emplace(flag_t::END_HEADERS);
		// Определяем тип фрейма
		switch(static_cast <uint8_t> (frame->hd.type)){
			// Если мы получили фрейм данных
			case static_cast <uint8_t> (NGHTTP2_DATA):
				// Выполняем установку фрейма
				type = frame_t::DATA;
			break;
			// Если мы получили фрейм пингов
			case static_cast <uint8_t> (NGHTTP2_PING):
				// Выполняем установку фрейма
				type = frame_t::PING;
			break;
			// Если мы получили фрейм требования отключиться от сервера
			case static_cast <uint8_t> (NGHTTP2_GOAWAY):
				// Выполняем установку фрейма
				type = frame_t::GOAWAY;
			break;
			// Если мы получили фрейм передачи альтернативных желаемых сервисов
			case static_cast <uint8_t> (NGHTTP2_ALTSVC): {
				// Выполняем установку фрейма
				type = frame_t::ALTSVC;
				// Извлекаем данныеальтернативных сервисов
				nghttp2_ext_altsvc * altsvc = reinterpret_cast <nghttp2_ext_altsvc *> (frame->ext.payload);
				// Если функция обратного вызова установлена
				if(self->_callback.is("altsvc"))
					// Выполняем функцию обратного вызова
					self->_callback.call <void (const string &, const string &)> ("altsvc", string(reinterpret_cast <const char *> (altsvc->origin), altsvc->origin_len), string(reinterpret_cast <const char *> (altsvc->field_value), altsvc->field_value_len));
			} break;
			// Если мы получили фрейм списка разрешённых ресурсов для подключения
			case static_cast <uint8_t> (NGHTTP2_ORIGIN): {
				// Выполняем установку фрейма
				type = frame_t::ORIGIN;
				// Выполняем получение массив разрешённых ресурсов для подключения
				nghttp2_ext_origin * ov = reinterpret_cast <nghttp2_ext_origin *> (frame->ext.payload);
				// Если функция обратного вызова установлена
				if(self->_callback.is("origin")){
					// Создаём список полученных ресурсов
					vector <string> origins;
					// Выполняем перебор всех элементов полученного массива
					for(size_t i = 0; i < ov->nov; i++)
						// Выполняем заполнение списка полученных ресурсов
						origins.push_back(string(reinterpret_cast <const char *> (ov->ov[i].origin), ov->ov[i].origin_len));
					// Выполняем функцию обратного вызова
					self->_callback.call <void (const vector <string> &)> ("origin", origins);
				}
			} break;
			// Если мы получили фрейм заголовков
			case static_cast <uint8_t> (NGHTTP2_HEADERS):
				// Выполняем установку фрейма
				type = frame_t::HEADERS;
			break;
			// Если мы получили фрейм приоритетов
			case static_cast <uint8_t> (NGHTTP2_PRIORITY):
				// Выполняем установку фрейма
				type = frame_t::PRIORITY;
			break;
			// Если мы получили фрейм полученя настроек
			case static_cast <uint8_t> (NGHTTP2_SETTINGS):
				// Выполняем установку фрейма
				type = frame_t::SETTINGS;
			break;
			// Если мы получили фрейм сброса подключения клиента
			case static_cast <uint8_t> (NGHTTP2_RST_STREAM):
				// Выполняем установку фрейма
				type = frame_t::RST_STREAM;
			break;
			// Если мы получили фрейм продолжения работы
			case static_cast <uint8_t> (NGHTTP2_CONTINUATION):
				// Выполняем установку фрейма
				type = frame_t::CONTINUATION;
			break;
			// Если мы получили фрейм отправки push-уведомления
			case static_cast <uint8_t> (NGHTTP2_PUSH_PROMISE):
				// Выполняем установку фрейма
				type = frame_t::PUSH_PROMISE;
			break;
			// Если мы получили фрейм обновления окна фрейма
			case static_cast <uint8_t> (NGHTTP2_WINDOW_UPDATE): {
				// Выполняем установку фрейма
				type = frame_t::WINDOW_UPDATE;
				// Если список полезной нагрузки заполнен
				if(!self->_payloads.empty()){
					// Если получен идентификатор нулевого потока
					if(sid == 0){
						// Выполняем перебор всего списка записей
						for(auto & record : self->_records){
							// Выполняем перебор всего списка записей для которых есть неотправленные данные
							while(!record.second.empty()){
								// Если на принимаемой стороне достаточно памяти для получения данных
								if(self->available(record.first) >= record.second.front().first)
									// Выполняем отправку данных полезной нагрузки для указанного потока
									self->submit(record.first, record.second.front().second);
								// Если данных не достаточно, выходим
								else break;
							}
						}
					// Если получен идентификатор потока
					} else {
						// Выполняем поиск записей для потока
						auto i = self->_records.find(sid);
						// Если список неотправленных записей для потока существуют
						if((i != self->_records.end()) && !i->second.empty()){
							// Выполняем перебор всего списка записей которые ещё не отправленны
							while(!i->second.empty()){
								// Если на принимаемой стороне достаточно памяти для получения данных
								if(self->available(i->first) >= i->second.front().first)
									// Выполняем отправку записи для указанного потока
									self->submit(i->first, i->second.front().second);
								// Если данных не достаточно, выходим
								else break;
							}
						}
					}
				}
			} break;
			// Если мы получили фрейм обновления приоритетов
			case static_cast <uint8_t> (NGHTTP2_PRIORITY_UPDATE):
				// Выполняем установку фрейма
				type = frame_t::PRIORITY_UPDATE;
			break;
		}
		// Выполняем функцию обратного вызова
		return self->_callback.call <int32_t (const int32_t, const direct_t, const frame_t, const std::set <flag_t> &)> ("frame", sid, direct_t::RECV, type, flags);
	}
	// Выводим результат
	return 0;
}
/**
 * frameSend Функция обратного вызова при отправки фрейма
 * @param session объект сессии
 * @param frame   объект фрейма заголовков
 * @param ctx     передаваемый промежуточный контекст
 * @return        статус обработки полученных данных
 */
int32_t awh::Http2::frameSend([[maybe_unused]] nghttp2_session * session, const nghttp2_frame * frame, void * ctx) noexcept {
	// Получаем объект родительского объекта
	http2_t * self = reinterpret_cast <http2_t *> (ctx);
	// Если функция обратного вызова установлена
	if(self->_callback.is("frame")){
		// Выполняем создание флага по умолчанию
		std::set <flag_t> flags;
		// Выполняем создание идентификатора фрейма по умолчанию
		frame_t type = frame_t::NONE;
		// Если мы получили флаг PADDED
		if(frame->hd.flags & NGHTTP2_FLAG_PADDED)
			// Выполняем установку флага
			flags.emplace(flag_t::PADDED);
		// Если мы получили флаг установки приоритетов
		if(frame->hd.flags & NGHTTP2_FLAG_PRIORITY)
			// Выполняем установку флага
			flags.emplace(flag_t::PRIORITY);
		// Если мы получили флаг завершения передачи потока
		if(frame->hd.flags & NGHTTP2_FLAG_END_STREAM)
			// Выполняем установку флага
			flags.emplace(flag_t::END_STREAM);
		// Если мы получили флаг завершения передачи заголовков
		if(frame->hd.flags & NGHTTP2_FLAG_END_HEADERS)
			// Выполняем установку флага
			flags.emplace(flag_t::END_HEADERS);
		// Определяем тип фрейма
		switch(static_cast <uint8_t> (frame->hd.type)){
			// Если мы получили фрейм данных
			case static_cast <uint8_t> (NGHTTP2_DATA):
				// Выполняем установку фрейма
				type = frame_t::DATA;
			break;
			// Если мы получили фрейм пингов
			case static_cast <uint8_t> (NGHTTP2_PING):
				// Выполняем установку фрейма
				type = frame_t::PING;
			break;
			// Если мы получили фрейм требования отключиться от сервера
			case static_cast <uint8_t> (NGHTTP2_GOAWAY):
				// Выполняем установку фрейма
				type = frame_t::GOAWAY;
			break;
			// Если мы получили фрейм передачи альтернативных желаемых протоколов
			case static_cast <uint8_t> (NGHTTP2_ALTSVC):
				// Выполняем установку фрейма
				type = frame_t::ALTSVC;
			break;
			// Если мы получили фрейм списка разрешённых ресурсов для подключения
			case static_cast <uint8_t> (NGHTTP2_ORIGIN):
				// Выполняем установку фрейма
				type = frame_t::ORIGIN;
			break;
			// Если мы получили фрейм заголовков
			case static_cast <uint8_t> (NGHTTP2_HEADERS):
				// Выполняем установку фрейма
				type = frame_t::HEADERS;
			break;
			// Если мы получили фрейм приоритетов
			case static_cast <uint8_t> (NGHTTP2_PRIORITY):
				// Выполняем установку фрейма
				type = frame_t::PRIORITY;
			break;
			// Если мы получили фрейм полученя настроек
			case static_cast <uint8_t> (NGHTTP2_SETTINGS):
				// Выполняем установку фрейма
				type = frame_t::SETTINGS;
			break;
			// Если мы получили фрейм сброса подключения клиента
			case static_cast <uint8_t> (NGHTTP2_RST_STREAM):
				// Выполняем установку фрейма
				type = frame_t::RST_STREAM;
			break;
			// Если мы получили фрейм продолжения работы
			case static_cast <uint8_t> (NGHTTP2_CONTINUATION):
				// Выполняем установку фрейма
				type = frame_t::CONTINUATION;
			break;
			// Если мы получили фрейм отправки push-уведомления
			case static_cast <uint8_t> (NGHTTP2_PUSH_PROMISE):
				// Выполняем установку фрейма
				type = frame_t::PUSH_PROMISE;
			break;
			// Если мы получили фрейм обновления окна фрейма
			case static_cast <uint8_t> (NGHTTP2_WINDOW_UPDATE):
				// Выполняем установку фрейма
				type = frame_t::WINDOW_UPDATE;
			break;
			// Если мы получили фрейм обновления приоритетов
			case static_cast <uint8_t> (NGHTTP2_PRIORITY_UPDATE):
				// Выполняем установку фрейма
				type = frame_t::PRIORITY_UPDATE;
			break;
		}
		// Выполняем функцию обратного вызова
		return self->_callback.call <int32_t (const int32_t, const direct_t, const frame_t, const std::set <flag_t> &)> ("frame", frame->hd.stream_id, direct_t::SEND, type, flags);
	}
	// Выводим результат
	return 0;
}
/**
 * close Функция закрытия подключения
 * @param session объект сессии
 * @param sid     идентификатор потока
 * @param error   флаг ошибки если присутствует
 * @param ctx     передаваемый промежуточный контекст
 * @return        статус обработки полученных данных
 */
int32_t awh::Http2::close(nghttp2_session * session, const int32_t sid, const uint32_t error, void * ctx) noexcept {
	// Получаем объект родительского объекта
	http2_t * self = reinterpret_cast <http2_t *> (ctx);
	/**
	 * Если включён режим отладки
	 */
	#if DEBUG_MODE
		// Выводим заголовок ответа
		std::cout << "\x1B[33m\x1B[1m^^^^^^^^^ CLOSE STREAM HTTP2 ^^^^^^^^^\x1B[0m" << std::endl << std::flush;
		// Если ошибка не была получена
		if(error == 0x0)
			// Выводим информацию о закрытии сессии
			std::cout << self->_fmk->format("Stream %d closed", sid) << std::endl << std::endl << std::flush;
	#endif
	// Определяем код ошибки
	error_t code = error_t::NONE;
	// Определяем тип получаемой ошибки
	switch(error){
		// Если получена ошибка протокола
		case NGHTTP2_PROTOCOL_ERROR: {
			// Устанавливаем код ошибки
			code = error_t::PROTOCOL_ERROR;
			// Выводим информацию о закрытии сессии с ошибкой
			self->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, sid, "PROTOCOL_ERROR");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(self->_callback.is("error"))
				// Выполняем функцию обратного вызова
				self->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_PROTOCOL, self->_fmk->format("Stream %d closed with error=%s", sid, "PROTOCOL_ERROR"));
		} break;
		// Если получена ошибка реализации
		case NGHTTP2_INTERNAL_ERROR: {
			// Устанавливаем код ошибки
			code = error_t::INTERNAL_ERROR;
			// Выводим информацию о закрытии сессии с ошибкой
			self->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, sid, "INTERNAL_ERROR");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(self->_callback.is("error"))
				// Выполняем функцию обратного вызова
				self->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_INTERNAL, self->_fmk->format("Stream %d closed with error=%s", sid, "INTERNAL_ERROR"));
		} break;
		// Если получена ошибка превышения предела управления потоком
		case NGHTTP2_FLOW_CONTROL_ERROR: {
			// Устанавливаем код ошибки
			code = error_t::FLOW_CONTROL_ERROR;
			// Выводим информацию о закрытии сессии с ошибкой
			self->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, sid, "FLOW_CONTROL_ERROR");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(self->_callback.is("error"))
				// Выполняем функцию обратного вызова
				self->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_FLOW_CONTROL, self->_fmk->format("Stream %d closed with error=%s", sid, "FLOW_CONTROL_ERROR"));
		} break;
		// Если установка параметров завершилась по таймауту
		case NGHTTP2_SETTINGS_TIMEOUT: {
			// Устанавливаем код ошибки
			code = error_t::SETTINGS_TIMEOUT;
			// Выводим информацию о закрытии сессии с ошибкой
			self->_log->print("Stream %d closed with error=%s", log_t::flag_t::WARNING, sid, "SETTINGS_TIMEOUT");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(self->_callback.is("error"))
				// Выполняем функцию обратного вызова
				self->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::WARNING, http::error_t::HTTP2_SETTINGS_TIMEOUT, self->_fmk->format("Stream %d closed with error=%s", sid, "SETTINGS_TIMEOUT"));
		} break;
		// Если получен кадр для завершения потока
		case NGHTTP2_STREAM_CLOSED: {
			// Устанавливаем код ошибки
			code = error_t::STREAM_CLOSED;
			// Выводим информацию о закрытии сессии с ошибкой
			self->_log->print("Stream %d closed with error=%s", log_t::flag_t::WARNING, sid, "STREAM_CLOSED");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(self->_callback.is("error"))
				// Выполняем функцию обратного вызова
				self->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::WARNING, http::error_t::HTTP2_STREAM_CLOSED, self->_fmk->format("Stream %d closed with error=%s", sid, "STREAM_CLOSED"));
		} break;
		// Если размер кадра некорректен
		case NGHTTP2_FRAME_SIZE_ERROR: {
			// Устанавливаем код ошибки
			code = error_t::FRAME_SIZE_ERROR;
			// Выводим информацию о закрытии сессии с ошибкой
			self->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, sid, "FRAME_SIZE_ERROR");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(self->_callback.is("error"))
				// Выполняем функцию обратного вызова
				self->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_FRAME_SIZE, self->_fmk->format("Stream %d closed with error=%s", sid, "FRAME_SIZE_ERROR"));
		} break;
		// Если поток не обработан
		case NGHTTP2_REFUSED_STREAM: {
			// Устанавливаем код ошибки
			code = error_t::REFUSED_STREAM;
			// Выводим информацию о закрытии сессии с ошибкой
			self->_log->print("Stream %d closed with error=%s", log_t::flag_t::WARNING, sid, "REFUSED_STREAM");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(self->_callback.is("error"))
				// Выполняем функцию обратного вызова
				self->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::WARNING, http::error_t::HTTP2_REFUSED_STREAM, self->_fmk->format("Stream %d closed with error=%s", sid, "REFUSED_STREAM"));
		} break;
		// Если поток аннулирован
		case NGHTTP2_CANCEL: {
			// Устанавливаем код ошибки
			code = error_t::CANCEL;
			// Выводим информацию о закрытии сессии с ошибкой
			self->_log->print("Stream %d closed with error=%s", log_t::flag_t::WARNING, sid, "CANCEL");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(self->_callback.is("error"))
				// Выполняем функцию обратного вызова
				self->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::WARNING, http::error_t::HTTP2_CANCEL, self->_fmk->format("Stream %d closed with error=%s", sid, "CANCEL"));
		} break;
		// Если состояние компрессии не обновлено
		case NGHTTP2_COMPRESSION_ERROR: {
			// Устанавливаем код ошибки
			code = error_t::COMPRESSION_ERROR;
			// Выводим информацию о закрытии сессии с ошибкой
			self->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, sid, "COMPRESSION_ERROR");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(self->_callback.is("error"))
				// Выполняем функцию обратного вызова
				self->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_COMPRESSION, self->_fmk->format("Stream %d closed with error=%s", sid, "COMPRESSION_ERROR"));
		} break;
		// Если получена ошибка TCP-соединения для метода CONNECT
		case NGHTTP2_CONNECT_ERROR: {
			// Устанавливаем код ошибки
			code = error_t::CONNECT_ERROR;
			// Выводим информацию о закрытии сессии с ошибкой
			self->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, sid, "CONNECT_ERROR");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(self->_callback.is("error"))
				// Выполняем функцию обратного вызова
				self->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_CONNECT, self->_fmk->format("Stream %d closed with error=%s", sid, "CONNECT_ERROR"));
		} break;
		// Если превышена емкость для обработки
		case NGHTTP2_ENHANCE_YOUR_CALM: {
			// Устанавливаем код ошибки
			code = error_t::ENHANCE_YOUR_CALM;
			// Выводим информацию о закрытии сессии с ошибкой
			self->_log->print("Stream %d closed with error=%s", log_t::flag_t::WARNING, sid, "ENHANCE_YOUR_CALM");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(self->_callback.is("error"))
				// Выполняем функцию обратного вызова
				self->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::WARNING, http::error_t::HTTP2_ENHANCE_YOUR_CALM, self->_fmk->format("Stream %d closed with error=%s", sid, "ENHANCE_YOUR_CALM"));
		} break;
		// Если согласованные параметры SSL не приемлемы
		case NGHTTP2_INADEQUATE_SECURITY: {
			// Устанавливаем код ошибки
			code = error_t::INADEQUATE_SECURITY;
			// Выводим информацию о закрытии сессии с ошибкой
			self->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, sid, "INADEQUATE_SECURITY");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(self->_callback.is("error"))
				// Выполняем функцию обратного вызова
				self->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_INADEQUATE_SECURITY, self->_fmk->format("Stream %d closed with error=%s", sid, "INADEQUATE_SECURITY"));
		} break;
		// Если для запроса используется HTTP/1.1
		case NGHTTP2_HTTP_1_1_REQUIRED: {
			// Устанавливаем код ошибки
			code = error_t::HTTP_1_1_REQUIRED;
			// Выводим информацию о закрытии сессии с ошибкой
			self->_log->print("Stream %d closed with error=%s", log_t::flag_t::WARNING, sid, "HTTP_1_1_REQUIRED");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(self->_callback.is("error"))
				// Выполняем функцию обратного вызова
				self->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::WARNING, http::error_t::HTTP2_HTTP_1_1_REQUIRED, self->_fmk->format("Stream %d closed with error=%s", sid, "HTTP_1_1_REQUIRED"));
		} break;
	}
	// Если функция обратного вызова установлена
	if(self->_callback.is("close"))
		// Выполняем функцию обратного вызова
		return self->_callback.call <int32_t (const int32_t, const error_t)> ("close", sid, code);
	// Выводим значение по умолчанию
	return 0;
}
/**
 * error Функция обратного вызова при получении ошибок
 * @param session объект сессии
 * @param code    код полученной ошибки
 * @param msg     сообщение ошибки
 * @param size    размер текста ошибки
 * @param ctx     передаваемый промежуточный контекст
 * @return        статус обработки полученных данных
 */
int32_t awh::Http2::error(nghttp2_session * session, const int32_t code, const char * msg, const size_t size, void * ctx) noexcept {
	// Получаем объект родительского объекта
	http2_t * self = reinterpret_cast <http2_t *> (ctx);
	// Выводим информацию о закрытии сессии с ошибкой
	self->_log->print("%s [%d]", log_t::flag_t::CRITICAL, string(msg, size).c_str(), code);
	// Если функция обратного вызова на на вывод ошибок установлена
	if(self->_callback.is("error"))
		// Выполняем функцию обратного вызова
		self->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, self->_fmk->format("%s [%d]", string(msg, size).c_str(), code));
	// Выводим результат
	return 0;
}
/**
 * chunk Функция обратного вызова при получении чанка
 * @param session объект сессии
 * @param flags   флаги события для сессии
 * @param sid     идентификатор потока
 * @param buffer  буфер данных который содержит полученный чанк
 * @param size    размер полученного буфера данных чанка
 * @param ctx     передаваемый промежуточный контекст
 * @return        статус обработки полученных данных
 */
int32_t awh::Http2::chunk([[maybe_unused]] nghttp2_session * session, [[maybe_unused]] const uint8_t flags, const int32_t sid, const uint8_t * buffer, const size_t size, void * ctx) noexcept {
	// Результат работы функции
	int32_t result = 0;
	// Получаем объект родительского объекта
	http2_t * self = reinterpret_cast <http2_t *> (ctx);
	// Если функция обратного вызова установлена
	if(self->_callback.is("chunk"))
		// Выполняем функцию обратного вызова
		result = self->_callback.call <int32_t (const int32_t, const uint8_t *, const size_t)> ("chunk", sid, buffer, size);
	// Если блок данных обработан удачно
	if(result == 0){
		// Если сессия инициализированна
		if(self->_session != nullptr){
			// Фиксируем полученный результат
			const int32_t rv = nghttp2_session_consume(self->_session, sid, size);
			// Если зафиксифровать результат не вышло
			if(nghttp2_is_fatal(rv)){
				// Выводим сообщение об полученной ошибке
				self->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
				// Если функция обратного вызова на на вывод ошибок установлена
				if(self->_callback.is("error"))
					// Выполняем функцию обратного вызова
					self->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_RECV, nghttp2_strerror(rv));
				// Выходим из функции
				return rv;
			// Выполняем обновление размеров окна
			}// else self->windowUpdate(sid, size); // Система делает это сама когда потребуется
		}
	}
	// Выводим значение по умолчанию
	return result;
}
/**
 * header Функция обратного вызова при получении заголовка
 * @param session объект сессии
 * @param frame   объект фрейма заголовков
 * @param key     данные ключа заголовка
 * @param keySize размер ключа заголовка
 * @param val     данные значения заголовка
 * @param valSize размер значения заголовка
 * @param flags   флаги события для сессии
 * @param ctx     передаваемый промежуточный контекст
 * @return        статус обработки полученных данных
 */
int32_t awh::Http2::header([[maybe_unused]] nghttp2_session * session, const nghttp2_frame * frame, nghttp2_rcbuf * name, nghttp2_rcbuf * value, [[maybe_unused]] const uint8_t flags, void * ctx) noexcept {
	// Получаем объект родительского объекта
	http2_t * self = reinterpret_cast <http2_t *> (ctx);
	// Если функция обратного вызова установлена
	if(self->_callback.is("header")){
		// Получаем буфер названия заголовка
		auto nameBuffer = nghttp2_rcbuf_get_buf(name);
		// Получаем буфер значения заголовка
		auto valueBuffer = nghttp2_rcbuf_get_buf(value);
		// Выполняем определение типа фрейма
		switch(frame->hd.type){
			// Если мы получили push-уведомление
			case NGHTTP2_PUSH_PROMISE: {
				// Если мы получили заголовки ответа с клиента
				if(frame->headers.cat == NGHTTP2_HCAT_REQUEST)
					// Выполняем функцию обратного вызова
					return self->_callback.call <int32_t (const int32_t, const string &, const string &)> ("header", frame->hd.stream_id, string(reinterpret_cast <const char *> (nameBuffer.base), nameBuffer.len), string(reinterpret_cast <const char *> (valueBuffer.base), valueBuffer.len));
			} break;
			// Если мы получили входящие данные заголовков ответа
			case NGHTTP2_HEADERS: {
				// Определяем идентификатор сервиса
				switch(static_cast <uint8_t> (self->_mode)){
					// Если сервис идентифицирован как клиент
					case static_cast <uint8_t> (mode_t::CLIENT): {
						// Определяем флаг ответа сервера
						switch(static_cast <uint8_t> (frame->headers.cat)){
							// Если мы получили сырые заголовки с сервера
							case static_cast <uint8_t> (NGHTTP2_HCAT_HEADERS):
							// Если мы получили заголовки ответа с сервера
							case static_cast <uint8_t> (NGHTTP2_HCAT_RESPONSE):
							// Если мы получили заголовки промисов с сервера
							case static_cast <uint8_t> (NGHTTP2_HCAT_PUSH_RESPONSE):
								// Выполняем функцию обратного вызова
								return self->_callback.call <int32_t (const int32_t, const string &, const string &)> ("header", frame->hd.stream_id, string(reinterpret_cast <const char *> (nameBuffer.base), nameBuffer.len), string(reinterpret_cast <const char *> (valueBuffer.base), valueBuffer.len));
						}
					} break;
					// Если сервис идентифицирован как сервер
					case static_cast <uint8_t> (mode_t::SERVER): {
						// Если мы получили запрос клиента
						switch(static_cast <uint8_t> (frame->headers.cat)){
							// Если мы получили сырые заголовки с клиента
							case static_cast <uint8_t> (NGHTTP2_HCAT_HEADERS):
							// Если мы получили заголовки ответа с клиента
							case static_cast <uint8_t> (NGHTTP2_HCAT_REQUEST):
								// Выполняем функцию обратного вызова
								return self->_callback.call <int32_t (const int32_t, const string &, const string &)> ("header", frame->hd.stream_id, string(reinterpret_cast <const char *> (nameBuffer.base), nameBuffer.len), string(reinterpret_cast <const char *> (valueBuffer.base), valueBuffer.len));
						}
					} break;
				}
			} break;
		}
	}
	// Выводим результат
	return 0;
}
/**
 * send Функция обратного вызова при подготовки данных для отправки
 * @param session объект сессии
 * @param buffer  буфер данных которые следует отправить
 * @param size    размер буфера данных для отправки
 * @param flags   флаги события для сессии
 * @param ctx     передаваемый промежуточный контекст
 * @return        количество отправленных байт
 */
ssize_t awh::Http2::send([[maybe_unused]] nghttp2_session * session, const uint8_t * buffer, const size_t size, [[maybe_unused]] const int32_t flags, void * ctx) noexcept {
	// Получаем объект родительского объекта
	http2_t * self = reinterpret_cast <http2_t *> (ctx);
	// Если функция обратного вызова установлена
	if(self->_callback.is("send"))
		// Выполняем функцию обратного вызова
		self->_callback.call <void (const uint8_t *, const size_t)> ("send", buffer, size);
	// Возвращаем количество отправленных байт
	return static_cast <ssize_t> (size);
}
/**
 * send Функция отправки подготовленного буфера данных по сети
 * @param session объект сессии
 * @param sid     идентификатор потока
 * @param buffer  буфер данных которые следует отправить
 * @param size    размер буфера данных для отправки
 * @param flags   флаги события для сессии
 * @param source  объект промежуточных данных локального подключения
 * @param ctx     передаваемый промежуточный контекст
 * @return        количество отправленных байт
 */
ssize_t awh::Http2::send([[maybe_unused]] nghttp2_session * session, const int32_t sid, uint8_t * buffer, const size_t size, uint32_t * flags, [[maybe_unused]] nghttp2_data_source * source, void * ctx) noexcept {
	// Получаем объект родительского объекта
	http2_t * self = reinterpret_cast <http2_t *> (ctx);
	// Выполняем поиск неотправленных записей для потока
	auto i = self->_records.find(sid);
	// Если неотправленные записи для потока найдены
	if((i != self->_records.end()) && !i->second.empty()){
		// Количество доступных байт для отправки
		ssize_t result = 0;
		// Выполняем поиск буфера данных для потока
		auto j = self->_payloads.find(sid);
		// Если в буфере есть данные для отправки
		if((j != self->_payloads.end()) && !j->second->empty() && (j->second->size() >= i->second.front().first)){
			// Определяем размер данных который мы можем отправить
			result = static_cast <ssize_t> (i->second.front().first > size ? size : i->second.front().first);
			// Выполняем копирование нужного нам количества данных
			::memcpy(buffer, j->second->get(), result);
			// Выполняем удаление из буфера отправленные данные
			j->second->erase(result);
			// Уменьшаем размер отправленных данных записи
			i->second.front().first -= result;
			// Если все данные записи отправленны
			if(i->second.front().first == 0){
				// Выполняем удаление записи
				i->second.pop();
				// Если буфер пустой
				if(j->second->empty())
					// Очищаем буфер
					j->second->clear();
			// Если установленно требование завершить работу потока
			} else if(i->second.front().second == flag_t::END_STREAM)
				// Отменяем завершение работы подключения
				(* flags) |= NGHTTP2_DATA_FLAG_NO_END_STREAM;
		// Если произошла рассинхронизация буфера и потоков
		} else {
			// Удаляем записи для потока
			self->_records.erase(i);
			// Удаляем буфер данных
			self->_payloads.erase(sid);
			// Выводим сообщение об ошибке
			return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
		}
		// Устанавливаем флаг, завершения отправки данных
		(* flags) |= NGHTTP2_DATA_FLAG_EOF;
		// Выводим результат количество отправленных байт
		return result;
	// Выводим сообщение об ошибке
	} else return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
}
/**
 * available Метод проверки сколько байт доступно для отправки
 * @param sid идентификатор потока
 * @return    количество байт доступных для отправки
 */
size_t awh::Http2::available(const int32_t sid) const noexcept {
	// Результат работы функции
	size_t result = 0;
	// Если сессия инициализированна
	if(this->_session != nullptr){
		// Определяем сколько байт достуно для отправки в сессию
		const int32_t sessionBytes = nghttp2_session_get_remote_window_size(this->_session);
		// Определяем сколько байт доступно для отправки в поток
		const int32_t streamBytes = nghttp2_session_get_stream_remote_window_size(this->_session, sid);
		// Если количество байт получены положительные
		if((sessionBytes > 0) && (streamBytes > 0))
			// Определяем минимальное количество байт которые возможно отправить
			result = static_cast <size_t> (::min(sessionBytes, streamBytes));
		// Если количество байт достуных для отправки в поток получены отрицательные
		else if((sessionBytes > 0) && (streamBytes <= 0))
			// Устанавливаем количество доступных байт для отправки
			result = static_cast <size_t> (sessionBytes);
	}
	// Выводим результат
	return result;
}
/**
 * commit Метод применения изменений
 * @param event событие которому соответствует фиксация
 * @return      результат отправки
 */
bool awh::Http2::commit(const event_t event) noexcept {
	// Выполняем установку активного события
	this->_event = event;
	// Если сессия инициализированна
	if(this->_session != nullptr){
		// Фиксируем отправленный результат
		const int32_t rv = nghttp2_session_send(this->_session);
		// Если зафиксифровать результат не вышло
		if(nghttp2_is_fatal(rv)){
			// Выводим сообщение об полученной ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_SEND, nghttp2_strerror(rv));
			// Выходим из функции
			return false;
		}
	}
	// Выводим результат
	return true;
}
/**
 * completed Метод завершения выполнения операции
 * @param event событие выполненной операции
 */
void awh::Http2::completed(const event_t event) noexcept {
	// Если выполненное событие соответствует последнему событию
	if(event == this->_event){
		// Выполняем сброс активного события
		this->_event = event_t::NONE;
		// Если функция обратного вызова на тригер установлена
		if(this->_callback.is(1)){
			// Выполняем функцию триггера
			this->_callback.call <void (void)> (1);
			// Выполняем удаление функции триггера
			this->_callback.erase(1);
		}
		// Если есть требование закрыть подключение
		if(this->_close)
			// Выполняем закрытие подключения
			this->close();
	}
}
/**
 * submit Метод подготовки отправки данных полезной нагрузки
 * @param sid  идентификатор потока 
 * @param flag флаг передаваемого потока по сети
 * @return     результат работы функции
 */
bool awh::Http2::submit(const int32_t sid, const flag_t flag) noexcept {
	// Создаём объект передачи данных тела полезной нагрузки
	nghttp2_data_provider2 data;
	// Зануляем передаваемый контекст
	data.source.ptr = nullptr;
	// Устанавливаем функцию обратного вызова
	data.read_callback = &http2_t::send;
	// Если сессия инициализированна
	if(this->_session != nullptr){
		// Флаги фрейма передаваемого по сети
		uint8_t flags = NGHTTP2_FLAG_NONE;
		// Если флаг установлен завершения кадра
		if(flag == flag_t::END_STREAM)
			// Устанавливаем флаг фрейма передаваемого по сети
			flags = NGHTTP2_FLAG_END_STREAM;
		// Если библиотека не готова отдать или принять данные, тогда закрываем подключение
		if((nghttp2_session_want_read(this->_session) == 0) && (nghttp2_session_want_write(this->_session) == 0))
			// Выводим результат
			return false;
		// Выполняем формирование данных фрейма для отправки
		int32_t rv = nghttp2_submit_data2(this->_session, flags, sid, &data);
		// Если сформировать данные фрейма не вышло
		if(nghttp2_is_fatal(rv)){
			// Выводим сообщение об полученной ошибке
			this->_log->print("%s", log_t::flag_t::WARNING, nghttp2_strerror(rv));
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::WARNING, http::error_t::HTTP2_SUBMIT, nghttp2_strerror(rv));
			// Выводим результат
			return false;
		}
		// Если сессия инициализированна
		if(this->_session != nullptr){
			// Фиксируем отправленный результат
			rv = nghttp2_session_send(this->_session);
			// Если зафиксифровать результат не вышло
			if(nghttp2_is_fatal(rv)){
				// Выводим сообщение об полученной ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_SEND, nghttp2_strerror(rv));
				// Выходим из функции
				return false;
			}
			// Выводим результат
			return true;
		}
	}
	// Выводим результат
	return false;
}
/**
 * windowUpdate Метод обновления размера окна фрейма
 * @param sid  идентификатор потока
 * @param size размер нового окна
 * @return     результат установки размера офна фрейма
 */
bool awh::Http2::windowUpdate(const int32_t sid, const int32_t size) noexcept {
	// Если размер окна фрейма передан
	if(size > 0){
		// Если сессия инициализированна
		if(this->_session != nullptr){
			// Если библиотека не готова отдать или принять данные, тогда закрываем подключение
			if((nghttp2_session_want_read(this->_session) == 0) && (nghttp2_session_want_write(this->_session) == 0))
				// Выходим из функции
				return false;
			// Выполняем установку нового размера окна фрейма
			int32_t rv = nghttp2_submit_window_update(this->_session, NGHTTP2_FLAG_NONE, sid, size);
			// Если установить нового размера окна фрейма не вышло
			if(nghttp2_is_fatal(rv)){
				// Выводим сообщение об полученной ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
				// Выходим из функции
				return false;
			}
			// Фиксируем отправленный результат
			rv = nghttp2_session_send(this->_session);
			// Если зафиксифровать результат не вышло
			if(nghttp2_is_fatal(rv)){
				// Выводим сообщение об полученной ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_SEND, nghttp2_strerror(rv));
				// Выходим из функции
				return false;
			}
			// Выводим результат
			return true;
		}
	}
	// Выводим результат
	return false;
}
/**
 * ping Метод выполнения пинга
 * @return результат работы пинга
 */
bool awh::Http2::ping() noexcept {
	// Выполняем установку активного события
	this->_event = event_t::SEND_PING;
	// Если сессия инициализированна
	if(this->_session != nullptr){
		// Если библиотека не готова отдать или принять данные, тогда закрываем подключение
		if((nghttp2_session_want_read(this->_session) == 0) && (nghttp2_session_want_write(this->_session) == 0)){
			// Выполняем вызов метода выполненного события
			this->completed(event_t::SEND_PING);
			// Выходим из функции
			return false;
		}
		// Выполняем пинг удалённого узла
		const int32_t rv = nghttp2_submit_ping(this->_session, NGHTTP2_FLAG_ACK, nullptr);
		// Если отправить пинг не вышло
		if(nghttp2_is_fatal(rv)){
			// Выводим сообщение об полученной ошибке
			this->_log->print("%s", log_t::flag_t::WARNING, nghttp2_strerror(rv));
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::WARNING, http::error_t::HTTP2_PING, nghttp2_strerror(rv));
			// Выполняем вызов метода выполненного события
			this->completed(event_t::SEND_PING);
			// Выходим из функции
			return false;
		}
		// Выполняем применение изменений
		if(!this->commit(event_t::SEND_PING)){
			// Выполняем вызов метода выполненного события
			this->completed(event_t::SEND_PING);
			// Выходим из функции
			return false;
		}
	}
	// Выполняем вызов метода выполненного события
	this->completed(event_t::SEND_PING);
	// Выводим результат
	return true;
}
/**
 * shutdown Метод запрещения получения данных с клиента
 * @return результат выполнения операции
 */
bool awh::Http2::shutdown() noexcept {
	// Выполняем установку активного события
	this->_event = event_t::SEND_SHUTDOWN;
	// Если сессия инициализированна
	if(this->_session != nullptr){
		// Если библиотека не готова отдать или принять данные, тогда закрываем подключение
		if((nghttp2_session_want_read(this->_session) == 0) && (nghttp2_session_want_write(this->_session) == 0)){
			// Выполняем вызов метода выполненного события
			this->completed(event_t::SEND_SHUTDOWN);
			// Выходим из функции
			return false;
		}
		// Запрощаем получение данных с клиента
		const int32_t rv = nghttp2_submit_shutdown_notice(this->_session);
		// Если запретить получение данных с клиента не вышло
		if(nghttp2_is_fatal(rv)){
			// Выводим сообщение об полученной ошибке
			this->_log->print("%s", log_t::flag_t::WARNING, nghttp2_strerror(rv));
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::WARNING, http::error_t::HTTP2_PING, nghttp2_strerror(rv));
			// Выполняем вызов метода выполненного события
			this->completed(event_t::SEND_SHUTDOWN);
			// Выходим из функции
			return false;
		}
		// Выполняем применение изменений
		if(!this->commit(event_t::SEND_SHUTDOWN)){
			// Выполняем вызов метода выполненного события
			this->completed(event_t::SEND_SHUTDOWN);
			// Выходим из функции
			return false;
		}
	}
	// Выполняем вызов метода выполненного события
	this->completed(event_t::SEND_SHUTDOWN);
	// Выводим результат
	return true;
}
/**
 * frame Метод чтения данных фрейма из бинарного буфера
 * @param buffer буфер бинарных данных для чтения фрейма
 * @param size   размер буфера бинарных данных
 * @return       результат чтения данных фрейма
 */
bool awh::Http2::frame(const uint8_t * buffer, const size_t size) noexcept {
	// Выполняем установку активного события
	this->_event = event_t::RECV_FRAME;
	// Если данные для чтения переданы
	if((buffer != nullptr) && (size > 0)){
		// Если сессия инициализированна
		if(this->_session != nullptr){
			// Если библиотека не готова отдать или принять данные, тогда закрываем подключение
			if((nghttp2_session_want_read(this->_session) == 0) && (nghttp2_session_want_write(this->_session) == 0))
				// Выполняем завершение работы
				goto End;
			// Выполняем извлечение полученного чанка данных из сокета
			ssize_t bytes = nghttp2_session_mem_recv2(this->_session, buffer, size);
			// Если данные не прочитаны, выводим ошибку и выходим
			if(nghttp2_is_fatal(bytes)){
				// Выводим сообщение об полученной ошибке
				this->_log->print("%s", log_t::flag_t::WARNING, nghttp2_strerror(static_cast <int32_t> (bytes)));
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::WARNING, http::error_t::HTTP2_RECV, nghttp2_strerror(static_cast <int32_t> (bytes)));
				// Выполняем завершение работы
				goto End;
			}
			// Выполняем применение изменений
			if(!this->commit(event_t::RECV_FRAME))
				// Выполняем завершение работы
				goto End;
		}
		// Выполняем вызов метода выполненного события
		this->completed(event_t::RECV_FRAME);
		// Выводим результат
		return true;
	}
	// Устанавливаем метку завершения работы
	End:
	// Выполняем вызов метода выполненного события
	this->completed(event_t::RECV_FRAME);
	// Выводим результат
	return false;
}
/**
 * reject Метод выполнения сброса подключения
 * @param sid   идентификатор потока
 * @param error код отправляемой ошибки
 * @return      результат отправки сообщения
 */
bool awh::Http2::reject(const int32_t sid, const error_t error) noexcept {
	// Выполняем установку активного события
	this->_event = event_t::SEND_REJECT;
	// Определяем идентификатор сервиса
	switch(static_cast <uint8_t> (this->_mode)){
		// Если сервис идентифицирован как клиент
		case static_cast <uint8_t> (mode_t::CLIENT): {
			// Выводим сообщение об ошибке
			this->_log->print("Client is not allowed to call the \"%s\" method", log_t::flag_t::WARNING, "REJECT");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::WARNING, http::error_t::HTTP2_CANCEL, "Client is not allowed to call the \"REJECT\" method");
		} break;
		// Если сервис идентифицирован как сервер
		case static_cast <uint8_t> (mode_t::SERVER): {
			// Если сессия инициализированна
			if(this->_session != nullptr){
				// Создаём код передаваемой ошибки
				uint32_t code = NGHTTP2_NO_ERROR;
				// Определяем тип переданной ошибки
				switch(static_cast <uint8_t> (error)){
					// Требование выполнения отмены запроса
					case static_cast <uint8_t> (error_t::CANCEL):
						// Устанавливаем код ошибки
						code = NGHTTP2_CANCEL;
					break;
					// Ошибка TCP-соединения для метода CONNECT
					case static_cast <uint8_t> (error_t::CONNECT_ERROR):
						// Устанавливаем код ошибки
						code = NGHTTP2_CONNECT_ERROR;
					break;
					// Получен кадр для завершения потока
					case static_cast <uint8_t> (error_t::STREAM_CLOSED):
						// Устанавливаем код ошибки
						code = NGHTTP2_STREAM_CLOSED;
					break;
					// Поток не обработан
					case static_cast <uint8_t> (error_t::REFUSED_STREAM):
						// Устанавливаем код ошибки
						code = NGHTTP2_REFUSED_STREAM;
					break;
					// Ошибка протокола HTTP/2
					case static_cast <uint8_t> (error_t::PROTOCOL_ERROR):
						// Устанавливаем код ошибки
						code = NGHTTP2_PROTOCOL_ERROR;
					break;
					// Получена ошибка реализации
					case static_cast <uint8_t> (error_t::INTERNAL_ERROR):
						// Устанавливаем код ошибки
						code = NGHTTP2_INTERNAL_ERROR;
					break;
					// Размер кадра некорректен
					case static_cast <uint8_t> (error_t::FRAME_SIZE_ERROR):
						// Устанавливаем код ошибки
						code = NGHTTP2_FRAME_SIZE_ERROR;
					break;
					// Установка параметров завершилась по таймауту
					case static_cast <uint8_t> (error_t::SETTINGS_TIMEOUT):
						// Устанавливаем код ошибки
						code = NGHTTP2_SETTINGS_TIMEOUT;
					break;
					// Состояние компрессии не обновлено
					case static_cast <uint8_t> (error_t::COMPRESSION_ERROR):
						// Устанавливаем код ошибки
						code = NGHTTP2_COMPRESSION_ERROR;
					break;
					// Превышена емкость для обработки
					case static_cast <uint8_t> (error_t::ENHANCE_YOUR_CALM):
						// Устанавливаем код ошибки
						code = NGHTTP2_ENHANCE_YOUR_CALM;
					break;
					// Для запроса требуется протокол HTTP/1.1
					case static_cast <uint8_t> (error_t::HTTP_1_1_REQUIRED):
						// Устанавливаем код ошибки
						code = NGHTTP2_HTTP_1_1_REQUIRED;
					break;
					// Ошибка превышения предела управления потоком
					case static_cast <uint8_t> (error_t::FLOW_CONTROL_ERROR):
						// Устанавливаем код ошибки
						code = NGHTTP2_FLOW_CONTROL_ERROR;
					break;
					// Согласованные параметры SSL не приемлемы
					case static_cast <uint8_t> (error_t::INADEQUATE_SECURITY):
						// Устанавливаем код ошибки
						code = NGHTTP2_INADEQUATE_SECURITY;
					break;
				}
				// Если библиотека не готова отдать или принять данные, тогда закрываем подключение
				if((nghttp2_session_want_read(this->_session) == 0) && (nghttp2_session_want_write(this->_session) == 0))
					// Выполняем завершение работы
					goto End;
				// Выполняем сброс подключения клиента
				const int32_t rv = nghttp2_submit_rst_stream(this->_session, NGHTTP2_FLAG_NONE, sid, code);
				// Если сброс подключения клиента не выполнен
				if(nghttp2_is_fatal(rv)){
					// Выводим сообщение об полученной ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
					// Выполняем завершение работы
					goto End;
				}
				// Выполняем применение изменений
				if(!this->commit(event_t::SEND_REJECT))
					// Выполняем завершение работы
					goto End;
				// Выполняем вызов метода выполненного события
				this->completed(event_t::SEND_REJECT);
				// Выводим результат
				return true;
			}
		} break;
	}
	// Устанавливаем метку завершения работы
	End:
	// Выполняем вызов метода выполненного события
	this->completed(event_t::SEND_REJECT);
	// Выводим результат
	return false;
}
/**
 * sendOrigin Метод отправки списка разрешённых источников
 */
void awh::Http2::sendOrigin() noexcept {
	// Выполняем установку активного события
	this->_event = event_t::SEND_ORIGIN;
	// Если список источников передан
	if(!this->_origins.empty()){
		// Определяем идентификатор сервиса
		switch(static_cast <uint8_t> (this->_mode)){
			// Если сервис идентифицирован как клиент
			case static_cast <uint8_t> (mode_t::CLIENT): {
				// Выводим сообщение об ошибке
				this->_log->print("Client is not allowed to call the \"%s\" method", log_t::flag_t::WARNING, "ORIGIN");
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::WARNING, http::error_t::HTTP2_CANCEL, "Client is not allowed to call the \"ORIGIN\" method");
			} break;
			// Если сервис идентифицирован как сервер
			case static_cast <uint8_t> (mode_t::SERVER): {
				// Список источников для установки на клиенте
				vector <nghttp2_origin_entry> ov;
				// Выполняем перебор списка источников
				for(auto & origin : this->_origins)
					// Выполняем добавление источника в списку
					ov.push_back({(uint8_t *) origin.c_str(), origin.size()});
				// Если сессия инициализированна
				if(this->_session != nullptr){
					// Если библиотека не готова отдать или принять данные, тогда закрываем подключение
					if((nghttp2_session_want_read(this->_session) == 0) && (nghttp2_session_want_write(this->_session) == 0))
						// Выполняем завершение работы
						goto End;
					// Выполняем отправки списка разрешённых источников
					const int32_t rv = nghttp2_submit_origin(this->_session, NGHTTP2_FLAG_NONE, (!ov.empty() ? ov.data() : nullptr), ov.size());
					// Если отправить список разрешённых источников не вышло
					if(nghttp2_is_fatal(rv)){
						// Выводим сообщение об полученной ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
						// Выходим из функции
						goto End;
					}
					// Выполняем применение изменений
					this->commit(event_t::SEND_ORIGIN);
				}
			}
		}
	}
	// Устанавливаем метку завершения работы
	End:
	// Выполняем вызов метода выполненного события
	this->completed(event_t::SEND_ORIGIN);
}
/**
 * sendAltSvc Метод отправки расширения альтернативного сервиса RFC7383
 * @param sid идентификатор потока
 */
void awh::Http2::sendAltSvc(const int32_t sid) noexcept {
	// Выполняем установку активного события
	this->_event = event_t::SEND_ALTSVC;
	// Если список альтернативных сервисов не пустой
	if(!this->_altsvc.empty()){
		// Определяем идентификатор сервиса
		switch(static_cast <uint8_t> (this->_mode)){
			// Если сервис идентифицирован как клиент
			case static_cast <uint8_t> (mode_t::CLIENT): {
				// Выводим сообщение об ошибке
				this->_log->print("Client is not allowed to call the \"%s\" method", log_t::flag_t::WARNING, "ALTSVC");
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::WARNING, http::error_t::HTTP2_CANCEL, "Client is not allowed to call the \"ALTSVC\" method");
			} break;
			// Если сервис идентифицирован как сервер
			case static_cast <uint8_t> (mode_t::SERVER): {
				// Если сессия инициализированна
				if(this->_session != nullptr){
					// Определяем идентификатор потока
					switch(sid){
						// Если фрейм является системным
						case 0: {
							// Выполняем перебор всех альтернативных сервисов
							for(auto & item : this->_altsvc){
								// Если библиотека не готова отдать или принять данные, тогда закрываем подключение
								if((nghttp2_session_want_read(this->_session) == 0) && (nghttp2_session_want_write(this->_session) == 0))
									// Выполняем завершение работы
									goto End;
								// Выполняем отправку альтернативного сервиса
								const int32_t rv = nghttp2_submit_altsvc(this->_session, NGHTTP2_FLAG_NONE, sid, reinterpret_cast <const uint8_t *> (item.first.c_str()), item.first.size(), reinterpret_cast <const uint8_t *> (item.second.c_str()), item.second.size());
								// Если отправить алтернативного сервиса не вышло
								if(nghttp2_is_fatal(rv)){
									// Выводим сообщение об полученной ошибке
									this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
									// Выходим из функции
									goto End;
								}
								// Выполняем применение изменений
								if(!this->commit(event_t::SEND_ALTSVC))
									// Выходим из функции
									goto End;
							}
						} break;
						// Если фрейм является пользовательским запросом
						default: {
							// Если библиотека не готова отдать или принять данные, тогда закрываем подключение
							if((nghttp2_session_want_read(this->_session) == 0) && (nghttp2_session_want_write(this->_session) == 0))
								// Выполняем завершение работы
								goto End;
							// Выполняем отправку альтернативного сервиса
							const int32_t rv = nghttp2_submit_altsvc(this->_session, NGHTTP2_FLAG_NONE, sid, nullptr, 0, nullptr, 0);
							// Если отправить алтернативного сервиса не вышло
							if(nghttp2_is_fatal(rv)){
								// Выводим сообщение об полученной ошибке
								this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
								// Выходим из функции
								goto End;
							}
							// Выполняем применение изменений
							if(!this->commit(event_t::SEND_ALTSVC))
								// Выходим из функции
								goto End;
						}
					}
				}
			} break;
		}
	}
	// Устанавливаем метку завершения работы
	End:
	// Выполняем вызов метода выполненного события
	this->completed(event_t::SEND_ALTSVC);
}
/**
 * sendTrailers Метод отправки трейлеров
 * @param id      идентификатор потока
 * @param headers заголовки отправляемые
 * @return        результат отправки данных фрейма
 */
bool awh::Http2::sendTrailers(const int32_t id, const vector <pair <string, string>> & headers) noexcept {
	// Выполняем установку активного события
	this->_event = event_t::SEND_TRAILERS;
	// Если заголовки для отправки переданы и сессия инициализированна
	if(!headers.empty() && (this->_session != nullptr)){
		// Определяем идентификатор сервиса
		switch(static_cast <uint8_t> (this->_mode)){
			// Если сервис идентифицирован как клиент
			case static_cast <uint8_t> (mode_t::CLIENT): {
				// Выводим сообщение об ошибке
				this->_log->print("Client is not allowed to call the \"%s\" method", log_t::flag_t::WARNING, "TRAILERS");
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::WARNING, http::error_t::HTTP2_CANCEL, "Client is not allowed to call the \"TRAILERS\" method");
			} break;
			// Если сервис идентифицирован как сервер
			case static_cast <uint8_t> (mode_t::SERVER): {
				// Список заголовков для запроса
				vector <nghttp2_nv> nva;
				// Выполняем перебор всех заголовков запроса
				for(auto & header : headers){
					// Выполняем добавление метода запроса
					nva.push_back({
						(uint8_t *) header.first.c_str(),
						(uint8_t *) header.second.c_str(),
						header.first.size(),
						header.second.size(),
						NGHTTP2_NV_FLAG_NONE
					});
				}
				// Если библиотека не готова отдать или принять данные, тогда закрываем подключение
				if((nghttp2_session_want_read(this->_session) == 0) && (nghttp2_session_want_write(this->_session) == 0))
					// Выполняем завершение работы
					goto End;
				// Выполняем формирование фрейма трейлера
				const int32_t rv = nghttp2_submit_trailer(this->_session, id, nva.data(), nva.size());
				// Если сформировать фрейма трейлера не выполнено
				if(nghttp2_is_fatal(rv)){
					// Выводим сообщение об полученной ошибке
					this->_log->print("%s", log_t::flag_t::WARNING, nghttp2_strerror(rv));
					// Если функция обратного вызова на на вывод ошибок установлена
					if(this->_callback.is("error"))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::WARNING, http::error_t::HTTP2_SUBMIT, nghttp2_strerror(rv));
					// Выходим из функции
					goto End;
				}
				// Выполняем применение изменений
				if(!this->commit(event_t::SEND_TRAILERS))
					// Выходим из функции
					goto End;
				// Выполняем вызов метода выполненного события
				this->completed(event_t::SEND_TRAILERS);
				// Выводим результат
				return true;
			}
		}
	}
	// Устанавливаем метку завершения работы
	End:
	// Выполняем вызов метода выполненного события
	this->completed(event_t::SEND_TRAILERS);
	// Выводим результат
	return false;
}
/**
 * sendData Метод отправки бинарных данных
 * @param id     идентификатор потока
 * @param buffer буфер бинарных данных передаваемых
 * @param size   размер передаваемых данных в байтах
 * @param flag   флаг передаваемого потока по сети
 * @return       результат отправки данных фрейма
 */
bool awh::Http2::sendData(const int32_t id, const uint8_t * buffer, const size_t size, const flag_t flag) noexcept {
	// Выполняем установку активного события
	this->_event = event_t::SEND_DATA;
	// Если данные полезной нагрузки для отправки в сеть переданы
	if((buffer != nullptr) && (size > 0)){
		// Если сессия инициализированна
		if(this->_session != nullptr){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				{
					// Выполняем поиск буфера полезной нагрузки
					auto i = this->_payloads.find(id);
					// Если буфер полезной нагрузки существует
					if(i != this->_payloads.end())
						// Выполняем добавление в буфер данных полезной нагрузки
						i->second->push(buffer, size);
					// Если буфер полезной нагрузки не существует
					else {
						// Выполняем создание буфера полезной нагрузки
						auto ret = this->_payloads.emplace(id, std::make_unique <buffer_t> (this->_log));
						// Выполняем добавление в буфер данных полезной нагрузки
						ret.first->second->push(buffer, size);
					}
				}{
					// Выполняем получение списка записей для потока
					auto i = this->_records.find(id);
					// Если список записей для потока получен
					if(i != this->_records.end())
						// Выполняем добавление новой записи
						i->second.push(std::make_pair(size, flag));
					// Выполняем создание нового списка записей для потока
					else {
						// Выполняем создание нового списка записей для потока
						auto ret = this->_records.emplace(id, std::queue <pair <size_t, flag_t>> ());
						// Выполняем добавление новой записи
						ret.first->second.push(std::make_pair(size, flag));
					}
				}
				// Если библиотека не готова отдать или принять данные, тогда закрываем подключение
				if((nghttp2_session_want_read(this->_session) == 0) && (nghttp2_session_want_write(this->_session) == 0))
					// Выполняем завершение работы
					goto End;
				{
					// Выполняем поиск записей для потока
					auto i = this->_records.find(id);
					// Если список неотправленных записей для потока существуют
					if((i != this->_records.end()) && !i->second.empty()){
						// Выполняем перебор всего списка записей которые ещё не отправленны
						while(!i->second.empty()){
							// Если на принимаемой стороне достаточно памяти для получения данных
							if(this->available(i->first) >= i->second.front().first)
								// Выполняем отправку записи для указанного потока
								this->submit(i->first, i->second.front().second);
							// Если данных не достаточно, выходим
							else break;
						}
					}
				}
				// Выполняем вызов метода выполненного события
				this->completed(event_t::SEND_DATA);
				// Выводим результат
				return true;
			/**
			 * Если возникает ошибка
			 */
			} catch(const bad_alloc &) {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, buffer, size, static_cast <uint16_t> (flag)), log_t::flag_t::CRITICAL, "Memory allocation error");
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
				#endif
				// Выходим из приложения
				::exit(EXIT_FAILURE);
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, buffer, size, static_cast <uint16_t> (flag)), log_t::flag_t::CRITICAL, error.what());
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			}
		}
	}
	// Устанавливаем метку завершения работы
	End:
	// Если записи для потока существуют
	if(this->_records.find(id) != this->_records.end())
		// Выполняем очистку всех записей для потока
		this->_records.erase(id);
	// Если буферы полезной нагрузки для потока существуют
	if(this->_payloads.find(id) != this->_payloads.end())
		// Выполняем очистку буферов полезной нагрузки
		this->_payloads.erase(id);
	// Выполняем вызов метода выполненного события
	this->completed(event_t::SEND_DATA);
	// Выводим результат
	return false;
}
/**
 * sendPush Метод отправки push-уведомлений
 * @param id      идентификатор потока
 * @param headers заголовки отправляемые
 * @param flag    флаг передаваемого потока по сети
 * @return        флаг завершения потока передачи данных
 */
int32_t awh::Http2::sendPush(const int32_t id, const vector <pair <string, string>> & headers, const flag_t flag) noexcept {
	// Результат работы функции
	int32_t result = -1;
	// Выполняем установку активного события
	this->_event = event_t::SEND_PUSH;
	// Если заголовки для отправки переданы и сессия инициализированна
	if(!headers.empty() && (this->_session != nullptr)){
		// Список заголовков для запроса
		vector <nghttp2_nv> nva;
		// Выполняем перебор всех заголовков запроса
		for(auto & header : headers){
			// Выполняем добавление метода запроса
			nva.push_back({
				(uint8_t *) header.first.c_str(),
				(uint8_t *) header.second.c_str(),
				header.first.size(),
				header.second.size(),
				NGHTTP2_NV_FLAG_NONE
			});
		}
		// Флаги фрейма передаваемого по сети
		uint8_t flags = NGHTTP2_FLAG_NONE;
		// Определяем флаг переданный в запросе
		switch(static_cast <uint8_t> (flag)){
			// Если требуется завершить передачу заголовков
			case static_cast <uint8_t> (flag_t::END_HEADERS):
				// Выполняем установку флагов
				flags = NGHTTP2_FLAG_END_HEADERS;
			break;
			// Если требуется завершить поток после передачи фрейма
			case static_cast <uint8_t> (flag_t::END_STREAM):
				// Устанавливаем флаг фрейма передаваемого по сети
				flags = NGHTTP2_FLAG_END_STREAM;
			break;
		}
		// Если библиотека не готова отдать или принять данные, тогда закрываем подключение
		if((nghttp2_session_want_read(this->_session) == 0) && (nghttp2_session_want_write(this->_session) == 0)){
			// Выполняем вызов метода выполненного события
			this->completed(event_t::SEND_PUSH);
			// Выходим из функции
			return -1;
		}
		// Выполняем push-уведомление клиенту
		result = nghttp2_submit_push_promise(this->_session, flags, id, nva.data(), nva.size(), nullptr);
		// Если запрос не получилось отправить
		if(nghttp2_is_fatal(result)){
			// Выводим в лог сообщение
			this->_log->print("%s", log_t::flag_t::WARNING, nghttp2_strerror(result));
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::WARNING, http::error_t::HTTP2_SUBMIT, nghttp2_strerror(result));
			// Выполняем вызов метода выполненного события
			this->completed(event_t::SEND_PUSH);
			// Выходим из функции
			return result;
		}
		// Выполняем применение изменений
		if(!this->commit(event_t::SEND_PUSH)){
			// Выполняем вызов метода выполненного события
			this->completed(event_t::SEND_PUSH);
			// Выходим из функции
			return -1;
		}
	}
	// Выполняем вызов метода выполненного события
	this->completed(event_t::SEND_PUSH);
	// Выводим результат
	return result;
}
/**
 * sendHeaders Метод отправки заголовков
 * @param id      идентификатор потока
 * @param headers заголовки отправляемые
 * @param flag    флаг передаваемого потока по сети
 * @return        флаг завершения потока передачи данных
 */
int32_t awh::Http2::sendHeaders(const int32_t id, const vector <pair <string, string>> & headers, const flag_t flag) noexcept {
	// Результат работы функции
	int32_t result = -1;
	// Выполняем установку активного события
	this->_event = event_t::SEND_HEADERS;
	// Если заголовки для отправки переданы и сессия инициализированна
	if(!headers.empty() && (this->_session != nullptr)){
		// Список заголовков для запроса
		vector <nghttp2_nv> nva;
		// Выполняем перебор всех заголовков запроса
		for(auto & header : headers){
			// Выполняем добавление метода запроса
			nva.push_back({
				(uint8_t *) header.first.c_str(),
				(uint8_t *) header.second.c_str(),
				header.first.size(),
				header.second.size(),
				NGHTTP2_NV_FLAG_NONE
			});
		}
		// Флаги фрейма передаваемого по сети
		uint8_t flags = NGHTTP2_FLAG_NONE;
		// Определяем флаг переданный в запросе
		switch(static_cast <uint8_t> (flag)){
			// Если требуется завершить передачу заголовков
			case static_cast <uint8_t> (flag_t::END_HEADERS):
				// Выполняем установку флагов
				flags = NGHTTP2_FLAG_END_HEADERS;
			break;
			// Если требуется завершить поток после передачи фрейма
			case static_cast <uint8_t> (flag_t::END_STREAM):
				// Устанавливаем флаг фрейма передаваемого по сети
				flags = NGHTTP2_FLAG_END_STREAM;
			break;
		}
		// Если библиотека не готова отдать или принять данные, тогда закрываем подключение
		if((nghttp2_session_want_read(this->_session) == 0) && (nghttp2_session_want_write(this->_session) == 0)){
			// Выполняем вызов метода выполненного события
			this->completed(event_t::SEND_HEADERS);
			// Выходим из функции
			return -1;
		}
		// Выполняем отправку заголовков удалённому узлу
		result = nghttp2_submit_headers(this->_session, flags, id, nullptr, nva.data(), nva.size(), nullptr);
		// Если запрос не получилось отправить
		if(nghttp2_is_fatal(result)){
			// Выводим в лог сообщение
			this->_log->print("%s", log_t::flag_t::WARNING, nghttp2_strerror(result));
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::WARNING, http::error_t::HTTP2_SUBMIT, nghttp2_strerror(result));
			// Выполняем вызов метода выполненного события
			this->completed(event_t::SEND_HEADERS);
			// Выходим из функции
			return result;
		}
		// Выполняем применение изменений
		if(!this->commit(event_t::SEND_HEADERS)){
			// Выполняем вызов метода выполненного события
			this->completed(event_t::SEND_HEADERS);
			// Выходим из функции
			return -1;
		}
	}
	// Выполняем вызов метода выполненного события
	this->completed(event_t::SEND_HEADERS);
	// Выводим результат
	return result;
}
/**
 * goaway Метод отправки сообщения закрытия всех потоков
 * @param last   идентификатор последнего потока
 * @param error  код отправляемой ошибки
 * @param buffer буфер отправляемых данных если требуется
 * @param size   размер отправляемого буфера данных
 * @return       результат отправки данных фрейма
 */
bool awh::Http2::goaway(const int32_t last, const error_t error, const uint8_t * buffer, const size_t size) noexcept {
	// Выполняем установку активного события
	this->_event = event_t::SEND_GOAWAY;
	// Если размер окна фрейма передан
	if(last > 0){
		// Определяем идентификатор сервиса
		switch(static_cast <uint8_t> (this->_mode)){
			// Если сервис идентифицирован как клиент
			case static_cast <uint8_t> (mode_t::CLIENT): {
				// Выводим сообщение об ошибке
				this->_log->print("Client is not allowed to call the \"%s\" method", log_t::flag_t::WARNING, "GOAWAY");
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::WARNING, http::error_t::HTTP2_CANCEL, "Client is not allowed to call the \"GOAWAY\" method");
			} break;
			// Если сервис идентифицирован как сервер
			case static_cast <uint8_t> (mode_t::SERVER): {
				// Если сессия инициализированна
				if(this->_session != nullptr){
					// Создаём код передаваемой ошибки
					uint32_t code = NGHTTP2_NO_ERROR;
					// Определяем тип переданной ошибки
					switch(static_cast <uint8_t> (error)){
						// Требование выполнения отмены запроса
						case static_cast <uint8_t> (error_t::CANCEL):
							// Устанавливаем код ошибки
							code = NGHTTP2_CANCEL;
						break;
						// Ошибка TCP-соединения для метода CONNECT
						case static_cast <uint8_t> (error_t::CONNECT_ERROR):
							// Устанавливаем код ошибки
							code = NGHTTP2_CONNECT_ERROR;
						break;
						// Получен кадр для завершения потока
						case static_cast <uint8_t> (error_t::STREAM_CLOSED):
							// Устанавливаем код ошибки
							code = NGHTTP2_STREAM_CLOSED;
						break;
						// Поток не обработан
						case static_cast <uint8_t> (error_t::REFUSED_STREAM):
							// Устанавливаем код ошибки
							code = NGHTTP2_REFUSED_STREAM;
						break;
						// Ошибка протокола HTTP/2
						case static_cast <uint8_t> (error_t::PROTOCOL_ERROR):
							// Устанавливаем код ошибки
							code = NGHTTP2_PROTOCOL_ERROR;
						break;
						// Получена ошибка реализации
						case static_cast <uint8_t> (error_t::INTERNAL_ERROR):
							// Устанавливаем код ошибки
							code = NGHTTP2_INTERNAL_ERROR;
						break;
						// Размер кадра некорректен
						case static_cast <uint8_t> (error_t::FRAME_SIZE_ERROR):
							// Устанавливаем код ошибки
							code = NGHTTP2_FRAME_SIZE_ERROR;
						break;
						// Установка параметров завершилась по таймауту
						case static_cast <uint8_t> (error_t::SETTINGS_TIMEOUT):
							// Устанавливаем код ошибки
							code = NGHTTP2_SETTINGS_TIMEOUT;
						break;
						// Состояние компрессии не обновлено
						case static_cast <uint8_t> (error_t::COMPRESSION_ERROR):
							// Устанавливаем код ошибки
							code = NGHTTP2_COMPRESSION_ERROR;
						break;
						// Превышена емкость для обработки
						case static_cast <uint8_t> (error_t::ENHANCE_YOUR_CALM):
							// Устанавливаем код ошибки
							code = NGHTTP2_ENHANCE_YOUR_CALM;
						break;
						// Для запроса требуется протокол HTTP/1.1
						case static_cast <uint8_t> (error_t::HTTP_1_1_REQUIRED):
							// Устанавливаем код ошибки
							code = NGHTTP2_HTTP_1_1_REQUIRED;
						break;
						// Ошибка превышения предела управления потоком
						case static_cast <uint8_t> (error_t::FLOW_CONTROL_ERROR):
							// Устанавливаем код ошибки
							code = NGHTTP2_FLOW_CONTROL_ERROR;
						break;
						// Согласованные параметры SSL не приемлемы
						case static_cast <uint8_t> (error_t::INADEQUATE_SECURITY):
							// Устанавливаем код ошибки
							code = NGHTTP2_INADEQUATE_SECURITY;
						break;
					}
					// Если библиотека не готова отдать или принять данные, тогда закрываем подключение
					if((nghttp2_session_want_read(this->_session) == 0) && (nghttp2_session_want_write(this->_session) == 0))
						// Выполняем завершение работы
						goto End;
					// Выполняем отправку сообщения закрытия всех потоков
					const int32_t rv = nghttp2_submit_goaway(this->_session, NGHTTP2_FLAG_NONE, last, code, buffer, size);
					// Если отправить сообщения закрытия потоков, не получилось
					if(nghttp2_is_fatal(rv)){
						// Выводим сообщение об полученной ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
						// Выполняем завершение работы
						goto End;
					}
					// Выполняем применение изменений
					if(!this->commit(event_t::SEND_GOAWAY))
						// Выполняем завершение работы
						goto End;
					// Выполняем вызов метода выполненного события
					this->completed(event_t::SEND_GOAWAY);
					// Выводим результат
					return true;
				}
			} break;
		}
	}
	// Устанавливаем метку завершения работы
	End:
	// Выполняем вызов метода выполненного события
	this->completed(event_t::SEND_GOAWAY);
	// Выводим результат
	return false;
}
/**
 * free Метод очистки активной сессии
 */
void awh::Http2::free() noexcept {
	// Если сессия создана удачно
	if(this->_session != nullptr){
		// Выполняем удаление сессии
		nghttp2_session_del(this->_session);
		// Выполняем обнуление активной сессии
		this->_session = nullptr;
	}
	// Если очереди полезной нагрузки созданы
	if(!this->_payloads.empty())
		// Выполняем очистку очередей полезной нагрузки
		this->_payloads.clear();
	// Если список записей существует
	if(!this->_records.empty())
		// Выполняем удаление всего списка записей
		this->_records.clear();
}
/**
 * close Метод закрытия подключения
 */
void awh::Http2::close() noexcept {
	// Если активное событие не установлено
	if(!(this->_close = (this->_event != event_t::NONE))){
		// Если сессия создана удачно
		if(this->_session != nullptr){
			// Результат завершения сессии
			const int32_t rv = nghttp2_session_terminate_session(this->_session, NGHTTP2_NO_ERROR);
			// Если выполнить остановку сессии не вышло
			if(nghttp2_is_fatal(rv)){
				// Выводим сообщение об ошибке
				this->_log->print("Could not terminate session: %s", log_t::flag_t::WARNING, nghttp2_strerror(rv));
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::WARNING, http::error_t::HTTP2_CANCEL, this->_fmk->format("Could not terminate session: %s", nghttp2_strerror(rv)));
			}
			// Выполняем удаление созданную ранее сессию
			this->free();
		}
	}
}
/**
 * is Метод проверки инициализации модуля
 * @return результат проверки инициализации
 */
bool awh::Http2::is() const noexcept {
	// Выводим результат проверки
	return (this->_session != nullptr);
}
/**
 * callback Метод установки функций обратного вызова
 * @param callback функции обратного вызова
 */
void awh::Http2::callback(const callback_t & callback) noexcept {
	// Если установлена триггерная функция
	if(callback.is(1)){
		// Если активное событие не установлено
		if(this->_event == event_t::NONE)
			// Выполняем функцию обратного вызова
			callback.call <void (void)> (1);
		// Устанавливаем функцию обратного вызова
		else this->_callback.set(1, callback);
	// Если триггерная функция не установлена
	} else {
		// Устанавливаем функцию обратного вызова при отправки сообщения
		this->_callback.set("send", callback);
		// Устанавливаем функцию обратного вызова при закрытии потока
		this->_callback.set("close", callback);
		// Устанавливаем функцию обратного вызова начала открытии потока
		this->_callback.set("begin", callback);
		// Устанавливаем функцию обратного вызова при получении чанка
		this->_callback.set("chunk", callback);
		// Устанавливаем функцию обратного вызова на событие получения ошибки
		this->_callback.set("error", callback);
		// Устанавливаем функцию обратного вызова при обмене фреймами
		this->_callback.set("frame", callback);
		// Устанавливаем функцию обратного вызова при создании фрейма
		this->_callback.set("create", callback);
		// Устанавливаем функцию обратного вызова при получении данных заголовка
		this->_callback.set("header", callback);
		// Устанавливаем функцию обратного вызова при получении источника подключения
		this->_callback.set("origin", callback);
		// Устанавливаем функцию обратного вызова при получении альтернативных сервисов
		this->_callback.set("altsvc", callback);
	}
}
/**
 * origin Метод установки списка разрешённых источников
 * @param origins список разрешённых источников
 */
void awh::Http2::origin(const vector <string> & origins) noexcept {
	// Выполняем установку списка разрешённых источников
	this->_origins.assign(origins.begin(), origins.end());
}
/**
 * altsvc Метод установки списка альтернативных сервисов
 * @param origins список альтернативных сервисов
 */
void awh::Http2::altsvc(const std::unordered_multimap <string, string> & origins) noexcept {
	// Выполняем установку списка альтернативных сервисов
	this->_altsvc = origins;
}
/**
 * init Метод инициализации
 * @param mode     идентификатор сервиса
 * @param settings параметры настроек сессии
 * @return         результат выполнения инициализации
 */
bool awh::Http2::init(const mode_t mode, const std::map <settings_t, uint32_t> & settings) noexcept {
	// Результат работы функции
	bool result = false;
	// Если параметры настроек переданы
	if(!settings.empty() && (mode != mode_t::NONE)){
		// Выполняем очистку предыдущей сессии
		this->free();
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выполняем установку функции для вывода отладочной информации
			nghttp2_set_debug_vprintf_callback(&http2_t::debug);
		#endif
		// Выполняем установку идентификатора сессии
		this->_mode = mode;
		// Создаём объект функций обратного вызова
		nghttp2_session_callbacks * callback;
		// Выполняем инициализацию сессию функций обратного вызова
		nghttp2_session_callbacks_new(&callback);
		// Выполняем установку функции обратного вызова при подготовки данных для отправки
		nghttp2_session_callbacks_set_send_callback2(callback, &http2_t::send);
		// Выполняем установку функции обратного вызова при перехвате ошибок протокола
		nghttp2_session_callbacks_set_error_callback2(callback, &http2_t::error);
		// Выполняем установку функции обратного вызова при получении заголовка
		nghttp2_session_callbacks_set_on_header_callback2(callback, &http2_t::header);
		// Выполняем установку функции обратного вызова при создании нового фрейма
		nghttp2_session_callbacks_set_on_begin_frame_callback(callback, &http2_t::create);
		// Выполняем установку функции обратного вызова закрытия подключения
		nghttp2_session_callbacks_set_on_stream_close_callback(callback, &http2_t::close);
		// Выполняем установку функции обратного вызова начала получения фрейма заголовков
		nghttp2_session_callbacks_set_on_begin_headers_callback(callback, &http2_t::begin);
		// Выполняем установку функции обратного вызова при получении фрейма заголовков
		nghttp2_session_callbacks_set_on_frame_recv_callback(callback, &http2_t::frameRecv);
		// Выполняем установку функции обратного вызова при отправки фрейма заголовков
		nghttp2_session_callbacks_set_on_frame_send_callback(callback, &http2_t::frameSend);
		// Выполняем установку функции обратного вызова при получении чанка
		nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callback, &http2_t::chunk);
		// Создаём параметры сессии и активируем запрет использования той самой неудачной системы приоритизации из первой итерации HTTP/2.
		vector <nghttp2_settings_entry> iv = {{NGHTTP2_SETTINGS_NO_RFC7540_PRIORITIES, 1}};
		// Определяем идентификатор сервиса
		switch(static_cast <uint8_t> (mode)){
			// Если сервис идентифицирован как клиент
			case static_cast <uint8_t> (mode_t::CLIENT): {
				// Создаём объект опции
				nghttp2_option * option;
				// Выпделяем память под объект опции
				nghttp2_option_new(&option);
				/**
				 * Убеждаемся, что закрытые соединения не сохраняются и не занимают память.
				 * Обратите внимание, что это нарушает дерево приоритетов, которое мы не используем.
				 */
				nghttp2_option_set_no_closed_streams(option, 1);
				/**
				 * Мы вручную обрабатываем управление потоком внутри сеанса, чтобы реализовать противодавление,
				 * то есть мы отправляем кадры WINDOW_UPDATE удаленному узлу только тогда, когда данные фактически используются пользовательским кодом.
				 * Это гарантирует, что поток данных по соединению не будет перемещаться слишком быстро, и ограничит объем данных, которые нам необходимо буферизовать.
				 */
				nghttp2_option_set_no_auto_window_update(option, 1);
				// Выполняем перебор полученных настроек
				for(auto & item : settings){
					// Определяем код параметра настроек
					switch(static_cast <uint8_t> (item.first)){
						// Если разрешено передавать расширения ALTSVC
						case static_cast <uint8_t> (settings_t::ENABLE_ALTSVC):
							// Если параметр активирован в настройках
							if(item.second > 0)
								// Выполняем установку зарешения использования расширения ALTSVC
								nghttp2_option_set_builtin_recv_extension_type(option, NGHTTP2_ALTSVC);
						break;
						// Если активированно разрешенение на передачу расширения ORIGIN
						case static_cast <uint8_t> (settings_t::ENABLE_ORIGIN):
							// Если параметр активирован в настройках
							if(item.second > 0)
								// Выполняем установку зарешения использования расширения ORIGIN
								nghttp2_option_set_builtin_recv_extension_type(option, NGHTTP2_ORIGIN);
						break;
						// Если мы получили разрешение присылать push-уведомления
						case static_cast <uint8_t> (settings_t::ENABLE_PUSH):
							// Устанавливаем разрешение присылать push-уведомления
							iv.push_back({NGHTTP2_SETTINGS_ENABLE_PUSH, item.second});
						break;
						// Если мы получили максимальный размер фрейма
						case static_cast <uint8_t> (settings_t::FRAME_SIZE):
							/**
							 * Устанавливаем максимальный размер кадра в октетах, который собеседнику разрешено отправлять.
							 * Значение по умолчанию, оно же минимальное — 16 384=214 октетов.
							 */
							iv.push_back({NGHTTP2_SETTINGS_MAX_FRAME_SIZE, item.second});
						break;
						// Если мы получили максимальный размер таблицы заголовков
						case static_cast <uint8_t> (settings_t::HEADER_TABLE_SIZE):
							// Устанавливаем максимальный размер таблицы заголовков
							iv.push_back({NGHTTP2_SETTINGS_HEADER_TABLE_SIZE, item.second});
						break;
						// Если мы получили максимальный размер окна полезной нагрузки
						case static_cast <uint8_t> (settings_t::WINDOW_SIZE):
							// Устанавливаем элемент управления потоком (flow control)
							iv.push_back({NGHTTP2_SETTINGS_INITIAL_WINDOW_SIZE, item.second});
						break;
						// Если настройки соответствуют количествам доступных потоков
						case static_cast <uint8_t> (settings_t::STREAMS): {
							// Выполняем установку количество потоков разрешенных использовать в подключении
							nghttp2_option_set_peer_max_concurrent_streams(option, item.second);
							/**
							 * Устанавливаем максимальное количество потоков, которое собеседнику разрешается использовать одновременно.
							 * Считаются только открытые потоки, то есть по которым что-то ещё передаётся.
							 * Можно указать значение 0: тогда собеседник не сможет отправлять новые сообщения, пока сторона его снова не увеличит.
							 */
							iv.push_back({NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, item.second});
						} break;
					}
				}
				// Выполняем создание клиента
				// nghttp2_session_client_new(&this->_session, callback, this);
				// Выполняем создание клиента
				nghttp2_session_client_new2(&this->_session, callback, this, option);
				// Выполняем удаление памяти объекта опции
				nghttp2_option_del(option);
			} break;
			// Если сервис идентифицирован как сервер
			case static_cast <uint8_t> (mode_t::SERVER): {
				// Создаём объект опции
				nghttp2_option * option;
				// Выпделяем память под объект опции
				nghttp2_option_new(&option);
				/**
				 * Убеждаемся, что закрытые соединения не сохраняются и не занимают память.
				 * Обратите внимание, что это нарушает дерево приоритетов, которое мы не используем.
				 */
				nghttp2_option_set_no_closed_streams(option, 1);
				/**
				 * Мы вручную обрабатываем управление потоком внутри сеанса, чтобы реализовать противодавление,
				 * то есть мы отправляем кадры WINDOW_UPDATE удаленному узлу только тогда, когда данные фактически используются пользовательским кодом.
				 * Это гарантирует, что поток данных по соединению не будет перемещаться слишком быстро, и ограничит объем данных, которые нам необходимо буферизовать.
				 */
				nghttp2_option_set_no_auto_window_update(option, 1);
				// Выполняем переход по всему списку настроек
				for(auto & item : settings){
					// Определяем тип настройки
					switch(static_cast <uint8_t> (item.first)){
						// Если мы получили разрешение присылать push-уведомления
						case static_cast <uint8_t> (settings_t::ENABLE_PUSH):
							// Устанавливаем разрешение присылать push-уведомления
							iv.push_back({NGHTTP2_SETTINGS_ENABLE_PUSH, item.second});
						break;
						// Если мы получили максимальный размер фрейма
						case static_cast <uint8_t> (settings_t::FRAME_SIZE):
							/**
							 * Устанавливаем максимальный размер кадра в октетах, который собеседнику разрешено отправлять.
							 * Значение по умолчанию, оно же минимальное — 16 384=214 октетов.
							 */
							iv.push_back({NGHTTP2_SETTINGS_MAX_FRAME_SIZE, item.second});
						break;
						// Если мы получили максимальный размер таблицы заголовков
						case static_cast <uint8_t> (settings_t::HEADER_TABLE_SIZE):
							// Устанавливаем максимальный размер таблицы заголовков
							iv.push_back({NGHTTP2_SETTINGS_HEADER_TABLE_SIZE, item.second});
						break;
						// Если мы получили максимальный размер окна полезной нагрузки
						case static_cast <uint8_t> (settings_t::WINDOW_SIZE):
							// Устанавливаем элемент управления потоком (flow control)
							iv.push_back({NGHTTP2_SETTINGS_INITIAL_WINDOW_SIZE, item.second});
						break;
						// Если мы получили максимальное количество потоков
						case static_cast <uint8_t> (settings_t::STREAMS):
							/**
							 * Устанавливаем максимальное количество потоков, которое собеседнику разрешается использовать одновременно.
							 * Считаются только открытые потоки, то есть по которым что-то ещё передаётся.
							 * Можно указать значение 0: тогда собеседник не сможет отправлять новые сообщения, пока сторона его снова не увеличит.
							 */
							iv.push_back({NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, item.second});
						break;
						// Если мы получили разрешение использования метода CONNECT
						case static_cast <uint8_t> (settings_t::CONNECT):
							// Устанавливаем разрешение применения метода CONNECT
							iv.push_back({NGHTTP2_SETTINGS_ENABLE_CONNECT_PROTOCOL, item.second});
						break;
					}
				}
				// Выполняем создание сервера
				nghttp2_session_server_new2(&this->_session, callback, this, option);
				// Выполняем удаление памяти объекта опции
				nghttp2_option_del(option);
			} break;
		}
		// Выполняем удаление объекта функций обратного вызова
		nghttp2_session_callbacks_del(callback);
		// Выполняем поиск максимального размера буфера полезной нагрузки
		auto i = settings.find(settings_t::PAYLOAD_SIZE);
		// Если параметр настроек максимального размера буфера полезной нагрузки установлен
		if(i != settings.end()){
			// Если максимальный размер буфера полезной нагрузки передан
			if(i->second > 0){
				// Выполняем установку максимального размера полезной нагрузки
				const int32_t rv = nghttp2_session_set_local_window_size(this->_session, NGHTTP2_FLAG_NONE, 0, i->second);
				// Если настройки для сессии установить не удалось
				if(!(result = !nghttp2_is_fatal(rv))){
					// Выводим сообщение об ошибке
					this->_log->print("Could not set PAYLOAD SIZE: %s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
					// Если функция обратного вызова на на вывод ошибок установлена
					if(this->_callback.is("error"))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_SETTINGS, this->_fmk->format("Could not set PAYLOAD SIZE: %s", nghttp2_strerror(rv)));
					// Выполняем очистку предыдущей сессии
					this->free();
					// Выходим из функции
					return result;
				}
			}
		}
		// Если список параметров настроек не пустой
		if(!iv.empty()){
			// Клиентская 24-байтовая магическая строка будет отправлена библиотекой nghttp2
			const int32_t rv = nghttp2_submit_settings(this->_session, NGHTTP2_FLAG_NONE, iv.data(), iv.size());
			// Если настройки для сессии установить не удалось
			if(!(result = !nghttp2_is_fatal(rv))){
				// Выводим сообщение об ошибке
				this->_log->print("Could not submit SETTINGS: %s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_SETTINGS, this->_fmk->format("Could not submit SETTINGS: %s", nghttp2_strerror(rv)));
				// Выполняем очистку предыдущей сессии
				this->free();
			}
		// Если список параметров настроек пустой
		} else {
			// Выводим сообщение об ошибке
			this->_log->print("SETTINGS list is empty: %s", log_t::flag_t::CRITICAL);
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_SETTINGS, "SETTINGS list is empty");
			// Выполняем очистку предыдущей сессии
			this->free();
		}
	}
	// Выводим результат
	return result;
}
/**
 * Оператор [=] зануления фрейма Http2
 * @return сформированный объект Http2
 */
awh::Http2 & awh::Http2::operator = (nullptr_t) noexcept {
	// Выполняем копирование сессии подключения
	this->_session = nullptr;
	// Выводим текущий объект в качестве результата
	return (* this);
}
/**
 * Оператор [=] копирования объекта фрейма Http2
 * @param ctx объект фрейма Http2
 * @return    сформированный объект Http2
 */
awh::Http2 & awh::Http2::operator = (const http2_t & ctx) noexcept {
	// Выполняем копирование сессии подключения
	this->_session = ctx._session;
	// Выводим текущий объект в качестве результата
	return (* this);
}
/**
 * ~Http2 Деструктор
 */
awh::Http2::~Http2() noexcept {
	// Если сессия создана удачно
	if(this->_session != nullptr)
		// Выполняем удаление сессии
		nghttp2_session_del(this->_session);
}
