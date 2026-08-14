/**
 * @file nghttp3-negative.cpp
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
 * @brief Пробник дифференциальной сверки обработки ошибок HTTP/3 с эталонной реализацией nghttp3 —
 *        подача одного набора некорректных потоков обоим серверным парсерам и сравнение кода реакции
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Пробник: дифференциальная сверка обработки ошибок с эталонной реализацией.
 * Один и тот же набор потоков подаётся нашему серверному парсеру и серверной
 * сессии nghttp3, после чего сравнивается код ошибки, которым каждая сторона
 * ответила на некорректные данные.
 *
 * Сверяется именно код ошибки, а не уровень реакции. Причина в устройстве
 * эталонного API: nghttp3 сам решает уровень только для однонаправленных потоков
 * неизвестного типа, а во всех прочих случаях возвращает код ошибки наружу
 * и оставляет приложению выбор - оборвать соединение или сбросить поток.
 * У нас этот выбор сделан внутри парсера, поэтому уровень выводится справочно
 * в скобках и на счётчик расхождений не влияет: сравнивать его было бы
 * сравнением нашей реализации с чужим примером использования, а не с RFC.
 */

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
 * @brief Структура порции байт одного потока
 *
 */
typedef struct Chunk {
	// Идентификатор потока
	uint64_t sid;
	// Подаваемые байты потока
	string data;
	// Признак завершения потока
	bool fin;
} chunk_t;

/**
 * @brief Структура проверяемого случая
 *
 */
typedef struct Case {
	// Название случая
	string title;
	// Признак подстановки корректного начала соединения
	bool prelude;
	// Порции байт потоков в порядке подачи
	vector <chunk_t> chunks;
} case_t;

/**
 * @brief Структура реакции эндпоинта на некорректные данные
 *
 */
typedef struct Reaction {
	// Признак наличия реакции
	bool present = false;
	// Признак реакции уровня соединения
	bool connection = false;
	// Код ошибки протокола
	uint64_t code = 0;
} reaction_t;

/**
 * @brief Функция кодирования целого переменной длины QUIC (RFC 9000 §16)
 *
 * @param value кодируемое значение
 * @return      закодированное значение
 *
 */
static string varint(const uint64_t value) noexcept {
	// Результат работы функции - закодированное значение
	string result;
	// Если значение помещается в один октет
	if(value <= 0x3F)
		// Дописываем однооктетное представление
		result.push_back(static_cast <char> (value & 0xFF));
	// Если значение помещается в два октета
	else if(value <= 0x3FFF) {
		// Дописываем старший октет с признаком длины
		result.push_back(static_cast <char> (0x40 | ((value >> 8) & 0x3F)));
		// Дописываем младший октет
		result.push_back(static_cast <char> (value & 0xFF));
	// Если значение помещается в четыре октета
	} else if(value <= 0x3FFFFFFF) {
		// Дописываем старший октет с признаком длины
		result.push_back(static_cast <char> (0x80 | ((value >> 24) & 0x3F)));
		/**
		 * Выполняем дозапись оставшихся октетов значения
		 */
		for(int32_t shift = 16; shift >= 0; shift -= 8)
			// Дописываем очередной октет значения
			result.push_back(static_cast <char> ((value >> shift) & 0xFF));
	// Если значение требует восьми октетов
	} else {
		// Дописываем старший октет с признаком длины
		result.push_back(static_cast <char> (0xC0 | ((value >> 56) & 0x3F)));
		/**
		 * Выполняем дозапись оставшихся октетов значения
		 */
		for(int32_t shift = 48; shift >= 0; shift -= 8)
			// Дописываем очередной октет значения
			result.push_back(static_cast <char> ((value >> shift) & 0xFF));
	}
	// Выводим закодированное значение
	return result;
}
/**
 * @brief Функция кодирования целого с префиксом (RFC 7541 §5.1)
 *
 * @details Кодек целых с префиксом собран здесь заново, а не взят из библиотеки:
 *          пробник обязан подавать байты, собранные независимо от проверяемого кода,
 *          иначе ошибка в кодеке скрыла бы сама себя
 *
 * @param value   кодируемое значение
 * @param bits    разрядность префикса
 * @param pattern биты старшей части первого октета
 * @return        закодированное значение
 *
 */
