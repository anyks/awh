/**
 * @file value.cpp
 * @date 2026-08-17
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Проверки владеющего значения YAML — сборка дерева из значений языка, обход,
 *        извлечение, мост от дерева документа и потоковая сборка
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cmath>
#include <limits>
#include <string>
#include <vector>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <gtest/gtest.h>
#include <codec/yaml/yaml.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;
using namespace awh::codec;

/**
 * @brief Проверка сборки дерева из значений языка
 *
 * @details Проверка эта закрепляет главное, ради чего владеющее значение и заведено:
 *          дерево собирается из ничего, тогда как дерево документа заводится лишь
 *          разбором текста
 *
 */
TEST(CodecYamlValue, Assembling) {
	// Собираемое значение
	yaml::value_t value;
	// Выполняем проверку неопределённости пустого значения
	ASSERT_FALSE(value.valid());
	// Выполняем проверку вида пустого значения
	ASSERT_EQ(value.kind(), yaml::kind_t::NONE);
	// Выполняем занесение строкового поля
	value["name"] = yaml::value_t("сервер");
	// Выполняем занесение числового поля
	value["port"] = yaml::value_t(static_cast <int64_t> (8080));
	// Выполняем проверку вида собранного значения
	ASSERT_EQ(value.kind(), yaml::kind_t::MAPPING);
	// Выполняем проверку количества полей собранного значения
	ASSERT_EQ(value.size(), 2u);
	// Выполняем проверку порядка занесения полей
	ASSERT_EQ(value.key(0), "name");
	// Выполняем проверку порядка занесения полей
	ASSERT_EQ(value.key(1), "port");
	// Извлекаемое числовое значение
	int64_t port = 0;
	// Выполняем проверку успешности извлечения числа
	ASSERT_TRUE(value["port"].value(port));
	// Выполняем проверку извлечённого числа
	ASSERT_EQ(port, 8080);
	// Выполняем проверку наличия занесённого поля
	ASSERT_TRUE(value.contains("name"));
	// Выполняем проверку отсутствия незанесённого поля
	ASSERT_FALSE(value.contains("host"));
}
/**
 * @brief Проверка передачи значения наружу итогом
 *
 * @details Ссылка на узел дерева документ пережить не может, а владеющее значение -
 *          обязано: проверка эта закрепляет ровно ту возможность, какой у ссылки нет
 *
 */
TEST(CodecYamlValue, Outliving) {
	/**
	 * @brief Функция сборки значения, наружу выдаваемого
	 *
	 * @return собранное значение
	 *
	 */
	const auto produce = []() noexcept -> yaml::value_t {
		// Дерево документа, живущее лишь внутри вызова
		yaml::document_t doc;
		// Выполняем разбор текста в дерево документа
		doc.parse("a: 1\nb:\n- x\n- y\n");
		// Выводим значение, с дерева документа снятое
		return yaml::value_t(doc.root());
	};
	// Получаем значение, вызов переживающее
	const yaml::value_t value = produce();
	// Выполняем проверку вида пережившего значения
	ASSERT_EQ(value.kind(), yaml::kind_t::MAPPING);
	// Выполняем проверку количества полей пережившего значения
	ASSERT_EQ(value.size(), 2u);
	// Выполняем проверку записи вложенного значения
	ASSERT_EQ(value["b"][1].text(), "y");
	// Перечень значений, наружу выданных
	vector <yaml::value_t> list;
	// Выполняем занесение значения в перечень
	list.push_back(value);
	// Выполняем занесение значения в перечень
	list.push_back(produce());
	// Выполняем проверку совпадения занесённых значений
	ASSERT_TRUE(list.front() == list.back());
}
/**
 * @brief Проверка обхода значения путём
 *
 */
TEST(CodecYamlValue, Routing) {
	// Собираемое значение
	yaml::value_t value;
	// Выполняем заведение значения по пути
	value.place("/server/hosts/0") = yaml::value_t("alpha");
	// Выполняем заведение значения по пути
	value.place("/server/hosts/1") = yaml::value_t("beta");
	// Выполняем заведение значения по пути
	value.place("/server/port") = yaml::value_t(static_cast <int64_t> (443));
	// Выполняем проверку вида заведённого вместилища
	ASSERT_EQ(value["server"]["hosts"].kind(), yaml::kind_t::SEQUENCE);
	// Выполняем проверку количества значений заведённого перечня
	ASSERT_EQ(value["server"]["hosts"].size(), 2u);
	// Выполняем проверку обхода путём
	ASSERT_EQ(value.at("/server/hosts/1").text(), "beta");
	// Выполняем проверку обхода путём без ведущего разделителя
	ASSERT_EQ(value.at("server/port").text(), "443");
	// Выполняем проверку обхода пути отсутствующего
	ASSERT_FALSE(value.at("/server/missing").valid());
	// Выполняем проверку того, что обход отсутствующего пути ничего не завёл
	ASSERT_FALSE(value["server"].contains("missing"));
}
/**
 * @brief Проверка перезаписи значения в текст и обратного чтения его
 *
 */
