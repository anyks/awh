/**
 * @file common.cpp
 * @date 2026-08-18
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Проверки общих объявлений бинарного контейнера ABC — описания кодов отказов,
 *        названия видов узлов и значений, сборка и разбор ведущего октета проволочной
 *        записи и выбор наименьшей записи значения
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <limits>
#include <unordered_set>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <gtest/gtest.h>
#include <codec/abc/common.hpp>
#include <codec/abc/header.hpp>
#include <codec/abc/chunk.hpp>
#include <codec/abc/index.hpp>
#include <codec/abc/signature.hpp>
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <sys/fmk.hpp>

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
	[[maybe_unused]] const log_t * logger() noexcept {
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
};

/**
 * @brief Проверка описаний кодов отказов разбора
 *
 * @details Всякий объявленный код обязан нести своё описание: описание неизвестного
 * кода у объявленного означало бы, что отказ выдаётся, а объяснить его нечем
 *
 */
TEST(CodecAbcCommon, Messages) {
	/**
	 * Коды отказов разбора записи документа
	 */
	/**
	 * Перебор ведётся ЧИСЛАМИ от нуля до последнего кода перечня, а не рукописным
	 * списком: список от перечня отстаёт молча. Здесь он и отстал - кончался он кодом
	 * `ENCRYPTION_FAILED`, а пять кодов за ним (`REFUSED_SIGNATURE`, `SIGNING_FAILED`,
	 * `INVALID_SEGMENT`, `INVALID_EXTENSION`, `NON_MINIMAL_TAG`) не сличались НИ РАЗУ
	 */
	unordered_set <string> seen;
	/**
	 * Выполняем перебор всех кодов отказов перечня
	 */
	for(uint16_t code = 0; code <= static_cast <uint16_t> (abc::error_t::NON_MINIMAL_TAG); code++){
		// Выполняем получение описания кода отказа
		const char * message = abc::message(static_cast <abc::error_t> (code));
		// Выполняем проверку, что описание кода отказа выдано
		ASSERT_NE(message, nullptr) << "код отказа: " << code;
		// Выполняем проверку, что описание кода отказа не пусто
		EXPECT_NE(strlen(message), 0u) << "код отказа: " << code;
		// Выполняем проверку, что объявленный код не выдаёт описания неизвестного
		EXPECT_STRNE(message, "unknown error") << "код отказа: " << code;
		// Выполняем проверку, что описание кода отказа не повторяется
		EXPECT_TRUE(seen.emplace(message).second) << "код отказа: " << code
			<< ", описание повторяется: " << message;
	}
	/**
	 * Выполняем проверку границы перебора ЗАМЕРОМ: код, стоящий сразу за последним,
	 * обязан выдавать описание неизвестного
	 *
	 * @note Утверждение это стережёт САМУ ПРОВЕРКУ: заведи перечень новый код за
	 *       `NON_MINIMAL_TAG` - и перебор выше его не достанет, ибо граница названа
	 *       именем. Сличать границу глазом дороже, чем одним сличением числа
	 */
	ASSERT_STREQ(abc::message(static_cast <abc::error_t> (
	 static_cast <uint16_t> (abc::error_t::NON_MINIMAL_TAG) + 1)), "unknown error");
}
/**
 * @brief Проверка названий видов узлов документа
 *
 */
TEST(CodecAbcCommon, KindNames) {
	/**
	 * Виды узлов документа
	 */
	const vector <abc::kind_t> kinds = {
		abc::kind_t::NONE, abc::kind_t::NUL, abc::kind_t::BOOL, abc::kind_t::NUMBER,
		abc::kind_t::STRING, abc::kind_t::BLOB, abc::kind_t::TIME, abc::kind_t::UUID,
		abc::kind_t::ARRAY, abc::kind_t::MAP
	};
	// Набор уже встреченных названий видов узлов
	unordered_set <string> seen;
	/**
	 * Выполняем перебор всех видов узлов документа
	 */
	for(const abc::kind_t kind : kinds){
		// Выполняем получение названия вида узла
		const char * name = abc::name(kind);
		// Выполняем проверку, что название вида узла выдано
		ASSERT_NE(name, nullptr) << "вид узла: " << static_cast <uint32_t> (kind);
		// Выполняем проверку, что название вида узла не пусто
		ASSERT_NE(strlen(name), 0u) << "вид узла: " << static_cast <uint32_t> (kind);
		// Выполняем проверку, что название вида узла не повторяется
		ASSERT_TRUE(seen.emplace(name).second) << "название повторяется: " << name;
	}
}
/**
 * @brief Проверка названий видов значений документа
 *
 * @details Название выдаётся точному виду: сборный вид назван быть не может, ибо он
 * есть множество видов, а не вид
 *
 */
