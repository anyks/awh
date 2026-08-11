/**
 * @file: writer.cpp
 * @date: 2026-08-01
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Автоматические тесты записи текста разметки — сборка конверта SOAP, экранирование
 *        содержимого, виды записи, отклонение неправильного построения и обратный ход
 *        «текст - дерево - текст - дерево»
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
	// Выполняем проверку расстановки отступов
	ASSERT_EQ(pretty.text(), "<r>\n  <i>1</i>\n  <i>2</i>\n</r>");
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
	 */
	{
		// Объект записи текста разметки
		xml::writer_t writer;
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
}
