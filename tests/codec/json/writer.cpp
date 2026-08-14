/**
 * @file writer.cpp
 * @date 2026-08-14
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Проверки записи текста JSON — оформление с отступами и без, способы
 *        экранирования, стережение строения документа, кратчайшая запись чисел,
 *        потоковое изъятие собранного и круговой проход записанного через разбор
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <limits>

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
 * @brief Проверка записи значений верхнего уровня
 *
 */
TEST(CodecJsonWriter, Scalars) {
	// Объект записи текста документа
	json::writer_t writer;
	// Выполняем проверку записи пустого значения
	ASSERT_TRUE(writer.null());
	// Выполняем проверку собранного текста
	ASSERT_EQ(writer.text(), "null");
	// Выполняем сброс состояния записи
	writer.reset();
	// Выполняем проверку записи истины
	ASSERT_TRUE(writer.value(true));
	// Выполняем проверку собранного текста
	ASSERT_EQ(writer.text(), "true");
	// Выполняем сброс состояния записи
	writer.reset();
	// Выполняем проверку записи целого числа со знаком
	ASSERT_TRUE(writer.value(static_cast <int64_t> (-42)));
	// Выполняем проверку собранного текста
	ASSERT_EQ(writer.text(), "-42");
	// Выполняем сброс состояния записи
	writer.reset();
	// Выполняем проверку записи беззнакового целого числа
	ASSERT_TRUE(writer.value(static_cast <uint64_t> (18446744073709551615ull)));
	// Выполняем проверку собранного текста
	ASSERT_EQ(writer.text(), "18446744073709551615");
	// Выполняем сброс состояния записи
	writer.reset();
	// Выполняем проверку записи строкового значения
	ASSERT_TRUE(writer.value(string("текст")));
	// Выполняем проверку собранного текста
	ASSERT_EQ(writer.text(), "\"текст\"");
}
/**
 * @brief Проверка записи вместилищ
 *
 */
TEST(CodecJsonWriter, Containers) {
	// Объект записи текста документа
	json::writer_t writer;
	// Выполняем запись пустого массива
	ASSERT_TRUE(writer.array());
	// Выполняем закрытие пустого массива
	ASSERT_TRUE(writer.close());
	// Выполняем проверку собранного текста
	ASSERT_EQ(writer.text(), "[]");
	// Выполняем сброс состояния записи
	writer.reset();
	// Выполняем запись объекта с полями
	ASSERT_TRUE(writer.object());
	// Выполняем запись имени первого поля объекта
	ASSERT_TRUE(writer.key("a"));
	// Выполняем запись значения первого поля объекта
	ASSERT_TRUE(writer.value(static_cast <int64_t> (1)));
	// Выполняем запись имени второго поля объекта
	ASSERT_TRUE(writer.key("b"));
	// Выполняем запись вложенного массива
	ASSERT_TRUE(writer.array());
	// Выполняем запись значения вложенного массива
	ASSERT_TRUE(writer.value(string("x")));
	// Выполняем запись вложенного объекта
	ASSERT_TRUE(writer.object());
	// Выполняем закрытие вложенного объекта
	ASSERT_TRUE(writer.close());
	// Выполняем закрытие вложенного массива
	ASSERT_TRUE(writer.close());
	// Выполняем закрытие объекта
	ASSERT_TRUE(writer.close());
	// Выполняем проверку собранного текста
	ASSERT_EQ(writer.text(), "{\"a\":1,\"b\":[\"x\",{}]}");
	// Выполняем проверку глубины вложенности по окончании записи
	ASSERT_EQ(writer.depth(), 0u);
}
/**
 * @brief Проверка стережения строения документа
 *
 * @details Сборщик обещает правильность собранного текста по строению, и обещание
 * это держится отказом на всякое действие, строение нарушающее
 *
 */
