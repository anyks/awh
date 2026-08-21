/**
 * @file writer.cpp
 * @date 2026-08-01
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
 * @brief Автоматические тесты записи текста разметки — сборка конверта SOAP, экранирование
 *        содержимого, виды записи, отклонение неправильного построения и обратный ход
 *        «текст - дерево - текст - дерево»
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
#include <codec/xml/xml.hpp>

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
 * @brief Проверка сборки конверта по договору SOAP
 *
 */
TEST(CodecXmlWriter, Soap) {
	// Объект записи текста разметки
	xml::writer_t writer;
	// Выполняем запись объявления разметки
	ASSERT_TRUE(writer.declaration());
	// Выполняем запись конверта запроса
	ASSERT_TRUE(writer.open("Envelope", "http://schemas.xmlsoap.org/soap/envelope/"));
	// Выполняем запись способа записи содержимого
	ASSERT_TRUE(writer.attribute("encodingStyle", "http://schemas.xmlsoap.org/soap/encoding/", "http://schemas.xmlsoap.org/soap/envelope/"));
	// Выполняем запись тела запроса
	ASSERT_TRUE(writer.open("Body", "http://schemas.xmlsoap.org/soap/envelope/"));
	// Выполняем запись запрашиваемого действия
	ASSERT_TRUE(writer.open("GetExternalIPAddress", "urn:schemas-upnp-org:service:WANIPConnection:1"));
	// Выполняем закрытие запрашиваемого действия
	ASSERT_TRUE(writer.close());
	// Выполняем закрытие тела запроса
	ASSERT_TRUE(writer.close());
	// Выполняем закрытие конверта запроса
	ASSERT_TRUE(writer.close());
	// Выполняем проверку завершённости собранного текста
	ASSERT_TRUE(writer.complete()) << xml::message(writer.error());
	// Объект дерева разметки
	xml::document_t document;
	// Выполняем разбор собранного текста разметки
	ASSERT_TRUE(document.parse(writer.text())) << xml::message(document.error());
	// Выполняем проверку имени корневого узла
	ASSERT_TRUE(document.element().name().is("http://schemas.xmlsoap.org/soap/envelope/", "Envelope"));
	// Выполняем проверку записанного действия
	ASSERT_TRUE(document.element().find("GetExternalIPAddress", "urn:schemas-upnp-org:service:WANIPConnection:1").valid()) << writer.text();
}
/**
 * @brief Проверка экранирования содержимого и значений атрибутов
 *
 */
TEST(CodecXmlWriter, Escape) {
	// Объект записи текстового содержимого
	xml::writer_t content;
	// Выполняем запись узла с содержимым, требующим экранирования
	ASSERT_TRUE(content.element("a", "<&>\"'\r"));
	// Выполняем проверку экранирования содержимого
	ASSERT_EQ(content.text(), "<a>&lt;&amp;&gt;\"'&#13;</a>");
	// Объект записи значения атрибута
	xml::writer_t attribute;
	// Выполняем запись узла разметки
	ASSERT_TRUE(attribute.open("a"));
	// Выполняем запись атрибута со значением, требующим экранирования
	ASSERT_TRUE(attribute.attribute("x", "<&>\"'\n\t"));
	// Выполняем закрытие узла разметки
	ASSERT_TRUE(attribute.close());
	// Выполняем проверку экранирования значения атрибута
	ASSERT_EQ(attribute.text(), "<a x=\"&lt;&amp;&gt;&quot;'&#10;&#9;\"/>");
}
/**
 * @brief Проверка видов записи собираемого текста разметки
 *
 */
TEST(CodecXmlWriter, Format) {
	// Настройки записи с отступами
	xml::writer_t::settings_t settings;
	// Выполняем установку вида записи с отступами
	settings.format = xml::format_t::PRETTY;
	// Объект записи текста разметки с отступами
	xml::writer_t pretty(settings);
	// Выполняем запись узла разметки
	ASSERT_TRUE(pretty.open("r"));
	// Выполняем запись первого вложенного узла
	ASSERT_TRUE(pretty.element("i", "1"));
	// Выполняем запись второго вложенного узла
	ASSERT_TRUE(pretty.element("i", "2"));
	// Выполняем закрытие узла разметки
	ASSERT_TRUE(pretty.close());
	// Выполняем проверку расстановки отступов знаком по умолчанию
	ASSERT_EQ(pretty.text(), "<r>\n\t<i>1</i>\n\t<i>2</i>\n</r>");
	/**
	 * Выполняем проверку расстановки отступов пробелами
	 */
	{
		// Настройки записи с отступами пробелами
		xml::writer_t::settings_t spaced;
		// Выполняем установку вида записи с отступами
		spaced.format = xml::format_t::PRETTY;
		// Выполняем установку знака отступа
		spaced.separator = xml::separator_t::SPACES;
		// Выполняем установку количества знаков отступа
		spaced.indent = 2;
		// Объект записи текста разметки с отступами пробелами
		xml::writer_t writer(spaced);
		// Выполняем запись узла разметки с вложенными узлами
		ASSERT_TRUE(writer.open("r") && writer.element("i", "1") && writer.element("i", "2") && writer.close());
		// Выполняем проверку расстановки отступов пробелами
		ASSERT_EQ(writer.text(), "<r>\n  <i>1</i>\n  <i>2</i>\n</r>");
	}
	/**
	 * Выполняем проверку нарядной записи без отступов
	 */
	{
		// Настройки записи без отступов
		xml::writer_t::settings_t plain;
		// Выполняем установку вида записи с отступами
		plain.format = xml::format_t::PRETTY;
		// Выполняем отмену знака отступа
		plain.separator = xml::separator_t::NONE;
		// Объект записи текста разметки без отступов
		xml::writer_t writer(plain);
		// Выполняем запись узла разметки с вложенными узлами
		ASSERT_TRUE(writer.open("r") && writer.element("i", "1") && writer.element("i", "2") && writer.close());
		// Выполняем проверку того, что переводы строк расставлены, а отступы нет
		ASSERT_EQ(writer.text(), "<r>\n<i>1</i>\n<i>2</i>\n</r>");
	}
	// Объект записи текста разметки видом по умолчанию
	xml::writer_t plain;
	// Выполняем проверку вида записи по умолчанию
	ASSERT_EQ(plain.settings().format, xml::format_t::COMPACT);
	// Выполняем запись узла разметки
	ASSERT_TRUE(plain.open("r"));
	// Выполняем запись вложенного узла
	ASSERT_TRUE(plain.element("i", "1"));
	// Выполняем закрытие узла разметки
	ASSERT_TRUE(plain.close());
	// Выполняем проверку плотной записи
	ASSERT_EQ(plain.text(), "<r><i>1</i></r>");
	// Настройки плотной записи
	xml::writer_t::settings_t compact;
	// Выполняем установку плотного вида записи
	compact.format = xml::format_t::COMPACT;
	// Объект записи с переключением вида на ходу
	xml::writer_t mixed(settings);
	// Выполняем запись узла разметки
	ASSERT_TRUE(mixed.open("r"));
	// Выполняем переключение вида записи
	mixed.settings(compact);
	// Выполняем запись вложенного узла
	ASSERT_TRUE(mixed.element("i", "1"));
	// Выполняем закрытие узла разметки
	ASSERT_TRUE(mixed.close());
	// Выполняем проверку переключения вида записи
	ASSERT_EQ(mixed.text(), "<r><i>1</i></r>");
}
/**
 * @brief Проверка отклонения неправильного построения текста разметки
 *
 */
