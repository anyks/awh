/**
 * @file reader.cpp
 * @date 2026-08-19
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
 * @brief Замеры потокового чтения бинарного контейнера ABC
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл
 */
#include "abc.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh::benchmark::binary;

/**
 * @brief Внутренние параметры сценариев потокового чтения
 *
 */
namespace {
	/**
	 * @brief Количество разбираемых записей ответа службы
	 *
	 */
	static constexpr size_t SMALL_ROUNDS = 20000;
	/**
	 * @brief Количество разбираемых крупных записей
	 *
	 */
	static constexpr size_t LARGE_ROUNDS = 8;
	/**
	 * @brief Количество разбираемых записей с преобладанием одного вида значений
	 *
	 */
	static constexpr size_t FOCUSED_ROUNDS = 20;
	/**
	 * @brief Размер куска подачи записи в октетах
	 *
	 * @note Взят равным полезной части кадра сети: запись приходит такими кусками и с
	 *       гнезда, и с файловой системы, а подача её целиком - случай не самый частый
	 *
	 */
	static constexpr size_t CHUNK_SIZE = 1460;

	/**
	 * @brief Пороги пропускной способности потокового чтения в мегабайтах в секунду
	 *
	 * @details Пороги назначены по замеру на рабочей машине 19.08.2026 с запасом
	 *          вчетверо-впятеро: отладочные стенды отстают от неё вчетверо-впятеро, и
	 *          порог, назначенный по рабочей машине впритык, валил бы прогон на них
	 *
	 */
	static constexpr double READ_SERVICE_THRESHOLD = 8.0;
	/**
	 * @brief Порог пропускной способности чтения крупной записи
	 *
	 */
	static constexpr double READ_LARGE_THRESHOLD = 10.0;
	/**
	 * @brief Порог пропускной способности чтения записи с преобладанием чисел
	 *
	 */
	static constexpr double READ_NUMBERS_THRESHOLD = 12.0;
	/**
	 * @brief Порог пропускной способности чтения записи с преобладанием строк
	 *
	 */
	static constexpr double READ_STRINGS_THRESHOLD = 53.0;
	/**
	 * @brief Порог пропускной способности чтения записи с двоичными значениями
	 *
	 */
	static constexpr double READ_BLOBS_THRESHOLD = 93.0;
	/**
	 * @brief Порог пропускной способности чтения записи с глубокой вложенностью
	 *
	 */
	static constexpr double READ_NESTED_THRESHOLD = 2.5;
	/**
	 * @brief Порог количества выделений памяти на чтение крупной записи
	 *
	 * @details Показатель воспроизводим до единиц и потому годится в порог куда больше
	 *          времени: чтение ведётся на переиспользуемых хранилищах, и количество
	 *          выделений на запись от её размера не зависит. Рост показателя означает,
	 *          что какое-то хранилище перестало переиспользоваться
	 *
	 */
	static constexpr double READ_ALLOCATIONS_THRESHOLD = 48.0;
	/**
	 * @brief Порог просадки чтения от подачи записи кусками
	 *
	 * @note Измеряется отношение времени подачи кусками ко времени подачи той же записи
	 *       целиком, а не пропускная способность сама по себе. Отношение двух прогонов
	 *       на одной машине от её быстродействия не зависит, и порог ему можно назначить
	 *       впритык
	 *
	 */
	static constexpr double READ_CHUNKED_THRESHOLD = 1.26;
	/**
	 * @brief Порог задержки чтения записи ответа службы в микросекундах
	 *
	 * @note Порог держится по самому медленному стенду, а не по рабочей машине: стенд
	 *       FreeBSD равномерно впятеро медленнее её
	 *
	 */
	static constexpr double READ_SERVICE_LATENCY_THRESHOLD = 8.0;

