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
	xml::reader_t reader(::logger(), settings);
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
	/**
	 * Выполняем проверку обращения со ссылкой на внешнюю сущность в содержимом
	 *
	 * @details Договор велит разбору, внешних сущностей не читающему, ссылку РАСПОЗНАТЬ и
	 * пропустить, а текст принять; отказ - строгость сверх договора, и вынесен он в
	 * настройку. Проверяются обе ветви
	 *
	 * @warning Безопасности смена умолчания не касается: сущность не загружается НИ В
	 *          ОДНОМ из порядков, и разница лишь в том, отвергается ли текст. Пропуск
	 *          выдаёт содержимое без подстановки, а не содержимое запрошенного источника
	 */
	{
		// Собираемая запись событий разбора
		string result = ::run("<!DOCTYPE a [<!ENTITY e SYSTEM \"/etc/passwd\">]><a>&e;</a>", 4096, error);
		// Выполняем проверку принятия текста по умолчанию
		ASSERT_EQ(error, xml::error_t::NONE) << xml::message(error);
		// Выполняем проверку того, что содержимое источника наружу не выдано
		ASSERT_EQ(result.find("root:"), string::npos) << result;
		// Настройки потокового чтения текста разметки
		xml::reader_t::settings_t settings;
		// Устанавливаем строгость к ссылкам на внешние сущности
		settings.externals = true;
		// Выполняем разбор текста со ссылкой на внешнюю сущность
		::run("<!DOCTYPE a [<!ENTITY e SYSTEM \"/etc/passwd\">]><a>&e;</a>", 4096, error, settings);
		// Выполняем проверку отклонения ссылки на внешнюю сущность
		ASSERT_EQ(error, xml::error_t::EXTERNAL_ENTITY);
	}
	/**
	 * Выполняем проверку отклонения ссылки на внешнюю сущность в значении атрибута
	 *
	 * @note Там она запрещена самим договором, и отказ следует ВСЕГДА - независимо от
	 *       настройки строгости
	 */
	::run("<!DOCTYPE a [<!ENTITY e SYSTEM \"/etc/passwd\">]><a b=\"&e;\"/>", 4096, error);
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
		xml::reader_t reader(::logger(), settings);
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
		xml::reader_t reader(::logger(), settings);
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
		ASSERT_EQ(error, xml::error_t::TOO_MANY_ATTRIBUTES);
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
		xml::reader_t reader(::logger(), settings);
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
		xml::reader_t reader(::logger());
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
		settings.emitComments = true;
		// Объект потокового чтения текста разметки
		xml::reader_t reader(::logger(), settings);
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
		xml::reader_t reader(::logger());
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
		xml::reader_t reader(::logger(), settings);
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
		xml::reader_t reader(::logger());
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
		xml::reader_t reader(::logger());
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
		xml::reader_t reader(::logger(), settings);
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
		xml::reader_t reader(::logger(), settings);
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
			xml::reader_t reader(::logger(), settings);
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
		xml::reader_t reader(::logger(), settings);
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
		xml::reader_t reader(::logger(), settings);
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
		xml::reader_t reader(::logger());
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
		xml::reader_t reader(::logger());
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
	settings.emitComments = true;
	// Объект потокового чтения текста разметки
	xml::reader_t reader(::logger(), settings);
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
	xml::reader_t reader(::logger(), settings);
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
			xml::reader_t reader(::logger(), settings);
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
		xml::reader_t reader(::logger(), settings);
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
		xml::reader_t reader(::logger());
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
		xml::reader_t reader(::logger(), settings);
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
		xml::reader_t reader(::logger());
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
		xml::reader_t reader(::logger());
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
 * @brief Проверка обезвреживания круговой подстановки сущностей
 *
 * @note Проверка `MarkupEntityInjection` стережёт лишь ПРЯМОЕ самоуказание сущности.
 *       Круг же из двух и более сущностей прямого самоуказания не содержит нигде, и
 *       набор из одного лишь прямого случая прошёл бы и у разбора, круг не ловящего:
 *       такой разбор подставлял бы звенья круга по очереди вечно
 * @warning Отсутствие отказа здесь означает не мягкость разбора, а зависание: без
 *          обезвреживания подстановка не завершается вовсе, и проверка не возвращается.
 *          Оттого случаи взяты и в содержимом узла, и в значении свойства - пути
 *          подстановки у них разные
 */
TEST(CodecXmlReader, RecursiveEntityCycles) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	/**
	 * Выполняем перебор всех видов круговой подстановки сущностей
	 */
	for(auto & text : vector <string> {
		// Круг из двух сущностей, замкнутый через разметку
		"<!DOCTYPE a [<!ENTITY x \"<p>&y;</p>\"><!ENTITY y \"<q>&x;</q>\">]><a>&x;</a>",
		// Круг из трёх сущностей, замкнутый одними ссылками
		"<!DOCTYPE a [<!ENTITY x \"&y;\"><!ENTITY y \"&z;\"><!ENTITY z \"&x;\">]><a>&x;</a>",
		// Самоуказание, обложенное с обеих сторон текстом
		"<!DOCTYPE a [<!ENTITY e \"тут &e; тут\">]><a>&e;</a>",
		// Самоуказание в значении свойства
		"<!DOCTYPE a [<!ENTITY e \"&e;\">]><a k=\"&e;\"/>",
		// Круг из двух сущностей в значении свойства
		"<!DOCTYPE a [<!ENTITY x \"&y;\"><!ENTITY y \"&x;\">]><a k=\"&x;\"/>"
	}) {
		// Выполняем сброс кода ошибки разбора
		error = xml::error_t::NONE;
		// Выполняем разбор текста с круговой подстановкой сущностей
		::run(text, string::npos, error);
		// Выполняем проверку отказа по рекурсии подстановки
		ASSERT_EQ(error, xml::error_t::RECURSIVE_ENTITY) << text;
	}
	/**
	 * Выполняем проверку обезвреживания разрастания подстановки вложением
	 *
	 * @note Круга здесь нет ни одного, и признак идущей подстановки такое не ловит:
	 *       звенья раскрываются каждое по разу, но всего их выходит миллион
	 */
	::run("<!DOCTYPE a [<!ENTITY e0 \"aaaaaaaaaa\">"
	      "<!ENTITY e1 \"&e0;&e0;&e0;&e0;&e0;&e0;&e0;&e0;&e0;&e0;\">"
	      "<!ENTITY e2 \"&e1;&e1;&e1;&e1;&e1;&e1;&e1;&e1;&e1;&e1;\">"
	      "<!ENTITY e3 \"&e2;&e2;&e2;&e2;&e2;&e2;&e2;&e2;&e2;&e2;\">"
	      "<!ENTITY e4 \"&e3;&e3;&e3;&e3;&e3;&e3;&e3;&e3;&e3;&e3;\">"
	      "<!ENTITY e5 \"&e4;&e4;&e4;&e4;&e4;&e4;&e4;&e4;&e4;&e4;\">"
	      "<!ENTITY e6 \"&e5;&e5;&e5;&e5;&e5;&e5;&e5;&e5;&e5;&e5;\">"
	      "]><a>&e6;</a>", string::npos, error);
	// Выполняем проверку отказа по объёму подстановки
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
		ASSERT_EQ(error, xml::error_t::TOO_MANY_ATTRIBUTES);
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
/**
 * @brief Проверка отказов разбора ссылок и имён с префиксом
 *
 * @note Ссылка разбирается тремя разными местами - в содержимом, в значении свойства
 *       и в значении объявляемой сущности, - и каждое из них выносит СВОЙ код отказа.
 *       Проверяются все три: общего прикрытия у них нет
 * @warning Отказ ссылки внутри описания типа документа даёт код описания типа, а не
 *          код ссылки: место отказа для потребителя важнее его вида, ибо чинить ему
 *          объявление, а не ссылку саму по себе
 */
TEST(CodecXmlReader, ReferenceAndPrefixRefusals) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	/**
	 * Ссылки, построенные ошибочно, в содержимом и в значении свойства
	 */
	for(auto & text : vector <string> {
		// Ссылка без имени и ссылка, точкой с запятой не закрытая
		"<a>&;</a>",
		"<a>&amp</a>",
		// Ссылка с именем, цифрой начатым
		"<a>&1x;</a>",
		// Ссылка без имени в значении свойства
		"<a x=\"&;\"/>"
	}){
		// Выполняем разбор текста разметки
		::run(text, 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::INVALID_REFERENCE) << text;
	}
	/**
	 * Знак начала метки в значении свойства
	 *
	 * @note Знак этот в значении свойства стоять не вправе ни при каких условиях:
	 *       договор велит записывать его ссылкой, и разбор, его принявший, разошёлся
	 *       бы с разбором записи правильной
	 */
	::run("<a x=\"<\"/>", 4096, error);
	// Выполняем проверку кода ошибки разбора
	ASSERT_EQ(error, xml::error_t::INVALID_ATTRIBUTE);
	/**
	 * Пробельные знаки в значении свойства
	 *
	 * @note Подача строки и отступ значению свойства дозволены и обращаются пробелом:
	 *       правило это зовётся приведением значения свойства, и отвергать такое
	 *       значение нельзя
	 */
	::run("<a x=\"з\nз\"/>", 4096, error);
	// Выполняем проверку принятия значения свойства с подачей строки
	ASSERT_EQ(error, xml::error_t::NONE);
	// Выполняем разбор значения свойства с отступом
	::run("<a x=\"з\tз\"/>", 4096, error);
	// Выполняем проверку принятия значения свойства с отступом
	ASSERT_EQ(error, xml::error_t::NONE);
	/**
	 * Имена, построенные с двоеточием ошибочно
	 *
	 * @note Двоеточие отделяет префикс пространства имён, и половины имени пустыми
	 *       быть не могут ни одна: ни префикс, ни местное имя
	 */
	for(auto & text : vector <string> {"<:a/>", "<a:/>", "<a:b:c/>"}){
		// Выполняем разбор текста разметки
		::run(text, 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::INVALID_NAME) << text;
	}
	/**
	 * Ссылки внутри объявления сущности описания типа документа
	 *
	 * @note Ссылка на сущность параметрическую внутри значения объявляемой сущности
	 *       договором не допускается: во внутреннем подмножестве такая ссылка вправе
	 *       стоять лишь НА МЕСТЕ САМОГО ОБЪЯВЛЕНИЯ. Отказ этот намеренный и стандарту
	 *       отвечает, обходить его нельзя
	 */
	for(auto & text : vector <string> {
		// Ссылка без имени и ссылка, точкой с запятой не закрытая
		"<!DOCTYPE a [<!ENTITY e '&;'>]><a/>",
		"<!DOCTYPE a [<!ENTITY e '&amp'>]><a/>",
		// Ссылка на сущность параметрическую внутри значения объявляемой
		"<!DOCTYPE a [<!ENTITY % p 'v'><!ENTITY e '%p;'>]><a/>"
	}){
		// Выполняем разбор текста разметки
		::run(text, 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::INVALID_DOCTYPE) << text;
	}
	/**
	 * Глубина вложенности сущностей
	 *
	 * @note Прикрытие это оберегает разбор от разрастания подстановки: цепочка
	 *       сущностей, ссылающихся одна на другую, разворачивалась бы вглубь без
	 *       конца, и памяти на это ушло бы столько, сколько её есть
	 */
	{
		// Собираемый текст разметки с цепочкой сущностей
		string text = "<!DOCTYPE a [";
		/**
		 * Выполняем построение цепочки сущностей, ссылающихся одна на другую
		 */
		for(uint32_t i = 0; i < 40; i++)
			// Выполняем добавление очередного звена цепочки сущностей
			text.append("<!ENTITY e").append(::std::to_string(i))
			    .append(" '&e").append(::std::to_string(i + 1)).append(";'>");
		// Выполняем завершение цепочки сущностей и текста разметки
		text.append("<!ENTITY e40 'дно'>]><a>&e0;</a>");
		// Выполняем разбор текста разметки
		::run(text, 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::ENTITY_DEPTH_EXCEEDED);
	}
	/**
	 * Разбор с отключённой подстановкой сущностей
	 *
	 * @note Отключение снимает подстановку сущностей ОБЪЯВЛЕННЫХ, а пять сущностей,
	 *       отведённых договором, подставляются всегда: без них записать знаки
	 *       разметки в содержимом стало бы нечем
	 */
	{
		// Настройки разбора с отключённой подстановкой сущностей
		xml::reader_t::settings_t settings;
		// Выполняем отключение подстановки объявленных сущностей
		settings.entities = false;
		// Выполняем разбор текста с сущностью, договором отведённой
		::run("<a>&amp;</a>", 4096, error, settings);
		// Выполняем проверку принятия отведённой договором сущности
		ASSERT_EQ(error, xml::error_t::NONE);
		// Выполняем разбор текста с сущностью знака начала метки
		::run("<a>&lt;</a>", 4096, error, settings);
		// Выполняем проверку принятия отведённой договором сущности
		ASSERT_EQ(error, xml::error_t::NONE);
	}
}
/**
 * @brief Проверка приведения знаков подачи строки и отказов построения меток
 *
 * @note Собраны остатки поверхности чтения: приведение знаков подачи строки,
 *       отказы построения закрывающих меток, пересечение границы подставленной
 *       сущности и навязанная настройками кодировка
 * @warning Приведение подачи строки проверяется ОБОИМИ видами записи - одиночным
 *          возвратом каретки и парой с подачей строки: договор велит обращать оба
 *          в подачу строки, и разбор, различающий их, отдавал бы содержимое,
 *          зависящее от того, на какой машине текст записан
 */
TEST(CodecXmlReader, NewlinesAndTagRefusals) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	/**
	 * Приведение знаков подачи строки в содержимом и в дословном разделе
	 */
	for(auto & text : vector <string> {
		string("<a>x\ry</a>"),
		string("<a><![CDATA[x\r\ny]]></a>"),
		string("<a><![CDATA[x\ry]]></a>")
	}){
		// Выполняем разбор текста разметки
		::run(text, 4096, error);
		// Выполняем проверку принятия знаков подачи строки
		ASSERT_EQ(error, xml::error_t::NONE) << text;
	}
	/**
	 * Знак, разметке недопустимый, внутри дословного раздела
	 *
	 * @note Дословный раздел снимает разбор разметки, но НЕ снимает проверки знаков:
	 *       знак, договором запрещённый, недопустим и там, ибо записать его в текст
	 *       нечем ни дословно, ни ссылкой
	 */
	::run(string("<a><![CDATA[x\x01y]]></a>"), 4096, error);
	// Выполняем проверку кода ошибки разбора
	ASSERT_EQ(error, xml::error_t::INVALID_CHARACTER);
	/**
	 * Отказы построения закрывающей метки
	 */
	{
		// Выполняем разбор закрывающей метки со знаком начала метки внутри
		::run("<a></a<>", 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::INVALID_TAG);
		// Выполняем разбор закрывающей метки с лишними знаками за именем
		::run("<a></a z>", 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::INVALID_TAG);
		// Выполняем разбор закрывающей метки без имени
		::run("<a></>", 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::INVALID_NAME);
	}
	/**
	 * Указание обработчику, оборванное концом текста
	 */
	::run("<a><?pi zz</a>", 4096, error);
	// Выполняем проверку кода ошибки разбора
	ASSERT_EQ(error, xml::error_t::INVALID_PROCESSING);
	/**
	 * Объявление разметки, построенное ошибочно
	 */
	{
		// Выполняем разбор объявления без знака присвоения
		::run("<?xml version \"1.0\"?><a/>", 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::INVALID_DECLARATION);
		// Выполняем разбор объявления с неизвестным содержимым
		::run("<?xml zzz?><a/>", 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::INVALID_DECLARATION);
	}
	/**
	 * Префикс свойства, ни с чем не связанный
	 *
	 * @note Проверяется отдельно от префикса узла: связывание ищется теми же
	 *       правилами, но место обращения иное, и общего прикрытия у них нет
	 */
	::run("<a p:x=\"1\"/>", 4096, error);
	// Выполняем проверку кода ошибки разбора
	ASSERT_EQ(error, xml::error_t::UNBOUND_PREFIX);
	// Выполняем разбор дважды объявленного связывания одного префикса
	::run("<a xmlns:p=\"u\" xmlns:p=\"v\"/>", 4096, error);
	// Выполняем проверку кода ошибки разбора
	ASSERT_EQ(error, xml::error_t::DUPLICATE_ATTRIBUTE);
	/**
	 * Узел разметки, пересекающий границу подставленной сущности
	 *
	 * @note Сущность обязана подставляться целиком построенным куском: метка,
	 *       начатая внутри подстановки и закрытая снаружи, дала бы дерево, зависящее
	 *       от порядка подстановки, а не от самого текста
	 */
	::run("<!DOCTYPE a [<!ENTITY e '<b>'>]><a>&e;</b></a>", 4096, error);
	// Выполняем проверку кода ошибки разбора
	ASSERT_EQ(error, xml::error_t::ENTITY_BOUNDARY);
	/**
	 * Ссылки в значении свойства, разворачиваемые из объявленной сущности
	 */
	{
		// Выполняем разбор значения свойства со ссылкой на объявленную сущность
		::run("<!DOCTYPE a [<!ENTITY e 'v'>]><a x=\"&e;\"/>", 4096, error);
		// Выполняем проверку принятия ссылки на объявленную сущность
		ASSERT_EQ(error, xml::error_t::NONE);
		/**
		 * Выполняем разбор сущности, ссылающейся на необъявленную
		 *
		 * @note Отказ выносится при РАЗВОРАЧИВАНИИ, а не при объявлении: сущность,
		 *       ни разу не помянутая, отказа не даёт вовсе - объявить её вправе
		 *       и текст, ею не пользующийся
		 */
		::run("<!DOCTYPE a [<!ENTITY e '&nope;'>]><a x=\"&e;\"/>", 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::UNKNOWN_ENTITY);
	}
	/**
	 * Навязанная настройками кодировка исходного текста
	 */
	{
		// Настройки разбора с навязанной кодировкой исходного текста
		xml::reader_t::settings_t settings;
		// Выполняем навязывание кодировки, тексту отвечающей
		settings.encoding = xml::encoding_t::UTF8;
		// Выполняем разбор текста разметки
		::run("<a/>", 4096, error, settings);
		// Выполняем проверку принятия текста разметки
		ASSERT_EQ(error, xml::error_t::NONE);
		/**
		 * Выполняем навязывание кодировки, тексту не отвечающей
		 *
		 * @note Отказ выносится не кодировкой, а разбором: байты текста в кодировке
		 *       UTF-8 прочитанные как UTF-16 дают знаки, разметкой не являющиеся
		 */
		settings.encoding = xml::encoding_t::UTF16LE;
		// Выполняем разбор текста разметки
		::run("<a/>", 4096, error, settings);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::CONTENT_OUTSIDE_ROOT);
	}
	/**
	 * Подача текста после завершения разбора
	 *
	 * @note Разбор завершён, и дерево собрано: подача продолжения обязана отвергаться,
	 *       иначе второй текст дописался бы к первому, дав дерево с двумя корнями
	 */
	{
		// Объект потокового чтения текста разметки
		xml::reader_t reader(::logger());
		// Выполняем подачу текста разметки целиком
		ASSERT_TRUE(reader.feed("<a/>", 4, true));
		/**
		 * Выполняем перебор всех событий разбора
		 */
		while(reader.next());
		// Выполняем проверку отклонения подачи после завершения разбора
		ASSERT_FALSE(reader.feed("<b/>", 4, true));
	}
}
/**
 * @brief Проверка внешних указателей описания типа документа
 *
 * @note Описание типа вправе нести внешний указатель обоих видов, а следом за ним -
 *       ещё и внутреннее подмножество. Проверяются все сочетания: указатель без
 *       подмножества, подмножество без указателя и оба разом
 * @warning Исход сличается при ТРЁХ размерах куска подачи. Разбор описания типа
 *          при разрезанной подаче идёт своим путём, и расхождение здесь означало бы,
 *          что нарезка текста на куски меняет исход разбора
 */
TEST(CodecXmlReader, DoctypeExternal) {
	/**
	 * Способ разбора текста разметки тремя размерами куска подачи
	 *
	 * @param text разбираемый текст разметки
	 * @return     код ошибки разбора, единый для всех размеров куска
	 */
	auto chunked = [](const string & text) noexcept -> xml::error_t {
		// Код ошибки разбора при подаче текста целиком
		xml::error_t whole = xml::error_t::NONE;
		// Код ошибки разбора при подаче текста по одному байту
		xml::error_t single = xml::error_t::NONE;
		// Код ошибки разбора при подаче текста по три байта
		xml::error_t triple = xml::error_t::NONE;
		// Выполняем разбор текста разметки целиком
		::run(text, 4096, whole);
		// Выполняем разбор текста разметки по одному байту
		::run(text, 1, single);
		// Выполняем разбор текста разметки по три байта
		::run(text, 3, triple);
		/**
		 * Если исход разбора зависит от нарезки текста на куски
		 */
		if((whole != single) || (whole != triple))
			// Выводим внутреннюю ошибку признаком расхождения
			return xml::error_t::INTERNAL;
		// Выводим код ошибки разбора
		return whole;
	};
	/**
	 * Выполняем перебор всех законных внешних указателей описания типа
	 */
	for(auto & text : vector <string> {
		// Внешний указатель обоих видов без внутреннего подмножества
		"<!DOCTYPE a SYSTEM 'u'><a/>",
		"<!DOCTYPE a PUBLIC 'p' 'u'><a/>",
		// Внешний указатель обоих видов с внутренним подмножеством следом
		"<!DOCTYPE a SYSTEM 'u' [<!ELEMENT a EMPTY>]><a/>",
		"<!DOCTYPE a PUBLIC 'p' 'u' [<!ENTITY e 'v'>]><a>&e;</a>"
	}){
		// Выполняем проверку принятия законного внешнего указателя
		ASSERT_EQ(chunked(text), xml::error_t::NONE) << text;
	}
	/**
	 * Выполняем перебор всех негодных внешних указателей описания типа
	 */
	for(auto & text : vector <string> {
		/**
		 * Указатель вида PUBLIC с одним лишь общим обозначением
		 *
		 * @note Указатель этот строится ДВУМЯ записями подряд: общим обозначением
		 *       и указателем на местоположение. Одной из них недостаточно
		 */
		"<!DOCTYPE a PUBLIC 'p'><a/>",
		// Указатель обоих видов без единой записи
		"<!DOCTYPE a PUBLIC><a/>",
		"<!DOCTYPE a SYSTEM>]><a/>",
		// Указатель вида SYSTEM с лишней записью
		"<!DOCTYPE a SYSTEM 'u' 'v'><a/>"
	}){
		// Выполняем проверку отклонения негодного внешнего указателя
		ASSERT_EQ(chunked(text), xml::error_t::INVALID_DOCTYPE) << text;
	}
	/**
	 * Выполняем перебор объявлений внутреннего подмножества при разной нарезке
	 *
	 * @note Объявления эти уже проверены разбором текста целиком, здесь же
	 *       сличается лишь НЕЗАВИСИМОСТЬ исхода от нарезки текста на куски
	 */
	for(auto & text : vector <string> {
		"<!DOCTYPE a [<!ELEMENT a (b,>]><a/>",
		"<!DOCTYPE a [<!ATTLIST a x CDATA #WRONG>]><a/>",
		"<!DOCTYPE a [<!NOTATION n SYSTEM>]><a/>",
		"<!DOCTYPE a [<!ENTITY e SYSTEM 'u' NDATA>]><a/>",
		"<!DOCTYPE a [<!WRONG>]><a/>"
	}){
		// Выполняем проверку отклонения негодного объявления при всякой нарезке
		ASSERT_EQ(chunked(text), xml::error_t::INVALID_DOCTYPE) << text;
	}
	/**
	 * Выполняем перебор законных объявлений внутреннего подмножества при разной нарезке
	 */
	for(auto & text : vector <string> {
		"<!DOCTYPE a [<!ELEMENT a (b|c)+>]><a/>",
		"<!DOCTYPE a [<!ATTLIST a>]><a/>",
		"<!DOCTYPE a [<!ENTITY % p 'v'> <!ENTITY e 'x'>]><a/>"
	}){
		// Выполняем проверку принятия законного объявления при всякой нарезке
		ASSERT_EQ(chunked(text), xml::error_t::NONE) << text;
	}
}
/**
 * @brief Проверка объявлений, оборванных закрытием внутреннего подмножества
 *
 * @note Всякое объявление разбирается в пределах подмножества, и обрыв его закрывающей
 *       скобкой отличен от обрыва знаком завершения: разбору тут не хватает не знака
 *       завершения, а самого места. Ветвь эта у каждого объявления своя
 * @warning Отказ обязан выноситься, а не пропускаться молча: подмножество, принятое
 *          с оборванным объявлением, дало бы дерево, построенное по описанию типа,
 *          прочитанному наполовину
 */
