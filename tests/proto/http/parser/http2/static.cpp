/**
 * @file: static.cpp
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
 * Стандартные заголовочные файлы
 */
#include <string>
#include <memory>
#include <vector>
#include <utility>
#include <cstdint>
#include <cstring>
#include <functional>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "http2.hpp"

/**
 * Подписываемся на пространство имён HTTP-протокола
 */
using namespace awh::http;

/**
 * ==================== HPACK: целочисленное кодирование (RFC 7541 §5.1) ====================
 */

/**
 * @brief Метод проверки кодирования и декодирования целых с префиксом переменной длины
 *
 */
TEST(Http2Hpack, IntegerCodecTest){
	// Буфер закодированного целого
	std::string out;
	// Кодируем значение 10 с префиксом 5 бит (RFC 7541 C.1.1 - помещается в префикс)
	h2::hpack::encodeInteger(out, 10, 5, 0x00);
	// Проверяем что значение закодировано одним байтом
	ASSERT_EQ(out.size(), 1u);
	// Проверяем байт закодированного значения
	ASSERT_EQ(static_cast <uint8_t> (out[0]), 0x0A);
	// Очищаем буфер закодированного целого
	out.clear();
	// Кодируем значение 1337 с префиксом 5 бит (RFC 7541 C.1.2 - многобайтовое продолжение)
	h2::hpack::encodeInteger(out, 1337, 5, 0x00);
	// Проверяем что значение закодировано тремя байтами
	ASSERT_EQ(out.size(), 3u);
	// Проверяем первый байт (префикс заполнен целиком)
	ASSERT_EQ(static_cast <uint8_t> (out[0]), 0x1F);
	// Проверяем второй байт продолжения
	ASSERT_EQ(static_cast <uint8_t> (out[1]), 0x9A);
	// Проверяем третий байт продолжения
	ASSERT_EQ(static_cast <uint8_t> (out[2]), 0x0A);
	// Декодированное значение
	uint64_t value = 0;
	// Количество прочитанных байт
	size_t consumed = 0;
	// Декодируем закодированное значение обратно
	ASSERT_EQ(h2::hpack::decodeInteger(reinterpret_cast <const uint8_t *> (out.data()), out.size(), 5, value, consumed), h2::status_t::OK);
	// Проверяем декодированное значение
	ASSERT_EQ(value, 1337u);
	// Проверяем количество прочитанных байт
	ASSERT_EQ(consumed, 3u);
	// Декодируем неполный буфер (без последнего байта продолжения)
	ASSERT_EQ(h2::hpack::decodeInteger(reinterpret_cast <const uint8_t *> (out.data()), out.size() - 1, 5, value, consumed), h2::status_t::INCOMPLETE);
	// Формируем заведомо переполняющую последовательность продолжений
	const std::string overflow = "\x1F\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x7F";
	// Декодируем переполняющую последовательность - ожидаем ошибку
	ASSERT_EQ(h2::hpack::decodeInteger(reinterpret_cast <const uint8_t *> (overflow.data()), overflow.size(), 5, value, consumed), h2::status_t::ERROR);
}

/**
 * @brief Метод проверки Huffman-кодирования строк (RFC 7541 Appendix B)
 *
 */
TEST(Http2Hpack, HuffmanCodecTest){
	// Буфер закодированной строки
	std::string encoded;
	// Кодируем строку из примера RFC 7541 C.4.1
	h2::hpack::huffmanEncode("www.example.com", encoded);
	// Формируем эталон закодированной строки из RFC 7541 C.4.1
	const std::string expected = "\xF1\xE3\xC2\xE5\xF2\x3A\x6B\xA0\xAB\x90\xF4\xFF";
	// Проверяем что строка закодирована как в эталоне
	ASSERT_EQ(encoded, expected);
	// Проверяем что предвычисленная длина совпадает с фактической
	ASSERT_EQ(h2::hpack::huffmanLength("www.example.com"), encoded.size());
	// Буфер декодированной строки
	std::string decoded;
	// Декодируем закодированную строку обратно
	ASSERT_TRUE(h2::hpack::huffmanDecode(reinterpret_cast <const uint8_t *> (encoded.data()), encoded.size(), decoded));
	// Проверяем что строка декодирована без искажений
	ASSERT_EQ(decoded, "www.example.com");
	// Формируем строку со всеми значениями октетов
	std::string binary;
	/**
	 * Выполняем формирование строки со всеми значениями октетов
	 */
	for(uint16_t i = 0; i < 256; ++i)
		// Дописываем очередной октет
		binary.push_back(static_cast <char> (i));
	// Очищаем буфер закодированной строки
	encoded.clear();
	// Очищаем буфер декодированной строки
	decoded.clear();
	// Кодируем бинарную строку
	h2::hpack::huffmanEncode(binary, encoded);
	// Декодируем бинарную строку обратно
	ASSERT_TRUE(h2::hpack::huffmanDecode(reinterpret_cast <const uint8_t *> (encoded.data()), encoded.size(), decoded));
	// Проверяем что бинарная строка пережила кодирование без искажений
	ASSERT_EQ(decoded, binary);
}

/**
 * @brief Метод проверки статической таблицы HPACK (RFC 7541 Appendix A)
 *
 */
TEST(Http2Hpack, StaticTableTest){
	// Получаем первую запись статической таблицы
	const h2::hpack::static_entry_t * first = h2::hpack::staticTable(1);
	// Проверяем что запись существует
	ASSERT_TRUE(first != nullptr);
	// Проверяем название заголовка первой записи
	ASSERT_EQ(first->name, ":authority");
	// Получаем вторую запись статической таблицы
	const h2::hpack::static_entry_t * second = h2::hpack::staticTable(2);
	// Проверяем название заголовка второй записи
	ASSERT_EQ(second->name, ":method");
	// Проверяем значение заголовка второй записи
	ASSERT_EQ(second->value, "GET");
	// Получаем последнюю запись статической таблицы
	const h2::hpack::static_entry_t * last = h2::hpack::staticTable(h2::hpack::STATIC_TABLE_SIZE);
	// Проверяем название заголовка последней записи
	ASSERT_EQ(last->name, "www-authenticate");
	// Проверяем что нулевой индекс невалиден
	ASSERT_TRUE(h2::hpack::staticTable(0) == nullptr);
	// Проверяем что индекс за пределами таблицы невалиден
	ASSERT_TRUE(h2::hpack::staticTable(h2::hpack::STATIC_TABLE_SIZE + 1) == nullptr);
}

/**
 * @brief Метод проверки динамической таблицы HPACK с вытеснением по размеру (RFC 7541 §4)
 *
 */
TEST(Http2Hpack, DynamicTableTest){
	// Создаём динамическую таблицу
	h2::hpack::dynamic_table_t table(4096);
	// Добавляем запись в таблицу
	table.add("host", "anyks.com");
	// Проверяем количество записей таблицы
	ASSERT_EQ(table.count(), 1u);
	// Проверяем суммарный размер таблицы (len(name) + len(value) + 32, RFC 7541 §4.1)
	ASSERT_EQ(table.size(), static_cast <uint32_t> (4 + 9 + 32));
	// Добавляем вторую запись в таблицу
	table.add("user-agent", "awh");
	// Проверяем что свежая запись доступна под индексом 1
	ASSERT_EQ(table.at(1)->name, "user-agent");
	// Проверяем что старая запись сместилась на индекс 2
	ASSERT_EQ(table.at(2)->name, "host");
	// Проверяем что индекс за пределами таблицы невалиден
	ASSERT_TRUE(table.at(3) == nullptr);
	// Уменьшаем лимит размера таблицы до размера одной записи
	table.setMaxSize(static_cast <uint32_t> (10 + 3 + 32));
	// Проверяем что старая запись вытеснена
	ASSERT_EQ(table.count(), 1u);
	// Проверяем что осталась самая свежая запись
	ASSERT_EQ(table.at(1)->name, "user-agent");
	// Обнуляем лимит размера таблицы
	table.setMaxSize(0);
	// Проверяем что таблица полностью очищена
	ASSERT_EQ(table.count(), 0u);
}

/**
 * @brief Метод проверки сквозного кодирования и декодирования заголовков (Encoder -> Decoder)
 *
 */
TEST(Http2Hpack, EncoderDecoderRoundtripTest){
	// Создаём кодер HPACK
	h2::hpack::encoder_t encoder;
	// Создаём декодер HPACK
	h2::hpack::decoder_t decoder;
	// Формируем список кодируемых заголовков
	std::vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода (полное совпадение со статической таблицей)
	fields.emplace_back(":method", "GET");
	// Дописываем псевдо-заголовок пути
	fields.emplace_back(":path", "/index.html?q=awh");
	// Дописываем обычный заголовок (попадёт в динамическую таблицу)
	fields.emplace_back("user-agent", "awh/1.0");
	// Дописываем чувствительный заголовок (Literal Never Indexed, в таблицу не попадёт)
	fields.emplace_back("authorization", "Bearer secret-token");
	// Буфер первого закодированного блока
	std::string first;
	// Кодируем первый блок заголовков
	encoder.encode(fields, first, true);
	// Список декодированных заголовков
	std::vector <h2::hpack::field_t> decoded;
	// Код ошибки протокола
	h2::error_t err = h2::error_t::NO_ERROR;
	// Декодируем первый блок заголовков
	ASSERT_EQ(decoder.decode(first, decoded, 0, err), h2::status_t::OK);
	// Проверяем количество декодированных заголовков
	ASSERT_EQ(decoded.size(), fields.size());
	/**
	 * Выполняем сверку всех декодированных заголовков с оригиналом
	 */
	for(size_t i = 0; i < fields.size(); ++i){
		// Проверяем название заголовка
		ASSERT_EQ(decoded[i].name, fields[i].name);
		// Проверяем значение заголовка
		ASSERT_EQ(decoded[i].value, fields[i].value);
	}
	// Проверяем что чувствительный заголовок не попал в динамическую таблицу кодера
	ASSERT_EQ(encoder.table().count(), 2u);
	// Буфер второго закодированного блока
	std::string second;
	// Кодируем тот же список заголовков повторно (индексация через динамическую таблицу)
	encoder.encode(fields, second, true);
	// Проверяем что повторный блок закодирован компактнее первого
	ASSERT_LT(second.size(), first.size());
	// Очищаем список декодированных заголовков
	decoded.clear();
	// Декодируем второй блок заголовков (динамические таблицы синхронны)
	ASSERT_EQ(decoder.decode(second, decoded, 0, err), h2::status_t::OK);
	// Проверяем количество декодированных заголовков
	ASSERT_EQ(decoded.size(), fields.size());
	/**
	 * Выполняем сверку всех декодированных заголовков с оригиналом
	 */
	for(size_t i = 0; i < fields.size(); ++i){
		// Проверяем название заголовка (взято из динамической таблицы)
		ASSERT_EQ(decoded[i].name, fields[i].name);
		// Проверяем значение заголовка
		ASSERT_EQ(decoded[i].value, fields[i].value);
	}
}