TEST(CodecXmlWriter, Malformed) {
	// Объект записи атрибута после содержимого узла
	xml::writer_t attribute;
	// Выполняем запись узла разметки
	ASSERT_TRUE(attribute.open("a"));
	// Выполняем запись содержимого узла
	ASSERT_TRUE(attribute.text("т"));
	// Выполняем проверку отклонения атрибута после содержимого
	ASSERT_FALSE(attribute.attribute("x", "1"));
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(attribute.error(), xml::error_t::INVALID_ATTRIBUTE);
	// Объект записи узла с ошибочным именем
	xml::writer_t name;
	// Выполняем проверку отклонения ошибочного имени
	ASSERT_FALSE(name.open("1узел"));
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(name.error(), xml::error_t::INVALID_NAME);
	// Объект записи лишнего закрытия узла
	xml::writer_t closing;
	// Выполняем проверку отклонения лишнего закрытия узла
	ASSERT_FALSE(closing.close());
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(closing.error(), xml::error_t::UNEXPECTED_CLOSE_TAG);
	// Объект записи второго корневого узла
	xml::writer_t roots;
	// Выполняем запись корневого узла разметки
	ASSERT_TRUE(roots.open("a"));
	// Выполняем закрытие корневого узла разметки
	ASSERT_TRUE(roots.close());
	// Выполняем проверку отклонения второго корневого узла
	ASSERT_FALSE(roots.open("b"));
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(roots.error(), xml::error_t::MULTIPLE_ROOTS);
	// Объект записи дословного раздела
	xml::writer_t cdata;
	// Выполняем запись узла разметки
	ASSERT_TRUE(cdata.open("a"));
	// Выполняем проверку отклонения завершения раздела внутри его содержимого
	ASSERT_FALSE(cdata.cdata("х]]>х"));
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(cdata.error(), xml::error_t::INVALID_CDATA);
	// Объект записи примечания
	xml::writer_t comment;
	// Выполняем запись узла разметки
	ASSERT_TRUE(comment.open("a"));
	// Выполняем проверку отклонения двойного дефиса в примечании
	ASSERT_FALSE(comment.comment("--"));
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(comment.error(), xml::error_t::INVALID_COMMENT);
	// Объект записи указания обработчику
	xml::writer_t processing;
	// Выполняем запись узла разметки
	ASSERT_TRUE(processing.open("a"));
	// Выполняем проверку отклонения отведённого договором имени
	ASSERT_FALSE(processing.processing("xml", "d"));
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(processing.error(), xml::error_t::RESERVED_PROCESSING);
	// Объект записи незавершённого текста разметки
	xml::writer_t incomplete;
	// Выполняем запись узла разметки
	ASSERT_TRUE(incomplete.open("a"));
	// Выполняем запись вложенного узла разметки
	ASSERT_TRUE(incomplete.open("b"));
	// Выполняем закрытие вложенного узла разметки
	ASSERT_TRUE(incomplete.close());
	// Выполняем проверку незавершённости собранного текста
	ASSERT_FALSE(incomplete.complete());
}
/**
 * @brief Проверка обратного хода «текст - дерево - текст - дерево»
 *
 */
TEST(CodecXmlWriter, Roundtrip) {
	// Исходный текст разметки
	const string source =
		"<?xml version=\"1.0\"?><root xmlns=\"urn:d\" xmlns:s=\"urn:s\">"
		"<!--примечание--><?цель данные?>"
		"<item s:id=\"1\" name=\"первый\">значение</item>"
		"<item s:id=\"2\"><![CDATA[<дословно>]]></item>"
		"</root>";
	// Объект дерева разметки исходного текста
	xml::document_t first;
	// Выполняем разбор исходного текста разметки
	ASSERT_TRUE(first.parse(source)) << xml::message(first.error());
	// Объект записи текста разметки
	xml::writer_t writer;
	// Выполняем запись объявления разметки
	ASSERT_TRUE(writer.declaration());
	// Выполняем запись разобранного дерева разметки
	ASSERT_TRUE(writer.element(first.root())) << xml::message(writer.error());
	// Выполняем проверку завершённости собранного текста
	ASSERT_TRUE(writer.complete());
	// Объект дерева разметки записанного текста
	xml::document_t second;
	// Выполняем разбор записанного текста разметки
	ASSERT_TRUE(second.parse(writer.text())) << xml::message(second.error());
	// Получаем корневой узел исходного дерева
	const xml::node_t a = first.element();
	// Получаем корневой узел записанного дерева
	const xml::node_t b = second.element();
	// Выполняем проверку совпадения имён корневых узлов
	ASSERT_TRUE(a.name().is(b.name().uri, b.name().local));
	// Выполняем проверку совпадения количества узлов деревьев
	ASSERT_EQ(first.size(), second.size());
	// Выполняем проверку совпадения содержимого деревьев
	ASSERT_EQ(a.text(), b.text());
	// Получаем перечень одноимённых узлов записанного дерева
	const vector <xml::node_t> items = b.children("item", "urn:d");
	// Выполняем проверку количества одноимённых узлов
	ASSERT_EQ(items.size(), static_cast <size_t> (2));
	// Выполняем проверку значения атрибута с префиксом
	ASSERT_EQ(items[1].attribute("id", "urn:s"), "2") << writer.text();
	// Выполняем проверку дословного содержимого
	ASSERT_EQ(items[1].text(), "<дословно>") << writer.text();
}
/**
 * @brief Проверка записи цели указания обработчику с разделителем префикса
 *
 * @details Договор о пространствах имён на цели указаний обработчику не
 * распространяется, и разделитель префикса обычным знаком имени в ней и
 * является. Разбор с выключенным разрешением префиксов такую цель принимает,
 * и записать разобранное дерево обратно обязано удаваться
 *
 */
TEST(CodecXmlWriter, ProcessingTargetColon) {
	// Исходный текст разметки с разделителем в цели указания обработчику
	const string source = "<?a:b данные?><root/>";
	// Настройки разбора без разрешения префиксов
	xml::reader_t::settings_t settings;
	// Выполняем выключение разрешения префиксов пространств имён
	settings.namespaces = false;
	// Объект дерева разметки исходного текста
	xml::document_t document;
	// Выполняем разбор исходного текста разметки
	ASSERT_TRUE(document.parse(source, settings)) << xml::message(document.error());
	// Объект записи текста разметки
	xml::writer_t writer;
	// Выполняем запись разобранного дерева разметки
	ASSERT_TRUE(writer.element(document.root())) << xml::message(writer.error());
	// Выполняем проверку совпадения записанного текста с исходным
	ASSERT_EQ(writer.text(), source);
	// Объект записи указания обработчику напрямую
	xml::writer_t direct;
	// Выполняем запись указания обработчику с разделителем в цели
	ASSERT_TRUE(direct.processing("a:b", "данные")) << xml::message(direct.error());
	/**
	 * Выполняем проверку отказа записи цели, построенной ошибочно
	 *
	 * @note Разделитель префикса знаком начала имени договором дозволен наравне с
	 *       буквой, и цель `:b` построена правильно: ошибочной её делает лишь цифра
	 *       в начале имени
	 */
	ASSERT_FALSE(direct.processing("1b", "данные"));
}
/**
 * @brief Проверка экранирования знаков, выходящих за пределы US-ASCII
 *
 */
TEST(CodecXmlWriter, NonAscii) {
	// Настройки записи с экранированием знаков вне US-ASCII
	xml::writer_t::settings_t settings;
	// Выполняем активацию экранирования знаков вне US-ASCII
	settings.escapeNonAscii = true;
	// Объект записи текста разметки
	xml::writer_t writer(settings);
	// Выполняем запись узла с содержимым вне US-ASCII
	ASSERT_TRUE(writer.element("a", "да"));
	// Выполняем проверку экранирования знаков вне US-ASCII
	ASSERT_EQ(writer.text(), "<a>&#x434;&#x430;</a>");
	// Объект дерева разметки
	xml::document_t document;
	// Выполняем разбор записанного текста разметки
	ASSERT_TRUE(document.parse(writer.text())) << xml::message(document.error());
	// Выполняем проверку содержимого разобранного узла
	ASSERT_EQ(document.element().text(), "да");
}
/**
 * @brief Проверка отклонения указаний, собирающих неправильно построенный текст
 *
 * @details Договор класса обещает, что указание, нарушающее строение, отвергается, а не
 *          записывается. Проверяются построения, которые запись прежде принимала и
 *          собирала текст, который её же собственное чтение принять не может
 *
 */
/**
 * @brief Проверка экранирования знаков вне US-ASCII в значении атрибута
 *
 * @details Настройка экранирования до сих пор проверялась лишь на содержимом узла,
 *          тогда как значение атрибута записывается отдельным ходом со своим набором
 *          отводимых знаков. Ход этот не исполнялся ни одной проверкой
 *
 */
TEST(CodecXmlWriter, NonAsciiAttribute) {
	// Настройки записи с экранированием знаков вне US-ASCII
	xml::writer_t::settings_t settings;
	// Выполняем активацию экранирования знаков вне US-ASCII
	settings.escapeNonAscii = true;
	// Объект записи текста разметки
	xml::writer_t writer(settings);
	// Выполняем открытие узла разметки
	ASSERT_TRUE(writer.open("a"));
	// Выполняем запись значения атрибута со знаками вне US-ASCII
	ASSERT_TRUE(writer.attribute("k", "да\u20AC"));
	// Выполняем завершение узла разметки
	ASSERT_TRUE(writer.close());
	// Выполняем проверку экранирования знаков вне US-ASCII
	ASSERT_EQ(writer.text(), "<a k=\"&#x434;&#x430;&#x20AC;\"/>");
	// Объект дерева разметки
	xml::document_t document;
	// Выполняем разбор записанного текста разметки
	ASSERT_TRUE(document.parse(writer.text())) << xml::message(document.error());
	// Выполняем проверку значения разобранного атрибута
	ASSERT_EQ(document.element().attribute("k"), "да\u20AC");
}
/**
 * @brief Проверка отмены объявления пространства имён по умолчанию
 *
 * @details Узел без пространства имён, вложенный в узел с пространством по умолчанию,
 *          записывается вместе с отменой объявления: без неё он попал бы в пространство
 *          родителя, и прочитанное обратно дерево с записанным не совпало бы
 *
 */
