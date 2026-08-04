/**
 * @file: reader.cpp
 * @date: 2026-08-01
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Автоматические тесты потокового чтения текста разметки — выдача событий разбора,
 *        разрешение пространств имён, подстановка сущностей, значения атрибутов по умолчанию
 *        и совпадение итога при подаче текста целиком и по одному байту
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
#include <codec/xml/reader.hpp>

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
 * @brief Метод разбора текста разметки с подачей кусками заданного размера
 *
 * @details События разбора записываются в сокращённом виде, пригодном для сличения:
 *          такая запись показывает и порядок событий, и их содержимое
 *
 * @param text     разбираемый текст разметки
 * @param step     размер куска подаваемого текста
 * @param error    код ошибки разбора
 * @param settings настройки разбора текста разметки
 * @return         запись полученных событий разбора
 *
 */
static string run(const string & text, const size_t step, xml::error_t & error, const xml::reader_t::settings_t & settings = xml::reader_t::settings_t()) noexcept {
	// Собираемая запись событий разбора
	string result;
	// Объект потокового чтения текста разметки
	xml::reader_t reader(settings);
	// Положение подачи в разбираемом тексте
	size_t offset = 0;
	/**
	 * Выполняем подачу текста разметки кусками
	 */
	for(;;){
		// Получаем размер очередного куска текста
		const size_t size = ((offset + step) > text.size() ? (text.size() - offset) : step);
		// Получаем признак последнего куска текста
		const bool end = ((offset + size) >= text.size());
		/**
		 * Если передачу куска текста выполнить не удалось
		 */
		if(!reader.feed(text.data() + offset, size, end))
			// Выходим из подачи текста
			break;
		// Выполняем переход к следующему куску текста
		offset += size;
		/**
		 * Выполняем перебор всех событий разбора
		 */
		while(reader.next()){
			/**
			 * Определяем вид полученного события разбора
			 */
			switch(static_cast <uint8_t> (reader.event())){
				// Если получено объявление разметки
				case static_cast <uint8_t> (xml::event_t::DECLARATION):
					result.append("[decl ").append(reader.text()).append("]");
				break;
				// Если получено описание типа документа
				case static_cast <uint8_t> (xml::event_t::DOCTYPE):
					result.append("[doctype ").append(reader.text()).append("]");
				break;
				// Если получено указание обработчику
				case static_cast <uint8_t> (xml::event_t::PROCESSING):
					result.append("[pi ").append(reader.name().local).append(" ").append(reader.text()).append("]");
				break;
				// Если получено примечание
				case static_cast <uint8_t> (xml::event_t::COMMENT):
					result.append("[!").append(reader.text()).append("]");
				break;
				// Если получено начало узла разметки
				case static_cast <uint8_t> (xml::event_t::ELEMENT_OPEN): {
					// Выполняем добавление имени узла
					result.append("<").append(reader.name().local);
					/**
					 * Если узел принадлежит пространству имён
					 */
					if(!reader.name().uri.empty())
						// Выполняем добавление обозначения пространства имён
						result.append("{").append(reader.name().uri).append("}");
					/**
					 * Выполняем перебор всех атрибутов узла
					 */
					for(const xml::attribute_t & attribute : reader.attributes()){
						// Выполняем добавление имени атрибута
						result.append(" ").append(attribute.name.local);
						/**
						 * Если атрибут принадлежит пространству имён
						 */
						if(!attribute.name.uri.empty())
							// Выполняем добавление обозначения пространства имён
							result.append("{").append(attribute.name.uri).append("}");
						// Выполняем добавление значения атрибута
						result.append("=").append(attribute.value);
					}
					// Выполняем добавление конца метки узла
					result.append(">");
				} break;
				// Если получен конец узла разметки
				case static_cast <uint8_t> (xml::event_t::ELEMENT_CLOSE):
					result.append("</").append(reader.name().local).append(">");
				break;
				// Если получено текстовое содержимое
				case static_cast <uint8_t> (xml::event_t::TEXT): result.append(reader.text()); break;
				// Если получено пробельное содержимое
				case static_cast <uint8_t> (xml::event_t::SPACE): result.append("_"); break;
				// Если получен раздел дословного текста
				case static_cast <uint8_t> (xml::event_t::CDATA): result.append("[cd ").append(reader.text()).append("]"); break;
				// Если текст разобран до конца
				case static_cast <uint8_t> (xml::event_t::FINISH): result.append("[end]"); break;
			}
		}
		/**
		 * Если разбор прекращён либо текст разобран до конца
		 */
		if((reader.state() == xml::state_t::FAILED) || (reader.state() == xml::state_t::FINISHED))
			// Выходим из подачи текста
			break;
		/**
		 * Если текст подан целиком, но разбор ждёт продолжения
		 */
		if(end && (reader.state() == xml::state_t::HUNGRY))
			// Выходим из подачи текста
			break;
	}
	// Запоминаем код ошибки разбора
	error = reader.error();
	// Выводим собранную запись событий разбора
	return result;
}
/**
 * @brief Метод разбора текста разметки целиком и по одному байту
 *
 * @details Разбиение исходного текста на куски при включённой склейке содержимого
 *          обязано оставаться незаметным: итог обоих разборов совпадает знак в знак
 *
 * @param text  разбираемый текст разметки
 * @param error код ошибки разбора
 * @return      запись полученных событий разбора
 *
 */
