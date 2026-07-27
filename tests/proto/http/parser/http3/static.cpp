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
	 * Запись, не помещающаяся в ёмкость целиком, не вставляется. Здесь QPACK
	 * расходится с HPACK: в HPACK такая вставка опустошает таблицу и ошибкой
	 * не является (RFC 7541 §4.4), а в QPACK кодер обязан следить за размером сам,
	 * и попытка - ошибка соединения QPACK_ENCODER_STREAM_ERROR (RFC 9204 §3.2.2).
	 * Поэтому таблица остаётся как была: поднимать ошибку - дело вызывающего,
	 * а трогать состояние на пути, ведущем к обрыву соединения, незачем
	 */
	ASSERT_FALSE(table.add(std::string(100, 'x'), "value"));
	// Таблица обязана остаться нетронутой
	ASSERT_EQ(table.count(), 2u);
	// Общее количество вставок изменяться не должно
	ASSERT_EQ(table.inserts(), 3u);
	// Записи таблицы обязаны остаться на месте
	ASSERT_NE(table.at(2), nullptr);
	// Название последней записи обязано совпасть
	ASSERT_EQ(table.at(2)->name, "cccc");
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
	/**
	 * Обрыв нагрузки посреди пары - нарушение требований к нагрузке любого кадра,
	 * а не к содержимому именно SETTINGS, поэтому код ошибки другой (RFC 9114 §7.1)
	 */
	// Собираем нагрузку из одного идентификатора без значения
	std::string truncated;
	// Записываем идентификатор параметра без его значения
	quic::varint::write(truncated, static_cast <uint64_t> (setting_t::QPACK_MAX_TABLE_CAPACITY));
	// Оборванный набор параметров разбираться не должен
	ASSERT_EQ(frame::parser::settings(reinterpret_cast <const uint8_t *> (truncated.data()), truncated.size(), items, error), status_t::ERROR);
	// Код ошибки обязан указывать на нарушение требований к нагрузке кадра
	ASSERT_EQ(error, error_t::H3_FRAME_ERROR);
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
/**
 * @brief Проверка порядка частей сообщения после блокировки секции полей
 *
 * @details Секция, потребовавшая ещё не пришедших вставок QPACK, откладывается,
 *          но кадры за ней разобрать нельзя: тело до разобранной секции
 *          недопустимо. Хвост потока обязан накопиться и разобраться сразу
 *          после того, как секция разошлась по обработчикам
 *
 */
TEST_F(ParserHttp3Fixture, QpackBlockedSectionKeepsBodyOrder){
	// Сторона сервера
	endpoint_t server;
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Отправляем параметры соединения со стороны сервера
	server.parser->sendSettings();
	// Собираем управляющий поток клиента с кадром параметров
	std::string control = ::unistream(static_cast <uint64_t> (unistream_t::CONTROL));
	// Дописываем кадр параметров соединения
	control.append(::settings());
	// Подаём управляющий поток клиента
	ASSERT_EQ(this->feed(server, 2, control), status_t::OK);
	// Открываем поток инструкций кодера QPACK клиента
	ASSERT_EQ(this->feed(server, 6, ::unistream(static_cast <uint64_t> (unistream_t::QPACK_ENCODER))), status_t::OK);
	/**
	 * Кодер работает в обход потоков соединения: его инструкции подаются вручную
	 * и позже секции - ровно так, как их обгоняет секция на живом соединении
	 */
	qpack::encoder_t encoder;
	// Устанавливаем ёмкость таблицы, анонсированную сервером
	encoder.maxCapacity(4096);
	// Устанавливаем число потоков, которым сервер разрешил ожидание
	encoder.maxBlocked(16);
	// Отключаем адаптивную индексацию, чтобы вставка произошла с первого поля
	encoder.adaptiveIndexing(false);
	// Собираем набор полей запроса клиента
	std::vector <qpack::field_t> fields = this->request("POST", "/upload");
	// Дописываем поле, попадающее в динамическую таблицу
	fields.emplace_back("x-session", "0f9c1a2b3d4e5f60");
	// Буфер секции полей
	std::string section;
	// Выполняем кодирование секции полей
	encoder.encode(0, fields, section);
	// Запоминаем накопленные инструкции потока кодера
	const std::string instructions(encoder.pending());
	// Инструкции обязаны быть накоплены: поле занесено в таблицу
	ASSERT_FALSE(instructions.empty());
	// Собираем поток запроса: секция полей и следом тело
	std::string stream;
	// Дописываем кадр секции полей
	frame::serialize::headers(stream, section);
	// Дописываем кадр тела сообщения
	frame::serialize::data(stream, "hello");
	// Подаём поток запроса, не подавая инструкций кодера
	ASSERT_EQ(this->feed(server, 0, stream, true), status_t::OK);
	// Соединение обязано остаться живым: кадр тела не разбирался вовсе
	ASSERT_TRUE(server.events.errors.empty());
	// Поток обрываться не должен
	ASSERT_TRUE(server.events.aborts.empty());
	// Ничего доставлено быть не должно: секция ждёт вставок
	ASSERT_TRUE(server.events.headers.empty());
	// Тело доставлено быть не должно
	ASSERT_TRUE(server.events.bodies.empty());
	// Подаём инструкции потока кодера
	ASSERT_EQ(this->feed(server, 6, instructions), status_t::OK);
	// Соединение обязано остаться живым
	ASSERT_TRUE(server.events.errors.empty());
	// Поля секции обязаны быть доставлены
	ASSERT_EQ(this->field(server.events, 0, "x-session"), "0f9c1a2b3d4e5f60");
	// Метод запроса обязан совпасть
	ASSERT_EQ(server.events.method, "POST");
	// Тело, накопленное за время блокировки, обязано быть доставлено
	ASSERT_EQ(server.events.bodies[0], "hello");
}
/**
 * @brief Проверка сохранности отложенной секции при второй секции следом
 *
 * @details Слот отложенной секции у потока один, поэтому вторая секция, пришедшая
 *          следом за заблокированной, затирала бы первую - и запрос пропадал бы
 *          молча, без ошибки и обрыва
 *
 */
TEST_F(ParserHttp3Fixture, QpackBlockedSectionNotOverwritten){
	// Сторона сервера
	endpoint_t server;
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Отправляем параметры соединения со стороны сервера
	server.parser->sendSettings();
	// Собираем управляющий поток клиента с кадром параметров
	std::string control = ::unistream(static_cast <uint64_t> (unistream_t::CONTROL));
	// Дописываем кадр параметров соединения
	control.append(::settings());
	// Подаём управляющий поток клиента
	ASSERT_EQ(this->feed(server, 2, control), status_t::OK);
	// Открываем поток инструкций кодера QPACK клиента
	ASSERT_EQ(this->feed(server, 6, ::unistream(static_cast <uint64_t> (unistream_t::QPACK_ENCODER))), status_t::OK);
	// Объект кодера полей, работающий в обход потоков соединения
	qpack::encoder_t encoder;
	// Устанавливаем ёмкость таблицы, анонсированную сервером
	encoder.maxCapacity(4096);
	// Устанавливаем число потоков, которым сервер разрешил ожидание
	encoder.maxBlocked(16);
	// Отключаем адаптивную индексацию, чтобы вставка произошла с первого поля
	encoder.adaptiveIndexing(false);
	// Собираем набор полей запроса клиента
	std::vector <qpack::field_t> fields = this->request("GET", "/");
	// Дописываем поле, попадающее в динамическую таблицу
	fields.emplace_back("x-session", "0f9c1a2b3d4e5f60");
	// Буфер секции полей запроса
	std::string section;
	// Выполняем кодирование секции полей запроса
	encoder.encode(0, fields, section);
	// Запоминаем инструкции, пополняющие таблицу под секцию запроса
	const std::string instructions(encoder.pending());
	// Набор полей секции трейлеров
	std::vector <qpack::field_t> trailers;
	// Дописываем поле секции трейлеров
	trailers.emplace_back("x-checksum", "1a2b3c4d5e6f7a8b");
	// Буфер секции трейлеров
	std::string trailer;
	// Выполняем кодирование секции трейлеров
	encoder.encode(0, trailers, trailer);
	// Собираем поток запроса: секция полей и следом секция трейлеров
	std::string stream;
	// Дописываем кадр секции полей
	frame::serialize::headers(stream, section);
	// Дописываем кадр секции трейлеров
	frame::serialize::headers(stream, trailer);
	// Подаём поток запроса, не подавая инструкций кодера
	ASSERT_EQ(this->feed(server, 0, stream, true), status_t::OK);
	// Подаём инструкции, пополняющие таблицу под секцию запроса
	ASSERT_EQ(this->feed(server, 6, instructions), status_t::OK);
	// Соединение обязано остаться живым
	ASSERT_TRUE(server.events.errors.empty());
	// Секция запроса обязана быть доставлена, а не затёрта секцией трейлеров
	ASSERT_EQ(this->field(server.events, 0, "x-session"), "0f9c1a2b3d4e5f60");
	// Метод запроса обязан совпасть
	ASSERT_EQ(server.events.method, "GET");
}
/**
 * @brief Проверка ограничения хвоста заблокированного потока
 *
 * @details Пир, не присылающий инструкций кодера вовсе, задавал бы потребление
 *          памяти получателем: кадры за заблокированной секцией копятся в буфере
 *
 */
