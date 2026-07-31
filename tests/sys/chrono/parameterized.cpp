/**
 * @file: parameterized.cpp
 * @date: 2025-12-10
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Параметризованные тесты модуля работы с датой и временем —
 *        прогон подготовленных наборов входных данных через методы модуля с проверкой разбора и форматирования дат,
 *        конвертации единиц времени и работы с временными зонами
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Подключаем стандартные заголовочные файлы
 */
#include <iomanip>

/**
 * Подключаем заголовочный файл проекта
 */
#include "chrono.hpp"

/**
 * @brief Структура параметров тестирования метода парсинга
 *
 */
struct ParsingChronoTestParameter {
	// Дата для парсинга
	std::string date = "";
	// Формат даты
	std::string format = "";
	// Ожидаемый результат
	uint64_t result = 0;
};

/**
 * @brief Структура параметров метода парсинга
 *
 */
struct ParsingParameterizedFixture : public ChronoFixture, public ::testing::WithParamInterface <ParsingChronoTestParameter> {
	// Параметры теста
	ParsingChronoTestParameter _parameter = GetParam();
};

/**
 * @brief Тест метода парсинга
 *
 */
TEST_P(ParsingParameterizedFixture, ExecutionParsingChronoTest){
	// Выполняем проверку результата
	ASSERT_EQ(this->_chrono->parse(this->_parameter.date, this->_parameter.format), this->_parameter.result);
}

/**
 * @brief Параметры тестирования метода парсинга
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, ParsingParameterizedFixture,
	::testing::Values(
		ParsingChronoTestParameter({"2023-03-05T12:55:58.0490925Z", "%Y-%m-%dT%H:%M:%S.%s%Z", 1678020958049}),
		ParsingChronoTestParameter({"2024-08-06T11:08:55.101Z", "%Y-%m-%dT%H:%M:%S.%s%Z", 1722942535101}),
		ParsingChronoTestParameter({"2024-08-06T14:47:34.741876+03:00", "%Y-%m-%dT%H:%M:%S.%s%o", 1722944854741}),
		ParsingChronoTestParameter({"2024-08-06T14:47:34.728093306+03:0", "%Y-%m-%dT%H:%M:%S.%s%o", 1722944854728}),
		ParsingChronoTestParameter({"7/26/2023 2:39:42 PM", "%m/%d/%Y %I:%M:%S %p", 1690382382000}),
		ParsingChronoTestParameter({"2023-07-26T14:39:4", "%Y-%m-%dT%H:%M:%S", 1690382344000}),
		ParsingChronoTestParameter({"7/26/2023 2:39:42 PM (2934007)", "%m/%d/%Y %I:%M:%S %p (%s)", 1690382382293}),
		ParsingChronoTestParameter({"2024-11-15 17:14:03,331", "%Y-%m-%d %H:%M:%S,%s", 1731690843331}),
		ParsingChronoTestParameter({"Tue Jul 16 10:45:40.020399 2024", "%a %h %d %H:%M:%S.%s %Y", 1721126740020}),
		ParsingChronoTestParameter({"05/Apr/2023:12:45:12.345678901 +0300", "%d/%h/%Y:%H:%M:%S.%s %z", 1680687912345}),
		ParsingChronoTestParameter({"2024-10-16 10:30:45.789", "%Y-%m-%d %H:%M:%S.%s", 1729074645789}),
		ParsingChronoTestParameter({"[18/Jul/2024:13:34:00 +0300]", "%d/%h/%Y:%H:%M:%S %z", 1721298840000}),
		ParsingChronoTestParameter({"[18/Jul/24:13:34:00 +0300]", "%d/%h/%y:%H:%M:%S %z", 1721298840000}),
		ParsingChronoTestParameter({"2024/07/18 13:33:17", "%Y/%m/%d %H:%M:%S", 1721309597000}),
		ParsingChronoTestParameter({"17.07.2023 13:25:53", "%d.%m.%Y %H:%M:%S", 1689600353000}),
		ParsingChronoTestParameter({"Wed Mar 19 2025 15:51:10 GMT+0300", "%a %h %e %Y %H:%M:%S %z", 1742388670000}),
		ParsingChronoTestParameter({"Wed Mar 30 2025 15:51:10 GMT+0300", "%a %h %e %Y %H:%M:%S %Z%z", 1743339070000}),
		ParsingChronoTestParameter({"20050809T183142+0330", "%Y%m%dT%H%M%S%z", 1123599702000}),
		ParsingChronoTestParameter({"Wed Mar 19 2025 15:51:10", "%a %h %e %Y %H:%M:%S", 1742399470000})//,
		// ParsingChronoTestParameter({"May 27 13:12:47", "%Y-%m-%dT%H:%M:%S.%fZ", 0})
	)
);

/**
 * @brief Структура параметров тестирования метода получения аббревиатуры
 *
 */
struct AbbreviationChronoTestParameter {
	// Дата для получения аббревиатуры
	uint64_t date = 0;
	// Ожидаемый результат
	std::string result = "";
	// Тип результата
	awh::chrono_t::type_t type;
};

/**
 * @brief Структура параметров метода получения аббревиатуры
 *
 */
struct AbbreviationParameterizedFixture : public ChronoFixture, public ::testing::WithParamInterface <AbbreviationChronoTestParameter> {
	// Параметры теста
	AbbreviationChronoTestParameter _parameter = GetParam();
};

/**
 * @brief Тест метода получения аббревиатуры
 *
 */
TEST_P(AbbreviationParameterizedFixture, ExecutionAbbreviationChronoTest){
	// Выполняем проверку результата
	double intpart = 0;
	// Форматируем результат
	std::string result = "";
	// Создаём строковый поток
	std::stringstream ss = {};
	// Получаем аббревиатуру
	auto abbr = this->_chrono->abbreviation(this->_parameter.date);
	// Проверяем наличие дробной части
	if(std::modf(abbr.second, &intpart) > 0)
		// Форматируем число в строку
		ss << std::fixed << std::setprecision(3) << abbr.second;
	// Иначе форматируем без дробной части
	else ss << std::fixed << std::setprecision(0) << abbr.second;
	// Сохраняем результат
	ss >> result;
	// Проверяем результаты
	ASSERT_EQ(abbr.first, this->_parameter.type);
	// Проверяем результат
	ASSERT_EQ(result, this->_parameter.result);
}

/**
 * @brief Тестирование метода получения аббревиатуры
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, AbbreviationParameterizedFixture,
	::testing::Values(
		AbbreviationChronoTestParameter({1743943021520, "60.073", awh::chrono_t::type_t::YEAR}),
		AbbreviationChronoTestParameter({17439430215, "7.209", awh::chrono_t::type_t::MONTH}),
		AbbreviationChronoTestParameter({1743943021, "2.884", awh::chrono_t::type_t::WEEK}),
		AbbreviationChronoTestParameter({174394302, "2.018", awh::chrono_t::type_t::DAY}),
		AbbreviationChronoTestParameter({17439430, "4.844", awh::chrono_t::type_t::HOUR}),
		AbbreviationChronoTestParameter({1743943, "29.066", awh::chrono_t::type_t::MINUTES}),
		AbbreviationChronoTestParameter({17439, "17.439", awh::chrono_t::type_t::SECONDS}),
		AbbreviationChronoTestParameter({174, "174", awh::chrono_t::type_t::MILLISECONDS})
	)
);

/**
 * @brief Структура параметров тестирования метода получения начала/конца позиции указанной даты
 *
 */
struct IterationChronoTestParameter {
	// Дата для получения позиции
	uint64_t date = 0;
	// Ожидаемый результат
	uint64_t result = 0;
	// Тип результата
	awh::chrono_t::type_t type;
};

/**
 * @brief Структура параметров метода получения конца позиции указанной даты
 *
 */
struct EndParameterizedFixture : public ChronoFixture, public ::testing::WithParamInterface <IterationChronoTestParameter> {
	// Параметры теста
	IterationChronoTestParameter _parameter = GetParam();
};

