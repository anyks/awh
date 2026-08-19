/**
 * @file writer.cpp
 * @date 2026-08-18
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Проверки сборки записи бинарного контейнера ABC — учёт вместимых, строгий вид
 *        записи и круговой обход собранного через чтение
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
#include <codec/abc/writer.hpp>
#include <codec/abc/reader.hpp>

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
	 * @brief Функция получения краткой записи собранных событий разбора
	 *
	 * @param data   разбираемая запись
	 * @param reader читатель бинарной записи
	 * @return       собранная последовательность событий разбора
	 *
	 */
	vector <string> replay(const vector <uint8_t> & data, abc::reader_t & reader) noexcept {
		// Собираемая последовательность событий разбора
		vector <string> result;
		// Если подача записи отвечена отказом
		if(!reader.feed(data.data(), data.size(), true))
			// Выводим собранную последовательность событий
			return result;
		/**
		 * Выполняем снятие всех собранных событий разбора
		 */
		while(reader.next()){
			// Выполняем получение значения текущего события
			const abc::reader_t::value_t value = reader.value();
			/**
			 * Определяем вид текущего события разбора
			 */
			switch(static_cast <uint8_t> (reader.event())){
				// Если событием является пустое значение
				case static_cast <uint8_t> (abc::event_t::NUL): result.push_back("null"); break;
				// Если событием является логическое значение
				case static_cast <uint8_t> (abc::event_t::BOOL): result.push_back(value.boolean ? "true" : "false"); break;
				// Если событием является начало массива
				case static_cast <uint8_t> (abc::event_t::ARRAY_BEGIN): result.push_back("["); break;
				// Если событием является конец массива
				case static_cast <uint8_t> (abc::event_t::ARRAY_END): result.push_back("]"); break;
				// Если событием является начало отображения
				case static_cast <uint8_t> (abc::event_t::MAP_BEGIN): result.push_back("{"); break;
				// Если событием является конец отображения
				case static_cast <uint8_t> (abc::event_t::MAP_END): result.push_back("}"); break;
				// Если событием является завершённый документ
				case static_cast <uint8_t> (abc::event_t::DOCUMENT): result.push_back(";"); break;
				// Если событием является окончание записи
				case static_cast <uint8_t> (abc::event_t::FINISH): result.push_back("."); break;
				// Если событием является отметка времени
				case static_cast <uint8_t> (abc::event_t::TIME): result.push_back("T" + to_string(value.integer)); break;
				// Если событием является опознаватель
				case static_cast <uint8_t> (abc::event_t::UUID): result.push_back("U" + to_string(value.data.size())); break;
				// Если событием являются двоичные данные
				case static_cast <uint8_t> (abc::event_t::BLOB): result.push_back("B" + to_string(value.data.size())); break;
				// Если событием является имя поля отображения
				case static_cast <uint8_t> (abc::event_t::KEY): result.push_back("K:" + string(value.data)); break;
				// Если событием является строка
				case static_cast <uint8_t> (abc::event_t::STRING): result.push_back("S:" + string(value.data)); break;
				// Если событием является число
				case static_cast <uint8_t> (abc::event_t::NUMBER): {
					// Если число является целым без знака
					if(static_cast <uint32_t> (value.type) & static_cast <uint32_t> (abc::type_t::UNSIGNED))
						// Выполняем добавление целого без знака
						result.push_back("N" + to_string(value.number));
					// Если число является целым со знаком
					else if(static_cast <uint32_t> (value.type) & static_cast <uint32_t> (abc::type_t::SIGNED))
						// Выполняем добавление целого со знаком
						result.push_back("N" + to_string(value.integer));
					// Если число является дробным
					else if(static_cast <uint32_t> (value.type) & static_cast <uint32_t> (abc::type_t::REAL))
						// Выполняем добавление дробного числа
						result.push_back("R" + to_string(value.real));
					// Если число является десятичным
					else if(value.type == abc::type_t::DECIMAL)
						// Выполняем добавление десятичного числа
						result.push_back("D" + string(value.negative ? "-" : "+") +
						 to_string(value.data.size()) + "e" + to_string(value.exponent));
					// Выполняем добавление целого неограниченной ширины
					else result.push_back("X" + string(value.negative ? "-" : "+") + to_string(value.data.size()));
				} break;
			}
		}
		// Выводим собранную последовательность событий
		return result;
	}
};

/**
 * @brief Проверка кругового обхода сборки и чтения
 *
 * @details Собранная запись обязана читаться своим же разбором, и выдача его обязана
 * совпадать с тем, что было уложено
 *
 */