TEST(CodecXmlWriter, NamespaceUndeclaration) {
	/**
	 * @brief Метод кругового хода дерева разметки
	 *
	 * @param text исходный текст разметки
	 * @return     записанный обратно текст разметки
	 *
	 */
	const auto trip = [](const string & text) noexcept -> string {
		// Объект дерева разметки
		xml::document_t document;
		// Если разбор текста разметки выполнить не удалось, выводим пустой текст
		if(!document.parse(text)) return string();
		// Объект записи текста разметки
		xml::writer_t writer;
		// Если запись дерева разметки выполнить не удалось, выводим пустой текст
		if(!writer.element(document.root())) return string();
		// Выводим записанный обратно текст разметки
		return string(writer.text());
	};
	// Выполняем проверку записи отмены объявления пространства имён по умолчанию
	ASSERT_EQ(trip("<a xmlns=\"urn:x\"><b xmlns=\"\"/></a>"), "<a xmlns=\"urn:x\"><b xmlns=\"\"/></a>");
	// Выполняем проверку записи отмены при наличии у узла собственных атрибутов
	ASSERT_EQ(trip("<a xmlns=\"urn:x\"><b xmlns=\"\" k=\"1\"/></a>"), "<a xmlns=\"urn:x\"><b xmlns=\"\" k=\"1\"/></a>");
	// Выполняем проверку записи отмены под сменённым пространством имён по умолчанию
	ASSERT_EQ(trip("<a xmlns=\"urn:x\"><b xmlns=\"urn:y\"><c xmlns=\"\"/></b></a>"), "<a xmlns=\"urn:x\"><b xmlns=\"urn:y\"><c xmlns=\"\"/></b></a>");
	// Объект дерева разметки
	xml::document_t document;
	// Выполняем разбор записанного обратно текста разметки
	ASSERT_TRUE(document.parse(trip("<a xmlns=\"urn:x\"><b xmlns=\"\"/></a>")));
	// Выполняем проверку того, что вложенный узел пространства имён не получил
	ASSERT_TRUE(document.element().first().name().uri.empty());
}
/**
 * @brief Проверка выдачи кодов ошибок записи, не выдававшихся ни одной проверкой
 *
 * @details Замер охвата показал, что до этих ходов не доходило ни одно испытание.
 *          Ход, ни разу не исполнявшийся, не проверен ничем: код ошибки в нём может
 *          не отвечать поводу вовсе
 *
 */
TEST(CodecXmlWriter, ErrorCodes) {
	/**
	 * Выполняем проверку отклонения ошибочно построенного префикса объявления
	 */
	{
		// Объект записи текста разметки
		xml::writer_t writer;
		// Выполняем открытие узла разметки
		ASSERT_TRUE(writer.open("a"));
		// Выполняем проверку отклонения объявления с ошибочно построенным префиксом
		ASSERT_FALSE(writer.binding("не:префикс", "urn:x"));
		// Выполняем проверку выданного кода ошибки записи
		ASSERT_EQ(writer.error(), xml::error_t::INVALID_PREFIX);
	}
	/**
	 * Выполняем проверку отклонения записи непригодного узла дерева
	 */
	{
		// Объект записи текста разметки
		xml::writer_t writer;
		// Непригодный узел дерева разметки
		const xml::node_t node;
		// Выполняем проверку отклонения записи непригодного узла
		ASSERT_FALSE(writer.element(node));
		// Выполняем проверку выданного кода ошибки записи
		ASSERT_EQ(writer.error(), xml::error_t::INTERNAL);
	}
}
TEST(CodecXmlWriter, Malformed2) {
	/**
	 * Выполняем проверку отклонения повторного имени атрибута
	 */
	{
		// Объект записи текста разметки
		xml::writer_t writer;
		// Выполняем открытие узла разметки
		ASSERT_TRUE(writer.open("a"));
		// Выполняем запись атрибута узла
		ASSERT_TRUE(writer.attribute("id", "1"));
		// Выполняем проверку отклонения повторного имени атрибута
		ASSERT_FALSE(writer.attribute("id", "2"));
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), xml::error_t::DUPLICATE_ATTRIBUTE);
	}
	/**
	 * Выполняем проверку отклонения повторного объявления префикса
	 */
	{
		// Объект записи текста разметки
		xml::writer_t writer;
		// Выполняем открытие узла разметки
		ASSERT_TRUE(writer.open("a"));
		// Выполняем объявление пространства имён
		ASSERT_TRUE(writer.binding("p", "urn:1"));
		// Выполняем проверку отклонения повторного объявления префикса
		ASSERT_FALSE(writer.binding("p", "urn:2"));
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), xml::error_t::DUPLICATE_ATTRIBUTE);
	}
	/**
	 * Выполняем проверку отклонения объявления пространства имён атрибутом
	 */
	{
		// Объект записи текста разметки
		xml::writer_t writer;
		// Выполняем открытие узла разметки
		ASSERT_TRUE(writer.open("a"));
		// Выполняем проверку отклонения объявления пространства имён атрибутом
		ASSERT_FALSE(writer.attribute("xmlns", "urn:x"));
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), xml::error_t::INVALID_ATTRIBUTE);
	}
	/**
	 * Выполняем проверку отклонения недопустимых знаков дословного содержимого
	 */
	{
		// Объект записи текста разметки
		xml::writer_t writer;
		// Выполняем открытие узла разметки
		ASSERT_TRUE(writer.open("a"));
		// Выполняем проверку отклонения управляющего знака в примечании
		ASSERT_FALSE(writer.comment(string("до\x01после")));
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), xml::error_t::INVALID_CHARACTER);
	}
	/**
	 * Выполняем проверку отклонения ошибочной последовательности в дословном разделе
	 */
	{
		// Объект записи текста разметки
		xml::writer_t writer;
		// Выполняем открытие узла разметки
		ASSERT_TRUE(writer.open("a"));
		// Выполняем проверку отклонения ошибочной последовательности кодировки
		ASSERT_FALSE(writer.cdata(string("до\xFF" "после")));
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), xml::error_t::INVALID_ENCODING);
	}
	/**
	 * Выполняем проверку отклонения управляющего знака в указании обработчику
	 */
	{
		// Объект записи текста разметки
		xml::writer_t writer;
		// Выполняем проверку отклонения управляющего знака в указании обработчику
		ASSERT_FALSE(writer.processing("php", string("до\x0C" "после")));
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), xml::error_t::INVALID_CHARACTER);
	}
}
/**
 * @brief Проверка назначения префиксов пространств имён
 *
 * @details Запись назначает префиксы самостоятельно, когда пространство имён ещё не
 *          связано ни с одним из них. Назначенный префикс не вправе совпасть с уже
 *          занятым, а поиск действующего префикса обязан учитывать перекрытие
 *          связывания более близким объявлением: иначе записанное имя при обратном
 *          чтении окажется в чужом пространстве имён
 *
 */
TEST(CodecXmlWriter, Prefixes) {
	/**
	 * Выполняем проверку обхода занятого префикса при самостоятельном назначении
	 */
	{
		// Объект записи текста разметки
		xml::writer_t writer;
		// Выполняем открытие узла разметки
		ASSERT_TRUE(writer.open("root"));
		// Выполняем объявление пространства имён с префиксом, назначаемым записью
		ASSERT_TRUE(writer.binding("n1", "urn:a"));
		// Выполняем запись атрибута в ещё не связанном пространстве имён
		ASSERT_TRUE(writer.attribute("id", "1", "urn:b"));
		// Выполняем закрытие узла разметки
		ASSERT_TRUE(writer.close());
		// Выполняем завершение записи текста разметки
		ASSERT_TRUE(writer.complete());
		// Объект дерева разметки
		xml::document_t document;
		// Выполняем разбор записанного текста разметки
		ASSERT_TRUE(document.parse(writer.text())) << writer.text() << " -> " << xml::message(document.error());
		// Выполняем проверку пространства имён записанного атрибута
		ASSERT_EQ(document.element().attribute("id", "urn:b"), "1") << writer.text();
	}
	/**
	 * Выполняем проверку учёта перекрытия связывания вложенным узлом
	 */
	{
		// Объект записи текста разметки
		xml::writer_t writer;
		// Выполняем открытие корневого узла разметки
		ASSERT_TRUE(writer.open("root"));
		// Выполняем объявление пространства имён при корневом узле
		ASSERT_TRUE(writer.binding("p", "urn:a"));
		// Выполняем открытие вложенного узла разметки
		ASSERT_TRUE(writer.open("mid"));
		// Выполняем перевязывание того же префикса на иное пространство имён
		ASSERT_TRUE(writer.binding("p", "urn:b"));
		// Выполняем открытие узла в перекрытом пространстве имён
		ASSERT_TRUE(writer.open("leaf", "urn:a"));
		// Выполняем закрытие вложенного узла разметки
		ASSERT_TRUE(writer.close());
		// Выполняем закрытие вложенного узла разметки
		ASSERT_TRUE(writer.close());
		// Выполняем закрытие корневого узла разметки
		ASSERT_TRUE(writer.close());
		// Выполняем завершение записи текста разметки
		ASSERT_TRUE(writer.complete());
		// Объект дерева разметки
		xml::document_t document;
		// Выполняем разбор записанного текста разметки
		ASSERT_TRUE(document.parse(writer.text())) << writer.text() << " -> " << xml::message(document.error());
		// Выполняем поиск записанного узла
		const xml::node_t leaf = document.element().find("leaf", "urn:a");
		// Выполняем проверку пространства имён записанного узла
		ASSERT_TRUE(leaf.valid()) << writer.text();
	}
}

