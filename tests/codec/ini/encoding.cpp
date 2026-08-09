/**
 * @file: encoding.cpp
 * @date: 2026-08-09
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Автоматические тесты приведения текста настроек к кодировке UTF-8 —
 *        определение кодировки по метке порядка байтов, суррогатные пары, отклонение
 *        управляющих знаков и подача текста кусками произвольного размера
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
#include <codec/ini/ini.hpp>

/**
 * Подключаем заголовочные файлы тестового окружения
 */
#include "../../main.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh::codec;

/**
 * @brief Метод приведения текста к кодировке UTF-8 с подачей кусками заданного размера
 *
 * @param input  приводимый исходный текст
 * @param step   размер куска подаваемого текста
 * @param result приведённый к кодировке UTF-8 текст
 * @param error  код ошибки приведения
 * @param enc    определённая кодировка исходного текста
 * @return       результат выполнения приведения
 *
 */
static bool convert(const string & input, const size_t step, string & result, ini::error_t & error, ini::encoding_t & enc) noexcept {
	// Объект приведения исходного текста к кодировке UTF-8
	ini::decoder_t decoder;
	// Выполняем очистку приведённого текста
	result.clear();
	/**
	 * Выполняем перебор всех кусков исходного текста
	 */
	for(size_t i = 0; (i < input.size()) || (i == 0); i += step){
		// Получаем размер очередного куска исходного текста
		const size_t size = ((i + step) > input.size() ? (input.size() - i) : step);
		// Получаем признак последнего куска исходного текста
		const bool end = ((i + size) >= input.size());
		/**
		 * Если приведение куска исходного текста выполнить не удалось
		 */
		if(!decoder.convert(input.data() + i, size, end, result)){
			// Запоминаем код ошибки приведения
			error = decoder.error();
			// Запоминаем определённую кодировку исходного текста
			enc = decoder.encoding();
			// Выводим отрицательный результат выполнения приведения
			return false;
		}
		/**
		 * Если исходный текст исчерпан
		 */
		if(end)
			// Выполняем прекращение подачи кусков исходного текста
			break;
	}
	// Запоминаем код ошибки приведения
	error = decoder.error();
	// Запоминаем определённую кодировку исходного текста
	enc = decoder.encoding();
	// Выводим положительный результат выполнения приведения
	return true;
}

/**
 * @brief Проверка приведения текста без метки порядка байтов
 *
 */
TEST(CodecIniEncoding, Plain) {
	// Приведённый к кодировке UTF-8 текст
	string result;
	// Код ошибки приведения
	ini::error_t error = ini::error_t::NONE;
	// Определённая кодировка исходного текста
	ini::encoding_t encoding = ini::encoding_t::NONE;
	// Выполняем приведение текста настроек целиком
	ASSERT_TRUE(::convert("[a]\nk=v\n", 64, result, error, encoding));
	// Выполняем проверку приведённого текста
	ASSERT_EQ(result, "[a]\nk=v\n");
	// Выполняем проверку определённой кодировки исходного текста
	ASSERT_EQ(encoding, ini::encoding_t::UTF8);
}
/**
 * @brief Проверка снятия метки порядка байтов кодировки UTF-8
 *
 */
TEST(CodecIniEncoding, SignatureUtf8) {
	// Приведённый к кодировке UTF-8 текст
	string result;
	// Код ошибки приведения
	ini::error_t error = ini::error_t::NONE;
	// Определённая кодировка исходного текста
	ini::encoding_t encoding = ini::encoding_t::NONE;
	/**
	 * Выполняем перебор размеров куска подаваемого текста
	 */
	for(size_t step = 1; step < 8; step++){
		// Выполняем приведение текста настроек кусками
		ASSERT_TRUE(::convert("\xEF\xBB\xBF[a]\nk=v\n", step, result, error, encoding));
		// Выполняем проверку приведённого текста
		ASSERT_EQ(result, "[a]\nk=v\n");
		// Выполняем проверку определённой кодировки исходного текста
		ASSERT_EQ(encoding, ini::encoding_t::UTF8);
	}
}
/**
 * @brief Проверка приведения текста в кодировке UTF-16
 *
 */
