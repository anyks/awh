/**
 * @file: nghttp3-session.cpp
 * @date: 2026-07-27
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Пробник сверки парсера сессии HTTP/3 с эталонной реализацией nghttp3 — обе роли
 *        эндпоинта проверяются против чужой сессии на полном обмене сообщениями
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Сверка ведётся в обе стороны: наш клиент против сессии сервера nghttp3 и наш
 * сервер против сессии клиента nghttp3. Это единственная проверка, где вторая
 * сторона написана не нами, поэтому она ловит расхождения в прочтении RFC,
 * которые собственные тесты повторили бы вместе с ошибкой.
 *
 * Транспорт QUIC эмулируется очередями: обе стороны обмениваются байтами потоков
 * без потерь и переупорядочивания. Проверяется именно слой HTTP/3, а не транспорт.
 */

#include <map>
#include <deque>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdint>
#include <cstring>

#include <nghttp3/nghttp3.h>

#include <proto/http/parser/http3/http.hpp>

using namespace std;
using namespace awh;
using namespace awh::http;
using namespace awh::http::h3;

/**
 * @brief Структура собранного сообщения одной стороны
 *
 */
typedef struct Message {
	// Поля секции заголовков в порядке приёма
	vector <pair <string, string>> headers;
	// Поля секции трейлеров в порядке приёма
	vector <pair <string, string>> trailers;
	// Собранное тело сообщения
	string body;
	// Признак полного приёма сообщения
	bool completed = false;
} message_t;

/**
 * @brief Структура состояния эталонной стороны соединения
 *
 */
typedef struct Reference {
	// Объект сессии эталонной реализации
	nghttp3_conn * conn = nullptr;
	// Очередь исходящих данных (поток, данные, признак завершения)
	deque <tuple <int64_t, string, bool>> queue;
	// Собранные сообщения по потокам
	map <int64_t, message_t> messages;
	// Количество обнаруженных расхождений
	size_t failures = 0;
} reference_t;

/**
 * @brief Структура состояния нашей стороны соединения
 *
 */
typedef struct Local {
	// Объект парсера библиотеки
	parser_http3_t * parser = nullptr;
	// Очередь исходящих данных (поток, данные, признак завершения)
	deque <tuple <uint64_t, string, bool>> queue;
	// Собранные сообщения по потокам
	map <uint64_t, message_t> messages;
	// Идентификатор следующего выдаваемого однонаправленного потока
	int64_t unistream = 0;
	// Количество обнаруженных расхождений
	size_t failures = 0;
} local_t;

/**
 * @brief Набор полей запроса, отправляемого в обеих сверках
 *
 */
static const vector <pair <string, string>> REQUEST = {
	{":method", "POST"},
	{":scheme", "https"},
	{":authority", "www.example.com"},
	{":path", "/api/v1/submit"},
	{"content-type", "application/json"},
	{"content-length", "27"},
	{"user-agent", "awh/5.0 (interop)"},
	{"accept-encoding", "gzip, deflate, br"}
};
/**
 * @brief Набор полей ответа, отправляемого в обеих сверках
 *
 */
static const vector <pair <string, string>> RESPONSE = {
	{":status", "200"},
	{"content-type", "application/json"},
	{"cache-control", "no-store"},
	{"server", "awh"},
	{"x-request-id", "0f9c1a2b-3d4e-5f60-7a8b-9c0d1e2f3a4b"}
};
/**
 * @brief Тело сообщения, отправляемое в обеих сверках
 *
 */
static const string BODY = "{\"status\":\"ok\",\"count\":7}\r\n";

/**
 * @brief Функция приведения набора полей к представлению эталонной реализации
 *
 * @param fields  набор полей
 * @param storage хранилище представлений полей
 * @return        набор полей в представлении эталонной реализации
 *
 */
