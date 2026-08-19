/**
 * @file reader.cpp
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
 * @brief Автоматические тесты потокового чтения текста разметки — выдача событий разбора,
 *        разрешение пространств имён, подстановка сущностей, значения атрибутов по умолчанию
 *        и совпадение итога при подаче текста целиком и по одному байту
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <chrono>

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
	 * Если размер куска не задан, подаём текст целиком
	 *
	 * @note Размер нулевой вешал подачу навечно: кусок выходил пустым, положение подачи
	 *       не двигалось, а передача пустого куска отказа не даёт. Признака тому не было
	 *       никакого - проверка просто не возвращалась, и отличить это от долгого разбора
	 *       нельзя ничем
	 */
	const size_t length = ((step > 0) ? step : (text.size() + 1));
	/**
	 * Выполняем подачу текста разметки кусками
	 */
	for(;;){
		// Получаем размер очередного куска текста
		const size_t size = ((offset + length) > text.size() ? (text.size() - offset) : length);
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
	 * Выполняем проверку приведения конца строки в примечании и указании обработчику
	 *
	 * @note Приведение конца строки договор относит ко всему исходному тексту без
	 *       изъятий, а не к одному лишь содержимому узлов
	 */
	ASSERT_EQ(::both("<a><!--1\r2--><?pi 3\r\n4?></a>", error), "<a>[!1\n2][pi pi 3\n4]</a>[end]");
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
	/**
	 * Выполняем проверку приведения пробельных знаков подставленной сущности
	 *
	 * @note Договор предписывает приводить значение атрибута повторно и к тому, что
	 *       подставлено ссылкой на сущность: пробельные знаки её значения обращаются в
	 *       пробел наравне с записанными в самом значении атрибута
	 */
	ASSERT_EQ(::both("<!DOCTYPE a [<!ENTITY e \"x&#10;y\">]><a k=\"&e;\"/>", error), "[doctype a]<a k=x y></a>[end]");
	/**
	 * Выполняем проверку того, что конец строки в значении сущности одним знаком не считается
	 *
	 * @note Приведение конца строки договор относит к исходному тексту, а значение
	 *       сущности исходным текстом уже не является: пара из возврата каретки с
	 *       переводом строки даёт здесь два пробела, а не один
	 */
	ASSERT_EQ(::both("<!DOCTYPE a [<!ENTITY e \"x&#13;&#10;y\">]><a k=\"&e;\"/>", error), "[doctype a]<a k=x  y></a>[end]");
	/**
	 * Выполняем проверку того, что содержимое узла приведению не подлежит
	 *
	 * @note Приведение пробельных знаков договор относит к значению атрибута, а не к
	 *       содержимому узла: там подставленное значение доходит как есть
	 */
	ASSERT_EQ(::both("<!DOCTYPE a [<!ENTITY e \"x&#10;y\">]><a>&e;</a>", error), "[doctype a]<a>x\ny</a>[end]");
	/**
	 * Выполняем проверку приведения пары в объявленном по умолчанию значении
	 *
	 * @note Объявление входит в исходный текст, и приведение конца строки к нему
	 *       прилагается: пара даёт один пробел, как и в значении, записанном у узла
	 */
	ASSERT_EQ(::both("<!DOCTYPE a [<!ATTLIST a k CDATA \"p\r\nq\">]><a/>", error), "[doctype a]<a k=p q></a>[end]");
	/**
	 * Выполняем проверку приведения при подстановке сущности в объявленное значение
	 *
	 * @note Приведение при объявлении такую подстановку не покрывает: ссылка на сущность
	 *       подставляется не при объявлении, а при применении значения к узлу, - и
	 *       пробельные знаки её значения приводятся уже там
	 */
	ASSERT_EQ(::both("<!DOCTYPE a [<!ENTITY e \"x&#10;y\"><!ATTLIST a k CDATA \"&e;\">]><a/>", error), "[doctype a]<a k=x y></a>[end]");
	// Выполняем проверку приведения возврата каретки и табуляции из значения сущности
	ASSERT_EQ(::both("<!DOCTYPE a [<!ENTITY e \"x&#13;y&#9;z\"><!ATTLIST a k CDATA \"&e;\">]><a/>", error), "[doctype a]<a k=x y z></a>[end]");
	/**
	 * Выполняем проверку двойной записи знака начала разметки
	 *
	 * @note Значением `&#38;#60;` выходит `&#60;`, и разбирается оно повторно уже при
	 *       подстановке: знак начала разметки выходит данными, а не разметкой. Ради этого
	 *       договор двойную запись предопределённым сущностям и предписывает
	 */
	ASSERT_EQ(::both("<!DOCTYPE a [<!ENTITY e \"&#38;#60;b/&#38;#62;\">]><a>&e;</a>", error), "[doctype a]<a><b/></a>[end]");
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
	/**
	 * Выполняем проверку правильности построения имени с префиксом
	 *
	 * @details Договор о пространствах имён строит имя с префиксом из двух имён без
	 *          двоеточия, и каждое из них подчиняется тем же правилам, что и имя без
	 *          префикса: начинаться цифрой, дефисом либо точкой ему не дозволено.
	 *          Проверка первого знака всего имени этого не покрывает - она приходится
	 *          на первый знак префикса, а местное имя стоит за разделителем
	 */
	{
		// Перечень проверяемых имён, договором не допускаемых
		const vector <const char *> probes = {
			// Местное имя узла начинается цифрой
			"<a:1 xmlns:a='urn:x'/>",
			// Местное имя узла начинается дефисом
			"<a:-x xmlns:a='urn:x'/>",
			// Местное имя узла начинается точкой
			"<a:.x xmlns:a='urn:x'/>",
			// Префикс объявления пространства имён начинается цифрой
			"<a xmlns:1='urn:y'/>",
			// Местное имя атрибута начинается цифрой
			"<a b:1='x' xmlns:b='urn:x'/>"
		};
		/**
		 * Выполняем перебор всех проверяемых имён
		 */
		for(const char * probe : probes){
			// Выполняем разбор проверяемого имени
			::both(probe, error);
			// Выполняем проверку того, что имя отвергнуто
			ASSERT_EQ(error, xml::error_t::INVALID_NAME) << probe;
		}
	}
	// Выполняем проверку того, что имя с префиксом вне US-ASCII принимается
	ASSERT_EQ(::both("<a:дом xmlns:a='urn:x'/>", error), "<дом{urn:x}></дом>[end]");
	// Выполняем проверку того, что точка внутри местного имени допускается
	ASSERT_EQ(::both("<a:x.y xmlns:a='urn:x'/>", error), "<x.y{urn:x}></x.y>[end]");
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
	/**
	 * Выполняем проверку разметки, записанной числовыми ссылками
	 *
	 * @note Числовые ссылки подставляются при объявлении, и знак начала разметки, ими
	 *       записанный, входит в значение сущности наравне с записанным прямо: договор
	 *       различает исходную запись и подставленное значение, а не то, каким способом
	 *       знак в это значение попал
	 */
	ASSERT_EQ(::both("<!DOCTYPE a [<!ENTITY e \"&#60;b/&#62;\">]><a>&e;</a>", error), "[doctype a]<a><b></b></a>[end]");
	/**
	 * Выполняем проверку переноса разметки по вложенной ссылке
	 *
	 * @note Ссылка на сущность подставляется не при объявлении, а при обращении, и
	 *       сущность, сама разметки не несущая, приносит разметку той, на которую
	 *       ссылается: без переноса признака подстановка пошла бы путём значения
	 *       атрибута и завершилась бы отказом посреди содержимого узла
	 */
	ASSERT_EQ(::both("<!DOCTYPE a [<!ENTITY inner \"<b/>\"><!ENTITY outer \"x&inner;y\">]><a>&outer;</a>", error), "[doctype a]<a>x<b></b>y</a>[end]");
	/**
	 * Выполняем проверку переноса разметки по ссылке на объявленное ниже
	 *
	 * @note Порядок объявлений договор не задаёт, и ссылаться дозволено на объявленное
	 *       следом: перенос ведётся до неподвижности, а не одним обходом
	 */
	ASSERT_EQ(::both("<!DOCTYPE a [<!ENTITY outer \"&inner;\"><!ENTITY inner \"<b/>\">]><a>&outer;</a>", error), "[doctype a]<a><b></b></a>[end]");
}
/**
 * @brief Проверка запрета знака начала разметки в значении атрибута
 *
 * @details Договор не допускает знака начала разметки в значении атрибута ни прямо, ни
 *          подстановкой ссылки на сущность: значение, его несущее, делает текст
 *          неправильно построенным
 *
 */