TEST(CodecJsonWriter, Structure) {
	// Объект записи текста документа
	json::writer_t writer;
	// Выполняем проверку отклонения закрытия неоткрытого вместилища
	ASSERT_FALSE(writer.close());
	// Выполняем открытие объекта
	ASSERT_TRUE(writer.object());
	// Выполняем проверку отклонения значения на месте имени поля объекта
	ASSERT_FALSE(writer.value(true));
	// Выполняем проверку отклонения вместилища на месте имени поля объекта
	ASSERT_FALSE(writer.array());
	// Выполняем запись имени поля объекта
	ASSERT_TRUE(writer.key("a"));
	// Выполняем проверку отклонения повторного имени поля объекта
	ASSERT_FALSE(writer.key("b"));
	// Выполняем проверку отклонения закрытия объекта без значения поля
	ASSERT_FALSE(writer.close());
	// Выполняем запись значения поля объекта
	ASSERT_TRUE(writer.value(true));
	// Выполняем закрытие объекта
	ASSERT_TRUE(writer.close());
	// Выполняем проверку отклонения второго документа подряд
	ASSERT_FALSE(writer.value(true));
	// Выполняем проверку собранного текста
	ASSERT_EQ(writer.text(), "{\"a\":true}");
	// Выполняем сброс состояния записи
	writer.reset();
	// Выполняем открытие массива
	ASSERT_TRUE(writer.array());
	// Выполняем проверку отклонения имени поля объекта внутри массива
	ASSERT_FALSE(writer.key("a"));
}
/**
 * @brief Проверка оформления собираемого текста отступами
 *
 */
TEST(CodecJsonWriter, Pretty) {
	// Объект записи текста документа
	json::writer_t writer;
	// Получаем настройки записи текста документа
	json::writer_t::settings_t settings = writer.settings();
	// Устанавливаем оформление собираемого текста отступами
	settings.format = json::format_t::PRETTY;
	// Выполняем установку настроек записи текста
	writer.settings(settings);
	// Выполняем запись объекта с вложенными вместилищами
	writer.object();
	// Выполняем запись имени первого поля объекта
	writer.key("a");
	// Выполняем запись значения первого поля объекта
	writer.value(static_cast <int64_t> (1));
	// Выполняем запись имени второго поля объекта
	writer.key("b");
	// Выполняем запись вложенного массива
	writer.array();
	// Выполняем запись значения вложенного массива
	writer.value(true);
	// Выполняем закрытие вложенного массива
	writer.close();
	// Выполняем запись имени третьего поля объекта
	writer.key("c");
	// Выполняем запись пустого вложенного объекта
	writer.object();
	// Выполняем закрытие пустого вложенного объекта
	writer.close();
	// Выполняем закрытие объекта
	writer.close();
	/**
	 * Выполняем проверку собранного текста
	 *
	 * @note Пустое вместилище закрывается вплотную намеренно: перевод строки внутри
	 *       него ничего не разделяет, а место занимает
	 */
	ASSERT_EQ(writer.text(), "{\n  \"a\": 1,\n  \"b\": [\n    true\n  ],\n  \"c\": {}\n}");
	// Устанавливаем отступ знаком табуляции
	settings.indent = 0;
	// Выполняем установку настроек записи текста
	writer.settings(settings);
	// Выполняем сброс состояния записи
	writer.reset();
	// Выполняем запись массива со значением
	writer.array();
	// Выполняем запись значения массива
	writer.value(static_cast <int64_t> (1));
	// Выполняем закрытие массива
	writer.close();
	// Выполняем проверку собранного текста
	ASSERT_EQ(writer.text(), "[\n\t1\n]");
	// Устанавливаем отступ четырьмя пробелами
	settings.indent = 4;
	// Выполняем установку настроек записи текста
	writer.settings(settings);
	// Выполняем сброс состояния записи
	writer.reset();
	// Выполняем запись массива со значением
	writer.array();
	// Выполняем запись значения массива
	writer.value(static_cast <int64_t> (1));
	// Выполняем закрытие массива
	writer.close();
	// Выполняем проверку собранного текста
	ASSERT_EQ(writer.text(), "[\n    1\n]");
}
/**
 * @brief Проверка экранирования записываемых строк
 *
 */
