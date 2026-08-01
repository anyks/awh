/**
 * @file: semantics.cpp
 * @date: 2026-07-31
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Тесты договорённостей модуля — начало эпохи как законная дата, очистка
 *        локальных данных, метка времени суток перед часом, ширина номера недели,
 *        доля миллисекунды, согласованность местного хранилища с временной зоной
 *        и намеренные решения, правке не подлежащие
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include "chrono.hpp"

/**
 * @brief Тест начала эпохи как законной даты
 *
 * @details Нулевой штамп времени - это первое января 1970 года, а не признак
 *          отсутствия даты: методы разложения календаря считали его пустым доводом
 *          и выдавали ноль либо оставляли объект даты нетронутым
 *
 */
TEST_F(ChronoFixture, ExecutionEpochStampChronoTest){
	// Первое января 1970 года приходится на четверг
	ASSERT_EQ(this->_chrono->format(static_cast <uint64_t> (0), "%a %Y-%m-%d %H:%M:%S"), "Thu 1970-01-01 00:00:00");
	// Начало суток начала эпохи - она сама
	ASSERT_EQ(this->_chrono->begin(static_cast <uint64_t> (0), awh::chrono_t::type_t::DAY), static_cast <uint64_t> (0));
	// Конец суток начала эпохи отстоит от неё на сутки
	ASSERT_EQ(this->_chrono->end(static_cast <uint64_t> (0), awh::chrono_t::type_t::DAY), static_cast <uint64_t> (86400000));
	// Первая миллисекунда эпохи лежит в тех же сутках
	ASSERT_EQ(this->_chrono->end(static_cast <uint64_t> (1), awh::chrono_t::type_t::DAY), static_cast <uint64_t> (86400000));
	// Начало эпохи приходится на невисокосный год
	ASSERT_FALSE(this->_chrono->leap(static_cast <uint64_t> (0)));
	// Год начала эпохи - 1970-й
	ASSERT_EQ(this->_chrono->year(static_cast <uint64_t> (0)), static_cast <uint16_t> (1970));
}

/**
 * @brief Тест установки начала эпохи в локальное хранилище
 *
 */
TEST_F(ChronoFixture, ExecutionEpochStorageChronoTest){
	// Выполняем установку начала эпохи
	this->_chrono->timestamp(static_cast <uint64_t> (0), awh::chrono_t::type_t::MILLISECONDS);
	// Установленный штамп времени обязан читаться обратно
	ASSERT_EQ(
		this->_chrono->timestamp(awh::chrono_t::type_t::MILLISECONDS, awh::chrono_t::storage_t::LOCAL),
		static_cast <uint64_t> (0)
	);
	// Выполняем установку вторых суток эпохи
	this->_chrono->timestamp(static_cast <uint64_t> (86400000), awh::chrono_t::type_t::MILLISECONDS);
	// Установленный штамп времени обязан читаться обратно
	ASSERT_EQ(
		this->_chrono->timestamp(awh::chrono_t::type_t::MILLISECONDS, awh::chrono_t::storage_t::LOCAL),
		static_cast <uint64_t> (86400000)
	);
}

/**
 * @brief Тест очистки локальных данных
 *
 * @details Очистка локальных данных возвращает объект даты к текущему моменту, но
 *          реестр временных зон не трогает: его очищает отдельный метод
 *
 */
TEST_F(ChronoFixture, ExecutionClearChronoTest){
	// Выполняем добавление временной зоны в реестр
	this->_chrono->addTimeZone("XYZ", 12345);
	// Выполняем установку временной зоны, отличной от зоны окружения
	this->_chrono->setTimeZone("MSK");
	// Выполняем очистку локальных данных
	this->_chrono->clear();
	// Реестр временных зон обязан пережить очистку локальных данных
	ASSERT_EQ(this->_chrono->getTimeZone("XYZ"), 12345);
	// Поля даты раскладываются в нулевой зоне, поэтому смещение сбрасывается вместе с ними
	ASSERT_EQ(this->_chrono->getTimeZone(awh::chrono_t::storage_t::LOCAL), 0);
	// Выполняем очистку реестра временных зон
	this->_chrono->clearTimeZones();
	// Реестр временных зон обязан опустеть
	ASSERT_EQ(this->_chrono->getTimeZone("XYZ"), this->_chrono->getTimeZone(awh::chrono_t::storage_t::LOCAL));
}

/**
 * @brief Тест метки времени суток, стоящей перед часом
 *
 * @details Метка времени суток приводит час к суточному счёту после разбора всей
 *          записи: приведение в момент разбора метки терялось, когда метка стояла
 *          в записи перед часом
 *
 */
TEST_F(ChronoFixture, ExecutionMeridiemOrderChronoTest){
	// Метка после часа
	ASSERT_EQ(this->_chrono->format(this->_chrono->parse("03 PM", "%I %p"), "%H:%M"), "15:00");
	// Метка перед часом
	ASSERT_EQ(this->_chrono->format(this->_chrono->parse("PM 03", "%p %I"), "%H:%M"), "15:00");
	// Полночь записывается двенадцатым часом с меткой AM
	ASSERT_EQ(this->_chrono->format(this->_chrono->parse("AM 12", "%p %I"), "%H:%M"), "00:00");
	// Полдень записывается двенадцатым часом с меткой PM
	ASSERT_EQ(this->_chrono->format(this->_chrono->parse("PM 12", "%p %I"), "%H:%M"), "12:00");
	// Час без метки времени суток остаётся нетронутым
	ASSERT_EQ(this->_chrono->format(this->_chrono->parse("12", "%H"), "%H:%M"), "12:00");
	// Время двенадцатичасовой записью разбирается целиком
	ASSERT_EQ(this->_chrono->format(this->_chrono->parse("03:04:05 PM", "%r"), "%H:%M:%S"), "15:04:05");
}

/**
 * @brief Тест ширины номера недели в году
 *
 * @details Номер недели в году занимает не больше двух разрядов: правило
 *          неограниченной длины съедало разряды следующего поля записи
 *
 */
TEST_F(ChronoFixture, ExecutionWeekNumberWidthChronoTest){
	// Номер недели, отделённый от года пробелом
	ASSERT_EQ(this->_chrono->format(this->_chrono->parse("2025 15", "%Y %W"), "%Y"), "2025");
	// Номер недели, стоящий вплотную к году
	ASSERT_EQ(this->_chrono->format(this->_chrono->parse("152025", "%W%Y"), "%Y"), "2025");
	// Номер недели, отсчитываемый от воскресенья
	ASSERT_EQ(this->_chrono->format(this->_chrono->parse("102025", "%U%Y"), "%Y"), "2025");
}

/**
 * @brief Тест согласованности местного хранилища при ненулевой временной зоне
 *
 * @details Разбор в местное хранилище обязан дать тот же штамп времени, что и разбор
 *          в глобальное, а чтение штампа времени из местного хранилища - повторить
 *          разобранный. Прежде поля объекта после разбора перекладывались в нулевую
 *          зону, а смещение оставалось от записи, и чтение сдвигалось на величину зоны
 *
 */
TEST_F(ChronoFixture, ExecutionLocalStorageZoneChronoTest){
	/**
	 * Выполняем перебор записей с явно указанной временной зоной
	 */
	for(const char * date : {"2025-04-06T12:37:01+0300", "2025-04-06T12:37:01-0330", "2025-04-06T12:37:01+0000"}){
		// Выполняем разбор записи в местное хранилище
		const uint64_t local = this->_chrono->parse(date, "%Y-%m-%dT%H:%M:%S%z", awh::chrono_t::storage_t::LOCAL);
		// Выполняем разбор записи в глобальное хранилище
		const uint64_t global = this->_chrono->parse(date, "%Y-%m-%dT%H:%M:%S%z", awh::chrono_t::storage_t::GLOBAL);
		// Хранилище на разобранный штамп времени влиять не должно
		ASSERT_EQ(local, global) << date;
		// Разобранный штамп времени обязан читаться из местного хранилища обратно
		ASSERT_EQ(
			this->_chrono->timestamp(awh::chrono_t::type_t::MILLISECONDS, awh::chrono_t::storage_t::LOCAL),
			local
		) << date;
		// Запись обязана восстанавливаться из местного хранилища без потерь
		ASSERT_EQ(
			this->_chrono->format("%Y-%m-%dT%H:%M:%S%z", awh::chrono_t::storage_t::LOCAL),
			date
		) << date;
	}
}

/**
 * @brief Тест отката года при ненулевой временной зоне
 *
 * @details Запись без года, уходящая вперёд дальше допуска, относится к предыдущему
 *          году: прежде откат в местном хранилище собирал штамп времени по полям
 *          нулевой зоны со смещением записи и сдвигал время на величину зоны
 *
 */
TEST_F(ChronoFixture, ExecutionYearRollbackZoneChronoTest){
	// Получаем момент времени, отстоящий от текущего на полгода вперёд
	const uint64_t date = (this->_chrono->timestamp(awh::chrono_t::type_t::MILLISECONDS) + (182ULL * 86400000ULL));
	/**
	 * Выполняем перебор смещений временных зон
	 */
	for(const int32_t zone : {10800, -12600, 0}){
		// Формируем запись системного журнала без года в проверяемой временной зоне
		const std::string record = this->_chrono->format(date, zone, "%b %e %H:%M:%S %z");
		// Выполняем разбор записи в местное хранилище
		const uint64_t local = this->_chrono->parse(record, "%b %e %H:%M:%S %z", awh::chrono_t::storage_t::LOCAL);
		// Выполняем разбор записи в глобальное хранилище
		const uint64_t global = this->_chrono->parse(record, "%b %e %H:%M:%S %z", awh::chrono_t::storage_t::GLOBAL);
		// Хранилище на разобранный штамп времени влиять не должно
		ASSERT_EQ(local, global) << record;
		// Запись без года отнесена к предыдущему году, время суток при этом сохранено
		ASSERT_EQ(this->_chrono->format(local, zone, "%H:%M:%S"), this->_chrono->format(date, zone, "%H:%M:%S")) << record;
		// Разобранный штамп времени обязан читаться из местного хранилища обратно
		ASSERT_EQ(
			this->_chrono->timestamp(awh::chrono_t::type_t::MILLISECONDS, awh::chrono_t::storage_t::LOCAL),
			local
		) << record;
	}
}

/**
 * @brief Тест повторной установки временной зоны
 *
 * @details Установка временной зоны момент времени менять не должна, меняется лишь
 *          зона, в которой записаны поля объекта даты: прежде каждая установка
 *          сдвигала дату ещё раз
 *
 */
TEST_F(ChronoFixture, ExecutionSetTimeZoneChronoTest){
	// Выполняем установку штампа времени
	const uint64_t date = 1743943021520;
	// Выполняем установку даты
	this->_chrono->timestamp(date, awh::chrono_t::type_t::MILLISECONDS);
	/**
	 * Выполняем перебор временных зон
	 */
	for(const char * zone : {"MSK", "UTC", "MSK", "YEKT"}){
		// Выполняем установку временной зоны
		this->_chrono->setTimeZone(zone);
		// Момент времени от смены временной зоны меняться не должен
		ASSERT_EQ(
			this->_chrono->timestamp(awh::chrono_t::type_t::MILLISECONDS, awh::chrono_t::storage_t::LOCAL),
			date
		) << zone;
		// Установленная временная зона обязана читаться обратно
		ASSERT_EQ(this->_chrono->getTimeZone(awh::chrono_t::storage_t::LOCAL), this->_chrono->getTimeZone(zone)) << zone;
	}
}

/**
 * @brief Тест доли миллисекунды
 *
 * @details Поля доли миллисекунды хранят остаток, а не полный штамп времени:
 *          прежде единицы измерения устанавливаемого значения угадывались по
 *          количеству разрядов с оглядкой на текущий момент
 *
 */
TEST_F(ChronoFixture, ExecutionFractionChronoTest){
	// Выполняем установку доли миллисекунды в наносекундах
	this->_chrono->set(static_cast <uint64_t> (123456), awh::chrono_t::unit_t::NANOSECONDS);
	// Установленная доля обязана читаться обратно
	ASSERT_EQ(
		this->_chrono->get <uint64_t> (awh::chrono_t::unit_t::NANOSECONDS, awh::chrono_t::storage_t::LOCAL),
		static_cast <uint64_t> (123456)
	);
	// Выполняем установку доли миллисекунды в микросекундах
	this->_chrono->set(static_cast <uint64_t> (789), awh::chrono_t::unit_t::MICROSECONDS);
	// Установленная доля обязана читаться обратно
	ASSERT_EQ(
		this->_chrono->get <uint64_t> (awh::chrono_t::unit_t::MICROSECONDS, awh::chrono_t::storage_t::LOCAL),
		static_cast <uint64_t> (789)
	);
	// Полный штамп времени в наносекундах обязан дать ту же долю миллисекунды
	this->_chrono->set(static_cast <uint64_t> (1743943021520123456ULL), awh::chrono_t::unit_t::NANOSECONDS);
	// Установленная доля обязана читаться обратно
	ASSERT_EQ(
		this->_chrono->get <uint64_t> (awh::chrono_t::unit_t::NANOSECONDS, awh::chrono_t::storage_t::LOCAL),
		static_cast <uint64_t> (123456)
	);
}

/**
 * @brief Тест намеренных решений модуля
 *
 * @details Перечисленное здесь выглядит несообразностью, но выбрано осознанно и
 *          описано в разделе намеренных решений заголовочного файла. Тест закрепляет
 *          эти решения, чтобы очередной разбор кода не переделал их молча
 *
 * @see chrono_decisions
 *
 */
TEST_F(ChronoFixture, ExecutionDecisionsChronoTest){
	// Переменные %G и %g обозначают календарный год, а не год недельной нумерации ISO 8601
	for(const char * date : {"2010-01-01", "2019-12-30", "2025-04-06"}){
		// Выполняем разбор проверяемой записи
		const uint64_t value = this->_chrono->parse(date, "%Y-%m-%d");
		// Год недельной нумерации дал бы здесь 2009 и 2020 соответственно
		ASSERT_EQ(this->_chrono->format(value, "%G"), this->_chrono->format(value, "%Y")) << date;
		// Двузначное обозначение года ведёт себя так же
		ASSERT_EQ(this->_chrono->format(value, "%g"), this->_chrono->format(value, "%y")) << date;
	}
	// Переменные %U и %W при разборе дают одинаковый результат, как того требует POSIX
	ASSERT_EQ(this->_chrono->parse("2025 10", "%Y %U"), this->_chrono->parse("2025 10", "%Y %W"));
	// Единица unit_t::WEEKS округляется до ближайшего целого
	{
		// 2025-04-06 - 95-й прошедший день года, 95/7 = 13.57
		const uint64_t value = this->_chrono->parse("2025-04-06", "%Y-%m-%d");
		// Отбрасывание неполной недели дало бы здесь 13
		ASSERT_EQ(this->_chrono->get <uint8_t> (value, awh::chrono_t::unit_t::WEEKS), 14);
		// Номер недели в году - величина иная, от единицы unit_t::WEEKS отличная
		ASSERT_EQ(this->_chrono->format(value, "%W"), "13");
	}
	// Смещение на микросекунды и наносекунды меняет размерность ответа
	{
		// Выполняем установку проверяемого штампа времени
		const uint64_t value = 1743943021000;
		// Единицы крупнее микросекунды размерность сохраняют
		ASSERT_EQ(
			this->_chrono->offset(value, 1, awh::chrono_t::type_t::SECONDS, awh::chrono_t::offset_t::INCREMENT),
			static_cast <uint64_t> (1743943022000)
		);
		// Смещение на микросекунду выдаёт микросекундный штамп времени
		ASSERT_EQ(
			this->_chrono->offset(value, 1, awh::chrono_t::type_t::MICROSECONDS, awh::chrono_t::offset_t::INCREMENT),
			static_cast <uint64_t> (1743943021000001)
		);
		// Смещение на наносекунду выдаёт наносекундный штамп времени
		ASSERT_EQ(
			this->_chrono->offset(value, 1, awh::chrono_t::type_t::NANOSECONDS, awh::chrono_t::offset_t::INCREMENT),
			static_cast <uint64_t> (1743943021000000001)
		);
	}
	// Нулевой месяц переполнения не даёт, а пригодной запись не считается
	ASSERT_EQ(this->_chrono->parse("2025-00-15", "%Y-%m-%d"), this->_chrono->parse("2025-01-15", "%Y-%m-%d"));
	// Пригодной запись с нулевым месяцем не является
	ASSERT_FALSE(this->_chrono->validate("2025-00-15", "%Y-%m-%d"));
}

/**
 * @brief Тест границ лет всего промежутка представимости
 *
 * @details Год извлекается из штампа времени приблизительной оценкой на long double
 *          с последующим уточнением: тест проходит по каждой границе лет промежутка
 *          и проверяет, что последняя миллисекунда уходящего года и первая
 *          миллисекунда наступающего относятся к разным годам
 *
 */
TEST_F(ChronoFixture, ExecutionYearBoundaryChronoTest){
	/**
	 * Выполняем перебор всех границ лет промежутка представимости
	 */
	for(uint16_t year = 1971; year <= 9999; year++){
		// Формируем запись начала проверяемого года
		const std::string date = (std::to_string(year) + "-01-01T00:00:00");
		// Получаем штамп времени начала проверяемого года
		const uint64_t value = this->_chrono->parse(date, "%Y-%m-%dT%H:%M:%S");
		// Первая миллисекунда года относится к нему самому
		ASSERT_EQ(this->_chrono->year(value), year) << date;
		// Последняя миллисекунда предыдущего года относится к предыдущему году
		ASSERT_EQ(this->_chrono->year(value - 1), static_cast <uint16_t> (year - 1)) << date;
	}
}

/**
 * @brief Тест начала недели у начала эпохи
 *
 * @details Неделя, на которую приходится начало эпохи, начинается 29 декабря 1969
 *          года и календарём не представима: начало такой недели приводится к началу
 *          эпохи. Прежде вычитание уходило за ноль и беззнаковый оборот давал начало
 *          недели где-то в 584-миллионном году
 *
 */