/**
 * @brief Метод проверки пофиледного кодирования заголовков (zero-copy API кодера)
 *
 */
TEST(Http2Hpack, EncoderPerFieldTest){
	// Создаём кодер HPACK для пофиледного кодирования
	h2::hpack::encoder_t fieldEncoder;
	// Создаём кодер HPACK для кодирования списком
	h2::hpack::encoder_t listEncoder;
	// Буфер блока пофиледного кодирования
	std::string fieldBlock;
	// Начинаем кодирование блока заголовков
	fieldEncoder.begin(fieldBlock);
	// Кодируем заголовки по одному
	fieldEncoder.encode(":method", "POST", fieldBlock);
	// Кодируем очередной заголовок
	fieldEncoder.encode(":path", "/upload", fieldBlock);
	// Кодируем очередной заголовок
	fieldEncoder.encode("content-type", "text/plain", fieldBlock);
	// Кодируем чувствительный заголовок явно
	fieldEncoder.encode("x-token", "secret", fieldBlock, true);
	// Формируем тот же список заголовков
	std::vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода
	fields.emplace_back(":method", "POST");
	// Дописываем псевдо-заголовок пути
	fields.emplace_back(":path", "/upload");
	// Дописываем обычный заголовок
	fields.emplace_back("content-type", "text/plain");
	// Дописываем чувствительный заголовок
	fields.emplace_back("x-token", "secret", true);
	// Буфер блока кодирования списком
	std::string listBlock;
	// Кодируем список заголовков целиком
	listEncoder.encode(fields, listBlock, true);
	// Проверяем что оба способа кодирования дают идентичный блок
	ASSERT_EQ(fieldBlock, listBlock);
	// Проверяем что чувствительный заголовок не попал в динамическую таблицу
	ASSERT_EQ(fieldEncoder.table().count(), 2u);
}

/**
 * @brief Метод проверки защиты декодера от decompression bomb (лимит размера списка)
 *
 */
TEST(Http2Hpack, DecoderListSizeLimitTest){
	// Создаём кодер HPACK
	h2::hpack::encoder_t encoder;
	// Создаём декодер HPACK
	h2::hpack::decoder_t decoder;
	// Формируем список заголовков с большим суммарным размером
	std::vector <h2::hpack::field_t> fields;
	// Дописываем заголовок с длинным значением
	fields.emplace_back("x-large", std::string(1024, 'a'));
	// Буфер закодированного блока
	std::string block;
	// Кодируем блок заголовков
	encoder.encode(fields, block, true);
	// Список декодированных заголовков
	std::vector <h2::hpack::field_t> decoded;
	// Код ошибки протокола
	h2::error_t err = h2::error_t::NO_ERROR;
	// Декодируем блок с лимитом меньше распакованного размера - ожидаем ошибку
	ASSERT_EQ(decoder.decode(block, decoded, 100, err), h2::status_t::ERROR);
	// Проверяем что зафиксирован код ошибки чрезмерного поведения
	ASSERT_EQ(err, h2::error_t::ENHANCE_YOUR_CALM);
}

/**
 * ==================== Framing: разбор и сборка фреймов (RFC 9113 §4-6) ====================
 */

/**
 * @brief Метод проверки сборки и разбора фрейма SETTINGS
 *
 */
TEST(Http2Frame, SettingsRoundtripTest){
	// Формируем список параметров SETTINGS
	h2::frame::setting_entry_t items[2];
	// Устанавливаем параметр размера динамической таблицы HPACK
	items[0].id = h2::setting_t::HEADER_TABLE_SIZE;
	// Устанавливаем значение параметра
	items[0].value = 8192;
	// Устанавливаем параметр максимального размера фрейма
	items[1].id = h2::setting_t::MAX_FRAME_SIZE;
	// Устанавливаем значение параметра
	items[1].value = 32768;
	// Буфер собранного фрейма
	std::string out;
	// Собираем фрейм SETTINGS
	h2::frame::serializeSettings(out, items, 2, false);
	// Разобранный заголовок фрейма
	h2::frame::header_t header;
	// Разбираем заголовок фрейма
	ASSERT_TRUE(h2::frame::parseHeader(reinterpret_cast <const uint8_t *> (out.data()), out.size(), header));
	// Проверяем тип фрейма
	ASSERT_EQ(header.type, h2::frame_t::SETTINGS);
	// Проверяем длину полезной нагрузки (2 параметра по 6 байт)
	ASSERT_EQ(header.length, 12u);
	// Проверяем идентификатор потока (SETTINGS - фрейм соединения)
	ASSERT_EQ(header.streamId, 0u);
	// Список разобранных параметров
	std::vector <h2::frame::setting_entry_t> parsed;
	// Код ошибки протокола
	h2::error_t err = h2::error_t::NO_ERROR;
	// Разбираем полезную нагрузку SETTINGS
	ASSERT_EQ(h2::frame::parseSettings(header, reinterpret_cast <const uint8_t *> (out.data()) + h2::proto::FRAME_HEADER_SIZE, parsed, err), h2::status_t::OK);
	// Проверяем количество разобранных параметров
	ASSERT_EQ(parsed.size(), 2u);
	// Проверяем идентификатор первого параметра
	ASSERT_EQ(parsed[0].id, h2::setting_t::HEADER_TABLE_SIZE);
	// Проверяем значение первого параметра
	ASSERT_EQ(parsed[0].value, 8192u);
	// Проверяем значение второго параметра
	ASSERT_EQ(parsed[1].value, 32768u);
}

/**
 * @brief Метод проверки сборки и разбора фрейма DATA (включая padding)
 *
 */
TEST(Http2Frame, DataRoundtripTest){
	// Буфер собранного фрейма
	std::string out;
	// Собираем фрейм DATA с флагом END_STREAM
	h2::frame::serializeData(out, 1, "Hello, HTTP/2!", true);
	// Разобранный заголовок фрейма
	h2::frame::header_t header;
	// Разбираем заголовок фрейма
	ASSERT_TRUE(h2::frame::parseHeader(reinterpret_cast <const uint8_t *> (out.data()), out.size(), header));
	// Проверяем тип фрейма
	ASSERT_EQ(header.type, h2::frame_t::DATA);
	// Проверяем идентификатор потока
	ASSERT_EQ(header.streamId, 1u);
	// Разобранная полезная нагрузка DATA
	h2::frame::data_t data;
	// Код ошибки протокола
	h2::error_t err = h2::error_t::NO_ERROR;
	// Разбираем полезную нагрузку DATA
	ASSERT_EQ(h2::frame::parseData(header, reinterpret_cast <const uint8_t *> (out.data()) + h2::proto::FRAME_HEADER_SIZE, data, err), h2::status_t::OK);
	// Проверяем данные тела
	ASSERT_EQ(data.data, "Hello, HTTP/2!");
	// Проверяем флаг завершения потока
	ASSERT_TRUE(data.endStream);
	// Формируем заголовок padded-фрейма вручную (Pad Length = 2)
	h2::frame::header_t padded;
	// Устанавливаем длину полезной нагрузки (1 байт Pad Length + 2 байта данных + 2 байта padding)
	padded.length = 5;
	// Устанавливаем тип фрейма
	padded.type = h2::frame_t::DATA;
	// Устанавливаем флаг наличия padding
	padded.flags = h2::flag::PADDED;
	// Устанавливаем идентификатор потока
	padded.streamId = 1;
	// Формируем полезную нагрузку padded-фрейма
	const uint8_t payload[5] = {0x02, 'h', 'i', 0x00, 0x00};
	// Разбираем полезную нагрузку padded-фрейма
	ASSERT_EQ(h2::frame::parseData(padded, payload, data, err), h2::status_t::OK);
	// Проверяем что padding снят и данные извлечены корректно
	ASSERT_EQ(data.data, "hi");
	// Формируем некорректный padding (Pad Length >= длины нагрузки)
	const uint8_t broken[2] = {0x05, 'x'};
	// Корректируем длину полезной нагрузки
	padded.length = 2;
	// Разбираем некорректную полезную нагрузку - ожидаем ошибку
	ASSERT_EQ(h2::frame::parseData(padded, broken, data, err), h2::status_t::ERROR);
	// Проверяем код ошибки протокола
	ASSERT_EQ(err, h2::error_t::PROTOCOL_ERROR);
}

/**
 * @brief Метод проверки нарезки большого блока заголовков на HEADERS + CONTINUATION
 *
 */
