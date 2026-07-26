/**
 * @file: nghttp2-negative.cpp
 * @date: 2026-07-27
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Пробник дифференциальной сверки обработки ошибок HTTP/2 с эталонной реализацией nghttp2 —
 *        подача одного потока некорректных кадров обоим серверным парсерам и сравнение класса реакции:
 *        ошибка соединения, потоковая ошибка или приём без возражений
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Пробник: дифференциальная сверка обработки ошибок с эталонной реализацией.
 * Один и тот же поток некорректных кадров подаётся нашему серверному парсеру
 * и серверной сессии nghttp2, после чего сравнивается класс реакции:
 * ошибка соединения, потоковая ошибка либо приём без возражений
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
 * @brief Функция сборки произвольного кадра HTTP/2
 *
 * @param type    тип кадра
 * @param flags   флаги кадра
 * @param sid     идентификатор потока
 * @param payload полезная нагрузка кадра
 * @return        собранный кадр
 *
 */
static std::string frame(const uint8_t type, const uint8_t flags, const uint32_t sid, const std::string & payload) noexcept {
	// Результат работы функции - собранный кадр
	std::string result;
	// Дописываем 24-битную длину полезной нагрузки
	result.push_back(static_cast <char> ((payload.size() >> 16) & 0xFF));
	result.push_back(static_cast <char> ((payload.size() >> 8) & 0xFF));
	result.push_back(static_cast <char> (payload.size() & 0xFF));
	// Дописываем тип кадра
	result.push_back(static_cast <char> (type));
	// Дописываем флаги кадра
	result.push_back(static_cast <char> (flags));
	// Дописываем идентификатор потока
	result.push_back(static_cast <char> ((sid >> 24) & 0xFF));
	result.push_back(static_cast <char> ((sid >> 16) & 0xFF));
	result.push_back(static_cast <char> ((sid >> 8) & 0xFF));
	result.push_back(static_cast <char> (sid & 0xFF));
	// Дописываем полезную нагрузку
	result += payload;
	// Выводим собранный кадр
	return result;
}

/**
 * @brief Функция упаковки 32-битного значения
 *
 * @param value упаковываемое значение
 * @return      четыре байта значения
 *
 */
static std::string u32(const uint32_t value) noexcept {
	// Результат работы функции - четыре байта значения
	std::string result;
	// Дописываем байты значения
	result.push_back(static_cast <char> ((value >> 24) & 0xFF));
	result.push_back(static_cast <char> ((value >> 16) & 0xFF));
	result.push_back(static_cast <char> ((value >> 8) & 0xFF));
	result.push_back(static_cast <char> (value & 0xFF));
	// Выводим четыре байта значения
	return result;
}

/**
 * @brief Функция кодирования блока заголовков
 *
 * @param fields кодируемые заголовки
 * @return       закодированный блок заголовков
 *
 */
static std::string block(const std::vector <std::pair <std::string, std::string>> & fields) noexcept {
	// Создаём объект кодера заголовков
	h2::hpack::encoder_t encoder;
	// Список кодируемых заголовков
	std::vector <h2::hpack::field_t> items;
	// Резервируем память под кодируемые заголовки
	items.reserve(fields.size());
	/**
	 * Выполняем перебор всех кодируемых заголовков
	 */
	for(const auto & field : fields)
		// Дописываем кодируемый заголовок
		items.emplace_back(field.first, field.second);
	// Буфер закодированного блока заголовков
	std::string result;
	// Кодируем блок заголовков без сжатия Huffman (читаемость дампа важнее размера)
	encoder.encode(items, result, false);
	// Выводим закодированный блок заголовков
	return result;
}

// Признак подробного вывода кадров
static bool verbose = false;

/**
 * @brief Функция печати кадров исходящего потока
 *
 * @param prefix описание источника потока
 * @param output исходящий поток байт
 *
 */
