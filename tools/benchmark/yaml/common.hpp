/**
 * @file common.hpp
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
 * @brief Общее окружение эталонных стендов сравнения контейнера YAML —
 *        эталонные тексты настроек, параметры нагрузки, учёт выполненной работы,
 *        разбор параметров запуска и вывод результатов
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_BENCHMARK_RIVAL_YAML__
#define __AWH_BENCHMARK_RIVAL_YAML__

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
 * @brief Пространство имён эталонных стендов сравнения контейнера YAML
 *
 * @details Эталонные тексты настроек и параметры нагрузки обязаны совпадать со
 *          сценариями `benchmark/codec/yaml` библиотеки AWH: сравниваются
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
	 * @brief Количество пар в одном разделе крупного файла настроек
	 *
	 */
	static constexpr size_t TABLE_KEYS = 32;

	/**
	 * @brief Количество меток, объявляемых текстом с преобладанием ссылок
	 *
	 */
	static constexpr size_t ANCHOR_COUNT = 64;

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
	 * @brief Количество прогонов сценария мелкого файла настроек деревом
	 *
	 * @note Сборка дерева обходится дороже потокового чтения, и количество прогонов
	 *       у стендов дерева своё: оно обязано совпадать с `benchmark/codec/yaml/document.cpp`
	 *
	 */
	static constexpr size_t TREE_SMALL_ROUNDS = 8000;

	/**
	 * @brief Количество прогонов сценария крупного файла настроек деревом
	 *
	 */
	static constexpr size_t TREE_LARGE_ROUNDS = 4;

	/**
	 * @brief Количество прогонов сценариев одного вида записи деревом
	 *
	 */
	static constexpr size_t TREE_FOCUSED_ROUNDS = 12;

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
	 * @brief Контрольная сумма скалярных значений, прочитанных стендом
	 *
	 * @details Складывается из октетов содержимого скалярных значений в том виде,
	 * в каком реализация их выдаёт. Совпадение суммы у стендов означает, что они
	 * прочитали одни и те же значения, а не разный их объём
	 *
	 * @note Имя пары отображения складывается наравне со значением её: у YAML имя
	 * само является скалярным значением, и отделять его значило бы вводить различие,
	 * какого в разбираемом тексте нет. Оттого стенды потокового чтения, где имя
	 * приходит тем же событием скалярного значения, и стенды дерева, где оно
	 * приходит отдельным полем узла, сличаются одною суммой
	 *
	 * @note Складывается содержимое всех скалярных значений без изъятия, а не одних
	 * лишь строковых, как это сделано у стендов контейнера TOML. Причина в устройстве
	 * YAML: разрешение вида скалярного значения ведётся схемою, а схемы наречий 1.1 и
	 * 1.2 расходятся, и отбор по виду значения сличал бы схемы разрешения вместо
	 * разбора. Содержимое же выдаётся всеми сличаемыми реализациями одинаково -
	 * отрезком исходного текста, ограды лишённым
	 *
	 * @warning Изъятие из правила одно, и оно оговорено: реализация fkYAML содержимого
	 * в исходном виде не хранит вовсе - разрешённое значение выдаётся ею числом либо
	 * логическим значением языка, и вернуть по нему исходную запись нельзя. Стенд её
	 * складывает содержимое одних лишь строковых значений и о том сообщает сам
	 *
	 */
	static volatile uint64_t checksum = 0;

	/**
	 * @brief Количество скалярных значений, обработанных стендом
	 *
	 * @details Показатель этот совпадать обязан у всех стендов без изъятия, включая
	 * и тот, чья контрольная сумма неполна: сколько бы реализация ни расходилась в
	 * представлении значений, число их в тексте настроек одно
	 *
	 */
	static volatile uint64_t entries = 0;

	/**
	 * @brief Метод учёта обработанного скалярного значения
	 *
	 */
	static inline void entry() noexcept {
		// Выполняем учёт обработанного скалярного значения
		entries += 1;
	}
	/**
	 * @brief Метод учёта прочитанного содержимого в контрольной сумме
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
			"title: служба обмена сообщениями\n"
			"version: '1.4.2'\n"
			"started: 1979-05-27T07:32:00Z\n"
			"\n"
			"server:\n"
			"  host: 127.0.0.1\n"
			"  port: 8080\n"
			"  backlog: 512\n"
			"  secure: true\n"
			"  timeout: 30.5\n"
			"  mask: 0xFF00\n"
			"  hosts:\n"
			"    - первый\n"
			"    - второй\n"
			"    - третий\n"
			"  limits:\n"
			"    # предельные величины\n"
			"    connections: 10000\n"
			"    requests: 1000000\n"
			"    payload: 0x100000\n"
			"\n"
			"logging:\n"
			"  level: debug # уровень подробности\n"
			"  path: 'C:\\logs\\service.log'\n"
			"  rotate: true\n"
			"  keep: 14\n"
			"  banner: |\n"
			"    служба обмена сообщениями\n"
			"    версия 1.4.2\n"
			"\n"
			"database:\n"
			"  driver: postgres\n"
			"  dsn: \"host=localhost user=awh dbname=service\"\n"
			"  pool: { size: 16, idle: 4 }\n"
			"\n"
			"endpoint:\n"
			"  - path: /api/v1/messages\n"
			"    methods: [GET, POST]\n"
			"  - path: /api/v1/status\n"
			"    methods: [GET]\n";
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
			// Порядковый номер собираемого раздела
			uint32_t index = 0;
			/**
			 * Выполняем сборку текста настроек до достижения требуемого размера
			 */
			while(text.length() < LARGE_SIZE){
				// Выполняем запись имени очередного раздела
				text.append("table").append(number(index)).append(":\n");
				/**
				 * Выполняем сборку всех пар очередного раздела
				 */
				for(uint32_t i = 0; i < TABLE_KEYS; i++){
					// Выполняем запись имени очередной пары
					text.append("  key").append(number(i)).append(": ");
					/**
					 * Выполняем выбор вида значения очередной пары
					 */
					switch(i % 4){
						// Если значением является последовательность знаков
						case 0: text.append("значение ").append(number(i)).append("\n"); break;
						// Если значением является целое число
						case 1: text.append(number(i * 1000)).append("\n"); break;
						// Если значением является логическое значение
						case 2: text.append((i % 8) == 2 ? "true\n" : "false\n"); break;
						// Если значением является число с плавающей точкой
						case 3: text.append(number(i)).append(".25\n"); break;
					}
				}
				// Выполняем запись пустой строки за разделом
				text.append("\n");
				// Выполняем переход к следующему разделу
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
			// Выполняем запись имени раздела
			text.append("strings:\n");
			// Порядковый номер собираемой пары
			uint32_t index = 0;
			/**
			 * Выполняем сборку текста настроек до достижения требуемого размера
			 */
			while(text.length() < FOCUSED_SIZE){
				// Выполняем запись имени очередной пары
				text.append("  key").append(number(index)).append(": ");
				/**
				 * Выполняем выбор ограды строкового значения очередной пары
				 */
				switch(index % 4){
					// Если значение записано без ограды
					case 0: text.append("обыкновенное значение с пробелами\n"); break;
					// Если значение обнесено двойной оградою
					case 1: text.append("\"путь\\\\к\\tфайлу\\u0041 и \\\"кавычки\\\"\"\n"); break;
					// Если значение обнесено одинарной оградою
					case 2: text.append("'C:\\путь\\без\\отмены\\последовательностей'\n"); break;
					// Если значение обнесено одинарной оградою с удвоенной кавычкой
					case 3: text.append("'кавычка '' внутри ограды'\n"); break;
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
			// Выполняем запись имени раздела
			text.append("numbers:\n");
			// Порядковый номер собираемой пары
			uint32_t index = 0;
			/**
			 * Выполняем сборку текста настроек до достижения требуемого размера
			 */
			while(text.length() < FOCUSED_SIZE){
				// Выполняем запись имени очередной пары
				text.append("  key").append(number(index)).append(": ");
				/**
				 * Выполняем выбор записи числового значения очередной пары
				 */
				switch(index % 5){
					// Если число записано десятичной системой счисления
					case 0: text.append("-").append(number(index)).append("\n"); break;
					// Если число записано шестнадцатеричной системой счисления
					case 1: text.append("0xDEADBEEF\n"); break;
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
			// Выполняем запись имени раздела
			text.append("arrays:\n");
			// Порядковый номер собираемой пары
			uint32_t index = 0;
			/**
			 * Выполняем сборку текста настроек до достижения требуемого размера
			 */
			while(text.length() < FOCUSED_SIZE){
				// Выполняем запись имени очередной пары
				text.append("  key").append(number(index)).append(":");
				/**
				 * Выполняем выбор построения составного значения очередной пары
				 */
				switch(index % 3){
					// Если значением является перечень блочного построения
					case 0: text.append("\n    - 1\n    - 2\n    - 3\n    - 4\n"); break;
					// Если значением является перечень поточного построения
					case 1: text.append(" [1, 2, [3, 4], ['пять', \"шесть\"]]\n"); break;
					// Если значением является отображение поточного построения
					case 2: text.append(" { x: 1, y: 2, name: точка, flag: true }\n"); break;
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
	 * @brief Метод получения эталонного текста с преобладанием блочных значений
	 *
	 * @return эталонный текст настроек
	 *
	 */
	static inline const std::string & blocks() noexcept {
		// Эталонный текст настроек с преобладанием блочных значений
		static const std::string result = []() noexcept -> std::string {
			// Собираемый текст настроек
			std::string text;
			// Выполняем резервирование памяти под собираемый текст
			text.reserve(FOCUSED_SIZE + 4096);
			// Выполняем запись имени раздела
			text.append("blocks:\n");
			// Порядковый номер собираемой пары
			uint32_t index = 0;
			/**
			 * Выполняем сборку текста настроек до достижения требуемого размера
			 */
			while(text.length() < FOCUSED_SIZE){
				// Выполняем запись имени очередной пары
				text.append("  key").append(number(index)).append(": ");
				/**
				 * Выполняем выбор построения блочного значения очередной пары
				 */
				switch(index % 3){
					// Если значение записано с сохранением переводов строк
					case 0: text.append("|\n    первая строка содержимого\n    вторая строка содержимого\n    третья строка содержимого\n"); break;
					// Если значение записано со свёрткой строк пробелом
					case 1: text.append(">\n    первая строка свёртки\n    вторая строка свёртки\n\n    строка за пустою\n"); break;
					// Если значение записано с усечением всех переводов строк
					case 2: text.append("|-\n    строка с усечением\n    вторая строка с усечением\n"); break;
				}
				// Выполняем переход к следующей паре
				index++;
			}
			// Выводим собранный текст настроек
			return text;
		}();
		// Выводим эталонный текст настроек с преобладанием блочных значений
		return result;
	}
	/**
	 * @brief Метод получения эталонного текста с преобладанием меток и ссылок
	 *
	 * @return эталонный текст настроек
	 *
	 */
	static inline const std::string & anchors() noexcept {
		// Эталонный текст настроек с преобладанием меток и ссылок
		static const std::string result = []() noexcept -> std::string {
			// Собираемый текст настроек
			std::string text;
			// Выполняем резервирование памяти под собираемый текст
			text.reserve(FOCUSED_SIZE + 4096);
			// Выполняем запись имени раздела объявленных меток
			text.append("defaults:\n");
			/**
			 * Выполняем объявление всех меток текста
			 */
			for(uint32_t i = 0; i < ANCHOR_COUNT; i++){
				// Выполняем запись имени очередной пары объявления
				text.append("  base").append(number(i)).append(": &base").append(number(i)).append("\n");
				// Выполняем запись первой пары помеченного отображения
				text.append("    host: узел").append(number(i)).append("\n");
				// Выполняем запись второй пары помеченного отображения
				text.append("    port: ").append(number(8000 + i)).append("\n");
				// Выполняем запись третьей пары помеченного отображения
				text.append("    secure: true\n");
			}
			// Выполняем запись имени раздела ссылок
			text.append("nodes:\n");
			// Порядковый номер собираемой ссылки
			uint32_t index = 0;
			/**
			 * Выполняем сборку текста настроек до достижения требуемого размера
			 */
			while(text.length() < FOCUSED_SIZE){
				// Выполняем запись очередной ссылки на объявленную метку
				text.append("  - *base").append(number(index % ANCHOR_COUNT)).append("\n");
				// Выполняем переход к следующей ссылке
				index++;
			}
			// Выводим собранный текст настроек
			return text;
		}();
		// Выводим эталонный текст настроек с преобладанием меток и ссылок
		return result;
	}
	/**
	 * @brief Метод получения эталонного текста с примечаниями и оформлением
	 *
	 * @return эталонный текст настроек
	 *
	 */
	static inline const std::string & decorated() noexcept {
		// Эталонный текст настроек с примечаниями и оформлением
		static const std::string result = []() noexcept -> std::string {
			// Собираемый текст настроек
			std::string text;
			// Выполняем резервирование памяти под собираемый текст
			text.reserve(FOCUSED_SIZE + 4096);
			// Выполняем запись примечания над текстом настроек
			text.append("# настройки, человеком правленные\n");
			// Порядковый номер собираемого раздела
			uint32_t index = 0;
			/**
			 * Выполняем сборку текста настроек до достижения требуемого размера
			 */
			while(text.length() < FOCUSED_SIZE){
				// Выполняем запись примечания над очередным разделом
				text.append("\n# раздел ").append(number(index)).append("\n");
				// Выполняем запись имени очередного раздела
				text.append("section").append(number(index)).append(":\n");
				// Выполняем запись первой пары очередного раздела
				text.append("    host:    'узел ").append(number(index)).append("'   # адрес\n");
				// Выполняем запись примечания над второй парой раздела
				text.append("    # предельные величины\n");
				// Выполняем запись второй пары очередного раздела
				text.append("    port:  ").append(number(8000 + index)).append("\n");
				// Выполняем запись третьей пары очередного раздела
				text.append("    tags:\n");
				// Выполняем запись первого значения перечня третьей пары
				text.append("        - первый     # с примечанием\n");
				// Выполняем запись второго значения перечня третьей пары
				text.append("        - второй\n");
				// Выполняем переход к следующему разделу
				index++;
			}
			// Выводим собранный текст настроек
			return text;
		}();
		// Выводим эталонный текст настроек с примечаниями и оформлением
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
				// Выводим контрольную сумму значений и количество скалярных значений
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
			/**
			 * Если прогон сценария выполнить не удалось
			 */
			if(!parsing(scenarios[i].subject, scenarios[i].text(), scenarios[i].rounds, outcome)){
				// Выводим сообщение о пропуске сценария
				skip(scenarios[i].name, "parsing failed");
				// Выполняем переход к следующему сценарию
				continue;
			}
			// Выводим результат прогона сценария
			report(scenarios[i].name, outcome);
		}
		// Выводим контрольную сумму работы, выполненной стендом
		digest(argc, argv);
		// Выводим успешный код выхода из стенда
		return EXIT_SUCCESS;
	}
};

#endif // __AWH_BENCHMARK_RIVAL_YAML__
