/**
 * @file: writer.cpp
 * @date: 2026-08-09
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Автоматические тесты записи текста настроек INI — сборка текста по наречиям,
 *        ограждение значений кавычками и управляющими последовательностями, отклонение
 *        неправильного построения имён и обратный ход «запись - чтение»
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <clocale>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/ini/ini.hpp>

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
 * @brief Проверка записи текста настроек с настройками по умолчанию
 *
 */
TEST(CodecIniWriter, Default) {
	// Объект записи текста настроек
	ini::writer_t writer;
	// Выполняем запись примечания
	ASSERT_TRUE(writer.comment("собрано автоматически"));
	// Выполняем запись объявления раздела
	ASSERT_TRUE(writer.section("server"));
	// Выполняем запись свойства с обозначением узла
	ASSERT_TRUE(writer.property("host", "127.0.0.1"));
	// Выполняем запись свойства с номером порта
	ASSERT_TRUE(writer.number("port", 8080));
	// Выполняем проверку собранного текста настроек
	ASSERT_EQ(writer.text(), "; собрано автоматически\n[server]\nhost = 127.0.0.1\nport = 8080\n");
}
/**
 * @brief Проверка записи текста настроек наречия MS Windows
 *
 */
TEST(CodecIniWriter, Windows) {
	// Объект записи текста настроек
	ini::writer_t writer(ini::writer_t::settings_t::windows());
	// Выполняем запись объявления раздела
	ASSERT_TRUE(writer.section("paths"));
	/**
	 * Выполняем запись свойства с точкой с запятой внутри значения
	 *
	 * @note Наречие это примечания в конце строки не признаёт, и ограждения такое
	 *       значение не требует
	 */
	ASSERT_TRUE(writer.property("PATH", "c:\\bin;c:\\sbin"));
	// Выполняем проверку собранного текста настроек
	ASSERT_EQ(writer.text(), "[paths]\r\nPATH=c:\\bin;c:\\sbin\r\n");
}
/**
 * @brief Проверка записи текста настроек наречия настроек Git
 *
 */
TEST(CodecIniWriter, Git) {
	// Объект записи текста настроек
	ini::writer_t writer(ini::writer_t::settings_t::git());
	// Выполняем запись объявления раздела с подразделом
	ASSERT_TRUE(writer.section("remote", "origin"));
	// Выполняем запись свойства с обозначением источника
	ASSERT_TRUE(writer.property("url", "git@host:repo.git"));
	// Выполняем запись объявления раздела
	ASSERT_TRUE(writer.section("core"));
	// Выполняем запись свойства со знаком примечания внутри значения
	ASSERT_TRUE(writer.property("path", "a;b"));
	/**
	 * Выполняем проверку собранного текста настроек
	 *
	 * @note Точка с запятой в значении ограждается кавычками, хотя примечания это
	 *       наречие пишет решёткой: читает оно оба знака, и неограждённое значение
	 *       при обратном чтении обрезалось бы
	 */
	ASSERT_EQ(writer.text(), "[remote \"origin\"]\n\turl = git@host:repo.git\n\n[core]\n\tpath = \"a;b\"\n");
}
/**
 * @brief Проверка ограждения значения кавычками
 *
 */
TEST(CodecIniWriter, Quoting) {
	// Объект записи текста настроек
	ini::writer_t writer;
	// Выполняем запись объявления раздела
	ASSERT_TRUE(writer.section("a"));
	// Выполняем запись свойства с пробельной обвязкой значения
	ASSERT_TRUE(writer.property("padded", " value "));
	// Выполняем запись свойства без нужды в ограждении значения
	ASSERT_TRUE(writer.property("plain", "value"));
	// Выполняем проверку собранного текста настроек
	ASSERT_EQ(writer.text(), "[a]\npadded = \" value \"\nplain = value\n");
}
/**
 * @brief Проверка записи значения управляющими последовательностями
 *
 */
