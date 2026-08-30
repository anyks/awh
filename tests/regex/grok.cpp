/**
 * @file grok.cpp
 * @date 2026-08-04
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
 * @brief Автоматические тесты модуля Grok —
 *        реестр именованных шаблонов, разворот ссылок вида «%{NAME:поле:вид}»
 *        в регулярное выражение и извлечение именованных полей из текста
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <unordered_map>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <regex/grok/grok.hpp>

/**
 * Подключаем заголовочные файлы тестового окружения
 */
#include "../main.hpp"
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

/**
 * @brief Метод получения записи JSON извлечённых полей текстом
 *
 * @details Надстройка Grok отдаёт извлечённые поля значением кодека, а не
 *          текстом: строковые перегрузки убраны как повторяющие вывод самого
 *          кодека. Проверкам же текст нужен для сличения, отчего посредник
 *          и заведён тут, а не в надстройке.
 *
 * @param grok   объект разбора текста по шаблонам Grok
 * @param text   текст сопоставления шаблона
 * @param exp    собранный шаблон Grok
 * @param result записываемая запись JSON извлечённых полей
 * @param pretty признак вывода записи с отступами
 * @return       результат вывода извлечённых полей записью JSON
 *
 */
static bool dumping(const awh::grok_t & grok, const string & text,
 const awh::grok_t::exp_t & exp, string & result, const bool pretty = false) noexcept {
	// Выполняем очистку записи JSON извлечённых полей
	result.clear();
	// Значение JSON извлечённых полей
	awh::grok::json_t value;
	/**
	 * Если вывод извлечённых полей значением не выполнен
	 */
	if(!grok.json(text, exp, value))
		// Выводим результат вывода извлечённых полей записью JSON
		return false;
	// Выполняем вывод записи JSON извлечённых полей
	result = value.dump(pretty ?
	 awh::codec::json::format_t::PRETTY : awh::codec::json::format_t::COMPACT);
	// Выводим результат вывода извлечённых полей записью JSON
	return !result.empty();
}

/**
 * @brief Тест сборки всего встроенного набора шаблонов
 *
 * @details Каждый шаблон набора собирается отдельно: набор поставляется вместе
 *          с модулем, и запись, собрать какую нельзя, обнаруживается здесь, а
 *          не у потребителя.
 *
 */
TEST(Grok, PatternsBuild) {
	// Создаём объект разбора текста по шаблонам Grok
	grok_t grok(::logger());
	/**
	 * Выполняем перебор встроенного набора шаблонов
	 */
	for(size_t i = 0; i < grok::PATTERNS_COUNT; i++) {
		// Получаем текст ссылки на шаблон набора
		const string text = string("%{") + grok::PATTERNS[i].name + "}";
		// Выполняем сборку шаблона Grok
		const auto exp = grok.build(text);
		// Выполняем проверку сборки шаблона Grok
		EXPECT_TRUE(!!exp) << "Шаблон \"" << grok::PATTERNS[i].name << "\" не собран, код "
		                   << static_cast <uint32_t> (grok.error());
	}
	// Выполняем проверку количества шаблонов набора
	EXPECT_EQ(grok::PATTERNS_COUNT, 307);
}
/**
 * @brief Тест извлечения именованных полей из текста
 *
 */
TEST(Grok, FieldsExtract) {
	// Создаём объект разбора текста по шаблонам Grok
	grok_t grok(::logger());
	// Выполняем сборку шаблона Grok
	const auto exp = grok.build("%{IP:client} %{WORD:method} %{URIPATHPARAM:request}");
	// Выполняем проверку сборки шаблона Grok
	ASSERT_TRUE(!!exp);
	// Набор извлечённых полей
	unordered_map <string, string> fields;
	// Выполняем извлечение именованных полей из текста
	ASSERT_TRUE(grok.exec("10.0.0.7 GET /index.html?a=1", exp, fields));
	// Выполняем проверку извлечённых полей
	EXPECT_EQ(fields["client"], "10.0.0.7");
	EXPECT_EQ(fields["method"], "GET");
	EXPECT_EQ(fields["request"], "/index.html?a=1");
}
/**
 * @brief Тест нумерации групп захвата вложенных шаблонов
 *
 * @details Шаблон «COMMONAPACHELOG» ссылается на шаблоны, несущие группы
 *          захвата и впрямую - «IPV6» одна несёт их четыре десятка, - поэтому
 *          номер группы поля определяется не порядком полей, а порядком
 *          открывающих скобок текста развёрнутого. Тест закрепляет подсчёт.
 *
 */
TEST(Grok, NestedNumbering) {
	// Создаём объект разбора текста по шаблонам Grok
	grok_t grok(::logger());
	// Выполняем сборку шаблона Grok
	const auto exp = grok.build("%{COMMONAPACHELOG}");
	// Выполняем проверку сборки шаблона Grok
	ASSERT_TRUE(!!exp);
	// Набор извлечённых полей
	unordered_map <string, string> fields;
	// Выполняем извлечение именованных полей из текста
	ASSERT_TRUE(grok.exec("127.0.0.1 - frank [10/Oct/2000:13:55:36 -0700] \"GET /apache_pb.gif HTTP/1.0\" 200 2326", exp, fields));
	// Выполняем проверку извлечённых полей
	EXPECT_EQ(fields["clientip"], "127.0.0.1");
	EXPECT_EQ(fields["auth"], "frank");
	EXPECT_EQ(fields["timestamp"], "10/Oct/2000:13:55:36 -0700");
	EXPECT_EQ(fields["verb"], "GET");
	EXPECT_EQ(fields["request"], "/apache_pb.gif");
	EXPECT_EQ(fields["httpversion"], "1.0");
	EXPECT_EQ(fields["response"], "200");
	EXPECT_EQ(fields["bytes"], "2326");
}
/**
 * @brief Тест видов значений полей
 *
 * @details Ссылка вида «%{INT:code:int}» задаёт вид значения третьей частью.
 *          Модуль вид запоминает, но приведением значения не занимается.
 *
 */
TEST(Grok, FieldKinds) {
	// Создаём объект разбора текста по шаблонам Grok
	grok_t grok(::logger());
	// Выполняем сборку шаблона Grok
	const auto exp = grok.build("%{WORD:name} %{INT:code:int} %{NUMBER:rate:float}");
	// Выполняем проверку сборки шаблона Grok
	ASSERT_TRUE(!!exp);
	// Получаем набор полей собранного шаблона
	const auto & fields = grok.fields(exp);
	// Выполняем проверку количества полей собранного шаблона
	ASSERT_EQ(fields.size(), 3);
	// Выполняем проверку видов значений полей
	EXPECT_EQ(fields.at(0).name, "name");
	EXPECT_EQ(fields.at(0).kind, grok_t::kind_t::TEXT);
	EXPECT_EQ(fields.at(1).name, "code");
	EXPECT_EQ(fields.at(1).kind, grok_t::kind_t::INTEGER);
	EXPECT_EQ(fields.at(2).name, "rate");
	EXPECT_EQ(fields.at(2).kind, grok_t::kind_t::FLOATING);
	// Набор извлечённых полей
	unordered_map <string, string> values;
	// Выполняем извлечение именованных полей из текста
	ASSERT_TRUE(grok.exec("node 42 3.14", exp, values));
	// Выполняем проверку выдачи значений текстом
	EXPECT_EQ(values["code"], "42");
	EXPECT_EQ(values["rate"], "3.14");
}
/**
 * @brief Тест названий полей, названиям групп выражения не отвечающих
 *
 * @details Названия полей Grok шире названий групп регулярного выражения: в
 *          них встречаются дефис и точка, а внутри одного шаблона название
 *          вправе повторяться. Тест закрепляет решение о своей таблице полей.
 *
 */
