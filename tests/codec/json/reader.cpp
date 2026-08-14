/**
 * @file reader.cpp
 * @date 2026-08-14
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Проверки разбора текста JSON — выдача событий значение за значением, снятие
 *        экранирования, разбор чисел, отказы по каждому виду негодного текста, пределы
 *        разбора и независимость выдачи от нарезки текста на куски
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
#include <gtest/gtest.h>
#include <codec/json/json.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;
using namespace awh::codec;

/**
 * @brief Безымянное пространство имён вспомогательных объявлений проверки
 *
 * @note Пространство это обязательно: объявления проверок всех кодеков сходятся в
 *       одну программу, и имя «Event» занято проверками не одного лишь JSON. Два
 *       разных строения под одним именем нарушают правило одного определения, а
 *       сказывается это порчей кучи вдали от места объявления
 *
 */
namespace {
/**
 * @brief Собранное событие разбора текста документа
 *
 */
typedef struct Event {
	// Вид собранного события
	json::event_t event;
	// Содержимое собранного события
	string text;
	// Номер строки события в исходном тексте
	uint32_t line;
	// Положение события в строке исходного текста
	uint32_t column;
} event_t;

/**
 * @brief Метод разбора текста документа кусками заданного размера
 *
 * @param text     разбираемый текст документа
 * @param chunk    размер куска, каким подаётся текст документа, ноль - текст целиком
 * @param settings настройки разбора текста
 * @param error    код отказа разбора
 * @return         собранные события разбора текста документа
 *
 */
static vector <event_t> parse(const string & text, const size_t chunk = 0, const json::reader_t::settings_t & settings = json::reader_t::settings_t(), json::error_t * error = nullptr) noexcept {
	// Собранные события разбора текста документа
	vector <event_t> result;
	// Объект разбора текста документа
	json::reader_t reader;
	// Выполняем установку настроек разбора текста
	reader.settings(settings);
	// Получаем размер куска, каким подаётся текст документа
	const size_t size = ((chunk > 0) ? chunk : text.size());
	/**
	 * Выполняем подачу текста документа кусками заданного размера
	 */
	for(size_t offset = 0; offset <= text.size(); offset += size){
		// Получаем размер очередного подаваемого куска
		const size_t length = (((offset + size) < text.size()) ? size : (text.size() - offset));
		// Выполняем подачу очередного куска текста документа разбору
		const bool ok = reader.feed(text.data() + offset, length, ((offset + length) >= text.size()));
		/**
		 * Выполняем перебор всех собранных событий разбора
		 */
		while(reader.next()){
			// Получаем значение очередного события разбора
			const json::reader_t::value_t value = reader.value();
			// Выполняем добавление собранного события к полученным
			result.push_back(event_t{reader.event(), string(value.text), reader.location().line, reader.location().column});
		}
		/**
		 * Если разбор куска текста документа завершился отказом
		 */
		if(!ok)
			// Прекращаем разбор текста документа
			break;
		/**
		 * Если текст документа исчерпан
		 */
		if((offset + length) >= text.size())
			// Прекращаем разбор текста документа
			break;
	}
	/**
	 * Если код отказа разбора затребован
	 */
	if(error != nullptr)
		// Запоминаем код отказа разбора
		(* error) = reader.error();
	// Выводим полученный результат
	return result;
}
/**
 * @brief Метод сведения событий разбора в одну запись
 *
 * @param events сводимые события разбора
 * @return       запись сведённых событий разбора
 *
 */
static string join(const vector <event_t> & events) noexcept {
	// Запись сведённых событий разбора
	string result;
	/**
	 * Выполняем перебор всех событий разбора
	 */
	for(const event_t & item : events){
		/**
		 * Если запись уже содержит события
		 */
		if(!result.empty())
			// Записываем знак-разделитель событий
			result.append(1, ' ');
		/**
		 * Определяем вид очередного события разбора
		 */
		switch(static_cast <uint8_t> (item.event)){
			// Если событие является именем поля объекта
			case static_cast <uint8_t> (json::event_t::KEY):
				// Записываем имя поля объекта
				result.append("KEY(").append(item.text).append(1, ')');
			break;
			// Если событие является пустым значением
			case static_cast <uint8_t> (json::event_t::NUL):
				// Записываем пустое значение
				result.append("NULL");
			break;
			// Если событие является логическим значением
			case static_cast <uint8_t> (json::event_t::BOOL):
				// Записываем логическое значение
				result.append("BOOL(").append(item.text).append(1, ')');
			break;
			// Если событие является числом
			case static_cast <uint8_t> (json::event_t::NUMBER):
				// Записываем число его записью как она есть
				result.append("NUM(").append(item.text).append(1, ')');
			break;
			// Если событие является строкой
			case static_cast <uint8_t> (json::event_t::STRING):
				// Записываем строковое значение
				result.append("STR(").append(item.text).append(1, ')');
			break;
			// Если событие является примечанием
			case static_cast <uint8_t> (json::event_t::COMMENT):
				// Записываем содержимое примечания
				result.append("REM(").append(item.text).append(1, ')');
			break;
			// Если событие является открытием массива
			case static_cast <uint8_t> (json::event_t::ARRAY_BEGIN):
				// Записываем открытие массива
				result.append(1, '[');
			break;
			// Если событие является закрытием массива
			case static_cast <uint8_t> (json::event_t::ARRAY_END):
				// Записываем закрытие массива
				result.append(1, ']');
			break;
			// Если событие является открытием объекта
			case static_cast <uint8_t> (json::event_t::OBJECT_BEGIN):
				// Записываем открытие объекта
				result.append(1, '{');
			break;
			// Если событие является закрытием объекта
			case static_cast <uint8_t> (json::event_t::OBJECT_END):
				// Записываем закрытие объекта
				result.append(1, '}');
			break;
			// Если событие является окончанием документа
			case static_cast <uint8_t> (json::event_t::DOCUMENT):
				// Записываем окончание документа
				result.append("DOC");
			break;
			// Если событие является исчерпанием подаваемого текста
			case static_cast <uint8_t> (json::event_t::FINISH):
				// Записываем исчерпание подаваемого текста
				result.append("END");
			break;
		}
	}
	// Выводим полученный результат
	return result;
}
/**
 * @brief Метод извлечения кода отказа разбора текста документа
 *
 * @param text     разбираемый текст документа
 * @param settings настройки разбора текста
 * @return         код отказа разбора
 *
 */
static json::error_t failure(const string & text, const json::reader_t::settings_t & settings = json::reader_t::settings_t()) noexcept {
	// Код отказа разбора текста документа
	json::error_t result = json::error_t::NONE;
	// Выполняем разбор текста документа
	::parse(text, 0, settings, & result);
	// Выводим полученный результат
	return result;
}
}