/**
 * @brief Проверка отведения места под собираемый текст разметки
 *
 * @details Отведение места наперёд на итог записи не влияет: оно избавляет лишь от
 *          перекладывания собранного при росте накопителя. Занижение размера отказом
 *          не является - недостающее доводится обычным ростом
 *
 */
TEST(CodecXmlWriter, Reserve) {
	// Объект записи текста разметки
	xml::writer_t writer;
	// Выполняем отведение места под собираемый текст разметки
	writer.reserve(4096);
	// Выполняем проверку того, что отведённого места хватает
	ASSERT_GE(writer.text().capacity(), static_cast <size_t> (4096));
	// Выполняем открытие корневого узла разметки
	ASSERT_TRUE(writer.open("root"));
	// Выполняем запись содержимого узла разметки
	ASSERT_TRUE(writer.text("Москва"));
	// Выполняем закрытие корневого узла разметки
	ASSERT_TRUE(writer.close());
	// Выполняем завершение записи текста разметки
	ASSERT_TRUE(writer.complete());
	// Выполняем проверку записанного текста разметки
	ASSERT_EQ(writer.text(), "<root>Москва</root>");
	// Объект записи текста разметки с заниженным отведением места
	xml::writer_t narrow;
	// Выполняем заведомо заниженное отведение места
	narrow.reserve(1);
	// Выполняем открытие корневого узла разметки
	ASSERT_TRUE(narrow.open("root"));
	// Выполняем запись содержимого узла разметки
	ASSERT_TRUE(narrow.text("Москва"));
	// Выполняем закрытие корневого узла разметки
	ASSERT_TRUE(narrow.close());
	// Выполняем проверку того, что занижение на итог не повлияло
	ASSERT_EQ(narrow.text(), "<root>Москва</root>");
}

/**
 * @brief Проверка запрета отведённых договором пространств имён при записи
 *
 * @details Правила записи повторяют правила разбора: запись, их не соблюдающая,
 *          собрала бы текст, который собственное чтение отвергает, - и обнаружилось
 *          бы это уже у принимающей стороны
 *
 */
TEST(CodecXmlWriter, ReservedNamespaces) {
	/**
	 * @brief Объявления пространств имён, записи не подлежащие
	 *
	 */
	const struct {
		// Префикс объявляемого пространства имён
		const char * prefix;
		// Обозначение объявляемого пространства имён
		string uri;
	} items[] = {
		{"foo",   string(xml::XML_NAMESPACE)},
		{"foo",   string(xml::XMLNS_NAMESPACE)},
		{"",      string(xml::XML_NAMESPACE)},
		{"",      string(xml::XMLNS_NAMESPACE)},
		{"xmlns", string("urn:x")},
		{"foo",   string()}
	};
	/**
	 * Выполняем перебор всех объявлений пространств имён
	 */
	for(const auto & item : items){
		// Объект записи текста разметки
		xml::writer_t writer;
		// Выполняем открытие корневого узла разметки
		ASSERT_TRUE(writer.open("root"));
		// Выполняем проверку отказа записи объявления пространства имён
		ASSERT_FALSE(writer.binding(item.prefix, item.uri)) << item.prefix << " -> " << item.uri;
	}
	// Объект записи текста разметки
	xml::writer_t writer;
	// Выполняем открытие корневого узла разметки
	ASSERT_TRUE(writer.open("root"));
	// Выполняем проверку того, что обычное объявление записи подлежит
	ASSERT_TRUE(writer.binding("ok", "urn:x"));
	// Выполняем закрытие корневого узла разметки
	ASSERT_TRUE(writer.close());
	// Объект дерева разметки
	xml::document_t document;
	// Выполняем проверку того, что записанное собственное чтение принимает
	ASSERT_TRUE(document.parse(writer.text())) << writer.text();
}
/**
 * @brief Проверка отказов записи, не выдававшихся ни одной проверкой
 *
 * @details Замер охвата показал, что до этих ходов не доходило ни одно испытание:
 *          объявление разметки с признаком самодостаточности, отклонение содержимого
 *          вне корневого узла, отказ по недопустимой последовательности байтов и
 *          терминальность ошибки записи. Ход, ни разу не исполнявшийся, не проверен
 *          ничем: код ошибки в нём может не отвечать поводу вовсе
 *
 */
