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
#include <sys/fmk.hpp>


/**
 * @brief Проверка состояния модуля ядра, единственного на процесс
 *
 * @details Прежде эти проверки испытывали заведение и сброс ОБЪЕКТА фреймворка.
 *          Объекта не стало: средства модуля глобальны на приложение, и закреплять
 *          нужно, что заведение выполняется само и повторным обращением не сбрасывается
 *
 */
TEST_F(FmkFixture, SingleStateFmkTest){
	// Выполняем заведение модуля на весь процесс
	awh::fmk::initialize();
	// Утверждаем, что средства модуля работают без построения объекта
	ASSERT_EQ(awh::fmk::format("%s-%u", "число", 42), std::string{"число-42"})
	 << "модуль ядра не работает без построения объекта";
	// Выполняем добавление опознаваемой доменной зоны
	awh::fmk::domainZone("единственность");
	// Утверждаем, что состояние модуля заведено и настройку приняло
	ASSERT_TRUE(awh::fmk::domainZones().count("единственность") > 0)
	 << "состояние модуля ядра настройку не приняло";
	// Выполняем повторное заведение модуля
	awh::fmk::initialize();
	// Утверждаем, что повторное заведение состояния не сбросило
	ASSERT_TRUE(awh::fmk::domainZones().count("единственность") > 0)
	 << "повторное заведение модуля сбросило ранее установленную настройку";
}


/**
 * @brief Метод повторного создания объекта фреймворка
 *
 */
TEST_F(FmkFixture, ReCreateFmkTest){
}

/**
 * @brief Метод тестирования установки бита в указанную позицию
 *
 */
TEST_F(FmkFixture, CaseFmkTest){
	// Тестируем установку бита в указанную позицию
	ASSERT_EQ(awh::fmk::setBit <uint64_t> (2, 4), 8);
	// Тестируем проверку установленного бита
	ASSERT_TRUE(awh::fmk::isBit <uint64_t> (3, 8));
	// Тестируем сброс бита в указанной позиции
	ASSERT_EQ(awh::fmk::resetBit <uint64_t> (3, 8), 0);
	// Тестируем инверсию бита в указанной позиции
	ASSERT_EQ(awh::fmk::flipBit <uint64_t> (3, 8), 0);
}

/**
 * @brief Метод тестирования форматирования строк
 *
 */
TEST_F(FmkFixture, FormatFmkTest){
	// Тестируем форматирование строк
	ASSERT_EQ("Hello World!!!", awh::fmk::format("%s %s!!!", "Hello", "World"));
	// Тестируем форматирование строк с числовыми параметрами
	ASSERT_EQ("Вашм присвоен идентификатор ID=984 и ID=586", awh::fmk::format("%s ID=%u и ID=%u", "Вашм присвоен идентификатор", 984, 586));
}

/**
 * @brief Метод тестирования установки доменных зон
 *
 */
TEST_F(FmkFixture, DomainZoneFmkTest){
	// Тестируем установку доменных зон
	awh::fmk::domainZones({"anyks", "google", "yandex"});
	// Добавляем новую доменную зону
	awh::fmk::domainZone("goga");
	// Добавляем ещё одну доменную зону
	ASSERT_EQ(4, awh::fmk::domainZones().size());
	/**
	 * Проходим по всем доменным зонам
	 */
	for(auto & zone : awh::fmk::domainZones())
		// Проверяем что доменная зона установлена корректно
		ASSERT_TRUE((zone.compare("goga") == 0) || (zone.compare("anyks") == 0) || (zone.compare("google") == 0) || (zone.compare("yandex") == 0));
}

/**
 * @brief Метод тестирования иконок
 *
 */
TEST_F(FmkFixture, IconFmkTest){
	// Тестируем иконки
	ASSERT_TRUE(!awh::fmk::icon().empty());
}

/**
 * @brief Метод тестирования идентификаторов
 *
 */
