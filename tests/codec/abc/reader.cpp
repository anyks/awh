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
#include <cstring>
#include <limits>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <gtest/gtest.h>
#include <codec/abc/reader.hpp>
#include <codec/abc/writer.hpp>

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
	 * @brief Функция извлечения объекта журнала проверок
	 *
	 * @details Журнал заводится единожды на весь набор и гасится: проверки отказов
	 *          выводили бы записью всякий свой отказ, а их тут большинство. Гашение
	 *          это - настройка журнала, а не молчание модуля: модуль доносит как
	 *          обычно, а показывать ли - решает журнал
	 *
	 * @return объект журнала проверок
	 *
	 */
	const log_t * logger() noexcept {
		// Объект фреймворка проверок
		static fmk_t fmk;
		// Объект журнала проверок
		static log_t log(& fmk);
		// Признак выполненной настройки журнала
		static const bool ready = [](){
			// Выполняем гашение вывода журнала проверок
			log.level(log_t::level_t::NONE);
			// Выводим признак выполненной настройки
			return true;
		}();
		// Снимаем неиспользуемый признак настройки
		(void) ready;
		// Выводим объект журнала проверок
		return & log;
	}
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
		abc::put(result, abc::group_t::STRING, static_cast <uint64_t> (text.size()));
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
	abc::mark(data, abc::group_t::SINGLE, static_cast <uint8_t> (abc::single_t::NUL));
	// Читатель бинарной записи
	abc::reader_t reader(::logger());
	// Выполняем сбор всех событий разбора
	vector <string> events = collect(data, 0, reader);
	// Выполняем проверку собранной последовательности событий
	ASSERT_EQ(events, (vector <string> {"null", ";", "."})) << "код отказа: " << abc::message(reader.error());
	// Выполняем очистку буфера собираемой записи
	data.clear();
	// Выполняем сброс состояния разбора
	reader.reset();
	// Выполняем укладку логического значения
	abc::mark(data, abc::group_t::SINGLE, static_cast <uint8_t> (abc::single_t::TRUE));
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
		abc::reader_t reader(::logger());
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
	abc::put(data, abc::group_t::ARRAY, 3);
	// Выполняем укладку первого значения массива
	abc::put(data, abc::group_t::UNSIGNED, 1);
	// Выполняем укладку вложенного массива из двух значений
	abc::put(data, abc::group_t::ARRAY, 2);
	// Выполняем укладку первого значения вложенного массива
	abc::put(data, abc::group_t::UNSIGNED, 2);
	// Выполняем укладку второго значения вложенного массива
	abc::put(data, abc::group_t::UNSIGNED, 3);
	// Выполняем укладку отображения из одной пары
	abc::put(data, abc::group_t::MAP, 1);
	// Выполняем укладку имени поля отображения
	text(data, "имя");
	// Выполняем укладку значения поля отображения
	abc::mark(data, abc::group_t::SINGLE, static_cast <uint8_t> (abc::single_t::NUL));
	// Читатель бинарной записи
	abc::reader_t reader(::logger());
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
	abc::put(data, abc::group_t::ARRAY, 2);
	// Выполняем укладку пустого массива
	abc::put(data, abc::group_t::ARRAY, 0);
	// Выполняем укладку пустого отображения
	abc::put(data, abc::group_t::MAP, 0);
	// Читатель бинарной записи
	abc::reader_t reader(::logger());
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
	abc::mark(data, abc::group_t::MAP, static_cast <uint8_t> (abc::single_t::BREAK));
	// Выполняем укладку имени поля отображения числом
	abc::put(data, abc::group_t::UNSIGNED, 7);
	// Выполняем укладку массива неопределённой длины
	abc::mark(data, abc::group_t::ARRAY, static_cast <uint8_t> (abc::single_t::BREAK));
	// Выполняем укладку значения массива
	abc::put(data, abc::group_t::UNSIGNED, 8);
	// Выполняем укладку конца массива
	abc::mark(data, abc::group_t::SINGLE, static_cast <uint8_t> (abc::single_t::BREAK));
	// Выполняем укладку конца отображения
	abc::mark(data, abc::group_t::SINGLE, static_cast <uint8_t> (abc::single_t::BREAK));
	// Читатель бинарной записи
	abc::reader_t reader(::logger());
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
	abc::put(data, abc::group_t::MAP, 3);
	// Выполняем укладку имени первого поля отображения
	text(data, "первое");
	// Выполняем укладку значения первого поля отображения
	abc::put(data, abc::group_t::UNSIGNED, 300000);
	// Выполняем укладку имени второго поля отображения
	text(data, "второе");
	// Выполняем укладку дробного значения двойной точности
	abc::real(data, 1.5);
	// Выполняем укладку имени третьего поля отображения
	text(data, "третье");
	// Выполняем укладку массива из двух значений
	abc::put(data, abc::group_t::ARRAY, 2);
	// Выполняем укладку двоичного значения
	abc::put(data, abc::group_t::BLOB, 4);
	// Выполняем укладку октетов двоичного значения
	data.insert(data.end(), {0x01, 0x02, 0x03, 0x04});
	// Выполняем укладку опознавателя
	abc::mark(data, abc::group_t::SINGLE, static_cast <uint8_t> (abc::single_t::UUID));
	/**
	 * Выполняем укладку октетов опознавателя
	 */
	for(uint8_t i = 0; i < 16; i++)
		// Выполняем укладку очередного октета опознавателя
		data.push_back(i);
	// Читатель бинарной записи, подающейся целиком
	abc::reader_t whole(::logger());
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
		abc::reader_t reader(::logger());
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
	abc::put(data, abc::group_t::UNSIGNED, 1);
	// Выполняем укладку второго документа
	abc::put(data, abc::group_t::UNSIGNED, 2);
	// Выполняем укладку третьего документа
	abc::put(data, abc::group_t::ARRAY, 0);
	// Читатель бинарной записи
	abc::reader_t reader(::logger());
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
	abc::reader_t single(::logger());
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
		abc::reader_t reader(::logger());
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
		abc::reader_t reader(::logger());
		// Выполняем укладку конца вместимого
		abc::mark(data, abc::group_t::SINGLE, static_cast <uint8_t> (abc::single_t::BREAK));
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
		abc::reader_t reader(::logger());
		// Выполняем укладку отображения неопределённой длины
		abc::mark(data, abc::group_t::MAP, static_cast <uint8_t> (abc::single_t::BREAK));
		// Выполняем укладку имени поля отображения
		abc::put(data, abc::group_t::UNSIGNED, 1);
		// Выполняем укладку конца отображения
		abc::mark(data, abc::group_t::SINGLE, static_cast <uint8_t> (abc::single_t::BREAK));
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
		abc::reader_t reader(::logger());
		// Выполняем укладку отображения из одной пары
		abc::put(data, abc::group_t::MAP, 1);
		// Выполняем укладку массива именем поля отображения
		abc::put(data, abc::group_t::ARRAY, 0);
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
		abc::reader_t reader(::logger());
		// Выполняем укладку метки строки вместе с её длиной
		abc::put(data, abc::group_t::STRING, 2);
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
		abc::reader_t reader(::logger());
		// Выполняем укладку метки строки вместе с её длиной
		abc::put(data, abc::group_t::STRING, 4);
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
		abc::reader_t reader(::logger());
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
			abc::put(data, abc::group_t::ARRAY, 1);
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
		abc::reader_t reader(::logger());
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
		abc::reader_t reader(::logger());
		// Выполняем укладку метки расширения
		abc::mark(data, abc::group_t::EXTEND, static_cast <uint8_t> (abc::extend_t::BIGNUM));
		// Выполняем укладку длины октетов величины
		abc::put(data, abc::group_t::UNSIGNED, 9);
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
		abc::reader_t reader(::logger());
		// Выполняем укладку метки расширения
		abc::mark(data, abc::group_t::EXTEND, static_cast <uint8_t> (abc::extend_t::DECIMAL));
		// Выполняем укладку десятичного порядка величины
		abc::integer(data, -2);
		// Выполняем укладку длины октетов величины
		abc::put(data, abc::group_t::UNSIGNED, 2);
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
		abc::reader_t reader(::logger());
		// Выполняем укладку метки расширения
		abc::mark(data, abc::group_t::EXTEND, static_cast <uint8_t> (abc::extend_t::BIGNUM));
		// Выполняем укладку длины октетов величины
		abc::put(data, abc::group_t::UNSIGNED, 2);
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
		abc::reader_t reader(::logger());
		// Выполняем укладку метки расширения
		abc::mark(data, abc::group_t::EXTEND, static_cast <uint8_t> (abc::extend_t::BIGNUM));
		// Выполняем укладку длины октетов величины
		abc::put(data, abc::group_t::UNSIGNED, 0);
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
	abc::put(data, abc::group_t::ARRAY, 4);
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
		abc::reader_t reader(::logger());
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
/**
 * @brief Проверка приёма октетов прямо в буфер разбора
 *
 */