TEST(CodecYamlValue, Rewriting) {
	// Собираемое значение
	yaml::value_t value;
	// Выполняем занесение строкового поля
	value["name"] = yaml::value_t("сервер");
	// Выполняем занесение дробного поля
	value["ratio"] = yaml::value_t(static_cast <double> (0.25));
	// Выполняем занесение логического поля
	value["ready"] = yaml::value_t(true);
	// Заводимый перечень значений
	yaml::value_t list(yaml::kind_t::SEQUENCE);
	// Выполняем занесение значения в перечень
	ASSERT_TRUE(list.push(yaml::value_t(static_cast <int64_t> (1))));
	// Выполняем занесение значения в перечень
	ASSERT_TRUE(list.push(yaml::value_t(static_cast <int64_t> (2))));
	// Выполняем занесение перечня полем значения
	ASSERT_TRUE(value.insert("list", list));
	// Прочитанное обратно значение
	yaml::value_t parsed;
	// Выполняем обратное чтение записанного текста
	ASSERT_TRUE(parsed.parse(value.dump()));
	// Выполняем проверку совпадения записанного и прочитанного
	ASSERT_TRUE(parsed == value);
	// Выполняем проверку сохранения записи дробного числа
	ASSERT_EQ(parsed["ratio"].text(), "0.25");
}
/**
 * @brief Проверка перерождения простого значения вместилищем
 *
 * @details Обращение изменяемое есть заявление о том, что здесь стоит вместилище:
 *          спорить с ним нечем, и простое содержимое при том теряется
 *
 */
TEST(CodecYamlValue, Reborning) {
	// Собираемое значение
	yaml::value_t value(string("простое"));
	// Выполняем проверку вида заведённого значения
	ASSERT_EQ(value.kind(), yaml::kind_t::STRING);
	// Выполняем занесение поля отображения
	value["field"] = yaml::value_t(static_cast <int64_t> (1));
	// Выполняем проверку перерождения значения отображением
	ASSERT_EQ(value.kind(), yaml::kind_t::MAPPING);
	// Выполняем проверку потери прежнего содержимого
	ASSERT_TRUE(value.text().empty());
	// Выполняем проверку количества полей перерождённого значения
	ASSERT_EQ(value.size(), 1u);
	// Собираемый перечень значений
	yaml::value_t list(yaml::kind_t::SEQUENCE);
	// Выполняем проверку отказа занесения поля отображения в перечень
	ASSERT_FALSE(list.insert("field", yaml::value_t(true)));
	// Простое значение, добавление в какое отвергается
	yaml::value_t plain(static_cast <int64_t> (5));
	// Выполняем проверку отказа добавления значения в простое значение
	ASSERT_FALSE(plain.push(yaml::value_t(true)));
}
/**
 * @brief Проверка снятия значений и правил их сличения
 *
 */
TEST(CodecYamlValue, Extraction) {
	// Прочитанное значение
	yaml::value_t value;
	// Выполняем разбор текста во владеющее значение
	ASSERT_TRUE(value.parse("hex: 0x1F\nreal: 2.5\nflag: true\nnul: null\nname: строка\n"));
	// Извлекаемое целое число
	int64_t integer = 0;
	// Выполняем проверку успешности извлечения числа записью шестнадцатеричной
	ASSERT_TRUE(value["hex"].value(integer));
	// Выполняем проверку извлечённого числа
	ASSERT_EQ(integer, 31);
	// Извлекаемая запись значения
	string text;
	// Выполняем проверку успешности извлечения записи
	ASSERT_TRUE(value["hex"].value(text));
	// Выполняем проверку того, что выдаётся запись, а не число
	ASSERT_EQ(text, "0x1F");
	// Извлекаемое дробное число
	double real = 0.;
	// Выполняем проверку успешности извлечения дробного числа
	ASSERT_TRUE(value["real"].value(real));
	// Выполняем проверку извлечённого дробного числа
	ASSERT_DOUBLE_EQ(real, 2.5);
	// Извлекаемое логическое значение
	bool flag = false;
	// Выполняем проверку успешности извлечения логического значения
	ASSERT_TRUE(value["flag"].value(flag));
	// Выполняем проверку извлечённого логического значения
	ASSERT_TRUE(flag);
	// Выполняем проверку отказа извлечения числа из строки
	ASSERT_FALSE(value["name"].value(integer));
	// Выполняем проверку отказа извлечения записи вместилища
	ASSERT_FALSE(value.value(text));
	// Выполняем проверку вида пустого значения
	ASSERT_EQ(value["nul"].kind(), yaml::kind_t::NUL);
	// Выполняем проверку того, что пустота значением определённым является
	ASSERT_TRUE(value["nul"].valid());
	// Выполняем проверку того, что отсутствующее поле значением не является
	ASSERT_FALSE(value["missing"].valid());
}
/**
 * @brief Проверка правил сличения значений
 *
 * @details Сличается суть значения, а не запись его: оформление, якорь и метка
 *          сличению не подлежат, а `0x1F` и `31` суть одно число
 *
 */
