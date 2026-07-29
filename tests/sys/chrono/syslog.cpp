/**
 * @file: syslog.cpp
 * @date: 2026-07-29
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Тесты соответствия записи и разбора штампов времени стандартам системного
 *        журнала — устаревшему RFC 3164 и действующему RFC 5424
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include "chrono.hpp"

/**
 * @brief Формат штампа времени стандарта RFC 3164
 *
 * @details Стандарт задаёт запись Mmm dd hh:mm:ss длиной ровно пятнадцать разрядов
 *          (раздел 4.1.2). Число месяца меньше десяти дополняется слева пробелом, а
 *          не нулём: «седьмое августа записывается как Aug  7, с двумя пробелами
 *          между g и 7». Переменная %e даёт именно такое дополнение, %d - дополнение
 *          нулём, стандарту не соответствующее
 *
 */
static constexpr const char * BSD_FORMAT = "%b %e %H:%M:%S";

/**
 * @brief Формат штампа времени стандарта RFC 5424
 *
 * @details Стандарт задаёт штамп как date-time стандарта RFC 3339 с добавочными
 *          ограничениями (раздел 6.2.3): разделитель T обязателен, доля секунды не
 *          длиннее шести разрядов, смещение зоны записывается через двоеточие либо
 *          обозначением Z. Переменная %o даёт запись смещения ±hh:mm, требуемую
 *          стандартом; переменная %Z стандарту не соответствует, поскольку при
 *          нулевом смещении даёт UTC, а грамматика RFC 3339 такого обозначения не
 *          содержит
 *
 */
static constexpr const char * SYSLOG_FORMAT = "%Y-%m-%dT%H:%M:%S.%s%o";

/**
 * @brief Формат штампа времени с полем time-offset стандарта RFC 3339
 *
 * @details Стандарт задаёт поле смещения как «Z» при нулевом смещении либо как
 *          запись ±hh:mm при любом другом (раздел 5.6). Переменная %i выбирает между
 *          ними сама, давая ту запись, которую стандарты RFC 3339 и RFC 5424 считают
 *          основной. Переменная %o даёт при нулевом смещении +00:00 - запись, тем же
 *          стандартам не противоречащую, но не совпадающую с их примерами
 *
 */
static constexpr const char * ZULU_FORMAT = "%Y-%m-%dT%H:%M:%S.%s%i";

/**
 * @brief Структура параметров тестирования записи штампа RFC 3164
 *
 */
struct SyslogBsdTestParameter {
	// Штамп времени в миллисекундах
	uint64_t date = 0;
	// Ожидаемая запись штампа времени
	std::string result = "";
};

/**
 * @brief Структура параметров теста записи штампа RFC 3164
 *
 */
struct SyslogBsdParameterizedFixture : public ChronoFixture, public ::testing::WithParamInterface <SyslogBsdTestParameter> {
	// Параметры теста
	SyslogBsdTestParameter _parameter = GetParam();
};

/**
 * @brief Тест записи штампа времени стандарта RFC 3164
 *
 */
TEST_P(SyslogBsdParameterizedFixture, ExecutionSyslogBsdChronoTest){
	// Формируем запись штампа времени в формате устаревшего стандарта
	const std::string result = this->_chrono->format(this->_parameter.date, BSD_FORMAT);
	// Выполняем проверку сформированной записи
	ASSERT_EQ(result, this->_parameter.result);
	// Выполняем проверку разрядности записи, заданной стандартом
	ASSERT_EQ(result.length(), 15) << result;
}

