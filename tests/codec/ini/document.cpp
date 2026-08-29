/**
 * @file document.cpp
 * @date 2026-08-10
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
 * @brief Автоматические тесты дерева настроек INI — поиск по разделам и свойствам,
 *        перечни повторных значений, подстановка обращений, правка на месте с сохранением
 *        оформления и обратный ход «текст - дерево - текст»
 *
 * @copyright Copyright © 2026
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
using namespace awh::codec;

/**
 * @brief Проверка разбора текста настроек в дерево
 *
 */
TEST(CodecIniDocument, Parse) {
	// Дерево настроек
	ini::document_t document(::logger());
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
	ini::document_t document(::logger());
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
		ini::document_t document(::logger());
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
		ini::document_t document(::logger());
		// Выполняем разбор текста настроек
		ASSERT_TRUE(document.parse("[a]\nk = первое\nk = второе\n", settings));
		// Выполняем проверку выдачи последнего объявления свойства
		ASSERT_EQ(document.get("k", "a"), "второе");
	}{
		// Дерево настроек
		ini::document_t document(::logger());
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
	ini::document_t document(::logger());
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
		ini::document_t document(::logger());
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
		ini::document_t document(::logger());
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
		ini::document_t document(::logger());
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
		ini::document_t document(::logger());
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
	ini::document_t document(::logger());
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
	ini::document_t document(::logger());
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
	ini::document_t document(::logger());
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
	ini::document_t document(::logger());
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
	ini::document_t document(::logger());
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
	ini::document_t document(::logger());
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
	ini::document_t document(::logger());
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
	ini::document_t document(::logger());
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
	ini::document_t document(::logger());
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
 * @brief Проверка подстановки обращения при обращении с повторами по последнему
 *
 * @note Обращение к значению обязано разрешаться тем же объявлением, какое
 *       выдаёт поиск по имени: иначе «${x}» и «get(x)» давали бы разное
 *
 */
TEST(CodecIniDocument, ReferenceDuplicates) {
	// Собираемые настройки дерева настроек
	ini::document_t::settings_t settings;
	// Устанавливаем построение обращения к значению другого свойства
	settings.references = ini::reference_t::SHELL;
	// Устанавливаем обращение с повторным объявлением свойства
	settings.reader.duplicates = ini::duplicate_t::LAST;
	// Дерево настроек
	ini::document_t document(::logger());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse("[a]\nref = ${x}\nx = первое\nx = последнее\n", settings));
	// Выполняем проверку выдачи последнего объявления свойства
	ASSERT_EQ(document.get("x", "a"), "последнее");
	// Выполняем проверку разрешения обращения тем же объявлением
	ASSERT_EQ(document.get("ref", "a"), "последнее");
}
/**
 * @brief Проверка отказа обратной записи при неограждаемом значении
 *
 * @note Обрубок текста опаснее отказа: он прочитается без нареканий и молча
 *       подменит данные
 *
 */
TEST(CodecIniDocument, WriteFailure) {
	// Дерево настроек
	ini::document_t document(::logger());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse("[a]\n"));
	// Выполняем установку значения со знаком конца строки
	ASSERT_TRUE(document.set("k", "первая\nвторая", "a"));
	/**
	 * Выполняем проверку отказа обратной записи дерева
	 *
	 * @note Запись управляющих последовательностей по умолчанию снята, и знак
	 *       конца строки внутри значения записать нечем
	 */
	ASSERT_TRUE(document.text().empty());
	// Выполняем проверку кода ошибки записи
	ASSERT_NE(document.error(), ini::error_t::NONE);
	// Собираемые настройки записи текста настроек
	ini::writer_t::settings_t settings;
	// Устанавливаем запись управляющих последовательностей в значении
	settings.escapes = true;
	// Выполняем проверку прохождения записи с управляющими последовательностями
	ASSERT_FALSE(document.text(settings).empty());
}
/**
 * @brief Проверка сохранности точки с запятой в значении наречия Git
 *
 * @note Наречие это пишет примечания решёткой, а читает и точку с запятой:
 *       значение с нею обязано быть ограждено, иначе обратное чтение его обрежет
 *
 */
TEST(CodecIniDocument, RoundtripGitMarkers) {
	// Собираемые настройки дерева настроек
	ini::document_t::settings_t settings;
	// Устанавливаем настройки разбора по образцу Git
	settings.reader = ini::reader_t::settings_t::git();
	// Дерево настроек
	ini::document_t document(::logger());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse("[a]\n", settings));
	// Выполняем установку значения со знаком примечания внутри
	ASSERT_TRUE(document.set("k", "one ; two", "a"));
	// Получаем записанный текст настроек
	const string text = document.text(ini::writer_t::settings_t::git());
	// Дерево настроек обратного чтения
	ini::document_t again(::logger());
	// Выполняем разбор записанного текста настроек
	ASSERT_TRUE(again.parse(text, settings));
	// Выполняем проверку сохранности значения при обороте «запись - чтение»
	ASSERT_EQ(again.get("k", "a"), "one ; two");
}
/**
 * @brief Проверка сохранности записи добавления к перечню значений
 *
 */
TEST(CodecIniDocument, RoundtripArrays) {
	// Собираемые настройки дерева настроек
	ini::document_t::settings_t settings;
	// Устанавливаем признание записи добавления к перечню значений
	settings.reader.arrays = true;
	// Дерево настроек
	ini::document_t document(::logger());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse("[a]\nk[] = one\nk[] = two\n", settings));
	// Выполняем проверку количества значений свойства
	ASSERT_EQ(document.values("k", "a").size(), 2u);
	// Выполняем проверку сохранности записи добавления к перечню
	ASSERT_EQ(document.text(), "[a]\nk[] = one\nk[] = two\n");
}
/**
 * @brief Проверка соблюдения предела подстановки удвоенным знаком обращения
 *
 * @note Знак, мимо предела проходящий, обращал бы предел в необязательный: набрать
 *       им можно было бы сколько угодно
 *
 */
TEST(CodecIniDocument, ExpansionBudget) {
	// Собираемые настройки дерева настроек
	ini::document_t::settings_t settings;
	// Устанавливаем построение обращения к значению другого свойства
	settings.references = ini::reference_t::SHELL;
	// Устанавливаем наибольший допустимый объём подстановки
	settings.maxExpansion = 128;
	// Собираемый текст настроек
	string text = "[a]\nk = ";
	// Выполняем сборку значения из удвоенных знаков обращения
	text.append(512, '$');
	// Выполняем добавление знака конца строки
	text.append(1, '\n');
	// Дерево настроек
	ini::document_t document(::logger());
	// Выполняем проверку отклонения разбора по превышению предела
	ASSERT_FALSE(document.parse(text, settings));
	// Выполняем проверку кода ошибки разбора
	ASSERT_EQ(document.error(), ini::error_t::EXPANSION_EXCEEDED);
}
/**
 * @brief Проверка сборки дерева с нуля в разделе без имени
 *
 */
TEST(CodecIniDocument, GlobalAssembly) {
	// Дерево настроек
	ini::document_t document(::logger());
	// Выполняем установку значения свойства в раздел без имени
	ASSERT_TRUE(document.set("name", "значение"));
	// Выполняем проверку установленного значения свойства
	ASSERT_EQ(document.get("name"), "значение");
	// Выполняем проверку записи собранного дерева
	ASSERT_EQ(document.text(), "name = значение\n");
	/**
	 * Выполняем проверку того, что раздел без имени объявленным не считается
	 */
	ASSERT_EQ(document.size(), 0u);
}
/**
 * @brief Проверка учёта регистра имён разделов наречием configparser
 *
 */
TEST(CodecIniDocument, SensitiveSections) {
	{
		// Собираемые настройки дерева настроек
		ini::document_t::settings_t settings;
		// Устанавливаем настройки разбора по образцу configparser
		settings.reader = ini::reader_t::settings_t::python();
		// Дерево настроек
		ini::document_t document(::logger());
		// Выполняем разбор текста настроек
		ASSERT_TRUE(document.parse("[Server]\nHost = a\n[server]\nHost = b\n", settings));
		/**
		 * Выполняем проверку раздельного учёта разделов
		 *
		 * @note Имена разделов это наречие сличает как записаны, а имена свойств
		 *       приводит к нижнему регистру
		 */
		ASSERT_EQ(document.size(), 2u);
		// Выполняем проверку значения свойства первого раздела
		ASSERT_EQ(document.get("host", "Server"), "a");
		// Выполняем проверку значения свойства второго раздела
		ASSERT_EQ(document.get("HOST", "server"), "b");
	}{
		// Дерево настроек
		ini::document_t document(::logger());
		// Выполняем разбор текста настроек
		ASSERT_TRUE(document.parse("[Server]\nHost = a\n[server]\nHost = b\n"));
		/**
		 * Выполняем проверку слияния разделов при умолчании
		 */
		ASSERT_EQ(document.size(), 1u);
	}
}
/**
 * @brief Проверка оборота незримых знаков «запись - чтение»
 *
 */
TEST(CodecIniDocument, RoundtripControls) {
	// Собираемые настройки дерева настроек
	ini::document_t::settings_t settings;
	// Устанавливаем разбор управляющих последовательностей в значении
	settings.reader.escapes = true;
	// Дерево настроек
	ini::document_t document(::logger());
	// Выполняем разбор текста настроек с незримыми знаками
	ASSERT_TRUE(document.parse("[a]\nk = one\\btwo\\fthree\n", settings));
	// Собираемые настройки записи текста настроек
	ini::writer_t::settings_t writing;
	// Устанавливаем запись управляющих последовательностей в значении
	writing.escapes = true;
	// Получаем записанный текст настроек
	const string text = document.text(writing);
	// Выполняем проверку прохождения записи
	ASSERT_FALSE(text.empty());
	// Дерево настроек обратного чтения
	ini::document_t again(::logger());
	// Выполняем проверку прохождения обратного чтения
	ASSERT_TRUE(again.parse(text, settings));
	// Выполняем проверку сохранности значения при обороте
	ASSERT_EQ(again.get("k", "a"), document.get("k", "a"));
}
/**
 * @brief Проверка перестроения указателей при смене настроек
 *
 * @note Указатели собраны по свёртке имён, а свёртка зависит от учёта регистра:
 *       смена настроек без перестроения обращала бы поиск в неверный
 *
 */