/**
 * @brief Структура параметров метода получения начала позиции указанной даты
 *
 */
struct BeginParameterizedFixture : public ChronoFixture, public ::testing::WithParamInterface <IterationChronoTestParameter> {
	// Параметры теста
	IterationChronoTestParameter _parameter = GetParam();
};

/**
 * @brief Тест метода получения конца позиции указанной даты
 *
 */
TEST_P(EndParameterizedFixture, ExecutionEndChronoTest){
	// Выполняем проверку результата
	ASSERT_EQ(this->_chrono->end(this->_parameter.date, this->_parameter.type), this->_parameter.result);
}

/**
 * @brief Тест метода получения начала позиции указанной даты
 *
 */
TEST_P(BeginParameterizedFixture, ExecutionBeginChronoTest){
	// Выполняем проверку результата
	ASSERT_EQ(this->_chrono->begin(this->_parameter.date, this->_parameter.type), this->_parameter.result);
}

/**
 * @brief Тестирование метода получения конца позиции указанной даты
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, EndParameterizedFixture,
	::testing::Values(
		IterationChronoTestParameter({1743943021520, 1767225600000, awh::chrono_t::type_t::YEAR}),
		IterationChronoTestParameter({1743943021520, 1746057600000, awh::chrono_t::type_t::MONTH}),
		IterationChronoTestParameter({1743943021520, 1743984000000, awh::chrono_t::type_t::WEEK}),
		IterationChronoTestParameter({1743943021520, 1743984000000, awh::chrono_t::type_t::DAY}),
		IterationChronoTestParameter({1743943021520, 1743944400000, awh::chrono_t::type_t::HOUR}),
		IterationChronoTestParameter({1743943021520, 1743943080000, awh::chrono_t::type_t::MINUTES}),
		IterationChronoTestParameter({1743943021520, 1743943022000, awh::chrono_t::type_t::SECONDS}),
		IterationChronoTestParameter({1712410208000, 1735689600000, awh::chrono_t::type_t::YEAR}),
		IterationChronoTestParameter({1712410208000, 1714521600000, awh::chrono_t::type_t::MONTH}),
		IterationChronoTestParameter({1712410208000, 1712534400000, awh::chrono_t::type_t::WEEK}),
		IterationChronoTestParameter({1712410208000, 1712448000000, awh::chrono_t::type_t::DAY}),
		IterationChronoTestParameter({1712410208000, 1712412000000, awh::chrono_t::type_t::HOUR}),
		IterationChronoTestParameter({1712410208000, 1712410260000, awh::chrono_t::type_t::MINUTES}),
		IterationChronoTestParameter({1712410208000, 1712410209000, awh::chrono_t::type_t::SECONDS}),
		IterationChronoTestParameter({1704891600000, 1735689600000, awh::chrono_t::type_t::YEAR}),
		IterationChronoTestParameter({1704891600000, 1706745600000, awh::chrono_t::type_t::MONTH}),
		IterationChronoTestParameter({1704891600000, 1705276800000, awh::chrono_t::type_t::WEEK}),
		IterationChronoTestParameter({1704891600000, 1704931200000, awh::chrono_t::type_t::DAY}),
		IterationChronoTestParameter({1704891600000, 1704895200000, awh::chrono_t::type_t::HOUR}),
		IterationChronoTestParameter({1704891600000, 1704891660000, awh::chrono_t::type_t::MINUTES}),
		IterationChronoTestParameter({1704891600000, 1704891601000, awh::chrono_t::type_t::SECONDS}),
		IterationChronoTestParameter({1707570000000, 1735689600000, awh::chrono_t::type_t::YEAR}),
		IterationChronoTestParameter({1707570000000, 1709251200000, awh::chrono_t::type_t::MONTH}),
		IterationChronoTestParameter({1707570000000, 1707696000000, awh::chrono_t::type_t::WEEK}),
		IterationChronoTestParameter({1707570000000, 1707609600000, awh::chrono_t::type_t::DAY}),
		IterationChronoTestParameter({1707570000000, 1707573600000, awh::chrono_t::type_t::HOUR}),
		IterationChronoTestParameter({1707570000000, 1707570060000, awh::chrono_t::type_t::MINUTES}),
		IterationChronoTestParameter({1707570000000, 1707570001000, awh::chrono_t::type_t::SECONDS})
	)
);

/**
 * @brief Тестирование метода получения начала позиции указанной даты
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, BeginParameterizedFixture,
	::testing::Values(
		IterationChronoTestParameter({1743943021520, 1735689600000, awh::chrono_t::type_t::YEAR}),
		IterationChronoTestParameter({1743943021520, 1743465600000, awh::chrono_t::type_t::MONTH}),
		IterationChronoTestParameter({1743943021520, 1743379200000, awh::chrono_t::type_t::WEEK}),
		IterationChronoTestParameter({1743943021520, 1743897600000, awh::chrono_t::type_t::DAY}),
		IterationChronoTestParameter({1743943021520, 1743940800000, awh::chrono_t::type_t::HOUR}),
		IterationChronoTestParameter({1743943021520, 1743943020000, awh::chrono_t::type_t::MINUTES}),
		IterationChronoTestParameter({1743943021520, 1743943021000, awh::chrono_t::type_t::SECONDS}),
		IterationChronoTestParameter({1712410208000, 1704067200000, awh::chrono_t::type_t::YEAR}),
		IterationChronoTestParameter({1712410208000, 1711929600000, awh::chrono_t::type_t::MONTH}),
		IterationChronoTestParameter({1712410208000, 1711929600000, awh::chrono_t::type_t::WEEK}),
		IterationChronoTestParameter({1712410208000, 1712361600000, awh::chrono_t::type_t::DAY}),
		IterationChronoTestParameter({1712410208000, 1712408400000, awh::chrono_t::type_t::HOUR}),
		IterationChronoTestParameter({1712410208000, 1712410200000, awh::chrono_t::type_t::MINUTES}),
		IterationChronoTestParameter({1712410208000, 1712410208000, awh::chrono_t::type_t::SECONDS}),
		IterationChronoTestParameter({1704891600000, 1704067200000, awh::chrono_t::type_t::YEAR}),
		IterationChronoTestParameter({1704891600000, 1704067200000, awh::chrono_t::type_t::MONTH}),
		IterationChronoTestParameter({1704891600000, 1704672000000, awh::chrono_t::type_t::WEEK}),
		IterationChronoTestParameter({1704891600000, 1704844800000, awh::chrono_t::type_t::DAY}),
		IterationChronoTestParameter({1704891600000, 1704891600000, awh::chrono_t::type_t::HOUR}),
		IterationChronoTestParameter({1704891600000, 1704891600000, awh::chrono_t::type_t::MINUTES}),
		IterationChronoTestParameter({1704891600000, 1704891600000, awh::chrono_t::type_t::SECONDS}),
		IterationChronoTestParameter({1707570000000, 1704067200000, awh::chrono_t::type_t::YEAR}),
		IterationChronoTestParameter({1707570000000, 1706745600000, awh::chrono_t::type_t::MONTH}),
		IterationChronoTestParameter({1707570000000, 1707091200000, awh::chrono_t::type_t::WEEK}),
		IterationChronoTestParameter({1707570000000, 1707523200000, awh::chrono_t::type_t::DAY}),
		IterationChronoTestParameter({1707570000000, 1707570000000, awh::chrono_t::type_t::HOUR}),
		IterationChronoTestParameter({1707570000000, 1707570000000, awh::chrono_t::type_t::MINUTES}),
		IterationChronoTestParameter({1707570000000, 1707570000000, awh::chrono_t::type_t::SECONDS})
	)
);

/**
 * @brief Структура параметров тестирования метода актуализации прошедшего и оставшегося времени
 *
 */
struct ActualChronoTestParameter {
	// Дата для актуализации
	uint64_t date = 0;
	// Ожидаемый результат
	uint64_t result = 0;
	// Значение для актуализации
	awh::chrono_t::type_t value;
	// Тип результата
	awh::chrono_t::type_t type;
	// Тип актуализации
	awh::chrono_t::actual_t actual;
};
/**
 * @brief Структура параметров метода актуализации прошедшего и оставшегося времени
 *
 */