TEST(CodecXmlReader, DoctypeTruncatedDeclarations) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	/**
	 * Выполняем перебор объявлений, оборванных закрытием подмножества
	 */
	for(auto & text : vector <string> {
		// Строение узла, оборванное посреди перечня и посреди смешанного содержимого
		"<!DOCTYPE a [<!ELEMENT a (b]><a/>",
		"<!DOCTYPE a [<!ELEMENT a (b,c]><a/>",
		"<!DOCTYPE a [<!ELEMENT a (#PCDATA]><a/>",
		// Строение узла, оборванное после закрытия перечня
		"<!DOCTYPE a [<!ELEMENT a (b)]><a/>",
		// Перечисление значений свойства, оборванное закрытием подмножества
		"<!DOCTYPE a [<!ATTLIST a x (1|2]><a/>",
		// Умолчание свойства, оборванное закрытием подмножества
		"<!DOCTYPE a [<!ATTLIST a x CDATA 'v]><a/>",
		/**
		 * Обозначение перечисления, за именем вида не отделённое пробельным знаком
		 *
		 * @note Договор велит отделять перечисление от вида свойства пробельным
		 *       знаком: запись слитная видом свойства уже не является
		 */
		"<!DOCTYPE a [<!ATTLIST a x NOTATION(n) #IMPLIED>]><a/>",
		// Объявления всех видов, оборванные сразу за словом вида
		"<!DOCTYPE a [<!NOTATION n]><a/>",
		"<!DOCTYPE a [<!NOTATION]><a/>",
		"<!DOCTYPE a [<!ENTITY e]><a/>",
		"<!DOCTYPE a [<!ENTITY]><a/>",
		"<!DOCTYPE a [<!ELEMENT]><a/>",
		"<!DOCTYPE a [<!ATTLIST]><a/>",
		// Примечание и указание обработчику, оборванные закрытием подмножества
		"<!DOCTYPE a [<!-- c]><a/>",
		"<!DOCTYPE a [<?pi z]><a/>"
	}){
		// Выполняем разбор текста разметки
		::run(text, 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::INVALID_DOCTYPE) << text;
	}
	/**
	 * Вложенность строения узла
	 *
	 * @note Строение узла разбирается вглубь себя самого, и глубина его ограничена
	 *       тем же пределом, что и глубина дерева: без предела запись из одних скобок
	 *       уводила бы разбор вглубь до срыва стека
	 */
	{
		// Выполняем разбор строения узла с посильной вложенностью
		::run("<!DOCTYPE a [<!ELEMENT a ((((b))))>]><a/>", 4096, error);
		// Выполняем проверку принятия строения узла
		ASSERT_EQ(error, xml::error_t::NONE);
		// Собираемый текст разметки с вложенностью за пределом
		string text = "<!DOCTYPE a [<!ELEMENT a ";
		// Собираемое закрытие вложенных перечней строения узла
		string tail = "";
		/**
		 * Выполняем построение вложенных перечней строения узла
		 */
		for(uint32_t i = 0; i < 1100; i++){
			// Выполняем добавление открытия очередного перечня
			text.push_back('(');
			// Выполняем добавление закрытия очередного перечня
			tail.push_back(')');
		}
		// Выполняем завершение строения узла и текста разметки
		text.append("b").append(tail).append(">]><a/>");
		// Выполняем разбор текста разметки
		::run(text, 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::INVALID_DOCTYPE);
	}
	/**
	 * Повторители смешанного содержимого
	 */
	{
		// Выполняем разбор смешанного содержимого с перечнем имён
		::run("<!DOCTYPE a [<!ELEMENT a (#PCDATA|b|c)*>]><a/>", 4096, error);
		// Выполняем проверку принятия смешанного содержимого
		ASSERT_EQ(error, xml::error_t::NONE);
		// Выполняем разбор смешанного содержимого без перечня имён
		::run("<!DOCTYPE a [<!ELEMENT a (#PCDATA)*>]><a/>", 4096, error);
		// Выполняем проверку принятия смешанного содержимого
		ASSERT_EQ(error, xml::error_t::NONE);
	}
}
/**
 * @brief Проверка имён и записей описания типа за пределами латиницы
 *
 * @note Знаки имени разбираются двумя разными путями: однобайтовые - по таблице,
 *       прочие - чтением кодового значения. Второй путь набором не проходился вовсе,
 *       а именно им идут все имена за пределами латиницы
 * @warning Знаки СОЕДИНИТЕЛЬНЫЕ имя начинать не вправе, но внутри имени законны:
 *          два эти правила различны, и проверять их обязательно порознь - прикрытие,
 *          принявшее соединительный знак началом, испортило бы опознание имён молча
 */
TEST(CodecXmlReader, WideNamesAndLiterals) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	/**
	 * Имена узлов и свойств за пределами латиницы
	 */
	for(auto & text : vector <string> {
		// Имя из знака с надстрочным знаком и имя кириллицей
		string("<\xC3\xA9/>"),
		string("<\xD1\x89/>"),
		// Имя со знаком соединительным внутри
		string("<a\xCC\x81/>"),
		// Имя со знаком отделения слов внутри
		string("<a\xC2\xB7/>"),
		// Имя со знаком подчёркивания надстрочным внутри
		string("<a\xE2\x80\xBF/>"),
		// Имя свойства со знаком соединительным внутри
		string("<a b\xCC\x81=\"1\"/>"),
		// Знак соединительный в содержимом узла
		string("<a>\xCC\x81</a>"),
		// Описание типа документа с именами кириллицей
		string("<!DOCTYPE \xD1\x89 [<!ENTITY \xD1\x91 'v'>]><\xD1\x89>&\xD1\x91;</\xD1\x89>")
	}){
		// Выполняем разбор текста разметки
		::run(text, 4096, error);
		// Выполняем проверку принятия имени за пределами латиницы
		ASSERT_EQ(error, xml::error_t::NONE) << text;
	}
	/**
	 * Знак соединительный началом имени
	 *
	 * @note Договор разделяет знаки, имя НАЧИНАЮЩИЕ, и знаки, внутри имени
	 *       допустимые: соединительные принадлежат лишь вторым
	 */
	{
		// Собираемый текст разметки с именем, начатым знаком соединительным
		string text = "<";
		// Выполняем добавление знака соединительного началом имени
		text.append("\xCC\x81");
		// Выполняем завершение текста разметки
		text.append("a/>");
		// Выполняем разбор текста разметки
		::run(text, 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::INVALID_NAME);
	}
	{
		// Собираемый текст разметки с именем сущности, начатым знаком отделения слов
		string text = "<!DOCTYPE a [<!ENTITY ";
		// Выполняем добавление знака отделения слов началом имени
		text.append("\xC2\xB7");
		// Выполняем завершение текста разметки
		text.append("x 'v'>]><a/>");
		// Выполняем разбор текста разметки
		::run(text, 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::INVALID_DOCTYPE);
	}
	/**
	 * Записи внутри описания типа документа
	 */
	{
		// Выполняем разбор записи без кавычек вовсе
		::run("<!DOCTYPE a [<!ENTITY e v>]><a/>", 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::INVALID_DOCTYPE);
		// Выполняем разбор записи, кавычкой не закрытой
		::run("<!DOCTYPE a [<!ENTITY e 'v>]><a/>", 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::INVALID_DOCTYPE);
		/**
		 * Выполняем разбор записи со знаком начала метки внутри
		 *
		 * @note Знак этот в записи описания типа стоять не вправе так же, как и в
		 *       значении свойства: записать его положено ссылкой
		 */
		::run("<!DOCTYPE a [<!ATTLIST a x CDATA '<'>]><a/>", 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::INVALID_DOCTYPE);
		// Выполняем разбор записи с числовой ссылкой, построенной ошибочно
		::run("<!DOCTYPE a [<!ENTITY e '&#xZ;'>]><a/>", 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::INVALID_DOCTYPE);
		// Выполняем разбор строения узла без перечня вовсе
		::run("<!DOCTYPE a [<!ELEMENT a b>]><a/>", 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::INVALID_DOCTYPE);
	}
	/**
	 * Пробельные знаки и числовые ссылки внутри записей описания типа
	 *
	 * @note Подача строки и отступ в записи описания типа обращаются пробелом тем же
	 *       правилом, что и в значении свойства: запись эта значением свойства и
	 *       становится, будучи умолчанием
	 */
	{
		// Выполняем разбор записи с подачей строки внутри
		::run("<!DOCTYPE a [<!ATTLIST a x CDATA 'з\nз'>]><a/>", 4096, error);
		// Выполняем проверку принятия записи с подачей строки
		ASSERT_EQ(error, xml::error_t::NONE);
		// Выполняем разбор записи с отступом внутри
		::run("<!DOCTYPE a [<!ATTLIST a x CDATA 'з\tз'>]><a/>", 4096, error);
		// Выполняем проверку принятия записи с отступом
		ASSERT_EQ(error, xml::error_t::NONE);
		// Выполняем разбор записи с числовой ссылкой внутри
		::run("<!DOCTYPE a [<!ENTITY e '&#1090;'>]><a>&e;</a>", 4096, error);
		// Выполняем проверку принятия записи с числовой ссылкой
		ASSERT_EQ(error, xml::error_t::NONE);
	}
	/**
	 * Ссылка на объявленную сущность при отключённой подстановке
	 *
	 * @note Отключение снимает подстановку сущностей объявленных, и ссылка на такую
	 *       сущность становится ссылкой на неизвестную: объявление её разбор помнит,
	 *       но разворачивать отказывается
	 */
	{
		// Настройки разбора с отключённой подстановкой сущностей
		xml::reader_t::settings_t settings;
		// Выполняем отключение подстановки объявленных сущностей
		settings.entities = false;
		// Выполняем разбор текста со ссылкой на объявленную сущность
		::run("<!DOCTYPE a [<!ENTITY e 'v'>]><a>&e;</a>", 4096, error, settings);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::UNKNOWN_ENTITY);
	}
}
/**
 * @brief Проверка обрывов записей и негодных имён внутри описания типа документа
 *
 * @note Собраны ветви, до каких набор не добирался: имя, оборванное знаком именем
 *       не являющимся, запись без кавычек у каждого из объявлений порознь, ссылка
 *       внутри записи и обрыв самим концом текста
 * @warning Обрыв КОНЦОМ ТЕКСТА проверяется отдельно от обрыва знаком: разбору тут
 *          не хватает не знака завершения, а самого текста, и ветвь эта иная
 */
TEST(CodecXmlReader, DoctypeLiteralsAndNames) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	/**
	 * Выполняем перебор негодных построений внутри описания типа документа
	 */
	for(auto & text : vector <string> {
		// Имя сущности, цифрой начатое
		string("<!DOCTYPE a [<!ENTITY 1x 'v'>]><a/>"),
		/**
		 * Имя сущности, оборванное знаком, имени не принадлежащим
		 *
		 * @note Знак этот многобайтовый, и оборвать имя обязан тем же путём, что
		 *       и однобайтовый: разбор имени идёт двумя дорогами, и вторая набором
		 *       не проходилась
		 */
		string("<!DOCTYPE a [<!ENTITY e\xC2\xA9 'v'>]><a/>"),
		// Строение узла с вложенным перечнем, построенным ошибочно
		string("<!DOCTYPE a [<!ELEMENT a ((b),*)>]><a/>"),
		string("<!DOCTYPE a [<!ELEMENT a (b,*)>]><a/>"),
		// Запись умолчания свойства и указателя обозначения без кавычек
		string("<!DOCTYPE a [<!ATTLIST a x CDATA v>]><a/>"),
		string("<!DOCTYPE a [<!NOTATION n SYSTEM u>]><a/>"),
		// Ссылка внутри записи с негодным именем и без завершения
		string("<!DOCTYPE a [<!ENTITY e '&1x;'>]><a/>"),
		string("<!DOCTYPE a [<!ENTITY e '&x'>]><a/>"),
		// Запись, оборванная самим концом текста
		string("<!DOCTYPE a [<!ENTITY e 'v")
	}){
		// Выполняем разбор текста разметки
		::run(text, 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::INVALID_DOCTYPE) << text;
	}
	/**
	 * Обрывы концом текста вне описания типа документа
	 *
	 * @note Отказ здесь иной: метка не закрыта, а не построена ошибочно. Разбор
	 *       обязан различать эти два случая, ибо чинить их потребителю по-разному -
	 *       в первом дописать текст, во втором исправить написанное
	 */
	{
		// Выполняем разбор значения свойства, оборванного концом текста
		::run("<a x=\"v", 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::UNCLOSED_TAG);
		// Выполняем разбор ссылки в значении свойства, оборванной концом текста
		::run("<a x=\"&am", 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::UNCLOSED_TAG);
		// Выполняем разбор ссылки в содержимом, точкой с запятой не закрытой
		::run("<a>&am</a>", 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::INVALID_REFERENCE);
	}
	/**
	 * Глубина вложенности сущностей в значении свойства
	 *
	 * @note Значение свойства разворачивает сущности своим путём, отличным от пути
	 *       содержимого: прикрытие по глубине обязано стоять на обоих, иначе цепочка
	 *       сущностей уходила бы вглубь через свойство
	 */
	{
		// Собираемый текст разметки с цепочкой сущностей
		string text = "<!DOCTYPE a [";
		/**
		 * Выполняем построение цепочки сущностей, ссылающихся одна на другую
		 */
		for(uint32_t i = 0; i < 40; i++)
			// Выполняем добавление очередного звена цепочки сущностей
			text.append("<!ENTITY e").append(::std::to_string(i))
			    .append(" '&e").append(::std::to_string(i + 1)).append(";'>");
		// Выполняем завершение цепочки сущностей и текста разметки
		text.append("<!ENTITY e40 'дно'>]><a x=\"&e0;\"/>");
		// Выполняем разбор текста разметки
		::run(text, 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::ENTITY_DEPTH_EXCEEDED);
	}
}
/**
 * @brief Проверка обращения к свойствам узла и пробельных знаков в метке
 *
 * @note Собраны места, разбором проходимые всякий раз, но набором не сличавшиеся:
 *       выдача отсутствующего свойства, пробельные знаки во всех местах метки, где
 *       договор их допускает, и повторное объявление одного пространства имён
 * @warning Отсутствующее свойство отдаётся ПУСТОЙ ЗАПИСЬЮ, а не отказом: свойство,
 *          не объявленное узлом, равносильно свойству пустому лишь для потребителя,
 *          заведомо знающего, что оно необязательно - оттого наличие проверяется
 *          отдельным обращением
 */
TEST(CodecXmlReader, AttributeAccessAndSpacing) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	/**
	 * Обращение к свойству узла по имени
	 */
	{
		// Объект потокового чтения текста разметки
		xml::reader_t reader(::logger());
		// Выполняем подачу текста разметки целиком
		ASSERT_TRUE(reader.feed("<a x=\"1\"/>", 10, true));
		// Признак того, что узел разметки встречен
		bool found = false;
		/**
		 * Выполняем перебор всех событий разбора
		 */
		while(reader.next()){
			/**
			 * Если встречено открытие узла разметки
			 */
			if(reader.event() == xml::event_t::ELEMENT_OPEN){
				// Запоминаем, что узел разметки встречен
				found = true;
				// Выполняем проверку выдачи объявленного свойства
				ASSERT_EQ(reader.attribute("x"), "1");
				// Выполняем проверку выдачи свойства, узлом не объявленного
				ASSERT_TRUE(reader.attribute("y").empty());
				// Выполняем проверку наличия объявленного свойства
				ASSERT_TRUE(reader.has("x"));
				// Выполняем проверку отсутствия свойства, узлом не объявленного
				ASSERT_FALSE(reader.has("y"));
			}
		}
		/**
		 * Выполняем проверку того, что узел разметки встречен
		 *
		 * @note Без этой проверки набор прошёл бы и у разбора, событий не выдающего
		 *       вовсе: проверки внутри перебора не исполнились бы ни разу
		 */
		ASSERT_TRUE(found);
	}
	/**
	 * Пробельные знаки во всех местах метки, где договор их допускает
	 */
	for(auto & text : vector <string> {
		// Пробел перед закрытием метки и внутри закрывающей метки
		"<a />",
		"<a></a >",
		// Подача строки и отступ между свойствами узла
		"<a\n  x=\"1\"\n  y=\"2\"\n/>"
	}){
		// Выполняем разбор текста разметки
		::run(text, 4096, error);
		// Выполняем проверку принятия текста разметки
		ASSERT_EQ(error, xml::error_t::NONE) << text;
	}
	/**
	 * Повторное объявление одного пространства имён на вложенном узле
	 *
	 * @note Объявление это не повтор, а ПЕРЕКРЫТИЕ: связывание принадлежит узлу,
	 *       и вложенный узел вправе объявить тот же префикс тем же обозначением
	 */
	::run("<a xmlns:p=\"u\"><b xmlns:p=\"u\"/></a>", 4096, error);
	// Выполняем проверку принятия текста разметки
	ASSERT_EQ(error, xml::error_t::NONE);
	// Выполняем разбор узла с несколькими свойствами и связыванием префикса
	::run("<a xmlns:p=\"u\" p:x=\"1\" p:y=\"2\" z=\"3\" w=\"4\" v=\"5\"/>", 4096, error);
	// Выполняем проверку принятия текста разметки
	ASSERT_EQ(error, xml::error_t::NONE);
	/**
	 * Имя, оборванное знаком многобайтовым, имени не принадлежащим
	 *
	 * @note Отказ у имени узла и у имени свойства РАЗНЫЙ: у первого метка построена
	 *       ошибочно, у второго ошибочно построено свойство. Разница эта существенна
	 *       для потребителя, ибо чинить ему разные места
	 */
	{
		// Выполняем разбор имени узла, оборванного знаком именем не являющимся
		::run(string("<a\xC2\xA9/>"), 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::INVALID_TAG);
		// Выполняем разбор имени свойства, оборванного тем же знаком
		::run(string("<a b\xC2\xA9=\"1\"/>"), 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::INVALID_ATTRIBUTE);
	}
}
/**
 * @brief Проверка названия кодировки в объявлении разметки двумя дорогами
 *
 * @note Название кодировки осматривают ДВОЕ: приведение, определяющее кодировку по
 *       объявлению, и сам разбор объявления. Дороги эти взаимно исключающи - при
 *       навязанной настройками кодировке приведение объявление не читает вовсе,
 *       и осмотр остаётся за разбором
 * @warning Коды отказа у них РАЗНЫЕ и разными обязаны быть: приведение отвечает
 *          «кодировка не поддерживается», разбор - «объявление построено ошибочно».
 *          Первое означает, что кодировка названа, но неизвестна; второе - что
 *          названное кодировкой не является вовсе
 */
TEST(CodecXmlReader, DeclarationEncodingNames) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	/**
	 * Выполняем перебор негодных названий кодировки
	 */
	for(auto & text : vector <string> {
		// Название пустое, цифрой начатое и с пробелом внутри
		"<?xml version=\"1.0\" encoding=''?><a/>",
		"<?xml version=\"1.0\" encoding='1utf'?><a/>",
		"<?xml version=\"1.0\" encoding='ut f8'?><a/>"
	}){
		/**
		 * Дорога первая: кодировка определяется приведением по самому объявлению
		 */
		{
			// Выполняем разбор текста разметки
			::run(text, 4096, error);
			// Выполняем проверку кода ошибки разбора
			ASSERT_EQ(error, xml::error_t::UNSUPPORTED_ENCODING) << text;
		}
		/**
		 * Дорога вторая: кодировка навязана настройками разбора
		 *
		 * @note Приведению читать объявление незачем - кодировка ему уже задана,
		 *       и негодное название встречает лишь разбор объявления
		 */
		{
			// Настройки разбора с навязанной кодировкой исходного текста
			xml::reader_t::settings_t settings;
			// Выполняем навязывание кодировки исходного текста
			settings.encoding = xml::encoding_t::UTF8;
			// Выполняем разбор текста разметки
			::run(text, 4096, error, settings);
			// Выполняем проверку кода ошибки разбора
			ASSERT_EQ(error, xml::error_t::INVALID_DECLARATION) << text;
		}
	}
	/**
	 * Законное название кодировки проходит обеими дорогами
	 */
	{
		// Выполняем разбор текста разметки с определением кодировки
		::run("<?xml version=\"1.0\" encoding='utf-8'?><a/>", 4096, error);
		// Выполняем проверку принятия текста разметки
		ASSERT_EQ(error, xml::error_t::NONE);
		// Настройки разбора с навязанной кодировкой исходного текста
		xml::reader_t::settings_t settings;
		// Выполняем навязывание кодировки исходного текста
		settings.encoding = xml::encoding_t::UTF8;
		// Выполняем разбор текста разметки с навязанной кодировкой
		::run("<?xml version=\"1.0\" encoding='utf-8'?><a/>", 4096, error, settings);
		// Выполняем проверку принятия текста разметки
		ASSERT_EQ(error, xml::error_t::NONE);
	}
}

/**
 * @brief Проверка отказов разбора внутреннего подмножества описания типа документа
 *
 * @details Разбор подмножества отвечает отказом на всякое нарушение построения
 *          объявления, и код отказа у всех этих нарушений ОДИН - `INVALID_DOCTYPE`.
 *          Различить их по коду потребитель не может, и оттого проверяется здесь не
 *          код сам по себе, а то, что разбор вообще не пропускает построения мимо:
 *          пропущенное объявление становится объявлением НЕИЗВЕСТНЫМ, а неизвестная
 *          сущность далее подставляется пустотою, и документ разбирается молча не тем,
 *          чем он записан
 *
 * @note Проверяется каждое построение ДВУМЯ размерами куска подачи - целиком и по
 *       одному октету: разбор подмножества ведётся с оглядкой на границу куска, и
 *       отказ, наступающий лишь при подаче целиком, означал бы, что разбор кусками
 *       нарушение пропускает
 *
 */
