/**
 * @file writer.cpp
 * @date 2026-08-15
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
 * @brief Исходный файл тестов записи чисел в строковое представление —
 *        проверка обратимости и кратчайшести записи чисел с плавающей точкой,
 *        а также записи целых чисел в системах счисления от двоичной до тридцатишестеричной
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл тестовой фикстуры
 */
#include "lexical.hpp"

/**
 * Подключаем стандартные заголовочные файлы
 */
#include <random>

/**
 * Используем пространство имён модуля
 */
using namespace awh;
using namespace awh::lexical;

/**
 * @brief Метод формирования записи числа с плавающей точкой
 *
 * @param value  записываемое число
 * @param format требуемый вид записи числа
 * @return       сформированная запись числа
 *
 */
static string compose(const double value, const format_t format = format_t::GENERAL) noexcept {
	// Хранилище записи числа достаточного размера
	char buffer[maxRecordLength <double> ()];
	// Выполняем запись числа
	const lexical_t::output_t <char> result = lexical_t::toChars(buffer, buffer + sizeof(buffer), value, format);
	// Если запись числа выполнить не удалось
	if(!static_cast <bool> (result))
		// Выводим пустую запись числа
		return "";
	// Выводим сформированную запись числа
	return string(buffer, static_cast <size_t> (result.ptr - buffer));
}

/**
 * @brief Метод разбора записи числа с плавающей точкой
 *
 * @param text разбираемая запись числа
 * @return     разобранное значение числа
 *
 */
static double restore(const string & text) noexcept {
	// Разобранное значение числа
	double result = 0.;
	// Выполняем разбор записи числа
	lexical_t::fromChars(text.data(), text.data() + text.size(), result);
	// Выводим разобранное значение числа
	return result;
}

/**
 * @brief Тест записи числа с плавающей точкой известными значениями
 *
 */
TEST_F(LexicalFixture, WriteFloatKnownTest){
	// Проверяем запись целых значений
	ASSERT_EQ(compose(1.), "1");
	// Проверяем запись отрицательных значений
	ASSERT_EQ(compose(-1.), "-1");
	// Проверяем запись нулевого значения
	ASSERT_EQ(compose(0.), "0");
	// Проверяем запись отрицательного нуля с сохранением знака
	ASSERT_EQ(compose(-0.), "-0");
	// Проверяем запись значения, не представимого точно
	ASSERT_EQ(compose(0.3), "0.3");
	// Проверяем запись значения с дробной частью
	ASSERT_EQ(compose(123.456), "123.456");
	// Проверяем запись половины
	ASSERT_EQ(compose(0.5), "0.5");
	// Проверяем запись наименьшего денормализованного значения
	ASSERT_EQ(compose(5e-324), "5e-324");
	// Проверяем запись наибольшего представимого значения
	ASSERT_EQ(compose(numeric_limits <double>::max()), "1.7976931348623157e+308");
	// Проверяем запись наименьшего нормализованного значения
	ASSERT_EQ(compose(numeric_limits <double>::min()), "2.2250738585072014e-308");
	// Проверяем запись бесконечности
	ASSERT_EQ(compose(numeric_limits <double>::infinity()), "inf");
	// Проверяем запись отрицательной бесконечности
	ASSERT_EQ(compose(-numeric_limits <double>::infinity()), "-inf");
	// Проверяем запись значения, не являющегося числом
	ASSERT_EQ(compose(numeric_limits <double>::quiet_NaN()), "nan");
}

/**
 * @brief Тест выбора вида записи числа с плавающей точкой
 *
 */
TEST_F(LexicalFixture, WriteFloatFormatTest){
	// Проверяем что запись без порядка выбирается при равной длине
	ASSERT_EQ(compose(100.), "100");
	// Проверяем что запись с порядком выбирается когда она короче
	ASSERT_EQ(compose(1e6), "1e+06");
	// Проверяем что вид записи без порядка выбирается принудительно
	ASSERT_EQ(compose(1e6, format_t::FIXED), "1000000");
	// Проверяем что вид записи с порядком выбирается принудительно
	ASSERT_EQ(compose(100., format_t::SCIENTIFIC), "1e+02");
	// Проверяем что запись с порядком содержит три цифры при большом порядке
	ASSERT_EQ(compose(1e300, format_t::SCIENTIFIC), "1e+300");
	// Проверяем запись с дробной частью и порядком
	ASSERT_EQ(compose(1.5e-7, format_t::SCIENTIFIC), "1.5e-07");
	// Проверяем что запись без порядка получает ведущий ноль целой части
	ASSERT_EQ(compose(0.001, format_t::FIXED), "0.001");
	// Проверяем что запись без порядка дописывает незначащие нули целой части
	ASSERT_EQ(compose(1.5e10, format_t::FIXED), "15000000000");
}

/**
 * @brief Тест обратимости записи числа с плавающей точкой
 *
 * @details Запись обязана читаться обратно тем же самым числом вплоть до последнего
 *          бита мантиссы при любом виде записи.
 *
 */
