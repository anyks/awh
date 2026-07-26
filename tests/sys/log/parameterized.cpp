/**
 * @file: parameterized.cpp
 * @date: 2025-12-12
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Параметризованные тесты модуля логирования — прогон подготовленных наборов входных данных через методы
 *        модуля с проверкой форматирования сообщений по уровням важности, работы приёмников вывода и ротации файлов
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "log.hpp"

/**
 * @brief Структура параметров тестов логов
 *
 */
struct LogTestParameter {
	// Формат сообщения лога
	std::string format = "";
	// Метод вызова лога
	std::string method = "";
	// Ожидаемый результат лога
	std::string result = "";
	// Аргументы формирования лога
	std::vector <std::string> args;
	// Флаг типа логирования
	awh::log_t::flag_t flag = awh::log_t::flag_t::NONE;
};

/**
 * @brief Параметризованный тестовый класс для работы с логами
 *
 */
class LogTestParameterizedFixture : public LogFixture, public ::testing::WithParamInterface <LogTestParameter> {
	public:
		// Параметры теста
		LogTestParameter _parameter = GetParam();
};

/**
 * @brief Тесты параметризованного вывода логов
 *
 */
TEST_P(LogTestParameterizedFixture, LogPrintTest){
	// Подписываемся на получение логов
	this->_log->subscribe([this](const awh::log_t::flag_t flag, std::string_view text) noexcept -> void {
		// Проверяем корректность полученного флага лога
		ASSERT_EQ(this->_parameter.flag, flag);
		// Проверяем корректность текста лога
		ASSERT_EQ(this->_parameter.result, text);
	});
	// Устанавливаем режимы логов
	this->_log->mode({awh::log_t::mode_t::CONSOLE});
	// Записываем в лог с параметрами в отладочном режиме
	this->_log->debug(this->_parameter.format, this->_parameter.method, {}, this->_parameter.flag, this->_parameter.args);
	// Устанавливаем режимы логов в отложенном режиме
	this->_log->mode({awh::log_t::mode_t::DEFERRED});
	// Выполняем формирование лога в отложенном режиме
	this->_log->print(this->_parameter.format, this->_parameter.flag, this->_parameter.args);
}

/**
 * @brief Инициализация параметризованных тестов логов
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, LogTestParameterizedFixture,
	::testing::Values(
		LogTestParameter({
			"$1 $2$3",
			__PRETTY_FUNCTION__,
			"Hello World!!!",
			{"Hello", "World", "!!!"},
			awh::log_t::flag_t::NONE
		}),
		LogTestParameter({
			"$1 $2$3",
			__PRETTY_FUNCTION__,
			"Привет Мир!!!",
			{"Привет", "Мир", "!!!"},
			awh::log_t::flag_t::CRITICAL
		}),
		LogTestParameter({
			"$1 $2$3",
			__PRETTY_FUNCTION__,
			"Hello World!!!",
			{"Hello", "World", "!!!"},
			awh::log_t::flag_t::INFO
		}),
		LogTestParameter({
			"$1 $2$3",
			__PRETTY_FUNCTION__,
			"Привет Мир!!!",
			{"Привет", "Мир", "!!!"},
			awh::log_t::flag_t::WARNING
		})
	)
);
