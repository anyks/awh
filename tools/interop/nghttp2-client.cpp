/**
 * @file: nghttp2-client.cpp
 * @date: 2026-07-27
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

/**
 * Пробник: сквозная сессия нашего клиентского парсера против сервера nghttp2.
 * Проверяются запрос с телом, ответ с телом и трейлерами, а также приём
 * анонсированного сервером push-потока
 */

#include <map>
#include <string>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>

#include <nghttp2/nghttp2.h>

#include <proto/http/parser/http2/http.hpp>

using namespace awh;
using namespace awh::http;

// Тело ответа сервера nghttp2
static std::string responseBody;
// Тело push-ответа сервера nghttp2
static std::string pushBody;
// Позиции отдачи тел по идентификаторам потоков
static std::map <int32_t, size_t> offsets;

/**
 * @brief Структура состояния проверки
 *
 */
typedef struct State {
	// Статус-коды ответов по потокам
	std::map <uint32_t, std::string> status;
	// Тела ответов по потокам
	std::map <uint32_t, std::string> bodies;
	// Значение полученного трейлера
	std::string trailer;
	// Идентификатор анонсированного push-потока
	uint32_t promised = 0;
	// Путь анонсированного push-запроса
	std::string promisedPath;
	// Признак ошибки уровня соединения нашего клиента
	bool failed = false;
	// Описание ошибки нашего клиента
	std::string error;
	// Байты от нашего клиента к серверу nghttp2
	std::string toServer;
	// Байты от сервера nghttp2 к нашему клиенту
	std::string toClient;
} state_t;

/**
 * @brief Функция выдачи тела ответа сервером nghttp2
 *
 */
static ssize_t readBody(nghttp2_session * session, int32_t sid, uint8_t * buf, size_t length, uint32_t * flags, nghttp2_data_source * source, void *) noexcept {
	// Получаем отдаваемое тело
	const std::string & body = (* static_cast <const std::string *> (source->ptr));
	// Получаем текущую позицию отдачи тела
	size_t & offset = ::offsets[sid];
	// Вычисляем размер отдаваемой порции тела
	const size_t chunk = ((body.size() - offset) < length ? (body.size() - offset) : length);
	// Копируем порцию тела в буфер nghttp2
	::memcpy(buf, body.data() + offset, chunk);
	// Сдвигаем позицию отдачи тела
	offset += chunk;
	// Если тело отдано полностью
	if(offset >= body.size()){
		// Помечаем завершение потока данных
		(* flags) |= NGHTTP2_DATA_FLAG_EOF;
		// Если отдавалось тело основного ответа - завершаем поток секцией трейлеров
		if(&body == &::responseBody){
			// Помечаем что поток завершат трейлеры, а не данные
			(* flags) |= NGHTTP2_DATA_FLAG_NO_END_STREAM;
			// Формируем секцию трейлеров
			const nghttp2_nv trailers[] = {
				{(uint8_t *) "x-checksum", (uint8_t *) "0xCAFEBABE", 10, 10, NGHTTP2_NV_FLAG_NONE}
			};
			// Отправляем секцию трейлеров
			::nghttp2_submit_trailer(session, sid, trailers, 1);
		}
	}
	// Выводим размер отданной порции
	return static_cast <ssize_t> (chunk);
}

/**
 * @brief Функция обработки принятого сервером nghttp2 кадра
 *
 */
