/**
 * @file writer.cpp
 * @date 2026-08-12
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
 * @brief Проверки записи текста настроек TOML — построение имён ключей, ограждение
 *        строковых значений, запись чисел, отметок времени, перечней, встроенных таблиц,
 *        примечаний, а также договор о совпадении записанного с прочитанным обратно
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 *
 * @note Заголовок cmath нужен ради std::isinf и std::isnan. Собиратели посвежее
 *       подтягивают его попутно другими заголовками, а gcc 12 с glibc 2.36 - нет,
 *       и сборка валится с "isinf is not a member of std". Проверено на стенде
 *       Debian 12
 */
#include <clocale>
#include <cmath>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <gtest/gtest.h>
#include <codec/toml/toml.hpp>
#include <sys/log.hpp>

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
	toml::reader_t reader(::logger());
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
	toml::writer_t writer(::logger());
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
	toml::writer_t writer(::logger());
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
	toml::writer_t writer(::logger());
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
	toml::writer_t writer(::logger(), settings);
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
	toml::writer_t writer(::logger());
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
	toml::writer_t writer(::logger());
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
	toml::writer_t writer(::logger());
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
	 * @note Знак конца строки за открывающей оградой ставится лишь тогда, когда
	 *       содержимое им начато: разбор его отбрасывает, и без него такое значение
	 *       прочиталось бы обратно без своего первого перевода. Прежде он ставился
	 *       ВСЕГДА, и здесь закреплялось именно это - неверно: всякое многострочное
	 *       значение растягивало запись на строку лишнюю, а перечень, такое значение
	 *       несущий, при обратном чтении становился многострочным, хотя человек написал
	 *       его одною строкой, - перезапись не сходилась сама с собою
	 */
	ASSERT_EQ(writer.text(), "basic = \"\"\"первая\nвторая\"\"\"\nliteral = '''\\путь\nвторая'''\n");
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
	toml::writer_t writer(::logger());
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
	toml::writer_t writer(::logger());
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
	toml::writer_t writer(::logger());
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
	toml::writer_t writer(::logger());
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
	toml::writer_t writer(::logger());
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
 * @brief Проверка сохранения знака у нечисла при обратном чтении
 *
 * @note Читающий знак у нечисла хранит, и пишущий обязан его ставить: иначе круг
 *       «прочли - записали - прочли» размыкается, и дерево перезаписи расходится
 *       с деревом исходника
 *
 */
/**
 * @brief Проверка посредников записи значения, одним именем зовущихся
 *
 * @details Посредники заведены затем, чтобы потребитель, вид значения по месту
 *          разбирающий, не знал про запись TOML того, чего знать не должен, - про
 *          систему счисления числа да про ограду строки
 *
 */
TEST(CodecTomlWriter, ValueOverloadsMatchTheirKinds) {
	// Объект записи текста настроек
	toml::writer_t writer(::logger());
	// Выполняем запись логического значения посредником
	ASSERT_TRUE(writer.key("flag"));
	ASSERT_TRUE(writer.value(true));
	// Выполняем запись целого числа со знаком посредником
	ASSERT_TRUE(writer.key("count"));
	ASSERT_TRUE(writer.value(static_cast <int64_t> (-7)));
	// Выполняем запись целого числа без знака посредником
	ASSERT_TRUE(writer.key("size"));
	ASSERT_TRUE(writer.value(static_cast <uint64_t> (42)));
	// Выполняем запись вещественного числа посредником
	ASSERT_TRUE(writer.key("ratio"));
	ASSERT_TRUE(writer.value(2.5));
	// Выполняем запись строкового значения посредником
	ASSERT_TRUE(writer.key("host"));
	ASSERT_TRUE(writer.value(string_view("anyks")));
	// Выполняем проверку собранного текста настроек
	ASSERT_EQ(writer.text(), "flag = true\ncount = -7\nsize = 42\nratio = 2.5\nhost = \"anyks\"\n");
}
/**
 * @brief Проверка отказа посредника на числе без знака, предел знакового превысившем
 *
 * @details Описание TOML целых без знака не несёт вовсе, и приведение со знаком
 *          переменило бы число МОЛЧА: потребитель получил бы текст с числом
 *          отрицательным вместо затребованного. Оттого запись отвергается с названною
 *          причиною, а не приводится
 *
 */
TEST(CodecTomlWriter, UnsignedBeyondSignedLimitIsRefused) {
	// Объект записи текста настроек
	toml::writer_t writer(::logger());
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("huge"));
	// Выполняем проверку отказа записи числа, предел знакового превысившего
	ASSERT_FALSE(writer.value(static_cast <uint64_t> (numeric_limits <int64_t>::max()) + 1u));
	// Выполняем проверку того, что отказ причину свою назвал
	ASSERT_EQ(writer.error(), toml::error_t::NUMBER_OVERFLOW);
}
TEST(CodecTomlWriter, NegativeNotANumberKeepsSign) {
	// Объект записи текста настроек
	toml::writer_t writer(::logger());
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("undefined"));
	// Выполняем запись нечисла со знаком
	ASSERT_TRUE(writer.real(-numeric_limits <double>::quiet_NaN()));
	// Выполняем проверку собранного текста настроек
	ASSERT_EQ(writer.text(), "undefined = -nan\n");
	// Собранные события разбора значений
	vector <Scalar> events;
	// Выполняем проверку разбора собранного текста настроек
	ASSERT_EQ(::reread(writer.text(), events), toml::error_t::NONE);
	// Выполняем проверку количества прочитанных значений
	ASSERT_EQ(events.size(), 1u);
	// Выполняем проверку сохранения знака нечисла при обратном чтении
	ASSERT_TRUE(std::isnan(events.at(0).real) && std::signbit(events.at(0).real));
}
/**
 * @brief Проверка записи чисел при локали с иным десятичным знаком
 *
 * @details Запись числа ведётся через snprintf, а тот знак десятичной точки берёт из
 * действующей локали: описание TOML дозволяет точку и только её, и запись «0,1» ни
 * одним чтецом принята не будет. Подмена знака закрепляется здесь, а не полагается на
 * локаль испытательной среды
 *
 */