static void dump(const char * prefix, const std::string & output) noexcept {
	// Таблица названий типов кадров
	static const char * names[] = {
		"DATA", "HEADERS", "PRIORITY", "RST_STREAM", "SETTINGS",
		"PUSH_PROMISE", "PING", "GOAWAY", "WINDOW_UPDATE", "CONTINUATION"
	};
	// Текущая позиция разбора потока
	size_t pos = 0;
	/**
	 * Выполняем перебор всех кадров исходящего потока
	 */
	while((pos + 9) <= output.size()){
		// Извлекаем длину полезной нагрузки кадра
		const size_t length = (
			(static_cast <size_t> (static_cast <uint8_t> (output[pos])) << 16) |
			(static_cast <size_t> (static_cast <uint8_t> (output[pos + 1])) << 8) |
			static_cast <size_t> (static_cast <uint8_t> (output[pos + 2]))
		);
		// Извлекаем тип кадра
		const uint8_t type = static_cast <uint8_t> (output[pos + 3]);
		// Печатаем параметры кадра
		::printf("    %s %s len=%zu\n", prefix, ((type < 10) ? names[type] : "UNKNOWN"), length);
		// Если кадр целиком не поместился - прекращаем разбор
		if((pos + 9 + length) > output.size())
			// Прекращаем разбор потока
			break;
		// Сдвигаем позицию за разобранный кадр
		pos += (9 + length);
	}
}

/**
 * @brief Функция классификации реакции эндпоинта
 *
 * @param output исходящий поток байт эндпоинта
 * @return       текстовое описание класса реакции
 *
 */
static std::string reaction(const std::string & output) noexcept {
	// Результат работы функции - описание реакции
	std::string result;
	// Текущая позиция разбора потока
	size_t pos = 0;
	/**
	 * Выполняем перебор всех кадров исходящего потока
	 */
	while((pos + 9) <= output.size()){
		// Извлекаем длину полезной нагрузки кадра
		const size_t length = (
			(static_cast <size_t> (static_cast <uint8_t> (output[pos])) << 16) |
			(static_cast <size_t> (static_cast <uint8_t> (output[pos + 1])) << 8) |
			static_cast <size_t> (static_cast <uint8_t> (output[pos + 2]))
		);
		// Извлекаем тип кадра
		const uint8_t type = static_cast <uint8_t> (output[pos + 3]);
		// Если кадр целиком не поместился - прекращаем разбор
		if((pos + 9 + length) > output.size())
			// Прекращаем разбор потока
			break;
		// Если получен кадр сброса потока
		if((type == 0x03) && (length >= 4) && result.empty())
			// Формируем описание потоковой ошибки
			result = ("RST(" + std::to_string(static_cast <uint32_t> (static_cast <uint8_t> (output[pos + 12]))) + ")");
		// Если получен кадр завершения соединения с ненулевым кодом
		else if((type == 0x07) && (length >= 8)) {
			// Извлекаем код ошибки завершения соединения
			const uint32_t code = static_cast <uint32_t> (static_cast <uint8_t> (output[pos + 16]));
			// Завершение с кодом ошибки перекрывает потоковую реакцию
			if(code != 0)
				// Формируем описание ошибки соединения
				result = ("GOAWAY(" + std::to_string(code) + ")");
		}
		// Сдвигаем позицию за разобранный кадр
		pos += (9 + length);
	}
	// Если реакции не было - помечаем приём без возражений
	if(result.empty())
		// Формируем описание приёма
		result = "-";
	// Выводим описание реакции
	return result;
}

/**
 * @brief Функция прогона потока байт через наш парсер
 *
 * @param fmk   объект фреймворка
 * @param log   объект логов
 * @param input подаваемый поток байт
 * @return      описание класса реакции
 *
 */
