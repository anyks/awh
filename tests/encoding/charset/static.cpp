/**
 * @file static.cpp
 * @date 2026-08-03
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
 * @brief Статические тесты модуля перекодировки — разбор имён кодировок, перекодировка
 *        текста между однобайтовыми кодировками и UTF-8, порядок обращения с символами,
 *        кодировке не представимыми, и правильность работы кодировщика UTF-8
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл
 */
#include "charset.hpp"

/**
 * Используем пространства имён
 */
using namespace awh;
using namespace awh::charset;

/**
 * @brief Тест разбора имён кодировок
 *
 */
TEST_F(CharsetFixture, EncodingByName){
	// Выполняем проверку разбора канонических имён кодировок
	ASSERT_EQ(encoding("windows-1251"), encoding_t::CP1251);
	ASSERT_EQ(encoding("UTF-8"), encoding_t::UTF8);
	ASSERT_EQ(encoding("KOI8-R"), encoding_t::KOI8_R);
	ASSERT_EQ(encoding("IBM866"), encoding_t::CP866);
	// Выполняем проверку разбора имён без учёта регистра и пробельных символов
	ASSERT_EQ(encoding("  Windows-1251 "), encoding_t::CP1251);
	ASSERT_EQ(encoding("utf8"), encoding_t::UTF8);
	ASSERT_EQ(encoding("KoI8R"), encoding_t::KOI8_R);
	// Выполняем проверку разбора имён, задаваемых стандартом кодировок дополнительно
	ASSERT_EQ(encoding("cp1251"), encoding_t::CP1251);
	ASSERT_EQ(encoding("x-cp1251"), encoding_t::CP1251);
	ASSERT_EQ(encoding("cyrillic"), encoding_t::ISO8859_5);
	ASSERT_EQ(encoding("x-mac-cyrillic"), encoding_t::MAC_CYRILLIC);
	/**
	 * Выполняем проверку разбора имён, стандартом кодировок отнесённых к CP1252
	 *
	 * @details Стандарт кодировок предписывает разбирать имена «iso-8859-1», «ascii»
	 *          и «latin1» как имена кодировки CP1252: отправители задают ими текст,
	 *          записанный в CP1252, чаще, чем текст, записанный в ISO-8859-1.
	 */
	ASSERT_EQ(encoding("iso-8859-1"), encoding_t::CP1252);
	ASSERT_EQ(encoding("us-ascii"), encoding_t::CP1252);
	ASSERT_EQ(encoding("latin1"), encoding_t::CP1252);
	// Выполняем проверку отказа разбора нераспознанного имени
	ASSERT_EQ(encoding("windows-9999"), encoding_t::NONE);
	ASSERT_EQ(encoding(""), encoding_t::NONE);
}
/**
 * @brief Тест извлечения канонических имён кодировок
 *
 */
TEST_F(CharsetFixture, LabelByEncoding){
	// Выполняем проверку извлечения канонических имён кодировок
	ASSERT_EQ(label(encoding_t::CP1251), "windows-1251");
	ASSERT_EQ(label(encoding_t::UTF8), "UTF-8");
	ASSERT_EQ(label(encoding_t::ASCII), "US-ASCII");
	ASSERT_EQ(label(encoding_t::KOI8_R), "KOI8-R");
	// Выполняем проверку отсутствия имени у неустановленной кодировки
	ASSERT_EQ(label(encoding_t::NONE), "");
}
/**
 * @brief Тест перекодировки текста между кодировкой CP1251 и UTF-8
 *
 */
TEST_F(CharsetFixture, TranscodeCyrillic){
	// Записываем слово «Привет» в кодировке CP1251
	const string source = "\xCF\xF0\xE8\xE2\xE5\xF2";
	// Записываем слово «Привет» в кодировке UTF-8
	const string target = "Привет";
	// Результат перекодировки текста
	string result = "";
	// Выполняем перекодировку текста из кодировки CP1251 в UTF-8
	ASSERT_TRUE(transcode(source, encoding_t::CP1251, encoding_t::UTF8, result));
	ASSERT_EQ(result, target);
	// Выполняем перекодировку текста из кодировки UTF-8 в CP1251
	ASSERT_TRUE(transcode(target, encoding_t::UTF8, encoding_t::CP1251, result));
	ASSERT_EQ(result, source);
}
/**
 * @brief Тест перекодировки текста между однобайтовыми кодировками
 *
 */
TEST_F(CharsetFixture, TranscodeBetweenCodepages){
	// Записываем слово «Привет» в кодировке CP1251
	const string source = "\xCF\xF0\xE8\xE2\xE5\xF2";
	// Записываем слово «Привет» в кодировке KOI8-R
	const string target = "\xF0\xD2\xC9\xD7\xC5\xD4";
	// Результат перекодировки текста
	string result = "";
	// Выполняем перекодировку текста из кодировки CP1251 в KOI8-R
	ASSERT_TRUE(transcode(source, encoding_t::CP1251, encoding_t::KOI8_R, result));
	ASSERT_EQ(result, target);
	// Выполняем перекодировку текста из кодировки KOI8-R в CP1251
	ASSERT_TRUE(transcode(target, encoding_t::KOI8_R, encoding_t::CP1251, result));
	ASSERT_EQ(result, source);
}
/**
 * @brief Тест обращения с символами, кодировке не представимыми
 *
 */
