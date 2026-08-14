/**
 * @file nghttp3-qpack.cpp
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
 * @brief Пробник сверки QPACK-кодека с эталонной реализацией nghttp3 — обе стороны кодека
 *        проверяются против чужой реализации в обоих направлениях
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Сверка ведётся в обе стороны, и это принципиально: кодек, ошибающийся одинаково
 * при кодировании и декодировании, сверку сам с собой прошёл бы. Здесь же наш кодер
 * проверяется чужим декодером, а наш декодер - чужим кодером, поэтому расходящееся
 * прочтение RFC обнаруживается сразу.
 *
 * Проверяется и обратная связь кодека: инструкции потока декодера подаются кодеру
 * противоположной стороны. Без неё динамическая таблица не пополнялась бы вовсе,
 * и сверка выродилась бы в проверку одной статической таблицы.
 */

#include <string>
#include <vector>
#include <cstdio>
#include <cstdint>
#include <cstring>

#include <nghttp3/nghttp3.h>

#include <proto/http/parser/http3/qpack.hpp>
#include <proto/http/parser/http2/hpack.hpp>

using namespace std;
using namespace awh;
using namespace awh::http;
using namespace awh::http::h3;

/**
 * @brief Ёмкость динамической таблицы, анонсируемая обеими сторонами сверки
 *
 */
static constexpr uint64_t TABLE_CAPACITY = 4096;
/**
 * @brief Число потоков, которым разрешено ожидать пополнения таблицы
 *
 */
static constexpr uint64_t BLOCKED_STREAMS = 16;
/**
 * @brief Количество прогонов набора сверяемых секций
 *
 * @details Один прогон не наполняет динамическую таблицу, и сверка проверяла бы
 *          только статическую. Повторение выводит кодек в установившийся режим,
 *          где работают и индексация, и вытеснение, и подтверждения секций
 *
 */
static constexpr size_t ROUNDS = 64;

/**
 * @brief Структура сверяемого набора полей
 *
 */
typedef struct Sample {
	// Название набора для диагностики
	const char * title;
	// Поля набора
	vector <qpack::field_t> fields;
} sample_t;

/**
 * @brief Функция получения набора сверяемых секций полей
 *
 * @return набор сверяемых секций полей
 *
 */
static const vector <sample_t> & samples() noexcept {
	// Набор сверяемых секций полей
	static const vector <sample_t> result = {
		{"запрос страницы", {
			qpack::field_t{":method", "GET"},
			qpack::field_t{":scheme", "https"},
			qpack::field_t{":authority", "www.example.com"},
			qpack::field_t{":path", "/index.html"},
			qpack::field_t{"user-agent", "awh/5.0 (interop)"},
			qpack::field_t{"accept", "text/html,application/xhtml+xml"},
			qpack::field_t{"accept-encoding", "gzip, deflate, br"},
			qpack::field_t{"accept-language", "ru-RU,ru;q=0.9,en;q=0.8"}
		}},
		{"запрос с cookie", {
			qpack::field_t{":method", "POST"},
			qpack::field_t{":scheme", "https"},
			qpack::field_t{":authority", "www.example.com"},
			qpack::field_t{":path", "/api/v1/submit"},
			qpack::field_t{"content-type", "application/json"},
			qpack::field_t{"content-length", "142"},
			qpack::field_t{"cookie", "session=8f3b1c2d4e5a6b7c8d9e0f1a2b3c4d5e; theme=dark"},
			qpack::field_t{"authorization", "Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9"}
		}},
		{"ответ страницы", {
			qpack::field_t{":status", "200"},
			qpack::field_t{"content-type", "text/html; charset=utf-8"},
			qpack::field_t{"content-encoding", "gzip"},
			qpack::field_t{"cache-control", "max-age=604800"},
			qpack::field_t{"vary", "accept-encoding"},
			qpack::field_t{"server", "awh"},
			qpack::field_t{"date", "Mon, 27 Jul 2026 00:00:00 GMT"},
			qpack::field_t{"x-content-type-options", "nosniff"}
		}},
		{"ответ с редиректом", {
			qpack::field_t{":status", "302"},
			qpack::field_t{"location", "https://www.example.com/moved"},
			qpack::field_t{"set-cookie", "session=deleted; Max-Age=0; Path=/"},
			qpack::field_t{"content-length", "0"}
		}},
		{"поля вне таблиц", {
			qpack::field_t{":method", "GET"},
			qpack::field_t{":scheme", "https"},
			qpack::field_t{":authority", "www.example.com"},
			qpack::field_t{":path", "/uncached/resource"},
			qpack::field_t{"x-request-id", "0f9c1a2b-3d4e-5f60-7a8b-9c0d1e2f3a4b"},
			qpack::field_t{"x-trace-flags", "01"},
			qpack::field_t{"x-custom-header", "значение с юникодом и пробелами"}
		}}
	};
	// Выводим набор сверяемых секций полей
	return result;
}