TEST(CodecAbcWriter, Roundtrip) {
	// Сборщик бинарной записи
	abc::writer_t writer;
	// Октеты опознавателя
	const vector <uint8_t> identifier(16, 0x5A);
	// Октеты двоичного значения
	const vector <uint8_t> binary = {0x01, 0x02, 0x03};
	// Выполняем укладку отображения из шести пар
	ASSERT_TRUE(writer.mapBegin(6));
	// Выполняем укладку имени поля пустого значения
	ASSERT_TRUE(writer.text("пусто"));
	// Выполняем укладку пустого значения
	ASSERT_TRUE(writer.nul());
	// Выполняем укладку имени поля логического значения
	ASSERT_TRUE(writer.text("да"));
	// Выполняем укладку логического значения
	ASSERT_TRUE(writer.boolean(true));
	// Выполняем укладку имени поля числа
	ASSERT_TRUE(writer.text("число"));
	// Выполняем укладку целого числа со знаком
	ASSERT_TRUE(writer.number(static_cast <int64_t> (-70000)));
	// Выполняем укладку имени поля двоичных данных
	ASSERT_TRUE(writer.text("данные"));
	// Выполняем укладку двоичных данных
	ASSERT_TRUE(writer.blob(binary.data(), binary.size()));
	// Выполняем укладку имени поля опознавателя
	ASSERT_TRUE(writer.text("метка"));
	// Выполняем укладку опознавателя
	ASSERT_TRUE(writer.uuid(identifier.data(), identifier.size()));
	// Выполняем укладку имени поля массива
	ASSERT_TRUE(writer.text("список"));
	// Выполняем укладку массива из трёх значений
	ASSERT_TRUE(writer.arrayBegin(3));
	// Выполняем укладку строки
	ASSERT_TRUE(writer.text("строка"));
	// Выполняем укладку отметки времени
	ASSERT_TRUE(writer.timestamp(1755500000));
	// Выполняем укладку дробного числа двойной точности
	ASSERT_TRUE(writer.number(0.5));
	// Выполняем укладку конца массива
	ASSERT_TRUE(writer.arrayEnd());
	// Выполняем укладку конца отображения
	ASSERT_TRUE(writer.mapEnd());
	// Выполняем проверку завершённости собранной записи
	ASSERT_TRUE(writer.complete()) << "код отказа: " << abc::message(writer.error());
	// Читатель бинарной записи
	abc::reader_t reader;
	// Выполняем разбор собранной записи
	const vector <string> events = replay(writer.record(), reader);
	// Выполняем проверку собранной последовательности событий
	ASSERT_EQ(events, (vector <string> {"{", "K:пусто", "null", "K:да", "true", "K:число",
	 "N-70000", "K:данные", "B3", "K:метка", "U16", "K:список", "[", "S:строка",
	 "T1755500000", "R0.500000", "]", "}", ";", "."})) << "код отказа: " << abc::message(reader.error());
}
/**
 * @brief Проверка кругового обхода вместимых неопределённой длины
 *
 */
TEST(CodecAbcWriter, IndefiniteRoundtrip) {
	// Сборщик бинарной записи
	abc::writer_t writer;
	// Выполняем укладку массива неопределённой длины
	ASSERT_TRUE(writer.arrayBegin());
	// Выполняем укладку отображения неопределённой длины
	ASSERT_TRUE(writer.mapBegin());
	// Выполняем укладку имени поля отображения
	ASSERT_TRUE(writer.text("ключ"));
	// Выполняем укладку значения поля отображения
	ASSERT_TRUE(writer.number(static_cast <uint64_t> (5)));
	// Выполняем укладку конца отображения
	ASSERT_TRUE(writer.mapEnd());
	// Выполняем укладку конца массива
	ASSERT_TRUE(writer.arrayEnd());
	// Выполняем проверку завершённости собранной записи
	ASSERT_TRUE(writer.complete()) << "код отказа: " << abc::message(writer.error());
	// Читатель бинарной записи
	abc::reader_t reader;
	// Выполняем разбор собранной записи
	const vector <string> events = replay(writer.record(), reader);
	// Выполняем проверку собранной последовательности событий
	ASSERT_EQ(events, (vector <string> {"[", "{", "K:ключ", "N5", "}", "]", ";", "."}))
		<< "код отказа: " << abc::message(reader.error());
}
/**
 * @brief Проверка кругового обхода расширений
 *
 * @details Порядок, равный нулю, укладывается целым неограниченной ширины: значение у
 * них одно и то же, а строгий вид записи обязывает одному значению отвечать одной записи
 *
 */
TEST(CodecAbcWriter, ExtensionRoundtrip) {
	// Октеты величины числа
	const vector <uint8_t> magnitude = {0x39, 0x30, 0x01};
	// Сборщик бинарной записи
	abc::writer_t writer;
	// Выполняем укладку массива из трёх значений
	ASSERT_TRUE(writer.arrayBegin(3));
	// Выполняем укладку целого числа неограниченной ширины
	ASSERT_TRUE(writer.bignum(magnitude.data(), magnitude.size(), true));
	// Выполняем укладку десятичного числа
	ASSERT_TRUE(writer.decimal(magnitude.data(), magnitude.size(), false, -3));
	// Выполняем укладку десятичного числа с нулевым порядком
	ASSERT_TRUE(writer.decimal(magnitude.data(), magnitude.size(), false, 0));
	// Выполняем укладку конца массива
	ASSERT_TRUE(writer.arrayEnd());
	// Читатель бинарной записи
	abc::reader_t reader;
	// Выполняем разбор собранной записи
	const vector <string> events = replay(writer.record(), reader);
	// Выполняем проверку собранной последовательности событий
	ASSERT_EQ(events, (vector <string> {"[", "X-3", "D+3e-3", "X+3", "]", ";", "."}))
		<< "код отказа: " << abc::message(reader.error());
}
/**
 * @brief Проверка учёта вместимых сборкой
 *
 * @details Учёт ведётся сборкой, а не оставляется на разбор: запись, собранная неверно,
 * до разбора вовсе не доходит
 *
 */