static string both(const string & text, xml::error_t & error) noexcept {
	// Выполняем разбор текста целиком
	const string whole = ::run(text, text.size() + 1, error);
	// Настройки разбора со склейкой содержимого
	xml::reader_t::settings_t settings;
	// Выполняем активацию склейки содержимого
	settings.mergeText = true;
	// Код ошибки разбора по одному байту
	xml::error_t second = xml::error_t::NONE;
	// Выполняем разбор текста по одному байту
	const string parts = ::run(text, 1, second, settings);
	/**
	 * Если итоги разборов не совпадают
	 */
	if(whole.compare(parts) != 0)
		// Выводим запись обоих итогов разбора
		return string("целиком: ").append(whole).append(" | по байту: ").append(parts);
	// Выводим запись полученных событий разбора
	return whole;
}

/**
 * @brief Проверка разбора простейшего текста разметки
 *
 */
TEST(CodecXmlReader, Simple) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	// Выполняем проверку разбора простого узла
	ASSERT_EQ(::both("<a>текст</a>", error), "<a>текст</a>[end]");
	// Выполняем проверку разбора самозакрывающейся метки
	ASSERT_EQ(::both("<a/>", error), "<a></a>[end]");
	// Выполняем проверку разбора вложенности и атрибутов
	ASSERT_EQ(::both("<a x='1' y=\"2\"><b/><c>з</c></a>", error), "<a x=1 y=2><b></b><c>з</c></a>[end]");
}
/**
 * @brief Проверка разбора пролога текста разметки
 *
 */
TEST(CodecXmlReader, Prolog) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	// Выполняем проверку разбора объявления разметки, примечания и указания обработчику
	ASSERT_EQ(::both("<?xml version=\"1.0\"?><!--п--><?f d?><a/>", error), "[decl 1.0][!п][pi f d]<a></a>[end]");
	/**
	 * Выполняем проверку принятия объявленного издания разметки вида «1.x»
	 *
	 * @note Пятое издание договора 1.0 велит обрабатывать текст со всяким изданием
	 *       вида «1.x» по правилам издания 1.0 и выдавать ошибку лишь начиная с «2.x»:
	 *       порядковый номер после точки правил построения текста не меняет
	 */
	ASSERT_EQ(::both("<?xml version=\"1.7\"?><a/>", error), "[decl 1.7]<a></a>[end]");
	// Выполняем проверку принятия объявленного издания разметки вида «1.x»
	ASSERT_EQ(::both("<?xml version=\"1.1\"?><a/>", error), "[decl 1.1]<a></a>[end]");
}
/**
 * @brief Проверка подстановки ссылок на сущности
 *
 */
TEST(CodecXmlReader, References) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	// Выполняем проверку подстановки предопределённых и числовых ссылок
	ASSERT_EQ(::both("<a>&lt;&amp;&#65;&#x42;</a>", error), "<a><&AB</a>[end]");
	// Выполняем проверку подстановки объявленной сущности
	ASSERT_EQ(::both("<!DOCTYPE a [<!ENTITY e \"знач\">]><a>&e;</a>", error), "[doctype a]<a>знач</a>[end]");
	// Выполняем проверку подстановки вложенной сущности
	ASSERT_EQ(::both("<!DOCTYPE a [<!ENTITY x \"1\"><!ENTITY y \"&x;2\">]><a>&y;</a>", error), "[doctype a]<a>12</a>[end]");
}
/**
 * @brief Проверка разбора раздела дословного текста и приведения конца строки
 *
 */
TEST(CodecXmlReader, Content) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	// Выполняем проверку разбора раздела дословного текста
	ASSERT_EQ(::both("<a><![CDATA[<b>&x]]></a>", error), "<a>[cd <b>&x]</a>[end]");
	// Выполняем проверку приведения всех видов конца строки
	ASSERT_EQ(::both("<a>1\r\n2\r3</a>", error), "<a>1\n2\n3</a>[end]");
	/**
	 * Выполняем проверку приведения пробельных знаков в значении атрибута
	 *
	 * @note Приведение ведётся в два приёма, и порядок их договором задан: сперва конец
	 *       строки обращается в перевод строки, отчего пара из возврата каретки с
	 *       переводом строки становится одним знаком, - и лишь затем пробельные знаки
	 *       обращаются в пробел. Замена каждого знака пары порознь дала бы два пробела
	 *       там, где договор велит один
	 */
	ASSERT_EQ(::both("<a x=\"1\r\n2\r3\n4\t5\"/>", error), "<a x=1 2 3 4 5></a>[end]");
	/**
	 * Выполняем проверку того, что записанное ссылкой приведению не подлежит
	 *
	 * @note Договор относит приведение к самому тексту, а не к тому, что ссылкой в него
	 *       подставлено: возврат каретки, записанный ссылкой, обязан дойти как есть
	 */
	ASSERT_EQ(::both("<a x=\"1&#13;&#10;2\"/>", error), "<a x=1\r\n2></a>[end]");
}
/**
 * @brief Проверка разрешения пространств имён
 *
 */
