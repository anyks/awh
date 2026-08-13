/**
 * @file: encoding.cpp
 * @date: 2026-08-13
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Проверки приведения исходного текста CSV к кодировке UTF-8 — определение
 *        кодировки по метке порядка байтов, чтение и запись знаков Юникода, независимость
 *        приведения от нарезки текста на куски
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <gtest/gtest.h>
#include <codec/csv/csv.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;
using namespace awh::codec;

/**
 * @brief Метод приведения текста к кодировке UTF-8 кусками заданного размера
 *
 * @param text  приводимый исходный текст
 * @param chunk размер куска, каким подаётся исходный текст
 * @param error код ошибки приведения
 * @return      приведённый текст в кодировке UTF-8
 *
 */
static string convert(const string & text, const size_t chunk, csv::error_t & error) noexcept {
	// Результат приведения исходного текста
	string result = "";
	// Объект приведения исходного текста
	csv::decoder_t decoder;
	/**
	 * Выполняем подачу исходного текста кусками заданного размера
	 */
	for(size_t offset = 0; offset < text.size(); offset += chunk){
		// Получаем размер очередного подаваемого куска
		const size_t size = ((offset + chunk) < text.size() ? chunk : (text.size() - offset));
		/**
		 * Выполняем приведение очередного куска исходного текста
		 */
		if(!decoder.convert(text.data() + offset, size, ((offset + size) >= text.size()), result)){
			// Получаем код ошибки приведения
			error = decoder.error();
			// Выводим полученный результат
			return result;
		}
	}
	/**
	 * Выполняем завершение приведения для текста нулевой длины
	 */
	if(text.empty() && !decoder.convert(text.data(), 0, true, result))
		// Получаем код ошибки приведения
		error = decoder.error();
	// Выводим полученный результат
	return result;
}

/**
 * @brief Проверка чтения кодового значения из текста в кодировке UTF-8
 *
 */
TEST(CodecCsvEncoding, Decode) {
	// Длина прочитанной последовательности знака
	size_t length = 0;
	// Выполняем проверку чтения однобайтовой последовательности
	ASSERT_EQ(csv::decode("A", 1, length), 0x41u);
	// Выполняем проверку длины однобайтовой последовательности
	ASSERT_EQ(length, 1u);
	// Выполняем проверку чтения двухбайтовой последовательности
	ASSERT_EQ(csv::decode("\xD0\x90", 2, length), 0x410u);
	// Выполняем проверку длины двухбайтовой последовательности
	ASSERT_EQ(length, 2u);
	// Выполняем проверку чтения трёхбайтовой последовательности
	ASSERT_EQ(csv::decode("\xE2\x82\xAC", 3, length), 0x20ACu);
	// Выполняем проверку длины трёхбайтовой последовательности
	ASSERT_EQ(length, 3u);
	// Выполняем проверку чтения четырёхбайтовой последовательности
	ASSERT_EQ(csv::decode("\xF0\x9F\x98\x80", 4, length), 0x1F600u);
	// Выполняем проверку длины четырёхбайтовой последовательности
	ASSERT_EQ(length, 4u);
}

/**
 * @brief Проверка отклонения ошибочных последовательностей UTF-8
 *
 * @details Избыточно длинная запись и суррогатное кодовое значение отвергаются: первая
 * служит обходом проверок содержимого, второе в кодировке UTF-8 недопустимо вовсе
 *
 */
TEST(CodecCsvEncoding, DecodeInvalid) {
	// Длина прочитанной последовательности знака
	size_t length = 0;
	// Выполняем проверку отклонения избыточно длинной записи знака
	ASSERT_EQ(csv::decode("\xC0\x80", 2, length), csv::INVALID_CODEPOINT);
	// Выполняем проверку отклонения продолжения последовательности без её начала
	ASSERT_EQ(csv::decode("\x80", 1, length), csv::INVALID_CODEPOINT);
	// Выполняем проверку отклонения суррогатного кодового значения
	ASSERT_EQ(csv::decode("\xED\xA0\x80", 3, length), csv::INVALID_CODEPOINT);
	// Выполняем проверку отклонения оборванной последовательности знака
	ASSERT_EQ(csv::decode("\xE2\x82", 2, length), csv::INVALID_CODEPOINT);
	// Выполняем проверку отклонения кодового значения свыше наибольшего
	ASSERT_EQ(csv::decode("\xF5\x80\x80\x80", 4, length), csv::INVALID_CODEPOINT);
}

/**
 * @brief Проверка записи кодового значения в кодировке UTF-8
 *
 */
TEST(CodecCsvEncoding, Encode) {
	// Текст, к которому дописываются знаки
	string result = "";
	// Выполняем запись однобайтового кодового значения
	ASSERT_TRUE(csv::encode(0x41, result));
	// Выполняем запись четырёхбайтового кодового значения
	ASSERT_TRUE(csv::encode(0x1F600, result));
	// Выполняем проверку записанного текста
	ASSERT_EQ(result, "A\xF0\x9F\x98\x80");
	// Выполняем проверку отклонения кодового значения свыше наибольшего
	ASSERT_FALSE(csv::encode(csv::MAX_CODEPOINT + 1, result));
	// Выполняем проверку отклонения суррогатного кодового значения
	ASSERT_FALSE(csv::encode(0xD800, result));
}

