/**
 * @file writer.cpp
 * @date 2026-08-13
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
 * @brief Бенчмарки записи текста CSV — пропускная способность записи полей разного
 *        склада, стоимость обрамления кавычками, расход выделений памяти и стоимость
 *        потокового изъятия собранного
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл бенчмарков контейнера CSV
 */
#include "csv.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён бенчмарков контейнера CSV
 */
using namespace awh::benchmark::table;

/**
 * @brief Внутренние параметры и сценарии бенчмарков записи
 *
 */
namespace {
	/**
	 * @brief Количество записываемых таблиц
	 *
	 */
	static constexpr size_t WRITE_ROUNDS = 200;
	/**
	 * @brief Количество записей в записываемой таблице
	 *
	 */
	static constexpr size_t WRITE_RECORDS = 20000;
	/**
	 * @brief Порог накопления, по достижении которого собранное изымается
	 *
	 * @note Взят таким же, каким его берёт потребитель потоковой записи: собранное
	 *       изымается кусками и отправляется дальше, а в памяти не оседает
	 *
	 */
	static constexpr size_t TAKE_WATERMARK = 0x10000;

	/**
	 * @brief Пороги пропускной способности записи в мегабайтах в секунду
	 *
	 * @details Пороги сняты прогоном по двенадцати отладочным стендам x86_64
	 *          (14.08.2026) и назначены по самому медленному, делённому надвое - тем же
	 *          правилом, что и пороги чтения
	 *
	 */
	static constexpr double WRITE_PLAIN_THRESHOLD = 100.0;
	/**
	 * @brief Порог пропускной способности записи полей, требующих кавычек
	 *
	 */
	static constexpr double WRITE_QUOTED_THRESHOLD = 50.0;
	/**
	 * @brief Порог пропускной способности записи числовых полей
	 *
	 * @details Числовое поле записывается подбором кратчайшей записи, читающейся
	 *          обратно тем же числом: наращивание точности от единицы до наибольшей
	 *          стоит дороже записи последовательности знаков, и стережётся оно отдельно
	 *
	 * @note Показатель этот **зависит от библиотеки языка Си**, а не от быстродействия
	 *       машины: подбор упирается в обратное чтение числа средствами библиотеки, и
	 *       стенды glibc дают 3.4-3.7 против 4.8-7.2 у musl и систем BSD - вдвое
	 *       меньше при вдвое более быстрой машине. Порог оттого держится по glibc, и
	 *       порог, снятый на рабочей машине, здесь оказывался ЗАВЫШЕН: Debian,
	 *       Ubuntu и Fedora давали 3.42-3.64 при пороге 4.0
	 *
	 */
	static constexpr double WRITE_NUMBER_THRESHOLD = 1.70;
	/**
	 * @brief Порог количества выделений памяти на запись одной таблицы
	 *
	 * @details Запись ведётся дописыванием в одно хранилище, и количество выделений на
	 *          таблицу определяется его наращиванием - то есть растёт логарифмом от
	 *          размера, а не числом записей. Рост показателя означает, что хранилище
	 *          перестало переиспользоваться
	 *
	 * @note По стендам показатель равен 16 у libc++ и 17 у libstdc++ - порог держится
	 *       туго намеренно
	 *
	 */
	static constexpr double WRITE_ALLOCATIONS_THRESHOLD = 32.0;
	/**
	 * @brief Порог надбавки потокового изъятия собранного
	 *
	 * @details Изъятие отдаёт собранное целиком и оставляет сборщик готовым продолжать:
	 *          надбавка эта - стоимость переноса собранного и заведения хранилища
	 *          заново. Измеряется отношением ко времени записи той же таблицы без
	 *          изъятия, и от быстродействия машины отношение не зависит
	 *
	 */
	static constexpr double WRITE_TAKE_THRESHOLD = 1.5;