TEST(CodecIniWriter, Escapes) {
	// Собираемые настройки записи текста настроек
	ini::writer_t::settings_t settings;
	// Устанавливаем запись управляющих последовательностей в значении
	settings.escapes = true;
	// Объект записи текста настроек
	ini::writer_t writer(settings);
	// Выполняем запись объявления раздела
	ASSERT_TRUE(writer.section("a"));
	// Выполняем запись свойства со знаком конца строки в значении
	ASSERT_TRUE(writer.property("multi", "first\nsecond"));
	// Выполняем проверку собранного текста настроек
	ASSERT_EQ(writer.text(), "[a]\nmulti = first\\nsecond\n");
}
/**
 * @brief Проверка отклонения неправильного построения записи
 *
 */
TEST(CodecIniWriter, Malformed) {
	{
		// Объект записи текста настроек
		ini::writer_t writer;
		// Выполняем проверку отклонения пустого имени раздела
		ASSERT_FALSE(writer.section(""));
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), ini::error_t::EMPTY_SECTION);
	}{
		// Объект записи текста настроек
		ini::writer_t writer;
		// Выполняем проверку отклонения квадратной скобки в имени раздела
		ASSERT_FALSE(writer.section("a]b"));
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), ini::error_t::INVALID_SECTION);
	}{
		// Объект записи текста настроек
		ini::writer_t writer;
		// Выполняем проверку отклонения разделителя в имени свойства
		ASSERT_FALSE(writer.property("a=b", "value"));
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), ini::error_t::INVALID_KEY);
	}{
		// Объект записи текста настроек
		ini::writer_t writer;
		/**
		 * Выполняем проверку отклонения знака конца строки в значении
		 *
		 * @note Записать такое значение нечем: управляющие последовательности
		 *       настройками не разрешены, а ограждение кавычками знак конца строки
		 *       не спасает - строка на нём обрывается
		 */
		ASSERT_FALSE(writer.property("key", "first\nsecond"));
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), ini::error_t::INVALID_CHARACTER);
	}{
		// Объект записи текста настроек
		ini::writer_t writer;
		// Выполняем проверку отклонения имени подраздела без заданного его построения
		ASSERT_FALSE(writer.section("a", "b"));
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), ini::error_t::INVALID_SUBSECTION);
	}
}
/**
 * @brief Проверка обратного хода «запись - чтение»
 *
 * @details Собранный текст обязан читаться обратно в то же самое: расхождение здесь
 * означает, что запись выдала текст, который разбор понимает иначе, чем он собран
 *
 */
TEST(CodecIniWriter, Roundtrip) {
	// Перечень записываемых значений свойств
	const string values[] = {
		"простое значение", "  обвязка  ", "a;b", "a#b", "", "\"кавычки\"", "путь\\к\\файлу", "a=b", "10.25"
	};
	// Собираемые настройки записи текста настроек
	ini::writer_t::settings_t settings = ini::writer_t::settings_t::git();
	// Объект записи текста настроек
	ini::writer_t writer(settings);
	// Выполняем запись объявления раздела с подразделом
	ASSERT_TRUE(writer.section("раздел", "под\"раздел"));
	/**
	 * Выполняем перебор всех записываемых значений свойств
	 */
	for(size_t i = 0; i < (sizeof(values) / sizeof(values[0])); i++)
		// Выполняем запись очередного свойства
		ASSERT_TRUE(writer.property(string("key").append(to_string(i)), values[i])) << "value " << i;
	// Собираемые настройки разбора текста настроек
	ini::reader_t::settings_t parsing = ini::reader_t::settings_t::git();
	// Объект потокового чтения текста настроек
	ini::reader_t reader(parsing);
	// Выполняем передачу собранного текста настроек
	ASSERT_TRUE(reader.feed(writer.text()));
	// Выполняем переход к объявлению раздела
	ASSERT_TRUE(reader.next());
	// Выполняем проверку имени прочитанного раздела
	ASSERT_TRUE(reader.section().is("раздел", "под\"раздел"));
	/**
	 * Выполняем перебор всех записанных свойств
	 */
	for(size_t i = 0; i < (sizeof(values) / sizeof(values[0])); i++){
		// Выполняем переход к очередному свойству
		ASSERT_TRUE(reader.next()) << "value " << i;
		// Выполняем проверку имени прочитанного свойства
		ASSERT_EQ(reader.key(), string("key").append(to_string(i))) << "value " << i;
		// Выполняем проверку значения прочитанного свойства
		ASSERT_EQ(reader.text(), values[i]) << "value " << i;
	}
	// Выполняем проверку исчерпания событий разбора
	ASSERT_FALSE(reader.next());
	// Выполняем проверку завершения разбора текста настроек
	ASSERT_EQ(reader.state(), ini::state_t::FINISHED);
}
/**
 * @brief Проверка сброса записи в исходное состояние
 *
 */
