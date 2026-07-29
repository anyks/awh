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

/**
 * @brief Тест разложения первых суток эпохи
 *
 * @details Оценка количества суток, добавленных високосными годами, округляется
 *          вверх и на дате меньше суток давала целые сутки. Вычитание её из даты
 *          уходило за ноль, а беззнаковый оборот превращал год в 35588: разложение
 *          ломалось на всём отрезке от одной миллисекунды до конца первых суток,
 *          то есть на всей первой дате эпохи, кроме самой её нулевой миллисекунды
 *
 */
TEST_F(ChronoFixture, CalendarEpochChronoTest){
	// Моменты первых суток эпохи, на которых разложение давало год 35588
	const uint64_t dates[] = {1, 1000, 3600000, 43200000, 86399999};
	/**
	 * Выполняем перебор всех проверяемых моментов первых суток эпохи
	 */
	for(auto & date : dates){
		// Выполняем проверку извлечения года
		ASSERT_EQ(this->_chrono->year(date), 1970) << date;
		ASSERT_EQ(this->_chrono->get <uint16_t> (date, awh::chrono_t::unit_t::YEAR), 1970) << date;
		// Выполняем проверку извлечения месяца и числа месяца
		ASSERT_EQ(this->_chrono->get <uint8_t> (date, awh::chrono_t::unit_t::MONTH), 1) << date;
		ASSERT_EQ(this->_chrono->get <uint8_t> (date, awh::chrono_t::unit_t::DATE), 1) << date;
		// Выполняем проверку извлечения количества прошедших с начала года дней
		ASSERT_EQ(this->_chrono->get <uint16_t> (date, awh::chrono_t::unit_t::DAYS), 0) << date;
	}
	/**
	 * Записи дат первых суток задаются доводом разрядности штампа времени: у метода
	 * формирования есть перегрузка со смещением временной зоны первым доводом, и
	 * малое целое подходит ей точнее, чем перегрузка с датой
	 */
	/**
	 * Нулевая дата - это полночь первого января 1970 года, дата не менее
	 * действительная, чем любая другая: до правки формирование записи по ней
	 * выдавало пустую строку, хотя заполнение объекта даты её обрабатывало
	 */
	// Выполняем проверку записи нулевой даты
	ASSERT_EQ(this->_chrono->format(static_cast <uint64_t> (0), "%Y-%m-%dT%H:%M:%S.%s"), "1970-01-01T00:00:00.000");
	// Выполняем проверку обратимости записи нулевой даты
	ASSERT_EQ(this->_chrono->parse("1970-01-01T00:00:00.000", "%Y-%m-%dT%H:%M:%S.%s"), 0);
	// Выполняем проверку записи нулевой даты в ненулевой временной зоне
	ASSERT_EQ(this->_chrono->format(static_cast <uint64_t> (0), 10800, "%Y-%m-%dT%H:%M:%S%i"), "1970-01-01T03:00:00+03:00");
	// Выполняем проверку того, что пустой формат по-прежнему даёт пустую строку
	ASSERT_TRUE(this->_chrono->format(static_cast <uint64_t> (0), "").empty());
	// Выполняем проверку записи первых и последних миллисекунд первых суток эпохи
	ASSERT_EQ(this->_chrono->format(static_cast <uint64_t> (1), "%Y-%m-%dT%H:%M:%S.%s"), "1970-01-01T00:00:00.001");
	ASSERT_EQ(this->_chrono->format(static_cast <uint64_t> (43200000), "%Y-%m-%dT%H:%M:%S.%s"), "1970-01-01T12:00:00.000");
	ASSERT_EQ(this->_chrono->format(static_cast <uint64_t> (86399999), "%Y-%m-%dT%H:%M:%S.%s"), "1970-01-01T23:59:59.999");
	// Выполняем проверку записи первой миллисекунды вторых суток эпохи
	ASSERT_EQ(this->_chrono->format(static_cast <uint64_t> (86400000), "%Y-%m-%dT%H:%M:%S.%s"), "1970-01-02T00:00:00.000");
	// Выполняем проверку границ первых суток эпохи, конечная из которых исключающая
	ASSERT_EQ(this->_chrono->begin(static_cast <uint64_t> (43200000), awh::chrono_t::type_t::DAY), 0);
	ASSERT_EQ(this->_chrono->end(static_cast <uint64_t> (43200000), awh::chrono_t::type_t::DAY), 86400000);
}

/**
 * @brief Тест непрерывности извлечения года по всему диапазону дат
 *
 * @details Перебираются границы суток всего поддерживаемого диапазона: год обязан
 *          расти строго не убывая и меняться ровно на границах годов. Проверка ловит
 *          разрыв в любой точке, а не только в заранее известных
 *
 */
