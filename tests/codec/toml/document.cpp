/**
 * @file document.cpp
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
 * @brief Проверки дерева настроек TOML — сборка дерева разбором, чтение значений по
 *        составному имени, правка записей на месте, удаление пар и таблиц, а также
 *        договор о сохранении оформления при обратной записи
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем системные заголовочные файлы
 *
 * @note Заголовок алгоритмов подключается прямо: подсчёт знаков конца строки идёт
 *       через std::count, а достаётся он не всякой стандартной библиотекой исподволь
 *       - у Solaris сборка отвечала «count was not declared in this scope»
 */
#include <algorithm>

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
 * @brief Разбираемый текст настроек проверок дерева
 *
 * @details Текст записан тем же оформлением, каким его собирает запись: договор о
 * сохранении оформления меряется побайтовым совпадением, и расхождение украшений
 * пришлось бы принимать за потерю
 *
 */
static const char * SAMPLE =
 "# настройки сервера\n"
 "title = \"пример\"\n"
 "\n"
 "[server]\n"
 "host = 'локальный' # хозяин\n"
 "port = 8080\n"
 "ratio = 0.25\n"
 "flags = [true, false]\n"
 "point = { x = 1, y = 2 }\n"
 "\n"
 "[server.limits]\n"
 "depth = 0x10\n"
 "\n"
 "[[products]]\n"
 "name = \"гвоздь\"\n"
 "\n"
 "[[products]]\n"
 "name = \"молоток\"\n";

/**
 * @brief Проверка чтения значений по составному имени
 *
 */
/**
 * @brief Проверка постановки владеющего значения обоими концами
 *
 * @details Правка дерева зовётся у кодеков рамки С РАЗНЫХ КОНЦОВ: у одних ходом
 *          значения (`value.graft(документ, путь)`), у других ходом документа
 *          (`документ.set(путь, значение)`). Переименованием расхождение это не
 *          снимается - имя вышло бы одно, а порядок слов обратный, - и потребитель,
 *          два кодека рядом читающий, разыскивал ход не там, где тот лежал. Оттого
 *          заведены ОБА конца, и проверка держит их равенство
 *
 */
/**
 * @brief Проверка счёта size() детьми корня
 *
 * @details Счёт этот един у всех семи кодеков рамки. Прежде под именем `size()` лежали
 *          ТРИ РАЗНЫЕ величины - узлы арены у YAML, объявленные разделы у INI,
 *          объявленные таблицы у TOML, - и общего договора из них не выходило вовсе
 *
 * @note Величина, от устройства либо от вида записи зависящая, получает СВОЁ имя:
 *       объявленных таблиц. Проверка держит обе стороны разом - и новый смысл `size()`, и то, что
 *       прежний счёт никуда не делся, а лишь переехал. Без второй стороны смена вышла
 *       бы молчаливой: зовущий получил бы другое число без единого признака беды
 *
 * @note Смена согласована с владельцами прочих кодеков 03.09.2026
 *
 */
TEST(CodecTomlDocument, SizeCountsChildrenOfTheRoot){
	// Объект дерева документа, текст разбирающий
	toml::document_t document(::logger());
	// Выполняем разбор текста о двух детях корня
	ASSERT_TRUE(document.parse("a = 1\n[b]\nc = 2\nd = 3\n")) << toml::message(document.error());
	// Выполняем проверку того, что size() считает ДЕТЕЙ КОРНЯ
	ASSERT_EQ(document.size(), 2u);
	// Выполняем проверку того, что счёт дерева согласен со счётом владеющего значения
	ASSERT_EQ(document.size(), toml::value_t(document).size());
	// Выполняем проверку того, что прежний счёт никуда не делся, а лишь переехал
	ASSERT_NE(document.tables().size(), 0u);
}
TEST(CodecTomlDocument, SettingByBothEndsAgrees) {
	// Объект дерева документа, концом документа правимый
	toml::document_t first(::logger());
	// Объект дерева документа, концом значения правимый
	toml::document_t second(::logger());
	// Выполняем разбор одного и того же текста обоими деревьями
	ASSERT_TRUE(first.parse("a = 1\n")) << toml::message(first.error());
	ASSERT_TRUE(second.parse("a = 1\n")) << toml::message(second.error());
	// Ставимое владеющее значение
	const toml::value_t item(string("anyks"));
	// Выполняем постановку значения концом ДОКУМЕНТА
	ASSERT_TRUE(first.set("/b", item)) << toml::message(first.error());
	// Выполняем постановку того же значения концом ЗНАЧЕНИЯ
	ASSERT_TRUE(item.graft(second, vector <string_view>{"b"})) << toml::message(second.error());
	// Выполняем проверку того, что оба конца дали один и тот же текст
	ASSERT_EQ(first.dump(), second.dump());
}
TEST(CodecTomlDocument, Reading) {
	// Объект дерева настроек
	toml::document_t document(::logger());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(SAMPLE)) << static_cast <uint32_t> (document.error());
	// Выполняем проверку чтения значения верхнего уровня
	ASSERT_EQ(document.text({"title"}), "пример");
	// Выполняем проверку чтения строкового значения таблицы
	ASSERT_EQ(document.text({"server", "host"}), "локальный");
	// Целое число значения пары
	int64_t port = 0;
	// Выполняем проверку чтения целого числа
	ASSERT_TRUE(document.value(port, {"server", "port"}));
	// Выполняем проверку прочитанного целого числа
	ASSERT_EQ(port, 8080);
	// Число с плавающей точкой значения пары
	double ratio = 0.0;
	// Выполняем проверку чтения числа с плавающей точкой
	ASSERT_TRUE(document.value(ratio, {"server", "ratio"}));
	// Выполняем проверку прочитанного числа с плавающей точкой
	ASSERT_EQ(ratio, 0.25);
	// Целое число значения вложенной таблицы
	int64_t depth = 0;
	// Выполняем проверку чтения значения вложенной таблицы
	ASSERT_TRUE(document.value(depth, {"server", "limits", "depth"}));
	// Выполняем проверку прочитанного значения вложенной таблицы
	ASSERT_EQ(depth, 16);
	// Выполняем проверку отсутствия незаписанной пары
	ASSERT_FALSE(document.has({"server", "missing"}));
	// Выполняем проверку типа значения пары
	ASSERT_EQ(document.type({"server", "flags"}), toml::type_t::ARRAY);
}
/**
 * @brief Проверка договора извлечения числа
 *
 * @details Отказом извлечение завершается лишь тогда, когда значение числом не является
 * вовсе: вид хранения ему не указ, а сужение выполняется приведением языка
 *
 * @note Проверка эта прежде закрепляла соблюдение вида значения и отвергала как чтение
 *       дробного целым, так и выход за отрезок вида. Отменено владельцем 20.08.2026:
 *       договор извлечения общий у всех кодеков рамки, а приведение языка не отказывает
 *       нигде. Отказ остался за одним лишь чтением числа логическим значением - число
 *       логическим значением не является
 *
 */
TEST(CodecTomlDocument, Typed) {
	// Объект дерева настроек
	toml::document_t document(::logger());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(SAMPLE));
	// Целое число значения пары
	int64_t integer = 0;
	// Выполняем проверку чтения числа с плавающей точкой целым
	ASSERT_TRUE(document.value(integer, {"server", "ratio"}));
	/**
	 * Выполняем проверку прочитанного числа
	 *
	 * @note Запись «0.25» округляется к нулю: половины в ней нет, и увод её от нуля к
	 *       делу не относится
	 */
	ASSERT_EQ(integer, 0);
	/**
	 * Выполняем проверку увода половины от нуля, а не к ближайшему чётному
	 *
	 * @note Случай этот отделяет `round` от `rint`: округление к ближайшему чётному
	 *       отдало бы здесь двойку, а увод половины от нуля даёт тройку. Расходятся две
	 *       разновидности ровно на половинах, и без такой проверки подмена одной другою
	 *       прошла бы незамеченной
	 */
	{
		// Объект дерева настроек половинной записи
		toml::document_t halved(::logger());
		// Выполняем разбор текста настроек
		ASSERT_TRUE(halved.parse("half = 2.5\nback = -2.5\n"));
		// Выполняем проверку чтения половинной записи целым видом
		ASSERT_TRUE(halved.value(integer, {"half"}));
		// Выполняем проверку прочитанного числа
		ASSERT_EQ(integer, 3);
		// Выполняем проверку чтения отрицательной половинной записи целым видом
		ASSERT_TRUE(halved.value(integer, {"back"}));
		// Выполняем проверку прочитанного числа
		ASSERT_EQ(integer, -3);
	}
	// Логическое значение пары
	bool boolean = false;
	// Выполняем проверку отказа чтения целого числа логическим значением
	ASSERT_FALSE(document.value(boolean, {"server", "port"}));
	// Целое число значения пары, отрезком типа не вмещаемое
	int8_t narrow = 0;
	// Выполняем проверку чтения числа сверх отрезка значений типа
	ASSERT_TRUE(document.value(narrow, {"server", "port"}));
	/**
	 * Выполняем проверку прочитанного числа
	 *
	 * @note Запись «8080» переносится младшими разрядами: младший байт её есть `0x90`,
	 *       а знаковым однобайтовым видом он читается как `-112`
	 */
	ASSERT_EQ(narrow, -112);
	// Число с плавающей точкой значения пары
	double real = 0.0;
	/**
	 * Выполняем проверку чтения целого числа с плавающей точкой
	 *
	 * @note Приведение это значения не искажает, и оно дозволено
	 */
	ASSERT_TRUE(document.value(real, {"server", "port"}));
	// Выполняем проверку прочитанного числа
	ASSERT_EQ(real, 8080.0);
}
/**
 * @brief Проверка чтения перечня значений и встроенной таблицы
 *
 */
/**
 * @brief Проверка подачи документу значения, с него же и снятого
 *
 * @details Значение отдаётся видом в хранилище знаков самого документа, и подача его
 *          обратно в `set()` есть прямой способ размножить настройку: долив хранилища
 *          его перераспределяет, то есть читается то самое место, какое освобождается.
 *          У соседнего кодека CSV подобное было настоящим дефектом, и отличие здесь
 *          лишь в том, что долив ровно один, а не по полю на запись
 *
 */
TEST(CodecTomlDocument, SetFromOwnView) {
	// Объект дерева настроек
	toml::document_t document(::logger());
	// Значение, заведомо превышающее короткий запас строки
	const string big(4096, 'z');
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse("[server]\nhost = \"" + big + "\"\n"));
	/**
	 * Выполняем набивку дерева, заведомо перераспределяющую хранилище знаков
	 */
	for(size_t i = 0; i < 64; i++)
		// Выполняем установку очередного значения
		ASSERT_TRUE(document.set({"server", "поле" + to_string(i)}, string(256, 'a')));
	// Выполняем снятие значения видом в хранилище знаков
	const string_view value = document.text({"server", "host"});
	// Выполняем проверку длины снятого значения
	ASSERT_EQ(value.length(), big.length());
	// Выполняем подачу снятого значения тому же документу
	ASSERT_TRUE(document.set({"server", "копия"}, value));
	// Выполняем проверку того, что значение перенесено целым
	ASSERT_EQ(document.text({"server", "копия"}), big);
	// Выполняем проверку сохранности исходного значения
	ASSERT_EQ(document.text({"server", "host"}), big);
}

TEST(CodecTomlDocument, Composite) {
	// Объект дерева настроек
	toml::document_t document(::logger());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(SAMPLE));
	// Выполняем проверку количества значений перечня
	ASSERT_EQ(document.length({"server", "flags"}), 2u);
	// Прочитанное значение перечня
	toml::content_t value;
	// Выполняем проверку чтения первого значения перечня
	ASSERT_TRUE(document.item({"server", "flags"}, 0, value));
	// Выполняем проверку типа прочитанного значения
	ASSERT_EQ(value.type, toml::type_t::BOOLEAN);
	// Выполняем проверку прочитанного логического значения
	ASSERT_TRUE(value.boolean);
	// Выполняем проверку отказа чтения значения перечня за его пределами
	ASSERT_FALSE(document.item({"server", "flags"}, 2, value));
	// Выполняем проверку типа значения встроенной таблицы
	ASSERT_EQ(document.type({"server", "point"}), toml::type_t::TABLE);
}
/**
 * @brief Проверка перечней таблиц и дочерних имён
 *
 */