TEST(CodecIniWriter, Clear) {
	// Объект записи текста настроек
	ini::writer_t writer;
	// Выполняем проверку отклонения пустого имени раздела
	ASSERT_FALSE(writer.section(""));
	// Выполняем сброс записи в исходное состояние
	writer.clear();
	// Выполняем проверку сброса кода ошибки записи
	ASSERT_EQ(writer.error(), ini::error_t::NONE);
	// Выполняем запись объявления раздела
	ASSERT_TRUE(writer.section("a"));
	// Выполняем проверку собранного текста настроек
	ASSERT_EQ(writer.text(), "[a]\n");
}
/**
 * @brief Проверка отклонения знака-разделителя в имени раздела
 *
 */
TEST(CodecIniWriter, DelimitedSection) {
	// Собираемые настройки записи текста настроек
	ini::writer_t::settings_t settings;
	// Устанавливаем построение имени подраздела разделителем
	settings.subsections = ini::subsection_t::DELIMITED;
	// Объект записи текста настроек
	ini::writer_t writer(settings);
	/**
	 * Выполняем проверку отклонения знака-разделителя в имени раздела
	 *
	 * @note Запись «[a.b.c]» читающий разобрал бы разделом «a» с подразделом «b.c»,
	 *       а не разделом «a.b» с подразделом «c»
	 */
	ASSERT_FALSE(writer.section("a.b", "c"));
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(writer.error(), ini::error_t::INVALID_SECTION);
	// Выполняем очистку собранного текста настроек
	writer.clear();
	// Выполняем проверку прохождения записи обычного имени раздела
	ASSERT_TRUE(writer.section("a", "b.c"));
	// Выполняем проверку собранного текста настроек
	ASSERT_EQ(writer.text(), "[a.b.c]\n");
}
/**
 * @brief Проверка отката собранного текста при отказе записи
 *
 * @note Отказ, оставляющий в собранном тексте начатую запись, портит его молча:
 *       текст этот выглядит собранным, а несёт обрубок
 *
 */
TEST(CodecIniWriter, Rollback) {
	{
		// Собираемые настройки записи текста настроек
		ini::writer_t::settings_t settings;
		// Устанавливаем построение имени подраздела разделителем
		settings.subsections = ini::subsection_t::DELIMITED;
		// Объект записи текста настроек
		ini::writer_t writer(settings);
		// Выполняем проверку отклонения недопустимого имени подраздела
		ASSERT_FALSE(writer.section("a", "b]c"));
		// Выполняем проверку отсутствия хвоста в собранном тексте
		ASSERT_TRUE(writer.text().empty());
	}{
		// Объект записи текста настроек
		ini::writer_t writer;
		// Выполняем запись объявления раздела
		ASSERT_TRUE(writer.section("a"));
		// Выполняем проверку отклонения значения со знаком конца строки
		ASSERT_FALSE(writer.property("k", "one\ntwo"));
		// Выполняем проверку отсутствия хвоста в собранном тексте
		ASSERT_EQ(writer.text(), "[a]\n");
	}{
		// Собираемые настройки записи текста настроек
		ini::writer_t::settings_t settings;
		// Устанавливаем построение имени подраздела разделителем
		settings.subsections = ini::subsection_t::DELIMITED;
		// Объект записи текста настроек
		ini::writer_t writer(settings);
		// Выполняем запись объявления раздела
		ASSERT_TRUE(writer.section("a"));
		// Выполняем запись свойства со значением
		ASSERT_TRUE(writer.property("k", "v"));
		// Получаем собранный текст настроек до отказа
		const string before = writer.text();
		// Выполняем проверку отклонения недопустимого имени подраздела
		ASSERT_FALSE(writer.section("b", "c]d"));
		/**
		 * Выполняем проверку неизменности собранного текста
		 *
		 * @note Пустая строка, отделяющая раздел от предыдущего, откатывается вместе
		 *       с ним: объявления, которое она отделяет, не вышло
		 */
		ASSERT_EQ(writer.text(), before);
	}
}
/**
 * @brief Проверка отклонения знака конца строки в примечании конца строки
 *
 * @note Примечание это дописывается к готовой строке, и знак конца строки в нём
 *       разорвал бы её надвое
 *
 */
