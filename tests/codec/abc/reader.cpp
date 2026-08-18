/**
 * @file reader.cpp
 * @date 2026-08-18
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Проверки поточного чтения бинарного контейнера ABC — выдача событий, стек
 *        вместимых, независимость выдачи от нарезки записи на куски и коды отказов
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <string>
#include <cstdint>
#include <limits>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <gtest/gtest.h>
#include <codec/abc/abc.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;
using namespace awh::codec;

/**
 * @brief Пространство имён работ, доступных лишь этому файлу
 *
 */
namespace {
	/**
	 * @brief Функция получения краткой записи события разбора
	 *
	 * @param reader читатель бинарной записи
	 * @return       краткая запись события разбора
	 *
	 */
	string describe(const abc::reader_t & reader) noexcept {
		// Выполняем получение значения текущего события
		const abc::reader_t::value_t value = reader.value();
		/**
		 * Определяем вид текущего события разбора
		 */
		switch(static_cast <uint8_t> (reader.event())){
			// Если событием является пустое значение
			case static_cast <uint8_t> (abc::event_t::NUL): return "null";
			// Если событием является логическое значение
			case static_cast <uint8_t> (abc::event_t::BOOL): return (value.boolean ? "true" : "false");
			// Если событием является начало массива
			case static_cast <uint8_t> (abc::event_t::ARRAY_BEGIN): return "[";
			// Если событием является конец массива
			case static_cast <uint8_t> (abc::event_t::ARRAY_END): return "]";
			// Если событием является начало отображения
			case static_cast <uint8_t> (abc::event_t::MAP_BEGIN): return "{";
			// Если событием является конец отображения
			case static_cast <uint8_t> (abc::event_t::MAP_END): return "}";
			// Если событием является завершённый документ
			case static_cast <uint8_t> (abc::event_t::DOCUMENT): return ";";
			// Если событием является окончание записи
			case static_cast <uint8_t> (abc::event_t::FINISH): return ".";
			// Если событием является отметка времени
			case static_cast <uint8_t> (abc::event_t::TIME): return ("T" + to_string(value.integer));
			// Если событием является опознаватель
			case static_cast <uint8_t> (abc::event_t::UUID): return ("U" + to_string(value.data.size()));
			// Если событием является имя поля отображения
			case static_cast <uint8_t> (abc::event_t::KEY): {
				// Если именем поля является строка
				if(value.type == abc::type_t::STRING)
					// Выводим имя поля строкою
					return ("K:" + string(value.data));
				// Выводим имя поля числом
				return ("K#" + to_string(value.number));
			}
			// Если событием является строка
			case static_cast <uint8_t> (abc::event_t::STRING): return ("S:" + string(value.data));
			// Если событием являются двоичные данные
			case static_cast <uint8_t> (abc::event_t::BLOB): return ("B" + to_string(value.data.size()));
			// Если событием является число
			case static_cast <uint8_t> (abc::event_t::NUMBER): {
				// Если число является целым без знака
				if(static_cast <uint32_t> (value.type) & static_cast <uint32_t> (abc::type_t::UNSIGNED))
					// Выводим целое без знака
					return ("N" + to_string(value.number));
				// Если число является целым со знаком
				else if(static_cast <uint32_t> (value.type) & static_cast <uint32_t> (abc::type_t::SIGNED))
					// Выводим целое со знаком
					return ("N" + to_string(value.integer));
				// Если число является дробным
				else if(static_cast <uint32_t> (value.type) & static_cast <uint32_t> (abc::type_t::REAL))
					// Выводим дробное число
					return ("R" + to_string(value.real));
				// Если число является десятичным
				else if(value.type == abc::type_t::DECIMAL)
					// Выводим десятичное число
					return ("D" + string(value.negative ? "-" : "+") + to_string(value.data.size()) +
					 "e" + to_string(value.exponent));
				// Выводим целое неограниченной ширины
				return ("X" + string(value.negative ? "-" : "+") + to_string(value.data.size()));
			}
		}
		// Выводим признак неопознанного события
		return "?";
	}
	/**
	 * @brief Функция сбора всех событий разбора поданной записи
	 *
	 * @param data   разбираемая запись
	 * @param chunk  размер куска подачи, ноль - подача целиком
	 * @param reader читатель бинарной записи
	 * @return       собранная последовательность событий разбора
	 *
	 */
	vector <string> collect(const vector <uint8_t> & data, const size_t chunk, abc::reader_t & reader) noexcept {
		// Собираемая последовательность событий разбора
		vector <string> result;
		// Выполняем установку размера куска подачи
		const size_t step = ((chunk > 0) ? chunk : (data.empty() ? 1 : data.size()));
		// Смещение подаваемого куска записи
		size_t offset = 0;
		/**
		 * Выполняем подачу записи кусками установленного размера
		 */
		while(offset < data.size()){
			// Выполняем получение размера подаваемого куска
			const size_t size = (((data.size() - offset) < step) ? (data.size() - offset) : step);
			// Признак того, что подаваемый кусок последний
			const bool last = ((offset + size) >= data.size());
			// Если подача куска записи отвечена отказом
			if(!reader.feed(data.data() + offset, size, last))
				// Выводим собранную последовательность событий
				return result;
			/**
			 * Выполняем снятие всех собранных событий разбора
			 */
			while(reader.next())
				// Выполняем добавление краткой записи события
				result.push_back(describe(reader));
			// Выполняем сдвиг смещения подаваемого куска
			offset += size;
		}
		// Если запись пуста
		if(data.empty()){
			// Выполняем подачу пустой записи
			if(!reader.feed(nullptr, 0, true))
				// Выводим собранную последовательность событий
				return result;
			/**
			 * Выполняем снятие всех собранных событий разбора
			 */
			while(reader.next())
				// Выполняем добавление краткой записи события
				result.push_back(describe(reader));
		}
		// Выводим собранную последовательность событий
		return result;
	}
	/**
	 * @brief Функция укладки строки в запись
	 *
	 * @param result буфер собираемой записи
	 * @param text   укладываемая строка
	 *
	 */
	void text(vector <uint8_t> & result, const string & text) noexcept {
		// Выполняем укладку метки строки вместе с её длиной
		abc::put(result, abc::major_t::STRING, static_cast <uint64_t> (text.size()));
		// Выполняем укладку октетов строки
		result.insert(result.end(), text.begin(), text.end());
	}
};

