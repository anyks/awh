/**
 * @file: document.cpp
 * @date: 2026-08-12
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Проверки дерева настроек TOML — сборка дерева разбором, чтение значений по
 *        составному имени, правка записей на месте, удаление пар и таблиц, а также
 *        договор о сохранении оформления при обратной записи
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <gtest/gtest.h>
#include <codec/toml/toml.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;
using namespace awh::codec;

/**
 * @brief Разбираемый текст настроек проверок дерева
 *
 * @details Текст записан тем же оформлением, каким его собирает запись: договор о
 * сохранении оформления меряется побайтовым совпадением, и расхождение украшений
 * пришлось бы принимать за потерю
 *
 */
static const char * SAMPLE =
 "# настройки сервера\n"
 "title = \"пример\"\n"
 "\n"
 "[server]\n"
 "host = 'локальный' # хозяин\n"
 "port = 8080\n"
 "ratio = 0.25\n"
 "flags = [true, false]\n"
 "point = { x = 1, y = 2 }\n"
 "\n"
 "[server.limits]\n"
 "depth = 0x10\n"
 "\n"
 "[[products]]\n"
 "name = \"гвоздь\"\n"
 "\n"
 "[[products]]\n"
 "name = \"молоток\"\n";

/**
 * @brief Проверка чтения значений по составному имени
 *
 */
TEST(CodecTomlDocument, Reading) {
	// Объект дерева настроек
	toml::document_t document;
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(SAMPLE)) << static_cast <uint32_t> (document.error());
	// Выполняем проверку чтения значения верхнего уровня
	ASSERT_EQ(document.text({"title"}), "пример");
	// Выполняем проверку чтения строкового значения таблицы
	ASSERT_EQ(document.text({"server", "host"}), "локальный");
	// Целое число значения пары
	int64_t port = 0;
	// Выполняем проверку чтения целого числа
	ASSERT_TRUE(document.value(port, {"server", "port"}));
	// Выполняем проверку прочитанного целого числа
	ASSERT_EQ(port, 8080);
	// Число с плавающей точкой значения пары
	double ratio = 0.0;
	// Выполняем проверку чтения числа с плавающей точкой
	ASSERT_TRUE(document.value(ratio, {"server", "ratio"}));
	// Выполняем проверку прочитанного числа с плавающей точкой
	ASSERT_EQ(ratio, 0.25);
	// Целое число значения вложенной таблицы
	int64_t depth = 0;
	// Выполняем проверку чтения значения вложенной таблицы
	ASSERT_TRUE(document.value(depth, {"server", "limits", "depth"}));
	// Выполняем проверку прочитанного значения вложенной таблицы
	ASSERT_EQ(depth, 16);
	// Выполняем проверку отсутствия незаписанной пары
	ASSERT_FALSE(document.has({"server", "missing"}));
	// Выполняем проверку типа значения пары
	ASSERT_EQ(document.type({"server", "flags"}), toml::type_t::ARRAY);
}
/**
 * @brief Проверка соблюдения типа значения при чтении числом
 *
 * @details Тип значения задан описанием, и выдавать целое число дробным - равно как
 * и наоборот - значило бы читать запись неверно
 *
 */
TEST(CodecTomlDocument, Typed) {
	// Объект дерева настроек
	toml::document_t document;
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(SAMPLE));
	// Целое число значения пары
	int64_t integer = 0;
	// Выполняем проверку отказа чтения числа с плавающей точкой целым
	ASSERT_FALSE(document.value(integer, {"server", "ratio"}));
	// Логическое значение пары
	bool boolean = false;
	// Выполняем проверку отказа чтения целого числа логическим значением
	ASSERT_FALSE(document.value(boolean, {"server", "port"}));
	// Целое число значения пары, отрезком типа не вмещаемое
	int8_t narrow = 0;
	// Выполняем проверку отказа чтения числа сверх отрезка значений типа
	ASSERT_FALSE(document.value(narrow, {"server", "port"}));
	// Число с плавающей точкой значения пары
	double real = 0.0;
	/**
	 * Выполняем проверку чтения целого числа с плавающей точкой
	 *
	 * @note Приведение это значения не искажает, и оно дозволено
	 */
	ASSERT_TRUE(document.value(real, {"server", "port"}));
	// Выполняем проверку прочитанного числа
	ASSERT_EQ(real, 8080.0);
}
/**
 * @brief Проверка чтения перечня значений и встроенной таблицы
 *
 */
