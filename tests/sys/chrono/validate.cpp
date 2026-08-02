/**
 * @file: validate.cpp
 * @date: 2026-07-29
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Тесты проверки пригодности разбираемых записей — записи даты, обозначения
 *        временной зоны и обозначения размерности времени
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include "chrono.hpp"

/**
 * @brief Структура параметров тестирования пригодности записи даты
 *
 */
struct ValidateTestParameter {
	// Проверяемая запись даты
	std::string date = "";
	// Формат разбора записи
	std::string format = "";
	// Ожидаемый признак пригодности
	bool result = false;
};

/**
 * @brief Структура параметров теста пригодности записи даты
 *
 */
struct ValidateParameterizedFixture : public ChronoFixture, public ::testing::WithParamInterface <ValidateTestParameter> {
	// Параметры теста
	ValidateTestParameter _parameter = GetParam();
};

/**
 * @brief Тест проверки пригодности записи даты для разбора
 *
 */
TEST_P(ValidateParameterizedFixture, ExecutionValidateChronoTest){
	// Выполняем проверку признака пригодности записи
	ASSERT_EQ(this->_chrono->validate(this->_parameter.date, this->_parameter.format), this->_parameter.result)
		<< this->_parameter.date << " (" << this->_parameter.format << ")";
}

/**
 * @brief Параметры тестирования пригодности записи даты
 *
 * @details Пригодной считается запись, в которой нашлась каждая переменная формата и
 *          разобранные поля которой лежат в допустимых пределах. Набор покрывает обе
 *          причины непригодности по отдельности: записи без единого поля формата и
 *          записи со структурой верной, но полями вне пределов. Секунда координации
 *          пригодной считается: её разрешают RFC 3339 и ссылающиеся на него стандарты
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, ValidateParameterizedFixture,
	::testing::Values(
		// Записи стандартов
		ValidateTestParameter({"2025-04-06", "%Y-%m-%d", true}),
		ValidateTestParameter({"2025-04-06T12:37:01.520Z", "%Y-%m-%dT%H:%M:%S.%s%i", true}),
		ValidateTestParameter({"Sun, 06 Apr 2025 12:37:01 +0000", "%a, %d %b %Y %H:%M:%S %z", true}),
		ValidateTestParameter({"06/Apr/2025:12:37:01 +0000", "%d/%b/%Y:%H:%M:%S %z", true}),
		ValidateTestParameter({"Apr  6 12:37:01", "%b %e %H:%M:%S", true}),
		// Записи, в которых полей формата не нашлось
		ValidateTestParameter({"мусор", "%Y-%m-%d", false}),
		ValidateTestParameter({"not a date at all", "%Y-%m-%dT%H:%M:%S", false}),
		ValidateTestParameter({"---", "%Y-%m-%d", false}),
		// Пустые доводы
		ValidateTestParameter({"", "%Y-%m-%d", false}),
		ValidateTestParameter({"2025-04-06", "", false}),
		ValidateTestParameter({"", "", false}),
		// Номер месяца вне пределов
		ValidateTestParameter({"2025-13-01", "%Y-%m-%d", false}),
		ValidateTestParameter({"2025-00-01", "%Y-%m-%d", false}),
		ValidateTestParameter({"2025-12-01", "%Y-%m-%d", true}),
		// Число месяца вне пределов месяца
		ValidateTestParameter({"2025-04-31", "%Y-%m-%d", false}),
		ValidateTestParameter({"2025-04-30", "%Y-%m-%d", true}),
		ValidateTestParameter({"2025-01-31", "%Y-%m-%d", true}),
		ValidateTestParameter({"2025-01-32", "%Y-%m-%d", false}),
		ValidateTestParameter({"2025-04-00", "%Y-%m-%d", false}),
		// Число месяца февраля с учётом високосности года
		ValidateTestParameter({"2024-02-29", "%Y-%m-%d", true}),
		ValidateTestParameter({"2025-02-29", "%Y-%m-%d", false}),
		ValidateTestParameter({"2000-02-29", "%Y-%m-%d", true}),
		ValidateTestParameter({"2100-02-29", "%Y-%m-%d", false}),
		ValidateTestParameter({"2024-02-30", "%Y-%m-%d", false}),
		// Составляющие времени суток вне пределов
		ValidateTestParameter({"2025-04-06 23:59:59", "%Y-%m-%d %H:%M:%S", true}),
		ValidateTestParameter({"2025-04-06 24:00:00", "%Y-%m-%d %H:%M:%S", false}),
		ValidateTestParameter({"2025-04-06 12:60:00", "%Y-%m-%d %H:%M:%S", false}),
		ValidateTestParameter({"2025-04-06 12:00:61", "%Y-%m-%d %H:%M:%S", false}),
		// Секунда координации, разрешённая стандартом RFC 3339
		ValidateTestParameter({"2025-04-06 12:00:60", "%Y-%m-%d %H:%M:%S", true})
	)
);

/**
 * @brief Структура параметров тестирования пригодности обозначения зоны
 *
 */
