/**
 * @file common.cpp
 * @date 2026-08-17
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Проверки общих объявлений контейнера YAML — описания кодов отказов, названия
 *        видов и событий, разрешение вида скалярного значения действующей схемой и
 *        согласие выбора ограды с этим разрешением
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cmath>
#include <limits>
#include <string>
#include <vector>
#include <cstring>
#include <unordered_set>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <gtest/gtest.h>
#include <codec/yaml/yaml.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;
using namespace awh::codec;

/**
 * @brief Проверка описаний кодов отказов разбора
 *
 * @details Всякий объявленный код обязан нести своё описание: описание неизвестного
 * кода у объявленного означало бы, что отказ выдаётся, а объяснить его нечем
 *
 */
TEST(CodecYamlCommon, Messages) {
	/**
	 * Коды отказов разбора текста настроек
	 */
	const vector <yaml::error_t> errors = {
		yaml::error_t::NONE, yaml::error_t::INTERNAL, yaml::error_t::UNEXPECTED_EOF,
		yaml::error_t::INVALID_CHARACTER, yaml::error_t::INVALID_ENCODING,
		yaml::error_t::UNSUPPORTED_ENCODING, yaml::error_t::INVALID_INDENTATION,
		yaml::error_t::TAB_IN_INDENTATION, yaml::error_t::UNTERMINATED_SCALAR,
		yaml::error_t::INVALID_ESCAPE, yaml::error_t::INVALID_UNICODE,
		yaml::error_t::UNPAIRED_SURROGATE, yaml::error_t::INVALID_BLOCK_HEADER,
		yaml::error_t::INVALID_NUMBER, yaml::error_t::NUMBER_OUT_OF_RANGE,
		yaml::error_t::INVALID_BINARY, yaml::error_t::INVALID_STAMP,
		yaml::error_t::EXPECTED_VALUE, yaml::error_t::EXPECTED_KEY,
		yaml::error_t::EXPECTED_COLON, yaml::error_t::EXPECTED_COMMA,
		yaml::error_t::UNCLOSED_FLOW, yaml::error_t::MIXED_COLLECTION,
		yaml::error_t::DUPLICATE_KEY, yaml::error_t::COMPLEX_KEY,
		yaml::error_t::UNKNOWN_ALIAS, yaml::error_t::DUPLICATE_ANCHOR,
		yaml::error_t::RECURSIVE_ALIAS, yaml::error_t::EXPANSION_EXCEEDED,
		yaml::error_t::INVALID_TAG, yaml::error_t::UNKNOWN_TAG_HANDLE,
		yaml::error_t::TAG_MISMATCH, yaml::error_t::INVALID_DIRECTIVE,
		yaml::error_t::UNSUPPORTED_VERSION, yaml::error_t::UNEXPECTED_DOCUMENT,
		yaml::error_t::TRAILING_CHARACTERS, yaml::error_t::DEPTH_EXCEEDED,
		yaml::error_t::SCALAR_TOO_LONG, yaml::error_t::NUMBER_TOO_LONG,
		yaml::error_t::ANCHOR_TOO_LONG, yaml::error_t::TOO_MANY_NODES,
		yaml::error_t::EMPTY_TEXT, yaml::error_t::OVERFLOW_LIMIT,
		yaml::error_t::CONFLICTING_SETTINGS
	};
	// Собрание уже встреченных описаний
	unordered_set <string> descriptions;
	/**
	 * Выполняем перебор всех кодов отказов разбора
	 */
	for(const yaml::error_t error : errors){
		// Получаем описание очередного кода отказа
		const char * message = yaml::message(error);
		// Выполняем проверку того, что описание выдано
		ASSERT_NE(message, nullptr);
		// Выполняем проверку того, что описание не пусто
		ASSERT_GT(::strlen(message), 0u);
		// Выполняем проверку того, что описание не является описанием неизвестного кода
		ASSERT_STRNE(message, "неизвестный код отказа");
		/**
		 * Выполняем проверку того, что описание не повторяет уже встреченного
		 *
		 * @note Два кода с одним описанием неразличимы для того, кто читает журнал, и
		 *       отказ, объяснённый чужим описанием, уводит поиск причины в сторону
		 */
		ASSERT_TRUE(descriptions.emplace(message).second) << "описание повторяется: " << message;
	}
}
/**
 * @brief Проверка названий видов узлов, видов значений и событий чтения
 *
 */
