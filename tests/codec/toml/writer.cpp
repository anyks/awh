/**
 * @file: writer.cpp
 * @date: 2026-08-12
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Проверки записи текста настроек TOML — построение имён ключей, ограждение
 *        строковых значений, запись чисел, отметок времени, перечней, встроенных таблиц,
 *        примечаний, а также договор о совпадении записанного с прочитанным обратно
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
 * @brief Метод сборки составного имени ключа
 *
 * @param names составные части собираемого имени ключа
 * @return      собранное составное имя ключа
 *
 */
static vector <toml::part_t> path(const vector <string_view> & names) noexcept {
	// Собираемое составное имя ключа
	vector <toml::part_t> result(names.size());
	/**
	 * Выполняем перебор всех составных частей имени ключа
	 */
	for(size_t i = 0; i < names.size(); i++)
		// Устанавливаем очередную составную часть имени ключа
		result.at(i).name = names.at(i);
	// Выводим собранное составное имя ключа
	return result;
}
/**
 * @brief Прочитанное значение текста настроек
 *
 * @details Содержимое строкового значения хранится своей копией намеренно: разбор
 * выдаёт его ссылкой на свою память, живущей лишь до следующего события
 *
 */
struct Scalar {
	// Тип прочитанного значения
	toml::type_t type;
	// Система счисления записи целого числа
	toml::radix_t radix;
	// Целое число со знаком
	int64_t integer;
	// Число с плавающей точкой
	double real;
	// Логическое значение
	bool boolean;
	// Отметка времени
	toml::stamp_t stamp;
	// Содержимое строкового значения
	string text;
	/**
	 * @brief Конструктор
	 *
	 */
	Scalar() noexcept :
	 type(toml::type_t::NONE), radix(toml::radix_t::DECIMAL),
	 integer(0), real(0.0), boolean(false) {}
};

/**
 * @brief Метод разбора собранного текста настроек
 *
 * @details Служит договору о совпадении записанного с прочитанным обратно: собранный
 * текст обязан разбираться без ошибок, а прочитанные значения - совпадать с
 * записанными
 *
 * @param text   разбираемый текст настроек
 * @param events собранные события разбора значений
 * @return       код ошибки разбора собранного текста
 *
 */
static toml::error_t reread(const string & text, vector <Scalar> & events) noexcept {
	// Объект потокового чтения текста настроек
	toml::reader_t reader;
	// Выполняем очистку собранных событий разбора
	events.clear();
	/**
	 * Если подача разбираемого текста не удалась
	 */
	if(!reader.feed(text.data(), text.size(), true))
		// Выводим код ошибки разбора собранного текста
		return reader.error();
	/**
	 * Выполняем перебор выданных разбором событий
	 */
	while(reader.next()){
		/**
		 * Если событием является значение
		 */
		if(reader.event() == toml::event_t::VALUE){
			// Собираемое прочитанное значение
			Scalar item;
			// Запоминаем тип прочитанного значения
			item.type = reader.value().type;
			// Запоминаем систему счисления записи целого числа
			item.radix = reader.value().radix;
			// Запоминаем целое число со знаком
			item.integer = reader.value().integer;
			// Запоминаем число с плавающей точкой
			item.real = reader.value().real;
			// Запоминаем логическое значение
			item.boolean = reader.value().boolean;
			// Запоминаем отметку времени
			item.stamp = reader.value().stamp;
			// Запоминаем содержимое строкового значения
			item.text.assign(reader.value().text);
			// Выполняем добавление значения к собранным событиям разбора
			events.push_back(item);
		}
	}
	// Выводим код ошибки разбора собранного текста
	return reader.error();
}

/**
 * @brief Проверка записи простого текста настроек
 *
 */
TEST(CodecTomlWriter, Simple) {
	// Объект записи текста настроек
	toml::writer_t writer;
	// Выполняем запись объявления таблицы
	ASSERT_TRUE(writer.table("server"));
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("host"));
	// Выполняем запись строкового значения пары
	ASSERT_TRUE(writer.text("127.0.0.1"));
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("port"));
	// Выполняем запись целого числа
	ASSERT_TRUE(writer.integer(8080));
	// Выполняем проверку собранного текста настроек
	ASSERT_EQ(writer.text(), "[server]\nhost = \"127.0.0.1\"\nport = 8080\n");
}
/**
 * @brief Проверка записи составного имени ключа
 *
 */
TEST(CodecTomlWriter, DottedKeys) {
	// Объект записи текста настроек
	toml::writer_t writer;
	// Выполняем запись объявления таблицы составным именем
	ASSERT_TRUE(writer.table(::path({"a", "b c"})));
	// Выполняем запись имени ключа пары составным именем
	ASSERT_TRUE(writer.key(::path({"x", "y"})));
	// Выполняем запись логического значения
	ASSERT_TRUE(writer.boolean(true));
	/**
	 * Выполняем проверку собранного текста настроек
	 *
	 * @note Имя со знаком пробела к записи без кавычек непригодно и ограждается
	 *       кавычками само по себе
	 */
	ASSERT_EQ(writer.text(), "[a.\"b c\"]\nx.y = true\n");
}
/**
 * @brief Проверка ограждения имени ключа со знаками Юникода
 *
 * @details Описание версии 1.0.0 отводит имени без кавычек лишь знаки US-ASCII, и
 * умолчанием имя со знаками Юникода ограждается кавычками
 *
 */
TEST(CodecTomlWriter, UnicodeKeys) {
	// Объект записи текста настроек
	toml::writer_t writer;
	// Выполняем запись имени ключа со знаками Юникода
	ASSERT_TRUE(writer.key("ключ"));
	// Выполняем запись целого числа
	ASSERT_TRUE(writer.integer(1));
	// Выполняем проверку ограждения имени ключа кавычками
	ASSERT_EQ(writer.text(), "\"ключ\" = 1\n");
	// Получаем настройки записи текста настроек
	toml::writer_t::settings_t settings;
	// Разрешаем читающему знаки Юникода в имени без кавычек
	settings.unicode = true;
	// Выполняем установку настроек записи текста настроек
	writer.settings(settings);
	// Выполняем сброс записи в исходное состояние
	writer.clear();
	// Выполняем запись имени ключа со знаками Юникода
	ASSERT_TRUE(writer.key("ключ"));
	// Выполняем запись целого числа
	ASSERT_TRUE(writer.integer(1));
	// Выполняем проверку записи имени ключа без кавычек
	ASSERT_EQ(writer.text(), "ключ = 1\n");
}
/**
 * @brief Проверка отказа записи имени ключа при запрете смены ограды
 *
 */
