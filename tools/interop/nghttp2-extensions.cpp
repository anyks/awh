/**
 * @file: nghttp2-extensions.cpp
 * @date: 2026-07-27
 * @license: GPL-3.0
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
 * Пробник: сверка расширений и завершения соединения с эталонной реализацией.
 * Проверяются расширенный CONNECT (RFC 8441) в обе стороны, кадр PRIORITY_UPDATE
 * (RFC 9218) и реакция обеих сторон на GOAWAY с необработанными потоками
 */

#include <map>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>

#include <nghttp2/nghttp2.h>

#include <proto/http/parser/http2/http.hpp>

using namespace awh;
using namespace awh::http;

// Количество обнаруженных расхождений
static size_t failures = 0;

/**
 * @brief Функция фиксации расхождения
 *
 * @param text описание расхождения
 */
static void mismatch(const std::string & text) noexcept {
	// Наращиваем счётчик расхождений
	::failures++;
	// Печатаем описание расхождения
	std::cout << "  РАСХОЖДЕНИЕ: " << text << std::endl;
}

/**
 * @brief Структура состояния проверки расширенного CONNECT
 *
 */
typedef struct Tunnel {
	// Признак получения запроса сервером
	bool received = false;
	// Полученный сервером метод запроса
	std::string method;
	// Полученный сервером протокол туннеля
	std::string protocol;
	// Полученный сервером путь запроса
	std::string path;
	// Статус-код ответа, полученный клиентом
	std::string status;
	// Данные, принятые сервером из туннеля
	std::string upstream;
	// Данные, принятые клиентом из туннеля
	std::string downstream;
	// Байты от клиента к серверу
	std::string toServer;
	// Байты от сервера к клиенту
	std::string toClient;
	// Идентификатор потока туннеля на сервере
	uint32_t sid = 0;
} tunnel_t;

/**
 * @brief Функция обработки заголовка, принятого сервером nghttp2
 *
 */
static int onHeader(nghttp2_session *, const nghttp2_frame * frame, const uint8_t * name, size_t namelen, const uint8_t * value, size_t valuelen, uint8_t, void * data) noexcept {
	// Получаем объект состояния проверки
	tunnel_t * state = static_cast <tunnel_t *> (data);
	// Формируем название заголовка
	const std::string key(reinterpret_cast <const char *> (name), namelen);
	// Если получен псевдо-заголовок метода запроса
	if(key.compare(":method") == 0)
		// Запоминаем метод запроса
		state->method.assign(reinterpret_cast <const char *> (value), valuelen);
	// Если получен псевдо-заголовок протокола туннеля
	else if(key.compare(":protocol") == 0)
		// Запоминаем протокол туннеля
		state->protocol.assign(reinterpret_cast <const char *> (value), valuelen);
	// Если получен псевдо-заголовок пути запроса
	else if(key.compare(":path") == 0)
		// Запоминаем путь запроса
		state->path.assign(reinterpret_cast <const char *> (value), valuelen);
	// Не используемый параметр
	(void) frame;
	// Продолжаем разбор
	return 0;
}

/**
 * @brief Функция обработки кадра, принятого сервером nghttp2
 *
 */
static int onFrame(nghttp2_session * session, const nghttp2_frame * frame, void * data) noexcept {
	// Получаем объект состояния проверки
	tunnel_t * state = static_cast <tunnel_t *> (data);
	// Если получен блок заголовков запроса
	if((frame->hd.type == NGHTTP2_HEADERS) && (frame->headers.cat == NGHTTP2_HCAT_REQUEST)){
		// Помечаем что запрос получен
		state->received = true;
		// Запоминаем идентификатор потока туннеля
		state->sid = static_cast <uint32_t> (frame->hd.stream_id);
		// Формируем заголовки ответа сервера
		const nghttp2_nv head[] = {
			{(uint8_t *) ":status", (uint8_t *) "200", 7, 3, NGHTTP2_NV_FLAG_NONE}
		};
		// Отправляем ответ без завершения потока: туннель остаётся открытым
		::nghttp2_submit_response(session, frame->hd.stream_id, head, 1, nullptr);
	}
	// Продолжаем разбор
	return 0;
}