TEST(CodecIniDocument, SettingsReindex) {
	// Дерево настроек
	ini::document_t document(::logger());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse("[Server]\nHost = a\n"));
	// Выполняем проверку поиска без учёта регистра
	ASSERT_TRUE(document.has("host", "server"));
	// Собираемые настройки дерева настроек
	ini::document_t::settings_t settings;
	// Устанавливаем учёт регистра имён при сличении
	settings.reader.sensitive = true;
	// Выполняем установку настроек дерева настроек
	document.settings(settings);
	// Выполняем проверку поиска с учётом регистра
	ASSERT_TRUE(document.has("Host", "Server"));
	// Выполняем проверку отсутствия имени в ином регистре
	ASSERT_FALSE(document.has("host", "server"));
}
/**
 * @brief Проверка отклонения недопустимых имён при правке дерева
 *
 * @note Имя проверяется в месте правки, а не при записи собранного дерева: иначе
 *       правка проходила бы, а записать её было бы нельзя
 *
 */
TEST(CodecIniDocument, InvalidNames) {
	// Дерево настроек
	ini::document_t document(::logger());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse("[a]\n"));
	// Выполняем проверку отклонения имени свойства с квадратной скобкой
	ASSERT_FALSE(document.set("k[x", "v", "a"));
	// Выполняем проверку кода ошибки правки
	ASSERT_EQ(document.error(), ini::error_t::INVALID_KEY);
	// Выполняем проверку отклонения имени свойства с разделителем
	ASSERT_FALSE(document.set("k=x", "v", "a"));
	// Выполняем проверку отклонения имени свойства с пробельной обвязкой
	ASSERT_FALSE(document.set(" k", "v", "a"));
	// Выполняем проверку отклонения имени раздела со знаком конца строки
	ASSERT_FALSE(document.create("a\nb"));
	// Выполняем проверку кода ошибки правки
	ASSERT_EQ(document.error(), ini::error_t::INVALID_SECTION);
	// Выполняем проверку прохождения записи собранного дерева
	ASSERT_EQ(document.text(), "[a]\n");
}
/**
 * @brief Проверка обращения к значению чужого подраздела
 *
 */
TEST(CodecIniDocument, ReferenceSubsection) {
	// Собираемые настройки дерева настроек
	ini::document_t::settings_t settings;
	// Устанавливаем построение обращения к значению другого свойства
	settings.references = ini::reference_t::SHELL;
	// Устанавливаем настройки разбора по образцу Git
	settings.reader = ini::reader_t::settings_t::git();
	// Дерево настроек
	ini::document_t document(::logger());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse("[remote \"origin\"]\n\turl = https://host/repo\n[core]\n\tmirror = ${remote.origin:url}\n", settings));
	/**
	 * Выполняем проверку подстановки значения чужого подраздела
	 *
	 * @note Имя подраздела отделяется тем же знаком, каким оно отделяется в
	 *       объявлении раздела при построении разделителем
	 */
	ASSERT_EQ(document.get("mirror", "core"), "https://host/repo");
}
/**
 * @brief Проверка согласия проверки имён при правке с проверкой при записи
 *
 * @note Имя, которое отвергает запись, обязано отвергаться и правкой: иначе
 *       правка проходит, а записать её нечем
 *
 */
TEST(CodecIniDocument, NamesMatchWriter) {
	// Дерево настроек
	ini::document_t document(::logger());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse("[sec]\n"));
	/**
	 * Выполняем проверку допустимости знака примечания внутри имени свойства
	 *
	 * @note Знак этот значащ лишь первым знаком строки: там он обращает всю строку
	 *       в примечание, а внутри имени читается наравне с прочими
	 */
	ASSERT_TRUE(document.set("a;b", "1", "sec"));
	// Выполняем проверку отклонения знака примечания первым знаком имени
	ASSERT_FALSE(document.set(";ab", "1", "sec"));
	// Выполняем проверку кода ошибки правки
	ASSERT_EQ(document.error(), ini::error_t::INVALID_KEY);
	// Выполняем проверку отклонения знака решётки первым знаком имени
	ASSERT_FALSE(document.set("#ab", "1", "sec"));
	// Выполняем проверку допустимости знака примечания в имени раздела
	ASSERT_TRUE(document.create("a;b"));
	// Выполняем проверку прохождения записи собранного дерева
	ASSERT_EQ(document.text(), "[sec]\na;b = 1\n\n[a;b]\n");
	{
		// Собираемые настройки дерева настроек
		ini::document_t::settings_t settings;
		// Устанавливаем построение имени подраздела разделителем
		settings.reader.subsections = ini::subsection_t::DELIMITED;
		// Дерево настроек
		ini::document_t document(::logger());
		// Выполняем разбор текста настроек
		ASSERT_TRUE(document.parse("[a]\n", settings));
		/**
		 * Выполняем проверку отклонения знака-разделителя в имени раздела
		 *
		 * @note Читающий режет имя по первому его появлению, и раздел «a.b»
		 *       прочитался бы разделом «a» с подразделом «b»
		 */
		ASSERT_FALSE(document.create("a.b"));
		// Выполняем проверку допустимости знака-разделителя в имени подраздела
		ASSERT_TRUE(document.create("a", "b.c"));
	}
}
/**
 * @brief Проверка сброса кода ошибки удачной операцией
 *
 */
TEST(CodecIniDocument, ErrorReset) {
	// Дерево настроек
	ini::document_t document(::logger());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse("[a]\n"));
	// Выполняем проверку отклонения недопустимого имени свойства
	ASSERT_FALSE(document.set("k[x", "v", "a"));
	// Выполняем проверку кода ошибки правки
	ASSERT_EQ(document.error(), ini::error_t::INVALID_KEY);
	// Выполняем проверку прохождения установки значения
	ASSERT_TRUE(document.set("k", "v", "a"));
	/**
	 * Выполняем проверку сброса кода ошибки удачной операцией
	 */
	ASSERT_EQ(document.error(), ini::error_t::NONE);
}
/**
 * @brief Проверка отклонения записи байтов, кодировке UTF-8 не отвечающих
 *
 */
TEST(CodecIniDocument, InvalidHexEscape) {
	// Собираемые настройки дерева настроек
	ini::document_t::settings_t settings;
	// Устанавливаем разбор управляющих последовательностей в значении
	settings.reader.escapes = true;
	{
		// Дерево настроек
		ini::document_t document(::logger());
		// Выполняем проверку отклонения одиночного байта вне набора US-ASCII
		ASSERT_FALSE(document.parse("[a]\nk = one\\xFFtwo\n", settings));
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(document.error(), ini::error_t::INVALID_ENCODING);
	}{
		// Дерево настроек
		ini::document_t document(::logger());
		/**
		 * Выполняем проверку прохождения знака, записанного байтами верно
		 *
		 * @note Знак вне набора US-ASCII записывается в кодировке UTF-8 несколькими
		 *       байтами, и каждый из них вправе быть записан своей последовательностью
		 */
		ASSERT_TRUE(document.parse("[a]\nk = \\xc3\\xa9\n", settings));
		// Выполняем проверку собранного значения свойства
		ASSERT_EQ(document.get("k", "a"), "é");
	}
}
/**
 * @brief Проверка оборота имён со знаком примечания
 *
 */
TEST(CodecIniDocument, RoundtripMarkedNames) {
	// Дерево настроек
	ini::document_t document(::logger());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse("[a;b]\nk;x = v\n"));
	// Выполняем проверку сохранности имён при обратной записи
	ASSERT_EQ(document.text(), "[a;b]\nk;x = v\n");
	// Дерево настроек обратного чтения
	ini::document_t again(::logger());
	// Выполняем разбор записанного текста настроек
	ASSERT_TRUE(again.parse(document.text()));
	// Выполняем проверку сохранности значения при обороте
	ASSERT_EQ(again.get("k;x", "a;b"), "v");
}
/**
 * @brief Проверка наследования наречия записи от наречия разбора
 *
 * @note Без наследования обратная запись дерева, прочитанного не умолчанием,
 *       отвечала бы отказом: наречие записи о подразделах ничего бы не знало
 *
 */
TEST(CodecIniDocument, InheritedWriting) {
	{
		// Собираемые настройки дерева настроек
		ini::document_t::settings_t settings;
		// Устанавливаем построение имени подраздела разделителем
		settings.reader.subsections = ini::subsection_t::DELIMITED;
		// Дерево настроек
		ini::document_t document(::logger());
		// Выполняем разбор текста настроек
		ASSERT_TRUE(document.parse("[a.b]\nk = v\n", settings));
		// Выполняем проверку обратной записи без явных настроек
		ASSERT_EQ(document.text(), "[a.b]\nk = v\n");
	}{
		// Собираемые настройки дерева настроек
		ini::document_t::settings_t settings;
		// Устанавливаем настройки разбора по образцу Git
		settings.reader = ini::reader_t::settings_t::git();
		// Дерево настроек
		ini::document_t document(::logger());
		// Выполняем разбор текста настроек
		ASSERT_TRUE(document.parse("[remote \"origin\"]\n\turl = https://host\n", settings));
		// Получаем записанный текст настроек
		const string text = document.text();
		// Выполняем проверку прохождения обратной записи
		ASSERT_FALSE(text.empty());
		// Дерево настроек обратного чтения
		ini::document_t again(::logger());
		// Выполняем разбор записанного текста настроек
		ASSERT_TRUE(again.parse(text, settings));
		// Выполняем проверку сохранности значения подраздела при обороте
		ASSERT_EQ(again.get("url", "remote", "origin"), "https://host");
	}
}
/**
 * @brief Проверка отклонения подраздела, записать который нечем
 *
 */
TEST(CodecIniDocument, SubsectionUnsupported) {
	// Дерево настроек
	ini::document_t document(::logger());
	/**
	 * Выполняем проверку отклонения подраздела при построении NONE
	 */
	ASSERT_FALSE(document.create("a", "b"));
	// Выполняем проверку кода ошибки правки
	ASSERT_EQ(document.error(), ini::error_t::INVALID_SUBSECTION);
}
/**
 * @brief Проверка обратной записи наречием, кавычки за данные считающим
 *
 * @note Наречиям MS Windows и configparser кавычки при чтении не снимаются, и
 *       ограждение ими значения отдало бы читающему кавычки данными
 *
 */
