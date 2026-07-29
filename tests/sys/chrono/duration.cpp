/**
 * @file: duration.cpp
 * @date: 2026-07-29
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Тесты работы с продолжительностью — разбор обозначения размерности времени
 *        в секунды и обратное формирование текстового обозначения продолжительности
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <vector>

/**
 * Подключаем заголовочный файл проекта
 */
#include "chrono.hpp"

/**
 * @brief Структура параметров тестирования разбора обозначения размерности
 *
 */
struct DurationParseTestParameter {
	// Разбираемое обозначение размерности времени
	std::string value = "";
	// Ожидаемое количество секунд
	double result = 0.;
};

/**
 * @brief Структура параметров теста разбора обозначения размерности
 *
 */
struct DurationParseParameterizedFixture : public ChronoFixture, public ::testing::WithParamInterface <DurationParseTestParameter> {
	// Параметры теста
	DurationParseTestParameter _parameter = GetParam();
};

/**
 * @brief Тест разбора обозначения размерности времени
 *
 */
TEST_P(DurationParseParameterizedFixture, ExecutionDurationParseChronoTest){
	// Выполняем проверку результата разбора
	ASSERT_DOUBLE_EQ(this->_chrono->seconds(this->_parameter.value), this->_parameter.result);
}

/**
 * @brief Параметры тестирования разбора обозначения размерности
 *
 * @details Месяц принимается равным средней длительности месяца по григорианскому
 *          календарю, а год - тремстам шестидесяти пяти суткам, поэтому двенадцать
 *          месяцев и год - это разные величины, и таковыми задуманы
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, DurationParseParameterizedFixture,
	::testing::Values(
		// Секунды
		DurationParseTestParameter({"1s", 1.}),
		DurationParseTestParameter({"30s", 30.}),
		DurationParseTestParameter({"0s", 0.}),
		// Минуты
		DurationParseTestParameter({"1m", 60.}),
		DurationParseTestParameter({"90m", 5400.}),
		// Часы
		DurationParseTestParameter({"1h", 3600.}),
		DurationParseTestParameter({"24h", 86400.}),
		// Сутки
		DurationParseTestParameter({"1d", 86400.}),
		DurationParseTestParameter({"7d", 604800.}),
		// Недели
		DurationParseTestParameter({"1w", 604800.}),
		DurationParseTestParameter({"4w", 2419200.}),
		// Месяцы средней длительности по григорианскому календарю
		DurationParseTestParameter({"1M", 2629746.}),
		DurationParseTestParameter({"12M", 31556952.}),
		// Годы длительностью в триста шестьдесят пять суток
		DurationParseTestParameter({"1y", 31536000.}),
		DurationParseTestParameter({"2y", 63072000.}),
		DurationParseTestParameter({"10y", 315360000.}),
		// Дробные значения через точку
		DurationParseTestParameter({"1.5h", 5400.}),
		DurationParseTestParameter({"0.5d", 43200.}),
		// Знак числа при разборе не учитывается
		DurationParseTestParameter({"-1h", 3600.}),
		// Обозначения, разбору не подлежащие
		DurationParseTestParameter({"", 0.}),
		DurationParseTestParameter({"abc", 0.}),
		// Число без размерности отвергается
		DurationParseTestParameter({"100", 0.}),
		DurationParseTestParameter({"0", 0.}),
		// Дробная часть через запятую не принимается, в отличие от записи даты
		DurationParseTestParameter({"1,5h", 0.}),
		// Размерность миллисекунд не предусмотрена
		DurationParseTestParameter({"1000ms", 0.})
	)
);

/**
 * @brief Структура параметров тестирования формирования обозначения продолжительности
 *
 */
struct DurationPrintTestParameter {
	// Продолжительность в секундах
	double value = 0.;
	// Ожидаемое текстовое обозначение
	std::string result = "";
};

/**
 * @brief Структура параметров теста формирования обозначения продолжительности
 *
 */
struct DurationPrintParameterizedFixture : public ChronoFixture, public ::testing::WithParamInterface <DurationPrintTestParameter> {
	// Параметры теста
	DurationPrintTestParameter _parameter = GetParam();
};

/**
 * @brief Тест формирования текстового обозначения продолжительности
 *
 */
TEST_P(DurationPrintParameterizedFixture, ExecutionDurationPrintChronoTest){
	// Выполняем проверку результата формирования
	ASSERT_EQ(this->_chrono->seconds(this->_parameter.value), this->_parameter.result);
}

/**
 * @brief Параметры тестирования формирования обозначения продолжительности
 *
 * @details Размерность выбирается наибольшая из тех, в которой продолжительность
 *          выражается значащим числом, а дробная часть выводится в записи без
 *          показателя степени
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, DurationPrintParameterizedFixture,
	::testing::Values(
		// Нулевая и дробная продолжительность в секундах
		DurationPrintTestParameter({0., "0s"}),
		DurationPrintTestParameter({0.5, "0.5s"}),
		DurationPrintTestParameter({1., "1s"}),
		DurationPrintTestParameter({1.5, "1.5s"}),
		DurationPrintTestParameter({59., "59s"}),
		// Границы перехода к минутам
		DurationPrintTestParameter({60., "1m"}),
		DurationPrintTestParameter({61., "1.0167m"}),
		DurationPrintTestParameter({3599., "59.9833m"}),
		// Границы перехода к часам
		DurationPrintTestParameter({3600., "1h"}),
		DurationPrintTestParameter({3725., "1.034722h"}),
		// Последняя секунда суток округляется до целого числа часов
		DurationPrintTestParameter({86399., "24h"}),
		// Границы перехода к суткам
		DurationPrintTestParameter({86400., "1d"}),
		DurationPrintTestParameter({90000., "1.04167d"}),
		// Границы перехода к неделям
		DurationPrintTestParameter({604800., "1w"}),
		DurationPrintTestParameter({1209600., "2w"}),
		// Границы перехода к месяцам
		DurationPrintTestParameter({2629746., "1M"}),
		DurationPrintTestParameter({5259492., "2M"}),
		// Границы перехода к годам
		DurationPrintTestParameter({31536000., "1y"}),
		DurationPrintTestParameter({63072000., "2y"})
	)
);

/**
 * @brief Тест кругового прогона продолжительности
 *
 * @details Обозначения целых размерностей обязаны переводиться в секунды и
 *          обратно без потерь: именно в таком виде продолжительности задаются
 *          в настройках
 *
 */
TEST_F(ChronoFixture, DurationRoundtripChronoTest){
	// Набор проверяемых обозначений целых размерностей
	const std::vector <std::string> values = {"1s", "30s", "1m", "1h", "1d", "1w", "1M", "1y", "2y"};
	/**
	 * Выполняем перебор проверяемых обозначений
	 */
	for(auto & value : values){
		// Выполняем разбор обозначения размерности в секунды
		const double seconds = this->_chrono->seconds(value);
		// Выполняем обратное формирование обозначения продолжительности
		ASSERT_EQ(this->_chrono->seconds(seconds), value) << "обозначение: " << value;
	}
}