TEST(CodecIniWriter, TrailingNewline) {
	// Объект записи текста настроек
	ini::writer_t writer;
	// Выполняем запись свойства со значением
	ASSERT_TRUE(writer.property("k", "v"));
	// Выполняем проверку отклонения примечания со знаком конца строки
	ASSERT_FALSE(writer.trailing("одна\nдве"));
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(writer.error(), ini::error_t::INVALID_CHARACTER);
	// Выполняем проверку неизменности собранного текста
	ASSERT_EQ(writer.text(), "k = v\n");
}
/**
 * @brief Проверка записи чисел с плавающей точкой кратчайшей записью
 *
 * @note Наибольшая точность оборот переживает, но выдаёт «0.1» как
 *       «0.10000000000000001» - в файле, писанном для человека, это нечитаемо
 *
 */
TEST(CodecIniWriter, ShortestNumbers) {
	// Объект записи текста настроек
	ini::writer_t writer;
	// Выполняем запись числа с плавающей точкой двойной точности
	ASSERT_TRUE(writer.number("a", 0.1));
	// Выполняем запись числа с плавающей точкой одинарной точности
	ASSERT_TRUE(writer.number("b", 0.1f));
	// Выполняем запись целого значения числом с плавающей точкой
	ASSERT_TRUE(writer.number("c", 2.0));
	// Выполняем проверку собранного текста настроек
	ASSERT_EQ(writer.text(), "a = 0.1\nb = 0.1\nc = 2\n");
	// Дерево настроек обратного чтения
	ini::document_t document;
	// Выполняем разбор записанного текста настроек
	ASSERT_TRUE(document.parse(writer.text()));
	// Прочитанное обратно значение
	double value = 0;
	// Выполняем разбор значения свойства числом
	ASSERT_TRUE(document.value(value, "a"));
	// Выполняем проверку сохранности числа при обороте
	ASSERT_EQ(value, 0.1);
}
/**
 * @brief Проверка записи многострочного значения продолжением отступом
 *
 */
TEST(CodecIniWriter, IndentedValue) {
	// Собираемые настройки записи текста настроек
	ini::writer_t::settings_t settings;
	// Устанавливаем запись многострочного значения продолжением отступом
	settings.indents = true;
	// Объект записи текста настроек
	ini::writer_t writer(settings);
	// Выполняем запись свойства с многострочным значением
	ASSERT_TRUE(writer.property("k", "one\ntwo"));
	// Выполняем проверку собранного текста настроек
	ASSERT_EQ(writer.text(), "k = one\n\ttwo\n");
	{
		// Объект записи текста настроек
		ini::writer_t writer;
		/**
		 * Выполняем проверку отклонения многострочного значения без продолжений
		 *
		 * @note Записать такое значение в одну строку нечем
		 */
		ASSERT_FALSE(writer.property("k", "one\ntwo"));
	}
}

/**
 * @brief Проверка отказа записи имени и значения, склеивающих строки при чтении
 *
 */