struct ActualParameterizedFixture : public ChronoFixture, public ::testing::WithParamInterface <ActualChronoTestParameter> {
	// Параметры теста
	ActualChronoTestParameter _parameter = GetParam();
};

/**
 * @brief Тест метода актуализации прошедшего и оставшегося времени
 *
 */
TEST_P(ActualParameterizedFixture, ExecutionActualChronoTest){
	// Выполняем проверку результата
	ASSERT_EQ(this->_chrono->actual(this->_parameter.date, this->_parameter.value, this->_parameter.type, this->_parameter.actual), this->_parameter.result);
}

/**
 * @brief Тестирование метода актуализации прошедшего и оставшегося времени
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, ActualParameterizedFixture,
	::testing::Values(
		// Сколько осталось месяцев в году, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 8, awh::chrono_t::type_t::MONTH, awh::chrono_t::type_t::YEAR, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось недель в году, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 38, awh::chrono_t::type_t::WEEK, awh::chrono_t::type_t::YEAR, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось дней в году, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 269, awh::chrono_t::type_t::DAY, awh::chrono_t::type_t::YEAR, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось часов в году, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 6467, awh::chrono_t::type_t::HOUR, awh::chrono_t::type_t::YEAR, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось минут в году, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 388042, awh::chrono_t::type_t::MINUTES, awh::chrono_t::type_t::YEAR, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось секунд в году, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 23282578, awh::chrono_t::type_t::SECONDS, awh::chrono_t::type_t::YEAR, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось миллисекунд в году, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 23282578479, awh::chrono_t::type_t::MILLISECONDS, awh::chrono_t::type_t::YEAR, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось микросекунд в году, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 23282578479000, awh::chrono_t::type_t::MICROSECONDS, awh::chrono_t::type_t::YEAR, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось наносекунд в году, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 23282578479000000, awh::chrono_t::type_t::NANOSECONDS, awh::chrono_t::type_t::YEAR, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось недель в месяце, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 4, awh::chrono_t::type_t::WEEK, awh::chrono_t::type_t::MONTH, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось дней в месяце, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 24, awh::chrono_t::type_t::DAY, awh::chrono_t::type_t::MONTH, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось часов в месяце, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 587, awh::chrono_t::type_t::HOUR, awh::chrono_t::type_t::MONTH, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось минут в месяце, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 35242, awh::chrono_t::type_t::MINUTES, awh::chrono_t::type_t::MONTH, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось секунд в месяце, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 2114578, awh::chrono_t::type_t::SECONDS, awh::chrono_t::type_t::MONTH, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось миллисекунд в месяце, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 2114578480, awh::chrono_t::type_t::MILLISECONDS, awh::chrono_t::type_t::MONTH, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось микросекунд в месяце, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 2114578480000, awh::chrono_t::type_t::MICROSECONDS, awh::chrono_t::type_t::MONTH, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось наносекунд в месяце, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 2114578480000000, awh::chrono_t::type_t::NANOSECONDS, awh::chrono_t::type_t::MONTH, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось дней в неделе, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 0, awh::chrono_t::type_t::DAY, awh::chrono_t::type_t::WEEK, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось часов в неделе, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 11, awh::chrono_t::type_t::HOUR, awh::chrono_t::type_t::WEEK, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось минут в неделе, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 682, awh::chrono_t::type_t::MINUTES, awh::chrono_t::type_t::WEEK, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось секунд в неделе, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 40978, awh::chrono_t::type_t::SECONDS, awh::chrono_t::type_t::WEEK, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось миллисекунд в неделе, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 40978480, awh::chrono_t::type_t::MILLISECONDS, awh::chrono_t::type_t::WEEK, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось микросекунд в неделе, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 40978480000, awh::chrono_t::type_t::MICROSECONDS, awh::chrono_t::type_t::WEEK, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось наносекунд в неделе, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 40978480000000, awh::chrono_t::type_t::NANOSECONDS, awh::chrono_t::type_t::WEEK, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось часов в дне, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 11, awh::chrono_t::type_t::HOUR, awh::chrono_t::type_t::DAY, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось минут в дне, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 682, awh::chrono_t::type_t::MINUTES, awh::chrono_t::type_t::DAY, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось секунд в дне, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 40978, awh::chrono_t::type_t::SECONDS, awh::chrono_t::type_t::DAY, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось миллисекунд в дне, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 40978480, awh::chrono_t::type_t::MILLISECONDS, awh::chrono_t::type_t::DAY, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось микросекунд в дне, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 40978480000, awh::chrono_t::type_t::MICROSECONDS, awh::chrono_t::type_t::DAY, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось наносекунд в дне, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 40978480000000, awh::chrono_t::type_t::NANOSECONDS, awh::chrono_t::type_t::DAY, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось минут в часе, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 22, awh::chrono_t::type_t::MINUTES, awh::chrono_t::type_t::HOUR, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось секунд в часе, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 1378, awh::chrono_t::type_t::SECONDS, awh::chrono_t::type_t::HOUR, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось миллисекунд в часе, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 1378480, awh::chrono_t::type_t::MILLISECONDS, awh::chrono_t::type_t::HOUR, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось микросекунд в часе, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 1378480000, awh::chrono_t::type_t::MICROSECONDS, awh::chrono_t::type_t::HOUR, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось наносекунд в часе, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 1378480000000, awh::chrono_t::type_t::NANOSECONDS, awh::chrono_t::type_t::HOUR, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось секунд в минуте, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 58, awh::chrono_t::type_t::SECONDS, awh::chrono_t::type_t::MINUTES, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось миллисекунд в минуте, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 58480, awh::chrono_t::type_t::MILLISECONDS, awh::chrono_t::type_t::MINUTES, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось микросекунд в минуте, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 58480000, awh::chrono_t::type_t::MICROSECONDS, awh::chrono_t::type_t::MINUTES, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось наносекунд в минуте, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 58480000000, awh::chrono_t::type_t::NANOSECONDS, awh::chrono_t::type_t::MINUTES, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось миллисекунд в секунде, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 480, awh::chrono_t::type_t::MILLISECONDS, awh::chrono_t::type_t::SECONDS, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось микросекунд в секунде, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 480000, awh::chrono_t::type_t::MICROSECONDS, awh::chrono_t::type_t::SECONDS, awh::chrono_t::actual_t::LEFT}),
		// Сколько осталось наносекунд в секунде, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 480000000, awh::chrono_t::type_t::NANOSECONDS, awh::chrono_t::type_t::SECONDS, awh::chrono_t::actual_t::LEFT}),
		// Сколько прошло месяцев в году, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 3, awh::chrono_t::type_t::MONTH, awh::chrono_t::type_t::YEAR, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло недель в году, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 14, awh::chrono_t::type_t::WEEK, awh::chrono_t::type_t::YEAR, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло дней в году, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 95, awh::chrono_t::type_t::DAY, awh::chrono_t::type_t::YEAR, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло часов в году, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 2292, awh::chrono_t::type_t::HOUR, awh::chrono_t::type_t::YEAR, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло минут в году, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 137557, awh::chrono_t::type_t::MINUTES, awh::chrono_t::type_t::YEAR, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло секунд в году, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 8253421, awh::chrono_t::type_t::SECONDS, awh::chrono_t::type_t::YEAR, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло миллисекунд в году, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 8253421520, awh::chrono_t::type_t::MILLISECONDS, awh::chrono_t::type_t::YEAR, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло микросекунд в году, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 8253421520000, awh::chrono_t::type_t::MICROSECONDS, awh::chrono_t::type_t::YEAR, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло наносекунд в году, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 8253421520000000, awh::chrono_t::type_t::NANOSECONDS, awh::chrono_t::type_t::YEAR, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло недель в месяце, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 1, awh::chrono_t::type_t::WEEK, awh::chrono_t::type_t::MONTH, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло дней в месяце, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 5, awh::chrono_t::type_t::DAY, awh::chrono_t::type_t::MONTH, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло часов в месяце, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 132, awh::chrono_t::type_t::HOUR, awh::chrono_t::type_t::MONTH, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло минут в месяце, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 7957, awh::chrono_t::type_t::MINUTES, awh::chrono_t::type_t::MONTH, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло секунд в месяце, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 477421, awh::chrono_t::type_t::SECONDS, awh::chrono_t::type_t::MONTH, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло миллисекунд в месяце, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 477421520, awh::chrono_t::type_t::MILLISECONDS, awh::chrono_t::type_t::MONTH, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло микросекунд в месяце, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 477421520000, awh::chrono_t::type_t::MICROSECONDS, awh::chrono_t::type_t::MONTH, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло наносекунд в месяце, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 477421520000000, awh::chrono_t::type_t::NANOSECONDS, awh::chrono_t::type_t::MONTH, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло дней в неделе, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 6, awh::chrono_t::type_t::DAY, awh::chrono_t::type_t::WEEK, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло часов в неделе, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 156, awh::chrono_t::type_t::HOUR, awh::chrono_t::type_t::WEEK, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло минут в неделе, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 9397, awh::chrono_t::type_t::MINUTES, awh::chrono_t::type_t::WEEK, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло секунд в неделе, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 563821, awh::chrono_t::type_t::SECONDS, awh::chrono_t::type_t::WEEK, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло миллисекунд в неделе, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 563821520, awh::chrono_t::type_t::MILLISECONDS, awh::chrono_t::type_t::WEEK, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло микросекунд в неделе, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 563821520000, awh::chrono_t::type_t::MICROSECONDS, awh::chrono_t::type_t::WEEK, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло наносекунд в неделе, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 563821520000000, awh::chrono_t::type_t::NANOSECONDS, awh::chrono_t::type_t::WEEK, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло часов в дне, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 12, awh::chrono_t::type_t::HOUR, awh::chrono_t::type_t::DAY, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло минут в дне, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 757, awh::chrono_t::type_t::MINUTES, awh::chrono_t::type_t::DAY, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло секунд в дне, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 45421, awh::chrono_t::type_t::SECONDS, awh::chrono_t::type_t::DAY, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло миллисекунд в дне, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 45421520, awh::chrono_t::type_t::MILLISECONDS, awh::chrono_t::type_t::DAY, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло микросекунд в дне, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 45421520000, awh::chrono_t::type_t::MICROSECONDS, awh::chrono_t::type_t::DAY, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло наносекунд в дне, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 45421520000000, awh::chrono_t::type_t::NANOSECONDS, awh::chrono_t::type_t::DAY, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло минут в часе, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 37, awh::chrono_t::type_t::MINUTES, awh::chrono_t::type_t::HOUR, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло секунд в часе, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 2221, awh::chrono_t::type_t::SECONDS, awh::chrono_t::type_t::HOUR, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло миллисекунд в часе, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 2221520, awh::chrono_t::type_t::MILLISECONDS, awh::chrono_t::type_t::HOUR, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло микросекунд в часе, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 2221520000, awh::chrono_t::type_t::MICROSECONDS, awh::chrono_t::type_t::HOUR, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло наносекунд в часе, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 2221520000000, awh::chrono_t::type_t::NANOSECONDS, awh::chrono_t::type_t::HOUR, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло секунд в минуте, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 1, awh::chrono_t::type_t::SECONDS, awh::chrono_t::type_t::MINUTES, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло миллисекунд в минуте, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 1520, awh::chrono_t::type_t::MILLISECONDS, awh::chrono_t::type_t::MINUTES, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло микросекунд в минуте, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 1520000, awh::chrono_t::type_t::MICROSECONDS, awh::chrono_t::type_t::MINUTES, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло наносекунд в минуте, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 1520000000, awh::chrono_t::type_t::NANOSECONDS, awh::chrono_t::type_t::MINUTES, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло миллисекунд в секунде, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 520, awh::chrono_t::type_t::MILLISECONDS, awh::chrono_t::type_t::SECONDS, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло микросекунд в секунде, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 520000, awh::chrono_t::type_t::MICROSECONDS, awh::chrono_t::type_t::SECONDS, awh::chrono_t::actual_t::PASSED}),
		// Сколько прошло наносекунд в секунде, относительно даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
		ActualChronoTestParameter({1743943021520, 520000000, awh::chrono_t::type_t::NANOSECONDS, awh::chrono_t::type_t::SECONDS, awh::chrono_t::actual_t::PASSED})
	)
);

/**
 * @brief Структура параметров тестирования метода смещения на указанное количество единиц времени
 *
 */
