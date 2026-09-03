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
/**
 * @note Заголовок cmath нужен ради std::isnan. Собиратели посвежее подтягивают его
 *       попутно другими заголовками, а gcc 12 с glibc 2.36 и gcc у NetBSD 10 - нет,
 *       и сборка валится. Проверено на стендах Debian 12 и NetBSD 10.1
 */
#include <cmath>
#include <limits>
#include <functional>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <gtest/gtest.h>
#include <codec/abc/document.hpp>
#include <codec/abc/value.hpp>
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
	 * @brief Функция сборки записи для проверок
	 *
	 * @param writer сборщик бинарной записи
	 *
	 */
	void assemble(abc::writer_t & writer) noexcept {
		// Выполняем укладку отображения из четырёх пар
		ASSERT_TRUE(writer.mapBegin(4));
		// Выполняем укладку имени поля числа
		ASSERT_TRUE(writer.text("число"));
		// Выполняем укладку целого числа со знаком
		ASSERT_TRUE(writer.number(static_cast <int64_t> (-70000)));
		// Выполняем укладку имени поля строки
		ASSERT_TRUE(writer.text("строка"));
		// Выполняем укладку строки
		ASSERT_TRUE(writer.text("значение"));
		// Выполняем укладку имени поля массива
		ASSERT_TRUE(writer.text("список"));
		// Выполняем укладку массива из трёх значений
		ASSERT_TRUE(writer.arrayBegin(3));
		// Выполняем укладку первого значения массива
		ASSERT_TRUE(writer.boolean(true));
		// Выполняем укладку вложенного пустого массива
		ASSERT_TRUE(writer.arrayBegin(0));
		// Выполняем укладку конца вложенного массива
		ASSERT_TRUE(writer.arrayEnd());
		// Выполняем укладку дробного значения
		ASSERT_TRUE(writer.number(0.25));
		// Выполняем укладку конца массива
		ASSERT_TRUE(writer.arrayEnd());
		// Выполняем укладку имени поля вложенного отображения
		ASSERT_TRUE(writer.text("узел"));
		// Выполняем укладку вложенного отображения из одной пары
		ASSERT_TRUE(writer.mapBegin(1));
		// Выполняем укладку имени поля вложенного отображения числом
		ASSERT_TRUE(writer.number(static_cast <uint64_t> (42)));
		// Выполняем укладку пустого значения
		ASSERT_TRUE(writer.nul());
		// Выполняем укладку конца вложенного отображения
		ASSERT_TRUE(writer.mapEnd());
		// Выполняем укладку конца отображения
		ASSERT_TRUE(writer.mapEnd());
	}
};

/**
 * @brief Проверка разбора записи в дерево и обхода его
 *
 */
TEST(CodecAbcDocument, ParseAndNavigate) {
	// Сборщик бинарной записи
	abc::writer_t writer(::logger());
	// Выполняем сборку записи для проверок
	assemble(writer);
	// Выполняем проверку завершённости собранной записи
	ASSERT_TRUE(writer.complete()) << "код отказа: " << abc::message(writer.error());
	// Дерево документа
	abc::document_t document(::logger());
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
	abc::writer_t writer(::logger());
	// Выполняем сборку записи для проверок
	assemble(writer);
	// Дерево документа
	abc::document_t document(::logger());
	// Выполняем разбор записи в дерево документа
	ASSERT_TRUE(document.parse(writer.record().data(), writer.record().size()))
		<< "код отказа: " << abc::message(document.error());
	// Сборщик пересобираемой бинарной записи
	abc::writer_t rebuild(::logger());
	// Выполняем сборку записи из дерева документа
	ASSERT_TRUE(document.build(rebuild)) << "код отказа: " << abc::message(rebuild.error());
	// Выполняем проверку завершённости пересобранной записи
	ASSERT_TRUE(rebuild.complete()) << "код отказа: " << abc::message(rebuild.error());
	// Выполняем проверку совпадения пересобранной записи с исходной
	ASSERT_EQ(rebuild.record(), writer.record());
}
/**
 * @brief Проверка извлечения дробных значений, целому не отвечающих
 *
 * @details Договор извлечения велит переносить дробное в целое округлением, а что делать
 * с не-числом и с бесконечностями, округление не определяет вовсе. Договор кодека таков:
 * не-число даёт НОЛЬ, а выходящее за отрезок вида прижимается к его краю, - и отказом
 * извлечение при этом НЕ отвечает, ибо значение числом является
 *
 */
TEST(CodecAbcDocument, RealToIntegerEdges) {
	// Сборщик бинарной записи
	abc::writer_t writer(::logger());
	// Выполняем укладку массива дробных значений
	ASSERT_TRUE(writer.arrayBegin(static_cast <uint64_t> (5)));
	// Выполняем укладку не-числа
	ASSERT_TRUE(writer.number(numeric_limits <double>::quiet_NaN()));
	// Выполняем укладку положительной бесконечности
	ASSERT_TRUE(writer.number(numeric_limits <double>::infinity()));
	// Выполняем укладку отрицательной бесконечности
	ASSERT_TRUE(writer.number(-numeric_limits <double>::infinity()));
	// Выполняем укладку дробного, округляемого с уводом половины от нуля
	ASSERT_TRUE(writer.number(static_cast <double> (2.5)));
	// Выполняем укладку отрицательного дробного
	ASSERT_TRUE(writer.number(static_cast <double> (-2.5)));
	// Выполняем укладку конца массива значений
	ASSERT_TRUE(writer.arrayEnd());
	// Дерево документа
	abc::document_t document(::logger());
	// Выполняем разбор записи в дерево документа
	ASSERT_TRUE(document.parse(writer.record().data(), writer.record().size()))
		<< "код отказа: " << abc::message(document.error());
	// Корень разобранного дерева документа
	const abc::document_t::value_t root = document.root();
	// Извлекаемое целое значение со знаком
	int64_t integer = 1;
	// Извлекаемое целое значение без знака
	uint64_t natural = 1;
	// Извлекаемое дробное значение
	double real = 0.0;
	// Выполняем проверку того, что не-число извлекается дробным
	ASSERT_TRUE(root.at(0).value(real));
	// Выполняем проверку того, что извлечённое дробное числом не является
	ASSERT_TRUE(std::isnan(real));
	// Выполняем проверку того, что не-число целым даёт ноль
	ASSERT_TRUE(root.at(0).value(integer));
	// Выполняем проверку извлечённого из не-числа целого
	ASSERT_EQ(integer, static_cast <int64_t> (0));
	// Выполняем проверку того, что не-число целым без знака даёт ноль
	ASSERT_TRUE(root.at(0).value(natural));
	// Выполняем проверку извлечённого из не-числа целого без знака
	ASSERT_EQ(natural, static_cast <uint64_t> (0));
	// Выполняем проверку того, что бесконечность целым прижимается к краю отрезка
	ASSERT_TRUE(root.at(1).value(integer));
	// Выполняем проверку прижатия положительной бесконечности к верхнему краю
	ASSERT_EQ(integer, numeric_limits <int64_t>::max());
	// Выполняем проверку того, что отрицательная бесконечность прижимается к нижнему краю
	ASSERT_TRUE(root.at(2).value(integer));
	// Выполняем проверку прижатия отрицательной бесконечности к нижнему краю
	ASSERT_EQ(integer, numeric_limits <int64_t>::lowest());
	// Выполняем проверку того, что отрицательная бесконечность без знака даёт ноль
	ASSERT_TRUE(root.at(2).value(natural));
	// Выполняем проверку извлечённого из отрицательной бесконечности целого без знака
	ASSERT_EQ(natural, static_cast <uint64_t> (0));
	// Выполняем проверку округления половины с уводом от нуля
	ASSERT_TRUE(root.at(3).value(integer));
	// Выполняем проверку округлённого положительного дробного
	ASSERT_EQ(integer, static_cast <int64_t> (3));
	// Выполняем проверку округления отрицательной половины с уводом от нуля
	ASSERT_TRUE(root.at(4).value(integer));
	// Выполняем проверку округлённого отрицательного дробного
	ASSERT_EQ(integer, static_cast <int64_t> (-3));
}
/**
 * @brief Проверка кругового обхода записи с двоичными значениями и метками времени
 *
 * @details Круговой обход «Roundtrip» ведётся по записи из чисел, строк и вместимых, а
 * пересборка двоичного значения, опознавателя, метки времени и дробного одинарной
 * точности лежала вне его вовсе. Собираются они иными работами сборщика, и промах в любой
 * из них обратил бы пересборку в запись, подписью не сходящуюся
 *
 */