TEST_F(ChronoFixture, ExecutionEpochWeekChronoTest){
	/**
	 * Выполняем перебор первых суток эпохи, до первого её понедельника
	 */
	for(uint64_t day = 0; day < 4; day++){
		// Получаем штамп времени проверяемых суток
		const uint64_t date = (day * 86400000);
		// Начало недели приводится к началу эпохи
		ASSERT_EQ(this->_chrono->begin(date, awh::chrono_t::type_t::WEEK), static_cast <uint64_t> (0)) << day;
		// Конец недели отстоит от её начала ровно на неделю
		ASSERT_EQ(this->_chrono->end(date, awh::chrono_t::type_t::WEEK), static_cast <uint64_t> (604800000)) << day;
	}
	// Первый понедельник эпохи - 5 января 1970 года
	ASSERT_EQ(this->_chrono->format(static_cast <uint64_t> (4 * 86400000), "%Y-%m-%d %a"), "1970-01-05 Mon");
	// Начало недели первого понедельника эпохи - он сам
	ASSERT_EQ(
		this->_chrono->begin(static_cast <uint64_t> (4 * 86400000), awh::chrono_t::type_t::WEEK),
		static_cast <uint64_t> (4 * 86400000)
	);
}

/**
 * @brief Тест опорного момента отката года при выставленной временной зоне
 *
 * @details Опорный момент, относительно которого опознаётся запись прошлого года,
 *          снимается до сброса временной зоны: прежде сборка штампа времени после
 *          обнуления смещения сдвигала опорный момент на величину зоны, отчего порог
 *          отката года смещался на столько же
 *
 */
TEST_F(ChronoFixture, ExecutionRollbackReferenceChronoTest){
	// Получаем допуск отката года в миллисекундах
	const uint64_t rollback = (static_cast <uint64_t> (this->_chrono->yearRollback()) * 1000);
	// Получаем смещение временной зоны, выставляемой объекту
	const int32_t zone = this->_chrono->getTimeZone("MSK");
	/**
	 * Выполняем перебор сдвигов вокруг порога отката года: записи до порога относятся
	 * к текущему году, записи за порогом - к предыдущему
	 */
	for(const int64_t shift : {-10800000LL, -3600000LL, -600000LL, 600000LL, 3600000LL, 10800000LL}){
		// Объект местного хранилища с выставленной временной зоной
		awh::chrono_t local(this->_fmk.get(), this->_log.get());
		// Выполняем установку временной зоны, отличной от нулевой
		local.setTimeZone("MSK");
		// Получаем текущий момент времени
		const uint64_t now = this->_chrono->timestamp(awh::chrono_t::type_t::MILLISECONDS);
		// Получаем момент времени, отстоящий от порога отката на проверяемый сдвиг
		const uint64_t date = static_cast <uint64_t> (static_cast <int64_t> (now + rollback) + shift);
		/**
		 * Запись формируется в той же временной зоне, в которой объект её прочтёт:
		 * года запись не содержит, а секунды - наименьшая её подробность
		 */
		const std::string record = this->_chrono->format(date, zone, "%b %e %H:%M:%S");
		// Выполняем разбор записи в местное хранилище
		const uint64_t value = local.parse(record, "%b %e %H:%M:%S", awh::chrono_t::storage_t::LOCAL);
		// Если запись до порога отката, она относится к текущему году
		if(shift < 0)
			// Разобранный момент времени совпадает с исходным с точностью до секунды
			ASSERT_EQ(value, (date - (date % 1000))) << record;
		// Если запись за порогом отката, она относится к предыдущему году
		else ASSERT_LT(value, date) << record;
	}
}

/**
 * @brief Тест приведения полей объекта к разобранному штампу времени
 *
 * @details Запись способна нести поля вне промежутка - нулевой либо тринадцатый
 *          месяц, девятый день недели, - а объект обязан описывать ту же дату, что
 *          выдал разбор. Прежде такие поля оставались в объекте сырыми, и
 *          формирование записи читало названия месяцев и дней недели за границами
 *          своих таблиц
 *
 */
TEST_F(ChronoFixture, ExecutionFieldRangeChronoTest){
	// Нулевой месяц равнозначен первому
	{
		// Выполняем разбор записи с нулевым месяцем
		const uint64_t date = this->_chrono->parse("2025-00-15", "%Y-%m-%d", awh::chrono_t::storage_t::LOCAL);
		// Разбор относит запись к январю
		ASSERT_EQ(this->_chrono->format(date, 0, "%Y-%m-%d"), "2025-01-15");
		// Поля объекта описывают ту же дату, что выдал разбор
		ASSERT_EQ(this->_chrono->get <uint8_t> (awh::chrono_t::unit_t::MONTH, awh::chrono_t::storage_t::LOCAL), 1);
		// Название месяца читается из таблицы, а не за её границей
		ASSERT_EQ(this->_chrono->format("%B", awh::chrono_t::storage_t::LOCAL), "January");
	}
	// Тринадцатый месяц переносит запись в следующий год
	{
		// Выполняем разбор записи с тринадцатым месяцем
		const uint64_t date = this->_chrono->parse("2025-13-15", "%Y-%m-%d", awh::chrono_t::storage_t::LOCAL);
		// Разбор относит запись к январю следующего года
		ASSERT_EQ(this->_chrono->format(date, 0, "%Y-%m-%d"), "2026-01-15");
		// Поля объекта описывают ту же дату, что выдал разбор
		ASSERT_EQ(this->_chrono->get <uint8_t> (awh::chrono_t::unit_t::MONTH, awh::chrono_t::storage_t::LOCAL), 1);
		// Название месяца читается из таблицы, а не за её границей
		ASSERT_EQ(this->_chrono->format("%B", awh::chrono_t::storage_t::LOCAL), "January");
	}
	// Номер дня недели вне промежутка приводится к дню разобранной даты
	{
		// Выполняем разбор записи с девятым днём недели
		this->_chrono->parse("9", "%u", awh::chrono_t::storage_t::LOCAL);
		// Номер дня недели лежит в своём промежутке
		const uint8_t day = this->_chrono->get <uint8_t> (awh::chrono_t::unit_t::DAY, awh::chrono_t::storage_t::LOCAL);
		// Номер дня недели обязан лежать от одного до семи
		ASSERT_GE(day, 1);
		// Номер дня недели обязан лежать от одного до семи
		ASSERT_LE(day, 7);
		// Название дня недели совпадает с днём недели разобранной даты
		ASSERT_EQ(
			this->_chrono->format("%A", awh::chrono_t::storage_t::LOCAL),
			this->_chrono->format(this->_chrono->timestamp(awh::chrono_t::type_t::MILLISECONDS, awh::chrono_t::storage_t::LOCAL), 0, "%A")
		);
	}
}

/**
 * @brief Тест смещения, указанного за названием временной зоны
 *
 * @details Смещение накладывается независимо от того, откуда взято само название:
 *          прежде наложение стояло в ветке поиска по таблице известных зон, и своя
 *          зона из реестра хвост обозначения теряла
 *
 */
TEST_F(ChronoFixture, ExecutionZoneOffsetTailChronoTest){
	// Выполняем добавление своей временной зоны в реестр
	this->_chrono->addTimeZone("ANYKS", 9839);
	// Своя зона без хвоста даёт своё смещение
	ASSERT_EQ(this->_chrono->getTimeZone("ANYKS"), 9839);
	// Своя зона с хвостом смещение накладывает
	ASSERT_EQ(this->_chrono->getTimeZone("ANYKS+1"), (9839 + 3600));
	// Запись хвоста с двоеточием равнозначна записи без него
	ASSERT_EQ(this->_chrono->getTimeZone("ANYKS+01:00"), (9839 + 3600));
	// Известная зона ведёт себя так же
	ASSERT_EQ(this->_chrono->getTimeZone("MSK+1"), (10800 + 3600));
	// Известная зона без хвоста даёт своё смещение
	ASSERT_EQ(this->_chrono->getTimeZone("MSK"), 10800);
}

/**
 * @brief Тест обозначений времени в Западной Гренландии
 *
 * @details Стандартное время в Западной Гренландии обозначается WGT и отстаёт от
 *          нулевого пояса на три часа, летнее - WGST и на два: обозначение WGST
 *          относится именно к летнему времени, а WGT прежде в таблице отсутствовало
 *
 */
TEST_F(ChronoFixture, ExecutionGreenlandZoneChronoTest){
	// Стандартное время в Западной Гренландии
	ASSERT_EQ(this->_chrono->getTimeZone("WGT"), -10800);
	// Летнее время в Западной Гренландии
	ASSERT_EQ(this->_chrono->getTimeZone("WGST"), -7200);
	// Время в Бруней-Даруссаламе
	ASSERT_EQ(this->_chrono->getTimeZone("BNT"), 28800);
}

/**
 * @brief Тест согласованности долей миллисекунды
 *
 * @details Доли миллисекунды в микросекундах и наносекундах описывают одну и ту же
 *          величину с разной подробностью и обязаны сходиться: врозь они давали
 *          разные мгновения при чтении штампа времени в тех и других единицах
 *
 */
TEST_F(ChronoFixture, ExecutionFractionSyncChronoTest){
	// Выполняем установку штампа времени
	this->_chrono->timestamp(static_cast <uint64_t> (1743943021520), awh::chrono_t::type_t::MILLISECONDS);
	// Выполняем установку доли миллисекунды в наносекундах
	this->_chrono->set(static_cast <uint64_t> (123456), awh::chrono_t::unit_t::NANOSECONDS);
	// Микросекунды обязаны стать старшей частью наносекунд
	ASSERT_EQ(this->_chrono->get <uint64_t> (awh::chrono_t::unit_t::MICROSECONDS, awh::chrono_t::storage_t::LOCAL), 123);
	// Штамп времени в микросекундах обязан сойтись со штампом в наносекундах
	ASSERT_EQ(
		this->_chrono->timestamp(awh::chrono_t::type_t::MICROSECONDS, awh::chrono_t::storage_t::LOCAL),
		(this->_chrono->timestamp(awh::chrono_t::type_t::NANOSECONDS, awh::chrono_t::storage_t::LOCAL) / 1000)
	);
	// Выполняем установку доли миллисекунды в микросекундах
	this->_chrono->set(static_cast <uint64_t> (789), awh::chrono_t::unit_t::MICROSECONDS);
	// Наносекунды обязаны сойтись с микросекундами
	ASSERT_EQ(
		(this->_chrono->get <uint64_t> (awh::chrono_t::unit_t::NANOSECONDS, awh::chrono_t::storage_t::LOCAL) / 1000),
		static_cast <uint64_t> (789)
	);
	// Штамп времени в микросекундах обязан сойтись со штампом в наносекундах
	ASSERT_EQ(
		this->_chrono->timestamp(awh::chrono_t::type_t::MICROSECONDS, awh::chrono_t::storage_t::LOCAL),
		(this->_chrono->timestamp(awh::chrono_t::type_t::NANOSECONDS, awh::chrono_t::storage_t::LOCAL) / 1000)
	);
}

/**
 * @brief Тест смещения временной зоны окружения при действующем летнем времени
 *
 * @details Смещение снимается с разложенного времени, а не с глобальной переменной
 *          timezone: та несёт стандартное смещение зоны и о переходе на летнее время
 *          не знает, отчего в зоне с переходом всё лето смещение отставало на час
 *
 *          Тест закрепляет заодно и сброс запомненного смещения: зоны здесь меняются
 *          в пределах одной секунды, и смещение обязано следовать за каждой сменой
 *
 */
TEST_F(ChronoFixture, ExecutionEnvironmentZoneChronoTest){
	/**
	 * Фикстура закрепляет зону окружения на UTC, где перехода на летнее время нет:
	 * зоны для проверки выставляются здесь и снимаются обратно
	 */
	for(const char * zone : {"America/New_York", "Europe/Berlin", "Australia/Sydney", "Europe/Moscow", "UTC"}){
		// Выполняем установку проверяемой временной зоны окружения
		::setenv("TZ", zone, 1);
		// Устанавливаем временную зону по умолчанию
		::tzset();
		// Создаем структуру времени
		std::tm tm = {};
		// Получаем значение текущего времени
		const time_t value = std::time(nullptr);
		// Заполняем структуру времени
		::localtime_r(&value, &tm);
		// Смещение зоны окружения обязано совпасть со смещением, которое даёт система
		ASSERT_EQ(this->_chrono->getTimeZone(awh::chrono_t::storage_t::GLOBAL), static_cast <int32_t> (tm.tm_gmtoff)) << zone;
	}
	// Возвращаем временную зону окружения, закреплённую фикстурой
	::setenv("TZ", "UTC", 1);
	// Устанавливаем временную зону по умолчанию
	::tzset();
}

/**
 * @brief Тест разбора в местное хранилище без временной зоны в формате
 *
 * @details Запись, зоны не содержащая, читается в той временной зоне, которая
 *          выставлена объекту методом setTimeZone, и лишь при её отсутствии - в зоне
 *          окружения. Прежде выставленная зона разбором отбрасывалась
 *
 */
TEST_F(ChronoFixture, ExecutionLocalStorageDefaultZoneChronoTest){
	/**
	 * Выполняем перебор временных зон, выставляемых объекту
	 */
	for(const char * zone : {"YEKT", "MSK", "EST"}){
		// Объект местного хранилища
		awh::chrono_t chrono(this->_fmk.get(), this->_log.get());
		// Выполняем установку временной зоны
		chrono.setTimeZone(zone);
		// Выполняем разбор записи, временной зоны не содержащей
		const uint64_t value = chrono.parse("2025-04-06 12:00:00", "%Y-%m-%d %H:%M:%S", awh::chrono_t::storage_t::LOCAL);
		// Выставленная объекту временная зона разбор пережить обязана
		ASSERT_EQ(chrono.getTimeZone(awh::chrono_t::storage_t::LOCAL), this->_chrono->getTimeZone(zone)) << zone;
		// Запись прочитана в выставленной временной зоне
		ASSERT_EQ(
			this->_chrono->format(value, this->_chrono->getTimeZone(zone), "%Y-%m-%d %H:%M:%S"),
			"2025-04-06 12:00:00"
		) << zone;
	}
	// Объект без выставленной временной зоны читает запись в зоне окружения
	{
		// Объект местного хранилища
		awh::chrono_t chrono(this->_fmk.get(), this->_log.get());
		// Выполняем разбор записи, временной зоны не содержащей
		chrono.parse("2025-04-06 12:00:00", "%Y-%m-%d %H:%M:%S", awh::chrono_t::storage_t::LOCAL);
		// Временная зона объекта совпадает с зоной окружения
		ASSERT_EQ(
			chrono.getTimeZone(awh::chrono_t::storage_t::LOCAL),
			this->_chrono->getTimeZone(awh::chrono_t::storage_t::GLOBAL)
		);
	}
}

/**
 * @brief Тест номера дня в году вне промежутка
 *
 * @details Номер дня в году проверяется отдельно: сборка штампа времени опирается на
 *          месяц и число месяца, поэтому номер за длиной года до неё не доходил и
 *          запись «2025 999» считалась пригодной
 *
 */
TEST_F(ChronoFixture, ExecutionDayOfYearRangeChronoTest){
	// Первый день года пригоден
	ASSERT_TRUE(this->_chrono->validate("2025 001", "%Y %j"));
	// Последний день невисокосного года пригоден
	ASSERT_TRUE(this->_chrono->validate("2025 365", "%Y %j"));
	// Триста шестьдесят шестой день в невисокосном году непригоден
	ASSERT_FALSE(this->_chrono->validate("2025 366", "%Y %j"));
	// Триста шестьдесят шестой день в високосном году пригоден
	ASSERT_TRUE(this->_chrono->validate("2024 366", "%Y %j"));
	// Номер дня за длиной года непригоден
	ASSERT_FALSE(this->_chrono->validate("2025 999", "%Y %j"));
	// Разбор непригодной записи за промежуток представимости не уходит
	ASSERT_EQ(this->_chrono->format(this->_chrono->parse("2025 999", "%Y %j"), 0, "%Y-%m-%d"), "2025-12-31");
}

/**
 * @brief Тест смещения по годам за промежуток представимости
 *
 * @details Перебор лет останавливается на пределах промежутка: прежде он шёл по всему
 *          доводу, отчего смещение на миллион лет занимало миллион шагов, обозначение
 *          года оборачивалось, а штамп времени уходил за представимый предел
 *
 */
TEST_F(ChronoFixture, ExecutionYearOffsetRangeChronoTest){
	// Выполняем разбор проверяемой даты
	const uint64_t date = this->_chrono->parse("1971-06-15T12:00:00", "%Y-%m-%dT%H:%M:%S");
	// Смещение внутри промежутка представимости выполняется как есть
	ASSERT_EQ(
		this->_chrono->format(this->_chrono->offset(date, 1, awh::chrono_t::type_t::YEAR, awh::chrono_t::offset_t::DECREMENT), 0, "%Y-%m-%d"),
		"1970-06-15"
	);
	/**
	 * Выполняем перебор смещений, уводящих дату ниже начала эпохи
	 */
	for(const uint64_t years : {static_cast <uint64_t> (2), static_cast <uint64_t> (100), static_cast <uint64_t> (1000000)})
		// Дата приводится к началу эпохи
		ASSERT_EQ(this->_chrono->offset(date, years, awh::chrono_t::type_t::YEAR, awh::chrono_t::offset_t::DECREMENT), static_cast <uint64_t> (0)) << years;
	// Выполняем разбор проверяемой даты
	const uint64_t value = this->_chrono->parse("2025-04-06T12:00:00", "%Y-%m-%dT%H:%M:%S");
	// Смещение внутри промежутка представимости выполняется как есть
	ASSERT_EQ(
		this->_chrono->format(this->_chrono->offset(value, 10, awh::chrono_t::type_t::YEAR, awh::chrono_t::offset_t::INCREMENT), 0, "%Y-%m-%d"),
		"2035-04-06"
	);
	/**
	 * Выполняем перебор смещений, уводящих дату выше предела представимости
	 */
	for(const uint64_t years : {static_cast <uint64_t> (10000), static_cast <uint64_t> (1000000)})
		// Дата приводится к последней представимой
		ASSERT_EQ(
			this->_chrono->offset(value, years, awh::chrono_t::type_t::YEAR, awh::chrono_t::offset_t::INCREMENT),
			static_cast <uint64_t> (253402300799999)
		) << years;
}