TEST(Grok, FieldNamesWide) {
	// Создаём объект разбора текста по шаблонам Grok
	grok_t grok(::logger());
	// Выполняем сборку шаблона Grok
	const auto exp = grok.build("%{IP:src-ip}:%{INT:src.port} %{IP:dst-ip}");
	// Выполняем проверку сборки шаблона Grok
	ASSERT_TRUE(!!exp);
	// Набор извлечённых полей
	unordered_map <string, string> fields;
	// Выполняем извлечение именованных полей из текста
	ASSERT_TRUE(grok.exec("10.0.0.1:443 10.0.0.2", exp, fields));
	// Выполняем проверку извлечённых полей
	EXPECT_EQ(fields["src-ip"], "10.0.0.1");
	EXPECT_EQ(fields["src.port"], "443");
	EXPECT_EQ(fields["dst-ip"], "10.0.0.2");
}
/**
 * @brief Тест повторного объявления названия поля
 *
 * @details Шаблон «BACULA_LOGLINE» объединяет ветви, объявляющие одно и то же
 *          название группы впрямую, поэтому выражение собирается с режимом
 *          DUPNAMES. Тест закрепляет решение.
 *
 */
TEST(Grok, DuplicateNames) {
	// Создаём объект разбора текста по шаблонам Grok
	grok_t grok(::logger());
	// Выполняем сборку шаблона Grok
	EXPECT_TRUE(!!grok.build("%{BACULA_LOGLINE}"));
	// Выполняем сборку шаблона Grok с повторяющимся названием поля
	const auto exp = grok.build("%{WORD:tag}-%{WORD:tag}");
	// Выполняем проверку сборки шаблона Grok
	ASSERT_TRUE(!!exp);
	// Выполняем проверку количества полей собранного шаблона
	EXPECT_EQ(grok.fields(exp).size(), 2);
}
/**
 * @brief Тест пользовательских шаблонов реестра
 *
 */
TEST(Grok, RegistryCustom) {
	// Создаём объект разбора текста по шаблонам Grok
	grok_t grok(::logger());
	// Выполняем проверку наличия встроенного шаблона в реестре
	EXPECT_TRUE(grok.has("IPV4"));
	// Выполняем проверку отсутствия пользовательского шаблона в реестре
	EXPECT_FALSE(grok.has("ANYKS_TAG"));
	// Выполняем добавление пользовательского шаблона в реестр
	ASSERT_TRUE(grok.pattern("ANYKS_TAG", "\\[%{WORD:tag}\\]"));
	// Выполняем проверку наличия пользовательского шаблона в реестре
	EXPECT_TRUE(grok.has("ANYKS_TAG"));
	// Выполняем сборку шаблона Grok
	const auto exp = grok.build("%{ANYKS_TAG} %{GREEDYDATA:message}");
	// Выполняем проверку сборки шаблона Grok
	ASSERT_TRUE(!!exp);
	// Набор извлечённых полей
	unordered_map <string, string> fields;
	// Выполняем извлечение именованных полей из текста
	ASSERT_TRUE(grok.exec("[debug] all is well", exp, fields));
	// Выполняем проверку извлечённых полей
	EXPECT_EQ(fields["tag"], "debug");
	EXPECT_EQ(fields["message"], "all is well");
	// Выполняем удаление пользовательского шаблона из реестра
	EXPECT_TRUE(grok.erase("ANYKS_TAG"));
	// Выполняем проверку отсутствия пользовательского шаблона в реестре
	EXPECT_FALSE(grok.has("ANYKS_TAG"));
	// Выполняем восстановление встроенного набора шаблонов
	grok.reset();
	// Выполняем проверку наличия встроенного шаблона в реестре
	EXPECT_TRUE(grok.has("IPV4"));
}
/**
 * @brief Тест замены встроенного шаблона пользовательским
 *
 */
TEST(Grok, RegistryOverride) {
	// Создаём объект разбора текста по шаблонам Grok
	grok_t grok(::logger());
	// Выполняем замену встроенного шаблона пользовательским
	ASSERT_TRUE(grok.pattern("WORD", "[0-9]+"));
	// Выполняем сборку шаблона Grok
	const auto exp = grok.build("%{WORD:value}");
	// Выполняем проверку сборки шаблона Grok
	ASSERT_TRUE(!!exp);
	// Набор извлечённых полей
	unordered_map <string, string> fields;
	// Выполняем извлечение именованных полей из текста
	ASSERT_TRUE(grok.exec("abc 12345", exp, fields));
	// Выполняем проверку замены встроенного шаблона
	EXPECT_EQ(fields["value"], "12345");
	// Выполняем восстановление встроенного набора шаблонов
	grok.reset();
	// Выполняем проверку восстановления встроенного шаблона
	EXPECT_EQ(grok.pattern("WORD"), "\\b\\w+\\b");
}
/**
 * @brief Тест обнаружения круговых ссылок
 *
 */
TEST(Grok, CircularReference) {
	// Создаём объект разбора текста по шаблонам Grok
	grok_t grok(::logger());
	// Выполняем добавление шаблона, ссылающегося на себя
	ASSERT_TRUE(grok.pattern("LOOP", "a%{LOOP}b"));
	// Выполняем сборку шаблона Grok
	EXPECT_FALSE(!!grok.build("%{LOOP}"));
	// Выполняем проверку кода ошибки разбора шаблона
	EXPECT_EQ(grok.error(), grok_t::error_t::REFERENCE_CIRCULAR);
	// Выполняем добавление шаблонов, ссылающихся друг на друга
	ASSERT_TRUE(grok.pattern("PING", "a%{PONG}"));
	ASSERT_TRUE(grok.pattern("PONG", "b%{PING}"));
	// Выполняем сборку шаблона Grok
	EXPECT_FALSE(!!grok.build("%{PING}"));
	// Выполняем проверку кода ошибки разбора шаблона
	EXPECT_EQ(grok.error(), grok_t::error_t::REFERENCE_CIRCULAR);
}
/**
 * @brief Тест обнаружения ошибок разбора шаблона
 *
 */
TEST(Grok, Errors) {
	// Создаём объект разбора текста по шаблонам Grok
	grok_t grok(::logger());
	// Выполняем сборку шаблона с пустым текстом
	EXPECT_FALSE(!!grok.build(""));
	EXPECT_EQ(grok.error(), grok_t::error_t::PATTERN_EMPTY);
	// Выполняем сборку шаблона с незакрытой скобкой ссылки
	EXPECT_FALSE(!!grok.build("%{WORD"));
	EXPECT_EQ(grok.error(), grok_t::error_t::REFERENCE_UNCLOSED);
	// Выполняем сборку шаблона со ссылкой на неизвестный шаблон
	EXPECT_FALSE(!!grok.build("%{NOSUCHPATTERN}"));
	EXPECT_EQ(grok.error(), grok_t::error_t::REFERENCE_UNKNOWN);
	// Выполняем сборку шаблона со ссылкой без названия
	EXPECT_FALSE(!!grok.build("%{}"));
	EXPECT_EQ(grok.error(), grok_t::error_t::REFERENCE_EMPTY);
	// Выполняем сборку шаблона со ссылкой с пустым названием поля
	EXPECT_FALSE(!!grok.build("%{WORD:}"));
	EXPECT_EQ(grok.error(), grok_t::error_t::FIELD_EMPTY);
	// Выполняем сборку шаблона со ссылкой с неизвестным видом значения
	EXPECT_FALSE(!!grok.build("%{WORD:value:money}"));
	EXPECT_EQ(grok.error(), grok_t::error_t::KIND_UNKNOWN);
	// Выполняем сборку шаблона с выражением, сборке не поддающимся
	EXPECT_FALSE(!!grok.build("(%{WORD:value}"));
	EXPECT_EQ(grok.error(), grok_t::error_t::EXPRESSION);
}
/**
 * @brief Тест отсутствия совпадения с текстом
 *
 */
TEST(Grok, NoMatching) {
	// Создаём объект разбора текста по шаблонам Grok
	grok_t grok(::logger());
	// Выполняем сборку шаблона Grok
	const auto exp = grok.build("%{IP:client} %{WORD:method}");
	// Выполняем проверку сборки шаблона Grok
	ASSERT_TRUE(!!exp);
	// Выполняем проверку отсутствия совпадения с текстом
	EXPECT_FALSE(grok.test("совпадения здесь нет", exp));
	// Набор извлечённых полей
	unordered_map <string, string> fields;
	// Выполняем извлечение именованных полей из текста
	EXPECT_FALSE(grok.exec("совпадения здесь нет", exp, fields));
	// Выполняем проверку пустоты набора извлечённых полей
	EXPECT_TRUE(fields.empty());
}
/**
 * @brief Тест поля, захват какого не выполнен
 *
 * @details Поле, захват какого не выполнен, в набор не добавляется: отличить
 *          пустой захват от невыполненного иначе нельзя.
 *
 */