/**
 * @brief Проверка разбора значений верхнего уровня
 *
 */
TEST(CodecJsonReader, Scalars) {
	// Выполняем проверку разбора пустого значения
	ASSERT_EQ(::join(::parse("null")), "NULL DOC END");
	// Выполняем проверку разбора истины
	ASSERT_EQ(::join(::parse("true")), "BOOL(true) DOC END");
	// Выполняем проверку разбора лжи
	ASSERT_EQ(::join(::parse("false")), "BOOL(false) DOC END");
	// Выполняем проверку разбора числа
	ASSERT_EQ(::join(::parse("42")), "NUM(42) DOC END");
	// Выполняем проверку разбора строки
	ASSERT_EQ(::join(::parse("\"текст\"")), "STR(текст) DOC END");
	// Выполняем проверку разбора значения с пробельной обвязкой
	ASSERT_EQ(::join(::parse("  \r\n\t 17 \n ")), "NUM(17) DOC END");
}
/**
 * @brief Проверка разбора вместилищ
 *
 */
TEST(CodecJsonReader, Containers) {
	// Выполняем проверку разбора пустого массива
	ASSERT_EQ(::join(::parse("[]")), "[ ] DOC END");
	// Выполняем проверку разбора пустого объекта
	ASSERT_EQ(::join(::parse("{}")), "{ } DOC END");
	// Выполняем проверку разбора массива значений
	ASSERT_EQ(::join(::parse("[1,\"a\",true,null]")), "[ NUM(1) STR(a) BOOL(true) NULL ] DOC END");
	// Выполняем проверку разбора объекта полей
	ASSERT_EQ(::join(::parse("{\"a\":1,\"b\":2}")), "{ KEY(a) NUM(1) KEY(b) NUM(2) } DOC END");
	// Выполняем проверку разбора вложенных вместилищ
	ASSERT_EQ(::join(::parse("{\"a\":[{\"b\":[]}]}")), "{ KEY(a) [ { KEY(b) [ ] } ] } DOC END");
	// Выполняем проверку разбора вместилищ с пробельной обвязкой
	ASSERT_EQ(::join(::parse("{ \"a\" : [ 1 , 2 ] }")), "{ KEY(a) [ NUM(1) NUM(2) ] } DOC END");
}
/**
 * @brief Проверка снятия экранирования строк
 *
 */
