/**
 * @file abc.cpp
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
 * @brief Общее окружение замеров бинарного контейнера ABC — эталонные записи и
 *        средства проведения замера
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл
 */
#include "abc.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Внутренние параметры сборки эталонных записей
 *
 */
namespace {
	/**
	 * @brief Функция извлечения объекта журнала замеров
	 *
	 * @details Журнал гасится: замер меряет работу кодека, а не вывод записей, и
	 *          сценарии отказа портили бы и вывод, и время
	 *
	 * @return объект журнала замеров
	 *
	 */
	const awh::log_t * logger() noexcept {
		// Объект фреймворка замеров
		static awh::fmk_t fmk;
		// Объект журнала замеров
		static awh::log_t log(& fmk);
		// Признак выполненной настройки журнала
		static const bool ready = [](){
			// Выполняем гашение вывода журнала замеров
			log.level(awh::log_t::level_t::NONE);
			// Выводим признак выполненной настройки
			return true;
		}();
		// Снимаем неиспользуемый признак настройки
		(void) ready;
		// Выводим объект журнала замеров
		return & log;
	}
	/**
	 * @brief Размер эталонной крупной записи в октетах
	 *
	 * @details Размер выбран заведомо превосходящим кэш последнего уровня: разбор
	 *          записи, целиком укладывающейся в кэш, показывает скорость работы с
	 *          кэшем, а не установившуюся пропускную способность
	 *
	 */
	static constexpr size_t LARGE_SIZE = (16 * 1024 * 1024);
	/**
	 * @brief Размер эталонных записей с преобладанием одного вида значений в октетах
	 *
	 */
	static constexpr size_t FOCUSED_SIZE = (4 * 1024 * 1024);
	/**
	 * @brief Глубина вложенности эталонной записи с глубокой вложенностью
	 *
	 */
	static constexpr uint32_t NESTED_DEPTH = 24;

	/**
	 * @brief Функция получения десятичной записи числа
	 *
	 * @note Запись выполняется средствами стандартной библиотеки намеренно: эталонные
	 *       записи собираются однократно до замера, и стоимость их сборки в замер не
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
	/**
	 * @brief Функция укладки ветви эталонной записи с глубокой вложенностью
	 *
	 * @param writer сборка бинарной записи
	 * @param depth  оставшаяся глубина вложенности
	 * @return       признак успешности укладки
	 *
	 */
	static bool branch(awh::codec::abc::writer_t & writer, const uint32_t depth) noexcept {
		/**
		 * Если глубина вложенности исчерпана
		 */
		if(depth == 0)
			// Выводим результат укладки листа ветви
			return (writer.mapBegin(static_cast <uint64_t> (1)) && writer.text("value") &&
			 writer.number(static_cast <uint64_t> (1)) && writer.mapEnd());
		// Выполняем получение номера яруса вложенности
		const uint32_t level = (NESTED_DEPTH - depth);
		/**
		 * Если открыть ярус вложенности не удалось
		 */
		if(!(writer.mapBegin(static_cast <uint64_t> (1)) && writer.text("k" + ::number(level)) &&
		     writer.arrayBegin(static_cast <uint64_t> (2))))
			// Выводим признак неудачной укладки
			return false;
		// Если уложить вложенную ветвь не удалось
		if(!::branch(writer, depth - 1))
			// Выводим признак неудачной укладки
			return false;
		// Выводим результат укладки соседа вложенной ветви вместе с закрытием яруса
		return (writer.mapBegin(static_cast <uint64_t> (1)) && writer.text("n") &&
		 writer.number(static_cast <uint64_t> (level)) && writer.mapEnd() &&
		 writer.arrayEnd() && writer.mapEnd());
	}
};

/**
 * @brief Функция формирования сведений о прогоне сценария
 *
 * @param output итоги прогона сценария
 * @return       сведения о прогоне для вывода
 *
 */
string awh::benchmark::binary::details(const outcome_t & output) noexcept {
	// Собираемые сведения о прогоне
	string result;
	// Выполняем добавление количества обработанных записей
	result.append(::number(static_cast <uint32_t> (output.operations))).append(" зап., ");
	// Выполняем добавление количества выделений памяти на одну запись
	result.append(::number(static_cast <uint32_t> (perDocument(output) + 0.5))).append(" выд./зап., ");
	// Выполняем добавление объёма выделенной памяти на одну запись
	result.append(::number(static_cast <uint32_t> (output.operations > 0 ? (output.allocated / output.operations) : 0))).append(" окт./зап.");
	// Выводим собранные сведения о прогоне
	return result;
}
/**
 * @brief Функция извлечения пропускной способности обработки
 *
 * @param output итоги прогона сценария
 * @return       пропускная способность в мегабайтах в секунду
 *
 */
