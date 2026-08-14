/**
 * @file csv.cpp
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
 * @brief Реализация общего окружения бенчмарков контейнера CSV — сведения о прогоне,
 *        извлечение показателей и сборка эталонных таблиц всех путей разбора
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
 * @brief Внутренние параметры сборки эталонных таблиц
 *
 */
namespace {
	/**
	 * @brief Размер эталонной крупной таблицы в октетах
	 *
	 * @details Размер выбран заведомо превосходящим кэш последнего уровня: разбор
	 *          текста, целиком укладывающегося в кэш, показывает скорость работы с
	 *          кэшем, а не установившуюся пропускную способность
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
	 */
	static constexpr uint32_t WIDE_COLUMNS = 32;

	/**
	 * @brief Функция получения десятичной записи числа
	 *
	 * @note Запись выполняется средствами стандартной библиотеки намеренно: эталонные
	 *       таблицы собираются однократно до замера, и стоимость их сборки в замер не
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
string awh::benchmark::table::details(const outcome_t & output) noexcept {
	// Собираемые сведения о прогоне
	string result;
	// Выполняем добавление количества разобранных таблиц
	result.append(::number(static_cast <uint32_t> (output.operations))).append(" табл., ");
	// Выполняем добавление количества выделений памяти на одну таблицу
	result.append(::number(static_cast <uint32_t> (perDocument(output) + 0.5))).append(" выд./табл., ");
	// Выполняем добавление объёма выделенной памяти на одну таблицу
	result.append(::number(static_cast <uint32_t> (output.operations > 0 ? (output.allocated / output.operations) : 0))).append(" окт./табл.");
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
double awh::benchmark::table::perSecond(const outcome_t & output) noexcept {
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
 * @brief Функция извлечения количества выделений памяти на одну таблицу
 *
 * @param output итоги прогона сценария
 * @return       количество выделений памяти на одну таблицу
 *
 */
double awh::benchmark::table::perDocument(const outcome_t & output) noexcept {
	/**
	 * Если замер не состоялся
	 */
	if(output.operations == 0)
		// Выводим нулевое количество выделений памяти
		return 0.0;
	// Выводим количество выделений памяти на одну таблицу
	return (static_cast <double> (output.allocations) / static_cast <double> (output.operations));
}
/**
 * @brief Функция извлечения задержки обработки одной таблицы
 *
 * @param output итоги прогона сценария
 * @return       задержка обработки одной таблицы в микросекундах
 *
 */
double awh::benchmark::table::perLatency(const outcome_t & output) noexcept {
	/**
	 * Если замер не состоялся
	 */
	if(output.operations == 0)
		// Выводим нулевую задержку обработки
		return 0.0;
	// Выводим задержку обработки одной таблицы
	return ((output.seconds * 1000000.0) / static_cast <double> (output.operations));
}
/**
 * @brief Функция извлечения объёма выделенной памяти на одну запись таблицы
 *
 * @param output  итоги прогона сценария
 * @param records количество обработанных записей за весь прогон
 * @return        объём выделенной памяти на одну запись в октетах
 *
 */
double awh::benchmark::table::perRecord(const outcome_t & output, const size_t records) noexcept {
	/**
	 * Если замер не состоялся
	 */
	if(records == 0)
		// Выводим нулевой объём выделенной памяти
		return 0.0;
	// Выводим объём выделенной памяти на одну запись
	return (static_cast <double> (output.allocated) / static_cast <double> (records));
}
/**
 * @brief Функция получения контрольной суммы прогонов
 *
 * @return ссылка на контрольную сумму прогонов
 *
 */
volatile uint64_t & awh::benchmark::table::checksum() noexcept {
	// Контрольная сумма прогонов
	static volatile uint64_t result = 0;
	// Выводим ссылку на контрольную сумму прогонов
	return result;
}
/**
 * @brief Функция получения эталонной таблицы ответа службы
 *
 * @return эталонный текст таблицы
 *
 */
const string & awh::benchmark::table::service() noexcept {
	// Эталонный текст таблицы
	static const string result =
		"id,name,city,amount,status\r\n"
		"1,Первый,Москва,100.50,active\r\n"
		"2,Второй,Тверь,200.25,pending\r\n"
		"3,\"Третий, особый\",Клин,300.00,active\r\n"
		"4,Четвёртый,Дубна,400.75,active\r\n"
		"5,\"Пятый \"\"особый\"\"\",Икша,500.00,pending\r\n";
	// Выводим эталонный текст таблицы
	return result;
}
/**
 * @brief Функция получения эталонной крупной таблицы
 *
 * @return эталонный текст таблицы
 *
 */
const string & awh::benchmark::table::large() noexcept {
	// Эталонный текст таблицы
	static const string result = []() noexcept -> string {
		// Собираемый текст таблицы
		string result = "id,name,city,amount,status\r\n";
		// Порядковый номер очередной записи
		uint32_t i = 0;
		/**
		 * Выполняем сборку текста таблицы заданного размера
		 */
		while(result.size() < ::LARGE_SIZE){
			// Выполняем добавление очередной записи таблицы
			result.append(::number(i)).append(",Товар ").append(::number(i))
				.append(",Москва,").append(::number(i % 9973)).append(".")
				.append(::number(i % 100)).append(",active\r\n");
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
 * @brief Функция получения эталонной широкой таблицы
 *
 * @return эталонный текст таблицы
 *
 */
const string & awh::benchmark::table::wide() noexcept {
	// Эталонный текст таблицы
	static const string result = []() noexcept -> string {
		// Собираемый текст таблицы
		string result;
		/**
		 * Выполняем сборку заголовка таблицы
		 */
		for(uint32_t i = 0; i < ::WIDE_COLUMNS; i++){
			/**
			 * Если столбец не первый
			 */
			if(i > 0)
				// Выполняем добавление разделителя полей
				result.push_back(',');
			// Выполняем добавление имени очередного столбца
			result.append("col").append(::number(i));
		}
		// Выполняем завершение заголовка таблицы
		result.append("\r\n");
		// Порядковый номер очередной записи
		uint32_t index = 0;
		/**
		 * Выполняем сборку текста таблицы заданного размера
		 */
		while(result.size() < ::FOCUSED_SIZE){
			/**
			 * Выполняем сборку очередной записи таблицы
			 */
			for(uint32_t i = 0; i < ::WIDE_COLUMNS; i++){
				/**
				 * Если поле не первое
				 */
				if(i > 0)
					// Выполняем добавление разделителя полей
					result.push_back(',');
				// Выполняем добавление содержимого очередного поля
				result.append(::number((index + i) % 100000));
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
 * @brief Функция получения эталонной таблицы с преобладанием кавычек
 *
 * @return эталонный текст таблицы
 *
 */
const string & awh::benchmark::table::quoted() noexcept {
	// Эталонный текст таблицы
	static const string result = []() noexcept -> string {
		// Собираемый текст таблицы
		string result = "id,title,note,city\r\n";
		// Порядковый номер очередной записи
		uint32_t i = 0;
		/**
		 * Выполняем сборку текста таблицы заданного размера
		 */
		while(result.size() < ::FOCUSED_SIZE){
			// Выполняем добавление очередной записи таблицы
			result.append("\"").append(::number(i)).append("\",\"Товар ").append(::number(i))
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
 * @brief Функция получения эталонной таблицы с многострочными полями
 *
 * @return эталонный текст таблицы
 *
 */
const string & awh::benchmark::table::multiline() noexcept {
	// Эталонный текст таблицы
	static const string result = []() noexcept -> string {
		// Собираемый текст таблицы
		string result = "id,description,city\r\n";
		// Порядковый номер очередной записи
		uint32_t i = 0;
		/**
		 * Выполняем сборку текста таблицы заданного размера
		 */
		while(result.size() < ::FOCUSED_SIZE){
			// Выполняем добавление очередной записи таблицы
			result.append(::number(i)).append(",\"Описание товара ").append(::number(i))
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
