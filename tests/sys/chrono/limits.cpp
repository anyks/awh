/**
 * @file: limits.cpp
 * @date: 2026-07-29
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Тесты пределов представимости календаря — записи ранее начала эпохи, штампы
 *        времени за пределом четырёхзначного года, промежуток смещений временных зон
 *        и знак продолжительности
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include "chrono.hpp"

/**
 * @brief Наибольший штамп времени, представимый календарём модуля
 *
 * @details Последняя миллисекунда 9999 года
 *
 */
static constexpr uint64_t MAX_TIMESTAMP = 253402300799999ULL;

/**
 * @brief Структура параметров тестирования записи года вне пределов эпохи
 *
 */
struct EpochTestParameter {
	// Проверяемая запись даты
	std::string date = "";
	// Ожидаемый признак пригодности записи
	bool valid = false;
};

/**
 * @brief Структура параметров теста записи года вне пределов эпохи
 *
 */
struct EpochParameterizedFixture : public ChronoFixture, public ::testing::WithParamInterface <EpochTestParameter> {
	// Параметры теста
	EpochTestParameter _parameter = GetParam();
};

/**
 * @brief Тест разбора записи года вне пределов эпохи
 *
 * @details Отсчёт штампа времени ведётся от начала 1970 года, и записи прежних лет
 *          представить нечем. Разность лет считалась в разрядности поля года, и
 *          запись 1969 года сворачивалась в шестьдесят три тысячи прошедших лет,
 *          давая штамп времени порядка двух квадриллионов вместо отказа
 *
 */
TEST_P(EpochParameterizedFixture, ExecutionEpochChronoTest){
	// Выполняем разбор проверяемой записи
	const uint64_t result = this->_chrono->parse(this->_parameter.date, "%Y-%m-%d");
	// Признак пригодности записи обязан соответствовать представимости года
	ASSERT_EQ(this->_chrono->validate(this->_parameter.date, "%Y-%m-%d"), this->_parameter.valid)
		<< this->_parameter.date;
	// Запись непредставимого года обязана давать начало эпохи, а не штамп из будущего
	if(!this->_parameter.valid)
		// Выполняем проверку того, что разбор дал начало эпохи
		ASSERT_EQ(result, static_cast <uint64_t> (0)) << this->_parameter.date;
	// Разобранный штамп времени обязан лежать в пределах представимости
	ASSERT_LE(result, MAX_TIMESTAMP) << this->_parameter.date;
}

/**
 * @brief Параметры тестирования записи года вне пределов эпохи
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, EpochParameterizedFixture,
	::testing::Values(
		EpochTestParameter({"1970-01-01", true}),
		EpochTestParameter({"1970-01-02", true}),
		EpochTestParameter({"2025-04-06", true}),
		EpochTestParameter({"9999-12-31", true}),
		EpochTestParameter({"1969-12-31", false}),
		EpochTestParameter({"1968-01-01", false}),
		EpochTestParameter({"1900-01-01", false}),
		EpochTestParameter({"1000-01-01", false}),
		EpochTestParameter({"0001-01-01", false}),
		EpochTestParameter({"0000-01-01", false})
	)
);

/**
 * @brief Тест разбора первого дня эпохи в зоне восточнее UTC
 *
 * @details Местная полночь первого дня эпохи в любой зоне восточнее UTC приходится на
 *          момент до её начала, и беззнаковый оборот давал вместо нуля штамп времени
 *          порядка восемнадцати квинтиллионов. Тесты закрепляют зону UTC, поэтому
 *          проверка выполняется установкой смещения самой записью
 *
 */
