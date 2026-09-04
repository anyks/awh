/**
 * @file args.cpp
 * @date 2026-09-02
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
 * @brief Автоматические тесты сбора параметров запуска — укладка разобранного в дерево
 *        настроек, выведение вида значения, старшинство источников, повторно поданные
 *        параметры, позиционные доводы и сбор переменных окружения
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstdlib>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <args/args.hpp>

/**
 * Подключаем заголовочные файлы тестового окружения
 */
#include "../main.hpp"

/**
 * Подключаем заголовочный файл пути к временному файлу
 */
#include "../codec/temporary.hpp"

/**
 * Стандартные заголовочные файлы записи в файл
 */
#include <fstream>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;
using namespace awh::args;

/**
 * @brief Внутренние помощники набора проверок сбора параметров
 *
 */
namespace {
	/**
	 * @brief Метод установки переменной окружения
	 *
	 * @details Посредник заведён оттого, что установка переменной окружения у
	 *          систем расходится: у MS Windows хода «setenv» нет вовсе. Без него
	 *          условная сборка расползлась бы по всем местам, где проверки трогают
	 *          окружение, и следующая правка их снова развела бы
	 *
	 * @param name  имя переменной окружения
	 * @param value значение переменной окружения
	 * @return      результат установки
	 *
	 */
	bool setupEnv(const char * name, const char * value) noexcept {
		/**
		 * Если операционной системой является MS Windows
		 */
		#if _WIN32 || _WIN64
			// Выполняем установку переменной окружения ходом системы
			return (::_putenv_s(name, value) == 0);
		/**
		 * Для всех остальных операционных систем
		 */
		#else
			// Выполняем установку переменной окружения с перезаписью прежней
			return (::setenv(name, value, 1) == 0);
		#endif
	}
	/**
	 * @brief Метод снятия переменной окружения
	 *
	 * @details У MS Windows снятие делается установкою пустого значения, и это
	 *          снятие НАСТОЯЩЕЕ: замерено на стенде Windows ARM64 - переменной не
	 *          видит ни «getenv», ни сам набор «_environ», - а не оставление её с
	 *          пустым значением
	 *
	 * @param name имя переменной окружения
	 * @return     результат снятия
	 *
	 */
	bool clearEnv(const char * name) noexcept {
		/**
		 * Если операционной системой является MS Windows
		 */
		#if _WIN32 || _WIN64
			// Выполняем снятие переменной окружения пустым значением
			return (::_putenv_s(name, "") == 0);
		/**
		 * Для всех остальных операционных систем
		 */
		#else
			// Выполняем снятие переменной окружения ходом системы
			return (::unsetenv(name) == 0);
		#endif
	}
}

/**
 * @brief Проверка укладки разобранных параметров в дерево настроек
 *
 */
TEST(ArgsArgs, Laying) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём объект сбора параметров запуска
	args_t args(&fmk, &log);
	// Выполняем разбор набора доводов запуска
	ASSERT_TRUE(args.parse({"--name=value", "--count", "17", "--rate=1.5", "--verbose"}));
	// Выполняем проверку извлечения последовательности знаков
	ASSERT_EQ(args.get <string> ("name"), "value");
	// Выполняем проверку извлечения числа целого
	ASSERT_EQ(args.get <uint16_t> ("count"), 17);
	// Выполняем проверку извлечения числа дробного
	ASSERT_DOUBLE_EQ(args.get <double> ("rate"), 1.5);
	/**
	 * Выполняем проверку того, что параметр без значения уложен ИСТИНОЙ, а не
	 * пустою последовательностью знаков
	 */
	ASSERT_TRUE(args.get <bool> ("verbose"));
	// Выполняем проверку наличия уложенного параметра
	ASSERT_TRUE(args.has("name"));
	// Выполняем проверку отсутствия не поданного параметра
	ASSERT_FALSE(args.has("missing"));
}

/**
 * @brief Проверка укладки параметра по вложенному пути
 *
 */
