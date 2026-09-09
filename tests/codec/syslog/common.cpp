/**
 * @file common.cpp
 * @date 2026-09-07
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
 * @brief Автоматические тесты общих определений контейнера SysLog — текстов сообщений об отказах,
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
#include <codec/syslog/syslog.hpp>

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
 * @details Проверка эта заведена потому, что часть модуля `common` иначе не поверялась
 *          бы вовсе: сборка её в набор была бы, а спроса на неё - нет. Молчание
 *          непроверенной части означает не здоровье её, а отсутствие спроса
 *
 */
TEST(CodecSysLogCommon, Messages) {
	// Перечень уже виденных текстов сообщений об отказах
	set <string> seen;
	/**
	 * Выполняем перебор всех кодов отказов разбора
	 */
	for(uint8_t code = 0; code <= static_cast <uint8_t> (syslog::error_t::FILE_NOT_READ); code++){
		// Получаем текст сообщения об очередном отказе
		const string message(syslog::message(static_cast <syslog::error_t> (code)));
		// Выполняем проверку непустоты текста сообщения об отказе
		EXPECT_FALSE(message.empty()) << "код отказа: " << static_cast <uint32_t> (code);
		// Выполняем проверку того, что коду отвечает свой текст, а не общая заглушка
		EXPECT_NE(message, "unknown error") << "код отказа: " << static_cast <uint32_t> (code);
		// Выполняем проверку неповторимости текста сообщения об отказе
		EXPECT_TRUE(seen.insert(message).second) << "повтор текста у кода: " << static_cast <uint32_t> (code);
	}
	/**
	 * Выполняем проверку выдачи заглушки коду, ЗА ГРАНИЦЕЙ перечня стоящему
	 *
	 * @details Довод здесь не 0xFF, а «граница плюс один», и разница эта решает всё:
	 * заглушка у 0xFF стоит на месте при всяком новом коде, а заглушка сразу за
	 * границей ПАДАЕТ, коль скоро код заведён, а граница перебора выше за ним не
	 * подвинута. Тем молчаливый пропуск нового кода обращается в красное
	 *
	 * @note Сторож этот стережёт и обратное - что перечень СПЛОШНОЙ до самой границы:
	 *       пропуск внутри дал бы «unknown error» посреди перебора выше
	 *
	 * @warning Заведено 09.09.2026 по указанию Василия, у какого сторож этот стоит во
	 *          всех трёх его наборах. Собственная беда того же дня: код FILE_NOT_READ
	 *          заведён, а граница перебора осталась именем FILE_NOT_OPENED, и новый код
	 *          не поверялся вовсе. Выдала это лишь карта покрытия - `common.cpp` упал со
	 *          100% до 97.22%, - а не разум и не прогон
	 */
	EXPECT_STREQ(syslog::message(static_cast <syslog::error_t> (static_cast <uint32_t> (syslog::error_t::FILE_NOT_READ) + 1)), "unknown error");
}

/**
 * @brief Проверка неизменности пределов, описанием заданных
 *
 * @details Пределы эти взяты не из головы, а из RFC 5424, раздел 6: правка любого из
 *          них означает расхождение с описанием, и проверка эта её замечает
 *
 */
TEST(CodecSysLogCommon, Limits) {
	// Выполняем проверку предела номера описания записи
	EXPECT_EQ(syslog::MAX_VERSION, 1u);
	// Выполняем проверку предела приоритета: источник 23, важность 7
	EXPECT_EQ(syslog::MAX_PRIORITY, 191u);
	// Выполняем проверку предела длины имени узла
	EXPECT_EQ(syslog::MAX_HOSTNAME, 255u);
	// Выполняем проверку предела длины названия приложения
	EXPECT_EQ(syslog::MAX_APPLICATION, 48u);
	// Выполняем проверку предела длины опознавателя работы
	EXPECT_EQ(syslog::MAX_PROCESS, 128u);
	// Выполняем проверку предела длины опознавателя сообщения
	EXPECT_EQ(syslog::MAX_MESSAGE_ID, 32u);
	// Выполняем проверку предела длины имени структурированных данных
	EXPECT_EQ(syslog::MAX_NAME, 32u);
	// Выполняем проверку того, что предел приоритета отвечает пределам частей его
	EXPECT_EQ(syslog::MAX_PRIORITY, ((static_cast <uint32_t> (syslog::facility_t::LOCAL7) * 8) +
	                                  static_cast <uint32_t> (syslog::severity_t::DEBUG)));
}

/**
 * @brief Проверка неизменности знаков, описанием назначенных
 *
 */
TEST(CodecSysLogCommon, Markers) {
	// Выполняем проверку метки порядка байтов
	EXPECT_EQ(syslog::BOM, "\xEF\xBB\xBF");
	// Выполняем проверку знака отсутствующего значения
	EXPECT_EQ(syslog::NIL, "-");
}

/**
 * @brief Проверка имён источников сообщений и степеней их важности
 *
 * @details Имена взяты из RFC 5424, таблицы 1 и 2, и по ним пишутся правила отбора у
 *          служб журналов: расхождение сделало бы правила несовместимыми
 *
 */