TEST_F(ChronoFixture, CalendarYearSweepChronoTest){
	// Год предыдущих проверенных суток
	uint16_t previous = 1970;
	// Количество обнаруженных смен года
	uint16_t changes = 0;
	// Выполняем перебор границ суток всего поддерживаемого диапазона дат
	for(uint64_t date = 0; date < 4102444800000; date += 86400000){
		// Извлекаем год начала суток
		const uint16_t year = this->_chrono->year(date);
		// Выполняем проверку того, что год не убывает
		ASSERT_GE(year, previous) << date;
		// Выполняем проверку того, что год растёт не более чем на единицу
		ASSERT_LE(year - previous, 1) << date;
		// Если год сменился
		if(year != previous){
			// Учитываем смену года
			changes++;
			// Выполняем проверку того, что смена года пришлась на первое января
			ASSERT_EQ(this->_chrono->format(date, "%m-%d"), "01-01") << date;
			// Запоминаем год текущих суток
			previous = year;
		}
	}
	// Диапазон охватывает годы с 1970-го по 2099-й, то есть 129 смен года
	ASSERT_EQ(changes, 129);
	// Выполняем проверку года последних суток диапазона
	ASSERT_EQ(previous, 2099);
}

/**
 * @brief Тест раскладки номера дня в году на дату
 *
 * @details Переменная %j задаёт дату целиком, но сборка штампа времени опирается на
 *          месяц и число месяца, а номер дня не читает. До правки разбор записи с
 *          одним только номером дня давал первое января: номер прочитывался, но на
 *          результат не влиял
 *
 */
TEST_F(ChronoFixture, CalendarDayOfYearChronoTest){
	// Записи номера дня в году и отвечающие им даты
	const std::pair <const char *, const char *> cases[] = {
		{"2025 001", "2025-01-01"},
		{"2025 032", "2025-02-01"},
		{"2025 096", "2025-04-06"},
		{"2025 365", "2025-12-31"},
		// Високосный год содержит дополнительный день
		{"2024 060", "2024-02-29"},
		{"2024 061", "2024-03-01"},
		{"2024 366", "2024-12-31"},
		// Невисокосный год того же дополнительного дня не содержит
		{"2025 060", "2025-03-01"}
	};
	/**
	 * Выполняем перебор всех проверяемых записей номера дня в году
	 */
	for(auto & item : cases)
		// Выполняем проверку раскладки номера дня в году на дату
		ASSERT_EQ(this->_chrono->format(this->_chrono->parse(item.first, "%Y %j"), "%Y-%m-%d"), item.second)
			<< item.first;
	// Выполняем проверку того, что месяц и число месяца имеют преимущество перед номером дня
	ASSERT_EQ(this->_chrono->format(this->_chrono->parse("2025 096 12 25", "%Y %j %m %d"), "%Y-%m-%d"), "2025-12-25");
}

/**
 * @brief Тест обратимости номера дня в году
 *
 * @details Перебираются моменты времени по всему поддерживаемому диапазону дат: дата,
 *          записанная номером дня в году, обязана прочитаться обратно той же датой
 *
 */
TEST_F(ChronoFixture, CalendarDayOfYearRoundtripChronoTest){
	// Выполняем перебор моментов времени по всему поддерживаемому диапазону дат
	for(uint64_t date = 86400001; date < 4102444800000; date += 651600037){
		// Формируем запись даты номером дня в году
		const std::string result = this->_chrono->format(date, "%Y %j");
		// Выполняем проверку того, что обратный разбор дал ту же дату
		ASSERT_EQ(
			this->_chrono->format(this->_chrono->parse(result, "%Y %j"), "%Y-%m-%d"),
			this->_chrono->format(date, "%Y-%m-%d")
		) << result;
	}
}

/**
 * @brief Тест номера недели в году
 *
 * @details Переменная %U считает неделю начинающейся с воскресенья, переменная %W - с
 *          понедельника, и значения их совпадают не всегда. До правки обе выводили
 *          количество недель, прошедших с начала года, - величину, отличную от обеих.
 *          Эталонные значения получены независимо от модуля, средствами языка Python
 *
 */
TEST_F(ChronoFixture, CalendarWeekNumberChronoTest){
	// Даты и отвечающие им номера недель по обеим переменным
	struct Week {
		// Штамп времени проверяемой даты
		uint64_t date;
		// Номер недели, отсчитываемой с воскресенья
		const char * sunday;
		// Номер недели, отсчитываемой с понедельника
		const char * monday;
	};
	// Набор дат, на которых обе переменные расходятся и совпадают
	const Week cases[] = {
		// Первое января, выпадающее на понедельник
		{1704067200000, "00", "01"},
		// Дополнительный день високосного года
		{1709208000000, "08", "09"},
		// Последний день високосного года
		{1735689599999, "52", "53"},
		// Воскресенье середины года
		{1743943021520, "14", "13"},
		// Воскресенье конца года
		{1762084800000, "44", "43"},
		// Последний день диапазона
		{4102444799000, "52", "52"},
		// Вторые сутки эпохи
		{86400000, "00", "00"},
		// Дополнительный день високосного века
		{951825600000, "09", "09"}
	};
	/**
	 * Выполняем перебор всех проверяемых дат
	 */
	for(auto & item : cases){
		// Выполняем проверку номера недели, отсчитываемой с воскресенья
		ASSERT_EQ(this->_chrono->format(item.date, "%U"), item.sunday)
			<< this->_chrono->format(item.date, "%Y-%m-%d");
		// Выполняем проверку номера недели, отсчитываемой с понедельника
		ASSERT_EQ(this->_chrono->format(item.date, "%W"), item.monday)
			<< this->_chrono->format(item.date, "%Y-%m-%d");
	}
}