TEST_F(ParserHttp3Fixture, QpackBlockedTailLimit){
	// Сторона сервера
	endpoint_t server;
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Получаем лимиты безопасности парсера сервера
	awh::http::parser_http3_t::limits_t limits = server.parser->limits();
	// Опускаем лимит хвоста заблокированного потока до заведомо малого
	limits.maxBlockedTail = 64;
	// Устанавливаем лимиты безопасности парсера сервера
	server.parser->limits(limits);
	// Отправляем параметры соединения со стороны сервера
	server.parser->sendSettings();
	// Собираем управляющий поток клиента с кадром параметров
	std::string control = ::unistream(static_cast <uint64_t> (unistream_t::CONTROL));
	// Дописываем кадр параметров соединения
	control.append(::settings());
	// Подаём управляющий поток клиента
	ASSERT_EQ(this->feed(server, 2, control), status_t::OK);
	// Объект кодера полей, работающий в обход потоков соединения
	qpack::encoder_t encoder;
	// Устанавливаем ёмкость таблицы, анонсированную сервером
	encoder.maxCapacity(4096);
	// Устанавливаем число потоков, которым сервер разрешил ожидание
	encoder.maxBlocked(16);
	// Отключаем адаптивную индексацию, чтобы вставка произошла с первого поля
	encoder.adaptiveIndexing(false);
	// Собираем набор полей запроса клиента
	std::vector <qpack::field_t> fields = this->request("POST", "/upload");
	// Дописываем поле, попадающее в динамическую таблицу
	fields.emplace_back("x-session", "0f9c1a2b3d4e5f60");
	// Буфер секции полей
	std::string section;
	// Выполняем кодирование секции полей
	encoder.encode(0, fields, section);
	// Собираем поток запроса: секция полей и следом тело заведомо больше лимита
	std::string stream;
	// Дописываем кадр секции полей
	frame::serialize::headers(stream, section);
	// Дописываем кадр тела сообщения
	frame::serialize::data(stream, std::string(256, 'x'));
	// Подаём поток запроса, не подавая инструкций кодера
	ASSERT_EQ(this->feed(server, 0, stream, false), status_t::OK);
	// Соединение обязано остаться живым: это причина отвергнуть один поток
	ASSERT_TRUE(server.events.errors.empty());
	// Поток обязан быть оборван
	ASSERT_FALSE(server.events.aborts.empty());
	// Код обрыва потока обязан указывать на чрезмерную нагрузку
	ASSERT_EQ(std::get <1> (server.events.aborts.front()), error_t::H3_EXCESSIVE_LOAD);
}
/**
 * @brief Проверка отбраковки потока отменённого обещания push
 *
 * @details Отмена обгоняет поток: кадр CANCEL_PUSH идёт управляющим потоком,
 *          а сам push - своим, и порядок между ними не задан
 *
 */
TEST_F(ParserHttp3Fixture, CancelledPushStreamAborted){
	// Сторона клиента
	endpoint_t client;
	// Подготавливаем сторону клиента
	this->setup(client, direct_t::RESPONSE);
	// Отправляем параметры соединения со стороны клиента
	client.parser->sendSettings();
	// Разрешаем серверу выдавать обещания push
	client.parser->sendMaxPushId(8);
	// Собираем управляющий поток сервера с кадром параметров
	std::string control = ::unistream(static_cast <uint64_t> (unistream_t::CONTROL));
	// Дописываем кадр параметров соединения
	control.append(::settings());
	// Подаём управляющий поток сервера
	ASSERT_EQ(this->feed(client, 3, control), status_t::OK);
	// Отменяем обещание push
	client.parser->sendCancelPush(0);
	// Отменённое обещание обязано числиться отменённым
	ASSERT_TRUE(client.parser->pushCancelled(0));
	// Собираем поток push с идентификатором отменённого обещания
	std::string stream = ::unistream(static_cast <uint64_t> (unistream_t::PUSH));
	// Дописываем идентификатор обещанного push
	quic::varint::write(stream, 0);
	// Дописываем кадр секции полей ответа
	frame::serialize::headers(stream, std::string(1, '\x00') + std::string(1, '\x00') + std::string(1, '\xD9'));
	// Подаём поток отменённого push
	ASSERT_EQ(this->feed(client, 7, stream), status_t::OK);
	// Соединение обязано остаться живым
	ASSERT_TRUE(client.events.errors.empty());
	// Поток обязан быть оборван
	ASSERT_FALSE(client.events.aborts.empty());
	// Код обрыва потока обязан указывать на отмену запроса
	ASSERT_EQ(std::get <1> (client.events.aborts.front()), error_t::H3_REQUEST_CANCELLED);
	// Приём потока обязан быть остановлен
	ASSERT_TRUE(std::get <2> (client.events.aborts.front()));
	// Ничего доставлено быть не должно
	ASSERT_TRUE(client.events.headers.empty());
	// Запись отмены обязана сняться приходом потока
	ASSERT_FALSE(client.parser->pushCancelled(0));
}
/**
 * @brief Проверка класса идентификатора в кадре GOAWAY
 *
 * @details Сервер объявляет в GOAWAY идентификатор потока запроса, а потоки
 *          запросов бывают только двунаправленными и только клиентскими
 *          (RFC 9114 §5.2)
 *
 */
TEST_F(ParserHttp3Fixture, GoawayStreamIdClass){
	// Сторона клиента
	endpoint_t client;
	// Подготавливаем сторону клиента
	this->setup(client, direct_t::RESPONSE);
	// Отправляем параметры соединения со стороны клиента
	client.parser->sendSettings();
	// Собираем управляющий поток сервера с кадром параметров
	std::string control = ::unistream(static_cast <uint64_t> (unistream_t::CONTROL));
	// Дописываем кадр параметров соединения
	control.append(::settings());
	// Дописываем кадр завершения соединения с идентификатором однонаправленного потока
	frame::serialize::goaway(control, 3);
	// Подаём управляющий поток сервера
	ASSERT_EQ(this->feed(client, 3, control), status_t::ERROR);
	// Ошибка уровня соединения обязана быть зафиксирована
	ASSERT_FALSE(client.events.errors.empty());
	// Код ошибки обязан указывать на расхождение в учёте идентификаторов
	ASSERT_EQ(client.events.errors.front().first, error_t::H3_ID_ERROR);
}
/**
 * @brief Проверка отбраковки тела у безтелесного ответа
 *
 * @details Ответы 204 и 304 содержимого не несут по определению
 *          (RFC 9110 §8.6): тело в них делает сообщение искажённым
 *
 */
TEST_F(ParserHttp3Fixture, DataOnHeadlessResponse){
	// Стороны соединения
	endpoint_t client, server;
	// Подготавливаем сторону клиента
	this->setup(client, direct_t::RESPONSE);
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Выполняем рукопожатие соединения
	this->handshake(client, server);
	// Отправляем секцию полей запроса
	client.parser->sendHeaders(0, this->request("GET", "/"), true);
	// Выполняем прокачку очередей исходящих данных
	this->pump(client, server);
	// Отправляем безтелесный ответ сервера
	server.parser->sendHeaders(0, this->response("204"), false);
	// Отправляем тело, которого у ответа 204 быть не может
	server.parser->sendData(0, "hello", 5, true);
	// Выполняем прокачку очередей исходящих данных
	this->pump(client, server);
	// Соединение обязано остаться живым
	ASSERT_TRUE(client.events.errors.empty());
	// Поток обязан быть оборван
	ASSERT_FALSE(client.events.aborts.empty());
	// Код обрыва потока обязан указывать на искажённое сообщение
	ASSERT_EQ(std::get <1> (client.events.aborts.front()), error_t::H3_MESSAGE_ERROR);
	// Тело доставлено быть не должно
	ASSERT_TRUE(client.events.bodies.empty());
}
/**
 * @brief Проверка отбраковки тела сверх объявленной длины
 *
 * @details Расхождение с объявленной длиной видно на первом же лишнем октете,
 *          и ждать завершения потока незачем: иначе лишние октеты прошли бы
 *          через обработчик тела как часть сообщения (RFC 9110 §8.6)
 *
 */
