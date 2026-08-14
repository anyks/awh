/**
 * @file nghttp2-server.cpp
 * @date 2026-07-27
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * @brief Пробник совместимости серверного парсера HTTP/2 с эталонной реализацией nghttp2 — проверка рукопожатия,
 *        обмена SETTINGS, передачи тела сверх окна управления потоком, секции трейлеров и штатного закрытия потока
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Пробник: сквозная сессия нашего серверного парсера против клиента nghttp2.
 * Проверяются рукопожатие, обмен SETTINGS, тело запроса и ответа сверх окна
 * управления потоком, секция трейлеров и штатное закрытие потока
 */

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

/**
 * @brief Структура состояния проверки
 *
 */
typedef struct State {
	// Статус-код ответа, полученный клиентом nghttp2
	std::string status;
	// Тело ответа, полученное клиентом nghttp2
	std::string body;
	// Значение трейлера, полученное клиентом nghttp2
	std::string trailer;
	// Код закрытия потока на стороне клиента nghttp2
	uint32_t closeCode = 0xFFFFFFFF;
	// Признак получения некорректного кадра клиентом nghttp2
	bool invalid = false;
	// Текст ошибки клиента nghttp2
	std::string error;
	// Тело запроса, принятое нашим сервером
	std::string request;
	// Идентификатор потока запроса на нашем сервере
	uint32_t sid = 0;
	// Признак завершения приёма запроса нашим сервером
	bool received = false;
	// Байты от клиента nghttp2 к нашему серверу
	std::string toServer;
	// Байты от нашего сервера к клиенту nghttp2
	std::string toClient;
} state_t;

// Тело запроса клиента
static std::string requestBody;
// Текущая позиция отдачи тела запроса
static size_t requestOffset = 0;

/**
 * @brief Функция выдачи тела запроса клиентом nghttp2
 *
 */
static ssize_t readRequest(nghttp2_session *, int32_t, uint8_t * buf, size_t length, uint32_t * flags, nghttp2_data_source *, void *) noexcept {
	// Вычисляем размер отдаваемой порции тела
	const size_t chunk = ((::requestBody.size() - ::requestOffset) < length ? (::requestBody.size() - ::requestOffset) : length);
	// Копируем порцию тела в буфер nghttp2
	::memcpy(buf, ::requestBody.data() + ::requestOffset, chunk);
	// Сдвигаем позицию отдачи тела
	::requestOffset += chunk;
	// Если тело отдано полностью
	if(::requestOffset >= ::requestBody.size())
		// Помечаем завершение потока данных
		(* flags) |= NGHTTP2_DATA_FLAG_EOF;
	// Выводим размер отданной порции
	return static_cast <ssize_t> (chunk);
}

/**
 * @brief Функция обработки заголовка, полученного клиентом nghttp2
 *
 */
static int onHeader(nghttp2_session *, const nghttp2_frame * frame, const uint8_t * name, size_t namelen, const uint8_t * value, size_t valuelen, uint8_t, void * data) noexcept {
	// Получаем объект состояния проверки
	state_t * state = static_cast <state_t *> (data);
	// Формируем название заголовка
	const std::string key(reinterpret_cast <const char *> (name), namelen);
	// Если получен псевдо-заголовок статуса ответа
	if(key.compare(":status") == 0)
		// Запоминаем статус-код ответа
		state->status.assign(reinterpret_cast <const char *> (value), valuelen);
	// Если получен трейлер контрольной суммы
	else if(key.compare("x-checksum") == 0)
		// Запоминаем значение трейлера
		state->trailer.assign(reinterpret_cast <const char *> (value), valuelen);
	// Не используемый параметр
	(void) frame;
	// Продолжаем разбор
	return 0;
}

/**
 * @brief Функция обработки фрагмента тела, полученного клиентом nghttp2
 *
 */
static int onData(nghttp2_session *, uint8_t, int32_t, const uint8_t * data, size_t len, void * user) noexcept {
	// Получаем объект состояния проверки
	state_t * state = static_cast <state_t *> (user);
	// Накапливаем тело ответа
	state->body.append(reinterpret_cast <const char *> (data), len);
	// Продолжаем разбор
	return 0;
}

