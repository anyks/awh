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
	ASSERT_EQ(again.nodes(), document.nodes());
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
	 */
	ASSERT_EQ(document.nodes(), 3ul);
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
