/**
 * @file encoding.cpp
 * @date 2026-08-01
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
 * @brief Автоматические тесты приведения исходного текста разметки к кодировке UTF-8 — определение
 *        кодировки по метке порядка байтов и по объявлению разметки, сборка суррогатных пар,
 *        отклонение ошибочных последовательностей и разрыв последовательностей границей куска
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/xml/encoding.hpp>
#include <codec/xml/reader.hpp>
#include <codec/xml/document.hpp>
#include <codec/xml/writer.hpp>

/**
 * Подключаем заголовочные файлы тестового окружения
 */
#include "../../main.hpp"

/**
 * @brief Пространство имён проверок этого файла
 *
 * @note Держится оно безымянным намеренно: проверки кодеков собираются одной
 *       программою, и одноимённые построения разных файлов иначе сходятся в
 *       одно, порождая порчу вдали от места её причины
 *
 */
namespace {
	/**
	 * @brief Объект журнала проверок с отключённым выводом
	 *
	 * @details Вывод отключается назначением пустого перечня приёмников: отказы
	 *          разбора проверки наводят намеренно, и журнал их засорял бы выдачу
	 *
	 */
	struct Silent {
		/**
		 * @brief Функция получения объекта фреймворка проверок
		 *
		 * @details Объект заводится статикою местною, а не общею файла: заведение его
		 *          порядком построения статики оканчивается падением ещё до входа в
		 *          проверки, ибо фреймворк сам опирается на статику из библиотеки
		 *
		 * @return объект фреймворка проверок
		 *
		 */
		static const awh::fmk_t & framework() noexcept {
			// Объект фреймворка проверок
			static awh::fmk_t fmk;
			// Выводим объект фреймворка проверок
			return fmk;
		}
		// Объект журнала проверок
		awh::log_t log;
		/**
		 * @brief Конструктор
		 *
		 */
		Silent() noexcept : log(&Silent::framework()) {
			// Выполняем отключение вывода логов
			this->log.mode({});
		}
	};
	/**
	 * @brief Функция получения объекта журнала проверок
	 *
	 * @return объект журнала проверок
	 *
	 */
	const awh::log_t * logger() noexcept {
		// Объект журнала проверок
		static Silent silent;
		// Выводим объект журнала проверок
		return &silent.log;
	}
}

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
	xml::decoder_t decoder(::logger());
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
	/**
	 * Выполняем приведение текста с кодировкою, приведению неизвестной
	 *
	 * @note Прежде здесь стояла «KOI8-R», и решение то ПЕРЕСМОТРЕНО 23.08.2026: всякая
	 *       однобайтовая кодировка ныне берётся таблицею у `awh::charset`, и KOI8-R
	 *       читается. Отвергается лишь то, чего не знает и общий модуль, - имя, кодировки
	 *       не значащее вовсе
	 */
	ASSERT_FALSE(convert(string("<?xml version=\"1.0\" encoding=\"выдуманная\"?><a/>"), 4096, result, error, enc));
	// Выполняем проверку кода ошибки приведения
	ASSERT_EQ(error, xml::error_t::UNSUPPORTED_ENCODING);
	// Выполняем сброс кода ошибки приведения
	error = xml::error_t::NONE;
	/**
	 * Выполняем приведение текста с кодировкою многобайтовой, своей ветви не имеющей
	 *
	 * @warning Отвергать её обязательно: таблицы байта в знак у неё нет, и разбор её
	 *          однобайтовым способом выдал бы вместо текста мусор
	 */
	ASSERT_FALSE(convert(string("<?xml version=\"1.0\" encoding=\"UTF-32\"?><a/>"), 4096, result, error, enc));
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
	xml::decoder_t decoder(::logger());
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
		xml::reader_t reader(::logger());
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

/**
 * @brief Проверка отказа разметке, оборванной посреди знака
 *
 * @details Путь этот не звался ни одной проверкой. Последовательность знака UTF-8,
 *          оборванная посреди, обязана быть отвергнута: принять её значило бы завести в
 *          дереве содержимое, кодировке не отвечающее вовсе
 *
 */
TEST(CodecXmlEncoding, TruncatedCharacter) {
	// Собираемый текст разметки с оборванным знаком в содержимом
	string text = "<a>";
	// Добавляем знак кириллицы целиком
	text.append("\xD0\xB0");
	// Добавляем начало знака кириллицы без хвоста его
	text.push_back('\xD0');
	// Дописываем закрывающую метку узла разметки
	text.append("</a>");
	// Объект потокового чтения разметки
	xml::reader_t reader(::logger());
	// Выполняем подачу текста разметки целиком
	ASSERT_TRUE(reader.feed(text.data(), text.size(), true));
	/**
	 * Выполняем перебор всех событий разбора
	 *
	 * @note Отказ выносится не подачей, а разбором: подача лишь принимает байты, тогда
	 *       как кодировка сличается при выдаче события
	 */
	while(reader.next());
	// Выполняем проверку того, что отказ вынесен именно по кодировке
	ASSERT_EQ(reader.error(), xml::error_t::INVALID_ENCODING);
}

/**
 * @brief Проверка сличения объявленной кодировки с меткой порядка байтов
 *
 * @details Объявление `UTF-16` порядка байтов не задаёт вовсе: договор велит определять
 *          его меткою в начале текста, и запись эта отвечает обоим порядкам. Сличение
 *          принимало её за прямой порядок и отвергало всякий текст с обратным - то есть
 *          отвергало совершенно правильные документы, каких большинство: имя `UTF-16`
 *          пишут в объявлении куда чаще, чем `UTF-16LE`
 *
 * @note Проверка держит обе стороны: правильное принимается, а объявление, порядок
 *       задающее и с меткою расходящееся, по-прежнему отвергается
 *
 */
