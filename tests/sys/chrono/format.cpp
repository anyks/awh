/**
 * @file: format.cpp
 * @date: 2026-07-29
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Тесты формирования записи даты — проверка каждой переменной формата по отдельности
 *        на наборе граничных дат, а также составных форматов и обработки литералов
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include "chrono.hpp"

/**
 * @brief Структура параметров тестирования одной переменной формата
 *
 * @details Ожидаемые значения получены независимым эталоном, а не самим модулем:
 *          сверка модуля с собственным выводом закрепила бы и его ошибки
 *
 */
struct FormatSpecifierTestParameter {
	// Дата в миллисекундах с эпохи
	uint64_t date = 0;
	// Проверяемая переменная формата
	std::string format = "";
	// Ожидаемый результат формирования
	std::string result = "";
};

/**
 * @brief Структура параметров теста одной переменной формата
 *
 */
struct FormatSpecifierParameterizedFixture : public ChronoFixture, public ::testing::WithParamInterface <FormatSpecifierTestParameter> {
	// Параметры теста
	FormatSpecifierTestParameter _parameter = GetParam();
};

/**
 * @brief Тест формирования записи одной переменной формата
 *
 */
TEST_P(FormatSpecifierParameterizedFixture, ExecutionFormatSpecifierChronoTest){
	// Выполняем проверку результата формирования
	ASSERT_EQ(this->_chrono->format(this->_parameter.date, this->_parameter.format), this->_parameter.result);
}