TEST(CodecTomlDocument, Structure) {
	// Объект дерева настроек
	toml::document_t document(::logger());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(SAMPLE));
	// Получаем перечень объявленных таблиц
	const vector <vector <string_view>> tables = document.tables();
	// Выполняем проверку количества объявленных таблиц
	ASSERT_EQ(tables.size(), 4u);
	// Выполняем проверку количества объявленных таблиц
	ASSERT_EQ(document.tables().size(), 4u);
	// Выполняем проверку имени первой объявленной таблицы
	ASSERT_EQ(tables.at(0).size(), 1u);
	// Выполняем проверку имени первой объявленной таблицы
	ASSERT_EQ(tables.at(0).at(0), "server");
	// Выполняем проверку составного имени вложенной таблицы
	ASSERT_EQ(tables.at(1).size(), 2u);
	// Выполняем проверку наличия объявленной таблицы
	ASSERT_TRUE(document.table({"server", "limits"}));
	// Выполняем проверку отсутствия необъявленной таблицы
	ASSERT_FALSE(document.table({"server", "missing"}));
	// Получаем перечень дочерних имён таблицы
	const vector <string_view> keys = document.keys({"server"});
	/**
	 * Выполняем проверку количества дочерних имён таблицы
	 *
	 * @note Вложенная таблица дочерним именем своей объемлющей и является: имя
	 *       «limits» идёт наравне с именами пар
	 */
	ASSERT_EQ(keys.size(), 6u);
	// Выполняем проверку первого дочернего имени таблицы
	ASSERT_EQ(keys.at(0), "host");
	// Выполняем проверку последнего дочернего имени пар таблицы
	ASSERT_EQ(keys.at(4), "point");
	// Выполняем проверку дочернего имени вложенной таблицы
	ASSERT_EQ(keys.at(5), "limits");
	// Получаем перечень дочерних имён верхнего уровня
	const vector <string_view> roots = document.keys();
	// Выполняем проверку количества дочерних имён верхнего уровня
	ASSERT_EQ(roots.size(), 3u);
	// Выполняем проверку первого дочернего имени верхнего уровня
	ASSERT_EQ(roots.at(0), "title");
}
/**
 * @brief Проверка обращения к таблицам набора таблиц
 *
 * @details Имя у таблиц набора общее, и различить их можно лишь порядковым номером
 * частью составного имени
 *
 */
TEST(CodecTomlDocument, ArrayTables) {
	// Объект дерева настроек
	toml::document_t document(::logger());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(SAMPLE));
	// Выполняем проверку количества таблиц набора таблиц
	ASSERT_EQ(document.count({"products"}), 2u);
	// Выполняем проверку чтения значения первой таблицы набора
	ASSERT_EQ(document.text({"products", "0", "name"}), "гвоздь");
	// Выполняем проверку чтения значения второй таблицы набора
	ASSERT_EQ(document.text({"products", "1", "name"}), "молоток");
	// Выполняем проверку наличия таблицы набора таблиц
	ASSERT_TRUE(document.table({"products", "1"}));
	// Выполняем проверку отсутствия таблицы набора за его пределами
	ASSERT_FALSE(document.table({"products", "2"}));
	// Выполняем проверку нулевого количества таблиц у набора, не объявленного вовсе
	ASSERT_EQ(document.count({"missing"}), 0u);
}
/**
 * @brief Проверка обратной записи дерева настроек
 *
 * @details Договор о сохранении оформления: перезапись обязана возвращать файл,
 * узнаваемый его хозяином, - с примечаниями, пустыми строками, порядком записей,
 * оградой строк и системой счисления чисел, какими их выбрал человек
 *
 */
TEST(CodecTomlDocument, Rewrite) {
	// Объект дерева настроек
	toml::document_t document(::logger());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(SAMPLE));
	// Выполняем проверку побайтового совпадения перезаписи с исходным текстом
	ASSERT_EQ(document.text(), SAMPLE);
}
/**
 * @brief Проверка сохранения многострочной записи перечня
 *
 */
TEST(CodecTomlDocument, MultilineArray) {
	// Разбираемый текст настроек
	const string text =
	 "hosts = [\n"
	 "\t\"первый\",\n"
	 "\t\"второй\"\n"
	 "]\n"
	 "ports = [80, 443]\n";
	// Объект дерева настроек
	toml::document_t document(::logger());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(text)) << static_cast <uint32_t> (document.error());
	/**
	 * Выполняем проверку сохранения расстановки строк перечня
	 *
	 * @note Расстановка эта выбрана человеком, и разбор о ней не сообщает: узнаётся
	 *       она по местам событий начала и конца перечня
	 */
	ASSERT_EQ(document.text(), text);
}
/**
 * @brief Проверка правки значения объявленной пары
 *
 * @details Значение заменяется в собственной записи пары, отчего ни порядок, ни
 * примечания, ни пустые строки не страдают
 *
 */
TEST(CodecTomlDocument, Editing) {
	// Объект дерева настроек
	toml::document_t document(::logger());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(SAMPLE));
	// Выполняем установку значения объявленной пары
	ASSERT_TRUE(document.set({"server", "port"}, static_cast <int64_t> (9090)));
	// Целое число значения пары
	int64_t port = 0;
	// Выполняем проверку чтения установленного значения
	ASSERT_TRUE(document.value(port, {"server", "port"}));
	// Выполняем проверку прочитанного значения
	ASSERT_EQ(port, 9090);
	// Получаем собранный текст настроек
	const string result = document.text();
	// Выполняем проверку записи установленного значения
	ASSERT_NE(result.find("port = 9090\n"), string::npos);
	// Выполняем проверку сохранения примечания в конце строки
	ASSERT_NE(result.find("host = 'локальный' # хозяин\n"), string::npos);
	// Получаем исходный текст настроек
	const string source(SAMPLE);
	// Выполняем проверку сохранения количества строк текста настроек
	ASSERT_EQ(count(result.begin(), result.end(), '\n'), count(source.begin(), source.end(), '\n'));
}
/**
 * @brief Проверка заведения отсутствующей пары
 *
 * @details Отсутствующая пара дописывается в конец своей таблицы, а не в конец
 * текста: место её определяется именем, а не порядком правок
 *
 */
TEST(CodecTomlDocument, Appending) {
	// Объект дерева настроек
	toml::document_t document(::logger());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(SAMPLE));
	// Выполняем заведение отсутствующей пары объявленной таблицы
	ASSERT_TRUE(document.set({"server", "backlog"}, static_cast <int64_t> (128)));
	// Выполняем заведение отсутствующей пары верхнего уровня
	ASSERT_TRUE(document.set({"version"}, "1.0"));
	// Получаем собранный текст настроек
	const string result = document.text();
	// Выполняем проверку записи заведённой пары таблицы
	ASSERT_NE(result.find("point = { x = 1, y = 2 }\nbacklog = 128\n"), string::npos);
	/**
	 * Выполняем проверку записи заведённой пары верхнего уровня
	 *
	 * @note Пара верхнего уровня записана до первого объявления таблицы: за ним она
	 *       принадлежала бы уже той таблице
	 */
	ASSERT_NE(result.find("version = \"1.0\"\n"), string::npos);
	// Выполняем проверку того, что пара верхнего уровня записана до объявления таблицы
	ASSERT_LT(result.find("version = "), result.find("[server]"));
	// Выполняем проверку чтения заведённой пары верхнего уровня
	ASSERT_EQ(document.text({"version"}), "1.0");
}
/**
 * @brief Проверка заведения пары необъявленной таблицы
 *
 */
TEST(CodecTomlDocument, Creating) {
	// Объект дерева настроек
	toml::document_t document(::logger());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(SAMPLE));
	// Выполняем объявление отсутствующей таблицы
	ASSERT_TRUE(document.create({"client"}));
	// Выполняем повторное объявление таблицы
	ASSERT_TRUE(document.create({"client"}));
	// Выполняем проверку того, что таблица объявлена однажды
	ASSERT_EQ(document.tables().size(), 5u);
	// Выполняем заведение пары объявленной таблицы
	ASSERT_TRUE(document.set({"client", "retries"}, static_cast <int64_t> (3)));
	// Получаем собранный текст настроек
	const string result = document.text();
	// Выполняем проверку записи объявленной таблицы с её парой
	ASSERT_NE(result.find("[client]\nretries = 3\n"), string::npos);
	// Выполняем проверку разбора собранного текста настроек
	toml::document_t reread(::logger());
	// Выполняем разбор собранного текста настроек
	ASSERT_TRUE(reread.parse(result)) << static_cast <uint32_t> (reread.error());
	// Выполняем проверку чтения заведённого значения
	ASSERT_TRUE(reread.has({"client", "retries"}));
}
/**
 * @brief Проверка удаления пары
 *
 */
TEST(CodecTomlDocument, Erasing) {
	// Объект дерева настроек
	toml::document_t document(::logger());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(SAMPLE));
	// Выполняем удаление объявленной пары
	ASSERT_TRUE(document.erase({"server", "host"}));
	// Выполняем проверку отсутствия удалённой пары
	ASSERT_FALSE(document.has({"server", "host"}));
	// Выполняем проверку отказа повторного удаления пары
	ASSERT_FALSE(document.erase({"server", "host"}));
	// Получаем собранный текст настроек
	const string result = document.text();
	// Выполняем проверку отсутствия удалённой пары в собранном тексте
	ASSERT_EQ(result.find("host = "), string::npos);
	/**
	 * Выполняем проверку удаления примечания в конце строки
	 *
	 * @note Примечание это писано к удалённой паре: оставить его значило бы оставить
	 *       пояснение к тому, чего в файле более нет
	 */
	ASSERT_EQ(result.find("хозяин"), string::npos);
	// Выполняем проверку сохранения примечания отдельной строкой
	ASSERT_NE(result.find("# настройки сервера"), string::npos);
	// Выполняем проверку сохранения прочих пар таблицы
	ASSERT_NE(result.find("port = 8080"), string::npos);
}
/**
 * @brief Проверка удаления таблицы
 *
 */
TEST(CodecTomlDocument, Removing) {
	// Объект дерева настроек
	toml::document_t document(::logger());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(SAMPLE));
	// Выполняем удаление объявленной таблицы
	ASSERT_TRUE(document.remove({"server"}));
	// Выполняем проверку отсутствия удалённой таблицы
	ASSERT_FALSE(document.table({"server"}));
	// Выполняем проверку отсутствия пар удалённой таблицы
	ASSERT_FALSE(document.has({"server", "port"}));
	/**
	 * Выполняем проверку сохранения вложенной таблицы
	 *
	 * @note Вложенная таблица объявлена своей записью, и удаление объемлющей её не
	 *       касается: удаляются записи до следующего объявления
	 */
	ASSERT_TRUE(document.table({"server", "limits"}));
	// Получаем собранный текст настроек
	const string result = document.text();
	// Выполняем проверку отсутствия удалённой таблицы в собранном тексте
	ASSERT_EQ(result.find("[server]"), string::npos);
	// Выполняем проверку сохранения вложенной таблицы в собранном тексте
	ASSERT_NE(result.find("[server.limits]"), string::npos);
	/**
	 * Выполняем проверку отказа удаления необъявленной таблицы
	 *
	 * @note Отказ отвечает кодом отсутствия таблицы, а не ошибочного построения её
	 *       объявления: имя составлено верно, таблицы с ним в дереве нет
	 */
	ASSERT_FALSE(document.remove({"missing"}));
	// Выполняем проверку кода ошибки правки дерева
	ASSERT_EQ(document.error(), toml::error_t::UNKNOWN_TABLE);
}
/**
 * @brief Проверка сохранения выбранной человеком записи значений
 *
 */
TEST(CodecTomlDocument, Preserving) {
	// Объект дерева настроек
	toml::document_t document(::logger());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(SAMPLE));
	// Прочитанное значение пары
	toml::content_t value;
	// Выполняем чтение значения пары
	ASSERT_TRUE(document.get({"server", "host"}, value));
	// Выполняем проверку сохранения ограды строкового значения
	ASSERT_EQ(value.quoting, toml::string_t::LITERAL);
	// Выполняем чтение значения пары вложенной таблицы
	ASSERT_TRUE(document.get({"server", "limits", "depth"}, value));
	// Выполняем проверку сохранения системы счисления записи числа
	ASSERT_EQ(value.radix, toml::radix_t::HEX);
	// Выполняем проверку записи числа выбранной человеком системой счисления
	ASSERT_NE(document.text().find("depth = 0x10\n"), string::npos);
}
/**
 * @brief Проверка установки значений всех простых типов
 *
 */
