/**
 * @file: substitution.cpp
 * @date: 2026-07-29
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Тесты правил подстановки недостающих полей даты — какие поля берутся
 *        текущими, а какие задаются наименьшим своим значением
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include "chrono.hpp"

/**
 * @brief Формат записи даты со всеми полями
 *
 * @details Через него выражаются эталонные значения набора: разбор полной записи
 *          подстановки не выполняет вовсе и потому годится точкой отсчёта
 *
 */
static constexpr const char * FULL_FORMAT = "%Y-%m-%d %H:%M:%S.%s";

/**
 * @brief Структура параметров тестирования подстановки недостающих полей
 *
 */
struct SubstitutionTestParameter {
	// Разбираемая неполная запись даты
	std::string date = "";
	// Формат разбора неполной записи
	std::string format = "";
	// Равнозначная полная запись даты
	std::string result = "";
};

/**
 * @brief Структура параметров теста подстановки недостающих полей
 *
 */
struct SubstitutionParameterizedFixture : public ChronoFixture, public ::testing::WithParamInterface <SubstitutionTestParameter> {
	// Параметры теста
	SubstitutionTestParameter _parameter = GetParam();
};

/**
 * @brief Тест подстановки недостающих полей даты
 *
 * @details Эталон задаётся не числом, а равнозначной полной записью: так проверка
 *          не зависит ни от временной зоны, ни от момента её выполнения
 *
 */
TEST_P(SubstitutionParameterizedFixture, ExecutionSubstitutionChronoTest){
	// Выполняем разбор равнозначной полной записи даты
	const uint64_t result = this->_chrono->parse(this->_parameter.result, FULL_FORMAT);
	// Выполняем проверку разбора неполной записи
	ASSERT_EQ(this->_chrono->parse(this->_parameter.date, this->_parameter.format), result)
		<< this->_parameter.date << " (" << this->_parameter.format << ")";
}

/**
 * @brief Параметры тестирования подстановки недостающих полей
 *
 * @details Правило подстановки несимметрично: поля крупнее самого крупного заданного
 *          берутся текущими, поля мельче самого мелкого заданного задаются наименьшим
 *          своим значением. Первая половина правила введена ради устаревшего стандарта
 *          системного журнала RFC 3164, года не записывающего: без неё все его записи
 *          попадали бы в 1970-й год. Вторая половина - ради устойчивости разбора: до
 *          правки запись одной только даты дополнялась текущим временем суток, отчего
 *          один и тот же довод давал разный результат при каждом вызове
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, SubstitutionParameterizedFixture,
	::testing::Values(
		// Задан только год: месяц и число задаются первыми, время - началом суток
		SubstitutionTestParameter({"2025", "%Y", "2025-01-01 00:00:00.000"}),
		SubstitutionTestParameter({"1999", "%Y", "1999-01-01 00:00:00.000"}),
		// Заданы год и месяц: число задаётся первым, время - началом суток
		SubstitutionTestParameter({"2025-04", "%Y-%m", "2025-04-01 00:00:00.000"}),
		SubstitutionTestParameter({"2025-12", "%Y-%m", "2025-12-01 00:00:00.000"}),
		SubstitutionTestParameter({"2024 Feb", "%Y %b", "2024-02-01 00:00:00.000"}),
		// Задана дата целиком: время задаётся началом суток
		SubstitutionTestParameter({"2025-04-06", "%Y-%m-%d", "2025-04-06 00:00:00.000"}),
		SubstitutionTestParameter({"06/04/2025", "%d/%m/%Y", "2025-04-06 00:00:00.000"}),
		SubstitutionTestParameter({"2025-04-06", "%F", "2025-04-06 00:00:00.000"}),
		SubstitutionTestParameter({"04/06/25", "%D", "2025-04-06 00:00:00.000"}),
		SubstitutionTestParameter({"29.02.2024", "%d.%m.%Y", "2024-02-29 00:00:00.000"}),
		// Задан час: минуты, секунды и миллисекунды задаются нулевыми
		SubstitutionTestParameter({"2025-04-06 12", "%Y-%m-%d %H", "2025-04-06 12:00:00.000"}),
		// Заданы минуты: секунды и миллисекунды задаются нулевыми
		SubstitutionTestParameter({"2025-04-06 12:37", "%Y-%m-%d %H:%M", "2025-04-06 12:37:00.000"}),
		SubstitutionTestParameter({"2025-04-06 12:37", "%Y-%m-%d %R", "2025-04-06 12:37:00.000"}),
		// Заданы секунды: миллисекунды задаются нулевыми
		SubstitutionTestParameter({"2025-04-06 12:37:01", "%Y-%m-%d %H:%M:%S", "2025-04-06 12:37:01.000"}),
		SubstitutionTestParameter({"2025-04-06 12:37:01", "%Y-%m-%d %T", "2025-04-06 12:37:01.000"}),
		// Задано всё: подстановка не выполняется
		SubstitutionTestParameter({"2025-04-06 12:37:01.520", FULL_FORMAT, "2025-04-06 12:37:01.520"})
	)
);

/**
 * @brief Структура теста правил подстановки
 *
 */