TEST(CodecIniDocument, WritingKeptQuotes) {
	// Собираемые настройки дерева настроек
	ini::document_t::settings_t settings;
	// Устанавливаем настройки разбора по образцу MS Windows
	settings.reader = ini::reader_t::settings_t::windows();
	{
		// Дерево настроек
		ini::document_t document(::logger());
		// Выполняем разбор текста настроек с кавычками внутри значения
		ASSERT_TRUE(document.parse("[a]\nk = \"x\"\n", settings));
		// Выполняем проверку того, что кавычки остались частью значения
		ASSERT_EQ(document.get("k", "a"), "\"x\"");
		// Выполняем проверку сохранности значения при обратной записи
		ASSERT_EQ(document.text(), "[a]\nk = \"x\"\n");
	}{
		// Дерево настроек
		ini::document_t document(::logger());
		// Выполняем разбор текста настроек
		ASSERT_TRUE(document.parse("[a]\n", settings));
		// Выполняем установку значения с пробельной обвязкой
		ASSERT_TRUE(document.set("k", " v ", "a"));
		/**
		 * Выполняем проверку отказа обратной записи
		 *
		 * @note Значение это записать нечем: обвязку читающий отбросит, а оградить её
		 *       кавычками нельзя - кавычки достанутся ему частью значения
		 */
		ASSERT_TRUE(document.text().empty());
		// Выполняем проверку кода ошибки записи
		ASSERT_NE(document.error(), ini::error_t::NONE);
	}
}
/**
 * @brief Проверка знаков примечания в именах по наречию разбора
 *
 */
TEST(CodecIniDocument, MarkersByDialect) {
	// Собираемые настройки дерева настроек
	ini::document_t::settings_t settings;
	// Устанавливаем настройки разбора по образцу MS Windows
	settings.reader = ini::reader_t::settings_t::windows();
	// Дерево настроек
	ini::document_t document(::logger());
	/**
	 * Выполняем разбор текста настроек со знаком решётки в начале имени
	 *
	 * @note Наречие это примечанием признаёт лишь точку с запятой, и строка эта -
	 *       обыкновенное свойство
	 */
	ASSERT_TRUE(document.parse("[a]\n#path = c:\\bin\n", settings));
	// Выполняем проверку прочитанного имени свойства
	ASSERT_TRUE(document.has("#path", "a"));
	// Выполняем проверку принятия того же имени правкой
	ASSERT_TRUE(document.set("#path", "d:\\bin", "a"));
	// Выполняем проверку отклонения знака примечания этого наречия
	ASSERT_FALSE(document.set(";path", "v", "a"));
	// Выполняем проверку прохождения обратной записи
	ASSERT_EQ(document.text(), "[a]\n#path = d:\\bin\n");
}
/**
 * @brief Проверка правки подразделов, заключаемых в кавычки
 *
 * @note Внутри кавычек значащи лишь знаки конца строки: правка обязана принимать
 *       всякое имя, которое разбор читает, а запись ограждает
 *
 */
TEST(CodecIniDocument, QuotedSubsectionNames) {
	// Собираемые настройки дерева настроек
	ini::document_t::settings_t settings;
	// Устанавливаем настройки разбора по образцу Git
	settings.reader = ini::reader_t::settings_t::git();
	// Дерево настроек
	ini::document_t document(::logger());
	// Выполняем разбор текста настроек с квадратной скобкой в имени подраздела
	ASSERT_TRUE(document.parse("[remote \"a]b\"]\n\turl = x\n", settings));
	// Выполняем проверку значения свойства подраздела
	ASSERT_EQ(document.get("url", "remote", "a]b"), "x");
	// Выполняем удаление раздела с подразделом
	ASSERT_TRUE(document.remove("remote", "a]b"));
	// Выполняем проверку принятия того же имени правкой
	ASSERT_TRUE(document.create("remote", "a]b"));
	// Выполняем проверку принятия имени подраздела с кавычкой
	ASSERT_TRUE(document.create("remote", "a\"b"));
	// Получаем записанный текст настроек
	const string text = document.text();
	// Дерево настроек обратного чтения
	ini::document_t again(::logger());
	// Выполняем проверку прохождения обратного чтения
	ASSERT_TRUE(again.parse(text, settings));
	// Выполняем проверку сохранности имени подраздела с кавычкой
	ASSERT_TRUE(again.section("remote", "a\"b"));
}
/**
 * @brief Проверка снятия признака добавления к перечню при установке значения
 *
 */
/**
 * @brief Проверка подачи документу значения, с него же и снятого
 *
 * @details Значение отдаётся видом в хранилище знаков самого документа, и подача его
 *          обратно в `set()` есть прямой способ размножить настройку. Долив хранилища
 *          при этом его перераспределяет, то есть читается то самое место, какое
 *          освобождается. Проверка закрепляет, что перенос идёт целым: у соседнего
 *          кодека CSV подобное чтение освобождённой памяти было настоящим дефектом,
 *          и отличие здесь лишь в том, что долив ровно один, а не по полю на запись
 *
 */
TEST(CodecIniDocument, SetFromOwnView) {
	// Объект дерева настроек
	ini::document_t document(::logger());
	// Значение, заведомо превышающее короткий запас строки
	const string big(4096, 'z');
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse("[server]\nhost = " + big + "\n"));
	/**
	 * Выполняем набивку дерева, заведомо перераспределяющую хранилище знаков
	 */
	for(size_t i = 0; i < 64; i++)
		// Выполняем установку очередной настройки
		ASSERT_TRUE(document.set("поле" + to_string(i), string(256, 'a'), "server"));
	// Выполняем снятие значения настройки видом в хранилище знаков
	const string_view value = document.get("host", "server");
	// Выполняем проверку длины снятого значения
	ASSERT_EQ(value.length(), big.length());
	// Выполняем подачу снятого значения тому же документу
	ASSERT_TRUE(document.set("копия", value, "server"));
	// Выполняем проверку того, что значение перенесено целым
	ASSERT_EQ(document.get("копия", "server"), big);
	// Выполняем проверку сохранности исходной настройки
	ASSERT_EQ(document.get("host", "server"), big);
}

TEST(CodecIniDocument, SetDropsAppend) {
	// Собираемые настройки дерева настроек
	ini::document_t::settings_t settings;
	// Устанавливаем признание записи добавления к перечню значений
	settings.reader.arrays = true;
	// Дерево настроек
	ini::document_t document(::logger());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse("[a]\nk[] = 1\n", settings));
	// Выполняем установку значения объявленного свойства
	ASSERT_TRUE(document.set("k", "2", "a"));
	/**
	 * Выполняем проверку записи свойства обычной записью
	 *
	 * @note Установка значения заменяет прежнее, а не добавляет к перечню
	 */
	ASSERT_EQ(document.text(), "[a]\nk = 2\n");
}
/**
 * @brief Проверка предела глубины подразделов при объявлении раздела
 *
 * @note Дерево, которое запись соберёт, а повторный разбор отвергнет, собирать
 *       нельзя: отказ обязан приходить в месте правки
 *
 */
TEST(CodecIniDocument, DepthOnCreate) {
	// Собираемые настройки дерева настроек
	ini::document_t::settings_t settings;
	// Устанавливаем построение имени подраздела разделителем
	settings.reader.subsections = ini::subsection_t::DELIMITED;
	// Устанавливаем наибольшую допустимую глубину вложенности подразделов
	settings.reader.maxDepth = 1;
	// Дерево настроек
	ini::document_t document(::logger());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse("[a]\n", settings));
	// Выполняем проверку отклонения подраздела предельной глубины
	ASSERT_FALSE(document.create("a", "b.c"));
	// Выполняем проверку кода ошибки правки
	ASSERT_EQ(document.error(), ini::error_t::DEPTH_EXCEEDED);
	// Получаем записанный текст настроек
	const string text = document.text();
	// Дерево настроек обратного чтения
	ini::document_t again(::logger());
	// Выполняем проверку прохождения обратного чтения
	ASSERT_TRUE(again.parse(text, settings));
}
/**
 * @brief Проверка обращения к разделу, чьё имя несёт знак-разделитель
 *
 */
TEST(CodecIniDocument, ReferenceDelimited) {
	// Собираемые настройки дерева настроек
	ini::document_t::settings_t settings;
	// Устанавливаем построение обращения к значению другого свойства
	settings.references = ini::reference_t::SHELL;
	// Устанавливаем построение имени подраздела разделителем
	settings.reader.subsections = ini::subsection_t::DELIMITED;
	// Дерево настроек
	ini::document_t document(::logger());
	/**
	 * Выполняем разбор текста настроек
	 *
	 * @note Разбор режет имя по первому разделителю: раздел «a», подраздел «b.c»
	 */
	ASSERT_TRUE(document.parse("[a.b.c]\nk = v\n[z]\nr = ${a.b.c:k}\n", settings));
	// Выполняем проверку подстановки значения раздела с подразделом
	ASSERT_EQ(document.get("r", "z"), "v");
}
/**
 * @brief Проверка отсутствия сдвоенной пустой строки при объявлении раздела
 *
 */
TEST(CodecIniDocument, CreateSpacing) {
	// Дерево настроек
	ini::document_t document(::logger());
	// Выполняем разбор текста настроек, оканчивающегося пустой строкой
	ASSERT_TRUE(document.parse("[a]\nk = v\n\n"));
	// Выполняем объявление раздела
	ASSERT_TRUE(document.create("b"));
	/**
	 * Выполняем проверку записи объявленного раздела
	 *
	 * @note Пустая строка перед объявлением ставится лишь тогда, когда её ещё нет
	 */
	ASSERT_EQ(document.text(), "[a]\nk = v\n\n[b]\n");
}
/**
 * @brief Проверка сохранения кавычек значения при обратной записи
 *
 * @note Кавычки берутся из исходного текста наравне со знаком примечания: снимать
 *       их при перезаписи значило бы править то, о чём не просили
 *
 */
TEST(CodecIniDocument, RoundtripQuotes) {
	// Дерево настроек
	ini::document_t document(::logger());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse("[a]\nk = \"x\"\nplain = y\n"));
	// Выполняем проверку снятия кавычек со значения
	ASSERT_EQ(document.get("k", "a"), "x");
	// Выполняем проверку сохранности кавычек при обратной записи
	ASSERT_EQ(document.text(), "[a]\nk = \"x\"\nplain = y\n");
	// Выполняем установку нового значения свойства
	ASSERT_TRUE(document.set("k", "z", "a"));
	/**
	 * Выполняем проверку записи установленного значения без кавычек
	 *
	 * @note Кавычки сохраняются у значения, прочитанного в них; значение,
	 *       установленное правкой, в них не нуждается
	 */
	ASSERT_EQ(document.text(), "[a]\nk = z\nplain = y\n");
}
/**
 * @brief Проверка отсутствия сдвоенной пустой строки после удаления раздела
 *
 * @note Записи удалённого раздела остаются надгробиями, и проверка последней
 *       записи обязана их пропускать: иначе пустая строка ставится второй
 *
 */