TEST(CodecYamlValue, Comparison) {
	// Значение, записью шестнадцатеричной данное
	yaml::value_t hex;
	// Выполняем разбор текста во владеющее значение
	ASSERT_TRUE(hex.parse("value: 0x1F\n"));
	// Значение, записью десятичной данное
	yaml::value_t dec;
	// Выполняем разбор текста во владеющее значение
	ASSERT_TRUE(dec.parse("value: 31\n"));
	// Выполняем проверку совпадения чисел, записанных по-разному
	ASSERT_TRUE(hex == dec);
	// Значение, оградою обнесённое
	yaml::value_t quoted(string("текст"), yaml::style_t::DOUBLE);
	// Значение, оградою не обнесённое
	yaml::value_t plain(string("текст"), yaml::style_t::PLAIN);
	// Выполняем проверку совпадения значений, записанных разною оградою
	ASSERT_TRUE(quoted == plain);
	// Пустота, записью описания данная
	yaml::value_t worded;
	// Выполняем разбор текста во владеющее значение
	ASSERT_TRUE(worded.parse("value: null\n"));
	// Пустота, заведённая видом своим
	yaml::value_t empty;
	// Выполняем занесение пустоты полем значения
	ASSERT_TRUE(empty.insert("value", yaml::value_t(yaml::kind_t::NUL)));
	// Выполняем проверку совпадения пустот, записанных по-разному
	ASSERT_TRUE(worded == empty);
	// Значение целое, с дробным сличаемое
	yaml::value_t integer(static_cast <int64_t> (1));
	// Значение дробное, с целым сличаемое
	yaml::value_t real(static_cast <double> (1.));
	// Выполняем проверку несовпадения числа целого с дробным
	ASSERT_TRUE(integer != real);
}
/**
 * @brief Проверка удержания частностей наречия YAML
 *
 * @details Обёртка общий облик с прочими кодеками делит, а своеобразия наречия не
 *          теряет: оформление записи, построение вместилища, якорь и метка живут
 *          значением наравне с содержимым
 *
 */
TEST(CodecYamlValue, Particulars) {
	// Собираемое значение
	yaml::value_t value;
	// Заводимый перечень значений
	yaml::value_t list(yaml::kind_t::SEQUENCE);
	// Выполняем установку построения перечня скобками
	list.layout(yaml::layout_t::FLOW);
	// Выполняем занесение значения в перечень
	ASSERT_TRUE(list.push(yaml::value_t("alpha")));
	// Выполняем занесение значения в перечень
	ASSERT_TRUE(list.push(yaml::value_t("beta")));
	// Выполняем занесение перечня полем значения
	ASSERT_TRUE(value.insert("list", list));
	// Получаем записанный текст значения
	const string text = value.dump();
	// Выполняем проверку записи перечня скобками
	ASSERT_NE(text.find("[alpha, beta]"), string::npos);
	// Значение блочное, переводы строк несущее
	yaml::value_t block(string("одна\nдве\n"), yaml::style_t::LITERAL);
	// Выполняем установку правила усечения переводов строк
	block.chomp(yaml::chomp_t::KEEP);
	// Собираемое значение блочное
	yaml::value_t owner;
	// Выполняем занесение блочного значения полем
	ASSERT_TRUE(owner.insert("text", block));
	// Прочитанное обратно значение
	yaml::value_t parsed;
	// Выполняем обратное чтение записанного текста
	ASSERT_TRUE(parsed.parse(owner.dump()));
	// Выполняем проверку сохранения содержимого блочного значения
	ASSERT_EQ(parsed["text"].text(), "одна\nдве\n");
	// Значение, якорь и метку несущее
	yaml::value_t marked(string("значение"));
	// Выполняем установку якоря значения
	marked.anchor("метка");
	// Выполняем проверку установленного якоря
	ASSERT_EQ(marked.anchor(), "метка");
	// Выполняем установку метки значения
	marked.tag("!!str");
	// Выполняем проверку установленной метки
	ASSERT_EQ(marked.tag(), "!!str");
	// Собираемое значение с якорем
	yaml::value_t anchored;
	// Выполняем занесение помеченного значения полем
	ASSERT_TRUE(anchored.insert("field", marked));
	// Получаем записанный текст значения с якорем
	const string record = anchored.dump();
	// Выполняем проверку записи якоря значения
	ASSERT_NE(record.find("&метка"), string::npos);
	// Выполняем проверку записи метки значения
	ASSERT_NE(record.find("!!str"), string::npos);
}
/**
 * @brief Проверка снятия значений полей отображения и снятия их
 *
 */
