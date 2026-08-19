/**
 * @file common.hpp
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
 * @brief Общее окружение эталонных стендов сравнения бинарного контейнера ABC —
 *        образец содержимого, параметры нагрузки, разбор параметров запуска и
 *        вывод результатов
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_BENCHMARK_RIVAL_ABC__
#define __AWH_BENCHMARK_RIVAL_ABC__

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <cstdint>
#include <string>
#include <chrono>
#include <vector>

/**
 * @brief Пространство имён эталонных стендов сравнения бинарного контейнера ABC
 *
 * @details Двоичные записи сличаются иначе, нежели текстовые: одного эталонного
 *          документа тут быть не может вовсе, ибо всякая реализация пишет своими
 *          октетами. Оттого эталоном служит СОДЕРЖИМОЕ, а не запись: стенды строят
 *          из него запись своим средством, а затем её же разбирают
 *
 * @details Мерятся оба пути - сборка записи и разбор её вместе с полным обходом, - и
 *          отдельно доносится размер собранной записи. Разбор без обхода сличать
 *          нельзя: реализация с отложенным разбором выглядела бы быстрее прочих,
 *          ничего для того не сделав
 *
 */
namespace rival {
	/**
	 * @brief Количество однородных записей образца обиходного вида
	 *
	 */
	static constexpr size_t OBJECT_COUNT = 100000;

	/**
	 * @brief Количество значений образцов с преобладанием одного вида
	 *
	 */
	static constexpr size_t FOCUSED_COUNT = 400000;

	/**
	 * @brief Количество двоичных значений образца
	 *
	 */
	static constexpr size_t BLOB_COUNT = 100000;

	/**
	 * @brief Количество ветвей образца с глубокой вложенностью
	 *
	 */
	static constexpr size_t NESTED_COUNT = 20000;

	/**
	 * @brief Глубина вложенности образца с глубокой вложенностью
	 *
	 */
	static constexpr uint32_t NESTED_DEPTH = 24;

	/**
	 * @brief Количество прогонов сценариев крупного образца
	 *
	 */
	static constexpr size_t LARGE_ROUNDS = 8;

	/**
	 * @brief Количество прогонов сценария малого образца
	 *
	 */
	static constexpr size_t SMALL_ROUNDS = 200000;

	/**
	 * @brief Разновидности числа образца с преобладанием чисел
	 *
	 */
	enum class numeric_t : uint8_t {
		NATURAL  = 0x00, // Целое без знака
		INTEGER  = 0x01, // Целое со знаком, меньшее нуля
		REAL     = 0x02, // Дробное число
		LARGE    = 0x03  // Целое без знака, не вмещающееся в четыре октета
	};

	/**
	 * @brief Число образца с преобладанием чисел
	 *
	 */
	struct number_t {
		// Разновидность числа образца
		numeric_t kind;
		// Целое без знака
		uint64_t natural;
		// Целое со знаком
		int64_t integer;
		// Дробное число
		double real;
	};

	/**
	 * @brief Однородная запись образца обиходного вида
	 *
	 */
	struct object_t {
		// Опознаватель записи
		uint64_t id;
		// Название записи
		std::string name;
		// Город записи
		std::string city;
		// Величина записи
		double amount;
		// Признак деятельности записи
		bool active;
	};

	/**
	 * @brief Контрольная сумма содержимого, прочитанного стендом
	 *
	 * @details Складывается из октетов строк и из значений чисел. Совпадение суммы у
	 * всех стендов означает, что они прочитали одно и то же содержимое, а не разный
	 * его объём
	 *
	 */
	static volatile uint64_t checksum = 0;

	/**
	 * @brief Количество значений, прочитанных стендом
	 *
	 */
	static volatile uint64_t values = 0;