static string prefixed(const uint64_t value, const uint8_t bits, const uint8_t pattern) noexcept {
	// Результат работы функции - закодированное значение
	string result;
	// Наибольшее значение, помещающееся в префикс
	const uint64_t limit = ((1ULL << bits) - 1);
	// Если значение помещается в префикс целиком
	if(value < limit){
		// Дописываем значение прямо в префикс
		result.push_back(static_cast <char> (pattern | static_cast <uint8_t> (value)));
		// Выводим закодированное значение
		return result;
	}
	// Заполняем префикс единицами - признак продолжения значения
	result.push_back(static_cast <char> (pattern | static_cast <uint8_t> (limit)));
	// Остаток значения сверх префикса
	uint64_t rest = (value - limit);
	/**
	 * Выполняем дозапись остатка значения семибитными группами
	 */
	while(rest >= 128){
		// Дописываем очередную группу с признаком продолжения
		result.push_back(static_cast <char> ((rest & 0x7F) | 0x80));
		// Сдвигаем остаток значения на разобранную группу
		rest >>= 7;
	}
	// Дописываем последнюю группу остатка
	result.push_back(static_cast <char> (rest & 0x7F));
	// Выводим закодированное значение
	return result;
}
/**
 * @brief Функция сборки кадра HTTP/3
 *
 * @param type    тип кадра
 * @param payload полезная нагрузка кадра
 * @return        собранный кадр
 *
 */
static string frame(const uint64_t type, const string & payload) noexcept {
	// Выводим собранный кадр
	return (::varint(type) + ::varint(payload.size()) + payload);
}
/**
 * @brief Функция сборки полезной нагрузки кадра SETTINGS
 *
 * @param values набор параметров соединения
 * @return       собранная полезная нагрузка
 *
 */
static string settings(const vector <pair <uint64_t, uint64_t>> & values) noexcept {
	// Результат работы функции - собранная полезная нагрузка
	string result;
	/**
	 * Выполняем перебор всех параметров соединения
	 */
	for(const auto & value : values)
		// Дописываем идентификатор параметра и его значение
		result.append(::varint(value.first) + ::varint(value.second));
	// Выводим собранную полезную нагрузку
	return result;
}
/**
 * @brief Функция кодирования секции полей
 *
 * @details Кодирование ведётся без динамической таблицы: секция обязана разбираться
 *          сама по себе, без инструкций потока кодера, иначе проверяемым оказался бы
 *          не тот случай, который описан в названии
 *
 * @param fields кодируемые поля
 * @return       закодированная секция полей
 *
 */
static string block(const vector <pair <string, string>> & fields) noexcept {
	// Создаём объект кодера полей без динамической таблицы
	qpack::encoder_t encoder(0, 0);
	// Список кодируемых полей
	vector <qpack::field_t> items;
	// Резервируем память под кодируемые поля
	items.reserve(fields.size());
	/**
	 * Выполняем перебор всех кодируемых полей
	 */
	for(const auto & field : fields)
		// Дописываем кодируемое поле
		items.emplace_back(field.first, field.second);
	// Буфер закодированной секции полей
	string result;
	// Кодируем секцию полей без сжатия Huffman: читаемость дампа важнее размера
	encoder.encode(0, items, result, false);
	// Выводим закодированную секцию полей
	return result;
}

/**
 * @brief Набор полей корректного запроса
 *
 */
static const vector <pair <string, string>> REQUEST = {
	{":method", "GET"},
	{":scheme", "https"},
	{":authority", "www.example.com"},
	{":path", "/"}
};