TEST(Http2Frame, HeaderBlockSplitTest){
	// Формируем большой блок заголовков (40000 байт при лимите фрейма 16384)
	const std::string block(40000, 'h');
	// Буфер собранных фреймов
	std::string out;
	// Собираем блок заголовков с нарезкой на HEADERS + CONTINUATION
	h2::frame::serializeHeaderBlock(out, 1, block, true, h2::proto::DEFAULT_MAX_FRAME_SIZE);
	// Буфер собранного обратно блока заголовков
	std::string reassembled;
	// Позиция разбора в буфере фреймов
	size_t pos = 0;
	// Счётчик разобранных фреймов
	size_t frames = 0;
	// Код ошибки протокола
	h2::error_t err = h2::error_t::NO_ERROR;
	/**
	 * Выполняем разбор всех собранных фреймов
	 */
	while(pos < out.size()){
		// Разобранный заголовок фрейма
		h2::frame::header_t header;
		// Разбираем заголовок очередного фрейма
		ASSERT_TRUE(h2::frame::parseHeader(reinterpret_cast <const uint8_t *> (out.data()) + pos, out.size() - pos, header));
		// Смещаемся на полезную нагрузку фрейма
		pos += h2::proto::FRAME_HEADER_SIZE;
		// Проверяем что размер полезной нагрузки не превышает лимит фрейма
		ASSERT_LE(header.length, h2::proto::DEFAULT_MAX_FRAME_SIZE);
		// Если разобран первый фрейм
		if(frames == 0){
			// Проверяем что первый фрейм имеет тип HEADERS
			ASSERT_EQ(header.type, h2::frame_t::HEADERS);
			// Разобранная полезная нагрузка HEADERS
			h2::frame::headers_t headers;
			// Разбираем полезную нагрузку HEADERS
			ASSERT_EQ(h2::frame::parseHeaders(header, reinterpret_cast <const uint8_t *> (out.data()) + pos, headers, err), h2::status_t::OK);
			// Проверяем что END_STREAM установлен на первом фрейме
			ASSERT_TRUE(headers.endStream);
			// Проверяем что блок продолжается в CONTINUATION
			ASSERT_FALSE(headers.endHeaders);
			// Собираем фрагмент блока заголовков
			reassembled.append(headers.block);
		// Если разобран последующий фрейм
		} else {
			// Проверяем что последующие фреймы имеют тип CONTINUATION
			ASSERT_EQ(header.type, h2::frame_t::CONTINUATION);
			// Фрагмент блока заголовков
			std::string_view fragment;
			// Флаг завершения блока заголовков
			bool endHeaders = false;
			// Разбираем полезную нагрузку CONTINUATION
			ASSERT_EQ(h2::frame::parseContinuation(header, reinterpret_cast <const uint8_t *> (out.data()) + pos, fragment, endHeaders, err), h2::status_t::OK);
			// Собираем фрагмент блока заголовков
			reassembled.append(fragment);
			// Если это последний фрейм буфера - на нём обязан стоять END_HEADERS
			if((pos + header.length) >= out.size())
				// Проверяем флаг завершения блока заголовков
				ASSERT_TRUE(endHeaders);
		}
		// Смещаемся на следующий фрейм
		pos += header.length;
		// Считаем разобранные фреймы
		frames++;
	}
	// Проверяем что блок нарезан на три фрейма (16384 + 16384 + 7232)
	ASSERT_EQ(frames, 3u);
	// Проверяем что блок собран обратно без искажений
	ASSERT_EQ(reassembled, block);
}

/**
 * @brief Метод проверки сборки и разбора управляющих фреймов (PING/GOAWAY/WINDOW_UPDATE/RST_STREAM)
 *
 */
TEST(Http2Frame, ControlFramesRoundtripTest){
	// Код ошибки протокола
	h2::error_t err = h2::error_t::NO_ERROR;
	// Разобранный заголовок фрейма
	h2::frame::header_t header;
	{
		// Формируем opaque-данные PING
		const uint8_t opaque[8] = {1, 2, 3, 4, 5, 6, 7, 8};
		// Буфер собранного фрейма
		std::string out;
		// Собираем фрейм PING
		h2::frame::serializePing(out, opaque, false);
		// Разбираем заголовок фрейма
		ASSERT_TRUE(h2::frame::parseHeader(reinterpret_cast <const uint8_t *> (out.data()), out.size(), header));
		// Проверяем тип фрейма
		ASSERT_EQ(header.type, h2::frame_t::PING);
		// Извлечённые opaque-данные
		uint8_t parsed[8] = {0};
		// Разбираем полезную нагрузку PING
		ASSERT_EQ(h2::frame::parsePing(header, reinterpret_cast <const uint8_t *> (out.data()) + h2::proto::FRAME_HEADER_SIZE, parsed, err), h2::status_t::OK);
		// Проверяем что opaque-данные извлечены без искажений
		ASSERT_EQ(::memcmp(parsed, opaque, 8), 0);
	}
	{
		// Буфер собранного фрейма
		std::string out;
		// Собираем фрейм GOAWAY с отладочными данными
		h2::frame::serializeGoaway(out, 5, h2::error_t::ENHANCE_YOUR_CALM, "too fast");
		// Разбираем заголовок фрейма
		ASSERT_TRUE(h2::frame::parseHeader(reinterpret_cast <const uint8_t *> (out.data()), out.size(), header));
		// Разобранная полезная нагрузка GOAWAY
		h2::frame::goaway_t goaway;
		// Разбираем полезную нагрузку GOAWAY
		ASSERT_EQ(h2::frame::parseGoaway(header, reinterpret_cast <const uint8_t *> (out.data()) + h2::proto::FRAME_HEADER_SIZE, goaway, err), h2::status_t::OK);
		// Проверяем наибольший идентификатор обработанного потока
		ASSERT_EQ(goaway.lastStreamId, 5u);
		// Проверяем код ошибки завершения соединения
		ASSERT_EQ(goaway.code, h2::error_t::ENHANCE_YOUR_CALM);
		// Проверяем отладочные данные
		ASSERT_EQ(goaway.debugData, "too fast");
	}
	{
		// Буфер собранного фрейма
		std::string out;
		// Собираем фрейм WINDOW_UPDATE
		h2::frame::serializeWindowUpdate(out, 3, 65535);
		// Разбираем заголовок фрейма
		ASSERT_TRUE(h2::frame::parseHeader(reinterpret_cast <const uint8_t *> (out.data()), out.size(), header));
		// Извлечённый инкремент окна
		uint32_t increment = 0;
		// Разбираем полезную нагрузку WINDOW_UPDATE
		ASSERT_EQ(h2::frame::parseWindowUpdate(header, reinterpret_cast <const uint8_t *> (out.data()) + h2::proto::FRAME_HEADER_SIZE, increment, err), h2::status_t::OK);
		// Проверяем инкремент окна
		ASSERT_EQ(increment, 65535u);
	}
	{
		// Буфер собранного фрейма
		std::string out;
		// Собираем фрейм RST_STREAM
		h2::frame::serializeRstStream(out, 7, h2::error_t::CANCEL);
		// Разбираем заголовок фрейма
		ASSERT_TRUE(h2::frame::parseHeader(reinterpret_cast <const uint8_t *> (out.data()), out.size(), header));
		// Извлечённый код ошибки сброса потока
		h2::error_t code = h2::error_t::NO_ERROR;
		// Разбираем полезную нагрузку RST_STREAM
		ASSERT_EQ(h2::frame::parseRstStream(header, reinterpret_cast <const uint8_t *> (out.data()) + h2::proto::FRAME_HEADER_SIZE, code, err), h2::status_t::OK);
		// Проверяем код ошибки сброса потока
		ASSERT_EQ(code, h2::error_t::CANCEL);
	}
}

/**
 * ==================== Session: полный жизненный цикл соединения ====================
 */

/**
 * @brief Метод проверки рукопожатия соединения (preface + обмен SETTINGS)
 *
 */
TEST_F(ParserHttp2Fixture, HandshakeTest){
	// Создаём объект парсера сервера (разбирает запросы клиента)
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект парсера клиента (разбирает ответы сервера)
	auto client = this->make(direct_t::RESPONSE);
	// Создаём объекты сборщиков событий парсеров
	events_t serverEvents, clientEvents;
	// Подписываем сборщики событий на все функции обратного вызова парсеров
	this->attach(* server, serverEvents);
	// Подписываем сборщик событий клиента
	this->attach(* client, clientEvents);
	// Настраиваем нестандартные параметры SETTINGS клиента
	parser_http2_t::settings_t settings;
	// Устанавливаем нестандартный размер динамической таблицы HPACK
	settings.headerTableSize = 8192;
	// Устанавливаем нестандартный лимит одновременных потоков
	settings.maxConcurrentStreams = 42;
	// Применяем параметры SETTINGS клиента
	client->settings(settings);
	// Соединяем парсеры каналами записи
	this->connect(* client, * server);
	// Выполняем рукопожатие соединения
	this->handshake(* client, * server);
	// Проверяем что сервер применил SETTINGS клиента
	ASSERT_EQ(serverEvents.settingsApplied, 1u);
	// Проверяем что клиент применил SETTINGS сервера
	ASSERT_EQ(clientEvents.settingsApplied, 1u);
	// Проверяем что сервер видит параметры SETTINGS клиента
	ASSERT_EQ(server->remoteSettings().headerTableSize, 8192u);
	// Проверяем что сервер видит лимит одновременных потоков клиента
	ASSERT_EQ(server->remoteSettings().maxConcurrentStreams, 42u);
	// Проверяем что соединение сервера живо
	ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL);
	// Проверяем что соединение клиента живо
	ASSERT_EQ(client->status(), parser_t::status_t::PARTIAL);
	// Проверяем что ошибок уровня соединения нет
	ASSERT_EQ(server->error(), parser_http2_t::error_t::NO_ERROR);
	// Проверяем что соединение не помечено на завершение
	ASSERT_FALSE(server->isClosed());
}

/**
 * @brief Метод проверки полного обмена запросом и ответом с телами
 *
 */