TEST(CodecXmlReader, AttributeMarkup) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	// Выполняем проверку отказа при разметке, записанной прямо
	::both("<!DOCTYPE a [<!ENTITY e \"<b/>\">]><a x=\"&e;\"/>", error);
	// Выполняем проверку кода причины отказа
	ASSERT_EQ(error, xml::error_t::INVALID_ATTRIBUTE);
	// Выполняем проверку отказа при разметке, записанной числовой ссылкой
	::both("<!DOCTYPE a [<!ENTITY e \"&#60;\">]><a x=\"&e;\"/>", error);
	// Выполняем проверку кода причины отказа
	ASSERT_EQ(error, xml::error_t::INVALID_ATTRIBUTE);
	/**
	 * Выполняем проверку того, что предопределённая сущность отказа не вызывает
	 *
	 * @note Значением её договор предписывает числовую ссылку, а не сам знак: `&lt;`
	 *       даёт знак данными, разметкой не становясь
	 */
	ASSERT_EQ(::both("<a x=\"&lt;\"/>", error), "<a x=<></a>[end]");
	// Выполняем проверку того, что отказа при предопределённой сущности не было
	ASSERT_EQ(error, xml::error_t::NONE);
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
		/**
		 * Отказы разбора ссылок, набором прежде не проверявшиеся
		 *
		 * @note Ветви эти показала нетронутыми карта покрытия. Всякая из них - свой
		 *       повод отказа: пустое имя, запись кодового значения, разобранная не
		 *       целиком, и кодовое значение за пределами Юникода
		 */
		{"<a>&;</a>",                                            xml::error_t::INVALID_REFERENCE},
		{"<a>&#1f;</a>",                                         xml::error_t::INVALID_CHAR_REFERENCE},
		{"<a>&#x110000;</a>",                                    xml::error_t::INVALID_CHAR_REFERENCE},
		{"<a>&#1114112;</a>",                                    xml::error_t::INVALID_CHAR_REFERENCE},
		/**
		 * Отказы разбора внутреннего подмножества и пространств имён
		 *
		 * @note Ветвей отказа во внутреннем подмножестве набор не звал почти вовсе:
		 *       карта покрытия показала нетронутыми четыре десятка их. Всякая запись
		 *       здесь ломает объявление своим порядком - видом объявления, именем,
		 *       отделителем, значением по умолчанию и закрывающей скобкой
		 */
		{"<!DOCTYPE a [<!ELEMENT>]><a/>",                        xml::error_t::INVALID_DOCTYPE},
		{"<!DOCTYPE a [<!ELEMENT a>]><a/>",                      xml::error_t::INVALID_DOCTYPE},
		{"<!DOCTYPE a [<!ELEMENT a ()>]><a/>",                   xml::error_t::INVALID_DOCTYPE},
		{"<!DOCTYPE a [<!ATTLIST>]><a/>",                        xml::error_t::INVALID_DOCTYPE},
		{"<!DOCTYPE a [<!ATTLIST a x>]><a/>",                    xml::error_t::INVALID_DOCTYPE},
		{"<!DOCTYPE a [<!ATTLIST a x CDATA>]><a/>",              xml::error_t::INVALID_DOCTYPE},
		{"<!DOCTYPE a [<!ATTLIST a x CDATA #WRONG>]><a/>",       xml::error_t::INVALID_DOCTYPE},
		{"<!DOCTYPE a [<!ENTITY>]><a/>",                         xml::error_t::INVALID_DOCTYPE},
		{"<!DOCTYPE a [<!ENTITY e>]><a/>",                       xml::error_t::INVALID_DOCTYPE},
		{"<!DOCTYPE a [<!NOTATION>]><a/>",                       xml::error_t::INVALID_DOCTYPE},
		{"<!DOCTYPE a [<!WRONG a>]><a/>",                        xml::error_t::INVALID_DOCTYPE},
		{"<!DOCTYPE a [<!ELEMENT a EMPTY>",                      xml::error_t::INVALID_DOCTYPE},
		/**
		 * Примечания и указания обработчику внутри внутреннего подмножества
		 *
		 * @note Разбираются они там своим кодом, а не тем, каким разбираются в самом
		 *       тексте: договор дозволяет им стоять между объявлениями. Карта покрытия
		 *       показала обе ветви отказа нетронутыми
		 *
		 * @note Обрыв записи концом подмножества выносится отказом описания типа
		 *       документа, а не примечания либо указания: отыскание закрывающей скобки
		 *       подмножества идёт раньше и обрыв улавливает само
		 */
		{"<!DOCTYPE a [<!-- незакрытое ]><a/>",                  xml::error_t::INVALID_DOCTYPE},
		{"<!DOCTYPE a [<!-- двойной -- перенос -->]><a/>",       xml::error_t::INVALID_COMMENT},
		{"<!DOCTYPE a [<?pi незакрытое ]><a/>",                  xml::error_t::INVALID_DOCTYPE},
		{"<!DOCTYPE a [<?pi\"тело\"?>]><a/>",                     xml::error_t::INVALID_PROCESSING},
		{"<!DOCTYPE a [<?xml тело?>]><a/>",                      xml::error_t::RESERVED_PROCESSING},
		{"<a xmlns:p=''><p:b/></a>",                             xml::error_t::INVALID_NAMESPACE},
		{"<a xmlns:xmlns='urn:x'/>",                             xml::error_t::RESERVED_PREFIX},
		/**
		 * Отказы объявления разметки, размещения описания типа и построения меток
		 *
		 * @note Ветви эти показала нетронутыми карта покрытия. Всякая запись здесь
		 *       ломает своё: признак самостоятельности, место описания типа документа,
		 *       отведённое договором обозначение обработчика, построение метки и её
		 *       имени, построение атрибута и ссылки в значении его
		 */
		{"<?xml version='1.0' standalone='maybe'?><a/>",         xml::error_t::INVALID_DECLARATION},
		{"<?xml version='1.0' encoding='UTF-8' bad='1'?><a/>",   xml::error_t::INVALID_DECLARATION},
		{"<a/><!DOCTYPE a>",                                     xml::error_t::DOCTYPE_MISPLACED},
		{"<a><?XmL тело?></a>",                                  xml::error_t::RESERVED_PROCESSING},
		{"<a><b>1</a>",                                          xml::error_t::MISMATCHED_TAG},
		{"<a><1b/></a>",                                         xml::error_t::INVALID_NAME},
		{"<a></ b>",                                             xml::error_t::INVALID_NAME},
		{"<a b'1'/>",                                            xml::error_t::INVALID_ATTRIBUTE},
		{"<a b=&x;/>",                                           xml::error_t::UNQUOTED_ATTRIBUTE},
		{"<a x='&nope;'/>",                                      xml::error_t::UNKNOWN_ENTITY},
		{"<a><![CDATA[з</a>",                                    xml::error_t::INVALID_CDATA},
		{"<a><!-- п </a>",                                       xml::error_t::INVALID_COMMENT},
		{"<a/>хвост",                                            xml::error_t::CONTENT_OUTSIDE_ROOT},
		{"<a xmlns:p='urn:x' xmlns:p='urn:y'/>",                 xml::error_t::DUPLICATE_ATTRIBUTE},
		{"<a xmlns:p='urn:x'><b p:x='1' p:x='2'/></a>",          xml::error_t::DUPLICATE_ATTRIBUTE},
		/**
		 * Последовательность конца дословного раздела в значении сущности
		 *
		 * @note Запрет отнесён договором к содержимому узла, а не к глубине вложения
		 *       ссылок: ссылка-посредник, значение которой запрета не нарушает, от
		 *       проверки не избавляет - проверяется всякая сущность, чьё значение
		 *       подставляется в содержимое
		 */
		{"<!DOCTYPE a [<!ENTITY e \"]]>\">]><a>&e;</a>",          xml::error_t::INVALID_CHARACTER},
		{"<!DOCTYPE a [<!ENTITY e \"]]>\"><!ENTITY f \"&e;\">]><a>&f;</a>", xml::error_t::INVALID_CHARACTER},
		{"<!DOCTYPE a [<!ENTITY e \"<b>\">]><a>&e;</b></a>",     xml::error_t::ENTITY_BOUNDARY},
		{"<!DOCTYPE a [<!ENTITY e \"<b/>\">]><a x='&e;'/>",      xml::error_t::INVALID_ATTRIBUTE},
		/**
		 * Значение по умолчанию, ссылающееся на сущность с разметкой
		 *
		 * @note Запрет отнесён к самому объявлению, а не к его применению: негодным
		 *       объявление остаётся и тогда, когда атрибут задан явно и значение по
		 *       умолчанию не применяется ни разу. Внешняя сущность рядом проверялась
		 *       на объявлении и прежде
		 */
		{"<!DOCTYPE a [<!ENTITY e \"<b/>\"><!ATTLIST a x CDATA \"&e;\">]><a x=\"ok\"/>", xml::error_t::INVALID_REFERENCE},
		{"<!DOCTYPE a [<!ENTITY e \"<b/>\"><!ATTLIST a x CDATA \"&e;\">]><a/>", xml::error_t::INVALID_REFERENCE},
		/**
		 * Разметка, принесённая цепочкой ссылок
		 *
		 * @note Порядок объявлений значения не имеет: сущность вправе ссылаться на
		 *       объявленную ниже, и признак разметки переносится по цепочке до
		 *       неподвижности
		 */
		{"<!DOCTYPE a [<!ENTITY inner \"<b/>\"><!ENTITY outer \"&inner;\"><!ATTLIST a x CDATA \"&outer;\">]><a x=\"ok\"/>", xml::error_t::INVALID_REFERENCE},
		{"<!DOCTYPE a [<!ENTITY outer \"&inner;\"><!ENTITY inner \"<b/>\"><!ATTLIST a x CDATA \"&outer;\">]><a x=\"ok\"/>", xml::error_t::INVALID_REFERENCE},
		{"<!DOCTYPE a [<!ENTITY inner \"<b/>\"><!ENTITY outer \"&inner;\"><!ATTLIST a x CDATA \"&outer;\">]><a/>", xml::error_t::INVALID_REFERENCE},
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
	/**
	 * @brief Метод разбора текста разметки целиком
	 *
	 * @param text     разбираемый текст разметки
	 * @param settings настройки разбора текста разметки
	 * @param chunk    размер куска подачи, ноль подаёт текст целиком
	 * @param error    ссылка на код отказа разбора
	 * @return         признак того, что текст разобран до конца
	 *
	 */
	const auto parse = [](const string & text, const xml::reader_t::settings_t & settings, const size_t chunk, xml::error_t & error) noexcept -> bool {
		// Объект потокового чтения текста разметки
		xml::reader_t reader(settings);
		/**
		 * Если текст подаётся целиком
		 */
		if(chunk == 0){
			// Выполняем передачу исходного текста разметки
			reader.feed(text.data(), text.size(), true);
			// Выполняем перебор всех событий разбора
			while(reader.next());
		/**
		 * Если текст подаётся кусками
		 */
		} else {
			// Положение подачи в исходном тексте разметки
			size_t offset = 0;
			/**
			 * Выполняем подачу исходного текста кусками
			 */
			for(;;){
				// Выполняем перебор всех событий разбора
				while(reader.next());
				// Если исходный текст подан целиком, выходим из подачи
				if(offset >= text.size()) break;
				// Получаем размер очередного подаваемого куска
				const size_t size = (((offset + chunk) > text.size()) ? (text.size() - offset) : chunk);
				// Выполняем передачу очередного куска исходного текста
				reader.feed(text.data() + offset, size, (offset + size) >= text.size());
				// Выполняем переход к следующему куску исходного текста
				offset += size;
			}
		}
		// Запоминаем код отказа разбора
		error = reader.error();
		// Выводим признак того, что текст разобран до конца
		return (reader.state() == xml::state_t::FINISHED);
	};
	// Код отказа разбора текста разметки
	xml::error_t error = xml::error_t::NONE;
	/**
	 * Выполняем проверку предела длины имени
	 *
	 * @note Отказ по длине прежде выдавался кодом ошибочно построенного имени, а код
	 *       превышения длины не выдавался вовсе - ни из одного места разбора
	 */
	{
		// Настройки разбора с пределом длины имени
		xml::reader_t::settings_t settings;
		// Устанавливаем предел длины имени
		settings.maxName = 8;
		// Выполняем проверку того, что имя ровно по пределу принимается
		ASSERT_TRUE(parse("<" + string(8, 'a') + "/>", settings, 0, error));
		// Выполняем проверку того, что имя сверх предела отвергается
		ASSERT_FALSE(parse("<" + string(9, 'a') + "/>", settings, 0, error));
		// Выполняем проверку кода отказа по длине имени
		ASSERT_EQ(error, xml::error_t::NAME_TOO_LONG);
		// Выполняем проверку того, что ошибочно построенное имя выдаёт свой код отказа
		ASSERT_FALSE(parse("<-a/>", settings, 0, error));
		// Выполняем проверку кода отказа по построению имени
		ASSERT_EQ(error, xml::error_t::INVALID_NAME);
		/**
		 * Выполняем проверку того, что предел считается знаками, а не байтами
		 *
		 * @note Знак вне US-ASCII занимает несколько байтов, и счёт байтами отверг бы
		 *       имя, уместившееся в предел по знакам
		 */
		string wide("<");
		// Собираем имя из восьми знаков вне US-ASCII
		for(size_t i = 0; i < 8; i++) wide.append("\xD1\x8E");
		// Завершаем имя самозакрывающейся меткой
		wide.append("/>");
		// Выполняем проверку того, что имя из восьми таких знаков принимается
		ASSERT_TRUE(parse(wide, settings, 0, error));
	}
	/**
	 * Выполняем проверку предела объёма одного события
	 *
	 * @note Предел прежде держался лишь по ходу накопления кусков, и текст, пришедший
	 *       целиком, проходил мимо него: величина его зависела от устройства сети
	 */
	{
		// Настройки разбора с пределом объёма события
		xml::reader_t::settings_t settings;
		// Устанавливаем предел объёма одного события
		settings.maxEvent = 16;
		// Выполняем склеивание подряд идущих кусков содержимого
		settings.mergeText = true;
		// Выполняем проверку того, что событие ровно по пределу принимается
		ASSERT_TRUE(parse("<a>" + string(16, 'x') + "</a>", settings, 0, error));
		// Выполняем проверку того, что событие сверх предела отвергается при подаче целиком
		ASSERT_FALSE(parse("<a>" + string(64, 'x') + "</a>", settings, 0, error));
		// Выполняем проверку кода отказа по объёму события
		ASSERT_EQ(error, xml::error_t::OVERFLOW_LIMIT);
		// Выполняем проверку того, что событие сверх предела отвергается и при подаче кусками
		ASSERT_FALSE(parse("<a>" + string(64, 'x') + "</a>", settings, 7, error));
		// Выполняем проверку кода отказа по объёму события при подаче кусками
		ASSERT_EQ(error, xml::error_t::OVERFLOW_LIMIT);
		// Выполняем проверку того, что предел держится и на разделе дословного текста
		ASSERT_FALSE(parse("<a><![CDATA[" + string(64, 'x') + "]]></a>", settings, 0, error));
		// Выполняем проверку того, что предел держится и на примечании
		ASSERT_FALSE(parse("<a><!--" + string(64, 'x') + "--></a>", settings, 0, error));
		// Настройки разбора без предела объёма события
		xml::reader_t::settings_t free;
		// Снимаем предел объёма одного события
		free.maxEvent = 0;
		// Выполняем склеивание подряд идущих кусков содержимого
		free.mergeText = true;
		// Выполняем проверку того, что при снятом пределе событие любого объёма принимается
		ASSERT_TRUE(parse("<a>" + string(100000, 'x') + "</a>", free, 0, error));
	}
	/**
	 * Выполняем проверку отыскания повторных объявлений пространств имён
	 *
	 * @details Отыскание ведётся двумя путями: попарным сличением там, где объявлений
	 *          немного, и раскладкой по свёртке свыше того. Пути эти обязаны выносить
	 *          одно и то же решение, а раскладка заведена затем, что попарное сличение
	 *          растёт квадратом: узел с шестью десятками тысяч объявлений разбирался бы
	 *          три секунды вместо трёх сотых
	 *
	 * @note Проверяются оба пути и обе их границы: разлад между ними означал бы, что
	 *       повтор, отвергаемый на малом узле, на большом проходит насквозь
	 */
	{
		/**
		 * @brief Метод сборки узла с объявлениями пространств имён
		 *
		 * @param count количество собираемых объявлений
		 * @param at    место повторного объявления, `npos` собирает узел без повтора
		 * @return      собранный текст разметки
		 *
		 */
		const auto build = [](const size_t count, const size_t at) noexcept -> string {
			// Собираемый текст разметки
			string result("<a");
			/**
			 * Выполняем сборку объявлений пространств имён узла
			 */
			for(size_t i = 0; i < count; i++){
				// Получаем номер префикса очередного объявления
				const size_t number = (((at != string::npos) && (i == at)) ? (at / 2) : i);
				// Выполняем добавление очередного объявления пространства имён
				result.append(" xmlns:p").append(to_string(number)).append("=\"u").append(to_string(i)).append("\"");
			}
			// Выводим собранный текст разметки
			return result.append("/>");
		};
		// Настройки разбора со снятым пределом количества атрибутов
		xml::reader_t::settings_t settings;
		// Снимаем предел количества атрибутов узла
		settings.maxAttributes = 0xFFFFFFFF;
		/**
		 * Выполняем перебор количеств объявлений по обе стороны от границы путей
		 */
		for(const size_t count : {size_t(2), size_t(32), size_t(33), size_t(64), size_t(2000)}){
			// Выполняем проверку того, что узел без повтора принимается
			ASSERT_TRUE(parse(build(count, string::npos), settings, 0, error)) << count << " " << xml::message(error);
			/**
			 * Выполняем перебор мест повторного объявления
			 */
			for(const size_t at : {size_t(1), count / 2, count - 1}){
				// Пропускаем места, для этого количества объявлений неприменимые
				if((at == 0) || (at >= count)) continue;
				// Выполняем проверку того, что узел с повтором отвергается
				ASSERT_FALSE(parse(build(count, at), settings, 0, error)) << count << " " << at;
				// Выполняем проверку кода отказа по повторному объявлению
				ASSERT_EQ(error, xml::error_t::DUPLICATE_ATTRIBUTE) << count << " " << at;
			}
		}
		/**
		 * Выполняем проверку того, что разные префиксы с одним значением повтором не являются
		 *
		 * @note Раскладка ведётся по имени и префиксу, а не по объявляемому значению:
		 *       совпадение значений повтора не составляет
		 */
		{
			// Собираемый текст разметки с одним значением на все объявления
			string text("<a");
			/**
			 * Выполняем сборку объявлений пространств имён узла
			 */
			for(size_t i = 0; i < 2000; i++)
				// Выполняем добавление очередного объявления пространства имён
				text.append(" xmlns:p").append(to_string(i)).append("=\"u\"");
			// Выполняем проверку того, что узел принимается
			ASSERT_TRUE(parse(text.append("/>"), settings, 0, error)) << xml::message(error);
		}
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
		{"<r>\n\t<n a=\"1\" b=\"2\"/>\n</r>", {{"a", 2, 5}, {"b", 2, 11}}},
		/**
		 * Метка, разорванная возвратом каретки
		 *
		 * @note Концом строки договор считает и возврат каретки: отрезок, его несущий,
		 *       простым не является, и место в нём вычитанием не считается
		 */
		{"<r a=\"1\"\r   b=\"2\"/>", {{"a", 1, 4}, {"b", 2, 4}}},
		// Метка, разорванная возвратом каретки с переводом строки
		{"<r a=\"1\"\r\n   b=\"2\"/>", {{"a", 1, 4}, {"b", 2, 4}}},
		// Узел, стоящий за строкой, завершённой возвратом каретки
		{"<r>\r\t<n a=\"1\"/>\r</r>", {{"a", 2, 5}}}
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
/**
 * @brief Проверка восстановления места за подстановкой сущности с именем вне US-ASCII
 *
 * @details Место разбора за ссылкой на сущность восстанавливается по её длине, и считать
 *          её следует знаками, а не байтами: знак за пределами US-ASCII занимает
 *          несколько байтов, и счёт байтами сдвигал бы место всей последующей разметки
 *
 */
/**
 * @brief Проверка смещения за подстановкой сущности разной длины
 *
 * @details Подстановка замещает ссылку значением прямо в разбираемом тексте, и разница их
 *          длин изымается из счёта пройденных байтов. Значение бывает и длиннее ссылки, и
 *          короче её, и смещение разметки за подстановкой обязано отвечать исходному
 *          тексту в обоих случаях
 *
 */
TEST(CodecXmlReader, SpliceOffset) {
	/**
	 * @brief Метод получения смещения примечания за подстановкой сущности
	 *
	 * @param text разбираемый текст разметки
	 * @return     смещение примечания в исходном тексте
	 *
	 */
	const auto place = [](const string & text) noexcept -> uint64_t {
		// Настройки разбора с выдачей примечаний отдельным событием
		xml::reader_t::settings_t settings;
		// Выполняем включение выдачи примечаний отдельным событием
		settings.comments = true;
		// Объект потокового чтения текста разметки
		xml::reader_t reader(settings);
		// Смещение примечания в исходном тексте
		uint64_t result = 0;
		// Если передачу текста разметки выполнить не удалось, выводим пустое смещение
		if(!reader.feed(text)) return result;
		/**
		 * Выполняем перебор всех событий разбора
		 */
		while(reader.next()){
			// Если получено примечание, запоминаем его смещение
			if(reader.event() == xml::event_t::COMMENT)
				// Запоминаем смещение обнаруженного примечания
				result = reader.location().offset;
		}
		// Выводим смещение примечания в исходном тексте
		return result;
	};
	// Перечень проверяемых текстов разметки
	const vector <string> probes = {
		// Ссылка на сущность длиннее подставляемого значения
		"<!DOCTYPE r [<!ENTITY оченьдлинноеимя '<b/>'>]><r>&оченьдлинноеимя;</r><!--конец-->",
		// Подставляемое значение длиннее ссылки на сущность
		"<!DOCTYPE r [<!ENTITY e '<b>ааааааааааааааааааааа</b>'>]><r>&e;</r><!--конец-->",
		// Две подстановки подряд
		"<!DOCTYPE r [<!ENTITY e '<b/>'>]><r>&e;&e;</r><!--конец-->",
		// Подстановка внутри подстановки
		"<!DOCTYPE r [<!ENTITY i '<c/>'><!ENTITY e '<b>&i;</b>'>]><r>&e;</r><!--конец-->"
	};
	/**
	 * Выполняем перебор всех проверяемых текстов разметки
	 */
	for(const string & probe : probes)
		// Выполняем проверку смещения примечания в исходном тексте
		ASSERT_EQ(place(probe), static_cast <uint64_t> (probe.find("<!--конец-->"))) << probe;
}
TEST(CodecXmlReader, SpliceLocation) {
	/**
	 * @brief Метод получения места атрибута за подстановкой сущности
	 *
	 * @param name имя подставляемой сущности
	 * @return     место атрибута в исходном тексте
	 *
	 */
	const auto place = [](const string & name) noexcept -> xml::location_t {
		// Собираемый текст разметки с подстановкой сущности
		const string text = string("<!DOCTYPE a [<!ENTITY ").append(name).append(" \"<b/>\">]><a>&").append(name).append(";<c x=\"1\"/></a>");
		// Объект потокового чтения текста разметки
		xml::reader_t reader;
		// Место атрибута в исходном тексте
		xml::location_t result;
		// Если передачу текста разметки выполнить не удалось, выводим пустое место
		if(!reader.feed(text)) return result;
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
					// Запоминаем место обнаруженного атрибута
					result = attribute.location;
			}
		}
		// Выводим место атрибута в исходном тексте
		return result;
	};
	// Получаем место атрибута за сущностью с именем из знаков US-ASCII
	const xml::location_t plain = place("abc");
	// Получаем место атрибута за сущностью с именем вне US-ASCII
	const xml::location_t wide = place("дом");
	// Выполняем проверку того, что имя из знаков US-ASCII место дало
	ASSERT_GT(plain.column, static_cast <uint32_t> (0));
	// Выполняем проверку совпадения номера строки при обоих именах
	ASSERT_EQ(wide.line, plain.line);
	/**
	 * Выполняем проверку совпадения положения в строке при обоих именах
	 *
	 * @note Оба имени записаны тремя знаками, и разметка за ссылкой стоит в обоих текстах
	 *       на одном и том же месте: разница между ними лишь в числе байтов имени
	 */
	ASSERT_EQ(wide.column, plain.column);
}
TEST(CodecXmlReader, ManyAttributes) {
	/**
	 * @brief Метод разбора узла с заданным перечнем атрибутов
	 *
	 * @param tail       перечень атрибутов, дописываемый к узлу
	 * @param count      количество атрибутов, записываемых узлу перед перечнем
	 * @param namespaces признак разрешения префиксов по договору о пространствах имён
	 * @return           результат разбора текста разметки
	 *
	 */
	const auto walk = [](const string & tail, const size_t count, const bool namespaces) noexcept -> bool {
		// Собираемый текст разметки
		string text("<r");
		/**
		 * Выполняем запись атрибутов узла
		 *
		 * @note Имена подобраны неразличимыми по началу намеренно: поиск повторов
		 *       отсеивает имена ключом по началу, и различаться они обязаны разбором,
		 *       а не ключом
		 */
		for(size_t i = 0; i < count; i++)
			// Выполняем добавление очередного атрибута узла
			text.append(" alphabet").append(std::to_string(i)).append("=\"v\"");
		// Выполняем добавление перечня атрибутов к узлу
		text.append(tail).append("/>");
		// Настройки разбора текста разметки
		xml::reader_t::settings_t settings;
		// Выполняем установку разрешения префиксов по договору о пространствах имён
		settings.namespaces = namespaces;
		// Объект потокового чтения текста разметки
		xml::reader_t reader(settings);
		// Если передачу текста разметки выполнить не удалось, выводим отказ
		if(!reader.feed(text)) return false;
		// Выполняем перебор всех событий разбора
		while(reader.next());
		// Выводим результат разбора текста разметки
		return (reader.state() == xml::state_t::FINISHED);
	};
	/**
	 * Выполняем проверку поиска повторов при разном количестве атрибутов
	 *
	 * @note Поиск повторов ведётся двумя способами: попарным сличением при небольшом
	 *       количестве атрибутов и раскладкой по свёртке имени при большом. Проверяются
	 *       оба, и порог между ними накрыт с обеих сторон
	 */
	for(const size_t count : {size_t(4), size_t(32), size_t(33), size_t(300)}){
		// Выполняем проверку того, что узел без повторов разобран
		ASSERT_TRUE(walk("", count, true));
		// Выполняем проверку того, что повтор в конце перечня обнаружен
		ASSERT_FALSE(walk(" alphabet0=\"w\"", count, true));
		// Выполняем проверку того, что повтор обнаружен и без разрешения префиксов
		ASSERT_FALSE(walk(" alphabet0=\"w\"", count, false));
		/**
		 * Выполняем проверку сличения имён по обозначению пространства имён
		 *
		 * @note Разные префиксы одного пространства имён дают одно и то же имя, и
		 *       повтором является именно оно, а не запись с префиксом
		 */
		ASSERT_FALSE(walk(" xmlns:p=\"u\" xmlns:q=\"u\" p:k=\"1\" q:k=\"2\"", count, true));
		// Выполняем проверку того, что одно имя в разных пространствах повтором не является
		ASSERT_TRUE(walk(" xmlns:p=\"u\" xmlns:q=\"w\" p:k=\"1\" q:k=\"2\"", count, true));
		// Выполняем проверку того, что повтор объявления пространства имён обнаружен
		ASSERT_FALSE(walk(" xmlns:p=\"u\" xmlns:p=\"w\"", count, true));
	}
}
TEST(CodecXmlReader, DoctypeChunked) {
	/**
	 * Разбираемый текст с описанием типа документа, полным ловушек поиска его конца
	 *
	 * @note Знаки конца описания и конца внутреннего подмножества стоят здесь внутри
	 *       значений, примечаний и указаний обработчику: поиск обязан пройти мимо них
	 *       и найти настоящий конец, как бы текст ни был разбит на куски
	 */
	const string text =
		"<!DOCTYPE d [<!-- ] > -->"
		"<?pi ] > ?>"
		"<!ENTITY a \"]> value >\">"
		"<!ENTITY b 'quoted ] here'>"
		"<!ATTLIST r k CDATA \"] > \">"
		"]><r>&a;|&b;</r>";
	/**
	 * @brief Метод разбора текста разметки кусками заданной длины
	 *
	 * @param step длина куска исходного текста
	 * @return     собранный по событиям разбора слепок
	 *
	 */
	const auto walk = [&text](const size_t step) noexcept -> string {
		// Объект потокового чтения текста разметки
		xml::reader_t reader;
		// Собираемый слепок событий разбора
		string result;
		/**
		 * Выполняем передачу исходного текста кусками заданной длины
		 */
		for(size_t offset = 0; offset < text.size(); offset += step){
			// Получаем длину очередного куска исходного текста
			const size_t length = ((offset + step) < text.size() ? step : (text.size() - offset));
			// Если передачу очередного куска выполнить не удалось, выводим отказ
			if(!reader.feed(text.data() + offset, length, (offset + length) >= text.size()))
				return string("ОТКАЗ");
			/**
			 * Выполняем перебор всех событий разбора
			 */
			while(reader.next()){
				/**
				 * Если получено начало узла разметки
				 */
				if(reader.event() == xml::event_t::ELEMENT_OPEN){
					// Выполняем добавление имени узла к слепку
					result.append("<").append(reader.name().local).append(">");
					/**
					 * Выполняем перебор всех атрибутов узла
					 */
					for(const xml::attribute_t & attribute : reader.attributes())
						// Выполняем добавление атрибута узла к слепку
						result.append(attribute.name.local).append("=").append(attribute.value).append(";");
				// Выполняем добавление содержимого узла к слепку
				} else if(reader.event() == xml::event_t::TEXT) result.append(reader.text());
			}
		}
		// Если разбор до конца довести не удалось, выводим отказ
		if(reader.state() != xml::state_t::FINISHED) return string("ОТКАЗ");
		// Выводим собранный слепок событий разбора
		return result;
	};
	// Получаем слепок разбора текста, переданного целиком
	const string whole = walk(text.size());
	// Выполняем проверку того, что описание типа документа разобрано
	ASSERT_EQ(whole, string("<r>k=] > ;]> value >|quoted ] here"));
	/**
	 * Выполняем проверку совпадения разбора при передаче текста кусками
	 *
	 * @note Удерживаемый между кусками ход поиска обязан давать то же самое, что и
	 *       перебор с начала: разбиение исходного текста на куски для разбора незаметно
	 */
	for(size_t step = 1; step <= 8; step++)
		// Выполняем проверку совпадения слепка разбора кусками со слепком целиком
		ASSERT_EQ(walk(step), whole);
}
/**
 * @brief Проверка набора пробельных знаков и обращения с пробельным содержимым
 *
 * @details Договор относит к пробельным лишь пробел, знак горизонтального отступа,
 *          перевод строки и возврат каретки. Вертикальный отступ и подача страницы
 *          пробельными не считаются и знаками разметки вообще не являются, а значение
 *          атрибута xml:space, отличное от объявленных, ошибкой не считается: перечень
 *          его значений - требование действительности, которую разбор не проверяет
 *
 */