TEST(CodecAbcWriter, ContainerAccounting) {
	// Выполняем проверку отказа на значение сверх объявленной длины
	{
		// Сборщик бинарной записи
		abc::writer_t writer;
		// Выполняем укладку массива из одного значения
		ASSERT_TRUE(writer.arrayBegin(1));
		// Выполняем укладку первого значения массива
		ASSERT_TRUE(writer.nul());
		// Выполняем проверку отказа на укладку значения сверх объявленной длины
		ASSERT_FALSE(writer.nul());
		// Выполняем проверку кода отказа
		ASSERT_EQ(writer.error(), abc::error_t::CONTAINER_OVERFLOW);
	}
	// Выполняем проверку отказа на вместимое, закрытое недособранным
	{
		// Сборщик бинарной записи
		abc::writer_t writer;
		// Выполняем укладку массива из двух значений
		ASSERT_TRUE(writer.arrayBegin(2));
		// Выполняем укладку первого значения массива
		ASSERT_TRUE(writer.nul());
		// Выполняем проверку отказа на закрытие недособранного массива
		ASSERT_FALSE(writer.arrayEnd());
		// Выполняем проверку кода отказа
		ASSERT_EQ(writer.error(), abc::error_t::UNBALANCED_CONTAINER);
	}
	// Выполняем проверку отказа на закрытие вместимого иного вида
	{
		// Сборщик бинарной записи
		abc::writer_t writer;
		// Выполняем укладку массива из нуля значений
		ASSERT_TRUE(writer.arrayBegin(0));
		// Выполняем проверку отказа на закрытие массива концом отображения
		ASSERT_FALSE(writer.mapEnd());
		// Выполняем проверку кода отказа
		ASSERT_EQ(writer.error(), abc::error_t::UNBALANCED_CONTAINER);
	}
	// Выполняем проверку отказа на конец вместимого вне вместимого
	{
		// Сборщик бинарной записи
		abc::writer_t writer;
		// Выполняем проверку отказа на закрытие вместимого, какого нет
		ASSERT_FALSE(writer.arrayEnd());
		// Выполняем проверку кода отказа
		ASSERT_EQ(writer.error(), abc::error_t::UNBALANCED_CONTAINER);
	}
	// Выполняем проверку отказа на отображение, закрытое на имени поля
	{
		// Сборщик бинарной записи
		abc::writer_t writer;
		// Выполняем укладку отображения неопределённой длины
		ASSERT_TRUE(writer.mapBegin());
		// Выполняем укладку имени поля отображения
		ASSERT_TRUE(writer.text("ключ"));
		// Выполняем проверку отказа на закрытие отображения без значения
		ASSERT_FALSE(writer.mapEnd());
		// Выполняем проверку кода отказа
		ASSERT_EQ(writer.error(), abc::error_t::MISSING_VALUE);
	}
	// Выполняем проверку отказа на вместимое именем поля отображения
	{
		// Сборщик бинарной записи
		abc::writer_t writer;
		// Выполняем укладку отображения из одной пары
		ASSERT_TRUE(writer.mapBegin(1));
		// Выполняем проверку отказа на укладку массива именем поля
		ASSERT_FALSE(writer.arrayBegin(0));
		// Выполняем проверку кода отказа
		ASSERT_EQ(writer.error(), abc::error_t::INVALID_KEY);
	}
	// Выполняем проверку незавершённости недособранной записи
	{
		// Сборщик бинарной записи
		abc::writer_t writer;
		// Выполняем укладку массива из одного значения
		ASSERT_TRUE(writer.arrayBegin(1));
		// Выполняем укладку значения массива
		ASSERT_TRUE(writer.nul());
		// Выполняем проверку незавершённости собранной записи
		ASSERT_FALSE(writer.complete());
		// Выполняем проверку глубины вложенности сборки
		ASSERT_EQ(writer.depth(), 1u);
	}
}
/**
 * @brief Проверка отказа на повторяющееся имя поля отображения
 *
 */
TEST(CodecAbcWriter, DuplicateKey) {
	// Сборщик бинарной записи
	abc::writer_t writer;
	// Выполняем укладку отображения из двух пар
	ASSERT_TRUE(writer.mapBegin(2));
	// Выполняем укладку имени первого поля отображения
	ASSERT_TRUE(writer.text("ключ"));
	// Выполняем укладку значения первого поля отображения
	ASSERT_TRUE(writer.nul());
	// Выполняем проверку отказа на повторяющееся имя поля
	ASSERT_FALSE(writer.text("ключ"));
	// Выполняем проверку кода отказа
	ASSERT_EQ(writer.error(), abc::error_t::DUPLICATE_KEY);
}
/**
 * @brief Проверка строгого вида записи
 *
 * @details Строгий вид нужен там, где запись подписывается: одинаковое содержимое
 * обязано давать одинаковые октеты, иначе подпись при пересборке перестаёт совпадать
 *
 */
