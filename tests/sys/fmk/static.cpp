/**
 * @file static.cpp
 * @date 2025-12-07
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
 * @brief Статические тесты ядра фреймворка — проверка создания и сброса объекта модуля,
 *        а также корректности работы со строками и кодировками, смены регистра, форматирования и конвертации типов
 *
 * @copyright Copyright © 2025
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include <clocale>

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
	ASSERT_EQ(this->_fmk->transcode(cp1251, awh::fmk_t::codepage_t::CP1251, awh::fmk_t::codepage_t::KOI8_R), koi8r);
	ASSERT_EQ(this->_fmk->transcode(koi8r, awh::fmk_t::codepage_t::KOI8_R, awh::fmk_t::codepage_t::CP1251), cp1251);
	// Выполняем проверку конвертирования в UTF-8 и обратно
	ASSERT_EQ(this->_fmk->transcode(cp1251, awh::fmk_t::codepage_t::CP1251, awh::fmk_t::codepage_t::UTF8), "Привет");
	ASSERT_EQ(this->_fmk->transcode("Привет", awh::fmk_t::codepage_t::UTF8, awh::fmk_t::codepage_t::CP1251), cp1251);
	// Выполняем проверку конвертирования текста западных языков
	ASSERT_EQ(this->_fmk->transcode("München", awh::fmk_t::codepage_t::UTF8, awh::fmk_t::codepage_t::ISO8859_1), "M\xFCnchen");
	ASSERT_EQ(this->_fmk->transcode("M\xFCnchen", awh::fmk_t::codepage_t::ISO8859_1, awh::fmk_t::codepage_t::UTF8), "München");
	// Выполняем проверку отказа конвертирования непредставимого символа
	ASSERT_TRUE(this->_fmk->transcode("Привет", awh::fmk_t::codepage_t::UTF8, awh::fmk_t::codepage_t::ISO8859_1).empty());
	// Выполняем проверку замены непредставимых символов
	ASSERT_EQ(this->_fmk->transcode("aПb", awh::fmk_t::codepage_t::UTF8,
		awh::fmk_t::codepage_t::ISO8859_1, awh::fmk_t::replace_t::REPLACE), "a?b");
}
/**
 * @brief Тест конвертирования строки по кодировкам, заданным именем
 *
 */
TEST_F(FmkFixture, TranscodeNameFmkTest){
	// Записываем слово «Привет» в кодировке CP1251
	const std::string cp1251 = "\xCF\xF0\xE8\xE2\xE5\xF2";
	// Выполняем конвертирование по кодировкам, полученным разбором имён
	ASSERT_EQ(this->_fmk->transcode(cp1251, this->_fmk->codepage("windows-1251"), this->_fmk->codepage("utf-8")), "Привет");
	ASSERT_EQ(this->_fmk->transcode("Привет", this->_fmk->codepage("UTF-8"), this->_fmk->codepage("CP1251")), cp1251);
	ASSERT_EQ(this->_fmk->transcode(cp1251, this->_fmk->codepage("windows-1251"), this->_fmk->codepage("koi8-r")), "\xF0\xD2\xC9\xD7\xC5\xD4");
	// Выполняем проверку отказа при нераспознанном имени кодировки
	ASSERT_TRUE(this->_fmk->transcode(cp1251, this->_fmk->codepage("windows-9999"), this->_fmk->codepage("utf-8")).empty());
}
/**
 * @brief Тест разбора имён кодировок и определения кодировки текста
 *
 */