TEST(CodecIniDocument, CreateAfterRemove) {
	// Дерево настроек
	ini::document_t document(::logger());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse("[a]\nk = 1\n\n[b]\nm = 2\n"));
	// Выполняем удаление раздела
	ASSERT_TRUE(document.remove("b"));
	// Выполняем объявление нового раздела
	ASSERT_TRUE(document.create("c"));
	// Выполняем проверку записи собранного дерева
	ASSERT_EQ(document.text(), "[a]\nk = 1\n\n[c]\n");
}
/**
 * @brief Проверка примечания конца строки у значения из строк продолжения
 *
 * @note Примечание кончается вместе со своей физической строкой: идущее до конца
 *       собранного значения, оно поглотило бы строки продолжения за собой
 *
 */
TEST(CodecIniDocument, InlineCommentContinued) {
	// Собираемые настройки дерева настроек
	ini::document_t::settings_t settings;
	// Устанавливаем настройки разбора по образцу configparser
	settings.reader = ini::reader_t::settings_t::python();
	// Устанавливаем признание примечания в конце строки свойства
	settings.reader.inlineComments = true;
	// Дерево настроек
	ini::document_t document(::logger());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse("[a]\nk = one ; хвост\n  two\nm = 2\n", settings));
	/**
	 * Выполняем проверку собранного значения свойства
	 *
	 * @note Значение это в точности то, какое даёт configparser
	 */
	ASSERT_EQ(document.get("k", "a"), "one\ntwo");
	// Выполняем проверку значения следующего свойства
	ASSERT_EQ(document.get("m", "a"), "2");
	// Получаем записанный текст настроек
	const string text = document.text();
	// Выполняем проверку прохождения обратной записи
	ASSERT_FALSE(text.empty());
	// Дерево настроек обратного чтения
	ini::document_t again(::logger());
	// Выполняем проверку прохождения обратного чтения
	ASSERT_TRUE(again.parse(text, settings));
	// Выполняем проверку сохранности значения при обороте
	ASSERT_EQ(again.get("k", "a"), "one\ntwo");
	// Выполняем проверку устойчивости записи при повторном обороте
	ASSERT_EQ(again.text(), text);
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
	ini::document_t document(::logger());
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
	ini::document_t document(::logger());
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

/**
 * @brief Проверка чтения значений всех поддерживаемых видов
 *
 */
TEST(CodecIniDocument, Values) {
	// Собираемые настройки дерева настроек
	ini::document_t::settings_t settings;
	// Устанавливаем настройки разбора наречия MS Windows
	settings.reader = ini::reader_t::settings_t::windows();
	// Объект дерева настроек
	ini::document_t document(::logger(), settings);
	// Выполняем проверку сохранения переданных настроек
	ASSERT_EQ(document.settings().reader.quotes, settings.reader.quotes);
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(
		"[s]\na = yes\nb = -128\nc = 255\nd = -32768\ne = 65535\n"
		"f = -2147483648\ng = 4294967295\nh = -9007199254740993\n"
		"i = 18446744073709551615\nj = 0.5\nk = -1250\n"
	));
	// Прочитанное логическое значение
	bool a = false;
	// Выполняем чтение логического значения
	ASSERT_TRUE(document.value(a, "a", "s"));
	// Выполняем проверку прочитанного значения
	ASSERT_TRUE(a);
	// Прочитанное знаковое однобайтовое целое
	int8_t b = 0;
	// Выполняем чтение знакового однобайтового целого
	ASSERT_TRUE(document.value(b, "b", "s"));
	// Выполняем проверку прочитанного значения
	ASSERT_EQ(b, -128);
	// Прочитанное беззнаковое однобайтовое целое
	uint8_t c = 0;
	// Выполняем чтение беззнакового однобайтового целого
	ASSERT_TRUE(document.value(c, "c", "s"));
	// Выполняем проверку прочитанного значения
	ASSERT_EQ(c, 255);
	// Прочитанное знаковое двухбайтовое целое
	int16_t d = 0;
	// Выполняем чтение знакового двухбайтового целого
	ASSERT_TRUE(document.value(d, "d", "s"));
	// Выполняем проверку прочитанного значения
	ASSERT_EQ(d, -32768);
	// Прочитанное беззнаковое двухбайтовое целое
	uint16_t e = 0;
	// Выполняем чтение беззнакового двухбайтового целого
	ASSERT_TRUE(document.value(e, "e", "s"));
	// Выполняем проверку прочитанного значения
	ASSERT_EQ(e, 65535);
	// Прочитанное знаковое четырёхбайтовое целое
	int32_t f = 0;
	// Выполняем чтение знакового четырёхбайтового целого
	ASSERT_TRUE(document.value(f, "f", "s"));
	// Выполняем проверку прочитанного значения
	ASSERT_EQ(f, -2147483647 - 1);
	// Прочитанное беззнаковое четырёхбайтовое целое
	uint32_t g = 0;
	// Выполняем чтение беззнакового четырёхбайтового целого
	ASSERT_TRUE(document.value(g, "g", "s"));
	// Выполняем проверку прочитанного значения
	ASSERT_EQ(g, 4294967295U);
	// Прочитанное знаковое восьмибайтовое целое
	int64_t h = 0;
	// Выполняем чтение знакового восьмибайтового целого
	ASSERT_TRUE(document.value(h, "h", "s"));
	// Выполняем проверку прочитанного значения
	ASSERT_EQ(h, -9007199254740993LL);
	// Прочитанное беззнаковое восьмибайтовое целое
	uint64_t i = 0;
	// Выполняем чтение беззнакового восьмибайтового целого
	ASSERT_TRUE(document.value(i, "i", "s"));
	// Выполняем проверку прочитанного значения
	ASSERT_EQ(i, 18446744073709551615ULL);
	// Прочитанное число с плавающей точкой одинарной точности
	float j = 0.f;
	// Выполняем чтение числа с плавающей точкой одинарной точности
	ASSERT_TRUE(document.value(j, "j", "s"));
	// Выполняем проверку прочитанного значения
	ASSERT_FLOAT_EQ(j, 0.5f);
	// Прочитанное число с плавающей точкой двойной точности
	double k = 0.;
	// Выполняем чтение числа с плавающей точкой двойной точности
	ASSERT_TRUE(document.value(k, "k", "s"));
	// Выполняем проверку прочитанного значения
	ASSERT_DOUBLE_EQ(k, -1250.);
	// Выполняем проверку отказа чтения несуществующего свойства
	ASSERT_FALSE(document.value(k, "z", "s"));
	// Выполняем проверку отказа чтения значения несовместимого вида
	ASSERT_FALSE(document.value(f, "a", "s"));
}
/**
 * @brief Проверка пересчёта подстановки обращений после правки дерева
 *
 * @details Правка меняет значения, на которые обращения ссылаются, и разрешённые
 *          прежде значения от неё устаревают: чтение выдавало бы подставленное до
 *          правки, расходясь с тем, что дало бы чтение записанного дерева обратно.
 *          Обращение, заведённое самой правкой, прежде не разрешалось вовсе
 *
 */
TEST(CodecIniDocument, ReferencesAfterEditing) {
	// Настройки дерева настроек с подстановкой обращений вида оболочки
	ini::document_t::settings_t options;
	// Выполняем установку вида записи обращения к значению
	options.references = ini::reference_t::SHELL;
	/**
	 * Выполняем проверку пересчёта после правки источника обращения
	 */
	{
		// Объект дерева настроек
		ini::document_t document(::logger(), options);
		// Выполняем проверку того, что разбор текста настроек удался
		ASSERT_TRUE(document.parse("[a]\nb = мир\nk = привет ${a:b}\n"));
		// Выполняем проверку разрешённого при разборе значения
		ASSERT_EQ(document.get("k", "a"), "привет мир");
		// Выполняем правку значения, на которое обращение ссылается
		ASSERT_TRUE(document.set("b", "друг", "a"));
		// Выполняем проверку пересчитанного значения обращения
		ASSERT_EQ(document.get("k", "a"), "привет друг");
		/**
		 * Выполняем проверку совпадения с чтением записанного дерева обратно
		 *
		 * @note Ради этого пересчёт и заведён: дерево, чьё чтение расходится с чтением
		 *       собственной записи, выдаёт потребителю то одно, то другое
		 */
		ini::document_t back(::logger(), options);
		// Выполняем проверку того, что разбор записанного дерева удался
		ASSERT_TRUE(back.parse(document.text()));
		// Выполняем проверку совпадения значений обращения
		ASSERT_EQ(back.get("k", "a"), document.get("k", "a"));
		/**
		 * Выполняем проверку сохранности обращения в записанном тексте
		 *
		 * @note В файл настроек уходит обращение, а не подставленное значение: оно на
		 *       то и записано, чтобы разрешаться заново при каждом чтении файла
		 */
		ASSERT_NE(document.text().find("${a:b}"), string::npos) << document.text();
	}
	/**
	 * Выполняем проверку разрешения обращения, заведённого правкой
	 */
	{
		// Объект дерева настроек
		ini::document_t document(::logger(), options);
		// Выполняем проверку того, что разбор текста настроек удался
		ASSERT_TRUE(document.parse("[a]\nb = мир\n"));
		// Выполняем заведение свойства с обращением к другому значению
		ASSERT_TRUE(document.set("k", "привет ${a:b}", "a"));
		// Выполняем проверку разрешённого значения обращения
		ASSERT_EQ(document.get("k", "a"), "привет мир");
	}
	/**
	 * Выполняем проверку правки при отключённой подстановке
	 *
	 * @note Обращение остаётся записанным как есть: подстановка настройками отменена
	 */
	{
		// Настройки дерева настроек без подстановки обращений
		ini::document_t::settings_t plain;
		// Выполняем отмену подстановки обращений
		plain.references = ini::reference_t::NONE;
		// Объект дерева настроек
		ini::document_t document(::logger(), plain);
		// Выполняем проверку того, что разбор текста настроек удался
		ASSERT_TRUE(document.parse("[a]\nb = мир\nk = привет ${a:b}\n"));
		// Выполняем правку значения, на которое обращение ссылается
		ASSERT_TRUE(document.set("b", "друг", "a"));
		// Выполняем проверку того, что обращение осталось неразрешённым
		ASSERT_EQ(document.get("k", "a"), "привет ${a:b}");
	}
	/**
	 * Выполняем проверку удаления источника обращения
	 *
	 * @note Правка вправе оставить дерево в состоянии, где обращение разрешить не по
	 *       чему, и объявлять её неудавшейся за это нельзя: значение остаётся таким,
	 *       каким записано, и разрешится само, едва источник появится снова
	 */
	{
		// Объект дерева настроек
		ini::document_t document(::logger(), options);
		// Выполняем проверку того, что разбор текста настроек удался
		ASSERT_TRUE(document.parse("[a]\nb = мир\nk = привет ${a:b}\n"));
		// Выполняем удаление свойства, на которое обращение ссылается
		ASSERT_TRUE(document.erase("b", "a"));
		// Выполняем проверку того, что значение осталось в записанном виде
		ASSERT_EQ(document.get("k", "a"), "привет ${a:b}");
		/**
		 * Выполняем проверку отказа записи при неразрешимом обращении
		 *
		 * @note Разбор такой текст с включённой подстановкой не примет, и выдать его
		 *       значило бы отдать потребителю заведомо негодный файл настроек
		 */
		ASSERT_TRUE(document.text().empty());
		// Выполняем проверку кода ошибки отказа записи
		ASSERT_EQ(document.error(), ini::error_t::UNKNOWN_REFERENCE);
		// Выполняем заведение источника обращения заново
		ASSERT_TRUE(document.set("b", "снова", "a"));
		// Выполняем проверку разрешения обращения по заведённому источнику
		ASSERT_EQ(document.get("k", "a"), "привет снова");
	}
	/**
	 * Выполняем проверку удаления раздела с источником обращения
	 */
	{
		// Объект дерева настроек
		ini::document_t document(::logger(), options);
		// Выполняем проверку того, что разбор текста настроек удался
		ASSERT_TRUE(document.parse("[a]\nb = мир\n[c]\nk = привет ${a:b}\n"));
		// Выполняем проверку разрешённого при разборе значения
		ASSERT_EQ(document.get("k", "c"), "привет мир");
		// Выполняем удаление раздела с источником обращения
		ASSERT_TRUE(document.remove("a"));
		// Выполняем проверку того, что значение осталось в записанном виде
		ASSERT_EQ(document.get("k", "c"), "привет ${a:b}");
	}
}

