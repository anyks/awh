/**
 * @file: static.cpp
 * @date: 2025-12-07
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "fmk.hpp"

/**
 * @brief Метод настройки тестового окружения
 *
 */
TEST_F(FmkFixture, CreateFmkTest){
	// Если объект фреймворка уже создан
	ASSERT_TRUE(this->_fmk != nullptr);
	// Сбрасываем объект фреймворка
	this->_fmk.reset();
	// Проверяем что объект фреймворка сброшен
	ASSERT_TRUE(this->_fmk == nullptr);
}

/**
 * @brief Метод сброса и повторного создания объекта фреймворка
 *
 */
TEST_F(FmkFixture, ResetAndCreateFmkTest){
	// Если объект фреймворка уже создан
	ASSERT_TRUE(this->_fmk != nullptr);
	// Сбрасываем объект фреймворка
	this->_fmk.reset();
	// Проверяем что объект фреймворка сброшен
	ASSERT_TRUE(this->_fmk == nullptr);
	// Создаём объект фреймворка
	this->_fmk = std::make_unique <awh::fmk_t> ();
	// Проверяем что объект фреймворка создан
	ASSERT_TRUE(this->_fmk != nullptr);
}

/**
 * @brief Метод повторного создания объекта фреймворка
 *
 */
TEST_F(FmkFixture, ReCreateFmkTest){
	// Если объект фреймворка уже создан
	ASSERT_TRUE(this->_fmk != nullptr);
	// Создаём объект фреймворка
	this->_fmk = std::make_unique <awh::fmk_t> ();
	// Проверяем что объект фреймворка создан
	ASSERT_TRUE(this->_fmk != nullptr);
}

/**
 * @brief Метод тестирования установки бита в указанную позицию
 *
 */
TEST_F(FmkFixture, CaseFmkTest){
	// Если объект фреймворка уже создан
	ASSERT_TRUE(this->_fmk != nullptr);
	// Создаём объект фреймворка
	this->_fmk = std::make_unique <awh::fmk_t> ();
	// Проверяем что объект фреймворка создан
	ASSERT_TRUE(this->_fmk != nullptr);
	// Тестируем установку бита в указанную позицию
	ASSERT_EQ(this->_fmk->setBit <uint64_t> (2, 4), 8);
	// Тестируем проверку установленного бита
	ASSERT_TRUE(this->_fmk->isBit <uint64_t> (3, 8));
	// Тестируем сброс бита в указанной позиции
	ASSERT_EQ(this->_fmk->resetBit <uint64_t> (3, 8), 0);
	// Тестируем инверсию бита в указанной позиции
	ASSERT_EQ(this->_fmk->flipBit <uint64_t> (3, 8), 0);
}

/**
 * @brief Метод тестирования форматирования строк
 *
 */
TEST_F(FmkFixture, FormatFmkTest){
	// Если объект фреймворка уже создан
	ASSERT_TRUE(this->_fmk != nullptr);
	// Создаём объект фреймворка
	this->_fmk = std::make_unique <awh::fmk_t> ();
	// Проверяем что объект фреймворка создан
	ASSERT_TRUE(this->_fmk != nullptr);
	// Тестируем форматирование строк
	ASSERT_EQ("Hello World!!!", this->_fmk->format("%s %s!!!", "Hello", "World"));
	// Тестируем форматирование строк с числовыми параметрами
	ASSERT_EQ("Вашм присвоен идентификатор ID=984 и ID=586", this->_fmk->format("%s ID=%u и ID=%u", "Вашм присвоен идентификатор", 984, 586));
}

/**
 * @brief Метод тестирования установки доменных зон
 *
 */
TEST_F(FmkFixture, DomainZoneFmkTest){
	// Если объект фреймворка уже создан
	ASSERT_TRUE(this->_fmk != nullptr);
	// Создаём объект фреймворка
	this->_fmk = std::make_unique <awh::fmk_t> ();
	// Проверяем что объект фреймворка создан
	ASSERT_TRUE(this->_fmk != nullptr);
	// Тестируем установку доменных зон
	this->_fmk->domainZones({"anyks", "google", "yandex"});
	// Добавляем новую доменную зону
	this->_fmk->domainZone("goga");
	// Добавляем ещё одну доменную зону
	ASSERT_EQ(4, this->_fmk->domainZones().size());
	/**
	 * Проходим по всем доменным зонам
	 */
	for(auto & zone : this->_fmk->domainZones())
		// Проверяем что доменная зона установлена корректно
		ASSERT_TRUE((zone.compare("goga") == 0) || (zone.compare("anyks") == 0) || (zone.compare("google") == 0) || (zone.compare("yandex") == 0));
}

/**
 * @brief Метод тестирования иконок
 *
 */
TEST_F(FmkFixture, IconFmkTest){
	// Если объект фреймворка уже создан
	ASSERT_TRUE(this->_fmk != nullptr);
	// Создаём объект фреймворка
	this->_fmk = std::make_unique <awh::fmk_t> ();
	// Проверяем что объект фреймворка создан
	ASSERT_TRUE(this->_fmk != nullptr);
	// Тестируем иконки
	ASSERT_TRUE(!this->_fmk->icon().empty());
}

/**
 * @brief Метод тестирования идентификаторов
 *
 */
TEST_F(FmkFixture, IdentifierFmkTest){
	// Если объект фреймворка уже создан
	ASSERT_TRUE(this->_fmk != nullptr);
	// Создаём объект фреймворка
	this->_fmk = std::make_unique <awh::fmk_t> ();
	// Проверяем что объект фреймворка создан
	ASSERT_TRUE(this->_fmk != nullptr);
	// Тестируем идентификаторы
	ASSERT_EQ(this->_fmk->identifier(), 1);
	ASSERT_EQ(this->_fmk->identifier(), 2);
	ASSERT_EQ(this->_fmk->identifier(), 3);
	ASSERT_EQ(this->_fmk->identifier(), 4);
	ASSERT_EQ(this->_fmk->identifier(), 5);
}
