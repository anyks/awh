/**
 * @file roundtrip.cpp
 * @date 2026-07-29
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
 * @brief Круговые тесты модуля работы с датой и временем — формирование записи с последующим
 *        её разбором на широком диапазоне дат, а также обратный круг из записи в запись
 *
 * @copyright Copyright © 2026
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
 * @brief Структура параметров кругового прогона одного формата
 *
 * @details Круговой прогон ловит то, чего не ловит проверка направлений по
 *          отдельности: расхождение между формированием и разбором. Каждое
 *          направление может быть согласовано с эталоном на выбранных датах и
 *          при этом расходиться с другим направлением на прочих
 *
 */
struct RoundtripTestParameter {
	// Проверяемый формат записи даты
	std::string format = "";
	// Точность формата в миллисекундах
	uint64_t precision = 0;
};

/**
 * @brief Структура параметров кругового теста
 *
 */
struct RoundtripParameterizedFixture : public ChronoFixture, public ::testing::WithParamInterface <RoundtripTestParameter> {
	// Параметры теста
	RoundtripTestParameter _parameter = GetParam();
};

/**
 * @brief Тест кругового прогона формата на диапазоне дат
 *
 * @details Диапазон покрывает 1970-2100 годы шагом в семь суток с четвертью:
 *          шаг намеренно некратен суткам и неделе, иначе прогон обходил бы одни
 *          и те же положения внутри суток и внутри недели
 *
 */
TEST_P(RoundtripParameterizedFixture, ExecutionRoundtripChronoTest){
	// Шаг перебора дат: семь суток и тринадцать часов
	static constexpr uint64_t STEP = 651600000;
	// Верхняя граница перебора: начало 2100 года
	static constexpr uint64_t LIMIT = 4102444800000;
	// Количество выполненных кругов
	size_t rounds = 0;
	/**
	 * Выполняем перебор дат диапазона
	 */
	for(uint64_t date = 86400000; date < LIMIT; date += STEP){
		// Приводим дату к точности проверяемого формата
		const uint64_t expected = (date - (date % this->_parameter.precision));
		// Выполняем формирование записи даты
		const std::string text = this->_chrono->format(date, this->_parameter.format);
		// Проверяем, что запись сформирована
		ASSERT_FALSE(text.empty()) << "формат: " << this->_parameter.format << ", дата: " << date;
		// Выполняем разбор сформированной записи и сверяем с исходной датой
		ASSERT_EQ(this->_chrono->parse(text, this->_parameter.format), expected)
		 << "формат: " << this->_parameter.format << ", запись: " << text << ", дата: " << date;
		// Считаем выполненный круг
		rounds++;
	}
	// Проверяем, что перебор дат действительно выполнялся
	ASSERT_GT(rounds, 6000u);
}

/**
 * @brief Параметры кругового тестирования форматов
 *
 * @details Формат с двузначным годом в круговой прогон не входит: запись "70"
 *          разбирается как 2070 год, поэтому даты до 2000 года через него не
 *          проходят по устройству, а не по ошибке
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, RoundtripParameterizedFixture,
	::testing::Values(
		// Запись обмена данными по ISO 8601 с миллисекундами
		RoundtripTestParameter({"%Y-%m-%dT%H:%M:%S.%s", 1}),
		// Запись обмена данными по ISO 8601 со смещением зоны
		RoundtripTestParameter({"%Y-%m-%dT%H:%M:%S.%s%z", 1}),
		// Запись с пробелом в качестве разделителя даты и времени
		RoundtripTestParameter({"%Y-%m-%d %H:%M:%S", 1000}),
		// Запись журнала веб-сервера
		RoundtripTestParameter({"%d/%h/%Y:%H:%M:%S", 1000}),
		// Сплошная запись без разделителей
		RoundtripTestParameter({"%Y%m%dT%H%M%S", 1000}),
		// Запись с двенадцатичасовым временем
		RoundtripTestParameter({"%m/%d/%Y %I:%M:%S %p", 1000}),
		// Запись с месяцем и днём недели названиями
		RoundtripTestParameter({"%a %h %d %H:%M:%S %Y", 1000}),
		// Запись с полными названиями месяца и дня недели
		RoundtripTestParameter({"%A %B %d %Y %H:%M:%S", 1000}),
		// Запись через составные переменные даты и времени
		RoundtripTestParameter({"%F %T", 1000}),
		// Запись с точностью до минут
		RoundtripTestParameter({"%F %R", 60000}),
		// Запись европейского порядка следования полей
		RoundtripTestParameter({"%d.%m.%Y %H:%M:%S", 1000})
	)
);

/**
 * @brief Тест обратного кругового прогона: из записи в запись
 *
 * @details Проверяется устойчивость перевода записи между форматами: повторный
 *          перевод в исходный формат обязан вернуть исходную запись
 *
 */
TEST_F(ChronoFixture, RoundtripStripChronoTest){
	// Набор проверяемых пар форматов
	const std::vector <std::pair <std::string, std::string>> formats = {
		{"%Y-%m-%dT%H:%M:%S", "%d/%h/%Y:%H:%M:%S"},
		{"%Y-%m-%d %H:%M:%S", "%m/%d/%Y %I:%M:%S %p"},
		{"%F %T", "%a %h %d %H:%M:%S %Y"},
		{"%d.%m.%Y %H:%M:%S", "%Y%m%dT%H%M%S"}
	};
	// Эталонная дата прогона
	static constexpr uint64_t DATE = 1743943021520;
	/**
	 * Выполняем перебор проверяемых пар форматов
	 */
	for(auto & format : formats){
		// Выполняем формирование записи в исходном формате
		const std::string source = this->_chrono->format(DATE, format.first);
		// Выполняем перевод записи в конечный формат
		const std::string target = this->_chrono->strip(source, format.first, format.second);
		// Выполняем обратный перевод записи в исходный формат
		const std::string result = this->_chrono->strip(target, format.second, format.first);
		// Проверяем, что обратный перевод вернул исходную запись
		ASSERT_EQ(result, source) << "форматы: " << format.first << " <-> " << format.second;
	}
}