TEST_F(FmkFixture, CodepageFmkTest){
	// Выполняем проверку разбора имён кодировок
	ASSERT_EQ(this->_fmk->codepage("windows-1251"), awh::fmk_t::codepage_t::CP1251);
	ASSERT_EQ(this->_fmk->codepage("KOI8-R"), awh::fmk_t::codepage_t::KOI8_R);
	ASSERT_EQ(this->_fmk->codepage("utf-8"), awh::fmk_t::codepage_t::UTF8);
	// Выполняем проверку отказа разбора нераспознанного имени
	ASSERT_EQ(this->_fmk->codepage("windows-9999"), awh::fmk_t::codepage_t::NONE);
	// Выполняем проверку извлечения имени кодировки по её обозначению
	ASSERT_EQ(this->_fmk->codepage(awh::fmk_t::codepage_t::CP1251), "windows-1251");
	ASSERT_EQ(this->_fmk->codepage(awh::fmk_t::codepage_t::UTF8), "UTF-8");
	// Выполняем проверку определения кодировки текста
	ASSERT_EQ(this->_fmk->detect("Привет"), awh::fmk_t::codepage_t::UTF8);
	ASSERT_EQ(this->_fmk->detect("\xCF\xF0\xE8\xE2\xE5\xF2"), awh::fmk_t::codepage_t::CP1251);
	ASSERT_EQ(this->_fmk->detect("\xF0\xD2\xC9\xD7\xC5\xD4", awh::fmk_t::codepage_t::KOI8_R), awh::fmk_t::codepage_t::KOI8_R);
}
/**
 * @brief Тест поиска в контейнере по значению записи
 *
 */
TEST_F(FmkFixture, FindInMapCaselessFmkTest){
	// Набор записей, поиск по значению которых выполняется
	const std::unordered_map <uint16_t, std::string> codes = {{200, "OK"}, {404, "Not Found"}};
	// Выполняем проверку сличения строковых значений без учёта регистра
	auto it = this->_fmk->findInMap(std::string{"not found"}, codes);
	ASSERT_NE(it, codes.end());
	ASSERT_EQ(it->first, 404);
	// Выполняем проверку сличения значения, записанного иным регистром
	it = this->_fmk->findInMap(std::string{"ok"}, codes);
	ASSERT_NE(it, codes.end());
	ASSERT_EQ(it->first, 200);
	// Выполняем проверку отсутствия записи в контейнере
	ASSERT_EQ(this->_fmk->findInMap(std::string{"missing"}, codes), codes.end());
	// Выполняем проверку сличения значений, строками не являющихся
	const std::unordered_map <uint16_t, uint32_t> numbers = {{1, 10}, {2, 20}};
	const auto num = this->_fmk->findInMap(static_cast <uint32_t> (20), numbers);
	ASSERT_NE(num, numbers.end());
	ASSERT_EQ(num->first, 2);
}
/**
 * @brief Тест разбора записей размерности данных
 *
 */
TEST_F(FmkFixture, BytesFractionFmkTest){
	// Выполняем проверку разбора записей с целым числом
	ASSERT_EQ(this->_fmk->bytes("1Kb"), 1024.);
	ASSERT_EQ(this->_fmk->bytes("1 Kb"), 1024.);
	ASSERT_EQ(this->_fmk->bytes("10Mb"), 10485760.);
	ASSERT_EQ(this->_fmk->bytes("100 Gb"), 107374182400.);
	ASSERT_EQ(this->_fmk->bytes("1024 bytes"), 1024.);
	// Выполняем проверку разбора записей с дробным числом
	ASSERT_EQ(this->_fmk->bytes("1.5 Mb"), 1572864.);
	ASSERT_EQ(this->_fmk->bytes("1.5Mb"), 1572864.);
	ASSERT_EQ(this->_fmk->bytes("0.5Kb"), 512.);
	ASSERT_EQ(this->_fmk->bytes("2.25 Gb"), 2415919104.);
	/**
	 * Выполняем проверку кругового обхода записи размерности
	 *
	 * @details Запись, выводимая методом, обязана разбираться обратно тем же
	 *          методом до исходного значения.
	 */
	ASSERT_EQ(this->_fmk->bytes(this->_fmk->bytes(1572864.)), 1572864.);
	ASSERT_EQ(this->_fmk->bytes(this->_fmk->bytes(1024.)), 1024.);
	// Выполняем проверку разбора пропускной способности сети с дробным числом
	ASSERT_EQ(this->_fmk->bpsSize("1.5Mbps"), static_cast <size_t> (187500));
	ASSERT_EQ(this->_fmk->bpsSize("100Mbps"), static_cast <size_t> (12500000));
}
/**
 * @brief Тест разбора строковых чисел
 *
 * @details Разбор задан перекрытиями, принимающими представление строки. Прежде
 *          рядом с ними стояли перекрытия, принимающие указатель с длиной, и вызов
 *          вида atoi("ff", 16) выбирал их молча, принимая основание системы счисления
 *          за длину строки. Тест закрепляет выбор перекрытия с основанием.
 *
 */