TEST(CodecXmlReader, SpaceChars) {
	/**
	 * @brief Метод разбора текста разметки целиком
	 *
	 * @param text разбираемый текст разметки
	 * @return     признак того, что текст разобран до конца
	 *
	 */
	const auto parse = [](const string & text) noexcept -> bool {
		// Объект потокового чтения текста разметки
		xml::reader_t reader;
		// Если передачу текста разметки выполнить не удалось, выводим отрицательный результат
		if(!reader.feed(text)) return false;
		// Выполняем перебор всех событий разбора
		while(reader.next());
		// Выводим признак того, что текст разобран до конца
		return (reader.state() == xml::state_t::FINISHED);
	};
	// Выполняем проверку разбора текста с пробельными знаками, допустимыми договором
	ASSERT_TRUE(parse("<a\tx=\"1\"\r\n y=\"2\" />"));
	/**
	 * Выполняем проверку отклонения знаков, пробельными по договору не являющихся
	 *
	 * @note Вертикальный отступ и подача страницы в разметку не допускаются вовсе, и
	 *       разделителем между именем узла и его атрибутом служить не вправе
	 */
	ASSERT_FALSE(parse("<a\vx=\"1\"/>"));
	// Выполняем проверку отклонения подачи страницы в качестве разделителя
	ASSERT_FALSE(parse("<a\fx=\"1\"/>"));
	// Выполняем проверку того, что объявленное обращение с пробельным содержимым принято
	ASSERT_TRUE(parse("<a xml:space=\"preserve\"> </a>"));
	/**
	 * Выполняем проверку того, что незнакомое значение обращения ошибкой не считается
	 *
	 * @note Перечень значений атрибута задан договором в объявлении типа документа, то
	 *       есть требованием действительности; разбор её не проверяет и отвергать такой
	 *       текст не вправе
	 */
	ASSERT_TRUE(parse("<a xml:space=\"keep\"> </a>"));
}
/**
 * @brief Проверка независимости отделения пробельного содержимого от границ кусков
 *
 * @details Незначимым пробельное содержимое считается лишь тогда, когда пробельным
 *          оказалось оно целиком. Выдаётся же содержимое частями по мере поступления
 *          кусков исходного текста, и часть его бывает пробельной, тогда как всё
 *          содержимое пробельным не является. Вид события обязан определяться самим
 *          содержимым, а не тем, где легла граница куска
 *
 */
TEST(CodecXmlReader, SeparateSpacesChunked) {
	/**
	 * @brief Метод сбора потока видов событий разбора
	 *
	 * @param text  разбираемый текст разметки
	 * @param chunk размер куска исходного текста, ноль передаёт текст целиком
	 * @return      поток видов событий со склеенным содержимым
	 *
	 */
	const auto parse = [](const string & text, const size_t chunk) noexcept -> string {
		// Собираемые настройки разбора текста разметки
		xml::reader_t::settings_t settings;
		// Выполняем включение отделения незначимого пробельного содержимого
		settings.separateSpaces = true;
		// Объект потокового чтения текста разметки
		xml::reader_t reader(settings);
		// Собираемый поток видов событий разбора
		string result;
		// Вид предыдущего события разбора
		int32_t prev = -1;
		// Положение передачи исходного текста
		size_t offset = 0;
		/**
		 * Выполняем передачу исходного текста кусками
		 */
		do {
			// Получаем размер очередного передаваемого куска
			const size_t size = (chunk == 0 ? text.size() : ::std::min(chunk, text.size() - offset));
			// Если передачу очередного куска выполнить не удалось, выводим собранное
			if(!reader.feed(text.data() + offset, size, ((offset + size) >= text.size()))) break;
			// Выполняем переход к следующему куску исходного текста
			offset += size;
			/**
			 * Выполняем перебор всех событий разбора
			 */
			while(reader.next()){
				// Получаем вид очередного события разбора
				const int32_t event = static_cast <int32_t> (reader.event());
				/**
				 * Если вид события сменился
				 */
				if(event != prev){
					// Выполняем добавление вида события к собираемому потоку
					result.append(1, '|').append(::std::to_string(event)).append(1, ':');
					// Запоминаем вид предыдущего события разбора
					prev = event;
				}
				// Выполняем добавление содержимого события к собираемому потоку
				result.append(reader.text());
			}
			// Если разбор прекращён ошибкой либо текст разобран до конца, выводим собранное
			if((reader.state() == xml::state_t::FAILED) || (reader.state() == xml::state_t::FINISHED)) break;
		} while(offset < text.size());
		// Выводим собранный поток видов событий разбора
		return result;
	};
	/**
	 * Выполняем перебор текстов, содержимое которых пробельным не является целиком
	 */
	for(const string & text : {
		string("<a>   hello   </a>"), string("<a>  &amp;  </a>"),
		string("<a>  <b/>  </a>"), string("<a>x<![CDATA[  ]]>  </a>")
	}){
		// Получаем поток видов событий разбора текста целиком
		const string expected = parse(text, 0);
		/**
		 * Выполняем перебор размеров куска исходного текста
		 */
		for(size_t chunk = 1; chunk <= 9; chunk++)
			// Выполняем проверку того, что разбиение на куски потока событий не изменило
			ASSERT_EQ(parse(text, chunk), expected) << "текст=" << text << " кусок=" << chunk;
	}
	// Выполняем проверку того, что содержимое, пробельное целиком, отделяется и кусками
	ASSERT_EQ(parse("<a>   </a>", 2), parse("<a>   </a>", 0));
	// Выполняем проверку того, что пробельное содержимое отделено отдельным событием
	ASSERT_NE(parse("<a>   </a>", 0).find(::std::to_string(static_cast <int32_t> (xml::event_t::SPACE))), string::npos);
}
/**
 * @brief Проверка изъятия разобранного начала приведённого текста
 *
 * @details Разбор изымает разобранное начало буфера, чтобы не удерживать весь текст
 *          целиком, и ведёт смещение от количества изъятых байтов. Изымается начало
 *          лишь у текста длиннее предела, и до этого предела не дотягивал ни один
 *          набор проверок: ход изъятия не исполнялся ни разу
 *
 * @note Склеивание кусков содержимого включено намеренно: без него содержимое
 *       разрывается границей куска, и потоки событий сличать было бы не с чем
 *
 */
TEST(CodecXmlReader, Compaction) {
	/**
	 * @brief Метод сбора потока событий с местами их начала
	 *
	 * @param text  разбираемый текст разметки
	 * @param chunk размер куска исходного текста, ноль передаёт текст целиком
	 * @return      поток событий разбора с местами их начала
	 *
	 */
	const auto parse = [](const string & text, const size_t chunk) noexcept -> string {
		// Собираемые настройки разбора текста разметки
		xml::reader_t::settings_t settings;
		// Выполняем включение склеивания кусков содержимого
		settings.mergeText = true;
		// Объект потокового чтения текста разметки
		xml::reader_t reader(settings);
		// Собираемый поток событий разбора
		string result;
		// Положение передачи исходного текста
		size_t offset = 0;
		/**
		 * Выполняем передачу исходного текста кусками
		 */
		do {
			// Получаем размер очередного передаваемого куска
			const size_t size = (chunk == 0 ? text.size() : ::std::min(chunk, text.size() - offset));
			// Если передачу очередного куска выполнить не удалось, выводим собранное
			if(!reader.feed(text.data() + offset, size, ((offset + size) >= text.size()))) break;
			// Выполняем переход к следующему куску исходного текста
			offset += size;
			/**
			 * Выполняем перебор всех событий разбора
			 */
			while(reader.next()){
				// Выполняем добавление вида события к собираемому потоку
				result.append(::std::to_string(static_cast <int32_t> (reader.event()))).append(1, '@');
				// Выполняем добавление смещения события от начала исходного текста
				result.append(::std::to_string(reader.location().offset)).append(1, ':');
				// Выполняем добавление номера строки события в исходном тексте
				result.append(::std::to_string(reader.location().line)).append(1, ':');
				// Выполняем добавление положения события в строке исходного текста
				result.append(::std::to_string(reader.location().column)).append(1, '|');
			}
			// Если разбор прекращён ошибкой либо текст разобран до конца, выводим собранное
			if((reader.state() == xml::state_t::FAILED) || (reader.state() == xml::state_t::FINISHED)) break;
		} while(offset < text.size());
		// Выводим собранный поток событий разбора
		return result;
	};
	// Собираемый текст разметки заведомо длиннее предела изъятия начала буфера
	string text = "<root>\n";
	/**
	 * Выполняем сборку текста разметки из множества однородных узлов
	 */
	for(size_t i = 0; i < 8000; i++){
		// Выполняем добавление очередного узла разметки
		text.append("  <item id=\"").append(::std::to_string(i)).append("\">значение ");
		// Выполняем завершение очередного узла разметки
		text.append(::std::to_string(i)).append("</item>\n");
	}
	// Выполняем завершение собираемого текста разметки
	text.append("</root>\n");
	// Выполняем проверку того, что собранный текст предел изъятия превысил
	ASSERT_GT(text.size(), static_cast <size_t> (0x10000));
	// Получаем поток событий разбора текста целиком
	const string expected = parse(text, 0);
	// Выполняем проверку того, что текст разобран
	ASSERT_FALSE(expected.empty());
	/**
	 * Выполняем перебор размеров куска исходного текста
	 */
	for(const size_t chunk : {static_cast <size_t> (0x400), static_cast <size_t> (0x1E61), static_cast <size_t> (0x10000), static_cast <size_t> (0x186A0)})
		// Выполняем проверку того, что изъятие начала буфера потока событий не изменило
		ASSERT_EQ(parse(text, chunk), expected) << "кусок=" << chunk;
	/**
	 * Выполняем проверку изъятия начала под разделом дословного текста
	 *
	 * @note Начало раздела задано положением в приведённом тексте и лежит позади места
	 *       разбора: изъятие обесценило бы его, а по нему проверяется и граница
	 *       подставленной сущности, и место самого события. Раздел взят длиннее предела
	 *       нарочно - изъятие обязано случиться, пока раздел ещё не дочитан
	 */
	{
		// Собираемый текст разметки с разделом дословного текста
		string verbatim = "<root><![CDATA[";
		/**
		 * Выполняем сборку раздела дословного текста длиннее предела изъятия
		 */
		for(size_t i = 0; i < 6000; i++)
			// Выполняем добавление очередного куска дословного текста
			verbatim.append("дословный кусок ").append(::std::to_string(i)).append("\n");
		// Выполняем завершение раздела дословного текста и корневого узла
		verbatim.append("]]></root>");
		// Выполняем проверку того, что собранный текст предел изъятия превысил
		ASSERT_GT(verbatim.size(), static_cast <size_t> (0x10000));
		// Получаем поток событий разбора текста целиком
		const string sample = parse(verbatim, 0);
		// Выполняем проверку того, что текст разобран
		ASSERT_FALSE(sample.empty());
		/**
		 * Выполняем перебор размеров куска исходного текста
		 */
		for(const size_t chunk : {static_cast <size_t> (0x400), static_cast <size_t> (0x10000)})
			// Выполняем проверку того, что изъятие начала буфера выдачу не изменило
			ASSERT_EQ(parse(verbatim, chunk), sample) << "дословный кусок=" << chunk;
	}
	/**
	 * Выполняем проверку изъятия начала при открытой подстановке сущности
	 *
	 * @note Область подстановки задана положениями в приведённом тексте, и изъятие его
	 *       начала эти положения обесценит: изъятие обязано быть отложено до закрытия
	 *       области. Подстановка взята длиннее предела нарочно
	 */
	{
		// Собираемое значение подставляемой сущности
		string value;
		/**
		 * Выполняем сборку значения сущности длиннее предела изъятия
		 */
		for(size_t i = 0; i < 6000; i++)
			// Выполняем добавление очередного куска значения сущности
			value.append("подставляемый кусок ").append(::std::to_string(i)).append(" ");
		// Собираемый текст разметки с объявлением сущности
		string entity = "<!DOCTYPE root [<!ENTITY big \"";
		// Выполняем добавление значения сущности и её подстановки
		entity.append(value).append("\">]>\n<root>&big;</root>");
		// Выполняем проверку того, что собранный текст предел изъятия превысил
		ASSERT_GT(entity.size(), static_cast <size_t> (0x10000));
		// Собираемые настройки разбора с поднятым пределом подстановки
		xml::reader_t::settings_t settings;
		// Выполняем поднятие предела общего объёма подстановки сущностей
		settings.maxExpansion = (16u * 1024u * 1024u);
		// Выполняем включение склеивания кусков содержимого
		settings.mergeText = true;
		/**
		 * @brief Метод разбора текста разметки с поднятым пределом подстановки
		 *
		 * @param chunk размер куска исходного текста, ноль передаёт текст целиком
		 * @return      длина подставленного содержимого
		 *
		 */
		const auto expand = [&entity, &settings](const size_t chunk) noexcept -> size_t {
			// Объект потокового чтения текста разметки
			xml::reader_t reader(settings);
			// Длина подставленного содержимого
			size_t length = 0;
			// Положение передачи исходного текста
			size_t offset = 0;
			/**
			 * Выполняем передачу исходного текста кусками
			 */
			do {
				// Получаем размер очередного передаваемого куска
				const size_t size = ((chunk == 0) ? entity.size() : ::std::min(chunk, entity.size() - offset));
				// Если передачу очередного куска выполнить не удалось, выводим собранное
				if(!reader.feed(entity.data() + offset, size, ((offset + size) >= entity.size()))) break;
				// Выполняем переход к следующему куску исходного текста
				offset += size;
				/**
				 * Выполняем перебор всех событий разбора
				 */
				while(reader.next()){
					// Если получено текстовое содержимое, учитываем его длину
					if(reader.event() == xml::event_t::TEXT)
						// Выполняем учёт длины подставленного содержимого
						length += reader.text().size();
				}
				// Если разбор прекращён ошибкой либо текст разобран до конца
				if((reader.state() == xml::state_t::FAILED) || (reader.state() == xml::state_t::FINISHED)) break;
			} while(offset < entity.size());
			// Выполняем проверку того, что разбор завершён удачно
			EXPECT_EQ(reader.state(), xml::state_t::FINISHED) << "кусок=" << chunk;
			// Выводим длину подставленного содержимого
			return length;
		};
		// Получаем длину подставленного содержимого при подаче текста целиком
		const size_t whole = expand(0);
		// Выполняем проверку того, что подстановка сущности выполнена
		ASSERT_EQ(whole, value.size());
		/**
		 * Выполняем перебор размеров куска исходного текста
		 */
		for(const size_t chunk : {static_cast <size_t> (0x400), static_cast <size_t> (0x10000)})
			// Выполняем проверку того, что изъятие начала буфера подстановку не сбило
			ASSERT_EQ(expand(chunk), whole) << "подстановка кусок=" << chunk;
	}
	// Выполняем добавление к тексту разметки недопустимой последовательности
	text.append("]]>");
	// Получаем поток событий разбора текста с ошибкой целиком
	const string broken = parse(text, 0);
	/**
	 * Выполняем перебор размеров куска исходного текста с ошибкой
	 */
	for(const size_t chunk : {static_cast <size_t> (0x400), static_cast <size_t> (0x10000)})
		// Выполняем проверку того, что изъятие начала буфера места ошибки не сбило
		ASSERT_EQ(parse(text, chunk), broken) << "кусок=" << chunk;
}
/**
 * @brief Проверка выдачи кодов ошибок разбора, не выдававшихся ни одной проверкой
 *
 * @details Замер охвата показал, что до этих ходов не доходило ни одно испытание.
 *          Ход, ни разу не исполнявшийся, не проверен ничем: код ошибки в нём может
 *          не отвечать поводу вовсе, а сама проверка - не срабатывать
 *
 */