TEST(CodecAbcCommon, TypeNames) {
	/**
	 * Точные виды значений документа
	 */
	const vector <abc::type_t> types = {
		abc::type_t::UNDEFINED, abc::type_t::NUL, abc::type_t::BOOL, abc::type_t::STRING,
		abc::type_t::BLOB, abc::type_t::ARRAY, abc::type_t::MAP, abc::type_t::TIME,
		abc::type_t::UUID, abc::type_t::INT8, abc::type_t::INT16, abc::type_t::INT32,
		abc::type_t::INT64, abc::type_t::UINT8, abc::type_t::UINT16, abc::type_t::UINT32,
		abc::type_t::UINT64, abc::type_t::FLOAT, abc::type_t::DOUBLE,
		abc::type_t::EXTENDED, abc::type_t::DECIMAL
	};
	// Набор уже встреченных названий видов значений
	unordered_set <string> seen;
	/**
	 * Выполняем перебор всех точных видов значений документа
	 */
	for(const abc::type_t type : types){
		// Выполняем получение названия вида значения
		const char * name = abc::name(type);
		// Выполняем проверку, что название вида значения выдано
		ASSERT_NE(name, nullptr) << "вид значения: " << static_cast <uint32_t> (type);
		// Выполняем проверку, что название вида значения не пусто
		ASSERT_NE(strlen(name), 0u) << "вид значения: " << static_cast <uint32_t> (type);
		// Выполняем проверку, что название вида значения не повторяется
		ASSERT_TRUE(seen.emplace(name).second) << "название повторяется: " << name;
	}
}
/**
 * @brief Проверка разрядной раскладки видов значений
 *
 * @details Виды заданы разрядами, а сборные виды - объединением разрядов. Наложение
 * двух точных видов друг на друга означало бы, что вопрос о виде значения неразрешим
 *
 */
