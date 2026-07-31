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