	/**
	 * @brief Метод учёта прочитанного отрезка октетов в контрольной сумме
	 *
	 * @param buffer учитываемая последовательность октетов
	 * @param size   размер учитываемой последовательности
	 *
	 */
	static inline void consume(const void * buffer, const size_t size) noexcept {
		// Накапливаемая сумма октетов
		size_t sum = 0;
		/**
		 * Если учитываемая последовательность передана
		 */
		if((buffer != nullptr) && (size > 0)){
			// Получаем учитываемую последовательность октетов
			const uint8_t * data = reinterpret_cast <const uint8_t *> (buffer);
			/**
			 * Выполняем перебор всех октетов последовательности
			 */
			for(size_t i = 0; i < size; i++)
				// Выполняем накопление суммы октетов
				sum += data[i];
		}
		// Выполняем учёт выполненной работы в контрольной сумме
		checksum += (sum + size);
		// Выполняем учёт прочитанного значения
		values += 1;
	}
	/**
	 * @brief Метод учёта прочитанного числа в контрольной сумме
	 *
	 * @details Число учитывается целой своей частью намеренно: одни реализации выдают
	 * целое число видом с плавающей запятой, другие - целым, и сложение дробных частей
	 * развело бы контрольные суммы там, где работа одна и та же
	 *
	 * @param value учитываемое число
	 *
	 */
	static inline void consume(const double value) noexcept {
		// Выполняем учёт выполненной работы в контрольной сумме
		checksum += static_cast <uint64_t> (static_cast <int64_t> (value));
		// Выполняем учёт прочитанного значения
		values += 1;
	}
	/**
	 * @brief Метод учёта прочитанного логического значения в контрольной сумме
	 *
	 * @param value учитываемое логическое значение
	 *
	 */
	static inline void consume(const bool value) noexcept {
		// Выполняем учёт выполненной работы в контрольной сумме
		checksum += (value ? 3u : 5u);
		// Выполняем учёт прочитанного значения
		values += 1;
	}
	/**
	 * @brief Метод учёта прочитанного пустого значения в контрольной сумме
	 *
	 */
	static inline void nothing() noexcept {
		// Выполняем учёт выполненной работы в контрольной сумме
		checksum += 7u;
		// Выполняем учёт прочитанного значения
		values += 1;
	}
	/**
	 * @brief Метод получения образца обиходного вида
	 *
	 * @details Перечень однородных записей о немногих полях - вид этот выходит из
	 * служб и выгрузок чаще всякого иного
	 *
	 * @return образец обиходного вида
	 *
	 */
	static inline const std::vector <object_t> & objects() noexcept {
		// Образец обиходного вида
		static const std::vector <object_t> result = []() noexcept -> std::vector <object_t> {
			// Собираемый образец обиходного вида
			std::vector <object_t> result;
			// Выполняем заведение места под записи образца
			result.reserve(OBJECT_COUNT);
			/**
			 * Выполняем сборку всех записей образца
			 */
			for(size_t i = 0; i < OBJECT_COUNT; i++){
				// Собираемая запись образца
				object_t object;
				// Выполняем установку опознавателя записи
				object.id = static_cast <uint64_t> (i);
				// Выполняем установку названия записи
				object.name = ("Товар " + std::to_string(i));
				// Выполняем установку города записи
				object.city = "Москва";
				// Выполняем установку величины записи
				object.amount = (static_cast <double> (i % 9973) + (static_cast <double> (i % 100) / 100.0));
				// Выполняем установку признака деятельности записи
				object.active = ((i % 3) != 0);
				// Выполняем добавление записи в образец
				result.push_back(object);
			}
			// Выводим собранный образец
			return result;
		}();
		// Выводим образец обиходного вида
		return result;
	}
	/**
	 * @brief Метод получения образца с преобладанием чисел
	 *
	 * @return образец с преобладанием чисел
	 *
	 */
	static inline const std::vector <number_t> & numbers() noexcept {
		// Образец с преобладанием чисел
		static const std::vector <number_t> result = []() noexcept -> std::vector <number_t> {
			// Собираемый образец с преобладанием чисел
			std::vector <number_t> result;
			// Выполняем заведение места под числа образца
			result.reserve(FOCUSED_COUNT);
			/**
			 * Выполняем сборку всех чисел образца
			 */
			for(size_t i = 0; i < FOCUSED_COUNT; i++){
				// Собираемое число образца
				number_t number{numeric_t::NATURAL, 0, 0, 0.0};
				// Выполняем установку разновидности числа образца
				number.kind = static_cast <numeric_t> (i % 4);
				/**
				 * Определяем разновидность числа образца
				 */
				switch(static_cast <uint8_t> (number.kind)){
					// Если числом является целое без знака
					case static_cast <uint8_t> (numeric_t::NATURAL):
						number.natural = static_cast <uint64_t> (i % 100000);
					break;
					// Если числом является целое со знаком
					case static_cast <uint8_t> (numeric_t::INTEGER):
						number.integer = -static_cast <int64_t> (i % 4096);
					break;
					// Если числом является дробное
					case static_cast <uint8_t> (numeric_t::REAL):
						number.real = (static_cast <double> (i % 1000) + (static_cast <double> (i % 100) / 100.0));
					break;
					// Если числом является крупное целое без знака
					case static_cast <uint8_t> (numeric_t::LARGE):
						number.natural = (static_cast <uint64_t> (i) * 1000000007ull);
					break;
				}
				// Выполняем добавление числа в образец
				result.push_back(number);
			}
			// Выводим собранный образец
			return result;
		}();
		// Выводим образец с преобладанием чисел
		return result;
	}
	/**
	 * @brief Метод получения образца с преобладанием строк
	 *
	 * @details Часть строк несёт знаки вне US-ASCII: длина записи их иная, а у
	 * реализации, проверяющей строки на соответствие кодировке, иная и стоимость
	 *
	 * @return образец с преобладанием строк
	 *
	 */
	static inline const std::vector <std::string> & strings() noexcept {
		// Образец с преобладанием строк
		static const std::vector <std::string> result = []() noexcept -> std::vector <std::string> {
			// Собираемый образец с преобладанием строк
			std::vector <std::string> result;
			// Выполняем заведение места под строки образца
			result.reserve(FOCUSED_COUNT);
			/**
			 * Выполняем сборку всех строк образца
			 */
			for(size_t i = 0; i < FOCUSED_COUNT; i++){
				/**
				 * Определяем вид очередной строки образца
				 */
				switch(i % 4){
					// Если строка знаков вне US-ASCII не несёт
					case 0: result.push_back("Простое значение " + std::to_string(i)); break;
					// Если строка несёт кавычки и знаки отмены
					case 1: result.push_back("Значение с \"кавычками\" и \\ знаком отмены " + std::to_string(i)); break;
					// Если строка несёт знаки кириллицы и иероглифы
					case 2: result.push_back("Значение по-русски " + std::to_string(i) + " и по-японски 漢字"); break;
					// Если строка коротка
					case 3: result.push_back("k" + std::to_string(i)); break;
				}
			}
			// Выводим собранный образец
			return result;
		}();
		// Выводим образец с преобладанием строк
		return result;
	}
	/**
	 * @brief Метод получения образца с преобладанием двоичных значений
	 *
	 * @details Двоичное значение есть то, ради чего двоичный контейнер и берут: в
	 * текстовом виде оно потребовало бы перекодировки, а здесь ложится как есть
	 *
	 * @return образец с преобладанием двоичных значений
	 *
	 */
	static inline const std::vector <std::vector <uint8_t>> & blobs() noexcept {
		// Образец с преобладанием двоичных значений
		static const std::vector <std::vector <uint8_t>> result = []() noexcept -> std::vector <std::vector <uint8_t>> {
			// Собираемый образец с преобладанием двоичных значений
			std::vector <std::vector <uint8_t>> result;
			// Выполняем заведение места под двоичные значения образца
			result.reserve(BLOB_COUNT);
			/**
			 * Выполняем сборку всех двоичных значений образца
			 */
			for(size_t i = 0; i < BLOB_COUNT; i++){
				// Выполняем получение размера очередного двоичного значения
				const size_t size = (16 + (i % 48));
				// Собираемое двоичное значение образца
				std::vector <uint8_t> blob(size, 0);
				/**
				 * Выполняем сборку октетов двоичного значения
				 */
				for(size_t j = 0; j < size; j++)
					// Выполняем установку очередного октета двоичного значения
					blob[j] = static_cast <uint8_t> ((i + (j * 31)) & 0xFF);
				// Выполняем добавление двоичного значения в образец
				result.push_back(blob);
			}
			// Выводим собранный образец
			return result;
		}();
		// Выводим образец с преобладанием двоичных значений
		return result;
	}
	/**
	 * @brief Метод получения имён полей образца обиходного вида
	 *
	 * @return имена полей образца обиходного вида
	 *
	 */
	static inline const char * const * fields() noexcept {
		/**
		 * Имена полей образца обиходного вида.
		 *
		 * Идут они по возрастанию записи намеренно: строгий вид записи ABC того и
		 * требует, а прочим реализациям порядок безразличен. Поле `note` несёт пустое
		 * значение: пустое значение есть отдельный вид у всех трёх записей
		 */
		static const char * const result[] = {"active", "amount", "city", "id", "name", "note"};
		// Выводим имена полей образца обиходного вида
		return result;
	}
	/**
	 * @brief Структура итогов прогона сценария
	 *
	 */
	struct outcome_t {
		// Количество собранных либо разобранных записей
		size_t rounds;
		// Размер собранной записи в октетах
		size_t size;
		// Длительность сборки записи в секундах
		double writing;
		// Длительность разбора записи в секундах
		double reading;
	};

