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
 * @brief Параметризованные тесты модуля обработки сигналов — прогон подготовленных наборов входных данных через
 *        методы модуля с проверкой установки и снятия обработчиков сигналов и доставки события пользовательскому коду
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл
 */
#include "signals.hpp"
#include <mutex>
#include <thread>
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

	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		/**
		 * Сигнал поднимается прямо здесь намеренно
		 *
		 * @details Обработчики сигналов SIGFPE, SIGILL и SIGSEGV у библиотеки времени
		 *          исполнения MS Windows **потоковые**, а не общие на процесс:
		 *          поставленный одним потоком, у другого он не действует вовсе, и
		 *          поднятый там сигнал уходит к обработке по умолчанию - процесс
		 *          гибнет с кодом 3, не позвав обработчика. Проверено опытом
		 *          отдельной пробой вне библиотеки: SIGFPE, поднятый в том же потоке,
		 *          где поставлен обработчик, доходит до него, а поднятый в другом -
		 *          валит процесс
		 *
		 *          Парковки же, ради которой у POSIX сигнал поднимается из отдельного
		 *          потока, у MS Windows нет: обработчик там лишь ставит сигнал в
		 *          очередь рабочего потока и возвращается, оттого поднятие у себя
		 *          ожидания не срывает
		 *
		 */
		std::raise(this->_parameter.signal);
	/**
	 * Для операционных систем Linux, FreeBSD, NetBSD, OpenBSD, macOS и Solaris
	 */
	#else
		/**
		 * Сигнал поднимается из отдельного потока намеренно
		 *
		 * @details Обработчик модуля в выпускной сборке паркует поток, поднявший
		 *          фатальный синхронный сигнал, навсегда: без этого сбойная инструкция
		 *          исполнялась бы повторно без конца, а завершить процесс должен
		 *          рабочий поток. Подними проверка сигнал у себя - главный поток
		 *          встал бы прямо в `raise`, до ожидания дело не дошло бы, и набор
		 *          вис бы намертво, не дожидаясь даже собственного предела в две
		 *          секунды. Поднявший поток отвязывается, потому что вернуться из
		 *          обработчика он не обязан
		 */
		std::thread([signal = this->_parameter.signal]{
			// Отправляем сигнал из отвязанного потока
			std::raise(signal);
		}).detach();
	#endif

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