TEST(CodecTomlDocument, Composite) {
	// Объект дерева настроек
	toml::document_t document;
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(SAMPLE));
	// Выполняем проверку количества значений перечня
	ASSERT_EQ(document.length({"server", "flags"}), 2u);
	// Прочитанное значение перечня
	toml::value_t value;
	// Выполняем проверку чтения первого значения перечня
	ASSERT_TRUE(document.item({"server", "flags"}, 0, value));
	// Выполняем проверку типа прочитанного значения
	ASSERT_EQ(value.type, toml::type_t::BOOLEAN);
	// Выполняем проверку прочитанного логического значения
	ASSERT_TRUE(value.boolean);
	// Выполняем проверку отказа чтения значения перечня за его пределами
	ASSERT_FALSE(document.item({"server", "flags"}, 2, value));
	// Выполняем проверку типа значения встроенной таблицы
	ASSERT_EQ(document.type({"server", "point"}), toml::type_t::TABLE);
}
/**
 * @brief Проверка перечней таблиц и дочерних имён
 *
 */
TEST(CodecTomlDocument, Structure) {
	// Объект дерева настроек
	toml::document_t document;
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(SAMPLE));
	// Получаем перечень объявленных таблиц
	const vector <vector <string_view>> tables = document.tables();
	// Выполняем проверку количества объявленных таблиц
	ASSERT_EQ(tables.size(), 4u);
	// Выполняем проверку количества объявленных таблиц
	ASSERT_EQ(document.size(), 4u);
	// Выполняем проверку имени первой объявленной таблицы
	ASSERT_EQ(tables.at(0).size(), 1u);
	// Выполняем проверку имени первой объявленной таблицы
	ASSERT_EQ(tables.at(0).at(0), "server");
	// Выполняем проверку составного имени вложенной таблицы
	ASSERT_EQ(tables.at(1).size(), 2u);
	// Выполняем проверку наличия объявленной таблицы
	ASSERT_TRUE(document.table({"server", "limits"}));
	// Выполняем проверку отсутствия необъявленной таблицы
	ASSERT_FALSE(document.table({"server", "missing"}));
	// Получаем перечень дочерних имён таблицы
	const vector <string_view> keys = document.keys({"server"});
	/**
	 * Выполняем проверку количества дочерних имён таблицы
	 *
	 * @note Вложенная таблица дочерним именем своей объемлющей и является: имя
	 *       «limits» идёт наравне с именами пар
	 */
	ASSERT_EQ(keys.size(), 6u);
	// Выполняем проверку первого дочернего имени таблицы
	ASSERT_EQ(keys.at(0), "host");
	// Выполняем проверку последнего дочернего имени пар таблицы
	ASSERT_EQ(keys.at(4), "point");
	// Выполняем проверку дочернего имени вложенной таблицы
	ASSERT_EQ(keys.at(5), "limits");
	// Получаем перечень дочерних имён верхнего уровня
	const vector <string_view> roots = document.keys();
	// Выполняем проверку количества дочерних имён верхнего уровня
	ASSERT_EQ(roots.size(), 3u);
	// Выполняем проверку первого дочернего имени верхнего уровня
	ASSERT_EQ(roots.at(0), "title");
}
/**
 * @brief Проверка обращения к таблицам набора таблиц
 *
 * @details Имя у таблиц набора общее, и различить их можно лишь порядковым номером
 * частью составного имени
 *
 */
TEST(CodecTomlDocument, ArrayTables) {
	// Объект дерева настроек
	toml::document_t document;
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(SAMPLE));
	// Выполняем проверку количества таблиц набора таблиц
	ASSERT_EQ(document.count({"products"}), 2u);
	// Выполняем проверку чтения значения первой таблицы набора
	ASSERT_EQ(document.text({"products", "0", "name"}), "гвоздь");
	// Выполняем проверку чтения значения второй таблицы набора
	ASSERT_EQ(document.text({"products", "1", "name"}), "молоток");
	// Выполняем проверку наличия таблицы набора таблиц
	ASSERT_TRUE(document.table({"products", "1"}));
	// Выполняем проверку отсутствия таблицы набора за его пределами
	ASSERT_FALSE(document.table({"products", "2"}));
	// Выполняем проверку нулевого количества таблиц у набора, не объявленного вовсе
	ASSERT_EQ(document.count({"missing"}), 0u);
}
/**
 * @brief Проверка обратной записи дерева настроек
 *
 * @details Договор о сохранении оформления: перезапись обязана возвращать файл,
 * узнаваемый его хозяином, - с примечаниями, пустыми строками, порядком записей,
 * оградой строк и системой счисления чисел, какими их выбрал человек
 *
 */
