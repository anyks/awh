/**
 * @file schema.cpp
 * @date 2026-09-03
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
 * @brief Автоматические тесты описания ожидаемых параметров запуска — заведение
 *        описаний, розыск по длинному и короткому имени, разбор склейки коротких имён,
 *        строгая проверка, значения по умолчанию и сборка справки о применении
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <args/args.hpp>

/**
 * Подключаем заголовочные файлы тестового окружения
 */
#include "../main.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;
using namespace awh::args;

/**
 * @brief Проверка заведения описаний ожидаемых параметров
 *
 */
TEST(ArgsSchema, Creation) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём описание ожидаемых параметров запуска
	schema_t schema(&fmk, &log);
	// Выполняем проверку пустоты нового описания
	ASSERT_TRUE(schema.empty());
	// Выполняем заведение описания ожидаемого параметра
	ASSERT_TRUE(schema.add("verbose", 'v', schema_t::value_t::NONE, "Подробный вывод"));
	// Выполняем заведение второго описания ожидаемого параметра
	ASSERT_TRUE(schema.add("net.port", 'p', schema_t::value_t::REQUIRED, "Порт службы"));
	// Выполняем проверку числа заведённых описаний
	ASSERT_EQ(schema.params().size(), 2);
	// Выполняем проверку розыска описания по длинному имени
	ASSERT_NE(schema.get("verbose"), nullptr);
	// Выполняем проверку розыска описания по короткому имени
	ASSERT_NE(schema.get('p'), nullptr);
	// Выполняем проверку длинного имени, разысканного по короткому
	ASSERT_EQ(schema.get('p')->name, "net.port");
	// Выполняем проверку отсутствия описания, не заведённого вовсе
	ASSERT_EQ(schema.get("missing"), nullptr);
	// Выполняем проверку отсутствия описания по незаведённому короткому имени
	ASSERT_EQ(schema.get('z'), nullptr);
	/**
	 * Выполняем проверку ОТКАЗА заведения при занятом коротком имени: два описания
	 * под одним знаком сделали бы разбор его гаданием
	 */
	ASSERT_FALSE(schema.add("другое", 'v', schema_t::value_t::NONE));
	// Выполняем проверку сохранности числа описаний после отказа
	ASSERT_EQ(schema.params().size(), 2);
	// Выполняем очистку описания ожидаемых параметров
	schema.clear();
	// Выполняем проверку пустоты очищенного описания
	ASSERT_TRUE(schema.empty());
}

/**
 * @brief Проверка замены описания, заведённого повторно
 *
 */
TEST(ArgsSchema, Replacement) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём описание ожидаемых параметров запуска
	schema_t schema(&fmk, &log);
	// Выполняем заведение описания ожидаемого параметра
	ASSERT_TRUE(schema.add("port", 'p', schema_t::value_t::REQUIRED));
	// Выполняем заведение описания с тем же длинным именем и иным коротким
	ASSERT_TRUE(schema.add("port", 'P', schema_t::value_t::NONE));
	// Выполняем проверку сохранности числа описаний
	ASSERT_EQ(schema.params().size(), 1);
	// Выполняем проверку замены потребности параметра в значении
	ASSERT_EQ(schema.get("port")->value, schema_t::value_t::NONE);
	// Выполняем проверку розыска по новому короткому имени
	ASSERT_NE(schema.get('P'), nullptr);
	/**
	 * Выполняем проверку СНЯТИЯ розыска по прежнему короткому имени: иначе знак
	 * оставался бы занятым описанием, которого более нет
	 */
	ASSERT_EQ(schema.get('p'), nullptr);
}

/**
 * @brief Проверка разбора склейки коротких имён
 *
 */