TEST(CodecYamlCommon, Names) {
	/**
	 * Виды узлов документа
	 */
	const vector <yaml::kind_t> kinds = {
		yaml::kind_t::NONE, yaml::kind_t::NUL, yaml::kind_t::BOOL, yaml::kind_t::NUMBER,
		yaml::kind_t::STRING, yaml::kind_t::BINARY, yaml::kind_t::STAMP,
		yaml::kind_t::SEQUENCE, yaml::kind_t::MAPPING
	};
	/**
	 * Выполняем перебор всех видов узлов документа
	 */
	for(const yaml::kind_t kind : kinds)
		// Выполняем проверку того, что вид узла назван
		ASSERT_STRNE(yaml::name(kind), "unknown");
	/**
	 * Точные виды значений документа
	 */
	const vector <yaml::type_t> types = {
		yaml::type_t::UNDEFINED, yaml::type_t::NUL, yaml::type_t::BOOL, yaml::type_t::STRING,
		yaml::type_t::SEQUENCE, yaml::type_t::MAPPING, yaml::type_t::INT8, yaml::type_t::INT16,
		yaml::type_t::INT32, yaml::type_t::INT64, yaml::type_t::UINT8, yaml::type_t::UINT16,
		yaml::type_t::UINT32, yaml::type_t::UINT64, yaml::type_t::FLOAT, yaml::type_t::DOUBLE,
		yaml::type_t::EXTENDED, yaml::type_t::BINARY, yaml::type_t::STAMP
	};
	/**
	 * Выполняем перебор всех точных видов значений
	 */
	for(const yaml::type_t type : types)
		// Выполняем проверку того, что вид значения назван
		ASSERT_STRNE(yaml::name(type), "unknown");
	/**
	 * Виды событий чтения текста
	 */
	const vector <yaml::event_t> events = {
		yaml::event_t::NONE, yaml::event_t::STREAM_START, yaml::event_t::STREAM_END,
		yaml::event_t::DOCUMENT_START, yaml::event_t::DOCUMENT_END,
		yaml::event_t::MAPPING_START, yaml::event_t::MAPPING_END,
		yaml::event_t::SEQUENCE_START, yaml::event_t::SEQUENCE_END,
		yaml::event_t::SCALAR, yaml::event_t::ALIAS, yaml::event_t::COMMENT,
		yaml::event_t::BLANK, yaml::event_t::FINISH
	};
	/**
	 * Выполняем перебор всех видов событий чтения
	 */
	for(const yaml::event_t event : events)
		// Выполняем проверку того, что событие названо
		ASSERT_STRNE(yaml::name(event), "UNKNOWN");
	/**
	 * Выполняем проверку названий, сличаемых с эталонным набором
	 *
	 * @note Названия эти совпадают с названиями `yaml-test-suite` дословно: сличение с
	 *       ним ведётся строка в строку, и переименование здесь сломало бы его молча
	 */
	ASSERT_STREQ(yaml::name(yaml::event_t::MAPPING_START), "MAPPING_START");
	ASSERT_STREQ(yaml::name(yaml::event_t::SEQUENCE_END), "SEQUENCE_END");
	ASSERT_STREQ(yaml::name(yaml::event_t::SCALAR), "SCALAR");
	ASSERT_STREQ(yaml::name(yaml::event_t::ALIAS), "ALIAS");
}
/**
 * @brief Проверка огрубления вида значения до вида узла
 *
 */
TEST(CodecYamlCommon, Kinds) {
	// Выполняем проверку того, что всякое число огрубляется до узла числа
	ASSERT_EQ(yaml::kind(yaml::type_t::INT8), yaml::kind_t::NUMBER);
	// Выполняем проверку огрубления целого без знака
	ASSERT_EQ(yaml::kind(yaml::type_t::UINT64), yaml::kind_t::NUMBER);
	// Выполняем проверку огрубления дробного
	ASSERT_EQ(yaml::kind(yaml::type_t::DOUBLE), yaml::kind_t::NUMBER);
	// Выполняем проверку огрубления числа, в родной вид не вместившегося
	ASSERT_EQ(yaml::kind(yaml::type_t::EXTENDED), yaml::kind_t::NUMBER);
	// Выполняем проверку огрубления строки
	ASSERT_EQ(yaml::kind(yaml::type_t::STRING), yaml::kind_t::STRING);
	// Выполняем проверку огрубления пустого значения
	ASSERT_EQ(yaml::kind(yaml::type_t::NUL), yaml::kind_t::NUL);
	// Выполняем проверку огрубления отметки времени
	ASSERT_EQ(yaml::kind(yaml::type_t::STAMP), yaml::kind_t::STAMP);
	// Выполняем проверку огрубления двоичного содержимого
	ASSERT_EQ(yaml::kind(yaml::type_t::BINARY), yaml::kind_t::BINARY);
	// Выполняем проверку огрубления неопределённого значения
	ASSERT_EQ(yaml::kind(yaml::type_t::UNDEFINED), yaml::kind_t::NONE);
}
/**
 * @brief Проверка разрешения видов ядровой схемой наречия 1.2
 *
 */