TEST(CodecAbcReader, ReserveAndCommit){
	// Сборка бинарной записи
	abc::writer_t writer(::logger());
	// Выполняем укладку начала массива
	ASSERT_TRUE(writer.arrayBegin(static_cast <uint64_t> (2)));
	// Выполняем укладку первого значения массива
	ASSERT_TRUE(writer.text("первое"));
	// Выполняем укладку второго значения массива
	ASSERT_TRUE(writer.number(static_cast <uint64_t> (7)));
	// Выполняем укладку конца массива
	ASSERT_TRUE(writer.arrayEnd());
	// Выполняем получение собранной записи
	const vector <uint8_t> & record = writer.record();
	// Разбиратель бинарной записи
	abc::reader_t reader(::logger());
	// Смещение поданной части записи
	size_t offset = 0;
	// Количество снятых значений массива
	size_t values = 0;
	/**
	 * Выполняем подачу записи кусками по три октета прямо в буфер разбора
	 */
	while(offset < record.size()){
		// Выполняем получение размера подаваемого куска записи
		const size_t size = ((record.size() - offset) < 3 ? (record.size() - offset) : 3);
		/**
		 * Выполняем выдачу места под приём октетов: место это запрашивается с запасом,
		 * а принято бывает меньше, и хвост его обязан быть возвращён
		 */
		void * place = reader.reserve(8);
		// Выполняем проверку выданного места под приём октетов
		ASSERT_TRUE(place != nullptr);
		// Выполняем приём октетов записи прямо в выданное место
		::memcpy(place, record.data() + offset, size);
		// Выполняем сдвиг смещения поданной части записи
		offset += size;
		// Выполняем подачу принятых октетов разбирателю
		ASSERT_TRUE(reader.commit(size, offset >= record.size()))
			<< "код отказа: " << abc::message(reader.error());
		/**
		 * Выполняем снятие собранных событий разбора
		 */
		while(reader.next()){
			// Если событием является строковое значение либо число
			if((reader.event() == abc::event_t::STRING) || (reader.event() == abc::event_t::NUMBER))
				// Выполняем учёт снятого значения массива
				values++;
		}
	}
	// Выполняем проверку количества снятых значений массива
	ASSERT_EQ(values, 2ul);
}
/**
 * @brief Проверка отказа на подачу сверх выданного места
 *
 */
TEST(CodecAbcReader, CommitBeyondReserve){
	// Разбиратель бинарной записи
	abc::reader_t reader(::logger());
	// Выполняем выдачу места под приём октетов записи
	ASSERT_TRUE(reader.reserve(4) != nullptr);
	// Выполняем проверку отказа на подачу сверх выданного места
	ASSERT_FALSE(reader.commit(8));
	// Выполняем проверку кода отказа разбора
	ASSERT_EQ(reader.error(), abc::error_t::INTERNAL);
}
/**
 * @brief Проверка отказа на число внутри значения, собираемого кусками
 *
 */
TEST(CodecAbcReader, NumberInsideSegment){
	// Запись, где внутри строки, собираемой кусками, стоит число неограниченной ширины
	const vector <uint8_t> record = {
		0x5F,             // Начало строки, собираемой кусками
		0xE0,             // Метка целого неограниченной ширины
		0x01,             // Длина октетов величины
		0x00,             // Знак величины
		0x01,             // Октет величины
		0xDF              // Конец значения, собираемого кусками
	};
	// Разбиратель бинарной записи
	abc::reader_t reader(::logger());
	/**
	 * Выполняем проверку отказа разбора: куском собираемого значения вправе стоять лишь
	 * значение того же вида, и число внутри строки означает запись негодную
	 */
	ASSERT_FALSE(reader.feed(record.data(), record.size(), true));
	// Выполняем проверку кода отказа разбора
	ASSERT_EQ(reader.error(), abc::error_t::INVALID_SEGMENT);
}
/**
 * @brief Проверка того, что обработчик прямой выдачи заменяет собою очередь
 *
 */
TEST(CodecAbcReader, HandlerReplacesQueue){
	// Сборка бинарной записи
	abc::writer_t writer(::logger());
	// Выполняем укладку начала массива
	ASSERT_TRUE(writer.arrayBegin(static_cast <uint64_t> (3)));
	// Выполняем укладку значений массива
	ASSERT_TRUE(writer.number(static_cast <uint64_t> (1)));
	// Выполняем укладку второго значения массива
	ASSERT_TRUE(writer.number(static_cast <uint64_t> (2)));
	// Выполняем укладку третьего значения массива
	ASSERT_TRUE(writer.number(static_cast <uint64_t> (3)));
	// Выполняем укладку конца массива
	ASSERT_TRUE(writer.arrayEnd());
	// Количество событий, принятых обработчиком
	size_t taken = 0;
	// Разбиратель бинарной записи
	abc::reader_t reader(::logger());
	// Выполняем установку обработчика прямой выдачи событий разбора
	reader.handler([](void * context, abc::reader_t & reader, const abc::event_t event) noexcept -> void {
		// Разбиратель работе обработчика не нужен
		(void) reader;
		// Событие разбора работе обработчика не нужно
		(void) event;
		// Выполняем учёт принятого события разбора
		(* reinterpret_cast <size_t *> (context))++;
	}, & taken);
	// Выполняем подачу собранной записи разбирателю
	ASSERT_TRUE(reader.feed(writer.record().data(), writer.record().size(), true))
		<< "код отказа: " << abc::message(reader.error());
	// Выполняем проверку того, что события до обработчика дошли
	ASSERT_GT(taken, 5ul);
	/**
	 * Выполняем проверку того, что очередь выдачи пуста: событие, уже принятое
	 * обработчиком, ложась ещё и в очередь, копило бы её без предела - снимать с неё
	 * стало бы некому
	 */
	ASSERT_FALSE(reader.next());
}
/**
 * @brief Проверка того, что отказ разбора доходит до журнала фреймворка
 *
 * @details Кодек доносит об отказах штатным журналом, и донесение это - часть договора
 *          его, а не удобство. Без сторожа воронка отказа могла бы молчать вовсе, и
 *          заметить это стало бы некому: набор судит по коду отказа, а не по записям
 *
 */
TEST(CodecAbcReader, FailureReachesLogger) {
	// Объект фреймворка проверки
	fmk_t fmk;
	// Объект журнала проверки
	log_t log(& fmk);
	// Накопленные записи журнала
	vector <string> records;
	// Выполняем разрешение вывода записей в функцию обратного вызова
	log.mode({log_t::mode_t::DEFERRED});
	// Выполняем разрешение вывода записей предупреждения
	log.level(log_t::level_t::ALL);
	// Выполняем подписку на получение записей журнала
	log.subscribe([&records](const log_t::flag_t flag, string_view text) noexcept -> void {
		// Выполняем накопление полученной записи журнала
		records.push_back(string(text));
		// Снимаем неиспользуемый вид записи
		(void) flag;
	});
	// Разборщик бинарной записи с журналом проверки
	abc::reader_t reader(& log);
	// Заведомо негодная запись: подробность 0x1C ведущего октета не отведена ничему
	const vector <uint8_t> broken = {0x1C};
	// Выполняем подачу негодной записи разборщику
	ASSERT_FALSE(reader.feed(broken.data(), broken.size()));
	// Выполняем проверку того, что разбор отвечен отказом
	ASSERT_NE(reader.error(), abc::error_t::NONE);
	// Выполняем проверку того, что отказ дошёл до журнала
	ASSERT_FALSE(records.empty()) << "отказ разбора до журнала не дошёл";
	// Выполняем проверку того, что запись несёт вид отказа
	{
		// Признак найденной записи об отказе
		bool found = false;
		/**
		 * Выполняем перебор всех накопленных записей журнала
		 */
		for(const string & record : records)
			// Выполняем поиск текста отказа в очередной записи
			found = (found || (record.find(abc::message(reader.error())) != string::npos));
		// Выполняем проверку того, что запись об отказе найдена
		ASSERT_TRUE(found) << "запись журнала не несёт текста отказа: " << records.front();
	}
	/**
	 * Выполняем проверку того, что успешный разбор записей не порождает.
	 *
	 * Без этого сторож прошёл бы и при донесении обо ВСЯКОМ событии разбора: журнал
	 * заполнялся бы работою, а не отказами, и толку от него не стало бы вовсе
	 */
	{
		// Выполняем очистку накопленных записей журнала
		records.clear();
		// Разборщик годной записи с журналом проверки
		abc::reader_t plain(& log);
		// Сборщик годной записи
		abc::writer_t writer(& log);
		// Выполняем укладку строки в собираемую запись
		ASSERT_TRUE(writer.text("годная запись"));
		// Выполняем подачу годной записи разборщику
		ASSERT_TRUE(plain.feed(writer.record().data(), writer.record().size()))
			<< "код отказа: " << abc::message(plain.error());
		// Выполняем проверку того, что записей журнала не появилось
		ASSERT_TRUE(records.empty()) << "успешный разбор оставил запись: " << records.front();
	}
}
/**
 * @brief Проверка поверки записи на строгий вид
 *
 * @details Строгий вид требует трёх условий разом: наименьшая запись всякой метки,
 * запрет неопределённой длины и возрастание имён полей отображения. Признак
 * `flag_t::CANONICAL` заголовка контейнера есть ОБЪЯВЛЕНИЕ собирателя, а поверяется
 * оно здесь, разбором самих записей
 *
 * @note Проверка эта закрепляет и то, что вне строгого вида все четыре записи
 * разбираются: поверка обязана быть настройкой, а не свойством разбора
 *
 */
