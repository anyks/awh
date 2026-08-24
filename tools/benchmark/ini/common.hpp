/**
 * @file common.hpp
 * @date 2026-08-10
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
 * @brief Общее окружение эталонных стендов сравнения контейнера INI —
 *        эталонные тексты настроек, параметры нагрузки, учёт выполненной работы,
 *        разбор параметров запуска и вывод результатов
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_BENCHMARK_RIVAL_INI__
#define __AWH_BENCHMARK_RIVAL_INI__

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
 * @brief Пространство имён эталонных стендов сравнения контейнера INI
 *
 * @details Эталонные тексты настроек и параметры нагрузки обязаны совпадать со
 *          сценариями `benchmark/codec/ini` библиотеки AWH: сравниваются
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
	 * @brief Количество свойств в одном разделе крупного файла настроек
	 *
	 */
	static constexpr size_t SECTION_KEYS = 32;

	/**
	 * @brief Количество прогонов сценариев малых файлов настроек
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
	 * @brief Контрольная сумма значений свойств, прочитанных стендом
	 *
	 * @details Складывается из октетов значений. Совпадение суммы у всех стендов
	 * означает, что они прочитали одни и те же значения, а не разный их объём
	 *
	 * @note Имена разделов и свойств в сумму не входят намеренно: реализации
	 * выдают их по-разному - именем раздела целиком, парой из раздела и
	 * подраздела, склейкой «раздел:свойство». Это разница моделей разбора, а не
	 * разный объём работы, и складывать её в одну сумму значило бы объявить
	 * расхождением то, что расхождением не является
	 *
	 */
	static volatile uint64_t checksum = 0;

	/**
	 * @brief Количество свойств, обработанных стендом
	 *
	 */
	static volatile uint64_t entries = 0;

	/**
	 * @brief Приёмник имён разделов и свойств
	 *
	 * @details Имена в контрольную сумму не входят, однако читать их стенд обязан:
	 * без чтения часть реализаций работу по их выдаче попросту не выполнит, и
	 * сравнение потеряет смысл
	 *
	 */
	static volatile uint64_t sink = 0;

	/**
	 * @brief Метод чтения имени раздела либо свойства
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
	 * @brief Метод учёта обработанного свойства
	 *
	 */
	static inline void entry() noexcept {
		// Выполняем учёт обработанного свойства
		entries += 1;
	}
	/**
	 * @brief Метод учёта прочитанного значения в контрольной сумме
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
	 * @return эталонный текст настроек
	 *
	 */
	static inline const std::string & service() noexcept {
		// Эталонный текст настроек
		static const std::string result =
			"; настройки службы, правились вручную\n"
			"\n"
			"[server]\n"
			"host = 127.0.0.1\n"
			"port = 8080\n"
			"workers = 4\n"
			"backlog = 512\n"
			"timeout = 30\n"
			"\n"
			"; пути размещения\n"
			"[paths]\n"
			"root = /opt/awh\n"
			"logs = /var/log/awh\n"
			"cache = /var/cache/awh\n"
			"pidfile = /var/run/awh.pid\n"
			"\n"
			"[logging]\n"
			"level = debug\n"
			"rotate = daily\n"
			"keep = 14\n"
			"\n"
			"[security]\n"
			"tls = on\n"
			"certificate = /etc/awh/server.pem\n"
			"ciphers = ECDHE-ECDSA-AES256-GCM-SHA384\n";
		// Выводим эталонный текст настроек
		return result;
	}
	/**
	 * @brief Метод получения эталонного файла настроек по образцу Git
	 *
	 * @note Правильно прочесть этот текст умеет лишь та реализация, что знает
	 *       наречие настроек Git: прочие возьмут имя раздела «remote "origin"»
	 *       целиком, не разобрав его на раздел и подраздел, и оставят кавычки в
	 *       значении. Объём работы при этом у всех одинаков - разбирается один и
	 *       тот же текст, - и для сравнения пропускной способности того довольно
	 *
	 * @return эталонный текст настроек
	 *
	 */
	static inline const std::string & repository() noexcept {
		// Эталонный текст настроек
		static const std::string result =
			"[core]\n"
			"\trepositoryformatversion = 0\n"
			"\tfilemode = true\n"
			"\tbare = false\n"
			"\tlogallrefupdates = true\n"
			"\teditor = \"vim -f\" ; правится вручную\n"
			"[remote \"origin\"]\n"
			"\turl = git@example.com:anyks/awh.git\n"
			"\tfetch = +refs/heads/*:refs/remotes/origin/*\n"
			"[branch \"main\"]\n"
			"\tremote = origin\n"
			"\tmerge = refs/heads/main\n"
			"[branch \"v5\"]\n"
			"\tremote = origin\n"
			"\tmerge = refs/heads/v5\n"
			"[alias]\n"
			"\tlg = log --graph --pretty=format:\"%h\\t%s\"\n"
			"[user]\n"
			"\tname = ANYKS\n"
			"\temail = info@anyks.com\n";
		// Выводим эталонный текст настроек
		return result;
	}
	/**
	 * @brief Метод получения эталонного крупного файла настроек
	 *
	 * @return эталонный текст настроек
	 *
	 */
	static inline const std::string & large() noexcept {
		// Эталонный текст настроек
		static const std::string result = []() noexcept -> std::string {
			// Собираемый текст настроек
			std::string result;
			// Выполняем упреждающее выделение памяти под собираемый текст
			result.reserve(LARGE_SIZE + 1024);
			// Порядковый номер собираемого раздела
			uint32_t index = 0;
			/**
			 * Выполняем сборку разделов до достижения требуемого размера
			 */
			while(result.size() < LARGE_SIZE){
				// Выполняем запись объявления очередного раздела
				result.append("[section").append(number(index++)).append("]\n");
				/**
				 * Выполняем запись свойств очередного раздела
				 */
				for(size_t i = 0; i < SECTION_KEYS; i++)
					// Выполняем запись очередного свойства раздела
					result.append("key").append(number(static_cast <uint32_t> (i)))
					      .append(" = значение свойства номер ").append(number(static_cast <uint32_t> (i))).append("\n");
				// Выполняем запись пустой строки за разделом
				result.append("\n");
			}
			// Выводим собранный текст настроек
			return result;
		}();
		// Выводим эталонный текст настроек
		return result;
	}
	/**
	 * @brief Метод получения эталонного текста с преобладанием примечаний
	 *
	 * @return эталонный текст настроек
	 *
	 */
	static inline const std::string & annotated() noexcept {
		// Эталонный текст настроек
		static const std::string result = []() noexcept -> std::string {
			// Собираемый текст настроек
			std::string result("[annotated]\n");
			// Выполняем упреждающее выделение памяти под собираемый текст
			result.reserve(FOCUSED_SIZE + 1024);
			// Порядковый номер собираемой записи
			uint32_t index = 0;
			/**
			 * Выполняем сборку записей до достижения требуемого размера
			 */
			while(result.size() < FOCUSED_SIZE){
				// Выполняем запись примечания отдельной строкой
				result.append("; примечание к свойству номер ").append(number(index)).append("\n");
				// Выполняем запись примечания отдельной строкой
				result.append("# и второе примечание к нему же\n");
				// Выполняем запись свойства со значением
				result.append("key").append(number(index++)).append(" = значение\n");
			}
			// Выводим собранный текст настроек
			return result;
		}();
		// Выводим эталонный текст настроек
		return result;
	}
	/**
	 * @brief Метод получения эталонного текста со множеством разделов
	 *
	 * @return эталонный текст настроек
	 *
	 */
	static inline const std::string & sections() noexcept {
		// Эталонный текст настроек
		static const std::string result = []() noexcept -> std::string {
			// Собираемый текст настроек
			std::string result;
			// Выполняем упреждающее выделение памяти под собираемый текст
			result.reserve(FOCUSED_SIZE + 1024);
			// Порядковый номер собираемого раздела
			uint32_t index = 0;
			/**
			 * Выполняем сборку разделов до достижения требуемого размера
			 */
			while(result.size() < FOCUSED_SIZE){
				// Получаем десятичную запись порядкового номера раздела
				const std::string value = number(index++);
				// Выполняем запись объявления очередного раздела
				result.append("[раздел.").append(value).append("]\n");
				// Выполняем запись единственного свойства раздела
				result.append("key = ").append(value).append("\n");
			}
			// Выводим собранный текст настроек
			return result;
		}();
		// Выводим эталонный текст настроек
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
				// Выводим контрольную сумму значений и количество свойств
				::printf("checksum %llu entries %llu\n", static_cast <unsigned long long> (checksum), static_cast <unsigned long long> (entries));
				// Выходим из перебора параметров запуска
				return;
			}
		}
	}
	/**
	 * @brief Метод прогона всех сценариев стенда
	 *
	 * @details Перебор сценариев вынесен в общее окружение, а не повторён в каждом
	 * стенде: стендов здесь семеро, и перебор у всех до знака одинаков. У стендов
	 * сравнения контейнера XML он повторён в каждом - там их пятеро и разошлись они
	 * заметнее
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

#endif // __AWH_BENCHMARK_RIVAL_INI__
