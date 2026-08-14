/**
 * @file common.hpp
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
 * @brief Общее окружение эталонных стендов сравнения контейнера CSV —
 *        эталонные таблицы, параметры нагрузки, разбор параметров запуска и
 *        вывод результатов
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_BENCHMARK_RIVAL_CSV__
#define __AWH_BENCHMARK_RIVAL_CSV__

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
#include <fstream>

/**
 * @brief Пространство имён эталонных стендов сравнения контейнера CSV
 *
 * @details Эталонные таблицы и параметры нагрузки обязаны совпадать у всех стендов:
 *          сравниваются реализации разбора, а не разные объёмы работы, поэтому любое
 *          расхождение здесь обесценивает отчёт целиком
 *
 */
namespace rival {
	/**
	 * @brief Размер эталонных крупных таблиц в октетах
	 *
	 */
	static constexpr size_t LARGE_SIZE = (16 * 1024 * 1024);

	/**
	 * @brief Размер эталонных таблиц с преобладанием одного вида содержимого в октетах
	 *
	 */
	static constexpr size_t FOCUSED_SIZE = (8 * 1024 * 1024);

	/**
	 * @brief Количество столбцов эталонной широкой таблицы
	 *
	 * @note Количество это закреплено числом, а не выбрано: одна из сравниваемых
	 *       реализаций требует знать количество столбцов во время сборки, и без
	 *       закреплённого числа сравнение с нею было бы невозможно
	 */
	static constexpr uint32_t WIDE_COLUMNS = 32;

	/**
	 * @brief Количество прогонов сценариев крупных таблиц
	 *
	 */
	static constexpr size_t LARGE_ROUNDS = 8;

	/**
	 * @brief Количество прогонов сценариев с преобладанием одного вида содержимого
	 *
	 */
	static constexpr size_t FOCUSED_ROUNDS = 16;

	/**
	 * @brief Количество прогонов сценария малой таблицы
	 *
	 */
	static constexpr size_t SMALL_ROUNDS = 200000;

	/**
	 * @brief Структура итогов прогона сценария
	 *
	 */
	struct outcome_t {
		// Количество разобранных таблиц
		size_t documents;
		// Количество разобранных октетов
		size_t bytes;
		// Длительность замера в секундах
		double seconds;
	};

	/**
	 * @brief Контрольная сумма содержимого полей, прочитанного стендом
	 *
	 * @details Складывается из октетов содержимого полей. Совпадение суммы у всех
	 * стендов означает, что они прочитали одно и то же содержимое, а не разный его
	 * объём: без такой сверки стенд, выдающий поля ссылками и содержимого не
	 * читающий, выглядел бы быстрее прочих, ничего для того не сделав
	 *
	 * @note Имена столбцов в сумму не входят намеренно: одни реализации выдают их
	 * отдельно, другие - первой записью, третьи не выдают вовсе. Это разница моделей
	 * разбора, а не разный объём работы
	 *
	 */
	static volatile uint64_t checksum = 0;

	/**
	 * @brief Количество полей, обработанных стендом
	 *
	 */
	static volatile uint64_t fields = 0;

	/**
	 * @brief Количество записей, обработанных стендом
	 *
	 */
	static volatile uint64_t records = 0;

