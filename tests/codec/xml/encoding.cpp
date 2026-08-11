/**
 * @file: encoding.cpp
 * @date: 2026-08-01
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Автоматические тесты приведения исходного текста разметки к кодировке UTF-8 — определение
 *        кодировки по метке порядка байтов и по объявлению разметки, сборка суррогатных пар,
 *        отклонение ошибочных последовательностей и разрыв последовательностей границей куска
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
#include <codec/xml/encoding.hpp>
#include <codec/xml/reader.hpp>

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
static bool convert(const string & input, const size_t step, string & result, xml::error_t & error, xml::encoding_t & enc) noexcept {
	// Объект приведения исходного текста к кодировке UTF-8
	xml::decoder_t decoder;
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
		 * Если исходный текст на этом окончен
		 */
		if(end)
			// Выходим из перебора кусков
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
 * @brief Метод сборки текста в кодировке UTF-16
 *
 * @param text    собираемый текст в кодировке US-ASCII
 * @param reverse признак обратного порядка байтов
 * @return        собранный текст в кодировке UTF-16
 *
 */
static string utf16(const string & text, const bool reverse) noexcept {
	// Собираемый текст в кодировке UTF-16
	string result;
	/**
	 * Выполняем перебор всех знаков собираемого текста
	 */
	for(const char letter : text){
		/**
		 * Если байты записываются в обратном порядке
		 */
		if(reverse){
			// Выполняем добавление младшего байта знака
			result.push_back(letter);
			// Выполняем добавление старшего байта знака
			result.push_back('\0');
		/**
		 * Если байты записываются в прямом порядке
		 */
		} else {
			// Выполняем добавление старшего байта знака
			result.push_back('\0');
			// Выполняем добавление младшего байта знака
			result.push_back(letter);
		}
	}
	// Выводим собранный текст в кодировке UTF-16
	return result;
}

/**
 * @brief Проверка приведения текста в кодировке UTF-8
 *
 */
TEST(CodecXmlEncoding, Utf8) {
	// Приведённый к кодировке UTF-8 текст
	string result;
	// Код ошибки приведения
	xml::error_t error = xml::error_t::NONE;
	// Определённая кодировка исходного текста
	xml::encoding_t enc = xml::encoding_t::NONE;
	// Выполняем приведение текста целиком
	ASSERT_TRUE(convert("<a>привет</a>", 4096, result, error, enc));
	// Выполняем проверку приведённого текста
	ASSERT_EQ(result, "<a>привет</a>");
	// Выполняем проверку определённой кодировки
	ASSERT_EQ(enc, xml::encoding_t::UTF8);
}
/**
 * @brief Проверка разрыва многобайтовой последовательности границей куска
 *
 */
TEST(CodecXmlEncoding, Utf8Split) {
	// Приведённый к кодировке UTF-8 текст
	string result;
	// Код ошибки приведения
	xml::error_t error = xml::error_t::NONE;
	// Определённая кодировка исходного текста
	xml::encoding_t enc = xml::encoding_t::NONE;
	// Выполняем приведение текста по одному байту
	ASSERT_TRUE(convert("<a>привет</a>", 1, result, error, enc));
	// Выполняем проверку приведённого текста
	ASSERT_EQ(result, "<a>привет</a>");
}
/**
 * @brief Проверка снятия метки порядка байтов кодировки UTF-8
 *
 */
TEST(CodecXmlEncoding, Utf8Mark) {
	// Приведённый к кодировке UTF-8 текст
	string result;
	// Код ошибки приведения
	xml::error_t error = xml::error_t::NONE;
	// Определённая кодировка исходного текста
	xml::encoding_t enc = xml::encoding_t::NONE;
	// Выполняем приведение текста с меткой порядка байтов
	ASSERT_TRUE(convert(string("\xEF\xBB\xBF<a/>"), 4096, result, error, enc));
	// Выполняем проверку снятия метки порядка байтов
	ASSERT_EQ(result, "<a/>");
	// Выполняем проверку определённой кодировки
	ASSERT_EQ(enc, xml::encoding_t::UTF8);
}
/**
 * @brief Проверка приведения текста в кодировке UTF-16 с меткой порядка байтов
 *
 */