TEST(CodecXmlReader, Namespaces) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	// Выполняем проверку разрешения объявленных пространств имён
	ASSERT_EQ(::both("<s:a xmlns:s='urn:s' xmlns='urn:d'><b s:x='1'/></s:a>", error), "<a{urn:s}><b{urn:d} x{urn:s}=1></b></a>[end]");
	// Выполняем проверку отведённого договором префикса, объявления не требующего
	ASSERT_EQ(::both("<a xml:space='preserve'>т</a>", error), "<a space{http://www.w3.org/XML/1998/namespace}=preserve>т</a>[end]");
}
/**
 * @brief Проверка подстановки значений атрибутов по умолчанию
 *
 */
TEST(CodecXmlReader, Defaults) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	// Выполняем проверку подстановки объявленных по умолчанию значений
	ASSERT_EQ(::both("<!DOCTYPE a [<!ATTLIST a x CDATA \"по-умолчанию\" y CDATA #IMPLIED z NMTOKEN #FIXED \"1\">]><a/>", error),
		"[doctype a]<a x=по-умолчанию z=1></a>[end]");
	// Выполняем проверку старшинства значения, записанного в тексте
	ASSERT_EQ(::both("<!DOCTYPE a [<!ATTLIST a x CDATA \"d\">]><a x='t'/>", error), "[doctype a]<a x=t></a>[end]");
	// Выполняем проверку разбора перечня допустимых значений атрибута
	ASSERT_EQ(::both("<!DOCTYPE a [<!ATTLIST a x (да|нет) \"да\">]><a/>", error), "[doctype a]<a x=да></a>[end]");
	// Настройки разбора без подстановки значений по умолчанию
	xml::reader_t::settings_t settings;
	// Выполняем отключение подстановки значений по умолчанию
	settings.defaults = false;
	// Выполняем проверку отключения подстановки значений по умолчанию
	ASSERT_EQ(::run("<!DOCTYPE a [<!ATTLIST a x CDATA \"d\">]><a/>", 4096, error, settings), "[doctype a]<a></a>[end]");
}
/**
 * @brief Проверка подстановки сущности, содержащей разметку
 *
 */
TEST(CodecXmlReader, EntityMarkup) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	// Выполняем проверку разбора разметки, полученной подстановкой сущности
	ASSERT_EQ(::both("<!DOCTYPE a [<!ENTITY e \"<b x='1'>т</b>\">]><a>&e;</a>", error), "[doctype a]<a><b x=1>т</b></a>[end]");
	// Выполняем проверку разбора разметки сущности вперемешку с содержимым
	ASSERT_EQ(::both("<!DOCTYPE a [<!ENTITY e \"<b/>\">]><a>до&e;после</a>", error), "[doctype a]<a>до<b></b>после</a>[end]");
	// Выполняем проверку разбора вложенных сущностей с разметкой
	ASSERT_EQ(::both("<!DOCTYPE a [<!ENTITY x \"<c/>\"><!ENTITY y \"<b>&x;</b>\">]><a>&y;</a>", error), "[doctype a]<a><b><c></c></b></a>[end]");
}
/**
 * @brief Проверка отделения пробельного содержимого
 *
 */
TEST(CodecXmlReader, Spaces) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	// Настройки разбора с отделением пробельного содержимого
	xml::reader_t::settings_t settings;
	// Выполняем активацию отделения пробельного содержимого
	settings.separateSpaces = true;
	// Выполняем проверку отделения пробельного содержимого
	ASSERT_EQ(::run("<a>  <b/>  </a>", 4096, error, settings), "<a>_<b></b>_</a>[end]");
}
/**
 * @brief Проверка совпадения разбора большого текста при подаче кусками
 *
 */
TEST(CodecXmlReader, Chunks) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	// Собираемый разбираемый текст разметки
	string text = "<r>";
	/**
	 * Выполняем сборку текста разметки
	 */
	for(uint32_t i = 0; i < 2000; i++)
		// Выполняем добавление очередного узла разметки
		text.append("<i n='").append(to_string(i)).append("'>значение</i>");
	// Выполняем завершение текста разметки
	text.append("</r>");
	// Выполняем разбор текста целиком
	const string whole = ::run(text, text.size() + 1, error);
	// Выполняем разбор текста кусками по семь байтов
	const string parts = ::run(text, 7, error);
	// Выполняем проверку совпадения итогов разбора
	ASSERT_EQ(whole, parts);
	// Выполняем проверку полноты разбора
	ASSERT_GT(whole.size(), static_cast <size_t> (10000));
}
/**
 * @brief Проверка отклонения неправильно построенного текста разметки
 *
 */
