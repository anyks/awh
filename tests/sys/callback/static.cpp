/**
 * @file static.cpp
 * @date 2026-01-22
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
 * @brief Статические тесты модуля функций обратного вызова — проверка создания и сброса объекта модуля,
 *        а также корректности регистрации колбэков произвольных сигнатур,
 *        адресации по идентификатору и имени и их вызова
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "callback.hpp"

/**
 * Стандартные заголовочные файлы для многопоточного стресс-тестирования
 */
#include <array>
#include <atomic>
#include <thread>
#include <vector>
#include <cstdint>

/**
 * @brief Метод тестирования создания объекта обратного вызова
 *
 */
TEST_F(CallbackFixture, CreateCallbackTest){
	// Если объект обратного вызова уже создан
	ASSERT_TRUE(this->_callback != nullptr);
	// Проверяем что контейнер пуст
	ASSERT_TRUE(this->_callback->empty());
}

/**
 * @brief Метод тестирования добавления и вызова функции
 *
 */
TEST_F(CallbackFixture, OnAndCallTest){
	// Регистрируем функцию обратного вызова
	ASSERT_TRUE(this->_callback->on <int32_t()> ("test", []() -> int32_t { return 42; }) > 0);
	// Проверяем существование функции
	ASSERT_TRUE(this->_callback->is("test"));
	// Вызываем функцию и проверяем результат
	ASSERT_EQ(42, this->_callback->call <int32_t()> ("test"));
}

/**
 * @brief Метод тестирования удаления функции
 *
 */
TEST_F(CallbackFixture, EraseTest){
	// Регистрируем функцию
	this->_callback->on <void()> ("test", [](){});
	// Проверяем что функция существует
	ASSERT_TRUE(this->_callback->is("test"));

	// Удаляем функцию
	this->_callback->erase("test");
	// Проверяем что функции нет
	ASSERT_FALSE(this->_callback->is("test"));
}

/**
 * @brief Метод тестирования получения функции
 *
 */
TEST_F(CallbackFixture, GetTest){
	// Регистрируем функцию
	this->_callback->on <int32_t()> ("test", []() -> int32_t { return 100; });

	// Получаем функцию
	auto fn = this->_callback->get <int32_t()> ("test");

	// Проверяем что функция получена
	ASSERT_TRUE(fn != nullptr);
	// Вызываем полученную функцию
	ASSERT_EQ(100, fn());
}

/**
 * @brief Метод тестирования очистки контейнера
 *
 */
TEST_F(CallbackFixture, ClearTest){
	// Регистрируем функции
	this->_callback->on <void()> ("test1", [](){});
	this->_callback->on <void()> ("test2", [](){});

	// Проверяем что контейнер не пуст
	ASSERT_FALSE(this->_callback->empty());

	// Очищаем контейнер
	this->_callback->clear();

	// Проверяем что контейнер пуст
	ASSERT_TRUE(this->_callback->empty());
}

/**
 * @brief Метод тестирования обмена функциями
 *
 */
TEST_F(CallbackFixture, SwapTest){
	// Создаем второй контейнер
	awh::callback_t callback2(this->_fmk.get(), this->_log.get());

	// Регистрируем функцию в первом контейнере
	this->_callback->on <int32_t()> ("test1", []() -> int32_t { return 1; });
	// Регистрируем функцию во втором контейнере
	callback2.on <int32_t()> ("test2", []() -> int32_t { return 2; });

	// Меняем местами контейнеры
	this->_callback->swap(callback2);

	// Проверяем что функции поменялись местами
	ASSERT_FALSE(this->_callback->is("test1"));
	// Проверяем что функция существует в первом контейнере
	ASSERT_TRUE(this->_callback->is("test2"));
	// Проверяем что функция существует в другом контейнере
	ASSERT_TRUE(callback2.is("test1"));
	// Проверяем что функция существует в другом контейнере
	ASSERT_FALSE(callback2.is("test2"));
}

/**
 * @brief Метод тестирования событий обратного вызова
 *
 */
TEST_F(CallbackFixture, EventTest){
	// Флаги вызова событий
	bool setCalled = false;
	bool delCalled = false;
	bool runCalled = false;

	// Устанавливаем обработчик событий
	this->_callback->on([&](const awh::callback_t::event_t event, const awh::callback_t::id_t, const awh::callback_t::fn_t &){
		/**
		 * Обрабатываем событие
		 */
		switch(event){
			// Событие установки функции
			case awh::callback_t::event_t::SET:
				// Устанавливаем флаг вызова
				setCalled = true;
			break;
			// Событие удаления функции
			case awh::callback_t::event_t::DEL:
				// Устанавливаем флаг вызова
				delCalled = true;
			break;
			// Событие запуска функции
			case awh::callback_t::event_t::RUN:
				// Устанавливаем флаг вызова
				runCalled = true;
			break;
			// Другие события
			default: break;
		}
	});

	// Действие SET
	this->_callback->on <void()> ("eventCheck", [](){});
	// Проверяем что событие сработало
	ASSERT_TRUE(setCalled);

	// Действие RUN
	this->_callback->call <void()> ("eventCheck");
	// Проверяем что событие сработало
	ASSERT_TRUE(runCalled);

	// Действие DEL
	this->_callback->erase("eventCheck");
	// Проверяем что событие сработало
	ASSERT_TRUE(delCalled);
}