TEST(CodecJsonReader, Escapes) {
	// Выполняем проверку снятия сокращённых отменяющих записей
	ASSERT_EQ(::join(::parse("\"\\\"\\\\\\/\\b\\f\\n\\r\\t\"")), string("STR(\"\\/\b\f\n\r\t) DOC END"));
	// Выполняем проверку снятия отменяющей записи однобайтового знака
	ASSERT_EQ(::join(::parse("\"\\u0041\"")), "STR(A) DOC END");
	// Выполняем проверку снятия отменяющей записи двухбайтового знака
	ASSERT_EQ(::join(::parse("\"\\u00e9\"")), "STR(é) DOC END");
	// Выполняем проверку снятия отменяющей записи трёхбайтового знака
	ASSERT_EQ(::join(::parse("\"\\u20ac\"")), "STR(€) DOC END");
	// Выполняем проверку снятия отменяющей записи в верхнем регистре
	ASSERT_EQ(::join(::parse("\"\\u20AC\"")), "STR(€) DOC END");
	// Выполняем проверку сборки суррогатной пары в один знак
	ASSERT_EQ(::join(::parse("\"\\ud83d\\ude00\"")), "STR(😀) DOC END");
	// Выполняем проверку разбора знака, записанного самим собою
	ASSERT_EQ(::join(::parse("\"привет 😀\"")), "STR(привет 😀) DOC END");
}
/**
 * @brief Проверка отказа на негодном экранировании строк
 *
 */
TEST(CodecJsonReader, EscapeFailures) {
	// Выполняем проверку отклонения неопознанной отменяющей записи
	ASSERT_EQ(::failure("\"\\x\""), json::error_t::INVALID_ESCAPE);
	// Выполняем проверку отклонения отменяющей записи с недопустимым знаком
	ASSERT_EQ(::failure("\"\\u00zz\""), json::error_t::INVALID_UNICODE);
	// Выполняем проверку отклонения одинокого старшего суррогата
	ASSERT_EQ(::failure("\"\\ud83d\""), json::error_t::UNPAIRED_SURROGATE);
	// Выполняем проверку отклонения одинокого младшего суррогата
	ASSERT_EQ(::failure("\"\\ude00\""), json::error_t::UNPAIRED_SURROGATE);
	// Выполняем проверку отклонения старшего суррогата без пары
	ASSERT_EQ(::failure("\"\\ud83dA\""), json::error_t::UNPAIRED_SURROGATE);
	// Выполняем проверку отклонения управляющего знака внутри строки
	ASSERT_EQ(::failure(string("\"a\nb\"")), json::error_t::CONTROL_IN_STRING);
	// Выполняем проверку отклонения незакрытой строки
	ASSERT_EQ(::failure("\"abc"), json::error_t::UNTERMINATED_STRING);
}
/**
 * @brief Проверка разбора чисел
 *
 */