/**
 * @brief Тест обратного пути обозначений времени в Западной Гренландии
 *
 * @details Обозначения времени в Западной Гренландии однозначны и потому выводятся
 *          названиями, а не смещением: числовую запись получают лишь те зоны, чьё
 *          обозначение занято другой зоной и обратно ведёт не туда
 *
 */
TEST_F(ChronoFixture, ExecutionGreenlandRoundtripChronoTest){
	// Стандартное время в Западной Гренландии выводится своим обозначением
	ASSERT_EQ(this->_chrono->format(awh::chrono_t::zone_t::WGST), "WGT");
	// Летнее время в Западной Гренландии выводится своим обозначением
	ASSERT_EQ(this->_chrono->format(awh::chrono_t::zone_t::WGSTST), "WGST");
	// Обозначение читается обратно в ту же временную зону
	ASSERT_EQ(this->_chrono->getTimeZone(this->_chrono->format(awh::chrono_t::zone_t::WGST)), -10800);
	// Обозначение читается обратно в ту же временную зону
	ASSERT_EQ(this->_chrono->getTimeZone(this->_chrono->format(awh::chrono_t::zone_t::WGSTST)), -7200);
}

/**
 * @brief Тест дня недели на всём промежутке представимости
 *
 * @details День недели определяется количеством суток, прошедших с начала эпохи:
 *          первое января 1970 года - четверг. Прежде он считался по таблице
 *          двадцативосьмилетнего цикла, а цикл этот держится лишь пока високосным
 *          оказывается каждый четвёртый год: правило григорианского календаря, по
 *          которому век високосен только при делимости на 400, цикл разрывает, и с
 *          2100 года день недели уходил на сутки и дальше - неверными оказывались
 *          все 86% промежутка представимости
 *
 */
TEST_F(ChronoFixture, ExecutionDayOfWeekChronoTest){
	// Наибольший штамп времени, представимый календарём модуля
	const uint64_t limit = 253402300799999ULL;
	/**
	 * Опорные даты по обе стороны от каждого векового рубежа: век високосен только
	 * при делимости на 400, и именно на этих рубежах прежний расчёт разъезжался
	 */
	const struct {
		// Проверяемая запись даты
		const char * date;
		// Ожидаемое сокращённое название дня недели
		const char * name;
	} dates[] = {
		{"1970-01-01", "Thu"}, {"2000-01-01", "Sat"}, {"2025-04-06", "Sun"},
		{"2099-12-31", "Thu"}, {"2100-01-01", "Fri"}, {"2200-01-01", "Wed"},
		{"2300-01-01", "Mon"}, {"2400-01-01", "Sat"}, {"9999-12-31", "Fri"}
	};
	/**
	 * Выполняем перебор опорных дат
	 */
	for(const auto & item : dates)
		// День недели опорной даты обязан совпасть с ожидаемым
		ASSERT_EQ(this->_chrono->format(this->_chrono->parse(item.date, "%Y-%m-%d"), 0, "%a"), item.name) << item.date;
	/**
	 * Выполняем сплошную сверку по суткам с шагом, взаимно простым с длиной недели:
	 * такой шаг проходит по всем дням недели, не пропуская ни одного
	 */
	for(uint64_t date = 0; date <= limit; date += (13ULL * 86400000ULL)){
		// Получаем ожидаемый номер дня недели, отсчитываемый от воскресенья
		const uint8_t value = static_cast <uint8_t> (((date / 86400000ULL) + 4ULL) % 7ULL);
		// День недели обязан совпасть с ожидаемым
		ASSERT_EQ(
			this->_chrono->get <uint8_t> (date, awh::chrono_t::unit_t::DAY),
			static_cast <uint8_t> ((value == 0) ? 7 : value)
		) << this->_chrono->format(date, 0, "%Y-%m-%d");
	}
	/**
	 * Выполняем сплошную сверку по суткам вокруг каждого векового рубежа
	 */
	for(uint16_t year = 2000; year <= 9900; year += 100){
		// Формируем запись начала проверяемого года
		const std::string record = (std::to_string(year) + "-01-01");
		// Получаем штамп времени начала проверяемого года
		const uint64_t begin = this->_chrono->parse(record, "%Y-%m-%d");
		/**
		 * Выполняем перебор недели по обе стороны от векового рубежа
		 */
		for(uint64_t date = (begin - (7ULL * 86400000ULL)); date <= (begin + (7ULL * 86400000ULL)); date += 86400000ULL){
			// Получаем ожидаемый номер дня недели, отсчитываемый от воскресенья
			const uint8_t value = static_cast <uint8_t> (((date / 86400000ULL) + 4ULL) % 7ULL);
			// День недели обязан совпасть с ожидаемым
			ASSERT_EQ(
				this->_chrono->get <uint8_t> (date, awh::chrono_t::unit_t::DAY),
				static_cast <uint8_t> ((value == 0) ? 7 : value)
			) << this->_chrono->format(date, 0, "%Y-%m-%d");
		}
	}
}

/**
 * @brief Тест начала недели на всём промежутке представимости
 *
 * @details Начало недели опирается на день недели и разъезжалось вместе с ним
 *
 */
TEST_F(ChronoFixture, ExecutionWeekBeginChronoTest){
	/**
	 * Выполняем перебор дат по обе стороны от вековых рубежей
	 */
	for(const char * date : {"2025-04-06", "2100-01-01", "2100-01-04", "2200-06-15", "2400-02-29"}){
		// Получаем штамп времени проверяемой даты
		const uint64_t value = this->_chrono->parse(date, "%Y-%m-%d");
		// Получаем начало недели проверяемой даты
		const uint64_t begin = this->_chrono->begin(value, awh::chrono_t::type_t::WEEK);
		// Неделя начинается с понедельника
		ASSERT_EQ(this->_chrono->format(begin, 0, "%a"), "Mon") << date;
		// Начало недели не позже самой даты
		ASSERT_LE(begin, value) << date;
		// Начало недели отстоит от даты меньше чем на неделю
		ASSERT_LT((value - begin), static_cast <uint64_t> (604800000)) << date;
	}
}

/**
 * @brief Тест ограничения смещения пределами представимости
 *
 * @details Смещение на единицы постоянной длительности ограничивается пределами
 *          промежутка так же, как смещение на годы и месяцы: прежде произведение
 *          довода на длительность единицы переполнялось, а сумма уходила за предел
 *
 */
TEST_F(ChronoFixture, ExecutionOffsetClampChronoTest){
	// Наибольший штамп времени, представимый календарём модуля
	const uint64_t limit = 253402300799999ULL;
	// Выполняем разбор проверяемой даты
	const uint64_t date = this->_chrono->parse("2025-04-06T12:00:00", "%Y-%m-%dT%H:%M:%S");
	/**
	 * Выполняем перебор единиц постоянной длительности
	 */
	for(const awh::chrono_t::type_t type : {
		awh::chrono_t::type_t::YEAR, awh::chrono_t::type_t::MONTH, awh::chrono_t::type_t::WEEK,
		awh::chrono_t::type_t::DAY, awh::chrono_t::type_t::HOUR, awh::chrono_t::type_t::MINUTES,
		awh::chrono_t::type_t::SECONDS, awh::chrono_t::type_t::MILLISECONDS
	}){
		// Смещение вперёд за предел приводит дату к последней представимой
		ASSERT_EQ(
			this->_chrono->offset(date, 1000000000000000000ULL, type, awh::chrono_t::offset_t::INCREMENT),
			limit
		) << static_cast <uint16_t> (type);
		// Смещение назад за начало эпохи приводит дату к нему самому
		ASSERT_EQ(
			this->_chrono->offset(date, 1000000000000000000ULL, type, awh::chrono_t::offset_t::DECREMENT),
			static_cast <uint64_t> (0)
		) << static_cast <uint16_t> (type);
	}
}

/**
 * @brief Тест наносекундного штампа времени за пределом разрядности
 *
 * @details Наибольший наносекундный штамп времени, умещающийся в разрядность uint64_t,
 *          приходится на 21 июля 2554 года, тогда как календарь модуля доходит до
 *          9999-го: прежде произведение оборачивалось и выдавало дату, не имеющую
 *          отношения к исходной
 *
 */
TEST_F(ChronoFixture, ExecutionNanosecondRangeChronoTest){
	// Наибольшее значение, представимое в разрядности штампа времени
	const uint64_t limit = static_cast <uint64_t> (-1);
	/**
	 * Выполняем перебор дат внутри промежутка, представимого в наносекундах
	 */
	for(const char * date : {"1970-01-01", "2025-04-06", "2554-07-21"}){
		// Получаем штамп времени проверяемой даты
		const uint64_t value = this->_chrono->parse(date, "%Y-%m-%d");
		// Наносекундный штамп времени внутри промежутка выдаётся как есть
		ASSERT_EQ(
			this->_chrono->offset(value, 1, awh::chrono_t::type_t::NANOSECONDS, awh::chrono_t::offset_t::INCREMENT),
			((value * 1000000ULL) + 1ULL)
		) << date;
	}
	/**
	 * Выполняем перебор дат за пределом промежутка, представимого в наносекундах
	 */
	for(const char * date : {"3000-01-01", "9999-12-31"}){
		// Получаем штамп времени проверяемой даты
		const uint64_t value = this->_chrono->parse(date, "%Y-%m-%d");
		// Дата за пределом выдаётся наибольшим представимым значением
		ASSERT_EQ(
			this->_chrono->offset(value, 1, awh::chrono_t::type_t::NANOSECONDS, awh::chrono_t::offset_t::INCREMENT),
			limit
		) << date;
		// Выполняем установку даты за пределом промежутка
		this->_chrono->timestamp(value, awh::chrono_t::type_t::MILLISECONDS);
		// Чтение наносекундного штампа времени выдаёт наибольшее представимое значение
		ASSERT_EQ(this->_chrono->timestamp(awh::chrono_t::type_t::NANOSECONDS, awh::chrono_t::storage_t::LOCAL), limit) << date;
		/**
		 * Перевод в микросекунды подобной границы не имеет и покрывает весь промежуток
		 */
		ASSERT_LT(this->_chrono->timestamp(awh::chrono_t::type_t::MICROSECONDS, awh::chrono_t::storage_t::LOCAL), limit) << date;
	}
}

/**
 * @brief Тест счёта суток внутри месяца
 *
 * @details Считаются целые сутки: неполные в счёт не идут, как и всюду в методе.
 *          Прежде счёт вёлся округлением, отчего до полудня первого числа оставшихся
 *          суток выходило на одни больше, чем их есть
 *
 */
TEST_F(ChronoFixture, ExecutionDaysInMonthChronoTest){
	/**
	 * Выполняем перебор моментов внутри месяца, включая полдень первого числа
	 */
	for(const char * date : {"2025-04-01T00:00:00", "2025-04-01T11:59:00", "2025-04-01T12:00:00",
	                         "2025-04-06T12:37:00", "2025-04-30T23:59:00"}){
		// Получаем штамп времени проверяемого момента
		const uint64_t value = this->_chrono->parse(date, "%Y-%m-%dT%H:%M:%S");
		// Получаем количество прошедших целых суток месяца
		const uint64_t passed = this->_chrono->actual(value, awh::chrono_t::type_t::DAY, awh::chrono_t::type_t::MONTH, awh::chrono_t::actual_t::PASSED);
		// Получаем количество оставшихся целых суток месяца
		const uint64_t left = this->_chrono->actual(value, awh::chrono_t::type_t::DAY, awh::chrono_t::type_t::MONTH, awh::chrono_t::actual_t::LEFT);
		// В апреле тридцать суток, поэтому целых суток вокруг любого момента двадцать девять
		ASSERT_EQ((passed + left), static_cast <uint64_t> (29)) << date;
		// Количество прошедших суток совпадает с числом месяца без единицы
		ASSERT_EQ(passed, static_cast <uint64_t> (this->_chrono->get <uint8_t> (value, awh::chrono_t::unit_t::DATE) - 1)) << date;
	}
	// В феврале високосного года двадцать девять суток
	{
		// Получаем штамп времени первого февраля високосного года
		const uint64_t value = this->_chrono->parse("2024-02-01T00:00:00", "%Y-%m-%dT%H:%M:%S");
		// Целых суток вокруг первого числа двадцать восемь
		ASSERT_EQ(
			this->_chrono->actual(value, awh::chrono_t::type_t::DAY, awh::chrono_t::type_t::MONTH, awh::chrono_t::actual_t::LEFT),
			static_cast <uint64_t> (28)
		);
	}
}

/**
 * @brief Тест насыщения смещения по месяцам за пределом разрядности года
 *
 * @details Обозначение года хранится в разрядности uint16_t, и суммарный порядковый
 *          номер месяца свыше 65536 лет оборачивался: прибавление 786432 месяцев
 *          возвращало исходную дату, а не последнюю представимую. Проверка ведётся по
 *          кратным разрядности и соседним с ними значениям
 *
 */
TEST_F(ChronoFixture, ExecutionOffsetMonthWrapChronoTest){
	// Получаем эталонный момент времени
	const uint64_t date = this->_chrono->parse("2025-04-06T12:00:00", "%Y-%m-%dT%H:%M:%S");
	// Выполняем перебор смещений, кратных разрядности обозначения года
	for(const uint64_t value : {static_cast <uint64_t> (65536) * 12, (static_cast <uint64_t> (65536) * 12) + 12,
		(static_cast <uint64_t> (65536) * 12) - 12, static_cast <uint64_t> (65536) * 24, static_cast <uint64_t> (1000000000)})
		// Выполняем проверку насыщения смещения последней представимой датой
		ASSERT_EQ(this->_chrono->offset(date, value, awh::chrono_t::type_t::MONTH, awh::chrono_t::offset_t::INCREMENT),
			static_cast <uint64_t> (253402300799999ULL)) << value;
}

/**
 * @brief Тест обнуления смещения долями миллисекунды свыше самого штампа
 *
 * @details Уменьшение на величину, превосходящую штамп времени, обязано давать начало
 *          промежутка представимости, как это делает путь миллисекунд. Прежде
 *          вычитаемое просто отбрасывалось, и уменьшение возвращало исходную дату
 *
 */
TEST_F(ChronoFixture, ExecutionOffsetFractionUnderflowChronoTest){
	// Выполняем перебор долей миллисекунды
	for(const awh::chrono_t::type_t type : {awh::chrono_t::type_t::MILLISECONDS,
		awh::chrono_t::type_t::MICROSECONDS, awh::chrono_t::type_t::NANOSECONDS})
		// Выполняем проверку обнуления смещения свыше самого штампа времени
		ASSERT_EQ(this->_chrono->offset(5000, 999999999999ULL, type, awh::chrono_t::offset_t::DECREMENT),
			static_cast <uint64_t> (0)) << static_cast <uint16_t> (type);
}

/**
 * @brief Тест согласованности смещения по годам и по месяцам
 *
 * @details Год - единица календарная, как и месяц: прибавление года обязано давать ту
 *          же дату, что и прибавление двенадцати месяцев. Прежде год отсчитывался
 *          сутками, отчего 29 февраля переливалось на 1 марта, тогда как те же
 *          двенадцать месяцев давали 28 февраля
 *
 */
TEST_F(ChronoFixture, ExecutionOffsetYearCalendarChronoTest){
	// Получаем момент времени 29 февраля високосного года
	const uint64_t date = this->_chrono->parse("2024-02-29T12:00:00", "%Y-%m-%dT%H:%M:%S");
	// Выполняем перебор направлений смещения
	for(const awh::chrono_t::offset_t offset : {awh::chrono_t::offset_t::INCREMENT, awh::chrono_t::offset_t::DECREMENT}){
		// Выполняем перебор количества лет смещения
		for(uint64_t years = 1; years <= 8; years++)
			// Выполняем проверку совпадения смещения по годам и по двенадцати месяцам
			ASSERT_EQ(this->_chrono->offset(date, years, awh::chrono_t::type_t::YEAR, offset),
				this->_chrono->offset(date, years * 12, awh::chrono_t::type_t::MONTH, offset)) << years;
	}
	// Выполняем проверку приведения 29 февраля к последнему дню февраля обычного года
	ASSERT_EQ(this->_chrono->format(this->_chrono->offset(date, 1, awh::chrono_t::type_t::YEAR,
		awh::chrono_t::offset_t::INCREMENT), 0, "%Y-%m-%d"), "2025-02-28");
	// Выполняем проверку сохранения 29 февраля при попадании в високосный год
	ASSERT_EQ(this->_chrono->format(this->_chrono->offset(date, 4, awh::chrono_t::type_t::YEAR,
		awh::chrono_t::offset_t::INCREMENT), 0, "%Y-%m-%d"), "2028-02-29");
}

/**
 * @brief Тест насыщения конца промежутка у края календаря
 *
 * @details Конец периода вычисляется как начало следующего без единицы, и для
 *          последнего года, месяца, недели и суток календаря начало следующего периода
 *          выходило за предел представимости: конец 9999 года выдавался мгновением,
 *          следующим за последним
 *
 */
TEST_F(ChronoFixture, ExecutionEndEdgeChronoTest){
	// Получаем последний момент времени, представимый календарём
	const uint64_t date = this->_chrono->parse("9999-12-31T23:59:59", "%Y-%m-%dT%H:%M:%S");
	// Выполняем перебор единиц измерения промежутка
	for(const awh::chrono_t::type_t type : {awh::chrono_t::type_t::YEAR, awh::chrono_t::type_t::MONTH,
		awh::chrono_t::type_t::WEEK, awh::chrono_t::type_t::DAY, awh::chrono_t::type_t::HOUR})
		// Выполняем проверку насыщения конца промежутка последней представимой датой
		ASSERT_EQ(this->_chrono->end(date, type), static_cast <uint64_t> (253402300799999ULL))
			<< static_cast <uint16_t> (type);
}