TEST(CodecTomlWriter, LocaleNumbers) {
	// Запоминаем действующую локаль записи чисел
	const string current(::setlocale(LC_NUMERIC, nullptr));
	// Количество проверенных локалей с иным десятичным знаком
	uint32_t checked = 0;
	/**
	 * Выполняем перебор названий локали с иным знаком десятичной точки
	 *
	 * @note Названия эти у разных систем свои: у POSIX - «de_DE.UTF-8», у MS Windows
	 *       - «German_Germany», и ни одно из них не признаётся всюду. Локали
	 *       «fa_IR» и «ar_SA» взяты особо: десятичным знаком там служит «٫»
	 *       (U+066B), занимающий в UTF-8 два байта, - замена одного лишь первого
	 *       байта оставляла бы от него обрубок
	 */
	for(const char * name : {"de_DE.UTF-8", "de_DE.utf8", "German_Germany.1252", "German_Germany", "fa_IR.UTF-8", "ar_SA.UTF-8"}){
		// Если установить очередную локаль не удалось
		if(::setlocale(LC_NUMERIC, name) == nullptr)
			// Выполняем переход к следующей локали
			continue;
		// Если знаком десятичной точки установленной локали точка всё же осталась
		if(::localeconv()->decimal_point[0] == '.')
			// Выполняем переход к следующей локали
			continue;
		// Объект записи текста настроек
		toml::writer_t writer(::logger());
		// Выполняем запись имени ключа пары
		ASSERT_TRUE(writer.key("k")) << name;
		// Выполняем запись числа с плавающей точкой
		ASSERT_TRUE(writer.real(0.1)) << name;
		// Выполняем запись имени ключа пары
		ASSERT_TRUE(writer.key("m")) << name;
		// Выполняем запись числа с большим количеством значащих разрядов
		ASSERT_TRUE(writer.real(2986.808299)) << name;
		// Выполняем запись имени ключа пары
		ASSERT_TRUE(writer.key("w")) << name;
		// Выполняем запись числа с плавающей точкой без дробной части
		ASSERT_TRUE(writer.real(1.0)) << name;
		// Выполняем проверку записи чисел с точкой в качестве десятичного знака
		ASSERT_EQ(writer.text(), "k = 0.1\nm = 2986.808299\nw = 1.0\n") << name;
		// Собранные события разбора значений
		vector <Scalar> events;
		// Выполняем проверку разбора собранного текста настроек
		ASSERT_EQ(::reread(writer.text(), events), toml::error_t::NONE) << name;
		// Выполняем проверку количества прочитанных значений
		ASSERT_EQ(events.size(), 3u) << name;
		// Выполняем проверку совпадения прочитанного числа с записанным
		ASSERT_EQ(events.at(0).real, 0.1) << name;
		// Выполняем учёт проверенной локали
		checked++;
	}
	// Выполняем возврат действующей локали записи чисел
	::setlocale(LC_NUMERIC, current.c_str());
	/**
	 * Если ни одной локали с иным десятичным знаком в системе не нашлось
	 *
	 * @note Пропуск здесь - СВОЙСТВО БИБЛИОТЕКИ, а не пробел проверки. Библиотека
	 *       musl локалей не несёт вовсе: `setlocale` принимает всякое имя, а
	 *       десятичным знаком неизменно остаётся точка, и проверка эта на musl
	 *       пропускается ВСЕГДА. Замер 01.09.2026, стенд OpenWrt 25.12 x86_64,
	 *       musl 1.2.5: оба пропуска, у настроек и у наречия TOML
	 *
	 * @warning Глухота записи чисел к локали на musl оттого и НЕ ПРОВЕРЕНА - ни этой
	 *          проверкою, ни какой другой: система не даёт завести условие, при каком
	 *          глухота эта что-либо значит. Зелёный прогон на musl обещания такого не
	 *          несёт, и принимать его за поверку нельзя
	 */
	if(checked == 0)
		// Выполняем пропуск проверки
		GTEST_SKIP() << "no locale with a foreign decimal point is available";
}
/**
 * @brief Проверка записи отметок времени
 *
 */
TEST(CodecTomlWriter, Stamps) {
	// Объект записи текста настроек
	toml::writer_t writer(::logger());
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
	toml::writer_t writer(::logger());
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
	toml::writer_t writer(::logger());
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
	toml::writer_t writer(::logger());
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
	toml::writer_t writer(::logger());
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
	toml::writer_t writer(::logger());
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
	toml::writer_t writer(::logger());
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
	toml::writer_t writer(::logger());
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
	toml::writer_t writer(::logger());
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
	/**
	 * Выполняем проверку отказа записи объявления таблицы при недописанной паре
	 */
	{
		// Объект записи текста настроек
		toml::writer_t writer(::logger());
		// Выполняем запись имени ключа пары
		ASSERT_TRUE(writer.key("value"));
		// Выполняем проверку отказа записи объявления таблицы
		ASSERT_FALSE(writer.table("server"));
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), toml::error_t::MISSING_VALUE);
		// Выполняем проверку отказа выдачи недописанного текста настроек
		ASSERT_TRUE(writer.text().empty());
	}
	/**
	 * Выполняем проверку отказа записи начала перечня значений вне пары
	 */
	{
		// Объект записи текста настроек
		toml::writer_t writer(::logger());
		// Выполняем запись имени ключа пары
		ASSERT_TRUE(writer.key("value"));
		// Выполняем запись значения пары
		ASSERT_TRUE(writer.integer(1));
		// Выполняем проверку выдачи собранного текста настроек
		ASSERT_EQ(writer.text(), "value = 1\n");
		// Выполняем запись начала перечня значений вне пары
		ASSERT_FALSE(writer.arrayOpen());
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), toml::error_t::UNEXPECTED_CONTENT);
	}
}
/**
 * @brief Проверка отказа закрытия незаписанного составного значения
 *
 */