TEST_F(ParserHttp2Fixture, SimpleExchangeTest){
	// Создаём объект парсера сервера
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект парсера клиента
	auto client = this->make(direct_t::RESPONSE);
	// Создаём объекты сборщиков событий парсеров
	events_t serverEvents, clientEvents;
	// Подписываем сборщики событий на все функции обратного вызова парсеров
	this->attach(* server, serverEvents);
	// Подписываем сборщик событий клиента
	this->attach(* client, clientEvents);
	// Соединяем парсеры каналами записи
	this->connect(* client, * server);
	// Выполняем рукопожатие соединения
	this->handshake(* client, * server);
	// Выделяем идентификатор нового потока клиента
	const uint32_t sid = client->nextStreamId();
	// Проверяем что клиент получил нечётный идентификатор потока
	ASSERT_EQ(sid, 1u);
	// Формируем заголовки POST-запроса
	std::vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода запроса
	fields.emplace_back(":method", "POST");
	// Дописываем псевдо-заголовок схемы запроса
	fields.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути запроса
	fields.emplace_back(":path", "/api/upload");
	// Дописываем псевдо-заголовок авторитета запроса
	fields.emplace_back(":authority", "anyks.com");
	// Дописываем обычный заголовок
	fields.emplace_back("content-type", "application/json");
	// Отправляем заголовки запроса (тело последует отдельно)
	client->sendHeaders(sid, fields, false);
	// Отправляем тело запроса с завершением потока
	client->sendData(sid, "{\"id\":7}", 8, true);
	// Проверяем что сервер увидел открытие потока
	ASSERT_EQ(serverEvents.begins.size(), 1u);
	// Проверяем идентификатор открытого потока
	ASSERT_EQ(serverEvents.begins.front(), sid);
	// Проверяем что сервер получил все заголовки запроса
	ASSERT_EQ(serverEvents.headers.size(), 5u);
	// Проверяем что провайдер заголовков собран
	ASSERT_EQ(serverEvents.providers.size(), 1u);
	// Проверяем метод запроса из провайдера
	ASSERT_EQ(serverEvents.method, method_t::POST);
	// Проверяем параметры URI-запроса из провайдера
	ASSERT_EQ(serverEvents.uri, "/api/upload");
	// Проверяем что сервер получил тело запроса
	ASSERT_EQ(serverEvents.bodies[sid], "{\"id\":7}");
	// Формируем заголовки ответа сервера
	std::vector <h2::hpack::field_t> response;
	// Дописываем псевдо-заголовок статуса ответа
	response.emplace_back(":status", "200");
	// Дописываем обычный заголовок
	response.emplace_back("content-type", "application/json");
	// Отправляем заголовки ответа (тело последует отдельно)
	server->sendHeaders(sid, response, false);
	// Отправляем тело ответа с завершением потока
	server->sendData(sid, "{\"ok\":true}", 11, true);
	// Проверяем что клиент получил статус-код ответа
	ASSERT_EQ(clientEvents.code, 200u);
	// Проверяем что клиент получил тело ответа
	ASSERT_EQ(clientEvents.bodies[sid], "{\"ok\":true}");
	// Проверяем что поток клиента закрыт штатно
	ASSERT_EQ(clientEvents.closes.size(), 1u);
	// Проверяем код закрытия потока клиента
	ASSERT_EQ(clientEvents.closes.front().second, parser_http2_t::error_t::NO_ERROR);
	// Проверяем что поток сервера закрыт штатно
	ASSERT_EQ(serverEvents.closes.size(), 1u);
	// Проверяем что соединения живы (готовы к следующим потокам)
	ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL);
	// Проверяем статус клиента
	ASSERT_EQ(client->status(), parser_t::status_t::PARTIAL);
}

/**
 * @brief Метод проверки отправки заголовков из контейнера headers_t (zero-copy)
 *
 */
TEST_F(ParserHttp2Fixture, ContainerHeadersTest){
	// Создаём объект парсера сервера
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект парсера клиента
	auto client = this->make(direct_t::RESPONSE);
	// Создаём объекты сборщиков событий парсеров
	events_t serverEvents, clientEvents;
	// Подписываем сборщики событий на все функции обратного вызова парсеров
	this->attach(* server, serverEvents);
	// Подписываем сборщик событий клиента
	this->attach(* client, clientEvents);
	// Соединяем парсеры каналами записи
	this->connect(* client, * server);
	// Выполняем рукопожатие соединения
	this->handshake(* client, * server);
	// Выделяем идентификатор нового потока клиента
	const uint32_t sid = client->nextStreamId();
	// Формируем контейнер заголовков запроса с провайдером
	headers_t request(std::make_unique <request_t> (version_t::HTTP2, method_t::GET, std::string("/index.html")));
	// Дописываем заголовок Host (конвертируется в псевдо-заголовок [:authority])
	request.emplace("Host", "anyks.com");
	// Дописываем заголовок в смешанном регистре (приводится к нижнему)
	request.emplace("User-Agent", "awh");
	// Дописываем запрещённый в HTTP/2 connection-specific заголовок (выбрасывается)
	request.emplace("Connection", "keep-alive");
	// Дописываем заголовок TE с недопустимым значением (выбрасывается)
	request.emplace("TE", "gzip");
	// Отправляем заголовки запроса из контейнера с завершением потока
	client->sendHeaders(sid, request, true);
	// Проверяем что сервер получил ровно пять заголовков (Host конвертирован, запрещённые выброшены)
	ASSERT_EQ(serverEvents.headers.size(), 5u);
	// Проверяем псевдо-заголовок метода запроса
	ASSERT_EQ(std::get <1> (serverEvents.headers[0]), ":method");
	// Проверяем значение псевдо-заголовка метода
	ASSERT_EQ(std::get <2> (serverEvents.headers[0]), "GET");
	// Проверяем псевдо-заголовок схемы запроса
	ASSERT_EQ(std::get <1> (serverEvents.headers[1]), ":scheme");
	// Проверяем значение схемы по умолчанию
	ASSERT_EQ(std::get <2> (serverEvents.headers[1]), "https");
	// Проверяем псевдо-заголовок авторитета запроса (конвертирован из Host)
	ASSERT_EQ(std::get <1> (serverEvents.headers[2]), ":authority");
	// Проверяем значение авторитета запроса
	ASSERT_EQ(std::get <2> (serverEvents.headers[2]), "anyks.com");
	// Проверяем псевдо-заголовок пути запроса
	ASSERT_EQ(std::get <1> (serverEvents.headers[3]), ":path");
	// Проверяем значение пути запроса
	ASSERT_EQ(std::get <2> (serverEvents.headers[3]), "/index.html");
	// Проверяем что название обычного заголовка приведено к нижнему регистру
	ASSERT_EQ(std::get <1> (serverEvents.headers[4]), "user-agent");
	// Проверяем метод запроса из провайдера
	ASSERT_EQ(serverEvents.method, method_t::GET);
	// Проверяем параметры URI-запроса из провайдера
	ASSERT_EQ(serverEvents.uri, "/index.html");
	// Формируем контейнер заголовков ответа с провайдером
	headers_t response(std::make_unique <response_t> (version_t::HTTP2, static_cast <uint16_t> (404)));
	// Дописываем обычный заголовок ответа
	response.emplace("Content-Type", "text/html");
	// Отправляем заголовки ответа из контейнера с завершением потока
	server->sendHeaders(sid, response, true);
	// Проверяем что клиент получил статус-код ответа
	ASSERT_EQ(clientEvents.code, 404u);
	// Проверяем что клиент получил оба заголовка ответа
	ASSERT_EQ(clientEvents.headers.size(), 2u);
	// Проверяем псевдо-заголовок статуса ответа
	ASSERT_EQ(std::get <1> (clientEvents.headers[0]), ":status");
	// Проверяем значение статуса ответа
	ASSERT_EQ(std::get <2> (clientEvents.headers[0]), "404");
}

/**
 * @brief Метод проверки запроса методом CONNECT (RFC 9113 §8.5)
 *
 */
TEST_F(ParserHttp2Fixture, ConnectRequestTest){
	// Создаём объект парсера сервера
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект парсера клиента
	auto client = this->make(direct_t::RESPONSE);
	// Создаём объекты сборщиков событий парсеров
	events_t serverEvents, clientEvents;
	// Подписываем сборщики событий на все функции обратного вызова парсеров
	this->attach(* server, serverEvents);
	// Подписываем сборщик событий клиента
	this->attach(* client, clientEvents);
	// Соединяем парсеры каналами записи
	this->connect(* client, * server);
	// Выполняем рукопожатие соединения
	this->handshake(* client, * server);
	// Выделяем идентификатор нового потока клиента
	const uint32_t sid = client->nextStreamId();
	// Формируем контейнер заголовков CONNECT-запроса
	headers_t request(std::make_unique <request_t> (version_t::HTTP2, method_t::CONNECT));
	// Дописываем заголовок Host (конвертируется в псевдо-заголовок [:authority])
	request.emplace("Host", "anyks.com:443");
	// Отправляем заголовки CONNECT-запроса (туннель остаётся открытым)
	client->sendHeaders(sid, request, false);
	// Проверяем что сервер получил ровно два заголовка ([:scheme]/[:path] запрещены для CONNECT)
	ASSERT_EQ(serverEvents.headers.size(), 2u);
	// Проверяем псевдо-заголовок метода запроса
	ASSERT_EQ(std::get <2> (serverEvents.headers[0]), "CONNECT");
	// Проверяем псевдо-заголовок авторитета запроса
	ASSERT_EQ(std::get <1> (serverEvents.headers[1]), ":authority");
	// Проверяем значение авторитета запроса
	ASSERT_EQ(std::get <2> (serverEvents.headers[1]), "anyks.com:443");
	// Проверяем метод запроса из провайдера
	ASSERT_EQ(serverEvents.method, method_t::CONNECT);
	// Проверяем что соединение сервера живо (CONNECT прошёл валидацию)
	ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL);
}

/**
 * @brief Метод проверки отправки трейлеров (контейнер без провайдера)
 *
 */