TEST(ArgsArgs, NestedPath) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём объект сбора параметров запуска
	args_t args(&fmk, &log);
	// Выполняем разбор набора доводов запуска со вложенными путями
	ASSERT_TRUE(args.parse({"--net.port=8080", "--net.host=localhost"}));
	// Выполняем проверку извлечения значения по вложенному пути
	ASSERT_EQ(args.get <uint16_t> ("net.port"), 8080);
	// Выполняем проверку извлечения второго значения по вложенному пути
	ASSERT_EQ(args.get <string> ("net.host"), "localhost");
	// Выполняем проверку заведения промежуточного звена пути отображением
	ASSERT_TRUE(args.root().at("net").is(codec::abc::type_t::MAP));
	// Выполняем проверку числа полей промежуточного звена пути
	ASSERT_EQ(args.root().at("net").size(), 2);
}

/**
 * @brief Проверка выведения вида значения из его записи
 *
 */
TEST(ArgsArgs, Derivation) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём объект сбора параметров запуска
	args_t args(&fmk, &log);
	// Выполняем разбор набора доводов запуска записями всех видов
	ASSERT_TRUE(args.parse({"--flag=true", "--number=17", "--real=1.5", "--text=value", "--zero=007", "--empty=null"}));
	// Выполняем проверку выведения логического значения
	ASSERT_TRUE(args.root().at("flag").is(codec::abc::type_t::BOOL));
	// Выполняем проверку выведения числа целого
	ASSERT_TRUE(args.root().at("number").is(codec::abc::type_t::INT));
	// Выполняем проверку выведения числа дробного
	ASSERT_TRUE(args.root().at("real").is(codec::abc::type_t::REAL));
	// Выполняем проверку укладки записи последовательностью знаков
	ASSERT_TRUE(args.root().at("text").is(codec::abc::type_t::STRING));
	/**
	 * Выполняем проверку того, что запись с ведущим нулём числом НЕ ВЗЯТА: записи
	 * вида «007» встречаются номерами, и перевод их в число срезал бы нули
	 */
	ASSERT_TRUE(args.root().at("zero").is(codec::abc::type_t::STRING));
	// Выполняем проверку сохранности записи с ведущим нулём
	ASSERT_EQ(args.get <string> ("zero"), "007");
	// Выполняем проверку выведения пустого значения
	ASSERT_TRUE(args.root().at("empty").is(codec::abc::type_t::NUL));
}

/**
 * @brief Проверка отключения выведения вида значения настройками
 *
 */
TEST(ArgsArgs, UntypedLaying) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём объект сбора параметров запуска
	args_t args(&fmk, &log);
	// Создаём настройки сбора параметров запуска
	args_t::settings_t settings;
	// Снимаем признак выведения вида значения из его записи
	settings.typed = false;
	// Устанавливаем настройки сбора параметров запуска
	args.settings(settings);
	// Выполняем разбор набора доводов запуска
	ASSERT_TRUE(args.parse({"--number=17", "--flag=true"}));
	// Выполняем проверку укладки числа последовательностью знаков
	ASSERT_TRUE(args.root().at("number").is(codec::abc::type_t::STRING));
	/**
	 * Выполняем проверку того, что извлечение числом работает и без выведения вида:
	 * потребитель от отключённой настройки извлечения лишаться не должен
	 */
	ASSERT_EQ(args.get <uint16_t> ("number"), 17);
	// Выполняем проверку извлечения логического значения из записи
	ASSERT_TRUE(args.get <bool> ("flag"));
}

/**
 * @brief Проверка укладки повторно поданного параметра вместимым
 *
 */
TEST(ArgsArgs, Multiple) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём объект сбора параметров запуска
	args_t args(&fmk, &log);
	// Выполняем разбор набора доводов запуска с повторами параметра
	ASSERT_TRUE(args.parse({"--host=first", "--host=second", "--host=third"}));
	// Выполняем проверку укладки повторов вместимым
	ASSERT_TRUE(args.root().at("host").is(codec::abc::type_t::ARRAY));
	// Выполняем проверку числа значений вместимого
	ASSERT_EQ(args.size("host"), 3);
	// Извлекаем значения вместимого параметра
	const vector <string> & items = args.arr <string> ("host");
	// Выполняем проверку числа извлечённых значений
	ASSERT_EQ(items.size(), 3);
	// Выполняем проверку сохранности порядка поданных значений
	ASSERT_EQ(items.at(0), "first");
	// Выполняем проверку второго значения вместимого
	ASSERT_EQ(items.at(1), "second");
	// Выполняем проверку третьего значения вместимого
	ASSERT_EQ(items.at(2), "third");
}

