/**
 * @file: parameterized.cpp
 * @date: 2026-01-22
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Параметризованные тесты модуля функций обратного вызова — прогон подготовленных наборов входных данных через
 *        методы модуля с проверкой регистрации колбэков произвольных сигнатур,
 *        адресации по идентификатору и имени и их вызова
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "callback.hpp"

/**
 * @brief Структура параметров тестирования метода регистрации
 *
 */
struct CallbackOnTestParameter {
	// Ожидаемый результат (существование)
	bool exists = false;
	// Имя функции
	std::string name = "";
};

/**
 * @brief Класс параметризованного теста для метода регистрации
 *
 */
class CallbackOnParameterizedFixture : public CallbackFixture, public ::testing::WithParamInterface <CallbackOnTestParameter> {
	public:
		CallbackOnTestParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования регистрации
 *
 */
TEST_P(CallbackOnParameterizedFixture, OnTest){
	// Регистрируем
	this->_callback->on <void()> (this->_parameter.name, [](){});
	// Проверяем
	ASSERT_EQ(this->_parameter.exists, this->_callback->is(this->_parameter.name));
}

/**
 * @brief Инициализация параметров
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, CallbackOnParameterizedFixture,
	::testing::Values(
		CallbackOnTestParameter({true, "test1"}),
		CallbackOnTestParameter({true, "test2"}),
		CallbackOnTestParameter({true, "function"})
	)
);