TEST(CodecXmlReader, ErrorCodes) {
	/**
	 * @brief Метод разбора текста разметки целиком
	 *
	 * @param text     разбираемый текст разметки
	 * @param settings настройки разбора текста разметки
	 * @return         код ошибки прекращённого разбора
	 *
	 */
	const auto parse = [](const string & text, const xml::reader_t::settings_t & settings) noexcept -> xml::error_t {
		// Объект потокового чтения текста разметки
		xml::reader_t reader(settings);
		// Выполняем передачу исходного текста разметки
		reader.feed(text);
		// Выполняем перебор всех событий разбора
		while(reader.next());
		// Выводим код ошибки прекращённого разбора
		return reader.error();
	};
	// Настройки разбора текста разметки по умолчанию
	const xml::reader_t::settings_t defaults;
	/**
	 * Выполняем проверку отклонения превышенной глубины вложенности узлов
	 */
	{
		// Собираемые настройки разбора текста разметки
		xml::reader_t::settings_t settings;
		// Выполняем установку наибольшей допустимой глубины вложенности
		settings.maxDepth = 3;
		// Выполняем проверку выданного кода ошибки разбора
		ASSERT_EQ(parse("<a><b><c><e/></c></b></a>", settings), xml::error_t::DEPTH_EXCEEDED);
		// Выполняем проверку того, что глубина в пределах допустимой принимается
		ASSERT_EQ(parse("<a><b><c/></b></a>", settings), xml::error_t::NONE);
	}
	/**
	 * Выполняем проверку отклонения превышенного количества объявленных сущностей
	 */
	{
		// Собираемые настройки разбора текста разметки
		xml::reader_t::settings_t settings;
		// Выполняем установку наибольшего допустимого количества сущностей
		settings.maxEntities = 2;
		// Выполняем проверку выданного кода ошибки разбора
		ASSERT_EQ(parse("<!DOCTYPE a [<!ENTITY e1 \"1\"><!ENTITY e2 \"2\"><!ENTITY e3 \"3\">]><a/>", settings), xml::error_t::ENTITY_COUNT_EXCEEDED);
		// Выполняем проверку того, что количество в пределах допустимого принимается
		ASSERT_EQ(parse("<!DOCTYPE a [<!ENTITY e1 \"1\"><!ENTITY e2 \"2\">]><a/>", settings), xml::error_t::NONE);
	}
	/**
	 * Выполняем проверку отклонения превышенной глубины вложенности подстановок
	 */
	{
		// Собираемый текст разметки с длинной цепочкой подстановок сущностей
		string text = "<!DOCTYPE a [";
		/**
		 * Выполняем сборку цепочки сущностей, ссылающихся одна на другую
		 */
		for(size_t i = 1; i < 40; i++){
			// Выполняем добавление очередного объявления сущности
			text.append("<!ENTITY e").append(::std::to_string(i)).append(" \"&e");
			// Выполняем добавление ссылки на следующую сущность цепочки
			text.append(::std::to_string(i + 1)).append(";\">");
		}
		// Выполняем завершение цепочки сущностью со значением
		text.append("<!ENTITY e40 \"конец\">]><a>&e1;</a>");
		// Выполняем проверку выданного кода ошибки разбора
		ASSERT_EQ(parse(text, defaults), xml::error_t::ENTITY_DEPTH_EXCEEDED);
	}
	// Выполняем проверку отклонения ошибочно построенной ссылки на сущность
	ASSERT_EQ(parse("<a>&;</a>", defaults), xml::error_t::INVALID_REFERENCE);
	// Выполняем проверку отклонения ссылки без имени сущности
	ASSERT_EQ(parse("<a>&x</a>", defaults), xml::error_t::INVALID_REFERENCE);
	// Выполняем проверку отклонения числовой ссылки без записи кодового значения
	ASSERT_EQ(parse("<a>&#;</a>", defaults), xml::error_t::INVALID_CHAR_REFERENCE);
	// Выполняем проверку отклонения шестнадцатеричной ссылки без записи кодового значения
	ASSERT_EQ(parse("<a>&#x;</a>", defaults), xml::error_t::INVALID_CHAR_REFERENCE);
	// Выполняем проверку отклонения ошибочно построенного указания обработчику
	ASSERT_EQ(parse("<a><? ?></a>", defaults), xml::error_t::INVALID_PROCESSING);
	// Выполняем проверку отклонения такого указания внутри описания типа документа
	ASSERT_EQ(parse("<!DOCTYPE a [<? ?>]><a/>", defaults), xml::error_t::INVALID_PROCESSING);
}
/**
 * @brief Проверка правильности построения, вскрытая набором W3C
 *
 * @details Набор XML Test Suite содержит около двух тысяч случаев с заранее объявленным
 *          приговором, и на нём вскрылось девять построений, которые разбор принимал
 *          вопреки договору либо отвергал вопреки ему же. Каждое из них закреплено здесь
 *          отдельно, чтобы прогон набора не был единственным их сторожем
 *
 * @note Приговоры сверены со сторонним разбором expat: по всем девяти он согласен с
 *       набором, то есть вина лежала на нас, а не на строгости набора
 *
 */
TEST(CodecXmlReader, Conformance) {
	/**
	 * @brief Метод разбора текста разметки целиком
	 *
	 * @param text     разбираемый текст разметки
	 * @param settings настройки разбора текста разметки
	 * @return         признак того, что текст разобран до конца
	 *
	 */
	const auto parse = [](const string & text, const xml::reader_t::settings_t & settings) noexcept -> bool {
		// Объект потокового чтения текста разметки
		xml::reader_t reader(settings);
		// Выполняем передачу исходного текста разметки
		reader.feed(text);
		// Выполняем перебор всех событий разбора
		while(reader.next());
		// Выводим признак того, что текст разобран до конца
		return (reader.state() == xml::state_t::FINISHED);
	};
	// Настройки разбора текста разметки по умолчанию
	const xml::reader_t::settings_t defaults;
	/**
	 * Выполняем проверку отклонения объявления разметки с полем без значения
	 *
	 * @note Отсутствие поля и ошибочное его построение прежде давали разбору один и тот
	 *       же итог: объявление вида «encoding=» сходило за объявление без поля кодировки
	 */
	ASSERT_FALSE(parse("<?xml version=\"1.0\" encoding= ?><a/>", defaults));
	// Выполняем проверку отклонения объявления с полем без закрывающей кавычки
	ASSERT_FALSE(parse("<?xml version=\"1.0\" encoding=\"utf-8?><a/>", defaults));
	// Выполняем проверку того, что объявление без поля кодировки принимается
	ASSERT_TRUE(parse("<?xml version=\"1.0\"?><a/>", defaults));
	// Выполняем проверку того, что объявление с полем кодировки принимается
	ASSERT_TRUE(parse("<?xml version=\"1.0\" encoding=\"utf-8\"?><a/>", defaults));
	/**
	 * Выполняем проверку отклонения отведённого имени указания в начале текста
	 *
	 * @note Начало текста отведено объявлению разметки, и указание обработчику, названное
	 *       именем с началом «xml», прикрыло бы собой объявление, записанное без пробела
	 */
	ASSERT_FALSE(parse("<?xmlversion='1.0' ?><a/>", defaults));
	// Выполняем проверку того, что то же имя внутри текста разбором принимается
	ASSERT_TRUE(parse("<a><?xmlversion x?></a>", defaults));
	// Выполняем проверку того, что отведённое договором о стилях имя принимается
	ASSERT_TRUE(parse("<?xml-stylesheet href=\"x\"?><a/>", defaults));
	/**
	 * Выполняем проверку отклонения узла, закрытого подставленной сущностью
	 *
	 * @note Глубины стека для проверки мало: значение сущности здесь закрывает узел и
	 *       открывает новый, оставляя глубину прежней
	 */
	ASSERT_FALSE(parse("<!DOCTYPE a [<!ENTITY e \"</b><b>\">]><a><b>&e;</b></a>", defaults));
	// Выполняем проверку отклонения раздела дословного текста, начатого сущностью
	ASSERT_FALSE(parse("<!DOCTYPE a [<!ENTITY e \"&#60;![CDATA[\">]><a>&e;]]></a>", defaults));
	// Выполняем проверку отклонения примечания, начатого подставленной сущностью
	ASSERT_FALSE(parse("<!DOCTYPE a [<!ENTITY e \"&#60;!--\">]><a>&e;--></a>", defaults));
	// Выполняем проверку того, что разметка целиком внутри сущности принимается
	ASSERT_TRUE(parse("<!DOCTYPE a [<!ENTITY e \"&#60;b>текст&#60;/b>\">]><a>&e;</a>", defaults));
	/**
	 * Выполняем проверку отклонения записи за концом внутреннего подмножества
	 *
	 * @note За закрывающей скобкой подмножества договор допускает лишь пробельные знаки
	 */
	ASSERT_FALSE(parse("<!DOCTYPE a [<!ELEMENT a (#PCDATA)><!ENTITY % e \"\">] %e; ><a></a>", defaults));
	// Выполняем проверку того, что пробельные знаки за подмножеством принимаются
	ASSERT_TRUE(parse("<!DOCTYPE a [<!ELEMENT a (#PCDATA)>]  ><a></a>", defaults));
	/**
	 * Выполняем проверку отклонения ссылки вперёд в значении атрибута по умолчанию
	 *
	 * @note Объявления читаются по порядку, и ссылка на сущность, объявленную ниже,
	 *       опиралась бы на то, чего к этому мигу ещё нет
	 */
	ASSERT_FALSE(parse("<!DOCTYPE a [<!ATTLIST a k CDATA \"&e;\"><!ENTITY e \"v\">]><a/>", defaults));
	// Выполняем проверку того, что ссылка на объявленную выше сущность принимается
	ASSERT_TRUE(parse("<!DOCTYPE a [<!ENTITY e \"v\"><!ATTLIST a k CDATA \"&e;\">]><a/>", defaults));
	// Выполняем проверку отклонения ссылки на внешнюю сущность в значении по умолчанию
	ASSERT_FALSE(parse("<!DOCTYPE a [<!ENTITY e SYSTEM \"e\"><!ATTLIST a k CDATA \"&e;\">]><a/>", defaults));
	/**
	 * Выполняем проверку того, что необъявленная сущность не отвергается, когда
	 * объявления могут приходить извне разобранного
	 *
	 * @note Значение параметрической сущности разбор не разворачивает, и объявление
	 *       вправе лежать там: договор относит недостачу к действительности, а не к
	 *       построению текста
	 */
	ASSERT_TRUE(parse("<!DOCTYPE a [<!ENTITY % p \"&#60;!ENTITY e 'v'>\">%p;]><a>&z;</a>", defaults));
	// Выполняем проверку того, что без такой возможности недостача отвергается
	ASSERT_FALSE(parse("<!DOCTYPE a [<!ELEMENT a (#PCDATA)>]><a>&z;</a>", defaults));
	/**
	 * Выполняем проверку отклонения ссылки на сущность с ошибочно построенным именем
	 *
	 * @note Прежде имя ссылки не проверялось вовсе, а отказ выходил из того, что сущности
	 *       с таким именем не находилось. Там, где объявления вправе прийти извне
	 *       разобранного, отыскание не ведётся, и такая ссылка проходила разбор насквозь
	 */
	ASSERT_FALSE(parse("<!DOCTYPE a SYSTEM \"d.dtd\"><a>&65;</a>", defaults));
	// Выполняем проверку отклонения ссылки с пробельным знаком в имени
	ASSERT_FALSE(parse("<!DOCTYPE a SYSTEM \"d.dtd\"><a>&e 1;</a>", defaults));
	// Выполняем проверку отклонения ссылки с недопустимым знаком в начале имени
	ASSERT_FALSE(parse("<!DOCTYPE a SYSTEM \"d.dtd\"><a>&-x;</a>", defaults));
	// Выполняем проверку отклонения ошибочно построенной ссылки в значении атрибута
	ASSERT_FALSE(parse("<!DOCTYPE a SYSTEM \"d.dtd\"><a x=\"&65;\"/>", defaults));
	// Выполняем проверку того, что ссылка с правильно построенным именем принимается
	ASSERT_TRUE(parse("<!DOCTYPE a SYSTEM \"d.dtd\"><a>&e1;</a>", defaults));
	/**
	 * Выполняем проверку отклонения значения сущности с последовательностью, отведённой
	 * концу дословного раздела, подставляемой в содержимое узла
	 *
	 * @note Запрет проверялся по разбираемому тексту, где на месте сущности стоит ссылка,
	 *       и значения её не достигал вовсе
	 */
	ASSERT_FALSE(parse("<!DOCTYPE a [<!ENTITY e \"]]>\">]><a>&e;</a>", defaults));
	// Выполняем проверку отклонения такой последовательности внутри значения сущности
	ASSERT_FALSE(parse("<!DOCTYPE a [<!ENTITY e \"x]]>y\">]><a>&e;</a>", defaults));
	/**
	 * Выполняем проверку того, что значению атрибута последовательность эта дозволена
	 *
	 * @note Правило договора отнесено к содержимому узла, и переносить его на значения
	 *       атрибутов неверно
	 */
	ASSERT_TRUE(parse("<!DOCTYPE a [<!ENTITY e \"]]>\">]><a x=\"&e;\"/>", defaults));
	/**
	 * Выполняем проверку того, что знак, полученный ссылкой, запрета не нарушает
	 *
	 * @note Проверяется объявленное значение сущности, а не итог подстановки: запись
	 *       «]]&gt;» договором прямо предписана как правильная
	 */
	ASSERT_TRUE(parse("<a>]]&gt;</a>", defaults));
	// Выполняем проверку того, что запрет к вложенным подстановкам не относится
	ASSERT_TRUE(parse("<!DOCTYPE a [<!ENTITY e \"]]\"><!ENTITY f \"&e;>\">]><a>&f;</a>", defaults));
	/**
	 * Выполняем проверку отклонения указания обработчику без разделителя за целью
	 *
	 * @note Разбор имени останавливается на первом знаке, имени не принадлежащем, и всё,
	 *       что за ним, прежде уходило в данные указания молча
	 */
	ASSERT_FALSE(parse("<?pi[d?><a/>", defaults));
	// Выполняем проверку отклонения такого указания внутри содержимого узла
	ASSERT_FALSE(parse("<a><?pi[<! d?></a>", defaults));
	// Выполняем проверку того, что указание без данных вовсе принимается
	ASSERT_TRUE(parse("<a><?pi?></a>", defaults));
	// Выполняем проверку того, что указание с пробельным разделителем принимается
	ASSERT_TRUE(parse("<a><?pi d?></a>", defaults));
	// Выполняем проверку того, что дефис знаком имени является и указание принимается
	ASSERT_TRUE(parse("<a><?pi-x d?></a>", defaults));
}
/**
 * @brief Проверка договора о выданном содержимом при внешнем подмножестве
 *
 * @details Внешнее подмножество описания типа документа разбор не читает, и объявление
 *          сущности вправе лежать именно там. Недостача объявления отнесена договором к
 *          действительности, а не к построению текста, и разбор такой текст принимает.
 *          Следствие же таково, что ссылка из выданного содержимого пропадает вовсе, то
 *          есть содержимое полным не является
 *
 * @note Испытание закрепляет именно следствие, а не одно лишь принятие текста: принятие
 *       проверено набором Conformance, а вот молчаливая утрата знаков вызывающему видна
 *       не была и в договоре описана не была тоже
 *
 * @warning Выдать ссылку знаками текста нельзя: на письме она осталась бы ссылкой и
 *          прочлась бы обратно уже разметкой. Оттого пропуск здесь - решение намеренное,
 *          и подмена его выдачей исходных знаков договор нарушает
 *
 */