/**
 * @brief Тест приведения числа месяца при его установке
 *
 * @details Смена месяца оставляла в объекте число, в новом месяце отсутствующее, и
 *          сборка штампа времени переносила дату вперёд: 31 января со сменой месяца на
 *          второй давало 3 марта. Длина месяца зависит и от самого месяца, и от
 *          високосности года, поэтому проверка ведётся по обоим
 *
 */
TEST_F(ChronoFixture, ExecutionSetMonthClampChronoTest){
	// Структура параметров проверки
	struct Parameter {
		// Исходный момент времени
		const char * date;
		// Устанавливаемый номер месяца
		uint8_t month;
		// Ожидаемая запись даты
		const char * result;
	};
	// Перечень проверяемых случаев
	const Parameter parameters[] = {
		{"2025-01-31T12:00:00", 2, "2025-02-28T12:00:00"},
		{"2024-01-31T12:00:00", 2, "2024-02-29T12:00:00"},
		{"2025-03-31T12:00:00", 4, "2025-04-30T12:00:00"},
		{"2025-01-15T12:00:00", 2, "2025-02-15T12:00:00"}
	};
	// Выполняем перебор всех проверяемых случаев
	for(const Parameter & parameter : parameters){
		// Закрепляем исходный момент времени в местном хранилище объекта
		this->_chrono->timestamp(this->_chrono->parse(parameter.date, "%Y-%m-%dT%H:%M:%S"),
			awh::chrono_t::type_t::MILLISECONDS);
		// Выполняем установку номера месяца
		this->_chrono->set <uint8_t> (parameter.month, awh::chrono_t::unit_t::MONTH);
		// Выполняем проверку приведения числа месяца к его настоящей длине
		ASSERT_EQ(this->_chrono->format("%Y-%m-%dT%H:%M:%S", awh::chrono_t::storage_t::LOCAL),
			std::string(parameter.result)) << parameter.date;
	}
	// Закрепляем 29 февраля високосного года в местном хранилище объекта
	this->_chrono->timestamp(this->_chrono->parse("2024-02-29T12:00:00", "%Y-%m-%dT%H:%M:%S"),
		awh::chrono_t::type_t::MILLISECONDS);
	// Выполняем установку обычного года
	this->_chrono->set <uint16_t> (2025, awh::chrono_t::unit_t::YEAR);
	// Выполняем проверку приведения числа месяца при смене високосного года на обычный
	ASSERT_EQ(this->_chrono->format("%Y-%m-%dT%H:%M:%S", awh::chrono_t::storage_t::LOCAL), "2025-02-28T12:00:00");
}

/**
 * @brief Тест согласованности обозначения зоны с её смещением
 *
 * @details Запись, зоны не содержащая, читается в зоне окружения, обозначения не
 *          имеющей. Прежде обозначение выставлялось в UTC при любом смещении, отчего
 *          разбор в московской зоне давал смещение +10800 при обозначении UTC, и
 *          переменная формата \%Z печатала UTC для московского времени. Проверка
 *          сверяет обозначение с самим смещением, а не с ожидаемой строкой: имя зоны
 *          зависит от настроек машины, а их согласованность - нет
 *
 */
TEST_F(ChronoFixture, ExecutionZoneDesignationChronoTest){
	// Перечень проверяемых временных зон окружения
	const char * timezones[] = {"UTC", "Europe/Moscow", "America/New_York", "Asia/Kolkata", "Australia/Sydney"};
	// Выполняем перебор всех проверяемых временных зон окружения
	for(const char * timezone : timezones){
		// Выставляем временную зону окружения
		::setenv("TZ", timezone, 1);
		// Создаём объект работы с датой и временем
		awh::chrono_t chrono(this->_fmk.get(), this->_log.get());
		// Выполняем разбор записи, временной зоны не содержащей
		chrono.parse("2025-04-06 12:37:01", "%Y-%m-%d %H:%M:%S", awh::chrono_t::storage_t::LOCAL);
		// Получаем смещение временной зоны, выставленной объекту разбором
		const int32_t offset = chrono.getTimeZone(awh::chrono_t::storage_t::LOCAL);
		// Выполняем проверку согласованности обозначения зоны с её смещением
		ASSERT_EQ(chrono.getTimeZone(chrono.format("%Z", awh::chrono_t::storage_t::LOCAL)), offset) << timezone;
	}
	// Восстанавливаем временную зону окружения, закреплённую фикстурой
	::setenv("TZ", "UTC", 1);
	// Выполняем установку смещения временной зоны числом
	this->_chrono->setTimeZone(10800);
	// Выполняем проверку согласованности обозначения зоны, заданной числом
	ASSERT_EQ(this->_chrono->getTimeZone(this->_chrono->format("%Z", awh::chrono_t::storage_t::LOCAL)),
		static_cast <int32_t> (10800));
}

/**
 * @brief Тест сброса долей миллисекунды при смене единиц штампа времени
 *
 * @details Штамп времени в единицах крупнее микросекунды долей миллисекунды не несёт,
 *          а разложение штампа поля долей не трогает: прежде от прошлой установки в
 *          объекте оставались чужие доли, и после наносекундного штампа миллисекундный
 *          читался обратно в наносекундах не тем мгновением, каким был задан
 *
 */
TEST_F(ChronoFixture, ExecutionTimestampFractionResetChronoTest){
	// Выполняем установку наносекундного штампа времени с ненулевыми долями
	this->_chrono->timestamp(1743943021123456789ULL, awh::chrono_t::type_t::NANOSECONDS);
	// Выполняем проверку обратного чтения наносекундного штампа времени
	ASSERT_EQ(this->_chrono->timestamp(awh::chrono_t::type_t::NANOSECONDS, awh::chrono_t::storage_t::LOCAL),
		1743943021123456789ULL);
	// Выполняем установку миллисекундного штампа времени
	this->_chrono->timestamp(1743943021123ULL, awh::chrono_t::type_t::MILLISECONDS);
	// Выполняем проверку обратного чтения миллисекундного штампа времени
	ASSERT_EQ(this->_chrono->timestamp(awh::chrono_t::type_t::MILLISECONDS, awh::chrono_t::storage_t::LOCAL),
		1743943021123ULL);
	// Выполняем проверку отсутствия долей прошлой установки в микросекундах
	ASSERT_EQ(this->_chrono->timestamp(awh::chrono_t::type_t::MICROSECONDS, awh::chrono_t::storage_t::LOCAL),
		1743943021123000ULL);
	// Выполняем проверку отсутствия долей прошлой установки в наносекундах
	ASSERT_EQ(this->_chrono->timestamp(awh::chrono_t::type_t::NANOSECONDS, awh::chrono_t::storage_t::LOCAL),
		1743943021123000000ULL);
	// Выполняем перебор единиц измерения крупнее микросекунды
	for(const awh::chrono_t::type_t type : {awh::chrono_t::type_t::SECONDS, awh::chrono_t::type_t::MINUTES,
		awh::chrono_t::type_t::HOUR, awh::chrono_t::type_t::DAY}){
		// Выполняем установку наносекундного штампа времени с ненулевыми долями
		this->_chrono->timestamp(1743943021123456789ULL, awh::chrono_t::type_t::NANOSECONDS);
		// Выполняем установку штампа времени в единицах крупнее микросекунды
		this->_chrono->timestamp(1000, type);
		// Выполняем проверку кратности наносекундного штампа миллисекунде
		ASSERT_EQ(this->_chrono->timestamp(awh::chrono_t::type_t::NANOSECONDS, awh::chrono_t::storage_t::LOCAL) % 1000000ULL,
			static_cast <uint64_t> (0)) << static_cast <uint16_t> (type);
	}
}

/**
 * @brief Тест сохранения явно заданной нулевой временной зоны
 *
 * @details Нулевое смещение, заданное записью либо методом setTimeZone, обозначается
 *          UTC: обозначение здесь единственное, чем нулевая зона, заданная явно,
 *          отличается от незаданной, а от этого зависит, перекладывать ли запись в
 *          зону окружения при формировании. Прежде запись с явным смещением +0000 под
 *          зоной окружения Europe/Moscow формировалась обратно как +0300 со сдвигом
 *          времени на три часа. Проверка ведётся вне зоны UTC намеренно: фикстура
 *          закрепляет окружение на UTC, и в ней расхождение не проявляется
 *
 */
TEST_F(ChronoFixture, ExecutionExplicitZeroZoneChronoTest){
	// Перечень проверяемых временных зон окружения
	const char * timezones[] = {"Europe/Moscow", "America/New_York", "Asia/Kolkata"};
	// Выполняем перебор всех проверяемых временных зон окружения
	for(const char * timezone : timezones){
		// Выставляем временную зону окружения
		::setenv("TZ", timezone, 1);
		{
			// Создаём объект работы с датой и временем
			awh::chrono_t chrono(this->_fmk.get(), this->_log.get());
			// Выполняем разбор записи с явным нулевым смещением
			chrono.parse("2025-04-06T12:37:01+0000", "%Y-%m-%dT%H:%M:%S%z", awh::chrono_t::storage_t::LOCAL);
			// Выполняем проверку сохранения записи в её собственной временной зоне
			ASSERT_EQ(chrono.format("%Y-%m-%dT%H:%M:%S%z", awh::chrono_t::storage_t::LOCAL),
				"2025-04-06T12:37:01+0000") << timezone;
		}
		{
			// Создаём объект работы с датой и временем
			awh::chrono_t chrono(this->_fmk.get(), this->_log.get());
			// Закрепляем момент времени в местном хранилище объекта
			chrono.timestamp(1743943021000ULL, awh::chrono_t::type_t::MILLISECONDS);
			// Выполняем установку нулевого смещения временной зоны
			chrono.setTimeZone(0);
			// Выполняем проверку сохранения выставленной временной зоны
			ASSERT_EQ(chrono.format("%Y-%m-%dT%H:%M:%S%z", awh::chrono_t::storage_t::LOCAL),
				"2025-04-06T12:37:01+0000") << timezone;
		}
	}
	// Восстанавливаем временную зону окружения, закреплённую фикстурой
	::setenv("TZ", "UTC", 1);
}

/**
 * @brief Тест сохранения момента времени при смене смещения временной зоны
 *
 * @details Смена смещения перекладывает запись в новую зону, сохраняя сам момент
 *          времени, ровно как это делает setTimeZone: прежде смещение писалось прямо в
 *          поле, а гражданское время оставалось прежним, отчего установка нулевого
 *          смещения поверх московского сдвигала момент на три часа вперёд
 *
 */
TEST_F(ChronoFixture, ExecutionSetOffsetShiftChronoTest){
	// Выполняем перебор смещений с шагом в час по всему диапазону зон
	for(int32_t offset = -43200; offset <= 50400; offset += 3600){
		// Закрепляем момент времени в местном хранилище объекта
		this->_chrono->timestamp(1743943021000ULL, awh::chrono_t::type_t::MILLISECONDS);
		// Выполняем установку временной зоны методом её выставления
		this->_chrono->setTimeZone(10800);
		// Выполняем установку смещения временной зоны через единицу данных
		this->_chrono->set <int32_t> (offset, awh::chrono_t::unit_t::OFFSET);
		// Выполняем проверку сохранения момента времени
		ASSERT_EQ(this->_chrono->timestamp(awh::chrono_t::type_t::MILLISECONDS, awh::chrono_t::storage_t::LOCAL),
			1743943021000ULL) << offset;
		// Выполняем проверку установки самого смещения временной зоны
		ASSERT_EQ(this->_chrono->getTimeZone(awh::chrono_t::storage_t::LOCAL), offset);
	}
}

/**
 * @brief Тест переноса даты установкой номера дня в году и номера недели
 *
 * @details Номер дня в году задаёт дату целиком, номер недели переносит её на
 *          указанную неделю, сохраняя день недели и время суток. Прежде оба сеттера
 *          писали одно лишь поле, дату не двигали вовсе, а сами поля перезаписывались
 *          при следующем разложении штампа времени
 *
 */
TEST_F(ChronoFixture, ExecutionSetDaysWeeksChronoTest){
	// Получаем момент времени начала високосного года
	const uint64_t date = this->_chrono->parse("2024-01-01T12:34:56", "%Y-%m-%dT%H:%M:%S");
	// Структура параметров проверки номера дня в году
	struct Parameter {
		// Устанавливаемый номер дня в году
		uint16_t days;
		// Ожидаемая запись даты
		const char * result;
	};
	// Перечень проверяемых случаев
	const Parameter parameters[] = {
		{0, "2024-01-01"}, {31, "2024-02-01"}, {59, "2024-02-29"},
		{60, "2024-03-01"}, {99, "2024-04-09"}, {365, "2024-12-31"}
	};
	// Выполняем перебор всех проверяемых случаев
	for(const Parameter & parameter : parameters){
		// Закрепляем момент времени в местном хранилище объекта
		this->_chrono->timestamp(date, awh::chrono_t::type_t::MILLISECONDS);
		// Выполняем установку номера дня в году
		this->_chrono->set <uint16_t> (parameter.days, awh::chrono_t::unit_t::DAYS);
		// Выполняем проверку переноса даты на указанный день года
		ASSERT_EQ(this->_chrono->format("%Y-%m-%d", awh::chrono_t::storage_t::LOCAL), std::string(parameter.result))
			<< parameter.days;
		// Выполняем проверку сохранения времени суток
		ASSERT_EQ(this->_chrono->format("%H:%M:%S", awh::chrono_t::storage_t::LOCAL), "12:34:56") << parameter.days;
		// Выполняем проверку обратного чтения номера дня в году
		ASSERT_EQ(this->_chrono->get <uint16_t> (awh::chrono_t::unit_t::DAYS, awh::chrono_t::storage_t::LOCAL),
			parameter.days);
	}
	// Получаем момент времени середины недели
	const uint64_t week = this->_chrono->parse("2024-01-03T12:34:56", "%Y-%m-%dT%H:%M:%S");
	// Выполняем перебор номеров недели, умещающихся в год
	for(uint8_t weeks = 0; weeks < 52; weeks++){
		// Закрепляем момент времени в местном хранилище объекта
		this->_chrono->timestamp(week, awh::chrono_t::type_t::MILLISECONDS);
		// Выполняем установку номера недели
		this->_chrono->set <uint8_t> (weeks, awh::chrono_t::unit_t::WEEKS);
		// Выполняем проверку обратного чтения номера недели
		ASSERT_EQ(this->_chrono->get <uint8_t> (awh::chrono_t::unit_t::WEEKS, awh::chrono_t::storage_t::LOCAL), weeks);
		// Выполняем проверку сохранения дня недели
		ASSERT_EQ(this->_chrono->format("%A", awh::chrono_t::storage_t::LOCAL), "Wednesday") << static_cast <uint16_t> (weeks);
		// Выполняем проверку сохранения времени суток
		ASSERT_EQ(this->_chrono->format("%H:%M:%S", awh::chrono_t::storage_t::LOCAL), "12:34:56") << static_cast <uint16_t> (weeks);
	}
}

/**
 * @brief Тест согласованности выводных полей после установки задающих
 *
 * @details Год, месяц и число месяца задают дату целиком, а номер дня в году, номер
 *          недели и день недели из них выводятся. Прежде установка задающего поля
 *          выводные оставляла прежними, и два открытых способа прочитать один объект
 *          расходились: format вычислял их заново, get читал поля как есть. После
 *          31 января со сменой месяца на второй метод format давал 59-й день года, а
 *          get - 30-й, то есть день, оставшийся от января
 *
 */
TEST_F(ChronoFixture, ExecutionSetSyncChronoTest){
	// Перечень исходных моментов времени
	const char * dates[] = {"2025-01-31T12:00:00", "2024-02-29T12:00:00", "2025-04-06T12:00:00", "2024-12-31T23:00:00"};
	// Выполняем перебор всех исходных моментов времени
	for(const char * date : dates){
		// Получаем исходный момент времени
		const uint64_t stamp = this->_chrono->parse(date, "%Y-%m-%dT%H:%M:%S");
		// Выполняем перебор устанавливаемых номеров месяца
		for(uint8_t month = 1; month < 13; month++){
			// Закрепляем исходный момент времени в местном хранилище объекта
			this->_chrono->timestamp(stamp, awh::chrono_t::type_t::MILLISECONDS);
			// Выполняем установку номера месяца
			this->_chrono->set <uint8_t> (month, awh::chrono_t::unit_t::MONTH);
			// Выполняем проверку согласованности номера дня в году
			ASSERT_EQ(this->_chrono->get <uint16_t> (awh::chrono_t::unit_t::DAYS, awh::chrono_t::storage_t::LOCAL) + 1,
				static_cast <uint16_t> (std::stoi(this->_chrono->format("%j", awh::chrono_t::storage_t::LOCAL))))
				<< date << " " << static_cast <uint16_t> (month);
			// Выполняем проверку согласованности дня недели
			ASSERT_EQ(this->_chrono->get <uint8_t> (awh::chrono_t::unit_t::DAY, awh::chrono_t::storage_t::LOCAL),
				static_cast <uint8_t> (std::stoi(this->_chrono->format("%u", awh::chrono_t::storage_t::LOCAL))))
				<< date << " " << static_cast <uint16_t> (month);
		}
		// Выполняем перебор устанавливаемых чисел месяца
		for(uint8_t day = 1; day < 32; day++){
			// Закрепляем исходный момент времени в местном хранилище объекта
			this->_chrono->timestamp(stamp, awh::chrono_t::type_t::MILLISECONDS);
			// Выполняем установку числа месяца
			this->_chrono->set <uint8_t> (day, awh::chrono_t::unit_t::DATE);
			// Выполняем проверку согласованности номера дня в году
			ASSERT_EQ(this->_chrono->get <uint16_t> (awh::chrono_t::unit_t::DAYS, awh::chrono_t::storage_t::LOCAL) + 1,
				static_cast <uint16_t> (std::stoi(this->_chrono->format("%j", awh::chrono_t::storage_t::LOCAL))))
				<< date << " " << static_cast <uint16_t> (day);
			// Выполняем проверку приведения числа месяца к его настоящей длине
			ASSERT_LE(this->_chrono->get <uint8_t> (awh::chrono_t::unit_t::DATE, awh::chrono_t::storage_t::LOCAL), day)
				<< date << " " << static_cast <uint16_t> (day);
		}
	}
	// Закрепляем февраль в местном хранилище объекта
	this->_chrono->timestamp(this->_chrono->parse("2025-02-10T12:00:00", "%Y-%m-%dT%H:%M:%S"),
		awh::chrono_t::type_t::MILLISECONDS);
	// Выполняем установку числа месяца, в феврале отсутствующего
	this->_chrono->set <uint8_t> (31, awh::chrono_t::unit_t::DATE);
	// Выполняем проверку приведения числа к последнему дню февраля
	ASSERT_EQ(this->_chrono->format("%Y-%m-%d", awh::chrono_t::storage_t::LOCAL), "2025-02-28");
}

