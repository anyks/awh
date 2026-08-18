/**
 * @file document.cpp
 * @date 2026-08-19
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Проверки дерева документа бинарного контейнера ABC — разбор записи в дерево,
 *        обход его, извлечение значений и обратная сборка записи
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
	 * @brief Функция сборки записи для проверок
	 *
	 * @param writer сборщик бинарной записи
	 *
	 */
	void assemble(abc::writer_t & writer) noexcept {
		// Выполняем укладку отображения из четырёх пар
		writer.mapBegin(4);
		// Выполняем укладку имени поля числа
		writer.text("число");
		// Выполняем укладку целого числа со знаком
		writer.number(static_cast <int64_t> (-70000));
		// Выполняем укладку имени поля строки
		writer.text("строка");
		// Выполняем укладку строки
		writer.text("значение");
		// Выполняем укладку имени поля массива
		writer.text("список");
		// Выполняем укладку массива из трёх значений
		writer.arrayBegin(3);
		// Выполняем укладку первого значения массива
		writer.boolean(true);
		// Выполняем укладку вложенного пустого массива
		writer.arrayBegin(0);
		// Выполняем укладку конца вложенного массива
		writer.arrayEnd();
		// Выполняем укладку дробного значения
		writer.number(0.25);
		// Выполняем укладку конца массива
		writer.arrayEnd();
		// Выполняем укладку имени поля вложенного отображения
		writer.text("узел");
		// Выполняем укладку вложенного отображения из одной пары
		writer.mapBegin(1);
		// Выполняем укладку имени поля вложенного отображения числом
		writer.number(static_cast <uint64_t> (42));
		// Выполняем укладку пустого значения
		writer.nul();
		// Выполняем укладку конца вложенного отображения
		writer.mapEnd();
		// Выполняем укладку конца отображения
		writer.mapEnd();
	}
};

/**
 * @brief Проверка разбора записи в дерево и обхода его
 *
 */
TEST(CodecAbcDocument, ParseAndNavigate) {
	// Сборщик бинарной записи
	abc::writer_t writer;
	// Выполняем сборку записи для проверок
	assemble(writer);
	// Выполняем проверку завершённости собранной записи
	ASSERT_TRUE(writer.complete()) << "код отказа: " << abc::message(writer.error());
	// Дерево документа
	abc::document_t document;
	// Выполняем разбор записи в дерево документа
	ASSERT_TRUE(document.parse(writer.record().data(), writer.record().size()))
		<< "код отказа: " << abc::message(document.error());
	// Выполняем получение корня дерева документа
	const abc::document_t::value_t root = document.root();
	// Выполняем проверку вида корня дерева документа
	ASSERT_EQ(root.type(), abc::type_t::MAP);
	// Выполняем проверку количества пар корня дерева документа
	ASSERT_EQ(root.size(), 4u);
	// Выполняем проверку наличия поля числа
	ASSERT_TRUE(root.has("число"));
	// Выполняем проверку отсутствия поля, какого нет
	ASSERT_FALSE(root.has("нету"));
	// Извлекаемое целое число со знаком
	int64_t integer = 0;
	// Выполняем извлечение целого числа со знаком
	ASSERT_TRUE(root.get("число").value(integer));
	// Выполняем проверку извлечённого числа
	ASSERT_EQ(integer, -70000);
	// Выполняем проверку содержимого строкового поля
	ASSERT_EQ(root.get("строка").data(), "значение");
	// Выполняем получение поля массива
	const abc::document_t::value_t list = root.get("список");
	// Выполняем проверку вида поля массива
	ASSERT_EQ(list.type(), abc::type_t::ARRAY);
	// Выполняем проверку количества значений массива
	ASSERT_EQ(list.size(), 3u);
	// Извлекаемое логическое значение
	bool boolean = false;
	// Выполняем извлечение логического значения
	ASSERT_TRUE(list.at(0).value(boolean));
	// Выполняем проверку извлечённого логического значения
	ASSERT_TRUE(boolean);
	// Выполняем проверку вида вложенного массива
	ASSERT_EQ(list.at(1).type(), abc::type_t::ARRAY);
	// Выполняем проверку количества значений вложенного массива
	ASSERT_EQ(list.at(1).size(), 0u);
	// Извлекаемое дробное значение
	double real = 0.0;
	// Выполняем извлечение дробного значения, стоящего за вложенным массивом
	ASSERT_TRUE(list.at(2).value(real));
	// Выполняем проверку извлечённого дробного значения
	ASSERT_DOUBLE_EQ(real, 0.25);
	// Выполняем получение поля вложенного отображения
	const abc::document_t::value_t node = root.get("узел");
	// Выполняем проверку количества пар вложенного отображения
	ASSERT_EQ(node.size(), 1u);
	// Извлекаемое имя поля вложенного отображения
	uint64_t named = 0;
	// Выполняем извлечение имени поля вложенного отображения числом
	ASSERT_TRUE(node.key(0).value(named));
	// Выполняем проверку извлечённого имени поля
	ASSERT_EQ(named, 42u);
	// Выполняем проверку вида значения поля вложенного отображения
	ASSERT_EQ(node.at(0).type(), abc::type_t::NUL);
	// Выполняем проверку недействительности ссылки на значение, какого нет
	ASSERT_FALSE(root.at(9).valid());
}
/**
 * @brief Проверка кругового обхода записи через дерево документа
 *
 * @details Запись, разобранная в дерево и собранная из него заново, обязана совпасть с
 * исходной октет в октет: расхождение означало бы, что подпись при пересборке перестаёт
 * совпадать
 *
 */