TEST(CodecJsonReader, Numbers) {
	// Выполняем проверку разбора нуля
	ASSERT_EQ(::join(::parse("0")), "NUM(0) DOC END");
	// Выполняем проверку разбора отрицательного числа
	ASSERT_EQ(::join(::parse("-17")), "NUM(-17) DOC END");
	// Выполняем проверку разбора дробного числа
	ASSERT_EQ(::join(::parse("3.14159")), "NUM(3.14159) DOC END");
	// Выполняем проверку разбора числа с порядком
	ASSERT_EQ(::join(::parse("1e10")), "NUM(1e10) DOC END");
	// Выполняем проверку разбора числа с порядком в верхнем регистре
	ASSERT_EQ(::join(::parse("1E+10")), "NUM(1E+10) DOC END");
	// Выполняем проверку разбора числа с отрицательным порядком
	ASSERT_EQ(::join(::parse("-1.5e-30")), "NUM(-1.5e-30) DOC END");
	/**
	 * Выполняем проверку сохранности точности записи числа
	 *
	 * @note Число это не представимо видом с плавающей запятой двойной точности, и
	 *       преобразование при разборе потеряло бы младший разряд его
	 */
	ASSERT_EQ(::join(::parse("9007199254740993")), "NUM(9007199254740993) DOC END");
}
/**
 * @brief Проверка отказа на негодной записи числа
 *
 */
TEST(CodecJsonReader, NumberFailures) {
	// Выполняем проверку отклонения ведущего нуля
	ASSERT_EQ(::failure("01"), json::error_t::INVALID_NUMBER);
	// Выполняем проверку отклонения ведущего плюса
	ASSERT_EQ(::failure("+1"), json::error_t::EXPECTED_VALUE);
	// Выполняем проверку отклонения точки без цифры перед нею
	ASSERT_EQ(::failure(".5"), json::error_t::EXPECTED_VALUE);
	// Выполняем проверку отклонения точки без цифры за нею
	ASSERT_EQ(::failure("[1.]"), json::error_t::INVALID_NUMBER);
	// Выполняем проверку отклонения порядка без цифр
	ASSERT_EQ(::failure("[1e]"), json::error_t::INVALID_NUMBER);
	// Выполняем проверку отклонения порядка со знаком, но без цифр
	ASSERT_EQ(::failure("[1e+]"), json::error_t::INVALID_NUMBER);
	// Выполняем проверку отклонения одинокого знака минуса
	ASSERT_EQ(::failure("[-]"), json::error_t::INVALID_NUMBER);
	/**
	 * Выполняем проверку отклонения записи числа, оборванной концом текста
	 *
	 * @note Отказ здесь иной: запись числа могла бы продолжиться следующим куском
	 *       текста, и негодной её делает именно обрыв, а не сама запись
	 */
	ASSERT_EQ(::failure("1."), json::error_t::UNEXPECTED_EOF);
	// Выполняем проверку отклонения записи NaN при строгом разборе
	ASSERT_EQ(::failure("NaN"), json::error_t::EXPECTED_VALUE);
	// Выполняем проверку отклонения записи Infinity при строгом разборе
	ASSERT_EQ(::failure("Infinity"), json::error_t::EXPECTED_VALUE);
}
/**
 * @brief Проверка отказа на негодном строении документа
 *
 */
