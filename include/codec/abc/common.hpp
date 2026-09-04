/**
 * @file common.hpp
 * @date 2026-08-18
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
 * @brief Заголовочный файл общих объявлений бинарного контейнера ABC — метки проволочной
 *        записи, пределы разбора, коды отказов, виды узлов и события чтения
 *
 * \~english
 * @brief Header file of the common declarations of the ABC binary container — the tags of the wire
 *        record, the limits of the parsing, the codes of the refusals, the kinds of the nodes and the events of the reading
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_ABC_COMMON__
#define __AWH_CODEC_ABC_COMMON__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../sys/global.hpp"

/**
 * Подавляем системные макросы, занявшие имена членов перечислений ниже:
 * DELETE и ERROR у MS Windows, CS и PRIVATE у Sun Solaris, CS5 у termios.
 * Имена снимаются лишь на время объявлений - возврат в конце файла
 */
#include "../../sys/macro/suppress.hpp"

/**
 * \~russian
 * @brief Принудительная подстановка горячих обращений контейнера
 *
 * @details Обход дерева и снятие событий разбора - самые горячие пути контейнера, а работы
 *          в них одна арифметика. Оставленные в переводимом наборе, они обращаются в вызовы
 *          через границу единиц трансляции и стоят дороже самой работы
 *
 * @note Приём этот - принятое в AWH исключение из правила о чистых заголовочных файлах:
 *       реализация живёт в `.cpp`, а для встраивания заводятся посредники с
 *       `always_inline`. Смотри `include/codec/json/common.hpp`
 *
 * \~english
 * @brief Forced inlining of the hot accessors of the container
 * @details The traversal of the tree and the taking of the parsing events are the hottest paths of the container,
 * while the work in them is mere arithmetic. Left in the translation unit, they turn into calls
 * across the boundary of the translation units and cost more than the work itself
 * @note This device is an accepted exception in AWH from the rule about clean header files:
 * the implementation lives in `.cpp`, while for the inlining mediators with
 * `always_inline` are made. See `include/codec/json/common.hpp`
 *
 * \~
 */
#if defined(_MSC_VER)
	/**
	 * Принудительная подстановка средствами Visual Studio
	 */
	#define AWH_ABC_INLINE __forceinline
/**
 * Если компилятор принадлежит к семейству GCC или Clang
 */
#else
	/**
	 * Принудительная подстановка средствами GCC и Clang
	 */
	#define AWH_ABC_INLINE inline __attribute__((always_inline))