struct OffsetChronoTestParameter {
	// Дата в миллисекундах с эпохи
	uint64_t date = 0;
	// Значение смещения
	uint64_t value = 0;
	// Ожидаемый результат
	uint64_t result = 0;
	// Тип единицы времени
	awh::chrono_t::type_t type;
	// Направление смещения
	awh::chrono_t::offset_t offset;
};

/**
 * @brief Структура параметров метода смещения на указанное количество единиц времени
 *
 */
struct OffsetParameterizedFixture : public ChronoFixture, public ::testing::WithParamInterface <OffsetChronoTestParameter> {
	// Параметр теста
	OffsetChronoTestParameter _parameter = GetParam();
};

/**
 * @brief Тест метода смещения на указанное количество единиц времени
 *
 */
TEST_P(OffsetParameterizedFixture, ExecutionOffsetChronoTest){
	// Проверка результата смещения даты
	ASSERT_EQ(this->_chrono->offset(this->_parameter.date, this->_parameter.value, this->_parameter.type, this->_parameter.offset), this->_parameter.result);
}

/**
 * @brief Тестирование метода смещения на указанное количество единиц времени
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, OffsetParameterizedFixture,
	::testing::Values(
		// Увеличиваем дату (Sun, 06 Apr 2025 15:37:01 UTC+3) на 30 лет
		OffsetChronoTestParameter({1743943021520, 30, 2690627821520, awh::chrono_t::type_t::YEAR, awh::chrono_t::offset_t::INCREMENT}),
		// Увеличиваем дату (Sun, 06 Apr 2025 15:37:01 UTC+3) на 31 месяц
		OffsetChronoTestParameter({1743943021520, 31, 1825504621520, awh::chrono_t::type_t::MONTH, awh::chrono_t::offset_t::INCREMENT}),
		// Увеличиваем дату (Sun, 06 Apr 2025 15:37:01 UTC+3) на 5 недель
		OffsetChronoTestParameter({1743943021520, 5, 1746967021520, awh::chrono_t::type_t::WEEK, awh::chrono_t::offset_t::INCREMENT}),
		// Увеличиваем дату (Sun, 06 Apr 2025 15:37:01 UTC+3) на 3 дня
		OffsetChronoTestParameter({1743943021520, 3, 1744202221520, awh::chrono_t::type_t::DAY, awh::chrono_t::offset_t::INCREMENT}),
		// Увеличиваем дату (Sun, 06 Apr 2025 15:37:01 UTC+3) на 6 часов
		OffsetChronoTestParameter({1743943021520, 6, 1743964621520, awh::chrono_t::type_t::HOUR, awh::chrono_t::offset_t::INCREMENT}),
		// Увеличиваем дату (Sun, 06 Apr 2025 15:37:01 UTC+3) на 15 минут
		OffsetChronoTestParameter({1743943021520, 15, 1743943921520, awh::chrono_t::type_t::MINUTES, awh::chrono_t::offset_t::INCREMENT}),
		// Увеличиваем дату (Sun, 06 Apr 2025 15:37:01 UTC+3) на 64 секунд
		OffsetChronoTestParameter({1743943021520, 64, 1743943085520, awh::chrono_t::type_t::SECONDS, awh::chrono_t::offset_t::INCREMENT}),
		// Увеличиваем дату (Sun, 06 Apr 2025 15:37:01 UTC+3) на 586432 миллисекунд
		OffsetChronoTestParameter({1743943021520, 586432, 1743943607952, awh::chrono_t::type_t::MILLISECONDS, awh::chrono_t::offset_t::INCREMENT}),
		// Уменьшаем дату (Sun, 06 Apr 2025 15:37:01 UTC+3) на 30 лет
		OffsetChronoTestParameter({1743943021520, 30, 797171821520, awh::chrono_t::type_t::YEAR, awh::chrono_t::offset_t::DECREMENT}),
		// Уменьшаем дату (Sun, 06 Apr 2025 15:37:01 UTC+3) на 31 месяц
		OffsetChronoTestParameter({1743943021520, 31, 1662467821520, awh::chrono_t::type_t::MONTH, awh::chrono_t::offset_t::DECREMENT}),
		// Уменьшаем дату (Sun, 06 Apr 2025 15:37:01 UTC+3) на 5 недель
		OffsetChronoTestParameter({1743943021520, 5, 1740919021520, awh::chrono_t::type_t::WEEK, awh::chrono_t::offset_t::DECREMENT}),
		// Уменьшаем дату (Sun, 06 Apr 2025 15:37:01 UTC+3) на 3 дня
		OffsetChronoTestParameter({1743943021520, 3, 1743683821520, awh::chrono_t::type_t::DAY, awh::chrono_t::offset_t::DECREMENT}),
		// Уменьшаем дату (Sun, 06 Apr 2025 15:37:01 UTC+3) на 6 часов
		OffsetChronoTestParameter({1743943021520, 6, 1743921421520, awh::chrono_t::type_t::HOUR, awh::chrono_t::offset_t::DECREMENT}),
		// Уменьшаем дату (Sun, 06 Apr 2025 15:37:01 UTC+3) на 15 минут
		OffsetChronoTestParameter({1743943021520, 15, 1743942121520, awh::chrono_t::type_t::MINUTES, awh::chrono_t::offset_t::DECREMENT}),
		// Уменьшаем дату (Sun, 06 Apr 2025 15:37:01 UTC+3) на 64 секунд
		OffsetChronoTestParameter({1743943021520, 64, 1743942957520, awh::chrono_t::type_t::SECONDS, awh::chrono_t::offset_t::DECREMENT}),
		// Уменьшаем дату (Sun, 06 Apr 2025 15:37:01 UTC+3) на 586432 миллисекунд
		OffsetChronoTestParameter({1743943021520, 586432, 1743942435088, awh::chrono_t::type_t::MILLISECONDS, awh::chrono_t::offset_t::DECREMENT}),
		// Увеличиваем дату января високосного года (Mon, 15 Jan 2024 12:00:00 UTC) на 1 год
		OffsetChronoTestParameter({1705320000000, 1, 1736942400000, awh::chrono_t::type_t::YEAR, awh::chrono_t::offset_t::INCREMENT}),
		// Увеличиваем дату февраля високосного года (Sat, 10 Feb 2024 12:00:00 UTC) на 1 год
		OffsetChronoTestParameter({1707566400000, 1, 1739188800000, awh::chrono_t::type_t::YEAR, awh::chrono_t::offset_t::INCREMENT}),
		// Уменьшаем дату апреля (Sun, 06 Apr 2025 12:00:00 UTC) на 1 год до високосного года
		OffsetChronoTestParameter({1743940800000, 1, 1712404800000, awh::chrono_t::type_t::YEAR, awh::chrono_t::offset_t::DECREMENT}),
		// Уменьшаем дату января (Wed, 15 Jan 2025 12:00:00 UTC) на 1 год до високосного года
		OffsetChronoTestParameter({1736942400000, 1, 1705320000000, awh::chrono_t::type_t::YEAR, awh::chrono_t::offset_t::DECREMENT}),
		// Прибавляем 1 месяц к 31 января 2025 (Fri, 31 Jan 2025 12:00:00 UTC) - ограничение до 28 февраля
		OffsetChronoTestParameter({1738324800000, 1, 1740744000000, awh::chrono_t::type_t::MONTH, awh::chrono_t::offset_t::INCREMENT}),
		// Прибавляем 1 месяц к 31 января 2024 (Wed, 31 Jan 2024 12:00:00 UTC) - ограничение до 29 февраля
		OffsetChronoTestParameter({1706702400000, 1, 1709208000000, awh::chrono_t::type_t::MONTH, awh::chrono_t::offset_t::INCREMENT}),
		// Прибавляем 1 месяц к 31 марта 2025 (Mon, 31 Mar 2025 12:00:00 UTC) - ограничение до 30 апреля
		OffsetChronoTestParameter({1743422400000, 1, 1746014400000, awh::chrono_t::type_t::MONTH, awh::chrono_t::offset_t::INCREMENT}),
		// Вычитаем 1 месяц из 31 марта 2025 (Mon, 31 Mar 2025 12:00:00 UTC) - ограничение до 28 февраля
		OffsetChronoTestParameter({1743422400000, 1, 1740744000000, awh::chrono_t::type_t::MONTH, awh::chrono_t::offset_t::DECREMENT}),
		// Прибавляем 2 месяца к 31 декабря 2025 (Wed, 31 Dec 2025 12:00:00 UTC) - ограничение до 28 февраля 2026
		OffsetChronoTestParameter({1767182400000, 2, 1772280000000, awh::chrono_t::type_t::MONTH, awh::chrono_t::offset_t::INCREMENT})
	)
);

/**
 * @brief Тестируем метод извлечения статуса 12-и часового формата времени
 *
 */
