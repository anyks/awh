/**
 * @file lexer.cpp
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
 * @brief Автоматические тесты разборщика параметров запуска — записи параметров всех
 *        поддержанных видов, позиционные доводы, признак конца именованных параметров,
 *        разрез текстового потока на слова и коды отказов разбора
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
#include <args/lexer.hpp>

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
 * @brief Внутренние помощники набора проверок разборщика
 *
 */
namespace {
	/**
	 * @brief Снимок разобранной лексемы, содержимое копирующий
	 *
	 * @details Лексема разбора ссылается на поданный набор и живёт не дольше его,
	 *          оттого проверки хранят её снимком, а не самой лексемой
	 *
	 */
	typedef struct Shot {
		// Вид лексемы разбора
		token_t type;
		// Признак наличия значения у именованного параметра
		bool assigned;
		// Имя параметра без ведущих тире
		string key;
		// Значение параметра либо содержимое позиционного довода
		string value;
		// Порядковый номер довода в поданном наборе
		size_t index;
		/**
		 * @brief Конструктор
		 *
		 * @param lexeme лексема разбора для снятия снимка
		 *
		 */
		Shot(const lexeme_t & lexeme) noexcept :
		 type(lexeme.type), assigned(lexeme.assigned),
		 key(lexeme.key), value(lexeme.value), index(lexeme.location.index) {}
	} shot_t;

	/**
	 * @brief Метод разбора набора доводов со снятием снимков лексем
	 *
	 * @param lexer  разборщик параметров запуска
	 * @param items  набор доводов запуска
	 * @param shots  контейнер снимков разобранных лексем
	 * @param errors контейнер кодов отказов разбора
	 * @return       результат разбора
	 *
	 */
	bool collect(const lexer_t & lexer, const vector <string> & items, vector <shot_t> & shots, vector <error_t> & errors) noexcept {
		// Выполняем очистку контейнера снимков разобранных лексем
		shots.clear();
		// Выполняем очистку контейнера кодов отказов разбора
		errors.clear();
		// Выполняем разбор набора доводов запуска
		return lexer.parse(items, [&shots](const lexeme_t & lexeme) noexcept -> bool {
			// Выполняем снятие снимка разобранной лексемы
			shots.emplace_back(lexeme);
			// Сообщаем, что разбор следует продолжить
			return true;
		}, [&errors](const error_t error, const location_t &) noexcept -> bool {
			// Выполняем запоминание кода отказа разбора
			errors.push_back(error);
			// Сообщаем, что разбор следует продолжить
			return true;
		});
	}
}

/**
 * @brief Проверка описания всех кодов ошибок разбора
 *
 */
TEST(ArgsLexer, Messages) {
	/**
	 * Выполняем перебор всех кодов ошибок разбора
	 *
	 * @warning Верхний предел перебора берётся ПОСЛЕДНИМ членом перечня, а не тем, что
	 *          стоял последним в день написания
	 */
	for(uint8_t i = 0; i <= static_cast <uint8_t> (error_t::FILESYSTEM); i++){
		// Получаем описание очередного кода ошибки разбора
		const char * text = args::message(static_cast <error_t> (i));
		// Выполняем проверку наличия описания кода ошибки
		ASSERT_NE(text, nullptr);
		// Выполняем проверку того, что описание кода ошибки не пусто
		ASSERT_FALSE(string(text).empty()) << static_cast <uint32_t> (i);
		/**
		 * Выполняем проверку того, что описание коду ОТВЕДЕНО, а не подставлено общим:
		 * выдача отвечает на код неизвестный строкою «unknown error», и она не пуста
		 */
		ASSERT_STRNE(text, "unknown error") << static_cast <uint32_t> (i);
	}
	// Выполняем проверку выдачи общего описания коду, отведённого не имеющему
	ASSERT_STREQ(args::message(static_cast <error_t> (0xFF)), "unknown error");
}

/**
 * @brief Проверка разбора записи параметра со значением через знак равенства
 *
 */
TEST(ArgsLexer, AssignedForm) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём разборщик параметров запуска
	const lexer_t lexer(&fmk, &log);
	// Контейнер снимков разобранных лексем
	vector <shot_t> shots;
	// Контейнер кодов отказов разбора
	vector <error_t> errors;
	// Выполняем разбор записей параметра обоими видами тире
	ASSERT_TRUE(collect(lexer, {"--name=value", "-other=second"}, shots, errors));
	// Выполняем проверку отсутствия отказов разбора
	ASSERT_TRUE(errors.empty());
	// Выполняем проверку числа разобранных лексем
	ASSERT_EQ(shots.size(), 2);
	// Выполняем проверку вида первой разобранной лексемы
	ASSERT_EQ(shots.at(0).type, token_t::PARAM);
	// Выполняем проверку имени первого разобранного параметра
	ASSERT_EQ(shots.at(0).key, "name");
	// Выполняем проверку значения первого разобранного параметра
	ASSERT_EQ(shots.at(0).value, "value");
	// Выполняем проверку признака наличия значения у первого параметра
	ASSERT_TRUE(shots.at(0).assigned);
	// Выполняем проверку имени второго разобранного параметра
	ASSERT_EQ(shots.at(1).key, "other");
	// Выполняем проверку значения второго разобранного параметра
	ASSERT_EQ(shots.at(1).value, "second");
}