TEST(CodecYamlCommon, ResolveCore) {
	// Действующая схема разрешения
	const yaml::schema_t schema = yaml::schema_t::CORE;
	// Выполняем проверку написаний пустого значения
	ASSERT_EQ(yaml::resolve("~", schema), yaml::type_t::NUL);
	// Выполняем проверку написания пустого значения строчными буквами
	ASSERT_EQ(yaml::resolve("null", schema), yaml::type_t::NUL);
	// Выполняем проверку написания пустого значения прописными буквами
	ASSERT_EQ(yaml::resolve("NULL", schema), yaml::type_t::NUL);
	// Выполняем проверку пустой записи
	ASSERT_EQ(yaml::resolve("", schema), yaml::type_t::NUL);
	/**
	 * Выполняем проверку написания, описанием не предусмотренного
	 *
	 * @note Описание дозволяет ровно три написания, а не всякое: `nULL` есть строка
	 */
	ASSERT_EQ(yaml::resolve("nULL", schema), yaml::type_t::STRING);
	// Выполняем проверку логического значения
	ASSERT_EQ(yaml::resolve("true", schema), yaml::type_t::BOOL);
	// Выполняем проверку логического значения прописными буквами
	ASSERT_EQ(yaml::resolve("FALSE", schema), yaml::type_t::BOOL);
	/**
	 * Выполняем проверку написаний наречия 1.1, ядровой схемою не признаваемых
	 *
	 * @note Здесь и лежит беда, известная под именем норвежской: страна NO обязана
	 *       остаться строкой
	 */
	ASSERT_EQ(yaml::resolve("no", schema), yaml::type_t::STRING);
	// Выполняем проверку написания NO прописными буквами
	ASSERT_EQ(yaml::resolve("NO", schema), yaml::type_t::STRING);
	// Выполняем проверку написания on
	ASSERT_EQ(yaml::resolve("on", schema), yaml::type_t::STRING);
	// Выполняем проверку целого десятичного числа
	ASSERT_EQ(yaml::resolve("42", schema), yaml::type_t::NUMBER);
	// Выполняем проверку целого числа со знаком
	ASSERT_EQ(yaml::resolve("-42", schema), yaml::type_t::NUMBER);
	// Выполняем проверку целого числа с ведущим плюсом
	ASSERT_EQ(yaml::resolve("+42", schema), yaml::type_t::NUMBER);
	// Выполняем проверку восьмеричной записи наречия 1.2
	ASSERT_EQ(yaml::resolve("0o17", schema), yaml::type_t::NUMBER);
	// Выполняем проверку шестнадцатеричной записи
	ASSERT_EQ(yaml::resolve("0x1F", schema), yaml::type_t::NUMBER);
	/**
	 * Выполняем проверку записи с ведущим нулём
	 *
	 * @note Ядровая схема ведущего нуля восьмеричной записью не считает: `017` есть
	 *       обыкновенное десятичное семнадцать
	 */
	ASSERT_EQ(yaml::resolve("017", schema), yaml::type_t::NUMBER);
	// Выполняем проверку двоичной записи, ядровой схемою не признаваемой
	ASSERT_EQ(yaml::resolve("0b1010", schema), yaml::type_t::STRING);
	// Выполняем проверку разделения разрядов, ядровой схемою не признаваемого
	ASSERT_EQ(yaml::resolve("1_000", schema), yaml::type_t::STRING);
	// Выполняем проверку шестидесятиричной записи, ядровой схемою не признаваемой
	ASSERT_EQ(yaml::resolve("12:30", schema), yaml::type_t::STRING);
	// Выполняем проверку дробного числа
	ASSERT_EQ(yaml::resolve("3.14", schema), yaml::type_t::NUMBER);
	// Выполняем проверку записи числа порядком
	ASSERT_EQ(yaml::resolve("1e3", schema), yaml::type_t::NUMBER);
	// Выполняем проверку записи числа без целой части
	ASSERT_EQ(yaml::resolve(".5", schema), yaml::type_t::NUMBER);
	// Выполняем проверку записи порядка без цифр
	ASSERT_EQ(yaml::resolve("1e", schema), yaml::type_t::STRING);
	// Выполняем проверку записи бесконечности
	ASSERT_EQ(yaml::resolve(".inf", schema), yaml::type_t::DOUBLE);
	// Выполняем проверку записи бесконечности со знаком
	ASSERT_EQ(yaml::resolve("-.inf", schema), yaml::type_t::DOUBLE);
	// Выполняем проверку записи нечисловой величины
	ASSERT_EQ(yaml::resolve(".nan", schema), yaml::type_t::DOUBLE);
	/**
	 * Выполняем проверку записи даты
	 *
	 * @note Отметок времени наречие 1.2 не знает вовсе, и дата есть там строка
	 */
	ASSERT_EQ(yaml::resolve("2001-12-14", schema), yaml::type_t::STRING);
}
/**
 * @brief Проверка разрешения видов схемою наречия 1.1
 *
 */
TEST(CodecYamlCommon, ResolveLegacy) {
	// Действующая схема разрешения
	const yaml::schema_t schema = yaml::schema_t::LEGACY;
	// Выполняем проверку логического значения наречия 1.1
	ASSERT_EQ(yaml::resolve("no", schema), yaml::type_t::BOOL);
	// Выполняем проверку логического значения одною буквой
	ASSERT_EQ(yaml::resolve("y", schema), yaml::type_t::BOOL);
	// Выполняем проверку логического значения включением
	ASSERT_EQ(yaml::resolve("ON", schema), yaml::type_t::BOOL);
	// Выполняем проверку восьмеричной записи с ведущим нулём
	ASSERT_EQ(yaml::resolve("0777", schema), yaml::type_t::NUMBER);
	// Выполняем проверку двоичной записи
	ASSERT_EQ(yaml::resolve("0b1010", schema), yaml::type_t::NUMBER);
	// Выполняем проверку разделения разрядов знаком подчёркивания
	ASSERT_EQ(yaml::resolve("1_000", schema), yaml::type_t::NUMBER);
	// Выполняем проверку шестидесятиричной записи
	ASSERT_EQ(yaml::resolve("12:30", schema), yaml::type_t::NUMBER);
	// Выполняем проверку шестидесятиричной записи о трёх частях
	ASSERT_EQ(yaml::resolve("190:20:30", schema), yaml::type_t::NUMBER);
	// Выполняем проверку шестидесятиричной записи с дробной частью
	ASSERT_EQ(yaml::resolve("12:30.5", schema), yaml::type_t::NUMBER);
	// Выполняем проверку записи, шестидесятиричной не являющейся
	ASSERT_EQ(yaml::resolve("12:ab", schema), yaml::type_t::STRING);
}
/**
 * @brief Проверка разрешения видов схемою строгого соответствия правилам JSON
 *
 */
