/**
 * @file common.hpp
 * @date 2026-08-16
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
 * @brief Общее окружение эталонных стендов сравнения контейнера TOML —
 *        эталонные тексты настроек, параметры нагрузки, учёт выполненной работы,
 *        разбор параметров запуска и вывод результатов
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_BENCHMARK_RIVAL_TOML__
#define __AWH_BENCHMARK_RIVAL_TOML__

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

/**
 * @brief Пространство имён эталонных стендов сравнения контейнера TOML
 *
 * @details Эталонные тексты настроек и параметры нагрузки обязаны совпадать со
 *          сценариями `benchmark/codec/toml` библиотеки AWH: сравниваются
 *          реализации разбора, а не разные объёмы работы, поэтому любое
 *          расхождение здесь обесценивает отчёт целиком
 *
 */
namespace rival {
	/**
	 * @brief Размер эталонного крупного файла настроек в октетах
	 *
	 */
	static constexpr size_t LARGE_SIZE = (16 * 1024 * 1024);

	/**
	 * @brief Размер эталонных текстов с преобладанием одного вида записи в октетах
	 *
	 */
	static constexpr size_t FOCUSED_SIZE = (4 * 1024 * 1024);

	/**
	 * @brief Количество пар в одной таблице крупного файла настроек
	 *
	 */
	static constexpr size_t TABLE_KEYS = 32;

	/**
	 * @brief Количество прогонов сценария мелкого файла настроек
	 *
	 */
	static constexpr size_t SMALL_ROUNDS = 20000;

	/**
	 * @brief Количество прогонов сценария крупного файла настроек
	 *
	 */
	static constexpr size_t LARGE_ROUNDS = 8;

	/**
	 * @brief Количество прогонов сценариев с преобладанием одного вида записи
	 *
	 */
	static constexpr size_t FOCUSED_ROUNDS = 20;

	/**
	 * @brief Структура итогов прогона сценария
	 *
	 */
	struct outcome_t {
		// Количество разобранных файлов настроек
		size_t documents;
		// Количество разобранных октетов
		size_t bytes;
		// Длительность замера в секундах
		double seconds;
	};

	/**
	 * @brief Контрольная сумма строковых значений, прочитанных стендом
	 *
	 * @details Складывается из октетов строковых значений. Совпадение суммы у
	 * стендов означает, что они прочитали одни и те же значения, а не разный их
	 * объём
	 *
	 * @note В сумму входят лишь значения строковые, но не числа, логические
	 * значения и отметки времени. Реализации выдают их разными типами языка -
	 * числом с плавающей точкой, целым числом заданной разрядности, собственной
	 * структурой отметки, - и складывать их в одну сумму значило бы объявить
	 * расхождением разницу представлений, а не разницу выполненной работы.
	 * Количество прочитанных пар при этом учитывается для всех значений без
	 * изъятия, и оно у стендов совпадать обязано
	 *
	 */
	static volatile uint64_t checksum = 0;

	/**
	 * @brief Количество пар, обработанных стендом
	 *
	 */
	static volatile uint64_t entries = 0;

	/**
	 * @brief Приёмник имён таблиц и ключей
	 *
	 * @details Имена в контрольную сумму не входят: реализации выдают их
	 * по-разному - составным именем целиком либо частями. Читать их стенд, однако,
	 * обязан: без чтения часть реализаций работу по их выдаче попросту не выполнит,
	 * и сравнение потеряет смысл
	 *
	 */
	static volatile uint64_t sink = 0;

