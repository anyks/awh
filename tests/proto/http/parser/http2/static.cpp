/**
 * @file: static.cpp
 * @date: 2026-07-19
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Статические тесты парсера протокола HTTP/2 — проверка создания и сброса объекта модуля,
 *        а также корректности разбора фреймов, управления состояниями потоков,
 *        окнами flow control и кодирования HPACK
 *
 * @copyright: Copyright © 2026
 *
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
#include <algorithm>
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
 * @brief Вспомогательные функции тестов (внутренняя компоновка)
 *
 */
namespace {
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
	std::string frame(const uint8_t type, const uint8_t flags, const uint32_t sid, const std::string & payload) noexcept {
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
		result.push_back(static_cast <char> ((sid >> 24) & 0x7F));
		result.push_back(static_cast <char> ((sid >> 16) & 0xFF));
		result.push_back(static_cast <char> ((sid >> 8) & 0xFF));
		result.push_back(static_cast <char> (sid & 0xFF));
		// Дописываем полезную нагрузку
		result += payload;
		// Выводим собранный кадр
		return result;
	}
}

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
	h2::hpack::prefixed::encode(out, 10, 5, 0x00);
	// Проверяем что значение закодировано одним байтом
	ASSERT_EQ(out.size(), 1u);
	// Проверяем байт закодированного значения
	ASSERT_EQ(static_cast <uint8_t> (out[0]), 0x0A);
	// Очищаем буфер закодированного целого
	out.clear();
	// Кодируем значение 1337 с префиксом 5 бит (RFC 7541 C.1.2 - многобайтовое продолжение)
	h2::hpack::prefixed::encode(out, 1337, 5, 0x00);
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
	ASSERT_EQ(h2::hpack::prefixed::decode(reinterpret_cast <const uint8_t *> (out.data()), out.size(), 5, value, consumed), h2::status_t::OK);
	// Проверяем декодированное значение
	ASSERT_EQ(value, 1337u);
	// Проверяем количество прочитанных байт
	ASSERT_EQ(consumed, 3u);
	// Декодируем неполный буфер (без последнего байта продолжения)
	ASSERT_EQ(h2::hpack::prefixed::decode(reinterpret_cast <const uint8_t *> (out.data()), out.size() - 1, 5, value, consumed), h2::status_t::INCOMPLETE);
	// Формируем заведомо переполняющую последовательность продолжений
	const std::string overflow = "\x1F\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x7F";
	// Декодируем переполняющую последовательность - ожидаем ошибку
	ASSERT_EQ(h2::hpack::prefixed::decode(reinterpret_cast <const uint8_t *> (overflow.data()), overflow.size(), 5, value, consumed), h2::status_t::ERROR);
}

/**
 * @brief Метод проверки Huffman-кодирования строк (RFC 7541 Appendix B)
 *
 */
TEST(Http2Hpack, HuffmanCodecTest){
	// Буфер закодированной строки
	std::string encoded;
	// Кодируем строку из примера RFC 7541 C.4.1
	h2::hpack::huffman::encode("www.example.com", encoded);
	// Формируем эталон закодированной строки из RFC 7541 C.4.1
	const std::string expected = "\xF1\xE3\xC2\xE5\xF2\x3A\x6B\xA0\xAB\x90\xF4\xFF";
	// Проверяем что строка закодирована как в эталоне
	ASSERT_EQ(encoded, expected);
	// Проверяем что предвычисленная длина совпадает с фактической
	ASSERT_EQ(h2::hpack::huffman::length("www.example.com"), encoded.size());
	// Буфер декодированной строки
	std::string decoded;
	// Декодируем закодированную строку обратно
	ASSERT_TRUE(h2::hpack::huffman::decode(reinterpret_cast <const uint8_t *> (encoded.data()), encoded.size(), decoded));
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
	h2::hpack::huffman::encode(binary, encoded);
	// Декодируем бинарную строку обратно
	ASSERT_TRUE(h2::hpack::huffman::decode(reinterpret_cast <const uint8_t *> (encoded.data()), encoded.size(), decoded));
	// Проверяем что бинарная строка пережила кодирование без искажений
	ASSERT_EQ(decoded, binary);
	/**
	 * Декодирование идёт табличными шагами по шестнадцать бит, а хвост, на котором
	 * шага уже не набирается, и коды длиннее шестнадцати бит - побитовым спуском
	 * по дереву. Переход между путями обязан быть незаметен на любом выравнивании
	 * хвоста и на любом символе, поэтому перебираются все значения октета
	 * во всех длинах, дающих различные остатки по модулю восьми бит
	 */
	for(uint16_t letter = 0; letter < 256; ++letter){
		/**
		 * Выполняем перебор всех длин строки, покрывающих все выравнивания хвоста
		 */
		for(size_t length = 1; length <= 24; ++length){
			// Формируем строку из повторяющегося октета
			const std::string sample(length, static_cast <char> (letter));
			// Очищаем буфер закодированной строки
			encoded.clear();
			// Очищаем буфер декодированной строки
			decoded.clear();
			// Кодируем строку из повторяющегося октета
			h2::hpack::huffman::encode(sample, encoded);
			// Проверяем что предвычисленная длина совпадает с фактической
			ASSERT_EQ(h2::hpack::huffman::length(sample), encoded.size());
			// Декодируем строку обратно
			ASSERT_TRUE(h2::hpack::huffman::decode(reinterpret_cast <const uint8_t *> (encoded.data()), encoded.size(), decoded));
			// Проверяем что строка пережила кодирование без искажений
			ASSERT_EQ(decoded, sample);
		}
	}
	/**
	 * Заполнение хвоста задано единичными битами и не длиннее семи (RFC 7541 §5.2):
	 * нулевой бит в заполнении и заполнение в целый октет - ошибка сжатия
	 */
	const std::string zero = "\xF1\xE3\xC2\xE5\xF2\x3A\x6B\xA0\xAB\x90\xF4\xFE";
	// Декодируем строку с нулевым битом в заполнении - ожидаем ошибку
	ASSERT_FALSE(h2::hpack::huffman::decode(reinterpret_cast <const uint8_t *> (zero.data()), zero.size(), decoded));
	// Формируем строку с заполнением длиною в целый октет
	const std::string padded = "\xF1\xE3\xC2\xE5\xF2\x3A\x6B\xA0\xAB\x90\xF4\xFF\xFF";
	// Декодируем строку с избыточным заполнением - ожидаем ошибку
	ASSERT_FALSE(h2::hpack::huffman::decode(reinterpret_cast <const uint8_t *> (padded.data()), padded.size(), decoded));
	// Формируем строку с кодом символа EOS, который внутри потока недопустим
	const std::string eos = "\xFF\xFF\xFF\xFF\xFF";
	// Декодируем строку с кодом EOS - ожидаем ошибку
	ASSERT_FALSE(h2::hpack::huffman::decode(reinterpret_cast <const uint8_t *> (eos.data()), eos.size(), decoded));
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
	// Список декодированных заголовков (представления в арену декодера)
	std::vector <h2::hpack::field_view_t> decoded;
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
	// Список декодированных заголовков (представления в арену декодера)
	std::vector <h2::hpack::field_view_t> decoded;
	// Код ошибки протокола
	h2::error_t err = h2::error_t::NO_ERROR;
	/**
	 * Декодируем блок с лимитом меньше распакованного размера. Блок разбирается целиком -
	 * иначе динамическая таблица разъедется с кодером пира и соединение придётся рвать, -
	 * но заголовки наружу не отдаются
	 */
	ASSERT_EQ(decoder.decode(block, decoded, 100, err), h2::status_t::OK);
	// Проверяем что зафиксировано превышение лимита
	ASSERT_TRUE(decoder.overflowed());
	// Проверяем что зафиксирован код ошибки чрезмерного поведения
	ASSERT_EQ(err, h2::error_t::ENHANCE_YOUR_CALM);
	// Проверяем что заголовки наружу не отданы
	ASSERT_TRUE(decoded.empty());
	// Проверяем что заголовок всё же попал в динамическую таблицу (состояние HPACK синхронно)
	ASSERT_EQ(decoder.table().count(), 1u);
	// Очищаем буфер закодированного блока
	block.clear();
	// Кодируем тот же заголовок повторно - кодер сошлётся на запись динамической таблицы
	encoder.encode(fields, block, true);
	// Декодируем второй блок без ограничения размера списка
	ASSERT_EQ(decoder.decode(block, decoded, 0, err), h2::status_t::OK);
	// Проверяем что превышение больше не фиксируется
	ASSERT_FALSE(decoder.overflowed());
	// Проверяем что заголовок разрешён по индексу - таблица не рассинхронизировалась
	ASSERT_EQ(decoded.size(), 1u);
	// Проверяем название разрешённого заголовка
	ASSERT_EQ(decoded.front().name, "x-large");
	// Проверяем значение разрешённого заголовка
	ASSERT_EQ(decoded.front().value, std::string(1024, 'a'));
}

/**
 * @brief Метод проверки переживания заголовком вытеснения его записи внутри блока
 *
 * @details Декодер отдаёт наружу представления прямо в строки динамической таблицы,
 *          не копируя их. Заголовок, разрешённый по индексу, обязан пережить
 *          вытеснение своей записи заголовками того же блока: вытеснение отложено
 *          до начала следующего разбора
 *
 */