TEST(CodecTomlDocument, Values) {
	// Объект дерева настроек
	toml::document_t document(::logger());
	// Выполняем установку строкового значения пары
	ASSERT_TRUE(document.set({"text"}, "значение"));
	// Выполняем установку логического значения пары
	ASSERT_TRUE(document.set({"flag"}, true));
	// Выполняем установку целого числа значением пары
	ASSERT_TRUE(document.set({"count"}, static_cast <int64_t> (7), toml::radix_t::HEX));
	// Выполняем установку числа с плавающей точкой значением пары
	ASSERT_TRUE(document.set({"ratio"}, 1.0));
	// Устанавливаемая отметка времени
	toml::stamp_t stamp;
	// Устанавливаем год отметки времени
	stamp.date.year = 2026;
	// Устанавливаем месяц отметки времени
	stamp.date.month = 8;
	// Устанавливаем день отметки времени
	stamp.date.day = 12;
	// Выполняем установку отметки времени значением пары
	ASSERT_TRUE(document.set({"date"}, stamp, toml::type_t::LOCAL_DATE));
	/**
	 * Выполняем проверку собранного текста настроек
	 *
	 * @note Строковое значение, записанное прямо в месте вызова, установлено
	 *       строкою, а не логическим значением
	 */
	ASSERT_EQ(document.text(),
	 "text = \"значение\"\n"
	 "flag = true\n"
	 "count = 0x7\n"
	 "ratio = 1.0\n"
	 "date = 2026-08-12\n");
	// Выполняем проверку отказа установки отрицательного числа системой с приставкой
	ASSERT_FALSE(document.set({"wrong"}, static_cast <int64_t> (-1), toml::radix_t::HEX));
	// Выполняем проверку кода ошибки правки дерева
	ASSERT_EQ(document.error(), toml::error_t::INVALID_NUMBER);
}
/**
 * @brief Проверка замены составного значения простым
 *
 * @details Прежнее значение вправе нести перечень со вложенными узлами, и правка его
 * на месте оставила бы их обрывками
 *
 */
TEST(CodecTomlDocument, Replacing) {
	// Объект дерева настроек
	toml::document_t document(::logger());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(SAMPLE));
	// Выполняем замену перечня значений простым значением
	ASSERT_TRUE(document.set({"server", "flags"}, static_cast <int64_t> (1)));
	// Выполняем проверку смены типа значения пары
	ASSERT_EQ(document.type({"server", "flags"}), toml::type_t::INTEGER);
	// Выполняем проверку нулевой длины перечня, перечнем более не являющегося
	ASSERT_EQ(document.length({"server", "flags"}), 0u);
	// Получаем собранный текст настроек
	const string result = document.text();
	// Выполняем проверку записи установленного значения
	ASSERT_NE(result.find("flags = 1\n"), string::npos);
	// Объект дерева настроек для разбора собранного текста
	toml::document_t reread(::logger());
	// Выполняем проверку разбора собранного текста настроек
	ASSERT_TRUE(reread.parse(result)) << static_cast <uint32_t> (reread.error());
}
/**
 * @brief Проверка отказа разбора ошибочно построенного текста
 *
 */
TEST(CodecTomlDocument, Failure) {
	// Объект дерева настроек
	toml::document_t document(::logger());
	// Выполняем проверку отказа разбора ошибочно построенного текста
	ASSERT_FALSE(document.parse("[server]\nport = \n"));
	// Выполняем проверку кода ошибки разбора текста настроек
	ASSERT_EQ(document.error(), toml::error_t::MISSING_VALUE);
	// Выполняем проверку номера строки обнаружения ошибки
	ASSERT_EQ(document.errorLocation().line, 2u);
	// Выполняем проверку того, что дерево настроек освобождено
	ASSERT_TRUE(document.empty());
	// Выполняем проверку того, что собранный текст настроек пуст
	ASSERT_TRUE(document.text().empty());
}
/**
 * @brief Проверка отказа правки непригодным составным именем
 *
 */
TEST(CodecTomlDocument, Rejection) {
	// Объект дерева настроек
	toml::document_t document(::logger());
	// Выполняем проверку отказа установки значения пустым составным именем
	ASSERT_FALSE(document.set({}, static_cast <int64_t> (1)));
	// Выполняем проверку кода ошибки правки дерева
	ASSERT_EQ(document.error(), toml::error_t::EMPTY_KEY);
	// Выполняем проверку отказа установки значения именем с управляющим знаком
	ASSERT_FALSE(document.set({string_view("имя\nключа")}, static_cast <int64_t> (1)));
	// Выполняем проверку кода ошибки правки дерева
	ASSERT_EQ(document.error(), toml::error_t::INVALID_KEY);
	// Выполняем проверку того, что дерево настроек осталось нетронутым
	ASSERT_TRUE(document.empty());
}
/**
 * @brief Проверка правки составным именем ключа
 *
 */
TEST(CodecTomlDocument, DottedKeys) {
	// Разбираемый текст настроек
	const string text = "[server]\nlimits.depth = 1\n";
	// Объект дерева настроек
	toml::document_t document(::logger());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(text)) << static_cast <uint32_t> (document.error());
	// Целое число значения пары
	int64_t depth = 0;
	// Выполняем проверку чтения значения составным именем ключа
	ASSERT_TRUE(document.value(depth, {"server", "limits", "depth"}));
	// Выполняем проверку прочитанного значения
	ASSERT_EQ(depth, 1);
	// Получаем перечень дочерних имён таблицы
	const vector <string_view> keys = document.keys({"server"});
	// Выполняем проверку количества дочерних имён таблицы
	ASSERT_EQ(keys.size(), 1u);
	// Выполняем проверку дочернего имени таблицы
	ASSERT_EQ(keys.at(0), "limits");
	// Выполняем проверку побайтового совпадения перезаписи с исходным текстом
	ASSERT_EQ(document.text(), text);
	// Выполняем правку значения составным именем ключа
	ASSERT_TRUE(document.set({"server", "limits", "depth"}, static_cast <int64_t> (2)));
	// Выполняем проверку записи установленного значения
	ASSERT_EQ(document.text(), "[server]\nlimits.depth = 2\n");
}
/**
 * @brief Проверка обращения с пустым именем ключа
 *
 * @details Описание дозволяет имя ключа пустым, и указатель дерева обязан отличать
 * его от верхнего уровня текста настроек: иначе обход дерева по дочерним именам
 * возвращался бы к его началу и не кончался вовсе
 *
 */
TEST(CodecTomlDocument, EmptyKey) {
	// Разбираемый текст настроек
	const string text = "\"\" = 1\n[table]\n\"\" = 2\n";
	// Объект дерева настроек
	toml::document_t document(::logger());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(text)) << static_cast <uint32_t> (document.error());
	// Целое число значения пары с пустым именем ключа
	int64_t value = 0;
	// Выполняем проверку чтения значения пары с пустым именем ключа
	ASSERT_TRUE(document.value(value, {""}));
	// Выполняем проверку прочитанного значения
	ASSERT_EQ(value, 1);
	// Выполняем проверку чтения значения пары таблицы с пустым именем ключа
	ASSERT_TRUE(document.value(value, {"table", ""}));
	// Выполняем проверку прочитанного значения
	ASSERT_EQ(value, 2);
	// Получаем перечень дочерних имён верхнего уровня
	const vector <string_view> roots = document.keys();
	// Выполняем проверку количества дочерних имён верхнего уровня
	ASSERT_EQ(roots.size(), 2u);
	// Выполняем проверку пустого дочернего имени верхнего уровня
	ASSERT_TRUE(roots.at(0).empty());
	/**
	 * Получаем перечень дочерних имён пары с пустым именем ключа
	 *
	 * @note Верхний уровень выдал бы здесь собственные дочерние имена, и обход
	 *       дерева пошёл бы по кругу
	 */
	const vector <string_view> keys = document.keys({""});
	// Выполняем проверку отсутствия дочерних имён у пары с пустым именем ключа
	ASSERT_TRUE(keys.empty());
	// Выполняем проверку побайтового совпадения перезаписи с исходным текстом
	ASSERT_EQ(document.text(), text);
}
/**
 * @brief Проверка освобождения дерева настроек
 *
 */
TEST(CodecTomlDocument, Clearing) {
	// Объект дерева настроек
	toml::document_t document(::logger());
	// Выполняем проверку того, что дерево настроек пусто
	ASSERT_TRUE(document.empty());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(SAMPLE));
	// Выполняем проверку того, что дерево настроек не пусто
	ASSERT_FALSE(document.empty());
	// Выполняем освобождение дерева настроек
	document.clear();
	// Выполняем проверку того, что дерево настроек освобождено
	ASSERT_TRUE(document.empty());
	// Выполняем проверку того, что таблиц в дереве не осталось
	ASSERT_EQ(document.size(), 0u);
	// Выполняем проверку того, что собранный текст настроек пуст
	ASSERT_TRUE(document.text().empty());
}
/**
 * @brief Проверка отказа правки поверх занятого имени
 *
 * @details Объявить таблицу поверх пары либо завести пару поверх таблицы значило бы
 * собрать текст, который разбор отвергнет переопределением: отвергается это в месте
 * правки, а не при записи собранного дерева
 *
 */
TEST(CodecTomlDocument, Redefine) {
	// Объект дерева настроек
	toml::document_t document(::logger());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(SAMPLE));
	// Выполняем проверку отказа объявления таблицы поверх объявленной пары
	ASSERT_FALSE(document.create({"title"}));
	// Выполняем проверку кода ошибки правки дерева
	ASSERT_EQ(document.error(), toml::error_t::REDEFINE_TABLE);
	// Выполняем проверку отказа заведения пары поверх объявленной таблицы
	ASSERT_FALSE(document.set({"server"}, static_cast <int64_t> (1)));
	// Выполняем проверку кода ошибки правки дерева
	ASSERT_EQ(document.error(), toml::error_t::REDEFINE_TABLE);
	// Выполняем проверку отказа заведения пары поверх набора таблиц
	ASSERT_FALSE(document.set({"products"}, static_cast <int64_t> (1)));
	// Выполняем проверку кода ошибки правки дерева
	ASSERT_EQ(document.error(), toml::error_t::REDEFINE_TABLE);
	/**
	 * Выполняем проверку отказа заведения пары под набором таблиц без номера
	 *
	 * @note Набор таблиц адресуется порядковым номером, и всякое иное имя за ним
	 *       завело бы пару поверх набора
	 */
	ASSERT_FALSE(document.set({"products", "name"}, "гвоздь"));
	// Выполняем проверку кода ошибки правки дерева
	ASSERT_EQ(document.error(), toml::error_t::REDEFINE_TABLE);
	// Выполняем проверку отказа заведения пары под таблицей набора за его пределами
	ASSERT_FALSE(document.set({"products", "2", "name"}, "молоток"));
	// Выполняем проверку кода ошибки правки дерева
	ASSERT_EQ(document.error(), toml::error_t::REDEFINE_TABLE);
	// Выполняем проверку правки пары объявленной таблицы набора
	ASSERT_TRUE(document.set({"products", "1", "name"}, "клещи"));
	// Выполняем проверку чтения установленного значения
	ASSERT_EQ(document.text({"products", "1", "name"}), "клещи");
	// Выполняем проверку побайтового совпадения перезаписи с ожидаемым текстом
	ASSERT_NE(document.text().find("name = \"клещи\"\n"), string::npos);
	// Объект дерева настроек для разбора собранного текста
	toml::document_t reread(::logger());
	// Выполняем проверку разбора собранного текста настроек
	ASSERT_TRUE(reread.parse(document.text())) << static_cast <uint32_t> (reread.error());
}
/**
 * @brief Проверка правил правки вровень с правилами разбора
 *
 * @details Правка обязана отвергать всё, что разбор отверг бы при обратном чтении:
 * пару поверх таблицы, заголовок поверх таблицы, заведённой составным именем ключа,
 * и всякое заведение под парой - под простым значением, перечнем и встроенной
 * таблицей
 *
 */