TEST(CodecYamlCommon, ResolveJson) {
	// Действующая схема разрешения
	const yaml::schema_t schema = yaml::schema_t::JSON;
	// Выполняем проверку пустого значения
	ASSERT_EQ(yaml::resolve("null", schema), yaml::type_t::NUL);
	// Выполняем проверку написания пустого значения, правилам JSON не отвечающего
	ASSERT_EQ(yaml::resolve("~", schema), yaml::type_t::STRING);
	// Выполняем проверку написания логического значения с прописной буквы
	ASSERT_EQ(yaml::resolve("True", schema), yaml::type_t::STRING);
	// Выполняем проверку целого числа
	ASSERT_EQ(yaml::resolve("42", schema), yaml::type_t::NUMBER);
	// Выполняем проверку записи с ведущим нулём, правилами JSON запрещённой
	ASSERT_EQ(yaml::resolve("017", schema), yaml::type_t::STRING);
	// Выполняем проверку записи с ведущим плюсом, правилами JSON запрещённой
	ASSERT_EQ(yaml::resolve("+42", schema), yaml::type_t::STRING);
	// Выполняем проверку шестнадцатеричной записи, правилами JSON запрещённой
	ASSERT_EQ(yaml::resolve("0x1F", schema), yaml::type_t::STRING);
	// Выполняем проверку записи бесконечности, правилами JSON запрещённой
	ASSERT_EQ(yaml::resolve(".inf", schema), yaml::type_t::STRING);
}
/**
 * @brief Проверка разрешения видов схемою, признающей одни лишь строки
 *
 */
TEST(CodecYamlCommon, ResolveFailsafe) {
	// Действующая схема разрешения
	const yaml::schema_t schema = yaml::schema_t::FAILSAFE;
	// Выполняем проверку того, что пустое значение осталось строкой
	ASSERT_EQ(yaml::resolve("~", schema), yaml::type_t::STRING);
	// Выполняем проверку того, что логическое значение осталось строкой
	ASSERT_EQ(yaml::resolve("true", schema), yaml::type_t::STRING);
	// Выполняем проверку того, что число осталось строкой
	ASSERT_EQ(yaml::resolve("42", schema), yaml::type_t::STRING);
}
/**
 * @brief Проверка допустимости имён меток и ссылок
 *
 */
TEST(CodecYamlCommon, Anchors) {
	// Выполняем проверку обыкновенного имени метки
	ASSERT_TRUE(yaml::anchored("base"));
	// Выполняем проверку имени метки со знаком подчёркивания
	ASSERT_TRUE(yaml::anchored("base_1"));
	// Выполняем проверку пустого имени метки
	ASSERT_FALSE(yaml::anchored(""));
	// Выполняем проверку имени метки с пробелом
	ASSERT_FALSE(yaml::anchored("base one"));
	// Выполняем проверку имени метки со скобкой поточного построения
	ASSERT_FALSE(yaml::anchored("base[1]"));
	// Выполняем проверку имени метки с запятой
	ASSERT_FALSE(yaml::anchored("base,one"));
	// Выполняем проверку имени метки, предел длины превышающего
	/**
	 * Выполняем проверку имени метки длиною ровно в предел
	 *
	 * @note Проверялась прежде одна сторона границы - имя длиннее предела. Сторона
	 *       принимающая держится теперь тоже: без неё сдвиг предела на единицу проверкою
	 *       не ловится, что порчею и доказано
	 */
	ASSERT_TRUE(yaml::anchored(string(yaml::MAX_ANCHOR, 'a')));
	ASSERT_FALSE(yaml::anchored(string((yaml::MAX_ANCHOR + 1), 'a')));
}
/**
 * @brief Проверка выбора вида записи скалярного значения
 *
 */