TEST(CodecAbcWriter, Canonical) {
	// Настройки сборки записи строгого вида
	abc::writer_t::settings_t settings;
	// Выполняем установку признака строгого вида записи
	settings.canonical = true;
	// Выполняем проверку отказа на неопределённую длину вместимого
	{
		// Сборщик бинарной записи
		abc::writer_t writer;
		// Выполняем установку настроек сборки записи
		writer.settings(settings);
		// Выполняем проверку отказа на укладку массива неопределённой длины
		ASSERT_FALSE(writer.arrayBegin());
		// Выполняем проверку кода отказа
		ASSERT_EQ(writer.error(), abc::error_t::INDEFINITE_REFUSED);
	}
	// Выполняем проверку отказа на имена полей не по возрастанию
	{
		// Сборщик бинарной записи
		abc::writer_t writer;
		// Выполняем установку настроек сборки записи
		writer.settings(settings);
		// Выполняем укладку отображения из двух пар
		ASSERT_TRUE(writer.mapBegin(2));
		// Выполняем укладку имени первого поля отображения
		ASSERT_TRUE(writer.text("яблоко"));
		// Выполняем укладку значения первого поля отображения
		ASSERT_TRUE(writer.nul());
		// Выполняем проверку отказа на имя поля, стоящее ниже прежнего
		ASSERT_FALSE(writer.text("арбуз"));
		// Выполняем проверку кода отказа
		ASSERT_EQ(writer.error(), abc::error_t::UNORDERED_KEY);
	}
	// Выполняем проверку приёма имён полей по возрастанию
	{
		// Сборщик бинарной записи
		abc::writer_t writer;
		// Выполняем установку настроек сборки записи
		writer.settings(settings);
		// Выполняем укладку отображения из двух пар
		ASSERT_TRUE(writer.mapBegin(2));
		// Выполняем укладку имени первого поля отображения
		ASSERT_TRUE(writer.text("арбуз"));
		// Выполняем укладку значения первого поля отображения
		ASSERT_TRUE(writer.nul());
		// Выполняем укладку имени второго поля отображения
		ASSERT_TRUE(writer.text("яблоко"));
		// Выполняем укладку значения второго поля отображения
		ASSERT_TRUE(writer.nul());
		// Выполняем укладку конца отображения
		ASSERT_TRUE(writer.mapEnd());
		// Выполняем проверку завершённости собранной записи
		ASSERT_TRUE(writer.complete()) << "код отказа: " << abc::message(writer.error());
	}
	// Выполняем проверку того, что имена сличаются записью, а не длиною
	{
		// Сборщик бинарной записи
		abc::writer_t writer;
		// Выполняем установку настроек сборки записи
		writer.settings(settings);
		// Выполняем укладку отображения из двух пар
		ASSERT_TRUE(writer.mapBegin(2));
		// Выполняем укладку имени первого поля отображения
		ASSERT_TRUE(writer.text("аб"));
		// Выполняем укладку значения первого поля отображения
		ASSERT_TRUE(writer.nul());
		// Выполняем укладку имени второго поля отображения, начинающегося тем же
		ASSERT_TRUE(writer.text("абв"));
		// Выполняем укладку значения второго поля отображения
		ASSERT_TRUE(writer.nul());
		// Выполняем проверку завершённости укладки
		ASSERT_TRUE(writer.mapEnd()) << "код отказа: " << abc::message(writer.error());
	}
}
/**
 * @brief Проверка отказов укладки значений
 *
 */
TEST(CodecAbcWriter, ValueFailures) {
	// Выполняем проверку отказа на строку, кодировке не отвечающую
	{
		// Сборщик бинарной записи
		abc::writer_t writer;
		// Выполняем проверку отказа на укладку негодной строки
		ASSERT_FALSE(writer.text(string_view("\xC0\x80", 2)));
		// Выполняем проверку кода отказа
		ASSERT_EQ(writer.error(), abc::error_t::INVALID_ENCODING);
	}
	// Выполняем проверку отказа на опознаватель недопустимой ширины
	{
		// Октеты опознавателя недопустимой ширины
		const vector <uint8_t> identifier(8, 0x11);
		// Сборщик бинарной записи
		abc::writer_t writer;
		// Выполняем проверку отказа на укладку опознавателя
		ASSERT_FALSE(writer.uuid(identifier.data(), identifier.size()));
		// Выполняем проверку кода отказа
		ASSERT_EQ(writer.error(), abc::error_t::INVALID_LENGTH);
	}
	// Выполняем проверку отказа на неканоническую запись величины
	{
		// Октеты величины с нулевым старшим октетом
		const vector <uint8_t> magnitude = {0x01, 0x00};
		// Сборщик бинарной записи
		abc::writer_t writer;
		// Выполняем проверку отказа на укладку числа неограниченной ширины
		ASSERT_FALSE(writer.bignum(magnitude.data(), magnitude.size(), false));
		// Выполняем проверку кода отказа
		ASSERT_EQ(writer.error(), abc::error_t::INVALID_BIGNUM);
	}
	// Выполняем проверку отказа на величину, объявленную отрицательным нулём
	{
		// Сборщик бинарной записи
		abc::writer_t writer;
		// Выполняем проверку отказа на укладку отрицательного нуля
		ASSERT_FALSE(writer.bignum(nullptr, 0, true));
		// Выполняем проверку кода отказа
		ASSERT_EQ(writer.error(), abc::error_t::INVALID_BIGNUM);
	}
	// Выполняем проверку того, что отказ сборки останавливает её насовсем
	{
		// Сборщик бинарной записи
		abc::writer_t writer;
		// Выполняем проверку отказа на укладку негодной строки
		ASSERT_FALSE(writer.text(string_view("\xFE", 1)));
		// Выполняем проверку отказа на укладку значения после отказа
		ASSERT_FALSE(writer.nul());
		// Выполняем проверку незавершённости собранной записи
		ASSERT_FALSE(writer.complete());
		// Выполняем сброс состояния сборки
		writer.reset();
		// Выполняем проверку укладки значения после сброса состояния
		ASSERT_TRUE(writer.nul());
		// Выполняем проверку завершённости собранной записи
		ASSERT_TRUE(writer.complete());
	}
}
/**
 * @brief Проверка наименьшей записи собранных значений
 *
 * @details Наименьшая запись обязана быть единственной: иначе одно и то же значение
 * записалось бы двумя видами, и подпись при пересборке перестала бы совпадать
 *
 */