TEST(CodecTomlDocument, EditRules) {
	/**
	 * Выполняем перебор правок, правилами описания отвергаемых
	 */
	for(const auto & item : {
		make_pair(string("a.b = 1\n"), false),
		make_pair(string("[a.b]\n"), false),
		make_pair(string("[[a]]\n"), true)
	}){
		// Объект дерева настроек
		toml::document_t document(::logger());
		// Выполняем проверку разбора исходного текста настроек
		ASSERT_TRUE(document.parse(item.first));
		// Выполняем проверку отказа заведения пары поверх таблицы
		ASSERT_FALSE(document.set(vector <string_view> {"a"}, static_cast <int64_t> (1)));
		/**
		 * Если таблица заведена составным именем ключа
		 */
		if(!item.second)
			// Выполняем проверку заведения заголовка поверх таблицы
			ASSERT_EQ(document.create(vector <string_view> {"a"}), item.first.compare("[a.b]\n") == 0);
		// Выполняем проверку отказа заведения заголовка поверх набора таблиц
		else ASSERT_FALSE(document.create(vector <string_view> {"a"}));
	}
	/**
	 * Выполняем перебор исходных текстов с занятым объемлющим именем
	 */
	for(const string & source : {
		string("a = 1\n"),
		string("a = [1]\n"),
		string("a = {b = 1}\n")
	}){
		// Объект дерева настроек
		toml::document_t document(::logger());
		// Выполняем проверку разбора исходного текста настроек
		ASSERT_TRUE(document.parse(source));
		// Выполняем проверку отказа заведения пары под занятым именем
		ASSERT_FALSE(document.set(vector <string_view> {"a", "c"}, static_cast <int64_t> (1)));
		// Выполняем проверку отказа заведения таблицы под занятым именем
		ASSERT_FALSE(document.create(vector <string_view> {"a", "c"}));
	}
}
/**
 * @brief Проверка обратной читаемости всякой принятой правки
 *
 * @details Правка, дерево не отвергшее, обязана давать текст, который разбор
 * принимает: расхождение правил правки с правилами разбора иначе обнаруживалось бы
 * лишь у потребителя
 *
 */
TEST(CodecTomlDocument, EditReadable) {
	/**
	 * Выполняем перебор исходных текстов настроек
	 */
	for(const string & source : {
		string("x = 1\n"),
		string("[t]\nx = 1\n"),
		string("[[a]]\nx = 1\n"),
		string("a.b = 1\n"),
		string("[a.b]\nx = 1\n")
	}){
		/**
		 * Выполняем перебор вносимых правок
		 */
		for(uint32_t kind = 0; kind < 4; kind++){
			// Объект дерева настроек
			toml::document_t document(::logger());
			// Выполняем проверку разбора исходного текста настроек
			ASSERT_TRUE(document.parse(source));
			// Признак того, что правка принята
			bool edited = false;
			/**
			 * Выполняем выбор разновидности вносимой правки
			 */
			switch(kind){
				// Если правкой является заведение таблицы
				case 0: edited = document.create(vector <string_view> {"a"}); break;
				// Если правкой является заведение пары
				case 1: edited = document.set(vector <string_view> {"a"}, static_cast <int64_t> (1)); break;
				// Если правкой является заведение составного имени ключа
				case 2: edited = document.set(vector <string_view> {"a", "b"}, static_cast <int64_t> (1)); break;
				// Если правкой является заведение вложенной таблицы
				case 3: edited = document.create(vector <string_view> {"a", "b"}); break;
			}
			/**
			 * Если правка дерева отвергнута
			 */
			if(!edited)
				// Выполняем переход к следующей правке
				continue;
			// Выполняем перезапись правленого дерева настроек
			const string text = document.text();
			// Выполняем проверку того, что перезапись собрана
			ASSERT_FALSE(text.empty());
			// Объект дерева настроек для обратного чтения
			toml::document_t after(::logger());
			// Выполняем проверку обратного чтения перезаписи
			ASSERT_TRUE(after.parse(text)) << "правка " << kind << " над «" << source << "» дала «" << text << "»";
		}
	}
}
/**
 * @brief Проверка сохранения примечаний внутри перечня значений
 *
 * @details Примечание внутри перечня строкою текста настроек не является и держится
 * узлом значения, а не записью. Перезапись, его теряющая, обедняла бы файл настроек
 * вопреки договору дерева
 *
 */
TEST(CodecTomlDocument, ArrayRemarks) {
	/**
	 * Выполняем перебор исходных текстов настроек
	 */
	for(const string & source : {
		string("a = [\n\t1, # первое\n\t2\n]\n"),
		string("a = [\n\t# перед первым\n\t1,\n\t2\n]\n"),
		string("a = [ # за скобкой\n\t1\n]\n"),
		string("a = [\n\t1\n\t# за последним\n]\n"),
		string("a = [\n\t# одни примечания\n]\n"),
		string("a = [\n\t[\n\t\t1, # внутри\n\t\t2\n\t], # снаружи\n\t3\n]\n"),
		string("a = [\n\t1, #\n\t2\n]\n"),
		string("# сверху\na = [\n\t1 # к первому\n] # к паре\n")
	}){
		// Объект дерева настроек
		toml::document_t document(::logger());
		// Выполняем проверку разбора исходного текста настроек
		ASSERT_TRUE(document.parse(source)) << "«" << source << "»";
		// Выполняем проверку того, что перезапись повторяет исходный текст
		ASSERT_EQ(document.text(), source);
	}
}
/**
 * @brief Проверка ограниченности памяти дерева при долгой правке
 *
 * @details Правка замещает узел значения новым, а прежний оставляет мусором: изъять
 * его на месте нельзя, ибо прежнее значение вправе нести вложенные узлы. Дерево
 * уплотняется само по накоплении мусора вровень с живым, и без уплотнения того
 * четыреста тысяч правок двух ключей отнимали сто тринадцать мегабайт при неизменном
 * составе дерева
 *
 */
TEST(CodecTomlDocument, EditFootprint) {
	// Исходный текст настроек с примечаниями, перечнями и набором таблиц
	const string source(
		"# сверху\n"
		"title = \"пример\"\n"
		"\n"
		"[server]\n"
		"host = '127.0.0.1' # к паре\n"
		"ports = [\n"
		"\t8080, # первый\n"
		"\t8443\n"
		"]\n"
		"\n"
		"[[products]]\n"
		"name = \"молоток\"\n"
	);
	// Объект дерева настроек
	toml::document_t document(::logger());
	// Выполняем проверку разбора исходного текста настроек
	ASSERT_TRUE(document.parse(source));
	// Запоминаем перезапись дерева до правок
	const string before = document.text();
	// Запоминаем объём памяти дерева до правок
	const size_t footprint = document.footprint();
	// Выполняем проверку того, что объём памяти дерева получен
	ASSERT_GT(footprint, static_cast <size_t> (0));
	/**
	 * Выполняем перебор вносимых правок дерева настроек
	 */
	for(size_t i = 0; i < 20000; i++){
		// Выполняем правку строкового значения пары
		ASSERT_TRUE(document.set(vector <string_view> {"server", "host"}, string_view("10.0.0.1"), toml::string_t::LITERAL));
		// Выполняем правку целого значения пары
		ASSERT_TRUE(document.set(vector <string_view> {"title"}, static_cast <int64_t> (i)));
		// Выполняем возврат строкового значения пары
		ASSERT_TRUE(document.set(vector <string_view> {"server", "host"}, string_view("127.0.0.1"), toml::string_t::LITERAL));
		// Выполняем возврат строкового значения пары
		ASSERT_TRUE(document.set(vector <string_view> {"title"}, string_view("пример"), toml::string_t::BASIC));
	}
	/**
	 * Выполняем проверку того, что память дерева ограничена
	 *
	 * @note Запас взят восьмикратным против исходного: уплотнение приходит по
	 *       накоплении мусора вровень с живым, и объём дерева гуляет между уплотнениями,
	 *       а рост неограниченный дал бы здесь не разы, а тысячи
	 */
	ASSERT_LT(document.footprint(), (footprint * 8))
	 << "до правок " << footprint << " октетов, после восьмидесяти тысяч правок " << document.footprint();
	// Выполняем проверку того, что перезапись дерева правками не испорчена
	ASSERT_EQ(document.text(), before);
}
/**
 * @brief Проверка заведения пары после долгой правки дерева
 *
 * @details Правка получает указатель на узел значения, а уплотнение дерева перечень
 * узлов переносит: указатель, взятый до уплотнения, указывал бы на чужой узел либо за
 * край перечня. Заведение новой пары дописывает имя к хранилищу знаков и вправе
 * повлечь уплотнение прямо посреди себя
 *
 */
TEST(CodecTomlDocument, EditAfterCompaction) {
	// Объект дерева настроек
	toml::document_t document(::logger());
	// Выполняем проверку разбора исходного текста настроек
	ASSERT_TRUE(document.parse("[server]\nhost = \"127.0.0.1\"\n"));
	/**
	 * Выполняем перебор вносимых правок дерева настроек
	 */
	for(size_t i = 0; i < 2000; i++){
		// Собираемое имя заводимой пары
		const string name = ("ключ-подлиннее-чтобы-хранилище-росло-" + to_string(i));
		// Выполняем правку значения объявленной пары
		ASSERT_TRUE(document.set(vector <string_view> {"server", "host"}, string_view("10.0.0.1"), toml::string_t::BASIC));
		// Выполняем заведение новой пары дерева настроек
		ASSERT_TRUE(document.set(vector <string_view> {"server", name}, static_cast <int64_t> (i)));
		// Читаемое значение заведённой пары
		int64_t value = 0;
		// Выполняем проверку чтения значения заведённой пары
		ASSERT_TRUE(document.value(value, {"server", name}));
		// Выполняем проверку того, что значение легло в свой узел
		ASSERT_EQ(value, static_cast <int64_t> (i));
	}
	// Объект дерева настроек для обратного чтения
	toml::document_t after(::logger());
	// Выполняем проверку обратного чтения перезаписи правленого дерева
	ASSERT_TRUE(after.parse(document.text()));
	// Читаемое значение последней заведённой пары
	int64_t value = 0;
	// Выполняем проверку чтения значения последней заведённой пары
	ASSERT_TRUE(after.value(value, {"server", "ключ-подлиннее-чтобы-хранилище-росло-1999"}));
	// Выполняем проверку значения последней заведённой пары
	ASSERT_EQ(value, static_cast <int64_t> (1999));
}
/**
 * @brief Проверка счёта мусора при снятии пары без примечания
 *
 * @details Снятие пары, примечания в конце строки не несущей, обращает в мусор одну
 * запись, а не две: счёт лишнего гнал бы уплотнение дерева чаще нужного
 *
 */
TEST(CodecTomlDocument, EraseGarbageCount) {
	// Объект дерева настроек
	toml::document_t document(::logger());
	// Выполняем проверку разбора исходного текста настроек
	ASSERT_TRUE(document.parse("[t]\na = 1\nb = 2\n"));
	// Выполняем проверку снятия пары дерева настроек
	ASSERT_TRUE(document.erase(vector <string_view> {"t", "a"}));
	// Выполняем проверку того, что снятая пара из выдачи ушла
	ASSERT_EQ(document.text(), string("[t]\nb = 2\n"));
	// Выполняем проверку того, что оставшаяся пара читается
	int64_t value = 0;
	// Выполняем проверку чтения значения оставшейся пары
	ASSERT_TRUE(document.value(value, {"t", "b"}));
	// Выполняем проверку значения оставшейся пары
	ASSERT_EQ(value, static_cast <int64_t> (2));
	// Выполняем проверку того, что снятая пара более не находится
	ASSERT_FALSE(document.value(value, {"t", "a"}));
}
/**
 * @brief Проверка согласия правки отметки времени с календарём разбора
 *
 * @details Правка, принявшая дату несуществующую, собрала бы текст, который свой же
 * разбор отвергает, и узнал бы о том потребитель много позже правки. Проверка ведётся
 * общим для разбора, записи и правки календарём
 *
 */