/**
 * @brief Функция сверки статической таблицы с эталонной реализацией
 *
 * @details Каждая запись таблицы кодируется ссылкой на её индекс и отдаётся
 *          эталонному декодеру: расхождение хотя бы в одном октете значения
 *          означает, что таблицы разошлись. Такое расхождение не всплывает
 *          на обычной нагрузке - оно проявляется только тогда, когда пир
 *          сошлётся именно на разошедшуюся запись, - поэтому сверяется
 *          вся таблица целиком, а не те записи, что попали в набор
 *
 * @param failures количество обнаруженных расхождений
 * @param entries  количество сверенных записей
 *
 */
static void staticTableAgainstReference(size_t & failures, size_t & entries) noexcept {
	// Получаем аллокатор эталонной реализации
	const nghttp3_mem * mem = ::nghttp3_mem_default();
	// Объект декодера эталонной реализации
	nghttp3_qpack_decoder * decoder = nullptr;
	// Создаём декодер эталонной реализации
	if(::nghttp3_qpack_decoder_new(&decoder, 0, 0, mem) != 0){
		// Выводим сообщение о неудачном создании декодера
		::printf("декодер эталонной реализации не создан\n");
		// Считаем обнаруженное расхождение
		failures++;
		// Выходим из функции
		return;
	}
	/**
	 * Выполняем сверку всех записей статической таблицы
	 */
	for(size_t index = 0; index < qpack::STATIC_TABLE_SIZE; index++){
		// Получаем сверяемую запись нашей статической таблицы
		const qpack::static_entry_t * entry = qpack::staticTable(index);
		// Собираемая секция полей из единственной ссылки на запись
		string section;
		/**
		 * Префикс секции: требуемое число вставок и разница базы нулевые -
		 * ссылка на статическую таблицу динамической не касается
		 */
		section.push_back('\0');
		// Дописываем нулевую разницу базы
		section.push_back('\0');
		// Дописываем ссылку на запись статической таблицы
		h2::hpack::prefixed::encode(section, index, 6, 0xC0);
		// Объект контекста потока эталонной реализации
		nghttp3_qpack_stream_context * context = nullptr;
		// Создаём контекст потока эталонной реализации
		if(::nghttp3_qpack_stream_context_new(&context, static_cast <int64_t> (index * 4), mem) != 0){
			// Выводим описание расхождения
			::printf("запись %zu: контекст потока эталона не создан\n", index);
			// Считаем обнаруженное расхождение
			failures++;
			// Переходим к следующей записи
			continue;
		}
		// Декодированное поле
		nghttp3_qpack_nv nv;
		// Флаги результата декодирования
		uint8_t flags = 0;
		// Выполняем декодирование секции эталонной реализацией
		const nghttp3_ssize read = ::nghttp3_qpack_decoder_read_request(
			decoder, context, &nv, &flags, reinterpret_cast <const uint8_t *> (section.data()), section.size(), 1
		);
		// Если декодирование не удалось
		if((read < 0) || ((flags & NGHTTP3_QPACK_DECODE_FLAG_EMIT) == 0)){
			// Выводим описание расхождения
			::printf("запись %zu: эталон не смог разобрать ссылку на статическую таблицу\n", index);
			// Считаем обнаруженное расхождение
			failures++;
			// Удаляем контекст потока эталонной реализации
			::nghttp3_qpack_stream_context_del(context);
			// Переходим к следующей записи
			continue;
		}
		// Получаем название записи эталонной таблицы
		const string_view name(reinterpret_cast <const char *> (::nghttp3_rcbuf_get_buf(nv.name).base), ::nghttp3_rcbuf_get_buf(nv.name).len);
		// Получаем значение записи эталонной таблицы
		const string_view value(reinterpret_cast <const char *> (::nghttp3_rcbuf_get_buf(nv.value).base), ::nghttp3_rcbuf_get_buf(nv.value).len);
		// Если название либо значение записи разошлись
		if((name != entry->name) || (value != entry->value)){
			// Выводим описание расхождения
			::printf(
				"запись %zu: у нас [%.*s] = [%.*s], у эталона [%.*s] = [%.*s]\n", index,
				static_cast <int> (entry->name.size()), entry->name.data(),
				static_cast <int> (entry->value.size()), entry->value.data(),
				static_cast <int> (name.size()), name.data(),
				static_cast <int> (value.size()), value.data()
			);
			// Считаем обнаруженное расхождение
			failures++;
		}
		// Считаем сверенную запись
		entries++;
		// Освобождаем название декодированного поля
		::nghttp3_rcbuf_decref(nv.name);
		// Освобождаем значение декодированного поля
		::nghttp3_rcbuf_decref(nv.value);
		// Удаляем контекст потока эталонной реализации
		::nghttp3_qpack_stream_context_del(context);
	}
	// Удаляем декодер эталонной реализации
	::nghttp3_qpack_decoder_del(decoder);
}
/**
 * @brief Функция сверки нашего кодера с эталонным декодером
 *
 * @param failures количество обнаруженных расхождений
 * @param sections количество сверенных секций
 * @param fields   количество сверенных полей
 *
 */