/**
 * @brief Проверка подстановки обращения через посредника
 *
 * @details Значение посредника само несёт удвоенный знак обращения, а тот служит записи
 * знака самого: разрешается посредник в запись обращения, обращением уже не являющуюся.
 * Свойство, к посреднику обращённое, обязано получить эту запись как есть. Подстановка
 * же читала разрешённое значение вместо записанного и подставляла второй раз, выдавая
 * значение цели
 *
 * @note Заметен изъян лишь после правки дерева, пересчёт подстановки вызывающей: при
 *       первом разборе разрешённое значение посредника ещё совпадает с записанным
 *
 */
TEST(CodecIniDocument, ReferenceThroughProxy) {
	// Настройки дерева настроек
	ini::document_t::settings_t options;
	// Устанавливаем подстановку обращений по образцу оболочки
	options.references = ini::reference_t::SHELL;
	// Объект дерева настроек
	ini::document_t document(::logger(), options);
	// Выполняем разбор текста настроек с обращением через посредника
	ASSERT_TRUE(document.parse("[s]\nцель = добыто\nпосредник = $${цель}\nчерез = ${s:посредник}\n"));
	// Выполняем проверку разрешения посредника в запись обращения
	ASSERT_EQ(document.get("посредник", "s"), "${цель}");
	// Выполняем проверку того, что запись эта досталась обращённому свойству как есть
	ASSERT_EQ(document.get("через", "s"), "${цель}");
	// Выполняем правку дерева, пересчёт подстановки вызывающую
	ASSERT_TRUE(document.set("прочее", "значение", "s"));
	// Выполняем проверку того, что пересчёт значения посредника не изменил
	ASSERT_EQ(document.get("посредник", "s"), "${цель}");
	// Выполняем проверку того, что пересчёт не подставил обращение второй раз
	ASSERT_EQ(document.get("через", "s"), "${цель}");
}

/**
 * @brief Проверка места свойства, заведённого в разделе без имени
 *
 * @details Раздел без имени объявления не имеет, а значит, и последней записи: место
 * вставки бралось тогда концом перечня записей, то есть за объявлениями всех прочих
 * разделов. Записанное свойство доставалось при обратном чтении последнему из них -
 * дерево звало значение своим, а его же запись отдавала чужому разделу
 *
 */
TEST(CodecIniDocument, GlobalAfterSection) {
	// Настройки дерева настроек
	ini::document_t::settings_t options;
	// Объект дерева настроек
	ini::document_t document(::logger(), options);
	// Выполняем установку свойства в именованном разделе
	ASSERT_TRUE(document.set("j", "в разделе", "a"));
	// Выполняем установку свойства в разделе без имени
	ASSERT_TRUE(document.set("g", "глобальное", ""));
	// Выполняем перезапись дерева настроек
	const string text = document.text();
	// Выполняем проверку того, что свойство записано прежде объявления раздела
	ASSERT_LT(text.find("g = глобальное"), text.find("[a]"));
	// Объект дерева настроек, собираемого обратным разбором
	ini::document_t again(::logger(), options);
	// Выполняем разбор перезаписанного текста настроек
	ASSERT_TRUE(again.parse(text));
	// Выполняем проверку того, что свойство осталось в разделе без имени
	ASSERT_EQ(again.get("g"), "глобальное");
	// Выполняем проверку того, что именованному разделу свойство не досталось
	ASSERT_FALSE(again.has("g", "a"));
	// Выполняем проверку того, что свойство раздела осталось на месте
	ASSERT_EQ(again.get("j", "a"), "в разделе");
}

/**
 * @brief Проверка долива к перечню значением, взятым у того же дерева
 *
 * @note Долив кладёт значение в хранилище знаков того же дерева, и вид, в него
 *       указывающий, наращивание хранилища пережить обязан
 *
 */
TEST(CodecIniDocument, PushFromOwnView) {
	// Объект дерева настроек
	ini::document_t document(::logger());
	// Значение, заведомо превышающее короткий запас строки
	const string big(4096, 'z');
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse("[server]\nhost = " + big + "\n"));
	/**
	 * Выполняем набивку дерева, заведомо перераспределяющую хранилище знаков
	 */
	for(size_t i = 0; i < 64; i++)
		// Выполняем установку очередной настройки
		ASSERT_TRUE(document.set("поле" + to_string(i), string(256, 'a'), "server"));
	// Выполняем снятие значения настройки видом в хранилище знаков
	const string_view value = document.get("host", "server");
	// Выполняем долив снятого значения тому же дереву
	ASSERT_TRUE(document.push("перечень", value, "server"));
	// Выполняем долив второго значения к перечню
	ASSERT_TRUE(document.push("перечень", "второе", "server"));
	// Выполняем проверку количества значений собранного перечня
	ASSERT_EQ(document.values("перечень", "server").size(), 2);
	// Выполняем проверку того, что значение перенесено целым
	ASSERT_EQ(document.values("перечень", "server").front(), big);
	// Выполняем проверку сохранности исходной настройки
	ASSERT_EQ(document.get("host", "server"), big);
	/**
	 * Выполняем проверку того, что перечень уходит в текст повтором имени
	 *
	 * @note Перечень записи INI есть последовательность одноимённых свойств, и
	 *       записаться иначе он не вправе
	 */
	ASSERT_NE(document.text().find("перечень = второе"), string::npos);
}

/**
 * @brief Проверка совпадения наречия Git со средством «git config»
 *
 * @details Наречие заявлено по образцу настроек Git, и расхождения с самим средством
 * есть дефект, а не выбор. Три расхождения найдены сличением с ним и здесь закреплены:
 * примечание без пробельного знака, свойства до первого раздела и подраздел, отделённый
 * знаком-разделителем наравне с кавычками
 *
 * @note Всякое из ожиданий сверено с выдачей «git config -f файл --list» на той же
 *       записи, а не выведено из описания: описание Git о части этих случаев молчит
 *
 */
TEST(CodecIniDocument, GitDialectMatchesTool) {
	// Собираемые настройки дерева настроек
	ini::document_t::settings_t settings;
	// Устанавливаем настройки разбора по образцу Git
	settings.reader = ini::reader_t::settings_t::git();
	{
		// Дерево настроек
		ini::document_t document(::logger());
		/**
		 * Выполняем разбор текста настроек с примечанием без пробельного знака
		 *
		 * @note Средство выдаёт «s.key=a»: знак примечания режет значение где угодно
		 */
		ASSERT_TRUE(document.parse("[s]\nkey = a#b\n", settings));
		// Выполняем проверку того, что значение обрезано по знаку примечания
		ASSERT_EQ(document.get("key", "s"), "a");
	}{
		// Дерево настроек
		ini::document_t document(::logger());
		/**
		 * Выполняем разбор текста настроек со свойством до первого раздела
		 *
		 * @note Средство выдаёт «loose=1» и «s.k=v»: свойство без раздела оно принимает
		 */
		ASSERT_TRUE(document.parse("loose = 1\n[s]\nk = v\n", settings));
		// Выполняем проверку того, что свойство до первого раздела принято
		ASSERT_EQ(document.get("loose"), "1");
		// Выполняем проверку того, что свойство раздела принято тоже
		ASSERT_EQ(document.get("k", "s"), "v");
	}{
		// Дерево настроек
		ini::document_t document(::logger());
		/**
		 * Выполняем разбор текста настроек с подразделом, отделённым разделителем
		 *
		 * @note Средство выдаёт «a.b.key=value»: запись эту оно принимает наравне с
		 *       записью кавычками
		 */
		ASSERT_TRUE(document.parse("[a.b]\nkey = value\n", settings));
		// Выполняем проверку того, что подраздел выделен
		ASSERT_TRUE(document.section("a", "b"));
		// Выполняем проверку значения свойства подраздела
		ASSERT_EQ(document.get("key", "a", "b"), "value");
	}{
		// Дерево настроек
		ini::document_t document(::logger());
		/**
		 * Выполняем разбор текста настроек с подразделом, кавычками взятым
		 *
		 * @note Признание обоих построений разом кавычек не отменяет
		 */
		ASSERT_TRUE(document.parse("[remote \"origin\"]\nurl = https://host\n", settings));
		// Выполняем проверку того, что подраздел выделен
		ASSERT_TRUE(document.section("remote", "origin"));
		// Выполняем проверку значения свойства подраздела
		ASSERT_EQ(document.get("url", "remote", "origin"), "https://host");
	}{
		// Дерево настроек
		ini::document_t document(::logger());
		/**
		 * Выполняем разбор текста настроек с разделителем внутри имени, кавычками взятого
		 *
		 * @note Ровно это правка и ломала: поиск разделителя уходил внутрь кавычек и рвал
		 *       имя надвое, выдавая раздел «remote "a» с подразделом «b"»
		 */
		ASSERT_TRUE(document.parse("[remote \"a.b\"]\nkey = value\n", settings));
		// Выполняем проверку того, что имя подраздела осталось целым
		ASSERT_TRUE(document.section("remote", "a.b"));
		// Выполняем проверку значения свойства подраздела
		ASSERT_EQ(document.get("key", "remote", "a.b"), "value");
	}
}