TEST(Http2Hpack, DecoderEvictedEntrySurvivesBlockTest){
	/**
	 * @brief Функция дописывания представления Literal with Incremental Indexing
	 *
	 * @param block буфер собираемого блока заголовков
	 * @param name  название заголовка
	 * @param value значение заголовка
	 *
	 */
	auto literal = [](std::string & block, const std::string & name, const std::string & value) noexcept -> void {
		// Дописываем представление с новым названием (RFC 7541 §6.2.1)
		block.push_back(static_cast <char> (0x40));
		// Дописываем длину названия заголовка без Huffman-кодирования
		h2::hpack::prefixed::encode(block, name.size(), 7, 0x00);
		// Дописываем название заголовка
		block.append(name);
		// Дописываем длину значения заголовка без Huffman-кодирования
		h2::hpack::prefixed::encode(block, value.size(), 7, 0x00);
		// Дописываем значение заголовка
		block.append(value);
	};
	/**
	 * Строки берутся длинными намеренно: короткие стандартная библиотека держит
	 * внутри самого объекта, и разрушение записи оставило бы их читаемыми - проверка
	 * прошла бы и без отложенного вытеснения. Длинные лежат в отдельной памяти,
	 * и обращение к вытесненной записи стало бы обращением к освобождённой
	 */
	const std::string first(40, 'a'), second(40, 'b'), third(40, 'c');
	/**
	 * Размер таблицы выбран так, чтобы вместить ровно две записи: размер записи
	 * равен сумме длин названия и значения плюс 32 (RFC 7541 §4.1), то есть 112
	 */
	h2::hpack::decoder_t decoder(240);
	// Буфер собираемого блока заголовков
	std::string block;
	// Дописываем заголовок, который попадёт в динамическую таблицу
	literal(block, first, first);
	// Дописываем ссылку на только что добавленную запись (Indexed Header Field, индекс 62)
	block.push_back(static_cast <char> (0xBE));
	// Дописываем второй заголовок в таблицу
	literal(block, second, second);
	// Дописываем третий заголовок - он вытеснит первый
	literal(block, third, third);
	// Список декодированных заголовков (представления в арену и в таблицу декодера)
	std::vector <h2::hpack::field_view_t> decoded;
	// Код ошибки протокола
	h2::error_t err = h2::error_t::NO_ERROR;
	// Декодируем собранный блок заголовков
	ASSERT_EQ(decoder.decode(block, decoded, 0, err), h2::status_t::OK);
	// Проверяем что отданы все четыре заголовка блока
	ASSERT_EQ(decoded.size(), 4u);
	// Проверяем что первый заголовок разобран верно
	ASSERT_EQ(decoded[0].name, first);
	// Проверяем значение первого заголовка
	ASSERT_EQ(decoded[0].value, first);
	/**
	 * Второй заголовок разрешён по индексу, а его запись вытеснена четвёртым:
	 * представление обязано остаться действительным до конца разбора
	 */
	ASSERT_EQ(decoded[1].name, first);
	// Проверяем значение заголовка, чья запись вытеснена внутри блока
	ASSERT_EQ(decoded[1].value, first);
	// Проверяем что третий заголовок разобран верно
	ASSERT_EQ(decoded[2].name, second);
	// Проверяем значение третьего заголовка
	ASSERT_EQ(decoded[2].value, second);
	// Проверяем что четвёртый заголовок разобран верно
	ASSERT_EQ(decoded[3].name, third);
	// Проверяем значение четвёртого заголовка
	ASSERT_EQ(decoded[3].value, third);
	// Проверяем что в таблице остались только две последние записи
	ASSERT_EQ(decoder.table().count(), 2u);
	// Очищаем буфер собираемого блока заголовков
	block.clear();
	/**
	 * Тот же случай, но вытеснение вызвано записью, не помещающейся в таблицу целиком:
	 * такая запись очищает таблицу вовсе (RFC 7541 §4.4)
	 */
	const std::string fourth(40, 'd');
	// Дописываем заголовок, который попадёт в динамическую таблицу
	literal(block, fourth, fourth);
	// Дописываем ссылку на только что добавленную запись
	block.push_back(static_cast <char> (0xBE));
	// Дописываем заголовок, не помещающийся в таблицу целиком
	literal(block, std::string(40, 'e'), std::string(200, 'e'));
	// Декодируем собранный блок заголовков
	ASSERT_EQ(decoder.decode(block, decoded, 0, err), h2::status_t::OK);
	// Проверяем что отданы все три заголовка блока
	ASSERT_EQ(decoded.size(), 3u);
	// Проверяем что заголовок, чья запись очищена вместе с таблицей, читается верно
	ASSERT_EQ(decoded[1].name, fourth);
	// Проверяем значение заголовка, чья запись очищена вместе с таблицей
	ASSERT_EQ(decoded[1].value, fourth);
	// Проверяем что негабаритная запись очистила таблицу целиком
	ASSERT_EQ(decoder.table().count(), 0u);
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
	h2::frame::serialize::settings(out, items, 2, false);
	// Разобранный заголовок фрейма
	h2::frame::header_t header;
	// Разбираем заголовок фрейма
	ASSERT_TRUE(h2::frame::parser::header(reinterpret_cast <const uint8_t *> (out.data()), out.size(), header));
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
	ASSERT_EQ(h2::frame::parser::settings(header, reinterpret_cast <const uint8_t *> (out.data()) + h2::proto::FRAME_HEADER_SIZE, parsed, err), h2::status_t::OK);
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
	h2::frame::serialize::data(out, 1, "Hello, HTTP/2!", true);
	// Разобранный заголовок фрейма
	h2::frame::header_t header;
	// Разбираем заголовок фрейма
	ASSERT_TRUE(h2::frame::parser::header(reinterpret_cast <const uint8_t *> (out.data()), out.size(), header));
	// Проверяем тип фрейма
	ASSERT_EQ(header.type, h2::frame_t::DATA);
	// Проверяем идентификатор потока
	ASSERT_EQ(header.streamId, 1u);
	// Разобранная полезная нагрузка DATA
	h2::frame::data_t data;
	// Код ошибки протокола
	h2::error_t err = h2::error_t::NO_ERROR;
	// Разбираем полезную нагрузку DATA
	ASSERT_EQ(h2::frame::parser::data(header, reinterpret_cast <const uint8_t *> (out.data()) + h2::proto::FRAME_HEADER_SIZE, data, err), h2::status_t::OK);
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
	ASSERT_EQ(h2::frame::parser::data(padded, payload, data, err), h2::status_t::OK);
	// Проверяем что padding снят и данные извлечены корректно
	ASSERT_EQ(data.data, "hi");
	// Формируем некорректный padding (Pad Length >= длины нагрузки)
	const uint8_t broken[2] = {0x05, 'x'};
	// Корректируем длину полезной нагрузки
	padded.length = 2;
	// Разбираем некорректную полезную нагрузку - ожидаем ошибку
	ASSERT_EQ(h2::frame::parser::data(padded, broken, data, err), h2::status_t::ERROR);
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
	h2::frame::serialize::headerBlock(out, 1, block, true, h2::proto::DEFAULT_MAX_FRAME_SIZE);
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
		ASSERT_TRUE(h2::frame::parser::header(reinterpret_cast <const uint8_t *> (out.data()) + pos, out.size() - pos, header));
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
			ASSERT_EQ(h2::frame::parser::headers(header, reinterpret_cast <const uint8_t *> (out.data()) + pos, headers, err), h2::status_t::OK);
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
			ASSERT_EQ(h2::frame::parser::continuation(header, reinterpret_cast <const uint8_t *> (out.data()) + pos, fragment, endHeaders, err), h2::status_t::OK);
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
		h2::frame::serialize::ping(out, opaque, false);
		// Разбираем заголовок фрейма
		ASSERT_TRUE(h2::frame::parser::header(reinterpret_cast <const uint8_t *> (out.data()), out.size(), header));
		// Проверяем тип фрейма
		ASSERT_EQ(header.type, h2::frame_t::PING);
		// Извлечённые opaque-данные
		uint8_t parsed[8] = {0};
		// Разбираем полезную нагрузку PING
		ASSERT_EQ(h2::frame::parser::ping(header, reinterpret_cast <const uint8_t *> (out.data()) + h2::proto::FRAME_HEADER_SIZE, parsed, err), h2::status_t::OK);
		// Проверяем что opaque-данные извлечены без искажений
		ASSERT_EQ(::memcmp(parsed, opaque, 8), 0);
	}
	{
		// Буфер собранного фрейма
		std::string out;
		// Собираем фрейм GOAWAY с отладочными данными
		h2::frame::serialize::goaway(out, 5, h2::error_t::ENHANCE_YOUR_CALM, "too fast");
		// Разбираем заголовок фрейма
		ASSERT_TRUE(h2::frame::parser::header(reinterpret_cast <const uint8_t *> (out.data()), out.size(), header));
		// Разобранная полезная нагрузка GOAWAY
		h2::frame::goaway_t goaway;
		// Разбираем полезную нагрузку GOAWAY
		ASSERT_EQ(h2::frame::parser::goaway(header, reinterpret_cast <const uint8_t *> (out.data()) + h2::proto::FRAME_HEADER_SIZE, goaway, err), h2::status_t::OK);
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
		h2::frame::serialize::windowUpdate(out, 3, 65535);
		// Разбираем заголовок фрейма
		ASSERT_TRUE(h2::frame::parser::header(reinterpret_cast <const uint8_t *> (out.data()), out.size(), header));
		// Извлечённый инкремент окна
		uint32_t increment = 0;
		// Разбираем полезную нагрузку WINDOW_UPDATE
		ASSERT_EQ(h2::frame::parser::windowUpdate(header, reinterpret_cast <const uint8_t *> (out.data()) + h2::proto::FRAME_HEADER_SIZE, increment, err), h2::status_t::OK);
		// Проверяем инкремент окна
		ASSERT_EQ(increment, 65535u);
	}
	{
		// Буфер собранного фрейма
		std::string out;
		// Собираем фрейм RST_STREAM
		h2::frame::serialize::rstStream(out, 7, h2::error_t::CANCEL);
		// Разбираем заголовок фрейма
		ASSERT_TRUE(h2::frame::parser::header(reinterpret_cast <const uint8_t *> (out.data()), out.size(), header));
		// Извлечённый код ошибки сброса потока
		h2::error_t code = h2::error_t::NO_ERROR;
		// Разбираем полезную нагрузку RST_STREAM
		ASSERT_EQ(h2::frame::parser::rstStream(header, reinterpret_cast <const uint8_t *> (out.data()) + h2::proto::FRAME_HEADER_SIZE, code, err), h2::status_t::OK);
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
 * @brief Метод проверки последовательности фазовых событий приёма сообщений потоков
 *
 */
TEST_F(ParserHttp2Fixture, PhaseSequenceTest){
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
	/**
	 * Сценарий 1: запрос без тела (END_STREAM с заголовками)
	 */
	const uint32_t sid1 = client->nextStreamId();
	// Формируем контейнер заголовков запроса без тела
	headers_t getRequest(std::make_unique <request_t> (version_t::HTTP2, method_t::GET, std::string("/")));
	// Дописываем заголовок Host
	getRequest.emplace("Host", "anyks.com");
	// Отправляем заголовки запроса с завершением потока
	client->sendHeaders(sid1, getRequest, true);
	/**
	 * Сценарий 2: запрос с телом и трейлерами
	 */
	const uint32_t sid2 = client->nextStreamId();
	// Формируем контейнер заголовков запроса с телом
	headers_t postRequest(std::make_unique <request_t> (version_t::HTTP2, method_t::POST, std::string("/upload")));
	// Дописываем заголовок Host
	postRequest.emplace("Host", "anyks.com");
	// Отправляем заголовки запроса (тело и трейлеры последуют)
	client->sendHeaders(sid2, postRequest, false);
	// Отправляем тело запроса (поток остаётся открытым для трейлеров)
	client->sendData(sid2, "hello", 5, false);
	// Формируем контейнер трейлеров (без провайдера - псевдо-заголовки не формируются)
	headers_t trailers;
	// Дописываем трейлер контрольной суммы
	trailers.emplace("X-Checksum", "5d41402a");
	// Отправляем трейлеры с завершением потока
	client->sendHeaders(sid2, trailers, true);
	// Собранные фазовые события по каждому из потоков
	std::vector <std::pair <parser_t::phase_t, parser_t::part_t>> phases1, phases2;
	/**
	 * Выполняем разбор собранных фазовых событий сервера по потокам
	 */
	for(const auto & phase : serverEvents.phases){
		// Если событие принадлежит потоку запроса без тела
		if(std::get <0> (phase) == sid1)
			// Собираем фазовое событие потока запроса без тела
			phases1.emplace_back(std::get <1> (phase), std::get <2> (phase));
		// Если событие принадлежит потоку запроса с телом и трейлерами
		else if(std::get <0> (phase) == sid2)
			// Собираем фазовое событие потока запроса с телом и трейлерами
			phases2.emplace_back(std::get <1> (phase), std::get <2> (phase));
	}
	// Ожидаемая последовательность фаз для запроса без тела
	const std::vector <std::pair <parser_t::phase_t, parser_t::part_t>> expected1 = {
		{parser_t::phase_t::BEGIN, parser_t::part_t::NONE},
		{parser_t::phase_t::END, parser_t::part_t::HEADERS},
		{parser_t::phase_t::END, parser_t::part_t::NONE}
	};
	// Ожидаемая последовательность фаз для запроса с телом и трейлерами
	const std::vector <std::pair <parser_t::phase_t, parser_t::part_t>> expected2 = {
		{parser_t::phase_t::BEGIN, parser_t::part_t::NONE},
		{parser_t::phase_t::END, parser_t::part_t::HEADERS},
		{parser_t::phase_t::BEGIN, parser_t::part_t::BODY},
		{parser_t::phase_t::END, parser_t::part_t::BODY},
		{parser_t::phase_t::BEGIN, parser_t::part_t::TRAILER},
		{parser_t::phase_t::END, parser_t::part_t::TRAILER},
		{parser_t::phase_t::END, parser_t::part_t::NONE}
	};
	// Проверяем последовательность фаз для запроса без тела
	ASSERT_EQ(phases1, expected1);
	// Проверяем последовательность фаз для запроса с телом и трейлерами
	ASSERT_EQ(phases2, expected2);
	/**
	 * Сценарий 3: тело завершается фреймом DATA с END_STREAM (без трейлеров)
	 */
	const uint32_t sid3 = client->nextStreamId();
	// Формируем контейнер заголовков запроса с телом без трейлеров
	headers_t putRequest(std::make_unique <request_t> (version_t::HTTP2, method_t::PUT, std::string("/data")));
	// Дописываем заголовок Host
	putRequest.emplace("Host", "anyks.com");
	// Отправляем заголовки запроса (тело последует)
	client->sendHeaders(sid3, putRequest, false);
	// Отправляем тело запроса с завершением потока
	client->sendData(sid3, "world", 5, true);
	// Собранные фазовые события потока с телом без трейлеров
	std::vector <std::pair <parser_t::phase_t, parser_t::part_t>> phases3;
	/**
	 * Выполняем разбор собранных фазовых событий сервера по потокам
	 */
	for(const auto & phase : serverEvents.phases){
		// Если событие принадлежит потоку с телом без трейлеров
		if(std::get <0> (phase) == sid3)
			// Собираем фазовое событие потока с телом без трейлеров
			phases3.emplace_back(std::get <1> (phase), std::get <2> (phase));
	}
	// Ожидаемая последовательность фаз для тела, завершённого фреймом DATA
	const std::vector <std::pair <parser_t::phase_t, parser_t::part_t>> expected3 = {
		{parser_t::phase_t::BEGIN, parser_t::part_t::NONE},
		{parser_t::phase_t::END, parser_t::part_t::HEADERS},
		{parser_t::phase_t::BEGIN, parser_t::part_t::BODY},
		{parser_t::phase_t::END, parser_t::part_t::BODY},
		{parser_t::phase_t::END, parser_t::part_t::NONE}
	};
	// Проверяем последовательность фаз для тела, завершённого фреймом DATA
	ASSERT_EQ(phases3, expected3);
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
 * @brief Метод проверки сброса потока прямо из pull-источника данных тела
 *
 * @details Источник вызывается для живого объекта потока, поэтому его закрытие
 *          изнутри обязано прекращать дозагрузку, а не продолжать работу
 *          с уничтоженным буфером отправки
 *
 */
TEST_F(ParserHttp2Fixture, DataSourceClosesStreamTest){
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
	// Формируем заголовки ответа сервера
	std::vector <h2::hpack::field_t> response;
	// Дописываем псевдо-заголовок статуса ответа
	response.emplace_back(":status", "200");
	// Отправляем заголовки ответа (тело выдаст pull-источник)
	server->sendHeaders(sid, response, false);
	// Счётчик обращений к источнику данных
	size_t calls = 0;
	// Назначаем pull-источник данных тела, сбрасывающий поток изнутри
	server->dataSource(sid, [&](const uint32_t id, uint8_t * buffer, const size_t cap, bool & eof) noexcept -> int64_t {
		// Считаем обращения к источнику данных
		calls++;
		// Приложение решило прервать передачу прямо из источника
		server->sendRstStream(id, parser_http2_t::error_t::CANCEL);
		// Помечаем достижение конца тела
		eof = true;
		// Не используемые параметры
		(void) buffer;
		(void) cap;
		// Данных нет
		return 0;
	});
	// Проверяем что источник вызван ровно один раз (дозагрузка прекращена)
	ASSERT_EQ(calls, 1u);
	// Проверяем что поток сервера закрыт
	ASSERT_EQ(serverEvents.closes.size(), 1u);
	// Проверяем код закрытия потока сервера
	ASSERT_EQ(serverEvents.closes.front().second, parser_http2_t::error_t::CANCEL);
	// Проверяем что клиент получил сброс потока
	ASSERT_EQ(clientEvents.closes.size(), 1u);
	// Проверяем что соединение сервера осталось живо
	ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL);
	// Проверяем что соединение клиента осталось живо
	ASSERT_EQ(client->status(), parser_t::status_t::PARTIAL);
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
	h2::frame::serialize::settings(raw, nullptr, 0, false);
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
	h2::frame::serialize::headerBlock(raw, 1, block, true, h2::proto::DEFAULT_MAX_FRAME_SIZE);
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
	 *
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

/**
 * @brief Метод проверки промежуточных информационных ответов 1xx (RFC 9113 §8.1)
 *
 */
TEST_F(ParserHttp2Fixture, InformationalResponseTest){
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
	fields.emplace_back(":path", "/hints");
	// Отправляем заголовки запроса с завершением потока
	client->sendHeaders(sid, fields, true);
	// Формируем промежуточный информационный ответ (Early Hints)
	headers_t interim(std::make_unique <response_t> (version_t::HTTP2, 103));
	// Отправляем информационный ответ без завершения потока
	server->sendHeaders(sid, interim, false);
	// Проверяем что информационный ответ доставлен клиенту
	ASSERT_EQ(clientEvents.providers.size(), 1u);
	// Проверяем что доставлен именно информационный статус-код
	ASSERT_EQ(clientEvents.code, 103);
	// Проверяем что фазы приёма сообщения по промежуточному ответу не начинались
	ASSERT_TRUE(clientEvents.phases.empty());
	// Формируем финальный ответ сервера
	headers_t response(std::make_unique <response_t> (version_t::HTTP2, 200));
	// Отправляем финальный ответ с завершением потока
	server->sendHeaders(sid, response, true);
	// Проверяем что финальный ответ доставлен отдельным событием провайдера
	ASSERT_EQ(clientEvents.providers.size(), 2u);
	// Проверяем что статус-код финального ответа получен без искажений
	ASSERT_EQ(clientEvents.code, 200);
	// Проверяем что фазы приёма сообщения начались только с финальным ответом
	ASSERT_FALSE(clientEvents.phases.empty());
	// Проверяем что приём сообщения потока начат
	ASSERT_EQ(std::get <1> (clientEvents.phases.front()), parser_t::phase_t::BEGIN);
	// Проверяем что первая фаза относится к сообщению целиком
	ASSERT_EQ(std::get <2> (clientEvents.phases.front()), parser_t::part_t::NONE);
	// Проверяем что приём сообщения потока завершён
	ASSERT_EQ(std::get <1> (clientEvents.phases.back()), parser_t::phase_t::END);
	// Проверяем что последняя фаза относится к сообщению целиком
	ASSERT_EQ(std::get <2> (clientEvents.phases.back()), parser_t::part_t::NONE);
	// Проверяем что соединение живо
	ASSERT_EQ(client->status(), parser_t::status_t::PARTIAL);
	// Проверяем статус сервера
	ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL);
}

/**
 * @brief Метод проверки отклонения информационного ответа с флагом END_STREAM
 *
 */
TEST_F(ParserHttp2Fixture, InformationalEndStreamTest){
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
	fields.emplace_back(":path", "/hints");
	// Отправляем заголовки запроса с завершением потока
	client->sendHeaders(sid, fields, true);
	// Формируем информационный ответ, ошибочно завершающий поток
	headers_t interim(std::make_unique <response_t> (version_t::HTTP2, 100));
	// Отправляем информационный ответ с завершением потока
	server->sendHeaders(sid, interim, true);
	// Проверяем что малформированный ответ клиенту не доставлен
	ASSERT_TRUE(clientEvents.providers.empty());
	// Проверяем что клиент сбросил поток
	ASSERT_EQ(clientEvents.closes.size(), 1u);
	// Проверяем код сброса потока
	ASSERT_EQ(clientEvents.closes.front().second, parser_http2_t::error_t::PROTOCOL_ERROR);
	// Проверяем что фазы приёма сообщения не начинались
	ASSERT_TRUE(clientEvents.phases.empty());
	// Проверяем что соединение живо
	ASSERT_EQ(client->status(), parser_t::status_t::PARTIAL);
	// Проверяем статус сервера
	ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL);
}

/**
 * @brief Метод проверки отбраковки недопустимых имён и значений заголовков (RFC 9113 §8.2.1)
 *
 */
TEST_F(ParserHttp2Fixture, HeaderFieldValidationTest){
	/**
	 * @brief Структура проверяемого некорректного заголовка
	 *
	 */
	struct Entry {
		// Название заголовка
		std::string name;
		// Значение заголовка
		std::string value;
	};
	// Таблица недопустимых заголовков
	const Entry entries[] = {
		{"x-inject", "value\r\nevil: 1"},          // перевод строки в значении
		{"x-inject", "value\r"},                   // возврат каретки в значении
		{"x-inject", std::string("a\0b", 3)},      // нулевой байт в значении
		{"bad name", "1"},                         // пробел в названии
		{"x-tab", "\tvalue"},                      // начальный пробельный символ значения
		{"x-tail", "value "},                      // конечный пробельный символ значения
		{"Upper", "1"},                            // верхний регистр в названии
		{"x:colon", "1"}                           // двоеточие не первым символом названия
	};
	/**
	 * Выполняем перебор всех проверяемых заголовков
	 */
	for(const auto & entry : entries){
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
		fields.emplace_back(":path", "/");
		// Дописываем проверяемый некорректный заголовок
		fields.emplace_back(entry.name, entry.value);
		// Отправляем заголовки запроса с завершением потока
		client->sendHeaders(sid, fields, true);
		// Проверяем что малформированный запрос серверу не доставлен
		ASSERT_TRUE(serverEvents.providers.empty()) << "accepted header: " << entry.name;
		// Проверяем что поток сброшен как малформированный
		ASSERT_EQ(serverEvents.closes.size(), 1u) << "not reset for header: " << entry.name;
		// Проверяем код сброса потока
		ASSERT_EQ(serverEvents.closes.front().second, parser_http2_t::error_t::PROTOCOL_ERROR);
		// Проверяем что соединение осталось живо (малформированность - потоковая ошибка)
		ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL) << "connection killed by header: " << entry.name;
	}
}

/**
 * @brief Метод проверки строгого регистра псевдо-заголовков и метода запроса (RFC 9113 §8.2.1)
 *
 */
TEST_F(ParserHttp2Fixture, PseudoHeaderCaseTest){
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
	// Формируем заголовки запроса с псевдо-заголовком в верхнем регистре
	std::vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода запроса в верхнем регистре
	fields.emplace_back(":METHOD", "GET");
	// Дописываем псевдо-заголовок схемы запроса
	fields.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути запроса
	fields.emplace_back(":path", "/");
	// Отправляем заголовки запроса с завершением потока
	client->sendHeaders(sid, fields, true);
	// Проверяем что запрос с псевдо-заголовком в верхнем регистре отклонён
	ASSERT_TRUE(serverEvents.providers.empty());
	// Проверяем что поток сброшен как малформированный
	ASSERT_EQ(serverEvents.closes.size(), 1u);
	// Проверяем что соединение осталось живо
	ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL);
}

/**
 * @brief Метод проверки сверки заголовка [host] с псевдо-заголовком [:authority] (RFC 9113 §8.3.1)
 *
 */
TEST_F(ParserHttp2Fixture, AuthorityHostMismatchTest){
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
	// Формируем заголовки запроса с расходящимися [host] и [:authority]
	std::vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода запроса
	fields.emplace_back(":method", "GET");
	// Дописываем псевдо-заголовок схемы запроса
	fields.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути запроса
	fields.emplace_back(":path", "/");
	// Дописываем псевдо-заголовок авторитета запроса
	fields.emplace_back(":authority", "example.com");
	// Дописываем расходящийся с ним заголовок [host]
	fields.emplace_back("host", "evil.example.net");
	// Отправляем заголовки запроса с завершением потока
	client->sendHeaders(sid, fields, true);
	// Проверяем что расходящийся запрос серверу не доставлен
	ASSERT_TRUE(serverEvents.providers.empty());
	// Проверяем что поток сброшен как малформированный
	ASSERT_EQ(serverEvents.closes.size(), 1u);
	// Проверяем что соединение осталось живо
	ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL);
}

/**
 * @brief Метод проверки живучести соединения при запоздалых фреймах на закрытом потоке (RFC 9113 §5.1)
 *
 */