TEST(CodecXmlReader, DoctypeSubsetRefusals) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	/**
	 * Выполняем перебор негодных построений объявлений подмножества
	 */
	for(auto & text : vector <string> {
		// Объявление параметрической сущности без пробела после знака процента
		"<!DOCTYPE a [<!ENTITY %e 'v'>]><a/>",
		// Объявление параметрической сущности без имени вовсе
		"<!DOCTYPE a [<!ENTITY % >]><a/>",
		// Объявление сущности с мусором вместо завершения
		"<!DOCTYPE a [<!ENTITY e 'v' junk]><a/>",
		// Объявление узла без пробела перед именем
		"<!DOCTYPE a [<!ELEMENT]><a/>",
		// Объявление узла без имени вовсе
		"<!DOCTYPE a [<!ELEMENT >]><a/>",
		// Объявление перечня атрибутов, оборванное концом текста
		"<!DOCTYPE a [<!ATTLIST a "
	}){
		/**
		 * Дорога первая: текст подаётся целиком
		 */
		{
			// Выполняем разбор текста разметки
			::run(text, text.size() + 1, error);
			// Выполняем проверку кода ошибки разбора
			ASSERT_EQ(error, xml::error_t::INVALID_DOCTYPE) << text;
		}
		/**
		 * Дорога вторая: текст подаётся по одному октету
		 */
		{
			// Выполняем разбор текста разметки
			::run(text, 1, error);
			// Выполняем проверку кода ошибки разбора
			ASSERT_EQ(error, xml::error_t::INVALID_DOCTYPE) << text;
		}
	}
	/**
	 * Законные построения тех же объявлений отказа не вызывают
	 *
	 * @warning Случай законный тут обязателен: проверка, сплошь состоящая из отказов,
	 *          проходит и при разборе, отвергающем ВСЁ подряд
	 */
	for(auto & text : vector <string> {
		// Объявление параметрической сущности по правилам
		"<!DOCTYPE a [<!ENTITY % e 'v'>]><a/>",
		// Объявление обычной сущности по правилам
		"<!DOCTYPE a [<!ENTITY e 'v'>]><a/>",
		// Объявление узла по правилам
		"<!DOCTYPE a [<!ELEMENT a (#PCDATA)>]><a/>",
		// Объявление перечня атрибутов по правилам
		"<!DOCTYPE a [<!ATTLIST a x CDATA #IMPLIED>]><a/>"
	}){
		// Выполняем разбор текста разметки
		::run(text, text.size() + 1, error);
		// Выполняем проверку отсутствия ошибки разбора
		ASSERT_EQ(error, xml::error_t::NONE) << text;
	}
}

/**
 * @brief Проверка пределов подстановки сущностей и отказов подмножества
 *
 * @details Подстановка сущности ведётся вглубь, и предел глубины тут не украшение:
 *          сущность, ссылающаяся на себя саму через цепочку соседей, раскрывалась бы
 *          бесконечно, съедая память до отказа выделения. Предел `MAX_ENTITY_DEPTH`
 *          обрывает раскрытие отказом, и проверяется здесь то, что обрыв этот
 *          наступает, а не то, на каком именно уровне
 *
 * @note Цепочка строится с запасом сверх предела, а не ровно по нему: проверка,
 *       привязанная к самому числу, отказывала бы при всяком его пересмотре, тогда
 *       как стеречь ей положено наличие предела, а не его величину
 *
 * @warning Сущность, пересекающая границу разметки, отвергается ОТДЕЛЬНЫМ отказом:
 *          подставь её разбор как есть, и закрывающая метка пришла бы из подстановки,
 *          а не из текста, - дерево вышло бы построенным не по записанному
 *
 */
TEST(CodecXmlReader, EntityDepthAndSubsetRefusals) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	/**
	 * Дорога первая: цепочка подстановок глубже предела
	 */
	{
		// Собираемый текст описания типа документа
		string text = "<!DOCTYPE a [";
		// Число звеньев цепочки подстановок с запасом сверх предела
		const uint32_t count = (xml::MAX_ENTITY_DEPTH + 8);
		/**
		 * Выполняем сборку цепочки сущностей, ссылающихся одна на другую
		 */
		for(uint32_t i = 1; i < count; i++)
			// Выполняем добавление очередного звена цепочки
			text.append("<!ENTITY e").append(std::to_string(i)).append(" '&e")
			    .append(std::to_string(i + 1)).append(";'>");
		// Выполняем завершение цепочки значением без ссылок
		text.append("<!ENTITY e").append(std::to_string(count)).append(" 'x'>]><a>&e1;</a>");
		// Выполняем разбор текста разметки
		::run(text, text.size() + 1, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::ENTITY_DEPTH_EXCEEDED);
		// Выполняем разбор того же текста подачей по одному октету
		::run(text, 1, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::ENTITY_DEPTH_EXCEEDED);
	}
	/**
	 * Дорога вторая: цепочка подстановок в пределах дозволенного
	 *
	 * @note Случай этот доказывает, что отказ выше вызван именно ПРЕДЕЛОМ, а не самой
	 *       вложенностью подстановок
	 */
	{
		// Собираемый текст описания типа документа
		string text = "<!DOCTYPE a [";
		/**
		 * Выполняем сборку короткой цепочки сущностей
		 */
		for(uint32_t i = 1; i < 8; i++)
			// Выполняем добавление очередного звена цепочки
			text.append("<!ENTITY e").append(std::to_string(i)).append(" '&e")
			    .append(std::to_string(i + 1)).append(";'>");
		// Выполняем завершение цепочки значением без ссылок
		text.append("<!ENTITY e8 'x'>]><a>&e1;</a>");
		// Выполняем разбор текста разметки
		const string result = ::run(text, text.size() + 1, error);
		// Выполняем проверку отсутствия ошибки разбора
		ASSERT_EQ(error, xml::error_t::NONE);
		// Выполняем проверку подстановки значения через всю цепочку
		ASSERT_NE(result.find('x'), string::npos) << result;
	}
	/**
	 * Дорога третья: сущность, пересекающая границу разметки
	 */
	for(auto & text : vector <string> {
		// Сущность, несущая закрывающую метку узла
		"<!DOCTYPE a [<!ENTITY e '</a>'>]><a>&e;</a>",
		// Сущность, несущая открывающую метку узла
		"<!DOCTYPE a [<!ENTITY e '<b>'>]><a>&e;</b></a>"
	}){
		// Выполняем разбор текста разметки
		::run(text, text.size() + 1, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::ENTITY_BOUNDARY) << text;
	}
	/**
	 * Дорога четвёртая: примечания и указания обработчику внутри подмножества
	 */
	{
		// Примечание с двойным дефисом внутри
		::run("<!DOCTYPE a [<!--x--y-->]><a/>", 32, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::INVALID_COMMENT);
		// Указание обработчику без имени
		::run("<!DOCTYPE a [<? ?>]><a/>", 32, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::INVALID_PROCESSING);
		// Примечание и указание обработчику по правилам отказа не вызывают
		::run("<!DOCTYPE a [<!--x--><?p v?>]><a/>", 32, error);
		// Выполняем проверку отсутствия ошибки разбора
		ASSERT_EQ(error, xml::error_t::NONE);
	}
}

/**
 * @brief Проверка отказов разбора открывающей метки и значений атрибутов из описания
 *
 * @details Разбор открывающей метки отвергает два построения, которые легко принять за
 *          безобидные: знак «меньше» внутри самой метки и косую черту, стоящую не перед
 *          закрытием. Оба они означают, что метка не закрыта там, где разбор её закончил,
 *          и пропусти он их - дерево получило бы узел с именем, собранным из двух меток
 *
 * @note Значения атрибутов по умолчанию, объявленные в описании типа документа,
 *       подставляются лишь тем узлам, где атрибут НЕ записан в тексте. Проверяются
 *       здесь обе дороги сразу - записанный атрибут и незаписанный, - потому что
 *       различает их поиск среди уже разобранных атрибутов, и ошибка в нём даёт
 *       либо потерю записанного значения, либо потерю подстановки
 *
 */
TEST(CodecXmlReader, ElementTagAndDefaultAttributes) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	/**
	 * Выполняем перебор негодных построений открывающей метки
	 */
	for(auto & text : vector <string> {
		// Знак «меньше» на месте имени атрибута
		"<a <b/>",
		// Знак «меньше» после разобранного атрибута
		"<a x=\"1\" <b/>",
		// Косая черта, стоящая не перед закрытием метки
		"<a/x>",
		// Косая черта после разобранного атрибута не перед закрытием
		"<a x=\"1\"/x>"
	}){
		/**
		 * Выполняем разбор двумя размерами куска подачи
		 */
		for(const size_t step : {static_cast <size_t> (1), text.size() + 1}){
			// Выполняем разбор текста разметки
			::run(text, step, error);
			// Выполняем проверку кода ошибки разбора
			ASSERT_EQ(error, xml::error_t::INVALID_TAG) << text << " кусок " << step;
		}
	}
	/**
	 * Значение атрибута по умолчанию подставляется незаписанному атрибуту
	 */
	{
		// Выполняем разбор текста разметки без записанного атрибута
		const string result = ::run("<!DOCTYPE a [<!ATTLIST a x CDATA 'd'>]><a/>", 4096, error);
		// Выполняем проверку отсутствия ошибки разбора
		ASSERT_EQ(error, xml::error_t::NONE);
		// Выполняем проверку подстановки значения по умолчанию
		ASSERT_NE(result.find('d'), string::npos) << result;
	}
	/**
	 * Записанное в тексте значение подстановкой НЕ замещается
	 *
	 * @warning Случай этот и есть главный тут: подстановка, не сличающая имя атрибута с
	 *          уже разобранными, затирала бы записанное значение умолчанием из описания
	 */
	{
		// Выполняем разбор текста разметки с записанным атрибутом
		const string result = ::run("<!DOCTYPE a [<!ATTLIST a x CDATA 'd'>]><a x='t'/>", 4096, error);
		// Выполняем проверку отсутствия ошибки разбора
		ASSERT_EQ(error, xml::error_t::NONE);
		// Выполняем проверку сохранения записанного значения
		ASSERT_NE(result.find('t'), string::npos) << result;
	}
	/**
	 * Сличение ведётся по имени вместе с обозначением, а не по одному имени
	 */
	{
		// Выполняем разбор текста разметки с атрибутом, снабжённым обозначением
		const string result = ::run("<!DOCTYPE a [<!ATTLIST a p:x CDATA 'd'>]>"
		                            "<a xmlns:p='u' p:x='t'/>", 4096, error);
		// Выполняем проверку отсутствия ошибки разбора
		ASSERT_EQ(error, xml::error_t::NONE);
		// Выполняем проверку сохранения записанного значения
		ASSERT_NE(result.find('t'), string::npos) << result;
	}
}

/**
 * @brief Проверка разбора раздела дословного текста
 *
 * @details Раздел дословного текста выдаётся потребителю ЧАСТЯМИ, не дожидаясь закрытия:
 *          раздел размером в мегабайты иначе пришлось бы держать в памяти целиком. Оттого
 *          выдача останавливается, не доходя до конца накопленного, - закрытие раздела
 *          могло прийти следующим куском, и выданное было бы уже не вернуть
 *
 * @note Возврат каретки на самой границе выдачи изымается особо: пара из возврата каретки
 *       с переводом строки договором обращается в один знак, и разорви выдача её пополам,
 *       обращение это не состоялось бы вовсе - потребитель получил бы два знака вместо
 *       одного, причём лишь при определённом размере куска подачи
 *
 * @warning Проверяется раздел НЕСКОЛЬКИМИ размерами куска подачи: разбор его ведётся с
 *          оглядкой на границу куска, и проверка одним размером доказывает лишь то, что
 *          при этом размере он работает
 *
 */
TEST(CodecXmlReader, CdataSectionsAndChunking) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	/**
	 * Раздел дословного текста вне корневого узла отвергается
	 */
	for(auto & text : vector <string> {
		// Раздел, стоящий прежде корневого узла
		"<![CDATA[x]]>",
		// Раздел, стоящий после закрытия корневого узла
		"<a/><![CDATA[x]]>"
	}){
		// Выполняем разбор текста разметки
		::run(text, 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::CONTENT_OUTSIDE_ROOT) << text;
	}
	/**
	 * Раздел, поданный кусками, разбирается одинаково при любом размере куска
	 */
	{
		// Собираемый текст раздела дословного текста
		string content;
		/**
		 * Выполняем сборку содержимого, заведомо большего одного куска подачи
		 */
		for(uint32_t i = 0; i < 400; i++)
			// Выполняем добавление очередного звена содержимого
			content.append("0123456789");
		// Выполняем завершение содержимого возвратом каретки на границе выдачи
		const string text = ("<a><![CDATA[" + content + "\r]]></a>");
		/**
		 * Собираем содержимое разделов дословного текста из записи событий разбора
		 *
		 * @warning Сличать записи событий целиком тут НЕЛЬЗЯ: раздел дословного текста
		 *          выдаётся частями, и число событий законно зависит от размера куска
		 *          подачи - в том и смысл потоковой выдачи. Сличению подлежит собранное
		 *          СОДЕРЖИМОЕ, а не то, сколькими событиями оно пришло
		 */
		const auto gather = [](const string & record) noexcept -> string {
			// Собираемое содержимое разделов дословного текста
			string result;
			// Положение поиска очередного события раздела
			size_t offset = 0;
			/**
			 * Выполняем перебор всех событий раздела дословного текста
			 */
			for(;;){
				// Выполняем поиск очередного события раздела дословного текста
				const size_t begin = record.find("[cd ", offset);
				// Если событий раздела больше не осталось, выходим из перебора
				if(begin == string::npos)
					break;
				// Выполняем поиск конца очередного события раздела
				const size_t end = record.find(']', begin);
				// Если конец события не найден, выходим из перебора
				if(end == string::npos)
					break;
				// Выполняем добавление содержимого события к собранному
				result.append(record, begin + 4, end - begin - 4);
				// Выполняем переход к следующему событию раздела
				offset = (end + 1);
			}
			// Выводим собранное содержимое разделов дословного текста
			return result;
		};
		// Содержимое раздела, снятое подачей текста целиком
		const string whole = gather(::run(text, text.size() + 1, error));
		// Выполняем проверку отсутствия ошибки разбора
		ASSERT_EQ(error, xml::error_t::NONE);
		// Выполняем проверку непустоты собранного содержимого
		ASSERT_FALSE(whole.empty());
		/**
		 * Выполняем перебор размеров куска подачи
		 *
		 * @note Размеры взяты и малые, и близкие к запасу выдачи: разрыв возврата каретки
		 *       наступает лишь при определённых размерах, и один размер его не ловит
		 */
		for(const size_t step : {static_cast <size_t> (1), static_cast <size_t> (7),
		                         static_cast <size_t> (64), static_cast <size_t> (511)}){
			// Выполняем разбор текста разметки подачей кусками
			const string parts = gather(::run(text, step, error));
			// Выполняем проверку отсутствия ошибки разбора
			ASSERT_EQ(error, xml::error_t::NONE) << "кусок " << step;
			// Выполняем проверку совпадения собранного содержимого с поданным целиком
			ASSERT_EQ(parts, whole) << "кусок " << step;
		}
	}
}

/**
 * @brief Проверка изъятия разобранного начала внутри раздела дословного текста
 *
 * @details Разобранное начало текста изымается из буфера, когда его накопится довольно:
 *          иначе буфер рос бы на весь поданный поток. Внутри раздела дословного текста
 *          изъятие требует особого обхождения - начало раздела задано положением в
 *          приведённом тексте и лежит ПОЗАДИ места разбора, и сдвинуть его надлежит на
 *          столько же, на сколько изъято, иначе оно указывает мимо
 *
 * @warning Раздел здесь заведомо больше предела изъятия (64 КБ), и в этом весь смысл
 *          проверки: разделы меньшего размера, какими набор полон, до изъятия попросту
 *          не доходят, и сдвиг начала раздела не выполняется ни разу
 *
 * @note Сличается собранное СОДЕРЖИМОЕ, а не запись событий: раздел выдаётся частями, и
 *       число событий законно зависит от размера куска подачи
 *
 */
TEST(CodecXmlReader, CdataCompactionBeyondLimit) {
	/**
	 * Собираем содержимое раздела, заведомо большее предела изъятия
	 */
	string content;
	/**
	 * Выполняем сборку содержимого раздела дословного текста
	 */
	for(uint32_t i = 0; i < 40000; i++)
		// Выполняем добавление очередного звена содержимого
		content.append("0123456789");
	// Собираем текст разметки с разделом дословного текста
	const string text = ("<a><![CDATA[" + content + "]]></a>");
	/**
	 * Выполняем перебор размеров куска подачи текста
	 */
	for(auto & step : vector <size_t> {4096, 65536, 131072, 0}){
		// Код ошибки разбора
		xml::error_t error = xml::error_t::NONE;
		// Объект потокового чтения текста разметки
		xml::reader_t reader(::logger());
		// Положение подачи в разбираемом тексте
		size_t offset = 0;
		// Собираемое содержимое разделов дословного текста
		string gathered;
		// Получаем размер куска подачи текста
		const size_t length = ((step > 0) ? step : (text.size() + 1));
		/**
		 * Выполняем подачу текста разметки кусками
		 */
		for(;;){
			// Получаем размер очередного куска текста
			const size_t size = ((offset + length) > text.size() ? (text.size() - offset) : length);
			// Получаем признак последнего куска текста
			const bool end = ((offset + size) >= text.size());
			// Выполняем передачу очередного куска текста
			ASSERT_TRUE(reader.feed(text.data() + offset, size, end)) << "кусок " << step;
			// Выполняем переход к следующему куску текста
			offset += size;
			/**
			 * Выполняем перебор всех событий разбора
			 */
			while(reader.next()){
				/**
				 * Если получен раздел дословного текста
				 */
				if(reader.event() == xml::event_t::CDATA)
					// Выполняем добавление содержимого раздела к собираемому
					gathered.append(reader.text());
			}
			// Запоминаем код ошибки разбора
			error = reader.error();
			/**
			 * Если текст разметки подан целиком
			 */
			if(end)
				// Выходим из подачи текста
				break;
		}
		// Выполняем проверку отсутствия ошибки разбора
		ASSERT_EQ(error, xml::error_t::NONE) << "кусок " << step;
		// Выполняем проверку собранного содержимого раздела
		ASSERT_EQ(gathered, content) << "кусок " << step;
	}
}

/**
 * @brief Проверка заслонов, отысканных ворошителем по карте покрытия
 *
 * @details Места эти набор не проходил ни разу, и отыскал их ворошитель, пущенный по
 *          объектным файлам с покрытием. Каждое из них достижимо коротким текстом, и
 *          закрепляются они здесь порознь - образцом, отысканным ворошителем, и кодом
 *          отказа, какой он при этом дал
 *
 * @note Размер куска подачи и настройки разбора части образцов существенны: их подобрал
 *       ворошитель, и произвольная замена уводит разбор мимо закрепляемого места
 *
 */
TEST(CodecXmlReader, ProbedGuards) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	/**
	 * Пара из возврата каретки с переводом строки внутри примечания
	 *
	 * @details Приведение конца строки внутри примечания обращает пару в один знак, и
	 *          перевод строки при этом пропускается отдельным шагом. Набор примечаний с
	 *          парою знаков не подавал вовсе, и шаг этот не выполнялся ни разу
	 */
	{
		// Выполняем разбор примечания вне корневого узла
		::run("<!--c\r\n-->", 3, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::MISSING_ROOT);
		// Выполняем разбор того же примечания внутри корневого узла
		const string record = ::run("<a><!--c\r\n--></a>", 3, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::NONE);
		// Выполняем проверку приведения пары знаков к переводу строки
		ASSERT_EQ(record, "<a>[!c\n]</a>[end]");
	}
	/**
	 * Разметка открывающей метки узла, превысившая предел объёма события
	 *
	 * @warning Предел этот задан НАСТРОЙКАМИ, а не разрядностью отрезка: прежде место
	 *          это было помечено в теле кодека как требующее четырёх гигабайт текста, и
	 *          помета та была неверна
	 */
	{
		// Настройки разбора текста разметки
		xml::reader_t::settings_t settings;
		// Задаём предел объёма события
		settings.maxEvent = 8;
		// Отключаем разбор пространств имён
		settings.namespaces = false;
		// Выполняем разбор текста разметки с меткой длиннее предела
		::run("<a =\"\"fa>", 0, error, settings);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::OVERFLOW_LIMIT);
	}
	/**
	 * Ссылка на сущность вне корневого узла при опустевшем стеке узлов
	 */
	{
		// Настройки разбора текста разметки
		xml::reader_t::settings_t settings;
		// Задаём предел объёма события
		settings.maxEvent = 8;
		// Выполняем разбор ссылки, поданной без корневого узла
		::run("&", 3, error, settings);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::CONTENT_OUTSIDE_ROOT);
	}
	/**
	 * Обозначение обработчика внутреннего подмножества длиною ровно в три знака
	 *
	 * @details Отведённое договором имя сличается лишь при совпадении длины, и набор
	 *          подавал внутри описания типа документа обозначения либо отведённые, либо
	 *          иной длины. Обозначение из трёх знаков, отведённым НЕ являющееся, дороги
	 *          мимо сличения не проходило ни разу
	 */
	{
		// Выполняем разбор указания обработчику с трёхзначным обозначением
		::run("<!DOCTYPE a [<?xSi?>]><a/>", 12, error);
		// Выполняем проверку отсутствия ошибки разбора
		ASSERT_EQ(error, xml::error_t::NONE);
		// Выполняем разбор указания обработчику с отведённым обозначением
		::run("<!DOCTYPE a [<?xml?>]><a/>", 12, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::RESERVED_PROCESSING);
		// Выполняем разбор указания обработчику с отведённым обозначением в ином написании
		::run("<!DOCTYPE a [<?XmL?>]><a/>", 12, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::RESERVED_PROCESSING);
	}
	/**
	 * Объявление обозначения без указания источника
	 *
	 * @details Договор велит обозначению нести источник - общедоступный либо внешний, - и
	 *          объявление без него отвергается. Набор объявлений обозначений подавал лишь
	 *          построенные верно, и дорога отказа не проходилась ни разу
	 */
	{
		/**
		 * Выполняем перебор устройств разбора пространств имён
		 *
		 * @warning Разбор с ОТКЛЮЧЁННЫМИ пространствами имён здесь существенен: сличение
		 *          имени с разделителем префикса при них отпадает, и проверка источника
		 *          разбирается своею дорогой. Дорога эта набором не проходилась ни разу
		 */
		for(auto & namespaces : vector <uint8_t> {1, 0}){
			// Настройки разбора текста разметки
			xml::reader_t::settings_t settings;
			// Задаём устройство разбора пространств имён
			settings.namespaces = (namespaces != 0);
			// Выполняем перебор объявлений обозначения без источника
			for(auto & text : vector <string> {
				// Объявление, оборванное сразу за именем
				"<!DOCTYPE a [<!NOTATION n]><a/>",
				// Объявление, закрытое сразу за именем
				"<!DOCTYPE a [<!NOTATION n>]><a/>",
				// Объявление с видом источника, но без него самого
				"<!DOCTYPE a [<!NOTATION n SYSTEM>]><a/>"
			}){
				// Выполняем разбор текста разметки
				::run(text, 0, error, settings);
				// Выполняем проверку кода ошибки разбора
				ASSERT_EQ(error, xml::error_t::INVALID_DOCTYPE) << text << " " << static_cast <uint32_t> (namespaces);
			}
			// Выполняем разбор объявления обозначения, построенного верно
			::run("<!DOCTYPE a [<!NOTATION n SYSTEM \"s\">]><a/>", 0, error, settings);
			// Выполняем проверку отсутствия ошибки разбора
			ASSERT_EQ(error, xml::error_t::NONE) << static_cast <uint32_t> (namespaces);
		}
	}
	/**
	 * Пустая ссылка в содержимом узла при объявленных сущностях
	 *
	 * @details Просмотр разметки сличает ссылку с объявленными сущностями, и пустое имя
	 *          отсеивается прежде поиска - наравне с числовой ссылкой. Набор пустых ссылок
	 *          при НЕПУСТОМ перечне сущностей не подавал, и отсев этот не выполнялся
	 */
	{
		// Выполняем разбор пустой ссылки при объявленной сущности
		::run("<!DOCTYPE a [<!ENTITY e \"Z\">]><a>&;</a>", 0, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::INVALID_REFERENCE);
		// Выполняем разбор числовой ссылки при объявленной сущности
		const string record = ::run("<!DOCTYPE a [<!ENTITY e \"Z\">]><a>&#65;&e;</a>", 0, error);
		// Выполняем проверку отсутствия ошибки разбора
		ASSERT_EQ(error, xml::error_t::NONE);
		// Выполняем проверку подстановки обеих ссылок
		ASSERT_NE(record.find("AZ"), string::npos) << record;
	}
}