TEST(CodecAbcWriter, SmallestRecord) {
	/**
	 * Проверяемые значения вместе с ожидаемой длиной записи
	 */
	const vector <pair <uint64_t, size_t>> values = {
		{0, 1}, {23, 1}, {24, 2}, {255, 2}, {256, 3}, {65535, 3}, {65536, 5},
		{4294967295ull, 5}, {4294967296ull, 9}, {numeric_limits <uint64_t>::max(), 9}
	};
	/**
	 * Выполняем перебор всех проверяемых значений
	 */
	for(const auto & item : values){
		// Сборщик бинарной записи
		abc::writer_t writer;
		// Выполняем укладку целого числа без знака
		ASSERT_TRUE(writer.number(item.first));
		// Выполняем проверку длины собранной записи
		ASSERT_EQ(writer.record().size(), item.second) << "значение: " << item.first;
	}
	// Выполняем проверку того, что пустое отображение занимает один октет
	{
		// Сборщик бинарной записи
		abc::writer_t writer;
		// Выполняем укладку пустого отображения
		ASSERT_TRUE(writer.mapBegin(0));
		// Выполняем укладку конца отображения
		ASSERT_TRUE(writer.mapEnd());
		// Выполняем проверку длины собранной записи
		ASSERT_EQ(writer.record().size(), 1u);
	}
}

/**
 * @brief Проверка строки, собираемой кусками
 *
 * @details Строка эта кладётся, когда длина её наперёд неизвестна: так большой текст
 *          уходит в запись потоком, не собираясь в памяти целиком
 *
 */
TEST(CodecAbcWriter, SegmentedString) {
	// Сборщик бинарной записи
	abc::writer_t writer;
	// Выполняем укладку начала строки, собираемой кусками
	ASSERT_TRUE(writer.textBegin()) << "код отказа: " << abc::message(writer.error());
	// Выполняем укладку первого куска строки
	ASSERT_TRUE(writer.text("первый кусок, "));
	// Выполняем укладку второго куска строки
	ASSERT_TRUE(writer.text("второй кусок"));
	// Выполняем укладку конца строки, собираемой кусками
	ASSERT_TRUE(writer.textEnd()) << "код отказа: " << abc::message(writer.error());
	// Выполняем проверку завершённости собранной записи
	ASSERT_TRUE(writer.complete());
	// Разбиратель бинарной записи
	abc::reader_t reader;
	// Выполняем подачу собранной записи разбирателю
	ASSERT_TRUE(reader.feed(writer.record().data(), writer.record().size(), true))
		<< "код отказа: " << abc::message(reader.error());
	// Собираемое содержимое строки
	string content;
	// Виды снятых событий разбора
	vector <abc::event_t> events;
	/**
	 * Выполняем снятие всех событий разбора
	 */
	while(reader.next()){
		// Выполняем запоминание вида снятого события
		events.push_back(reader.event());
		// Если событием является кусок строки
		if(reader.event() == abc::event_t::STRING)
			// Выполняем накопление содержимого куска
			content.append(reader.value().data);
	}
	// Выполняем проверку череды снятых событий
	ASSERT_EQ(events, (vector <abc::event_t> {
		abc::event_t::STRING_BEGIN, abc::event_t::STRING, abc::event_t::STRING,
		abc::event_t::STRING_END, abc::event_t::DOCUMENT, abc::event_t::FINISH
	}));
	// Выполняем проверку собранного содержимого строки
	ASSERT_EQ(content, "первый кусок, второй кусок");
}
/**
 * @brief Проверка отказов при сборке значения кусками
 *
 */