TEST(CodecAbcDocument, Roundtrip) {
	// Сборщик исходной бинарной записи
	abc::writer_t writer;
	// Выполняем сборку записи для проверок
	assemble(writer);
	// Дерево документа
	abc::document_t document;
	// Выполняем разбор записи в дерево документа
	ASSERT_TRUE(document.parse(writer.record().data(), writer.record().size()))
		<< "код отказа: " << abc::message(document.error());
	// Сборщик пересобираемой бинарной записи
	abc::writer_t rebuild;
	// Выполняем сборку записи из дерева документа
	ASSERT_TRUE(document.build(rebuild)) << "код отказа: " << abc::message(rebuild.error());
	// Выполняем проверку завершённости пересобранной записи
	ASSERT_TRUE(rebuild.complete()) << "код отказа: " << abc::message(rebuild.error());
	// Выполняем проверку совпадения пересобранной записи с исходной
	ASSERT_EQ(rebuild.record(), writer.record());
}
/**
 * @brief Проверка обращения неопределённой длины в объявленную
 *
 * @details Дерево длины не помнит: оно знает количество детей, а не то, как оно было
 * записано. Оттого пересборка вместимого неопределённой длины даёт длину объявленную, и
 * это намеренно - строгий вид записи иного и не допускает
 *
 */
TEST(CodecAbcDocument, IndefiniteBecomesDefinite) {
	// Сборщик исходной бинарной записи
	abc::writer_t writer;
	// Выполняем укладку массива неопределённой длины
	ASSERT_TRUE(writer.arrayBegin());
	// Выполняем укладку первого значения массива
	ASSERT_TRUE(writer.number(static_cast <uint64_t> (1)));
	// Выполняем укладку второго значения массива
	ASSERT_TRUE(writer.number(static_cast <uint64_t> (2)));
	// Выполняем укладку конца массива
	ASSERT_TRUE(writer.arrayEnd());
	// Дерево документа
	abc::document_t document;
	// Выполняем разбор записи в дерево документа
	ASSERT_TRUE(document.parse(writer.record().data(), writer.record().size()))
		<< "код отказа: " << abc::message(document.error());
	// Выполняем проверку количества значений массива
	ASSERT_EQ(document.root().size(), 2u);
	// Сборщик пересобираемой бинарной записи
	abc::writer_t rebuild;
	// Выполняем сборку записи из дерева документа
	ASSERT_TRUE(document.build(rebuild)) << "код отказа: " << abc::message(rebuild.error());
	// Выполняем проверку того, что пересобранная запись короче исходной
	ASSERT_LT(rebuild.record().size(), writer.record().size());
	// Дерево документа, собранного заново
	abc::document_t again;
	// Выполняем разбор пересобранной записи в дерево документа
	ASSERT_TRUE(again.parse(rebuild.record().data(), rebuild.record().size()))
		<< "код отказа: " << abc::message(again.error());
	// Выполняем проверку совпадения количества узлов деревьев
	ASSERT_EQ(again.nodes(), document.nodes());
}
/**
 * @brief Проверка извлечения чисел с проверкой пределов
 *
 * @details Извлечение сличает само значение с пределами затребованного вида, а не вид
 * хранения с видом затребованным
 *
 */