TEST_F(CharsetFixture, TranscodeReplacement){
	// Записываем текст, кириллица которого кодировке ISO-8859-1 не представима
	const string source = "aПb";
	// Результат перекодировки текста
	string result = "";
	// Выполняем проверку отклонения перекодировки текста целиком
	ASSERT_FALSE(transcode(source, encoding_t::UTF8, encoding_t::ISO8859_1, result, replace_t::STRICT));
	ASSERT_TRUE(result.empty());
	// Выполняем проверку пропуска непредставимых символов
	ASSERT_TRUE(transcode(source, encoding_t::UTF8, encoding_t::ISO8859_1, result, replace_t::SKIP));
	ASSERT_EQ(result, "ab");
	// Выполняем проверку замены непредставимых символов
	ASSERT_TRUE(transcode(source, encoding_t::UTF8, encoding_t::ISO8859_1, result, replace_t::REPLACE));
	ASSERT_EQ(result, "a?b");
}
/**
 * @brief Тест отклонения неправильной записи текста
 *
 */
TEST_F(CharsetFixture, TranscodeInvalid){
	// Результат перекодировки текста
	string result = "";
	// Выполняем проверку отклонения оборванной записи символа UTF-8
	ASSERT_FALSE(transcode("\xD0", encoding_t::UTF8, encoding_t::CP1251, result));
	// Выполняем проверку отклонения избыточной записи символа UTF-8
	ASSERT_FALSE(transcode("\xC0\xAF", encoding_t::UTF8, encoding_t::CP1251, result));
	// Выполняем проверку отклонения записи суррогатной пары
	ASSERT_FALSE(transcode("\xED\xA0\x80", encoding_t::UTF8, encoding_t::CP1251, result));
	// Выполняем проверку отклонения байта, набору ASCII не принадлежащего
	ASSERT_FALSE(transcode("\xCF", encoding_t::ASCII, encoding_t::UTF8, result));
	// Выполняем проверку отклонения перекодировки при неустановленной кодировке
	ASSERT_FALSE(transcode("text", encoding_t::NONE, encoding_t::UTF8, result));
	ASSERT_FALSE(transcode("text", encoding_t::UTF8, encoding_t::NONE, result));
}
/**
 * @brief Тест перекодировки пустого текста
 *
 */
TEST_F(CharsetFixture, TranscodeEmpty){
	// Результат перекодировки текста
	string result = "неочищенное значение";
	// Выполняем проверку перекодировки пустого текста
	ASSERT_TRUE(transcode("", encoding_t::UTF8, encoding_t::CP1251, result));
	ASSERT_TRUE(result.empty());
}
/**
 * @brief Тест определения кодировки текста
 *
 */
TEST_F(CharsetFixture, DetectEncoding){
	// Выполняем проверку определения кодировки текста, записанного в UTF-8
	ASSERT_EQ(detect("Привет"), encoding_t::UTF8);
	ASSERT_EQ(detect("plain text"), encoding_t::UTF8);
	// Выполняем проверку определения кодировки текста, записью UTF-8 не являющегося
	ASSERT_EQ(detect("\xCF\xF0\xE8\xE2\xE5\xF2"), encoding_t::CP1251);
	ASSERT_EQ(detect("\xF0\xD2\xC9\xD7\xC5\xD4", encoding_t::KOI8_R), encoding_t::KOI8_R);
}
/**
 * @brief Тест представления кодовых значений записью UTF-8
 *
 */
TEST_F(CharsetFixture, Utf8Encode){
	// Буфер записи символа в кодировке UTF-8
	char buffer[utf8::MAX_LENGTH];
	// Выполняем проверку записи символов всех длин
	ASSERT_EQ(utf8::encode(0x41, buffer), 1u);
	ASSERT_EQ(string(buffer, 1), "A");
	ASSERT_EQ(utf8::encode(0x430, buffer), 2u);
	ASSERT_EQ(string(buffer, 2), "а");
	ASSERT_EQ(utf8::encode(0x20AC, buffer), 3u);
	ASSERT_EQ(string(buffer, 3), "€");
	ASSERT_EQ(utf8::encode(0x1F600, buffer), 4u);
	ASSERT_EQ(string(buffer, 4), "😀");
	// Выполняем проверку отказа записи суррогатной пары и значения вне Юникода
	ASSERT_EQ(utf8::encode(0xD800, buffer), 0u);
	ASSERT_EQ(utf8::encode(0x110000, buffer), 0u);
	// Выполняем проверку отказа записи при отсутствии буфера
	ASSERT_EQ(utf8::encode(0x41, nullptr), 0u);
}
/**
 * @brief Тест разбора записи символов в кодировке UTF-8
 *
 */