TEST(Grok, FieldNotCaptured) {
	// Создаём объект разбора текста по шаблонам Grok
	grok_t grok(::logger());
	// Выполняем сборку шаблона Grok
	const auto exp = grok.build("%{WORD:name}(?: %{INT:code})?");
	// Выполняем проверку сборки шаблона Grok
	ASSERT_TRUE(!!exp);
	// Набор извлечённых полей
	unordered_map <string, string> fields;
	// Выполняем извлечение именованных полей из текста
	ASSERT_TRUE(grok.exec("node", exp, fields));
	// Выполняем проверку извлечённых полей
	EXPECT_EQ(fields["name"], "node");
	EXPECT_EQ(fields.count("code"), 0);
	// Выполняем извлечение именованных полей из текста
	ASSERT_TRUE(grok.exec("node 42", exp, fields));
	// Выполняем проверку извлечённых полей
	EXPECT_EQ(fields["code"], "42");
}
/**
 * @brief Тест экранирования знака ссылки
 *
 */
TEST(Grok, EscapedReference) {
	// Создаём объект разбора текста по шаблонам Grok
	grok_t grok(::logger());
	// Выполняем сборку шаблона Grok
	const auto exp = grok.build("\\%\\{WORD\\} %{WORD:value}");
	// Выполняем проверку сборки шаблона Grok
	ASSERT_TRUE(!!exp);
	// Набор извлечённых полей
	unordered_map <string, string> fields;
	// Выполняем извлечение именованных полей из текста
	ASSERT_TRUE(grok.exec("%{WORD} node", exp, fields));
	// Выполняем проверку извлечённых полей
	EXPECT_EQ(fields["value"], "node");
}
/**
 * @brief Тест кэша собранных шаблонов
 *
 * @details Повторная сборка того же шаблона с тем же набором режимов выводит
 *          собранный ранее шаблон, пока его удерживает вызывающая сторона.
 *          Правка реестра кэш очищает целиком: развёрнутый текст зависит от
 *          всего реестра, а не от одного текста шаблона.
 *
 */
TEST(Grok, Caching) {
	// Создаём объект разбора текста по шаблонам Grok
	grok_t grok(::logger());
	// Выполняем сборку шаблона Grok
	const auto first = grok.build("%{IP:client} %{WORD:method}");
	// Выполняем проверку сборки шаблона Grok
	ASSERT_TRUE(!!first);
	// Выполняем повторную сборку того же шаблона Grok
	const auto second = grok.build("%{IP:client} %{WORD:method}");
	// Выполняем проверку выдачи собранного ранее шаблона
	EXPECT_EQ(first.get(), second.get());
	// Выполняем сборку того же шаблона с иным набором режимов
	const auto third = grok.build("%{IP:client} %{WORD:method}", {grok_t::flag_t::CASELESS});
	// Выполняем проверку сборки отдельного шаблона
	ASSERT_TRUE(!!third);
	EXPECT_NE(first.get(), third.get());
	// Выполняем замену встроенного шаблона пользовательским
	ASSERT_TRUE(grok.pattern("WORD", "[0-9]+"));
	// Выполняем сборку того же шаблона Grok
	const auto fourth = grok.build("%{IP:client} %{WORD:method}");
	// Выполняем проверку сборки шаблона заново
	ASSERT_TRUE(!!fourth);
	EXPECT_NE(first.get(), fourth.get());
	// Набор извлечённых полей
	unordered_map <string, string> fields;
	// Выполняем извлечение именованных полей из текста
	ASSERT_TRUE(grok.exec("10.0.0.1 42", fourth, fields));
	// Выполняем проверку действия правки реестра
	EXPECT_EQ(fields["method"], "42");
}
/**
 * @brief Тест извлечения значений полей набором
 *
 * @details Выдача набором сохраняет порядок объявления полей и повторы
 *          названий, какие выдача набором соответствий теряет.
 *
 */
TEST(Grok, ValuesExtract) {
	// Создаём объект разбора текста по шаблонам Grok
	grok_t grok(::logger());
	// Выполняем сборку шаблона Grok
	const auto exp = grok.build("%{WORD:tag}-%{WORD:tag} %{INT:code:int} %{NUMBER:rate:float}");
	// Выполняем проверку сборки шаблона Grok
	ASSERT_TRUE(!!exp);
	// Набор извлечённых значений полей
	vector <grok_t::value_t> values;
	// Выполняем извлечение значений полей из текста
	ASSERT_TRUE(grok.exec("alpha-bravo 42 3.14", exp, values));
	// Выполняем проверку количества извлечённых значений полей
	ASSERT_EQ(values.size(), 4);
	// Выполняем проверку сохранения порядка и повторов названий
	EXPECT_EQ(values.at(0).name, "tag");
	EXPECT_EQ(values.at(0).value, "alpha");
	EXPECT_EQ(values.at(1).name, "tag");
	EXPECT_EQ(values.at(1).value, "bravo");
	// Выполняем проверку видов значений полей
	EXPECT_EQ(values.at(2).name, "code");
	EXPECT_EQ(values.at(2).value, "42");
	EXPECT_EQ(values.at(2).kind, grok_t::kind_t::INTEGER);
	EXPECT_EQ(values.at(3).name, "rate");
	EXPECT_EQ(values.at(3).value, "3.14");
	EXPECT_EQ(values.at(3).kind, grok_t::kind_t::FLOATING);
	// Набор извлечённых полей
	unordered_map <string, string> fields;
	// Выполняем извлечение именованных полей из текста
	ASSERT_TRUE(grok.exec("alpha-bravo 42 3.14", exp, fields));
	// Выполняем проверку потери повтора названия выдачей набором соответствий
	EXPECT_EQ(fields.size(), 3);
	// Выполняем проверку отсутствия совпадения с текстом
	EXPECT_FALSE(grok.exec("совпадения здесь нет", exp, values));
	// Выполняем проверку пустоты набора извлечённых значений полей
	EXPECT_TRUE(values.empty());
}
/**
 * @brief Тест вывода извлечённых полей записью JSON
 *
 */
TEST(Grok, JsonOutput) {
	// Создаём объект разбора текста по шаблонам Grok
	grok_t grok(::logger());
	// Выполняем сборку шаблона Grok
	const auto exp = grok.build("%{WORD:name} %{INT:code:int} %{NUMBER:rate:float}");
	// Выполняем проверку сборки шаблона Grok
	ASSERT_TRUE(!!exp);
	// Запись JSON извлечённых полей
	string result;
	// Выполняем вывод извлечённых полей записью JSON
	ASSERT_TRUE(dumping(grok, "node 42 3.14", exp, result));
	// Выполняем проверку приведения значений по виду поля
	EXPECT_EQ(result, "{\"name\":\"node\",\"code\":42,\"rate\":3.14}");
	// Выполняем вывод извлечённых полей записью JSON с отступами
	ASSERT_TRUE(dumping(grok, "node 42 3.14", exp, result, true));
	/**
	 * Выполняем проверку вывода записи с отступами
	 *
	 * @details Отступ ведёт кодек, а не Grok: прежде Grok отступал табуляцией
	 *          собственного изготовления, теперь отступ берётся видом записи
	 *          кодека и составляет два пробела. Изменение это наблюдаемое
	 *          и намеренное - вид записи задаётся настройками кодека,
	 *          а не зашивается в потребителя.
	 *
	 */
	EXPECT_EQ(result, "{\n  \"name\": \"node\",\n  \"code\": 42,\n  \"rate\": 3.14\n}");
	// Выполняем проверку отказа вывода при отсутствии совпадения
	EXPECT_FALSE(dumping(grok, "совпадения здесь нет", exp, result));
	// Выполняем проверку очистки записи JSON
	EXPECT_TRUE(result.empty());
}
/**
 * @brief Тест вывода текстом захвата, объявленному виду не отвечающего
 *
 * @details Числом выводится лишь захват, объявленному виду отвечающий целиком:
 *          запись «42abc» числом объявлена быть вправе, но числом не является,
 *          и вывод её числом дал бы запись JSON неправильную.
 *
 */