TEST(ArgsSchema, Cluster) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём описание ожидаемых параметров запуска
	schema_t schema(&fmk, &log);
	// Выполняем заведение описаний признаков, значений не принимающих
	ASSERT_TRUE(schema.add("verbose", 'v', schema_t::value_t::NONE));
	// Выполняем заведение второго описания признака
	ASSERT_TRUE(schema.add("debug", 'd', schema_t::value_t::NONE));
	// Выполняем заведение описания параметра, значение требующего
	ASSERT_TRUE(schema.add("port", 'p', schema_t::value_t::REQUIRED));
	// Контейнер разобранных длинных имён
	vector <string> names;
	// Выполняем разбор склейки двух коротких имён
	ASSERT_TRUE(schema.cluster("vd", names));
	// Выполняем проверку числа разобранных имён
	ASSERT_EQ(names.size(), 2);
	// Выполняем проверку первого разобранного длинного имени
	ASSERT_EQ(names.at(0), "verbose");
	// Выполняем проверку второго разобранного длинного имени
	ASSERT_EQ(names.at(1), "debug");
	/**
	 * Выполняем проверку ОТКАЗА разбора при знаке, описанию неизвестном: запись эта
	 * неотличима от длинного имени под одним тире, и разбирать её наполовину значило
	 * бы гадать
	 */
	ASSERT_FALSE(schema.cluster("vz", names));
	// Выполняем проверку очистки контейнера при отказе разбора
	ASSERT_TRUE(names.empty());
	/**
	 * Выполняем проверку ОТКАЗА разбора при знаке, значения требующем: значение
	 * досталось бы лишь последнему знаку склейки, а прочие остались бы без него
	 */
	ASSERT_FALSE(schema.cluster("vp", names));
	// Выполняем проверку очистки контейнера при отказе разбора
	ASSERT_TRUE(names.empty());
}

/**
 * @brief Проверка разбора параметров по описанию ожидаемых
 *
 */
TEST(ArgsSchema, ParsingByDescription) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём объект сбора параметров запуска
	args_t args(&fmk, &log);
	// Выполняем заведение описания ожидаемого параметра
	ASSERT_TRUE(args.schema().add("verbose", 'v', schema_t::value_t::NONE));
	// Выполняем заведение описания параметра, значение требующего
	ASSERT_TRUE(args.schema().add("net.port", 'p', schema_t::value_t::REQUIRED));
	// Выполняем разбор набора доводов запуска короткими именами
	ASSERT_TRUE(args.parse({"-v", "-p", "8080"}));
	/**
	 * Выполняем проверку укладки по ДЛИННОМУ имени: короткое имя есть запись, а не
	 * второе имя настройки, и потребитель обращается к настройке одним именем
	 */
	ASSERT_TRUE(args.get <bool> ("verbose"));
	// Выполняем проверку укладки значения по длинному имени
	ASSERT_EQ(args.get <uint16_t> ("net.port"), 8080);
	// Выполняем проверку отсутствия укладки по короткому имени
	ASSERT_FALSE(args.has("v"));
}

/**
 * @brief Проверка разбора склейки коротких имён при разборе набора
 *
 */
TEST(ArgsSchema, ClusterParsing) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём объект сбора параметров запуска
	args_t args(&fmk, &log);
	// Выполняем заведение описаний признаков, значений не принимающих
	ASSERT_TRUE(args.schema().add("verbose", 'v', schema_t::value_t::NONE));
	// Выполняем заведение второго описания признака
	ASSERT_TRUE(args.schema().add("debug", 'd', schema_t::value_t::NONE));
	// Выполняем разбор набора доводов запуска со склейкой коротких имён
	ASSERT_TRUE(args.parse({"-vd"}));
	// Выполняем проверку укладки первого разобранного признака
	ASSERT_TRUE(args.get <bool> ("verbose"));
	// Выполняем проверку укладки второго разобранного признака
	ASSERT_TRUE(args.get <bool> ("debug"));
}

/**
 * @brief Проверка строгого разбора по описанию ожидаемых
 *
 */
