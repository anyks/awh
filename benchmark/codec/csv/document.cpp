/**
 * @file document.cpp
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
 * @brief Бенчмарки контейнера таблицы CSV — сборка таблицы целиком, обход собранного,
 *        доступ по имени столбца, перезапись таблицы и потоковая выдача записей
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
 * @brief Внутренние параметры и сценарии бенчмарков контейнера таблицы
 *
 */
namespace {
	/**
	 * @brief Количество собираемых крупных таблиц
	 *
	 */
	static constexpr size_t LARGE_ROUNDS = 8;
	/**
	 * @brief Количество собираемых мелких таблиц
	 *
	 */
	static constexpr size_t SMALL_ROUNDS = 20000;

	/**
	 * @brief Пороги пропускной способности контейнера в мегабайтах в секунду
	 *
	 * @details Пороги сняты прогоном по двенадцати отладочным стендам x86_64
	 *          (14.08.2026) и назначены по самому медленному, делённому надвое - тем же
	 *          правилом, что и пороги чтения
	 *
	 */
	static constexpr double PARSE_LARGE_THRESHOLD = 16.0;
	/**
	 * @brief Порог пропускной способности обхода собранной таблицы
	 *
	 * @details Обход ведётся по указаниям в единое хранилище знаков: он обязан идти
	 *          много быстрее самого разбора, и просадка здесь означала бы, что поля
	 *          перестали быть указаниями и обратились в отдельные строки
	 *
	 */
	static constexpr double WALK_LARGE_THRESHOLD = 450.0;
	/**
	 * @brief Порог пропускной способности доступа по имени столбца
	 *
	 * @details Доступ по имени ведётся соответствием имён их номерам: поиск попарным
	 *          сличением имён обратил бы обход таблицы в квадратичный, и стережётся
	 *          здесь именно это
	 *
	 */
	static constexpr double COLUMN_LARGE_THRESHOLD = 280.0;
	/**
	 * @brief Порог пропускной способности перезаписи собранной таблицы
	 *
	 */
	static constexpr double TEXT_LARGE_THRESHOLD = 45.0;
	/**
	 * @brief Порог просадки потоковой выдачи записей обработчику
	 *
	 * @details Потоковая выдача проходит те же события разбора, что и сборка таблицы,
	 *          но таблицу не заполняет вовсе: она обязана идти не медленнее сборки.
	 *          Измеряется отношением ко времени сборки той же таблицы
	 *
	 */
	static constexpr double STREAM_LARGE_THRESHOLD = 1.2;
	/**
	 * @brief Порог объёма выделяемой памяти на одну запись при потоковой выдаче
	 *
	 * @details Потоковая выдача заведена ради таблиц, в память не помещающихся: в
	 *          памяти держится лишь текущая запись, и объём, приходящийся на одну
	 *          запись, от размера таблицы зависеть не должен вовсе. Рост показателя
	 *          означает, что таблица всё же оседает в памяти - то есть путь потерял то
	 *          единственное, ради чего заведён
	 *
	 * @note Порог задан с двукратным запасом от самого расточительного стенда: замер
	 *       по стендам даёт 46.45 октета у libc++ и 56.56 у libstdc++ - разнятся они
	 *       коротким запасом строки, а не системой
	 *
	 */
	static constexpr double STREAM_MEMORY_THRESHOLD = 128.0;
	/**
	 * @brief Порог количества выделений памяти на сборку крупной таблицы
	 *
	 * @details Поля хранятся указаниями в единое хранилище знаков, и количество
	 *          выделений на таблицу определяется наращиванием хранилищ - то есть
	 *          растёт логарифмом от размера, а не числом полей. Рост показателя
	 *          означает, что поле обратилось в отдельную строку
	 *
	 * @note Порог этот назначен впритык намеренно: он ловит именно то, что уже
	 *       случалось. Очередь событий разбора, заведённая двусторонней очередью,
	 *       выделяла по блоку памяти на каждые несколько десятков событий, и сборка
	 *       крупной таблицы стоила двадцати шести тысяч выделений вместо сотни
	 *
	 */
	static constexpr double PARSE_ALLOCATIONS_THRESHOLD = 128.0;
	/**
	 * @brief Порог задержки сборки таблицы ответа службы в микросекундах
	 *
	 */
	static constexpr double PARSE_SERVICE_LATENCY_THRESHOLD = 36.0;