TEST_F(ChronoFixture, H12ChronoTest){
	// Проверяем дату (Sun, 06 Apr 2025 15:37:01 UTC+3)
	ASSERT_EQ(this->_chrono->h12(1743943021520), awh::chrono_t::h12_t::PM);
	// Проверяем дату (Sun, 06 Apr 2025 10:37:01 UTC+3)
	ASSERT_EQ(this->_chrono->h12(1743925021121), awh::chrono_t::h12_t::AM);
}

/**
 * @brief Тестируем метод извлечения значения года
 *
 */
TEST_F(ChronoFixture, YearChronoTest){
	// Проверяем дату (Sun, 06 Apr 2025 15:37:01 UTC+3)
	ASSERT_EQ(this->_chrono->year(1743943021520), 2025);
	// Проверяем дату (Sun, 06 Apr 2026 15:37:01 UTC+3)
	ASSERT_EQ(this->_chrono->year(1775479021000), 2026);
	// Проверяем дату (Sun, 06 Apr 2023 15:37:01 UTC+3)
	ASSERT_EQ(this->_chrono->year(1680784621000), 2023);
	// Проверяем дату (Sun, 06 Apr 2029 15:37:01 UTC+3)
	ASSERT_EQ(this->_chrono->year(1870173421000), 2029);
}

/**
 * @brief Тестируем метод проверки действия летнего времени (DST по правилам США/Канады)
 *
 */
TEST_F(ChronoFixture, DSTChronoTest){
	// Зима (Wed, 15 Jan 2025 12:00:00 UTC) - летнее время не действует
	ASSERT_FALSE(this->_chrono->dst(1736942400000));
	// Март до 2-го воскресенья (Sat, 01 Mar 2025 12:00:00 UTC) - не действует
	ASSERT_FALSE(this->_chrono->dst(1740830400000));
	// 2-е воскресенье марта, после 02:00 (Sun, 09 Mar 2025 12:00:00 UTC) - действует
	ASSERT_TRUE(this->_chrono->dst(1741521600000));
	// 2-е воскресенье марта, до 02:00 (Sun, 09 Mar 2025 01:00:00 UTC) - ещё не действует
	ASSERT_FALSE(this->_chrono->dst(1741482000000));
	// Лето (Tue, 15 Jul 2025 12:00:00 UTC) - действует
	ASSERT_TRUE(this->_chrono->dst(1752580800000));
	// 1-е воскресенье ноября, до 02:00 (Sun, 02 Nov 2025 01:00:00 UTC) - ещё действует
	ASSERT_TRUE(this->_chrono->dst(1762045200000));
	// 1-е воскресенье ноября, после 02:00 (Sun, 02 Nov 2025 12:00:00 UTC) - уже не действует
	ASSERT_FALSE(this->_chrono->dst(1762084800000));
	// Конец ноября (Sat, 15 Nov 2025 12:00:00 UTC) - не действует
	ASSERT_FALSE(this->_chrono->dst(1763208000000));
	// Зима (Thu, 25 Dec 2025 12:00:00 UTC) - не действует
	ASSERT_FALSE(this->_chrono->dst(1766664000000));
}