TEST(ArgsSchema, StrictParsing) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём объект сбора параметров запуска
	args_t args(&fmk, &log);
	// Выполняем заведение описания ожидаемого параметра
	ASSERT_TRUE(args.schema().add("port", 'p', schema_t::value_t::REQUIRED));
	/**
	 * Выполняем разбор параметра, описанию неизвестного, БЕЗ строгости: приложению,
	 * принимающему настройки сверх описанных, отказ мешал бы
	 */
	ASSERT_TRUE(args.parse({"--other=value"}));
	// Выполняем проверку укладки параметра, описанию неизвестного
	ASSERT_EQ(args.get <string> ("other"), "value");
	// Создаём настройки сбора параметров запуска
	args_t::settings_t settings;
	// Взводим признак отказа на параметр, описанию неизвестный
	settings.strict = true;
	// Устанавливаем настройки сбора параметров запуска
	args.settings(settings);
	// Выполняем разбор того же параметра со взведённой строгостью
	ASSERT_FALSE(args.parse({"--other=second"}));
	// Выполняем проверку кода отказа разбора
	ASSERT_EQ(args.errors().at(0).first, error_t::UNKNOWN);
}

/**
 * @brief Проверка отказов по потребности параметра в значении
 *
 */
TEST(ArgsSchema, ValueNeeds) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём объект сбора параметров запуска
	args_t args(&fmk, &log);
	// Выполняем заведение описания признака, значений не принимающего
	ASSERT_TRUE(args.schema().add("verbose", 'v', schema_t::value_t::NONE));
	// Выполняем заведение описания параметра, значение требующего
	ASSERT_TRUE(args.schema().add("port", 'p', schema_t::value_t::REQUIRED));
	// Выполняем заведение описания параметра, значение принимающего необязательно
	ASSERT_TRUE(args.schema().add("log", 'l', schema_t::value_t::OPTIONAL));
	// Выполняем разбор признака с поданным значением
	ASSERT_FALSE(args.parse({"--verbose=да"}));
	// Выполняем проверку кода отказа разбора
	ASSERT_EQ(args.errors().at(0).first, error_t::ODD_VALUE);
	// Выполняем разбор параметра без потребного значения
	ASSERT_FALSE(args.parse({"--port"}));
	// Выполняем проверку кода отказа разбора
	ASSERT_EQ(args.errors().at(0).first, error_t::NO_VALUE);
	/**
	 * Выполняем разбор параметра, значение принимающего необязательно, ОБОИМИ
	 * способами: без значения он есть взведённый признак, со значением - настройка
	 */
	ASSERT_TRUE(args.parse({"--log"}));
	// Выполняем проверку укладки взведённого признака
	ASSERT_TRUE(args.get <bool> ("log"));
	// Выполняем очистку собранных параметров запуска
	args.clear();
	// Выполняем разбор того же параметра с поданным значением
	ASSERT_TRUE(args.parse({"--log=/var/log/app.log"}));
	// Выполняем проверку укладки поданного значения
	ASSERT_EQ(args.get <string> ("log"), "/var/log/app.log");
}

/**
 * @brief Проверка отказа при повторной подаче параметра
 *
 */
TEST(ArgsSchema, Duplicate) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём объект сбора параметров запуска
	args_t args(&fmk, &log);
	// Создаём описание ожидаемого параметра
	schema_t::param_t param;
	// Устанавливаем длинное имя параметра
	param.name = "host";
	// Устанавливаем потребность параметра в значении
	param.value = schema_t::value_t::REQUIRED;
	// Выполняем заведение описания ожидаемого параметра
	ASSERT_TRUE(args.schema().add(param));
	// Выполняем разбор набора с повторной подачей параметра
	ASSERT_FALSE(args.parse({"--host=first", "--host=second"}));
	// Выполняем проверку кода отказа разбора
	ASSERT_EQ(args.errors().at(0).first, error_t::DUPLICATE);
	// Выполняем очистку собранных параметров запуска
	args.clear();
	// Взводим признак дозволенности повторной подачи параметра
	param.multiple = true;
	// Выполняем замену описания ожидаемого параметра
	ASSERT_TRUE(args.schema().add(param));
	// Выполняем разбор того же набора с дозволенным повтором
	ASSERT_TRUE(args.parse({"--host=first", "--host=second"}));
	// Выполняем проверку числа значений вместимого
	ASSERT_EQ(args.size("host"), 2);
}

/**
 * @brief Проверка обязательных параметров и значений по умолчанию
 *
 */