/**
 * @brief Проверка повторного просмотра значения сущности, несущего голый знак ссылки
 *
 * @details Объявление сущности проверяется при разборе описания типа документа, и
 *          негодная ссылка внутри значения там же и отвергается. Однако числовая ссылка
 *          при объявлении ПОДСТАВЛЯЕТСЯ, и записанное `&#38;` оставляет в сохранённом
 *          значении голый знак ссылки, ссылкою не являющийся. Дороги, ведущие к разбору
 *          такого значения, проверкою при объявлении не прикрыты вовсе - оно построено
 *          верно, и негодным становится лишь после подстановки
 *
 * @note Дороги эти отыскал ворошитель, пущенный по объектным файлам с покрытием: перебор
 *       предположений их не брал, поскольку требуют они не негодной записи, а записи
 *       ВЕРНОЙ, обращающейся негодной при подстановке
 *
 */
TEST(CodecXmlReader, EntityValueRescanAfterCharReference) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	/**
	 * Голый знак ссылки в значении сущности, употреблённой в содержимом и в атрибуте
	 */
	for(auto & text : vector <string> {
		// Значение, оканчивающееся ссылкой без завершителя
		"<!DOCTYPE a [<!ENTITY e \"&#38;xy\">]><a>&e;</a>",
		// То же значение в значении атрибута
		"<!DOCTYPE a [<!ENTITY e \"&#38;xy\">]><a b=\"&e;\"/>",
		// Значение с пустым именем ссылки
		"<!DOCTYPE a [<!ENTITY e \"&#38;;\">]><a>&e;</a>",
		// То же значение в значении атрибута
		"<!DOCTYPE a [<!ENTITY e \"&#38;;\">]><a b=\"&e;\"/>",
		// Значение по умолчанию, оканчивающееся ссылкой без завершителя
		"<!DOCTYPE a [<!ATTLIST a b CDATA \"&#38;xy\">]><a/>",
		// Значение по умолчанию с пустым именем ссылки
		"<!DOCTYPE a [<!ATTLIST a b CDATA \"&#38;;\">]><a/>"
	}){
		// Выполняем разбор текста разметки
		::run(text, 0, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::INVALID_REFERENCE) << text;
	}
	/**
	 * Значение по умолчанию, ведущее к отказу подстановки ВЛОЖЕННОЙ ссылки
	 *
	 * @warning Двух уровней здесь требует само закрепляемое место: отказ поиска сущности
	 *          верхнего уровня разбирается иною дорогой, и однослойное объявление до
	 *          закрепляемого отказа не доходит
	 */
	{
		// Выполняем разбор объявления со ссылкой на неизвестную сущность внутри значения
		::run("<!DOCTYPE a [<!ENTITY z \"&#38;q;\"><!ATTLIST a b CDATA \"&#38;z;\">]><a/>", 0, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::UNKNOWN_ENTITY);
	}
	/**
	 * Перенос признака разметки прекращается на ссылке без завершителя
	 *
	 * @note Проверяется здесь не отказ, а ПРОДОЛЖЕНИЕ разбора: голый знак ссылки в
	 *       значении одной сущности не должен мешать переносу признака разметки у другой
	 */
	{
		// Выполняем разбор объявления с голым знаком ссылки рядом с несущей разметку
		const string record = ::run("<!DOCTYPE a [<!ENTITY e \"&#38;x\"><!ENTITY m \"<b/>\">]><a>&m;</a>", 0, error);
		// Выполняем проверку отсутствия ошибки разбора
		ASSERT_EQ(error, xml::error_t::NONE);
		// Выполняем проверку подстановки разметки из значения сущности
		ASSERT_NE(record.find("<b"), string::npos) << record;
	}
}

/**
 * @brief Проверка раздела дословного текста, открытого значением сущности
 *
 * @details Раздел, открытый внутри подставленного значения и не закрытый в нём же,
 *          пересекает границу подстановки: начало его приходит из объявления сущности, а
 *          конец из самого текста, и построение документа зависело бы от того, чем
 *          сущность объявлена. Договор такого не дозволяет
 *
 * @warning Знак, недопустимый в разметке, следом за возвратом каретки здесь СУЩЕСТВЕНЕН,
 *          и подобран он ворошителем: без него разбор прекращается прежде, на негодном
 *          построении самого раздела, и отказ приходит иной. Отвергаются оба построения,
 *          и разница тут лишь в том, какой отказ поспевает первым
 *
 */
TEST(CodecXmlReader, CdataCrossingEntityBoundary) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	// Собираем текст разметки со знаком, недопустимым в разметке
	string text("<!DOCTYPE a [<!ENTITY e \"<![CDATA[z\">]><a>&e;a\r");
	// Выполняем добавление недопустимого знака
	text.push_back('\x0c');
	// Выполняем разбор текста разметки
	::run(text, 0, error);
	// Выполняем проверку кода ошибки разбора
	ASSERT_EQ(error, xml::error_t::ENTITY_BOUNDARY);
	/**
	 * Раздел, открытый значением сущности, отвергается и без недопустимого знака
	 */
	{
		// Выполняем разбор текста разметки без недопустимого знака
		::run("<!DOCTYPE a [<!ENTITY e \"<![CDATA[z\">]><a>&e;</a>", 0, error);
		// Выполняем проверку отказа разбора
		ASSERT_NE(error, xml::error_t::NONE);
	}
}

/**
 * @brief Проверка предела объёма события у отделяемого пробельного содержимого
 *
 * @details Пробельное содержимое, отделяемое настройками в своё событие, копится до
 *          конца наравне со склеиванием кусков, и предел объёма события обязан держаться
 *          и на нём: иначе он обходился бы простым отделением пробелов
 *
 * @warning Подача КУСКАМИ здесь существенна: текст, пришедший целиком, копить не
 *          приходится вовсе, и до места накопления разбор не доходит - предел там
 *          проверяется иначе. Дорога эта набором не проходилась ни разу
 *
 */
TEST(CodecXmlReader, SeparatedSpacesEventLimit) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	// Настройки разбора текста разметки
	xml::reader_t::settings_t settings;
	// Задаём отделение пробельного содержимого в своё событие
	settings.separateSpaces = true;
	// Задаём предел объёма события
	settings.maxEvent = 8;
	// Собираем пробельное содержимое, заведомо большее заданного предела
	const string blanks(200, ' ');
	/**
	 * Выполняем перебор построений, за которыми следует пробельное содержимое
	 */
	for(auto & text : vector <string> {
		// Пробелы, завершаемые узлом
		("<a>" + blanks + "<b/></a>"),
		// Пробелы, завершаемые текстом
		("<a>" + blanks + "x</a>")
	}){
		// Выполняем разбор текста разметки кусками по четыре байта
		::run(text, 4, error, settings);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::OVERFLOW_LIMIT) << text;
	}
	/**
	 * Предел держится и при подаче текста целиком
	 *
	 * @note Дорога тут иная, и сличение это закрепляет независимость предела от нарезки
	 */
	{
		// Выполняем разбор текста разметки целиком
		::run(("<a>" + blanks + "<b/></a>"), 0, error, settings);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::OVERFLOW_LIMIT);
	}
	/**
	 * Пробельное содержимое, не отделяемое настройками, предела не нарушает
	 */
	{
		// Задаём слияние пробельного содержимого с прочим
		settings.separateSpaces = false;
		// Выполняем разбор текста разметки кусками по четыре байта
		::run(("<a>" + blanks + "<b/></a>"), 4, error, settings);
		// Выполняем проверку отсутствия ошибки разбора
		ASSERT_EQ(error, xml::error_t::NONE);
	}
}

/**
 * @brief Проверка предела вложенности описания строения узла
 *
 * @details Описание строения узла разбирается вглубь: скобка внутри скобки уводит разбор
 *          на новый уровень, и предел вложенности здесь свой - тот же, что и у самих
 *          узлов разметки. Без него подставной текст уводил бы разбор в возвратность
 *          такой глубины, какую задаст сам, и стек кончался бы прежде разбора
 *
 * @note Предел этот набор не проходил ни разу: описания строения в нём мелкие, а разбор
 *       вглубь ошибки не даёт вплоть до тысячи с лишним уровней
 *
 */
TEST(CodecXmlReader, SubsetChildrenDepthLimit) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	/**
	 * Выполняем перебор глубин вложенности описания строения узла
	 */
	for(auto & depth : vector <uint32_t> {512, 1100}){
		// Собираемое описание строения узла
		string model;
		/**
		 * Выполняем сборку открывающих скобок описания строения
		 */
		for(uint32_t i = 0; i < depth; i++)
			// Выполняем добавление очередной открывающей скобки
			model.append("(");
		// Выполняем добавление имени узла к описанию строения
		model.append("b");
		/**
		 * Выполняем сборку закрывающих скобок описания строения
		 */
		for(uint32_t i = 0; i < depth; i++)
			// Выполняем добавление очередной закрывающей скобки
			model.append(")");
		// Выполняем разбор текста разметки с собранным описанием строения
		::run(("<!DOCTYPE a [<!ELEMENT a " + model + ">]><a><b/></a>"), 0, error);
		/**
		 * Если глубина вложенности предела не превышает
		 */
		if(depth < 1024)
			// Выполняем проверку отсутствия ошибки разбора
			ASSERT_EQ(error, xml::error_t::NONE) << depth;
		// Выполняем проверку отказа разбора при превышении предела
		else ASSERT_EQ(error, xml::error_t::INVALID_DOCTYPE) << depth;
	}
}

/**
 * @brief Проверка отказов подстановки объявленных сущностей
 *
 * @details Значение сущности подставляется не как есть, а разбирается заново: ссылки
 *          внутри него раскрываются тою же дорогой, что и ссылки в самом тексте. Оттого
 *          негодная ссылка ВНУТРИ значения обнаруживается лишь при употреблении
 *          сущности, а не при её объявлении, - объявление проходит без единой жалобы
 *
 * @note Отказ приходит при УПОТРЕБЛЕНИИ, и это стоит отдельной проверки: сущность,
 *       объявленную и ни разу не употреблённую, разбор пропускает, и негодная ссылка в
 *       ней остаётся необнаруженной законно
 *
 */
TEST(CodecXmlReader, EntityExpansionRefusals) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	/**
	 * Ссылка на неизвестную сущность внутри значения объявленной
	 */
	{
		// Выполняем разбор текста разметки с употреблением сущности
		::run("<!DOCTYPE a [<!ENTITY e '&nope;'>]><a>&e;</a>", 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::UNKNOWN_ENTITY);
		/**
		 * Та же сущность, ни разу не употреблённая, отказа НЕ вызывает
		 *
		 * @warning Случай этот обязателен: обнаружь разбор негодную ссылку при
		 *          объявлении, документ отвергался бы за сущность, к нему не
		 *          относящуюся вовсе
		 */
		::run("<!DOCTYPE a [<!ENTITY e '&nope;'>]><a>x</a>", 4096, error);
		// Выполняем проверку отсутствия ошибки разбора
		ASSERT_EQ(error, xml::error_t::NONE);
	}
	/**
	 * Сущность, ссылающаяся сама на себя
	 */
	{
		// Выполняем разбор текста разметки с самоссылающейся сущностью
		::run("<!DOCTYPE a [<!ENTITY e '&e;'>]><a>&e;</a>", 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::RECURSIVE_ENTITY);
	}
	/**
	 * Обозначение параметрической сущности в теле документа сущностью НЕ является
	 *
	 * @note Знак процента в содержимом узла - обычный знак, и подстановке он не
	 *       подлежит: параметрические сущности живут лишь внутри описания типа
	 *       документа
	 */
	{
		// Выполняем разбор текста разметки со знаком процента в содержимом
		const string result = ::run("<!DOCTYPE a [<!ENTITY % e 'v'>]><a>%e;</a>", 4096, error);
		// Выполняем проверку отсутствия ошибки разбора
		ASSERT_EQ(error, xml::error_t::NONE);
		// Выполняем проверку сохранения знака процента в содержимом
		ASSERT_NE(result.find("%e;"), string::npos) << result;
	}
}

/**
 * @brief Проверка построения атрибутов и умолчаний из описания типа документа
 *
 * @details Пробельные знаки вокруг знака равенства договором дозволены, и разбор обязан
 *          принимать их с обеих сторон, включая перевод строки. Запрет их означал бы
 *          отказ на разметке, записанной в несколько строк, - а именно так её и пишут,
 *          когда атрибутов много
 *
 * @note Умолчание, объявленное описанием для ОБЪЯВЛЕНИЯ ПРОСТРАНСТВА ИМЁН, сличается не
 *       с обычными атрибутами узла, а с объявлениями обозначений: хранятся они порознь,
 *       и сличение с одними лишь атрибутами затирало бы записанное в тексте обозначение
 *       умолчанием из описания
 *
 * @warning Значение умолчания раскрывается ССЫЛКАМИ, как и всякое значение атрибута, и
 *          негодная ссылка в нём обнаруживается при подстановке умолчания, а не при
 *          объявлении
 *
 */
TEST(CodecXmlReader, AttributeSpacingAndNamespaceDefaults) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	/**
	 * Пробельные знаки вокруг знака равенства отказа не вызывают
	 */
	{
		// Запись событий разбора, снятая с записи без пробельных знаков
		const string plain = ::run("<a x=\"1\"/>", 4096, error);
		// Выполняем проверку отсутствия ошибки разбора
		ASSERT_EQ(error, xml::error_t::NONE);
		/**
		 * Выполняем перебор записей с пробельными знаками вокруг знака равенства
		 */
		for(auto & text : vector <string> {
			// Пробел перед знаком равенства
			"<a x =\"1\"/>",
			// Пробел после знака равенства
			"<a x= \"1\"/>",
			// Пробелы с обеих сторон знака равенства
			"<a x = \"1\"/>",
			// Перевод строки с обеих сторон знака равенства
			"<a x\n=\n\"1\"/>"
		}){
			// Выполняем разбор текста разметки
			const string result = ::run(text, 4096, error);
			// Выполняем проверку отсутствия ошибки разбора
			ASSERT_EQ(error, xml::error_t::NONE) << text;
			// Выполняем проверку совпадения разбора с записью без пробельных знаков
			ASSERT_EQ(result, plain) << text;
		}
	}
	/**
	 * Умолчание для объявления пространства имён не затирает записанное в тексте
	 */
	{
		// Выполняем разбор текста разметки с записанным объявлением обозначения
		const string result = ::run("<!DOCTYPE a [<!ATTLIST a xmlns:p CDATA 'u'>]>"
		                            "<a xmlns:p='v'><p:b/></a>", 4096, error);
		// Выполняем проверку отсутствия ошибки разбора
		ASSERT_EQ(error, xml::error_t::NONE);
		// Выполняем проверку сохранения записанного пространства имён
		ASSERT_NE(result.find("{v}"), string::npos) << result;
		// Выполняем проверку отсутствия пространства имён из умолчания
		ASSERT_EQ(result.find("{u}"), string::npos) << result;
	}
	/**
	 * Умолчание для объявления пространства имён подставляется незаписанному
	 */
	{
		// Выполняем разбор текста разметки без записанного объявления обозначения
		const string result = ::run("<!DOCTYPE a [<!ATTLIST a xmlns:p CDATA 'u'>]>"
		                            "<a><p:b/></a>", 4096, error);
		// Выполняем проверку отсутствия ошибки разбора
		ASSERT_EQ(error, xml::error_t::NONE);
		// Выполняем проверку подстановки пространства имён из умолчания
		ASSERT_NE(result.find("{u}"), string::npos) << result;
	}
	/**
	 * Ссылка на сущность внутри значения умолчания раскрывается
	 */
	{
		// Выполняем разбор текста разметки с годной ссылкой в умолчании
		const string result = ::run("<!DOCTYPE a [<!ENTITY e 'v'>"
		                            "<!ATTLIST a x CDATA '&e;'>]><a/>", 4096, error);
		// Выполняем проверку отсутствия ошибки разбора
		ASSERT_EQ(error, xml::error_t::NONE);
		// Выполняем проверку раскрытия ссылки в значении умолчания
		ASSERT_NE(result.find("x=v"), string::npos) << result;
		// Выполняем разбор текста разметки с негодной ссылкой в умолчании
		::run("<!DOCTYPE a [<!ATTLIST a x CDATA '&nope;'>]><a/>", 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::UNKNOWN_ENTITY);
	}
}

/**
 * @brief Проверка пределов, задаваемых настройками разбора
 *
 * @details Предел размера события стережёт ОДНО событие, а не весь разбираемый текст:
 *          содержимое узла выдаётся частями, и текст любой длины проходит, покуда каждая
 *          выданная часть в предел укладывается. Оттого один и тот же текст при малом
 *          куске подачи разбирается без отказа, а при крупном отвергается, - и это не
 *          зависимость разбора от нарезки, а прямое следствие того, что предел положен
 *          событию
 *
 * @warning Проверяется здесь ИМЕННО ЭТО различие, а не сам отказ: проверка, подающая
 *          текст одним размером куска, не отличила бы предел, положенный событию, от
 *          предела, положенного всему тексту, - а последний отвергал бы длинные
 *          документы целиком
 *
 */
TEST(CodecXmlReader, SettingsLimits) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	// Собираемое содержимое узла, заведомо большее предела события
	string content;
	/**
	 * Выполняем сборку содержимого узла
	 */
	for(uint32_t i = 0; i < 300; i++)
		// Выполняем добавление очередного звена содержимого
		content.append("0123456789");
	// Собранный текст разметки с длинным содержимым узла
	const string text = ("<a>" + content + "</a>");
	/**
	 * Дорога первая: кусок подачи меньше предела события
	 */
	{
		// Настройки разбора с пределом размера события
		xml::reader_t::settings_t settings;
		// Выполняем задание предела размера события
		settings.maxEvent = 64;
		// Выполняем разбор текста разметки малыми кусками
		::run(text, 32, error, settings);
		// Выполняем проверку отсутствия ошибки разбора
		ASSERT_EQ(error, xml::error_t::NONE);
	}
	/**
	 * Дорога вторая: кусок подачи больше предела события
	 */
	{
		// Настройки разбора с пределом размера события
		xml::reader_t::settings_t settings;
		// Выполняем задание предела размера события
		settings.maxEvent = 64;
		/**
		 * Выполняем перебор размеров куска подачи, превышающих предел
		 */
		for(const size_t step : {static_cast <size_t> (96), static_cast <size_t> (256),
		                         text.size() + 1}){
			// Выполняем разбор текста разметки
			::run(text, step, error, settings);
			// Выполняем проверку кода ошибки разбора
			ASSERT_EQ(error, xml::error_t::OVERFLOW_LIMIT) << "кусок " << step;
		}
	}
	/**
	 * Предел, снятый настройками, отказа не вызывает вовсе
	 */
	{
		// Настройки разбора без предела размера события
		xml::reader_t::settings_t settings;
		// Выполняем снятие предела размера события
		settings.maxEvent = 0;
		// Выполняем разбор текста разметки крупным куском
		::run(text, text.size() + 1, error, settings);
		// Выполняем проверку отсутствия ошибки разбора
		ASSERT_EQ(error, xml::error_t::NONE);
	}
	/**
	 * Предел глубины вложенности узлов
	 */
	{
		// Настройки разбора с пределом глубины вложенности
		xml::reader_t::settings_t settings;
		// Выполняем задание предела глубины вложенности
		settings.maxDepth = 3;
		// Выполняем разбор текста разметки глубже предела
		::run("<a><b><c><d><e/></d></c></b></a>", 4096, error, settings);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::DEPTH_EXCEEDED);
		// Выполняем разбор текста разметки в пределах дозволенного
		::run("<a><b><c/></b></a>", 4096, error, settings);
		// Выполняем проверку отсутствия ошибки разбора
		ASSERT_EQ(error, xml::error_t::NONE);
	}
}

/**
 * @brief Проверка ссылок внутри дословных значений объявлений подмножества
 *
 * @details Дословное значение объявления - не просто отрезок текста между кавычками:
 *          ссылки внутри него разбираются при самом объявлении, и негодная ссылка
 *          отвергает объявление целиком. Все нарушения отвечают одним кодом
 *          `INVALID_DOCTYPE`, и различить их по нему потребитель не может, - оттого
 *          проверяется здесь не код, а то, что разбор ни одного из них не пропускает
 *
 * @warning Пропусти разбор негодную ссылку, объявление легло бы в перечень сущностей с
 *          покалеченным значением, и подстановка его отдала бы потребителю НЕ ТО, что
 *          записано, - молча, без единого признака отказа
 *
 * @note Годные ссылки тех же двух видов - на знак по коду и на объявленную сущность -
 *       проверяются рядом: проверка, сплошь состоящая из отказов, проходит и при
 *       разборе, отвергающем всякую ссылку подряд
 *
 */