/**
 * @brief Параметры тестирования записи штампа RFC 3164
 *
 * @details Набор покрывает оба разряда числа месяца по обе стороны границы в десять
 *          дней, все положения нуля во времени суток и обе границы года. Пример
 *          самого стандарта - Oct 11 22:14:15 - входит в набор дословно
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, SyslogBsdParameterizedFixture,
	::testing::Values(
		// Пример записи из текста самого стандарта
		SyslogBsdTestParameter({1760220855000, "Oct 11 22:14:15"}),
		// Число месяца меньше десяти дополняется пробелом
		SyslogBsdTestParameter({1760048055000, "Oct  9 22:14:15"}),
		SyslogBsdTestParameter({1754604855000, "Aug  7 22:14:15"}),
		SyslogBsdTestParameter({1735784055000, "Jan  2 02:14:15"}),
		// Границы года
		SyslogBsdTestParameter({1735689600000, "Jan  1 00:00:00"}),
		SyslogBsdTestParameter({1767225599000, "Dec 31 23:59:59"}),
		// Полночь и полдень
		SyslogBsdTestParameter({1743897600000, "Apr  6 00:00:00"}),
		SyslogBsdTestParameter({1743940800000, "Apr  6 12:00:00"}),
		// Дополнительный день високосного года
		SyslogBsdTestParameter({1709208000000, "Feb 29 12:00:00"})
	)
);

/**
 * @brief Структура параметров тестирования разбора штампа RFC 5424
 *
 */
struct SyslogFractionTestParameter {
	// Разбираемая доля секунды, записанная после разделителя
	std::string fraction = "";
	// Ожидаемое количество миллисекунд
	uint32_t milliseconds = 0;
};

/**
 * @brief Структура параметров теста разбора доли секунды
 *
 */
struct SyslogFractionParameterizedFixture : public ChronoFixture, public ::testing::WithParamInterface <SyslogFractionTestParameter> {
	// Параметры теста
	SyslogFractionTestParameter _parameter = GetParam();
};

/**
 * @brief Тест разбора доли секунды штампа RFC 5424
 *
 * @details Доля секунды - десятичная дробь, а не число миллисекунд: её значение
 *          задают разряды после точки, а не их количество, поэтому .5, .50 и .500
 *          означают одну и ту же половину секунды. Разряды за третьим отбрасываются:
 *          миллисекунда - предел разрешающей способности штампа времени модуля
 *
 */
TEST_P(SyslogFractionParameterizedFixture, ExecutionSyslogFractionChronoTest){
	// Формируем разбираемую запись с проверяемой долей секунды
	const std::string date = ("2025-04-06T12:37:01." + this->_parameter.fraction + "Z");
	// Выполняем разбор сформированной записи
	const uint64_t result = this->_chrono->parse(date, "%Y-%m-%dT%H:%M:%S.%s%Z");
	// Выполняем проверку полученного момента времени
	ASSERT_EQ(result, 1743943021000 + this->_parameter.milliseconds) << date;
}

/**
 * @brief Параметры тестирования разбора доли секунды
 *
 * @details Набор покрывает разрядность доли от одного разряда до девяти, обе стороны
 *          предела разрешающей способности и записи из примеров самого стандарта
 *          (.52 и .000003 раздела 6.2.3.1). До правки доля читалась как целое число
 *          миллисекунд, отчего .52 давало пятьдесят две миллисекунды вместо
 *          пятисот двадцати, а доля длиннее трёх разрядов сдвигала весь штамп: запись
 *          .345678901 добавляла к нему четверо суток
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, SyslogFractionParameterizedFixture,
	::testing::Values(
		// Доля короче трёх разрядов дополняется нулями справа
		SyslogFractionTestParameter({"5", 500}),
		SyslogFractionTestParameter({"52", 520}),
		SyslogFractionTestParameter({"05", 50}),
		SyslogFractionTestParameter({"00", 0}),
		// Доля ровно в три разряда читается как есть
		SyslogFractionTestParameter({"520", 520}),
		SyslogFractionTestParameter({"003", 3}),
		SyslogFractionTestParameter({"000", 0}),
		// Доля длиннее трёх разрядов усекается до миллисекунды
		SyslogFractionTestParameter({"0030", 3}),
		SyslogFractionTestParameter({"5209", 520}),
		SyslogFractionTestParameter({"52000", 520}),
		SyslogFractionTestParameter({"000003", 0}),
		SyslogFractionTestParameter({"0490925", 49}),
		SyslogFractionTestParameter({"345678901", 345}),
		SyslogFractionTestParameter({"999999999", 999})
	)
);

/**
 * @brief Структура параметров тестирования разбора записей стандартов
 *
 */