TEST_F(ParserHttp2Fixture, LateFramesOnClosedStreamTest){
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
	fields.emplace_back(":path", "/");
	// Отправляем заголовки запроса с завершением потока
	client->sendHeaders(sid, fields, true);
	// Формируем финальный ответ сервера
	headers_t response(std::make_unique <response_t> (version_t::HTTP2, 200));
	// Отправляем ответ с завершением потока (после этого поток закрыт и удалён)
	server->sendHeaders(sid, response, true);
	// Проверяем что обмен завершён и ответ доставлен
	ASSERT_EQ(clientEvents.code, 200);
	// Сервер шлёт запоздалый WINDOW_UPDATE на уже закрытом потоке клиента
	server->sendWindowUpdate(sid, 1024);
	// Проверяем что запоздалый WINDOW_UPDATE не обрушил соединение
	ASSERT_EQ(client->status(), parser_t::status_t::PARTIAL);
	// Проверяем что ошибка уровня соединения не зафиксирована
	ASSERT_FALSE(clientEvents.errorFired);
	// Сервер шлёт запоздалый RST_STREAM на том же потоке
	server->sendRstStream(sid, parser_http2_t::error_t::NO_ERROR);
	// Проверяем что запоздалый RST_STREAM не обрушил соединение
	ASSERT_EQ(client->status(), parser_t::status_t::PARTIAL);
	// Проверяем что ошибка уровня соединения не зафиксирована
	ASSERT_FALSE(clientEvents.errorFired);
}

/**
 * @brief Метод проверки того, что исключение из пользовательской функции не рушит процесс
 *
 */
TEST_F(ParserHttp2Fixture, CallbackExceptionTest){
	// Создаём объект парсера сервера
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект парсера клиента
	auto client = this->make(direct_t::RESPONSE);
	// Создаём объект сборщика событий сервера
	events_t serverEvents;
	// Подписываем сборщик событий сервера
	this->attach(* server, serverEvents);
	// Устанавливаем функцию обратного вызова заголовков, бросающую исключение
	server->on(parser_http2_t::header_callback_t([](const uint32_t, const std::string_view, const std::string_view, const parser_t::part_t) -> bool {
		// Бросаем исключение из пользовательской функции
		throw std::runtime_error("header callback failed");
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
	// Проверяем что поток сброшен, а не завершён процесс
	ASSERT_EQ(serverEvents.closes.size(), 1u);
	// Проверяем код сброса потока
	ASSERT_EQ(serverEvents.closes.front().second, parser_http2_t::error_t::CANCEL);
	// Проверяем что соединение осталось живо
	ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL);
}

/**
 * @brief Метод проверки того, что первым кадром соединения обязан быть SETTINGS (RFC 9113 §3.4)
 *
 */
TEST_F(ParserHttp2Fixture, FirstFrameMustBeSettingsTest){
	// Создаём объект парсера сервера
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект сборщика событий сервера
	events_t serverEvents;
	// Подписываем сборщик событий сервера
	this->attach(* server, serverEvents);
	// Отправляем клиентский preface без SETTINGS
	server->parse(h2::proto::PREFACE.data(), h2::proto::PREFACE.size());
	// Формируем кадр PING вместо ожидаемого SETTINGS
	const uint8_t ping[] = {
		0x00, 0x00, 0x08, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08
	};
	// Выполняем разбор кадра PING
	server->parse(ping, sizeof(ping));
	// Проверяем что зафиксирована ошибка уровня соединения
	ASSERT_TRUE(serverEvents.errorFired);
	// Проверяем код ошибки уровня соединения
	ASSERT_EQ(serverEvents.errorCode, parser_http2_t::error_t::PROTOCOL_ERROR);
	// Проверяем итоговый статус разбора
	ASSERT_EQ(server->status(), parser_t::status_t::ERROR);
}

/**
 * @brief Метод проверки сохранения признака Literal Never Indexed при перекодировании (RFC 7541 §7.1.3)
 *
 */
TEST(Http2Hpack, NeverIndexedRoundTripTest){
	// Создаём объект кодера
	h2::hpack::encoder_t encoder;
	// Создаём объект декодера
	h2::hpack::decoder_t decoder;
	// Формируем список кодируемых заголовков
	std::vector <h2::hpack::field_t> fields;
	// Дописываем обычный заголовок
	fields.emplace_back("x-plain", "public");
	// Дописываем заголовок, явно помеченный чувствительным
	fields.emplace_back("x-secret", "topsecret", true);
	// Буфер закодированного блока
	std::string block;
	// Кодируем блок заголовков
	encoder.encode(fields, block, true);
	// Список декодированных заголовков
	std::vector <h2::hpack::field_view_t> decoded;
	// Код ошибки протокола
	h2::error_t err = h2::error_t::NO_ERROR;
	// Декодируем блок заголовков
	ASSERT_EQ(decoder.decode(block, decoded, 0, err), h2::status_t::OK);
	// Проверяем количество декодированных заголовков
	ASSERT_EQ(decoded.size(), 2u);
	// Проверяем что обычный заголовок чувствительным не помечен
	ASSERT_FALSE(decoded[0].sensitive);
	// Проверяем что чувствительный заголовок распознан как never indexed
	ASSERT_TRUE(decoded[1].sensitive);
	// Проверяем что значение чувствительного заголовка не попало в динамическую таблицу
	ASSERT_EQ(decoder.table().count(), 1u);
	/**
	 * Выполняем перекодирование разобранного блока: признак обязан сохраниться,
	 * иначе значение уйдёт в динамическую таблицу следующего узла
	 */
	h2::hpack::encoder_t proxy;
	// Буфер перекодированного блока
	std::string forwarded;
	// Перекодируем декодированные заголовки
	proxy.encode(decoded, forwarded, true);
	// Создаём объект декодера следующего узла
	h2::hpack::decoder_t next;
	// Список заголовков следующего узла
	std::vector <h2::hpack::field_view_t> result;
	// Декодируем перекодированный блок
	ASSERT_EQ(next.decode(forwarded, result, 0, err), h2::status_t::OK);
	// Проверяем количество декодированных заголовков
	ASSERT_EQ(result.size(), 2u);
	// Проверяем что признак чувствительности пережил перекодирование
	ASSERT_TRUE(result[1].sensitive);
	// Проверяем что чувствительное значение не проиндексировано и на следующем узле
	ASSERT_EQ(next.table().count(), 1u);
	// Проверяем что значения заголовков переданы без искажений
	ASSERT_EQ(result[1].value, "topsecret");
}

/**
 * @brief Метод проверки сверки принятого тела с заголовком content-length (RFC 9113 §8.1.1)
 *
 */
TEST_F(ParserHttp2Fixture, ContentLengthMismatchTest){
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
	// Формируем заголовки запроса с объявленной длиной тела
	std::vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода запроса
	fields.emplace_back(":method", "POST");
	// Дописываем псевдо-заголовок схемы запроса
	fields.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути запроса
	fields.emplace_back(":path", "/upload");
	// Дописываем объявленную длину тела запроса
	fields.emplace_back("content-length", "100");
	// Отправляем заголовки запроса (тело последует)
	client->sendHeaders(sid, fields, false);
	// Проверяем что запрос доставлен серверу
	ASSERT_FALSE(serverEvents.providers.empty());
	// Формируем тело короче объявленного
	const std::string body(50, 'x');
	// Отправляем тело с завершением потока
	client->sendData(sid, body.data(), body.size(), true);
	// Проверяем что поток сброшен как малформированный
	ASSERT_EQ(serverEvents.closes.size(), 1u);
	// Проверяем код сброса потока
	ASSERT_EQ(serverEvents.closes.front().second, parser_http2_t::error_t::PROTOCOL_ERROR);
	// Проверяем что сообщение не было объявлено полностью принятым
	ASSERT_EQ(std::count_if(
		serverEvents.phases.begin(), serverEvents.phases.end(),
		[](const auto & item) noexcept -> bool {
			// Отбираем события завершения приёма всего сообщения
			return ((std::get <1> (item) == parser_t::phase_t::END) && (std::get <2> (item) == parser_t::part_t::NONE));
		}
	), 0);
	// Проверяем что соединение осталось живо
	ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL);
}

/**
 * @brief Метод проверки закрытия необработанных потоков по входящему GOAWAY (RFC 9113 §6.8)
 *
 */
TEST_F(ParserHttp2Fixture, GoawayRefusesUnprocessedStreamsTest){
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
	// Формируем заголовки запроса
	std::vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода запроса
	fields.emplace_back(":method", "GET");
	// Дописываем псевдо-заголовок схемы запроса
	fields.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути запроса
	fields.emplace_back(":path", "/");
	// Выделяем идентификатор первого потока клиента
	const uint32_t first = client->nextStreamId();
	// Открываем первый поток
	client->sendHeaders(first, fields, false);
	// Выделяем идентификатор второго потока клиента
	const uint32_t second = client->nextStreamId();
	// Открываем второй поток
	client->sendHeaders(second, fields, false);
	/**
	 * Сервер завершает соединение, объявив наибольшим обработанным первый поток:
	 * кадр собирается вручную, чтобы задать lastStreamId меньше фактически принятого
	 */
	const uint8_t goaway[] = {
		0x00, 0x00, 0x08, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, static_cast <uint8_t> (first),
		0x00, 0x00, 0x00, 0x00
	};
	// Подаём клиенту кадр завершения соединения
	client->parse(goaway, sizeof(goaway));
	// Проверяем что клиент получил GOAWAY
	ASSERT_TRUE(clientEvents.goawayFired);
	// Проверяем что необработанный поток закрыт с кодом отклонения
	ASSERT_EQ(clientEvents.closes.size(), 1u);
	// Проверяем идентификатор закрытого потока
	ASSERT_EQ(clientEvents.closes.front().first, second);
	// Проверяем код закрытия потока (запрос можно повторить на новом соединении)
	ASSERT_EQ(clientEvents.closes.front().second, parser_http2_t::error_t::REFUSED_STREAM);
	// Проверяем что новые потоки после GOAWAY не открываются
	const uint32_t third = client->nextStreamId();
	// Пытаемся открыть поток после полученного GOAWAY
	client->sendHeaders(third, fields, false);
	// Проверяем что поток не был открыт
	ASSERT_EQ(serverEvents.begins.size(), 2u);
}

/**
 * @brief Метод проверки отправки данных из функции обратного вызова закрытия по GOAWAY
 *
 * @details Отправка реентрантно запускает прокачку потоков, а та переиспользует
 *          собственный снимок идентификаторов: перебор закрываемых потоков не
 *          должен от этого разъезжаться
 *
 */
TEST_F(ParserHttp2Fixture, GoawaySendFromCloseCallbackTest){
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
	// Формируем заголовки запроса
	std::vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода запроса
	fields.emplace_back(":method", "POST");
	// Дописываем псевдо-заголовок схемы запроса
	fields.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути запроса
	fields.emplace_back(":path", "/");
	// Идентификаторы открытых потоков клиента
	std::vector <uint32_t> ids;
	/**
	 * Открываем набор потоков: снимок закрываемых потоков должен быть заметно
	 * меньше набора всех потоков, иначе перезаполнение чужого снимка незаметно
	 */
	for(size_t i = 0; i < 12; i++){
		// Выделяем идентификатор очередного потока клиента
		ids.push_back(client->nextStreamId());
		// Открываем очередной поток
		client->sendHeaders(ids.back(), fields, false);
	}
	// Признак первой отправки из функции обратного вызова
	bool once = true;
	// Устанавливаем функцию обратного вызова закрытия потока
	client->on(parser_http2_t::close_callback_t([&](const uint32_t sid, const parser_http2_t::error_t code) noexcept {
		// Собираем событие закрытия потока
		clientEvents.closes.emplace_back(sid, code);
		// Если отправка ещё не выполнялась
		if(once){
			// Сбрасываем признак первой отправки
			once = false;
			// Отправляем тело в первый поток (реентерабельная прокачка отправки)
			client->sendData(ids.front(), "hello", 5, false);
		}
	}));
	/**
	 * Сервер завершает соединение, объявив обработанными все потоки кроме двух последних
	 */
	const uint8_t goaway[] = {
		0x00, 0x00, 0x08, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, static_cast <uint8_t> (ids[9]),
		0x00, 0x00, 0x00, 0x00
	};
	// Подаём клиенту кадр завершения соединения
	client->parse(goaway, sizeof(goaway));
	// Проверяем что закрыты ровно два необработанных потока
	ASSERT_EQ(clientEvents.closes.size(), 2u);
	// Собираем идентификаторы закрытых потоков (порядок задаёт карта потоков)
	std::vector <uint32_t> closed;
	/**
	 * Выполняем перебор всех событий закрытия потоков
	 */
	for(const auto & item : clientEvents.closes){
		// Собираем идентификатор закрытого потока
		closed.push_back(item.first);
		// Проверяем код закрытия потока (запрос можно повторить на новом соединении)
		ASSERT_EQ(item.second, parser_http2_t::error_t::REFUSED_STREAM);
	}
	// Упорядочиваем идентификаторы закрытых потоков
	std::sort(closed.begin(), closed.end());
	// Проверяем идентификатор первого закрытого потока
	ASSERT_EQ(closed[0], ids[10]);
	// Проверяем идентификатор второго закрытого потока
	ASSERT_EQ(closed[1], ids[11]);
}

/**
 * @brief Метод проверки закрытия парного потока из функции обратного вызова закрытия
 *
 * @details Закрытие потока обязано быть однократным: пока запись остаётся в карте
 *          потоков, взаимные сбросы связанных потоков вызывали бы друг друга
 *          до исчерпания стека
 *
 */
TEST_F(ParserHttp2Fixture, CloseFromCloseCallbackTest){
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
	// Формируем заголовки запроса
	std::vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода запроса
	fields.emplace_back(":method", "GET");
	// Дописываем псевдо-заголовок схемы запроса
	fields.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути запроса
	fields.emplace_back(":path", "/");
	// Выделяем идентификатор первого потока клиента
	const uint32_t first = client->nextStreamId();
	// Открываем первый поток
	client->sendHeaders(first, fields, false);
	// Выделяем идентификатор второго потока клиента
	const uint32_t second = client->nextStreamId();
	// Открываем второй поток
	client->sendHeaders(second, fields, false);
	// Глубина вложенности уведомлений о закрытии
	size_t depth = 0;
	// Устанавливаем функцию обратного вызова закрытия потока
	client->on(parser_http2_t::close_callback_t([&](const uint32_t sid, const parser_http2_t::error_t code) noexcept {
		// Собираем событие закрытия потока
		clientEvents.closes.emplace_back(sid, code);
		// Если вложенность вышла за пределы разумного - прекращаем сбросы
		if(++depth > 4)
			// Выходим из функции обратного вызова
			return;
		// Сбрасываем парный поток
		client->sendRstStream(((sid == first) ? second : first), parser_http2_t::error_t::CANCEL);
	}));
	// Сбрасываем первый поток
	client->sendRstStream(first, parser_http2_t::error_t::CANCEL);
	// Проверяем что каждый поток закрыт ровно один раз
	ASSERT_EQ(clientEvents.closes.size(), 2u);
	// Проверяем идентификатор первого закрытого потока
	ASSERT_EQ(clientEvents.closes[0].first, first);
	// Проверяем идентификатор второго закрытого потока
	ASSERT_EQ(clientEvents.closes[1].first, second);
}

/**
 * @brief Метод проверки сброса парсера из функции обратного вызова ошибки соединения
 *
 * @details Приложение вправе переиспользовать парсер под новое соединение прямо
 *          из обработчика ошибки: GOAWAY прежнего соединения не должен оказаться
 *          в очереди нового - там первым обязан идти connection preface
 *
 */
TEST_F(ParserHttp2Fixture, ResetFromErrorCallbackTest){
	// Создаём объект парсера клиента (pull-модель: функция записи не установлена)
	auto client = this->make(direct_t::RESPONSE);
	// Признак полученной ошибки уровня соединения
	bool failed = false;
	// Устанавливаем функцию обратного вызова ошибки уровня соединения
	client->on(parser_http2_t::error_callback_t([&](const parser_http2_t::error_t, const std::string_view) noexcept {
		// Помечаем что ошибка получена
		failed = true;
		// Приложение переиспользует парсер под новое соединение
		client->reset();
	}));
	// Отправляем preface клиента
	client->sendPreface();
	// Подаём SETTINGS сервера на разбор
	const std::string settings = ::frame(0x04, 0x00, 0, "");
	// Выполняем разбор SETTINGS сервера
	client->parse(settings.data(), settings.size());
	// Освобождаем всю очередь исходящих данных (соединение отработало)
	client->consumePending(client->pending().size());
	// Формируем кадр PING недопустимой длины (7 байт вместо 8)
	const std::string ping = ::frame(0x06, 0x00, 0, std::string(7, '\0'));
	// Подаём некорректный кадр на разбор
	client->parse(ping.data(), ping.size());
	// Проверяем что ошибка уровня соединения зафиксирована
	ASSERT_TRUE(failed);
	// Проверяем что очередь исходящих данных пуста (GOAWAY прежнего соединения отброшен)
	ASSERT_TRUE(client->pending().empty());
	// Начинаем новое соединение
	client->sendPreface();
	// Проверяем что новое соединение начинается с magic-строки preface
	ASSERT_EQ(client->pending().compare(0, h2::proto::PREFACE.size(), h2::proto::PREFACE), 0);
}

/**
 * @brief Метод проверки сброса парсера из функции обратного вызова открытия потока
 *
 * @details Сборка блока заголовков после сброса наполнила бы динамическую таблицу
 *          HPACK нового соединения записями, которых пир не присылал
 *
 */
TEST_F(ParserHttp2Fixture, ResetFromBeginCallbackTest){
	// Создаём объект парсера сервера (pull-модель: функция записи не установлена)
	auto server = this->make(direct_t::REQUEST);
	// Идентификаторы открытых пиром потоков
	std::vector <uint32_t> begins;
	// Устанавливаем функцию обратного вызова открытия нового потока
	server->on(parser_http2_t::begin_callback_t([&](const uint32_t sid) noexcept -> bool {
		// Собираем идентификатор открытого потока
		begins.push_back(sid);
		// Приложение переиспользует парсер под новое соединение
		server->reset();
		// Поток не принимаем
		return false;
	}));
	// Отправляем preface сервера
	server->sendPreface();
	// Освобождаем всю очередь исходящих данных
	server->consumePending(server->pending().size());
	// Буфер входящего потока клиента
	std::string input;
	// Дописываем magic-строку preface клиента
	input.append(h2::proto::PREFACE.data(), h2::proto::PREFACE.size());
	// Дописываем SETTINGS клиента
	input += ::frame(0x04, 0x00, 0, "");
	// Создаём объект кодера заголовков клиента
	h2::hpack::encoder_t encoder;
	// Формируем заголовки запроса
	std::vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода запроса
	fields.emplace_back(":method", "GET");
	// Дописываем псевдо-заголовок схемы запроса
	fields.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути запроса
	fields.emplace_back(":path", "/");
	// Дописываем псевдо-заголовок авторитета запроса
	fields.emplace_back(":authority", "example.com");
	// Буфер закодированного блока заголовков
	std::string block;
	// Кодируем блок заголовков запроса
	encoder.encode(fields, block, false);
	// Дописываем кадр заголовков запроса
	input += ::frame(0x01, 0x04, 1, block);
	// Подаём входящий поток клиента на разбор
	server->parse(input.data(), input.size());
	// Проверяем что поток был открыт ровно один раз
	ASSERT_EQ(begins.size(), 1u);
	// Проверяем что очередь исходящих данных пуста (кадры прежнего соединения отброшены)
	ASSERT_TRUE(server->pending().empty());
	// Начинаем новое соединение
	server->sendPreface();
	// Проверяем что новое соединение начинается с кадра SETTINGS
	ASSERT_EQ(static_cast <uint8_t> (server->pending()[3]), 0x04);
}

/**
 * @brief Метод проверки сброса парсера из функции обратного вызова тела потока
 *
 * @details Учёт окна приёма относится к прежнему соединению: после сброса
 *          WINDOW_UPDATE не должен попасть в очередь нового
 *
 */
TEST_F(ParserHttp2Fixture, ResetFromDataCallbackTest){
	// Создаём объект парсера клиента (pull-модель: функция записи не установлена)
	auto client = this->make(direct_t::RESPONSE);
	// Получаем параметры SETTINGS парсера
	auto settings = client->settings();
	// Поднимаем анонсируемый максимальный размер кадра
	settings.maxFrameSize = 40000;
	// Применяем параметры SETTINGS парсера
	client->settings(settings);
	// Суммарный объём принятого тела
	size_t received = 0;
	// Устанавливаем функцию обратного вызова тела потока
	client->on(parser_http2_t::data_callback_t([&](const uint32_t, const void *, const size_t size, const bool) noexcept -> bool {
		// Накапливаем объём принятого тела
		received += size;
		// Приложение переиспользует парсер под новое соединение
		client->reset();
		// Продолжаем разбор
		return true;
	}));
	// Отправляем preface клиента
	client->sendPreface();
	// Подаём SETTINGS сервера на разбор
	const std::string peer = ::frame(0x04, 0x00, 0, "");
	// Выполняем разбор SETTINGS сервера
	client->parse(peer.data(), peer.size());
	// Формируем заголовки запроса
	std::vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода запроса
	fields.emplace_back(":method", "GET");
	// Дописываем псевдо-заголовок схемы запроса
	fields.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути запроса
	fields.emplace_back(":path", "/");
	// Выделяем идентификатор нового потока клиента
	const uint32_t sid = client->nextStreamId();
	// Отправляем заголовки запроса с завершением потока
	client->sendHeaders(sid, fields, true);
	// Освобождаем всю очередь исходящих данных
	client->consumePending(client->pending().size());
	// Создаём объект кодера заголовков сервера
	h2::hpack::encoder_t encoder;
	// Формируем заголовки ответа сервера
	std::vector <h2::hpack::field_t> response;
	// Дописываем псевдо-заголовок статуса ответа
	response.emplace_back(":status", "200");
	// Буфер закодированного блока заголовков
	std::string block;
	// Кодируем блок заголовков ответа
	encoder.encode(response, block, false);
	// Формируем кадр заголовков ответа сервера
	const std::string headers = ::frame(0x01, 0x04, sid, block);
	// Подаём клиенту заголовки ответа сервера
	client->parse(headers.data(), headers.size());
	// Формируем крупный кадр тела ответа
	const std::string data = ::frame(0x00, 0x00, sid, std::string(40000, 'x'));
	// Подаём клиенту кадр тела ответа
	client->parse(data.data(), data.size());
	// Проверяем что тело доставлено приложению
	ASSERT_EQ(received, 40000u);
	// Проверяем что очередь исходящих данных пуста (WINDOW_UPDATE прежнего соединения отброшен)
	ASSERT_TRUE(client->pending().empty());
	// Начинаем новое соединение
	client->sendPreface();
	// Проверяем что новое соединение начинается с magic-строки preface
	ASSERT_EQ(client->pending().compare(0, h2::proto::PREFACE.size(), h2::proto::PREFACE), 0);
}

/**
 * @brief Метод проверки лимита частоты кадров приоритета
 *
 * @details Кадры приоритета состояния не меняют, но обрабатываются, поэтому их
 *          поток ограничен отдельным лимитом
 *
 */
TEST_F(ParserHttp2Fixture, PriorityFloodProtectionTest){
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
	// Формируем строгий лимит частоты кадров приоритета
	parser_http2_t::limits_t limits;
	// Ограничиваем стартовый запас лимита частоты кадров приоритета
	limits.prioLimitBurst = 3;
	// Ограничиваем пополнение лимита частоты кадров приоритета
	limits.prioLimitRate = 1;
	// Применяем лимиты безопасности сервера
	server->limits(limits);
	// Соединяем парсеры каналами записи
	this->connect(* client, * server);
	// Выполняем рукопожатие соединения
	this->handshake(* client, * server);
	/**
	 * Выполняем поток кадров обновления приоритета
	 */
	for(size_t i = 0; i < 5; ++i){
		// Формируем полезную нагрузку кадра обновления приоритета
		std::string payload;
		// Дописываем идентификатор приоритизируемого потока
		payload.push_back(0x00);
		payload.push_back(0x00);
		payload.push_back(0x00);
		payload.push_back(static_cast <char> (1 + (2 * i)));
		// Дописываем значение поля приоритета
		payload += "u=3";
		// Формируем кадр обновления приоритета
		const std::string current = ::frame(0x10, 0x00, 0, payload);
		// Подаём кадр обновления приоритета серверу
		server->parse(current.data(), current.size());
	}
	// Проверяем что сервер зафиксировал ошибку уровня соединения
	ASSERT_EQ(server->status(), parser_t::status_t::ERROR);
	// Проверяем код ошибки уровня соединения
	ASSERT_EQ(server->error(), parser_http2_t::error_t::ENHANCE_YOUR_CALM);
	// Проверяем что функция обратного вызова ошибки вызвана
	ASSERT_TRUE(serverEvents.errorFired);
}

/**
 * @brief Метод проверки штатного потока кадров приоритета при лимитах по умолчанию
 *
 * @details Переустановка приоритетов на каждый загружаемый ресурс - нормальное
 *          поведение клиента и не должна обрываться защитой
 *
 */
TEST_F(ParserHttp2Fixture, PriorityBurstAllowedTest){
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
	/**
	 * Выполняем штатный поток кадров обновления приоритета
	 */
	for(size_t i = 0; i < 500; ++i){
		// Формируем полезную нагрузку кадра обновления приоритета
		std::string payload;
		// Дописываем идентификатор приоритизируемого потока
		payload.push_back(0x00);
		payload.push_back(0x00);
		payload.push_back(0x00);
		payload.push_back(0x01);
		// Дописываем значение поля приоритета
		payload += "u=5, i";
		// Формируем кадр обновления приоритета
		const std::string current = ::frame(0x10, 0x00, 0, payload);
		// Подаём кадр обновления приоритета серверу
		server->parse(current.data(), current.size());
	}
	// Проверяем что соединение осталось живо
	ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL);
	// Проверяем что ошибка уровня соединения не фиксировалась
	ASSERT_FALSE(serverEvents.errorFired);
}