TEST(CodecTomlWriter, StrictNaming) {
	// Получаем настройки записи текста настроек
	toml::writer_t::settings_t settings;
	// Запрещаем смену ограды имени и значения
	settings.promote = false;
	// Объект записи текста настроек
	toml::writer_t writer(settings);
	// Собираемое составное имя ключа
	vector <toml::part_t> name(1);
	// Устанавливаем имя ключа, к записи без кавычек непригодное
	name.front().name = "a b";
	// Выполняем проверку отказа записи имени ключа
	ASSERT_FALSE(writer.key(name));
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(writer.error(), toml::error_t::INVALID_KEY);
}
/**
 * @brief Проверка ограждения строковых значений
 *
 */
TEST(CodecTomlWriter, Strings) {
	// Объект записи текста настроек
	toml::writer_t writer;
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("basic"));
	// Выполняем запись строкового значения со знаками, требующими ограждения
	ASSERT_TRUE(writer.text("путь\\к\"файлу\"\t\n\x01"));
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("literal"));
	// Выполняем запись строкового значения дословной оградой
	ASSERT_TRUE(writer.text("C:\\путь", toml::string_t::LITERAL));
	// Выполняем проверку собранного текста настроек
	ASSERT_EQ(writer.text(), "basic = \"путь\\\\к\\\"файлу\\\"\\t\\n\\u0001\"\nliteral = 'C:\\путь'\n");
	// Собранные события разбора значений
	vector <Scalar> events;
	// Выполняем проверку разбора собранного текста настроек
	ASSERT_EQ(::reread(writer.text(), events), toml::error_t::NONE);
	// Выполняем проверку количества прочитанных значений
	ASSERT_EQ(events.size(), 2u);
	// Выполняем проверку совпадения первого значения с записанным
	ASSERT_EQ(events.at(0).text, "путь\\к\"файлу\"\t\n\x01");
	// Выполняем проверку совпадения второго значения с записанным
	ASSERT_EQ(events.at(1).text, "C:\\путь");
}
/**
 * @brief Проверка смены ограды строки, содержимого не несущей
 *
 * @details Дословная строка одинарной кавычки не несёт, и записать её ею нечем:
 * содержимое при смене ограды не меняется
 *
 */
TEST(CodecTomlWriter, Promotion) {
	// Объект записи текста настроек
	toml::writer_t writer;
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("value"));
	// Выполняем запись строкового значения с одинарной кавычкой дословной оградой
	ASSERT_TRUE(writer.text("it's", toml::string_t::LITERAL));
	// Выполняем проверку смены ограды строки на основную
	ASSERT_EQ(writer.text(), "value = \"it's\"\n");
	// Получаем настройки записи текста настроек
	toml::writer_t::settings_t settings;
	// Запрещаем смену ограды имени и значения
	settings.promote = false;
	// Выполняем установку настроек записи текста настроек
	writer.settings(settings);
	// Выполняем сброс записи в исходное состояние
	writer.clear();
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("value"));
	// Выполняем проверку отказа записи строкового значения
	ASSERT_FALSE(writer.text("it's", toml::string_t::LITERAL));
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(writer.error(), toml::error_t::INVALID_VALUE);
}
/**
 * @brief Проверка записи многострочных строковых значений
 *
 */
TEST(CodecTomlWriter, Multiline) {
	// Объект записи текста настроек
	toml::writer_t writer;
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("basic"));
	// Выполняем запись многострочного строкового значения
	ASSERT_TRUE(writer.text("первая\nвторая", toml::string_t::MULTILINE_BASIC));
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("literal"));
	// Выполняем запись многострочного дословного строкового значения
	ASSERT_TRUE(writer.text("\\путь\nвторая", toml::string_t::MULTILINE_LITERAL));
	/**
	 * Выполняем проверку собранного текста настроек
	 *
	 * @note Знак конца строки за открывающей оградой разбор отбрасывает, и запись
	 *       его ставит всегда
	 */
	ASSERT_EQ(writer.text(), "basic = \"\"\"\nпервая\nвторая\"\"\"\nliteral = '''\n\\путь\nвторая'''\n");
	// Собранные события разбора значений
	vector <Scalar> events;
	// Выполняем проверку разбора собранного текста настроек
	ASSERT_EQ(::reread(writer.text(), events), toml::error_t::NONE);
	// Выполняем проверку количества прочитанных значений
	ASSERT_EQ(events.size(), 2u);
	// Выполняем проверку совпадения первого значения с записанным
	ASSERT_EQ(events.at(0).text, "первая\nвторая");
	// Выполняем проверку совпадения второго значения с записанным
	ASSERT_EQ(events.at(1).text, "\\путь\nвторая");
}
/**
 * @brief Проверка записи строки, начинающейся знаком конца строки
 *
 * @details Знак конца строки за открывающей оградой разбор отбрасывает: без своего
 * знака конца строки значение вернулось бы из записи обеднённым
 *
 */
TEST(CodecTomlWriter, LeadingNewline) {
	// Объект записи текста настроек
	toml::writer_t writer;
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("value"));
	// Выполняем запись строкового значения, начинающегося знаком конца строки
	ASSERT_TRUE(writer.text("\nтело", toml::string_t::MULTILINE_BASIC));
	// Собранные события разбора значений
	vector <Scalar> events;
	// Выполняем проверку разбора собранного текста настроек
	ASSERT_EQ(::reread(writer.text(), events), toml::error_t::NONE);
	// Выполняем проверку количества прочитанных значений
	ASSERT_EQ(events.size(), 1u);
	// Выполняем проверку совпадения прочитанного значения с записанным
	ASSERT_EQ(events.at(0).text, "\nтело");
}
/**
 * @brief Проверка ограждения кавычки, сливающейся с закрывающей оградой
 *
 */
TEST(CodecTomlWriter, MultilineQuotes) {
	// Объект записи текста настроек
	toml::writer_t writer;
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("value"));
	// Выполняем запись строкового значения, оканчивающегося кавычкой
	ASSERT_TRUE(writer.text("тело \"\"\" хвост\"", toml::string_t::MULTILINE_BASIC));
	// Собранные события разбора значений
	vector <Scalar> events;
	// Выполняем проверку разбора собранного текста настроек
	ASSERT_EQ(::reread(writer.text(), events), toml::error_t::NONE);
	// Выполняем проверку количества прочитанных значений
	ASSERT_EQ(events.size(), 1u);
	// Выполняем проверку совпадения прочитанного значения с записанным
	ASSERT_EQ(events.at(0).text, "тело \"\"\" хвост\"");
}
/**
 * @brief Проверка записи целых чисел разными системами счисления
 *
 */