/**
 * @brief Функция дополнения строки пробелами до заданной ширины
 *
 * @details Форматирование средствами printf здесь неприменимо: ширину поля оно
 *          считает в октетах, а названия случаев записаны кириллицей, где символ
 *          занимает два октета, и таблица разъезжается. Считаются символы UTF-8:
 *          продолжающие октеты формы 10xxxxxx на ширину не влияют
 *
 * @param text  дополняемая строка
 * @param width требуемая ширина поля
 * @return      дополненная строка
 *
 */
static string pad(const string & text, const size_t width) noexcept {
	// Количество символов в строке
	size_t count = 0;
	/**
	 * Выполняем перебор всех октетов строки
	 */
	for(const auto letter : text)
		// Считаем октеты, не являющиеся продолжением символа
		count += ((static_cast <uint8_t> (letter) & 0xC0) != 0x80 ? 1 : 0);
	// Выводим дополненную пробелами строку
	return (text + string((width > count ? (width - count) : 1), ' '));
}
/**
 * @brief Функция получения человекочитаемого описания реакции
 *
 * @param reaction реакция эндпоинта
 * @return         описание реакции
 *
 */
static string describe(const reaction_t & reaction) noexcept {
	// Если реакции не было
	if(!reaction.present)
		// Выводим признак приёма без возражений
		return "-";
	// Получаем название кода ошибки
	const string_view name = h3::errorName(static_cast <error_t> (reaction.code));
	// Выводим описание реакции вместе с её уровнем
	return (string(name) + (reaction.connection ? " (соед)" : " (поток)"));
}
/**
 * @brief Функция прогона набора потоков через наш парсер
 *
 * @param item проверяемый случай
 * @return     реакция нашего парсера
 *
 */
static reaction_t ours(const vector <chunk_t> & chunks) noexcept {
	// Результат работы функции - реакция нашего парсера
	reaction_t result;
	// Создаём объект парсера сервера
	parser_http3_t parser(direct_t::REQUEST, nullptr, nullptr);
	// Идентификатор следующего выдаваемого однонаправленного потока сервера
	int64_t unistream = 3;
	// Устанавливаем функцию обратного вызова открытия однонаправленного потока
	parser.on(parser_http3_t::open_callback_t([&unistream]() noexcept -> int64_t {
		// Выделяем идентификатор однонаправленного потока
		const int64_t sid = unistream;
		// Продвигаем идентификатор следующего однонаправленного потока
		unistream += 4;
		// Выводим идентификатор открытого потока
		return sid;
	}));
	// Устанавливаем функцию обратного вызова записи исходящих байтов потока
	parser.on(parser_http3_t::write_callback_t([](const uint64_t, const void *, const size_t, const bool) noexcept {
		// Исходящие байты пробнику не нужны: сверяется только реакция на ввод
	}));
	// Устанавливаем функцию обратного вызова ошибки уровня соединения
	parser.on(parser_http3_t::error_callback_t([&result](const parser_http3_t::error_t code, const string_view) noexcept {
		// Если реакция ещё не зафиксирована
		if(!result.present){
			// Запоминаем наличие реакции
			result.present = true;
			// Помечаем реакцию уровнем соединения
			result.connection = true;
			// Запоминаем код ошибки протокола
			result.code = static_cast <uint64_t> (code);
		}
	}));
	// Устанавливаем функцию обратного вызова обрыва потока
	parser.on(parser_http3_t::abort_callback_t([&result](const uint64_t, const parser_http3_t::error_t code, const bool) noexcept {
		// Если реакция ещё не зафиксирована
		if(!result.present){
			// Запоминаем наличие реакции
			result.present = true;
			// Помечаем реакцию уровнем потока
			result.connection = false;
			// Запоминаем код ошибки протокола
			result.code = static_cast <uint64_t> (code);
		}
	}));
	// Устанавливаем функцию обратного вызова поля секции
	parser.on(parser_http3_t::header_callback_t([](const uint64_t, const string_view, const string_view, const parser_t::part_t) noexcept -> bool {
		// Продолжаем разбор
		return true;
	}));
	// Устанавливаем функцию обратного вызова фрагмента тела потока
	parser.on(parser_http3_t::data_callback_t([](const uint64_t, const void *, const size_t, const bool) noexcept -> bool {
		// Продолжаем разбор
		return true;
	}));
	// Отправляем параметры соединения
	parser.sendSettings();
	/**
	 * Выполняем перебор всех порций байт потоков
	 */
	for(const auto & chunk : chunks){
		// Если реакция уже зафиксирована - дальнейшая подача смысла не имеет
		if(result.present)
			// Прекращаем подачу байт
			break;
		// Подаём порцию байт потока на разбор
		parser.parse(chunk.sid, chunk.data.data(), chunk.data.size(), chunk.fin);
	}
	// Выводим реакцию нашего парсера
	return result;
}