TEST_F(ChronoFixture, ExecutionEpochOffsetChronoTest){
	/**
	 * Выполняем перебор смещений зон восточнее UTC
	 */
	for(const char * offset : {"+0100", "+0300", "+0530", "+1200", "+1400"}){
		// Формируем запись первого дня эпохи со смещением временной зоны
		const std::string date = (std::string("1970-01-01T00:00:00") + offset);
		// Разбор обязан дать начало эпохи, а не оборот беззнакового штампа времени
		ASSERT_EQ(this->_chrono->parse(date, "%Y-%m-%dT%H:%M:%S%z"), static_cast <uint64_t> (0)) << date;
	}
	/**
	 * Выполняем перебор смещений зон западнее UTC, где момент представим
	 */
	for(const char * offset : {"-0100", "-0500", "-1200"}){
		// Формируем запись первого дня эпохи со смещением временной зоны
		const std::string date = (std::string("1970-01-01T00:00:00") + offset);
		// Разбор обязан дать положительный штамп времени
		ASSERT_GT(this->_chrono->parse(date, "%Y-%m-%dT%H:%M:%S%z"), static_cast <uint64_t> (0)) << date;
	}
}

/**
 * @brief Тест формирования записи из штампа времени за пределом представимости
 *
 * @details Поле года разрядностью в два октета обрезало год, а остаток суток за
 *          вычетом начала обрезанного года выходил за разрядность полей месяца и
 *          числа: формирование давало несуществующие даты вида 1934-12-200
 *
 */
TEST_F(ChronoFixture, ExecutionOverflowChronoTest){
	// Последний представимый момент выводится как есть
	ASSERT_EQ(this->_chrono->format(MAX_TIMESTAMP, "%Y-%m-%d %H:%M:%S"), "9999-12-31 23:59:59");
	/**
	 * Выполняем перебор штампов времени за пределом представимости
	 */
	for(const uint64_t date : {(MAX_TIMESTAMP + 1), (MAX_TIMESTAMP * 2), std::numeric_limits <uint64_t>::max()}){
		// Получаем сформированную запись штампа времени
		const std::string result = this->_chrono->format(date, "%Y-%m-%d %H:%M:%S");
		// Запись обязана приводиться к последнему представимому моменту
		ASSERT_EQ(result, "9999-12-31 23:59:59") << date;
		// Число месяца в записи обязано оставаться числом месяца
		ASSERT_EQ(result.length(), static_cast <size_t> (19)) << result;
	}
	/**
	 * Выполняем перебор штампов времени за пределом представимости для извлечения года
	 */
	for(const uint64_t date : {(MAX_TIMESTAMP + 1), std::numeric_limits <uint64_t>::max()})
		// Извлечение года обязано давать наибольший представимый год
		ASSERT_EQ(this->_chrono->year(date), static_cast <uint16_t> (9999)) << date;
}

/**
 * @brief Тест разбора последнего момента 9999 года в зоне западнее UTC
 *
 * @details Местная полночь в зоне западнее UTC наступает позже, чем в нулевой, и
 *          последний момент 9999 года в такой зоне выходил за предел представимости:
 *          разбор выдавал штамп, который формирование записи прочитать обратно уже
 *          не могло
 *
 */
TEST_F(ChronoFixture, ExecutionOverflowOffsetChronoTest){
	/**
	 * Выполняем перебор смещений зон западнее UTC
	 */
	for(const char * offset : {"-0000", "-0100", "-0500", "-1200"}){
		// Формируем запись последнего момента 9999 года со смещением временной зоны
		const std::string date = (std::string("9999-12-31T23:59:59") + offset);
		// Получаем разобранный штамп времени
		const uint64_t result = this->_chrono->parse(date, "%Y-%m-%dT%H:%M:%S%z");
		// Разобранный штамп времени обязан лежать в пределах представимости
		ASSERT_LE(result, MAX_TIMESTAMP) << date;
		// Формирование записи из разобранного штампа обязано давать 9999 год
		ASSERT_EQ(this->_chrono->format(result, "%Y"), "9999") << date;
	}
}

/**
 * @brief Структура параметров тестирования промежутка смещений временных зон
 *
 */
struct ZoneRangeTestParameter {
	// Проверяемое смещение временной зоны в записи
	std::string offset = "";
	// Ожидаемый признак пригодности записи
	bool valid = false;
};

/**
 * @brief Структура параметров теста промежутка смещений временных зон
 *
 */
struct ZoneRangeParameterizedFixture : public ChronoFixture, public ::testing::WithParamInterface <ZoneRangeTestParameter> {
	// Параметры теста
	ZoneRangeTestParameter _parameter = GetParam();
};