/**
 * @brief Тестируем метод проверки является ли год високосным
 *
 */
TEST_F(ChronoFixture, LeapChronoTest){
	// Проверяем год 2025
	ASSERT_FALSE(this->_chrono->leap(static_cast <uint16_t> (2025)));
	// Проверяем дату 2024
	ASSERT_TRUE(this->_chrono->leap(static_cast <uint16_t> (2024)));
	// Проверяем дату 2026
	ASSERT_FALSE(this->_chrono->leap(static_cast <uint16_t> (2026)));
	// Проверяем дату 2028
	ASSERT_TRUE(this->_chrono->leap(static_cast <uint16_t> (2028)));
	// Проверяем дату (Sun, 06 Apr 2025 15:37:01 UTC+3)
	ASSERT_FALSE(this->_chrono->leap(static_cast <uint64_t> (1743943021520)));
	// Проверяем дату (Sun, 06 Apr 2024 15:37:01 UTC+3)
	ASSERT_TRUE(this->_chrono->leap(static_cast <uint64_t> (1712407021000)));
	// Проверяем дату (Sun, 06 Apr 2026 15:37:01 UTC+3)
	ASSERT_FALSE(this->_chrono->leap(static_cast <uint64_t> (1775479021000)));
	// Проверяем дату (Sun, 06 Apr 2028 15:37:01 UTC+3)
	ASSERT_TRUE(this->_chrono->leap(static_cast <uint64_t> (1838637421000)));
}

/**
 * @brief Тестируем метод установки и извлечения данных
 *
 */
TEST_F(ChronoFixture, GetSetChronoTest){
	// Устанавливаем Субботу как день недели
	this->_chrono->set(static_cast <uint8_t> (6), awh::chrono_t::unit_t::DAY);
	ASSERT_EQ(this->_chrono->get <uint8_t> (awh::chrono_t::unit_t::DAY, awh::chrono_t::storage_t::LOCAL), 6);
	// Устанавливаем 30-е число месяца
	this->_chrono->set(static_cast <uint8_t> (30), awh::chrono_t::unit_t::DATE);
	ASSERT_EQ(this->_chrono->get <uint8_t> (awh::chrono_t::unit_t::DATE, awh::chrono_t::storage_t::LOCAL), 30);
	// Устанавливаем 2030-й год
	this->_chrono->set(static_cast <uint16_t> (2030), awh::chrono_t::unit_t::YEAR);
	ASSERT_EQ(this->_chrono->get <uint16_t> (awh::chrono_t::unit_t::YEAR, awh::chrono_t::storage_t::LOCAL), 2030);
	// Устанавливаем 12-й час
	this->_chrono->set(static_cast <uint8_t> (12), awh::chrono_t::unit_t::HOUR);
	ASSERT_EQ(this->_chrono->get <uint8_t> (awh::chrono_t::unit_t::HOUR, awh::chrono_t::storage_t::LOCAL), 12);
	// Устанавливаем 324 дня прошедших с 1-го Января
	this->_chrono->set(static_cast <uint16_t> (324), awh::chrono_t::unit_t::DAYS);
	ASSERT_EQ(this->_chrono->get <uint16_t> (awh::chrono_t::unit_t::DAYS, awh::chrono_t::storage_t::LOCAL), 324);
	// Устанавливаем месяц Сентябрь
	this->_chrono->set(static_cast <uint8_t> (9), awh::chrono_t::unit_t::MONTH);
	ASSERT_EQ(this->_chrono->get <uint8_t> (awh::chrono_t::unit_t::MONTH, awh::chrono_t::storage_t::LOCAL), 9);
	// Устанавливаем 32-ю неделю прошедшую с 1-го Января
	this->_chrono->set(static_cast <uint8_t> (32), awh::chrono_t::unit_t::WEEKS);
	ASSERT_EQ(this->_chrono->get <uint8_t> (awh::chrono_t::unit_t::WEEKS, awh::chrono_t::storage_t::LOCAL), 32);
	// Устанавливаем смещение временной зоны
	this->_chrono->set(static_cast <int32_t> (-10800), awh::chrono_t::unit_t::OFFSET);
	ASSERT_EQ(this->_chrono->get <int32_t> (awh::chrono_t::unit_t::OFFSET, awh::chrono_t::storage_t::LOCAL), -10800);
	// Устанавливаем 59 минут
	this->_chrono->set(static_cast <uint8_t> (59), awh::chrono_t::unit_t::MINUTES);
	ASSERT_EQ(this->_chrono->get <uint8_t> (awh::chrono_t::unit_t::MINUTES, awh::chrono_t::storage_t::LOCAL), 59);
	// Устанавливаем 32 секунды
	this->_chrono->set(static_cast <uint8_t> (32), awh::chrono_t::unit_t::SECONDS);
	ASSERT_EQ(this->_chrono->get <uint8_t> (awh::chrono_t::unit_t::SECONDS, awh::chrono_t::storage_t::LOCAL), 32);
	// Устанавливаем 864 миллисекунды
	this->_chrono->set(static_cast <uint32_t> (864), awh::chrono_t::unit_t::MILLISECONDS);
	ASSERT_EQ(this->_chrono->get <uint32_t> (awh::chrono_t::unit_t::MILLISECONDS, awh::chrono_t::storage_t::LOCAL), 864);
	// Устанавливаем 891 микросекунду: поле хранит долю миллисекунды, поэтому берётся остаток
	this->_chrono->set(static_cast <uint64_t> (1392891), awh::chrono_t::unit_t::MICROSECONDS);
	ASSERT_EQ(this->_chrono->get <uint64_t> (awh::chrono_t::unit_t::MICROSECONDS, awh::chrono_t::storage_t::LOCAL), 891);
	// Устанавливаем 341839 наносекунд: поле хранит долю миллисекунды, поэтому берётся остаток
	this->_chrono->set(static_cast <uint64_t> (7892341839), awh::chrono_t::unit_t::NANOSECONDS);
	ASSERT_EQ(this->_chrono->get <uint64_t> (awh::chrono_t::unit_t::NANOSECONDS, awh::chrono_t::storage_t::LOCAL), 341839);
	// Очищаем объект
	this->_chrono->clear();
	ASSERT_NE(this->_chrono->get <uint32_t> (awh::chrono_t::unit_t::MILLISECONDS, awh::chrono_t::storage_t::LOCAL), 864);
	// Извлекаем день недели из даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
	ASSERT_EQ(this->_chrono->get <uint8_t> (1743943021520, awh::chrono_t::unit_t::DAY), 7);
	// Извлекаем число месяца из даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
	ASSERT_EQ(this->_chrono->get <uint8_t> (1743943021520, awh::chrono_t::unit_t::DATE), 6);
	// Извлекаем год из даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
	ASSERT_EQ(this->_chrono->get <uint16_t> (1743943021520, awh::chrono_t::unit_t::YEAR), 2025);
	// Извлекаем час из даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
	ASSERT_EQ(this->_chrono->get <uint8_t> (1743943021520, awh::chrono_t::unit_t::HOUR), 12);
	// Извлекаем количество дней прошедших с 1-го Января из даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
	ASSERT_EQ(this->_chrono->get <uint16_t> (1743943021520, awh::chrono_t::unit_t::DAYS), 95);
	// Извлекаем номер месяца из даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
	ASSERT_EQ(this->_chrono->get <uint8_t> (1743943021520, awh::chrono_t::unit_t::MONTH), 4);
	// Извлекаем количество недель прошедших с 1-го Января из даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
	ASSERT_EQ(this->_chrono->get <uint8_t> (1743943021520, awh::chrono_t::unit_t::WEEKS), 14);
	// Извлекаем смещение временной зоны из даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
	ASSERT_EQ(this->_chrono->get <int32_t> (1743943021520, awh::chrono_t::unit_t::OFFSET), 0);
	// Извлекаем количество минут из даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
	ASSERT_EQ(this->_chrono->get <uint8_t> (1743943021520, awh::chrono_t::unit_t::MINUTES), 37);
	// Извлекаем количество секунд из даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
	ASSERT_EQ(this->_chrono->get <uint8_t> (1743943021520, awh::chrono_t::unit_t::SECONDS), 1);
	// Извлекаем количество миллисекунд из даты (Sun, 06 Apr 2025 15:37:01 UTC+3)
	ASSERT_EQ(this->_chrono->get <uint32_t> (1743943021520, awh::chrono_t::unit_t::MILLISECONDS), 520);
}