/**
 * @brief Проверка приведения текста без метки порядка байтов
 *
 * @details Текст без метки принимается за UTF-8 и проходит приведение неизменным
 *
 */
TEST(CodecCsvEncoding, Utf8) {
	// Код ошибки приведения исходного текста
	csv::error_t error = csv::error_t::NONE;
	// Выполняем приведение текста без метки порядка байтов
	const string & result = convert("имя,значение\n", 4, error);
	// Выполняем проверку отсутствия ошибки приведения
	ASSERT_EQ(error, csv::error_t::NONE);
	// Выполняем проверку того, что текст прошёл приведение неизменным
	ASSERT_EQ(result, "имя,значение\n");
}

/**
 * @brief Проверка снятия метки порядка байтов кодировки UTF-8
 *
 * @details Метка содержимым текста не является и разбору выдаваться не должна: иначе
 * первое имя столбца начиналось бы с невидимого знака и поиск по имени не находил бы его
 *
 */
TEST(CodecCsvEncoding, Signature) {
	// Код ошибки приведения исходного текста
	csv::error_t error = csv::error_t::NONE;
	// Объект приведения исходного текста
	csv::decoder_t decoder;
	// Результат приведения исходного текста
	string result = "";
	// Выполняем приведение текста с меткой порядка байтов
	ASSERT_TRUE(decoder.convert("\xEF\xBB\xBF" "a,b\n", 7, true, result));
	// Выполняем проверку того, что метка порядка байтов снята
	ASSERT_EQ(result, "a,b\n");
	// Выполняем проверку признака обнаружения метки порядка байтов
	ASSERT_TRUE(decoder.signature());
	// Выполняем проверку определённой кодировки исходного текста
	ASSERT_EQ(decoder.encoding(), csv::encoding_t::UTF8);
	// Выполняем проверку снятия метки при подаче текста по одному байту
	ASSERT_EQ(convert("\xEF\xBB\xBF" "a,b\n", 1, error), "a,b\n");
	// Выполняем проверку отсутствия ошибки приведения
	ASSERT_EQ(error, csv::error_t::NONE);
}

/**
 * @brief Проверка приведения текста в кодировке UTF-16
 *
 */
TEST(CodecCsvEncoding, Utf16) {
	/**
	 * Выполняем перебор размеров куска, каким подаётся исходный текст
	 */
	for(size_t chunk = 1; chunk < 8; chunk++){
		// Код ошибки приведения исходного текста
		csv::error_t error = csv::error_t::NONE;
		// Выполняем приведение текста с обратным порядком байтов
		ASSERT_EQ(convert(string("\xFF\xFE" "a\0,\0" "b\0\n\0", 10), chunk, error), "a,b\n") << chunk;
		// Выполняем проверку отсутствия ошибки приведения
		ASSERT_EQ(error, csv::error_t::NONE) << chunk;
		// Выполняем приведение текста с прямым порядком байтов
		ASSERT_EQ(convert(string("\xFE\xFF\0" "a\0,\0" "b\0\n", 10), chunk, error), "a,b\n") << chunk;
		// Выполняем проверку отсутствия ошибки приведения
		ASSERT_EQ(error, csv::error_t::NONE) << chunk;
	}
}

/**
 * @brief Проверка сборки суррогатной пары кодировки UTF-16
 *
 * @details Половины пары разделяются границей куска намеренно: старшая половина обязана
 * удерживаться до прихода младшей, а не выдаваться знаком сама по себе
 *
 */
TEST(CodecCsvEncoding, Surrogate) {
	/**
	 * Выполняем перебор размеров куска, каким подаётся исходный текст
	 */
	for(size_t chunk = 1; chunk < 8; chunk++){
		// Код ошибки приведения исходного текста
		csv::error_t error = csv::error_t::NONE;
		// Выполняем приведение текста, содержащего суррогатную пару
		ASSERT_EQ(convert(string("\xFF\xFE\x3D\xD8\x00\xDE", 6), chunk, error), "\xF0\x9F\x98\x80") << chunk;
		// Выполняем проверку отсутствия ошибки приведения
		ASSERT_EQ(error, csv::error_t::NONE) << chunk;
	}
}

/**
 * @brief Проверка отклонения кодировки UTF-32
 *
 * @details Метка порядка байтов кодировки UTF-32 с обратным порядком начинается теми же
 * двумя байтами, что и метка UTF-16: не опознав её отдельно, приведение прочло бы текст
 * как UTF-16 и выдало разбору знаки вперемежку с нулевыми
 *
 */