TEST(CodecIniEncoding, Utf16) {
	// Приведённый к кодировке UTF-8 текст
	string result;
	// Код ошибки приведения
	ini::error_t error = ini::error_t::NONE;
	// Определённая кодировка исходного текста
	ini::encoding_t encoding = ini::encoding_t::NONE;
	// Исходный текст в кодировке UTF-16 с обратным порядком байтов
	const string little("\xFF\xFE" "k\0" "=\0" "v\0", 8);
	// Исходный текст в кодировке UTF-16 с прямым порядком байтов
	const string big("\xFE\xFF" "\0k" "\0=" "\0v", 8);
	/**
	 * Выполняем перебор размеров куска подаваемого текста
	 */
	for(size_t step = 1; step < 10; step++){
		// Выполняем приведение текста с обратным порядком байтов
		ASSERT_TRUE(::convert(little, step, result, error, encoding));
		// Выполняем проверку приведённого текста
		ASSERT_EQ(result, "k=v");
		// Выполняем проверку определённой кодировки исходного текста
		ASSERT_EQ(encoding, ini::encoding_t::UTF16LE);
		// Выполняем приведение текста с прямым порядком байтов
		ASSERT_TRUE(::convert(big, step, result, error, encoding));
		// Выполняем проверку приведённого текста
		ASSERT_EQ(result, "k=v");
		// Выполняем проверку определённой кодировки исходного текста
		ASSERT_EQ(encoding, ini::encoding_t::UTF16BE);
	}
}
/**
 * @brief Проверка приведения суррогатной пары кодировки UTF-16
 *
 */
TEST(CodecIniEncoding, Surrogate) {
	// Приведённый к кодировке UTF-8 текст
	string result;
	// Код ошибки приведения
	ini::error_t error = ini::error_t::NONE;
	// Определённая кодировка исходного текста
	ini::encoding_t encoding = ini::encoding_t::NONE;
	// Исходный текст со знаком, записанным суррогатной парой
	const string input("\xFF\xFE" "\x3D\xD8\x00\xDE", 6);
	/**
	 * Выполняем перебор размеров куска подаваемого текста
	 */
	for(size_t step = 1; step < 8; step++){
		// Выполняем приведение текста с суррогатной парой
		ASSERT_TRUE(::convert(input, step, result, error, encoding));
		// Выполняем проверку приведённого текста
		ASSERT_EQ(result, "\xF0\x9F\x98\x80");
	}
	// Исходный текст с оборванной суррогатной парой
	const string broken("\xFF\xFE" "\x3D\xD8", 4);
	// Выполняем приведение текста с оборванной суррогатной парой
	ASSERT_FALSE(::convert(broken, 64, result, error, encoding));
	// Выполняем проверку кода ошибки приведения
	ASSERT_EQ(error, ini::error_t::INVALID_ENCODING);
}
/**
 * @brief Проверка отклонения кодировки UTF-32
 *
 */
TEST(CodecIniEncoding, Utf32) {
	// Приведённый к кодировке UTF-8 текст
	string result;
	// Код ошибки приведения
	ini::error_t error = ini::error_t::NONE;
	// Определённая кодировка исходного текста
	ini::encoding_t encoding = ini::encoding_t::NONE;
	// Исходный текст в кодировке UTF-32 с обратным порядком байтов
	const string input("\xFF\xFE\x00\x00" "k\0\0\0", 8);
	// Выполняем приведение текста в кодировке UTF-32
	ASSERT_FALSE(::convert(input, 64, result, error, encoding));
	// Выполняем проверку кода ошибки приведения
	ASSERT_EQ(error, ini::error_t::UNSUPPORTED_ENCODING);
}
/**
 * @brief Проверка приведения текста в кодировке ISO-8859-1
 *
 */
