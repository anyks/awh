/**
 * @file: static.cpp
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

/**
 * Стандартные заголовочные файлы
 */
#include <mutex>
#include <atomic>
#include <chrono>
#include <condition_variable>

/**
 * Системные заголовочные файлы для операционной системы не являющейся MS Windows
 */
#if !_WIN32 && !_WIN64
	#include <unistd.h>
	#include <sys/wait.h>
	#include <sys/resource.h>
#endif

/**
 * @brief Тест создания объекта сигналов
 *
 */
TEST_F(SignalsFixture, CreateSignalsTest){
	// Проверяем создание объекта
	ASSERT_TRUE(this->_signals != nullptr);
	
	// Сбрасываем объект
	this->_signals.reset();
	
	// Проверяем сброс объекта
	ASSERT_TRUE(this->_signals == nullptr);
}

/**
 * @brief Тест установки функции обратного вызова
 *
 */
TEST_F(SignalsFixture, SetCallbackTest){
	// Проверяем создание объекта
	ASSERT_TRUE(this->_signals != nullptr);

	// Устанавливаем пустой callback
	this->_signals->on([](const int32_t){});
	
	// Если бы был метод getCallback, мы бы проверили его, но здесь просто проверяем что метод вызывается без ошибок
	SUCCEED();
}

/**
 * @brief Тест запуска и остановки отслеживания
 *
 */
TEST_F(SignalsFixture, StartStopTest){
	// Проверяем создание объекта
	ASSERT_TRUE(this->_signals != nullptr);

	// Запускаем отслеживание
	this->_signals->start();
	
	// Останавливаем отслеживание
	this->_signals->stop();
	
	// Повторный запуск
	this->_signals->start();
	
	// Завершаем отслеживание, тест удачен
	SUCCEED();
}

/**
 * @brief Тест безопасной остановки без предварительного запуска
 *
 */
TEST_F(SignalsFixture, StopWithoutStartTest){
	// Проверяем создание объекта
	ASSERT_TRUE(this->_signals != nullptr);

	// Останавливаем отслеживание без предварительного запуска
	this->_signals->stop();
	// Повторная остановка также должна быть безопасной
	this->_signals->stop();

	// Тест удачен, если не произошло аварийного завершения
	SUCCEED();
}

/**
 * @brief Тест идемпотентности повторного запуска отслеживания
 *
 */
TEST_F(SignalsFixture, DoubleStartTest){
	// Проверяем создание объекта
	ASSERT_TRUE(this->_signals != nullptr);

	// Создаём мьютекс для блокировки потока
	std::mutex mtx;
	// Создаём условную переменную
	std::condition_variable cv;
	// Количество полученных сигналов
	int32_t count = 0;

	/**
	 * Устанавливаем обработчик сигнала
	 */
	this->_signals->on([&](const int32_t){
		// Формируем блокировку потока
		std::unique_lock <std::mutex> lock(mtx);
		// Увеличиваем счётчик полученных сигналов
		++count;
		// Пробуждаем ожидающий поток
		cv.notify_one();
	});

	// Запускаем отслеживание дважды подряд
	this->_signals->start();
	// Повторный запуск должен быть проигнорирован
	this->_signals->start();

	// Отправляем сигнал
	std::raise(SIGINT);

	{
		// Формируем блокировку потока
		std::unique_lock <std::mutex> lock(mtx);
		// Ожидаем получения сигнала с таймаутом
		ASSERT_TRUE(cv.wait_for(lock, std::chrono::seconds(2), [&]{return (count >= 1);}));
	}
}

/**
 * @brief Тест повторной доставки сигнала после перезапуска отслеживания
 *
 */
TEST_F(SignalsFixture, RestartDeliveryTest){
	// Проверяем создание объекта
	ASSERT_TRUE(this->_signals != nullptr);

	// Создаём мьютекс для блокировки потока
	std::mutex mtx;
	// Создаём условную переменную
	std::condition_variable cv;
	// Количество полученных сигналов
	int32_t count = 0;
	// Последний полученный сигнал
	int32_t last = 0;

	/**
	 * Устанавливаем обработчик сигнала
	 */
	this->_signals->on([&](const int32_t sig){
		// Формируем блокировку потока
		std::unique_lock <std::mutex> lock(mtx);
		// Запоминаем последний полученный сигнал
		last = sig;
		// Увеличиваем счётчик полученных сигналов
		++count;
		// Пробуждаем ожидающий поток
		cv.notify_one();
	});

	// Запускаем первый цикл отслеживания
	this->_signals->start();
	// Отправляем сигнал
	std::raise(SIGINT);
	{
		// Формируем блокировку потока
		std::unique_lock <std::mutex> lock(mtx);
		// Ожидаем получения первого сигнала
		ASSERT_TRUE(cv.wait_for(lock, std::chrono::seconds(2), [&]{return (count >= 1);}));
	}

	// Останавливаем отслеживание
	this->_signals->stop();

	// Запускаем второй цикл отслеживания с переустановкой обработчиков
	this->_signals->start();
	// Повторно отправляем сигнал
	std::raise(SIGINT);
	{
		// Формируем блокировку потока
		std::unique_lock <std::mutex> lock(mtx);
		// Ожидаем получения второго сигнала
		ASSERT_TRUE(cv.wait_for(lock, std::chrono::seconds(2), [&]{return (count >= 2);}));
	}

	// Проверяем корректность последнего полученного сигнала
	ASSERT_EQ(SIGINT, last);
}

/**
 * @brief Тест остановки отслеживания из функции обратного вызова
 *
 */