/**
 * @brief Проверка требования пробельного знака перед примечанием
 *
 * @details Требование это - защита пути и пароля, где точка с запятой и решётка стоят
 * посреди значения. Стоит оно по умолчанию, а снимается настройкою: наречие Git режет
 * значение без пробела
 *
 */
TEST(CodecIniDocument, SpacedCommentsSetting) {
	// Собираемые настройки дерева настроек
	ini::document_t::settings_t settings;
	// Устанавливаем признание примечания в конце строки свойства
	settings.reader.inlineComments = true;
	{
		// Дерево настроек
		ini::document_t document(::logger());
		// Выполняем разбор текста настроек с примечанием без пробельного знака
		ASSERT_TRUE(document.parse("[s]\nkey = a#b\n", settings));
		// Выполняем проверку того, что значение осталось целым
		ASSERT_EQ(document.get("key", "s"), "a#b");
	}{
		// Дерево настроек
		ini::document_t document(::logger());
		// Выполняем разбор текста настроек с примечанием за пробельным знаком
		ASSERT_TRUE(document.parse("[s]\nkey = a #b\n", settings));
		// Выполняем проверку того, что значение обрезано по знаку примечания
		ASSERT_EQ(document.get("key", "s"), "a");
	}{
		// Снимаем требование пробельного знака перед началом примечания
		settings.reader.spacedComments = false;
		// Дерево настроек
		ini::document_t document(::logger());
		// Выполняем разбор текста настроек с примечанием без пробельного знака
		ASSERT_TRUE(document.parse("[s]\nkey = a#b\n", settings));
		// Выполняем проверку того, что значение обрезано по знаку примечания
		ASSERT_EQ(document.get("key", "s"), "a");
	}
}

/**
 * @brief Проверка построения имени раздела наречием configparser
 *
 * @details Наречие заявлено по образцу разбора языка Python, и два его свойства с
 * образцом расходились: закрывающая скобка объявления искалась до первой в строке, а
 * пробельная обвязка имени раздела отбрасывалась. Образец берёт имя выражением
 * `\[(?P<header>.+)\]`, жадность которого доводит поиск до последней скобки, и обвязку
 * сохраняет
 *
 * @note Оба ожидания сверены с самим `configparser`, а не выведены из описания
 *
 */
TEST(CodecIniDocument, PythonDialectSectionNames) {
	// Собираемые настройки дерева настроек
	ini::document_t::settings_t settings;
	// Устанавливаем настройки разбора по образцу configparser
	settings.reader = ini::reader_t::settings_t::python();
	{
		// Дерево настроек
		ini::document_t document(::logger());
		// Выполняем разбор текста настроек с квадратными скобками в имени раздела
		ASSERT_TRUE(document.parse("[x[]]\nkey = value\n", settings));
		// Выполняем проверку того, что имя раздела взято до последней скобки
		ASSERT_TRUE(document.section("x[]"));
		// Выполняем проверку значения свойства раздела
		ASSERT_EQ(document.get("key", "x[]"), "value");
	}{
		// Дерево настроек
		ini::document_t document(::logger());
		// Выполняем разбор текста настроек с пробельной обвязкой имени раздела
		ASSERT_TRUE(document.parse("[ section ]\nkey = value\n", settings));
		// Выполняем проверку того, что обвязка имени раздела сохранена
		ASSERT_TRUE(document.section(" section "));
		// Выполняем проверку значения свойства раздела
		ASSERT_EQ(document.get("key", " section "), "value");
	}{
		// Собираемые настройки дерева настроек по умолчанию
		ini::document_t::settings_t plain;
		// Дерево настроек
		ini::document_t document(::logger());
		/**
		 * Выполняем разбор текста настроек умолчанием
		 *
		 * @note Умолчание обвязку отбрасывает и берёт первую скобку: свойства эти
		 *       принадлежат наречию, а не записи INI вообще
		 */
		ASSERT_TRUE(document.parse("[ section ]\nkey = value\n", plain));
		// Выполняем проверку того, что обвязка имени раздела отброшена
		ASSERT_TRUE(document.section("section"));
	}
}

/**
 * @brief Проверка пробела перед примечанием за объявлением раздела
 *
 * @details Пробел перед знаком примечания не ставился при читающем, обвязки не
 * отбрасывающем: он достался бы частью значения предыдущего свойства, и значение росло
 * бы при каждом обороте «чтение - запись». За объявлением же раздела значения нет вовсе,
 * и довод этот к нему не относится, а пробел ему НЕОБХОДИМ
 *
 * @note Без него читающий, требующий пробела, примечания не признаёт и забирает его себе
 *       в имя раздела - тем вернее, что закрывающая скобка ищется до последней в строке,
 *       и скобка внутри примечания становится границей имени. Перезапись от того теряла
 *       устойчивость: второй оборот давал текст, от первого отличный
 *
 * @note Нашёл расхождение это ворошитель, и вскрылось оно лишь по заведении поиска до
 *       последней скобки: прежде тот же изъян давал отказ разбора, а не искажение
 *
 */
TEST(CodecIniDocument, HeaderCommentKeepsSpace) {
	// Собираемые настройки дерева настроек
	ini::document_t::settings_t settings;
	// Устанавливаем признание примечания в конце строки
	settings.reader.inlineComments = true;
	// Устанавливаем поиск закрывающей скобки до последней в строке
	settings.reader.greedySections = true;
	// Снимаем отбрасывание пробельной обвязки значения свойства
	settings.reader.trim = false;
	// Дерево настроек
	ini::document_t document(::logger());
	// Выполняем разбор текста настроек с примечанием за объявлением раздела
	ASSERT_TRUE(document.parse("[c] ; хвост [не раздел]\nkey = value\n", settings));
	// Выполняем первую перезапись дерева настроек
	const string first = document.text();
	/**
	 * Выполняем проверку того, что пробел перед примечанием записан
	 *
	 * @note Знак примечания берётся не тот, каким оно записано в исходном тексте, а тот,
	 *       каким пишет наречие: читающий признаёт оба, а пишущий - один свой
	 */
	ASSERT_NE(first.find("] ;"), string::npos);
	// Дерево настроек обратного чтения
	ini::document_t again(::logger());
	// Выполняем разбор первой перезаписи
	ASSERT_TRUE(again.parse(first, settings));
	/**
	 * Выполняем проверку того, что имя раздела при обороте сохранилось
	 *
	 * @note Без пробела перед примечанием имя вышло бы «c] ; хвост [не раздел»: поиск
	 *       закрывающей скобки, примечания не признав, дошёл бы до последней в строке
	 */
	ASSERT_TRUE(again.section("c"));
	// Выполняем проверку устойчивости перезаписи
	ASSERT_EQ(again.text(), first);
}

/**
 * @brief Проверка отличения ошибочного обращения от обращения к необъявленному
 *
 * @details Отказы эти разного рода, и путать их нельзя: имя, к какому обращено
 *          значение, может быть объявлено, а само обращение оборвано. Прежде оба
 *          отвечали кодом обращения к необъявленному значению, и потребитель искал
 *          недостающее свойство вместо незакрытой скобки
 *
 */
TEST(CodecIniDocument, MalformedReferenceDiffersFromUnknown) {
	{
		// Собираемые настройки дерева настроек
		ini::document_t::settings_t settings;
		// Устанавливаем построение обращения к значению другого свойства
		settings.references = ini::reference_t::SHELL;
		// Дерево настроек
		ini::document_t document(::logger());
		/**
		 * Выполняем проверку отказа разбора незакрытого обращения
		 *
		 * @note Свойство «root» объявлено тут же: отказ принадлежит построению
		 *       обращения, а не отсутствию имени
		 */
		ASSERT_FALSE(document.parse("[a]\nroot = /opt\nlogs = ${root/logs\n", settings));
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(document.error(), ini::error_t::INVALID_REFERENCE);
	}{
		// Собираемые настройки дерева настроек
		ini::document_t::settings_t settings;
		// Устанавливаем построение обращения по образцу configparser
		settings.references = ini::reference_t::PYTHON;
		// Дерево настроек
		ini::document_t document(::logger());
		// Выполняем проверку отказа разбора обращения без завершающего признака
		ASSERT_FALSE(document.parse("[a]\nname = awh\ngreeting = привет, %(name)\n", settings));
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(document.error(), ini::error_t::INVALID_REFERENCE);
	}{
		// Собираемые настройки дерева настроек
		ini::document_t::settings_t settings;
		// Устанавливаем построение обращения к значению другого свойства
		settings.references = ini::reference_t::SHELL;
		// Дерево настроек
		ini::document_t document(::logger());
		// Выполняем проверку сохранения кода обращения к необъявленному значению
		ASSERT_FALSE(document.parse("[a]\nk = ${missing}\n", settings));
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(document.error(), ini::error_t::UNKNOWN_REFERENCE);
	}
}

/**
 * @brief Проверка того, что правка неизвестного имени называет причину
 *
 * @details Снос неизвестного свойства и удаление неизвестного раздела отвечали ложью
 *          без кода ошибки, и потребитель отличить отсутствие имени от изъяна его не
 *          мог вовсе. Кодек TOML на том же случае причину называет, и расходиться им
 *          в этом незачем
 *
 */