TEST(CodecAbcCommon, TypeBits) {
	/**
	 * Точные виды значений документа, кроме отсутствующего
	 */
	const vector <abc::type_t> types = {
		abc::type_t::NUL, abc::type_t::BOOL, abc::type_t::STRING, abc::type_t::BLOB,
		abc::type_t::ARRAY, abc::type_t::MAP, abc::type_t::TIME, abc::type_t::UUID,
		abc::type_t::INT8, abc::type_t::INT16, abc::type_t::INT32, abc::type_t::INT64,
		abc::type_t::UINT8, abc::type_t::UINT16, abc::type_t::UINT32, abc::type_t::UINT64,
		abc::type_t::FLOAT, abc::type_t::DOUBLE, abc::type_t::EXTENDED, abc::type_t::DECIMAL
	};
	// Собранное объединение разрядов всех точных видов
	uint32_t united = 0;
	/**
	 * Выполняем перебор всех точных видов значений документа
	 */
	for(const abc::type_t type : types){
		// Выполняем получение разрядов вида значения
		const uint32_t bits = static_cast <uint32_t> (type);
		// Выполняем проверку, что вид значения занимает ровно один разряд
		ASSERT_EQ(bits & (bits - 1), 0u) << "вид значения занимает не один разряд: " << abc::name(type);
		// Выполняем проверку, что разряд вида значения ещё не занят
		ASSERT_EQ(united & bits, 0u) << "разряд вида значения занят: " << abc::name(type);
		// Выполняем добавление разряда вида значения к объединению
		united |= bits;
	}
	// Выполняем проверку, что целое со знаком собрано из целых со знаком
	ASSERT_EQ(static_cast <uint32_t> (abc::type_t::SIGNED), static_cast <uint32_t> (abc::type_t::INT8) |
	          static_cast <uint32_t> (abc::type_t::INT16) | static_cast <uint32_t> (abc::type_t::INT32) |
	          static_cast <uint32_t> (abc::type_t::INT64));
	// Выполняем проверку, что целое без знака собрано из целых без знака
	ASSERT_EQ(static_cast <uint32_t> (abc::type_t::UNSIGNED), static_cast <uint32_t> (abc::type_t::UINT8) |
	          static_cast <uint32_t> (abc::type_t::UINT16) | static_cast <uint32_t> (abc::type_t::UINT32) |
	          static_cast <uint32_t> (abc::type_t::UINT64));
	// Выполняем проверку, что целое со знаком и без знака не пересекаются
	ASSERT_EQ(static_cast <uint32_t> (abc::type_t::SIGNED) & static_cast <uint32_t> (abc::type_t::UNSIGNED), 0u);
	// Выполняем проверку, что число вмещает все числовые виды
	ASSERT_EQ(static_cast <uint32_t> (abc::type_t::NUMBER) & static_cast <uint32_t> (abc::type_t::INT),
	          static_cast <uint32_t> (abc::type_t::INT));
	// Выполняем проверку, что число вмещает дробные виды
	ASSERT_EQ(static_cast <uint32_t> (abc::type_t::NUMBER) & static_cast <uint32_t> (abc::type_t::REAL),
	          static_cast <uint32_t> (abc::type_t::REAL));
	// Выполняем проверку, что число не вмещает строку
	ASSERT_EQ(static_cast <uint32_t> (abc::type_t::NUMBER) & static_cast <uint32_t> (abc::type_t::STRING), 0u);
	// Выполняем проверку, что отрезок собран из строки и двоичных данных
	ASSERT_EQ(static_cast <uint32_t> (abc::type_t::SEGMENT), static_cast <uint32_t> (abc::type_t::STRING) |
	          static_cast <uint32_t> (abc::type_t::BLOB));
	// Выполняем проверку, что вместимое собрано из массива и отображения
	ASSERT_EQ(static_cast <uint32_t> (abc::type_t::CONTAINER), static_cast <uint32_t> (abc::type_t::ARRAY) |
	          static_cast <uint32_t> (abc::type_t::MAP));
}
/**
 * @brief Проверка огрубления вида значения до вида узла
 *
 * @details Всякое число, каким бы видом оно ни хранилось, есть узел вида `NUMBER`
 *
 */
TEST(CodecAbcCommon, KindOfType) {
	/**
	 * Числовые виды значений документа
	 */
	const vector <abc::type_t> numbers = {
		abc::type_t::INT8, abc::type_t::INT16, abc::type_t::INT32, abc::type_t::INT64,
		abc::type_t::UINT8, abc::type_t::UINT16, abc::type_t::UINT32, abc::type_t::UINT64,
		abc::type_t::FLOAT, abc::type_t::DOUBLE, abc::type_t::EXTENDED, abc::type_t::DECIMAL
	};
	/**
	 * Выполняем перебор всех числовых видов значений документа
	 */
	for(const abc::type_t type : numbers)
		// Выполняем проверку, что числовой вид огрубляется до узла числа
		ASSERT_EQ(abc::kind(type), abc::kind_t::NUMBER) << "вид значения: " << abc::name(type);
	// Выполняем проверку огрубления пустого значения
	ASSERT_EQ(abc::kind(abc::type_t::NUL), abc::kind_t::NUL);
	// Выполняем проверку огрубления логического значения
	ASSERT_EQ(abc::kind(abc::type_t::BOOL), abc::kind_t::BOOL);
	// Выполняем проверку огрубления строки
	ASSERT_EQ(abc::kind(abc::type_t::STRING), abc::kind_t::STRING);
	// Выполняем проверку огрубления двоичных данных
	ASSERT_EQ(abc::kind(abc::type_t::BLOB), abc::kind_t::BLOB);
	// Выполняем проверку огрубления отметки времени
	ASSERT_EQ(abc::kind(abc::type_t::TIME), abc::kind_t::TIME);
	// Выполняем проверку огрубления опознавателя
	ASSERT_EQ(abc::kind(abc::type_t::UUID), abc::kind_t::UUID);
	// Выполняем проверку огрубления массива
	ASSERT_EQ(abc::kind(abc::type_t::ARRAY), abc::kind_t::ARRAY);
	// Выполняем проверку огрубления отображения
	ASSERT_EQ(abc::kind(abc::type_t::MAP), abc::kind_t::MAP);
	// Выполняем проверку огрубления отсутствующего значения
	ASSERT_EQ(abc::kind(abc::type_t::UNDEFINED), abc::kind_t::NONE);
}
/**
 * @brief Проверка сборки и разбора ведущего октета проволочной записи
 *
 * @details Ведущий октет несёт крупный вид в трёх старших разрядах, а подробность - в
 * пяти младших. Разбор обязан выдавать ровно то, что было собрано, на всякой паре
 *
 */