TEST(CodecAbcReader, CanonicalRefusal){
	/**
	 * Работа разбора записи затребованным видом
	 *
	 * @param record    разбираемая запись
	 * @param canonical признак поверки на строгий вид
	 * @return          код отказа разбора
	 */
	auto digest = [](const vector <uint8_t> & record, const bool canonical,
	 const abc::duplicate_t duplicates = abc::duplicate_t::KEEP) noexcept -> abc::error_t {
		// Разборщик бинарной записи
		abc::reader_t reader(::logger());
		// Выполняем получение настроек разбора
		abc::reader_t::settings_t settings = reader.settings();
		// Выполняем установку признака поверки на строгий вид
		settings.canonical = canonical;
		// Выполняем установку правила обращения с повтором имени поля
		settings.duplicates = duplicates;
		// Выполняем установку настроек разбора
		reader.settings(settings);
		// Если подача записи разборщику отвечена отказом
		if(!reader.feed(record.data(), record.size(), true))
			// Выводим код отказа разбора
			return reader.error();
		// Сообщаем, что запись разобрана
		return abc::error_t::NONE;
	};
	// Строгая запись отображения из двух пар: имена идут по возрастанию
	const vector <uint8_t> strict = {0xA2, 0x41, 'a', 0x01, 0x41, 'b', 0x02};
	// Та же пара, но имена идут по убыванию
	const vector <uint8_t> unordered = {0xA2, 0x41, 'b', 0x02, 0x41, 'a', 0x01};
	// То же имя, объявленное дважды
	const vector <uint8_t> duplicate = {0xA2, 0x41, 'a', 0x01, 0x41, 'a', 0x02};
	// Число единица, уложенное ведомым октетом вместо самой метки
	const vector <uint8_t> wide = {0x18, 0x01};
	// Массив неопределённой длины
	const vector <uint8_t> indefinite = {0x9F, 0x01, 0xDF};
	/**
	 * Вне строгого вида разбираются ВСЕ пять записей: поверка есть настройка
	 *
	 * @note Повтор имени поля разбирается лишь при правиле, его дозволяющем: умолчанием
	 * взят ОТКАЗ, и он поверяется ниже отдельно. Прочие четыре записи от правила этого
	 * не зависят вовсе
	 */
	ASSERT_EQ(digest(strict, false), abc::error_t::NONE);
	ASSERT_EQ(digest(unordered, false), abc::error_t::NONE);
	ASSERT_EQ(digest(duplicate, false), abc::error_t::NONE);
	ASSERT_EQ(digest(wide, false), abc::error_t::NONE);
	ASSERT_EQ(digest(indefinite, false), abc::error_t::NONE);
	/**
	 * Повтор имени поля отвергается УМОЛЧАНИЕМ вне строгого вида
	 *
	 * @details Умолчание это объявлено у самого правила `duplicate_t`: молчаливый выбор
	 * одного из двух значений означал бы потерю данных, а повтор имени есть приём
	 * путаницы разборов. Прочие четыре записи повтора не несут и умолчанием проходят
	 */
	ASSERT_EQ(digest(duplicate, false, abc::duplicate_t::REFUSE), abc::error_t::DUPLICATE_KEY);
	ASSERT_EQ(digest(strict, false, abc::duplicate_t::REFUSE), abc::error_t::NONE);
	ASSERT_EQ(digest(unordered, false, abc::duplicate_t::REFUSE), abc::error_t::NONE);
	ASSERT_EQ(digest(wide, false, abc::duplicate_t::REFUSE), abc::error_t::NONE);
	ASSERT_EQ(digest(indefinite, false, abc::duplicate_t::REFUSE), abc::error_t::NONE);
	/**
	 * Строгим видом принимается лишь строгая запись, прочие отвергаются по своему поводу
	 */
	ASSERT_EQ(digest(strict, true), abc::error_t::NONE);
	ASSERT_EQ(digest(unordered, true), abc::error_t::UNORDERED_KEY);
	ASSERT_EQ(digest(duplicate, true), abc::error_t::UNORDERED_KEY);
	ASSERT_EQ(digest(wide, true), abc::error_t::NON_MINIMAL_TAG);
	ASSERT_EQ(digest(indefinite, true), abc::error_t::INDEFINITE_REFUSED);
}
/**
 * @brief Проверка поверки повтора имени поля на широком отображении
 *
 * @details Поверка эта идёт двумя дорогами: малое отображение сличается обходом
 * перечня имён, а по достижении порога заводится указатель, и сличение идёт гнездом.
 * Проверка ведёт число имён ЧЕРЕЗ порог, накрывая обе дороги и само перестроение
 *
 * @note Повтор кладётся то в начало отображения, то в конец: указатель, потерявший
 * прежние имена при перестроении, пропустил бы первый и поймал второй
 *
 */
TEST(CodecAbcReader, DuplicateWideMapping){
	/**
	 * Сборка записи отображения объявленного числа пар
	 *
	 * @param count  число пар отображения
	 * @param repeat место пары, чьё имя повторяется последней парой, либо число пар
	 * @return       собранная запись отображения
	 */
	auto build = [](const uint32_t count, const uint32_t repeat) noexcept -> vector <uint8_t> {
		// Собираемая запись отображения
		vector <uint8_t> result;
		// Выполняем укладку метки отображения неопределённой длины
		result.push_back(0xBF);
		/**
		 * Выполняем укладку всех пар отображения
		 */
		for(uint32_t i = 0; i < count; i++){
			// Выполняем получение порядка имени поля, укладываемого парой
			const uint32_t index = (((i + 1) == count) && (repeat < count) ? repeat : i);
			// Выполняем укладку метки имени поля длиною в два октета
			result.push_back(0x42);
			// Выполняем укладку старшего октета имени поля
			result.push_back(static_cast <uint8_t> (0x30 + ((index >> 6) & 0x3F)));
			// Выполняем укладку младшего октета имени поля
			result.push_back(static_cast <uint8_t> (0x30 + (index & 0x3F)));
			// Выполняем укладку значения пары отображения
			result.push_back(0x01);
		}
		// Выполняем укладку метки конца отображения
		result.push_back(0xDF);
		// Выводим собранную запись отображения
		return result;
	};
	/**
	 * Работа разбора записи при отказе от повтора имени поля
	 *
	 * @param record разбираемая запись
	 * @return       код отказа разбора
	 */
	auto digest = [](const vector <uint8_t> & record) noexcept -> abc::error_t {
		// Разборщик бинарной записи
		abc::reader_t reader(::logger());
		// Выполняем получение настроек разбора
		abc::reader_t::settings_t settings = reader.settings();
		// Выполняем установку правила обращения с повтором имени поля
		settings.duplicates = abc::duplicate_t::REFUSE;
		// Выполняем установку настроек разбора
		reader.settings(settings);
		// Если подача записи разборщику отвечена отказом
		if(!reader.feed(record.data(), record.size(), true))
			// Выводим код отказа разбора
			return reader.error();
		// Сообщаем, что запись разобрана
		return abc::error_t::NONE;
	};
	/**
	 * Выполняем перебор чисел пар отображения, ведущих число имён через порог
	 *
	 * @note Порог заведения указателя равен 32, а перестроение его случается на
	 * удвоении: ряд накрывает обход перечня, заведение указателя и два перестроения
	 */
	for(const uint32_t count : {2u, 31u, 32u, 33u, 64u, 65u, 129u, 300u}){
		// Записи отображения без повтора имени поля разбор отвечает согласием
		ASSERT_EQ(digest(build(count, count)), abc::error_t::NONE) << "число пар: " << count;
		/**
		 * Выполняем перебор мест повторяемого имени поля отображения
		 */
		for(const uint32_t repeat : {0u, (count / 2u), (count - 2u)}){
			// Если места того у отображения нет вовсе, переходим к следующему
			if((count < 2) || (repeat > (count - 2)))
				// Переходим к следующему месту повторяемого имени поля
				continue;
			// Записи отображения с повтором имени поля разбор отвечает отказом
			ASSERT_EQ(digest(build(count, repeat)), abc::error_t::DUPLICATE_KEY)
			 << "число пар: " << count << ", место повтора: " << repeat;
		}
	}
	/**
	 * Выполняем поверку усечения перечня имён вложенным отображением
	 *
	 * @details Указатель вмещающего отображения обязан пережить открытие и закрытие
	 * вложенного: имена его укладываются в тот же перечень, а закрытие усекает перечень
	 * до начала части вмещающего. Повтор кладётся ПОСЛЕ вложенного отображения
	 */
	{
		// Собираемая запись отображения с вложенным отображением
		vector <uint8_t> record = build(64, 64);
		// Выполняем снятие метки конца вмещающего отображения
		record.pop_back();
		// Выполняем укладку имени поля вложенного отображения
		record.insert(record.end(), {0x42, 0x7A, 0x7A});
		// Выполняем укладку вложенного отображения из одной пары
		record.insert(record.end(), {0xBF, 0x42, 0x30, 0x30, 0x01, 0xDF});
		// Выполняем укладку пары, повторяющей самое первое имя поля
		record.insert(record.end(), {0x42, 0x30, 0x30, 0x01});
		// Выполняем укладку метки конца вмещающего отображения
		record.push_back(0xDF);
		// Записи этой разбор отвечает отказом повтора имени поля
		ASSERT_EQ(digest(record), abc::error_t::DUPLICATE_KEY);
	}
}
/**
 * @brief Проверка согласия строгой сборки со строгим разбором
 *
 * @details Сборка и разбор судят о строгом виде порознь, и разойтись им нельзя:
 * запись, собранная строгим видом, обязана строгим видом и приниматься
 *
 */
TEST(CodecAbcReader, CanonicalAgreesWithWriter){
	// Сборка бинарной записи строгим видом
	abc::writer_t writer(::logger());
	// Выполняем получение настроек сборки
	abc::writer_t::settings_t settings = writer.settings();
	// Выполняем установку строгого вида записи
	settings.canonical = true;
	// Выполняем установку настроек сборки
	writer.settings(settings);
	// Выполняем укладку начала отображения
	ASSERT_TRUE(writer.mapBegin(static_cast <uint64_t> (3)));
	// Выполняем укладку первой пары отображения
	ASSERT_TRUE(writer.text("aa") && writer.number(static_cast <uint64_t> (0)));
	// Выполняем укладку второй пары отображения
	ASSERT_TRUE(writer.text("bb") && writer.number(static_cast <uint64_t> (300)));
	// Выполняем укладку третьей пары отображения
	ASSERT_TRUE(writer.text("cc") && writer.text(string(400, 'z')));
	// Выполняем укладку конца отображения
	ASSERT_TRUE(writer.mapEnd());
	// Разборщик бинарной записи строгим видом
	abc::reader_t reader(::logger());
	// Выполняем получение настроек разбора
	abc::reader_t::settings_t parsing = reader.settings();
	// Выполняем установку признака поверки на строгий вид
	parsing.canonical = true;
	// Выполняем установку настроек разбора
	reader.settings(parsing);
	// Выполняем подачу собранной записи разборщику
	ASSERT_TRUE(reader.feed(writer.record().data(), writer.record().size(), true))
		<< "код отказа: " << abc::message(reader.error());
}
/**
 * @brief Проверка правил обращения со строкой, не отвечающей кодировке UTF-8
 *
 * @details Правило `malformed_t` стоит НА МЕСТЕ прежнего признака проверки строк, а не
 *          рядом с ним: `REFUSE` отвечает признаку поднятым, `PASS` - снятым, а `REPLACE`
 *          заведён им обоим третьим исходом. Проверка эта закрепляет все три разом на
 *          одной и той же записи
 *
 * @note `REPLACE` выдаёт содержимое из ОТДЕЛЬНОГО хранилища: длина знака замены с длиною
 *       подменяемого не совпадает, и отрезок в буфере разбора её не вместил бы. Признак
 *       `repaired` сообщает потребителю, откуда взято содержимое, и поверяется здесь же
 *
 */