TEST(CodecXmlReader, SubsetLiteralReferences) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	/**
	 * Выполняем перебор негодных ссылок внутри дословных значений
	 */
	for(auto & text : vector <string> {
		// Ссылка на знак по коду с негодными шестнадцатеричными цифрами
		"<!DOCTYPE a [<!ENTITY e '&#xZZ;'>]><a/>",
		// Ссылка на знак по коду за пределом области значений
		"<!DOCTYPE a [<!ENTITY e '&#x110000;'>]><a/>",
		// Ссылка на сущность без точки с запятой
		"<!DOCTYPE a [<!ENTITY e '&amp'>]><a/>",
		// Ссылка на сущность с именем, начатым цифрой
		"<!DOCTYPE a [<!ENTITY e '&1x;'>]><a/>",
		// Дословное значение без закрывающей кавычки
		"<!DOCTYPE a [<!ENTITY e 'xy>]><a/>",
		/**
		 * Те же нарушения внутри значения атрибута по умолчанию
		 *
		 * @warning Разбор дословного значения разведён по ВИДУ объявления, и ссылки
		 *          внутри значения атрибута разбираются иным участком, нежели ссылки
		 *          внутри значения сущности. Проверка, взявшая один лишь вид, оставляла
		 *          бы второй участок непроверенным целиком - при том что код отказа у
		 *          обоих один и тот же, и по нему различия не видно
		 */
		// Ссылка на знак по коду с негодными шестнадцатеричными цифрами
		"<!DOCTYPE a [<!ATTLIST a x CDATA '&#xZZ;'>]><a/>",
		// Ссылка на знак по коду за пределом области значений
		"<!DOCTYPE a [<!ATTLIST a x CDATA '&#x110000;'>]><a/>",
		// Ссылка на сущность без точки с запятой
		"<!DOCTYPE a [<!ATTLIST a x CDATA '&amp'>]><a/>",
		// Ссылка на сущность с именем, начатым цифрой
		"<!DOCTYPE a [<!ATTLIST a x CDATA '&1x;'>]><a/>",
		// Знак «меньше» внутри значения атрибута по умолчанию
		"<!DOCTYPE a [<!ATTLIST a x CDATA '<'>]><a/>",
		// Значение атрибута по умолчанию без закрывающей кавычки
		"<!DOCTYPE a [<!ATTLIST a x CDATA 'xy>]><a/>"
	}){
		/**
		 * Выполняем разбор двумя размерами куска подачи
		 */
		for(const size_t step : {static_cast <size_t> (1), text.size() + 1}){
			// Выполняем разбор текста разметки
			::run(text, step, error);
			// Выполняем проверку кода ошибки разбора
			ASSERT_EQ(error, xml::error_t::INVALID_DOCTYPE) << text << " кусок " << step;
		}
	}
	/**
	 * Годная ссылка на знак по коду раскрывается при подстановке
	 */
	{
		// Выполняем разбор текста разметки со ссылкой на знак по коду
		const string result = ::run("<!DOCTYPE a [<!ENTITY e '&#x41;'>]><a>&e;</a>", 4096, error);
		// Выполняем проверку отсутствия ошибки разбора
		ASSERT_EQ(error, xml::error_t::NONE);
		// Выполняем проверку раскрытия ссылки на знак по коду
		ASSERT_NE(result.find('A'), string::npos) << result;
	}
	/**
	 * Годная ссылка на знак по коду в значении атрибута раскрывается
	 */
	{
		// Выполняем разбор текста разметки со ссылкой в значении атрибута
		::run("<!DOCTYPE a [<!ATTLIST a x CDATA '&#x41;'>]><a/>", 4096, error);
		// Выполняем проверку отсутствия ошибки разбора
		ASSERT_EQ(error, xml::error_t::NONE);
	}
	/**
	 * Годная ссылка на встроенную сущность раскрывается при подстановке
	 */
	{
		// Выполняем разбор текста разметки со ссылкой на встроенную сущность
		const string result = ::run("<!DOCTYPE a [<!ENTITY e '&amp;'>]><a>&e;</a>", 4096, error);
		// Выполняем проверку отсутствия ошибки разбора
		ASSERT_EQ(error, xml::error_t::NONE);
		// Выполняем проверку раскрытия ссылки на встроенную сущность
		ASSERT_NE(result.find('&'), string::npos) << result;
	}
}

/**
 * @brief Проверка отказов на негодных именах в объявлениях подмножества
 *
 * @details Имя в объявлении подмножества сличается с правилами построения имён, и
 *          отвергается не только ОТСУТСТВУЮЩЕЕ имя, но и негодно ПОСТРОЕННОЕ - начатое
 *          цифрой либо знаком переноса. Различие это существенно: отсутствие имени
 *          ловится сличением с пробельными знаками задолго до разбора самого имени, и
 *          проверка, взявшая одни лишь пропуски, оставляет разбор имени непроверенным
 *
 * @note Проверяются здесь три объявления - узла, перечня атрибутов и самого атрибута
 *       внутри перечня, - потому что имя в каждом из них разбирается ОТДЕЛЬНЫМ вызовом,
 *       и отказ одного о прочих не говорит ничего
 *
 * @warning Пропусти разбор негодное имя, объявление легло бы в описание с именем, какое
 *          в тексте разметки встретиться не может вовсе, - и умолчания его не
 *          подставились бы ни одному узлу, молча и без признака отказа
 *
 */
TEST(CodecXmlReader, SubsetDeclarationNames) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	/**
	 * Выполняем перебор объявлений с негодно построенными именами
	 */
	for(auto & text : vector <string> {
		// Имя объявляемого узла, начатое цифрой
		"<!DOCTYPE a [<!ELEMENT 1a EMPTY>]><a/>",
		// Имя объявляемого узла, начатое знаком переноса
		"<!DOCTYPE a [<!ELEMENT -a EMPTY>]><a/>",
		// Имя узла в перечне атрибутов, начатое цифрой
		"<!DOCTYPE a [<!ATTLIST 1a x CDATA #IMPLIED>]><a/>",
		// Имя объявляемого атрибута, начатое цифрой
		"<!DOCTYPE a [<!ATTLIST a 1x CDATA #IMPLIED>]><a/>",
		// Имя объявляемого атрибута, начатое знаком переноса
		"<!DOCTYPE a [<!ATTLIST a -x CDATA #IMPLIED>]><a/>",
		// Два объявления атрибутов, не разделённые пробельным знаком
		"<!DOCTYPE a [<!ATTLIST a x CDATA #IMPLIEDy CDATA #IMPLIED>]><a/>"
	}){
		/**
		 * Выполняем разбор двумя размерами куска подачи
		 */
		for(const size_t step : {static_cast <size_t> (1), text.size() + 1}){
			// Выполняем разбор текста разметки
			::run(text, step, error);
			// Выполняем проверку кода ошибки разбора
			ASSERT_EQ(error, xml::error_t::INVALID_DOCTYPE) << text << " кусок " << step;
		}
	}
	/**
	 * Те же объявления с именами, построенными по правилам, отказа не вызывают
	 */
	for(auto & text : vector <string> {
		// Объявление узла с именем по правилам
		"<!DOCTYPE a [<!ELEMENT a EMPTY>]><a/>",
		// Объявление перечня атрибутов с именем узла по правилам
		"<!DOCTYPE a [<!ATTLIST a x CDATA #IMPLIED>]><a/>",
		// Два объявления атрибутов, разделённые пробельным знаком
		"<!DOCTYPE a [<!ATTLIST a x CDATA #IMPLIED y CDATA #IMPLIED>]><a/>"
	}){
		// Выполняем разбор текста разметки
		::run(text, 4096, error);
		// Выполняем проверку отсутствия ошибки разбора
		ASSERT_EQ(error, xml::error_t::NONE) << text;
	}
}

/**
 * @brief Проверка заповедных обозначений пространств имён
 *
 * @details Договор о пространствах имён закрепляет два обозначения за собою: `xmlns` не
 *          может быть объявлено обозначением вовсе, а `xml` связано навсегда с одним
 *          определённым адресом и объявлению с иным адресом не подлежит. Обратное тоже
 *          запрещено: заповедный адрес нельзя связать с обозначением произвольным
 *
 * @note Проверяется здесь и ЗАКОННАЯ сторона каждого запрета: объявление `xml` с
 *       положенным ему адресом дозволено, отмена пространства имён по умолчанию пустым
 *       адресом дозволена, а объявление обычного обозначения пустым адресом - нет.
 *       Различие последних двух легко упустить, и запрет их обоих выглядел бы
 *       последовательным, будучи неверным
 *
 * @warning Пропусти разбор объявление `xml` с чужим адресом, узлы с этим обозначением
 *          разошлись бы по двум разным пространствам имён - тому, что записан, и тому,
 *          что подразумевается договором, - и сличение имён давало бы разный исход в
 *          зависимости от того, чем оно велось
 *
 */
TEST(CodecXmlReader, ReservedNamespacePrefixes) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	/**
	 * Объявления, запрещённые договором о пространствах имён
	 */
	{
		// Обозначение `xmlns` объявлению не подлежит вовсе
		::run("<a xmlns:xmlns='u'/>", 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::RESERVED_PREFIX);
		// Обозначение `xml` не подлежит объявлению с чужим адресом
		::run("<a xmlns:xml='u'/>", 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::RESERVED_PREFIX);
		// Заповедный адрес не подлежит связыванию с обычным обозначением
		::run("<a xmlns:p='http://www.w3.org/XML/1998/namespace'/>", 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::RESERVED_PREFIX);
	}
	/**
	 * Обычное обозначение пустым адресом объявлению не подлежит
	 */
	{
		// Выполняем разбор текста разметки с пустым адресом обозначения
		::run("<a xmlns:p=''><p:b/></a>", 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::INVALID_NAMESPACE);
	}
	/**
	 * Законная сторона тех же запретов
	 */
	for(auto & text : vector <string> {
		// Объявление обозначения `xml` с положенным ему адресом
		"<a xmlns:xml='http://www.w3.org/XML/1998/namespace'/>",
		// Отмена пространства имён по умолчанию пустым адресом
		"<a xmlns=''/>",
		// Отмена пространства имён по умолчанию у вложенного узла
		"<a xmlns='u'><b xmlns=''/></a>",
		// Объявление обычного обозначения по правилам
		"<a xmlns:p='u'><p:b/></a>"
	}){
		// Выполняем разбор текста разметки
		::run(text, 4096, error);
		// Выполняем проверку отсутствия ошибки разбора
		ASSERT_EQ(error, xml::error_t::NONE) << text;
	}
}

/**
 * @brief Проверка отказов на тексте, оборванном посреди разметки
 *
 * @details Текст, оборванный посреди метки, отличается от текста, поданного не целиком,
 *          одним лишь признаком последнего куска: покуда куски ещё ожидаются, разбор
 *          отвечает требованием следующего, а получив последний - отказом. Проверяется
 *          здесь ИМЕННО отказ на последнем куске, потому что молчаливое согласие тут
 *          означало бы, что оборванный документ разобран как целый
 *
 * @note Обрыв на каждом построении отвечает СВОИМ кодом, и коды эти проверяются
 *       поимённо: незакрытая метка узла, незавершённое указание обработчику и
 *       незавершённое примечание - три разных нарушения, и сведение их к одному коду
 *       лишило бы потребителя возможности отличить одно от другого
 *
 * @warning Раздел дословного текста, оборванный концом текста, отвечает НЕ «незакрытой
 *          меткой», а содержимым вне корневого узла: раздел этот открывается прежде
 *          корневого узла и оттого встречает заслон корня раньше, чем заслон
 *          завершённости. Различие намеренное и проверкой закреплено
 *
 */
TEST(CodecXmlReader, TruncatedMarkupRefusals) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	/**
	 * Обрыв текста посреди метки узла отвечает незакрытой меткой
	 */
	for(auto & text : vector <string> {
		// Обрыв сразу за именем узла
		"<a",
		// Обрыв за именем атрибута
		"<a x",
		// Обрыв за знаком равенства
		"<a x=",
		// Обрыв за значением атрибута
		"<a x='1'",
		// Обрыв посреди закрывающей метки
		"</a"
	}){
		// Выполняем разбор текста разметки
		::run(text, 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::UNCLOSED_TAG) << text;
	}
	/**
	 * Обрыв прочих построений отвечает своим кодом у каждого
	 */
	{
		// Обрыв посреди указания обработчику
		::run("<?x", 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::INVALID_PROCESSING);
		// Обрыв посреди примечания
		::run("<!--x", 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::INVALID_COMMENT);
		// Обрыв посреди раздела дословного текста прежде корневого узла
		::run("<![CDATA[x", 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::CONTENT_OUTSIDE_ROOT);
	}
	/**
	 * Ошибочное построение метки отвечает ошибкой построения
	 */
	for(auto & text : vector <string> {
		// Объявление с именем, не отвечающим ни одному известному
		"<!x>",
		// Объявление без имени вовсе
		"<!>",
		// Объявление с именем, оборванным на первом знаке
		"<!D>",
		// Условный раздел с неизвестным словом
		"<![X["
	}){
		// Выполняем разбор текста разметки
		::run(text, 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::INVALID_TAG) << text;
	}
	/**
	 * Раздел дословного текста, пришедший подстановкой сущности, разбирается
	 *
	 * @note Случай законный: раздел приходит целиком внутри одного значения и границы
	 *       подстановки не пересекает
	 */
	{
		// Выполняем разбор текста разметки с разделом внутри значения сущности
		::run("<!DOCTYPE a [<!ENTITY e '<![CDATA[x]]>'>]><a>&e;</a>", 4096, error);
		// Выполняем проверку отсутствия ошибки разбора
		ASSERT_EQ(error, xml::error_t::NONE);
	}
}

/**
 * @brief Проверка предела глубины подстановки сущностей, несущих разметку
 *
 * @details Сущность, значение которой содержит разметку, подставляется ИНАЧЕ, нежели
 *          сущность со значением из одного текста: текст раскрывается на месте, а
 *          разметка вставляется в разбираемый поток и разбирается заново, порождая
 *          события открытия и закрытия узлов. Оттого предел вложенности стережётся у
 *          обеих дорог порознь, и проверка одной о другой не говорит ничего
 *
 * @note Подстановка разметки сама по себе стережётся проверкой `MarkupEntityInjection`,
 *       и там же стережётся предел ОБЪЁМА подстановки вместе с оградой рекурсии. Здесь
 *       стережётся ГЛУБИНА, и это иной предел: объём считается по всему документу разом,
 *       а глубина - по одной цепочке. Цепочка нарочно составлена из значений вида
 *       `<b>&eN;</b>`, чтобы каждое звено порождало узел, а не один лишь текст
 *
 * @note Проверка вложенности через ЗНАЧЕНИЯ стережётся отдельно проверкой
 *       `EntityDepthAndSubsetRefusals`: дороги эти разные, и предел у каждой свой
 *
 * @warning Без предела цепочка сущностей, ссылающихся одна на другую через разметку,
 *          раскрывалась бы вглубь до исчерпания памяти, и документ на несколько сотен
 *          октетов клал бы разбор
 *
 */
TEST(CodecXmlReader, MarkupEntityDepth) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	/**
	 * Дорога первая: цепочка подстановок разметкой глубже предела
	 */
	{
		// Собираемый текст описания типа документа
		string text = "<!DOCTYPE a [";
		// Число звеньев цепочки подстановок с запасом сверх предела
		const uint32_t count = (xml::MAX_ENTITY_DEPTH + 8);
		/**
		 * Выполняем сборку цепочки сущностей, несущих разметку
		 */
		for(uint32_t i = 1; i < count; i++)
			// Выполняем добавление очередного звена цепочки
			text.append("<!ENTITY e").append(std::to_string(i)).append(" '<b>&e")
			    .append(std::to_string(i + 1)).append(";</b>'>");
		// Выполняем завершение цепочки значением без ссылок
		text.append("<!ENTITY e").append(std::to_string(count)).append(" 'x'>]><a>&e1;</a>");
		// Выполняем разбор текста разметки
		::run(text, text.size() + 1, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::ENTITY_DEPTH_EXCEEDED);
	}
	/**
	 * Дорога вторая: та же цепочка в пределах дозволенного
	 *
	 * @note Случай этот доказывает, что отказ выше вызван ПРЕДЕЛОМ, а не самой
	 *       подстановкой разметки
	 */
	{
		// Собираемый текст описания типа документа
		string text = "<!DOCTYPE a [";
		/**
		 * Выполняем сборку короткой цепочки сущностей
		 */
		for(uint32_t i = 1; i < 6; i++)
			// Выполняем добавление очередного звена цепочки
			text.append("<!ENTITY e").append(std::to_string(i)).append(" '<b>&e")
			    .append(std::to_string(i + 1)).append(";</b>'>");
		// Выполняем завершение цепочки значением без ссылок
		text.append("<!ENTITY e6 'x'>]><a>&e1;</a>");
		// Выполняем разбор текста разметки
		const string result = ::run(text, text.size() + 1, error);
		// Выполняем проверку отсутствия ошибки разбора
		ASSERT_EQ(error, xml::error_t::NONE);
		// Выполняем проверку порождения узлов подстановкой разметки
		ASSERT_NE(result.find("<b>"), string::npos) << result;
		// Выполняем проверку подстановки значения через всю цепочку
		ASSERT_NE(result.find('x'), string::npos) << result;
	}
	/**
	 * Дорога третья: неизвестная сущность внутри значения, несущего разметку
	 */
	{
		// Выполняем разбор текста разметки с неизвестной сущностью внутри разметки
		::run("<!DOCTYPE a [<!ENTITY e '<b>&nope;</b>'>]><a>&e;</a>", 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::UNKNOWN_ENTITY);
	}
}

/**
 * @brief Проверка навязывания кодировки в разные мгновения разбора
 *
 * @details Навязать кодировку приведению можно лишь покуда разбор текста НЕ ЗАВЕРШЁН:
 *          завершённое приведение навязывания не принимает, и настройки сбрасывают
 *          кодировку в неопределённую. Поведение это существенно для потребителя,
 *          переиспользующего один объект чтения под несколько документов: задай он
 *          настройки, не выполнив сброса, - кодировка молча не применится, и второй
 *          документ будет разобран не тою кодировкой, какую ему задали
 *
 * @note Проверяются здесь ЧЕТЫРЕ мгновения подряд, а не одно: навязывание принимается до
 *       подачи, после первого куска и даже после разбора событий, - и отвергается лишь
 *       по завершении. Проверка одним мгновением не отличила бы «принимается всегда» от
 *       «принимается до завершения»
 *
 * @warning Сброс возвращает возможность навязать кодировку, и это тоже проверяется:
 *          иначе переиспользование объекта чтения было бы невозможно вовсе
 *
 */
TEST(CodecXmlReader, EncodingImpositionTiming) {
	// Настройки разбора с навязанной кодировкой исходного текста
	xml::reader_t::settings_t settings;
	// Выполняем навязывание кодировки исходного текста
	settings.encoding = xml::encoding_t::UTF8;
	/**
	 * Навязывание до подачи текста принимается
	 */
	{
		// Объект потокового чтения текста разметки
		xml::reader_t reader(::logger());
		// Выполняем задание настроек разбора
		reader.settings(settings);
		// Выполняем проверку принятия навязанной кодировки
		ASSERT_EQ(reader.settings().encoding, xml::encoding_t::UTF8);
	}
	/**
	 * Навязывание после подачи куска текста принимается
	 */
	{
		// Объект потокового чтения текста разметки
		xml::reader_t reader(::logger());
		// Подаваемый кусок текста разметки
		const string text = "<a>";
		// Выполняем подачу куска текста разметки
		reader.feed(text.data(), text.size(), false);
		// Выполняем задание настроек разбора
		reader.settings(settings);
		// Выполняем проверку принятия навязанной кодировки
		ASSERT_EQ(reader.settings().encoding, xml::encoding_t::UTF8);
	}
	/**
	 * Навязывание после разбора событий принимается
	 */
	{
		// Объект потокового чтения текста разметки
		xml::reader_t reader(::logger());
		// Подаваемый кусок текста разметки
		const string text = "<a>x";
		// Выполняем подачу куска текста разметки
		reader.feed(text.data(), text.size(), false);
		// Выполняем разбор накопленных событий
		while(reader.next());
		// Выполняем задание настроек разбора
		reader.settings(settings);
		// Выполняем проверку принятия навязанной кодировки
		ASSERT_EQ(reader.settings().encoding, xml::encoding_t::UTF8);
	}
	/**
	 * Навязывание по завершении разбора ОТВЕРГАЕТСЯ и сбрасывает кодировку
	 */
	{
		// Объект потокового чтения текста разметки
		xml::reader_t reader(::logger());
		// Подаваемый текст разметки целиком
		const string text = "<a>x</a>";
		// Выполняем подачу текста разметки целиком
		reader.feed(text.data(), text.size(), true);
		// Выполняем разбор накопленных событий
		while(reader.next());
		// Выполняем задание настроек разбора
		reader.settings(settings);
		// Выполняем проверку сброса кодировки, навязать которую не удалось
		ASSERT_EQ(reader.settings().encoding, xml::encoding_t::NONE);
		/**
		 * Сброс возвращает возможность навязать кодировку
		 */
		reader.reset();
		// Выполняем задание настроек разбора после сброса
		reader.settings(settings);
		// Выполняем проверку принятия навязанной кодировки
		ASSERT_EQ(reader.settings().encoding, xml::encoding_t::UTF8);
	}
}

/**
 * @brief Проверка отказов разбора самого описания типа документа
 *
 * @details Отказы эти сидят прежде разбора внутреннего подмножества и стерегут построение
 *          самого объявления: пробельный знак после слова `DOCTYPE` и имя описываемого
 *          типа документа. Пропусти разбор любой из них, объявление легло бы с именем,
 *          собранным не из того, - а имя это сличается с именем корневого узла
 *
 */
TEST(CodecXmlReader, DoctypeHeaderRefusals) {
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	/**
	 * Выполняем перебор негодных построений объявления
	 */
	for(auto & text : vector <string> {
		// Объявление без пробельного знака и без имени вовсе
		"<!DOCTYPE>",
		// Объявление без пробельного знака перед именем
		"<!DOCTYPEa>",
		// Имя описываемого типа документа, начатое цифрой
		"<!DOCTYPE 1a>",
		// Имя описываемого типа документа, начатое знаком переноса
		"<!DOCTYPE ->"
	}){
		// Выполняем разбор текста разметки
		::run(text, 4096, error);
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(error, xml::error_t::INVALID_DOCTYPE) << text;
	}
	/**
	 * Объявление, построенное по правилам, отказа не вызывает
	 */
	{
		// Выполняем разбор текста разметки с объявлением по правилам
		::run("<!DOCTYPE a><a/>", 4096, error);
		// Выполняем проверку отсутствия ошибки разбора
		ASSERT_EQ(error, xml::error_t::NONE);
	}
}

/**
 * @brief Проверка приведения значений атрибутов по объявленному виду
 *
 * @details Договор велит у атрибута, объявленного видом, отличным от «CDATA», снять
 * пробельные знаки по краям и свести всякую их вереницу к одному пробелу. Приведение это
 * к ПРОВЕРКЕ по описанию типа документа не относится и обязано выполняться независимо от
 * неё: вид атрибута берётся из объявления, а не из проверки соответствия ему
 *
 * @note Все четыре ветви найдены поверкой на наборе соответствия W3C, а не набором
 *       проверок: набор сличает нас с нами же и о договоре не знает ничего
 */
TEST(CodecXmlReader, AttributeTokenization){
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	/**
	 * Выполняем проверку приведения значения атрибута, объявленного видом «NMTOKEN»
	 */
	ASSERT_EQ(::run("<!DOCTYPE d [<!ELEMENT d ANY><!ATTLIST d a NMTOKEN #IMPLIED>]><d a=\"  x   y  \"/>", 4096, error),
		"[doctype d]<d a=x y></d>[end]") << xml::message(error);
	/**
	 * Выполняем проверку сохранности значения атрибута, объявленного видом «CDATA»
	 *
	 * @note Вид «CDATA» единственный, при котором приведения не происходит
	 */
	ASSERT_EQ(::run("<!DOCTYPE d [<!ELEMENT d ANY><!ATTLIST d a CDATA #IMPLIED>]><d a=\"  x   y  \"/>", 4096, error),
		"[doctype d]<d a=  x   y  ></d>[end]") << xml::message(error);
	/**
	 * Выполняем проверку сохранности значения атрибута, вовсе не объявленного
	 */
	ASSERT_EQ(::run("<!DOCTYPE d [<!ELEMENT d ANY>]><d a=\"  x   y  \"/>", 4096, error),
		"[doctype d]<d a=  x   y  ></d>[end]") << xml::message(error);
	/**
	 * Выполняем проверку связывающего действия ПЕРВОГО объявления атрибута
	 *
	 * @note Договор велит считать связывающим первое объявление, а повторные пропускать.
	 *       Разбор брал последнее, и вид «NMTOKENS» из повторного объявления отменял вид
	 *       «CDATA» из первого - значение приводилось там, где приводить его нельзя
	 */
	ASSERT_EQ(::run("<!DOCTYPE d [<!ATTLIST d a CDATA #IMPLIED><!ATTLIST d a NMTOKENS #IMPLIED><!ELEMENT d ANY>]><d a=\"1  2\"/>", 4096, error),
		"[doctype d]<d a=1  2></d>[end]") << xml::message(error);
}