struct SyslogRecordTestParameter {
	// Разбираемая запись штампа времени
	std::string date = "";
	// Формат разбора записи
	std::string format = "";
	// Ожидаемый момент времени в миллисекундах
	uint64_t result = 0;
};

/**
 * @brief Структура параметров теста разбора записей стандартов
 *
 */
struct SyslogRecordParameterizedFixture : public ChronoFixture, public ::testing::WithParamInterface <SyslogRecordTestParameter> {
	// Параметры теста
	SyslogRecordTestParameter _parameter = GetParam();
};

/**
 * @brief Тест разбора записи штампа времени, приведённой в тексте стандарта
 *
 */
TEST_P(SyslogRecordParameterizedFixture, ExecutionSyslogRecordChronoTest){
	// Выполняем проверку результата разбора записи штампа времени
	ASSERT_EQ(this->_chrono->parse(this->_parameter.date, this->_parameter.format), this->_parameter.result)
		<< this->_parameter.date;
}

/**
 * @brief Параметры тестирования разбора записей стандартов
 *
 * @details Все записи взяты дословно из раздела 6.2.3.1 стандарта RFC 5424. Эталонные
 *          значения получены независимо от модуля - переводом записи в момент времени
 *          средствами языка Python
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, SyslogRecordParameterizedFixture,
	::testing::Values(
		// Пример стандарта: доля секунды в два разряда означает пятьсот двадцать миллисекунд
		SyslogRecordTestParameter({"1985-04-12T23:20:50.52Z", "%Y-%m-%dT%H:%M:%S.%s%Z", 482196050520}),
		// Тот же момент времени, записанный в зоне -04:00
		SyslogRecordTestParameter({"1985-04-12T19:20:50.52-04:00", "%Y-%m-%dT%H:%M:%S.%s%o", 482196050520}),
		// Пример стандарта: три миллисекунды
		SyslogRecordTestParameter({"2003-10-11T22:14:15.003Z", "%Y-%m-%dT%H:%M:%S.%s%Z", 1065910455003}),
		// Пример стандарта: три микросекунды, миллисекундами не выражаются
		SyslogRecordTestParameter({"2003-08-24T05:14:15.000003-07:00", "%Y-%m-%dT%H:%M:%S.%s%o", 1061727255000}),
		// Штамп без доли секунды
		SyslogRecordTestParameter({"2003-10-11T22:14:15Z", "%Y-%m-%dT%H:%M:%S%Z", 1065910455000}),
		// Штамп в зоне с некратным часу смещением
		SyslogRecordTestParameter({"2003-10-12T03:44:15.003+05:30", "%Y-%m-%dT%H:%M:%S.%s%o", 1065910455003})
	)
);

/**
 * @brief Структура теста штампов времени системного журнала
 *
 */
struct SyslogFixture : public ChronoFixture {};

/**
 * @brief Тест обратимости штампа времени стандарта RFC 5424
 *
 * @details Перебираются моменты времени по всему поддерживаемому диапазону дат с
 *          шагом, не кратным суткам, часу и минуте, чтобы доля секунды и время суток
 *          принимали значащие значения на каждом шаге
 *
 */
