/**
 * @file common.cpp
 * @date 2026-08-14
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Проверки общих объявлений контейнера JSON — описания кодов отказов, названия
 *        видов узлов, проверка записи числа на соответствие стандарту и разбор
 *        необходимости экранирования
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
#include <unordered_set>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <gtest/gtest.h>
#include <codec/json/json.hpp>

/**
 * @brief Пространство имён проверок этого файла
 *
 * @note Держится оно безымянным намеренно: проверки кодеков собираются одной
 *       программою, и одноимённые построения разных файлов иначе сходятся в
 *       одно, порождая порчу вдали от места её причины
 *
 */
namespace {
	/**
	 * @brief Объект журнала проверок с отключённым выводом
	 *
	 * @details Вывод отключается назначением пустого перечня приёмников: отказы
	 *          разбора проверки наводят намеренно, и журнал их засорял бы выдачу
	 *
	 */
	struct Silent {
		/**
		 * @brief Функция получения объекта фреймворка проверок
		 *
		 * @details Объект заводится статикою местною, а не общею файла: заведение его
		 *          порядком построения статики оканчивается падением ещё до входа в
		 *          проверки, ибо фреймворк сам опирается на статику из библиотеки
		 *
		 * @return объект фреймворка проверок
		 *
		 */
		static const awh::fmk_t & framework() noexcept {
			// Объект фреймворка проверок
			static awh::fmk_t fmk;
			// Выводим объект фреймворка проверок
			return fmk;
		}
		// Объект журнала проверок
		awh::log_t log;
		/**
		 * @brief Конструктор
		 *
		 */
		Silent() noexcept : log(&Silent::framework()) {
			// Выполняем отключение вывода логов
			this->log.mode({});
		}
	};
	/**
	 * @brief Функция получения объекта журнала проверок
	 *
	 * @return объект журнала проверок
	 *
	 */
	[[maybe_unused]] const awh::log_t * logger() noexcept {
		// Объект журнала проверок
		static Silent silent;
		// Выводим объект журнала проверок
		return &silent.log;
	}
}

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;
using namespace awh::codec;

/**
 * @brief Проверка описаний кодов отказов разбора
 *
 * @details Всякий объявленный код обязан нести своё описание: описание неизвестного
 * кода у объявленного означало бы, что отказ выдаётся, а объяснить его нечем
 *
 */
TEST(CodecJsonCommon, Messages) {
	/**
	 * Выполняем перебор всех кодов отказа, договором отведённых
	 *
	 * @note Перебор ведётся числом от нуля до последнего кода перечня: список кодов,
	 *       выписанный рукою, от перечня отстаёт молча - именно так коды, заведённые
	 *       последними, не сличались вовсе
	 */
	for(uint32_t code = 0; code <= static_cast <uint32_t> (json::error_t::KEY_OUTSIDE_OBJECT); code++){
		// Получаем описание очередного кода отказа
		const char * message = json::message(static_cast <json::error_t> (code));
		// Выполняем проверку наличия описания кода отказа
		ASSERT_NE(message, nullptr) << code;
		// Выполняем проверку того, что описание кода отказа не пусто
		ASSERT_GT(::strlen(message), 0u) << code;
		// Выполняем проверку того, что код отказа описанием ОПОЗНАН
		ASSERT_STRNE(message, "unknown error") << code;
	}
	/**
	 * Выполняем проверку того, что перечень кодов на том и оканчивается
	 *
	 * @note Сличение это отмечает конец перечня, а сторожем НЕ является: код, дописанный
	 *       без описания, оно пропускает - именно его отсутствие оно и сличает. Проверено
	 *       щупом: дописанный код отказа проверку не уронил. Сторожем тут выступает
	 *       собиратель - смотри примечание у самой выдачи описаний
	 */
	ASSERT_STREQ(json::message(static_cast <json::error_t> (static_cast <uint32_t> (json::error_t::KEY_OUTSIDE_OBJECT) + 1)), "unknown error");
	// Выполняем проверку описания кода, договором не отведённого
	ASSERT_STREQ(json::message(static_cast <json::error_t> (0xFF)), "unknown error");
}
/**
 * @brief Проверка названий видов узлов документа
 *
 */
TEST(CodecJsonCommon, Names) {
	// Выполняем проверку названия неопределённого узла
	ASSERT_STREQ(json::name(json::kind_t::NONE), "none");
	// Выполняем проверку названия пустого значения
	ASSERT_STREQ(json::name(json::kind_t::NUL), "null");
	// Выполняем проверку названия логического значения
	ASSERT_STREQ(json::name(json::kind_t::BOOL), "boolean");
	// Выполняем проверку названия числа
	ASSERT_STREQ(json::name(json::kind_t::NUMBER), "number");
	// Выполняем проверку названия строки
	ASSERT_STREQ(json::name(json::kind_t::STRING), "string");
	// Выполняем проверку названия массива
	ASSERT_STREQ(json::name(json::kind_t::ARRAY), "array");
	// Выполняем проверку названия объекта
	ASSERT_STREQ(json::name(json::kind_t::OBJECT), "object");
	// Выполняем проверку выдачи заглушки на неизвестный вид узла
	ASSERT_STREQ(json::name(static_cast <json::kind_t> (0xFF)), "unknown");
}
/**
 * @brief Проверка признания годных записей числа
 *
 */