TEST(CodecJsonReader, StructureFailures) {
	// Выполняем проверку отклонения пустого текста
	ASSERT_EQ(::failure(""), json::error_t::EMPTY_TEXT);
	// Выполняем проверку отклонения незакрытого массива
	ASSERT_EQ(::failure("[1,2"), json::error_t::UNEXPECTED_EOF);
	// Выполняем проверку отклонения незакрытого объекта
	ASSERT_EQ(::failure("{\"a\":1"), json::error_t::UNEXPECTED_EOF);
	// Выполняем проверку отклонения закрытия массива скобкой объекта
	ASSERT_EQ(::failure("[1}"), json::error_t::EXPECTED_COMMA);
	// Выполняем проверку отклонения имени поля объекта без кавычек
	ASSERT_EQ(::failure("{a:1}"), json::error_t::EXPECTED_KEY);
	// Выполняем проверку отклонения имени поля объекта без двоеточия
	ASSERT_EQ(::failure("{\"a\" 1}"), json::error_t::EXPECTED_COLON);
	// Выполняем проверку отклонения значения без запятой перед ним
	ASSERT_EQ(::failure("[1 2]"), json::error_t::EXPECTED_COMMA);
	// Выполняем проверку отклонения запятой перед закрывающей скобкой
	ASSERT_EQ(::failure("[1,]"), json::error_t::TRAILING_COMMA);
	// Выполняем проверку отклонения запятой перед закрывающей скобкой объекта
	ASSERT_EQ(::failure("{\"a\":1,}"), json::error_t::TRAILING_COMMA);
	// Выполняем проверку отклонения пустого места вместо значения
	ASSERT_EQ(::failure("[,]"), json::error_t::EXPECTED_VALUE);
	// Выполняем проверку отклонения слова, оборванного концом текста
	ASSERT_EQ(::failure("tru"), json::error_t::UNEXPECTED_EOF);
	// Выполняем проверку отклонения искажённого слова
	ASSERT_EQ(::failure("truu"), json::error_t::INVALID_LITERAL);
	// Выполняем проверку отклонения знаков за окончанием документа
	ASSERT_EQ(::failure("{} {}"), json::error_t::TRAILING_CHARACTERS);
}
/**
 * @brief Проверка разбора послаблений сверх стандарта
 *
 */
TEST(CodecJsonReader, Relaxations) {
	// Настройки разбора текста документа
	json::reader_t::settings_t settings;
	// Разрешаем запятую перед закрывающей скобкой
	settings.allowTrailingCommas = true;
	// Выполняем проверку разбора запятой перед закрывающей скобкой массива
	ASSERT_EQ(::join(::parse("[1,]", 0, settings)), "[ NUM(1) ] DOC END");
	// Выполняем проверку разбора запятой перед закрывающей скобкой объекта
	ASSERT_EQ(::join(::parse("{\"a\":1,}", 0, settings)), "{ KEY(a) NUM(1) } DOC END");
	// Возвращаем настройки разбора текста документа к умолчанию
	settings = json::reader_t::settings_t();
	// Разрешаем строки в одинарных кавычках
	settings.allowSingleQuotes = true;
	// Выполняем проверку разбора строки в одинарных кавычках
	ASSERT_EQ(::join(::parse("{'a':'б'}", 0, settings)), "{ KEY(a) STR(б) } DOC END");
	// Выполняем проверку разбора двойной кавычки внутри одинарных
	ASSERT_EQ(::join(::parse("'a\"b'", 0, settings)), "STR(a\"b) DOC END");
	// Возвращаем настройки разбора текста документа к умолчанию
	settings = json::reader_t::settings_t();
	// Разрешаем записи NaN и бесконечности
	settings.allowInfinityAndNan = true;
	// Выполняем проверку разбора записи NaN
	ASSERT_EQ(::join(::parse("NaN", 0, settings)), "NUM(NaN) DOC END");
	// Выполняем проверку разбора записи бесконечности
	ASSERT_EQ(::join(::parse("Infinity", 0, settings)), "NUM(Infinity) DOC END");
	// Выполняем проверку разбора записи отрицательной бесконечности
	ASSERT_EQ(::join(::parse("-Infinity", 0, settings)), "NUM(-Infinity) DOC END");
}
/**
 * @brief Проверка разбора примечаний
 *
 */
