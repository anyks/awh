/**
 * @file: static.cpp
 * @date: 2026-03-30
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
 * Подключаем заголовочный файлы проекта
 */
#include "uri.hpp"

/**
 * @brief Тест создания объекта работы с URI
 *
 */
TEST_F(UriFixture, CreateUriTest){
	// Проверяем, что объект работы с URI создан
	ASSERT_TRUE(this->_uri != nullptr);
	// Сбрасываем объект работы с URI
	this->_uri.reset();
	// Проверяем, что объект работы с URI сброшен
	ASSERT_TRUE(this->_uri == nullptr);
}

/**
 * @brief Тест сброса и повторного создания объекта работы с URI
 *
 */
TEST_F(UriFixture, ResetAndCreateUriTest){
	// Проверяем, что объект работы с URI создан
	ASSERT_TRUE(this->_uri != nullptr);
	// Сбрасываем объект работы с URI
	this->_uri.reset();
	// Проверяем, что объект работы с URI сброшен
	ASSERT_TRUE(this->_uri == nullptr);
	// Создаём объект работы с URI заново
	this->_uri = std::make_unique <awh::uri_t> (this->_fmk.get(), this->_log.get());
	// Проверяем, что объект работы с URI создан
	ASSERT_TRUE(this->_uri != nullptr);
}

/**
 * @brief Тест повторного создания объекта работы с URI
 *
 */
TEST_F(UriFixture, ReCreateUriTest){
	// Проверяем, что объект работы с URI создан
	ASSERT_TRUE(this->_uri != nullptr);
	// Создаём объект работы с URI заново
	this->_uri = std::make_unique <awh::uri_t> (this->_fmk.get(), this->_log.get());
	// Проверяем, что объект работы с URI создан
	ASSERT_TRUE(this->_uri != nullptr);
}

/**
 * @brief Тест декодирования процент-последовательностей логина и пароля при парсинге
 *
 * @note Регрессионный тест: компоненты userinfo должны декодироваться так же, как остальные части URI
 */
TEST_F(UriFixture, ParseUserinfoPercentDecodeTest){
	// Выполняем очистку объекта работы с URI
	this->_uri->clear();
	// Выполняем парсинг URI с процент-кодированными логином и паролем (%40 == '@')
	ASSERT_EQ(awh::uri_t::type_t::HTTP, this->_uri->parse("http://user%40name:p%40ss@example.com/"));
	// Извлекаем параметры пользователя URI
	const auto & user = this->_uri->user();
	// Проверяем, что логин пользователя декодирован
	ASSERT_EQ("user@name", user.username);
	// Проверяем, что пароль пользователя декодирован
	ASSERT_EQ("p@ss", user.password);
}

/**
 * @brief Тест отбраковки порта, выходящего за пределы диапазона uint16_t, при парсинге
 *
 * @note Регрессионный тест: порт > 65535 не должен молча усекаться (99999 mod 65536 == 34463)
 */
TEST_F(UriFixture, ParsePortRangeOverflowTest){
	// Выполняем очистку объекта работы с URI
	this->_uri->clear();
	// Выполняем парсинг URI с портом, выходящим за пределы диапазона uint16_t
	ASSERT_EQ(awh::uri_t::type_t::HTTP, this->_uri->parse("http://example.com:99999/"));
	// Порт игнорируется, поэтому возвращается стандартный порт схемы HTTP (а не усечённое значение)
	ASSERT_EQ(80, this->_uri->port());
	// Проверяем, что усечённое значение не попало в результат
	ASSERT_NE(34463, this->_uri->port());
}

/**
 * @brief Тест корректной обработки некорректных и обрезанных процент-последовательностей при декодировании
 *
 * @note Регрессионный тест: завершающий '%' не должен приводить к чтению за границами буфера
 */
TEST_F(UriFixture, DecodeMalformedPercentNoCrashTest){
	// Выполняем очистку объекта работы с URI
	this->_uri->clear();
	// Завершающий '%' без двух шестнадцатеричных цифр должен оставаться литералом и не вызывать выход за границы
	ASSERT_EQ(awh::uri_t::type_t::HTTP, this->_uri->parse("http://example.com/path%"));
	// Проверяем хост URI
	ASSERT_EQ("example.com", this->_uri->host());
	// Извлекаем путь URI
	const auto & path = this->_uri->path();
	// Проверяем, что путь состоит из одного сегмента
	ASSERT_EQ(1u, path.size());
	// Проверяем, что некорректная процент-последовательность сохранена как литерал
	ASSERT_EQ("path%", path[0]);
	// Выполняем очистку объекта работы с URI
	this->_uri->clear();
	// Корректные процент-последовательности по-прежнему декодируются (%41 == 'A', %42 == 'B')
	ASSERT_EQ(awh::uri_t::type_t::HTTP, this->_uri->parse("http://example.com/%41%42"));
	// Извлекаем путь URI
	const auto & decoded = this->_uri->path();
	// Проверяем, что путь состоит из одного сегмента
	ASSERT_EQ(1u, decoded.size());
	// Проверяем, что корректная процент-последовательность декодирована
	ASSERT_EQ("AB", decoded[0]);
}