/**
 * @brief Проверка разбора записи параметра со значением следующим доводом
 *
 */
TEST(ArgsLexer, SeparatedForm) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём разборщик параметров запуска
	const lexer_t lexer(&fmk, &log);
	// Контейнер снимков разобранных лексем
	vector <shot_t> shots;
	// Контейнер кодов отказов разбора
	vector <error_t> errors;
	// Выполняем разбор записей параметра обоими видами тире
	ASSERT_TRUE(collect(lexer, {"--name", "value", "-other", "second"}, shots, errors));
	// Выполняем проверку отсутствия отказов разбора
	ASSERT_TRUE(errors.empty());
	// Выполняем проверку числа разобранных лексем
	ASSERT_EQ(shots.size(), 2);
	// Выполняем проверку имени первого разобранного параметра
	ASSERT_EQ(shots.at(0).key, "name");
	// Выполняем проверку значения первого разобранного параметра
	ASSERT_EQ(shots.at(0).value, "value");
	// Выполняем проверку имени второго разобранного параметра
	ASSERT_EQ(shots.at(1).key, "other");
	// Выполняем проверку значения второго разобранного параметра
	ASSERT_EQ(shots.at(1).value, "second");
}

/**
 * @brief Проверка независимости разбора от положения параметра в наборе
 *
 */
TEST(ArgsLexer, OrderIndependence) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём разборщик параметров запуска
	const lexer_t lexer(&fmk, &log);
	// Контейнер снимков разобранных лексем
	vector <shot_t> shots;
	// Контейнер кодов отказов разбора
	vector <error_t> errors;
	// Выполняем разбор набора, где записи перемешаны с позиционными доводами
	ASSERT_TRUE(collect(lexer, {"first", "--name=value", "second", "-other", "third"}, shots, errors));
	// Выполняем проверку отсутствия отказов разбора
	ASSERT_TRUE(errors.empty());
	// Выполняем проверку числа разобранных лексем
	ASSERT_EQ(shots.size(), 4);
	// Выполняем проверку вида первой разобранной лексемы
	ASSERT_EQ(shots.at(0).type, token_t::OPERAND);
	// Выполняем проверку содержимого первого позиционного довода
	ASSERT_EQ(shots.at(0).value, "first");
	// Выполняем проверку имени разобранного параметра
	ASSERT_EQ(shots.at(1).key, "name");
	// Выполняем проверку содержимого второго позиционного довода
	ASSERT_EQ(shots.at(2).value, "second");
	// Выполняем проверку имени второго разобранного параметра
	ASSERT_EQ(shots.at(3).key, "other");
	// Выполняем проверку взятия следующего довода значением параметра
	ASSERT_EQ(shots.at(3).value, "third");
}

/**
 * @brief Проверка разбора параметра, значения не имеющего
 *
 */
TEST(ArgsLexer, FlagForm) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём разборщик параметров запуска
	const lexer_t lexer(&fmk, &log);
	// Контейнер снимков разобранных лексем
	vector <shot_t> shots;
	// Контейнер кодов отказов разбора
	vector <error_t> errors;
	// Выполняем разбор набора двух признаков, идущих подряд
	ASSERT_TRUE(collect(lexer, {"--verbose", "--debug"}, shots, errors));
	// Выполняем проверку отсутствия отказов разбора
	ASSERT_TRUE(errors.empty());
	// Выполняем проверку числа разобранных лексем
	ASSERT_EQ(shots.size(), 2);
	/**
	 * Выполняем проверку того, что первый признак второго НЕ СЪЕЛ: иначе всякий
	 * признак съедал бы следующий за ним параметр, и набор менял бы смысл от
	 * перестановки
	 */
	ASSERT_FALSE(shots.at(0).assigned);
	// Выполняем проверку имени первого разобранного признака
	ASSERT_EQ(shots.at(0).key, "verbose");
	// Выполняем проверку отсутствия значения у второго признака
	ASSERT_FALSE(shots.at(1).assigned);
	// Выполняем проверку имени второго разобранного признака
	ASSERT_EQ(shots.at(1).key, "debug");
}

/**
 * @brief Проверка отличия поданного пустого значения от значения, не поданного вовсе
 *
 */