TEST(CodecXmlEncoding, ByteOrderMarkAgreement) {
	/**
	 * @brief Функция сборки текста разметки в кодировке UTF-16
	 *
	 * @param text    собираемый текст разметки
	 * @param reverse признак обратного порядка байтов
	 * @return        собранный текст разметки с меткой порядка байтов
	 *
	 */
	auto compose = [](const string & text, const bool reverse) -> string {
		// Собираемый текст разметки
		string result;
		// Выполняем добавление метки порядка байтов
		result.push_back(reverse ? '\xFF' : '\xFE');
		result.push_back(reverse ? '\xFE' : '\xFF');
		/**
		 * Выполняем перебор всех знаков собираемого текста
		 */
		for(const char letter : text){
			// Выполняем добавление знака в затребованном порядке байтов
			result.push_back(reverse ? letter : '\0');
			result.push_back(reverse ? '\0' : letter);
		}
		// Выводим собранный текст разметки
		return result;
	};
	/**
	 * @brief Функция разбора собранного текста разметки
	 *
	 * @param text разбираемый текст разметки
	 * @return     код отказа разбора
	 *
	 */
	auto outcome = [](const string & text) -> xml::error_t {
		// Объект потокового чтения разметки
		xml::reader_t reader(::logger());
		// Выполняем подачу текста разметки целиком
		reader.feed(text.data(), text.size(), true);
		// Выполняем перебор всех событий разбора
		while(reader.next());
		// Выводим код отказа разбора
		return reader.error();
	};
	// Выполняем проверку того, что объявление без порядка байтов годится обоим порядкам
	ASSERT_EQ(outcome(compose("<?xml version=\"1.0\" encoding=\"UTF-16\"?><a/>", true)), xml::error_t::NONE);
	ASSERT_EQ(outcome(compose("<?xml version=\"1.0\" encoding=\"UTF-16\"?><a/>", false)), xml::error_t::NONE);
	// Выполняем проверку того, что имя кодировки сличается без учёта разности написания
	ASSERT_EQ(outcome(compose("<?xml version=\"1.0\" encoding=\"utf-16\"?><a/>", false)), xml::error_t::NONE);
	// Выполняем проверку того, что объявление с порядком, метке отвечающим, принимается
	ASSERT_EQ(outcome(compose("<?xml version=\"1.0\" encoding=\"UTF-16LE\"?><a/>", true)), xml::error_t::NONE);
	ASSERT_EQ(outcome(compose("<?xml version=\"1.0\" encoding=\"UTF-16BE\"?><a/>", false)), xml::error_t::NONE);
	// Выполняем проверку того, что объявление с порядком, метке противоречащим, отвергается
	ASSERT_EQ(outcome(compose("<?xml version=\"1.0\" encoding=\"UTF-16BE\"?><a/>", true)), xml::error_t::INVALID_ENCODING);
	ASSERT_EQ(outcome(compose("<?xml version=\"1.0\" encoding=\"UTF-16LE\"?><a/>", false)), xml::error_t::INVALID_ENCODING);
	// Выполняем проверку того, что объявление иной кодировки при метке UTF-16 отвергается
	ASSERT_EQ(outcome(compose("<?xml version=\"1.0\" encoding=\"UTF-8\"?><a/>", true)), xml::error_t::INVALID_ENCODING);
}
/**
 * @brief Проверка расхождения объявленной кодировки с записью самого текста
 *
 * @details Кодировка определяется тремя источниками: меткой порядка байтов,
 *          объявлением разметки и записью первых байтов текста. Расхождения между
 *          ними договором не дозволены, и всякое из них - отказ. Карта покрытия
 *          показала обе ветви отказа нетронутыми
 *
 * @note Метка порядка байтов достовернее объявления: объявление является частью
 *       текста и прочитано быть не может, пока кодировка неизвестна. Оттого
 *       расхождение означает подложное объявление, а не выбор между источниками
 *
 */
TEST(CodecXmlEncoding, DeclarationDisagreement) {
	// Приведённый текст разметки
	string result;
	// Код ошибки приведения
	xml::error_t error = xml::error_t::NONE;
	// Определённая кодировка исходного текста
	xml::encoding_t enc = xml::encoding_t::NONE;
	/**
	 * Выполняем перебор всех размеров подаваемого куска
	 *
	 * @note Отказ обязан выноситься при всяком размере куска: объявление разметки
	 *       границей куска разрывается, и накопление его ведётся отдельно
	 */
	for(size_t step = 1; step < 8; step++){
		// Выполняем проверку отказа объявления двухбайтовой кодировки при однобайтовой записи
		ASSERT_FALSE(::convert("<?xml version='1.0' encoding='UTF-16'?><a/>", step, result, error, enc)) << step;
		// Выполняем проверку кода ошибки приведения
		ASSERT_EQ(error, xml::error_t::INVALID_ENCODING) << step;
		// Выполняем проверку отказа объявления, расходящегося с меткой порядка байтов
		ASSERT_FALSE(::convert("\xEF\xBB\xBF<?xml version='1.0' encoding='ISO-8859-1'?><a/>", step, result, error, enc)) << step;
		// Выполняем проверку кода ошибки приведения
		ASSERT_EQ(error, xml::error_t::INVALID_ENCODING) << step;
	}
}
/**
 * @brief Проверка отказов приведения кодировки через поток чтения
 *
 * @note Негодная запись знаков подаётся ЧЕРЕЗ ЧТЕНИЕ, а не приведению напрямую:
 *       так проходится дорога, какой пользуется всякий разбор, и разорванные
 *       границей куска последовательности удерживаются между подачами
 * @warning Годная половина важнее негодной: ошибка на этой дороге отвергала бы
 *          законные тексты, разбираемые прямо из буфера потребителя
 */
