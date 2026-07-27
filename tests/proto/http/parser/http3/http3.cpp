/**
 * @file: http3.cpp
 * @date: 2026-07-27
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация тестовой фикстуры парсера протокола HTTP/3 — эмулятор транспорта QUIC,
 *        подписка сборщика событий и вспомогательные методы тестов
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "http3.hpp"

/**
 * @brief Метод настройки тестового окружения
 *
 */
void ParserHttp3Fixture::SetUp(){
	// Создаём объект фреймворка
	this->_fmk = std::make_unique <awh::fmk_t> ();
	// Создаём объект логгера
	this->_log = std::make_unique <awh::log_t> (this->_fmk.get());
}

/**
 * @brief Метод очистки тестового окружения
 *
 */
void ParserHttp3Fixture::TearDown() {}

/**
 * @brief Метод подготовки стороны соединения
 *
 * @param endpoint сторона соединения
 * @param direct   направление трафика (REQUEST - мы сервер, RESPONSE - мы клиент)
 *
 */
void ParserHttp3Fixture::setup(endpoint_t & endpoint, const awh::http::direct_t direct) const noexcept {
	// Создаём объект парсера стороны соединения
	endpoint.parser = std::make_unique <awh::http::parser_http3_t> (direct, this->_fmk.get(), this->_log.get());
	/**
	 * Идентификаторы потоков выдаются по правилам RFC 9000 §2.1: младший бит
	 * различает инициатора (0 - клиент, 1 - сервер), второй - направленность
	 * (0 - двунаправленный, 1 - однонаправленный)
	 */
	const bool server = (direct == awh::http::direct_t::REQUEST);
	// Устанавливаем идентификатор первого однонаправленного потока стороны
	endpoint.unistream = (server ? 3 : 2);
	// Устанавливаем идентификатор первого двунаправленного потока стороны
	endpoint.bistream = (server ? 1 : 0);
	// Получаем ссылку на сборщик событий стороны соединения
	events_t & events = endpoint.events;
	// Устанавливаем функцию обратного вызова открытия однонаправленного потока
	endpoint.parser->on(awh::http::parser_http3_t::open_callback_t([&endpoint]() noexcept -> int64_t {
		// Если транспорт отказывается открывать потоки
		if(endpoint.refuse)
			// Выводим признак отказа транспорта
			return -1;
		// Выделяем идентификатор однонаправленного потока
		const int64_t sid = endpoint.unistream;
		// Продвигаем идентификатор следующего однонаправленного потока
		endpoint.unistream += 4;
		// Выводим идентификатор открытого потока
		return sid;
	}));
	// Устанавливаем функцию обратного вызова записи исходящих байтов потока
	endpoint.parser->on(awh::http::parser_http3_t::write_callback_t([&endpoint](const uint64_t sid, const void * buffer, const size_t size, const bool fin) noexcept {
		// Складываем исходящие байты в очередь транспорта
		endpoint.queue.emplace_back(sid, std::string(reinterpret_cast <const char *> (buffer), size), fin);
	}));
	// Устанавливаем функцию обратного вызова обрыва потока
	endpoint.parser->on(awh::http::parser_http3_t::abort_callback_t([&events](const uint64_t sid, const awh::http::parser_http3_t::error_t code, const bool stop) noexcept {
		// Собираем событие обрыва потока
		events.aborts.emplace_back(sid, code, stop);
	}));
	// Устанавливаем функцию обратного вызова ошибки уровня соединения
	endpoint.parser->on(awh::http::parser_http3_t::error_callback_t([&events](const awh::http::parser_http3_t::error_t code, const std::string_view message) noexcept {
		// Собираем событие ошибки уровня соединения
		events.errors.emplace_back(code, std::string(message));
	}));
	// Устанавливаем функцию обратного вызова открытия нового потока
	endpoint.parser->on(awh::http::parser_http3_t::begin_callback_t([&events](const uint64_t sid) noexcept -> bool {
		// Собираем идентификатор открытого пиром потока
		events.begins.push_back(sid);
		// Продолжаем разбор
		return true;
	}));
	// Устанавливаем функцию обратного вызова фазы приёма сообщения потока
	endpoint.parser->on(awh::http::parser_http3_t::phase_callback_t([&events](const uint64_t sid, const awh::http::parser_t::phase_t phase, const awh::http::parser_t::part_t part) noexcept -> bool {
		// Собираем фазовое событие приёма сообщения потока
		events.phases.emplace_back(sid, phase, part);
		// Продолжаем разбор
		return true;
	}));
	// Устанавливаем функцию обратного вызова поля секции
	endpoint.parser->on(awh::http::parser_http3_t::header_callback_t([&events](const uint64_t sid, const std::string_view name, const std::string_view value, const awh::http::parser_t::part_t part) noexcept -> bool {
		// Собираем поле секции
		events.headers.emplace_back(sid, std::string(name), std::string(value), part);
		// Продолжаем разбор
		return true;
	}));
	// Устанавливаем функцию обратного вызова провайдера полей потока
	endpoint.parser->on(awh::http::parser_http3_t::provider_callback_t([&events](const uint64_t sid, const awh::http::provider_t * provider, const bool endStream) noexcept -> bool {
		// Собираем событие провайдера полей потока
		events.providers.emplace_back(sid, (provider == nullptr), endStream);
		// Если провайдер полей потока собран
		if(provider != nullptr){
			// Если провайдер является запросом клиента
			if(provider->direct == awh::http::direct_t::REQUEST){
				// Получаем объект провайдера запроса клиента
				const awh::http::request_t * request = static_cast <const awh::http::request_t *> (provider);
				// Запоминаем метод запроса клиента
				events.method = std::string(awh::http::methodName(request));
				// Запоминаем URI-запрос клиента
				events.uri = request->uri;
			// Если провайдер является ответом сервера
			} else {
				// Получаем объект провайдера ответа сервера
				const awh::http::response_t * response = static_cast <const awh::http::response_t *> (provider);
				// Запоминаем статус-код ответа сервера
				events.code = response->code;
			}
		}
		// Продолжаем разбор
		return true;
	}));
	// Устанавливаем функцию обратного вызова фрагмента тела потока
	endpoint.parser->on(awh::http::parser_http3_t::data_callback_t([&events](const uint64_t sid, const void * buffer, const size_t size, const bool) noexcept -> bool {
		// Собираем фрагмент тела потока
		events.bodies[sid].append(reinterpret_cast <const char *> (buffer), size);
		// Продолжаем разбор
		return true;
	}));
	// Устанавливаем функцию обратного вызова закрытия потока
	endpoint.parser->on(awh::http::parser_http3_t::close_callback_t([&events](const uint64_t sid, const awh::http::parser_http3_t::error_t code) noexcept {
		// Собираем событие закрытия потока
		events.closes.emplace_back(sid, code);
	}));
	// Устанавливаем функцию обратного вызова анонса server push
	endpoint.parser->on(awh::http::parser_http3_t::push_callback_t([&events](const uint64_t sid, const uint64_t pushId) noexcept -> bool {
		// Собираем анонс server push
		events.pushes.emplace_back(sid, pushId);
		// Принимаем обещанный push
		return true;
	}));
	// Устанавливаем функцию обратного вызова завершения соединения пиром
	endpoint.parser->on(awh::http::parser_http3_t::goaway_callback_t([&events](const uint64_t identifier) noexcept {
		// Собираем объявленный пиром идентификатор
		events.goaways.push_back(identifier);
	}));
	// Устанавливаем функцию обратного вызова применённых параметров пира
	endpoint.parser->on(awh::http::parser_http3_t::settings_callback_t([&events]() noexcept {
		// Считаем применённый набор параметров пира
		events.settings++;
	}));
}
/**
 * @brief Метод прокачки очередей исходящих данных между сторонами соединения
 *
 * @param client сторона клиента
 * @param server сторона сервера
 *
 */
