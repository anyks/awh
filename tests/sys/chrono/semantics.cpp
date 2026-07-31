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