/**
 * @brief Параметры тестирования переменных формата
 *
 * @details Набор дат подобран по границам: начало и конец года, 29 февраля
 *          обычного високосного года и високосного века, полночь и последняя
 *          миллисекунда суток, граница перехода на летнее время
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, FormatSpecifierParameterizedFixture,
	::testing::Values(
		// воскресенье, середина года, ненулевые миллисекунды (2025-04-06T12:37:01.520Z)
		FormatSpecifierTestParameter({1743943021520, "%y", "25"}),
		FormatSpecifierTestParameter({1743943021520, "%Y", "2025"}),
		FormatSpecifierTestParameter({1743943021520, "%g", "25"}),
		FormatSpecifierTestParameter({1743943021520, "%G", "2025"}),
		FormatSpecifierTestParameter({1743943021520, "%b", "Apr"}),
		FormatSpecifierTestParameter({1743943021520, "%h", "Apr"}),
		FormatSpecifierTestParameter({1743943021520, "%B", "April"}),
		FormatSpecifierTestParameter({1743943021520, "%m", "04"}),
		FormatSpecifierTestParameter({1743943021520, "%d", "06"}),
		FormatSpecifierTestParameter({1743943021520, "%e", " 6"}),
		FormatSpecifierTestParameter({1743943021520, "%a", "Sun"}),
		FormatSpecifierTestParameter({1743943021520, "%A", "Sunday"}),
		FormatSpecifierTestParameter({1743943021520, "%u", "7"}),
		FormatSpecifierTestParameter({1743943021520, "%w", "0"}),
		FormatSpecifierTestParameter({1743943021520, "%W", "13"}),
		FormatSpecifierTestParameter({1743943021520, "%U", "14"}),
		FormatSpecifierTestParameter({1743943021520, "%j", "096"}),
		FormatSpecifierTestParameter({1743943021520, "%D", "04/06/25"}),
		FormatSpecifierTestParameter({1743943021520, "%x", "04/06/25"}),
		FormatSpecifierTestParameter({1743943021520, "%F", "2025-04-06"}),
		FormatSpecifierTestParameter({1743943021520, "%H", "12"}),
		FormatSpecifierTestParameter({1743943021520, "%I", "12"}),
		FormatSpecifierTestParameter({1743943021520, "%M", "37"}),
		FormatSpecifierTestParameter({1743943021520, "%s", "520"}),
		FormatSpecifierTestParameter({1743943021520, "%S", "01"}),
		FormatSpecifierTestParameter({1743943021520, "%p", "PM"}),
		FormatSpecifierTestParameter({1743943021520, "%R", "12:37"}),
		FormatSpecifierTestParameter({1743943021520, "%T", "12:37:01"}),
		FormatSpecifierTestParameter({1743943021520, "%X", "12:37:01"}),
		FormatSpecifierTestParameter({1743943021520, "%r", "12:37:01 PM"}),
		FormatSpecifierTestParameter({1743943021520, "%c", "Sun Apr 6 12:37:01 2025"}),
		// начало високосного года, полночь, понедельник (2024-01-01T00:00:00.000Z)
		FormatSpecifierTestParameter({1704067200000, "%y", "24"}),
		FormatSpecifierTestParameter({1704067200000, "%Y", "2024"}),
		FormatSpecifierTestParameter({1704067200000, "%g", "24"}),
		FormatSpecifierTestParameter({1704067200000, "%G", "2024"}),
		FormatSpecifierTestParameter({1704067200000, "%b", "Jan"}),
		FormatSpecifierTestParameter({1704067200000, "%h", "Jan"}),
		FormatSpecifierTestParameter({1704067200000, "%B", "January"}),
		FormatSpecifierTestParameter({1704067200000, "%m", "01"}),
		FormatSpecifierTestParameter({1704067200000, "%d", "01"}),
		FormatSpecifierTestParameter({1704067200000, "%e", " 1"}),
		FormatSpecifierTestParameter({1704067200000, "%a", "Mon"}),
		FormatSpecifierTestParameter({1704067200000, "%A", "Monday"}),
		FormatSpecifierTestParameter({1704067200000, "%u", "1"}),
		FormatSpecifierTestParameter({1704067200000, "%w", "1"}),
		FormatSpecifierTestParameter({1704067200000, "%W", "01"}),
		FormatSpecifierTestParameter({1704067200000, "%U", "00"}),
		FormatSpecifierTestParameter({1704067200000, "%j", "001"}),
		FormatSpecifierTestParameter({1704067200000, "%D", "01/01/24"}),
		FormatSpecifierTestParameter({1704067200000, "%x", "01/01/24"}),
		FormatSpecifierTestParameter({1704067200000, "%F", "2024-01-01"}),
		FormatSpecifierTestParameter({1704067200000, "%H", "00"}),
		FormatSpecifierTestParameter({1704067200000, "%I", "12"}),
		FormatSpecifierTestParameter({1704067200000, "%M", "00"}),
		FormatSpecifierTestParameter({1704067200000, "%s", "000"}),
		FormatSpecifierTestParameter({1704067200000, "%S", "00"}),
		FormatSpecifierTestParameter({1704067200000, "%p", "AM"}),
		FormatSpecifierTestParameter({1704067200000, "%R", "00:00"}),
		FormatSpecifierTestParameter({1704067200000, "%T", "00:00:00"}),
		FormatSpecifierTestParameter({1704067200000, "%X", "00:00:00"}),
		FormatSpecifierTestParameter({1704067200000, "%r", "12:00:00 AM"}),
		FormatSpecifierTestParameter({1704067200000, "%c", "Mon Jan 1 00:00:00 2024"}),
		// 29 февраля високосного года, полдень (2024-02-29T12:00:00.000Z)
		FormatSpecifierTestParameter({1709208000000, "%y", "24"}),
		FormatSpecifierTestParameter({1709208000000, "%Y", "2024"}),
		FormatSpecifierTestParameter({1709208000000, "%g", "24"}),
		FormatSpecifierTestParameter({1709208000000, "%G", "2024"}),
		FormatSpecifierTestParameter({1709208000000, "%b", "Feb"}),
		FormatSpecifierTestParameter({1709208000000, "%h", "Feb"}),
		FormatSpecifierTestParameter({1709208000000, "%B", "February"}),
		FormatSpecifierTestParameter({1709208000000, "%m", "02"}),
		FormatSpecifierTestParameter({1709208000000, "%d", "29"}),
		FormatSpecifierTestParameter({1709208000000, "%e", "29"}),
		FormatSpecifierTestParameter({1709208000000, "%a", "Thu"}),
		FormatSpecifierTestParameter({1709208000000, "%A", "Thursday"}),
		FormatSpecifierTestParameter({1709208000000, "%u", "4"}),
		FormatSpecifierTestParameter({1709208000000, "%w", "4"}),
		FormatSpecifierTestParameter({1709208000000, "%W", "09"}),
		FormatSpecifierTestParameter({1709208000000, "%U", "08"}),
		FormatSpecifierTestParameter({1709208000000, "%j", "060"}),
		FormatSpecifierTestParameter({1709208000000, "%D", "02/29/24"}),
		FormatSpecifierTestParameter({1709208000000, "%x", "02/29/24"}),
		FormatSpecifierTestParameter({1709208000000, "%F", "2024-02-29"}),
		FormatSpecifierTestParameter({1709208000000, "%H", "12"}),
		FormatSpecifierTestParameter({1709208000000, "%I", "12"}),
		FormatSpecifierTestParameter({1709208000000, "%M", "00"}),
		FormatSpecifierTestParameter({1709208000000, "%s", "000"}),
		FormatSpecifierTestParameter({1709208000000, "%S", "00"}),
		FormatSpecifierTestParameter({1709208000000, "%p", "PM"}),
		FormatSpecifierTestParameter({1709208000000, "%R", "12:00"}),
		FormatSpecifierTestParameter({1709208000000, "%T", "12:00:00"}),
		FormatSpecifierTestParameter({1709208000000, "%X", "12:00:00"}),
		FormatSpecifierTestParameter({1709208000000, "%r", "12:00:00 PM"}),
		FormatSpecifierTestParameter({1709208000000, "%c", "Thu Feb 29 12:00:00 2024"}),
		// последняя миллисекунда високосного года (2024-12-31T23:59:59.999Z)
		FormatSpecifierTestParameter({1735689599999, "%y", "24"}),
		FormatSpecifierTestParameter({1735689599999, "%Y", "2024"}),
		FormatSpecifierTestParameter({1735689599999, "%g", "24"}),
		FormatSpecifierTestParameter({1735689599999, "%G", "2024"}),
		FormatSpecifierTestParameter({1735689599999, "%b", "Dec"}),
		FormatSpecifierTestParameter({1735689599999, "%h", "Dec"}),
		FormatSpecifierTestParameter({1735689599999, "%B", "December"}),
		FormatSpecifierTestParameter({1735689599999, "%m", "12"}),
		FormatSpecifierTestParameter({1735689599999, "%d", "31"}),
		FormatSpecifierTestParameter({1735689599999, "%e", "31"}),
		FormatSpecifierTestParameter({1735689599999, "%a", "Tue"}),
		FormatSpecifierTestParameter({1735689599999, "%A", "Tuesday"}),
		FormatSpecifierTestParameter({1735689599999, "%u", "2"}),
		FormatSpecifierTestParameter({1735689599999, "%w", "2"}),
		FormatSpecifierTestParameter({1735689599999, "%W", "53"}),
		FormatSpecifierTestParameter({1735689599999, "%U", "52"}),
		FormatSpecifierTestParameter({1735689599999, "%j", "366"}),
		FormatSpecifierTestParameter({1735689599999, "%D", "12/31/24"}),
		FormatSpecifierTestParameter({1735689599999, "%x", "12/31/24"}),
		FormatSpecifierTestParameter({1735689599999, "%F", "2024-12-31"}),
		FormatSpecifierTestParameter({1735689599999, "%H", "23"}),
		FormatSpecifierTestParameter({1735689599999, "%I", "11"}),
		FormatSpecifierTestParameter({1735689599999, "%M", "59"}),
		FormatSpecifierTestParameter({1735689599999, "%s", "999"}),
		FormatSpecifierTestParameter({1735689599999, "%S", "59"}),
		FormatSpecifierTestParameter({1735689599999, "%p", "PM"}),
		FormatSpecifierTestParameter({1735689599999, "%R", "23:59"}),
		FormatSpecifierTestParameter({1735689599999, "%T", "23:59:59"}),
		FormatSpecifierTestParameter({1735689599999, "%X", "23:59:59"}),
		FormatSpecifierTestParameter({1735689599999, "%r", "11:59:59 PM"}),
		FormatSpecifierTestParameter({1735689599999, "%c", "Tue Dec 31 23:59:59 2024"}),
		// 29 февраля 2000 года - високосный век (2000-02-29T12:00:00.000Z)
		FormatSpecifierTestParameter({951825600000, "%y", "00"}),
		FormatSpecifierTestParameter({951825600000, "%Y", "2000"}),
		FormatSpecifierTestParameter({951825600000, "%g", "00"}),
		FormatSpecifierTestParameter({951825600000, "%G", "2000"}),
		FormatSpecifierTestParameter({951825600000, "%b", "Feb"}),
		FormatSpecifierTestParameter({951825600000, "%h", "Feb"}),
		FormatSpecifierTestParameter({951825600000, "%B", "February"}),
		FormatSpecifierTestParameter({951825600000, "%m", "02"}),
		FormatSpecifierTestParameter({951825600000, "%d", "29"}),
		FormatSpecifierTestParameter({951825600000, "%e", "29"}),
		FormatSpecifierTestParameter({951825600000, "%a", "Tue"}),
		FormatSpecifierTestParameter({951825600000, "%A", "Tuesday"}),
		FormatSpecifierTestParameter({951825600000, "%u", "2"}),
		FormatSpecifierTestParameter({951825600000, "%w", "2"}),
		FormatSpecifierTestParameter({951825600000, "%W", "09"}),
		FormatSpecifierTestParameter({951825600000, "%U", "09"}),
		FormatSpecifierTestParameter({951825600000, "%j", "060"}),
		FormatSpecifierTestParameter({951825600000, "%D", "02/29/00"}),
		FormatSpecifierTestParameter({951825600000, "%x", "02/29/00"}),
		FormatSpecifierTestParameter({951825600000, "%F", "2000-02-29"}),
		FormatSpecifierTestParameter({951825600000, "%H", "12"}),
		FormatSpecifierTestParameter({951825600000, "%I", "12"}),
		FormatSpecifierTestParameter({951825600000, "%M", "00"}),
		FormatSpecifierTestParameter({951825600000, "%s", "000"}),
		FormatSpecifierTestParameter({951825600000, "%S", "00"}),
		FormatSpecifierTestParameter({951825600000, "%p", "PM"}),
		FormatSpecifierTestParameter({951825600000, "%R", "12:00"}),
		FormatSpecifierTestParameter({951825600000, "%T", "12:00:00"}),
		FormatSpecifierTestParameter({951825600000, "%X", "12:00:00"}),
		FormatSpecifierTestParameter({951825600000, "%r", "12:00:00 PM"}),
		FormatSpecifierTestParameter({951825600000, "%c", "Tue Feb 29 12:00:00 2000"}),
		// конец 2099 года (2099-12-31T23:59:59.000Z)
		FormatSpecifierTestParameter({4102444799000, "%y", "99"}),
		FormatSpecifierTestParameter({4102444799000, "%Y", "2099"}),
		FormatSpecifierTestParameter({4102444799000, "%g", "99"}),
		FormatSpecifierTestParameter({4102444799000, "%G", "2099"}),
		FormatSpecifierTestParameter({4102444799000, "%b", "Dec"}),
		FormatSpecifierTestParameter({4102444799000, "%h", "Dec"}),
		FormatSpecifierTestParameter({4102444799000, "%B", "December"}),
		FormatSpecifierTestParameter({4102444799000, "%m", "12"}),
		FormatSpecifierTestParameter({4102444799000, "%d", "31"}),
		FormatSpecifierTestParameter({4102444799000, "%e", "31"}),
		FormatSpecifierTestParameter({4102444799000, "%a", "Thu"}),
		FormatSpecifierTestParameter({4102444799000, "%A", "Thursday"}),
		FormatSpecifierTestParameter({4102444799000, "%u", "4"}),
		FormatSpecifierTestParameter({4102444799000, "%w", "4"}),
		FormatSpecifierTestParameter({4102444799000, "%W", "52"}),
		FormatSpecifierTestParameter({4102444799000, "%U", "52"}),
		FormatSpecifierTestParameter({4102444799000, "%j", "365"}),
		FormatSpecifierTestParameter({4102444799000, "%D", "12/31/99"}),
		FormatSpecifierTestParameter({4102444799000, "%x", "12/31/99"}),
		FormatSpecifierTestParameter({4102444799000, "%F", "2099-12-31"}),
		FormatSpecifierTestParameter({4102444799000, "%H", "23"}),
		FormatSpecifierTestParameter({4102444799000, "%I", "11"}),
		FormatSpecifierTestParameter({4102444799000, "%M", "59"}),
		FormatSpecifierTestParameter({4102444799000, "%s", "000"}),
		FormatSpecifierTestParameter({4102444799000, "%S", "59"}),
		FormatSpecifierTestParameter({4102444799000, "%p", "PM"}),
		FormatSpecifierTestParameter({4102444799000, "%R", "23:59"}),
		FormatSpecifierTestParameter({4102444799000, "%T", "23:59:59"}),
		FormatSpecifierTestParameter({4102444799000, "%X", "23:59:59"}),
		FormatSpecifierTestParameter({4102444799000, "%r", "11:59:59 PM"}),
		FormatSpecifierTestParameter({4102444799000, "%c", "Thu Dec 31 23:59:59 2099"}),
		// вторые сутки эпохи (1970-01-02T00:00:00.000Z)
		FormatSpecifierTestParameter({86400000, "%y", "70"}),
		FormatSpecifierTestParameter({86400000, "%Y", "1970"}),
		FormatSpecifierTestParameter({86400000, "%g", "70"}),
		FormatSpecifierTestParameter({86400000, "%G", "1970"}),
		FormatSpecifierTestParameter({86400000, "%b", "Jan"}),
		FormatSpecifierTestParameter({86400000, "%h", "Jan"}),
		FormatSpecifierTestParameter({86400000, "%B", "January"}),
		FormatSpecifierTestParameter({86400000, "%m", "01"}),
		FormatSpecifierTestParameter({86400000, "%d", "02"}),
		FormatSpecifierTestParameter({86400000, "%e", " 2"}),
		FormatSpecifierTestParameter({86400000, "%a", "Fri"}),
		FormatSpecifierTestParameter({86400000, "%A", "Friday"}),
		FormatSpecifierTestParameter({86400000, "%u", "5"}),
		FormatSpecifierTestParameter({86400000, "%w", "5"}),
		FormatSpecifierTestParameter({86400000, "%W", "00"}),
		FormatSpecifierTestParameter({86400000, "%U", "00"}),
		FormatSpecifierTestParameter({86400000, "%j", "002"}),
		FormatSpecifierTestParameter({86400000, "%D", "01/02/70"}),
		FormatSpecifierTestParameter({86400000, "%x", "01/02/70"}),
		FormatSpecifierTestParameter({86400000, "%F", "1970-01-02"}),
		FormatSpecifierTestParameter({86400000, "%H", "00"}),
		FormatSpecifierTestParameter({86400000, "%I", "12"}),
		FormatSpecifierTestParameter({86400000, "%M", "00"}),
		FormatSpecifierTestParameter({86400000, "%s", "000"}),
		FormatSpecifierTestParameter({86400000, "%S", "00"}),
		FormatSpecifierTestParameter({86400000, "%p", "AM"}),
		FormatSpecifierTestParameter({86400000, "%R", "00:00"}),
		FormatSpecifierTestParameter({86400000, "%T", "00:00:00"}),
		FormatSpecifierTestParameter({86400000, "%X", "00:00:00"}),
		FormatSpecifierTestParameter({86400000, "%r", "12:00:00 AM"}),
		FormatSpecifierTestParameter({86400000, "%c", "Fri Jan 2 00:00:00 1970"}),
		// первое воскресенье ноября - граница летнего времени США (2025-11-02T12:00:00.000Z)
		FormatSpecifierTestParameter({1762084800000, "%y", "25"}),
		FormatSpecifierTestParameter({1762084800000, "%Y", "2025"}),
		FormatSpecifierTestParameter({1762084800000, "%g", "25"}),
		FormatSpecifierTestParameter({1762084800000, "%G", "2025"}),
		FormatSpecifierTestParameter({1762084800000, "%b", "Nov"}),
		FormatSpecifierTestParameter({1762084800000, "%h", "Nov"}),
		FormatSpecifierTestParameter({1762084800000, "%B", "November"}),
		FormatSpecifierTestParameter({1762084800000, "%m", "11"}),
		FormatSpecifierTestParameter({1762084800000, "%d", "02"}),
		FormatSpecifierTestParameter({1762084800000, "%e", " 2"}),
		FormatSpecifierTestParameter({1762084800000, "%a", "Sun"}),
		FormatSpecifierTestParameter({1762084800000, "%A", "Sunday"}),
		FormatSpecifierTestParameter({1762084800000, "%u", "7"}),
		FormatSpecifierTestParameter({1762084800000, "%w", "0"}),
		FormatSpecifierTestParameter({1762084800000, "%W", "43"}),
		FormatSpecifierTestParameter({1762084800000, "%U", "44"}),
		FormatSpecifierTestParameter({1762084800000, "%j", "306"}),
		FormatSpecifierTestParameter({1762084800000, "%D", "11/02/25"}),
		FormatSpecifierTestParameter({1762084800000, "%x", "11/02/25"}),
		FormatSpecifierTestParameter({1762084800000, "%F", "2025-11-02"}),
		FormatSpecifierTestParameter({1762084800000, "%H", "12"}),
		FormatSpecifierTestParameter({1762084800000, "%I", "12"}),
		FormatSpecifierTestParameter({1762084800000, "%M", "00"}),
		FormatSpecifierTestParameter({1762084800000, "%s", "000"}),
		FormatSpecifierTestParameter({1762084800000, "%S", "00"}),
		FormatSpecifierTestParameter({1762084800000, "%p", "PM"}),
		FormatSpecifierTestParameter({1762084800000, "%R", "12:00"}),
		FormatSpecifierTestParameter({1762084800000, "%T", "12:00:00"}),
		FormatSpecifierTestParameter({1762084800000, "%X", "12:00:00"}),
		FormatSpecifierTestParameter({1762084800000, "%r", "12:00:00 PM"}),
		FormatSpecifierTestParameter({1762084800000, "%c", "Sun Nov 2 12:00:00 2025"})
	)
);

/**
 * @brief Тест записи литерального знака процента
 *
 * @details Удвоенный знак процента означает сам этот знак, а не начало переменной
 *          формата: так его определяет стандарт POSIX для strftime. До правки второй
 *          знак лишь повторно включал режим поиска переменной, отчего запись
 *          проглатывала оба знака целиком, а формат «100%%» давал «100»
 *
 */