TEST(CodecXmlEncoding, ReaderRefusals) {
	/**
	 * Способ подачи текста разметки чтению кусками заданного размера
	 *
	 * @param text разбираемый текст разметки
	 * @param step размер куска подаваемого текста
	 * @return     код ошибки разбора
	 */
	auto feed = [](const string & text, const size_t step) noexcept -> xml::error_t {
		// Объект потокового чтения текста разметки
		xml::reader_t reader(::logger());
		// Положение подачи в разбираемом тексте
		size_t offset = 0;
		/**
		 * Выполняем подачу текста разметки кусками
		 */
		for(;;){
			// Получаем размер очередного куска текста
			const size_t size = ((offset + step) > text.size() ? (text.size() - offset) : step);
			// Получаем признак последнего куска текста
			const bool end = ((offset + size) >= text.size());
			/**
			 * Если передачу куска текста выполнить не удалось
			 */
			if(!reader.feed(text.data() + offset, size, end))
				// Выходим из подачи текста
				break;
			// Выполняем переход к следующему куску текста
			offset += size;
			/**
			 * Выполняем перебор всех событий разбора
			 */
			while(reader.next());
			/**
			 * Если кусок текста являлся последним
			 */
			if(end)
				// Выходим из подачи текста
				break;
		}
		// Выводим код ошибки разбора
		return reader.error();
	};
	/**
	 * Выполняем перебор всех негодных записей знаков
	 */
	for(auto & text : vector <string> {
		// Хвостовой байт без ведущего
		string("<a>\x82</a>"),
		// Последовательность знака, оборванная содержимым
		string("<a>\xE2\x82</a>"),
		// Суррогат, записанный в кодировке UTF-8
		string("<a>\xED\xA0\x80</a>"),
		// Последовательность знака, оборванная концом текста
		string("<a>\xE2\x82")
	}){
		// Выполняем проверку отклонения записи при подаче текста целиком
		ASSERT_EQ(feed(text, 4096), xml::error_t::INVALID_ENCODING);
		/**
		 * Выполняем проверку отклонения записи при подаче текста по одному байту
		 *
		 * @note Подача по байту разрывает всякую последовательность знака: отказ
		 *       обязан выноситься тот же самый, иначе нарезка текста на куски меняла
		 *       бы исход разбора
		 */
		ASSERT_EQ(feed(text, 1), xml::error_t::INVALID_ENCODING);
	}
	// Выполняем проверку принятия годной записи знака при подаче по одному байту
	ASSERT_EQ(feed(string("<a>\xE2\x82\xAC</a>"), 1), xml::error_t::NONE);
	/**
	 * Опознание кодировки UTF-16 без метки порядка байтов
	 *
	 * @note Кодировка опознаётся по самому объявлению разметки: знаки «<?» в записи
	 *       UTF-16 дают приметную пару байтов с нулём, и по ней порядок байтов
	 *       определяется без всякой метки
	 */
	{
		// Собираемый текст разметки в кодировке UTF-16 с младшим байтом впереди
		string little;
		// Собираемый текст разметки в кодировке UTF-16 со старшим байтом впереди
		string big;
		/**
		 * Выполняем перебор знаков текста разметки
		 */
		for(auto & letter : string("<?xml version=\"1.0\"?><a/>")){
			// Выполняем запись знака младшим байтом впереди
			little.push_back(letter);
			// Выполняем дополнение знака старшим байтом
			little.push_back('\0');
			// Выполняем дополнение знака старшим байтом
			big.push_back('\0');
			// Выполняем запись знака младшим байтом следом
			big.push_back(letter);
		}
		// Выполняем проверку принятия текста с младшим байтом впереди
		ASSERT_EQ(feed(little, 4096), xml::error_t::NONE);
		// Выполняем проверку принятия текста со старшим байтом впереди
		ASSERT_EQ(feed(big, 4096), xml::error_t::NONE);
	}
}

/**
 * @brief Проверка приведения кодировки на границах кусков и по видам кодировок
 *
 * @details Места эти отысканы по карте покрытия: приведение держит недочитанную
 *          последовательность знака от куска к куску, и вся дорога её доработки набором не
 *          проходилась ни разу
 *
 * @warning Первый кусок здесь заведомо длиннее объявления разметки: пока кодировка не
 *          определена, приведение удерживает поданное целиком и до удержания недочитанной
 *          последовательности дело не доходит. Короткий первый кусок увёл бы проверку
 *          мимо закрепляемого места, ничем себя не выдав
 *
 */