/**
 * @brief Функция обработки данных туннеля, принятых сервером nghttp2
 *
 */
static int onData(nghttp2_session *, uint8_t, int32_t, const uint8_t * data, size_t len, void * user) noexcept {
	// Получаем объект состояния проверки
	tunnel_t * state = static_cast <tunnel_t *> (user);
	// Накапливаем данные туннеля
	state->upstream.append(reinterpret_cast <const char *> (data), len);
	// Продолжаем разбор
	return 0;
}

/**
 * @brief Функция обмена байтами между нашим парсером и сессией nghttp2
 *
 * @param parser  объект нашего парсера
 * @param session объект сессии nghttp2
 * @param state   объект состояния проверки
 * @param toPeer  накопитель байт для сессии nghttp2
 * @param toOurs  накопитель байт для нашего парсера
 */
static void exchange(parser_http2_t & parser, nghttp2_session * session, std::string & toPeer, std::string & toOurs) noexcept {
	/**
	 * Выполняем обмен байтами, пока хоть одна сторона его продолжает
	 */
	for(size_t round = 0; round < 1000; round++){
		// Признак наличия обмена в текущем раунде
		bool progress = false;
		// Указатель на исходящие байты сессии nghttp2
		const uint8_t * out = nullptr;
		/**
		 * Забираем все исходящие байты сессии nghttp2
		 */
		for(;;){
			// Получаем очередную порцию исходящих байт сессии
			const ssize_t length = ::nghttp2_session_mem_send(session, &out);
			// Если порция пуста - прекращаем выборку
			if(length <= 0)
				// Прекращаем выборку исходящих байт
				break;
			// Накапливаем байты для нашего парсера
			toOurs.append(reinterpret_cast <const char *> (out), static_cast <size_t> (length));
			// Помечаем наличие обмена
			progress = true;
		}
		// Если для нашего парсера есть входящие байты
		if(!toOurs.empty()){
			// Забираем накопленные байты
			const std::string input = toOurs;
			// Очищаем накопитель
			toOurs.clear();
			// Подаём байты на разбор нашему парсеру
			parser.parse(input.data(), input.size());
			// Помечаем наличие обмена
			progress = true;
		}
		// Если для сессии nghttp2 есть входящие байты
		if(!toPeer.empty()){
			// Забираем накопленные байты
			const std::string input = toPeer;
			// Очищаем накопитель
			toPeer.clear();
			// Подаём байты на разбор сессии nghttp2
			if(::nghttp2_session_mem_recv(session, reinterpret_cast <const uint8_t *> (input.data()), input.size()) < 0)
				// Прекращаем обмен
				break;
			// Помечаем наличие обмена
			progress = true;
		}
		// Если обмена не было - сессия завершена
		if(!progress)
			// Прекращаем обмен
			break;
	}
}

/**
 * @brief Функция проверки расширенного CONNECT нашим клиентом (RFC 8441)
 *
 * @param fmk объект фреймворка
 * @param log объект логов
 */