TEST_F(FmkFixture, IdentifierFmkTest){
	// Тестируем идентификаторы
	ASSERT_EQ(awh::fmk::identifier(), 1);
	ASSERT_EQ(awh::fmk::identifier(), 2);
	ASSERT_EQ(awh::fmk::identifier(), 3);
	ASSERT_EQ(awh::fmk::identifier(), 4);
	ASSERT_EQ(awh::fmk::identifier(), 5);
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
	ASSERT_EQ(awh::fmk::transcode(cp1251, awh::fmk::codepage_t::CP1251, awh::fmk::codepage_t::KOI8_R), koi8r);
	ASSERT_EQ(awh::fmk::transcode(koi8r, awh::fmk::codepage_t::KOI8_R, awh::fmk::codepage_t::CP1251), cp1251);
	// Выполняем проверку конвертирования в UTF-8 и обратно
	ASSERT_EQ(awh::fmk::transcode(cp1251, awh::fmk::codepage_t::CP1251, awh::fmk::codepage_t::UTF8), "Привет");
	ASSERT_EQ(awh::fmk::transcode("Привет", awh::fmk::codepage_t::UTF8, awh::fmk::codepage_t::CP1251), cp1251);
	// Выполняем проверку конвертирования текста западных языков
	ASSERT_EQ(awh::fmk::transcode("München", awh::fmk::codepage_t::UTF8, awh::fmk::codepage_t::ISO8859_1), "M\xFCnchen");
	ASSERT_EQ(awh::fmk::transcode("M\xFCnchen", awh::fmk::codepage_t::ISO8859_1, awh::fmk::codepage_t::UTF8), "München");
	// Выполняем проверку отказа конвертирования непредставимого символа
	ASSERT_TRUE(awh::fmk::transcode("Привет", awh::fmk::codepage_t::UTF8, awh::fmk::codepage_t::ISO8859_1).empty());
	// Выполняем проверку замены непредставимых символов
	ASSERT_EQ(awh::fmk::transcode("aПb", awh::fmk::codepage_t::UTF8,
		awh::fmk::codepage_t::ISO8859_1, awh::fmk::replace_t::REPLACE), "a?b");
}
/**
 * @brief Тест конвертирования строки по кодировкам, заданным именем
 *
 */
TEST_F(FmkFixture, TranscodeNameFmkTest){
	// Записываем слово «Привет» в кодировке CP1251
	const std::string cp1251 = "\xCF\xF0\xE8\xE2\xE5\xF2";
	// Выполняем конвертирование по кодировкам, полученным разбором имён
	ASSERT_EQ(awh::fmk::transcode(cp1251, awh::fmk::codepage("windows-1251"), awh::fmk::codepage("utf-8")), "Привет");
	ASSERT_EQ(awh::fmk::transcode("Привет", awh::fmk::codepage("UTF-8"), awh::fmk::codepage("CP1251")), cp1251);
	ASSERT_EQ(awh::fmk::transcode(cp1251, awh::fmk::codepage("windows-1251"), awh::fmk::codepage("koi8-r")), "\xF0\xD2\xC9\xD7\xC5\xD4");
	// Выполняем проверку отказа при нераспознанном имени кодировки
	ASSERT_TRUE(awh::fmk::transcode(cp1251, awh::fmk::codepage("windows-9999"), awh::fmk::codepage("utf-8")).empty());
}
/**
 * @brief Тест разбора имён кодировок и определения кодировки текста
 *
 */
TEST_F(FmkFixture, CodepageFmkTest){
	// Выполняем проверку разбора имён кодировок
	ASSERT_EQ(awh::fmk::codepage("windows-1251"), awh::fmk::codepage_t::CP1251);
	ASSERT_EQ(awh::fmk::codepage("KOI8-R"), awh::fmk::codepage_t::KOI8_R);
	ASSERT_EQ(awh::fmk::codepage("utf-8"), awh::fmk::codepage_t::UTF8);
	// Выполняем проверку отказа разбора нераспознанного имени
	ASSERT_EQ(awh::fmk::codepage("windows-9999"), awh::fmk::codepage_t::NONE);
	// Выполняем проверку извлечения имени кодировки по её обозначению
	ASSERT_EQ(awh::fmk::codepage(awh::fmk::codepage_t::CP1251), "windows-1251");
	ASSERT_EQ(awh::fmk::codepage(awh::fmk::codepage_t::UTF8), "UTF-8");
	// Выполняем проверку определения кодировки текста
	ASSERT_EQ(awh::fmk::detect("Привет"), awh::fmk::codepage_t::UTF8);
	ASSERT_EQ(awh::fmk::detect("\xCF\xF0\xE8\xE2\xE5\xF2"), awh::fmk::codepage_t::CP1251);
	ASSERT_EQ(awh::fmk::detect("\xF0\xD2\xC9\xD7\xC5\xD4", awh::fmk::codepage_t::KOI8_R), awh::fmk::codepage_t::KOI8_R);
}
/**
 * @brief Тест разбора записей размерности данных
 *
 */