TEST(CodecSysLogCommon, Names) {
	// Перечень уже виденных имён источников сообщений
	set <string> seen;
	/**
	 * Выполняем перебор всех источников сообщений
	 */
	for(uint8_t code = 0; code <= static_cast <uint8_t> (syslog::facility_t::LOCAL7); code++){
		// Получаем имя очередного источника сообщения
		const string name(syslog::name(static_cast <syslog::facility_t> (code)));
		// Выполняем проверку непустоты имени источника сообщения
		EXPECT_FALSE(name.empty()) << "код источника: " << static_cast <uint32_t> (code);
		// Выполняем проверку неповторимости имени источника сообщения
		EXPECT_TRUE(seen.insert(name).second) << "повтор имени у источника: " << static_cast <uint32_t> (code);
	}
	// Выполняем проверку имён, описанием поимённо названных
	EXPECT_STREQ(syslog::name(syslog::facility_t::KERNEL), "kern");
	// Выполняем проверку имени источника службы журнала
	EXPECT_STREQ(syslog::name(syslog::facility_t::SYSLOG), "syslog");
	// Выполняем проверку имени источника службы печати
	EXPECT_STREQ(syslog::name(syslog::facility_t::PRINTER), "lpr");
	// Выполняем проверку имени последнего набора местного употребления
	EXPECT_STREQ(syslog::name(syslog::facility_t::LOCAL7), "local7");
	// Выполняем очистку перечня виденных имён
	seen.clear();
	/**
	 * Выполняем перебор всех степеней важности сообщений
	 */
	for(uint8_t code = 0; code <= static_cast <uint8_t> (syslog::severity_t::DEBUG); code++){
		// Получаем имя очередной степени важности сообщения
		const string name(syslog::name(static_cast <syslog::severity_t> (code)));
		// Выполняем проверку непустоты имени степени важности сообщения
		EXPECT_FALSE(name.empty()) << "код важности: " << static_cast <uint32_t> (code);
		// Выполняем проверку неповторимости имени степени важности сообщения
		EXPECT_TRUE(seen.insert(name).second) << "повтор имени у важности: " << static_cast <uint32_t> (code);
	}
	// Выполняем проверку имени наивысшей степени важности
	EXPECT_STREQ(syslog::name(syslog::severity_t::EMERGENCY), "emerg");
	// Выполняем проверку имени условия отказа
	EXPECT_STREQ(syslog::name(syslog::severity_t::ERROR), "err");
	// Выполняем проверку имени наинизшей степени важности
	EXPECT_STREQ(syslog::name(syslog::severity_t::DEBUG), "debug");
	// Выполняем проверку выдачи пустоты источнику, перечню неведомому
	EXPECT_STREQ(syslog::name(static_cast <syslog::facility_t> (0xFF)), "");
	// Выполняем проверку выдачи пустоты важности, перечню неведомой
	EXPECT_STREQ(syslog::name(static_cast <syslog::severity_t> (0xFF)), "");
}

/**
 * @brief Проверка неизменности числовых значений перечня описаний
 *
 * @details Значения эти выданы наружу и разбираются потребителем: правка их молча
 *          сменила бы смысл записанного им числа
 *
 */
TEST(CodecSysLogCommon, StandardOrder) {
	// Выполняем проверку значения самоопределения описания
	EXPECT_EQ(static_cast <uint8_t> (syslog::standard_t::AUTO), 0x00);
	// Выполняем проверку значения устаревшего описания
	EXPECT_EQ(static_cast <uint8_t> (syslog::standard_t::RFC3164), 0x01);
	// Выполняем проверку значения нынешнего описания
	EXPECT_EQ(static_cast <uint8_t> (syslog::standard_t::RFC5424), 0x02);
}

/**
 * @brief Проверка порядка полей заголовка записи
 *
 * @details Порядок членов перечня отвечает порядку полей в записи RFC 5424: поле
 *          кладётся в дерево по счёту, и перестановка членов положила бы значение поля
 *          под чужое имя - молча
 *
 */
TEST(CodecSysLogCommon, FieldOrder) {
	// Выполняем проверку неопределённого поля заголовка
	EXPECT_EQ(static_cast <uint8_t> (syslog::field_t::NONE), 0x00);
	// Выполняем проверку места приоритета в порядке полей
	EXPECT_EQ(static_cast <uint8_t> (syslog::field_t::PRIORITY), 0x01);
	// Выполняем проверку места номера описания в порядке полей
	EXPECT_EQ(static_cast <uint8_t> (syslog::field_t::VERSION), 0x02);
	// Выполняем проверку места даты сообщения в порядке полей
	EXPECT_EQ(static_cast <uint8_t> (syslog::field_t::TIMESTAMP), 0x03);
	// Выполняем проверку места имени узла в порядке полей
	EXPECT_EQ(static_cast <uint8_t> (syslog::field_t::HOSTNAME), 0x04);
	// Выполняем проверку места названия приложения в порядке полей
	EXPECT_EQ(static_cast <uint8_t> (syslog::field_t::APPLICATION), 0x05);
	// Выполняем проверку места опознавателя работы в порядке полей
	EXPECT_EQ(static_cast <uint8_t> (syslog::field_t::PROCESS), 0x06);
	// Выполняем проверку места опознавателя сообщения в порядке полей
	EXPECT_EQ(static_cast <uint8_t> (syslog::field_t::MESSAGE_ID), 0x07);
}