/**
 * @brief Метод проверки блока заголовков на закрытом потоке (RFC 9113 §5.1)
 *
 * @details Это потоковая ошибка, а не ошибка соединения, но блок обязан быть
 *          декодирован - иначе динамическая таблица HPACK разъедется с кодером пира
 *
 */
TEST_F(ParserHttp2Fixture, HeadersOnClosedStreamTest){
	// Создаём объект парсера сервера (pull-модель: функция записи не установлена)
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект сборщика событий парсера
	events_t serverEvents;
	// Подписываем сборщик событий на все функции обратного вызова парсера
	this->attach(* server, serverEvents);
	// Отправляем preface сервера
	server->sendPreface();
	// Создаём объект кодера заголовков клиента
	h2::hpack::encoder_t encoder;
	// Буфер входящего потока клиента
	std::string input;
	// Дописываем magic-строку preface клиента
	input.append(h2::proto::PREFACE.data(), h2::proto::PREFACE.size());
	// Дописываем SETTINGS клиента
	input += ::frame(0x04, 0x00, 0, "");
	// Формируем заголовки первого запроса
	std::vector <h2::hpack::field_t> first;
	// Дописываем псевдо-заголовки первого запроса
	first.emplace_back(":method", "GET");
	first.emplace_back(":scheme", "https");
	first.emplace_back(":path", "/first");
	// Буфер закодированного блока первого запроса
	std::string block;
	// Кодируем блок первого запроса
	encoder.encode(first, block, false);
	// Дописываем кадр заголовков первого запроса
	input += ::frame(0x01, 0x04, 1, block);
	// Дописываем кадр сброса первого потока клиентом
	input += ::frame(0x03, 0x00, 1, std::string("\x00\x00\x00\x08", 4));
	// Подаём входящий поток клиента на разбор
	server->parse(input.data(), input.size());
	// Проверяем что первый поток закрыт
	ASSERT_EQ(serverEvents.closes.size(), 1u);
	// Освобождаем всю очередь исходящих данных
	server->consumePending(server->pending().size());
	// Формируем заголовки запроса на уже закрытом потоке
	std::vector <h2::hpack::field_t> closed;
	// Дописываем псевдо-заголовки запроса
	closed.emplace_back(":method", "GET");
	closed.emplace_back(":scheme", "https");
	closed.emplace_back(":path", "/closed");
	// Дописываем заголовок, попадающий в динамическую таблицу
	closed.emplace_back("x-marker", "value");
	// Очищаем буфер закодированного блока
	block.clear();
	// Кодируем блок запроса на закрытом потоке
	encoder.encode(closed, block, false);
	// Формируем кадр заголовков на закрытом потоке
	const std::string late = ::frame(0x01, 0x04, 1, block);
	// Подаём кадр на разбор
	server->parse(late.data(), late.size());
	// Проверяем что соединение осталось живо
	ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL);
	// Проверяем что ошибка уровня соединения не фиксировалась
	ASSERT_FALSE(serverEvents.errorFired);
	// Проверяем что сервер ответил сбросом потока
	ASSERT_FALSE(server->pending().empty());
	// Проверяем что отправлен именно кадр RST_STREAM
	ASSERT_EQ(static_cast <uint8_t> (server->pending()[3]), 0x03);
	// Проверяем код сброса потока (закрытый поток)
	ASSERT_EQ(static_cast <uint8_t> (server->pending()[12]), static_cast <uint8_t> (parser_http2_t::error_t::STREAM_CLOSED));
	// Освобождаем всю очередь исходящих данных
	server->consumePending(server->pending().size());
	// Формируем заголовки следующего запроса со ссылкой на запись динамической таблицы
	std::vector <h2::hpack::field_t> next;
	// Дописываем псевдо-заголовки следующего запроса
	next.emplace_back(":method", "GET");
	next.emplace_back(":scheme", "https");
	next.emplace_back(":path", "/next");
	// Дописываем заголовок, который кодер передаст индексом
	next.emplace_back("x-marker", "value");
	// Очищаем буфер закодированного блока
	block.clear();
	// Кодируем блок следующего запроса
	encoder.encode(next, block, false);
	// Формируем кадр заголовков следующего запроса
	const std::string following = ::frame(0x01, 0x05, 3, block);
	// Подаём кадр на разбор
	server->parse(following.data(), following.size());
	// Проверяем что соединение осталось живо
	ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL);
	// Проверяем что следующий запрос доставлен
	ASSERT_FALSE(serverEvents.providers.empty());
	// Проверяем путь доставленного запроса
	ASSERT_EQ(serverEvents.uri, "/next");
	// Флаг наличия заголовка, переданного индексом
	bool hasMarker = false;
	/**
	 * Выполняем перебор всех доставленных заголовков
	 */
	for(const auto & item : serverEvents.headers){
		// Если получен заголовок, переданный индексом
		if((std::get <1> (item) == "x-marker") && (std::get <2> (item) == "value"))
			// Помечаем что заголовок разрешён по индексу
			hasMarker = true;
	}
	// Проверяем что заголовок разрешён по индексу - таблица HPACK осталась синхронной
	ASSERT_TRUE(hasMarker);
}

/**
 * @brief Метод проверки данных тела на зарезервированном потоке (RFC 9113 §5.1)
 *
 * @details Зарезервированный под push поток до блока заголовков тела не принимает,
 *          и это ошибка соединения: пир рассинхронизирован
 *
 */
TEST_F(ParserHttp2Fixture, DataOnReservedStreamRecvTest){
	// Создаём объект парсера сервера (pull-модель: функция записи не установлена)
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект сборщика событий парсера
	events_t serverEvents;
	// Подписываем сборщик событий на все функции обратного вызова парсера
	this->attach(* server, serverEvents);
	// Отправляем preface сервера
	server->sendPreface();
	// Создаём объект кодера заголовков клиента
	h2::hpack::encoder_t encoder;
	// Буфер входящего потока клиента
	std::string input;
	// Дописываем magic-строку preface клиента
	input.append(h2::proto::PREFACE.data(), h2::proto::PREFACE.size());
	// Дописываем SETTINGS клиента
	input += ::frame(0x04, 0x00, 0, "");
	// Формируем заголовки запроса клиента
	std::vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовки запроса
	fields.emplace_back(":method", "GET");
	fields.emplace_back(":scheme", "https");
	fields.emplace_back(":path", "/");
	// Буфер закодированного блока заголовков
	std::string block;
	// Кодируем блок заголовков запроса
	encoder.encode(fields, block, false);
	// Дописываем кадр заголовков запроса
	input += ::frame(0x01, 0x04, 1, block);
	// Подаём входящий поток клиента на разбор
	server->parse(input.data(), input.size());
	// Формируем заголовки обещанного запроса
	std::vector <h2::hpack::field_t> promise;
	// Дописываем псевдо-заголовки обещанного запроса
	promise.emplace_back(":method", "GET");
	promise.emplace_back(":scheme", "https");
	promise.emplace_back(":path", "/push");
	promise.emplace_back(":authority", "example.com");
	// Отправляем анонс server push
	const uint32_t pushed = server->sendPushPromise(1, promise);
	// Проверяем что push-поток зарезервирован
	ASSERT_NE(pushed, 0u);
	// Формируем кадр тела на зарезервированном потоке
	const std::string data = ::frame(0x00, 0x00, pushed, "body");
	// Подаём кадр тела на разбор
	server->parse(data.data(), data.size());
	// Проверяем что зафиксирована ошибка уровня соединения
	ASSERT_EQ(server->status(), parser_t::status_t::ERROR);
	// Проверяем код ошибки уровня соединения
	ASSERT_EQ(server->error(), parser_http2_t::error_t::PROTOCOL_ERROR);
}

/**
 * @brief Метод проверки анонса параметра разрешения server push (RFC 9113 §6.5.2)
 *
 * @details Параметром распоряжается только клиент. Сервер отправлять его не вправе:
 *          значение 1 от сервера обязывает клиента оборвать соединение
 *
 */
TEST_F(ParserHttp2Fixture, EnablePushAnnouncementTest){
	// Создаём объект парсера сервера
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект парсера клиента
	auto client = this->make(direct_t::RESPONSE);
	// Отправляем preface сервера
	server->sendPreface();
	// Отправляем preface клиента
	client->sendPreface();
	/**
	 * @brief Функция поиска параметра в кадре SETTINGS
	 *
	 * @param output очередь исходящих данных парсера
	 * @param id     искомый идентификатор параметра
	 * @return       признак наличия параметра
	 *
	 */
	auto has = [](const std::string_view output, const uint16_t id) noexcept -> bool {
		// Текущая позиция разбора очереди
		size_t pos = 0;
		// Очередь клиента начинается с magic-строки preface, а не с кадра
		if((output.size() >= h2::proto::PREFACE.size()) && (output.compare(0, h2::proto::PREFACE.size(), h2::proto::PREFACE) == 0))
			// Пропускаем magic-строку preface
			pos = h2::proto::PREFACE.size();
		/**
		 * Выполняем перебор всех кадров очереди
		 */
		while((pos + 9) <= output.size()){
			// Извлекаем длину полезной нагрузки кадра
			const size_t length = (
				(static_cast <size_t> (static_cast <uint8_t> (output[pos])) << 16) |
				(static_cast <size_t> (static_cast <uint8_t> (output[pos + 1])) << 8) |
				static_cast <size_t> (static_cast <uint8_t> (output[pos + 2]))
			);
			// Если кадр целиком не поместился - прекращаем разбор
			if((pos + 9 + length) > output.size())
				// Прекращаем разбор очереди
				break;
			// Если получен кадр параметров соединения
			if(static_cast <uint8_t> (output[pos + 3]) == 0x04){
				/**
				 * Выполняем перебор всех параметров кадра
				 */
				for(size_t i = 0; (i + 6) <= length; i += 6){
					// Извлекаем идентификатор параметра
					const uint16_t current = static_cast <uint16_t> (
						(static_cast <uint16_t> (static_cast <uint8_t> (output[pos + 9 + i])) << 8) |
						static_cast <uint16_t> (static_cast <uint8_t> (output[pos + 10 + i]))
					);
					// Если параметр найден
					if(current == id)
						// Параметр присутствует
						return true;
				}
			}
			// Сдвигаем позицию за разобранный кадр
			pos += (9 + length);
		}
		// Параметр отсутствует
		return false;
	};
	// Проверяем что сервер параметр разрешения push не анонсирует
	ASSERT_FALSE(has(server->pending(), 0x02));
	// Проверяем что клиент параметр разрешения push анонсирует
	ASSERT_TRUE(has(client->pending(), 0x02));
}