TEST(CodecTomlDocument, Rewrite) {
	// Объект дерева настроек
	toml::document_t document;
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(SAMPLE));
	// Выполняем проверку побайтового совпадения перезаписи с исходным текстом
	ASSERT_EQ(document.text(), SAMPLE);
}
/**
 * @brief Проверка сохранения многострочной записи перечня
 *
 */
TEST(CodecTomlDocument, MultilineArray) {
	// Разбираемый текст настроек
	const string text =
	 "hosts = [\n"
	 "\t\"первый\",\n"
	 "\t\"второй\"\n"
	 "]\n"
	 "ports = [80, 443]\n";
	// Объект дерева настроек
	toml::document_t document;
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(text)) << static_cast <uint32_t> (document.error());
	/**
	 * Выполняем проверку сохранения расстановки строк перечня
	 *
	 * @note Расстановка эта выбрана человеком, и разбор о ней не сообщает: узнаётся
	 *       она по местам событий начала и конца перечня
	 */
	ASSERT_EQ(document.text(), text);
}
/**
 * @brief Проверка правки значения объявленной пары
 *
 * @details Значение заменяется в собственной записи пары, отчего ни порядок, ни
 * примечания, ни пустые строки не страдают
 *
 */
TEST(CodecTomlDocument, Editing) {
	// Объект дерева настроек
	toml::document_t document;
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(SAMPLE));
	// Выполняем установку значения объявленной пары
	ASSERT_TRUE(document.set({"server", "port"}, static_cast <int64_t> (9090)));
	// Целое число значения пары
	int64_t port = 0;
	// Выполняем проверку чтения установленного значения
	ASSERT_TRUE(document.value(port, {"server", "port"}));
	// Выполняем проверку прочитанного значения
	ASSERT_EQ(port, 9090);
	// Получаем собранный текст настроек
	const string result = document.text();
	// Выполняем проверку записи установленного значения
	ASSERT_NE(result.find("port = 9090\n"), string::npos);
	// Выполняем проверку сохранения примечания в конце строки
	ASSERT_NE(result.find("host = 'локальный' # хозяин\n"), string::npos);
	// Получаем исходный текст настроек
	const string source(SAMPLE);
	// Выполняем проверку сохранения количества строк текста настроек
	ASSERT_EQ(count(result.begin(), result.end(), '\n'), count(source.begin(), source.end(), '\n'));
}
/**
 * @brief Проверка заведения отсутствующей пары
 *
 * @details Отсутствующая пара дописывается в конец своей таблицы, а не в конец
 * текста: место её определяется именем, а не порядком правок
 *
 */
TEST(CodecTomlDocument, Appending) {
	// Объект дерева настроек
	toml::document_t document;
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(SAMPLE));
	// Выполняем заведение отсутствующей пары объявленной таблицы
	ASSERT_TRUE(document.set({"server", "backlog"}, static_cast <int64_t> (128)));
	// Выполняем заведение отсутствующей пары верхнего уровня
	ASSERT_TRUE(document.set({"version"}, "1.0"));
	// Получаем собранный текст настроек
	const string result = document.text();
	// Выполняем проверку записи заведённой пары таблицы
	ASSERT_NE(result.find("point = { x = 1, y = 2 }\nbacklog = 128\n"), string::npos);
	/**
	 * Выполняем проверку записи заведённой пары верхнего уровня
	 *
	 * @note Пара верхнего уровня записана до первого объявления таблицы: за ним она
	 *       принадлежала бы уже той таблице
	 */
	ASSERT_NE(result.find("version = \"1.0\"\n"), string::npos);
	// Выполняем проверку того, что пара верхнего уровня записана до объявления таблицы
	ASSERT_LT(result.find("version = "), result.find("[server]"));
	// Выполняем проверку чтения заведённой пары верхнего уровня
	ASSERT_EQ(document.text({"version"}), "1.0");
}
/**
 * @brief Проверка заведения пары необъявленной таблицы
 *
 */