TEST_F(FmkFixture, AtoiOverloadsFmkTest){
	// Выполняем проверку разбора записей строковым литералом
	ASSERT_EQ(this->_fmk->atoi <uint32_t> ("12345"), static_cast <uint32_t> (12345));
	ASSERT_EQ(this->_fmk->atoi <int32_t> ("-42"), static_cast <int32_t> (-42));
	ASSERT_DOUBLE_EQ(this->_fmk->atoi <double> ("3.14159"), 3.14159);
	// Выполняем проверку разбора записей с указанием системы счисления
	ASSERT_EQ(this->_fmk->atoi <uint32_t> ("ff", 16), static_cast <uint32_t> (255));
	ASSERT_EQ(this->_fmk->atoi <uint32_t> ("11111111", 2), static_cast <uint32_t> (255));
	ASSERT_EQ(this->_fmk->atoi <uint32_t> ("777", 8), static_cast <uint32_t> (511));
	// Выполняем проверку разбора записей строкой
	ASSERT_EQ(this->_fmk->atoi <uint32_t> (std::string{"777"}), static_cast <uint32_t> (777));
	// Выполняем проверку разбора части строки её представлением
	const std::string text = "0123456789";
	ASSERT_EQ(this->_fmk->atoi <uint32_t> (std::string_view{text.data() + 2, 3}), static_cast <uint32_t> (234));
	// Выполняем проверку кругового обхода записи числа
	ASSERT_EQ(this->_fmk->atoi <int32_t> (this->_fmk->itoa <int32_t> (-42, 16), 16), static_cast <int32_t> (-42));
	ASSERT_EQ(this->_fmk->atoi <uint32_t> (this->_fmk->itoa <uint32_t> (255, 16), 16), static_cast <uint32_t> (255));
}

/**
 * @brief Метод проверки записи чисел при чужой местности
 *
 */
TEST_F(FmkFixture, LocaleIndependentNumbersTest){
	// Запоминаем действующую местность записи чисел
	const std::string current(::setlocale(LC_NUMERIC, nullptr));
	// Количество проверенных местностей с иным десятичным знаком
	uint32_t checked = 0;
	/**
	 * Выполняем перебор названий местности с иным знаком десятичной точки
	 *
	 * @note Названия эти у разных систем свои, и ни одно не признаётся всюду.
	 *       Местности «fa_IR» и «ar_SA» взяты особо: десятичным знаком там служит
	 *       «٫» (U+066B), занимающий в UTF-8 два байта, - замена одного лишь первого
	 *       байта оставляла бы от него обрубок
	 */
	for(const char * name : {"de_DE.UTF-8", "de_DE.utf8", "German_Germany.1252", "German_Germany", "fa_IR.UTF-8", "ar_SA.UTF-8"}){
		// Если установить очередную местность не удалось
		if(::setlocale(LC_NUMERIC, name) == nullptr)
			// Выполняем переход к следующей местности
			continue;
		// Если знаком десятичной точки установленной местности точка всё же осталась
		if(::localeconv()->decimal_point[0] == '.')
			// Выполняем переход к следующей местности
			continue;
		// Выполняем проверку записи числа наименьшей точной записью
		ASSERT_EQ(this->_fmk->noexp(1536. / 1024.), "1.5") << name;
		// Выполняем проверку записи числа с подбором точности
		ASSERT_EQ(this->_fmk->noexp(0.1), "0.1") << name;
		// Выполняем проверку записи числа с заданным размером шага
		ASSERT_EQ(this->_fmk->noexp(2986.808299, static_cast <uint8_t> (3)), "2986.808") << name;
		// Выполняем проверку записи числа с разделением разрядов
		ASSERT_EQ(this->_fmk->grouped(1234567.25, 2), "1,234,567.25") << name;
		// Выполняем учёт проверенной местности
		checked++;
	}
	// Выполняем возврат действующей местности записи чисел
	::setlocale(LC_NUMERIC, current.c_str());
	// Если ни одной местности с иным десятичным знаком в системе не нашлось
	if(checked == 0)
		// Выполняем пропуск проверки
		GTEST_SKIP() << "no locale with a foreign decimal point is available";
}

