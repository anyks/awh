/**
 * @file: document.cpp
 * @date: 2026-08-10
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Автоматические тесты дерева настроек INI — поиск по разделам и свойствам,
 *        перечни повторных значений, подстановка обращений, правка на месте с сохранением
 *        оформления и обратный ход «текст - дерево - текст»
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
 * @brief Проверка разбора текста настроек в дерево
 *
 */
TEST(CodecIniDocument, Parse) {
	// Дерево настроек
	ini::document_t document;
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse("; шапка\n[server]\nhost = 127.0.0.1\nport = 8080\n\n[client]\nretries = 3\n"));
	// Выполняем проверку количества объявленных разделов
	ASSERT_EQ(document.size(), 2u);
	// Выполняем проверку наличия объявленного раздела
	ASSERT_TRUE(document.section("server"));
	// Выполняем проверку отсутствия необъявленного раздела
	ASSERT_FALSE(document.section("proxy"));
	// Выполняем проверку значения свойства
	ASSERT_EQ(document.get("host", "server"), "127.0.0.1");
	// Значение номера порта
	uint16_t port = 0;
	// Выполняем разбор значения свойства числом
	ASSERT_TRUE(document.value(port, "port", "server"));
	// Выполняем проверку разобранного значения свойства
	ASSERT_EQ(port, 8080);
	// Выполняем проверку значения свойства чужого раздела
	ASSERT_EQ(document.get("retries", "client"), "3");
	// Выполняем проверку отсутствия свойства в чужом разделе
	ASSERT_FALSE(document.has("host", "client"));
	// Получаем перечень объявленных разделов
	const vector <ini::name_t> sections = document.sections();
	// Выполняем проверку количества объявленных разделов
	ASSERT_EQ(sections.size(), 2u);
	// Выполняем проверку имени первого объявленного раздела
	ASSERT_TRUE(sections.front().is("server"));
	// Выполняем проверку имени последнего объявленного раздела
	ASSERT_TRUE(sections.back().is("client"));
	// Получаем перечень имён свойств раздела
	const vector <string_view> keys = document.keys("server");
	// Выполняем проверку количества имён свойств раздела
	ASSERT_EQ(keys.size(), 2u);
	// Выполняем проверку имени первого свойства раздела
	ASSERT_EQ(keys.front(), "host");
}
/**
 * @brief Проверка обращения со свойствами до первого раздела
 *
 */
TEST(CodecIniDocument, Global) {
	// Дерево настроек
	ini::document_t document;
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse("name = значение\n[a]\nname = другое\n"));
	// Выполняем проверку значения свойства раздела без имени
	ASSERT_EQ(document.get("name"), "значение");
	// Выполняем проверку значения свойства объявленного раздела
	ASSERT_EQ(document.get("name", "a"), "другое");
	/**
	 * Выполняем проверку того, что раздел без имени объявленным не считается
	 */
	ASSERT_EQ(document.size(), 1u);
}
/**
 * @brief Проверка обращения с повторным объявлением свойства
 *
 */
TEST(CodecIniDocument, Duplicates) {
	{
		// Собираемые настройки дерева настроек
		ini::document_t::settings_t settings;
		// Устанавливаем обращение с повторным объявлением свойства
		settings.reader.duplicates = ini::duplicate_t::FIRST;
		// Дерево настроек
		ini::document_t document;
		// Выполняем разбор текста настроек
		ASSERT_TRUE(document.parse("[a]\nk = первое\nk = второе\n", settings));
		// Выполняем проверку выдачи первого объявления свойства
		ASSERT_EQ(document.get("k", "a"), "первое");
	}{
		// Собираемые настройки дерева настроек
		ini::document_t::settings_t settings;
		// Устанавливаем обращение с повторным объявлением свойства
		settings.reader.duplicates = ini::duplicate_t::LAST;
		// Дерево настроек
		ini::document_t document;
		// Выполняем разбор текста настроек
		ASSERT_TRUE(document.parse("[a]\nk = первое\nk = второе\n", settings));
		// Выполняем проверку выдачи последнего объявления свойства
		ASSERT_EQ(document.get("k", "a"), "второе");
	}{
		// Дерево настроек
		ini::document_t document;
		// Выполняем разбор текста настроек
		ASSERT_TRUE(document.parse("[a]\nk = первое\nk = второе\n"));
		// Получаем перечень значений свойства
		const vector <string_view> values = document.values("k", "a");
		// Выполняем проверку количества значений свойства
		ASSERT_EQ(values.size(), 2u);
		// Выполняем проверку первого значения свойства
		ASSERT_EQ(values.front(), "первое");
		// Выполняем проверку последнего значения свойства
		ASSERT_EQ(values.back(), "второе");
	}
}
/**
 * @brief Проверка продолжения раздела повторным его объявлением
 *
 */
