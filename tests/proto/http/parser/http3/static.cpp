/**
 * @file: static.cpp
 * @date: 2026-07-27
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Статические тесты парсера протокола HTTP/3 — константы протокола, слой кадров,
 *        кодек QPACK и сквозные проверки соединения двух парсеров
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "http3.hpp"

/**
 * Используем пространства имён протокола
 */
using namespace awh;
using namespace awh::http;
using namespace awh::http::h3;

/**
 * @brief Функция сборки заголовка однонаправленного потока
 *
 * @param type тип однонаправленного потока
 * @return     собранный заголовок потока
 *
 */
static std::string unistream(const uint64_t type) noexcept {
	// Собираемый заголовок однонаправленного потока
	std::string result;
	// Записываем тип однонаправленного потока
	frame::serialize::unistream(result, type);
	// Выводим собранный заголовок потока
	return result;
}
/**
 * @brief Функция сборки кадра параметров соединения с набором по умолчанию
 *
 * @return собранный кадр параметров соединения
 *
 */
static std::string settings() noexcept {
	// Собираемый кадр параметров соединения
	std::string result;
	// Набор параметров соединения
	frame::setting_entry_t items[2];
	// Устанавливаем идентификатор параметра размера таблицы QPACK
	items[0].id = static_cast <uint64_t> (setting_t::QPACK_MAX_TABLE_CAPACITY);
	// Устанавливаем значение параметра размера таблицы QPACK
	items[0].value = 4096;
	// Устанавливаем идентификатор параметра числа ожидающих потоков
	items[1].id = static_cast <uint64_t> (setting_t::QPACK_BLOCKED_STREAMS);
	// Устанавливаем значение параметра числа ожидающих потоков
	items[1].value = 16;
	// Собираем кадр параметров соединения
	frame::serialize::settings(result, items, 2);
	// Выводим собранный кадр параметров соединения
	return result;
}

/**
 * @brief Проверка полноты статической таблицы QPACK (RFC 9204 Appendix A)
 *
 */
TEST_F(ParserHttp3Fixture, QpackStaticTableCoverage){
	// Статическая таблица QPACK содержит ровно девяносто девять записей
	ASSERT_EQ(qpack::STATIC_TABLE_SIZE, 99u);
	/**
	 * Выполняем перебор всех записей статической таблицы
	 */
	for(size_t i = 0; i < qpack::STATIC_TABLE_SIZE; i++){
		// Получаем запись статической таблицы
		const qpack::static_entry_t * entry = qpack::staticTable(i);
		// Запись обязана существовать
		ASSERT_NE(entry, nullptr);
		// Название записи не может быть пустым
		ASSERT_FALSE(entry->name.empty());
	}
	// Индекс за границей таблицы записи не даёт
	ASSERT_EQ(qpack::staticTable(qpack::STATIC_TABLE_SIZE), nullptr);
	// Нулевой индекс адресует первую запись, а не отсутствие записи
	ASSERT_EQ(qpack::staticTable(0)->name, ":authority");
	// Последняя запись таблицы известна спецификацией
	ASSERT_EQ(qpack::staticTable(98)->name, "x-frame-options");
	// Значение последней записи таблицы известно спецификацией
	ASSERT_EQ(qpack::staticTable(98)->value, "sameorigin");
}
/**
 * @brief Проверка поиска в статической таблице QPACK
 *
 */
TEST_F(ParserHttp3Fixture, QpackStaticTableLookup){
	// Индекс полного совпадения
	size_t index = 0;
	// Индекс совпадения только по названию
	size_t nameOnly = 0;
	// Полное совпадение пары название-значение обязано находиться
	ASSERT_TRUE(qpack::stat::find(":method", "GET", index, nameOnly));
	// Индекс метода GET задан спецификацией
	ASSERT_EQ(index, 17u);
	/**
	 * Записи с одним названием в статической таблице QPACK не занимают непрерывный
	 * отрезок: статус встречается и в позициях 24-28, и в позициях 63-71
	 */
	ASSERT_TRUE(qpack::stat::find(":status", "425", index, nameOnly));
	// Индекс статуса 425 лежит во второй группе записей
	ASSERT_EQ(index, 70u);
	// Совпадение только по названию обязано находиться при отсутствии полного
	ASSERT_FALSE(qpack::stat::find(":status", "599", index, nameOnly));
	// Индекс совпадения только по названию обязан быть найден
	ASSERT_LT(nameOnly, qpack::STATIC_TABLE_SIZE);
	// Название найденной записи обязано совпасть с искомым
	ASSERT_EQ(qpack::staticTable(nameOnly)->name, ":status");
	// Отсутствующее название совпадения не даёт
	ASSERT_FALSE(qpack::stat::find("x-no-such-header", "value", index, nameOnly));
	// Индекс совпадения только по названию обязан остаться незаполненным
	ASSERT_EQ(nameOnly, qpack::STATIC_TABLE_SIZE);
}
/**
 * @brief Проверка кодирования и декодирования секции полей QPACK
 *
 */