/**
 * @brief Тест промежутка смещений временных зон
 *
 * @details Пояса Земли укладываются в промежуток от UTC-12 до UTC+14, и крайние его
 *          точки заняты: UTC-12 - острова Бейкер и Хауленд, UTC+14 - острова Лайн в
 *          составе Кирибати. Запись вида «+9999» разбор принимал и накладывал
 *          смещение в сто часов, унося момент записи на четверо суток
 *
 */
TEST_P(ZoneRangeParameterizedFixture, ExecutionZoneRangeChronoTest){
	// Формируем запись даты с проверяемым смещением временной зоны
	const std::string date = (std::string("2025-04-06T14:30:45") + this->_parameter.offset);
	// Выполняем проверку признака пригодности записи
	ASSERT_EQ(this->_chrono->validate(date, "%Y-%m-%dT%H:%M:%S%z"), this->_parameter.valid) << date;
}

/**
 * @brief Параметры тестирования промежутка смещений временных зон
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, ZoneRangeParameterizedFixture,
	::testing::Values(
		ZoneRangeTestParameter({"+0000", true}),
		ZoneRangeTestParameter({"+0300", true}),
		ZoneRangeTestParameter({"+0530", true}),
		ZoneRangeTestParameter({"-0330", true}),
		ZoneRangeTestParameter({"+1245", true}),
		ZoneRangeTestParameter({"+1400", true}),
		ZoneRangeTestParameter({"-1200", true}),
		ZoneRangeTestParameter({"+1401", false}),
		ZoneRangeTestParameter({"+1500", false}),
		ZoneRangeTestParameter({"-1201", false}),
		ZoneRangeTestParameter({"-1300", false}),
		ZoneRangeTestParameter({"+9999", false}),
		ZoneRangeTestParameter({"-9999", false})
	)
);

/**
 * @brief Тест ограничения числового обозначения временной зоны
 *
 * @details Название зоны числом означает целое количество часов смещения, и
 *          ограничивать его необходимо: произведение считалось в разрядности int и
 *          переполнялось на названиях от шестисот тысяч, а название «999» задавало
 *          смещение в тысячу часов, уводившее в разнос всякий последующий расчёт
 *
 */
TEST_F(ChronoFixture, ExecutionZoneNameChronoTest){
	/**
	 * Выполняем перебор числовых обозначений внутри промежутка земных поясов
	 */
	for(const auto & item : {std::make_pair("3", 10800), std::make_pair("14", 50400), std::make_pair("-12", -43200)})
		// Обозначение внутри промежутка обязано давать заданное им смещение
		ASSERT_EQ(this->_chrono->getTimeZone(item.first), item.second) << item.first;
	/**
	 * Выполняем перебор числовых обозначений за промежутком земных поясов
	 */
	for(const char * zone : {"15", "999", "100000", "-13", "-99999"})
		// Обозначение за промежутком обязано оставлять смещение нетронутым
		ASSERT_EQ(this->_chrono->getTimeZone(zone), 0) << zone;
	/**
	 * Проверка обозначения согласуется с его переводом: обозначение со смещением за
	 * промежутком земных поясов проверка признавала пригодным, тогда как перевод его
	 * выдавал ноль - вызывающей стороне обещалась зона там, где она получала UTC
	 */
	for(const char * zone : {"+1500", "-1300", "+15:00", "GMT+15", "+9999"}){
		// Обозначение за промежутком пригодным не является
		ASSERT_FALSE(this->_chrono->validateTimeZone(zone)) << zone;
		// Перевод такого обозначения смещения не даёт
		ASSERT_EQ(this->_chrono->getTimeZone(zone), 0) << zone;
	}
	/**
	 * Выполняем перебор обозначений внутри промежутка земных поясов
	 */
	for(const char * zone : {"+1400", "-1200", "+05:30", "GMT+14", "MSK"})
		// Обозначение внутри промежутка пригодным является
		ASSERT_TRUE(this->_chrono->validateTimeZone(zone)) << zone;
}