TEST_F(FmkFixture, BytesFractionFmkTest){
	// Выполняем проверку разбора записей с целым числом
	ASSERT_EQ(awh::fmk::bytes("1Kb"), 1024.);
	ASSERT_EQ(awh::fmk::bytes("1 Kb"), 1024.);
	ASSERT_EQ(awh::fmk::bytes("10Mb"), 10485760.);
	ASSERT_EQ(awh::fmk::bytes("100 Gb"), 107374182400.);
	ASSERT_EQ(awh::fmk::bytes("1024 bytes"), 1024.);
	// Выполняем проверку разбора записей с дробным числом
	ASSERT_EQ(awh::fmk::bytes("1.5 Mb"), 1572864.);
	ASSERT_EQ(awh::fmk::bytes("1.5Mb"), 1572864.);
	ASSERT_EQ(awh::fmk::bytes("0.5Kb"), 512.);
	ASSERT_EQ(awh::fmk::bytes("2.25 Gb"), 2415919104.);
	/**
	 * Выполняем проверку кругового обхода записи размерности
	 *
	 * @details Запись, выводимая методом, обязана разбираться обратно тем же
	 *          методом до исходного значения.
	 */
	ASSERT_EQ(awh::fmk::bytes(awh::fmk::bytes(1572864.)), 1572864.);
	ASSERT_EQ(awh::fmk::bytes(awh::fmk::bytes(1024.)), 1024.);
	// Выполняем проверку разбора пропускной способности сети с дробным числом
	ASSERT_EQ(awh::fmk::bpsSize("1.5Mbps"), static_cast <size_t> (187500));
	ASSERT_EQ(awh::fmk::bpsSize("100Mbps"), static_cast <size_t> (12500000));
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
	ASSERT_EQ(awh::fmk::atoi <uint32_t> ("12345"), static_cast <uint32_t> (12345));
	ASSERT_EQ(awh::fmk::atoi <int32_t> ("-42"), static_cast <int32_t> (-42));
	ASSERT_DOUBLE_EQ(awh::fmk::atoi <double> ("3.14159"), 3.14159);
	// Выполняем проверку разбора записей с указанием системы счисления
	ASSERT_EQ(awh::fmk::atoi <uint32_t> ("ff", 16), static_cast <uint32_t> (255));
	ASSERT_EQ(awh::fmk::atoi <uint32_t> ("11111111", 2), static_cast <uint32_t> (255));
	ASSERT_EQ(awh::fmk::atoi <uint32_t> ("777", 8), static_cast <uint32_t> (511));
	// Выполняем проверку разбора записей строкой
	ASSERT_EQ(awh::fmk::atoi <uint32_t> (std::string{"777"}), static_cast <uint32_t> (777));
	// Выполняем проверку разбора части строки её представлением
	const std::string text = "0123456789";
	ASSERT_EQ(awh::fmk::atoi <uint32_t> (std::string_view{text.data() + 2, 3}), static_cast <uint32_t> (234));
	// Выполняем проверку кругового обхода записи числа
	ASSERT_EQ(awh::fmk::atoi <int32_t> (awh::fmk::itoa <int32_t> (-42, 16), 16), static_cast <int32_t> (-42));
	ASSERT_EQ(awh::fmk::atoi <uint32_t> (awh::fmk::itoa <uint32_t> (255, 16), 16), static_cast <uint32_t> (255));
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
		ASSERT_EQ(awh::fmk::noexp(1536. / 1024.), "1.5") << name;
		// Выполняем проверку записи числа с подбором точности
		ASSERT_EQ(awh::fmk::noexp(0.1), "0.1") << name;
		// Выполняем проверку записи числа с заданным размером шага
		ASSERT_EQ(awh::fmk::noexp(2986.808299, static_cast <uint8_t> (3)), "2986.808") << name;
		// Выполняем проверку записи числа с разделением разрядов
		ASSERT_EQ(awh::fmk::grouped(1234567.25, 2), "1,234,567.25") << name;
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
	ASSERT_EQ(awh::fmk::grouped(10000000000.5, 4), "10,000,000,000.5000");
	// Выполняем проверку записи с иным знаком-разделителем разрядов
	ASSERT_EQ(awh::fmk::grouped(10000000000.5, 4, ' '), "10 000 000 000.5000");
	// Выполняем проверку записи с подбором количества знаков после запятой
	ASSERT_EQ(awh::fmk::grouped(1234567.), "1,234,567");
	// Выполняем проверку записи без дробной части вовсе
	ASSERT_EQ(awh::fmk::grouped(123456789., 0), "123,456,789");
	// Выполняем проверку записи с иным количеством разрядов в группе
	ASSERT_EQ(awh::fmk::grouped(123456789., 0, ',', 4), "1,2345,6789");
	// Выполняем проверку записи отрицательного числа
	ASSERT_EQ(awh::fmk::grouped(-9876.5, 1), "-9,876.5");
	// Выполняем проверку записи числа, разрядов которого на группу не набирается
	ASSERT_EQ(awh::fmk::grouped(123.), "123");
	// Выполняем проверку записи числа ровно в одну группу разрядов
	ASSERT_EQ(awh::fmk::grouped(1000.), "1,000");
	// Выполняем проверку записи числа с нулевой целой частью
	ASSERT_EQ(awh::fmk::grouped(0.5, 2), "0.50");
	// Выполняем проверку отключения разделения нулевым размером группы
	ASSERT_EQ(awh::fmk::grouped(1234567., -1, ',', 0), "1234567");
	// Выполняем проверку записи наибольшего беззнакового целого
	ASSERT_EQ(awh::fmk::grouped <uint64_t> (18446744073709551615ULL), "18,446,744,073,709,551,615");
	// Выполняем проверку записи отрицательного целого
	ASSERT_EQ(awh::fmk::grouped <int32_t> (-1234567), "-1,234,567");
	// Выполняем проверку записи наименьшего знакового целого
	ASSERT_EQ(awh::fmk::grouped <int64_t> (-9223372036854775807LL - 1), "-9,223,372,036,854,775,808");
	// Выполняем проверку записи нуля
	ASSERT_EQ(awh::fmk::grouped <int32_t> (0), "0");
	// Выполняем проверку того, что запись без разделения разрядов не изменилась
	ASSERT_EQ(awh::fmk::noexp(1234567.), "1234567");
}