TEST(CodecYamlCommon, Quoting) {
	// Действующая схема разрешения
	const yaml::schema_t schema = yaml::schema_t::CORE;
	// Выполняем проверку обыкновенного значения
	ASSERT_EQ(yaml::quoting("hello", schema, false), yaml::style_t::PLAIN);
	// Выполняем проверку значения с пробелом
	ASSERT_EQ(yaml::quoting("hello world", schema, false), yaml::style_t::PLAIN);
	/**
	 * Выполняем проверку значения с двоеточием без пробела за ним
	 *
	 * @note Двоеточие отделяет имя пары лишь тогда, когда за ним стоит пробельный знак:
	 *       `a:b` есть значение целиком
	 */
	ASSERT_EQ(yaml::quoting("a:b", schema, false), yaml::style_t::PLAIN);
	// Выполняем проверку значения с двоеточием и пробелом за ним
	ASSERT_EQ(yaml::quoting("a: b", schema, false), yaml::style_t::SINGLE);
	/**
	 * Выполняем проверку двоеточия внутри имени пары
	 *
	 * @note Имя пары строже значения: там двоеточие оборвало бы имя на себе
	 */
	ASSERT_EQ(yaml::quoting("a:b", schema, true), yaml::style_t::SINGLE);
	/**
	 * Выполняем проверку знака примечания без пробела перед ним
	 *
	 * @note Примечание открывается решёткой лишь за пробельным знаком либо в начале
	 *       записи: `b#c` есть значение целиком
	 */
	ASSERT_EQ(yaml::quoting("b#c", schema, false), yaml::style_t::PLAIN);
	// Выполняем проверку знака примечания с пробелом перед ним
	ASSERT_EQ(yaml::quoting("b #c", schema, false), yaml::style_t::SINGLE);
	// Выполняем проверку знака примечания в начале записи
	ASSERT_EQ(yaml::quoting("#c", schema, false), yaml::style_t::SINGLE);
	// Выполняем проверку значения с обвязкой пробелами
	ASSERT_EQ(yaml::quoting(" x", schema, false), yaml::style_t::SINGLE);
	// Выполняем проверку пустого значения
	ASSERT_EQ(yaml::quoting("", schema, false), yaml::style_t::SINGLE);
	// Выполняем проверку значения с переводом строки
	ASSERT_EQ(yaml::quoting("одна\nдве", schema, false), yaml::style_t::DOUBLE);
	// Выполняем проверку значения со скобкой поточного построения
	ASSERT_EQ(yaml::quoting("[перечень]", schema, false), yaml::style_t::SINGLE);
	// Выполняем проверку значения со знаком доли, значащим лишь первым знаком
	ASSERT_EQ(yaml::quoting("100%", schema, false), yaml::style_t::PLAIN);
	// Выполняем проверку значения, начинающегося со знака объявления перечня
	ASSERT_EQ(yaml::quoting("- x", schema, false), yaml::style_t::SINGLE);
	// Выполняем проверку значения со знаком объявления перечня без пробела за ним
	ASSERT_EQ(yaml::quoting("-x", schema, false), yaml::style_t::PLAIN);
	/**
	 * Выполняем проверку значений, схемою разрешаемых не строкой
	 *
	 * @note Ограда здесь необходима: без неё строка `12` вернулась бы числом
	 */
	ASSERT_EQ(yaml::quoting("12", schema, false), yaml::style_t::SINGLE);
	// Выполняем проверку значения, разрешаемого логическим
	ASSERT_EQ(yaml::quoting("true", schema, false), yaml::style_t::SINGLE);
	// Выполняем проверку значения, ядровой схемою разрешаемого строкой
	ASSERT_EQ(yaml::quoting("yes", schema, false), yaml::style_t::PLAIN);
	// Выполняем проверку того же значения схемою наречия 1.1
	ASSERT_EQ(yaml::quoting("yes", yaml::schema_t::LEGACY, false), yaml::style_t::SINGLE);
}
/**
 * @brief Проверка согласия выбора ограды с разрешением видов
 *
 * @details Проверка эта стережёт главную опасность записи: значение, записанное без
 * ограды, обязано читаться обратно строкой. Расхождение здесь означало бы, что запись
 * выдаёт текст, который собственное чтение прочтёт иначе
 *
 * @note Два свода правил - разрешение и выбор ограды - разойдутся при первой же правке
 *       одного из них, и проверка эта сличает их сплошным перебором по всем схемам
 *
 */
TEST(CodecYamlCommon, QuotingAgreement) {
	/**
	 * Образцы значений, охватывающие оба наречия и оба положения записи
	 */
	const vector <string> samples = {
		"hello", "hello world", "a:b", "b#c", "-x", "12", "0x1F", "yes", "no", "on", "~",
		"null", ".inf", ".nan", "017", "0777", "1_000", "12:30", "3.14", "1e3", "", " x",
		"true", "False", "2001-12-14", "текст", "a: b", "b #c", "?", "-", ":", "@ссылка",
		"100%", "x*y", "a|b", "0b1010", "y", "N", "+42", ".5", "1.", "12:30.5"
	};
	/**
	 * Схемы разрешения видов скалярных значений
	 */
	const vector <yaml::schema_t> schemas = {
		yaml::schema_t::FAILSAFE, yaml::schema_t::JSON,
		yaml::schema_t::CORE, yaml::schema_t::LEGACY
	};
	/**
	 * Выполняем перебор всех схем разрешения
	 */
	for(const yaml::schema_t schema : schemas){
		/**
		 * Выполняем перебор всех образцов значений
		 */
		for(const string & sample : samples){
			/**
			 * Выполняем перебор записи значением и именем пары
			 */
			for(const bool key : {false, true}){
				/**
				 * Если образец записывается без ограды
				 */
				if(yaml::quoting(sample, schema, key) == yaml::style_t::PLAIN){
					// Выполняем проверку того, что образец читается обратно строкой
					ASSERT_EQ(yaml::resolve(sample, schema), yaml::type_t::STRING)
						<< "значение «" << sample << "» пишется без ограды, а читается видом "
						<< yaml::name(yaml::resolve(sample, schema));
				}
			}
		}
	}
}
/**
 * @brief Проверка разбора записи числа к самому узкому вмещающему виду
 *
 * @details Разбор ведётся действующей схемой: `0777` есть 511 по наречию 1.1 и строка
 * по наречию 1.2, а `12:30` есть 750 там же
 *
 * @note Потоковое чтение число не разбирает вовсе и выдаёт сборный вид `NUMBER`:
 *       разбирать всякое число, никем не затребованное, значило бы платить за то, чего
 *       потребитель не просил. Разбирает его дерево при постройке своей
 *
 */