/**
 * @brief Проверка разбора одиночных значений
 *
 */
TEST(CodecAbcReader, Scalars) {
	// Буфер собираемой записи
	vector <uint8_t> data;
	// Выполняем укладку пустого значения
	abc::mark(data, abc::major_t::SINGLE, static_cast <uint8_t> (abc::single_t::NUL));
	// Читатель бинарной записи
	abc::reader_t reader;
	// Выполняем сбор всех событий разбора
	vector <string> events = collect(data, 0, reader);
	// Выполняем проверку собранной последовательности событий
	ASSERT_EQ(events, (vector <string> {"null", ";", "."})) << "код отказа: " << abc::message(reader.error());
	// Выполняем очистку буфера собираемой записи
	data.clear();
	// Выполняем сброс состояния разбора
	reader.reset();
	// Выполняем укладку логического значения
	abc::mark(data, abc::major_t::SINGLE, static_cast <uint8_t> (abc::single_t::TRUE));
	// Выполняем сбор всех событий разбора
	events = collect(data, 0, reader);
	// Выполняем проверку собранной последовательности событий
	ASSERT_EQ(events, (vector <string> {"true", ";", "."})) << "код отказа: " << abc::message(reader.error());
	// Выполняем очистку буфера собираемой записи
	data.clear();
	// Выполняем сброс состояния разбора
	reader.reset();
	// Выполняем укладку целого числа со знаком
	abc::integer(data, -1000);
	// Выполняем сбор всех событий разбора
	events = collect(data, 0, reader);
	// Выполняем проверку собранной последовательности событий
	ASSERT_EQ(events, (vector <string> {"N-1000", ";", "."})) << "код отказа: " << abc::message(reader.error());
	// Выполняем очистку буфера собираемой записи
	data.clear();
	// Выполняем сброс состояния разбора
	reader.reset();
	// Выполняем укладку строки
	text(data, "привет");
	// Выполняем сбор всех событий разбора
	events = collect(data, 0, reader);
	// Выполняем проверку собранной последовательности событий
	ASSERT_EQ(events, (vector <string> {"S:привет", ";", "."})) << "код отказа: " << abc::message(reader.error());
}
/**
 * @brief Проверка самого узкого вида, вмещающего число
 *
 * @details Разбор определяет вид сразу и повторного разбора записи не требует
 *
 */