TEST(CodecXmlReader, ForeignSubsetContent) {
	/**
	 * @brief Метод сбора текстового содержимого разбираемого текста разметки
	 *
	 * @param text разбираемый текст разметки
	 * @return     пара из признака разбора до конца и склеенного содержимого узлов
	 *
	 */
	const auto parse = [](const string & text) noexcept -> pair <bool, string> {
		// Объект потокового чтения текста разметки
		xml::reader_t reader;
		// Собираемое текстовое содержимое узлов
		string result;
		// Выполняем передачу исходного текста разметки
		reader.feed(text);
		/**
		 * Выполняем перебор всех событий разбора
		 */
		while(reader.next()){
			// Если очередное событие несёт текстовое содержимое узла
			if(reader.event() == xml::event_t::TEXT)
				// Выполняем добавление содержимого события к собираемому тексту
				result.append(reader.text());
		}
		// Выводим пару из признака разбора до конца и собранного содержимого
		return make_pair((reader.state() == xml::state_t::FINISHED), ::move(result));
	};
	// Выполняем разбор текста с внешним подмножеством и необъявленной сущностью
	const auto foreign = parse("<!DOCTYPE a SYSTEM \"d.dtd\"><a>до&z;после</a>");
	// Выполняем проверку того, что текст этот разбор принимает
	ASSERT_TRUE(foreign.first);
	/**
	 * Выполняем проверку того, что ссылка из выданного содержимого пропала
	 *
	 * @note Знаки «&z;» не выданы ни ссылкой, ни подстановкой: содержимое склеено из
	 *       того, что стоит по обе стороны от неё
	 */
	ASSERT_EQ(foreign.second, string("допосле"));
	// Выполняем разбор того же текста без внешнего подмножества
	const auto internal = parse("<!DOCTYPE a [<!ELEMENT a (#PCDATA)>]><a>до&z;после</a>");
	// Выполняем проверку того, что без внешнего подмножества недостача отвергается
	ASSERT_FALSE(internal.first);
	// Выполняем разбор текста с внешним подмножеством и объявленной сущностью
	const auto declared = parse("<!DOCTYPE a SYSTEM \"d.dtd\" [<!ENTITY z \"|\">]><a>до&z;после</a>");
	// Выполняем проверку того, что объявленная сущность подставляется по-прежнему
	ASSERT_TRUE(declared.first);
	// Выполняем проверку того, что значение сущности стоит на месте ссылки
	ASSERT_EQ(declared.second, string("до|после"));
}
TEST(CodecXmlReader, SpliceTail) {
	/**
	 * @brief Метод получения места разметки за подстановкой сущности
	 *
	 * @param text разбираемый текст разметки
	 * @return     место узла «c» в исходном тексте
	 *
	 */
	const auto place = [](const string & text) noexcept -> xml::location_t {
		// Объект потокового чтения текста разметки
		xml::reader_t reader;
		// Место узла в исходном тексте
		xml::location_t result;
		// Если передачу текста разметки выполнить не удалось, выводим пустое место
		if(!reader.feed(text)) return result;
		/**
		 * Выполняем перебор всех событий разбора
		 */
		while(reader.next()){
			/**
			 * Если получено начало искомого узла разметки
			 */
			if((reader.event() == xml::event_t::ELEMENT_OPEN) && (reader.name().local.compare("c") == 0))
				// Запоминаем место обнаруженного узла
				result = reader.location();
		}
		// Выводим место узла в исходном тексте
		return result;
	};
	/**
	 * Выполняем проверку места разметки за хвостом подстановки
	 *
	 * @note Содержимое узла обрывается началом следующей разметки, а не границей
	 *       сущности: хвост «tail» принадлежит уже исходному тексту, и место за ним
	 *       обязано считаться по нему, а не оставаться на месте за ссылкой
	 */
	const xml::location_t tail = place("<!DOCTYPE d [<!ENTITY e \"<b/>xy\">]><a>&e;tail<c/></a>");
	// Выполняем проверку положения узла в строке за хвостом подстановки
	ASSERT_EQ(tail.column, static_cast <uint32_t> (46));
	// Выполняем проверку номера строки узла за хвостом подстановки
	ASSERT_EQ(tail.line, static_cast <uint32_t> (1));
	// Выполняем проверку места разметки за хвостом подстановки с концом строки
	const xml::location_t broken = place("<!DOCTYPE d [<!ENTITY e \"<b/>x\">]><a>&e;t\nail<c/></a>");
	// Выполняем проверку номера строки узла за концом строки в хвосте подстановки
	ASSERT_EQ(broken.line, static_cast <uint32_t> (2));
	// Выполняем проверку положения узла в строке за концом строки в хвосте подстановки
	ASSERT_EQ(broken.column, static_cast <uint32_t> (4));
	// Выполняем проверку места разметки за двумя подстановками подряд
	const xml::location_t twice = place("<!DOCTYPE d [<!ENTITY e \"<b/>x\">]><a>&e;&e;q<c/></a>");
	// Выполняем проверку того, что отставание места с подстановками не накапливается
	ASSERT_EQ(twice.column, static_cast <uint32_t> (45));
	// Выполняем проверку места разметки за вложенной подстановкой
	const xml::location_t nested = place("<!DOCTYPE d [<!ENTITY i \"<b/>\"><!ENTITY o \"<w>&i;</w>z\">]><a>&o;q<c/></a>");
	// Выполняем проверку того, что вложенная подстановка место не сбивает
	ASSERT_EQ(nested.column, static_cast <uint32_t> (66));
}
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
/**
 * @brief Проверка сложности разбора
 *
 * @note Проверка эта - единственная во всём наборе, что судит не о правильности
 *       разбора, а о его стоимости. Заведена она затем, что обе просадки, найденные
 *       замером, весь остальной набор проходили: итог разбора они не меняли вовсе,
 *       меняли лишь время. Судится потому не время само по себе, зависящее от машины
 *       и от загрузки, а отношение времён двух прогонов на одной и той же машине -
 *       им устройство разбора и выражается
 *
 * @warning Пределы отношений взяты с большим запасом против замеренного, чтобы
 *          проверка не отказывала от случайной загрузки машины: отсекается порядок
 *          роста, а не проигрыш в разы. Всякий отказ здесь означает возврат к
 *          разбору с иным порядком сложности, а не потерю нескольких процентов
 */
TEST(CodecXmlReader, Scaling) {
	/**
	 * @brief Метод замера времени разбора текста разметки
	 *
	 * @param text  разбираемый текст разметки
	 * @param limit наибольшее допустимое количество атрибутов у одного узла
	 * @return      наименьшее время разбора из нескольких заходов в секундах
	 *
	 * @note Берётся наименьшее из заходов, а не среднее: наименьшее выражает
	 *       стоимость самого разбора, а всё сверх него добавлено соседями по машине
	 */
	const auto spent = [](const string & text, const uint32_t limit) noexcept -> double {
		// Наименьшее время разбора из выполненных заходов
		double result = -1.;
		/**
		 * Выполняем несколько заходов разбора одного и того же текста
		 */
		for(size_t pass = 0; pass < 3; pass++){
			// Настройки разбора текста разметки
			xml::reader_t::settings_t settings;
			// Выполняем установку наибольшего допустимого количества атрибутов узла
			settings.maxAttributes = limit;
			// Объект потокового чтения текста разметки
			xml::reader_t reader(settings);
			// Время начала разбора текста разметки
			const auto begin = chrono::steady_clock::now();
			// Если передачу текста разметки выполнить не удалось, выводим отказ
			if(!reader.feed(text)) return -1.;
			// Выполняем перебор всех событий разбора
			while(reader.next());
			// Время, затраченное на разбор текста разметки
			const double passed = chrono::duration <double> (chrono::steady_clock::now() - begin).count();
			// Если разбор текста разметки завершён неудачно, выводим отказ
			if(reader.state() != xml::state_t::FINISHED) return -1.;
			// Если время очередного захода оказалось наименьшим, запоминаем его
			if((result < 0.) || (passed < result)) result = passed;
		}
		// Выводим наименьшее время разбора из выполненных заходов
		return result;
	};
	/**
	 * @brief Метод сборки текста разметки с заданным количеством объявлений пространств имён
	 *
	 * @param count количество объявлений пространств имён у одного узла
	 * @return      собранный текст разметки
	 */
	const auto declares = [](const size_t count) noexcept -> string {
		// Собираемый текст разметки
		string result("<r");
		/**
		 * Выполняем запись объявлений пространств имён узла
		 *
		 * @note Обозначения подобраны различными намеренно: повтор объявления
		 *       разбор отвергает, а искать его приходится среди всех прочих
		 */
		for(size_t i = 0; i < count; i++)
			// Выполняем добавление очередного объявления пространства имён
			result.append(" xmlns:p").append(std::to_string(i)).append("=\"urn:example:").append(std::to_string(i)).append("\"");
		// Выводим собранный текст разметки
		return result.append("/>");
	};
	/**
	 * Выполняем проверку стоимости поиска повторов среди объявлений пространств имён
	 *
	 * @note Поиск повторов обязан вестись раскладкой по свёртке обозначения, а не
	 *       попарным сличением: замер давал на шестидесяти четырёх тысячах объявлений
	 *       три секунды вместо десяти миллисекунд
	 *
	 * @warning Объёмы выбраны малыми намеренно. У систем со страницей в 4 КБ стоимость
	 *          разбора одного объявления возрастает втрое, начиная примерно с шестнадцати
	 *          тысяч объявлений на узле: объём рабочих данных перерастает охват буфера
	 *          трансляции адресов, и на страницах вчетверо меньших промахи его начинаются
	 *          вчетверо раньше. Рост при этом остаётся линейным по обе стороны от порога,
	 *          но пара 8000/32000 приходилась ровно на него и давала девятикратный рост
	 *          при дозволенном восьмикратном - на OpenBSD 7.9 aarch64 устойчиво, в трёх
	 *          прогонах подряд. Проверено: у macOS со страницей в 16 КБ того же порога на
	 *          этих объёмах нет, а сборка Release растёт там ровно вдвое на удвоение
	 */
	{
		// Текст разметки с малым количеством объявлений пространств имён
		const string small = declares(2000);
		// Текст разметки с четырёхкратным количеством объявлений пространств имён
		const string large = declares(8000);
		// Время разбора текста разметки с малым количеством объявлений
		const double first = spent(small, 65536);
		// Время разбора текста разметки с четырёхкратным количеством объявлений
		const double second = spent(large, 65536);
		// Выполняем проверку того, что разбор обоих текстов завершён удачно
		ASSERT_GT(first, 0.);
		ASSERT_GT(second, 0.);
		/**
		 * Выполняем проверку того, что рост стоимости не превышает восьмикратного
		 *
		 * @note Четырёхкратный рост количества объявлений при линейной стоимости
		 *       даёт рост времени вчетверо, при квадратичной - в шестнадцать раз.
		 *       Порог посередине разделяет их надёжно
		 */
		ASSERT_LT(second, (first * 8.)) << "рост количества объявлений вчетверо поднял время в " << (second / first) << " раз";
	}
}
/**
 * @brief Проверка выдачи разбора при отказе приведения исходного текста
 *
 * @details Приведение к кодировке UTF-8 переносит в хранилище всё, что успело
 *          проверить, и лишь затем отвечает отказом. Отказ выдаётся не сразу, а
 *          по исчерпании приведённого начала текста: иначе разбор одного и того
 *          же текста целиком и кусками расходился бы событиями - кусками события
 *          начала текста выдаются, а целиком нет
 *
 * @note Найдено ворошителем `tools/fuzz/xml.cpp`: подача целиком давала ноль
 *       событий там, где подача кусками давала объявление разметки
 *
 */
TEST(CodecXmlReader, DecodingPrefix) {
	// Код ошибки разбора текста разметки
	xml::error_t error = xml::error_t::NONE;
	// Текст разметки с недопустимой последовательностью UTF-8 в содержимом узла
	const string text("<?xml version=\"1.0\"?>\n<a>bad\xC3\x28 tail</a>\n");
	// Выполняем разбор текста разметки, поданного целиком
	const string whole = run(text, text.size(), error);
	// Выполняем проверку кода отказа приведения при подаче текста целиком
	ASSERT_EQ(error, xml::error_t::INVALID_ENCODING);
	// Выполняем разбор того же текста разметки, поданного кусками
	const string chunked = run(text, 5, error);
	// Выполняем проверку кода отказа приведения при подаче текста кусками
	ASSERT_EQ(error, xml::error_t::INVALID_ENCODING);
	// Выполняем проверку совпадения выдачи разбора при обоих способах подачи
	ASSERT_EQ(whole, chunked);
	// Выполняем проверку того, что разобранное начало текста выдано событиями
	ASSERT_NE(whole.find("[decl 1.0]"), string::npos) << whole;
	// Выполняем проверку того, что начало узла разметки выдано событием
	ASSERT_NE(whole.find("<a>"), string::npos) << whole;
}
/**
 * @brief Проверка независимости предела объёма события от нарезки текста на куски
 *
 * @details Предел сличается по ходу накопления с длиной накопленной разметки, а
 *          не с содержимым события: содержимое к тому мигу ещё неизвестно. Событиям,
 *          чьё содержимое короче своей разметки, - началу узла, описанию типа
 *          документа, указанию обработчику, содержимому со ссылками на сущности -
 *          одной проверки содержимого потому мало
 *
 * @note Найдено ворошителем `tools/fuzz/xml.cpp`: те же тексты при подаче целиком
 *       разбирались, а кусками отвергались по превышению предела
 *
 */
TEST(CodecXmlReader, EventLimitChunking) {
	/**
	 * @brief Разметка, чьё содержимое события короче её самой
	 *
	 */
	const char * items[] = {
		"<!DOCTYPE a SYSTEM \"веcьма-длинное-обозначение-источника.dtd\">\n<a/>",
		"<a очень-длинное-имя-атрибута=\"значение\" второе-длинное-имя=\"значение\"/>",
		"<a><?обработчик весьма длинные данные указания обработчику?></a>",
		"<a>&lt;&amp;&gt;&quot;&apos;&lt;&amp;&gt;&quot;&apos;</a>",
		"<a><!-- весьма длинное примечание, не помещающееся в предел --></a>",
		"<a></узел-с-весьма-длинным-именем-закрывающей-метки>",
		"<a><![CDATA[весьма длинный раздел дословного текста]]></a>"
	};
	/**
	 * Выполняем перебор всех разновидностей разметки
	 */
	for(const char * item : items){
		// Настройки разбора с заданным пределом объёма события
		xml::reader_t::settings_t settings;
		// Выполняем установку предела объёма одного события
		settings.maxEvent = 24;
		// Выполняем склеивание подряд идущих кусков содержимого
		settings.mergeText = true;
		// Код ошибки разбора текста, поданного целиком
		xml::error_t first = xml::error_t::NONE;
		// Выполняем разбор текста разметки, поданного целиком
		const string whole = run(item, ::strlen(item), first, settings);
		// Код ошибки разбора текста, поданного кусками
		xml::error_t second = xml::error_t::NONE;
		// Выполняем разбор того же текста разметки, поданного по три байта
		const string chunked = run(item, 3, second, settings);
		// Выполняем проверку совпадения кода отказа при обоих способах подачи
		ASSERT_EQ(first, second) << item;
		// Выполняем проверку совпадения выдачи разбора при обоих способах подачи
		ASSERT_EQ(whole, chunked) << item;
		// Выполняем проверку того, что предел объёма события удержан
		ASSERT_EQ(first, xml::error_t::OVERFLOW_LIMIT) << item;
	}
	/**
	 * @brief Разметка, событий не дающая вовсе
	 *
	 * @details Пробельное содержимое вне корневого узла разбор пропускает молча, а
	 * содержимое, целиком составленное ссылками на сущности с пустыми значениями,
	 * приводится к пустоте и событием не выдаётся. Накопление кусков сличает с
	 * пределом длину разметки и на этих ходах, и пропускать их без той же проверки
	 * нельзя: разбор кусками отвечал бы отказом там, где разбор целиком проходит
	 *
	 */
	const char * skipped[] = {
		"                                                            <a/>",
		"<!DOCTYPE a [<!ENTITY e \"\">]><a>&e;&e;&e;&e;&e;&e;&e;&e;&e;&e;</a>"
	};
	/**
	 * Выполняем перебор всех разновидностей разметки, событий не дающей
	 */
	for(const char * item : skipped){
		// Настройки разбора с заданным пределом объёма события
		xml::reader_t::settings_t settings;
		// Выполняем установку предела объёма одного события
		settings.maxEvent = 24;
		// Выполняем склеивание подряд идущих кусков содержимого
		settings.mergeText = true;
		// Выполняем включение подстановки ссылок на объявленные сущности
		settings.entities = true;
		// Код ошибки разбора текста, поданного целиком
		xml::error_t first = xml::error_t::NONE;
		// Выполняем разбор текста разметки, поданного целиком
		const string whole = run(item, ::strlen(item), first, settings);
		/**
		 * Выполняем перебор размеров куска подачи текста разметки
		 */
		for(size_t step = 1; step < 8; step++){
			// Код ошибки разбора текста, поданного кусками
			xml::error_t second = xml::error_t::NONE;
			// Выполняем разбор того же текста разметки, поданного кусками
			const string chunked = run(item, step, second, settings);
			// Выполняем проверку совпадения кода отказа при обоих способах подачи
			ASSERT_EQ(first, second) << item << " шаг " << step;
			// Выполняем проверку совпадения выдачи разбора при обоих способах подачи
			ASSERT_EQ(whole, chunked) << item << " шаг " << step;
		}
		// Выполняем проверку того, что предел объёма события удержан
		ASSERT_EQ(first, xml::error_t::OVERFLOW_LIMIT) << item;
	}
}
/**
 * @brief Проверка отсутствия пустых событий содержимого узла
 *
 * @details Содержимое, целиком составленное ссылками на сущности с пустыми
 *          значениями, приводится к пустоте. Выдавать пустое событие незачем: узлу
 *          оно ничего не добавляет, а дерево получает от него пустой узел
 *          содержимого, из-за которого запись узла перестаёт складываться
 *          самозакрывающейся меткой - и переход текст→дерево→текст, повторённый
 *          дважды, даёт разное написание
 *
 * @note Найдено ворошителем `tools/fuzz/xml.cpp` по неустойчивости перезаписи
 *
 */
