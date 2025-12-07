/**
 * @file: parameterized.cpp
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
 * Подключаем заголовочный файл проекта
 */
#include "fmk.hpp"

/*
struct FmkFindInMapTestParameter {
	uint32_t key = 0;
	uint32_t val = 0;
	std::map <uint32_t, uint32_t> map = {{1,15},{22,45},{32,88},{84,95}};
};

class FmkFindInMapParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkFindInMapTestParameter> {
	public:
		FmkFindInMapTestParameter _parameter = GetParam();
};

TEST_P(FmkFindInMapParameterizedFixture, FmkFindInMapTest){

	auto i = this->_fmk->findInMap(this->_parameter.val, this->_parameter.map);

	ASSERT_TRUE(i != this->_parameter.map.end());
	
	ASSERT_EQ(this->_parameter.key, i->first);
}

INSTANTIATE_TEST_SUITE_P(TestParameters, FmkFindInMapParameterizedFixture,
	::testing::Values(
		FmkFindInMapTestParameter({1,15}),
		FmkFindInMapTestParameter({22,45}),
		FmkFindInMapTestParameter({32,88}),
		FmkFindInMapTestParameter({84,95})
	)
);

struct FmkIsTestParameter {
	char letter1 = 0;
	wchar_t letter2 = 0;
	std::string text1 = "";
	std::wstring text2 = L"";
	awh::fmk_t::check_t flag = awh::fmk_t::check_t::NONE;
};

class FmkIsParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkIsTestParameter> {
	public:
		FmkIsTestParameter _parameter = GetParam();
};

TEST_P(FmkIsParameterizedFixture, FmkIsLetter1Test){
	switch(static_cast <uint8_t> (this->_parameter.flag)){
		case static_cast <uint8_t> (awh::fmk_t::check_t::URL):
			ASSERT_TRUE(true);
		break;
		case static_cast <uint8_t> (awh::fmk_t::check_t::DECIMAL):
		case static_cast <uint8_t> (awh::fmk_t::check_t::PSEUDO_NUMBER):
			ASSERT_TRUE(this->_fmk->is(this->_parameter.letter1, awh::fmk_t::check_t::NUMBER));
		break;
		case static_cast <uint8_t> (awh::fmk_t::check_t::PRESENCE_LATIAN):
			ASSERT_TRUE(this->_fmk->is(this->_parameter.letter1, awh::fmk_t::check_t::LATIAN));
		break;
		default:
			ASSERT_TRUE(this->_fmk->is(this->_parameter.letter1, this->_parameter.flag));
	}
}

TEST_P(FmkIsParameterizedFixture, FmkIsLetter2Test){
	switch(static_cast <uint8_t> (this->_parameter.flag)){
		case static_cast <uint8_t> (awh::fmk_t::check_t::URL):
			ASSERT_TRUE(true);
		break;
		case static_cast <uint8_t> (awh::fmk_t::check_t::DECIMAL):
		case static_cast <uint8_t> (awh::fmk_t::check_t::PSEUDO_NUMBER):
			ASSERT_TRUE(this->_fmk->is(this->_parameter.letter2, awh::fmk_t::check_t::NUMBER));
		break;
		case static_cast <uint8_t> (awh::fmk_t::check_t::PRESENCE_LATIAN):
			ASSERT_TRUE(this->_fmk->is(this->_parameter.letter2, awh::fmk_t::check_t::LATIAN));
		break;
		default:
			ASSERT_TRUE(this->_fmk->is(this->_parameter.letter2, this->_parameter.flag));
	}
}

TEST_P(FmkIsParameterizedFixture, FmkIsText1Test){
	ASSERT_TRUE(this->_fmk->is(this->_parameter.text1, this->_parameter.flag));
}

TEST_P(FmkIsParameterizedFixture, FmkIsText2Test){
	ASSERT_TRUE(this->_fmk->is(this->_parameter.text2, this->_parameter.flag));
}

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

struct FmkCompareTestParameter {
	std::string forst1 = "";
	std::string second1 = "";
	std::wstring forst2 = L"";
	std::wstring second2 = L"";
};

class FmkCompareParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkCompareTestParameter> {
	public:
		FmkCompareTestParameter _parameter = GetParam();
};

TEST_P(FmkCompareParameterizedFixture, FmkCompare1Test){
	ASSERT_TRUE(this->_fmk->compare(this->_parameter.forst1, this->_parameter.second1));
}

TEST_P(FmkCompareParameterizedFixture, FmkCompare2Test){
	ASSERT_TRUE(this->_fmk->compare(this->_parameter.forst2, this->_parameter.second2));
}

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

struct FmkTimestampTestParameter {
	awh::fmk_t::stamp_t stamp = awh::fmk_t::stamp_t::NONE;
};

class FmkTimestampParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkTimestampTestParameter> {
	public:
		FmkTimestampTestParameter _parameter = GetParam();
};

TEST_P(FmkTimestampParameterizedFixture, FmkTimestampTest){
	ASSERT_TRUE(this->_fmk->timestamp(this->_parameter.stamp) > 0);
}

INSTANTIATE_TEST_SUITE_P(TestParameters, FmkTimestampParameterizedFixture,
	::testing::Values(
		// FmkTimestampTestParameter({awh::fmk_t::stamp_t::YEARS}),
		// FmkTimestampTestParameter({awh::fmk_t::stamp_t::MONTHS}),
		// FmkTimestampTestParameter({awh::fmk_t::stamp_t::WEEKS}),
		// FmkTimestampTestParameter({awh::fmk_t::stamp_t::DAYS}),
		FmkTimestampTestParameter({awh::fmk_t::stamp_t::HOURS}),
		FmkTimestampTestParameter({awh::fmk_t::stamp_t::MINUTES}),
		FmkTimestampTestParameter({awh::fmk_t::stamp_t::SECONDS}),
		FmkTimestampTestParameter({awh::fmk_t::stamp_t::MILLISECONDS}),
		FmkTimestampTestParameter({awh::fmk_t::stamp_t::MICROSECONDS}),
		FmkTimestampTestParameter({awh::fmk_t::stamp_t::NANOSECONDS})
	)
);

struct FmkIconvTestParameter {
	std::string text = "";
};

class FmkIconvParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkIconvTestParameter> {
	public:
		FmkIconvTestParameter _parameter = GetParam();
};

TEST_P(FmkIconvParameterizedFixture, FmkIconvTest){

	const auto & result1 = this->_fmk->iconv(this->_parameter.text, awh::fmk_t::codepage_t::UTF8_CP1251);

	ASSERT_FALSE(this->_fmk->is(result1, awh::fmk_t::check_t::UTF8));

	const auto & result2 = this->_fmk->iconv(result1, awh::fmk_t::codepage_t::CP1251_UTF8);

	ASSERT_TRUE(this->_fmk->is(result2, awh::fmk_t::check_t::UTF8));

	ASSERT_EQ(this->_parameter.text, result2);
}

INSTANTIATE_TEST_SUITE_P(TestParameters, FmkIconvParameterizedFixture,
	::testing::Values(
		FmkIconvTestParameter({"Привет Мир!!!"}),
		FmkIconvTestParameter({"ООО Пангео Радар"})
	)
);

struct FmkTransformTestParameter {
	char letter1 = 0;
	wchar_t letter2 = 0;
	std::string text1 = "";
	std::wstring text2 = L"";
	std::string result1 = "";
	std::wstring result2 = L"";
	awh::fmk_t::transform_t flag = awh::fmk_t::transform_t::NONE;
};

class FmkTransformParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkTransformTestParameter> {
	public:
		FmkTransformTestParameter _parameter = GetParam();
};

TEST_P(FmkTransformParameterizedFixture, FmkTransformLetter1Test){
	switch(static_cast <uint8_t> (this->_parameter.flag)){
		case static_cast <uint8_t> (awh::fmk_t::transform_t::TRIM):
			ASSERT_EQ(this->_parameter.result1.front(), this->_parameter.letter1);
		break;
		case static_cast <uint8_t> (awh::fmk_t::transform_t::SMART):
			ASSERT_EQ(this->_parameter.result1.front(), this->_fmk->transform(this->_parameter.letter1, awh::fmk_t::transform_t::UPPER));
		break;
		default:
			ASSERT_EQ(this->_parameter.result1.front(), this->_fmk->transform(this->_parameter.letter1, this->_parameter.flag));
	}
}

TEST_P(FmkTransformParameterizedFixture, FmkTransformLetter2Test){
	switch(static_cast <uint8_t> (this->_parameter.flag)){
		case static_cast <uint8_t> (awh::fmk_t::transform_t::TRIM):
			ASSERT_EQ(this->_parameter.result2.front(), this->_parameter.letter2);
		break;
		case static_cast <uint8_t> (awh::fmk_t::transform_t::SMART):
			ASSERT_EQ(this->_parameter.result2.front(), this->_fmk->transform(this->_parameter.letter2, awh::fmk_t::transform_t::UPPER));
		break;
		default:
			ASSERT_EQ(this->_parameter.result2.front(), this->_fmk->transform(this->_parameter.letter2, this->_parameter.flag));
	}
}

TEST_P(FmkTransformParameterizedFixture, FmkTransformText1Test){
	ASSERT_EQ(this->_parameter.result1, this->_fmk->transform(this->_parameter.text1, this->_parameter.flag));
}

TEST_P(FmkTransformParameterizedFixture, FmkTransformText2Test){
	ASSERT_EQ(this->_parameter.result2, this->_fmk->transform(this->_parameter.text2, this->_parameter.flag));
}

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
			awh::fmk_t::transform_t::UPPER
		}),
		FmkTransformTestParameter({
			'H',
			L'П',
			"Hello World!!!",
			L"Привет Мир!!!",
			"hello world!!!",
			L"привет мир!!!",
			awh::fmk_t::transform_t::LOWER
		}),
		FmkTransformTestParameter({
			'H',
			L'П',
			"hello world!!!",
			L"ПРИВЕТ МИР!!!",
			"Hello World!!!",
			L"Привет Мир!!!",
			awh::fmk_t::transform_t::SMART
		})
	)
);

struct FmkJoinTestParameter {
	std::string delim1 = "";
	std::wstring delim2 = L"";
	std::string result1 = "";
	std::wstring result2 = L"";
	std::vector <std::string> items1;
	std::vector <std::wstring> items2;
};

class FmkJoinParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkJoinTestParameter> {
	public:
		FmkJoinTestParameter _parameter = GetParam();
};

TEST_P(FmkJoinParameterizedFixture, FmkJoin1Test){
	ASSERT_EQ(this->_parameter.result1, this->_fmk->join(this->_parameter.items1, this->_parameter.delim1));
}

TEST_P(FmkJoinParameterizedFixture, FmkJoin2Test){
	ASSERT_EQ(this->_parameter.result2, this->_fmk->join(this->_parameter.items2, this->_parameter.delim2));
}

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

struct FmkSplitTestParameter {
	std::string delim1 = "";
	std::wstring delim2 = L"";
	std::string text1 = "";
	std::wstring text2 = L"";
	std::vector <std::string> result1;
	std::vector <std::wstring> result2;
};

class FmkSplitParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkSplitTestParameter> {
	public:
		FmkSplitTestParameter _parameter = GetParam();
};

TEST_P(FmkSplitParameterizedFixture, FmkSplit1Test){
	std::vector <std::string> container;
	ASSERT_EQ(this->_parameter.result1, this->_fmk->split(this->_parameter.text1, this->_parameter.delim1, container));
}

TEST_P(FmkSplitParameterizedFixture, FmkSplit2Test){
	std::vector <std::wstring> container;
	ASSERT_EQ(this->_parameter.result2, this->_fmk->split(this->_parameter.text2, this->_parameter.delim2, container));
}

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

struct FmkConvertTestParameter {
	std::string text1 = "";
	std::wstring text2 = L"";
};

class FmkConvertParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkConvertTestParameter> {
	public:
		FmkConvertTestParameter _parameter = GetParam();
};

TEST_P(FmkConvertParameterizedFixture, FmkConvertTest){
	ASSERT_EQ(this->_parameter.text2, this->_fmk->convert(this->_parameter.text1));
	ASSERT_EQ(this->_parameter.text1, this->_fmk->convert(this->_parameter.text2));
}

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

struct FmkSizeTestParameter {
	size_t size = 0;
	uint64_t num = 0;
	const char * data = nullptr;
};

class FmkSizeParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkSizeTestParameter> {
	public:
		FmkSizeTestParameter _parameter = GetParam();
};

TEST_P(FmkSizeParameterizedFixture, FmkSizeTest){
	if(this->_parameter.data == nullptr)
		ASSERT_EQ(this->_parameter.size, this->_fmk->size(this->_parameter.num));
	else ASSERT_EQ(static_cast <size_t> (this->_parameter.num), this->_fmk->size(this->_parameter.data, this->_parameter.size));
}

INSTANTIATE_TEST_SUITE_P(TestParameters, FmkSizeParameterizedFixture,
	::testing::Values(
		FmkSizeTestParameter({8, 18446744073709551615}),
		FmkSizeTestParameter({4, 4294967295}),
		FmkSizeTestParameter({2, 65535}),
		FmkSizeTestParameter({1, 255}),
		FmkSizeTestParameter({14, 14, "Hello World!!!"})
	)
);

struct FmkGreaterTestParameter {
	uint64_t num1 = 0;
	uint64_t num2 = 0;
	std::vector <uint64_t> data1;
	std::vector <uint64_t> data2;
};

class FmkGreaterParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkGreaterTestParameter> {
	public:
		FmkGreaterTestParameter _parameter = GetParam();
};

TEST_P(FmkGreaterParameterizedFixture, FmkGreaterTest){
	if(this->_parameter.data1.empty() && this->_parameter.data2.empty())
		ASSERT_TRUE(this->_fmk->greater(this->_parameter.num1, this->_parameter.num2));
	else ASSERT_TRUE(this->_fmk->greater(this->_parameter.data1.data(), this->_parameter.data2.data(), this->_parameter.data2.size() * sizeof(uint64_t)));
}

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

struct FmkItoaTestParameter {
	uint8_t radix = 0;
	uint32_t value = 0;
	std::string text = "";
	std::string result = "";
};

class FmkItoaParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkItoaTestParameter> {
	public:
		FmkItoaTestParameter _parameter = GetParam();
};

TEST_P(FmkItoaParameterizedFixture, FmkItoaTest){
	if(this->_parameter.text.empty())
		ASSERT_EQ(this->_parameter.result, this->_fmk->itoa(this->_parameter.value, this->_parameter.radix));
	else ASSERT_EQ(this->_parameter.result, this->_fmk->itoa(this->_parameter.text.c_str(), this->_parameter.text.length(), 2));
}

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

struct FmkAtoiTestParameter {
	uint8_t radix = 0;
	uint32_t result = 0;
	std::string value = "";
	std::string text = "";
};

class FmkAtoiParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkAtoiTestParameter> {
	public:
		FmkAtoiTestParameter _parameter = GetParam();
};

TEST_P(FmkAtoiParameterizedFixture, FmkAtoiTest){
	if(this->_parameter.text.empty())
		ASSERT_EQ(this->_parameter.result, this->_fmk->atoi <uint32_t> (this->_parameter.value, this->_parameter.radix));
	else {
		size_t size = 0;
		const size_t count = (this->_parameter.value.length() % 8);
		if(count == 0)
			size = (this->_parameter.value.length() / 8);
		else size = ((this->_parameter.value.length() + (8 - count)) / 8);

		std::string result(size, 0);

		this->_fmk->atoi(this->_parameter.value, 2, result.data(), result.size());
		
		ASSERT_EQ(result, this->_parameter.text);
	}
}

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

struct FmkNoexpTestParameter {
	uint8_t step = 0;
	double value = 0.;
	bool onlyNum = false;
	std::string result = "";
};

class FmkNoexpParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkNoexpTestParameter> {
	public:
		FmkNoexpTestParameter _parameter = GetParam();
};

TEST_P(FmkNoexpParameterizedFixture, FmkNoexpTest){
	
	this->_fmk->setLocale();

	if(this->_parameter.step > 0)
		ASSERT_EQ(this->_parameter.result, this->_fmk->noexp(this->_parameter.value, this->_parameter.step));
	else ASSERT_EQ(this->_parameter.result, this->_fmk->noexp(this->_parameter.value, this->_parameter.onlyNum));
}

INSTANTIATE_TEST_SUITE_P(TestParameters, FmkNoexpParameterizedFixture,
	::testing::Values(
		FmkNoexpTestParameter({3, 2986.808299, false, "2986.808"}),
		FmkNoexpTestParameter({4, 2986.808299, false, "2986.8083"}),
		FmkNoexpTestParameter({0, 1000000000.808299, false, "1000000000.808299"}),
		FmkNoexpTestParameter({0, 1000000000.808299, true, "1000000000.808299"}),
		FmkNoexpTestParameter({0, 0.80829989222211145, true, "0.8083"}),
		FmkNoexpTestParameter({0, 1e+19, false, "10000000000000000000"})
	)
);

struct FmkRateTestParameter {
	float num1 = 0.f;
	float num2 = 0.f;
	float result = 0.f;
};

class FmkRateParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkRateTestParameter> {
	public:
		FmkRateTestParameter _parameter = GetParam();
};

TEST_P(FmkRateParameterizedFixture, FmkRateTest){
	ASSERT_EQ(static_cast <int32_t> (this->_parameter.result), static_cast <int32_t> (this->_fmk->rate(this->_parameter.num1, this->_parameter.num2)));
}

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

struct FmkFloorTestParameter {
	uint8_t count = 0;
	double num = 0.;
	double result = 0.;
};

class FmkFloorParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkFloorTestParameter> {
	public:
		FmkFloorTestParameter _parameter = GetParam();
};

TEST_P(FmkFloorParameterizedFixture, FmkFloorTest){
	ASSERT_EQ(this->_parameter.result, this->_fmk->floor(this->_parameter.num, this->_parameter.count));
}

INSTANTIATE_TEST_SUITE_P(TestParameters, FmkFloorParameterizedFixture,
	::testing::Values(
		FmkFloorTestParameter({3, 38963.892549, 38963.892}),
		FmkFloorTestParameter({4, 38963.892549, 38963.8925}),
		FmkFloorTestParameter({5, 38963.892549, 38963.89254}),
		FmkFloorTestParameter({6, 38963.892549, 38963.892549})
	)
);

struct FmkRome2arabicTestParameter {
	uint16_t result = 0;
	std::string num1 = "";
	std::wstring num2 = L"";
};

class FmkRome2arabicParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkRome2arabicTestParameter> {
	public:
		FmkRome2arabicTestParameter _parameter = GetParam();
};

TEST_P(FmkRome2arabicParameterizedFixture, FmkRome2arabicTest){
	if(!this->_parameter.num1.empty())
		ASSERT_EQ(this->_parameter.result, this->_fmk->rome2arabic(this->_parameter.num1));
	else if(!this->_parameter.num2.empty())
		ASSERT_EQ(this->_parameter.result, this->_fmk->rome2arabic(this->_parameter.num2));
	else ASSERT_TRUE(false);
}

INSTANTIATE_TEST_SUITE_P(TestParameters, FmkRome2arabicParameterizedFixture,
	::testing::Values(
		FmkRome2arabicTestParameter({3874, "MMMDCCCLXXIV"}),
		FmkRome2arabicTestParameter({3874, "", L"MMMDCCCLXXIV"}),
		FmkRome2arabicTestParameter({2025, "MMXXV"}),
		FmkRome2arabicTestParameter({2025, "", L"MMXXV"})
	)
);

struct FmkArabic2romeTestParameter {
	uint32_t number = 0;
	std::string word1 = "";
	std::wstring word2 = L"";
	std::string result1 = "";
	std::wstring result2 = L"";
};

class FmkArabic2romeParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkArabic2romeTestParameter> {
	public:
		FmkArabic2romeTestParameter _parameter = GetParam();
};

TEST_P(FmkArabic2romeParameterizedFixture, FmkArabic2romeTest){
	if(this->_parameter.number > 0)
		ASSERT_EQ(this->_parameter.result2, this->_fmk->arabic2rome(this->_parameter.number));
	else if(!this->_parameter.word1.empty())
		ASSERT_EQ(this->_parameter.result1, this->_fmk->arabic2rome(this->_parameter.word1));
	else if(!this->_parameter.word2.empty())
		ASSERT_EQ(this->_parameter.result2, this->_fmk->arabic2rome(this->_parameter.word2));
	else ASSERT_TRUE(false);
}

INSTANTIATE_TEST_SUITE_P(TestParameters, FmkArabic2romeParameterizedFixture,
	::testing::Values(
		FmkArabic2romeTestParameter({3874, "", L"", "", L"MMMDCCCLXXIV"}),
		FmkArabic2romeTestParameter({0, "3874", L"", "MMMDCCCLXXIV"}),
		FmkArabic2romeTestParameter({0, "", L"2025", "", L"MMXXV"})
	)
);

struct FmkCountLetterTestParameter {
	size_t result = 0;
	wchar_t letter = 0;
	std::wstring word = L"";
};

class FmkCountLetterParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkCountLetterTestParameter> {
	public:
		FmkCountLetterTestParameter _parameter = GetParam();
};

TEST_P(FmkCountLetterParameterizedFixture, FmkCountLetterTest){
	ASSERT_EQ(this->_parameter.result, this->_fmk->countLetter(this->_parameter.word, this->_parameter.letter));
}

INSTANTIATE_TEST_SUITE_P(TestParameters, FmkCountLetterParameterizedFixture,
	::testing::Values(
		FmkCountLetterTestParameter({3, L'т', L"Привет этот мир!!!"}),
		FmkCountLetterTestParameter({3, L'l', L"Hello World!!!"}),
		FmkCountLetterTestParameter({2, L'o', L"Hello World!!!"})
	)
);

struct FmkFormatTestParameter {
	std::string result = "";
	std::string format = "";
	std::vector <std::string> items;
};

class FmkFormatParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkFormatTestParameter> {
	public:
		FmkFormatTestParameter _parameter = GetParam();
};

TEST_P(FmkFormatParameterizedFixture, FmkFormatTest){
	ASSERT_EQ(this->_parameter.result, this->_fmk->format(this->_parameter.format, this->_parameter.items));
}

INSTANTIATE_TEST_SUITE_P(TestParameters, FmkFormatParameterizedFixture,
	::testing::Values(
		FmkFormatTestParameter({"Hello World!!!", "$1 $2!!!", {"Hello", "World"}}),
		FmkFormatTestParameter({"Вашм присвоен идентификатор ID=984 и ID=586", "$1 ID=$2 и ID=$3", {"Вашм присвоен идентификатор", "984", "586"}})
	)
);

struct FmkExistsTestParameter {
	std::string word1 = "";
	std::string text1 = "";
	std::wstring word2 = L"";
	std::wstring text2 = L"";
};

class FmkExistsParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkExistsTestParameter> {
	public:
		FmkExistsTestParameter _parameter = GetParam();
};

TEST_P(FmkExistsParameterizedFixture, FmkExistsTest){
	if(!this->_parameter.word1.empty() && !this->_parameter.text1.empty())
		ASSERT_TRUE(this->_fmk->exists(this->_parameter.word1, this->_parameter.text1));
	else ASSERT_TRUE(this->_fmk->exists(this->_parameter.word2, this->_parameter.text2));
}

INSTANTIATE_TEST_SUITE_P(TestParameters, FmkExistsParameterizedFixture,
	::testing::Values(
		FmkExistsTestParameter({"Wor", "Hello World!!!"}),
		FmkExistsTestParameter({"", "", L"вет", L"Привет Мир!!!"})
	)
);

struct FmkReplaceTestParameter {
	std::string text1 = "";
	std::string word1 = "";
	std::string alt1 = "";
	std::string result1 = "";
	std::wstring text2 = L"";
	std::wstring word2 = L"";
	std::wstring alt2 = L"";
	std::wstring result2 = L"";
};

class FmkReplaceParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkReplaceTestParameter> {
	public:
		FmkReplaceTestParameter _parameter = GetParam();
};

TEST_P(FmkReplaceParameterizedFixture, FmkReplaceTest){
	if(!this->_parameter.text1.empty() && !this->_parameter.word1.empty() && !this->_parameter.result1.empty())
		ASSERT_EQ(this->_parameter.result1, this->_fmk->replace(this->_parameter.text1, this->_parameter.word1, this->_parameter.alt1));
	else ASSERT_EQ(this->_fmk->convert(this->_parameter.result2), this->_fmk->convert(this->_fmk->replace(this->_parameter.text2, this->_parameter.word2, this->_parameter.alt2)));
}

INSTANTIATE_TEST_SUITE_P(TestParameters, FmkReplaceParameterizedFixture,
	::testing::Values(
		FmkReplaceTestParameter({"Hello World!!!", "o W", "o and W", "Hello and World!!!"}),
		FmkReplaceTestParameter({"", "", "", "", L"Привет Мир!!!", L"ивет Мир", L"ивет этот Мир", L"Привет этот Мир!!!"})
	)
);

struct FmkUrlsTestParameter {
	std::string text;
	std::map <size_t, size_t> map;
};

class FmkUrlsParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkUrlsTestParameter> {
	public:
		FmkUrlsTestParameter _parameter = GetParam();
};

TEST_P(FmkUrlsParameterizedFixture, FmkUrlsTest){
	ASSERT_EQ(this->_parameter.map, this->_fmk->urls(this->_parameter.text));
}

INSTANTIATE_TEST_SUITE_P(TestParameters, FmkUrlsParameterizedFixture,
	::testing::Values(
		FmkUrlsTestParameter({"The address of our site https://www.ddd.com:8080/?id=test&key=hash#is_not_work is not working. You can contact us at another address http://www.example.ru/", {{24,78},{133,155}}}),
		FmkUrlsTestParameter({"По адресу сайта http://пангеорадар.рф/?id=test&key=hash вы можете прочитать подробнее. По адресу http://пангеорадар.ру/ вы можете заказать наш продукт.", {{29,81},{157,192}}})
	)
);

struct FmkBytesTestParameter {
	double number = 0.;
	std::string word = "";
};

class FmkBytesParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkBytesTestParameter> {
	public:
		FmkBytesTestParameter _parameter = GetParam();
};

TEST_P(FmkBytesParameterizedFixture, FmkBytesTest){
	ASSERT_EQ(this->_parameter.number, this->_fmk->bytes(this->_parameter.word));
	ASSERT_EQ(this->_parameter.word, this->_fmk->bytes(this->_parameter.number));
}

INSTANTIATE_TEST_SUITE_P(TestParameters, FmkBytesParameterizedFixture,
	::testing::Values(
		FmkBytesTestParameter({107374182400., "100 Gb"}),
		FmkBytesTestParameter({20480., "20 Kb"}),
		FmkBytesTestParameter({20971520., "20 Mb"}),
		FmkBytesTestParameter({37580963840., "35 Gb"}),
		FmkBytesTestParameter({16492674416640., "15 Tb"})
	)
);

struct FmkSecondsTestParameter {
	time_t seconds = 0;
	std::string str = "";
};

class FmkSecondsParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkSecondsTestParameter> {
	public:
		FmkSecondsTestParameter _parameter = GetParam();
};

TEST_P(FmkSecondsParameterizedFixture, FmkSecondsTest){
	ASSERT_EQ(this->_parameter.seconds, this->_fmk->seconds(this->_parameter.str));
	ASSERT_EQ(this->_parameter.str, this->_fmk->seconds(this->_parameter.seconds));
}

INSTANTIATE_TEST_SUITE_P(TestParameters, FmkSecondsParameterizedFixture,
	::testing::Values(
		FmkSecondsTestParameter({1020, "17m"}),
		FmkSecondsTestParameter({86400, "1d"}),
		FmkSecondsTestParameter({8672400, "3.3M"}),
		FmkSecondsTestParameter({88300800, "2.8y"}),
		FmkSecondsTestParameter({94608000, "3y"})
	)
);

struct FmkSizeBufferTestParameter {
	size_t result = 0;
	std::string str = "";
};

class FmkSizeBufferParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkSizeBufferTestParameter> {
	public:
		FmkSizeBufferTestParameter _parameter = GetParam();
};

TEST_P(FmkSizeBufferParameterizedFixture, FmkSizeBufferTest){
	ASSERT_EQ(this->_parameter.result, this->_fmk->sizeBuffer(this->_parameter.str));
}

INSTANTIATE_TEST_SUITE_P(TestParameters, FmkSizeBufferParameterizedFixture,
	::testing::Values(
		FmkSizeBufferTestParameter({5, "1024 bps"}),
		FmkSizeBufferTestParameter({1024, "200 kbps"}),
		FmkSizeBufferTestParameter({768000, "150 Mbps"}),
		FmkSizeBufferTestParameter({56320000, "11 Gbps"})
	)
);

struct FmkTime2abbrTestParameter {
	time_t date = 0;
	std::string result = "";
};

class FmkTime2abbrParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkTime2abbrTestParameter> {
	public:
		FmkTime2abbrTestParameter _parameter = GetParam();
};

TEST_P(FmkTime2abbrParameterizedFixture, FmkTime2abbrTest){
	ASSERT_EQ(this->_parameter.result, this->_fmk->time2abbr(this->_parameter.date));
}

INSTANTIATE_TEST_SUITE_P(TestParameters, FmkTime2abbrParameterizedFixture,
	::testing::Values(
		FmkTime2abbrTestParameter({582892, "9.7 min"}),
		FmkTime2abbrTestParameter({47799, "47.8 sec"}),
		FmkTime2abbrTestParameter({2589, "2.6 sec"}),
		FmkTime2abbrTestParameter({10844922, "3.0 hour"})
	)
);

struct FmkTime2strTestParameter {
	time_t date = 0;
	std::string format;
	std::string result;
};

class FmkTime2strParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkTime2strTestParameter> {
	public:
		FmkTime2strTestParameter _parameter = GetParam();
};

TEST_P(FmkTime2strParameterizedFixture, FmkTime2strTest){
	ASSERT_EQ(this->_parameter.result, this->_fmk->time2str(this->_parameter.date, this->_parameter.format));
}

INSTANTIATE_TEST_SUITE_P(TestParameters, FmkTime2strParameterizedFixture,
	::testing::Values(
		FmkTime2strTestParameter({1743943021, "%Y-%m-%dT%H:%M:%S%z", "2025-04-06T15:37:01+0300"}),
		FmkTime2strTestParameter({1743943021, "%m/%d/%Y %I:%M:%S %p", "04/06/2025 03:37:01 PM"}),
		FmkTime2strTestParameter({1743943021, "%m/%d/%y %I:%M:%S %p", "04/06/25 03:37:01 PM"}),
		FmkTime2strTestParameter({1743943021, "%Y-%m-%dT%H:%M:%S", "2025-04-06T15:37:01"}),
		FmkTime2strTestParameter({1743943021, "%m/%d/%Y %I:%M:%S %p", "04/06/2025 03:37:01 PM"}),
		FmkTime2strTestParameter({1743943021, "%a %h %d %H:%M:%S %Y", "Sun Apr 06 15:37:01 2025"}),
		FmkTime2strTestParameter({1743943021, "%d/%h/%Y:%H:%M:%S %z", "06/Apr/2025:15:37:01 +0300"}),
		FmkTime2strTestParameter({1743943021, "%h %d %H:%M:%S", "Apr 06 15:37:01"}),
		FmkTime2strTestParameter({1743943021, "%Y-%m-%d %H:%M:%S", "2025-04-06 15:37:01"}),
		FmkTime2strTestParameter({1743943021, "%d/%h/%Y:%H:%M:%S %z", "06/Apr/2025:15:37:01 +0300"}),
		FmkTime2strTestParameter({1743943021, "%Y/%m/%d %H:%M:%S", "2025/04/06 15:37:01"}),
		FmkTime2strTestParameter({1743943021, "%d.%m.%Y %H:%M:%S", "06.04.2025 15:37:01"}),
		FmkTime2strTestParameter({1743943021, "%m-%d %H:%M:%S", "04-06 15:37:01"}),
		FmkTime2strTestParameter({1743943021, "%H:%M:%S", "15:37:01"}),
		FmkTime2strTestParameter({1743943021, "%a %h %e %Y %H:%M:%S %z", "Sun Apr  6 2025 15:37:01 +0300"}),
		FmkTime2strTestParameter({1743943021, "%a %h %e %H:%M:%S %z", "Sun Apr  6 15:37:01 +0300"}),
		FmkTime2strTestParameter({1743943021, "%a %h %e %Y %H:%M:%S %Z%z", "Sun Apr  6 2025 15:37:01 MSK+0300"}),
		FmkTime2strTestParameter({1743943021, "%h %d %H:%M %Z%z", "Apr 06 15:37 MSK+0300"}),
		FmkTime2strTestParameter({1743943021, "%a %h %e %H:%M:%S %W %z %j", "Sun Apr  6 15:37:01 13 +0300 096"}),
		FmkTime2strTestParameter({1743943021, "%Y%m%dT%H%M%S%z", "20250406T153701+0300"})
	)
);

struct FmkStr2timeTestParameter {
	std::string date;
	std::string format;
	time_t result = 0;
};

class FmkStr2timeParameterizedFixture : public FmkFixture, public ::testing::WithParamInterface <FmkStr2timeTestParameter> {
	public:
		FmkStr2timeTestParameter _parameter = GetParam();
};

TEST_P(FmkStr2timeParameterizedFixture, FmkStr2timeTest){
	ASSERT_EQ(this->_parameter.result, this->_fmk->str2time(this->_parameter.date, this->_parameter.format));
}

INSTANTIATE_TEST_SUITE_P(TestParameters, FmkStr2timeParameterizedFixture,
	::testing::Values(
		FmkStr2timeTestParameter({"2023-03-05T12:55:58.0490925Z", "%Y-%m-%dT%H:%M:%S.%s%Z", 1678010158}),
		FmkStr2timeTestParameter({"2024-08-06T11:08:55Z", "%Y-%m-%dT%H:%M:%S%Z", 1722931735}),
		FmkStr2timeTestParameter({"2024-08-06T14:47:34+0300", "%Y-%m-%dT%H:%M:%S%z", 1722944854}),
		FmkStr2timeTestParameter({"7/26/2023 2:39:42 PM", "%m/%d/%Y %I:%M:%S %p", 1690371582}),
		FmkStr2timeTestParameter({"2023-07-26T14:39:4", "%Y-%m-%dT%H:%M:%S", 1690371544}),
		FmkStr2timeTestParameter({"7/26/2023 2:39:42 PM", "%m/%d/%Y %I:%M:%S %p", 1690371582}),
		FmkStr2timeTestParameter({"2024-11-15 17:14:03", "%Y-%m-%d %H:%M:%S", 1731680043}),
		FmkStr2timeTestParameter({"Tue Jul 16 10:45:40 2024", "%a %h %d %H:%M:%S %Y", 1721115940}),
		FmkStr2timeTestParameter({"05/Apr/2023:12:45:12 +0300", "%d/%h/%Y:%H:%M:%S %z", 1680687912}),
		FmkStr2timeTestParameter({"2024-10-16 10:30:45", "%Y-%m-%d %H:%M:%S", 1729063845}),
		FmkStr2timeTestParameter({"[18/Jul/2024:13:34:00 +0300]", "[%d/%h/%Y:%H:%M:%S %z]", 1721298840}),
		FmkStr2timeTestParameter({"[18/Jul/24:13:34:00 +0300]", "[%d/%h/%y:%H:%M:%S %z]", 1721298840}),
		FmkStr2timeTestParameter({"2024/07/18 13:33:17", "%Y/%m/%d %H:%M:%S", 1721298797}),
		FmkStr2timeTestParameter({"17.07.2023 13:25:53", "%d.%m.%Y %H:%M:%S", 1689589553}),
		FmkStr2timeTestParameter({"Wed Mar 19 2025 15:51:10 GMT+0300", "%a %h %e %Y %H:%M:%S %z", 1742388670}),
		FmkStr2timeTestParameter({"Wed Mar 30 2025 15:51:10 GMT+0300", "%a %h %e %Y %H:%M:%S %Z%z", 1743339070}),
		FmkStr2timeTestParameter({"20050809T183142+0330", "%Y%m%dT%H%M%S%z", 1123601502}),
		FmkStr2timeTestParameter({"Wed Mar 19 2025 15:51:10", "%a %h %e %Y %H:%M:%S %z", 1742388670})
	)
);
*/