TEST(CodecAbcWriter, SegmentedRefusals) {
	/**
	 * Выполняем проверку отказа неопределённой длины при строгом виде записи
	 */
	{
		// Сборщик бинарной записи
		abc::writer_t writer;
		// Получаем настройки сборки записи
		abc::writer_t::settings_t settings = writer.settings();
		// Выполняем установку строгого вида записи
		settings.canonical = true;
		// Выполняем установку настроек сборки записи
		writer.settings(settings);
		// Выполняем проверку отказа начала строки, собираемой кусками
		ASSERT_FALSE(writer.textBegin());
		// Выполняем проверку кода отказа
		ASSERT_EQ(writer.error(), abc::error_t::INDEFINITE_REFUSED);
	}
	/**
	 * Выполняем проверку отказа куска иного вида
	 */
	{
		// Сборщик бинарной записи
		abc::writer_t writer;
		// Выполняем укладку начала строки, собираемой кусками
		ASSERT_TRUE(writer.textBegin());
		// Выполняем проверку отказа укладки двоичных данных куском строки
		ASSERT_FALSE(writer.blob("октеты", 6));
		// Выполняем проверку кода отказа
		ASSERT_EQ(writer.error(), abc::error_t::INVALID_SEGMENT);
	}
	/**
	 * Выполняем проверку отказа значения иного вида внутри собираемого
	 */
	{
		// Сборщик бинарной записи
		abc::writer_t writer;
		// Выполняем укладку начала двоичных данных, собираемых кусками
		ASSERT_TRUE(writer.blobBegin());
		// Выполняем проверку отказа укладки числа внутрь собираемого значения
		ASSERT_FALSE(writer.number(static_cast <uint64_t> (7)));
		// Выполняем проверку кода отказа
		ASSERT_EQ(writer.error(), abc::error_t::INVALID_SEGMENT);
	}
	/**
	 * Выполняем проверку отказа собираемого значения именем поля отображения
	 */
	{
		// Сборщик бинарной записи
		abc::writer_t writer;
		// Выполняем укладку начала отображения
		ASSERT_TRUE(writer.mapBegin(1));
		// Выполняем проверку отказа собираемого значения именем поля
		ASSERT_FALSE(writer.textBegin());
		// Выполняем проверку кода отказа
		ASSERT_EQ(writer.error(), abc::error_t::INVALID_KEY);
	}
	/**
	 * Выполняем проверку отказа конца, не отвечающего началу
	 */
	{
		// Сборщик бинарной записи
		abc::writer_t writer;
		// Выполняем укладку начала строки, собираемой кусками
		ASSERT_TRUE(writer.textBegin());
		// Выполняем проверку отказа конца двоичных данных у начатой строки
		ASSERT_FALSE(writer.blobEnd());
		// Выполняем проверку кода отказа
		ASSERT_EQ(writer.error(), abc::error_t::UNBALANCED_CONTAINER);
	}
}
/**
 * @brief Проверка сборки двоичных данных кусками внутри вместимого
 *
 */
TEST(CodecAbcWriter, SegmentedBlobInsideContainer) {
	// Сборщик бинарной записи
	abc::writer_t writer;
	// Выполняем укладку начала массива из двух значений
	ASSERT_TRUE(writer.arrayBegin(2));
	// Выполняем укладку начала двоичных данных, собираемых кусками
	ASSERT_TRUE(writer.blobBegin());
	// Выполняем укладку первого куска двоичных данных
	ASSERT_TRUE(writer.blob("\x01\x02", 2));
	// Выполняем укладку второго куска двоичных данных
	ASSERT_TRUE(writer.blob("\x03", 1));
	// Выполняем укладку конца двоичных данных
	ASSERT_TRUE(writer.blobEnd()) << "код отказа: " << abc::message(writer.error());
	/**
	 * Выполняем укладку второго значения массива: значение, собранное кусками, есть
	 * ОДНО значение вместившего, а не череда их
	 */
	ASSERT_TRUE(writer.number(static_cast <uint64_t> (42)));
	// Выполняем укладку конца массива
	ASSERT_TRUE(writer.arrayEnd()) << "код отказа: " << abc::message(writer.error());
	// Выполняем проверку завершённости собранной записи
	ASSERT_TRUE(writer.complete());
	// Разбиратель бинарной записи
	abc::reader_t reader;
	// Выполняем подачу собранной записи разбирателю
	ASSERT_TRUE(reader.feed(writer.record().data(), writer.record().size(), true))
		<< "код отказа: " << abc::message(reader.error());
	// Количество снятых значений массива
	size_t values = 0;
	// Собираемое содержимое двоичных данных
	string content;
	/**
	 * Выполняем снятие всех событий разбора
	 */
	while(reader.next()){
		// Если событием является кусок двоичных данных
		if(reader.event() == abc::event_t::BLOB)
			// Выполняем накопление содержимого куска
			content.append(reader.value().data);
		// Если событием является конец собранного значения либо число
		if((reader.event() == abc::event_t::BLOB_END) || (reader.event() == abc::event_t::NUMBER))
			// Выполняем учёт снятого значения массива
			values++;
	}
	// Выполняем проверку количества снятых значений массива
	ASSERT_EQ(values, 2ul);
	// Выполняем проверку собранного содержимого двоичных данных
	ASSERT_EQ(content.size(), 3ul);
}
/**
 * @brief Проверка укладки содержимого ссылкой
 *
 */
