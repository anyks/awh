/**
 * @file: parse.cpp
 * @date: 2026-07-29
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Тесты разбора записи даты — проверка каждой переменной формата по отдельности
 *        внутри несущего формата, составных переменных и обозначений временной зоны
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include "chrono.hpp"

/**
 * @brief Структура параметров тестирования разбора одной переменной формата
 *
 * @details Проверяемая переменная подставляется в несущий формат, задающий все
 *          остальные поля вплоть до миллисекунд. Разбор неполного формата
 *          дополняет недостающие поля текущей датой, и такой случай эталонного
 *          значения не имеет: он проверяется отдельно, в тестах правил подстановки
 *
 */
struct ParseSpecifierTestParameter {
	// Разбираемая запись даты
	std::string date = "";
	// Формат разбора записи
	std::string format = "";
	// Ожидаемый штамп времени в миллисекундах
	uint64_t result = 0;
};

/**
 * @brief Структура параметров теста разбора одной переменной формата
 *
 */
struct ParseSpecifierParameterizedFixture : public ChronoFixture, public ::testing::WithParamInterface <ParseSpecifierTestParameter> {
	// Параметры теста
	ParseSpecifierTestParameter _parameter = GetParam();
};

/**
 * @brief Тест разбора записи с одной проверяемой переменной формата
 *
 */
TEST_P(ParseSpecifierParameterizedFixture, ExecutionParseSpecifierChronoTest){
	// Выполняем проверку результата разбора
	ASSERT_EQ(this->_chrono->parse(this->_parameter.date, this->_parameter.format), this->_parameter.result);
}

/**
 * @brief Параметры тестирования разбора переменных формата
 *
 * @details Эталонная дата всех случаев - 2025-04-06T12:37:01.520Z, она же
 *          1743943021520. Записи без миллисекунд дают 1743943021000, запись с
 *          точностью до минут - 1743943020000: поля мельче заданного обнуляются
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, ParseSpecifierParameterizedFixture,
	::testing::Values(
		// Полная запись: эталон, относительно которого проверяются остальные
		ParseSpecifierTestParameter({"2025-04-06 12:37:01.520", "%Y-%m-%d %H:%M:%S.%s", 1743943021520}),
		// Обозначения года
		ParseSpecifierTestParameter({"25-04-06 12:37:01.520", "%y-%m-%d %H:%M:%S.%s", 1743943021520}),
		ParseSpecifierTestParameter({"2025-04-06 12:37:01.520", "%G-%m-%d %H:%M:%S.%s", 1743943021520}),
		ParseSpecifierTestParameter({"25-04-06 12:37:01.520", "%g-%m-%d %H:%M:%S.%s", 1743943021520}),
		// Обозначения месяца названием
		ParseSpecifierTestParameter({"Apr 06 2025 12:37:01.520", "%b %d %Y %H:%M:%S.%s", 1743943021520}),
		ParseSpecifierTestParameter({"Apr 06 2025 12:37:01.520", "%h %d %Y %H:%M:%S.%s", 1743943021520}),
		ParseSpecifierTestParameter({"April 06 2025 12:37:01.520", "%B %d %Y %H:%M:%S.%s", 1743943021520}),
		// Число месяца без ведущего нуля
		ParseSpecifierTestParameter({"Apr 6 2025 12:37:01.520", "%h %e %Y %H:%M:%S.%s", 1743943021520}),
		// Обозначения дня недели: разбираются, но на результат не влияют
		ParseSpecifierTestParameter({"Sun Apr 06 2025 12:37:01.520", "%a %h %d %Y %H:%M:%S.%s", 1743943021520}),
		ParseSpecifierTestParameter({"Sunday Apr 06 2025 12:37:01.520", "%A %h %d %Y %H:%M:%S.%s", 1743943021520}),
		ParseSpecifierTestParameter({"7 2025-04-06 12:37:01", "%u %Y-%m-%d %H:%M:%S", 1743943021000}),
		ParseSpecifierTestParameter({"0 2025-04-06 12:37:01", "%w %Y-%m-%d %H:%M:%S", 1743943021000}),
		ParseSpecifierTestParameter({"14 2025-04-06 12:37:01", "%W %Y-%m-%d %H:%M:%S", 1743943021000}),
		// Двенадцатичасовой формат времени с указателем половины суток
		ParseSpecifierTestParameter({"2025-04-06 12:37:01.520 PM", "%Y-%m-%d %I:%M:%S.%s %p", 1743943021520}),
		ParseSpecifierTestParameter({"2025-04-06 01:37:01.520 AM", "%Y-%m-%d %I:%M:%S.%s %p", 1743903421520}),
		// Составные переменные даты
		ParseSpecifierTestParameter({"04/06/25 12:37:01.520", "%D %H:%M:%S.%s", 1743943021520}),
		ParseSpecifierTestParameter({"04/06/25 12:37:01.520", "%x %H:%M:%S.%s", 1743943021520}),
		ParseSpecifierTestParameter({"2025-04-06 12:37:01.520", "%F %H:%M:%S.%s", 1743943021520}),
		// Составные переменные времени
		ParseSpecifierTestParameter({"2025-04-06 12:37:01", "%F %T", 1743943021000}),
		ParseSpecifierTestParameter({"2025-04-06 12:37:01", "%F %X", 1743943021000}),
		ParseSpecifierTestParameter({"2025-04-06 12:37", "%F %R", 1743943020000}),
		ParseSpecifierTestParameter({"2025-04-06 12:37:01 PM", "%F %r", 1743943021000}),
		ParseSpecifierTestParameter({"Sun Apr 6 12:37:01 2025", "%c", 1743943021000}),
		// Обозначения временной зоны
		ParseSpecifierTestParameter({"2025-04-06T12:37:01+0000", "%Y-%m-%dT%H:%M:%S%z", 1743943021000}),
		ParseSpecifierTestParameter({"2025-04-06T15:37:01+0300", "%Y-%m-%dT%H:%M:%S%z", 1743943021000}),
		ParseSpecifierTestParameter({"2025-04-06T09:37:01-0300", "%Y-%m-%dT%H:%M:%S%z", 1743943021000}),
		ParseSpecifierTestParameter({"2025-04-06T15:37:01+03:00", "%Y-%m-%dT%H:%M:%S%o", 1743943021000}),
		ParseSpecifierTestParameter({"2025-04-06T12:37:01Z", "%Y-%m-%dT%H:%M:%S%Z", 1743943021000}),
		ParseSpecifierTestParameter({"2025-04-06T15:37:01MSK", "%Y-%m-%dT%H:%M:%S%Z", 1743943021000})
	)
);

/**
 * @brief Тест разбора записей, встречающихся в журналах
 *
 * @details Форматы взяты у распространённых источников записей: веб-сервера,
 *          систем сбора событий и обмена данными
 *
 */