	/**
	 * @brief Метод получения пропускной способности
	 *
	 * @param bytes   количество обработанных октетов
	 * @param seconds длительность замера в секундах
	 * @return        пропускная способность в мегабайтах в секунду
	 *
	 */
	static inline double megabytes(const size_t bytes, const double seconds) noexcept {
		/**
		 * Если замер не состоялся
		 */
		if(seconds <= 0.0)
			// Выводим нулевую пропускную способность
			return 0.0;
		// Выводим пропускную способность
		return ((static_cast <double> (bytes) / (1024.0 * 1024.0)) / seconds);
	}
	/**
	 * @brief Метод вывода результата прогона сценария
	 *
	 * @param name    название сценария
	 * @param outcome итоги прогона сценария
	 *
	 */
	static inline void report(const char * name, const outcome_t & outcome) noexcept {
		// Выполняем получение количества обработанных октетов
		const size_t bytes = (outcome.size * outcome.rounds);
		// Выводим результат прогона сценария
		::printf("%-10s write %10.2f MB/s  read %10.2f MB/s  size %10zu B\n", name,
			megabytes(bytes, outcome.writing), megabytes(bytes, outcome.reading), outcome.size);
	}
	/**
	 * @brief Метод вывода сообщения о пропуске сценария
	 *
	 * @param name   название сценария
	 * @param reason причина пропуска сценария
	 *
	 */
	static inline void skip(const char * name, const char * reason) noexcept {
		// Выводим сообщение о пропуске сценария
		::printf("%-10s %s\n", name, reason);
	}
	/**
	 * @brief Метод проверки отбора сценария
	 *
	 * @param name   название сценария
	 * @param filter отбор сценариев по вхождению в название
	 * @return       признак отбора сценария
	 *
	 */
	static inline bool selected(const char * name, const char * filter) noexcept {
		// Выводим признак отбора сценария
		return ((filter == nullptr) || (::strstr(name, filter) != nullptr));
	}
	/**
	 * @brief Метод получения отбора сценариев из параметров запуска
	 *
	 * @param argc длина массива параметров
	 * @param argv массив параметров
	 * @return     отбор сценариев по вхождению в название
	 *
	 */
	static inline const char * filter(const int32_t argc, char ** argv) noexcept {
		/**
		 * Выполняем перебор всех параметров запуска
		 */
		for(int32_t i = 1; i < argc; i++){
			/**
			 * Если параметром является отбор сценариев
			 */
			if(::strncmp(argv[i], "--filter=", 9) == 0)
				// Выводим отбор сценариев по вхождению в название
				return (argv[i] + 9);
		}
		// Выводим пустой отбор сценариев
		return nullptr;
	}
	/**
	 * @brief Метод вывода контрольной суммы работы, выполненной стендом
	 *
	 * @param argc длина массива параметров
	 * @param argv массив параметров
	 *
	 */
	static inline void digest(const int32_t argc, char ** argv) noexcept {
		/**
		 * Выполняем перебор всех параметров запуска
		 */
		for(int32_t i = 1; i < argc; i++){
			/**
			 * Если параметром является вывод контрольной суммы
			 */
			if(::strcmp(argv[i], "--checksum") == 0){
				// Выводим контрольную сумму содержимого и количество прочитанных значений
				::printf("checksum %llu values %llu\n",
					static_cast <unsigned long long> (checksum),
					static_cast <unsigned long long> (values));
				// Выходим из перебора параметров запуска
				return;
			}
		}
	}
	/**
	 * @brief Разновидности сценариев стенда
	 *
	 */
	enum class scene_t : uint8_t {
		OBJECTS = 0x00, // Перечень однородных записей обиходного вида
		NUMBERS = 0x01, // Перечень чисел разных видов
		STRINGS = 0x02, // Перечень строк
		BLOBS   = 0x03, // Перечень двоичных значений
		NESTED  = 0x04, // Глубокая вложенность
		SMALL   = 0x05  // Малая запись, собираемая и разбираемая многие тысячи раз
	};