TEST(CodecTomlWriter, Integers) {
	// Объект записи текста настроек
	toml::writer_t writer;
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("dec"));
	// Выполняем запись целого числа десятичной системой счисления
	ASSERT_TRUE(writer.integer(-1250));
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("hex"));
	// Выполняем запись целого числа шестнадцатеричной системой счисления
	ASSERT_TRUE(writer.integer(3735928559, toml::radix_t::HEX));
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("oct"));
	// Выполняем запись целого числа восьмеричной системой счисления
	ASSERT_TRUE(writer.integer(8, toml::radix_t::OCTAL));
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("bin"));
	// Выполняем запись целого числа двоичной системой счисления
	ASSERT_TRUE(writer.integer(5, toml::radix_t::BINARY));
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("zero"));
	// Выполняем запись нулевого значения двоичной системой счисления
	ASSERT_TRUE(writer.integer(0, toml::radix_t::BINARY));
	// Выполняем проверку собранного текста настроек
	ASSERT_EQ(writer.text(), "dec = -1250\nhex = 0xDEADBEEF\noct = 0o10\nbin = 0b101\nzero = 0b0\n");
	// Собранные события разбора значений
	vector <Scalar> events;
	// Выполняем проверку разбора собранного текста настроек
	ASSERT_EQ(::reread(writer.text(), events), toml::error_t::NONE);
	// Выполняем проверку количества прочитанных значений
	ASSERT_EQ(events.size(), 5u);
	// Выполняем проверку совпадения прочитанного числа с записанным
	ASSERT_EQ(events.at(1).integer, 3735928559);
	// Выполняем проверку сохранения системы счисления записи числа
	ASSERT_EQ(events.at(1).radix, toml::radix_t::HEX);
}
/**
 * @brief Проверка отказа записи отрицательного числа системой счисления с приставкой
 *
 * @details Описание отводит знак числа лишь десятичной записи, и сменить систему
 * счисления молча запись не вправе: выбрана она человеком
 *
 */
TEST(CodecTomlWriter, NegativeRadix) {
	// Объект записи текста настроек
	toml::writer_t writer;
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("value"));
	// Выполняем проверку отказа записи отрицательного числа
	ASSERT_FALSE(writer.integer(-1, toml::radix_t::HEX));
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(writer.error(), toml::error_t::INVALID_NUMBER);
}
/**
 * @brief Проверка записи чисел с плавающей точкой
 *
 * @details Число с плавающей точкой обязано оставаться им при обратном чтении:
 * запись «1» описание читает целым числом
 *
 */
TEST(CodecTomlWriter, Floats) {
	// Объект записи текста настроек
	toml::writer_t writer;
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("whole"));
	// Выполняем запись числа с плавающей точкой без дробной части
	ASSERT_TRUE(writer.real(1.0));
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("short"));
	// Выполняем запись числа с плавающей точкой
	ASSERT_TRUE(writer.real(0.1));
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("infinite"));
	// Выполняем запись бесконечности
	ASSERT_TRUE(writer.real(-numeric_limits <double>::infinity()));
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("undefined"));
	// Выполняем запись нечисла
	ASSERT_TRUE(writer.real(numeric_limits <double>::quiet_NaN()));
	// Выполняем проверку собранного текста настроек
	ASSERT_EQ(writer.text(), "whole = 1.0\nshort = 0.1\ninfinite = -inf\nundefined = nan\n");
	// Собранные события разбора значений
	vector <Scalar> events;
	// Выполняем проверку разбора собранного текста настроек
	ASSERT_EQ(::reread(writer.text(), events), toml::error_t::NONE);
	// Выполняем проверку количества прочитанных значений
	ASSERT_EQ(events.size(), 4u);
	// Выполняем проверку сохранения типа значения при обратном чтении
	ASSERT_EQ(events.at(0).type, toml::type_t::FLOAT);
	// Выполняем проверку совпадения прочитанного числа с записанным
	ASSERT_EQ(events.at(1).real, 0.1);
	// Выполняем проверку прочитанной бесконечности
	ASSERT_TRUE(std::isinf(events.at(2).real) && (events.at(2).real < 0));
	// Выполняем проверку прочитанного нечисла
	ASSERT_TRUE(std::isnan(events.at(3).real));
}
/**
 * @brief Проверка записи отметок времени
 *
 */
TEST(CodecTomlWriter, Stamps) {
	// Объект записи текста настроек
	toml::writer_t writer;
	// Записываемая отметка времени
	toml::stamp_t stamp;
	// Устанавливаем год отметки времени
	stamp.date.year = 2026;
	// Устанавливаем месяц отметки времени
	stamp.date.month = 8;
	// Устанавливаем день отметки времени
	stamp.date.day = 12;
	// Устанавливаем час отметки времени
	stamp.time.hour = 7;
	// Устанавливаем минуту отметки времени
	stamp.time.minute = 32;
	// Устанавливаем секунду отметки времени
	stamp.time.second = 0;
	// Устанавливаем долю секунды отметки времени
	stamp.time.nanosecond = 123000000;
	// Устанавливаем количество записанных разрядов доли секунды
	stamp.time.digits = 3;
	// Устанавливаем смещение часового пояса
	stamp.offset = -(5 * 60 + 30);
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("offset"));
	// Выполняем запись отметки времени со смещением часового пояса
	ASSERT_TRUE(writer.stamp(stamp, toml::type_t::OFFSET_DATETIME));
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("local"));
	// Выполняем запись отметки времени без смещения часового пояса
	ASSERT_TRUE(writer.stamp(stamp, toml::type_t::LOCAL_DATETIME));
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("date"));
	// Выполняем запись местной даты
	ASSERT_TRUE(writer.stamp(stamp, toml::type_t::LOCAL_DATE));
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("time"));
	// Выполняем запись местного времени
	ASSERT_TRUE(writer.stamp(stamp, toml::type_t::LOCAL_TIME));
	// Выполняем проверку собранного текста настроек
	ASSERT_EQ(writer.text(),
	 "offset = 2026-08-12T07:32:00.123-05:30\n"
	 "local = 2026-08-12T07:32:00.123\n"
	 "date = 2026-08-12\n"
	 "time = 07:32:00.123\n");
	// Собранные события разбора значений
	vector <Scalar> events;
	// Выполняем проверку разбора собранного текста настроек
	ASSERT_EQ(::reread(writer.text(), events), toml::error_t::NONE);
	// Выполняем проверку количества прочитанных значений
	ASSERT_EQ(events.size(), 4u);
	// Выполняем проверку сохранения вида отметки времени
	ASSERT_EQ(events.at(0).type, toml::type_t::OFFSET_DATETIME);
	// Выполняем проверку сохранения смещения часового пояса
	ASSERT_EQ(events.at(0).stamp.offset, -(5 * 60 + 30));
	// Выполняем проверку сохранения доли секунды отметки времени
	ASSERT_EQ(events.at(1).stamp.time.nanosecond, 123000000u);
	// Выполняем проверку сохранения количества разрядов доли секунды
	ASSERT_EQ(events.at(1).stamp.time.digits, 3u);
	// Выполняем проверку вида отметки местной даты
	ASSERT_EQ(events.at(2).type, toml::type_t::LOCAL_DATE);
	// Выполняем проверку вида отметки местного времени
	ASSERT_EQ(events.at(3).type, toml::type_t::LOCAL_TIME);
}
/**
 * @brief Проверка записи часового пояса UTC знаком «Z»
 *
 */