TEST(CodecXmlEncoding, ChunkBoundaryAndKinds) {
	// Начало текста разметки, кодировку определяющее
	const string head("<?xml version=\"1.0\" encoding=\"UTF-8\"?><a>xxxxxxxxxx");
	/**
	 * Знак, разметке недопустимый, обнаруживается и через границу куска
	 */
	{
		// Объект приведения кодировки исходного текста
		xml::decoder_t decoder(::logger());
		// Приведённый текст
		string result;
		// Выполняем подачу куска, оборванного посреди последовательности знака
		ASSERT_TRUE(decoder.convert((head + "\xEF").data(), head.size() + 1, false, result));
		// Выполняем подачу остатка последовательности знака
		ASSERT_FALSE(decoder.convert("\xBF\xBE", 2, true, result));
		// Выполняем проверку кода ошибки приведения
		ASSERT_EQ(decoder.error(), xml::error_t::INVALID_CHARACTER);
	}
	/**
	 * Последовательность, построенная ошибочно, обнаруживается и через границу куска
	 */
	{
		// Объект приведения кодировки исходного текста
		xml::decoder_t decoder(::logger());
		// Приведённый текст
		string result;
		// Выполняем подачу куска, оборванного посреди последовательности знака
		ASSERT_TRUE(decoder.convert((head + "\xED").data(), head.size() + 1, false, result));
		// Выполняем подачу остатка, дающего суррогатное кодовое значение
		ASSERT_FALSE(decoder.convert("\xA0\x80", 2, true, result));
		// Выполняем проверку кода ошибки приведения
		ASSERT_EQ(decoder.error(), xml::error_t::INVALID_ENCODING);
	}
	/**
	 * Последовательность, оборванная концом текста, отвергается
	 *
	 * @note Оборванная НЕ на конце отказа не даёт: остаток её вправе прийти следующим
	 *       куском, и решение откладывается до признака последнего куска
	 */
	{
		// Объект приведения кодировки исходного текста
		xml::decoder_t decoder(::logger());
		// Приведённый текст
		string result;
		// Выполняем подачу куска, оборванного посреди последовательности знака
		ASSERT_TRUE(decoder.convert((head + "\xE2").data(), head.size() + 1, false, result));
		// Выполняем подачу остатка, последовательность не завершающего
		ASSERT_FALSE(decoder.convert("\x82", 1, true, result));
		// Выполняем проверку кода ошибки приведения
		ASSERT_EQ(decoder.error(), xml::error_t::INVALID_ENCODING);
	}
	/**
	 * Знак, разметке недопустимый, отвергается при всякой кодировке
	 */
	{
		// Выполняем перебор кодировок и недопустимых знаков в них
		for(uint32_t kind = 0; kind < 4; kind++){
			// Объект приведения кодировки исходного текста
			xml::decoder_t decoder(::logger());
			// Приведённый текст
			string result;
			// Приводимый кусок исходного текста
			string chunk;
			/**
			 * Определяем кодировку и недопустимый знак в ней
			 */
			switch(kind){
				// Кодировка UTF-8 со знаком, отведённым договором Юникода
				case 0: chunk.assign("a\xEF\xBF\xBE", 4); break;
				// Кодировка UTF-16 с обратным порядком байтов
				case 1: {
					ASSERT_TRUE(decoder.encoding(xml::encoding_t::UTF16LE));
					chunk.assign("a\x00\x01\x00", 4);
				} break;
				// Кодировка ISO-8859-1 с управляющим знаком
				case 2: {
					ASSERT_TRUE(decoder.encoding(xml::encoding_t::LATIN1));
					chunk.assign("a\x01", 2);
				} break;
				// Кодировка US-ASCII с управляющим знаком
				case 3: {
					ASSERT_TRUE(decoder.encoding(xml::encoding_t::ASCII));
					chunk.assign("a\x01", 2);
				} break;
			}
			// Выполняем проверку отказа приведения
			ASSERT_FALSE(decoder.convert(chunk.data(), chunk.size(), true, result)) << kind;
			// Выполняем проверку кода ошибки приведения
			ASSERT_EQ(decoder.error(), xml::error_t::INVALID_CHARACTER) << kind;
		}
	}
	/**
	 * Приведение, единожды прекращённое ошибкой, отвергает и дальнейшее
	 */
	{
		// Объект приведения кодировки исходного текста
		xml::decoder_t decoder(::logger());
		// Приведённый текст
		string result;
		// Выполняем подачу негодной последовательности байтов
		ASSERT_FALSE(decoder.convert("\xC3\x28", 2, true, result));
		// Выполняем проверку отказа приведения годного текста следом
		ASSERT_FALSE(decoder.convert("a", 1, true, result));
	}
	/**
	 * Вхождение слова «encoding» без знака равенства полем не является
	 *
	 * @note Объявление разметки разбирается поиском слова, и найденное вхождение обязано
	 *       быть полем: без сличения со знаком равенства слово, стоящее в объявлении само
	 *       по себе, приняли бы за поле кодировки и разбирали бы то, чего там нет
	 */
	{
		// Объект приведения кодировки исходного текста
		xml::decoder_t decoder(::logger());
		// Приведённый текст
		string result;
		// Собираем объявление разметки со словом без знака равенства
		const string text("<?xml version=\"1.0\" encoding ?><a/>");
		// Выполняем приведение исходного текста
		ASSERT_TRUE(decoder.convert(text.data(), text.size(), true, result));
		// Выполняем проверку отсутствия ошибки приведения
		ASSERT_EQ(decoder.error(), xml::error_t::NONE);
		// Выполняем проверку определения кодировки по умолчанию
		ASSERT_EQ(decoder.encoding(), xml::encoding_t::UTF8);
	}
	/**
	 * Навязанная извне кодировка снимается подачею неопределённой
	 */
	{
		// Объект приведения кодировки исходного текста
		xml::decoder_t decoder(::logger());
		// Выполняем навязывание кодировки извне
		ASSERT_TRUE(decoder.encoding(xml::encoding_t::UTF8));
		// Выполняем проверку установки навязанной кодировки
		ASSERT_EQ(decoder.encoding(), xml::encoding_t::UTF8);
		// Выполняем снятие навязанной кодировки
		ASSERT_TRUE(decoder.encoding(xml::encoding_t::NONE));
		// Выполняем проверку снятия навязанной кодировки
		ASSERT_EQ(decoder.encoding(), xml::encoding_t::NONE);
	}
}

/**
 * @brief Проверка разбора однобайтовых кодировок таблицею общего модуля
 *
 * @details Свой перечень кодировок несёт лишь договорные, разбираемые своими ветвями.
 * Всякая же однобайтовая кодировка разбирается одинаково - байт в кодовое значение по
 * таблице, - и таблицы берутся у `awh::charset`
 *
 * @note Прежде разметка с объявлением `windows-1251` либо `koi8-r` отвергалась наотрез,
 *       хотя таблицы для неё лежали в дереве готовые. Решение пересмотрено 23.08.2026,
 *       и запись о нём стоит в шапке `common.hpp`
 * @warning Проверяются ОБЕ половины: опознанное имя обязано читаться, а неопознанное -
 *          отвергаться. Набор из одной лишь первой половины прошёл бы и у разбора,
 *          принимающего что угодно и читающего текст вслепую как US-ASCII
 */
TEST(CodecXmlEncoding, SingleByteEncodingsFromCharset){
	/**
	 * Выполняем перебор всех однобайтовых кодировок, читаться обязанных
	 *
	 * @note Содержимое у всех одно и то же слово «Привет», записанное каждой кодировкой
	 *       по-своему: сличается именно ПРОЧТЁННОЕ, а не одно лишь отсутствие отказа
	 */
	for(auto & item : vector <pair <string, string>> {
		{"windows-1251", "\xCF\xF0\xE8\xE2\xE5\xF2"},
		{"koi8-r",       "\xF0\xD2\xC9\xD7\xC5\xD4"},
		{"ISO-8859-5",   "\xBF\xE0\xD8\xD2\xD5\xE2"},
		{"cp866",        "\x8F\xE0\xA8\xA2\xA5\xE2"}
	}) {
		// Дерево разметки
		xml::document_t document(::logger());
		// Собираем текст разметки с объявлением очередной кодировки
		const string text = ("<?xml version=\"1.0\" encoding=\"" + item.first + "\"?><a>" + item.second + "</a>");
		// Выполняем разбор текста разметки
		ASSERT_TRUE(document.parse(text)) << item.first << ": " << xml::message(document.error());
		// Выполняем сличение прочтённого содержимого
		ASSERT_EQ(string(document.root().child("a").text()), "Привет") << item.first;
	}
	/**
	 * Выполняем перебор всех имён кодировок, отвергаться обязанных
	 */
	for(auto & item : vector <string> {
		// Имя, не значащее кодировки вовсе
		"выдуманная",
		/**
		 * Кодировка многобайтовая, своей ветви разбора не имеющая
		 *
		 * @note Отвергать её обязательно: таблицы байта в знак у неё нет, и разбор её
		 *       однобайтовым способом выдал бы вместо текста мусор
		 */
		"UTF-32"
	}) {
		// Дерево разметки
		xml::document_t document(::logger());
		// Собираем текст разметки с объявлением неподдерживаемой кодировки
		const string text = ("<?xml version=\"1.0\" encoding=\"" + item + "\"?><a>x</a>");
		// Выполняем проверку отказа разбора
		ASSERT_FALSE(document.parse(text)) << item;
		// Выполняем проверку кода отказа
		ASSERT_EQ(document.error(), xml::error_t::UNSUPPORTED_ENCODING) << item;
	}
}