	/**
	 * @brief Структура сценария стенда
	 *
	 */
	struct scenario_t {
		// Разновидность сценария стенда
		scene_t scene;
		// Название сценария
		const char * name;
		// Количество прогонов сборки и разбора
		size_t rounds;
	};

	/**
	 * @brief Метод получения перечня сценариев стенда
	 *
	 * @details Перечень этот принадлежит общему окружению, а не стендам: сценарии,
	 * заведённые у всякого стенда порознь, разошлись бы составом или количеством
	 * прогонов, а сличать разный объём работы бессмысленно
	 *
	 * @return перечень сценариев стенда
	 *
	 */
	static inline const std::vector <scenario_t> & scenarios() noexcept {
		// Перечень сценариев стенда
		static const std::vector <scenario_t> result = {
			{scene_t::OBJECTS, "objects", LARGE_ROUNDS},
			{scene_t::NUMBERS, "numbers", LARGE_ROUNDS},
			{scene_t::STRINGS, "strings", LARGE_ROUNDS},
			{scene_t::BLOBS,   "blobs",   LARGE_ROUNDS},
			{scene_t::NESTED,  "nested",  LARGE_ROUNDS},
			{scene_t::SMALL,   "small",   SMALL_ROUNDS}
		};
		// Выводим перечень сценариев стенда
		return result;
	}
	/**
	 * @brief Работы стенда, сличаемые с прочими стендами
	 *
	 * @details Сборка записи и разбор её ведутся средством самого стенда, а образец
	 * содержимого и порядок обхода - общие
	 *
	 */
	struct stand_t {
		// Работа сборки записи из образца
		bool (* write) (const scene_t scene, std::string & record);
		// Работа разбора записи вместе с полным обходом
		bool (* read) (const scene_t scene, const std::string & record);
	};