/**
 * @brief Функция обработки закрытия потока клиентом nghttp2
 *
 */
static int onClose(nghttp2_session *, int32_t, uint32_t code, void * user) noexcept {
	// Получаем объект состояния проверки
	state_t * state = static_cast <state_t *> (user);
	// Запоминаем код закрытия потока
	state->closeCode = code;
	// Продолжаем разбор
	return 0;
}

/**
 * @brief Функция обработки некорректного кадра клиентом nghttp2
 *
 */
static int onInvalid(nghttp2_session *, const nghttp2_frame * frame, int code, void * user) noexcept {
	// Получаем объект состояния проверки
	state_t * state = static_cast <state_t *> (user);
	// Помечаем получение некорректного кадра
	state->invalid = true;
	// Запоминаем описание ошибки
	state->error = ::nghttp2_strerror(code);
	// Печатаем тип и параметры отвергнутого кадра
	std::cout << "отвергнут кадр: тип=" << static_cast <uint32_t> (frame->hd.type)
	          << " поток=" << frame->hd.stream_id
	          << " длина=" << frame->hd.length
	          << " флаги=" << static_cast <uint32_t> (frame->hd.flags)
	          << " причина=" << state->error << std::endl;
	// Продолжаем разбор
	return 0;
}

/**
 * @brief Функция обработки текстовой диагностики nghttp2
 *
 */