TEST_F(LexicalFixture, WriteFloatRoundTripTest){
	// Создаём источник псевдослучайных чисел
	mt19937_64 gen(20260815);
	/**
	 * Выполняем перебор случайных двоичных образов чисел
	 */
	for(uint32_t i = 0; i < 200000; ++i){
		// Получаем очередной двоичный образ числа
		const uint64_t bits = gen();
		// Значение числа с плавающей точкой
		double value = 0.;
		// Выполняем перенос двоичного образа в число
		::memcpy(&value, &bits, sizeof(value));
		// Если число конечным не является
		if(::isnan(value) || ::isinf(value))
			// Продолжаем перебор дальше
			continue;
		/**
		 * Выполняем перебор всех видов записи числа
		 */
		for(const format_t format : {format_t::GENERAL, format_t::FIXED, format_t::SCIENTIFIC}){
			// Выполняем запись числа
			const string text = compose(value, format);

			// Проверяем что запись числа сформирована
			ASSERT_FALSE(text.empty()) << "value = " << value;
			// Проверяем что запись читается обратно тем же самым числом
			ASSERT_TRUE(this->sameBits(restore(text), value)) << "text = " << text;
		}
	}
}

/**
 * @brief Тест кратчайшести записи числа с плавающей точкой
 *
 * @details Запись обязана содержать наименьшее количество значащих цифр: запись,
 *          укороченная на одну цифру, читаться обратно тем же числом не должна.
 *
 */
TEST_F(LexicalFixture, WriteFloatShortestTest){
	// Создаём источник псевдослучайных чисел
	mt19937_64 gen(31415926);
	/**
	 * Выполняем перебор случайных двоичных образов чисел
	 */
	for(uint32_t i = 0; i < 100000; ++i){
		// Получаем очередной двоичный образ числа
		const uint64_t bits = gen();
		// Значение числа с плавающей точкой
		double value = 0.;
		// Выполняем перенос двоичного образа в число
		::memcpy(&value, &bits, sizeof(value));
		// Если число конечным не является либо является нулевым
		if(::isnan(value) || ::isinf(value) || (value == 0.))
			// Продолжаем перебор дальше
			continue;
		// Выполняем запись числа с порядком
		const string text = compose(value, format_t::SCIENTIFIC);
		// Определяем положение буквы порядка записи
		const size_t position = text.find('e');

		// Проверяем что запись числа содержит букву порядка
		ASSERT_NE(position, string::npos) << "text = " << text;
		// Получаем запись мантиссы числа
		string mantissa = text.substr(0, position);
		// Получаем запись порядка числа
		const string exponent = text.substr(position);
		// Определяем положение десятичной точки записи мантиссы
		const size_t point = mantissa.find('.');
		/**
		 * Если запись мантиссы содержит дробную часть
		 */
		if(point != string::npos){
			// Выполняем удаление младшей цифры записи мантиссы
			mantissa.erase(mantissa.size() - 1);
			// Если от дробной части остался лишь разделитель
			if(mantissa.back() == '.')
				// Выполняем удаление разделителя дробной части
				mantissa.erase(mantissa.size() - 1);
			// Проверяем что укороченная запись читается обратно иным числом
			ASSERT_FALSE(this->sameBits(restore(mantissa + exponent), value))
				<< "text = " << text << ", shorter = " << (mantissa + exponent);
		}
	}
}

/**
 * @brief Тест записи числа с плавающей точкой одинарной точности
 *
 */
TEST_F(LexicalFixture, WriteFloatSingleTest){
	// Хранилище записи числа
	char buffer[64];
	// Создаём источник псевдослучайных чисел
	mt19937_64 gen(2718281);
	/**
	 * Выполняем перебор случайных двоичных образов чисел
	 */
	for(uint32_t i = 0; i < 200000; ++i){
		// Получаем очередной двоичный образ числа
		const uint32_t bits = static_cast <uint32_t> (gen());
		// Значение числа с плавающей точкой
		float value = 0.f;
		// Выполняем перенос двоичного образа в число
		::memcpy(&value, &bits, sizeof(value));
		// Если число конечным не является
		if(::isnan(value) || ::isinf(value))
			// Продолжаем перебор дальше
			continue;
		// Выполняем запись числа
		const lexical_t::output_t <char> result = lexical_t::toChars(buffer, buffer + sizeof(buffer), value);

		// Проверяем что запись числа выполнена
		ASSERT_TRUE(static_cast <bool> (result)) << "value = " << value;
		// Разобранное обратно значение числа
		float back = 0.f;
		// Выполняем разбор записи числа
		lexical_t::fromChars(buffer, result.ptr, back);
		// Проверяем что запись читается обратно тем же самым числом
		ASSERT_EQ(back, value) << "text = " << string(buffer, static_cast <size_t> (result.ptr - buffer));
	}
}

/**
 * @brief Тест записи целых чисел
 *
 */