TEST(Grok, JsonKindMismatch) {
	// Создаём объект разбора текста по шаблонам Grok
	grok_t grok(::logger());
	// Выполняем добавление пользовательского шаблона в реестр
	ASSERT_TRUE(grok.pattern("ANYTHING", "\\S+"));
	// Выполняем сборку шаблона Grok
	const auto exp = grok.build("%{ANYTHING:value:int}");
	// Выполняем проверку сборки шаблона Grok
	ASSERT_TRUE(!!exp);
	// Запись JSON извлечённых полей
	string result;
	// Выполняем вывод извлечённых полей записью JSON
	ASSERT_TRUE(dumping(grok, "42abc", exp, result));
	// Выполняем проверку вывода захвата текстом
	EXPECT_EQ(result, "{\"value\":\"42abc\"}");
	// Выполняем вывод извлечённых полей записью JSON
	ASSERT_TRUE(dumping(grok, "42", exp, result));
	// Выполняем проверку вывода захвата числом
	EXPECT_EQ(result, "{\"value\":42}");
	// Выполняем сборку шаблона Grok без объявления вида значения
	const auto plain = grok.build("%{ANYTHING:value}");
	// Выполняем проверку сборки шаблона Grok
	ASSERT_TRUE(!!plain);
	// Выполняем вывод извлечённых полей записью JSON
	ASSERT_TRUE(dumping(grok, "42", plain, result));
	// Выполняем проверку вывода захвата текстом при виде необъявленном
	EXPECT_EQ(result, "{\"value\":\"42\"}");
}
/**
 * @brief Тест экранирования записи JSON
 *
 */
TEST(Grok, JsonEscaping) {
	// Создаём объект разбора текста по шаблонам Grok
	grok_t grok(::logger());
	// Выполняем сборку шаблона Grok
	const auto exp = grok.build("%{GREEDYDATA:message}");
	// Выполняем проверку сборки шаблона Grok
	ASSERT_TRUE(!!exp);
	// Запись JSON извлечённых полей
	string result;
	// Выполняем вывод извлечённых полей записью JSON
	ASSERT_TRUE(dumping(grok, "кавычка \" косая \\ таб\tи \x01", exp, result));
	// Выполняем проверку экранирования записи JSON
	EXPECT_EQ(result, "{\"message\":\"кавычка \\\" косая \\\\ таб\\tи \\u0001\"}");
}
/**
 * @brief Тест краёв записи числа в выводе JSON
 *
 * @details Числом ложится лишь захват, числом JSON являющийся: вид, полем
 *          объявленный, это дозволяет, но не предписывает. Записи, привычными
 *          средствами разбора принимаемые, а договором JSON запрещённые -
 *          ведущие нули, «3.», «.5», «nan», «inf», «0x10» и знак плюса, -
 *          выводятся текстом.
 *
 *          Отдельно проверяется выход за предел вида: захват, числом
 *          не представимый, обязан лечь текстом, а не быть подведённым
 *          к пределу. Преобразование средствами библиотеки С выход за предел
 *          не отвергает, а подводит к пределу самому, отчего
 *          «18446744073709551616» легло бы числом «18446744073709551615» -
 *          записью иной. То же и с «1e999»: числом JSON оно является,
 *          а видом дробным не представимо.
 *
 *          Образцы обнаружены разбором выданного самим кодеком JSON: запись,
 *          кодеком не разбираемая, есть запись неправильная. Прежний вывод,
 *          собственноручный, давал неправильную запись на восьми образцах
 *          из двадцати.
 *
 */
TEST(Grok, JsonNumberEdges) {
	// Создаём объект разбора текста по шаблонам Grok
	grok_t grok(::logger());
	// Выполняем добавление пользовательского шаблона в реестр
	ASSERT_TRUE(grok.pattern("ANY", "[\\s\\S]+"));
	/**
	 * @brief Набор образцов вывода числа
	 *
	 */
	const struct {
		// Текст шаблона Grok
		const char * pattern;
		// Текст сопоставления шаблона
		const char * text;
		// Ожидаемая запись JSON
		const char * expected;
	} samples[] = {
		// Числа, договору JSON отвечающие
		{"%{ANY:value:int}",   "0",                    "{\"value\":0}"},
		{"%{ANY:value:int}",   "42",                   "{\"value\":42}"},
		{"%{ANY:value:int}",   "-42",                  "{\"value\":-42}"},
		{"%{ANY:value:int}",   "9223372036854775807",  "{\"value\":9223372036854775807}"},
		{"%{ANY:value:int}",   "9223372036854775808",  "{\"value\":9223372036854775808}"},
		{"%{ANY:value:float}", "3.14",                 "{\"value\":3.14}"},
		// Записи, договором JSON числом не признаваемые
		{"%{ANY:value:int}",   "007",                  "{\"value\":\"007\"}"},
		{"%{ANY:value:int}",   "+42",                  "{\"value\":\"+42\"}"},
		{"%{ANY:value:float}", "3.",                   "{\"value\":\"3.\"}"},
		{"%{ANY:value:float}", ".5",                   "{\"value\":\".5\"}"},
		{"%{ANY:value:float}", "nan",                  "{\"value\":\"nan\"}"},
		{"%{ANY:value:float}", "inf",                  "{\"value\":\"inf\"}"},
		{"%{ANY:value:float}", "0x10",                 "{\"value\":\"0x10\"}"},
		{"%{ANY:value:float}", "1e",                   "{\"value\":\"1e\"}"},
		// Числа, за пределы вида выходящие
		{"%{ANY:value:int}",   "18446744073709551616", "{\"value\":\"18446744073709551616\"}"},
		{"%{ANY:value:int}",   "-99999999999999999999", "{\"value\":\"-99999999999999999999\"}"},
		{"%{ANY:value:float}", "1e999",                "{\"value\":\"1e999\"}"}
	};
	/**
	 * Выполняем обход набора образцов вывода числа
	 */
	for(auto & sample : samples) {
		// Выполняем сборку шаблона Grok
		const auto exp = grok.build(sample.pattern);
		// Выполняем проверку сборки шаблона Grok
		ASSERT_TRUE(!!exp) << sample.text;
		// Запись JSON извлечённых полей
		string result;
		// Выполняем вывод извлечённых полей записью JSON
		ASSERT_TRUE(dumping(grok, sample.text, exp, result)) << sample.text;
		// Выполняем проверку записи JSON извлечённых полей
		EXPECT_EQ(result, sample.expected) << sample.text;
	}
}
/**
 * @brief Тест вывода извлечённых полей значением JSON
 *
 * @details Вывод значением - вид основной, а вывод записью - посредник над ним:
 *          значение владеет содержимым, переживает документ и годится
 *          к встраиванию в больший документ, тогда как запись есть лишь текст.
 *
 */