/**
 * @brief Проверка выдачи одиночного значения вместимым из него одного
 *
 */
TEST(ArgsArgs, SingleAsContainer) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём объект сбора параметров запуска
	args_t args(&fmk, &log);
	// Выполняем разбор набора доводов запуска с одиночным параметром
	ASSERT_TRUE(args.parse({"--host=first"}));
	// Извлекаем значения вместимого параметра
	const vector <string> & items = args.arr <string> ("host");
	/**
	 * Выполняем проверку того, что одиночное значение выдано вместимым из него
	 * одного: параметр, поданный единожды и дважды, потребителю различаться не должен
	 */
	ASSERT_EQ(items.size(), 1);
	// Выполняем проверку содержимого выданного вместимого
	ASSERT_EQ(items.at(0), "first");
}

/**
 * @brief Проверка старшинства источников значений
 *
 */
TEST(ArgsArgs, Seniority) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём объект сбора параметров запуска
	args_t args(&fmk, &log);
	// Выполняем установку значения по умолчанию
	ASSERT_TRUE(args.fallback("port", "80"));
	// Выполняем проверку укладки значения по умолчанию
	ASSERT_EQ(args.get <uint16_t> ("port"), 80);
	// Выполняем проверку источника уложенного значения
	ASSERT_EQ(args.source("port"), source_t::DEFAULT);
	// Выполняем разбор набора доводов запуска
	ASSERT_TRUE(args.parse({"--port=8080"}));
	// Выполняем проверку перекрытия значения по умолчанию набором запуска
	ASSERT_EQ(args.get <uint16_t> ("port"), 8080);
	// Выполняем проверку источника перекрывшего значения
	ASSERT_EQ(args.source("port"), source_t::CLI);
	/**
	 * Выполняем установку значения по умолчанию ПОСЛЕ разбора набора запуска:
	 * значение младшего источника поверх старшего не ложится, в каком бы порядке
	 * источники ни подавались
	 */
	ASSERT_TRUE(args.fallback("port", "443"));
	// Выполняем проверку сохранности значения, поданного набором запуска
	ASSERT_EQ(args.get <uint16_t> ("port"), 8080);
	// Выполняем проверку сохранности источника уложенного значения
	ASSERT_EQ(args.source("port"), source_t::CLI);
	// Выполняем проверку неопределённости источника у не поданного параметра
	ASSERT_EQ(args.source("missing"), source_t::NONE);
}

/**
 * @brief Проверка сбора позиционных доводов набора запуска
 *
 */
TEST(ArgsArgs, Operands) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём объект сбора параметров запуска
	args_t args(&fmk, &log);
	// Выполняем разбор набора доводов запуска с позиционными доводами
	ASSERT_TRUE(args.parse({"first", "--name=value", "second"}));
	// Выполняем проверку числа собранных позиционных доводов
	ASSERT_EQ(args.operands().size(), 2);
	// Выполняем проверку сохранности порядка позиционных доводов
	ASSERT_EQ(args.operands().at(0), "first");
	// Выполняем проверку второго позиционного довода
	ASSERT_EQ(args.operands().at(1), "second");
	// Выполняем проверку укладки именованного параметра
	ASSERT_EQ(args.get <string> ("name"), "value");
}

/**
 * @brief Проверка пропуска первого довода набора запуска
 *
 */