TEST(CodecXmlReader, EmptyContent) {
	// Код ошибки разбора текста разметки
	xml::error_t error = xml::error_t::NONE;
	// Настройки разбора с подстановкой ссылок на объявленные сущности
	xml::reader_t::settings_t settings;
	// Выполняем включение подстановки ссылок на объявленные сущности
	settings.entities = true;
	// Выполняем склеивание подряд идущих кусков содержимого
	settings.mergeText = true;
	// Выполняем разбор текста разметки со ссылкой на пустую сущность
	const string result = run("<!DOCTYPE a [<!ENTITY e \"\">]><a><c>&e;</c></a>", 0x1000, error, settings);
	// Выполняем проверку того, что разбор завершён удачно
	ASSERT_EQ(error, xml::error_t::NONE);
	/**
	 * Выполняем разбор того же текста разметки без ссылки на пустую сущность
	 *
	 * @note Ссылка на сущность с пустым значением содержимого узлу не добавляет,
	 *       и выдача разбора обязана совпасть с выдачей по тексту без неё
	 */
	const string plain = run("<!DOCTYPE a [<!ENTITY e \"\">]><a><c></c></a>", 0x1000, error, settings);
	// Выполняем проверку того, что разбор завершён удачно
	ASSERT_EQ(error, xml::error_t::NONE);
	// Выполняем проверку совпадения выдачи разбора обоих текстов разметки
	ASSERT_EQ(result, plain);
}
/**
 * @brief Проверка приведения концов строк во всех местах разметки
 *
 * @details Договор предписывает приводить возврат каретки и его пару с переводом
 *          строки к одному переводу строки — и делать это до всякого разбора, то
 *          есть повсюду: в содержимом узла, в разделе дословного текста, в значении
 *          атрибута, в значении объявляемой сущности и в объявленном по умолчанию
 *          значении атрибута
 *
 * @note Замер охвата показал, что приведение внутри дословного раздела и внутри
 *       объявлений описания типа не проверялось ничем
 *
 */
TEST(CodecXmlReader, LineEndings) {
	// Код ошибки разбора текста разметки
	xml::error_t error = xml::error_t::NONE;
	// Настройки разбора текста разметки
	xml::reader_t::settings_t settings;
	// Выполняем включение подстановки ссылок на объявленные сущности
	settings.entities = true;
	// Выполняем включение подстановки значений атрибутов по умолчанию
	settings.defaults = true;
	// Выполняем склеивание подряд идущих кусков содержимого
	settings.mergeText = true;
	/**
	 * Выполняем проверку приведения концов строк в содержимом узла
	 */
	{
		// Выполняем разбор содержимого узла с концами строк всех видов
		const string result = run(string("<a>первая\rвторая\r\nтретья\n</a>"), 0x1000, error, settings);
		// Выполняем проверку того, что разбор завершён удачно
		ASSERT_EQ(error, xml::error_t::NONE);
		// Выполняем проверку того, что возврат каретки в содержимое не попал
		ASSERT_EQ(result.find('\r'), string::npos);
		// Выполняем проверку приведённого содержимого узла
		ASSERT_NE(result.find("первая\nвторая\nтретья\n"), string::npos) << result;
	}
	/**
	 * Выполняем проверку приведения концов строк в разделе дословного текста
	 */
	{
		// Выполняем разбор раздела дословного текста с концами строк всех видов
		const string result = run(string("<a><![CDATA[первая\rвторая\r\nтретья]]></a>"), 0x1000, error, settings);
		// Выполняем проверку того, что разбор завершён удачно
		ASSERT_EQ(error, xml::error_t::NONE);
		// Выполняем проверку того, что возврат каретки в содержимое не попал
		ASSERT_EQ(result.find('\r'), string::npos) << result;
		// Выполняем проверку приведённого содержимого дословного раздела
		ASSERT_NE(result.find("первая\nвторая\nтретья"), string::npos) << result;
	}
	/**
	 * Выполняем проверку приведения концов строк в значении атрибута
	 *
	 * @note Пробельные знаки значения атрибута договор велит заменять пробелом, и
	 *       пара из возврата с переводом обязана дать один пробел, а не два
	 */
	{
		// Выполняем разбор значения атрибута с концами строк всех видов
		const string result = run(string("<a k=\"первое\rвторое\r\nтретье\"/>"), 0x1000, error, settings);
		// Выполняем проверку того, что разбор завершён удачно
		ASSERT_EQ(error, xml::error_t::NONE);
		// Выполняем проверку приведённого значения атрибута
		ASSERT_NE(result.find("первое второе третье"), string::npos) << result;
	}
	/**
	 * Выполняем проверку приведения концов строк в значении объявленной сущности
	 */
	{
		// Выполняем разбор текста разметки со ссылкой на объявленную сущность
		const string result = run(string("<!DOCTYPE a [<!ENTITY e \"первое\rвторое\r\nтретье\">]><a>&e;</a>"), 0x1000, error, settings);
		// Выполняем проверку того, что разбор завершён удачно
		ASSERT_EQ(error, xml::error_t::NONE);
		// Выполняем проверку того, что возврат каретки в содержимое не попал
		ASSERT_EQ(result.find('\r'), string::npos) << result;
		// Выполняем проверку подставленного значения сущности
		ASSERT_NE(result.find("первое\nвторое\nтретье"), string::npos) << result;
	}
	/**
	 * Выполняем проверку приведения концов строк в значении атрибута по умолчанию
	 */
	{
		// Выполняем разбор текста разметки с объявленным умолчанием атрибута
		const string result = run(string("<!DOCTYPE a [<!ATTLIST a k CDATA \"первое\rвторое\r\nтретье\">]><a/>"), 0x1000, error, settings);
		// Выполняем проверку того, что разбор завершён удачно
		ASSERT_EQ(error, xml::error_t::NONE);
		// Выполняем проверку подставленного значения атрибута по умолчанию
		ASSERT_NE(result.find("первое второе третье"), string::npos) << result;
	}
	/**
	 * Выполняем проверку совпадения выдачи при подаче текста кусками
	 *
	 * @note Пара из возврата с переводом строки разрывается границей куска, и
	 *       приведение обязано остаться тем же самым
	 */
	{
		// Разбираемый текст разметки с концами строк всех видов
		const string text("<a k=\"з\rн\r\nа\">первая\rвторая\r\n<![CDATA[третья\r\n]]></a>");
		// Выполняем разбор текста разметки, поданного целиком
		const string whole = run(text, text.size(), error, settings);
		/**
		 * Выполняем перебор размеров куска подачи текста разметки
		 */
		for(size_t step = 1; step < 8; step++){
			// Выполняем разбор того же текста разметки, поданного кусками
			ASSERT_EQ(run(text, step, error, settings), whole) << step;
		}
	}
}
/**
 * @brief Проверка независимости места события от нарезки текста на куски
 *
 * @details Место события считается по ходу разбора и только вперёд, а раздел
 *          дословного текста разбор начинает на одном куске и дочитывает на
 *          следующих: к мигу выдачи его начало остаётся позади места разбора.
 *          Местом события служит начало самого раздела, и оно обязано совпадать
 *          при всякой нарезке
 *
 * @note Найдено разбором правок: прежде раздел, разорванный границей куска, давал
 *       местом события начало своего содержимого, а пришедший целиком - начало
 *       самой записи раздела
 *
 */
TEST(CodecXmlReader, EventLocation) {
	/**
	 * @brief Метод сборки записи мест всех событий разбора
	 *
	 * @param text разбираемый текст разметки
	 * @param step размер куска подаваемого текста
	 * @return     собранная запись мест событий разбора
	 *
	 */
	const auto places = [](const string & text, const size_t step) noexcept -> string {
		// Собираемая запись мест событий разбора
		string result;
		/**
		 * Настройки разбора со склеиванием кусков содержимого
		 *
		 * @note Склеивание обязательно: без него содержимое выдаётся частями по мере
		 *       поступления кусков, и мест событий столько же, сколько кусков
		 */
		xml::reader_t::settings_t settings;
		// Выполняем склеивание подряд идущих кусков содержимого
		settings.mergeText = true;
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
			// Если передачу куска текста выполнить не удалось, выходим из подачи
			if(!reader.feed(text.data() + offset, size, ((offset + size) >= text.size()))) break;
			// Выполняем переход к следующему куску текста
			offset += size;
			/**
			 * Выполняем перебор всех событий разбора
			 */
			while(reader.next()){
				// Выполняем добавление вида очередного события
				result.append(std::to_string(static_cast <uint32_t> (reader.event())));
				// Выполняем добавление места очередного события
				result.append("@").append(std::to_string(reader.location().line));
				result.append(":").append(std::to_string(reader.location().column));
				result.append("+").append(std::to_string(reader.location().offset)).append(" ");
			}
			// Если разбор прекращён ошибкой, выходим из подачи
			if(reader.state() == xml::state_t::FAILED) break;
			// Если текст разметки исчерпан, выходим из подачи
			if(offset >= text.size()) break;
		}
		// Выводим собранную запись мест событий разбора
		return result;
	};
	/**
	 * @brief Разметка, места событий которой проверяются
	 *
	 */
	const string items[] = {
		string("<a><![CDATA[ раздел\nдословного текста ]]>хвост</a>"),
		string("<a>\n <b/>\n <!-- примечание -->\n <?цель данные?>\n <![CDATA[]]>\n</a>"),
		string("<?xml version=\"1.0\"?>\n<!DOCTYPE a>\n<a k=\"v\">\r\nсодержимое\r\n</a>")
	};
	/**
	 * Выполняем перебор всех проверяемых текстов разметки
	 */
	for(const string & item : items){
		// Выполняем разбор текста разметки, поданного целиком
		const string whole = places(item, item.size());
		// Выполняем проверку того, что события выданы
		ASSERT_FALSE(whole.empty()) << item;
		/**
		 * Выполняем перебор размеров куска подачи текста разметки
		 */
		for(size_t step = 1; step < 12; step++){
			// Выполняем проверку совпадения мест событий при подаче кусками
			ASSERT_EQ(places(item, step), whole) << item << " шаг " << step;
		}
	}
}
/**
 * @brief Проверка старшинства отказов при нескольких нарушениях сразу
 *
 * @details Текст, и предел объёма события перебравший, и построенный ошибочно,
 *          отвергается по пределу при всякой нарезке. Иначе выдача зависела бы от
 *          устройства сети: накопление кусков сличает с пределом накопленное и
 *          отвергает текст, не дойдя ни до конца построения, ни до ошибки в нём,
 *          тогда как текст, пришедший целиком, видит их сразу
 *
 * @note Найдено разбором правок: обход открывающей метки заходил за предел и
 *       отвечал ошибкой построения, а тот же текст кусками - превышением предела
 *
 */
TEST(CodecXmlReader, FailurePrecedence) {
	/**
	 * @brief Разметка, нарушающая предел объёма события вместе с построением
	 *
	 */
	const char * items[] = {
		"<b xmlns=\"urn:default\" p=\"value\" q=\"value\"U</b>",
		"<!DOCTYPE a [\n<!ENTITY plain \"значение\">\n<!ENTITY markup \"<b>",
		"<a><!-- примечание, оборванное концом текста",
		"<a><?цель данные, оборванные концом текста",
		"<a><![CDATA[раздел, оборванный концом текста",
		"<a привет=\"значение\" второй=\"значение\" третий=\"значение\""
	};
	/**
	 * Выполняем перебор всех разновидностей разметки
	 */
	for(const char * item : items){
		// Настройки разбора с заданным пределом объёма события
		xml::reader_t::settings_t settings;
		// Выполняем установку предела объёма одного события
		settings.maxEvent = 24;
		// Выполняем склеивание подряд идущих кусков содержимого
		settings.mergeText = true;
		// Код ошибки разбора текста, поданного целиком
		xml::error_t first = xml::error_t::NONE;
		// Выполняем разбор текста разметки, поданного целиком
		const string whole = run(item, ::strlen(item), first, settings);
		// Выполняем проверку того, что предел объёма события удержан
		ASSERT_EQ(first, xml::error_t::OVERFLOW_LIMIT) << item;
		/**
		 * Выполняем перебор размеров куска подачи текста разметки
		 */
		for(size_t step = 1; step < 8; step++){
			// Код ошибки разбора текста, поданного кусками
			xml::error_t second = xml::error_t::NONE;
			// Выполняем разбор того же текста разметки, поданного кусками
			const string chunked = run(item, step, second, settings);
			// Выполняем проверку совпадения кода отказа при обоих способах подачи
			ASSERT_EQ(first, second) << item << " шаг " << step;
			// Выполняем проверку совпадения выдачи разбора при обоих способах подачи
			ASSERT_EQ(whole, chunked) << item << " шаг " << step;
		}
	}
}
/**
 * @brief Проверка учёта обращения с пробельным содержимым без пространств имён
 *
 * @details Атрибут `xml:space` отведён самим договором о разметке, а не договором о
 *          пространствах имён: он действует и там, где разрешение префиксов
 *          выключено, - имя его в этом случае не разделено и узнаётся целиком
 *
 */
TEST(CodecXmlReader, SpaceWithoutNamespaces) {
	/**
	 * Выполняем перебор обоих состояний разрешения префиксов
	 */
	for(const bool namespaces : {true, false}){
		// Настройки разбора текста разметки
		xml::reader_t::settings_t settings;
		// Выполняем установку разрешения префиксов по договору о пространствах имён
		settings.namespaces = namespaces;
		// Объект потокового чтения текста разметки
		xml::reader_t reader;
		// Выполняем установку настроек разбора текста разметки
		reader.settings(settings);
		// Выполняем передачу текста разметки целиком
		ASSERT_TRUE(reader.feed("<r xml:space=\"preserve\"><a/><b xml:space=\"default\"><c/></b></r>"));
		// Количество узлов, обращение с пробелами которых проверено
		uint32_t checked = 0;
		/**
		 * Выполняем перебор всех событий разбора
		 */
		while(reader.next()){
			/**
			 * Если получено начало узла разметки
			 */
			if(reader.event() == xml::event_t::ELEMENT_OPEN){
				// Получаем местное имя открытого узла разметки
				const string_view local = reader.name().local;
				/**
				 * Если открыт узел, задающий сохранение пробельного содержимого
				 */
				if((local.compare("r") == 0) || (local.compare("a") == 0)){
					// Выполняем проверку установленного сохранения пробельного содержимого
					ASSERT_EQ(reader.space(), xml::space_t::PRESERVE) << local << " " << namespaces;
					// Выполняем подсчёт проверенных узлов разметки
					checked++;
				/**
				 * Если открыт узел, отменяющий сохранение пробельного содержимого
				 */
				} else if((local.compare("b") == 0) || (local.compare("c") == 0)){
					// Выполняем проверку отменённого сохранения пробельного содержимого
					ASSERT_EQ(reader.space(), xml::space_t::DEFAULT) << local << " " << namespaces;
					// Выполняем подсчёт проверенных узлов разметки
					checked++;
				}
			}
		}
		// Выполняем проверку того, что разбор завершён удачно
		ASSERT_EQ(reader.state(), xml::state_t::FINISHED) << namespaces;
		// Выполняем проверку того, что все узлы разметки встречены разбором
		ASSERT_EQ(checked, 4u) << namespaces;
	}
}
/**
 * @brief Проверка выдачи содержимого частями при выключенной склейке
 *
 * @details Склейка выключена у разбора по умолчанию, и содержимое выдаётся частями по
 * мере прихода кусков исходного текста. Нарезка при этом обязана оставаться незаметной:
 * содержимое, собранное из частей, обязано совпасть с выданным целиком, а место
 * события и его вид - не зависеть от того, где легла граница куска
 *
 */
TEST(CodecXmlReader, PartialContent) {
	/**
	 * @brief Метод сбора содержимого и мест событий
	 *
	 * @param text  разбираемый текст разметки
	 * @param chunk размер куска исходного текста, ноль передаёт текст целиком
	 * @return      собранное содержимое с местами начала событий
	 *
	 */
	auto parse = [](const string & text, const size_t chunk) noexcept -> string {
		// Собираемые настройки разбора текста разметки
		xml::reader_t::settings_t settings;
		// Выполняем выключение склейки подряд идущего содержимого
		settings.mergeText = false;
		// Объект потокового чтения текста разметки
		xml::reader_t reader(settings);
		// Собираемое содержимое с местами начала событий
		string result;
		// Вид события, содержимое которого собирается
		uint32_t kind = 0;
		// Получаем размер подаваемого куска исходного текста
		const size_t size = (chunk > 0 ? chunk : text.length());
		// Смещение начала очередного куска подачи
		size_t offset = 0;
		/**
		 * Выполняем подачу текста разметки кусками
		 */
		do {
			// Получаем размер подаваемого куска текста разметки
			const size_t length = ((text.length() - offset) < size ? (text.length() - offset) : size);
			// Если подача куска текста разметки не удалась, разбор прекращается
			if(!reader.feed(text.data() + offset, length, ((offset + length) >= text.length()))) break;
			// Выполняем смещение начала очередного куска подачи
			offset += length;
			/**
			 * Выполняем чтение выданных разбором событий
			 */
			while(reader.next()){
				// Получаем вид очередного события разбора
				const uint32_t event = static_cast <uint32_t> (reader.event());
				/**
				 * Если содержимое продолжает событие того же вида
				 *
				 * @note Подряд идущее содержимое сводится в одно: без склейки нарезка на
				 *       события задаётся размером куска, а сличению подлежит содержимое
				 */
				if(event == kind){
					// Выполняем присоединение содержимого к собранному
					result.append(reader.text());
					// Выполняем переход к следующему событию разбора
					continue;
				}
				// Запоминаем вид события, содержимое которого собирается
				kind = (((event == static_cast <uint32_t> (xml::event_t::TEXT)) ||
				         (event == static_cast <uint32_t> (xml::event_t::SPACE)) ||
				         (event == static_cast <uint32_t> (xml::event_t::CDATA))) ? event : 0);
				// Выполняем добавление вида события к собранному
				result.append(" ").append(::std::to_string(event)).append("@");
				// Выполняем добавление места начала события к собранному
				result.append(::std::to_string(reader.location().line)).append(":");
				/**
				 * Выполняем добавление положения события в строке к собранному
				 *
				 * @note Содержимое закрывающего знака не получает: части одного содержимого
				 *       дописываются к нему по мере прихода кусков, и знак этот оказался бы
				 *       посреди собранного
				 */
				result.append(::std::to_string(reader.location().column)).append("[");
				// Выполняем добавление содержимого события к собранному
				result.append(reader.text());
			}
			// Если разбор прекращён ошибкой, чтение прекращается
			if(reader.state() == xml::state_t::FAILED) break;
		// Выполняем подачу до исчерпания текста разметки
		} while(offset < text.length());
		// Выполняем добавление итога разбора к собранному
		result.append(" итог=").append(::std::to_string(static_cast <uint32_t> (reader.state())));
		// Выводим собранное содержимое с местами начала событий
		return result;
	};
	/**
	 * Разбираемые тексты разметки
	 */
	const vector <string> texts = {
		// Обычное содержимое с разделом дословного текста
		"<a>содержимое<![CDATA[дословно]]>ещё</a>",
		// Раздел дословного текста, оканчивающийся на границе куска
		"<a><![CDATA[abc]]></a>",
		// Раздел дословного текста, пустой сам по себе
		"<a><![CDATA[]]></a>",
		// Содержимое, начатое ссылками на сущности с пустыми значениями
		"<!DOCTYPE a [<!ENTITY e \"\">]><a>&e;&e;&lt;текст</a>",
		// Содержимое с концами строк всех трёх видов
		"<a>a\r\nb\rc\nd</a>"
	};
	/**
	 * Выполняем перебор всех разбираемых текстов разметки
	 */
	for(const string & text : texts){
		// Получаем выдачу подачи текста разметки целиком
		const string expected = parse(text, 0);
		/**
		 * Выполняем перебор размеров куска исходного текста
		 */
		for(size_t chunk = 1; chunk <= 9; chunk++)
			// Выполняем проверку того, что нарезка выдачи не изменила
			ASSERT_EQ(parse(text, chunk), expected) << "текст=" << text << " кусок=" << chunk;
	}
}
/**
 * @brief Проверка старшинства отказов в содержимом узла
 *
 * @details Отказ выдаётся по тому поводу, что встретился в тексте раньше, а не по тому,
 * чья проверка выполняется первой: иначе тот же текст, пришедший кусками, отвергался бы
 * по другому поводу - приведение доходит до своего отказа, не увидев ещё запрещённой
 * последовательности
 *
 */