TEST(CodecXmlWriter, Refusals) {
	/**
	 * Выполняем проверку записи объявления разметки с признаком самодостаточности
	 */
	{
		// Объект записи текста разметки
		xml::writer_t writer;
		// Выполняем запись объявления самодостаточного текста разметки
		ASSERT_TRUE(writer.declaration(xml::standalone_t::YES));
		// Выполняем проверку записанного объявления разметки
		ASSERT_NE(writer.text().find("standalone=\"yes\""), string::npos) << writer.text();
		// Выполняем проверку отклонения повторного объявления разметки
		ASSERT_FALSE(writer.declaration());
		// Выполняем проверку выданного кода ошибки записи
		ASSERT_EQ(writer.error(), xml::error_t::INVALID_DECLARATION);
	}
	/**
	 * Выполняем проверку записи объявления зависящего от подмножества текста
	 */
	{
		// Объект записи текста разметки
		xml::writer_t writer;
		// Выполняем запись объявления зависящего от внешнего подмножества текста
		ASSERT_TRUE(writer.declaration(xml::standalone_t::NO));
		// Выполняем проверку записанного объявления разметки
		ASSERT_NE(writer.text().find("standalone=\"no\""), string::npos) << writer.text();
	}
	/**
	 * Выполняем проверку отклонения содержимого вне корневого узла
	 */
	{
		// Объект записи текста разметки
		xml::writer_t writer;
		// Выполняем проверку отклонения текстового содержимого вне корневого узла
		ASSERT_FALSE(writer.text("содержимое"));
		// Выполняем проверку выданного кода ошибки записи
		ASSERT_EQ(writer.error(), xml::error_t::CONTENT_OUTSIDE_ROOT);
	}
	/**
	 * Выполняем проверку отклонения раздела дословного текста вне корневого узла
	 */
	{
		// Объект записи текста разметки
		xml::writer_t writer;
		// Выполняем проверку отклонения раздела дословного текста вне корневого узла
		ASSERT_FALSE(writer.cdata("содержимое"));
		// Выполняем проверку выданного кода ошибки записи
		ASSERT_EQ(writer.error(), xml::error_t::CONTENT_OUTSIDE_ROOT);
	}
	/**
	 * Выполняем проверку отклонения недопустимой последовательности байтов
	 */
	{
		// Объект записи текста разметки
		xml::writer_t writer;
		// Выполняем открытие узла разметки
		ASSERT_TRUE(writer.open("a"));
		// Выполняем проверку отклонения содержимого с оборванной последовательностью UTF-8
		ASSERT_FALSE(writer.text(string("bad\xD0", 4)));
		// Выполняем проверку выданного кода ошибки записи
		ASSERT_EQ(writer.error(), xml::error_t::INVALID_ENCODING);
		/**
		 * Выполняем проверку терминальности ошибки записи
		 *
		 * @note Всякое действие после отказа отвечает отказом, а собранный текст
		 *       наружу не отдаётся: он оборван посреди метки узла
		 */
		ASSERT_FALSE(writer.open("b"));
		ASSERT_FALSE(writer.close());
		ASSERT_FALSE(writer.comment("примечание"));
		ASSERT_FALSE(writer.complete());
		// Выполняем проверку того, что собранный текст наружу не отдаётся
		ASSERT_TRUE(writer.text().empty());
	}
	/**
	 * Выполняем проверку отклонения знака, недопустимого в разметке
	 */
	{
		// Объект записи текста разметки
		xml::writer_t writer;
		// Выполняем открытие узла разметки
		ASSERT_TRUE(writer.open("a"));
		// Выполняем проверку отклонения содержимого с управляющим знаком
		ASSERT_FALSE(writer.text(string("bad\x01", 4)));
		// Выполняем проверку выданного кода ошибки записи
		ASSERT_EQ(writer.error(), xml::error_t::INVALID_CHARACTER);
	}
	/**
	 * Выполняем проверку отклонения ошибочно построенного имени узла
	 */
	{
		// Объект записи текста разметки
		xml::writer_t writer;
		// Выполняем проверку отклонения имени узла, начатого цифрой
		ASSERT_FALSE(writer.open("1a"));
		// Выполняем проверку выданного кода ошибки записи
		ASSERT_EQ(writer.error(), xml::error_t::INVALID_NAME);
	}
	/**
	 * Выполняем проверку отклонения атрибута, записываемого после содержимого
	 */
	{
		// Объект записи текста разметки
		xml::writer_t writer;
		// Выполняем открытие узла разметки
		ASSERT_TRUE(writer.open("a"));
		// Выполняем запись содержимого узла разметки
		ASSERT_TRUE(writer.text("содержимое"));
		/**
		 * Выполняем проверку отклонения атрибута после записанного содержимого
		 *
		 * @note Метка узла к этому мигу уже завершена, и дописывать в неё нечего
		 */
		ASSERT_FALSE(writer.attribute("k", "v"));
		// Выполняем проверку выданного кода ошибки записи
		ASSERT_EQ(writer.error(), xml::error_t::INVALID_ATTRIBUTE);
	}
	/**
	 * Выполняем проверку удержания предела глубины вложенности узлов
	 *
	 * @note Предел задаётся настройками и по умолчанию снят: обход дерева ведётся
	 *       собственным стеком, и держать здесь предел разбора значило бы отвергать
	 *       дерево, разобранное с поднятым пределом глубины
	 */
	{
		// Настройки записи текста разметки с заданным пределом глубины
		xml::writer_t::settings_t settings;
		// Выполняем установку предела глубины вложенности узлов
		settings.maxDepth = xml::MAX_DEPTH;
		// Объект записи текста разметки
		xml::writer_t writer(settings);
		// Признак достижения предела глубины вложенности узлов
		bool reached = false;
		/**
		 * Выполняем открытие узлов разметки до предела глубины вложенности
		 */
		for(uint32_t i = 0; i <= xml::MAX_DEPTH; i++){
			/**
			 * Если открыть очередной узел разметки не удалось
			 */
			if(!writer.open("a")){
				// Запоминаем достижение предела глубины вложенности узлов
				reached = true;
				// Выходим из открытия узлов разметки
				break;
			}
		}
		// Выполняем проверку достижения предела глубины вложенности узлов
		ASSERT_TRUE(reached);
		// Выполняем проверку выданного кода ошибки записи
		ASSERT_EQ(writer.error(), xml::error_t::DEPTH_EXCEEDED);
	}
	/**
	 * Выполняем проверку записи поддерева под сохранением пробелов предка
	 *
	 * @note Записывать можно всякий узел дерева, а не только корень, и обращение с
	 *       пробельным содержимым задаётся у него зачастую выше по дереву: приняв
	 *       такой узел за обычный, запись расставила бы внутри него отступы
	 */
	{
		// Объект дерева разметки
		xml::document_t document;
		// Выполняем проверку того, что разбор текста разметки удался
		ASSERT_TRUE(document.parse("<r xml:space=\"preserve\"><a> <b/> </a></r>"));
		// Получаем корневой узел дерева разметки
		const xml::node_t root = document.element();
		// Получаем узел поддерева, лежащий под сохранением пробелов
		const xml::node_t node = root.first();
		// Выполняем проверку того, что узел поддерева найден
		ASSERT_TRUE(node.valid());
		// Настройки нарядной записи текста разметки
		xml::writer_t::settings_t settings;
		// Выполняем установку нарядного вида записи
		settings.format = xml::format_t::PRETTY;
		// Объект записи текста разметки
		xml::writer_t writer(settings);
		// Выполняем проверку того, что запись поддерева удалась
		ASSERT_TRUE(writer.element(node));
		// Выполняем проверку завершённости собранного текста разметки
		ASSERT_TRUE(writer.complete());
		// Выполняем проверку сохранности значимого пробельного содержимого
		ASSERT_EQ(writer.text(), "<a> <b/> </a>");
	}
	/**
	 * Выполняем проверку записи дерева глубже прежнего жёсткого предела
	 *
	 * @note Разобранное обязано записываться обратно: глубина дерева задаётся тем, кто
	 *       его построил, и запись собственным стеком обходит его целиком
	 */
	{
		// Глубина вложенности узлов проверяемого дерева разметки
		const uint32_t depth = (xml::MAX_DEPTH * 8);
		// Собираемый текст разметки заданной глубины вложенности
		string text;
		// Выполняем сборку открывающих меток узлов разметки
		for(uint32_t i = 0; i < depth; i++) text.append("<a>");
		// Выполняем добавление содержимого самого глубокого узла
		text.append("текст");
		// Выполняем сборку закрывающих меток узлов разметки
		for(uint32_t i = 0; i < depth; i++) text.append("</a>");
		// Настройки разбора с поднятым пределом глубины вложенности
		xml::reader_t::settings_t reading;
		// Выполняем поднятие предела глубины вложенности узлов
		reading.maxDepth = (depth + 1);
		// Объект дерева разметки
		xml::document_t document;
		// Выполняем проверку того, что разбор глубокого текста разметки удался
		ASSERT_TRUE(document.parse(text, reading));
		// Объект записи текста разметки
		xml::writer_t writer;
		// Выполняем проверку того, что запись глубокого дерева разметки удалась
		ASSERT_TRUE(writer.element(document.root()));
		// Выполняем проверку завершённости собранного текста разметки
		ASSERT_TRUE(writer.complete());
		// Выполняем проверку совпадения записанного текста с исходным
		ASSERT_EQ(writer.text(), text);
	}
}
/**
 * @brief Проверка повторяемости нарядной записи
 *
 * @details Отступы нарядной записи ложатся в текст пробельным содержимым, и дерево,
 *          собранное разбором такого текста, несёт их наравне с прочим содержимым.
 *          Записывать их снова нельзя: следующая запись отступила бы уже от них, и
 *          разметка росла бы пустыми строками с каждым переходом текст→дерево→текст.
 *          Внутри же узлов, где пробельные знаки значимы, отступы не ставятся вовсе
 *
 * @note Найдено разбором кодека: прежде нарядная запись росла на каждом заходе, а
 *       узлы со смешанным содержимым получали перевод строки прямо посреди текста
 *
 */
TEST(CodecXmlWriter, PrettyStability) {
	/**
	 * @brief Метод кругового хода текст→дерево→текст нарядной записью
	 *
	 * @param text      исходный текст разметки
	 * @param separator знак отступа нарядной записи
	 * @param namespaces признак разрешения префиксов по договору о пространствах имён
	 * @return          записанный обратно текст разметки
	 *
	 */
	const auto trip = [](const string & text, const xml::separator_t separator, const bool namespaces = true) noexcept -> string {
		// Объект дерева разметки
		xml::document_t document;
		// Настройки разбора текста разметки
		xml::reader_t::settings_t reading;
		// Выполняем установку разрешения префиксов по договору о пространствах имён
		reading.namespaces = namespaces;
		// Если разбор текста разметки выполнить не удалось, выводим признак отказа
		if(!document.parse(text, reading)) return "ОТКАЗ РАЗБОРА";
		// Настройки нарядной записи текста разметки
		xml::writer_t::settings_t settings;
		// Выполняем установку нарядного вида записи
		settings.format = xml::format_t::PRETTY;
		// Выполняем установку знака отступа
		settings.separator = separator;
		// Объект записи текста разметки
		xml::writer_t writer(settings);
		// Если запись дерева разметки выполнить не удалось, выводим признак отказа
		if(!writer.element(document.root()) || !writer.complete()) return "ОТКАЗ ЗАПИСИ";
		// Выводим записанный обратно текст разметки
		return writer.text();
	};
	/**
	 * @brief Тексты разметки, повторяемость записи которых проверяется
	 *
	 */
	const char * items[] = {
		"<list><item>первый</item><item>второй</item></list>",
		"<r><a/><!-- примечание --><?цель данные?><b><c>текст</c></b></r>",
		"<r>\n\t<a>\n\t\t<b/>\n\t</a>\n</r>",
		"<r><a>текст</a>хвост<b/></r>",
		"<r xml:space=\"preserve\"><a/> <b/></r>"
	};
	/**
	 * Выполняем перебор всех знаков отступа нарядной записи
	 */
	for(const xml::separator_t separator : {xml::separator_t::TABS, xml::separator_t::SPACES, xml::separator_t::NONE}){
		/**
		 * Выполняем перебор всех проверяемых текстов разметки
		 */
		for(const char * item : items){
			// Выполняем первую нарядную запись дерева разметки
			const string first = trip(item, separator);
			// Выполняем проверку того, что запись дерева разметки удалась
			ASSERT_NE(first.rfind("ОТКАЗ", 0), 0u) << item;
			// Выполняем повторную нарядную запись дерева разметки
			const string second = trip(first, separator);
			// Выполняем проверку повторяемости нарядной записи
			ASSERT_EQ(first, second) << item;
			// Выполняем третью нарядную запись дерева разметки
			ASSERT_EQ(trip(second, separator), second) << item;
		}
	}
	/**
	 * Выполняем проверку сохранности смешанного содержимого
	 *
	 * @note Узел, несущий разом текст и вложенные узлы, записывается в одну строку:
	 *       перевод строки перед вложенным узлом попал бы в текст соседа
	 */
	ASSERT_EQ(trip("<p>Здравствуй, <b>мир</b>!</p>", xml::separator_t::TABS), "<p>Здравствуй, <b>мир</b>!</p>");
	/**
	 * Выполняем проверку сохранности пробельного содержимого при его сохранении
	 *
	 * @note Договор велит держать пробельное содержимое узла, помеченного отведённым
	 *       атрибутом, и отступов внутри такого узла быть не должно
	 */
	ASSERT_EQ(
		trip("<r xml:space=\"preserve\"><a/> <b/></r>", xml::separator_t::TABS),
		"<r xml:space=\"preserve\"><a/> <b/></r>"
	);
	/**
	 * Выполняем проверку сохранности пробельного содержимого без пространств имён
	 *
	 * @note Обращение с пробельным содержимым отведено самим договором о разметке, а не
	 *       договором о пространствах имён: атрибут действует и там, где префиксы не
	 *       разрешаются вовсе, и имя его в дереве не разделено
	 */
	ASSERT_EQ(
		trip("<r xml:space=\"preserve\"><a/> <b/></r>", xml::separator_t::TABS, false),
		"<r xml:space=\"preserve\"><a/> <b/></r>"
	);
	/**
	 * Выполняем проверку толкования значения, договором не отведённого
	 *
	 * @note Договор отводит атрибуту ровно два значения, и всякое иное запись толкует
	 *       отменой сохранения - тем же самым образом, каким его толкует чтение
	 */
	ASSERT_EQ(
		trip("<r xml:space=\"preserve\"><s xml:space=\"неведомо\"><a/><b/></s></r>", xml::separator_t::TABS),
		"<r xml:space=\"preserve\"><s xml:space=\"неведомо\">\n\t\t<a/>\n\t\t<b/>\n\t</s></r>"
	);
	/**
	 * Выполняем проверку сохранности раздела дословного текста из одних пробелов
	 *
	 * @note Раздел дословного текста значим сам по себе, каким бы ни было его
	 *       содержимое: отступ рядом с ним переменил бы содержимое объемлющего узла
	 */
	ASSERT_EQ(
		trip("<r><a/><![CDATA[   ]]><b/></r>", xml::separator_t::TABS),
		"<r><a/><![CDATA[   ]]><b/></r>"
	);
	// Выполняем проверку расстановки отступов у узла без смешанного содержимого
	ASSERT_EQ(trip("<r><a/><b/></r>", xml::separator_t::TABS), "<r>\n\t<a/>\n\t<b/>\n</r>");
}
/**
 * @brief Проверка отмены пространства имён по умолчанию, назначенной записью самостоятельно
 *
 * @details Отмена может прийти двумя путями: объявлением, прочитанным из исходного
 *          текста, и назначением самой записи - когда пользователь открывает узел без
 *          обозначения пространства имён внутри области, где пространство по умолчанию
 *          объявлено. Второй путь набор не проверял вовсе: карта покрытия показала
 *          ветвь назначения нетронутой, тогда как первый закреплён проверкой
 *          «NamespaceUndeclaration»
 *
 * @note Без отмены имя без префикса попало бы в объявленное по умолчанию пространство
 *       имён, и записанный текст означал бы не то, что просили записать. Оттого
 *       проверяется не одна запись, а и обратный разбор её
 *
 */
