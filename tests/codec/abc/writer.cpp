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