/**
 * @brief Проверка прекращения обработки объявлений за непрочитанной параметрической сущностью
 *
 * @details Договор велит разбору, внешних сущностей не читающему, обрабатывать объявления
 * лишь ДО первой ссылки на непрочитанную параметрическую сущность: за нею объявления
 * вправе быть отменены тем, что лежит внутри непрочитанного, и опираться на них нельзя.
 * Исключение отведено одно - текст, объявленный самодостаточным
 *
 * @warning Проверка сличает ВЫДАННЫЕ атрибуты, а не приговор: текст принимается в обоих
 *          случаях, и расхождение видно лишь по подставленным значениям
 */
TEST(CodecXmlReader, DeclarationsAfterUnreadEntity){
	// Код ошибки разбора
	xml::error_t error = xml::error_t::NONE;
	/**
	 * Выполняем проверку подстановки значения, объявленного ДО ссылки
	 */
	ASSERT_EQ(::run("<!DOCTYPE d [<!ELEMENT d ANY><!ATTLIST d a CDATA \"v1\"><!ENTITY % e SYSTEM \"e.ent\">%e;]><d/>", 4096, error),
		"[doctype d]<d a=v1></d>[end]") << xml::message(error);
	/**
	 * Выполняем проверку отмены значения, объявленного ЗА ссылкой
	 */
	ASSERT_EQ(::run("<!DOCTYPE d [<!ELEMENT d ANY><!ENTITY % e SYSTEM \"e.ent\">%e;<!ATTLIST d a CDATA \"v1\">]><d/>", 4096, error),
		"[doctype d]<d></d>[end]") << xml::message(error);
	/**
	 * Выполняем проверку действия объявлений у текста, объявленного самодостаточным
	 *
	 * @note Самодостаточность обещает, что от внешнего подмножества смысл текста не
	 *       зависит, и объявления его действительны все
	 */
	ASSERT_EQ(::run("<?xml version=\"1.0\" standalone=\"yes\"?><!DOCTYPE d [<!ELEMENT d ANY><!ENTITY % e SYSTEM \"e.ent\">%e;<!ATTLIST d a CDATA \"v1\">]><d/>", 4096, error),
		"[decl 1.0][doctype d]<d a=v1></d>[end]") << xml::message(error);
}

/**
 * @brief Проверка независимости выдачи и пределов от нарезки текста на куски
 *
 * @details Собраны здесь четыре повода, найденные ворошителем: содержимое, разобранное
 *          до ошибочной ссылки, выдаётся событием прежде отказа; пробельность
 *          отбирается по приведённому содержимому, а не по разметке; отказ приведения
 *          равносилен концу текста; предел объёма события меряется у раздела дословного
 *          текста от начала раздела, а не от начала выдаваемой части
 *
 */
TEST(CodecXmlReader, ChunkIndependentEventsAndLimits) {
	/**
	 * @brief Функция разбора текста с выдачей слепка событий
	 *
	 * @details События одного вида подряд склеиваются: содержимое выдаётся частями, и
	 * число выдач договором не закреплено - закреплён лишь вид события и содержимое
	 *
	 * @param text     разбираемый текст разметки
	 * @param settings настройки разбора текста
	 * @param chunk    размер куска подачи, ноль подаёт текст целиком
	 * @return         слепок событий вместе с кодом отказа
	 *
	 */
	const auto digest = [](const string & text, const xml::reader_t::settings_t & settings, const size_t chunk) noexcept -> string {
		// Объект потокового чтения текста разметки
		xml::reader_t reader(::logger(), settings);
		// Собираемый слепок событий разбора
		string result;
		// Вид предыдущего события разбора
		int32_t previous = -1;
		// Содержимое, накопленное событиями одного вида подряд
		string merged;
		/**
		 * @brief Функция сброса накопленного события в слепок
		 *
		 */
		const auto flush = [&]() noexcept -> void {
			// Если накопленное событие есть, заносим его в слепок
			if(previous >= 0) result.append(std::to_string(previous)).append("[").append(merged).append("]");
			// Выполняем сброс накопленного события
			previous = -1; merged.clear();
		};
		/**
		 * @brief Функция вычерпывания событий разбора
		 *
		 */
		const auto drain = [&]() noexcept -> void {
			/**
			 * Выполняем перебор всех событий разбора
			 */
			while(reader.next()){
				// Получаем вид очередного события разбора
				const int32_t kind = static_cast <int32_t> (reader.event());
				// Если вид события сменился, сбрасываем накопленное
				if(kind != previous) flush();
				// Запоминаем вид события и накапливаем его содержимое
				previous = kind; merged.append(reader.text());
			}
		};
		/**
		 * Если текст подаётся целиком
		 */
		if(chunk == 0){
			// Выполняем подачу текста целиком
			reader.feed(text.data(), text.size(), true);
			// Выполняем вычерпывание событий разбора
			drain();
		/**
		 * Если текст подаётся кусками
		 */
		} else {
			// Положение подачи в исходном тексте
			size_t offset = 0;
			/**
			 * Выполняем подачу текста кусками
			 */
			for(;;){
				// Выполняем вычерпывание событий разбора
				drain();
				// Если текст подан целиком, выходим из подачи
				if(offset >= text.size()) break;
				// Получаем размер очередного подаваемого куска
				const size_t size = ((offset + chunk) > text.size() ? (text.size() - offset) : chunk);
				// Выполняем подачу очередного куска текста
				reader.feed(text.data() + offset, size, (offset + size) >= text.size());
				// Выполняем переход к следующему куску текста
				offset += size;
			}
		}
		// Выполняем сброс последнего накопленного события
		flush();
		// Дописываем к слепку код отказа разбора
		return result.append("/").append(std::to_string(static_cast <int32_t> (reader.error())));
	};
	/**
	 * @brief Функция проверки совпадения слепков при всякой нарезке
	 *
	 * @param text     разбираемый текст разметки
	 * @param settings настройки разбора текста
	 * @return         слепок событий, полученный подачей текста целиком
	 *
	 */
	/**
	 * @brief Функция изъятия содержимого из слепка событий
	 *
	 * @details Построение, выдаваемое частями, к мигу отказа по пределу успевает выдать
	 * столько частей, сколько позволила нарезка: подача целиком приносит его разом и
	 * отвергает, не выдав ничего, а подача по байту выдаёт части, пока накопленное под
	 * пределом. Иного и быть не может - удержание всех частей до конца построения копило
	 * бы ровно то, чему предел и поставлен заслоном. Договором закреплён потому ПРИГОВОР:
	 * код отказа и его место, - а не число частей, до отказа выданных
	 *
	 * @param digested слепок событий разбора
	 * @return         приговор разбора
	 *
	 */
	const auto verdict = [](const string & digested) noexcept -> string {
		// Выводим приговор разбора, отсекая слепок событий
		return digested.substr(digested.rfind('/'));
	};
	const auto stable = [&](const string & text, const xml::reader_t::settings_t & settings) noexcept -> string {
		// Получаем слепок событий при подаче текста целиком
		const string whole = digest(text, settings, 0);
		/**
		 * Выполняем перебор размеров куска подачи
		 */
		for(size_t chunk = 1; chunk <= 24; chunk++)
			// Выполняем проверку совпадения слепка при подаче кусками
			EXPECT_EQ(digest(text, settings, chunk), whole) << "кусок " << chunk;
		// Выводим слепок событий при подаче текста целиком
		return whole;
	};
	/**
	 * Выполняем проверку выдачи содержимого, разобранного до ошибочной ссылки
	 *
	 * @note Прежде подача кусками успевала выдать содержимое событием, а подача целиком
	 *       выбрасывала его вместе с отказом
	 */
	{
		// Настройки разбора текста
		xml::reader_t::settings_t settings;
		// Получаем слепок событий разбора
		const string digested = stable("<d>a&missing;b</d>", settings);
		// Выполняем проверку того, что содержимое до ссылки выдано событием
		ASSERT_NE(digested.find("[a]"), string::npos) << digested;
		// Выполняем проверку кода отказа разбора
		ASSERT_NE(digested.find("/" + std::to_string(static_cast <int32_t> (xml::error_t::UNKNOWN_ENTITY))), string::npos) << digested;
	}
	/**
	 * Выполняем проверку отбора пробельности по ПРИВЕДЁННОМУ содержимому
	 *
	 * @note Ссылка на сущность, объявление которой лежит во внешнем подмножестве,
	 *       пропускается без подстановки, и непробельная разметка обращается в
	 *       пробельное содержимое: вид события выходил тот, куда легла граница куска
	 */
	{
		// Настройки разбора текста с отделением незначимого пробельного содержимого
		xml::reader_t::settings_t settings;
		// Включаем отделение незначимого пробельного содержимого
		settings.separateSpaces = true;
		// Выполняем проверку устойчивости выдачи к нарезке текста на куски
		stable("<!DOCTYPE d SYSTEM \"d.dtd\"><d><![CDATA[ a ]]>&missing; -1234 </d>", settings);
	}
	/**
	 * Выполняем проверку старшинства отказов при отказавшем приведении
	 *
	 * @note Отказ приведения равносилен концу текста: подачи после него не принимаются.
	 *       Ссылка на необъявленную сущность стоит в тексте раньше негодного байта и по
	 *       старшинству побеждает
	 */
	{
		// Настройки разбора текста с отделением незначимого пробельного содержимого
		xml::reader_t::settings_t settings;
		// Включаем отделение незначимого пробельного содержимого
		settings.separateSpaces = true;
		// Получаем слепок событий разбора текста с негодной последовательностью байтов
		const string digested = stable("<c> \t\r\n &missing;bad\xC3( tail</c>", settings);
		// Выполняем проверку того, что победил отказ по ссылке, а не по приведению
		ASSERT_NE(digested.find("/" + std::to_string(static_cast <int32_t> (xml::error_t::UNKNOWN_ENTITY))), string::npos) << digested;
	}
	/**
	 * Выполняем проверку предела объёма события у раздела дословного текста
	 *
	 * @note Предел меряется от начала раздела вместе с закрывающей последовательностью:
	 *       части раздела сличались с ним порознь, и размер их задавала нарезка
	 */
	{
		// Разбираемый текст с разделом дословного текста
		const string text = "<a><![CDATA[ a\r\n b\r c ]]></a>";
		/**
		 * Выполняем перебор пределов объёма события вокруг длины разметки раздела
		 */
		for(uint64_t limit = 8; limit <= 30; limit++){
			// Настройки разбора текста с пределом объёма события
			xml::reader_t::settings_t settings;
			// Задаём наибольший допустимый объём одного события
			settings.maxEvent = limit;
			// Получаем приговор разбора при подаче текста целиком
			const string whole = verdict(digest(text, settings, 0));
			/**
			 * Выполняем перебор размеров куска подачи
			 */
			for(size_t chunk = 1; chunk <= 24; chunk++)
				// Выполняем проверку совпадения приговора при подаче кусками
				EXPECT_EQ(verdict(digest(text, settings, chunk)), whole) << "предел " << limit << " кусок " << chunk;
		}
	}
}
/**
 * @brief Проверка умолчания предела объёма события
 *
 * @details Чтение накапливает построение целиком, прежде чем разбирать его, и предел
 *          этот - единственное, что накопление стережёт. Умолчанием он снимался, и
 *          разбор при умолчаниях памяти не берёг вовсе: незакрытая метка набирала
 *          64 мегабайта и более, не отвечая отказом
 *
 */
TEST(CodecXmlReader, EventLimitGuardsMemoryByDefault) {
	// Настройки разбора текста с умолчаниями
	xml::reader_t::settings_t settings;
	// Выполняем проверку того, что предел объёма события умолчанием задан
	ASSERT_EQ(settings.maxEvent, static_cast <uint64_t> (xml::MAX_EVENT));
	// Выполняем проверку того, что умолчание сходится с пределами кодеков-соседей
	ASSERT_EQ(static_cast <uint64_t> (xml::MAX_EVENT), static_cast <uint64_t> (0x1000000));
	// Объект потокового чтения текста разметки
	xml::reader_t reader(::logger(), settings);
	// Выполняем подачу начала метки, конца не имеющей
	ASSERT_TRUE(reader.feed("<", 1, false));
	// Кусок имени метки в мегабайт длиной
	const string chunk(1024 * 1024, 'x');
	// Количество поданных кусков имени метки
	size_t fed = 0;
	/**
	 * Выполняем подачу кусков имени метки, пока разбор их принимает
	 */
	for(size_t i = 0; i < 32; i++){
		// Выполняем вычерпывание событий разбора
		while(reader.next());
		// Если разбор ответил отказом, прекращаем подачу
		if(reader.error() != xml::error_t::NONE) break;
		// Выполняем подачу очередного куска имени метки
		if(!reader.feed(chunk.data(), chunk.size(), false)) break;
		// Выполняем учёт поданного куска имени метки
		fed++;
	}
	// Выполняем вычерпывание оставшихся событий разбора
	while(reader.next());
	// Выполняем проверку того, что накопление прекращено отказом
	ASSERT_EQ(reader.error(), xml::error_t::OVERFLOW_LIMIT);
	// Выполняем проверку того, что отказ последовал в пределах умолчания
	ASSERT_LE(fed, static_cast <size_t> (17)) << "накоплено мегабайтов: " << fed;
}

/**
 * @brief Проверка отказа подачи после объявленного конца текста
 *
 * @details Прежде подача после объявленного конца отвергалась голым признаком неудачи:
 *          ни кода отказа, ни записи в журнале не заводилось, и отличить исчерпанный
 *          текст от отказа разбора вызывающему было нечем. Кодек CSV отвечает здесь
 *          своим кодом, и разметка отвечает им же
 *
 */
TEST(CodecXmlReader, FeedAfterLastChunkRefused) {
	// Объект потокового чтения текста разметки
	xml::reader_t reader(::logger());
	// Выполняем проверку приёма текста разметки целиком
	ASSERT_TRUE(reader.feed("<a/>", 4, true));
	// Выполняем перебор всех событий разбора
	while(reader.next());
	// Выполняем проверку того, что разбор дошёл до конца без отказа
	ASSERT_EQ(reader.error(), xml::error_t::NONE);
	/**
	 * Выполняем проверку того, что пустая подача отказом не является
	 *
	 * @note Она ничего к тексту не добавляет, и повторить объявление конца вызывающий вправе
	 */
	ASSERT_TRUE(reader.feed("", 0, true));
	// Выполняем проверку того, что отказа пустая подача не навела
	ASSERT_EQ(reader.error(), xml::error_t::NONE);
	// Выполняем проверку отказа подачи после объявленного конца текста
	ASSERT_FALSE(reader.feed("<b/>", 4, true));
	// Выполняем проверку кода отказа разбора
	ASSERT_EQ(reader.error(), xml::error_t::TEXT_ALREADY_ENDED);
	// Выполняем сброс состояния разбора
	reader.reset();
	// Выполняем проверку того, что после сброса подача принимается вновь
	ASSERT_TRUE(reader.feed("<b/>", 4, true));
	// Выполняем перебор всех событий разбора
	while(reader.next());
	// Выполняем проверку того, что разбор дошёл до конца без отказа
	ASSERT_EQ(reader.error(), xml::error_t::NONE);
}

/**
 * @brief Проверка примечания и указания обработчику, конца не имеющих
 *
 * @details Конец разыскивается по образцу, и отказ следует как тогда, когда образца
 * нет вовсе, так и тогда, когда он найден за пределами разбираемого отрезка. Вторая
 * ветвь покрытием пройдена не была
 *
 */
TEST(CodecXmlReader, UnterminatedCommentAndProcessing) {
	/**
	 * Перебираемые тексты разметки и ожидаемые коды отказа
	 */
	const struct {
		const char * text;
		const xml::error_t error;
	} samples[] = {
		{"<r><!-- текст</r>", xml::error_t::INVALID_COMMENT},
		{"<r><!-- текст -", xml::error_t::INVALID_COMMENT},
		{"<r><?цель текст</r>", xml::error_t::INVALID_PROCESSING},
		{"<r><?цель текст ?", xml::error_t::INVALID_PROCESSING}
	};
	/**
	 * Выполняем перебор всех разбираемых текстов разметки
	 */
	for(const auto & sample : samples){
		// Чтение текста разметки
		xml::reader_t reader(::logger());
		// Выполняем подачу текста разметки
		reader.feed(sample.text);
		/**
		 * Выполняем опустошение очереди событий разбора
		 */
		while(reader.next())
			// Выполняем переход к следующему событию разбора
			continue;
		// Выполняем проверку кода отказа разбора
		ASSERT_EQ(reader.error(), sample.error) << sample.text;
	}
}

/**
 * @brief Проверка предела вложенности подстановки сущностей
 *
 * @details Сущность, подставляющая сущность, разбирается возвратно, и предел вложенности
 * оберегает стек вызовов от срыва. Ветвь эта покрытием пройдена не была
 *
 */
TEST(CodecXmlReader, EntityDepthExceeded) {
	// Собираемый текст разметки
	string text = "<!DOCTYPE r [";
	/**
	 * Выполняем сбор объявлений сущностей, друг друга подставляющих
	 */
	for(size_t i = 0; i < 40; i++)
		// Заносим очередное объявление сущности
		text.append("<!ENTITY e").append(std::to_string(i)).append(" \"&e").append(std::to_string(i + 1)).append(";\">");
	// Завершаем цепочку объявлений сущностью со своим содержимым
	text.append("<!ENTITY e40 \"конец\">]><r>&e0;</r>");
	// Чтение текста разметки
	xml::reader_t reader(::logger());
	// Выполняем подачу текста разметки
	reader.feed(text);
	/**
	 * Выполняем опустошение очереди событий разбора
	 */
	while(reader.next())
		// Выполняем переход к следующему событию разбора
		continue;
	// Выполняем проверку кода отказа разбора
	ASSERT_EQ(reader.error(), xml::error_t::ENTITY_DEPTH_EXCEEDED);
}

/**
 * @brief Проверка приведения значений атрибутов по объявленному виду
 *
 * @details Договор велит приводить значения атрибутов, объявленных видом приводимым:
 * пробельная обвязка снимается, а череда пробельных знаков внутри сводится к одному.
 * Значения вида `CDATA` приведению НЕ подлежат и переносятся как есть. Ветвь приведения
 * покрытием пройдена не была
 *
 */
TEST(CodecXmlReader, TokenizedAttributesNormalized) {
	/**
	 * Перебираемые тексты разметки и ожидаемые значения атрибута
	 */
	const struct {
		const char * text;
		const char * value;
	} samples[] = {
		{"<!DOCTYPE r [<!ATTLIST r a NMTOKEN #IMPLIED>]><r a=\"  знач  \">x</r>", "знач"},
		{"<!DOCTYPE r [<!ATTLIST r a CDATA #IMPLIED>]><r a=\"  знач  \">x</r>", "  знач  "},
		{"<!DOCTYPE r [<!ATTLIST r a NMTOKENS #IMPLIED>]><r a=\" один   два \">x</r>", "один два"}
	};
	/**
	 * Выполняем перебор всех разбираемых текстов разметки
	 */
	for(const auto & sample : samples){
		// Чтение текста разметки
		xml::reader_t reader(::logger());
		// Выполняем подачу текста разметки
		reader.feed(sample.text);
		// Признак того, что значение атрибута сличено
		bool checked = false;
		/**
		 * Выполняем перебор всех событий разбора
		 */
		while(reader.next()){
			/**
			 * Если событием является открытие узла разметки
			 */
			if(reader.event() == xml::event_t::ELEMENT_OPEN){
				// Выполняем проверку количества свойств узла разметки
				ASSERT_EQ(reader.attributes().size(), static_cast <size_t> (1)) << sample.text;
				// Выполняем проверку значения свойства узла разметки
				ASSERT_EQ(reader.attributes().front().value, sample.value) << sample.text;
				// Запоминаем, что значение атрибута сличено
				checked = true;
			}
		}
		// Выполняем проверку отсутствия отказа разбора
		ASSERT_EQ(reader.error(), xml::error_t::NONE) << sample.text;
		// Выполняем проверку того, что сличение состоялось
		ASSERT_TRUE(checked) << sample.text;
	}
}

/**
 * @brief Проверка отказа закрывающей скобки раздела дословного текста в содержимом
 *
 * @details Договор запрещает последовательность «]]>» в содержимом узла: она означала
 * бы конец раздела дословного текста, какого не открывали. Ветвь эта покрытием пройдена
 * не была
 *
 */
TEST(CodecXmlReader, SectionCloseInContentRefused) {
	// Чтение текста разметки
	xml::reader_t reader(::logger());
	// Выполняем подачу текста разметки
	reader.feed("<r>текст]]>ещё</r>");
	/**
	 * Выполняем опустошение очереди событий разбора
	 */
	while(reader.next())
		// Выполняем переход к следующему событию разбора
		continue;
	// Выполняем проверку кода отказа разбора
	ASSERT_EQ(reader.error(), xml::error_t::INVALID_CHARACTER);
	// Чтение текста разметки с той же последовательностью внутри раздела дословного текста
	xml::reader_t verbatim(::logger());
	// Выполняем подачу текста разметки
	verbatim.feed("<r><![CDATA[текст]]>ещё</r>");
	/**
	 * Выполняем опустошение очереди событий разбора
	 */
	while(verbatim.next())
		// Выполняем переход к следующему событию разбора
		continue;
	// Выполняем проверку отсутствия отказа разбора
	ASSERT_EQ(verbatim.error(), xml::error_t::NONE);
}

/**
 * @brief Проверка однобайтовой кодировки, объявленной разметкой
 *
 * @details Разметка, объявленная кодировкой вроде `windows-1251`, приводится по
 * перекодировочной таблице, а знак, в разметке недопустимый, отвергается отказом. Ветвь
 * эта покрытием пройдена не была
 *
 */
TEST(CodecXmlReader, SingleByteEncodingRefusesDisallowedCharacter) {
	{
		// Чтение текста разметки
		xml::reader_t reader(::logger());
		// Собираемый текст разметки
		string text = "<?xml version=\"1.0\" encoding=\"windows-1251\"?><r>";
		// Заносим управляющий знак, в разметке недопустимый
		text.push_back('\x01');
		// Завершаем текст разметки
		text.append("</r>");
		// Выполняем подачу текста разметки
		reader.feed(text);
		/**
		 * Выполняем опустошение очереди событий разбора
		 */
		while(reader.next())
			// Выполняем переход к следующему событию разбора
			continue;
		// Выполняем проверку кода отказа разбора
		ASSERT_EQ(reader.error(), xml::error_t::INVALID_CHARACTER);
	}
	{
		// Чтение текста разметки
		xml::reader_t reader(::logger());
		// Собираемый текст разметки
		string text = "<?xml version=\"1.0\" encoding=\"windows-1251\"?><r>";
		// Заносим знаки кириллицы в объявленной кодировке
		text.push_back('\xcf');
		// Заносим знаки кириллицы в объявленной кодировке
		text.push_back('\xf0');
		// Завершаем текст разметки
		text.append("</r>");
		// Выполняем подачу текста разметки
		reader.feed(text);
		// Собираемое содержимое узла разметки
		string content;
		/**
		 * Выполняем перебор всех событий разбора
		 */
		while(reader.next()){
			// Если событием является содержимое узла разметки
			if(reader.event() == xml::event_t::TEXT)
				// Заносим содержимое узла разметки
				content.append(reader.text());
		}
		// Выполняем проверку отсутствия отказа разбора
		ASSERT_EQ(reader.error(), xml::error_t::NONE);
		// Выполняем проверку приведённого содержимого узла разметки
		ASSERT_EQ(content, "Пр");
	}
}

