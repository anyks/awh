/**
 * @file: parameterized.cpp
 * @date: 2025-12-07
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Параметризованные тесты ядра фреймворка — прогон подготовленных наборов входных данных через методы модуля с
 *        проверкой работы со строками и кодировками, смены регистра, форматирования и конвертации типов
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include "fmk.hpp"

/**
 * @brief Структура параметров тестирования метода поиска в отображении
 *
 */
struct FmkFindInMapTestParameter {
	// Ожидаемый ключ найденного элемента
	uint32_t key = 0;
	// Искомое значение
	uint32_t val = 0;
	// Отображение для поиска
	std::map <uint32_t, uint32_t> map = {{1,15},{22,45},{32,88},{84,95}};
};

/**
 * @brief Класс параметризованного теста для метода поиска в отображении
 *
 */
class FmkFindInMapParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkFindInMapTestParameter> {
	public:
		FmkFindInMapTestParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования метода поиска в отображении
 *
 */
TEST_P(FmkFindInMapParameterizedFixture, FmkFindInMapTest){
	// Выполняем поиск значения в отображении
	auto i = this->_fmk->findInMap(this->_parameter.val, this->_parameter.map);
	// Проверяем что значение найдено
	ASSERT_TRUE(i != this->_parameter.map.end());
	// Проверяем что ключ и значение совпадают с ожидаемыми
	ASSERT_EQ(this->_parameter.key, i->first);
}

/**
 * @brief Инициализация параметров тестирования метода поиска в отображении
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, FmkFindInMapParameterizedFixture,
	::testing::Values(
		FmkFindInMapTestParameter({1,15}),
		FmkFindInMapTestParameter({22,45}),
		FmkFindInMapTestParameter({32,88}),
		FmkFindInMapTestParameter({84,95})
	)
);

/**
 * @brief Структура параметров тестирования метода проверки символов и строк
 *
 */
struct FmkIsTestParameter {
	// Символ для проверки в однобайтовой кодировке
	char letter1 = 0;
	// Символ для проверки в многобайтовой кодировке
	wchar_t letter2 = 0;
	// Строка для проверки в однобайтовой кодировке
	std::string text1 = "";
	// Строка для проверки в многобайтовой кодировке
	std::wstring text2 = L"";
	// Флаг проверки
	awh::fmk_t::check_t flag = awh::fmk_t::check_t::NONE;
};

/**
 * @brief Класс параметризованного теста для метода проверки символов и строк
 *
 */
class FmkIsParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkIsTestParameter> {
	public:
		FmkIsTestParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования метода проверки символов и строк
 *
 */
TEST_P(FmkIsParameterizedFixture, FmkIsLetter1Test){
	/**
	 * Выполняем проверку символа в зависимости от флага
	 */
	switch(static_cast <uint8_t> (this->_parameter.flag)){
		// Особый случай для URL-адресов
		case static_cast <uint8_t> (awh::fmk_t::check_t::URL):
			ASSERT_TRUE(true);
		break;
		// Особый случай для чисел с плавающей точкой и псевдо-чисел
		case static_cast <uint8_t> (awh::fmk_t::check_t::DECIMAL):
		// Особый случай для чисел с плавающей точкой и псевдо-чисел
		case static_cast <uint8_t> (awh::fmk_t::check_t::PSEUDO_NUMBER):
			ASSERT_TRUE(this->_fmk->is(this->_parameter.letter1, awh::fmk_t::check_t::NUMBER));
		break;
		// Особый случай для проверки наличия латинских символов в строке
		case static_cast <uint8_t> (awh::fmk_t::check_t::PRESENCE_LATIAN):
			ASSERT_TRUE(this->_fmk->is(this->_parameter.letter1, awh::fmk_t::check_t::LATIAN));
		break;
		// Общий случай для всех остальных флагов
		default:
			ASSERT_TRUE(this->_fmk->is(this->_parameter.letter1, this->_parameter.flag));
	}
}

/**
 * @brief Метод тестирования метода проверки символов и строк
 *
 */
TEST_P(FmkIsParameterizedFixture, FmkIsLetter2Test){
	/**
	 * Выполняем проверку символа в зависимости от флага
	 */
	switch(static_cast <uint8_t> (this->_parameter.flag)){
		// Особый случай для URL-адресов
		case static_cast <uint8_t> (awh::fmk_t::check_t::URL):
			ASSERT_TRUE(true);
		break;
		// Особый случай для чисел с плавающей точкой и псевдо-чисел
		case static_cast <uint8_t> (awh::fmk_t::check_t::DECIMAL):
		case static_cast <uint8_t> (awh::fmk_t::check_t::PSEUDO_NUMBER):
			ASSERT_TRUE(this->_fmk->is(this->_parameter.letter2, awh::fmk_t::check_t::NUMBER));
		break;
		// Особый случай для проверки наличия латинских символов в строке
		case static_cast <uint8_t> (awh::fmk_t::check_t::PRESENCE_LATIAN):
			ASSERT_TRUE(this->_fmk->is(this->_parameter.letter2, awh::fmk_t::check_t::LATIAN));
		break;
		// Общий случай для всех остальных флагов
		default:
			ASSERT_TRUE(this->_fmk->is(this->_parameter.letter2, this->_parameter.flag));
	}
}

/**
 * @brief Метод тестирования метода проверки символов и строк
 *
 */
TEST_P(FmkIsParameterizedFixture, FmkIsText1Test){
	// Тестируем проверку строки
	ASSERT_TRUE(this->_fmk->is(this->_parameter.text1, this->_parameter.flag));
}

/**
 * @brief Метод тестирования метода проверки символов и строк
 *
 */
TEST_P(FmkIsParameterizedFixture, FmkIsText2Test){
	// Тестируем проверку строки
	ASSERT_TRUE(this->_fmk->is(this->_parameter.text2, this->_parameter.flag));
}

/**
 * @brief Инициализация параметров тестирования метода проверки символов и строк
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, FmkIsParameterizedFixture,
	::testing::Values(
		FmkIsTestParameter({
			'A',
			L'Ф',
			"AFGHANISTAN",
			L"ФРАНЦИЯ",
			awh::fmk_t::check_t::UPPER
		}),
		FmkIsTestParameter({
			'a',
			L'а',
			"afghanistan",
			L"франиция",
			awh::fmk_t::check_t::LOWER
		}),
		FmkIsTestParameter({
			'a',
			L'a',
			"afghanistan",
			L"france",
			awh::fmk_t::check_t::LATIAN
		}),
		FmkIsTestParameter({
			'a',
			L'a',
			"afghanistan",
			L"france",
			awh::fmk_t::check_t::UTF8
		}),
		FmkIsTestParameter({
			0,
			0,
			"https://pangeoradar.ru/host/data?id=15&post=get#stop",
			L"https://пангеорадар.рф/host/data?id=15&post=get#stop",
			awh::fmk_t::check_t::URL
		}),
		FmkIsTestParameter({
			'a',
			L'a',
			"afghanistan",
			L"france",
			awh::fmk_t::check_t::PRINT
		}),
		FmkIsTestParameter({
			' ',
			L' ',
			"Hello World!!!",
			L"Привет Мир!!!",
			awh::fmk_t::check_t::SPACE
		}),
		FmkIsTestParameter({
			'5',
			L'8',
			"-802843",
			L"+18842",
			awh::fmk_t::check_t::NUMBER
		}),
		FmkIsTestParameter({
			'5',
			L'8',
			"-802843.3882",
			L"+18842.8892",
			awh::fmk_t::check_t::DECIMAL
		}),
		FmkIsTestParameter({
			'5',
			L'8',
			"53ABC-32",
			L"84MB15",
			awh::fmk_t::check_t::PSEUDO_NUMBER
		}),
		FmkIsTestParameter({
			'A',
			L'b',
			"53ABC-32",
			L"Ваш номер AB-332 до 2-х часов",
			awh::fmk_t::check_t::PRESENCE_LATIAN
		})
	)
);

/**
 * @brief Структура параметров тестирования метода сравнения строк
 *
 */
struct FmkCompareTestParameter {
	// Первая строка для сравнения в однобайтовой кодировке
	std::string forst1 = "";
	// Вторая строка для сравнения в однобайтовой кодировке
	std::string second1 = "";
	// Первая строка для сравнения в многобайтовой кодировке
	std::wstring forst2 = L"";
	// Вторая строка для сравнения в многобайтовой кодировке
	std::wstring second2 = L"";
};

/**
 * @brief Класс параметризованного теста для метода сравнения строк
 *
 */
class FmkCompareParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkCompareTestParameter> {
	public:
		FmkCompareTestParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования метода сравнения строк №1
 *
 */
TEST_P(FmkCompareParameterizedFixture, FmkCompare1Test){
	// Тестируем сравнение строк
	ASSERT_TRUE(this->_fmk->compare(this->_parameter.forst1, this->_parameter.second1));
}

/**
 * @brief Метод тестирования метода сравнения строк №2
 *
 */
TEST_P(FmkCompareParameterizedFixture, FmkCompare2Test){
	// Тестируем сравнение строк
	ASSERT_TRUE(this->_fmk->compare(this->_parameter.forst2, this->_parameter.second2));
}

/**
 * @brief Инициализация параметров тестирования метода сравнения строк
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, FmkCompareParameterizedFixture,
	::testing::Values(
		FmkCompareTestParameter({
			"Hello World!!!",
			"Hello World!!!",
			L"Привет Мир!!!",
			L"Привет Мир!!!"
		}),
		FmkCompareTestParameter({
			"PangeoRadar",
			"PangeoRadar",
			L"ПангеоРадар",
			L"ПангеоРадар"
		})
	)
);

/**
 * @brief Структура параметров тестирования метода получения временной метки
 *
 */
struct FmkTimestampTestParameter {
	// Тип временной метки
	awh::fmk_t::chrono_t stamp = awh::fmk_t::chrono_t::NONE;
};

/**
 * @brief Класс параметризованного теста для метода получения временной метки
 *
 */
class FmkTimestampParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkTimestampTestParameter> {
	public:
		FmkTimestampTestParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования метода получения временной метки
 *
 */
TEST_P(FmkTimestampParameterizedFixture, FmkTimestampTest){
	// Тестируем получение временной метки
	ASSERT_TRUE(this->_fmk->timestamp <uint8_t> (this->_parameter.stamp) > 0);
	ASSERT_TRUE(this->_fmk->timestamp <uint16_t> (this->_parameter.stamp) > 0);
	ASSERT_TRUE(this->_fmk->timestamp <uint32_t> (this->_parameter.stamp) > 0);
	ASSERT_TRUE(this->_fmk->timestamp <uint64_t> (this->_parameter.stamp) > 0);
	ASSERT_TRUE(this->_fmk->timestamp <float> (this->_parameter.stamp) > .0f);
	ASSERT_TRUE(this->_fmk->timestamp <double> (this->_parameter.stamp) > .0);
	ASSERT_FALSE(this->_fmk->timestamp <std::string> (this->_parameter.stamp).empty());
}

/**
 * @brief Инициализация параметров тестирования метода получения временной метки
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, FmkTimestampParameterizedFixture,
	::testing::Values(
		FmkTimestampTestParameter({awh::fmk_t::chrono_t::YEAR}),
		FmkTimestampTestParameter({awh::fmk_t::chrono_t::MONTH}),
		FmkTimestampTestParameter({awh::fmk_t::chrono_t::WEEK}),
		FmkTimestampTestParameter({awh::fmk_t::chrono_t::DAY}),
		FmkTimestampTestParameter({awh::fmk_t::chrono_t::HOUR}),
		FmkTimestampTestParameter({awh::fmk_t::chrono_t::MINUTES}),
		FmkTimestampTestParameter({awh::fmk_t::chrono_t::SECONDS}),
		FmkTimestampTestParameter({awh::fmk_t::chrono_t::MILLISECONDS}),
		FmkTimestampTestParameter({awh::fmk_t::chrono_t::MICROSECONDS}),
		FmkTimestampTestParameter({awh::fmk_t::chrono_t::NANOSECONDS})
	)
);

/**
 * @brief Структура параметров тестирования метода конвертации кодировок
 *
 */
struct FmkIconvTestParameter {
	// Текст для конвертации кодировок
	std::string text = "";
};

/**
 * @brief Класс параметризованного теста для метода конвертации кодировок
 *
 */
class FmkIconvParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkIconvTestParameter> {
	public:
		FmkIconvTestParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования метода конвертации кодировок
 *
 */
TEST_P(FmkIconvParameterizedFixture, FmkIconvTest){
	/**
	 * @details Перекодировка выполняется модулем перекодировки Framework и от состава
	 *          операционной системы не зависит, поэтому проверка выполняется на всех
	 *          операционных системах одинаково.
	 */
	// Конвертируем из UTF-8 в CP1251
	const auto & result1 = this->_fmk->transcode(this->_parameter.text, awh::fmk_t::codepage_t::UTF8, awh::fmk_t::codepage_t::CP1251);
	// Проверяем что результат не в UTF-8
	ASSERT_FALSE(this->_fmk->is(result1, awh::fmk_t::check_t::UTF8));
	// Конвертируем из CP1251 в UTF-8
	const auto & result2 = this->_fmk->transcode(result1, awh::fmk_t::codepage_t::CP1251, awh::fmk_t::codepage_t::UTF8);
	// Проверяем что результат в UTF-8
	ASSERT_TRUE(this->_fmk->is(result2, awh::fmk_t::check_t::UTF8));
	// Сравниваем что исходный текст и результат совпадают
	ASSERT_EQ(this->_parameter.text, result2);
}

/**
 * @brief Инициализация параметров тестирования метода конвертации кодировок
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, FmkIconvParameterizedFixture,
	::testing::Values(
		FmkIconvTestParameter({"Привет Мир!!!"}),
		FmkIconvTestParameter({"ООО Пангео Радар"})
	)
);

/**
 * @brief Структура параметров тестирования метода трансформации символов и строк
 *
 */
struct FmkTransformTestParameter {
	// Ожидаемый символ в однобайтовой кодировке
	char letter1 = 0;
	// Ожидаемый символ в многобайтовой кодировке
	wchar_t letter2 = 0;
	// Ожидаемая строка в однобайтовой кодировке
	std::string text1 = "";
	// Ожидаемая строка в многобайтовой кодировке
	std::wstring text2 = L"";
	// Результат в однобайтовой кодировке
	std::string result1 = "";
	// Результат в многобайтовой кодировке
	std::wstring result2 = L"";
	// Флаг трансформации
	awh::fmk_t::transform_t flag = awh::fmk_t::transform_t::NONE;
};

/**
 * @brief Класс параметризованного теста для метода трансформации символов и строк
 *
 */
class FmkTransformParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkTransformTestParameter> {
	public:
		FmkTransformTestParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования метода трансформации символов и строк
 *
 */
TEST_P(FmkTransformParameterizedFixture, FmkTransformLetter1Test){
	/**
	 * Выполняем проверку символа в зависимости от флага
	 */
	switch(static_cast <uint8_t> (this->_parameter.flag)){
		// Особый случай для обрезки пробелов
		case static_cast <uint8_t> (awh::fmk_t::transform_t::TRIM):
			ASSERT_EQ(this->_parameter.result1.front(), this->_parameter.letter1);
		break;
		// Особый случай для преобразования в верхний регистр
		case static_cast <uint8_t> (awh::fmk_t::transform_t::SMART_CASE):
			ASSERT_EQ(this->_parameter.result1.front(), this->_fmk->transform(this->_parameter.letter1, awh::fmk_t::transform_t::UPPER_CASE));
		break;
		// Общий случай для всех остальных флагов
		default:
			ASSERT_EQ(this->_parameter.result1.front(), this->_fmk->transform(this->_parameter.letter1, this->_parameter.flag));
	}
}

/**
 * @brief Метод тестирования метода трансформации символов и строк
 *
 */
TEST_P(FmkTransformParameterizedFixture, FmkTransformLetter2Test){
	/**
	 * Выполняем проверку символа в зависимости от флага
	 */
	switch(static_cast <uint8_t> (this->_parameter.flag)){
		// Особый случай для обрезки пробелов
		case static_cast <uint8_t> (awh::fmk_t::transform_t::TRIM):
			ASSERT_EQ(this->_parameter.result2.front(), this->_parameter.letter2);
		break;
		// Особый случай для преобразования в верхний регистр
		case static_cast <uint8_t> (awh::fmk_t::transform_t::SMART_CASE):
			ASSERT_EQ(this->_parameter.result2.front(), this->_fmk->transform(this->_parameter.letter2, awh::fmk_t::transform_t::UPPER_CASE));
		break;
		// Общий случай для всех остальных флагов
		default:
			ASSERT_EQ(this->_parameter.result2.front(), this->_fmk->transform(this->_parameter.letter2, this->_parameter.flag));
	}
}

/**
 * @brief Метод тестирования метода трансформации символов и строк
 *
 */
TEST_P(FmkTransformParameterizedFixture, FmkTransformText1Test){
	// Тестируем метод трансформации строки
	ASSERT_EQ(this->_parameter.result1, this->_fmk->transform(this->_parameter.text1, this->_parameter.flag));
}

/**
 * @brief Метод тестирования метода трансформации символов и строк
 *
 */
TEST_P(FmkTransformParameterizedFixture, FmkTransformText2Test){
	// Тестируем метод трансформации строки
	ASSERT_EQ(this->_parameter.result2, this->_fmk->transform(this->_parameter.text2, this->_parameter.flag));
}

/**
 * @brief Инициализация параметров тестирования метода трансформации символов и строк
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, FmkTransformParameterizedFixture,
	::testing::Values(
		FmkTransformTestParameter({
			'H',
			L'П',
			"     Hello World!!! ",
			L"    Привет Мир!!! ",
			"Hello World!!!",
			L"Привет Мир!!!",
			awh::fmk_t::transform_t::TRIM
		}),
		FmkTransformTestParameter({
			'h',
			L'п',
			"Hello World!!!",
			L"Привет Мир!!!",
			"HELLO WORLD!!!",
			L"ПРИВЕТ МИР!!!",
			awh::fmk_t::transform_t::UPPER_CASE
		}),
		FmkTransformTestParameter({
			'H',
			L'П',
			"Hello World!!!",
			L"Привет Мир!!!",
			"hello world!!!",
			L"привет мир!!!",
			awh::fmk_t::transform_t::LOWER_CASE
		}),
		FmkTransformTestParameter({
			'H',
			L'П',
			"hello world!!!",
			L"ПРИВЕТ МИР!!!",
			"Hello World!!!",
			L"Привет Мир!!!",
			awh::fmk_t::transform_t::SMART_CASE
		})
	)
);

/**
 * @brief Структура параметров тестирования метода разделения строк
 *
 */
struct FmkJoinTestParameter {
	// Разделитель для однобайтовой строки
	std::string delim1 = "";
	// Разделитель для многобайтовой строки
	std::wstring delim2 = L"";
	// Ожидаемый результат для однобайтовой строки
	std::string result1 = "";
	// Ожидаемый результат для многобайтовой строки
	std::wstring result2 = L"";
	// Список элементов для однобайтовой строки
	std::vector <std::string> items1;
	// Список элементов для многобайтовой строки
	std::vector <std::wstring> items2;
};

/**
 * @brief Класс параметризованного теста для метода соединения строк
 *
 */
class FmkJoinParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkJoinTestParameter> {
	public:
		FmkJoinTestParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования метода соединения строк №1
 *
 */
TEST_P(FmkJoinParameterizedFixture, FmkJoin1Test){
	ASSERT_EQ(this->_parameter.result1, this->_fmk->join(this->_parameter.items1, this->_parameter.delim1));
}

/**
 * @brief Метод тестирования метода соединения строк №2
 *
 */
TEST_P(FmkJoinParameterizedFixture, FmkJoin2Test){
	ASSERT_EQ(this->_parameter.result2, this->_fmk->join(this->_parameter.items2, this->_parameter.delim2));
}

/**
 * @brief Инициализация параметров тестирования метода соединения строк
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, FmkJoinParameterizedFixture,
	::testing::Values(
		FmkJoinTestParameter({
			" ",
			L", ",
			"Hello World !!!",
			L"Один, Два, Три, Четыре, Пять",
			{"Hello", "World", "!!!"},
			{L"Один", L"Два", L"Три", L"Четыре", L"Пять"}
		}),
		FmkJoinTestParameter({
			"&",
			L"=",
			"params&test&data",
			L"2=Два=1=Один=3=Три",
			{"params", "test", "data"},
			{L"2", L"Два", L"1", L"Один", L"3", L"Три"}
		})
	)
);

/**
 * @brief Структура параметров тестирования метода разделения строк
 *
 */
struct FmkSplitTestParameter {
	// Разделитель для однобайтовой строки
	std::string delim1 = "";
	// Разделитель для многобайтовой строки
	std::wstring delim2 = L"";
	// Текст для однобайтовой строки
	std::string text1 = "";
	// Текст для многобайтовой строки
	std::wstring text2 = L"";
	// Ожидаемый результат для однобайтовой строки
	std::vector <std::string> result1;
	// Ожидаемый результат для многобайтовой строки
	std::vector <std::wstring> result2;
};

/**
 * @brief Класс параметризованного теста для метода разделения строк
 *
 */
class FmkSplitParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkSplitTestParameter> {
	public:
		FmkSplitTestParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования метода разделения строк №1
 *
 */
TEST_P(FmkSplitParameterizedFixture, FmkSplit1Test){
	// Создаем контейнер для хранения результата разделения
	std::vector <std::string> container;
	// Выполняем разделение строки
	ASSERT_EQ(this->_parameter.result1, this->_fmk->split(this->_parameter.text1, this->_parameter.delim1, container));
}

/**
 * @brief Метод тестирования метода разделения строк №2
 *
 */
TEST_P(FmkSplitParameterizedFixture, FmkSplit2Test){
	// Создаем контейнер для хранения результата разделения
	std::vector <std::wstring> container;
	// Выполняем разделение строки
	ASSERT_EQ(this->_parameter.result2, this->_fmk->split(this->_parameter.text2, this->_parameter.delim2, container));
}

/**
 * @brief Инициализация параметров тестирования метода разделения строк
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, FmkSplitParameterizedFixture,
	::testing::Values(
		FmkSplitTestParameter({
			" ",
			L", ",
			"Hello World !!!",
			L"Один, Два, Три, Четыре, Пять",
			{"Hello", "World", "!!!"},
			{L"Один", L"Два", L"Три", L"Четыре", L"Пять"}
		}),
		FmkSplitTestParameter({
			"&",
			L"=",
			"params&test&data",
			L"2=Два=1=Один=3=Три",
			{"params", "test", "data"},
			{L"2", L"Два", L"1", L"Один", L"3", L"Три"}
		})
	)
);

/**
 * @brief Структура параметров тестирования метода конвертации кодировок
 *
 */
struct FmkConvertTestParameter {
	// Текст для конвертации из однобайтовой кодировки в многобайтовую
	std::string text1 = "";
	// Текст для конвертации из многобайтовой кодировки в однобайтовую
	std::wstring text2 = L"";
};

/**
 * @brief Класс параметризованного теста для метода конвертации кодировок
 *
 */
class FmkConvertParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkConvertTestParameter> {
	public:
		FmkConvertTestParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования метода конвертации кодировок
 *
 */
TEST_P(FmkConvertParameterizedFixture, FmkConvertTest){
	// Тестируем конвертацию из однобайтовой кодировки в многобайтовую
	ASSERT_EQ(this->_parameter.text2, this->_fmk->convert(this->_parameter.text1));
	// Тестируем конвертацию из однобайтовой кодировки в многобайтовую
	ASSERT_EQ(this->_parameter.text2.c_str(), this->_fmk->convert(this->_parameter.text1.c_str()));
	// Тестируем конвертацию из многобайтовой кодировки в однобайтовую
	ASSERT_EQ(this->_parameter.text1, this->_fmk->convert(this->_parameter.text2));
	// Тестируем конвертацию из многобайтовой кодировки в однобайтовую
	ASSERT_EQ(this->_parameter.text1.c_str(), this->_fmk->convert(this->_parameter.text2.c_str()));
}

/**
 * @brief Инициализация параметров тестирования метода конвертации кодировок
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, FmkConvertParameterizedFixture,
	::testing::Values(
		FmkConvertTestParameter({
			"Привет Мир!!!",
			L"Привет Мир!!!"
		}),
		FmkConvertTestParameter({
			"ООО ПангеоРадар!",
			L"ООО ПангеоРадар!"
		})
	)
);

/**
 * @brief Структура параметров тестирования метода получения размера
 *
 */
struct FmkSizeTestParameter {
	// Размер данных в байтах
	size_t size = 0;
	// Номер элемента для получения размера
	uint64_t num = 0;
	// Данные для получения размера
	const char * data = nullptr;
};

/**
 * @brief Класс параметризованного теста для метода получения размера
 *
 */
class FmkSizeParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkSizeTestParameter> {
	public:
		FmkSizeTestParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования метода получения размера
 *
 */
TEST_P(FmkSizeParameterizedFixture, FmkSizeTest){
	// Проверяем что размер данных соответствует ожидаемому
	if(this->_parameter.data == nullptr)
		// Если данные не указаны, то проверяем размер по номеру элемента
		ASSERT_EQ(this->_parameter.size, this->_fmk->size(this->_parameter.num));
	// Если данные указаны, то проверяем размер по данным и размеру
	else ASSERT_EQ(static_cast <size_t> (this->_parameter.num), this->_fmk->size(this->_parameter.data, this->_parameter.size));
}

/**
 * @brief Инициализация параметров тестирования метода получения размера
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, FmkSizeParameterizedFixture,
	::testing::Values(
		FmkSizeTestParameter({8, 18446744073709551615}),
		FmkSizeTestParameter({4, 4294967295}),
		FmkSizeTestParameter({2, 65535}),
		FmkSizeTestParameter({1, 255}),
		FmkSizeTestParameter({14, 14, "Hello World!!!"})
	)
);

/**
 * @brief Структура параметров тестирования метода сравнения чисел
 *
 */
struct FmkGreaterTestParameter {
	// Первое число для сравнения
	uint64_t num1 = 0;
	// Второе число для сравнения
	uint64_t num2 = 0;
	// Первые данные для сравнения
	std::vector <uint64_t> data1;
	// Вторые данные для сравнения
	std::vector <uint64_t> data2;
};

/**
 * @brief Класс параметризованного теста для метода сравнения чисел
 *
 */
class FmkGreaterParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkGreaterTestParameter> {
	public:
		FmkGreaterTestParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования метода сравнения чисел
 *
 */
TEST_P(FmkGreaterParameterizedFixture, FmkGreaterTest){
	// Проверяем что первое число больше второго
	if(this->_parameter.data1.empty() && this->_parameter.data2.empty())
		// Если данные не указаны, то проверяем числа
		ASSERT_TRUE(this->_fmk->isGreater(this->_parameter.num1, this->_parameter.num2));
	// Если данные указаны, то проверяем данные
	else ASSERT_TRUE(this->_fmk->isGreater(this->_parameter.data1.data(), this->_parameter.data2.data(), this->_parameter.data2.size() * sizeof(uint64_t)));
}

/**
 * @brief Инициализация параметров тестирования метода сравнения чисел
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, FmkGreaterParameterizedFixture,
	::testing::Values(
		FmkGreaterTestParameter({18446744073709551615, 18446744073709551614}),
		FmkGreaterTestParameter({4294967295, 4294967294}),
		FmkGreaterTestParameter({65535, 65534}),
		FmkGreaterTestParameter({255, 254}),
		FmkGreaterTestParameter({
			0, 0,
			std::vector <uint64_t> ({18446744073709551615, 18446744073709551615}),
			std::vector <uint64_t> ({18446744073709551615, 18446744073709551614})
		})
	)
);

/**
 * @brief Структура параметров тестирования метода преобразования числа в строку
 *
 */
struct FmkItoaTestParameter {
	// Система счисления
	uint8_t radix = 0;
	// Число для преобразования
	uint32_t value = 0;
	// Текст для преобразования
	std::string text = "";
	// Ожидаемый результат
	std::string result = "";
};

/**
 * @brief Класс параметризованного теста для метода преобразования числа в строку
 *
 */
class FmkItoaParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkItoaTestParameter> {
	public:
		FmkItoaTestParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования метода преобразования числа в строку
 *
 */
TEST_P(FmkItoaParameterizedFixture, FmkItoaTest){
	// Проверяем что текст для преобразования пустой
	if(this->_parameter.text.empty())
		// Если текст не указан, то проверяем число
		ASSERT_EQ(this->_parameter.result, this->_fmk->itoa(this->_parameter.value, this->_parameter.radix));
	// Если текст указан, то проверяем текст
	else ASSERT_EQ(this->_parameter.result, this->_fmk->itoa(this->_parameter.text.c_str(), this->_parameter.text.length(), 2));
}

/**
 * @brief Инициализация параметров тестирования метода преобразования числа в строку
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, FmkItoaParameterizedFixture,
	::testing::Values(
		FmkItoaTestParameter({2, 2986, "", "00000000000000000000101110101010"}),
		FmkItoaTestParameter({8, 2986, "", "5652"}),
		FmkItoaTestParameter({16, 2986, "", "BAA"}),
		FmkItoaTestParameter({20, 2986, "", "796"}),
		FmkItoaTestParameter({35, 2986, "", "2FB"}),
		FmkItoaTestParameter({0, 0, "Hello World!!!", "0010000100100001001000010110010001101100011100100110111101010111001000000110111101101100011011000110010101001000"}),
		FmkItoaTestParameter({0, 0, "Привет Мир!!!", "00100001001000010010000110000000110100011011100011010000100111001101000000100000100000101101000110110101110100001011001011010000101110001101000010000000110100011001111111010000"})
	)
);

/**
 * @brief Структура параметров тестирования метода преобразования строки в число
 *
 */
struct FmkAtoiTestParameter {
	// Система счисления
	uint8_t radix = 0;
	// Ожидаемый результат
	uint32_t result = 0;
	// Строка для преобразования
	std::string value = "";
	// Текст для преобразования
	std::string text = "";
};

/**
 * @brief Класс параметризованного теста для метода преобразования строки в число
 *
 */
class FmkAtoiParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkAtoiTestParameter> {
	public:
		FmkAtoiTestParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования метода преобразования строки в число
 *
 */
TEST_P(FmkAtoiParameterizedFixture, FmkAtoiTest){
	// Если текст для преобразования пустой
	if(this->_parameter.text.empty())
		// Проверяем что результат преобразования строки в число совпадает с ожидаемым
		ASSERT_EQ(this->_parameter.result, this->_fmk->atoi <uint32_t> (this->_parameter.value, this->_parameter.radix));
	// Если текст для преобразования не пустой
	else {
		// Вычисляем размер результата
		size_t size = 0;
		// Получаем количество битов в строке
		const size_t count = (this->_parameter.value.length() % 8);
		// Если количество битов равно нулю
		if(count == 0)
			// Получаем размер результата
			size = (this->_parameter.value.length() / 8);
		// Если количество битов не равно нулю, получаем размер результата с учетом остатка
		else size = ((this->_parameter.value.length() + (8 - count)) / 8);
		// Результат преобразования
		std::string result(size, 0);
		// Выполняем преобразование
		this->_fmk->atoi(this->_parameter.value, 2, result.data(), result.size());
		// Проверяем что результат преобразования совпадает с ожидаемым
		ASSERT_EQ(result, this->_parameter.text);
	}
}

/**
 * @brief Инициализация параметров тестирования метода преобразования строки в число
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, FmkAtoiParameterizedFixture,
	::testing::Values(
		FmkAtoiTestParameter({2, 2986, "00000000000000000000101110101010"}),
		FmkAtoiTestParameter({8, 2986, "5652"}),
		FmkAtoiTestParameter({16, 2986, "BAA"}),
		FmkAtoiTestParameter({20, 2986, "796"}),
		FmkAtoiTestParameter({35, 2986, "2FB"}),
		FmkAtoiTestParameter({0, 0, "0010000100100001001000010110010001101100011100100110111101010111001000000110111101101100011011000110010101001000", "Hello World!!!"}),
		FmkAtoiTestParameter({0, 0, "00100001001000010010000110000000110100011011100011010000100111001101000000100000100000101101000110110101110100001011001011010000101110001101000010000000110100011001111111010000", "Привет Мир!!!"})
	)
);

/**
 * @brief Структура параметров тестирования метода noexp
 *
 */
struct FmkNoexpTestParameter {
	// Количество знаков после запятой
	uint8_t step = 0;
	// Значение для преобразования
	double value = 0.;
	// Флаг, указывающий на то, что нужно преобразовать только число
	bool onlyNum = false;
	// Ожидаемый результат
	std::string result = "";
};

/**
 * @brief Класс параметризованного теста для метода noexp
 *
 */
class FmkNoexpParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkNoexpTestParameter> {
	public:
		FmkNoexpTestParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования метода noexp
 *
 */
TEST_P(FmkNoexpParameterizedFixture, FmkNoexpTest){
	// Устанавливаем локаль для корректного отображения чисел
	this->_fmk->setLocale();
	// Проверяем, что количество знаков после запятой больше нуля
	if(this->_parameter.step > 0)
		// Если количество знаков после запятой больше нуля, то проверяем результат с учетом шага
		ASSERT_EQ(this->_parameter.result, this->_fmk->noexp(this->_parameter.value, this->_parameter.step));
	// Если количество знаков после запятой равно нулю
	else ASSERT_EQ(this->_parameter.result, this->_fmk->noexp(this->_parameter.value, this->_parameter.onlyNum));
}

/**
 * @brief Инициализация параметров тестирования метода noexp
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, FmkNoexpParameterizedFixture,
	::testing::Values(
		FmkNoexpTestParameter({3, 2986.808299, false, "2986.808"}),
		FmkNoexpTestParameter({4, 2986.808299, false, "2986.8083"}),
		FmkNoexpTestParameter({0, 1000000000.808299, false, "1000000000.808299"}),
		FmkNoexpTestParameter({0, 1000000000.808299, true, "1000000000.808299"}),
		FmkNoexpTestParameter({0, 0.80829989222211145, true, "0.8082998922221114"}),
		FmkNoexpTestParameter({0, 1e+19, false, "10000000000000000000"}),
		FmkNoexpTestParameter({0, 0.111, false, "0.111"}),
		FmkNoexpTestParameter({0, 0.001, false, "0.001"}),
		FmkNoexpTestParameter({0, 0.0001, false, "0.0001"}),
		FmkNoexpTestParameter({0, 1e-5, false, "0.00001"}),
		FmkNoexpTestParameter({0, 1.001, false, "1.001"}),
		FmkNoexpTestParameter({0, 1536. / 1024., false, "1.5"}),
		FmkNoexpTestParameter({0, 1500. / 1024., false, "1.46484375"}),
		FmkNoexpTestParameter({0, 2986., false, "2986"}),
		FmkNoexpTestParameter({0, 0., false, "0"}),
		FmkNoexpTestParameter({0, -2986.808299, false, "-2986.808299"}),
		FmkNoexpTestParameter({0, -0.111, true, "-0.111"}),
		FmkNoexpTestParameter({3, 2986., false, "2986"}),
		FmkNoexpTestParameter({3, -2986.808299, false, "-2986.808"}),
		FmkNoexpTestParameter({8, 0.111, false, "0.11100000"})
	)
);

/**
 * @brief Структура параметров тестирования метода rate
 *
 */
struct FmkRateTestParameter {
	// Первое число для расчета
	float num1 = 0.f;
	// Второе число для расчета
	float num2 = 0.f;
	// Ожидаемый результат
	float result = 0.f;
};

/**
 * @brief Класс параметризованного теста для метода rate
 *
 */
class FmkRateParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkRateTestParameter> {
	public:
		FmkRateTestParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования метода rate
 *
 */
TEST_P(FmkRateParameterizedFixture, FmkRateTest){
	// Проверяем, что результат расчета совпадает с ожидаемым
	ASSERT_EQ(static_cast <int32_t> (this->_parameter.result), static_cast <int32_t> (this->_fmk->rate(this->_parameter.num1, this->_parameter.num2)));
}

/**
 * @brief Инициализация параметров тестирования метода проверки на сколько одно число больше другого в процентах
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, FmkRateParameterizedFixture,
	::testing::Values(
		FmkRateTestParameter({58929.f, 38963.f, 51.f}),
		FmkRateTestParameter({38963.f, 58929.f, -33.f}),
		FmkRateTestParameter({74423.f, 22.f, 338186.f}),
		FmkRateTestParameter({345.f, 9865.f, -96.f}),
		FmkRateTestParameter({34567.f, 90876.f, -61.f}),
		FmkRateTestParameter({13.f, 100.f, -87.f}),
		FmkRateTestParameter({100.f, 13.f, 669.f})
	)
);

/**
 * @brief Структура параметров тестирования метода floor
 *
 */
struct FmkFloorTestParameter {
	// Количество знаков после запятой
	uint8_t count = 0;
	// Число для округления
	double num = 0.;
	// Ожидаемый результат
	double result = 0.;
};

/**
 * @brief Класс параметризованного теста для метода floor
 *
 */
class FmkFloorParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkFloorTestParameter> {
	public:
		FmkFloorTestParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования метода floor
 *
 */
TEST_P(FmkFloorParameterizedFixture, FmkFloorTest){
	// Проверяем, что результат округления совпадает с ожидаемым
	ASSERT_EQ(this->_parameter.result, this->_fmk->floor(this->_parameter.num, this->_parameter.count));
}

/**
 * @brief Инициализация параметров тестирования метода floor
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, FmkFloorParameterizedFixture,
	::testing::Values(
		FmkFloorTestParameter({3, 38963.892549, 38963.892}),
		FmkFloorTestParameter({4, 38963.892549, 38963.8925}),
		FmkFloorTestParameter({5, 38963.892549, 38963.89254}),
		FmkFloorTestParameter({6, 38963.892549, 38963.892549})
	)
);

/**
 * @brief Структура параметров тестирования метода конвертирования римских чисел в арабские
 *
 */
struct FmkRome2arabicTestParameter {
	// Результат конвертации
	uint16_t result = 0;
	// Римское число в виде строки
	std::string num1 = "";
	// Римское число в виде широкой строки
	std::wstring num2 = L"";
};

/**
 * @brief Класс параметризованного теста для метода конвертирования римских чисел в арабские
 *
 */
class FmkRome2arabicParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkRome2arabicTestParameter> {
	public:
		FmkRome2arabicTestParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования метода конвертирования римских чисел в арабские
 *
 */
TEST_P(FmkRome2arabicParameterizedFixture, FmkRome2arabicTest){
	// Если римское число представлено в виде строки
	if(!this->_parameter.num1.empty())
		// Выполняем проверку конвертации римского числа в арабское число из строки
		ASSERT_EQ(this->_parameter.result, this->_fmk->rome2arabic(this->_parameter.num1));
	// Если римское число представлено в виде широкой строки
	else if(!this->_parameter.num2.empty())
		// Выполняем проверку конвертации римского числа в арабское число из широкой строки
		ASSERT_EQ(this->_parameter.result, this->_fmk->rome2arabic(this->_parameter.num2));
	// Если римское число не представлено ни в одной из форм
	else ASSERT_TRUE(false);
}

/**
 * @brief Инициализация параметров тестирования метода конвертирования римских чисел в арабские
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, FmkRome2arabicParameterizedFixture,
	::testing::Values(
		FmkRome2arabicTestParameter({3874, "MMMDCCCLXXIV"}),
		FmkRome2arabicTestParameter({3874, "", L"MMMDCCCLXXIV"}),
		FmkRome2arabicTestParameter({2025, "MMXXV"}),
		FmkRome2arabicTestParameter({2025, "", L"MMXXV"})
	)
);

/**
 * @brief Структура параметров тестирования метода конвертирования арабских чисел в римские
 *
 */
struct FmkArabic2romeTestParameter {
	// Число для конвертации
	uint32_t number = 0;
	// Число в виде строки
	std::string word1 = "";
	// Число в виде широкой строки
	std::wstring word2 = L"";
	// Результат конвертации в виде строки
	std::string result1 = "";
	// Результат конвертации в виде широкой строки
	std::wstring result2 = L"";
};

/**
 * @brief Класс параметризованного теста для метода конвертирования арабских чисел в римские
 *
 */
class FmkArabic2romeParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkArabic2romeTestParameter> {
	public:
		FmkArabic2romeTestParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования метода конвертирования арабских чисел в римские
 *
 */
TEST_P(FmkArabic2romeParameterizedFixture, FmkArabic2romeTest){
	// Если число для конвертации больше нуля
	if(this->_parameter.number > 0)
		// Выполняем проверку конвертации арабского числа в римское число из числа
		ASSERT_EQ(this->_parameter.result2, this->_fmk->arabic2rome(this->_parameter.number));
	// Если число для конвертации представлено в виде строки
	else if(!this->_parameter.word1.empty())
		// Выполняем проверку конвертации арабского числа в римское число из строки
		ASSERT_EQ(this->_parameter.result1, this->_fmk->arabic2rome(this->_parameter.word1));
	// Если число для конвертации представлено в виде широкой строки
	else if(!this->_parameter.word2.empty())
		// Выполняем проверку конвертации арабского числа в римское число из широкой строки
		ASSERT_EQ(this->_parameter.result2, this->_fmk->arabic2rome(this->_parameter.word2));
	// Если римское число не представлено ни в одной из форм
	else ASSERT_TRUE(false);
}

/**
 * @brief Инициализация параметров тестирования метода конвертирования арабских чисел в римские
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, FmkArabic2romeParameterizedFixture,
	::testing::Values(
		FmkArabic2romeTestParameter({3874, "", L"", "", L"MMMDCCCLXXIV"}),
		FmkArabic2romeTestParameter({0, "3874", L"", "MMMDCCCLXXIV"}),
		FmkArabic2romeTestParameter({0, "", L"2025", "", L"MMXXV"})
	)
);

/**
 * @brief Структура параметров тестирования метода подсчёта количества вхождений буквы в строку
 *
 */
struct FmkCountLetterTestParameter {
	// Результат подсчёта количества вхождений буквы в строку
	size_t result = 0;
	// Буква для подсчёта
	wchar_t letter = 0;
	// Строка для подсчёта
	std::wstring word = L"";
};

/**
 * @brief Класс параметризованного теста для метода подсчёта количества вхождений буквы в строку
 *
 */
class FmkCountLetterParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkCountLetterTestParameter> {
	public:
		FmkCountLetterTestParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования метода подсчёта количества вхождений буквы в строку
 *
 */
TEST_P(FmkCountLetterParameterizedFixture, FmkCountLetterTest){
	// Выполняем подсчёт количества вхождений буквы в строку
	ASSERT_EQ(this->_parameter.result, this->_fmk->countLetter(this->_parameter.word, this->_parameter.letter));
}

/**
 * @brief Инициализация параметров тестирования метода подсчёта количества вхождений буквы в строку
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, FmkCountLetterParameterizedFixture,
	::testing::Values(
		FmkCountLetterTestParameter({3, L'т', L"Привет этот мир!!!"}),
		FmkCountLetterTestParameter({3, L'l', L"Hello World!!!"}),
		FmkCountLetterTestParameter({2, L'o', L"Hello World!!!"})
	)
);

/**
 * @brief Структура параметров тестирования метода форматирования строки
 *
 */
struct FmkFormatTestParameter {
	// Результат форматирования строки
	std::string result = "";
	// Формат формирования строки
	std::string format = "";
	// Элементы для форматирования
	std::vector <std::string> items;
};

/**
 * @brief Класс параметризованного теста для метода форматирования строки
 *
 */
class FmkFormatParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkFormatTestParameter> {
	public:
		FmkFormatTestParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования метода форматирования строки
 *
 */
TEST_P(FmkFormatParameterizedFixture, FmkFormatTest){
	// Выполняем форматирование строки и проверяем результат
	ASSERT_EQ(this->_parameter.result, this->_fmk->format("%s", this->_parameter.result.c_str()));
	ASSERT_EQ(this->_parameter.result, this->_fmk->format(this->_parameter.format, this->_parameter.items));
}

/**
 * @brief Инициализация параметров тестирования метода форматирования строки
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, FmkFormatParameterizedFixture,
	::testing::Values(
		FmkFormatTestParameter({"Hello World!!!", "$1 $2!!!", {"Hello", "World"}}),
		FmkFormatTestParameter({"Вашм присвоен идентификатор ID=984 и ID=586", "$1 ID=$2 и ID=$3", {"Вашм присвоен идентификатор", "984", "586"}})
	)
);

/**
 * @brief Структура параметров тестирования метода проверки существования подстроки в строке
 *
 */
struct FmkExistsTestParameter {
	// Слово для проверки нахождения в тексте
	std::string word1 = "";
	// Текст в котором производится проверка
	std::string text1 = "";
	// Слово для проверки нахождения в тексте (широкая строка)
	std::wstring word2 = L"";
	// Текст в котором производится проверки (широкая строка)
	std::wstring text2 = L"";
};

/**
 * @brief Класс параметризованного теста для метода проверки существования подстроки в строке
 *
 */
class FmkExistsParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkExistsTestParameter> {
	public:
		FmkExistsTestParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования метода проверки существования подстроки в строке
 *
 */
TEST_P(FmkExistsParameterizedFixture, FmkExistsTest){
	// Проверяем, что слово существует в тексте
	if(!this->_parameter.word1.empty() && !this->_parameter.text1.empty())
		// Если слово и текст указаны, то проверяем существование слова в тексте
		ASSERT_TRUE(this->_fmk->exists(this->_parameter.word1, this->_parameter.text1));
	// Если слово и текст не указаны, то проверяем существование слова в тексте (широкая строка)
	else ASSERT_TRUE(this->_fmk->exists(this->_parameter.word2, this->_parameter.text2));
}

/**
 * @brief Инициализация параметров тестирования метода проверки существования подстроки в строке
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, FmkExistsParameterizedFixture,
	::testing::Values(
		FmkExistsTestParameter({"Wor", "Hello World!!!"}),
		FmkExistsTestParameter({"", "", L"вет", L"Привет Мир!!!"})
	)
);

/**
 * @brief Структура параметров тестирования метода замены подстроки в строке
 *
 */
struct FmkReplaceTestParameter {
	// Текст в котором производится замена
	std::string text1 = "";
	// Слово для замены
	std::string word1 = "";
	// Замена слова
	std::string alt1 = "";
	// Результат после замены
	std::string result1 = "";
	// Текст в котором производится замена (широкая строка)
	std::wstring text2 = L"";
	// Слово для замены (широкая строка)
	std::wstring word2 = L"";
	// Замена слова (широкая строка)
	std::wstring alt2 = L"";
	// Результат после замены (широкая строка)
	std::wstring result2 = L"";
};

/**
 * @brief Класс параметризованного теста для метода замены подстроки в строке
 *
 */
class FmkReplaceParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkReplaceTestParameter> {
	public:
		FmkReplaceTestParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования метода замены подстроки в строке
 *
 */
TEST_P(FmkReplaceParameterizedFixture, FmkReplaceTest){
	// Если текстовые параметры для замены заданы и не пустые
	if(!this->_parameter.text1.empty() && !this->_parameter.word1.empty() && !this->_parameter.result1.empty())
		// Проверяем, что результат замены слова в тексте совпадает с ожидаемым
		ASSERT_EQ(this->_parameter.result1, this->_fmk->replace(this->_parameter.text1, this->_parameter.word1, this->_parameter.alt1));
	// Если широкие текстовые параметры для замены заданы и не пустые, то проверяем результат замены слова в тексте (широкая строка)
	else ASSERT_EQ(this->_fmk->convert(this->_parameter.result2), this->_fmk->convert(this->_fmk->replace(this->_parameter.text2, this->_parameter.word2, this->_parameter.alt2)));
}

/**
 * @brief Инициализация параметров тестирования метода замены подстроки в строке
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, FmkReplaceParameterizedFixture,
	::testing::Values(
		FmkReplaceTestParameter({"Hello World!!!", "o W", "o and W", "Hello and World!!!"}),
		FmkReplaceTestParameter({"", "", "", "", L"Привет Мир!!!", L"ивет Мир", L"ивет этот Мир", L"Привет этот Мир!!!"})
	)
);

/**
 * @brief Структура параметров тестирования метода разбора ключ-значение
 *
 */
struct FmkKVTestParameter {
	// Текст для разбора
	std::string text1 = "";
	// Текст для разбора (широкая строка)
	std::wstring text2 = L"";
	// Разделитель ключ-значение
	std::string delim1 = " ";
	// Разделитель ключ-значение (широкая строка)
	std::wstring delim2 = L" ";
	// Альтернативный результат разбора
	std::unordered_multimap <std::string, std::string> result1;
	// Альтернативный результат разбора (широкая строка)
	std::unordered_multimap <std::wstring, std::wstring> result2;
};

/**
 * @brief Класс параметризованного теста для метода разбора ключ-значение
 *
 */
class FmkKVParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkKVTestParameter> {
	public:
		FmkKVTestParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования метода разбора ключ-значение
 *
 */
TEST_P(FmkKVParameterizedFixture, FmkKVTest){
	// Если текстовые параметры для замены заданы и не пустые
	if(!this->_parameter.text1.empty() && !this->_parameter.delim1.empty()){
		// Проверяем результат разбора ключ-значение
		ASSERT_EQ(this->_parameter.result1, this->_fmk->kv(this->_parameter.text1, this->_parameter.delim1));
	// Если широкие текстовые параметры для замены заданы и не пустые, то проверяем результат разбора ключ-значение (широкая строка)
	} else ASSERT_EQ(this->_parameter.result2, this->_fmk->kv(this->_parameter.text2, this->_parameter.delim2));
}

/**
 * @brief Метод тестирования потокового разбора ключ-значение
 *
 * @details Потоковый разбор обязан выдавать те же самые записи, что и разбор с формированием контейнера,
 *          а также пробрасывать в функцию обратного вызова переданный идентификатор потока разбора
 *
 */
TEST_P(FmkKVParameterizedFixture, FmkKVCallbackTest){
	// Идентификатор потока разбора
	const uint64_t sid = 0x1234567890ABCDEF;
	// Если текстовые параметры для замены заданы и не пустые
	if(!this->_parameter.text1.empty() && !this->_parameter.delim1.empty()){
		// Результат потокового разбора
		std::unordered_multimap <std::string, std::string> result;
		// Выполняем потоковый разбор строки ключ-значение
		this->_fmk->kv(sid, this->_parameter.text1, this->_parameter.delim1, [&result, sid](const uint64_t id, const std::string_view key, const std::string_view value) noexcept -> void {
			// Проверяем, что идентификатор потока разбора проброшен без изменений
			ASSERT_EQ(sid, id);
			// Выполняем формирование записи результата
			result.emplace(key, value);
		});
		// Проверяем результат потокового разбора
		ASSERT_EQ(this->_parameter.result1, result);
	// Если широкие текстовые параметры для замены заданы и не пустые
	} else {
		// Результат потокового разбора (широкая строка)
		std::unordered_multimap <std::wstring, std::wstring> result;
		// Выполняем потоковый разбор строки ключ-значение (широкая строка)
		this->_fmk->kv(sid, this->_parameter.text2, this->_parameter.delim2, [&result, sid](const uint64_t id, const std::wstring_view key, const std::wstring_view value) noexcept -> void {
			// Проверяем, что идентификатор потока разбора проброшен без изменений
			ASSERT_EQ(sid, id);
			// Выполняем формирование записи результата
			result.emplace(key, value);
		});
		// Проверяем результат потокового разбора
		ASSERT_EQ(this->_parameter.result2, result);
	}
}

/**
 * @brief Инициализация параметров тестирования метода разбора ключ-значение
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, FmkKVParameterizedFixture,
	::testing::Values(
		FmkKVTestParameter({
			"http_agent=\"Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:107.0) Gecko/20100101 Firefox/107.0\" http_retcode=200 msg=\"HTTPS post request from 188.43.251.186:59420 to 10.77.194.51:80\" data=\"abc:\\\" deas\\\"\"",
			L"http_agent=\"Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:107.0) Gecko/20100101 Firefox/107.0\" http_retcode=200 msg=\"HTTPS post request from 188.43.251.186:59420 to 10.77.194.51:80\" data=\"abc:\\\" deas\\\"\"",
			" ",
			L" ",
			{
				{"http_agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:107.0) Gecko/20100101 Firefox/107.0"},
				{"http_retcode", "200"},
				{"msg", "HTTPS post request from 188.43.251.186:59420 to 10.77.194.51:80"},
				{"data", "abc:\\\" deas\\\""}
			},{
				{L"http_agent", L"Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:107.0) Gecko/20100101 Firefox/107.0"},
				{L"http_retcode", L"200"},
				{L"msg", L"HTTPS post request from 188.43.251.186:59420 to 10.77.194.51:80"},
				{L"data", L"abc:\\\" deas\\\""}
			}
		}),
		/**
		 * Разбор расширения CEF: пустые значения, значение из нескольких слов и разделитель записей в начале текста
		 */
		FmkKVTestParameter({
			" cs3= cs3Label=CVEID rt=Feb 17 2023 23:30:15.734 YEKT src=172.16.0.4 user=test",
			L"",
			" ",
			L"",
			{
				{"cs3", ""},
				{"cs3Label", "CVEID"},
				{"rt", "Feb 17 2023 23:30:15.734 YEKT"},
				{"src", "172.16.0.4"},
				{"user", "test"}
			},{}
		}),
		/**
		 * Разбор расширения CEF: экранированный разделитель ключа и значения внутри значения,
		 * а также значение последней записи, занимающее весь остаток текста
		 */
		FmkKVTestParameter({
			"origin=10.120.63.36 originsicname=CN\\=chr-cpsg-01,O\\=stal sequencenum=446 action_reason=Early Drop: blocking the connection",
			L"",
			" ",
			L"",
			{
				{"origin", "10.120.63.36"},
				{"originsicname", "CN\\=chr-cpsg-01,O\\=stal"},
				{"sequencenum", "446"},
				{"action_reason", "Early Drop: blocking the connection"}
			},{}
		}),
		/**
		 * Разбор записей с разделителем записей состоящим из нескольких символов
		 */
		FmkKVTestParameter({
			"a=\"x\"; b=\"y\"; c=1",
			L"",
			"; ",
			L"",
			{
				{"a", "x"},
				{"b", "y"},
				{"c", "1"}
			},{}
		}),
		/**
		 * Разбор расширения CEF с повторяющимися ключами: все вхождения должны сохраняться
		 */
		FmkKVTestParameter({
			"ad.prog-id=128394 deviceExternalId=1330334083 ad.prog-id=128394 ad.gid=0 deviceExternalId=1330334083 ad.prog-id=128394",
			L"",
			" ",
			L"",
			{
				{"ad.prog-id", "128394"},
				{"ad.prog-id", "128394"},
				{"ad.prog-id", "128394"},
				{"deviceExternalId", "1330334083"},
				{"deviceExternalId", "1330334083"},
				{"ad.gid", "0"}
			},{}
		}),
		/**
		 * Разбор записей широкой строкой: значение из нескольких слов и пустое значение
		 */
		FmkKVTestParameter({
			"",
			L"cs3= cs3Label=CVEID rt=Feb 17 2023 YEKT src=172.16.0.4",
			"",
			L" ",
			{},{
				{L"cs3", L""},
				{L"cs3Label", L"CVEID"},
				{L"rt", L"Feb 17 2023 YEKT"},
				{L"src", L"172.16.0.4"}
			}
		})
	)
);

/**
 * @brief Структура параметров тестирования метода извлечения URL из текста
 *
 */
struct FmkUrlsTestParameter {
	// Текст для извлечения URL
	std::string text = "";
	// Ожидаемый результат извлечения URL
	std::unordered_map <size_t, size_t> map;
};

/**
 * @brief Класс параметризованного теста для метода извлечения URL из текста
 *
 */
class FmkUrlsParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkUrlsTestParameter> {
	public:
		FmkUrlsTestParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования метода извлечения URL из текста
 *
 */
TEST_P(FmkUrlsParameterizedFixture, FmkUrlsTest){
	// Проверяем, что результат извлечения URL совпадает с ожидаемым
	ASSERT_EQ(this->_parameter.map, this->_fmk->urls(this->_parameter.text));
}

/**
 * @brief Инициализация параметров тестирования метода извлечения URL из текста
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, FmkUrlsParameterizedFixture,
	::testing::Values(
		FmkUrlsTestParameter({"The address of our site https://www.ddd.com:8080/?id=test&key=hash#is_not_work is not working. You can contact us at another address http://www.example.ru/", {{24,78},{133,155}}}),
		FmkUrlsTestParameter({"По адресу сайта http://пангеорадар.рф/?id=test&key=hash вы можете прочитать подробнее. По адресу http://пангеорадар.ру/ вы можете заказать наш продукт.", {{29,81},{157,192}}}),
		FmkUrlsTestParameter({"Bad token foo.zzz must be skipped, but https://anyks.com/page is valid", {{39,61}}})
	)
);

/**
 * @brief Структура параметров тестирования метода преобразования байт в человекочитаемый формат и обратно
 *
 */
struct FmkBytesTestParameter {
	// Число байт
	double number = 0.;
	// Человекочитаемый формат
	std::string word = "";
};

/**
 * @brief Класс параметризованного теста для метода преобразования байт в человекочитаемый формат и обратно
 *
 */
class FmkBytesParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkBytesTestParameter> {
	public:
		FmkBytesTestParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования метода преобразования байт в человекочитаемый формат и обратно
 *
 */
TEST_P(FmkBytesParameterizedFixture, FmkBytesTest){
	// Проверяем, что результат преобразования байт в человекочитаемый формат и обратно совпадает с ожидаемым
	ASSERT_EQ(this->_parameter.number, this->_fmk->bytes(this->_parameter.word));
	// Проверяем обратное преобразование человекочитаемого формата в байты
	ASSERT_EQ(this->_parameter.word, this->_fmk->bytes(this->_parameter.number));
}

/**
 * @brief Инициализация параметров тестирования метода преобразования байт в человекочитаемый формат и обратно
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, FmkBytesParameterizedFixture,
	::testing::Values(
		FmkBytesTestParameter({107374182400., "100 Gb"}),
		FmkBytesTestParameter({20480., "20 Kb"}),
		FmkBytesTestParameter({20971520., "20 Mb"}),
		FmkBytesTestParameter({37580963840., "35 Gb"}),
		FmkBytesTestParameter({16492674416640., "15 Tb"})
	)
);

/**
 * @brief Структура параметров тестирования метода преобразования человекочитаемого формата в байты
 *
 */
struct FmkSizeBufferTestParameter {
	// Ожидаемый результат для размера буфера
	size_t result1 = 0;
	// Ожидаемый результат для скорости передачи данных
	size_t result2 = 0;
	// Человекочитаемый формат
	std::string str = "";
};

/**
 * @brief Класс параметризованного теста для метода преобразования человекочитаемого формата в байты
 *
 */
class FmkSizeBufferParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkSizeBufferTestParameter> {
	public:
		FmkSizeBufferTestParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования метода преобразования человекочитаемого формата в байты
 *
 */
TEST_P(FmkSizeBufferParameterizedFixture, FmkSizeBufferTest){
	// Проверяем, что результат преобразования человекочитаемого формата в байты совпадает с ожидаемым
	ASSERT_EQ(this->_parameter.result1, this->_fmk->bpsBuffer(this->_parameter.str));
}

/**
 * @brief Метод тестирования метода преобразования человекочитаемого формата в байты
 *
 */
TEST_P(FmkSizeBufferParameterizedFixture, FmkBytesPerSecondTest){
	// Проверяем, что результат преобразования человекочитаемого формата в байты совпадает с ожидаемым
	ASSERT_EQ(this->_parameter.result2, this->_fmk->bpsSize(this->_parameter.str));
}

/**
 * @brief Инициализация параметров тестирования метода преобразования человекочитаемого формата в байты
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, FmkSizeBufferParameterizedFixture,
	::testing::Values(
		FmkSizeBufferTestParameter({5, 128, "1024 bps"}),
		FmkSizeBufferTestParameter({1024, 25000, "200 kbps"}),
		FmkSizeBufferTestParameter({768000, 18750000, "150 Mbps"}),
		FmkSizeBufferTestParameter({56320000, 1375000064, "11 Gbps"}),
		FmkSizeBufferTestParameter({2000000000, 49999998976, "400Gbps"})
	)
);