TEST(CodecTomlDocument, Creating) {
	// Объект дерева настроек
	toml::document_t document;
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(SAMPLE));
	// Выполняем объявление отсутствующей таблицы
	ASSERT_TRUE(document.create({"client"}));
	// Выполняем повторное объявление таблицы
	ASSERT_TRUE(document.create({"client"}));
	// Выполняем проверку того, что таблица объявлена однажды
	ASSERT_EQ(document.size(), 5u);
	// Выполняем заведение пары объявленной таблицы
	ASSERT_TRUE(document.set({"client", "retries"}, static_cast <int64_t> (3)));
	// Получаем собранный текст настроек
	const string result = document.text();
	// Выполняем проверку записи объявленной таблицы с её парой
	ASSERT_NE(result.find("[client]\nretries = 3\n"), string::npos);
	// Выполняем проверку разбора собранного текста настроек
	toml::document_t reread;
	// Выполняем разбор собранного текста настроек
	ASSERT_TRUE(reread.parse(result)) << static_cast <uint32_t> (reread.error());
	// Выполняем проверку чтения заведённого значения
	ASSERT_TRUE(reread.has({"client", "retries"}));
}
/**
 * @brief Проверка удаления пары
 *
 */
TEST(CodecTomlDocument, Erasing) {
	// Объект дерева настроек
	toml::document_t document;
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(SAMPLE));
	// Выполняем удаление объявленной пары
	ASSERT_TRUE(document.erase({"server", "host"}));
	// Выполняем проверку отсутствия удалённой пары
	ASSERT_FALSE(document.has({"server", "host"}));
	// Выполняем проверку отказа повторного удаления пары
	ASSERT_FALSE(document.erase({"server", "host"}));
	// Получаем собранный текст настроек
	const string result = document.text();
	// Выполняем проверку отсутствия удалённой пары в собранном тексте
	ASSERT_EQ(result.find("host = "), string::npos);
	/**
	 * Выполняем проверку удаления примечания в конце строки
	 *
	 * @note Примечание это писано к удалённой паре: оставить его значило бы оставить
	 *       пояснение к тому, чего в файле более нет
	 */
	ASSERT_EQ(result.find("хозяин"), string::npos);
	// Выполняем проверку сохранения примечания отдельной строкой
	ASSERT_NE(result.find("# настройки сервера"), string::npos);
	// Выполняем проверку сохранения прочих пар таблицы
	ASSERT_NE(result.find("port = 8080"), string::npos);
}
/**
 * @brief Проверка удаления таблицы
 *
 */
TEST(CodecTomlDocument, Removing) {
	// Объект дерева настроек
	toml::document_t document;
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(SAMPLE));
	// Выполняем удаление объявленной таблицы
	ASSERT_TRUE(document.remove({"server"}));
	// Выполняем проверку отсутствия удалённой таблицы
	ASSERT_FALSE(document.table({"server"}));
	// Выполняем проверку отсутствия пар удалённой таблицы
	ASSERT_FALSE(document.has({"server", "port"}));
	/**
	 * Выполняем проверку сохранения вложенной таблицы
	 *
	 * @note Вложенная таблица объявлена своей записью, и удаление объемлющей её не
	 *       касается: удаляются записи до следующего объявления
	 */
	ASSERT_TRUE(document.table({"server", "limits"}));
	// Получаем собранный текст настроек
	const string result = document.text();
	// Выполняем проверку отсутствия удалённой таблицы в собранном тексте
	ASSERT_EQ(result.find("[server]"), string::npos);
	// Выполняем проверку сохранения вложенной таблицы в собранном тексте
	ASSERT_NE(result.find("[server.limits]"), string::npos);
	// Выполняем проверку отказа удаления необъявленной таблицы
	ASSERT_FALSE(document.remove({"missing"}));
	// Выполняем проверку кода ошибки правки дерева
	ASSERT_EQ(document.error(), toml::error_t::INVALID_TABLE);
}
/**
 * @brief Проверка сохранения выбранной человеком записи значений
 *
 */