TEST_F(ParserHttp3Fixture, BodyExceedsContentLength){
	// Стороны соединения
	endpoint_t client, server;
	// Подготавливаем сторону клиента
	this->setup(client, direct_t::RESPONSE);
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Выполняем рукопожатие соединения
	this->handshake(client, server);
	// Собираем набор полей запроса клиента
	std::vector <qpack::field_t> fields = this->request("POST", "/upload");
	// Объявляем длину тела заведомо меньше отправляемой
	fields.emplace_back("content-length", "4");
	// Отправляем секцию полей запроса
	client.parser->sendHeaders(0, fields, false);
	// Отправляем тело больше объявленной длины, не завершая поток
	client.parser->sendData(0, "hello world", 11, false);
	// Выполняем прокачку очередей исходящих данных
	this->pump(client, server);
	// Соединение обязано остаться живым
	ASSERT_TRUE(server.events.errors.empty());
	// Поток обязан быть оборван до завершения потока пиром
	ASSERT_FALSE(server.events.aborts.empty());
	// Код обрыва потока обязан указывать на искажённое сообщение
	ASSERT_EQ(std::get <1> (server.events.aborts.front()), error_t::H3_MESSAGE_ERROR);
}
/**
 * @brief Проверка отката ровно последней закодированной секции
 *
 * @details Секция, не ушедшая в сеть, снимается с учёта одна: снятие всего потока
 *          лишило бы записи уже отправленные секции, и подтверждение на них
 *          пришло бы в пустоту - а это ошибка соединения (RFC 9204 §4.4.1)
 *
 */
TEST_F(ParserHttp3Fixture, QpackRollbackKeepsSentSections){
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
	// Набор полей первой секции
	const std::vector <qpack::field_t> first = {qpack::field_t{"x-session", "0f9c1a2b3d4e5f60"}};
	// Буфер первой секции полей
	std::string sent;
	// Кодируем первую секцию полей: она уходит в сеть
	encoder.encode(0, first, sent);
	// Набор полей второй секции
	const std::vector <qpack::field_t> second = {qpack::field_t{"x-checksum", "1a2b3c4d5e6f7a8b"}};
	// Буфер второй секции полей
	std::string dropped;
	// Кодируем вторую секцию полей: в сеть она не уйдёт
	encoder.encode(0, second, dropped);
	// Откатываем ровно вторую секцию
	encoder.rollback(0);
	// Количество разобранных октетов
	size_t consumed = 0;
	// Код ошибки протокола
	error_t error = error_t::H3_NO_ERROR;
	// Подаём декодеру инструкции потока кодера
	ASSERT_EQ(decoder.decodeEncoderStream(encoder.pending(), consumed, error), status_t::OK);
	// Отмечаем инструкции отправленными
	encoder.consumePending(consumed);
	// Декодированные поля секции
	std::vector <qpack::field_view_t> output;
	// Первая секция обязана разбираться
	ASSERT_EQ(decoder.decode(0, sent, output, 0, error), status_t::OK);
	// Получаем накопленные инструкции потока декодера
	const std::string feedback(decoder.pending());
	// Подтверждение секции обязано быть накоплено
	ASSERT_FALSE(feedback.empty());
	/**
	 * Подтверждение первой секции обязано разбираться кодером: откат второй
	 * секции её учёта не касается
	 */
	ASSERT_EQ(encoder.decodeDecoderStream(feedback, consumed, error), status_t::OK);
	// Отмечаем инструкции отправленными
	decoder.consumePending(consumed);
	/**
	 * Второго подтверждения быть не может: откаченная секция в сеть не уходила,
	 * и лишнее подтверждение обязано отвергаться ошибкой потока декодера
	 */
	ASSERT_EQ(encoder.decodeDecoderStream(feedback, consumed, error), status_t::ERROR);
	// Код ошибки обязан указывать на ошибку потока декодера
	ASSERT_EQ(error, error_t::QPACK_DECODER_STREAM_ERROR);
}
/**
 * @brief Проверка того, что отмена обещания push не копится без предела
 *
 * @details Отмена обещания, поток которого уже пришёл, эффекта не имеет
 *          (RFC 9114 §7.2.3), а отмены обещаний, потоки которых не придут
 *          никогда, вытесняются из кольца - но соединение не рвут
 *
 */
TEST_F(ParserHttp3Fixture, CancelPushDoesNotAccumulate){
	// Сторона клиента
	endpoint_t client;
	// Подготавливаем сторону клиента
	this->setup(client, direct_t::RESPONSE);
	// Отправляем параметры соединения со стороны клиента
	client.parser->sendSettings();
	// Разрешаем серверу выдавать обещания push с запасом
	client.parser->sendMaxPushId(4096);
	// Собираем управляющий поток сервера с кадром параметров
	std::string control = ::unistream(static_cast <uint64_t> (unistream_t::CONTROL));
	// Дописываем кадр параметров соединения
	control.append(::settings());
	/**
	 * Дописываем отмены заведомо большего числа обещаний, чем помнит кольцо
	 * и чем разрешено одновременных потоков
	 */
	for(uint64_t identifier = 0; identifier < 512; identifier++)
		// Дописываем кадр отмены очередного обещания push
		frame::serialize::cancelPush(control, identifier);
	// Подаём управляющий поток сервера
	ASSERT_EQ(this->feed(client, 3, control), status_t::OK);
	// Соединение обязано остаться живым: отмены копиться не должны
	ASSERT_TRUE(client.events.errors.empty());
	// Последние отмены обязаны помниться
	ASSERT_TRUE(client.parser->pushCancelled(511));
	// Самые старые отмены обязаны быть вытеснены, а не оборвать соединение
	ASSERT_FALSE(client.parser->pushCancelled(0));
}
/**
 * @brief Проверка отбраковки повторного потока одного обещания push
 *
 * @details Идентификатор обещания используется ровно одним потоком: второй
 *          поток с тем же идентификатором - ошибка соединения (RFC 9114 §4.6)
 *
 */
TEST_F(ParserHttp3Fixture, DuplicatePushStreamRejected){
	// Сторона клиента
	endpoint_t client;
	// Подготавливаем сторону клиента
	this->setup(client, direct_t::RESPONSE);
	// Отправляем параметры соединения со стороны клиента
	client.parser->sendSettings();
	// Разрешаем серверу выдавать обещания push
	client.parser->sendMaxPushId(8);
	// Собираем управляющий поток сервера с кадром параметров
	std::string control = ::unistream(static_cast <uint64_t> (unistream_t::CONTROL));
	// Дописываем кадр параметров соединения
	control.append(::settings());
	// Подаём управляющий поток сервера
	ASSERT_EQ(this->feed(client, 3, control), status_t::OK);
	// Собираем заголовок потока push с идентификатором обещания
	std::string stream = ::unistream(static_cast <uint64_t> (unistream_t::PUSH));
	// Дописываем идентификатор обещанного push
	quic::varint::write(stream, 0);
	// Первый поток обещания обязан приниматься
	ASSERT_EQ(this->feed(client, 7, stream), status_t::OK);
	// Соединение обязано остаться живым
	ASSERT_TRUE(client.events.errors.empty());
	// Второй поток того же обещания обязан обрывать соединение
	ASSERT_EQ(this->feed(client, 11, stream), status_t::ERROR);
	// Ошибка уровня соединения обязана быть зафиксирована
	ASSERT_FALSE(client.events.errors.empty());
	// Код ошибки обязан указывать на расхождение в учёте идентификаторов
	ASSERT_EQ(client.events.errors.front().first, error_t::H3_ID_ERROR);
}
/**
 * @brief Проверка учёта потоков push в лимите одновременных потоков
 *
 * @details Поток push несёт сообщение и живёт в той же карте, что и потоки
 *          запросов: без общего лимита сервер обходил бы его потоками push
 *
 */
TEST_F(ParserHttp3Fixture, PushStreamsCountedInStreamLimit){
	// Сторона клиента
	endpoint_t client;
	// Подготавливаем сторону клиента
	this->setup(client, direct_t::RESPONSE);
	// Получаем лимиты безопасности парсера клиента
	awh::http::parser_http3_t::limits_t limits = client.parser->limits();
	// Опускаем лимит одновременно живых потоков до одного
	limits.maxStreams = 1;
	// Устанавливаем лимиты безопасности парсера клиента
	client.parser->limits(limits);
	// Отправляем параметры соединения со стороны клиента
	client.parser->sendSettings();
	// Разрешаем серверу выдавать обещания push
	client.parser->sendMaxPushId(8);
	// Собираем управляющий поток сервера с кадром параметров
	std::string control = ::unistream(static_cast <uint64_t> (unistream_t::CONTROL));
	// Дописываем кадр параметров соединения
	control.append(::settings());
	// Подаём управляющий поток сервера
	ASSERT_EQ(this->feed(client, 3, control), status_t::OK);
	// Собираем заголовок первого потока push
	std::string first = ::unistream(static_cast <uint64_t> (unistream_t::PUSH));
	// Дописываем идентификатор первого обещанного push
	quic::varint::write(first, 0);
	// Первый поток обещания обязан приниматься
	ASSERT_EQ(this->feed(client, 7, first), status_t::OK);
	// Собираем заголовок второго потока push
	std::string second = ::unistream(static_cast <uint64_t> (unistream_t::PUSH));
	// Дописываем идентификатор второго обещанного push
	quic::varint::write(second, 1);
	// Подаём второй поток обещания
	ASSERT_EQ(this->feed(client, 11, second), status_t::OK);
	// Соединение обязано остаться живым: это причина отвергнуть один поток
	ASSERT_TRUE(client.events.errors.empty());
	// Второй поток обязан быть отвергнут
	ASSERT_FALSE(client.events.aborts.empty());
	// Код обрыва потока обязан указывать на отказ в потоке
	ASSERT_EQ(std::get <1> (client.events.aborts.front()), error_t::H3_REQUEST_REJECTED);
}
/**
 * @brief Проверка сохранности отложенного состояния при повторной блокировке
 *
 * @details Секция и накопленный за ней хвост забираются из состояния потока
 *          на время разбора. Если вставок пришло недостаточно и секция
 *          заблокирована снова, оба обязаны вернуться на место - иначе запрос
 *          пропадает молча на второй порции инструкций кодера
 *
 */