TEST(CodecAbcReader, NarrowType) {
	/**
	 * Проверяемые значения вместе с ожидаемым видом
	 */
	const vector <pair <int64_t, abc::type_t>> values = {
		{0, abc::type_t::UINT8}, {255, abc::type_t::UINT8}, {256, abc::type_t::UINT16},
		{65536, abc::type_t::UINT32}, {-1, abc::type_t::INT8}, {-128, abc::type_t::INT8},
		{-129, abc::type_t::INT16}, {-32769, abc::type_t::INT32}
	};
	/**
	 * Выполняем перебор всех проверяемых значений
	 */
	for(const auto & item : values){
		// Буфер собираемой записи
		vector <uint8_t> data;
		// Читатель бинарной записи
		abc::reader_t reader;
		// Выполняем укладку целого числа со знаком
		abc::integer(data, item.first);
		// Выполняем подачу записи целиком
		ASSERT_TRUE(reader.feed(data.data(), data.size(), true)) << "значение: " << item.first;
		// Выполняем переход к первому собранному событию
		ASSERT_TRUE(reader.next()) << "значение: " << item.first;
		// Выполняем проверку вида значения
		ASSERT_EQ(reader.value().type, item.second) << "значение: " << item.first
			<< ", выдан вид: " << abc::name(reader.value().type);
	}
}
/**
 * @brief Проверка разбора вместимых определённой длины
 *
 */
TEST(CodecAbcReader, Containers) {
	// Буфер собираемой записи
	vector <uint8_t> data;
	// Выполняем укладку массива из трёх значений
	abc::put(data, abc::major_t::ARRAY, 3);
	// Выполняем укладку первого значения массива
	abc::put(data, abc::major_t::UNSIGNED, 1);
	// Выполняем укладку вложенного массива из двух значений
	abc::put(data, abc::major_t::ARRAY, 2);
	// Выполняем укладку первого значения вложенного массива
	abc::put(data, abc::major_t::UNSIGNED, 2);
	// Выполняем укладку второго значения вложенного массива
	abc::put(data, abc::major_t::UNSIGNED, 3);
	// Выполняем укладку отображения из одной пары
	abc::put(data, abc::major_t::MAP, 1);
	// Выполняем укладку имени поля отображения
	text(data, "имя");
	// Выполняем укладку значения поля отображения
	abc::mark(data, abc::major_t::SINGLE, static_cast <uint8_t> (abc::single_t::NUL));
	// Читатель бинарной записи
	abc::reader_t reader;
	// Выполняем сбор всех событий разбора
	const vector <string> events = collect(data, 0, reader);
	// Выполняем проверку собранной последовательности событий
	ASSERT_EQ(events, (vector <string> {"[", "N1", "[", "N2", "N3", "]", "{", "K:имя", "null", "}", "]", ";", "."}))
		<< "код отказа: " << abc::message(reader.error());
}
/**
 * @brief Проверка разбора пустых вместимых
 *
 * @details Вместимое без значений закрывается тем же ведущим октетом, каким открыто
 *
 */
TEST(CodecAbcReader, EmptyContainers) {
	// Буфер собираемой записи
	vector <uint8_t> data;
	// Выполняем укладку массива из двух значений
	abc::put(data, abc::major_t::ARRAY, 2);
	// Выполняем укладку пустого массива
	abc::put(data, abc::major_t::ARRAY, 0);
	// Выполняем укладку пустого отображения
	abc::put(data, abc::major_t::MAP, 0);
	// Читатель бинарной записи
	abc::reader_t reader;
	// Выполняем сбор всех событий разбора
	const vector <string> events = collect(data, 0, reader);
	// Выполняем проверку собранной последовательности событий
	ASSERT_EQ(events, (vector <string> {"[", "[", "]", "{", "}", "]", ";", "."}))
		<< "код отказа: " << abc::message(reader.error());
}
/**
 * @brief Проверка разбора вместимых неопределённой длины
 *
 */