double awh::benchmark::binary::perSecond(const outcome_t & output) noexcept {
	// Если замер не состоялся
	if(output.seconds <= 0.0)
		// Выводим нулевую пропускную способность
		return 0.0;
	// Выводим пропускную способность обработки
	return ((static_cast <double> (output.bytes) / (1024.0 * 1024.0)) / output.seconds);
}
/**
 * @brief Функция извлечения количества выделений памяти на одну запись
 *
 * @param output итоги прогона сценария
 * @return       количество выделений памяти на одну запись
 *
 */
double awh::benchmark::binary::perDocument(const outcome_t & output) noexcept {
	// Если замер не состоялся
	if(output.operations == 0)
		// Выводим нулевое количество выделений памяти
		return 0.0;
	// Выводим количество выделений памяти на одну запись
	return (static_cast <double> (output.allocations) / static_cast <double> (output.operations));
}
/**
 * @brief Функция извлечения задержки обработки одной записи
 *
 * @param output итоги прогона сценария
 * @return       задержка обработки одной записи в микросекундах
 *
 */
double awh::benchmark::binary::perLatency(const outcome_t & output) noexcept {
	// Если замер не состоялся
	if(output.operations == 0)
		// Выводим нулевую задержку обработки
		return 0.0;
	// Выводим задержку обработки одной записи
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
bool awh::benchmark::binary::counted(const outcome_t & output, awh::benchmark::result_t & result) noexcept {
	// Если счётчик выделений памяти хоть что-нибудь насчитал
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
 * @brief Функция проверки того, что сценарий выполнил работу
 *
 * @param output итоги прогона сценария
 * @param result заполняемый результат измерения
 * @return       признак выполненной работы
 *
 */
bool awh::benchmark::binary::worked(const outcome_t & output, awh::benchmark::result_t & result) noexcept {
	// Если сценарий выполнил хоть одну операцию
	if(output.operations > 0)
		// Выводим признак выполненной работы
		return true;
	// Устанавливаем признак негодности измерения
	result.invalid = true;
	// Устанавливаем причину негодности измерения
	result.reason = "сценарий не выполнил ни одной операции";
	// Выводим признак невыполненной работы
	return false;
}
/**
 * @brief Функция получения контрольной суммы прогонов
 *
 * @return ссылка на контрольную сумму прогонов
 *
 */
volatile uint64_t & awh::benchmark::binary::checksum() noexcept {
	// Контрольная сумма прогонов сценариев
	static volatile uint64_t result = 0;
	// Выводим ссылку на контрольную сумму прогонов
	return result;
}
/**
 * @brief Функция получения эталонной записи ответа службы
 *
 * @return эталонная запись ответа службы
 *
 */
const vector <uint8_t> & awh::benchmark::binary::service() noexcept {
	// Эталонная запись ответа службы
	static const vector <uint8_t> result = []() noexcept -> vector <uint8_t> {
		// Сборка бинарной записи
		awh::codec::abc::writer_t writer(::logger());
		// Выполняем укладку ответа службы
		if(!(writer.mapBegin(static_cast <uint64_t> (6)) &&
		     writer.text("active") && writer.boolean(true) &&
		     writer.text("amount") && writer.number(static_cast <double> (42.5)) &&
		     writer.text("id") && writer.number(static_cast <uint64_t> (17)) &&
		     writer.text("name") && writer.text("Товар") &&
		     writer.text("note") && writer.nul() &&
		     writer.text("tags") && writer.arrayBegin(static_cast <uint64_t> (2)) &&
		     writer.text("один") && writer.text("два") && writer.arrayEnd() && writer.mapEnd()))
			// Выводим пустую запись
			return vector <uint8_t> ();
		// Выводим собранную запись
		return writer.record();
	}();
	// Выводим эталонную запись ответа службы
	return result;
}
/**
 * @brief Функция получения эталонной крупной записи
 *
 * @return эталонная крупная запись
 *
 */
const vector <uint8_t> & awh::benchmark::binary::large() noexcept {
	// Эталонная крупная запись
	static const vector <uint8_t> result = []() noexcept -> vector <uint8_t> {
		// Сборка бинарной записи
		awh::codec::abc::writer_t writer(::logger());
		// Выполняем открытие массива однородных отображений неопределённой длины
		if(!writer.arrayBegin())
			// Выводим пустую запись
			return vector <uint8_t> ();
		// Порядковый номер очередного отображения
		uint32_t i = 0;
		/**
		 * Выполняем сборку записи заданного размера
		 */
		while(writer.record().size() < LARGE_SIZE){
			// Выполняем укладку очередного отображения записи
			if(!(writer.mapBegin(static_cast <uint64_t> (6)) &&
			     writer.text("active") && writer.boolean((i % 3) != 0) &&
			     writer.text("amount") && writer.number(static_cast <double> (i % 9973) + 0.25) &&
			     writer.text("city") && writer.text("Москва") &&
			     writer.text("id") && writer.number(static_cast <uint64_t> (i)) &&
			     writer.text("name") && writer.text("Товар " + ::number(i)) &&
			     writer.text("note") && writer.nul() && writer.mapEnd()))
				// Выводим пустую запись
				return vector <uint8_t> ();
			// Выполняем переход к следующему отображению записи
			i++;
		}
		// Если закрыть массив однородных отображений не удалось
		if(!writer.arrayEnd())
			// Выводим пустую запись
			return vector <uint8_t> ();
		// Выводим собранную запись
		return writer.record();
	}();
	// Выводим эталонную крупную запись
	return result;
}
/**
 * @brief Функция получения эталонной записи с преобладанием чисел
 *
 * @return эталонная запись с преобладанием чисел
 *
 */
const vector <uint8_t> & awh::benchmark::binary::numbers() noexcept {
	// Эталонная запись с преобладанием чисел
	static const vector <uint8_t> result = []() noexcept -> vector <uint8_t> {
		// Сборка бинарной записи
		awh::codec::abc::writer_t writer(::logger());
		// Выполняем открытие массива чисел неопределённой длины
		if(!writer.arrayBegin())
			// Выводим пустую запись
			return vector <uint8_t> ();
		// Порядковый номер очередного числа
		uint32_t i = 0;
		// Признак успешности укладки очередного числа
		bool laid = true;
		/**
		 * Выполняем сборку записи заданного размера
		 */
		while(laid && (writer.record().size() < FOCUSED_SIZE)){
			/**
			 * Определяем вид очередного числа записи
			 */
			switch(i % 4){
				// Если числом является целое без знака
				case 0: laid = writer.number(static_cast <uint64_t> (i % 100000)); break;
				// Если числом является целое со знаком
				case 1: laid = writer.number(-static_cast <int64_t> (i % 4096)); break;
				// Если числом является дробное
				case 2: laid = writer.number(static_cast <double> (i % 1000) + 0.5); break;
				// Если числом является крупное целое без знака
				case 3: laid = writer.number(static_cast <uint64_t> (static_cast <uint64_t> (i) * 1000000007ull)); break;
			}
			// Выполняем переход к следующему числу записи
			i++;
		}
		// Если укладка чисел отвечена отказом либо закрыть массив не удалось
		if(!laid || !writer.arrayEnd())
			// Выводим пустую запись
			return vector <uint8_t> ();
		// Выводим собранную запись
		return writer.record();
	}();
	// Выводим эталонную запись с преобладанием чисел
	return result;
}
/**
 * @brief Функция получения эталонной записи с преобладанием строк
 *
 * @return эталонная запись с преобладанием строк
 *
 */
const vector <uint8_t> & awh::benchmark::binary::strings() noexcept {
	// Эталонная запись с преобладанием строк
	static const vector <uint8_t> result = []() noexcept -> vector <uint8_t> {
		// Сборка бинарной записи
		awh::codec::abc::writer_t writer(::logger());
		// Выполняем открытие массива строк неопределённой длины
		if(!writer.arrayBegin())
			// Выводим пустую запись
			return vector <uint8_t> ();
		// Порядковый номер очередной строки
		uint32_t i = 0;
		// Признак успешности укладки очередной строки
		bool laid = true;
		/**
		 * Выполняем сборку записи заданного размера
		 */
		while(laid && (writer.record().size() < FOCUSED_SIZE)){
			/**
			 * Определяем вид очередной строки записи
			 */
			switch(i % 4){
				// Если строка знаков вне US-ASCII не несёт
				case 0: laid = writer.text("Простое значение " + ::number(i)); break;
				// Если строка несёт кавычки и знаки отмены
				case 1: laid = writer.text("Значение с \"кавычками\" и \\ знаком отмены " + ::number(i)); break;
				// Если строка несёт кириллицу и иероглифы
				case 2: laid = writer.text("Значение по-русски " + ::number(i) + " и по-японски 漢字"); break;
				// Если строка коротка
				case 3: laid = writer.text("k" + ::number(i)); break;
			}
			// Выполняем переход к следующей строке записи
			i++;
		}
		// Если укладка строк отвечена отказом либо закрыть массив не удалось
		if(!laid || !writer.arrayEnd())
			// Выводим пустую запись
			return vector <uint8_t> ();
		// Выводим собранную запись
		return writer.record();
	}();
	// Выводим эталонную запись с преобладанием строк
	return result;
}
/**
 * @brief Функция получения эталонной записи с преобладанием двоичных значений
 *
 * @return эталонная запись с преобладанием двоичных значений
 *
 */
const vector <uint8_t> & awh::benchmark::binary::blobs() noexcept {
	// Эталонная запись с преобладанием двоичных значений
	static const vector <uint8_t> result = []() noexcept -> vector <uint8_t> {
		// Сборка бинарной записи
		awh::codec::abc::writer_t writer(::logger());
		// Выполняем открытие массива двоичных значений неопределённой длины
		if(!writer.arrayBegin())
			// Выводим пустую запись
			return vector <uint8_t> ();
		// Порядковый номер очередного двоичного значения
		uint32_t i = 0;
		// Признак успешности укладки очередного двоичного значения
		bool laid = true;
		/**
		 * Выполняем сборку записи заданного размера
		 */
		while(laid && (writer.record().size() < FOCUSED_SIZE)){
			// Выполняем получение размера очередного двоичного значения
			const size_t size = (16 + (i % 48));
			// Собираемое двоичное значение записи
			vector <uint8_t> blob(size, 0);
			/**
			 * Выполняем сборку октетов двоичного значения
			 */
			for(size_t j = 0; j < size; j++)
				// Выполняем установку очередного октета двоичного значения
				blob[j] = static_cast <uint8_t> ((i + (j * 31)) & 0xFF);
			// Выполняем укладку очередного двоичного значения
			laid = writer.blob(blob.data(), blob.size());
			// Выполняем переход к следующему двоичному значению записи
			i++;
		}
		// Если укладка двоичных значений отвечена отказом либо закрыть массив не удалось
		if(!laid || !writer.arrayEnd())
			// Выводим пустую запись
			return vector <uint8_t> ();
		// Выводим собранную запись
		return writer.record();
	}();
	// Выводим эталонную запись с преобладанием двоичных значений
	return result;
}
/**
 * @brief Функция получения эталонной записи с глубокой вложенностью
 *
 * @return эталонная запись с глубокой вложенностью
 *
 */
const vector <uint8_t> & awh::benchmark::binary::nested() noexcept {
	// Эталонная запись с глубокой вложенностью
	static const vector <uint8_t> result = []() noexcept -> vector <uint8_t> {
		// Сборка бинарной записи
		awh::codec::abc::writer_t writer(::logger());
		// Выполняем открытие массива ветвей неопределённой длины
		if(!writer.arrayBegin())
			// Выводим пустую запись
			return vector <uint8_t> ();
		// Признак успешности укладки очередной ветви
		bool laid = true;
		/**
		 * Выполняем сборку записи заданного размера
		 */
		while(laid && (writer.record().size() < FOCUSED_SIZE))
			// Выполняем укладку очередной ветви записи
			laid = ::branch(writer, NESTED_DEPTH);
		// Если укладка ветвей отвечена отказом либо закрыть массив не удалось
		if(!laid || !writer.arrayEnd())
			// Выводим пустую запись
			return vector <uint8_t> ();
		// Выводим собранную запись
		return writer.record();
	}();
	// Выводим эталонную запись с глубокой вложенностью
	return result;
}