TEST(CodecIniWriter, TrailingBackslash) {
	// Объект записи текста настроек наречия Git
	ini::writer_t writer(ini::writer_t::settings_t::git());
	// Выполняем проверку отказа записи имени раздела с обратной косой чертой в конце
	ASSERT_FALSE(writer.section("a\\"));
	// Выполняем проверку кода ошибки записи имени раздела
	ASSERT_EQ(writer.error(), ini::error_t::INVALID_SECTION);
	// Выполняем сброс собранного текста настроек
	writer.clear();
	// Выполняем запись объявления раздела
	ASSERT_TRUE(writer.section("a"));
	// Выполняем проверку отказа записи имени свойства с обратной косой чертой в конце
	ASSERT_FALSE(writer.property("k\\", "v"));
	// Выполняем проверку кода ошибки записи имени свойства
	ASSERT_EQ(writer.error(), ini::error_t::INVALID_KEY);
	// Выполняем сброс собранного текста настроек
	writer.clear();
	// Выполняем запись объявления раздела
	ASSERT_TRUE(writer.section("a"));
	// Выполняем проверку записи имени с удвоенной обратной косой чертой в конце
	ASSERT_TRUE(writer.property("k\\\\", "v"));
	// Собираемые настройки записи текста настроек без управляющих последовательностей
	ini::writer_t::settings_t settings;
	// Устанавливаем склеивание строк, продолженных обратной косой чертой, читающим
	settings.continuations = true;
	// Объект записи текста настроек без управляющих последовательностей
	ini::writer_t plain(settings);
	// Выполняем запись объявления раздела
	ASSERT_TRUE(plain.section("a"));
	// Выполняем проверку отказа записи значения с обратной косой чертой в конце
	ASSERT_FALSE(plain.property("k", "v\\"));
	// Выполняем проверку кода ошибки записи значения свойства
	ASSERT_EQ(plain.error(), ini::error_t::INVALID_CHARACTER);
}

/**
 * @brief Проверка записи числа с плавающей точкой под чужой локалью
 *
 */
TEST(CodecIniWriter, LocaleNumbers) {
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
		ini::writer_t writer;
		// Выполняем запись объявления раздела
		ASSERT_TRUE(writer.section("s")) << name;
		// Выполняем запись числа с плавающей точкой
		ASSERT_TRUE(writer.number <double> ("k", 0.1)) << name;
		// Выполняем запись числа с большим количеством значащих разрядов
		ASSERT_TRUE(writer.number <double> ("m", 2986.808299)) << name;
		// Выполняем проверку записи чисел с точкой в качестве десятичного знака
		ASSERT_EQ(writer.text(), "[s]\nk = 0.1\nm = 2986.808299\n") << name;
		// Выполняем учёт проверенной локали
		checked++;
	}
	// Выполняем возврат действующей локали записи чисел
	::setlocale(LC_NUMERIC, current.c_str());
	// Если ни одной локали с иным десятичным знаком в системе не нашлось
	if(checked == 0)
		// Выполняем пропуск проверки
		GTEST_SKIP() << "no locale with a foreign decimal point is available";
}

/**
 * @brief Проверка записи чисел всех поддерживаемых видов
 *
 */
TEST(CodecIniWriter, Numbers) {
	// Объект записи текста настроек
	ini::writer_t writer;
	// Выполняем запись объявления раздела
	ASSERT_TRUE(writer.section("s"));
	// Выполняем запись логического значения
	ASSERT_TRUE(writer.number <bool> ("a", true));
	// Выполняем запись знакового однобайтового целого
	ASSERT_TRUE(writer.number <int8_t> ("b", -128));
	// Выполняем запись беззнакового однобайтового целого
	ASSERT_TRUE(writer.number <uint8_t> ("c", 255));
	// Выполняем запись знакового двухбайтового целого
	ASSERT_TRUE(writer.number <int16_t> ("d", -32768));
	// Выполняем запись беззнакового двухбайтового целого
	ASSERT_TRUE(writer.number <uint16_t> ("e", 65535));
	// Выполняем запись знакового четырёхбайтового целого
	ASSERT_TRUE(writer.number <int32_t> ("f", -2147483647 - 1));
	// Выполняем запись беззнакового четырёхбайтового целого
	ASSERT_TRUE(writer.number <uint32_t> ("g", 4294967295U));
	// Выполняем запись знакового восьмибайтового целого
	ASSERT_TRUE(writer.number <int64_t> ("h", -9007199254740993LL));
	// Выполняем запись беззнакового восьмибайтового целого
	ASSERT_TRUE(writer.number <uint64_t> ("i", 18446744073709551615ULL));
	// Выполняем запись числа с плавающей точкой одинарной точности
	ASSERT_TRUE(writer.number <float> ("j", 0.5f));
	// Выполняем запись числа с плавающей точкой двойной точности
	ASSERT_TRUE(writer.number <double> ("k", -1250.));
	// Выполняем проверку записанного текста настроек
	ASSERT_EQ(writer.text(),
		"[s]\na = true\nb = -128\nc = 255\nd = -32768\ne = 65535\n"
		"f = -2147483648\ng = 4294967295\nh = -9007199254740993\n"
		"i = 18446744073709551615\nj = 0.5\nk = -1250\n"
	);
}