struct ValidateZoneTestParameter {
	// Проверяемое обозначение временной зоны
	std::string zone = "";
	// Ожидаемый признак пригодности
	bool result = false;
};

/**
 * @brief Структура параметров теста пригодности обозначения зоны
 *
 */
struct ValidateZoneParameterizedFixture : public ChronoFixture, public ::testing::WithParamInterface <ValidateZoneTestParameter> {
	// Параметры теста
	ValidateZoneTestParameter _parameter = GetParam();
};

/**
 * @brief Тест проверки пригодности обозначения временной зоны
 *
 */
TEST_P(ValidateZoneParameterizedFixture, ExecutionValidateZoneChronoTest){
	// Выполняем проверку признака пригодности обозначения
	ASSERT_EQ(this->_chrono->validateTimeZone(this->_parameter.zone), this->_parameter.result)
		<< this->_parameter.zone;
}

/**
 * @brief Параметры тестирования пригодности обозначения зоны
 *
 * @details Перевод обозначения в смещение неизвестное обозначение возвращает нулём и
 *          от честного нулевого смещения не отличает. Проверка закрывает этот пробел
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, ValidateZoneParameterizedFixture,
	::testing::Values(
		// Названия временных зон
		ValidateZoneTestParameter({"UTC", true}),
		ValidateZoneTestParameter({"GMT", true}),
		ValidateZoneTestParameter({"MSK", true}),
		ValidateZoneTestParameter({"EST", true}),
		// Смещения всех принимаемых написаний
		ValidateZoneTestParameter({"+1", true}),
		ValidateZoneTestParameter({"+05:30", true}),
		ValidateZoneTestParameter({"+0530", true}),
		ValidateZoneTestParameter({"+530", true}),
		ValidateZoneTestParameter({"-0330", true}),
		// Название со смещением от него
		ValidateZoneTestParameter({"GMT+0530", true}),
		ValidateZoneTestParameter({"MSK+1", true}),
		ValidateZoneTestParameter({"UTC-3:30", true}),
		// Обозначения непригодные
		ValidateZoneTestParameter({"XXXX", false}),
		ValidateZoneTestParameter({"мусор", false}),
		ValidateZoneTestParameter({"", false}),
		ValidateZoneTestParameter({"+", false})
	)
);

/**
 * @brief Структура параметров тестирования пригодности обозначения размерности
 *
 */
struct ValidateSecondsTestParameter {
	// Проверяемое обозначение размерности времени
	std::string value = "";
	// Ожидаемый признак пригодности
	bool result = false;
};

/**
 * @brief Структура параметров теста пригодности обозначения размерности
 *
 */
struct ValidateSecondsParameterizedFixture : public ChronoFixture, public ::testing::WithParamInterface <ValidateSecondsTestParameter> {
	// Параметры теста
	ValidateSecondsTestParameter _parameter = GetParam();
};

/**
 * @brief Тест проверки пригодности обозначения размерности времени
 *
 */
TEST_P(ValidateSecondsParameterizedFixture, ExecutionValidateSecondsChronoTest){
	// Выполняем проверку признака пригодности обозначения
	ASSERT_EQ(this->_chrono->validateSeconds(this->_parameter.value), this->_parameter.result)
		<< this->_parameter.value;
}