/**
 * @brief Тест приведения смещения записи к земным пределам
 *
 * @details Разбор снисходителен и запись со смещением, которого на Земле нет,
 *          принимает, но применять такое смещение как есть он не вправе: запись
 *          «+1500» уводила момент на час дальше самого дальнего пояса. Проверка же
 *          записи такое смещение отвергает - она судит по разобранным полям, и
 *          приведение при разборе поля отняло бы у неё эту возможность
 *
 */
TEST_F(ChronoFixture, ExecutionZoneClampChronoTest){
	// Смещение за дальним пределом приводится к нему самому
	ASSERT_EQ(this->_chrono->parse("2025-04-06T14:30:45+1500", "%Y-%m-%dT%H:%M:%S%z"),
	          this->_chrono->parse("2025-04-06T14:30:45+1400", "%Y-%m-%dT%H:%M:%S%z"));
	// Смещение за ближним пределом приводится к нему самому
	ASSERT_EQ(this->_chrono->parse("2025-04-06T14:30:45-1300", "%Y-%m-%dT%H:%M:%S%z"),
	          this->_chrono->parse("2025-04-06T14:30:45-1200", "%Y-%m-%dT%H:%M:%S%z"));
	// Запись со смещением за пределами пригодной не является
	ASSERT_FALSE(this->_chrono->validate("2025-04-06T14:30:45+1500", "%Y-%m-%dT%H:%M:%S%z"));
	// Запись со смещением на самом пределе пригодна
	ASSERT_TRUE(this->_chrono->validate("2025-04-06T14:30:45+1400", "%Y-%m-%dT%H:%M:%S%z"));
}

/**
 * @brief Тест независимости обозначения временной зоны от установленной зоны
 *
 * @details Обозначение задаёт временную зону целиком, а не поправку к установленной.
 *          Смещение собиралось от смещения, установленного объекту, и метод выходил
 *          накапливающим: обозначение «3» после установки той же зоны давало шесть
 *          часов вместо трёх, а установка зоны дважды подряд удваивала смещение
 *
 */
TEST_F(ChronoFixture, ExecutionZoneIdempotentChronoTest){
	/**
	 * Выполняем перебор обозначений временных зон всех видов вместе с ожидаемым
	 * смещением, которое каждое из них задаёт
	 */
	for(const auto & item : {
		std::make_pair("3", 10800), std::make_pair("14", 50400), std::make_pair("-12", -43200),
		std::make_pair("MSK", 10800), std::make_pair("UTC", 0), std::make_pair("GMT", 0),
		std::make_pair("+05:30", 19800), std::make_pair("GMT+0530", 19800), std::make_pair("UTC-3:28", -12480)
	}){
		// Объект работы с датой и временем для проверки обозначения
		awh::chrono_t chrono(this->_fmk.get(), this->_log.get());
		/**
		 * Выполняем повторную установку одного и того же обозначения. Смещение читается
		 * из местного хранилища: без довода хранилища метод выдаёт зону операционной
		 * системы, а не установленную объекту
		 */
		for(uint8_t i = 0; i < 3; i++){
			// Выполняем установку временной зоны обозначением
			chrono.setTimeZone(item.first);
			// Сохранённое смещение обязано равняться заданному, сколько бы раз зона ни устанавливалась
			ASSERT_EQ(chrono.getTimeZone(awh::chrono_t::storage_t::LOCAL), item.second)
				<< item.first << " (установка " << static_cast <uint16_t> (i + 1) << ")";
		}
	}
	/**
	 * Выполняем повторную установку временной зоны смещением в секундах
	 */
	{
		// Объект работы с датой и временем для проверки установки смещением
		awh::chrono_t chrono(this->_fmk.get(), this->_log.get());
		/**
		 * Выполняем повторную установку одного и того же смещения
		 */
		for(uint8_t i = 0; i < 3; i++){
			// Выполняем установку временной зоны смещением в секундах
			chrono.setTimeZone(static_cast <int32_t> (19800));
			// Сохранённое смещение обязано равняться заданному
			ASSERT_EQ(chrono.getTimeZone(awh::chrono_t::storage_t::LOCAL), 19800)
				<< "установка " << static_cast <uint16_t> (i + 1);
		}
	}
	/**
	 * Выполняем повторную установку временной зоны её идентификатором
	 */
	{
		// Объект работы с датой и временем для проверки установки идентификатором
		awh::chrono_t chrono(this->_fmk.get(), this->_log.get());
		/**
		 * Выполняем повторную установку одного и того же идентификатора
		 */
		for(uint8_t i = 0; i < 3; i++){
			// Выполняем установку временной зоны её идентификатором
			chrono.setTimeZone(awh::chrono_t::zone_t::MSK);
			// Сохранённое смещение обязано равняться смещению указанной зоны
			ASSERT_EQ(chrono.getTimeZone(awh::chrono_t::storage_t::LOCAL), 10800)
				<< "установка " << static_cast <uint16_t> (i + 1);
		}
	}
	// Объект работы с датой и временем для проверки непригодных обозначений
	awh::chrono_t chrono(this->_fmk.get(), this->_log.get());
	// Выполняем установку известной временной зоны
	chrono.setTimeZone("MSK");
	/**
	 * Выполняем перебор обозначений, зоны не задающих
	 */
	for(const char * zone : {"999", "15", "-13", "XXXX", ""})
		// Непригодное обозначение обязано оставлять установленную зону нетронутой
		ASSERT_EQ(chrono.getTimeZone(zone), 10800) << zone;
}