TEST(CodecTomlDocument, StampCalendar) {
	/**
	 * Выполняем перебор проверяемых отметок времени
	 */
	for(const auto & item : {
		make_tuple(uint16_t(2026), uint8_t(2), uint8_t(31), false),
		make_tuple(uint16_t(2025), uint8_t(2), uint8_t(29), false),
		make_tuple(uint16_t(2026), uint8_t(13), uint8_t(1), false),
		make_tuple(uint16_t(2026), uint8_t(4), uint8_t(31), false),
		make_tuple(uint16_t(2026), uint8_t(1), uint8_t(0), false),
		make_tuple(uint16_t(2024), uint8_t(2), uint8_t(29), true),
		make_tuple(uint16_t(2000), uint8_t(2), uint8_t(29), true),
		make_tuple(uint16_t(1900), uint8_t(2), uint8_t(29), false),
		make_tuple(uint16_t(2026), uint8_t(12), uint8_t(31), true)
	}){
		// Собираемая отметка времени
		toml::stamp_t stamp;
		// Запоминаем год собираемой отметки времени
		stamp.date.year = get <0> (item);
		// Запоминаем месяц собираемой отметки времени
		stamp.date.month = get <1> (item);
		// Запоминаем день собираемой отметки времени
		stamp.date.day = get <2> (item);
		// Объект дерева настроек
		toml::document_t document(::logger());
		// Выполняем проверку разбора исходного текста настроек
		ASSERT_TRUE(document.parse("a = 2026-01-01\n"));
		// Выполняем проверку итога правки отметки времени
		ASSERT_EQ(document.set(vector <string_view> {"a"}, stamp, toml::type_t::LOCAL_DATE), get <3> (item))
		 << get <0> (item) << "-" << static_cast <uint32_t> (get <1> (item)) << "-" << static_cast <uint32_t> (get <2> (item));
		// Объект записи текста настроек
		toml::writer_t writer(::logger());
		// Выполняем запись имени ключа пары
		ASSERT_TRUE(writer.key("a"));
		// Выполняем проверку итога записи отметки времени
		ASSERT_EQ(writer.stamp(stamp, toml::type_t::LOCAL_DATE), get <3> (item));
		/**
		 * Если отметка времени записана
		 */
		if(get <3> (item)){
			// Объект дерева настроек для обратного чтения
			toml::document_t after(::logger());
			// Выполняем проверку того, что записанное разбор принимает
			ASSERT_TRUE(after.parse(document.text())) << document.text();
		}
	}
}
/**
 * @brief Проверка чтения значений внутри составных значений
 *
 * @details Остаток составного имени ведёт внутрь значения пары: порядковым номером в
 * перечень, именем ключа во встроенную таблицу. Без этого перечень перечней и
 * встроенная таблица во встроенной таблице читались бы одним лишь обходом событий
 *
 */
TEST(CodecTomlDocument, NestedReading) {
	// Объект дерева настроек
	toml::document_t document(::logger());
	// Выполняем проверку разбора исходного текста настроек
	ASSERT_TRUE(document.parse(
		"a = [[1, 2], [3, 4]]\n"
		"b = [10, 20]\n"
		"c = {d = {e = 5}}\n"
		"f = [{g = 7}, {g = 8}]\n"
		"h = {\"i.j\" = 9, k.l = 11}\n"
		"[[products]]\n"
		"name = \"молоток\"\n"
		"[[products]]\n"
		"name = \"гвоздь\"\n"
	));
	// Читаемое целое значение
	int64_t value = 0;
	// Выполняем проверку чтения значения перечня
	ASSERT_TRUE(document.value(value, {"b", "0"}));
	// Выполняем проверку значения перечня
	ASSERT_EQ(value, static_cast <int64_t> (10));
	// Выполняем проверку чтения значения вложенного перечня
	ASSERT_TRUE(document.value(value, {"a", "1", "0"}));
	// Выполняем проверку значения вложенного перечня
	ASSERT_EQ(value, static_cast <int64_t> (3));
	// Выполняем проверку количества значений вложенного перечня
	ASSERT_EQ(document.length({"a", "0"}), static_cast <size_t> (2));
	// Выполняем проверку чтения значения вложенной встроенной таблицы
	ASSERT_TRUE(document.value(value, {"c", "d", "e"}));
	// Выполняем проверку значения вложенной встроенной таблицы
	ASSERT_EQ(value, static_cast <int64_t> (5));
	// Выполняем проверку чтения значения встроенной таблицы перечня
	ASSERT_TRUE(document.value(value, {"f", "1", "g"}));
	// Выполняем проверку значения встроенной таблицы перечня
	ASSERT_EQ(value, static_cast <int64_t> (8));
	// Выполняем проверку чтения значения по имени с точкой внутри
	ASSERT_TRUE(document.value(value, {"h", "i.j"}));
	// Выполняем проверку значения по имени с точкой внутри
	ASSERT_EQ(value, static_cast <int64_t> (9));
	// Выполняем проверку чтения значения по составному имени встроенной таблицы
	ASSERT_TRUE(document.value(value, {"h", "k", "l"}));
	// Выполняем проверку значения по составному имени встроенной таблицы
	ASSERT_EQ(value, static_cast <int64_t> (11));
	// Выполняем проверку того, что таблица набора номером адресуется по-прежнему
	ASSERT_EQ(document.text({"products", "1", "name"}), string_view("гвоздь"));
	// Выполняем проверку того, что номер за пределами перечня не читается
	ASSERT_FALSE(document.value(value, {"b", "2"}));
	// Выполняем проверку того, что номер с ведущим нулём не читается
	ASSERT_FALSE(document.value(value, {"b", "01"}));
	// Выполняем проверку того, что спуск в простое значение не выполняется
	ASSERT_FALSE(document.value(value, {"b", "0", "0"}));
	// Выполняем проверку того, что имя, встроенной таблице не принадлежащее, не читается
	ASSERT_FALSE(document.value(value, {"c", "d", "x"}));
}
/**
 * @brief Проверка отказа правки по порядковому номеру с ведущим нулём
 *
 * @details Таблицы набора адресуются порядковым номером, и указатель дерева ведёт
 * их записью кратчайшей. Имя «01» ни с одной таблицей набора не совпадает, и пара по
 * нему вставала бы верхним уровнем поверх набора: собранный текст свой же разбор
 * отверг бы дополнением набора таблиц именем, набором не являющимся
 *
 */
TEST(CodecTomlDocument, LeadingZeroOrdinal) {
	// Объект дерева настроек
	toml::document_t document(::logger());
	// Выполняем разбор исходного текста настроек
	ASSERT_TRUE(document.parse("[[products]]\nname = \"первый\"\n[[products]]\nname = \"второй\"\n"));
	// Выполняем проверку правки по порядковому номеру без ведущего нуля
	ASSERT_TRUE(document.set(vector <string_view> {"products", "1", "name"}, string_view("иной")));
	// Выполняем проверку отказа правки по порядковому номеру с ведущим нулём
	ASSERT_FALSE(document.set(vector <string_view> {"products", "01", "name"}, string_view("третий")));
	// Выполняем проверку кода ошибки правки дерева настроек
	ASSERT_EQ(document.error(), toml::error_t::REDEFINE_TABLE);
	// Собранный правкой текст настроек
	const string text = document.text();
	// Объект дерева настроек, собранного обратным разбором
	toml::document_t reread(::logger());
	/**
	 * Выполняем проверку того, что собранный текст читается собственным разбором
	 *
	 * @note Без проверки правка выдавала бы текст с парой «products.01.name»,
	 *       который разбор отвергает
	 */
	ASSERT_TRUE(reread.parse(text)) << toml::message(reread.error());
	// Получаемое значение правленой пары
	toml::content_t value;
	// Выполняем проверку получения значения правленой пары
	ASSERT_TRUE(reread.get({"products", "1", "name"}, value));
	// Выполняем проверку правленого значения
	ASSERT_EQ(value.text, "иной");
}
/**
 * @brief Проверка сброса состояния обхода освобождением дерева
 *
 * @details Состояние обхода несёт имена, уже объявленные им, и заведение имени заново
 * ими отсекается. Без сброса дерево, собранное правками после освобождения, теряло бы
 * дочерние имена, совпавшие с именами прежнего дерева
 *
 */
TEST(CodecTomlDocument, ClearedTraversal) {
	// Объект дерева настроек
	toml::document_t document(::logger());
	// Выполняем разбор исходного текста настроек
	ASSERT_TRUE(document.parse("[a]\nx = 1\n"));
	// Выполняем проверку дочерних имён верхнего уровня
	ASSERT_EQ(document.keys(vector <string_view> {}).size(), 1u);
	// Выполняем освобождение дерева настроек
	document.clear();
	// Выполняем проверку объявления таблицы с тем же именем заново
	ASSERT_TRUE(document.create(vector <string_view> {"a"}));
	/**
	 * Выполняем проверку дочерних имён верхнего уровня заново собранного дерева
	 *
	 * @note Без сброса состояния обхода имя «a» считалось бы уже объявленным, и
	 *       перечень дочерних имён остался бы пустым
	 */
	ASSERT_EQ(document.keys(vector <string_view> {}).size(), 1u);
	// Выполняем проверку правки заново собранного дерева
	ASSERT_TRUE(document.set(vector <string_view> {"a", "x"}, static_cast <int64_t> (2)));
	// Выполняем проверку собранного текста настроек
	ASSERT_EQ(document.text(), "[a]\nx = 2\n");
}

/**
 * @brief Проверка сборки перечня значений правкой дерева
 *
 */
TEST(CodecTomlDocument, ArrayBuilding) {
	// Собираемое дерево настроек
	toml::document_t document(::logger());
	// Выполняем разбор пустого текста настроек
	ASSERT_TRUE(document.parse(""));
	// Собираемое содержимое добавляемого значения
	toml::content_t content;
	/**
	 * Выполняем проверку отказа долива к перечню, не объявленному вовсе
	 *
	 * @note Отказ намеренный: долив наполняет перечень, а не заводит его, и завести
	 *       его молча значило бы стереть значение, у места стоявшее
	 */
	content.type = toml::type_t::INTEGER;
	// Запоминаем добавляемое целое число
	content.integer = 1;
	// Выполняем проверку отказа долива к перечню, не объявленному вовсе
	ASSERT_FALSE(document.push({"numbers"}, content));
	// Выполняем объявление перечня значений
	ASSERT_TRUE(document.arrange({"numbers"}));
	// Выполняем проверку того, что объявленный перечень пуст
	ASSERT_EQ(document.length({"numbers"}), 0);
	// Выполняем долив первого значения перечня
	ASSERT_TRUE(document.push({"numbers"}, content));
	// Запоминаем добавляемое второе целое число
	content.integer = 2;
	// Выполняем долив второго значения перечня
	ASSERT_TRUE(document.push({"numbers"}, content));
	// Выполняем проверку количества значений перечня
	ASSERT_EQ(document.length({"numbers"}), 2);
	/**
	 * Выполняем проверку отказа добавления пары к перечню
	 *
	 * @note Пара кладётся во встроенную таблицу, и перечень её принять не вправе:
	 *       имени у значения перечня нет вовсе
	 */
	ASSERT_FALSE(document.put({"numbers"}, {"name"}, content));
	// Выполняем объявление вложенной встроенной таблицы доливом пустого значения
	content.type = toml::type_t::TABLE;
	// Выполняем долив встроенной таблицы к перечню
	ASSERT_TRUE(document.push({"numbers"}, content));
	// Запоминаем добавляемое строковое значение пары встроенной таблицы
	content.type = toml::type_t::STRING;
	// Запоминаем содержимое строкового значения
	content.text = "значение";
	// Выполняем добавление пары к встроенной таблице перечня
	ASSERT_TRUE(document.put({"numbers", "2"}, {"key"}, content));
	// Выполняем проверку значения пары встроенной таблицы перечня
	ASSERT_EQ(document.text({"numbers", "2", "key"}), "значение");
	// Выполняем проверку собранного текста настроек
	ASSERT_EQ(document.text(), "numbers = [1, 2, { key = \"значение\" }]\n");
	/**
	 * Выполняем повторное объявление перечня у того же места
	 *
	 * @note Объявление кладёт перечень заново: дописывать к прежнему значило бы
	 *       лишить потребителя способа перечень заменить
	 */
	ASSERT_TRUE(document.arrange({"numbers"}));
	// Выполняем проверку того, что перечень объявлен пустым заново
	ASSERT_EQ(document.length({"numbers"}), 0);
}