TEST(CodecYamlCommon, Narrow) {
	/**
	 * Выполняем проверку выбора самого узкого вида, число вмещающего
	 */
	{
		// Разобранное число всеми видами его
		yaml::numeric_t number;
		// Выполняем проверку вида числа, в один байт без знака вмещающегося
		ASSERT_EQ(yaml::narrow("12", yaml::schema_t::CORE, number), yaml::type_t::UINT8);
		// Выполняем проверку разобранного числа
		ASSERT_EQ(number.natural, 12u);
		// Выполняем проверку вида числа, в один байт без знака не вмещающегося
		ASSERT_EQ(yaml::narrow("256", yaml::schema_t::CORE, number), yaml::type_t::UINT16);
		// Выполняем проверку вида числа со знаком
		ASSERT_EQ(yaml::narrow("-1250", yaml::schema_t::CORE, number), yaml::type_t::INT16);
		// Выполняем проверку разобранного числа со знаком
		ASSERT_EQ(number.integer, -1250);
	}
	/**
	 * Выполняем проверку разбора чисел иных систем счисления
	 */
	{
		// Разобранное число всеми видами его
		yaml::numeric_t number;
		// Выполняем проверку разбора шестнадцатеричной записи
		ASSERT_EQ(yaml::narrow("0x1F", yaml::schema_t::CORE, number), yaml::type_t::UINT8);
		// Выполняем проверку разобранного шестнадцатеричного числа
		ASSERT_EQ(number.natural, 31u);
		// Выполняем проверку разбора восьмеричной записи наречия 1.2
		ASSERT_EQ(yaml::narrow("0o755", yaml::schema_t::CORE, number), yaml::type_t::UINT16);
		// Выполняем проверку разобранного восьмеричного числа
		ASSERT_EQ(number.natural, 493u);
	}
	/**
	 * Выполняем проверку разбора дробных чисел
	 */
	{
		// Разобранное число всеми видами его
		yaml::numeric_t number;
		// Выполняем проверку разбора дробного числа
		ASSERT_EQ(yaml::narrow("3.14159", yaml::schema_t::CORE, number), yaml::type_t::DOUBLE);
		// Выполняем проверку разобранного дробного числа
		ASSERT_DOUBLE_EQ(number.real, 3.14159);
		// Выполняем проверку разбора числа показательного вида
		ASSERT_EQ(yaml::narrow("-2.5e-13", yaml::schema_t::CORE, number), yaml::type_t::DOUBLE);
		// Выполняем проверку разобранного числа показательного вида
		ASSERT_DOUBLE_EQ(number.real, -2.5e-13);
		// Выполняем проверку разбора бесконечности
		ASSERT_EQ(yaml::narrow("-.inf", yaml::schema_t::CORE, number), yaml::type_t::DOUBLE);
		// Выполняем проверку разобранной бесконечности
		ASSERT_TRUE(::isinf(number.real) && (number.real < 0));
		// Выполняем проверку разбора нечисловой величины
		ASSERT_EQ(yaml::narrow(".nan", yaml::schema_t::CORE, number), yaml::type_t::DOUBLE);
		// Выполняем проверку разобранной нечисловой величины
		ASSERT_TRUE(::isnan(number.real));
	}
	/**
	 * Выполняем проверку пределов целых чисел
	 */
	{
		// Разобранное число всеми видами его
		yaml::numeric_t number;
		// Выполняем проверку разбора наибольшего целого без знака
		ASSERT_EQ(yaml::narrow("18446744073709551615", yaml::schema_t::CORE, number), yaml::type_t::UINT64);
		// Выполняем проверку разобранного наибольшего целого без знака
		ASSERT_EQ(number.natural, 18446744073709551615ull);
		// Выполняем проверку разбора наименьшего целого со знаком
		ASSERT_EQ(yaml::narrow("-9223372036854775808", yaml::schema_t::CORE, number), yaml::type_t::INT64);
		// Выполняем проверку разобранного наименьшего целого со знаком
		ASSERT_EQ(number.integer, numeric_limits <int64_t>::min());
		/**
		 * Выполняем проверку разбора числа, ни в один родной вид не вместимого
		 *
		 * @note Число такое хранится записью своей и выдаётся дробным приближением:
		 *       отбросить его значило бы потерять содержимое, записанное текстом верно
		 */
		ASSERT_EQ(yaml::narrow("99999999999999999999999999", yaml::schema_t::CORE, number), yaml::type_t::EXTENDED);
		// Выполняем проверку дробного приближения числа
		ASSERT_DOUBLE_EQ(number.real, 1e26);
	}
	/**
	 * Выполняем проверку разбора чисел наречия 1.1
	 */
	{
		// Разобранное число всеми видами его
		yaml::numeric_t number;
		/**
		 * Выполняем проверку разбора восьмеричной записи с ведущим нулём
		 *
		 * @note Ведущий нуль знаменует здесь восьмеричную запись, и `0777` есть 511
		 */
		ASSERT_EQ(yaml::narrow("0777", yaml::schema_t::LEGACY, number), yaml::type_t::UINT16);
		// Выполняем проверку разобранного восьмеричного числа
		ASSERT_EQ(number.natural, 511u);
		// Выполняем проверку разбора двоичной записи
		ASSERT_EQ(yaml::narrow("0b1010", yaml::schema_t::LEGACY, number), yaml::type_t::UINT8);
		// Выполняем проверку разобранного двоичного числа
		ASSERT_EQ(number.natural, 10u);
		/**
		 * Выполняем проверку разбора шестидесятиричной записи
		 *
		 * @note Записью этой пользуются для времени и углов: `12:30` есть 750
		 */
		ASSERT_EQ(yaml::narrow("12:30", yaml::schema_t::LEGACY, number), yaml::type_t::UINT16);
		// Выполняем проверку разобранного шестидесятиричного числа
		ASSERT_EQ(number.natural, 750u);
		// Выполняем проверку разбора шестидесятиричной записи с дробной частью
		ASSERT_EQ(yaml::narrow("190:20:30.15", yaml::schema_t::LEGACY, number), yaml::type_t::DOUBLE);
		// Выполняем проверку разобранного шестидесятиричного числа с дробной частью
		ASSERT_DOUBLE_EQ(number.real, 685230.15);
		/**
		 * Выполняем проверку разбора записи со знаком подчёркивания
		 *
		 * @note Знак этот наречие 1.1 дозволяет между разрядами для удобочитаемости
		 */
		ASSERT_EQ(yaml::narrow("1_000_000", yaml::schema_t::LEGACY, number), yaml::type_t::UINT32);
		// Выполняем проверку разобранного числа со знаком подчёркивания
		ASSERT_EQ(number.natural, 1000000u);
	}
	/**
	 * Выполняем проверку отказа записи, числом не являющейся
	 *
	 * @note Разрешение стоит прежде разбора нарочно: своды правил у них разойтись не
	 *       вправе, и разбор берётся лишь за то, что разрешение числом признало
	 */
	{
		// Разобранное число всеми видами его
		yaml::numeric_t number;
		// Выполняем проверку отказа записи, числом не являющейся
		ASSERT_EQ(yaml::narrow("текст", yaml::schema_t::CORE, number), yaml::type_t::UNDEFINED);
		/**
		 * Выполняем проверку записи с ведущим нулём вне наречия 1.1
		 *
		 * @note Запись эта числом остаётся и здесь, но числом иным: наречие 1.2 читает её
		 *       десятичной и выдаёт 777, а наречие 1.1 - восьмеричной и выдаёт 511. Запрета
		 *       ведущего нуля наречие 1.2 не знает вовсе, и тем оно от JSON расходится
		 */
		ASSERT_EQ(yaml::narrow("0777", yaml::schema_t::CORE, number), yaml::type_t::UINT16);
		// Выполняем проверку разобранного десятичного числа
		ASSERT_EQ(number.natural, 777u);
		// Выполняем проверку отказа шестидесятиричной записи вне наречия 1.1
		ASSERT_EQ(yaml::narrow("12:30", yaml::schema_t::CORE, number), yaml::type_t::UNDEFINED);
		// Выполняем проверку отказа записи под схемою, признающей одни лишь строки
		ASSERT_EQ(yaml::narrow("12", yaml::schema_t::FAILSAFE, number), yaml::type_t::UNDEFINED);
	}
}
/**
 * @brief Проверка выбора двойной ограды по неотменимым знакам
 *
 * @details Знак управляющий записать без отмены нельзя вовсе, и двойная ограда нужна ему
 *          независимо от того, чем значение разрешается действующей схемой. Проверка
 *          неотменимых оттого идёт прежде разрешения вида, а не за ним
 *
 * @note Нашёл это ворошитель сличением перезаписи: значение с переводом строки
 *       разрешалось видом, строкою не являющимся, получало ограду одинарную и растекалось
 *       по строкам, обратным чтением уже не читаясь
 *
 */