TEST(CodecTomlWriter, Unbalanced) {
	/**
	 * Выполняем проверку отказа закрытия перечня скобкой встроенной таблицы
	 */
	{
		// Объект записи текста настроек
		toml::writer_t writer(::logger());
		// Выполняем запись имени ключа пары
		ASSERT_TRUE(writer.key("value"));
		// Выполняем запись начала перечня значений
		ASSERT_TRUE(writer.arrayOpen());
		// Выполняем проверку отказа закрытия перечня скобкой встроенной таблицы
		ASSERT_FALSE(writer.inlineClose());
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), toml::error_t::UNCLOSED_INLINE_TABLE);
	}
	/**
	 * Выполняем проверку отказа закрытия незаписанного перечня значений
	 */
	{
		// Объект записи текста настроек
		toml::writer_t writer(::logger());
		// Выполняем запись имени ключа пары
		ASSERT_TRUE(writer.key("value"));
		// Выполняем запись начала перечня значений
		ASSERT_TRUE(writer.arrayOpen());
		// Выполняем запись конца перечня значений
		ASSERT_TRUE(writer.arrayClose());
		// Выполняем проверку отказа закрытия незаписанного перечня значений
		ASSERT_FALSE(writer.arrayClose());
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), toml::error_t::UNCLOSED_ARRAY);
	}
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
	toml::writer_t writer(::logger(), settings);
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
	toml::writer_t writer(::logger(), settings);
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
		toml::reader_t reader(::logger(), reading);
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
		toml::writer_t writer(::logger(), writing);
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
	toml::writer_t writer(::logger(), settings);
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
	toml::writer_t writer(::logger(), settings);
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
	toml::writer_t writer(::logger());
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
	toml::reader_t reader(::logger());
	// Объект записи текста настроек
	toml::writer_t writer(::logger());
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
	toml::writer_t writer(::logger());
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
	toml::writer_t writer(::logger(), settings);
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
	toml::writer_t writer(::logger(), settings);
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
		toml::writer_t writer(::logger(), writing);
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
		toml::reader_t reader(::logger(), reading);
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
	toml::writer_t writer(::logger(), writing);
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("a"));
	// Выполняем проверку отказа записи перечня значений
	ASSERT_FALSE(writer.arrayOpen());
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(writer.error(), toml::error_t::DEPTH_EXCEEDED);
	// Объект записи текста настроек встроенной таблицы
	toml::writer_t inlined(::logger(), writing);
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
	toml::reader_t reader(::logger(), reading);
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
	toml::writer_t writer(::logger());
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
	toml::writer_t single(::logger());
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
	toml::writer_t outside(::logger());
	// Выполняем проверку отказа записи примечания перечня вне перечня
	ASSERT_FALSE(outside.remark("нельзя"));
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(outside.error(), toml::error_t::UNEXPECTED_CONTENT);
}
/**
 * @brief Проверка того, что отказ записи писателя заклинивает
 *
 * @details Договор этот у трёх кодеков сведён к одному решением владельца: отказ записи
 *          липкий - писатель, отказом задетый, дальнейших записей не принимает вовсе, а
 *          код отказа держит до сброса. Прежде кодеки расходились: у INI отказ прилипал,
 *          у TOML собранный текст метился рваным, а YAML запись продолжал
 *
 * @warning Перебираются ВСЕ методы записи открытого договора, а не выборка их: сторож
 *          стоит в каждом теле по отдельности, и метод, сторожа не получивший, выпал бы
 *          из договора молча. Заведётся у записи новый метод - приписать его сюда
 *
 */
TEST(CodecTomlWriter, RefusalLocksWriter) {
	// Объект записи текста настроек
	toml::writer_t writer(::logger());
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("value"));
	// Выполняем запись значения пары
	ASSERT_TRUE(writer.integer(1));
	// Запоминаем собранный текст настроек до отказа
	const string before = writer.text();
	// Выполняем проверку отказа дописки примечания со знаком конца строки
	ASSERT_FALSE(writer.trailing("первая\nвторая"));
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(writer.error(), toml::error_t::INVALID_CHARACTER);
	// Запоминаем код отказа записи
	const toml::error_t error = writer.error();
	// Отметка времени, к записи подаваемая
	toml::stamp_t stamp;
	// Путь к записываемому имени
	const vector <toml::part_t> parts = ::path({"имя"});
	/**
	 * Выполняем перебор всех методов записи открытого договора
	 */
	/**
	 * @note Утверждения ниже суть EXPECT, а не ASSERT, намеренно: ASSERT обрывает тело
	 *       проверки первым же отказом, и методы за ним не спрашивались бы вовсе
	 *
	 * @note Снятие всех двадцати одного сторожа роняет девять утверждений из двадцати
	 *       двух: остальные отвергаются и своими условиями, сторожа не спрашивая.
	 *       Замерено срывом
	 */
	EXPECT_FALSE(writer.table(parts));
	EXPECT_FALSE(writer.table("таблица"));
	EXPECT_FALSE(writer.arrayTable(parts));
	EXPECT_FALSE(writer.arrayTable("таблица"));
	EXPECT_FALSE(writer.key(parts));
	EXPECT_FALSE(writer.key("ключ"));
	EXPECT_FALSE(writer.value(toml::content_t{}));
	EXPECT_FALSE(writer.text("значение"));
	EXPECT_FALSE(writer.boolean(true));
	EXPECT_FALSE(writer.integer(1));
	EXPECT_FALSE(writer.real(1.5));
	EXPECT_FALSE(writer.stamp(stamp, toml::type_t::LOCAL_DATE));
	EXPECT_FALSE(writer.arrayOpen());
	EXPECT_FALSE(writer.arrayClose());
	EXPECT_FALSE(writer.inlineOpen());
	EXPECT_FALSE(writer.inlineClose());
	EXPECT_FALSE(writer.comment("примечание"));
	EXPECT_FALSE(writer.trailing("дописка"));
	EXPECT_FALSE(writer.remark("примечание перечня"));
	EXPECT_FALSE(writer.remarked("примечание перечня", true));
	EXPECT_FALSE(writer.blank());
	EXPECT_FALSE(writer.number("число", 1));
	// Выполняем проверку того, что код отказа заклинившим писателем сохранён
	ASSERT_EQ(writer.error(), error);
	// Выполняем проверку того, что собранный текст отказом не тронут
	ASSERT_EQ(writer.text(), before);
	// Выполняем сброс записи в исходное состояние
	writer.clear();
	// Выполняем проверку того, что сброс код отказа отпускает
	ASSERT_EQ(writer.error(), toml::error_t::NONE);
	// Выполняем проверку того, что запись после сброса принимается
	ASSERT_TRUE(writer.key("ключ") && writer.integer(1));
}
/**
 * @brief Проверка отказа выдачи текста, отказом оборванного
 *
 * @details Отказ, случившийся после того, как операция уже дописала начало своё,
 * оставляет строку оборванной: имя таблицы, отвергнутое длиной, оставляет за собою
 * открывающую скобку, за которой имени уже не будет. Выдать такой текст значило бы
 * выдать текст, который собственный разбор целым не признаёт
 *
 */