TEST(CodecAbcWriter, ReferencedContent){
	// Сборка бинарной записи
	abc::writer_t writer;
	// Выполняем получение настроек сборки записи
	abc::writer_t::settings_t settings = writer.settings();
	// Выполняем установку порога укладки содержимого ссылкой
	settings.reference = 16;
	// Выполняем установку настроек сборки записи
	writer.settings(settings);
	// Крупное значение, укладываемое ссылкой
	const string large(1024, 'a');
	// Мелкое значение, укладываемое копией
	const string small(4, 'b');
	// Выполняем укладку начала массива
	ASSERT_TRUE(writer.arrayBegin(static_cast <uint64_t> (2)));
	// Выполняем укладку крупного значения
	ASSERT_TRUE(writer.text(large)) << "код отказа: " << abc::message(writer.error());
	// Выполняем укладку мелкого значения
	ASSERT_TRUE(writer.text(small));
	// Выполняем укладку конца массива
	ASSERT_TRUE(writer.arrayEnd());
	// Выполняем проверку завершённости собранной записи
	ASSERT_TRUE(writer.complete());
	// Выполняем получение кусков собранной записи
	const vector <abc::piece_t> pieces = writer.pieces();
	/**
	 * Выполняем проверку того, что крупное значение уложено ссылкой: буфер собираемой
	 * записи октетов его не держит вовсе, и выдача идёт кусками
	 */
	ASSERT_GT(pieces.size(), 1ul);
	// Признак того, что октеты крупного значения выданы своей же памятью
	bool referenced = false;
	// Общая длина кусков собранной записи
	size_t length = 0;
	/**
	 * Выполняем перебор кусков собранной записи
	 */
	for(auto & piece : pieces){
		// Выполняем учёт длины куска собранной записи
		length += piece.size;
		// Если кусок выдан памятью крупного значения
		if(piece.buffer == large.data()){
			// Выполняем установку признака укладки ссылкой
			referenced = true;
			// Выполняем проверку размера куска собранной записи
			ASSERT_EQ(piece.size, large.size());
		}
	}
	// Выполняем проверку того, что крупное значение уложено ссылкой
	ASSERT_TRUE(referenced);
	// Выполняем проверку длины собранной записи
	ASSERT_EQ(length, writer.length());
	/**
	 * Выполняем проверку того, что цельная запись выходит той же самой: выдача её
	 * вклеивает содержимое, уложенное ссылкой, и потребителю ничего не меняется
	 */
	const vector <uint8_t> record = writer.record();
	// Выполняем проверку длины цельной записи
	ASSERT_EQ(record.size(), length);
	// Разбиратель бинарной записи
	abc::reader_t reader;
	// Выполняем подачу собранной записи разбирателю
	ASSERT_TRUE(reader.feed(record.data(), record.size(), true))
		<< "код отказа: " << abc::message(reader.error());
	// Собираемые значения массива
	vector <string> values;
	/**
	 * Выполняем снятие всех событий разбора
	 */
	while(reader.next()){
		// Если событием является строковое значение
		if(reader.event() == abc::event_t::STRING)
			// Выполняем накопление снятого значения
			values.push_back(string(reader.value().data));
	}
	// Выполняем проверку количества снятых значений
	ASSERT_EQ(values.size(), 2ul);
	// Выполняем проверку крупного значения
	ASSERT_EQ(values.front(), large);
	// Выполняем проверку мелкого значения
	ASSERT_EQ(values.back(), small);
}
/**
 * @brief Проверка укладки имени поля отображения копией
 *
 */
TEST(CodecAbcWriter, ReferencedKeysAreCopied){
	// Сборка бинарной записи
	abc::writer_t writer;
	// Выполняем получение настроек сборки записи
	abc::writer_t::settings_t settings = writer.settings();
	// Выполняем установку порога укладки содержимого ссылкой
	settings.reference = 8;
	// Выполняем установку строгого вида записи
	settings.canonical = true;
	// Выполняем установку настроек сборки записи
	writer.settings(settings);
	// Имена полей отображения, чья длина порог укладки ссылкой превышает
	const string first(32, 'a'), second(32, 'b');
	// Выполняем укладку начала отображения
	ASSERT_TRUE(writer.mapBegin(static_cast <uint64_t> (2)));
	// Выполняем укладку первого имени поля
	ASSERT_TRUE(writer.text(first)) << "код отказа: " << abc::message(writer.error());
	// Выполняем укладку значения первого поля
	ASSERT_TRUE(writer.number(static_cast <uint64_t> (1)));
	/**
	 * Выполняем укладку второго имени поля: сличается оно отрезком в буфере собираемой
	 * записи, и уложи мы имя ссылкой, сличать было бы нечего
	 */
	ASSERT_TRUE(writer.text(second)) << "код отказа: " << abc::message(writer.error());
	// Выполняем укладку значения второго поля
	ASSERT_TRUE(writer.number(static_cast <uint64_t> (2)));
	// Выполняем укладку конца отображения
	ASSERT_TRUE(writer.mapEnd());
	// Выполняем проверку завершённости собранной записи
	ASSERT_TRUE(writer.complete());
	// Выполняем проверку того, что имена полей уложены не по возрастанию
	abc::writer_t other;
	// Выполняем установку настроек сборки записи
	other.settings(settings);
	// Выполняем укладку начала отображения
	ASSERT_TRUE(other.mapBegin(static_cast <uint64_t> (2)));
	// Выполняем укладку первого имени поля
	ASSERT_TRUE(other.text(second));
	// Выполняем укладку значения первого поля
	ASSERT_TRUE(other.number(static_cast <uint64_t> (1)));
	// Выполняем проверку отказа на имя поля, стоящее не по возрастанию
	ASSERT_FALSE(other.text(first));
	// Выполняем проверку кода отказа сборки
	ASSERT_EQ(other.error(), abc::error_t::UNORDERED_KEY);
}
/**
 * @brief Проверка сличения имён полей после вклейки содержимого
 *
 */