static void checkConnect(const fmk_t * fmk, const log_t * log) noexcept {
	// Печатаем название проверки
	std::cout << "расширенный CONNECT: наш клиент против сервера nghttp2" << std::endl;
	// Объект состояния проверки
	tunnel_t state;
	// Создаём объект парсера нашего клиента
	parser_http2_t client(direct_t::RESPONSE, fmk, log);
	// Устанавливаем функцию обратного вызова записи исходящих байт
	client.on(parser_http2_t::write_callback_t([&](const void * buffer, const size_t size) noexcept {
		// Накапливаем байты для сессии nghttp2
		state.toServer.append(static_cast <const char *> (buffer), size);
	}));
	// Устанавливаем функцию обратного вызова заголовков
	client.on(parser_http2_t::header_callback_t([&](const uint32_t, const std::string_view name, const std::string_view value, const parser_t::part_t) noexcept -> bool {
		// Если получен псевдо-заголовок статуса ответа
		if(name.compare(":status") == 0)
			// Запоминаем статус-код ответа
			state.status = std::string(value);
		// Продолжаем разбор
		return true;
	}));
	// Устанавливаем функцию обратного вызова данных туннеля
	client.on(parser_http2_t::data_callback_t([&](const uint32_t, const void * buffer, const size_t size, const bool) noexcept -> bool {
		// Накапливаем данные туннеля
		state.downstream.append(static_cast <const char *> (buffer), size);
		// Продолжаем разбор
		return true;
	}));
	// Объект набора функций обратного вызова nghttp2
	nghttp2_session_callbacks * callbacks = nullptr;
	// Создаём объект набора функций обратного вызова nghttp2
	::nghttp2_session_callbacks_new(&callbacks);
	// Устанавливаем функцию обработки заголовка
	::nghttp2_session_callbacks_set_on_header_callback(callbacks, ::onHeader);
	// Устанавливаем функцию обработки кадра
	::nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, ::onFrame);
	// Устанавливаем функцию обработки данных туннеля
	::nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, ::onData);
	// Объект сессии сервера nghttp2
	nghttp2_session * session = nullptr;
	// Создаём объект сессии сервера nghttp2
	::nghttp2_session_server_new(&session, callbacks, &state);
	// Удаляем объект набора функций обратного вызова nghttp2
	::nghttp2_session_callbacks_del(callbacks);
	// Параметр разрешения расширенного CONNECT
	const nghttp2_settings_entry entry = {NGHTTP2_SETTINGS_ENABLE_CONNECT_PROTOCOL, 1};
	// Отправляем SETTINGS сервера nghttp2 с разрешением расширенного CONNECT
	::nghttp2_submit_settings(session, NGHTTP2_FLAG_NONE, &entry, 1);
	// Отправляем preface нашего клиента
	client.sendPreface();
	// Выполняем обмен байтами до получения параметров сервера
	::exchange(client, session, state.toServer, state.toClient);
	// Проверяем что разрешение расширенного CONNECT получено
	if(client.remoteSettings().enableConnectProtocol != 1)
		// Фиксируем расхождение
		::mismatch("разрешение расширенного CONNECT не принято от эталона");
	// Формируем провайдер запроса расширенного CONNECT
	auto request = std::make_unique <request_t> (version_t::HTTP2, method_t::CONNECT, "/chat");
	// Устанавливаем протокол поднимаемого туннеля
	request->protocol = "websocket";
	// Формируем контейнер заголовков запроса
	headers_t headers(std::move(request));
	// Дописываем заголовок авторитета запроса
	headers.emplace("Host", "example.com");
	// Выделяем идентификатор нового потока клиента
	const uint32_t sid = client.nextStreamId();
	// Отправляем запрос расширенного CONNECT (туннель остаётся открытым)
	client.sendHeaders(sid, headers, false);
	// Выполняем обмен байтами
	::exchange(client, session, state.toServer, state.toClient);
	// Проверяем что запрос принят эталонной реализацией
	if(!state.received)
		// Фиксируем расхождение
		::mismatch("эталон не принял запрос расширенного CONNECT");
	// Проверяем метод принятого запроса
	if(state.method.compare("CONNECT") != 0)
		// Фиксируем расхождение
		::mismatch("метод запроса принят как [" + state.method + "]");
	// Проверяем протокол туннеля
	if(state.protocol.compare("websocket") != 0)
		// Фиксируем расхождение
		::mismatch("протокол туннеля принят как [" + state.protocol + "]");
	// Проверяем путь запроса
	if(state.path.compare("/chat") != 0)
		// Фиксируем расхождение
		::mismatch("путь запроса принят как [" + state.path + "]");
	// Проверяем статус-код ответа
	if(state.status.compare("200") != 0)
		// Фиксируем расхождение
		::mismatch("статус ответа получен как [" + state.status + "]");
	// Отправляем данные в туннель
	client.sendData(sid, "ping over tunnel", 16, false);
	// Выполняем обмен байтами
	::exchange(client, session, state.toServer, state.toClient);
	// Проверяем что данные туннеля доставлены эталону
	if(state.upstream.compare("ping over tunnel") != 0)
		// Фиксируем расхождение
		::mismatch("данные туннеля доставлены как [" + state.upstream + "]");
	// Печатаем итог проверки
	std::cout << "  метод: " << state.method << ", протокол: " << state.protocol
	          << ", путь: " << state.path << ", статус: " << state.status
	          << ", туннель: " << state.upstream.size() << " байт" << std::endl;
	// Удаляем объект сессии сервера nghttp2
	::nghttp2_session_del(session);
}