TEST(ArgsArgs, ExecutablePath) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём объект сбора параметров запуска
	args_t args(&fmk, &log);
	// Создаём набор доводов запуска видом системы
	const char * items[] = {"/usr/local/bin/application", "--name=value"};
	// Выполняем разбор набора доводов запуска
	ASSERT_TRUE(args.parse(2, items));
	/**
	 * Выполняем проверку того, что первый довод набора ПРОПУЩЕН: им приходит путь
	 * к исполняемому файлу, а не параметр приложения
	 */
	ASSERT_TRUE(args.operands().empty());
	// Выполняем проверку укладки именованного параметра
	ASSERT_EQ(args.get <string> ("name"), "value");
}

/**
 * @brief Проверка разбора набора доводов запуска широкими знаками
 *
 */
TEST(ArgsArgs, WideArguments) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём объект сбора параметров запуска
	args_t args(&fmk, &log);
	// Создаём набор доводов запуска широкими знаками
	const wchar_t * items[] = {L"application", L"--name=value", L"--count=17"};
	// Выполняем разбор набора доводов запуска
	ASSERT_TRUE(args.parse(3, items));
	// Выполняем проверку укладки именованного параметра
	ASSERT_EQ(args.get <string> ("name"), "value");
	// Выполняем проверку укладки числового параметра
	ASSERT_EQ(args.get <uint16_t> ("count"), 17);
}

/**
 * @brief Проверка разбора текстового потока
 *
 */
TEST(ArgsArgs, TextStream) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём объект сбора параметров запуска
	args_t args(&fmk, &log);
	// Выполняем разбор поданного текстового потока
	ASSERT_TRUE(args.text("--name=\"first second\" --count 17 operand"));
	// Выполняем проверку укладки значения, объединённого кавычками
	ASSERT_EQ(args.get <string> ("name"), "first second");
	// Выполняем проверку укладки числового параметра
	ASSERT_EQ(args.get <uint16_t> ("count"), 17);
	// Выполняем проверку источника уложенного значения
	ASSERT_EQ(args.source("name"), source_t::TEXT);
	// Выполняем проверку сбора позиционного довода
	ASSERT_EQ(args.operands().size(), 1);
	// Выполняем проверку содержимого позиционного довода
	ASSERT_EQ(args.operands().at(0), "operand");
	// Выполняем разбор набора доводов запуска поверх текстового потока
	ASSERT_TRUE(args.parse({"--name=third"}));
	// Выполняем проверку перекрытия текстового потока набором запуска
	ASSERT_EQ(args.get <string> ("name"), "third");
}

/**
 * @brief Проверка сбора переменных окружения
 *
 */
TEST(ArgsArgs, Environment) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём объект сбора параметров запуска
	args_t args(&fmk, &log);
	// Выполняем установку переменной окружения приложения
	ASSERT_TRUE(setupEnv("AWHTEST_NET_PORT", "8080"));
	// Выполняем установку второй переменной окружения приложения
	ASSERT_TRUE(setupEnv("AWHTEST_NAME", "value"));
	/**
	 * Выполняем установку переменной, начало имени которой совпадает с отведённым
	 * приложению лишь ЧАСТИЧНО: отбор ведётся по началу вместе с подчёркиванием
	 */
	ASSERT_TRUE(setupEnv("AWHTESTING_NAME", "foreign"));
	// Устанавливаем начало имён переменных окружения приложения
	args.prefix("AWHTEST");
	// Выполняем сбор переменных окружения
	ASSERT_TRUE(args.env());
	// Выполняем проверку укладки переменной окружения по вложенному пути
	ASSERT_EQ(args.get <uint16_t> ("net.port"), 8080);
	// Выполняем проверку укладки второй переменной окружения
	ASSERT_EQ(args.get <string> ("name"), "value");
	// Выполняем проверку источника уложенного значения
	ASSERT_EQ(args.source("name"), source_t::ENV);
	// Выполняем проверку числа полей дерева собранных настроек
	ASSERT_EQ(args.root().size(), 2);
	// Выполняем разбор набора доводов запуска поверх переменных окружения
	ASSERT_TRUE(args.parse({"--name=third"}));
	// Выполняем проверку перекрытия переменной окружения набором запуска
	ASSERT_EQ(args.get <string> ("name"), "third");
	// Выполняем проверку сохранности переменной, набором не поданной
	ASSERT_EQ(args.get <uint16_t> ("net.port"), 8080);
	// Выполняем снятие установленных переменных окружения
	ASSERT_TRUE(clearEnv("AWHTEST_NET_PORT"));
	// Выполняем снятие второй установленной переменной окружения
	ASSERT_TRUE(clearEnv("AWHTEST_NAME"));
	// Выполняем снятие переменной окружения с частично совпавшим началом
	ASSERT_TRUE(clearEnv("AWHTESTING_NAME"));
}