TEST(CodecAbcWriter, FlattenKeepsKeys){
	// Сборка бинарной записи
	abc::writer_t writer;
	// Выполняем получение настроек сборки записи
	abc::writer_t::settings_t settings = writer.settings();
	// Выполняем установку порога укладки содержимого ссылкой
	settings.reference = 8;
	// Выполняем установку строгого вида записи
	settings.canonical = true;
	// Выполняем установку настроек сборки записи
	writer.settings(settings);
	// Крупное значение, укладываемое ссылкой
	const string large(256, 'z');
	// Выполняем укладку начала отображения
	ASSERT_TRUE(writer.mapBegin(static_cast <uint64_t> (3)));
	// Выполняем укладку первого имени поля
	ASSERT_TRUE(writer.text("bbb"));
	// Выполняем укладку крупного значения первого поля
	ASSERT_TRUE(writer.text(large));
	/**
	 * Выполняем укладку второго имени поля: стоит оно ПОСЛЕ содержимого, уложенного
	 * ссылкой, и вклейка того содержимого отрезок его сдвигает
	 */
	ASSERT_TRUE(writer.text("ccc"));
	// Выполняем укладку значения второго поля
	ASSERT_TRUE(writer.number(static_cast <uint64_t> (1)));
	/**
	 * Выполняем выдачу цельной записи посреди сборки: выдача эта вклеивает содержимое,
	 * уложенное ссылкой, и отрезок имени поля, уложенного прежде, обязан быть сдвинут
	 * на вклеенное - иначе сличение имён сличало бы не имена
	 */
	ASSERT_FALSE(writer.record().empty());
	/**
	 * Выполняем укладку третьего имени поля, стоящего по возрастанию: сдвинь мы отрезок
	 * имени неверно, сличалось бы оно с октетами крупного значения, и имя, стоящее по
	 * возрастанию, было бы отвергнуто ни за что
	 */
	ASSERT_TRUE(writer.text("ddd")) << "код отказа: " << abc::message(writer.error());
	// Выполняем укладку значения третьего поля
	ASSERT_TRUE(writer.number(static_cast <uint64_t> (2)));
	// Выполняем укладку конца отображения
	ASSERT_TRUE(writer.mapEnd());
	// Выполняем проверку завершённости собранной записи
	ASSERT_TRUE(writer.complete());
}
/**
 * @brief Проверка открытого расширения
 *
 */
TEST(CodecAbcWriter, CustomExtension){
	// Сборка бинарной записи
	abc::writer_t writer;
	// Октеты расширения, заведённого потребителем
	const string content = "\x01\x02\x03\x04";
	// Выполняем укладку начала массива
	ASSERT_TRUE(writer.arrayBegin(static_cast <uint64_t> (3)));
	// Выполняем укладку расширения с малым номером подвида
	ASSERT_TRUE(writer.custom(static_cast <uint64_t> (7), content.data(), content.size()))
		<< "код отказа: " << abc::message(writer.error());
	// Выполняем укладку расширения с крупным номером подвида
	ASSERT_TRUE(writer.custom(numeric_limits <uint64_t>::max(), content.data(), content.size()));
	// Выполняем укладку расширения без октетов
	ASSERT_TRUE(writer.custom(static_cast <uint64_t> (0), nullptr, 0));
	// Выполняем укладку конца массива
	ASSERT_TRUE(writer.arrayEnd());
	// Выполняем проверку завершённости собранной записи
	ASSERT_TRUE(writer.complete());
	// Разбиратель бинарной записи
	abc::reader_t reader;
	// Выполняем подачу собранной записи разбирателю
	ASSERT_TRUE(reader.feed(writer.record().data(), writer.record().size(), true))
		<< "код отказа: " << abc::message(reader.error());
	// Снятые номера подвидов расширения
	vector <uint64_t> subtypes;
	// Снятое содержимое расширений
	vector <string> contents;
	/**
	 * Выполняем снятие всех событий разбора
	 */
	while(reader.next()){
		// Если событием является открытое расширение
		if(reader.event() == abc::event_t::CUSTOM){
			// Выполняем получение значения текущего события
			const abc::reader_t::value_t value = reader.value();
			// Выполняем проверку вида значения
			ASSERT_EQ(value.type, abc::type_t::CUSTOM);
			// Выполняем накопление номера подвида расширения
			subtypes.push_back(value.number);
			// Выполняем накопление содержимого расширения
			contents.push_back(string(value.data));
		}
	}
	// Выполняем проверку количества снятых расширений
	ASSERT_EQ(subtypes.size(), 3ul);
	// Выполняем проверку малого номера подвида
	ASSERT_EQ(subtypes.at(0), static_cast <uint64_t> (7));
	// Выполняем проверку крупного номера подвида
	ASSERT_EQ(subtypes.at(1), numeric_limits <uint64_t>::max());
	// Выполняем проверку номера подвида расширения без октетов
	ASSERT_EQ(subtypes.at(2), static_cast <uint64_t> (0));
	// Выполняем проверку содержимого первого расширения
	ASSERT_EQ(contents.at(0), content);
	// Выполняем проверку содержимого расширения без октетов
	ASSERT_TRUE(contents.at(2).empty());
}
