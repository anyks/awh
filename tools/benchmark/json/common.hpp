/**
 * @file common.hpp
 * @date 2026-08-14
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
 * @brief Общее окружение эталонных стендов сравнения контейнера JSON —
 *        эталонные документы, параметры нагрузки, разбор параметров запуска и
 *        вывод результатов
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_BENCHMARK_RIVAL_JSON__
#define __AWH_BENCHMARK_RIVAL_JSON__

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
 * @brief Пространство имён эталонных стендов сравнения контейнера JSON
 *
 * @details Эталонные документы и параметры нагрузки обязаны совпадать у всех стендов:
 *          сравниваются реализации разбора, а не разные объёмы работы, поэтому любое
 *          расхождение здесь обесценивает отчёт целиком
 *
 */
namespace rival {
	/**
	 * @brief Размер эталонного крупного документа в октетах
	 *
	 */
	static constexpr size_t LARGE_SIZE = (16 * 1024 * 1024);

	/**
	 * @brief Размер эталонных документов с преобладанием одного вида значений в октетах
	 *
	 */
	static constexpr size_t FOCUSED_SIZE = (8 * 1024 * 1024);

	/**
	 * @brief Количество прогонов сценариев крупного документа
	 *
	 */
	static constexpr size_t LARGE_ROUNDS = 8;

	/**
	 * @brief Количество прогонов сценариев с преобладанием одного вида значений
	 *
	 */
	static constexpr size_t FOCUSED_ROUNDS = 16;

	/**
	 * @brief Количество прогонов сценария малого документа
	 *
	 */
	static constexpr size_t SMALL_ROUNDS = 200000;

	/**
	 * @brief Глубина вложенности эталонного документа с вложенностью
	 *
	 */
	static constexpr uint32_t NESTED_DEPTH = 24;

	/**
	 * @brief Структура итогов прогона сценария
	 *
	 */
	struct outcome_t {
		// Количество разобранных документов
		size_t documents;
		// Количество разобранных октетов
		size_t bytes;
		// Длительность замера в секундах
		double seconds;
	};

	/**
	 * @brief Контрольная сумма содержимого, прочитанного стендом
	 *
	 * @details Складывается из октетов строк и из значений чисел. Совпадение суммы у
	 * всех стендов означает, что они прочитали одно и то же содержимое, а не разный
	 * его объём: разбор с отложенным чтением значений выглядел бы быстрее прочих,
	 * ничего для того не сделав
	 *
	 * @note Оттого сличаются не разборы, а разборы вместе с полным обходом дерева:
	 *       в обиходе разобранный документ читают целиком, а не оставляют лежать
	 *
	 */
	static volatile uint64_t checksum = 0;

	/**
	 * @brief Количество значений, прочитанных стендом
	 *
	 */
	static volatile uint64_t values = 0;