/**
 * @brief Многопоточный стресс-тест конкурентных on/call/erase/is при threadSafety(true)
 *
 */
TEST_F(CallbackFixture, ThreadSafetyStressTest){
	// Включаем режим потокобезопасности
	this->_callback->threadSafety(true);

	// Набор имён функций обратного вызова (короткие и длинные, разные ветки генерации идентификатора)
	static const std::array <const char *, 8> names = {
		"read", "write", "state", "error", "connect", "traffic", "timeout", "available"
	};

	// Счётчики выполненных операций
	std::atomic <uint64_t> calls{0};
	std::atomic <uint64_t> sets{0};
	std::atomic <uint64_t> erases{0};
	// Флаг обнаружения некорректного результата вызова функции обратного вызова
	std::atomic <bool> invalidResult{false};

	// Количество итераций на поток
	constexpr int32_t ITERATIONS = 20000;

	// Устанавливаем системный обработчик событий (стрессируем путь копирования системного callback)
	std::atomic <uint64_t> events{0};
	this->_callback->on([&](const awh::callback_t::event_t, const awh::callback_t::id_t, const awh::callback_t::fn_t &){
		// Увеличиваем счётчик системных событий
		events.fetch_add(1, std::memory_order_relaxed);
	});

	// Список рабочих потоков
	std::vector <std::thread> workers;

	/**
	 * Потоки-установщики функций обратного вызова
	 */
	for(int32_t t = 0; t < 2; t++){
		// Создаём поток-установщик функций обратного вызова
		workers.emplace_back([&, t](){
			/**
			 * Выполняем установку функций обратного вызова
			 */
			for(int32_t i = 0; i < ITERATIONS; i++){
				// Получаем имя функции обратного вызова
				const char * name = names[(i + t) % names.size()];
				// Выполняем установку функции обратного вызова
				this->_callback->on <int32_t ()> (name, [](){ return static_cast <int32_t> (42); });
				// Увеличиваем счётчик установок
				sets.fetch_add(1, std::memory_order_relaxed);
			}
		});
	}

	/**
	 * Потоки-вызыватели функций обратного вызова
	 */
	for(int32_t t = 0; t < 4; t++){
		// Создаём поток-вызыватель функций обратного вызова
		workers.emplace_back([&, t](){
			/**
			 * Выполняем вызовы функций обратного вызова
			 */
			for(int32_t i = 0; i < ITERATIONS; i++){
				// Получаем имя функции обратного вызова
				const char * name = names[((i * 3) + t) % names.size()];
				// Выполняем вызов функции обратного вызова (результат либо 42, либо 0, если функция отсутствует)
				const int32_t result = this->_callback->call <int32_t ()> (name);
				// Проверяем корректность возвращаемого значения (фиксируем некорректный результат для проверки после join)
				if((result != 42) && (result != 0))
					// Устанавливаем флаг некорректного результата
					invalidResult.store(true, std::memory_order_relaxed);
				// Увеличиваем счётчик вызовов
				calls.fetch_add(1, std::memory_order_relaxed);
			}
		});
	}

	/**
	 * Потоки-удалятели и проверятели функций обратного вызова
	 */
	for(int32_t t = 0; t < 2; t++){
		// Создаём поток-удалятор и проверятор функций обратного вызова
		workers.emplace_back([&, t](){
			/**
			 * Выполняем удаление и проверку функций обратного вызова
			 */
			for(int32_t i = 0; i < ITERATIONS; i++){
				// Получаем имя функции обратного вызова
				const char * name = names[((i * 5) + t) % names.size()];
				// Чередуем удаление и проверку существования функции обратного вызова
				if((i % 2) == 0){
					// Выполняем удаление функции обратного вызова
					this->_callback->erase(name);
					// Увеличиваем счётчик удалений
					erases.fetch_add(1, std::memory_order_relaxed);
				// Выполняем проверку существования функции обратного вызова
				} else (void) this->_callback->is(name);
			}
		});
	}

	/**
	 * Ожидаем завершения всех рабочих потоков
	 */
	for(auto & worker : workers)
		// Выполняем ожидание завершения потока
		worker.join();

	// Если мы дошли до этой точки без падений и дедлоков - тест успешен
	ASSERT_FALSE(invalidResult.load());
	ASSERT_GT(sets.load(), 0u);
	ASSERT_GT(calls.load(), 0u);
	ASSERT_GT(erases.load(), 0u);
}