TEST_F(ChronoFixture, ParseRealWorldChronoTest){
	// Запись журнала веб-сервера в общем формате
	ASSERT_EQ(this->_chrono->parse("[06/Apr/2025:12:37:01 +0000]", "%d/%h/%Y:%H:%M:%S %z"), 1743943021000);
	// Запись журнала веб-сервера со смещением зоны
	ASSERT_EQ(this->_chrono->parse("[06/Apr/2025:15:37:01 +0300]", "%d/%h/%Y:%H:%M:%S %z"), 1743943021000);
	// Запись обмена данными по ISO 8601 с обозначением всемирного времени
	ASSERT_EQ(this->_chrono->parse("2025-04-06T12:37:01.520Z", "%Y-%m-%dT%H:%M:%S.%s%Z"), 1743943021520);
	// Запись обмена данными по ISO 8601 со смещением через двоеточие
	ASSERT_EQ(this->_chrono->parse("2025-04-06T15:37:01.520+03:00", "%Y-%m-%dT%H:%M:%S.%s%o"), 1743943021520);
	// Сплошная запись без разделителей
	ASSERT_EQ(this->_chrono->parse("20250406T123701+0000", "%Y%m%dT%H%M%S%z"), 1743943021000);
	// Запись с запятой в качестве разделителя дробной части
	ASSERT_EQ(this->_chrono->parse("2025-04-06 12:37:01,520", "%Y-%m-%d %H:%M:%S,%s"), 1743943021520);
	// Запись в европейском порядке следования полей
	ASSERT_EQ(this->_chrono->parse("06.04.2025 12:37:01", "%d.%m.%Y %H:%M:%S"), 1743943021000);
}

/**
 * @brief Тест разбора границ календаря
 *
 * @details Проверяются переходы года и месяца, високосный день обычного
 *          високосного года и високосного века
 *
 */
TEST_F(ChronoFixture, ParseBoundariesChronoTest){
	// Начало високосного года
	ASSERT_EQ(this->_chrono->parse("2024-01-01 00:00:00.000", "%Y-%m-%d %H:%M:%S.%s"), 1704067200000);
	// Последняя миллисекунда високосного года
	ASSERT_EQ(this->_chrono->parse("2024-12-31 23:59:59.999", "%Y-%m-%d %H:%M:%S.%s"), 1735689599999);
	// Високосный день обычного високосного года
	ASSERT_EQ(this->_chrono->parse("2024-02-29 12:00:00.000", "%Y-%m-%d %H:%M:%S.%s"), 1709208000000);
	// Високосный день високосного века
	ASSERT_EQ(this->_chrono->parse("2000-02-29 12:00:00.000", "%Y-%m-%d %H:%M:%S.%s"), 951825600000);
	// Первый день марта года, не являющегося високосным
	ASSERT_EQ(this->_chrono->parse("2025-03-01 00:00:00.000", "%Y-%m-%d %H:%M:%S.%s"), 1740787200000);
	// Конец столетия
	ASSERT_EQ(this->_chrono->parse("2099-12-31 23:59:59.000", "%Y-%m-%d %H:%M:%S.%s"), 4102444799000);
}

/**
 * @brief Тест отказа разбора
 *
 * @details Пустой ввод и пустой формат разбору не подлежат, и модуль сообщает об
 *          этом нулевым штампом времени. Поведение на прочем ущербном вводе
 *          проверяется отдельно, в тестах правил подстановки
 *
 */
TEST_F(ChronoFixture, ParseRefusalChronoTest){
	// Пустая запись даты
	ASSERT_EQ(this->_chrono->parse("", "%Y-%m-%d %H:%M:%S"), 0);
	// Пустой формат разбора
	ASSERT_EQ(this->_chrono->parse("2025-04-06 12:37:01", ""), 0);
	// Пустые запись и формат
	ASSERT_EQ(this->_chrono->parse("", ""), 0);
}
