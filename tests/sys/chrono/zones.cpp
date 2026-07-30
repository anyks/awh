/**
 * @file: zones.cpp
 * @date: 2026-07-29
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Тесты записи и разбора смещения временной зоны — соответствие записи
 *        стандартам, обратимость каждой из трёх переменных формата и перевод
 *        обозначения зоны в смещение
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include "chrono.hpp"

/**
 * @brief Эталонный момент времени всех тестов набора
 *
 * @details Момент соответствует 2025-04-06T12:37:01Z. Все проверки берут его же и
 *          отличаются только временной зоной, в которой он записывается: гражданское
 *          время записи меняется вместе со смещением, а сам момент остаётся прежним,
 *          поэтому обратный разбор обязан давать ровно это число
 *
 */
static constexpr uint64_t ZONE_DATE = 1743943021000;

/**
 * @brief Структура параметров тестирования записи смещения временной зоны
 *
 */
struct ZoneFormatTestParameter {
	// Смещение временной зоны в секундах
	int32_t offset = 0;
	// Ожидаемая запись переменной %z (основная запись ISO 8601)
	std::string basic = "";
	// Ожидаемая запись переменной %o (расширенная запись ISO 8601)
	std::string extended = "";
	// Ожидаемая запись переменной %Z (сокращённая запись модуля)
	std::string compact = "";
};

/**
 * @brief Структура параметров теста записи смещения временной зоны
 *
 */
struct ZoneFormatParameterizedFixture : public ChronoFixture, public ::testing::WithParamInterface <ZoneFormatTestParameter> {
	// Параметры теста
	ZoneFormatTestParameter _parameter = GetParam();
};

/**
 * @brief Тест записи смещения временной зоны всеми тремя переменными формата
 *
 */
TEST_P(ZoneFormatParameterizedFixture, ExecutionZoneFormatChronoTest){
	// Выполняем проверку основной записи ISO 8601
	ASSERT_EQ(this->_chrono->format(ZONE_DATE, this->_parameter.offset, "%z"), this->_parameter.basic);
	// Выполняем проверку расширенной записи ISO 8601
	ASSERT_EQ(this->_chrono->format(ZONE_DATE, this->_parameter.offset, "%o"), this->_parameter.extended);
	// Выполняем проверку сокращённой записи модуля
	ASSERT_EQ(this->_chrono->format(ZONE_DATE, this->_parameter.offset, "%Z"), this->_parameter.compact);
	// Выполняем проверку отдельного метода формирования обозначения зоны
	ASSERT_EQ(this->_chrono->format(this->_parameter.offset), this->_parameter.compact);
}

/**
 * @brief Тест обратимости записи смещения временной зоны
 *
 * @details Запись, сформированная модулем, разбирается им же той самой переменной
 *          формата, которой была сформирована, и обязана дать исходный момент
 *          времени. Проверка ловит расхождение записи с разбором - разряд, который
 *          записывается, но не читается, и наоборот
 *
 */
TEST_P(ZoneFormatParameterizedFixture, ExecutionZoneRoundtripChronoTest){
	// Выполняем перебор всех трёх переменных обозначения временной зоны
	for(auto & specifier : {"%z", "%o", "%Z"}){
		// Формируем несущий формат записи с проверяемой переменной
		const std::string format = (std::string("%Y-%m-%dT%H:%M:%S ") + specifier);
		// Формируем запись даты в проверяемой временной зоне
		const std::string date = this->_chrono->format(ZONE_DATE, this->_parameter.offset, format);
		// Выполняем проверку обратного разбора сформированной записи
		ASSERT_EQ(this->_chrono->parse(date, format), ZONE_DATE) << date << " (" << specifier << ")";
	}
}

/**
 * @brief Тест записи смещения переменной поля time-offset стандарта RFC 3339
 *
 * @details Стандарт задаёт поле как «Z» либо как запись ±hh:mm, и переменная %i
 *          выбирает между ними по величине смещения. Ожидаемое значение выводится
 *          из уже проверенной записи %o, а не задаётся отдельно: обе записи обязаны
 *          совпадать всюду, кроме нулевого смещения
 *
 */
TEST_P(ZoneFormatParameterizedFixture, ExecutionZoneZuluChronoTest){
	// Формируем запись смещения переменной поля стандарта RFC 3339
	const std::string result = this->_chrono->format(ZONE_DATE, this->_parameter.offset, "%i");
	// Если смещение временной зоны нулевое
	if(this->_parameter.offset == 0)
		// Выполняем проверку обозначения нулевого смещения
		ASSERT_EQ(result, "Z");
	// Выполняем проверку совпадения записи с расширенной
	else ASSERT_EQ(result, this->_parameter.extended);
	// Формируем несущий формат записи с проверяемой переменной
	const std::string format = "%Y-%m-%dT%H:%M:%S%i";
	// Формируем запись даты в проверяемой временной зоне
	const std::string date = this->_chrono->format(ZONE_DATE, this->_parameter.offset, format);
	// Выполняем проверку обратного разбора сформированной записи
	ASSERT_EQ(this->_chrono->parse(date, format), ZONE_DATE) << date;
}