TEST_F(ParserHttp3Fixture, QpackBlockedSectionSurvivesSecondBlock){
	// Сторона сервера
	endpoint_t server;
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Отправляем параметры соединения со стороны сервера
	server.parser->sendSettings();
	// Собираем управляющий поток клиента с кадром параметров
	std::string control = ::unistream(static_cast <uint64_t> (unistream_t::CONTROL));
	// Дописываем кадр параметров соединения
	control.append(::settings());
	// Подаём управляющий поток клиента
	ASSERT_EQ(this->feed(server, 2, control), status_t::OK);
	// Открываем поток инструкций кодера QPACK клиента
	ASSERT_EQ(this->feed(server, 6, ::unistream(static_cast <uint64_t> (unistream_t::QPACK_ENCODER))), status_t::OK);
	// Объект кодера полей, работающий в обход потоков соединения
	qpack::encoder_t encoder;
	// Устанавливаем ёмкость таблицы, анонсированную сервером
	encoder.maxCapacity(4096);
	// Устанавливаем число потоков, которым сервер разрешил ожидание
	encoder.maxBlocked(16);
	// Отключаем адаптивную индексацию, чтобы вставка произошла с первого поля
	encoder.adaptiveIndexing(false);
	// Буфер вспомогательной секции: она нужна только ради первой вставки
	std::string warmup;
	// Кодируем вспомогательную секцию с первым полем
	encoder.encode(0, std::vector <qpack::field_t> {qpack::field_t{"x-first", "0f9c1a2b3d4e5f60"}}, warmup);
	// Запоминаем инструкцию первой вставки
	const std::string insertFirst(encoder.pending());
	// Первая вставка обязана быть накоплена
	ASSERT_FALSE(insertFirst.empty());
	// Отмечаем инструкцию первой вставки отправленной
	encoder.consumePending(insertFirst.size());
	// Собираем набор полей запроса клиента
	std::vector <qpack::field_t> fields = this->request("POST", "/upload");
	// Дописываем поле первой вставки
	fields.emplace_back("x-first", "0f9c1a2b3d4e5f60");
	// Дописываем поле второй вставки
	fields.emplace_back("x-second", "1a2b3c4d5e6f7a8b");
	// Буфер секции полей запроса
	std::string section;
	// Кодируем секцию полей запроса: она потребует обеих вставок
	encoder.encode(0, fields, section);
	// Запоминаем инструкцию второй вставки
	const std::string insertSecond(encoder.pending());
	// Вторая вставка обязана быть накоплена
	ASSERT_FALSE(insertSecond.empty());
	// Собираем поток запроса: секция полей и следом тело
	std::string stream;
	// Дописываем кадр секции полей
	frame::serialize::headers(stream, section);
	// Дописываем кадр тела сообщения
	frame::serialize::data(stream, "hello");
	// Подаём поток запроса, не подавая ни одной вставки
	ASSERT_EQ(this->feed(server, 0, stream, true), status_t::OK);
	// Ничего доставлено быть не должно: секция ждёт обеих вставок
	ASSERT_TRUE(server.events.headers.empty());
	// Подаём только первую вставку: секции этого ещё недостаточно
	ASSERT_EQ(this->feed(server, 6, insertFirst), status_t::OK);
	// Соединение обязано остаться живым
	ASSERT_TRUE(server.events.errors.empty());
	// Секция по-прежнему заблокирована, доставлять нечего
	ASSERT_TRUE(server.events.headers.empty());
	// Подаём вторую вставку: теперь секция разбирается
	ASSERT_EQ(this->feed(server, 6, insertSecond), status_t::OK);
	// Соединение обязано остаться живым
	ASSERT_TRUE(server.events.errors.empty());
	// Поля секции обязаны быть доставлены
	ASSERT_EQ(this->field(server.events, 0, "x-second"), "1a2b3c4d5e6f7a8b");
	// Метод запроса обязан совпасть
	ASSERT_EQ(server.events.method, "POST");
	// Тело, пережившее две блокировки, обязано быть доставлено
	ASSERT_EQ(server.events.bodies[0], "hello");
}
/**
 * @brief Проверка сверки секций повторно обещанного push
 *
 * @details Один push сервер вправе пообещать на нескольких потоках запросов,
 *          и повтор идентификатора обещания сам по себе допустим. Недопустимо
 *          расхождение секций полей при таком повторе (RFC 9114 §7.2.2)
 *
 */
TEST_F(ParserHttp3Fixture, RepeatedPushPromiseSectionMismatch){
	// Сторона клиента
	endpoint_t client;
	// Подготавливаем сторону клиента
	this->setup(client, direct_t::RESPONSE);
	// Отправляем параметры соединения со стороны клиента
	client.parser->sendSettings();
	// Разрешаем серверу выдавать обещания push
	client.parser->sendMaxPushId(8);
	// Собираем управляющий поток сервера с кадром параметров
	std::string control = ::unistream(static_cast <uint64_t> (unistream_t::CONTROL));
	// Дописываем кадр параметров соединения
	control.append(::settings());
	// Подаём управляющий поток сервера
	ASSERT_EQ(this->feed(client, 3, control), status_t::OK);
	// Объект кодера полей сервера, работающий в обход потоков соединения
	qpack::encoder_t encoder;
	// Буфер секции полей обещания
	std::string section;
	// Кодируем секцию полей обещанного запроса
	encoder.encode(0, this->request("GET", "/promised"), section);
	// Буфер нагрузки кадра обещания
	std::string payload;
	// Дописываем идентификатор обещанного push
	quic::varint::write(payload, 0);
	// Дописываем секцию полей обещанного запроса
	payload.append(section);
	// Буфер потока запроса клиента
	std::string stream;
	// Дописываем кадр обещания push
	frame::serialize::header(stream, static_cast <uint64_t> (frame_t::PUSH_PROMISE), payload.size());
	// Дописываем нагрузку кадра обещания
	stream.append(payload);
	// Подаём кадр обещания на разбор
	ASSERT_EQ(this->feed(client, 0, stream), status_t::OK);
	// Обещание обязано быть доставлено
	ASSERT_FALSE(client.events.pushes.empty());
	// Соединение обязано остаться живым
	ASSERT_TRUE(client.events.errors.empty());
	/**
	 * Повторяем то же обещание с той же секцией на другом потоке запроса:
	 * это штатный случай, ошибкой он не является
	 */
	ASSERT_EQ(this->feed(client, 4, stream), status_t::OK);
	// Соединение обязано остаться живым
	ASSERT_TRUE(client.events.errors.empty());
	// Обещание обязано быть доставлено повторно
	ASSERT_EQ(client.events.pushes.size(), 2u);
	// Буфер секции полей расходящегося обещания
	std::string other;
	// Кодируем другую секцию полей под тем же идентификатором обещания
	encoder.encode(0, this->request("GET", "/another"), other);
	// Буфер нагрузки расходящегося обещания
	std::string mismatch;
	// Дописываем тот же идентификатор обещанного push
	quic::varint::write(mismatch, 0);
	// Дописываем расходящуюся секцию полей
	mismatch.append(other);
	// Буфер потока запроса с расходящимся обещанием
	std::string conflict;
	// Дописываем кадр обещания push
	frame::serialize::header(conflict, static_cast <uint64_t> (frame_t::PUSH_PROMISE), mismatch.size());
	// Дописываем нагрузку кадра обещания
	conflict.append(mismatch);
	// Подаём расходящееся обещание на разбор
	ASSERT_EQ(this->feed(client, 8, conflict), status_t::ERROR);
	// Ошибка уровня соединения обязана быть зафиксирована
	ASSERT_FALSE(client.events.errors.empty());
	// Код ошибки обязан указывать на нарушение протокола
	ASSERT_EQ(client.events.errors.front().first, error_t::H3_GENERAL_PROTOCOL_ERROR);
}

/**
 * @brief Проверка семантики секции обещанного push
 *
 * @details Обещание несёт запрос, каким бы ни было направление разбора: его
 *          отправляет сервер от имени клиента (RFC 9114 §4.6). Секция, запросом
 *          не являющаяся, наружу не отдаётся, а обещание отменяется. Вместе
 *          с семантикой на этом пути проверяются и поля секции: до вызова
 *          проверки из защит оставался только лимит распакованного списка
 *
 */