/**
 * @brief Проверка долива к перечню значением, взятым у того же дерева
 *
 * @note Долив кладёт значение в хранилище знаков того же дерева, и вид, в него
 *       указывающий, наращивание хранилища пережить обязан
 *
 */
TEST(CodecTomlDocument, PushFromOwnView) {
	// Объект дерева настроек
	toml::document_t document(::logger());
	// Значение, заведомо превышающее короткий запас строки
	const string big(4096, 'z');
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse("[server]\nhost = \"" + big + "\"\n"));
	/**
	 * Выполняем набивку дерева, заведомо перераспределяющую хранилище знаков
	 */
	for(size_t i = 0; i < 64; i++)
		// Выполняем установку очередного значения
		ASSERT_TRUE(document.set({"server", "поле" + to_string(i)}, string(256, 'a')));
	// Выполняем объявление перечня значений
	ASSERT_TRUE(document.arrange({"server", "перечень"}));
	// Собираемое содержимое доливаемого значения
	toml::content_t content;
	// Запоминаем тип доливаемого значения
	content.type = toml::type_t::STRING;
	// Выполняем снятие значения видом в хранилище знаков
	content.text = document.text({"server", "host"});
	// Выполняем проверку длины снятого значения
	ASSERT_EQ(content.text.length(), big.length());
	// Выполняем долив снятого значения тому же дереву
	ASSERT_TRUE(document.push({"server", "перечень"}, content));
	// Извлекаемое содержимое значения перечня
	toml::content_t result;
	// Выполняем проверку успешности получения доливаемого значения
	ASSERT_TRUE(document.item({"server", "перечень"}, 0, result));
	// Выполняем проверку того, что значение перенесено целым
	ASSERT_EQ(result.text, big);
	// Выполняем проверку сохранности исходного значения
	ASSERT_EQ(document.text({"server", "host"}), big);
	// Выполняем объявление встроенной таблицы
	ASSERT_TRUE(document.arrange({"server", "таблица"}, true));
	// Выполняем снятие имени видом в хранилище знаков
	const string_view name = document.text({"server", "host"});
	// Запоминаем содержимое добавляемой пары видом в то же хранилище
	content.text = name;
	// Выполняем добавление пары встроенной таблицы именем из того же хранилища
	ASSERT_TRUE(document.put({"server", "таблица"}, {name.substr(0, 8)}, content));
	// Выполняем проверку значения добавленной пары встроенной таблицы
	ASSERT_EQ(document.text({"server", "таблица", big.substr(0, 8)}), big);
}

/**
 * @brief Проверка отличения отсутствия имени от ошибочного построения его
 *
 * @details Отказы эти разного рода: имя может быть составлено верно, а пары либо
 *          таблицы с ним в дереве не быть. Прежде оба отвечали кодами ошибочного
 *          построения имени, и потребитель искал изъян в имени, какого там нет
 *
 */
TEST(CodecTomlDocument, UnknownNameDiffersFromMalformed) {
	// Дерево настроек
	toml::document_t document(::logger());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse("[server]\nhost = \"localhost\"\n"));
	// Выполняем проверку отказа снятия необъявленной пары
	ASSERT_FALSE(document.erase({"server", "missing"}));
	// Выполняем проверку кода ошибки правки дерева
	ASSERT_EQ(document.error(), toml::error_t::UNKNOWN_KEY);
	// Выполняем проверку отказа удаления необъявленной таблицы
	ASSERT_FALSE(document.remove({"missing"}));
	// Выполняем проверку кода ошибки правки дерева
	ASSERT_EQ(document.error(), toml::error_t::UNKNOWN_TABLE);
	/**
	 * Выполняем проверку сохранения кода ошибочного построения имени
	 *
	 * @note Управляющий знак в имени - изъян самого имени, и код его прежним остаётся
	 */
	ASSERT_FALSE(document.set({string_view("имя\nключа")}, static_cast <int64_t> (1)));
	// Выполняем проверку кода ошибки правки дерева
	ASSERT_EQ(document.error(), toml::error_t::INVALID_KEY);
}
/**
 * @brief Проверка извлечения признака истины из дерева настроек
 *
 * @details Логический тип язык причисляет к целым, и сличение с ним ведётся ПРЕЖДЕ
 *          целых чисел намеренно: без того истина выдавалась бы единицей. Обратное
 *          столь же обязательно - целое единицей истиной выдаваться не должно
 *
 * @note Заход этот пересечение трёх прогонов числило непройденным ничем: извлечение
 *       проверялось числами и записями, а признак истины ни одним прогоном не брался
 *
 */
TEST(CodecTomlDocument, BooleanExtraction) {
	// Дерево настроек, значения разбирающее
	toml::document_t document(::logger());
	// Выполняем проверку успешности разбора текста настроек
	ASSERT_TRUE(document.parse("yes = true\nno = false\nnum = 1\n"));
	// Извлекаемый признак истины
	bool taken = false;
	// Выполняем проверку успешности извлечения истины
	ASSERT_TRUE(document.value(taken, {"yes"}));
	// Выполняем проверку того, что извлечена именно истина
	ASSERT_TRUE(taken);
	// Извлекаемый признак лжи
	bool second = true;
	// Выполняем проверку успешности извлечения лжи
	ASSERT_TRUE(document.value(second, {"no"}));
	// Выполняем проверку того, что извлечена именно ложь
	ASSERT_FALSE(second);
	// Извлекаемый признак истины из целого числа
	bool third = false;
	/**
	 * Выполняем проверку того, что целое признаком истины не выдаётся
	 *
	 * @note Половина эта обязательна: извлечение, всякое значение истиной выдающее,
	 *       прошло бы обе половины выше
	 */
	ASSERT_FALSE(document.value(third, {"num"}));
	// Извлекаемое целое число из признака истины
	int64_t number = 0;
	// Выполняем проверку того, что истина целым числом не выдаётся
	ASSERT_FALSE(document.value(number, {"yes"}));
}
/**
 * @brief Проверка отказа правки дерева отметкой времени построения ошибочного
 *
 * @details Правка отметкой обязана отвергать и вид, отметкой времени не являющийся, и
 *          построение, календарю не отвечающее, оставляя дерево нетронутым: приняв
 *          такое, дерево собрало бы текст, самим же кодеком отвергаемый
 *
 * @note Заход вида пересечение трёх прогонов числило непройденным ничем: отметки
 *       приходили лишь разбором текста, где вид назначает сам разбор
 *
 */
TEST(CodecTomlDocument, StampEditRefusesMalformed) {
	// Исходный текст настроек с отметкой времени
	const string text("mark = 1979-05-27T07:32:00Z\n");
	/**
	 * Выполняем проверку отказа правки видом, отметкой времени не являющимся
	 */
	{
		// Дерево настроек, отметку разбирающее
		toml::document_t document(::logger());
		// Выполняем проверку успешности разбора текста настроек
		ASSERT_TRUE(document.parse(text));
		// Отметка времени, правкою назначаемая
		toml::stamp_t stamp;
		// Устанавливаем год отметки
		stamp.date.year = 2026;
		// Устанавливаем месяц отметки
		stamp.date.month = 8;
		// Устанавливаем день отметки
		stamp.date.day = 27;
		// Выполняем проверку отказа правки видом не-отметкой
		ASSERT_FALSE(document.set({"mark"}, stamp, toml::type_t::STRING));
		// Выполняем проверку того, что отказ назван причиною своею
		ASSERT_EQ(document.error(), toml::error_t::INVALID_DATETIME);
		// Выполняем проверку того, что дерево отказом не тронуто
		ASSERT_EQ(document.text(), text);
	}
	/**
	 * Выполняем проверку отказа правки датой, календарю не отвечающей
	 */
	{
		// Дерево настроек, отметку разбирающее
		toml::document_t document(::logger());
		// Выполняем проверку успешности разбора текста настроек
		ASSERT_TRUE(document.parse(text));
		// Отметка времени, правкою назначаемая
		toml::stamp_t stamp;
		// Устанавливаем год отметки
		stamp.date.year = 2026;
		// Устанавливаем месяц отметки
		stamp.date.month = 2;
		// Устанавливаем день, февралю не отвечающий
		stamp.date.day = 31;
		// Выполняем проверку отказа правки негодною датой
		ASSERT_FALSE(document.set({"mark"}, stamp, toml::type_t::LOCAL_DATE));
		// Выполняем проверку того, что отказ назван причиною своею
		ASSERT_EQ(document.error(), toml::error_t::INVALID_DATETIME);
		// Выполняем проверку того, что дерево отказом не тронуто
		ASSERT_EQ(document.text(), text);
	}
	/**
	 * Выполняем проверку того, что правка отметкой годной проходит
	 *
	 * @note Половина эта обязательна: правка, отвергающая всякую отметку, прошла бы обе
	 *       половины выше, не переписав ничего вовсе
	 */
	{
		// Дерево настроек, отметку разбирающее
		toml::document_t document(::logger());
		// Выполняем проверку успешности разбора текста настроек
		ASSERT_TRUE(document.parse(text));
		// Отметка времени, правкою назначаемая
		toml::stamp_t stamp;
		// Устанавливаем год отметки
		stamp.date.year = 2026;
		// Устанавливаем месяц отметки
		stamp.date.month = 8;
		// Устанавливаем день отметки
		stamp.date.day = 27;
		// Выполняем проверку успешности правки отметкой годной
		ASSERT_TRUE(document.set({"mark"}, stamp, toml::type_t::LOCAL_DATE)) << toml::message(document.error());
		// Выполняем проверку записи правленого дерева
		ASSERT_EQ(document.text(), "mark = 2026-08-27\n");
	}
}
/**
 * @brief Проверка усечения дробного значения пределами вида при извлечении
 *
 * @details Дробное значение, в затребованный вид целого не вмещающееся, извлечение
 *          усекает пределом этого вида, а нечисло обращает нулём. Заходы эти
 *          пересечение прогонов числило непройденными, а без них потребитель, взявший
 *          вид поуже, получал бы число, переполнением искажённое: приведение языка
 *          дробного к целому за пределами вида поведения не определяет вовсе
 *
 * @note Виды берутся узкими нарочно: у восьмибайтового целого предел столь велик, что
 *       заход этот дробным значением не достаётся
 *
 */
TEST(CodecTomlDocument, NumericClampOnExtraction) {
	// Объект дерева настроек
	toml::document_t document(::logger());
	// Выполняем разбор текста с дробными значениями за пределами узкого целого
	ASSERT_TRUE(document.parse("big = 1e300\nsmall = -1e300\nnone = nan\n"));
	// Извлекаемое число, верхним пределом усекаемое
	int32_t большое = 0;
	// Выполняем извлечение числа, вид переполняющего
	ASSERT_TRUE(document.value(большое, {"big"}));
	// Выполняем проверку усечения верхним пределом вида
	ASSERT_EQ(большое, numeric_limits <int32_t>::max());
	// Извлекаемое число, нижним пределом усекаемое
	int32_t малое = 0;
	// Выполняем извлечение числа, вид переполняющего снизу
	ASSERT_TRUE(document.value(малое, {"small"}));
	// Выполняем проверку усечения нижним пределом вида
	ASSERT_EQ(малое, numeric_limits <int32_t>::lowest());
	// Извлекаемое число, нечислом заданное
	int32_t нечисло = 12345;
	// Выполняем извлечение нечисла
	ASSERT_TRUE(document.value(нечисло, {"none"}));
	// Выполняем проверку того, что нечисло обращается нулём
	ASSERT_EQ(нечисло, 0);
}
/**
 * @brief Проверка обращений по пути ненайденному либо к значению вида чужого
 *
 * @details Снятие записи и признака многострочности спрашивают не только наличие пары,
 *          но и вид её значения: запись у числа не берётся, признак многострочности у
 *          записи - тоже, и выдаются они пустой записью да отрицанием. Правка же по
 *          пути, дереву неведомому, отвергается вовсе. Строки эти пересечение трёх
 *          прогонов числило слепыми - обращения велись по путям наличным и видам
 *          подходящим
 *
 */