TEST(Grok, JsonValueOutput) {
	// Создаём объект разбора текста по шаблонам Grok
	grok_t grok(::logger());
	// Выполняем сборку шаблона Grok
	const auto exp = grok.build("%{WORD:name} %{INT:code:int}");
	// Выполняем проверку сборки шаблона Grok
	ASSERT_TRUE(!!exp);
	// Значение JSON извлечённых полей
	awh::grok::json_t value;
	// Выполняем вывод извлечённых полей значением JSON
	ASSERT_TRUE(grok.json("node 42", exp, value));
	// Выполняем проверку вида значения
	EXPECT_TRUE(value.is(awh::codec::json::type_t::OBJECT));
	// Выполняем проверку количества полей значения
	EXPECT_EQ(value.size(), 2);
	// Извлекаемое название поля
	string name;
	// Выполняем извлечение значения поля названия
	ASSERT_TRUE(value["name"].value(name));
	// Выполняем проверку значения поля названия
	EXPECT_EQ(name, "node");
	// Извлекаемый код поля
	int64_t code = 0;
	/**
	 * Выполняем извлечение значения поля кода
	 *
	 * @details Вид хранения извлечению не указ: число заносится узким видом,
	 *          а извлекается тем, какой запрошен.
	 *
	 */
	ASSERT_TRUE(value["code"].value(code));
	// Выполняем проверку значения поля кода
	EXPECT_EQ(code, 42);
	// Выполняем проверку вывода значения записью
	EXPECT_EQ(value.dump(), "{\"name\":\"node\",\"code\":42}");
	// Выполняем проверку отказа вывода при отсутствии совпадения
	EXPECT_FALSE(grok.json("совпадения здесь нет", exp, value));
}
/**
 * @brief Тест повторного названия поля в выводе JSON
 *
 * @details Объект JSON повторов ключа не несёт, поэтому повторное название
 *          выигрывает последним, а место в порядке вывода занимает первым.
 *          Способ, потерь не несущий, - извлечение набором значений.
 *
 */
TEST(Grok, JsonDuplicateNames) {
	// Создаём объект разбора текста по шаблонам Grok
	grok_t grok(::logger());
	// Выполняем сборку шаблона Grok
	const auto exp = grok.build("%{WORD:tag}-%{WORD:tag} %{WORD:name}");
	// Выполняем проверку сборки шаблона Grok
	ASSERT_TRUE(!!exp);
	// Запись JSON извлечённых полей
	string result;
	// Выполняем вывод извлечённых полей записью JSON
	ASSERT_TRUE(dumping(grok, "alpha-bravo node", exp, result));
	// Выполняем проверку выигрыша последнего значения
	EXPECT_EQ(result, "{\"tag\":\"bravo\",\"name\":\"node\"}");
	// Набор извлечённых значений полей
	vector <grok_t::value_t> values;
	// Выполняем извлечение значений полей из текста
	ASSERT_TRUE(grok.exec("alpha-bravo node", exp, values));
	// Выполняем проверку сохранения повтора извлечением набором значений
	ASSERT_EQ(values.size(), 3);
	EXPECT_EQ(values.at(0).value, "alpha");
	EXPECT_EQ(values.at(1).value, "bravo");
}
/**
 * @brief Тест наполнения реестра набором шаблонов из текста
 *
 */
TEST(Grok, ReadSet) {
	// Создаём объект разбора текста по шаблонам Grok
	Grok grok(::logger());
	/**
	 * @brief Текст набора шаблонов
	 *
	 * @details Набор несёт примечание, пустую строку, строку из одних
	 *          пробельных знаков, строку без текста шаблона и строку,
	 *          завершённую парой знаков перевода.
	 *
	 */
	const string set =
		"# набор шаблонов проверки\n"
		"\n"
		"   \t \n"
		"NGINXSTAMP  \\d{2}/\\w{3}/\\d{4}:\\d{2}:\\d{2}:\\d{2}\r\n"
		"\tNGINXCODE\t%{NUMBER:code:int}\n"
		"ШАБЛОН_БЕЗ_ТЕЛА\n"
		"NGINXLINE  %{IP:client} \\[%{NGINXSTAMP:stamp}\\] %{NGINXCODE}\n";
	// Выполняем наполнение реестра набором шаблонов
	ASSERT_EQ(grok.read(set), 3);
	// Выполняем проверку наличия шаблонов в реестре
	EXPECT_TRUE(grok.has("NGINXSTAMP"));
	EXPECT_TRUE(grok.has("NGINXCODE"));
	EXPECT_TRUE(grok.has("NGINXLINE"));
	// Выполняем проверку пропуска строки без текста шаблона
	EXPECT_FALSE(grok.has("ШАБЛОН_БЕЗ_ТЕЛА"));
	// Выполняем проверку отсечения возврата каретки в конце строки
	EXPECT_EQ(grok.pattern("NGINXSTAMP"), "\\d{2}/\\w{3}/\\d{4}:\\d{2}:\\d{2}:\\d{2}");
	// Выполняем проверку отсечения пробельных знаков вокруг названия
	EXPECT_EQ(grok.pattern("NGINXCODE"), "%{NUMBER:code:int}");
	// Выполняем сборку шаблона, ссылающегося на прочитанные
	const auto exp = grok.build("%{NGINXLINE}");
	// Выполняем проверку сборки шаблона
	ASSERT_TRUE(!!exp);
	// Извлечённые значения полей шаблона
	unordered_map <string, string> values;
	// Выполняем извлечение значений полей шаблона
	ASSERT_TRUE(grok.exec("192.168.5.150 [04/Aug/2026:12:30:00] 200", exp, values));
	// Выполняем проверку извлечённых значений полей шаблона
	EXPECT_EQ(values["client"], "192.168.5.150");
	EXPECT_EQ(values["stamp"], "04/Aug/2026:12:30:00");
	EXPECT_EQ(values["code"], "200");
	// Выполняем проверку замещения шаблона реестра прочитанным
	EXPECT_EQ(grok.read("NGINXCODE %{NUMBER:status:int}"), 1);
	// Выполняем проверку замещения текста шаблона реестра
	EXPECT_EQ(grok.pattern("NGINXCODE"), "%{NUMBER:status:int}");
	// Выполняем проверку пропуска набора, шаблонов не несущего
	EXPECT_EQ(grok.read("# одни лишь примечания\n\n#\n"), 0);
}
/**
 * @brief Тест записи и восстановления собранных шаблонов
 *
 */
TEST(Grok, StorageRoundtrip) {
	// Создаём объект разбора текста по шаблонам Grok
	Grok grok(::logger());
	/**
	 * @brief Набор текстов шаблонов записи
	 *
	 */
	const vector <string> patterns = {
		"%{IP:client}", "%{NUMBER:code:int}", "%{WORD:method} %{URIPATH:path}",
		"%{TIMESTAMP_ISO8601:stamp}", "%{EMAILADDRESS:mail}", "%{LOGLEVEL:level}",
		"%{IP:client} - %{USER:user} %{NUMBER:code:int} %{NUMBER:bytes:int}",
		"%{HOSTNAME:host}", "%{QUOTEDSTRING:text}", "%{BASE16NUM:hex}"
	};
	// Запись собранных шаблонов Grok
	string record;
	// Выполняем запись собранных шаблонов
	ASSERT_TRUE(grok.save(patterns, record)) << "код " << static_cast <uint32_t> (grok.error());
	// Выполняем проверку непустоты записи собранных шаблонов
	ASSERT_FALSE(record.empty());
	// Создаём объект разбора текста по шаблонам Grok
	Grok restored(::logger());
	// Набор восстановленных шаблонов Grok
	vector <Grok::exp_t> expressions;
	// Выполняем восстановление собранных шаблонов
	ASSERT_TRUE(restored.load(record, expressions)) << "код " << static_cast <uint32_t> (restored.error());
	// Выполняем проверку количества восстановленных шаблонов
	ASSERT_EQ(expressions.size(), patterns.size());
	/**
	 * @brief Набор текстов сличения поведения шаблонов
	 *
	 */
	const vector <string> texts = {
		"192.168.5.150", "forman@anyks.com", "2026-08-04T12:30:00Z", "GET /a/b",
		"-42.5e3", "ERROR", "0x1F2E", "anyks.com", "\"текст в кавычках\"",
		"192.168.5.150 - forman 200 4711", "", "слово"
	};
	/**
	 * Выполняем перебор набора восстановленных шаблонов
	 */
	for(size_t i = 0; i < patterns.size(); i++) {
		// Выполняем сборку шаблона начисто
		const auto fresh = grok.build(patterns.at(i));
		// Выполняем проверку сборки шаблона начисто
		ASSERT_TRUE(!!fresh) << patterns.at(i);
		// Выполняем сличение исходного текста шаблона
		EXPECT_EQ(expressions.at(i)->pattern, patterns.at(i));
		// Выполняем сличение развёрнутого текста регулярного выражения
		EXPECT_EQ(expressions.at(i)->expression, fresh->expression);
		// Выполняем сличение количества полей шаблона
		ASSERT_EQ(expressions.at(i)->fields.size(), fresh->fields.size());
		/**
		 * Выполняем перебор набора полей шаблона
		 */
		for(size_t j = 0; j < fresh->fields.size(); j++) {
			// Выполняем сличение номера группы захвата поля
			EXPECT_EQ(expressions.at(i)->fields.at(j).number, fresh->fields.at(j).number);
			// Выполняем сличение вида значения поля
			EXPECT_EQ(expressions.at(i)->fields.at(j).kind, fresh->fields.at(j).kind);
			// Выполняем сличение названия поля шаблона
			EXPECT_EQ(expressions.at(i)->fields.at(j).name, fresh->fields.at(j).name);
		}
		/**
		 * Выполняем перебор набора текстов сличения
		 */
		for(const auto & text : texts) {
			// Выполняем сличение соответствия текста шаблону
			EXPECT_EQ(grok.test(text, fresh), restored.test(text, expressions.at(i)))
			 << "шаблон \"" << patterns.at(i) << "\" на тексте \"" << text << "\"";
			// Выполняем сличение вывода значений полей в JSON
			string first, second;
			dumping(grok, text, fresh, first);
			dumping(restored, text, expressions.at(i), second);
			// Выполняем сличение выведенных значений полей
			EXPECT_EQ(first, second) << "шаблон \"" << patterns.at(i) << "\" на тексте \"" << text << "\"";
		}
	}
	/**
	 * Выполняем проверку размещения восстановленных шаблонов в кэше
	 *
	 * @details Восстановленный шаблон обязан выдаваться сборкой без разворота
	 *          ссылок и компиляции, пока его удерживает набор восстановленных.
	 *
	 */
	EXPECT_EQ(restored.build(patterns.front()).get(), expressions.front().get());
}
/**
 * @brief Тест отказов записи и восстановления собранных шаблонов
 *
 */