void ParserHttp3Fixture::pump(endpoint_t & client, endpoint_t & server) const noexcept {
	/**
	 * Прокачка ограничена сверху: обработчик вправе отправить данные в ответ
	 * на разбор, и без границы взаимная переписка сторон могла бы не закончиться
	 */
	for(size_t guard = 0; guard < 64; guard++){
		// Признак того, что на этом обороте что-то передавалось
		bool moved = false;
		/**
		 * Выполняем перебор обеих сторон соединения
		 */
		for(endpoint_t * side : {&client, &server}){
			// Определяем противоположную сторону соединения
			endpoint_t * peer = ((side == &client) ? &server : &client);
			/**
			 * Выполняем передачу всех накопленных стороной данных
			 */
			while(!side->queue.empty()){
				// Забираем очередную порцию исходящих данных
				const auto item = side->queue.front();
				// Удаляем порцию из очереди транспорта
				side->queue.pop_front();
				// Подаём порцию на разбор противоположной стороне
				peer->parser->parse(std::get <0> (item), std::get <1> (item).data(), std::get <1> (item).size(), std::get <2> (item));
				// Запоминаем, что передача состоялась
				moved = true;
			}
		}
		// Если передавать больше нечего
		if(!moved)
			// Прекращаем прокачку
			break;
	}
}
/**
 * @brief Метод выполнения рукопожатия соединения (обмен SETTINGS)
 *
 * @param client сторона клиента
 * @param server сторона сервера
 *
 */