/**
 * @brief Тест генерации запроса для E-mail URI без атрибутов хоста
 *
 * @note Регрессионный тест: print(REQUEST) для EMAIL без атрибутов не должен разыменовывать нулевой указатель
 */
TEST_F(UriFixture, EmailRequestNoHostNoCrashTest){
	// Выполняем очистку объекта работы с URI
	this->_uri->clear();
	// Устанавливаем схему mailto, что задаёт тип EMAIL, но не инициализирует атрибуты хоста
	this->_uri->scheme("mailto");
	// Проверяем, что тип URI определён как E-mail
	ASSERT_EQ(awh::uri_t::type_t::EMAIL, this->_uri->type());
	// Генерация запроса не должна приводить к падению при отсутствии атрибутов хоста
	const std::string result = this->_uri->print(awh::uri_t::item_t::REQUEST);
	// При отсутствии логина и хоста результат должен быть пустым
	ASSERT_TRUE(result.empty());
}

/**
 * @brief Тест чувствительности к регистру при сравнении URI
 *
 * @note Регрессионный тест: схема и хост сравниваются без учёта регистра, путь — с учётом регистра (RFC 3986)
 */
TEST_F(UriFixture, CaseSensitiveComparisonTest){
	// Создаём два объекта работы с URI
	awh::uri_t a(this->_fmk.get(), this->_log.get());
	awh::uri_t b(this->_fmk.get(), this->_log.get());
	// Схема и хост различаются только регистром — URI должны считаться равными
	a.parse("HTTP://WWW.EXAMPLE.COM/path");
	b.parse("http://www.example.com/path");
	// Проверяем равенство URI, различающихся только регистром схемы и хоста
	ASSERT_TRUE(a == b);
	// Проверяем отсутствие неравенства
	ASSERT_FALSE(a != b);
	// Создаём ещё два объекта работы с URI
	awh::uri_t c(this->_fmk.get(), this->_log.get());
	awh::uri_t d(this->_fmk.get(), this->_log.get());
	// Пути различаются только регистром — URI должны считаться разными (путь регистрозависим)
	c.parse("http://example.com/Path");
	d.parse("http://example.com/path");
	// Проверяем неравенство URI, различающихся регистром пути
	ASSERT_FALSE(c == d);
	// Проверяем наличие неравенства
	ASSERT_TRUE(c != d);
	// Создаём ещё два объекта работы с URI
	awh::uri_t e(this->_fmk.get(), this->_log.get());
	awh::uri_t f(this->_fmk.get(), this->_log.get());
	// Якоря различаются только регистром — URI должны считаться разными (якорь регистрозависим)
	e.parse("http://example.com/path#Frag");
	f.parse("http://example.com/path#frag");
	// Проверяем неравенство URI, различающихся регистром якоря
	ASSERT_FALSE(e == f);
	// Проверяем наличие неравенства
	ASSERT_TRUE(e != f);
}

/**
 * @brief Тест сохранения нестандартного порта при генерации SMART-формата для URI без определённого типа
 *
 * @note Регрессионный тест: для типа NONE нестандартный порт не должен теряться при генерации SMART
 */
TEST_F(UriFixture, SmartNonStandardPortForUntypedUriTest){
	// Выполняем очистку объекта работы с URI
	this->_uri->clear();
	// Выполняем парсинг хоста с нестандартным портом без схемы (тип URI остаётся неопределённым)
	ASSERT_EQ(awh::uri_t::type_t::NONE, this->_uri->parse("example.com:8080"));
	// Проверяем хост URI
	ASSERT_EQ("example.com", this->_uri->host());
	// Проверяем порт URI
	ASSERT_EQ(8080, this->_uri->port());
	// Нестандартный порт должен сохраняться в SMART-формате, так как стандартный порт для типа не определён
	ASSERT_EQ("example.com:8080", this->_uri->print(awh::uri_t::item_t::URI, awh::uri_t::format_t::SMART));
}