	/**
	 * @brief Метод прогона всех сценариев стенда
	 *
	 * @param stand работы стенда
	 * @param argc  длина массива параметров
	 * @param argv  массив параметров
	 * @return      код выхода из стенда
	 *
	 */
	static inline int32_t drive(const stand_t & stand, const int32_t argc, char ** argv) noexcept {
		// Получаем отбор сценариев по вхождению в название
		const char * chosen = filter(argc, argv);
		/**
		 * Выполняем перебор всех сценариев стенда
		 */
		for(const auto & scenario : scenarios()){
			/**
			 * Если сценарий отбором не выбран
			 */
			if(!selected(scenario.name, chosen))
				// Выполняем переход к следующему сценарию
				continue;
			// Итоги прогона сценария
			outcome_t outcome{scenario.rounds, 0, 0.0, 0.0};
			// Собираемая запись сценария
			std::string record;
			/**
			 * Если собрать запись сценария не удалось
			 */
			if(!stand.write(scenario.scene, record)){
				// Выводим сообщение о пропуске сценария
				skip(scenario.name, "writing failed");
				// Выполняем переход к следующему сценарию
				continue;
			}
			// Запоминаем размер собранной записи
			outcome.size = record.size();
			// Запоминаем время начала замера сборки
			auto start = std::chrono::steady_clock::now();
			/**
			 * Выполняем прогон сборки записи заданное количество раз
			 */
			for(size_t i = 0; i < scenario.rounds; i++){
				// Собираемая запись очередного прогона
				std::string produced;
				/**
				 * Если собрать запись очередного прогона не удалось
				 */
				if(!stand.write(scenario.scene, produced)){
					// Выводим сообщение о пропуске сценария
					skip(scenario.name, "writing failed");
					// Выполняем выход из прогона сборки
					return EXIT_FAILURE;
				}
			}
			// Запоминаем длительность сборки записи
			outcome.writing = std::chrono::duration <double> (std::chrono::steady_clock::now() - start).count();
			// Запоминаем время начала замера разбора
			start = std::chrono::steady_clock::now();
			/**
			 * Выполняем прогон разбора записи заданное количество раз
			 */
			for(size_t i = 0; i < scenario.rounds; i++){
				/**
				 * Если разобрать запись не удалось
				 */
				if(!stand.read(scenario.scene, record)){
					// Выводим сообщение о пропуске сценария
					skip(scenario.name, "reading failed");
					// Выполняем выход из прогона разбора
					return EXIT_FAILURE;
				}
			}
			// Запоминаем длительность разбора записи
			outcome.reading = std::chrono::duration <double> (std::chrono::steady_clock::now() - start).count();
			// Выводим результат прогона сценария
			report(scenario.name, outcome);
		}
		// Выводим контрольную сумму работы, выполненной стендом
		digest(argc, argv);
		// Выводим успешный код выхода из стенда
		return EXIT_SUCCESS;
	}
};

#endif // __AWH_BENCHMARK_RIVAL_ABC__