TEST(ArgsLexer, EmptyValue) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём разборщик параметров запуска
	const lexer_t lexer(&fmk, &log);
	// Контейнер снимков разобранных лексем
	vector <shot_t> shots;
	// Контейнер кодов отказов разбора
	vector <error_t> errors;
	// Выполняем разбор записи с пустым значением и записи без значения вовсе
	ASSERT_TRUE(collect(lexer, {"--name=", "--other"}, shots, errors));
	// Выполняем проверку отсутствия отказов разбора
	ASSERT_TRUE(errors.empty());
	// Выполняем проверку числа разобранных лексем
	ASSERT_EQ(shots.size(), 2);
	// Выполняем проверку того, что пустое значение поданным считается
	ASSERT_TRUE(shots.at(0).assigned);
	// Выполняем проверку пустоты поданного значения
	ASSERT_TRUE(shots.at(0).value.empty());
	// Выполняем проверку того, что значение не подано вовсе
	ASSERT_FALSE(shots.at(1).assigned);
}

/**
 * @brief Проверка взятия отрицательного числа значением параметра
 *
 */
TEST(ArgsLexer, NegativeNumber) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём разборщик параметров запуска
	const lexer_t lexer(&fmk, &log);
	// Контейнер снимков разобранных лексем
	vector <shot_t> shots;
	// Контейнер кодов отказов разбора
	vector <error_t> errors;
	// Выполняем разбор записи параметра с отрицательным числом значением
	ASSERT_TRUE(collect(lexer, {"--count", "-5", "--rate", "-1.5e-3"}, shots, errors));
	// Выполняем проверку отсутствия отказов разбора
	ASSERT_TRUE(errors.empty());
	// Выполняем проверку числа разобранных лексем
	ASSERT_EQ(shots.size(), 2);
	// Выполняем проверку взятия целого отрицательного числа значением
	ASSERT_EQ(shots.at(0).value, "-5");
	// Выполняем проверку взятия дробного отрицательного числа значением
	ASSERT_EQ(shots.at(1).value, "-1.5e-3");
	// Создаём настройки разбора параметров
	lexer_t::settings_t settings;
	// Снимаем признак взятия числа значением параметра
	settings.negative = false;
	// Создаём разборщик параметров запуска с изменёнными настройками
	lexer_t plain(&fmk, &log);
	// Устанавливаем настройки разбора параметров
	plain.settings(settings);
	// Выполняем разбор того же набора с отключённым признаком
	ASSERT_TRUE(collect(plain, {"--count", "-5"}, shots, errors));
	// Выполняем проверку того, что число значением более не берётся
	ASSERT_EQ(shots.size(), 2);
	// Выполняем проверку отсутствия значения у параметра
	ASSERT_FALSE(shots.at(0).assigned);
	// Выполняем проверку разбора числа именованным параметром
	ASSERT_EQ(shots.at(1).key, "5");
}

/**
 * @brief Проверка признака конца именованных параметров
 *
 */
TEST(ArgsLexer, Terminus) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём разборщик параметров запуска
	const lexer_t lexer(&fmk, &log);
	// Контейнер снимков разобранных лексем
	vector <shot_t> shots;
	// Контейнер кодов отказов разбора
	vector <error_t> errors;
	// Выполняем разбор набора с признаком конца именованных параметров
	ASSERT_TRUE(collect(lexer, {"--name=value", "--", "--other=second"}, shots, errors));
	// Выполняем проверку отсутствия отказов разбора
	ASSERT_TRUE(errors.empty());
	// Выполняем проверку числа разобранных лексем
	ASSERT_EQ(shots.size(), 3);
	// Выполняем проверку вида лексемы признака конца параметров
	ASSERT_EQ(shots.at(1).type, token_t::TERMINUS);
	/**
	 * Выполняем проверку того, что довод за признаком конца разобран позиционным
	 * ЦЕЛИКОМ, а не именованным параметром
	 */
	ASSERT_EQ(shots.at(2).type, token_t::OPERAND);
	// Выполняем проверку содержимого позиционного довода
	ASSERT_EQ(shots.at(2).value, "--other=second");
}

/**
 * @brief Проверка отказа разбора при пустом имени параметра
 *
 */
TEST(ArgsLexer, EmptyKeyFailure) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём разборщик параметров запуска
	const lexer_t lexer(&fmk, &log);
	// Контейнер снимков разобранных лексем
	vector <shot_t> shots;
	// Контейнер кодов отказов разбора
	vector <error_t> errors;
	// Выполняем разбор набора с пустым именем параметра
	ASSERT_TRUE(collect(lexer, {"--=value", "--name=value"}, shots, errors));
	// Выполняем проверку числа отказов разбора
	ASSERT_EQ(errors.size(), 1);
	// Выполняем проверку кода отказа разбора
	ASSERT_EQ(errors.at(0), error_t::EMPTY_KEY);
	/**
	 * Выполняем проверку того, что разбор отказом НЕ ПРЕРВАН: набор разбирается
	 * целиком, чтобы приложение показало сразу все огрехи набора
	 */
	ASSERT_EQ(shots.size(), 1);
	// Выполняем проверку имени разобранного параметра
	ASSERT_EQ(shots.at(0).key, "name");
}