TEST_F(ParserHttp3Fixture, QpackRoundTrip){
	// Объект кодера полей
	qpack::encoder_t encoder;
	// Объект декодера полей
	qpack::decoder_t decoder(4096, 16);
	// Устанавливаем ёмкость таблицы, анонсированную пиром
	encoder.maxCapacity(4096);
	// Устанавливаем число потоков, которым пир разрешил ожидание
	encoder.maxBlocked(16);
	// Набор кодируемых полей
	const std::vector <qpack::field_t> fields = {
		qpack::field_t{":method", "GET"},
		qpack::field_t{":path", "/index.html"},
		qpack::field_t{"user-agent", "awh/5.0"},
		qpack::field_t{"x-custom", "значение"}
	};
	/**
	 * Прогоняем набор несколько раз: первый проход наполняет динамическую таблицу,
	 * а установившийся режим проверяется последующими
	 */
	for(uint64_t sid = 0; sid < 16; sid += 4){
		// Буфер секции полей
		std::string section;
		// Выполняем кодирование секции полей
		encoder.encode(sid, fields, section);
		// Получаем накопленные инструкции потока кодера
		const std::string_view instructions = encoder.pending();
		// Количество разобранных октетов
		size_t consumed = 0;
		// Код ошибки протокола
		error_t error = error_t::H3_NO_ERROR;
		// Если инструкции потока кодера накоплены
		if(!instructions.empty()){
			// Инструкции обязаны разбираться декодером
			ASSERT_EQ(decoder.decodeEncoderStream(instructions, consumed, error), status_t::OK);
			// Отмечаем инструкции отправленными
			encoder.consumePending(consumed);
		}
		// Декодированные поля секции
		std::vector <qpack::field_view_t> output;
		// Секция полей обязана разбираться
		ASSERT_EQ(decoder.decode(sid, section, output, 0, error), status_t::OK);
		// Количество декодированных полей обязано совпасть
		ASSERT_EQ(output.size(), fields.size());
		/**
		 * Выполняем сверку всех декодированных полей
		 */
		for(size_t i = 0; i < output.size(); i++){
			// Название поля обязано совпасть
			ASSERT_EQ(output[i].name, fields[i].name);
			// Значение поля обязано совпасть
			ASSERT_EQ(output[i].value, fields[i].value);
		}
		// Получаем накопленные инструкции потока декодера
		const std::string_view feedback = decoder.pending();
		// Если инструкции потока декодера накоплены
		if(!feedback.empty()){
			// Инструкции обязаны разбираться кодером
			ASSERT_EQ(encoder.decodeDecoderStream(feedback, consumed, error), status_t::OK);
			// Отмечаем инструкции отправленными
			decoder.consumePending(consumed);
		}
	}
	// Динамическая таблица кодера обязана наполниться
	ASSERT_GT(encoder.table().inserts(), 0u);
	// Динамическая таблица декодера обязана остаться синхронной с кодером
	ASSERT_EQ(decoder.table().inserts(), encoder.table().inserts());
}
/**
 * @brief Проверка блокировки потока при отставании инструкций QPACK
 *
 * @details Секция полей и инструкции кодека идут разными потоками и обгоняют друг
 *          друга. Секция, пришедшая раньше нужных вставок, обязана откладываться,
 *          а не отвергаться: это штатное состояние протокола
 *
 */
TEST_F(ParserHttp3Fixture, QpackBlockedStream){
	// Объект кодера полей
	qpack::encoder_t encoder;
	// Объект декодера полей
	qpack::decoder_t decoder(4096, 16);
	// Устанавливаем ёмкость таблицы, анонсированную пиром
	encoder.maxCapacity(4096);
	// Устанавливаем число потоков, которым пир разрешил ожидание
	encoder.maxBlocked(16);
	// Отключаем адаптивную индексацию, чтобы вставка произошла с первого поля
	encoder.adaptiveIndexing(false);
	// Набор кодируемых полей
	const std::vector <qpack::field_t> fields = {
		qpack::field_t{"x-session", "0f9c1a2b3d4e5f60"}
	};
	// Буфер секции полей
	std::string section;
	// Выполняем кодирование секции полей
	encoder.encode(0, fields, section);
	// Запоминаем накопленные инструкции потока кодера
	const std::string instructions(encoder.pending());
	// Инструкции обязаны быть накоплены: поле занесено в таблицу
	ASSERT_FALSE(instructions.empty());
	// Декодированные поля секции
	std::vector <qpack::field_view_t> output;
	// Код ошибки протокола
	error_t error = error_t::H3_NO_ERROR;
	/**
	 * Секция подаётся раньше инструкций: декодер обязан сообщить о блокировке,
	 * а не об ошибке
	 */
	ASSERT_EQ(decoder.decode(0, section, output, 0, error), status_t::BLOCKED);
	// Поток обязан числиться заблокированным
	ASSERT_EQ(decoder.blocked(), 1u);
	// Количество разобранных октетов
	size_t consumed = 0;
	// Подаём инструкции потока кодера
	ASSERT_EQ(decoder.decodeEncoderStream(instructions, consumed, error), status_t::OK);
	// После прихода вставок секция обязана разбираться
	ASSERT_EQ(decoder.decode(0, section, output, 0, error), status_t::OK);
	// Блокировка потока обязана быть снята
	ASSERT_EQ(decoder.blocked(), 0u);
	// Количество декодированных полей обязано совпасть
	ASSERT_EQ(output.size(), 1u);
	// Название поля обязано совпасть
	ASSERT_EQ(output[0].name, "x-session");
	// Значение поля обязано совпасть
	ASSERT_EQ(output[0].value, "0f9c1a2b3d4e5f60");
}
/**
 * @brief Проверка отказа от индексации чувствительных полей
 *
 */
TEST_F(ParserHttp3Fixture, QpackSensitiveNotIndexed){
	// Объект кодера полей
	qpack::encoder_t encoder;
	// Устанавливаем ёмкость таблицы, анонсированную пиром
	encoder.maxCapacity(4096);
	// Устанавливаем число потоков, которым пир разрешил ожидание
	encoder.maxBlocked(16);
	// Отключаем адаптивную индексацию: она сама отложила бы первую вставку
	encoder.adaptiveIndexing(false);
	// Набор полей с чувствительным значением
	const std::vector <qpack::field_t> fields = {
		qpack::field_t{"cookie", "session=0f9c1a2b3d4e5f60"}
	};
	// Буфер секции полей
	std::string section;
	/**
	 * Прогоняем набор дважды: одного прохода недостаточно, чтобы отличить отказ
	 * от индексации от ещё не наступившего решения об индексации
	 */
	for(uint64_t sid = 0; sid < 8; sid += 4)
		// Выполняем кодирование секции полей
		encoder.encode(sid, fields, section);
	// Чувствительное поле в динамическую таблицу попасть не должно
	ASSERT_EQ(encoder.table().inserts(), 0u);
}
/**
 * @brief Проверка вытеснения записей динамической таблицы QPACK
 *
 */