/**
 * @brief Метод проверки отклонения параметра разрешения push от сервера
 *
 * @details Клиент обязан оборвать соединение, если сервер прислал
 *          SETTINGS_ENABLE_PUSH со значением, отличным от нуля (RFC 9113 §6.5.2)
 *
 */
TEST_F(ParserHttp2Fixture, ServerEnablePushRejectedTest){
	// Создаём объект парсера клиента
	auto client = this->make(direct_t::RESPONSE);
	// Создаём объект сборщика событий парсера
	events_t clientEvents;
	// Подписываем сборщик событий на все функции обратного вызова парсера
	this->attach(* client, clientEvents);
	// Отправляем preface клиента
	client->sendPreface();
	// Формируем полезную нагрузку SETTINGS сервера с разрешением push
	std::string payload;
	// Дописываем идентификатор параметра разрешения push
	payload.push_back(0x00);
	payload.push_back(0x02);
	// Дописываем значение параметра
	payload.push_back(0x00);
	payload.push_back(0x00);
	payload.push_back(0x00);
	payload.push_back(0x01);
	// Формируем кадр SETTINGS сервера
	const std::string settings = ::frame(0x04, 0x00, 0, payload);
	// Подаём кадр SETTINGS сервера на разбор
	client->parse(settings.data(), settings.size());
	// Проверяем что зафиксирована ошибка уровня соединения
	ASSERT_EQ(client->status(), parser_t::status_t::ERROR);
	// Проверяем код ошибки уровня соединения
	ASSERT_EQ(client->error(), parser_http2_t::error_t::PROTOCOL_ERROR);
	// Проверяем что функция обратного вызова ошибки вызвана
	ASSERT_TRUE(clientEvents.errorFired);
}

/**
 * @brief Метод проверки приёма параметра разрешения push сервером
 *
 * @details Клиент вправе анонсировать параметр в любом допустимом значении -
 *          сервер обязан его принять
 *
 */
TEST_F(ParserHttp2Fixture, ClientEnablePushAcceptedTest){
	// Создаём объект парсера сервера
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект сборщика событий парсера
	events_t serverEvents;
	// Подписываем сборщик событий на все функции обратного вызова парсера
	this->attach(* server, serverEvents);
	// Отправляем preface сервера
	server->sendPreface();
	// Буфер входящего потока клиента
	std::string input;
	// Дописываем magic-строку preface клиента
	input.append(h2::proto::PREFACE.data(), h2::proto::PREFACE.size());
	// Формируем полезную нагрузку SETTINGS клиента с разрешением push
	std::string payload;
	// Дописываем идентификатор параметра разрешения push
	payload.push_back(0x00);
	payload.push_back(0x02);
	// Дописываем значение параметра
	payload.push_back(0x00);
	payload.push_back(0x00);
	payload.push_back(0x00);
	payload.push_back(0x01);
	// Дописываем кадр SETTINGS клиента
	input += ::frame(0x04, 0x00, 0, payload);
	// Подаём входящий поток клиента на разбор
	server->parse(input.data(), input.size());
	// Проверяем что соединение осталось живо
	ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL);
	// Проверяем что ошибка уровня соединения не фиксировалась
	ASSERT_FALSE(serverEvents.errorFired);
	// Проверяем что разрешение push от клиента принято
	ASSERT_EQ(server->remoteSettings().enablePush, 1u);
}

/**
 * @brief Метод проверки блока заголовков на потоке, завершённом END_STREAM
 *
 * @details В отличие от потока, оборванного RST_STREAM, здесь пир закрыл поток сам
 *          и знает об этом, поэтому блок заголовков на нём - ошибка соединения
 *          (RFC 9113 §5.1)
 *
 */
TEST_F(ParserHttp2Fixture, HeadersOnFinishedStreamTest){
	// Создаём объект парсера сервера (pull-модель: функция записи не установлена)
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект сборщика событий парсера
	events_t serverEvents;
	// Подписываем сборщик событий на все функции обратного вызова парсера
	this->attach(* server, serverEvents);
	// Устанавливаем функцию обратного вызова фазы приёма сообщения
	server->on(parser_http2_t::phase_callback_t([&](const uint32_t sid, const parser_t::phase_t phase, const parser_t::part_t part) noexcept -> bool {
		// Если приём запроса завершён полностью
		if((phase == parser_t::phase_t::END) && (part == parser_t::part_t::NONE)){
			// Формируем заголовки ответа сервера
			std::vector <h2::hpack::field_t> response;
			// Дописываем псевдо-заголовок статуса ответа
			response.emplace_back(":status", "200");
			// Отправляем ответ с завершением потока
			server->sendHeaders(sid, response, true);
		}
		// Продолжаем разбор
		return true;
	}));
	// Отправляем preface сервера
	server->sendPreface();
	// Создаём объект кодера заголовков клиента
	h2::hpack::encoder_t encoder;
	// Буфер входящего потока клиента
	std::string input;
	// Дописываем magic-строку preface клиента
	input.append(h2::proto::PREFACE.data(), h2::proto::PREFACE.size());
	// Дописываем SETTINGS клиента
	input += ::frame(0x04, 0x00, 0, "");
	// Формируем заголовки запроса
	std::vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовки запроса
	fields.emplace_back(":method", "GET");
	fields.emplace_back(":scheme", "https");
	fields.emplace_back(":path", "/");
	// Буфер закодированного блока заголовков
	std::string block;
	// Кодируем блок заголовков запроса
	encoder.encode(fields, block, false);
	// Дописываем кадр заголовков запроса с завершением потока
	input += ::frame(0x01, 0x05, 1, block);
	// Подаём входящий поток клиента на разбор
	server->parse(input.data(), input.size());
	// Проверяем что обмен по потоку завершён
	ASSERT_EQ(serverEvents.closes.size(), 1u);
	// Очищаем буфер закодированного блока
	block.clear();
	// Кодируем повторный блок заголовков
	encoder.encode(fields, block, false);
	// Формируем кадр заголовков на завершённом потоке
	const std::string late = ::frame(0x01, 0x04, 1, block);
	// Подаём кадр на разбор
	server->parse(late.data(), late.size());
	// Проверяем что зафиксирована ошибка уровня соединения
	ASSERT_EQ(server->status(), parser_t::status_t::ERROR);
	// Проверяем код ошибки уровня соединения
	ASSERT_EQ(server->error(), parser_http2_t::error_t::STREAM_CLOSED);
}

/**
 * @brief Метод проверки замкнутой на себя зависимости потока (RFC 9113 §5.3.1)
 *
 * @details Приоритеты RFC 7540 признаны устаревшими и игнорируются, но поток,
 *          объявленный зависимым от самого себя, обязан быть отвергнут
 *
 */
TEST_F(ParserHttp2Fixture, SelfDependentStreamTest){
	// Создаём объект парсера сервера (pull-модель: функция записи не установлена)
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект сборщика событий парсера
	events_t serverEvents;
	// Подписываем сборщик событий на все функции обратного вызова парсера
	this->attach(* server, serverEvents);
	// Отправляем preface сервера
	server->sendPreface();
	// Создаём объект кодера заголовков клиента
	h2::hpack::encoder_t encoder;
	// Буфер входящего потока клиента
	std::string input;
	// Дописываем magic-строку preface клиента
	input.append(h2::proto::PREFACE.data(), h2::proto::PREFACE.size());
	// Дописываем SETTINGS клиента
	input += ::frame(0x04, 0x00, 0, "");
	// Формируем заголовки запроса
	std::vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовки запроса
	fields.emplace_back(":method", "GET");
	fields.emplace_back(":scheme", "https");
	fields.emplace_back(":path", "/");
	// Буфер закодированного блока заголовков
	std::string block;
	// Кодируем блок заголовков запроса
	encoder.encode(fields, block, false);
	// Формируем полезную нагрузку с полями приоритета
	std::string payload;
	// Дописываем идентификатор потока зависимости (тот же поток)
	payload.push_back(0x00);
	payload.push_back(0x00);
	payload.push_back(0x00);
	payload.push_back(0x01);
	// Дописываем вес потока
	payload.push_back(0x10);
	// Дописываем блок заголовков
	payload += block;
	// Дописываем кадр заголовков с полями приоритета
	input += ::frame(0x01, 0x24, 1, payload);
	// Подаём входящий поток клиента на разбор
	server->parse(input.data(), input.size());
	// Проверяем что соединение осталось живо
	ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL);
	// Проверяем что заголовки приложению не доставлены
	ASSERT_TRUE(serverEvents.providers.empty());
	// Освобождаем очередь до кадра приоритета
	server->consumePending(server->pending().size());
	// Формируем полезную нагрузку кадра приоритета с зависимостью от себя
	std::string priority;
	// Дописываем идентификатор потока зависимости (тот же поток)
	priority.push_back(0x00);
	priority.push_back(0x00);
	priority.push_back(0x00);
	priority.push_back(0x03);
	// Дописываем вес потока
	priority.push_back(0x10);
	// Формируем кадр приоритета
	const std::string frame = ::frame(0x02, 0x00, 3, priority);
	// Подаём кадр приоритета на разбор
	server->parse(frame.data(), frame.size());
	// Проверяем что соединение осталось живо
	ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL);
	// Проверяем что отправлен кадр сброса потока
	ASSERT_FALSE(server->pending().empty());
	// Проверяем тип отправленного кадра
	ASSERT_EQ(static_cast <uint8_t> (server->pending()[3]), 0x03);
	// Проверяем код сброса потока
	ASSERT_EQ(static_cast <uint8_t> (server->pending()[12]), static_cast <uint8_t> (parser_http2_t::error_t::PROTOCOL_ERROR));
}

/**
 * @brief Метод проверки кадра приоритета некорректной длины на закрытом потоке
 *
 * @details Кадр приоритета допустим в любом состоянии потока, поэтому ответ
 *          на некорректную длину не зависит от того, существует ли поток
 *          (RFC 9113 §6.3)
 *
 */
TEST_F(ParserHttp2Fixture, PriorityBadLengthUnknownStreamTest){
	// Создаём объект парсера сервера (pull-модель: функция записи не установлена)
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект сборщика событий парсера
	events_t serverEvents;
	// Подписываем сборщик событий на все функции обратного вызова парсера
	this->attach(* server, serverEvents);
	// Отправляем preface сервера
	server->sendPreface();
	// Буфер входящего потока клиента
	std::string input;
	// Дописываем magic-строку preface клиента
	input.append(h2::proto::PREFACE.data(), h2::proto::PREFACE.size());
	// Дописываем SETTINGS клиента
	input += ::frame(0x04, 0x00, 0, "");
	// Подаём входящий поток клиента на разбор
	server->parse(input.data(), input.size());
	// Освобождаем всю очередь исходящих данных
	server->consumePending(server->pending().size());
	// Формируем кадр приоритета недопустимой длины на неизвестном потоке
	const std::string frame = ::frame(0x02, 0x00, 5, std::string(4, '\0'));
	// Подаём кадр приоритета на разбор
	server->parse(frame.data(), frame.size());
	// Проверяем что соединение осталось живо
	ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL);
	// Проверяем что отправлен кадр сброса потока
	ASSERT_FALSE(server->pending().empty());
	// Проверяем тип отправленного кадра
	ASSERT_EQ(static_cast <uint8_t> (server->pending()[3]), 0x03);
	// Проверяем код сброса потока
	ASSERT_EQ(static_cast <uint8_t> (server->pending()[12]), static_cast <uint8_t> (parser_http2_t::error_t::FRAME_SIZE_ERROR));
}

/**
 * @brief Метод проверки переполнения окна отправки потока (RFC 9113 §6.9.1)
 *
 * @details Переполнение окна потока обрывает только этот поток, тогда как
 *          переполнение окна соединения завершает соединение
 *
 */
TEST_F(ParserHttp2Fixture, StreamWindowOverflowTest){
	// Создаём объект парсера сервера (pull-модель: функция записи не установлена)
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект сборщика событий парсера
	events_t serverEvents;
	// Подписываем сборщик событий на все функции обратного вызова парсера
	this->attach(* server, serverEvents);
	// Отправляем preface сервера
	server->sendPreface();
	// Создаём объект кодера заголовков клиента
	h2::hpack::encoder_t encoder;
	// Буфер входящего потока клиента
	std::string input;
	// Дописываем magic-строку preface клиента
	input.append(h2::proto::PREFACE.data(), h2::proto::PREFACE.size());
	// Дописываем SETTINGS клиента
	input += ::frame(0x04, 0x00, 0, "");
	// Формируем заголовки запроса
	std::vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовки запроса
	fields.emplace_back(":method", "GET");
	fields.emplace_back(":scheme", "https");
	fields.emplace_back(":path", "/");
	// Буфер закодированного блока заголовков
	std::string block;
	// Кодируем блок заголовков запроса
	encoder.encode(fields, block, false);
	// Дописываем кадр заголовков запроса (поток остаётся открытым)
	input += ::frame(0x01, 0x04, 1, block);
	// Подаём входящий поток клиента на разбор
	server->parse(input.data(), input.size());
	// Освобождаем всю очередь исходящих данных
	server->consumePending(server->pending().size());
	// Формируем инкремент, переполняющий окно потока
	std::string payload;
	// Дописываем максимально возможный инкремент окна
	payload.push_back(0x7F);
	payload.push_back(0xFF);
	payload.push_back(0xFF);
	payload.push_back(0xFF);
	// Формируем кадр обновления окна потока
	const std::string window = ::frame(0x08, 0x00, 1, payload);
	// Подаём кадр обновления окна на разбор
	server->parse(window.data(), window.size());
	// Проверяем что соединение осталось живо
	ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL);
	// Проверяем что поток сброшен
	ASSERT_EQ(serverEvents.closes.size(), 1u);
	// Проверяем код сброса потока
	ASSERT_EQ(serverEvents.closes.front().second, parser_http2_t::error_t::FLOW_CONTROL_ERROR);
	// Проверяем что отправлен кадр сброса потока
	ASSERT_FALSE(server->pending().empty());
	// Проверяем тип отправленного кадра
	ASSERT_EQ(static_cast <uint8_t> (server->pending()[3]), 0x03);
}

/**
 * @brief Метод проверки расширенного метода CONNECT (RFC 8441)
 *
 */
TEST_F(ParserHttp2Fixture, ExtendedConnectTest){
	// Создаём объект парсера сервера
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект парсера клиента
	auto client = this->make(direct_t::RESPONSE);
	// Получаем параметры SETTINGS сервера
	auto settings = server->settings();
	// Разрешаем расширенный метод CONNECT
	settings.enableConnectProtocol = 1;
	// Применяем параметры SETTINGS сервера
	server->settings(settings);
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
	// Проверяем что клиент получил разрешение на расширенный CONNECT
	ASSERT_EQ(client->remoteSettings().enableConnectProtocol, 1u);
	// Формируем провайдер запроса расширенного CONNECT
	auto request = std::make_unique <request_t> (version_t::HTTP2, method_t::CONNECT, "/chat");
	// Устанавливаем протокол поднимаемого туннеля
	request->protocol = "websocket";
	// Формируем контейнер заголовков запроса
	headers_t headers(std::move(request));
	// Дописываем заголовок авторитета запроса
	headers.emplace("Host", "example.com");
	// Выделяем идентификатор нового потока клиента
	const uint32_t sid = client->nextStreamId();
	// Отправляем запрос расширенного CONNECT (туннель остаётся открытым)
	client->sendHeaders(sid, headers, false);
	// Проверяем что запрос доставлен серверу
	ASSERT_FALSE(serverEvents.providers.empty());
	// Проверяем что метод запроса распознан как CONNECT
	ASSERT_EQ(serverEvents.method, method_t::CONNECT);
	// Проверяем что путь запроса доставлен (расширенный CONNECT его требует)
	ASSERT_EQ(serverEvents.uri, "/chat");
	// Проверяем что соединение живо
	ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL);
	// Флаг наличия псевдо-заголовка протокола
	bool hasProtocol = false;
	/**
	 * Выполняем перебор всех доставленных серверу заголовков
	 */
	for(const auto & item : serverEvents.headers){
		// Если получен псевдо-заголовок протокола туннеля
		if(std::get <1> (item) == ":protocol"){
			// Помечаем что псевдо-заголовок получен
			hasProtocol = true;
			// Проверяем значение протокола туннеля
			ASSERT_EQ(std::get <2> (item), "websocket");
		}
	}
	// Проверяем что псевдо-заголовок протокола доставлен
	ASSERT_TRUE(hasProtocol);
}

/**
 * @brief Метод проверки отказа от отправки расширенного CONNECT без разрешения пира
 *
 * @details Клиент не вправе отправлять псевдо-заголовок [:protocol], пока сервер
 *          не анонсировал SETTINGS_ENABLE_CONNECT_PROTOCOL (RFC 8441 §3)
 *
 */
TEST_F(ParserHttp2Fixture, ExtendedConnectNotAnnouncedTest){
	// Создаём объект парсера сервера (расширенный CONNECT не разрешён)
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
	// Проверяем что расширенный CONNECT сервером не анонсирован
	ASSERT_EQ(client->remoteSettings().enableConnectProtocol, 0u);
	// Формируем провайдер запроса расширенного CONNECT
	auto request = std::make_unique <request_t> (version_t::HTTP2, method_t::CONNECT, "/chat");
	// Устанавливаем протокол поднимаемого туннеля
	request->protocol = "websocket";
	// Формируем контейнер заголовков запроса
	headers_t headers(std::move(request));
	// Дописываем заголовок авторитета запроса
	headers.emplace("Host", "example.com");
	// Выделяем идентификатор нового потока клиента
	const uint32_t sid = client->nextStreamId();
	// Пытаемся отправить запрос расширенного CONNECT
	client->sendHeaders(sid, headers, false);
	// Проверяем что запрос не отправлен (поток на сервере не открыт)
	ASSERT_TRUE(serverEvents.begins.empty());
	// Проверяем что заголовки серверу не доставлены
	ASSERT_TRUE(serverEvents.providers.empty());
	// Проверяем что соединение клиента живо
	ASSERT_EQ(client->status(), parser_t::status_t::PARTIAL);
	// Проверяем что соединение сервера живо
	ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL);
}

