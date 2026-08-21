/**
 * @file reader.cpp
 * @date 2026-08-17
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
 * @brief Бенчмарки потокового чтения текста настроек YAML — пропускная способность
 *        разбора на текстах всех путей, расход выделений памяти, просадка от подачи
 *        текста кусками и задержка разбора мелкого файла настроек
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы модуля
 */
#include "yaml.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён бенчмарков контейнера YAML
 */
using namespace awh::benchmark::manifest;

/**
 * @brief Внутренние параметры и сценарии бенчмарков потокового чтения
 *
 */
namespace {
	/**
	 * @brief Количество разбираемых мелких файлов настроек
	 *
	 */
	static constexpr size_t SMALL_ROUNDS = 20000;
	/**
	 * @brief Количество разбираемых крупных файлов настроек
	 *
	 */
	static constexpr size_t LARGE_ROUNDS = 8;
	/**
	 * @brief Количество разбираемых файлов с преобладанием одного вида записи
	 *
	 */
	static constexpr size_t FOCUSED_ROUNDS = 20;
	/**
	 * @brief Размер куска подачи текста настроек в октетах
	 *
	 * @note Взят равным полезной части кадра сети: текст настроек приходит такими
	 *       кусками и с гнезда, и с файловой системы, а подача его целиком - случай
	 *       не самый частый
	 *
	 */
	static constexpr size_t CHUNK_SIZE = 1460;

	/**
	 * @brief Пороги пропускной способности потокового чтения в мегабайтах в секунду
	 *
	 * @details Пороги назначены по замеру на рабочей машине 17.08.2026 с запасом
	 *          вчетверо: отладочные стенды отстают от неё вчетверо-впятеро, и порог,
	 *          назначенный по рабочей машине впритык, валил бы прогон на них
	 *
	 * @note Запаса этого довольно, чтобы заметить возвращение уже случавшихся ошибок
	 *       разбора: квадратичное изъятие разобранного из хранилища и обратный проход
	 *       по накопленному тексту при каждом поданном куске роняли показатель у
	 *       кодека настроек INI втрое и более
	 *
	 */
	static constexpr double READ_SERVICE_THRESHOLD = 6.61;
	/**
	 * @brief Порог пропускной способности чтения крупного файла настроек
	 *
	 * @note Запас взят двукратным к самому медленному из известных стендов: у соседних
	 *       кодеков OpenBSD 7.9/amd64 отстаёт от рабочей машины в пять-шесть раз, и
	 *       просадка эта принадлежит машине, а не модулю
	 *
	 */
	static constexpr double READ_LARGE_THRESHOLD = 4.57;
	/**
	 * @brief Порог пропускной способности чтения текста с преобладанием строк
	 *
	 */
	static constexpr double READ_STRINGS_THRESHOLD = 8.71;
	/**
	 * @brief Порог пропускной способности чтения текста с преобладанием чисел
	 *
	 */
	static constexpr double READ_NUMBERS_THRESHOLD = 5.12;
	/**
	 * @brief Порог пропускной способности чтения текста с преобладанием построений
	 *
	 */
	static constexpr double READ_ARRAYS_THRESHOLD = 3.46;
	/**
	 * @brief Порог пропускной способности чтения текста с блочными значениями
	 *
	 * @details Блочное значение собирается построчно: содержимое переносится в
	 *          накопитель, отступ усекается, а свёртка обращает переводы строк в
	 *          пробелы. Путь этот у соседних кодеков подобия не имеет вовсе
	 *
	 */
	static constexpr double READ_BLOCKS_THRESHOLD = 12.21;
	/**
	 * @brief Порог пропускной способности чтения текста с метками и ссылками
	 *
	 * @note Потоковое чтение ссылок не раскрывает: мерится здесь объявление меток и
	 *       выдача события ссылки, а раскрытие их мерит сценарий дерева документа
	 *
	 */
	static constexpr double READ_ANCHORS_THRESHOLD = 5.61;
	/**
	 * @brief Порог количества выделений памяти на чтение крупного файла настроек
	 *
	 * @details Показатель воспроизводим до единиц и потому годится в порог куда
	 *          больше времени: чтение ведётся на переиспользуемых хранилищах, и
	 *          количество выделений на файл растёт логарифмом размера его, а не
	 *          линейно. Рост показателя означает, что какое-то хранилище перестало
	 *          переиспользоваться и заводится заново на каждую строку
	 *
	 * @note Порог этот уже ловил дефект: очередь событий держалась двусторонней
	 *       очередью, а та освобождает опустевший кусок памяти и заводит его заново
	 *       приходом следующего события. Разбор файла в шестнадцать мегабайт обходился
	 *       в 46 001 выделение вместо 66 - по одному на всякие четыре десятка событий
	 *
	 */
	static constexpr double READ_ALLOCATIONS_THRESHOLD = 128.0;
	/**
	 * @brief Порог просадки чтения от подачи текста кусками
	 *
	 * @details Стоимость подачи кусками стережётся здесь, а не в наборе проверок:
	 *          надбавка эта постоянного размера, и счётчики покрытия отладочной
	 *          сборки затушёвывают её напрочь
	 *
	 * @note Измеряется отношение времени подачи кусками ко времени подачи того же
	 *       текста целиком, а не пропускная способность сама по себе. Отношение
	 *       двух прогонов на одной машине от её быстродействия не зависит, и порог
	 *       ему можно назначить впритык
	 *
	 */
	static constexpr double READ_CHUNKED_THRESHOLD = 1.9;
	/**
	 * @brief Порог задержки чтения файла настроек приложения в микросекундах
	 *
	 * @details Файл настроек приложения - это единицы килобайт, и на нём решает не
	 *          пропускная способность, а постоянные издержки на запуск разбора
	 *
	 * @note Порог держится по самому медленному стенду, а не по рабочей машине: стенд
	 *       FreeBSD равномерно впятеро медленнее её, и запас взят четырнадцатикратным
	 *       к рабочей машине, наравне с соседними кодеками
	 *
	 */
	static constexpr double READ_SERVICE_LATENCY_THRESHOLD = 121.24;