TEST(CodecIniDocument, UnknownNameReportsReason) {
	// Дерево настроек
	ini::document_t document(::logger());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse("[a]\nk = v\n"));
	// Выполняем проверку отказа сноса необъявленного свойства
	ASSERT_FALSE(document.erase("missing", "a"));
	// Выполняем проверку кода ошибки правки дерева
	ASSERT_EQ(document.error(), ini::error_t::UNKNOWN_KEY);
	// Выполняем проверку отказа удаления необъявленного раздела
	ASSERT_FALSE(document.remove("missing"));
	// Выполняем проверку кода ошибки правки дерева
	ASSERT_EQ(document.error(), ini::error_t::UNKNOWN_SECTION);
	// Выполняем проверку отказа сноса свойства из необъявленного раздела
	ASSERT_FALSE(document.erase("k", "нет"));
	// Выполняем проверку кода ошибки правки дерева
	ASSERT_EQ(document.error(), ini::error_t::UNKNOWN_SECTION);
	// Выполняем проверку отказа удаления раздела с пустым именем
	ASSERT_FALSE(document.remove(""));
	// Выполняем проверку кода ошибки правки дерева
	ASSERT_EQ(document.error(), ini::error_t::EMPTY_SECTION);
	/**
	 * Выполняем проверку того, что опрос кодом ошибки не метится
	 *
	 * @note Опрос наличия отсутствием отвечает законно, и отказом это не является
	 */
	ASSERT_FALSE(document.has("missing", "a"));
	// Выполняем проверку сохранения дерева в целости
	ASSERT_EQ(document.get("k", "a"), "v");
}
/**
 * @brief Проверка независимости предела обращений от предела подразделов
 *
 * @note Настройки эти вложены одна в другую и прежде звались одинаково: правка не той
 *       из них отказа не давала и подмены не выдавала. Проверка стережёт именно это -
 *       что всякая стережёт своё
 *
 */
TEST(CodecIniDocument, ReferenceDepthDiffersFromSubsectionDepth) {
	// Объект документа настроек
	ini::document_t document(::logger());
	// Настройки разбора текста настроек
	ini::document_t::settings_t settings;
	// Устанавливаем построение имени подраздела разделителем
	settings.reader.subsections = ini::subsection_t::DELIMITED;
	// Выполняем запрет подразделов вовсе пределом читающего
	settings.reader.maxDepth = 0;
	// Выполняем разбор текста настроек с подразделом
	ASSERT_FALSE(document.parse("[раздел.подраздел]\nkey = value\n", settings));
	// Выполняем дозволение подразделов пределом читающего
	settings.reader.maxDepth = 4;
	// Выполняем разбор того же текста настроек
	ASSERT_TRUE(document.parse("[раздел.подраздел]\nkey = value\n", settings));
	// Выполняем запрет вложенности обращений пределом дерева
	settings.maxReferenceDepth = 0;
	// Выполняем разбор того же текста настроек, обращений не несущего
	ASSERT_TRUE(document.parse("[раздел.подраздел]\nkey = value\n", settings));
	// Значения свойства, из документа снятые
	const vector <string_view> values = document.values("key", "раздел", "подраздел");
	// Выполняем проверку количества снятых значений
	ASSERT_EQ(values.size(), 1u);
	// Выполняем проверку того, что предел обращений подразделу не помеха
	ASSERT_EQ(values.front(), "value");
}
/**
 * @brief Проверка заслонов от ссылочной бомбы
 *
 * @details Подстановка обращений к значениям других свойств несёт два предела:
 *          глубину вложенности обращений и общий объём подстановки. Пределы эти суть
 *          заслон от ссылочной бомбы - записи, где всякий уровень удваивает объём
 *          предыдущего и десяток уровней даёт гигабайты
 *
 * @warning Проверка эта обязательна именно потому, что заслоны не срабатывают у
 *          потребителя обычного: предел, ни разу не сработавший, вправе оказаться
 *          недостижимым вовсе, и защиты тогда нет никакой, а по коду она есть
 *
 * @note Половина прохода обязательна у обоих пределов: заслон, отвергающий ВСЯКОЕ
 *       обращение, прошёл бы половину отказа и оставил бы подстановку неработающей
 *
 */
TEST(CodecIniDocument, ReferenceBombGuards) {
	// Собираемые настройки дерева настроек
	ini::document_t::settings_t settings;
	// Устанавливаем запись обращения видом оболочки
	settings.references = ini::reference_t::SHELL;
	/**
	 * Выполняем проверку предела глубины вложенности обращений
	 */
	{
		// Собираемый текст настроек с цепью обращений
		string text("[a]\nk0 = дно\n");
		/**
		 * Выполняем сборку цепи обращений глубже предела вдвое
		 */
		for(uint32_t i = 1; i < (ini::MAX_REFERENCE_DEPTH * 2); i++)
			// Добавляем очередное звено цепи обращений
			text.append("k" + std::to_string(i) + " = ${a:k" + std::to_string(i - 1) + "}\n");
		// Дерево настроек, цепь разбирающее
		ini::document_t document(::logger(), settings);
		// Выполняем проверку отказа разбора цепи глубже предела
		ASSERT_FALSE(document.parse(text));
		// Выполняем проверку того, что отказ назван причиною своею
		ASSERT_EQ(document.error(), ini::error_t::REFERENCE_DEPTH);
	}
	/**
	 * Выполняем проверку того, что цепь мельче предела проходит
	 */
	{
		// Собираемый текст настроек с цепью обращений
		string text("[a]\nk0 = дно\n");
		/**
		 * Выполняем сборку цепи обращений вдвое мельче предела
		 */
		for(uint32_t i = 1; i < (ini::MAX_REFERENCE_DEPTH / 2); i++)
			// Добавляем очередное звено цепи обращений
			text.append("k" + std::to_string(i) + " = ${a:k" + std::to_string(i - 1) + "}\n");
		// Дерево настроек, цепь разбирающее
		ini::document_t document(::logger(), settings);
		// Выполняем проверку успешности разбора цепи мельче предела
		ASSERT_TRUE(document.parse(text)) << ini::message(document.error());
		// Выполняем проверку того, что подстановка дна цепи достигла
		ASSERT_EQ(document.get("k" + std::to_string((ini::MAX_REFERENCE_DEPTH / 2) - 1), "a"), "дно");
	}
	/**
	 * Выполняем проверку предела общего объёма подстановки
	 *
	 * @note Всякое звено здесь удваивает объём предыдущего: двадцать четыре звена от
	 *       шестидесяти четырёх знаков дают около гигабайта, и заслон обязан оборвать
	 *       подстановку задолго до того
	 */
	{
		// Собираемый текст настроек со ссылочной бомбой
		string text("[a]\nk0 = " + string(64, 'x') + "\n");
		/**
		 * Выполняем сборку звеньев, объём предыдущего удваивающих
		 */
		for(uint32_t i = 1; i < 24; i++)
			// Добавляем очередное звено ссылочной бомбы
			text.append("k" + std::to_string(i) + " = ${a:k" + std::to_string(i - 1) +
			 "}${a:k" + std::to_string(i - 1) + "}\n");
		// Дерево настроек, бомбу разбирающее
		ini::document_t document(::logger(), settings);
		// Выполняем проверку отказа разбора ссылочной бомбы
		ASSERT_FALSE(document.parse(text));
		// Выполняем проверку того, что отказ назван причиною своею
		ASSERT_EQ(document.error(), ini::error_t::EXPANSION_EXCEEDED);
	}
	/**
	 * Выполняем проверку того, что подстановка объёмом мельче предела проходит
	 */
	{
		// Дерево настроек, обращение разбирающее
		ini::document_t document(::logger(), settings);
		// Выполняем проверку успешности разбора обращения
		ASSERT_TRUE(document.parse("[a]\nцель = добыто\nссылка = ${a:цель}\n")) << ini::message(document.error());
		// Выполняем проверку того, что обращение подставлено
		ASSERT_EQ(document.get("ссылка", "a"), "добыто");
	}
	/**
	 * Выполняем проверку отказа кругового обращения
	 */
	{
		// Дерево настроек, круговое обращение разбирающее
		ini::document_t document(::logger(), settings);
		// Выполняем проверку отказа разбора кругового обращения
		ASSERT_FALSE(document.parse("[a]\nx = ${a:y}\ny = ${a:x}\n"));
		// Выполняем проверку того, что отказ назван причиною своею
		ASSERT_NE(document.error(), ini::error_t::NONE);
	}
}
/**
 * @brief Проверка долива свойства в раздел, ещё не заведённый
 *
 * @details Долив обязан заводить раздел сам, а дерево пустое вовсе - принимать
 *          свойство в раздел без имени: раздел тот объявления не требует и через
 *          объявление заведён быть не может
 *
 * @note Заходы эти пересечение трёх прогонов числило непройденными ничем: набор
 *       проверок доливал лишь в разделы, разбором уже заведённые
 *
 */
TEST(CodecIniDocument, PushCreatesSection) {
	/**
	 * Выполняем проверку долива в дерево пустое вовсе
	 */
	{
		// Дерево настроек, разбора не видевшее
		ini::document_t document(::logger());
		// Выполняем проверку успешности долива свойства разделу без имени
		ASSERT_TRUE(document.push("ключ", "значение"));
		// Выполняем проверку того, что свойство найдено
		ASSERT_EQ(document.get("ключ"), "значение");
		// Выполняем проверку записи дерева
		ASSERT_EQ(document.text(), "ключ = значение\n");
	}
	/**
	 * Выполняем проверку долива в раздел, ещё не заведённый
	 */
	{
		// Дерево настроек, раздел уже несущее
		ini::document_t document(::logger());
		// Выполняем проверку успешности разбора текста настроек
		ASSERT_TRUE(document.parse("[есть]\nk = 1\n"));
		// Выполняем проверку успешности долива свойства в раздел незаведённый
		ASSERT_TRUE(document.push("ключ", "значение", "новый"));
		// Выполняем проверку того, что свойство легло именно в новый раздел
		ASSERT_EQ(document.get("ключ", "новый"), "значение");
		// Выполняем проверку того, что прежний раздел доливом не тронут
		ASSERT_EQ(document.get("k", "есть"), "1");
	}
}
/**
 * @brief Проверка разрешения обращения, правкой дерева заведённого
 *
 * @details Признак того, что дерево несёт обращения, взводится не одной лишь
 *          подстановкой при разборе, но и правкой: обращение, правкой заведённое в
 *          дереве, обращений прежде не несшем, подстановка иначе не разрешила бы вовсе
 *
 * @warning Проверка эта обязательна именно потому, что порок был бы молчалив:
 *          потребитель получил бы запись обращения вместо значения его, и ни отказа,
 *          ни кода ошибки при том не увидел бы
 *
 * @note Заход пересечение трёх прогонов числило непройденным ничем: подстановка
 *       проверялась лишь на обращениях, самим текстом принесённых
 *
 */