TEST_F(ParserHttp2Fixture, TrailersTest){
	// Создаём объект парсера сервера
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект парсера клиента
	auto client = this->make(direct_t::RESPONSE);
	// Создаём объекты сборщиков событий парсеров
	events_t serverEvents, clientEvents;
	// Подписываем сборщики событий на все функции обратного вызова парсеров
	this->attach(* server, serverEvents);
	// Подписываем сборщик событий клиента
	this->attach(* client, clientEvents);
	// Соединяем парсеры каналами записи
	this->connect(* client, * server);
	// Выполняем рукопожатие соединения
	this->handshake(* client, * server);
	// Выделяем идентификатор нового потока клиента
	const uint32_t sid = client->nextStreamId();
	// Формируем контейнер заголовков запроса с провайдером
	headers_t request(std::make_unique <request_t> (version_t::HTTP2, method_t::POST, std::string("/upload")));
	// Дописываем заголовок Host
	request.emplace("Host", "anyks.com");
	// Отправляем заголовки запроса (тело и трейлеры последуют)
	client->sendHeaders(sid, request, false);
	// Отправляем тело запроса (поток остаётся открытым для трейлеров)
	client->sendData(sid, "hello", 5, false);
	// Формируем контейнер трейлеров (без провайдера - псевдо-заголовки не формируются)
	headers_t trailers;
	// Дописываем трейлер контрольной суммы
	trailers.emplace("X-Checksum", "5d41402a");
	// Отправляем трейлеры с завершением потока
	client->sendHeaders(sid, trailers, true);
	// Проверяем что сервер получил тело запроса
	ASSERT_EQ(serverEvents.bodies[sid], "hello");
	// Ищем полученный сервером трейлер
	bool trailerFound = false;
	/**
	 * Выполняем перебор всех собранных заголовков сервера
	 */
	for(const auto & header : serverEvents.headers){
		// Если найден трейлер контрольной суммы
		if(std::get <3> (header) == parser_t::part_t::TRAILER){
			// Помечаем что трейлер найден
			trailerFound = true;
			// Проверяем название трейлера (приведено к нижнему регистру)
			ASSERT_EQ(std::get <1> (header), "x-checksum");
			// Проверяем значение трейлера
			ASSERT_EQ(std::get <2> (header), "5d41402a");
		}
	}
	// Проверяем что трейлер получен
	ASSERT_TRUE(trailerFound);
	// Проверяем что провайдер трейлеров передан как nullptr
	ASSERT_EQ(serverEvents.providers.size(), 2u);
	// Проверяем флаг трейлеров второго события провайдера
	ASSERT_TRUE(std::get <1> (serverEvents.providers[1]));
	// Формируем заголовки ответа сервера (для полного закрытия потока)
	std::vector <h2::hpack::field_t> response;
	// Дописываем псевдо-заголовок статуса ответа
	response.emplace_back(":status", "200");
	// Отправляем заголовки ответа с завершением потока
	server->sendHeaders(sid, response, true);
	// Проверяем что поток сервера закрыт штатно (обе половины завершены)
	ASSERT_EQ(serverEvents.closes.size(), 1u);
	// Проверяем код закрытия потока сервера
	ASSERT_EQ(serverEvents.closes.front().second, parser_http2_t::error_t::NO_ERROR);
}

/**
 * @brief Метод проверки flow control при отправке тела больше окна и буфера отправки
 *
 */
TEST_F(ParserHttp2Fixture, LargeBodyFlowControlTest){
	// Создаём объект парсера сервера
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект парсера клиента
	auto client = this->make(direct_t::RESPONSE);
	// Создаём объекты сборщиков событий парсеров
	events_t serverEvents, clientEvents;
	// Подписываем сборщики событий на все функции обратного вызова парсеров
	this->attach(* server, serverEvents);
	// Подписываем сборщик событий клиента
	this->attach(* client, clientEvents);
	// Соединяем парсеры каналами записи
	this->connect(* client, * server);
	// Выполняем рукопожатие соединения
	this->handshake(* client, * server);
	// Формируем тело больше окна flow control (65535) и буфера отправки (256 КиБ)
	std::string body(300000, '\0');
	/**
	 * Выполняем заполнение тела псевдослучайным паттерном
	 */
	for(size_t i = 0; i < body.size(); ++i)
		// Заполняем очередной байт тела
		body[i] = static_cast <char> ((i * 31 + 7) & 0xFF);
	// Выделяем идентификатор нового потока клиента
	const uint32_t sid = client->nextStreamId();
	// Формируем заголовки запроса
	std::vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода запроса
	fields.emplace_back(":method", "POST");
	// Дописываем псевдо-заголовок схемы запроса
	fields.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути запроса
	fields.emplace_back(":path", "/big");
	// Отправляем заголовки запроса
	client->sendHeaders(sid, fields, false);
	// Смещение отправки тела
	size_t offset = 0;
	// Ограничитель количества попыток отправки (защита теста от зависания)
	size_t attempts = 0;
	/**
	 * Отправляем тело порциями: sendData принимает столько, сколько влезает в буфер
	 * до high-water, а прокачка отправки и WINDOW_UPDATE выполняются автоматически
	 */
	while((offset < body.size()) && (attempts++ < 100))
		// Отправляем очередную порцию тела с завершением потока на последнем фрагменте
		offset += client->sendData(sid, body.data() + offset, body.size() - offset, true);
	// Проверяем что всё тело принято парсером
	ASSERT_EQ(offset, body.size());
	// Проверяем что сервер получил всё тело без искажений
	ASSERT_EQ(serverEvents.bodies[sid], body);
	// Проверяем что сигнал готовности потока принимать данные подавался
	ASSERT_FALSE(clientEvents.writables.empty());
	// Формируем заголовки ответа сервера (для полного закрытия потока)
	std::vector <h2::hpack::field_t> response;
	// Дописываем псевдо-заголовок статуса ответа
	response.emplace_back(":status", "200");
	// Отправляем заголовки ответа с завершением потока
	server->sendHeaders(sid, response, true);
	// Проверяем что поток сервера закрыт штатно (обе половины завершены)
	ASSERT_EQ(serverEvents.closes.size(), 1u);
	// Проверяем код закрытия потока сервера
	ASSERT_EQ(serverEvents.closes.front().second, parser_http2_t::error_t::NO_ERROR);
}

/**
 * @brief Метод проверки pull-источника данных тела потока
 *
 */
TEST_F(ParserHttp2Fixture, DataSourceTest){
	// Создаём объект парсера сервера
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект парсера клиента
	auto client = this->make(direct_t::RESPONSE);
	// Создаём объекты сборщиков событий парсеров
	events_t serverEvents, clientEvents;
	// Подписываем сборщики событий на все функции обратного вызова парсеров
	this->attach(* server, serverEvents);
	// Подписываем сборщик событий клиента
	this->attach(* client, clientEvents);
	// Соединяем парсеры каналами записи
	this->connect(* client, * server);
	// Выполняем рукопожатие соединения
	this->handshake(* client, * server);
	// Выделяем идентификатор нового потока клиента
	const uint32_t sid = client->nextStreamId();
	// Формируем заголовки запроса
	std::vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода запроса
	fields.emplace_back(":method", "GET");
	// Дописываем псевдо-заголовок схемы запроса
	fields.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути запроса
	fields.emplace_back(":path", "/download");
	// Отправляем заголовки запроса с завершением потока
	client->sendHeaders(sid, fields, true);
	// Формируем тело ответа сервера
	std::string payload(100000, '\0');
	/**
	 * Выполняем заполнение тела псевдослучайным паттерном
	 */
	for(size_t i = 0; i < payload.size(); ++i)
		// Заполняем очередной байт тела
		payload[i] = static_cast <char> ((i * 17 + 3) & 0xFF);
	// Формируем заголовки ответа сервера
	std::vector <h2::hpack::field_t> response;
	// Дописываем псевдо-заголовок статуса ответа
	response.emplace_back(":status", "200");
	// Отправляем заголовки ответа (тело выдаст pull-источник)
	server->sendHeaders(sid, response, false);
	// Счётчик выданных источником байт
	auto offset = std::make_shared <size_t> (0);
	// Назначаем pull-источник данных тела потока
	server->dataSource(sid, [offset, &payload](const uint32_t id, uint8_t * buffer, const size_t cap, bool & eof) noexcept -> int64_t {
		// Не используемый параметр
		(void) id;
		// Вычисляем размер очередной порции (не более 7000 байт за раз)
		const size_t chunk = std::min({cap, payload.size() - (* offset), static_cast <size_t> (7000)});
		// Копируем порцию тела в буфер парсера
		::memcpy(buffer, payload.data() + (* offset), chunk);
		// Смещаем счётчик выданных байт
		(* offset) += chunk;
		// Помечаем достижение конца тела
		eof = ((* offset) >= payload.size());
		// Выводим число записанных байт
		return static_cast <int64_t> (chunk);
	});
	// Проверяем что источник выдал всё тело
	ASSERT_EQ(* offset, payload.size());
	// Проверяем что клиент получил всё тело без искажений
	ASSERT_EQ(clientEvents.bodies[sid], payload);
	// Проверяем что поток клиента закрыт штатно
	ASSERT_EQ(clientEvents.closes.size(), 1u);
	// Проверяем код закрытия потока клиента
	ASSERT_EQ(clientEvents.closes.front().second, parser_http2_t::error_t::NO_ERROR);
}

/**
 * @brief Метод проверки server push (PUSH_PROMISE)
 *
 */
