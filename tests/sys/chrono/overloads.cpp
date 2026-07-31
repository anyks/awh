/**
 * @file: overloads.cpp
 * @date: 2026-07-31
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Тесты согласованности перегрузок формирования записи — каждая перегрузка,
 *        задающая временную зону своим способом, обязана давать ту же запись, что и
 *        перегрузка, принимающая смещение числом
 *
 * @details Набор написан по итогам замера покрытия: семь открытых перегрузок
 *          формирования записи и методы digits и h12 не вызывались ни одним тестом,
 *          то есть об их поведении не было известно ничего. Проверки построены на
 *          сличении перегрузок между собой, поскольку эталон здесь - не внешний
 *          стандарт, а обещание заголовочного файла, что перегрузки различаются лишь
 *          способом задать зону и хранилище
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include "chrono.hpp"

/**
 * @brief Эталонный момент времени всех тестов набора
 *
 * @details Момент соответствует 2025-04-06T12:37:01Z
 *
 */
static constexpr uint64_t OVERLOAD_DATE = 1743943021000;

/**
 * @brief Эталонный формат записи всех тестов набора
 *
 * @details Формат собран так, чтобы задействовать все поля гражданского времени сразу
 *          с обеими записями смещения: расхождение перегрузок в любом из полей
 *          изменит запись целиком.
 *
 *          Переменная %Z в формат намеренно не входит: обозначение зоны несут лишь
 *          перечисление и строка, тогда как смещение, переданное числом, обозначения
 *          не имеет и записывается числовой формой. Обозначение проверяется отдельно
 *
 */
static constexpr const char * OVERLOAD_FORMAT = "%Y-%m-%dT%H:%M:%S%z %o %j %u";

/**
 * @brief Структура параметров теста перегрузок формирования записи
 *
 */
struct OverloadFixture : public ChronoFixture {};

/**
 * @brief Тест согласованности перегрузок формирования записи по штампу времени
 *
 * @details Перегрузки, принимающие временную зону перечислением и обозначением,
 *          обязаны давать ту же запись, что и перегрузка, принимающая смещение числом.
 *          Перебор ведётся по всему перечислению: расхождение хотя бы одной зоны
 *          означает, что запись зависит от способа её задать
 *
 */
TEST_F(OverloadFixture, ExecutionOverloadDateZoneChronoTest){
	// Выполняем перебор всех временных зон перечисления
	for(uint8_t i = 1; i <= static_cast <uint8_t> (awh::chrono_t::zone_t::WGSTST); i++){
		// Получаем временную зону перечисления
		const awh::chrono_t::zone_t zone = static_cast <awh::chrono_t::zone_t> (i);
		// Получаем смещение временной зоны
		const int32_t offset = this->_chrono->getTimeZone(zone);
		// Формируем запись по смещению временной зоны
		const std::string expected = this->_chrono->format(OVERLOAD_DATE, offset, OVERLOAD_FORMAT);
		// Выполняем проверку согласованности перегрузки, принимающей зону перечислением
		ASSERT_EQ(this->_chrono->format(OVERLOAD_DATE, zone, OVERLOAD_FORMAT), expected)
			<< this->_chrono->format(zone);
		// Выполняем проверку согласованности перегрузки, принимающей зону обозначением
		ASSERT_EQ(this->_chrono->format(OVERLOAD_DATE, this->_chrono->format(zone), OVERLOAD_FORMAT), expected)
			<< this->_chrono->format(zone);
		// Выполняем проверку записи обозначения временной зоны, переданной перечислением
		ASSERT_EQ(this->_chrono->format(OVERLOAD_DATE, zone, "%Z"), this->_chrono->format(zone))
			<< this->_chrono->format(zone);
	}
}

/**
 * @brief Тест согласованности перегрузок формирования записи по местному хранилищу
 *
 * @details Перегрузки без штампа времени берут момент из хранилища. Момент
 *          закрепляется в объекте заранее, поскольку глобальное хранилище берёт
 *          текущий момент и запись менялась бы между вызовами
 *
 */