TEST(Grok, StorageErrors) {
	// Создаём объект разбора текста по шаблонам Grok
	Grok grok(::logger());
	// Запись собранных шаблонов Grok
	string record;
	// Набор восстановленных шаблонов Grok
	vector <Grok::exp_t> expressions;
	// Выполняем проверку отказа записи пустого набора шаблонов
	EXPECT_FALSE(grok.save({}, record));
	EXPECT_EQ(grok.error(), Grok::error_t::STORAGE_EMPTY);
	// Выполняем проверку отказа записи шаблона со ссылкой неизвестной
	EXPECT_FALSE(grok.save({"%{НЕТ_ТАКОГО:поле}"}, record));
	EXPECT_EQ(grok.error(), Grok::error_t::REFERENCE_UNKNOWN);
	// Выполняем проверку отказа восстановления пустой записи
	EXPECT_FALSE(grok.load("", expressions));
	EXPECT_EQ(grok.error(), Grok::error_t::STORAGE_EMPTY);
	// Выполняем запись собранных шаблонов
	ASSERT_TRUE(grok.save({"%{IP:client}", "%{NUMBER:code:int}"}, record));
	/**
	 * Выполняем проверку отказа восстановления записи с чужим опознанием
	 */
	{
		// Получаем запись с подменённым опознанием
		string foreign = record;
		// Выполняем подмену опознания записи
		foreign[0] = static_cast <char> (foreign[0] ^ 0x5A);
		// Выполняем проверку отказа восстановления записи
		EXPECT_FALSE(grok.load(foreign, expressions));
		EXPECT_EQ(grok.error(), Grok::error_t::STORAGE_MAGIC);
	}
	/**
	 * Выполняем проверку отказа восстановления записи версии иной
	 */
	{
		// Получаем запись с подменённой версией устройства
		string aged = record;
		// Выполняем подмену версии устройства записи
		aged[8] = static_cast <char> (0x5A);
		// Выполняем проверку отказа восстановления записи
		EXPECT_FALSE(grok.load(aged, expressions));
		EXPECT_EQ(grok.error(), Grok::error_t::STORAGE_VERSION);
	}
	/**
	 * Выполняем проверку отказа восстановления записи оборванной
	 *
	 * @details Обрыв проверяется на всякой длине: отказ обязан быть при всяком,
	 *          а падения быть не обязано ни при каком.
	 *
	 */
	for(size_t i = 0; i < record.size(); i++) {
		// Получаем оборванную запись собранных шаблонов
		const string cut = record.substr(0, i);
		// Выполняем проверку отказа восстановления записи
		EXPECT_FALSE(grok.load(cut, expressions)) << "оборвано на " << i;
		// Выполняем проверку установки кода ошибки разбора
		EXPECT_NE(grok.error(), Grok::error_t::NONE);
	}
	/**
	 * Выполняем проверку отказа восстановления записи порченой
	 *
	 * @details Порча части записи, шаблоны описывающей, принималась молча
	 *          и выдавала шаблоны искажённые, пока контрольной суммы у части
	 *          этой не было: испытание закрепляет, что всякая порча её
	 *          обращается отказом, а не искажением.
	 *
	 */
	{
		// Количество принятых записей и записей, искажение выдавших
		size_t taken = 0, distorted = 0;
		// Набор шаблонов, записью нетронутой восстановленных
		vector <Grok::exp_t> sample;
		// Выполняем восстановление записи нетронутой
		ASSERT_TRUE(grok.load(record, sample));
		/**
		 * Выполняем перебор байтов начала записи
		 *
		 * @details Часть, шаблоны описывающая, стоит первой, а запись хранилища
		 *          к ней приложена и порчу свою отвергает сама, отчего проверке
		 *          довольно начала записи: порча далее отвергается хранилищем.
		 *
		 */
		for(size_t i = 0; (i < record.size()) && (i < 512); i++) {
			/**
			 * Выполняем перебор подмен значения очередного байта
			 */
			for(uint32_t value = 0; value < 256; value += 37) {
				/**
				 * Если подмена значения байта его не меняет
				 */
				if(static_cast <char> (value) == record[i])
					// Переходим к следующей подмене значения байта
					continue;
				// Получаем порченую запись собранных шаблонов
				string bad = record;
				// Выполняем подмену значения очередного байта записи
				bad[i] = static_cast <char> (value);
				// Набор шаблонов, записью порченой восстановленных
				vector <Grok::exp_t> broken;
				/**
				 * Если запись порченая восстановлению не поддалась
				 */
				if(!grok.load(bad, broken))
					// Переходим к следующей подмене значения байта
					continue;
				// Увеличиваем количество принятых записей
				taken++;
				// Признак совпадения восстановленного набора с образцом
				bool same = (broken.size() == sample.size());
				/**
				 * Выполняем сличение восстановленного набора с образцом
				 */
				for(size_t j = 0; same && (j < broken.size()); j++)
					// Выполняем сличение развёрнутых текстов шаблонов
					same = (broken.at(j)->pattern == sample.at(j)->pattern) &&
					 (broken.at(j)->expression == sample.at(j)->expression) &&
					 (broken.at(j)->fields.size() == sample.at(j)->fields.size());
				/**
				 * Если восстановленный набор с образцом расходится
				 */
				if(!same)
					// Увеличиваем количество записей, искажение выдавших
					distorted++;
			}
		}
		// Выполняем проверку отсутствия молча принятых искажений
		EXPECT_EQ(distorted, static_cast <size_t> (0)) << "принято записей " << taken;
	}
	// Выполняем проверку восстановления записи нетронутой
	EXPECT_TRUE(grok.load(record, expressions));
}
/**
 * @brief Функция обращения последовательности байтов
 *
 * @param source исходная последовательность байтов
 * @param result выводимая последовательность байтов
 * @return       результат обращения последовательности
 *
 * @details Обращение служит образцом обработчика сжатия: настоящие методы
 *          берутся из модуля «compressor», но тесты Grok от него намеренно
 *          не зависят - проверять надлежит устройство крючков.
 *
 */
static bool reversed(string_view source, string & result) noexcept {
	// Выполняем размещение выводимой последовательности байтов
	result.assign(source.rbegin(), source.rend());
	// Выводим результат обращения последовательности
	return true;
}
/**
 * @brief Тест записи собранных шаблонов со сжатием
 *
 */