/**
 * @brief Метод проверки установки несуществующей местности
 *
 */
TEST_F(FmkFixture, SetLocaleFallbackTest){
	// Запоминаем действующую местность приложения
	const std::string current(::setlocale(LC_ALL, nullptr));
	// Выполняем установку заведомо несуществующей местности
	awh::fmk::setLocale("xx_YY.UTF-8");
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
/**
 * @brief Проверка построения узкой строки, не умещающейся в исходный буфер
 *
 * @note Буфер под построенную строку заводится размером 1024 байта и растёт кругом,
 *       пока строка не уместится. Круг держался на знаке ответа vsnprintf, а длина
 *       принималась в size_t: ответ об отказе -1 обращался в SIZE_MAX, ветка роста
 *       удвоением делалась мёртвым кодом, а размер буфера вычислялся НУЛЁМ. Узкого
 *       близнеца это не валило: vsnprintf отвечает потребной длиной, и рост шёл
 *       живой веткой, - но проверка ставится и на него, потому что подпорка эта
 *       негласная, и смена её у любого из близнецов обязана всплывать здесь
 *
 */
TEST_F(FmkFixture, FormattedNarrowGrowsBeyondInitialBufferTest){
	// Длина, заведомо превышающая исходный буфер в 1024 байта
	constexpr size_t LENGTH = 5000;
	// Создаём заполнитель нужной длины
	const std::string filler(LENGTH, 'x');
	// Выполняем построение форматированной строки
	const std::string result = awh::fmk::format("[%s]", filler.c_str());
	// Выполняем проверку длины построенной строки
	ASSERT_EQ(result.size(), LENGTH + 2);
	// Выполняем проверку содержимого построенной строки
	ASSERT_EQ(result.front(), '[');
	// Выполняем проверку содержимого построенной строки
	ASSERT_EQ(result.back(), ']');
}
/**
 * @brief Проверка построения широкой строки, не умещающейся в исходный буфер
 *
 * @warning Разделение узкого и широкого близнецов здесь намеренное: у них РАЗНАЯ
 *          подверженность одному и тому же изъяну, и объединённая проверка о том
 *          молчала бы. Узкий выбирается сам, потому что vsnprintf отвечает потребной
 *          длиной, и рост буфера идёт живой веткой. А vswprintf при нехватке буфера
 *          отвечает именно -1, и единственным путём роста там была ветка удвоения -
 *          та самая, которую беззнаковый тип длины делал мёртвым кодом. Оттого
 *          построение всякой широкой строки длиннее буфера уходило в круг НАВСЕГДА,
 *          на любой системе. Проверка эта на неисправленном коде не падает, а виснет:
 *          снятие её по сроку и есть здесь отказ, что подтверждено опытом - возвратом
 *          беззнакового типа она снималась по сроку, с исправленным проходит за 1 мс
 *
 */
TEST_F(FmkFixture, FormattedWideGrowsBeyondInitialBufferTest){
	// Длина, заведомо превышающая исходный буфер в 1024 байта
	constexpr size_t LENGTH = 5000;
	// Создаём заполнитель нужной длины
	const std::wstring filler(LENGTH, L'x');
	// Выполняем построение форматированной строки
	const std::wstring result = awh::fmk::format(L"[%ls]", filler.c_str());
	// Выполняем проверку длины построенной строки
	ASSERT_EQ(result.size(), LENGTH + 2);
	// Выполняем проверку содержимого построенной строки
	ASSERT_EQ(result.front(), L'[');
	// Выполняем проверку содержимого построенной строки
	ASSERT_EQ(result.back(), L']');
}