TEST(CodecTomlDocument, AccessRefusesUnknownPathAndForeignType) {
	// Дерево настроек
	toml::document_t document(::logger());
	// Выполняем проверку успешности разбора текста настроек
	ASSERT_TRUE(document.parse("record = \"значение\"\nnumber = 7\nlist = [1, 2]\n"));
	// Выполняем проверку снятия записи по пути наличному
	ASSERT_EQ(document.text({"record"}), "значение");
	// Выполняем проверку того, что запись у значения числового не берётся
	ASSERT_TRUE(document.text({"number"}).empty());
	// Выполняем проверку того, что запись по пути ненайденному не берётся
	ASSERT_TRUE(document.text({"нет"}).empty());
	// Выполняем проверку признака многострочности у перечня наличного
	ASSERT_FALSE(document.multiline({"list"}));
	// Выполняем проверку того, что признак многострочности у записи отрицается
	ASSERT_FALSE(document.multiline({"record"}));
	// Выполняем проверку того, что признак многострочности по пути ненайденному отрицается
	ASSERT_FALSE(document.multiline({"нет"}));
	// Выполняем проверку правки значения по пути наличному
	ASSERT_TRUE(document.set({"number"}, true));
	/**
	 * Выполняем проверку заведения пары правкой по пути ненайденному
	 *
	 * @note Правка путь недостающий ЗАВОДИТ, а не отвергает: это договор её, и
	 *       ожидание отказа тут было бы ошибкою читающего - замерено
	 */
	ASSERT_TRUE(document.set({"нет"}, true));
	// Выполняем проверку того, что заведённая пара стоит в тексте
	ASSERT_NE(document.text({"record"}), string_view());
}
/**
 * @brief Проверка того, что код отказа и положение его отвечают последней работе
 *
 * @details Код отказа отвечает за ПОСЛЕДНЮЮ работу над деревом, а не за один разбор:
 *          переживи он удачную работу следом, потребитель судил бы о ней по причине,
 *          к ней отношения не имеющей
 *
 * @details Положение же ошибки принадлежит РАЗБОРУ текста, и работа над деревом текста
 *          под собою не имеет вовсе. Переживи оно код, донесение вышло бы стройным, но
 *          ложным - «раздел не объявлен в строке 1, столбце 5», указующее в текст,
 *          которого более нет
 *
 * @note Замерено щупом до правки: у INI положение прежнего разбора переживало и отказ
 *       работы, и удачную работу; у TOML сверх того переживал и сам КОД - сброса на
 *       входе не было вовсе, и удачный set() оставлял код прежнего отказа разбора
 *
 * @note Беду эту нашёл Василий у своих кодеков и передал соседям
 *
 */
TEST(CodecTomlDocument, ErrorAnswersForLastOperation) {
	// Объект дерева настроек
	toml::document_t document(::logger());
	// Выполняем проверку отказа разбора негодного текста
	ASSERT_FALSE(document.parse("a = [1, 2\n"));
	// Выполняем проверку того, что отказ разбора положение своё поставил
	ASSERT_NE(document.errorLocation().line, static_cast <uint32_t> (0));
	// Запоминаем код отказа разбора
	const toml::error_t parsed = document.error();
	// Выполняем проверку того, что код отказа разбора поставлен
	ASSERT_NE(parsed, toml::error_t::NONE);
	/**
	 * Выполняем проверку отказа работы над деревом сразу за отказом разбора
	 */
	ASSERT_FALSE(document.remove({"нет-такой"}));
	// Выполняем проверку того, что код отвечает работе последней, а не разбору
	ASSERT_EQ(document.error(), toml::error_t::UNKNOWN_TABLE);
	/**
	 * Выполняем проверку того, что положение прежнего разбора отказом работы снято
	 *
	 * @note Без сброса донесение указывало бы в текст, которого более нет
	 */
	ASSERT_EQ(document.errorLocation().line, static_cast <uint32_t> (0));
	// Выполняем проверку того, что положение прежнего разбора снято и по столбцу
	ASSERT_EQ(document.errorLocation().column, static_cast <uint32_t> (0));
	/**
	 * Выполняем проверку того, что удачная работа код отпускает
	 */
	ASSERT_TRUE(document.set({"ключ"}, static_cast <int64_t> (1)));
	// Выполняем проверку того, что код отказа удачной работой снят
	ASSERT_EQ(document.error(), toml::error_t::NONE);
	// Выполняем проверку того, что положение ошибки удачной работой снято
	ASSERT_EQ(document.errorLocation().line, static_cast <uint32_t> (0));
}
/**
 * @brief Проверка круга записи имени пары с переводом строки внутри
 *
 * @details Имя пары, кавычками ограждённое, вправе нести перевод строки отменяющей
 *          последовательностью: описание такое имя дозволяет. Запись же дерева обязана
 *          вернуть его тою же последовательностью, а не переводом строки дословным -
 *          иначе текст распался бы надвое, и круг «разбор - запись - разбор» дал бы
 *          иное дерево
 *
 * @note Отказов записи в составлении текста проверка НЕ задела: запись такое имя
 *       принимает, и девять строк распространения отказа у составления остаются
 *       открытыми - замерено охватом
 *
 */
TEST(CodecTomlDocument, KeyWithNewlineSurvivesRoundtrip) {
	// Объект дерева настроек
	toml::document_t document(::logger());
	// Выполняем проверку разбора имени пары с переводом строки внутри
	ASSERT_TRUE(document.parse("\"a\\nb\" = 1\n"));
	// Получаем записанный текст настроек
	const string text = document.text();
	// Выполняем проверку того, что записанный текст не пуст
	ASSERT_FALSE(text.empty());
	// Второе дерево настроек для круга
	toml::document_t other(::logger());
	// Выполняем проверку разбора записанного текста
	ASSERT_TRUE(other.parse(text));
	// Выполняем проверку того, что значение круг пережило
	ASSERT_EQ(other.text(), text);
}
/**
 * @brief Проверка передачи отказа записи изнутри составного значения
 *
 * @details Снятие дерева ведётся записью, и отказ её обязан дойти до потребителя:
 * снятое дерево без отвергнутой части выглядело бы годным текстом, а пропажи
 * потребитель не заметил бы вовсе. Пути передачи эти покрытием не задевались ни
 * разу - ни один набор не снимал дерева настройками своими, а настройками по
 * умолчанию запись не отвергает ничего
 *
 * @warning Отказ наводится пределом ГЛУБИНЫ, а не длины строки: предел длины
 *          проверяется у знака конца строки записи целиком, и запись многострочная
 *          отвергается им лишь по завершении своём - уже вне перечня. Замерено
 *          пределами от 3 до 40: ни один не задел ни одной искомой строки
 *
 * @note Глубина ноль отвергает само открытие составного значения, глубина единица -
 *       открытие вложенного в него: пути эти разные, и проверяются они порознь
 *
 */
TEST(CodecTomlDocument, DumpRefusalPropagates) {
	/**
	 * @brief Описание проверяемого захода передачи отказа
	 *
	 */
	struct probe_t {
		// Пояснение проверяемого захода
		const char * note;
		// Разбираемый текст настроек
		const char * text;
		// Предел глубины вложенности записи
		uint32_t depth;
	};
	// Набор проверяемых заходов передачи отказа
	static const probe_t PROBES[] = {
		{"открытие перечня значений", "b = [1, 2]\n", 0},
		{"открытие перечня, в перечень вложенного", "b = [1, [2, 3], 4]\n", 1},
		{"открытие встроенной таблицы", "c = {dd = 1}\n", 0},
		{"открытие таблицы, во встроенную таблицу вложенной", "c = {dd = {ff = 1}, ee = 2}\n", 1},
		{"открытие перечня, во встроенную таблицу вложенного", "c = {dd = [1, 2], ee = 3}\n", 1}
	};
	/**
	 * Выполняем перебор проверяемых заходов передачи отказа
	 */
	for(auto & probe : PROBES){
		// Объект дерева настроек
		toml::document_t document(::logger());
		// Выполняем разбор текста настроек
		ASSERT_TRUE(document.parse(probe.text)) << probe.note;
		// Получаем настройки записи, дереву отвечающие
		const toml::writer_t::settings_t writing = document.writing();
		// Получаем настройки записи снимаемого дерева настроек
		toml::writer_t::settings_t settings = writing;
		// Запоминаем предел глубины вложенности записываемого текста настроек
		settings.maxDepth = probe.depth;
		// Выполняем проверку того, что снятое дерево пусто
		EXPECT_TRUE(document.text(settings).empty()) << probe.note;
		// Выполняем проверку выданного кода ошибки записи дерева
		EXPECT_EQ(document.error(), toml::error_t::DEPTH_EXCEEDED) << probe.note;
		/**
		 * Выполняем проверку того, что дерево настройками своими снимается по-прежнему
		 *
		 * @note Заход этот стоит рядом нарочно: отказ, приходящий на всякое снятие,
		 *       отвечал бы тому же ожиданию, и порча снятия осталась бы незамечена
		 */
		EXPECT_FALSE(document.text(writing).empty()) << probe.note;
	}
	/**
	 * Выполняем проверку передачи отказа имени ключа пары встроенной таблицы
	 */
	{
		// Объект дерева настроек
		toml::document_t document(::logger());
		// Выполняем разбор текста настроек
		ASSERT_TRUE(document.parse("c = {dd = 1, ee = 2}\n"));
		// Получаем настройки записи снимаемого дерева настроек
		toml::writer_t::settings_t settings = document.writing();
		// Запоминаем предел длины имени ключа, имени верхнего уровня отвечающий
		settings.maxKey = 1;
		// Выполняем проверку того, что снятое дерево пусто
		ASSERT_TRUE(document.text(settings).empty());
		// Выполняем проверку выданного кода ошибки записи дерева
		ASSERT_EQ(document.error(), toml::error_t::KEY_TOO_LONG);
	}
}
/**
 * @brief Проверка уплотнения дерева настроек правкой
 *
 * @details Правка дерева оставляет за собою мусор - снятые записи да брошенные
 * знаки хранилища, - и дерево уплотняет себя само, когда мусора накопится довольно.
 * Уплотнение переносит содержимое в новое хранилище, и всякий вид, в старое
 * указывавший, обязан быть перенесён вместе с ним: примечания перечня, составные
 * имена пар встроенной таблицы, указатели записей
 *
 * @note Правок нарочно много: уплотнение заводится порогом мусора, и десяток правок
 *       его не достигает вовсе - дерево остаётся неуплотнённым, а проверка зелёной
 *
 */
TEST(CodecTomlDocument, CompactionKeepsContent) {
	// Объект дерева настроек
	toml::document_t document(::logger());
	// Разбираемый текст настроек с примечаниями перечня и встроенной таблицей
	const string text =
		"array = [\n\t1, # первое\n\t# своей строкой\n\t2\n]\n"
		"inline = {first = 1, second = \"два\"}\n"
		"[table]\n"
		"pair = \"значение\"\n";
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse(text));
	/**
	 * Выполняем правку дерева настроек до накопления мусора
	 */
	for(size_t i = 0; i < 200; i++){
		// Собираемое имя очередного правимого ключа
		const string name = "temp" + std::to_string(i);
		// Выполняем запись значения очередного ключа
		ASSERT_TRUE(document.set({name}, static_cast <int64_t> (i))) << i;
		// Выполняем снятие очередного ключа
		ASSERT_TRUE(document.erase({name})) << i;
	}
	/**
	 * Выполняем проверку сохранности примечаний перечня и содержимого пар
	 *
	 * @note Пробелы внутри встроенной таблицы ставит запись по настройке своей:
	 *       снятое дерево сличается с ожиданием, а не с исходным текстом
	 */
	ASSERT_EQ(document.text(), string(
		"array = [\n\t1, # первое\n\t# своей строкой\n\t2\n]\n"
		"inline = { first = 1, second = \"два\" }\n"
		"[table]\n"
		"pair = \"значение\"\n"
	));
	// Выполняем проверку сохранности значения пары встроенной таблицы
	ASSERT_EQ(document.text({"inline", "second"}), "два");
	// Выполняем проверку сохранности значения пары таблицы
	ASSERT_EQ(document.text({"table", "pair"}), "значение");
	/**
	 * Выполняем заведение новой пары следом за накоплением мусора
	 *
	 * @note Заведение ведёт своим путём, отличным от правки: уплотнение оно спрашивает
	 *       у себя, и путь этот покрытием не задевался - наборы правили уже заведённое
	 */
	ASSERT_TRUE(document.set({"fresh", "pair"}, static_cast <int64_t> (5)));
	// Выполняем проверку значения заведённой пары
	ASSERT_TRUE(document.has({"fresh", "pair"}));
	// Выполняем заведение перечня значений следом за накоплением мусора
	ASSERT_TRUE(document.arrange({"fresh", "list"}));
	// Выполняем проверку заведённого перечня значений
	ASSERT_TRUE(document.has({"fresh", "list"}));
	// Значение второго знака перечня
	toml::content_t item;
	// Выполняем получение второго значения перечня
	ASSERT_TRUE(document.item({"array"}, 1, item));
	// Выполняем проверку сохранности второго значения перечня
	ASSERT_EQ(item.integer, static_cast <int64_t> (2));
}
/**
 * @brief Проверка отказов правки дерева, покрытием не задетых
 *
 * @details Правка дерева отвергает путь, узла не имеющий, имя пустое и порядковый
 * номер, за отведённый ему отрезок значений выходящий. Заходы эти покрытием не
 * задевались ни разу: наборы правили дерево путями годными, а ворошитель строит
 * пути от уже разобранного текста и негодных не подаёт вовсе
 *
 * @note Путём негодным взят путь СКВОЗЬ значение простое: имя, узла не имеющее,
 *       правка заводит сама, и отказа на нём не будет - а вот пройти сквозь число
 *       вглубь нельзя, узла там нет и не заведётся
 *
 */