TEST(CodecJsonReader, Comments) {
	// Выполняем проверку отклонения примечания при строгом разборе
	ASSERT_EQ(::failure("[1] // хвост"), json::error_t::COMMENT_NOT_ALLOWED);
	// Настройки разбора текста документа
	json::reader_t::settings_t settings;
	// Разрешаем примечания
	settings.allowComments = true;
	// Выполняем проверку пропуска примечания до конца строки
	ASSERT_EQ(::join(::parse("[1] // хвост", 0, settings)), "[ NUM(1) ] DOC END");
	// Выполняем проверку пропуска примечания в скобках
	ASSERT_EQ(::join(::parse("[/* тут */1]", 0, settings)), "[ NUM(1) ] DOC END");
	// Выполняем проверку пропуска примечания между полями объекта
	ASSERT_EQ(::join(::parse("{\"a\":1 /* и */, \"b\":2}", 0, settings)), "{ KEY(a) NUM(1) KEY(b) NUM(2) } DOC END");
	// Выполняем проверку отклонения незакрытого примечания в скобках
	ASSERT_EQ(::failure("[1 /* хвост", settings), json::error_t::UNTERMINATED_COMMENT);
	// Затребуем выдачу событий примечаний
	settings.emitComments = true;
	// Выполняем проверку выдачи содержимого примечания
	ASSERT_EQ(::join(::parse("[1]// хвост", 0, settings)), "[ NUM(1) ] DOC REM( хвост) END");
}
/**
 * @brief Проверка разбора потока документов NDJSON
 *
 */
TEST(CodecJsonReader, Stream) {
	// Настройки разбора текста документа
	json::reader_t::settings_t settings;
	// Разрешаем разбор потока документов
	settings.stream = true;
	// Выполняем проверку разбора нескольких документов подряд
	ASSERT_EQ(::join(::parse("{\"a\":1}\n{\"a\":2}\n[3]", 0, settings)), "{ KEY(a) NUM(1) } DOC { KEY(a) NUM(2) } DOC [ NUM(3) ] DOC END");
	// Выполняем проверку разбора потока значений верхнего уровня
	ASSERT_EQ(::join(::parse("1\n2\n3", 0, settings)), "NUM(1) DOC NUM(2) DOC NUM(3) DOC END");
}
/**
 * @brief Проверка соблюдения пределов разбора
 *
 */
TEST(CodecJsonReader, Limits) {
	// Настройки разбора текста документа
	json::reader_t::settings_t settings;
	// Устанавливаем наибольшую допустимую длину строкового значения
	settings.maxString = 4;
	// Выполняем проверку разбора строки на пределе длины
	ASSERT_EQ(::join(::parse("\"abcd\"", 0, settings)), "STR(abcd) DOC END");
	// Выполняем проверку отклонения строки сверх предела длины
	ASSERT_EQ(::failure("\"abcde\"", settings), json::error_t::STRING_TOO_LONG);
	// Возвращаем настройки разбора текста документа к умолчанию
	settings = json::reader_t::settings_t();
	// Устанавливаем наибольшую допустимую длину записи числа
	settings.maxNumber = 3;
	// Выполняем проверку отклонения записи числа сверх предела длины
	ASSERT_EQ(::failure("12345", settings), json::error_t::NUMBER_TOO_LONG);
	// Возвращаем настройки разбора текста документа к умолчанию
	settings = json::reader_t::settings_t();
	// Устанавливаем наибольшую допустимую глубину вложенности
	settings.maxDepth = 3;
	// Выполняем проверку разбора вложенности на пределе глубины
	ASSERT_EQ(::join(::parse("[[[]]]", 0, settings)), "[ [ [ ] ] ] DOC END");
	// Выполняем проверку отклонения вложенности сверх предела глубины
	ASSERT_EQ(::failure("[[[[]]]]", settings), json::error_t::DEPTH_EXCEEDED);
}
/**
 * @brief Проверка учёта положения событий в исходном тексте
 *
 */