TEST(CodecTomlWriter, ZuluStamp) {
	// Объект записи текста настроек
	toml::writer_t writer;
	// Записываемая отметка времени
	toml::stamp_t stamp;
	// Устанавливаем год отметки времени
	stamp.date.year = 2026;
	// Устанавливаем месяц отметки времени
	stamp.date.month = 1;
	// Устанавливаем день отметки времени
	stamp.date.day = 2;
	// Устанавливаем смещение часового пояса UTC
	stamp.offset = 0;
	// Устанавливаем признак записи часового пояса знаком «Z»
	stamp.zulu = true;
	// Устанавливаем признак разделения даты и времени пробелом
	stamp.spaced = true;
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("value"));
	// Выполняем запись отметки времени со смещением часового пояса
	ASSERT_TRUE(writer.stamp(stamp, toml::type_t::OFFSET_DATETIME));
	// Выполняем проверку собранного текста настроек
	ASSERT_EQ(writer.text(), "value = 2026-01-02 00:00:00Z\n");
}
/**
 * @brief Проверка отказа записи ошибочно построенной отметки времени
 *
 */
TEST(CodecTomlWriter, InvalidStamp) {
	// Объект записи текста настроек
	toml::writer_t writer;
	// Записываемая отметка времени
	toml::stamp_t stamp;
	// Устанавливаем год отметки времени
	stamp.date.year = 2026;
	// Устанавливаем ошибочный месяц отметки времени
	stamp.date.month = 13;
	// Устанавливаем день отметки времени
	stamp.date.day = 1;
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("value"));
	// Выполняем проверку отказа записи отметки времени
	ASSERT_FALSE(writer.stamp(stamp, toml::type_t::LOCAL_DATE));
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(writer.error(), toml::error_t::INVALID_DATETIME);
}
/**
 * @brief Проверка записи перечня значений одной строкой
 *
 */
TEST(CodecTomlWriter, Arrays) {
	// Объект записи текста настроек
	toml::writer_t writer;
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("ports"));
	// Выполняем запись начала перечня значений
	ASSERT_TRUE(writer.arrayOpen());
	// Выполняем запись первого значения перечня
	ASSERT_TRUE(writer.integer(80));
	// Выполняем запись второго значения перечня
	ASSERT_TRUE(writer.integer(443));
	// Выполняем запись вложенного перечня значений
	ASSERT_TRUE(writer.arrayOpen());
	// Выполняем запись значения вложенного перечня
	ASSERT_TRUE(writer.text("вложенное"));
	// Выполняем запись конца вложенного перечня значений
	ASSERT_TRUE(writer.arrayClose());
	// Выполняем запись конца перечня значений
	ASSERT_TRUE(writer.arrayClose());
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("empty"));
	// Выполняем запись начала пустого перечня значений
	ASSERT_TRUE(writer.arrayOpen());
	// Выполняем запись конца пустого перечня значений
	ASSERT_TRUE(writer.arrayClose());
	// Выполняем проверку собранного текста настроек
	ASSERT_EQ(writer.text(), "ports = [80, 443, [\"вложенное\"]]\nempty = []\n");
	// Собранные события разбора значений
	vector <Scalar> events;
	// Выполняем проверку разбора собранного текста настроек
	ASSERT_EQ(::reread(writer.text(), events), toml::error_t::NONE);
	// Выполняем проверку количества прочитанных значений
	ASSERT_EQ(events.size(), 3u);
}
/**
 * @brief Проверка записи перечня значений несколькими строками
 *
 */
TEST(CodecTomlWriter, MultilineArray) {
	// Объект записи текста настроек
	toml::writer_t writer;
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("hosts"));
	// Выполняем запись начала перечня значений несколькими строками
	ASSERT_TRUE(writer.arrayOpen(true));
	// Выполняем запись первого значения перечня
	ASSERT_TRUE(writer.text("первый"));
	// Выполняем запись второго значения перечня
	ASSERT_TRUE(writer.text("второй"));
	// Выполняем запись конца перечня значений
	ASSERT_TRUE(writer.arrayClose());
	// Выполняем проверку собранного текста настроек
	ASSERT_EQ(writer.text(), "hosts = [\n\t\"первый\",\n\t\"второй\"\n]\n");
	// Собранные события разбора значений
	vector <Scalar> events;
	// Выполняем проверку разбора собранного текста настроек
	ASSERT_EQ(::reread(writer.text(), events), toml::error_t::NONE);
	// Выполняем проверку количества прочитанных значений
	ASSERT_EQ(events.size(), 2u);
}
/**
 * @brief Проверка записи встроенной таблицы
 *
 */
TEST(CodecTomlWriter, InlineTable) {
	// Объект записи текста настроек
	toml::writer_t writer;
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("point"));
	// Выполняем запись начала встроенной таблицы
	ASSERT_TRUE(writer.inlineOpen());
	// Выполняем запись имени ключа пары встроенной таблицы
	ASSERT_TRUE(writer.key("x"));
	// Выполняем запись значения пары встроенной таблицы
	ASSERT_TRUE(writer.integer(1));
	// Выполняем запись имени ключа пары встроенной таблицы
	ASSERT_TRUE(writer.key("y"));
	// Выполняем запись значения пары встроенной таблицы
	ASSERT_TRUE(writer.integer(2));
	// Выполняем запись конца встроенной таблицы
	ASSERT_TRUE(writer.inlineClose());
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("empty"));
	// Выполняем запись начала пустой встроенной таблицы
	ASSERT_TRUE(writer.inlineOpen());
	// Выполняем запись конца пустой встроенной таблицы
	ASSERT_TRUE(writer.inlineClose());
	// Выполняем проверку собранного текста настроек
	ASSERT_EQ(writer.text(), "point = { x = 1, y = 2 }\nempty = {}\n");
	// Собранные события разбора значений
	vector <Scalar> events;
	// Выполняем проверку разбора собранного текста настроек
	ASSERT_EQ(::reread(writer.text(), events), toml::error_t::NONE);
	// Выполняем проверку количества прочитанных значений
	ASSERT_EQ(events.size(), 2u);
}
/**
 * @brief Проверка записи наборов таблиц
 *
 */