TEST(CodecTomlDocument, EditRefusalsNotCoveredBefore) {
	// Объект дерева настроек
	toml::document_t document(::logger());
	// Выполняем разбор текста настроек
	ASSERT_TRUE(document.parse("plain = 1\narray = [1, 2]\ntext = \"значение\"\ninline = {first = 1}\n"));
	// Устанавливаемое значение пары
	toml::content_t content;
	// Запоминаем тип устанавливаемого значения
	content.type = toml::type_t::INTEGER;
	// Запоминаем устанавливаемое значение
	content.integer = 5;
	// Выполняем проверку отказа записи значения сквозь значение простое
	EXPECT_FALSE(document.set({"plain", "inner"}, static_cast <int64_t> (5)));
	// Выполняем проверку отказа записи строкового значения сквозь значение простое
	EXPECT_FALSE(document.set({"plain", "inner"}, "значение"));
	// Выполняем проверку отказа дописывания значения перечня сквозь значение простое
	EXPECT_FALSE(document.push({"plain", "inner"}, content));
	// Выполняем проверку отказа записи логического значения сквозь значение простое
	EXPECT_FALSE(document.set({"plain", "inner"}, true));
	// Выполняем проверку отказа записи дробного значения сквозь значение простое
	EXPECT_FALSE(document.set({"plain", "inner"}, 1.5));
	// Отметка времени, записываемая значением пары
	toml::stamp_t stamp;
	// Выполняем проверку отказа записи отметки времени сквозь значение простое
	EXPECT_FALSE(document.set({"plain", "inner"}, stamp, toml::type_t::LOCAL_TIME));
	// Выполняем проверку отказа заведения перечня сквозь значение простое
	EXPECT_FALSE(document.arrange({"plain", "inner"}));
	// Выполняем проверку отказа заведения таблицы сквозь значение простое
	EXPECT_FALSE(document.arrange({"plain", "inner"}, true));
	/**
	 * Выполняем проверку отказа заведения пары встроенной таблицы с пустым именем
	 *
	 * @warning Путь берётся ГОДНЫЙ намеренно: путь негодный отвергается сам по себе, и
	 *          проверка проходила бы и с поверкою имени снесённой - срывом замерено,
	 *          что пустое имя принимается тогда молча
	 */
	EXPECT_FALSE(document.put({"inline"}, {}, content));
	/**
	 * Выполняем проверку того, что имя годное тем же путём принимается
	 *
	 * @note Заход этот стоит рядом нарочно: отказ, приходящий на всякое заведение,
	 *       отвечал бы тому же ожиданию, и порча поверки имени осталась бы незамечена
	 */
	EXPECT_TRUE(document.put({"inline"}, {"added"}, content));
	/**
	 * Выполняем проверку отказа порядкового номера сверх отрезка значений
	 *
	 * @note Порядковый номер разбирается из звена пути, и запись его вправе быть
	 *       любой длины: число, хранилищу не по мерке, отвергается разбором звена
	 */
	EXPECT_FALSE(document.has({"array", "99999999999999999999999999"}));
	// Выполняем проверку того, что порядковый номер годный распознаётся
	EXPECT_TRUE(document.has({"array", "1"}));
	/**
	 * Выполняем проверку отказа получения числа из значения строкового
	 */
	{
		// Получаемое дробное число значения
		double number = 0.;
		// Выполняем проверку отказа получения числа из значения строкового
		EXPECT_FALSE(document.value(number, {"text"}));
		// Выполняем проверку получения числа из значения целого
		EXPECT_TRUE(document.value(number, {"plain"}));
		// Выполняем проверку полученного числа
		EXPECT_DOUBLE_EQ(number, 1.);
	}
	/**
	 * Выполняем проверку правки дерева, разбора не знавшего
	 */
	{
		// Объект пустого дерева настроек
		toml::document_t empty(::logger());
		// Выполняем запись значения в пустое дерево настроек
		EXPECT_TRUE(empty.set({"first"}, static_cast <int64_t> (1)));
		// Выполняем проверку снятого дерева настроек
		EXPECT_EQ(empty.text(), "first = 1\n");
	}
}
/**
 * @brief Проверка перебора имён пар встроенной таблицы
 *
 * @details Пары встроенной таблицы хранятся не узлами дерева, а составными именами
 * при узле таблицы, и перебор их устроен своим путём: имя вправе быть составным,
 * и звенья его сличаются с остатком запрошенного пути. Путь этот покрытием не
 * задевался - наборы перебирали имена таблиц обычных
 *
 */
TEST(CodecTomlDocument, InlineTableKeys) {
	// Объект дерева настроек
	toml::document_t document(::logger());
	// Выполняем разбор текста настроек со встроенной таблицей
	ASSERT_TRUE(document.parse("inline = {first = 1, nested.deep = 2, second = 3, other.far = 4}\n"));
	// Получаем имена пар встроенной таблицы
	const vector <string_view> names = document.keys({"inline"});
	// Выполняем проверку количества полученных имён пар встроенной таблицы
	ASSERT_EQ(names.size(), static_cast <size_t> (4));
	// Выполняем проверку первого полученного имени пары встроенной таблицы
	ASSERT_EQ(names.at(0), "first");
	// Выполняем проверку второго полученного имени пары встроенной таблицы
	ASSERT_EQ(names.at(1), "nested");
	// Выполняем проверку третьего полученного имени пары встроенной таблицы
	ASSERT_EQ(names.at(2), "second");
	// Выполняем проверку четвёртого полученного имени пары встроенной таблицы
	ASSERT_EQ(names.at(3), "other");
	// Получаем имена пар вложенной части составного имени
	const vector <string_view> nested = document.keys({"inline", "nested"});
	// Выполняем проверку количества полученных имён вложенной части
	ASSERT_EQ(nested.size(), static_cast <size_t> (1));
	// Выполняем проверку полученного имени вложенной части
	ASSERT_EQ(nested.at(0), "deep");
	/**
	 * Получаем имена пар второго составного имени той же длины
	 *
	 * @note Имя это заведено нарочно: сличение остатка составного имени с путём
	 *       расходится лишь тогда, когда длины их равны, а звенья разнятся -
	 *       имена разной длины отсеиваются заходом выше, и путь расхождения
	 *       оставался нетронутым
	 */
	const vector <string_view> other = document.keys({"inline", "other"});
	// Выполняем проверку количества полученных имён второго составного имени
	ASSERT_EQ(other.size(), static_cast <size_t> (1));
	// Выполняем проверку полученного имени второго составного имени
	ASSERT_EQ(other.at(0), "far");
	/**
	 * Выполняем проверку того, что имя чужого составного имени сюда не попало
	 *
	 * @warning Звенья вторых частей взяты РАЗНЫМИ намеренно: при одинаковых сличение
	 *          звеньев ничего не решает - выдача совпадает и с поверкой снесённой.
	 *          Замерено срывом: с «other.deep» порча сличения проверки не роняла
	 */
	ASSERT_EQ(nested.at(0), "deep");
	// Получаем имена пар по имени, встроенной таблице не принадлежащему
	const vector <string_view> foreign = document.keys({"inline", "first"});
	/**
	 * Выполняем проверку того, что имён по чужому имени не выдано
	 *
	 * @note Заход этот стоит рядом нарочно: перебор, выдающий имена на всякий путь,
	 *       отвечал бы тем же ожиданиям на путях годных
	 */
	ASSERT_TRUE(foreign.empty());
}
/**
 * @brief Проверка границы посредника, судящего имя ключа по одному знаку
 *
 * @details Посредник `bare()` возвращает `bool`, выглядит приговором о голом имени и
 *          молчит о том, чего не видит: на входе у него один знак набора US-ASCII - ни
 *          длины имени, ни настроек. Обход имени им расходится с записью по двум
 *          поводам, и оба закреплены здесь: имя пустое обходом даёт истину впустую, а
 *          записывается кавычками; имя из знаков письменностей мира при настройке
 *          `unicode` записывается голым, тогда как посредник ложен на каждом его байте
 *
 * @note Закрепляется и согласие там, где оно есть: имя из знаков голых записывается
 *       голым, и приговор посредника с приговором записи совпадает
 *
 */
TEST(CodecTomlDocument, BarePredicateScopeAgainstWriter) {
	// Выполняем проверку того, что посредник голые знаки признаёт
	ASSERT_TRUE(toml::bare('a'));
	// Выполняем проверку того, что посредник черту и подчёркивание признаёт
	ASSERT_TRUE(toml::bare('-') && toml::bare('_'));
	// Выполняем проверку того, что посредник точку голой не признаёт
	ASSERT_FALSE(toml::bare('.'));
	// Выполняем проверку того, что посредник ложен на байте знака письменности
	ASSERT_FALSE(toml::bare(static_cast <char> (0xD0)));
	// Выполняем проверку того, что знак письменности признаётся по коду, а не по байту
	ASSERT_TRUE(toml::named(0x043A));
	{
		// Дерево настроек
		toml::document_t document(::logger());
		// Выполняем проверку успешности разбора пустого текста настроек
		ASSERT_TRUE(document.parse(""));
		// Выполняем установку свойства с именем из знаков голых
		ASSERT_TRUE(document.set(vector <string_view> {"a-b_1"}, string_view("v")));
		// Выполняем проверку того, что имя записано голым
		ASSERT_NE(document.text().find("a-b_1 = "), string::npos);
	}{
		// Дерево настроек
		toml::document_t document(::logger());
		// Выполняем проверку успешности разбора пустого текста настроек
		ASSERT_TRUE(document.parse(""));
		// Выполняем установку свойства с именем пустым
		ASSERT_TRUE(document.set(vector <string_view> {""}, string_view("v")));
		// Выполняем проверку того, что пустое имя записано кавычками
		ASSERT_NE(document.text().find("\"\" = "), string::npos);
	}{
		// Дерево настроек
		toml::document_t document(::logger());
		// Выполняем проверку успешности разбора пустого текста настроек
		ASSERT_TRUE(document.parse(""));
		// Выполняем установку свойства с именем из знаков письменности
		ASSERT_TRUE(document.set(vector <string_view> {"ключ"}, string_view("v")));
		// Выполняем проверку того, что умолчанием имя записано кавычками
		ASSERT_NE(document.text().find("\"ключ\" = "), string::npos);
		// Настройки записи текста настроек
		toml::writer_t::settings_t settings;
		// Дозволяем знаки письменностей мира в имени ключа
		settings.unicode = true;
		// Записанный текст настроек с дозволением знаков письменностей
		const string text = document.text(settings);
		// Выполняем проверку того, что дозволением имя записано голым
		ASSERT_NE(text.find("ключ = "), string::npos);
		// Выполняем проверку того, что кавычек у имени не осталось
		ASSERT_EQ(text.find("\"ключ\""), string::npos);
	}
}
