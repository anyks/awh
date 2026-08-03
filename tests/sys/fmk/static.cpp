/**
 * @file: static.cpp
 * @date: 2025-12-07
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Статические тесты ядра фреймворка — проверка создания и сброса объекта модуля,
 *        а также корректности работы со строками и кодировками, смены регистра, форматирования и конвертации типов
 *
 * @copyright: Copyright © 2025
 *
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
/**
 * @brief Тест конвертирования строки между кодировками, заданными обозначением
 *
 */
TEST_F(FmkFixture, TranscodeEncodingFmkTest){
	// Записываем слово «Привет» в кодировке CP1251
	const std::string cp1251 = "\xCF\xF0\xE8\xE2\xE5\xF2";
	// Записываем слово «Привет» в кодировке KOI8-R
	const std::string koi8r = "\xF0\xD2\xC9\xD7\xC5\xD4";
	// Выполняем проверку конвертирования между однобайтовыми кодировками
	ASSERT_EQ(this->_fmk->transcode(cp1251, awh::charset::encoding_t::CP1251, awh::charset::encoding_t::KOI8_R), koi8r);
	ASSERT_EQ(this->_fmk->transcode(koi8r, awh::charset::encoding_t::KOI8_R, awh::charset::encoding_t::CP1251), cp1251);
	// Выполняем проверку конвертирования в UTF-8 и обратно
	ASSERT_EQ(this->_fmk->transcode(cp1251, awh::charset::encoding_t::CP1251, awh::charset::encoding_t::UTF8), "Привет");
	ASSERT_EQ(this->_fmk->transcode("Привет", awh::charset::encoding_t::UTF8, awh::charset::encoding_t::CP1251), cp1251);
	// Выполняем проверку конвертирования текста западных языков
	ASSERT_EQ(this->_fmk->transcode("München", awh::charset::encoding_t::UTF8, awh::charset::encoding_t::ISO8859_1), "M\xFCnchen");
	ASSERT_EQ(this->_fmk->transcode("M\xFCnchen", awh::charset::encoding_t::ISO8859_1, awh::charset::encoding_t::UTF8), "München");
	// Выполняем проверку отказа конвертирования непредставимого символа
	ASSERT_TRUE(this->_fmk->transcode("Привет", awh::charset::encoding_t::UTF8, awh::charset::encoding_t::ISO8859_1).empty());
	// Выполняем проверку замены непредставимых символов
	ASSERT_EQ(this->_fmk->transcode("aПb", awh::charset::encoding_t::UTF8,
		awh::charset::encoding_t::ISO8859_1, awh::charset::replace_t::REPLACE), "a?b");
}
/**
 * @brief Тест конвертирования строки между кодировками, заданными именем
 *
 */
TEST_F(FmkFixture, TranscodeNameFmkTest){
	// Записываем слово «Привет» в кодировке CP1251
	const std::string cp1251 = "\xCF\xF0\xE8\xE2\xE5\xF2";
	// Выполняем проверку конвертирования по именам кодировок
	ASSERT_EQ(this->_fmk->transcode(cp1251, "windows-1251", "utf-8"), "Привет");
	ASSERT_EQ(this->_fmk->transcode("Привет", "UTF-8", "CP1251"), cp1251);
	ASSERT_EQ(this->_fmk->transcode(cp1251, "windows-1251", "koi8-r"), "\xF0\xD2\xC9\xD7\xC5\xD4");
	// Выполняем проверку отказа при нераспознанном имени кодировки
	ASSERT_TRUE(this->_fmk->transcode(cp1251, "windows-9999", "utf-8").empty());
	ASSERT_TRUE(this->_fmk->transcode(cp1251, "windows-1251", "").empty());
}
/**
 * @brief Тест разбора имён кодировок и определения кодировки текста
 *
 */
TEST_F(FmkFixture, EncodingFmkTest){
	// Выполняем проверку разбора имён кодировок
	ASSERT_EQ(this->_fmk->encoding("windows-1251"), awh::charset::encoding_t::CP1251);
	ASSERT_EQ(this->_fmk->encoding("KOI8-R"), awh::charset::encoding_t::KOI8_R);
	ASSERT_EQ(this->_fmk->encoding("utf-8"), awh::charset::encoding_t::UTF8);
	// Выполняем проверку отказа разбора нераспознанного имени
	ASSERT_EQ(this->_fmk->encoding("windows-9999"), awh::charset::encoding_t::NONE);
	// Выполняем проверку извлечения имени кодировки по её обозначению
	ASSERT_EQ(this->_fmk->encoding(awh::charset::encoding_t::CP1251), "windows-1251");
	ASSERT_EQ(this->_fmk->encoding(awh::charset::encoding_t::UTF8), "UTF-8");
	// Выполняем проверку определения кодировки текста
	ASSERT_EQ(this->_fmk->detect("Привет"), awh::charset::encoding_t::UTF8);
	ASSERT_EQ(this->_fmk->detect("\xCF\xF0\xE8\xE2\xE5\xF2"), awh::charset::encoding_t::CP1251);
	ASSERT_EQ(this->_fmk->detect("\xF0\xD2\xC9\xD7\xC5\xD4", awh::charset::encoding_t::KOI8_R), awh::charset::encoding_t::KOI8_R);
}