TEST(CodecXmlReader, ContentFailurePrecedence) {
	/**
	 * @brief Метод получения повода отказа разбора
	 *
	 * @param text разбираемый текст разметки
	 * @return     код ошибки прекращённого разбора
	 *
	 */
	auto refuse = [](const string & text) noexcept -> xml::error_t {
		// Объект потокового чтения текста разметки
		xml::reader_t reader;
		// Выполняем подачу текста разметки целиком
		reader.feed(text);
		// Выполняем чтение всех выданных разбором событий
		while(reader.next());
		// Выводим код ошибки прекращённого разбора
		return reader.error();
	};
	// Выполняем проверку отказа по ошибочной ссылке, стоящей прежде запрещённой записи
	ASSERT_EQ(refuse("<a>&#0;текст]]>хвост</a>"), xml::error_t::INVALID_CHAR_REFERENCE);
	// Выполняем проверку отказа по запрещённой записи, стоящей прежде ошибочной ссылки
	ASSERT_EQ(refuse("<a>текст]]>хвост&#0;</a>"), xml::error_t::INVALID_CHARACTER);
}
/**
 * @brief Проверка подстановки предопределённых ссылок на кавычки
 *
 * @details Договор предопределяет пять сущностей, и две из них - «quot» и «apos» -
 *          набором не проверялись вовсе: карта покрытия показала обе ветви
 *          подстановки нетронутыми. Кавычка коварна тем, что совпадает с
 *          отделителем значения атрибута, и подстановка её обязана вестись после
 *          отыскания отделителя, а не до
 *
 */
TEST(CodecXmlReader, QuoteEntities) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	// Выполняем проверку подстановки кавычек в содержимом узла
	ASSERT_EQ(::both("<a>&quot;&apos;</a>", error), "<a>\"'</a>[end]");
	// Выполняем проверку подстановки кавычек в значении атрибута под двойным отделителем
	ASSERT_EQ(::both("<a x=\"&quot;&apos;\"/>", error), "<a x=\"'></a>[end]");
	// Выполняем проверку подстановки кавычек в значении атрибута под одинарным отделителем
	ASSERT_EQ(::both("<a x='&quot;&apos;'/>", error), "<a x=\"'></a>[end]");
	// Выполняем проверку подстановки кавычек из значения объявленной сущности
	ASSERT_EQ(::both("<!DOCTYPE a [<!ENTITY e \"&quot;з&apos;\">]><a>&e;</a>", error), "[doctype a]<a>\"з'</a>[end]");
}
/**
 * @brief Проверка обрыва значения атрибута концом текста
 *
 * @details Значение, не закрытое кавычкой до конца исходного текста, набором не
 *          проверялось: карта покрытия показала выход из перебора знаков и отказ
 *          за ним нетронутыми. Отличать этот отказ от ожидания продолжения
 *          необходимо: разбор ведётся по кускам, и незакрытое значение внутри
 *          куска - это ещё не ошибка
 *
 */
TEST(CodecXmlReader, UnterminatedAttribute) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	// Выполняем разбор узла с незакрытым значением атрибута
	::both("<a x=\"1", error);
	// Выполняем проверку кода ошибки разбора
	ASSERT_EQ(error, xml::error_t::UNCLOSED_TAG);
	/**
	 * Выполняем проверку значения с угловой скобкой внутри
	 *
	 * @note Отыскание конца метки ведётся с оглядкой на кавычки, и скобка внутри
	 *       значения концом метки не считается: обрыв здесь тот же самый
	 */
	::both("<a x=\"1>", error);
	// Выполняем проверку кода ошибки разбора
	ASSERT_EQ(error, xml::error_t::UNCLOSED_TAG);
}
/**
 * @brief Проверка предела вложенности ссылок на сущности
 *
 * @details Предел этот прикрывает разбор от расхода стека на цепочке ссылок и от
 *          самоссылающейся сущности, которую договор запрещает, а текст содержать
 *          всё же может. Карта покрытия показала ветвь отказа нетронутой: цепочки
 *          такой длины набор не строил ни разу
 *
 * @note Сличаются оба края: цепочка длиною ровно в предел обязана разбираться, и
 *       лишь превышающая её - отвергаться. Проверка одного лишь отказа прошла бы и
 *       при пределе, сорвавшемся вдвое раньше
 *
 */
TEST(CodecXmlReader, EntityDepthLimit) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	/**
	 * @brief Метод построения текста с цепочкой ссылок заданной длины
	 *
	 * @param depth длина строимой цепочки ссылок
	 * @return      построенный текст разметки
	 *
	 */
	auto chain = [](const uint32_t depth) noexcept -> string {
		// Строимый текст разметки
		string result = "<!DOCTYPE a [<!ENTITY e0 \"з\">";
		// Выполняем перебор всех звеньев строимой цепочки
		for(uint32_t i = 1; i < depth; i++)
			// Выполняем добавление очередного звена цепочки
			result.append("<!ENTITY e" + std::to_string(i) + " \"&e" + std::to_string(i - 1) + ";\">");
		// Выводим построенный текст разметки
		return result.append("]><a>&e" + std::to_string(depth - 1) + ";</a>");
	};
	// Выполняем проверку разбора цепочки длиною в дозволенный предел
	ASSERT_EQ(::both(chain(xml::MAX_ENTITY_DEPTH), error), "[doctype a]<a>з</a>[end]");
	// Выполняем проверку кода ошибки разбора
	ASSERT_EQ(error, xml::error_t::NONE);
	// Выполняем разбор цепочки, предел вложенности превышающей
	::both(chain(xml::MAX_ENTITY_DEPTH + 8), error);
	// Выполняем проверку кода ошибки разбора
	ASSERT_EQ(error, xml::error_t::ENTITY_DEPTH_EXCEEDED);
	// Выполняем разбор самоссылающейся сущности
	::both("<!DOCTYPE a [<!ENTITY e \"&e;\">]><a>&e;</a>", error);
	// Выполняем проверку отказа разбора самоссылающейся сущности
	ASSERT_NE(error, xml::error_t::NONE);
}
/**
 * @brief Проверка значения по умолчанию у атрибута с приставкой
 *
 * @details Описание типа документа договора о пространствах имён не знает, и
 *          разделитель приставки является там обычным знаком имени. Разделение
 *          ведётся лишь ради сличения объявленного по умолчанию с записанным в
 *          тексте: сличать иначе, чем разбор разделяет имена, нельзя. Карта покрытия
 *          показала ветвь разделения нетронутой - имена с приставкой в объявлениях
 *          набор не встречал ни разу
 *
 */
TEST(CodecXmlReader, DefaultPrefixedAttribute) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	// Выполняем проверку подстановки значения по умолчанию у атрибута с приставкой
	ASSERT_EQ(::both("<!DOCTYPE a [<!ATTLIST a p:x CDATA \"1\">]><a xmlns:p='urn:p'/>", error),
	          "[doctype a]<a x{urn:p}=1></a>[end]");
	/**
	 * Выполняем проверку записанного в тексте значения того же атрибута
	 *
	 * @note Записанное значение обязано вытеснить объявленное по умолчанию, а не
	 *       встать рядом с ним: сличение имён ведётся по приставке и местному имени
	 *       порознь
	 */
	ASSERT_EQ(::both("<!DOCTYPE a [<!ATTLIST a p:x CDATA \"1\">]><a xmlns:p='urn:p' p:x='2'/>", error),
	          "[doctype a]<a x{urn:p}=2></a>[end]");
}
/**
 * @brief Проверка имён из дальних областей Юникода
 *
 * @details Договор дозволяет именам знаки далеко за пределами латиницы, и проверка
 *          дозволенности ведётся перебором областей. Набор проверял лишь ближние из
 *          них: карта покрытия показала нетронутыми области иероглифов, знаков
 *          нулевой ширины и дополнительных плоскостей, а из знаков, дозволенных лишь
 *          внутри имени, - сочетающиеся надстрочные
 *
 * @note Знаки нулевой ширины дозволены договором именно как начало имени, а
 *       сочетающиеся надстрочные - лишь внутри его, и разница эта проверяется парой
 *       записей: имя, начатое сочетающимся знаком, обязано быть отвергнуто
 *
 */
TEST(CodecXmlReader, WideNames) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	// Выполняем проверку разбора имени из области иероглифов
	ASSERT_EQ(::both("<漢字>з</漢字>", error), "<漢字>з</漢字>[end]");
	// Выполняем проверку разбора имени из дополнительной плоскости Юникода
	ASSERT_EQ(::both("<\xF0\x90\x80\x80/>", error), "<\xF0\x90\x80\x80></\xF0\x90\x80\x80>[end]");
	// Выполняем проверку разбора имени, начатого знаком нулевой ширины
	ASSERT_EQ(::both("<\xE2\x80\x8C/>", error), "<\xE2\x80\x8C></\xE2\x80\x8C>[end]");
	// Выполняем проверку разбора имени с сочетающимся надстрочным знаком внутри
	ASSERT_EQ(::both("<a\xCC\x81/>", error), "<a\xCC\x81></a\xCC\x81>[end]");
	// Выполняем разбор имени, начатого сочетающимся надстрочным знаком
	::both("<\xCC\x81/>", error);
	// Выполняем проверку отказа разбора такого имени
	ASSERT_EQ(error, xml::error_t::INVALID_NAME);
}
/**
 * @brief Проверка примечаний и указаний обработчику внутри внутреннего подмножества
 *
 * @details Договор дозволяет им стоять между объявлениями описания типа документа, и
 *          разбираются они там СВОИМ кодом, а не тем, каким разбираются в самом
 *          тексте. Карта покрытия показала эти ветви нетронутыми, а значит, законная
 *          запись описания могла быть отвергнута, и никто бы того не заметил
 *
 * @note Событий они не порождают: описание типа документа выдаётся разбору одним
 *       событием целиком. Оттого проверяется не выдача, а то, что разбор проходит и
 *       объявления, стоящие ЗА примечанием, действуют
 *
 */
TEST(CodecXmlReader, SubsetCommentsAndProcessing) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	// Выполняем проверку разбора примечания внутри внутреннего подмножества
	ASSERT_EQ(::both("<!DOCTYPE a [<!-- примечание --><!ENTITY e \"з\">]><a>&e;</a>", error),
	          "[doctype a]<a>з</a>[end]");
	// Выполняем проверку разбора указания обработчику внутри внутреннего подмножества
	ASSERT_EQ(::both("<!DOCTYPE a [<?pi тело?><!ENTITY e \"з\">]><a>&e;</a>", error),
	          "[doctype a]<a>з</a>[end]");
	// Выполняем проверку разбора указания обработчику без содержимого
	ASSERT_EQ(::both("<!DOCTYPE a [<?pi?><!ENTITY e \"з\">]><a>&e;</a>", error),
	          "[doctype a]<a>з</a>[end]");
	// Выполняем проверку разбора примечания, стоящего последним объявлением
	ASSERT_EQ(::both("<!DOCTYPE a [<!ENTITY e \"з\"><!-- хвост -->]><a>&e;</a>", error),
	          "[doctype a]<a>з</a>[end]");
	// Выполняем проверку отсутствия отказа разбора
	ASSERT_EQ(error, xml::error_t::NONE);
}
/**
 * @brief Проверка признака самостоятельности документа
 *
 * @details Признак этот заявляет, обходится ли документ без внешнего подмножества, и
 *          снимается он потребителем отдельно от прочего объявления. Карта покрытия
 *          показала ветвь значения «no» нетронутой: набор проверял лишь «yes», а
 *          значения два, и второе - умолчание договора
 *
 * @note Сличается и отсутствие объявления вовсе: договор велит считать документ
 *       несамостоятельным по умолчанию, и признак «нет объявления» с признаком
 *       «объявлено no» смешивать нельзя - первое означает умолчание, второе заявление
 *
 */
TEST(CodecXmlReader, Standalone) {
	/**
	 * @brief Метод снятия признака самостоятельности с разобранного текста
	 *
	 * @param text разбираемый текст разметки
	 * @return     снятый признак самостоятельности документа
	 *
	 */
	auto taken = [](const string & text) noexcept -> xml::standalone_t {
		// Объект потокового чтения разметки
		xml::reader_t reader;
		// Выполняем подачу текста разметки целиком
		reader.feed(text.data(), text.size(), true);
		// Выполняем перебор всех событий разбора
		while(reader.next());
		// Выводим снятый признак самостоятельности документа
		return reader.standalone();
	};
	// Выполняем проверку снятия заявленной самостоятельности документа
	ASSERT_EQ(taken("<?xml version='1.0' standalone='yes'?><a/>"), xml::standalone_t::YES);
	// Выполняем проверку снятия заявленной несамостоятельности документа
	ASSERT_EQ(taken("<?xml version='1.0' standalone='no'?><a/>"), xml::standalone_t::NO);
	// Выполняем проверку того, что без объявления признак остаётся неопределённым
	ASSERT_EQ(taken("<?xml version='1.0'?><a/>"), xml::standalone_t::NONE);
	// Выполняем проверку того, что без объявления разметки признак тоже неопределён
	ASSERT_EQ(taken("<a/>"), xml::standalone_t::NONE);
	// Выполняем проверку снятия признака при объявленной кодировке
	ASSERT_EQ(taken("<?xml version='1.0' encoding='UTF-8' standalone='no'?><a/>"), xml::standalone_t::NO);
}
/**
 * @brief Проверка объявлений описания типа документа
 *
 * @details Описание типа документа несёт пять видов объявлений, и всякое из них
 *          разбирается своим кодом: обозначение, сущность разбираемая и неразбираемая,
 *          сущность параметрическая, строение узла и перечень атрибутов. Карта
 *          покрытия показала четыре десятка ветвей отказа нетронутыми
 *
 * @note Сличаются ОБЕ стороны, и сторона принимающая здесь важнее отвергающей: кодек,
 *       отвергающий законное описание, ошибается молча - потребитель решит, что негоден
 *       его документ, а не разбор. Отказы же видны сразу
 *
 * @warning Внешнее подмножество разбор не забирает и не разворачивает: объявления его
 *          лежат по обозначению вовне, а сеть кодеку недоступна. Проверяется здесь
 *          построение самого объявления, а не то, что за ним стоит
 *
 */