/**
 * @brief Функция обработки требования эталона прекратить приём потока
 *
 * @param code код ошибки протокола
 * @param user пользовательские данные соединения
 * @return     результат обработки
 *
 */
static int onStopSending(nghttp3_conn *, int64_t, uint64_t code, void * user, void *) noexcept {
	// Получаем реакцию эталонной реализации
	reaction_t * reaction = reinterpret_cast <reaction_t *> (user);
	// Если реакция ещё не зафиксирована
	if(!reaction->present){
		// Запоминаем наличие реакции
		reaction->present = true;
		// Помечаем реакцию уровнем потока
		reaction->connection = false;
		// Запоминаем код ошибки протокола
		reaction->code = code;
	}
	// Продолжаем разбор
	return 0;
}
/**
 * @brief Функция обработки требования эталона сбросить поток
 *
 * @param code код ошибки протокола
 * @param user пользовательские данные соединения
 * @return     результат обработки
 *
 */
static int onResetStream(nghttp3_conn *, int64_t, uint64_t code, void * user, void *) noexcept {
	// Получаем реакцию эталонной реализации
	reaction_t * reaction = reinterpret_cast <reaction_t *> (user);
	// Если реакция ещё не зафиксирована
	if(!reaction->present){
		// Запоминаем наличие реакции
		reaction->present = true;
		// Помечаем реакцию уровнем потока
		reaction->connection = false;
		// Запоминаем код ошибки протокола
		reaction->code = code;
	}
	// Продолжаем разбор
	return 0;
}
/**
 * @brief Функция прогона набора потоков через сессию эталонной реализации
 *
 * @param chunks порции байт потоков в порядке подачи
 * @return       реакция эталонной реализации
 *
 */