TEST(CodecYamlValue, Removing) {
	// Собираемое значение
	yaml::value_t value;
	// Выполняем занесение поля отображения
	value["a"] = yaml::value_t(static_cast <int64_t> (1));
	// Выполняем занесение поля отображения
	value["b"] = yaml::value_t(static_cast <int64_t> (2));
	// Выполняем занесение поля отображения
	value["c"] = yaml::value_t(static_cast <int64_t> (3));
	// Выполняем проверку успешности снятия поля отображения
	ASSERT_TRUE(value.erase("b"));
	// Выполняем проверку количества оставшихся полей
	ASSERT_EQ(value.size(), 2u);
	// Выполняем проверку сохранения порядка оставшихся полей
	ASSERT_EQ(value.key(0), "a");
	// Выполняем проверку сохранения порядка оставшихся полей
	ASSERT_EQ(value.key(1), "c");
	// Выполняем проверку отказа снятия отсутствующего поля
	ASSERT_FALSE(value.erase("b"));
	// Выполняем проверку успешности снятия поля по номеру
	ASSERT_TRUE(value.erase(static_cast <size_t> (0)));
	// Выполняем проверку количества оставшихся полей
	ASSERT_EQ(value.size(), 1u);
	// Выполняем проверку того, что имя снято вместе со значением
	ASSERT_EQ(value.key(0), "c");
	// Выполняем проверку отказа снятия значения за границею вместилища
	ASSERT_FALSE(value.erase(static_cast <size_t> (5)));
}
/**
 * @brief Проверка потоковой сборки значения
 *
 * @details Договор сборки слово в слово повторяет договор потока записи: открыть
 *          вместилище, назвать поле, записать значение, закрыть
 *
 */
TEST(CodecYamlValue, Building) {
	// Потоковая сборка значения
	yaml::builder_t builder;
	// Выполняем открытие отображения пар
	ASSERT_TRUE(builder.mapping());
	// Выполняем назначение имени поля отображения
	ASSERT_TRUE(builder.key("name"));
	// Выполняем запись строкового значения
	ASSERT_TRUE(builder.value("grok"));
	// Выполняем назначение имени поля отображения
	ASSERT_TRUE(builder.key("count"));
	// Выполняем запись целого числа
	ASSERT_TRUE(builder.value(static_cast <int64_t> (42)));
	// Выполняем назначение имени поля отображения
	ASSERT_TRUE(builder.key("tags"));
	// Выполняем открытие перечня значений
	ASSERT_TRUE(builder.sequence());
	// Выполняем проверку глубины открытых вместилищ
	ASSERT_EQ(builder.depth(), 2u);
	// Выполняем запись строкового значения
	ASSERT_TRUE(builder.value("a"));
	// Выполняем запись строкового значения
	ASSERT_TRUE(builder.value("b"));
	// Выполняем закрытие перечня значений
	ASSERT_TRUE(builder.close());
	// Выполняем назначение имени поля отображения
	ASSERT_TRUE(builder.key("empty"));
	// Выполняем запись пустого значения
	ASSERT_TRUE(builder.null());
	// Выполняем закрытие отображения пар
	ASSERT_TRUE(builder.close());
	// Получаем собранное значение
	const yaml::value_t value = builder.finish();
	// Выполняем проверку опустошения сборки изъятием
	ASSERT_EQ(builder.depth(), 0u);
	// Выполняем проверку вида собранного значения
	ASSERT_EQ(value.kind(), yaml::kind_t::MAPPING);
	// Выполняем проверку количества полей собранного значения
	ASSERT_EQ(value.size(), 4u);
	// Извлекаемое целое число
	int64_t count = 0;
	// Выполняем проверку успешности извлечения числа
	ASSERT_TRUE(value["count"].value(count));
	// Выполняем проверку извлечённого числа
	ASSERT_EQ(count, 42);
	// Выполняем проверку количества значений собранного перечня
	ASSERT_EQ(value["tags"].size(), 2u);
	// Выполняем проверку вида собранной пустоты
	ASSERT_EQ(value["empty"].kind(), yaml::kind_t::NUL);
	// Прочитанное обратно значение
	yaml::value_t parsed;
	// Выполняем обратное чтение записанного текста
	ASSERT_TRUE(parsed.parse(value.dump()));
	// Выполняем проверку совпадения собранного и прочитанного
	ASSERT_TRUE(parsed == value);
}
/**
 * @brief Проверка правила повторных имён потоковой сборки
 *
 * @details Значение берётся от последнего, а место от первого: порядок полей есть
 *          порядок записи их в текст, и перестановка поля при перезаписи его
 *          переменила бы текст там, где менялось лишь значение
 *
 */