TEST_F(ParserHttp3Fixture, QpackDynamicTableEviction){
	// Объект динамической таблицы ёмкостью в две записи по сорок октетов
	qpack::dynamic_table_t table(80);
	// Включаем сопровождение индекса записей
	table.indexing(true);
	// Первая запись обязана вставляться
	ASSERT_TRUE(table.add("aaaa", "1234"));
	// Вторая запись обязана вставляться
	ASSERT_TRUE(table.add("bbbb", "5678"));
	// Таблица обязана содержать обе записи
	ASSERT_EQ(table.count(), 2u);
	// Третья запись обязана вытеснить самую старую
	ASSERT_TRUE(table.add("cccc", "9012"));
	// Количество живых записей обязано остаться прежним
	ASSERT_EQ(table.count(), 2u);
	// Общее количество вставок обязано вырасти
	ASSERT_EQ(table.inserts(), 3u);
	// Самая старая запись обязана быть вытеснена
	ASSERT_EQ(table.at(0), nullptr);
	// Оставшиеся записи обязаны адресоваться абсолютными номерами
	ASSERT_NE(table.at(1), nullptr);
	// Название второй записи обязано совпасть
	ASSERT_EQ(table.at(1)->name, "bbbb");
	// Название третьей записи обязано совпасть
	ASSERT_EQ(table.at(2)->name, "cccc");
	// Номер за границей вставок записи не даёт
	ASSERT_EQ(table.at(3), nullptr);
	/**
	 * Запись, не помещающаяся в ёмкость целиком, опустошает таблицу и сама
	 * не вставляется (RFC 9204 §3.2.2)
	 */
	ASSERT_FALSE(table.add(std::string(100, 'x'), "value"));
	// Таблица обязана опустеть
	ASSERT_EQ(table.count(), 0u);
	// Общее количество вставок изменяться не должно
	ASSERT_EQ(table.inserts(), 3u);
}
/**
 * @brief Проверка распознавания зарезервированных и изъятых идентификаторов
 *
 */
TEST_F(ParserHttp3Fixture, ReservedAndRetiredIdentifiers){
	/**
	 * Зарезервированная последовательность задана как 0x1F * N + 0x21
	 * (RFC 9114 §7.2.8)
	 */
	ASSERT_TRUE(h3::reserved(0x21));
	// Второй элемент последовательности зарезервированных идентификаторов
	ASSERT_TRUE(h3::reserved(0x21 + 0x1F));
	// Третий элемент последовательности зарезервированных идентификаторов
	ASSERT_TRUE(h3::reserved(0x21 + (0x1F * 2)));
	// Идентификаторы вне последовательности зарезервированными не являются
	ASSERT_FALSE(h3::reserved(0x22));
	// Идентификаторы ниже начала последовательности зарезервированными не являются
	ASSERT_FALSE(h3::reserved(0x20));
	/**
	 * Типы кадров, занятые в HTTP/2 кадрами PRIORITY, PING, WINDOW_UPDATE
	 * и CONTINUATION, в HTTP/3 не переиспользуются (RFC 9114 §11.2.1)
	 */
	ASSERT_TRUE(h3::retired(0x02));
	// Тип кадра PING из HTTP/2
	ASSERT_TRUE(h3::retired(0x06));
	// Тип кадра WINDOW_UPDATE из HTTP/2
	ASSERT_TRUE(h3::retired(0x08));
	// Тип кадра CONTINUATION из HTTP/2
	ASSERT_TRUE(h3::retired(0x09));
	// Действующие типы кадров изъятыми не являются
	ASSERT_FALSE(h3::retired(static_cast <uint64_t> (frame_t::DATA)));
	// Тип кадра SETTINGS изъятым не является
	ASSERT_FALSE(h3::retired(static_cast <uint64_t> (frame_t::SETTINGS)));
	/**
	 * Параметры, занятые в HTTP/2 ENABLE_PUSH, MAX_CONCURRENT_STREAMS,
	 * INITIAL_WINDOW_SIZE и MAX_FRAME_SIZE (RFC 9114 §7.2.4.1)
	 */
	ASSERT_TRUE(h3::retiredSetting(0x02));
	// Параметр MAX_FRAME_SIZE из HTTP/2
	ASSERT_TRUE(h3::retiredSetting(0x05));
	// Действующие параметры изъятыми не являются
	ASSERT_FALSE(h3::retiredSetting(static_cast <uint64_t> (setting_t::QPACK_MAX_TABLE_CAPACITY)));
	// Параметр размера секции полей изъятым не является
	ASSERT_FALSE(h3::retiredSetting(static_cast <uint64_t> (setting_t::MAX_FIELD_SECTION_SIZE)));
}
/**
 * @brief Проверка разбора и сборки кадров HTTP/3
 *
 */
TEST_F(ParserHttp3Fixture, FrameRoundTrip){
	// Буфер собираемого кадра
	std::string buffer;
	// Собираем кадр данных тела
	frame::serialize::data(buffer, "hello");
	// Разбираемый заголовок кадра
	frame::header_t head;
	// Заголовок кадра обязан разбираться
	ASSERT_GT(frame::parser::header(reinterpret_cast <const uint8_t *> (buffer.data()), buffer.size(), head), 0u);
	// Тип кадра обязан совпасть
	ASSERT_EQ(head.type, static_cast <uint64_t> (frame_t::DATA));
	// Длина нагрузки кадра обязана совпасть
	ASSERT_EQ(head.length, 5u);
	// Неполный заголовок кадра разбираться не должен
	ASSERT_EQ(frame::parser::header(reinterpret_cast <const uint8_t *> (buffer.data()), 1, head), 0u);
	// Код ошибки протокола
	error_t error = error_t::H3_NO_ERROR;
	// Разобранное значение идентификатора
	uint64_t identifier = 0;
	// Выполняем очистку буфера собираемого кадра
	buffer.clear();
	// Собираем кадр завершения соединения
	frame::serialize::goaway(buffer, 12);
	// Заголовок кадра обязан разбираться
	const size_t used = frame::parser::header(reinterpret_cast <const uint8_t *> (buffer.data()), buffer.size(), head);
	// Нагрузка кадра обязана разбираться
	ASSERT_EQ(frame::parser::identifier(reinterpret_cast <const uint8_t *> (buffer.data() + used), (buffer.size() - used), identifier, error), status_t::OK);
	// Разобранный идентификатор обязан совпасть
	ASSERT_EQ(identifier, 12u);
	/**
	 * Лишние октеты после единственного числа - ошибка: нагрузка обязана
	 * заканчиваться ровно на нём (RFC 9114 §7.1)
	 */
	buffer.push_back('\0');
	// Нагрузка с лишними октетами разбираться не должна
	ASSERT_EQ(frame::parser::identifier(reinterpret_cast <const uint8_t *> (buffer.data() + used), (buffer.size() - used), identifier, error), status_t::ERROR);
	// Код ошибки обязан указывать на нарушение требований к нагрузке
	ASSERT_EQ(error, error_t::H3_FRAME_ERROR);
}
/**
 * @brief Проверка разбора кадра параметров соединения
 *
 */
