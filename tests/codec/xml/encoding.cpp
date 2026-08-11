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