/**
 * @brief Проверка того, что исправление негодных строк повторов имён НЕ создаёт
 *
 * @details Правило `malformed_t::REPLACE` подменяет негодную последовательность знаком
 * U+FFFD, и два РАЗНЫХ негодных имени обращаются в одно и то же содержимое. Сличение же
 * имён - и на повтор, и на возрастание - ведётся по ЗАПИСИ, а не по исправленной строке,
 * оттого повтором такие имена не становятся
 *
 * @note Уклад объявлен намеренным: поверка строгого вида есть договор о ЗАПИСИ, и
 * исправление содержимого его не касается. Сличай разбор исправленные строки - запись
 * ниже отвечала бы отказом повтора, а строгий вид - отказом порядка, и оба отказа были
 * бы о том, чего в записи нет
 *
 */
TEST(CodecAbcReader, RepairCreatesNoDuplicates){
	/**
	 * Отображение из двух пар, чьи имена суть РАЗНЫЕ негодные октеты: {\xFE: 1, \xFF: 2}
	 *
	 * @note Обе строки исправляются в один и тот же знак замены, а записи их расходятся
	 * последним октетом и идут ПО ВОЗРАСТАНИЮ
	 */
	const vector <uint8_t> record = {
		0xA2,
		0x41, 0xFE, 0x01,
		0x41, 0xFF, 0x02
	};
	/**
	 * @brief Работа разбора записи объявленными правилами
	 *
	 * @param canonical  признак поверки на строгий вид
	 * @param duplicates правило обращения с повтором имени поля
	 * @param patched    количество исправленных строк
	 * @return           код отказа разбора
	 */
	auto digest = [&record](const bool canonical, const abc::duplicate_t duplicates,
	 size_t & patched) noexcept -> abc::error_t {
		// Разборщик бинарной записи
		abc::reader_t reader(::logger());
		// Выполняем получение настроек разбора
		abc::reader_t::settings_t settings = reader.settings();
		// Выполняем установку правила подмены негодной строки знаком замены
		settings.malformed = abc::malformed_t::REPLACE;
		// Выполняем установку признака поверки на строгий вид
		settings.canonical = canonical;
		// Выполняем установку правила обращения с повтором имени поля
		settings.duplicates = duplicates;
		// Выполняем установку настроек разбора
		reader.settings(settings);
		// Выполняем сброс количества исправленных строк
		patched = 0;
		// Если подача записи разборщику отвечена отказом
		if(!reader.feed(record.data(), record.size(), true))
			// Выводим код отказа разбора
			return reader.error();
		/**
		 * Выполняем снятие всех событий разбора
		 */
		while(reader.next()){
			// Если событием является имя поля отображения
			if(reader.event() == abc::event_t::KEY){
				// Если содержимое имени поля было исправлено
				if(reader.value().repaired)
					// Выполняем учёт исправленной строки
					patched++;
			}
		}
		// Сообщаем, что запись разобрана
		return abc::error_t::NONE;
	};
	// Количество исправленных строк разбора
	size_t patched = 0;
	/**
	 * Отказ от повтора имени поля записи этой не отвергает: записи имён различны
	 */
	ASSERT_EQ(digest(false, abc::duplicate_t::REFUSE, patched), abc::error_t::NONE);
	// Исправлены обязаны быть ОБА имени поля
	ASSERT_EQ(patched, static_cast <size_t> (2));
	/**
	 * Строгий вид записи этой не отвергает: записи имён идут по возрастанию
	 *
	 * @note Возрастание это - записи, а не исправленной строки: строки у обоих имён
	 * одинаковы, и сличение их дало бы отказ порядка
	 */
	ASSERT_EQ(digest(true, abc::duplicate_t::KEEP, patched), abc::error_t::NONE);
	// Исправлены обязаны быть ОБА имени поля
	ASSERT_EQ(patched, static_cast <size_t> (2));
	/**
	 * Настоящий повтор записи отвергается и при исправлении строк
	 *
	 * @note Половина вторая договора: исправление не должно ни создавать повторов, ни
	 * ПРЯТАТЬ их. Оба имени ниже суть одна и та же негодная запись
	 */
	{
		// Отображение, где одна и та же негодная запись имени объявлена дважды
		const vector <uint8_t> repeated = {
			0xA2,
			0x41, 0xFF, 0x01,
			0x41, 0xFF, 0x02
		};
		// Разборщик бинарной записи
		abc::reader_t reader(::logger());
		// Выполняем получение настроек разбора
		abc::reader_t::settings_t settings = reader.settings();
		// Выполняем установку правила подмены негодной строки знаком замены
		settings.malformed = abc::malformed_t::REPLACE;
		// Выполняем установку отказа от повтора имени поля
		settings.duplicates = abc::duplicate_t::REFUSE;
		// Выполняем установку настроек разбора
		reader.settings(settings);
		// Подача записи разборщику обязана быть отвечена отказом
		ASSERT_FALSE(reader.feed(repeated.data(), repeated.size(), true));
		// Отказ обязан быть именно о повторе имени поля
		ASSERT_EQ(reader.error(), abc::error_t::DUPLICATE_KEY);
	}
}
/**
 * @brief Проверка отказа на имени поля отображения, собираемом кусками
 *
 * @details Отказ этот держит собою УКЛАД поверки повтора. Ведущие октеты имени
 * запоминаются в миг снятия единицы, а употребляются в миг учёта её, - и годно это лишь
 * покуда имя есть ОДНА единица. Собирайся имя кусками, между запоминанием и
 * употреблением легли бы куски, звенья стека да метка конца, и запомненное разошлось бы
 * с употребляемым молча
 *
 * @note Отказ этот не заводился ради поверки повтора - строка неопределённой длины
 * отвергается именем поля наравне с перечнем и отображением, - но поверка на него
 * ОПИРАЕТСЯ. Закрепляется потому здесь, у разбора, а не только у значения: отступление
 * от отказа сломало бы сличение имён, а не одно лишь понятие имени
 *
 */
TEST(CodecAbcReader, ChunkedKeyRefused){
	/**
	 * Работа разбора записи при отказе от повтора имени поля
	 *
	 * @param record разбираемая запись
	 * @return       код отказа разбора
	 */
	auto digest = [](const vector <uint8_t> & record) noexcept -> abc::error_t {
		// Разборщик бинарной записи
		abc::reader_t reader(::logger());
		// Выполняем получение настроек разбора
		abc::reader_t::settings_t settings = reader.settings();
		// Выполняем установку отказа от повтора имени поля
		settings.duplicates = abc::duplicate_t::REFUSE;
		// Выполняем установку настроек разбора
		reader.settings(settings);
		// Если подача записи разборщику отвечена отказом
		if(!reader.feed(record.data(), record.size(), true))
			// Выводим код отказа разбора
			return reader.error();
		// Сообщаем, что запись разобрана
		return abc::error_t::NONE;
	};
	/**
	 * Отображение, чьё имя поля собирается кусками: {"ab": 1}
	 */
	const vector <uint8_t> chunked = {
		0xA1,
		0x5F, 0x41, 'a', 0x41, 'b', 0xDF,
		0x01
	};
	// Имя поля, собираемое кусками, обязано быть отвергнуто
	ASSERT_EQ(digest(chunked), abc::error_t::INVALID_KEY);
	/**
	 * То же имя, уложенное ОДНОЙ единицей, принимается
	 *
	 * @note Половина вторая договора: отвергается собираемость кусками, а не содержимое
	 */
	const vector <uint8_t> whole = {
		0xA1,
		0x42, 'a', 'b',
		0x01
	};
	// Имя поля, уложенное одной единицей, обязано быть принято
	ASSERT_EQ(digest(whole), abc::error_t::NONE);
	/**
	 * Значение пары, собираемое кусками, отказа НЕ вызывает
	 *
	 * @note Половина третья: отказ принадлежит имени, а не строке неопределённой длины
	 * вообще, - иначе он запрещал бы поточную укладку значений заодно
	 */
	const vector <uint8_t> value = {
		0xA1,
		0x42, 'a', 'b',
		0x5F, 0x41, 'c', 0x41, 'd', 0xDF
	};
	// Значение, собираемое кусками, обязано быть принято
	ASSERT_EQ(digest(value), abc::error_t::NONE);
}
/**
 * @brief Проверка независимости от нарезки на куски у настроек, ведущих состояние
 *
 * @details Проверка `ChunkIndependence` берёт запись простую, где разбор состояния
 * между кусками почти не несёт. Настройки же, заведённые позже, состояние это ВЕДУТ:
 * отказ от повтора имени копит записи имён вместе с указателем на них, строгий вид
 * держит предыдущее имя, а подмена негодной строки копит исправленное содержимое в своём
 * вместилище и указывает на него отрезком. Всякое из этих вместилищ живёт ДОЛЬШЕ одного
 * куска, а буфер разбора между подачами усекается
 *
 * @note Сличается и выдача событий, и код отказа: настройка, теряющая состояние на
 * границе куска, отвечает не отказом, а ИНЫМ разбором - повтор пропускается, порядок не
 * поверяется, исправленная строка читается чужою. Сличение с подачей целиком ловит все
 * три случая разом
 *
 */