TEST_F(ParserHttp2Fixture, PushPromiseTest){
	// Создаём объект парсера сервера
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект парсера клиента
	auto client = this->make(direct_t::RESPONSE);
	// Создаём объекты сборщиков событий парсеров
	events_t serverEvents, clientEvents;
	// Подписываем сборщики событий на все функции обратного вызова парсеров
	this->attach(* server, serverEvents);
	// Подписываем сборщик событий клиента
	this->attach(* client, clientEvents);
	// Соединяем парсеры каналами записи
	this->connect(* client, * server);
	// Выполняем рукопожатие соединения
	this->handshake(* client, * server);
	// Выделяем идентификатор нового потока клиента
	const uint32_t sid = client->nextStreamId();
	// Формируем заголовки запроса страницы
	std::vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода запроса
	fields.emplace_back(":method", "GET");
	// Дописываем псевдо-заголовок схемы запроса
	fields.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути запроса
	fields.emplace_back(":path", "/index.html");
	// Отправляем заголовки запроса с завершением потока
	client->sendHeaders(sid, fields, true);
	// Формируем заголовки обещанного запроса (стиль как у запроса клиента)
	std::vector <h2::hpack::field_t> promise;
	// Дописываем псевдо-заголовок метода обещанного запроса
	promise.emplace_back(":method", "GET");
	// Дописываем псевдо-заголовок схемы обещанного запроса
	promise.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути обещанного запроса
	promise.emplace_back(":path", "/style.css");
	// Анонсируем server push на потоке запроса клиента
	const uint32_t promisedSid = server->sendPushPromise(sid, promise);
	// Проверяем что push-поток получил чётный идентификатор
	ASSERT_EQ(promisedSid, 2u);
	// Проверяем что клиент получил анонс server push
	ASSERT_EQ(clientEvents.pushes.size(), 1u);
	// Проверяем идентификатор ассоциированного потока клиента
	ASSERT_EQ(clientEvents.pushes.front().first, sid);
	// Проверяем идентификатор обещанного потока
	ASSERT_EQ(clientEvents.pushes.front().second, promisedSid);
	// Проверяем что провайдер обещанного запроса собран (метод и путь)
	ASSERT_EQ(clientEvents.method, method_t::GET);
	// Проверяем параметры URI-запроса обещанного запроса
	ASSERT_EQ(clientEvents.uri, "/style.css");
	// Формируем заголовки ответа push-потока
	std::vector <h2::hpack::field_t> response;
	// Дописываем псевдо-заголовок статуса ответа
	response.emplace_back(":status", "200");
	// Дописываем обычный заголовок
	response.emplace_back("content-type", "text/css");
	// Отправляем заголовки ответа push-потока
	server->sendHeaders(promisedSid, response, false);
	// Отправляем тело ответа push-потока с завершением потока
	server->sendData(promisedSid, "body{color:red}", 15, true);
	// Проверяем что клиент получил статус-код ответа push-потока
	ASSERT_EQ(clientEvents.code, 200u);
	// Проверяем что клиент получил тело ответа push-потока
	ASSERT_EQ(clientEvents.bodies[promisedSid], "body{color:red}");
	// Ищем событие штатного закрытия push-потока клиента
	bool pushClosed = false;
	/**
	 * Выполняем перебор всех событий закрытия потоков клиента
	 */
	for(const auto & close : clientEvents.closes){
		// Если найдено штатное закрытие push-потока
		if((close.first == promisedSid) && (close.second == parser_http2_t::error_t::NO_ERROR))
			// Помечаем что push-поток закрыт штатно
			pushClosed = true;
	}
	// Проверяем что push-поток клиента закрыт штатно
	ASSERT_TRUE(pushClosed);
}

/**
 * @brief Метод проверки отклонения server push клиентом
 *
 */
TEST_F(ParserHttp2Fixture, PushRejectTest){
	// Создаём объект парсера сервера
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект парсера клиента
	auto client = this->make(direct_t::RESPONSE);
	// Создаём объекты сборщиков событий парсеров
	events_t serverEvents, clientEvents;
	// Подписываем сборщики событий на все функции обратного вызова парсеров
	this->attach(* server, serverEvents);
	// Подписываем сборщик событий клиента
	this->attach(* client, clientEvents);
	// Переустанавливаем функцию обратного вызова анонса server push на отклонение
	client->on(parser_http2_t::push_callback_t([&clientEvents](const uint32_t sid, const uint32_t promisedSid) noexcept -> bool {
		// Собираем событие анонса server push
		clientEvents.pushes.emplace_back(sid, promisedSid);
		// Отклоняем push
		return false;
	}));
	// Соединяем парсеры каналами записи
	this->connect(* client, * server);
	// Выполняем рукопожатие соединения
	this->handshake(* client, * server);
	// Выделяем идентификатор нового потока клиента
	const uint32_t sid = client->nextStreamId();
	// Формируем заголовки запроса страницы
	std::vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода запроса
	fields.emplace_back(":method", "GET");
	// Дописываем псевдо-заголовок схемы запроса
	fields.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути запроса
	fields.emplace_back(":path", "/index.html");
	// Отправляем заголовки запроса с завершением потока
	client->sendHeaders(sid, fields, true);
	// Формируем заголовки обещанного запроса
	std::vector <h2::hpack::field_t> promise;
	// Дописываем псевдо-заголовок метода обещанного запроса
	promise.emplace_back(":method", "GET");
	// Дописываем псевдо-заголовок схемы обещанного запроса
	promise.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути обещанного запроса
	promise.emplace_back(":path", "/style.css");
	// Анонсируем server push (клиент отклонит его RST_STREAM с кодом CANCEL)
	const uint32_t promisedSid = server->sendPushPromise(sid, promise);
	// Проверяем что push-поток был зарезервирован
	ASSERT_EQ(promisedSid, 2u);
	// Проверяем что клиент получил анонс server push
	ASSERT_EQ(clientEvents.pushes.size(), 1u);
	// Ищем событие отклонения push-потока на сервере
	bool pushRejected = false;
	/**
	 * Выполняем перебор всех событий закрытия потоков сервера
	 */
	for(const auto & close : serverEvents.closes){
		// Если найдено отклонение push-потока
		if((close.first == promisedSid) && (close.second == parser_http2_t::error_t::CANCEL))
			// Помечаем что push-поток отклонён
			pushRejected = true;
	}
	// Проверяем что сервер получил отклонение push-потока
	ASSERT_TRUE(pushRejected);
	// Проверяем что соединение сервера живо
	ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL);
}

/**
 * @brief Метод проверки сброса потока функцией обратного вызова (возврат false)
 *
 */
TEST_F(ParserHttp2Fixture, HeaderCallbackResetTest){
	// Создаём объект парсера сервера
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект парсера клиента
	auto client = this->make(direct_t::RESPONSE);
	// Создаём объекты сборщиков событий парсеров
	events_t serverEvents, clientEvents;
	// Подписываем сборщики событий на все функции обратного вызова парсеров
	this->attach(* server, serverEvents);
	// Подписываем сборщик событий клиента
	this->attach(* client, clientEvents);
	// Переустанавливаем функцию обратного вызова заголовков сервера на отклонение
	server->on(parser_http2_t::header_callback_t([](const uint32_t sid, const std::string_view name, const std::string_view value, const parser_t::part_t part) noexcept -> bool {
		// Не используемые параметры
		(void) sid; (void) name; (void) value; (void) part;
		// Требуем сбросить поток
		return false;
	}));
	// Соединяем парсеры каналами записи
	this->connect(* client, * server);
	// Выполняем рукопожатие соединения
	this->handshake(* client, * server);
	// Выделяем идентификатор нового потока клиента
	const uint32_t sid = client->nextStreamId();
	// Формируем заголовки запроса
	std::vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода запроса
	fields.emplace_back(":method", "GET");
	// Дописываем псевдо-заголовок схемы запроса
	fields.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути запроса
	fields.emplace_back(":path", "/");
	// Отправляем заголовки запроса с завершением потока
	client->sendHeaders(sid, fields, true);
	// Проверяем что клиент получил сброс потока с кодом CANCEL
	ASSERT_EQ(clientEvents.closes.size(), 1u);
	// Проверяем идентификатор сброшенного потока
	ASSERT_EQ(clientEvents.closes.front().first, sid);
	// Проверяем код сброса потока
	ASSERT_EQ(clientEvents.closes.front().second, parser_http2_t::error_t::CANCEL);
	// Проверяем что соединения живы (потоковая ошибка не рушит соединение)
	ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL);
	// Проверяем статус клиента
	ASSERT_EQ(client->status(), parser_t::status_t::PARTIAL);
}

/**
 * @brief Метод проверки аварийного закрытия потока (RST_STREAM)
 *
 */
TEST_F(ParserHttp2Fixture, RstStreamTest){
	// Создаём объект парсера сервера
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект парсера клиента
	auto client = this->make(direct_t::RESPONSE);
	// Создаём объекты сборщиков событий парсеров
	events_t serverEvents, clientEvents;
	// Подписываем сборщики событий на все функции обратного вызова парсеров
	this->attach(* server, serverEvents);
	// Подписываем сборщик событий клиента
	this->attach(* client, clientEvents);
	// Соединяем парсеры каналами записи
	this->connect(* client, * server);
	// Выполняем рукопожатие соединения
	this->handshake(* client, * server);
	// Выделяем идентификатор нового потока клиента
	const uint32_t sid = client->nextStreamId();
	// Формируем заголовки запроса
	std::vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода запроса
	fields.emplace_back(":method", "GET");
	// Дописываем псевдо-заголовок схемы запроса
	fields.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути запроса
	fields.emplace_back(":path", "/slow");
	// Отправляем заголовки запроса (поток остаётся открытым)
	client->sendHeaders(sid, fields, false);
	// Отменяем запрос аварийным закрытием потока
	client->sendRstStream(sid, parser_http2_t::error_t::CANCEL);
	// Проверяем что сервер получил закрытие потока
	ASSERT_EQ(serverEvents.closes.size(), 1u);
	// Проверяем идентификатор закрытого потока
	ASSERT_EQ(serverEvents.closes.front().first, sid);
	// Проверяем код закрытия потока
	ASSERT_EQ(serverEvents.closes.front().second, parser_http2_t::error_t::CANCEL);
	// Проверяем что соединения живы
	ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL);
	// Проверяем статус клиента
	ASSERT_EQ(client->status(), parser_t::status_t::PARTIAL);
}

/**
 * @brief Метод проверки штатного завершения соединения (GOAWAY + eof)
 *
 */