	/**
	 * @brief Функция потокового чтения записи
	 *
	 * @note Название `read` здесь занято работой POSIX: свободная работа с таким именем
	 *       видна из заголовочных файлов системы, и вызов ушёл бы не туда
	 *
	 * @param record разбираемая запись
	 * @return       количество полученных событий разбора
	 *
	 */
	static uint64_t scan(const vector <uint8_t> & record) noexcept {
		// Разбиратель бинарной записи
		awh::codec::abc::reader_t reader;
		// Если подать запись разбирателю не удалось
		if(!reader.feed(record.data(), record.size(), true))
			// Выводим нулевое количество событий разбора
			return 0;
		// Количество полученных событий разбора
		uint64_t result = 0;
		/**
		 * Выполняем перебор всех событий разбора
		 */
		while(reader.next())
			// Выполняем подсчёт полученных событий разбора
			result++;
		// Выводим количество полученных событий разбора
		return result;
	}
	/**
	 * @brief Функция потокового чтения записи, поданной кусками
	 *
	 * @param record разбираемая запись
	 * @return       количество полученных событий разбора
	 *
	 */
	static uint64_t feed(const vector <uint8_t> & record) noexcept {
		// Разбиратель бинарной записи
		awh::codec::abc::reader_t reader;
		// Количество полученных событий разбора
		uint64_t result = 0;
		// Смещение очередного подаваемого куска записи
		size_t offset = 0;
		/**
		 * Выполняем подачу записи до её окончания
		 */
		do {
			// Размер очередного подаваемого куска записи
			const size_t size = (((offset + CHUNK_SIZE) > record.size()) ? (record.size() - offset) : CHUNK_SIZE);
			// Если передать очередной кусок записи не удалось
			if(!reader.feed(record.data() + offset, size, ((offset + size) >= record.size())))
				// Выводим нулевое количество событий разбора
				return 0;
			/**
			 * Выполняем перебор всех событий, полученных из очередного куска
			 */
			while(reader.next())
				// Выполняем подсчёт полученных событий разбора
				result++;
			// Выполняем смещение на размер поданного куска записи
			offset += size;
		// Выполняем подачу до исчерпания записи
		} while(offset < record.size());
		// Выводим количество полученных событий разбора
		return result;
	}
	/**
	 * @brief Функция прогона сценария чтения заданной записи
	 *
	 * @param record разбираемая запись
	 * @param rounds количество разбираемых записей
	 * @return       результат измерения
	 *
	 */
	static awh::benchmark::result_t reading(const vector <uint8_t> & record, const size_t rounds) noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(record.size(), rounds, [&record]() noexcept {
			// Выполняем чтение записи
			return ::scan(record);
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария чтения записи ответа службы
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readService() noexcept {
		// Выполняем прогон сценария чтения записи ответа службы
		return ::reading(service(), SMALL_ROUNDS);
	}
	/**
	 * @brief Функция прогона сценария чтения крупной записи
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readLarge() noexcept {
		// Выполняем прогон сценария чтения крупной записи
		return ::reading(large(), LARGE_ROUNDS);
	}
	/**
	 * @brief Функция прогона сценария чтения записи с преобладанием чисел
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readNumbers() noexcept {
		// Выполняем прогон сценария чтения записи с преобладанием чисел
		return ::reading(numbers(), FOCUSED_ROUNDS);
	}
	/**
	 * @brief Функция прогона сценария чтения записи с преобладанием строк
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readStrings() noexcept {
		// Выполняем прогон сценария чтения записи с преобладанием строк
		return ::reading(strings(), FOCUSED_ROUNDS);
	}
	/**
	 * @brief Функция прогона сценария чтения записи с двоичными значениями
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readBlobs() noexcept {
		// Выполняем прогон сценария чтения записи с двоичными значениями
		return ::reading(blobs(), FOCUSED_ROUNDS);
	}
	/**
	 * @brief Функция прогона сценария чтения записи с глубокой вложенностью
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readNested() noexcept {
		// Выполняем прогон сценария чтения записи с глубокой вложенностью
		return ::reading(nested(), FOCUSED_ROUNDS);
	}
	/**
	 * @brief Функция прогона сценария расхода выделений памяти на чтение
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readAllocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемая запись
		const vector <uint8_t> & record = large();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(record.size(), LARGE_ROUNDS, [&record]() noexcept {
			// Выполняем чтение записи
			return ::scan(record);
		});
		// Если сценарий работы не выполнил
		if(!worked(outcome, result))
			// Выводим результат измерения
			return result;
		// Если учёт выделений памяти не работает
		if(!counted(outcome, result))
			// Выводим результат измерения
			return result;
		// Устанавливаем измеренное количество выделений памяти на одну запись
		result.value = perDocument(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария просадки чтения от подачи записи кусками
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readChunked() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемая запись
		const vector <uint8_t> & record = large();
		// Выполняем прогон чтения записи, поданной целиком
		const outcome_t whole = measure(record.size(), LARGE_ROUNDS, [&record]() noexcept {
			// Выполняем чтение записи, поданной целиком
			return ::scan(record);
		});
		// Выполняем прогон чтения записи, поданной кусками
		const outcome_t sliced = measure(record.size(), LARGE_ROUNDS, [&record]() noexcept {
			// Выполняем чтение записи, поданной кусками
			return ::feed(record);
		});
		/**
		 * Если время подачи записи целиком не измерено
		 */
		if(whole.seconds <= 0.0){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "время подачи записи целиком не измерено";
			// Выводим результат измерения
			return result;
		}
		// Устанавливаем измеренную просадку чтения
		result.value = (sliced.seconds / whole.seconds);
		// Устанавливаем сведения о прогоне
		result.details = details(sliced);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария задержки чтения записи ответа службы
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t latencyService() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемая запись
		const vector <uint8_t> & record = service();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(record.size(), SMALL_ROUNDS, [&record]() noexcept {
			// Выполняем чтение записи
			return ::scan(record);
		});
		// Если сценарий работы не выполнил
		if(!worked(outcome, result))
			// Выводим результат измерения
			return result;
		// Устанавливаем измеренную задержку чтения записи
		result.value = perLatency(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	/**
	 * Выполняем регистрацию сценария чтения записи ответа службы
	 */
	static const bool SERVICE_REGISTERED = awh::benchmark::add(
		"codec/abc: чтение ответа службы", "МБ/с", READ_SERVICE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readService
	);
	/**
	 * Выполняем регистрацию сценария чтения крупной записи
	 */
	static const bool LARGE_REGISTERED = awh::benchmark::add(
		"codec/abc: чтение крупной записи", "МБ/с", READ_LARGE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readLarge
	);
	/**
	 * Выполняем регистрацию сценария чтения записи с преобладанием чисел
	 */
	static const bool NUMBERS_REGISTERED = awh::benchmark::add(
		"codec/abc: чтение чисел", "МБ/с", READ_NUMBERS_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readNumbers
	);
	/**
	 * Выполняем регистрацию сценария чтения записи с преобладанием строк
	 */
	static const bool STRINGS_REGISTERED = awh::benchmark::add(
		"codec/abc: чтение строк", "МБ/с", READ_STRINGS_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readStrings
	);
	/**
	 * Выполняем регистрацию сценария чтения записи с двоичными значениями
	 */
	static const bool BLOBS_REGISTERED = awh::benchmark::add(
		"codec/abc: чтение двоичных значений", "МБ/с", READ_BLOBS_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readBlobs
	);
	/**
	 * Выполняем регистрацию сценария чтения записи с глубокой вложенностью
	 */
	static const bool NESTED_REGISTERED = awh::benchmark::add(
		"codec/abc: чтение вложенности", "МБ/с", READ_NESTED_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readNested
	);
	/**
	 * Выполняем регистрацию сценария расхода выделений памяти на чтение
	 */
	static const bool ALLOCATIONS_REGISTERED = awh::benchmark::add(
		"codec/abc: выделения на чтение", "выд./зап.", READ_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, readAllocations
	);
	/**
	 * Выполняем регистрацию сценария просадки чтения от подачи кусками
	 */
	static const bool CHUNKED_REGISTERED = awh::benchmark::add(
		"codec/abc: просадка от подачи кусками", "раз", READ_CHUNKED_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, readChunked
	);
	/**
	 * Выполняем регистрацию сценария задержки чтения записи ответа службы
	 */
	static const bool SERVICE_LATENCY_REGISTERED = awh::benchmark::add(
		"codec/abc: задержка чтения ответа службы", "мкс/зап.", READ_SERVICE_LATENCY_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, latencyService
	);
};