TEST(CodecYamlCommon, QuotingEscapes) {
	// Выполняем проверку выбора двойной ограды для многострочного значения
	ASSERT_EQ(yaml::quoting("\"\"\n  a:b\n", yaml::schema_t::CORE, false), yaml::style_t::DOUBLE);
	// Выполняем проверку выбора двойной ограды для многострочного имени пары
	ASSERT_EQ(yaml::quoting("\"\"\n  a:b\n", yaml::schema_t::CORE, true), yaml::style_t::DOUBLE);
	// Выполняем проверку выбора двойной ограды для значения со знаком горизонтальной подачи
	ASSERT_EQ(yaml::quoting("12\t30", yaml::schema_t::CORE, false), yaml::style_t::DOUBLE);
	// Выполняем проверку выбора двойной ограды для записи, числом разрешаемой
	ASSERT_EQ(yaml::quoting("12\n30", yaml::schema_t::CORE, false), yaml::style_t::DOUBLE);
	// Выполняем проверку того, что запись без неотменимых знаков ограды двойной не требует
	ASSERT_EQ(yaml::quoting("12", yaml::schema_t::CORE, false), yaml::style_t::SINGLE);
}
/**
 * @brief Проверка разбора шестидесятиричной записи за пределом целого вида
 *
 * @details Разряды шестидесятиричной записи накапливаются дробным видом, и число их
 *          описанием не ограничено вовсе: запись из десятка разрядов выходит за предел
 *          целого. Приведение такого числа к целому есть неопределённое поведение, и
 *          оттого оно выдаётся дробным видом
 *
 * @note Нашёл это ворошитель под UBSan
 *
 */
TEST(CodecYamlCommon, NarrowSexagesimalOverflow) {
	// Разобранное число, видами своими
	yaml::numeric_t number;
	// Выполняем проверку выдачи дробного вида по числу за пределом целого
	ASSERT_EQ(yaml::narrow("1:1:1:1:1:1:1:1:1:1:1:1", yaml::schema_t::LEGACY, number), yaml::type_t::DOUBLE);
	// Выполняем проверку выдачи узкого целого вида по числу в пределе
	ASSERT_EQ(yaml::narrow("12:30", yaml::schema_t::LEGACY, number), yaml::type_t::UINT16);
	// Выполняем проверку разобранного числа
	ASSERT_EQ(number.natural, 750u);
}
/**
 * @brief Проверка дробного приближения записей иного основания
 *
 * @details Число, за предел целого вышедшее, хранится записью своей, а наружу выдаётся
 * приближением дробным. Собиралось оно разбором дробного числа, а тот основания записи
 * не знает: запись `0xFFFFFFFFFFFFFFFFFF` он читал нулём - разрядов шестнадцатеричных
 * без указателя он не признаёт, - а восьмеричную запись наречия 1.1 читал десятичной
 *
 * @note Нашло это чтение кода с нуля
 *
 */