	/**
	 * @brief Функция сборки таблицы целиком
	 *
	 * @param text разбираемый текст таблицы
	 * @return     количество записей собранной таблицы
	 *
	 */
	static uint64_t parse(const string & text) noexcept {
		// Объект контейнера таблицы
		awh::codec::csv::document_t document;
		/**
		 * Если разобрать текст таблицы не удалось
		 */
		if(!document.parse(text))
			// Выводим нулевое количество записей таблицы
			return 0;
		// Выводим количество записей собранной таблицы
		return static_cast <uint64_t> (document.rows());
	}
	/**
	 * @brief Функция потоковой выдачи записей обработчику
	 *
	 * @param text разбираемый текст таблицы
	 * @return     количество выданных записей
	 *
	 */
	static uint64_t stream(const string & text) noexcept {
		// Количество выданных записей
		uint64_t result = 0;
		// Объект контейнера таблицы
		awh::codec::csv::document_t document;
		// Выполняем потоковый разбор текста таблицы записями
		document.parse(text, [&result](const vector <string_view> & fields) noexcept -> bool {
			// Выполняем учёт количества полей выданной записи
			result += static_cast <uint64_t> (fields.size());
			// Выводим признак продолжения разбора
			return true;
		});
		// Выводим количество выданных записей
		return result;
	}
	/**
	 * @brief Функция получения собранной крупной таблицы
	 *
	 * @details Таблица собирается однажды и переиспользуется сценариями обхода:
	 *          сборка её внутри измеряемого цикла замерялась бы наравне с обходом
	 *
	 * @return собранная крупная таблица
	 *
	 */
	static const awh::codec::csv::document_t & assembled() noexcept {
		// Собранная крупная таблица
		static const awh::codec::csv::document_t result = []() noexcept -> awh::codec::csv::document_t {
			// Собираемые настройки контейнера таблицы
			awh::codec::csv::document_t::settings_t settings;
			// Объявляем наличие заголовка в разбираемой таблице
			settings.reader.header = awh::codec::csv::header_t::PRESENT;
			// Объект контейнера таблицы
			awh::codec::csv::document_t document(settings);
			// Выполняем разбор текста крупной таблицы
			document.parse(large());
			// Выводим собранную крупную таблицу
			return document;
		}();
		// Выводим собранную крупную таблицу
		return result;
	}
	/**
	 * @brief Функция прогона сценария сборки крупной таблицы
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t parseLarge() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст таблицы
		const string & text = large();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), ::LARGE_ROUNDS, [&text]() noexcept {
			// Выполняем сборку таблицы целиком
			return ::parse(text);
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария обхода собранной таблицы
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t walkLarge() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем собранную крупную таблицу
		const awh::codec::csv::document_t & document = ::assembled();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(large().size(), ::LARGE_ROUNDS, [&document]() noexcept {
			// Накопитель размеров содержимого полей
			uint64_t accumulator = 0;
			/**
			 * Выполняем перебор всех записей таблицы
			 */
			for(size_t i = 0; i < document.rows(); i++){
				/**
				 * Выполняем перебор всех полей записи
				 */
				for(size_t j = 0; j < document.size(i); j++)
					// Выполняем накопление размера содержимого поля
					accumulator += static_cast <uint64_t> (document.get(i, j).size());
			}
			// Выводим накопитель размеров содержимого полей
			return accumulator;
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария доступа по имени столбца
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t columnLarge() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем собранную крупную таблицу
		const awh::codec::csv::document_t & document = ::assembled();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(large().size(), ::LARGE_ROUNDS, [&document]() noexcept {
			// Накопитель размеров содержимого полей
			uint64_t accumulator = 0;
			/**
			 * Выполняем перебор всех записей таблицы
			 */
			for(size_t i = 0; i < document.rows(); i++){
				// Выполняем накопление размера содержимого поля имени
				accumulator += static_cast <uint64_t> (document.get(i, "name").size());
				// Выполняем накопление размера содержимого поля значения
				accumulator += static_cast <uint64_t> (document.get(i, "amount").size());
			}
			// Выводим накопитель размеров содержимого полей
			return accumulator;
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария перезаписи собранной таблицы
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t textLarge() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем собранную крупную таблицу
		const awh::codec::csv::document_t & document = ::assembled();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(large().size(), ::LARGE_ROUNDS, [&document]() noexcept {
			// Выводим размер перезаписанного текста таблицы
			return static_cast <uint64_t> (document.text().size());
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария просадки потоковой выдачи записей
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t streamLarge() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст таблицы
		const string & text = large();
		// Выполняем прогон сборки таблицы целиком
		const outcome_t whole = measure(text.size(), ::LARGE_ROUNDS, [&text]() noexcept {
			// Выполняем сборку таблицы целиком
			return ::parse(text);
		});
		// Выполняем прогон потоковой выдачи записей обработчику
		const outcome_t streamed = measure(text.size(), ::LARGE_ROUNDS, [&text]() noexcept {
			// Выполняем потоковую выдачу записей обработчику
			return ::stream(text);
		});
		/**
		 * Если хотя бы один из прогонов не состоялся
		 */
		if((whole.seconds <= 0.0) || (streamed.seconds <= 0.0)){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Выводим результат измерения
			return result;
		}
		// Устанавливаем измеренное значение
		result.value = (streamed.seconds / whole.seconds);
		// Устанавливаем сведения о прогоне
		result.details = details(streamed);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария объёма памяти потоковой выдачи записей
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t streamMemory() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст таблицы
		const string & text = large();
		// Количество выданных записей за один прогон
		size_t records = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), ::LARGE_ROUNDS, [&text, &records]() noexcept {
			// Объект контейнера таблицы
			awh::codec::csv::document_t document;
			// Количество выданных записей
			uint64_t count = 0;
			// Выполняем потоковый разбор текста таблицы записями
			document.parse(text, [&count](const vector <string_view> &) noexcept -> bool {
				// Выполняем учёт выданной записи
				count++;
				// Выводим признак продолжения разбора
				return true;
			});
			// Запоминаем количество выданных записей за один прогон
			records = static_cast <size_t> (count);
			// Выводим количество выданных записей
			return count;
		});
		// Устанавливаем измеренное значение
		result.value = perRecord(outcome, (records * ::LARGE_ROUNDS));
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария расхода выделений памяти на сборку таблицы
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t parseAllocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст таблицы
		const string & text = large();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), ::LARGE_ROUNDS, [&text]() noexcept {
			// Выполняем сборку таблицы целиком
			return ::parse(text);
		});
		// Устанавливаем измеренное значение
		result.value = perDocument(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария задержки сборки таблицы ответа службы
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t latencyService() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст таблицы
		const string & text = service();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), ::SMALL_ROUNDS, [&text]() noexcept {
			// Выполняем сборку таблицы целиком
			return ::parse(text);
		});
		// Устанавливаем измеренное значение
		result.value = perLatency(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	/**
	 * Выполняем регистрацию сценария сборки крупной таблицы
	 */
	static const bool PARSE_REGISTERED = awh::benchmark::add(
		"codec/csv: сборка крупной таблицы", "МБ/с", ::PARSE_LARGE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, ::parseLarge
	);
	/**
	 * Выполняем регистрацию сценария обхода собранной таблицы
	 */
	static const bool WALK_REGISTERED = awh::benchmark::add(
		"codec/csv: обход собранной таблицы", "МБ/с", ::WALK_LARGE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, ::walkLarge
	);
	/**
	 * Выполняем регистрацию сценария доступа по имени столбца
	 */
	static const bool COLUMN_REGISTERED = awh::benchmark::add(
		"codec/csv: доступ по имени столбца", "МБ/с", ::COLUMN_LARGE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, ::columnLarge
	);
	/**
	 * Выполняем регистрацию сценария перезаписи собранной таблицы
	 */
	static const bool TEXT_REGISTERED = awh::benchmark::add(
		"codec/csv: перезапись собранной таблицы", "МБ/с", ::TEXT_LARGE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, ::textLarge
	);
	/**
	 * Выполняем регистрацию сценария просадки потоковой выдачи записей
	 */
	static const bool STREAM_REGISTERED = awh::benchmark::add(
		"codec/csv: просадка потоковой выдачи", "раз", ::STREAM_LARGE_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, ::streamLarge
	);
	/**
	 * Выполняем регистрацию сценария объёма памяти потоковой выдачи записей
	 */
	static const bool MEMORY_REGISTERED = awh::benchmark::add(
		"codec/csv: память потоковой выдачи", "окт./табл.", ::STREAM_MEMORY_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, ::streamMemory
	);
	/**
	 * Выполняем регистрацию сценария расхода выделений памяти на сборку таблицы
	 */
	static const bool ALLOCATIONS_REGISTERED = awh::benchmark::add(
		"codec/csv: выделения на сборку таблицы", "выд./табл.", ::PARSE_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, ::parseAllocations
	);
	/**
	 * Выполняем регистрацию сценария задержки сборки таблицы ответа службы
	 */
	static const bool SERVICE_LATENCY_REGISTERED = awh::benchmark::add(
		"codec/csv: задержка сборки ответа службы", "мкс/табл.", ::PARSE_SERVICE_LATENCY_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, ::latencyService
	);
};