#endif

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
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Пространство имён контейнеров данных
	 *
	 * \~english
	 * @brief Data containers namespace
	 *
	 * \~
	 */
	namespace codec {
		/**
		 * \~russian
		 * @brief Пространство имён бинарного контейнера ABC
		 *
		 * @details ABC (ANYKS Binary Container) хранит те же построения, что и текстовые
		 * кодеки библиотеки - значения, массивы и отображения любой глубины вложенности, -
		 * но записью двоичной, а не знаковой. Устройство его задано четырьмя решениями:
		 *
		 * @li **Метка ведёт значение.** Ведущий октет несёт крупный вид в трёх старших
		 * разрядах и подробность в пяти младших: малое число, короткая строка и все
		 * значения-одиночки умещаются в него целиком, не занимая ни октета сверх
		 * @li **Вид числа записан, а не выведен.** Текстовый кодек хранит запись числа и
		 * определяет вид её разбором; здесь вид стоит в метке, и разбирать нечего
		 * @li **Строки и двоичные данные не перекодируются.** Экранирования в двоичной
		 * записи нет вовсе, оттого значение при чтении из поданного целиком буфера не
		 * копируется, а отмечается отрезком в нём
		 * @li **Узел дерева несёт размах своего поддерева.** Обход собранного дерева идёт
		 * вперёд по памяти, а пропуск вложенного построения целиком стоит сложения. В самой
		 * же записи вместимое несёт **количество** значений, а не размах в октетах: пропуск
		 * по записи оттого стоит обхода. Даётся он вынесенным поддеревом страничного
		 * хранилища, а не полем длины у всякого вместимого - поле такое платило бы октетами
		 * на всяком вместимом ради выигрыша, нужного лишь крупным
		 *
		 * @details **Намеренные решения.** Перечисленное ниже выбрано осознанно, и
		 * возвращаться к этим вопросам при разборе кода не следует:
		 *
		 * @li **Имя поля отображения - значение любого вида, кроме вместимого.** Двоичной
		 * записи ограничение строкою ничего не даёт, а целое именем и короче, и сличается
		 * быстрее. Вместимое же именем отвергается: розыск по такому имени требовал бы
		 * сличения поддеревьев, и цена его несоразмерна получаемому
		 * @li **Число свыше родных видов хранится точно.** Целое шире 64 разрядов и
		 * десятичное с точным дробным разрядом записываются своими видами через `awh::BigNum`,
		 * а не обращаются в `double` с потерей точности
		 * @li **Двоичные данные - полноправное значение.** Текстовые кодеки вынуждены гнать
		 * их через BASE64, прирастая на треть; здесь они лежат как есть
		 * @li **Глубина вложенности ограничена.** Разбор ведётся без рекурсии, но предел
		 * нужен и без неё: миллион открытых вместимых иначе съедает память под стек состояний
		 * @li **Порядок октетов записи - от младшего к старшему.** Он совпадает с родным
		 * порядком у всех целевых машин, и перестановка на них не стоит ничего
		 *
		 * \~english
		 * @brief ABC binary container namespace
		 * @details ABC (ANYKS Binary Container) stores the same constructions as the textual codecs
		 * of the library — the values, the arrays and the mappings of any depth of the nesting — but
		 * by a binary record rather than by a character one. Its structure is set by four decisions:
		 * @li **A tag leads a value.** The leading octet carries the group kind in its three high bits
		 * and the detail in its five low ones: a small number, a short string and all the singleton values
		 * fit into it entirely without occupying a single octet more
		 * @li **The kind of a number is recorded rather than deduced.** A textual codec stores the record of a number
		 * and determines its kind by a parsing; here the kind stands in the tag, and there is nothing to parse
		 * @li **The strings and the binary data are not transcoded.** There is no escaping in a binary
		 * record at all, whereby a value at the reading from a buffer submitted in full is not
		 * copied but marked by a segment in it
		 * @li **A node of the tree carries the extent of its own subtree.** The traversal of an assembled tree goes
		 * forward through the memory, and the skipping of a nested construction as a whole costs an addition. In the record
		 * itself a container carries the **number** of the values rather than the extent in octets: the skipping
		 * by the record therefore costs a traversal. It is given by an external subtree of the paged
		 * storage rather than by a length field on every container — such a field would pay by the octets
		 * on every container for the sake of a gain needed only by the large ones
		 * @details **Deliberate decisions.** The below is chosen consciously, and one should not
		 * return to these questions at the reading of the code:
		 * @li **The name of a field of a mapping is a value of any kind except a container.** The restriction
		 * to a string gives nothing to a binary record, while an integer as a name is both shorter and compared
		 * faster. A container as a name is rejected: a search by such a name would require
		 * a comparison of the subtrees, and its price is disproportionate to what is obtained
		 * @li **A number above the native kinds is stored exactly.** An integer wider than 64 bits and
		 * a decimal one with an exact fractional bit are written by their own kinds through `awh::BigNum`
		 * rather than turned into a `double` with a loss of the precision
		 * @li **The binary data are a full-fledged value.** The textual codecs are forced to drive
		 * them through BASE64 growing by a third; here they lie as they are
		 * @li **The depth of the nesting is limited.** The parsing is conducted without a recursion, but the limit
		 * is needed even without it: a million of the opened containers would otherwise eat up the memory for the stack of the states
		 * @li **The order of the octets of the record is from the low to the high one.** It coincides with the native
		 * order on all the target machines, and a rearrangement on them costs nothing
		 *
		 * \~
		 */
		namespace abc {
			/**
			 * \~russian
			 * @brief Наибольшая допустимая глубина вложенности
			 *
			 * @details Предел этот бережёт не стек вызовов - разбор ведётся без рекурсии, - а
			 * память под стек состояний: запись из одних открывающих вместимых иначе заняла бы
			 * её всю
			 *
			 * \~english
			 * @brief Largest admissible depth of the nesting
			 * @details This limit guards not the stack of the calls — the parsing is conducted without a recursion —
			 * but the memory for the stack of the states: a record of the opening containers alone would otherwise occupy
			 * all of it
			 *
			 * \~
			 */
			constexpr uint32_t MAX_DEPTH = 0x400;

			/**
			 * \~russian
			 * @brief Наибольшее допустимое количество узлов в документе
			 *
			 * \~english
			 * @brief Largest admissible number of the nodes in a document
			 *
			 * \~
			 */
			constexpr uint32_t MAX_NODES = 0x4000000;

			/**
			 * \~russian
			 * @brief Количество полей отображения, начиная с какого заводится указатель имён
			 *
			 * @details Ниже этого порога имя разыскивается перебором, и это дешевле всякого
			 * отображения: сличение имён идёт по памяти подряд, а заведение отображения
			 * стоит выделения памяти и подсчёта хеша на всякое имя
			 *
			 * @note Указатель заводится **по требованию** - при первом обращении по имени, а
			 * не при разборе. Отображение, к которому по имени не обращались, не платит ничего
			 *
			 * \~english
			 * @brief Number of the fields of a mapping starting from which the index of the names is created
			 * @details Below this threshold a name is searched for by an enumeration, and this is cheaper than any
			 * mapping: the comparison of the names goes through the memory consecutively, while the creation of a mapping
			 * costs an allocation of the memory and a computation of a hash for every name
			 * @note The index is created **on demand** — at the first access by a name rather
			 * than at the parsing. A mapping which has not been accessed by a name pays nothing
			 *
			 * \~
			 */
			constexpr uint32_t INDEX_THRESHOLD = 16;

			/**
			 * \~russian
			 * @brief Обозначение отсутствующего смещения в поданной записи
			 *
			 * \~english
			 * @brief Designation of an absent offset in the submitted record
			 *
			 * \~
			 */
			constexpr uint64_t NO_OFFSET = static_cast <uint64_t> (~0ull);

			/**
			 * \~russian
			 * @brief Обозначение отсутствующего номера узла
			 *
			 * \~english
			 * @brief Designation of an absent number of a node
			 *
			 * \~
			 */
			constexpr uint32_t NO_INDEX = static_cast <uint32_t> (~0u);

			/**
			 * \~russian
			 * @brief Крупные виды проволочной записи
			 *
			 * @details Ведущий октет всякого значения несёт крупный вид в трёх старших
			 * разрядах, а подробность - в пяти младших. Подробность значит одно из трёх:
			 * само значение, когда оно умещается в пять разрядов; ширину следующей за
			 * меткой длины; либо разновидность одиночного значения
			 *
			 * @note Целое разделено на два крупных вида по знаку намеренно. Единый вид
			 * потребовал бы записи со смещённым нулём либо зигзагом, и малое отрицательное
			 * число перестало бы умещаться в ведущий октет. Обещание это о РАЗМЕРЕ записи,
			 * и закреплено проверкой `CodecAbcWriter.SmallNumbersFitLeadingOctet`: числа
			 * от −24 до 23 обязаны укладываться одним октетом, а 24 и −25 - двумя
			 *
			 * \~english
			 * @brief Major kinds of the wire record
			 * @details The leading octet of every value carries the group kind in its three high bits,
			 * and the detail in its five low ones. The detail means one of the three: the value
			 * itself when it fits into the five bits; the width of the length following the tag;
			 * or the variety of a singleton value
			 * @note The integer is divided into two group kinds by the sign deliberately. A single kind
			 * would require a record with a shifted zero or a zigzag, and a small negative
			 * number would cease to fit into the leading octet
			 *
			 * \~
			 */
			enum class group_t : uint8_t {
				UNSIGNED = 0x00, // Целое без знака, подробность - само значение либо ширина записи
				NEGATIVE = 0x01, // Целое со знаком, меньшее нуля, записанное дополнением до −1
				STRING   = 0x02, // Строка, подробность - длина в октетах либо её ширина
				BLOB     = 0x03, // Двоичные данные, подробность - длина в октетах либо её ширина
				ARRAY    = 0x04, // Массив, подробность - количество значений либо его ширина
				MAP      = 0x05, // Отображение, подробность - количество пар либо его ширина
				SINGLE   = 0x06, // Одиночное значение, подробность - его разновидность
				EXTEND   = 0x07  // Расширение, подробность - его разновидность
			};

			/**
			 * \~russian
			 * @brief Разновидности одиночного значения
			 *
			 * @details Значения эти умещаются в ведущий октет целиком либо ведут за собой
			 * запись установленной ширины. Длины у них нет, оттого и подробность метки
			 * занята разновидностью
			 *
			 * \~english
			 * @brief Varieties of a singleton value
			 * @details These values fit into the leading octet entirely or lead a record
			 * of a set width. They have no length, whereby the detail of the tag is occupied
			 * by the variety
			 *
			 * @note Логическая пара зовётся `NO`/`YES`, а не `FALSE`/`TRUE`, НАМЕРЕННО.
			 * `windef.h` заводит `FALSE` и `TRUE` макросами-значениями, а раскрытие макроса
			 * идёт ДО всякого разбора имён - квалификация `abc::single_t::FALSE` от него не
			 * спасает, и пара снятия макросов в заголовке тоже: она бережёт текст СВОЙ, а не
			 * место вызова у потребителя. Замер 01.09.2026 на Windows 11 ARM64: из 177 имён
			 * открытого договора ABC сталкивались ровно эти два и более ни одного.
			 * Переименование - решение владельца от 02.09.2026
			 *
			 * @warning Числа договора при переименовании НЕ тронуты: `NO` есть прежний
			 * `FALSE` (0x01), `YES` - прежний `TRUE` (0x02). Запись проволочная не менялась
			 * ни октетом
			 *
			 * \~
			 */
			enum class single_t : uint8_t {
				NUL    = 0x00, // Пустое значение
				NO     = 0x01, // Ложь
				YES    = 0x02, // Истина
				FLOAT  = 0x03, // Дробное одинарной точности, за меткой четыре октета
				DOUBLE = 0x04, // Дробное двойной точности, за меткой восемь октетов
				TIME   = 0x05, // Отметка времени, за меткой восемь октетов
				UUID   = 0x06, // Опознаватель, за меткой шестнадцать октетов
				BREAK  = 0x1F  // Конец вместимого неопределённой длины
			};

			/**
			 * \~russian
			 * @brief Разновидности расширения
			 *
			 * @details Расширением записывается то, чему родного вида в машине нет вовсе:
			 * число, не вместимое в восемь октетов, и десятичное с точным дробным разрядом
			 *
			 * \~english
			 * @brief Varieties of an extension
			 * @details By an extension is written that which has no native kind in a machine at all:
			 * a number not containable in eight octets and a decimal one with an exact fractional bit
			 *
			 * \~
			 */
			enum class extend_t : uint8_t {
				BIGNUM  = 0x00, // Целое любой ширины, за меткой длина и октеты числа
				DECIMAL = 0x01, // Десятичное, за меткой порядок и целое любой ширины
				CUSTOM  = 0x02, // Открытое расширение, за меткой номер подвида, длина и октеты
				SPANNED = 0x03  // Вместимое с объявленным размахом, за меткой размах и само вместимое
			};

			/**
			 * \~russian
			 * @brief Ширина записи размаха вместимого в октетах
			 *
			 * @details Размах кладётся записью ПОСТОЯННОЙ ширины, а не переменной: сборка
			 * узнаёт его лишь при закрытии вместимого, и место под него отводится наперёд,
			 * а заполняется задним числом. Переменная ширина обязала бы сдвигать всё
			 * уложенное следом
			 *
			 * \~english
			 * @brief Width of the record of the span of a container in octets
			 * @details The span is laid by a record of a CONSTANT width rather than of a variable one:
			 * the assembling learns it only upon the closing of the container, and the place for it is
			 * set aside in advance and filled in after the fact. A variable width would oblige one to
			 * shift everything laid afterwards
			 *
			 * \~
			 */
			constexpr size_t SPAN_LENGTH = 8;

			/**
			 * \~russian
			 * @brief Наибольшее значение, умещающееся в подробность ведущего октета
			 *
			 * @details Значения от нуля до этого предела записываются ведущим октетом целиком.
			 * Значения выше ведут за собой запись, а подробность занимается её шириною
			 *
			 * \~english
			 * @brief Largest value fitting into the detail of the leading octet
			 * @details The values from zero to this limit are written by the leading octet entirely.
			 * The values above lead a record, and the detail is occupied by its width
			 *
			 * \~
			 */
			constexpr uint8_t INLINE_LIMIT = 0x17;

			/**
			 * \~russian
			 * @brief Виды узлов документа
			 *
			 * @details Вид узла отвечает на вопрос о построении: чем значение является, -
			 * тогда как вид значения отвечает на вопрос о хранении: каким именно числом оно
			 * записано
			 *
			 * @note Целое и дробное числа видом узла не разделяются: разделение это
			 * принадлежит не построению, а тому, каким видом значение затребовано
			 *
			 * \~english
			 * @brief Kinds of the nodes of a document
			 * @details The kind of a node answers the question about the construction: what the value is, —
			 * whereas the kind of a value answers the question about the storage: by which number exactly it
			 * is written
			 * @note An integer and a fractional number are not divided by the kind of a node: this division
			 * belongs not to the construction but to that by which kind the value is demanded
			 *
			 * \~
			 */
			enum class kind_t : uint8_t {
				NONE   = 0x00, // Узел не определён либо ссылка недействительна
				NUL    = 0x01, // Пустое значение
				BOOL   = 0x02, // Логическое значение
				NUMBER = 0x03, // Число любого вида
				STRING = 0x04, // Строка
				BLOB   = 0x05, // Двоичные данные
				TIME   = 0x06, // Отметка времени
				UUID   = 0x07, // Опознаватель
				ARRAY  = 0x08, // Массив
				MAP    = 0x09, // Отображение
				CUSTOM = 0x0A  // Открытое расширение, заведённое потребителем
			};

			/**
			 * \~russian
			 * @brief Виды значений документа
			 *
			 * @details Виды заданы разрядами, а сборные виды - объединением разрядов. Оттого
			 * точный вопрос (`INT32`) и сборный (`SIGNED`, `NUMBER`) стоят ровно одинаково -
			 * одно наложение разрядов
			 *
			 * @note Вид хранения извлечению не указ: `INT8` извлекается и как `double`, и как
			 * `uint64_t`. Извлечение сличает само значение с пределами затребованного вида,
			 * а не вид хранения с видом затребованным
			 *
			 * \~english
			 * @brief Kinds of the values of a document
			 * @details The kinds are set by the bits, and the composite kinds by a union of the bits. Whereby
			 * an exact question (`INT32`) and a composite one (`SIGNED`, `NUMBER`) cost exactly the same —
			 * one overlaying of the bits
			 * @note The kind of the storage is not a directive to the extraction: `INT8` is extracted both as a `double` and as
			 * a `uint64_t`. The extraction compares the value itself with the limits of the demanded kind
			 * rather than the kind of the storage with the demanded kind
			 *
			 * \~
			 */
			enum class type_t : uint32_t {
				UNDEFINED = 0x00000000, // Значения нет вовсе: ссылка недействительна
				NUL       = 0x00000001, // Пустое значение
				BOOL      = 0x00000002, // Логическое значение
				STRING    = 0x00000004, // Строка
				BLOB      = 0x00000008, // Двоичные данные
				ARRAY     = 0x00000010, // Массив
				MAP       = 0x00000020, // Отображение
				TIME      = 0x00000040, // Отметка времени
				UUID      = 0x00000080, // Опознаватель
				INT8      = 0x00000100, // Целое со знаком шириною в один октет
				INT16     = 0x00000200, // Целое со знаком шириною в два октета
				INT32     = 0x00000400, // Целое со знаком шириною в четыре октета
				INT64     = 0x00000800, // Целое со знаком шириною в восемь октетов
				UINT8     = 0x00001000, // Целое без знака шириною в один октет
				UINT16    = 0x00002000, // Целое без знака шириною в два октета
				UINT32    = 0x00004000, // Целое без знака шириною в четыре октета
				UINT64    = 0x00008000, // Целое без знака шириною в восемь октетов
				FLOAT     = 0x00010000, // Дробное, точно представимое одинарной точностью
				DOUBLE    = 0x00020000, // Дробное двойной точности
				EXTENDED  = 0x00040000, // Целое, не вместимое ни в один родной вид
				DECIMAL   = 0x00080000, // Десятичное с точным дробным разрядом
				CUSTOM    = 0x00100000, // Открытое расширение, заведённое потребителем
				// Целое со знаком любой ширины
				SIGNED    = (INT8 | INT16 | INT32 | INT64),
				// Целое без знака любой ширины
				UNSIGNED  = (UINT8 | UINT16 | UINT32 | UINT64),
				// Целое любой ширины и любой знаковости
				INT       = (SIGNED | UNSIGNED),
				// Дробное любой точности
				REAL      = (FLOAT | DOUBLE),
				// Число любого вида
				NUMBER    = (INT | REAL | EXTENDED | DECIMAL),
				// Значение, хранимое отрезком октетов
				SEGMENT   = (STRING | BLOB),
				// Вместимое любого вида
				CONTAINER = (ARRAY | MAP)
			};

			/**
			 * \~russian
			 * @brief Коды ошибок разбора записи
			 *
			 * @details Разбор не выбрасывает исключений: признаком отказа служит код ошибки
			 * вместе со смещением в поданной записи, где отказ произошёл
			 *
			 * \~english
			 * @brief Error codes of the parsing of a record
			 * @details The parsing does not throw exceptions: the error code together with the offset
			 * in the submitted record where the refusal has occurred serves as the sign of a refusal
			 *
			 * \~
			 */
			enum class error_t : uint8_t {
				NONE                 = 0x00, // Ошибок не обнаружено
				INTERNAL             = 0x01, // Внутренняя ошибка разбора
				UNEXPECTED_EOF       = 0x02, // Запись оборвалась посреди значения
				UNKNOWN_TAG          = 0x03, // Метка не опознана
				RESERVED_TAG         = 0x04, // Метка отведена под будущее и к употреблению не годна
				INVALID_LENGTH       = 0x05, // Объявленная длина недопустима
				INVALID_ENCODING     = 0x06, // Строка содержит последовательность, не отвечающую UTF-8
				NUMBER_OUT_OF_RANGE  = 0x07, // Число не представимо затребованным видом
				INVALID_BIGNUM       = 0x08, // Запись числа неограниченной ширины повреждена
				INVALID_DECIMAL      = 0x09, // Запись десятичного числа повреждена
				UNBALANCED_BREAK     = 0x0A, // Конец вместимого встречен вне вместимого неопределённой длины
				MISSING_VALUE        = 0x0B, // Отображение оборвалось на имени, значения за ним нет
				DUPLICATE_KEY        = 0x0C, // Имя поля отображения объявлено повторно
				DEPTH_EXCEEDED       = 0x0D, // Глубина вложенности превышает допустимую
				STRING_TOO_LONG      = 0x0E, // Длина строкового значения превышает допустимую
				BLOB_TOO_LONG        = 0x0F, // Длина двоичного значения превышает допустимую
				TOO_MANY_NODES       = 0x10, // Количество узлов документа превышает допустимое
				TRAILING_OCTETS      = 0x11, // Октеты за окончанием документа
				EMPTY_RECORD         = 0x12, // Запись пуста, а документ затребован
				OVERFLOW_LIMIT       = 0x13, // Превышен предел, заданный настройками разбора
				INVALID_KEY          = 0x14, // Именем поля отображения стоит вместимое
				UNORDERED_KEY        = 0x15, // Имена полей отображения идут не по возрастанию
				INDEFINITE_REFUSED   = 0x16, // Неопределённая длина при строгом виде записи
				UNBALANCED_CONTAINER = 0x17, // Вместимое не закрыто либо закрыто лишний раз
				CONTAINER_OVERFLOW   = 0x18, // Значений вместимого больше объявленного
				INVALID_MAGIC        = 0x19, // Заголовок не несёт опознавательной записи контейнера
				INVALID_VERSION      = 0x1A, // Вид записи контейнера не поддерживается
				INVALID_CHECKSUM     = 0x1B, // Контрольная сумма заголовка не сошлась
				TRUNCATED_HEADER     = 0x1C, // Заголовок оборван, октетов его недостаёт
				TRUNCATED_CHUNK      = 0x1D, // Кадр оборван, октетов его недостаёт
				INVALID_CHUNK        = 0x1E, // Заголовок кадра не опознан
				COMPRESSION_FAILED   = 0x1F, // Сжатие либо разжатие кадра отвечено отказом
				ENCRYPTION_FAILED    = 0x20, // Шифрование либо расшифровка кадра отвечены отказом
				MISSING_INDEX        = 0x21, // Оглавление контейнера не объявлено заголовком
				INVALID_INDEX        = 0x22, // Запись оглавления повреждена либо указывает за тело
				UNREADABLE_SOURCE    = 0x23, // Работа чтения октетов контейнера отвечена отказом
				UNWRITABLE_SINK      = 0x24, // Работа записи октетов контейнера отвечена отказом
				MISSING_RECORD       = 0x25, // Запись снесена правкой контейнера
				TRUNCATED_SIGNATURE  = 0x26, // Запись подписи оборвана, октетов её недостаёт
				INVALID_SIGNATURE    = 0x27, // Запись подписи повреждена
				UNSIGNED_CONTAINER   = 0x28, // Подпись владельца контейнером не объявлена
				REFUSED_SIGNATURE    = 0x29, // Подпись владельца контейнера не сошлась
				SIGNING_FAILED       = 0x2A, // Выработка подписи владельца отвечена отказом
				INVALID_SEGMENT      = 0x2B, // Значение, собираемое кусками, несёт кусок иного вида
				INVALID_EXTENSION    = 0x2C, // Запись открытого расширения повреждена
				NON_MINIMAL_TAG      = 0x2D  // Метка несёт запись шире наименьшей при строгом виде
			};

			/**
			 * \~russian
			 * @brief Виды событий чтения записи
			 *
			 * @details Чтение выдаёт события по мере разбора записи, не удерживая её целиком
			 *
			 * @note Начало и конец вместимого выдаются отдельными событиями, а не одним
			 * событием с готовым вместимым: массив на миллион значений иначе удерживался бы
			 * в памяти целиком прежде первой выдачи, и потоковое чтение потеряло бы смысл
			 *
			 * @note Имя поля отображения выдаётся своим событием, предшествующим событию
			 * значения. Значением при этом вправе оказаться вместимое, и тогда за именем
			 * идёт начало вместимого
			 *
			 * \~english
			 * @brief Kinds of the events of the reading of a record
			 * @details The reading issues the events as the record is parsed without holding it in full
			 * @note The beginning and the end of a container are issued by separate events rather than by one
			 * event with a ready container: an array of a million values would otherwise be held
			 * in the memory in full before the first issuance, and the streaming reading would lose its sense
			 * @note The name of a field of a mapping is issued by its own event preceding the event
			 * of the value. The value may thereby turn out to be a container, and then the name
			 * is followed by the beginning of a container
			 *
			 * \~
			 */
			enum class event_t : uint8_t {
				NONE        = 0x00, // Событие не определено
				KEY         = 0x01, // Имя поля отображения
				NUL         = 0x02, // Пустое значение
				BOOL        = 0x03, // Логическое значение
				NUMBER      = 0x04, // Число, выдаётся своим видом хранения
				STRING      = 0x05, // Строка
				BLOB        = 0x06, // Двоичные данные
				TIME        = 0x07, // Отметка времени
				UUID        = 0x08, // Опознаватель
				ARRAY_BEGIN = 0x09, // Начало массива
				ARRAY_END   = 0x0A, // Конец массива
				MAP_BEGIN   = 0x0B, // Начало отображения
				MAP_END     = 0x0C, // Конец отображения
				DOCUMENT    = 0x0D, // Документ разобран до конца, следом вправе идти новый
				FINISH      = 0x0E, // Запись разобрана до конца, событие видно после цикла разбора
				STRING_BEGIN = 0x0F, // Начало строки, собираемой кусками
				STRING_END   = 0x10, // Конец строки, собираемой кусками
				BLOB_BEGIN   = 0x11, // Начало двоичных данных, собираемых кусками
				BLOB_END     = 0x12, // Конец двоичных данных, собираемых кусками
				CUSTOM       = 0x13  // Открытое расширение, заведённое потребителем
			};

			/**
			 * \~russian
			 * @brief Правила обращения с повторяющимся именем поля отображения
			 *
			 * @details Молчаливый выбор одного из значений означал бы потерю данных, оттого
			 * отказ и взят по умолчанию
			 *
			 * \~english
			 * @brief Rules of the handling of a repeating name of a field of a mapping
			 * @details A silent choice of one of the values would mean a loss of the data, whereby
			 * the refusal is taken by default
			 *
			 * \~
			 */
			enum class duplicate_t : uint8_t {
				REFUSE = 0x00, // Разбор отвечает отказом
				FIRST  = 0x01, // Оставляется значение, встреченное первым
				LAST   = 0x02, // Оставляется значение, встреченное последним
				KEEP   = 0x03  // Оставляются все, доступ по имени выдаёт первое
			};

			/**
			 * \~russian
			 * @note Правила эти живут РАЗНЫМИ слоями кодека, и знать о том потребителю
			 * должно. `REFUSE` отвечается потоковым разбором: повтор виден ему сразу, и
			 * запись отвергается кодом `DUPLICATE_KEY`, не дойдя до дерева. `FIRST` и
			 * `LAST` осуществлены ДЕРЕВОМ: выбор одного из двух значений требует видеть
			 * отображение целиком, а события выдаются по одному и назад разбор не ходит,
			 * оттого потоковый разбор пропускает их наравне с `KEEP`, а выбор ведётся в
			 * миг закрытия отображения. Потребителю потока правила эти обещанного не
			 * дают, и опираться на них следует лишь при разборе в дерево либо в значение
			 *
			 * @note Сличение имён у `FIRST` и `LAST` ведётся по ЗНАЧЕНИЮ узлов, тогда как
			 * `REFUSE` сличает полную запись имени вместе с меткой. Оттого два имени, несущие
			 * одно число, но писанные разной шириною метки, деревом сочтутся повтором, а
			 * потоковым разбором - различными именами. Расхождение это намеренно: `FIRST`
			 * и `LAST` выбирают между значениями, и ширина метки к выбору отношения не имеет
			 *
			 * \~english
			 * @note These rules live in DIFFERENT layers of the codec. `REFUSE` is answered by the
			 * streaming parsing, while `FIRST` and `LAST` are realized by the tree and are passed
			 * through by the streaming parsing on a par with `KEEP`
			 *
			 * \~
			 */

			/**
			 * \~russian
			 * @brief Правила обращения со строкой, не отвечающей кодировке UTF-8
			 *
			 * @details Строка записи объявлена кодировкой UTF-8, и байты, ей не отвечающие,
			 * разбор отвергает. Данные, кодировке не подчинённые, записываются двоичным
			 * значением, а не строкой, - на то оно и заведено
			 *
			 * \~english
			 * @brief Rules of the handling of a string not conforming to the UTF-8 encoding
			 * @details A string of the record is declared to be in the UTF-8 encoding, and the bytes not conforming to it
			 * the parsing rejects. The data not subordinate to an encoding are written by a binary
			 * value rather than by a string — that is what it is made for
			 *
			 * \~
			 */
			enum class malformed_t : uint8_t {
				REFUSE  = 0x00, // Запись отвергается, ничего не записав
				REPLACE = 0x01, // Негодная последовательность заменяется знаком U+FFFD
				PASS    = 0x02  // Октеты пропускаются как есть
			};

			/**
			 * \~russian
			 * @brief Отрезок в хранилище октетов
			 *
			 * @details Отрезок хранит смещение и длину, а не указатель: хранилище растёт
			 * дописыванием и вправе переехать в памяти, и указатель после переезда стал бы
			 * недействителен
			 *
			 * \~english
			 * @brief Segment in the storage of the octets
			 * @details A segment holds an offset and a length rather than a pointer: the storage grows
			 * by an appending and may move in the memory, and a pointer after the move would become
			 * invalid
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Span {
				// Смещение начала отрезка в хранилище октетов
				uint32_t offset;
				// Длина отрезка в октетах
				uint32_t length;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * \~
				 */
				Span() noexcept : offset(0), length(0) {}
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param offset смещение начала отрезка в хранилище октетов
				 * @param length длина отрезка в октетах
				 *
				 * \~english
				 * @brief Constructor
				 * @param offset offset of the beginning of the segment in the storage of the octets
				 * @param length length of the segment in octets
				 *
				 * \~
				 */
				Span(const uint32_t offset, const uint32_t length) noexcept : offset(offset), length(length) {}
			} span_t;

			/**
			 * \~russian
			 * @brief Кусок выдачи собранной записи
			 *
			 * @details Служит выдаче записи без склейки её в один буфер: куски подаются
			 * средству записи россыпью, и содержимое, взятое ссылкой, не копируется вовсе
			 *
			 * @note Кусок ссылается на чужую память и живёт не дольше её: содержимое,
			 * уложенное ссылкой, обязано пережить выдачу записи
			 *
			 * \~english
			 * @brief Piece of the issuance of an assembled record
			 * @details Serves the issuance of a record without gluing it into one buffer: the pieces are submitted
			 * to the means of the writing scattered, and the content taken by a reference is not copied at all
			 * @note A piece refers to a foreign memory and lives no longer than it: the content
			 * laid by a reference is obliged to outlive the issuance of the record
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Piece {
				// Буфер октетов куска выдачи
				const void * buffer;
				// Размер октетов куска выдачи
				size_t size;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * \~
				 */
				Piece() noexcept : buffer(nullptr), size(0) {}
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param buffer буфер октетов куска выдачи
				 * @param size   размер октетов куска выдачи
				 *
				 * \~english
				 * @brief Constructor
				 * @param buffer buffer of the octets of the piece of the issuance
				 * @param size size of the octets of the piece of the issuance
				 *
				 * \~
				 */
				Piece(const void * buffer, const size_t size) noexcept : buffer(buffer), size(size) {}
			} piece_t;

			/**
			 * \~russian
			 * @brief Положение в поданной записи
			 *
			 * @details Служит для указания места отказа и для привязки значений к поданной
			 * записи. Строк и столбцов у двоичной записи нет, оттого положение и задаётся
			 * одним смещением
			 *
			 * \~english
			 * @brief Position in the submitted record
			 * @details Serves for the indication of the place of a refusal and for the binding of the values to the submitted
			 * record. A binary record has no lines and columns, whereby the position is set
			 * by one offset
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Location {
				// Смещение от начала записи в октетах
				uint64_t offset;
				// Глубина вложенности, считая с нуля
				uint32_t depth;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * \~
				 */
				Location() noexcept : offset(NO_OFFSET), depth(0) {}
			} location_t;

			/**
			 * \~russian
			 * @brief Функция получения текстового описания кода отказа
			 *
			 * @param error код отказа разбора
			 * @return      текстовое описание кода отказа
			 *
			 * \~english
			 * @brief Function of the obtaining of the textual description of a refusal code
			 * @param error error code of the parsing
			 * @return textual description of the error code
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ const char * message(const error_t error) noexcept;
			/**
			 * \~russian
			 * @brief Функция получения названия вида узла
			 *
			 * @param kind вид узла документа
			 * @return     название вида узла
			 *
			 * \~english
			 * @brief Function of the obtaining of the name of a kind of a node
			 * @param kind kind of a node of a document
			 * @return name of the kind of the node
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ const char * name(const kind_t kind) noexcept;
			/**
			 * \~russian
			 * @brief Функция получения названия вида значения
			 *
			 * @note Название выдаётся точному виду, а не сборному: сборный вид назван быть не
			 *       может, ибо он есть множество видов, а не вид
			 *
			 * @param type вид значения документа
			 * @return     название вида значения
			 *
			 * \~english
			 * @brief Function of the obtaining of the name of a kind of a value
			 * @note The name is issued for an exact kind rather than for a composite one: a composite kind cannot be
			 *       named, for it is a set of the kinds rather than a kind
			 * @param type kind of a value of a document
			 * @return name of the kind of the value
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ const char * name(const type_t type) noexcept;
			/**
			 * \~russian
			 * @brief Функция получения вида узла по виду значения
			 *
			 * @details Вид узла есть огрубление вида значения до словаря построения: всякое
			 * число, каким бы видом оно ни хранилось, есть узел вида `NUMBER`
			 *
			 * @param type вид значения документа
			 * @return     вид узла документа
			 *
			 * \~english
			 * @brief Function of the obtaining of the kind of a node by the kind of a value
			 * @details The kind of a node is a coarsening of the kind of a value down to the vocabulary of the construction: every
			 * number, by whichever kind it is stored, is a node of the kind `NUMBER`
			 * @param type kind of a value of a document
			 * @return kind of a node of a document
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ kind_t kind(const type_t type) noexcept;
			/**
			 * \~russian
			 * @brief Функция сборки ведущего октета значения
			 *
			 * @param group  крупный вид проволочной записи
			 * @param detail подробность метки
			 * @return       собранный ведущий октет
			 *
			 * \~english
			 * @brief Function of the assembling of the leading octet of a value
			 * @param group group kind of the wire record
			 * @param detail detail of the tag
			 * @return assembled leading octet
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ uint8_t tag(const group_t group, const uint8_t detail) noexcept;
			/**
			 * \~russian
			 * @brief Функция извлечения крупного вида из ведущего октета
			 *
			 * @param tag ведущий октет значения
			 * @return    крупный вид проволочной записи
			 *
			 * \~english
			 * @brief Function of the extraction of the group kind from the leading octet
			 * @param tag leading octet of a value
			 * @return group kind of the wire record
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ group_t group(const uint8_t tag) noexcept;
			/**
			 * \~russian
			 * @brief Функция извлечения подробности из ведущего октета
			 *
			 * @param tag ведущий октет значения
			 * @return    подробность метки
			 *
			 * \~english
			 * @brief Function of the extraction of the detail from the leading octet
			 * @param tag leading octet of a value
			 * @return detail of the tag
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ uint8_t detail(const uint8_t tag) noexcept;
			/**
			 * \~russian
			 * @brief Функция получения ширины записи, ведомой подробностью метки
			 *
			 * @details Подробность до `INLINE_LIMIT` включительно несёт само значение, и
			 * ширина ведомой записи равна нулю. Подробности выше означают ширину в один, два,
			 * четыре и восемь октетов
			 *
			 * @note Подробности от `0x1C` до `0x1E` отведены под будущее, а `0x1F` означает
			 *       у вместимого неопределённую длину, а у одиночного значения - конец такого
			 *       вместимого. Ширины они не ведут, и работа отвечает отказом на все четыре
			 *
			 * @param detail подробность метки
			 * @param result ширина ведомой записи в октетах
			 * @return       признак опознания подробности
			 *
			 * \~english
			 * @brief Function of the obtaining of the width of the record led by the detail of a tag
			 * @details A detail up to `INLINE_LIMIT` inclusive carries the value itself, and
			 * the width of the led record equals zero. The details above mean a width of one, two,
			 * four and eight octets
			 * @note The details from `0x1C` to `0x1E` are reserved for the future, while `0x1F` means
			 *       an indefinite length for a container and the end of such a container for a singleton value.
			 *       They lead no width, and the work answers with a refusal for all the four
			 * @param detail detail of the tag
			 * @param result width of the led record in octets
			 * @return sign of the recognition of the detail
			 *
			 * \~
			 */
			[[nodiscard]] __AWH_SHARED_EXPORT__ bool width(const uint8_t detail, uint8_t & result) noexcept;
			/**
			 * \~russian
			 * @brief Функция получения наименьшей подробности, вмещающей значение
			 *
			 * @details Служит записи: значение до `INLINE_LIMIT` укладывается в саму метку, а
			 * прочие получают наименьшую ширину, какая их вмещает. Наименьшая запись обязана
			 * быть единственной, иначе одно и то же значение записалось бы двумя видами, и
			 * строгий вид записи стал бы невозможен
			 *
			 * @param value укладываемое значение
			 * @return      подробность метки
			 *
			 * \~english
			 * @brief Function of the obtaining of the smallest detail containing a value
			 * @details Serves the writing: a value up to `INLINE_LIMIT` is laid into the tag itself, while
			 * the others receive the smallest width which contains them. The smallest record must
			 * be the only one, otherwise one and the same value would be written by two kinds, and
			 * a strict kind of the record would become impossible
			 * @param value value being laid
			 * @return detail of the tag
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ uint8_t fit(const uint64_t value) noexcept;
			/**
			 * \~russian
			 * @brief Функция выдачи очередного звена пути
			 *
			 * @details Разбор пути держится ЗДЕСЬ, в одном месте на всех, и заведён он
			 * намеренно общим: правила эти нужны четверым - извлечению и заведению у
			 * владеющего значения, извлечению и опросу наличия у дерева разбора, - а
			 * сторож, поставленный на одной дороге и забытый на близнеце, есть почерк,
			 * каким вскрыты двенадцать находок этого кодека
			 *
			 * @details Правила равняются на RFC 6901. Путь пустой значит ВЕСЬ документ и
			 * звеньев не даёт вовсе. Ведущая косая черта есть приставка указателя и
			 * отбрасывается, а не даёт пустого звена: `/a/b` суть два звена. Косая черта
			 * ОДНА даёт одно звено с пустым именем. Звено пустое ВНУТРИ пути есть законное
			 * имя поля, а не пропуск: `a//b` суть три звена, среднее из них - поле с
			 * пустым именем
			 *
			 * @note Путь без ведущей косой черты принимается наравне с указателем: `a/b`
			 * даёт то же, что `/a/b`. Оставлено ради написанного прежде
			 *
			 * @note О том, взять ли звено номером либо именем, работа эта НЕ судит: судит
			 * о том вид ВМЕСТИЛИЩА у зовущего, как то и велит RFC 6901
			 *
			 * @param path   разбираемый путь
			 * @param offset смещение разбора, изменяемое работой
			 * @param result выдаваемое звено пути
			 * @return       признак выданного звена
			 *
			 * \~english
			 * @brief Function of the issuance of the next segment of a path
			 * @param path path being parsed
			 * @param offset offset of the parsing, changed by the function
			 * @param result segment of the path being issued
			 * @return sign of an issued segment
			 *
			 * \~
			 */
			[[nodiscard]] __AWH_SHARED_EXPORT__ bool segment(const string_view path, size_t & offset, string_view & result,
			 string & buffer, bool & invalid) noexcept;
			/**
			 * \~russian
			 * @brief Функция разбора звена пути номером значения
			 *
			 * @details Работа эта решает лишь ОДНО: годится ли запись звена номером. О том
			 * же, брать ли звено номером, судит вид ВМЕСТИЛИЩА у зовущего: у отображения
			 * звено есть имя поля всегда, хотя бы и записанное цифрами
			 *
			 * @details Ведущий нуль номером негоден по RFC 6901 (раздел 4): без того `01` и
			 * `1` означали бы одно и то же, и путь перестал бы задавать значение однозначно
			 *
			 * @note Работа вынесена сюда из владеющего значения 03.09.2026, когда путь
			 * понадобился и дереву разбора: правила пути держатся В ОДНОМ месте на всех
			 *
			 * @param segment разбираемое звено пути
			 * @param result  разобранный номер значения
			 * @return        признак годности записи номером
			 *
			 * \~english
			 * @brief Function of the parsing of a segment of a path as an index of a value
			 * @param segment segment of the path being parsed
			 * @param result parsed index of the value
			 * @return sign of the fitness of the record as an index
			 *
			 * \~
			 */
			[[nodiscard]] __AWH_SHARED_EXPORT__ bool indexed(const string_view segment, size_t & result) noexcept;
		};
	};
};

/**
 * Возвращаем системные макросы потребителю библиотеки:
 * имена, подавленные в начале файла, снова принадлежат ему
 */
#include "../../sys/macro/restore.hpp"

#endif // __AWH_CODEC_ABC_COMMON__