static reaction_t reference(const vector <chunk_t> & chunks) noexcept {
	// Результат работы функции - реакция эталонной реализации
	reaction_t result;
	// Набор функций обратного вызова эталонной реализации
	nghttp3_callbacks handlers;
	// Обнуляем набор функций обратного вызова
	::memset(&handlers, 0, sizeof(handlers));
	// Устанавливаем функцию обработки требования прекратить приём потока
	handlers.stop_sending = ::onStopSending;
	// Устанавливаем функцию обработки требования сбросить поток
	handlers.reset_stream = ::onResetStream;
	// Параметры сессии эталонной реализации
	nghttp3_settings options;
	// Заполняем параметры сессии значениями по умолчанию
	::nghttp3_settings_default(&options);
	// Устанавливаем ёмкость динамической таблицы QPACK
	options.qpack_max_dtable_capacity = 4096;
	// Устанавливаем число потоков, которым разрешено ожидать пополнения таблицы
	options.qpack_blocked_streams = 16;
	// Объект сессии эталонной реализации
	nghttp3_conn * conn = nullptr;
	// Создаём сессию сервера эталонной реализации
	if(::nghttp3_conn_server_new(&conn, &handlers, &options, ::nghttp3_mem_default(), &result) != 0)
		// Выводим реакцию эталонной реализации
		return result;
	// Однонаправленные потоки сервера нумеруются как 3, 7, 11 (RFC 9000 §2.1)
	::nghttp3_conn_bind_control_stream(conn, 3);
	// Привязываем потоки инструкций кодека QPACK сессии сервера
	::nghttp3_conn_bind_qpack_streams(conn, 7, 11);
	/**
	 * Выполняем перебор всех порций байт потоков
	 */
	for(const auto & chunk : chunks){
		// Если реакция уже зафиксирована - дальнейшая подача смысла не имеет
		if(result.present)
			// Прекращаем подачу байт
			break;
		// Подаём порцию байт потока на разбор
		const nghttp3_ssize read = ::nghttp3_conn_read_stream(
			conn, static_cast <int64_t> (chunk.sid),
			reinterpret_cast <const uint8_t *> (chunk.data.data()),
			chunk.data.size(), (chunk.fin ? 1 : 0)
		);
		// Если эталонная реализация отвергла данные
		if(read < 0){
			// Запоминаем наличие реакции
			result.present = true;
			// Эталон возвращает ошибку наружу, оставляя выбор уровня приложению
			result.connection = true;
			// Приводим ошибку библиотеки к коду ошибки протокола
			result.code = ::nghttp3_err_infer_quic_app_error_code(static_cast <int> (read));
		}
	}
	// Удаляем сессию эталонной реализации
	::nghttp3_conn_del(conn);
	// Выводим реакцию эталонной реализации
	return result;
}

/**
 * @brief Функция сборки набора проверяемых случаев
 *
 * @return собранный набор проверяемых случаев
 *
 */
