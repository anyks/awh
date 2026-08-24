/**
 * @file date.cpp
 * @date 2026-08-02
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
 * @brief Пример работы с модулем даты и времени — разбор и формирование записей дат по строке формата и по
 *        обозначению стандарта, проверка пригодности записи, извлечение и установка составляющих даты,
 *        календарные отрезки и смещения, работа с временными зонами и переходом на летнее время
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/log.hpp>
#include <sys/chrono.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * @brief Главная функция приложения
 *
 * @return код выхода из приложения
 *
 */
int32_t main(){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Устанавливаем объект логера
	fmk.setLogger(&log);
	// Устанавливаем название сервиса
	log.name("Chrono");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");
	// Создаём объект работы с датой и временем
	chrono_t chrono(&fmk, &log);

	/**
	 * Разбор записи даты по строке формата
	 *
	 * Разбор ищет поля по всей записи и разделители между ними не сверяет, поэтому один
	 * образец читает записи, отличающиеся знаками препинания. Строгую сверку записи с
	 * образцом выполняет метод validate.
	 */
	{
		// Записываем в лог заголовок раздела
		log.print("=== Разбор записи даты по строке формата ===", log_t::flag_t::INFO);
		// Список записей дат и образцов, по которым они разбираются
		const std::pair <const char *, const char *> records[] = {
			// Запись с долей секунды и нулевым смещением зоны
			{"2026-04-06T12:37:01.520Z", "%Y-%m-%dT%H:%M:%S.%s%i"},
			// Запись со смещением зоны, заданным двоеточием
			{"2026-04-06T15:37:01.520+03:00", "%Y-%m-%dT%H:%M:%S.%s%o"},
			// Запись журнала веб-сервера
			{"06/Apr/2026:12:37:01 +0000", "%d/%b/%Y:%H:%M:%S %z"},
			// Запись заголовка HTTP
			{"Mon, 06 Apr 2026 12:37:01 GMT", "%a, %d %b %Y %H:%M:%S %Z"},
			// Запись с двенадцатичасовым временем
			{"4/6/2026 2:39:42 PM", "%m/%d/%Y %I:%M:%S %p"},
			// Запись, заданная днём года вместо месяца и числа
			{"2026-096 12:37:01", "%Y-%j %H:%M:%S"},
			// Запись без разделителей
			{"20260406T123701+0330", "%Y%m%dT%H%M%S%z"},
			// Запись с обозначением зоны словом
			{"Dec 03 12:00 MSK", "%b %d %H:%M %Z"}
		};
		// Выполняем перебор всех записей дат
		for(auto & record : records){
			// Выполняем разбор записи даты
			const uint64_t date = chrono.parse(record.first, record.second);
			// Записываем в лог полученный штамп времени и запись даты в едином виде
			log.print(
				"%-30s [%s] => %s (%llu)", log_t::flag_t::INFO, record.first, record.second,
				chrono.format(date, "%Y-%m-%d %H:%M:%S.%s %i").c_str(), date
			);
		}
	}

	/**
	 * Разбор и формирование записи даты по обозначению стандарта
	 *
	 * Обозначение стандарта избавляет от переписывания образца и от ошибок в его мелочах.
	 * Разбор принимает все разновидности записи, стандартом допускаемые, а не одну: RFC 5322
	 * позволяет опустить день недели, RFC 3339 - долю секунды и даёт зону либо буквой Z, либо
	 * смещением, ISO 8601 знает основную и расширенную формы. Записи HTTP - RFC 1123 и RFC 850 -
	 * формируются в нулевой зоне со словом GMT независимо от зоны объекта, как стандарт и требует.
	 */
	{
		// Записываем в лог заголовок раздела
		log.print("=== Записи даты, заданные стандартом ===", log_t::flag_t::INFO);
		// Момент времени, для которого формируются записи
		const uint64_t date = chrono.parse("2026-04-06T12:37:01.520Z", chrono_t::standard_t::RFC3339);
		// Список стандартов, записи которых формируются
		const std::pair <const char *, chrono_t::standard_t> standards[] = {
			{"CLF",     chrono_t::standard_t::CLF},
			{"RFC 850", chrono_t::standard_t::RFC850},
			{"RFC 1123", chrono_t::standard_t::RFC1123},
			{"RFC 3164", chrono_t::standard_t::RFC3164},
			{"RFC 3339", chrono_t::standard_t::RFC3339},
			{"RFC 5322", chrono_t::standard_t::RFC5322},
			{"ISO 8601", chrono_t::standard_t::ISO8601},
			{"asctime", chrono_t::standard_t::ASCTIME}
		};
		// Выполняем перебор всех стандартов
		for(auto & standard : standards){
			// Выполняем формирование записи даты по стандарту
			const string & result = chrono.format(date, standard.second);
			// Записываем в лог полученную запись и итог обратного её разбора
			log.print(
				"%-9s %-34s => %llu", log_t::flag_t::INFO, standard.first,
				result.c_str(), chrono.parse(result, standard.second)
			);
		}
		// Выполняем формирование записи RFC 3339 в зоне, смещённой на три часа
		log.print(
			"RFC 3339 в зоне +03:00: %s", log_t::flag_t::INFO,
			chrono.format(date, 3 * 3600, chrono_t::standard_t::RFC3339).c_str()
		);
	}

	/**
	 * Проверка пригодности записи даты
	 *
	 * В отличие от разбора, проверка требует, чтобы разобрались все переменные образца и чтобы
	 * значения полей уложились в допустимые пределы. Проверка по обозначению стандарта сверяет
	 * ещё и разделители записи.
	 */
	{
		// Записываем в лог заголовок раздела
		log.print("=== Проверка пригодности записи даты ===", log_t::flag_t::INFO);
		// Список записей дат, пригодность которых проверяется
		const std::pair <const char *, const char *> records[] = {
			// Пригодная запись
			{"2026-04-06 12:37:01", "%Y-%m-%d %H:%M:%S"},
			// Число месяца за пределами его длины
			{"2026-02-30 12:37:01", "%Y-%m-%d %H:%M:%S"},
			// Записи не хватает секунд, заданных образцом
			{"2026-04-06 12:37", "%Y-%m-%d %H:%M:%S"},
			// Час за пределами суток
			{"2026-04-06 24:37:01", "%Y-%m-%d %H:%M:%S"}
		};
		// Выполняем перебор всех записей дат
		for(auto & record : records)
			// Записываем в лог итог проверки записи даты
			log.print("%-21s [%s] => %s", log_t::flag_t::INFO, record.first, record.second, (chrono.validate(record.first, record.second) ? "пригодна" : "непригодна"));
		// Записываем в лог итог проверки записи заголовка HTTP по стандарту
		log.print("Запись HTTP по стандарту => %s", log_t::flag_t::INFO, (chrono.validate("Mon, 06 Apr 2026 12:37:01 GMT", chrono_t::standard_t::RFC1123) ? "пригодна" : "непригодна"));
		// Записываем в лог итог проверки записи с чужими разделителями
		log.print("Запись с чужими разделителями => %s", log_t::flag_t::INFO, (chrono.validate("Mon 06/Apr/2026 12:37:01 GMT", chrono_t::standard_t::RFC1123) ? "пригодна" : "непригодна"));
		// Записываем в лог итог проверки обозначения временной зоны
		log.print("Обозначение зоны \"+03:30\" => %s", log_t::flag_t::INFO, (chrono.validateTimeZone("+03:30") ? "пригодно" : "непригодно"));
		// Записываем в лог итог проверки обозначения размерности времени
		log.print("Размерность \"90m\" => %s", log_t::flag_t::INFO, (chrono.validateSeconds("90m") ? "пригодна" : "непригодна"));
		// Записываем в лог итог проверки обозначения размерности без единицы
		log.print("Размерность \"42\" => %s", log_t::flag_t::INFO, (chrono.validateSeconds("42") ? "пригодна" : "непригодна"));
	}

	/**
	 * Правило раскрытия двузначного года и приём секунды координации
	 *
	 * Единого правила для года, записанного двумя разрядами, в мире нет: RFC 9110 задаёт
	 * скользящее окно от текущего года, POSIX для strptime - неподвижный рубеж между 68 и 69
	 * годами. Действуют оба, поэтому выбор оставлен за вызывающим.
	 */
	{
		// Записываем в лог заголовок раздела
		log.print("=== Двузначный год и секунда координации ===", log_t::flag_t::INFO);
		// Устанавливаем неподвижный рубеж по правилам POSIX
		chrono.century(chrono_t::century_t::POSIX);
		// Записываем в лог год, раскрытый по правилам POSIX
		log.print("POSIX:  \"70-04-06\" => %s", log_t::flag_t::INFO, chrono.format(chrono.parse("70-04-06", "%y-%m-%d"), "%Y-%m-%d").c_str());
		// Возвращаем скользящее окно по правилам RFC 9110
		chrono.century(chrono_t::century_t::WINDOW);
		// Записываем в лог год, раскрытый по правилам RFC 9110
		log.print("RFC 9110: \"70-04-06\" => %s", log_t::flag_t::INFO, chrono.format(chrono.parse("70-04-06", "%y-%m-%d"), "%Y-%m-%d").c_str());
		// Отключаем приём секунды координации при проверке записи
		chrono.leapSecond(false);
		// Записываем в лог итог проверки записи с секундой координации
		log.print("Секунда координации отклоняется => %s", log_t::flag_t::INFO, (chrono.validate("2026-12-31 23:59:60", "%Y-%m-%d %H:%M:%S") ? "пригодна" : "непригодна"));
		// Возвращаем приём секунды координации, требуемый RFC 3339 и ISO 8601
		chrono.leapSecond(true);
		// Записываем в лог итог проверки записи с секундой координации
		log.print("Секунда координации принимается => %s", log_t::flag_t::INFO, (chrono.validate("2026-12-31 23:59:60", "%Y-%m-%d %H:%M:%S") ? "пригодна" : "непригодна"));
	}

	/**
	 * Извлечение и установка составляющих даты
	 *
	 * Составляющие DAY и MONTH извлекаются и числом, и названием - смотря какой тип задан
	 * шаблонным доводом.
	 */
	{
		// Записываем в лог заголовок раздела
		log.print("=== Составляющие даты ===", log_t::flag_t::INFO);
		// Момент времени, составляющие которого извлекаются
		const uint64_t date = chrono.parse("2026-04-06T12:37:01.520Z", chrono_t::standard_t::RFC3339);
		// Записываем в лог составляющие даты, извлечённые числом
		log.print(
			"Год=%u месяц=%u число=%u час=%u минуты=%u секунды=%u миллисекунды=%u",
			log_t::flag_t::INFO,
			chrono.get <uint16_t> (date, chrono_t::unit_t::YEAR),
			chrono.get <uint8_t> (date, chrono_t::unit_t::MONTH),
			chrono.get <uint8_t> (date, chrono_t::unit_t::DATE),
			chrono.get <uint8_t> (date, chrono_t::unit_t::HOUR),
			chrono.get <uint8_t> (date, chrono_t::unit_t::MINUTES),
			chrono.get <uint8_t> (date, chrono_t::unit_t::SECONDS),
			chrono.get <uint16_t> (date, chrono_t::unit_t::MILLISECONDS)
		);
		// Записываем в лог составляющие даты, извлечённые названием
		log.print(
			"Месяц=%s день недели=%s дней с начала года=%u недель=%u",
			log_t::flag_t::INFO,
			chrono.get <string> (date, chrono_t::unit_t::MONTH).c_str(),
			chrono.get <string> (date, chrono_t::unit_t::DAY).c_str(),
			chrono.get <uint16_t> (date, chrono_t::unit_t::DAYS),
			chrono.get <uint8_t> (date, chrono_t::unit_t::WEEKS)
		);
		// Записываем в лог признаки года и половины суток
		log.print(
			"Високосный=%s половина суток=%s летнее время=%s",
			log_t::flag_t::INFO,
			(chrono.leap(date) ? "да" : "нет"),
			((chrono.h12(date) == chrono_t::h12_t::AM) ? "AM" : "PM"),
			(chrono.dst(date) ? "да" : "нет")
		);
		/**
		 * Недельный счёт по ISO 8601 разбор читает и пропускает: календарную дату год
		 * недельного счёта и номер недели задают лишь совместно с днём недели, а порознь
		 * ни одного поля не определяют. Так же поступает и strptime.
		 */
		// Записываем в лог дату записью недельного счёта ISO 8601
		log.print("Недельный счёт ISO 8601: %s", log_t::flag_t::INFO, chrono.format(date, "%G-W%V-%u").c_str());
		// Устанавливаем момент времени во внутренний объект даты
		chrono.timestamp(date, chrono_t::type_t::MILLISECONDS);
		// Заменяем год во внутреннем объекте даты
		chrono.set <uint16_t> (2030, chrono_t::unit_t::YEAR);
		// Заменяем месяц во внутреннем объекте даты его названием
		chrono.set <string> ("December", chrono_t::unit_t::MONTH);
		// Записываем в лог собранную по частям дату
		log.print("Собранная по частям дата: %s", log_t::flag_t::INFO, chrono.format("%Y-%m-%d %H:%M:%S", chrono_t::storage_t::LOCAL).c_str());
		// Выполняем очистку внутреннего объекта даты
		chrono.clear();
	}

	/**
	 * Календарные отрезки, смещения и остаток времени
	 *
	 * Отрезок задаётся полуоткрытым промежутком: begin даёт первый его миллисекунд, end -
	 * последний. Неполная единица в счёт actual не идёт.
	 */
	{
		// Записываем в лог заголовок раздела
		log.print("=== Календарные отрезки и смещения ===", log_t::flag_t::INFO);
		// Момент времени, отрезки вокруг которого считаются
		const uint64_t date = chrono.parse("2026-04-06T12:37:01.520Z", chrono_t::standard_t::RFC3339);
		// Записываем в лог начало и конец суток
		log.print("Сутки:  [%s .. %s]", log_t::flag_t::INFO, chrono.format(chrono.begin(date, chrono_t::type_t::DAY), "%Y-%m-%d %H:%M:%S.%s").c_str(), chrono.format(chrono.end(date, chrono_t::type_t::DAY), "%Y-%m-%d %H:%M:%S.%s").c_str());
		// Записываем в лог начало и конец недели
		log.print("Неделя: [%s .. %s]", log_t::flag_t::INFO, chrono.format(chrono.begin(date, chrono_t::type_t::WEEK), "%Y-%m-%d %H:%M:%S.%s").c_str(), chrono.format(chrono.end(date, chrono_t::type_t::WEEK), "%Y-%m-%d %H:%M:%S.%s").c_str());
		// Записываем в лог начало и конец месяца
		log.print("Месяц:  [%s .. %s]", log_t::flag_t::INFO, chrono.format(chrono.begin(date, chrono_t::type_t::MONTH), "%Y-%m-%d %H:%M:%S.%s").c_str(), chrono.format(chrono.end(date, chrono_t::type_t::MONTH), "%Y-%m-%d %H:%M:%S.%s").c_str());
		// Записываем в лог смещение даты на месяц вперёд
		log.print("Месяц вперёд:  %s", log_t::flag_t::INFO, chrono.format(chrono.offset(date, 1, chrono_t::type_t::MONTH, chrono_t::offset_t::INCREMENT), "%Y-%m-%d %H:%M:%S").c_str());
		// Записываем в лог смещение даты на трое суток назад
		log.print("Трое суток назад: %s", log_t::flag_t::INFO, chrono.format(chrono.offset(date, 3, chrono_t::type_t::DAY, chrono_t::offset_t::DECREMENT), "%Y-%m-%d %H:%M:%S").c_str());
		// Записываем в лог количество прошедших с начала года суток
		log.print("Прошло суток с начала года: %llu", log_t::flag_t::INFO, chrono.actual(date, chrono_t::type_t::DAY, chrono_t::type_t::YEAR, chrono_t::actual_t::PASSED));
		// Записываем в лог количество оставшихся до конца года суток
		log.print("Осталось суток до конца года: %llu", log_t::flag_t::INFO, chrono.actual(date, chrono_t::type_t::DAY, chrono_t::type_t::YEAR, chrono_t::actual_t::LEFT));
	}

	/**
	 * Размерности времени и перевод записи из одного вида в другой
	 *
	 * Обозначение состоит из одного числа и одной единицы, стоящей последним символом:
	 * составные записи вида «1h30m» не предусмотрены. Месяц и год берутся средней
	 * длительности - 30.436875 и 365 суток соответственно.
	 */
	{
		// Записываем в лог заголовок раздела
		log.print("=== Размерности времени ===", log_t::flag_t::INFO);
		// Список обозначений размерности времени, переводимых в секунды
		const char * durations[] = {"45s", "90m", "1.5h", "2d", "1w", "3M", "1y", "-2h"};
		// Выполняем перебор всех обозначений размерности времени
		for(auto & duration : durations)
			// Записываем в лог перевод обозначения размерности в секунды
			log.print("%-5s => %.0f секунд", log_t::flag_t::INFO, duration, chrono.seconds(duration));
		// Записываем в лог обратный перевод количества секунд в обозначение
		log.print("788130 секунд => %s", log_t::flag_t::INFO, chrono.seconds(788130.).c_str());
		// Записываем в лог перевод записи даты из одного вида в другой
		log.print("Перевод записи: %s", log_t::flag_t::INFO, chrono.strip("06/Apr/2026:12:37:01 +0000", "%d/%b/%Y:%H:%M:%S %z", "%Y-%m-%dT%H:%M:%SZ").c_str());
	}

	/**
	 * Временные зоны
	 *
	 * Перечисление задаёт лишь смещение зоны, но не правила перехода на летнее время:
	 * стандартное и летнее время зоны - это два разных её элемента. Сводные зоны Северной
	 * Америки разрешаются по самому моменту времени записи.
	 */
	{
		// Записываем в лог заголовок раздела
		log.print("=== Временные зоны ===", log_t::flag_t::INFO);
		// Момент времени, для которого разрешаются зоны
		const uint64_t date = chrono.parse("2026-07-06T12:37:01Z", chrono_t::standard_t::RFC3339);
		// Записываем в лог смещение зоны, заданной обозначением
		log.print("Смещение зоны \"MSK\": %s", log_t::flag_t::INFO, chrono.format(chrono.getTimeZone("MSK")).c_str());
		// Записываем в лог смещение сводной зоны, разрешённое по моменту времени
		log.print("Смещение зоны ET на июль: %s", log_t::flag_t::INFO, chrono.format(chrono.getTimeZone(chrono_t::zone_t::ET, date)).c_str());
		// Записываем в лог смещение зоны, обозначение которой переведено в элемент перечисления
		log.print("Сопоставление \"msk\" => %s", log_t::flag_t::INFO, chrono.format(chrono.getTimeZone(chrono.matchTimeZone("msk"))).c_str());
		// Записываем в лог смещение зоны, заданной названием со смещением от него
		log.print("Смещение зоны \"GMT+0530\": %s", log_t::flag_t::INFO, chrono.format(chrono.getTimeZone("GMT+0530")).c_str());
		// Записываем в лог одну и ту же дату в разных зонах
		log.print("В зоне окружения: %s", log_t::flag_t::INFO, chrono.format(date, "%Y-%m-%d %H:%M:%S %o").c_str());
		// Записываем в лог дату в зоне, заданной обозначением
		log.print("В зоне \"MSK\":     %s", log_t::flag_t::INFO, chrono.format(date, "MSK", "%Y-%m-%d %H:%M:%S %o").c_str());
		// Записываем в лог дату в зоне, заданной элементом перечисления
		log.print("В зоне PT:        %s", log_t::flag_t::INFO, chrono.format(date, chrono_t::zone_t::PT, "%Y-%m-%d %H:%M:%S %o").c_str());
		// Добавляем собственное обозначение временной зоны
		chrono.addTimeZone("ANYKS", 5 * 3600 + 1800);
		// Записываем в лог дату в собственной временной зоне
		log.print("В зоне \"ANYKS\":   %s", log_t::flag_t::INFO, chrono.format(date, "ANYKS", "%Y-%m-%d %H:%M:%S %o").c_str());
	}

	/**
	 * Текущий момент времени
	 */
	{
		// Записываем в лог заголовок раздела
		log.print("=== Текущий момент времени ===", log_t::flag_t::INFO);
		// Записываем в лог текущий момент времени в разных размерностях
		log.print(
			"Секунды=%llu миллисекунды=%llu наносекунды=%llu",
			log_t::flag_t::INFO,
			chrono.timestamp(chrono_t::type_t::SECONDS),
			chrono.timestamp(chrono_t::type_t::MILLISECONDS),
			chrono.timestamp(chrono_t::type_t::NANOSECONDS)
		);
		// Записываем в лог текущую дату записью заголовка HTTP
		log.print("Заголовок HTTP: %s", log_t::flag_t::INFO, chrono.format(chrono.timestamp(chrono_t::type_t::MILLISECONDS), chrono_t::standard_t::RFC1123).c_str());
		// Записываем в лог текущую дату в зоне окружения
		log.print("В зоне окружения: %s", log_t::flag_t::INFO, chrono.format("%A, %d %B %Y %H:%M:%S %o").c_str());
	}
	// Выводим результат
	return EXIT_SUCCESS;
}