/**
 * @brief Тест переноса даты установкой дня недели
 *
 * @details День недели переносит дату на этот день той же недели, сохраняя время
 *          суток. Прежде установка писала одно лишь поле, дату не двигала вовсе, а
 *          само поле перезаписывалось при следующем разложении штампа времени
 *
 */
TEST_F(ChronoFixture, ExecutionSetDayOfWeekChronoTest){
	// Получаем момент времени середины недели
	const uint64_t stamp = this->_chrono->parse("2025-04-09T12:34:56", "%Y-%m-%dT%H:%M:%S");
	// Выполняем перебор всех дней недели
	for(uint8_t day = 1; day < 8; day++){
		// Закрепляем момент времени в местном хранилище объекта
		this->_chrono->timestamp(stamp, awh::chrono_t::type_t::MILLISECONDS);
		// Выполняем установку дня недели
		this->_chrono->set <uint8_t> (day, awh::chrono_t::unit_t::DAY);
		// Выполняем проверку переноса даты на указанный день недели
		ASSERT_EQ(this->_chrono->get <uint8_t> (awh::chrono_t::unit_t::DAY, awh::chrono_t::storage_t::LOCAL), day);
		// Выполняем проверку сохранения времени суток
		ASSERT_EQ(this->_chrono->format("%H:%M:%S", awh::chrono_t::storage_t::LOCAL), "12:34:56")
			<< static_cast <uint16_t> (day);
	}
}

/**
 * @brief Тест приведения года и номера дня в году к пределам календаря
 *
 * @details Год приводится к промежутку представимости: прежде принимался любой
 *          ненулевой, и год до эпохи сбрасывал дату к её началу целиком, теряя месяц и
 *          число месяца. Номер дня в году приводится к длине самого года: обычный год
 *          несёт 365 суток, и номер 365 в нём выходит за последние
 *
 */
TEST_F(ChronoFixture, ExecutionSetBoundsChronoTest){
	// Получаем эталонный момент времени
	const uint64_t stamp = this->_chrono->parse("2025-04-06T12:00:00", "%Y-%m-%dT%H:%M:%S");
	// Выполняем перебор годов за пределами промежутка представимости
	for(const uint16_t year : {static_cast <uint16_t> (1), static_cast <uint16_t> (1969),
		static_cast <uint16_t> (10000), static_cast <uint16_t> (65535)}){
		// Закрепляем эталонный момент времени в местном хранилище объекта
		this->_chrono->timestamp(stamp, awh::chrono_t::type_t::MILLISECONDS);
		// Выполняем установку года
		this->_chrono->set <uint16_t> (year, awh::chrono_t::unit_t::YEAR);
		// Выполняем проверку сохранения месяца и числа месяца
		ASSERT_EQ(this->_chrono->format("%m-%dT%H:%M:%S", awh::chrono_t::storage_t::LOCAL), "04-06T12:00:00") << year;
		// Выполняем проверку приведения года к промежутку представимости
		ASSERT_EQ(this->_chrono->format("%Y", awh::chrono_t::storage_t::LOCAL), ((year < 1970) ? "1970" : "9999")) << year;
	}
	// Закрепляем начало обычного года в местном хранилище объекта
	this->_chrono->timestamp(this->_chrono->parse("2025-01-01T12:00:00", "%Y-%m-%dT%H:%M:%S"),
		awh::chrono_t::type_t::MILLISECONDS);
	// Выполняем установку номера дня, за длину обычного года выходящего
	this->_chrono->set <uint16_t> (365, awh::chrono_t::unit_t::DAYS);
	// Выполняем проверку приведения даты к последним суткам года
	ASSERT_EQ(this->_chrono->format("%Y-%m-%d", awh::chrono_t::storage_t::LOCAL), "2025-12-31");
	// Выполняем проверку приведения номера дня к длине обычного года
	ASSERT_EQ(this->_chrono->get <uint16_t> (awh::chrono_t::unit_t::DAYS, awh::chrono_t::storage_t::LOCAL),
		static_cast <uint16_t> (364));
}

/**
 * @brief Тест разбора названий месяцев и дней недели в любом регистре
 *
 * @details Сравнение разобранного названия с самим названием ведётся без оглядки на
 *          регистр, а правило захвата требовало заглавной буквы с последующими
 *          строчными и отвергало записи вида APR и apr ещё до сравнения. Посторонние
 *          слова отсеиваются теперь не регистром, а сверкой со словарём названий
 *
 */
TEST_F(ChronoFixture, ExecutionNameCaseChronoTest){
	// Перечень записей месяца в разном регистре
	const char * months[] = {"Apr", "APR", "apr", "aPr", "April", "APRIL", "april"};
	// Выполняем перебор всех записей месяца
	for(const char * month : months){
		// Формируем запись даты с текущим написанием месяца
		const std::string record = (std::string("06 ") + month + " 2025");
		// Выполняем проверку разбора записи с текущим написанием месяца
		ASSERT_EQ(this->_chrono->format(this->_chrono->parse(record, "%d %b %Y"), 0, "%Y-%m-%d"), "2025-04-06")
			<< record;
	}
	// Перечень записей дня недели в разном регистре
	const char * days[] = {"Sun", "SUN", "sun", "Sunday", "SUNDAY", "sunday"};
	// Выполняем перебор всех записей дня недели
	for(const char * day : days){
		// Формируем запись даты с текущим написанием дня недели
		const std::string record = (std::string(day) + ", 06 Apr 2025");
		// Выполняем проверку разбора записи с текущим написанием дня недели
		ASSERT_EQ(this->_chrono->format(this->_chrono->parse(record, "%a, %d %b %Y"), 0, "%Y-%m-%d"), "2025-04-06")
			<< record;
	}
	/**
	 * Слово, месяцем не являющееся, захватываться не должно: прежде от него защищал
	 * один лишь регистр, а теперь - сверка со словарём названий
	 */
	// Выполняем проверку разбора записи с посторонним словом перед месяцем
	ASSERT_EQ(this->_chrono->format(this->_chrono->parse("some 06 Apr 2025", "%d %b %Y"), 0, "%Y-%m-%d"), "2025-04-06");
	// Выполняем проверку разбора записи с посторонним словом в нижнем регистре
	ASSERT_EQ(this->_chrono->format(this->_chrono->parse("word 06 apr 2025", "%d %b %Y"), 0, "%Y-%m-%d"), "2025-04-06");
}

/**
 * @brief Тест взаимозаменяемости кратких и полных названий
 *
 * @details Стандарт POSIX объявляет переменные формата \%a и \%A, равно как \%b, \%h и
 *          \%B, взаимозаменяемыми: каждая принимает и краткое название, и полное.
 *          Прежде присвоение принимало лишь свою форму, тогда как сверка захвата со
 *          словарём принимала обе, отчего запись "06 Apr 2025" по формату "\%d \%B \%Y"
 *          разбор проходила, но месяц оставался неустановленным и дата уходила в
 *          другой месяц
 *
 */
TEST_F(ChronoFixture, ExecutionNameFormsChronoTest){
	// Перечень записей месяца в обеих формах
	const char * months[] = {"Apr", "APR", "apr", "April", "APRIL", "april"};
	// Выполняем перебор обеих переменных формата месяца
	for(const char * format : {"%d %b %Y", "%d %B %Y", "%d %h %Y"}){
		// Выполняем перебор всех записей месяца
		for(const char * month : months){
			// Формируем запись даты с текущим написанием месяца
			const std::string record = (std::string("06 ") + month + " 2025");
			// Выполняем проверку разбора записи с текущим написанием месяца
			ASSERT_EQ(this->_chrono->format(this->_chrono->parse(record, format), 0, "%Y-%m-%d"), "2025-04-06")
				<< format << " " << record;
			// Выполняем проверку признания записи годной
			ASSERT_TRUE(this->_chrono->validate(record, format)) << format << " " << record;
		}
	}
	// Перечень записей дня недели в обеих формах
	const char * days[] = {"Sun", "SUN", "sun", "Sunday", "SUNDAY", "sunday"};
	// Выполняем перебор обеих переменных формата дня недели
	for(const char * format : {"%a, %d %b %Y", "%A, %d %b %Y"}){
		// Выполняем перебор всех записей дня недели
		for(const char * day : days){
			// Формируем запись даты с текущим написанием дня недели
			const std::string record = (std::string(day) + ", 06 Apr 2025");
			// Выполняем проверку разбора записи с текущим написанием дня недели
			ASSERT_EQ(this->_chrono->format(this->_chrono->parse(record, format), 0, "%Y-%m-%d %A"), "2025-04-06 Sunday")
				<< format << " " << record;
		}
	}
	/**
	 * Название подбирается по словарю, а не набирается жадно с последующей сверкой:
	 * жадный набор захватывал слово целиком и отвергал название, за которым идут
	 * другие буквы, тогда как strptime стандарта POSIX читает в нём месяц
	 */
	// Выполняем проверку разбора названия, за которым идут другие буквы
	ASSERT_EQ(this->_chrono->format(this->_chrono->parse("06 AprilFoo 2025", "%d %B %Y"), 0, "%Y-%m-%d"), "2025-04-06");
	// Выполняем проверку разбора названия, за которым идёт знак препинания
	ASSERT_EQ(this->_chrono->format(this->_chrono->parse("06 Apr. 2025", "%d %b %Y"), 0, "%Y-%m-%d"), "2025-04-06");
}

/**
 * @brief Тест сверки названий записи asctime со словарём
 *
 * @details Правило захвата названия регистра букв не ограничивает, и без сверки со
 *          словарём за название сходил любой трёхбуквенный набор: запись вида
 *          "XXX YYY  6 12:37:01 2025" разбор проходила, оставляя месяц неустановленным
 *
 */
TEST_F(ChronoFixture, ExecutionAsctimeNamesChronoTest){
	// Выполняем проверку разбора записи asctime
	ASSERT_EQ(this->_chrono->format(this->_chrono->parse("Sun Apr  6 12:37:01 2025", "%c"), 0, "%Y-%m-%dT%H:%M:%S"),
		"2025-04-06T12:37:01");
	// Выполняем проверку признания записи asctime годной
	ASSERT_TRUE(this->_chrono->validate("Sun Apr  6 12:37:01 2025", "%c"));
	// Перечень записей с названиями, в словаре отсутствующими
	const char * records[] = {"XXX YYY  6 12:37:01 2025", "Foo Bar  6 12:37:01 2025", "Sun Yyy  6 12:37:01 2025"};
	// Выполняем перебор всех записей с посторонними названиями
	for(const char * record : records)
		// Выполняем проверку отказа признать запись годной
		ASSERT_FALSE(this->_chrono->validate(record, "%c")) << record;
}

/**
 * @brief Тест разрешения смещения сводной зоны по самой записи
 *
 * @details Смещение сводных зон Северной Америки зависит от момента времени, а установка
 *          зоны объекту брала его от текущего мгновения и больше не пересчитывала: зимняя
 *          запись, разобранная летом, получала летнее смещение, отчего переменные формата
 *          %z и %Z расходились между собой - "-0400 EST"
 *
 */
TEST_F(ChronoFixture, ExecutionCompositeZoneResolveChronoTest){
	// Перечень сводных зон с их обозначениями и смещениями по обе стороны перехода
	const struct {
		awh::Chrono::zone_t zone;
		const char * winter;
		const char * summer;
	} zones[] = {
		{awh::Chrono::zone_t::AT, "-0400 UTC-4", "-0300 ADT"},
		{awh::Chrono::zone_t::CT, "-0600 UTC-6", "-0500 CDT"},
		{awh::Chrono::zone_t::ET, "-0500 EST",   "-0400 EDT"},
		{awh::Chrono::zone_t::MT, "-0700 UTC-7", "-0600 MDT"},
		{awh::Chrono::zone_t::NT, "-0330 NST",   "-0230 NDT"},
		{awh::Chrono::zone_t::PT, "-0800 PST",   "-0700 PDT"}
	};
	// Выполняем перебор всех сводных временных зон
	for(auto & item : zones){
		// Выполняем установку сводной временной зоны объекту
		this->_chrono->setTimeZone(item.zone);
		// Выполняем разбор записи, приходящейся на стандартное время зоны
		this->_chrono->parse("2025-01-15T12:00:00", "%Y-%m-%dT%H:%M:%S", awh::Chrono::storage_t::LOCAL);
		// Выполняем проверку согласия смещения с обозначением на стандартном времени
		ASSERT_EQ(this->_chrono->format("%Y-%m-%dT%H:%M:%S %z %Z", awh::Chrono::storage_t::LOCAL),
			std::string("2025-01-15T12:00:00 ") + item.winter);
		/**
		 * Установка календарных полей смещение пересчитывает наравне с разбором:
		 * перевод записи в июль уводит её на летнее время той же зоны
		 */
		// Выполняем перевод записи на летнее время зоны
		this->_chrono->set(7, awh::Chrono::unit_t::MONTH);
		// Выполняем проверку согласия смещения с обозначением на летнем времени
		ASSERT_EQ(this->_chrono->format("%Y-%m-%dT%H:%M:%S %z %Z", awh::Chrono::storage_t::LOCAL),
			std::string("2025-07-15T12:00:00 ") + item.summer);
	}
	/**
	 * Перекладка момента времени в сводную зону момента не меняет, и смещение
	 * разрешается по нему самому, а не по текущему мгновению
	 */
	// Выполняем разбор записи в нулевой временной зоне
	this->_chrono->parse("2025-01-15T17:00:00+0000", "%Y-%m-%dT%H:%M:%S%z", awh::Chrono::storage_t::LOCAL);
	// Выполняем перекладку записи в сводную временную зону
	this->_chrono->setTimeZone(awh::Chrono::zone_t::ET);
	// Выполняем проверку перекладки записи по стандартному времени зоны
	ASSERT_EQ(this->_chrono->format("%Y-%m-%dT%H:%M:%S %z %Z", awh::Chrono::storage_t::LOCAL),
		"2025-01-15T12:00:00 -0500 EST");
	// Выполняем установку сводной временной зоны объекту по её обозначению
	this->_chrono->setTimeZone("ET");
	// Выполняем разбор записи, приходящейся на стандартное время зоны
	this->_chrono->parse("2025-01-15T12:00:00", "%Y-%m-%dT%H:%M:%S", awh::Chrono::storage_t::LOCAL);
	// Выполняем проверку разрешения смещения зоны, заданной обозначением
	ASSERT_EQ(this->_chrono->format("%Y-%m-%dT%H:%M:%S %z %Z", awh::Chrono::storage_t::LOCAL),
		"2025-01-15T12:00:00 -0500 EST");
}

/**
 * @brief Тест поведения шестидесятой секунды
 *
 * @details Разбор записи високосную секунду не моделирует, а переносит её на следующую
 *          минуту - ровно как это делает связка strptime и timegm стандарта POSIX
 *
 */
TEST_F(ChronoFixture, ExecutionLeapSecondChronoTest){
	// Выполняем проверку переноса шестидесятой секунды на следующую минуту
	ASSERT_EQ(this->_chrono->format(this->_chrono->parse("2025-01-15T12:00:60", "%Y-%m-%dT%H:%M:%S"), 0, "%Y-%m-%dT%H:%M:%S"),
		"2025-01-15T12:01:00");
	// Выполняем проверку признания записи с шестидесятой секундой годной
	ASSERT_TRUE(this->_chrono->validate("2025-01-15T12:00:60", "%Y-%m-%dT%H:%M:%S"));
	// Выполняем проверку отказа признать годной запись с шестьдесят первой секундой
	ASSERT_FALSE(this->_chrono->validate("2025-01-15T12:00:61", "%Y-%m-%dT%H:%M:%S"));
}

/**
 * @brief Тест разрешения смещения сводной зоны по прочим путям
 *
 * @details Разрешение смещения по самой записи заводилось разбором записи без зоны,
 *          установкой календарной единицы и перекладкой в зону, а обозначение сводной
 *          зоны приходит и переменной формата %Z, и печатается формированием в
 *          указанную зону, и меняется установкой часа: каждый из этих путей брал
 *          смещение от текущего мгновения
 *
 */