TEST(CodecTomlDocument, Preserving) {
	// Объект дерева настроек
	toml::document_t document;
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(SAMPLE));
	// Прочитанное значение пары
	toml::value_t value;
	// Выполняем чтение значения пары
	ASSERT_TRUE(document.get({"server", "host"}, value));
	// Выполняем проверку сохранения ограды строкового значения
	ASSERT_EQ(value.quoting, toml::string_t::LITERAL);
	// Выполняем чтение значения пары вложенной таблицы
	ASSERT_TRUE(document.get({"server", "limits", "depth"}, value));
	// Выполняем проверку сохранения системы счисления записи числа
	ASSERT_EQ(value.radix, toml::radix_t::HEX);
	// Выполняем проверку записи числа выбранной человеком системой счисления
	ASSERT_NE(document.text().find("depth = 0x10\n"), string::npos);
}
/**
 * @brief Проверка установки значений всех простых типов
 *
 */
TEST(CodecTomlDocument, Values) {
	// Объект дерева настроек
	toml::document_t document;
	// Выполняем установку строкового значения пары
	ASSERT_TRUE(document.set({"text"}, "значение"));
	// Выполняем установку логического значения пары
	ASSERT_TRUE(document.set({"flag"}, true));
	// Выполняем установку целого числа значением пары
	ASSERT_TRUE(document.set({"count"}, static_cast <int64_t> (7), toml::radix_t::HEX));
	// Выполняем установку числа с плавающей точкой значением пары
	ASSERT_TRUE(document.set({"ratio"}, 1.0));
	// Устанавливаемая отметка времени
	toml::stamp_t stamp;
	// Устанавливаем год отметки времени
	stamp.date.year = 2026;
	// Устанавливаем месяц отметки времени
	stamp.date.month = 8;
	// Устанавливаем день отметки времени
	stamp.date.day = 12;
	// Выполняем установку отметки времени значением пары
	ASSERT_TRUE(document.set({"date"}, stamp, toml::type_t::LOCAL_DATE));
	/**
	 * Выполняем проверку собранного текста настроек
	 *
	 * @note Строковое значение, записанное прямо в месте вызова, установлено
	 *       строкою, а не логическим значением
	 */
	ASSERT_EQ(document.text(),
	 "text = \"значение\"\n"
	 "flag = true\n"
	 "count = 0x7\n"
	 "ratio = 1.0\n"
	 "date = 2026-08-12\n");
	// Выполняем проверку отказа установки отрицательного числа системой с приставкой
	ASSERT_FALSE(document.set({"wrong"}, static_cast <int64_t> (-1), toml::radix_t::HEX));
	// Выполняем проверку кода ошибки правки дерева
	ASSERT_EQ(document.error(), toml::error_t::INVALID_NUMBER);
}
/**
 * @brief Проверка замены составного значения простым
 *
 * @details Прежнее значение вправе нести перечень со вложенными узлами, и правка его
 * на месте оставила бы их обрывками
 *
 */
TEST(CodecTomlDocument, Replacing) {
	// Объект дерева настроек
	toml::document_t document;
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(SAMPLE));
	// Выполняем замену перечня значений простым значением
	ASSERT_TRUE(document.set({"server", "flags"}, static_cast <int64_t> (1)));
	// Выполняем проверку смены типа значения пары
	ASSERT_EQ(document.type({"server", "flags"}), toml::type_t::INTEGER);
	// Выполняем проверку нулевой длины перечня, перечнем более не являющегося
	ASSERT_EQ(document.length({"server", "flags"}), 0u);
	// Получаем собранный текст настроек
	const string result = document.text();
	// Выполняем проверку записи установленного значения
	ASSERT_NE(result.find("flags = 1\n"), string::npos);
	// Объект дерева настроек для разбора собранного текста
	toml::document_t reread;
	// Выполняем проверку разбора собранного текста настроек
	ASSERT_TRUE(reread.parse(result)) << static_cast <uint32_t> (reread.error());
}
/**
 * @brief Проверка отказа разбора ошибочно построенного текста
 *
 */