TEST(CodecYamlValue, Duplicates) {
	// Потоковая сборка значения
	yaml::builder_t builder;
	// Выполняем открытие отображения пар
	ASSERT_TRUE(builder.mapping());
	// Выполняем назначение имени поля отображения
	ASSERT_TRUE(builder.key("first"));
	// Выполняем запись целого числа
	ASSERT_TRUE(builder.value(static_cast <int64_t> (1)));
	// Выполняем назначение имени поля отображения
	ASSERT_TRUE(builder.key("second"));
	// Выполняем запись целого числа
	ASSERT_TRUE(builder.value(static_cast <int64_t> (2)));
	// Выполняем назначение имени поля отображения повторного
	ASSERT_TRUE(builder.key("first"));
	// Выполняем запись целого числа
	ASSERT_TRUE(builder.value(static_cast <int64_t> (3)));
	// Получаем собранное значение
	const yaml::value_t value = builder.finish();
	// Выполняем проверку количества полей собранного значения
	ASSERT_EQ(value.size(), 2u);
	// Выполняем проверку того, что место поля взято от первого
	ASSERT_EQ(value.key(0), "first");
	// Выполняем проверку того, что место поля взято от первого
	ASSERT_EQ(value.key(1), "second");
	// Извлекаемое целое число
	int64_t first = 0;
	// Выполняем проверку успешности извлечения числа
	ASSERT_TRUE(value["first"].value(first));
	// Выполняем проверку того, что значение поля взято от последнего
	ASSERT_EQ(first, 3);
}
/**
 * @brief Проверка сборки значения простого корнем
 *
 */
TEST(CodecYamlValue, Solitary) {
	// Потоковая сборка значения
	yaml::builder_t builder;
	// Выполняем запись строкового значения корнем
	ASSERT_TRUE(builder.value("одно"));
	// Выполняем проверку отсутствия открытых вместилищ
	ASSERT_EQ(builder.depth(), 0u);
	// Выполняем проверку отказа записи после завершения сборки
	ASSERT_FALSE(builder.value("два"));
	// Получаем собранное значение
	const yaml::value_t value = builder.finish();
	// Выполняем проверку вида собранного значения
	ASSERT_EQ(value.kind(), yaml::kind_t::STRING);
	// Выполняем проверку записи собранного значения
	ASSERT_EQ(value.text(), "одно");
}
/**
 * @brief Проверка записи значения в файл и чтения его оттуда
 *
 */
TEST(CodecYamlValue, Storing) {
	// Собираемое значение
	yaml::value_t value;
	// Выполняем занесение строкового поля
	value["name"] = yaml::value_t("сервер");
	// Выполняем занесение числового поля
	value["port"] = yaml::value_t(static_cast <int64_t> (8080));
	// Выполняем проверку успешности записи значения в файл
	ASSERT_TRUE(value.save("./value.yaml"));
	// Прочитанное обратно значение
	yaml::value_t loaded;
	// Выполняем проверку успешности чтения значения из файла
	ASSERT_TRUE(loaded.load("./value.yaml"));
	// Выполняем проверку совпадения записанного и прочитанного
	ASSERT_TRUE(loaded == value);
	// Выполняем снятие записанного файла
	::remove("./value.yaml");
}
/**
 * @brief Проверка отказов потоковой сборки
 *
 * @details Отказы эти обнаруживают промах потребителя там, где молчаливая подстановка
 *          спрятала бы его: поле отображения без имени и имя, назначенное дважды
 *
 */