TEST_F(LexicalFixture, WriteIntegerTest){
	// Хранилище записи числа
	char buffer[64];
	/**
	 * @brief Метод формирования записи целого числа
	 *
	 * @param value записываемое число
	 * @param base  основание системы счисления
	 * @return      сформированная запись числа
	 *
	 */
	auto write = [&](const int64_t value, const int32_t base) noexcept -> string {
		// Выполняем запись целого числа
		const lexical_t::output_t <char> result = lexical_t::toChars(buffer, buffer + sizeof(buffer), value, base);
		// Если запись числа выполнить не удалось
		if(!static_cast <bool> (result))
			// Выводим пустую запись числа
			return "";
		// Выводим сформированную запись числа
		return string(buffer, static_cast <size_t> (result.ptr - buffer));
	};

	// Проверяем запись нулевого значения
	ASSERT_EQ(write(0, 10), "0");
	// Проверяем запись положительного значения
	ASSERT_EQ(write(42, 10), "42");
	// Проверяем запись отрицательного значения
	ASSERT_EQ(write(-42, 10), "-42");
	// Проверяем запись наибольшего представимого значения
	ASSERT_EQ(write(numeric_limits <int64_t>::max(), 10), "9223372036854775807");
	// Проверяем запись наименьшего представимого значения
	ASSERT_EQ(write(numeric_limits <int64_t>::min(), 10), "-9223372036854775808");
	// Проверяем запись в шестнадцатеричной системе счисления
	ASSERT_EQ(write(255, 16), "ff");
	// Проверяем запись в двоичной системе счисления
	ASSERT_EQ(write(255, 2), "11111111");
	// Проверяем запись в восьмеричной системе счисления
	ASSERT_EQ(write(8, 8), "10");
	// Проверяем запись в тридцатишестеричной системе счисления
	ASSERT_EQ(write(35, 36), "z");
	// Проверяем что недопустимое основание системы счисления отвергается
	ASSERT_EQ(write(1, 1), "");
	// Проверяем что недопустимое основание системы счисления отвергается
	ASSERT_EQ(write(1, 37), "");
}

/**
 * @brief Тест обратимости записи целых чисел
 *
 */
TEST_F(LexicalFixture, WriteIntegerRoundTripTest){
	// Хранилище записи числа
	char buffer[64];
	// Создаём источник псевдослучайных чисел
	mt19937_64 gen(161803398);
	/**
	 * Выполняем перебор случайных значений целых чисел
	 */
	for(uint32_t i = 0; i < 200000; ++i){
		// Получаем очередное значение целого числа
		const int64_t value = static_cast <int64_t> (gen());
		/**
		 * Выполняем перебор оснований систем счисления
		 */
		for(const int32_t base : {2, 8, 10, 16, 36}){
			// Выполняем запись целого числа
			const lexical_t::output_t <char> result = lexical_t::toChars(buffer, buffer + sizeof(buffer), value, base);

			// Проверяем что запись числа выполнена
			ASSERT_TRUE(static_cast <bool> (result)) << "value = " << value;
			// Разобранное обратно значение числа
			int64_t back = 0;
			// Выполняем разбор записи числа
			lexical_t::fromChars(buffer, result.ptr, back, base);
			// Проверяем что запись читается обратно тем же самым числом
			ASSERT_EQ(back, value) << "base = " << base;
		}
	}
}

/**
 * @brief Тест отказа записи при недостатке отведённого места
 *
 * @details Запись обязана отвечать отказом, а не выходить за пределы буфера,
 *          поэтому проверка ведётся для каждой длины буфера от нулевой до достаточной.
 *
 */
TEST_F(LexicalFixture, WriteInsufficientBufferTest){
	// Хранилище записи числа с охранной областью
	char buffer[maxRecordLength <double> ()];
	/**
	 * Выполняем перебор проверяемых значений
	 */
	for(const double value : vector <double> ({
		1., -1., 0., -0., 0.3, 123.456, 1e300, 5e-324, -2.5e-10,
		numeric_limits <double>::max(), numeric_limits <double>::infinity(),
		numeric_limits <double>::quiet_NaN()
	})){
		// Определяем достаточную длину записи числа
		const size_t length = compose(value).size();
		/**
		 * Выполняем перебор всех недостаточных длин отведённого места
		 */
		for(size_t size = 0; size < length; ++size){
			// Заполняем хранилище записи охранным значением
			::memset(buffer, '#', sizeof(buffer));
			// Выполняем запись числа в недостаточное место
			const lexical_t::output_t <char> result = lexical_t::toChars(buffer, buffer + size, value);

			// Проверяем что запись числа отвергнута
			ASSERT_FALSE(static_cast <bool> (result)) << "value = " << value << ", size = " << size;
			// Проверяем что причиной отказа указан недостаток места
			ASSERT_EQ(result.error, error_t::INSUFFICIENT_BUFFER) << "value = " << value;
			/**
			 * Выполняем перебор байтов за пределами отведённого места
			 */
			for(size_t i = size; i < sizeof(buffer); ++i)
				// Проверяем что байт за пределами отведённого места не изменён
				ASSERT_EQ(buffer[i], '#') << "value = " << value << ", size = " << size << ", index = " << i;
		}
		// Выполняем запись числа в достаточное место
		const lexical_t::output_t <char> result = lexical_t::toChars(buffer, buffer + length, value);

		// Проверяем что запись числа выполнена
		ASSERT_TRUE(static_cast <bool> (result)) << "value = " << value;
	}
}
