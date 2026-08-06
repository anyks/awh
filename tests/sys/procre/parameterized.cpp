/**
 * @file: parameterized.cpp
 * @date: 2026-01-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Параметризованные тесты модуля резольвера процессов — прогон подготовленных наборов входных данных через
 *        методы модуля с проверкой сопоставления сетевого соединения по адресам и портам с владеющим им процессом
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем восполнение средств POSIX, отсутствующих у MS Windows
 */
#include "../../posix.hpp"

/**
 * Подключаем заголовочный файл
 */
#include "procre.hpp"

/**
 * @brief Структура параметров теста
 *
 */
struct ProcreTestParameter {
	pid_t pid = 0;
	bool empty = false;
};

/**
 * @brief Параметризованный тестовый класс для работы с процессами
 *
 */
class ProcreTestParameterizedFixture : public ProcreFixture, public ::testing::WithParamInterface <ProcreTestParameter> {
	public:
		ProcreTestParameter _parameter = GetParam();
};

/**
 * @brief Тест параметризованного получения имени процесса
 *
 */
TEST_P(ProcreTestParameterizedFixture, NameTest){
	// Получаем имя процесса
	std::string name = this->_procre->name(this->_parameter.pid);

	// Если результат должен быть пустым
	if(this->_parameter.empty)
		// Проверяем что имя пустое
		ASSERT_TRUE(name.empty());
	// Если результат не должен быть пустым
	else if(this->_parameter.pid == 1) {
		// Если мы работаем от имени суперпользователя
		if(::geteuid() == 0)
			// Проверяем что имя не пустое
			ASSERT_FALSE(name.empty());
	// Проверяем что имя не пустое
	} else ASSERT_FALSE(name.empty());
}

/**
 * @brief Инициализация параметров теста
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, ProcreTestParameterizedFixture,
	::testing::Values(
		ProcreTestParameter({
			::getpid(),
			false
		}),
		ProcreTestParameter({
			1,
			false
		}),
		ProcreTestParameter({
			-1,
			true
		})
	)
);