TEST(CodecIniDocument, Continued) {
	// Дерево настроек
	ini::document_t document;
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse("[a]\nfirst = 1\n[b]\nx = 0\n[a]\nsecond = 2\n"));
	// Выполняем проверку количества объявленных разделов
	ASSERT_EQ(document.size(), 2u);
	// Выполняем проверку значения свойства первого объявления раздела
	ASSERT_EQ(document.get("first", "a"), "1");
	/**
	 * Выполняем проверку значения свойства повторного объявления раздела
	 *
	 * @note Повторное объявление раздела заводит не новый раздел, а продолжение
	 *       прежнего: свойства обоих объявлений принадлежат одному разделу
	 */
	ASSERT_EQ(document.get("second", "a"), "2");
}
/**
 * @brief Проверка подстановки обращений к значениям других свойств
 *
 */
TEST(CodecIniDocument, References) {
	{
		// Собираемые настройки дерева настроек
		ini::document_t::settings_t settings;
		// Устанавливаем построение обращения к значению другого свойства
		settings.references = ini::reference_t::SHELL;
		// Дерево настроек
		ini::document_t document;
		// Выполняем разбор текста настроек
		ASSERT_TRUE(document.parse("[paths]\nroot = /opt/awh\nlogs = ${root}/logs\n[run]\npid = ${paths:root}/run.pid\n", settings));
		// Выполняем проверку подстановки обращения к значению своего раздела
		ASSERT_EQ(document.get("logs", "paths"), "/opt/awh/logs");
		// Выполняем проверку подстановки обращения к значению чужого раздела
		ASSERT_EQ(document.get("pid", "run"), "/opt/awh/run.pid");
	}{
		// Собираемые настройки дерева настроек
		ini::document_t::settings_t settings;
		// Устанавливаем построение обращения по образцу configparser
		settings.references = ini::reference_t::PYTHON;
		// Дерево настроек
		ini::document_t document;
		// Выполняем разбор текста настроек
		ASSERT_TRUE(document.parse("[a]\nname = awh\ngreeting = привет, %(name)s\nescaped = 50%%\n", settings));
		// Выполняем проверку подстановки обращения к значению
		ASSERT_EQ(document.get("greeting", "a"), "привет, awh");
		// Выполняем проверку записи знака обращения удвоением
		ASSERT_EQ(document.get("escaped", "a"), "50%");
	}{
		// Собираемые настройки дерева настроек
		ini::document_t::settings_t settings;
		// Устанавливаем построение обращения к значению другого свойства
		settings.references = ini::reference_t::SHELL;
		// Дерево настроек
		ini::document_t document;
		// Выполняем проверку отклонения круговой ссылки
		ASSERT_FALSE(document.parse("[a]\nfirst = ${second}\nsecond = ${first}\n", settings));
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(document.error(), ini::error_t::RECURSIVE_REFERENCE);
		// Выполняем проверку отклонения обращения к необъявленному значению
		ASSERT_FALSE(document.parse("[a]\nk = ${missing}\n", settings));
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(document.error(), ini::error_t::UNKNOWN_REFERENCE);
	}{
		// Дерево настроек
		ini::document_t document;
		/**
		 * Выполняем проверку того, что без настройки обращения не подставляются
		 */
		ASSERT_TRUE(document.parse("[a]\nroot = /opt\nlogs = ${root}/logs\n"));
		// Выполняем проверку сохранения обращения в значении
		ASSERT_EQ(document.get("logs", "a"), "${root}/logs");
	}
}
/**
 * @brief Проверка соблюдения предела объёма подстановки
 *
 */