TEST_F(ChronoFixture, ExecutionCompositeZonePathsChronoTest){
	/**
	 * Обозначение сводной зоны, полученное переменной формата %Z, разрешается по
	 * самой разобранной записи
	 */
	// Выполняем разбор записи с обозначением сводной временной зоны
	this->_chrono->parse("2025-01-15T12:00:00 ET", "%Y-%m-%dT%H:%M:%S %Z", awh::Chrono::storage_t::LOCAL);
	// Выполняем проверку разрешения смещения зоны, записью заданной
	ASSERT_EQ(this->_chrono->format("%Y-%m-%dT%H:%M:%S %z %Z", awh::Chrono::storage_t::LOCAL),
		"2025-01-15T12:00:00 -0500 EST");
	// Выполняем разбор летней записи с обозначением сводной временной зоны
	this->_chrono->parse("2025-07-15T12:00:00 ET", "%Y-%m-%dT%H:%M:%S %Z", awh::Chrono::storage_t::LOCAL);
	// Выполняем проверку разрешения смещения зоны на летнем времени
	ASSERT_EQ(this->_chrono->format("%Y-%m-%dT%H:%M:%S %z %Z", awh::Chrono::storage_t::LOCAL),
		"2025-07-15T12:00:00 -0400 EDT");
	/**
	 * Смещение, записью заданное числом, обозначения не несёт: прежде при объекте
	 * оставалось обозначение, выставленное ему до разбора, и запись формировалась
	 * обратно как "+0000 EST"
	 */
	// Выполняем установку сводной временной зоны объекту
	this->_chrono->setTimeZone(awh::Chrono::zone_t::ET);
	// Выполняем разбор записи со смещением, заданным числом
	this->_chrono->parse("2025-01-15T17:00:00+0000", "%Y-%m-%dT%H:%M:%S%z", awh::Chrono::storage_t::LOCAL);
	// Выполняем проверку сброса обозначения зоны, объекту выставленного
	ASSERT_EQ(this->_chrono->format("%Y-%m-%dT%H:%M:%S %z %Z", awh::Chrono::storage_t::LOCAL),
		"2025-01-15T17:00:00 +0000 UTC");
	/**
	 * Формирование записи в указанную зону момента времени не меняет, и смещение
	 * сводной зоны разрешается по нему самому
	 */
	// Выполняем проверку формирования записи в сводную зону по её обозначению
	ASSERT_EQ(this->_chrono->format(awh::Chrono::zone_t::ET, "%Y-%m-%dT%H:%M:%S %z %Z", awh::Chrono::storage_t::LOCAL),
		"2025-01-15T12:00:00 -0500 EST");
	// Выполняем проверку формирования записи в сводную зону по её названию
	ASSERT_EQ(this->_chrono->format(std::string_view("ET"), "%Y-%m-%dT%H:%M:%S %z %Z", awh::Chrono::storage_t::LOCAL),
		"2025-01-15T12:00:00 -0500 EST");
	/**
	 * Установка часа дату двигает наравне с календарными единицами и через границу
	 * перехода уводит точно так же
	 */
	// Выполняем установку сводной временной зоны объекту
	this->_chrono->setTimeZone(awh::Chrono::zone_t::ET);
	// Выполняем разбор записи, приходящейся на самый день весеннего перехода
	this->_chrono->parse("2025-03-09T01:30:00", "%Y-%m-%dT%H:%M:%S", awh::Chrono::storage_t::LOCAL);
	// Выполняем проверку стандартного времени зоны до перехода
	ASSERT_EQ(this->_chrono->format("%z %Z", awh::Chrono::storage_t::LOCAL), "-0500 EST");
	// Выполняем перевод записи за границу весеннего перехода
	this->_chrono->set(3, awh::Chrono::unit_t::HOUR);
	// Выполняем проверку летнего времени зоны после перехода
	ASSERT_EQ(this->_chrono->format("%Y-%m-%dT%H:%M:%S %z %Z", awh::Chrono::storage_t::LOCAL),
		"2025-03-09T03:30:00 -0400 EDT");
}

/**
 * @brief Тест разрешения сводной зоны в общем хранилище
 *
 * @details Разбор записи со сводной зоной пересчитывал смещение по самой записи лишь
 *          в местном хранилище: ветка общего хранилища ту же завершающую часть
 *          повторяет отдельно, и разрешение в неё не попало
 *
 */
TEST_F(ChronoFixture, ExecutionCompositeZoneGlobalChronoTest){
	// Выполняем проверку разбора записи стандартного времени зоны
	ASSERT_EQ(this->_chrono->format(this->_chrono->parse("2025-01-15T12:00:00 ET", "%Y-%m-%dT%H:%M:%S %Z"), 0, "%Y-%m-%dT%H:%M:%S"),
		"2025-01-15T17:00:00");
	// Выполняем проверку разбора записи летнего времени зоны
	ASSERT_EQ(this->_chrono->format(this->_chrono->parse("2025-07-15T12:00:00 ET", "%Y-%m-%dT%H:%M:%S %Z"), 0, "%Y-%m-%dT%H:%M:%S"),
		"2025-07-15T16:00:00");
}

/**
 * @brief Тест разрешения сводной зоны на самой границе перехода
 *
 * @details Внутри часа перехода местное время либо повторяется, либо не существует
 *          вовсе. Опорой разрешения служит стандартное время зоны, а не прежнее
 *          смещение объекта: иначе одна и та же запись разрешалась по-разному в
 *          зависимости от того, что лежало в объекте до разбора
 *
 */
TEST_F(ChronoFixture, ExecutionCompositeZoneBoundaryChronoTest){
	// Перечень записей на границе перехода с ожидаемым разрешением зоны
	const struct {
		const char * record;
		const char * result;
	} records[] = {
		{"2025-03-09T02:30:00", "-0400 EDT"},
		{"2025-11-02T01:30:00", "-0500 EST"}
	};
	// Перечень записей, задающих объекту смещение до разбора
	const char * origins[] = {"2025-01-01T00:00:00", "2025-07-01T00:00:00"};
	// Выполняем перебор всех записей на границе перехода
	for(auto & item : records){
		// Выполняем перебор всех записей, задающих объекту смещение до разбора
		for(const char * origin : origins){
			// Выполняем установку сводной временной зоны объекту
			this->_chrono->setTimeZone(awh::Chrono::zone_t::ET);
			// Выполняем разбор записи, задающей объекту смещение до разбора
			this->_chrono->parse(origin, "%Y-%m-%dT%H:%M:%S", awh::Chrono::storage_t::LOCAL);
			// Выполняем разбор записи, приходящейся на границу перехода
			this->_chrono->parse(item.record, "%Y-%m-%dT%H:%M:%S", awh::Chrono::storage_t::LOCAL);
			// Выполняем проверку разрешения зоны, от прежнего смещения не зависящего
			ASSERT_EQ(this->_chrono->format("%z %Z", awh::Chrono::storage_t::LOCAL), item.result)
				<< item.record << " " << origin;
		}
	}
	/**
	 * Признак летнего времени берётся у сводных зон из самого разрешённого смещения,
	 * а не из полей объекта: правило перехода задано стандартным временем зоны, а поля
	 * лежат в её гражданском, и в ноябрьской складке они расходились между собой
	 */
	// Выполняем установку сводной временной зоны объекту
	this->_chrono->setTimeZone(awh::Chrono::zone_t::ET);
	// Выполняем разбор записи, приходящейся на ноябрьскую складку
	this->_chrono->parse("2025-11-02T01:30:00", "%Y-%m-%dT%H:%M:%S", awh::Chrono::storage_t::LOCAL);
	// Выполняем проверку согласия признака летнего времени со смещением зоны
	ASSERT_FALSE(this->_chrono->dst(awh::Chrono::storage_t::LOCAL));
	// Выполняем разбор записи, приходящейся на летнее время зоны
	this->_chrono->parse("2025-07-15T12:00:00", "%Y-%m-%dT%H:%M:%S", awh::Chrono::storage_t::LOCAL);
	// Выполняем проверку согласия признака летнего времени со смещением зоны
	ASSERT_TRUE(this->_chrono->dst(awh::Chrono::storage_t::LOCAL));
}

/**
 * @brief Тест признака летнего времени местного хранилища
 *
 * @details Признак местного хранилища собирался заново по штампу времени объекта, а
 *          сборка раскладывает его в нулевой зоне и потому судит по всемирному часу:
 *          запись 01:30 утра весеннего перехода под сводной зоной выдавала летнее
 *          время при стандартном смещении, поскольку всемирный её час - шестой
 *
 */
TEST_F(ChronoFixture, ExecutionLocalDaylightChronoTest){
	// Перечень записей с ожидаемым смещением зоны и признаком летнего времени
	const struct {
		const char * record;
		const char * zone;
		bool dst;
	} records[] = {
		{"2025-01-15T12:00:00", "-0500 EST", false},
		{"2025-03-09T01:30:00", "-0500 EST", false},
		{"2025-03-09T03:30:00", "-0400 EDT", true},
		{"2025-07-15T12:00:00", "-0400 EDT", true},
		{"2025-11-02T01:30:00", "-0500 EST", false}
	};
	// Выполняем перебор всех записей
	for(auto & item : records){
		// Выполняем установку сводной временной зоны объекту
		this->_chrono->setTimeZone(awh::Chrono::zone_t::ET);
		// Выполняем разбор записи
		this->_chrono->parse(item.record, "%Y-%m-%dT%H:%M:%S", awh::Chrono::storage_t::LOCAL);
		// Выполняем проверку смещения и обозначения временной зоны
		ASSERT_EQ(this->_chrono->format("%z %Z", awh::Chrono::storage_t::LOCAL), item.zone) << item.record;
		// Выполняем проверку согласия признака летнего времени со смещением зоны
		ASSERT_EQ(this->_chrono->dst(awh::Chrono::storage_t::LOCAL), item.dst) << item.record;
	}
}

/**
 * @brief Тест разделения перегрузок временной зоны по текущему моменту и по дате
 *
 * @details Перегрузки getTimeZone и format, даты не принимающие, отвечают по текущему
 *          моменту - это их назначение, а не упущение. Вопрос «какое смещение и какое
 *          обозначение у зоны сейчас» задаётся не реже, чем вопрос о смещении на
 *          заданную дату, и отвечать на него обязан кто-то один. Для второго вопроса
 *          заведены перегрузки, дату принимающие, и все внутренние пути разбора и
 *          формирования записей идут через них. Тест заведён затем, чтобы разбор кода
 *          не выдавал это разделение за расхождение
 *
 */
TEST_F(ChronoFixture, ExecutionZoneOverloadContractChronoTest){
	// Штамп времени момента стандартного времени зоны
	const uint64_t winter = this->_chrono->parse("2025-01-15T12:00:00+0000", "%Y-%m-%dT%H:%M:%S%z");
	// Штамп времени момента летнего времени зоны
	const uint64_t summer = this->_chrono->parse("2025-07-15T12:00:00+0000", "%Y-%m-%dT%H:%M:%S%z");
	/**
	 * Перегрузки, дату принимающие, отвечают по ней самой и от текущего момента не
	 * зависят вовсе
	 */
	// Выполняем проверку смещения сводной зоны на стандартном времени
	ASSERT_EQ(this->_chrono->getTimeZone(awh::Chrono::zone_t::ET, winter), -18000);
	// Выполняем проверку смещения сводной зоны на летнем времени
	ASSERT_EQ(this->_chrono->getTimeZone(awh::Chrono::zone_t::ET, summer), -14400);
	// Выполняем проверку обозначения сводной зоны на стандартном времени
	ASSERT_EQ(this->_chrono->format(awh::Chrono::zone_t::ET, winter), "EST");
	// Выполняем проверку обозначения сводной зоны на летнем времени
	ASSERT_EQ(this->_chrono->format(awh::Chrono::zone_t::ET, summer), "EDT");
	/**
	 * Перегрузки, даты не принимающие, отвечают по текущему моменту, и ответ их равен
	 * ответу перегрузки с датой, взятой на этот самый момент
	 */
	// Получаем текущий момент времени
	const uint64_t current = this->_chrono->timestamp(awh::Chrono::type_t::MILLISECONDS);
	// Выполняем проверку смещения сводной зоны по текущему моменту
	ASSERT_EQ(this->_chrono->getTimeZone(awh::Chrono::zone_t::ET),
		this->_chrono->getTimeZone(awh::Chrono::zone_t::ET, current));
	// Выполняем проверку обозначения сводной зоны по текущему моменту
	ASSERT_EQ(this->_chrono->format(awh::Chrono::zone_t::ET),
		this->_chrono->format(awh::Chrono::zone_t::ET, current));
	/**
	 * Смещение зон, сводными не являющихся, от момента не зависит вовсе, и обе
	 * перегрузки дают у них один и тот же ответ всегда
	 */
	// Перечень зон, сводными не являющихся
	const awh::Chrono::zone_t zones[] = {
		awh::Chrono::zone_t::UTC, awh::Chrono::zone_t::MSK,
		awh::Chrono::zone_t::EST, awh::Chrono::zone_t::EDT
	};
	// Выполняем перебор всех зон, сводными не являющихся
	for(auto zone : zones){
		// Выполняем проверку смещения зоны на стандартном времени
		ASSERT_EQ(this->_chrono->getTimeZone(zone), this->_chrono->getTimeZone(zone, winter));
		// Выполняем проверку смещения зоны на летнем времени
		ASSERT_EQ(this->_chrono->getTimeZone(zone), this->_chrono->getTimeZone(zone, summer));
		// Выполняем проверку обозначения зоны на стандартном времени
		ASSERT_EQ(this->_chrono->format(zone), this->_chrono->format(zone, winter));
		// Выполняем проверку обозначения зоны на летнем времени
		ASSERT_EQ(this->_chrono->format(zone), this->_chrono->format(zone, summer));
	}
	/**
	 * Разбор и формирование записей идут через перегрузки, дату принимающие, и от
	 * текущего момента не зависят: обозначение сводной зоны разрешается по самой записи
	 */
	// Выполняем проверку разбора записи стандартного времени зоны
	ASSERT_EQ(this->_chrono->parse("2025-01-15T12:00:00 ET", "%Y-%m-%dT%H:%M:%S %Z"), winter + 18000000ULL);
	// Выполняем проверку разбора записи летнего времени зоны
	ASSERT_EQ(this->_chrono->parse("2025-07-15T12:00:00 ET", "%Y-%m-%dT%H:%M:%S %Z"), summer + 14400000ULL);
	// Выполняем проверку формирования записи стандартного времени зоны
	ASSERT_EQ(this->_chrono->format(winter, awh::Chrono::zone_t::ET, "%H:%M %z %Z"), "07:00 -0500 EST");
	// Выполняем проверку формирования записи летнего времени зоны
	ASSERT_EQ(this->_chrono->format(summer, awh::Chrono::zone_t::ET, "%H:%M %z %Z"), "08:00 -0400 EDT");
}

/**
 * @brief Тест согласования выводных признаков установкой времени суток
 *
 * @details Метка времени суток и признак летнего времени задающими полями не являются,
 *          а из них выводятся. Календарные единицы согласовывали их разложением штампа,
 *          а установка часа, минут либо секунд разложения не вызывает вовсе, и признаки
 *          оставались от прежней даты: час, выставленный пополудни поверх утреннего,
 *          метку времени суток не менял, отчего переменная формата %r печатала
 *          "03:00:00 AM"
 *
 */
TEST_F(ChronoFixture, ExecutionSetHourFlagsChronoTest){
	// Выполняем разбор записи утреннего времени
	this->_chrono->parse("2025-01-15T09:00:00+0000", "%Y-%m-%dT%H:%M:%S%z", awh::Chrono::storage_t::LOCAL);
	// Выполняем проверку метки времени суток до установки часа
	ASSERT_EQ(this->_chrono->format("%H %I %p %r", awh::Chrono::storage_t::LOCAL), "09 09 AM 09:00:00 AM");
	// Выполняем установку часа пополудни
	this->_chrono->set(15, awh::Chrono::unit_t::HOUR);
	// Выполняем проверку согласия метки времени суток с часом
	ASSERT_EQ(this->_chrono->format("%H %I %p %r", awh::Chrono::storage_t::LOCAL), "15 03 PM 03:00:00 PM");
	// Выполняем проверку метки времени суток, объектом хранимой
	ASSERT_EQ(this->_chrono->h12(awh::Chrono::storage_t::LOCAL), awh::Chrono::h12_t::PM);
	// Выполняем установку часа до полудня
	this->_chrono->set(9, awh::Chrono::unit_t::HOUR);
	// Выполняем проверку возврата метки времени суток
	ASSERT_EQ(this->_chrono->format("%H %I %p %r", awh::Chrono::storage_t::LOCAL), "09 09 AM 09:00:00 AM");
	// Выполняем проверку метки времени суток, объектом хранимой
	ASSERT_EQ(this->_chrono->h12(awh::Chrono::storage_t::LOCAL), awh::Chrono::h12_t::AM);
	/**
	 * Признак летнего времени согласуется установкой часа наравне с меткой времени
	 * суток, и не у одних лишь сводных зон: у зон Северной Америки с постоянным
	 * смещением он выводится из полей объекта по тем же правилам
	 */
	// Выполняем установку временной зоны с постоянным смещением
	this->_chrono->setTimeZone(awh::Chrono::zone_t::EST);
	// Выполняем разбор записи, приходящейся на самый день весеннего перехода
	this->_chrono->parse("2025-03-09T01:30:00", "%Y-%m-%dT%H:%M:%S", awh::Chrono::storage_t::LOCAL);
	// Выполняем проверку признака летнего времени до перехода
	ASSERT_FALSE(this->_chrono->dst(awh::Chrono::storage_t::LOCAL));
	// Выполняем перевод записи за границу весеннего перехода
	this->_chrono->set(3, awh::Chrono::unit_t::HOUR);
	// Выполняем проверку признака летнего времени после перехода
	ASSERT_TRUE(this->_chrono->dst(awh::Chrono::storage_t::LOCAL));
}

/**
 * @brief Тест повторной установки сводной временной зоны
 *
 * @details Перекладка момент времени сохраняет, и повторная установка той же зоны
 *          записи не меняет: смещение разрешается по самому моменту, а не по текущему
 *          мгновению, и потому от числа перекладок не зависит
 *
 */