TEST(CodecJsonWriter, Escapes) {
	// Объект записи текста документа
	json::writer_t writer;
	// Выполняем запись строки со знаками, требующими экранирования
	ASSERT_TRUE(writer.value(string("\"\\\b\f\n\r\t")));
	// Выполняем проверку записи знаков сокращёнными записями
	ASSERT_EQ(writer.text(), "\"\\\"\\\\\\b\\f\\n\\r\\t\"");
	// Выполняем сброс состояния записи
	writer.reset();
	// Выполняем запись строки с управляющим знаком без сокращённой записи
	ASSERT_TRUE(writer.value(string(1, '\x01')));
	// Выполняем проверку записи управляющего знака кодовым значением
	ASSERT_EQ(writer.text(), "\"\\u0001\"");
	// Выполняем сброс состояния записи
	writer.reset();
	// Выполняем запись строки с косой чертой
	ASSERT_TRUE(writer.value(string("a/b")));
	// Выполняем проверку записи косой черты как есть по умолчанию
	ASSERT_EQ(writer.text(), "\"a/b\"");
	// Получаем настройки записи текста документа
	json::writer_t::settings_t settings = writer.settings();
	// Устанавливаем экранирование косой черты
	settings.escape = json::escape_t::SOLIDUS;
	// Выполняем установку настроек записи текста
	writer.settings(settings);
	// Выполняем сброс состояния записи
	writer.reset();
	// Выполняем запись строки с косой чертой
	ASSERT_TRUE(writer.value(string("a/b")));
	// Выполняем проверку экранирования косой черты
	ASSERT_EQ(writer.text(), "\"a\\/b\"");
	// Устанавливаем запись знаков вне US-ASCII кодовыми значениями
	settings.escape = json::escape_t::ASCII;
	// Выполняем установку настроек записи текста
	writer.settings(settings);
	// Выполняем сброс состояния записи
	writer.reset();
	// Выполняем запись строки со знаками вне US-ASCII
	ASSERT_TRUE(writer.value(string("é€")));
	// Выполняем проверку записи знаков кодовыми значениями
	ASSERT_EQ(writer.text(), "\"\\u00e9\\u20ac\"");
	// Выполняем сброс состояния записи
	writer.reset();
	/**
	 * Выполняем запись знака за пределами одного слова
	 */
	ASSERT_TRUE(writer.value(string("😀")));
	// Выполняем проверку записи знака суррогатной парой
	ASSERT_EQ(writer.text(), "\"\\ud83d\\ude00\"");
}
/**
 * @brief Проверка записи чисел с плавающей запятой
 *
 * @details Число записывается кратчайшей записью, читающейся обратно тем же числом:
 * запись наибольшей точностью оборот переживает тоже, но выдаёт «0.1» как
 * «0.10000000000000001»
 *
 */
TEST(CodecJsonWriter, Reals) {
	/**
	 * Записываемые числа с плавающей запятой
	 */
	const vector <double> values = {
		0., -0.5, 0.1, 1., 3.14159, 1e10, 1e-10, 1e308, -1.5e-30,
		numeric_limits <double>::min(), numeric_limits <double>::max()
	};
	/**
	 * Выполняем перебор всех записываемых чисел с плавающей запятой
	 */
	for(const double value : values){
		// Объект записи текста документа
		json::writer_t writer;
		// Выполняем запись очередного числа с плавающей запятой
		ASSERT_TRUE(writer.value(value));
		// Выполняем проверку соответствия записи числа стандарту
		ASSERT_TRUE(json::numeric(writer.text())) << "запись «" << writer.text() << "»";
		// Объект документа для обратного чтения записанного числа
		json::document_t doc;
		// Выполняем разбор записанного числа
		ASSERT_TRUE(doc.parse(writer.text()));
		// Прочитанное обратно значение записанного числа
		double back = 0.;
		// Выполняем извлечение прочитанного обратно значения
		ASSERT_TRUE(doc.root().value(back));
		// Выполняем проверку совпадения прочитанного обратно значения с записанным
		ASSERT_EQ(back, value) << "запись «" << writer.text() << "»";
	}
	// Объект записи текста документа
	json::writer_t writer;
	// Выполняем запись числа, у какого сокращение записи заметно на глаз
	ASSERT_TRUE(writer.value(0.1));
	// Выполняем проверку кратчайшей записи числа
	ASSERT_EQ(writer.text(), "0.1");
}
/**
 * @brief Проверка записи чисел, стандарту не отвечающих
 *
 */