TEST_F(ParserHttp3Fixture, PushPromiseSectionValidatedAsRequest){
	/**
	 * Проверяем три случая искажения секции обещания: ответ вместо запроса,
	 * запрос без обязательного псевдо-заголовка метода и поле управления
	 * соединением, принадлежащее HTTP/1.1
	 */
	for(uint32_t variant = 0; variant < 3; variant++){
		// Сторона клиента
		endpoint_t client;
		// Подготавливаем сторону клиента
		this->setup(client, direct_t::RESPONSE);
		// Отправляем параметры соединения со стороны клиента
		client.parser->sendSettings();
		// Разрешаем серверу выдавать обещания push
		client.parser->sendMaxPushId(8);
		// Собираем управляющий поток сервера с кадром параметров
		std::string control = ::unistream(static_cast <uint64_t> (unistream_t::CONTROL));
		// Дописываем кадр параметров соединения
		control.append(::settings());
		// Подаём управляющий поток сервера
		ASSERT_EQ(this->feed(client, 3, control), status_t::OK);
		// Набор полей искажённой секции обещания
		std::vector <qpack::field_t> fields;
		// Если проверяется секция ответа вместо запроса
		if(variant == 0)
			// Формируем секцию ответа сервера
			fields = this->response("200");
		// Если проверяется запрос без псевдо-заголовка метода
		else if(variant == 1) {
			// Формируем корректный запрос
			fields = this->request("GET", "/promised");
			// Снимаем псевдо-заголовок метода запроса
			fields.erase(fields.begin());
		// Если проверяется поле управления соединением
		} else {
			// Формируем корректный запрос
			fields = this->request("GET", "/promised");
			// Дописываем поле управления соединением HTTP/1.1
			fields.emplace_back("connection", "keep-alive");
		}
		// Объект кодера полей сервера, работающий в обход потоков соединения
		qpack::encoder_t encoder;
		// Буфер секции полей обещания
		std::string section;
		// Кодируем искажённую секцию полей обещания
		encoder.encode(0, fields, section);
		// Буфер нагрузки кадра обещания
		std::string payload;
		// Дописываем идентификатор обещанного push
		quic::varint::write(payload, 0);
		// Дописываем секцию полей обещания
		payload.append(section);
		// Буфер потока запроса клиента
		std::string stream;
		// Дописываем кадр обещания push
		frame::serialize::header(stream, static_cast <uint64_t> (frame_t::PUSH_PROMISE), payload.size());
		// Дописываем нагрузку кадра обещания
		stream.append(payload);
		// Очищаем очередь исходящих данных перед подачей обещания
		client.queue.clear();
		// Подаём кадр обещания на разбор
		ASSERT_EQ(this->feed(client, 0, stream), status_t::OK);
		// Искажённое обещание наружу отдано быть не должно
		ASSERT_TRUE(client.events.pushes.empty());
		// Соединение обязано остаться живым: это ошибка сообщения, а не соединения
		ASSERT_TRUE(client.events.errors.empty());
		// Собранные исходящие байты стороны клиента
		std::string outgoing;
		/**
		 * Выполняем сборку всех исходящих байтов очереди
		 */
		for(const auto & item : client.queue)
			// Дописываем очередную порцию исходящих байтов
			outgoing.append(std::get <1> (item));
		// Буфер ожидаемого кадра отмены обещания
		std::string cancel;
		// Собираем кадр отмены обещанного push
		frame::serialize::cancelPush(cancel, 0);
		// Отмена обещания обязана быть отправлена
		ASSERT_NE(outgoing.find(cancel), std::string::npos);
	}
}

/**
 * @brief Проверка того, что Content-Length информационного ответа не переносится на финальный
 *
 * @details Информационный ответ промежуточный: объявленная в нём длина тела
 *          к финальному сообщению отношения не имеет
 *
 */
TEST_F(ParserHttp3Fixture, InformationalContentLengthIgnored){
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
	// Формируем информационный ответ сервера с объявленной длиной тела
	std::vector <awh::http::h3::qpack::field_t> informational = this->response("103");
	// Дописываем заведомо неподходящую финальному ответу длину тела
	informational.emplace_back("content-length", "100");
	// Отправляем информационный ответ сервера
	server.parser->sendHeaders(0, informational, false);
	// Выполняем прокачку очередей исходящих данных
	this->pump(client, server);
	// Информационный ответ обязан быть доставлен
	ASSERT_EQ(this->field(client.events, 0, ":status"), "103");
	// Отправляем финальный ответ сервера без объявленной длины тела
	server.parser->sendHeaders(0, this->response("200"), false);
	// Формируем тело финального ответа
	const std::string body(5, 'z');
	// Отправляем тело финального ответа
	server.parser->sendData(0, body.data(), body.size(), true);
	// Выполняем прокачку очередей исходящих данных
	this->pump(client, server);
	// Тело обязано быть доставлено целиком
	ASSERT_EQ(client.events.bodies[0], body);
	// Поток не должен быть оборван ошибкой семантики сообщения
	ASSERT_EQ(this->phases(client.events, 0), "BEGIN:NONE END:HEADERS BEGIN:BODY END:BODY END:NONE");
	// Ошибок уровня соединения быть не должно
	ASSERT_TRUE(client.events.errors.empty());
}

/**
 * @brief Проверка запрета новых запросов после GOAWAY сервера
 *
 * @details Сервер объявил, что не станет обрабатывать потоки от указанного
 *          идентификатора: открывать их клиенту незачем (RFC 9114 §5.2)
 *
 */
TEST_F(ParserHttp3Fixture, RequestAfterPeerGoaway){
	// Сторона клиента
	endpoint_t client;
	// Подготавливаем сторону клиента
	this->setup(client, direct_t::RESPONSE);
	// Отправляем параметры соединения со стороны клиента
	client.parser->sendSettings();
	// Открываем поток запроса до объявления сервера
	client.parser->sendHeaders(0, this->request("GET", "/first"), false);
	// Собираем управляющий поток сервера с кадром параметров
	std::string control = ::unistream(static_cast <uint64_t> (unistream_t::CONTROL));
	// Дописываем кадр параметров соединения
	control.append(::settings());
	// Дописываем кадр завершения соединения с идентификатором потока запроса
	frame::serialize::goaway(control, 4);
	// Подаём управляющий поток сервера
	ASSERT_EQ(this->feed(client, 3, control), status_t::OK);
	// Соединение обязано остаться живым
	ASSERT_TRUE(client.events.errors.empty());
	// Объявленный сервером идентификатор обязан быть доставлен
	ASSERT_EQ(client.events.goaways.size(), 1u);
	// Запоминаем объём очереди исходящих данных до отправки
	const size_t pending = client.queue.size();
	// Открывать поток от объявленного идентификатора клиент больше не вправе
	client.parser->sendHeaders(4, this->request("GET", "/second"), true);
	// Проверяем что ничего в сеть не ушло
	ASSERT_EQ(client.queue.size(), pending);
	// Формируем секцию трейлеров уже открытого потока
	std::vector <qpack::field_t> trailers;
	// Дописываем поле секции трейлеров
	trailers.emplace_back("x-checksum", "0f9c");
	// Секции уже открытого потока отправляются штатно
	client.parser->sendHeaders(0, trailers, true);
	// Проверяем что секция трейлеров ушла в сеть
	ASSERT_GT(client.queue.size(), pending);
}

/**
 * @brief Проверка запрета новых обещаний push после GOAWAY клиента
 *
 * @details От клиента GOAWAY несёт идентификатор обещания push: обещания
 *          от него и выше сервер выдавать не вправе (RFC 9114 §5.2)
 *
 */
TEST_F(ParserHttp3Fixture, PushPromiseAfterPeerGoaway){
	// Сторона сервера
	endpoint_t server;
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Отправляем параметры соединения со стороны сервера
	server.parser->sendSettings();
	// Собираем управляющий поток клиента с кадром параметров
	std::string control = ::unistream(static_cast <uint64_t> (unistream_t::CONTROL));
	// Дописываем кадр параметров соединения
	control.append(::settings());
	// Дописываем кадр разрешения обещаний push
	frame::serialize::maxPushId(control, 8);
	// Подаём управляющий поток клиента
	ASSERT_EQ(this->feed(server, 2, control), status_t::OK);
	// Открываем поток запроса подачей начала кадра секции полей
	ASSERT_EQ(this->feed(server, 0, std::string(1, '\x01')), status_t::OK);
	// Формируем поля обещанного запроса
	std::vector <qpack::field_t> promise = this->request("GET", "/style.css");
	// Обещание в границах разрешения клиента выдаётся штатно
	ASSERT_EQ(server.parser->sendPushPromise(0, promise), 0u);
	// Собираем управляющий поток клиента с кадром завершения соединения
	std::string goaway;
	// Дописываем кадр завершения соединения с идентификатором обещания push
	frame::serialize::goaway(goaway, 1);
	// Подаём кадр завершения соединения
	ASSERT_EQ(this->feed(server, 2, goaway), status_t::OK);
	// Соединение обязано остаться живым
	ASSERT_TRUE(server.events.errors.empty());
	// Обещание от объявленного идентификатора выдаваться больше не вправе
	ASSERT_EQ(server.parser->sendPushPromise(0, promise), UINT64_MAX);
}