TEST_F(ParserHttp3Fixture, FrameSettingsDuplicate){
	// Разобранный набор параметров
	std::vector <frame::setting_entry_t> items;
	// Код ошибки протокола
	error_t error = error_t::H3_NO_ERROR;
	// Собираемая нагрузка кадра параметров
	std::string payload;
	// Записываем идентификатор первого параметра
	quic::varint::write(payload, static_cast <uint64_t> (setting_t::QPACK_MAX_TABLE_CAPACITY));
	// Записываем значение первого параметра
	quic::varint::write(payload, 4096);
	// Записываем идентификатор второго параметра
	quic::varint::write(payload, static_cast <uint64_t> (setting_t::QPACK_BLOCKED_STREAMS));
	// Записываем значение второго параметра
	quic::varint::write(payload, 16);
	// Набор параметров обязан разбираться
	ASSERT_EQ(frame::parser::settings(reinterpret_cast <const uint8_t *> (payload.data()), payload.size(), items, error), status_t::OK);
	// Количество разобранных параметров обязано совпасть
	ASSERT_EQ(items.size(), 2u);
	/**
	 * Повторное объявление параметра - ошибка уровня соединения
	 * (RFC 9114 §7.2.4.1)
	 */
	quic::varint::write(payload, static_cast <uint64_t> (setting_t::QPACK_MAX_TABLE_CAPACITY));
	// Записываем значение повторного параметра
	quic::varint::write(payload, 8192);
	// Набор с повторным параметром разбираться не должен
	ASSERT_EQ(frame::parser::settings(reinterpret_cast <const uint8_t *> (payload.data()), payload.size(), items, error), status_t::ERROR);
	// Код ошибки обязан указывать на недопустимое содержимое кадра параметров
	ASSERT_EQ(error, error_t::H3_SETTINGS_ERROR);
}
/**
 * @brief Проверка обмена параметрами соединения
 *
 */
TEST_F(ParserHttp3Fixture, ConnectionHandshake){
	// Стороны соединения
	endpoint_t client, server;
	// Подготавливаем сторону клиента
	this->setup(client, direct_t::RESPONSE);
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Выполняем рукопожатие соединения
	this->handshake(client, server);
	// Клиент обязан получить параметры сервера
	ASSERT_TRUE(client.parser->isSettingsReceived());
	// Сервер обязан получить параметры клиента
	ASSERT_TRUE(server.parser->isSettingsReceived());
	// Событие применения параметров обязано быть выпущено ровно один раз
	ASSERT_EQ(client.events.settings, 1u);
	// Событие применения параметров сервера обязано быть выпущено ровно один раз
	ASSERT_EQ(server.events.settings, 1u);
	// Ошибок уровня соединения быть не должно
	ASSERT_TRUE(client.events.errors.empty());
	// Ошибок уровня соединения на стороне сервера быть не должно
	ASSERT_TRUE(server.events.errors.empty());
	// Ёмкость таблицы QPACK пира обязана быть принята
	ASSERT_EQ(server.parser->remoteSettings().qpackMaxTableCapacity, proto::QPACK_TABLE_CAPACITY);
}
/**
 * @brief Проверка сквозного обмена запросом и ответом
 *
 */
TEST_F(ParserHttp3Fixture, RequestResponseExchange){
	// Стороны соединения
	endpoint_t client, server;
	// Подготавливаем сторону клиента
	this->setup(client, direct_t::RESPONSE);
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Выполняем рукопожатие соединения
	this->handshake(client, server);
	// Собираем набор полей запроса клиента
	std::vector <qpack::field_t> fields = this->request("POST", "/submit");
	// Дописываем поле типа содержимого
	fields.emplace_back("content-type", "application/json");
	// Дописываем поле объявленной длины тела
	fields.emplace_back("content-length", "13");
	// Отправляем секцию полей запроса
	client.parser->sendHeaders(0, fields, false);
	// Отправляем тело запроса вместе с завершением потока
	client.parser->sendData(0, "{\"ok\":true}\r\n", 13, true);
	// Выполняем прокачку очередей исходящих данных
	this->pump(client, server);
	// Ошибок уровня соединения быть не должно
	ASSERT_TRUE(server.events.errors.empty());
	// Поток обязан быть объявлен открытым
	ASSERT_EQ(server.events.begins.size(), 1u);
	// Идентификатор открытого потока обязан совпасть
	ASSERT_EQ(server.events.begins.front(), 0u);
	// Псевдо-заголовок метода обязан быть доставлен
	ASSERT_EQ(this->field(server.events, 0, ":method"), "POST");
	// Псевдо-заголовок пути обязан быть доставлен
	ASSERT_EQ(this->field(server.events, 0, ":path"), "/submit");
	// Метод запроса обязан быть разобран провайдером
	ASSERT_EQ(server.events.method, "POST");
	// URI-запрос обязан быть разобран провайдером
	ASSERT_EQ(server.events.uri, "/submit");
	// Тело запроса обязано быть доставлено целиком
	ASSERT_EQ(server.events.bodies[0], "{\"ok\":true}\r\n");
	// Последовательность фазовых событий обязана совпасть с документированной
	ASSERT_EQ(this->phases(server.events, 0), "BEGIN:NONE END:HEADERS BEGIN:BODY END:BODY END:NONE");
	// Отправляем секцию полей ответа сервера
	server.parser->sendHeaders(0, this->response("200"), false);
	// Отправляем тело ответа вместе с завершением потока
	server.parser->sendData(0, "pong", 4, true);
	// Выполняем прокачку очередей исходящих данных
	this->pump(client, server);
	// Ошибок уровня соединения на стороне клиента быть не должно
	ASSERT_TRUE(client.events.errors.empty());
	// Псевдо-заголовок статуса обязан быть доставлен
	ASSERT_EQ(this->field(client.events, 0, ":status"), "200");
	// Статус-код ответа обязан быть разобран провайдером
	ASSERT_EQ(client.events.code, 200u);
	// Тело ответа обязано быть доставлено целиком
	ASSERT_EQ(client.events.bodies[0], "pong");
	// Поток обязан быть закрыт после завершения обоих направлений
	ASSERT_EQ(client.events.closes.size(), 1u);
	// Код закрытия потока обязан быть штатным
	ASSERT_EQ(client.events.closes.front().second, error_t::H3_NO_ERROR);
	// Поток обязан быть закрыт и на стороне сервера
	ASSERT_EQ(server.events.closes.size(), 1u);
}
/**
 * @brief Проверка доставки секции трейлеров
 *
 * @details Секция трейлеров завершает тело сообщения, поэтому фаза приёма тела
 *          обязана закрываться до неё, а не после
 *
 */