/**
 * @brief Параметры тестирования пригодности обозначения размерности
 *
 * @details Непригодное обозначение переводится в ноль секунд и от честного нуля не
 *          отличается. Запятая разбором захватывается вместе с числом, но в число не
 *          переводится, поэтому обозначение с нею непригодно
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, ValidateSecondsParameterizedFixture,
	::testing::Values(
		// Обозначения всех единиц размерности
		ValidateSecondsTestParameter({"45s", true}),
		ValidateSecondsTestParameter({"90m", true}),
		ValidateSecondsTestParameter({"2h", true}),
		ValidateSecondsTestParameter({"2d", true}),
		ValidateSecondsTestParameter({"1w", true}),
		ValidateSecondsTestParameter({"3M", true}),
		ValidateSecondsTestParameter({"1y", true}),
		// Дробное число с разделителем в виде точки
		ValidateSecondsTestParameter({"1.5h", true}),
		ValidateSecondsTestParameter({"0.5s", true}),
		// Обозначения непригодные
		ValidateSecondsTestParameter({"42", false}),
		ValidateSecondsTestParameter({"1,5h", false}),
		ValidateSecondsTestParameter({"1.2.3h", false}),
		ValidateSecondsTestParameter({"", false}),
		ValidateSecondsTestParameter({"мусор", false}),
		ValidateSecondsTestParameter({"h", false}),
		/**
		 * Составная запись обозначением не является: прежде проверка её одобряла, хотя
		 * перевод брал у неё один лишь хвост, отбрасывая начало молча
		 */
		ValidateSecondsTestParameter({"1h30m", false}),
		ValidateSecondsTestParameter({"1w2d3h15m30s", false}),
		// Обозначение занимает запись целиком, посторонних символов перед ним нет
		ValidateSecondsTestParameter({"timeout=90m", false}),
		ValidateSecondsTestParameter({" 90m", false}),
		// Пробел между числом и единицей размерности допускается
		ValidateSecondsTestParameter({"90 m", true})
	)
);

/**
 * @brief Структура теста проверок пригодности
 *
 */
struct ValidateFixture : public ChronoFixture {};

/**
 * @brief Тест отсутствия побочных действий у проверки
 *
 * @details Проверка выполняется в глобальном хранилище и внутреннего объекта даты не
 *          изменяет, каким бы ни было хранилище последующего разбора. Без этого
 *          проверка записи сбивала бы часы, относительно которых разбирается
 *          следующая запись, года не содержащая
 *
 */
TEST_F(ValidateFixture, ExecutionValidateSideEffectChronoTest){
	// Переводим часы локального хранилища на известный момент
	this->_chrono->timestamp(1743943021520, awh::chrono_t::type_t::MILLISECONDS);
	// Запоминаем показание часов локального хранилища
	const std::string before = this->_chrono->format("%Y-%m-%dT%H:%M:%S.%s", awh::chrono_t::storage_t::LOCAL);
	// Выполняем проверку пригодности записей обоих исходов
	ASSERT_TRUE(this->_chrono->validate("2030-12-25", "%Y-%m-%d"));
	ASSERT_FALSE(this->_chrono->validate("мусор", "%Y-%m-%d"));
	// Выполняем проверку того, что часы локального хранилища не сбились
	ASSERT_EQ(this->_chrono->format("%Y-%m-%dT%H:%M:%S.%s", awh::chrono_t::storage_t::LOCAL), before);
}

/**
 * @brief Тест согласованности проверки с разбором
 *
 * @details Запись, признанную пригодной, разбор обязан прочитать тем же моментом
 *          времени, каким она была записана. Проверка перебирает записи по всему
 *          поддерживаемому диапазону дат
 *
 */
TEST_F(ValidateFixture, ExecutionValidateAgreementChronoTest){
	// Формат записи даты со всеми составляющими
	const char * format = "%Y-%m-%dT%H:%M:%S.%s";
	// Выполняем перебор моментов времени по всему поддерживаемому диапазону дат
	for(uint64_t date = 86400001; date < 4102444800000; date += 651600037){
		// Формируем запись штампа времени
		const std::string result = this->_chrono->format(date, format);
		// Выполняем проверку того, что сформированная запись признана пригодной
		ASSERT_TRUE(this->_chrono->validate(result, format)) << result;
		// Выполняем проверку того, что разбор дал исходный момент времени
		ASSERT_EQ(this->_chrono->parse(result, format), date) << result;
	}
}