TEST(CodecJsonReader, Location) {
	// Выполняем разбор текста документа, разложенного по строкам
	const vector <event_t> events = ::parse("{\n  \"a\": 1,\n  \"b\": [\n    true\n  ]\n}");
	// Выполняем проверку количества собранных событий разбора
	ASSERT_EQ(events.size(), 10u);
	// Выполняем проверку положения открытия объекта
	ASSERT_EQ(events[0].line, 1u);
	// Выполняем проверку положения имени первого поля объекта
	ASSERT_EQ(events[1].line, 2u);
	// Выполняем проверку положения имени первого поля объекта в строке
	ASSERT_EQ(events[1].column, 3u);
	// Выполняем проверку положения логического значения внутри массива
	ASSERT_EQ(events[5].line, 4u);
	// Выполняем проверку положения логического значения в строке
	ASSERT_EQ(events[5].column, 5u);
	// Объект разбора текста документа
	json::reader_t reader;
	// Выполняем подачу негодного текста документа разбору
	ASSERT_FALSE(reader.feed(string_view("{\n  \"a\": tru\n}")));
	// Выполняем проверку кода отказа разбора
	ASSERT_EQ(reader.error(), json::error_t::INVALID_LITERAL);
	// Выполняем проверку номера строки отказа разбора
	ASSERT_EQ(reader.location().line, 2u);
}
/**
 * @brief Проверка учёта глубины вложенности разбора
 *
 */
TEST(CodecJsonReader, Depth) {
	// Объект разбора текста документа
	json::reader_t reader;
	// Выполняем проверку глубины вложенности до подачи текста
	ASSERT_EQ(reader.depth(), 0u);
	/**
	 * Выполняем подачу незавершённого текста документа разбору
	 *
	 * @note Первые четыре байта текста разбор удерживает до определения кодировки по
	 *       метке порядка байтов, и глубина вложенности до того остаётся нулевой
	 */
	ASSERT_TRUE(reader.feed("[[{ ", 4, false));
	// Выполняем проверку глубины вложенности посреди разбора
	ASSERT_EQ(reader.depth(), 3u);
	// Выполняем подачу остатка текста документа разбору
	ASSERT_TRUE(reader.feed("}]]", 3, true));
	// Выполняем проверку глубины вложенности по окончании разбора
	ASSERT_EQ(reader.depth(), 0u);
}
/**
 * @brief Проверка сброса состояния разбора
 *
 */
TEST(CodecJsonReader, Reset) {
	// Объект разбора текста документа
	json::reader_t reader;
	// Выполняем подачу негодного текста документа разбору
	ASSERT_FALSE(reader.feed(string_view("[1,]")));
	// Выполняем проверку кода отказа разбора
	ASSERT_EQ(reader.error(), json::error_t::TRAILING_COMMA);
	// Выполняем сброс состояния разбора
	reader.reset();
	// Выполняем проверку сброса кода отказа разбора
	ASSERT_EQ(reader.error(), json::error_t::NONE);
	// Выполняем подачу годного текста документа разбору
	ASSERT_TRUE(reader.feed(string_view("[1]")));
	// Выполняем проверку отсутствия отказа разбора
	ASSERT_EQ(reader.error(), json::error_t::NONE);
}
/**
 * @brief Проверка допустимости управляющих знаков по местам текста
 *
 * @details Приведение кодировки судит лишь о том, обозначает ли последовательность
 * знак вообще, а место знака решает разбор. RFC 8259 дозволяет без отмены всё, что
 * не ниже пробела, и забой с областью C1 в том числе - отказ на них сделал бы разбор
 * строже стандарта
 *
 * @note Проверка эта закрепляет намеренное решение: приведение кодировок прочих
 * кодеков забой и область C1 отвергает, и правило это было перенесено сюда вместе с
 * прочим устройством. Обнаружено сличением с набором JSONTestSuite
 *
 */