/**
 * @brief Метод проверки отправки блока заголовков сверх лимита списка пира
 *
 * @details SETTINGS_MAX_HEADER_LIST_SIZE носит рекомендательный характер
 *          (RFC 9113 §6.5.2): отправку не блокируем, но пир вправе отвергнуть
 *          такой блок - именно это и должно быть видно приложению
 *
 */
TEST_F(ParserHttp2Fixture, PeerHeaderListLimitTest){
	// Создаём объект парсера сервера
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект парсера клиента
	auto client = this->make(direct_t::RESPONSE);
	// Получаем параметры SETTINGS сервера
	auto settings = server->settings();
	// Анонсируем строгий лимит списка заголовков
	settings.maxHeaderListSize = 200;
	// Применяем параметры SETTINGS сервера
	server->settings(settings);
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
	// Проверяем что клиент принял анонсированный сервером лимит
	ASSERT_EQ(client->remoteSettings().maxHeaderListSize, 200u);
	// Формируем заголовки запроса
	std::vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода запроса
	fields.emplace_back(":method", "GET");
	// Дописываем псевдо-заголовок схемы запроса
	fields.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути запроса
	fields.emplace_back(":path", "/");
	// Дописываем заголовок, выводящий список за анонсированный лимит
	fields.emplace_back("x-large", std::string(512, 'v'));
	// Выделяем идентификатор нового потока клиента
	const uint32_t sid = client->nextStreamId();
	// Отправляем запрос с завершением потока
	client->sendHeaders(sid, fields, true);
	// Проверяем что отправка не заблокирована - поток на сервере открыт
	ASSERT_EQ(serverEvents.begins.size(), 1u);
	// Проверяем что сервер отверг блок сверх собственного лимита
	ASSERT_TRUE(serverEvents.providers.empty());
	// Проверяем что поток сброшен
	ASSERT_EQ(serverEvents.closes.size(), 1u);
	// Проверяем код сброса потока
	ASSERT_EQ(serverEvents.closes.front().second, parser_http2_t::error_t::ENHANCE_YOUR_CALM);
	/**
	 * Проверяем что соединение осталось живо: раздутые заголовки одного сообщения
	 * не вправе уносить с собой остальные потоки соединения
	 */
	ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL);
	// Проверяем что клиент не получил GOAWAY
	ASSERT_FALSE(clientEvents.goawayFired);
	// Формируем заголовки следующего запроса
	std::vector <h2::hpack::field_t> next;
	// Дописываем псевдо-заголовок метода запроса
	next.emplace_back(":method", "GET");
	// Дописываем псевдо-заголовок схемы запроса
	next.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути запроса
	next.emplace_back(":path", "/next");
	// Выделяем идентификатор следующего потока клиента
	const uint32_t following = client->nextStreamId();
	// Отправляем следующий запрос по тому же соединению
	client->sendHeaders(following, next, true);
	// Проверяем что следующий запрос доставлен - состояние HPACK не разъехалось
	ASSERT_FALSE(serverEvents.providers.empty());
	// Проверяем путь доставленного запроса
	ASSERT_EQ(serverEvents.uri, "/next");
}

/**
 * @brief Метод проверки отклонения расширенного CONNECT без разрешения сервера (RFC 8441 §3)
 *
 */
TEST_F(ParserHttp2Fixture, ExtendedConnectDeniedTest){
	// Создаём объект парсера сервера (расширенный CONNECT не разрешён)
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
	// Проверяем что расширенный CONNECT сервером не анонсирован
	ASSERT_EQ(client->remoteSettings().enableConnectProtocol, 0u);
	// Выделяем идентификатор нового потока клиента
	const uint32_t sid = client->nextStreamId();
	// Формируем заголовки запроса расширенного CONNECT вручную
	std::vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода запроса
	fields.emplace_back(":method", "CONNECT");
	// Дописываем псевдо-заголовок схемы запроса
	fields.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути запроса
	fields.emplace_back(":path", "/chat");
	// Дописываем псевдо-заголовок авторитета запроса
	fields.emplace_back(":authority", "example.com");
	// Дописываем псевдо-заголовок протокола туннеля
	fields.emplace_back(":protocol", "websocket");
	// Отправляем запрос расширенного CONNECT
	client->sendHeaders(sid, fields, false);
	// Проверяем что запрос отклонён как малформированный
	ASSERT_TRUE(serverEvents.providers.empty());
	// Проверяем что поток сброшен
	ASSERT_EQ(serverEvents.closes.size(), 1u);
	// Проверяем что соединение осталось живо
	ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL);
}

/**
 * @brief Метод проверки отклонения CONNECT с пустым авторитетом (RFC 9113 §8.5)
 *
 */
TEST_F(ParserHttp2Fixture, ConnectEmptyAuthorityTest){
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
	// Формируем заголовки запроса CONNECT вручную
	std::vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода запроса
	fields.emplace_back(":method", "CONNECT");
	// Дописываем псевдо-заголовок авторитета запроса с пустым значением
	fields.emplace_back(":authority", "");
	// Отправляем запрос туннеля без адресата
	client->sendHeaders(sid, fields, false);
	// Проверяем что запрос отклонён как малформированный
	ASSERT_TRUE(serverEvents.providers.empty());
	// Проверяем что поток сброшен
	ASSERT_EQ(serverEvents.closes.size(), 1u);
	// Проверяем что соединение осталось живо
	ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL);
}

/**
 * @brief Метод проверки запрещённых заголовков в секции трейлеров (RFC 9110 §6.5.1)
 *
 */
TEST_F(ParserHttp2Fixture, ForbiddenTrailerTest){
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
	// Отправляем заголовки запроса (тело последует)
	client->sendHeaders(sid, fields, false);
	// Формируем тело запроса
	const std::string body(16, 'x');
	// Отправляем тело без завершения потока (завершат трейлеры)
	client->sendData(sid, body.data(), body.size(), false);
	// Формируем секцию трейлеров с запрещённым в ней заголовком
	std::vector <h2::hpack::field_t> trailers;
	// Дописываем запрещённый в трейлерах заголовок
	trailers.emplace_back("content-length", "16");
	// Отправляем трейлеры с завершением потока
	client->sendHeaders(sid, trailers, true);
	// Проверяем что поток сброшен как малформированный
	ASSERT_EQ(serverEvents.closes.size(), 1u);
	// Проверяем код сброса потока
	ASSERT_EQ(serverEvents.closes.front().second, parser_http2_t::error_t::PROTOCOL_ERROR);
	// Проверяем что соединение осталось живо
	ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL);
}

/**
 * @brief Метод проверки плавного завершения соединения двухфазным GOAWAY (RFC 9113 §6.8)
 *
 */
TEST_F(ParserHttp2Fixture, GracefulShutdownTest){
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
	// Формируем заголовки запроса
	std::vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода запроса
	fields.emplace_back(":method", "GET");
	// Дописываем псевдо-заголовок схемы запроса
	fields.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути запроса
	fields.emplace_back(":path", "/early");
	// Выделяем идентификатор потока, открытого до предупреждения
	const uint32_t early = client->nextStreamId();
	// Открываем поток до объявления о завершении (тело последует)
	client->sendHeaders(early, fields, false);
	// Сервер объявляет о предстоящем завершении соединения
	server->sendShutdown();
	// Проверяем что клиент получил предупреждение
	ASSERT_TRUE(clientEvents.goawayFired);
	// Проверяем что предупреждение не отклоняет ни одного потока (RFC 9113 §6.8)
	ASSERT_EQ(clientEvents.goawayLast, 0x7FFFFFFFu);
	// Проверяем что ни один из открытых потоков не закрыт
	ASSERT_TRUE(clientEvents.closes.empty());
	// Проверяем что предупреждение не помечает соединение завершаемым для сервера
	ASSERT_FALSE(server->isClosed());
	// Формируем тело уже открытого потока
	const std::string body(64, 'x');
	// Проверяем что начатый до предупреждения поток продолжает работать
	ASSERT_EQ(client->sendData(early, body.data(), body.size(), true), body.size());
	// Проверяем что тело доставлено серверу
	ASSERT_EQ(serverEvents.bodies[early].size(), body.size());
	// Выделяем идентификатор нового потока
	const uint32_t late = client->nextStreamId();
	// Пытаемся открыть новый поток после предупреждения
	client->sendHeaders(late, fields, true);
	// Проверяем что новый поток не открыт: предупреждение запрещает новые запросы
	ASSERT_EQ(serverEvents.begins.size(), 1u);
	// Сервер завершает соединение второй фазой
	server->sendGoaway(parser_http2_t::error_t::NO_ERROR);
	// Проверяем что соединение помечено завершаемым
	ASSERT_TRUE(server->isClosed());
	// Проверяем что клиент получил фактический наибольший обработанный поток
	ASSERT_EQ(clientEvents.goawayLast, early);
}

/**
 * @brief Метод проверки расширенных приоритетов потоков (RFC 9218)
 *
 */
TEST_F(ParserHttp2Fixture, ExtensiblePriorityTest){
	// Создаём объект парсера сервера
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект парсера клиента
	auto client = this->make(direct_t::RESPONSE);
	// Получаем параметры SETTINGS клиента
	auto settings = client->settings();
	/**
	 * Закрываем начальное окно приёма клиента: сервер сможет поставить тела обоих
	 * потоков в очередь, но не отправит их, пока окно не будет открыто. Это позволяет
	 * проверить именно порядок планирования, а не порядок постановки в очередь
	 */
	settings.windowSize = 0;
	// Применяем параметры SETTINGS клиента
	client->settings(settings);
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
	// Проверяем что отказ от приоритетов RFC 7540 согласован обеими сторонами
	ASSERT_EQ(client->remoteSettings().noRfc7540Priorities, 1u);
	// Проверяем согласование на стороне сервера
	ASSERT_EQ(server->remoteSettings().noRfc7540Priorities, 1u);
	// Порядок, в котором сервер отдаёт тела потоков
	std::vector <uint32_t> order;
	// Устанавливаем функцию обратного вызова тела на клиенте для сбора порядка отдачи
	client->on(parser_http2_t::data_callback_t([&order, &clientEvents](const uint32_t sid, const void * buffer, const size_t size, const bool) noexcept -> bool {
		// Если поток ещё не отмечен последним в порядке отдачи
		if(order.empty() || (order.back() != sid))
			// Запоминаем поток, отдающий данные
			order.push_back(sid);
		// Собираем фрагмент тела потока (функция обратного вызова фикстуры замещена)
		clientEvents.bodies[sid].append(static_cast <const char *> (buffer), size);
		// Продолжаем разбор
		return true;
	}));
	// Формируем заголовки запроса
	std::vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода запроса
	fields.emplace_back(":method", "GET");
	// Дописываем псевдо-заголовок схемы запроса
	fields.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути запроса
	fields.emplace_back(":path", "/data");
	// Выделяем идентификатор первого потока клиента (срочность по умолчанию)
	const uint32_t first = client->nextStreamId();
	// Открываем первый поток
	client->sendHeaders(first, fields, true);
	// Формируем заголовки запроса повышенной срочности
	std::vector <h2::hpack::field_t> urgent = fields;
	// Дописываем заголовок расширенного приоритета (RFC 9218 §5)
	urgent.emplace_back("priority", "u=0");
	// Выделяем идентификатор второго потока клиента
	const uint32_t second = client->nextStreamId();
	// Открываем второй поток с повышенной срочностью
	client->sendHeaders(second, urgent, true);
	// Формируем заголовки ответа первого потока
	headers_t first_response(std::make_unique <response_t> (version_t::HTTP2, 200));
	// Отправляем заголовки ответа первого потока
	server->sendHeaders(first, first_response, false);
	// Формируем заголовки ответа второго потока
	headers_t second_response(std::make_unique <response_t> (version_t::HTTP2, 200));
	// Отправляем заголовки ответа второго потока
	server->sendHeaders(second, second_response, false);
	// Формируем тело ответа
	const std::string body(8 * 1024, 'z');
	// Ставим тело в очередь отправки первого потока (менее срочный)
	server->sendData(first, body.data(), body.size(), true);
	// Ставим тело в очередь отправки второго потока (более срочный)
	server->sendData(second, body.data(), body.size(), true);
	// Проверяем что при закрытом окне приёма ни один поток данных не отдал
	ASSERT_TRUE(order.empty());
	// Открываем начальное окно приёма клиента
	settings.windowSize = 65535;
	// Применяем параметры SETTINGS клиента
	client->settings(settings);
	// Отправляем обновлённые параметры: сервер сдвинет окна обоих потоков и прокачает отправку
	client->sendSettings();
	// Проверяем что оба потока получили данные
	ASSERT_EQ(order.size(), 2u);
	// Проверяем что более срочный поток обслужен первым
	ASSERT_EQ(order.front(), second);
	// Проверяем что тела обоих потоков доставлены полностью
	ASSERT_EQ(clientEvents.bodies[first].size(), body.size());
	// Проверяем размер тела более срочного потока
	ASSERT_EQ(clientEvents.bodies[second].size(), body.size());
}

/**
 * @brief Метод проверки безтелесных ответов с объявленным content-length (RFC 9110 §8.6, §9.3.2)
 *
 */
TEST_F(ParserHttp2Fixture, BodylessContentLengthTest){
	/**
	 * Выполняем проверку для запроса методом HEAD и для ответа со статусом 304:
	 * оба объявляют длину тела, которого не будет
	 */
	for(int variant = 0; variant < 2; ++variant){
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
		fields.emplace_back(":method", ((variant == 0) ? "HEAD" : "GET"));
		// Дописываем псевдо-заголовок схемы запроса
		fields.emplace_back(":scheme", "https");
		// Дописываем псевдо-заголовок пути запроса
		fields.emplace_back(":path", "/resource");
		// Отправляем запрос с завершением потока
		client->sendHeaders(sid, fields, true);
		// Формируем заголовки ответа с объявленной длиной отсутствующего тела
		std::vector <h2::hpack::field_t> response;
		// Дописываем псевдо-заголовок статуса ответа
		response.emplace_back(":status", ((variant == 0) ? "200" : "304"));
		// Дописываем объявленную длину тела
		response.emplace_back("content-length", "4096");
		// Отправляем ответ с завершением потока и без тела
		server->sendHeaders(sid, response, true);
		// Проверяем что ответ доставлен приложению, а не отброшен как малформированный
		ASSERT_FALSE(clientEvents.providers.empty()) << "variant: " << variant;
		// Проверяем что поток не сброшен
		ASSERT_TRUE(clientEvents.closes.empty() || (clientEvents.closes.front().second == parser_http2_t::error_t::NO_ERROR));
		// Проверяем что соединение осталось живо
		ASSERT_EQ(client->status(), parser_t::status_t::PARTIAL) << "variant: " << variant;
	}
}

/**
 * @brief Метод проверки того, что SETTINGS ACK не закрывает требование connection preface (RFC 9113 §3.4)
 *
 */
TEST_F(ParserHttp2Fixture, SettingsAckIsNotPrefaceTest){
	// Создаём объект парсера сервера
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект сборщика событий сервера
	events_t serverEvents;
	// Подписываем сборщик событий сервера
	this->attach(* server, serverEvents);
	// Отправляем клиентский preface
	server->parse(h2::proto::PREFACE.data(), h2::proto::PREFACE.size());
	// Формируем пустой кадр SETTINGS с флагом подтверждения
	const uint8_t ack[] = {0x00, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x00, 0x00};
	// Подтверждение допустимо само по себе и ошибкой не является
	server->parse(ack, sizeof(ack));
	// Проверяем что подтверждение соединение не обрушило
	ASSERT_FALSE(serverEvents.errorFired);
	// Формируем кадр PING вместо ожидаемого объявления параметров
	const uint8_t ping[] = {
		0x00, 0x00, 0x08, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08
	};
	// Выполняем разбор кадра PING
	server->parse(ping, sizeof(ping));
	// Проверяем что содержательный кадр до объявления параметров отвергнут
	ASSERT_TRUE(serverEvents.errorFired);
	// Проверяем код ошибки уровня соединения
	ASSERT_EQ(serverEvents.errorCode, parser_http2_t::error_t::PROTOCOL_ERROR);
}

/**
 * @brief Метод проверки отклонения тела до финального блока заголовков (RFC 9113 §8.1)
 *
 */
TEST_F(ParserHttp2Fixture, DataBeforeFinalHeadersTest){
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
	fields.emplace_back(":path", "/hints");
	// Отправляем запрос (поток остаётся открытым)
	client->sendHeaders(sid, fields, false);
	// Формируем промежуточный информационный ответ
	headers_t interim(std::make_unique <response_t> (version_t::HTTP2, 103));
	// Отправляем информационный ответ без завершения потока
	server->sendHeaders(sid, interim, false);
	// Проверяем что промежуточный ответ доставлен
	ASSERT_EQ(clientEvents.code, 103);
	// Формируем тело, отправляемое до финального блока заголовков
	const std::string body(32, 'x');
	// Сервер отправляет тело, не прислав финальных заголовков
	server->sendData(sid, body.data(), body.size(), false);
	// Проверяем что тело клиенту не доставлено
	ASSERT_TRUE(clientEvents.bodies[sid].empty());
	// Проверяем что поток сброшен как малформированный
	ASSERT_EQ(clientEvents.closes.size(), 1u);
	// Проверяем код сброса потока
	ASSERT_EQ(clientEvents.closes.front().second, parser_http2_t::error_t::PROTOCOL_ERROR);
	// Проверяем что соединение осталось живо
	ASSERT_EQ(client->status(), parser_t::status_t::PARTIAL);
}

/**
 * @brief Метод проверки приведения некорректных параметров SETTINGS к допустимому диапазону
 *
 */
