/**
 * @file toml.cpp
 * @date 2026-08-12
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
 * @brief Реализация общего окружения бенчмарков контейнера TOML — сборка эталонных
 *        текстов настроек всех путей разбора и извлечение показателей прогона
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы модуля
 */
#include "toml.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Внутренние параметры сборки эталонных текстов настроек
 *
 */
namespace {
	/**
	 * @brief Размер эталонного крупного файла настроек в октетах
	 *
	 * @details Размер выбран заведомо превосходящим кэш последнего уровня: разбор
	 *          текста, целиком укладывающегося в кэш, показывает скорость работы с
	 *          кэшем, а не установившуюся пропускную способность
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
	 * @brief Функция получения десятичной записи числа
	 *
	 * @note Запись выполняется средствами стандартной библиотеки намеренно: эталонные
	 *       тексты собираются однократно до замера, и стоимость их сборки в замер не
	 *       входит
	 *
	 * @param value записываемое число
	 * @return      десятичная запись числа
	 *
	 */
	static string number(const uint32_t value) noexcept {
		// Выводим десятичную запись числа
		return to_string(value);
	}
};

/**
 * @brief Функция формирования сведений о прогоне сценария
 *
 * @param output итоги прогона сценария
 * @return       сведения о прогоне для вывода
 *
 */
string awh::benchmark::settings::details(const outcome_t & output) noexcept {
	// Собираемые сведения о прогоне
	string result;
	// Выполняем добавление количества разобранных файлов настроек
	result.append(::number(static_cast <uint32_t> (output.operations))).append(" файл., ");
	// Выполняем добавление количества выделений памяти на один файл настроек
	result.append(::number(static_cast <uint32_t> (perDocument(output) + 0.5))).append(" выд./файл, ");
	// Выполняем добавление объёма выделенной памяти на один файл настроек
	result.append(::number(static_cast <uint32_t> (output.operations > 0 ? (output.allocated / output.operations) : 0))).append(" окт./файл");
	// Выводим собранные сведения о прогоне
	return result;
}
/**
 * @brief Функция извлечения пропускной способности разбора
 *
 * @param output итоги прогона сценария
 * @return       пропускная способность в мегабайтах в секунду
 *
 */
double awh::benchmark::settings::perSecond(const outcome_t & output) noexcept {
	/**
	 * Если замер не состоялся
	 */
	if(output.seconds <= 0.0)
		// Выводим нулевую пропускную способность
		return 0.0;
	// Выводим пропускную способность разбора
	return ((static_cast <double> (output.bytes) / (1024.0 * 1024.0)) / output.seconds);
}
/**
 * @brief Функция извлечения количества выделений памяти на один файл
 *
 * @param output итоги прогона сценария
 * @return       количество выделений памяти на один файл настроек
 *
 */
double awh::benchmark::settings::perDocument(const outcome_t & output) noexcept {
	/**
	 * Если замер не состоялся
	 */
	if(output.operations == 0)
		// Выводим нулевое количество выделений памяти
		return 0.0;
	// Выводим количество выделений памяти на один файл настроек
	return (static_cast <double> (output.allocations) / static_cast <double> (output.operations));
}
/**
 * @brief Функция извлечения задержки обработки одного файла настроек
 *
 * @param output итоги прогона сценария
 * @return       задержка обработки одного файла в микросекундах
 *
 */
double awh::benchmark::settings::perLatency(const outcome_t & output) noexcept {
	/**
	 * Если замер не состоялся
	 */
	if(output.operations == 0)
		// Выводим нулевую задержку обработки
		return 0.0;
	// Выводим задержку обработки одного файла настроек
	return ((output.seconds * 1000000.0) / static_cast <double> (output.operations));
}
/**
 * @brief Функция получения контрольной суммы прогонов
 *
 * @return ссылка на контрольную сумму прогонов
 *
 */
volatile uint64_t & awh::benchmark::settings::checksum() noexcept {
	// Контрольная сумма прогонов сценариев
	static volatile uint64_t result = 0;
	// Выводим ссылку на контрольную сумму прогонов
	return result;
}
/**
 * @brief Функция получения эталонного файла настроек приложения
 *
 * @return эталонный текст настроек
 *
 */