TEST_F(SignalsFixture, StopFromCallbackTest){
	// Проверяем создание объекта
	ASSERT_TRUE(this->_signals != nullptr);

	// Создаём мьютекс для блокировки потока
	std::mutex mtx;
	// Создаём условную переменную
	std::condition_variable cv;
	// Флаг завершения обработки колбэка
	bool done = false;

	/**
	 * Устанавливаем обработчик сигнала, который останавливает отслеживание из самого колбэка.
	 * Проверяется отсутствие самоблокировки при вызове stop() из рабочего потока.
	 */
	this->_signals->on([&](const int32_t){
		// Останавливаем отслеживание из контекста рабочего потока
		this->_signals->stop();
		// Формируем блокировку потока
		std::unique_lock <std::mutex> lock(mtx);
		// Устанавливаем флаг завершения
		done = true;
		// Пробуждаем ожидающий поток
		cv.notify_one();
	});

	// Запускаем отслеживание
	this->_signals->start();

	// Отправляем сигнал
	std::raise(SIGINT);

	{
		// Формируем блокировку потока
		std::unique_lock <std::mutex> lock(mtx);
		// Ожидаем завершения обработки колбэка без самоблокировки
		ASSERT_TRUE(cv.wait_for(lock, std::chrono::seconds(2), [&]{return done;}));
	}
}

/**
 * @brief Тест передачи корректного номера сигнала в функцию обратного вызова
 *
 */
TEST_F(SignalsFixture, CallbackSignalValueTest){
	// Проверяем создание объекта
	ASSERT_TRUE(this->_signals != nullptr);

	// Создаём мьютекс для блокировки потока
	std::mutex mtx;
	// Создаём условную переменную
	std::condition_variable cv;
	// Полученный сигнал
	int32_t signal = 0;
	// Флаг получения сигнала
	bool signaled = false;

	/**
	 * Устанавливаем обработчик сигнала
	 */
	this->_signals->on([&](const int32_t sig){
		// Формируем блокировку потока
		std::unique_lock <std::mutex> lock(mtx);
		// Запоминаем полученный сигнал
		signal = sig;
		// Устанавливаем флаг получения сигнала
		signaled = true;
		// Пробуждаем ожидающий поток
		cv.notify_one();
	});

	// Запускаем отслеживание
	this->_signals->start();

	// Отправляем сигнал завершения работы
	std::raise(SIGTERM);

	{
		// Формируем блокировку потока
		std::unique_lock <std::mutex> lock(mtx);
		// Ожидаем получения сигнала
		ASSERT_TRUE(cv.wait_for(lock, std::chrono::seconds(2), [&]{return signaled;}));
		// Проверяем корректность номера полученного сигнала
		ASSERT_EQ(SIGTERM, signal);
	}
}

/**
 * Процессный тест фатального сигнала доступен только на UNIX-подобных системах
 */
#if !_WIN32 && !_WIN64
	/**
	 * @brief Тест обработки реального фатального сигнала в отдельном процессе
	 *
	 * Тест провоцирует реальное нарушение сегментации в дочернем процессе и проверяет
	 * корректность завершения в зависимости от режима сборки:
	 *  - в режиме отладки процесс завершается сигналом (с возможным core dump);
	 *  - в режиме релиза сигнал обрабатывается грациозно через функцию обратного вызова,
	 *    и процесс завершается штатно с заданным кодом выхода.
	 */
	TEST_F(SignalsFixture, FatalSignalProcessTest){
		// Создаём дочерний процесс для безопасной проверки фатального сигнала
		const pid_t pid = ::fork();
		// Проверяем успешность создания дочернего процесса
		ASSERT_NE(-1, pid);
		// Если выполняется дочерний процесс
		if(pid == 0){
			// Запрещаем создание core dump в дочернем процессе
			struct rlimit limit = {0, 0};
			// Устанавливаем нулевой лимит на размер core dump
			::setrlimit(RLIMIT_CORE, &limit);
			// Создаём локальный объект работы с сигналами
			awh::signals_t signals(nullptr, nullptr);
			// Устанавливаем обработчик сигнала
			signals.on([](const int32_t){
				/**
				 * В режиме релиза фатальный сигнал обрабатывается грациозно: приложение
				 * завершается с заданным кодом из функции обратного вызова.
				 */
				#if !DEBUG_MODE
					// Завершаем процесс с кодом грациозной обработки
					::_exit(42);
				#endif
			});
			// Запускаем отслеживание сигналов
			signals.start();
			// Формируем недопустимый указатель
			volatile int * ptr = nullptr;
			/**
			 *  Провоцируем реальный сигнал нарушения сегментации
			 */
			for(;;)
				// Выполняем запись по недопустимому адресу
				*ptr = 0xDEAD;
			// Если процесс не завершился ожидаемым образом, выходим с кодом ошибки
			::_exit(1);
		}
		// Статус завершения дочернего процесса
		int32_t status = 0;
		// Ожидаем завершения дочернего процесса
		ASSERT_EQ(pid, ::waitpid(pid, &status, 0));
		/**
		 * В режиме отладки фатальный сигнал приводит к аварийному завершению процесса.
		 */
		#if DEBUG_MODE
			// Проверяем, что процесс завершён сигналом, а не штатно
			ASSERT_TRUE(WIFSIGNALED(status));
			// Проверяем, что процесс завершён сигналом нарушения доступа к памяти
			ASSERT_TRUE((WTERMSIG(status) == SIGSEGV) || (WTERMSIG(status) == SIGBUS));
		/**
		 * В режиме релиза фатальный сигнал обрабатывается грациозно через колбэк.
		 */
		#else
			// Проверяем, что процесс завершён штатно
			ASSERT_TRUE(WIFEXITED(status));
			// Проверяем корректность кода выхода грациозного завершения
			ASSERT_EQ(42, WEXITSTATUS(status));
		#endif
	}
#endif
