/**
 * @file: calendar.cpp
 * @date: 2026-07-29
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Тесты календарной арифметики на границах — начало и конец месяца, года и суток
 *        для високосных дней, переходов года и границ столетия
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include "chrono.hpp"

/**
 * @brief Структура параметров тестирования календарных границ
 *
 * @details Границы месяца и года считаются перебором длительностей месяцев с
 *          поправкой на високосный год, и именно там ошибка календаря проявляется
 *          раньше всего. Ожидаемые значения получены независимым эталоном
 *
 */
struct CalendarBoundaryTestParameter {
	// Проверяемая дата в миллисекундах
	uint64_t date = 0;
	// Ожидаемое начало месяца
	uint64_t beginMonth = 0;
	// Ожидаемый конец месяца
	uint64_t endMonth = 0;
	// Ожидаемое начало года
	uint64_t beginYear = 0;
	// Ожидаемый конец года
	uint64_t endYear = 0;
	// Ожидаемое начало суток
	uint64_t beginDay = 0;
};

/**
 * @brief Структура параметров теста календарных границ
 *
 */
struct CalendarBoundaryParameterizedFixture : public ChronoFixture, public ::testing::WithParamInterface <CalendarBoundaryTestParameter> {
	// Параметры теста
	CalendarBoundaryTestParameter _parameter = GetParam();
};

/**
 * @brief Тест получения начала календарных отрезков
 *
 */
TEST_P(CalendarBoundaryParameterizedFixture, ExecutionCalendarBeginChronoTest){
	// Проверяем начало суток указанной даты
	ASSERT_EQ(this->_chrono->begin(this->_parameter.date, awh::chrono_t::type_t::DAY), this->_parameter.beginDay);
	// Проверяем начало месяца указанной даты
	ASSERT_EQ(this->_chrono->begin(this->_parameter.date, awh::chrono_t::type_t::MONTH), this->_parameter.beginMonth);
	// Проверяем начало года указанной даты
	ASSERT_EQ(this->_chrono->begin(this->_parameter.date, awh::chrono_t::type_t::YEAR), this->_parameter.beginYear);
}

/**
 * @brief Тест получения конца календарных отрезков
 *
 */
TEST_P(CalendarBoundaryParameterizedFixture, ExecutionCalendarEndChronoTest){
	// Проверяем конец месяца указанной даты
	ASSERT_EQ(this->_chrono->end(this->_parameter.date, awh::chrono_t::type_t::MONTH), this->_parameter.endMonth);
	// Проверяем конец года указанной даты
	ASSERT_EQ(this->_chrono->end(this->_parameter.date, awh::chrono_t::type_t::YEAR), this->_parameter.endYear);
}

/**
 * @brief Параметры тестирования календарных границ
 *
 * @details Даты подобраны там, где календарь ошибается охотнее всего: високосный
 *          день обычного високосного года и високосного века, первый и последний
 *          миг года, первое марта невисокосного года и невисокосного столетия
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, CalendarBoundaryParameterizedFixture,
	::testing::Values(
		CalendarBoundaryTestParameter({1709208000000, 1706745600000, 1709251200000, 1704067200000, 1735689600000, 1709164800000}),
		CalendarBoundaryTestParameter({951825600000, 949363200000, 951868800000, 946684800000, 978307200000, 951782400000}),
		CalendarBoundaryTestParameter({1704067200000, 1704067200000, 1706745600000, 1704067200000, 1735689600000, 1704067200000}),
		CalendarBoundaryTestParameter({1735689599000, 1733011200000, 1735689600000, 1704067200000, 1735689600000, 1735603200000}),
		CalendarBoundaryTestParameter({1740787200000, 1740787200000, 1743465600000, 1735689600000, 1767225600000, 1740787200000}),
		CalendarBoundaryTestParameter({1767182400000, 1764547200000, 1767225600000, 1735689600000, 1767225600000, 1767139200000}),
		CalendarBoundaryTestParameter({4107542400000, 4107542400000, 4110220800000, 4102444800000, 4133980800000, 4107542400000}),
		CalendarBoundaryTestParameter({4075963200000, 4073587200000, 4076006400000, 4070908800000, 4102444800000, 4075920000000})
	)
);

/**
 * @brief Тест смещения даты на месяцы через границы года и високосного дня
 *
 * @details Смещение на месяц - единственная операция непостоянного шага, и число
 *          месяца при ней ограничивается последним днём конечного месяца
 *
 */