TEST(CodecAbcDocument, TypedRoundtrip) {
	// Октеты двоичного значения записи
	static const vector <uint8_t> blob = {0x00, 0x11, 0x22, 0x33, 0xFF, 0xFE};
	// Октеты опознавателя записи
	static const vector <uint8_t> uuid = {
		0x55, 0x0E, 0x84, 0x00, 0xE2, 0x9B, 0x41, 0xD4,
		0xA7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00
	};
	// Сборщик исходной бинарной записи
	abc::writer_t writer(::logger());
	// Выполняем укладку массива значений разного вида
	ASSERT_TRUE(writer.arrayBegin(static_cast <uint64_t> (5)));
	// Выполняем укладку двоичного значения записи
	ASSERT_TRUE(writer.blob(blob.data(), blob.size()));
	// Выполняем укладку опознавателя записи
	ASSERT_TRUE(writer.uuid(uuid.data(), uuid.size()));
	// Выполняем укладку метки времени записи
	ASSERT_TRUE(writer.timestamp(static_cast <int64_t> (1755993600)));
	// Выполняем укладку дробного одинарной точности
	ASSERT_TRUE(writer.number(static_cast <float> (0.5f)));
	// Выполняем укладку отрицательной метки времени записи
	ASSERT_TRUE(writer.timestamp(static_cast <int64_t> (-86400)));
	// Выполняем укладку конца массива значений
	ASSERT_TRUE(writer.arrayEnd());
	// Дерево документа
	abc::document_t document(::logger());
	// Выполняем разбор записи в дерево документа
	ASSERT_TRUE(document.parse(writer.record().data(), writer.record().size()))
		<< "код отказа: " << abc::message(document.error());
	// Выполняем проверку количества значений разобранного массива
	ASSERT_EQ(document.root().size(), 5u);
	// Сборщик пересобираемой бинарной записи
	abc::writer_t rebuild(::logger());
	// Выполняем сборку записи из дерева документа
	ASSERT_TRUE(document.build(rebuild)) << "код отказа: " << abc::message(rebuild.error());
	// Выполняем проверку завершённости пересобранной записи
	ASSERT_TRUE(rebuild.complete()) << "код отказа: " << abc::message(rebuild.error());
	/**
	 * Выполняем проверку совпадения пересобранной записи с исходной октет в октет:
	 * расхождение означало бы, что подпись при пересборке перестаёт совпадать
	 */
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
	abc::writer_t writer(::logger());
	// Выполняем укладку массива неопределённой длины
	ASSERT_TRUE(writer.arrayBegin());
	// Выполняем укладку первого значения массива
	ASSERT_TRUE(writer.number(static_cast <uint64_t> (1)));
	// Выполняем укладку второго значения массива
	ASSERT_TRUE(writer.number(static_cast <uint64_t> (2)));
	// Выполняем укладку конца массива
	ASSERT_TRUE(writer.arrayEnd());
	// Дерево документа
	abc::document_t document(::logger());
	// Выполняем разбор записи в дерево документа
	ASSERT_TRUE(document.parse(writer.record().data(), writer.record().size()))
		<< "код отказа: " << abc::message(document.error());
	// Выполняем проверку количества значений массива
	ASSERT_EQ(document.root().size(), 2u);
	// Сборщик пересобираемой бинарной записи
	abc::writer_t rebuild(::logger());
	// Выполняем сборку записи из дерева документа
	ASSERT_TRUE(document.build(rebuild)) << "код отказа: " << abc::message(rebuild.error());
	// Выполняем проверку того, что пересобранная запись короче исходной
	ASSERT_LT(rebuild.record().size(), writer.record().size());
	// Дерево документа, собранного заново
	abc::document_t again(::logger());
	// Выполняем разбор пересобранной записи в дерево документа
	ASSERT_TRUE(again.parse(rebuild.record().data(), rebuild.record().size()))
		<< "код отказа: " << abc::message(again.error());
	// Выполняем проверку совпадения количества узлов деревьев
	ASSERT_EQ(again.size(), document.size());
}
/**
 * @brief Проверка извлечения чисел с проверкой пределов
 *
 * @details Договор извлечения общий у кодеков рамки: вид хранения извлечению не указ.
 * Целое, за отрезок затребованного вида выходящее, переносится младшими разрядами, а
 * дробное округляется по правилам математики с уводом половины ОТ НУЛЯ. Отказом
 * извлечение отвечает лишь тогда, когда значение числом не является вовсе
 *
 * @note Прежде вид хранения соблюдался и приведение выполнялось лишь тогда, когда оно
 *       значения не искажало. Отменено владельцем 20.08.2026 разом у всех кодеков
 *
 */