/**
 * @brief Тестируем метод установки временной зоны
 *
 */
TEST_F(ChronoFixture, SetTimeZoneChronoTest){
	// Устанавливаем временную зону UTC
	ASSERT_EQ(this->_chrono->get <int32_t> (awh::chrono_t::unit_t::OFFSET, awh::chrono_t::storage_t::LOCAL), 0);
	// Устанавливаем временную зону GMT
	this->_chrono->setTimeZone(-10800);
	// Проверяем смещение временной зоны GMT
	ASSERT_EQ(this->_chrono->get <int32_t> (awh::chrono_t::unit_t::OFFSET, awh::chrono_t::storage_t::LOCAL), -10800);
	// Устанавливаем временную зону MSK
	this->_chrono->setTimeZone(awh::chrono_t::zone_t::MSK);
	// Проверяем смещение временной зоны MSK
	ASSERT_EQ(this->_chrono->get <int32_t> (awh::chrono_t::unit_t::OFFSET, awh::chrono_t::storage_t::LOCAL), 10800);
	// Устанавливаем временную зону YEKT
	this->_chrono->setTimeZone("MSK+03:00");
	// Проверяем смещение временной зоны YEKT
	ASSERT_EQ(this->_chrono->get <int32_t> (awh::chrono_t::unit_t::OFFSET, awh::chrono_t::storage_t::LOCAL), 21600);
	// Устанавливаем временную зону GMT-0328
	this->_chrono->setTimeZone("GMT-0328");
	// Проверяем смещение временной зоны GMT-0328
	ASSERT_EQ(this->_chrono->get <int32_t> (awh::chrono_t::unit_t::OFFSET, awh::chrono_t::storage_t::LOCAL), -12480);
	// Устанавливаем временную зону YEKT
	this->_chrono->setTimeZone("YEKT");
	// Проверяем смещение временной зоны YEKT
	ASSERT_EQ(this->_chrono->get <int32_t> (awh::chrono_t::unit_t::OFFSET, awh::chrono_t::storage_t::LOCAL), 18000);
}

/**
 * @brief Тестируем метод выполнения матчинга временной зоны
 *
 */
TEST_F(ChronoFixture, MatchTimeZoneChronoTest){
	// Проверяем матчинг временных зон MSK
	ASSERT_EQ(this->_chrono->matchTimeZone("MSK"), awh::chrono_t::zone_t::MSK);
	// Проверяем матчинг временных зон YEKT
	ASSERT_EQ(this->_chrono->matchTimeZone("YEKT"), awh::chrono_t::zone_t::YEKT);
	// Проверяем матчинг временных зон GMT
	ASSERT_EQ(this->_chrono->matchTimeZone("GMT"), awh::chrono_t::zone_t::GMT);
	// Проверяем матчинг временных зон UTC+3
	ASSERT_EQ(this->_chrono->matchTimeZone(), awh::chrono_t::zone_t::UTC);
	// Устанавливаем временную зону MSK
	this->_chrono->setTimeZone("MSK");
	// Проверяем матчинг локальной временной зоны MSK
	ASSERT_EQ(this->_chrono->matchTimeZone(awh::chrono_t::storage_t::LOCAL), awh::chrono_t::zone_t::MSK);
}

/**
 * @brief Тестируем метод извлечения смещения временной зоны
 *
 */
TEST_F(ChronoFixture, GetTimeZoneChronoTest){
	// Проверяем смещение временных зон MSK
	ASSERT_EQ(this->_chrono->getTimeZone(awh::chrono_t::zone_t::MSK), 10800);
	// Проверяем смещение временных зон YEKT
	ASSERT_EQ(this->_chrono->getTimeZone(awh::chrono_t::zone_t::YEKT), 18000);
	// Проверяем смещение временных зон GMT
	ASSERT_EQ(this->_chrono->getTimeZone(awh::chrono_t::zone_t::GMT), 0);
	// Проверяем смещение временных зон UTC+3
	ASSERT_EQ(this->_chrono->getTimeZone("MSK+03:00"), 21600);
	// Проверяем смещение временных зон GMT-0328
	ASSERT_EQ(this->_chrono->getTimeZone("GMT-0328"), -12480);
	// Проверяем смещение временных зон YEKT
	ASSERT_EQ(this->_chrono->getTimeZone("YEKT"), 18000);
	// Проверяем смещение временной зоны окружения: фикстура закрепляет её на UTC
	ASSERT_EQ(this->_chrono->getTimeZone(), 0);
}

/**
 * @brief Тестируем метод добавления своей временной зоны
 *
 */
TEST_F(ChronoFixture, AddTimeZoneChronoTest){
	// Добавляем свою временную зону ANYKS с смещением 9839 секунд
	this->_chrono->addTimeZone("ANYKS", 9839);
	// Проверяем смещение своей временной зоны ANYKS
	ASSERT_EQ(this->_chrono->getTimeZone("ANYKS"), 9839);
	// Очищаем объект
	this->_chrono->clear();
	// Очистка локальных данных реестр временных зон не затрагивает
	ASSERT_EQ(this->_chrono->getTimeZone("ANYKS"), 9839);
	// Очищаем реестр временных зон
	this->_chrono->clearTimeZones();
	// Проверяем смещение своей временной зоны ANYKS после очистки реестра
	ASSERT_EQ(this->_chrono->getTimeZone("ANYKS"), 0);
}

/**
 * @brief Тестируем метод добавления списка своих временных зон
 *
 */
TEST_F(ChronoFixture, SetTimeZonesChronoTest){
	// Добавляем список своих временных зон
	this->_chrono->setTimeZones({
		{"ANYKS", 9839},
		{"Testing", 3820}
	});
	// Проверяем смещение своей временной зоны ANYKS
	ASSERT_EQ(this->_chrono->getTimeZone("ANYKS"), 9839);
	// Проверяем смещение своей временной зоны Testing
	ASSERT_EQ(this->_chrono->getTimeZone("Testing"), 3820);
	// Очищаем объект
	this->_chrono->clear();
	// Очистка локальных данных реестр временных зон не затрагивает
	ASSERT_EQ(this->_chrono->getTimeZone("ANYKS"), 9839);
	// Очистка локальных данных реестр временных зон не затрагивает
	ASSERT_EQ(this->_chrono->getTimeZone("Testing"), 3820);
	// Очищаем реестр временных зон
	this->_chrono->clearTimeZones();
	// Проверяем смещение своей временной зоны ANYKS после очистки реестра
	ASSERT_EQ(this->_chrono->getTimeZone("ANYKS"), 0);
	// Проверяем смещение своей временной зоны Testing после очистки реестра
	ASSERT_EQ(this->_chrono->getTimeZone("Testing"), 0);
}

/**
 * @brief Тестируем метод установки и извлечения штампа времени
 *
 */