TEST_F(ParserHttp2Fixture, InvalidLocalSettingsTest){
	// Создаём объект парсера сервера
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект парсера клиента
	auto client = this->make(direct_t::RESPONSE);
	// Получаем параметры SETTINGS сервера
	auto settings = server->settings();
	// Задаём отрицательное начальное окно потока
	settings.windowSize = -1;
	// Задаём размер фрейма ниже допустимого протоколом
	settings.maxFrameSize = 1024;
	// Задаём недопустимое значение разрешения server push
	settings.enablePush = 5;
	// Применяем некорректные параметры SETTINGS
	server->settings(settings);
	// Проверяем что начальное окно потока приведено к значению по умолчанию
	ASSERT_EQ(server->settings().windowSize, 65535);
	// Проверяем что размер фрейма приведён к значению по умолчанию
	ASSERT_EQ(server->settings().maxFrameSize, 16384u);
	// Проверяем что разрешение server push приведено к значению по умолчанию
	ASSERT_EQ(server->settings().enablePush, 1u);
	// Создаём объект сборщика событий клиента
	events_t clientEvents;
	// Подписываем сборщик событий клиента
	this->attach(* client, clientEvents);
	// Соединяем парсеры каналами записи
	this->connect(* client, * server);
	// Выполняем рукопожатие соединения
	this->handshake(* client, * server);
	// Проверяем что соединение установлено, а не оборвано на некорректном параметре
	ASSERT_FALSE(clientEvents.errorFired);
	// Проверяем что клиент принял приведённые параметры
	ASSERT_EQ(client->remoteSettings().maxFrameSize, 16384u);
}

/**
 * @brief Метод проверки безопасного сброса парсера из пользовательской функции обратного вызова
 *
 */
TEST_F(ParserHttp2Fixture, ResetFromCallbackTest){
	/**
	 * Выполняем проверку сброса из разных обработчиков: заголовков и тела.
	 * Каждый из них вызывается посреди разбора, когда парсер ещё перебирает
	 * собственные списки и снимки
	 */
	for(int variant = 0; variant < 2; ++variant){
		// Создаём объект парсера сервера
		auto server = this->make(direct_t::REQUEST);
		// Создаём объект сборщика событий сервера
		events_t serverEvents;
		// Подписываем сборщик событий сервера
		this->attach(* server, serverEvents);
		// Получаем сырой указатель на парсер для использования в функциях обратного вызова
		parser_http2_t * parser = server.get();
		// Если проверяется сброс из обработчика заголовков
		if(variant == 0){
			// Устанавливаем функцию обратного вызова заголовков, сбрасывающую парсер
			server->on(parser_http2_t::header_callback_t([parser](const uint32_t, const std::string_view, const std::string_view, const parser_t::part_t) noexcept -> bool {
				// Сбрасываем состояние соединения прямо из обработчика
				parser->reset();
				// Продолжаем разбор
				return true;
			}));
		// Если проверяется сброс из обработчика тела
		} else {
			// Устанавливаем функцию обратного вызова тела, очищающую парсер
			server->on(parser_http2_t::data_callback_t([parser](const uint32_t, const void *, const size_t, const bool) noexcept -> bool {
				// Выполняем полную очистку парсера прямо из обработчика
				parser->clear();
				// Продолжаем разбор
				return true;
			}));
		}
		// Отправляем свой preface
		server->sendPreface();
		// Формируем входящий поток байт
		std::string input(h2::proto::PREFACE);
		// Дописываем пустой кадр SETTINGS клиента
		const uint8_t settings[] = {0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00};
		// Дописываем кадр SETTINGS во входящий поток
		input.append(reinterpret_cast <const char *> (settings), sizeof(settings));
		// Создаём объект кодера заголовков
		h2::hpack::encoder_t encoder;
		// Формируем заголовки запроса
		std::vector <h2::hpack::field_t> fields;
		// Дописываем псевдо-заголовок метода запроса
		fields.emplace_back(":method", "POST");
		// Дописываем псевдо-заголовок схемы запроса
		fields.emplace_back(":scheme", "https");
		// Дописываем псевдо-заголовок пути запроса
		fields.emplace_back(":path", "/reset");
		// Буфер закодированного блока заголовков
		std::string block;
		// Кодируем блок заголовков
		encoder.encode(fields, block, false);
		// Формируем заголовок кадра HEADERS
		std::string frame;
		// Дописываем длину полезной нагрузки кадра
		frame.push_back(static_cast <char> ((block.size() >> 16) & 0xFF));
		// Дописываем средний байт длины
		frame.push_back(static_cast <char> ((block.size() >> 8) & 0xFF));
		// Дописываем младший байт длины
		frame.push_back(static_cast <char> (block.size() & 0xFF));
		// Дописываем тип кадра HEADERS
		frame.push_back(0x01);
		// Дописываем флаг завершения блока заголовков
		frame.push_back(0x04);
		// Дописываем идентификатор потока
		frame.append("\x00\x00\x00\x01", 4);
		// Дописываем блок заголовков
		frame.append(block);
		// Дописываем кадр HEADERS во входящий поток
		input.append(frame);
		// Формируем кадр тела запроса
		const uint8_t data[] = {
			0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 'b', 'o', 'd', 'y'
		};
		// Дописываем кадр тела во входящий поток
		input.append(reinterpret_cast <const char *> (data), sizeof(data));
		// Дописываем ещё два кадра PING, идущих после точки сброса
		const uint8_t ping[] = {
			0x00, 0x00, 0x08, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08
		};
		// Дописываем первый кадр PING
		input.append(reinterpret_cast <const char *> (ping), sizeof(ping));
		// Дописываем второй кадр PING
		input.append(reinterpret_cast <const char *> (ping), sizeof(ping));
		// Выполняем разбор всего потока одной порцией
		server->parse(input.data(), input.size());
		/**
		 * Сброс посреди разбора обязан свернуть его безопасно: устаревшие байты
		 * не должны быть разобраны как кадры уже нового соединения
		 */
		ASSERT_FALSE(serverEvents.errorFired) << "variant: " << variant << ", error: " << serverEvents.errorMessage;
		// Проверяем что парсер остался работоспособен
		ASSERT_NE(server->status(), parser_t::status_t::ERROR) << "variant: " << variant;
		// Проверяем что состояние соединения действительно сброшено
		ASSERT_EQ(server->error(), parser_http2_t::error_t::NO_ERROR) << "variant: " << variant;
	}
}

/**
 * @brief Метод проверки отклонения PRIORITY_UPDATE с нулевым приоритизируемым потоком (RFC 9218 §7.1)
 *
 */
TEST_F(ParserHttp2Fixture, PriorityUpdateZeroStreamTest){
	/**
	 * Проверяем обе роли: по чётности идентификатора ноль неотличим от потока,
	 * инициированного сервером, поэтому у клиента дефект проявляется отдельно
	 */
	for(int variant = 0; variant < 2; ++variant){
		// Создаём объект парсера проверяемой роли
		auto parser = this->make((variant == 0) ? direct_t::REQUEST : direct_t::RESPONSE);
		// Создаём объект сборщика событий парсера
		events_t events;
		// Подписываем сборщик событий парсера
		this->attach(* parser, events);
		// Отправляем свой preface
		parser->sendPreface();
		// Если проверяется сервер - подаём клиентский preface
		if(variant == 0)
			// Выполняем разбор клиентского preface
			parser->parse(h2::proto::PREFACE.data(), h2::proto::PREFACE.size());
		// Формируем пустой кадр SETTINGS пира
		const uint8_t settings[] = {0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00};
		// Выполняем разбор кадра SETTINGS
		parser->parse(settings, sizeof(settings));
		// Формируем кадр PRIORITY_UPDATE с нулевым приоритизируемым потоком
		const uint8_t update[] = {
			0x00, 0x00, 0x07, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 'u', '=', '0'
		};
		// Выполняем разбор кадра PRIORITY_UPDATE
		parser->parse(update, sizeof(update));
		// Проверяем что зафиксирована ошибка уровня соединения
		ASSERT_TRUE(events.errorFired) << "variant: " << variant;
		// Проверяем код ошибки уровня соединения
		ASSERT_EQ(events.errorCode, parser_http2_t::error_t::PROTOCOL_ERROR) << "variant: " << variant;
	}
}

/**
 * @brief Метод проверки отклонения PUSH_PROMISE на недопустимом ассоциированном потоке (RFC 9113 §6.6)
 *
 */
TEST_F(ParserHttp2Fixture, PushPromiseInvalidAssocTest){
	// Создаём объект парсера клиента
	auto client = this->make(direct_t::RESPONSE);
	// Создаём объект сборщика событий клиента
	events_t clientEvents;
	// Подписываем сборщик событий клиента
	this->attach(* client, clientEvents);
	// Отправляем свой preface
	client->sendPreface();
	// Формируем пустой кадр SETTINGS сервера
	const uint8_t settings[] = {0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00};
	// Выполняем разбор кадра SETTINGS
	client->parse(settings, sizeof(settings));
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
	// Отправляем запрос с завершением потока
	client->sendHeaders(sid, fields, true);
	// Создаём объект кодера заголовков сервера
	h2::hpack::encoder_t encoder;
	// Формируем заголовки ответа сервера
	std::vector <h2::hpack::field_t> response;
	// Дописываем псевдо-заголовок статуса ответа
	response.emplace_back(":status", "200");
	// Буфер закодированного блока заголовков ответа
	std::string block;
	// Кодируем блок заголовков ответа
	encoder.encode(response, block, false);
	// Формируем кадр HEADERS ответа без завершения потока
	std::string frame;
	// Дописываем длину полезной нагрузки кадра
	frame.push_back(static_cast <char> ((block.size() >> 16) & 0xFF));
	// Дописываем средний байт длины
	frame.push_back(static_cast <char> ((block.size() >> 8) & 0xFF));
	// Дописываем младший байт длины
	frame.push_back(static_cast <char> (block.size() & 0xFF));
	// Дописываем тип кадра HEADERS
	frame.push_back(0x01);
	// Дописываем флаги завершения блока заголовков и потока
	frame.push_back(0x05);
	// Дописываем идентификатор потока
	frame.append("\x00\x00\x00\x01", 4);
	// Дописываем блок заголовков
	frame.append(block);
	// Выполняем разбор ответа: после него поток закрыт с обеих сторон
	client->parse(frame.data(), frame.size());
	// Проверяем что ответ доставлен
	ASSERT_EQ(clientEvents.code, 200);
	// Формируем блок заголовков обещанного запроса
	std::vector <h2::hpack::field_t> promised;
	// Дописываем псевдо-заголовок метода обещанного запроса
	promised.emplace_back(":method", "GET");
	// Дописываем псевдо-заголовок схемы обещанного запроса
	promised.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути обещанного запроса
	promised.emplace_back(":path", "/push");
	// Дописываем псевдо-заголовок авторитета обещанного запроса
	promised.emplace_back(":authority", "example.com");
	// Буфер закодированного блока обещанного запроса
	std::string promise;
	// Кодируем блок заголовков обещанного запроса
	encoder.encode(promised, promise, false);
	// Формируем кадр PUSH_PROMISE на уже отработавшем потоке
	std::string push;
	// Вычисляем длину полезной нагрузки кадра с учётом идентификатора обещанного потока
	const size_t length = (promise.size() + 4);
	// Дописываем длину полезной нагрузки кадра
	push.push_back(static_cast <char> ((length >> 16) & 0xFF));
	// Дописываем средний байт длины
	push.push_back(static_cast <char> ((length >> 8) & 0xFF));
	// Дописываем младший байт длины
	push.push_back(static_cast <char> (length & 0xFF));
	// Дописываем тип кадра PUSH_PROMISE
	push.push_back(0x05);
	// Дописываем флаг завершения блока заголовков
	push.push_back(0x04);
	// Дописываем идентификатор ассоциированного потока
	push.append("\x00\x00\x00\x01", 4);
	// Дописываем идентификатор обещанного потока
	push.append("\x00\x00\x00\x02", 4);
	// Дописываем блок заголовков обещанного запроса
	push.append(promise);
	// Выполняем разбор кадра PUSH_PROMISE
	client->parse(push.data(), push.size());
	// Проверяем что промис на недопустимом потоке отвергнут
	ASSERT_TRUE(clientEvents.errorFired);
	// Проверяем код ошибки уровня соединения
	ASSERT_EQ(clientEvents.errorCode, parser_http2_t::error_t::PROTOCOL_ERROR);
	// Проверяем что анонс push приложению не доставлен
	ASSERT_TRUE(clientEvents.pushes.empty());
}

/**
 * @brief Метод проверки отклонения тела в сообщении, которое его нести не может
 *
 */
TEST_F(ParserHttp2Fixture, BodyInBodylessMessageTest){
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
	// Формируем заголовки запроса методом HEAD
	std::vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода запроса
	fields.emplace_back(":method", "HEAD");
	// Дописываем псевдо-заголовок схемы запроса
	fields.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути запроса
	fields.emplace_back(":path", "/resource");
	// Отправляем запрос с завершением потока
	client->sendHeaders(sid, fields, true);
	// Формируем заголовки ответа сервера
	headers_t response(std::make_unique <response_t> (version_t::HTTP2, 200));
	// Отправляем заголовки ответа без завершения потока
	server->sendHeaders(sid, response, false);
	// Проверяем что ответ доставлен
	ASSERT_EQ(clientEvents.code, 200);
	// Формируем тело, недопустимое в ответе на HEAD
	const std::string body(32, 'x');
	// Сервер ошибочно отправляет тело
	server->sendData(sid, body.data(), body.size(), true);
	// Проверяем что тело приложению не доставлено
	ASSERT_TRUE(clientEvents.bodies[sid].empty());
	// Проверяем что поток сброшен как малформированный
	ASSERT_EQ(clientEvents.closes.size(), 1u);
	// Проверяем код сброса потока
	ASSERT_EQ(clientEvents.closes.front().second, parser_http2_t::error_t::PROTOCOL_ERROR);
	// Проверяем что соединение осталось живо
	ASSERT_EQ(client->status(), parser_t::status_t::PARTIAL);
}

/**
 * @brief Метод проверки нулевого инкремента WINDOW_UPDATE на потоке и на соединении (RFC 9113 §6.9)
 *
 */
TEST_F(ParserHttp2Fixture, ZeroWindowUpdateTest){
	/**
	 * Нулевой инкремент на потоке - потоковая ошибка, на соединении - ошибка
	 * уровня соединения: проверяем обе ветки
	 */
	for(int variant = 0; variant < 2; ++variant){
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
		fields.emplace_back(":path", "/");
		// Открываем поток (тело последует)
		client->sendHeaders(sid, fields, false);
		// Определяем идентификатор потока проверяемого кадра
		const uint8_t target = ((variant == 0) ? static_cast <uint8_t> (sid) : 0x00);
		// Формируем кадр WINDOW_UPDATE с нулевым инкрементом
		const uint8_t update[] = {
			0x00, 0x00, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, target,
			0x00, 0x00, 0x00, 0x00
		};
		// Выполняем разбор кадра WINDOW_UPDATE на сервере
		server->parse(update, sizeof(update));
		// Если проверяется нулевой инкремент на потоке
		if(variant == 0){
			// Проверяем что соединение осталось живо
			ASSERT_FALSE(serverEvents.errorFired);
			// Проверяем что поток сброшен
			ASSERT_EQ(serverEvents.closes.size(), 1u);
			// Проверяем код сброса потока
			ASSERT_EQ(serverEvents.closes.front().second, parser_http2_t::error_t::PROTOCOL_ERROR);
		// Если проверяется нулевой инкремент на соединении
		} else {
			// Проверяем что зафиксирована ошибка уровня соединения
			ASSERT_TRUE(serverEvents.errorFired);
			// Проверяем код ошибки уровня соединения
			ASSERT_EQ(serverEvents.errorCode, parser_http2_t::error_t::PROTOCOL_ERROR);
		}
	}
}

/**
 * @brief Метод проверки отклонения Dynamic Table Size Update не в начале блока (RFC 7541 §4.2)
 *
 */
TEST(Http2Hpack, SizeUpdateNotAtBlockStartTest){
	// Создаём объект кодера
	h2::hpack::encoder_t encoder;
	// Создаём объект декодера
	h2::hpack::decoder_t decoder;
	// Формируем список кодируемых заголовков
	std::vector <h2::hpack::field_t> fields;
	// Дописываем обычный заголовок
	fields.emplace_back("x-first", "1");
	// Буфер закодированного блока
	std::string block;
	// Кодируем блок заголовков
	encoder.encode(fields, block, false);
	// Дописываем Dynamic Table Size Update в середину блока (паттерн 001xxxxx)
	block.push_back(static_cast <char> (0x20));
	// Формируем второй список кодируемых заголовков
	std::vector <h2::hpack::field_t> tail;
	// Дописываем ещё один заголовок после запрещённого обновления
	tail.emplace_back("x-second", "2");
	// Буфер второй части блока
	std::string rest;
	// Кодируем вторую часть блока
	encoder.encode(tail, rest, false);
	// Дописываем вторую часть блока
	block.append(rest);
	// Список декодированных заголовков
	std::vector <h2::hpack::field_view_t> decoded;
	// Код ошибки протокола
	h2::error_t err = h2::error_t::NO_ERROR;
	// Проверяем что блок отвергнут
	ASSERT_EQ(decoder.decode(block, decoded, 0, err), h2::status_t::ERROR);
	// Проверяем что зафиксирована ошибка состояния HPACK
	ASSERT_EQ(err, h2::error_t::COMPRESSION_ERROR);
}

/**
 * @brief Метод проверки сигнализации серии изменений размера таблицы (RFC 7541 §4.2)
 *
 */