TEST(CodecXmlWriter, NamespaceUndeclarationSynthesized) {
	// Объект записи текста разметки
	xml::writer_t writer;
	// Объявляемое узлом связывание пространства имён по умолчанию
	xml::binding_t binding;
	// Запоминаем обозначение объявляемого пространства имён
	binding.uri = "urn:x";
	// Собираем перечень объявляемых узлом связываний
	const vector <xml::binding_t> declares = {binding};
	// Выполняем открытие узла с пространством имён по умолчанию
	ASSERT_TRUE(writer.open("a", "urn:x", declares));
	// Выполняем запись вложенного узла без пространства имён
	ASSERT_TRUE(writer.element("b", "з"));
	// Выполняем закрытие узла с пространством имён по умолчанию
	ASSERT_TRUE(writer.close());
	// Выполняем проверку записанного текста разметки
	ASSERT_EQ(string(writer.text()), "<a xmlns=\"urn:x\"><b xmlns=\"\">з</b></a>");
	// Объект дерева разметки
	xml::document_t document;
	// Выполняем разбор записанного обратно текста разметки
	ASSERT_TRUE(document.parse(writer.text()));
	// Выполняем проверку того, что вложенный узел пространства имён не получил
	ASSERT_TRUE(document.element().first().name().uri.empty());
	// Выполняем проверку того, что внешний узел пространство имён сохранил
	ASSERT_EQ(document.element().name().uri, "urn:x");
}
/**
 * @brief Проверка отказа записи имени в отведённом договором пространстве имён
 *
 * @details Пространство имён, отведённое договором самим объявлениям, именем узла
 *          либо атрибута занято быть не может. Проверка «ReservedNamespaces»
 *          закрепляет отказ объявления такого связывания, но не отказ назначения
 *          префикса имени: карта покрытия показала вторую ветвь нетронутой, а
 *          добраться до неё можно и без объявления - обозначением, заданным прямо
 *          при открытии узла
 *
 */
TEST(CodecXmlWriter, ReservedNamespaceName) {
	// Объект записи текста разметки
	xml::writer_t writer;
	// Выполняем проверку отказа открытия узла в отведённом пространстве имён
	ASSERT_FALSE(writer.open("a", xml::XMLNS_NAMESPACE));
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(writer.error(), xml::error_t::INVALID_NAMESPACE);
	// Объект записи текста разметки
	xml::writer_t second;
	// Выполняем открытие корневого узла разметки
	ASSERT_TRUE(second.open("a"));
	// Выполняем проверку отказа записи атрибута в отведённом пространстве имён
	ASSERT_FALSE(second.attribute("x", "1", xml::XMLNS_NAMESPACE));
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(second.error(), xml::error_t::INVALID_NAMESPACE);
}
/**
 * @brief Проверка отказов записи текста разметки
 *
 * @note Запись отвергает построения, разбором не восстановимые: имена негодные,
 *       содержимое, несущее знаки завершения своего же вида, и обращения, порядку
 *       записи противоречащие
 * @warning Отказ ЗАПОМИНАЕТСЯ, и дальнейшая запись отвергается вся: текст, начатый
 *          ошибочно, годным уже не станет, а выдача его половины была бы хуже отказа
 */
TEST(CodecXmlWriter, RefusalCodes) {
	/**
	 * Негодные имена узла и свойства
	 */
	{
		// Поток записи текста разметки
		xml::writer_t writer;
		// Выполняем проверку отклонения имени узла пустого
		ASSERT_FALSE(writer.open(""));
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), xml::error_t::INVALID_NAME);
	}
	{
		// Поток записи текста разметки
		xml::writer_t writer;
		// Выполняем проверку отклонения имени узла, цифрой начатого
		ASSERT_FALSE(writer.open("1a"));
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), xml::error_t::INVALID_NAME);
	}
	{
		// Поток записи текста разметки
		xml::writer_t writer;
		// Выполняем открытие узла разметки
		ASSERT_TRUE(writer.open("r"));
		// Выполняем проверку отклонения имени свойства пустого
		ASSERT_FALSE(writer.attribute("", "1"));
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), xml::error_t::INVALID_NAME);
	}
	/**
	 * Обращения, порядку записи противоречащие
	 */
	{
		// Поток записи текста разметки
		xml::writer_t writer;
		// Выполняем проверку отклонения свойства вне открытой метки
		ASSERT_FALSE(writer.attribute("a", "1"));
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), xml::error_t::INVALID_ATTRIBUTE);
	}
	{
		// Поток записи текста разметки
		xml::writer_t writer;
		// Выполняем проверку отклонения закрытия без открытия
		ASSERT_FALSE(writer.close());
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), xml::error_t::UNEXPECTED_CLOSE_TAG);
	}
	{
		// Поток записи текста разметки
		xml::writer_t writer;
		// Выполняем открытие корневого узла
		ASSERT_TRUE(writer.open("r"));
		// Выполняем закрытие корневого узла
		ASSERT_TRUE(writer.close());
		// Выполняем проверку отклонения второго корневого узла
		ASSERT_FALSE(writer.open("q"));
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), xml::error_t::MULTIPLE_ROOTS);
	}
	{
		// Поток записи текста разметки
		xml::writer_t writer;
		// Выполняем открытие корневого узла
		ASSERT_TRUE(writer.open("r"));
		// Выполняем проверку отклонения завершения записи при незакрытом узле
		ASSERT_FALSE(writer.complete());
	}
	/**
	 * Отведённый договором префикс пространства имён
	 */
	{
		// Поток записи текста разметки
		xml::writer_t writer;
		// Выполняем открытие корневого узла
		ASSERT_TRUE(writer.open("r"));
		// Выполняем проверку отклонения связывания отведённого префикса
		ASSERT_FALSE(writer.binding("xml", "urn:u"));
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), xml::error_t::RESERVED_PREFIX);
	}
	/**
	 * Содержимое, несущее знаки завершения своего же вида
	 *
	 * @note Знаки эти уйти в текст не могут ничем: ни примечание, ни раздел дословного
	 *       текста, ни указание обработчику замен внутри себя не допускают, и запись
	 *       такого содержимого дала бы текст, разбором не восстановимый
	 */
	{
		// Поток записи текста разметки
		xml::writer_t writer;
		// Выполняем открытие корневого узла
		ASSERT_TRUE(writer.open("r"));
		// Выполняем проверку отклонения указания обработчику со знаками завершения
		ASSERT_FALSE(writer.processing("pi", "a?>b"));
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), xml::error_t::INVALID_PROCESSING);
	}
	{
		// Поток записи текста разметки
		xml::writer_t writer;
		// Выполняем открытие корневого узла
		ASSERT_TRUE(writer.open("r"));
		// Выполняем проверку отклонения примечания с двойным знаком отделения
		ASSERT_FALSE(writer.comment("a--b"));
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), xml::error_t::INVALID_COMMENT);
	}
	{
		// Поток записи текста разметки
		xml::writer_t writer;
		// Выполняем открытие корневого узла
		ASSERT_TRUE(writer.open("r"));
		// Выполняем проверку отклонения раздела дословного текста со знаками завершения
		ASSERT_FALSE(writer.cdata("a]]>b"));
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), xml::error_t::INVALID_CDATA);
	}
	{
		// Поток записи текста разметки
		xml::writer_t writer;
		// Выполняем открытие корневого узла
		ASSERT_TRUE(writer.open("r"));
		// Выполняем проверку отклонения содержимого со знаком, разметке недопустимым
		ASSERT_FALSE(writer.text(string("a\x01""b")));
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), xml::error_t::INVALID_CHARACTER);
	}
	/**
	 * Отказ запоминается на весь дальнейший ход записи
	 */
	{
		// Поток записи текста разметки
		xml::writer_t writer;
		// Выполняем обращение, отказом завершающееся
		ASSERT_FALSE(writer.open(""));
		// Выполняем проверку отклонения записи годного узла после отказа
		ASSERT_FALSE(writer.open("r"));
		// Выполняем проверку того, что записанного текста не появилось
		ASSERT_TRUE(writer.text().empty());
	}
}