TEST(CodecXmlReader, DoctypeDeclarations) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	/**
	 * Выполняем перебор всех законных объявлений описания типа документа
	 */
	for(auto & text : vector <string> {
		// Объявления обозначения внешними указателями обоих видов
		"<!DOCTYPE a [<!NOTATION n SYSTEM 'u'>]><a/>",
		"<!DOCTYPE a [<!NOTATION n PUBLIC 'p' 'u'>]><a/>",
		// Объявления сущности внешней, разбираемой и неразбираемой
		"<!DOCTYPE a [<!ENTITY e SYSTEM 'u'>]><a/>",
		"<!DOCTYPE a [<!ENTITY e SYSTEM 'u' NDATA n>]><a/>",
		// Объявление сущности параметрической
		"<!DOCTYPE a [<!ENTITY % e 'з'>]><a/>",
		// Объявления строения узла всех видов
		"<!DOCTYPE a [<!ELEMENT a (b,c)>]><a/>",
		"<!DOCTYPE a [<!ELEMENT a (b|c)*>]><a/>",
		"<!DOCTYPE a [<!ELEMENT a (#PCDATA)>]><a/>",
		"<!DOCTYPE a [<!ELEMENT a (#PCDATA|b)*>]><a/>",
		"<!DOCTYPE a [<!ELEMENT a ANY>]><a/>",
		// Объявления перечня атрибутов всех видов
		"<!DOCTYPE a [<!ATTLIST a x (1|2) '1'>]><a/>",
		"<!DOCTYPE a [<!ATTLIST a x NOTATION (n) #IMPLIED>]><a/>",
		"<!DOCTYPE a [<!ATTLIST a x ID #REQUIRED>]><a/>",
		"<!DOCTYPE a [<!ATTLIST a x CDATA #FIXED 'z'>]><a/>",
		// Указатели на внешнее подмножество обоих видов
		"<!DOCTYPE a SYSTEM 'u'><a/>",
		"<!DOCTYPE a PUBLIC 'p' 'u'><a/>"
	}){
		// Выполняем разбор законного описания типа документа
		::both(text, error);
		// Выполняем проверку того, что законное описание принято
		ASSERT_EQ(error, xml::error_t::NONE) << "запись [" << text << "]";
	}
	/**
	 * Выполняем перебор всех негодных объявлений описания типа документа
	 */
	for(auto & text : vector <string> {
		// Объявление обозначения без указателя, с неполным указателем и без конца
		"<!DOCTYPE a [<!NOTATION n>]><a/>",
		"<!DOCTYPE a [<!NOTATION n SYSTEM>]><a/>",
		"<!DOCTYPE a [<!NOTATION n SYSTEM 'u'>",
		/**
		 * Объявление обозначения именем с разделителем приставки
		 *
		 * @note Договор о пространствах имён к описанию типа документа не применяется, а
		 *       вот обозначению имя с разделителем запрещает он же: обозначения ходят
		 *       через границу пространств имён и приставки нести не вправе
		 */
		"<!DOCTYPE a [<!NOTATION p:n SYSTEM 'u'>]><a/>",
		// Объявление обозначения с лишним словом за указателем
		"<!DOCTYPE a [<!NOTATION n SYSTEM 'u' лишнее>]><a/>",
		// Объявление неразбираемой сущности без обозначения за словом NDATA
		"<!DOCTYPE a [<!ENTITY e SYSTEM 'u' NDATA>]><a/>",
		// Объявление параметрической сущности без значения
		"<!DOCTYPE a [<!ENTITY % e>]><a/>",
		// Объявление строения узла с пустым звеном перечня
		"<!DOCTYPE a [<!ELEMENT a (b,)>]><a/>",
		// Объявление перечня атрибутов с перечислением без значения по умолчанию
		"<!DOCTYPE a [<!ATTLIST a x (1|2)>]><a/>",
		// Объявление закреплённого значения без самого значения
		"<!DOCTYPE a [<!ATTLIST a x CDATA #FIXED>]><a/>",
		// Указатель на внешнее подмножество без обозначения при заявленном виде
		"<!DOCTYPE a PUBLIC 'p'><a/>",
		"<!DOCTYPE a SYSTEM><a/>"
	}){
		// Выполняем разбор негодного описания типа документа
		::both(text, error);
		// Выполняем проверку того, что негодное описание отвергнуто
		ASSERT_EQ(error, xml::error_t::INVALID_DOCTYPE) << "запись [" << text << "]";
	}
}
/**
 * @brief Проверка подстановки сущности, содержащей разметку
 *
 * @details Сущность, чьё значение несёт разметку, подставляется НЕ в собираемое
 *          содержимое, а прямо в разбираемый текст: подставленное разбирается наравне
 *          с записанным, иначе узлы из сущности деревом не стали бы. Дорога эта своя,
 *          и ограды у неё свои - карта покрытия показала все три нетронутыми, тогда
 *          как те же ограды на дороге текстовой закреплены давно
 *
 * @note Ограда рекурсии здесь опирается на признак «подстановка идёт в текущий миг», а
 *       не на глубину: сущность, ссылающаяся на себя через разметку, глубины не
 *       набирает вовсе, покуда не съест всю память
 *
 * @warning Предел общего объёма подстановки считается на ОБЕИХ дорогах разом, и
 *          сличается он здесь вложением: сущность внутри сущности набирает объём
 *          дважды, и предел обязан ловить сумму, а не наибольшее
 *
 */
TEST(CodecXmlReader, MarkupEntityInjection) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	// Выполняем проверку подстановки сущности с разметкой
	ASSERT_EQ(::both("<!DOCTYPE a [<!ENTITY e \"<b/>\">]><a>&e;&e;</a>", error),
	          "[doctype a]<a><b></b><b></b></a>[end]");
	// Выполняем проверку подстановки сущности, вложенной в сущность
	ASSERT_EQ(::both("<!DOCTYPE a [<!ENTITY x \"<b/>\"><!ENTITY e \"<c>&x;</c>\">]><a>&e;</a>", error),
	          "[doctype a]<a><c><b></b></c></a>[end]");
	// Выполняем проверку отсутствия отказа разбора
	ASSERT_EQ(error, xml::error_t::NONE);
	// Выполняем разбор сущности, ссылающейся на себя через разметку
	::both("<!DOCTYPE a [<!ENTITY e \"<b>&e;</b>\">]><a>&e;</a>", error);
	// Выполняем проверку отказа по рекурсии подстановки
	ASSERT_EQ(error, xml::error_t::RECURSIVE_ENTITY);
	// Настройки разбора с укороченным пределом объёма подстановки
	xml::reader_t::settings_t narrow;
	// Ограничиваем общий объём подстановки сущностей
	narrow.maxExpansion = 8;
	// Выполняем разбор текста, предел объёма подстановки превысившего
	::run("<!DOCTYPE a [<!ENTITY e \"<b/>\">]><a>&e;&e;&e;</a>", string::npos, error, narrow);
	// Выполняем проверку отказа по объёму подстановки
	ASSERT_EQ(error, xml::error_t::ENTITY_LIMIT_EXCEEDED);
	// Ограничиваем общий объём подстановки сущностей вложением
	narrow.maxExpansion = 6;
	// Выполняем разбор текста с сущностью, вложенной в сущность
	::run("<!DOCTYPE a [<!ENTITY x \"<b/>\"><!ENTITY e \"<c>&x;</c>\">]><a>&e;</a>", string::npos, error, narrow);
	// Выполняем проверку того, что предел ловит сумму вложенных подстановок
	ASSERT_EQ(error, xml::error_t::ENTITY_LIMIT_EXCEEDED);
}
/**
 * @brief Проверка разбора объявлений описания типа документа
 *
 * @note Разбор подмножества описания типа идёт своим набором способов, стоящим
 *       особняком от разбора самой разметки: каждое объявление разбирается своей
 *       ветвью, и негодные виды их обязаны отвергаться порознь, а не общим прикрытием
 * @warning Проверяются ОБЕ половины: законные объявления обязаны приниматься.
 *          Набор из одних лишь негодных прошёл бы и у разбора, отвергающего описание
 *          типа целиком
 */
TEST(CodecXmlReader, DoctypeDeclarationForms) {
	/**
	 * Выполняем перебор всех негодных объявлений описания типа документа
	 */
	for(auto & text : vector <string> {
		// Строение узла: перечень, оборванный и построенный ошибочно
		"<!DOCTYPE a [<!ELEMENT a (b,>]><a/>",
		"<!DOCTYPE a [<!ELEMENT a (b c)>]><a/>",
		"<!DOCTYPE a [<!ELEMENT a (>]><a/>",
		// Строение узла: содержимое смешанное, построенное ошибочно
		"<!DOCTYPE a [<!ELEMENT a (#PCDATA|)*>]><a/>",
		"<!DOCTYPE a [<!ELEMENT a (#PCDATA|b)>]><a/>",
		"<!DOCTYPE a [<!ELEMENT a (#PCDATA,b)*>]><a/>",
		// Строение узла: объявление без строения, без имени и без завершения
		"<!DOCTYPE a [<!ELEMENT a>]><a/>",
		"<!DOCTYPE a [<!ELEMENT (b)>]><a/>",
		"<!DOCTYPE a [<!ELEMENT a (b)]><a/>",
		// Перечень свойств: свойство без вида и без умолчания
		"<!DOCTYPE a [<!ATTLIST a x>]><a/>",
		"<!DOCTYPE a [<!ATTLIST a x CDATA>]><a/>",
		// Перечень свойств: перечисление, построенное ошибочно
		"<!DOCTYPE a [<!ATTLIST a x (1|)>]><a/>",
		"<!DOCTYPE a [<!ATTLIST a x (1 2) '1'>]><a/>",
		// Перечень свойств: обозначение без перечисления и умолчание неизвестное
		"<!DOCTYPE a [<!ATTLIST a x NOTATION n #IMPLIED>]><a/>",
		"<!DOCTYPE a [<!ATTLIST a x CDATA #FIXED>]><a/>",
		"<!DOCTYPE a [<!ATTLIST a x CDATA #WRONG>]><a/>",
		// Обозначение: без имени, без указателя и с указателем оборванным
		"<!DOCTYPE a [<!NOTATION>]><a/>",
		"<!DOCTYPE a [<!NOTATION n>]><a/>",
		"<!DOCTYPE a [<!NOTATION n SYSTEM>]><a/>",
		"<!DOCTYPE a [<!NOTATION n SYSTEM 'u'']><a/>",
		/**
		 * Обозначение с именем составным
		 *
		 * @note Имя обозначения двоеточия нести не может при разборе с пространствами
		 *       имён: двоеточие там отделяет префикс, а префикса у обозначения нет
		 */
		"<!DOCTYPE a [<!NOTATION n:m SYSTEM 'u'>]><a/>",
		// Сущность: без имени, без содержимого и параметрическая без имени
		"<!DOCTYPE a [<!ENTITY>]><a/>",
		"<!DOCTYPE a [<!ENTITY e>]><a/>",
		"<!DOCTYPE a [<!ENTITY % >]><a/>",
		// Сущность внешняя: без указателя, без обозначения и с указателем неполным
		"<!DOCTYPE a [<!ENTITY e SYSTEM>]><a/>",
		"<!DOCTYPE a [<!ENTITY e SYSTEM 'u' NDATA>]><a/>",
		"<!DOCTYPE a [<!ENTITY e PUBLIC 'p'>]><a/>",
		// Сущность: содержимое, оборванное и несущее негодную ссылку
		"<!DOCTYPE a [<!ENTITY e 'з'']><a/>",
		"<!DOCTYPE a [<!ENTITY e '&x'>]><a/>",
		"<!DOCTYPE a [<!ENTITY e '&#xZZ;'>]><a/>",
		// Примечание и обработка, оборванные внутри подмножества
		"<!DOCTYPE a [<!-- примечание ]><a/>",
		"<!DOCTYPE a [<?pi]><a/>",
		// Объявление неизвестного вида
		"<!DOCTYPE a [<!WRONG>]><a/>",
		// Ссылка на сущность параметрическую, оборванная и без имени
		"<!DOCTYPE a [%e]><a/>",
		"<!DOCTYPE a [%;]><a/>",
		// Внешний указатель описания типа без самого указателя
		"<!DOCTYPE a SYSTEM>]><a/>",
	}){
		// Код ошибки разбора
		xml::error_t error = xml::error_t::NONE;
		// Выполняем разбор текста разметки
		::run(text, 4096, error);
		// Выполняем проверку отклонения негодного объявления
		ASSERT_NE(error, xml::error_t::NONE) << text;
	}
	/**
	 * Выполняем перебор всех законных объявлений описания типа документа
	 */
	for(auto & text : vector <string> {
		// Строение узла: повторители всех видов и вложенные перечни
		"<!DOCTYPE a [<!ELEMENT a (b|c)+>]><a/>",
		"<!DOCTYPE a [<!ELEMENT a (b?,c*)+>]><a/>",
		"<!DOCTYPE a [<!ELEMENT a ((b,c)|d)>]><a/>",
		"<!DOCTYPE a [<!ELEMENT a EMPTY>]><a/>",
		/**
		 * Перечень свойств без единого свойства
		 *
		 * @note Пустой перечень стандартом дозволен: объявление строится из имени
		 *       узла и НУЛЯ ЛИБО БОЛЕЕ описаний свойств
		 */
		"<!DOCTYPE a [<!ATTLIST a>]><a/>",
		// Перечень свойств из нескольких описаний подряд
		"<!DOCTYPE a [<!ATTLIST a x IDREFS #IMPLIED y ENTITY #IMPLIED>]><a/>",
		// Объявления сущности параметрической и обычной подряд
		"<!DOCTYPE a [<!ENTITY % p 'v'> <!ENTITY e 'x'>]><a/>",
	}){
		// Код ошибки разбора
		xml::error_t error = xml::error_t::NONE;
		// Выполняем разбор текста разметки
		::run(text, 4096, error);
		// Выполняем проверку принятия законного объявления
		ASSERT_EQ(error, xml::error_t::NONE) << text;
	}
}
/**
 * @brief Проверка отказов разбора по видам построения
 *
 * @note Собраны отказы, до каких набор не добирался: отведённые договором имена
 *       указаний обработчику, негодные названия кодировок, отведённые пространства
 *       имён, пределы настроек разбора
 * @warning Каждый вид отказа сличается СВОИМ кодом, а не одним лишь признаком отказа:
 *          отказ с чужим кодом увёл бы потребителя чинить не то место
 */
TEST(CodecXmlReader, RefusalCodes) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	/**
	 * Отведённые договором имена указаний обработчику
	 *
	 * @note Имя «xml» отведено объявлению разметки, и указанием обработчику быть
	 *       не вправе. Имена же, теми знаками лишь НАЧАТЫЕ, отвергаются только в
	 *       самом начале текста: там стоять положено объявлению, и указание такое
	 *       молча пропустило бы объявление, записанное без пробела
	 */
	::run("<a/><?xml version=\"1.0\"?>", 4096, error);
	// Выполняем проверку кода ошибки разбора
	ASSERT_EQ(error, xml::error_t::RESERVED_PROCESSING);
	// Выполняем разбор указания с именем, записанным вперемешку
	::run("<?XmL zz?><a/>", 4096, error);
	// Выполняем проверку кода ошибки разбора
	ASSERT_EQ(error, xml::error_t::RESERVED_PROCESSING);
	// Выполняем разбор указания с именем, отведённым знаками начатым
	::run("<?xmlfoo zz?><a/>", 4096, error);
	// Выполняем проверку кода ошибки разбора
	ASSERT_EQ(error, xml::error_t::RESERVED_PROCESSING);
	/**
	 * Выполняем разбор указания с именем, отведённым договором о стилевых таблицах
	 *
	 * @note Имя «xml-stylesheet» встречается повсеместно, и отвергать его значило бы
	 *       отвергать правильно построенные тексты
	 */
	::run("<?xml-stylesheet zz?><a/>", 4096, error);
	// Выполняем проверку принятия указания обработчику
	ASSERT_EQ(error, xml::error_t::NONE);
	/**
	 * Негодные названия кодировок в объявлении разметки
	 *
	 * @note Название кодировки начинается буквой и строится из букв, цифр и трёх
	 *       знаков отделения: пустое название и название с пробелом кодировкой
	 *       не являются вовсе
	 */
	for(auto & text : vector <string> {
		"<?xml version=\"1.0\" encoding=\"\"?><a/>",
		"<?xml version=\"1.0\" encoding=\"8bit\"?><a/>",
		"<?xml version=\"1.0\" encoding=\"utf 8\"?><a/>"
	}){
		// Выполняем разбор текста разметки
		::run(text, 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::UNSUPPORTED_ENCODING) << text;
	}
	/**
	 * Отведённые договором пространства имён
	 *
	 * @note Обозначение пространства имён «xml» связано с одним лишь префиксом «xml»,
	 *       а обозначение «xmlns» не связывается ни с чем вовсе: связывание их иначе
	 *       разрушило бы опознание имён по всему дереву
	 */
	::run("<a xmlns:p=\"http://www.w3.org/XML/1998/namespace\"/>", 4096, error);
	// Выполняем проверку кода ошибки разбора
	ASSERT_EQ(error, xml::error_t::RESERVED_PREFIX);
	// Выполняем разбор связывания отведённого обозначения пространства имён
	::run("<a xmlns:p=\"http://www.w3.org/2000/xmlns/\"/>", 4096, error);
	// Выполняем проверку кода ошибки разбора
	ASSERT_EQ(error, xml::error_t::INVALID_NAMESPACE);
	// Выполняем разбор связывания отведённого обозначения пространством по умолчанию
	::run("<a xmlns=\"http://www.w3.org/XML/1998/namespace\"/>", 4096, error);
	// Выполняем проверку кода ошибки разбора
	ASSERT_EQ(error, xml::error_t::INVALID_NAMESPACE);
	// Выполняем разбор имени с префиксом, ни с чем не связанным
	::run("<p:a/>", 4096, error);
	// Выполняем проверку кода ошибки разбора
	ASSERT_EQ(error, xml::error_t::UNBOUND_PREFIX);
	/**
	 * Пределы, заданные настройками разбора
	 */
	{
		// Настройки разбора с заданным пределом количества свойств
		xml::reader_t::settings_t settings;
		// Выполняем установку предела количества свойств узла
		settings.maxAttributes = 2;
		// Выполняем разбор текста разметки с превышением предела
		::run("<a x=\"1\" y=\"2\" z=\"3\"/>", 4096, error, settings);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::OVERFLOW_LIMIT);
	}
	{
		// Настройки разбора с заданным пределом объёма события
		xml::reader_t::settings_t settings;
		// Выполняем установку предела объёма одного события
		settings.maxEvent = 4;
		// Выполняем разбор текста разметки с превышением предела
		::run("<a>длинное содержимое</a>", 4096, error, settings);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::OVERFLOW_LIMIT);
	}
	/**
	 * Имя сущности, знаком решётки начатое
	 *
	 * @note Решётка отведена числовым ссылкам, и именем сущности начинать её нельзя:
	 *       объявление такое перекрыло бы разбор числовых ссылок целиком
	 */
	::run("<!DOCTYPE a [<!ENTITY #x 'v'>]><a/>", 4096, error);
	// Выполняем проверку кода ошибки разбора
	ASSERT_EQ(error, xml::error_t::INVALID_DOCTYPE);
}
