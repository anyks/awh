/**
 * @file common.cpp
 * @date 2026-09-04
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
 * @brief Автоматические тесты общих определений контейнера CEF — текстов сообщений об отказах,
 *        пределов разбора и неизменности числовых значений кодов, наружу выданных
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <set>
#include <string>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/cef/cef.hpp>

/**
 * Подключаем заголовочные файлы тестового окружения
 */
#include "../../main.hpp"

/**
 * Подавляем системные макросы, занявшие имена членов перечислений AWH
 */
#include <sys/macro/suppress.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh::codec;

/**
 * @brief Проверка текстов сообщений об отказах разбора
 *
 * @details Проверка эта заведена потому, что часть модуля `common` не имела проверок
 *          ВОВСЕ: сборка её в набор была, а поверялась она лишь косвенно - отказами
 *          прочих проверок. Молчание непроверенной части означает не здоровье её, а
 *          отсутствие спроса
 *
 * @note Подсказано Василием 04.09.2026: у него часть владеющего значения таблицы CSV
 *       не входила в стенд вовсе, и стенд восемь машин подряд отчитывался успехом
 *
 */
TEST(CodecCefCommon, Messages) {
	// Перечень уже виденных текстов сообщений об отказах
	set <string> seen;
	/**
	 * Выполняем перебор всех кодов отказов разбора
	 */
	for(uint8_t code = 0; code <= static_cast <uint8_t> (cef::error_t::FILE_NOT_OPENED); code++){
		// Получаем текст сообщения об очередном отказе
		const string message(cef::message(static_cast <cef::error_t> (code)));
		// Выполняем проверку непустоты текста сообщения об отказе
		EXPECT_FALSE(message.empty()) << "код отказа: " << static_cast <uint32_t> (code);
		// Выполняем проверку того, что коду отвечает свой текст, а не общая заглушка
		EXPECT_NE(message, "unknown error") << "код отказа: " << static_cast <uint32_t> (code);
		// Выполняем проверку неповторимости текста сообщения об отказе
		EXPECT_TRUE(seen.insert(message).second) << "повтор текста у кода: " << static_cast <uint32_t> (code);
	}
	// Выполняем проверку выдачи заглушки коду, перечню неведомому
	EXPECT_STREQ(cef::message(static_cast <cef::error_t> (0xFF)), "unknown error");
}

/**
 * @brief Проверка отсутствия отказа у успешного разбора
 *
 */
TEST(CodecCefCommon, NoError) {
	// Выполняем проверку текста сообщения об отсутствии отказа
	EXPECT_STREQ(cef::message(cef::error_t::NONE), "no error");
}

/**
 * @brief Проверка пределов разбора и постоянных записи
 *
 * @details Значения эти видны потребителю и входят в договор кодека: смена их без
 *          нужды ломает разбор у того, кто на них опёрся
 *
 */
TEST(CodecCefCommon, Constants) {
	// Выполняем проверку количества полей заголовка записи
	EXPECT_EQ(cef::HEADER_FIELDS, 7u);
	// Выполняем проверку наибольшей допустимой важности события
	EXPECT_EQ(cef::MAX_SEVERITY, 10u);
	// Выполняем проверку слова, заголовок записи открывающего
	EXPECT_EQ(cef::SIGNATURE, "CEF:");
	// Выполняем проверку окончания имени ключа, метку имени несущего
	EXPECT_EQ(cef::LABEL_SUFFIX, "Label");
	// Выполняем проверку записи даты, меткам времени назначенной
	EXPECT_EQ(cef::TIMESTAMP_FORMAT, "%b %d %Y %H:%M:%S %Z");
	// Выполняем проверку записи даты, метку с долей секунды несущую
	EXPECT_EQ(cef::TIMESTAMP_FRACTION_FORMAT, "%b %d %Y %H:%M:%S.%s %Z");
	// Выполняем проверку того, что записи даты РАЗНЫЕ: одна обоих видов не покрывает
	EXPECT_NE(cef::TIMESTAMP_FORMAT, cef::TIMESTAMP_FRACTION_FORMAT);
	// Выполняем проверку того, что предел записи не меньше предела поля заголовка
	EXPECT_GE(cef::MAX_RECORD, cef::MAX_HEADER_FIELD);
	// Выполняем проверку того, что предел поля заголовка не меньше предела имени ключа
	EXPECT_GE(cef::MAX_HEADER_FIELD, cef::MAX_NAME);
}

/**
 * @brief Проверка соответствия полей заголовка их порядку в записи
 *
 * @details Поле заголовка разбирается ПО СЧЁТУ, а не по имени: имён у полей заголовка
 *          сама запись CEF не несёт вовсе. Числовые значения перечня оттого есть
 *          указатель поля, и перестановка их сместила бы разбор всей записи
 *
 */
TEST(CodecCefCommon, FieldOrder) {
	// Выполняем проверку порядкового номера поля номера редакции записи
	EXPECT_EQ(static_cast <uint8_t> (cef::field_t::VERSION), 0);
	// Выполняем проверку порядкового номера поля поставщика устройства
	EXPECT_EQ(static_cast <uint8_t> (cef::field_t::VENDOR), 1);
	// Выполняем проверку порядкового номера поля изделия поставщика
	EXPECT_EQ(static_cast <uint8_t> (cef::field_t::PRODUCT), 2);
	// Выполняем проверку порядкового номера поля редакции изделия
	EXPECT_EQ(static_cast <uint8_t> (cef::field_t::RELEASE), 3);
	// Выполняем проверку порядкового номера поля опознавателя события
	EXPECT_EQ(static_cast <uint8_t> (cef::field_t::SIGNATURE), 4);
	// Выполняем проверку порядкового номера поля имени события
	EXPECT_EQ(static_cast <uint8_t> (cef::field_t::NAME), 5);
	// Выполняем проверку порядкового номера поля важности события
	EXPECT_EQ(static_cast <uint8_t> (cef::field_t::SEVERITY), 6);
	// Выполняем проверку того, что полей ровно столько, сколько объявлено постоянной
	EXPECT_EQ(static_cast <uint8_t> (cef::field_t::SEVERITY) + 1, static_cast <int> (cef::HEADER_FIELDS));
}

/**
 * Возвращаем имена, системными макросами занятые
 */
#include <sys/macro/restore.hpp>