TEST(CodecAbcCommon, TagRoundtrip) {
	/**
	 * Крупные виды проволочной записи
	 */
	const vector <abc::group_t> majors = {
		abc::group_t::UNSIGNED, abc::group_t::NEGATIVE, abc::group_t::STRING,
		abc::group_t::BLOB, abc::group_t::ARRAY, abc::group_t::MAP,
		abc::group_t::SINGLE, abc::group_t::EXTEND
	};
	// Набор уже встреченных ведущих октетов
	unordered_set <uint32_t> seen;
	/**
	 * Выполняем перебор всех крупных видов проволочной записи
	 */
	for(const abc::group_t group : majors){
		/**
		 * Выполняем перебор всех подробностей метки
		 */
		for(uint8_t detail = 0; detail < 0x20; detail++){
			// Выполняем сборку ведущего октета значения
			const uint8_t tag = abc::tag(group, detail);
			// Выполняем проверку, что крупный вид разобран тем же
			ASSERT_EQ(abc::group(tag), group) << "ведущий октет: " << static_cast <uint32_t> (tag);
			// Выполняем проверку, что подробность разобрана тою же
			ASSERT_EQ(abc::detail(tag), detail) << "ведущий октет: " << static_cast <uint32_t> (tag);
			// Выполняем проверку, что ведущий октет не повторяется у иной пары
			ASSERT_TRUE(seen.emplace(tag).second) << "ведущий октет повторяется: " << static_cast <uint32_t> (tag);
		}
	}
	// Выполняем проверку, что перебором заняты все возможные ведущие октеты
	ASSERT_EQ(seen.size(), 256u);
}
/**
 * @brief Проверка ширины записи, ведомой подробностью метки
 *
 */
TEST(CodecAbcCommon, DetailWidth) {
	// Ширина ведомой записи в октетах
	uint8_t width = 0xFF;
	/**
	 * Выполняем перебор подробностей, несущих само значение
	 */
	for(uint8_t detail = 0; detail <= abc::INLINE_LIMIT; detail++){
		// Выполняем проверку, что подробность опознана
		ASSERT_TRUE(abc::width(detail, width)) << "подробность: " << static_cast <uint32_t> (detail);
		// Выполняем проверку, что ведомой записи за меткой нет
		ASSERT_EQ(width, 0u) << "подробность: " << static_cast <uint32_t> (detail);
	}
	// Выполняем проверку ширины записи в один октет
	ASSERT_TRUE(abc::width(abc::INLINE_LIMIT + 1, width));
	// Выполняем проверку значения ширины записи в один октет
	ASSERT_EQ(width, 1u);
	// Выполняем проверку ширины записи в два октета
	ASSERT_TRUE(abc::width(abc::INLINE_LIMIT + 2, width));
	// Выполняем проверку значения ширины записи в два октета
	ASSERT_EQ(width, 2u);
	// Выполняем проверку ширины записи в четыре октета
	ASSERT_TRUE(abc::width(abc::INLINE_LIMIT + 3, width));
	// Выполняем проверку значения ширины записи в четыре октета
	ASSERT_EQ(width, 4u);
	// Выполняем проверку ширины записи в восемь октетов
	ASSERT_TRUE(abc::width(abc::INLINE_LIMIT + 4, width));
	// Выполняем проверку значения ширины записи в восемь октетов
	ASSERT_EQ(width, 8u);
	/**
	 * Выполняем перебор подробностей, ширины не ведущих
	 */
	for(uint8_t detail = (abc::INLINE_LIMIT + 5); detail < 0x20; detail++){
		// Выполняем сброс ширины ведомой записи
		width = 0xFF;
		// Выполняем проверку, что подробность ширины не ведёт
		ASSERT_FALSE(abc::width(detail, width)) << "подробность: " << static_cast <uint32_t> (detail);
		// Выполняем проверку, что ширина ведомой записи обнулена
		ASSERT_EQ(width, 0u) << "подробность: " << static_cast <uint32_t> (detail);
	}
	// Выполняем проверку, что конец вместимого стоит последней подробностью
	ASSERT_EQ(static_cast <uint8_t> (abc::single_t::BREAK), 0x1Fu);
}
/**
 * @brief Проверка выбора наименьшей записи значения
 *
 * @details Наименьшая запись обязана быть единственной, иначе одно и то же значение
 * записалось бы двумя видами, и строгий вид записи стал бы невозможен
 *
 */