TEST(CodecAbcReader, ChunkIndependenceOfStatefulSettings){
	/**
	 * @brief Работа разбора записи объявленными настройками кусками заданного размера
	 *
	 * @param data     разбираемая запись
	 * @param settings настройки разбора записи
	 * @param chunk    размер куска подачи, ноль - подача целиком
	 * @param error    код отказа разбора
	 * @param defer    признак отложенного снятия событий
	 * @return         собранная последовательность событий разбора
	 */
	auto sweep = [](const vector <uint8_t> & data, const abc::reader_t::settings_t & settings,
	 const size_t chunk, abc::error_t & error, const bool defer = false) noexcept -> vector <string> {
		// Читатель бинарной записи
		abc::reader_t reader(::logger());
		// Выполняем установку настроек разбора записи
		reader.settings(settings);
		// Собираемая последовательность событий разбора
		vector <string> result;
		// Выполняем установку размера куска подачи
		const size_t step = ((chunk > 0) ? chunk : (data.empty() ? 1 : data.size()));
		// Смещение подаваемого куска записи
		size_t offset = 0;
		/**
		 * Выполняем подачу записи кусками установленного размера
		 *
		 * @note Сбор ведётся СВОЙ, а не общим помощником: тот при отказе выходит, не сняв
		 * уже собранных событий, и подача целиком давала бы пустую выдачу там, где подача
		 * кусками выдавала бы события, снятые до отказа. Расхождение это принадлежало бы
		 * помощнику, а не разбору
		 */
		while(offset < data.size()){
			// Выполняем получение размера подаваемого куска
			const size_t size = (((data.size() - offset) < step) ? (data.size() - offset) : step);
			// Признак того, что подаваемый кусок последний
			const bool last = ((offset + size) >= data.size());
			// Признак успешной подачи куска записи разборщику
			const bool taken = reader.feed(data.data() + offset, size, last);
			/**
			 * Если снятие событий не отложено, снимаем все собранные
			 *
			 * @note Отложенное снятие есть ВТОРОЙ способ работы потребителя, и опасность у
			 * него своя: событие, снятое позже, указывает отрезком в буфер разбора, а тот
			 * между подачами усекается. Способ этот и стережёт вместилища, живущие дольше
			 * куска, - снятие сразу после подачи их не трогает вовсе
			 */
			if(!defer){
				/**
				 * Выполняем снятие всех собранных событий разбора
				 */
				while(reader.next())
					// Выполняем добавление краткой записи события
					result.push_back(describe(reader));
			}
			// Если подача куска записи отвечена отказом, разбор прекращается
			if(!taken)
				// Прекращаем подачу записи разборщику
				break;
			// Выполняем сдвиг смещения подаваемого куска
			offset += size;
		}
		/**
		 * Если снятие событий отложено, снимаем все собранные разом
		 */
		if(defer){
			/**
			 * Выполняем снятие всех собранных событий разбора
			 */
			while(reader.next())
				// Выполняем добавление краткой записи события
				result.push_back(describe(reader));
		}
		// Выполняем снятие кода отказа разбора
		error = reader.error();
		// Выводим собранную последовательность событий разбора
		return result;
	};
	/**
	 * @brief Работа поверки независимости разбора от нарезки на куски
	 *
	 * @param title    название поверяемого случая
	 * @param data     разбираемая запись
	 * @param settings настройки разбора записи
	 */
	auto verify = [&sweep](const string & title, const vector <uint8_t> & data,
	 const abc::reader_t::settings_t & settings) noexcept -> void {
		// Код отказа разбора записи, поданной целиком
		abc::error_t expected = abc::error_t::NONE;
		// Выполняем сбор событий разбора записи, поданной целиком
		const vector <string> sample = sweep(data, settings, 0, expected);
		/**
		 * Выполняем перебор всех размеров куска подачи
		 */
		for(size_t chunk = 1; chunk <= data.size(); chunk++){
			// Код отказа разбора записи, поданной кусками
			abc::error_t error = abc::error_t::NONE;
			// Выполняем сбор событий разбора записи, поданной кусками
			const vector <string> events = sweep(data, settings, chunk, error);
			// Код отказа обязан отвечать коду при подаче целиком
			ASSERT_EQ(error, expected) << title << ", размер куска: " << chunk;
			// Выдача событий обязана отвечать выдаче при подаче целиком
			ASSERT_EQ(events, sample) << title << ", размер куска: " << chunk;
			// Код отказа при отложенном снятии событий
			abc::error_t delayed = abc::error_t::NONE;
			// Выполняем сбор событий разбора при отложенном снятии их
			const vector <string> deferred = sweep(data, settings, chunk, delayed, true);
			// Код отказа обязан отвечать коду при подаче целиком
			ASSERT_EQ(delayed, expected) << title << ", отложенно, размер куска: " << chunk;
			// Выдача событий обязана отвечать выдаче при подаче целиком
			ASSERT_EQ(deferred, sample) << title << ", отложенно, размер куска: " << chunk;
		}
	};
	/**
	 * Выполняем поверку отказа от повтора имени поля отображения
	 */
	{
		// Настройки разбора записи
		abc::reader_t::settings_t settings;
		// Выполняем установку отказа от повтора имени поля
		settings.duplicates = abc::duplicate_t::REFUSE;
		// Отображение без повтора имени поля: {"альфа": 1, "бета": 2, "гамма": 3}
		vector <uint8_t> sound;
		// Выполняем укладку отображения из трёх пар
		abc::put(sound, abc::group_t::MAP, 3);
		/**
		 * Выполняем укладку всех пар отображения
		 */
		for(const string & word : {string{"альфа"}, string{"бета"}, string{"гамма"}}){
			// Выполняем укладку имени очередного поля отображения
			text(sound, word);
			// Выполняем укладку значения очередного поля отображения
			abc::put(sound, abc::group_t::UNSIGNED, 1);
		}
		// Выполняем поверку записи без повтора имени поля
		verify("повтора нет", sound, settings);
		// Отображение С повтором имени поля: {"альфа": 1, "бета": 2, "альфа": 3}
		vector <uint8_t> repeated;
		// Выполняем укладку отображения из трёх пар
		abc::put(repeated, abc::group_t::MAP, 3);
		/**
		 * Выполняем укладку всех пар отображения
		 */
		for(const string & word : {string{"альфа"}, string{"бета"}, string{"альфа"}}){
			// Выполняем укладку имени очередного поля отображения
			text(repeated, word);
			// Выполняем укладку значения очередного поля отображения
			abc::put(repeated, abc::group_t::UNSIGNED, 1);
		}
		// Выполняем поверку записи с повтором имени поля
		verify("повтор есть", repeated, settings);
	}
	/**
	 * Выполняем поверку строгого вида записи
	 */
	{
		// Настройки разбора записи
		abc::reader_t::settings_t settings;
		// Выполняем установку поверки на строгий вид
		settings.canonical = true;
		// Строгое отображение: имена идут по возрастанию
		const vector <uint8_t> strict = {
			0xA3,
			0x41, 'a', 0x01,
			0x41, 'b', 0x02,
			0x41, 'c', 0x03
		};
		// Выполняем поверку записи строгого вида
		verify("строгий вид", strict, settings);
		// Отображение, чьи имена идут не по возрастанию
		const vector <uint8_t> unordered = {
			0xA3,
			0x41, 'a', 0x01,
			0x41, 'c', 0x03,
			0x41, 'b', 0x02
		};
		// Выполняем поверку записи, строгому виду не отвечающей
		verify("порядок нарушен", unordered, settings);
	}
	/**
	 * Выполняем поверку подмены негодной строки знаком замены
	 */
	{
		// Настройки разбора записи
		abc::reader_t::settings_t settings;
		// Выполняем установку подмены негодной строки знаком замены
		settings.malformed = abc::malformed_t::REPLACE;
		/**
		 * Перечень из трёх строк, две из которых негодны
		 *
		 * @note Негодных строк ДВЕ намеренно: вместилище исправленного растёт, и отрезок
		 * второй строки лежит за первой, - потеря его на границе куска дала бы первую
		 * строку вместо второй
		 */
		const vector <uint8_t> patched = {
			0x83,
			0x43, 0xD0, 0xB0, 0xFF,
			0x42, 'o', 'k',
			0x43, 0xFE, 0xD0, 0xB1
		};
		// Выполняем поверку записи с негодными строками
		verify("подмена негодного", patched, settings);
	}
}
/**
 * @brief Проверка сброса состояния ПОСРЕДИ незавершённой записи
 *
 * @details Сброс между записями завершёнными поверяется отдельно и лёгок: разбор к тому
 * мигу пуст сам собою. Сброс же посреди записи застаёт разбор во всей его полноте -
 * открытый стек вместимых, накопленные записи имён вместе с указателем на них,
 * исправленные строки да неснятые события, - и остаток любого из этих вместилищ
 * отравил бы СЛЕДУЮЩУЮ запись, а не текущую
 *
 * @note Сличение идёт с разбирателем, заведённым СВЕЖИМ: утверждать «вторая запись
 * разобралась» мало - она разобралась бы и с чужими именами в перечне повторов. Обрыв
 * ведётся по всем длинам начала разом, ибо какая из них застанет разбор в самом опасном
 * месте, разбором кода не решается
 *
 * @note Из четырёх вместилищ, сбрасываемых `reset()`, проверка эта ловит снятие ОДНОГО -
 * очереди событий. Прочие три - перечень имён, вместилище октетов их и вместилище
 * исправленных строк - остатком своим разбор НЕ отравляют: звено стека держит начало
 * своей части перечня, и остаток лежит НИЖЕ этого начала, куда сличение не заходит.
 * Очистка их есть опрятность памяти, а не правильность, и снятие её проверкою
 * поведения не ловится вовсе - лишь ростом расхода. Проверено щупом 30.08.2026: снятие
 * любой из трёх проверку не роняет
 *
 */