TEST(CodecIniEncoding, Latin1) {
	// Объект приведения исходного текста к кодировке UTF-8
	ini::decoder_t decoder;
	// Приведённый к кодировке UTF-8 текст
	string result;
	// Выполняем навязывание кодировки исходного текста
	ASSERT_TRUE(decoder.encoding(ini::encoding_t::LATIN1));
	// Выполняем приведение текста в кодировке ISO-8859-1
	ASSERT_TRUE(decoder.convert("k=\xE9", 3, true, result));
	// Выполняем проверку приведённого текста
	ASSERT_EQ(result, "k=\xC3\xA9");
	// Выполняем проверку определённой кодировки исходного текста
	ASSERT_EQ(decoder.encoding(), ini::encoding_t::LATIN1);
}
/**
 * @brief Проверка отклонения управляющих знаков
 *
 */
TEST(CodecIniEncoding, Control) {
	// Приведённый к кодировке UTF-8 текст
	string result;
	// Код ошибки приведения
	ini::error_t error = ini::error_t::NONE;
	// Определённая кодировка исходного текста
	ini::encoding_t encoding = ini::encoding_t::NONE;
	// Выполняем приведение текста с управляющим знаком области C0
	ASSERT_FALSE(::convert(string("k=\x01\n", 4), 64, result, error, encoding));
	// Выполняем проверку кода ошибки приведения
	ASSERT_EQ(error, ini::error_t::INVALID_CHARACTER);
	// Выполняем приведение текста с нулевым байтом
	ASSERT_FALSE(::convert(string("k=\0\n", 4), 64, result, error, encoding));
	// Выполняем проверку кода ошибки приведения
	ASSERT_EQ(error, ini::error_t::INVALID_CHARACTER);
	// Выполняем приведение текста со знаком горизонтальной табуляции
	ASSERT_TRUE(::convert("k=\tv\n", 64, result, error, encoding));
	// Выполняем проверку приведённого текста
	ASSERT_EQ(result, "k=\tv\n");
}
/**
 * @brief Проверка отклонения ошибочных последовательностей кодировки UTF-8
 *
 */
TEST(CodecIniEncoding, Malformed) {
	// Приведённый к кодировке UTF-8 текст
	string result;
	// Код ошибки приведения
	ini::error_t error = ini::error_t::NONE;
	// Определённая кодировка исходного текста
	ini::encoding_t encoding = ini::encoding_t::NONE;
	// Выполняем приведение текста с оборванной последовательностью знака
	ASSERT_FALSE(::convert("k=\xD0", 64, result, error, encoding));
	// Выполняем проверку кода ошибки приведения
	ASSERT_EQ(error, ini::error_t::INVALID_ENCODING);
	// Выполняем приведение текста с избыточно длинной записью знака
	ASSERT_FALSE(::convert("k=\xC0\xAF", 64, result, error, encoding));
	// Выполняем проверку кода ошибки приведения
	ASSERT_EQ(error, ini::error_t::INVALID_ENCODING);
	// Выполняем приведение текста с продолжающим байтом в начале последовательности
	ASSERT_FALSE(::convert("k=\x80", 64, result, error, encoding));
	// Выполняем проверку кода ошибки приведения
	ASSERT_EQ(error, ini::error_t::INVALID_ENCODING);
}
/**
 * @brief Проверка сохранения навязанной кодировки при сбросе приведения
 *
 */
TEST(CodecIniEncoding, Reset) {
	// Объект приведения исходного текста к кодировке UTF-8
	ini::decoder_t decoder;
	// Приведённый к кодировке UTF-8 текст
	string result;
	// Выполняем навязывание кодировки исходного текста
	ASSERT_TRUE(decoder.encoding(ini::encoding_t::LATIN1));
	// Выполняем приведение первого текста настроек
	ASSERT_TRUE(decoder.convert("a", 1, true, result));
	// Выполняем сброс приведения в исходное состояние
	decoder.reset();
	// Выполняем проверку сохранения навязанной кодировки
	ASSERT_EQ(decoder.encoding(), ini::encoding_t::LATIN1);
	/**
	 * Выполняем проверку отклонения смены кодировки посреди текста
	 */
	ASSERT_TRUE(decoder.convert("b", 1, false, result));
	// Выполняем проверку отклонения смены кодировки
	ASSERT_FALSE(decoder.encoding(ini::encoding_t::UTF8));
}