struct SubstitutionFixture : public ChronoFixture {};

/**
 * @brief Тест устойчивости разбора неполной записи
 *
 * @details Один и тот же довод обязан давать один и тот же результат вне зависимости
 *          от момента вызова. До правки поля мельче заданных дополнялись текущими, и
 *          разбор записи одной только даты возвращал разные значения при вызовах,
 *          разделённых миллисекундой. Признак устойчивости - выравнивание результата
 *          на границу заданного полем отрезка
 *
 */
TEST_F(SubstitutionFixture, ExecutionSubstitutionStableChronoTest){
	// Записи даты разной полноты и величина отрезка, на границу которого они обязаны попасть
	const std::pair <std::pair <const char *, const char *>, uint64_t> cases[] = {
		{{"2025", "%Y"}, 86400000},
		{{"2025-04", "%Y-%m"}, 86400000},
		{{"2025-04-06", "%Y-%m-%d"}, 86400000},
		{{"2025-04-06 12", "%Y-%m-%d %H"}, 3600000},
		{{"2025-04-06 12:37", "%Y-%m-%d %H:%M"}, 60000},
		{{"2025-04-06 12:37:01", "%Y-%m-%d %H:%M:%S"}, 1000}
	};
	/**
	 * Выполняем перебор всех проверяемых записей даты
	 */
	for(auto & item : cases){
		// Выполняем разбор неполной записи даты
		const uint64_t result = this->_chrono->parse(item.first.first, item.first.second);
		// Выполняем проверку выравнивания результата на границу отрезка
		ASSERT_EQ(result % item.second, 0) << item.first.first << " (" << item.first.second << ")";
		/**
		 * Выполняем повторный разбор той же записи несколько раз подряд
		 */
		for(uint8_t i = 0; i < 32; i++)
			// Выполняем проверку совпадения результата с полученным ранее
			ASSERT_EQ(this->_chrono->parse(item.first.first, item.first.second), result)
				<< item.first.first << " (" << item.first.second << ")";
	}
}

/**
 * @brief Тест подстановки текущих значений в поля крупнее заданных
 *
 * @details Поля крупнее самого крупного заданного берутся текущими - на этом стоит
 *          разбор записей устаревшего стандарта системного журнала, года не
 *          содержащих. Проверка выражает эталон через текущий момент, а не через
 *          записанное число, и потому не устаревает
 *
 */
TEST_F(SubstitutionFixture, ExecutionSubstitutionCurrentChronoTest){
	// Получаем текущий момент времени
	const uint64_t now = this->_chrono->timestamp(awh::chrono_t::type_t::MILLISECONDS);
	// Получаем начало текущих суток
	const uint64_t today = this->_chrono->begin(now, awh::chrono_t::type_t::DAY);
	// Выполняем разбор записи, содержащей одно только время суток
	const uint64_t parsed = this->_chrono->parse("12:37:01", "%H:%M:%S");
	// Выполняем проверку того, что дата взята текущая, а время - заданное
	ASSERT_EQ(parsed, today + 45421000);
	// Формируем запись даты текущего года без указания года
	const std::string date = this->_chrono->format(today, "%m-%d");
	// Выполняем разбор записи, года не содержащей
	const uint64_t year = this->_chrono->parse(date, "%m-%d");
	// Выполняем проверку того, что год взят текущий, а время - начало суток
	ASSERT_EQ(year, today) << date;
}