const string & awh::benchmark::settings::service() noexcept {
	// Эталонный текст настроек приложения
	static const string result =
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
 * @brief Функция получения эталонного крупного файла настроек
 *
 * @return эталонный текст настроек
 *
 */
const string & awh::benchmark::settings::large() noexcept {
	// Эталонный крупный текст настроек
	static const string result = []() noexcept -> string {
		// Собираемый текст настроек
		string text;
		// Выполняем резервирование памяти под собираемый текст
		text.reserve(LARGE_SIZE + 4096);
		// Порядковый номер собираемой таблицы
		uint32_t index = 0;
		/**
		 * Выполняем сборку текста настроек до достижения требуемого размера
		 */
		while(text.length() < LARGE_SIZE){
			// Выполняем запись объявления очередной таблицы
			text.append("[table").append(::number(index)).append("]\n");
			/**
			 * Выполняем сборку всех пар очередной таблицы
			 */
			for(uint32_t i = 0; i < TABLE_KEYS; i++){
				// Выполняем запись имени ключа очередной пары
				text.append("key").append(::number(i)).append(" = ");
				/**
				 * Выполняем выбор вида значения очередной пары
				 */
				switch(i % 4){
					// Если значением является последовательность знаков
					case 0: text.append("\"значение ").append(::number(i)).append("\"\n"); break;
					// Если значением является целое число
					case 1: text.append(::number(i * 1000)).append("\n"); break;
					// Если значением является логическое значение
					case 2: text.append((i % 8) == 2 ? "true\n" : "false\n"); break;
					// Если значением является число с плавающей точкой
					case 3: text.append(::number(i)).append(".25\n"); break;
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
 * @brief Функция получения эталонного текста с преобладанием строк
 *
 * @return эталонный текст настроек
 *
 */
const string & awh::benchmark::settings::strings() noexcept {
	// Эталонный текст настроек с преобладанием строк
	static const string result = []() noexcept -> string {
		// Собираемый текст настроек
		string text;
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
			text.append("key").append(::number(index)).append(" = ");
			/**
			 * Выполняем выбор ограды строкового значения очередной пары
			 */
			switch(index % 3){
				// Если строка записана основной оградой
				case 0: text.append("\"обыкновенное значение с пробелами\"\n"); break;
				/**
				 * Если строка записана основной оградой с управляющими последовательностями
				 *
				 * @note Путь разбора последовательностей проходится лишь такими
				 *       строками: без них он не нагружается вовсе
				 */
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
 * @brief Функция получения эталонного текста с преобладанием чисел
 *
 * @return эталонный текст настроек
 *
 */
const string & awh::benchmark::settings::numbers() noexcept {
	// Эталонный текст настроек с преобладанием чисел
	static const string result = []() noexcept -> string {
		// Собираемый текст настроек
		string text;
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
			text.append("key").append(::number(index)).append(" = ");
			/**
			 * Выполняем выбор записи числового значения очередной пары
			 */
			switch(index % 5){
				// Если число записано десятичной системой счисления
				case 0: text.append("-").append(::number(index)).append("\n"); break;
				// Если число записано шестнадцатеричной системой счисления
				case 1: text.append("0xDEAD_BEEF\n"); break;
				// Если число записано восьмеричной системой счисления
				case 2: text.append("0o755\n"); break;
				/**
				 * Если число записано с плавающей точкой
				 *
				 * @note Разбор такого числа идёт отдельным путём, и стоимость его от
				 *       стоимости разбора целого числа разнится многократно
				 */
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
 * @brief Функция получения эталонного текста с преобладанием перечней
 *
 * @return эталонный текст настроек
 *
 */
const string & awh::benchmark::settings::arrays() noexcept {
	// Эталонный текст настроек с преобладанием перечней
	static const string result = []() noexcept -> string {
		// Собираемый текст настроек
		string text;
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
			text.append("key").append(::number(index)).append(" = ");
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
 * @brief Функция получения эталонного текста со множеством таблиц
 *
 * @return эталонный текст настроек
 *
 */
const string & awh::benchmark::settings::tables() noexcept {
	// Эталонный текст настроек со множеством таблиц
	static const string result = []() noexcept -> string {
		// Собираемый текст настроек
		string text;
		// Выполняем резервирование памяти под собираемый текст
		text.reserve(FOCUSED_SIZE + 4096);
		// Порядковый номер собираемой таблицы
		uint32_t index = 0;
		/**
		 * Выполняем сборку текста настроек до достижения требуемого размера
		 *
		 * @note Пар в таблице всего две: нагружается здесь учёт объявленных имён и
		 *       указатели дерева, а не разбор значений
		 */
		while(text.length() < FOCUSED_SIZE){
			// Выполняем запись объявления очередной таблицы
			text.append("[group").append(::number(index)).append(".item").append(::number(index)).append("]\n");
			// Выполняем запись первой пары очередной таблицы
			text.append("name = \"элемент ").append(::number(index)).append("\"\n");
			// Выполняем запись второй пары очередной таблицы
			text.append("index = ").append(::number(index)).append("\n");
			// Выполняем переход к следующей таблице
			index++;
		}
		// Выводим собранный текст настроек
		return text;
	}();
	// Выводим эталонный текст настроек со множеством таблиц
	return result;
}