static std::string ours(const fmk_t * fmk, const log_t * log, const std::string & input) noexcept {
	// Создаём объект парсера сервера
	parser_http2_t server(direct_t::REQUEST, fmk, log);
	// Накопитель исходящего потока байт
	std::string output;
	// Устанавливаем функцию обратного вызова записи исходящих байт
	server.on(parser_http2_t::write_callback_t([&](const void * buffer, const size_t size) noexcept {
		// Накапливаем исходящие байты
		output.append(static_cast <const char *> (buffer), size);
	}));
	// Устанавливаем функцию обратного вызова фазы приёма сообщения
	server.on(parser_http2_t::phase_callback_t([&](const uint32_t sid, const parser_t::phase_t phase, const parser_t::part_t part) noexcept -> bool {
		// Если приём запроса завершён полностью
		if((phase == parser_t::phase_t::END) && (part == parser_t::part_t::NONE)){
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
	// Отправляем preface сервера
	server.sendPreface();
	// Очищаем накопитель: собственный preface к реакции не относится
	output.clear();
	// Подаём поток байт на разбор
	server.parse(input.data(), input.size());
	// Если включён подробный вывод
	if(::verbose)
		// Печатаем кадры нашего парсера
		::dump("наш:", output);
	// Выводим описание класса реакции
	return ::reaction(output);
}

/**
 * @brief Функция прогона потока байт через сессию nghttp2
 *
 * @param input подаваемый поток байт
 * @return      описание класса реакции
 *
 */
static std::string reference(const std::string & input) noexcept {
	// Объект набора функций обратного вызова nghttp2
	nghttp2_session_callbacks * callbacks = nullptr;
	// Создаём объект набора функций обратного вызова nghttp2
	::nghttp2_session_callbacks_new(&callbacks);
	// Объект сессии сервера nghttp2
	nghttp2_session * session = nullptr;
	// Создаём объект сессии сервера nghttp2
	::nghttp2_session_server_new(&session, callbacks, nullptr);
	// Удаляем объект набора функций обратного вызова nghttp2
	::nghttp2_session_callbacks_del(callbacks);
	// Отправляем SETTINGS сервера nghttp2
	::nghttp2_submit_settings(session, NGHTTP2_FLAG_NONE, nullptr, 0);
	// Указатель на исходящие байты сессии
	const uint8_t * out = nullptr;
	/**
	 * Забираем собственный preface сессии: к реакции он не относится
	 */
	while(::nghttp2_session_mem_send(session, &out) > 0){
		// Продолжаем выборку исходящих байт
		continue;
	}
	// Подаём поток байт на разбор
	const ssize_t used = ::nghttp2_session_mem_recv(session, reinterpret_cast <const uint8_t *> (input.data()), input.size());
	// Накопитель исходящего потока байт
	std::string output;
	/**
	 * Забираем реакцию сессии
	 */
	for(;;){
		// Получаем очередную порцию исходящих байт
		const ssize_t length = ::nghttp2_session_mem_send(session, &out);
		// Если порция пуста - прекращаем выборку
		if(length <= 0)
			// Прекращаем выборку исходящих байт
			break;
		// Накапливаем исходящие байты
		output.append(reinterpret_cast <const char *> (out), static_cast <size_t> (length));
	}
	// Удаляем объект сессии сервера nghttp2
	::nghttp2_session_del(session);
	// Если включён подробный вывод
	if(::verbose){
		// Печатаем объём разобранных эталоном байт
		::printf("    nghttp2 разобрал %zd из %zu байт\n", used, input.size());
		// Печатаем кадры эталонной реализации
		::dump("nghttp2:", output);
	}
	// Если разбор оборван внутренней ошибкой сессии - это тоже ошибка соединения
	if((used < 0) && (::reaction(output).compare("-") == 0))
		// Выводим описание ошибки соединения
		return "ОБРЫВ";
	// Выводим описание класса реакции
	return ::reaction(output);
}

int32_t main(int32_t argc, char * argv[]){
	// Включаем подробный вывод кадров по требованию
	::verbose = ((argc > 1) && (::strcmp(argv[1], "-v") == 0));
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Отключаем вывод логов: проверки намеренно шлют некорректный трафик
	log.level(log_t::level_t::NONE);
	// Корректный блок заголовков запроса
	const std::string request = ::block({{":method", "GET"}, {":scheme", "https"}, {":path", "/"}, {":authority", "example.com"}});
	// Преамбула соединения: magic-строка и SETTINGS клиента
	const std::string preface = (std::string(h2::proto::PREFACE) + ::frame(0x04, 0x00, 0, ""));
	// Таблица проверяемых случаев
	const std::vector <std::pair <std::string, std::string>> cases = {
		// Тело на ещё не открытом потоке
		{"DATA на idle-потоке", ::frame(0x00, 0x00, 1, "body")},
		// Заголовки на потоке с нулевым идентификатором
		{"HEADERS на потоке 0", ::frame(0x01, 0x04, 0, request)},
		// Заголовки на чётном потоке от клиента
		{"HEADERS с чётным идентификатором", ::frame(0x01, 0x04, 2, request)},
		// Убывающий идентификатор потока
		{"HEADERS с убывающим идентификатором", (::frame(0x01, 0x05, 3, request) + ::frame(0x01, 0x05, 1, request))},
		// Продолжение блока без начала
		{"CONTINUATION без HEADERS", ::frame(0x09, 0x04, 1, request)},
		// Кадр приоритета с зависимостью от самого себя
		{"PRIORITY с зависимостью от себя", ::frame(0x02, 0x00, 1, ::u32(1) + std::string(1, '\x10'))},
		// Кадр приоритета некорректной длины
		{"PRIORITY некорректной длины", ::frame(0x02, 0x00, 1, ::u32(0))},
		// Сброс несуществующего потока
		{"RST_STREAM на idle-потоке", ::frame(0x03, 0x00, 1, ::u32(8))},
		// Нулевой инкремент окна соединения
		{"WINDOW_UPDATE 0 на соединении", ::frame(0x08, 0x00, 0, ::u32(0))},
		// Нулевой инкремент окна потока
		{"WINDOW_UPDATE 0 на потоке", (::frame(0x01, 0x04, 1, request) + ::frame(0x08, 0x00, 1, ::u32(0)))},
		// Переполнение окна потока
		{"WINDOW_UPDATE переполняет окно потока", (::frame(0x01, 0x04, 1, request) + ::frame(0x08, 0x00, 1, ::u32(0x7FFFFFFF)))},
		// Переполнение окна соединения
		{"WINDOW_UPDATE переполняет окно соединения", ::frame(0x08, 0x00, 0, ::u32(0x7FFFFFFF))},
		// Кадр PING некорректной длины
		{"PING некорректной длины", ::frame(0x06, 0x00, 0, std::string(7, '\0'))},
		// Кадр PING на потоке
		{"PING на ненулевом потоке", ::frame(0x06, 0x00, 1, std::string(8, '\0'))},
		// Кадр SETTINGS некорректной длины
		{"SETTINGS некратной длины", ::frame(0x04, 0x00, 0, std::string(5, '\0'))},
		// Кадр SETTINGS с подтверждением и полезной нагрузкой
		{"SETTINGS ACK с нагрузкой", ::frame(0x04, 0x01, 0, std::string(6, '\0'))},
		// Кадр GOAWAY некорректной длины
		{"GOAWAY некорректной длины", ::frame(0x07, 0x00, 0, std::string(4, '\0'))},
		// Анонс push от клиента серверу
		{"PUSH_PROMISE от клиента", ::frame(0x05, 0x04, 1, ::u32(2) + request)},
		// Кадр неизвестного типа
		{"кадр неизвестного типа", ::frame(0x63, 0x00, 0, "unknown")},
		// Заголовки без обязательных псевдо-заголовков
		{"HEADERS без псевдо-заголовков", ::frame(0x01, 0x05, 1, ::block({{"accept", "*/*"}}))},
		// Заголовок в верхнем регистре
		{"HEADERS с заголовком в верхнем регистре", ::frame(0x01, 0x05, 1, ::block({{":method", "GET"}, {":scheme", "https"}, {":path", "/"}, {"Accept", "*/*"}}))},
		// Заголовок соединения, запрещённый в HTTP/2
		{"HEADERS с заголовком connection", ::frame(0x01, 0x05, 1, ::block({{":method", "GET"}, {":scheme", "https"}, {":path", "/"}, {"connection", "keep-alive"}}))},
		// Заголовок TE с недопустимым значением
		{"HEADERS с te: gzip", ::frame(0x01, 0x05, 1, ::block({{":method", "GET"}, {":scheme", "https"}, {":path", "/"}, {"te", "gzip"}}))},
		// Повторный псевдо-заголовок
		{"HEADERS с повторным :path", ::frame(0x01, 0x05, 1, ::block({{":method", "GET"}, {":scheme", "https"}, {":path", "/"}, {":path", "/other"}}))},
		// Псевдо-заголовок после обычного
		{"HEADERS с псевдо-заголовком после обычного", ::frame(0x01, 0x05, 1, ::block({{":method", "GET"}, {":scheme", "https"}, {"accept", "*/*"}, {":path", "/"}}))},
		// Псевдо-заголовок ответа в запросе
		{"HEADERS со :status в запросе", ::frame(0x01, 0x05, 1, ::block({{":method", "GET"}, {":scheme", "https"}, {":path", "/"}, {":status", "200"}}))},
		// Пустой путь запроса
		{"HEADERS с пустым :path", ::frame(0x01, 0x05, 1, ::block({{":method", "GET"}, {":scheme", "https"}, {":path", ""}}))},
		// Неизвестный псевдо-заголовок
		{"HEADERS с неизвестным псевдо-заголовком", ::frame(0x01, 0x05, 1, ::block({{":method", "GET"}, {":scheme", "https"}, {":path", "/"}, {":unknown", "x"}}))},
		// Метод CONNECT со схемой и путём
		{"CONNECT со :scheme и :path", ::frame(0x01, 0x04, 1, ::block({{":method", "CONNECT"}, {":scheme", "https"}, {":path", "/"}, {":authority", "example.com"}}))},
		// Расхождение объявленной и фактической длины тела
		{"content-length не совпадает с телом", (::frame(0x01, 0x04, 1, ::block({{":method", "POST"}, {":scheme", "https"}, {":path", "/"}, {"content-length", "10"}})) + ::frame(0x00, 0x01, 1, "short"))},
		// Секция трейлеров без завершения потока
		{"трейлеры без END_STREAM", (::frame(0x01, 0x04, 1, request) + ::frame(0x01, 0x04, 1, ::block({{"x-trailer", "value"}})))},
		// Псевдо-заголовок в секции трейлеров
		{"псевдо-заголовок в трейлерах", (::frame(0x01, 0x04, 1, request) + ::frame(0x01, 0x05, 1, ::block({{":method", "GET"}})))},
		// Длина заполнения превышает длину кадра
		{"DATA с избыточным padding", (::frame(0x01, 0x04, 1, request) + ::frame(0x00, 0x08, 1, std::string(1, '\xFF') + "body"))},
		// Некорректный индекс HPACK
		{"HPACK с недопустимым индексом", ::frame(0x01, 0x05, 1, std::string(1, '\xFE'))},
		// Обновление размера таблицы сверх анонсированного
		{"HPACK: размер таблицы сверх лимита", ::frame(0x01, 0x05, 1, std::string("\x3F\xE1\x1F", 3) + request)},
		// Усечённый блок заголовков
		{"усечённый блок HPACK", ::frame(0x01, 0x05, 1, std::string("\x40\x0A", 2))}
	};
	// Количество расхождений реакции
	size_t diffs = 0;
	// Печатаем шапку таблицы
	::printf("%-45s %-14s %-14s\n", "случай", "наш парсер", "nghttp2");
	/**
	 * Выполняем перебор всех проверяемых случаев
	 */
	for(const auto & item : cases){
		// Формируем полный поток байт случая
		const std::string input = (preface + item.second);
		// Получаем реакцию нашего парсера
		const std::string mine = ::ours(&fmk, &log, input);
		// Получаем реакцию эталонной реализации
		const std::string peer = ::reference(input);
		// Печатаем строку сравнения
		::printf("%-45s %-14s %-14s%s\n", item.first.c_str(), mine.c_str(), peer.c_str(), ((mine.compare(peer) != 0) ? "  <-- расхождение" : ""));
		// Если реакции разошлись
		if(mine.compare(peer) != 0)
			// Наращиваем счётчик расхождений
			diffs++;
	}
	// Печатаем итог сверки
	::printf("\nслучаев: %zu, расхождений: %zu\n", cases.size(), diffs);
	// Выводим результат
	return EXIT_SUCCESS;
}