/**
 * @brief Проверка отбраковки потока push сверх собственного GOAWAY
 *
 * @details Клиент объявил, что не станет принимать обещания от указанного
 *          идентификатора: пришедший следом поток отвергается так же,
 *          как поток запроса сверх объявленного предела (RFC 9114 §5.2)
 *
 */
TEST_F(ParserHttp3Fixture, PushStreamAfterLocalGoaway){
	// Сторона клиента
	endpoint_t client;
	// Подготавливаем сторону клиента
	this->setup(client, direct_t::RESPONSE);
	// Отправляем параметры соединения со стороны клиента
	client.parser->sendSettings();
	// Разрешаем серверу выдавать обещания push
	client.parser->sendMaxPushId(8);
	// Собираем управляющий поток сервера с кадром параметров
	std::string control = ::unistream(static_cast <uint64_t> (unistream_t::CONTROL));
	// Дописываем кадр параметров соединения
	control.append(::settings());
	// Подаём управляющий поток сервера
	ASSERT_EQ(this->feed(client, 3, control), status_t::OK);
	// Объявляем серверу, что обещания от нулевого идентификатора не принимаются
	client.parser->sendGoaway(0);
	// Собираем поток push с отвергаемым идентификатором обещания
	std::string stream = ::unistream(static_cast <uint64_t> (unistream_t::PUSH));
	// Дописываем идентификатор обещанного push
	quic::varint::write(stream, 0);
	// Дописываем кадр секции полей ответа
	frame::serialize::headers(stream, std::string(1, '\x00') + std::string(1, '\x00') + std::string(1, '\xD9'));
	// Подаём поток отвергаемого push
	ASSERT_EQ(this->feed(client, 7, stream), status_t::OK);
	// Соединение обязано остаться живым
	ASSERT_TRUE(client.events.errors.empty());
	// Поток обязан быть оборван
	ASSERT_EQ(client.events.aborts.size(), 1u);
	// Код обрыва потока обязан указывать на отклонение запроса
	ASSERT_EQ(std::get <1> (client.events.aborts.front()), error_t::H3_REQUEST_REJECTED);
	// Приём потока обязан быть остановлен
	ASSERT_TRUE(std::get <2> (client.events.aborts.front()));
	// Ничего доставлено быть не должно
	ASSERT_TRUE(client.events.headers.empty());
}

/**
 * @brief Проверка согласованности анонса и соблюдения предела секции полей
 *
 * @details Анонсировать больше, чем соблюдаем, нельзя: пир получил бы сброс
 *          за размер, который мы сами ему и разрешили (RFC 9114 §4.2.2)
 *
 */
TEST_F(ParserHttp3Fixture, AnnouncedFieldSectionSizeCapped){
	// Стороны соединения
	endpoint_t client, server;
	// Подготавливаем сторону клиента
	this->setup(client, direct_t::RESPONSE);
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Получаем лимиты безопасности сервера
	parser_http3_t::limits_t limits;
	// Устанавливаем соблюдаемый предел распакованного списка полей
	limits.maxHeadersTotal = 4096;
	// Применяем лимиты безопасности сервера
	server.parser->limits(limits);
	// Получаем параметры соединения сервера
	parser_http3_t::settings_t settings = server.parser->settings();
	// Анонсируем предел секции полей заведомо больше соблюдаемого
	settings.maxFieldSectionSize = (1024 * 1024);
	// Применяем параметры соединения сервера
	server.parser->settings(settings);
	// Выполняем рукопожатие соединения
	this->handshake(client, server);
	// Анонсированный предел обязан быть урезан до соблюдаемого
	ASSERT_EQ(client.parser->remoteSettings().maxFieldSectionSize, 4096u);
}

/**
 * @brief Проверка анонса предела секции полей при отключённом лимите списка
 *
 * @details Нулевой лимит распакованного списка означает его отсутствие,
 *          а не запрет полей: урезать анонс по нему нельзя (RFC 9114 §4.2.2)
 *
 */
TEST_F(ParserHttp3Fixture, AnnouncedFieldSectionSizeUnlimited){
	// Стороны соединения
	endpoint_t client, server;
	// Подготавливаем сторону клиента
	this->setup(client, direct_t::RESPONSE);
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Получаем лимиты безопасности сервера
	parser_http3_t::limits_t limits;
	// Снимаем предел распакованного списка полей
	limits.maxHeadersTotal = 0;
	// Применяем лимиты безопасности сервера
	server.parser->limits(limits);
	// Получаем параметры соединения сервера
	parser_http3_t::settings_t settings = server.parser->settings();
	// Анонсируем предел секции полей
	settings.maxFieldSectionSize = (1024 * 1024);
	// Применяем параметры соединения сервера
	server.parser->settings(settings);
	// Выполняем рукопожатие соединения
	this->handshake(client, server);
	// Анонсированный предел обязан дойти до пира неурезанным
	ASSERT_EQ(client.parser->remoteSettings().maxFieldSectionSize, (1024u * 1024u));
}

/**
 * @brief Проверка сохранности списка обходимых потоков при вложенном разборе
 *
 * @details Обход карты потоков выходит в пользовательские функции обратного
 *          вызова, а те вправе вызвать разбор повторно: вложенный обход не
 *          должен собирать свой список поверх нашего
 *
 */
TEST_F(ParserHttp3Fixture, NestedParseKeepsStreamList){
	// Сторона сервера
	endpoint_t server;
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Отправляем параметры соединения со стороны сервера
	server.parser->sendSettings();
	// Собираем управляющий поток клиента с кадром параметров
	std::string control = ::unistream(static_cast <uint64_t> (unistream_t::CONTROL));
	// Дописываем кадр параметров соединения
	control.append(::settings());
	// Подаём управляющий поток клиента
	ASSERT_EQ(this->feed(server, 2, control), status_t::OK);
	// Открываем поток инструкций кодера QPACK клиента
	ASSERT_EQ(this->feed(server, 6, ::unistream(static_cast <uint64_t> (unistream_t::QPACK_ENCODER))), status_t::OK);
	/**
	 * Открываем два потока подачей начала кадра секции полей: поток создаётся
	 * с первого октета, а разбирать в нём пока нечего
	 */
	ASSERT_EQ(this->feed(server, 0, std::string(1, '\x01')), status_t::OK);
	// Открываем второй поток запроса
	ASSERT_EQ(this->feed(server, 4, std::string(1, '\x01')), status_t::OK);
	// Объект кодера, работающего в обход потоков соединения
	qpack::encoder_t encoder;
	// Устанавливаем ёмкость таблицы, анонсированную сервером
	encoder.maxCapacity(4096);
	// Устанавливаем число потоков, которым сервер разрешил ожидание
	encoder.maxBlocked(16);
	// Отключаем адаптивную индексацию, чтобы вставка произошла с первого поля
	encoder.adaptiveIndexing(false);
	// Собираем набор полей запроса клиента
	std::vector <qpack::field_t> fields = this->request("POST", "/upload");
	// Дописываем поле, попадающее в динамическую таблицу
	fields.emplace_back("x-session", "0f9c1a2b3d4e5f60");
	// Буфер секции полей
	std::string section;
	// Выполняем кодирование секции полей
	encoder.encode(8, fields, section);
	// Запоминаем накопленные инструкции потока кодера
	const std::string instructions(encoder.pending());
	// Инструкции обязаны быть накоплены: поле занесено в таблицу
	ASSERT_FALSE(instructions.empty());
	// Собираем поток запроса с секцией, ждущей вставок
	std::string blocked;
	// Дописываем кадр секции полей
	frame::serialize::headers(blocked, section);
	// Подаём поток запроса, не подавая инструкций кодера
	ASSERT_EQ(this->feed(server, 8, blocked), status_t::OK);
	// Ничего доставлено быть не должно: секция ждёт вставок
	ASSERT_TRUE(server.events.headers.empty());
	// Собранные идентификаторы закрытых потоков
	std::vector <uint64_t> closed;
	// Признак того, что вложенный разбор ещё не выполнялся
	bool nested = true;
	/**
	 * Обработчик закрытия потока подаёт инструкции кодера: те разблокируют
	 * отложенную секцию и запустят обход заблокированных потоков прямо изнутри
	 * обхода закрываемых
	 */
	server.parser->on(parser_http3_t::close_callback_t([&](const uint64_t sid, const parser_http3_t::error_t code) noexcept {
		// Собираем идентификатор закрытого потока
		closed.push_back(sid);
		// Не используемый параметр
		(void) code;
		// Если вложенный разбор ещё не выполнялся
		if(nested){
			// Помечаем что вложенный разбор выполнен
			nested = false;
			// Подаём инструкции потока кодера изнутри обработчика
			this->feed(server, 6, instructions);
		}
	}));
	// Выполняем завершение ввода
	server.parser->eof();
	// Событие закрытия обязано прийти по каждому из трёх открытых потоков
	ASSERT_EQ(closed.size(), 3u);
}