TEST(CodecAbcReader, IndefiniteContainers) {
	// Буфер собираемой записи
	vector <uint8_t> data;
	// Выполняем укладку отображения неопределённой длины
	abc::mark(data, abc::major_t::MAP, static_cast <uint8_t> (abc::single_t::BREAK));
	// Выполняем укладку имени поля отображения числом
	abc::put(data, abc::major_t::UNSIGNED, 7);
	// Выполняем укладку массива неопределённой длины
	abc::mark(data, abc::major_t::ARRAY, static_cast <uint8_t> (abc::single_t::BREAK));
	// Выполняем укладку значения массива
	abc::put(data, abc::major_t::UNSIGNED, 8);
	// Выполняем укладку конца массива
	abc::mark(data, abc::major_t::SINGLE, static_cast <uint8_t> (abc::single_t::BREAK));
	// Выполняем укладку конца отображения
	abc::mark(data, abc::major_t::SINGLE, static_cast <uint8_t> (abc::single_t::BREAK));
	// Читатель бинарной записи
	abc::reader_t reader;
	// Выполняем сбор всех событий разбора
	const vector <string> events = collect(data, 0, reader);
	// Выполняем проверку собранной последовательности событий
	ASSERT_EQ(events, (vector <string> {"{", "K#7", "[", "N8", "]", "}", ";", "."}))
		<< "код отказа: " << abc::message(reader.error());
}
/**
 * @brief Проверка независимости выдачи событий от нарезки записи на куски
 *
 * @details Выдача, зависящая от того, как запись нарезана, обратила бы поточное чтение
 * в негодное: подающий сторона размера куска не выбирает
 *
 */
TEST(CodecAbcReader, ChunkIndependence) {
	// Буфер собираемой записи
	vector <uint8_t> data;
	// Выполняем укладку отображения из трёх пар
	abc::put(data, abc::major_t::MAP, 3);
	// Выполняем укладку имени первого поля отображения
	text(data, "первое");
	// Выполняем укладку значения первого поля отображения
	abc::put(data, abc::major_t::UNSIGNED, 300000);
	// Выполняем укладку имени второго поля отображения
	text(data, "второе");
	// Выполняем укладку дробного значения двойной точности
	abc::real(data, 1.5);
	// Выполняем укладку имени третьего поля отображения
	text(data, "третье");
	// Выполняем укладку массива из двух значений
	abc::put(data, abc::major_t::ARRAY, 2);
	// Выполняем укладку двоичного значения
	abc::put(data, abc::major_t::BLOB, 4);
	// Выполняем укладку октетов двоичного значения
	data.insert(data.end(), {0x01, 0x02, 0x03, 0x04});
	// Выполняем укладку опознавателя
	abc::mark(data, abc::major_t::SINGLE, static_cast <uint8_t> (abc::single_t::UUID));
	/**
	 * Выполняем укладку октетов опознавателя
	 */
	for(uint8_t i = 0; i < 16; i++)
		// Выполняем укладку очередного октета опознавателя
		data.push_back(i);
	// Читатель бинарной записи, подающейся целиком
	abc::reader_t whole;
	// Выполняем сбор всех событий разбора записи, поданной целиком
	const vector <string> expected = collect(data, 0, whole);
	// Выполняем проверку собранной последовательности событий
	ASSERT_EQ(expected, (vector <string> {"{", "K:первое", "N300000", "K:второе", "R1.500000",
	 "K:третье", "[", "B4", "U16", "]", "}", ";", "."})) << "код отказа: " << abc::message(whole.error());
	/**
	 * Выполняем перебор всех размеров куска подачи
	 */
	for(size_t chunk = 1; chunk <= data.size(); chunk++){
		// Читатель бинарной записи, подающейся кусками
		abc::reader_t reader;
		// Выполняем сбор всех событий разбора записи, поданной кусками
		const vector <string> events = collect(data, chunk, reader);
		// Выполняем проверку совпадения выдачи с выдачей записи, поданной целиком
		ASSERT_EQ(events, expected) << "размер куска: " << chunk
			<< ", код отказа: " << abc::message(reader.error());
	}
}
/**
 * @brief Проверка разбора потока документов
 *
 */