static vector <nghttp3_nv> convert(const vector <pair <string, string>> & fields) noexcept {
	// Собираемый набор полей в представлении эталонной реализации
	vector <nghttp3_nv> result;
	// Выделяем место под поля набора
	result.reserve(fields.size());
	/**
	 * Выполняем перебор всех полей набора
	 */
	for(const auto & field : fields){
		// Собираемое поле в представлении эталонной реализации
		nghttp3_nv nv;
		// Устанавливаем название поля
		nv.name = reinterpret_cast <uint8_t *> (const_cast <char *> (field.first.data()));
		// Устанавливаем длину названия поля
		nv.namelen = field.first.size();
		// Устанавливаем значение поля
		nv.value = reinterpret_cast <uint8_t *> (const_cast <char *> (field.second.data()));
		// Устанавливаем длину значения поля
		nv.valuelen = field.second.size();
		// Устанавливаем флаги поля
		nv.flags = NGHTTP3_NV_FLAG_NONE;
		// Дописываем поле в набор
		result.push_back(nv);
	}
	// Выводим собранный набор полей
	return result;
}
/**
 * @brief Функция сверки двух наборов полей
 *
 * @param title    название сверки для диагностики
 * @param expected ожидаемый набор полей
 * @param received принятый набор полей
 * @param failures количество обнаруженных расхождений
 *
 */
static void compare(const char * title, const vector <pair <string, string>> & expected, const vector <pair <string, string>> & received, size_t & failures) noexcept {
	// Если количество полей разошлось
	if(expected.size() != received.size()){
		// Выводим описание расхождения
		::printf("%s: отправлено %zu полей, получено %zu\n", title, expected.size(), received.size());
		// Считаем обнаруженное расхождение
		failures++;
		// Выходим из функции
		return;
	}
	/**
	 * Выполняем сверку всех полей набора
	 */
	for(size_t i = 0; i < expected.size(); i++){
		// Если название либо значение поля разошлись
		if((expected[i].first != received[i].first) || (expected[i].second != received[i].second)){
			// Выводим описание расхождения
			::printf(
				"%s, поле %zu: отправлено [%s] = [%s], получено [%s] = [%s]\n",
				title, i, expected[i].first.c_str(), expected[i].second.c_str(),
				received[i].first.c_str(), received[i].second.c_str()
			);
			// Считаем обнаруженное расхождение
			failures++;
		}
	}
}

/**
 * @brief Функция обработки принятого поля секции эталонной реализацией
 *
 * @param sid      идентификатор потока
 * @param name     название поля
 * @param value    значение поля
 * @param user     пользовательские данные соединения
 * @param trailer  признак секции трейлеров
 * @return         результат обработки
 *
 */
static int onHeader(nghttp3_conn *, int64_t sid, int32_t, nghttp3_rcbuf * name, nghttp3_rcbuf * value, uint8_t, void * user, void *) noexcept {
	// Получаем состояние эталонной стороны соединения
	reference_t * side = reinterpret_cast <reference_t *> (user);
	// Получаем представление названия поля
	const nghttp3_vec key = ::nghttp3_rcbuf_get_buf(name);
	// Получаем представление значения поля
	const nghttp3_vec val = ::nghttp3_rcbuf_get_buf(value);
	// Дописываем поле в секцию заголовков сообщения
	side->messages[sid].headers.emplace_back(
		string(reinterpret_cast <const char *> (key.base), key.len),
		string(reinterpret_cast <const char *> (val.base), val.len)
	);
	// Продолжаем разбор
	return 0;
}
/**
 * @brief Функция обработки принятого поля секции трейлеров эталонной реализацией
 *
 * @param sid   идентификатор потока
 * @param name  название поля
 * @param value значение поля
 * @param user  пользовательские данные соединения
 * @return      результат обработки
 *
 */