/**
 * @brief Пространство имён внешнего судьи годности UTF-8
 *
 * @note Держится оно безымянным намеренно: проверки кодеков собираются одной
 *       программою, и одноимённые построения разных файлов иначе сходятся в одно
 *
 * @note Судья этот повторяет собою тот, что стоит у проверок JSON. Повтор намеренный
 *       и неустранимый: общего заголовка у проверок нет, а вынести судью в кодек
 *       значило бы лишить его звания ВНЕШНЕГО - он стал бы частью проверяемого
 *
 */
namespace {
	/**
	 * @brief Метод чтения кодового значения внешним судьёй
	 *
	 * @details Судья писан по описанию Юникода и ни одному кодеку не принадлежит.
	 * Кратчайшесть записи, суррогаты и предел U+10FFFF отсекаются пределами первого
	 * продолжающего байта - ровно так, как то задано таблицей описания, а не проверками
	 * над собранным значением
	 *
	 * @param buffer буфер читаемой последовательности
	 * @param size   размер буфера читаемой последовательности
	 * @param length длина годной последовательности, ноль при негодной
	 * @return       прочитанное кодовое значение
	 *
	 */
	uint32_t judge(const unsigned char * buffer, const size_t size, size_t & length, const bool blind = false) noexcept {
		// Сбрасываем длину прочитанной последовательности
		length = 0;
		// Если читать нечего
		if(size == 0) return 0;
		// Получаем ведущий байт последовательности
		const unsigned char letter = buffer[0];
		// Количество продолжающих байтов последовательности
		size_t count = 0;
		// Пределы первого продолжающего байта последовательности
		unsigned char lower = 0x80, upper = 0xBF;
		// Собираемое кодовое значение знака
		uint32_t code = 0;
		/**
		 * Определяем ведущий байт очередной последовательности
		 */
		if(letter < 0x80){
			// Запоминаем длину прочитанной последовательности
			length = 1;
			// Выводим кодовое значение знака US-ASCII
			return letter;
		/**
		 * Если судья ослеплён самопроверкою
		 *
		 * @note Ослепление впускает переудлинённую двухбайтовую запись - ведущие байты
		 *       `C0` и `C1`, договором отведённые вовсе. Иного оно не меняет, и число
		 *       расхождений от него ПРЕДСКАЗУЕМО: по 64 продолжающих байта на каждый из
		 *       двух ведущих, итого 128 при исчерпывающем переборе двух байтов
		 */
		} else if(blind && ((letter == 0xC0) || (letter == 0xC1))){ count = 1; code = (letter & 0x1F); }
		else if((letter >= 0xC2) && (letter <= 0xDF)){ count = 1; code = (letter & 0x1F); }
		else if(letter == 0xE0){ count = 2; lower = 0xA0; code = (letter & 0x0F); }
		else if((letter >= 0xE1) && (letter <= 0xEC)){ count = 2; code = (letter & 0x0F); }
		else if(letter == 0xED){ count = 2; upper = 0x9F; code = (letter & 0x0F); }
		else if((letter >= 0xEE) && (letter <= 0xEF)){ count = 2; code = (letter & 0x0F); }
		else if(letter == 0xF0){ count = 3; lower = 0x90; code = (letter & 0x07); }
		else if((letter >= 0xF1) && (letter <= 0xF3)){ count = 3; code = (letter & 0x07); }
		else if(letter == 0xF4){ count = 3; upper = 0x8F; code = (letter & 0x07); }
		// Выводим признак негодной последовательности
		else return 0;
		// Если последовательность оборвана концом буфера
		if(size < (count + 1)) return 0;
		/**
		 * Выполняем перебор всех продолжающих байтов последовательности
		 */
		for(size_t i = 1; i <= count; i++){
			// Получаем очередной продолжающий байт последовательности
			const unsigned char next = buffer[i];
			// Получаем пределы очередного продолжающего байта
			const unsigned char from = ((i == 1) ? lower : 0x80), till = ((i == 1) ? upper : 0xBF);
			// Если продолжающий байт вышел за свои пределы
			if((next < from) || (next > till)) return 0;
			// Дописываем разряды продолжающего байта к кодовому значению
			code = ((code << 6) | (next & 0x3F));
		}
		// Запоминаем длину прочитанной последовательности
		length = (count + 1);
		// Выводим собранное кодовое значение знака
		return code;
	}
}

/**
 * @brief Проверка согласия трёх разбирателей UTF-8 с внешним судьёй
 *
 * @details Разбиратель последовательностей UTF-8 сличается здесь не с соседним кодеком,
 * а с судьёй, ни одному кодеку не принадлежащим. Сличение кодеков между собою было бы
 * кругом иного вида: согласие нескольких ничего не говорит о правоте, если правило у них
 * позаимствовано одно у другого
 *
 * @note Проверка держится самодостаточной НАМЕРЕННО: отдельный стенд этого кодека
 *       собирается без прочих, и обращение к соседнему разбирателю ломало бы его
 *       связывание. Судья потому и повторён в проверках трёх кодеков, а не вынесен в
 *       общее место
 *
 * @note Сличается ПРИГОВОР о годности, а не длина: длину при негодной последовательности
 *       судья и кодеки договорили по-разному, и сличение её мерило бы договорённость.
 *       У годной же последовательности сличаются и кодовое значение, и длина
 *
 * @note Замер щупом 31.08.2026 исчерпал ВСЕ последовательности длиною до трёх байтов -
 *       16 843 008 на кодек - и 2 102 784 четырёхбайтовых СРАЗУ У ТРЁХ кодеков:
 *       расхождений ноль у всех. Зрячесть доказана порчею ОДНОГО из трёх: снятие
 *       отсечения суррогатов дало ровно 2048 расхождений у него и ни одного у двух
 *       прочих - это доказывает заодно, что щуп зовёт каждого отдельно. Здесь перебор
 *       сокращён до
 *       исчерпывающего двухбайтового и всех граничных троек с четвёрками: проверки
 *       собираются с покрытием кода, и полный перебор стоил бы минут
 *
 */