	/**
	 * @brief Метод учёта прочитанного содержимого поля в контрольной сумме
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
		// Выполняем учёт обработанного поля
		fields += 1;
	}
	/**
	 * @brief Метод учёта обработанной записи
	 *
	 */
	static inline void record() noexcept {
		// Выполняем учёт обработанной записи
		records += 1;
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
	 * @brief Метод получения эталонной узкой таблицы
	 *
	 * @details Много записей о немногих столбцах без единой кавычки - таблица
	 * такого вида выходит из выгрузок и журналов чаще всякой иной
	 *
	 * @return эталонный текст таблицы
	 *
	 */
	static inline const std::string & narrow() noexcept {
		// Эталонный текст таблицы
		static const std::string result = []() noexcept -> std::string {
			// Собираемый текст таблицы
			std::string result = "id,name,city,amount,status\r\n";
			// Порядковый номер очередной записи
			uint32_t i = 0;
			/**
			 * Выполняем сборку текста таблицы заданного размера
			 */
			while(result.size() < LARGE_SIZE){
				// Выполняем добавление очередной записи таблицы
				result.append(number(i)).append(",Товар ").append(number(i))
					.append(",Москва,").append(number(i % 9973)).append(".")
					.append(number(i % 100)).append(",active\r\n");
				// Выполняем переход к следующей записи таблицы
				i++;
			}
			// Выводим собранный текст таблицы
			return result;
		}();
		// Выводим эталонный текст таблицы
		return result;
	}
	/**
	 * @brief Метод получения эталонной широкой таблицы
	 *
	 * @details Немного записей о многих столбцах: работа здесь приходится на
	 * разделители, а не на содержимое полей
	 *
	 * @return эталонный текст таблицы
	 *
	 */
	static inline const std::string & wide() noexcept {
		// Эталонный текст таблицы
		static const std::string result = []() noexcept -> std::string {
			// Собираемый текст таблицы
			std::string result;
			/**
			 * Выполняем сборку заголовка таблицы
			 */
			for(uint32_t i = 0; i < WIDE_COLUMNS; i++){
				/**
				 * Если столбец не первый
				 */
				if(i > 0)
					// Выполняем добавление разделителя полей
					result.push_back(',');
				// Выполняем добавление имени очередного столбца
				result.append("col").append(number(i));
			}
			// Выполняем завершение заголовка таблицы
			result.append("\r\n");
			// Порядковый номер очередной записи
			uint32_t index = 0;
			/**
			 * Выполняем сборку текста таблицы заданного размера
			 */
			while(result.size() < LARGE_SIZE){
				/**
				 * Выполняем сборку очередной записи таблицы
				 */
				for(uint32_t i = 0; i < WIDE_COLUMNS; i++){
					/**
					 * Если поле не первое
					 */
					if(i > 0)
						// Выполняем добавление разделителя полей
						result.push_back(',');
					// Выполняем добавление содержимого очередного поля
					result.append(number((index + i) % 100000));
				}
				// Выполняем завершение очередной записи таблицы
				result.append("\r\n");
				// Выполняем переход к следующей записи таблицы
				index++;
			}
			// Выводим собранный текст таблицы
			return result;
		}();
		// Выводим эталонный текст таблицы
		return result;
	}
	/**
	 * @brief Метод получения эталонной таблицы с преобладанием кавычек
	 *
	 * @details Всякое поле заключено в кавычки, а часть их содержит и разделитель, и
	 * удвоенную кавычку: путь этот у разбора отдельный, и стоимость его иная
	 *
	 * @return эталонный текст таблицы
	 *
	 */
	static inline const std::string & quoted() noexcept {
		// Эталонный текст таблицы
		static const std::string result = []() noexcept -> std::string {
			// Собираемый текст таблицы
			std::string result = "id,title,note,city\r\n";
			// Порядковый номер очередной записи
			uint32_t i = 0;
			/**
			 * Выполняем сборку текста таблицы заданного размера
			 */
			while(result.size() < FOCUSED_SIZE){
				// Выполняем добавление очередной записи таблицы
				result.append("\"").append(number(i)).append("\",\"Товар ").append(number(i))
					.append(", особый\",\"Примечание с \"\"кавычками\"\" и запятой, каких в поле хватает\","
					"\"Москва\"\r\n");
				// Выполняем переход к следующей записи таблицы
				i++;
			}
			// Выводим собранный текст таблицы
			return result;
		}();
		// Выводим эталонный текст таблицы
		return result;
	}
	/**
	 * @brief Метод получения эталонной таблицы с многострочными полями
	 *
	 * @details Поля содержат переводы строк внутри кавычек: записи здесь занимают по
	 * нескольку строк, и разбор по строкам на такой таблице разваливается
	 *
	 * @return эталонный текст таблицы
	 *
	 */
	static inline const std::string & multiline() noexcept {
		// Эталонный текст таблицы
		static const std::string result = []() noexcept -> std::string {
			// Собираемый текст таблицы
			std::string result = "id,description,city\r\n";
			// Порядковый номер очередной записи
			uint32_t i = 0;
			/**
			 * Выполняем сборку текста таблицы заданного размера
			 */
			while(result.size() < FOCUSED_SIZE){
				// Выполняем добавление очередной записи таблицы
				result.append(number(i)).append(",\"Описание товара ").append(number(i))
					.append("\r\nвторая строка описания\r\nтретья строка описания\",Москва\r\n");
				// Выполняем переход к следующей записи таблицы
				i++;
			}
			// Выводим собранный текст таблицы
			return result;
		}();
		// Выводим эталонный текст таблицы
		return result;
	}
	/**
	 * @brief Метод получения эталонной малой таблицы
	 *
	 * @details Таблица размером с ответ службы: замер на ней показывает стоимость
	 * заведения самого разбора, а не разбора содержимого
	 *
	 * @return эталонный текст таблицы
	 *
	 */
	static inline const std::string & small() noexcept {
		// Эталонный текст таблицы
		static const std::string result =
			"id,name,city,amount,status\r\n"
			"1,Первый,Москва,100.50,active\r\n"
			"2,Второй,Тверь,200.25,pending\r\n"
			"3,\"Третий, особый\",Клин,300.00,active\r\n";
		// Выводим эталонный текст таблицы
		return result;
	}
	/**
	 * @brief Метод записи эталонной таблицы во временный файл
	 *
	 * @details Часть сравниваемых реализаций разбирает лишь файл, отображённый в
	 * память, и подать им текст напрямую нельзя. Файл записывается однажды на всё
	 * время работы стенда
	 *
	 * @param name имя записываемого временного файла
	 * @param text записываемый текст таблицы
	 * @return     адрес записанного временного файла
	 *
	 */
	static inline std::string temporary(const char * name, const std::string & text) noexcept {
		// Адрес записываемого временного файла
		const std::string result = (std::string("/tmp/rival-csv-") + name + ".csv");
		// Объект записи временного файла
		std::ofstream file(result, std::ios::binary | std::ios::trunc);
		// Выполняем запись текста таблицы во временный файл
		file.write(text.data(), static_cast <std::streamsize> (text.size()));
		// Выполняем закрытие записанного временного файла
		file.close();
		// Выводим адрес записанного временного файла
		return result;
	}
	/**
	 * @brief Шаблон типа разбираемой таблицы
	 *
	 * @tparam Subject тип разбираемой таблицы
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
	 * @param subject функция разбора одной таблицы
	 * @param text    разбираемый текст таблицы
	 * @param rounds  количество прогонов разбора
	 * @param output  ссылка на итоги прогона сценария
	 * @return        признак успешного прогона
	 *
	 */
	static bool parsing(Subject && subject, const std::string & text, const size_t rounds, outcome_t & output) noexcept {
		// Выполняем прогрев разбора вне замера
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
			 * Если разбор таблицы выполнить не удалось
			 */
			if(!subject(text))
				// Выводим признак неудачного прогона
				return false;
		}
		// Запоминаем время окончания замера
		const auto finish = std::chrono::steady_clock::now();
		// Запоминаем количество разобранных таблиц
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
				// Выводим контрольную сумму содержимого, количество полей и записей
				::printf("checksum %llu fields %llu records %llu\n",
					static_cast <unsigned long long> (checksum),
					static_cast <unsigned long long> (fields),
					static_cast <unsigned long long> (records));
				// Выходим из перебора параметров запуска
				return;
			}
		}
	}
};

#endif // __AWH_BENCHMARK_RIVAL_CSV__
