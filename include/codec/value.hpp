/**
 * @file value.hpp
 * @date 2026-10-10
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл общих у всех кодеков рамки видов — вида значения, отрезка
 *        в хранилище знаков и положения в исходном тексте: через них потребитель
 *        спрашивает вид значения и место у любого из девяти
 *
 * \~english
 * @brief Header file of the kinds common to all the codecs of the framework — the kind of a value,
 *        a segment in the storage of the characters and a position in the source text: through them
 *        the consumer asks for the kind of the value and for the place at any of the nine
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#pragma once

/**
 * Стандартный заголовочный файл
 */
#include <cstdint>

/**
 * \~russian
 * @brief Основное пространство имён
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * Подключаем пространство имён стандартной библиотеки
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Пространство имён кодеков
	 *
	 * \~english
	 * @brief Namespace of the codecs
	 *
	 * \~
	 */
	namespace codec {
		/**
		 * \~russian
		 * @brief Кодировки исходного текста, общие у текстовых кодеков рамки
		 *
		 * @details Перечень несёт все кодировки, какие кодеки различают: UTF-8, UTF-16 и
		 *          UTF-32 обоих порядков байтов, ISO-8859-1, US-ASCII и Windows-1252.
		 *          Кодек, не поддерживающий какую-либо из них, обязан ответить отказом, а
		 *          не толковать её иначе.
		 *
		 * @note Вид `SINGLE` заведён ради XML: он обозначает однобайтовую кодировку,
		 *       взятую таблицею у `awh::charset`, и самой кодировки не называет. Прочие
		 *       кодеки этот вид не принимают.
		 *
		 * @note У ABC, CEF и syslog настройки кодировки нет: ABC двоичен, CEF и syslog
		 *       требуют UTF-8 — оттого и вида этого у них нет
		 *
		 * \~english
		 * @brief Encodings of the source text, common to the text codecs of the framework
		 *
		 * @details The list carries every encoding the codecs distinguish: UTF-8, UTF-16 and
		 *          UTF-32 of both byte orders, ISO-8859-1, US-ASCII and Windows-1252.
		 *          A codec that does not support any of them is obliged to answer with a refusal rather
		 *          than to interpret it otherwise.
		 *
		 * @note The `SINGLE` kind is introduced for the sake of XML: it designates a single-byte encoding
		 *       taken as a table from `awh::charset` and does not name the encoding itself. The other
		 *       codecs do not accept this kind.
		 *
		 * @note ABC, CEF and syslog have no encoding setting: ABC is binary, CEF and syslog
		 *       require UTF-8 — whereby they have no such kind
		 *
		 * \~
		 */
		enum class encoding_t : uint8_t {
			NONE    = 0x00, // Кодировка не определена
			UTF8    = 0x01, // Кодировка UTF-8
			UTF16LE = 0x02, // Кодировка UTF-16 с обратным порядком байтов
			UTF16BE = 0x03, // Кодировка UTF-16 с прямым порядком байтов
			UTF32LE = 0x04, // Кодировка UTF-32 с обратным порядком байтов
			UTF32BE = 0x05, // Кодировка UTF-32 с прямым порядком байтов
			LATIN1  = 0x06, // Кодировка ISO-8859-1
			ASCII   = 0x07, // Кодировка US-ASCII
			CP1252  = 0x08, // Кодировка Windows-1252
			SINGLE  = 0x09  // Однобайтовая кодировка, взятая таблицею у awh::charset
		};
		/**
		 * \~russian
		 * @brief Виды значения, общие у всех текстовых кодеков рамки
		 *
		 * @details Перечень видов заведён решением от 10.10.2026: текстовые кодеки
		 *          несут СВОИ системы видов (по одной в каждом из девяти — `toml::type_t`,
		 *          `cef::type_t`, `json::kind_t` и так далее), но ПОВЕРХ них потребителю
		 *          нужен общий, и прежде эту общность нес `awh::codec::abc::kind_t` — вид
		 *          значения бинарного контейнера ABC. Подмена эта была неверна: ABC лишь
		 *          одно из девяти текстовых представлений наряду с прочими, и тянуть
		 *          общий enum оттуда в toml, cef и другие — подмена зависимости на
		 *          зависимость, которой нет.
		 *
		 * @note Два перечня — `awh::codec::kind_t` (текстовое дерево) и
		 *       `awh::codec::abc::kind_t` (бинарный контейнер ABC) — намеренно
		 *       сосуществуют с одинаковыми значениями: контейнер ABC держит
		 *       свою систему видов под себя, текстовое дерево держит свою, и
		 *       сводить их в один тип — отдельная работа, которая здесь не идёт
		 *       и без слова владельца не пойдёт
		 *
		 * \~english
		 * @brief Kinds of a value, common for all the text codecs of the framework
		 *
		 * @details The set of kinds is introduced by the decision of 10.10.2026: the text codecs
		 *          carry their OWN systems of kinds (one in each of the nine — `toml::type_t`,
		 *          `cef::type_t`, `json::kind_t` and so on), but ON TOP of them the consumer
		 *          needs a common one, and before that this commonality was carried by `awh::codec::abc::kind_t` — the
		 *          kind of a value of the binary container ABC. That substitution was wrong: ABC is only
		 *          one of the nine text representations alongside the others, and pulling
		 *          the common enum from there into toml, cef and others is a substitution of a dependency by
		 *          a dependency that does not exist.
		 *
		 * @note The two enums — `awh::codec::kind_t` (text tree) and
		 *       `awh::codec::abc::kind_t` (binary container ABC) — intentionally
		 *       coexist with the same values: the container ABC holds
		 *       its own system of kinds for itself, the text tree holds its own, and
		 *       merging them into one type is a separate work that is not going here
		 *       and will not go without the word of the owner
		 *
		 * \~
		 */
		enum class kind_t : uint8_t {
			NONE   = 0x00, // Значение не определено либо путь недействителен
			NIL    = 0x01, // Пустое значение
			BOOL   = 0x02, // Логическое значение
			NUMBER = 0x03, // Число любого вида
			STRING = 0x04, // Последовательность знаков
			BLOB   = 0x05, // Двоичные данные
			TIME   = 0x06, // Отметка времени
			UUID   = 0x07, // Опознаватель
			ARRAY  = 0x08, // Массив
			MAP    = 0x09, // Отображение
			CUSTOM = 0x0A  // Открытое расширение, заведённое потребителем
		};

		/**
		 * \~russian
		 * @brief Отрезок в хранилище знаков
		 *
		 * @details Смещение и длина, а не указатель: хранилище растёт дописыванием и
		 *          может переехать в памяти, а указатель после переезда стал бы негодным.
		 *          Имя это общее у всех текстовых кодеков рамки и у контейнера ABC.
		 *
		 * \~english
		 * @brief Segment in the storage of the characters
		 *
		 * @details An offset and a length rather than a pointer: the storage grows by an
		 *          appending and may move in the memory, and a pointer after the move would become
		 *          invalid. This name is common to all the text codecs of the framework and to the ABC container.
		 *
		 * \~
		 */
		typedef struct Span {
			// Смещение начала отрезка в хранилище знаков
			uint32_t offset;
			// Длина отрезка в байтах
			uint32_t length;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * \~
			 */
			constexpr Span() noexcept : offset(0), length(0) {}
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param offset смещение начала отрезка в хранилище знаков
			 * @param length длина отрезка в байтах
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * @param offset offset of the beginning of the segment in the storage of the characters
			 * @param length length of the segment in bytes
			 *
			 * \~
			 */
			constexpr Span(const uint32_t offset, const uint32_t length) noexcept : offset(offset), length(length) {}
		} span_t;
		/**
		 * \~russian
		 * @brief Положение в исходном тексте, общее у всех кодеков рамки
		 *
		 * @details Несёт номер строки, положение в строке и смещение от начала текста.
		 *          Кодеки дополняют вид своими полями: глубиною вложенности, номером поля
		 *          либо записи. В общий вид вынесены только те поля, которые нужны каждому
		 *          кодеку разом.
		 *
		 * @note Номер строки и положение в строке считаются с единицы. Чем меряется
		 *       столбец (знаками либо байтами) и по каким байтам считается смещение,
		 *       задаёт кодек: у двоичной записи ABC строк и столбцов нет вовсе, и
		 *       общие поля остаются нулевыми
		 *
		 * @warning У текстовых кодеков смещение считается по тексту, ПРИВЕДЁННОМУ к
		 *          UTF-8, а не по исходным байтам: метка порядка байтов снята, текст в
		 *          иной кодировке переведён, и длины расходятся. У двоичной записи ABC
		 *          смещение считается по октетам поданной записи
		 *
		 * \~english
		 * @brief Position in the source text, common to all the codecs of the framework
		 *
		 * @details Carries the line number, the position in the line and the offset from the beginning
		 *          of the text. The codecs supplement the kind with their own fields: the depth of
		 *          nesting, the number of a field or of a record. Only the fields needed by every codec
		 *          at once are brought into the common kind.
		 *
		 * @note The line number and the position in the line are counted from one. What the column
		 *       is measured in (characters or bytes) and over which bytes the offset is counted
		 *       is set by the codec: a binary record of ABC has no lines and columns at all, and
		 *       the common fields remain zero
		 *
		 * @warning At the text codecs the offset is counted over the text CONVERTED to UTF-8 rather
		 *          than over the source bytes: the byte order mark is stripped, a text in another
		 *          encoding is converted, and the lengths diverge. At a binary record of ABC
		 *          the offset is counted over the octets of the submitted record
		 *
		 * \~
		 */
		typedef struct Location {
			// Номер строки, считая с единицы
			uint32_t line;
			// Положение в строке, считая с единицы
			uint32_t column;
			// Смещение от начала текста в байтах
			uint64_t offset;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * \~
			 */
			constexpr Location() noexcept :
			 line(0), column(0), offset(static_cast <uint64_t> (~0ull)) {}
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param line   номер строки, считая с единицы
			 * @param column положение в строке, считая с единицы
			 * @param offset смещение от начала текста в байтах
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * @param line   line number counted from one
			 * @param column position in the line counted from one
			 * @param offset offset from the beginning of the text in bytes
			 *
			 * \~
			 */
			constexpr Location(const uint32_t line, const uint32_t column, const uint64_t offset) noexcept :
			 line(line), column(column), offset(offset) {}
		} location_t;
	};
};