TEST_F(CharsetFixture, Utf8Decode){
	// Кодовое значение разобранного символа
	uint32_t code = 0;
	// Выполняем проверку разбора записей символов всех длин
	ASSERT_EQ(utf8::decode("A", 0, code), 1u);
	ASSERT_EQ(code, 0x41u);
	ASSERT_EQ(utf8::decode("а", 0, code), 2u);
	ASSERT_EQ(code, 0x430u);
	ASSERT_EQ(utf8::decode("€", 0, code), 3u);
	ASSERT_EQ(code, 0x20ACu);
	ASSERT_EQ(utf8::decode("😀", 0, code), 4u);
	ASSERT_EQ(code, 0x1F600u);
	// Выполняем проверку отказа разбора за пределами текста
	ASSERT_EQ(utf8::decode("A", 1, code), 0u);
	// Выполняем проверку отказа разбора продолжающего байта как ведущего
	ASSERT_EQ(utf8::decode("\x80", 0, code), 0u);
	// Выполняем проверку отказа разбора оборванной записи
	ASSERT_EQ(utf8::decode("\xD0", 0, code), 0u);
	// Выполняем проверку отказа разбора избыточной записи
	ASSERT_EQ(utf8::decode("\xC0\xAF", 0, code), 0u);
	ASSERT_EQ(utf8::decode("\xE0\x80\xAF", 0, code), 0u);
	// Выполняем проверку отказа разбора записи суррогатной пары
	ASSERT_EQ(utf8::decode("\xED\xA0\x80", 0, code), 0u);
	// Выполняем проверку отказа разбора значения, Юникод превышающего
	ASSERT_EQ(utf8::decode("\xF5\x80\x80\x80", 0, code), 0u);
}
/**
 * @brief Тест проверки правильности записи текста и подсчёта символов
 *
 */
TEST_F(CharsetFixture, Utf8ValidLength){
	// Выполняем проверку правильности записи текста
	ASSERT_TRUE(utf8::valid(""));
	ASSERT_TRUE(utf8::valid("Привет, мир!"));
	ASSERT_FALSE(utf8::valid("\xD0"));
	ASSERT_FALSE(utf8::valid("\xCF\xF0\xE8\xE2\xE5\xF2"));
	// Выполняем проверку подсчёта количества символов текста
	ASSERT_EQ(utf8::length(""), 0u);
	ASSERT_EQ(utf8::length("abc"), 3u);
	ASSERT_EQ(utf8::length("Привет"), 6u);
	ASSERT_EQ(utf8::length("😀😀"), 2u);
	// Выполняем проверку отказа подсчёта символов неправильной записи
	ASSERT_EQ(utf8::length("\xD0"), 0u);
}
/**
 * @brief Тест кругового обхода всех байтов каждой заданной кодировки
 *
 * @details Каждый байт каждой кодировки, кодовое значение которому соответствует,
 *          переводится в UTF-8 и обратно: получившийся байт обязан совпасть с
 *          исходным. Обход закрепляет согласованность прямой и обратной таблиц.
 *
 */
TEST_F(CharsetFixture, RoundtripAllCodepages){
	// Результат перекодировки текста
	string forward = "", backward = "";
	/**
	 * Выполняем обход набора таблиц соответствия кодировок
	 */
	for(size_t i = 0; i < CODEPAGES_COUNT; i++) {
		// Получаем обозначение очередной кодировки набора
		const encoding_t item = CODEPAGES[i].encoding;
		/**
		 * Выполняем обход всех байтов кодировки
		 */
		for(uint32_t byte = 0; byte <= 0xFF; byte++) {
			/**
			 * Если байту кодировки кодовое значение не соответствует
			 */
			if(CODEPAGES[i].unicode[byte] == UNMAPPED)
				// Переходим к следующему байту кодировки
				continue;
			// Записываем очередной байт кодировки
			const string source(1, static_cast <char> (byte));
			// Выполняем перекодировку байта кодировки в UTF-8
			ASSERT_TRUE(transcode(source, item, encoding_t::UTF8, forward))
				<< "кодировка " << label(item) << ", байт " << byte;
			// Выполняем перекодировку записи UTF-8 обратно в кодировку
			ASSERT_TRUE(transcode(forward, encoding_t::UTF8, item, backward))
				<< "кодировка " << label(item) << ", байт " << byte;
			/**
			 * Выполняем проверку совпадения байта с исходным
			 *
			 * @details Кодовому значению, которому соответствует несколько байтов
			 *          кодировки, обратная таблица сопоставляет наименьший из них,
			 *          поэтому сличается кодовое значение, а не сам байт.
			 */
			ASSERT_EQ(CODEPAGES[i].unicode[static_cast <uint8_t> (backward[0])], CODEPAGES[i].unicode[byte])
				<< "кодировка " << label(item) << ", байт " << byte;
		}
	}
}