/**
 * @brief Проверка отказа сбора переменных окружения без отведённого начала имён
 *
 */
TEST(ArgsArgs, EnvironmentWithoutPrefix) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём объект сбора параметров запуска
	args_t args(&fmk, &log);
	/**
	 * Выполняем проверку отказа сбора переменных окружения: без отведённого начала
	 * имён отбор вёлся бы по всему окружению целиком
	 */
	ASSERT_FALSE(args.env());
	// Выполняем проверку пустоты дерева собранных настроек
	ASSERT_EQ(args.root().size(), 0);
}

/**
 * @brief Проверка очистки собранных параметров запуска
 *
 */
TEST(ArgsArgs, Clearing) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём объект сбора параметров запуска
	args_t args(&fmk, &log);
	// Выполняем разбор набора доводов запуска
	ASSERT_TRUE(args.parse({"--name=value", "operand"}));
	// Выполняем очистку собранных параметров запуска
	args.clear();
	// Выполняем проверку очистки дерева собранных настроек
	ASSERT_FALSE(args.has("name"));
	// Выполняем проверку очистки позиционных доводов
	ASSERT_TRUE(args.operands().empty());
	// Выполняем проверку очистки источников собранных значений
	ASSERT_EQ(args.source("name"), source_t::NONE);
	// Выполняем разбор набора доводов запуска после очистки
	ASSERT_TRUE(args.parse({"--name=second"}));
	// Выполняем проверку укладки параметра после очистки
	ASSERT_EQ(args.get <string> ("name"), "second");
}

/**
 * @brief Проверка разбора записи настроек кодеком
 *
 */
TEST(ArgsArgs, Config) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём объект сбора параметров запуска
	args_t args(&fmk, &log);
	// Выполняем разбор записи настроек кодеком
	ASSERT_TRUE(args.config("{\"name\":\"value\",\"net\":{\"port\":8080,\"host\":\"localhost\"},\"list\":[1,2,3]}", codec::Bridge::format_t::JSON));
	// Выполняем проверку укладки значения записи настроек
	ASSERT_EQ(args.get <string> ("name"), "value");
	// Выполняем проверку укладки значения по вложенному пути
	ASSERT_EQ(args.get <uint16_t> ("net.port"), 8080);
	// Выполняем проверку источника уложенного значения
	ASSERT_EQ(args.source("net.port"), source_t::FILE);
	// Выполняем проверку укладки вместимого целиком
	ASSERT_EQ(args.size("list"), 3);
	// Выполняем разбор набора доводов запуска поверх записи настроек
	ASSERT_TRUE(args.parse({"--net.port=443"}));
	// Выполняем проверку перекрытия записи настроек набором запуска
	ASSERT_EQ(args.get <uint16_t> ("net.port"), 443);
	/**
	 * Выполняем проверку того, что слияние прошло ВГЛУБЬ: соседнее поле того же
	 * отображения набором запуска не перекрывалось и потеряться не должно
	 */
	ASSERT_EQ(args.get <string> ("net.host"), "localhost");
}

/**
 * @brief Проверка старшинства записи настроек перед значением по умолчанию
 *
 */