TEST(CodecTomlWriter, ArrayTables) {
	// Объект записи текста настроек
	toml::writer_t writer;
	// Выполняем запись объявления очередной таблицы набора таблиц
	ASSERT_TRUE(writer.arrayTable("products"));
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("name"));
	// Выполняем запись значения пары
	ASSERT_TRUE(writer.text("гвоздь"));
	// Выполняем запись объявления очередной таблицы набора таблиц
	ASSERT_TRUE(writer.arrayTable("products"));
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("name"));
	// Выполняем запись значения пары
	ASSERT_TRUE(writer.text("молоток"));
	// Выполняем проверку собранного текста настроек
	ASSERT_EQ(writer.text(), "[[products]]\nname = \"гвоздь\"\n\n[[products]]\nname = \"молоток\"\n");
	// Собранные события разбора значений
	vector <Scalar> events;
	// Выполняем проверку разбора собранного текста настроек
	ASSERT_EQ(::reread(writer.text(), events), toml::error_t::NONE);
	// Выполняем проверку количества прочитанных значений
	ASSERT_EQ(events.size(), 2u);
}
/**
 * @brief Проверка записи примечаний
 *
 */
TEST(CodecTomlWriter, Comments) {
	// Объект записи текста настроек
	toml::writer_t writer;
	// Выполняем запись примечания, занимающего несколько строк
	ASSERT_TRUE(writer.comment("первая\nвторая"));
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("value"));
	// Выполняем запись значения пары
	ASSERT_TRUE(writer.integer(1));
	// Выполняем дописывание примечания к записанной строке
	ASSERT_TRUE(writer.trailing("хвост"));
	// Выполняем запись пустой строки
	ASSERT_TRUE(writer.blank());
	// Выполняем проверку собранного текста настроек
	ASSERT_EQ(writer.text(), "# первая\n# вторая\nvalue = 1 # хвост\n\n");
	// Собранные события разбора значений
	vector <Scalar> events;
	// Выполняем проверку разбора собранного текста настроек
	ASSERT_EQ(::reread(writer.text(), events), toml::error_t::NONE);
	// Выполняем проверку количества прочитанных значений
	ASSERT_EQ(events.size(), 1u);
}
/**
 * @brief Проверка отказа дописывания примечания к строке примечания
 *
 */
TEST(CodecTomlWriter, TrailingRefusal) {
	// Объект записи текста настроек
	toml::writer_t writer;
	// Выполняем запись примечания
	ASSERT_TRUE(writer.comment("примечание"));
	// Выполняем проверку отказа дописывания примечания к строке примечания
	ASSERT_FALSE(writer.trailing("хвост"));
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(writer.error(), toml::error_t::UNEXPECTED_CONTENT);
	// Выполняем сброс записи в исходное состояние
	writer.clear();
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("value"));
	// Выполняем запись значения пары
	ASSERT_TRUE(writer.integer(1));
	// Выполняем проверку отказа дописывания примечания со знаком конца строки
	ASSERT_FALSE(writer.trailing("первая\nвторая"));
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(writer.error(), toml::error_t::INVALID_CHARACTER);
}
/**
 * @brief Проверка отказа записи строки текста посреди значения
 *
 * @details Незаписанное значение пары оставляет текст недописанным, и выдача его
 * отвечает отказом: собранным он не является
 *
 */
TEST(CodecTomlWriter, Unfinished) {
	// Объект записи текста настроек
	toml::writer_t writer;
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("value"));
	// Выполняем проверку отказа записи объявления таблицы
	ASSERT_FALSE(writer.table("server"));
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(writer.error(), toml::error_t::MISSING_VALUE);
	// Выполняем проверку отказа выдачи недописанного текста настроек
	ASSERT_TRUE(writer.text().empty());
	// Выполняем запись значения пары
	ASSERT_TRUE(writer.integer(1));
	// Выполняем проверку выдачи собранного текста настроек
	ASSERT_EQ(writer.text(), "value = 1\n");
	// Выполняем запись начала перечня значений вне пары
	ASSERT_FALSE(writer.arrayOpen());
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(writer.error(), toml::error_t::UNEXPECTED_CONTENT);
}
/**
 * @brief Проверка отказа закрытия незаписанного составного значения
 *
 */
TEST(CodecTomlWriter, Unbalanced) {
	// Объект записи текста настроек
	toml::writer_t writer;
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("value"));
	// Выполняем запись начала перечня значений
	ASSERT_TRUE(writer.arrayOpen());
	// Выполняем проверку отказа закрытия перечня скобкой встроенной таблицы
	ASSERT_FALSE(writer.inlineClose());
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(writer.error(), toml::error_t::UNCLOSED_INLINE_TABLE);
	// Выполняем запись конца перечня значений
	ASSERT_TRUE(writer.arrayClose());
	// Выполняем проверку отказа закрытия незаписанного перечня значений
	ASSERT_FALSE(writer.arrayClose());
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(writer.error(), toml::error_t::UNCLOSED_ARRAY);
}
/**
 * @brief Проверка украшений собираемого текста настроек
 *
 */
TEST(CodecTomlWriter, Decoration) {
	// Получаем настройки записи текста настроек
	toml::writer_t::settings_t settings;
	// Задаём запись отступа перед парами таблицы
	settings.indent = true;
	// Отменяем запись пробелов вокруг знака равенства
	settings.spaces = false;
	// Отменяем запись пустой строки перед объявлением таблицы
	settings.separated = false;
	// Задаём знаком конца строки пару возврата каретки с переводом строки
	settings.newline = toml::newline_t::CRLF;
	// Объект записи текста настроек
	toml::writer_t writer(settings);
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("global"));
	// Выполняем запись значения пары
	ASSERT_TRUE(writer.integer(1));
	// Выполняем запись объявления таблицы
	ASSERT_TRUE(writer.table("server"));
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("port"));
	// Выполняем запись значения пары
	ASSERT_TRUE(writer.integer(8080));
	// Выполняем проверку собранного текста настроек
	ASSERT_EQ(writer.text(), "global=1\r\n[server]\r\n\tport=8080\r\n");
	// Собранные события разбора значений
	vector <Scalar> events;
	// Выполняем проверку разбора собранного текста настроек
	ASSERT_EQ(::reread(writer.text(), events), toml::error_t::NONE);
	// Выполняем проверку количества прочитанных значений
	ASSERT_EQ(events.size(), 2u);
}
/**
 * @brief Проверка пределов записи текста настроек
 *
 */
