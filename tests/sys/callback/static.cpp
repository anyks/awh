/**
 * @file: static.cpp
 * @date: 2026-01-22
 * @license: GPL-3.0
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
 * Подключаем заголовочный файлы проекта
 */
#include "callback.hpp"

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
