/**
 * @file: parser2.cpp
 * @date: 2026-07-19
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
 * Стандартные модули
 */
#include <string>
#include <memory>
#include <vector>
#include <cstring>
#include <iostream>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <proto/http/parser/http2/http.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;
/**
 * Используем пространство имён HTTP-протокола
 */
using namespace awh::http;

/**
 * @brief Функция подключения отладочного вывода событий парсера
 *
 * @param parser объект парсера
 * @param role   роль парсера для префикса вывода
 */
static void subscribe(parser_http2_t & parser, const string role) noexcept {
	// Устанавливаем функцию обратного вызова для обработки открытия нового потока
	parser.on(parser_http2_t::begin_callback_t([role](const uint32_t sid) noexcept -> bool {
		// Выводим событие открытия нового потока
		cout << "[" << role << "] Stream #" << sid << " opened" << endl;
		// Продолжаем разбор
		return true;
	}));
	// Устанавливаем функцию обратного вызова для обработки заголовков или трейлеров потока
	parser.on(parser_http2_t::header_callback_t([role](const uint32_t sid, const string_view name, const string_view value, const parser_t::part_t part) noexcept -> bool {
		// Выводим название и значение очередного заголовка
		cout << "[" << role << "] Stream #" << sid << (part == parser_t::part_t::TRAILER ? " trailer" : " header") << ": [" << name << "] = [" << value << "]" << endl;
		// Продолжаем разбор
		return true;
	}));
	// Устанавливаем функцию обратного вызова для обработки провайдера заголовков потока
	parser.on(parser_http2_t::provider_callback_t([role](const uint32_t sid, const provider_t * provider, const bool endStream) noexcept -> bool {
		// Если получены трейлеры - провайдер не собирается
		if(provider == nullptr)
			// Выводим событие завершения трейлеров
			cout << "[" << role << "] Stream #" << sid << " trailers complete" << endl;
		// Если получен провайдер запроса клиента
		else if(provider->direct == direct_t::REQUEST)
			// Выводим разобранные параметры запроса
			cout << "[" << role << "] Stream #" << sid << " request URI: " << static_cast <const request_t *> (provider)->uri << (endStream ? " (no body)" : "") << endl;
		// Если получен провайдер ответа сервера - выводим статус-код
		else cout << "[" << role << "] Stream #" << sid << " response code: " << static_cast <const response_t *> (provider)->code << (endStream ? " (no body)" : "") << endl;
		// Продолжаем разбор
		return true;
	}));
	// Устанавливаем функцию обратного вызова для обработки фрагмента тела потока
	parser.on(parser_http2_t::data_callback_t([role](const uint32_t sid, const void * buffer, const size_t size, const bool endStream) noexcept -> bool {
		// Если фрагмент тела небольшой - выводим его содержимое
		if(size <= 64)
			// Выводим фрагмент тела потока
			cout << "[" << role << "] Stream #" << sid << " body (" << size << " bytes): [" << string(static_cast <const char *> (buffer), size) << "]" << (endStream ? " <end>" : "") << endl;
		// Если фрагмент тела большой - выводим только размер (zero-copy, буфер действителен только на время вызова)
		else cout << "[" << role << "] Stream #" << sid << " body fragment: " << size << " bytes" << (endStream ? " <end>" : "") << endl;
		// Продолжаем разбор
		return true;
	}));
	// Устанавливаем функцию обратного вызова для обработки закрытия потока
	parser.on(parser_http2_t::close_callback_t([role](const uint32_t sid, const parser_http2_t::error_t code) noexcept {
		// Выводим событие закрытия потока
		cout << "[" << role << "] Stream #" << sid << " closed: " << parser_http2_t::errorName(code) << endl;
	}));
	// Устанавливаем функцию обратного вызова для обработки полученного GOAWAY
	parser.on(parser_http2_t::goaway_callback_t([role](const uint32_t last, const parser_http2_t::error_t code, const string_view debug) noexcept {
		// Выводим событие получения GOAWAY
		cout << "[" << role << "] GOAWAY received: " << parser_http2_t::errorName(code) << " (last stream #" << last << ", debug: \"" << debug << "\")" << endl;
	}));
	// Устанавливаем функцию обратного вызова для обработки ошибки уровня соединения
	parser.on(parser_http2_t::error_callback_t([role](const parser_http2_t::error_t code, const string_view message) noexcept {
		// Выводим событие ошибки уровня соединения
		cout << "[" << role << "] Connection error: " << parser_http2_t::errorName(code) << " (" << message << ")" << endl;
	}));
}
/**
 * @brief Демонстрация рукопожатия соединения (preface + обмен SETTINGS)
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
static void sampleHandshake(const fmk_t * fmk, const log_t * log) noexcept {
	// Печатаем заголовок демонстрации
	cout << " ======== HANDSHAKE ======== " << endl;
	/**
	 * HTTP/2 - двунаправленный протокол: парсер не только разбирает входящие
	 * фреймы, но и формирует обязательные исходящие (SETTINGS ACK, PING ACK,
	 * WINDOW_UPDATE...). Направление трафика задаёт роль:
	 * - direct_t::REQUEST  - разбираем запросы клиента (мы - сервер);
	 * - direct_t::RESPONSE - разбираем ответы сервера (мы - клиент)
	 */
	parser_http2_t server(direct_t::REQUEST, fmk, log);
	// Создаём объект парсера клиента
	parser_http2_t client(direct_t::RESPONSE, fmk, log);
	// Исходящие байты клиента подаём на разбор серверу (эмуляция сети)
	client.on(parser_http2_t::write_callback_t([&server](const void * buffer, const size_t size) noexcept {
		// Выполняем разбор исходящих байтов клиента на сервере
		server.parse(buffer, size);
	}));
	// Исходящие байты сервера подаём на разбор клиенту
	server.on(parser_http2_t::write_callback_t([&client](const void * buffer, const size_t size) noexcept {
		// Выполняем разбор исходящих байтов сервера на клиенте
		client.parse(buffer, size);
	}));
	// Устанавливаем функцию обратного вызова для обработки применённого SETTINGS пира
	server.on(parser_http2_t::settings_callback_t([&server]() noexcept {
		// Выводим применённые параметры SETTINGS клиента
		cout << "[server] Client SETTINGS applied: headerTableSize = " << server.remoteSettings().headerTableSize
			<< ", maxConcurrentStreams = " << server.remoteSettings().maxConcurrentStreams
			<< ", initialWindowSize = " << server.remoteSettings().initialWindowSize << endl;
	}));
	// Настраиваем собственные параметры SETTINGS клиента
	parser_http2_t::settings_t settings;
	// Устанавливаем размер динамической таблицы HPACK
	settings.headerTableSize = 8192;
	// Устанавливаем лимит одновременных потоков
	settings.maxConcurrentStreams = 64;
	// Применяем параметры SETTINGS клиента (отправятся с preface)
	client.settings(settings);
	// Клиент отправляет magic-строку и свой SETTINGS
	client.sendPreface();
	// Сервер отправляет свой SETTINGS
	server.sendPreface();
	// Выводим итоговый статус соединения (PARTIAL - соединение живо)
	cout << "Connection alive: " << (server.status() == parser_t::status_t::PARTIAL ? "yes" : "no") << endl << endl;
}
/**
 * @brief Демонстрация полного обмена запросом и ответом с телами
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
static void sampleExchange(const fmk_t * fmk, const log_t * log) noexcept {
	// Печатаем заголовок демонстрации
	cout << " ======== EXCHANGE ======== " << endl;
	// Создаём объект парсера сервера
	parser_http2_t server(direct_t::REQUEST, fmk, log);
	// Создаём объект парсера клиента
	parser_http2_t client(direct_t::RESPONSE, fmk, log);
	// Исходящие байты клиента подаём на разбор серверу
	client.on(parser_http2_t::write_callback_t([&server](const void * buffer, const size_t size) noexcept {
		// Выполняем разбор исходящих байтов клиента на сервере
		server.parse(buffer, size);
	}));
	// Исходящие байты сервера подаём на разбор клиенту
	server.on(parser_http2_t::write_callback_t([&client](const void * buffer, const size_t size) noexcept {
		// Выполняем разбор исходящих байтов сервера на клиенте
		client.parse(buffer, size);
	}));
	// Подключаем отладочный вывод событий парсеров
	subscribe(server, "server");
	// Подключаем отладочный вывод событий клиента
	subscribe(client, "client");
	// Выполняем рукопожатие соединения
	client.sendPreface();
	// Сервер отправляет свой SETTINGS
	server.sendPreface();
	// Выделяем идентификатор нового потока (клиент получает нечётные: 1, 3, 5...)
	const uint32_t sid = client.nextStreamId();
	/**
	 * Заголовки можно отправлять "сырым" списком полей HPACK: псевдо-заголовки
	 * (:method/:scheme/:path) обязаны идти первыми, названия - в нижнем регистре
	 */
	vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода запроса
	fields.emplace_back(":method", "POST");
	// Дописываем псевдо-заголовок схемы запроса
	fields.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути запроса
	fields.emplace_back(":path", "/api/echo");
	// Дописываем псевдо-заголовок авторитета запроса
	fields.emplace_back(":authority", "anyks.com");
	// Дописываем обычный заголовок
	fields.emplace_back("content-type", "text/plain");
	// Отправляем заголовки запроса (тело последует отдельно)
	client.sendHeaders(sid, fields, false);
	// Отправляем тело запроса с завершением потока
	client.sendData(sid, "Hello, HTTP/2!", 14, true);
	// Формируем заголовки ответа сервера
	vector <h2::hpack::field_t> response;
	// Дописываем псевдо-заголовок статуса ответа
	response.emplace_back(":status", "200");
	// Дописываем обычный заголовок
	response.emplace_back("content-type", "text/plain");
	// Отправляем заголовки ответа (тело последует отдельно)
	server.sendHeaders(sid, response, false);
	// Отправляем тело ответа с завершением потока
	server.sendData(sid, "Hello, client!", 14, true);
	// Выводим пустую строку-разделитель
	cout << endl;
}
/**
 * @brief Демонстрация отправки заголовков из контейнера headers_t
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
static void sampleContainer(const fmk_t * fmk, const log_t * log) noexcept {
	// Печатаем заголовок демонстрации
	cout << " ======== HEADERS CONTAINER ======== " << endl;
	// Создаём объект парсера сервера
	parser_http2_t server(direct_t::REQUEST, fmk, log);
	// Создаём объект парсера клиента
	parser_http2_t client(direct_t::RESPONSE, fmk, log);
	// Исходящие байты клиента подаём на разбор серверу
	client.on(parser_http2_t::write_callback_t([&server](const void * buffer, const size_t size) noexcept {
		// Выполняем разбор исходящих байтов клиента на сервере
		server.parse(buffer, size);
	}));
	// Исходящие байты сервера подаём на разбор клиенту
	server.on(parser_http2_t::write_callback_t([&client](const void * buffer, const size_t size) noexcept {
		// Выполняем разбор исходящих байтов сервера на клиенте
		client.parse(buffer, size);
	}));
	// Подключаем отладочный вывод событий парсеров
	subscribe(server, "server");
	// Подключаем отладочный вывод событий клиента
	subscribe(client, "client");
	// Выполняем рукопожатие соединения
	client.sendPreface();
	// Сервер отправляет свой SETTINGS
	server.sendPreface();
	// Выделяем идентификатор нового потока клиента
	const uint32_t sid = client.nextStreamId();
	/**
	 * Удобнее отправлять заголовки прямо из контейнера headers_t: псевдо-заголовки
	 * формируются из провайдера автоматически, Host конвертируется в [:authority],
	 * названия приводятся к нижнему регистру, а запрещённые в HTTP/2
	 * connection-specific заголовки (Connection, Keep-Alive...) выбрасываются
	 */
	headers_t request(make_unique <request_t> (version_t::HTTP2, method_t::GET, string("/index.html?lang=ru")));
	// Дописываем заголовок Host (конвертируется в псевдо-заголовок [:authority])
	request.emplace("Host", "anyks.com");
	// Дописываем заголовок в смешанном регистре (приводится к нижнему)
	request.emplace("User-Agent", "AWH/1.0");
	// Дописываем запрещённый в HTTP/2 заголовок (выброшен при отправке)
	request.emplace("Connection", "keep-alive");
	// Отправляем заголовки запроса из контейнера с завершением потока
	client.sendHeaders(sid, request, true);
	// Формируем контейнер заголовков ответа с провайдером
	headers_t response(make_unique <response_t> (version_t::HTTP2, static_cast <uint16_t> (200)));
	// Дописываем обычный заголовок ответа
	response.emplace("Content-Type", "text/html; charset=utf-8");
	// Дописываем заголовок сервера
	response.emplace("Server", "AWH");
	// Отправляем заголовки ответа из контейнера с завершением потока
	server.sendHeaders(sid, response, true);
	// Выводим пустую строку-разделитель
	cout << endl;
}
/**
 * @brief Демонстрация pull-источника данных тела и flow control
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
static void sampleDataSource(const fmk_t * fmk, const log_t * log) noexcept {
	// Печатаем заголовок демонстрации
	cout << " ======== DATA SOURCE (FLOW CONTROL) ======== " << endl;
	// Создаём объект парсера сервера
	parser_http2_t server(direct_t::REQUEST, fmk, log);
	// Создаём объект парсера клиента
	parser_http2_t client(direct_t::RESPONSE, fmk, log);
	// Исходящие байты клиента подаём на разбор серверу
	client.on(parser_http2_t::write_callback_t([&server](const void * buffer, const size_t size) noexcept {
		// Выполняем разбор исходящих байтов клиента на сервере
		server.parse(buffer, size);
	}));
	// Исходящие байты сервера подаём на разбор клиенту
	server.on(parser_http2_t::write_callback_t([&client](const void * buffer, const size_t size) noexcept {
		// Выполняем разбор исходящих байтов сервера на клиенте
		client.parse(buffer, size);
	}));
	// Счётчик принятых клиентом байт тела
	size_t received = 0;
	// Устанавливаем функцию обратного вызова для подсчёта тела ответа
	client.on(parser_http2_t::data_callback_t([&received](const uint32_t sid, const void * buffer, const size_t size, const bool endStream) noexcept -> bool {
		// Не используемые параметры
		(void) sid; (void) buffer;
		// Считаем принятые байты тела
		received += size;
		// Если поток завершён - выводим итог приёма
		if(endStream)
			// Выводим количество принятых байт тела
			cout << "[client] Body received: " << received << " bytes" << endl;
		// Продолжаем разбор
		return true;
	}));
	// Выполняем рукопожатие соединения
	client.sendPreface();
	// Сервер отправляет свой SETTINGS
	server.sendPreface();
	// Выделяем идентификатор нового потока клиента
	const uint32_t sid = client.nextStreamId();
	// Формируем заголовки запроса большого файла
	vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода запроса
	fields.emplace_back(":method", "GET");
	// Дописываем псевдо-заголовок схемы запроса
	fields.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути запроса
	fields.emplace_back(":path", "/download/big.bin");
	// Отправляем заголовки запроса с завершением потока
	client.sendHeaders(sid, fields, true);
	// Формируем заголовки ответа сервера
	vector <h2::hpack::field_t> response;
	// Дописываем псевдо-заголовок статуса ответа
	response.emplace_back(":status", "200");
	// Отправляем заголовки ответа (тело выдаст pull-источник)
	server.sendHeaders(sid, response, false);
	// Общий размер отдаваемого файла (больше окна flow control 65535)
	constexpr size_t TOTAL = 500000;
	// Счётчик выданных источником байт
	auto offset = make_shared <size_t> (0);
	// Счётчик вызовов источника
	auto calls = make_shared <size_t> (0);
	/**
	 * Pull-источник - альтернатива sendData для больших тел: парсер сам
	 * запрашивает данные ровно тогда, когда открыто окно flow control и есть
	 * место в выходном буфере - тело не копится в памяти целиком
	 */
	server.dataSource(sid, [offset, calls](const uint32_t id, uint8_t * buffer, const size_t cap, bool & eof) noexcept -> int64_t {
		// Не используемый параметр
		(void) id;
		// Считаем вызовы источника
		(* calls)++;
		// Вычисляем размер очередной порции
		const size_t chunk = ((TOTAL - (* offset)) < cap ? (TOTAL - (* offset)) : cap);
		// Заполняем буфер парсера данными файла
		::memset(buffer, 'x', chunk);
		// Смещаем счётчик выданных байт
		(* offset) += chunk;
		// Помечаем достижение конца тела
		eof = ((* offset) >= TOTAL);
		// Выводим число записанных байт
		return static_cast <int64_t> (chunk);
	});
	// Выводим статистику работы pull-источника
	cout << "[server] Data source drained: " << (* offset) << " bytes in " << (* calls) << " calls" << endl << endl;
}
/**
 * @brief Демонстрация server push (PUSH_PROMISE)
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
static void samplePush(const fmk_t * fmk, const log_t * log) noexcept {
	// Печатаем заголовок демонстрации
	cout << " ======== SERVER PUSH ======== " << endl;
	// Создаём объект парсера сервера
	parser_http2_t server(direct_t::REQUEST, fmk, log);
	// Создаём объект парсера клиента
	parser_http2_t client(direct_t::RESPONSE, fmk, log);
	// Исходящие байты клиента подаём на разбор серверу
	client.on(parser_http2_t::write_callback_t([&server](const void * buffer, const size_t size) noexcept {
		// Выполняем разбор исходящих байтов клиента на сервере
		server.parse(buffer, size);
	}));
	// Исходящие байты сервера подаём на разбор клиенту
	server.on(parser_http2_t::write_callback_t([&client](const void * buffer, const size_t size) noexcept {
		// Выполняем разбор исходящих байтов сервера на клиенте
		client.parse(buffer, size);
	}));
	// Подключаем отладочный вывод событий клиента
	subscribe(client, "client");
	// Устанавливаем функцию обратного вызова для обработки анонса server push
	client.on(parser_http2_t::push_callback_t([](const uint32_t sid, const uint32_t promisedSid) noexcept -> bool {
		// Выводим событие анонса server push
		cout << "[client] Push promised: stream #" << promisedSid << " (associated with #" << sid << ")" << endl;
		// Принимаем push (возврат false отклонил бы его RST_STREAM с кодом CANCEL)
		return true;
	}));
	// Выполняем рукопожатие соединения
	client.sendPreface();
	// Сервер отправляет свой SETTINGS
	server.sendPreface();
	// Выделяем идентификатор нового потока клиента
	const uint32_t sid = client.nextStreamId();
	// Формируем заголовки запроса страницы
	vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода запроса
	fields.emplace_back(":method", "GET");
	// Дописываем псевдо-заголовок схемы запроса
	fields.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути запроса
	fields.emplace_back(":path", "/index.html");
	// Отправляем заголовки запроса с завершением потока
	client.sendHeaders(sid, fields, true);
	/**
	 * Сервер знает, что вместе со страницей понадобится стилевой файл -
	 * анонсирует его через PUSH_PROMISE, не дожидаясь запроса клиента.
	 * Push-потоки получают чётные идентификаторы (2, 4, 6...)
	 */
	vector <h2::hpack::field_t> promise;
	// Дописываем псевдо-заголовок метода обещанного запроса
	promise.emplace_back(":method", "GET");
	// Дописываем псевдо-заголовок схемы обещанного запроса
	promise.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути обещанного запроса
	promise.emplace_back(":path", "/css/style.css");
	// Анонсируем server push на потоке запроса клиента
	const uint32_t promisedSid = server.sendPushPromise(sid, promise);
	// Формируем заголовки ответа страницы
	vector <h2::hpack::field_t> page;
	// Дописываем псевдо-заголовок статуса ответа
	page.emplace_back(":status", "200");
	// Дописываем обычный заголовок
	page.emplace_back("content-type", "text/html");
	// Отправляем заголовки ответа страницы (тело последует отдельно)
	server.sendHeaders(sid, page, false);
	// Отправляем тело ответа страницы с завершением потока
	server.sendData(sid, "<html/>", 7, true);
	// Формируем заголовки ответа push-потока
	vector <h2::hpack::field_t> css;
	// Дописываем псевдо-заголовок статуса ответа
	css.emplace_back(":status", "200");
	// Дописываем обычный заголовок
	css.emplace_back("content-type", "text/css");
	// Отправляем заголовки ответа push-потока (тело последует отдельно)
	server.sendHeaders(promisedSid, css, false);
	// Отправляем тело ответа push-потока с завершением потока
	server.sendData(promisedSid, "body{color:red}", 15, true);
	// Выводим пустую строку-разделитель
	cout << endl;
}
/**
 * @brief Демонстрация pull-модели выборки исходящих байтов (pending/consumePending)
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
static void samplePullOutput(const fmk_t * fmk, const log_t * log) noexcept {
	// Печатаем заголовок демонстрации
	cout << " ======== PULL OUTPUT MODEL ======== " << endl;
	/**
	 * Если функция обратного вызова записи не установлена - исходящие байты
	 * копятся во внутреннем буфере парсера, откуда сетевой слой забирает их
	 * сам: pending() возвращает view на неотправленные байты, consumePending()
	 * освобождает отправленную в сокет часть
	 */
	parser_http2_t server(direct_t::REQUEST, fmk, log);
	// Создаём объект парсера клиента
	parser_http2_t client(direct_t::RESPONSE, fmk, log);
	// Подключаем отладочный вывод событий сервера
	subscribe(server, "server");
	// Клиент отправляет magic-строку и свой SETTINGS (байты копятся в буфере)
	client.sendPreface();
	// Выделяем идентификатор нового потока клиента
	const uint32_t sid = client.nextStreamId();
	// Формируем заголовки запроса
	vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода запроса
	fields.emplace_back(":method", "GET");
	// Дописываем псевдо-заголовок схемы запроса
	fields.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути запроса
	fields.emplace_back(":path", "/pull");
	// Отправляем заголовки запроса с завершением потока
	client.sendHeaders(sid, fields, true);
	// Выводим размер накопленных исходящих байтов клиента
	cout << "[client] Pending output: " << client.pending().size() << " bytes" << endl;
	/**
	 * Эмулируем запись в сокет порциями по 16 байт: view инвалидируется
	 * методами парсера, поэтому копируем порцию перед освобождением
	 */
	while(!client.pending().empty()){
		// Получаем очередную порцию исходящих байтов
		const string chunk(client.pending().substr(0, 16));
		// Освобождаем отправленные байты из исходящего буфера
		client.consumePending(chunk.size());
		// Выполняем разбор переданных байтов на сервере
		server.parse(chunk.data(), chunk.size());
	}
	// Выводим остаток исходящего буфера клиента
	cout << "[client] Pending output drained: " << client.pending().size() << " bytes left" << endl << endl;
}
/**
 * @brief Демонстрация обработки ошибок и завершения соединения
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
static void sampleShutdown(const fmk_t * fmk, const log_t * log) noexcept {
	// Печатаем заголовок демонстрации
	cout << " ======== ERRORS & SHUTDOWN ======== " << endl;
	// Создаём объект парсера сервера
	parser_http2_t server(direct_t::REQUEST, fmk, log);
	// Создаём объект парсера клиента
	parser_http2_t client(direct_t::RESPONSE, fmk, log);
	// Исходящие байты клиента подаём на разбор серверу
	client.on(parser_http2_t::write_callback_t([&server](const void * buffer, const size_t size) noexcept {
		// Выполняем разбор исходящих байтов клиента на сервере
		server.parse(buffer, size);
	}));
	// Исходящие байты сервера подаём на разбор клиенту
	server.on(parser_http2_t::write_callback_t([&client](const void * buffer, const size_t size) noexcept {
		// Выполняем разбор исходящих байтов сервера на клиенте
		client.parse(buffer, size);
	}));
	// Подключаем отладочный вывод событий парсеров
	subscribe(server, "server");
	// Подключаем отладочный вывод событий клиента
	subscribe(client, "client");
	// Выполняем рукопожатие соединения
	client.sendPreface();
	// Сервер отправляет свой SETTINGS
	server.sendPreface();
	// Выделяем идентификатор нового потока клиента
	const uint32_t sid = client.nextStreamId();
	// Формируем заголовки запроса
	vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода запроса
	fields.emplace_back(":method", "GET");
	// Дописываем псевдо-заголовок схемы запроса
	fields.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути запроса
	fields.emplace_back(":path", "/slow-endpoint");
	// Отправляем заголовки запроса (поток остаётся открытым)
	client.sendHeaders(sid, fields, false);
	// Отменяем запрос аварийным закрытием потока (соединение при этом живёт)
	client.sendRstStream(sid, parser_http2_t::error_t::CANCEL);
	// Завершаем соединение штатно с отладочными данными
	client.sendGoaway(parser_http2_t::error_t::NO_ERROR, "session complete");
	// Уведомляем парсеры о закрытии соединения
	server.eof();
	// Уведомляем клиента о закрытии соединения
	client.eof();
	// Выводим итоговый статус соединения сервера
	cout << "[server] Final status: " << (server.status() == parser_t::status_t::COMPLETE ? "COMPLETE" : "ERROR") << endl;
	// Выводим итоговый статус соединения клиента
	cout << "[client] Final status: " << (client.status() == parser_t::status_t::COMPLETE ? "COMPLETE" : "ERROR") << endl << endl;
}

/**
 * @brief Главная функция приложения
 *
 * @param count  количество аргументов запуска приложения
 * @param params параметры запуска приложения
 * @return       код выхода из приложения
 */
int32_t main(int32_t count, char * params[]) noexcept {
	// Не используемые параметры
	(void) count;
	(void) params;
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Устанавливаем название сервиса
	log.name("Parser HTTP2");
	// Устанавливаем формат даты
	log.format("%H:%M:%S %d.%m.%Y");
	// Выполняем демонстрацию рукопожатия соединения
	sampleHandshake(&fmk, &log);
	// Выполняем демонстрацию полного обмена запросом и ответом
	sampleExchange(&fmk, &log);
	// Выполняем демонстрацию отправки заголовков из контейнера
	sampleContainer(&fmk, &log);
	// Выполняем демонстрацию pull-источника данных тела
	sampleDataSource(&fmk, &log);
	// Выполняем демонстрацию server push
	samplePush(&fmk, &log);
	// Выполняем демонстрацию pull-модели выборки исходящих байтов
	samplePullOutput(&fmk, &log);
	// Выполняем демонстрацию обработки ошибок и завершения соединения
	sampleShutdown(&fmk, &log);
	// Выводим удачное завершение работы
	return EXIT_SUCCESS;
}