static void encoderAgainstReference(size_t & failures, size_t & sections, size_t & fields) noexcept {
	// Получаем аллокатор эталонной реализации
	const nghttp3_mem * mem = ::nghttp3_mem_default();
	// Объект декодера эталонной реализации
	nghttp3_qpack_decoder * decoder = nullptr;
	// Создаём декодер эталонной реализации
	if(::nghttp3_qpack_decoder_new(&decoder, TABLE_CAPACITY, BLOCKED_STREAMS, mem) != 0){
		// Выводим сообщение о неудачном создании декодера
		::printf("декодер эталонной реализации не создан\n");
		// Считаем обнаруженное расхождение
		failures++;
		// Выходим из функции
		return;
	}
	// Объект кодера библиотеки
	qpack::encoder_t encoder;
	// Устанавливаем ёмкость динамической таблицы, анонсированную пиром
	encoder.maxCapacity(TABLE_CAPACITY);
	// Устанавливаем число потоков, которым пир разрешил ожидать пополнения таблицы
	encoder.maxBlocked(BLOCKED_STREAMS);
	// Идентификатор очередного потока запроса
	int64_t sid = 0;
	// Буфер секции полей
	string section;
	// Буфер инструкций потока декодера эталонной реализации
	uint8_t feedback[4096];
	/**
	 * Выполняем прогон всех наборов сверяемых секций
	 */
	for(size_t round = 0; round < ROUNDS; round++){
		/**
		 * Выполняем сверку всех наборов полей
		 */
		for(const auto & sample : samples()){
			// Выполняем очистку буфера секции полей
			section.clear();
			// Выполняем кодирование секции полей
			encoder.encode(static_cast <uint64_t> (sid), sample.fields, section);
			// Получаем накопленные инструкции потока кодера
			const string_view instructions = encoder.pending();
			/**
			 * Инструкции потока кодера подаются декодеру пира до секции: секция
			 * ссылается на записи, которые ими и вставляются
			 */
			if(!instructions.empty()){
				// Выполняем обработку инструкций потока кодера эталонным декодером
				const nghttp3_ssize read = ::nghttp3_qpack_decoder_read_encoder(
					decoder, reinterpret_cast <const uint8_t *> (instructions.data()), instructions.size()
				);
				// Если эталонный декодер отверг инструкции потока кодера
				if(read < 0){
					// Выводим описание расхождения
					::printf("прогон %zu, набор [%s]: эталон отверг инструкции кодера: %s\n", round, sample.title, ::nghttp3_strerror(static_cast <int> (read)));
					// Считаем обнаруженное расхождение
					failures++;
					// Переходим к следующему набору полей
					continue;
				}
				// Отмечаем инструкции потока кодера отправленными
				encoder.consumePending(static_cast <size_t> (read));
			}
			// Объект контекста потока эталонной реализации
			nghttp3_qpack_stream_context * context = nullptr;
			// Создаём контекст потока эталонной реализации
			if(::nghttp3_qpack_stream_context_new(&context, sid, mem) != 0){
				// Выводим описание расхождения
				::printf("прогон %zu, набор [%s]: контекст потока эталона не создан\n", round, sample.title);
				// Считаем обнаруженное расхождение
				failures++;
				// Переходим к следующему набору полей
				continue;
			}
			// Указатель на неразобранный остаток секции
			const uint8_t * cursor = reinterpret_cast <const uint8_t *> (section.data());
			// Размер неразобранного остатка секции
			size_t left = section.size();
			// Номер сверяемого поля секции
			size_t index = 0;
			/**
			 * Выполняем декодирование секции эталонной реализацией
			 */
			for(;;){
				// Декодированное поле
				nghttp3_qpack_nv nv;
				// Флаги результата декодирования
				uint8_t flags = 0;
				// Выполняем декодирование очередного поля секции
				const nghttp3_ssize read = ::nghttp3_qpack_decoder_read_request(decoder, context, &nv, &flags, cursor, left, 1);
				// Если декодирование не удалось
				if(read < 0){
					// Выводим описание расхождения
					::printf("прогон %zu, набор [%s]: эталон не смог разобрать секцию: %s\n", round, sample.title, ::nghttp3_strerror(static_cast <int> (read)));
					// Считаем обнаруженное расхождение
					failures++;
					// Прекращаем разбор секции
					break;
				}
				// Продвигаем указатель на неразобранный остаток секции
				cursor += read;
				// Уменьшаем размер неразобранного остатка секции
				left -= static_cast <size_t> (read);
				/**
				 * Блокировка означала бы, что мы сослались на записи, инструкции
				 * вставки которых не отправили: это дефект кодера
				 */
				if(flags & NGHTTP3_QPACK_DECODE_FLAG_BLOCKED){
					// Выводим описание расхождения
					::printf("прогон %zu, набор [%s]: эталон заблокирован на секции\n", round, sample.title);
					// Считаем обнаруженное расхождение
					failures++;
					// Прекращаем разбор секции
					break;
				}
				// Если декодировано очередное поле
				if(flags & NGHTTP3_QPACK_DECODE_FLAG_EMIT){
					// Если полей декодировано больше отправленного
					if(index >= sample.fields.size()){
						// Выводим описание расхождения
						::printf("прогон %zu, набор [%s]: эталон выдал лишнее поле\n", round, sample.title);
						// Считаем обнаруженное расхождение
						failures++;
					// Если поле подлежит сверке
					} else {
						// Получаем название декодированного поля
						const string_view name(reinterpret_cast <const char *> (::nghttp3_rcbuf_get_buf(nv.name).base), ::nghttp3_rcbuf_get_buf(nv.name).len);
						// Получаем значение декодированного поля
						const string_view value(reinterpret_cast <const char *> (::nghttp3_rcbuf_get_buf(nv.value).base), ::nghttp3_rcbuf_get_buf(nv.value).len);
						// Если название либо значение поля разошлись
						if((name != sample.fields[index].name) || (value != sample.fields[index].value)){
							// Выводим описание расхождения
							::printf(
								"прогон %zu, набор [%s], поле %zu: отправлено [%s] = [%.32s], получено [%.*s] = [%.32s]\n",
								round, sample.title, index, sample.fields[index].name.c_str(), sample.fields[index].value.c_str(),
								static_cast <int> (name.size()), name.data(), value.data()
							);
							// Считаем обнаруженное расхождение
							failures++;
						}
						// Считаем сверенное поле
						fields++;
					}
					// Переходим к следующему полю секции
					index++;
					// Освобождаем название декодированного поля
					::nghttp3_rcbuf_decref(nv.name);
					// Освобождаем значение декодированного поля
					::nghttp3_rcbuf_decref(nv.value);
				}
				// Если секция разобрана целиком
				if(flags & NGHTTP3_QPACK_DECODE_FLAG_FINAL)
					// Прекращаем разбор секции
					break;
				// Если неразобранный остаток секции исчерпан
				if((read == 0) && (left == 0))
					// Прекращаем разбор секции
					break;
			}
			// Если декодировано меньше полей, чем отправлено
			if(index != sample.fields.size()){
				// Выводим описание расхождения
				::printf("прогон %zu, набор [%s]: отправлено %zu полей, получено %zu\n", round, sample.title, sample.fields.size(), index);
				// Считаем обнаруженное расхождение
				failures++;
			}
			// Удаляем контекст потока эталонной реализации
			::nghttp3_qpack_stream_context_del(context);
			// Получаем размер накопленных инструкций потока декодера
			const size_t length = ::nghttp3_qpack_decoder_get_decoder_streamlen(decoder);
			/**
			 * Инструкции потока декодера возвращаются нашему кодеру: без них он
			 * не узнал бы о полученных пиром вставках и не смог бы вытеснять записи
			 */
			if((length > 0) && (length <= sizeof(feedback))){
				// Буфер инструкций потока декодера эталонной реализации
				nghttp3_buf buffer;
				// Устанавливаем начало буфера инструкций
				buffer.begin = feedback;
				// Устанавливаем позицию чтения буфера инструкций
				buffer.pos = feedback;
				// Устанавливаем позицию записи буфера инструкций
				buffer.last = feedback;
				// Устанавливаем конец буфера инструкций
				buffer.end = (feedback + sizeof(feedback));
				// Выполняем запись накопленных инструкций потока декодера
				::nghttp3_qpack_decoder_write_decoder(decoder, &buffer);
				// Количество разобранных октетов потока декодера
				size_t consumed = 0;
				// Код ошибки протокола
				h3::error_t error = h3::error_t::H3_NO_ERROR;
				// Выполняем обработку инструкций потока декодера
				if(encoder.decodeDecoderStream(string_view(reinterpret_cast <const char *> (buffer.pos), ::nghttp3_buf_len(&buffer)), consumed, error) != h3::status_t::OK){
					// Выводим описание расхождения
					::printf("прогон %zu, набор [%s]: наш кодер отверг инструкции декодера эталона: %s\n", round, sample.title, h3::errorName(error).data());
					// Считаем обнаруженное расхождение
					failures++;
				}
			}
			// Считаем сверенную секцию
			sections++;
			// Переходим к следующему потоку запроса
			sid += 4;
		}
	}
	// Удаляем декодер эталонной реализации
	::nghttp3_qpack_decoder_del(decoder);
}
/**
 * @brief Функция сверки эталонного кодера с нашим декодером
 *
 * @param failures количество обнаруженных расхождений
 * @param sections количество сверенных секций
 * @param fields   количество сверенных полей
 *
 */