TEST_F(SyslogFixture, ExecutionSyslogRoundtripChronoTest){
	// Выполняем перебор моментов времени по всему поддерживаемому диапазону дат
	for(uint64_t date = 86400001; date < 4102444800000; date += 651600037){
		// Формируем запись штампа времени в формате действующего стандарта
		const std::string result = this->_chrono->format(date, SYSLOG_FORMAT);
		// Выполняем проверку обратного разбора сформированной записи
		ASSERT_EQ(this->_chrono->parse(result, SYSLOG_FORMAT), date) << result;
		// Формируем запись штампа времени с полем смещения стандарта RFC 3339
		const std::string zulu = this->_chrono->format(date, ZULU_FORMAT);
		// Выполняем проверку обратного разбора записи с полем смещения
		ASSERT_EQ(this->_chrono->parse(zulu, ZULU_FORMAT), date) << zulu;
	}
}

/**
 * @brief Тест записи обозначения нулевого смещения стандарта RFC 3339
 *
 * @details Штамп времени, записанный в нулевой зоне, обязан оканчиваться заглавной
 *          буквой Z: этого требует раздел 6.2.3 стандарта RFC 5424, оговаривающий
 *          регистр букв T и Z отдельным пунктом. Переменная %Z для этого не годится -
 *          она даёт обозначение UTC, которого грамматика RFC 3339 не содержит
 *
 */
TEST_F(SyslogFixture, ExecutionSyslogZuluChronoTest){
	// Формируем запись штампа времени в нулевой временной зоне
	const std::string result = this->_chrono->format(1743943021520, 0, ZULU_FORMAT);
	// Выполняем проверку сформированной записи
	ASSERT_EQ(result, "2025-04-06T12:37:01.520Z");
	// Выполняем проверку того, что запись оканчивается обозначением нулевого смещения
	ASSERT_EQ(result.back(), 'Z');
	// Формируем запись того же момента времени в зоне с некратным часу смещением
	ASSERT_EQ(this->_chrono->format(1743943021520, 19800, ZULU_FORMAT), "2025-04-06T18:07:01.520+05:30");
	// Выполняем проверку разбора обеих разновидностей поля смещения
	ASSERT_EQ(this->_chrono->parse("2025-04-06T12:37:01.520Z", ZULU_FORMAT), 1743943021520);
	ASSERT_EQ(this->_chrono->parse("2025-04-06T12:37:01.520+00:00", ZULU_FORMAT), 1743943021520);
	ASSERT_EQ(this->_chrono->parse("2025-04-06T18:07:01.520+05:30", ZULU_FORMAT), 1743943021520);
}

/**
 * @brief Тест разбора штампа времени в локальном хранилище даты
 *
 * @details Хранилище задаётся отдельным доводом метода разбора и переключает его на
 *          другую ветку обработки переменных формата. Ветка эта разбиралась не со
 *          всеми переменными обозначения зоны: переменная %o в ней не значилась
 *          вовсе, и разбор записи с нею обрывался на месте смещения
 *
 */
TEST_F(SyslogFixture, ExecutionSyslogStorageChronoTest){
	// Выполняем перебор всех переменных обозначения временной зоны
	for(auto & specifier : {"%o", "%z", "%Z", "%i"}){
		// Формируем несущий формат записи с проверяемой переменной
		const std::string format = (std::string("%Y-%m-%dT%H:%M:%S.%s") + specifier);
		// Формируем запись даты в нулевой временной зоне
		const std::string date = this->_chrono->format(1743943021520, 0, format);
		// Выполняем проверку разбора записи в локальном хранилище даты
		ASSERT_EQ(this->_chrono->parse(date, format, awh::chrono_t::storage_t::LOCAL), 1743943021520)
			<< date << " (" << specifier << ")";
	}
}

/**
 * @brief Тест подстановки года в штамп времени стандарта RFC 3164
 *
 * @details Устаревший стандарт года не записывает, и получатель обязан подставить его
 *          сам. Модуль подставляет текущий год, а если получившаяся дата уходит вперёд
 *          дальше допуска - предыдущий: записи системного журнала описывают прошедшее,
 *          и декабрьская запись, прочитанная в январе, относится к минувшему году
 *
 */