TEST(CodecAbcReader, ResetMidRecordForgetsEverything){
	// Настройки разбора, ведущие состояние между подачами
	abc::reader_t::settings_t settings;
	// Выполняем установку отказа от повтора имени поля
	settings.duplicates = abc::duplicate_t::REFUSE;
	// Выполняем установку подмены негодной строки знаком замены
	settings.malformed = abc::malformed_t::REPLACE;
	/**
	 * Обрываемая запись: отображение с негодной строкой значением
	 *
	 * @note Запись нарочно наполняет ВСЕ вместилища разом - стек открыт отображением,
	 * имена ложатся в перечень повторов, негодная строка ложится в исправленные
	 */
	const vector <uint8_t> broken = {
		0xA3,
		0x42, 'a', 'a', 0x43, 0xD0, 0xB0, 0xFF,
		0x42, 'b', 'b', 0x01,
		0x42, 'c', 'c', 0x02
	};
	/**
	 * Разбираемая следом запись: своё отображение со своими именами
	 */
	const vector <uint8_t> sound = {
		0xA2,
		0x42, 'a', 'a', 0x43, 0xFE, 0xD0, 0xB1,
		0x42, 'b', 'b', 0x03
	};
	/**
	 * @brief Работа сбора событий разбора поданной записи
	 *
	 * @param reader разбиратель, каким ведётся разбор
	 * @param data   разбираемая запись
	 * @return       собранная последовательность событий разбора
	 */
	auto drain = [](abc::reader_t & reader, const vector <uint8_t> & data) noexcept -> vector <string> {
		// Собираемая последовательность событий разбора
		vector <string> result;
		// Выполняем подачу записи разбирателю целиком
		const bool taken = reader.feed(data.data(), data.size(), true);
		/**
		 * Выполняем снятие всех собранных событий разбора
		 */
		while(reader.next())
			// Выполняем добавление краткой записи события
			result.push_back(describe(reader));
		// Если подача записи отвечена отказом, помечаем это выдачей
		if(!taken)
			// Выполняем добавление пометки отказа к выдаче
			result.push_back("!");
		// Выводим собранную последовательность событий разбора
		return result;
	};
	// Разбиратель, заведённый свежим ради эталона
	abc::reader_t pristine(::logger());
	// Выполняем установку настроек разбора эталонному разбирателю
	pristine.settings(settings);
	// Выполняем сбор эталонной выдачи событий разбора
	const vector <string> sample = drain(pristine, sound);
	// Эталонная выдача обязана выйти непустой
	ASSERT_FALSE(sample.empty());
	/**
	 * Выполняем перебор всех длин начала обрываемой записи
	 */
	for(size_t length = 1; length < broken.size(); length++){
		// Разбиратель, обрываемый посреди записи
		abc::reader_t reader(::logger());
		// Выполняем установку настроек разбора
		reader.settings(settings);
		// Выполняем подачу начала обрываемой записи, конца не объявляя
		ASSERT_TRUE(reader.feed(broken.data(), length, false)) << "длина начала: " << length;
		/**
		 * Выполняем снятие ЧАСТИ собранных событий, оставляя прочие неснятыми
		 *
		 * @note Часть снимается намеренно: очередь событий - такое же вместилище, и
		 * сброс обязан снять и её
		 */
		if(reader.next())
			// Выполняем снятие одного собранного события разбора
			(void) describe(reader);
		// Выполняем сброс состояния разбора посреди записи
		reader.reset();
		// Выполняем сбор выдачи событий разбора следующей записи
		const vector <string> events = drain(reader, sound);
		// Выдача обязана отвечать выдаче разбирателя, заведённого свежим
		ASSERT_EQ(events, sample) << "длина начала: " << length;
	}
}
/**
 * @brief Проверка сличения имён по широкому вместилищу их
 *
 * @details Отрезок записи имени несёт смещение тридцатью двумя разрядами, а вместилище
 * растёт всеми именами открытых отображений разом. Пока смещение верно, повтор ловится
 * где угодно во вместилище; усечься ему - сличение пошло бы к чужому месту, и повтор
 * ПРОПУСКАЛСЯ бы молча, отказа не подав ни единого
 *
 * @note Порог самого переполнения - четыре гигабайта, и проверкой его не достать. Здесь
 * закрепляется МЕХАНИЗМ, какой сторож бережёт: имена крупные, числом за порогом
 * заведения указателя, повтор кладётся последним и далеко от начала. Щуп 31.08.2026 -
 * поле смещения, суженное до шестнадцати разрядов, - эту проверку роняет
 *
 */
TEST(CodecAbcReader, DuplicateAcrossWidePool){
	// Количество имён полей отображения
	const size_t COUNT = 71;
	// Ширина записи имени поля отображения в октетах
	const size_t WIDTH = 1000;
	/**
	 * @brief Работа сборки записи отображения объявленных имён
	 *
	 * @param repeat место имени, повторяемого последней парой, либо число имён
	 * @return       собранная запись отображения
	 */
	auto build = [&](const size_t repeat) noexcept -> vector <uint8_t> {
		// Собираемая запись отображения
		vector <uint8_t> result;
		// Выполняем укладку отображения объявленной длины
		abc::put(result, abc::group_t::MAP, static_cast <uint64_t> (COUNT + 1));
		/**
		 * @brief Работа укладки пары отображения объявленного номера
		 *
		 * @param number номер укладываемого имени поля
		 */
		auto pair = [&](const size_t number) noexcept -> void {
			// Выполняем укладку метки имени поля отображения
			abc::put(result, abc::group_t::STRING, static_cast <uint64_t> (WIDTH));
			// Выполняем укладку номера имени поля тремя первыми октетами
			result.push_back(static_cast <uint8_t> ('0' + (number / 100)));
			// Выполняем укладку второго октета номера имени поля
			result.push_back(static_cast <uint8_t> ('0' + ((number / 10) % 10)));
			// Выполняем укладку третьего октета номера имени поля
			result.push_back(static_cast <uint8_t> ('0' + (number % 10)));
			/**
			 * Выполняем укладку заполнения имени поля отображения
			 */
			for(size_t i = 3; i < WIDTH; i++)
				// Выполняем укладку очередного октета заполнения
				result.push_back(static_cast <uint8_t> ('x'));
			// Выполняем укладку значения пары отображения
			result.push_back(0x01);
		};
		/**
		 * Выполняем укладку всех пар отображения
		 */
		for(size_t number = 0; number < COUNT; number++)
			// Выполняем укладку очередной пары отображения
			pair(number);
		// Выполняем укладку пары, повторяющей объявленное имя поля
		pair(repeat);
		// Выводим собранную запись отображения
		return result;
	};
	/**
	 * @brief Работа разбора записи при отказе от повтора имени поля
	 *
	 * @param record разбираемая запись
	 * @return       код отказа разбора
	 */
	auto digest = [](const vector <uint8_t> & record) noexcept -> abc::error_t {
		// Разборщик бинарной записи
		abc::reader_t reader(::logger());
		// Выполняем получение настроек разбора
		abc::reader_t::settings_t settings = reader.settings();
		// Выполняем установку отказа от повтора имени поля
		settings.duplicates = abc::duplicate_t::REFUSE;
		// Выполняем установку настроек разбора
		reader.settings(settings);
		// Если подача записи разборщику отвечена отказом
		if(!reader.feed(record.data(), record.size(), true))
			// Выводим код отказа разбора
			return reader.error();
		// Сообщаем, что запись разобрана
		return abc::error_t::NONE;
	};
	/**
	 * Выполняем перебор мест повторяемого имени поля по всему вместилищу
	 *
	 * @note Места взяты началом, серединой и концом: усечение смещения губит сличение
	 * лишь у имён, легших ЗА границей поля, и повтор у начала прошёл бы и без сторожа
	 */
	for(const size_t repeat : {static_cast <size_t> (0), COUNT / 2, COUNT - 1}){
		// Записи с повтором имени поля разбор обязан ответить отказом
		ASSERT_EQ(digest(build(repeat)), abc::error_t::DUPLICATE_KEY) << "место повтора: " << repeat;
	}
	/**
	 * Записи без повтора имени поля разбор отвечает согласием
	 *
	 * @note Половина вторая договора: сличение обязано находить не только совпадение, но
	 * и различие. Имена различаются ТРЕМЯ первыми октетами из тысячи
	 */
	ASSERT_EQ(digest(build(COUNT)), abc::error_t::NONE);
}
/**
 * @brief Проверка предела длины значения, собираемого кусками
 *
 * @details Предел на строку и на двоичные данные заведён ради ПАМЯТИ, и сличается он с
 * суммою кусков, а не со всяким куском порознь: сличай он куски порознь - предел
 * обходился бы дроблением, и значение любой длины проходило бы кусками по одному октету
 *
 * @note Обход этот молчалив вдвойне: отказа не подаётся, а память растёт кусок за
 * куском, - то есть предел, заведённый ради неё, оборачивается своею
 * противоположностью. Поверялся прежде лишь предел у значения ЦЕЛЬНОГО
 *
 */