TEST(CodecAbcReader, DocumentStream) {
	// Буфер собираемой записи
	vector <uint8_t> data;
	// Выполняем укладку первого документа
	abc::put(data, abc::major_t::UNSIGNED, 1);
	// Выполняем укладку второго документа
	abc::put(data, abc::major_t::UNSIGNED, 2);
	// Выполняем укладку третьего документа
	abc::put(data, abc::major_t::ARRAY, 0);
	// Читатель бинарной записи
	abc::reader_t reader;
	// Настройки разбора записи
	abc::reader_t::settings_t settings;
	// Выполняем разрешение разбора потока документов
	settings.stream = true;
	// Выполняем установку настроек разбора записи
	reader.settings(settings);
	// Выполняем сбор всех событий разбора
	const vector <string> events = collect(data, 0, reader);
	// Выполняем проверку собранной последовательности событий
	ASSERT_EQ(events, (vector <string> {"N1", ";", "N2", ";", "[", "]", ";", "."}))
		<< "код отказа: " << abc::message(reader.error());
	// Читатель бинарной записи, потока не ожидающий
	abc::reader_t single;
	// Выполняем сбор всех событий разбора
	collect(data, 0, single);
	// Выполняем проверку кода отказа на октеты за окончанием документа
	ASSERT_EQ(single.error(), abc::error_t::TRAILING_OCTETS);
}
/**
 * @brief Проверка отказов разбора
 *
 */
TEST(CodecAbcReader, Failures) {
	// Выполняем проверку отказа на пустую запись
	{
		// Читатель бинарной записи
		abc::reader_t reader;
		// Выполняем подачу пустой записи
		ASSERT_FALSE(reader.feed(nullptr, 0, true));
		// Выполняем проверку кода отказа
		ASSERT_EQ(reader.error(), abc::error_t::EMPTY_RECORD);
	}
	// Выполняем проверку отказа на конец вместимого вне вместимого
	{
		// Буфер собираемой записи
		vector <uint8_t> data;
		// Читатель бинарной записи
		abc::reader_t reader;
		// Выполняем укладку конца вместимого
		abc::mark(data, abc::major_t::SINGLE, static_cast <uint8_t> (abc::single_t::BREAK));
		// Выполняем подачу записи целиком
		ASSERT_FALSE(reader.feed(data.data(), data.size(), true));
		// Выполняем проверку кода отказа
		ASSERT_EQ(reader.error(), abc::error_t::UNBALANCED_BREAK);
	}
	// Выполняем проверку отказа на отображение, оборвавшееся на имени поля
	{
		// Буфер собираемой записи
		vector <uint8_t> data;
		// Читатель бинарной записи
		abc::reader_t reader;
		// Выполняем укладку отображения неопределённой длины
		abc::mark(data, abc::major_t::MAP, static_cast <uint8_t> (abc::single_t::BREAK));
		// Выполняем укладку имени поля отображения
		abc::put(data, abc::major_t::UNSIGNED, 1);
		// Выполняем укладку конца отображения
		abc::mark(data, abc::major_t::SINGLE, static_cast <uint8_t> (abc::single_t::BREAK));
		// Выполняем подачу записи целиком
		ASSERT_FALSE(reader.feed(data.data(), data.size(), true));
		// Выполняем проверку кода отказа
		ASSERT_EQ(reader.error(), abc::error_t::MISSING_VALUE);
	}
	// Выполняем проверку отказа на вместимое именем поля отображения
	{
		// Буфер собираемой записи
		vector <uint8_t> data;
		// Читатель бинарной записи
		abc::reader_t reader;
		// Выполняем укладку отображения из одной пары
		abc::put(data, abc::major_t::MAP, 1);
		// Выполняем укладку массива именем поля отображения
		abc::put(data, abc::major_t::ARRAY, 0);
		// Выполняем подачу записи целиком
		ASSERT_FALSE(reader.feed(data.data(), data.size(), true));
		// Выполняем проверку кода отказа
		ASSERT_EQ(reader.error(), abc::error_t::INVALID_KEY);
	}
	// Выполняем проверку отказа на строку, кодировке не отвечающую
	{
		// Буфер собираемой записи
		vector <uint8_t> data;
		// Читатель бинарной записи
		abc::reader_t reader;
		// Выполняем укладку метки строки вместе с её длиной
		abc::put(data, abc::major_t::STRING, 2);
		// Выполняем укладку октетов, кодировке не отвечающих
		data.insert(data.end(), {0xC0, 0x80});
		// Выполняем подачу записи целиком
		ASSERT_FALSE(reader.feed(data.data(), data.size(), true));
		// Выполняем проверку кода отказа
		ASSERT_EQ(reader.error(), abc::error_t::INVALID_ENCODING);
	}
	// Выполняем проверку отказа на запись, оборвавшуюся посреди значения
	{
		// Буфер собираемой записи
		vector <uint8_t> data;
		// Читатель бинарной записи
		abc::reader_t reader;
		// Выполняем укладку метки строки вместе с её длиной
		abc::put(data, abc::major_t::STRING, 4);
		// Выполняем укладку неполных октетов строки
		data.insert(data.end(), {'a', 'b'});
		// Выполняем подачу записи целиком
		ASSERT_FALSE(reader.feed(data.data(), data.size(), true));
		// Выполняем проверку кода отказа
		ASSERT_EQ(reader.error(), abc::error_t::UNEXPECTED_EOF);
	}
	// Выполняем проверку отказа на превышение глубины вложенности
	{
		// Буфер собираемой записи
		vector <uint8_t> data;
		// Читатель бинарной записи
		abc::reader_t reader;
		// Настройки разбора записи
		abc::reader_t::settings_t settings;
		// Выполняем установку предела глубины вложенности
		settings.maxDepth = 4;
		// Выполняем установку настроек разбора записи
		reader.settings(settings);
		/**
		 * Выполняем укладку вложенных массивов
		 */
		for(uint32_t i = 0; i < 8; i++)
			// Выполняем укладку очередного вложенного массива
			abc::put(data, abc::major_t::ARRAY, 1);
		// Выполняем подачу записи целиком
		ASSERT_FALSE(reader.feed(data.data(), data.size(), true));
		// Выполняем проверку кода отказа
		ASSERT_EQ(reader.error(), abc::error_t::DEPTH_EXCEEDED);
		// Выполняем проверку глубины, на какой отказ произошёл
		ASSERT_EQ(reader.location().depth, 4u);
	}
	// Выполняем проверку отказа на превышение длины строкового значения
	{
		// Буфер собираемой записи
		vector <uint8_t> data;
		// Читатель бинарной записи
		abc::reader_t reader;
		// Настройки разбора записи
		abc::reader_t::settings_t settings;
		// Выполняем установку предела длины строкового значения
		settings.maxString = 4;
		// Выполняем установку настроек разбора записи
		reader.settings(settings);
		// Выполняем укладку строки
		text(data, "слишком длинная");
		// Выполняем подачу записи целиком
		ASSERT_FALSE(reader.feed(data.data(), data.size(), true));
		// Выполняем проверку кода отказа
		ASSERT_EQ(reader.error(), abc::error_t::STRING_TOO_LONG);
	}
}
/**
 * @brief Проверка разбора расширений
 *
 * @details Целое любой ширины и десятичное выдаются октетами величины вместе со знаком
 * и порядком: преобразование их принадлежит не разбору, а потребителю
 *
 */