/**
 * @brief Параметры тестирования записи смещения временной зоны
 *
 * @details Набор смещений покрывает все встречающиеся разновидности: нулевое, целые
 *          часы обоих знаков, получасовые и четвертьчасовые зоны (India +5:30,
 *          Nepal +5:45, Newfoundland -3:30, Marquesas -9:30), оба края диапазона
 *          (Baker Island -11:00 и Kiritimati +14:00) и смещения с числом минут
 *          меньше десяти, на которых прежний расчёт через дробное число часов терял
 *          минуты. Записи %z и %o заданы по стандартам: %z - основная запись ±hhmm
 *          (POSIX strftime, RFC 5322, журнал веб-сервера), %o - расширенная ±hh:mm
 *          (RFC 3339, glibc %:z). Запись %Z стандартам не следует намеренно: она
 *          предназначена для чтения человеком
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, ZoneFormatParameterizedFixture,
	::testing::Values(
		// Нулевое смещение
		ZoneFormatTestParameter({0, "+0000", "+00:00", "UTC"}),
		// Целые часы обоих знаков
		ZoneFormatTestParameter({3600, "+0100", "+01:00", "UTC+1"}),
		ZoneFormatTestParameter({10800, "+0300", "+03:00", "UTC+3"}),
		ZoneFormatTestParameter({-14400, "-0400", "-04:00", "UTC-4"}),
		// Края диапазона временных зон
		ZoneFormatTestParameter({50400, "+1400", "+14:00", "UTC+14"}),
		ZoneFormatTestParameter({-39600, "-1100", "-11:00", "UTC-11"}),
		// Получасовые и четвертьчасовые зоны
		ZoneFormatTestParameter({19800, "+0530", "+05:30", "UTC+5:30"}),
		ZoneFormatTestParameter({20700, "+0545", "+05:45", "UTC+5:45"}),
		ZoneFormatTestParameter({-12600, "-0330", "-03:30", "UTC-3:30"}),
		ZoneFormatTestParameter({-34200, "-0930", "-09:30", "UTC-9:30"}),
		ZoneFormatTestParameter({45900, "+1245", "+12:45", "UTC+12:45"}),
		// Смещения меньше часа
		ZoneFormatTestParameter({1800, "+0030", "+00:30", "UTC+0:30"}),
		ZoneFormatTestParameter({-1800, "-0030", "-00:30", "UTC-0:30"}),
		// Смещения с числом минут меньше десяти
		ZoneFormatTestParameter({3660, "+0101", "+01:01", "UTC+1:01"}),
		ZoneFormatTestParameter({18300, "+0505", "+05:05", "UTC+5:05"}),
		ZoneFormatTestParameter({-12480, "-0328", "-03:28", "UTC-3:28"})
	)
);

/**
 * @brief Структура параметров тестирования разбора записи смещения
 *
 */
struct ZoneParseTestParameter {
	// Разбираемая запись смещения временной зоны
	std::string zone = "";
	// Ожидаемое смещение временной зоны в секундах
	int32_t offset = 0;
};

/**
 * @brief Структура параметров теста разбора записи смещения
 *
 */
struct ZoneParseParameterizedFixture : public ChronoFixture, public ::testing::WithParamInterface <ZoneParseTestParameter> {
	// Параметры теста
	ZoneParseTestParameter _parameter = GetParam();
};

/**
 * @brief Тест перевода обозначения временной зоны в смещение
 *
 */
TEST_P(ZoneParseParameterizedFixture, ExecutionZoneParseChronoTest){
	// Выполняем проверку полученного смещения временной зоны
	ASSERT_EQ(this->_chrono->getTimeZone(this->_parameter.zone), this->_parameter.offset);
}