	/**
	 * @brief Метод чтения имени таблицы либо ключа
	 *
	 * @param buffer читаемая последовательность знаков
	 * @param size   размер читаемой последовательности
	 *
	 */
	static inline void touch(const void * buffer, const size_t size) noexcept {
		// Накапливаемая сумма октетов
		size_t sum = 0;
		/**
		 * Если читаемая последовательность передана
		 */
		if((buffer != nullptr) && (size > 0)){
			// Получаем читаемую последовательность знаков
			const uint8_t * data = reinterpret_cast <const uint8_t *> (buffer);
			/**
			 * Выполняем перебор всех октетов последовательности
			 */
			for(size_t i = 0; i < size; i++)
				// Выполняем накопление суммы октетов
				sum += data[i];
		}
		// Выполняем накопление прочитанного в приёмнике
		sink += (sum + size);
	}
	/**
	 * @brief Метод учёта обработанной пары
	 *
	 */
	static inline void entry() noexcept {
		// Выполняем учёт обработанной пары
		entries += 1;
	}
	/**
	 * @brief Метод учёта прочитанного строкового значения в контрольной сумме
	 *
	 * @param buffer учитываемая последовательность знаков
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
			// Получаем учитываемую последовательность знаков
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
	}
	/**
	 * @brief Метод получения десятичной записи числа
	 *
	 * @param value записываемое число
	 * @return      десятичная запись числа
	 *
	 */
	static inline std::string number(const uint32_t value) noexcept {
		// Выводим десятичную запись числа
		return std::to_string(value);
	}
	/**
	 * @brief Метод получения эталонного файла настроек приложения
	 *
	 * @details Все эталонные тексты возвращаются объектом строки и разбираются по
	 * указателям на её данные: сборка текста внутри измеряемого цикла выделяла бы
	 * память и вносила бы в замер стоимость работы со строкой вместо стоимости
	 * разбора
	 *
	 * @return эталонный текст настроек
	 *
	 */
	static inline const std::string & service() noexcept {
		// Эталонный текст настроек приложения
		static const std::string result =
			"# настройки службы\n"
			"title = \"служба обмена сообщениями\"\n"
			"version = \"1.4.2\"\n"
			"started = 1979-05-27T07:32:00Z\n"
			"\n"
			"[server]\n"
			"host = \"127.0.0.1\"\n"
			"port = 8080\n"
			"backlog = 512\n"
			"secure = true\n"
			"timeout = 30.5\n"
			"mask = 0xFF00\n"
			"hosts = [\"первый\", \"второй\", \"третий\"]\n"
			"\n"
			"[server.limits]\n"
			"# предельные величины\n"
			"connections = 10000\n"
			"requests = 1_000_000\n"
			"payload = 0x10_0000\n"
			"\n"
			"[logging]\n"
			"level = \"debug\" # уровень подробности\n"
			"path = 'C:\\logs\\service.log'\n"
			"rotate = true\n"
			"keep = 14\n"
			"\n"
			"[database]\n"
			"driver = \"postgres\"\n"
			"dsn = \"host=localhost user=awh dbname=service\"\n"
			"pool = { size = 16, idle = 4 }\n"
			"\n"
			"[[endpoint]]\n"
			"path = \"/api/v1/messages\"\n"
			"methods = [\"GET\", \"POST\"]\n"
			"\n"
			"[[endpoint]]\n"
			"path = \"/api/v1/status\"\n"
			"methods = [\"GET\"]\n";
		// Выводим эталонный текст настроек приложения
		return result;
	}
	/**
	 * @brief Метод получения эталонного крупного файла настроек
	 *
	 * @return эталонный текст настроек
	 *
	 */
	static inline const std::string & large() noexcept {
		// Эталонный крупный текст настроек
		static const std::string result = []() noexcept -> std::string {
			// Собираемый текст настроек
			std::string text;
			// Выполняем резервирование памяти под собираемый текст
			text.reserve(LARGE_SIZE + 4096);
			// Порядковый номер собираемой таблицы
			uint32_t index = 0;
			/**
			 * Выполняем сборку текста настроек до достижения требуемого размера
			 */
			while(text.length() < LARGE_SIZE){
				// Выполняем запись объявления очередной таблицы
				text.append("[table").append(number(index)).append("]\n");
				/**
				 * Выполняем сборку всех пар очередной таблицы
				 */
				for(uint32_t i = 0; i < TABLE_KEYS; i++){
					// Выполняем запись имени ключа очередной пары
					text.append("key").append(number(i)).append(" = ");
					/**
					 * Выполняем выбор вида значения очередной пары
					 */
					switch(i % 4){
						// Если значением является последовательность знаков
						case 0: text.append("\"значение ").append(number(i)).append("\"\n"); break;
						// Если значением является целое число
						case 1: text.append(number(i * 1000)).append("\n"); break;
						// Если значением является логическое значение
						case 2: text.append((i % 8) == 2 ? "true\n" : "false\n"); break;
						// Если значением является число с плавающей точкой
						case 3: text.append(number(i)).append(".25\n"); break;
					}
				}
				// Выполняем запись пустой строки за таблицей
				text.append("\n");
				// Выполняем переход к следующей таблице
				index++;
			}
			// Выводим собранный текст настроек
			return text;
		}();
		// Выводим эталонный крупный текст настроек
		return result;
	}
	/**
	 * @brief Метод получения эталонного текста с преобладанием строк
	 *
	 * @return эталонный текст настроек
	 *
	 */
	static inline const std::string & strings() noexcept {
		// Эталонный текст настроек с преобладанием строк
		static const std::string result = []() noexcept -> std::string {
			// Собираемый текст настроек
			std::string text;
			// Выполняем резервирование памяти под собираемый текст
			text.reserve(FOCUSED_SIZE + 4096);
			// Выполняем запись объявления таблицы
			text.append("[strings]\n");
			// Порядковый номер собираемой пары
			uint32_t index = 0;
			/**
			 * Выполняем сборку текста настроек до достижения требуемого размера
			 */
			while(text.length() < FOCUSED_SIZE){
				// Выполняем запись имени ключа очередной пары
				text.append("key").append(number(index)).append(" = ");
				/**
				 * Выполняем выбор ограды строкового значения очередной пары
				 */
				switch(index % 3){
					// Если строка записана основной оградой
					case 0: text.append("\"обыкновенное значение с пробелами\"\n"); break;
					// Если строка записана основной оградой с управляющими последовательностями
					case 1: text.append("\"путь\\\\к\\tфайлу\\u0041 и \\\"кавычки\\\"\"\n"); break;
					// Если строка записана дословной оградой
					case 2: text.append("'C:\\путь\\без\\ограды\\и\\последовательностей'\n"); break;
				}
				// Выполняем переход к следующей паре
				index++;
			}
			// Выводим собранный текст настроек
			return text;
		}();
		// Выводим эталонный текст настроек с преобладанием строк
		return result;
	}
	/**
	 * @brief Метод получения эталонного текста с преобладанием чисел
	 *
	 * @return эталонный текст настроек
	 *
	 */
	static inline const std::string & numbers() noexcept {
		// Эталонный текст настроек с преобладанием чисел
		static const std::string result = []() noexcept -> std::string {
			// Собираемый текст настроек
			std::string text;
			// Выполняем резервирование памяти под собираемый текст
			text.reserve(FOCUSED_SIZE + 4096);
			// Выполняем запись объявления таблицы
			text.append("[numbers]\n");
			// Порядковый номер собираемой пары
			uint32_t index = 0;
			/**
			 * Выполняем сборку текста настроек до достижения требуемого размера
			 */
			while(text.length() < FOCUSED_SIZE){
				// Выполняем запись имени ключа очередной пары
				text.append("key").append(number(index)).append(" = ");
				/**
				 * Выполняем выбор записи числового значения очередной пары
				 */
				switch(index % 5){
					// Если число записано десятичной системой счисления
					case 0: text.append("-").append(number(index)).append("\n"); break;
					// Если число записано шестнадцатеричной системой счисления
					case 1: text.append("0xDEAD_BEEF\n"); break;
					// Если число записано восьмеричной системой счисления
					case 2: text.append("0o755\n"); break;
					// Если число записано с плавающей точкой
					case 3: text.append("3.14159265358979\n"); break;
					// Если значением является отметка времени
					case 4: text.append("1979-05-27T07:32:00.999999Z\n"); break;
				}
				// Выполняем переход к следующей паре
				index++;
			}
			// Выводим собранный текст настроек
			return text;
		}();
		// Выводим эталонный текст настроек с преобладанием чисел
		return result;
	}
	/**
	 * @brief Метод получения эталонного текста с преобладанием перечней
	 *
	 * @return эталонный текст настроек
	 *
	 */
	static inline const std::string & arrays() noexcept {
		// Эталонный текст настроек с преобладанием перечней
		static const std::string result = []() noexcept -> std::string {
			// Собираемый текст настроек
			std::string text;
			// Выполняем резервирование памяти под собираемый текст
			text.reserve(FOCUSED_SIZE + 4096);
			// Выполняем запись объявления таблицы
			text.append("[arrays]\n");
			// Порядковый номер собираемой пары
			uint32_t index = 0;
			/**
			 * Выполняем сборку текста настроек до достижения требуемого размера
			 */
			while(text.length() < FOCUSED_SIZE){
				// Выполняем запись имени ключа очередной пары
				text.append("key").append(number(index)).append(" = ");
				/**
				 * Выполняем выбор построения составного значения очередной пары
				 */
				switch(index % 3){
					// Если значением является перечень чисел
					case 0: text.append("[1, 2, 3, 4, 5, 6, 7, 8]\n"); break;
					// Если значением является перечень вложенных перечней
					case 1: text.append("[[1, 2], [3, 4], [\"пять\", \"шесть\"]]\n"); break;
					// Если значением является встроенная таблица
					case 2: text.append("{ x = 1, y = 2, name = \"точка\", flag = true }\n"); break;
				}
				// Выполняем переход к следующей паре
				index++;
			}
			// Выводим собранный текст настроек
			return text;
		}();
		// Выводим эталонный текст настроек с преобладанием перечней
		return result;
	}
	/**
	 * @brief Метод получения эталонного текста со множеством таблиц
	 *
	 * @return эталонный текст настроек
	 *
	 */
	static inline const std::string & tables() noexcept {
		// Эталонный текст настроек со множеством таблиц
		static const std::string result = []() noexcept -> std::string {
			// Собираемый текст настроек
			std::string text;
			// Выполняем резервирование памяти под собираемый текст
			text.reserve(FOCUSED_SIZE + 4096);
			// Порядковый номер собираемой таблицы
			uint32_t index = 0;
			/**
			 * Выполняем сборку текста настроек до достижения требуемого размера
			 */
			while(text.length() < FOCUSED_SIZE){
				// Выполняем запись объявления очередной таблицы
				text.append("[group").append(number(index)).append(".item").append(number(index)).append("]\n");
				// Выполняем запись первой пары очередной таблицы
				text.append("name = \"элемент ").append(number(index)).append("\"\n");
				// Выполняем запись второй пары очередной таблицы
				text.append("index = ").append(number(index)).append("\n");
				// Выполняем переход к следующей таблице
				index++;
			}
			// Выводим собранный текст настроек
			return text;
		}();
		// Выводим эталонный текст настроек со множеством таблиц
		return result;
	}
	/**
	 * @brief Шаблон типа разбираемого файла настроек
	 *
	 * @tparam Subject тип разбираемого файла настроек
	 *
	 */
	template <typename Subject>
	/**
	 * @brief Метод прогона одного сценария разбора
	 *
	 * @details Первый прогон выполняется до начала замера: он выводит накопители
	 * разбора на рабочий объём, и его стоимость к установившемуся режиму отношения
	 * не имеет
	 *
	 * @param subject функция разбора одного файла настроек
	 * @param text    разбираемый текст настроек
	 * @param rounds  количество прогонов разбора
	 * @param output  ссылка на итоги прогона сценария
	 * @return        признак успешного прогона
	 *
	 */
	static bool parsing(Subject && subject, const std::string & text, const size_t rounds, outcome_t & output) noexcept {
		/**
		 * Если прогрев разбора выполнить не удалось
		 */
		if(!subject(text))
			// Выводим признак неудачного прогона
			return false;
		// Запоминаем время начала замера
		const auto start = std::chrono::steady_clock::now();
		/**
		 * Выполняем прогон разбора заданное количество раз
		 */
		for(size_t i = 0; i < rounds; i++){
			/**
			 * Если разбор файла настроек выполнить не удалось
			 */
			if(!subject(text))
				// Выводим признак неудачного прогона
				return false;
		}
		// Запоминаем время окончания замера
		const auto finish = std::chrono::steady_clock::now();
		// Запоминаем количество разобранных файлов настроек
		output.documents = rounds;
		// Запоминаем количество разобранных октетов
		output.bytes = (text.size() * rounds);
		// Запоминаем длительность замера
		output.seconds = std::chrono::duration <double> (finish - start).count();
		// Выводим признак успешного прогона
		return true;
	}
	/**
	 * @brief Метод получения пропускной способности разбора
	 *
	 * @param outcome итоги прогона сценария
	 * @return        пропускная способность в мегабайтах в секунду
	 *
	 */
	static inline double megabytes(const outcome_t & outcome) noexcept {
		/**
		 * Если замер не состоялся
		 */
		if(outcome.seconds <= 0.0)
			// Выводим нулевую пропускную способность
			return 0.0;
		// Выводим пропускную способность разбора
		return ((static_cast <double> (outcome.bytes) / (1024.0 * 1024.0)) / outcome.seconds);
	}
	/**
	 * @brief Метод вывода результата прогона сценария
	 *
	 * @param name    название сценария
	 * @param outcome итоги прогона сценария
	 *
	 */
	static inline void report(const char * name, const outcome_t & outcome) noexcept {
		// Выводим результат прогона сценария
		::printf("%-24s %12.2f MB/s %10zu files\n", name, megabytes(outcome), outcome.documents);
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
		::printf("%-24s %12s      %s\n", name, "skipped", reason);
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
	 * @brief Структура сценария стенда
	 *
	 */
	struct scenario_t {
		// Название сценария
		const char * name;
		// Количество прогонов разбора
		size_t rounds;
		// Функция получения разбираемого текста настроек
		const std::string & (* text)() noexcept;
		// Функция разбора одного файла настроек
		bool (* subject)(const std::string &) noexcept;
	};

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
				// Выводим контрольную сумму значений и количество пар
				::printf("checksum %llu entries %llu\n", static_cast <unsigned long long> (checksum), static_cast <unsigned long long> (entries));
				// Выходим из перебора параметров запуска
				return;
			}
		}
	}
	/**
	 * @brief Метод прогона всех сценариев стенда
	 *
	 * @param argc      длина массива параметров
	 * @param argv      массив параметров
	 * @param scenarios перечень сценариев стенда
	 * @param count     количество сценариев стенда
	 * @return          код выхода из стенда
	 *
	 */
	static inline int32_t run(const int32_t argc, char ** argv, const scenario_t * scenarios, const size_t count) noexcept {
		// Получаем отбор сценариев по вхождению в название
		const char * filter = rival::filter(argc, argv);
		// Итоги прогона сценария
		outcome_t outcome{0, 0, 0.0};
		// Количество сценариев, отбором выбранных
		size_t chosen = 0;
		// Количество сценариев, прогон каких не удался
		size_t failed = 0;
		/**
		 * Выполняем перебор всех сценариев стенда
		 */
		for(size_t i = 0; i < count; i++){
			/**
			 * Если сценарий отбором не выбран
			 */
			if(!selected(scenarios[i].name, filter))
				// Выполняем переход к следующему сценарию
				continue;
			// Выполняем учёт сценария, отбором выбранного
			chosen++;
			/**
			 * Если прогон сценария выполнить не удалось
			 */
			if(!parsing(scenarios[i].subject, scenarios[i].text(), scenarios[i].rounds, outcome)){
				// Выводим сообщение о пропуске сценария
				skip(scenarios[i].name, "parsing failed");
				// Выполняем учёт сценария, прогон какого не удался
				failed++;
				// Выполняем переход к следующему сценарию
				continue;
			}
			// Выводим результат прогона сценария
			report(scenarios[i].name, outcome);
		}
		/**
		 * Если отбор не выбрал ни одного сценария
		 *
		 * @note Прежде стенд при таком отборе отчитывался нулевою суммою и успешным кодом
		 *       выхода: описка в названии сценария выглядела ровно как исправный прогон
		 *       нулевой работы, и сличать её было не с чем
		 */
		if(chosen == 0){
			// Выводим сообщение о том, что отбор не выбрал ни одного сценария
			::fprintf(stderr, "отбор «%s» не выбрал ни одного сценария из %zu\n",
			 ((filter != nullptr) ? filter : ""), count);
			/**
			 * Выполняем перебор всех сценариев стенда
			 */
			for(size_t i = 0; i < count; i++)
				// Выводим название очередного сценария стенда
				::fprintf(stderr, "  %s\n", scenarios[i].name);
			// Выводим отрицательный код выхода из стенда
			return EXIT_FAILURE;
		}
		// Выводим контрольную сумму работы, выполненной стендом
		digest(argc, argv);
		/**
		 * Если прогон хотя бы одного сценария не удался
		 *
		 * @note Пропуск сценария - это молчаливое отключение работы, и отчитываться о нём
		 *       успехом нельзя: числа прочих сценариев при этом верны, а отчёт целиком - нет
		 */
		if(failed > 0){
			// Выводим сообщение о количестве сценариев, прогон каких не удался
			::fprintf(stderr, "прогон не удался у %zu сценариев из %zu выбранных\n", failed, chosen);
			// Выводим отрицательный код выхода из стенда
			return EXIT_FAILURE;
		}
		// Выводим успешный код выхода из стенда
		return EXIT_SUCCESS;
	}
};

#endif // __AWH_BENCHMARK_RIVAL_TOML__