TEST(CodecAbcDocument, NumberLimits) {
	// Сборщик бинарной записи
	abc::writer_t writer;
	// Выполняем укладку массива из четырёх значений
	ASSERT_TRUE(writer.arrayBegin(4));
	// Выполняем укладку числа, меньшего нуля
	ASSERT_TRUE(writer.number(static_cast <int64_t> (-1)));
	// Выполняем укладку наибольшего целого без знака
	ASSERT_TRUE(writer.number(numeric_limits <uint64_t>::max()));
	// Выполняем укладку дробного числа с дробной частью
	ASSERT_TRUE(writer.number(1.5));
	// Выполняем укладку дробного числа без дробной части
	ASSERT_TRUE(writer.number(8.0));
	// Выполняем укладку конца массива
	ASSERT_TRUE(writer.arrayEnd());
	// Дерево документа
	abc::document_t document;
	// Выполняем разбор записи в дерево документа
	ASSERT_TRUE(document.parse(writer.record().data(), writer.record().size()))
		<< "код отказа: " << abc::message(document.error());
	// Выполняем получение корня дерева документа
	const abc::document_t::value_t root = document.root();
	// Извлекаемое целое без знака
	uint64_t unsignedValue = 0;
	// Извлекаемое целое со знаком
	int64_t signedValue = 0;
	// Извлекаемое дробное число
	double real = 0.0;
	// Выполняем проверку отказа извлечения отрицательного числа видом без знака
	ASSERT_FALSE(root.at(0).value(unsignedValue));
	// Выполняем проверку извлечения отрицательного числа видом со знаком
	ASSERT_TRUE(root.at(0).value(signedValue));
	// Выполняем проверку извлечённого числа
	ASSERT_EQ(signedValue, -1);
	// Выполняем проверку извлечения наибольшего целого видом без знака
	ASSERT_TRUE(root.at(1).value(unsignedValue));
	// Выполняем проверку извлечённого числа
	ASSERT_EQ(unsignedValue, numeric_limits <uint64_t>::max());
	// Выполняем проверку отказа извлечения наибольшего целого видом со знаком
	ASSERT_FALSE(root.at(1).value(signedValue));
	// Выполняем проверку отказа извлечения дробного числа видом целого
	ASSERT_FALSE(root.at(2).value(signedValue));
	// Выполняем проверку извлечения дробного числа видом дробным
	ASSERT_TRUE(root.at(2).value(real));
	// Выполняем проверку извлечённого числа
	ASSERT_DOUBLE_EQ(real, 1.5);
	// Выполняем проверку извлечения дробного без дробной части видом целого
	ASSERT_TRUE(root.at(3).value(signedValue));
	// Выполняем проверку извлечённого числа
	ASSERT_EQ(signedValue, 8);
	// Выполняем проверку отказа извлечения строки видом числа
	{
		// Сборщик бинарной записи со строкой
		abc::writer_t plain;
		// Выполняем укладку строки
		ASSERT_TRUE(plain.text("не число"));
		// Дерево документа со строкой
		abc::document_t text;
		// Выполняем разбор записи в дерево документа
		ASSERT_TRUE(text.parse(plain.record().data(), plain.record().size()));
		// Выполняем проверку отказа извлечения строки видом числа
		ASSERT_FALSE(text.root().value(signedValue));
	}
}
/**
 * @brief Проверка хранения чисел неограниченной ширины в дереве
 *
 */
TEST(CodecAbcDocument, Extensions) {
	// Октеты величины числа
	const vector <uint8_t> magnitude = {0x39, 0x30, 0x01};
	// Сборщик бинарной записи
	abc::writer_t writer;
	// Выполняем укладку массива из двух значений
	ASSERT_TRUE(writer.arrayBegin(2));
	// Выполняем укладку целого числа неограниченной ширины
	ASSERT_TRUE(writer.bignum(magnitude.data(), magnitude.size(), true));
	// Выполняем укладку десятичного числа
	ASSERT_TRUE(writer.decimal(magnitude.data(), magnitude.size(), false, -4));
	// Выполняем укладку конца массива
	ASSERT_TRUE(writer.arrayEnd());
	// Дерево документа
	abc::document_t document;
	// Выполняем разбор записи в дерево документа
	ASSERT_TRUE(document.parse(writer.record().data(), writer.record().size()))
		<< "код отказа: " << abc::message(document.error());
	// Выполняем получение корня дерева документа
	const abc::document_t::value_t root = document.root();
	// Выполняем проверку вида целого числа неограниченной ширины
	ASSERT_EQ(root.at(0).type(), abc::type_t::EXTENDED);
	// Выполняем проверку признака отрицательности величины
	ASSERT_TRUE(root.at(0).negative());
	// Выполняем проверку длины октетов величины
	ASSERT_EQ(root.at(0).data().size(), magnitude.size());
	// Выполняем проверку содержимого октетов величины
	ASSERT_EQ(static_cast <uint8_t> (root.at(0).data().at(0)), magnitude.at(0));
	// Выполняем проверку десятичного порядка целого числа
	ASSERT_EQ(root.at(0).exponent(), 0);
	// Выполняем проверку вида десятичного числа
	ASSERT_EQ(root.at(1).type(), abc::type_t::DECIMAL);
	// Выполняем проверку признака отрицательности величины
	ASSERT_FALSE(root.at(1).negative());
	// Выполняем проверку десятичного порядка величины
	ASSERT_EQ(root.at(1).exponent(), -4);
	// Сборщик пересобираемой бинарной записи
	abc::writer_t rebuild;
	// Выполняем сборку записи из дерева документа
	ASSERT_TRUE(document.build(rebuild)) << "код отказа: " << abc::message(rebuild.error());
	// Выполняем проверку совпадения пересобранной записи с исходной
	ASSERT_EQ(rebuild.record(), writer.record());
}
/**
 * @brief Проверка обхода дерева пропуском поддеревьев
 *
 * @details Переход к соседу идёт сложением размаха поддерева, а не обходом детей его.
 * Размах, посчитанный неверно, увёл бы обход в чужое поддерево
 *
 */