TEST(CodecIniDocument, EditedReferenceResolves) {
	// Собираемые настройки дерева настроек
	ini::document_t::settings_t settings;
	// Устанавливаем запись обращения видом оболочки
	settings.references = ini::reference_t::SHELL;
	/**
	 * Выполняем проверку обращения, правкой заведённого в свойстве прежнем
	 */
	{
		// Дерево настроек, обращений не несущее
		ini::document_t document(::logger(), settings);
		// Выполняем проверку успешности разбора текста настроек
		ASSERT_TRUE(document.parse("[a]\nцель = добыто\nпростое = было\n"));
		// Выполняем проверку того, что значение свойства прежнее
		ASSERT_EQ(document.get("простое", "a"), "было");
		// Выполняем проверку успешности правки значения обращением
		ASSERT_TRUE(document.set("простое", "${a:цель}", "a")) << ini::message(document.error());
		// Выполняем проверку того, что обращение разрешено
		ASSERT_EQ(document.get("простое", "a"), "добыто");
	}
	/**
	 * Выполняем проверку обращения, правкой заведённого в свойстве новом
	 */
	{
		// Дерево настроек, обращений не несущее
		ini::document_t document(::logger(), settings);
		// Выполняем проверку успешности разбора текста настроек
		ASSERT_TRUE(document.parse("[a]\nцель = добыто\n"));
		// Выполняем проверку успешности заведения свойства обращением
		ASSERT_TRUE(document.set("новое", "${a:цель}", "a")) << ini::message(document.error());
		// Выполняем проверку того, что обращение разрешено
		ASSERT_EQ(document.get("новое", "a"), "добыто");
	}
	/**
	 * Выполняем проверку обращения, доливом заведённого в разделе новом
	 */
	{
		// Дерево настроек, обращений не несущее
		ini::document_t document(::logger(), settings);
		// Выполняем проверку успешности разбора текста настроек
		ASSERT_TRUE(document.parse("[a]\nцель = добыто\n"));
		// Выполняем проверку успешности долива свойства обращением
		ASSERT_TRUE(document.push("ссылка", "${a:цель}", "новый")) << ini::message(document.error());
		// Выполняем проверку того, что обращение разрешено
		ASSERT_EQ(document.get("ссылка", "новый"), "добыто");
	}
}
/**
 * @brief Проверка выдачи значений свойства до подстановки обращений
 *
 * @details Значение до подстановки дерево держит ради обратной записи: перезапись обязана
 *          выйти той же, какою текст читался, а подставь она разрешённое значение -
 *          обращение пропало бы из текста навсегда. Выдача эта потребителю нужна, чтобы
 *          отличить записанное от разрешённого
 *
 * @note Способ этот не звался набором проверок ни разу - розыск по объявленным наружу не
 *       дал ни одного случая. Взят он договором наружу, а не картою охвата
 *
 * @warning Порчею доказано: подмена выдачи значением разрешённым вместо записанного
 *          проверку валит. Тем она и стережёт разницу между записанным и разрешённым, а
 *          не одну лишь непустоту выдачи
 *
 */
TEST(CodecIniDocument, SourcesYieldRecordsBeforeSubstitution) {
	// Собираемые настройки дерева настроек
	ini::document_t::settings_t settings;
	// Устанавливаем построение обращения к значению другого свойства
	settings.references = ini::reference_t::SHELL;
	// Дерево настроек
	ini::document_t document(::logger());
	// Выполняем разбор текста настроек с обращениями
	ASSERT_TRUE(document.parse("[paths]\nroot = /opt/awh\nlogs = ${root}/logs\n", settings));
	// Выполняем проверку того, что выдача обычная подстановку и провела
	ASSERT_EQ(document.get("logs", "paths"), "/opt/awh/logs");
	// Получаем значения свойства до подстановки обращений
	const vector <string_view> sources = document.sources("logs", "paths");
	// Выполняем проверку того, что значение до подстановки выдано одно
	ASSERT_EQ(sources.size(), static_cast <size_t> (1));
	// Выполняем проверку того, что обращение в нём осталось нетронутым
	ASSERT_EQ(sources.front(), "${root}/logs");
	/**
	 * Выполняем проверку выдачи у свойства, обращений не несущего
	 *
	 * @note Значение без обращений подстановки не знает вовсе, и выдача до подстановки
	 *       совпадает с выдачей обычной: проверка держит границу
	 */
	{
		// Получаем значения свойства, обращений не несущего
		const vector <string_view> plain = document.sources("root", "paths");
		// Выполняем проверку того, что значение выдано одно
		ASSERT_EQ(plain.size(), static_cast <size_t> (1));
		// Выполняем проверку того, что значение совпадает с выдачей обычной
		ASSERT_EQ(plain.front(), document.get("root", "paths"));
	}
	/**
	 * Выполняем проверку того, что перезапись обращение сохраняет
	 */
	{
		// Выполняем проверку того, что перезапись несёт обращение, а не разрешённое значение
		ASSERT_NE(document.text().find("${root}/logs"), string::npos) << document.text();
	}
}
/**
 * @brief Проверка расхода предела объёма подстановки на знаке обращения одиноком
 *
 * @details Предел объёма подстановки убывает на всяком знаке разрешаемого значения, а не
 *          только на подставленном: знак, мимо предела проходящий, обращал бы предел в
 *          необязательный. Знак обращения, за каким ни имя не открывается, ни знак тот же
 *          не стоит, обращением не является и уходит в значение знаком простым - предел
 *          убывает и на нём
 *
 * @note Место это лежало в стороне от всех трёх прогонов сразу. Набор брал предел на
 *       подстановке и на знаке удвоенном - оба хода свои, а этот, третий, оставался
 *       нетронутым
 *
 * @warning Написание подобрано по телу подстановки, а не по догадке: удвоенный знак
 *          уходит своим ходом и предел расходует иначе, оттого запись из одних знаков
 *          обращения сюда не доводит вовсе
 *
 */
TEST(CodecIniDocument, ExpansionBudgetOnLoneMarker) {
	/**
	 * Выполняем проверку отказа при пределе, знаком обращения одиноким исчерпанном
	 */
	{
		// Собираемые настройки дерева настроек
		ini::document_t::settings_t settings;
		// Устанавливаем построение обращения к значению другого свойства
		settings.references = ini::reference_t::SHELL;
		// Устанавливаем предел объёма подстановки в один байт
		settings.maxExpansion = 1;
		// Дерево настроек
		ini::document_t document(::logger());
		/**
		 * Выполняем проверку отказа разбора значения, предел исчерпавшего
		 *
		 * @note Знак обращения стоит последним нарочно: стой за ним ещё знак, предел
		 *       исчерпался бы на нём же ходом соседним, и порча этого хода проверкою не
		 *       ловилась бы вовсе - отказ приходил бы всё равно, только знаком позже
		 */
		ASSERT_FALSE(document.parse("[a]\nk = a$\n", settings));
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(document.error(), ini::error_t::EXPANSION_EXCEEDED);
	}
	/**
	 * Выполняем проверку принятия значения, в предел укладывающегося
	 *
	 * @note Проверка держит границу: то же написание при пределе, знакам его отвечающем,
	 *       принимается и знак обращения одинокий сохраняет
	 */
	{
		// Собираемые настройки дерева настроек
		ini::document_t::settings_t settings;
		// Устанавливаем построение обращения к значению другого свойства
		settings.references = ini::reference_t::SHELL;
		// Устанавливаем предел объёма подстановки в два байта
		settings.maxExpansion = 2;
		// Дерево настроек
		ini::document_t document(::logger());
		// Выполняем проверку успешности разбора значения, в предел укладывающегося
		ASSERT_TRUE(document.parse("[a]\nk = a$\n", settings));
		// Выполняем проверку того, что знак обращения одинокий сохранён
		ASSERT_EQ(document.get("k", "a"), "a$");
	}
}
/**
 * @brief Проверка поля настроек чтения, ни одной проверкой не назначавшегося
 *
 * @details Четыре поля настроек чтения набор проверок не назначал ни разу: знаки
 *          начала примечания, знак-разделитель имени подраздела, отбрасывание
 *          пробельной обвязки имени раздела и учёт регистра имён разделов. Поле,
 *          настройкою не читаемое вовсе, обратило бы настройку в украшение, а разбор
 *          шёл бы по умолчанию, о чём бы ни просил потребитель
 *
 * @note Судится РАЗЛИЧИЕ разборов одного текста при двух настройках, а не точная
 *       запись дерева: договор настройки в том и состоит, что она разбор меняет
 *
 * @warning Из четырёх полей закреплено ОДНО: comments и sensitiveSections дерево
 *          настроек не выдаёт вовсе - ни примечаний, ни различия регистра имён оно не
 *          хранит, - а заход разделителя имени подраздела не удался трижды: различие
 *          давала настройка subsections, а по устранении её порча чтения знака
 *          проверку не роняла. Стеречь все три надо рядом событий разбора, как это
 *          сделано у примечаний YAML; заходы эти НЕ заводить сличением дерева
 *
 */
TEST(CodecIniDocument, UnsetReaderSettingsFields) {
	// Разбираемый текст настроек с пробельной обвязкой имени раздела
	const char * text = "[ раздел ]\nk = 1\n";
	// Разбор текста при настройках умолчания
	ini::document_t fallback(::logger());
	// Выполняем проверку успешности разбора текста умолчанием
	ASSERT_TRUE(fallback.parse(text));
	// Настройки дерева настроек
	ini::document_t::settings_t settings;
	// Снимаем отбрасывание пробельной обвязки имени раздела
	settings.reader.trimSections = false;
	// Разбор текста при изменённой настройке
	ini::document_t tuned(::logger());
	// Выполняем проверку успешности разбора текста изменённой настройкой
	ASSERT_TRUE(tuned.parse(text, settings));
	/**
	 * @brief Функция снятия отпечатка разобранного дерева
	 *
	 * @param document дерево настроек, отпечаток с какого снимается
	 * @return         отпечаток разобранного дерева
	 *
	 */
	const auto fingerprint = [](const ini::document_t & document) noexcept -> string {
		// Собираемый отпечаток разобранного дерева
		string result = document.text();
		/**
		 * Выполняем перебор имён разделов дерева
		 */
		for(auto & name : document.sections())
			// Выполняем запись имени раздела вместе с именем подраздела
			result.append("|").append(name.section).append("~").append(name.subsection);
		// Выводим собранный отпечаток дерева
		return result;
	};
	// Выполняем проверку того, что настройка разбор изменила
	ASSERT_NE(fingerprint(fallback), fingerprint(tuned));
}