/**
 * @brief Проверка наречий записи языка Python и системы инициализации systemd
 *
 */
TEST(CodecIniWriter, Presets) {
	// Объект записи текста настроек наречия языка Python
	ini::writer_t python(ini::writer_t::settings_t::python());
	// Выполняем запись объявления раздела
	ASSERT_TRUE(python.section("s"));
	// Выполняем запись многострочного значения продолжением отступом
	ASSERT_TRUE(python.property("k", "one\ntwo"));
	// Объект чтения текста настроек наречия языка Python
	ini::reader_t reader(ini::reader_t::settings_t::python());
	// Выполняем передачу записанного текста настроек
	ASSERT_TRUE(reader.feed(python.text()));
	// Выполняем переход к объявлению раздела
	ASSERT_TRUE(reader.next());
	// Выполняем переход к свойству раздела
	ASSERT_TRUE(reader.next());
	// Выполняем проверку прочитанного многострочного значения
	ASSERT_EQ(reader.property().value, "one\ntwo");
	// Объект записи текста настроек наречия системы инициализации systemd
	ini::writer_t systemd(ini::writer_t::settings_t::systemd());
	// Выполняем запись объявления раздела
	ASSERT_TRUE(systemd.section("Unit"));
	// Выполняем запись свойства раздела
	ASSERT_TRUE(systemd.property("Description", "value"));
	// Выполняем проверку отказа записи имени, склеивающего строки при чтении
	ASSERT_FALSE(systemd.property("Exec\\", "value"));
	// Выполняем проверку записанного текста настроек
	ASSERT_EQ(systemd.text(), "[Unit]\nDescription=value\n");
}

/**
 * @brief Проверка выбора между обычным и показательным видом записи числа
 *
 */
TEST(CodecIniWriter, PlainNumbers) {
	// Объект записи текста настроек
	ini::writer_t writer;
	// Выполняем запись объявления раздела
	ASSERT_TRUE(writer.section("s"));
	// Выполняем запись числа, у которого обычный вид короче показательного
	ASSERT_TRUE(writer.number <double> ("a", 1250.));
	// Выполняем запись числа, у которого показательный вид короче обычного
	ASSERT_TRUE(writer.number <double> ("b", 1e20));
	// Выполняем запись числа, обычный вид которого невозможен
	ASSERT_TRUE(writer.number <double> ("c", 1e300));
	// Выполняем запись малого числа с показательным видом записи
	ASSERT_TRUE(writer.number <double> ("d", 1e-7));
	// Выполняем проверку записанного текста настроек
	ASSERT_EQ(writer.text(), "[s]\na = 1250\nb = 1e+20\nc = 1e+300\nd = 1e-07\n");
	// Объект дерева настроек для проверки обратного чтения
	ini::document_t document;
	// Выполняем разбор записанного текста настроек
	ASSERT_TRUE(document.parse(writer.text()));
	// Прочитанное обратно значение числа
	double value = 0.;
	// Выполняем чтение записанного показательным видом числа
	ASSERT_TRUE(document.value(value, "b", "s"));
	// Выполняем проверку прочитанного обратно значения
	ASSERT_DOUBLE_EQ(value, 1e20);
}