/**
 * @brief Функция проверки реакции нашего клиента на GOAWAY эталона
 *
 * @param fmk объект фреймворка
 * @param log объект логов
 */
static void checkGoaway(const fmk_t * fmk, const log_t * log) noexcept {
	// Печатаем название проверки
	std::cout << "GOAWAY: наш клиент против сервера nghttp2" << std::endl;
	// Объект состояния проверки
	tunnel_t state;
	// События закрытия потоков нашего клиента
	std::map <uint32_t, uint32_t> closes;
	// Создаём объект парсера нашего клиента
	parser_http2_t client(direct_t::RESPONSE, fmk, log);
	// Устанавливаем функцию обратного вызова записи исходящих байт
	client.on(parser_http2_t::write_callback_t([&](const void * buffer, const size_t size) noexcept {
		// Накапливаем байты для сессии nghttp2
		state.toServer.append(static_cast <const char *> (buffer), size);
	}));
	// Устанавливаем функцию обратного вызова закрытия потока
	client.on(parser_http2_t::close_callback_t([&](const uint32_t sid, const parser_http2_t::error_t code) noexcept {
		// Собираем событие закрытия потока
		closes[sid] = static_cast <uint32_t> (code);
	}));
	// Объект набора функций обратного вызова nghttp2
	nghttp2_session_callbacks * callbacks = nullptr;
	// Создаём объект набора функций обратного вызова nghttp2
	::nghttp2_session_callbacks_new(&callbacks);
	// Объект сессии сервера nghttp2
	nghttp2_session * session = nullptr;
	// Создаём объект сессии сервера nghttp2
	::nghttp2_session_server_new(&session, callbacks, &state);
	// Удаляем объект набора функций обратного вызова nghttp2
	::nghttp2_session_callbacks_del(callbacks);
	// Отправляем SETTINGS сервера nghttp2
	::nghttp2_submit_settings(session, NGHTTP2_FLAG_NONE, nullptr, 0);
	// Отправляем preface нашего клиента
	client.sendPreface();
	// Выполняем обмен байтами
	::exchange(client, session, state.toServer, state.toClient);
	// Формируем заголовки запроса
	std::vector <h2::hpack::field_t> fields;
	// Наполняем заголовки запроса
	fields.emplace_back(":method", "GET");
	fields.emplace_back(":scheme", "https");
	fields.emplace_back(":path", "/");
	fields.emplace_back(":authority", "example.com");
	// Идентификаторы открытых потоков
	std::vector <uint32_t> ids;
	/**
	 * Открываем три потока подряд
	 */
	for(size_t i = 0; i < 3; i++){
		// Выделяем идентификатор очередного потока
		ids.push_back(client.nextStreamId());
		// Отправляем заголовки запроса с завершением потока
		client.sendHeaders(ids.back(), fields, true);
	}
	// Завершаем соединение со стороны эталона, объявив обработанным только первый поток
	::nghttp2_submit_goaway(session, NGHTTP2_FLAG_NONE, static_cast <int32_t> (ids.front()), NGHTTP2_NO_ERROR, nullptr, 0);
	// Выполняем обмен байтами
	::exchange(client, session, state.toServer, state.toClient);
	// Проверяем что необработанные потоки закрыты кодом отклонения
	for(size_t i = 1; i < ids.size(); i++){
		// Если поток не закрыт
		if(closes.find(ids[i]) == closes.end())
			// Фиксируем расхождение
			::mismatch("поток " + std::to_string(ids[i]) + " не закрыт по GOAWAY");
		// Если код закрытия не совпал с кодом отклонения
		else if(closes[ids[i]] != static_cast <uint32_t> (parser_http2_t::error_t::REFUSED_STREAM))
			// Фиксируем расхождение
			::mismatch("поток " + std::to_string(ids[i]) + " закрыт кодом " + std::to_string(closes[ids[i]]));
	}
	// Проверяем что соединение помечено на завершение
	if(!client.isClosed())
		// Фиксируем расхождение
		::mismatch("соединение не помечено на завершение после GOAWAY");
	// Печатаем итог проверки
	std::cout << "  закрыто потоков: " << closes.size() << ", соединение помечено на завершение: " << (client.isClosed() ? "да" : "нет") << std::endl;
	// Удаляем объект сессии сервера nghttp2
	::nghttp2_session_del(session);
}