TEST(CodecAbcCommon, FitValue) {
	// Выполняем проверку укладки нуля в саму метку
	ASSERT_EQ(abc::fit(0), 0u);
	// Выполняем проверку укладки наибольшего значения, вмещаемого меткой
	ASSERT_EQ(abc::fit(abc::INLINE_LIMIT), static_cast <uint8_t> (abc::INLINE_LIMIT));
	// Выполняем проверку, что значение выше предела метки ведёт запись в один октет
	ASSERT_EQ(abc::fit(abc::INLINE_LIMIT + 1), static_cast <uint8_t> (abc::INLINE_LIMIT + 1));
	// Выполняем проверку, что наибольшее значение одного октета ведёт запись в один октет
	ASSERT_EQ(abc::fit(numeric_limits <uint8_t>::max()), static_cast <uint8_t> (abc::INLINE_LIMIT + 1));
	// Выполняем проверку, что значение выше одного октета ведёт запись в два октета
	ASSERT_EQ(abc::fit(static_cast <uint64_t> (numeric_limits <uint8_t>::max()) + 1),
	          static_cast <uint8_t> (abc::INLINE_LIMIT + 2));
	// Выполняем проверку, что наибольшее значение двух октетов ведёт запись в два октета
	ASSERT_EQ(abc::fit(numeric_limits <uint16_t>::max()), static_cast <uint8_t> (abc::INLINE_LIMIT + 2));
	// Выполняем проверку, что значение выше двух октетов ведёт запись в четыре октета
	ASSERT_EQ(abc::fit(static_cast <uint64_t> (numeric_limits <uint16_t>::max()) + 1),
	          static_cast <uint8_t> (abc::INLINE_LIMIT + 3));
	// Выполняем проверку, что наибольшее значение четырёх октетов ведёт запись в четыре октета
	ASSERT_EQ(abc::fit(numeric_limits <uint32_t>::max()), static_cast <uint8_t> (abc::INLINE_LIMIT + 3));
	// Выполняем проверку, что значение выше четырёх октетов ведёт запись в восемь октетов
	ASSERT_EQ(abc::fit(static_cast <uint64_t> (numeric_limits <uint32_t>::max()) + 1),
	          static_cast <uint8_t> (abc::INLINE_LIMIT + 4));
	// Выполняем проверку, что наибольшее значение восьми октетов ведёт запись в восемь октетов
	ASSERT_EQ(abc::fit(numeric_limits <uint64_t>::max()), static_cast <uint8_t> (abc::INLINE_LIMIT + 4));
}
/**
 * @brief Проверка согласия выбора записи с её разбором
 *
 * @details Подробность, выбранная записью, обязана быть опознана разбором, а ширина -
 * вмещать само значение. Расхождение этих двух работ означало бы запись, какую свой же
 * разбор прочитать не может
 *
 */