TEST(CodecTomlWriter, TornRefusal) {
	// Настройки записи текста настроек
	toml::writer_t::settings_t settings;
	// Устанавливаем наибольшую допустимую длину имени ключа
	settings.maxKey = 3;
	// Объект записи текста настроек
	toml::writer_t writer(::logger(), settings);
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("ab"));
	// Выполняем запись значения пары
	ASSERT_TRUE(writer.integer(1));
	// Выполняем проверку выдачи собранного текста настроек
	ASSERT_EQ(writer.text(), "ab = 1\n");
	// Выполняем проверку отказа записи объявления таблицы сверх предела длины имени
	ASSERT_FALSE(writer.table("длинное"));
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(writer.error(), toml::error_t::KEY_TOO_LONG);
	/**
	 * Выполняем проверку отказа выдачи оборванного текста настроек
	 *
	 * @note Без проверки выдавался бы текст с открывающей скобкой на конце
	 */
	ASSERT_TRUE(writer.text().empty());
	/**
	 * Выполняем проверку того, что отказ писателя заклинил
	 *
	 * @note Прежде запись после отказа продолжалась, и удачная пустая строка дописывала
	 *       знак конца строки к оборванной: по одному лишь виду собранного текста
	 *       рваность эта становилась неразличима. Ныне писатель дальнейших записей не
	 *       принимает вовсе, и случай этот отпал
	 */
	ASSERT_FALSE(writer.blank());
	// Выполняем проверку того, что код отказа заклинившим писателем сохранён
	ASSERT_EQ(writer.error(), toml::error_t::KEY_TOO_LONG);
	// Выполняем проверку того, что выдача текста остаётся пустой
	ASSERT_TRUE(writer.text().empty());
	// Выполняем сброс записи в исходное состояние
	writer.clear();
	// Выполняем проверку того, что сброс снимает и признак рваности, и заклинивание
	ASSERT_TRUE(writer.key("ab") && writer.integer(1));
	// Выполняем проверку выдачи собранного заново текста настроек
	ASSERT_EQ(writer.text(), "ab = 1\n");
}
/**
 * @brief Проверка того, что отказ записи оборванного текста не выдаёт
 *
 * @details Проверка TornRefusal поверяет одно место отказа, а мест этих у записи
 *          десятки. Здесь поверяется правило целиком: всякий отказ обязан оставить
 *          выдачу либо нетронутой, либо пустой. Текст, дописанный до половины и
 *          выданный как есть, от целого глазом неотличим, а разбором уже не читается
 *
 * @note Проверка утверждает и то, что отказы вообще случились: набор доводов,
 *       ни одного отказа не давший, прошёл бы её молча, ничего не поверив
 *
 */
TEST(CodecTomlWriter, RefusalNeverYieldsTornText) {
	// Настройки записи, отказы делающие достижимыми
	toml::writer_t::settings_t settings;
	// Устанавливаем наибольшую допустимую длину имени ключа
	settings.maxKey = 6;
	// Устанавливаем наибольшую допустимую длину строки
	settings.maxLine = 32;
	// Устанавливаем наибольшую допустимую глубину вложенности
	settings.maxDepth = 2;
	// Устанавливаем наибольшее допустимое количество долей имени
	settings.maxParts = 2;
	// Количество отказов, набором доводов полученных
	size_t refused = 0;
	/**
	 * @brief Описание поверяемого вызова записи
	 *
	 */
	struct probe_t {
		// Название поверяемого вызова
		const char * name;
		// Тело, поверяемый вызов совершающее
		bool (* call)(toml::writer_t & writer) noexcept;
	};
	// Набор поверяемых вызовов записи
	static const probe_t PROBES[] = {
		{"таблица с длинным именем", [](toml::writer_t & writer) noexcept -> bool {
			return writer.table("оченьдлинноеимя");
		}},
		{"таблица с долями сверх предела", [](toml::writer_t & writer) noexcept -> bool {
			return writer.table("a.b.c.d");
		}},
		{"перечень таблиц с длинным именем", [](toml::writer_t & writer) noexcept -> bool {
			return writer.arrayTable("оченьдлинноеимя");
		}},
		{"ключ с длинным именем", [](toml::writer_t & writer) noexcept -> bool {
			return writer.key("оченьдлинноеимя");
		}},
		{"ключ с долями сверх предела", [](toml::writer_t & writer) noexcept -> bool {
			return writer.key("a.b.c.d");
		}},
		{"значение сверх предела длины строки", [](toml::writer_t & writer) noexcept -> bool {
			return writer.key("k") && writer.text(string(64, 'x'));
		}},
		{"примечание с переводом строки", [](toml::writer_t & writer) noexcept -> bool {
			return writer.remark("первая\nвторая");
		}}
	};
	/**
	 * Выполняем перебор всех поверяемых вызовов записи
	 */
	for(const probe_t & probe : PROBES){
		// Объект записи текста настроек
		toml::writer_t writer(::logger(), settings);
		// Выполняем запись имени ключа пары
		ASSERT_TRUE(writer.key("k")) << probe.name;
		// Выполняем запись значения пары
		ASSERT_TRUE(writer.integer(1)) << probe.name;
		// Собранный текст до поверяемого вызова
		const string before(writer.text());
		// Выполняем проверку непустоты собранного текста
		ASSERT_FALSE(before.empty()) << probe.name;
		/**
		 * Если поверяемый вызов записи отказал
		 */
		if(!probe.call(writer)){
			// Выполняем учёт полученного отказа
			refused++;
			// Собранный текст после поверяемого вызова
			const string after(writer.text());
			// Выполняем проверку того, что выдача либо нетронута, либо пуста
			ASSERT_TRUE(after.empty() || (after == before)) << probe.name << ": [" << after << "]";
			// Выполняем проверку того, что отказ назвал свою причину
			ASSERT_NE(writer.error(), toml::error_t::NONE) << probe.name;
		}
	}
	// Выполняем проверку того, что набор доводов отказы вообще давал
	ASSERT_GE(refused, 5u);
}
/**
 * @brief Проверка того, что знак конца строки за оградой ставится лишь по нужде
 *
 * @details Многострочное значение, содержимое которого начато знаком конца строки,
 * иначе не записать: разбор первый перевод за открывающей оградой отбрасывает. Всякому
 * же прочему значению знак этот не нужен, и постановка его растягивает запись на строку
 * лишнюю
 *
 * @note Закрепляются ОБЕ половины правила: постановка по нужде и непостановка без неё.
 *       Одна половина без другой прошла бы и при прежнем поведении, знак ставившем
 *       всегда
 *
 */