static int onTrailer(nghttp3_conn *, int64_t sid, int32_t, nghttp3_rcbuf * name, nghttp3_rcbuf * value, uint8_t, void * user, void *) noexcept {
	// Получаем состояние эталонной стороны соединения
	reference_t * side = reinterpret_cast <reference_t *> (user);
	// Получаем представление названия поля
	const nghttp3_vec key = ::nghttp3_rcbuf_get_buf(name);
	// Получаем представление значения поля
	const nghttp3_vec val = ::nghttp3_rcbuf_get_buf(value);
	// Дописываем поле в секцию трейлеров сообщения
	side->messages[sid].trailers.emplace_back(
		string(reinterpret_cast <const char *> (key.base), key.len),
		string(reinterpret_cast <const char *> (val.base), val.len)
	);
	// Продолжаем разбор
	return 0;
}
/**
 * @brief Функция обработки принятых данных тела эталонной реализацией
 *
 * @param sid  идентификатор потока
 * @param data буфер данных тела
 * @param size размер данных тела
 * @param user пользовательские данные соединения
 * @return     результат обработки
 *
 */
static int onData(nghttp3_conn *, int64_t sid, const uint8_t * data, size_t size, void * user, void *) noexcept {
	// Получаем состояние эталонной стороны соединения
	reference_t * side = reinterpret_cast <reference_t *> (user);
	// Дописываем данные в тело сообщения
	side->messages[sid].body.append(reinterpret_cast <const char *> (data), size);
	// Продолжаем разбор
	return 0;
}
/**
 * @brief Функция обработки завершения потока эталонной реализацией
 *
 * @param sid  идентификатор потока
 * @param user пользовательские данные соединения
 * @return     результат обработки
 *
 */
static int onEnd(nghttp3_conn *, int64_t sid, void * user, void *) noexcept {
	// Получаем состояние эталонной стороны соединения
	reference_t * side = reinterpret_cast <reference_t *> (user);
	// Запоминаем полный приём сообщения
	side->messages[sid].completed = true;
	// Продолжаем разбор
	return 0;
}

/**
 * @brief Функция выдачи тела сообщения эталонной реализации
 *
 * @details Без источника тела эталон завершил бы поток сразу после секции полей,
 *          а объявленная в ней длина тела осталась бы неподтверждённой: наш парсер
 *          справедливо отверг бы такое сообщение
 *
 * @param vec    векторы выдаваемого тела
 * @param veccnt количество векторов
 * @param flags  флаги выдачи тела
 * @return       количество заполненных векторов
 *
 */
static nghttp3_ssize onRead(nghttp3_conn *, int64_t, nghttp3_vec * vec, size_t veccnt, uint32_t * flags, void *, void *) noexcept {
	// Если векторов для выдачи тела не предоставлено
	if(veccnt == 0)
		// Тело выдать некуда
		return 0;
	// Устанавливаем указатель на тело сообщения
	vec[0].base = reinterpret_cast <uint8_t *> (const_cast <char *> (BODY.data()));
	// Устанавливаем длину тела сообщения
	vec[0].len = BODY.size();
	// Помечаем тело выданным целиком
	(* flags) = NGHTTP3_DATA_FLAG_EOF;
	// Выводим количество заполненных векторов
	return 1;
}
/**
 * @brief Функция получения источника тела сообщения эталонной реализации
 *
 * @return источник тела сообщения
 *
 */
static nghttp3_data_reader reader() noexcept {
	// Собираемый источник тела сообщения
	nghttp3_data_reader result;
	// Устанавливаем функцию выдачи тела сообщения
	result.read_data = ::onRead;
	// Выводим собранный источник тела сообщения
	return result;
}
/**
 * @brief Функция подготовки набора функций обратного вызова эталонной реализации
 *
 * @return собранный набор функций обратного вызова
 *
 */
static nghttp3_callbacks callbacks() noexcept {
	// Собираемый набор функций обратного вызова
	nghttp3_callbacks result;
	// Обнуляем набор функций обратного вызова
	::memset(&result, 0, sizeof(result));
	// Устанавливаем функцию обработки принятого поля секции заголовков
	result.recv_header = ::onHeader;
	// Устанавливаем функцию обработки принятого поля секции трейлеров
	result.recv_trailer = ::onTrailer;
	// Устанавливаем функцию обработки принятых данных тела
	result.recv_data = ::onData;
	// Устанавливаем функцию обработки завершения потока
	result.end_stream = ::onEnd;
	// Выводим собранный набор функций обратного вызова
	return result;
}
/**
 * @brief Функция выгрузки исходящих данных сессии эталонной реализации
 *
 * @param side состояние эталонной стороны соединения
 *
 */