TEST(ArgsSchema, Verification) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём объект сбора параметров запуска
	args_t args(&fmk, &log);
	// Создаём описание обязательного параметра
	schema_t::param_t required;
	// Устанавливаем длинное имя обязательного параметра
	required.name = "host";
	// Взводим признак обязательности параметра
	required.required = true;
	// Выполняем заведение описания обязательного параметра
	ASSERT_TRUE(args.schema().add(required));
	// Создаём описание параметра со значением по умолчанию
	schema_t::param_t preset;
	// Устанавливаем длинное имя параметра
	preset.name = "net.port";
	// Устанавливаем значение параметра по умолчанию
	preset.fallback = "8080";
	// Взводим признак наличия значения по умолчанию
	preset.preset = true;
	// Выполняем заведение описания параметра
	ASSERT_TRUE(args.schema().add(preset));
	// Выполняем разбор набора без обязательного параметра
	ASSERT_TRUE(args.parse({"--net.port=443"}));
	// Выполняем проверку отказа проверки собранного
	ASSERT_FALSE(args.verify());
	// Выполняем проверку кода отказа проверки
	ASSERT_EQ(args.errors().back().first, error_t::REQUIRED);
	// Выполняем очистку собранных параметров запуска
	args.clear();
	// Выполняем разбор набора с обязательным параметром
	ASSERT_TRUE(args.parse({"--host=localhost"}));
	// Выполняем проверку собранного по описанию ожидаемых
	ASSERT_TRUE(args.verify());
	/**
	 * Выполняем проверку укладки значения по умолчанию проверкою: параметр,
	 * набором не поданный, берётся из описания
	 */
	ASSERT_EQ(args.get <uint16_t> ("net.port"), 8080);
	// Выполняем проверку источника уложенного значения
	ASSERT_EQ(args.source("net.port"), source_t::DEFAULT);
	// Выполняем проверку сохранности поданного набором значения
	ASSERT_EQ(args.get <string> ("host"), "localhost");
}

/**
 * @brief Проверка сборки справки о применении
 *
 */
TEST(ArgsSchema, Usage) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём объект сбора параметров запуска
	args_t args(&fmk, &log);
	// Устанавливаем название приложения для справки
	args.schema().application("application", "Служба, отдающая настройки");
	// Выполняем заведение описания признака
	ASSERT_TRUE(args.schema().add("verbose", 'v', schema_t::value_t::NONE, "Подробный вывод"));
	// Создаём описание параметра со значением по умолчанию
	schema_t::param_t param;
	// Устанавливаем длинное имя параметра
	param.name = "port";
	// Устанавливаем короткое имя параметра
	param.letter = 'p';
	// Устанавливаем описание назначения параметра
	param.description = "Порт службы";
	// Устанавливаем значение параметра по умолчанию
	param.fallback = "8080";
	// Взводим признак наличия значения по умолчанию
	param.preset = true;
	// Выполняем заведение описания параметра
	ASSERT_TRUE(args.schema().add(param));
	// Выполняем заведение описания параметра, значение принимающего необязательно
	ASSERT_TRUE(args.schema().add("log", 'l', schema_t::value_t::OPTIONAL, "Назначение журнала"));
	// Собираем справку о применении
	const string & usage = args.usage();
	// Выполняем проверку наличия в справке строки применения приложения
	ASSERT_NE(usage.find("Usage: application [OPTIONS]"), string::npos);
	// Выполняем проверку наличия в справке описания назначения приложения
	ASSERT_NE(usage.find("Служба, отдающая настройки"), string::npos);
	/**
	 * Выполняем проверку того, что справка собрана ИЗ ОПИСАНИЯ: имена, обозначения
	 * значений и описания назначений берутся у самих описаний, а не пишутся вторым
	 * разом руками
	 */
	ASSERT_NE(usage.find("-v, --verbose"), string::npos);
	// Выполняем проверку обозначения потребного значения
	ASSERT_NE(usage.find("-p, --port=VALUE"), string::npos);
	// Выполняем проверку обозначения необязательного значения
	ASSERT_NE(usage.find("-l, --log[=VALUE]"), string::npos);
	// Выполняем проверку наличия описания назначения параметра
	ASSERT_NE(usage.find("Порт службы"), string::npos);
	// Выполняем проверку наличия значения по умолчанию в справке
	ASSERT_NE(usage.find("(default: 8080)"), string::npos);
}