/**
 * @brief Параметры тестирования разбора записи смещения
 *
 * @details Приём записи намеренно шире вывода: смещение принимается одной, двумя,
 *          тремя и четырьмя цифрами, с двоеточием и без, с ведущим нулём часов и
 *          без него, отдельно и следом за названием зоны. Трёхзначная запись ни
 *          одному стандарту не соответствует, но встречается в журналах, поэтому
 *          принимается наравне с остальными: первая её цифра означает часы
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, ZoneParseParameterizedFixture,
	::testing::Values(
		// Названия временных зон
		ZoneParseTestParameter({"UTC", 0}),
		ZoneParseTestParameter({"GMT", 0}),
		ZoneParseTestParameter({"Z", 0}),
		ZoneParseTestParameter({"MSK", 10800}),
		ZoneParseTestParameter({"YEKT", 18000}),
		ZoneParseTestParameter({"EST", -18000}),
		// Смещение одной и двумя цифрами - целые часы
		ZoneParseTestParameter({"+1", 3600}),
		ZoneParseTestParameter({"+01", 3600}),
		ZoneParseTestParameter({"-3", -10800}),
		ZoneParseTestParameter({"-03", -10800}),
		// Смещение четырьмя цифрами - часы и минуты
		ZoneParseTestParameter({"+0530", 19800}),
		ZoneParseTestParameter({"-0330", -12600}),
		ZoneParseTestParameter({"+0101", 3660}),
		ZoneParseTestParameter({"+0505", 18300}),
		// Смещение тремя цифрами - те же часы и минуты без ведущего нуля
		ZoneParseTestParameter({"+530", 19800}),
		ZoneParseTestParameter({"-330", -12600}),
		ZoneParseTestParameter({"+101", 3660}),
		ZoneParseTestParameter({"+030", 1800}),
		// Смещение через двоеточие, с ведущим нулём часов и без него
		ZoneParseTestParameter({"+05:30", 19800}),
		ZoneParseTestParameter({"+5:30", 19800}),
		ZoneParseTestParameter({"-03:30", -12600}),
		ZoneParseTestParameter({"-3:30", -12600}),
		ZoneParseTestParameter({"+01:01", 3660}),
		ZoneParseTestParameter({"+1:01", 3660}),
		// Смещение следом за названием временной зоны
		ZoneParseTestParameter({"UTC+3", 10800}),
		ZoneParseTestParameter({"UTC-3", -10800}),
		ZoneParseTestParameter({"UTC+5:30", 19800}),
		ZoneParseTestParameter({"UTC+05:30", 19800}),
		ZoneParseTestParameter({"UTC+0530", 19800}),
		ZoneParseTestParameter({"UTC+530", 19800}),
		ZoneParseTestParameter({"GMT-0328", -12480}),
		ZoneParseTestParameter({"GMT+0530", 19800}),
		ZoneParseTestParameter({"MSK+1", 14400})
	)
);

/**
 * @brief Структура параметров тестирования разбора чужих записей даты
 *
 */
struct ZoneRecordTestParameter {
	// Разбираемая запись даты
	std::string date = "";
	// Формат разбора записи
	std::string format = "";
};

/**
 * @brief Структура параметров теста разбора чужих записей даты
 *
 */
struct ZoneRecordParameterizedFixture : public ChronoFixture, public ::testing::WithParamInterface <ZoneRecordTestParameter> {
	// Параметры теста
	ZoneRecordTestParameter _parameter = GetParam();
};

/**
 * @brief Тест разбора записи даты, размеченной временной зоной по стандарту
 *
 */
TEST_P(ZoneRecordParameterizedFixture, ExecutionZoneRecordChronoTest){
	// Выполняем проверку результата разбора записи даты
	ASSERT_EQ(this->_chrono->parse(this->_parameter.date, this->_parameter.format), ZONE_DATE) << this->_parameter.date;
}