/**
 * @brief Проверка отбраковки двунаправленного потока, открытого сервером
 *
 * @details Двунаправленные потоки в HTTP/3 открывает только клиент
 *          (RFC 9114 §6.1)
 *
 */
TEST_F(ParserHttp3Fixture, ServerInitiatedBidiRejected){
	/**
	 * Серверный идентификатор двунаправленного потока незаконен для обеих сторон:
	 * проверяем и клиента, и сервер
	 */
	for(uint32_t variant = 0; variant < 2; variant++){
		// Сторона соединения
		endpoint_t endpoint;
		// Подготавливаем сторону соединения
		this->setup(endpoint, ((variant == 0) ? direct_t::RESPONSE : direct_t::REQUEST));
		// Отправляем параметры соединения
		endpoint.parser->sendSettings();
		// Собираем управляющий поток пира с кадром параметров
		std::string control = ::unistream(static_cast <uint64_t> (unistream_t::CONTROL));
		// Дописываем кадр параметров соединения
		control.append(::settings());
		// Подаём управляющий поток пира
		ASSERT_EQ(this->feed(endpoint, ((variant == 0) ? 3 : 2), control), status_t::OK);
		// Подаём начало кадра секции полей в двунаправленный поток серверной чётности
		ASSERT_EQ(this->feed(endpoint, 1, std::string(1, '\x01')), status_t::ERROR);
		// Ошибка уровня соединения обязана быть зафиксирована
		ASSERT_FALSE(endpoint.events.errors.empty());
		// Код ошибки обязан указывать на недопустимое создание потока
		ASSERT_EQ(endpoint.events.errors.front().first, error_t::H3_STREAM_CREATION_ERROR);
	}
}

/**
 * @brief Проверка замещающей семантики сигнала приоритета (RFC 9218 §4)
 *
 * @details Сигнал задаёт приоритет целиком: параметр, в нём отсутствующий,
 *          принимает значение по умолчанию, а не сохраняет прежнее
 *
 */
TEST_F(ParserHttp3Fixture, PriorityUpdateReplacesDefaults){
	// Сторона сервера
	endpoint_t server;
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Отправляем параметры соединения со стороны сервера
	server.parser->sendSettings();
	// Собираем управляющий поток клиента с кадром параметров
	std::string control = ::unistream(static_cast <uint64_t> (unistream_t::CONTROL));
	// Дописываем кадр параметров соединения
	control.append(::settings());
	// Подаём управляющий поток клиента
	ASSERT_EQ(this->feed(server, 2, control), status_t::OK);
	// Открываем поток запроса подачей начала кадра секции полей
	ASSERT_EQ(this->feed(server, 0, std::string(1, '\x01')), status_t::OK);
	// Приоритет по умолчанию обязан быть неинкрементальным
	ASSERT_EQ(server.parser->priority(0).urgency, 3);
	// Признак инкрементальной доставки по умолчанию снят
	ASSERT_FALSE(server.parser->priority(0).incremental);
	// Собираем кадр приоритета с инкрементальной доставкой
	std::string update;
	// Дописываем кадр приоритета потока
	frame::serialize::priorityUpdate(update, false, 0, "u=1, i");
	// Подаём кадр приоритета
	ASSERT_EQ(this->feed(server, 2, update), status_t::OK);
	// Срочность потока обязана примениться
	ASSERT_EQ(server.parser->priority(0).urgency, 1);
	// Признак инкрементальной доставки обязан примениться
	ASSERT_TRUE(server.parser->priority(0).incremental);
	// Выполняем очистку буфера кадра
	update.clear();
	// Собираем кадр приоритета без признака инкрементальной доставки
	frame::serialize::priorityUpdate(update, false, 0, "u=5");
	// Подаём кадр приоритета
	ASSERT_EQ(this->feed(server, 2, update), status_t::OK);
	// Новая срочность потока обязана примениться
	ASSERT_EQ(server.parser->priority(0).urgency, 5);
	// Признак инкрементальной доставки обязан вернуться к значению по умолчанию
	ASSERT_FALSE(server.parser->priority(0).incremental);
}

/**
 * @brief Проверка приоритета, объявленного до открытия потока
 *
 * @details Кадр PRIORITY_UPDATE идёт по управляющему потоку, а порядок между
 *          потоками QUIC не гарантирует вовсе: сигнал вправе прийти раньше
 *          секции полей и обязан примениться при открытии потока (RFC 9218 §7.2).
 *          Он же перекрывает заголовок [priority] независимо от порядка прихода:
 *          именно так RFC предписывает решать эту гонку (§7)
 *
 */
TEST_F(ParserHttp3Fixture, PriorityUpdateBeforeStream){
	/**
	 * Проверяем обе ветки: секция полей без объявления приоритета и секция,
	 * объявляющая приоритет заголовком поверх опередившего её кадра
	 */
	for(uint32_t variant = 0; variant < 2; variant++){
		// Сторона сервера
		endpoint_t server;
		// Подготавливаем сторону сервера
		this->setup(server, direct_t::REQUEST);
		// Отправляем параметры соединения со стороны сервера
		server.parser->sendSettings();
		// Собираем управляющий поток клиента с кадром параметров
		std::string control = ::unistream(static_cast <uint64_t> (unistream_t::CONTROL));
		// Дописываем кадр параметров соединения
		control.append(::settings());
		// Дописываем кадр приоритета ещё не открытого потока запроса
		frame::serialize::priorityUpdate(control, false, 0, "u=1, i");
		// Подаём управляющий поток клиента
		ASSERT_EQ(this->feed(server, 2, control), status_t::OK);
		// Формируем поля запроса
		std::vector <qpack::field_t> fields = this->request("GET", "/data");
		// Если проверяется секция с объявленным заголовком приоритетом
		if(variant == 1)
			// Дописываем заголовок приоритета с наименьшей срочностью
			fields.emplace_back("priority", "u=7");
		// Объект кодера полей клиента, работающий в обход потоков соединения
		qpack::encoder_t encoder;
		// Буфер секции полей запроса
		std::string section;
		// Кодируем секцию полей запроса
		encoder.encode(0, fields, section);
		// Буфер потока запроса
		std::string stream;
		// Дописываем кадр секции полей
		frame::serialize::header(stream, static_cast <uint64_t> (frame_t::HEADERS), section.size());
		// Дописываем секцию полей запроса
		stream.append(section);
		// Подаём поток запроса на разбор
		ASSERT_EQ(this->feed(server, 0, stream, true), status_t::OK);
		// Соединение обязано остаться живым
		ASSERT_TRUE(server.events.errors.empty());
		// Срочность из опередившего кадра обязана примениться
		ASSERT_EQ(server.parser->priority(0).urgency, 1);
		// Признак инкрементальной доставки из опередившего кадра обязан примениться
		ASSERT_TRUE(server.parser->priority(0).incremental);
	}
}

/**
 * @brief Проверка приоритета обещания push
 *
 * @details Приоритет push адресуется идентификатором обещания, а не потока:
 *          поток push откроется позже и может не открыться вовсе (RFC 9218 §7.2).
 *          Кадр разбирался и прежде, но состояния не менял вовсе
 *
 */
TEST_F(ParserHttp3Fixture, PushPriorityApplied){
	// Сторона сервера
	endpoint_t server;
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Отправляем параметры соединения со стороны сервера
	server.parser->sendSettings();
	// Собираем управляющий поток клиента с кадром параметров
	std::string control = ::unistream(static_cast <uint64_t> (unistream_t::CONTROL));
	// Дописываем кадр параметров соединения
	control.append(::settings());
	// Подаём управляющий поток клиента
	ASSERT_EQ(this->feed(server, 2, control), status_t::OK);
	// Приоритет неизвестного обещания обязан быть значением по умолчанию
	ASSERT_EQ(server.parser->pushPriority(5).urgency, 3);
	// Признак инкрементальной доставки неизвестного обещания снят
	ASSERT_FALSE(server.parser->pushPriority(5).incremental);
	// Буфер кадра приоритета обещания push
	std::string update;
	// Собираем кадр приоритета обещания push
	frame::serialize::priorityUpdate(update, true, 5, "u=2, i");
	// Подаём кадр приоритета обещания
	ASSERT_EQ(this->feed(server, 2, update), status_t::OK);
	// Соединение обязано остаться живым
	ASSERT_TRUE(server.events.errors.empty());
	// Срочность обещания обязана примениться
	ASSERT_EQ(server.parser->pushPriority(5).urgency, 2);
	// Признак инкрементальной доставки обещания обязан примениться
	ASSERT_TRUE(server.parser->pushPriority(5).incremental);
	// Выполняем очистку буфера кадра
	update.clear();
	// Собираем кадр приоритета того же обещания без признака инкрементальности
	frame::serialize::priorityUpdate(update, true, 5, "u=6");
	// Подаём кадр приоритета обещания
	ASSERT_EQ(this->feed(server, 2, update), status_t::OK);
	// Новая срочность обещания обязана примениться
	ASSERT_EQ(server.parser->pushPriority(5).urgency, 6);
	// Признак инкрементальной доставки обязан вернуться к значению по умолчанию
	ASSERT_FALSE(server.parser->pushPriority(5).incremental);
	// Приоритет другого обещания остаётся значением по умолчанию
	ASSERT_EQ(server.parser->pushPriority(9).urgency, 3);
}