static int onFrame(nghttp2_session * session, const nghttp2_frame * frame, void *) noexcept {
	// Если получен блок заголовков запроса с завершением потока
	if((frame->hd.type == NGHTTP2_HEADERS) && (frame->headers.cat == NGHTTP2_HCAT_REQUEST))
		// Ожидаем завершения приёма запроса
		return 0;
	// Если получен кадр данных с завершением потока либо заголовки завершили поток
	if(((frame->hd.flags & NGHTTP2_FLAG_END_STREAM) != 0) && (frame->hd.stream_id != 0)){
		// Заголовки анонсируемого push-запроса
		const nghttp2_nv promise[] = {
			{(uint8_t *) ":method", (uint8_t *) "GET", 7, 3, NGHTTP2_NV_FLAG_NONE},
			{(uint8_t *) ":scheme", (uint8_t *) "https", 7, 5, NGHTTP2_NV_FLAG_NONE},
			{(uint8_t *) ":path", (uint8_t *) "/style.css", 5, 10, NGHTTP2_NV_FLAG_NONE},
			{(uint8_t *) ":authority", (uint8_t *) "example.com", 10, 11, NGHTTP2_NV_FLAG_NONE}
		};
		// Анонсируем push-поток
		const int32_t pushed = ::nghttp2_submit_push_promise(session, NGHTTP2_FLAG_NONE, frame->hd.stream_id, promise, 4, nullptr);
		// Если push-поток анонсирован
		if(pushed > 0){
			// Заголовки push-ответа
			const nghttp2_nv head[] = {
				{(uint8_t *) ":status", (uint8_t *) "200", 7, 3, NGHTTP2_NV_FLAG_NONE},
				{(uint8_t *) "content-type", (uint8_t *) "text/css", 12, 8, NGHTTP2_NV_FLAG_NONE}
			};
			// Объект источника тела push-ответа
			nghttp2_data_provider provider;
			// Устанавливаем отдаваемое тело
			provider.source.ptr = &::pushBody;
			// Устанавливаем функцию выдачи тела
			provider.read_callback = ::readBody;
			// Отправляем push-ответ
			::nghttp2_submit_response(session, pushed, head, 2, &provider);
		}
		// Заголовки основного ответа
		const nghttp2_nv head[] = {
			{(uint8_t *) ":status", (uint8_t *) "200", 7, 3, NGHTTP2_NV_FLAG_NONE},
			{(uint8_t *) "content-type", (uint8_t *) "text/html", 12, 9, NGHTTP2_NV_FLAG_NONE},
			{(uint8_t *) "trailer", (uint8_t *) "x-checksum", 7, 10, NGHTTP2_NV_FLAG_NONE}
		};
		// Объект источника тела основного ответа
		nghttp2_data_provider provider;
		// Устанавливаем отдаваемое тело
		provider.source.ptr = &::responseBody;
		// Устанавливаем функцию выдачи тела
		provider.read_callback = ::readBody;
		// Отправляем основной ответ
		::nghttp2_submit_response(session, frame->hd.stream_id, head, 3, &provider);
	}
	// Продолжаем разбор
	return 0;
}