/**
 * @brief Проверка разреза текстового потока на слова
 *
 */
TEST(ArgsLexer, TextSplit) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём разборщик параметров запуска
	const lexer_t lexer(&fmk, &log);
	// Контейнер слов, собранных разрезом текста
	vector <string> items;
	// Выполняем разрез текста с кавычками и обратной косой
	ASSERT_TRUE(lexer.split("  --name=\"first second\"   --other='third'  \\ fourth ", items));
	// Выполняем проверку числа собранных слов
	ASSERT_EQ(items.size(), 3);
	// Выполняем проверку объединения кавычками двойными
	ASSERT_EQ(items.at(0), "--name=first second");
	// Выполняем проверку объединения кавычками одинарными
	ASSERT_EQ(items.at(1), "--other=third");
	// Выполняем проверку снятия особого значения обратной косой
	ASSERT_EQ(items.at(2), " fourth");
}

/**
 * @brief Проверка совпадения разбора текстового потока с разбором набора запуска
 *
 */
TEST(ArgsLexer, TextMatchesArgv) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём разборщик параметров запуска
	const lexer_t lexer(&fmk, &log);
	// Контейнер снимков лексем, разобранных из набора запуска
	vector <shot_t> shots;
	// Контейнер кодов отказов разбора
	vector <error_t> errors;
	// Выполняем разбор набора доводов запуска
	ASSERT_TRUE(collect(lexer, {"--name", "first second", "-flag", "operand"}, shots, errors));
	// Контейнер снимков лексем, разобранных из текстового потока
	vector <shot_t> stream;
	// Выполняем разбор того же самого набора, поданного текстом
	ASSERT_TRUE(lexer.parse("--name \"first second\" -flag operand", [&stream](const lexeme_t & lexeme) noexcept -> bool {
		// Выполняем снятие снимка разобранной лексемы
		stream.emplace_back(lexeme);
		// Сообщаем, что разбор следует продолжить
		return true;
	}));
	// Выполняем проверку совпадения числа разобранных лексем
	ASSERT_EQ(shots.size(), stream.size());
	// Выполняем перебор всех разобранных лексем
	for(size_t i = 0; i < shots.size(); i++){
		// Выполняем проверку совпадения вида лексемы
		ASSERT_EQ(shots.at(i).type, stream.at(i).type) << i;
		// Выполняем проверку совпадения признака наличия значения
		ASSERT_EQ(shots.at(i).assigned, stream.at(i).assigned) << i;
		// Выполняем проверку совпадения имени параметра
		ASSERT_EQ(shots.at(i).key, stream.at(i).key) << i;
		// Выполняем проверку совпадения значения параметра
		ASSERT_EQ(shots.at(i).value, stream.at(i).value) << i;
	}
}

/**
 * @brief Проверка отказов разбора текстового потока
 *
 */
TEST(ArgsLexer, TextFailures) {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект работы с логами
	const log_t log(&fmk);
	// Создаём разборщик параметров запуска
	const lexer_t lexer(&fmk, &log);
	// Контейнер слов, собранных разрезом текста
	vector <string> items;
	// Контейнер кодов отказов разбора
	vector <error_t> errors;
	// Создаём отзыв извещения об отказе разбора
	const lexer_t::failure_t failure = [&errors](const error_t error, const location_t &) noexcept -> bool {
		// Выполняем запоминание кода отказа разбора
		errors.push_back(error);
		// Сообщаем, что разбор следует продолжить
		return true;
	};
	// Выполняем разрез текста с незакрытой кавычкой
	ASSERT_TRUE(lexer.split("--name=\"first", items, failure));
	// Выполняем проверку числа отказов разбора
	ASSERT_EQ(errors.size(), 1);
	// Выполняем проверку кода отказа разбора
	ASSERT_EQ(errors.at(0), error_t::UNPAIRED);
	// Выполняем очистку контейнера кодов отказов разбора
	errors.clear();
	// Выполняем разрез текста, оканчивающегося обратной косой
	ASSERT_TRUE(lexer.split("--name=value\\", items, failure));
	// Выполняем проверку числа отказов разбора
	ASSERT_EQ(errors.size(), 1);
	// Выполняем проверку кода отказа разбора
	ASSERT_EQ(errors.at(0), error_t::DANGLING);
}