TEST(CodecJsonReader, Controls) {
	// Выполняем проверку разбора забоя внутри строки
	ASSERT_EQ(::join(::parse(string("\"a\x7F""a\""))), string("STR(a\x7F""a) DOC END"));
	// Выполняем проверку разбора знака области C1 внутри строки
	ASSERT_EQ(::join(::parse(string("\"\xC2\x9F\""))), string("STR(\xC2\x9F) DOC END"));
	// Выполняем проверку отклонения управляющего знака области C0 внутри строки
	ASSERT_EQ(::failure(string("\"a\x01""a\"")), json::error_t::CONTROL_IN_STRING);
	// Выполняем проверку отклонения знака табуляции внутри строки без отмены
	ASSERT_EQ(::failure(string("\"a\ta\"")), json::error_t::CONTROL_IN_STRING);
	// Выполняем проверку разбора пробельных знаков между значениями
	ASSERT_EQ(::join(::parse(string("[\t1\r\n]"))), "[ NUM(1) ] DOC END");
	// Выполняем проверку отклонения управляющего знака между значениями
	ASSERT_EQ(::failure(string("[\x01""1]")), json::error_t::EXPECTED_VALUE);
}
/**
 * @brief Проверка независимости разбора от нарезки текста на куски
 *
 * @details Разбор обязан выдавать одни и те же события при всякой нарезке подаваемого
 * текста: знак, чьё значение зависит от следующего за ним, лишь переводит разбор в
 * отдельное состояние. Проверка ведётся всеми размерами куска от одного знака до
 * целого текста, а сличаются вид события, содержимое его и положение в исходном тексте
 *
 */
TEST(CodecJsonReader, ChunkIndependence) {
	// Настройки разбора текста документа
	json::reader_t::settings_t settings;
	// Разрешаем примечания
	settings.allowComments = true;
	// Затребуем выдачу событий примечаний
	settings.emitComments = true;
	// Разрешаем разбор потока документов
	settings.stream = true;
	// Разрешаем записи NaN и бесконечности
	settings.allowInfinityAndNan = true;
	/**
	 * Разбираемые тексты документов
	 */
	const vector <string> texts = {
		"{\"a\":1,\"b\":[true,null,\"x\"]}",
		"\"\\ud83d\\ude00\"",
		"\"\\u0041\\u00e9\\u20ac\"",
		"{\"n\":9007199254740993,\"f\":-1.5e-30}",
		"[[[[[1]]]]]",
		"{\"ключ\":\"значение\"}",
		"[1, /* примечание */ 2] // хвост",
		"{\"a\":1}\n{\"b\":2}",
		"[NaN,Infinity,-Infinity]",
		"\"\\\"\\\\\\/\\b\\f\\n\\r\\t\"",
		"[01]",
		"{\"a\":tru}"
	};
	/**
	 * Выполняем перебор всех разбираемых текстов документов
	 */
	for(const string & text : texts){
		// Код отказа разбора текста документа целиком
		json::error_t error = json::error_t::NONE;
		// Выполняем разбор текста документа целиком
		const string sample = ::join(::parse(text, 0, settings, & error));
		/**
		 * Выполняем перебор всех размеров куска подаваемого текста
		 */
		for(size_t chunk = 1; chunk <= text.size(); chunk++){
			// Код отказа разбора текста документа кусками
			json::error_t reason = json::error_t::NONE;
			// Выполняем проверку совпадения событий разбора с эталонными
			ASSERT_EQ(::join(::parse(text, chunk, settings, & reason)), sample)
				<< "текст «" << text << "» кусками по " << chunk << " знаков";
			// Выполняем проверку совпадения кода отказа разбора с эталонным
			ASSERT_EQ(reason, error)
				<< "текст «" << text << "» кусками по " << chunk << " знаков";
		}
	}
}