TEST(CodecAbcCommon, FitMatchesWidth) {
	/**
	 * Значения, стоящие на границах ширины записи
	 */
	const vector <uint64_t> values = {
		0, 1, abc::INLINE_LIMIT - 1, abc::INLINE_LIMIT, abc::INLINE_LIMIT + 1,
		static_cast <uint64_t> (numeric_limits <uint8_t>::max()) - 1,
		static_cast <uint64_t> (numeric_limits <uint8_t>::max()),
		static_cast <uint64_t> (numeric_limits <uint8_t>::max()) + 1,
		static_cast <uint64_t> (numeric_limits <uint16_t>::max()) - 1,
		static_cast <uint64_t> (numeric_limits <uint16_t>::max()),
		static_cast <uint64_t> (numeric_limits <uint16_t>::max()) + 1,
		static_cast <uint64_t> (numeric_limits <uint32_t>::max()) - 1,
		static_cast <uint64_t> (numeric_limits <uint32_t>::max()),
		static_cast <uint64_t> (numeric_limits <uint32_t>::max()) + 1,
		numeric_limits <uint64_t>::max() - 1,
		numeric_limits <uint64_t>::max()
	};
	// Ширина ведомой записи в октетах
	uint8_t width = 0;
	/**
	 * Выполняем перебор всех значений, стоящих на границах ширины записи
	 */
	for(const uint64_t value : values){
		// Выполняем получение наименьшей подробности, вмещающей значение
		const uint8_t detail = abc::fit(value);
		// Выполняем проверку, что выбранная подробность опознана разбором
		ASSERT_TRUE(abc::width(detail, width)) << "значение: " << value;
		// Если значение уложено в саму метку
		if(width == 0){
			// Выполняем проверку, что метка несёт само значение
			ASSERT_EQ(static_cast <uint64_t> (detail), value) << "значение: " << value;
			// Выполняем проверку, что уложенное в метку не превышает её предела
			ASSERT_LE(value, static_cast <uint64_t> (abc::INLINE_LIMIT)) << "значение: " << value;
		// Если значение ведёт за собой запись
		} else {
			// Выполняем проверку, что ширина записи вмещает значение
			ASSERT_TRUE((width == 8) || (value < (static_cast <uint64_t> (1) << (width * 8)))) << "значение: " << value;
			// Выполняем проверку, что запись вдвое уже значения не вмещает
			ASSERT_TRUE((width == 1) ? (value > static_cast <uint64_t> (abc::INLINE_LIMIT)) :
			            (value >= (static_cast <uint64_t> (1) << ((width / 2) * 8)))) << "значение: " << value;
		}
	}
}
/**
 * @brief Проверка величин, составляющих договор с носителем
 *
 * @details Величины эти суть САМ ДОГОВОР: смена любой из них обращает прежние контейнеры
 *          в нечитаемые. Проверки же по всему набору пользуются ИМЕНАМИ их, а не числами,
 *          и оттого смена величины проходит по набору ЗЕЛЁНОЙ: и укладка, и снятие
 *          двигаются согласно, круговой ход сходится, а носитель со старыми контейнерами
 *          остаётся за бортом молча
 *
 * @note Из девяти величин закреплены числами были ТРИ - длина заголовка опознания, длина
 *       признака владельца и длина отпечатка ключа. Шесть прочих не были закреплены ничем
 *       (замер 01.09.2026), и проверка эта заведена ради них
 *
 * @note Величины сличаются с ЛИТЕРАЛАМИ намеренно: сличение с именем было бы тождеством
 *       самому себе. Здесь тот же довод, что у раскладки заголовка по октетам, - число
 *       обязано стоять записанным дважды, иначе смена его нигде не отзовётся
 *
 */
