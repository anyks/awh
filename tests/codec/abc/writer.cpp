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
	abc::writer_t writer(::logger());
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
	abc::reader_t reader(::logger());
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
/**
 * @brief Проверка умещения малых чисел в один ведущий октет
 *
 * @details Целое разделено на два крупных вида по знаку НАМЕРЕННО, и довод тому один:
 * единый вид потребовал бы записи со смещённым нулём либо зигзагом, и малое
 * отрицательное число перестало бы умещаться в ведущий октет. Довод этот есть обещание о
 * размере записи, и держится он ровно до тех пор, покуда границы соблюдены
 *
 * @note Обещание такого рода уходит молча: запись, выросшая вдвое на всяком малом числе,
 * читается исправно и круговой ход проходит. Утверждается потому ДЛИНА записи, а не
 * целость её
 *
 */
TEST(CodecAbcWriter, SmallNumbersFitLeadingOctet){
	/**
	 * @brief Работа сборки записи одиночного числа
	 *
	 * @param value укладываемое число
	 * @return      собранная запись числа
	 */
	auto compose = [](const auto value) noexcept -> vector <uint8_t> {
		// Сборщик бинарной записи
		abc::writer_t writer(::logger());
		// Если укладка числа отвечена отказом
		if(!writer.number(value))
			// Выводим пустую запись
			return vector <uint8_t> ();
		// Выводим собранную запись числа
		return writer.record();
	};
	/**
	 * Выполняем перебор всех малых чисел БЕЗ знака, умещающихся в ведущий октет
	 *
	 * @note Подробность ведущего октета значит само число, покуда оно меньше 24: значение
	 * 24 и выше объявляет ширину следующей за меткой длины
	 */
	for(uint64_t value = 0; value < 24; value++){
		// Выполняем сборку записи очередного числа без знака
		const vector <uint8_t> record = compose(value);
		// Запись обязана уместиться в один ведущий октет
		ASSERT_EQ(record.size(), static_cast <size_t> (1)) << "число: " << value;
		// Ведущий октет обязан нести крупный вид целого без знака и само число
		ASSERT_EQ(record.at(0), static_cast <uint8_t> (value)) << "число: " << value;
	}
	// Число 24 в ведущий октет уже не умещается
	ASSERT_EQ(compose(static_cast <uint64_t> (24)).size(), static_cast <size_t> (2));
	/**
	 * Выполняем перебор всех малых чисел СО знаком, умещающихся в ведущий октет
	 *
	 * @note Отрицательные числа кладутся своим крупным видом, и подробность их значит
	 * число, уменьшенное на единицу: нуля среди отрицательных нет, и место его отдано
	 * числу минус один
	 */
	for(int64_t value = -1; value >= -24; value--){
		// Выполняем сборку записи очередного числа со знаком
		const vector <uint8_t> record = compose(value);
		// Запись обязана уместиться в один ведущий октет
		ASSERT_EQ(record.size(), static_cast <size_t> (1)) << "число: " << value;
		// Ведущий октет обязан нести крупный вид отрицательного целого и само число
		ASSERT_EQ(record.at(0), static_cast <uint8_t> (0x20 + (-value - 1))) << "число: " << value;
	}
	// Число минус 25 в ведущий октет уже не умещается
	ASSERT_EQ(compose(static_cast <int64_t> (-25)).size(), static_cast <size_t> (2));
	/**
	 * Выполняем поверку кругового хода всех чисел, умещающихся в ведущий октет
	 *
	 * @note Половина вторая договора: краткость записи ничего не стоит, буде число из
	 * неё вычитывается иным
	 */
	for(int64_t value = -24; value < 24; value++){
		// Выполняем сборку записи очередного числа
		const vector <uint8_t> record = ((value < 0) ? compose(value) : compose(static_cast <uint64_t> (value)));
		// Дерево документа
		abc::document_t document(::logger());
		// Выполняем разбор записи в дерево документа
		ASSERT_TRUE(document.parse(record.data(), record.size())) << "число: " << value;
		// Снимаемое число со знаком
		int64_t number = 0;
		// Число обязано сняться со знаком
		ASSERT_TRUE(document.root().value(number)) << "число: " << value;
		// Снятое число обязано отвечать уложенному
		ASSERT_EQ(number, value) << "число: " << value;
	}
}
TEST(CodecAbcWriter, IndefiniteRoundtrip) {
	// Сборщик бинарной записи
	abc::writer_t writer(::logger());
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
	abc::reader_t reader(::logger());
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
	abc::writer_t writer(::logger());
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
	abc::reader_t reader(::logger());
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
		abc::writer_t writer(::logger());
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
		abc::writer_t writer(::logger());
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
		abc::writer_t writer(::logger());
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
		abc::writer_t writer(::logger());
		// Выполняем проверку отказа на закрытие вместимого, какого нет
		ASSERT_FALSE(writer.arrayEnd());
		// Выполняем проверку кода отказа
		ASSERT_EQ(writer.error(), abc::error_t::UNBALANCED_CONTAINER);
	}
	// Выполняем проверку отказа на отображение, закрытое на имени поля
	{
		// Сборщик бинарной записи
		abc::writer_t writer(::logger());
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
		abc::writer_t writer(::logger());
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
		abc::writer_t writer(::logger());
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
	abc::writer_t writer(::logger());
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
		abc::writer_t writer(::logger());
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
		abc::writer_t writer(::logger());
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
		abc::writer_t writer(::logger());
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
		abc::writer_t writer(::logger());
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
		abc::writer_t writer(::logger());
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
		abc::writer_t writer(::logger());
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
		abc::writer_t writer(::logger());
		// Выполняем проверку отказа на укладку числа неограниченной ширины
		ASSERT_FALSE(writer.bignum(magnitude.data(), magnitude.size(), false));
		// Выполняем проверку кода отказа
		ASSERT_EQ(writer.error(), abc::error_t::INVALID_BIGNUM);
	}
	// Выполняем проверку отказа на величину, объявленную отрицательным нулём
	{
		// Сборщик бинарной записи
		abc::writer_t writer(::logger());
		// Выполняем проверку отказа на укладку отрицательного нуля
		ASSERT_FALSE(writer.bignum(nullptr, 0, true));
		// Выполняем проверку кода отказа
		ASSERT_EQ(writer.error(), abc::error_t::INVALID_BIGNUM);
	}
	// Выполняем проверку того, что отказ сборки останавливает её насовсем
	{
		// Сборщик бинарной записи
		abc::writer_t writer(::logger());
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
		abc::writer_t writer(::logger());
		// Выполняем укладку целого числа без знака
		ASSERT_TRUE(writer.number(item.first));
		// Выполняем проверку длины собранной записи
		ASSERT_EQ(writer.record().size(), item.second) << "значение: " << item.first;
	}
	// Выполняем проверку того, что пустое отображение занимает один октет
	{
		// Сборщик бинарной записи
		abc::writer_t writer(::logger());
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
	abc::writer_t writer(::logger());
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
	abc::reader_t reader(::logger());
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
		abc::writer_t writer(::logger());
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
		abc::writer_t writer(::logger());
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
		abc::writer_t writer(::logger());
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
		abc::writer_t writer(::logger());
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
		abc::writer_t writer(::logger());
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
	abc::writer_t writer(::logger());
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
	abc::reader_t reader(::logger());
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
	abc::writer_t writer(::logger());
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
	abc::reader_t reader(::logger());
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
	abc::writer_t writer(::logger());
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
	abc::writer_t other(::logger());
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
	abc::writer_t writer(::logger());
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
	abc::writer_t writer(::logger());
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
	abc::reader_t reader(::logger());
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
/**
 * @brief Проверка объявления размаха вместимого
 *
 * @details Размах даёт чтению пропустить вместимое одним сложением вместо обхода всех
 * его значений. Метка эта есть расширение проволочной записи, оттого настройка по
 * умолчанию снята: запись, собранная без неё, не отличается от прежней ни одним октетом
 *
 */
TEST(CodecAbcWriter, SpannedContainer) {
	// Сборщик записи без объявления размаха
	abc::writer_t plain(::logger());
	// Выполняем сборку записи с вложенным перечнем
	ASSERT_TRUE(plain.mapBegin(1));
	// Выполняем укладку имени поля отображения
	ASSERT_TRUE(plain.text("груз"));
	// Выполняем заведение вложенного перечня
	ASSERT_TRUE(plain.arrayBegin(4));
	/**
	 * Выполняем укладку значений вложенного перечня
	 */
	for(uint64_t i = 0; i < 4; i++)
		// Выполняем укладку очередного значения перечня
		ASSERT_TRUE(plain.number(i));
	// Выполняем закрытие вложенного перечня
	ASSERT_TRUE(plain.arrayEnd());
	// Выполняем закрытие отображения
	ASSERT_TRUE(plain.mapEnd());
	// Октеты записи без объявления размаха
	const vector <uint8_t> bare = plain.record();
	// Сборщик записи с объявлением размаха
	abc::writer_t spanned(::logger());
	// Настройки сборки записи
	abc::writer_t::settings_t settings = spanned.settings();
	// Выполняем установку порога объявления размаха
	settings.spanned = 4;
	// Выполняем установку настроек сборки записи
	spanned.settings(settings);
	// Выполняем сборку той же записи с объявлением размаха
	ASSERT_TRUE(spanned.mapBegin(1));
	// Выполняем укладку имени поля отображения
	ASSERT_TRUE(spanned.text("груз"));
	// Выполняем заведение вложенного перечня
	ASSERT_TRUE(spanned.arrayBegin(4));
	/**
	 * Выполняем укладку значений вложенного перечня
	 */
	for(uint64_t i = 0; i < 4; i++)
		// Выполняем укладку очередного значения перечня
		ASSERT_TRUE(spanned.number(i));
	// Выполняем закрытие вложенного перечня
	ASSERT_TRUE(spanned.arrayEnd());
	// Выполняем закрытие отображения
	ASSERT_TRUE(spanned.mapEnd());
	// Октеты записи с объявлением размаха
	const vector <uint8_t> wide = spanned.record();
	/**
	 * Выполняем проверку того, что запись с размахом длиннее ровно на метку и размах.
	 *
	 * Отображение из одной пары порога не достигает, и размах объявляется лишь у
	 * перечня: платит за размах только тот, кто его получает
	 */
	ASSERT_EQ(wide.size(), bare.size() + 1 + abc::SPAN_LENGTH);
	// Дерево документа записи без размаха
	abc::document_t first(::logger());
	// Выполняем разбор записи без размаха
	ASSERT_TRUE(first.parse(bare.data(), bare.size()));
	// Дерево документа записи с размахом
	abc::document_t second(::logger());
	/**
	 * Выполняем разбор записи с размахом: метка размаха разбору ПРОЗРАЧНА, и дерево
	 * обязано выйти тем же самым
	 */
	ASSERT_TRUE(second.parse(wide.data(), wide.size()));
	// Владеющее значение записи без размаха
	abc::value_t plainValue;
	// Выполняем разбор записи без размаха во владеющее значение
	ASSERT_TRUE(plainValue.parse(bare.data(), bare.size()));
	// Владеющее значение записи с размахом
	abc::value_t spannedValue;
	// Выполняем разбор записи с размахом во владеющее значение
	ASSERT_TRUE(spannedValue.parse(wide.data(), wide.size()));
	// Выполняем проверку того, что значения сошлись
	ASSERT_EQ(spannedValue, plainValue);
}
/**
 * @brief Проверка пропуска вместимого по объявленному размаху
 *
 */
TEST(CodecAbcWriter, SpannedSkip) {
	// Сборщик записи с объявлением размаха
	abc::writer_t writer(::logger());
	// Настройки сборки записи
	abc::writer_t::settings_t settings = writer.settings();
	// Выполняем установку порога объявления размаха
	settings.spanned = 8;
	// Выполняем установку настроек сборки записи
	writer.settings(settings);
	// Выполняем заведение отображения из двух полей
	ASSERT_TRUE(writer.mapBegin(2));
	// Выполняем укладку имени первого поля отображения
	ASSERT_TRUE(writer.text("груз"));
	// Выполняем заведение крупного вложенного перечня
	ASSERT_TRUE(writer.arrayBegin(64));
	/**
	 * Выполняем укладку значений крупного вложенного перечня
	 */
	for(uint64_t i = 0; i < 64; i++)
		// Выполняем укладку очередного значения перечня
		ASSERT_TRUE(writer.number(i));
	// Выполняем закрытие крупного вложенного перечня
	ASSERT_TRUE(writer.arrayEnd());
	// Выполняем укладку имени второго поля отображения
	ASSERT_TRUE(writer.text("хвост"));
	// Выполняем укладку искомого значения
	ASSERT_TRUE(writer.text("искомое"));
	// Выполняем закрытие отображения
	ASSERT_TRUE(writer.mapEnd());
	// Октеты собранной записи
	const vector <uint8_t> record = writer.record();
	/**
	 * @brief Опора прямой выдачи событий разбора
	 *
	 * @details Пропуск зовётся ТОЛЬКО из обработчика прямой выдачи: события копятся
	 * очередью, и к выдаче события разбор уходит вперёд - пропускать к тому времени
	 * уже нечего
	 *
	 */
	struct Sink {
		// Количество снятых событий разбора
		size_t counted = 0;
		// Снятое строковое значение
		string last;
	} sink;
	// Поточный разбиратель записи
	abc::reader_t reader(::logger());
	// Выполняем установку обработчика прямой выдачи событий разбора
	reader.handler([](void * context, abc::reader_t & reader, const abc::event_t event) noexcept -> void {
		// Выполняем получение опоры прямой выдачи событий
		Sink * sink = reinterpret_cast <Sink *> (context);
		// Выполняем учёт снятого события разбора
		sink->counted++;
		// Если снято событие начала перечня
		if(event == abc::event_t::ARRAY_BEGIN)
			// Выполняем пропуск крупного вложенного перечня целиком
			(void) reader.skip();
		// Если снято строковое значение
		else if(event == abc::event_t::STRING)
			// Выполняем запоминание снятого строкового значения
			sink->last = string(reader.value().data);
	}, & sink);
	// Выполняем подачу записи разбирателю целиком
	ASSERT_TRUE(reader.feed(record.data(), record.size()));
	/**
	 * Выполняем проверку того, что разбор дошёл до значения за пропущенным перечнем:
	 * ради него пропуск и заведён
	 */
	ASSERT_EQ(sink.last, "искомое");
	/**
	 * Выполняем проверку того, что значения пропущенного перечня событиями НЕ выданы:
	 * их шестьдесят четыре, и обойдись пропуск обходом - счёт событий выдал бы это
	 */
	ASSERT_LT(sink.counted, 20ul);

}
/**
 * @brief Проверка стойкости чтения к враждебному размаху вместимого
 *
 * @details Размах приходит ИЗВНЕ и доверия не заслуживает: объявленный слишком большим
 * он переполнил бы счёт мест записи, а слишком малым - увёл бы разбор назад, и
 * обработчик, пропускающий вместимое всякий раз, закружил бы разбор навсегда
 *
 */
TEST(CodecAbcWriter, SpannedHostile) {
	// Сборщик записи с объявлением размаха
	abc::writer_t writer(::logger());
	// Настройки сборки записи
	abc::writer_t::settings_t settings = writer.settings();
	// Выполняем установку порога объявления размаха
	settings.spanned = 4;
	// Выполняем установку настроек сборки записи
	writer.settings(settings);
	// Выполняем заведение отображения из двух полей
	ASSERT_TRUE(writer.mapBegin(2));
	// Выполняем укладку имени первого поля отображения
	ASSERT_TRUE(writer.text("груз"));
	// Выполняем заведение вложенного перечня
	ASSERT_TRUE(writer.arrayBegin(8));
	/**
	 * Выполняем укладку значений вложенного перечня
	 */
	for(uint64_t i = 0; i < 8; i++)
		// Выполняем укладку очередного значения перечня
		ASSERT_TRUE(writer.number(i));
	// Выполняем закрытие вложенного перечня
	ASSERT_TRUE(writer.arrayEnd());
	// Выполняем укладку имени второго поля отображения
	ASSERT_TRUE(writer.text("хвост"));
	// Выполняем укладку искомого значения
	ASSERT_TRUE(writer.text("искомое"));
	// Выполняем закрытие отображения
	ASSERT_TRUE(writer.mapEnd());
	// Октеты собранной записи
	const vector <uint8_t> origin = writer.record();
	// Место метки размаха в записи
	size_t place = origin.size();
	/**
	 * Выполняем разыскание метки размаха: крупный вид расширения в старших разрядах,
	 * разновидность размаха - в младших
	 */
	const uint8_t tag = static_cast <uint8_t> ((static_cast <uint8_t> (abc::group_t::EXTEND) << 5) |
	 static_cast <uint8_t> (abc::extend_t::SPANNED));
	// Выполняем перебор всех октетов записи
	for(size_t i = 0; i < origin.size(); i++){
		// Если очередной октет является меткой размаха
		if(origin.at(i) == tag){
			// Запоминаем место метки размаха
			place = i;
			// Прекращаем разыскание метки размаха
			break;
		}
	}
	// Выполняем проверку того, что метка размаха разыскана
	ASSERT_LT(place, origin.size()) << "метка размаха в записи не разыскана";
	/**
	 * @brief Функция подачи записи чтению с пропуском вместимых
	 *
	 * @param record подаваемая запись
	 * @param error  код отказа разбора
	 * @return       количество выданных событий разбора
	 *
	 */
	const auto submit = [](const vector <uint8_t> & record, abc::error_t & error) noexcept -> size_t {
		// Количество выданных событий разбора
		static size_t counted = 0;
		// Выполняем сброс количества выданных событий
		counted = 0;
		// Поточный разбиратель записи
		abc::reader_t reader(::logger());
		// Выполняем установку обработчика прямой выдачи событий разбора
		reader.handler([](void *, abc::reader_t & reader, const abc::event_t event) noexcept -> void {
			// Выполняем учёт выданного события разбора
			counted++;
			/**
			 * Если событий выдано заведомо больше, чем запись способна дать, разбор
			 * закружился: прекращаем работу, дабы проверка не висла до срока
			 */
			if(counted > 4096)
				// Выполняем прекращение работы
				::abort();
			// Если снято событие начала перечня
			if(event == abc::event_t::ARRAY_BEGIN)
				// Выполняем пропуск вложенного перечня целиком
				(void) reader.skip();
		}, nullptr);
		// Выполняем подачу записи разбирателю целиком
		(void) reader.feed(record.data(), record.size());
		// Выполняем установку кода отказа разбора
		error = reader.error();
		// Выводим количество выданных событий разбора
		return counted;
	};
	// Код отказа разбора записи
	abc::error_t error = abc::error_t::NONE;
	// Выполняем проверку того, что целая запись разбирается
	ASSERT_GT(submit(origin, error), 0ul);
	// Выполняем проверку того, что отказа на целой записи нет
	ASSERT_EQ(error, abc::error_t::NONE);
	/**
	 * Выполняем проверку записи с размахом, переполняющим счёт мест
	 */
	vector <uint8_t> huge = origin;
	// Выполняем перебор всех октетов записи размаха
	for(size_t i = 0; i < abc::SPAN_LENGTH; i++)
		// Выполняем установку наибольшего значения октета размаха
		huge.at(place + 1 + i) = 0xFF;
	// Выполняем подачу записи с переполняющим размахом
	(void) submit(huge, error);
	/**
	 * Выполняем проверку кода отказа: размах, переполняющий счёт мест, обязан
	 * отвергаться разбором, а не обращаться сложением в место ПРЕЖДЕ нынешнего
	 */
	ASSERT_EQ(error, abc::error_t::INVALID_LENGTH);
	/**
	 * Выполняем проверку записи с нулевым размахом
	 */
	vector <uint8_t> zero = origin;
	// Выполняем перебор всех октетов записи размаха
	for(size_t i = 0; i < abc::SPAN_LENGTH; i++)
		// Выполняем обнуление очередного октета размаха
		zero.at(place + 1 + i) = 0;
	/**
	 * Выполняем подачу записи с нулевым размахом.
	 *
	 * Пропуск по такому размаху увёл бы разбор назад, вместимое пошло бы разбираться
	 * наново, и обработчик закружил бы разбор навсегда. Пропуск обязан ОТКАЗАТЬ
	 */
	ASSERT_GT(submit(zero, error), 0ul);
	/**
	 * Выполняем проверку кода отказа: лживый размах отвергается разбором.
	 *
	 * Договор здесь ужесточён 22.08.2026. Прежде разбор нулевым размахом лишь не
	 * сбивался: пропуск отказывал, а запись проходила обычным ходом, и объявленному
	 * размаху верили на слово до тех пор, покуда разбор не натыкался на середину
	 * значения. Ныне размах поверяется у закрывающегося вместимого - настоящий конец
	 * его известен, - и лживый отвергается СРАЗУ, в том числе у того, кто пропуском
	 * не пользуется вовсе
	 */
	ASSERT_EQ(error, abc::error_t::INVALID_LENGTH);
	/**
	 * Выполняем проверку записи с размахом, объявленным на октет короче настоящего
	 *
	 * @note Случай этот прежде не ловился ничем: пропуск по такому размаху уводил в
	 *       середину значения, а разбор без пропуска расхождения не замечал вовсе
	 */
	vector <uint8_t> shortened = origin;
	// Выполняем проверку того, что размах есть чему укорачивать
	ASSERT_GT(shortened.at(place + 1), 0);
	// Выполняем уменьшение младшего октета размаха
	shortened.at(place + 1) = static_cast <uint8_t> (shortened.at(place + 1) - 1);
	// Поточный разбиратель записи с укороченным размахом
	abc::reader_t plain(::logger());
	// Выполняем подачу записи с укороченным размахом БЕЗ всякого пропуска
	(void) plain.feed(shortened.data(), shortened.size());
	/**
	 * Выполняем перебор всех событий разбора
	 */
	while(plain.next())
		// Выполняем продвижение разбора записи
		;
	/**
	 * Выполняем проверку кода отказа: у того, кто пропуском не пользуется, лживый
	 * размах вскрывается поверкою у закрывающегося вместимого, и код отказа тут
	 * определён однозначно
	 */
	ASSERT_EQ(plain.error(), abc::error_t::INVALID_LENGTH);
	// Выполняем подачу записи с укороченным размахом с пропуском вместимых
	(void) submit(shortened, error);
	/**
	 * Выполняем проверку того, что укороченный размах отвергается и при пропуске.
	 *
	 * Код отказа здесь НЕ закрепляется: пропуск уводит разбор по объявленному месту,
	 * а место это лживое, и что разбор встретит на нём - середину значения, метку не
	 * ту либо хвост за концом записи, - зависит от самой записи. Закреплено то, что
	 * разбор отвечает отказом, а не выдаёт запись за целую
	 */
	ASSERT_NE(error, abc::error_t::NONE);
}

/**
 * @brief Проверка принадлежности объявленного размаха ближайшему вместимому
 *
 * @details Метка размаха стоит впереди вместимого, и своя сборка иначе её не кладёт.
 *          Запись, однако, приходит извне: метка, поставленная перед НЕвместимым,
 *          оставляла размах висеть до следующего вместимого, и то пропускалось по
 *          чужому размаху - содержимое его пропадало, а запись отвергалась
 *
 */
TEST(CodecAbcWriter, SpannedBeforeSingle) {
	// Сборщик вложенного перечня записи
	abc::writer_t writer(::logger());
	// Выполняем заведение вложенного перечня из двух чисел
	ASSERT_TRUE(writer.arrayBegin(2));
	// Выполняем укладку первого числа вложенного перечня
	ASSERT_TRUE(writer.number(static_cast <uint64_t> (7)));
	// Выполняем укладку второго числа вложенного перечня
	ASSERT_TRUE(writer.number(static_cast <uint64_t> (8)));
	// Выполняем закрытие вложенного перечня
	ASSERT_TRUE(writer.arrayEnd());
	// Октеты вложенного перечня записи
	const vector <uint8_t> tail = writer.record();
	/**
	 * Выполняем перебор размахов, накрывающих разные места записи
	 */
	for(uint64_t width = 4; width <= 12; width += 2){
		// Октеты собираемой вручную записи
		vector <uint8_t> record;
		// Выполняем укладку метки корневого перечня из двух значений
		record.push_back(static_cast <uint8_t> ((static_cast <uint8_t> (abc::group_t::ARRAY) << 5) | 2));
		// Выполняем укладку метки объявленного размаха
		record.push_back(static_cast <uint8_t> ((static_cast <uint8_t> (abc::group_t::EXTEND) << 5) |
		 static_cast <uint8_t> (abc::extend_t::SPANNED)));
		/**
		 * Выполняем укладку самого размаха
		 */
		for(size_t i = 0; i < abc::SPAN_LENGTH; i++)
			// Выполняем укладку очередного октета размаха
			record.push_back(static_cast <uint8_t> ((width >> (i * 8)) & 0xFF));
		// Выполняем укладку метки строки длиною три октета
		record.push_back(static_cast <uint8_t> ((static_cast <uint8_t> (abc::group_t::STRING) << 5) | 3));
		// Выполняем укладку октетов строки
		record.push_back('a');
		record.push_back('b');
		record.push_back('c');
		// Выполняем укладку вложенного перечня записи
		record.insert(record.end(), tail.begin(), tail.end());
		/**
		 * @brief Опора прямой выдачи событий разбора
		 *
		 */
		struct Sink {
			// Количество снятых событий разбора
			size_t counted = 0;
			// Количество состоявшихся пропусков
			size_t skipped = 0;
			// Количество снятых чисел записи
			size_t numbers = 0;
		} sink;
		// Поточный разбиратель записи
		abc::reader_t reader(::logger());
		// Выполняем установку обработчика прямой выдачи событий разбора
		reader.handler([](void * context, abc::reader_t & reader, const abc::event_t event) noexcept -> void {
			// Выполняем получение опоры прямой выдачи событий
			Sink * sink = reinterpret_cast <Sink *> (context);
			// Выполняем учёт снятого события разбора
			sink->counted++;
			// Если снято число записи
			if(event == abc::event_t::NUMBER)
				// Выполняем учёт снятого числа записи
				sink->numbers++;
			/**
			 * Выполняем пропуск на ВСЯКОМ событии: потребитель вправе звать пропуск где
			 * угодно, и там, где пропускать нечего, пропуск обязан отвечать отказом
			 */
			if(reader.skip())
				// Выполняем учёт состоявшегося пропуска
				sink->skipped++;
			// Если событий снято непомерно много
			if(sink->counted > 4096)
				// Прекращаем работу: разбор закружил
				::abort();
		}, & sink);
		// Выполняем подачу собранной записи разбирателю
		ASSERT_TRUE(reader.feed(record.data(), record.size(), true));
		/**
		 * Выполняем продвижение разбора записи
		 */
		while(reader.next())
			// Выполняем продвижение разбора записи
			;
		// Выполняем проверку отсутствия отказа разбора
		ASSERT_EQ(reader.error(), abc::error_t::NONE);
		// Выполняем проверку отсутствия состоявшихся пропусков
		ASSERT_EQ(sink.skipped, static_cast <size_t> (0));
		// Выполняем проверку сохранности содержимого вложенного перечня
		ASSERT_EQ(sink.numbers, static_cast <size_t> (2));
	}
}

/**
 * @brief Проверка поверки размаха у вместимого неопределённой длины
 *
 * @details Вместимое неопределённой длины закрывается меткою конца, а не исчерпанием
 *          значений, и сматывание его не касается: дорога закрытия здесь ВТОРАЯ. Своя
 *          сборка размаха неопределённому вместимому не объявляет вовсе, но запись
 *          приходит извне, и объявление надлежит поверить на обеих дорогах - иначе
 *          поверенным окажется один вид записи из двух
 *
 */
TEST(CodecAbcWriter, SpannedIndefinite) {
	/**
	 * @brief Функция сборки записи с объявленным размахом и неопределённой длиной
	 *
	 * @param width объявляемый размах вместимого
	 * @return      октеты собранной записи
	 *
	 */
	const auto compose = [](const uint64_t width) noexcept -> vector <uint8_t> {
		// Октеты собираемой записи
		vector <uint8_t> result;
		// Выполняем укладку метки объявленного размаха
		result.push_back(static_cast <uint8_t> ((static_cast <uint8_t> (abc::group_t::EXTEND) << 5) |
		 static_cast <uint8_t> (abc::extend_t::SPANNED)));
		/**
		 * Выполняем укладку самого размаха
		 */
		for(size_t i = 0; i < abc::SPAN_LENGTH; i++)
			// Выполняем укладку очередного октета размаха
			result.push_back(static_cast <uint8_t> ((width >> (i * 8)) & 0xFF));
		// Выполняем укладку метки перечня неопределённой длины
		result.push_back(static_cast <uint8_t> ((static_cast <uint8_t> (abc::group_t::ARRAY) << 5) | 0x1F));
		// Выполняем укладку первого значения перечня
		result.push_back(static_cast <uint8_t> ((static_cast <uint8_t> (abc::group_t::UNSIGNED) << 5) | 1));
		// Выполняем укладку второго значения перечня
		result.push_back(static_cast <uint8_t> ((static_cast <uint8_t> (abc::group_t::UNSIGNED) << 5) | 2));
		// Выполняем укладку метки конца перечня
		result.push_back(static_cast <uint8_t> ((static_cast <uint8_t> (abc::group_t::SINGLE) << 5) | 0x1F));
		// Выводим октеты собранной записи
		return result;
	};
	/**
	 * @brief Функция подачи записи чтению
	 *
	 * @param record подаваемая запись
	 * @return       код отказа разбора
	 *
	 */
	const auto submit = [](const vector <uint8_t> & record) noexcept -> abc::error_t {
		// Поточный разбиратель записи
		abc::reader_t reader(::logger());
		// Выполняем подачу записи разбирателю целиком
		(void) reader.feed(record.data(), record.size(), true);
		/**
		 * Выполняем перебор всех событий разбора
		 */
		while(reader.next())
			// Выполняем продвижение разбора записи
			;
		// Выводим код отказа разбора
		return reader.error();
	};
	/**
	 * Выполняем проверку записи с верным размахом: метка конца принадлежит вместимому
	 * так же, как и его значения, и место за вместимым лежит ЗА нею
	 */
	ASSERT_EQ(submit(compose(4)), abc::error_t::NONE);
	// Выполняем проверку записи с размахом, объявленным короче настоящего
	ASSERT_EQ(submit(compose(3)), abc::error_t::INVALID_LENGTH);
	// Выполняем проверку записи с размахом, объявленным длиннее настоящего
	ASSERT_EQ(submit(compose(5)), abc::error_t::INVALID_LENGTH);
	// Выполняем проверку записи с нулевым размахом
	ASSERT_EQ(submit(compose(0)), abc::error_t::INVALID_LENGTH);
}
/**
 * @brief Проверка того, что порог укладки ссылкой записи не меняет
 *
 * @details Содержимое, чей размер порог превысил, ложится ВРЕЗКОЙ, а не копией в буфер.
 *          Собранная запись обязана от того не измениться ни единым октетом: по ней
 *          считается подпись контейнера, и запись, зависящая от порога, обратила бы
 *          подпись в лотерею. Ширина размаха при том обязана считать и уложенное
 *          ссылкой - без этого объявленный размах уводил бы пропуск не туда
 *
 * @note Поверяются разом три выдачи: цельный буфер, куски вразброс и объявленная длина
 *
 */
TEST(CodecAbcWriter, ReferenceKeepsRecord){
	/**
	 * @brief Функция сборки записи при заданном пороге укладки ссылкой
	 *
	 * @param reference порог укладки содержимого ссылкой в октетах
	 * @param width     ширина крупного содержимого записи
	 * @param spanned   признак объявления размаха вместимых
	 * @param pieces    буфер, куда следует сложить куски собранной записи подряд
	 * @param length    объявленная длина собранной записи
	 * @return          собранная запись цельным буфером
	 */
	const auto build = [](const size_t reference, const size_t width, const bool spanned,
	 vector <uint8_t> & pieces, size_t & length) noexcept -> vector <uint8_t> {
		// Сборка бинарной записи
		abc::writer_t writer(::logger());
		// Получаем настройки сборки записи
		abc::writer_t::settings_t settings = writer.settings();
		// Выполняем установку порога укладки содержимого ссылкой
		settings.reference = reference;
		// Выполняем установку порога объявления размаха вместимых
		settings.spanned = (spanned ? static_cast <uint64_t> (1) : static_cast <uint64_t> (0));
		// Выполняем установку настроек сборки записи
		writer.settings(settings);
		// Крупное содержимое, какое порог укладки ссылкой и превышает
		const string big(width, 'B');
		// Двоичное содержимое той же ширины
		const vector <uint8_t> blob(width, 0xAB);
		// Признак успешности укладки значений записи
		bool held = writer.mapBegin(static_cast <uint64_t> (3));
		// Выполняем укладку первой пары отображения
		held = held && writer.text("aa") && writer.text(big);
		// Выполняем укладку второй пары отображения
		held = held && writer.text("bb") && writer.blob(blob.data(), blob.size());
		// Выполняем укладку третьей пары отображения вместимым
		held = held && writer.text("cc") && writer.arrayBegin(static_cast <uint64_t> (3));
		// Выполняем укладку значений вместимого
		held = held && writer.text(big) && writer.number(static_cast <uint64_t> (42)) && writer.text("хвост");
		// Выполняем закрытие вместимого и отображения
		held = held && writer.arrayEnd() && writer.mapEnd();
		// Если уложить запись не удалось
		if(!held) return {};
		// Выполняем получение объявленной длины собранной записи
		length = writer.length();
		// Выполняем очистку буфера кусков собранной записи
		pieces.clear();
		/**
		 * Выполняем перебор всех кусков собранной записи
		 */
		for(const auto & piece : writer.pieces()){
			// Выполняем получение указателя на октеты очередного куска
			const uint8_t * octets = reinterpret_cast <const uint8_t *> (piece.buffer);
			// Выполняем складывание октетов очередного куска подряд
			pieces.insert(pieces.end(), octets, octets + piece.size);
		}
		// Выводим собранную запись цельным буфером
		return vector <uint8_t> (writer.record().begin(), writer.record().end());
	};
	/**
	 * Выполняем перебор ширины крупного содержимого записи
	 */
	for(const size_t width : {static_cast <size_t> (8), static_cast <size_t> (64),
	 static_cast <size_t> (512), static_cast <size_t> (4096)}){
		/**
		 * Выполняем перебор объявления размаха вместимых
		 */
		for(const bool spanned : {false, true}){
			// Буфер кусков эталонной записи
			vector <uint8_t> pieces;
			// Объявленная длина эталонной записи
			size_t length = 0;
			// Выполняем сборку эталонной записи: порог снят, всё копируется
			const vector <uint8_t> truth = build(0, width, spanned, pieces, length);
			// Выполняем проверку того, что эталонная запись собрана
			ASSERT_FALSE(truth.empty()) << "ширина: " << width;
			// Выполняем проверку того, что куски эталонной записи сошлись с нею
			ASSERT_EQ(pieces, truth) << "ширина: " << width;
			// Выполняем проверку того, что объявленная длина сошлась с записью
			ASSERT_EQ(length, truth.size()) << "ширина: " << width;
			/**
			 * Выполняем перебор порогов укладки содержимого ссылкой
			 */
			for(const size_t reference : {static_cast <size_t> (1), static_cast <size_t> (8),
			 static_cast <size_t> (64), static_cast <size_t> (512), static_cast <size_t> (4096)}){
				// Буфер кусков собранной записи
				vector <uint8_t> parts;
				// Объявленная длина собранной записи
				size_t declared = 0;
				// Выполняем сборку записи при очередном пороге укладки ссылкой
				const vector <uint8_t> record = build(reference, width, spanned, parts, declared);
				// Выполняем проверку того, что запись сошлась с эталонной октет в октет
				ASSERT_EQ(record, truth) << "ширина: " << width << ", порог: " << reference;
				// Выполняем проверку того, что куски сошлись с цельным буфером
				ASSERT_EQ(parts, record) << "ширина: " << width << ", порог: " << reference;
				// Выполняем проверку того, что объявленная длина считает и уложенное ссылкой
				ASSERT_EQ(declared, truth.size()) << "ширина: " << width << ", порог: " << reference;
			}
		}
	}
}
/**
 * @brief Проверка умолчаний настроек сборки записи
 *
 * @details Умолчания эти обещаны заголовком поимённо: строгий вид записи по умолчанию
 *          снят, укладка содержимого ссылкой по умолчанию снята, а отказ на повтор имени
 *          поля отображения взят умолчанием наравне с разбором
 *
 * @note Проверка заведена находкой 31.08.2026 вместе с сестрой своей у разбора
 *       (`CodecAbcReader.SettingsDefaults`): подмена умолчаний разбора была поставлена
 *       щупом, и набор ответил зеленью. Умолчания сборки не стерёг никто вовсе
 *
 * @note Умолчание `reference = 0` есть осанка ВЛАДЕНИЯ: содержимое, уложенное ссылкой,
 *       обязано пережить выдачу записи, и обязанность эта, как объявлено заголовком,
 *       ложится на потребителя. Впрягать её всем без спроса нельзя, и молчаливая
 *       подмена умолчания впрягла бы
 */
TEST(CodecAbcWriter, SettingsDefaults){
	// Настройки сборки записи, взятые умолчанием
	const abc::writer_t::settings_t settings;
	// Строгий вид записи умолчанием снят
	ASSERT_FALSE(settings.canonical);
	// Поверка укладываемого умолчанием ведётся
	ASSERT_TRUE(settings.validate);
	// Повтор имени поля отображения умолчанием отвергается
	ASSERT_TRUE(settings.duplicates);
	// Предел глубины вложения умолчанием снят
	ASSERT_EQ(settings.maxDepth, 0u);
	// Порог укладки содержимого ссылкой умолчанием снят
	ASSERT_EQ(settings.reference, 0ul);
	// Порог укладки значения отрезками умолчанием снят
	ASSERT_EQ(settings.spanned, 0ull);
}