/**
 * @brief Функция проверки кадра PRIORITY_UPDATE (RFC 9218)
 *
 * @param fmk объект фреймворка
 * @param log объект логов
 */
static void checkPriority(const fmk_t * fmk, const log_t * log) noexcept {
	// Печатаем название проверки
	std::cout << "PRIORITY_UPDATE: клиент nghttp2 против нашего сервера" << std::endl;
	// Объект состояния проверки
	tunnel_t state;
	// Признак получения запроса нашим сервером
	bool received = false;
	// Признак ошибки уровня соединения нашего сервера
	bool failed = false;
	// Создаём объект парсера нашего сервера
	parser_http2_t server(direct_t::REQUEST, fmk, log);
	// Устанавливаем функцию обратного вызова записи исходящих байт
	server.on(parser_http2_t::write_callback_t([&](const void * buffer, const size_t size) noexcept {
		// Накапливаем байты для сессии nghttp2
		state.toClient.append(static_cast <const char *> (buffer), size);
	}));
	// Устанавливаем функцию обратного вызова фазы приёма сообщения
	server.on(parser_http2_t::phase_callback_t([&](const uint32_t sid, const parser_t::phase_t phase, const parser_t::part_t part) noexcept -> bool {
		// Если приём запроса завершён полностью
		if((phase == parser_t::phase_t::END) && (part == parser_t::part_t::NONE)){
			// Помечаем что запрос получен
			received = true;
			// Формируем заголовки ответа сервера
			std::vector <h2::hpack::field_t> response;
			// Дописываем псевдо-заголовок статуса ответа
			response.emplace_back(":status", "200");
			// Отправляем ответ с завершением потока
			server.sendHeaders(sid, response, true);
		}
		// Продолжаем разбор
		return true;
	}));
	// Устанавливаем функцию обратного вызова ошибки уровня соединения
	server.on(parser_http2_t::error_callback_t([&](const parser_http2_t::error_t, const std::string_view message) noexcept {
		// Помечаем ошибку уровня соединения
		failed = true;
		// Печатаем описание ошибки
		std::cout << "  ошибка нашего сервера: " << message << std::endl;
	}));
	// Объект набора функций обратного вызова nghttp2
	nghttp2_session_callbacks * callbacks = nullptr;
	// Создаём объект набора функций обратного вызова nghttp2
	::nghttp2_session_callbacks_new(&callbacks);
	// Объект сессии клиента nghttp2
	nghttp2_session * session = nullptr;
	// Создаём объект сессии клиента nghttp2
	::nghttp2_session_client_new(&session, callbacks, &state);
	// Удаляем объект набора функций обратного вызова nghttp2
	::nghttp2_session_callbacks_del(callbacks);
	// Параметр отказа от приоритетов RFC 7540
	const nghttp2_settings_entry entry = {NGHTTP2_SETTINGS_NO_RFC7540_PRIORITIES, 1};
	// Отправляем SETTINGS клиента nghttp2
	::nghttp2_submit_settings(session, NGHTTP2_FLAG_NONE, &entry, 1);
	// Отправляем preface нашего сервера
	server.sendPreface();
	// Формируем заголовки запроса клиента nghttp2
	const nghttp2_nv nva[] = {
		{(uint8_t *) ":method", (uint8_t *) "GET", 7, 3, NGHTTP2_NV_FLAG_NONE},
		{(uint8_t *) ":scheme", (uint8_t *) "https", 7, 5, NGHTTP2_NV_FLAG_NONE},
		{(uint8_t *) ":path", (uint8_t *) "/", 5, 1, NGHTTP2_NV_FLAG_NONE},
		{(uint8_t *) ":authority", (uint8_t *) "example.com", 10, 11, NGHTTP2_NV_FLAG_NONE}
	};
	// Отправляем запрос клиента nghttp2
	const int32_t sid = ::nghttp2_submit_request(session, nullptr, nva, 4, nullptr, nullptr);
	// Отправляем кадр обновления приоритета потока
	::nghttp2_submit_priority_update(session, NGHTTP2_FLAG_NONE, sid, reinterpret_cast <const uint8_t *> ("u=2, i"), 6);
	// Выполняем обмен байтами
	::exchange(server, session, state.toClient, state.toServer);
	// Проверяем что запрос принят нашим сервером
	if(!received)
		// Фиксируем расхождение
		::mismatch("наш сервер не принял запрос с кадром обновления приоритета");
	// Проверяем что соединение не оборвано
	if(failed)
		// Фиксируем расхождение
		::mismatch("наш сервер оборвал соединение на кадре обновления приоритета");
	// Печатаем итог проверки
	std::cout << "  запрос принят: " << (received ? "да" : "нет") << ", соединение живо: " << (failed ? "нет" : "да") << std::endl;
	// Удаляем объект сессии клиента nghttp2
	::nghttp2_session_del(session);
}