static vector <case_t> build() noexcept {
	// Корректная секция полей запроса
	const string request = ::block(REQUEST);
	// Корректный кадр параметров соединения
	const string options = ::frame(0x04, ::settings({{0x01, 4096}, {0x07, 16}}));
	// Выводим собранный набор проверяемых случаев
	return {
		/**
		 * Управляющий поток: открытие, закрытие и допустимость кадров
		 */
		// Управляющий поток начат не кадром параметров соединения
		{"управляющий поток начат не SETTINGS", false, {{2, ::varint(0x00) + ::frame(0x00, "body"), false}}},
		// Кадр запроса пришёл до кадра параметров соединения
		{"секция полей до SETTINGS", false, {{0, ::frame(0x01, request), true}}},
		// Второй управляющий поток на соединении
		{"второй управляющий поток", true, {{14, ::varint(0x00) + options, false}}},
		// Управляющий поток закрыт признаком завершения
		{"управляющий поток закрыт", true, {{2, "", true}}},
		// Повторный кадр параметров соединения
		{"повторный SETTINGS", true, {{2, options, false}}},
		// Кадр тела на управляющем потоке
		{"DATA на управляющем потоке", true, {{2, ::frame(0x00, "body"), false}}},
		// Секция полей на управляющем потоке
		{"HEADERS на управляющем потоке", true, {{2, ::frame(0x01, request), false}}},
		/**
		 * Изъятые из употребления типы кадров HTTP/2 (RFC 9114 §11.2.1)
		 */
		// Кадр приоритета HTTP/2
		{"изъятый кадр PRIORITY (0x02)", true, {{2, ::frame(0x02, string(5, '\0')), false}}},
		// Кадр проверки живости HTTP/2
		{"изъятый кадр PING (0x06)", true, {{2, ::frame(0x06, string(8, '\0')), false}}},
		// Кадр обновления окна HTTP/2
		{"изъятый кадр WINDOW_UPDATE (0x08)", true, {{2, ::frame(0x08, string(4, '\0')), false}}},
		// Кадр продолжения блока заголовков HTTP/2
		{"изъятый кадр CONTINUATION (0x09)", true, {{2, ::frame(0x09, request), false}}},
		/**
		 * Содержимое кадра параметров соединения
		 */
		// Изъятый из употребления параметр соединения
		{"изъятый параметр SETTINGS (0x02)", false, {{2, ::varint(0x00) + ::frame(0x04, ::settings({{0x02, 1}})), false}}},
		// Повторный параметр соединения
		{"повторный параметр SETTINGS", false, {{2, ::varint(0x00) + ::frame(0x04, ::settings({{0x01, 4096}, {0x01, 8192}})), false}}},
		// Параметр соединения без значения
		{"SETTINGS с усечённым параметром", false, {{2, ::varint(0x00) + ::frame(0x04, ::varint(0x01)), false}}},
		// Зарезервированный параметр соединения обязан игнорироваться
		{"зарезервированный параметр SETTINGS", false, {{2, ::varint(0x00) + ::frame(0x04, ::settings({{0x21, 1}})), false}}},
		// Зарезервированный тип кадра обязан игнорироваться
		{"зарезервированный тип кадра", true, {{2, ::frame(0x21, "grease"), false}}},
		/**
		 * Идентификаторы завершения соединения и server push
		 */
		// Возрастающий идентификатор завершения соединения
		{"GOAWAY с возрастающим идентификатором", true, {{2, ::frame(0x07, ::varint(4)) + ::frame(0x07, ::varint(8)), false}}},
		// Убывающая верхняя граница идентификаторов push
		{"MAX_PUSH_ID с убывающим значением", true, {{2, ::frame(0x0D, ::varint(8)) + ::frame(0x0D, ::varint(4)), false}}},
		// Кадр отмены push с лишними октетами
		{"CANCEL_PUSH с лишними октетами", true, {{2, ::frame(0x03, ::varint(0) + string(2, '\0')), false}}},
		/**
		 * Однонаправленные потоки
		 */
		// Второй поток инструкций кодера QPACK
		{"второй поток кодера QPACK", true, {{18, ::varint(0x02), false}}},
		// Поток инструкций кодера QPACK закрыт признаком завершения
		{"поток кодера QPACK закрыт", true, {{6, "", true}}},
		// Поток инструкций декодера QPACK закрыт признаком завершения
		{"поток декодера QPACK закрыт", true, {{10, "", true}}},
		// Однонаправленный поток неизвестного типа обязан отбрасываться
		{"однонаправленный поток неизвестного типа", true, {{22, ::varint(0x40) + "payload", false}}},
		// Зарезервированный тип однонаправленного потока обязан отбрасываться
		{"зарезервированный тип потока", true, {{26, ::varint(0x21) + "payload", false}}},
		/**
		 * Поток запроса: допустимость кадров
		 */
		// Кадр тела до секции полей
		{"DATA до секции полей", true, {{0, ::frame(0x00, "body"), false}}},
		// Кадр параметров соединения на потоке запроса
		{"SETTINGS на потоке запроса", true, {{0, options, false}}},
		// Кадр завершения соединения на потоке запроса
		{"GOAWAY на потоке запроса", true, {{0, ::frame(0x07, ::varint(0)), false}}},
		// Кадр верхней границы идентификаторов push на потоке запроса
		{"MAX_PUSH_ID на потоке запроса", true, {{0, ::frame(0x0D, ::varint(8)), false}}},
		// Кадр отмены push на потоке запроса
		{"CANCEL_PUSH на потоке запроса", true, {{0, ::frame(0x03, ::varint(0)), false}}},
		// Анонс push от клиента серверу
		{"PUSH_PROMISE от клиента", true, {{0, ::frame(0x05, ::varint(0) + request), false}}},
		// Третья секция полей на потоке запроса
		{"третья секция полей на потоке", true, {{0, ::frame(0x01, request) + ::frame(0x01, ::block({{"x-trailer", "value"}})) + ::frame(0x01, ::block({{"x-extra", "value"}})), true}}},
		// Кадр неизвестного типа на потоке запроса обязан игнорироваться
		{"кадр неизвестного типа на потоке запроса", true, {{0, ::frame(0x01, request) + ::frame(0x1F1F, "unknown"), true}}},
		/**
		 * Поток запроса: семантика сообщения
		 */
		// Секция полей без обязательных псевдо-полей
		{"секция полей без псевдо-полей", true, {{0, ::frame(0x01, ::block({{"accept", "*/*"}})), true}}},
		// Название поля в верхнем регистре
		{"поле в верхнем регистре", true, {{0, ::frame(0x01, ::block({{":method", "GET"}, {":scheme", "https"}, {":path", "/"}, {"Accept", "*/*"}})), true}}},
		// Поле управления соединением, запрещённое в HTTP/3
		{"поле connection", true, {{0, ::frame(0x01, ::block({{":method", "GET"}, {":scheme", "https"}, {":path", "/"}, {"connection", "keep-alive"}})), true}}},
		// Поле te с недопустимым значением
		{"поле te: gzip", true, {{0, ::frame(0x01, ::block({{":method", "GET"}, {":scheme", "https"}, {":path", "/"}, {"te", "gzip"}})), true}}},
		// Повторное псевдо-поле пути запроса
		{"повторное псевдо-поле :path", true, {{0, ::frame(0x01, ::block({{":method", "GET"}, {":scheme", "https"}, {":path", "/"}, {":path", "/other"}})), true}}},
		// Псевдо-поле после обычного поля
		{"псевдо-поле после обычного", true, {{0, ::frame(0x01, ::block({{":method", "GET"}, {":scheme", "https"}, {"accept", "*/*"}, {":path", "/"}})), true}}},
		// Псевдо-поле ответа в запросе
		{"псевдо-поле :status в запросе", true, {{0, ::frame(0x01, ::block({{":method", "GET"}, {":scheme", "https"}, {":path", "/"}, {":status", "200"}})), true}}},
		// Пустой путь запроса
		{"пустое псевдо-поле :path", true, {{0, ::frame(0x01, ::block({{":method", "GET"}, {":scheme", "https"}, {":path", ""}})), true}}},
		// Неизвестное псевдо-поле
		{"неизвестное псевдо-поле", true, {{0, ::frame(0x01, ::block({{":method", "GET"}, {":scheme", "https"}, {":path", "/"}, {":unknown", "x"}})), true}}},
		// Метод CONNECT со схемой и путём
		{"CONNECT со :scheme и :path", true, {{0, ::frame(0x01, ::block({{":method", "CONNECT"}, {":scheme", "https"}, {":path", "/"}, {":authority", "example.com"}})), true}}},
		// Расхождение объявленной и фактической длины тела
		{"content-length не совпадает с телом", true, {{0, ::frame(0x01, ::block({{":method", "POST"}, {":scheme", "https"}, {":path", "/"}, {"content-length", "10"}})) + ::frame(0x00, "short"), true}}},
		// Псевдо-поле в секции трейлеров
		{"псевдо-поле в трейлерах", true, {{0, ::frame(0x01, request) + ::frame(0x00, "body") + ::frame(0x01, ::block({{":method", "GET"}})), true}}},
		/**
		 * Кодек QPACK
		 */
		// Индекс за границей статической таблицы
		{"индекс за границей статической таблицы", true, {{0, ::frame(0x01, string("\x00\x00", 2) + ::prefixed(99, 6, 0xC0)), true}}},
		// Ссылка на запись пустой динамической таблицы
		{"ссылка на пустую динамическую таблицу", true, {{0, ::frame(0x01, string("\x00\x00", 2) + ::prefixed(0, 6, 0x80)), true}}},
		// Требуемое число вставок сверх окна нумерации таблицы
		{"Required Insert Count сверх окна нумерации", true, {{0, ::frame(0x01, ::prefixed(300, 8, 0x00) + string("\x00", 1) + ::prefixed(17, 6, 0xC0)), true}}},
		// Секция, требующая ещё не пришедших вставок, обязана блокировать поток, а не рваться
		{"секция ждёт пополнения таблицы QPACK", true, {{0, ::frame(0x01, string("\x06\x00", 2) + ::prefixed(17, 6, 0xC0)), true}}},
		// Секция полей оборвана на середине
		{"усечённая секция полей", true, {{0, ::varint(0x01) + ::varint(request.size() + 16) + request, true}}},
		// Ёмкость динамической таблицы сверх анонсированной
		{"ёмкость таблицы QPACK сверх лимита", true, {{6, ::prefixed(100000, 5, 0x20), false}}},
		// Дублирование записи пустой динамической таблицы
		{"дублирование записи пустой таблицы", true, {{6, ::prefixed(0, 5, 0x00), false}}},
		// Подтверждение секции на потоке, где секций не было
		{"подтверждение секции на пустом потоке", true, {{10, ::prefixed(0, 7, 0x80), false}}},
		// Нулевой прирост числа вставок
		{"нулевой прирост числа вставок", true, {{10, ::prefixed(0, 6, 0x00), false}}}
	};
}