/**
 * @brief Проверка открытия фазы приёма тела по кадрированию, а не по данным
 *
 * @details Пир, оставивший поток открытым после секции полей, тело допускает:
 *          последовательность фазовых событий не должна зависеть от того,
 *          прислал ли он пустой кадр DATA
 *
 */
TEST_F(ParserHttp3Fixture, EmptyBodyOpensBodyPhase){
	// Стороны соединения
	endpoint_t client, server;
	// Подготавливаем сторону клиента
	this->setup(client, direct_t::RESPONSE);
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Выполняем рукопожатие соединения
	this->handshake(client, server);
	// Формируем поля запроса с объявленной нулевой длиной тела
	std::vector <qpack::field_t> fields = this->request("POST", "/upload");
	// Дописываем объявление нулевой длины тела
	fields.emplace_back("content-length", "0");
	// Отправляем секцию полей, оставляя поток открытым
	client.parser->sendHeaders(0, fields, false);
	// Выполняем прокачку очередей исходящих данных
	this->pump(client, server);
	// Фаза приёма тела обязана открыться сразу за секцией полей
	ASSERT_EQ(this->phases(server.events, 0), "BEGIN:NONE END:HEADERS BEGIN:BODY");
	// Завершаем поток без единого октета тела
	client.parser->sendData(0, nullptr, 0, true);
	// Выполняем прокачку очередей исходящих данных
	this->pump(client, server);
	// Последовательность фазовых событий обязана совпасть с HTTP/1 и HTTP/2
	ASSERT_EQ(this->phases(server.events, 0), "BEGIN:NONE END:HEADERS BEGIN:BODY END:BODY END:NONE");
	// Ошибок уровня соединения быть не должно
	ASSERT_TRUE(server.events.errors.empty());
}

/**
 * @brief Проверка отсутствия фазы приёма тела у завершённого сообщения
 *
 * @details Секция полей с завершением потока тело исключает: фазы приёма тела
 *          в таком сообщении быть не должно
 *
 */
TEST_F(ParserHttp3Fixture, FinishedHeadersSkipBodyPhase){
	// Стороны соединения
	endpoint_t client, server;
	// Подготавливаем сторону клиента
	this->setup(client, direct_t::RESPONSE);
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Выполняем рукопожатие соединения
	this->handshake(client, server);
	// Отправляем секцию полей запроса вместе с завершением потока
	client.parser->sendHeaders(0, this->request("GET", "/index.html"), true);
	// Выполняем прокачку очередей исходящих данных
	this->pump(client, server);
	// Фаз приёма тела в завершённом сообщении быть не должно
	ASSERT_EQ(this->phases(server.events, 0), "BEGIN:NONE END:HEADERS END:NONE");
	// Ошибок уровня соединения быть не должно
	ASSERT_TRUE(server.events.errors.empty());
}

/**
 * @brief Проверка отсутствия фазы приёма тела у безтелесного ответа
 *
 * @details Ответы 204 и 304 тела не несут по определению: оставленный открытым
 *          поток этого не меняет, и фаза приёма тела открываться не должна
 *
 */
TEST_F(ParserHttp3Fixture, HeadlessResponseSkipsBodyPhase){
	// Стороны соединения
	endpoint_t client, server;
	// Подготавливаем сторону клиента
	this->setup(client, direct_t::RESPONSE);
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Выполняем рукопожатие соединения
	this->handshake(client, server);
	// Отправляем секцию полей запроса
	client.parser->sendHeaders(0, this->request("GET", "/index.html"), true);
	// Выполняем прокачку очередей исходящих данных
	this->pump(client, server);
	// Отправляем безтелесный ответ, оставляя поток открытым
	server.parser->sendHeaders(0, this->response("204"), false);
	// Выполняем прокачку очередей исходящих данных
	this->pump(client, server);
	// Фаза приёма тела у безтелесного ответа открываться не должна
	ASSERT_EQ(this->phases(client.events, 0), "BEGIN:NONE END:HEADERS");
	// Завершаем поток без тела
	server.parser->sendData(0, nullptr, 0, true);
	// Выполняем прокачку очередей исходящих данных
	this->pump(client, server);
	// Последовательность фазовых событий обязана обойтись без фаз тела
	ASSERT_EQ(this->phases(client.events, 0), "BEGIN:NONE END:HEADERS END:NONE");
	// Ошибок уровня соединения быть не должно
	ASSERT_TRUE(client.events.errors.empty());
}

/**
 * @brief Проверка отсутствия фазы приёма тела у ответа на запрос HEAD
 *
 * @details Ответ на HEAD содержимого не несёт (RFC 9110 §9.3.2) даже при
 *          объявленной длине тела: фаза приёма тела открываться не должна
 *
 */
TEST_F(ParserHttp3Fixture, HeadResponseSkipsBodyPhase){
	// Стороны соединения
	endpoint_t client, server;
	// Подготавливаем сторону клиента
	this->setup(client, direct_t::RESPONSE);
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Выполняем рукопожатие соединения
	this->handshake(client, server);
	// Отправляем секцию полей запроса методом HEAD
	client.parser->sendHeaders(0, this->request("HEAD", "/index.html"), true);
	// Выполняем прокачку очередей исходящих данных
	this->pump(client, server);
	// Формируем ответ с объявленной длиной тела, которого не будет
	std::vector <qpack::field_t> fields = this->response("200");
	// Дописываем объявленную длину тела
	fields.emplace_back("content-length", "100");
	// Отправляем ответ, оставляя поток открытым
	server.parser->sendHeaders(0, fields, false);
	// Выполняем прокачку очередей исходящих данных
	this->pump(client, server);
	// Фаза приёма тела у ответа на HEAD открываться не должна
	ASSERT_EQ(this->phases(client.events, 0), "BEGIN:NONE END:HEADERS");
	// Ошибок уровня соединения быть не должно
	ASSERT_TRUE(client.events.errors.empty());
}

/**
 * @brief Проверка разбора приоритета с параметрами structured fields
 *
 * @details Член словаря вправе нести параметры (RFC 8941 §3.1.2): неизвестные
 *          игнорируются, но само значение члена от этого не отменяется
 *
 */
TEST_F(ParserHttp3Fixture, PriorityParametersIgnored){
	// Сторона сервера
	endpoint_t server;
	// Подготавливаем сторону сервера
	this->setup(server, direct_t::REQUEST);
	// Отправляем параметры соединения со стороны сервера
	server.parser->sendSettings();
	// Собираем управляющий поток клиента с кадром параметров
	std::string control = ::unistream(static_cast <uint64_t> (unistream_t::CONTROL));
	// Дописываем кадр параметров соединения
	control.append(::settings());
	// Подаём управляющий поток клиента
	ASSERT_EQ(this->feed(server, 2, control), status_t::OK);
	// Открываем поток запроса подачей начала кадра секции полей
	ASSERT_EQ(this->feed(server, 0, std::string(1, '\x01')), status_t::OK);
	// Собираем кадр приоритета с параметрами у обоих членов словаря
	std::string update;
	// Дописываем кадр приоритета потока
	frame::serialize::priorityUpdate(update, false, 0, "u=1;q=0.5, i;x=1");
	// Подаём кадр приоритета
	ASSERT_EQ(this->feed(server, 2, update), status_t::OK);
	// Срочность обязана примениться, несмотря на параметр члена
	ASSERT_EQ(server.parser->priority(0).urgency, 1);
	// Признак инкрементальной доставки обязан примениться вместе со своим параметром
	ASSERT_TRUE(server.parser->priority(0).incremental);
	// Собираем кадр приоритета с пробелом перед разделителем параметров
	update.clear();
	// Дописываем кадр приоритета потока
	frame::serialize::priorityUpdate(update, false, 0, "u=6 ;q=0.1");
	// Подаём кадр приоритета
	ASSERT_EQ(this->feed(server, 2, update), status_t::OK);
	// Новая срочность обязана примениться
	ASSERT_EQ(server.parser->priority(0).urgency, 6);
	// Признак инкрементальной доставки обязан вернуться к значению по умолчанию
	ASSERT_FALSE(server.parser->priority(0).incremental);
	// Ошибок уровня соединения быть не должно
	ASSERT_TRUE(server.events.errors.empty());
}
