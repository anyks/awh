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
 * @copyright: Copyright © 2026
 */

/**
 * Подключаем заголовочный файл
 */
#include "signals.hpp"
#include <mutex>
#include <condition_variable>

/**
 * @brief Структура параметров теста
 *
 */
struct SignalsTestParameter {
	// Сигнал для теста
	int32_t signal = 0;
};

/**
 * @brief Параметризованный тестовый класс для работы с сигналами
 *
 */
class SignalsTestParameterizedFixture : public SignalsFixture, public ::testing::WithParamInterface <SignalsTestParameter> {
	public:
		SignalsTestParameter _parameter = GetParam();
};

/**
 * @brief Тест параметризованной обработки сигналов
 *
 */
TEST_P(SignalsTestParameterizedFixture, SignalTest){
	// Создаём мьютекс для блокировки потока
	std::mutex mtx;
	// Создаём условную переменную
	std::condition_variable cv;
	// Полученный сигнал
	int32_t signal = 0;
	// Флаг для отслеживания получения сигнала
	bool signaled = false;
	
	/**
	 * Устанавливаем обработчик сигнала
	 */
	this->_signals->on([&](const int32_t sig){
		std::unique_lock <std::mutex> lk(mtx);
		signal = sig;
		signaled = true;
		cv.notify_one();
	});

	// Запускаем отслеживание
	this->_signals->start();

	// Отправляем сигнал
	std::raise(this->_parameter.signal);

	// Ожидаем получения сигнала
	{
		// Формируем блокировку потока
		std::unique_lock <std::mutex> lock(mtx);
		// Ожидаем сигнал с таймаутом
		if(cv.wait_for(lock, std::chrono::seconds(2), [&]{return signaled;}))
			// Проверяем что полученный сигнал совпадает с отправленным
			ASSERT_EQ(this->_parameter.signal, signal);
		// Если таймаут истёк
		else FAIL() << "Timeout waiting for signal " << this->_parameter.signal;
	}
}

/**
 * @brief Инициализация параметров теста
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, SignalsTestParameterizedFixture,
	::testing::Values(
		SignalsTestParameter({SIGABRT}),
		SignalsTestParameter({SIGINT}),
		SignalsTestParameter({SIGFPE}),
		SignalsTestParameter({SIGILL}),
		SignalsTestParameter({SIGSEGV}),
		/**
		 * Сигнал SIGBUS отсутствует на MS Windows
		 */
		#if !_WIN32 && !_WIN64
			SignalsTestParameter({SIGBUS}),
		#endif
		SignalsTestParameter({SIGTERM})
	)
);