TEST(CodecXmlEncoding, DecodersAgreeWithExternalJudge) {
	// Количество расхождений приговора, кодового значения и длины
	size_t verdicts = 0, codes = 0, lengths = 0;
	/**
	 * @brief Метод сличения приговора кодеков с приговором судьи
	 *
	 * @param buffer сличаемая последовательность байтов
	 * @param size   размер сличаемой последовательности
	 *
	 */
	const auto compare = [&](const unsigned char * buffer, const size_t size) noexcept {
		// Длина годной последовательности по приговору судьи
		size_t expected = 0;
		// Кодовое значение по приговору судьи
		const uint32_t wanted = ::judge(buffer, size, expected);
		// Получаем указание на сличаемую последовательность
		const char * text = reinterpret_cast <const char *> (buffer);
		// Длина последовательности по приговору кодека
		size_t length = 0;
		// Получаем кодовое значение приговором разбирателя кодека
		const uint32_t code = xml::decode(text, size, length);
		// Если приговоры о годности разошлись
		if((code == xml::INVALID_CODEPOINT) != (expected == 0)) verdicts++;
		/**
		 * Если последовательность годна
		 */
		else if(expected != 0){
			// Если разошлись кодовые значения
			if(code != wanted) codes++;
			// Если разошлись длины
			else if(length != expected) lengths++;
		}
	};
	// Сличаемая последовательность байтов
	unsigned char buffer[4] = {0};
	/**
	 * Выполняем самопроверку зрячести ПРЕЖДЕ всякого утверждения о кодеке
	 *
	 * @details Судья ослепляется нарочно - впускает переудлинённую двухбайтовую запись, -
	 * и сличение обязано дать РОВНО 128 расхождений: по 64 продолжающих байта на каждый
	 * из двух отведённых ведущих. Не сойдётся - значит сличение не сличает вовсе, и
	 * зелень его о кодеке не говорит ничего
	 *
	 * @note Приём взят у Геннадия, владельца кодека ABC: у него стенд портит судью сам
	 *       и требует точного числа прежде, чем отчитаться. Ручная порча доказывает
	 *       зрячесть однажды, встроенная - каждым прогоном
	 */
	{
		// Количество расхождений ослеплённого судьи с кодеком
		size_t blinded = 0;
		/**
		 * Выполняем исчерпывающий перебор всех последовательностей двух байтов
		 */
		for(uint32_t first = 0; first < 256; first++){
			buffer[0] = static_cast <unsigned char> (first);
			for(uint32_t second = 0; second < 256; second++){
				buffer[1] = static_cast <unsigned char> (second);
				// Длина годной последовательности по приговору ослеплённого судьи
				size_t length = 0;
				// Выполняем чтение последовательности ослеплённым судьёю
				::judge(buffer, 2, length, true);
				// Длина последовательности по приговору кодека
				size_t issued = 0;
				// Получаем кодовое значение приговором разбирателя кодека
				const uint32_t code = xml::decode(reinterpret_cast <const char *> (buffer), 2, issued);
				// Если приговоры о годности разошлись
				if((code == xml::INVALID_CODEPOINT) != (length == 0)) blinded++;
			}
		}
		// Выполняем проверку точного числа расхождений ослеплённого судьи
		ASSERT_EQ(blinded, static_cast <size_t> (128));
	}
	/**
	 * Выполняем исчерпывающий перебор всех последовательностей одного и двух байтов
	 */
	for(uint32_t first = 0; first < 256; first++){
		buffer[0] = static_cast <unsigned char> (first);
		compare(buffer, 1);
		for(uint32_t second = 0; second < 256; second++){
			buffer[1] = static_cast <unsigned char> (second);
			compare(buffer, 2);
		}
	}
	/**
	 * Выполняем перебор граничных троек и четвёрок
	 *
	 * @note Ведущие байты подобраны по границам описания: `C0`/`C1` - переудлинённая
	 *       запись, `E0` - её же трёхбайтовый случай, `ED` - суррогаты, `F0` - её же
	 *       четырёхбайтовый случай, `F4` - предел U+10FFFF, `F5` - за пределом
	 */
	for(const unsigned char leading : {0xC0, 0xC1, 0xE0, 0xE1, 0xED, 0xEE, 0xF0, 0xF1, 0xF4, 0xF5}){
		buffer[0] = leading;
		/**
		 * Выполняем исчерпывающий перебор продолжающих байтов у граничных ведущих
		 */
		for(uint32_t second = 0; second < 256; second++){
			buffer[1] = static_cast <unsigned char> (second);
			for(const unsigned char third : {0x00, 0x7F, 0x80, 0x8F, 0x90, 0x9F, 0xA0, 0xBF, 0xC0, 0xFF}){
				buffer[2] = third;
				compare(buffer, 3);
				for(const unsigned char fourth : {0x00, 0x80, 0xBF, 0xC0, 0xFF}){
					buffer[3] = fourth;
					compare(buffer, 4);
				}
			}
		}
	}
	// Выполняем проверку согласия приговоров о годности
	ASSERT_EQ(verdicts, static_cast <size_t> (0));
	// Выполняем проверку согласия кодовых значений
	ASSERT_EQ(codes, static_cast <size_t> (0));
	// Выполняем проверку согласия длин годных последовательностей
	ASSERT_EQ(lengths, static_cast <size_t> (0));
	/**
	 * Выполняем проверку зрячести самого судьи
	 *
	 * @note Судья, всё подряд отвергающий, прошёл бы проверку выше лишь при кодеках,
	 *       делающих то же, - но проверка стоит здесь затем, что молчащий судья
	 *       неотличим от согласия
	 */
	{
		// Длина годной последовательности по приговору судьи
		size_t length = 0;
		// Выполняем проверку отвержения суррогата судьёю
		const unsigned char surrogate[3] = {0xED, 0xA0, 0x80};
		::judge(surrogate, 3, length);
		ASSERT_EQ(length, static_cast <size_t> (0));
		// Выполняем проверку принятия годной последовательности судьёю
		const unsigned char letter[2] = {0xD0, 0x91};
		ASSERT_EQ(::judge(letter, 2, length), static_cast <uint32_t> (0x411));
		ASSERT_EQ(length, static_cast <size_t> (2));
	}
}