TEST(CodecTomlWriter, MultilineOpeningNewline) {
	/**
	 * Выполняем проверку непостановки знака у значения, им не начатого
	 */
	{
		// Объект записи текста настроек
		toml::writer_t writer(::logger());
		// Выполняем запись имени ключа пары
		ASSERT_TRUE(writer.key("k"));
		// Выполняем запись многострочного строкового значения
		ASSERT_TRUE(writer.text("одна строка", toml::string_t::MULTILINE_BASIC));
		// Выполняем проверку того, что знак конца строки за оградой не поставлен
		ASSERT_EQ(writer.text(), "k = \"\"\"одна строка\"\"\"\n");
	}
	/**
	 * Выполняем проверку постановки знака у значения, им начатого
	 */
	{
		// Объект записи текста настроек
		toml::writer_t writer(::logger());
		// Выполняем запись имени ключа пары
		ASSERT_TRUE(writer.key("k"));
		// Выполняем запись многострочного строкового значения
		ASSERT_TRUE(writer.text("\nодна строка", toml::string_t::MULTILINE_BASIC));
		/**
		 * Выполняем проверку того, что знак конца строки за оградой поставлен дважды
		 *
		 * @note Первый отбрасывается разбором, второй есть первый перевод содержимого:
		 *       без постановки значение прочиталось бы обратно без него
		 */
		ASSERT_EQ(writer.text(), "k = \"\"\"\n\nодна строка\"\"\"\n");
	}
	/**
	 * Выполняем проверку устойчивости перезаписи перечня с многострочным значением
	 *
	 * @note Ровно это и ломалось: перечень, человеком написанный одною строкой,
	 *       растягивался записью и при обратном чтении становился многострочным
	 */
	{
		// Собираемое дерево настроек
		toml::document_t document(::logger());
		// Выполняем проверку успешности разбора перечня с многострочным значением
		ASSERT_TRUE(document.parse("k = [ \"один\", \"\"\"два\"\"\" ]\n"));
		// Первая перезапись дерева настроек
		const string first = document.text();
		// Собираемое дерево настроек перезаписи
		toml::document_t back(::logger());
		// Выполняем проверку успешности разбора перезаписи
		ASSERT_TRUE(back.parse(first));
		// Выполняем проверку устойчивости перезаписи
		ASSERT_EQ(back.text(), first);
	}
}
/**
 * @brief Проверка отказов записи отметки времени построением ошибочным
 *
 * @details Отметка времени есть вид языка первого разряда, и запись её обязана
 *          отвергать построение, какого читающий не примет: час свыше суток, дату,
 *          календарю не отвечающую, отсутствующее смещение пояса у отметки со
 *          смещением. Записав такое, кодек собрал бы текст, самим же собою отвергаемый
 *
 * @note Заходы эти пересечение трёх прогонов числило непройденными ничем: отметки
 *       приходили лишь разбором текста, а разбор ошибочных построений отсеивает сам,
 *       и до записи они не доходили вовсе
 *
 */