/**
 * @brief Метод проверки записи чисел с разделением разрядов
 *
 */
TEST_F(FmkFixture, GroupedTest){
	// Выполняем проверку записи с заданным количеством знаков после запятой
	ASSERT_EQ(this->_fmk->grouped(10000000000.5, 4), "10,000,000,000.5000");
	// Выполняем проверку записи с иным знаком-разделителем разрядов
	ASSERT_EQ(this->_fmk->grouped(10000000000.5, 4, ' '), "10 000 000 000.5000");
	// Выполняем проверку записи с подбором количества знаков после запятой
	ASSERT_EQ(this->_fmk->grouped(1234567.), "1,234,567");
	// Выполняем проверку записи без дробной части вовсе
	ASSERT_EQ(this->_fmk->grouped(123456789., 0), "123,456,789");
	// Выполняем проверку записи с иным количеством разрядов в группе
	ASSERT_EQ(this->_fmk->grouped(123456789., 0, ',', 4), "1,2345,6789");
	// Выполняем проверку записи отрицательного числа
	ASSERT_EQ(this->_fmk->grouped(-9876.5, 1), "-9,876.5");
	// Выполняем проверку записи числа, разрядов которого на группу не набирается
	ASSERT_EQ(this->_fmk->grouped(123.), "123");
	// Выполняем проверку записи числа ровно в одну группу разрядов
	ASSERT_EQ(this->_fmk->grouped(1000.), "1,000");
	// Выполняем проверку записи числа с нулевой целой частью
	ASSERT_EQ(this->_fmk->grouped(0.5, 2), "0.50");
	// Выполняем проверку отключения разделения нулевым размером группы
	ASSERT_EQ(this->_fmk->grouped(1234567., -1, ',', 0), "1234567");
	// Выполняем проверку записи наибольшего беззнакового целого
	ASSERT_EQ(this->_fmk->grouped <uint64_t> (18446744073709551615ULL), "18,446,744,073,709,551,615");
	// Выполняем проверку записи отрицательного целого
	ASSERT_EQ(this->_fmk->grouped <int32_t> (-1234567), "-1,234,567");
	// Выполняем проверку записи наименьшего знакового целого
	ASSERT_EQ(this->_fmk->grouped <int64_t> (-9223372036854775807LL - 1), "-9,223,372,036,854,775,808");
	// Выполняем проверку записи нуля
	ASSERT_EQ(this->_fmk->grouped <int32_t> (0), "0");
	// Выполняем проверку того, что запись без разделения разрядов не изменилась
	ASSERT_EQ(this->_fmk->noexp(1234567.), "1234567");
}

/**
 * @brief Метод проверки установки несуществующей местности
 *
 */
TEST_F(FmkFixture, SetLocaleFallbackTest){
	// Запоминаем действующую местность приложения
	const std::string current(::setlocale(LC_ALL, nullptr));
	// Выполняем установку заведомо несуществующей местности
	this->_fmk->setLocale("xx_YY.UTF-8");
	/**
	 * Если система отказ от установки местности отдаёт
	 *
	 * @note Библиотека UCRT у MS Windows принимает любое название и отвечает успехом,
	 *       оттого откат к общепринятой местности там не выполняется и проверять его
	 *       нечем
	 */
	if(::setlocale(LC_ALL, nullptr) != nullptr){
		// Получаем действующую после отказа местность приложения
		const std::string established(::setlocale(LC_ALL, nullptr));
		// Если система отказ отдала, местностью обязана стать общепринятая
		if(established.compare("xx_YY.UTF-8") != 0)
			// Выполняем проверку отката к общепринятой местности
			ASSERT_EQ(established, "C");
	}
	// Выполняем возврат действующей местности приложения
	::setlocale(LC_ALL, current.c_str());
}