TEST(CodecTomlWriter, Limits) {
	// Получаем настройки записи текста настроек
	toml::writer_t::settings_t settings;
	// Устанавливаем наибольшую допустимую длину имени ключа
	settings.maxKey = 4;
	// Устанавливаем наибольшее допустимое количество частей имени ключа
	settings.maxParts = 2;
	// Устанавливаем наибольшую допустимую глубину вложенности значений
	settings.maxDepth = 2;
	// Объект записи текста настроек
	toml::writer_t writer(settings);
	// Выполняем проверку отказа записи имени ключа сверх предела длины
	ASSERT_FALSE(writer.key("длинное"));
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(writer.error(), toml::error_t::KEY_TOO_LONG);
	// Выполняем сброс записи в исходное состояние
	writer.clear();
	// Выполняем проверку отказа записи имени ключа сверх предела количества частей
	ASSERT_FALSE(writer.key(::path({"a", "b", "c"})));
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(writer.error(), toml::error_t::PARTS_EXCEEDED);
	// Выполняем сброс записи в исходное состояние
	writer.clear();
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("a"));
	/**
	 * Выполняем запись перечней по числу дозволенных уровней вложенности
	 *
	 * @note Уровень пары глубиной не считается: разбор меряет глубину вложенностью
	 *       перечней и встроенных таблиц друг в друга, и счёт уровня пары наравне с
	 *       ними отвергал бы запись того, что разбор принимает
	 */
	ASSERT_TRUE(writer.arrayOpen());
	// Выполняем запись начала вложенного перечня значений
	ASSERT_TRUE(writer.arrayOpen());
	// Выполняем проверку отказа записи перечня сверх предела глубины вложенности
	ASSERT_FALSE(writer.arrayOpen());
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(writer.error(), toml::error_t::DEPTH_EXCEEDED);
}
/**
 * @brief Проверка совпадения предела глубины записи с пределом разбора
 *
 * @details Пределы меряются одной и той же величиной, и расхождение их означало бы,
 * что запись отвергает то, что разбор принимает: дерево настроек, разобранное с
 * пределом, обратно не записалось бы
 *
 */
TEST(CodecTomlWriter, DepthLimitMatchesReader) {
	/**
	 * Выполняем перебор значений предела глубины вложенности
	 */
	for(uint32_t depth = 1; depth <= 4; depth++){
		// Собираемый текст настроек с перечнями по числу уровней вложенности
		string text("a = ");
		/**
		 * Выполняем перебор всех уровней вложенности
		 */
		for(uint32_t i = 0; i < depth; i++)
			// Выполняем добавление открывающей скобки перечня
			text.append("[");
		// Выполняем добавление значения самого вложенного перечня
		text.append("1");
		/**
		 * Выполняем перебор всех уровней вложенности
		 */
		for(uint32_t i = 0; i < depth; i++)
			// Выполняем добавление закрывающей скобки перечня
			text.append("]");
		// Выполняем добавление знака конца строки
		text.append("\n");
		// Настройки разбора текста настроек
		toml::reader_t::settings_t reading;
		// Устанавливаем наибольшую допустимую глубину вложенности значений
		reading.maxDepth = depth;
		// Объект потокового чтения текста настроек
		toml::reader_t reader(reading);
		// Выполняем подачу разбираемого текста настроек
		static_cast <void> (reader.feed(text.data(), text.size(), true));
		/**
		 * Выполняем перебор выданных разбором событий
		 */
		while(reader.next()){}
		// Выполняем проверку разбора текста настроек до конца
		ASSERT_EQ(reader.state(), toml::state_t::FINISHED);
		// Настройки записи текста настроек
		toml::writer_t::settings_t writing;
		// Устанавливаем наибольшую допустимую глубину вложенности значений
		writing.maxDepth = depth;
		// Объект записи текста настроек
		toml::writer_t writer(writing);
		// Выполняем запись имени ключа пары
		ASSERT_TRUE(writer.key("a"));
		/**
		 * Выполняем перебор всех уровней вложенности
		 */
		for(uint32_t i = 0; i < depth; i++)
			// Выполняем проверку записи начала очередного перечня значений
			ASSERT_TRUE(writer.arrayOpen());
		// Выполняем проверку записи значения самого вложенного перечня
		ASSERT_TRUE(writer.integer(1));
		/**
		 * Выполняем перебор всех уровней вложенности
		 */
		for(uint32_t i = 0; i < depth; i++)
			// Выполняем проверку записи конца очередного перечня значений
			ASSERT_TRUE(writer.arrayClose());
		// Выполняем проверку совпадения собранного текста с разобранным
		ASSERT_EQ(writer.text(), text);
	}
}
/**
 * @brief Проверка предела длины собираемой строки текста
 *
 */
TEST(CodecTomlWriter, LineLimit) {
	// Получаем настройки записи текста настроек
	toml::writer_t::settings_t settings;
	// Устанавливаем наибольшую допустимую длину логической строки
	settings.maxLine = 16;
	// Объект записи текста настроек
	toml::writer_t writer(settings);
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("value"));
	// Выполняем проверку отказа записи строки сверх предела её длины
	ASSERT_FALSE(writer.text(string(64, 'a')));
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(writer.error(), toml::error_t::LINE_TOO_LONG);
}
/**
 * @brief Проверка меры длины записи, занимающей несколько строк
 *
 * @details Разбор меряет пределом длины запись целиком - вместе со всеми строками
 * многострочного значения, - и запись обязана мерять её так же: иначе собранный ею
 * текст читающий отверг бы длиной записи
 *
 */
TEST(CodecTomlWriter, FoldedLineLimit) {
	// Получаем настройки записи текста настроек
	toml::writer_t::settings_t settings;
	// Устанавливаем наибольшую допустимую длину логической строки
	settings.maxLine = 48;
	// Объект записи текста настроек
	toml::writer_t writer(settings);
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("value"));
	/**
	 * Выполняем проверку отказа записи многострочного значения сверх предела
	 *
	 * @note Ни одна строка значения предела не превышает, а запись целиком его
	 *       превышает - отвергается она так же, как её отверг бы разбор
	 */
	ASSERT_FALSE(writer.text(string(20, 'a') + "\n" + string(20, 'b') + "\n" + string(20, 'c'), toml::string_t::MULTILINE_BASIC));
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(writer.error(), toml::error_t::LINE_TOO_LONG);
	// Выполняем сброс записи в исходное состояние
	writer.clear();
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("value"));
	// Выполняем запись начала перечня значений несколькими строками
	ASSERT_TRUE(writer.arrayOpen(true));
	// Выполняем запись первого значения перечня
	ASSERT_TRUE(writer.text(string(20, 'a')));
	// Выполняем запись второго значения перечня
	ASSERT_TRUE(writer.text(string(20, 'b')));
	// Выполняем проверку отказа записи перечня сверх предела длины записи
	ASSERT_FALSE(writer.arrayClose());
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(writer.error(), toml::error_t::LINE_TOO_LONG);
}
/**
 * @brief Проверка записи пары с числовым значением
 *
 */
TEST(CodecTomlWriter, Numbers) {
	// Объект записи текста настроек
	toml::writer_t writer;
	// Выполняем запись пары с логическим значением
	ASSERT_TRUE(writer.number("flag", true));
	// Выполняем запись пары с целым числом со знаком
	ASSERT_TRUE(writer.number("count", static_cast <int32_t> (-7)));
	// Выполняем запись пары с числом с плавающей точкой
	ASSERT_TRUE(writer.number("ratio", 2.5));
	// Выполняем проверку собранного текста настроек
	ASSERT_EQ(writer.text(), "flag = true\ncount = -7\nratio = 2.5\n");
	// Выполняем проверку отказа записи числа сверх отведённого отрезка значений
	ASSERT_FALSE(writer.number("huge", numeric_limits <uint64_t>::max()));
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(writer.error(), toml::error_t::NUMBER_OVERFLOW);
}
/**
 * @brief Проверка записи значения по признакам, сохранённым при чтении
 *
 * @details Договор о совпадении прочитанного с записанным: значение, прочитанное
 * разбором и записанное обратно, обязано читаться тем же самым
 *
 */