	/**
	 * @brief Функция потокового чтения текста настроек
	 *
	 * @param text разбираемый текст настроек
	 * @return     количество полученных событий разбора
	 *
	 */
	static uint64_t read(const string & text) noexcept {
		// Объект потокового чтения текста настроек
		awh::codec::yaml::reader_t reader;
		/**
		 * Если передать текст настроек не удалось
		 */
		if(!reader.feed(text.data(), text.size(), true))
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
	 * @brief Функция потокового чтения текста настроек, поданного кусками
	 *
	 * @param text разбираемый текст настроек
	 * @return     количество полученных событий разбора
	 *
	 */
	static uint64_t feed(const string & text) noexcept {
		// Объект потокового чтения текста настроек
		awh::codec::yaml::reader_t reader;
		// Количество полученных событий разбора
		uint64_t result = 0;
		// Смещение очередного подаваемого куска текста настроек
		size_t offset = 0;
		/**
		 * Выполняем подачу текста настроек до его окончания
		 */
		do {
			// Размер очередного подаваемого куска текста настроек
			const size_t size = (((offset + CHUNK_SIZE) > text.size()) ? (text.size() - offset) : CHUNK_SIZE);
			/**
			 * Если передать очередной кусок текста настроек не удалось
			 */
			if(!reader.feed(text.data() + offset, size, ((offset + size) >= text.size())))
				// Выводим нулевое количество событий разбора
				return 0;
			/**
			 * Выполняем перебор всех событий, полученных из очередного куска
			 */
			while(reader.next())
				// Выполняем подсчёт полученных событий разбора
				result++;
			// Выполняем смещение на размер поданного куска текста настроек
			offset += size;
		// Выполняем подачу до исчерпания текста настроек
		} while(offset < text.size());
		// Выводим количество полученных событий разбора
		return result;
	}
	/**
	 * @brief Функция прогона сценария чтения заданного текста настроек
	 *
	 * @param text   разбираемый текст настроек
	 * @param rounds количество разбираемых файлов настроек
	 * @return       результат измерения
	 *
	 */
	static awh::benchmark::result_t reading(const string & text, const size_t rounds) noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), rounds, [&text]() noexcept {
			// Выполняем чтение текста настроек
			return ::read(text);
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария чтения файла настроек приложения
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readService() noexcept {
		// Выполняем прогон сценария чтения файла настроек приложения
		return ::reading(service(), SMALL_ROUNDS);
	}
	/**
	 * @brief Функция прогона сценария чтения крупного файла настроек
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readLarge() noexcept {
		// Выполняем прогон сценария чтения крупного файла настроек
		return ::reading(large(), LARGE_ROUNDS);
	}
	/**
	 * @brief Функция прогона сценария чтения текста с преобладанием строк
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readStrings() noexcept {
		// Выполняем прогон сценария чтения текста с преобладанием строк
		return ::reading(strings(), FOCUSED_ROUNDS);
	}
	/**
	 * @brief Функция прогона сценария чтения текста с преобладанием чисел
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readNumbers() noexcept {
		// Выполняем прогон сценария чтения текста с преобладанием чисел
		return ::reading(numbers(), FOCUSED_ROUNDS);
	}
	/**
	 * @brief Функция прогона сценария чтения текста с преобладанием построений
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readArrays() noexcept {
		// Выполняем прогон сценария чтения текста с преобладанием построений
		return ::reading(arrays(), FOCUSED_ROUNDS);
	}
	/**
	 * @brief Функция прогона сценария чтения текста с блочными значениями
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readBlocks() noexcept {
		// Выполняем прогон сценария чтения текста с блочными значениями
		return ::reading(blocks(), FOCUSED_ROUNDS);
	}
	/**
	 * @brief Функция прогона сценария чтения текста с метками и ссылками
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readAnchors() noexcept {
		// Выполняем прогон сценария чтения текста с метками и ссылками
		return ::reading(anchors(), FOCUSED_ROUNDS);
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
		// Разбираемый текст настроек
		const string & text = large();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), LARGE_ROUNDS, [&text]() noexcept {
			// Выполняем чтение текста настроек
			return ::read(text);
		});
		/**
		 * Если ни одной операции не выполнено
		 *
		 * @note Показатель «на одну операцию» при нуле операций выдал бы ноль, а ноль
		 *       укладывается в любой порог с верхней границей: молчание сценария
		 *       отчиталось бы успехом
		 */
		if(outcome.operations == 0){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "разбор не выполнил ни одной операции";
			// Выводим результат измерения
			return result;
		}
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		/**
		 * Если учёт выделений памяти не работает
		 */
		if(!counted(outcome, result))
			// Выводим результат измерения
			return result;
		// Устанавливаем измеренное значение
		result.value = perDocument(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария просадки чтения от подачи кусками
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readChunked() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст настроек
		const string & text = large();
		// Выполняем прогон подачи текста настроек целиком
		const outcome_t whole = measure(text.size(), LARGE_ROUNDS, [&text]() noexcept {
			// Выполняем чтение текста настроек, поданного целиком
			return ::read(text);
		});
		// Выполняем прогон подачи текста настроек кусками
		const outcome_t chunked = measure(text.size(), LARGE_ROUNDS, [&text]() noexcept {
			// Выполняем чтение текста настроек, поданного кусками
			return ::feed(text);
		});
		/**
		 * Если замер не состоялся
		 */
		if((whole.seconds <= 0.0) || (chunked.seconds <= 0.0)){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "замер времени не состоялся";
			// Выводим результат измерения
			return result;
		}
		// Устанавливаем измеренное отношение времени подачи кусками ко времени подачи целиком
		result.value = (chunked.seconds / whole.seconds);
		// Устанавливаем сведения о прогоне
		result.details = details(chunked);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария задержки чтения файла настроек приложения
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t latencyService() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст настроек
		const string & text = service();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), SMALL_ROUNDS, [&text]() noexcept {
			// Выполняем чтение текста настроек
			return ::read(text);
		});
		/**
		 * Если ни одной операции не выполнено
		 */
		if(outcome.operations == 0){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "разбор не выполнил ни одной операции";
			// Выводим результат измерения
			return result;
		}
		// Устанавливаем измеренное значение
		result.value = perLatency(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	/**
	 * Выполняем регистрацию сценария чтения файла настроек приложения
	 */
	static const bool SERVICE_REGISTERED = awh::benchmark::add(
		"codec/yaml: чтение настроек службы", "МБ/с", READ_SERVICE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readService
	);
	/**
	 * Выполняем регистрацию сценария чтения крупного файла настроек
	 */
	static const bool LARGE_REGISTERED = awh::benchmark::add(
		"codec/yaml: чтение крупного файла", "МБ/с", READ_LARGE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readLarge
	);
	/**
	 * Выполняем регистрацию сценария чтения текста с преобладанием строк
	 */
	static const bool STRINGS_REGISTERED = awh::benchmark::add(
		"codec/yaml: чтение строковых значений", "МБ/с", READ_STRINGS_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readStrings
	);
	/**
	 * Выполняем регистрацию сценария чтения текста с преобладанием чисел
	 */
	static const bool NUMBERS_REGISTERED = awh::benchmark::add(
		"codec/yaml: чтение чисел и отметок времени", "МБ/с", READ_NUMBERS_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readNumbers
	);
	/**
	 * Выполняем регистрацию сценария чтения текста с преобладанием построений
	 */
	static const bool ARRAYS_REGISTERED = awh::benchmark::add(
		"codec/yaml: чтение перечней и отображений", "МБ/с", READ_ARRAYS_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readArrays
	);
	/**
	 * Выполняем регистрацию сценария чтения текста с блочными значениями
	 */
	static const bool BLOCKS_REGISTERED = awh::benchmark::add(
		"codec/yaml: чтение блочных значений", "МБ/с", READ_BLOCKS_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readBlocks
	);
	/**
	 * Выполняем регистрацию сценария чтения текста с метками и ссылками
	 */
	static const bool ANCHORS_REGISTERED = awh::benchmark::add(
		"codec/yaml: чтение меток и ссылок", "МБ/с", READ_ANCHORS_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readAnchors
	);
	/**
	 * Выполняем регистрацию сценария расхода выделений памяти на чтение
	 */
	static const bool ALLOCATIONS_REGISTERED = awh::benchmark::add(
		"codec/yaml: выделения на чтение", "выд./файл", READ_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, readAllocations
	);
	/**
	 * Выполняем регистрацию сценария просадки чтения от подачи кусками
	 */
	static const bool CHUNKED_REGISTERED = awh::benchmark::add(
		"codec/yaml: просадка от подачи кусками", "раз", READ_CHUNKED_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, readChunked
	);
	/**
	 * Выполняем регистрацию сценария задержки чтения файла настроек приложения
	 */
	static const bool SERVICE_LATENCY_REGISTERED = awh::benchmark::add(
		"codec/yaml: задержка чтения настроек службы", "мкс/файл", READ_SERVICE_LATENCY_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, latencyService
	);
};