TEST(Grok, StoragePacking) {
	// Создаём объект разбора текста по шаблонам Grok
	Grok grok(::logger());
	/**
	 * @brief Набор текстов шаблонов записи
	 *
	 */
	const vector <string> patterns = {
		"%{IP:client}", "%{NUMBER:code:int}", "%{TIMESTAMP_ISO8601:stamp}", "%{LOGLEVEL:level}"
	};
	// Запись собранных шаблонов без сжатия
	string plain;
	// Выполняем запись собранных шаблонов без сжатия
	ASSERT_TRUE(grok.save(patterns, plain));
	// Выполняем установку сжатия записи собранных шаблонов
	grok.packer(compressor::method_t::LZ4, &reversed, &reversed);
	// Запись собранных шаблонов со сжатием
	string packed;
	// Выполняем запись собранных шаблонов со сжатием
	ASSERT_TRUE(grok.save(patterns, packed)) << "код " << static_cast <uint32_t> (grok.error());
	/**
	 * Выполняем проверку совпадения размеров записей
	 *
	 * @details Обращение последовательности размера её не меняет, отчего
	 *          записи расходятся лишь размером поля, несущего длину сжатого
	 *          содержимого: без сжатия поле это несёт ноль.
	 *
	 */
	ASSERT_GE(packed.size(), plain.size());
	ASSERT_LE(packed.size(), plain.size() + 16);
	// Выполняем проверку расхождения содержимого записей
	EXPECT_NE(packed, plain);
	// Создаём объект разбора текста по шаблонам Grok
	Grok restored(::logger());
	// Набор восстановленных шаблонов Grok
	vector <Grok::exp_t> expressions;
	// Выполняем проверку отказа восстановления без обработчика разжатия
	EXPECT_FALSE(restored.load(packed, expressions));
	EXPECT_EQ(restored.error(), Grok::error_t::STORAGE_METHOD);
	// Выполняем установку сжатия записи собранных шаблонов
	restored.packer(compressor::method_t::LZ4, &reversed, &reversed);
	// Выполняем восстановление собранных шаблонов
	ASSERT_TRUE(restored.load(packed, expressions)) << "код " << static_cast <uint32_t> (restored.error());
	// Выполняем проверку количества восстановленных шаблонов
	ASSERT_EQ(expressions.size(), patterns.size());
	// Выполняем проверку сопоставления восстановленных шаблонов
	EXPECT_TRUE(restored.test("192.168.5.150", expressions.at(0)));
	EXPECT_TRUE(restored.test("4711", expressions.at(1)));
	EXPECT_TRUE(restored.test("2026-08-04T12:30:00Z", expressions.at(2)));
	EXPECT_TRUE(restored.test("ERROR", expressions.at(3)));
}
/**
 * @brief Тест записи и восстановления всего встроенного набора шаблонов
 *
 */
TEST(Grok, StorageBuiltin) {
	// Создаём объект разбора текста по шаблонам Grok
	Grok grok(::logger());
	// Набор текстов шаблонов записи
	vector <string> patterns;
	/**
	 * Выполняем перебор названий шаблонов реестра
	 */
	for(const auto & name : grok.patterns())
		// Выполняем добавление ссылки на шаблон в набор
		patterns.push_back("%{" + name + "}");
	// Выполняем проверку непустоты набора текстов шаблонов
	ASSERT_FALSE(patterns.empty());
	// Запись собранных шаблонов Grok
	string record;
	// Выполняем запись собранных шаблонов
	ASSERT_TRUE(grok.save(patterns, record)) << "код " << static_cast <uint32_t> (grok.error());
	// Создаём объект разбора текста по шаблонам Grok
	Grok restored(::logger());
	// Набор восстановленных шаблонов Grok
	vector <Grok::exp_t> expressions;
	// Выполняем восстановление собранных шаблонов
	ASSERT_TRUE(restored.load(record, expressions)) << "код " << static_cast <uint32_t> (restored.error());
	// Выполняем проверку количества восстановленных шаблонов
	ASSERT_EQ(expressions.size(), patterns.size());
	/**
	 * @brief Набор текстов сличения поведения шаблонов
	 *
	 */
	const vector <string> texts = {
		"192.168.5.150", "forman@anyks.com", "2026-08-04T12:30:00Z", "GET /a/b HTTP/1.1",
		"-42.5e3", "слово", "ERROR", "anyks.com", "0x1F2E3D4C"
	};
	/**
	 * Выполняем перебор набора восстановленных шаблонов
	 */
	for(size_t i = 0; i < patterns.size(); i++) {
		// Выполняем сборку шаблона начисто
		const auto fresh = grok.build(patterns.at(i));
		/**
		 * Если сборка шаблона начисто не выполнена
		 */
		if(!fresh)
			// Переходим к следующему шаблону набора
			continue;
		/**
		 * Выполняем перебор набора текстов сличения
		 */
		for(const auto & text : texts)
			// Выполняем сличение соответствия текста шаблону
			EXPECT_EQ(grok.test(text, fresh), restored.test(text, expressions.at(i)))
			 << "шаблон \"" << patterns.at(i) << "\" на тексте \"" << text << "\"";
	}
}

/**
 * @brief Тест пограничного употребления надстройки
 *
 * @details Отказы при употреблении неверном - пустое название, пустое тело,
 *          выражение несобранное - проверками не достигались вовсе, отчего
 *          снятие любого из них проходило незамеченным.
 *
 */
TEST(Grok, Refusal) {
	// Создаём объект надстройки шаблонов
	grok_t grok(::logger());
	// Выполняем проверку отказа поиска шаблона с пустым названием
	EXPECT_FALSE(grok.has(""));
	// Выполняем проверку отказа удаления шаблона с пустым названием
	EXPECT_FALSE(grok.erase(""));
	/**
	 * Выполняем проверку отказа добавления шаблона с пустым названием
	 */
	{
		// Выполняем добавление шаблона с пустым названием
		EXPECT_FALSE(grok.pattern("", "\\d+"));
		// Выполняем проверку установки кода ошибки пустого названия
		EXPECT_EQ(grok.error(), grok::error_t::NAME_EMPTY);
	}
	/**
	 * Выполняем проверку отказа добавления шаблона с пустым телом
	 */
	{
		// Выполняем добавление шаблона с пустым телом
		EXPECT_FALSE(grok.pattern("EMPTY", ""));
		// Выполняем проверку установки кода ошибки пустого тела
		EXPECT_EQ(grok.error(), grok::error_t::PATTERN_EMPTY);
	}
	/**
	 * Выполняем проверку отказа сопоставления выражением несобранным
	 *
	 * @details Выражение несобранное потребитель получает при отказе сборки,
	 *          и сопоставление им обязано отвечать отказом, а не обращением
	 *          к указателю пустому.
	 *
	 */
	{
		// Создаём несобранное выражение надстройки
		grok_t::exp_t none;
		// Набор извлечённых значений полей
		unordered_map <string, string> values;
		// Выполняем проверку отказа проверки соответствия текста
		EXPECT_FALSE(grok.test("abc", none));
		// Выполняем проверку отказа извлечения значений полей
		EXPECT_FALSE(grok.exec("abc", none, values));
		// Выполняем проверку отсутствия установленных границ
		EXPECT_TRUE(grok.match("abc", none).empty());
	}
	/**
	 * Выполняем проверку предела глубины разворота ссылок
	 *
	 * @details Ссылки шаблонов разворачиваются одна в другую, и глубина
	 *          разворота ограничена. Граница проверена опытом: шестьдесят
	 *          три уровня разворачиваются, шестьдесят четвёртый отвергается.
	 *
	 */
	{
		/**
		 * Выполняем сборку цепочки шаблонов, друг на друга ссылающихся
		 */
		for(uint32_t i = 0; i < 70; i++)
			// Выполняем добавление очередного шаблона цепочки
			ASSERT_TRUE(grok.pattern("N" + std::to_string(i),
			 (i == 0 ? string("\\d+") : ("%{N" + std::to_string(i - 1) + "}"))));
		// Выполняем сборку выражения глубины предельной
		EXPECT_TRUE(!!grok.build("%{N62}"));
		// Выполняем сборку выражения глубины сверх предельной
		EXPECT_FALSE(!!grok.build("%{N63}"));
		// Выполняем проверку установки кода ошибки превышения глубины
		EXPECT_EQ(grok.error(), grok::error_t::NESTING_TOO_DEEP);
	}
}