int main(){
	// Объект фреймворка
	fmk_t fmk;
	// Объект логов
	log_t log(&fmk);
	// Отключаем вывод логов
	log.level(log_t::level_t::NONE);
	// Объект состояния проверки
	state_t state;
	// Формируем тело ответа заведомо больше окна управления потоком
	::responseBody.assign(180000, '\0');
	/**
	 * Выполняем заполнение тела ответа псевдослучайным паттерном
	 */
	for(size_t i = 0; i < ::responseBody.size(); i++)
		// Заполняем очередной байт тела ответа
		::responseBody[i] = static_cast <char> ((i * 13 + 5) & 0xFF);
	// Формируем тело push-ответа
	::pushBody.assign(4096, '\0');
	/**
	 * Выполняем заполнение тела push-ответа псевдослучайным паттерном
	 */
	for(size_t i = 0; i < ::pushBody.size(); i++)
		// Заполняем очередной байт тела push-ответа
		::pushBody[i] = static_cast <char> ((i * 7 + 11) & 0xFF);
	// Тело запроса нашего клиента
	std::string requestBody(120000, '\0');
	/**
	 * Выполняем заполнение тела запроса псевдослучайным паттерном
	 */
	for(size_t i = 0; i < requestBody.size(); i++)
		// Заполняем очередной байт тела запроса
		requestBody[i] = static_cast <char> ((i * 23 + 3) & 0xFF);
	// Создаём объект парсера нашего клиента
	parser_http2_t client(direct_t::RESPONSE, &fmk, &log);
	// Устанавливаем функцию обратного вызова записи исходящих байт
	client.on(parser_http2_t::write_callback_t([&](const void * buffer, const size_t size) noexcept {
		// Накапливаем байты для сервера nghttp2
		state.toServer.append(static_cast <const char *> (buffer), size);
	}));
	// Устанавливаем функцию обратного вызова заголовков
	client.on(parser_http2_t::header_callback_t([&](const uint32_t sid, const std::string_view name, const std::string_view value, const parser_t::part_t) noexcept -> bool {
		// Если получен псевдо-заголовок статуса ответа
		if(name.compare(":status") == 0)
			// Запоминаем статус-код ответа потока
			state.status[sid] = std::string(value);
		// Если получен трейлер контрольной суммы
		else if(name.compare("x-checksum") == 0)
			// Запоминаем значение трейлера
			state.trailer = std::string(value);
		// Если получен псевдо-заголовок пути анонсированного запроса
		else if(name.compare(":path") == 0)
			// Запоминаем путь анонсированного запроса
			state.promisedPath = std::string(value);
		// Продолжаем разбор
		return true;
	}));
	// Устанавливаем функцию обратного вызова тела ответа
	client.on(parser_http2_t::data_callback_t([&](const uint32_t sid, const void * buffer, const size_t size, const bool) noexcept -> bool {
		// Накапливаем тело ответа потока
		state.bodies[sid].append(static_cast <const char *> (buffer), size);
		// Продолжаем разбор
		return true;
	}));
	// Устанавливаем функцию обратного вызова анонса server push
	client.on(parser_http2_t::push_callback_t([&](const uint32_t, const uint32_t promised) noexcept -> bool {
		// Запоминаем идентификатор анонсированного push-потока
		state.promised = promised;
		// Принимаем push
		return true;
	}));
	// Устанавливаем функцию обратного вызова ошибки уровня соединения
	client.on(parser_http2_t::error_callback_t([&](const parser_http2_t::error_t, const std::string_view message) noexcept {
		// Помечаем ошибку уровня соединения
		state.failed = true;
		// Запоминаем описание ошибки
		state.error = std::string(message);
	}));
	// Отправляем preface нашего клиента
	client.sendPreface();
	// Выделяем идентификатор нового потока клиента
	const uint32_t sid = client.nextStreamId();
	// Формируем заголовки запроса
	std::vector <h2::hpack::field_t> fields;
	// Наполняем заголовки запроса
	fields.emplace_back(":method", "POST");
	fields.emplace_back(":scheme", "https");
	fields.emplace_back(":path", "/index.html");
	fields.emplace_back(":authority", "example.com");
	fields.emplace_back("content-length", std::to_string(requestBody.size()));
	// Отправляем заголовки запроса
	client.sendHeaders(sid, fields, false);
	// Текущая позиция отправки тела запроса
	size_t offset = 0;
	/**
	 * Отправляем тело запроса порциями, пока парсер их принимает
	 */
	while(offset < requestBody.size()){
		// Передаём очередную порцию тела запроса
		const size_t taken = client.sendData(sid, requestBody.data() + offset, requestBody.size() - offset, true);
		// Если порция не принята - прекращаем отправку
		if(taken == 0)
			// Прекращаем отправку тела запроса
			break;
		// Сдвигаем позицию отправки тела запроса
		offset += taken;
	}
	// Объект набора функций обратного вызова nghttp2
	nghttp2_session_callbacks * callbacks = nullptr;
	// Создаём объект набора функций обратного вызова nghttp2
	::nghttp2_session_callbacks_new(&callbacks);
	// Устанавливаем функцию обработки принятого кадра
	::nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, ::onFrame);
	// Объект сессии сервера nghttp2
	nghttp2_session * session = nullptr;
	// Создаём объект сессии сервера nghttp2
	::nghttp2_session_server_new(&session, callbacks, &state);
	// Удаляем объект набора функций обратного вызова nghttp2
	::nghttp2_session_callbacks_del(callbacks);
	// Отправляем SETTINGS сервера nghttp2
	::nghttp2_submit_settings(session, NGHTTP2_FLAG_NONE, nullptr, 0);
	/**
	 * Выполняем обмен байтами между нашим клиентом и сервером nghttp2
	 */
	for(size_t round = 0; round < 20000; round++){
		// Признак наличия обмена в текущем раунде
		bool progress = false;
		// Указатель на исходящие байты сервера nghttp2
		const uint8_t * out = nullptr;
		/**
		 * Забираем все исходящие байты сервера nghttp2
		 */
		for(;;){
			// Получаем очередную порцию исходящих байт сервера
			const ssize_t length = ::nghttp2_session_mem_send(session, &out);
			// Если порция пуста - прекращаем выборку
			if(length <= 0)
				// Прекращаем выборку исходящих байт
				break;
			// Накапливаем байты для нашего клиента
			state.toClient.append(reinterpret_cast <const char *> (out), static_cast <size_t> (length));
			// Помечаем наличие обмена
			progress = true;
		}
		// Если для нашего клиента есть входящие байты
		if(!state.toClient.empty()){
			// Забираем накопленные байты
			const std::string input = state.toClient;
			// Очищаем накопитель
			state.toClient.clear();
			// Подаём байты на разбор нашему клиенту
			client.parse(input.data(), input.size());
			// Помечаем наличие обмена
			progress = true;
		}
		// Если для сервера nghttp2 есть входящие байты
		if(!state.toServer.empty()){
			// Забираем накопленные байты
			const std::string input = state.toServer;
			// Очищаем накопитель
			state.toServer.clear();
			// Подаём байты на разбор серверу nghttp2
			const ssize_t used = ::nghttp2_session_mem_recv(session, reinterpret_cast <const uint8_t *> (input.data()), input.size());
			// Если разбор завершился ошибкой
			if(used < 0){
				// Печатаем описание ошибки сервера nghttp2
				std::cout << "сервер nghttp2 отверг поток байт: " << ::nghttp2_strerror(static_cast <int> (used)) << std::endl;
				// Прекращаем обмен
				break;
			}
			// Помечаем наличие обмена
			progress = true;
		}
		// Если обмена не было - сессия завершена
		if(!progress)
			// Прекращаем обмен
			break;
	}
	// Удаляем объект сессии сервера nghttp2
	::nghttp2_session_del(session);
	// Количество обнаруженных расхождений
	size_t failures = 0;
	// Печатаем статус основного ответа
	std::cout << "статус основного ответа: " << state.status[sid] << std::endl;
	// Если статус основного ответа не совпал
	if(state.status[sid].compare("200") != 0)
		// Наращиваем счётчик расхождений
		failures++;
	// Печатаем объём принятого тела основного ответа
	std::cout << "тело основного ответа: " << state.bodies[sid].size() << " из " << ::responseBody.size() << std::endl;
	// Если тело основного ответа принято с искажениями
	if(state.bodies[sid] != ::responseBody)
		// Наращиваем счётчик расхождений
		failures++;
	// Печатаем значение полученного трейлера
	std::cout << "трейлер: [" << state.trailer << "]" << std::endl;
	// Если трейлер не получен
	if(state.trailer.compare("0xCAFEBABE") != 0)
		// Наращиваем счётчик расхождений
		failures++;
	// Печатаем идентификатор и путь анонсированного push-потока
	std::cout << "push-поток: " << state.promised << " путь: " << state.promisedPath << std::endl;
	// Если push-поток не анонсирован
	if(state.promised == 0)
		// Наращиваем счётчик расхождений
		failures++;
	// Печатаем объём принятого тела push-ответа
	std::cout << "тело push-ответа: " << state.bodies[state.promised].size() << " из " << ::pushBody.size() << std::endl;
	// Если тело push-ответа принято с искажениями
	if(state.bodies[state.promised] != ::pushBody)
		// Наращиваем счётчик расхождений
		failures++;
	// Если зафиксирована ошибка уровня соединения
	if(state.failed){
		// Печатаем описание ошибки
		std::cout << "ошибка нашего клиента: " << state.error << std::endl;
		// Наращиваем счётчик расхождений
		failures++;
	}
	// Печатаем итог проверки
	std::cout << "расхождений: " << failures << std::endl;
	// Выводим результат
	return (failures == 0 ? 0 : 1);
}