TEST(CodecTomlWriter, StampRefusesMalformed) {
	/**
	 * @brief Описание проверяемого захода отказа
	 *
	 */
	struct broken_t {
		// Пояснение проверяемого захода
		const char * note;
		// Отметка времени, записываемая заходом
		toml::stamp_t stamp;
		// Вид записываемой отметки времени
		toml::type_t type;
	};
	// Собираемый набор проверяемых заходов отказа
	vector <broken_t> broken;
	/**
	 * Выполняем сборку захода с часом свыше суток
	 */
	{
		// Отметка времени захода
		toml::stamp_t stamp;
		// Устанавливаем час свыше суток
		stamp.time.hour = 24;
		// Добавляем заход в набор проверяемых
		broken.push_back({"час 24", stamp, toml::type_t::LOCAL_TIME});
	}
	/**
	 * Выполняем сборку захода с минутой свыше часа
	 */
	{
		// Отметка времени захода
		toml::stamp_t stamp;
		// Устанавливаем час годный
		stamp.time.hour = 1;
		// Устанавливаем минуту свыше часа
		stamp.time.minute = 60;
		// Добавляем заход в набор проверяемых
		broken.push_back({"минута 60", stamp, toml::type_t::LOCAL_TIME});
	}
	/**
	 * Выполняем сборку захода с долей секунды разрядами свыше предела
	 */
	{
		// Отметка времени захода
		toml::stamp_t stamp;
		// Устанавливаем время годное
		stamp.time.hour = 1;
		// Устанавливаем разряды доли секунды свыше предела
		stamp.time.digits = static_cast <uint8_t> (toml::MAX_FRACTION + 1);
		// Добавляем заход в набор проверяемых
		broken.push_back({"разрядов доли свыше предела", stamp, toml::type_t::LOCAL_TIME});
	}
	/**
	 * Выполняем сборку захода с датой, календарю не отвечающей
	 */
	{
		// Отметка времени захода
		toml::stamp_t stamp;
		// Устанавливаем год годный
		stamp.date.year = 2026;
		// Устанавливаем месяц годный
		stamp.date.month = 2;
		// Устанавливаем день, февралю не отвечающий
		stamp.date.day = 31;
		// Добавляем заход в набор проверяемых
		broken.push_back({"31 февраля", stamp, toml::type_t::LOCAL_DATE});
	}
	/**
	 * Выполняем сборку захода со смещением пояса отсутствующим
	 */
	{
		// Отметка времени захода
		toml::stamp_t stamp;
		// Устанавливаем дату годную
		stamp.date.year = 1979;
		// Устанавливаем месяц годный
		stamp.date.month = 5;
		// Устанавливаем день годный
		stamp.date.day = 27;
		// Добавляем заход в набор проверяемых
		broken.push_back({"смещение пояса отсутствует", stamp, toml::type_t::OFFSET_DATETIME});
	}
	/**
	 * Выполняем сборку захода со смещением пояса свыше суток
	 */
	{
		// Отметка времени захода
		toml::stamp_t stamp;
		// Устанавливаем дату годную
		stamp.date.year = 1979;
		// Устанавливаем месяц годный
		stamp.date.month = 5;
		// Устанавливаем день годный
		stamp.date.day = 27;
		// Устанавливаем смещение пояса свыше суток
		stamp.offset = (24 * 60);
		// Добавляем заход в набор проверяемых
		broken.push_back({"смещение пояса 24 часа", stamp, toml::type_t::OFFSET_DATETIME});
	}
	/**
	 * Выполняем сборку захода с видом, отметкой времени не являющимся
	 */
	{
		// Отметка времени захода
		toml::stamp_t stamp;
		// Устанавливаем дату годную
		stamp.date.year = 1979;
		// Устанавливаем месяц годный
		stamp.date.month = 5;
		// Устанавливаем день годный
		stamp.date.day = 27;
		// Добавляем заход в набор проверяемых
		broken.push_back({"вид не отметка", stamp, toml::type_t::STRING});
	}
	/**
	 * Выполняем перебор проверяемых заходов отказа
	 */
	for(auto & item : broken){
		// Объект записи текста настроек
		toml::writer_t writer(::logger());
		// Выполняем проверку успешности записи имени пары
		ASSERT_TRUE(writer.key("к")) << item.note;
		// Выполняем проверку отказа записи отметки времени
		ASSERT_FALSE(writer.stamp(item.stamp, item.type)) << item.note;
		// Выполняем проверку того, что отказ назван причиною своею
		ASSERT_EQ(writer.error(), toml::error_t::INVALID_DATETIME) << item.note;
	}
	/**
	 * Выполняем проверку того, что отметка годная записывается
	 *
	 * @note Половина эта обязательна: запись, отвергающая ВСЯКУЮ отметку, прошла бы все
	 *       заходы выше, не записав ни одной
	 */
	{
		// Отметка времени, записи подлежащая
		toml::stamp_t stamp;
		// Устанавливаем год отметки
		stamp.date.year = 1979;
		// Устанавливаем месяц отметки
		stamp.date.month = 5;
		// Устанавливаем день отметки
		stamp.date.day = 27;
		// Устанавливаем час отметки
		stamp.time.hour = 7;
		// Устанавливаем минуту отметки
		stamp.time.minute = 32;
		// Устанавливаем смещение пояса отметки
		stamp.offset = 0;
		// Устанавливаем запись часового пояса знаком «Z»
		stamp.zulu = true;
		// Объект записи текста настроек
		toml::writer_t writer(::logger());
		// Выполняем проверку успешности записи имени пары
		ASSERT_TRUE(writer.key("к"));
		// Выполняем проверку успешности записи отметки времени
		ASSERT_TRUE(writer.stamp(stamp, toml::type_t::OFFSET_DATETIME)) << toml::message(writer.error());
		// Выполняем проверку записанного текста настроек
		ASSERT_EQ(writer.text(), "\"к\" = 1979-05-27T07:32:00Z\n");
	}
}
/**
 * @brief Проверка обхода забоя, перевода страницы и дословного многострочного значения
 *
 * @details Запись основной строки обязана обходить забой и перевод страницы своими
 *          последовательностями: языку они принадлежат наравне с переводом строки, а
 *          записанные знаками, вышли бы управляющими знаками в тексте, читающим
 *          отвергаемом. Дословное же многострочное значение, переводом строки начатое,
 *          обязано нести знак конца строки за оградой открывающей: разбор первый
 *          отбрасывает, и без постановки значение прочиталось бы обратно без него
 *
 * @note Заходы эти пересечение двух прогонов числило непройденными: ворошитель забоя
 *       и перевода страницы не порождает, а многострочное дословное значение доходило
 *       до записи лишь без перевода строки начального
 *
 */
TEST(CodecTomlWriter, BackspaceFormfeedAndLiteralOpeningNewline) {
	/**
	 * Выполняем проверку обхода забоя записью основной строки
	 */
	{
		// Объект записи текста настроек
		toml::writer_t writer(::logger());
		// Выполняем запись имени ключа пары
		ASSERT_TRUE(writer.key("k"));
		// Выполняем запись строкового значения с забоем
		ASSERT_TRUE(writer.text("a\bb", toml::string_t::BASIC));
		// Выполняем проверку обхода забоя последовательностью его
		ASSERT_EQ(writer.text(), "k = \"a\\bb\"\n");
	}
	/**
	 * Выполняем проверку обхода перевода страницы записью основной строки
	 */
	{
		// Объект записи текста настроек
		toml::writer_t writer(::logger());
		// Выполняем запись имени ключа пары
		ASSERT_TRUE(writer.key("k"));
		// Выполняем запись строкового значения с переводом страницы
		ASSERT_TRUE(writer.text("a\fb", toml::string_t::BASIC));
		// Выполняем проверку обхода перевода страницы последовательностью его
		ASSERT_EQ(writer.text(), "k = \"a\\fb\"\n");
	}
	/**
	 * Выполняем проверку постановки знака у дословного значения, им начатого
	 */
	{
		// Объект записи текста настроек
		toml::writer_t writer(::logger());
		// Выполняем запись имени ключа пары
		ASSERT_TRUE(writer.key("k"));
		// Выполняем запись многострочного дословного значения
		ASSERT_TRUE(writer.text("\nодна строка", toml::string_t::MULTILINE_LITERAL));
		// Выполняем проверку того, что знак конца строки за оградой поставлен дважды
		ASSERT_EQ(writer.text(), "k = '''\n\nодна строка'''\n");
	}
	/**
	 * Выполняем проверку непостановки знака у дословного значения, им не начатого
	 */
	{
		// Объект записи текста настроек
		toml::writer_t writer(::logger());
		// Выполняем запись имени ключа пары
		ASSERT_TRUE(writer.key("k"));
		// Выполняем запись многострочного дословного значения
		ASSERT_TRUE(writer.text("одна строка", toml::string_t::MULTILINE_LITERAL));
		// Выполняем проверку того, что знак конца строки за оградой не поставлен
		ASSERT_EQ(writer.text(), "k = '''одна строка'''\n");
	}
}
/**
 * @brief Проверка того, что дописка судит о написанном, а не о настройке
 *
 * @details Дописка примечания снимает знак конца строки с собранного текста, и снимать
 * она обязана ровно ТО, ЧТО БЫЛО ЗАПИСАНО: настройка разметки строк вправе смениться
 * между записью строки и допиской к ней
 *
 * @warning Замерено щупом до правки: при смене CRLF на LF дописка снимала один перевод
 *          строки из двух знаков и оставляла одинокий возврат каретки ПОСРЕДИ строки -
 *          `k = 1[CR] # хвост[LF]`, - отвечая при том успехом
 *
 * @note Довод «о написанном нельзя судить по настройке» принесён Василием от кодека
 *       JSON. Беда эта у INI и TOML одна, и закреплена она у обоих
 *
 */