/**
 * @brief Тест раскрытия двузначного обозначения года
 *
 * @details Правило раскрытия задаёт RFC 9110 (§5.6.7) для устаревшего формата даты
 *          RFC 850: обозначение, попадающее дальше в будущее, чем допускает окно,
 *          читается как ближайший прошедший год с теми же двумя разрядами. Прежде
 *          двузначный год всегда относился к двадцать первому веку, и запись
 *          «06-Nov-94» читалась 2094 годом - на семьдесят лет вперёд
 *
 */
TEST_F(ChronoFixture, ExecutionYearWindowChronoTest){
	// Получаем текущее обозначение года
	const uint16_t current = this->_chrono->year(this->_chrono->timestamp(awh::chrono_t::type_t::MILLISECONDS));
	// Окно двузначного года по умолчанию задано стандартом
	ASSERT_EQ(this->_chrono->yearWindow(), static_cast <uint8_t> (50));
	/**
	 * Выполняем перебор всех двузначных обозначений года
	 */
	for(uint16_t value = 0; value < 100; value++){
		// Получаем год того же столетия, что и текущий
		uint16_t expected = static_cast <uint16_t> (((current / 100) * 100) + value);
		// Обозначение дальше в будущем, чем допускает окно, относится к прошлому столетию
		if(expected > (current + 50))
			// Относим обозначение к предыдущему столетию
			expected -= 100;
		// Год ранее эпохи штампом времени не представим, такие обозначения пропускаем
		if(expected < 1970)
			// Переходим к следующему обозначению
			continue;
		// Формируем запись даты двузначным обозначением года
		char date[16];
		// Записываем обозначение года двумя разрядами
		::snprintf(date, sizeof(date), "%02u-01-15", static_cast <uint32_t> (value));
		// Выполняем проверку раскрытого обозначения года
		ASSERT_EQ(this->_chrono->format(this->_chrono->parse(date, "%y-%m-%d"), "%Y"), std::to_string(expected))
			<< date;
	}
	/**
	 * Выполняем проверку примера, приведённого самим стандартом
	 */
	{
		// Получаем год того же столетия, что и текущий, для обозначения 94
		uint16_t expected = static_cast <uint16_t> (((current / 100) * 100) + 94);
		// Обозначение дальше в будущем, чем допускает окно, относится к прошлому столетию
		if(expected > (current + 50))
			// Относим обозначение к предыдущему столетию
			expected -= 100;
		// Запись устаревшего формата RFC 850, приведённая в RFC 9110
		ASSERT_EQ(this->_chrono->format(this->_chrono->parse("Sunday, 06-Nov-94 08:49:37", "%A, %d-%b-%y %H:%M:%S"), "%Y-%m-%d"),
			(std::to_string(expected) + "-11-06"));
	}
}