/**
 * @brief Проверка согласия открытого посредника годности имени с самой записью
 *
 * @details Судья здесь ОДИН: запись зовёт `nameable` сама, четырьмя местами, и своего
 *          разбора имени у неё нет. Проверка оттого закрепляет НЕ согласие двух судей, а
 *          то, что запись не добавляет к приговору посредника НИЧЕГО сверх него: имя,
 *          посредником принятое, обязано быть принято и записью. Замер 01.09.2026 по
 *          двенадцати именам: совпадают все двенадцать
 *
 * @warning Первая редакция этой записки объявляла судей независимыми, и это было НЕВЕРНО.
 *          Доказывается единственность подменой САМОГО ПОСРЕДНИКА: приговор его изменён -
 *          поведение записи изменилось вместе с ним, и проверка осталась зелёной. Двое
 *          судей дали бы красное, как оно и вышло у `json::numeric`, где разбор посредника
 *          не зовёт вовсе. Розыск зова `grep`-ом годится, чтобы найти кандидата, и НЕ
 *          годится, чтобы объявить независимость: зов бывает слоем ниже
 *
 * @note Расхождение станет возможным в тот день, когда запись заведёт своё сличение имени
 *       ЛИБО добавит сторож помимо посредника. Проверка стережёт именно это: зрячесть
 *       показана подменой ЗАПИСИ - добавлен сторож, о каком посредник не знает, - и она
 *       краснеет, называя виновное имя
 *
 */
TEST(CodecXmlEncoding, NameableAgreesWithTheWriter) {
	// Проверяемые имена узлов разметки
	const char * names[] = {
		"а", "a", "a-b", "a.b", "a:b", "1a", "-a", "a b", "", "xml", "a_b", "a\xC2\xA0" "b"
	};
	// Выполняем перебор всех проверяемых имён
	for(const char * name : names){
		// Приговор открытого посредника годности имени
		const bool verdict = xml::nameable(name);
		// Объект записи текста разметки
		xml::writer_t writer(::logger());
		// Признак принятия имени самой записью
		const bool taken = writer.open(name);
		// Выполняем проверку совпадения приговоров
		EXPECT_EQ(verdict, taken) << "имя «" << name << "»";
	}
}


/**
 * @brief Проверка согласия двух половин кодека о годности имени
 *
 * @details Запись зовёт посредник `nameable` четырьмя местами, а разбор его не зовёт
 *          вовсе. Но независимость половин этим НЕ доказывается: знакового судью
 *          (`isNameStart` и `isName`) обе стороны делят - `nameable` зовёт его сам.
 *          Расходиться половины вправе лишь ОБВОДКОЙ: разбор поверх знаковых правил
 *          толкует построение меткой, атрибутом и префиксом. Расхождение означало бы
 *          разметку, которую кодек прочесть умеет, а записать обратно не может
 *
 * @note Что чем закрепляется, проверено подменами по отдельности. Столбец годности держит
 *       ЗНАКОВЫЕ правила: подмена `isNameStart` двигает обе стороны разом, согласие
 *       остаётся, и краснеет именно столбец. Столбец согласия держит ОБВОДКУ разбора:
 *       подмена внутри `parseName`, посреднику неведомая, краснеет на «a-b». Три подмены
 *       знаковых ветвей, к имени метки не относящихся, оставались зелёными - место
 *       подмены выбиралось замером, а не чтением
 *
 * @note Замер 01.09.2026 по двадцати двум именам, включая тонкие края договора: знак
 *       продолжения в начале, соединительный ноль, незнак `U+FDD0`, знак дополнительной
 *       плоскости. Приговоры совпали все двадцать два
 *
 * @warning Совпадение приговора ещё не есть совпадение ПРИЧИНЫ. У трёх имён разбор
 *          отвергает не имя: «a b» отвергается как ошибочный атрибут (0x11), «a:b» - как
 *          несвязанный префикс (0x23), «a\\uFDD0» - как ошибочная метка (0x09). Имя как имя
 *          разбор там не судил вовсе, и сличать эти три с приговором посредника значило бы
 *          мерить согласие там, где второй судья молчит. Оттого причина сличается
 *          отдельно, а три исключения названы поимённо
 *
 * @note Механизм третьего исключения снят замером, а не выведен: перебор знаков имени на
 *       незнаке `U+FDD0` не отказывает, а ОСТАНАВЛИВАЕТСЯ, и виновный знак достаётся уже
 *       толкованию метки - оттого и код метки. Щуп 01.09.2026: обрыв перебора случается на
 *       двадцати двух именах ровно однажды и ровно на этом знаке
 *
 */
TEST(CodecXmlEncoding, BothHalvesAgreeOnNameValidity) {
	// Проверяемые имена узлов разметки и ожидаемые коды отказа разбора
	const struct {
		// Проверяемое имя узла разметки
		const char * name;
		// Признак годности имени по договору
		bool valid;
		// Ожидаемый код отказа разбора, ноль - имя годно
		uint32_t code;
	} probes[] = {
		{"a", true, 0}, {"а", true, 0}, {"a-b", true, 0}, {"a.b", true, 0},
		{"a_b", true, 0}, {"a1", true, 0}, {"xml", true, 0}, {"XmL", true, 0},
		{"a\xC2\xB7" "b", true, 0}, {"a\xCC\x80", true, 0}, {"\xE2\x80\x8C" "a", true, 0},
		{"_a", true, 0}, {"\xF0\x90\x80\x80" "a", true, 0},
		{"1a", false, 0x07}, {"-a", false, 0x07}, {".a", false, 0x07},
		{"\xC2\xB7" "a", false, 0x07}, {"\xCC\x80" "a", false, 0x07}, {":a", false, 0x07},
		/**
		 * Три имени, отвергаемые разбором по ИНОЙ причине, нежели годность имени
		 *
		 * @note Приговор совпадает, а причина иная, и потому коды здесь свои: разбор
		 *       толкует эти построения атрибутом, префиксом и меткой, до сличения имени
		 *       не доходя
		 */
		{"a b", false, 0x11}, {"a:b", false, 0x23}, {"a\xEF\xB7\x90", false, 0x09}
	};
	// Выполняем перебор всех проверяемых имён
	for(const auto & probe : probes){
		// Приговор открытого посредника годности имени
		const bool judge = xml::nameable(probe.name);
		// Чтение текста разметки
		xml::reader_t reader(::logger());
		// Текст разметки с проверяемым именем узла
		const string text = string("<") + probe.name + "/>";
		// Выполняем подачу текста разметки целиком
		reader.feed(text.data(), text.size(), true);
		// Выполняем перебор всех событий разбора
		while(reader.next()) ;
		// Признак принятия имени разбором
		const bool read = (reader.error() == xml::error_t::NONE);
		// Выполняем проверку приговора посредника
		EXPECT_EQ(judge, probe.valid) << "имя «" << probe.name << "»";
		// Выполняем проверку совпадения приговоров обеих половин
		EXPECT_EQ(judge, read) << "имя «" << probe.name << "»";
		// Выполняем проверку совпадения ПРИЧИНЫ отказа разбора
		EXPECT_EQ(static_cast <uint32_t> (reader.error()), probe.code) << "имя «" << probe.name << "»";
	}
}