TEST_F(ParserHttp2Fixture, GoawayTest){
	// Создаём объект парсера сервера
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект парсера клиента
	auto client = this->make(direct_t::RESPONSE);
	// Создаём объекты сборщиков событий парсеров
	events_t serverEvents, clientEvents;
	// Подписываем сборщики событий на все функции обратного вызова парсеров
	this->attach(* server, serverEvents);
	// Подписываем сборщик событий клиента
	this->attach(* client, clientEvents);
	// Соединяем парсеры каналами записи
	this->connect(* client, * server);
	// Выполняем рукопожатие соединения
	this->handshake(* client, * server);
	// Отправляем GOAWAY с отладочными данными
	client->sendGoaway(parser_http2_t::error_t::NO_ERROR, "bye");
	// Проверяем что клиент помечен на завершение
	ASSERT_TRUE(client->isClosed());
	// Проверяем что сервер получил GOAWAY
	ASSERT_TRUE(serverEvents.goawayFired);
	// Проверяем код ошибки завершения соединения
	ASSERT_EQ(serverEvents.goawayCode, parser_http2_t::error_t::NO_ERROR);
	// Проверяем отладочные данные завершения соединения
	ASSERT_EQ(serverEvents.goawayDebug, "bye");
	// Проверяем что сервер помечен на завершение
	ASSERT_TRUE(server->isClosed());
	// Уведомляем парсеры о закрытии соединения
	server->eof();
	// Уведомляем клиента о закрытии соединения
	client->eof();
	// Проверяем что соединение сервера завершено корректно
	ASSERT_EQ(server->status(), parser_t::status_t::COMPLETE);
	// Проверяем что соединение клиента завершено корректно
	ASSERT_EQ(client->status(), parser_t::status_t::COMPLETE);
}

/**
 * @brief Метод проверки реакции на некорректный connection preface
 *
 */
TEST_F(ParserHttp2Fixture, BrokenPrefaceTest){
	// Создаём объект парсера сервера
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект сборщика событий парсера
	events_t events;
	// Подписываем сборщик событий на все функции обратного вызова парсера
	this->attach(* server, events);
	// Формируем данные HTTP/1.1-запроса вместо connection preface
	const std::string garbage = "GET / HTTP/1.1\r\nHost: anyks.com\r\n\r\n";
	// Выполняем разбор некорректных данных
	server->parse(garbage.data(), garbage.size());
	// Проверяем что зафиксирована ошибка уровня соединения
	ASSERT_EQ(server->status(), parser_t::status_t::ERROR);
	// Проверяем код ошибки уровня соединения
	ASSERT_EQ(server->error(), parser_http2_t::error_t::PROTOCOL_ERROR);
	// Проверяем что функция обратного вызова ошибки вызвана
	ASSERT_TRUE(events.errorFired);
	// Проверяем код ошибки из функции обратного вызова
	ASSERT_EQ(events.errorCode, parser_http2_t::error_t::PROTOCOL_ERROR);
	// Проверяем что название ошибки формируется
	ASSERT_EQ(server->errorName(), "PROTOCOL_ERROR");
}

/**
 * @brief Метод проверки обрыва соединения посреди активных потоков
 *
 */
TEST_F(ParserHttp2Fixture, UnexpectedEofTest){
	// Создаём объект парсера сервера
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект парсера клиента
	auto client = this->make(direct_t::RESPONSE);
	// Создаём объекты сборщиков событий парсеров
	events_t serverEvents, clientEvents;
	// Подписываем сборщики событий на все функции обратного вызова парсеров
	this->attach(* server, serverEvents);
	// Подписываем сборщик событий клиента
	this->attach(* client, clientEvents);
	// Соединяем парсеры каналами записи
	this->connect(* client, * server);
	// Выполняем рукопожатие соединения
	this->handshake(* client, * server);
	// Выделяем идентификатор нового потока клиента
	const uint32_t sid = client->nextStreamId();
	// Формируем заголовки запроса
	std::vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода запроса
	fields.emplace_back(":method", "POST");
	// Дописываем псевдо-заголовок схемы запроса
	fields.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути запроса
	fields.emplace_back(":path", "/upload");
	// Отправляем заголовки запроса (поток остаётся открытым - тело не дослано)
	client->sendHeaders(sid, fields, false);
	// Уведомляем сервер о закрытии соединения посреди активного потока
	server->eof();
	// Проверяем что зафиксирована ошибка уровня соединения
	ASSERT_EQ(server->status(), parser_t::status_t::ERROR);
	// Проверяем код ошибки уровня соединения
	ASSERT_EQ(server->error(), parser_http2_t::error_t::PROTOCOL_ERROR);
}

/**
 * @brief Метод проверки отклонения малформированного блока заголовков (RFC 9113 §8.1.1)
 *
 */
TEST_F(ParserHttp2Fixture, MalformedHeadersTest){
	// Создаём объект парсера сервера
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект сборщика событий парсера
	events_t events;
	// Подписываем сборщик событий на все функции обратного вызова парсера
	this->attach(* server, events);
	// Формируем сырые байты соединения: connection preface клиента
	std::string raw(h2::proto::PREFACE);
	// Дописываем пустой SETTINGS клиента
	h2::frame::serializeSettings(raw, nullptr, 0, false);
	// Создаём кодер HPACK для формирования малформированного блока
	h2::hpack::encoder_t encoder;
	// Формируем заголовки с названием в верхнем регистре (запрещено RFC 9113 §8.2.1)
	std::vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода запроса
	fields.emplace_back(":method", "GET");
	// Дописываем псевдо-заголовок схемы запроса
	fields.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути запроса
	fields.emplace_back(":path", "/");
	// Дописываем заголовок с названием в верхнем регистре
	fields.emplace_back("X-Bad-Header", "1");
	// Буфер закодированного блока заголовков
	std::string block;
	// Кодируем малформированный блок заголовков
	encoder.encode(fields, block, true);
	// Дописываем блок заголовков с завершением потока
	h2::frame::serializeHeaderBlock(raw, 1, block, true, h2::proto::DEFAULT_MAX_FRAME_SIZE);
	// Выполняем разбор сырых байтов соединения
	server->parse(raw.data(), raw.size());
	// Проверяем что поток открывался
	ASSERT_EQ(events.begins.size(), 1u);
	// Проверяем что малформированный поток сброшен
	ASSERT_EQ(events.closes.size(), 1u);
	// Проверяем код сброса малформированного потока
	ASSERT_EQ(events.closes.front().second, parser_http2_t::error_t::PROTOCOL_ERROR);
	// Проверяем что заголовки малформированного блока не доставлялись
	ASSERT_TRUE(events.headers.empty());
	// Проверяем что соединение живо (потоковая ошибка не рушит соединение)
	ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL);
}

/**
 * @brief Метод проверки лимитов безопасности на заголовки и тело потока
 *
 */
TEST_F(ParserHttp2Fixture, SecurityLimitsTest){
	// Создаём объект парсера сервера
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект парсера клиента
	auto client = this->make(direct_t::RESPONSE);
	// Создаём объекты сборщиков событий парсеров
	events_t serverEvents, clientEvents;
	// Подписываем сборщики событий на все функции обратного вызова парсеров
	this->attach(* server, serverEvents);
	// Подписываем сборщик событий клиента
	this->attach(* client, clientEvents);
	// Формируем строгие лимиты безопасности сервера
	parser_http2_t::limits_t limits;
	// Ограничиваем число заголовков в одном блоке
	limits.maxHeaderCount = 4;
	// Ограничиваем суммарный размер тела потока
	limits.maxBodySize = 10;
	// Применяем лимиты безопасности сервера
	server->limits(limits);
	// Соединяем парсеры каналами записи
	this->connect(* client, * server);
	// Выполняем рукопожатие соединения
	this->handshake(* client, * server);
	// Выделяем идентификатор нового потока клиента
	const uint32_t first = client->nextStreamId();
	// Формируем заголовки запроса с превышением лимита количества
	std::vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода запроса
	fields.emplace_back(":method", "GET");
	// Дописываем псевдо-заголовок схемы запроса
	fields.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути запроса
	fields.emplace_back(":path", "/");
	// Дописываем первый обычный заголовок
	fields.emplace_back("x-one", "1");
	// Дописываем второй обычный заголовок (пятый в блоке - превышение лимита)
	fields.emplace_back("x-two", "2");
	// Отправляем заголовки запроса с завершением потока
	client->sendHeaders(first, fields, true);
	// Проверяем что поток сброшен из-за превышения лимита заголовков
	ASSERT_EQ(serverEvents.closes.size(), 1u);
	// Проверяем код сброса потока
	ASSERT_EQ(serverEvents.closes.front().second, parser_http2_t::error_t::ENHANCE_YOUR_CALM);
	// Проверяем что соединение живо
	ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL);
	// Выделяем идентификатор следующего потока клиента
	const uint32_t second = client->nextStreamId();
	// Формируем корректные заголовки запроса
	std::vector <h2::hpack::field_t> valid;
	// Дописываем псевдо-заголовок метода запроса
	valid.emplace_back(":method", "POST");
	// Дописываем псевдо-заголовок схемы запроса
	valid.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути запроса
	valid.emplace_back(":path", "/upload");
	// Отправляем заголовки запроса (тело последует отдельно)
	client->sendHeaders(second, valid, false);
	// Отправляем тело запроса больше лимита размера тела
	client->sendData(second, "0123456789ABCDEF", 16, true);
	// Проверяем что второй поток сброшен из-за превышения лимита тела
	ASSERT_EQ(serverEvents.closes.size(), 2u);
	// Проверяем код сброса второго потока
	ASSERT_EQ(serverEvents.closes.back().second, parser_http2_t::error_t::ENHANCE_YOUR_CALM);
	// Проверяем что тело сверх лимита не доставлялось
	ASSERT_TRUE(serverEvents.bodies.find(second) == serverEvents.bodies.end());
	// Проверяем что соединение живо
	ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL);
}

/**
 * @brief Метод проверки защиты от атаки Rapid Reset (CVE-2023-44487)
 *
 */