TEST(CodecCsvEncoding, Utf32) {
	// Код ошибки приведения исходного текста
	csv::error_t error = csv::error_t::NONE;
	// Выполняем приведение текста с меткой UTF-32 с обратным порядком байтов
	convert(string("\xFF\xFE\x00\x00" "a\0\0\0", 8), 8, error);
	// Выполняем проверку отклонения кодировки исходного текста
	ASSERT_EQ(error, csv::error_t::UNSUPPORTED_ENCODING);
	// Сбрасываем код ошибки приведения исходного текста
	error = csv::error_t::NONE;
	// Выполняем приведение текста с меткой UTF-32 с прямым порядком байтов
	convert(string("\x00\x00\xFE\xFF\0\0\0" "a", 8), 8, error);
	// Выполняем проверку отклонения кодировки исходного текста
	ASSERT_EQ(error, csv::error_t::UNSUPPORTED_ENCODING);
}

/**
 * @brief Проверка приведения текста в однобайтовых кодировках
 *
 * @details Кодировка эта метки порядка байтов не имеет и определению не поддаётся:
 * навязать её вправе лишь потребитель, знающий кодировку из внешнего источника
 *
 */
TEST(CodecCsvEncoding, Forced) {
	// Результат приведения исходного текста
	string result = "";
	// Объект приведения исходного текста
	csv::decoder_t decoder;
	// Выполняем навязывание кодировки исходного текста
	ASSERT_TRUE(decoder.encoding(csv::encoding_t::LATIN1));
	// Выполняем приведение текста в навязанной кодировке
	ASSERT_TRUE(decoder.convert("\xE9,\xFF\n", 4, true, result));
	// Выполняем проверку приведённого текста
	ASSERT_EQ(result, "\xC3\xA9,\xC3\xBF\n");
	// Выполняем проверку того, что кодировка сменена не была
	ASSERT_EQ(decoder.encoding(), csv::encoding_t::LATIN1);
	// Выполняем проверку отклонения смены кодировки посреди текста
	ASSERT_FALSE(decoder.encoding(csv::encoding_t::UTF8));
}

/**
 * @brief Проверка приведения текста в кодировке Windows-1252
 *
 */
TEST(CodecCsvEncoding, Cp1252) {
	// Результат приведения исходного текста
	string result = "";
	// Объект приведения исходного текста
	csv::decoder_t decoder;
	// Выполняем навязывание кодировки исходного текста
	ASSERT_TRUE(decoder.encoding(csv::encoding_t::CP1252));
	// Выполняем приведение текста в навязанной кодировке
	ASSERT_TRUE(decoder.convert("\x80\x92", 2, true, result));
	// Выполняем проверку приведения знака денежной единицы евро
	ASSERT_EQ(result, "\xE2\x82\xAC\xE2\x80\x99");
}

/**
 * @brief Проверка отклонения ошибочного текста в кодировке UTF-8
 *
 */
TEST(CodecCsvEncoding, Invalid) {
	// Код ошибки приведения исходного текста
	csv::error_t error = csv::error_t::NONE;
	// Выполняем приведение текста с ошибочной последовательностью знака
	convert("a,\xC0\x80\n", 8, error);
	// Выполняем проверку отклонения текста исходной кодировки
	ASSERT_EQ(error, csv::error_t::INVALID_ENCODING);
	// Сбрасываем код ошибки приведения исходного текста
	error = csv::error_t::NONE;
	// Выполняем приведение текста с оборванной последовательностью знака
	convert("a,\xE2\x82", 8, error);
	// Выполняем проверку отклонения текста исходной кодировки
	ASSERT_EQ(error, csv::error_t::INVALID_ENCODING);
}

/**
 * @brief Проверка сброса приведения в исходное состояние
 *
 */
TEST(CodecCsvEncoding, Reset) {
	// Результат приведения исходного текста
	string result = "";
	// Объект приведения исходного текста
	csv::decoder_t decoder;
	// Выполняем приведение текста с меткой порядка байтов
	ASSERT_TRUE(decoder.convert(string("\xFF\xFE" "a\0", 4).data(), 4, true, result));
	// Выполняем проверку определённой кодировки исходного текста
	ASSERT_EQ(decoder.encoding(), csv::encoding_t::UTF16LE);
	// Выполняем сброс приведения в исходное состояние
	decoder.reset();
	// Выполняем проверку сброса определённой кодировки исходного текста
	ASSERT_EQ(decoder.encoding(), csv::encoding_t::NONE);
	// Выполняем проверку сброса признака обнаружения метки порядка байтов
	ASSERT_FALSE(decoder.signature());
	// Очищаем результат приведения исходного текста
	result.clear();
	// Выполняем приведение текста без метки порядка байтов
	ASSERT_TRUE(decoder.convert("b", 1, true, result));
	// Выполняем проверку того, что кодировка определена заново
	ASSERT_EQ(decoder.encoding(), csv::encoding_t::UTF8);
	// Выполняем проверку приведённого текста
	ASSERT_EQ(result, "b");
}
