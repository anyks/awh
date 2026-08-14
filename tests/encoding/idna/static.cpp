/**
 * @file static.cpp
 * @date 2026-08-03
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
 * @brief Статические тесты модуля приведения доменных имён — приведение имени к записи
 *        из символов набора ASCII и обратно, кодировщик Punycode, режимы приведения
 *        и правила окружения символов метки
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл
 */
#include "idna.hpp"

/**
 * Используем пространства имён
 */
using namespace awh;
using namespace awh::idna;

/**
 * @par Намеренные решения
 *
 * Имя awh::idna::error_t уточняется пространством имён, хотя оно и внесено объявлением выше
 *
 * Библиотека glibc объявляет свой awh::idna::error_t в глобальном пространстве имён, и голое
 * обращение становится двусмысленным: сборка отвечает отказом «reference to awh::idna::error_t
 * is ambiguous». Скрыть его нельзя - объявление это выходит при _GNU_SOURCE, а его
 * компилятор g++ определяет для C++ сам, помимо нашей воли. Проверено опытом: без
 * _GNU_SOURCE имя не видно вовсе, с ним - видно
 *
 * То же встретит и потребитель библиотеки, написавший using namespace для этого
 * пространства на Linux
 *
 */

/**
 * @brief Тест приведения доменных имён к записи из символов набора ASCII
 *
 */
TEST_F(IdnaFixture, ToAsciiDomains){
	// Выполняем проверку приведения имён разных письменностей
	ASSERT_EQ(toAscii("пример.рф"), "xn--e1afmkfd.xn--p1ai");
	ASSERT_EQ(toAscii("испытание.испытание"), "xn--80akhbyknj4f.xn--80akhbyknj4f");
	ASSERT_EQ(toAscii("münchen.de"), "xn--mnchen-3ya.de");
	ASSERT_EQ(toAscii("日本語.jp"), "xn--wgv71a119e.jp");
	// Выполняем проверку приведения имени, записанного набором ASCII
	ASSERT_EQ(toAscii("example.com"), "example.com");
	// Выполняем проверку приведения имени к нижнему регистру
	ASSERT_EQ(toAscii("EXAMPLE.COM"), "example.com");
	ASSERT_EQ(toAscii("Пример.РФ"), "xn--e1afmkfd.xn--p1ai");
	// Выполняем проверку приведения имени, уже записанного кодировкой Punycode
	ASSERT_EQ(toAscii("xn--e1afmkfd.xn--p1ai"), "xn--e1afmkfd.xn--p1ai");
	// Выполняем проверку приведения имени от корня зоны
	ASSERT_EQ(toAscii("example.com."), "example.com.");
}
/**
 * @brief Тест приведения доменных имён к записи Юникода
 *
 */
TEST_F(IdnaFixture, ToUnicodeDomains){
	// Выполняем проверку обратного приведения имён разных письменностей
	ASSERT_EQ(toUnicode("xn--e1afmkfd.xn--p1ai"), "пример.рф");
	ASSERT_EQ(toUnicode("xn--mnchen-3ya.de"), "münchen.de");
	ASSERT_EQ(toUnicode("xn--wgv71a119e.jp"), "日本語.jp");
	// Выполняем проверку обратного приведения имени, записанного набором ASCII
	ASSERT_EQ(toUnicode("example.com"), "example.com");
	// Выполняем проверку сличения приставки записи без учёта регистра
	ASSERT_EQ(toUnicode("XN--E1AFMKFD.XN--P1AI"), "пример.рф");
}
/**
 * @brief Тест кругового обхода приведения доменных имён
 *
 */
TEST_F(IdnaFixture, Roundtrip){
	// Набор доменных имён разных письменностей
	const vector <string> domains = {
		"пример.рф", "münchen.de", "日本語.jp", "한국어.kr", "ελληνικά.gr",
		"example.com", "тест-домен.москва", "xn--e1afmkfd.xn--p1ai"
	};
	/**
	 * Выполняем обход набора доменных имён
	 */
	for(auto & domain : domains) {
		// Выполняем приведение доменного имени к записи из символов набора ASCII
		const string encoded = toAscii(domain);
		// Выполняем проверку выполнения приведения доменного имени
		ASSERT_FALSE(encoded.empty()) << "доменное имя " << domain;
		// Выполняем проверку повторного приведения записи к тому же виду
		ASSERT_EQ(toAscii(encoded), encoded) << "доменное имя " << domain;
		// Выполняем обратное приведение доменного имени к записи Юникода
		ASSERT_EQ(toAscii(toUnicode(encoded)), encoded) << "доменное имя " << domain;
	}
}
/**
 * @brief Тест отклонения недопустимых доменных имён
 *
 */