static void drain(reference_t & side) noexcept {
	/**
	 * Выгружаем исходящие данные, пока сессия их отдаёт: за один вызов сессия
	 * возвращает данные только одного потока
	 */
	for(size_t guard = 0; guard < 256; guard++){
		// Векторы исходящих данных потока
		nghttp3_vec vec[8];
		// Идентификатор потока исходящих данных
		int64_t sid = -1;
		// Признак завершения потока
		int fin = 0;
		// Выполняем выгрузку исходящих данных сессии
		const nghttp3_ssize count = ::nghttp3_conn_writev_stream(side.conn, &sid, &fin, vec, 8);
		// Если выгрузка не удалась
		if(count < 0){
			// Выводим описание отказа эталонной реализации
			::printf("эталон не смог выгрузить исходящие данные: %s\n", ::nghttp3_strerror(static_cast <int> (count)));
			// Считаем обнаруженное расхождение
			side.failures++;
			// Прекращаем выгрузку
			return;
		}
		// Если исходящих данных больше нет
		if((count == 0) && (fin == 0))
			// Прекращаем выгрузку
			return;
		// Собираемая порция исходящих данных потока
		string chunk;
		// Суммарный размер выгруженных данных
		size_t total = 0;
		/**
		 * Выполняем сборку всех векторов исходящих данных
		 */
		for(nghttp3_ssize i = 0; i < count; i++){
			// Дописываем очередной вектор в порцию исходящих данных
			chunk.append(reinterpret_cast <const char *> (vec[i].base), vec[i].len);
			// Наращиваем суммарный размер выгруженных данных
			total += vec[i].len;
		}
		// Складываем порцию исходящих данных в очередь транспорта
		side.queue.emplace_back(sid, chunk, (fin != 0));
		// Извещаем сессию о принятых транспортом данных
		if(::nghttp3_conn_add_write_offset(side.conn, sid, total) != 0){
			// Выводим описание отказа эталонной реализации
			::printf("эталон отверг подтверждение отправки\n");
			// Считаем обнаруженное расхождение
			side.failures++;
			// Прекращаем выгрузку
			return;
		}
	}
}
/**
 * @brief Функция прокачки очередей между нашей стороной и эталонной
 *
 * @param local     состояние нашей стороны соединения
 * @param reference состояние эталонной стороны соединения
 *
 */
static void pump(local_t & local, reference_t & reference) noexcept {
	/**
	 * Прокачка ограничена сверху: обе стороны вправе отвечать на разбор отправкой,
	 * и без границы переписка могла бы не закончиться
	 */
	for(size_t guard = 0; guard < 256; guard++){
		// Выгружаем исходящие данные сессии эталонной реализации
		::drain(reference);
		// Признак того, что на этом обороте что-то передавалось
		bool moved = false;
		/**
		 * Передаём накопленные нашей стороной данные эталонной реализации
		 */
		while(!local.queue.empty()){
			// Забираем очередную порцию исходящих данных
			const auto item = local.queue.front();
			// Удаляем порцию из очереди транспорта
			local.queue.pop_front();
			// Подаём порцию на разбор эталонной реализации
			const nghttp3_ssize read = ::nghttp3_conn_read_stream(
				reference.conn, static_cast <int64_t> (get <0> (item)),
				reinterpret_cast <const uint8_t *> (get <1> (item).data()),
				get <1> (item).size(), (get <2> (item) ? 1 : 0)
			);
			// Если эталонная реализация отвергла данные
			if(read < 0){
				// Выводим описание расхождения
				::printf("эталон отверг поток %llu: %s\n", static_cast <unsigned long long> (get <0> (item)), ::nghttp3_strerror(static_cast <int> (read)));
				// Считаем обнаруженное расхождение
				reference.failures++;
				// Прекращаем прокачку
				return;
			}
			// Запоминаем, что передача состоялась
			moved = true;
		}
		// Выгружаем исходящие данные сессии эталонной реализации
		::drain(reference);
		/**
		 * Передаём накопленные эталонной реализацией данные нашей стороне
		 */
		while(!reference.queue.empty()){
			// Забираем очередную порцию исходящих данных
			const auto item = reference.queue.front();
			// Удаляем порцию из очереди транспорта
			reference.queue.pop_front();
			// Подаём порцию на разбор нашей стороне
			local.parser->parse(
				static_cast <uint64_t> (get <0> (item)), get <1> (item).data(), get <1> (item).size(), get <2> (item)
			);
			// Запоминаем, что передача состоялась
			moved = true;
		}
		// Если передавать больше нечего
		if(!moved)
			// Прекращаем прокачку
			break;
	}
}
/**
 * @brief Функция подписки нашей стороны соединения на функции обратного вызова
 *
 * @param local состояние нашей стороны соединения
 *
 */