TEST_F(OverloadFixture, ExecutionOverloadStorageZoneChronoTest){
	// Закрепляем эталонный момент времени в местном хранилище объекта
	this->_chrono->timestamp(OVERLOAD_DATE, awh::chrono_t::type_t::MILLISECONDS);
	// Выполняем перебор всех временных зон перечисления
	for(uint8_t i = 1; i <= static_cast <uint8_t> (awh::chrono_t::zone_t::WGSTST); i++){
		// Получаем временную зону перечисления
		const awh::chrono_t::zone_t zone = static_cast <awh::chrono_t::zone_t> (i);
		// Получаем смещение временной зоны
		const int32_t offset = this->_chrono->getTimeZone(zone);
		// Формируем запись по смещению временной зоны
		const std::string expected = this->_chrono->format(offset, OVERLOAD_FORMAT, awh::chrono_t::storage_t::LOCAL);
		// Выполняем проверку согласованности перегрузки, принимающей зону перечислением
		ASSERT_EQ(this->_chrono->format(zone, OVERLOAD_FORMAT, awh::chrono_t::storage_t::LOCAL), expected)
			<< this->_chrono->format(zone);
		// Выполняем проверку согласованности перегрузки, принимающей зону обозначением
		ASSERT_EQ(this->_chrono->format(this->_chrono->format(zone), OVERLOAD_FORMAT, awh::chrono_t::storage_t::LOCAL), expected)
			<< this->_chrono->format(zone);
		// Выполняем проверку записи обозначения временной зоны, переданной перечислением
		ASSERT_EQ(this->_chrono->format(zone, "%Z", awh::chrono_t::storage_t::LOCAL), this->_chrono->format(zone))
			<< this->_chrono->format(zone);
	}
}

/**
 * @brief Тест согласованности записи по местному хранилищу и по штампу времени
 *
 * @details Запись момента, закреплённого в хранилище, обязана совпадать с записью того
 *          же момента, переданного штампом времени: хранилище задаёт момент, а не
 *          правила его записи
 *
 */
TEST_F(OverloadFixture, ExecutionOverloadStorageDateChronoTest){
	// Закрепляем эталонный момент времени в местном хранилище объекта
	this->_chrono->timestamp(OVERLOAD_DATE, awh::chrono_t::type_t::MILLISECONDS);
	// Выполняем перебор смещений с шагом в час по всему диапазону зон
	for(int32_t offset = -43200; offset <= 50400; offset += 3600)
		// Выполняем проверку согласованности записи по хранилищу и по штампу времени
		ASSERT_EQ(this->_chrono->format(offset, OVERLOAD_FORMAT, awh::chrono_t::storage_t::LOCAL),
			this->_chrono->format(OVERLOAD_DATE, offset, OVERLOAD_FORMAT)) << offset;
}

/**
 * @brief Тест подсчёта количества разрядов числа
 *
 * @details Проверка ведётся по границам разрядности: наибольшее число каждого разряда
 *          и наименьшее число следующего. Метод остаётся открытым, хотя внутри модуля
 *          больше не вызывается, и без проверки его правка прошла бы незамеченной
 *
 */
TEST_F(OverloadFixture, ExecutionOverloadDigitsChronoTest){
	// Устанавливаем текущее количество разрядов числа
	uint64_t value = 1;
	// Выполняем перебор всех разрядов числа
	for(uint8_t i = 1; i < 20; i++){
		// Выполняем проверку количества разрядов наименьшего числа разряда
		ASSERT_EQ(this->_chrono->digits(value), i) << value;
		// Выполняем проверку количества разрядов наибольшего числа разряда
		ASSERT_EQ(this->_chrono->digits((value * 10) - 1), i) << ((value * 10) - 1);
		// Увеличиваем разрядность числа
		value *= 10;
	}
	// Выполняем проверку количества разрядов нулевого значения
	ASSERT_EQ(this->_chrono->digits(0), static_cast <uint8_t> (1));
}

/**
 * @brief Тест определения половины суток
 *
 * @details Половина суток обязана совпадать с записью переменной формата %p того же
 *          момента: обе берутся из одного поля гражданского времени, но разными
 *          путями. Проверка ведётся сплошным перебором часов суток
 *
 */