TEST(ArgsArgs, ConfigSeniority) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём объект сбора параметров запуска
	args_t args(&fmk, &log);
	// Выполняем разбор набора доводов запуска
	ASSERT_TRUE(args.parse({"--port=443"}));
	// Выполняем установку значения по умолчанию
	ASSERT_TRUE(args.fallback("timeout", "30"));
	/**
	 * Выполняем разбор записи настроек ПОСЛЕ набора запуска: значение младшего
	 * источника поверх старшего не ложится, в каком бы порядке источники ни подавались
	 */
	ASSERT_TRUE(args.config("{\"port\":8080,\"timeout\":60}", codec::Bridge::format_t::JSON));
	// Выполняем проверку сохранности значения, поданного набором запуска
	ASSERT_EQ(args.get <uint16_t> ("port"), 443);
	// Выполняем проверку перекрытия значения по умолчанию записью настроек
	ASSERT_EQ(args.get <uint16_t> ("timeout"), 60);
	// Выполняем проверку источника перекрывшего значения
	ASSERT_EQ(args.source("timeout"), source_t::FILE);
}

/**
 * @brief Проверка отказа разбора негодной записи настроек
 *
 */
TEST(ArgsArgs, ConfigFailure) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём объект сбора параметров запуска
	args_t args(&fmk, &log);
	// Выполняем разбор негодной записи настроек кодеком
	ASSERT_FALSE(args.config("{\"name\":", codec::Bridge::format_t::JSON));
	// Выполняем проверку числа отказов разбора
	ASSERT_EQ(args.errors().size(), 1);
	// Выполняем проверку кода отказа разбора
	ASSERT_EQ(args.errors().at(0).first, error_t::CODEC);
	// Выполняем проверку пустоты дерева собранных настроек
	ASSERT_EQ(args.root().size(), 0);
}

/**
 * @brief Проверка выдачи дерева настроек записью кодека
 *
 */
TEST(ArgsArgs, Dump) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём объект сбора параметров запуска
	args_t args(&fmk, &log);
	// Получаем настройки перевода дерева настроек
	codec::bridge_t::settings_t settings = args.bridge().settings();
	// Устанавливаем вид оформления собираемой записи без отступов
	settings.format = codec::json::format_t::COMPACT;
	// Устанавливаем настройки перевода дерева настроек
	args.bridge().settings(settings);
	// Выполняем разбор набора доводов запуска
	ASSERT_TRUE(args.parse({"--name=value", "--net.port=8080", "--verbose"}));
	// Собираемая запись настроек
	string text = "";
	// Выполняем выдачу дерева настроек записью кодека
	ASSERT_TRUE(args.dump(text, codec::Bridge::format_t::JSON));
	// Выполняем проверку собранной записи настроек
	ASSERT_EQ(text, "{\"name\":\"value\",\"net\":{\"port\":8080},\"verbose\":true}");
	// Создаём второй объект сбора параметров запуска
	args_t second(&fmk, &log);
	// Выполняем разбор выданной записи настроек
	ASSERT_TRUE(second.config(text, codec::Bridge::format_t::JSON));
	/**
	 * Выполняем проверку ОБРАТИМОСТИ выдачи: настройки, выданные записью кодека и
	 * прочитанные обратно, должны совпасть с исходными
	 */
	ASSERT_EQ(second.get <string> ("name"), "value");
	// Выполняем проверку обратимости значения по вложенному пути
	ASSERT_EQ(second.get <uint16_t> ("net.port"), 8080);
	// Выполняем проверку обратимости взведённого признака
	ASSERT_TRUE(second.get <bool> ("verbose"));
}

/**
 * @brief Проверка чтения и записи файла настроек
 *
 */