/**
 * @brief Функция проверки расширенного CONNECT нашим сервером (RFC 8441)
 *
 * @param fmk объект фреймворка
 * @param log объект логов
 */
static void checkConnectServer(const fmk_t * fmk, const log_t * log) noexcept {
	// Печатаем название проверки
	std::cout << "расширенный CONNECT: клиент nghttp2 против нашего сервера" << std::endl;
	// Объект состояния проверки
	tunnel_t state;
	// Признак получения запроса нашим сервером
	bool received = false;
	// Протокол туннеля, принятый нашим сервером
	std::string protocol;
	// Метод запроса, принятый нашим сервером
	method_t method = method_t::NONE;
	// Создаём объект парсера нашего сервера
	parser_http2_t server(direct_t::REQUEST, fmk, log);
	// Получаем параметры SETTINGS нашего сервера
	auto settings = server.settings();
	// Разрешаем расширенный метод CONNECT
	settings.enableConnectProtocol = 1;
	// Применяем параметры SETTINGS нашего сервера
	server.settings(settings);
	// Устанавливаем функцию обратного вызова записи исходящих байт
	server.on(parser_http2_t::write_callback_t([&](const void * buffer, const size_t size) noexcept {
		// Накапливаем байты для сессии nghttp2
		state.toClient.append(static_cast <const char *> (buffer), size);
	}));
	// Устанавливаем функцию обратного вызова заголовков
	server.on(parser_http2_t::header_callback_t([&](const uint32_t, const std::string_view name, const std::string_view value, const parser_t::part_t) noexcept -> bool {
		// Если получен псевдо-заголовок протокола туннеля
		if(name.compare(":protocol") == 0)
			// Запоминаем протокол туннеля
			protocol = std::string(value);
		// Продолжаем разбор
		return true;
	}));
	// Устанавливаем функцию обратного вызова провайдера заголовков
	server.on(parser_http2_t::provider_callback_t([&](const uint32_t sid, const provider_t * provider, const bool) noexcept -> bool {
		// Если получен провайдер запроса
		if((provider != nullptr) && (provider->direct == direct_t::REQUEST)){
			// Помечаем что запрос получен
			received = true;
			// Запоминаем метод запроса
			method = static_cast <const request_t *> (provider)->method;
			// Формируем заголовки ответа сервера
			std::vector <h2::hpack::field_t> response;
			// Дописываем псевдо-заголовок статуса ответа
			response.emplace_back(":status", "200");
			// Отправляем ответ без завершения потока: туннель остаётся открытым
			server.sendHeaders(sid, response, false);
			// Отправляем данные в туннель
			server.sendData(sid, "pong over tunnel", 16, false);
		}
		// Продолжаем разбор
		return true;
	}));
	// Объект набора функций обратного вызова nghttp2
	nghttp2_session_callbacks * callbacks = nullptr;
	// Создаём объект набора функций обратного вызова nghttp2
	::nghttp2_session_callbacks_new(&callbacks);
	// Устанавливаем функцию обработки данных туннеля
	::nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, ::onData);
	// Объект сессии клиента nghttp2
	nghttp2_session * session = nullptr;
	// Создаём объект сессии клиента nghttp2
	::nghttp2_session_client_new(&session, callbacks, &state);
	// Удаляем объект набора функций обратного вызова nghttp2
	::nghttp2_session_callbacks_del(callbacks);
	// Отправляем SETTINGS клиента nghttp2
	::nghttp2_submit_settings(session, NGHTTP2_FLAG_NONE, nullptr, 0);
	// Отправляем preface нашего сервера
	server.sendPreface();
	// Выполняем обмен байтами до получения параметров нашего сервера
	::exchange(server, session, state.toClient, state.toServer);
	// Формируем заголовки запроса расширенного CONNECT
	const nghttp2_nv nva[] = {
		{(uint8_t *) ":method", (uint8_t *) "CONNECT", 7, 7, NGHTTP2_NV_FLAG_NONE},
		{(uint8_t *) ":scheme", (uint8_t *) "https", 7, 5, NGHTTP2_NV_FLAG_NONE},
		{(uint8_t *) ":path", (uint8_t *) "/chat", 5, 5, NGHTTP2_NV_FLAG_NONE},
		{(uint8_t *) ":authority", (uint8_t *) "example.com", 10, 11, NGHTTP2_NV_FLAG_NONE},
		{(uint8_t *) ":protocol", (uint8_t *) "websocket", 9, 9, NGHTTP2_NV_FLAG_NONE}
	};
	// Отправляем запрос расширенного CONNECT (туннель остаётся открытым)
	const int32_t sid = ::nghttp2_submit_request(session, nullptr, nva, 5, nullptr, nullptr);
	// Если запрос отправить не удалось
	if(sid < 0)
		// Фиксируем расхождение
		::mismatch("эталон отказался отправлять расширенный CONNECT");
	// Выполняем обмен байтами
	::exchange(server, session, state.toClient, state.toServer);
	// Проверяем что запрос принят нашим сервером
	if(!received)
		// Фиксируем расхождение
		::mismatch("наш сервер не принял расширенный CONNECT от эталона");
	// Проверяем метод принятого запроса
	if(method != method_t::CONNECT)
		// Фиксируем расхождение
		::mismatch("метод запроса распознан неверно");
	// Проверяем протокол туннеля
	if(protocol.compare("websocket") != 0)
		// Фиксируем расхождение
		::mismatch("протокол туннеля принят как [" + protocol + "]");
	// Проверяем что данные туннеля доставлены эталону
	if(state.upstream.compare("pong over tunnel") != 0)
		// Фиксируем расхождение
		::mismatch("данные туннеля доставлены как [" + state.upstream + "]");
	// Печатаем итог проверки
	std::cout << "  запрос принят: " << (received ? "да" : "нет") << ", протокол: " << protocol
	          << ", туннель: " << state.upstream.size() << " байт" << std::endl;
	// Удаляем объект сессии клиента nghttp2
	::nghttp2_session_del(session);
}

int32_t main(){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Отключаем вывод логов
	log.level(log_t::level_t::NONE);
	// Выполняем проверку расширенного CONNECT нашим клиентом
	::checkConnect(&fmk, &log);
	// Выполняем проверку расширенного CONNECT нашим сервером
	::checkConnectServer(&fmk, &log);
	// Выполняем проверку реакции на GOAWAY
	::checkGoaway(&fmk, &log);
	// Выполняем проверку кадра обновления приоритета
	::checkPriority(&fmk, &log);
	// Печатаем итог сверки
	std::cout << "расхождений: " << ::failures << std::endl;
	// Выводим результат
	return ((::failures == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