TEST(CodecAbcReader, Extensions) {
	// Выполняем проверку разбора целого неограниченной ширины
	{
		// Буфер собираемой записи
		vector <uint8_t> data;
		// Читатель бинарной записи
		abc::reader_t reader;
		// Выполняем укладку метки расширения
		abc::mark(data, abc::major_t::EXTEND, static_cast <uint8_t> (abc::extend_t::BIGNUM));
		// Выполняем укладку длины октетов величины
		abc::put(data, abc::major_t::UNSIGNED, 9);
		// Выполняем укладку знака величины
		data.push_back(1);
		/**
		 * Выполняем укладку октетов величины
		 */
		for(uint8_t i = 0; i < 9; i++)
			// Выполняем укладку очередного октета величины
			data.push_back(static_cast <uint8_t> (i + 1));
		// Выполняем сбор всех событий разбора
		const vector <string> events = collect(data, 0, reader);
		// Выполняем проверку собранной последовательности событий
		ASSERT_EQ(events, (vector <string> {"X-9", ";", "."})) << "код отказа: " << abc::message(reader.error());
	}
	// Выполняем проверку разбора десятичного числа
	{
		// Буфер собираемой записи
		vector <uint8_t> data;
		// Читатель бинарной записи
		abc::reader_t reader;
		// Выполняем укладку метки расширения
		abc::mark(data, abc::major_t::EXTEND, static_cast <uint8_t> (abc::extend_t::DECIMAL));
		// Выполняем укладку десятичного порядка величины
		abc::integer(data, -2);
		// Выполняем укладку длины октетов величины
		abc::put(data, abc::major_t::UNSIGNED, 2);
		// Выполняем укладку знака величины
		data.push_back(0);
		// Выполняем укладку октетов величины
		data.insert(data.end(), {0x39, 0x30});
		// Выполняем сбор всех событий разбора
		const vector <string> events = collect(data, 0, reader);
		// Выполняем проверку собранной последовательности событий
		ASSERT_EQ(events, (vector <string> {"D+2e-2", ";", "."})) << "код отказа: " << abc::message(reader.error());
	}
	// Выполняем проверку отказа на неканоническую запись величины
	{
		// Буфер собираемой записи
		vector <uint8_t> data;
		// Читатель бинарной записи
		abc::reader_t reader;
		// Выполняем укладку метки расширения
		abc::mark(data, abc::major_t::EXTEND, static_cast <uint8_t> (abc::extend_t::BIGNUM));
		// Выполняем укладку длины октетов величины
		abc::put(data, abc::major_t::UNSIGNED, 2);
		// Выполняем укладку знака величины
		data.push_back(0);
		// Выполняем укладку октетов величины с нулевым старшим октетом
		data.insert(data.end(), {0x01, 0x00});
		// Выполняем подачу записи целиком
		ASSERT_FALSE(reader.feed(data.data(), data.size(), true));
		// Выполняем проверку кода отказа
		ASSERT_EQ(reader.error(), abc::error_t::INVALID_BIGNUM);
	}
	// Выполняем проверку отказа на величину, объявленную отрицательным нулём
	{
		// Буфер собираемой записи
		vector <uint8_t> data;
		// Читатель бинарной записи
		abc::reader_t reader;
		// Выполняем укладку метки расширения
		abc::mark(data, abc::major_t::EXTEND, static_cast <uint8_t> (abc::extend_t::BIGNUM));
		// Выполняем укладку длины октетов величины
		abc::put(data, abc::major_t::UNSIGNED, 0);
		// Выполняем укладку знака величины
		data.push_back(1);
		// Выполняем подачу записи целиком
		ASSERT_FALSE(reader.feed(data.data(), data.size(), true));
		// Выполняем проверку кода отказа
		ASSERT_EQ(reader.error(), abc::error_t::INVALID_BIGNUM);
	}
}
/**
 * @brief Проверка сохранности содержимого при отложенном снятии событий
 *
 * @details Содержимое события хранится отрезком в буфере разбора, а разобранная часть
 * буфера усекается. Усечение при непустой очереди событий обратило бы отрезки уже
 * выданных событий в указания на снесённое, и содержимое их стало бы чужим
 *
 * @note Проверка эта нужна оттого, что договор невидим в самой записи: разбор, снесший
 * буфер под собранными событиями, отвечает успехом и выдаёт события в верном порядке.
 * Неверным становится лишь их содержимое, и заметить это сличением порядка нельзя
 *
 */