static void decoderAgainstReference(size_t & failures, size_t & sections, size_t & fields) noexcept {
	// Получаем аллокатор эталонной реализации
	const nghttp3_mem * mem = ::nghttp3_mem_default();
	// Объект кодера эталонной реализации
	nghttp3_qpack_encoder * encoder = nullptr;
	// Создаём кодер эталонной реализации
	if(::nghttp3_qpack_encoder_new(&encoder, TABLE_CAPACITY, mem) != 0){
		// Выводим сообщение о неудачном создании кодера
		::printf("кодер эталонной реализации не создан\n");
		// Считаем обнаруженное расхождение
		failures++;
		// Выходим из функции
		return;
	}
	// Устанавливаем ёмкость динамической таблицы кодера эталонной реализации
	::nghttp3_qpack_encoder_set_max_dtable_capacity(encoder, TABLE_CAPACITY);
	// Устанавливаем число потоков, которым разрешено ожидать пополнения таблицы
	::nghttp3_qpack_encoder_set_max_blocked_streams(encoder, BLOCKED_STREAMS);
	// Объект декодера библиотеки
	qpack::decoder_t decoder(TABLE_CAPACITY, BLOCKED_STREAMS);
	// Идентификатор очередного потока запроса
	int64_t sid = 0;
	// Декодированные поля секции
	vector <qpack::field_view_t> output;
	// Набор полей в представлении эталонной реализации
	vector <nghttp3_nv> nva;
	/**
	 * Выполняем прогон всех наборов сверяемых секций
	 */
	for(size_t round = 0; round < ROUNDS; round++){
		/**
		 * Выполняем сверку всех наборов полей
		 */
		for(const auto & sample : samples()){
			// Выполняем очистку набора полей в представлении эталонной реализации
			nva.clear();
			/**
			 * Выполняем сбор набора полей в представлении эталонной реализации
			 */
			for(const auto & field : sample.fields){
				// Собираемое поле в представлении эталонной реализации
				nghttp3_nv nv;
				// Устанавливаем название поля
				nv.name = reinterpret_cast <uint8_t *> (const_cast <char *> (field.name.data()));
				// Устанавливаем длину названия поля
				nv.namelen = field.name.size();
				// Устанавливаем значение поля
				nv.value = reinterpret_cast <uint8_t *> (const_cast <char *> (field.value.data()));
				// Устанавливаем длину значения поля
				nv.valuelen = field.value.size();
				// Устанавливаем флаги поля
				nv.flags = (field.sensitive ? NGHTTP3_NV_FLAG_NEVER_INDEX : NGHTTP3_NV_FLAG_NONE);
				// Дописываем поле в набор
				nva.push_back(nv);
			}
			// Буфер префикса секции полей
			nghttp3_buf prefix;
			// Буфер представлений полей секции
			nghttp3_buf lines;
			// Буфер инструкций потока кодера
			nghttp3_buf instructions;
			// Выполняем инициализацию буфера префикса секции полей
			::nghttp3_buf_init(&prefix);
			// Выполняем инициализацию буфера представлений полей секции
			::nghttp3_buf_init(&lines);
			// Выполняем инициализацию буфера инструкций потока кодера
			::nghttp3_buf_init(&instructions);
			// Выполняем кодирование секции полей эталонной реализацией
			const int result = ::nghttp3_qpack_encoder_encode(encoder, &prefix, &lines, &instructions, sid, nva.data(), nva.size());
			// Если кодирование не удалось
			if(result != 0){
				// Выводим описание расхождения
				::printf("прогон %zu, набор [%s]: эталон не смог закодировать секцию: %s\n", round, sample.title, ::nghttp3_strerror(result));
				// Считаем обнаруженное расхождение
				failures++;
			// Если секция полей закодирована
			} else {
				// Код ошибки протокола
				h3::error_t error = h3::error_t::H3_NO_ERROR;
				// Количество разобранных октетов потока кодера
				size_t consumed = 0;
				// Получаем размер инструкций потока кодера
				const size_t length = ::nghttp3_buf_len(&instructions);
				/**
				 * Инструкции потока кодера обрабатываются до секции: секция
				 * ссылается на записи, которые ими и вставляются
				 */
				if(length > 0){
					// Выполняем обработку инструкций потока кодера
					if(decoder.decodeEncoderStream(string_view(reinterpret_cast <const char *> (instructions.pos), length), consumed, error) != h3::status_t::OK){
						// Выводим описание расхождения
						::printf("прогон %zu, набор [%s]: наш декодер отверг инструкции кодера эталона: %s\n", round, sample.title, h3::errorName(error).data());
						// Считаем обнаруженное расхождение
						failures++;
					// Если инструкции разобраны не целиком
					} else if(consumed != length) {
						// Выводим описание расхождения
						::printf("прогон %zu, набор [%s]: наш декодер разобрал %zu из %zu октетов потока кодера\n", round, sample.title, consumed, length);
						// Считаем обнаруженное расхождение
						failures++;
					}
				}
				// Собираем секцию полей из префикса и представлений
				string section(reinterpret_cast <const char *> (prefix.pos), ::nghttp3_buf_len(&prefix));
				// Дописываем представления полей секции
				section.append(reinterpret_cast <const char *> (lines.pos), ::nghttp3_buf_len(&lines));
				// Выполняем декодирование секции полей
				const h3::status_t status = decoder.decode(static_cast <uint64_t> (sid), section, output, 0, error);
				// Если декодирование не удалось
				if(status != h3::status_t::OK){
					// Выводим описание расхождения
					::printf(
						"прогон %zu, набор [%s]: наш декодер не смог разобрать секцию эталона: %s\n",
						round, sample.title, ((status == h3::status_t::BLOCKED) ? "поток заблокирован" : h3::errorName(error).data())
					);
					// Считаем обнаруженное расхождение
					failures++;
				// Если количество декодированных полей разошлось
				} else if(output.size() != sample.fields.size()) {
					// Выводим описание расхождения
					::printf("прогон %zu, набор [%s]: отправлено %zu полей, получено %zu\n", round, sample.title, sample.fields.size(), output.size());
					// Считаем обнаруженное расхождение
					failures++;
				// Если секция полей декодирована
				} else {
					/**
					 * Выполняем сверку всех декодированных полей секции
					 */
					for(size_t i = 0; i < output.size(); i++){
						// Если название либо значение поля разошлись
						if((output[i].name != sample.fields[i].name) || (output[i].value != sample.fields[i].value)){
							// Выводим описание расхождения
							::printf(
								"прогон %zu, набор [%s], поле %zu: отправлено [%s] = [%.32s], получено [%.*s] = [%.32s]\n",
								round, sample.title, i, sample.fields[i].name.c_str(), sample.fields[i].value.c_str(),
								static_cast <int> (output[i].name.size()), output[i].name.data(), output[i].value.data()
							);
							// Считаем обнаруженное расхождение
							failures++;
						}
						// Считаем сверенное поле
						fields++;
					}
				}
				// Получаем накопленные инструкции потока декодера
				const string_view feedback = decoder.pending();
				/**
				 * Инструкции потока декодера возвращаются кодеру эталона: без них
				 * он не узнал бы о полученных нами вставках
				 */
				if(!feedback.empty()){
					// Выполняем обработку инструкций потока декодера эталонным кодером
					const nghttp3_ssize read = ::nghttp3_qpack_encoder_read_decoder(
						encoder, reinterpret_cast <const uint8_t *> (feedback.data()), feedback.size()
					);
					// Если эталонный кодер отверг инструкции потока декодера
					if(read < 0){
						// Выводим описание расхождения
						::printf("прогон %zu, набор [%s]: эталон отверг инструкции нашего декодера: %s\n", round, sample.title, ::nghttp3_strerror(static_cast <int> (read)));
						// Считаем обнаруженное расхождение
						failures++;
					// Если инструкции потока декодера приняты
					} else decoder.consumePending(static_cast <size_t> (read));
				}
			}
			// Освобождаем буфер префикса секции полей
			::nghttp3_buf_free(&prefix, mem);
			// Освобождаем буфер представлений полей секции
			::nghttp3_buf_free(&lines, mem);
			// Освобождаем буфер инструкций потока кодера
			::nghttp3_buf_free(&instructions, mem);
			// Считаем сверенную секцию
			sections++;
			// Переходим к следующему потоку запроса
			sid += 4;
		}
	}
	// Удаляем кодер эталонной реализации
	::nghttp3_qpack_encoder_del(encoder);
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
	// Количество сверенных секций
	size_t sections = 0;
	// Количество сверенных полей
	size_t fields = 0;
	// Количество сверенных записей статической таблицы
	size_t entries = 0;
	// Выполняем сверку статической таблицы с эталонной реализацией
	::staticTableAgainstReference(failures, entries);
	// Выполняем сверку нашего кодера с эталонным декодером
	::encoderAgainstReference(failures, sections, fields);
	// Выполняем сверку эталонного кодера с нашим декодером
	::decoderAgainstReference(failures, sections, fields);
	// Выводим итог сверки
	::printf("сверено записей таблицы: %zu, секций: %zu, полей: %zu, расхождений: %zu\n", entries, sections, fields, failures);
	// Выводим код выхода по итогу сверки
	return (failures > 0 ? 1 : 0);
}