TEST(Http2Hpack, TableSizeSeriesTest){
	// Создаём объект кодера
	h2::hpack::encoder_t encoder;
	// Уменьшаем размер таблицы
	encoder.setMaxTableSize(0);
	// Возвращаем размер таблицы обратно
	encoder.setMaxTableSize(4096);
	// Буфер закодированного блока
	std::string block;
	// Формируем список кодируемых заголовков
	std::vector <h2::hpack::field_t> fields;
	// Дописываем заголовок
	fields.emplace_back("x-a", "1");
	// Кодируем блок заголовков
	encoder.encode(fields, block, false);
	/**
	 * Кодер обязан сигнализировать наименьший размер серии и только затем итоговый:
	 * первый байт - update со значением 0, следом - update со значением 4096
	 */
	ASSERT_GE(block.size(), 4u);
	// Проверяем что первым идёт обновление наименьшего размера серии
	ASSERT_EQ(static_cast <uint8_t> (block[0]), 0x20);
	// Проверяем что следом идёт обновление итогового размера
	ASSERT_EQ(static_cast <uint8_t> (block[1]), 0x3F);
	// Создаём объект декодера
	h2::hpack::decoder_t decoder;
	// Список декодированных заголовков
	std::vector <h2::hpack::field_view_t> decoded;
	// Код ошибки протокола
	h2::error_t err = h2::error_t::NO_ERROR;
	// Проверяем что декодер принимает серию обновлений в начале блока
	ASSERT_EQ(decoder.decode(block, decoded, 0, err), h2::status_t::OK);
	// Проверяем что заголовок декодирован
	ASSERT_EQ(decoded.size(), 1u);
	// Проверяем название декодированного заголовка
	ASSERT_EQ(decoded.front().name, "x-a");
}

/**
 * @brief Метод проверки адаптивной индексации: разовые значения в таблицу не попадают
 *
 * @details Заголовок с разовым значением, попав в динамическую таблицу, вытесняет
 *          из неё повторяющиеся заголовки - на следующем блоке их приходится
 *          передавать литералами заново, и таблица входит в постоянное вытеснение
 *          сама себя. Кодер обязан заносить в таблицу только то, что уже встречалось
 *
 */
/**
 * @brief Метод проверки полноты индекса статической таблицы
 *
 * @details Индекс статической таблицы ключуется хешем названия и хранит диапазон
 *          записей с этим названием. Столкновение хешей при построении потеряло бы
 *          запись молча: полное совпадение перестало бы находиться, а заголовок
 *          кодировался бы литералом вместо одного октета
 *
 */
TEST(Http2Hpack, StaticIndexCoverageTest){
	/**
	 * Выполняем перебор всех записей статической таблицы
	 */
	for(size_t i = 1; i <= h2::hpack::STATIC_TABLE_SIZE; i++){
		// Получаем очередную запись статической таблицы
		const h2::hpack::static_entry_t * entry = h2::hpack::staticTable(i);
		// Проверяем что запись получена
		ASSERT_NE(entry, nullptr);
		// Записи без значения индексом полного совпадения не кодируются
		if(entry->value.empty())
			// Переходим к следующей записи
			continue;
		// Создаём чистый кодер на каждую запись
		h2::hpack::encoder_t encoder;
		// Отключаем определение чувствительных заголовков
		encoder.sensitiveHeuristic(false);
		// Буфер закодированного блока
		std::string block;
		// Формируем список кодируемых заголовков
		std::vector <h2::hpack::field_t> fields;
		// Дописываем запись статической таблицы как есть
		fields.emplace_back(std::string(entry->name), std::string(entry->value));
		// Кодируем блок заголовков
		encoder.encode(fields, block, true);
		/**
		 * Полное совпадение обязано кодироваться одним октетом с индексом:
		 * старший бит 1 (RFC 7541 §6.1), значение префикса - номер записи
		 */
		ASSERT_EQ(block.size(), 1u);
		ASSERT_EQ((static_cast <uint8_t> (block[0]) & 0x80), 0x80);
		ASSERT_EQ((static_cast <uint8_t> (block[0]) & 0x7F), i);
	}
}

/**
 * @brief Метод сверки индексов динамической таблицы с полным перебором
 *
 * @details Таблица индексирована дважды: по хешу пары название-значение для полного
 *          совпадения и по хешу названия для ссылки только на название. Оба индекса
 *          обязаны давать в точности тот же результат, что и перебор записей,
 *          при любом чередовании вставок и вытеснений
 *
 */
TEST(Http2Hpack, DynamicTableIndexTest){
	// Названия с повторами: в таблице должны копиться одноимённые записи
	const char * names[] = {":path", "cookie", "set-cookie", "x-a", "x-b", "user-agent", "etag"};
	// Создаём объект кодера ради доступа к его таблице
	h2::hpack::encoder_t encoder;
	// Получаем динамическую таблицу кодера
	h2::hpack::dynamic_table_t & table = encoder.table();
	// Состояние генератора псевдослучайной последовательности
	uint64_t state = 20260726;
	/**
	 * Выполняем чередование вставок и сверок
	 */
	for(size_t round = 0; round < 2000; round++){
		// Продвигаем состояние генератора
		state = ((state * 6364136223846793005ULL) + 1442695040888963407ULL);
		// Добавляем в таблицу очередную запись
		table.add(names[(state >> 33) % 7], ("v-" + std::to_string((state >> 13) % 400)));
		// Продвигаем состояние генератора
		state = ((state * 6364136223846793005ULL) + 1442695040888963407ULL);
		// Формируем название искомого заголовка
		const std::string name = names[(state >> 33) % 7];
		// Формируем значение искомого заголовка
		const std::string value = ("v-" + std::to_string((state >> 13) % 400));
		// Индекс совпадения по названию, полученный индексом
		uint64_t indexName = 0;
		// Выполняем поиск по индексу
		const uint64_t found = table.find(name, value, indexName);
		// Индекс полного совпадения, полученный перебором
		uint64_t bruteFull = 0;
		// Индекс совпадения по названию, полученный перебором
		uint64_t bruteName = 0;
		/**
		 * Выполняем поиск полным перебором записей таблицы
		 */
		for(size_t i = 1; i <= table.count(); i++){
			// Получаем очередную запись таблицы
			const h2::hpack::field_t * entry = table.at(i);
			// Если запись не получена либо название не совпало
			if((entry == nullptr) || (entry->name != name))
				// Переходим к следующей записи
				continue;
			// Запоминаем первое совпадение по названию
			if(bruteName == 0)
				// Запоминаем индекс совпадения по названию
				bruteName = i;
			// Если значение совпало и полное совпадение ещё не найдено
			if((bruteFull == 0) && (entry->value == value))
				// Запоминаем индекс полного совпадения
				bruteFull = i;
		}
		// Проверяем совпадение полного поиска с перебором
		ASSERT_EQ(found, bruteFull);
		// Проверяем совпадение поиска по названию с перебором
		ASSERT_EQ(indexName, bruteName);
	}
	// Проверяем что вытеснение действительно происходило
	ASSERT_LT(table.count(), 2000u);
}

TEST(Http2Hpack, AdaptiveIndexingTest){
	// Создаём объект кодера
	h2::hpack::encoder_t encoder;
	// Отключаем определение чувствительных заголовков: оно к проверке отношения не имеет
	encoder.sensitiveHeuristic(false);
	// Буфер закодированного блока
	std::string block;
	/**
	 * Кодируем сотню заголовков с уникальными значениями: этого заведомо хватает,
	 * чтобы кольцо истории заполнилось целиком и прогрев закончился
	 */
	for(size_t i = 0; i < 100; i++){
		// Очищаем буфер закодированного блока
		block.clear();
		// Формируем список кодируемых заголовков
		std::vector <h2::hpack::field_t> fields;
		// Дописываем заголовок с уникальным значением
		fields.emplace_back("x-request-id", ("id-" + std::to_string(i)));
		// Кодируем блок заголовков
		encoder.encode(fields, block, true);
	}
	// Очищаем буфер закодированного блока
	block.clear();
	// Формируем список с ещё одним разовым значением
	std::vector <h2::hpack::field_t> fresh;
	// Дописываем заголовок с новым уникальным значением
	fresh.emplace_back("x-request-id", "id-fresh");
	// Кодируем блок заголовков
	encoder.encode(fresh, block, true);
	// Проверяем что блок не пустой
	ASSERT_FALSE(block.empty());
	/**
	 * Проверяем выбранное представление по старшим битам первого байта:
	 * 01xxxxxx - Literal with Incremental Indexing (RFC 7541 §6.2.1), заголовок
	 * заносится в таблицу; всё остальное - представления без занесения.
	 * Разовое значение после прогрева индексироваться не должно
	 */
	ASSERT_NE((static_cast <uint8_t> (block[0]) & 0xC0), 0x40);
	// Создаём объект кодера для повторяющегося значения
	h2::hpack::encoder_t repeated;
	// Отключаем определение чувствительных заголовков
	repeated.sensitiveHeuristic(false);
	/**
	 * Кодируем двести заголовков с одним и тем же значением
	 */
	for(size_t i = 0; i < 200; i++){
		// Очищаем буфер закодированного блока
		block.clear();
		// Формируем список кодируемых заголовков
		std::vector <h2::hpack::field_t> fields;
		// Дописываем заголовок с постоянным значением
		fields.emplace_back("x-request-id", "constant");
		// Кодируем блок заголовков
		repeated.encode(fields, block, true);
	}
	// Повторяющееся значение обязано попасть в таблицу ровно одной записью
	ASSERT_EQ(repeated.table().count(), 1u);
	/**
	 * Проверяем что выключение режима возвращает индексацию всего подряд:
	 * тот же поток разовых значений обязан заполнить таблицу
	 */
	h2::hpack::encoder_t plain;
	// Отключаем определение чувствительных заголовков
	plain.sensitiveHeuristic(false);
	// Отключаем адаптивную индексацию
	plain.adaptiveIndexing(false);
	/**
	 * Кодируем сотню заголовков с уникальными значениями
	 */
	for(size_t i = 0; i < 100; i++){
		// Очищаем буфер закодированного блока
		block.clear();
		// Формируем список кодируемых заголовков
		std::vector <h2::hpack::field_t> fields;
		// Дописываем заголовок с уникальным значением
		fields.emplace_back("x-request-id", ("id-" + std::to_string(i)));
		// Кодируем блок заголовков
		plain.encode(fields, block, true);
	}
	// Очищаем буфер закодированного блока
	block.clear();
	// Кодируем блок заголовков с новым уникальным значением
	plain.encode(fresh, block, true);
	// Проверяем что блок не пустой
	ASSERT_FALSE(block.empty());
	// Без адаптивной индексации разовое значение обязано заноситься в таблицу
	ASSERT_EQ((static_cast <uint8_t> (block[0]) & 0xC0), 0x40);
	// Таблица при этом обязана быть заполнена заметно плотнее
	ASSERT_GT(plain.table().count(), encoder.table().count());
}

/**
 * @brief Метод проверки согласованности решения об индексации с представлением
 *
 * @details Декодер пира добавляет запись в динамическую таблицу по представлению:
 *          Literal with Incremental Indexing добавляет, Literal without Indexing нет.
 *          Если кодер выберет представление, не соответствующее своему решению,
 *          таблицы разъедутся и все последующие индексы станут указывать не туда
 *
 */
TEST(Http2Hpack, AdaptiveIndexingSyncTest){
	// Создаём объект кодера
	h2::hpack::encoder_t encoder;
	// Отключаем определение чувствительных заголовков
	encoder.sensitiveHeuristic(false);
	// Создаём объект декодера
	h2::hpack::decoder_t decoder;
	// Список декодированных заголовков
	std::vector <h2::hpack::field_view_t> decoded;
	// Код ошибки протокола
	h2::error_t err = h2::error_t::NO_ERROR;
	/**
	 * Прогоняем поток блоков через пару кодер-декодер: часть значений уникальна,
	 * часть повторяется, поэтому кодер выберет оба представления
	 */
	for(size_t i = 0; i < 300; i++){
		// Буфер закодированного блока
		std::string block;
		// Формируем список кодируемых заголовков
		std::vector <h2::hpack::field_t> fields;
		// Дописываем заголовок с постоянным значением
		fields.emplace_back("x-constant", "value");
		// Дописываем заголовок с разовым значением
		fields.emplace_back("x-unique", ("id-" + std::to_string(i)));
		// Кодируем блок заголовков
		encoder.encode(fields, block, true);
		// Проверяем что блок декодирован без ошибок
		ASSERT_EQ(decoder.decode(block, decoded, 0, err), h2::status_t::OK);
		// Проверяем что декодированы оба заголовка
		ASSERT_EQ(decoded.size(), 2u);
		// Проверяем значение постоянного заголовка
		ASSERT_EQ(decoded[0].value, "value");
		// Проверяем значение разового заголовка
		ASSERT_EQ(decoded[1].value, ("id-" + std::to_string(i)));
		/**
		 * Таблицы кодера и декодера обязаны совпадать по числу записей: расхождение
		 * означает, что представление разошлось с решением об индексации
		 */
		ASSERT_EQ(encoder.table().count(), decoder.table().count());
	}
}

/**
 * @brief Метод проверки порядка секции трейлеров относительно недоотправленного тела
 *
 */
TEST_F(ParserHttp2Fixture, TrailersAfterPendingBodyTest){
	// Создаём объект парсера сервера
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект парсера клиента
	auto client = this->make(direct_t::RESPONSE);
	// Получаем параметры SETTINGS клиента
	auto settings = client->settings();
	/**
	 * Закрываем начальное окно приёма клиента: тело ответа осядет в буфере отправки
	 * сервера, и секция трейлеров окажется готова раньше, чем данные уйдут в сеть
	 */
	settings.windowSize = 0;
	// Применяем параметры SETTINGS клиента
	client->settings(settings);
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
	fields.emplace_back(":path", "/stream");
	// Отправляем запрос с завершением потока
	client->sendHeaders(sid, fields, true);
	// Формируем заголовки ответа сервера
	headers_t response(std::make_unique <response_t> (version_t::HTTP2, 200));
	// Отправляем заголовки ответа без завершения потока
	server->sendHeaders(sid, response, false);
	// Формируем тело ответа
	const std::string body(4096, 'x');
	// Передаём тело: окно закрыто, поэтому данные осядут в буфере отправки
	ASSERT_EQ(server->sendData(sid, body.data(), body.size(), false), body.size());
	// Проверяем что тело клиенту ещё не доставлено
	ASSERT_TRUE(clientEvents.bodies[sid].empty());
	// Формируем секцию трейлеров
	std::vector <h2::hpack::field_t> trailers;
	// Дописываем заголовок секции трейлеров
	trailers.emplace_back("x-checksum", "deadbeef");
	// Отправляем секцию трейлеров с завершением потока
	server->sendHeaders(sid, trailers, true);
	// Проверяем что трейлеры не обогнали тело и клиенту пока не доставлены
	ASSERT_TRUE(clientEvents.headers.empty() || (std::get <3> (clientEvents.headers.back()) != parser_t::part_t::TRAILER));
	// Открываем начальное окно приёма клиента
	settings.windowSize = 65535;
	// Применяем параметры SETTINGS клиента
	client->settings(settings);
	// Отправляем обновлённые параметры: сервер дошлёт тело, а затем трейлеры
	client->sendSettings();
	// Проверяем что тело доставлено полностью
	ASSERT_EQ(clientEvents.bodies[sid].size(), body.size());
	// Проверяем что секция трейлеров доставлена
	ASSERT_FALSE(clientEvents.headers.empty());
	// Проверяем что последним доставлен именно трейлер
	ASSERT_EQ(std::get <3> (clientEvents.headers.back()), parser_t::part_t::TRAILER);
	// Проверяем название доставленного трейлера
	ASSERT_EQ(std::get <1> (clientEvents.headers.back()), "x-checksum");
	// Проверяем что приём сообщения завершён
	ASSERT_EQ(std::get <1> (clientEvents.phases.back()), parser_t::phase_t::END);
	// Проверяем что завершено именно сообщение целиком
	ASSERT_EQ(std::get <2> (clientEvents.phases.back()), parser_t::part_t::NONE);
	// Проверяем что соединение живо
	ASSERT_EQ(client->status(), parser_t::status_t::PARTIAL);
	// Проверяем статус сервера
	ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL);
}

/**
 * @brief Метод проверки запрета тела в потоке, не готовом его принимать (RFC 9113 §5.1)
 *
 */
TEST_F(ParserHttp2Fixture, DataOnReservedStreamTest){
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
	fields.emplace_back(":path", "/index.html");
	// Отправляем запрос без завершения потока
	client->sendHeaders(sid, fields, false);
	// Формируем заголовки обещанного запроса
	std::vector <h2::hpack::field_t> promise;
	// Дописываем псевдо-заголовок метода обещанного запроса
	promise.emplace_back(":method", "GET");
	// Дописываем псевдо-заголовок схемы обещанного запроса
	promise.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок пути обещанного запроса
	promise.emplace_back(":path", "/style.css");
	// Дописываем псевдо-заголовок авторитета обещанного запроса
	promise.emplace_back(":authority", "example.com");
	// Анонсируем server push
	const uint32_t pushed = server->sendPushPromise(sid, promise);
	// Проверяем что push-поток зарезервирован
	ASSERT_NE(pushed, 0u);
	// Формируем тело обещанного ответа
	const std::string body(64, 'z');
	/**
	 * Приложение ошибается порядком: тело раньше заголовков ответа. Поток
	 * зарезервирован, но ещё не открыт, поэтому данные приниматься не должны
	 */
	ASSERT_EQ(server->sendData(pushed, body.data(), body.size(), true), 0u);
	// Проверяем что клиенту тело не доставлено
	ASSERT_TRUE(clientEvents.bodies[pushed].empty());
	// Проверяем что соединение осталось живо
	ASSERT_EQ(client->status(), parser_t::status_t::PARTIAL);
	// Формируем заголовки обещанного ответа
	headers_t response(std::make_unique <response_t> (version_t::HTTP2, 200));
	// Отправляем заголовки обещанного ответа
	server->sendHeaders(pushed, response, false);
	// После открытия потока тело принимается штатно
	ASSERT_EQ(server->sendData(pushed, body.data(), body.size(), true), body.size());
	// Проверяем что тело доставлено клиенту
	ASSERT_EQ(clientEvents.bodies[pushed].size(), body.size());
}