TEST_F(OverloadFixture, ExecutionOverloadMeridiemChronoTest){
	// Получаем штамп времени начала суток
	const uint64_t begin = this->_chrono->parse("2025-04-06", "%Y-%m-%d");
	// Выполняем перебор всех часов суток
	for(uint8_t hour = 0; hour < 24; hour++){
		// Получаем штамп времени текущего часа суток
		const uint64_t date = (begin + (static_cast <uint64_t> (hour) * 3600000ULL));
		// Получаем половину суток текущего часа
		const awh::chrono_t::h12_t h12 = this->_chrono->h12(date);
		// Выполняем проверку половины суток текущего часа
		ASSERT_EQ(h12, ((hour < 12) ? awh::chrono_t::h12_t::AM : awh::chrono_t::h12_t::PM))
			<< static_cast <uint16_t> (hour);
		// Выполняем проверку совпадения половины суток с записью переменной формата
		ASSERT_EQ(this->_chrono->format(date, 0, "%p"), ((h12 == awh::chrono_t::h12_t::AM) ? "AM" : "PM"))
			<< static_cast <uint16_t> (hour);
		// Закрепляем текущий момент времени в местном хранилище объекта
		this->_chrono->timestamp(date, awh::chrono_t::type_t::MILLISECONDS);
		// Выполняем проверку половины суток, взятой из местного хранилища
		ASSERT_EQ(this->_chrono->h12(awh::chrono_t::storage_t::LOCAL), h12) << static_cast <uint16_t> (hour);
	}
}

/**
 * @brief Перечень всех единиц данных даты и времени
 *
 */
static constexpr awh::chrono_t::unit_t OVERLOAD_UNITS[] = {
	awh::chrono_t::unit_t::DAY, awh::chrono_t::unit_t::DATE, awh::chrono_t::unit_t::YEAR,
	awh::chrono_t::unit_t::HOUR, awh::chrono_t::unit_t::DAYS, awh::chrono_t::unit_t::MONTH,
	awh::chrono_t::unit_t::WEEKS, awh::chrono_t::unit_t::OFFSET, awh::chrono_t::unit_t::MINUTES,
	awh::chrono_t::unit_t::SECONDS, awh::chrono_t::unit_t::MILLISECONDS,
	awh::chrono_t::unit_t::MICROSECONDS, awh::chrono_t::unit_t::NANOSECONDS
};

/**
 * @brief Тест независимости извлечения данных от запрошенного типа
 *
 * @details Значение единицы данных не зависит от типа, которым его извлекают: широкий
 *          тип обязан давать то же число, что и эталонный, а вещественный - то же
 *          число без дробной части. Прежде приватный движок копировал в буфер родную
 *          ширину поля, ничего не зная о типе результата: извлечение типом float либо
 *          double давало не значение, а его октеты, принятые за вещественное число, и
 *          все тринадцать единиц читались неверно
 *
 */
TEST_F(OverloadFixture, ExecutionOverloadTypeChronoTest){
	// Выполняем перебор всех единиц данных даты и времени
	for(const awh::chrono_t::unit_t unit : OVERLOAD_UNITS){
		// Получаем эталонное значение единицы данных
		const int64_t expected = this->_chrono->get <int64_t> (OVERLOAD_DATE, unit);
		// Выполняем проверку извлечения значения беззнаковым типом наибольшей разрядности
		ASSERT_EQ(this->_chrono->get <uint64_t> (OVERLOAD_DATE, unit), static_cast <uint64_t> (expected));
		// Выполняем проверку извлечения значения знаковым типом в четыре октета
		ASSERT_EQ(this->_chrono->get <int32_t> (OVERLOAD_DATE, unit), static_cast <int32_t> (expected));
		// Выполняем проверку извлечения значения вещественным типом одинарной точности
		ASSERT_EQ(this->_chrono->get <float> (OVERLOAD_DATE, unit), static_cast <float> (expected));
		// Выполняем проверку извлечения значения вещественным типом двойной точности
		ASSERT_EQ(this->_chrono->get <double> (OVERLOAD_DATE, unit), static_cast <double> (expected));
	}
}