TEST(CodecXmlEncoding, Utf16Mark) {
	// Приведённый к кодировке UTF-8 текст
	string result;
	// Код ошибки приведения
	xml::error_t error = xml::error_t::NONE;
	// Определённая кодировка исходного текста
	xml::encoding_t enc = xml::encoding_t::NONE;
	// Собираем текст в кодировке UTF-16 с обратным порядком байтов
	const string input = (string("\xFF\xFE", 2) + ::utf16("<a/>", true));
	// Выполняем приведение текста целиком
	ASSERT_TRUE(convert(input, 4096, result, error, enc));
	// Выполняем проверку приведённого текста
	ASSERT_EQ(result, "<a/>");
	// Выполняем проверку определённой кодировки
	ASSERT_EQ(enc, xml::encoding_t::UTF16LE);
	// Выполняем приведение текста по одному байту
	ASSERT_TRUE(convert(input, 1, result, error, enc));
	// Выполняем проверку приведённого текста
	ASSERT_EQ(result, "<a/>");
}
/**
 * @brief Проверка определения кодировки UTF-16 по объявлению разметки
 *
 */
TEST(CodecXmlEncoding, Utf16Sniff) {
	// Приведённый к кодировке UTF-8 текст
	string result;
	// Код ошибки приведения
	xml::error_t error = xml::error_t::NONE;
	// Определённая кодировка исходного текста
	xml::encoding_t enc = xml::encoding_t::NONE;
	// Собираем текст в кодировке UTF-16 с прямым порядком байтов
	const string input = ::utf16("<?xml version=\"1.0\"?><a/>", false);
	// Выполняем приведение текста целиком
	ASSERT_TRUE(convert(input, 4096, result, error, enc));
	// Выполняем проверку приведённого текста
	ASSERT_EQ(result, "<?xml version=\"1.0\"?><a/>");
	// Выполняем проверку определённой кодировки
	ASSERT_EQ(enc, xml::encoding_t::UTF16BE);
}
/**
 * @brief Проверка сборки суррогатной пары кодировки UTF-16
 *
 */
TEST(CodecXmlEncoding, Utf16Surrogate) {
	// Приведённый к кодировке UTF-8 текст
	string result;
	// Код ошибки приведения
	xml::error_t error = xml::error_t::NONE;
	// Определённая кодировка исходного текста
	xml::encoding_t enc = xml::encoding_t::NONE;
	// Собираем текст с суррогатной парой
	const string input("\xFF\xFE\x3D\xD8\x00\xDE", 6);
	// Выполняем приведение текста целиком
	ASSERT_TRUE(convert(input, 4096, result, error, enc));
	// Выполняем проверку сборки знака из суррогатной пары
	ASSERT_EQ(result, "\xF0\x9F\x98\x80");
	// Выполняем приведение текста по одному байту
	ASSERT_TRUE(convert(input, 1, result, error, enc));
	// Выполняем проверку сборки знака при разрыве пары границей куска
	ASSERT_EQ(result, "\xF0\x9F\x98\x80");
}
/**
 * @brief Проверка приведения текста в кодировке, объявленной в разметке
 *
 */