static void attach(local_t & local) noexcept {
	// Устанавливаем функцию обратного вызова открытия однонаправленного потока
	local.parser->on(parser_http3_t::open_callback_t([&local]() noexcept -> int64_t {
		// Выделяем идентификатор однонаправленного потока
		const int64_t sid = local.unistream;
		// Продвигаем идентификатор следующего однонаправленного потока
		local.unistream += 4;
		// Выводим идентификатор открытого потока
		return sid;
	}));
	// Устанавливаем функцию обратного вызова записи исходящих байтов потока
	local.parser->on(parser_http3_t::write_callback_t([&local](const uint64_t sid, const void * buffer, const size_t size, const bool fin) noexcept {
		// Складываем исходящие байты в очередь транспорта
		local.queue.emplace_back(sid, string(reinterpret_cast <const char *> (buffer), size), fin);
	}));
	// Устанавливаем функцию обратного вызова ошибки уровня соединения
	local.parser->on(parser_http3_t::error_callback_t([&local](const parser_http3_t::error_t code, const string_view message) noexcept {
		// Выводим описание расхождения
		::printf("наша сторона оборвала соединение: %s (%.*s)\n", h3::errorName(code).data(), static_cast <int> (message.size()), message.data());
		// Считаем обнаруженное расхождение
		local.failures++;
	}));
	// Устанавливаем функцию обратного вызова обрыва потока
	local.parser->on(parser_http3_t::abort_callback_t([&local](const uint64_t sid, const parser_http3_t::error_t code, const bool stop) noexcept {
		/**
		 * Остановка приёма однонаправленного потока неизвестного типа штатна:
		 * эталон намеренно отправляет такие потоки
		 */
		if(stop && ((sid & 0x02) != 0))
			// Расхождением это не является
			return;
		// Выводим описание расхождения
		::printf("наша сторона оборвала поток %llu: %s\n", static_cast <unsigned long long> (sid), h3::errorName(code).data());
		// Считаем обнаруженное расхождение
		local.failures++;
	}));
	// Устанавливаем функцию обратного вызова поля секции
	local.parser->on(parser_http3_t::header_callback_t([&local](const uint64_t sid, const string_view name, const string_view value, const parser_t::part_t part) noexcept -> bool {
		// Если поле принадлежит секции трейлеров
		if(part == parser_t::part_t::TRAILER)
			// Дописываем поле в секцию трейлеров сообщения
			local.messages[sid].trailers.emplace_back(string(name), string(value));
		// Если поле принадлежит секции заголовков
		else local.messages[sid].headers.emplace_back(string(name), string(value));
		// Продолжаем разбор
		return true;
	}));
	// Устанавливаем функцию обратного вызова фрагмента тела потока
	local.parser->on(parser_http3_t::data_callback_t([&local](const uint64_t sid, const void * buffer, const size_t size, const bool) noexcept -> bool {
		// Дописываем данные в тело сообщения
		local.messages[sid].body.append(reinterpret_cast <const char *> (buffer), size);
		// Продолжаем разбор
		return true;
	}));
	// Устанавливаем функцию обратного вызова фазы приёма сообщения потока
	local.parser->on(parser_http3_t::phase_callback_t([&local](const uint64_t sid, const parser_t::phase_t phase, const parser_t::part_t part) noexcept -> bool {
		// Если сообщение потока принято целиком
		if((phase == parser_t::phase_t::END) && (part == parser_t::part_t::NONE))
			// Запоминаем полный приём сообщения
			local.messages[sid].completed = true;
		// Продолжаем разбор
		return true;
	}));
}