TEST(CodecXmlReader, Malformed) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	/**
	 * @brief Разбираемый текст разметки и ожидаемый код ошибки
	 *
	 */
	const struct {
		// Разбираемый текст разметки
		const char * text;
		// Ожидаемый код ошибки разбора
		xml::error_t error;
	} items[] = {
		{"<a></b>",                                              xml::error_t::MISMATCHED_TAG},
		{"<a><b></b>",                                           xml::error_t::UNCLOSED_TAG},
		{"<a/></a>",                                             xml::error_t::UNEXPECTED_CLOSE_TAG},
		{"<a/><b/>",                                             xml::error_t::MULTIPLE_ROOTS},
		{"<!--п-->",                                             xml::error_t::MISSING_ROOT},
		{"т<a/>",                                                xml::error_t::CONTENT_OUTSIDE_ROOT},
		{"<a x='1' x='2'/>",                                     xml::error_t::DUPLICATE_ATTRIBUTE},
		{"<s:a/>",                                               xml::error_t::UNBOUND_PREFIX},
		{"<a>&nope;</a>",                                        xml::error_t::UNKNOWN_ENTITY},
		{"<a x=1/>",                                             xml::error_t::UNQUOTED_ATTRIBUTE},
		{"<a><!-- -- --></a>",                                   xml::error_t::INVALID_COMMENT},
		{"<a>&#0;</a>",                                          xml::error_t::INVALID_CHAR_REFERENCE},
		{"<?xml version='2.0'?><a/>",                            xml::error_t::UNSUPPORTED_VERSION},
		{"<?xml version='1'?><a/>",                              xml::error_t::UNSUPPORTED_VERSION},
		{"<?xml version='1.0.1'?><a/>",                          xml::error_t::UNSUPPORTED_VERSION},
		{"<a xmlns:xml='urn:x'/>",                               xml::error_t::RESERVED_PREFIX},
		{"<a>]]></a>",                                           xml::error_t::INVALID_CHARACTER},
		{"<!DOCTYPE a [<!ENTITY e \"<b>\">]><a>&e;</b></a>",     xml::error_t::ENTITY_BOUNDARY},
		{"<!DOCTYPE a [<!ENTITY e \"<b/>\">]><a x='&e;'/>",      xml::error_t::INVALID_ATTRIBUTE},
		{"<a>&#X41;</a>",                                        xml::error_t::INVALID_CHAR_REFERENCE},
		{"<a><!-- п ---></a>",                                   xml::error_t::INVALID_COMMENT},
		/**
		 * Построение объявления разметки
		 *
		 * @note Объявление разбирается по заданному договором строению, а не поиском
		 *       полей по названию: порядок полей задан, разделители обязательны
		 */
		{"<?xml encoding='UTF-8' version='1.0'?><a/>",           xml::error_t::INVALID_DECLARATION},
		{"<?xml version='1.0'encoding='UTF-8'?><a/>",            xml::error_t::INVALID_DECLARATION},
		{"<?xml version=='1.0'?><a/>",                           xml::error_t::INVALID_DECLARATION},
		{"<?xml version='1.0' Standalone='yes'?><a/>",           xml::error_t::INVALID_DECLARATION},
		{"<?xml version='1.0' valid='no'?><a/>",                 xml::error_t::INVALID_DECLARATION},
		{"<?xml version='1.0' version='1.0'?><a/>",              xml::error_t::INVALID_DECLARATION},
		/**
		 * Построение заголовка описания типа документа
		 *
		 */
		{"<!DOCTYPE SYSTEM 'a.dtd'><a/>",                        xml::error_t::INVALID_DOCTYPE},
		{"<!DOCTYPE a public '-//X//EN' 'a.dtd'><a/>",           xml::error_t::INVALID_DOCTYPE},
		{"<!DOCTYPE a PUBLIC '-//X//EN'><a/>",                   xml::error_t::INVALID_DOCTYPE},
		{"<!DOCTYPE a PUBLIC '-//X//EN''a.dtd'><a/>",            xml::error_t::INVALID_DOCTYPE},
		{"<!DOCTYPE a -- п -- []><a/>",                          xml::error_t::INVALID_DOCTYPE},
		{"<!DOCTYPE a PUBLIC '[' 'a.dtd'><a/>",                  xml::error_t::INVALID_DOCTYPE},
		/**
		 * Построение объявлений внутреннего подмножества
		 *
		 * @note Построение объявлений договор относит к правилам построения текста, а
		 *       не к правилам его действительности: проверяется именно построение
		 */
		{"<!DOCTYPE a [<!ELEMENT a CDATA>]><a/>",                xml::error_t::INVALID_DOCTYPE},
		{"<!DOCTYPE a [<!ELEMENT a (b|c,d)>]><a/>",              xml::error_t::INVALID_DOCTYPE},
		{"<!DOCTYPE a [<!ELEMENT a (#PCDATA|b)>]><a/>",          xml::error_t::INVALID_DOCTYPE},
		{"<!DOCTYPE a [<!ENTITY e PUBLIC 'x'>]><a/>",            xml::error_t::INVALID_DOCTYPE},
		{"<!DOCTYPE a [<!ENTITY % e '&'>]><a/>",                 xml::error_t::INVALID_DOCTYPE},
		{"<!DOCTYPE a [<!ENTITY % e ''><!ENTITY f '%e;'>]><a/>", xml::error_t::INVALID_DOCTYPE},
		{"<!DOCTYPE a [<!ATTLIST a x CDATA>]><a/>",              xml::error_t::INVALID_DOCTYPE},
		{"<!DOCTYPE a [<!ATTLIST a x WRONG #IMPLIED>]><a/>",     xml::error_t::INVALID_DOCTYPE},
		{"<!DOCTYPE a [<!NOTATION n>]><a/>",                     xml::error_t::INVALID_DOCTYPE},
		{"<!DOCTYPE a [<!ENTITY x:y 'v'>]><a/>",                 xml::error_t::INVALID_DOCTYPE},
		{"<!DOCTYPE a [<?xml version='1.0'?>]><a/>",             xml::error_t::RESERVED_PROCESSING}
	};
	/**
	 * Выполняем перебор всех разбираемых текстов разметки
	 */
	for(const auto & item : items){
		// Выполняем разбор очередного текста разметки
		::run(item.text, 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, item.error) << item.text << " -> " << xml::message(error);
	}
}
/**
 * @brief Проверка разбора правильно построенного описания типа документа
 *
 * @details Построение объявлений внутреннего подмножества проверяется разбором, и
 *          проверка эта обязана принимать всё, что договор допускает: объявления,
 *          записанные несколькими строками, перечни на выбор и заданные подряд,
 *          объявления обозначений и внешних сущностей
 *
 */