TEST_F(IdnaFixture, Rejection){
	// Получившаяся запись доменного имени
	string result = "";
	// Код ошибки приведения доменного имени
	awh::idna::error_t error = awh::idna::error_t::NONE;
	// Выполняем проверку отклонения имени, записанного неправильно
	ASSERT_FALSE(toAscii("\xD0", result, error));
	ASSERT_EQ(error, awh::idna::error_t::ENCODING);
	// Выполняем проверку отклонения метки, начинающейся знаком переноса
	ASSERT_FALSE(toAscii("-example.com", result, error));
	ASSERT_EQ(error, awh::idna::error_t::HYPHEN);
	// Выполняем проверку отклонения метки, завершающейся знаком переноса
	ASSERT_FALSE(toAscii("example-.com", result, error));
	ASSERT_EQ(error, awh::idna::error_t::HYPHEN);
	// Выполняем проверку отклонения записи, разобрать которую не вышло
	ASSERT_FALSE(toAscii("xn--a-ecp.ru", result, error));
	// Выполняем проверку отклонения пустой метки доменного имени
	ASSERT_FALSE(toAscii("example..com", result, error));
	ASSERT_EQ(error, awh::idna::error_t::LABEL_LENGTH);
	// Выполняем проверку отклонения пустого доменного имени
	ASSERT_FALSE(toAscii("", result, error));
}
/**
 * @brief Тест проверки длины доменного имени и его меток
 *
 */
TEST_F(IdnaFixture, Length){
	// Получившаяся запись доменного имени
	string result = "";
	// Код ошибки приведения доменного имени
	awh::idna::error_t error = awh::idna::error_t::NONE;
	// Выполняем проверку допустимости метки наибольшей длины
	ASSERT_TRUE(toAscii(string(MAX_LABEL, 'a') + ".com", result, error));
	// Выполняем проверку отклонения метки, длину превышающей
	ASSERT_FALSE(toAscii(string(MAX_LABEL + 1, 'a') + ".com", result, error));
	ASSERT_EQ(error, awh::idna::error_t::LABEL_LENGTH);
	// Выполняем проверку отклонения имени, длину превышающей
	string domain = "";
	// Выполняем сборку доменного имени, длину превышающего
	while(domain.size() <= MAX_DOMAIN)
		// Выполняем добавление очередной метки доменного имени
		domain.append("example.");
	// Выполняем проверку отклонения доменного имени
	ASSERT_FALSE(toAscii(domain, result, error));
	ASSERT_EQ(error, awh::idna::error_t::DOMAIN_LENGTH);
	/**
	 * Выполняем проверку приведения имени без проверки длины
	 *
	 * @details Проверка длины задана отдельным режимом приведения и снимается
	 *          потребителем, которому приведение требуется вне протокола DNS.
	 */
	ASSERT_TRUE(toAscii(string(MAX_LABEL + 1, 'a') + ".com", result,
		error, (DEFAULT_MODE & ~static_cast <uint16_t> (option_t::LENGTH))));
}
/**
 * @brief Тест переходного режима преобразования символов
 *
 */
TEST_F(IdnaFixture, Transitional){
	// Получаем набор режимов приведения с переходным преобразованием символов
	const uint16_t mode = (DEFAULT_MODE | static_cast <uint16_t> (option_t::TRANSITIONAL));
	/**
	 * Выполняем проверку преобразования немецкой буквы эсцет
	 *
	 * @details Переходный режим приводит её к паре латинских букв, тогда как
	 *          без него она записывается кодировкой Punycode.
	 */
	ASSERT_EQ(toAscii("faß.de", mode), "fass.de");
	ASSERT_EQ(toAscii("faß.de"), "xn--fa-hia.de");
	/**
	 * Выполняем проверку преобразования греческой конечной сигмы
	 */
	ASSERT_EQ(toAscii("βόλος.com", mode), "xn--nxasmq6b.com");
	ASSERT_EQ(toAscii("βόλος.com"), "xn--nxasmm1c.com");
}
/**
 * @brief Тест правил записи имён узлов
 *
 */
TEST_F(IdnaFixture, Std3Rules){
	// Получаем набор режимов приведения с правилами записи имён узлов
	const uint16_t mode = (DEFAULT_MODE | static_cast <uint16_t> (option_t::STD3));
	// Выполняем проверку отклонения знака подчёркивания правилами записи имён узлов
	ASSERT_TRUE(toAscii("a_b.com").size() > 0);
	ASSERT_TRUE(toAscii("a_b.com", mode).empty());
}
/**
 * @brief Тест правил сочетания соединителей нулевой ширины
 *
 */
