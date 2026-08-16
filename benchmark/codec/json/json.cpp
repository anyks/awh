/**
 * @file json.cpp
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
 * @brief Общая часть бенчмарков контейнера JSON — эталонные документы и показатели
 *        прогона сценариев
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл модуля
 */
#include "json.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Внутренние параметры сборки эталонных документов
 *
 */
namespace {
	/**
	 * @brief Размер эталонного крупного документа в октетах
	 *
	 * @details Размер выбран заведомо превосходящим кэш последнего уровня: разбор
	 *          документа, целиком укладывающегося в кэш, показывает скорость работы
	 *          с кэшем, а не установившуюся пропускную способность
	 *
	 */
	static constexpr size_t LARGE_SIZE = (16 * 1024 * 1024);
	/**
	 * @brief Размер эталонных документов с преобладанием одного вида содержимого
	 *
	 */
	static constexpr size_t FOCUSED_SIZE = (4 * 1024 * 1024);
	/**
	 * @brief Глубина вложенности эталонного глубоко вложенного документа
	 *
	 */
	static constexpr uint32_t NESTED_DEPTH = 250;

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
string awh::benchmark::notation::details(const outcome_t & output) noexcept {
	// Собираемые сведения о прогоне
	string result;
	// Выполняем добавление количества разобранных документов
	result.append(::number(static_cast <uint32_t> (output.operations))).append(" док., ");
	// Выполняем добавление количества выделений памяти на один документ
	result.append(::number(static_cast <uint32_t> (perDocument(output) + 0.5))).append(" выд./док., ");
	// Выполняем добавление объёма выделенной памяти на один документ
	result.append(::number(static_cast <uint32_t> (output.operations > 0 ? (output.allocated / output.operations) : 0))).append(" окт./док.");
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
double awh::benchmark::notation::perSecond(const outcome_t & output) noexcept {
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
 * @brief Функция извлечения количества выделений памяти на один документ
 *
 * @param output итоги прогона сценария
 * @return       количество выделений памяти на один документ
 *
 */
double awh::benchmark::notation::perDocument(const outcome_t & output) noexcept {
	/**
	 * Если замер не состоялся
	 */
	if(output.operations == 0)
		// Выводим нулевое количество выделений памяти
		return 0.0;
	// Выводим количество выделений памяти на один документ
	return (static_cast <double> (output.allocations) / static_cast <double> (output.operations));
}
/**
 * @brief Функция извлечения задержки обработки одного документа
 *
 * @param output итоги прогона сценария
 * @return       задержка обработки одного документа в микросекундах
 *
 */
double awh::benchmark::notation::perLatency(const outcome_t & output) noexcept {
	/**
	 * Если замер не состоялся
	 */
	if(output.operations == 0)
		// Выводим нулевую задержку обработки документа
		return 0.0;
	// Выводим задержку обработки одного документа
	return ((output.seconds * 1000000.0) / static_cast <double> (output.operations));
}
/**
 * @brief Функция проверки работоспособности учёта выделений памяти
 *
 * @param output итоги прогона сценария
 * @param result заполняемый результат измерения
 * @return       признак работоспособности учёта
 *
 */
bool awh::benchmark::notation::counted(const outcome_t & output, awh::benchmark::result_t & result) noexcept {
	/**
	 * Если счётчик выделений памяти хоть что-нибудь насчитал
	 */
	if(output.allocations > 0)
		// Выводим признак работоспособности учёта
		return true;
	// Устанавливаем признак негодности измерения
	result.invalid = true;
	// Устанавливаем причину негодности измерения
	result.reason = "счётчик выделений памяти молчит - сборка ведётся без замены оператора"
	                " выделения памяти либо стандартная библиотека подключена отдельной"
	                " библиотекой (MinGW: связывать с ключом -static-libstdc++)";
	// Выводим признак неработоспособности учёта
	return false;
}
/**
 * @brief Функция получения контрольной суммы прогонов
 *
 * @return ссылка на контрольную сумму прогонов
 *
 */
volatile uint64_t & awh::benchmark::notation::checksum() noexcept {
	// Контрольная сумма прогонов
	static volatile uint64_t result = 0;
	// Выводим ссылку на контрольную сумму прогонов
	return result;
}
/**
 * @brief Функция получения эталонного ответа службы
 *
 * @return эталонный текст документа
 *
 */
const string & awh::benchmark::notation::response() noexcept {
	// Эталонный текст документа
	static const string result =
		"{\"status\":\"ok\",\"code\":200,\"request\":\"a1b2c3d4\",\"elapsed\":12.5,"
		"\"user\":{\"id\":9007199254740993,\"name\":\"Иван Петров\",\"active\":true},"
		"\"tags\":[\"новый\",\"срочный\",\"оплачен\"],\"error\":null}";
	// Выводим эталонный текст документа
	return result;
}
/**
 * @brief Функция получения эталонного описания настроек
 *
 * @return эталонный текст документа
 *
 */
const string & awh::benchmark::notation::config() noexcept {
	// Эталонный текст документа
	static const string result = []() noexcept -> string {
		// Собираемый текст документа
		string result = "{\"service\":\"orders\",\"version\":3,\"nodes\":[";
		/**
		 * Выполняем сборку описания узлов службы
		 */
		for(uint32_t i = 0; i < 24; i++){
			/**
			 * Если узел не первый
			 */
			if(i > 0)
				// Выполняем добавление разделителя значений
				result.push_back(',');
			// Выполняем добавление описания очередного узла службы
			result.append("{\"name\":\"узел-").append(::number(i + 1))
			      .append("\",\"host\":\"10.0.0.").append(::number(i + 1))
			      .append("\",\"port\":").append(::number(8080 + i))
			      .append(",\"weight\":").append(::number(i % 5)).append(".5")
			      .append(",\"enabled\":").append((i % 3) != 0 ? "true" : "false")
			      .append(",\"limits\":{\"rps\":").append(::number(1000 + (i * 10)))
			      .append(",\"burst\":").append(::number(100 + i)).append("}}");
		}
		// Выполняем закрытие описания настроек
		result.append("],\"owner\":null}");
		// Выводим собранный текст документа
		return result;
	}();
	// Выводим эталонный текст документа
	return result;
}
/**
 * @brief Функция получения эталонного крупного документа
 *
 * @return эталонный текст документа
 *
 */
const string & awh::benchmark::notation::large() noexcept {
	// Эталонный текст документа
	static const string result = []() noexcept -> string {
		// Собираемый текст документа
		string result = "[";
		// Порядковый номер очередной записи документа
		uint32_t i = 0;
		/**
		 * Выполняем сборку текста документа заданного размера
		 */
		while(result.size() < LARGE_SIZE){
			/**
			 * Если запись не первая
			 */
			if(i > 0)
				// Выполняем добавление разделителя значений
				result.push_back(',');
			// Выполняем добавление очередной записи документа
			result.append("{\"id\":").append(::number(i))
			      .append(",\"name\":\"запись номер ").append(::number(i % 100000))
			      .append("\",\"score\":").append(::number(i % 1000)).append(".25")
			      .append(",\"active\":").append((i % 2) == 0 ? "true" : "false")
			      .append(",\"tags\":[\"альфа\",\"бета\"],\"note\":null}");
			// Выполняем переход к следующей записи документа
			i++;
		}
		// Выполняем закрытие текста документа
		result.push_back(']');
		// Выводим собранный текст документа
		return result;
	}();
	// Выводим эталонный текст документа
	return result;
}
/**
 * @brief Функция получения эталонного документа с преобладанием чисел
 *
 * @return эталонный текст документа
 *
 */
const string & awh::benchmark::notation::numbers() noexcept {
	// Эталонный текст документа
	static const string result = []() noexcept -> string {
		// Собираемый текст документа
		string result = "[";
		// Порядковый номер очередного числа документа
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
			 *
			 * @note Виды чередуются намеренно: разбор определяет самый узкий вмещающий
			 *       вид числа, и документ из одних лишь мелких целых мерил бы один
			 *       единственный путь определения
			 */
			switch(i % 4){
				// Если числом является целое без знака
				case 0: result.append(::number(i % 100000)); break;
				// Если числом является целое со знаком
				case 1: result.append("-").append(::number(i % 4096)); break;
				// Если числом является дробное
				case 2: result.append(::number(i % 1000)).append(".125"); break;
				// Если числом является дробное с порядком
				case 3: result.append(::number(i % 100)).append("e-3"); break;
			}
			// Выполняем переход к следующему числу документа
			i++;
		}
		// Выполняем закрытие текста документа
		result.push_back(']');
		// Выводим собранный текст документа
		return result;
	}();
	// Выводим эталонный текст документа
	return result;
}
/**
 * @brief Функция получения эталонного документа с преобладанием строк
 *
 * @return эталонный текст документа
 *
 */
const string & awh::benchmark::notation::strings() noexcept {
	// Эталонный текст документа
	static const string result = []() noexcept -> string {
		// Собираемый текст документа
		string result = "[";
		// Порядковый номер очередной строки документа
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
			// Выполняем открытие очередной строки документа
			result.push_back('"');
			/**
			 * Определяем вид очередной строки документа
			 *
			 * @note Виды чередуются намеренно: неанглийский текст и отменяющие
			 *       последовательности разбираются путями, отличными от простого
			 *       содержимого, и документ из одних лишь английских слов мерил бы
			 *       единственный из них
			 */
			switch(i % 3){
				// Если строка состоит из английского текста
				case 0: result.append("plain content without any escapes at all "); break;
				// Если строка состоит из неанглийского текста
				case 1: result.append("содержимое строки без единой отмены знаков "); break;
				// Если строка содержит отменяющие последовательности
				case 2: result.append("строка с \\\"кавычками\\\" и\\nпереводом строки "); break;
			}
			// Выполняем добавление номера очередной строки документа
			result.append(::number(i % 100000));
			// Выполняем закрытие очередной строки документа
			result.push_back('"');
			// Выполняем переход к следующей строке документа
			i++;
		}
		// Выполняем закрытие текста документа
		result.push_back(']');
		// Выводим собранный текст документа
		return result;
	}();
	// Выводим эталонный текст документа
	return result;
}
/**
 * @brief Функция получения эталонного глубоко вложенного документа
 *
 * @return эталонный текст документа
 *
 */
const string & awh::benchmark::notation::nested() noexcept {
	// Эталонный текст документа
	static const string result = []() noexcept -> string {
		// Собираемый текст документа
		string result = "[";
		// Порядковый номер очередной записи документа
		uint32_t i = 0;
		/**
		 * Выполняем сборку текста документа заданного размера
		 */
		while(result.size() < FOCUSED_SIZE){
			/**
			 * Если запись не первая
			 */
			if(i > 0)
				// Выполняем добавление разделителя значений
				result.push_back(',');
			/**
			 * Выполняем открытие вложенных вместилищ записи
			 */
			for(uint32_t depth = 0; depth < NESTED_DEPTH; depth++)
				// Выполняем открытие очередного вложенного объекта
				result.append("{\"n\":");
			// Выполняем добавление значения на дне вложенности
			result.append(::number(i % 1000));
			/**
			 * Выполняем закрытие вложенных вместилищ записи
			 */
			for(uint32_t depth = 0; depth < NESTED_DEPTH; depth++)
				// Выполняем закрытие очередного вложенного объекта
				result.push_back('}');
			// Выполняем переход к следующей записи документа
			i++;
		}
		// Выполняем закрытие текста документа
		result.push_back(']');
		// Выводим собранный текст документа
		return result;
	}();
	// Выводим эталонный текст документа
	return result;
}