TEST_F(ParserHttp2Fixture, RapidResetProtectionTest){
	// Создаём объект парсера сервера
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект парсера клиента
	auto client = this->make(direct_t::RESPONSE);
	// Создаём объекты сборщиков событий парсеров
	events_t serverEvents, clientEvents;
	// Подписываем сборщики событий на все функции обратного вызова парсеров
	this->attach(* server, serverEvents);
	// Подписываем сборщик событий клиента
	this->attach(* client, clientEvents);
	// Формируем строгий лимит частоты входящих RST_STREAM
	parser_http2_t::limits_t limits;
	// Ограничиваем стартовый запас лимита частоты RST_STREAM
	limits.rstLimitBurst = 3;
	// Ограничиваем пополнение лимита частоты RST_STREAM
	limits.rstLimitRate = 1;
	// Применяем лимиты безопасности сервера
	server->limits(limits);
	// Соединяем парсеры каналами записи
	this->connect(* client, * server);
	// Выполняем рукопожатие соединения
	this->handshake(* client, * server);
	/**
	 * Выполняем атаку Rapid Reset: открытие потока и немедленный сброс
	 */
	for(size_t i = 0; i < 5; ++i){
		// Выделяем идентификатор нового потока клиента
		const uint32_t sid = client->nextStreamId();
		// Формируем заголовки запроса
		std::vector <h2::hpack::field_t> fields;
		// Дописываем псевдо-заголовок метода запроса
		fields.emplace_back(":method", "GET");
		// Дописываем псевдо-заголовок схемы запроса
		fields.emplace_back(":scheme", "https");
		// Дописываем псевдо-заголовок пути запроса
		fields.emplace_back(":path", "/");
		// Отправляем заголовки запроса (поток остаётся открытым)
		client->sendHeaders(sid, fields, false);
		// Немедленно сбрасываем поток
		client->sendRstStream(sid, parser_http2_t::error_t::CANCEL);
	}
	// Проверяем что сервер зафиксировал ошибку уровня соединения
	ASSERT_EQ(server->status(), parser_t::status_t::ERROR);
	// Проверяем код ошибки уровня соединения
	ASSERT_EQ(server->error(), parser_http2_t::error_t::ENHANCE_YOUR_CALM);
	// Проверяем что функция обратного вызова ошибки вызвана
	ASSERT_TRUE(serverEvents.errorFired);
	// Проверяем что клиент получил GOAWAY с кодом чрезмерного поведения
	ASSERT_TRUE(clientEvents.goawayFired);
	// Проверяем код ошибки завершения соединения
	ASSERT_EQ(clientEvents.goawayCode, parser_http2_t::error_t::ENHANCE_YOUR_CALM);
}

/**
 * @brief Метод проверки pull-модели выборки исходящих байтов (pending/consumePending)
 *
 */
TEST_F(ParserHttp2Fixture, PullOutputModelTest){
	// Создаём объект парсера сервера
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект парсера клиента
	auto client = this->make(direct_t::RESPONSE);
	// Создаём объекты сборщиков событий парсеров
	events_t serverEvents, clientEvents;
	// Подписываем сборщики событий на все функции обратного вызова парсеров
	this->attach(* server, serverEvents);
	// Подписываем сборщик событий клиента
	this->attach(* client, clientEvents);
	/**
	 * @brief Функция передачи исходящих байтов между парсерами (эмуляция записи в сокет)
	 *
	 * @param from парсер-отправитель
	 * @param to   парсер-получатель
	 */
	auto transfer = [](parser_http2_t & from, parser_http2_t & to) noexcept {
		/**
		 * Передаём исходящие байты, пока они есть
		 */
		while(!from.pending().empty()){
			// Копируем исходящие байты (view инвалидируется методами парсера)
			const std::string data(from.pending());
			// Освобождаем отправленные байты из исходящего буфера
			from.consumePending(data.size());
			// Выполняем разбор переданных байтов на получателе
			to.parse(data.data(), data.size());
		}
	};
	// Клиент отправляет magic-строку и свой SETTINGS (исходящие байты копятся в буфере)
	client->sendPreface();
	// Проверяем что исходящие байты доступны для выборки
	ASSERT_FALSE(client->pending().empty());
	// Передаём исходящие байты клиента серверу
	transfer(* client, * server);
	// Проверяем что исходящий буфер клиента опустошён
	ASSERT_TRUE(client->pending().empty());
	// Сервер отправляет свой SETTINGS
	server->sendPreface();
	// Передаём исходящие байты сервера клиенту (SETTINGS + ACK)
	transfer(* server, * client);
	// Передаём ответный ACK клиента серверу
	transfer(* client, * server);
	// Выделяем идентификатор нового потока клиента
	const uint32_t sid = client->nextStreamId();
	// Формируем заголовки запроса
	std::vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода запроса
	fields.emplace_back(":method", "GET");
	// Дописываем псевдо-заголовок схемы запроса
	fields.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути запроса
	fields.emplace_back(":path", "/pull");
	// Отправляем заголовки запроса с завершением потока
	client->sendHeaders(sid, fields, true);
	// Передаём исходящие байты клиента серверу
	transfer(* client, * server);
	// Проверяем что сервер получил запрос
	ASSERT_EQ(serverEvents.providers.size(), 1u);
	// Проверяем параметры URI-запроса из провайдера
	ASSERT_EQ(serverEvents.uri, "/pull");
	// Проверяем что оба соединения живы
	ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL);
}

/**
 * @brief Метод проверки клонирования парсера (фабрика с теми же настройками)
 *
 */
TEST_F(ParserHttp2Fixture, CloneTest){
	// Создаём объект парсера клиента
	auto client = this->make(direct_t::RESPONSE);
	// Формируем нестандартные лимиты безопасности
	parser_http2_t::limits_t limits;
	// Устанавливаем нестандартный лимит числа заголовков
	limits.maxHeaderCount = 42;
	// Устанавливаем нестандартный лимит размера тела потока
	limits.maxBodySize = 12345;
	// Применяем лимиты безопасности
	client->limits(limits);
	// Формируем нестандартные параметры SETTINGS
	parser_http2_t::settings_t settings;
	// Устанавливаем нестандартный лимит одновременных потоков
	settings.maxConcurrentStreams = 7;
	// Устанавливаем нестандартное начальное окно потока
	settings.windowSize = 32768;
	// Применяем параметры SETTINGS
	client->settings(settings);
	// Выполняем клонирование объекта парсера
	auto clone = client->clone();
	// Получаем объект клонированного парсера HTTP/2
	parser_http2_t * copy = static_cast <parser_http2_t *> (clone.get());
	// Проверяем что лимит числа заголовков унаследован
	ASSERT_EQ(copy->limits().maxHeaderCount, 42u);
	// Проверяем что лимит размера тела унаследован
	ASSERT_EQ(copy->limits().maxBodySize, 12345u);
	// Проверяем что лимит одновременных потоков унаследован
	ASSERT_EQ(copy->settings().maxConcurrentStreams, 7u);
	// Проверяем что начальное окно потока унаследовано
	ASSERT_EQ(copy->settings().windowSize, 32768);
	// Проверяем что клон получил чистое состояние соединения
	ASSERT_EQ(copy->status(), parser_t::status_t::NONE);
	// Проверяем что ошибок уровня соединения нет
	ASSERT_EQ(copy->error(), parser_http2_t::error_t::NO_ERROR);
	// Проверяем что клон не помечен на завершение
	ASSERT_FALSE(copy->isClosed());
}

/**
 * @brief Метод проверки полного сброса состояния соединения (переиспользование парсера)
 *
 */
TEST_F(ParserHttp2Fixture, ResetTest){
	// Создаём объект парсера сервера
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект парсера клиента
	auto client = this->make(direct_t::RESPONSE);
	// Создаём объекты сборщиков событий парсеров
	events_t serverEvents, clientEvents;
	// Подписываем сборщики событий на все функции обратного вызова парсеров
	this->attach(* server, serverEvents);
	// Подписываем сборщик событий клиента
	this->attach(* client, clientEvents);
	// Соединяем парсеры каналами записи
	this->connect(* client, * server);
	// Выполняем рукопожатие соединения
	this->handshake(* client, * server);
	// Выделяем идентификатор нового потока клиента
	const uint32_t sid = client->nextStreamId();
	// Формируем заголовки запроса
	std::vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода запроса
	fields.emplace_back(":method", "GET");
	// Дописываем псевдо-заголовок схемы запроса
	fields.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути запроса
	fields.emplace_back(":path", "/first");
	// Отправляем заголовки запроса с завершением потока
	client->sendHeaders(sid, fields, true);
	// Завершаем соединение штатно
	client->sendGoaway(parser_http2_t::error_t::NO_ERROR);
	// Проверяем что клиент помечен на завершение
	ASSERT_TRUE(client->isClosed());
	// Выполняем полный сброс состояния соединения на обоих концах
	server->reset();
	// Выполняем сброс клиента
	client->reset();
	// Проверяем что итоговый статус разбора сброшен
	ASSERT_EQ(client->status(), parser_t::status_t::NONE);
	// Проверяем что код ошибки сброшен
	ASSERT_EQ(client->error(), parser_http2_t::error_t::NO_ERROR);
	// Проверяем что пометка завершения снята
	ASSERT_FALSE(client->isClosed());
	// Очищаем собранные события сервера
	serverEvents.bodies.clear();
	// Очищаем собранные параметры URI-запроса
	serverEvents.uri.clear();
	// Выполняем рукопожатие нового соединения (функции обратного вызова сохранены)
	this->handshake(* client, * server);
	// Выделяем идентификатор нового потока клиента (нумерация начата заново)
	const uint32_t next = client->nextStreamId();
	// Проверяем что нумерация потоков начата заново
	ASSERT_EQ(next, 1u);
	// Формируем заголовки нового запроса
	std::vector <h2::hpack::field_t> second;
	// Дописываем псевдо-заголовок метода запроса
	second.emplace_back(":method", "GET");
	// Дописываем псевдо-заголовок схемы запроса
	second.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути запроса
	second.emplace_back(":path", "/second");
	// Отправляем заголовки нового запроса с завершением потока
	client->sendHeaders(next, second, true);
	// Проверяем что сервер получил новый запрос
	ASSERT_EQ(serverEvents.uri, "/second");
	// Проверяем что оба соединения живы
	ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL);
	// Проверяем статус клиента
	ASSERT_EQ(client->status(), parser_t::status_t::PARTIAL);
}