TEST_F(ChronoFixture, TimestampChronoTest){
	// Устанавливаем дату (Sun, 06 Apr 2025 15:37:01 UTC+3)
	this->_chrono->timestamp(1743943021520, awh::chrono_t::type_t::MILLISECONDS);
	// Извлекаем штамп времени в единицах измерения года
	ASSERT_EQ(this->_chrono->timestamp(awh::chrono_t::type_t::YEAR, awh::chrono_t::storage_t::LOCAL), 55);
	// Извлекаем штамп времени в единицах измерения месяца
	ASSERT_EQ(this->_chrono->timestamp(awh::chrono_t::type_t::MONTH, awh::chrono_t::storage_t::LOCAL), 663);
	// Извлекаем штамп времени в единицах измерения недели
	ASSERT_EQ(this->_chrono->timestamp(awh::chrono_t::type_t::WEEK, awh::chrono_t::storage_t::LOCAL), 2883);
	// Извлекаем штамп времени в единицах измерения дня
	ASSERT_EQ(this->_chrono->timestamp(awh::chrono_t::type_t::DAY, awh::chrono_t::storage_t::LOCAL), 20184);
	// Извлекаем штамп времени в единицах измерения часа
	ASSERT_EQ(this->_chrono->timestamp(awh::chrono_t::type_t::HOUR, awh::chrono_t::storage_t::LOCAL), 484428);
	// Извлекаем штамп времени в единицах измерения минуты
	ASSERT_EQ(this->_chrono->timestamp(awh::chrono_t::type_t::MINUTES, awh::chrono_t::storage_t::LOCAL), 29065717);
	// Извлекаем штамп времени в единицах измерения секунды
	ASSERT_EQ(this->_chrono->timestamp(awh::chrono_t::type_t::SECONDS, awh::chrono_t::storage_t::LOCAL), 1743943021);
	// Извлекаем штамп времени в единицах измерения миллисекунды
	ASSERT_EQ(this->_chrono->timestamp(awh::chrono_t::type_t::MILLISECONDS, awh::chrono_t::storage_t::LOCAL), 1743943021520);
}

/**
 * @brief Тестируем метод извлечение штампа времени в текстовом виде
 *
 */
TEST_F(ChronoFixture, GetNameTimeZoneChronoTest){
	// Проверяем текстовое представление временной зоны UTC+3
	ASSERT_EQ(this->_chrono->format(10800), std::string{"UTC+3"});
	// Проверяем текстовое представление временной зоны UTC+5
	ASSERT_EQ(this->_chrono->format(18000), std::string{"UTC+5"});
	// Проверяем текстовое представление временной зоны GMT-2
	ASSERT_EQ(this->_chrono->format(0), std::string{"UTC"});
	// Проверяем текстовое представление временной зоны UTC+6
	ASSERT_EQ(this->_chrono->format(21600), std::string{"UTC+6"});
	// Проверяем текстовое представление временной зоны GMT-0328
	ASSERT_EQ(this->_chrono->format(-12480), std::string{"UTC-3:28"});
	// Проверяем текстовое представление временной зоны MSK
	ASSERT_EQ(this->_chrono->format(awh::chrono_t::zone_t::MSK), std::string{"MSK"});
	// Проверяем текстовое представление временной зоны YEKT
	ASSERT_EQ(this->_chrono->format(awh::chrono_t::zone_t::YEKT), std::string{"YEKT"});
	// Проверяем текстовое представление временной зоны GMT
	ASSERT_EQ(this->_chrono->format(awh::chrono_t::zone_t::GMT), std::string{"GMT"});
}

/**
 * @brief Структура параметров тестирования метода форматирования даты и времени
 *
 */
struct FormatChronoTestParameter {
	// Дата в миллисекундах с эпохи
	uint64_t date = 0;
	// Форматирование даты и времени
	std::string format = "";
	// Ожидаемый результат форматирования
	std::string result = "";
};

/**
 * @brief Структура параметров метода форматирования даты и времени
 *
 */
struct FormatParameterizedFixture : public ChronoFixture, public ::testing::WithParamInterface <FormatChronoTestParameter> {
	// Параметр теста
	FormatChronoTestParameter _parameter = GetParam();
};

/**
 * @brief Тест метода форматирования даты и времени
 *
 */
TEST_P(FormatParameterizedFixture, ExecutionFormatChronoTest){
	// Проверка результата форматирования даты и времени
	ASSERT_EQ(this->_chrono->format(this->_parameter.date, this->_parameter.format), this->_parameter.result);
}

/**
 * @brief Тестирование метода форматирования даты и времени
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, FormatParameterizedFixture,
	::testing::Values(
		FormatChronoTestParameter({1743943021520, "%Y-%m-%dT%H:%M:%S.%s%o", "2025-04-06T12:37:01.520+00:00"}),
		FormatChronoTestParameter({1743943021520, "%m/%d/%Y %I:%M:%S %p", "04/06/2025 12:37:01 PM"}),
		FormatChronoTestParameter({1743943021520, "%m/%d/%y %I:%M:%S %p", "04/06/25 12:37:01 PM"}),
		FormatChronoTestParameter({1743943021520, "%Y-%m-%dT%H:%M:%S", "2025-04-06T12:37:01"}),
		FormatChronoTestParameter({1743943021520, "%m/%d/%Y %I:%M:%S %p (%s)", "04/06/2025 12:37:01 PM (520)"}),
		FormatChronoTestParameter({1743943021520, "%a %h %d %H:%M:%S.%s %Y", "Sun Apr 06 12:37:01.520 2025"}),
		FormatChronoTestParameter({1743943021520, "%d/%h/%Y:%H:%M:%S.%s %z", "06/Apr/2025:12:37:01.520 +0000"}),
		FormatChronoTestParameter({1743943021520, "%h %d %H:%M:%S", "Apr 06 12:37:01"}),
		FormatChronoTestParameter({1743943021520, "%Y-%m-%d %H:%M:%S.%s", "2025-04-06 12:37:01.520"}),
		FormatChronoTestParameter({1743943021520, "%d/%h/%Y:%H:%M:%S %z", "06/Apr/2025:12:37:01 +0000"}),
		FormatChronoTestParameter({1743943021520, "%Y/%m/%d %H:%M:%S", "2025/04/06 12:37:01"}),
		FormatChronoTestParameter({1743943021520, "%d.%m.%Y %H:%M:%S", "06.04.2025 12:37:01"}),
		FormatChronoTestParameter({1743943021520, "%m-%d %H:%M:%S.%s", "04-06 12:37:01.520"}),
		FormatChronoTestParameter({1743943021520, "%H:%M:%S", "12:37:01"}),
		FormatChronoTestParameter({1743943021520, "%a %h %e %Y %H:%M:%S %z", "Sun Apr  6 2025 12:37:01 +0000"}),
		FormatChronoTestParameter({1743943021520, "%a %h %e %H:%M:%S %z", "Sun Apr  6 12:37:01 +0000"}),
		FormatChronoTestParameter({1743943021520, "%a %h %e %Y %H:%M:%S %Z%z", "Sun Apr  6 2025 12:37:01 UTC+0000"}),
		FormatChronoTestParameter({1743943021520, "%h %d %H:%M %Z%z", "Apr 06 12:37 UTC+0000"}),
		FormatChronoTestParameter({1743943021520, "%a %h %e %H:%M:%S %W %z %j", "Sun Apr  6 12:37:01 13 +0000 096"}),
		FormatChronoTestParameter({1743943021520, "%Y%m%dT%H%M%S%z", "20250406T123701+0000"})
	)
);

/**
 * @brief Тестируем метод преобразования даты из оного формата в другой
 *
 */
TEST_F(ChronoFixture, StripChronoTest){
	// Преобразуем дату из одного формата в другой
	ASSERT_EQ(this->_chrono->strip("Sun Apr 6 2025 15:37:01 +0300", "%a %h %e %Y %H:%M:%S %z", "%d/%h/%Y:%H:%M:%S %z"), std::string{"06/Apr/2025:12:37:01 +0000"});
}