TEST(CodecIniDocument, Expansion) {
	// Собираемые настройки дерева настроек
	ini::document_t::settings_t settings;
	// Устанавливаем построение обращения к значению другого свойства
	settings.references = ini::reference_t::SHELL;
	// Устанавливаем предел объёма подстановки значений
	settings.maxExpansion = 64;
	// Дерево настроек
	ini::document_t document;
	/**
	 * Выполняем проверку отклонения многократного разрастания значения
	 *
	 * @note Каждое следующее значение удваивает предыдущее: пять таких шагов
	 *       наращивают объём в тридцать два раза, и предел прекращает разбор
	 */
	ASSERT_FALSE(document.parse("[a]\nb0 = ЗначениеЗначение\nb1 = ${b0}${b0}\nb2 = ${b1}${b1}\nb3 = ${b2}${b2}\nb4 = ${b3}${b3}\n", settings));
	// Выполняем проверку кода ошибки разбора
	ASSERT_EQ(document.error(), ini::error_t::EXPANSION_EXCEEDED);
}
/**
 * @brief Проверка обратного хода «текст - дерево - текст»
 *
 * @details Текст, разобранный в дерево и записанный обратно, обязан совпасть с
 * исходным знак в знак: расхождение здесь означает потерю оформления
 *
 */
TEST(CodecIniDocument, Roundtrip) {
	// Разбираемый текст настроек
	const string text =
		"; шапка файла\n"
		"; вторая строка шапки\n"
		"\n"
		"[server]\n"
		"host = 127.0.0.1\n"
		"; примечание к порту\n"
		"port = 8080\n"
		"\n"
		"[client]\n"
		"retries = 3\n";
	// Дерево настроек
	ini::document_t document;
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(text));
	// Выполняем проверку совпадения записанного текста с исходным
	ASSERT_EQ(document.text(), text);
}
/**
 * @brief Проверка обратного хода с примечаниями в конце строк
 *
 */
TEST(CodecIniDocument, RoundtripInline) {
	// Разбираемый текст настроек
	const string text =
		"[core] ; примечание раздела\n"
		"\tpath = value ; примечание свойства\n"
		"\tbare\n";
	// Собираемые настройки дерева настроек
	ini::document_t::settings_t settings;
	// Устанавливаем настройки разбора наречия настроек Git
	settings.reader = ini::reader_t::settings_t::git();
	// Дерево настроек
	ini::document_t document;
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(text, settings));
	// Выполняем проверку значения свойства
	ASSERT_EQ(document.get("path", "core"), "value");
	// Собираемые настройки записи текста настроек
	ini::writer_t::settings_t options = ini::writer_t::settings_t::git();
	// Устанавливаем знак начала примечания
	options.marker = ';';
	// Выполняем проверку совпадения записанного текста с исходным
	ASSERT_EQ(document.text(options), text);
}
/**
 * @brief Проверка сохранения обращения к значению при обратной записи
 *
 * @details Разрешённое обращение при обратной записи в файл не попадает: записано
 * оно ради того, чтобы разрешаться заново при каждом чтении, и подстановка его
 * навсегда лишила бы файл настроек этой возможности
 *
 */
TEST(CodecIniDocument, RoundtripReferences) {
	// Разбираемый текст настроек
	const string text = "[paths]\nroot = /opt/awh\nlogs = ${root}/logs\n";
	// Собираемые настройки дерева настроек
	ini::document_t::settings_t settings;
	// Устанавливаем построение обращения к значению другого свойства
	settings.references = ini::reference_t::SHELL;
	// Дерево настроек
	ini::document_t document;
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(text, settings));
	// Выполняем проверку подстановки обращения при чтении значения
	ASSERT_EQ(document.get("logs", "paths"), "/opt/awh/logs");
	// Выполняем проверку сохранения обращения при обратной записи
	ASSERT_EQ(document.text(), text);
}
/**
 * @brief Проверка сохранения знака примечания при обратной записи
 *
 */
TEST(CodecIniDocument, RoundtripMarkers) {
	// Разбираемый текст настроек
	const string text = "# решётка\n; точка с запятой\n[a]\nk = v\n";
	// Дерево настроек
	ini::document_t document;
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(text));
	/**
	 * Выполняем проверку сохранения знака примечания каждой записи
	 *
	 * @note Знак берётся из исходного текста, а не из настроек записи: выбрал
	 *       его тот, кто файл настроек писал
	 */
	ASSERT_EQ(document.text(), text);
}
/**
 * @brief Проверка правки значения свойства на месте
 *
 */