/**
 * @brief Проверка согласия быстрой таблицы разбора имени с медленным судьёй
 *
 * @details Разбор имени идёт ДВУМЯ путями: знаки US-ASCII судятся таблицей разрядов, а
 *          прочие - посредниками `isNameStart` и `isName`. Судьи, стало быть, два, и об
 *          одном правиле: описка в единственном разряде таблицы развела бы их молча -
 *          имя прошло бы там, где договор его отвергает, и наоборот
 *
 * @note Перебор идёт по ВСЕМ ста двадцати семи знакам US-ASCII, а не по образцам:
 *       описка в разряде поражает один знак, и выбранная горстка её не поймает.
 *       Замер 01.09.2026: расхождение ровно одно, и оно намеренно
 *
 * @warning Единственное расхождение - двоеточие. Медленный судья зовёт его годным по
 *          правилу имени, а быстрый путь на нём делит имя на префикс и местное, и в
 *          местное имя оно не входит. Расхождение это есть само устройство пространств
 *          имён, а не описка: третий разряд таблицы затем и заведён
 *
 * @note Место подмены для этой проверки выбрано замером: подмена медленного пути знаком
 *       US-ASCII не задевает НИЧЕГО - такие знаки до него не доходят вовсе. Свалить её
 *       можно лишь знаком вне US-ASCII либо правкой самой таблицы
 *
 * @warning Проверка обрамляет испытуемый знак буквами «a» и «b», а годность их МЕРЯЕТ ЖЕ
 *          САМА. Порча разряда у «a» валит потому не один случай, а сто восемнадцать: она
 *          отравляет обрамление всех прочих. Зрячесть покознаковая доказывается порчею
 *          знака, обрамлением НЕ занятого: погашенный разряд у «-» валит ровно одно
 *          утверждение - «0x2d внутри имени», - и предсказание это сошлось до знака
 *
 */
TEST(CodecXmlEncoding, FastNameTableAgreesWithTheSlowJudge) {
	/**
	 * @brief Метод разбора имени узла с выдачей признака вхождения знака в имя
	 *
	 * @param text     разбираемый текст разметки
	 * @param expected ожидаемое местное имя узла
	 * @return         признак того, что имя разобрано полностью и ожидаемо
	 *
	 */
	const auto taken = [](const string & text, const string & expected) noexcept -> bool {
		// Чтение текста разметки
		xml::reader_t reader(::logger());
		// Выполняем подачу текста разметки целиком
		reader.feed(text.data(), text.size(), true);
		// Местное имя разобранного узла разметки
		string name;
		// Выполняем перебор всех событий разбора
		while(reader.next()){
			// Если получено событие начала узла разметки
			if(reader.event() == xml::event_t::ELEMENT_OPEN)
				// Запоминаем местное имя разобранного узла
				name = string(reader.name().local);
		}
		// Выводим признак полного и ожидаемого разбора имени
		return ((reader.error() == xml::error_t::NONE) && (name == expected));
	};
	// Количество знаков, на которых судьи разошлись
	size_t diverged = 0;
	/**
	 * Выполняем перебор всех знаков US-ASCII, кроме нулевого
	 */
	for(uint32_t code = 1; code < 128; code++){
		// Знак, годность которого проверяется
		const char letter = static_cast <char> (code);
		// Приговор медленного судьи о годности знака внутри имени
		const bool inside = xml::isName(code);
		// Приговор медленного судьи о годности знака в начале имени
		const bool starts = xml::isNameStart(code);
		// Признак вхождения знака в имя по быстрому пути разбора
		const bool fastInside = taken(string("<a") + letter + "b/>", string("a") + letter + "b");
		// Признак вхождения знака в начало имени по быстрому пути разбора
		const bool fastStarts = taken(string("<") + letter + "ab/>", string(1, letter) + "ab");
		/**
		 * Если знак является разделителем префикса и местного имени
		 *
		 * @note Двоеточие есть единственное намеренное расхождение судей, и оно
		 *       сличается своим утверждением: медленный судья зовёт его годным, а
		 *       быстрый путь делит им имя, и в местное имя оно не входит
		 */
		if(code == 0x3A){
			// Выполняем проверку годности двоеточия у медленного судьи
			EXPECT_TRUE(inside && starts);
			// Выполняем проверку невхождения двоеточия в местное имя
			EXPECT_FALSE(fastInside || fastStarts);
			// Выполняем подсчёт намеренного расхождения судей
			diverged++;
			// Выполняем переход к следующему знаку
			continue;
		}
		// Выполняем проверку согласия судей о знаке внутри имени
		EXPECT_EQ(inside, fastInside) << "знак 0x" << ::std::hex << code << " внутри имени";
		// Выполняем проверку согласия судей о знаке в начале имени
		EXPECT_EQ(starts, fastStarts) << "знак 0x" << ::std::hex << code << " в начале имени";
	}
	// Выполняем проверку того, что намеренное расхождение ровно одно
	ASSERT_EQ(diverged, 1u);
}