TEST(ArgsArgs, Filesystem) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём объект сбора параметров запуска
	args_t args(&fmk, &log);
	// Выполняем разбор набора доводов запуска
	ASSERT_TRUE(args.parse({"--name=value", "--net.port=8080"}));
	// Получаем путь к файлу настроек
	const string filename = "./args-settings.json";
	// Выполняем запись дерева настроек в файл
	ASSERT_TRUE(args.save(filename, codec::Bridge::format_t::JSON));
	// Создаём второй объект сбора параметров запуска
	args_t second(&fmk, &log);
	// Выполняем чтение записанного файла настроек
	ASSERT_TRUE(second.filename(filename, codec::Bridge::format_t::JSON));
	// Выполняем проверку прочитанного значения настроек
	ASSERT_EQ(second.get <string> ("name"), "value");
	// Выполняем проверку прочитанного значения по вложенному пути
	ASSERT_EQ(second.get <uint16_t> ("net.port"), 8080);
	// Выполняем проверку источника прочитанного значения
	ASSERT_EQ(second.source("name"), source_t::FILE);
	// Выполняем снятие записанного файла настроек
	ASSERT_EQ(::remove(filename.c_str()), 0);
	// Создаём третий объект сбора параметров запуска
	args_t third(&fmk, &log);
	// Выполняем проверку отказа чтения снесённого файла настроек
	ASSERT_FALSE(third.filename(filename, codec::Bridge::format_t::JSON));
	// Выполняем проверку кода отказа чтения файла настроек
	ASSERT_EQ(third.errors().at(0).first, error_t::FILESYSTEM);
}

/**
 * @brief Проверка приёма настроек записями всех кодеков
 *
 * @details Смысл модуля в том, чтобы настройки принимались ЛЮБЫМ видом записи,
 *          а разбирались единообразно: разные виды записи об одном и том же
 *          обязаны дать одно и то же дерево. Оттого проверка ведёт один и тот
 *          же набор настроек пятью видами и сличает исход, а не разбирает
 *          каждый вид сам по себе
 *
 */
TEST(ArgsArgs, ConfigEveryFormat){
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Перечень видов записи и записей настроек, им отвечающих
	const vector <pair <codec::Bridge::format_t, string>> samples = {
		{codec::Bridge::format_t::JSON, "{\"name\":\"значение\",\"net\":{\"port\":8080}}"},
		{codec::Bridge::format_t::YAML, "name: значение\nnet:\n  port: 8080\n"},
		{codec::Bridge::format_t::TOML, "name = \"значение\"\n[net]\nport = 8080\n"},
		{codec::Bridge::format_t::INI,  "name = значение\n[net]\nport = 8080\n"},
		{codec::Bridge::format_t::XML,  "<config><name>значение</name><net><port>8080</port></net></config>"}
	};
	// Выполняем перебор всех видов записи настроек
	for(auto & sample : samples){
		// Создаём объект сбора параметров запуска
		args_t args(&fmk, &log);
		// Выполняем разбор записи настроек кодеком
		ASSERT_TRUE(args.config(sample.second, sample.first)) << "вид записи " << static_cast <uint16_t> (sample.first);
		/**
		 * Выполняем выбор пути к значению
		 *
		 * @warning У записи XML корень ИМЕНОВАН - стандарт требует ровно один
		 *          корневой элемент, - и содержимое лежит под ним. Расхождение
		 *          это законно и вызвано стандартом, а не устройством моста,
		 *          потому путь здесь и разнится
		 */
		/**
		 * Приставки пути у записи разметки БОЛЬШЕ НЕТ
		 *
		 * @note Корневой узел разметки есть обнос записи, а не настройка, и модуль
		 *       снимает его при чтении: иначе настройка ложилась бы путём
		 *       `config.port` и доводом запуска не перекрывалась бы вовсе
		 */
		const string prefix = "";
		// Выполняем проверку укладки последовательности знаков
		ASSERT_EQ(args.get <string> (prefix + "name"), "значение") << "вид записи " << static_cast <uint16_t> (sample.first);
		// Выполняем проверку укладки значения по вложенному пути
		ASSERT_EQ(args.get <uint16_t> (prefix + "net.port"), 8080) << "вид записи " << static_cast <uint16_t> (sample.first);
	}
}

/**
 * @brief Проверка выдачи настроек записями всех кодеков
 *
 * @details Настройки, собранные из доводов запуска, обязаны выдаваться любым
 *          видом записи и приниматься обратно тем же видом. Круг здесь ведётся
 *          через сам модуль, а не через мост, потому что проверяется именно
 *          выдача модуля - с его именами путей и его разделителем
 *
 */