TEST(CodecAbcReader, DeferredEvents) {
	// Буфер собираемой записи
	vector <uint8_t> data;
	// Выполняем укладку массива из четырёх строк
	abc::put(data, abc::major_t::ARRAY, 4);
	// Выполняем укладку первой строки
	text(data, "первая строка");
	// Выполняем укладку второй строки
	text(data, "вторая строка");
	// Выполняем укладку третьей строки
	text(data, "третья строка");
	// Выполняем укладку четвёртой строки
	text(data, "четвёртая строка");
	/**
	 * Выполняем перебор всех размеров куска подачи
	 */
	for(size_t chunk = 1; chunk <= data.size(); chunk++){
		// Читатель бинарной записи
		abc::reader_t reader;
		// Смещение подаваемого куска записи
		size_t offset = 0;
		/**
		 * Выполняем подачу всей записи кусками, событий не снимая
		 */
		while(offset < data.size()){
			// Выполняем получение размера подаваемого куска
			const size_t size = (((data.size() - offset) < chunk) ? (data.size() - offset) : chunk);
			// Выполняем подачу куска записи
			ASSERT_TRUE(reader.feed(data.data() + offset, size, ((offset + size) >= data.size())))
				<< "размер куска: " << chunk << ", код отказа: " << abc::message(reader.error());
			// Выполняем сдвиг смещения подаваемого куска
			offset += size;
		}
		// Собираемая последовательность событий разбора
		vector <string> events;
		/**
		 * Выполняем снятие всех собранных событий разбора
		 */
		while(reader.next())
			// Выполняем добавление краткой записи события
			events.push_back(describe(reader));
		// Выполняем проверку собранной последовательности событий вместе с содержимым
		ASSERT_EQ(events, (vector <string> {"[", "S:первая строка", "S:вторая строка",
		 "S:третья строка", "S:четвёртая строка", "]", ";", "."})) << "размер куска: " << chunk;
	}
}