TEST(CodecJsonCommon, NumericValid) {
	/**
	 * Годные записи чисел
	 */
	const vector <string> values = {
		"0", "-0", "1", "-1", "42", "1234567890",
		"0.0", "-0.5", "3.14159", "0.000001",
		"1e10", "1E10", "1e+10", "1E-10", "-1.5e-30",
		"9007199254740993", "1e308"
	};
	/**
	 * Выполняем перебор всех годных записей чисел
	 */
	for(const string & value : values)
		// Выполняем проверку признания годной записи числа
		ASSERT_TRUE(json::numeric(value)) << "запись «" << value << "»";
}
/**
 * @brief Проверка отклонения негодных записей числа
 *
 */
TEST(CodecJsonCommon, NumericInvalid) {
	/**
	 * Негодные записи чисел
	 */
	const vector <string> values = {
		"", "-", "+", "+1", "01", "-01", "00", ".5", "-.5",
		"1.", "1.e5", "1e", "1e+", "1e-", "1.2.3", "1e1e1",
		" 1", "1 ", "0x10", "NaN", "Infinity", "abc", "1,5"
	};
	/**
	 * Выполняем перебор всех негодных записей чисел
	 */
	for(const string & value : values)
		// Выполняем проверку отклонения негодной записи числа
		ASSERT_FALSE(json::numeric(value)) << "запись «" << value << "»";
}
/**
 * @brief Проверка разбора необходимости экранирования
 *
 */
TEST(CodecJsonCommon, Escapable) {
	// Выполняем проверку отсутствия необходимости экранирования простого содержимого
	ASSERT_FALSE(json::escapable("простой текст", json::escape_t::MINIMAL));
	// Выполняем проверку необходимости экранирования кавычки
	ASSERT_TRUE(json::escapable("a\"b", json::escape_t::MINIMAL));
	// Выполняем проверку необходимости экранирования знака отмены
	ASSERT_TRUE(json::escapable("a\\b", json::escape_t::MINIMAL));
	// Выполняем проверку необходимости экранирования управляющего знака
	ASSERT_TRUE(json::escapable(string("a\nb"), json::escape_t::MINIMAL));
	// Выполняем проверку отсутствия необходимости экранирования косой черты по умолчанию
	ASSERT_FALSE(json::escapable("a/b", json::escape_t::MINIMAL));
	// Выполняем проверку необходимости экранирования косой черты при затребовании
	ASSERT_TRUE(json::escapable("a/b", json::escape_t::SOLIDUS));
	// Выполняем проверку отсутствия необходимости экранирования знаков вне US-ASCII
	ASSERT_FALSE(json::escapable("привет", json::escape_t::SOLIDUS));
	// Выполняем проверку необходимости экранирования знаков вне US-ASCII при затребовании
	ASSERT_TRUE(json::escapable("привет", json::escape_t::ASCII));
	// Выполняем проверку отсутствия необходимости экранирования пустого содержимого
	ASSERT_FALSE(json::escapable("", json::escape_t::ASCII));
}
/**
 * @brief Проверка названий видов значения и приведения вида значения к виду узла
 *
 * @details Названия эти уходят в сообщения о несовпадении вида, и потребитель судит
 *          по ним, что именно ему пришло. Карта покрытия показала обе выдачи
 *          нетронутыми целиком: набор не звал их ни разу, а стало быть, опечатка в
 *          названии либо разъехавшийся порядок ветвей прошли бы незамеченными
 *
 * @note Перечень здесь полон намеренно: проверка нескольких названий из полутора
 *       десятков доказывает лишь, что выдача отвечает, а не что отвечает верно.
 *       Добавленный впредь вид значения обязан ронять эту проверку, а не молчать
 *
 */
TEST(CodecJsonCommon, TypeNames) {
	/**
	 * @brief Вид значения, его название и отвечающий ему вид узла
	 *
	 */
	const struct {
		// Проверяемый вид значения документа
		json::type_t type;
		// Ожидаемое название вида значения
		const char * name;
		// Ожидаемый вид узла документа
		json::kind_t kind;
	} items[] = {
		{json::type_t::UNDEFINED, "undefined", json::kind_t::NONE},
		{json::type_t::NUL,       "null",      json::kind_t::NUL},
		{json::type_t::BOOL,      "boolean",   json::kind_t::BOOL},
		{json::type_t::STRING,    "string",    json::kind_t::STRING},
		{json::type_t::ARRAY,     "array",     json::kind_t::ARRAY},
		{json::type_t::OBJECT,    "object",    json::kind_t::OBJECT},
		{json::type_t::INT8,      "int8",      json::kind_t::NUMBER},
		{json::type_t::INT16,     "int16",     json::kind_t::NUMBER},
		{json::type_t::INT32,     "int32",     json::kind_t::NUMBER},
		{json::type_t::INT64,     "int64",     json::kind_t::NUMBER},
		{json::type_t::UINT8,     "uint8",     json::kind_t::NUMBER},
		{json::type_t::UINT16,    "uint16",    json::kind_t::NUMBER},
		{json::type_t::UINT32,    "uint32",    json::kind_t::NUMBER},
		{json::type_t::UINT64,    "uint64",    json::kind_t::NUMBER},
		{json::type_t::FLOAT,     "float",     json::kind_t::NUMBER}
	};
	/**
	 * Выполняем перебор всех проверяемых видов значения
	 */
	for(auto & item : items){
		// Выполняем проверку названия вида значения
		ASSERT_STREQ(json::name(item.type), item.name);
		// Выполняем проверку отвечающего виду значения вида узла
		ASSERT_EQ(json::kind(item.type), item.kind) << item.name;
	}
}