TEST(CodecYamlValue, Refusals) {
	// Потоковая сборка значения
	yaml::builder_t builder;
	// Выполняем открытие отображения пар
	ASSERT_TRUE(builder.mapping());
	// Выполняем проверку отказа записи значения без имени поля
	ASSERT_FALSE(builder.value("без имени"));
	// Выполняем проверку отказа открытия вместилища без имени поля
	ASSERT_FALSE(builder.sequence());
	// Выполняем назначение имени поля отображения
	ASSERT_TRUE(builder.key("name"));
	// Выполняем проверку отказа повторного назначения имени поля
	ASSERT_FALSE(builder.key("other"));
	// Выполняем запись строкового значения
	ASSERT_TRUE(builder.value("значение"));
	// Получаем собранное значение
	const yaml::value_t value = builder.finish();
	// Выполняем проверку количества полей собранного значения
	ASSERT_EQ(value.size(), 1u);
	// Выполняем проверку имени занесённого поля
	ASSERT_EQ(value.key(0), "name");
	// Потоковая сборка перечня значений
	yaml::builder_t list;
	// Выполняем открытие перечня значений
	ASSERT_TRUE(list.sequence());
	// Выполняем проверку того, что перечню имя поля не требуется
	ASSERT_TRUE(list.value("значение"));
	// Выполняем проверку количества значений собранного перечня
	ASSERT_EQ(list.finish().size(), 1u);
}
/**
 * @brief Проверка сличения отображений без учёта порядка полей
 *
 * @details Описание порядка полей отображения не предписывает вовсе, и два текста,
 *          одни и те же поля разным порядком записавшие, суть один документ
 *
 */
TEST(CodecYamlValue, Ordering) {
	// Значение, поля прямым порядком несущее
	yaml::value_t direct;
	// Выполняем разбор текста во владеющее значение
	ASSERT_TRUE(direct.parse("a: 1\nb: 2\n"));
	// Значение, поля обратным порядком несущее
	yaml::value_t reverse;
	// Выполняем разбор текста во владеющее значение
	ASSERT_TRUE(reverse.parse("b: 2\na: 1\n"));
	// Выполняем проверку совпадения отображений разного порядка
	ASSERT_TRUE(direct == reverse);
	// Выполняем проверку сохранения порядка записи полей
	ASSERT_EQ(direct.key(0), "a");
	// Выполняем проверку сохранения порядка записи полей
	ASSERT_EQ(reverse.key(0), "b");
	// Значение, поля с иным содержимым несущее
	yaml::value_t other;
	// Выполняем разбор текста во владеющее значение
	ASSERT_TRUE(other.parse("b: 3\na: 1\n"));
	// Выполняем проверку несовпадения отображений разного содержимого
	ASSERT_TRUE(direct != other);
	// Перечень значений прямого порядка
	yaml::value_t forward;
	// Выполняем разбор текста во владеющее значение
	ASSERT_TRUE(forward.parse("- 1\n- 2\n"));
	// Перечень значений обратного порядка
	yaml::value_t backward;
	// Выполняем разбор текста во владеющее значение
	ASSERT_TRUE(backward.parse("- 2\n- 1\n"));
	// Выполняем проверку того, что порядок значений перечня сличению подлежит
	ASSERT_TRUE(forward != backward);
}
/**
 * @brief Проверка заведения значения строковым литералом
 *
 * @details Без своего конструктора строковый литерал ушёл бы конструктором логического
 *          значения: приведение указателя к `bool` языку ближе приведения к строке
 *
 */
TEST(CodecYamlValue, Literals) {
	// Значение, литералом заведённое
	const yaml::value_t value("текст");
	// Выполняем проверку вида заведённого значения
	ASSERT_EQ(value.kind(), yaml::kind_t::STRING);
	// Выполняем проверку записи заведённого значения
	ASSERT_EQ(value.text(), "текст");
	// Значение, указателем пустым заведённое
	const yaml::value_t empty(static_cast <const char *> (nullptr));
	// Выполняем проверку вида заведённого значения
	ASSERT_EQ(empty.kind(), yaml::kind_t::STRING);
	// Выполняем проверку пустоты записи заведённого значения
	ASSERT_TRUE(empty.text().empty());
	// Значение логическое, заведённое видом своим
	const yaml::value_t flag(true);
	// Выполняем проверку вида заведённого значения
	ASSERT_EQ(flag.kind(), yaml::kind_t::BOOL);
}
/**
 * @brief Проверка согласия извлечения с деревом документа
 *
 * @details Проверка эта заведена оттого, что путей к числу здесь **два**: узел дерева
 *          хранит число, разобранное при чтении, а владеющее значение разбирает запись
 *          заново при снятии. Разойдись они - разойдутся и кодеки между собой, ибо
 *          договор извлечения у всех один
 *
 * @note Замечание это пришло от ведущего кодека JSON, где путь один и расхождение
 *       невозможно вовсе
 *
 */
