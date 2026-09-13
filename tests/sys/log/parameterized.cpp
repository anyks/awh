/**
 * @file parameterized.cpp
 * @date 2025-12-12
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
 * @brief Параметризованные тесты модуля логирования — прогон подготовленных наборов входных данных через методы
 *        модуля с проверкой форматирования сообщений по уровням важности, работы приёмников вывода и ротации файлов
 *
 * @copyright Copyright © 2025
 *
 */
#include "log.hpp"
#include <sys/log.hpp>

/**
 * Подключаем заголовочный файлы проекта
 */

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
	awh::log::flag_t flag = awh::log::flag_t::NONE;
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
	/**
	 * Счётчик записей, дошедших до подписчика
	 *
	 * @note Без него проверка эта проходила бы и при молчащем модуле: все её
	 *       утверждения живут в отклике, а не дошедший отклик означает лишь то,
	 *       что утверждать оказалось нечего. Счётчик обращает молчание в отказ
	 */
	uint16_t received = 0;
	// Подписываемся на получение логов
	awh::log::subscribe([this, &received](const awh::log::flag_t flag, std::string_view text) noexcept -> void {
		// Запоминаем, что запись до подписчика дошла
		received++;
		// Проверяем корректность полученного флага лога
		ASSERT_EQ(this->_parameter.flag, flag);
		// Проверяем корректность текста лога
		ASSERT_EQ(this->_parameter.result, text);
	});
	// Устанавливаем режимы логов
	awh::log::mode({awh::log::mode_t::CONSOLE});
	// Записываем в лог с параметрами в отладочном режиме
	awh::log::debug(this->_parameter.format, this->_parameter.method, {}, this->_parameter.flag, this->_parameter.args);
	// Устанавливаем режимы логов в отложенном режиме
	awh::log::mode({awh::log::mode_t::DEFERRED});
	// Выполняем формирование лога в отложенном режиме
	awh::log::print(this->_parameter.format, this->_parameter.flag, this->_parameter.args);
	/**
	 * До подписчика обязана дойти ровно одна запись - та, что сделана в режиме DEFERRED
	 *
	 * @note Их именно одна, а не две: `DEFERRED` и есть разрешение выводить логи в
	 *       функцию обратного вызова, а `CONSOLE` его не даёт. Запись, сделанная выше
	 *       в консольном режиме, до подписчика не доходит - и не должна. Замерено
	 *       щупом: после `debug` счётчик нулевой, после `print` - единица
	 */
	ASSERT_EQ(received, static_cast <uint16_t> (1));
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
			awh::log::flag_t::NONE
		}),
		LogTestParameter({
			"$1 $2$3",
			__PRETTY_FUNCTION__,
			"Привет Мир!!!",
			{"Привет", "Мир", "!!!"},
			awh::log::flag_t::CRITICAL
		}),
		LogTestParameter({
			"$1 $2$3",
			__PRETTY_FUNCTION__,
			"Hello World!!!",
			{"Hello", "World", "!!!"},
			awh::log::flag_t::INFO
		}),
		LogTestParameter({
			"$1 $2$3",
			__PRETTY_FUNCTION__,
			"Привет Мир!!!",
			{"Привет", "Мир", "!!!"},
			awh::log::flag_t::WARNING
		})
	)
);