/**
 * @brief Многопоточный стресс-тест встречного обмена двух контейнеров (проверка отсутствия AB-BA дедлока)
 *
 */
TEST_F(CallbackFixture, ThreadSafetySwapStressTest){
	// Создаём два контейнера функций обратного вызова
	awh::callback_t a(this->_fmk.get(), this->_log.get());
	awh::callback_t b(this->_fmk.get(), this->_log.get());

	// Включаем режим потокобезопасности для обоих контейнеров
	a.threadSafety(true);
	b.threadSafety(true);

	// Наполняем контейнеры функциями обратного вызова
	a.on <void ()> ("funcA", [](){});
	b.on <void ()> ("funcB", [](){});

	// Количество итераций обмена
	constexpr int32_t ITERATIONS = 50000;

	// Флаг одновременного старта потоков
	std::atomic <bool> go{false};

	// Поток встречного обмена A <-> B
	std::thread t1([&](){
		/**
		 *  Ожидаем сигнала старта
		 */
		while(!go.load(std::memory_order_acquire));
		/**
		 * Выполняем обмен контейнерами
		 */
		for(int32_t i = 0; i < ITERATIONS; i++)
			// Выполняем обмен A с B
			a.swap(b);
	});
	// Поток встречного обмена B <-> A
	std::thread t2([&](){
		/**
		 *  Ожидаем сигнала старта
		 */
		while(!go.load(std::memory_order_acquire));
		/**
		 * Выполняем обмен контейнерами
		 */
		for(int32_t i = 0; i < ITERATIONS; i++)
			// Выполняем обмен B с A (обратный порядок - проверяем отсутствие дедлока)
			b.swap(a);
	});

	// Запускаем потоки одновременно
	go.store(true, std::memory_order_release);

	// Ожидаем завершения потоков (если возникнет дедлок - тест зависнет)
	t1.join();
	t2.join();

	// Если мы дошли до этой точки - дедлока нет
	SUCCEED();
}

/**
 * @brief Многопоточный стресс-тест кросс-контейнерного set при одновременной модификации источника
 *
 */
TEST_F(CallbackFixture, ThreadSafetyCrossSetStressTest){
	// Создаём сторонний контейнер-источник функций обратного вызова
	awh::callback_t storage(this->_fmk.get(), this->_log.get());

	// Включаем режим потокобезопасности для обоих контейнеров
	this->_callback->threadSafety(true);
	storage.threadSafety(true);

	// Набор имён функций обратного вызова
	static const std::array <const char *, 6> names = {
		"alpha", "beta", "gamma", "delta", "read", "ping"
	};

	// Количество итераций на поток
	constexpr int32_t ITERATIONS = 20000;

	// Список рабочих потоков
	std::vector <std::thread> workers;

	// Поток, постоянно модифицирующий контейнер-источник
	workers.emplace_back([&](){
		/**
		 * Выполняем модификацию контейнера-источника
		 */
		for(int32_t i = 0; i < ITERATIONS; i++){
			// Получаем имя функции обратного вызова
			const char * name = names[i % names.size()];
			// Чередуем установку и удаление функции обратного вызова в источнике
			if((i % 3) != 0)
				// Выполняем установку функции обратного вызова
				storage.on <void ()> (name, [](){});
			// Выполняем удаление функции обратного вызова
			else storage.erase(name);
		}
	});

	/**
	 * Потоки, копирующие функции обратного вызова из источника в текущий контейнер
	 */
	for(int32_t t = 0; t < 3; t++){
		workers.emplace_back([&, t](){
			/**
			 * Выполняем копирование функций обратного вызова из источника
			 */
			for(int32_t i = 0; i < ITERATIONS; i++){
				// Получаем имя функции обратного вызова
				const char * name = names[((i * 2) + t) % names.size()];
				// Выполняем установку функции обратного вызова из стороннего контейнера
				(void) this->_callback->set(name, storage);
				// Выполняем вызов функции обратного вызова из текущего контейнера
				this->_callback->call <void ()> (name);
			}
		});
	}

	/**
	 * Ожидаем завершения всех рабочих потоков
	 */
	for(auto & worker : workers)
		// Выполняем ожидание завершения потока
		worker.join();

	// Если мы дошли до этой точки без падений и дедлоков - тест успешен
	SUCCEED();
}