TEST_F(ParserHttp3Fixture, TrailerSection){
	// Стороны соединения
	endpoint_t client, server;
	// Подготавливаем сторону клиента
	this->setup(client, direct_t::RESPONSE);
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Выполняем рукопожатие соединения
	this->handshake(client, server);
	// Отправляем секцию полей запроса
	client.parser->sendHeaders(0, this->request("GET", "/stream"), true);
	// Выполняем прокачку очередей исходящих данных
	this->pump(client, server);
	// Отправляем секцию полей ответа сервера
	server.parser->sendHeaders(0, this->response("200"), false);
	// Отправляем тело ответа
	server.parser->sendData(0, "chunk", 5, false);
	// Собираем секцию трейлеров
	const std::vector <qpack::field_t> trailers = {qpack::field_t{"x-checksum", "deadbeef"}};
	// Отправляем секцию трейлеров вместе с завершением потока
	server.parser->sendHeaders(0, trailers, true);
	// Выполняем прокачку очередей исходящих данных
	this->pump(client, server);
	// Ошибок уровня соединения быть не должно
	ASSERT_TRUE(client.events.errors.empty());
	// Тело ответа обязано быть доставлено целиком
	ASSERT_EQ(client.events.bodies[0], "chunk");
	// Трейлер обязан быть доставлен как часть секции трейлеров
	ASSERT_EQ(this->field(client.events, 0, "x-checksum"), "deadbeef");
	/**
	 * Фаза приёма тела обязана закрыться до секции трейлеров: иначе события
	 * пришли бы потребителю в обратном порядке
	 */
	ASSERT_EQ(this->phases(client.events, 0), "BEGIN:NONE END:HEADERS BEGIN:BODY END:BODY BEGIN:TRAILER END:TRAILER END:NONE");
	// Провайдер секции трейлеров обязан быть пустым
	ASSERT_FALSE(client.events.providers.empty());
	// Последний провайдер обязан быть провайдером секции трейлеров
	ASSERT_TRUE(std::get <1> (client.events.providers.back()));
}
/**
 * @brief Проверка отбраковки расхождения объявленной и принятой длины тела
 *
 */
TEST_F(ParserHttp3Fixture, ContentLengthMismatch){
	// Стороны соединения
	endpoint_t client, server;
	// Подготавливаем сторону клиента
	this->setup(client, direct_t::RESPONSE);
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Выполняем рукопожатие соединения
	this->handshake(client, server);
	// Собираем набор полей запроса клиента
	std::vector <qpack::field_t> fields = this->request("POST", "/submit");
	// Объявляем длину тела заведомо больше отправляемой
	fields.emplace_back("content-length", "100");
	// Отправляем секцию полей запроса
	client.parser->sendHeaders(0, fields, false);
	// Отправляем тело короче объявленного вместе с завершением потока
	client.parser->sendData(0, "short", 5, true);
	// Выполняем прокачку очередей исходящих данных
	this->pump(client, server);
	/**
	 * Расхождение длины - ошибка сообщения, а не соединения: отвергается один
	 * поток, остальные и само соединение живут (RFC 9110 §8.6)
	 */
	ASSERT_TRUE(server.events.errors.empty());
	// Поток обязан быть оборван
	ASSERT_FALSE(server.events.aborts.empty());
	// Код обрыва потока обязан указывать на ошибку сообщения
	ASSERT_EQ(std::get <1> (server.events.aborts.front()), error_t::H3_MESSAGE_ERROR);
}
/**
 * @brief Проверка отбраковки полей управления соединением
 *
 */
TEST_F(ParserHttp3Fixture, ForbiddenConnectionField){
	// Стороны соединения
	endpoint_t client, server;
	// Подготавливаем сторону клиента
	this->setup(client, direct_t::RESPONSE);
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Выполняем рукопожатие соединения
	this->handshake(client, server);
	// Собираем набор полей запроса клиента
	std::vector <qpack::field_t> fields = this->request("GET", "/");
	/**
	 * Поля управления соединением принадлежат HTTP/1.1 и в HTTP/3 запрещены:
	 * соединением распоряжается транспорт (RFC 9114 §4.2)
	 */
	fields.emplace_back("connection", "keep-alive");
	// Отправляем секцию полей запроса
	client.parser->sendHeaders(0, fields, true);
	// Выполняем прокачку очередей исходящих данных
	this->pump(client, server);
	// Соединение обязано остаться живым
	ASSERT_TRUE(server.events.errors.empty());
	// Поток обязан быть оборван
	ASSERT_FALSE(server.events.aborts.empty());
	// Код обрыва потока обязан указывать на ошибку сообщения
	ASSERT_EQ(std::get <1> (server.events.aborts.front()), error_t::H3_MESSAGE_ERROR);
}
/**
 * @brief Проверка требования начинать управляющий поток кадром SETTINGS
 *
 */
TEST_F(ParserHttp3Fixture, ControlStreamMissingSettings){
	// Сторона сервера
	endpoint_t server;
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Собираем управляющий поток, начатый не кадром параметров
	std::string stream = ::unistream(static_cast <uint64_t> (unistream_t::CONTROL));
	// Дописываем кадр завершения соединения вместо кадра параметров
	frame::serialize::goaway(stream, 0);
	// Подаём собранный управляющий поток на разбор
	ASSERT_EQ(this->feed(server, 2, stream), status_t::ERROR);
	// Ошибка уровня соединения обязана быть зафиксирована
	ASSERT_FALSE(server.events.errors.empty());
	// Код ошибки обязан указывать на отсутствие кадра параметров
	ASSERT_EQ(server.events.errors.front().first, error_t::H3_MISSING_SETTINGS);
}
/**
 * @brief Проверка отбраковки второго управляющего потока
 *
 */