/**
 * @brief Проверка установки объекта ведения журнала работы после создания
 *
 * @details Установка эта есть у всякого класса обоих кодеков, а у чтения разметки её не
 * было вовсе - единственного из десяти. Заведена по образцу чтения JSON, журнал уходит и
 * приведению исходного текста. Кодек CSV доведён до того же порядка тем же кругом
 *
 */
TEST(CodecXmlReader, LoggerSetAfterCreation) {
	// Собираемые сообщения журнала
	vector <string> messages;
	// Объект журнала с перехватом вывода
	awh::log_t log(&Silent::framework());
	// Выполняем назначение приёмника вывода в функцию обратного вызова
	log.mode({awh::log_t::mode_t::DEFERRED});
	// Выполняем назначение перехвата сообщений журнала
	log.subscribe([&messages](const awh::log_t::flag_t, string_view text) noexcept -> void {
		// Выполняем сбор очередного сообщения журнала
		messages.push_back(string(text));
	});
	{
		// Чтение текста разметки без объекта ведения журнала работы
		xml::reader_t reader(nullptr);
		// Выполняем подачу негодного текста разметки
		reader.feed("<r><a></r>");
		/**
		 * Выполняем опустошение очереди событий разбора
		 */
		while(reader.next())
			// Выполняем переход к следующему событию разбора
			continue;
		// Выполняем проверку оглашения отказа разбора
		ASSERT_NE(reader.error(), xml::error_t::NONE);
		// Выполняем проверку молчания журнала, покуда он не установлен
		ASSERT_TRUE(messages.empty());
		// Выполняем сброс состояния чтения
		reader.reset();
		// Выполняем установку объекта ведения журнала работы
		reader.setLogger(&log);
		// Выполняем подачу негодного текста разметки
		reader.feed("<r><a></r>");
		/**
		 * Выполняем опустошение очереди событий разбора
		 */
		while(reader.next())
			// Выполняем переход к следующему событию разбора
			continue;
		// Выполняем проверку оглашения отказа в журнале
		ASSERT_FALSE(messages.empty());
	}
	// Очищаем собранные сообщения журнала
	messages.clear();
	{
		// Приведение текста разметки без объекта ведения журнала работы
		xml::decoder_t decoder(nullptr);
		// Полученный приведением текст разметки
		string result;
		// Негодная последовательность знаков UTF-8
		const char broken[] = {'<', 'r', '>', ' ', '\xc2', '\xc2'};
		// Выполняем проверку отказа приведения негодного текста разметки
		ASSERT_FALSE(decoder.convert(broken, sizeof(broken), true, result));
		// Выполняем проверку молчания журнала, покуда он не установлен
		ASSERT_TRUE(messages.empty());
		// Выполняем сброс состояния приведения
		decoder.reset();
		// Выполняем установку объекта ведения журнала работы
		decoder.setLogger(&log);
		// Выполняем проверку отказа приведения негодного текста разметки
		ASSERT_FALSE(decoder.convert(broken, sizeof(broken), true, result));
		// Выполняем проверку оглашения отказа в журнале
		ASSERT_FALSE(messages.empty());
	}
}

/**
 * @brief Проверка того, что ноль снимает предел, а не задаёт предел в ноль
 *
 * @details Кодеки JSON и CSV договором объявляют ноль снятием предела, и предел объёма
 * события у самого чтения разметки держался того же правила. Прочие же пять пределов
 * чтения ноль понимали пределом В НОЛЬ и отвечали отказом на всякий текст. Расхождение
 * доходило до нелепости: `maxDepth = 0` у записи разметки предел снимал, а у чтения той
 * же разметки - валил разбор, при одном имени поля и одном кодеке
 *
 * @note Обнаружено сличением договоров кодеков между собой: у пределов разметки не было
 *       сказано о нуле ни слова, тогда как у JSON и CSV сказано у всякого
 *
 */
TEST(CodecXmlReader, ZeroLimitMeansNoLimit) {
	// Разбираемый текст разметки
	const string text = "<r a=\"1\" b=\"2\"><n>текст</n></r>";
	// Разбираемый текст разметки с объявленной сущностью
	const string entity = "<!DOCTYPE r [<!ENTITY e \"x\">]><r>&e;</r>";
	/**
	 * @brief Прогон разбора с заданными настройками
	 *
	 * @param settings настройки разбора
	 * @param text     разбираемый текст разметки
	 * @return         количество выданных событий разбора
	 *
	 */
	const auto run = [](const xml::reader_t::settings_t & settings, const string & text) noexcept -> pair <size_t, xml::error_t> {
		// Чтение текста разметки
		xml::reader_t reader(::logger(), settings);
		// Выполняем подачу текста разметки
		reader.feed(text);
		// Количество выданных событий разбора
		size_t events = 0;
		/**
		 * Выполняем перебор всех событий разбора
		 */
		while(reader.next())
			// Выполняем учёт очередного события разбора
			events++;
		// Выводим итог разбора текста разметки
		return make_pair(events, reader.error());
	};
	{
		// Настройки разбора со снятым пределом глубины вложенности
		xml::reader_t::settings_t settings;
		// Выполняем снятие предела глубины вложенности
		settings.maxDepth = 0;
		// Выполняем проверку отсутствия отказа разбора
		ASSERT_EQ(run(settings, text).second, xml::error_t::NONE);
	}
	{
		// Настройки разбора со снятым пределом длины имени
		xml::reader_t::settings_t settings;
		// Выполняем снятие предела длины имени
		settings.maxName = 0;
		// Выполняем проверку отсутствия отказа разбора
		ASSERT_EQ(run(settings, text).second, xml::error_t::NONE);
	}
	{
		// Настройки разбора со снятым пределом количества атрибутов
		xml::reader_t::settings_t settings;
		// Выполняем снятие предела количества атрибутов
		settings.maxAttributes = 0;
		// Выполняем проверку отсутствия отказа разбора
		ASSERT_EQ(run(settings, text).second, xml::error_t::NONE);
	}
	{
		// Настройки разбора со снятым пределом количества сущностей
		xml::reader_t::settings_t settings;
		// Выполняем снятие предела количества объявленных сущностей
		settings.maxEntities = 0;
		// Выполняем проверку отсутствия отказа разбора
		ASSERT_EQ(run(settings, entity).second, xml::error_t::NONE);
	}
	{
		// Настройки разбора со снятым пределом объёма подстановки
		xml::reader_t::settings_t settings;
		// Выполняем снятие предела объёма подстановки сущностей
		settings.maxExpansion = 0;
		// Выполняем проверку отсутствия отказа разбора
		ASSERT_EQ(run(settings, entity).second, xml::error_t::NONE);
	}
	{
		// Настройки разбора со снятым пределом объёма события
		xml::reader_t::settings_t settings;
		// Выполняем снятие предела объёма события
		settings.maxEvent = 0;
		// Выполняем проверку отсутствия отказа разбора
		ASSERT_EQ(run(settings, text).second, xml::error_t::NONE);
	}
	{
		// Настройки разбора с пределами, заданными единицей
		xml::reader_t::settings_t settings;
		// Выполняем указание предела глубины вложенности
		settings.maxDepth = 1;
		/**
		 * Выполняем проверку того, что предел ЗАДАННЫЙ по-прежнему держится
		 *
		 * @note Сличение это обязательно: снятие предела нулём не должно обращаться в
		 *       снятие предела вовсе
		 */
		ASSERT_EQ(run(settings, text).second, xml::error_t::DEPTH_EXCEEDED);
	}
}

/**
 * @brief Проверка того, что причина отказа отвечает беде, а не месту сличения
 *
 * @details Один код `OVERFLOW_LIMIT` отвечал за три разные беды: превышение предела
 * объёма события, превышение предела количества атрибутов и выход за разрядность
 * хранилища разбора. Последняя настройками не задаётся вовсе, и сообщение «configured
 * parser limit exceeded» отправляло потребителя искать предел, какого он не ставил.
 * Кодеки JSON и CSV дают всякому своему пределу собственный код, и расходиться с ними
 * здесь нечем
 *
 * @note Выход за разрядность хранилища проверкою не достигается - он требует четырёх
 *       гигабайт текста в одном разборе, - и сличается здесь лишь то, что код его
 *       заведён отдельным и описание его о настройках не говорит
 *
 */
TEST(CodecXmlReader, LimitRefusalsNameTheirOwnCause) {
	{
		// Настройки разбора с пределом количества атрибутов
		xml::reader_t::settings_t settings;
		// Выполняем указание предела количества атрибутов узла
		settings.maxAttributes = 2;
		// Чтение текста разметки
		xml::reader_t reader(::logger(), settings);
		// Выполняем подачу текста разметки
		reader.feed("<r a=\"1\" b=\"2\" c=\"3\"/>");
		/**
		 * Выполняем опустошение очереди событий разбора
		 */
		while(reader.next())
			// Выполняем переход к следующему событию разбора
			continue;
		// Выполняем проверку кода отказа разбора
		ASSERT_EQ(reader.error(), xml::error_t::TOO_MANY_ATTRIBUTES);
	}
	{
		// Настройки разбора с пределом объёма события
		xml::reader_t::settings_t settings;
		// Выполняем указание предела объёма события
		settings.maxEvent = 8;
		// Чтение текста разметки
		xml::reader_t reader(::logger(), settings);
		// Выполняем подачу текста разметки
		reader.feed("<r>очень длинное содержимое узла разметки</r>");
		/**
		 * Выполняем опустошение очереди событий разбора
		 */
		while(reader.next())
			// Выполняем переход к следующему событию разбора
			continue;
		// Выполняем проверку кода отказа разбора
		ASSERT_EQ(reader.error(), xml::error_t::OVERFLOW_LIMIT);
	}
	// Выполняем проверку того, что коды эти различны
	ASSERT_NE(xml::error_t::TOO_MANY_ATTRIBUTES, xml::error_t::OVERFLOW_LIMIT);
	// Выполняем проверку того, что разрядность хранилища заведена отдельным кодом
	ASSERT_NE(xml::error_t::STORAGE_EXHAUSTED, xml::error_t::OVERFLOW_LIMIT);
	// Выполняем проверку того, что описания кодов различны
	ASSERT_STRNE(xml::message(xml::error_t::STORAGE_EXHAUSTED), xml::message(xml::error_t::OVERFLOW_LIMIT));
	// Выполняем проверку того, что описание разрядности о настройках не говорит
	ASSERT_EQ(string(xml::message(xml::error_t::STORAGE_EXHAUSTED)).find("configured"), string::npos);
	// Выполняем проверку описания предела количества атрибутов
	ASSERT_STRNE(xml::message(xml::error_t::TOO_MANY_ATTRIBUTES), xml::message(xml::error_t::OVERFLOW_LIMIT));
}
/**
 * @brief Проверка независимости разбора от нарезки текста на куски
 *
 * @details Разбор всеми возможными разрезами сличается с подачей целиком. У таблиц CSV и
 * документов JSON слепок событий совпадает при любом разрезе, а разметка содержимое по
 * границе подачи РАЗРЫВАЕТ: склейка идёт в пределах поданного куска. Договор о склейке
 * перечислял разрывы - раздел дословного текста, примечание, указание обработчику, - а
 * границу подачи не называл, и обещание было шире исполнения
 *
 * @note Проверка закрепляет ОБА свойства: события дробятся, а содержимое цело. Первое -
 *       свойство потокового чтения, второе - его обязанность
 *
 */
TEST(CodecXmlReader, FeedBoundarySplitsEventsNotContent){
	// Разбираемый текст разметки с многобайтовым содержимым
	const string TEXT = "<к>ЖЖЖ</к>";
	// Содержимое, собранное подачей текста целиком
	string whole;
	// Количество событий содержимого при подаче целиком
	size_t events = 0;
	/**
	 * Выполняем подачу текста разметки целиком
	 */
	{
		// Объект чтения текста разметки
		xml::reader_t reader(::logger());
		// Выполняем подачу текста разметки целиком
		ASSERT_TRUE(reader.feed(TEXT.data(), TEXT.size(), true));
		/**
		 * Выполняем перебор всех событий разбора
		 */
		while(reader.next()){
			// Если событием является текстовое содержимое
			if(reader.event() == xml::event_t::TEXT){
				// Выполняем сбор содержимого события
				whole.append(reader.text());
				// Выполняем учёт события содержимого
				events++;
			}
		}
		// Выполняем проверку того, что разбор прошёл без отказа
		ASSERT_EQ(reader.error(), xml::error_t::NONE);
		// Выполняем проверку того, что содержимое выдано одним событием
		ASSERT_EQ(events, static_cast <size_t> (1));
		// Выполняем проверку собранного содержимого
		ASSERT_EQ(whole, "ЖЖЖ");
	}
	/**
	 * Выполняем перебор всех возможных разрезов текста разметки
	 */
	for(size_t cut = 1; cut < TEXT.size(); cut++){
		// Объект чтения текста разметки
		xml::reader_t reader(::logger());
		// Содержимое, собранное подачей текста двумя кусками
		string parts;
		// Количество событий содержимого при подаче кусками
		size_t issued = 0;
		// Границы подаваемых кусков текста разметки
		const size_t bounds[2] = {cut, TEXT.size()};
		// Смещение начала очередного подаваемого куска
		size_t offset = 0;
		/**
		 * Выполняем подачу текста разметки двумя кусками
		 */
		for(const size_t bound : bounds){
			// Выполняем подачу очередного куска текста разметки
			reader.feed(TEXT.data() + offset, bound - offset, (bound >= TEXT.size()));
			/**
			 * Выполняем перебор всех событий разбора
			 */
			while(reader.next()){
				// Если событием является текстовое содержимое
				if(reader.event() == xml::event_t::TEXT){
					// Выполняем сбор содержимого события
					parts.append(reader.text());
					// Выполняем учёт события содержимого
					issued++;
				}
			}
			// Запоминаем смещение начала следующего куска
			offset = bound;
		}
		// Выполняем проверку того, что разбор прошёл без отказа
		ASSERT_EQ(reader.error(), xml::error_t::NONE) << cut;
		/**
		 * Выполняем проверку того, что содержимое от нарезки не пострадало
		 *
		 * @note Это и есть обязанность: событий бывает больше одного, а сложенное из них
		 *       содержимое обязано совпадать с поданным целиком знак в знак
		 */
		ASSERT_EQ(parts, whole) << cut;
		// Выполняем проверку того, что событий содержимого не меньше одного
		ASSERT_GE(issued, static_cast <size_t> (1)) << cut;
	}
}
/**
 * @brief Проверка сроков жизни видов, отдаваемых чтением
 *
 * @details Три тела чтения отдают виды, и сроки у них РАЗНЫЕ: свойство живёт до разбора
 * следующего узла со свойствами, связывание области годно, пока открыт объявивший его
 * узел, а объявление пролога переживает подачу и живёт до сброса состояния. Прежде о
 * сроках молчали все три, при том что соседнее тело `text()` о своём ручалось громко:
 * молчание рядом с ручающимся соседом читается как «а тут можно»
 *
 * @note Замеры: удержанное свойство после двухсот узлов со свойствами читало испорченное
 *       содержимое; удержанное связывание по закрытии узла читалось прежним, тогда как
 *       снятое заново отвечало пустым; объявление пролога пережило двести подач
 *
 * @note Обесцененные виды здесь НЕ читаются ни разу: чтение их есть неопределённое
 *       поведение, и проверка на него утверждала бы случайность, а не договор кодека
 */
TEST(CodecXmlReader, ViewLifetimesDifferPerBody){
	// Чтение текста разметки
	xml::reader_t reader(::logger());
	// Первый подаваемый кусок текста разметки
	const string first = "<?xml version=\"1.0\"?><r xmlns:p=\"urn:A\" p:k=\"знач\">";
	// Выполняем подачу первого куска текста разметки
	reader.feed(first.data(), first.size(), false);
	// Копия содержимого свойства, снятая на месте
	string attribute;
	/**
	 * Выполняем перебор всех событий разбора
	 */
	while(reader.next()){
		/**
		 * Если событием является открытие узла разметки
		 */
		if(reader.event() == xml::event_t::ELEMENT_OPEN){
			// Снимаем копию содержимого свойства
			attribute.assign(reader.attribute("k", "urn:A"));
			// Выполняем проверку того, что связывание области разрешается
			ASSERT_EQ(reader.resolve("p"), "urn:A");
		}
	}
	// Выполняем проверку снятой копии содержимого свойства
	ASSERT_EQ(attribute, "знач");
	// Удерживаем вид на объявление пролога
	const string_view version = reader.version();
	// Выполняем проверку удержанного объявления пролога
	ASSERT_EQ(version, "1.0");
	/**
	 * Выполняем подачу узлов со свойствами, хранилище свойств переписывающих
	 */
	for(size_t i = 0; i < 200; i++){
		// Очередной подаваемый кусок текста разметки
		const string more = "<b" + to_string(i) + " m=\"" + string(64, 'y') + "\"/>";
		// Выполняем подачу очередного куска текста разметки
		reader.feed(more.data(), more.size(), false);
		// Выполняем перебор всех событий разбора
		while(reader.next()){}
	}
	// Выполняем проверку того, что снятая копия свойства подачу пережила
	ASSERT_EQ(attribute, "знач");
	// Выполняем проверку того, что удержанное объявление пролога подачу пережило
	ASSERT_EQ(version, "1.0");
	// Выполняем проверку того, что связывание области ещё разрешается
	ASSERT_EQ(reader.resolve("p"), "urn:A");
	// Закрывающий кусок текста разметки
	const string close = "</r>";
	// Выполняем подачу закрывающего куска текста разметки
	reader.feed(close.data(), close.size(), true);
	// Выполняем перебор всех событий разбора
	while(reader.next()){}
	/**
	 * Выполняем проверку того, что связывание области по закрытии узла снято
	 *
	 * @note Спрашивается оно ЗОВОМ, а не удержанным видом: вид по закрытии продолжает
	 *       читаться прежним, хранилище областей не освобождается
	 */
	ASSERT_TRUE(reader.resolve("p").empty());
	// Выполняем проверку того, что объявление пролога закрытие пережило
	ASSERT_EQ(version, "1.0");
}
/**
 * @brief Проверка того, что отказ разбора всплывает перебором событий, а не подачей
 *
 * @details Подача лишь принимает кусок в очередь, а разбирается он зовами `next()`:
 * оттого подача испорченного текста отвечает УСПЕХОМ и пустым кодом, а код появляется
 * по переборе событий. Соседние кодеки JSON и CSV поступают обратно - разбор идёт прямо
 * в подаче, - и договор о том прежде говорил лишь применительно к отказу кодировки,
 * подразумевая тем, что грамматические отказы видны подачей. Они не видны
 *
 * @note Замер: текст `<r><<a></a></r>` принят подачей успешно и с пустым кодом
 *
 * @note Единый цикл на несколько кодеков обязан спрашивать `error()`, а не один лишь
 *       возврат подачи
 */
TEST(CodecXmlReader, RefusalSurfacesOnTraversalNotOnFeed){
	// Чтение текста разметки
	xml::reader_t reader(::logger());
	// Испорченный текст разметки
	const string text = "<r><<a></a></r>";
	// Выполняем проверку того, что подача отвечает успехом
	ASSERT_TRUE(reader.feed(text.data(), text.size(), true));
	// Выполняем проверку того, что код отказа к мигу возврата подачи пуст
	ASSERT_EQ(reader.error(), xml::error_t::NONE);
	// Выполняем перебор всех событий разбора
	while(reader.next()){}
	// Выполняем проверку того, что отказ выдан перебором событий
	ASSERT_NE(reader.error(), xml::error_t::NONE);
	// Код отказа, выданный перебором событий
	const xml::error_t error = reader.error();
	// Остаток текста разметки
	const string more = "<b/>";
	// Выполняем проверку того, что следующий кусок принят уже не будет
	ASSERT_FALSE(reader.feed(more.data(), more.size(), true));
	// Выполняем проверку того, что код отказа сохранён
	ASSERT_EQ(reader.error(), error);
}
/**
 * @brief Проверка того, что склейка содержимого переживает границу подачи
 *
 * @details Признак `mergeText` склеивает содержимое узла в одно событие ЧЕРЕЗ границу
 *          подачи, а не в пределах куска: текст, поданный хоть по одному байту, даёт
 *          одно событие содержимого. При опущенной же склейке граница куска содержимое
 *          разрывает, и складывать события надлежит потребителю
 *
 * @note Проверка эта заведена оттого, что ЗАПИСЬ у признака утверждала обратное - будто
 *       склейка идёт в пределах куска всегда - и велела потребителю писать сложение,
 *       которое кодек делает сам. Вскрыто сплошным обходом настроек: признак этот
 *       единственный из восьми не дал расхождений, а разбор причины показал, что лжёт
 *       не код, а запись
 *
 * @note Проверка берёт ОБА положения признака намеренно: утверждай она лишь склейку,
 *       она проходила бы и у разбора, вовсе не умеющего разрывать содержимое
 *
 */
TEST(CodecXmlReader, MergeTextSurvivesFeedBoundary){
	// Собирает содержимое узла событиями при заданной склейке и нарезке
	const auto собрать = [](const bool merge, const size_t piece) noexcept -> vector <string> {
		// Объект чтения текста разметки
		xml::reader_t reader(::logger());
		// Получаем настройки чтения текста разметки
		xml::reader_t::settings_t settings = reader.settings();
		// Выполняем установку признака склейки содержимого
		settings.mergeText = merge;
		// Выполняем установку настроек чтения текста разметки
		reader.settings(settings);
		// Разбираемый текст разметки
		const string text = "<k>WWW</k>";
		// Собранные события содержимого
		vector <string> result;
		// Смещение подачи текста разметки
		size_t offset = 0;
		/**
		 * Выполняем подачу текста разметки кусками
		 */
		while(offset < text.size()){
			// Получаем размер очередного подаваемого куска
			const size_t length = ((piece > 0) ? std::min(piece, text.size() - offset) : (text.size() - offset));
			// Выполняем подачу очередного куска текста разметки
			if(!reader.feed(text.data() + offset, length, ((offset + length) >= text.size())))
				// Прекращаем подачу текста разметки
				break;
			// Выполняем сдвиг смещения подачи текста разметки
			offset += length;
			/**
			 * Выполняем перебор всех событий разбора
			 */
			while(reader.next()){
				/**
				 * Если событие несёт текстовое содержимое узла
				 */
				if(reader.event() == xml::event_t::TEXT)
					// Выполняем сбор текстового содержимого узла
					result.push_back(string(reader.text()));
			}
		}
		// Выводим собранные события содержимого
		return result;
	};
	/**
	 * Выполняем проверку разбора текста целиком
	 */
	{
		// Выполняем проверку того, что содержимое пришло одним событием без склейки
		ASSERT_EQ(собрать(false, 0).size(), static_cast <size_t> (1));
		// Выполняем проверку того, что содержимое пришло одним событием со склейкой
		ASSERT_EQ(собрать(true, 0).size(), static_cast <size_t> (1));
	}
	/**
	 * Выполняем проверку того, что граница подачи содержимое разрывает без склейки
	 */
	{
		// Собранные события содержимого при подаче по одному байту
		const vector <string> parts = собрать(false, 1);
		// Выполняем проверку того, что содержимое разорвано границей подачи
		ASSERT_GT(parts.size(), static_cast <size_t> (1));
		// Собранное содержимое узла
		string whole;
		// Выполняем сложение подряд идущих событий содержимого
		for(auto & item : parts)
			// Выполняем добавление очередного события содержимого
			whole.append(item);
		// Выполняем проверку того, что сложенное содержимое совпало с исходным
		ASSERT_EQ(whole, string("WWW"));
	}
	/**
	 * Выполняем проверку того, что склейка границу подачи переживает
	 */
	{
		// Собранные события содержимого при подаче по одному байту со склейкой
		const vector <string> merged = собрать(true, 1);
		// Выполняем проверку того, что содержимое пришло одним событием
		ASSERT_EQ(merged.size(), static_cast <size_t> (1));
		// Выполняем проверку содержимого узла
		ASSERT_EQ(merged.front(), string("WWW"));
	}
}