TEST(CodecJsonWriter, Specials) {
	// Объект записи текста документа
	json::writer_t writer;
	// Выполняем проверку отклонения числа, обычным числом не являющегося
	ASSERT_FALSE(writer.value(numeric_limits <double>::quiet_NaN()));
	// Выполняем проверку отклонения бесконечности
	ASSERT_FALSE(writer.value(numeric_limits <double>::infinity()));
	// Получаем настройки записи текста документа
	json::writer_t::settings_t settings = writer.settings();
	// Разрешаем запись NaN и бесконечности словами
	settings.allowInfinityAndNan = true;
	// Выполняем установку настроек записи текста
	writer.settings(settings);
	// Выполняем открытие массива
	ASSERT_TRUE(writer.array());
	// Выполняем запись числа, обычным числом не являющегося
	ASSERT_TRUE(writer.value(numeric_limits <double>::quiet_NaN()));
	// Выполняем запись бесконечности
	ASSERT_TRUE(writer.value(numeric_limits <double>::infinity()));
	// Выполняем запись отрицательной бесконечности
	ASSERT_TRUE(writer.value(-numeric_limits <double>::infinity()));
	// Выполняем закрытие массива
	ASSERT_TRUE(writer.close());
	// Выполняем проверку собранного текста
	ASSERT_EQ(writer.text(), "[NaN,Infinity,-Infinity]");
}
/**
 * @brief Проверка записи числа его готовой записью
 *
 */
TEST(CodecJsonWriter, Raw) {
	// Объект записи текста документа
	json::writer_t writer;
	/**
	 * Выполняем запись числа, не представимого видом с плавающей запятой
	 */
	ASSERT_TRUE(writer.raw("9007199254740993"));
	// Выполняем проверку сохранности точности записи числа
	ASSERT_EQ(writer.text(), "9007199254740993");
	// Выполняем сброс состояния записи
	writer.reset();
	// Выполняем проверку отклонения записи с ведущим нулём
	ASSERT_FALSE(writer.raw("01"));
	// Выполняем проверку отклонения записи, числом не являющейся
	ASSERT_FALSE(writer.raw("abc"));
	// Выполняем проверку отклонения пустой записи
	ASSERT_FALSE(writer.raw(""));
	// Выполняем проверку отклонения записи словами без дозволения настроек
	ASSERT_FALSE(writer.raw("NaN"));
	// Выполняем проверку того, что отклонённые записи текста не оставили
	ASSERT_TRUE(writer.text().empty());
}
/**
 * @brief Проверка потокового изъятия собранного текста
 *
 */
TEST(CodecJsonWriter, Take) {
	// Объект записи текста документа
	json::writer_t writer;
	// Выполняем открытие массива
	ASSERT_TRUE(writer.array());
	// Выполняем запись первого значения массива
	ASSERT_TRUE(writer.value(static_cast <int64_t> (1)));
	// Выполняем изъятие собранного текста
	ASSERT_EQ(writer.take(), "[1");
	// Выполняем проверку опустошения сборщика после изъятия
	ASSERT_EQ(writer.size(), 0u);
	// Выполняем проверку сохранности глубины вложенности после изъятия
	ASSERT_EQ(writer.depth(), 1u);
	// Выполняем запись второго значения массива
	ASSERT_TRUE(writer.value(static_cast <int64_t> (2)));
	// Выполняем закрытие массива
	ASSERT_TRUE(writer.close());
	// Выполняем проверку продолжения записи с изъятого места
	ASSERT_EQ(writer.text(), ",2]");
}
/**
 * @brief Проверка записи потока документов NDJSON
 *
 */