TEST_F(IdnaFixture, Joiners){
	// Выполняем проверку допустимости соединителя вслед за знаком сочетания согласных
	ASSERT_TRUE(joiners({0x0915, 0x094D, 0x200D, 0x0915}));
	// Выполняем проверку отклонения соединителя без знака сочетания согласных
	ASSERT_FALSE(joiners({0x0915, 0x200D, 0x0915}));
	// Выполняем проверку отклонения соединителя, размещённого первым символом
	ASSERT_FALSE(joiners({0x200D, 0x0915}));
	ASSERT_FALSE(joiners({0x200C, 0x0915}));
	// Выполняем проверку допустимости разъединителя между соединяющимися символами
	ASSERT_TRUE(joiners({0x0628, 0x200C, 0x0628}));
	// Выполняем проверку отклонения разъединителя вне соединяющихся символов
	ASSERT_FALSE(joiners({0x0061, 0x200C, 0x0062}));
	// Выполняем проверку метки без соединителей
	ASSERT_TRUE(joiners({0x0061, 0x0062, 0x0063}));
}
/**
 * @brief Тест правила двунаправленного письма
 *
 */
TEST_F(IdnaFixture, Bidirectional){
	// Выполняем проверку метки, записанной письмом слева направо
	ASSERT_TRUE(bidirectional({0x0061, 0x0062, 0x0063}));
	// Выполняем проверку метки, записанной письмом справа налево
	ASSERT_TRUE(bidirectional({0x05D0, 0x05D1}));
	// Выполняем проверку отклонения метки, завершающейся недопустимым символом
	ASSERT_FALSE(bidirectional({0x05D0, 0x002D}));
	// Выполняем проверку отклонения совместного размещения цифр разного письма
	ASSERT_FALSE(bidirectional({0x05D0, 0x0031, 0x0661, 0x05D1}));
	// Выполняем проверку определения принадлежности имени двунаправленному письму
	ASSERT_TRUE(directional({{0x05D0}, {0x0061}}));
	ASSERT_FALSE(directional({{0x0061}, {0x0062}}));
}
/**
 * @brief Тест кодировщика Punycode
 *
 */
TEST_F(IdnaFixture, Punycode){
	// Получившаяся запись кодировкой Punycode
	string encoded = "";
	// Набор кодовых значений разобранной записи
	vector <uint32_t> decoded;
	// Выполняем проверку представления набора кодовых значений
	ASSERT_TRUE(punycode::encode({0x043F, 0x0440, 0x0438, 0x043C, 0x0435, 0x0440}, encoded));
	ASSERT_EQ(encoded, "e1afmkfd");
	// Выполняем проверку обратного разбора получившейся записи
	ASSERT_TRUE(punycode::decode(encoded, decoded));
	ASSERT_EQ(decoded, (vector <uint32_t> {0x043F, 0x0440, 0x0438, 0x043C, 0x0435, 0x0440}));
	// Выполняем проверку представления набора символов ASCII без изменений
	ASSERT_TRUE(punycode::encode({0x0061, 0x0062, 0x0063}, encoded));
	ASSERT_EQ(encoded, "abc-");
	// Выполняем проверку обратного разбора записи из символов набора ASCII
	ASSERT_TRUE(punycode::decode("abc-", decoded));
	ASSERT_EQ(decoded, (vector <uint32_t> {0x0061, 0x0062, 0x0063}));
	// Выполняем проверку отклонения записи с недопустимым символом
	ASSERT_FALSE(punycode::decode("a\xFF", decoded));
}
/**
 * @brief Тест извлечения состояния символа в таблице преобразований
 *
 */
TEST_F(IdnaFixture, Status){
	// Набор кодовых значений преобразования символа
	vector <uint32_t> mapped;
	// Выполняем проверку состояния допустимого символа
	ASSERT_EQ(status(0x0061, mapped), status_t::VALID);
	// Выполняем проверку состояния преобразуемого символа
	ASSERT_EQ(status(0x0041, mapped), status_t::MAPPED);
	ASSERT_EQ(mapped, (vector <uint32_t> {0x0061}));
	// Выполняем проверку состояния опускаемого символа
	ASSERT_EQ(status(0x00AD, mapped), status_t::IGNORED);
	// Выполняем проверку состояния символа переходного преобразования
	ASSERT_EQ(status(0x00DF, mapped), status_t::DEVIATION);
	// Выполняем проверку состояния символа, допустимого вне правил записи имён узлов
	ASSERT_EQ(status(0x005F, mapped), status_t::DISALLOWED_STD3_VALID);
}