TEST(CodecYamlValue, Consistency) {
	// Записи чисел, извлечение каких сличается
	const vector <string> records = {
		"1e300", "-1.5", "9007199254740993", "0.1", "0x1F", "0o17", "1e400",
		"-9223372036854775808", "18446744073709551615", ".nan", ".inf", "-.inf",
		"255", "-128", "3.4e38", "1.7976931348623157e308", "0.0", "-0.0"
	};
	// Собираемый текст документа
	string text;
	/**
	 * Выполняем перебор записей сличаемых чисел
	 */
	for(size_t i = 0; i < records.size(); i++){
		// Выполняем добавление имени поля документа
		text.append("n").append(std::to_string(i)).append(": ");
		// Выполняем добавление записи числа документа
		text.append(records.at(i)).append("\n");
	}
	// Дерево документа, записи чисел несущее
	yaml::document_t doc;
	// Выполняем разбор текста в дерево документа
	ASSERT_TRUE(doc.parse(text));
	// Значение, с дерева документа снятое
	const yaml::value_t value(doc.root());
	// Выполняем проверку количества снятых полей
	ASSERT_EQ(value.size(), records.size());
	/**
	 * Выполняем перебор записей сличаемых чисел
	 */
	for(size_t i = 0; i < records.size(); i++){
		// Получаем имя сличаемого поля
		const string name("n" + std::to_string(i));
		// Получаем ссылку на узел дерева документа
		const yaml::document_t::value_t node = doc.root()[name];
		// Получаем ссылку на снятое значение
		const yaml::value_t & taken = value[name];
		// Выполняем проверку совпадения записей значений
		ASSERT_EQ(node.text(), taken.text()) << "запись " << records.at(i);
		// Выполняем проверку совпадения видов хранения значений
		ASSERT_EQ(node.type(), taken.type()) << "запись " << records.at(i);
		// Извлекаемое целое число из узла дерева документа
		int64_t integer = 0;
		// Извлекаемое целое число из снятого значения
		int64_t number = 1;
		// Выполняем проверку совпадения успешности извлечения целого числа
		ASSERT_EQ(node.value(integer), taken.value(number)) << "запись " << records.at(i);
		// Выполняем проверку совпадения извлечённых целых чисел
		ASSERT_EQ(integer, number) << "запись " << records.at(i);
		// Извлекаемое целое число без знака из узла дерева документа
		uint64_t natural = 0;
		// Извлекаемое целое число без знака из снятого значения
		uint64_t unsigned_number = 1;
		// Выполняем проверку совпадения успешности извлечения целого числа без знака
		ASSERT_EQ(node.value(natural), taken.value(unsigned_number)) << "запись " << records.at(i);
		// Выполняем проверку совпадения извлечённых целых чисел без знака
		ASSERT_EQ(natural, unsigned_number) << "запись " << records.at(i);
		// Извлекаемое дробное число из узла дерева документа
		double real = 0.;
		// Извлекаемое дробное число из снятого значения
		double fraction = 1.;
		// Выполняем проверку совпадения успешности извлечения дробного числа
		ASSERT_EQ(node.value(real), taken.value(fraction)) << "запись " << records.at(i);
		/**
		 * Если извлечённое число числом является
		 *
		 * @note Нечисло сличению обычным порядком не поддаётся: оно не равно даже самому
		 *       себе, и сличать здесь надлежит признак нечисла у обоих
		 */
		if(!::isnan(real) && !::isnan(fraction))
			// Выполняем проверку совпадения извлечённых дробных чисел
			ASSERT_DOUBLE_EQ(real, fraction) << "запись " << records.at(i);
		// Выполняем проверку совпадения признаков нечисла
		else ASSERT_EQ(::isnan(real), ::isnan(fraction)) << "запись " << records.at(i);
		// Извлекаемое число шириною в один байт из узла дерева документа
		int8_t narrow = 0;
		// Извлекаемое число шириною в один байт из снятого значения
		int8_t shrunk = 1;
		// Выполняем проверку совпадения успешности сужения числа
		ASSERT_EQ(node.value(narrow), taken.value(shrunk)) << "запись " << records.at(i);
		// Выполняем проверку совпадения суженных чисел
		ASSERT_EQ(narrow, shrunk) << "запись " << records.at(i);
	}
}
/**
 * @brief Проверка отказа перезаписи по негодной кодировке
 *
 * @details Текст, оборванный отказом на половине, негоден вовсе: разобрать его нельзя,
 *          а отличить от целого глазом трудно. Перезапись отдаёт пустоту
 *
 * @note Правило это решено одинаковым у всех кодеков
 *
 */