/**
 * @brief Параметры тестирования разбора чужих записей даты
 *
 * @details Все записи обозначают один и тот же момент времени, отличаясь стандартом
 *          и временной зоной записи. Набор проверяет, что модуль читает записи,
 *          сформированные не им: поле зоны RFC 5322 и журнала веб-сервера, смещение
 *          RFC 3339 с двоеточием и обозначение Zulu, основную запись ISO 8601 без
 *          разделителей
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, ZoneRecordParameterizedFixture,
	::testing::Values(
		// Поле зоны RFC 5322 (заголовок Date электронной почты)
		ZoneRecordTestParameter({"Sun, 06 Apr 2025 12:37:01 +0000", "%a, %d %b %Y %H:%M:%S %z"}),
		ZoneRecordTestParameter({"Sun, 06 Apr 2025 15:37:01 +0300", "%a, %d %b %Y %H:%M:%S %z"}),
		ZoneRecordTestParameter({"Sun, 06 Apr 2025 18:07:01 +0530", "%a, %d %b %Y %H:%M:%S %z"}),
		ZoneRecordTestParameter({"Sun, 06 Apr 2025 09:07:01 -0330", "%a, %d %b %Y %H:%M:%S %z"}),
		// Смещение RFC 3339 через двоеточие
		ZoneRecordTestParameter({"2025-04-06T12:37:01+00:00", "%Y-%m-%dT%H:%M:%S%o"}),
		ZoneRecordTestParameter({"2025-04-06T18:07:01+05:30", "%Y-%m-%dT%H:%M:%S%o"}),
		ZoneRecordTestParameter({"2025-04-06T09:07:01-03:30", "%Y-%m-%dT%H:%M:%S%o"}),
		// Обозначение Zulu стандарта RFC 3339
		ZoneRecordTestParameter({"2025-04-06T12:37:01Z", "%Y-%m-%dT%H:%M:%S%Z"}),
		// Основная запись ISO 8601 без разделителей
		ZoneRecordTestParameter({"20250406T123701+0000", "%Y%m%dT%H%M%S%z"}),
		ZoneRecordTestParameter({"20250406T180701+0530", "%Y%m%dT%H%M%S%z"}),
		// Журнал веб-сервера в общем формате
		ZoneRecordTestParameter({"06/Apr/2025:12:37:01 +0000", "%d/%b/%Y:%H:%M:%S %z"}),
		ZoneRecordTestParameter({"06/Apr/2025:18:07:01 +0530", "%d/%b/%Y:%H:%M:%S %z"}),
		// Запись с названием зоны вместо смещения
		ZoneRecordTestParameter({"2025-04-06T15:37:01 MSK", "%Y-%m-%dT%H:%M:%S %Z"}),
		ZoneRecordTestParameter({"2025-04-06T07:37:01 EST", "%Y-%m-%dT%H:%M:%S %Z"}),
		// Запись с названием зоны и смещением от неё
		ZoneRecordTestParameter({"2025-04-06T18:07:01 GMT+0530", "%Y-%m-%dT%H:%M:%S %Z"}),
		ZoneRecordTestParameter({"2025-04-06T16:37:01 MSK+1", "%Y-%m-%dT%H:%M:%S %Z"})
	)
);

/**
 * @brief Структура теста записи смещения временной зоны
 *
 */
struct ZoneFixture : public ChronoFixture {};

/**
 * @brief Тест соответствия записи смещения требованию разрядности
 *
 * @details Ни один стандарт не допускает записи смещения нечётным числом цифр:
 *          POSIX, RFC 5322 и основная запись ISO 8601 требуют ровно четырёх,
 *          расширенная запись и RFC 3339 - двух цифр по обе стороны двоеточия.
 *          Проверка ловит возврат к записи без ведущего нуля, при которой смещение
 *          +5:30 записывалось тремя цифрами и обратно уже не читалось
 *
 */
TEST_F(ZoneFixture, ExecutionZoneWidthChronoTest){
	// Выполняем перебор смещений с шагом в четверть часа по всему диапазону зон
	for(int32_t offset = -43200; offset <= 50400; offset += 900){
		// Формируем основную запись смещения временной зоны
		const std::string basic = this->_chrono->format(ZONE_DATE, offset, "%z");
		// Выполняем проверку разрядности основной записи
		ASSERT_EQ(basic.length(), static_cast <size_t> (5)) << basic;
		// Формируем расширенную запись смещения временной зоны
		const std::string extended = this->_chrono->format(ZONE_DATE, offset, "%o");
		// Выполняем проверку разрядности расширенной записи
		ASSERT_EQ(extended.length(), static_cast <size_t> (6)) << extended;
		// Выполняем проверку положения двоеточия в расширенной записи
		ASSERT_EQ(extended.at(3), ':') << extended;
		// Выполняем проверку знака смещения обеих записей
		ASSERT_EQ(basic.front(), ((offset < 0) ? '-' : '+')) << basic;
		ASSERT_EQ(extended.front(), ((offset < 0) ? '-' : '+')) << extended;
	}
}

/**
 * @brief Тест обратимости записи смещения по всему диапазону временных зон
 *
 * @details Диапазон перебирается с шагом в минуту, охватывая и смещения, зонами не
 *          используемые: они допустимы в записи и обязаны читаться обратно так же,
 *          как записались
 *
 */
TEST_F(ZoneFixture, ExecutionZoneSweepChronoTest){
	// Выполняем перебор смещений с шагом в минуту по всему диапазону зон
	for(int32_t offset = -43200; offset <= 50400; offset += 60){
		// Выполняем проверку обратимости основной записи смещения
		ASSERT_EQ(this->_chrono->getTimeZone(this->_chrono->format(ZONE_DATE, offset, "%z")), offset)
			<< this->_chrono->format(ZONE_DATE, offset, "%z");
		// Выполняем проверку обратимости расширенной записи смещения
		ASSERT_EQ(this->_chrono->getTimeZone(this->_chrono->format(ZONE_DATE, offset, "%o")), offset)
			<< this->_chrono->format(ZONE_DATE, offset, "%o");
		// Выполняем проверку обратимости сокращённой записи смещения
		ASSERT_EQ(this->_chrono->getTimeZone(this->_chrono->format(offset)), offset)
			<< this->_chrono->format(offset);
	}
}