TEST(CodecTomlWriter, Rewrite) {
	// Разбираемый текст настроек
	const string text =
	 "# заголовок\n"
	 "[server]\n"
	 "host = 'локальный'\n"
	 "port = 0xFF\n"
	 "ratio = 0.25\n"
	 "stamp = 1979-05-27T07:32:00Z\n"
	 "flag = false\n";
	// Объект потокового чтения текста настроек
	toml::reader_t reader;
	// Объект записи текста настроек
	toml::writer_t writer;
	// Выполняем подачу разбираемого текста настроек
	ASSERT_TRUE(reader.feed(text.data(), text.size(), true));
	/**
	 * Выполняем перебор выданных разбором событий
	 */
	while(reader.next()){
		/**
		 * Выполняем выбор разновидности события разбора
		 */
		switch(static_cast <uint8_t> (reader.event())){
			// Если событием является примечание
			case static_cast <uint8_t> (toml::event_t::COMMENT):
				// Выполняем запись примечания
				ASSERT_TRUE(writer.comment(reader.comment().text));
			break;
			// Если событием является объявление таблицы
			case static_cast <uint8_t> (toml::event_t::TABLE):
				// Выполняем запись объявления таблицы
				ASSERT_TRUE(writer.table(reader.path()));
			break;
			// Если событием является имя ключа пары
			case static_cast <uint8_t> (toml::event_t::KEY):
				// Выполняем запись имени ключа пары
				ASSERT_TRUE(writer.key(reader.path()));
			break;
			// Если событием является значение
			case static_cast <uint8_t> (toml::event_t::VALUE):
				// Выполняем запись значения
				ASSERT_TRUE(writer.value(reader.value()));
			break;
		}
	}
	// Выполняем проверку того, что разбор завершился без ошибок
	ASSERT_EQ(reader.error(), toml::error_t::NONE);
	/**
	 * Выполняем проверку собранного текста настроек
	 *
	 * @note Пустая строка перед объявлением таблицы поставлена украшением записи, а
	 *       ограда строки, система счисления числа и запись отметки времени взяты
	 *       такими, какими их выбрал человек
	 */
	ASSERT_EQ(writer.text(),
	 "# заголовок\n"
	 "\n"
	 "[server]\n"
	 "host = 'локальный'\n"
	 "port = 0xFF\n"
	 "ratio = 0.25\n"
	 "stamp = 1979-05-27T07:32:00Z\n"
	 "flag = false\n");
}
/**
 * @brief Проверка записи возврата каретки в строковом значении
 *
 * @details Разбор приводит пару «возврат каретки - перевод строки» к одному переводу,
 * и записанный собою возврат каретки при обратном чтении пропадал бы молча: в
 * многострочной основной ограде он записывается управляющей последовательностью, а
 * многострочная дословная его не несёт вовсе
 *
 */
TEST(CodecTomlWriter, CarriageReturn) {
	// Объект записи текста настроек
	toml::writer_t writer;
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("value"));
	// Выполняем запись многострочного значения с возвратом каретки
	ASSERT_TRUE(writer.text("возврат\r\nкаретки", toml::string_t::MULTILINE_BASIC));
	// Собранные события разбора значений
	vector <Scalar> events;
	// Выполняем проверку разбора собранного текста настроек
	ASSERT_EQ(::reread(writer.text(), events), toml::error_t::NONE);
	// Выполняем проверку количества прочитанных значений
	ASSERT_EQ(events.size(), 1u);
	// Выполняем проверку совпадения прочитанного значения с записанным
	ASSERT_EQ(events.at(0).text, "возврат\r\nкаретки");
	// Получаем настройки записи текста настроек
	toml::writer_t::settings_t settings;
	// Запрещаем смену ограды имени и значения
	settings.promote = false;
	// Выполняем установку настроек записи текста настроек
	writer.settings(settings);
	// Выполняем сброс записи в исходное состояние
	writer.clear();
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("value"));
	// Выполняем проверку отказа записи возврата каретки дословной оградой
	ASSERT_FALSE(writer.text("возврат\r\nкаретки", toml::string_t::MULTILINE_LITERAL));
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(writer.error(), toml::error_t::INVALID_VALUE);
}
/**
 * @brief Проверка отступа строк многострочного перечня
 *
 * @details Отступ перед парами таблицы украшающий, и строки продолжения перечня
 * обязаны нести его наравне со строкой самой пары: без него продолжение уходило бы
 * левее её начала
 *
 */
TEST(CodecTomlWriter, IndentedArray) {
	// Получаем настройки записи текста настроек
	toml::writer_t::settings_t settings;
	// Задаём запись отступа перед парами таблицы
	settings.indent = true;
	// Отменяем запись пустой строки перед объявлением таблицы
	settings.separated = false;
	// Объект записи текста настроек
	toml::writer_t writer(settings);
	// Выполняем запись объявления таблицы
	ASSERT_TRUE(writer.table("server"));
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("hosts"));
	// Выполняем запись начала перечня значений несколькими строками
	ASSERT_TRUE(writer.arrayOpen(true));
	// Выполняем запись первого значения перечня
	ASSERT_TRUE(writer.integer(1));
	// Выполняем запись второго значения перечня
	ASSERT_TRUE(writer.integer(2));
	// Выполняем запись конца перечня значений
	ASSERT_TRUE(writer.arrayClose());
	// Выполняем проверку собранного текста настроек
	ASSERT_EQ(writer.text(), "[server]\n\thosts = [\n\t\t1,\n\t\t2\n\t]\n");
	// Собранные события разбора значений
	vector <Scalar> events;
	// Выполняем проверку разбора собранного текста настроек
	ASSERT_EQ(::reread(writer.text(), events), toml::error_t::NONE);
	// Выполняем проверку количества прочитанных значений
	ASSERT_EQ(events.size(), 2u);
}
/**
 * @brief Проверка предела длины записи с примечанием в конце строки
 *
 * @details Примечание, дописываемое к готовой строке, снимает знак её конца и
 * дописывает содержимое к той же записи: длина записи ей продолжается. Считать её
 * заново по собранному тексту нельзя - разбор меряет пределом запись целиком, а по
 * тексту видна лишь последняя из её строк, и предел обходился бы примечанием
 *
 */
