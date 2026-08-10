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