/**
 * @brief Проверка прекращения записи ошибкой
 *
 * @details Запись, единожды прекращённая ошибкой, обязана отвечать отказом на ВСЯКОЕ
 *          дальнейшее действие: иначе вызывающий, не проверивший исход одного шага,
 *          собрал бы обрывок текста, ошибки не заметив
 *
 * @note Проверка эта отыскана по карте покрытия: заслон стоял у каждого действия порознь,
 *       а набор не проходил его ни у одного
 *
 */
TEST(CodecXmlWriter, RefusalAfterError) {
	// Объект потоковой записи текста разметки
	xml::writer_t writer;
	// Выполняем ввод записи в состояние отказа именем с пробелом
	ASSERT_FALSE(writer.open("a b"));
	// Выполняем проверку кода ошибки записи
	ASSERT_EQ(writer.error(), xml::error_t::INVALID_NAME);
	// Выполняем проверку отказа установки свойства
	ASSERT_FALSE(writer.attribute("x", "1"));
	// Выполняем проверку отказа объявления связывания префикса
	ASSERT_FALSE(writer.binding("p", "u"));
	// Выполняем проверку отказа записи текстового содержимого
	ASSERT_FALSE(writer.text("t"));
	// Выполняем проверку отказа записи раздела дословного текста
	ASSERT_FALSE(writer.cdata("c"));
	// Выполняем проверку отказа записи примечания
	ASSERT_FALSE(writer.comment("c"));
	// Выполняем проверку отказа записи указания обработчику
	ASSERT_FALSE(writer.processing("p", "v"));
	// Выполняем проверку отказа закрытия узла разметки
	ASSERT_FALSE(writer.close());
	// Выполняем проверку отказа записи узла целиком
	ASSERT_FALSE(writer.element("a", "v"));
	// Выполняем проверку отказа записи объявления разметки
	ASSERT_FALSE(writer.declaration());
	// Выполняем проверку отказа открытия нового узла разметки
	ASSERT_FALSE(writer.open("b"));
	// Выполняем проверку незавершённости записи
	ASSERT_FALSE(writer.complete());
	// Выполняем проверку пустоты собранного текста
	ASSERT_TRUE(writer.text().empty());
}

/**
 * @brief Проверка отказов записи по составу подаваемого
 *
 * @details Места эти отысканы по карте покрытия: каждое отвергает своё построение, и набор
 *          не проходил ни одного из них
 *
 * @note Негодная последовательность байтов здесь всюду одна и та же - «C3 28»: первый байт
 *       объявляет двухбайтовую последовательность, а второй продолжающим не является
 *
 */
TEST(CodecXmlWriter, RefusalsByContent) {
	// Негодная последовательность байтов
	const string broken("\xC3\x28");
	/**
	 * Пустое имя узла отвергается
	 */
	{
		// Объект потоковой записи текста разметки
		xml::writer_t writer;
		// Выполняем проверку отказа открытия узла с пустым именем
		ASSERT_FALSE(writer.open(""));
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), xml::error_t::INVALID_NAME);
	}
	/**
	 * Негодная последовательность байтов отвергается всюду, куда её подают
	 */
	{
		// Выполняем перебор мест подачи негодной последовательности байтов
		for(uint32_t place = 0; place < 5; place++){
			// Объект потоковой записи текста разметки
			xml::writer_t writer;
			// Признак успешности записи
			bool result = true;
			/**
			 * Определяем место подачи негодной последовательности байтов
			 */
			switch(place){
				// Подаём негодную последовательность именем узла
				case 0: result = writer.open("a" + broken); break;
				// Подаём негодную последовательность обозначением пространства имён узла
				case 1: result = writer.open("a", broken); break;
				// Подаём негодную последовательность значением свойства
				case 2: result = (writer.open("a") && writer.attribute("x", broken)); break;
				// Подаём негодную последовательность обозначением связывания префикса
				case 3: result = (writer.open("a") && writer.binding("p", broken)); break;
				// Подаём негодную последовательность текстовым содержимым
				case 4: result = (writer.open("a") && writer.text(broken)); break;
			}
			// Выполняем проверку отказа записи
			ASSERT_FALSE(result) << place;
			/**
			 * Выполняем проверку кода ошибки записи
			 *
			 * @note Имя узла отвергается ИНЫМ кодом: допустимость знаков имени сличается
			 *       прежде перекодировки, и негодная последовательность байтов имени
			 *       недопустима сама по себе, безотносительно кодировки
			 */
			ASSERT_EQ(writer.error(), (place == 0 ? xml::error_t::INVALID_NAME : xml::error_t::INVALID_ENCODING)) << place;
			// Выполняем проверку пустоты собранного текста
			ASSERT_TRUE(writer.text().empty()) << place;
		}
	}
	/**
	 * Негодное обозначение среди объявлений узла отвергает открытие узла
	 *
	 * @warning Обозначение здесь обязано пережить вызов: связывание держит его
	 *          последовательностью знаков, памятью не владеющей
	 */
	{
		// Объект потоковой записи текста разметки
		xml::writer_t writer;
		// Собираемый перечень объявлений пространств имён узла
		vector <xml::binding_t> declares(1);
		// Устанавливаем объявляемый префикс
		declares[0].prefix = "p";
		// Устанавливаем негодное обозначение пространства имён
		declares[0].uri = broken;
		// Выполняем проверку отказа открытия узла
		ASSERT_FALSE(writer.open("a", "", declares));
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), xml::error_t::INVALID_ENCODING);
	}
	/**
	 * Повтор свойства отвергается и при имени с префиксом
	 *
	 * @note Имя с префиксом собирается записью иначе, и повтор его отсеивается своею
	 *       дорогою - не тою, какою отсеивается повтор имени без префикса
	 */
	{
		// Объект потоковой записи текста разметки
		xml::writer_t writer;
		// Выполняем открытие узла разметки
		ASSERT_TRUE(writer.open("a"));
		// Выполняем установку свойства в пространстве имён
		ASSERT_TRUE(writer.attribute("x", "1", "u"));
		// Выполняем проверку отказа повторной установки того же свойства
		ASSERT_FALSE(writer.attribute("x", "2", "u"));
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), xml::error_t::DUPLICATE_ATTRIBUTE);
	}
	/**
	 * Объявление связывания префикса вне открытой метки отвергается
	 *
	 * @note Объявление принадлежит метке узла, а не его содержимому: за завершением метки
	 *       ставить его уже некуда. Оттого отвергается оно и без единого открытого узла, и
	 *       следом за первым же содержимым внутри открытого
	 */
	{
		// Объект потоковой записи текста разметки
		xml::writer_t writer;
		// Выполняем проверку отказа объявления связывания без открытого узла
		ASSERT_FALSE(writer.binding("p", "u"));
		/**
		 * Объект второй потоковой записи текста разметки
		 *
		 * @warning Второй объект здесь обязателен: отказ переводит запись в состояние
		 *          ошибки ОКОНЧАТЕЛЬНО, и всякое действие следом отвергается независимо от
		 *          своего построения. Продолжение проверки прежним объектом сличало бы не
		 *          то, что задумано
		 */
		xml::writer_t other;
		// Выполняем открытие узла разметки
		ASSERT_TRUE(other.open("a"));
		// Выполняем проверку успешного объявления связывания открытою меткой
		ASSERT_TRUE(other.binding("p", "u"));
		// Выполняем запись текстового содержимого, метку завершающего
		ASSERT_TRUE(other.text("t"));
		// Выполняем проверку отказа объявления связывания за завершением метки
		ASSERT_FALSE(other.binding("q", "v"));
	}
	/**
	 * Негодное содержимое отвергает запись узла целиком
	 *
	 * @note Запись узла целиком собирает его тремя шагами, и отказ любого из них обязан
	 *       доходить до вызывающего: иначе узел остался бы открытым, а отказ - незамеченным
	 */
	{
		// Объект потоковой записи текста разметки
		xml::writer_t writer;
		// Выполняем проверку отказа записи узла с негодным содержимым
		ASSERT_FALSE(writer.element("a", broken));
		// Выполняем проверку кода ошибки записи
		ASSERT_EQ(writer.error(), xml::error_t::INVALID_ENCODING);
	}
}