	/**
	 * @brief Функция получения полей записываемой записи без кавычек
	 *
	 * @return поля записываемой записи
	 *
	 */
	static const vector <string> & plain() noexcept {
		// Поля записываемой записи
		static const vector <string> result = {
			"1024", "Товар обыкновенный", "Москва", "100.50", "active"
		};
		// Выводим поля записываемой записи
		return result;
	}
	/**
	 * @brief Функция получения полей записываемой записи, требующих кавычек
	 *
	 * @return поля записываемой записи
	 *
	 */
	static const vector <string> & special() noexcept {
		// Поля записываемой записи
		static const vector <string> result = {
			"1024", "Товар, особый", "Примечание с \"кавычками\" и запятой",
			"Описание\r\nв две строки", " обвязка "
		};
		// Выводим поля записываемой записи
		return result;
	}
	/**
	 * @brief Функция записи таблицы полем за полем
	 *
	 * @param fields  поля записываемой записи
	 * @param records количество записываемых записей
	 * @return        размер собранного текста таблицы
	 *
	 */
	static uint64_t write(const vector <string> & fields, const size_t records) noexcept {
		// Объект записи текста таблицы
		awh::codec::csv::writer_t writer;
		/**
		 * Выполняем запись всех записей таблицы
		 */
		for(size_t i = 0; i < records; i++)
			// Выполняем запись очередной записи таблицы
			writer.record(fields);
		// Выводим размер собранного текста таблицы
		return static_cast <uint64_t> (writer.size());
	}
	/**
	 * @brief Функция записи таблицы с потоковым изъятием собранного
	 *
	 * @param fields  поля записываемой записи
	 * @param records количество записываемых записей
	 * @return        размер изъятого текста таблицы
	 *
	 */
	static uint64_t stream(const vector <string> & fields, const size_t records) noexcept {
		// Объект записи текста таблицы
		awh::codec::csv::writer_t writer;
		// Размер изъятого текста таблицы
		uint64_t result = 0;
		/**
		 * Выполняем запись всех записей таблицы
		 */
		for(size_t i = 0; i < records; i++){
			// Выполняем запись очередной записи таблицы
			writer.record(fields);
			/**
			 * Если собранного накопилось довольно
			 */
			if(writer.size() >= ::TAKE_WATERMARK)
				// Выполняем учёт размера изъятого текста таблицы
				result += static_cast <uint64_t> (writer.take().size());
		}
		// Выполняем учёт размера изъятого остатка текста таблицы
		result += static_cast <uint64_t> (writer.take().size());
		// Выводим размер изъятого текста таблицы
		return result;
	}
	/**
	 * @brief Функция записи таблицы числовых полей
	 *
	 * @param records количество записываемых записей
	 * @return        размер собранного текста таблицы
	 *
	 */
	static uint64_t numbers(const size_t records) noexcept {
		// Объект записи текста таблицы
		awh::codec::csv::writer_t writer;
		/**
		 * Выполняем запись всех записей таблицы
		 */
		for(size_t i = 0; i < records; i++){
			// Выполняем запись целого поля записи
			writer.number <int64_t> (static_cast <int64_t> (i));
			// Выполняем запись беззнакового целого поля записи
			writer.number <uint32_t> (static_cast <uint32_t> (i % 100000));
			// Выполняем запись поля записи с плавающей запятой
			writer.number <double> (static_cast <double> (i) / 7.0);
			// Выполняем запись логического поля записи
			writer.number <bool> ((i % 2) == 0);
			// Выполняем завершение очередной записи таблицы
			writer.record();
		}
		// Выводим размер собранного текста таблицы
		return static_cast <uint64_t> (writer.size());
	}
	/**
	 * @brief Функция получения размера таблицы, собираемой сценарием
	 *
	 * @param fields  поля записываемой записи
	 * @param records количество записываемых записей
	 * @return        размер собираемого текста таблицы в октетах
	 *
	 */
	static size_t volume(const vector <string> & fields, const size_t records) noexcept {
		// Объект записи текста таблицы
		awh::codec::csv::writer_t writer;
		// Выполняем запись одной записи таблицы
		writer.record(fields);
		// Выводим размер собираемого текста таблицы
		return (writer.size() * records);
	}
	/**
	 * @brief Функция прогона сценария записи полей без кавычек
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t writePlain() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Поля записываемой записи
		const vector <string> & fields = ::plain();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(::volume(fields, ::WRITE_RECORDS), ::WRITE_ROUNDS, [&fields]() noexcept {
			// Выполняем запись текста таблицы
			return ::write(fields, ::WRITE_RECORDS);
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария записи полей, требующих кавычек
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t writeQuoted() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Поля записываемой записи
		const vector <string> & fields = ::special();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(::volume(fields, ::WRITE_RECORDS), ::WRITE_ROUNDS, [&fields]() noexcept {
			// Выполняем запись текста таблицы
			return ::write(fields, ::WRITE_RECORDS);
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария записи числовых полей
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t writeNumber() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем размер собираемого текста таблицы
		const size_t bytes = static_cast <size_t> (::numbers(::WRITE_RECORDS));
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(bytes, ::WRITE_ROUNDS, []() noexcept {
			// Выполняем запись текста таблицы
			return ::numbers(::WRITE_RECORDS);
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария расхода выделений памяти на запись
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t writeAllocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Поля записываемой записи
		const vector <string> & fields = ::plain();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(::volume(fields, ::WRITE_RECORDS), ::WRITE_ROUNDS, [&fields]() noexcept {
			// Выполняем запись текста таблицы
			return ::write(fields, ::WRITE_RECORDS);
		});
		// Устанавливаем измеренное значение
		result.value = perDocument(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария надбавки потокового изъятия собранного
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t writeTake() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Поля записываемой записи
		const vector <string> & fields = ::plain();
		// Получаем размер собираемого текста таблицы
		const size_t bytes = ::volume(fields, ::WRITE_RECORDS);
		// Выполняем прогон записи таблицы целиком
		const outcome_t whole = measure(bytes, ::WRITE_ROUNDS, [&fields]() noexcept {
			// Выполняем запись текста таблицы
			return ::write(fields, ::WRITE_RECORDS);
		});
		// Выполняем прогон записи таблицы с потоковым изъятием собранного
		const outcome_t taken = measure(bytes, ::WRITE_ROUNDS, [&fields]() noexcept {
			// Выполняем запись текста таблицы с изъятием собранного
			return ::stream(fields, ::WRITE_RECORDS);
		});
		/**
		 * Если хотя бы один из прогонов не состоялся
		 */
		if((whole.seconds <= 0.0) || (taken.seconds <= 0.0)){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Выводим результат измерения
			return result;
		}
		// Устанавливаем измеренное значение
		result.value = (taken.seconds / whole.seconds);
		// Устанавливаем сведения о прогоне
		result.details = details(taken);
		// Выводим результат измерения
		return result;
	}

	/**
	 * Выполняем регистрацию сценария записи полей без кавычек
	 */
	static const bool PLAIN_REGISTERED = awh::benchmark::add(
		"codec/csv: запись полей без кавычек", "МБ/с", ::WRITE_PLAIN_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, ::writePlain
	);
	/**
	 * Выполняем регистрацию сценария записи полей, требующих кавычек
	 */
	static const bool QUOTED_REGISTERED = awh::benchmark::add(
		"codec/csv: запись полей в кавычках", "МБ/с", ::WRITE_QUOTED_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, ::writeQuoted
	);
	/**
	 * Выполняем регистрацию сценария записи числовых полей
	 */
	static const bool NUMBER_REGISTERED = awh::benchmark::add(
		"codec/csv: запись числовых полей", "МБ/с", ::WRITE_NUMBER_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, ::writeNumber
	);
	/**
	 * Выполняем регистрацию сценария расхода выделений памяти на запись
	 */
	static const bool ALLOCATIONS_REGISTERED = awh::benchmark::add(
		"codec/csv: выделения на запись", "выд./табл.", ::WRITE_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, ::writeAllocations
	);
	/**
	 * Выполняем регистрацию сценария надбавки потокового изъятия собранного
	 */
	static const bool TAKE_REGISTERED = awh::benchmark::add(
		"codec/csv: надбавка потокового изъятия", "раз", ::WRITE_TAKE_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, ::writeTake
	);
};