TEST(CodecIniDocument, Set) {
	// Разбираемый текст настроек
	const string text = "; шапка\n[server]\nhost = 127.0.0.1\nport = 8080\n";
	// Дерево настроек
	ini::document_t document;
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(text));
	// Выполняем правку значения объявленного свойства
	ASSERT_TRUE(document.set("port", "9090", "server"));
	// Выполняем проверку правленого значения свойства
	ASSERT_EQ(document.get("port", "server"), "9090");
	/**
	 * Выполняем проверку сохранения оформления при правке
	 */
	ASSERT_EQ(document.text(), "; шапка\n[server]\nhost = 127.0.0.1\nport = 9090\n");
	// Выполняем добавление свойства в объявленный раздел
	ASSERT_TRUE(document.set("timeout", "30", "server"));
	// Выполняем проверку добавленного значения свойства
	ASSERT_EQ(document.get("timeout", "server"), "30");
	// Выполняем проверку записи добавленного свойства в конец раздела
	ASSERT_EQ(document.text(), "; шапка\n[server]\nhost = 127.0.0.1\nport = 9090\ntimeout = 30\n");
	// Выполняем добавление свойства в необъявленный раздел
	ASSERT_TRUE(document.set("level", "debug", "logs"));
	// Выполняем проверку количества объявленных разделов
	ASSERT_EQ(document.size(), 2u);
	// Выполняем проверку записи объявленного раздела
	ASSERT_EQ(document.text(), "; шапка\n[server]\nhost = 127.0.0.1\nport = 9090\ntimeout = 30\n\n[logs]\nlevel = debug\n");
}
/**
 * @brief Проверка удаления свойства и раздела
 *
 */
TEST(CodecIniDocument, Erase) {
	// Дерево настроек
	ini::document_t document;
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse("[a]\nfirst = 1\nsecond = 2\n\n[b]\nthird = 3\n"));
	// Выполняем удаление свойства раздела
	ASSERT_TRUE(document.erase("first", "a"));
	// Выполняем проверку отсутствия удалённого свойства
	ASSERT_FALSE(document.has("first", "a"));
	// Выполняем проверку записи дерева без удалённого свойства
	ASSERT_EQ(document.text(), "[a]\nsecond = 2\n\n[b]\nthird = 3\n");
	// Выполняем проверку отклонения удаления необъявленного свойства
	ASSERT_FALSE(document.erase("missing", "a"));
	// Выполняем удаление раздела
	ASSERT_TRUE(document.remove("a"));
	// Выполняем проверку отсутствия удалённого раздела
	ASSERT_FALSE(document.section("a"));
	// Выполняем проверку количества объявленных разделов
	ASSERT_EQ(document.size(), 1u);
	// Выполняем проверку записи дерева без удалённого раздела
	ASSERT_EQ(document.text(), "[b]\nthird = 3\n");
}
/**
 * @brief Проверка объявления раздела
 *
 */
TEST(CodecIniDocument, Create) {
	// Дерево настроек
	ini::document_t document;
	// Выполняем объявление раздела
	ASSERT_TRUE(document.create("first"));
	// Выполняем повторное объявление того же раздела
	ASSERT_TRUE(document.create("first"));
	// Выполняем проверку количества объявленных разделов
	ASSERT_EQ(document.size(), 1u);
	// Выполняем объявление второго раздела
	ASSERT_TRUE(document.create("second"));
	// Выполняем проверку записи объявленных разделов
	ASSERT_EQ(document.text(), "[first]\n\n[second]\n");
	// Выполняем проверку отклонения объявления раздела с пустым именем
	ASSERT_FALSE(document.create(""));
}
/**
 * @brief Проверка сборки дерева вызовами установки значения
 *
 * @note Указатели поиска при добавлении записи в конец перечня наращиваются, а
 *       не перестраиваются целиком. Проверка стережёт согласие приращаемого
 *       пути с перестроением: вставка в середину, правка после удаления и
 *       порядок выдачи имён свойств
 *
 */