TEST(CodecAbcDocument, NumberLimits) {
	// Сборщик бинарной записи
	abc::writer_t writer(::logger());
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
	abc::document_t document(::logger());
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
	// Выполняем проверку извлечения отрицательного числа видом без знака
	ASSERT_TRUE(root.at(0).value(unsignedValue));
	// Выполняем проверку переноса отрицательного числа младшими разрядами
	ASSERT_EQ(unsignedValue, numeric_limits <uint64_t>::max());
	// Выполняем проверку извлечения отрицательного числа видом со знаком
	ASSERT_TRUE(root.at(0).value(signedValue));
	// Выполняем проверку извлечённого числа
	ASSERT_EQ(signedValue, -1);
	// Выполняем проверку извлечения наибольшего целого видом без знака
	ASSERT_TRUE(root.at(1).value(unsignedValue));
	// Выполняем проверку извлечённого числа
	ASSERT_EQ(unsignedValue, numeric_limits <uint64_t>::max());
	// Выполняем проверку извлечения наибольшего целого видом со знаком
	ASSERT_TRUE(root.at(1).value(signedValue));
	// Выполняем проверку переноса наибольшего целого младшими разрядами
	ASSERT_EQ(signedValue, -1);
	// Выполняем проверку извлечения дробного числа видом целого
	ASSERT_TRUE(root.at(2).value(signedValue));
	// Выполняем проверку округления половины с уводом от нуля
	ASSERT_EQ(signedValue, 2);
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
		abc::writer_t plain(::logger());
		// Выполняем укладку строки
		ASSERT_TRUE(plain.text("не число"));
		// Дерево документа со строкой
		abc::document_t text(::logger());
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
	abc::writer_t writer(::logger());
	// Выполняем укладку массива из двух значений
	ASSERT_TRUE(writer.arrayBegin(2));
	// Выполняем укладку целого числа неограниченной ширины
	ASSERT_TRUE(writer.bignum(magnitude.data(), magnitude.size(), true));
	// Выполняем укладку десятичного числа
	ASSERT_TRUE(writer.decimal(magnitude.data(), magnitude.size(), false, -4));
	// Выполняем укладку конца массива
	ASSERT_TRUE(writer.arrayEnd());
	// Дерево документа
	abc::document_t document(::logger());
	// Выполняем разбор записи в дерево документа
	ASSERT_TRUE(document.parse(writer.record().data(), writer.record().size()))
		<< "код отказа: " << abc::message(document.error());
	// Выполняем получение корня дерева документа
	const abc::document_t::value_t root = document.root();
	// Выполняем проверку вида целого числа неограниченной ширины
	ASSERT_EQ(root.at(0).type(), abc::type_t::EXTENDED);
	// Выполняем проверку признака отрицательности величины
	ASSERT_TRUE(root.at(0).negative());
	/**
	 * Выполняем проверку октетов величины ЦЕЛИКОМ, а не длины их и первого октета
	 *
	 * @note Прежде сличались длина да октет нулевой, и читатель, переставивший октеты
	 *       со второго, прошёл бы зелёным. Опора обязана лежать вне проверяемого, и
	 *       здесь ею служит сама поданная величина. Найдено 31.08.2026 разбором набора
	 */
	{
		// Октеты величины, снятые с дерева документа
		const auto & taken = root.at(0).data();
		// Длина снятых октетов обязана отвечать поданной
		ASSERT_EQ(taken.size(), magnitude.size());
		/**
		 * Выполняем перебор всех октетов величины
		 */
		for(size_t i = 0; i < magnitude.size(); i++)
			// Очередной снятый октет обязан отвечать поданному
			ASSERT_EQ(static_cast <uint8_t> (taken.at(i)), magnitude.at(i)) << "октет: " << i;
	}
	// Выполняем проверку десятичного порядка целого числа
	ASSERT_EQ(root.at(0).exponent(), 0);
	// Выполняем проверку вида десятичного числа
	ASSERT_EQ(root.at(1).type(), abc::type_t::DECIMAL);
	// Выполняем проверку признака отрицательности величины
	ASSERT_FALSE(root.at(1).negative());
	// Выполняем проверку десятичного порядка величины
	ASSERT_EQ(root.at(1).exponent(), -4);
	// Сборщик пересобираемой бинарной записи
	abc::writer_t rebuild(::logger());
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
	abc::writer_t writer(::logger());
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
	abc::document_t document(::logger());
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
	abc::document_t document(::logger());
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

/**
 * @brief Проверка сборки значения кусками в дереве документа
 *
 * @details Значение, собранное кусками, ложится в дерево ОДНИМ узлом: куски суть части
 *          значения, а не значения, и потребителю дерева они видны быть не должны
 *
 */
TEST(CodecAbcDocument, SegmentedValue) {
	// Сборщик бинарной записи
	abc::writer_t writer(::logger());
	// Выполняем укладку начала отображения из одного поля
	ASSERT_TRUE(writer.mapBegin(1));
	// Выполняем укладку имени поля отображения
	ASSERT_TRUE(writer.text("текст"));
	// Выполняем укладку начала строки, собираемой кусками
	ASSERT_TRUE(writer.textBegin());
	// Выполняем укладку первого куска строки
	ASSERT_TRUE(writer.text("часть один, "));
	// Выполняем укладку второго куска строки
	ASSERT_TRUE(writer.text("часть два"));
	// Выполняем укладку конца строки
	ASSERT_TRUE(writer.textEnd());
	// Выполняем укладку конца отображения
	ASSERT_TRUE(writer.mapEnd());
	// Выполняем проверку завершённости собранной записи
	ASSERT_TRUE(writer.complete());
	// Дерево документа
	abc::document_t document(::logger());
	// Выполняем разбор собранной записи в дерево документа
	ASSERT_TRUE(document.parse(writer.record().data(), writer.record().size()))
		<< "код отказа: " << abc::message(document.error());
	/**
	 * Выполняем проверку количества узлов дерева: отображение, имя поля и значение,
	 * собранное кусками, - три узла, а не четыре и не пять
	 *
	 * @note Мерою здесь `nodes()`, а не `size()`: с 03.09.2026 `size()` выдаёт ДЕТЕЙ
	 *       КОРНЯ, а стережётся тут именно то, что куски НЕ порождают лишних узлов арены
	 */
	ASSERT_EQ(document.nodes(), 3ul);
	// Корень же несёт ровно одну пару: имя поля со значением
	ASSERT_EQ(document.size(), 1ul);
	// Выполняем получение корня дерева документа
	const abc::document_t::value_t root = document.root();
	// Выполняем проверку вида корня дерева
	ASSERT_TRUE(root.is(abc::type_t::MAP));
	// Выполняем проверку количества полей отображения
	ASSERT_EQ(root.size(), 1ul);
	// Выполняем получение значения поля отображения
	const abc::document_t::value_t value = root.get("текст");
	// Выполняем проверку годности значения поля
	ASSERT_TRUE(value.valid());
	// Выполняем проверку вида значения поля
	ASSERT_TRUE(value.is(abc::type_t::STRING));
	// Выполняем проверку содержимого значения, собранного кусками
	ASSERT_EQ(value.data(), "часть один, часть два");
}
/**
 * @brief Проверка кругового хода открытого расширения через дерево документа
 *
 */
TEST(CodecAbcDocument, CustomExtensionRoundtrip){
	// Сборка бинарной записи
	abc::writer_t writer(::logger());
	// Октеты расширения, заведённого потребителем
	const string content = "\xDE\xAD\xBE\xEF";
	// Выполняем укладку начала отображения
	ASSERT_TRUE(writer.mapBegin(static_cast <uint64_t> (1)));
	// Выполняем укладку имени поля отображения
	ASSERT_TRUE(writer.text("ext"));
	// Выполняем укладку открытого расширения значением поля
	ASSERT_TRUE(writer.custom(static_cast <uint64_t> (1000), content.data(), content.size()));
	// Выполняем укладку конца отображения
	ASSERT_TRUE(writer.mapEnd());
	// Выполняем проверку завершённости собранной записи
	ASSERT_TRUE(writer.complete());
	// Выполняем получение собранной записи
	const vector <uint8_t> record = writer.record();
	// Дерево документа
	abc::document_t document(::logger());
	// Выполняем разбор записи в дерево документа
	ASSERT_TRUE(document.parse(record.data(), record.size()))
		<< "код отказа: " << abc::message(document.error());
	// Выполняем получение значения поля отображения
	const abc::document_t::value_t value = document.root().get("ext");
	// Выполняем проверку действительности значения поля
	ASSERT_TRUE(value.valid());
	// Выполняем проверку вида узла значения
	ASSERT_EQ(value.kind(), abc::kind_t::CUSTOM);
	// Выполняем проверку номера подвида расширения
	ASSERT_EQ(value.subtype(), static_cast <uint64_t> (1000));
	// Выполняем проверку содержимого расширения
	ASSERT_EQ(string(value.data()), content);
	// Сборка записи из дерева документа
	abc::writer_t rebuild(::logger());
	/**
	 * Выполняем перезапись дерева документа: путь этот свой, и расширение обязано
	 * пережить и его
	 */
	ASSERT_TRUE(document.build(rebuild)) << "код отказа: " << abc::message(document.error());
	// Выполняем проверку перезаписанной дереву документа записи
	ASSERT_EQ(rebuild.record(), record);
	// Владеющее значение документа
	abc::value_t owning;
	// Выполняем разбор записи во владеющее значение
	ASSERT_TRUE(owning.parse(record.data(), record.size()));
	// Выполняем перезапись владеющего значения
	const vector <uint8_t> rewritten = owning.dump();
	/**
	 * Выполняем проверку того, что перезаписанная запись совпала с исходной: расширение
	 * переживает и дерево документа, и владеющее значение
	 */
	ASSERT_EQ(rewritten, record);
}
/**
 * @brief Проверка последовательного обхода дерева документа
 *
 */
TEST(CodecAbcDocument, SequentialTraversal){
	// Сборка бинарной записи
	abc::writer_t writer(::logger());
	// Выполняем укладку начала массива
	ASSERT_TRUE(writer.arrayBegin(static_cast <uint64_t> (3)));
	// Выполняем укладку первого значения массива
	ASSERT_TRUE(writer.number(static_cast <uint64_t> (1)));
	// Выполняем укладку начала вложенного отображения
	ASSERT_TRUE(writer.mapBegin(static_cast <uint64_t> (2)));
	// Выполняем укладку первого имени поля отображения
	ASSERT_TRUE(writer.text("a"));
	// Выполняем укладку начала вложенного массива
	ASSERT_TRUE(writer.arrayBegin(static_cast <uint64_t> (2)));
	// Выполняем укладку первого значения вложенного массива
	ASSERT_TRUE(writer.number(static_cast <uint64_t> (2)));
	// Выполняем укладку второго значения вложенного массива
	ASSERT_TRUE(writer.number(static_cast <uint64_t> (3)));
	// Выполняем укладку конца вложенного массива
	ASSERT_TRUE(writer.arrayEnd());
	// Выполняем укладку второго имени поля отображения
	ASSERT_TRUE(writer.text("b"));
	// Выполняем укладку значения второго поля отображения
	ASSERT_TRUE(writer.number(static_cast <uint64_t> (4)));
	// Выполняем укладку конца вложенного отображения
	ASSERT_TRUE(writer.mapEnd());
	// Выполняем укладку третьего значения массива
	ASSERT_TRUE(writer.number(static_cast <uint64_t> (5)));
	// Выполняем укладку конца массива
	ASSERT_TRUE(writer.arrayEnd());
	// Дерево документа
	abc::document_t document(::logger());
	// Выполняем разбор записи в дерево документа
	ASSERT_TRUE(document.parse(writer.record().data(), writer.record().size()))
		<< "код отказа: " << abc::message(document.error());
	// Снятые числа дерева документа
	vector <uint64_t> numbers;
	// Снятые имена полей отображения
	vector <string> keys;
	/**
	 * Функция обхода дерева документа
	 *
	 * @param value обходимое значение документа
	 * @param self  ссылка на саму работу обхода
	 *
	 */
	const function <void (const abc::document_t::value_t &, const function <void (const abc::document_t::value_t &)> &)> traverse =
	[&numbers, &keys](const abc::document_t::value_t & value, const function <void (const abc::document_t::value_t &)> & self) noexcept -> void {
		// Если значение является вместимым
		if(value.is(abc::type_t::CONTAINER)){
			/**
			 * Выполняем обход всех значений вместимого
			 */
			for(auto item = value.begin(); item.valid(); item = item.next())
				// Выполняем обход очередного значения вместимого
				self(item);
			// Выходим из работы обхода
			return;
		}
		// Если значение является именем поля отображения
		if(value.keyed())
			// Выполняем накопление снятого имени поля
			keys.push_back(string(value.data()));
		/**
		 * Если значение является числом
		 */
		else {
			// Снимаемое число
			uint64_t result = 0;
			// Выполняем извлечение числа
			ASSERT_TRUE(value.value(result));
			// Выполняем накопление снятого числа
			numbers.push_back(result);
		}
	};
	// Работа обхода дерева документа
	function <void (const abc::document_t::value_t &)> walk;
	// Выполняем заведение работы обхода дерева документа
	walk = [&traverse, &walk](const abc::document_t::value_t & value) noexcept -> void {
		// Выполняем обход очередного значения документа
		traverse(value, walk);
	};
	// Выполняем обход дерева документа
	walk(document.root());
	// Выполняем проверку снятых чисел дерева документа
	ASSERT_EQ(numbers, (vector <uint64_t> {1, 2, 3, 4, 5}));
	/**
	 * Выполняем проверку снятых имён полей отображения: имя стоит таким же узлом, что
	 * и значение, и обход выдаёт его наравне со значениями
	 */
	ASSERT_EQ(keys, (vector <string> {"a", "b"}));
}
/**
 * @brief Проверка границы вместимого при последовательном обходе
 *
 */
TEST(CodecAbcDocument, TraversalStopsAtBound){
	// Сборка бинарной записи
	abc::writer_t writer(::logger());
	// Выполняем укладку начала внешнего массива
	ASSERT_TRUE(writer.arrayBegin(static_cast <uint64_t> (2)));
	// Выполняем укладку начала вложенного массива
	ASSERT_TRUE(writer.arrayBegin(static_cast <uint64_t> (1)));
	// Выполняем укладку значения вложенного массива
	ASSERT_TRUE(writer.number(static_cast <uint64_t> (1)));
	// Выполняем укладку конца вложенного массива
	ASSERT_TRUE(writer.arrayEnd());
	// Выполняем укладку соседа вложенного массива
	ASSERT_TRUE(writer.number(static_cast <uint64_t> (2)));
	// Выполняем укладку конца внешнего массива
	ASSERT_TRUE(writer.arrayEnd());
	// Дерево документа
	abc::document_t document(::logger());
	// Выполняем разбор записи в дерево документа
	ASSERT_TRUE(document.parse(writer.record().data(), writer.record().size()));
	// Выполняем получение вложенного массива
	const abc::document_t::value_t nested = document.root().begin();
	// Выполняем проверку действительности вложенного массива
	ASSERT_TRUE(nested.valid());
	// Выполняем получение единственного значения вложенного массива
	const abc::document_t::value_t item = nested.begin();
	// Выполняем проверку действительности значения вложенного массива
	ASSERT_TRUE(item.valid());
	/**
	 * Выполняем проверку того, что обход вложенного массива на нём и кончается: без
	 * границы вместимого он продолжился бы соседом родителя, и значение `2` попало бы
	 * во вложенный массив, которому оно не принадлежит вовсе
	 */
	ASSERT_FALSE(item.next().valid());
	// Выполняем проверку того, что у внешнего массива сосед вложенного есть
	ASSERT_TRUE(nested.next().valid());
}
/**
 * @brief Проверка правил выбора значения при повторе имени поля отображения
 *
 * @details Правила `FIRST` и `LAST` осуществлены ДЕРЕВОМ, а не потоковым разбором:
 *          выбор одного из двух значений требует видеть отображение целиком, тогда как
 *          события выдаются по одному и назад разбор не ходит. Проверка эта закрепляет
 *          три вещи разом: выбор ведётся тот самый, отображение теряет ровно одну пару,
 *          а уцелевшие соседи повтора остаются на местах
 *
 * @note Соседи взяты в проверку намеренно: изъятие пары переносит уцелевшие узлы на
 *       новое место, и порча размаха поддерева вскрылась бы именно на них, а не на
 *       самом повторе
 *
 */
TEST(CodecAbcDocument, DuplicateRules){
	/**
	 * Работа разбора записи затребованным правилом повтора имени
	 *
	 * @param record разбираемая запись
	 * @param rule   правило обращения с повторяющимся именем поля
	 * @param digest дерево документа, куда ложится разбор
	 * @return       признак успешности разбора
	 */
	auto parse = [](const vector <uint8_t> & record, const abc::duplicate_t rule,
	 abc::document_t & digest) noexcept -> bool {
		// Настройки разбора записи
		abc::reader_t::settings_t settings;
		// Выполняем установку правила обращения с повтором имени поля
		settings.duplicates = rule;
		// Выводим признак успешности разбора записи в дерево документа
		return digest.parse(record.data(), record.size(), settings);
	};
	/**
	 * Отображение из четырёх пар, где имя «b» объявлено дважды:
	 * {"a": 1, "b": 2, "c": 3, "b": 4}
	 */
	const vector <uint8_t> record = {
		0xA4,
		0x41, 'a', 0x01,
		0x41, 'b', 0x02,
		0x41, 'c', 0x03,
		0x41, 'b', 0x04
	};
	// Снимаемое целое значение поля отображения
	uint64_t number = 0;
	/**
	 * Правилом `FIRST` оставляется значение, встреченное первым
	 */
	{
		// Дерево документа
		abc::document_t document(::logger());
		// Выполняем разбор записи в дерево документа
		ASSERT_TRUE(parse(record, abc::duplicate_t::FIRST, document))
			<< "код отказа: " << abc::message(document.error());
		// Выполняем получение корня дерева документа
		const abc::document_t::value_t root = document.root();
		// Выполняем проверку того, что отображение потеряло ровно одну пару
		ASSERT_EQ(root.size(), 3u);
		// Выполняем проверку того, что оставлено значение, встреченное первым
		ASSERT_TRUE(root.get("b").value(number));
		// Выполняем проверку величины оставленного значения
		ASSERT_EQ(number, 2u);
		// Выполняем проверку того, что сосед повтора остался на месте
		ASSERT_TRUE(root.get("a").value(number));
		// Выполняем проверку величины значения соседа повтора
		ASSERT_EQ(number, 1u);
		// Выполняем проверку того, что сосед, стоявший ЗА повтором, остался на месте
		ASSERT_TRUE(root.get("c").value(number));
		// Выполняем проверку величины значения соседа, стоявшего за повтором
		ASSERT_EQ(number, 3u);
	}
	/**
	 * Правилом `LAST` оставляется значение, встреченное последним
	 */
	{
		// Дерево документа
		abc::document_t document(::logger());
		// Выполняем разбор записи в дерево документа
		ASSERT_TRUE(parse(record, abc::duplicate_t::LAST, document))
			<< "код отказа: " << abc::message(document.error());
		// Выполняем получение корня дерева документа
		const abc::document_t::value_t root = document.root();
		// Выполняем проверку того, что отображение потеряло ровно одну пару
		ASSERT_EQ(root.size(), 3u);
		// Выполняем проверку того, что оставлено значение, встреченное последним
		ASSERT_TRUE(root.get("b").value(number));
		// Выполняем проверку величины оставленного значения
		ASSERT_EQ(number, 4u);
		// Выполняем проверку того, что соседи повтора остались на местах
		ASSERT_TRUE(root.get("a").value(number));
		// Выполняем проверку величины значения соседа, стоявшего перед повтором
		ASSERT_EQ(number, 1u);
		// Выполняем проверку того, что сосед, стоявший между повторами, остался на месте
		ASSERT_TRUE(root.get("c").value(number));
		// Выполняем проверку величины значения соседа, стоявшего между повторами
		ASSERT_EQ(number, 3u);
	}
	/**
	 * Правилом `KEEP` отображение остаётся нетронутым, а доступ по имени выдаёт первое
	 */
	{
		// Дерево документа
		abc::document_t document(::logger());
		// Выполняем разбор записи в дерево документа
		ASSERT_TRUE(parse(record, abc::duplicate_t::KEEP, document))
			<< "код отказа: " << abc::message(document.error());
		// Выполняем проверку того, что отображение сохранило все четыре пары
		ASSERT_EQ(document.root().size(), 4u);
		// Выполняем проверку того, что доступ по имени выдаёт значение, встреченное первым
		ASSERT_TRUE(document.root().get("b").value(number));
		// Выполняем проверку величины выданного значения
		ASSERT_EQ(number, 2u);
	}
	/**
	 * Правилом `REFUSE` запись отвергается потоковым разбором, не дойдя до дерева
	 */
	{
		// Дерево документа
		abc::document_t document(::logger());
		// Выполняем проверку того, что разбор записи отвечен отказом
		ASSERT_FALSE(parse(record, abc::duplicate_t::REFUSE, document));
		// Выполняем проверку того, что отказ объявлен повтором имени поля
		ASSERT_EQ(document.error(), abc::error_t::DUPLICATE_KEY);
	}
}
/**
 * @brief Проверка выбора значения при повторе имени во вложенном отображении
 *
 * @details Проверка предыдущая берёт отображение плоское, а оно есть случай слабейший:
 *          изъятие пары у корня не задевает ничьих размахов, ибо размах корня снимается
 *          последним. Здесь же повтор стоит ВНУТРИ отображения, за каким следуют соседи,
 *          и неверное осуществление правила пережило бы плоскую проверку, свалившись
 *          лишь здесь: размах вместившего отображения считался бы по узлам, изъятым из
 *          дерева
 *
 * @note Значением повтора взято вместимое намеренно: изымается тогда не один узел, а
 *       поддерево целиком, и ошибка в размахе пары вскрылась бы порчею соседей
 *
 */
/**
 * @brief Проверка сличения имён полей, не лежащих в хранилище октетов
 *
 * @details Имя поля отображения записью ABC бывает значением ЛЮБОГО вида, а сличение
 * имён деревом идёт тремя дорогами: содержимым из хранилища октетов (строки, двоичные
 * данные, числа неограниченной ширины), размахом поддерева (вместимые) и разрядной
 * записью значения (числа, отметки времени, логические значения). Правила повтора прежде
 * поверялись ЛИШЬ строковыми именами, то есть одною дорогой из трёх
 *
 * @note Дороги эти взаимно исключающи, и ошибка в любой из двух непроверенных означала
 * бы либо потерю различного имени, либо сохранение повторного - молча в обоих случаях
 *
 */
TEST(CodecAbcDocument, DuplicateRulesByKeyKind){
	/**
	 * Работа разбора записи затребованным правилом повтора имени
	 *
	 * @param record разбираемая запись
	 * @param rule   правило обращения с повторяющимся именем поля
	 * @param digest дерево документа, куда ложится разбор
	 * @return       признак успешности разбора
	 */
	auto parse = [](const vector <uint8_t> & record, const abc::duplicate_t rule,
	 abc::document_t & digest) noexcept -> bool {
		// Настройки разбора записи
		abc::reader_t::settings_t settings;
		// Выполняем установку правила обращения с повтором имени поля
		settings.duplicates = rule;
		// Выводим признак успешности разбора записи в дерево документа
		return digest.parse(record.data(), record.size(), settings);
	};
	/**
	 * Отображение с ЧИСЛОВЫМИ именами полей, где имя «40» объявлено дважды:
	 * {30: 10, 40: 20, 50: 30, 40: 40}
	 *
	 * @note Имена взяты ВЕДОМЫМ октетом, а не самой меткой: имена, уместившиеся в метку,
	 * различаются длиною записи и до сличения разрядных записей дело не доводят вовсе
	 *
	 * @note Числовое имя в хранилище октетов НЕ ложится - оно лежит разрядной записью
	 * внутри узла, и сличается потому третьей дорогой
	 */
	const vector <uint8_t> numbers = {
		0xA4,
		0x18, 0x1E, 0x0A,
		0x18, 0x28, 0x14,
		0x18, 0x32, 0x18, 0x1E,
		0x18, 0x28, 0x18, 0x28
	};
	/**
	 * Правилом `FIRST` числовое имя отсеивается по разрядной записи
	 */
	{
		// Дерево документа
		abc::document_t document(::logger());
		// Выполняем разбор записи в дерево документа
		ASSERT_TRUE(parse(numbers, abc::duplicate_t::FIRST, document))
			<< "код отказа: " << abc::message(document.error());
		// Выполняем проверку того, что отображение потеряло ровно одну пару
		ASSERT_EQ(document.root().size(), 3u);
	}
	/**
	 * Правилом `KEEP` числовые имена остаются все
	 */
	{
		// Дерево документа
		abc::document_t document(::logger());
		// Выполняем разбор записи в дерево документа
		ASSERT_TRUE(parse(numbers, abc::duplicate_t::KEEP, document))
			<< "код отказа: " << abc::message(document.error());
		// Выполняем проверку того, что отображение сохранило все пары
		ASSERT_EQ(document.root().size(), 4u);
	}
	/**
	 * Отображение с числовыми именами БЕЗ повтора: {30: 10, 40: 20, 50: 30}
	 *
	 * @note Поверка эта - вторая половина договора: сличение обязано находить не только
	 * совпадение, но и различие. Сличение, отвечающее совпадением всегда, отсеяло бы
	 * здесь две пары из трёх, а на записи выше вело бы себя ровно так же
	 */
	const vector <uint8_t> distinct = {
		0xA3,
		0x18, 0x1E, 0x0A,
		0x18, 0x28, 0x14,
		0x18, 0x32, 0x18, 0x1E
	};
	/**
	 * Правилом `FIRST` различные числовые имена остаются все
	 */
	{
		// Дерево документа
		abc::document_t document(::logger());
		// Выполняем разбор записи в дерево документа
		ASSERT_TRUE(parse(distinct, abc::duplicate_t::FIRST, document))
			<< "код отказа: " << abc::message(document.error());
		// Выполняем проверку того, что отображение сохранило все пары
		ASSERT_EQ(document.root().size(), 3u);
	}
	/**
	 * Отображение с числовыми именами, расходящимися ЛИШЬ старшим словом
	 *
	 * @details Разрядная запись числа лежит двумя словами узла, и младшее из них служит
	 * заодно длиною содержимого - оно сличается прежде, отдельною поверкой. Старшее же
	 * сличается ЛИШЬ разрядной записью, и различие в нём есть единственный случай, где
	 * поверка эта решает. Имена ниже - 0x0000000100000005 и 0x0000000200000005 - сходятся
	 * младшим словом до октета и расходятся только старшим
	 *
	 * @note Без этой поверки они сошли бы за одно имя, и правило `FIRST` отсеяло бы
	 * пару, повтором НЕ являющуюся, - потеря данных молча
	 */
	const vector <uint8_t> words = {
		0xA2,
		0x1B, 0x05, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0A,
		0x1B, 0x05, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x14
	};
	/**
	 * Правилом `FIRST` имена, расходящиеся старшим словом, остаются оба
	 */
	{
		// Дерево документа
		abc::document_t document(::logger());
		// Выполняем разбор записи в дерево документа
		ASSERT_TRUE(parse(words, abc::duplicate_t::FIRST, document))
			<< "код отказа: " << abc::message(document.error());
		// Выполняем проверку того, что отображение сохранило обе пары
		ASSERT_EQ(document.root().size(), 2u);
	}
	/**
	 * Вместимое именем поля отображения НЕ принимается вовсе
	 *
	 * @details Дорога сличения размахом поддерева в `Document::identical` оттого сегодня
	 * недостижима: разбор отвергает такую запись прежде, чем дело дойдёт до сличения имён.
	 * Поверка эта закрепляет ИМЕННО отказ - чтобы отступление от него не прошло молча
	 */
	{
		// Отображение, где именем поля стоит перечень: {[1]: 10}
		const vector <uint8_t> record = {0xA1, 0x81, 0x01, 0x0A};
		// Дерево документа
		abc::document_t document(::logger());
		// Разбор записи этой обязан ответить отказом при всяком правиле повтора
		ASSERT_FALSE(parse(record, abc::duplicate_t::FIRST, document));
		// Отказ обязан быть именно о вместимом, ставшем именем поля
		ASSERT_EQ(document.error(), abc::error_t::INVALID_KEY);
	}
}
/**
 * @brief Проверка намеренного расхождения слоёв в понятии тождества имени
 *
 * @details Тождество имени поля понимается кодеком ДВОЯКО, и расхождение объявлено
 * намеренным у самого `Document::identical`. Разбиратель при правиле `duplicate_t::REFUSE`
 * сличает ПОЛНУЮ запись имени вместе с меткою: два имени, писанные меткой разной ширины,
 * суть для него имена разные. Дерево же записи не помнит вовсе и сличает значения: те же
 * два имени для него одно, и правила `FIRST` и `LAST` отсеивают одно из них
 *
 * @note Утверждение это стояло в заголовке доводом, но не поверялось ничем. Расхождение
 * слоёв - как раз то, что расходится молча: обе стороны его выглядят исправными порознь
 *
 */
TEST(CodecAbcDocument, KeyIdentityDivergesByLayer){
	/**
	 * Отображение, где одно и то же число 5 стоит именем поля ДВАЖДЫ, но писано
	 * метками разной ширины: {5: 10, 5: 20}
	 *
	 * @note Первое имя уместилось в саму метку, второе писано ведомым октетом. Запись
	 * это годная вне строгого вида: строгий вид отверг бы её кодом `NON_MINIMAL_TAG`
	 */
	const vector <uint8_t> record = {
		0xA2,
		0x05, 0x0A,
		0x18, 0x05, 0x14
	};
	/**
	 * Разбиратель сличает ПОЛНУЮ запись имени: имена эти для него различны
	 */
	{
		// Разборщик бинарной записи
		abc::reader_t reader(::logger());
		// Выполняем получение настроек разбора
		abc::reader_t::settings_t settings = reader.settings();
		// Выполняем установку отказа от повтора имени поля
		settings.duplicates = abc::duplicate_t::REFUSE;
		// Выполняем установку настроек разбора
		reader.settings(settings);
		// Разбор обязан ответить согласием: записи имён расходятся шириною метки
		ASSERT_TRUE(reader.feed(record.data(), record.size(), true))
			<< "код отказа: " << abc::message(reader.error());
	}
	/**
	 * Дерево сличает ЗНАЧЕНИЯ имён: имена эти для него одно и то же
	 */
	{
		// Настройки разбора записи
		abc::reader_t::settings_t settings;
		// Выполняем установку правила выбора первого встреченного значения
		settings.duplicates = abc::duplicate_t::FIRST;
		// Дерево документа
		abc::document_t document(::logger());
		// Выполняем разбор записи в дерево документа
		ASSERT_TRUE(document.parse(record.data(), record.size(), settings))
			<< "код отказа: " << abc::message(document.error());
		// Отображение обязано потерять пару: имена сличены равными
		ASSERT_EQ(document.root().size(), 1u);
		// Выполняем получение корня дерева документа
		const abc::document_t::value_t root = document.root();
		// Снимаемое целое имя поля отображения
		uint64_t name = 0;
		// Имя единственной уцелевшей пары обязано сняться целым
		ASSERT_TRUE(root.key(0).value(name));
		// Имя это обязано отвечать числу, писанному обеими метками
		ASSERT_EQ(name, static_cast <uint64_t> (5));
		// Снимаемое целое значение поля отображения
		uint64_t number = 0;
		// Оставлено обязано быть значение, встреченное первым
		ASSERT_TRUE(root.at(0).value(number));
		// Извлечённое значение обязано отвечать первому встреченному
		ASSERT_EQ(number, static_cast <uint64_t> (10));
	}
}
/**
 * @brief Проверка единообразия извлечения двух записей одного числа деревом
 *
 * @details Решение владельца от 30.08.2026 общее на все кодеки рамки, а тело приведения
 * у кодека ABC ДВА: своё у владеющего значения и своё у дерева документа. Правка,
 * внесённая в одно из них, оставила бы слои расходящимися - запись, прочитанная деревом,
 * давала бы одно, а тою же записью собранное значение - другое
 *
 * @note Проверка эта нарочно повторяет `CodecAbcValue.WholeRealMatchesInteger` иным
 * слоем: копии договора расходятся молча, и стеречь надобно КАЖДУЮ
 *
 */
TEST(CodecAbcDocument, WholeRealMatchesInteger){
	/**
	 * @brief Работа сличения извлечения двух записей одного числа деревом
	 *
	 * @param record разбираемая запись числа
	 * @param title  название сличаемого случая
	 * @param result извлечённое из записи число без знака
	 */
	auto extract = [](const vector <uint8_t> & record, const string & title,
	 uint64_t & result) noexcept -> void {
		// Дерево документа
		abc::document_t document(::logger());
		// Выполняем сброс извлекаемого числа
		result = 0;
		// Выполняем разбор записи в дерево документа
		ASSERT_TRUE(document.parse(record.data(), record.size()))
			<< title << ", код отказа: " << abc::message(document.error());
		// Число обязано извлечься числом без знака
		ASSERT_TRUE(document.root().value(result)) << title;
	};
	/**
	 * Выполняем перебор чисел, дробной части у каких нет
	 */
	for(const int64_t number : {static_cast <int64_t> (0), static_cast <int64_t> (7),
	 static_cast <int64_t> (300), static_cast <int64_t> (65536), static_cast <int64_t> (-1),
	 static_cast <int64_t> (-300)}){
		// Собираемая запись числа целым видом
		vector <uint8_t> integer;
		// Выполняем укладку числа целым видом
		abc::integer(integer, number);
		// Собираемая запись того же числа дробным видом
		vector <uint8_t> real;
		// Выполняем укладку того же числа дробным видом
		abc::real(real, static_cast <double> (number));
		// Число, извлечённое из записи целого вида
		uint64_t natural = 0;
		// Выполняем извлечение числа из записи целого вида
		extract(integer, "целое " + to_string(number), natural);
		// Число, извлечённое из записи дробного вида
		uint64_t other = 0;
		// Выполняем извлечение числа из записи дробного вида
		extract(real, "дробное " + to_string(number), other);
		// Извлечённые из обеих записей числа обязаны совпасть
		ASSERT_EQ(natural, other) << "число: " << number;
	}
	/**
	 * Число, дробная часть у какого есть НА ДЕЛЕ, идёт прежним путём
	 *
	 * @note Половина вторая правила: без неё `300.5` уехало бы дорогой целого молча
	 */
	{
		// Собираемая запись дробного числа с дробной частью
		vector <uint8_t> record;
		// Выполняем укладку дробного числа с дробной частью
		abc::real(record, 300.5);
		// Извлекаемое число без знака
		uint64_t natural = 0;
		// Выполняем извлечение числа из записи дробного вида
		extract(record, "дробное 300.5", natural);
		// Извлечённое число обязано отвечать округлению от нуля
		ASSERT_EQ(natural, static_cast <uint64_t> (301));
	}
	/**
	 * Число за разрядностью хранимого целого идёт пределом вида
	 */
	{
		// Собираемая запись дробного числа за разрядностью целого
		vector <uint8_t> record;
		// Выполняем укладку дробного числа за разрядностью целого
		abc::real(record, ::ldexp(1.0, 70));
		// Извлекаемое число без знака
		uint64_t natural = 0;
		// Выполняем извлечение числа из записи дробного вида
		extract(record, "дробное 2^70", natural);
		// Извлечённое число обязано отвечать верхнему пределу вида
		ASSERT_EQ(natural, numeric_limits <uint64_t>::max());
	}
}
TEST(CodecAbcDocument, DuplicateRulesNested){
	// Сборщик исходной бинарной записи
	abc::writer_t writer(::logger());
	// Выполняем получение настроек сборки записи
	abc::writer_t::settings_t assembly = writer.settings();
	/**
	 * Выполняем снятие отказа на повтор имени поля у сборщика. Умолчанием сборщик
	 * повтор отвергает, а проверке этой запись с повтором нужна исходным веществом
	 *
	 * @note Признак этот значит ОТКАЗ, а не дозволение: полярность его обратна
	 * одноимённой настройке разбора, где `duplicates` есть правило обращения
	 */
	assembly.duplicates = false;
	// Выполняем установку настроек сборки записи
	writer.settings(assembly);
	// Выполняем укладку внешнего отображения из двух пар
	ASSERT_TRUE(writer.mapBegin(static_cast <uint64_t> (2)));
	// Выполняем укладку имени поля вложенного отображения
	ASSERT_TRUE(writer.text("узел"));
	// Выполняем укладку вложенного отображения из трёх пар
	ASSERT_TRUE(writer.mapBegin(static_cast <uint64_t> (3)));
	// Выполняем укладку имени поля, объявляемого дважды
	ASSERT_TRUE(writer.text("дважды"));
	// Выполняем укладку массива значений первым значением повтора
	ASSERT_TRUE(writer.arrayBegin(static_cast <uint64_t> (2)));
	// Выполняем укладку первого значения массива
	ASSERT_TRUE(writer.number(static_cast <uint64_t> (10)));
	// Выполняем укладку второго значения массива
	ASSERT_TRUE(writer.number(static_cast <uint64_t> (11)));
	// Выполняем укладку конца массива значений
	ASSERT_TRUE(writer.arrayEnd());
	// Выполняем укладку имени поля соседа повтора
	ASSERT_TRUE(writer.text("сосед"));
	// Выполняем укладку значения соседа повтора
	ASSERT_TRUE(writer.number(static_cast <uint64_t> (7)));
	// Выполняем укладку имени поля, объявляемого дважды
	ASSERT_TRUE(writer.text("дважды"));
	// Выполняем укладку второго значения повтора
	ASSERT_TRUE(writer.number(static_cast <uint64_t> (99)));
	// Выполняем укладку конца вложенного отображения
	ASSERT_TRUE(writer.mapEnd());
	// Выполняем укладку имени поля соседа вложенного отображения
	ASSERT_TRUE(writer.text("хвост"));
	// Выполняем укладку значения соседа вложенного отображения
	ASSERT_TRUE(writer.text("конец"));
	// Выполняем укладку конца внешнего отображения
	ASSERT_TRUE(writer.mapEnd());
	// Настройки разбора записи
	abc::reader_t::settings_t settings;
	// Выполняем установку правила оставления последнего значения
	settings.duplicates = abc::duplicate_t::LAST;
	// Дерево документа
	abc::document_t document(::logger());
	// Выполняем разбор записи в дерево документа
	ASSERT_TRUE(document.parse(writer.record().data(), writer.record().size(), settings))
		<< "код отказа: " << abc::message(document.error());
	// Выполняем получение корня дерева документа
	const abc::document_t::value_t root = document.root();
	// Выполняем проверку того, что внешнее отображение осталось нетронутым
	ASSERT_EQ(root.size(), 2u);
	// Выполняем получение соседа вложенного отображения
	const abc::document_t::value_t tail = root.get("хвост");
	/**
	 * Выполняем проверку соседа, стоящего ЗА вложенным отображением. Изъятие поддерева
	 * из вложенного отображения сдвигает узлы, за ним стоящие, и порча размаха вскрылась
	 * бы именно здесь: сосед выдал бы не своё содержимое либо не нашёлся вовсе
	 */
	ASSERT_EQ(tail.data(), "конец");
	// Выполняем получение вложенного отображения
	const abc::document_t::value_t node = root.get("узел");
	// Выполняем проверку того, что вложенное отображение потеряло ровно одну пару
	ASSERT_EQ(node.size(), 2u);
	// Снимаемое целое значение поля отображения
	uint64_t number = 0;
	// Выполняем проверку того, что оставлено значение, встреченное последним
	ASSERT_TRUE(node.get("дважды").value(number));
	// Выполняем проверку величины оставленного значения
	ASSERT_EQ(number, 99u);
	// Выполняем проверку того, что сосед повтора остался на месте
	ASSERT_TRUE(node.get("сосед").value(number));
	// Выполняем проверку величины значения соседа повтора
	ASSERT_EQ(number, 7u);
	/**
	 * Выполняем пересборку записи из дерева документа.
	 *
	 * Пересборка есть поверка укладу дерева строжайшая: собиратель обходит узлы по
	 * размахам их и объявленным длинам, и всякое расхождение между количеством детей
	 * и настоящим числом узлов оборачивается незавершённой записью
	 */
	abc::writer_t rebuild(::logger());
	// Выполняем сборку записи из дерева документа
	ASSERT_TRUE(document.build(rebuild)) << "код отказа: " << abc::message(rebuild.error());
	// Выполняем проверку завершённости пересобранной записи
	ASSERT_TRUE(rebuild.complete()) << "код отказа: " << abc::message(rebuild.error());
	// Дерево документа, собранное из пересобранной записи
	abc::document_t repeated(::logger());
	// Выполняем разбор пересобранной записи в дерево документа
	ASSERT_TRUE(repeated.parse(rebuild.record().data(), rebuild.record().size()))
		<< "код отказа: " << abc::message(repeated.error());
	// Выполняем проверку того, что повтора в пересобранной записи более нет
	ASSERT_EQ(repeated.root().get("узел").size(), 2u);
}
/**
 * @brief Поданное число вместе с собранной из него записью
 *
 * @note Число несётся рядом с записью нарочно: оно и служит ОПОРОЮ ВНЕШНЕЙ при
 *       сличении слоёв, см. `CodecAbcDocument.LayersAgreeOnNumberEdges`
 *
 * @note Тип объявлен в БЕЗЫМЯННОМ пространстве: проверки кодеков собираются одной
 *       программой, и тип файлового охвата дал бы порчу кучи вдали от места
 */
namespace {
	/**
	 * Source Структура поданного числа вместе с записью его
	 */
	template <typename T>
	struct Source {
		// Собранная запись числа
		const vector <uint8_t> & record;
		// Само поданное число
		T value;
	};
}
/**
 * @brief Проверка схождения слоёв на всём наборе краевых чисел
 *
 * @details Проверка `WholeRealMatchesInteger` сличает ДВЕ ЗАПИСИ одного числа внутри
 *          одного слоя, а эта - ОДНУ запись двумя слоями: дерево документа и владеющее
 *          значение, из того же дерева собранное. Договор извлечения объявлен обоими
 *          слоями порознь, и разойтись копии его не вправе
 *
 * @note Набор взят ШИРОКИЙ намеренно. Расхождение слоёв у кодека XML было объявлено
 *       расхождением на переполнении по первому же случаю, а оказалось расхождением
 *       грамматики числа целиком - пяти разных родов. Семи записей, на каких столбцы
 *       сходились прежде, не увидели бы и там: краевое поведение живёт у краёв
 */
TEST(CodecAbcDocument, LayersAgreeOnNumberEdges){
	/**
	 * @brief Работа сличения извлечения одной записи двумя слоями
	 *
	 * @param record сличаемая запись числа
	 * @param title  наименование сличаемой записи
	 */
	auto same = [](const auto & source, const string & title) noexcept -> void {
		// Запись, собранная из поданного числа
		const vector <uint8_t> & record = source.record;
		// Дерево документа, снятое с поданной записи
		abc::document_t document(::logger());
		// Запись обязана сняться деревом
		ASSERT_TRUE(document.parse(record.data(), record.size())) << title;
		// Владеющее значение, собранное из того же дерева
		const abc::value_t value(document.root());
		/**
		 * Выполняем сличение снятого с ПОДАННЫМ числом - опорою внешней
		 *
		 * @note Сличения слоёв меж собою для поверки НЕ ДОСТАЁТ: оба слоя выведены из
		 *       ОДНОГО разбора, и ошибись он молча - оба ошибутся одинаково, а проверка
		 *       зазеленеет на утраченном. Сличалась бы НЕПОДВИЖНОСТЬ, а не верность
		 *
		 * @note Признак выведен 31.08.2026 вместе с Василием: опора обязана лежать ВНЕ
		 *       проверяемого. У него тем же признаком пал доклад «разбор - сборка -
		 *       разбор, расхождений ноль», где опорою служило то же, что и проверялось
		 */
		{
			// Число, снятое обратно видом поданного
			decltype(source.value) back = decltype(source.value)();
			// Снятие обязано быть успешным
			ASSERT_TRUE(document.root().value(back)) << title;
			// Снятое обязано отвечать поданному: сличаем печатью ради `nan`
			ASSERT_EQ(::testing::PrintToString(back), ::testing::PrintToString(source.value)) << title;
		}
		/**
		 * @brief Работа сличения слоёв одним затребованным видом
		 *
		 * @tparam T затребованный вид числа
		 */
		auto alike = [&](auto probe) noexcept -> void {
			// Число, извлечённое слоем дерева документа
			decltype(probe) tree = decltype(probe)();
			// Число, извлечённое слоем владеющего значения
			decltype(probe) owned = decltype(probe)();
			// Оба слоя обязаны ответить извлечению одинаково
			ASSERT_EQ(document.root().value(tree), value.value(owned)) << title;
			/**
			 * Извлечённые слоями числа обязаны совпасть
			 *
			 * @note Сличение ведётся ЧЕРЕЗ ЗАПИСЬ, а не сличением чисел: «не число» не
			 * равно самому себе по стандарту, и прямое сличение объявляло бы расхождение
			 * слоёв там, где оба слоя отдали одно и то же. Ловушка эта поймала себя сама
			 * на первом же прогоне
			 */
			ASSERT_EQ(::testing::PrintToString(tree), ::testing::PrintToString(owned)) << title;
		};
		// Выполняем сличение слоёв видом целого без знака
		alike(static_cast <uint64_t> (0));
		// Выполняем сличение слоёв видом целого со знаком
		alike(static_cast <int64_t> (0));
		// Выполняем сличение слоёв видом дробным
		alike(static_cast <double> (0));
		/**
		 * Выполняем сличение слоёв видом истинности: вид этот у числа берёт иную дорогу
		 * вовсе, а договор его объявлен обоими слоями точно так же
		 */
		alike(false);
	};
	// Буфер собираемой записи числа
	vector <uint8_t> record;
	/**
	 * @brief Работа сборки записи дробного числа
	 *
	 * @param value укладываемое дробное число
	 */
	auto fractional = [&record](const double value) noexcept -> Source <double> {
		// Выполняем очистку буфера собираемой записи
		record.clear();
		// Выполняем укладку записи дробного числа
		abc::real(record, value);
		// Выводим собранную запись вместе с поданным числом
		return Source <double> {record, value};
	};
	/**
	 * @brief Работа сборки записи целого числа
	 *
	 * @param value укладываемое целое число
	 */
	auto whole = [&record](const int64_t value) noexcept -> Source <int64_t> {
		// Выполняем очистку буфера собираемой записи
		record.clear();
		// Выполняем укладку записи целого числа
		abc::integer(record, value);
		// Выводим собранную запись вместе с поданным числом
		return Source <int64_t> {record, value};
	};
	// Выполняем сличение слоёв на числах обыкновенных
	same(whole(7), "целое семь");
	same(fractional(7.0), "дробное семь");
	same(fractional(0.5), "дробное половина");
	same(fractional(-0.5), "дробное минус половина");
	/**
	 * Выполняем сличение слоёв на нуле обоих знаков: знак нуля дробного вида кодек XML
	 * терял ОДНИМ слоем из двух, и сличение это стережёт ту же утрату у ABC
	 */
	same(fractional(0.0), "нуль дробный");
	same(fractional(-0.0), "минус нуль дробный");
	/**
	 * Выполняем сличение слоёв на значениях особых: числом они не являются вовсе, и
	 * приведение их к целому у самого языка неопределено
	 */
	same(fractional(numeric_limits <double>::quiet_NaN()), "не число");
	same(fractional(numeric_limits <double>::infinity()), "бесконечность");
	same(fractional(- numeric_limits <double>::infinity()), "минус бесконечность");
	/**
	 * Выполняем сличение слоёв у САМИХ границ дороги целого: ниже `ldexp(1., 63)` берётся
	 * дорога целого со знаком, ниже `ldexp(1., 64)` - без знака, а шире обеих идёт предел
	 * вида. Расхождение слоёв, буде оно случится, сядет ИМЕННО здесь
	 */
	same(fractional(::ldexp(1.0, 63)), "два в шестьдесят третьей");
	same(fractional(::ldexp(1.0, 64)), "два в шестьдесят четвёртой");
	same(fractional(- ::ldexp(1.0, 63)), "минус два в шестьдесят третьей");
	same(fractional(::ldexp(1.0, 70)), "два в семидесятой");
	same(fractional(numeric_limits <double>::max()), "предел дробного вида");
	/**
	 * Наименьшее поднормальное закрепляется ЗАКОННО, хотя память и предупреждает, что
	 * закреплять поднормальными нельзя: у NetBSD на aarch64 их нет вовсе
	 *
	 * @note Предупреждение то относится к кодекам ТЕКСТОВЫМ, где разбор числа ведёт
	 *       арифметика (`strtod`), а обнуление поднормальных правит именно ею. ABC же
	 *       несёт разрядную ЗАПИСЬ числа и снимает её обратно `memcpy` - девять октетов,
	 *       метка и восемь разрядов, - и арифметики над значением не ведёт вовсе
	 *
	 * @note Проверено щупом 31.08.2026 приёмом Василия: машина без поднормальных
	 *       воспроизведена на своей же разрядом `FZ` (24-й) регистра `FPCR`, стенд ARM64
	 *       не понадобился. Признак печатался рядом с итогом («поднормальные живы: да /
	 *       НЕТ»), иначе мерилась бы пустота. В обоих случаях число снято ТОЧНО
	 */
	same(fractional(numeric_limits <double>::denorm_min()), "наименьшее поднормальное");
	/**
	 * Выполняем сличение слоёв на целых у границ разрядной сетки
	 */
	same(whole(-1), "целое минус один");
	/**
	 * Края у порога 2^53 берутся НЕкруглыми нарочно: выше порога всякое дробное уже
	 * целое, и на круглых степенях двойки согласие слоёв сходится всегда, ничего не
	 * поверяя. Указано Василием 31.08.2026
	 */
	same(whole(static_cast <uint64_t> ((1ull << 53) + 1)), "два в пятьдесят третьей плюс один целым");
	same(fractional(static_cast <double> ((1ull << 53) + 1)), "два в пятьдесят третьей плюс один дробным");
	same(whole(static_cast <uint64_t> ((1ull << 53) - 1)), "два в пятьдесят третьей минус один целым");
	same(whole(numeric_limits <int64_t>::min()), "нижний предел целого со знаком");
	same(whole(numeric_limits <int64_t>::max()), "верхний предел целого со знаком");
}
/**
 * @brief Проверка того, что место отказа разбора доходит до потребителя
 *
 * @details Разбиратель заводится разбором и им же разрушается, и место отказа, у него
 * снятое, без переноса пропадало бы вместе с ним. Работа `errorLocation` заведена ради
 * согласия договоров кодеков: у шести текстовых кодеков рамки она зовётся тем же именем
 *
 * @note Поверка требует смещения НЕНУЛЕВОГО: запись оборвана посреди значения, и нуль
 * означал бы, что место не перенесено вовсе, а взято у пустого вида
 *
 */
TEST(CodecAbcDocument, ErrorLocationReachesTheConsumer){
	// Сборка бинарной записи
	abc::writer_t writer(::logger());
	// Выполняем укладку начала массива из трёх значений
	ASSERT_TRUE(writer.arrayBegin(static_cast <uint64_t> (3)));
	// Выполняем укладку первого значения массива
	ASSERT_TRUE(writer.number(static_cast <uint64_t> (1)));
	// Выполняем укладку второго значения массива
	ASSERT_TRUE(writer.number(static_cast <uint64_t> (2)));
	// Выполняем укладку третьего значения массива
	ASSERT_TRUE(writer.number(static_cast <uint64_t> (3)));
	// Выполняем укладку конца массива
	ASSERT_TRUE(writer.arrayEnd());
	// Собранная запись
	const vector <uint8_t> record = writer.record();
	// Дерево документа
	abc::document_t document(::logger());
	// Место отказа у свежего дерева обязано быть пустым
	ASSERT_EQ(document.errorLocation().offset, abc::NO_OFFSET);
	// Оборванная запись обязана быть отвергнута разбором
	ASSERT_FALSE(document.parse(record.data(), record.size() - 1));
	// Поводом отказа обязан стоять обрыв записи посреди значения
	ASSERT_EQ(document.error(), abc::error_t::UNEXPECTED_EOF);
	// Место отказа обязано указывать внутрь записи, а не оставаться пустым
	ASSERT_NE(document.errorLocation().offset, abc::NO_OFFSET);
	// Место отказа обязано лежать внутри поданной записи
	ASSERT_LE(document.errorLocation().offset, static_cast <uint64_t> (record.size()));
	// Очистка дерева обязана снимать и место отказа
	document.clear();
	// Снятое место отказа обязано вернуться к пустому виду
	ASSERT_EQ(document.errorLocation().offset, abc::NO_OFFSET);
}
/**
 * @brief Проверка согласия договора дерева документа с прочими кодеками рамки
 *
 * @details Владелец затребовал 02.09.2026 согласия договоров всех семи кодеков на уровне
 * общего API. Поверка держит ту его часть, какая у ABC заведена ради согласия: `size`,
 * `empty`, `dump` двумя видами, `settings` парою и `errorLocation`. Без неё добавки эти
 * стояли бы незваными - набор их не трогал, а согласие проверялось бы чтением заголовков
 *
 * @note Поверяется РАБОТА добавок, а не наличие их: наличие блюдёт сам собиратель, а
 * молчаливо negodный `dump` прошёл бы поверку наличия и провалил бы круговой ход
 *
 */
TEST(CodecAbcDocument, ContractAgreesWithTheOtherCodecs){
	// Сборка бинарной записи
	abc::writer_t writer(::logger());
	// Выполняем укладку начала массива из трёх значений
	ASSERT_TRUE(writer.arrayBegin(static_cast <uint64_t> (3)));
	/**
	 * Выполняем укладку всех значений массива
	 */
	for(uint64_t i = 1; i < 4; i++)
		// Выполняем укладку очередного значения массива
		ASSERT_TRUE(writer.number(i));
	// Выполняем укладку конца массива
	ASSERT_TRUE(writer.arrayEnd());
	// Собранная запись
	const vector <uint8_t> record = writer.record();
	// Дерево документа
	abc::document_t document(::logger());
	// Свежее дерево обязано быть пустым
	ASSERT_TRUE(document.empty());
	// Узлов у свежего дерева быть не должно
	ASSERT_EQ(document.size(), static_cast <size_t> (0));
	// Настройки разбора с отказом на повтор имени поля
	abc::reader_t::settings_t parsing;
	// Выполняем объявление строгого вида записи
	parsing.canonical = true;
	// Выполняем установку настроек разбора деревом
	document.settings(parsing);
	// Хранимые настройки обязаны отвечать установленным
	ASSERT_TRUE(document.settings().canonical);
	// Запись обязана разбираться в дерево документа
	ASSERT_TRUE(document.parse(record.data(), record.size()))
		<< "код отказа: " << abc::message(document.error());
	// Разобранное дерево пустым быть не должно
	ASSERT_FALSE(document.empty());
	// Узлов у дерева обязано выйти четыре: массив и три значения его
	ASSERT_EQ(document.nodes(), static_cast <size_t> (4));
	// Значений же у корня три: сам массив корнем и является
	ASSERT_EQ(document.size(), static_cast <size_t> (3));
	// Собранная деревом запись обязана совпасть с исходной октет в октет
	ASSERT_EQ(document.dump(), record);
	// Настройки сборки записи
	abc::writer_t::settings_t assembling;
	// Собранная затребованными настройками запись обязана совпасть с исходной
	ASSERT_EQ(document.dump(assembling), record);
	// Очистка обязана возвращать дерево к пустому виду
	document.clear();
	// Очищенное дерево обязано быть пустым
	ASSERT_TRUE(document.empty());
	// Узлов у очищенного дерева быть не должно
	ASSERT_EQ(document.size(), static_cast <size_t> (0));
	/**
	 * Настройки разбора очистка трогать НЕ должна: они принадлежат дереву, а не
	 * разобранному в него содержимому
	 */
	ASSERT_TRUE(document.settings().canonical);
}
/**
 * @brief Проверка того, что отметка времени извлекается обоими целыми видами
 *
 * @details Отметка времени есть целое, и извлекаться она обязана и видом со знаком, и
 * видом без знака. Дерево разбора отдавало её лишь видом со знаком и отвечало ОТКАЗОМ
 * виду без знака, тогда как владеющее значение отдавало обоими: одна и та же запись,
 * прочтённая двумя видами ОДНОГО кодека, давала разные ответы
 *
 * @note Поверка ведёт оба вида ABC СРАЗУ и сличает их между собой. В том и суть: порознь
 * всякий из них выглядел бы согласным сам с собою, и расхождение видно лишь рядом
 *
 * @note Дробным видом отметка не извлекается ни у одного из двух, и поверка это
 * закрепляет: согласие в отказе есть такая же часть договора, как и согласие в ответе
 *
 */
TEST(CodecAbcDocument, TimestampExtractsByBothIntegerKinds){
	// Отметка времени, укладываемая в запись
	const int64_t stamp = static_cast <int64_t> (1756800000);
	// Сборка бинарной записи
	abc::writer_t writer(::logger());
	// Отметка времени обязана укладываться сборкою
	ASSERT_TRUE(writer.timestamp(stamp));
	// Собранная запись
	const vector <uint8_t> record = writer.record();
	// Дерево документа
	abc::document_t document(::logger());
	// Запись обязана разбираться в дерево документа
	ASSERT_TRUE(document.parse(record.data(), record.size()))
		<< "код отказа: " << abc::message(document.error());
	// Владеющее значение
	abc::value_t owned;
	// Выполняем установку объекта логирования
	owned.setLogger(::logger());
	// Запись обязана разбираться во владеющее значение
	ASSERT_TRUE(owned.parse(record.data(), record.size()));
	// Указание на корень дерева разбора
	const abc::document_t::value_t view = document.root();
	// Извлекаемое целое со знаком
	int64_t integer = 0;
	// Отметка обязана извлекаться видом со знаком у дерева разбора
	ASSERT_TRUE(view.value(integer));
	// Извлечённая отметка обязана отвечать уложенной
	ASSERT_EQ(integer, stamp);
	// Выполняем сброс извлекаемого целого со знаком
	integer = 0;
	// Отметка обязана извлекаться видом со знаком у владеющего значения
	ASSERT_TRUE(owned.value(integer));
	// Извлечённая отметка обязана отвечать уложенной
	ASSERT_EQ(integer, stamp);
	// Извлекаемое целое без знака у дерева разбора
	uint64_t natural = 0;
	// Отметка обязана извлекаться видом без знака у дерева разбора
	ASSERT_TRUE(view.value(natural));
	// Извлечённая отметка обязана отвечать уложенной
	ASSERT_EQ(natural, static_cast <uint64_t> (stamp));
	// Извлекаемое целое без знака у владеющего значения
	uint64_t other = 0;
	// Отметка обязана извлекаться видом без знака у владеющего значения
	ASSERT_TRUE(owned.value(other));
	// Оба вида кодека обязаны отвечать ОДИНАКОВО
	ASSERT_EQ(natural, other);
	// Извлекаемое дробное у дерева разбора
	double real = 0.0;
	// Дробным видом отметка извлекаться не должна
	ASSERT_FALSE(view.value(real));
	// Извлекаемое дробное у владеющего значения
	double another = 0.0;
	// Оба вида кодека обязаны отвечать отказом одинаково
	ASSERT_FALSE(owned.value(another));
}
/**
 * @brief Проверка согласия хранимого счёта узлов с обходом дерева
 *
 * @details Учёт узлов у дерева поверялся ДО СИХ ПОР лишь случаем: подмена мест учёта
 * красила проверки о правиле повтора имени да о прививке, до счёта касательства не
 * имеющие. Сторожить учёт случаем нельзя - переставь завтра эти проверки, и он
 * останется без сторожа вовсе, а узнать о том будет неоткуда
 *
 * @note Довод подан Николаем 03.09.2026: он нашёл у себя ровно то же и завёл поверку
 * прямо на счёт. Правило общее - подмена, красящая ЧУЖУЮ проверку, наблюдаемости не
 * доказывает: доказывает её проверка, для того заведённая
 *
 * @note Поверка ведёт ДВА хода по одному дереву: счёт, деревом хранимый, сличается с
 * перебором обходом. Один ход не поверяет ничего - он и есть то, что поверяется
 *
 */
TEST(CodecAbcDocument, StoredCountsAgreeWithTheWalk){
	// Сборка бинарной записи
	abc::writer_t writer(::logger());
	// Выполняем укладку начала отображения из двух пар
	ASSERT_TRUE(writer.mapBegin(static_cast <uint64_t> (2)));
	// Выполняем укладку имени первой пары
	ASSERT_TRUE(writer.text(string{"перечень"}));
	// Выполняем укладку начала перечня из трёх значений
	ASSERT_TRUE(writer.arrayBegin(static_cast <uint64_t> (3)));
	/**
	 * Выполняем укладку значений перечня
	 */
	for(uint64_t i = 0; i < 3; i++)
		// Выполняем укладку очередного значения перечня
		ASSERT_TRUE(writer.number(i));
	// Выполняем укладку конца перечня
	ASSERT_TRUE(writer.arrayEnd());
	// Выполняем укладку имени второй пары
	ASSERT_TRUE(writer.text(string{"вложенное"}));
	// Выполняем укладку начала вложенного отображения из одной пары
	ASSERT_TRUE(writer.mapBegin(static_cast <uint64_t> (1)));
	// Выполняем укладку имени пары вложенного отображения
	ASSERT_TRUE(writer.text(string{"поле"}));
	// Выполняем укладку значения пары вложенного отображения
	ASSERT_TRUE(writer.boolean(true));
	// Выполняем укладку конца вложенного отображения
	ASSERT_TRUE(writer.mapEnd());
	// Выполняем укладку конца отображения
	ASSERT_TRUE(writer.mapEnd());
	// Собранная запись
	const vector <uint8_t> record = writer.record();
	// Дерево документа
	abc::document_t document(::logger());
	// Запись обязана разбираться в дерево документа
	ASSERT_TRUE(document.parse(record.data(), record.size()))
		<< "код отказа: " << abc::message(document.error());
	/**
	 * @brief Работа обхода дерева со счётом узлов и поверкой счёта детей
	 *
	 * @param value обходимое значение дерева документа
	 * @param self  ссылка на саму работу обхода
	 * @return      количество узлов обойденного поддерева
	 *
	 */
	auto walk = [](const abc::document_t::value_t & value, auto & self) noexcept -> size_t {
		// Количество узлов обойденного поддерева, считая сам узел
		size_t result = 1;
		// Если значение вместимым не является, поддерева у него нет
		if(!value.is(abc::type_t::CONTAINER))
			// Выводим количество узлов обойденного поддерева
			return result;
		// Количество детей, обходом насчитанное
		size_t counted = 0;
		/**
		 * Выполняем обход всех значений вместимого
		 */
		for(size_t i = 0; i < value.size(); i++){
			// Выполняем учёт обойденного ребёнка вместимого
			counted++;
			/**
			 * Если вместимое является отображением, имя поля есть такой же узел
			 */
			if(value.type() == abc::type_t::MAP)
				// Выполняем учёт узлов поддерева имени поля отображения
				result += self(value.key(i), self);
			// Выполняем учёт узлов поддерева значения вместимого
			result += self(value.at(i), self);
		}
		// Счёт детей, деревом хранимый, обязан отвечать насчитанному обходом
		EXPECT_EQ(counted, value.size());
		// Выводим количество узлов обойденного поддерева
		return result;
	};
	// Выполняем обход дерева документа со счётом узлов
	const size_t walked = walk(document.root(), walk);
	/**
	 * Счёт узлов, деревом хранимый, обязан отвечать насчитанному обходом
	 *
	 * @note Именно этот сторож и отсутствовал: счёт узлов арены отдаётся полем, а обход
	 *       идёт размахами, и разойтись они могут молча
	 *
	 * @note Мерою здесь `nodes()`, а не `size()`: с 03.09.2026 `size()` выдаёт детей
	 *       корня, а сличается тут длина АРЕНЫ с числом узлов, пройденных обходом
	 */
	ASSERT_EQ(walked, document.nodes());
	// Корень обязан нести ровно две пары
	ASSERT_EQ(document.root().size(), static_cast <size_t> (2));
}
/**
 * @brief Проверка согласия двух взглядов кодека на один путь
 *
 * @details У ABC два взгляда на один и тот же документ: дерево разбора `Document` -
 *          только для чтения, поверх арены узлов, - и владеющее значение `Value`. Путь
 *          понимать обязаны ОБА и понимать ОДИНАКОВО, иначе одна и та же запись,
 *          прочитанная двумя путями кодека, отдаст разное
 *
 * @note Работа `Document::at(path)` заведена 03.09.2026: прежде дерево разбора путей не
 *       понимало ВОВСЕ, тогда как владеющее значение их понимало. Вскрылось это при вводе
 *       ABC в общие проверки договора кодеков, где путь спрашивают у ДОКУМЕНТА
 *
 * @note Обход у обоих ведёт общий посредник `abc::segment` вместе с `abc::indexed`, и
 *       заведены они общими намеренно: правила пути нужны четверым - извлечению и
 *       заведению у владеющего значения, извлечению и опросу наличия у дерева разбора. Но
 *       общее устройство согласия НЕ ДОКАЗЫВАЕТ: зовы у них разные, и разойтись они могут
 *       на переходе, а не на разборе звена. Оттого проверка сличает ОТВЕТЫ, а не устройство
 *
 * @note Пути взяты так, чтобы бить в каждое правило договора порознь: ведущая черта и её
 *       отсутствие, пустое звено внутри пути, имя поля из цифр, номер перечня, номер с
 *       ведущим нулём, номер за пределом перечня, путь в никуда и путь пустой
 *
 */
TEST(CodecAbcDocument, BothViewsAgreeOnEveryPath) {
	// Собираемое значение документа
	abc::value_t root, holder, nested, listing;
	// Выполняем заведение значения, лежащего внутри поля с пустым именем
	ASSERT_TRUE(nested.insert(abc::value_t(string("b")), abc::value_t(string("глубокое"))));
	// Выполняем заведение поля с пустым именем
	ASSERT_TRUE(holder.insert(abc::value_t(string("")), nested));
	// Выполняем заведение соседнего поля с тем же именем
	ASSERT_TRUE(holder.insert(abc::value_t(string("b")), abc::value_t(string("соседнее"))));
	// Выполняем заведение поля, чьё имя записано цифрою
	ASSERT_TRUE(holder.insert(abc::value_t(string("1")), abc::value_t(string("имя-один"))));
	/**
	 * Выполняем заведение перечня из трёх значений
	 */
	for(size_t i = 0; i < 3; i++)
		// Выполняем заведение очередного значения перечня
		ASSERT_TRUE(listing.push(abc::value_t(string("эл-") + to_string(i))));
	// Выполняем заведение перечня полем вместилища
	ASSERT_TRUE(holder.insert(abc::value_t(string("сп")), listing));
	// Выполняем заведение вместилища полем корня
	ASSERT_TRUE(root.insert(abc::value_t(string("a")), holder));
	// Выполняем выдачу собранной записи
	const vector <uint8_t> record = root.dump();
	// Объект дерева разбора документа
	abc::document_t document(::logger());
	// Выполняем разбор собранной записи деревом
	ASSERT_TRUE(document.parse(record.data(), record.size()))
		<< "код отказа: " << abc::message(document.error());
	/**
	 * Пути, бьющие в каждое правило договора порознь
	 */
	const vector <string> paths = {
		"/a/b", "a/b", "/a//b", "a//b", "/a/1", "/a/сп/0", "/a/сп/2",
		"/a/сп/01", "/a/сп/9", "/нет", "/a/сп", "/a", ""
	};
	/**
	 * Выполняем сличение ответов двух взглядов на всяком пути
	 */
	for(const string & path : paths){
		// Значение, выданное владеющим деревом
		const abc::value_t & owned = root.at(path);
		// Значение, выданное деревом разбора
		const abc::document_t::value_t parsed = document.at(path);
		// Признак годности значения у владеющего дерева
		const bool own = (owned.type() != abc::type_t::UNDEFINED);
		// Годность значения обязана совпасть у обоих взглядов
		ASSERT_EQ(own, parsed.valid()) << "путь: " << path;
		// Опрос наличия обязан отвечать извлечению
		ASSERT_EQ(document.has(path), parsed.valid()) << "путь: " << path;
		// Если значения по пути нет, сличать более нечего
		if(!own)
			// Переходим к следующему пути
			continue;
		// Вид значения обязан совпасть у обоих взглядов
		ASSERT_EQ(owned.type(), parsed.type()) << "путь: " << path;
		/**
		 * Если значение является строкой, сличаем содержимое её
		 */
		if(owned.type() == abc::type_t::STRING)
			// Содержимое строки обязано совпасть у обоих взглядов
			ASSERT_EQ(owned.text(), string(parsed.data())) << "путь: " << path;
		/**
		 * Если значение является вместимым, сличаем число значений его
		 */
		if(owned.type() == abc::type_t::MAP || owned.type() == abc::type_t::ARRAY)
			// Число значений вместимого обязано совпасть у обоих взглядов
			ASSERT_EQ(owned.size(), parsed.size()) << "путь: " << path;
	}
}