TEST(CodecXmlReader, Doctype) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	/**
	 * @brief Разбираемые правильно построенные описания типа документа
	 *
	 */
	const char * items[] = {
		"<!DOCTYPE a><a/>",
		"<!DOCTYPE a SYSTEM 'a.dtd'><a/>",
		"<!DOCTYPE a PUBLIC '-//X//EN' 'a.dtd'><a/>",
		"<!DOCTYPE a SYSTEM 'a.dtd' [<!ELEMENT a EMPTY>]><a/>",
		"<!DOCTYPE a [<!ELEMENT a ANY><!ELEMENT b (a?)><!ELEMENT c (a|b)>]><a/>",
		"<!DOCTYPE a [<!ELEMENT c (\n a\n |\n b\n |\n c?\n )>]><a/>",
		"<!DOCTYPE a [<!ELEMENT c (\n a\n ,\n b\n ,\n c?\n )>]><a/>",
		"<!DOCTYPE a [<!ELEMENT a (#PCDATA)><!ELEMENT b (#PCDATA|a)*>]><a/>",
		"<!DOCTYPE a [<!ELEMENT a ((b|c)+,d)*>]><a/>",
		"<!DOCTYPE a [<!ATTLIST a x CDATA #IMPLIED y NMTOKENS #REQUIRED>]><a/>",
		"<!DOCTYPE a [<!ATTLIST a x (1|2|3) '1'>]><a/>",
		"<!DOCTYPE a [<!NOTATION n SYSTEM 'n'><!ATTLIST a x NOTATION (n) #IMPLIED>]><a/>",
		"<!DOCTYPE a [<!NOTATION n PUBLIC '-//X//EN'>]><a/>",
		"<!DOCTYPE a [<!ENTITY e SYSTEM 'e.ent'><!ENTITY g SYSTEM 'g' NDATA n>]><a/>",
		"<!DOCTYPE a [<!ENTITY % e ''><!--п--><?f d?>%e;]><a/>",
		"<!DOCTYPE a [<!ENTITY e 'v'><!ATTLIST a x CDATA '&#65;&e;'>]><a/>"
	};
	/**
	 * Выполняем перебор всех разбираемых описаний типа документа
	 */
	for(const char * item : items){
		// Выполняем разбор очередного текста разметки
		::run(item, 4096, error);
		// Выполняем проверку отсутствия ошибки разбора
		ASSERT_EQ(error, xml::error_t::NONE) << item << " -> " << xml::message(error);
	}
}
/**
 * @brief Проверка защиты от многократного разрастания подстановки сущностей
 *
 */
TEST(CodecXmlReader, EntityExpansion) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	// Собираемый текст разметки с вложенными друг в друга сущностями
	string text = "<!DOCTYPE a [<!ENTITY a0 \"AAAAAAAAAA\">";
	/**
	 * Выполняем сборку вложенных друг в друга сущностей
	 */
	for(uint32_t i = 1; i < 10; i++){
		// Выполняем добавление начала объявления сущности
		text.append("<!ENTITY a").append(to_string(i)).append(" \"");
		/**
		 * Выполняем добавление ссылок на предыдущую сущность
		 */
		for(uint32_t j = 0; j < 10; j++)
			// Выполняем добавление ссылки на предыдущую сущность
			text.append("&a").append(to_string(i - 1)).append(";");
		// Выполняем завершение объявления сущности
		text.append("\">");
	}
	// Выполняем завершение текста разметки
	text.append("]><a>&a9;</a>");
	// Выполняем разбор текста разметки
	::run(text, 4096, error);
	// Выполняем проверку прекращения разбора превышением предела подстановки
	ASSERT_EQ(error, xml::error_t::ENTITY_LIMIT_EXCEEDED);
}
/**
 * @brief Проверка отклонения самоссылающихся и внешних сущностей
 *
 */