TEST_F(SyslogFixture, ExecutionSyslogYearChronoTest){
	// Получаем текущий момент времени
	const uint64_t now = this->_chrono->timestamp(awh::chrono_t::type_t::MILLISECONDS);
	// Выполняем перебор смещений от текущего момента в обе стороны от него
	for(int32_t days = -300; days <= 300; days += 15){
		// Получаем смещённый момент времени
		const uint64_t date = static_cast <uint64_t> (static_cast <int64_t> (now) + (static_cast <int64_t> (days) * 86400000));
		// Формируем запись штампа времени в формате устаревшего стандарта
		const std::string result = this->_chrono->format(date, BSD_FORMAT);
		// Выполняем разбор записи, года не содержащей
		const uint64_t parsed = this->_chrono->parse(result, BSD_FORMAT);
		// Выполняем проверку того, что подставленный год не отнёс запись в будущее
		ASSERT_LE(parsed, now + 86400000) << result;
		/**
		 * Запись прошедшего момента обязана прочитаться тем же моментом с точностью
		 * до секунды: год подставляется текущий, и он же был у записанного момента
		 */
		if(days <= 0)
			// Выполняем проверку совпадения прочитанного момента с записанным
			ASSERT_EQ(parsed, ((date / 1000) * 1000)) << result;
	}
}

/**
 * @brief Тест допуска отката года
 *
 * @details Допуск задаёт, насколько далеко вперёд может отстоять запись, года не
 *          содержащая, прежде чем разбор отнесёт её к предыдущему году. Величина по
 *          умолчанию - двадцать шесть часов - равна полному разбросу временных зон,
 *          от UTC+14 до UTC-12. Устаревший стандарт временной зоны в записи не
 *          указывает: штамп содержит местное время отправителя, а читается он в зоне
 *          получателя, поэтому запись законно опережает получателя вплоть до этой
 *          величины. До введения допуска откат срабатывал при опережении на любую
 *          величину, и каждая запись хоста, стоящего восточнее получателя, уезжала
 *          на год назад
 *
 */
TEST_F(SyslogFixture, ExecutionSyslogRollbackChronoTest){
	// Выполняем проверку величины допуска по умолчанию
	ASSERT_EQ(this->_chrono->yearRollback(), 26 * 3600);
	// Получаем текущий момент времени
	const uint64_t now = this->_chrono->timestamp(awh::chrono_t::type_t::MILLISECONDS);
	// Получаем текущее значение года
	const uint16_t year = this->_chrono->year(now);
	/**
	 * Выполняем перебор опережений, укладывающихся в допуск по умолчанию
	 */
	for(uint64_t hours : {0, 1, 12, 20, 25}){
		// Формируем запись штампа времени опережающего момента
		const std::string record = this->_chrono->format(now + (hours * 3600000), BSD_FORMAT);
		// Выполняем проверку того, что год остался текущим
		ASSERT_EQ(this->_chrono->year(this->_chrono->parse(record, BSD_FORMAT)), year)
			<< record << " (опережение " << hours << " ч)";
	}
	/**
	 * Выполняем перебор опережений, допуск превышающих
	 */
	for(uint64_t hours : {27, 40, 24 * 30}){
		// Формируем запись штампа времени опережающего момента
		const std::string record = this->_chrono->format(now + (hours * 3600000), BSD_FORMAT);
		// Выполняем проверку того, что год откатился на предыдущий
		ASSERT_EQ(this->_chrono->year(this->_chrono->parse(record, BSD_FORMAT)), year - 1)
			<< record << " (опережение " << hours << " ч)";
	}
}

/**
 * @brief Тест настройки допуска отката года
 *
 * @details Величина допуска подбирается под размещение хостов: одному датацентру в
 *          одной временной зоне разброс зон не нужен, флоту по всему свету нужен
 *          полный. Нулевой допуск отключает откат целиком
 *
 */