/**
 * @brief Тест отключения правила раскрытия двузначного года
 *
 * @details Нулевое окно возвращает поведение, действовавшее до появления правила:
 *          двузначное обозначение всегда относится к двадцать первому веку
 *
 */
TEST_F(ChronoFixture, ExecutionYearWindowOffChronoTest){
	// Объект работы с датой и временем с отключённым правилом
	awh::chrono_t chrono(this->_fmk.get(), this->_log.get());
	// Выполняем отключение правила раскрытия двузначного года
	chrono.yearWindow(0);
	// Выполняем проверку того, что правило отключено
	ASSERT_EQ(chrono.yearWindow(), static_cast <uint8_t> (0));
	/**
	 * Выполняем перебор двузначных обозначений года
	 */
	for(const uint16_t value : {static_cast <uint16_t> (94), static_cast <uint16_t> (70), static_cast <uint16_t> (99), static_cast <uint16_t> (0)}){
		// Формируем запись даты двузначным обозначением года
		char date[16];
		// Записываем обозначение года двумя разрядами
		::snprintf(date, sizeof(date), "%02u-01-15", static_cast <uint32_t> (value));
		// Обозначение обязано относиться к двадцать первому веку
		ASSERT_EQ(chrono.format(chrono.parse(date, "%y-%m-%d"), "%Y"), std::to_string(2000 + value)) << date;
	}
	// Полное обозначение года правило не затрагивает ни в каком его виде
	ASSERT_EQ(chrono.format(chrono.parse("1994-11-06", "%Y-%m-%d"), "%Y"), "1994");
}

/**
 * @brief Структура параметров тестирования знака продолжительности
 *
 */
struct DurationSignTestParameter {
	// Проверяемая продолжительность в секундах
	double seconds = 0.;
	// Ожидаемое обозначение продолжительности
	std::string result = "";
};

/**
 * @brief Структура параметров теста знака продолжительности
 *
 */
struct DurationSignParameterizedFixture : public ChronoFixture, public ::testing::WithParamInterface <DurationSignTestParameter> {
	// Параметры теста
	DurationSignTestParameter _parameter = GetParam();
};

/**
 * @brief Тест знака продолжительности
 *
 * @details Разбор обозначения знак читает, и вывод обязан его писать: продолжительность
 *          в минус два часа выводилась обозначением «0s», и обратное преобразование
 *          теряло величину целиком
 *
 */
TEST_P(DurationSignParameterizedFixture, ExecutionDurationSignChronoTest){
	// Выполняем проверку сформированного обозначения продолжительности
	ASSERT_EQ(this->_chrono->seconds(this->_parameter.seconds), this->_parameter.result)
		<< this->_parameter.seconds;
}

/**
 * @brief Параметры тестирования знака продолжительности
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, DurationSignParameterizedFixture,
	::testing::Values(
		DurationSignTestParameter({0., "0s"}),
		DurationSignTestParameter({3725., "1.03h"}),
		DurationSignTestParameter({-3725., "-1.03h"}),
		DurationSignTestParameter({2592000., "4.29w"}),
		DurationSignTestParameter({-2592000., "-4.29w"}),
		DurationSignTestParameter({0.001, "0.001s"}),
		DurationSignTestParameter({-0.001, "-0.001s"}),
		DurationSignTestParameter({-7200., "-2h"}),
		DurationSignTestParameter({-259200., "-3d"})
	)
);

/**
 * @brief Тест обратимости обозначения продолжительности со знаком
 *
 */
TEST_F(ChronoFixture, ExecutionDurationRoundtripChronoTest){
	/**
	 * Выполняем перебор обозначений продолжительности со знаком
	 */
	for(const char * value : {"-2h", "-3d", "-1w", "2h", "3d", "1w"})
		// Обозначение обязано читаться обратно тем же обозначением
		ASSERT_EQ(this->_chrono->seconds(this->_chrono->seconds(value)), value) << value;
}