	/**
	 * @brief Метод учёта прочитанной строки в контрольной сумме
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
		// Выполняем учёт прочитанного значения
		values += 1;
	}
	/**
	 * @brief Метод учёта прочитанного числа в контрольной сумме
	 *
	 * @details Число учитывается целой своей частью намеренно: одни реализации
	 * выдают целое число видом с плавающей запятой, другие - целым, и сложение
	 * дробных частей развело бы контрольные суммы там, где работа одна и та же
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
	 * @brief Метод получения эталонного документа обиходного вида
	 *
	 * @details Массив однородных объектов о немногих полях - вид этот выходит из
	 * служб и выгрузок чаще всякого иного
	 *
	 * @return эталонный текст документа
	 *
	 */
	static inline const std::string & objects() noexcept {
		// Эталонный текст документа
		static const std::string result = []() noexcept -> std::string {
			// Собираемый текст документа
			std::string result = "[";
			// Порядковый номер очередного объекта
			uint32_t i = 0;
			/**
			 * Выполняем сборку текста документа заданного размера
			 */
			while(result.size() < LARGE_SIZE){
				/**
				 * Если объект не первый
				 */
				if(i > 0)
					// Выполняем добавление разделителя значений
					result.push_back(',');
				// Выполняем добавление очередного объекта документа
				result.append("{\"id\":").append(number(i))
					.append(",\"name\":\"Товар ").append(number(i))
					.append("\",\"city\":\"Москва\",\"amount\":").append(number(i % 9973))
					.append(".").append(number(i % 100))
					.append(",\"active\":true,\"note\":null}");
				// Выполняем переход к следующему объекту документа
				i++;
			}
			// Выполняем завершение текста документа
			result.push_back(']');
			// Выводим собранный текст документа
			return result;
		}();
		// Выводим эталонный текст документа
		return result;
	}
	/**
	 * @brief Метод получения эталонного документа с преобладанием чисел
	 *
	 * @details Работа здесь приходится на разбор записей чисел, а не на строки и
	 * не на строение документа
	 *
	 * @return эталонный текст документа
	 *
	 */
	static inline const std::string & numbers() noexcept {
		// Эталонный текст документа
		static const std::string result = []() noexcept -> std::string {
			// Собираемый текст документа
			std::string result = "[";
			// Порядковый номер очередного числа
			uint32_t i = 0;
			/**
			 * Выполняем сборку текста документа заданного размера
			 */
			while(result.size() < FOCUSED_SIZE){
				/**
				 * Если число не первое
				 */
				if(i > 0)
					// Выполняем добавление разделителя значений
					result.push_back(',');
				/**
				 * Определяем вид очередного числа документа
				 */
				switch(i % 4){
					// Если числом является целое
					case 0: result.append(number(i % 100000)); break;
					// Если числом является отрицательное целое
					case 1: result.append("-").append(number(i % 4096)); break;
					// Если числом является дробное
					case 2: result.append(number(i % 1000)).append(".").append(number(i % 100)); break;
					// Если числом является дробное с порядком
					case 3: result.append(number(i % 100)).append(".").append(number(i % 10)).append("e").append(number(i % 12)); break;
				}
				// Выполняем переход к следующему числу документа
				i++;
			}
			// Выполняем завершение текста документа
			result.push_back(']');
			// Выводим собранный текст документа
			return result;
		}();
		// Выводим эталонный текст документа
		return result;
	}
	/**
	 * @brief Метод получения эталонного документа с преобладанием строк
	 *
	 * @details Часть строк несёт отменяющие записи и знаки вне US-ASCII: путь этот
	 * у разбора отдельный, и стоимость его иная
	 *
	 * @return эталонный текст документа
	 *
	 */
	static inline const std::string & strings() noexcept {
		// Эталонный текст документа
		static const std::string result = []() noexcept -> std::string {
			// Собираемый текст документа
			std::string result = "[";
			// Порядковый номер очередной строки
			uint32_t i = 0;
			/**
			 * Выполняем сборку текста документа заданного размера
			 */
			while(result.size() < FOCUSED_SIZE){
				/**
				 * Если строка не первая
				 */
				if(i > 0)
					// Выполняем добавление разделителя значений
					result.push_back(',');
				/**
				 * Определяем вид очередной строки документа
				 */
				switch(i % 4){
					// Если строка знаков отмены не несёт
					case 0: result.append("\"Простое значение ").append(number(i)).append("\""); break;
					// Если строка несёт отменяющие записи
					case 1: result.append("\"Значение с \\\"кавычками\\\" и \\\\ знаком отмены ").append(number(i)).append("\""); break;
					// Если строка несёт знаки вне US-ASCII
					case 2: result.append("\"Значение по-русски ").append(number(i)).append(" и по-японски 漢字\""); break;
					// Если строка несёт отменяющие записи кодовых значений
					case 3: result.append("\"\\u041f\\u0440\\u0438\\u0432\\u0435\\u0442 ").append(number(i)).append(" \\ud83d\\ude00\""); break;
				}
				// Выполняем переход к следующей строке документа
				i++;
			}
			// Выполняем завершение текста документа
			result.push_back(']');
			// Выводим собранный текст документа
			return result;
		}();
		// Выводим эталонный текст документа
		return result;
	}
	/**
	 * @brief Метод получения эталонного документа с глубокой вложенностью
	 *
	 * @details Работа здесь приходится на строение документа: обход дерева тут стоит
	 * дороже разбора значений
	 *
	 * @return эталонный текст документа
	 *
	 */
	static inline const std::string & nested() noexcept {
		// Эталонный текст документа
		static const std::string result = []() noexcept -> std::string {
			// Собираемая ветвь документа
			std::string branch = "{\"value\":1}";
			/**
			 * Выполняем сборку ветви документа заданной глубины
			 */
			for(uint32_t i = 0; i < NESTED_DEPTH; i++)
				// Выполняем углубление ветви документа на один уровень
				branch = ("{\"k" + number(i) + "\":[" + branch + ",{\"n\":" + number(i) + "}]}");
			// Собираемый текст документа
			std::string result = "[";
			// Порядковый номер очередной ветви
			uint32_t index = 0;
			/**
			 * Выполняем сборку текста документа заданного размера
			 */
			while(result.size() < FOCUSED_SIZE){
				/**
				 * Если ветвь не первая
				 */
				if(index > 0)
					// Выполняем добавление разделителя значений
					result.push_back(',');
				// Выполняем добавление очередной ветви документа
				result.append(branch);
				// Выполняем переход к следующей ветви документа
				index++;
			}
			// Выполняем завершение текста документа
			result.push_back(']');
			// Выводим собранный текст документа
			return result;
		}();
		// Выводим эталонный текст документа
		return result;
	}
	/**
	 * @brief Метод получения эталонного малого документа
	 *
	 * @details Разбирается он многие тысячи раз подряд, и виден в нём не разбор
	 * содержимого, а стоимость самого обращения к разбору: выделение памяти,
	 * заведение состояния и всё, что реализация делает единожды на документ
	 *
	 * @return эталонный текст документа
	 *
	 */
	static inline const std::string & small() noexcept {
		// Эталонный текст документа
		static const std::string result =
			"{\"id\":17,\"name\":\"Товар\",\"amount\":42.5,\"active\":true,"
			"\"tags\":[\"один\",\"два\"],\"note\":null}";
		// Выводим эталонный текст документа
		return result;
	}
	/**
	 * @brief Метод прогона сценария разбора документа
	 *
	 * @param subject выполняемый разбор документа
	 * @param text    разбираемый текст документа
	 * @param rounds  количество прогонов разбора
	 * @param output  итоги прогона сценария
	 * @return        признак успешности прогона
	 *
	 */
	static inline bool parsing(bool (* subject)(const std::string &), const std::string & text, const size_t rounds, outcome_t & output) noexcept {
		// Запоминаем время начала замера
		const auto start = std::chrono::steady_clock::now();
		/**
		 * Выполняем прогон разбора заданное количество раз
		 */
		for(size_t i = 0; i < rounds; i++){
			/**
			 * Если разбор документа выполнить не удалось
			 */
			if(!subject(text))
				// Выводим признак неудачного прогона
				return false;
		}
		// Запоминаем время окончания замера
		const auto finish = std::chrono::steady_clock::now();
		// Запоминаем количество разобранных документов
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
		::printf("%-24s %12.2f MB/s %10zu docs\n", name, megabytes(outcome), outcome.documents);
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
	 * @brief Структура сценария стенда
	 *
	 */
	struct scenario_t {
		// Название сценария
		const char * name;
		// Количество прогонов разбора
		size_t rounds;
		// Функция получения разбираемого текста документа
		const std::string & (* text)() noexcept;
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
			{"objects", LARGE_ROUNDS,   objects},
			{"numbers", FOCUSED_ROUNDS, numbers},
			{"strings", FOCUSED_ROUNDS, strings},
			{"nested",  FOCUSED_ROUNDS, nested},
			{"small",   SMALL_ROUNDS,   small}
		};
		// Выводим перечень сценариев стенда
		return result;
	}
	/**
	 * @brief Метод прогона всех сценариев стенда
	 *
	 * @param subject выполняемый разбор документа
	 * @param argc    длина массива параметров
	 * @param argv    массив параметров
	 * @return        код выхода из стенда
	 *
	 */
	static inline int32_t drive(bool (* subject)(const std::string &), const int32_t argc, char ** argv) noexcept {
		// Получаем отбор сценариев по вхождению в название
		const char * chosen = filter(argc, argv);
		// Итоги прогона сценария
		outcome_t outcome{0, 0, 0.0};
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
			/**
			 * Если прогон сценария выполнить не удалось
			 */
			if(!parsing(subject, scenario.text(), scenario.rounds, outcome)){
				// Выводим сообщение о пропуске сценария
				skip(scenario.name, "parsing failed");
				// Выполняем переход к следующему сценарию
				continue;
			}
			// Выводим результат прогона сценария
			report(scenario.name, outcome);
		}
		// Выводим контрольную сумму работы, выполненной стендом
		digest(argc, argv);
		// Выводим успешный код выхода из стенда
		return EXIT_SUCCESS;
	}
};

#endif // __AWH_BENCHMARK_RIVAL_JSON__