/**
 * @brief Тест обратимости установки данных всеми типами
 *
 * @details Значение, установленное любым из поддерживаемых типов, обязано читаться
 *          обратно тем же числом. Прежде приватный движок читал из буфера родную ширину
 *          поля, принимая октеты вещественного числа за целое, и установка типом float
 *          либо double давала мусор в двенадцати единицах из тринадцати
 *
 */
TEST_F(OverloadFixture, ExecutionOverloadTypeRoundtripChronoTest){
	// Выполняем перебор всех единиц данных даты и времени
	for(const awh::chrono_t::unit_t unit : OVERLOAD_UNITS){
		// Получаем эталонное значение единицы данных
		const int64_t expected = this->_chrono->get <int64_t> (OVERLOAD_DATE, unit);
		// Выполняем установку значения знаковым типом наибольшей разрядности
		this->_chrono->clear();
		this->_chrono->set <int64_t> (expected, unit);
		// Выполняем проверку обратного чтения установленного значения
		ASSERT_EQ(this->_chrono->get <int64_t> (unit, awh::chrono_t::storage_t::LOCAL), expected);
		// Выполняем установку значения вещественным типом двойной точности
		this->_chrono->clear();
		this->_chrono->set <double> (static_cast <double> (expected), unit);
		// Выполняем проверку обратного чтения установленного значения
		ASSERT_EQ(this->_chrono->get <int64_t> (unit, awh::chrono_t::storage_t::LOCAL), expected);
		// Выполняем установку значения вещественным типом одинарной точности
		this->_chrono->clear();
		this->_chrono->set <float> (static_cast <float> (expected), unit);
		// Выполняем проверку обратного чтения установленного значения
		ASSERT_EQ(this->_chrono->get <int64_t> (unit, awh::chrono_t::storage_t::LOCAL), expected);
		// Выполняем установку значения текстом
		this->_chrono->clear();
		this->_chrono->set <std::string> (std::to_string(expected), unit);
		// Выполняем проверку обратного чтения установленного значения
		ASSERT_EQ(this->_chrono->get <int64_t> (unit, awh::chrono_t::storage_t::LOCAL), expected);
	}
}

/**
 * @brief Тест извлечения и установки отрицательного смещения временной зоны
 *
 * @details Смещение временной зоны - единственная единица данных, принимающая
 *          отрицательные значения. Прежде движок копировал четыре октета знакового
 *          поля в буфер большей разрядности, и старшие октеты оставались нулевыми:
 *          отрицательное смещение читалось типом int64_t как большое положительное
 *          число
 *
 */
TEST_F(OverloadFixture, ExecutionOverloadSignedOffsetChronoTest){
	// Выполняем перебор смещений с шагом в четверть часа по всему диапазону зон
	for(int32_t offset = -43200; offset <= 50400; offset += 900){
		// Выполняем установку смещения временной зоны
		this->_chrono->clear();
		this->_chrono->set <int32_t> (offset, awh::chrono_t::unit_t::OFFSET);
		// Выполняем проверку извлечения смещения знаковым типом в четыре октета
		ASSERT_EQ(this->_chrono->get <int32_t> (awh::chrono_t::unit_t::OFFSET, awh::chrono_t::storage_t::LOCAL), offset);
		// Выполняем проверку извлечения смещения знаковым типом наибольшей разрядности
		ASSERT_EQ(this->_chrono->get <int64_t> (awh::chrono_t::unit_t::OFFSET, awh::chrono_t::storage_t::LOCAL),
			static_cast <int64_t> (offset)) << offset;
		// Выполняем проверку извлечения смещения вещественным типом двойной точности
		ASSERT_EQ(this->_chrono->get <double> (awh::chrono_t::unit_t::OFFSET, awh::chrono_t::storage_t::LOCAL),
			static_cast <double> (offset)) << offset;
		// Выполняем проверку извлечения смещения текстом
		ASSERT_EQ(this->_chrono->get <std::string> (awh::chrono_t::unit_t::OFFSET, awh::chrono_t::storage_t::LOCAL),
			std::to_string(offset)) << offset;
	}
}