TEST(CodecXmlReader, EntitySafety) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	// Выполняем разбор текста с самоссылающейся сущностью
	::run("<!DOCTYPE a [<!ENTITY e \"&e;\">]><a>&e;</a>", 4096, error);
	// Выполняем проверку прекращения разбора ошибкой
	ASSERT_TRUE((error == xml::error_t::RECURSIVE_ENTITY) || (error == xml::error_t::ENTITY_DEPTH_EXCEEDED)) << xml::message(error);
	// Выполняем разбор текста со ссылкой на внешнюю сущность
	::run("<!DOCTYPE a [<!ENTITY e SYSTEM \"/etc/passwd\">]><a>&e;</a>", 4096, error);
	// Выполняем проверку отклонения ссылки на внешнюю сущность
	ASSERT_EQ(error, xml::error_t::EXTERNAL_ENTITY);
}
/**
 * @brief Проверка действия заявленных настройками пределов
 *
 * @details Пределы объявлены настройками и обязаны действовать на каждом пути разбора:
 *          предел, который обходится подачей текста особым образом либо объявлением в
 *          описании типа документа, защитой не является
 *
 */
TEST(CodecXmlReader, Limits) {
	// Собираемый поток байтов, которым проверяется удержание разбора
	const string junk(65536, 'x');
	/**
	 * @brief Незавершённые построения, удерживаемые разбором
	 *
	 */
	const char * items[] = {"<a></", "<a><!--", "<a><?p", "<a x=\"", "<!DOCTYPE a ["};
	/**
	 * Выполняем перебор всех незавершённых построений
	 */
	for(const char * item : items){
		// Настройки разбора с заданным пределом объёма события
		xml::reader_t::settings_t settings;
		// Выполняем установку предела объёма одного события
		settings.maxEvent = 4096;
		// Объект потокового чтения текста разметки
		xml::reader_t reader(settings);
		// Выполняем передачу начала незавершённого построения
		ASSERT_TRUE(reader.feed(item, ::strlen(item), false));
		/**
		 * Выполняем перебор всех событий разбора
		 */
		while(reader.next());
		/**
		 * Выполняем подачу потока байтов, не завершающего построения
		 */
		for(uint32_t i = 0; (i < 8) && (reader.state() != xml::state_t::FAILED); i++){
			// Выполняем передачу очередного куска потока байтов
			if(!reader.feed(junk.data(), junk.size(), false))
				// Выходим из подачи потока байтов
				break;
			/**
			 * Выполняем перебор всех событий разбора
			 */
			while(reader.next());
		}
		// Выполняем проверку прекращения разбора превышением предела
		ASSERT_EQ(reader.error(), xml::error_t::OVERFLOW_LIMIT) << item;
	}
	/**
	 * Выполняем проверку предела объёма события при склеивании содержимого
	 */
	{
		// Настройки разбора с заданным пределом объёма события
		xml::reader_t::settings_t settings;
		// Выполняем установку предела объёма одного события
		settings.maxEvent = 4096;
		// Выполняем включение склеивания кусков содержимого
		settings.mergeText = true;
		// Объект потокового чтения текста разметки
		xml::reader_t reader(settings);
		// Выполняем передачу начала текста разметки
		ASSERT_TRUE(reader.feed("<a>", 3, false));
		/**
		 * Выполняем перебор всех событий разбора
		 */
		while(reader.next());
		/**
		 * Выполняем подачу потока байтов содержимого узла
		 */
		for(uint32_t i = 0; (i < 8) && (reader.state() != xml::state_t::FAILED); i++){
			// Выполняем передачу очередного куска потока байтов
			if(!reader.feed(junk.data(), junk.size(), false))
				// Выходим из подачи потока байтов
				break;
			/**
			 * Выполняем перебор всех событий разбора
			 */
			while(reader.next());
		}
		// Выполняем проверку прекращения разбора превышением предела
		ASSERT_EQ(reader.error(), xml::error_t::OVERFLOW_LIMIT);
	}
	/**
	 * Выполняем проверку предела количества атрибутов на объявленных значениях
	 */
	{
		// Собираемый текст разметки с объявленным перечнем атрибутов
		string text = "<!DOCTYPE r [<!ATTLIST e";
		/**
		 * Выполняем сборку объявленного перечня атрибутов
		 */
		for(uint32_t i = 0; i < 64; i++)
			// Выполняем добавление очередного объявленного атрибута
			text.append(" a").append(to_string(i)).append(" CDATA \"x\"");
		// Выполняем завершение текста разметки
		text.append(">]><r><e/></r>");
		// Настройки разбора с заданным пределом количества атрибутов
		xml::reader_t::settings_t settings;
		// Выполняем установку предела количества атрибутов узла
		settings.maxAttributes = 16;
		// Код ошибки разбора
		xml::error_t error = xml::error_t::NONE;
		// Выполняем разбор собранного текста разметки
		::run(text, 4096, error, settings);
		// Выполняем проверку прекращения разбора превышением предела
		ASSERT_EQ(error, xml::error_t::OVERFLOW_LIMIT);
	}
}
/**
 * @brief Проверка неизменности итога разбора при подаче текста кусками
 *
 * @details Разбиение исходного текста на куски обязано быть для разбора незаметным:
 *          отведённая договором последовательность, разорванная границей куска,
 *          обязана улавливаться наравне с записанной целиком
 *
 */