TEST(CodecAbcReader, ChunkedValueRespectsLimit){
	/**
	 * @brief Работа разбора значения, собираемого кусками объявленного числа
	 *
	 * @param text     признак того, что значение является строкой
	 * @param pieces   количество кусков значения
	 * @param width    ширина куска значения в октетах
	 * @param limit    предел длины значения
	 * @return         код отказа разбора
	 */
	auto digest = [](const bool text, const size_t pieces, const size_t width,
	 const uint64_t limit) noexcept -> abc::error_t {
		// Крупный вид собираемого значения
		const abc::group_t group = (text ? abc::group_t::STRING : abc::group_t::BLOB);
		// Собираемая запись значения
		vector <uint8_t> data;
		// Выполняем укладку метки значения неопределённой длины
		abc::mark(data, group, 0x1F);
		/**
		 * Выполняем укладку всех кусков значения
		 */
		for(size_t piece = 0; piece < pieces; piece++){
			// Выполняем укладку метки очередного куска значения
			abc::put(data, group, static_cast <uint64_t> (width));
			/**
			 * Выполняем укладку октетов очередного куска значения
			 */
			for(size_t i = 0; i < width; i++)
				// Выполняем укладку очередного октета куска значения
				data.push_back(static_cast <uint8_t> ('a'));
		}
		// Выполняем укладку метки конца значения
		abc::mark(data, abc::group_t::SINGLE, static_cast <uint8_t> (0x1F));
		// Читатель бинарной записи
		abc::reader_t reader(::logger());
		// Настройки разбора записи
		abc::reader_t::settings_t settings;
		// Выполняем установку предела длины строкового значения
		settings.maxString = (text ? limit : 0);
		// Выполняем установку предела длины двоичного значения
		settings.maxBlob = (text ? 0 : limit);
		// Выполняем установку настроек разбора записи
		reader.settings(settings);
		// Если подача записи разборщику отвечена отказом
		if(!reader.feed(data.data(), data.size(), true))
			// Выводим код отказа разбора
			return reader.error();
		// Сообщаем, что запись разобрана
		return abc::error_t::NONE;
	};
	/**
	 * Выполняем перебор обоих видов значения, собираемого кусками
	 */
	for(const bool text : {true, false}){
		// Код отказа, отвечающий виду собираемого значения
		const abc::error_t refusal = (text ? abc::error_t::STRING_TOO_LONG : abc::error_t::BLOB_TOO_LONG);
		/**
		 * Сумма кусков в пределе - разбор отвечает согласием
		 */
		ASSERT_EQ(digest(text, 2, 4, 10), abc::error_t::NONE) << "строка: " << text;
		/**
		 * Сумма кусков ровно по пределу - разбор отвечает согласием
		 */
		ASSERT_EQ(digest(text, 5, 2, 10), abc::error_t::NONE) << "строка: " << text;
		/**
		 * Сумма кусков за пределом - разбор отвечает отказом
		 *
		 * @note Всякий кусок в предел УМЕЩАЕТСЯ: сличай разбор куски порознь, запись эта
		 * прошла бы, и предел обходился бы дроблением
		 */
		ASSERT_EQ(digest(text, 4, 4, 10), refusal) << "строка: " << text;
		/**
		 * Дробление до одного октета предела не обходит
		 */
		ASSERT_EQ(digest(text, 11, 1, 10), refusal) << "строка: " << text;
		/**
		 * Предел снят - сумма кусков любой длины принимается
		 */
		ASSERT_EQ(digest(text, 40, 4, 0), abc::error_t::NONE) << "строка: " << text;
	}
}
TEST(CodecAbcReader, MalformedRules){
	/**
	 * Работа разбора записи затребованным правилом обращения с негодной строкой
	 *
	 * @param record разбираемая запись
	 * @param rule   правило обращения с негодной строкой
	 * @param data   снятое содержимое строки
	 * @param patched признак того, что содержимое было исправлено
	 * @return       код отказа разбора
	 */
	auto digest = [](const vector <uint8_t> & record, const abc::malformed_t rule,
	 string & data, bool & patched) noexcept -> abc::error_t {
		// Разборщик бинарной записи
		abc::reader_t reader(::logger());
		// Выполняем получение настроек разбора
		abc::reader_t::settings_t settings = reader.settings();
		// Выполняем установку правила обращения с негодной строкой
		settings.malformed = rule;
		// Выполняем установку настроек разбора
		reader.settings(settings);
		// Выполняем сброс снятого содержимого строки
		data.clear();
		// Выполняем сброс признака исправления содержимого
		patched = false;
		// Если подача записи разборщику отвечена отказом
		if(!reader.feed(record.data(), record.size(), true))
			// Выводим код отказа разбора
			return reader.error();
		/**
		 * Выполняем снятие всех событий разбора
		 */
		while(reader.next()){
			// Если событием является строковое значение
			if(reader.event() == abc::event_t::STRING){
				// Выполняем снятие содержимого строкового значения
				data.assign(reader.value().data);
				// Выполняем снятие признака исправления содержимого
				patched = reader.value().repaired;
			}
		}
		// Сообщаем, что запись разобрана
		return abc::error_t::NONE;
	};
	/**
	 * Строка из трёх октетов, где стоит усечённая четырёхоктетная точка:
	 * метка строки длиною 4, знак «a», обрывок точки и знак «b»
	 */
	const vector <uint8_t> record = {0x44, 'a', 0xF0, 0x9F, 'b'};
	// Снятое содержимое строки
	string data;
	// Признак того, что содержимое строки было исправлено
	bool patched = false;
	/**
	 * Правилом `REFUSE` запись отвергается кодом негодной кодировки
	 */
	ASSERT_EQ(digest(record, abc::malformed_t::REFUSE, data, patched), abc::error_t::INVALID_ENCODING);
	/**
	 * Правилом `PASS` октеты выдаются как есть, ничем не тронутые
	 */
	ASSERT_EQ(digest(record, abc::malformed_t::PASS, data, patched), abc::error_t::NONE);
	// Выполняем проверку того, что содержимое выдано неизменным
	ASSERT_EQ(data, string("a\xF0\x9F" "b"));
	// Выполняем проверку того, что признак исправления не поднят
	ASSERT_FALSE(patched);
	/**
	 * Правилом `REPLACE` обрывок точки подменяется ОДНИМ знаком замены
	 *
	 * @note Обрывок этот занимает ДВА октета, а знак замены - три, и содержимое тем самым
	 * длиннее исходного: оттого оно и лежит в отдельном хранилище
	 */
	ASSERT_EQ(digest(record, abc::malformed_t::REPLACE, data, patched), abc::error_t::NONE);
	// Выполняем проверку того, что обрывок подменён одним знаком замены
	ASSERT_EQ(data, string("a\xEF\xBF\xBD" "b"));
	// Выполняем проверку того, что признак исправления поднят
	ASSERT_TRUE(patched);
	/**
	 * Строка, кодировке отвечающая, правилом `REPLACE` не трогается вовсе
	 */
	{
		// Строка «ab» из двух годных знаков
		const vector <uint8_t> sound = {0x42, 'a', 'b'};
		// Выполняем проверку того, что годная строка разобрана без отказа
		ASSERT_EQ(digest(sound, abc::malformed_t::REPLACE, data, patched), abc::error_t::NONE);
		// Выполняем проверку того, что содержимое выдано неизменным
		ASSERT_EQ(data, string("ab"));
		// Выполняем проверку того, что признак исправления не поднят
		ASSERT_FALSE(patched);
	}
	/**
	 * ДВЕ негодные строки в одной записи исправляются обе, не затирая друг друга
	 *
	 * @details Случай этот существен, а не полон: события копятся ОЧЕРЕДЬЮ и снимаются
	 * потребителем позже, когда разбор ушёл уже вперёд. Держи хранилище исправленных
	 * строк одну строку, вторая затёрла бы первую, ещё не выданную, - и первая выдала бы
	 * содержимое второй. Первая сборка правила этим и страдала
	 */
	{
		// Массив из двух строк, каждая с усечённой точкой своего вида
		const vector <uint8_t> pair = {
			0x82,
			0x43, 'a', 0xF0, 0x9F,
			0x43, 0xE1, 0x80, 'b'
		};
		// Разборщик бинарной записи
		abc::reader_t reader(::logger());
		// Выполняем получение настроек разбора
		abc::reader_t::settings_t settings = reader.settings();
		// Выполняем установку правила подмены негодной последовательности
		settings.malformed = abc::malformed_t::REPLACE;
		// Выполняем установку настроек разбора
		reader.settings(settings);
		// Выполняем подачу записи разборщику целиком
		ASSERT_TRUE(reader.feed(pair.data(), pair.size(), true));
		// Снятые содержимые строк записи
		vector <string> values;
		/**
		 * Выполняем снятие всех событий разбора
		 */
		while(reader.next()){
			// Если событием является строковое значение
			if(reader.event() == abc::event_t::STRING){
				// Выполняем проверку того, что признак исправления поднят
				ASSERT_TRUE(reader.value().repaired);
				// Выполняем учёт снятого содержимого строки
				values.push_back(string(reader.value().data));
			}
		}
		// Выполняем проверку того, что снято ровно две строки
		ASSERT_EQ(values.size(), 2u);
		// Выполняем проверку содержимого первой исправленной строки
		ASSERT_EQ(values.at(0), string("a\xEF\xBF\xBD"));
		// Выполняем проверку содержимого второй исправленной строки
		ASSERT_EQ(values.at(1), string("\xEF\xBF\xBD" "b"));
	}
	/**
	 * Двоичное значение правилом не трогается вовсе: кодировка объявлена лишь строке
	 */
	{
		// Двоичное значение из тех же октетов, что и негодная строка
		const vector <uint8_t> blob = {0x64, 'a', 0xF0, 0x9F, 'b'};
		// Выполняем проверку того, что двоичное значение разобрано без отказа
		ASSERT_EQ(digest(blob, abc::malformed_t::REFUSE, data, patched), abc::error_t::NONE);
	}
}
/**
 * @brief Проверка отказа на длину значения, отрезку события не вмещающуюся
 *
 * @details Отрезок события несёт смещение и длину ТРИДЦАТЬЮ ДВУМЯ разрядами: события
 *          копятся очередью, и восемьдесят разрядов на событие стоили бы памяти на всякой
 *          записи ради невиданного. Но длина, за предел выходящая, ложилась бы в отрезок
 *          усечённой МОЛЧА - событие вышло бы годным по виду, а содержимое его оборванным
 *
 * @note Проверка эта СТОИТ ДЁШЕВО, хотя стережёт величины в гигабайты: отказ приходит по
 *       ОБЪЯВЛЕННОЙ длине, а не по поданным октетам, и подавать четыре гигабайта незачем.
 *       Тем она и отличается от сторожа хранилища дерева, какой проверкою набора не
 *       закрепить: там отказ вырабатывается лишь настоящим содержимым
 *
 * @note Возможности записать значение длиннее четырёх гигабайт отказ НЕ отнимает: такое
 *       значение кладётся кусками, и проверка ниже закрепляет, что путь этот открыт
 *
 */