TEST_F(ChronoFixture, FormatPercentChronoTest){
	/**
	 * Дата задаётся переменной разрядности штампа времени: перегрузки метода
	 * формирования принимают первым доводом и дату, и смещение временной зоны,
	 * и целочисленный литерал подходит обеим одинаково
	 */
	// Эталонная дата всех проверок набора
	const uint64_t date = 1743943021520;
	// Выполняем проверку записи одного только удвоенного знака процента
	ASSERT_EQ(this->_chrono->format(date, "%%"), "%");
	// Выполняем проверку записи знака процента следом за обычным текстом
	ASSERT_EQ(this->_chrono->format(date, "100%%"), "100%");
	// Выполняем проверку записи знака процента следом за переменной формата
	ASSERT_EQ(this->_chrono->format(date, "%Y%%"), "2025%");
	// Выполняем проверку записи знака процента между переменными формата
	ASSERT_EQ(this->_chrono->format(date, "%d%% of %m"), "06% of 04");
	// Выполняем проверку записи нескольких знаков процента подряд
	ASSERT_EQ(this->_chrono->format(date, "%%%%"), "%%");
	// Выполняем проверку того, что знак процента не поглощает следующую переменную
	ASSERT_EQ(this->_chrono->format(date, "%%%Y"), "%2025");
	// Выполняем проверку записи знака процента внутри полного формата даты
	ASSERT_EQ(this->_chrono->format(date, "%Y-%m-%d 100%% %H:%M:%S"), "2025-04-06 100% 12:37:01");
}