void ParserHttp3Fixture::handshake(endpoint_t & client, endpoint_t & server) const noexcept {
	// Отправляем параметры соединения со стороны клиента
	client.parser->sendSettings();
	// Отправляем параметры соединения со стороны сервера
	server.parser->sendSettings();
	// Выполняем прокачку очередей исходящих данных
	this->pump(client, server);
}
/**
 * @brief Метод подачи произвольных байтов в поток одной стороны
 *
 * @param endpoint сторона соединения
 * @param sid      идентификатор потока
 * @param data     подаваемые байты
 * @param fin      признак завершения потока
 * @return         результат разбора
 *
 */
awh::http::h3::status_t ParserHttp3Fixture::feed(endpoint_t & endpoint, const uint64_t sid, const std::string & data, const bool fin) const noexcept {
	// Выполняем разбор поданных байтов
	return endpoint.parser->parse(sid, data.data(), data.size(), fin);
}
/**
 * @brief Метод сборки набора полей запроса клиента
 *
 * @param method метод запроса
 * @param path   путь запроса
 * @return       собранный набор полей
 *
 */
std::vector <awh::http::h3::qpack::field_t> ParserHttp3Fixture::request(const std::string & method, const std::string & path) const noexcept {
	// Выводим собранный набор полей запроса клиента
	return {
		awh::http::h3::qpack::field_t{":method", method},
		awh::http::h3::qpack::field_t{":scheme", "https"},
		awh::http::h3::qpack::field_t{":authority", "example.com"},
		awh::http::h3::qpack::field_t{":path", path}
	};
}
/**
 * @brief Метод сборки набора полей ответа сервера
 *
 * @param status статус-код ответа
 * @return       собранный набор полей
 *
 */
std::vector <awh::http::h3::qpack::field_t> ParserHttp3Fixture::response(const std::string & status) const noexcept {
	// Выводим собранный набор полей ответа сервера
	return {
		awh::http::h3::qpack::field_t{":status", status},
		awh::http::h3::qpack::field_t{"server", "awh"}
	};
}
/**
 * @brief Метод поиска значения поля в собранных событиях
 *
 * @param events сборщик событий парсера
 * @param sid    идентификатор потока
 * @param name   название искомого поля
 * @return       значение поля либо пустая строка
 *
 */
std::string ParserHttp3Fixture::field(const events_t & events, const uint64_t sid, const std::string & name) const noexcept {
	/**
	 * Выполняем перебор всех собранных полей секций
	 */
	for(const auto & item : events.headers){
		// Если поле принадлежит искомому потоку и совпало по названию
		if((std::get <0> (item) == sid) && (std::get <1> (item) == name))
			// Выводим значение найденного поля
			return std::get <2> (item);
	}
	// Поле не найдено
	return std::string();
}
/**
 * @brief Метод сборки последовательности фазовых событий потока в строку
 *
 * @param events сборщик событий парсера
 * @param sid    идентификатор потока
 * @return       последовательность фазовых событий
 *
 */
std::string ParserHttp3Fixture::phases(const events_t & events, const uint64_t sid) const noexcept {
	// Названия фаз приёма сообщения
	static const char * PHASES[] = {"NONE", "BEGIN", "END"};
	// Названия частей сообщения
	static const char * PARTS[] = {"NONE", "BODY", "HEADERS", "TRAILER"};
	// Собираемая последовательность фазовых событий
	std::string result;
	/**
	 * Выполняем перебор всех собранных фазовых событий
	 */
	for(const auto & item : events.phases){
		// Если событие принадлежит не искомому потоку
		if(std::get <0> (item) != sid)
			// Переходим к следующему событию
			continue;
		// Если последовательность не пуста - дописываем разделитель
		if(!result.empty())
			// Дописываем разделитель событий
			result.append(" ");
		// Дописываем название фазы приёма сообщения
		result.append(PHASES[static_cast <uint8_t> (std::get <1> (item))]);
		// Дописываем разделитель фазы и части сообщения
		result.append(":");
		// Дописываем название части сообщения
		result.append(PARTS[static_cast <uint8_t> (std::get <2> (item))]);
	}
	// Выводим собранную последовательность фазовых событий
	return result;
}