TEST(CodecAbcCommon, WireConstants) {
	// Длина заголовка опознания контейнера в октетах
	EXPECT_EQ(abc::HEADER_LENGTH, 96u);
	// Длина признака владельца контейнера в октетах
	EXPECT_EQ(abc::OWNER_LENGTH, 16u);
	// Длина отпечатка открытого ключа владельца в октетах
	EXPECT_EQ(abc::FINGERPRINT_LENGTH, 16u);
	// Длина заголовка кадра в октетах
	EXPECT_EQ(abc::CHUNK_HEADER, 32u);
	// Смещение контрольной суммы в заголовке кадра
	EXPECT_EQ(abc::CHUNK_DIGEST, 24u);
	// Смещение разрядов свойств в заголовке кадра
	EXPECT_EQ(abc::CHUNK_FLAGS, 1u);
	// Разряд объявления кадра мусором
	EXPECT_EQ(abc::CHUNK_WASTE, 0x02);
	// Длина строки оглавления в октетах
	EXPECT_EQ(abc::ENTRY_LENGTH, 24u);
	// Длина заголовка записи подписи в октетах
	EXPECT_EQ(abc::SIGNATURE_HEADER, 8u);
	// Длина свёртки дерева в октетах
	EXPECT_EQ(abc::DIGEST_LENGTH, 32u);
	// Наибольшая подробность метки, несущая само значение
	EXPECT_EQ(abc::INLINE_LIMIT, 0x17);
	/**
	 * Величины эти обязаны быть согласны между собою: сумма и её место обязаны
	 * умещаться в заголовке кадра, а запас за ними - существовать
	 */
	EXPECT_EQ(abc::CHUNK_DIGEST + 8u, abc::CHUNK_HEADER);
	// Разряды свойств кадра обязаны лежать внутри заголовка его
	EXPECT_LT(abc::CHUNK_FLAGS, abc::CHUNK_HEADER);
	// Свёртка заголовка контейнера обязана быть последними восемью октетами его
	EXPECT_GT(abc::HEADER_LENGTH, 8u);
	/**
	 * Величины перечней, ложащиеся в ведущий октет записи, суть тот же договор с
	 * носителем: смена любой из них обращает всякую прежнюю запись в иную. Сличаются
	 * они с ЛИТЕРАЛАМИ по тому же доводу - проверки набора употребляют их ПО ИМЕНАМ,
	 * и смена величины двигает укладку со снятием согласно
	 */
	EXPECT_EQ(static_cast <uint8_t> (abc::group_t::UNSIGNED), 0x00);
	// Крупный вид записи отрицательного числа
	EXPECT_EQ(static_cast <uint8_t> (abc::group_t::NEGATIVE), 0x01);
	// Крупный вид записи строки
	EXPECT_EQ(static_cast <uint8_t> (abc::group_t::STRING), 0x02);
	// Крупный вид записи двоичных данных
	EXPECT_EQ(static_cast <uint8_t> (abc::group_t::BLOB), 0x03);
	// Крупный вид записи массива
	EXPECT_EQ(static_cast <uint8_t> (abc::group_t::ARRAY), 0x04);
	// Крупный вид записи отображения
	EXPECT_EQ(static_cast <uint8_t> (abc::group_t::MAP), 0x05);
	// Крупный вид записи одиночного значения
	EXPECT_EQ(static_cast <uint8_t> (abc::group_t::SINGLE), 0x06);
	// Крупный вид записи расширения
	EXPECT_EQ(static_cast <uint8_t> (abc::group_t::EXTEND), 0x07);
	// Подробность записи пустого значения
	EXPECT_EQ(static_cast <uint8_t> (abc::single_t::NUL), 0x00);
	// Подробность записи лжи
	EXPECT_EQ(static_cast <uint8_t> (abc::single_t::NO), 0x01);
	// Подробность записи истины
	EXPECT_EQ(static_cast <uint8_t> (abc::single_t::YES), 0x02);
	// Подробность записи дробного одинарной точности
	EXPECT_EQ(static_cast <uint8_t> (abc::single_t::FLOAT), 0x03);
	// Подробность записи дробного двойной точности
	EXPECT_EQ(static_cast <uint8_t> (abc::single_t::DOUBLE), 0x04);
	// Подробность записи отметки времени
	EXPECT_EQ(static_cast <uint8_t> (abc::single_t::TIME), 0x05);
	// Подробность записи опознавателя
	EXPECT_EQ(static_cast <uint8_t> (abc::single_t::UUID), 0x06);
	// Подробность конца вместимого неопределённой длины
	EXPECT_EQ(static_cast <uint8_t> (abc::single_t::BREAK), 0x1F);
	// Подробность записи целого любой ширины
	EXPECT_EQ(static_cast <uint8_t> (abc::extend_t::BIGNUM), 0x00);
	// Подробность записи десятичного числа
	EXPECT_EQ(static_cast <uint8_t> (abc::extend_t::DECIMAL), 0x01);
	// Подробность записи открытого расширения
	EXPECT_EQ(static_cast <uint8_t> (abc::extend_t::CUSTOM), 0x02);
	// Подробность записи вместимого с объявленным размахом
	EXPECT_EQ(static_cast <uint8_t> (abc::extend_t::SPANNED), 0x03);
	/**
	 * Крупный вид записи обязан умещаться в три старших разряда ведущего октета, а
	 * конец вместимого - совпадать с подробностью неопределённой длины
	 */
	EXPECT_EQ(static_cast <uint8_t> (abc::group_t::EXTEND), 0x07);
	// Наибольшая подробность метки обязана лежать ниже ведомых ширин
	EXPECT_LT(abc::INLINE_LIMIT, static_cast <uint8_t> (abc::single_t::BREAK));
}