TEST_F(SyslogFixture, ExecutionSyslogRollbackSetupChronoTest){
	// Получаем текущий момент времени
	const uint64_t now = this->_chrono->timestamp(awh::chrono_t::type_t::MILLISECONDS);
	// Получаем текущее значение года
	const uint16_t year = this->_chrono->year(now);
	// Формируем запись штампа времени, опережающего текущий момент на двадцать часов
	const std::string record = this->_chrono->format(now + (20 * 3600000), BSD_FORMAT);
	// Устанавливаем допуск в один час, разброса зон не покрывающий
	this->_chrono->yearRollback(3600);
	// Выполняем проверку величины установленного допуска
	ASSERT_EQ(this->_chrono->yearRollback(), 3600);
	// Выполняем проверку того, что запись уехала на год назад
	ASSERT_EQ(this->_chrono->year(this->_chrono->parse(record, BSD_FORMAT)), year - 1) << record;
	// Устанавливаем допуск с запасом на разъехавшиеся часы
	this->_chrono->yearRollback(30 * 3600);
	// Выполняем проверку того, что год остался текущим
	ASSERT_EQ(this->_chrono->year(this->_chrono->parse(record, BSD_FORMAT)), year) << record;
	// Отключаем откат года целиком
	this->_chrono->yearRollback(0);
	// Выполняем проверку величины установленного допуска
	ASSERT_EQ(this->_chrono->yearRollback(), 0);
	// Выполняем проверку того, что год остался текущим при любом опережении
	ASSERT_EQ(this->_chrono->year(this->_chrono->parse(record, BSD_FORMAT)), year) << record;
	// Формируем запись штампа времени, опережающего текущий момент на месяц
	const std::string far = this->_chrono->format(now + (24 * 30 * 3600000ULL), BSD_FORMAT);
	// Выполняем проверку того, что откат не сработал и на нём
	ASSERT_EQ(this->_chrono->year(this->_chrono->parse(far, BSD_FORMAT)), year) << far;
}

/**
 * @brief Тест опознания записи прошедшего года
 *
 * @details Ради этого случая откат и введён: декабрьская запись, прочитанная в
 *          январе, отстоит от текущего момента почти на год вперёд и обязана быть
 *          отнесена к минувшему году. Случай воспроизводится через локальное
 *          хранилище даты: часы объекта переводятся на январь, и запись разбирается
 *          относительно них
 *
 * @note Разбор в локальном хранилище не только читает внутренний объект даты, но и
 *       записывает в него разобранный результат, поэтому часы приходится
 *       переводить заново перед каждой проверкой: иначе вторая из них считала бы
 *       текущим моментом результат первой
 *
 */
TEST_F(SyslogFixture, ExecutionSyslogNewYearChronoTest){
	// Получаем момент, на который переводятся часы локального хранилища
	const uint64_t january = this->_chrono->parse("2026-01-05 10:00:00", "%Y-%m-%d %H:%M:%S");
	// Переводим часы локального хранилища на пятое января 2026 года
	this->_chrono->timestamp(january, awh::chrono_t::type_t::MILLISECONDS);
	// Выполняем проверку того, что декабрьская запись отнесена к минувшему году
	ASSERT_EQ(
		this->_chrono->format(
			this->_chrono->parse("Dec 31 23:00:00", BSD_FORMAT, awh::chrono_t::storage_t::LOCAL),
			"%Y-%m-%d"
		), "2025-12-31"
	);
	// Возвращаем часы локального хранилища на исходный момент
	this->_chrono->timestamp(january, awh::chrono_t::type_t::MILLISECONDS);
	// Выполняем проверку того, что январская запись осталась в текущем году
	ASSERT_EQ(
		this->_chrono->format(
			this->_chrono->parse("Jan  4 23:00:00", BSD_FORMAT, awh::chrono_t::storage_t::LOCAL),
			"%Y-%m-%d"
		), "2026-01-04"
	);
}