TEST(CodecJsonWriter, Stream) {
	// Объект записи текста документа
	json::writer_t writer;
	// Получаем настройки записи текста документа
	json::writer_t::settings_t settings = writer.settings();
	// Разрешаем разделение документов переводом строки
	settings.stream = true;
	// Выполняем установку настроек записи текста
	writer.settings(settings);
	// Выполняем проверку отклонения завершения пустого документа
	ASSERT_FALSE(writer.finish());
	// Выполняем запись первого документа
	ASSERT_TRUE(writer.object());
	// Выполняем закрытие первого документа
	ASSERT_TRUE(writer.close());
	// Выполняем завершение первого документа
	ASSERT_TRUE(writer.finish());
	// Выполняем запись второго документа
	ASSERT_TRUE(writer.array());
	// Выполняем закрытие второго документа
	ASSERT_TRUE(writer.close());
	// Выполняем завершение второго документа
	ASSERT_TRUE(writer.finish());
	// Выполняем проверку собранного текста
	ASSERT_EQ(writer.text(), "{}\n[]\n");
	// Выполняем открытие массива
	ASSERT_TRUE(writer.array());
	// Выполняем проверку отклонения завершения документа посреди вместилища
	ASSERT_FALSE(writer.finish());
}
/**
 * @brief Проверка кругового прохода записанного через разбор
 *
 * @details Собранный текст обязан разбираться обратно тем же самым документом при
 * всяком оформлении: оформление меняет знаки между значениями, но не значения
 *
 */
TEST(CodecJsonWriter, RoundTrip) {
	// Объект записи текста документа
	json::writer_t writer;
	// Выполняем запись объекта со значениями всех видов
	writer.object();
	// Выполняем запись имени поля со строковым значением
	writer.key("строка");
	// Выполняем запись строкового значения со знаками, требующими экранирования
	writer.value(string("а\"б\\в\nг/д 😀"));
	// Выполняем запись имени поля с пустым значением
	writer.key("пусто");
	// Выполняем запись пустого значения
	writer.null();
	// Выполняем запись имени поля с логическим значением
	writer.key("да");
	// Выполняем запись логического значения
	writer.value(true);
	// Выполняем запись имени поля с числом
	writer.key("число");
	// Выполняем запись числа его готовой записью
	writer.raw("9007199254740993");
	// Выполняем запись имени поля с массивом
	writer.key("массив");
	// Выполняем открытие массива
	writer.array();
	// Выполняем запись первого значения массива
	writer.value(static_cast <int64_t> (1));
	// Выполняем запись второго значения массива
	writer.value(-1.5);
	// Выполняем открытие вложенного объекта
	writer.object();
	// Выполняем закрытие вложенного объекта
	writer.close();
	// Выполняем закрытие массива
	writer.close();
	// Выполняем закрытие объекта
	writer.close();
	// Получаем собранный текст документа
	const string compact = writer.text();
	// Объект документа для разбора собранного текста
	json::document_t doc;
	// Выполняем разбор собранного текста
	ASSERT_TRUE(doc.parse(compact)) << json::message(doc.error());
	// Выполняем проверку совпадения перезаписанного текста с собранным
	ASSERT_EQ(doc.dump(), compact);
	// Объект документа для разбора текста с отступами
	json::document_t pretty;
	// Выполняем разбор текста, оформленного отступами
	ASSERT_TRUE(pretty.parse(doc.dump(json::format_t::PRETTY))) << json::message(pretty.error());
	// Выполняем проверку совпадения документов при разном оформлении текста
	ASSERT_EQ(pretty.dump(), compact);
}