TEST_F(ChronoFixture, ExecutionRepeatTimeZoneChronoTest){
	// Перечень записей по обе стороны перехода с ожидаемым разрешением зоны
	const struct {
		const char * record;
		const char * result;
	} records[] = {
		{"2025-01-15T17:00:00+0000", "2025-01-15T12:00:00 -0500 EST"},
		{"2025-07-15T16:00:00+0000", "2025-07-15T12:00:00 -0400 EDT"}
	};
	// Выполняем перебор всех записей
	for(auto & item : records){
		// Выполняем разбор записи в нулевой временной зоне
		this->_chrono->parse(item.record, "%Y-%m-%dT%H:%M:%S%z", awh::Chrono::storage_t::LOCAL);
		// Выполняем троекратную перекладку записи в сводную временную зону
		for(uint8_t i = 0; i < 3; i++){
			// Выполняем перекладку записи в сводную временную зону
			this->_chrono->setTimeZone(awh::Chrono::zone_t::ET);
			// Выполняем проверку неизменности записи от числа перекладок
			ASSERT_EQ(this->_chrono->format("%Y-%m-%dT%H:%M:%S %z %Z", awh::Chrono::storage_t::LOCAL), item.result)
				<< item.record << " " << static_cast <uint16_t> (i);
		}
	}
}

/**
 * @brief Тест отката года у записи со сводной временной зоной
 *
 * @details Откат года считался по штампу времени, собранному до разрешения зоны, а до
 *          него штамп собран по стандартному смещению и на летней записи завышен на
 *          час. У самого порога допуска этот час давал ложный откат, а разрешение зоны
 *          год уже не возвращало
 *
 */
TEST_F(ChronoFixture, ExecutionYearRollbackCompositeChronoTest){
	// Выполняем установку допуска отката года
	this->_chrono->yearRollback(26 * 3600);
	// Выполняем разбор записи, задающей опорный момент
	this->_chrono->parse("2025-07-15T12:00:00+0000", "%Y-%m-%dT%H:%M:%S%z", awh::Chrono::storage_t::LOCAL);
	// Выполняем установку сводной временной зоны объекту
	this->_chrono->setTimeZone(awh::Chrono::zone_t::ET);
	// Выполняем разбор записи, года не содержащей
	this->_chrono->parse("Jul 16 09:30:00 ET", "%b %d %H:%M:%S %Z", awh::Chrono::storage_t::LOCAL);
	// Выполняем проверку отсутствия ложного отката года
	ASSERT_EQ(this->_chrono->format("%Y-%m-%dT%H:%M:%S %z %Z", awh::Chrono::storage_t::LOCAL),
		"2025-07-16T09:30:00 -0400 EDT");
	/**
	 * Запись, за допуск уходящую, откат года по-прежнему возвращает на год назад
	 */
	// Выполняем разбор записи, за допуск уходящей
	this->_chrono->parse("Jul 20 09:30:00 ET", "%b %d %H:%M:%S %Z", awh::Chrono::storage_t::LOCAL);
	// Выполняем проверку отката года
	ASSERT_EQ(this->_chrono->format("%Y-%m-%dT%H:%M:%S %z %Z", awh::Chrono::storage_t::LOCAL),
		"2024-07-20T09:30:00 -0400 EDT");
	// Выполняем сброс допуска отката года
	this->_chrono->yearRollback(0);
}

/**
 * @brief Тест приведения долей секунды и шестидесятой секунды установкой
 *
 * @details Количество миллисекунд писалось как есть, тогда как час, минуты и секунды
 *          промежутком ограничены: значение 1500 проверка записи отвергала, а
 *          формирование печатало его целиком и ширину поля ломало. Шестидесятая секунда
 *          установкой отвергалась, тогда как разбор записи её принимает и переносит на
 *          следующую минуту
 *
 */
TEST_F(ChronoFixture, ExecutionSetFractionBoundsChronoTest){
	// Выполняем разбор записи начала минуты
	this->_chrono->parse("2025-01-15T12:00:00.000+0000", "%Y-%m-%dT%H:%M:%S.%s%z", awh::Chrono::storage_t::LOCAL);
	// Выполняем установку количества миллисекунд за пределом доли секунды
	this->_chrono->set(1500, awh::Chrono::unit_t::MILLISECONDS);
	// Выполняем проверку отказа принять значение за пределом доли секунды
	ASSERT_EQ(this->_chrono->format("%Y-%m-%dT%H:%M:%S.%s", awh::Chrono::storage_t::LOCAL), "2025-01-15T12:00:00.000");
	// Выполняем установку количества миллисекунд у самого предела
	this->_chrono->set(999, awh::Chrono::unit_t::MILLISECONDS);
	// Выполняем проверку установки количества миллисекунд у самого предела
	ASSERT_EQ(this->_chrono->format("%Y-%m-%dT%H:%M:%S.%s", awh::Chrono::storage_t::LOCAL), "2025-01-15T12:00:00.999");
	// Выполняем разбор записи начала минуты
	this->_chrono->parse("2025-01-15T12:00:00+0000", "%Y-%m-%dT%H:%M:%S%z", awh::Chrono::storage_t::LOCAL);
	// Выполняем установку шестидесятой секунды
	this->_chrono->set(60, awh::Chrono::unit_t::SECONDS);
	// Выполняем проверку переноса шестидесятой секунды на следующую минуту
	ASSERT_EQ(this->_chrono->format("%Y-%m-%dT%H:%M:%S", awh::Chrono::storage_t::LOCAL), "2025-01-15T12:01:00");
	// Выполняем установку шестьдесят первой секунды
	this->_chrono->set(61, awh::Chrono::unit_t::SECONDS);
	// Выполняем проверку отказа принять шестьдесят первую секунду
	ASSERT_EQ(this->_chrono->format("%Y-%m-%dT%H:%M:%S", awh::Chrono::storage_t::LOCAL), "2025-01-15T12:01:00");
}

/**
 * @brief Тест приведения числовых полей записи к календарю
 *
 * @details Разбор числовые поля вне промежутка не отвергает, а приводит к календарю:
 *          тринадцатый месяц становится январём следующего года, тридцать второе число
 *          - первым числом следующего месяца, шестьдесят первая секунда - первой
 *          секундой следующей минуты. Судит о годности записи validate, и он такие поля
 *          отвергает. Тест закрепляет разделение обязанностей, чтобы разбор кода не
 *          выдавал его за расхождение
 *
 */
TEST_F(ChronoFixture, ExecutionParseOutOfRangeChronoTest){
	// Перечень записей с полями вне промежутка и их приведением к календарю
	const struct {
		const char * record;
		const char * result;
	} records[] = {
		{"2025-13-06T12:00:00", "2026-01-06T12:00:00"},
		{"2025-01-32T12:00:00", "2025-02-01T12:00:00"},
		{"2025-01-15T12:00:61", "2025-01-15T12:01:01"},
		{"2025-01-15T12:00:99", "2025-01-15T12:01:39"},
		{"2025-01-15T25:00:00", "2025-01-16T01:00:00"}
	};
	// Выполняем перебор всех записей с полями вне промежутка
	for(auto & item : records){
		// Выполняем проверку приведения полей записи к календарю
		ASSERT_EQ(this->_chrono->format(this->_chrono->parse(item.record, "%Y-%m-%dT%H:%M:%S"), 0, "%Y-%m-%dT%H:%M:%S"),
			item.result) << item.record;
		// Выполняем проверку отказа признать запись годной
		ASSERT_FALSE(this->_chrono->validate(item.record, "%Y-%m-%dT%H:%M:%S")) << item.record;
	}
}

/**
 * @brief Тест правой границы промежутка календаря
 *
 * @details Метод end выдаёт правую границу полуинтервала [begin, end), то есть само
 *          начало следующего периода, а не последнее его мгновение. На краю календаря
 *          начала следующего периода не существует, и граница насыщается последним
 *          представимым мгновением
 *
 */
TEST_F(ChronoFixture, ExecutionEndExclusiveChronoTest){
	// Выполняем проверку правой границы суток начала эпохи
	ASSERT_EQ(this->_chrono->end(0, awh::Chrono::type_t::DAY), 86400000ULL);
	// Выполняем проверку левой границы суток начала эпохи
	ASSERT_EQ(this->_chrono->begin(0, awh::Chrono::type_t::DAY), 0ULL);
	// Выполняем проверку равенства правой границы началу следующего периода
	ASSERT_EQ(this->_chrono->end(0, awh::Chrono::type_t::DAY),
		this->_chrono->begin(86400000ULL, awh::Chrono::type_t::DAY));
	// Выполняем проверку правой границы месяца начала эпохи
	ASSERT_EQ(this->_chrono->end(0, awh::Chrono::type_t::MONTH),
		this->_chrono->begin(2678400000ULL, awh::Chrono::type_t::MONTH));
}

/**
 * @brief Тест отказа установить значение вне разрядности поля
 *
 * @details Значение, в разрядность поля не умещающееся, объявлено отброшенным, но
 *          переменная под него заводилась от нуля, а не от нынешнего значения поля:
 *          отброшенное значение обнуляло поле молча. У смещения временной зоны это
 *          уводило объект в UTC, поскольку смещение задаёт зону целиком
 *
 */
TEST_F(ChronoFixture, ExecutionSetOutOfWidthChronoTest){
	// Значение, ни в одну разрядность поля не умещающееся
	const int64_t huge = (1LL << 40);
	// Перечень единиц данных с их переменными формата
	const struct {
		awh::Chrono::unit_t unit;
		const char * format;
	} units[] = {
		{awh::Chrono::unit_t::HOUR,    "%H"},
		{awh::Chrono::unit_t::MINUTES, "%M"},
		{awh::Chrono::unit_t::SECONDS, "%S"},
		{awh::Chrono::unit_t::DATE,    "%d"},
		{awh::Chrono::unit_t::MONTH,   "%m"},
		{awh::Chrono::unit_t::YEAR,    "%Y"},
		{awh::Chrono::unit_t::OFFSET,  "%z"}
	};
	// Выполняем перебор всех единиц данных
	for(auto & item : units){
		// Выполняем разбор опорной записи
		this->_chrono->parse("2025-01-15T12:30:45+0300", "%Y-%m-%dT%H:%M:%S%z", awh::Chrono::storage_t::LOCAL);
		// Запоминаем запись объекта до установки
		const std::string before = this->_chrono->format("%Y-%m-%dT%H:%M:%S %z %Z", awh::Chrono::storage_t::LOCAL);
		// Выполняем установку значения вне разрядности поля
		this->_chrono->set(huge, item.unit);
		// Выполняем проверку неизменности записи объекта
		ASSERT_EQ(this->_chrono->format("%Y-%m-%dT%H:%M:%S %z %Z", awh::Chrono::storage_t::LOCAL), before) << item.format;
	}
	/**
	 * Смещение временной зоны, числом не являющееся, записи не двигает наравне со
	 * значением вне разрядности поля
	 */
	// Выполняем разбор опорной записи
	this->_chrono->parse("2025-01-15T12:30:45+0300", "%Y-%m-%dT%H:%M:%S%z", awh::Chrono::storage_t::LOCAL);
	// Выполняем установку смещения временной зоны текстом, числом не являющимся
	this->_chrono->set(std::string("мусор"), awh::Chrono::unit_t::OFFSET);
	// Выполняем проверку неизменности записи объекта
	ASSERT_EQ(this->_chrono->format("%Y-%m-%dT%H:%M:%S %z %Z", awh::Chrono::storage_t::LOCAL),
		"2025-01-15T12:30:45 +0300 UTC+3");
	/**
	 * Значение, в разрядность поля умещающееся, по-прежнему принимается
	 */
	// Выполняем установку смещения временной зоны текстом
	this->_chrono->set(std::string("-18000"), awh::Chrono::unit_t::OFFSET);
	// Выполняем проверку установки смещения временной зоны
	ASSERT_EQ(this->_chrono->format("%Y-%m-%dT%H:%M:%S %z", awh::Chrono::storage_t::LOCAL), "2025-01-15T04:30:45 -0500");
	// Выполняем установку часа
	this->_chrono->set(7, awh::Chrono::unit_t::HOUR);
	// Выполняем проверку установки часа
	ASSERT_EQ(this->_chrono->format("%Y-%m-%dT%H:%M:%S %z", awh::Chrono::storage_t::LOCAL), "2025-01-15T07:30:45 -0500");
}

/**
 * @brief Тест однократного разрешения смещения зоны при установке единицы
 *
 * @details Установка календарной единицы согласует поля объекта, а согласование
 *          заканчивается разрешением смещения зоны, и второй раз разрешать его незачем.
 *          Тест закрепляет, что отказ от повторного разрешения ответа не изменил:
 *          сводная зона, переведённая установкой месяца через границу перехода, обязана
 *          получить летнее смещение, а переведённая обратно - стандартное
 *
 */
TEST_F(ChronoFixture, ExecutionSetResolveOnceChronoTest){
	// Перечень календарных единиц, согласованием полей заканчивающихся
	const struct {
		const char * record;
		awh::Chrono::unit_t unit;
		int64_t value;
		const char * result;
	} records[] = {
		{"2025-01-15T12:00:00", awh::Chrono::unit_t::MONTH, 7,   "2025-07-15T12:00:00 -0400 EDT"},
		{"2025-07-15T12:00:00", awh::Chrono::unit_t::MONTH, 1,   "2025-01-15T12:00:00 -0500 EST"},
		{"2025-01-15T12:00:00", awh::Chrono::unit_t::DAYS,  200, "2025-07-20T12:00:00 -0400 EDT"},
		{"2025-07-15T12:00:00", awh::Chrono::unit_t::DAYS,  20,  "2025-01-21T12:00:00 -0500 EST"},
		{"2025-01-15T12:00:00", awh::Chrono::unit_t::DATE,  20,  "2025-01-20T12:00:00 -0500 EST"},
		{"2025-01-15T12:00:00", awh::Chrono::unit_t::YEAR,  2024, "2024-01-15T12:00:00 -0500 EST"}
	};
	// Выполняем перебор всех календарных единиц
	for(auto & item : records){
		// Устанавливаем сводную временную зону Северной Америки
		this->_chrono->setTimeZone(awh::Chrono::zone_t::ET);
		// Выполняем разбор опорной записи
		this->_chrono->parse(item.record, "%Y-%m-%dT%H:%M:%S", awh::Chrono::storage_t::LOCAL);
		// Выполняем установку календарной единицы
		this->_chrono->set(item.value, item.unit);
		// Выполняем проверку разрешения смещения сводной зоны по новой дате
		ASSERT_EQ(this->_chrono->format("%Y-%m-%dT%H:%M:%S %z %Z", awh::Chrono::storage_t::LOCAL),
			item.result) << item.record;
	}
}

/**
 * @brief Тест согласия начала года, выведенного разными путями
 *
 * @details Начало года извлечению года попутно и выводится вместе с ним за один проход,
 *          тогда как метод begin считает его отдельным расчётом. Оба пути обязаны
 *          сходиться на всём промежутке представимости: тест сличает их на каждом году
 *          календаря и на обеих границах каждого года, чтобы уточнение года по остатку
 *          проверялось и слева, и справа от границы
 *
 */
TEST_F(ChronoFixture, ExecutionYearBeginAgreementChronoTest){
	// Выполняем перебор всех годов промежутка представимости
	for(uint16_t year = 1970; year < 10000; year++){
		// Получаем начало года отдельным расчётом
		const uint64_t begin = this->_chrono->begin(
			this->_chrono->parse(std::to_string(year) + "-01-01T00:00:00", "%Y-%m-%dT%H:%M:%S"),
			awh::Chrono::type_t::YEAR
		);
		// Выполняем проверку согласия года, извлечённого из начала года
		ASSERT_EQ(this->_chrono->get <uint16_t> (begin, awh::Chrono::unit_t::YEAR), year) << year;
		// Выполняем проверку согласия начала года, полученного из него же
		ASSERT_EQ(this->_chrono->begin(begin, awh::Chrono::type_t::YEAR), begin) << year;
		// Если год не последний в промежутке представимости
		if(year > 1970){
			// Получаем последнее мгновение предыдущего года
			const uint64_t last = (begin - 1);
			// Выполняем проверку согласия года слева от границы
			ASSERT_EQ(this->_chrono->get <uint16_t> (last, awh::Chrono::unit_t::YEAR), (year - 1)) << year;
			// Выполняем проверку согласия начала предыдущего года
			ASSERT_LT(this->_chrono->begin(last, awh::Chrono::type_t::YEAR), begin) << year;
		}
	}
}

/**
 * @brief Тест проверки номера дня недели записью
 *
 * @details Номер дня недели переменные формата %u и %w задают напрямую, и запись «9»
 *          разбор проходила: сам он день недели выводит из даты заново и потому
 *          значения не замечает, а проверка записи его не смотрела вовсе
 *
 */
TEST_F(ChronoFixture, ExecutionValidateDayOfWeekChronoTest){
	// Выполняем проверку отказа признать годным нулевой день недели
	ASSERT_FALSE(this->_chrono->validate("2025-04-06 0", "%Y-%m-%d %u"));
	// Выполняем проверку отказа признать годным девятый день недели
	ASSERT_FALSE(this->_chrono->validate("2025-04-06 9", "%Y-%m-%d %u"));
	// Выполняем проверку признания годным дня недели из промежутка
	ASSERT_TRUE(this->_chrono->validate("2025-04-06 7", "%Y-%m-%d %u"));
	/**
	 * Переменная формата %w счёт ведёт от нуля до шести, воскресенье обозначая нулём:
	 * седьмой день недели ей не принадлежит, и запись «7» прежде читалась воскресеньем
	 * наравне с нулём - после перевода нуля в семёрку сырое значение от переведённого
	 * отличить было нельзя
	 */
	// Выполняем перебор всех значений от нуля до девяти
	for(uint8_t i = 0; i < 10; i++){
		// Формируем запись с текущим номером дня недели
		const std::string record = ("2025-04-06 " + std::to_string(static_cast <uint16_t> (i)));
		// Выполняем проверку промежутка переменной %w, ведущей счёт от нуля до шести
		ASSERT_EQ(this->_chrono->validate(record, "%Y-%m-%d %w"), (i < 7)) << record;
		// Выполняем проверку промежутка переменной %u, ведущей счёт от одного до семи
		ASSERT_EQ(this->_chrono->validate(record, "%Y-%m-%d %u"), ((i > 0) && (i < 8))) << record;
	}
	// Выполняем проверку разбора воскресенья переменной %w
	ASSERT_EQ(this->_chrono->format(this->_chrono->parse("2025-04-06 0", "%Y-%m-%d %w"), 0, "%A"), "Sunday");
	/**
	 * Записи без дня недели проверка касаться не должна: поле выводится из даты и
	 * потому в промежутке лежит всегда
	 */
	// Выполняем проверку признания годной записи без дня недели
	ASSERT_TRUE(this->_chrono->validate("2025-04-06", "%Y-%m-%d"));
	/**
	 * Разбор день недели выводит из даты заново, и запись вне промежутка объекта не
	 * портит: названия дней недели читаются из таблицы по выведенному номеру
	 */
	// Выполняем разбор записи с днём недели вне промежутка
	this->_chrono->parse("2025-04-06 9", "%Y-%m-%d %u", awh::Chrono::storage_t::LOCAL);
	// Выполняем проверку вывода дня недели из самой даты
	ASSERT_EQ(this->_chrono->format("%Y-%m-%d %u %a %A", awh::Chrono::storage_t::LOCAL), "2025-04-06 7 Sun Sunday");
}