TEST(CodecTomlWriter, TrailingJudgesWrittenNotSettings) {
	// Настройки записи текста настроек
	toml::writer_t::settings_t settings;
	// Устанавливаем разметку строк возвратом каретки с переводом
	settings.newline = toml::newline_t::CRLF;
	// Объект записи текста настроек
	toml::writer_t writer(::logger(), settings);
	// Выполняем запись имени ключа пары
	ASSERT_TRUE(writer.key("k"));
	// Выполняем запись значения пары
	ASSERT_TRUE(writer.integer(1));
	// Выполняем смену разметки строк на перевод строки
	settings.newline = toml::newline_t::LF;
	// Выполняем установку сменённых настроек записи
	writer.settings(settings);
	// Выполняем дописку примечания к записанной строке
	ASSERT_TRUE(writer.trailing("хвост"));
	// Выполняем проверку того, что одинокого возврата каретки посреди строки нет
	ASSERT_EQ(writer.text(), string("k = 1 # хвост\n"));
}
/**
 * @brief Проверка отказа записи строк текста при недописанной паре
 *
 * @details Имя пары, записанное без значения, места очередной строке текста не
 *          оставляет: примечание, дописка примечания и пустая строка обязаны
 *          отвергаться кодом «пропущенное значение». Пересечение прогонов числило
 *          слепыми все три строки распространения этого отказа - задета была лишь
 *          выдача собранного текста
 *
 * @warning Объявления таблицы тут НЕТ: его отказ равнозначен и без сторожа
 *          готовности - имя таблицы отвергается поверкой имени следом, тем же кодом
 *          и с тем же откатом текста; порча сторожа проверки не роняет - замерено
 *
 */
TEST(CodecTomlWriter, LineRefusedOnPendingKey) {
	// Выполняем перебор записываемых строк текста
	for(const auto & sample : { string("примечание"), string("дописка"), string("пустая строка") }){
		// Объект записи текста настроек
		toml::writer_t writer(::logger());
		// Выполняем запись имени ключа пары
		ASSERT_TRUE(writer.key("value")) << sample;
		/**
		 * Если записывается примечание
		 */
		if(sample == "примечание")
			// Выполняем проверку отказа записи примечания
			ASSERT_FALSE(writer.comment("текст")) << sample;
		/**
		 * Если примечание дописывается к записанной строке
		 */
		else if(sample == "дописка")
			// Выполняем проверку отказа дописки примечания
			ASSERT_FALSE(writer.trailing("текст")) << sample;
		// Выполняем проверку отказа записи пустой строки
		else ASSERT_FALSE(writer.blank()) << sample;
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), toml::error_t::MISSING_VALUE) << sample;
		/**
		 * Выполняем проверку того, что отказ писателя заклинил
		 *
		 * @note Прежде отвергнутая строка проверялась дописыванием значения следом:
		 *       текст выдавался целым, и тем доказывалось, что отказ его не испортил.
		 *       Ныне писатель после отказа записей не принимает, и целость текста
		 *       судится пустой его выдачей вместе с сохранённым кодом отказа
		 */
		ASSERT_FALSE(writer.integer(1)) << sample;
		// Выполняем проверку того, что код отказа заклинившим писателем сохранён
		ASSERT_EQ(writer.error(), toml::error_t::MISSING_VALUE) << sample;
		// Выполняем проверку того, что недописанный текст выдаче не подлежит
		ASSERT_TRUE(writer.text().empty()) << sample;
	}
}
/**
 * @brief Проверка отказов записи, покрытием не задетых
 *
 * @details Заходы эти пересечение трёх прогонов числило слепыми: набор проверок,
 * ворошитель и корпус наречий не подавали ни примечания перечня со знаком конца
 * строки, ни пустого составного имени ключа, ни значения составного типа. Отказ,
 * ни разу не исполненный, обещания своего не даёт вовсе
 *
 * @note Перечень и таблица значениями не записываются по устройству: собираются
 *       они парой вызовов открытия и закрытия, и подача их значением означает
 *       не содержимое, а неверный порядок вызовов
 *
 */