/**
 * @brief Функция сверки нашего клиента с сессией сервера эталонной реализации
 *
 * @return количество обнаруженных расхождений
 *
 */
static size_t clientAgainstReference() noexcept {
	// Получаем аллокатор эталонной реализации
	const nghttp3_mem * mem = ::nghttp3_mem_default();
	// Состояние эталонной стороны соединения
	reference_t reference;
	// Состояние нашей стороны соединения
	local_t local;
	// Набор функций обратного вызова эталонной реализации
	nghttp3_callbacks handlers = ::callbacks();
	// Параметры сессии эталонной реализации
	nghttp3_settings settings;
	// Заполняем параметры сессии значениями по умолчанию
	::nghttp3_settings_default(&settings);
	// Устанавливаем ёмкость динамической таблицы QPACK
	settings.qpack_max_dtable_capacity = 4096;
	// Устанавливаем число потоков, которым разрешено ожидать пополнения таблицы
	settings.qpack_blocked_streams = 16;
	// Создаём сессию сервера эталонной реализации
	if(::nghttp3_conn_server_new(&reference.conn, &handlers, &settings, mem, &reference) != 0){
		// Выводим сообщение о неудачном создании сессии
		::printf("сессия сервера эталонной реализации не создана\n");
		// Выводим количество обнаруженных расхождений
		return 1;
	}
	/**
	 * Однонаправленные потоки сервера нумеруются как 3, 7, 11 (RFC 9000 §2.1)
	 */
	::nghttp3_conn_bind_control_stream(reference.conn, 3);
	// Привязываем потоки инструкций кодека QPACK сессии сервера
	::nghttp3_conn_bind_qpack_streams(reference.conn, 7, 11);
	// Объект парсера клиента библиотеки
	parser_http3_t parser(direct_t::RESPONSE, nullptr, nullptr);
	// Устанавливаем объект парсера нашей стороны соединения
	local.parser = &parser;
	// Однонаправленные потоки клиента нумеруются как 2, 6, 10
	local.unistream = 2;
	// Подписываем нашу сторону на функции обратного вызова
	::attach(local);
	// Отправляем параметры соединения
	parser.sendSettings();
	// Выполняем прокачку очередей
	::pump(local, reference);
	// Отправляем секцию полей запроса на двунаправленном потоке клиента
	parser.sendHeaders(0, [](){
		// Собираемый набор полей запроса
		vector <qpack::field_t> result;
		/**
		 * Выполняем сбор всех полей запроса
		 */
		for(const auto & field : REQUEST)
			// Дописываем поле в набор
			result.emplace_back(field.first, field.second);
		// Выводим собранный набор полей запроса
		return result;
	}(), false);
	// Отправляем тело запроса вместе с завершением потока
	parser.sendData(0, BODY.data(), BODY.size(), true);
	// Выполняем прокачку очередей
	::pump(local, reference);
	// Сверяем принятую эталоном секцию полей запроса
	::compare("клиент -> эталон, заголовки запроса", REQUEST, reference.messages[0].headers, reference.failures);
	// Если тело запроса разошлось
	if(reference.messages[0].body != BODY){
		// Выводим описание расхождения
		::printf("клиент -> эталон: тело запроса разошлось\n");
		// Считаем обнаруженное расхождение
		reference.failures++;
	}
	// Если эталон не считает сообщение принятым целиком
	if(!reference.messages[0].completed){
		// Выводим описание расхождения
		::printf("клиент -> эталон: сообщение не принято целиком\n");
		// Считаем обнаруженное расхождение
		reference.failures++;
	}
	// Собираем набор полей ответа в представлении эталонной реализации
	vector <nghttp3_nv> response = ::convert(RESPONSE);
	// Отправляем ответ сервера эталонной реализацией
	// Получаем источник тела ответа сервера
	const nghttp3_data_reader source = ::reader();
	// Отправляем ответ сервера вместе с телом
	if(::nghttp3_conn_submit_response(reference.conn, 0, response.data(), response.size(), &source) != 0){
		// Выводим описание расхождения
		::printf("эталон не смог отправить ответ\n");
		// Считаем обнаруженное расхождение
		reference.failures++;
	}
	// Выполняем прокачку очередей
	::pump(local, reference);
	// Сверяем принятую нами секцию полей ответа
	::compare("эталон -> клиент, заголовки ответа", RESPONSE, local.messages[0].headers, local.failures);
	// Если тело ответа разошлось
	if(local.messages[0].body != BODY){
		// Выводим описание расхождения
		::printf("эталон -> клиент: тело ответа разошлось\n");
		// Считаем обнаруженное расхождение
		local.failures++;
	}
	// Если наша сторона не считает сообщение принятым целиком
	if(!local.messages[0].completed){
		// Выводим описание расхождения
		::printf("эталон -> клиент: сообщение не принято целиком\n");
		// Считаем обнаруженное расхождение
		local.failures++;
	}
	// Удаляем сессию эталонной реализации
	::nghttp3_conn_del(reference.conn);
	// Выводим количество обнаруженных расхождений
	return (reference.failures + local.failures);
}
/**
 * @brief Функция сверки нашего сервера с сессией клиента эталонной реализации
 *
 * @return количество обнаруженных расхождений
 *
 */