/**
 * @brief Функция входа в пробник
 *
 * @return код выхода из пробника
 *
 */
int32_t main() noexcept {
	// Собираем набор проверяемых случаев
	const vector <case_t> cases = ::build();
	// Корректное начало соединения: управляющий поток и потоки инструкций QPACK клиента
	const vector <chunk_t> prelude = {
		{2, ::varint(0x00) + ::frame(0x04, ::settings({{0x01, 4096}, {0x07, 16}})), false},
		{6, ::varint(0x02), false},
		{10, ::varint(0x03), false}
	};
	// Количество расхождений кода реакции
	size_t diffs = 0;
	// Количество расхождений уровня реакции
	size_t levels = 0;
	// Печатаем шапку таблицы
	::printf("%s%s%s\n", ::pad("случай", 42).c_str(), ::pad("наш парсер", 34).c_str(), ::pad("nghttp3", 34).c_str());
	/**
	 * Выполняем перебор всех проверяемых случаев
	 */
	for(const auto & item : cases){
		// Собираемый набор порций байт потоков
		vector <chunk_t> chunks;
		// Если случаю требуется корректное начало соединения
		if(item.prelude)
			// Дописываем корректное начало соединения
			chunks.insert(chunks.end(), prelude.begin(), prelude.end());
		// Дописываем порции байт самого случая
		chunks.insert(chunks.end(), item.chunks.begin(), item.chunks.end());
		// Получаем реакцию нашего парсера
		const reaction_t mine = ::ours(chunks);
		// Получаем реакцию эталонной реализации
		const reaction_t peer = ::reference(chunks);
		// Определяем расхождение кода реакции
		const bool diff = ((mine.present != peer.present) || (mine.code != peer.code));
		// Печатаем строку сравнения
		::printf(
			"%s%s%s%s\n", ::pad(item.title, 42).c_str(),
			::pad(::describe(mine), 34).c_str(), ::pad(::describe(peer), 34).c_str(),
			(diff ? "<-- расхождение" : "")
		);
		// Если код реакции разошёлся
		if(diff)
			// Наращиваем счётчик расхождений кода реакции
			diffs++;
		// Если код совпал, а уровень реакции разошёлся
		else if(mine.present && (mine.connection != peer.connection))
			// Наращиваем счётчик расхождений уровня реакции
			levels++;
	}
	// Печатаем итог сверки
	::printf("\nслучаев: %zu, расхождений: %zu\n", cases.size(), diffs);
	// Печатаем справку об уровне реакции
	::printf("реакций совпавшего кода, но разного уровня: %zu (уровень эталон оставляет приложению)\n", levels);
	// Выводим код выхода по итогу сверки
	return EXIT_SUCCESS;
}