TEST(CodecTomlWriter, RefusalsNotCoveredBefore) {
	/**
	 * Выполняем проверку отказа примечания перечня со знаком конца строки
	 */
	{
		// Объект записи текста настроек
		toml::writer_t writer(::logger());
		// Выполняем запись имени ключа пары
		ASSERT_TRUE(writer.key("a"));
		// Выполняем запись начала многострочного перечня значений
		ASSERT_TRUE(writer.arrayOpen(true));
		// Выполняем проверку отказа записи примечания со знаком конца строки
		ASSERT_FALSE(writer.remark("первая\nвторая"));
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), toml::error_t::INVALID_CHARACTER);
	}
	/**
	 * Выполняем проверку отказа примечания перечня с управляющим знаком
	 */
	{
		// Объект записи текста настроек
		toml::writer_t writer(::logger());
		// Выполняем запись имени ключа пары
		ASSERT_TRUE(writer.key("a"));
		// Выполняем запись начала многострочного перечня значений
		ASSERT_TRUE(writer.arrayOpen(true));
		// Выполняем проверку отказа записи примечания с управляющим знаком
		ASSERT_FALSE(writer.remark(string("до\x01после")));
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), toml::error_t::INVALID_CHARACTER);
	}
	/**
	 * Выполняем проверку отказа записи пустого составного имени ключа
	 */
	{
		// Объект записи текста настроек
		toml::writer_t writer(::logger());
		// Пустое составное имя ключа
		const vector <toml::part_t> path;
		// Выполняем проверку отказа записи пустого составного имени ключа
		ASSERT_FALSE(writer.key(path));
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), toml::error_t::EMPTY_KEY);
	}
	/**
	 * Выполняем проверку отказа записи значения составного типа
	 */
	{
		// Объект записи текста настроек
		toml::writer_t writer(::logger());
		// Записываемое значение пары
		toml::content_t content;
		// Запоминаем составной тип записываемого значения
		content.type = toml::type_t::ARRAY;
		// Выполняем запись имени ключа пары
		ASSERT_TRUE(writer.key("a"));
		// Выполняем проверку отказа записи значения составного типа
		ASSERT_FALSE(writer.value(content));
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), toml::error_t::INVALID_VALUE);
	}
}
/**
 * @brief Проверка записи имени ключа со знаками Юникода
 *
 * @details Знаки Юникода в имени ключа без кавычек отводит лишь черновик следующей
 * версии описания, и признаются они настройкою - по умолчанию отключённой. Ветвь
 * настройки включённой покрытием не задевалась вовсе, а вместе с нею оставался
 * непроверенным и разбор имени по знакам: годится ли знак имени голому
 *
 * @note Знак, имени не годящийся, взят знаком-рисунком: буква кириллицы черновиком
 *       дозволена, и подмена поверки её пропуском осталась бы незамечена
 *
 */
TEST(CodecTomlWriter, UnicodeBareName) {
	// Объект записи текста настроек
	toml::writer_t writer(::logger());
	// Получаем текущие настройки записи текста настроек
	toml::writer_t::settings_t settings = writer.settings();
	// Запоминаем дозволение знаков Юникода в именах ключей без кавычек
	settings.unicode = true;
	// Выполняем установку настроек записи текста настроек
	writer.settings(settings);
	// Выполняем запись имени ключа пары буквами кириллицы
	ASSERT_TRUE(writer.key("ключ"));
	// Выполняем запись значения пары
	ASSERT_TRUE(writer.integer(1));
	// Выполняем проверку того, что имя ключа записано без кавычек
	ASSERT_EQ(writer.text(), string("ключ = 1\n"));
	// Объект записи текста настроек с именем ключа знаком-рисунком
	toml::writer_t drawn(::logger());
	// Выполняем установку настроек записи текста настроек
	drawn.settings(settings);
	// Выполняем запись имени ключа пары со знаком-рисунком
	ASSERT_TRUE(drawn.key("сне\xE2\x98\x83г"));
	// Выполняем запись значения пары
	ASSERT_TRUE(drawn.integer(1));
	// Выполняем проверку того, что имя ключа записано в кавычках
	ASSERT_EQ(drawn.text(), string("\"сне\xE2\x98\x83г\" = 1\n"));
	// Объект записи текста настроек с настройками по умолчанию
	toml::writer_t plain(::logger());
	// Выполняем запись имени ключа пары буквами кириллицы
	ASSERT_TRUE(plain.key("ключ"));
	// Выполняем запись значения пары
	ASSERT_TRUE(plain.integer(1));
	/**
	 * Выполняем проверку того, что настройкою отключённой то же имя пишется в кавычках
	 */
	ASSERT_EQ(plain.text(), string("\"ключ\" = 1\n"));
}
/**
 * @brief Проверка записи примечания несколькими строками
 *
 * @details Примечание вправе нести знаки конца строки: записывается оно тогда
 * несколькими строками примечания подряд. Разметка CRLF отбрасывается построчно -
 * возврат каретки, в строку попавший, вышел бы у примечания управляющим знаком
 *
 * @note Предел длины строки проверяется здесь же: отказ его приходит на записи
 *       знака конца строки очередной строки примечания, а не на поверке содержимого
 *
 */
TEST(CodecTomlWriter, MultilineComment) {
	/**
	 * Выполняем проверку записи примечания, разметку CRLF несущего
	 */
	{
		// Объект записи текста настроек
		toml::writer_t writer(::logger());
		// Выполняем запись примечания несколькими строками
		ASSERT_TRUE(writer.comment("первая\r\nвторая"));
		// Выполняем проверку того, что возврат каретки отброшен построчно
		ASSERT_EQ(writer.text(), string("# первая\n# вторая\n"));
	}
	/**
	 * Выполняем проверку отказа примечания с управляющим знаком
	 */
	{
		// Объект записи текста настроек
		toml::writer_t writer(::logger());
		// Выполняем проверку отказа записи примечания с управляющим знаком
		ASSERT_FALSE(writer.comment(string("до\x01после")));
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), toml::error_t::INVALID_CHARACTER);
	}
	/**
	 * Выполняем проверку отказа примечания, предел длины строки превысившего
	 */
	{
		// Объект записи текста настроек
		toml::writer_t writer(::logger());
		// Получаем текущие настройки записи текста настроек
		toml::writer_t::settings_t settings = writer.settings();
		// Запоминаем предел длины строки записываемого текста настроек
		settings.maxLine = 8;
		// Выполняем установку настроек записи текста настроек
		writer.settings(settings);
		// Выполняем проверку отказа записи примечания сверх предела длины строки
		ASSERT_FALSE(writer.comment("первая строка примечания"));
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), toml::error_t::LINE_TOO_LONG);
	}
}
/**
 * @brief Проверка записи пары с целым числом без знака
 *
 * @details Число без знака записывается тем же телом, что и число со знаком, но
 * приводится к нему через поверку предела: описание отводит целому числу
 * шестьдесят четыре разряда СО ЗНАКОМ. Ветвь приведения покрытием не задевалась -
 * проверки чисел подавали лишь числа со знаком да число сверх предела
 *
 */
TEST(CodecTomlWriter, UnsignedNumber) {
	// Объект записи текста настроек
	toml::writer_t writer(::logger());
	// Выполняем запись пары с целым числом без знака
	ASSERT_TRUE(writer.number("a", static_cast <uint32_t> (42)));
	// Выполняем проверку собранного текста настроек
	ASSERT_EQ(writer.text(), string("a = 42\n"));
}