TEST_F(ParserHttp3Fixture, SecondControlStream){
	// Сторона сервера
	endpoint_t server;
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Собираем управляющий поток с кадром параметров
	std::string stream = ::unistream(static_cast <uint64_t> (unistream_t::CONTROL));
	// Дописываем кадр параметров соединения
	stream.append(::settings());
	// Первый управляющий поток обязан приниматься
	ASSERT_EQ(this->feed(server, 2, stream), status_t::OK);
	/**
	 * Управляющий поток в соединении единственный: второй означает ошибку пира
	 * (RFC 9114 §6.2.1)
	 */
	ASSERT_EQ(this->feed(server, 6, stream), status_t::ERROR);
	// Ошибка уровня соединения обязана быть зафиксирована
	ASSERT_FALSE(server.events.errors.empty());
	// Код ошибки обязан указывать на недопустимое создание потока
	ASSERT_EQ(server.events.errors.front().first, error_t::H3_STREAM_CREATION_ERROR);
}
/**
 * @brief Проверка отбраковки закрытия управляющего потока
 *
 */
TEST_F(ParserHttp3Fixture, ClosedCriticalStream){
	// Сторона сервера
	endpoint_t server;
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Собираем управляющий поток с кадром параметров
	std::string stream = ::unistream(static_cast <uint64_t> (unistream_t::CONTROL));
	// Дописываем кадр параметров соединения
	stream.append(::settings());
	/**
	 * Управляющий поток обязан жить всё соединение: его закрытие лишает
	 * соединение управления (RFC 9114 §6.2.1)
	 */
	ASSERT_EQ(this->feed(server, 2, stream, true), status_t::ERROR);
	// Ошибка уровня соединения обязана быть зафиксирована
	ASSERT_FALSE(server.events.errors.empty());
	// Код ошибки обязан указывать на закрытие критического потока
	ASSERT_EQ(server.events.errors.front().first, error_t::H3_CLOSED_CRITICAL_STREAM);
}
/**
 * @brief Проверка отбраковки кадров, изъятых из употребления в HTTP/3
 *
 */
TEST_F(ParserHttp3Fixture, RetiredFrameType){
	// Сторона сервера
	endpoint_t server;
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Собираем управляющий поток с кадром параметров
	std::string stream = ::unistream(static_cast <uint64_t> (unistream_t::CONTROL));
	// Дописываем кадр параметров соединения
	stream.append(::settings());
	/**
	 * Дописываем кадр с типом, занятым в HTTP/2 кадром PING: пир, ошибочно
	 * отправляющий кадры HTTP/2, обязан быть замечен (RFC 9114 §11.2.1)
	 */
	frame::serialize::header(stream, 0x06, 0);
	// Управляющий поток с изъятым типом кадра принят быть не должен
	ASSERT_EQ(this->feed(server, 2, stream), status_t::ERROR);
	// Ошибка уровня соединения обязана быть зафиксирована
	ASSERT_FALSE(server.events.errors.empty());
	// Код ошибки обязан указывать на недопустимый в этом месте кадр
	ASSERT_EQ(server.events.errors.front().first, error_t::H3_FRAME_UNEXPECTED);
}
/**
 * @brief Проверка игнорирования зарезервированных кадров
 *
 * @details Пир отправляет такие кадры намеренно, проверяя, что реализация
 *          не считает набор типов кадров закрытым
 *
 */
TEST_F(ParserHttp3Fixture, ReservedFrameIgnored){
	// Сторона сервера
	endpoint_t server;
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Собираем управляющий поток с кадром параметров
	std::string stream = ::unistream(static_cast <uint64_t> (unistream_t::CONTROL));
	// Дописываем кадр параметров соединения
	stream.append(::settings());
	// Дописываем зарезервированный кадр с произвольной нагрузкой
	frame::serialize::reserved(stream, 3, "произвольная нагрузка");
	// Дописываем кадр завершения соединения после зарезервированного
	frame::serialize::goaway(stream, 0);
	// Управляющий поток обязан разбираться без ошибок
	ASSERT_EQ(this->feed(server, 2, stream), status_t::OK);
	// Ошибок уровня соединения быть не должно
	ASSERT_TRUE(server.events.errors.empty());
	// Кадр завершения соединения после зарезервированного обязан быть разобран
	ASSERT_EQ(server.events.goaways.size(), 1u);
}
/**
 * @brief Проверка отбраковки данных тела до секции полей
 *
 */
TEST_F(ParserHttp3Fixture, DataBeforeHeaders){
	// Сторона сервера
	endpoint_t server;
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Собираем поток запроса, начатый данными тела
	std::string stream;
	// Дописываем кадр данных тела без предшествующей секции полей
	frame::serialize::data(stream, "тело без заголовков");
	/**
	 * Данные до секции полей нарушают порядок частей сообщения
	 * (RFC 9114 §4.1)
	 */
	ASSERT_EQ(this->feed(server, 0, stream), status_t::ERROR);
	// Ошибка уровня соединения обязана быть зафиксирована
	ASSERT_FALSE(server.events.errors.empty());
	// Код ошибки обязан указывать на недопустимый в этом месте кадр
	ASSERT_EQ(server.events.errors.front().first, error_t::H3_FRAME_UNEXPECTED);
}
/**
 * @brief Проверка отбраковки управляющих кадров в потоке сообщения
 *
 */
TEST_F(ParserHttp3Fixture, ControlFrameOnRequestStream){
	// Сторона сервера
	endpoint_t server;
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Собираем поток запроса с кадром параметров соединения
	std::string stream = ::settings();
	/**
	 * Кадр параметров допустим только в управляющем потоке
	 * (RFC 9114 §7.1)
	 */
	ASSERT_EQ(this->feed(server, 0, stream), status_t::ERROR);
	// Ошибка уровня соединения обязана быть зафиксирована
	ASSERT_FALSE(server.events.errors.empty());
	// Код ошибки обязан указывать на недопустимый в этом месте кадр
	ASSERT_EQ(server.events.errors.front().first, error_t::H3_FRAME_UNEXPECTED);
}
/**
 * @brief Проверка отбраковки параметров, изъятых из употребления в HTTP/3
 *
 */