TEST(CodecYamlCommon, NarrowRadixApproximation) {
	// Разобранное число записи
	yaml::numeric_t number;
	/**
	 * Выполняем проверку шестнадцатеричной записи за пределом целого
	 */
	ASSERT_EQ(yaml::narrow("0xFFFFFFFFFFFFFFFFFF", yaml::schema_t::CORE, number), yaml::type_t::EXTENDED);
	// Выполняем проверку дробного приближения шестнадцатеричной записи
	ASSERT_NEAR(number.real, 4722366482869645213695., 1e8);
	/**
	 * Выполняем проверку восьмеричной записи наречия 1.1 за пределом целого
	 */
	ASSERT_EQ(yaml::narrow("07777777777777777777777777", yaml::schema_t::LEGACY, number), yaml::type_t::EXTENDED);
	// Выполняем проверку дробного приближения восьмеричной записи
	ASSERT_NEAR(number.real, 37778931862957161709567., 1e9);
	/**
	 * Выполняем проверку двоичной записи наречия 1.1 за пределом целого
	 */
	ASSERT_EQ(yaml::narrow("0b" + string(80, '1'), yaml::schema_t::LEGACY, number), yaml::type_t::EXTENDED);
	// Выполняем проверку дробного приближения двоичной записи
	ASSERT_NEAR(number.real, 1208925819614629174706175., 1e11);
	/**
	 * Выполняем проверку того, что десятичная запись приближается по-прежнему
	 */
	ASSERT_EQ(yaml::narrow("99999999999999999999999999", yaml::schema_t::CORE, number), yaml::type_t::EXTENDED);
	// Выполняем проверку дробного приближения десятичной записи
	ASSERT_NEAR(number.real, 1e26, 1e12);
	/**
	 * Выполняем проверку того, что запись со знаком минус приближается верно
	 */
	ASSERT_EQ(yaml::narrow("-0xFFFFFFFFFFFFFFFFFF", yaml::schema_t::CORE, number), yaml::type_t::EXTENDED);
	// Выполняем проверку знака дробного приближения
	ASSERT_NEAR(number.real, -4722366482869645213695., 1e8);
}
/**
 * @brief Проверка описаний кодов отказа и названий видов, набором не взятых
 *
 * @details Описания трёх кодов отказа - файла неоткрытого, узла ненайденного и пути
 *          ошибочного - и выдачи названий при виде неведомом лежали в стороне от всех
 *          трёх прогонов сразу: перечень кодов у проверки описаний три эти кода не
 *          нёс, а вида неведомого разбор не порождает вовсе
 *
 * @note Выдача при виде неведомом нужна журналу: получив число, перечню не
 *       принадлежащее, она обязана выдать слово, а не пустоту да не мусор с памяти
 *
 * @warning Порчею взяты два места из шести - описание узла ненайденного и название
 *          вида узла неведомого, - и подмена выдачи всякого из них проверку валит.
 *          Прочие четыре того же вида и держатся утверждениями теми же; порознь они
 *          не доказаны, ибо всякое доказательство здесь стоит полной пересборки стенда
 *
 */
TEST(CodecYamlCommon, MessagesAndUnknownNames) {
	/**
	 * Выполняем проверку описаний кодов отказа, перечнем проверки не взятых
	 */
	{
		// Коды отказа, описания каких проверяются
		const yaml::error_t errors[] = {
			// Файл документа открыть не удалось
			yaml::error_t::FILE_NOT_OPENED,
			// Узла по такому пути документ не несёт
			yaml::error_t::UNKNOWN_NODE,
			// Путь к узлу построен ошибочно
			yaml::error_t::INVALID_PATH,
			// Запись не отвечает построению, на этом месте открытому
			yaml::error_t::UNEXPECTED_CONTENT
		};
		/**
		 * Выполняем перебор проверяемых кодов отказа
		 */
		for(const yaml::error_t error : errors){
			// Получаем описание кода отказа
			const string description(yaml::message(error));
			// Выполняем проверку того, что описание не пусто
			ASSERT_FALSE(description.empty()) << static_cast <unsigned> (error);
			// Выполняем проверку того, что описание кодом отказа не является
			ASSERT_NE(description, "неизвестный код отказа") << static_cast <unsigned> (error);
		}
	}
	/**
	 * Выполняем проверку выдачи описания при коде отказа неведомом
	 */
	{
		// Выполняем проверку описания, коду неведомому выдаваемого
		ASSERT_STREQ(yaml::message(static_cast <yaml::error_t> (0xFE)), "неизвестный код отказа");
	}
	/**
	 * Выполняем проверку выдачи названий при виде неведомом
	 */
	{
		// Выполняем проверку названия, виду узла неведомому выдаваемого
		ASSERT_STREQ(yaml::name(static_cast <yaml::kind_t> (0xFE)), "unknown");
		// Выполняем проверку названия, виду значения неведомому выдаваемого
		ASSERT_STREQ(yaml::name(static_cast <yaml::type_t> (0xFE000000)), "unknown");
		// Выполняем проверку названия, событию чтения неведомому выдаваемого
		ASSERT_STREQ(yaml::name(static_cast <yaml::event_t> (0xFE)), "UNKNOWN");
	}
}