TEST(CodecTomlDocument, Failure) {
	// Объект дерева настроек
	toml::document_t document;
	// Выполняем проверку отказа разбора ошибочно построенного текста
	ASSERT_FALSE(document.parse("[server]\nport = \n"));
	// Выполняем проверку кода ошибки разбора текста настроек
	ASSERT_EQ(document.error(), toml::error_t::MISSING_VALUE);
	// Выполняем проверку номера строки обнаружения ошибки
	ASSERT_EQ(document.errorLocation().line, 2u);
	// Выполняем проверку того, что дерево настроек освобождено
	ASSERT_TRUE(document.empty());
	// Выполняем проверку того, что собранный текст настроек пуст
	ASSERT_TRUE(document.text().empty());
}
/**
 * @brief Проверка отказа правки непригодным составным именем
 *
 */
TEST(CodecTomlDocument, Rejection) {
	// Объект дерева настроек
	toml::document_t document;
	// Выполняем проверку отказа установки значения пустым составным именем
	ASSERT_FALSE(document.set({}, static_cast <int64_t> (1)));
	// Выполняем проверку кода ошибки правки дерева
	ASSERT_EQ(document.error(), toml::error_t::EMPTY_KEY);
	// Выполняем проверку отказа установки значения именем с управляющим знаком
	ASSERT_FALSE(document.set({string_view("имя\nключа")}, static_cast <int64_t> (1)));
	// Выполняем проверку кода ошибки правки дерева
	ASSERT_EQ(document.error(), toml::error_t::INVALID_KEY);
	// Выполняем проверку того, что дерево настроек осталось нетронутым
	ASSERT_TRUE(document.empty());
}
/**
 * @brief Проверка правки составным именем ключа
 *
 */
TEST(CodecTomlDocument, DottedKeys) {
	// Разбираемый текст настроек
	const string text = "[server]\nlimits.depth = 1\n";
	// Объект дерева настроек
	toml::document_t document;
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(text)) << static_cast <uint32_t> (document.error());
	// Целое число значения пары
	int64_t depth = 0;
	// Выполняем проверку чтения значения составным именем ключа
	ASSERT_TRUE(document.value(depth, {"server", "limits", "depth"}));
	// Выполняем проверку прочитанного значения
	ASSERT_EQ(depth, 1);
	// Получаем перечень дочерних имён таблицы
	const vector <string_view> keys = document.keys({"server"});
	// Выполняем проверку количества дочерних имён таблицы
	ASSERT_EQ(keys.size(), 1u);
	// Выполняем проверку дочернего имени таблицы
	ASSERT_EQ(keys.at(0), "limits");
	// Выполняем проверку побайтового совпадения перезаписи с исходным текстом
	ASSERT_EQ(document.text(), text);
	// Выполняем правку значения составным именем ключа
	ASSERT_TRUE(document.set({"server", "limits", "depth"}, static_cast <int64_t> (2)));
	// Выполняем проверку записи установленного значения
	ASSERT_EQ(document.text(), "[server]\nlimits.depth = 2\n");
}
/**
 * @brief Проверка обращения с пустым именем ключа
 *
 * @details Описание дозволяет имя ключа пустым, и указатель дерева обязан отличать
 * его от верхнего уровня текста настроек: иначе обход дерева по дочерним именам
 * возвращался бы к его началу и не кончался вовсе
 *
 */
TEST(CodecTomlDocument, EmptyKey) {
	// Разбираемый текст настроек
	const string text = "\"\" = 1\n[table]\n\"\" = 2\n";
	// Объект дерева настроек
	toml::document_t document;
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(text)) << static_cast <uint32_t> (document.error());
	// Целое число значения пары с пустым именем ключа
	int64_t value = 0;
	// Выполняем проверку чтения значения пары с пустым именем ключа
	ASSERT_TRUE(document.value(value, {""}));
	// Выполняем проверку прочитанного значения
	ASSERT_EQ(value, 1);
	// Выполняем проверку чтения значения пары таблицы с пустым именем ключа
	ASSERT_TRUE(document.value(value, {"table", ""}));
	// Выполняем проверку прочитанного значения
	ASSERT_EQ(value, 2);
	// Получаем перечень дочерних имён верхнего уровня
	const vector <string_view> roots = document.keys();
	// Выполняем проверку количества дочерних имён верхнего уровня
	ASSERT_EQ(roots.size(), 2u);
	// Выполняем проверку пустого дочернего имени верхнего уровня
	ASSERT_TRUE(roots.at(0).empty());
	/**
	 * Получаем перечень дочерних имён пары с пустым именем ключа
	 *
	 * @note Верхний уровень выдал бы здесь собственные дочерние имена, и обход
	 *       дерева пошёл бы по кругу
	 */
	const vector <string_view> keys = document.keys({""});
	// Выполняем проверку отсутствия дочерних имён у пары с пустым именем ключа
	ASSERT_TRUE(keys.empty());
	// Выполняем проверку побайтового совпадения перезаписи с исходным текстом
	ASSERT_EQ(document.text(), text);
}
/**
 * @brief Проверка освобождения дерева настроек
 *
 */