TEST_F(ParserHttp3Fixture, RetiredSettingRejected){
	// Сторона сервера
	endpoint_t server;
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Собираем нагрузку кадра параметров с изъятым параметром
	std::string payload;
	// Записываем идентификатор параметра ENABLE_PUSH из HTTP/2
	quic::varint::write(payload, 0x02);
	// Записываем значение изъятого параметра
	quic::varint::write(payload, 1);
	// Собираем управляющий поток
	std::string stream = ::unistream(static_cast <uint64_t> (unistream_t::CONTROL));
	// Дописываем заголовок кадра параметров
	frame::serialize::header(stream, static_cast <uint64_t> (frame_t::SETTINGS), payload.size());
	// Дописываем нагрузку кадра параметров
	stream.append(payload);
	// Управляющий поток с изъятым параметром принят быть не должен
	ASSERT_EQ(this->feed(server, 2, stream), status_t::ERROR);
	// Ошибка уровня соединения обязана быть зафиксирована
	ASSERT_FALSE(server.events.errors.empty());
	// Код ошибки обязан указывать на недопустимое содержимое кадра параметров
	ASSERT_EQ(server.events.errors.front().first, error_t::H3_SETTINGS_ERROR);
}
/**
 * @brief Проверка реакции на однонаправленный поток неизвестного типа
 *
 */
TEST_F(ParserHttp3Fixture, UnknownUnidirectionalStream){
	// Сторона сервера
	endpoint_t server;
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Собираем однонаправленный поток зарезервированного типа
	std::string stream = ::unistream(proto::GREASE_BASE);
	// Дописываем произвольное содержимое потока
	stream.append("произвольное содержимое");
	/**
	 * Неизвестный тип потока ошибкой не является: содержимое отбрасывается,
	 * а отправителю сообщается, что читать поток мы не будем (RFC 9114 §6.2)
	 */
	ASSERT_EQ(this->feed(server, 2, stream), status_t::OK);
	// Ошибок уровня соединения быть не должно
	ASSERT_TRUE(server.events.errors.empty());
	// Отправителю обязана быть отправлена просьба прекратить передачу
	ASSERT_EQ(server.events.aborts.size(), 1u);
	// Просьба обязана быть остановкой приёма, а не обрывом отправки
	ASSERT_TRUE(std::get <2> (server.events.aborts.front()));
	// Код обязан указывать на недопустимое создание потока
	ASSERT_EQ(std::get <1> (server.events.aborts.front()), error_t::H3_STREAM_CREATION_ERROR);
}
/**
 * @brief Проверка отбраковки возрастания идентификатора в кадре GOAWAY
 *
 */
TEST_F(ParserHttp3Fixture, GoawayMustNotIncrease){
	// Сторона клиента
	endpoint_t client;
	// Подготавливаем сторону клиента
	this->setup(client, direct_t::RESPONSE);
	// Собираем управляющий поток с кадром параметров
	std::string stream = ::unistream(static_cast <uint64_t> (unistream_t::CONTROL));
	// Дописываем кадр параметров соединения
	stream.append(::settings());
	// Дописываем кадр завершения соединения
	frame::serialize::goaway(stream, 8);
	// Управляющий поток обязан разбираться
	ASSERT_EQ(this->feed(client, 3, stream), status_t::OK);
	// Объявленный пиром идентификатор обязан быть доставлен
	ASSERT_EQ(client.events.goaways.size(), 1u);
	// Значение объявленного идентификатора обязано совпасть
	ASSERT_EQ(client.events.goaways.front(), 8u);
	// Выполняем очистку буфера продолжения управляющего потока
	stream.clear();
	// Дописываем кадр завершения с возросшим идентификатором
	frame::serialize::goaway(stream, 12);
	/**
	 * Возрастание идентификатора отозвало бы уже данное обещание не обрабатывать
	 * потоки (RFC 9114 §5.2)
	 */
	ASSERT_EQ(this->feed(client, 3, stream), status_t::ERROR);
	// Ошибка уровня соединения обязана быть зафиксирована
	ASSERT_FALSE(client.events.errors.empty());
	// Код ошибки обязан указывать на идентификатор вне допустимых границ
	ASSERT_EQ(client.events.errors.front().first, error_t::H3_ID_ERROR);
}
/**
 * @brief Проверка неприменимости унаследованной сигнатуры разбора
 *
 * @details Единого байтового потока у соединения HTTP/3 нет. Молчаливый отказ
 *          выглядел бы как исправная работа, поэтому обращение фиксируется ошибкой
 *
 */
TEST_F(ParserHttp3Fixture, InheritedParseSignatureRejected){
	// Сторона сервера
	endpoint_t server;
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Обращение по неприменимой сигнатуре разобранных байтов не даёт
	ASSERT_EQ(server.parser->parse("данные", 6), 0u);
	// Ошибка обязана быть зафиксирована
	ASSERT_FALSE(server.events.errors.empty());
	// Код ошибки обязан указывать на внутреннюю ошибку реализации
	ASSERT_EQ(server.events.errors.front().first, error_t::H3_INTERNAL_ERROR);
	// Итоговый статус разбора обязан стать ошибочным
	ASSERT_EQ(server.parser->status(), parser_t::status_t::ERROR);
}
/**
 * @brief Проверка ограничения размера секции полей
 *
 */