/**
 * @brief Тест отказа восстановления записи, оборванной с подделкой заголовка
 *
 * @details Запись собранных шаблонов несёт заголовок, часть с описанием
 *          шаблонов и приложенную к ней запись хранилища выражений. Обрыв
 *          простой отвергается заголовком - объявленный размер части её длине
 *          не отвечает, - и до заслонов разбора дело не доходит вовсе. Потому
 *          испытание подделывает заголовок под обрывок: переписывает
 *          объявленный размер и пересчитывает контрольную сумму.
 *
 */
TEST(Grok, StorageTruncatedForged) {
	/**
	 * @brief Функция вычисления контрольной суммы содержимого
	 *
	 * @param data содержимое, сумма какого вычисляется
	 * @return     вычисленная контрольная сумма содержимого
	 *
	 */
	auto checksum = [](const string_view data) noexcept -> uint64_t {
		// Накопленная контрольная сумма содержимого
		uint64_t result = 0xCBF29CE484222325ull;
		// Получаем позицию чтения содержимого
		size_t offset = 0;
		/**
		 * Выполняем перебор содержимого восьмибайтовыми долями
		 */
		for(; (offset + 8) <= data.size(); offset += 8) {
			// Собираемая доля содержимого
			uint64_t block = 0;
			/**
			 * Выполняем сборку доли содержимого байтами от младшего
			 */
			for(uint8_t shift = 0; shift < 64; shift += 8)
				// Выполняем добавление очередного байта доли
				block |= (static_cast <uint64_t> (static_cast <uint8_t> (data[offset + (shift >> 3)])) << shift);
			// Выполняем смешивание доли содержимого
			result ^= block;
			// Выполняем умножение накопленной суммы
			result *= 0x100000001B3ull;
			// Выполняем перемешивание накопленной суммы
			result ^= (result >> 29);
		}
		/**
		 * Выполняем перебор остатка содержимого побайтно
		 */
		for(; offset < data.size(); offset++) {
			// Выполняем смешивание очередного байта содержимого
			result ^= static_cast <uint64_t> (static_cast <uint8_t> (data[offset]));
			// Выполняем умножение накопленной суммы
			result *= 0x100000001B3ull;
		}
		// Выводим вычисленную контрольную сумму содержимого
		return result;
	};
	/**
	 * @brief Функция записи числа переменной длины
	 *
	 * @param value  записываемое число
	 * @param result запись, в какую ведётся запись числа
	 *
	 */
	auto writeNumber = [](uint64_t value, string & result) noexcept -> void {
		/**
		 * Выполняем запись числа долями по семь разрядов
		 */
		while(value >= 0x80) {
			// Выполняем запись очередной доли числа с признаком продолжения
			result.push_back(static_cast <char> ((value & 0x7F) | 0x80));
			// Переходим к следующей доле числа
			value >>= 7;
		}
		// Выполняем запись последней доли числа
		result.push_back(static_cast <char> (value & 0x7F));
	};
	/**
	 * @brief Функция чтения числа переменной длины
	 *
	 * @param data   читаемая запись
	 * @param offset позиция чтения записи
	 * @return       прочитанное число
	 *
	 */
	auto readNumber = [](const string_view data, size_t & offset) noexcept -> uint64_t {
		// Прочитанное число переменной длины
		uint64_t result = 0;
		/**
		 * Выполняем чтение числа долями по семь разрядов
		 */
		for(uint8_t shift = 0; shift < 64; shift += 7) {
			// Получаем очередной байт числа
			const uint8_t letter = static_cast <uint8_t> (data[offset++]);
			// Выполняем добавление доли числа
			result |= (static_cast <uint64_t> (letter & 0x7F) << shift);
			/**
			 * Если байт продолжения не отмечает
			 */
			if((letter & 0x80) == 0)
				// Прекращаем чтение числа переменной длины
				break;
		}
		// Выводим прочитанное число
		return result;
	};
	// Создаём объект надстройки шаблонов
	grok_t grok(::logger());
	/**
	 * @brief Набор шаблонов, устройством записи различающихся
	 *
	 */
	const char * patterns[] = {
		"%{INT:code}", "%{WORD:name} %{NUMBER:size:float}",
		"^%{IP:host} - %{WORD:user}$", "%{DATA:body}"
	};
	// Набор текстов собираемых шаблонов
	vector <string> texts;
	/**
	 * Выполняем сборку набора текстов шаблонов
	 */
	for(const char * pattern : patterns)
		// Выполняем добавление текста шаблона в набор
		texts.push_back(pattern);
	// Запись собранных шаблонов
	string record;
	// Выполняем запись собранных шаблонов
	ASSERT_TRUE(grok.save(texts, record)) << "код " << static_cast <uint32_t> (grok.error());
	// Получаем позицию разбора заголовка записи
	size_t offset = 8;
	// Получаем версию устройства записи и метод сжатия части её
	const char version = record[offset++], method = record[offset++];
	// Получаем размер содержимого части записи до сжатия
	const uint64_t origin = readNumber(record, offset);
	// Выполняем пропуск размера сжатого содержимого части записи
	readNumber(record, offset);
	// Выполняем пропуск контрольной суммы содержимого части записи
	offset += 8;
	// Выполняем проверку вмещения записью содержимого объявленного
	ASSERT_GT(record.size(), (offset + static_cast <size_t> (origin)));
	// Получаем обзор содержимого части записи, шаблоны описывающей
	const string content = record.substr(offset, static_cast <size_t> (origin));
	// Получаем запись хранилища собранных выражений, к части приложенную
	const string storage = record.substr(offset + static_cast <size_t> (origin));
	// Количество записей, отвергнутых обрывом содержимого
	size_t refused = 0;
	/**
	 * Выполняем обход длин обрыва содержимого части записи
	 */
	for(size_t length = 0; length < content.size(); length++) {
		// Собираемая подделанная запись собранных шаблонов
		string forged = record.substr(0, 8);
		// Выполняем запись версии устройства записи и метода сжатия
		forged.push_back(version);
		forged.push_back(method);
		// Выполняем запись размера оборванного содержимого части записи
		writeNumber(static_cast <uint64_t> (length), forged);
		// Выполняем запись размера сжатого содержимого части записи
		writeNumber(0, forged);
		// Получаем контрольную сумму оборванного содержимого части записи
		const uint64_t control = checksum(string_view(content).substr(0, length));
		/**
		 * Выполняем запись контрольной суммы байтами от младшего
		 */
		for(uint8_t shift = 0; shift < 64; shift += 8)
			// Выполняем запись очередного байта контрольной суммы
			forged.push_back(static_cast <char> ((control >> shift) & 0xFF));
		// Выполняем запись оборванного содержимого части записи
		forged.append(content, 0, length);
		// Выполняем запись хранилища собранных выражений
		forged.append(storage);
		// Набор шаблонов, записью подделанной восстановленных
		vector <grok_t::exp_t> restored;
		// Выполняем проверку отказа восстановления оборванной записи
		EXPECT_FALSE(grok.load(forged, restored)) << "обрывок длиной " << length;
		// Выполняем проверку установки кода ошибки надстройки
		EXPECT_NE(grok.error(), grok::error_t::NONE) << "обрывок длиной " << length;
		/**
		 * Если запись отвергнута обрывом содержимого
		 */
		if(grok.error() == grok::error_t::STORAGE_TRUNCATED)
			// Увеличиваем количество отвергнутых записей
			refused++;
	}
	/**
	 * Выполняем проверку отвержения записей оборванных
	 *
	 * @details Проверка утверждает не только отказ, но и повод его: без
	 *          заслонов обрыва в разборе части отказы эти обращались бы
	 *          в иные поводы либо в падение.
	 *
	 */
	EXPECT_GT(refused, static_cast <size_t> (10));
}