TEST(CodecXmlEncoding, Latin1) {
	// Приведённый к кодировке UTF-8 текст
	string result;
	// Код ошибки приведения
	xml::error_t error = xml::error_t::NONE;
	// Определённая кодировка исходного текста
	xml::encoding_t enc = xml::encoding_t::NONE;
	// Выполняем приведение текста целиком
	ASSERT_TRUE(convert(string("<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><a>\xE9</a>"), 4096, result, error, enc));
	// Выполняем проверку приведённого текста
	ASSERT_EQ(result, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><a>\xC3\xA9</a>");
	// Выполняем проверку определённой кодировки
	ASSERT_EQ(enc, xml::encoding_t::LATIN1);
}
/**
 * @brief Проверка отклонения неподдерживаемой кодировки
 *
 */
TEST(CodecXmlEncoding, Unsupported) {
	// Приведённый к кодировке UTF-8 текст
	string result;
	// Код ошибки приведения
	xml::error_t error = xml::error_t::NONE;
	// Определённая кодировка исходного текста
	xml::encoding_t enc = xml::encoding_t::NONE;
	// Выполняем приведение текста целиком
	ASSERT_FALSE(convert(string("<?xml version=\"1.0\" encoding=\"KOI8-R\"?><a/>"), 4096, result, error, enc));
	// Выполняем проверку кода ошибки приведения
	ASSERT_EQ(error, xml::error_t::UNSUPPORTED_ENCODING);
}
/**
 * @brief Проверка того, что название поля кодировки ищется лишь в объявлении разметки
 *
 * @details Слово «encoding» встречается и в содержимом документа - скажем, в имени
 *          атрибута «encodingStyle» ответа по договору SOAP, - и принимать его за
 *          объявление кодировки нельзя
 *
 */
TEST(CodecXmlEncoding, ContentIsNotDeclaration) {
	// Приведённый к кодировке UTF-8 текст
	string result;
	// Код ошибки приведения
	xml::error_t error = xml::error_t::NONE;
	// Определённая кодировка исходного текста
	xml::encoding_t enc = xml::encoding_t::NONE;
	// Выполняем приведение ответа по договору SOAP
	ASSERT_TRUE(convert(string("<?xml version=\"1.0\"?><s:Envelope s:encodingStyle=\"urn:e\"/>"), 4096, result, error, enc));
	// Выполняем проверку кода ошибки приведения
	ASSERT_EQ(error, xml::error_t::NONE);
	// Выполняем проверку определённой кодировки
	ASSERT_EQ(enc, xml::encoding_t::UTF8);
}
/**
 * @brief Проверка отклонения ошибочных последовательностей кодировки UTF-8
 *
 */
TEST(CodecXmlEncoding, Malformed) {
	// Приведённый к кодировке UTF-8 текст
	string result;
	// Код ошибки приведения
	xml::error_t error = xml::error_t::NONE;
	// Определённая кодировка исходного текста
	xml::encoding_t enc = xml::encoding_t::NONE;
	// Выполняем проверку отклонения ошибочной последовательности
	ASSERT_FALSE(convert(string("<a>\xC3\x28</a>"), 4096, result, error, enc));
	// Выполняем проверку кода ошибки приведения
	ASSERT_EQ(error, xml::error_t::INVALID_ENCODING);
	// Выполняем проверку отклонения избыточно длинной записи знака
	ASSERT_FALSE(convert(string("<a>\xC0\xAF</a>"), 4096, result, error, enc));
	// Выполняем проверку кода ошибки приведения
	ASSERT_EQ(error, xml::error_t::INVALID_ENCODING);
	// Выполняем проверку отклонения суррогатного кодового значения
	ASSERT_FALSE(convert(string("<a>\xED\xA0\x80</a>"), 4096, result, error, enc));
	// Выполняем проверку кода ошибки приведения
	ASSERT_EQ(error, xml::error_t::INVALID_ENCODING);
	// Выполняем проверку отклонения оборванной последовательности
	ASSERT_FALSE(convert(string("<a>\xD0"), 4096, result, error, enc));
	// Выполняем проверку кода ошибки приведения
	ASSERT_EQ(error, xml::error_t::INVALID_ENCODING);
	// Выполняем проверку отклонения управляющего знака
	ASSERT_FALSE(convert(string("<a>\x01</a>"), 4096, result, error, enc));
	// Выполняем проверку кода ошибки приведения
	ASSERT_EQ(error, xml::error_t::INVALID_CHARACTER);
}
/**
 * @brief Проверка приведения знаков, записанных двумя, тремя и четырьмя байтами
 *
 * @details Знаки, записанные двумя и тремя байтами, проверяются приведением на месте,
 *          без чтения кодового значения: письменности, на них записываемые, преобладают
 *          в разметке, и чтение значения обходилось бы там дороже самой проверки.
 *          Проверка на месте обязана отклонять ровно то же, что отклоняло чтение
 *
 */
TEST(CodecXmlEncoding, Sequences) {
	// Приведённый к кодировке UTF-8 текст
	string result;
	// Код ошибки приведения
	xml::error_t error = xml::error_t::NONE;
	// Определённая кодировка исходного текста
	xml::encoding_t enc = xml::encoding_t::NONE;
	// Выполняем проверку приведения знаков, записанных двумя байтами
	ASSERT_TRUE(convert(string("<a>Москва</a>"), 4096, result, error, enc)) << xml::message(error);
	// Выполняем проверку приведённого текста
	ASSERT_EQ(result, "<a>Москва</a>");
	// Выполняем проверку приведения знаков, записанных тремя байтами
	ASSERT_TRUE(convert(string("<a>日本語</a>"), 4096, result, error, enc)) << xml::message(error);
	// Выполняем проверку приведённого текста
	ASSERT_EQ(result, "<a>日本語</a>");
	// Выполняем проверку приведения знаков, записанных четырьмя байтами
	ASSERT_TRUE(convert(string("<a>\xF0\x9F\x98\x80</a>"), 4096, result, error, enc)) << xml::message(error);
	// Выполняем проверку приведённого текста
	ASSERT_EQ(result, "<a>\xF0\x9F\x98\x80</a>");
	// Выполняем проверку отклонения избыточно длинной записи знака тремя байтами
	ASSERT_FALSE(convert(string("<a>\xE0\x80\xAF</a>"), 4096, result, error, enc));
	// Выполняем проверку кода ошибки приведения
	ASSERT_EQ(error, xml::error_t::INVALID_ENCODING);
	// Выполняем проверку отклонения ошибочного продолжающего байта
	ASSERT_FALSE(convert(string("<a>\xE4\xB8\x28</a>"), 4096, result, error, enc));
	// Выполняем проверку кода ошибки приведения
	ASSERT_EQ(error, xml::error_t::INVALID_ENCODING);
	// Выполняем проверку отклонения неназначенного кодового значения
	ASSERT_FALSE(convert(string("<a>\xEF\xBF\xBE</a>"), 4096, result, error, enc));
	// Выполняем проверку кода ошибки приведения
	ASSERT_EQ(error, xml::error_t::INVALID_CHARACTER);
	// Выполняем проверку отклонения неназначенного кодового значения
	ASSERT_FALSE(convert(string("<a>\xEF\xBF\xBF</a>"), 4096, result, error, enc));
	// Выполняем проверку кода ошибки приведения
	ASSERT_EQ(error, xml::error_t::INVALID_CHARACTER);
	// Выполняем проверку приведения знаков, разорванных границей куска
	ASSERT_TRUE(convert(string("<a>Москва日本</a>"), 1, result, error, enc)) << xml::message(error);
	// Выполняем проверку приведённого текста
	ASSERT_EQ(result, "<a>Москва日本</a>");
}
/**
 * @brief Проверка распознавания поля кодировки среди похожих подстрок
 *
 * @details Слово «encoding» встречается и внутри значений соседних полей объявления
 *          разметки. Опознание его подстрокой уводило бы выбор кодировки на знаки,
 *          полем кодировки не являющиеся, - поле опознаётся по окружению: пробельный
 *          знак слева и знак равенства справа
 *
 */
TEST(CodecXmlEncoding, DeclarationFieldIsNotSubstring) {
	// Приведённый к кодировке UTF-8 текст
	string result;
	// Код ошибки приведения
	xml::error_t error = xml::error_t::NONE;
	// Определённая кодировка исходного текста
	xml::encoding_t enc = xml::encoding_t::NONE;
	// Выполняем приведение текста, где слово поля стоит и внутри значения соседнего поля
	ASSERT_TRUE(convert(string("<?xml version=\"1.0encoding\" encoding=\"iso-8859-1\"?><a>\xE9</a>"), 4096, result, error, enc));
	// Выполняем проверку кода ошибки приведения
	ASSERT_EQ(error, xml::error_t::NONE);
	// Выполняем проверку того, что кодировка взята из поля, а не из значения соседнего поля
	ASSERT_EQ(enc, xml::encoding_t::LATIN1);
	// Выполняем проверку приведённого текста
	ASSERT_EQ(result, "<?xml version=\"1.0encoding\" encoding=\"iso-8859-1\"?><a>\xC3\xA9</a>");
}
/**
 * @brief Проверка распознавания поля кодировки с пробелами вокруг знака равенства
 *
 */
TEST(CodecXmlEncoding, DeclarationFieldSpacedEquals) {
	// Приведённый к кодировке UTF-8 текст
	string result;
	// Код ошибки приведения
	xml::error_t error = xml::error_t::NONE;
	// Определённая кодировка исходного текста
	xml::encoding_t enc = xml::encoding_t::NONE;
	// Выполняем приведение текста, где знак равенства отделён от названия поля пробелами
	ASSERT_TRUE(convert(string("<?xml version=\"1.0\" encoding = \"iso-8859-1\"?><a>\xE9</a>"), 4096, result, error, enc));
	// Выполняем проверку кода ошибки приведения
	ASSERT_EQ(error, xml::error_t::NONE);
	// Выполняем проверку определённой кодировки
	ASSERT_EQ(enc, xml::encoding_t::LATIN1);
	// Выполняем проверку приведённого текста
	ASSERT_EQ(result, "<?xml version=\"1.0\" encoding = \"iso-8859-1\"?><a>\xC3\xA9</a>");
}
/**
 * @brief Проверка навязывания кодировки извне
 *
 */
TEST(CodecXmlEncoding, Forced) {
	// Приведённый к кодировке UTF-8 текст
	string result;
	// Объект приведения исходного текста к кодировке UTF-8
	xml::decoder_t decoder;
	// Выполняем навязывание кодировки исходного текста
	ASSERT_TRUE(decoder.encoding(xml::encoding_t::LATIN1));
	// Выполняем приведение исходного текста
	ASSERT_TRUE(decoder.convert("\xE9", 1, true, result));
	// Выполняем проверку приведённого текста
	ASSERT_EQ(result, "\xC3\xA9");
	// Выполняем проверку отказа смены кодировки после начала приведения
	ASSERT_FALSE(decoder.encoding(xml::encoding_t::UTF8));
}
/**
 * @brief Проверка разрядов знаков разметки
 *
 */
TEST(CodecXmlEncoding, Ranges) {
	// Выполняем проверку допустимости знаков разметки
	ASSERT_TRUE(xml::isChar(0x09));
	// Выполняем проверку допустимости знаков разметки
	ASSERT_TRUE(xml::isChar(0x10FFFF));
	// Выполняем проверку недопустимости управляющих знаков
	ASSERT_FALSE(xml::isChar(0x00));
	// Выполняем проверку недопустимости управляющих знаков
	ASSERT_FALSE(xml::isChar(0x0C));
	// Выполняем проверку недопустимости суррогатных кодовых значений
	ASSERT_FALSE(xml::isChar(0xD800));
	// Выполняем проверку допустимости знаков начала имени
	ASSERT_TRUE(xml::isNameStart('_'));
	// Выполняем проверку допустимости знаков начала имени
	ASSERT_TRUE(xml::isNameStart(0x430));
	// Выполняем проверку недопустимости цифр в начале имени
	ASSERT_FALSE(xml::isNameStart('1'));
	// Выполняем проверку недопустимости дефиса в начале имени
	ASSERT_FALSE(xml::isNameStart('-'));
	// Выполняем проверку допустимости цифр внутри имени
	ASSERT_TRUE(xml::isName('1'));
	// Выполняем проверку допустимости дефиса внутри имени
	ASSERT_TRUE(xml::isName('-'));
	// Выполняем проверку недопустимости пробела внутри имени
	ASSERT_FALSE(xml::isName(' '));
}
/**
 * @brief Проверка отклонения ошибочных построений в кодировках, кроме UTF-8
 *
 * @details Замер охвата показал, что до отказов приведения из кодировок UTF-16,
 *          ISO-8859-1 и US-ASCII не доходило ни одно испытание, а ворошитель их не
 *          достигает вовсе: он строит текст в UTF-8. Ход, ни разу не исполнявшийся,
 *          не проверен ничем — код ошибки в нём может не отвечать поводу
 *
 */
TEST(CodecXmlEncoding, MalformedEncodings) {
	// Приведённый к кодировке UTF-8 текст
	string result;
	// Код ошибки приведения
	xml::error_t error = xml::error_t::NONE;
	// Определённая кодировка исходного текста
	xml::encoding_t enc = xml::encoding_t::NONE;
	/**
	 * Выполняем проверку отклонения старшей половины суррогатной пары без младшей
	 *
	 * @note За старшей половиной стоит обычный знак: договор о всеобщей кодировке
	 *       требует именно младшей половины, и всякое иное значение построение рушит
	 */
	ASSERT_FALSE(convert(string("\xFF\xFE\x3D\xD8\x41\x00", 6), 4096, result, error, enc));
	// Выполняем проверку кода ошибки приведения
	ASSERT_EQ(error, xml::error_t::INVALID_ENCODING);
	// Выполняем проверку отклонения младшей половины суррогатной пары без старшей
	ASSERT_FALSE(convert(string("\xFF\xFE\x00\xDE\x41\x00", 6), 4096, result, error, enc));
	// Выполняем проверку кода ошибки приведения
	ASSERT_EQ(error, xml::error_t::INVALID_ENCODING);
	// Выполняем проверку отклонения знака, недопустимого в разметке
	ASSERT_FALSE(convert(string("\xFF\xFE\x01\x00", 4), 4096, result, error, enc));
	// Выполняем проверку кода ошибки приведения
	ASSERT_EQ(error, xml::error_t::INVALID_CHARACTER);
	/**
	 * Выполняем проверку отклонения текста, оборванного посреди знака
	 *
	 * @note Половина кодового значения удерживается до следующего куска, и конец
	 *       текста на ней означает оборванный знак, а не конец разметки
	 */
	ASSERT_FALSE(convert(string("\xFF\xFE\x41\x00\x42", 5), 4096, result, error, enc));
	// Выполняем проверку кода ошибки приведения
	ASSERT_EQ(error, xml::error_t::INVALID_ENCODING);
	// Выполняем проверку отклонения знака сверх US-ASCII вопреки объявленной кодировке
	ASSERT_FALSE(convert(string("<?xml version=\"1.0\" encoding=\"US-ASCII\"?><a>\xE9</a>"), 4096, result, error, enc));
	// Выполняем проверку кода ошибки приведения
	ASSERT_EQ(error, xml::error_t::INVALID_ENCODING);
	// Выполняем проверку отклонения управляющего знака в кодировке ISO-8859-1
	ASSERT_FALSE(convert(string("<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><a>\x01</a>"), 4096, result, error, enc));
	// Выполняем проверку кода ошибки приведения
	ASSERT_EQ(error, xml::error_t::INVALID_CHARACTER);
	// Выполняем проверку отклонения управляющего знака в кодировке US-ASCII
	ASSERT_FALSE(convert(string("<?xml version=\"1.0\" encoding=\"US-ASCII\"?><a>\x01</a>"), 4096, result, error, enc));
	// Выполняем проверку кода ошибки приведения
	ASSERT_EQ(error, xml::error_t::INVALID_CHARACTER);
	// Выполняем проверку отклонения текста, оборванного посреди знака UTF-8
	ASSERT_FALSE(convert(string("<a>\xD0</a>").substr(0, 4), 4096, result, error, enc));
	// Выполняем проверку кода ошибки приведения
	ASSERT_EQ(error, xml::error_t::INVALID_ENCODING);
}
/**
 * @brief Проверка независимости разбора от кодировки исходного текста
 *
 * @details Кодек обещает разбирать один и тот же текст одинаково, в какой бы из
 *          принимаемых кодировок он ни пришёл: приведение к UTF-8 ведётся до разбора,
 *          и ниже него кодировка исходного текста не различима вовсе. Сличается вся
 *          выдача, включая место каждого события, - место считается строками и
 *          знаками, а не байтами, и кодировкой разниться не вправе
 *
 */
TEST(CodecXmlEncoding, SameTextAcrossEncodings) {
	/**
	 * @brief Метод приведения текста разметки из UTF-8 в UTF-16
	 *
	 * @param source приводимый текст разметки в кодировке UTF-8
	 * @param big    признак прямого порядка байтов в приведённом тексте
	 * @return       приведённый текст разметки в кодировке UTF-16
	 *
	 */
	const auto transcode = [](const string & source, const bool big) noexcept -> string {
		// Собираемый текст разметки в кодировке UTF-16
		string result;
		// Выполняем добавление метки порядка байтов
		result.append(big ? "\xFE\xFF" : "\xFF\xFE", 2);
		/**
		 * Выполняем перебор всех знаков приводимого текста разметки
		 */
		for(size_t i = 0; i < source.size();){
			// Получаем очередной байт приводимого текста разметки
			const uint8_t byte = static_cast <uint8_t> (source[i]);
			// Значение знака, собираемое из байтов
			uint32_t code = byte;
			// Количество байтов, занятых знаком
			size_t length = 1;
			// Если знак записан четырьмя байтами
			if(byte >= 0xF0){ code = (byte & 0x07); length = 4; }
			// Если знак записан тремя байтами
			else if(byte >= 0xE0){ code = (byte & 0x0F); length = 3; }
			// Если знак записан двумя байтами
			else if(byte >= 0xC0){ code = (byte & 0x1F); length = 2; }
			/**
			 * Выполняем перебор всех продолжающих байтов знака
			 */
			for(size_t k = 1; k < length; k++)
				// Выполняем добавление разрядов продолжающего байта к значению знака
				code = ((code << 6) | (static_cast <uint8_t> (source[i + k]) & 0x3F));
			// Выполняем переход к следующему знаку приводимого текста
			i += length;
			// Пара шестнадцатибитных значений, записывающих знак
			uint16_t units[2] = {0, 0};
			// Количество шестнадцатибитных значений, занятых знаком
			size_t count = 1;
			/**
			 * Если знак лежит за пределами основного набора
			 */
			if(code >= 0x10000){
				// Получаем остаток значения знака сверх основного набора
				const uint32_t rest = (code - 0x10000);
				// Запоминаем старшее значение суррогатной пары
				units[0] = static_cast <uint16_t> (0xD800 + (rest >> 10));
				// Запоминаем младшее значение суррогатной пары
				units[1] = static_cast <uint16_t> (0xDC00 + (rest & 0x3FF));
				// Запоминаем количество значений, занятых знаком
				count = 2;
			// Запоминаем единственное значение, записывающее знак
			} else units[0] = static_cast <uint16_t> (code);
			/**
			 * Выполняем перебор всех значений, записывающих знак
			 */
			for(size_t k = 0; k < count; k++){
				// Получаем старший байт очередного значения
				const char high = static_cast <char> (units[k] >> 8);
				// Получаем младший байт очередного значения
				const char low = static_cast <char> (units[k] & 0xFF);
				// Выполняем добавление байтов значения в заданном порядке
				if(big){ result.push_back(high); result.push_back(low); }
				else { result.push_back(low); result.push_back(high); }
			}
		}
		// Выводим приведённый текст разметки
		return result;
	};
	/**
	 * @brief Метод сборки слепка выдачи разбора текста разметки
	 *
	 * @param text  разбираемый текст разметки
	 * @param chunk размер куска подачи текста разметки
	 * @return      собранный слепок выдачи разбора
	 *
	 */
	const auto events = [](const string & text, const size_t chunk) noexcept -> string {
		// Настройки разбора текста разметки
		xml::reader_t::settings_t settings;
		// Выполняем склеивание подряд идущих кусков содержимого
		settings.mergeText = true;
		// Объект потокового чтения текста разметки
		xml::reader_t reader;
		// Выполняем установку настроек разбора текста разметки
		reader.settings(settings);
		// Собираемый слепок выдачи разбора
		string result;
		// Положение подачи в разбираемом тексте разметки
		size_t offset = 0;
		/**
		 * Выполняем сборку слепка до исчерпания выдачи разбора
		 */
		while(true){
			/**
			 * Выполняем подачу текста разметки до получения очередного события
			 */
			while(!reader.next()){
				// Если разбор события не ждёт либо подавать больше нечего
				if((reader.state() != xml::state_t::HUNGRY) || (offset >= text.size()))
					// Выводим собранный слепок вместе с итогом разбора
					return result.append("=").append(std::to_string(static_cast <uint32_t> (reader.state())))
						.append("/").append(std::to_string(static_cast <uint32_t> (reader.error())));
				// Получаем размер очередного куска подачи текста разметки
				const size_t size = (((offset + chunk) > text.size()) ? (text.size() - offset) : chunk);
				// Выполняем подачу очередного куска текста разметки
				reader.feed(text.data() + offset, size, ((offset + size) >= text.size()));
				// Выполняем смещение положения подачи текста разметки
				offset += size;
			}
			// Дописываем к слепку разновидность полученного события
			result.append(std::to_string(static_cast <uint32_t> (reader.event()))).append(":");
			// Дописываем к слепку содержимое полученного события
			result.append(reader.text()).append("@");
			// Дописываем к слепку место начала полученного события
			result.append(std::to_string(reader.location().line)).append(",");
			result.append(std::to_string(reader.location().column)).append("|");
		}
	};
	// Разбираемый текст разметки в кодировке UTF-8
	const string text(
		"<r xmlns:ns=\"urn:example\" a=\"значение\">"
		"содержимое<ns:item/><!-- примечание --><![CDATA[дословно]]>\xF0\x9D\x84\x9E</r>"
	);
	// Слепок выдачи разбора текста разметки в кодировке UTF-8
	const string sample = events(text, text.size());
	// Выполняем проверку того, что разбор исходного текста удался
	ASSERT_NE(sample.find("=2/0"), string::npos) << sample;
	/**
	 * Выполняем перебор обоих порядков байтов кодировки UTF-16
	 */
	for(const bool big : {false, true}){
		// Выполняем приведение текста разметки к кодировке UTF-16
		const string wide = transcode(text, big);
		/**
		 * Выполняем перебор размеров куска подачи текста разметки
		 *
		 * @note Однобайтовый кусок разрывает всякое шестнадцатибитное значение, а
		 *       трёхбайтовый - ещё и всякую суррогатную пару, не совпадая с её длиной
		 */
		for(const size_t chunk : {wide.size(), static_cast <size_t> (1), static_cast <size_t> (3)})
			// Выполняем проверку совпадения выдачи разбора с выдачей по кодировке UTF-8
			ASSERT_EQ(events(wide, chunk), sample) << big << " " << chunk;
	}
	// Разбираемый текст разметки в кодировке ISO-8859-1
	const string latin("<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><r a=\"caf\xE9\">\xC7</r>");
	// Слепок выдачи разбора текста разметки в кодировке ISO-8859-1
	const string second = events(latin, latin.size());
	// Выполняем проверку того, что разбор текста в кодировке ISO-8859-1 удался
	ASSERT_NE(second.find("=2/0"), string::npos) << second;
	/**
	 * Выполняем перебор размеров куска подачи текста разметки
	 */
	for(size_t chunk = 1; chunk < 8; chunk++)
		// Выполняем проверку независимости выдачи разбора от нарезки на куски
		ASSERT_EQ(events(latin, chunk), second) << chunk;
}