static size_t serverAgainstReference() noexcept {
	// Получаем аллокатор эталонной реализации
	const nghttp3_mem * mem = ::nghttp3_mem_default();
	// Состояние эталонной стороны соединения
	reference_t reference;
	// Состояние нашей стороны соединения
	local_t local;
	// Набор функций обратного вызова эталонной реализации
	nghttp3_callbacks handlers = ::callbacks();
	// Параметры сессии эталонной реализации
	nghttp3_settings settings;
	// Заполняем параметры сессии значениями по умолчанию
	::nghttp3_settings_default(&settings);
	// Устанавливаем ёмкость динамической таблицы QPACK
	settings.qpack_max_dtable_capacity = 4096;
	// Устанавливаем число потоков, которым разрешено ожидать пополнения таблицы
	settings.qpack_blocked_streams = 16;
	// Создаём сессию клиента эталонной реализации
	if(::nghttp3_conn_client_new(&reference.conn, &handlers, &settings, mem, &reference) != 0){
		// Выводим сообщение о неудачном создании сессии
		::printf("сессия клиента эталонной реализации не создана\n");
		// Выводим количество обнаруженных расхождений
		return 1;
	}
	// Однонаправленные потоки клиента нумеруются как 2, 6, 10
	::nghttp3_conn_bind_control_stream(reference.conn, 2);
	// Привязываем потоки инструкций кодека QPACK сессии клиента
	::nghttp3_conn_bind_qpack_streams(reference.conn, 6, 10);
	// Объект парсера сервера библиотеки
	parser_http3_t parser(direct_t::REQUEST, nullptr, nullptr);
	// Устанавливаем объект парсера нашей стороны соединения
	local.parser = &parser;
	// Однонаправленные потоки сервера нумеруются как 3, 7, 11
	local.unistream = 3;
	// Подписываем нашу сторону на функции обратного вызова
	::attach(local);
	// Отправляем параметры соединения
	parser.sendSettings();
	// Выполняем прокачку очередей
	::pump(local, reference);
	// Собираем набор полей запроса в представлении эталонной реализации
	vector <nghttp3_nv> request = ::convert(REQUEST);
	// Отправляем запрос клиента эталонной реализацией
	// Получаем источник тела запроса клиента
	const nghttp3_data_reader source = ::reader();
	// Отправляем запрос клиента вместе с телом
	if(::nghttp3_conn_submit_request(reference.conn, 0, request.data(), request.size(), &source, nullptr) != 0){
		// Выводим описание расхождения
		::printf("эталон не смог отправить запрос\n");
		// Считаем обнаруженное расхождение
		reference.failures++;
	}
	// Выполняем прокачку очередей
	::pump(local, reference);
	/**
	 * Запрос без тела эталон завершает сразу, поэтому объявленная длина тела
	 * в наборе полей к сверке не относится: сверяем сами поля
	 */
	::compare("эталон -> сервер, заголовки запроса", REQUEST, local.messages[0].headers, local.failures);
	// Если тело запроса разошлось
	if(local.messages[0].body != BODY){
		// Выводим описание расхождения
		::printf("эталон -> сервер: тело запроса разошлось\n");
		// Считаем обнаруженное расхождение
		local.failures++;
	}
	// Отправляем секцию полей ответа сервера
	parser.sendHeaders(0, [](){
		// Собираемый набор полей ответа
		vector <qpack::field_t> result;
		/**
		 * Выполняем сбор всех полей ответа
		 */
		for(const auto & field : RESPONSE)
			// Дописываем поле в набор
			result.emplace_back(field.first, field.second);
		// Выводим собранный набор полей ответа
		return result;
	}(), false);
	// Отправляем тело ответа
	parser.sendData(0, BODY.data(), BODY.size(), false);
	// Собираем секцию трейлеров ответа
	const vector <qpack::field_t> trailers = {qpack::field_t{"x-checksum", "deadbeef"}};
	// Отправляем секцию трейлеров вместе с завершением потока
	parser.sendHeaders(0, trailers, true);
	// Выполняем прокачку очередей
	::pump(local, reference);
	// Сверяем принятую эталоном секцию полей ответа
	::compare("сервер -> эталон, заголовки ответа", RESPONSE, reference.messages[0].headers, reference.failures);
	// Сверяем принятую эталоном секцию трейлеров
	::compare("сервер -> эталон, трейлеры ответа", {{"x-checksum", "deadbeef"}}, reference.messages[0].trailers, reference.failures);
	// Если тело ответа разошлось
	if(reference.messages[0].body != BODY){
		// Выводим описание расхождения
		::printf("сервер -> эталон: тело ответа разошлось\n");
		// Считаем обнаруженное расхождение
		reference.failures++;
	}
	// Если эталон не считает сообщение принятым целиком
	if(!reference.messages[0].completed){
		// Выводим описание расхождения
		::printf("сервер -> эталон: сообщение не принято целиком\n");
		// Считаем обнаруженное расхождение
		reference.failures++;
	}
	// Удаляем сессию эталонной реализации
	::nghttp3_conn_del(reference.conn);
	// Выводим количество обнаруженных расхождений
	return (reference.failures + local.failures);
}
/**
 * @brief Функция входа в пробник
 *
 * @return код выхода из пробника
 *
 */
int32_t main() noexcept {
	// Количество обнаруженных расхождений
	size_t failures = 0;
	// Выполняем сверку нашего клиента с сессией сервера эталонной реализации
	failures += ::clientAgainstReference();
	// Выполняем сверку нашего сервера с сессией клиента эталонной реализации
	failures += ::serverAgainstReference();
	// Выводим итог сверки
	::printf("сверено сессий: 2, расхождений: %zu\n", failures);
	// Выводим код выхода по итогу сверки
	return (failures > 0 ? 1 : 0);
}