TEST(CodecAbcDocument, SubtreeSkipping) {
	// Сборщик бинарной записи
	abc::writer_t writer;
	// Выполняем укладку массива из трёх значений
	ASSERT_TRUE(writer.arrayBegin(3));
	/**
	 * Выполняем укладку трёх вложенных массивов разной длины
	 */
	for(uint64_t i = 0; i < 3; i++){
		// Выполняем укладку вложенного массива
		ASSERT_TRUE(writer.arrayBegin(i + 1));
		/**
		 * Выполняем укладку значений вложенного массива
		 */
		for(uint64_t j = 0; j <= i; j++)
			// Выполняем укладку значения вложенного массива
			ASSERT_TRUE(writer.number(static_cast <uint64_t> ((i * 10) + j)));
		// Выполняем укладку конца вложенного массива
		ASSERT_TRUE(writer.arrayEnd());
	}
	// Выполняем укладку конца массива
	ASSERT_TRUE(writer.arrayEnd());
	// Дерево документа
	abc::document_t document;
	// Выполняем разбор записи в дерево документа
	ASSERT_TRUE(document.parse(writer.record().data(), writer.record().size()))
		<< "код отказа: " << abc::message(document.error());
	// Выполняем получение корня дерева документа
	const abc::document_t::value_t root = document.root();
	// Выполняем проверку количества значений массива
	ASSERT_EQ(root.size(), 3u);
	/**
	 * Выполняем обход всех вложенных массивов
	 */
	for(size_t i = 0; i < 3; i++){
		// Выполняем получение очередного вложенного массива
		const abc::document_t::value_t nested = root.at(i);
		// Выполняем проверку количества значений вложенного массива
		ASSERT_EQ(nested.size(), (i + 1)) << "номер вложенного массива: " << i;
		/**
		 * Выполняем обход всех значений вложенного массива
		 */
		for(size_t j = 0; j <= i; j++){
			// Извлекаемое целое без знака
			uint64_t value = 0;
			// Выполняем извлечение значения вложенного массива
			ASSERT_TRUE(nested.at(j).value(value)) << "номера: " << i << ", " << j;
			// Выполняем проверку извлечённого значения
			ASSERT_EQ(value, ((i * 10) + j)) << "номера: " << i << ", " << j;
		}
	}
}
/**
 * @brief Проверка отказа разбора негодной записи
 *
 */
TEST(CodecAbcDocument, Failures) {
	// Дерево документа
	abc::document_t document;
	// Октеты записи, оборвавшейся посреди значения
	const vector <uint8_t> data = {0x58, 0x04, 'a', 'b'};
	// Выполняем проверку отказа разбора оборванной записи
	ASSERT_FALSE(document.parse(data.data(), data.size()));
	// Выполняем проверку кода отказа
	ASSERT_EQ(document.error(), abc::error_t::UNEXPECTED_EOF);
	// Выполняем проверку недействительности корня дерева документа
	ASSERT_FALSE(document.root().valid());
	// Выполняем проверку отказа разбора пустой записи
	ASSERT_FALSE(document.parse(nullptr, 0));
	// Выполняем проверку кода отказа
	ASSERT_EQ(document.error(), abc::error_t::EMPTY_RECORD);
}