/**
 * @brief Проверка записи указания обработчику и свёртки пустой метки
 *
 * @details Обозначение указания обработчику сличается ДОСЛОВНО - разрешение префиксов к
 *          нему не прилагается, - и отказы его отдельны от отказов имени узла
 *
 */
TEST(CodecXmlWriter, ProcessingTargetAndCollapse) {
	/**
	 * Обозначение указания обработчику, договором не дозволенное, отвергается
	 */
	{
		// Выполняем перебор недопустимых обозначений указания обработчику
		for(auto & target : vector <string> {
			// Пустое обозначение
			"",
			// Обозначение, начатое цифрой
			"1a",
			// Обозначение с негодной последовательностью байтов
			string("a\xC3\x28")
		}){
			// Объект потоковой записи текста разметки
			xml::writer_t writer;
			// Выполняем проверку отказа записи указания обработчику
			ASSERT_FALSE(writer.processing(target, "v")) << target;
			// Выполняем проверку кода ошибки записи
			ASSERT_EQ(writer.error(), xml::error_t::INVALID_PROCESSING) << target;
		}
	}
	/**
	 * Пустая метка узла записывается парой меток при отключённой свёртке
	 *
	 * @note Свёртка пустой метки - это НАСТРОЙКА, а не устройство: договор дозволяет обе
	 *       записи, и выбор оставлен вызывающему
	 */
	{
		// Настройки записи текста разметки
		xml::writer_t::settings_t settings;
		// Отключаем свёртку пустой метки узла
		settings.collapse = false;
		// Объект потоковой записи текста разметки
		xml::writer_t writer(settings);
		// Выполняем открытие узла разметки
		ASSERT_TRUE(writer.open("a"));
		// Выполняем закрытие узла разметки
		ASSERT_TRUE(writer.close());
		// Выполняем проверку записи пустого узла парою меток
		ASSERT_EQ(writer.text(), "<a></a>");
	}
	/**
	 * Свёртка пустой метки узла при настройке по умолчанию
	 */
	{
		// Объект потоковой записи текста разметки
		xml::writer_t writer;
		// Выполняем открытие узла разметки
		ASSERT_TRUE(writer.open("a"));
		// Выполняем закрытие узла разметки
		ASSERT_TRUE(writer.close());
		// Выполняем проверку свёртки пустого узла одною меткой
		ASSERT_EQ(writer.text(), "<a/>");
	}
}

/**
 * @brief Проверка объявлений пространств имён при записи
 *
 * @details Места эти отысканы по карте покрытия: набор объявлений по умолчанию записи не
 *          подавал вовсе, и вся дорога их обхождения не проходилась ни разу
 *
 * @warning Обозначения пространств имён здесь держатся ОТДЕЛЬНЫМИ переменными: связывание
 *          хранит последовательность знаков, памятью не владеющую, и временное значение
 *          погибло бы прежде вызова
 *
 */
TEST(CodecXmlWriter, NamespaceDeclarations) {
	// Обозначения пространств имён
	const string first("u1"), second("u2"), none("");
	/**
	 * Вложенный узел вне пространств имён отменяет объявление по умолчанию
	 *
	 * @note Без отмены узел достался бы пространству имён родителя: объявление по
	 *       умолчанию действует на всё поддерево, и молчание означало бы согласие
	 */
	{
		// Объект потоковой записи текста разметки
		xml::writer_t writer;
		// Собираемый перечень объявлений пространств имён узла
		vector <xml::binding_t> declares(1);
		// Устанавливаем объявление пространства имён по умолчанию
		declares[0].prefix = none;
		// Устанавливаем обозначение пространства имён
		declares[0].uri = first;
		// Выполняем открытие узла разметки в пространстве имён по умолчанию
		ASSERT_TRUE(writer.open("a", first, declares));
		// Выполняем открытие вложенного узла вне пространств имён
		ASSERT_TRUE(writer.open("b"));
		// Выполняем закрытие вложенного узла
		ASSERT_TRUE(writer.close());
		// Выполняем закрытие узла разметки
		ASSERT_TRUE(writer.close());
		// Выполняем проверку записи отмены объявления по умолчанию
		ASSERT_EQ(writer.text(), "<a xmlns=\"u1\"><b xmlns=\"\"/></a>");
	}
	/**
	 * Свойству объявления по умолчанию не достаётся
	 *
	 * @note Договор о пространствах имён относит объявление по умолчанию к именам УЗЛОВ, а
	 *       не свойств: свойство без префикса не принадлежит никакому пространству имён.
	 *       Оттого свойству в том же пространстве имён приходится порождать префикс, хотя
	 *       обозначение уже объявлено
	 */
	{
		// Объект потоковой записи текста разметки
		xml::writer_t writer;
		// Собираемый перечень объявлений пространств имён узла
		vector <xml::binding_t> declares(1);
		// Устанавливаем объявление пространства имён по умолчанию
		declares[0].prefix = none;
		// Устанавливаем обозначение пространства имён
		declares[0].uri = first;
		// Выполняем открытие узла разметки в пространстве имён по умолчанию
		ASSERT_TRUE(writer.open("a", first, declares));
		// Выполняем установку свойства в том же пространстве имён
		ASSERT_TRUE(writer.attribute("x", "1", first));
		// Выполняем закрытие узла разметки
		ASSERT_TRUE(writer.close());
		// Выполняем проверку порождения префикса свойству
		ASSERT_EQ(writer.text(), "<a xmlns=\"u1\" xmlns:n1=\"u1\" n1:x=\"1\"/>");
	}
	/**
	 * Порождаемый префикс обходит уже занятые имена
	 */
	{
		// Объект потоковой записи текста разметки
		xml::writer_t writer;
		// Собираемый перечень объявлений пространств имён узла
		vector <xml::binding_t> declares(1);
		// Устанавливаем объявляемый префикс, совпадающий с первым порождаемым
		declares[0].prefix = string_view("n1");
		// Устанавливаем обозначение пространства имён
		declares[0].uri = second;
		// Выполняем открытие узла разметки с объявлением префикса
		ASSERT_TRUE(writer.open("a", none, declares));
		// Выполняем установку свойства в ином пространстве имён
		ASSERT_TRUE(writer.attribute("x", "1", first));
		// Выполняем закрытие узла разметки
		ASSERT_TRUE(writer.close());
		// Выполняем проверку обхода занятого имени порождаемым префиксом
		ASSERT_EQ(writer.text(), "<a xmlns:n1=\"u2\" xmlns:n2=\"u1\" n2:x=\"1\"/>");
	}
	/**
	 * Свойство при двух объявлениях берёт то, что несёт префикс
	 */
	{
		// Объект потоковой записи текста разметки
		xml::writer_t writer;
		// Собираемый перечень объявлений пространств имён узла
		vector <xml::binding_t> declares(2);
		// Устанавливаем объявление пространства имён по умолчанию
		declares[0].prefix = none;
		// Устанавливаем обозначение пространства имён по умолчанию
		declares[0].uri = first;
		// Устанавливаем объявляемый префикс
		declares[1].prefix = string_view("p");
		// Устанавливаем обозначение пространства имён префикса
		declares[1].uri = second;
		// Выполняем открытие узла разметки с обоими объявлениями
		ASSERT_TRUE(writer.open("a", first, declares));
		// Выполняем установку свойства в пространстве имён префикса
		ASSERT_TRUE(writer.attribute("y", "1", second));
		// Выполняем закрытие узла разметки
		ASSERT_TRUE(writer.close());
		// Выполняем проверку выбора объявленного префикса, а не порождённого
		ASSERT_EQ(writer.text(), "<a xmlns=\"u1\" xmlns:p=\"u2\" p:y=\"1\"/>");
	}
}