TEST(CodecTomlWriter, TrailingLineLimit) {
	// Получаем настройки записи текста настроек
	toml::writer_t::settings_t settings;
	// Устанавливаем наибольшую допустимую длину логической строки
	settings.maxLine = 20;
	// Объект записи текста настроек
	toml::writer_t writer(settings);
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("a"));
	// Выполняем запись начала перечня значений несколькими строками
	ASSERT_TRUE(writer.arrayOpen(true));
	// Выполняем запись первого значения перечня
	ASSERT_TRUE(writer.integer(1));
	// Выполняем запись второго значения перечня
	ASSERT_TRUE(writer.integer(2));
	// Выполняем запись конца перечня значений
	ASSERT_TRUE(writer.arrayClose());
	// Выполняем проверку отказа дописывания примечания сверх предела длины записи
	ASSERT_FALSE(writer.trailing("длинное примечание"));
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(writer.error(), toml::error_t::LINE_TOO_LONG);
}
/**
 * @brief Проверка согласия предела длины записи с разбором
 *
 * @details Собранный записью текст обязан разбираться теми же пределами: расхождение
 * мер означало бы, что дерево настроек, разобранное с пределом, обратно записывается
 * текстом, который то же чтение отвергает
 *
 */
TEST(CodecTomlWriter, LineLimitMatchesReader) {
	/**
	 * Выполняем перебор значений предела длины логической строки
	 */
	for(uint32_t limit = 12; limit <= 40; limit++){
		// Настройки записи текста настроек
		toml::writer_t::settings_t writing;
		// Устанавливаем наибольшую допустимую длину логической строки
		writing.maxLine = limit;
		// Объект записи текста настроек
		toml::writer_t writer(writing);
		// Признак успешной сборки текста настроек
		bool wrote = (writer.key("a") && writer.arrayOpen(true) && writer.integer(1) &&
		              writer.integer(2) && writer.arrayClose() && writer.trailing("хвост"));
		/**
		 * Если собрать текст настроек не удалось
		 */
		if(!wrote){
			// Выполняем проверку кода ошибки записи
			ASSERT_EQ(writer.error(), toml::error_t::LINE_TOO_LONG);
			// Выполняем переход к следующему значению предела
			continue;
		}
		// Получаем собранный текст настроек
		const string text = writer.text();
		// Настройки разбора текста настроек
		toml::reader_t::settings_t reading;
		// Устанавливаем наибольшую допустимую длину логической строки
		reading.maxLine = limit;
		// Объект потокового чтения текста настроек
		toml::reader_t reader(reading);
		// Выполняем подачу разбираемого текста настроек
		static_cast <void> (reader.feed(text.data(), text.size(), true));
		/**
		 * Выполняем перебор выданных разбором событий
		 */
		while(reader.next()){}
		// Выполняем проверку разбора собранного текста до конца
		ASSERT_EQ(reader.state(), toml::state_t::FINISHED) << "предел " << limit << " текст «" << text << "»";
	}
}
/**
 * @brief Проверка запрета вложенности нулевым пределом глубины
 *
 * @details Ноль пределом глубины запрещает вложенные значения вовсе, и смысл этот у
 * записи тот же, что и у разбора: считай запись ноль снятием предела, дерево собирало
 * бы перечень, который читающий с теми же настройками отвергает
 *
 */
TEST(CodecTomlWriter, DepthLimitZero) {
	// Настройки записи текста настроек
	toml::writer_t::settings_t writing;
	// Устанавливаем запрет вложенных значений
	writing.maxDepth = 0;
	// Объект записи текста настроек
	toml::writer_t writer(writing);
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("a"));
	// Выполняем проверку отказа записи перечня значений
	ASSERT_FALSE(writer.arrayOpen());
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(writer.error(), toml::error_t::DEPTH_EXCEEDED);
	// Объект записи текста настроек встроенной таблицы
	toml::writer_t inlined(writing);
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(inlined.key("a"));
	// Выполняем проверку отказа записи встроенной таблицы
	ASSERT_FALSE(inlined.inlineOpen());
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(inlined.error(), toml::error_t::DEPTH_EXCEEDED);
	// Настройки разбора текста настроек
	toml::reader_t::settings_t reading;
	// Устанавливаем запрет вложенных значений
	reading.maxDepth = 0;
	// Объект потокового чтения текста настроек
	toml::reader_t reader(reading);
	// Собираемый текст настроек с перечнем значений
	const string text("a = [1]\n");
	// Выполняем подачу разбираемого текста настроек
	static_cast <void> (reader.feed(text.data(), text.size(), true));
	/**
	 * Выполняем перебор выданных разбором событий
	 */
	while(reader.next()){}
	// Выполняем проверку того, что разбор перечень отверг
	ASSERT_EQ(reader.error(), toml::error_t::DEPTH_EXCEEDED);
}
/**
 * @brief Проверка записи примечаний внутри перечня значений
 *
 * @details Примечание внутри перечня записывается лишь там, где разбор его прочтёт:
 * внутри перечня, собираемого несколькими строками. Одной строкой перечень с
 * примечанием не собрать - закрывающая скобка досталась бы содержимому его
 *
 */
TEST(CodecTomlWriter, ArrayRemarks) {
	// Объект записи текста настроек
	toml::writer_t writer;
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("a"));
	// Выполняем запись начала многострочного перечня значений
	ASSERT_TRUE(writer.arrayOpen(true));
	// Выполняем запись первого значения перечня
	ASSERT_TRUE(writer.integer(1));
	// Выполняем дописывание примечания к значению перечня
	ASSERT_TRUE(writer.remarked("первое", true));
	// Выполняем запись примечания строкой перечня
	ASSERT_TRUE(writer.remark("своей строкой"));
	// Выполняем запись второго значения перечня
	ASSERT_TRUE(writer.integer(2));
	// Выполняем дописывание примечания к последнему значению перечня
	ASSERT_TRUE(writer.remarked("последнее", false));
	// Выполняем запись конца перечня значений
	ASSERT_TRUE(writer.arrayClose());
	// Выполняем проверку собранного текста настроек
	ASSERT_EQ(writer.text(), string("a = [\n\t1, # первое\n\t# своей строкой\n\t2 # последнее\n]\n"));
	// Объект записи текста настроек одной строкой
	toml::writer_t single;
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(single.key("a"));
	// Выполняем запись начала перечня значений одной строкой
	ASSERT_TRUE(single.arrayOpen());
	// Выполняем запись значения перечня
	ASSERT_TRUE(single.integer(1));
	// Выполняем проверку отказа записи примечания внутри однострочного перечня
	ASSERT_FALSE(single.remarked("нельзя", false));
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(single.error(), toml::error_t::UNEXPECTED_CONTENT);
	// Объект записи текста настроек вне перечня
	toml::writer_t outside;
	// Выполняем проверку отказа записи примечания перечня вне перечня
	ASSERT_FALSE(outside.remark("нельзя"));
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(outside.error(), toml::error_t::UNEXPECTED_CONTENT);
}