TEST_F(ParserHttp3Fixture, HeaderSectionLimit){
	// Стороны соединения
	endpoint_t client, server;
	// Подготавливаем сторону клиента
	this->setup(client, direct_t::RESPONSE);
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Получаем лимиты безопасности парсера сервера
	awh::http::parser_http3_t::limits_t limits = server.parser->limits();
	// Опускаем лимит размера сжатой секции полей до заведомо малого
	limits.maxHeaderSection = 16;
	// Устанавливаем лимиты безопасности парсера сервера
	server.parser->limits(limits);
	// Выполняем рукопожатие соединения
	this->handshake(client, server);
	// Собираем набор полей запроса клиента
	std::vector <qpack::field_t> fields = this->request("GET", "/");
	/**
	 * Длинное значение обязано попасть в саму секцию, а не в динамическую таблицу:
	 * индексированное поле ушло бы инструкцией потока кадра, а в секции осталась бы
	 * короткая ссылка на него. Поле авторизации кодер считает чувствительным
	 * и не индексирует, поэтому его значение остаётся в секции целиком
	 */
	fields.emplace_back("authorization", std::string(256, 'a'));
	// Отправляем секцию полей запроса
	client.parser->sendHeaders(0, fields, true);
	// Выполняем прокачку очередей исходящих данных
	this->pump(client, server);
	/**
	 * Превышение лимита секции - причина отвергнуть один поток, а не соединение:
	 * остальные потоки к нему отношения не имеют
	 */
	ASSERT_TRUE(server.events.errors.empty());
	// Поток обязан быть оборван
	ASSERT_FALSE(server.events.aborts.empty());
	// Код обрыва потока обязан указывать на чрезмерную нагрузку
	ASSERT_EQ(std::get <1> (server.events.aborts.front()), error_t::H3_EXCESSIVE_LOAD);
}
/**
 * @brief Проверка ограничения суммарного размера тела потока
 *
 */
TEST_F(ParserHttp3Fixture, BodySizeLimit){
	// Стороны соединения
	endpoint_t client, server;
	// Подготавливаем сторону клиента
	this->setup(client, direct_t::RESPONSE);
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Получаем лимиты безопасности парсера сервера
	awh::http::parser_http3_t::limits_t limits = server.parser->limits();
	// Опускаем лимит суммарного размера тела потока до заведомо малого
	limits.maxBodySize = 8;
	// Устанавливаем лимиты безопасности парсера сервера
	server.parser->limits(limits);
	// Выполняем рукопожатие соединения
	this->handshake(client, server);
	// Отправляем секцию полей запроса
	client.parser->sendHeaders(0, this->request("POST", "/upload"), false);
	// Отправляем тело заведомо больше лимита
	client.parser->sendData(0, std::string(64, 'x').data(), 64, true);
	// Выполняем прокачку очередей исходящих данных
	this->pump(client, server);
	// Соединение обязано остаться живым
	ASSERT_TRUE(server.events.errors.empty());
	// Поток обязан быть оборван
	ASSERT_FALSE(server.events.aborts.empty());
	// Код обрыва потока обязан указывать на чрезмерную нагрузку
	ASSERT_EQ(std::get <1> (server.events.aborts.front()), error_t::H3_EXCESSIVE_LOAD);
}
/**
 * @brief Проверка информационного ответа сервера
 *
 * @details Информационный ответ промежуточный: он доставляется, но сообщения
 *          не завершает и фазу приёма не начинает
 *
 */
TEST_F(ParserHttp3Fixture, InformationalResponse){
	// Стороны соединения
	endpoint_t client, server;
	// Подготавливаем сторону клиента
	this->setup(client, direct_t::RESPONSE);
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Выполняем рукопожатие соединения
	this->handshake(client, server);
	// Отправляем секцию полей запроса
	client.parser->sendHeaders(0, this->request("GET", "/slow"), true);
	// Выполняем прокачку очередей исходящих данных
	this->pump(client, server);
	// Отправляем информационный ответ сервера
	server.parser->sendHeaders(0, this->response("103"), false);
	// Выполняем прокачку очередей исходящих данных
	this->pump(client, server);
	// Информационный ответ обязан быть доставлен
	ASSERT_EQ(this->field(client.events, 0, ":status"), "103");
	// Фазовых событий информационный ответ порождать не должен
	ASSERT_TRUE(this->phases(client.events, 0).empty());
	// Отправляем финальный ответ сервера
	server.parser->sendHeaders(0, this->response("200"), true);
	// Выполняем прокачку очередей исходящих данных
	this->pump(client, server);
	// Фазовые события обязаны начаться с финального ответа
	ASSERT_EQ(this->phases(client.events, 0), "BEGIN:NONE END:HEADERS END:NONE");
	// Ошибок уровня соединения быть не должно
	ASSERT_TRUE(client.events.errors.empty());
}
/**
 * @brief Проверка отбраковки смены протокола в HTTP/3
 *
 * @details Смена протокола принадлежит HTTP/1.1: туннели в HTTP/3 поднимаются
 *          расширенным методом CONNECT
 *
 */
TEST_F(ParserHttp3Fixture, SwitchingProtocolsRejected){
	// Стороны соединения
	endpoint_t client, server;
	// Подготавливаем сторону клиента
	this->setup(client, direct_t::RESPONSE);
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Выполняем рукопожатие соединения
	this->handshake(client, server);
	// Отправляем секцию полей запроса
	client.parser->sendHeaders(0, this->request("GET", "/upgrade"), true);
	// Выполняем прокачку очередей исходящих данных
	this->pump(client, server);
	// Отправляем ответ со статусом смены протокола
	server.parser->sendHeaders(0, this->response("101"), true);
	// Выполняем прокачку очередей исходящих данных
	this->pump(client, server);
	// Соединение обязано остаться живым
	ASSERT_TRUE(client.events.errors.empty());
	// Поток обязан быть оборван
	ASSERT_FALSE(client.events.aborts.empty());
	// Код обрыва потока обязан указывать на ошибку сообщения
	ASSERT_EQ(std::get <1> (client.events.aborts.front()), error_t::H3_MESSAGE_ERROR);
}
/**
 * @brief Проверка отказа транспорта открывать однонаправленные потоки
 *
 * @details Транспорт вправе отказать в открытии потока: парсер обязан
 *          не отправлять ничего и повторить попытку позже
 *
 */
TEST_F(ParserHttp3Fixture, TransportRefusesToOpenStreams){
	// Стороны соединения
	endpoint_t client, server;
	// Подготавливаем сторону клиента
	this->setup(client, direct_t::RESPONSE);
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Запрещаем транспорту клиента открывать потоки
	client.refuse = true;
	// Выполняем попытку отправить параметры соединения
	client.parser->sendSettings();
	// Ничего отправлено быть не должно
	ASSERT_TRUE(client.queue.empty());
	// Разрешаем транспорту клиента открывать потоки
	client.refuse = false;
	// Выполняем рукопожатие соединения
	this->handshake(client, server);
	// Сервер обязан получить параметры клиента
	ASSERT_TRUE(server.parser->isSettingsReceived());
	// Ошибок уровня соединения быть не должно
	ASSERT_TRUE(server.events.errors.empty());
}