TEST(CodecYamlValue, Refused) {
	// Собираемое значение
	yaml::value_t value;
	// Выполняем занесение годного поля
	value["good"] = yaml::value_t("годное");
	// Выполняем занесение поля, негодную последовательность несущего
	value["bad"] = yaml::value_t(string("\xFF\xFE", 2), yaml::style_t::PLAIN);
	// Выполняем занесение годного поля
	value["tail"] = yaml::value_t("хвост");
	// Настройки записи текста правилом замены
	yaml::writer_t::settings_t replacing;
	// Выполняем установку правила замены негодной последовательности
	replacing.malformed = yaml::malformed_t::REPLACE;
	// Получаем текст, правилом замены записанный
	const string replaced = value.dump(replacing);
	// Выполняем проверку того, что текст записан
	ASSERT_FALSE(replaced.empty());
	// Прочитанное обратно значение
	yaml::value_t parsed;
	// Выполняем проверку того, что записанное читается обратно
	ASSERT_TRUE(parsed.parse(replaced));
	// Выполняем проверку сохранности годных полей
	ASSERT_EQ(parsed["tail"].text(), "хвост");
	// Настройки записи текста правилом отказа
	yaml::writer_t::settings_t refusing;
	// Выполняем установку правила отказа записи
	refusing.malformed = yaml::malformed_t::REFUSE;
	// Получаем текст, правилом отказа записанный
	const string refused = value.dump(refusing);
	/**
	 * Выполняем проверку того, что отказ отдал пустоту, а не усечённый текст
	 *
	 * @note Усечённый текст нёс бы годное поле и половину негодного: разобрать его
	 *       нельзя, а на глаз он выглядит целым документом
	 */
	ASSERT_TRUE(refused.empty());
}
/**
 * @brief Проверка удержания наречия, директивой объявленного
 *
 * @details Директива `%YAML 1.1` переводит разбор на схему наречия 1.1, и записи вроде
 *          `on` становятся логическими. Перезапись обязана директиву сохранить: без неё
 *          чтение вернётся к схеме ядровой, и `on` вернётся строкою. Круговой ход
 *          переменил бы тем смысл документа, а не только написание его
 *
 * @note Нашёл это ворошитель круговым ходом снятого значения
 *
 */
TEST(CodecYamlValue, Dialect) {
	// Дерево документа, наречие директивой объявляющее
	yaml::document_t doc;
	// Выполняем разбор текста в дерево документа
	ASSERT_TRUE(doc.parse("%YAML 1.1\n---\n- on\n- 0777\n"));
	// Выполняем проверку того, что запись прочтена логическим значением
	ASSERT_EQ(doc.root()[0].kind(), yaml::kind_t::BOOL);
	// Выполняем проверку схемы, над разбором действовавшей
	ASSERT_EQ(doc.root().schema(), yaml::schema_t::LEGACY);
	// Выполняем проверку сохранения директивы перезаписью дерева
	ASSERT_NE(doc.dump().find("%YAML 1.1"), string::npos);
	// Дерево перезаписанного документа
	yaml::document_t back;
	// Выполняем проверку читаемости перезаписи обратным разбором
	ASSERT_TRUE(back.parse(doc.dump()));
	// Выполняем проверку сохранения вида значения круговым ходом
	ASSERT_EQ(back.root()[0].kind(), yaml::kind_t::BOOL);
	// Владеющее значение, с дерева снятое
	const yaml::value_t taken(doc.root());
	// Выполняем проверку снятия схемы вместе со значением
	ASSERT_EQ(taken[0].kind(), yaml::kind_t::BOOL);
	// Выполняем проверку сохранения директивы перезаписью значения
	ASSERT_NE(taken.dump().find("%YAML 1.1"), string::npos);
	// Прочитанное обратно владеющее значение
	yaml::value_t circled;
	// Выполняем проверку читаемости перезаписи обратным разбором
	ASSERT_TRUE(circled.parse(taken.dump()));
	// Выполняем проверку совпадения снятого и прочитанного
	ASSERT_TRUE(circled == taken);
	// Выполняем проверку сохранения восьмеричной записи наречия 1.1
	int64_t octal = 0;
	// Выполняем проверку успешности извлечения числа
	ASSERT_TRUE(circled[1].value(octal));
	// Выполняем проверку того, что число прочтено восьмеричным
	ASSERT_EQ(octal, 511);
	// Дерево документа без директивы наречия
	yaml::document_t plain;
	// Выполняем разбор текста в дерево документа
	ASSERT_TRUE(plain.parse("- on\n"));
	// Выполняем проверку того, что директива тексту не навязывается
	ASSERT_EQ(plain.dump().find("%YAML"), string::npos);
}