TEST(CodecXmlReader, Boundary) {
	/**
	 * @brief Тексты разметки, отведённые последовательности которых попадают на границу
	 *
	 */
	const char * items[] = {
		"<a>x]]>y</a>",
		"<a>x]]]>y</a>",
		"<a>текст &amp; ещё &#1071; конец</a>",
		"<a>строка\r\nвторая\rтретья</a>"
	};
	/**
	 * Выполняем перебор всех разбираемых текстов разметки
	 */
	for(const char * item : items){
		// Код ошибки разбора текста, поданного целиком
		xml::error_t whole = xml::error_t::NONE;
		// Код ошибки разбора текста, поданного по одному байту
		xml::error_t parts = xml::error_t::NONE;
		// Выполняем разбор текста разметки, поданного целиком
		const string first = ::run(item, 4096, whole);
		// Выполняем разбор текста разметки, поданного по одному байту
		const string second = ::run(item, 1, parts);
		// Выполняем проверку совпадения кода ошибки разбора
		ASSERT_EQ(whole, parts) << item;
		/**
		 * Если текст разметки разобран без ошибки
		 *
		 * @note Итоги сличаются лишь у разобранных без ошибки текстов: до отказа
		 *       разбор успевает выдать разное количество событий, и это его свойство,
		 *       а не расхождение - отказ наступает на том же самом месте
		 */
		if(whole == xml::error_t::NONE)
			// Выполняем проверку совпадения итога разбора
			ASSERT_EQ(first, second) << item;
	}
}
/**
 * @brief Проверка места атрибутов узла в исходном тексте
 *
 * @details Место атрибутов считается двумя способами: открывающая метка, состоящая из
 *          знаков ASCII и не содержащая переводов строки, разбирается быстрым путём, где
 *          положение атрибута получается сложением с началом метки, а метка с переводом
 *          строки или знаком вне ASCII досчитывается перебором. Оба пути обязаны давать
 *          одно и то же место, иначе выигрыш в скорости куплен сбитыми местами
 *
 * @note Проверка закрепляет намеренное решение: без неё быстрый путь остаётся непроверенным,
 *       так как место атрибута ни на что в разборе не влияет и расхождение молчаливо
 *
 */
TEST(CodecXmlReader, AttributeLocation) {
	/**
	 * @brief Ожидаемое место одного атрибута узла
	 *
	 */
	struct expected_t {
		// Местное имя атрибута
		const char * name;
		// Ожидаемый номер строки
		uint32_t line;
		// Ожидаемое положение в строке
		uint32_t column;
	};
	/**
	 * @brief Один проверяемый случай
	 *
	 */
	struct probe_t {
		// Разбираемый текст разметки
		const char * text;
		// Ожидаемые места атрибутов узла
		vector <expected_t> attributes;
	};
	// Перечень проверяемых случаев
	const vector <probe_t> probes = {
		// Метка из знаков ASCII в одну строку - быстрый путь
		{"<r a=\"1\" b=\"2\" c=\"3\"/>", {{"a", 1, 4}, {"b", 1, 10}, {"c", 1, 16}}},
		// Метка, разорванная переводом строки - запасной путь
		{"<r a=\"1\"\n   b=\"2\"/>", {{"a", 1, 4}, {"b", 2, 4}}},
		// Метка со знаком вне ASCII в значении - запасной путь
		{"<r a=\"дом\" b=\"2\"/>", {{"a", 1, 4}, {"b", 1, 12}}},
		// Метка со знаком вне ASCII в имени узла - запасной путь
		{"<дом a=\"1\" b=\"2\"/>", {{"a", 1, 6}, {"b", 1, 12}}},
		// Узел не в первой строке текста разметки
		{"<r>\n\t<n a=\"1\" b=\"2\"/>\n</r>", {{"a", 2, 5}, {"b", 2, 11}}}
	};
	/**
	 * Выполняем перебор всех проверяемых случаев
	 */
	for(const probe_t & probe : probes){
		// Объект потокового чтения текста разметки
		xml::reader_t reader;
		// Выполняем передачу текста разметки
		ASSERT_TRUE(reader.feed(probe.text)) << probe.text;
		// Собранные места атрибутов узлов
		vector <pair <uint32_t, uint32_t>> collected;
		/**
		 * Выполняем перебор всех событий разбора
		 */
		while(reader.next()){
			/**
			 * Если получено начало узла разметки
			 */
			if(reader.event() == xml::event_t::ELEMENT_OPEN){
				/**
				 * Выполняем перебор всех атрибутов узла
				 */
				for(const xml::attribute_t & attribute : reader.attributes())
					// Выполняем сбор места очередного атрибута узла
					collected.emplace_back(attribute.location.line, attribute.location.column);
			}
		}
		// Выполняем проверку разбора текста разметки до конца
		ASSERT_EQ(reader.state(), xml::state_t::FINISHED) << probe.text;
		// Выполняем проверку количества собранных атрибутов
		ASSERT_EQ(collected.size(), probe.attributes.size()) << probe.text;
		/**
		 * Выполняем перебор всех собранных атрибутов
		 */
		for(size_t i = 0; i < collected.size(); i++){
			// Выполняем проверку номера строки атрибута
			ASSERT_EQ(collected[i].first, probe.attributes[i].line) << probe.text << " [" << probe.attributes[i].name << "]";
			// Выполняем проверку положения атрибута в строке
			ASSERT_EQ(collected[i].second, probe.attributes[i].column) << probe.text << " [" << probe.attributes[i].name << "]";
		}
	}
}
/**
 * @brief Проверка места разбора в исходном тексте после подстановки сущности
 *
 * @details Подстановка замещает ссылку значением прямо в разбираемом тексте, и место
 *          разбора обязано восстанавливаться по закрытии области подстановки: иначе
 *          сбитое место осталось бы до конца разбора и накапливалось бы с каждой ссылкой
 *
 */