TEST(CodecAbcReader, SpanWidthLimit){
	/**
	 * Работа разбора записи с объявленной длиной значения
	 *
	 * @param lead   ведущий октет значения
	 * @param length объявляемая длина значения
	 * @return       код отказа разбора
	 */
	auto digest = [](const uint8_t lead, const uint64_t length) noexcept -> abc::error_t {
		// Собираемая запись
		vector <uint8_t> record;
		/**
		 * Выполняем укладку ведущего октета значения с объявлением длины восемью октетами
		 *
		 * @note Ширина метки берётся наибольшая намеренно: объявить четыре гигабайта с
		 * лишком иначе нельзя, а именно это и поверяется
		 */
		record.push_back(static_cast <uint8_t> (lead | 0x1B));
		/**
		 * Выполняем перебор всех октетов объявляемой длины
		 *
		 * @note Октеты кладутся МЛАДШИМ ВПЕРЁД, как то и делает запись: первая сборка
		 * проверки клала их старшим вперёд, и длина `0xFFFFFFFF` обращалась в
		 * `0xFFFFFFFF00000000` - проверка краснела на своей же ошибке, а не на кодеке
		 */
		for(uint8_t i = 0; i < 8; i++)
			// Выполняем укладку очередного октета длины младшим вперёд
			record.push_back(static_cast <uint8_t> ((length >> (i * 8)) & 0xFF));
		// Разборщик бинарной записи
		abc::reader_t reader(::logger());
		/**
		 * Выполняем подачу записи НЕОКОНЧЕННОЙ намеренно
		 *
		 * @note Поданной окончательно она отвергалась бы нехваткой октетов значения - тем
		 * же кодом, каким отвечает и сторож ширины отрезка, - и два случая стали бы
		 * неотличимы. Неоконченная подача разделяет их: длина, отрезку вмещающаяся, велит
		 * ждать октетов, а не вмещающаяся отвергается НЕМЕДЛЯ, октетов не дожидаясь
		 */
		if(!reader.feed(record.data(), record.size(), false))
			// Выводим код отказа разбора
			return reader.error();
		// Сообщаем, что запись разобрана без отказа
		return abc::error_t::NONE;
	};
	// Ведущий октет строкового значения
	const uint8_t text = static_cast <uint8_t> (static_cast <uint8_t> (abc::group_t::STRING) << 5);
	// Ведущий октет двоичного значения
	const uint8_t blob = static_cast <uint8_t> (static_cast <uint8_t> (abc::group_t::BLOB) << 5);
	/**
	 * Длина, отрезку события ровно вмещающаяся, отказа не вызывает: разбор ждёт октетов
	 */
	ASSERT_EQ(digest(text, static_cast <uint64_t> (numeric_limits <uint32_t>::max())),
	 abc::error_t::NONE);
	ASSERT_EQ(digest(blob, static_cast <uint64_t> (numeric_limits <uint32_t>::max())),
	 abc::error_t::NONE);
	/**
	 * Длина на единицу шире отрезка события отвергается ею самой
	 */
	ASSERT_EQ(digest(text, static_cast <uint64_t> (numeric_limits <uint32_t>::max()) + 1),
	 abc::error_t::INVALID_LENGTH);
	ASSERT_EQ(digest(blob, static_cast <uint64_t> (numeric_limits <uint32_t>::max()) + 1),
	 abc::error_t::INVALID_LENGTH);
	/**
	 * Длина, объявленная пределом разрядной сетки, отвергается ею же
	 */
	ASSERT_EQ(digest(text, numeric_limits <uint64_t>::max()), abc::error_t::INVALID_LENGTH);
	ASSERT_EQ(digest(blob, numeric_limits <uint64_t>::max()), abc::error_t::INVALID_LENGTH);
	/**
	 * Работа разбора величины неограниченной ширины с объявленной длиной октетов
	 *
	 * @param subtype подвид расширения: целое любой ширины либо десятичное
	 * @param length  объявляемая длина октетов величины
	 * @return        код отказа разбора
	 */
	auto extend = [](const abc::extend_t subtype, const uint64_t length) noexcept -> abc::error_t {
		// Собираемая запись
		vector <uint8_t> record;
		// Выполняем укладку метки расширения затребованного подвида
		record.push_back(static_cast <uint8_t> ((static_cast <uint8_t> (abc::group_t::EXTEND) << 5) |
		 static_cast <uint8_t> (subtype)));
		/**
		 * Если расширение является десятичным, впереди длины стоит порядок величины
		 */
		if(subtype == abc::extend_t::DECIMAL)
			// Выполняем укладку нулевого десятичного порядка величины
			record.push_back(0x00);
		// Выполняем укладку метки длины октетов величины шириною в восемь октетов
		record.push_back(static_cast <uint8_t> ((static_cast <uint8_t> (abc::group_t::UNSIGNED) << 5) | 0x1B));
		// Выполняем перебор всех октетов объявляемой длины
		for(uint8_t i = 0; i < 8; i++)
			// Выполняем укладку очередного октета длины младшим вперёд
			record.push_back(static_cast <uint8_t> ((length >> (i * 8)) & 0xFF));
		// Разборщик бинарной записи
		abc::reader_t reader(::logger());
		// Выполняем подачу записи разборщику неоконченной
		if(!reader.feed(record.data(), record.size(), false))
			// Выводим код отказа разбора
			return reader.error();
		// Сообщаем, что запись разобрана без отказа
		return abc::error_t::NONE;
	};
	/**
	 * Длина октетов величины, отрезку события ровно вмещающаяся, отказа не вызывает
	 */
	ASSERT_EQ(extend(abc::extend_t::BIGNUM, static_cast <uint64_t> (numeric_limits <uint32_t>::max())),
	 abc::error_t::NONE);
	/**
	 * Длина октетов величины шире отрезка события отвергается им
	 *
	 * @note Величина неограниченной ширины кусками НЕ кладётся, оттого предел этот ей
	 * настоящий - но величина в четыре гигабайта есть число о тридцати четырёх миллиардах
	 * разрядов, и тесным предел назвать нельзя
	 */
	ASSERT_EQ(extend(abc::extend_t::BIGNUM, static_cast <uint64_t> (numeric_limits <uint32_t>::max()) + 1),
	 abc::error_t::INVALID_LENGTH);
	// Выполняем проверку того же у десятичного расширения
	ASSERT_EQ(extend(abc::extend_t::DECIMAL, static_cast <uint64_t> (numeric_limits <uint32_t>::max()) + 1),
	 abc::error_t::INVALID_LENGTH);
}
/**
 * @brief Проверка сторожа ширины поля смещения отрезка события
 *
 * @details Отрезок события несёт смещение в буфер ТРИДЦАТЬЮ ДВУМЯ разрядами, а усечение
 *          разобранной части возможно лишь при пустой очереди событий - отрезки ссылаются
 *          в буфер смещением. Потребитель, подающий без снятия событий, растил буфер
 *          безгранично, и за четырьмя гигабайтами смещение ложилось УСЕЧЁННЫМ молча:
 *          число событий сходилось, отказа не было, а содержимое отдавалось ЧУЖОЕ
 *
 * @note Проверка ведётся дорогой ВЫДАЧИ МЕСТА, а не подачи копированием, единственно ради
 *       цены: сторож у выдачи стоит ПРЕЖДЕ отведения памяти, и запрос непомерного места
 *       отвечается отказом, не заведя ни октета. Дорога подачи копированием доказана
 *       замером - 4.10 ГиБ в 67134 записях, - но проверкою не закрепима: четырёх гигабайт
 *       памяти нет у половины стендов
 *
 * @note Отказ этот - обратное давление, а не поломка, и вторая половина проверки о том:
 *       разбор после отказа цел, и подача обыкновенной записи продолжается
 */
TEST(CodecAbcReader, BufferSpanWidthGuard){
	// Разбиратель записей контейнера
	abc::reader_t reader(::logger());
	/**
	 * Выполняем проверку того, что место шире поля смещения не выдаётся
	 */
	{
		// Запрашиваемый размер места, полю смещения не отвечающий
		const size_t width = (static_cast <size_t> (numeric_limits <uint32_t>::max()) + 1);
		// Места такой ширины выдаваться не должно
		ASSERT_EQ(reader.reserve(width), nullptr);
		// Отказ обязан быть объявлен превышением предела
		ASSERT_EQ(reader.error(), abc::error_t::OVERFLOW_LIMIT);
	}
	/**
	 * Выполняем проверку того, что разбор отказом НЕ испорчен
	 *
	 * @note Половина эта отделяет обратное давление от поломки: разбиратель, отказом
	 * испорченный, отвечал бы отказом и обыкновенной записи вслед за нею
	 */
	{
		// Буфер собираемой записи
		vector <uint8_t> record;
		// Выполняем укладку записи целого числа
		abc::integer(record, 7);
		// Запись обязана податься разбору
		ASSERT_TRUE(reader.feed(record.data(), record.size(), true))
			<< "код отказа: " << abc::message(reader.error());
		// Признак того, что число разбором выдано
		bool given = false;
		/**
		 * Выполняем перебор собранных разбором событий
		 */
		while(reader.next()){
			// Если событие несёт число
			if(reader.event() == abc::event_t::NUMBER){
				// Извлечённое число обязано отвечать уложенному
				ASSERT_EQ(reader.value().number, static_cast <uint64_t> (7));
				// Выполняем объявление числа выданным
				given = true;
			}
		}
		// Число обязано быть выдано разбором
		ASSERT_TRUE(given);
	}
}