/**
 * @brief Проверка точности пределов разбора и единицы их измерения
 *
 * @details Договор у всех пределов один: «наибольшая ДОПУСТИМАЯ». Значит величина,
 * пределу РАВНАЯ, обязана проходить, а превышающая его на единицу - отвергаться
 *
 * @note Второю половиной закрепляется ЕДИНИЦА измерения, и у разметки она иная, чем
 *       у прочих кодеков: предел длины имени договорен в ЗНАКАХ, а не в байтах. Имя
 *       из восьми знаков кириллицы занимает шестнадцать байтов и при пределе в восемь
 *       обязано ПРОХОДИТЬ. Замеряют такое обыкновенно латиницей, где байт со знаком
 *       совпадает, и подмена единицы остаётся невидимой
 *
 * @note Замер 01.09.2026 сличил все девять пределов трёх кодеков в трёх точках
 *       каждый: расхождений ноль
 *
 */
TEST(CodecXmlReader, LimitsAreExactAndNamesCountCharacters) {
	/**
	 * @brief Метод проверки принятия текста разметки при заданных настройках
	 *
	 * @param text     разбираемый текст разметки
	 * @param settings настройки разбора текста
	 * @return         признак принятия текста разбором
	 *
	 */
	const auto accepts = [](const string & text, const xml::reader_t::settings_t & settings) noexcept -> bool {
		// Объект потокового чтения текста разметки
		xml::reader_t reader(::logger(), settings);
		// Выполняем подачу текста разметки целиком
		reader.feed(text.data(), text.size(), true);
		// Выполняем перебор всех событий разбора
		while(reader.next()) ;
		// Выводим признак отсутствия отказа разбора
		return (reader.error() == xml::error_t::NONE);
	};
	// Проверяемая величина предела
	const uint32_t limit = 8;
	/**
	 * Выполняем проверку точности предела глубины вложения
	 */
	{
		// Настройки разбора с пределом глубины вложения
		xml::reader_t::settings_t settings;
		// Выполняем указание предела глубины вложения узлов
		settings.maxDepth = limit;
		/**
		 * @brief Метод сборки разметки заданной глубины вложения
		 *
		 * @param depth глубина вложения собираемой разметки
		 * @return      собранный текст разметки
		 *
		 */
		const auto nest = [](const uint32_t depth) noexcept -> string {
			// Собираемый текст разметки
			string result;
			// Выполняем открытие всех узлов вложения
			for(uint32_t i = 0; i < depth; i++) result.append("<a>");
			// Выполняем закрытие всех узлов вложения
			for(uint32_t i = 0; i < depth; i++) result.append("</a>");
			// Выводим собранный текст разметки
			return result;
		};
		// Выполняем проверку принятия вложения, пределу равного
		ASSERT_TRUE(accepts(nest(limit), settings));
		// Выполняем проверку отказа вложению, предел превысившему
		ASSERT_FALSE(accepts(nest(limit + 1), settings));
	}
	/**
	 * Выполняем проверку точности предела числа признаков узла
	 */
	{
		// Настройки разбора с пределом числа признаков
		xml::reader_t::settings_t settings;
		// Выполняем указание предела числа признаков узла
		settings.maxAttributes = limit;
		/**
		 * @brief Метод сборки узла с заданным числом признаков
		 *
		 * @param count число признаков собираемого узла
		 * @return      собранный текст разметки
		 *
		 */
		const auto node = [](const uint32_t count) noexcept -> string {
			// Собираемый текст разметки
			string result = "<a";
			// Выполняем добавление всех признаков узла
			for(uint32_t i = 0; i < count; i++)
				result.append(" p").append(std::to_string(i)).append("=\"v\"");
			// Выводим собранный текст разметки
			return result.append("/>");
		};
		// Выполняем проверку принятия узла, пределу равного
		ASSERT_TRUE(accepts(node(limit), settings));
		// Выполняем проверку отказа узлу, предел превысившему
		ASSERT_FALSE(accepts(node(limit + 1), settings));
	}
	/**
	 * Выполняем проверку точности предела длины имени
	 */
	{
		// Настройки разбора с пределом длины имени
		xml::reader_t::settings_t settings;
		// Выполняем указание предела длины имени узла
		settings.maxName = limit;
		// Выполняем проверку принятия имени, пределу равного
		ASSERT_TRUE(accepts("<" + string(limit, 'a') + "/>", settings));
		// Выполняем проверку отказа имени, предел превысившему
		ASSERT_FALSE(accepts("<" + string(limit + 1, 'a') + "/>", settings));
	}
	/**
	 * Выполняем проверку того, что предел длины имени меряется знаками, а не байтами
	 */
	{
		// Настройки разбора с пределом длины имени
		xml::reader_t::settings_t settings;
		// Выполняем указание предела длины имени узла
		settings.maxName = limit;
		// Имя из восьми знаков кириллицы, занимающее шестнадцать байтов
		const string wide = "бббббббб";
		// Выполняем проверку того, что имя длиною в восемь знаков занимает шестнадцать байтов
		ASSERT_EQ(wide.size(), static_cast <size_t> (16));
		/**
		 * Выполняем проверку принятия имени, пределу равного знаками
		 *
		 * @note Отказ здесь означал бы счёт байтами вопреки договору
		 */
		ASSERT_TRUE(accepts("<" + wide + "/>", settings));
		// Выполняем проверку отказа имени из девяти знаков кириллицы
		ASSERT_FALSE(accepts("<" + wide + "б/>", settings));
	}
}

/**
 * @brief Проверка стойкости отказа к подаче и полноты его снятия сбросом
 *
 * @details Проверки сброса у этого кодека не было вовсе: `reset()` звался внутри
 * прочих проверок, а обязанности его порознь не закреплялись. Обязанностей две.
 * Первая: подача, поданная ПОСЛЕ отказа, разбор возобновлять не вправе. Вторая:
 * сброс обязан снять не только код отказа, но и место его - оно накапливается по
 * ходу разбора, и застарелое значение указывало бы в текст, которого уже нет
 *
 * @note Тексты подобраны так, что второй отказ наступает БЛИЖЕ первого: равные места
 *       ничего не доказали бы, ибо совпали бы и при застарелом значении
 *
 */
TEST(CodecXmlReader, RefusalSurvivesFeedingAndResetClearsAll) {
	// Объект потокового чтения текста разметки
	xml::reader_t reader(::logger());
	/**
	 * Выполняем проверку того, что подача после отказа разбор не возобновляет
	 */
	{
		// Текст разметки, разбор которого прекращается отказом
		const string text = "<a><b/><c/><d/><e></zzz>";
		// Выполняем подачу текста разметки
		reader.feed(text.data(), text.size(), true);
		// Выполняем снятие всех событий, собранных до отказа
		while(reader.next()) ;
		// Выполняем проверку наступления отказа разбора
		ASSERT_NE(reader.error(), xml::error_t::NONE);
		// Запоминаем код и место первого отказа
		const xml::error_t code = reader.error();
		const uint64_t offset = reader.errorLocation().offset;
		// Выполняем подачу годного текста вслед за отказом
		const string more = "<c/>";
		reader.feed(more.data(), more.size(), true);
		// Количество событий, выданных после отказа
		size_t events = 0;
		// Выполняем перебор всех событий разбора
		while(reader.next()) events++;
		// Выполняем проверку того, что событий после отказа не выдано
		ASSERT_EQ(events, static_cast <size_t> (0));
		// Выполняем проверку сохранности кода отказа
		ASSERT_EQ(reader.error(), code);
		/**
		 * Выполняем сброс состояния разбора
		 */
		reader.reset();
		// Выполняем проверку снятия кода отказа сбросом
		ASSERT_EQ(reader.error(), xml::error_t::NONE);
		/**
		 * Выполняем проверку самостоятельности разбора после сброса
		 */
		{
			// Годный текст разметки
			const string good = "<c/>";
			// Выполняем подачу годного текста разметки
			reader.feed(good.data(), good.size(), true);
			// Количество событий, выданных после сброса
			size_t issued = 0;
			// Выполняем перебор всех событий разбора
			while(reader.next()) issued++;
			// Выполняем проверку выдачи событий после сброса
			ASSERT_GT(issued, static_cast <size_t> (0));
			// Выполняем проверку отсутствия отказа после сброса
			ASSERT_EQ(reader.error(), xml::error_t::NONE);
		}
		/**
		 * Выполняем проверку того, что место второго отказа указывает в новый текст
		 */
		{
			// Выполняем сброс состояния разбора
			reader.reset();
			// Текст разметки, отказ которого наступает у самого начала
			const string second = "<a></zzz>";
			// Выполняем подачу второго текста разметки
			reader.feed(second.data(), second.size(), true);
			// Выполняем снятие всех событий, собранных до отказа
			while(reader.next()) ;
			// Выполняем проверку наступления второго отказа
			ASSERT_NE(reader.error(), xml::error_t::NONE);
			// Выполняем проверку того, что место второго отказа лежит внутри нового текста
			ASSERT_LT(reader.errorLocation().offset, second.size());
			// Выполняем проверку того, что место второго отказа ближе места первого
			ASSERT_LT(reader.errorLocation().offset, offset);
		}
	}
}

/**
 * @brief Проверка кода отказа, возводимого каждым пределом разбора
 *
 * @details Четыре предела разметки отвечают своим кодом, а `maxEvent` - ЕДИНСТВЕННЫЙ
 *          из одиннадцати пределов трёх кодеков, у какого своего кода нет: он отвечает
 *          общим `OVERFLOW_LIMIT`. Расхождение это оговорено у самой настройки
 *
 * @note Щуп первой редакции мерил `maxEntities` встроенными сущностями («&lt;») и
 *       получал отсутствие отказа. Предел считает сущности ОБЪЯВЛЕННЫЕ, и встроенных
 *       не видит вовсе: неверен был щуп, а не кодек
 *
 */
TEST(CodecXmlReader, LimitsRaiseTheirOwnCodes) {
	const auto code = [](const string & text, const xml::reader_t::settings_t & settings) noexcept -> uint32_t {
		xml::reader_t reader(::logger(), settings);
		reader.feed(text.data(), text.size(), true);
		while(reader.next()) ;
		return static_cast <uint32_t> (reader.error());
	};
	{
		xml::reader_t::settings_t s;
		s.maxDepth = 2;
		ASSERT_EQ(code("<a><b><c><d/></c></b></a>", s), static_cast <uint32_t> (xml::error_t::DEPTH_EXCEEDED));
	}
	{
		xml::reader_t::settings_t s;
		s.maxName = 3;
		ASSERT_EQ(code("<abcdef/>", s), static_cast <uint32_t> (xml::error_t::NAME_TOO_LONG));
	}
	{
		xml::reader_t::settings_t s;
		s.maxAttributes = 1;
		ASSERT_EQ(code("<a x=\"1\" y=\"2\"/>", s), static_cast <uint32_t> (xml::error_t::TOO_MANY_ATTRIBUTES));
	}
	{
		xml::reader_t::settings_t s;
		s.maxEvent = 4;
		ASSERT_EQ(code("<a>ааааааааааа</a>", s), static_cast <uint32_t> (xml::error_t::OVERFLOW_LIMIT));
	}
	{
		xml::reader_t::settings_t s;
		s.maxEntities = 2;
		ASSERT_EQ(code("<!DOCTYPE a [<!ENTITY p \"1\"><!ENTITY q \"2\"><!ENTITY r \"3\">]><a/>", s), static_cast <uint32_t> (xml::error_t::ENTITY_COUNT_EXCEEDED));
	}
}



/**
 * @brief Проверка места отказа: оно не зависит от нарезки и пусто при успехе
 *
 * @details Место отказа выдаётся посредником `errorLocation()`, а НЕ `location()`:
 *          последний несёт место текущего события и с местом отказа расходится
 *          законно - у текста «<a x=1/>» отказ стоит на 5, а событие на 0
 *
 */
TEST(CodecXmlReader, ErrorLocationIsIndependentOfChunking) {
	/**
	 * @brief Метод разбора текста с выдачей места отказа
	 *
	 * @param text разбираемый текст разметки
	 * @param step размер куска подачи, ноль - подача целиком
	 * @return     место обнаружения отказа разбора
	 *
	 */
	const auto locate = [](const string & text, const size_t step) noexcept -> xml::location_t {
		// Чтение текста разметки
		xml::reader_t reader(::logger());
		// Если подача идёт текстом целиком
		if(step == 0)
			// Выполняем подачу текста разметки целиком
			reader.feed(text.data(), text.size(), true);
		// Если подача идёт кусками
		else {
			// Выполняем перебор всех кусков подаваемого текста
			for(size_t i = 0; i < text.size(); i += step){
				// Размер подаваемого куска текста
				const size_t size = ((i + step) < text.size() ? step : (text.size() - i));
				// Выполняем подачу очередного куска текста
				reader.feed(text.data() + i, size, ((i + size) >= text.size()));
				// Выполняем перебор всех событий разбора
				while(reader.next()) ;
			}
		}
		// Выполняем перебор всех оставшихся событий разбора
		while(reader.next()) ;
		// Выводим место обнаружения отказа разбора
		return reader.errorLocation();
	};
	/**
	 * Выполняем проверку независимости места отказа от нарезки текста
	 */
	{
		// Проверяемые тексты и ожидаемые смещения отказа в них
		const struct { const char * text; uint64_t offset; } probes[] = {
			{"<a><b></c></a>", 6}, {"<a>\n<b>\n</a>", 8}, {"<a x=1/>", 5},
			{"<a>&nope;</a>", 9}, {"<a/>x", 4}
		};
		// Выполняем перебор всех проверяемых текстов
		for(const auto & probe : probes){
			// Место отказа при подаче текста целиком
			const xml::location_t whole = locate(probe.text, 0);
			// Место отказа при подаче текста по одному байту
			const xml::location_t parts = locate(probe.text, 1);
			// Выполняем проверку указания места на виновный знак
			EXPECT_EQ(whole.offset, probe.offset);
			// Выполняем проверку совпадения места при обеих подачах
			EXPECT_EQ(whole.offset, parts.offset);
			// Выполняем проверку совпадения номера строки при обеих подачах
			EXPECT_EQ(whole.line, parts.line);
			// Выполняем проверку совпадения положения в строке при обеих подачах
			EXPECT_EQ(whole.column, parts.column);
		}
	}
	/**
	 * Выполняем проверку пустоты места отказа при разборе без отказа
	 */
	{
		// Место разбора текста, отказа не вызвавшего
		const xml::location_t clean = locate("<a><b/></a>", 0);
		// Выполняем проверку пустоты смещения места отказа
		EXPECT_EQ(clean.offset, xml::NO_OFFSET);
		// Выполняем проверку пустоты номера строки места отказа
		EXPECT_EQ(clean.line, 0u);
	}
}

/**
 * @brief Проверка области предела количества сущностей
 *
 * @details Предел считает сущности, ОБЪЯВЛЕННЫЕ описанием типа документа, и только их.
 *          Встроенных сущностей разметки он не видит вовсе, и текст, составленный из них
 *          одних, не отвергается никогда - сколько бы их ни было и каким бы малым ни был
 *          предел
 *
 * @note Утверждение это внесено в договор 01.09.2026 по замеру и прежде держалось лишь
 *       записью. Проверка заведена оттого, что зелёное само по себе ничего не говорит:
 *       щуп, случайно мерящий не ту величину, отчитался бы тем же
 *
 */
TEST(CodecXmlReader, EntityLimitCountsDeclaredOnly) {
	/**
	 * @brief Метод разбора текста разметки с выдачей кода отказа
	 *
	 * @param text  разбираемый текст разметки
	 * @param limit предел количества объявленных сущностей
	 * @return      код отказа разбора текста
	 *
	 */
	const auto code = [](const string & text, const uint32_t limit) noexcept -> xml::error_t {
		// Настройки разбора текста разметки
		xml::reader_t::settings_t settings;
		// Выполняем указание предела количества объявленных сущностей
		settings.maxEntities = limit;
		// Чтение текста разметки
		xml::reader_t reader(::logger(), settings);
		// Выполняем подачу текста разметки целиком
		reader.feed(text.data(), text.size(), true);
		// Выполняем перебор всех событий разбора
		while(reader.next()) ;
		// Выводим код отказа разбора текста
		return reader.error();
	};
	/**
	 * Выполняем проверку слепоты предела к встроенным сущностям разметки
	 */
	{
		// Выполняем проверку принятия четырёх встроенных сущностей при пределе в одну
		ASSERT_EQ(code("<a>&lt;&gt;&amp;&quot;</a>", 1), xml::error_t::NONE);
		// Выполняем проверку принятия их же при пределе, меньше какого не бывает
		ASSERT_EQ(code("<a>&lt;&gt;&amp;&quot;&apos;</a>", 1), xml::error_t::NONE);
	}
	/**
	 * Выполняем проверку точности предела на объявленных сущностях
	 */
	{
		// Описание типа документа с двумя объявленными сущностями
		const string two = "<!DOCTYPE a [<!ENTITY p \"1\"><!ENTITY q \"2\">]><a/>";
		// Описание типа документа с тремя объявленными сущностями
		const string three = "<!DOCTYPE a [<!ENTITY p \"1\"><!ENTITY q \"2\"><!ENTITY r \"3\">]><a/>";
		// Выполняем проверку принятия количества, пределу равного
		ASSERT_EQ(code(two, 2), xml::error_t::NONE);
		// Выполняем проверку отказа количеству, предел превысившему
		ASSERT_EQ(code(three, 2), xml::error_t::ENTITY_COUNT_EXCEEDED);
	}
	/**
	 * Выполняем проверку того, что встроенные сущности к объявленным не приплюсовываются
	 *
	 * @note Проверка эта отделяет слепоту предела от простого запаса: будь встроенные
	 *       сочтены наравне, объявленные две вместе с четырьмя встроенными предел в две
	 *       превысили бы
	 */
	{
		// Текст с двумя объявленными сущностями и четырьмя встроенными
		const string mixed = "<!DOCTYPE a [<!ENTITY p \"1\"><!ENTITY q \"2\">]><a>&lt;&gt;&amp;&quot;</a>";
		// Выполняем проверку принятия текста при пределе в две сущности
		ASSERT_EQ(code(mixed, 2), xml::error_t::NONE);
	}
}

TEST(CodecXmlReader, ProbeSurprisingCodes) {
	const char * texts[] = {
		"<a></b>", "<a>", "</a>", "<a/><b/>", "<a x/>", "<a x=y/>", "<a x='1' x='2'/>",
		"<a>&#xZ;</a>", "<a>&#;</a>", "<a><!-- -- --></a>", "<a><![CDATA[]]]]></a>",
		"<?xml version='2.0'?><a/>", "<?XML version='1.0'?><a/>", "<a>]]></a>", "text<a/>",
		"<a:b/>", "<a xmlns:xml='x'/>", "<!DOCTYPE a><b/>", "<a>&#x110000;</a>"
	};
	for(const char * text : texts){
		xml::reader_t reader(::logger());
		reader.feed(text, ::strlen(text), true);
		while(reader.next()) ;
		::printf("ЩУП %-28s -> 0x%02X %s\n", text, static_cast <uint32_t> (reader.error()),
			xml::message(reader.error()));
	}
}

/**
 * @brief Проверка того, что разбор по описанию типа документа НЕ поверяет
 *
 * @details Решение это объявлено намеренным в разделе «Намеренные решения» договора, и
 *          там же сказано, что всякое из решений закреплено испытанием. Для ЭТОГО решения
 *          испытания не было: замер 01.09.2026 нашёл лишь упоминания в записках соседних
 *          проверок, а сличения поведения - ни одного
 *
 * @note Взяты пять видов несоответствия описанию, и всякий обязан быть ПРИНЯТ: имя корня,
 *       описанию противное; узел, содержательной моделью не дозволенный; отсутствие
 *       обязательного атрибута; содержимое у узла, объявленного пустым; атрибут, вовсе не
 *       объявленный. Правильность ПОСТРОЕНИЯ при том проверяется наравне с прочим - здесь
 *       снимается именно поверка СООТВЕТСТВИЯ, а не разбор описания
 *
 * @warning Поверка по описанию, по схеме XSD либо по договору RelaxNG есть работа
 *          отдельная. Заведи её кто-нибудь внутри разбора - и проверка эта покраснеет
 *          пятью утверждениями сразу, что и требуется: решение владельца меняется
 *          осознанно, а не по дороге
 *
 */
TEST(CodecXmlReader, DocumentTypeIsParsedButNotValidated) {
	/**
	 * @brief Метод разбора текста разметки с выдачей кода отказа
	 *
	 * @param text разбираемый текст разметки
	 * @return     код отказа разбора текста
	 *
	 */
	const auto code = [](const char * text) noexcept -> xml::error_t {
		// Чтение текста разметки
		xml::reader_t reader(::logger());
		// Выполняем подачу текста разметки целиком
		reader.feed(text, ::strlen(text), true);
		// Выполняем перебор всех событий разбора
		while(reader.next()) ;
		// Выводим код отказа разбора текста
		return reader.error();
	};
	// Выполняем проверку принятия корня, имени в описании типа противного
	EXPECT_EQ(code("<!DOCTYPE a><b/>"), xml::error_t::NONE);
	// Выполняем проверку принятия узла, содержательной моделью не дозволенного
	EXPECT_EQ(code("<!DOCTYPE a [<!ELEMENT a (b)>]><a><c/></a>"), xml::error_t::NONE);
	// Выполняем проверку принятия узла без обязательного по описанию атрибута
	EXPECT_EQ(code("<!DOCTYPE a [<!ATTLIST a x CDATA #REQUIRED>]><a/>"), xml::error_t::NONE);
	// Выполняем проверку принятия содержимого у узла, объявленного пустым
	EXPECT_EQ(code("<!DOCTYPE a [<!ELEMENT a EMPTY>]><a>текст</a>"), xml::error_t::NONE);
	// Выполняем проверку принятия атрибута, описанием не объявленного
	EXPECT_EQ(code("<!DOCTYPE a [<!ATTLIST a x CDATA #IMPLIED>]><a y=\'1\'/>"), xml::error_t::NONE);
	/**
	 * Выполняем проверку того, что правильность ПОСТРОЕНИЯ при этом проверяется
	 *
	 * @note Утверждение это стоит здесь сторожем: без него проверка выше неотличима от
	 *       разбора, не проверяющего вовсе ничего
	 */
	{
		// Выполняем проверку отказа на несовпадение меток при том же описании типа
		EXPECT_EQ(code("<!DOCTYPE a><a></b>"), xml::error_t::MISMATCHED_TAG);
		// Выполняем проверку отказа на ошибочное построение самого описания типа
		EXPECT_EQ(code("<!DOCTYPE ><a/>"), xml::error_t::INVALID_DOCTYPE);
	}
}