TEST(CodecXmlReader, Location) {
	// Разбираемый текст разметки с подставляемой сущностью
	const string text = "<!DOCTYPE r [<!ENTITY e '<b>\n\n</b>'>]>\n<r>&e;&e;</r>\n<!--конец-->";
	// Настройки разбора с выдачей примечаний отдельным событием
	xml::reader_t::settings_t settings;
	// Выполняем включение выдачи примечаний отдельным событием
	settings.comments = true;
	// Объект потокового чтения текста разметки
	xml::reader_t reader(settings);
	// Выполняем передачу текста разметки
	ASSERT_TRUE(reader.feed(text));
	// Место примечания в исходном тексте
	xml::location_t place;
	/**
	 * Выполняем перебор всех событий разбора
	 */
	while(reader.next()){
		/**
		 * Если получено примечание
		 */
		if(reader.event() == xml::event_t::COMMENT)
			// Запоминаем место примечания в исходном тексте
			place = reader.location();
	}
	// Выполняем проверку разбора текста разметки до конца
	ASSERT_EQ(reader.state(), xml::state_t::FINISHED) << xml::message(reader.error());
	// Выполняем проверку смещения примечания в исходном тексте
	ASSERT_EQ(place.offset, static_cast <uint64_t> (text.find("<!--конец-->")));
	// Выполняем проверку номера строки примечания в исходном тексте
	ASSERT_EQ(place.line, static_cast <uint32_t> (5));
}
/**
 * @brief Проверка получения содержимого и атрибутов числом при потоковом чтении
 *
 * @details Числовой разбор доступен и потоковому чтению, а не только дереву: разбирать
 *          ответ службы числами нередко требуется по мере его поступления, дерева не
 *          собирая. Содержимое при этом обязано быть собрано целиком
 *
 */
TEST(CodecXmlReader, Numeric) {
	// Настройки разбора текста разметки
	xml::reader_t::settings_t settings;
	/**
	 * Выполняем склеивание подряд идущих кусков содержимого в одно событие
	 *
	 * @note Без склеивания содержимое, разорванное границей куска, выдаётся
	 *       несколькими событиями, и разбирать его числом нельзя
	 */
	settings.mergeText = true;
	// Объект потокового чтения текста разметки
	xml::reader_t reader(settings);
	// Разбираемый текст разметки
	const string text = "<r><port id=\"255\">52</port><rate>54.33</rate></r>";
	// Значение разбираемого целого числа
	uint16_t port = 0;
	// Значение разбираемого целого числа из атрибута
	uint16_t id = 0;
	// Значение разбираемого числа с плавающей точкой
	double rate = 0.;
	// Имя последнего открытого узла разметки
	string name;
	/**
	 * Выполняем подачу текста разметки кусками по одному октету
	 */
	for(size_t offset = 0; offset < text.size(); offset++){
		// Выполняем передачу очередного куска текста разметки
		ASSERT_TRUE(reader.feed(text.data() + offset, 1, ((offset + 1) >= text.size())));
		/**
		 * Выполняем перебор всех событий, полученных из очередного куска
		 */
		while(reader.next()){
			/**
			 * Если получено начало узла разметки
			 */
			if(reader.event() == xml::event_t::ELEMENT_OPEN){
				// Запоминаем имя открытого узла разметки
				name = reader.name().local;
				/**
				 * Если открыт узел с числовым атрибутом
				 */
				if(name.compare("port") == 0)
					// Выполняем разбор значения атрибута узла числом
					ASSERT_TRUE(reader.value(id, "id"));
			/**
			 * Если получено текстовое содержимое узла
			 */
			} else if(reader.event() == xml::event_t::TEXT) {
				/**
				 * Если содержимое принадлежит узлу целого числа
				 */
				if(name.compare("port") == 0)
					// Выполняем разбор содержимого узла целым числом
					ASSERT_TRUE(reader.value(port));
				/**
				 * Если содержимое принадлежит узлу числа с плавающей точкой
				 */
				else if(name.compare("rate") == 0)
					// Выполняем разбор содержимого узла числом с плавающей точкой
					ASSERT_TRUE(reader.value(rate));
			}
		}
	}
	// Выполняем проверку состояния разбора
	ASSERT_EQ(reader.state(), xml::state_t::FINISHED) << xml::message(reader.error());
	// Выполняем проверку разобранного значения атрибута
	ASSERT_EQ(id, 255);
	// Выполняем проверку разобранного целого числа
	ASSERT_EQ(port, 52);
	// Выполняем проверку разобранного числа с плавающей точкой
	ASSERT_DOUBLE_EQ(rate, 54.33);
}