static int onError(nghttp2_session *, int, const char * msg, size_t len, void *) noexcept {
	// Печатаем диагностику nghttp2
	std::cout << "nghttp2: " << std::string(msg, len) << std::endl;
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
	// Формируем тело запроса заведомо больше окна управления потоком
	::requestBody.assign(150000, 'q');
	/**
	 * Выполняем заполнение тела запроса псевдослучайным паттерном
	 */
	for(size_t i = 0; i < ::requestBody.size(); i++)
		// Заполняем очередной байт тела запроса
		::requestBody[i] = static_cast <char> ((i * 31 + 7) & 0xFF);
	// Формируем тело ответа заведомо больше окна управления потоком
	std::string responseBody(200000, 'r');
	/**
	 * Выполняем заполнение тела ответа псевдослучайным паттерном
	 */
	for(size_t i = 0; i < responseBody.size(); i++)
		// Заполняем очередной байт тела ответа
		responseBody[i] = static_cast <char> ((i * 17 + 3) & 0xFF);
	// Создаём объект парсера нашего сервера
	parser_http2_t server(direct_t::REQUEST, &fmk, &log);
	// Устанавливаем функцию обратного вызова записи исходящих байт
	server.on(parser_http2_t::write_callback_t([&](const void * buffer, const size_t size) noexcept {
		// Накапливаем байты для клиента nghttp2
		state.toClient.append(static_cast <const char *> (buffer), size);
	}));
	// Устанавливаем функцию обратного вызова провайдера заголовков
	server.on(parser_http2_t::provider_callback_t([&](const uint32_t sid, const provider_t * provider, const bool) noexcept -> bool {
		// Если получен провайдер запроса
		if(provider != nullptr)
			// Запоминаем идентификатор потока запроса
			state.sid = sid;
		// Продолжаем разбор
		return true;
	}));
	// Устанавливаем функцию обратного вызова тела запроса
	server.on(parser_http2_t::data_callback_t([&](const uint32_t, const void * buffer, const size_t size, const bool) noexcept -> bool {
		// Накапливаем тело запроса
		state.request.append(static_cast <const char *> (buffer), size);
		// Продолжаем разбор
		return true;
	}));
	// Устанавливаем функцию обратного вызова фазы приёма сообщения
	server.on(parser_http2_t::phase_callback_t([&](const uint32_t sid, const parser_t::phase_t phase, const parser_t::part_t part) noexcept -> bool {
		// Если приём запроса завершён полностью
		if((phase == parser_t::phase_t::END) && (part == parser_t::part_t::NONE) && !state.received){
			// Помечаем завершение приёма запроса
			state.received = true;
			// Формируем заголовки ответа сервера
			std::vector <h2::hpack::field_t> response;
			// Дописываем псевдо-заголовок статуса ответа
			response.emplace_back(":status", "200");
			// Дописываем заголовок типа содержимого
			response.emplace_back("content-type", "application/octet-stream");
			// Отправляем заголовки ответа
			server.sendHeaders(sid, response, false);
			// Текущая позиция отправки тела ответа
			size_t offset = 0;
			/**
			 * Отправляем тело ответа порциями, пока парсер их принимает
			 */
			while(offset < responseBody.size()){
				// Передаём очередную порцию тела ответа
				const size_t taken = server.sendData(sid, responseBody.data() + offset, responseBody.size() - offset, false);
				// Если порция не принята - прекращаем отправку
				if(taken == 0)
					// Прекращаем отправку тела ответа
					break;
				// Сдвигаем позицию отправки тела ответа
				offset += taken;
			}
			// Формируем секцию трейлеров
			std::vector <h2::hpack::field_t> trailers;
			// Дописываем трейлер контрольной суммы
			trailers.emplace_back("x-checksum", "0xDEADBEEF");
			// Отправляем секцию трейлеров с завершением потока
			server.sendHeaders(sid, trailers, true);
		}
		// Продолжаем разбор
		return true;
	}));
	// Отправляем preface нашего сервера
	server.sendPreface();
	// Объект набора функций обратного вызова nghttp2
	nghttp2_session_callbacks * callbacks = nullptr;
	// Создаём объект набора функций обратного вызова nghttp2
	::nghttp2_session_callbacks_new(&callbacks);
	// Устанавливаем функцию обработки заголовка
	::nghttp2_session_callbacks_set_on_header_callback(callbacks, ::onHeader);
	// Устанавливаем функцию обработки фрагмента тела
	::nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, ::onData);
	// Устанавливаем функцию обработки закрытия потока
	::nghttp2_session_callbacks_set_on_stream_close_callback(callbacks, ::onClose);
	// Устанавливаем функцию обработки некорректного кадра
	::nghttp2_session_callbacks_set_on_invalid_frame_recv_callback(callbacks, ::onInvalid);
	// Устанавливаем функцию текстовой диагностики
	::nghttp2_session_callbacks_set_error_callback2(callbacks, ::onError);
	// Объект сессии клиента nghttp2
	nghttp2_session * session = nullptr;
	// Создаём объект сессии клиента nghttp2
	::nghttp2_session_client_new(&session, callbacks, &state);
	// Удаляем объект набора функций обратного вызова nghttp2
	::nghttp2_session_callbacks_del(callbacks);
	// Отправляем SETTINGS клиента nghttp2
	::nghttp2_submit_settings(session, NGHTTP2_FLAG_NONE, nullptr, 0);
	// Формируем заголовки запроса клиента nghttp2
	const nghttp2_nv nva[] = {
		{(uint8_t *) ":method", (uint8_t *) "POST", 7, 4, NGHTTP2_NV_FLAG_NONE},
		{(uint8_t *) ":scheme", (uint8_t *) "https", 7, 5, NGHTTP2_NV_FLAG_NONE},
		{(uint8_t *) ":path", (uint8_t *) "/upload", 5, 7, NGHTTP2_NV_FLAG_NONE},
		{(uint8_t *) ":authority", (uint8_t *) "example.com", 10, 11, NGHTTP2_NV_FLAG_NONE},
		{(uint8_t *) "content-length", (uint8_t *) "150000", 14, 6, NGHTTP2_NV_FLAG_NONE}
	};
	// Объект источника тела запроса
	nghttp2_data_provider provider;
	// Источник данных не используется
	provider.source.ptr = nullptr;
	// Устанавливаем функцию выдачи тела запроса
	provider.read_callback = ::readRequest;
	// Отправляем запрос клиента nghttp2
	::nghttp2_submit_request(session, nullptr, nva, 5, &provider, nullptr);
	/**
	 * Выполняем обмен байтами между клиентом nghttp2 и нашим сервером
	 */
	for(size_t round = 0; round < 20000; round++){
		// Признак наличия обмена в текущем раунде
		bool progress = false;
		// Указатель на исходящие байты клиента nghttp2
		const uint8_t * out = nullptr;
		/**
		 * Забираем все исходящие байты клиента nghttp2
		 */
		for(;;){
			// Получаем очередную порцию исходящих байт клиента
			const ssize_t length = ::nghttp2_session_mem_send(session, &out);
			// Если порция пуста - прекращаем выборку
			if(length <= 0)
				// Прекращаем выборку исходящих байт
				break;
			// Накапливаем байты для нашего сервера
			state.toServer.append(reinterpret_cast <const char *> (out), static_cast <size_t> (length));
			// Помечаем наличие обмена
			progress = true;
		}
		// Если для нашего сервера есть входящие байты
		if(!state.toServer.empty()){
			// Забираем накопленные байты
			const std::string input = state.toServer;
			// Очищаем накопитель
			state.toServer.clear();
			// Подаём байты на разбор нашему серверу
			server.parse(input.data(), input.size());
			// Помечаем наличие обмена
			progress = true;
		}
		// Если для клиента nghttp2 есть входящие байты
		if(!state.toClient.empty()){
			// Забираем накопленные байты
			const std::string input = state.toClient;
			// Очищаем накопитель
			state.toClient.clear();
			// Подаём байты на разбор клиенту nghttp2
			const ssize_t used = ::nghttp2_session_mem_recv(session, reinterpret_cast <const uint8_t *> (input.data()), input.size());
			// Если разбор завершился ошибкой
			if(used < 0){
				// Запоминаем описание ошибки
				state.error = ::nghttp2_strerror(static_cast <int> (used));
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
	// Удаляем объект сессии клиента nghttp2
	::nghttp2_session_del(session);
	// Количество обнаруженных расхождений
	size_t failures = 0;
	// Печатаем статус-код ответа
	std::cout << "статус ответа: " << state.status << std::endl;
	// Если статус-код ответа не совпал
	if(state.status.compare("200") != 0)
		// Наращиваем счётчик расхождений
		failures++;
	// Печатаем объём принятого сервером тела запроса
	std::cout << "тело запроса принято сервером: " << state.request.size() << " из " << ::requestBody.size() << std::endl;
	// Если тело запроса принято с искажениями
	if(state.request != ::requestBody)
		// Наращиваем счётчик расхождений
		failures++;
	// Печатаем объём принятого клиентом тела ответа
	std::cout << "тело ответа принято клиентом: " << state.body.size() << " из " << responseBody.size() << std::endl;
	// Если тело ответа принято с искажениями
	if(state.body != responseBody)
		// Наращиваем счётчик расхождений
		failures++;
	// Печатаем значение полученного трейлера
	std::cout << "трейлер: [" << state.trailer << "]" << std::endl;
	// Если трейлер не получен
	if(state.trailer.compare("0xDEADBEEF") != 0)
		// Наращиваем счётчик расхождений
		failures++;
	// Печатаем код закрытия потока
	std::cout << "код закрытия потока: " << state.closeCode << std::endl;
	// Если поток закрыт не штатно
	if(state.closeCode != 0)
		// Наращиваем счётчик расхождений
		failures++;
	// Если клиент nghttp2 получил некорректный кадр
	if(state.invalid){
		// Печатаем описание некорректного кадра
		std::cout << "клиент nghttp2 отверг кадр: " << state.error << std::endl;
		// Наращиваем счётчик расхождений
		failures++;
	}
	// Если зафиксирована ошибка сессии
	if(!state.error.empty() && !state.invalid)
		// Печатаем описание ошибки сессии
		std::cout << "ошибка сессии nghttp2: " << state.error << std::endl;
	// Печатаем итог проверки
	std::cout << "расхождений: " << failures << std::endl;
	// Выводим результат
	return (failures == 0 ? 0 : 1);
}