/**
 * @brief Тест независимости сборки штампа времени от выводной високосности
 *
 * @details Високосность года - поле выводное, и сборка штампа времени считает её от
 *          самого года, а не берёт из поля: расхождение поля с годом сдвинуло бы дату
 *          на сутки, а опора на выводное поле ставит сборку в зависимость от того,
 *          обновил ли его тот, кто год менял
 *
 */
TEST_F(ChronoFixture, ExecutionLeapDerivedChronoTest){
	// Перечень записей конца февраля по обе стороны високосности
	const char * records[] = {
		"2024-02-29T12:00:00+0000", "2024-03-01T12:00:00+0000",
		"2025-02-28T12:00:00+0000", "2025-03-01T12:00:00+0000",
		"2100-02-28T12:00:00+0000", "2000-02-29T12:00:00+0000"
	};
	// Выполняем перебор всех записей конца февраля
	for(const char * record : records){
		// Выполняем разбор записи конца февраля
		this->_chrono->parse(record, "%Y-%m-%dT%H:%M:%S%z", awh::Chrono::storage_t::LOCAL);
		// Выполняем проверку обратного формирования записи
		ASSERT_EQ(this->_chrono->format("%Y-%m-%dT%H:%M:%S%z", awh::Chrono::storage_t::LOCAL), record);
		// Выполняем проверку согласия признака високосности с годом
		ASSERT_EQ(this->_chrono->leap(awh::Chrono::storage_t::LOCAL),
			this->_chrono->leap(this->_chrono->get <uint16_t> (awh::Chrono::unit_t::YEAR, awh::Chrono::storage_t::LOCAL))) << record;
	}
	/**
	 * Установка года високосность пересчитывает, и число месяца приводится к длине
	 * февраля того года, на который дата переносится
	 */
	// Выполняем разбор записи високосного двадцать девятого февраля
	this->_chrono->parse("2024-02-29T12:00:00+0000", "%Y-%m-%dT%H:%M:%S%z", awh::Chrono::storage_t::LOCAL);
	// Выполняем перенос записи на невисокосный год
	this->_chrono->set(2025, awh::Chrono::unit_t::YEAR);
	// Выполняем проверку приведения числа месяца к длине февраля
	ASSERT_EQ(this->_chrono->format("%Y-%m-%dT%H:%M:%S", awh::Chrono::storage_t::LOCAL), "2025-02-28T12:00:00");
}

/**
 * @brief Тест границ периода в зоне объекта
 *
 * @details Местное хранилище означает зону объекта всюду - и в формировании записи, и в
 *          чтении единиц, и в признаке летнего времени, - а границы периода
 *          отсчитывались от нулевой зоны, отчего начало суток записи, лежащей в зоне
 *          UTC+3, приходилось на три часа утра
 *
 */
TEST_F(ChronoFixture, ExecutionLocalBoundsChronoTest){
	// Выполняем разбор записи в зоне, от нулевой отстоящей
	this->_chrono->parse("2025-04-06T14:30:00+0300", "%Y-%m-%dT%H:%M:%S%z", awh::Chrono::storage_t::LOCAL);
	// Получаем начало суток местного хранилища
	const uint64_t begin = this->_chrono->begin(awh::Chrono::type_t::DAY, awh::Chrono::storage_t::LOCAL);
	// Получаем конец суток местного хранилища
	const uint64_t end = this->_chrono->end(awh::Chrono::type_t::DAY, awh::Chrono::storage_t::LOCAL);
	// Выполняем проверку начала суток по местной полуночи зоны объекта
	ASSERT_EQ(this->_chrono->format(begin, 10800, "%Y-%m-%dT%H:%M:%S%z"), "2025-04-06T00:00:00+0300");
	// Выполняем проверку конца суток по местной полуночи зоны объекта
	ASSERT_EQ(this->_chrono->format(end, 10800, "%Y-%m-%dT%H:%M:%S%z"), "2025-04-07T00:00:00+0300");
	// Выполняем проверку длины суток между границами
	ASSERT_EQ(end - begin, 86400000ULL);
	/**
	 * Штамп времени зоны не несёт и отсчитывается от начала эпохи в нулевой: границы
	 * выдаются моментами времени, а не полями объекта
	 */
	// Выполняем проверку начала суток в нулевой зоне
	ASSERT_EQ(this->_chrono->format(begin, 0, "%Y-%m-%dT%H:%M:%S"), "2025-04-05T21:00:00");
	/**
	 * Границы общего хранилища отсчитываются в нулевой зоне: момент времени зоны не
	 * несёт, и брать её неоткуда
	 */
	// Выполняем проверку границ общего хранилища по нулевой зоне
	ASSERT_EQ(this->_chrono->begin(this->_chrono->parse("2025-04-06T14:30:00+0300", "%Y-%m-%dT%H:%M:%S%z"),
		awh::Chrono::type_t::DAY), 1743897600000ULL);
	/**
	 * Запись, лежащая в нулевой зоне, обоими хранилищами обсчитывается одинаково
	 */
	// Выполняем разбор записи в нулевой временной зоне
	this->_chrono->parse("2025-04-06T14:30:00+0000", "%Y-%m-%dT%H:%M:%S%z", awh::Chrono::storage_t::LOCAL);
	// Выполняем проверку совпадения границ обоих хранилищ в нулевой зоне
	ASSERT_EQ(this->_chrono->begin(awh::Chrono::type_t::DAY, awh::Chrono::storage_t::LOCAL),
		this->_chrono->begin(this->_chrono->timestamp(awh::Chrono::type_t::MILLISECONDS, awh::Chrono::storage_t::LOCAL),
			awh::Chrono::type_t::DAY));
}

/**
 * @brief Тест несверки дня недели с календарной датой
 *
 * @details Проверка записи смотрит промежуток дня недели, но самому календарю его не
 *          сверяет: запись "Mon, 06 Apr 2025" проверку проходит, хотя шестое апреля
 *          2025 года - воскресенье. Так поступает и strptime стандарта POSIX: поле
 *          выводное, дату оно не задаёт, и разбор выводит его заново
 *
 */
TEST_F(ChronoFixture, ExecutionDayOfWeekMismatchChronoTest){
	// Выполняем проверку признания годной записи с днём недели, дате не отвечающим
	ASSERT_TRUE(this->_chrono->validate("Mon, 06 Apr 2025", "%a, %d %b %Y"));
	// Выполняем проверку вывода дня недели из самой даты
	ASSERT_EQ(this->_chrono->format(this->_chrono->parse("Mon, 06 Apr 2025", "%a, %d %b %Y"), 0, "%Y-%m-%d %A"),
		"2025-04-06 Sunday");
	// Выполняем проверку вывода дня недели из даты и при отвечающем ей названии
	ASSERT_EQ(this->_chrono->format(this->_chrono->parse("Sun, 06 Apr 2025", "%a, %d %b %Y"), 0, "%Y-%m-%d %A"),
		"2025-04-06 Sunday");
}

/**
 * @brief Тест календарного смещения в зоне объекта
 *
 * @details Длина года и месяца от самой даты зависит, а смещение местного хранилища
 *          считалось в нулевой зоне: запись 2025-01-31T01:00+0300 лежит тридцатым
 *          января в нулевой зоне, и прибавление месяца к ней давало первое марта
 *          вместо двадцать восьмого февраля
 *
 */
TEST_F(ChronoFixture, ExecutionLocalOffsetChronoTest){
	// Перечень записей с их смещением на единицу календаря
	const struct {
		const char * record;
		awh::Chrono::type_t type;
		const char * result;
	} records[] = {
		{"2025-01-31T01:00:00+0300", awh::Chrono::type_t::MONTH, "2025-02-28T01:00:00+0300"},
		{"2024-02-29T01:00:00+0300", awh::Chrono::type_t::YEAR,  "2025-02-28T01:00:00+0300"},
		{"2025-01-31T01:00:00+0300", awh::Chrono::type_t::DAY,   "2025-02-01T01:00:00+0300"},
		{"2025-03-31T01:00:00-0500", awh::Chrono::type_t::MONTH, "2025-04-30T01:00:00-0500"}
	};
	// Выполняем перебор всех записей
	for(auto & item : records){
		// Выполняем разбор записи в её временной зоне
		this->_chrono->parse(item.record, "%Y-%m-%dT%H:%M:%S%z", awh::Chrono::storage_t::LOCAL);
		// Получаем смещение временной зоны записи
		const int32_t zone = this->_chrono->getTimeZone(awh::Chrono::storage_t::LOCAL);
		// Выполняем смещение записи на единицу календаря
		const uint64_t date = this->_chrono->offset(1, item.type, awh::Chrono::offset_t::INCREMENT, awh::Chrono::storage_t::LOCAL);
		// Выполняем проверку смещения, посчитанного в зоне объекта
		ASSERT_EQ(this->_chrono->format(date, zone, "%Y-%m-%dT%H:%M:%S%z"), item.result) << item.record;
	}
	/**
	 * Смещение той же записи на единицу календаря установкой единицы данных даёт тот же
	 * ответ: обе части договора считают календарь в зоне объекта
	 */
	// Выполняем разбор записи последнего дня января
	this->_chrono->parse("2025-01-31T01:00:00+0300", "%Y-%m-%dT%H:%M:%S%z", awh::Chrono::storage_t::LOCAL);
	// Выполняем установку февраля месяцем записи
	this->_chrono->set(2, awh::Chrono::unit_t::MONTH);
	// Выполняем проверку согласия установки единицы со смещением календаря
	ASSERT_EQ(this->_chrono->format("%Y-%m-%dT%H:%M:%S%z", awh::Chrono::storage_t::LOCAL), "2025-02-28T01:00:00+0300");
	/**
	 * Запись, лежащая в нулевой зоне, обоими хранилищами смещается одинаково
	 */
	// Выполняем разбор записи в нулевой временной зоне
	this->_chrono->parse("2025-01-31T01:00:00+0000", "%Y-%m-%dT%H:%M:%S%z", awh::Chrono::storage_t::LOCAL);
	// Выполняем проверку совпадения смещений обоих хранилищ в нулевой зоне
	ASSERT_EQ(this->_chrono->offset(1, awh::Chrono::type_t::MONTH, awh::Chrono::offset_t::INCREMENT, awh::Chrono::storage_t::LOCAL),
		this->_chrono->offset(this->_chrono->timestamp(awh::Chrono::type_t::MILLISECONDS, awh::Chrono::storage_t::LOCAL),
			1, awh::Chrono::type_t::MONTH, awh::Chrono::offset_t::INCREMENT));
}

/**
 * @brief Тест насыщения перевода единиц времени в штамп
 *
 * @details Перевод количества единиц времени в штамп множил их на длину единицы без
 *          проверки: произведение разрядность uint64_t переполняло и давало дату
 *          случайную вместо края календаря, тогда как календарное смещение тем же
 *          пределом насыщается
 *
 */
TEST_F(ChronoFixture, ExecutionTimestampScaleChronoTest){
	// Перечень единиц измерения времени
	const awh::Chrono::type_t types[] = {
		awh::Chrono::type_t::YEAR, awh::Chrono::type_t::MONTH, awh::Chrono::type_t::WEEK,
		awh::Chrono::type_t::DAY,  awh::Chrono::type_t::HOUR,  awh::Chrono::type_t::MINUTES,
		awh::Chrono::type_t::SECONDS
	};
	// Выполняем перебор всех единиц измерения времени
	for(auto type : types){
		// Выполняем установку количества единиц, за предел представимости выводящего
		this->_chrono->timestamp(1000000000000ULL, type);
		// Выполняем проверку насыщения последним представимым мгновением
		ASSERT_EQ(this->_chrono->format("%Y-%m-%dT%H:%M:%S", awh::Chrono::storage_t::LOCAL), "9999-12-31T23:59:59")
			<< static_cast <uint16_t> (type);
	}
	/**
	 * Количество единиц, в предел представимости укладывающееся, переводится по-прежнему
	 */
	// Выполняем установку количества секунд, в предел представимости укладывающегося
	this->_chrono->timestamp(1743943021ULL, awh::Chrono::type_t::SECONDS);
	// Выполняем проверку перевода количества секунд в штамп времени
	ASSERT_EQ(this->_chrono->format("%Y-%m-%dT%H:%M:%S", awh::Chrono::storage_t::LOCAL), "2025-04-06T12:37:01");
}

/**
 * @brief Тест насыщения остатка недель в периоде
 *
 * @details Количество прошедших недель считается округлением и вплотную к концу периода
 *          в него упирается, а разность велась в разрядности числа недель: обгон дал бы
 *          беззнаковый оборот вместо нуля. Ни одна дата календаря до обгона не доводит,
 *          и насыщение здесь защищает сам счёт, а не исправляет известный ответ
 *
 */
TEST_F(ChronoFixture, ExecutionActualWeeksLeftChronoTest){
	// Перечень лет с их последними и первыми мгновениями
	const uint16_t years[] = {1970, 1999, 2000, 2024, 2025, 2100, 9999};
	// Выполняем перебор всех лет
	for(uint16_t year : years){
		// Формируем запись первого мгновения года
		const std::string record = (std::to_string(year) + "-01-01T00:00:00+0000");
		// Выполняем разбор записи первого мгновения года
		const uint64_t begin = this->_chrono->parse(record, "%Y-%m-%dT%H:%M:%S%z");
		// Выполняем перебор границ года и месяца
		for(auto type : {awh::Chrono::type_t::YEAR, awh::Chrono::type_t::MONTH}){
			// Получаем последнее мгновение периода
			const uint64_t last = (this->_chrono->end(begin, type) - 1);
			// Выполняем проверку остатка недель на первом мгновении периода
			ASSERT_LE(this->_chrono->actual(begin, awh::Chrono::type_t::WEEK, type, awh::Chrono::actual_t::LEFT), 53ULL) << record;
			// Выполняем проверку остатка недель на последнем мгновении периода
			ASSERT_LE(this->_chrono->actual(last, awh::Chrono::type_t::WEEK, type, awh::Chrono::actual_t::LEFT), 53ULL) << record;
		}
	}
	/**
	 * Остаток недель в феврале високосного года считается по его настоящей длине
	 */
	// Выполняем разбор записи начала февраля високосного года
	const uint64_t leap = this->_chrono->parse("2024-02-01T00:00:00+0000", "%Y-%m-%dT%H:%M:%S%z");
	// Выполняем проверку остатка недель в феврале високосного года
	ASSERT_EQ(this->_chrono->actual(leap, awh::Chrono::type_t::WEEK, awh::Chrono::type_t::MONTH, awh::Chrono::actual_t::LEFT), 5ULL);
	// Выполняем проверку остатка недель на последнем мгновении февраля високосного года
	ASSERT_EQ(this->_chrono->actual(this->_chrono->end(leap, awh::Chrono::type_t::MONTH) - 1,
		awh::Chrono::type_t::WEEK, awh::Chrono::type_t::MONTH, awh::Chrono::actual_t::LEFT), 1ULL);
}

/**
 * @brief Тест модели аббревиатуры продолжительности
 *
 * @details Месяц в аббревиатуре равен четырём неделям, а год - двенадцати таким месяцам,
 *          тогда как перевод единиц времени в штамп считает месяц средним по календарю.
 *          Расхождение намеренное: аббревиатура служит показу продолжительности человеку,
 *          и «два месяца» в ней означает восемь недель, а не отрезок календаря
 *
 */
TEST_F(ChronoFixture, ExecutionAbbreviationModelChronoTest){
	// Выполняем проверку месяца аббревиатуры длиной в четыре недели
	ASSERT_EQ(this->_chrono->abbreviation(4ULL * 604800000ULL).first, awh::Chrono::type_t::MONTH);
	// Выполняем проверку значения месяца аббревиатуры длиной в четыре недели
	ASSERT_DOUBLE_EQ(this->_chrono->abbreviation(4ULL * 604800000ULL).second, 1.0);
	// Выполняем проверку года аббревиатуры длиной в сорок восемь недель
	ASSERT_EQ(this->_chrono->abbreviation(48ULL * 604800000ULL).first, awh::Chrono::type_t::YEAR);
	// Выполняем проверку значения года аббревиатуры длиной в сорок восемь недель
	ASSERT_DOUBLE_EQ(this->_chrono->abbreviation(48ULL * 604800000ULL).second, 1.0);
	/**
	 * Календарный счёт от аббревиатуры не зависит: год перевода единиц времени равен
	 * тремстам шестидесяти пяти суткам
	 */
	// Выполняем установку одного года количеством единиц времени
	this->_chrono->timestamp(1, awh::Chrono::type_t::YEAR);
	// Выполняем проверку года календарного счёта длиной в триста шестьдесят пять суток
	ASSERT_EQ(this->_chrono->format("%Y-%m-%dT%H:%M:%S", awh::Chrono::storage_t::LOCAL), "1971-01-01T00:00:00");
}