TEST(CodecIniDocument, Assembly) {
	// Дерево настроек
	ini::document_t document;
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse("[a]\nfirst = 1\n\n[b]\nthird = 3\n"));
	/**
	 * Выполняем добавление свойства в раздел, последним не являющийся
	 *
	 * @note Записи прочих разделов при такой вставке сдвигаются, и указатели
	 *       поиска обязаны быть перестроены целиком
	 */
	ASSERT_TRUE(document.set("second", "2", "a"));
	// Выполняем проверку записи добавленного свойства в конец своего раздела
	ASSERT_EQ(document.text(), "[a]\nfirst = 1\nsecond = 2\n\n[b]\nthird = 3\n");
	// Выполняем проверку доступности свойств сдвинутого раздела
	ASSERT_EQ(document.get("third", "b"), "3");
	// Выполняем проверку порядка выдачи имён свойств раздела
	ASSERT_EQ(document.keys("a"), (vector <string_view> {"first", "second"}));
	// Выполняем удаление первого свойства раздела
	ASSERT_TRUE(document.erase("first", "a"));
	// Выполняем проверку порядка выдачи имён свойств после удаления
	ASSERT_EQ(document.keys("a"), (vector <string_view> {"second"}));
	/**
	 * Выполняем повторное заведение удалённого свойства
	 *
	 * @note Указатель свойства при удалении изъят, и заведение обязано пройти
	 *       путём нового свойства, а не правки записи, снятой пометкой удаления
	 */
	ASSERT_TRUE(document.set("first", "10", "a"));
	// Выполняем проверку заведённого значения свойства
	ASSERT_EQ(document.get("first", "a"), "10");
	// Выполняем проверку порядка выдачи имён свойств после заведения
	ASSERT_EQ(document.keys("a"), (vector <string_view> {"second", "first"}));
	// Выполняем проверку записи дерева после правок
	ASSERT_EQ(document.text(), "[a]\nsecond = 2\nfirst = 10\n\n[b]\nthird = 3\n");
	// Выполняем сборку раздела вызовами установки значения
	for(uint32_t i = 0; i < 64; i++)
		// Выполняем добавление свойства в заводимый раздел
		ASSERT_TRUE(document.set(("key" + std::to_string(i)), std::to_string(i), "bulk"));
	// Выполняем проверку количества собранных свойств раздела
	ASSERT_EQ(document.keys("bulk").size(), 64u);
	// Выполняем проверку последнего собранного значения
	ASSERT_EQ(document.get("key63", "bulk"), "63");
	// Выполняем проверку порядка выдачи первого и последнего имён
	ASSERT_EQ(document.keys("bulk").front(), "key0");
	// Выполняем проверку порядка выдачи последнего имени
	ASSERT_EQ(document.keys("bulk").back(), "key63");
	// Выполняем проверку сохранности прежних разделов
	ASSERT_EQ(document.size(), 3u);
}
/**
 * @brief Проверка отклонения неправильного построения текста настроек
 *
 */
TEST(CodecIniDocument, Malformed) {
	// Собираемые настройки дерева настроек
	ini::document_t::settings_t settings;
	// Устанавливаем строгие настройки разбора текста настроек
	settings.reader = ini::reader_t::settings_t::strict();
	// Дерево настроек
	ini::document_t document;
	// Выполняем проверку отклонения повторного объявления свойства
	ASSERT_FALSE(document.parse("[a]\nk=1\nk=2\n", settings));
	// Выполняем проверку кода ошибки разбора
	ASSERT_EQ(document.error(), ini::error_t::DUPLICATE_KEY);
	// Выполняем проверку номера строки обнаруженной ошибки
	ASSERT_EQ(document.errorLocation().line, 3u);
	// Выполняем проверку освобождения дерева при отказе разбора
	ASSERT_TRUE(document.empty());
}
/**
 * @brief Проверка освобождения дерева настроек
 *
 */
TEST(CodecIniDocument, Clear) {
	// Дерево настроек
	ini::document_t document;
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse("[a]\nk = v\n"));
	// Выполняем проверку наличия записей дерева
	ASSERT_FALSE(document.empty());
	// Выполняем освобождение дерева настроек
	document.clear();
	// Выполняем проверку отсутствия записей дерева
	ASSERT_TRUE(document.empty());
	// Выполняем проверку количества объявленных разделов
	ASSERT_EQ(document.size(), 0u);
	// Выполняем проверку пустоты записанного текста настроек
	ASSERT_TRUE(document.text().empty());
}