TEST(ArgsArgs, DumpEveryFormat){
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Перечень видов записи, круг через которые замкнут
	const vector <codec::Bridge::format_t> formats = {
		codec::Bridge::format_t::JSON,
		codec::Bridge::format_t::YAML,
		codec::Bridge::format_t::TOML,
		codec::Bridge::format_t::INI
	};
	// Выполняем перебор всех видов записи настроек
	for(auto & format : formats){
		// Создаём объект сбора параметров запуска
		args_t args(&fmk, &log);
		// Разбираемые доводы запуска приложения
		const char * argv[] = {"app", "--net.port=8080", "--name=значение"};
		// Выполняем разбор доводов запуска приложения
		ASSERT_TRUE(args.parse(3, argv)) << "вид записи " << static_cast <uint16_t> (format);
		// Собираемая запись настроек
		string text = "";
		// Выполняем выдачу дерева настроек записью кодека
		ASSERT_TRUE(args.dump(text, format)) << "вид записи " << static_cast <uint16_t> (format);
		// Создаём объект сбора параметров запуска для обратного приёма
		args_t second(&fmk, &log);
		// Выполняем разбор собранной записи настроек
		ASSERT_TRUE(second.config(text, format)) << "вид записи " << static_cast <uint16_t> (format) << ", собрано: " << text;
		// Выполняем проверку сохранности значения по вложенному пути
		ASSERT_EQ(second.get <uint16_t> ("net.port"), 8080) << "вид записи " << static_cast <uint16_t> (format) << ", собрано: " << text;
		// Выполняем проверку сохранности последовательности знаков
		ASSERT_EQ(second.get <string> ("name"), "значение") << "вид записи " << static_cast <uint16_t> (format) << ", собрано: " << text;
	}
}

/**
 * @brief Проверка вывода вида записи настроек из имени файла
 *
 * @warning Вид берётся РАСШИРЕНИЕМ, а не догадкою по содержимому: запись
 *          `port = 8080` годится и INI, и TOML разом, и догадка выбирала бы
 *          молча, а потребитель узнавал бы о выборе лишь по неверно прочтённым
 *          настройкам
 *
 */
TEST(ArgsConfig, FormatComesFromTheFileName){
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Перечень образцов записи настроек, расширением заданных
	const vector <pair <string, string>> samples = {
		{"ini",  "port = 7070\n"},
		{"json", "{\"port\":7071}"},
		{"yaml", "port: 7072\n"},
		{"toml", "port = 7073\n"},
		{"xml",  "<config><port>7074</port></config>"}
	};
	// Ожидаемая величина порта у очередного образца
	uint16_t expected = 7070;
	// Выполняем перебор всех образцов записи настроек
	for(auto & sample : samples){
		// Собираем путь к временному файлу настроек
		const string filename = ::temporary("awh-args-config." + sample.first);
		{
			// Открываем временный файл настроек на запись
			ofstream file(filename, ios::binary);
			// Выполняем запись образца настроек в файл
			file << sample.second;
		}
		// Создаём объект разбора доводов запуска
		args_t args(&fmk, &log);
		/**
		 * Выполняем чтение файла настроек без указания вида записи
		 *
		 * @note Вид выводится расширением имени, и все пять записей ложатся ОДНИМ
		 *       путём: обнос корневого узла у разметки снимается модулем
		 */
		ASSERT_TRUE(args.filename(filename)) << "расширение «" << sample.first << "» виду записи не сопоставлено";
		// Выполняем проверку прочтённой величины настройки
		ASSERT_EQ(args.get <uint16_t> ("port"), expected) << "расширение «" << sample.first << "»";
		// Увеличиваем ожидаемую величину порта
		expected++;
	}
	// Создаём объект разбора доводов запуска
	args_t args(&fmk, &log);
	/**
	 * Выполняем проверку отказа на расширении неведомом
	 *
	 * @warning Отказ здесь обязателен: догадка по содержимому выбирала бы вид
	 *          молча, а молчаливый выбор на записи, двум видам годной, есть
	 *          подмена настроек
	 */
	ASSERT_FALSE(args.filename("/tmp/awh-args-config.unknown")) << "расширение неведомое принято";
}