TEST(CodecTomlDocument, Clearing) {
	// Объект дерева настроек
	toml::document_t document;
	// Выполняем проверку того, что дерево настроек пусто
	ASSERT_TRUE(document.empty());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(SAMPLE));
	// Выполняем проверку того, что дерево настроек не пусто
	ASSERT_FALSE(document.empty());
	// Выполняем освобождение дерева настроек
	document.clear();
	// Выполняем проверку того, что дерево настроек освобождено
	ASSERT_TRUE(document.empty());
	// Выполняем проверку того, что таблиц в дереве не осталось
	ASSERT_EQ(document.size(), 0u);
	// Выполняем проверку того, что собранный текст настроек пуст
	ASSERT_TRUE(document.text().empty());
}
/**
 * @brief Проверка отказа правки поверх занятого имени
 *
 * @details Объявить таблицу поверх пары либо завести пару поверх таблицы значило бы
 * собрать текст, который разбор отвергнет переопределением: отвергается это в месте
 * правки, а не при записи собранного дерева
 *
 */
TEST(CodecTomlDocument, Redefine) {
	// Объект дерева настроек
	toml::document_t document;
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(SAMPLE));
	// Выполняем проверку отказа объявления таблицы поверх объявленной пары
	ASSERT_FALSE(document.create({"title"}));
	// Выполняем проверку кода ошибки правки дерева
	ASSERT_EQ(document.error(), toml::error_t::REDEFINE_TABLE);
	// Выполняем проверку отказа заведения пары поверх объявленной таблицы
	ASSERT_FALSE(document.set({"server"}, static_cast <int64_t> (1)));
	// Выполняем проверку кода ошибки правки дерева
	ASSERT_EQ(document.error(), toml::error_t::REDEFINE_TABLE);
	// Выполняем проверку отказа заведения пары поверх набора таблиц
	ASSERT_FALSE(document.set({"products"}, static_cast <int64_t> (1)));
	// Выполняем проверку кода ошибки правки дерева
	ASSERT_EQ(document.error(), toml::error_t::REDEFINE_TABLE);
	/**
	 * Выполняем проверку отказа заведения пары под набором таблиц без номера
	 *
	 * @note Набор таблиц адресуется порядковым номером, и всякое иное имя за ним
	 *       завело бы пару поверх набора
	 */
	ASSERT_FALSE(document.set({"products", "name"}, "гвоздь"));
	// Выполняем проверку кода ошибки правки дерева
	ASSERT_EQ(document.error(), toml::error_t::REDEFINE_TABLE);
	// Выполняем проверку отказа заведения пары под таблицей набора за его пределами
	ASSERT_FALSE(document.set({"products", "2", "name"}, "молоток"));
	// Выполняем проверку кода ошибки правки дерева
	ASSERT_EQ(document.error(), toml::error_t::REDEFINE_TABLE);
	// Выполняем проверку правки пары объявленной таблицы набора
	ASSERT_TRUE(document.set({"products", "1", "name"}, "клещи"));
	// Выполняем проверку чтения установленного значения
	ASSERT_EQ(document.text({"products", "1", "name"}), "клещи");
	// Выполняем проверку побайтового совпадения перезаписи с ожидаемым текстом
	ASSERT_NE(document.text().find("name = \"клещи\"\n"), string::npos);
	// Объект дерева настроек для разбора собранного текста
	toml::document_t reread;
	// Выполняем проверку разбора собранного текста настроек
	ASSERT_TRUE(reread.parse(document.text())) << static_cast <uint32_t> (reread.error());
}