TEST_F(ChronoFixture, CalendarOffsetMonthChronoTest){
	// Смещение с 31 января на месяц упирается в последний день февраля високосного года
	ASSERT_EQ(this->_chrono->offset(1706659200000, 1, awh::chrono_t::type_t::MONTH, awh::chrono_t::offset_t::INCREMENT), 1709164800000);
	// Смещение с 31 января на месяц упирается в последний день февраля обычного года
	ASSERT_EQ(this->_chrono->offset(1738281600000, 1, awh::chrono_t::type_t::MONTH, awh::chrono_t::offset_t::INCREMENT), 1740700800000);
	// Смещение с 29 февраля високосного года на год даёт последний день февраля обычного
	ASSERT_EQ(this->_chrono->offset(1709164800000, 12, awh::chrono_t::type_t::MONTH, awh::chrono_t::offset_t::INCREMENT), 1740700800000);
	// Смещение через границу года вперёд
	ASSERT_EQ(this->_chrono->offset(1735603200000, 1, awh::chrono_t::type_t::MONTH, awh::chrono_t::offset_t::INCREMENT), 1738281600000);
	// Смещение через границу года назад
	ASSERT_EQ(this->_chrono->offset(1704067200000, 1, awh::chrono_t::type_t::MONTH, awh::chrono_t::offset_t::DECREMENT), 1701388800000);
}

/**
 * @brief Тест признака високосного года на границах столетий
 *
 * @details Правило григорианского календаря даёт исключение из исключения:
 *          год, кратный ста, високосным не является, если только он не кратен
 *          четырёмстам
 *
 */
TEST_F(ChronoFixture, CalendarLeapCenturyChronoTest){
	// Год, кратный четырёмстам, високосный
	ASSERT_TRUE(this->_chrono->leap(static_cast <uint16_t> (2000)));
	// Год, кратный ста, но не четырёмстам, високосным не является
	ASSERT_FALSE(this->_chrono->leap(static_cast <uint16_t> (1900)));
	// Год, кратный ста, но не четырёмстам, високосным не является
	ASSERT_FALSE(this->_chrono->leap(static_cast <uint16_t> (2100)));
	// Год, кратный ста, но не четырёмстам, високосным не является
	ASSERT_FALSE(this->_chrono->leap(static_cast <uint16_t> (2200)));
	// Год, кратный четырёмстам, високосный
	ASSERT_TRUE(this->_chrono->leap(static_cast <uint16_t> (2400)));
	// Обычный год, кратный четырём, високосный
	ASSERT_TRUE(this->_chrono->leap(static_cast <uint16_t> (2024)));
	// Обычный год, не кратный четырём, високосным не является
	ASSERT_FALSE(this->_chrono->leap(static_cast <uint16_t> (2025)));
}

/**
 * @brief Тест извлечения составляющих даты на границах
 *
 * @details Извлечение любой составляющей требует полного разложения штампа
 *          времени, поэтому проверяются все составляющие разом
 *
 */
TEST_F(ChronoFixture, CalendarUnitsChronoTest){
	// Разложение високосного дня обычного високосного года
	ASSERT_EQ(this->_chrono->get <uint16_t> (1709208000000, awh::chrono_t::unit_t::YEAR), 2024);
	ASSERT_EQ(this->_chrono->get <uint8_t> (1709208000000, awh::chrono_t::unit_t::MONTH), 2);
	ASSERT_EQ(this->_chrono->get <uint8_t> (1709208000000, awh::chrono_t::unit_t::DATE), 29);
	ASSERT_EQ(this->_chrono->get <uint8_t> (1709208000000, awh::chrono_t::unit_t::DAY), 4);
	ASSERT_EQ(this->_chrono->get <uint16_t> (1709208000000, awh::chrono_t::unit_t::DAYS), 59);
	// Разложение последней миллисекунды високосного года
	ASSERT_EQ(this->_chrono->get <uint16_t> (1735689599999, awh::chrono_t::unit_t::YEAR), 2024);
	ASSERT_EQ(this->_chrono->get <uint8_t> (1735689599999, awh::chrono_t::unit_t::MONTH), 12);
	ASSERT_EQ(this->_chrono->get <uint8_t> (1735689599999, awh::chrono_t::unit_t::DATE), 31);
	ASSERT_EQ(this->_chrono->get <uint8_t> (1735689599999, awh::chrono_t::unit_t::HOUR), 23);
	ASSERT_EQ(this->_chrono->get <uint8_t> (1735689599999, awh::chrono_t::unit_t::MINUTES), 59);
	ASSERT_EQ(this->_chrono->get <uint8_t> (1735689599999, awh::chrono_t::unit_t::SECONDS), 59);
	ASSERT_EQ(this->_chrono->get <uint32_t> (1735689599999, awh::chrono_t::unit_t::MILLISECONDS), 999);
	ASSERT_EQ(this->_chrono->get <uint16_t> (1735689599999, awh::chrono_t::unit_t::DAYS), 365);
	// Разложение первой миллисекунды високосного года
	ASSERT_EQ(this->_chrono->get <uint16_t> (1704067200000, awh::chrono_t::unit_t::YEAR), 2024);
	ASSERT_EQ(this->_chrono->get <uint8_t> (1704067200000, awh::chrono_t::unit_t::MONTH), 1);
	ASSERT_EQ(this->_chrono->get <uint8_t> (1704067200000, awh::chrono_t::unit_t::DATE), 1);
	ASSERT_EQ(this->_chrono->get <uint8_t> (1704067200000, awh::chrono_t::unit_t::DAY), 1);
	ASSERT_EQ(this->_chrono->get <uint16_t> (1704067200000, awh::chrono_t::unit_t::DAYS), 0);
}
