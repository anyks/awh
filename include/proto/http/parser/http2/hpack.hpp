/**
 * @file: hpack.hpp
 * @date: 2026-07-19
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл кодека HPACK (RFC 7541) — статическая и динамическая таблицы заголовков,
 *        кодирование и декодирование целых с префиксом, Хаффман-кодирование,
 *        а также кодер и декодер блоков заголовков с защитой от decompression bomb
 *
 * \~english
 * @brief Header file of the codec of HPACK (RFC 7541) — the static and the dynamic tables of the headers,
 *        the encoding and the decoding of the integers with a prefix, the Huffman encoding,
 *        and also the encoder and the decoder of the blocks of the headers with a protection from a decompression bomb
 *
 * \~
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_HTTP_PARSER_HTTP2_HPACK__
#define __AWH_HTTP_PARSER_HTTP2_HPACK__

/**
 * Если компилятор принадлежит к семейству Visual Studio
 */
#if defined(_MSC_VER)
	/**
	 * Принудительная подстановка средствами Visual Studio
	 */
	#define AWH_ASCII_INLINE __forceinline
/**
 * Если компилятор принадлежит к семейству GCC или Clang
 */
#else
	/**
	 * Принудительная подстановка средствами GCC и Clang
	 */
	#define AWH_ASCII_INLINE inline __attribute__((always_inline))
#endif

/**
 * Стандартные заголовочные файлы
 */
#include <deque>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <unordered_map>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "h2.hpp"
#include "../../../../sys/global.hpp"

/**
 * \~russian
 * @brief основное пространство имён
 *
 *
 * \~english
 * @brief main namespace
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
	 * @brief Пространство имён HTTP-протокола
	 *
	 *
	 * \~english
	 * @brief HTTP protocol namespace
	 *
	 * \~
	 */
	namespace http {
		/**
		 * \~russian
		 * @brief Пространство имён внутренних слоёв протокола HTTP/2
		 *
		 * \~english
		 * @brief Namespace of the internal layers of the HTTP/2 protocol
		 *
		 * \~
		 */
		namespace h2 {
			/**
			 * \~russian
			 * @brief Пространство имён HPACK - сжатие заголовков HTTP/2 (RFC 7541)
			 *
			 * @details HPACK - отдельный самодостаточный кодек. Состоит из:
			 *          1. целочисленного кодирования с префиксом переменной длины (RFC 7541 §5.1);
			 *          2. строкового кодирования - литерал или Huffman (RFC 7541 §5.2);
			 *          3. статической таблицы - 61 запись (RFC 7541 §2.3.1, Appendix A);
			 *          4. динамической таблицы с вытеснением по размеру (RFC 7541 §2.3.2);
			 *          5. Huffman-кодирования по фиксированной таблице (RFC 7541 Appendix B).
			 *          Это главный источник уязвимостей (decompression bomb), поэтому декодер
			 *          обязан жёстко ограничивать суммарный размер распакованного списка заголовков.
			 *
			 * \~english
			 * @brief Namespace of HPACK - the compression of the headers of HTTP/2 (RFC 7541)
			 * @details HPACK is a separate self-sufficient codec. It consists of:
			 *          1. an integer encoding with a prefix of a variable length (RFC 7541 §5.1);
			 *          2. a string encoding - a literal or a Huffman one (RFC 7541 §5.2);
			 *          3. a static table - 61 records (RFC 7541 §2.3.1, Appendix A);
			 *          4. a dynamic table with an eviction by the size (RFC 7541 §2.3.2);
			 *          5. a Huffman encoding by a fixed table (RFC 7541 Appendix B).
			 *          This is the main source of the vulnerabilities (a decompression bomb), therefore the decoder
			 *          is obliged to limit rigidly the total size of the unpacked list of the headers.
			 *
			 * \~
			 */
			namespace hpack {
				/**
				 * \~russian
				 * @brief Количество записей в статической таблице (RFC 7541 Appendix A)
				 *
				 * \~english
				 * @brief Number of the records in the static table (RFC 7541 Appendix A)
				 *
				 * \~
				 */
				static constexpr size_t STATIC_TABLE_SIZE = 61;

				/**
				 * \~russian
				 * @brief Структура записи статической таблицы (zero-copy: ссылки на статические литералы)
				 *
				 * \~english
				 * @brief Structure of a record of the static table (zero-copy: the references to the static literals)
				 *
				 * \~
				 */
				typedef struct Static_Entry {
					// Название заголовка
					string_view name;
					// Значение заголовка
					string_view value;
				} static_entry_t;

				/**
				 * \~russian
				 * @brief Функция получения записи статической таблицы по индексу 1..61 (RFC 7541 Appendix A)
				 *
				 * @param index индекс записи (1-based); 0 или > 61 - невалиден
				 * @return      указатель на запись либо nullptr
				 *
				 * \~english
				 * @brief Function of getting a record of the static table by an index 1..61 (RFC 7541 Appendix A)
				 * @param index index of the record (1-based); a 0 or > 61 is invalid
				 * @return      pointer to the record or nullptr
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ const static_entry_t * staticTable(const size_t index) noexcept;

				/**
				 * \~russian
				 * @brief Структура пары декодированного заголовка (невладеющая)
				 *
				 * @details Название и значение указывают во внутреннюю арену декодера и
				 *          действительны до следующего вызова decode() на том же декодере
				 *          (а также до его reset/уничтожения). Если заголовок нужен дольше -
				 *          копируйте его значение.
				 *
				 * \~english
				 * @brief Structure of a pair of a decoded header (non-owning)
				 * @details The name and the value point into the internal arena of the decoder and
				 *          are valid until the next call of decode() on the same decoder
				 *          (and also until its reset/destruction). If a header is needed longer -
				 *          copy its value.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Field_View {
					public:
						// Название заголовка
						string_view name;
						// Значение заголовка
						string_view value;
						/**
						 * Значение получено представлением Literal Never Indexed (RFC 7541 §6.2.3):
						 * при перекодировании заголовок обязан остаться never indexed и не попасть
						 * в динамическую таблицу (RFC 7541 §7.1.3)
						 */
						bool sensitive;
					public:
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
						explicit Field_View() noexcept;
				} field_view_t;

				/**
				 * \~russian
				 * @brief Класс пары заголовка
				 *
				 * @details Название и значение - владеющие копии: используется на стороне
				 *          кодирования, где данные принадлежат вызывающему коду.
				 *
				 * \~english
				 * @brief Class of a pair of a header
				 * @details The name and the value are owning copies: it is used on the side
				 *          of the encoding, where the data belongs to the calling code.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Field {
					public:
						// Название заголовка
						string name;
						// Значение заголовка
						string value;
						/**
						 * Чувствительное значение (RFC 7541 §7.1.3): кодировать как Literal Never Indexed
						 * и не заносить в динамическую таблицу (защита от CRIME-подобных атак).
						 * Кодер дополнительно автоматически считает чувствительными authorization/cookie.
						 */
						bool sensitive;
					public:
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
						explicit Field() noexcept;
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * @param name  название заголовка
						 * @param value значение заголовка
						 *
						 * \~english
						 * @brief Constructor
						 * @param name  name of the header
						 * @param value value of the header
						 *
						 * \~
						 */
						explicit Field(string name, string value) noexcept;
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * @param name      название заголовка
						 * @param value     значение заголовка
						 * @param sensitive флаг чувствительного значения
						 *
						 * \~english
						 * @brief Constructor
						 * @param name      name of the header
						 * @param value     value of the header
						 * @param sensitive flag of a sensitive value
						 *
						 * \~
						 */
						explicit Field(string name, string value, const bool sensitive) noexcept;
				} field_t;

				/**
				 * \~russian
				 * @brief Функция сравнения названий и значений заголовков
				 *
				 * @details Названия и значения заголовков HTTP короткие: медиана названия -
				 *          восемь октетов, а вызов memcmp через таблицу связывания стоит дороже
				 *          самого сравнения. Сравнение ведётся перекрывающимися машинными
				 *          словами: чтения не выходят за пределы строки, поэтому границы
				 *          буфера не нарушаются даже на длине в один октет
				 *
				 * @note Функция общая для HPACK и QPACK: поиск по таблицам устроен у них
				 *       одинаково, и сравнение строк там - самая горячая операция кодирования
				 *
				 * @param first  первая сравниваемая строка
				 * @param second вторая сравниваемая строка
				 * @return       признак совпадения строк
				 *
				 * \~english
				 * @brief Function of the comparison of the names and of the values of the headers
				 * @details The names and the values of the headers of HTTP are short: the median of a name is
				 *          eight octets, while a call of memcmp through the table of the linking costs more
				 *          than the comparison itself. The comparison is conducted by the overlapping machine
				 *          words: the readings do not go beyond the limits of the string, therefore the boundaries
				 *          of the buffer are not violated even at a length of one octet
				 * @note The function is common for HPACK and QPACK: the search by the tables is arranged at them
				 *       identically, and the comparison of the strings there is the hottest operation of the encoding
				 * @param first  first string being compared
				 * @param second second string being compared
				 * @return       flag of the coincidence of the strings
				 *
				 * \~
				 */
				AWH_ASCII_INLINE bool sameText(string_view first, string_view second) noexcept {
					// Получаем длину сравниваемых строк
					const size_t length = first.size();
					// Если длины строк не совпадают
					if(length != second.size())
						// Выводим признак несовпадения строк
						return false;
					// Получаем указатель на первую сравниваемую строку
					const char * left = first.data();
					// Получаем указатель на вторую сравниваемую строку
					const char * right = second.data();
					/**
					 * Строки длиной в машинное слово и больше сравниваются двумя чтениями
					 * с перекрытием: первым машинным словом и последним
					 */
					if(length >= sizeof(uint64_t)){
						// Если строка не помещается в два машинных слова
						if(length > (sizeof(uint64_t) * 2))
							// Выводим результат сравнения строк целиком
							return (::memcmp(left, right, length) == 0);
						// Первое машинное слово первой строки
						uint64_t headLeft = 0;
						// Первое машинное слово второй строки
						uint64_t headRight = 0;
						// Последнее машинное слово первой строки
						uint64_t tailLeft = 0;
						// Последнее машинное слово второй строки
						uint64_t tailRight = 0;
						// Читаем первое машинное слово первой строки
						::memcpy(&headLeft, left, sizeof(uint64_t));
						// Читаем первое машинное слово второй строки
						::memcpy(&headRight, right, sizeof(uint64_t));
						// Читаем последнее машинное слово первой строки
						::memcpy(&tailLeft, (left + length - sizeof(uint64_t)), sizeof(uint64_t));
						// Читаем последнее машинное слово второй строки
						::memcpy(&tailRight, (right + length - sizeof(uint64_t)), sizeof(uint64_t));
						// Выводим результат сравнения обоих машинных слов
						return ((headLeft == headRight) && (tailLeft == tailRight));
					}
					/**
					 * Строки короче машинного слова, но не короче половины, сравниваются
					 * двумя половинными чтениями с перекрытием
					 */
					if(length >= sizeof(uint32_t)){
						// Первая половина первой строки
						uint32_t headLeft = 0;
						// Первая половина второй строки
						uint32_t headRight = 0;
						// Вторая половина первой строки
						uint32_t tailLeft = 0;
						// Вторая половина второй строки
						uint32_t tailRight = 0;
						// Читаем первую половину первой строки
						::memcpy(&headLeft, left, sizeof(uint32_t));
						// Читаем первую половину второй строки
						::memcpy(&headRight, right, sizeof(uint32_t));
						// Читаем вторую половину первой строки
						::memcpy(&tailLeft, (left + length - sizeof(uint32_t)), sizeof(uint32_t));
						// Читаем вторую половину второй строки
						::memcpy(&tailRight, (right + length - sizeof(uint32_t)), sizeof(uint32_t));
						// Выводим результат сравнения обеих половин
						return ((headLeft == headRight) && (tailLeft == tailRight));
					}
					// Если строки пусты
					if(length == 0)
						// Выводим признак совпадения строк
						return true;
					/**
					 * Строки короче половины машинного слова сравниваются тремя октетами:
					 * первым, средним и последним. На длине от одного до трёх октетов
					 * эти три позиции покрывают строку целиком
					 */
					return (
						(left[0] == right[0]) &&
						(left[length >> 1] == right[length >> 1]) &&
						(left[length - 1] == right[length - 1])
					);
				}

				/**
				 * \~russian
				 * @brief Пространство имён функций Хаффман-кодирования (RFC 7541 Appendix B)
				 *
				 * \~english
				 * @brief Namespace of the functions of the Huffman encoding (RFC 7541 Appendix B)
				 *
				 * \~
				 */
				namespace huffman {
					/**
					 * \~russian
					 * @brief Функция вычисления длины строки в байтах после Huffman-кодирования
					 *
					 * @note Используется для выбора способа кодирования (литерал/Huffman)
					 *
					 * @param input строка для вычисления
					 * @return      длина строки после кодирования
					 *
					 * \~english
					 * @brief Function of the calculation of the length of a string in octets after the Huffman encoding
					 * @note It is used for the choice of the way of the encoding (a literal/a Huffman one)
					 * @param input string for the calculation
					 * @return      length of the string after the encoding
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ size_t length(string_view input) noexcept;
					/**
					 * \~russian
					 * @brief Функция кодирования строки Huffman'ом (RFC 7541 Appendix B)
					 *
					 * @param input  кодируемая строка
					 * @param output выходной буфер закодированной строки
					 *
					 * \~english
					 * @brief Function of the encoding of a string by a Huffman one (RFC 7541 Appendix B)
					 * @param input  string being encoded
					 * @param output output buffer of the encoded string
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ void encode(string_view input, string & output) noexcept;
					/**
					 * \~russian
					 * @brief Функция декодирования Huffman-строки (RFC 7541 Appendix B)
					 *
					 * @note Содержимое выходного буфера замещается, а не дополняется
					 *
					 * @param data   входной буфер
					 * @param size   доступно байт
					 * @param output выходной буфер декодированной строки
					 * @return       результат декодирования (false - некорректная последовательность, COMPRESSION_ERROR)
					 *
					 * \~english
					 * @brief Function of the decoding of a Huffman string (RFC 7541 Appendix B)
					 * @note The content of the output buffer is replaced rather than supplemented
					 * @param data   input buffer
					 * @param size   octets available
					 * @param output output buffer of the decoded string
					 * @return       result of the decoding (false - an incorrect sequence, COMPRESSION_ERROR)
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ bool decode(const uint8_t * data, const size_t size, string & output) noexcept;
					/**
					 * \~russian
					 * @brief Функция вычисления места, требуемого под декодированную строку
					 *
					 * @details Оценка сверху: кодов короче пяти бит в таблице RFC 7541 Appendix B
					 *          нет, значит символов не больше, чем бит делённых на пять. Сверх
					 *          оценки берутся два октета запаса под шаг табличного декодирования
					 *
					 * @param size длина закодированной строки в байтах
					 * @return     требуемое количество октетов выходного буфера
					 *
					 * \~english
					 * @brief Function of the calculation of the place required for a decoded string
					 * @details An estimation from above: there are no codes shorter than five bits in the table of RFC 7541 Appendix B,
					 *          which means there are no more characters than the bits divided by five. Above
					 *          the estimation two octets of a reserve are taken for the step of the table decoding
					 * @param size length of the encoded string in octets
					 * @return     required number of the octets of the output buffer
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ size_t space(const size_t size) noexcept;
					/**
					 * \~russian
					 * @brief Функция декодирования Huffman-строки в готовый буфер (RFC 7541 Appendix B)
					 *
					 * @note Выходной буфер обязан вмещать space(size) октетов: проверка границы
					 *       на каждый символ обошлась бы дороже самого декодирования
					 *
					 * @param data   входной буфер
					 * @param size   доступно байт
					 * @param output выходной буфер декодированной строки
					 * @return       длина декодированной строки, либо SIZE_MAX при некорректной последовательности
					 *
					 * \~english
					 * @brief Function of the decoding of a Huffman string into a ready buffer (RFC 7541 Appendix B)
					 * @note The output buffer is obliged to hold space(size) octets: a check of the boundary
					 *       on every character would cost more than the decoding itself
					 * @param data   input buffer
					 * @param size   octets available
					 * @param output output buffer of the decoded string
					 * @return       length of the decoded string, or SIZE_MAX at an incorrect sequence
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ size_t decode(const uint8_t * data, const size_t size, char * output) noexcept;
				};

				/**
				 * \~russian
				 * @brief Пространство имён функций кодирования/декодирования целых с префиксом переменной длины (RFC 7541 §5.1)
				 *
				 * \~english
				 * @brief Namespace of the functions of the encoding/decoding of the integers with a prefix of a variable length (RFC 7541 §5.1)
				 *
				 * \~
				 */
				namespace prefixed {
					/**
					 * \~russian
					 * @brief Функция кодирования целого с префиксом переменной длины (RFC 7541 §5.1)
					 *
					 * @note Старшие (8 - prefixBits) бит первого байта берутся из prefixValue
					 *
					 * @param output      выходной буфер
					 * @param value       кодируемое значение
					 * @param prefixBits  размер префикса в битах (1..8)
					 * @param prefixValue значение старших бит первого байта
					 *
					 * \~english
					 * @brief Function of the encoding of an integer with a prefix of a variable length (RFC 7541 §5.1)
					 * @note The higher (8 - prefixBits) bits of the first octet are taken from prefixValue
					 * @param output      output buffer
					 * @param value       value being encoded
					 * @param prefixBits  size of the prefix in bits (1..8)
					 * @param prefixValue value of the higher bits of the first octet
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ void encode(string & output, const uint64_t value, const uint8_t prefixBits, const uint8_t prefixValue) noexcept;
					/**
					 * \~russian
					 * @brief Функция декодирования целого с префиксом переменной длины (RFC 7541 §5.1)
					 *
					 * @param data       входной буфер
					 * @param size       доступно байт
					 * @param prefixBits размер префикса в битах (1..8)
					 * @param value      декодированное значение
					 * @param consumed   количество прочитанных байт
					 * @return           результат декодирования (OK / INCOMPLETE - мало данных / ERROR - переполнение)
					 *
					 * \~english
					 * @brief Function of the decoding of an integer with a prefix of a variable length (RFC 7541 §5.1)
					 * @param data       input buffer
					 * @param size       octets available
					 * @param prefixBits size of the prefix in bits (1..8)
					 * @param value      decoded value
					 * @param consumed   number of the read octets
					 * @return           result of the decoding (OK / INCOMPLETE - little data / ERROR - an overflow)
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ status_t decode(const uint8_t * data, const size_t size, const uint8_t prefixBits, uint64_t & value, size_t & consumed) noexcept;
				};

				/**
				 * \~russian
				 * @brief Класс динамической таблицы HPACK с вытеснением по размеру FIFO (RFC 7541 §2.3.2)
				 *
				 * @details Размер записи = len(name) + len(value) + 32 (RFC 7541 §4.1)
				 *
				 * \~english
				 * @brief Class of the dynamic table of HPACK with an eviction by the size FIFO (RFC 7541 §2.3.2)
				 * @details The size of a record = len(name) + len(value) + 32 (RFC 7541 §4.1)
				 *
				 * \~
				 */
				typedef class __AWH_SHARED_EXPORT__ DynamicTable {
					private:
						/**
						 * \~russian
						 * Список записей таблицы ([0] - самая свежая запись)
						 *
						 * @details Живые записи занимают начало списка, вытесненные остаются
						 *          лежать за ними до вызова release(). Отложенное вытеснение
						 *          позволяет декодеру отдавать наружу представления прямо
						 *          в строки таблицы вместо их копирования: запись, вытесненная
						 *          соседним заголовком того же блока, обязана дожить до конца
						 *          разбора. Список именно deque: вставка в начало и удаление
						 *          с конца не делают недействительными ссылки на прочие записи
						 *
						 * \~english
						 * List of the records of the table ([0] - the freshest record)
						 * @details The living records occupy the beginning of the list, the evicted ones remain
						 *          lying behind them until the call of release(). The postponed eviction
						 *          allows the decoder to issue outside the representations right
						 *          into the strings of the table instead of their copying: a record evicted
						 *          by a neighbouring header of the same block is obliged to live to the end
						 *          of the parsing. The list is exactly a deque: an insertion into the beginning and a removal
						 *          from the end do not invalidate the references to the other records
						 *
						 * \~
						 */
						deque <field_t> _entries;
						// Количество живых записей в начале списка
						size_t _live;
					private:
						// Текущий суммарный размер таблицы
						uint32_t _size;
						// Лимит размера таблицы
						uint32_t _maxSize;
					private:
						/**
						 * \~russian
						 * Общее количество когда-либо добавленных записей
						 *
						 * @details Добавление записи сдвигает индексы всех остальных, поэтому
						 *          в индексе хранится не позиция, а сквозной номер добавления.
						 *          Живые записи всегда занимают непрерывный отрезок номеров,
						 *          и позиция получается вычитанием: index = _inserts - seq + 1
						 *
						 * \~english
						 * Total number of the ever added records
						 * @details An addition of a record shifts the indices of all the rest, therefore
						 *          in the index not a position is stored but a through number of the addition.
						 *          The living records always occupy a continuous segment of the numbers,
						 *          and the position is obtained by a subtraction: index = _inserts - seq + 1
						 *
						 * \~
						 */
						uint64_t _inserts;
						/**
						 * \~russian
						 * Индекс записей по хешу пары название-значение (хеш -> номер добавления)
						 *
						 * @details Без индекса поиск заголовка в таблице линейный, а выполняется он
						 *          для каждого заголовка каждого блока: на таблице по умолчанию это
						 *          две трети стоимости кодирования. Ключом служит хеш пары, а не
						 *          одного названия: заголовков с одинаковым названием и разными
						 *          значениями в таблице бывают десятки (:path, cookie, set-cookie),
						 *          и по хешу названия они все попадали бы в одно ведро, вырождая
						 *          поиск обратно в перебор. Ключом служит именно хеш, а не
						 *          представление в строку записи: таблица копируется целиком при
						 *          сбросе соединения, и представления указывали бы в строки
						 *          копии-источника. Совпадение хешей проверяется сравнением строк
						 *
						 * \~english
						 * Index of the records by the hash of a pair a name-a value (a hash -> a number of the addition)
						 * @details Without the index the search of a header in the table is a linear one, and it is performed
						 *          for every header of every block: on the table by default this is
						 *          two thirds of the cost of the encoding. The key is the hash of the pair rather than
						 *          of the name alone: the headers with the same name and different
						 *          values happen in the table by dozens (:path, cookie, set-cookie),
						 *          and by the hash of the name they would all get into one bucket, degenerating
						 *          the search back into an enumeration. The key is exactly the hash rather than
						 *          a representation into the string of the record: the table is copied entirely at
						 *          a reset of the connection, and the representations would point into the strings
						 *          of the copy-source. A coincidence of the hashes is checked by a comparison of the strings
						 *
						 * \~
						 */
						unordered_multimap <size_t, uint64_t> _index;
						/**
						 * \~russian
						 * Индекс самой свежей записи с каждым названием (хеш названия -> номер добавления)
						 *
						 * @details Нужен для ссылки только по названию, когда полного совпадения нет.
						 *          Хранится ровно одна запись на название - самая свежая: перебирать
						 *          нечего, а вытеснение идёт с самой старой, поэтому запись индекса
						 *          удаляется, когда уходит последняя запись с этим названием
						 *
						 * \~english
						 * Index of the freshest record with every name (a hash of the name -> a number of the addition)
						 * @details It is needed for a reference by the name alone, when there is no full coincidence.
						 *          Exactly one record per name is stored - the freshest one: there is nothing
						 *          to enumerate, while the eviction goes from the oldest one, therefore a record of the index
						 *          is removed when the last record with this name goes away
						 *
						 * \~
						 */
						unordered_map <size_t, uint64_t> _names;
					private:
						/**
						 * \~russian
						 * Признак сопровождения индекса записей
						 *
						 * @details Индекс нужен только кодеру: декодер обращается к таблице
						 *          по готовому номеру и поиском по названию не пользуется,
						 *          а сопровождение индекса стоит выделения памяти на запись
						 *
						 * \~english
						 * Flag of the maintenance of the index of the records
						 * @details The index is needed only by the encoder: the decoder addresses the table
						 *          by a ready number and does not make use of a search by the name,
						 *          while the maintenance of the index costs an allocation of the memory per record
						 *
						 * \~
						 */
						bool _indexing;
					private:
						/**
						 * \~russian
						 * @brief Метод вытеснения записей с конца, пока размер не уложится в лимит
						 *
						 * \~english
						 * @brief Method of the eviction of the records from the end until the size fits into the limit
						 *
						 * \~
						 */
						void evict() noexcept;
					public:
						/**
						 * \~russian
						 * @brief Метод получения количества записей таблицы
						 *
						 * @return количество записей таблицы
						 *
						 * \~english
						 * @brief Method of getting the number of the records of the table
						 * @return number of the records of the table
						 *
						 * \~
						 */
						size_t count() const noexcept;
						/**
						 * \~russian
						 * @brief Метод получения количества удерживаемых вытесненных записей
						 *
						 * @details Вытесненная запись остаётся на месте до вызова release():
						 *          представление в её строки могло быть выдано наружу соседним
						 *          заголовком того же блока. Наружу счётчик выведен ради проверки
						 *          того, что удержание ограничено: блок, превысивший лимит списка,
						 *          обязан оставить его нулевым
						 *
						 * @return количество вытесненных, но ещё удерживаемых записей
						 *
						 * \~english
						 * @brief Method of getting the number of the held evicted records
						 * @details An evicted record remains in place until the call of release():
						 *          a representation into its strings could have been issued outside by a neighbouring
						 *          header of the same block. Outside the counter is brought for the sake of a checking
						 *          that the holding is limited: a block which has exceeded the limit of the list
						 *          is obliged to leave it zero
						 * @return number of the evicted but still held records
						 *
						 * \~
						 */
						size_t retained() const noexcept;
						/**
						 * \~russian
						 * @brief Метод получения текущего суммарного размера таблицы
						 *
						 * @return текущий суммарный размер таблицы
						 *
						 * \~english
						 * @brief Method of getting the current total size of the table
						 * @return current total size of the table
						 *
						 * \~
						 */
						uint32_t size() const noexcept;
						/**
						 * \~russian
						 * @brief Метод получения лимита размера таблицы
						 *
						 * @return лимит размера таблицы
						 *
						 * \~english
						 * @brief Method of getting the limit of the size of the table
						 * @return limit of the size of the table
						 *
						 * \~
						 */
						uint32_t maxSize() const noexcept;
					public:
						/**
						 * \~russian
						 * @brief Метод изменения максимального размера таблицы (Dynamic Table Size Update)
						 *
						 * @note Лишние записи вытесняются сразу
						 *
						 * @param maxSize новый максимальный размер таблицы
						 *
						 * \~english
						 * @brief Method of changing the largest size of the table (a Dynamic Table Size Update)
						 * @note The superfluous records are evicted at once
						 * @param maxSize new largest size of the table
						 *
						 * \~
						 */
						void setMaxSize(const uint32_t maxSize) noexcept;
						/**
						 * \~russian
						 * @brief Метод доступа к записи по индексу (1-based внутри динамической части)
						 *
						 * @param index индекс записи
						 * @return      указатель на запись либо nullptr
						 *
						 * \~english
						 * @brief Method of the access to a record by an index (1-based inside the dynamic part)
						 * @param index index of the record
						 * @return      pointer to the record or nullptr
						 *
						 * \~
						 */
						const field_t * at(const size_t index) const noexcept;
						/**
						 * \~russian
						 * @brief Метод поиска записи по названию и значению заголовка
						 *
						 * @note Индексы 1-based внутри динамической части: к ним прибавляется
						 *       размер статической таблицы для получения объединённого индекса
						 *
						 * @param name      название искомого заголовка
						 * @param value     значение искомого заголовка
						 * @param nameIndex индекс совпадения только по названию заголовка
						 * @return          индекс полного совпадения либо 0
						 *
						 * \~english
						 * @brief Method of the search of a record by the name and the value of a header
						 * @note The indices are 1-based inside the dynamic part: to them the size
						 *       of the static table is added for the getting of a united index
						 * @param name      name of the sought header
						 * @param value     value of the sought header
						 * @param nameIndex index of a coincidence by the name of the header alone
						 * @return          index of a full coincidence or 0
						 *
						 * \~
						 */
						uint64_t find(string_view name, string_view value, uint64_t & nameIndex) const noexcept;
						/**
						 * \~russian
						 * @brief Метод поиска записи по названию и значению заголовка с готовым хешем названия
						 *
						 * @details Хеш названия нужен и индексу статической таблицы, и индексу
						 *          динамической. Вычислять его дважды на каждый кодируемый
						 *          заголовок незачем, поэтому вызывающая сторона считает его
						 *          один раз и передаёт готовым
						 *
						 * @param hashName  хеш названия искомого заголовка
						 * @param name      название искомого заголовка
						 * @param value     значение искомого заголовка
						 * @param nameIndex индекс совпадения только по названию заголовка
						 * @param needName  требуется ли ссылка на название заголовка
						 * @return          индекс полного совпадения либо 0
						 *
						 * \~english
						 * @brief Method of the search of a record by the name and the value of a header with a ready hash of the name
						 * @details The hash of the name is needed both by the index of the static table and by the index
						 *          of the dynamic one. There is no point in calculating it twice per encoded
						 *          header, therefore the calling side counts it
						 *          once and passes it ready
						 * @param hashName  hash of the name of the sought header
						 * @param name      name of the sought header
						 * @param value     value of the sought header
						 * @param nameIndex index of a coincidence by the name of the header alone
						 * @param needName  whether a reference to the name of the header is required
						 * @return          index of a full coincidence or 0
						 *
						 * \~
						 */
						uint64_t find(const size_t hashName, string_view name, string_view value, uint64_t & nameIndex, const bool needName) const noexcept;
						/**
						 * \~russian
						 * @brief Метод поиска записи только по названию заголовка
						 *
						 * @details Нужен там, где значение заведомо не индексируется -
						 *          для чувствительных заголовков. Полный поиск для них
						 *          отработал бы впустую, а стоит он хеширования значения
						 *
						 * @param hashName хеш названия искомого заголовка
						 * @param name     название искомого заголовка
						 * @return         индекс совпадения по названию либо 0
						 *
						 * \~english
						 * @brief Method of the search of a record by the name of a header alone
						 * @details It is needed there where the value is knowingly not indexed -
						 *          for the sensitive headers. A full search for them
						 *          would work in vain, while it costs a hashing of the value
						 * @param hashName hash of the name of the sought header
						 * @param name     name of the sought header
						 * @return         index of a coincidence by the name or 0
						 *
						 * \~
						 */
						uint64_t findName(const size_t hashName, string_view name) const noexcept;
						/**
						 * \~russian
						 * @brief Метод добавления записи в начало таблицы (с вытеснением старых при нехватке места)
						 *
						 * @param name  название заголовка
						 * @param value значение заголовка
						 *
						 * \~english
						 * @brief Method of adding a record into the beginning of the table (with an eviction of the old ones at a lack of the place)
						 * @param name  name of the header
						 * @param value value of the header
						 *
						 * \~
						 */
						void add(string_view name, string_view value) noexcept;
						/**
						 * \~russian
						 * @brief Метод освобождения вытесненных записей
						 *
						 * @note Вызывается в начале операции: представления в строки вытесненных
						 *       записей, выданные предыдущей операцией, после него недействительны
						 *
						 * \~english
						 * @brief Method of the release of the evicted records
						 * @note It is called at the beginning of an operation: the representations into the strings of the evicted
						 *       records issued by the previous operation are invalid after it
						 *
						 * \~
						 */
						void release() noexcept;
					public:
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * @param maxSize  максимальный размер таблицы
						 * @param indexing сопровождать индекс записей для поиска по названию
						 *
						 * \~english
						 * @brief Constructor
						 * @param maxSize  largest size of the table
						 * @param indexing to maintain the index of the records for the search by the name
						 *
						 * \~
						 */
						explicit DynamicTable(const uint32_t maxSize = proto::DEFAULT_HEADER_TABLE_SIZE, const bool indexing = false) noexcept;
						/**
						 * \~russian
						 * @brief Деструктор
						 *
						 *
						 * \~english
						 * @brief Destructor
						 *
						 * \~
						 */
						~DynamicTable() noexcept = default;
				} dynamic_table_t;

				/**
				 * \~russian
				 * @brief Класс декодера HPACK
				 *
				 * @details Хранит динамическую таблицу пира
				 *
				 * \~english
				 * @brief Class of the decoder of HPACK
				 * @details Stores the dynamic table of the peer
				 *
				 * \~
				 */
				typedef class __AWH_SHARED_EXPORT__ Decoder {
					private:
						/**
						 * \~russian
						 * @brief Структура среза декодированного заголовка
						 *
						 * @details Представления ведут либо в арену, либо прямо в строки
						 *          статической и динамической таблиц - копировать записи таблиц
						 *          незачем. Арена под блок отводится один раз по оценке сверху,
						 *          поэтому за время разбора не перевыделяется, а вытеснение
						 *          записей динамической таблицы отложено до конца разбора
						 *
						 * \~english
						 * @brief Structure of a slice of a decoded header
						 * @details The representations lead either into the arena or right into the strings
						 *          of the static and of the dynamic tables - there is no point in copying the records of the tables.
						 *          The arena for a block is allotted once by an estimation from above,
						 *          therefore for the time of the parsing it is not reallocated, while the eviction
						 *          of the records of the dynamic table is postponed to the end of the parsing
						 *
						 * \~
						 */
						typedef struct Slice {
							// Название заголовка
							string_view name;
							// Значение заголовка
							string_view value;
							// Признак чувствительного значения (Literal Never Indexed)
							bool sensitive;
						} slice_t;
					private:
						// Динамическая таблица пира
						dynamic_table_t _table;
					private:
						/**
						 * \~russian
						 * Арена декодированных строк текущего блока
						 *
						 * @details Размер буфера равен его ёмкости, а занятая часть отслеживается
						 *          отдельным счётчиком: дописывание методами строки обходится
						 *          вызовом через границу динамической библиотеки на каждый
						 *          заголовок, тогда как заголовки короткие и вызов стоит дороже
						 *          самого копирования. Ёмкость переиспользуется между блоками
						 *
						 * \~english
						 * Arena of the decoded strings of the current block
						 * @details The size of the buffer is equal to its capacity, while the occupied part is tracked
						 *          by a separate counter: an appending by the methods of a string costs
						 *          a call across the boundary of a dynamic library per
						 *          header, whereas the headers are short and a call costs more
						 *          than the copying itself. The capacity is reused between the blocks
						 *
						 * \~
						 */
						string _arena;
						// Занятая часть арены декодированных строк
						size_t _arenaLength;
						// Срезы декодированных заголовков текущего блока (ёмкость переиспользуется)
						vector <slice_t> _slices;
					private:
						// Максимум размера таблицы, разрешённый нашим SETTINGS_HEADER_TABLE_SIZE
						uint32_t _protocolMaxSize;
					private:
						// Последний блок превысил лимит списка заголовков (декодирован, но не отдан)
						bool _overflow;
					public:
						/**
						 * \~russian
						 * @brief Метод получения динамической таблицы пира
						 *
						 * @return динамическая таблица пира
						 *
						 * \~english
						 * @brief Method of getting the dynamic table of the peer
						 * @return dynamic table of the peer
						 *
						 * \~
						 */
						dynamic_table_t & table() noexcept;
						/**
						 * \~russian
						 * @brief Метод установки верхней границы для Dynamic Table Size Update (RFC 7541 §6.3)
						 *
						 * @note Должна равняться объявленному нами SETTINGS_HEADER_TABLE_SIZE.
						 *       Превышение пиром трактуется как COMPRESSION_ERROR.
						 *
						 * @param size верхняя граница размера таблицы
						 *
						 * \~english
						 * @brief Method of setting the upper boundary for a Dynamic Table Size Update (RFC 7541 §6.3)
						 * @note It is obliged to be equal to the SETTINGS_HEADER_TABLE_SIZE announced by us.
						 *       An exceeding by the peer is treated as a COMPRESSION_ERROR.
						 * @param size upper boundary of the size of the table
						 *
						 * \~
						 */
						void setProtocolMaxSize(const uint32_t size) noexcept;
					public:
						/**
						 * \~russian
						 * @brief Метод декодирования одного блока заголовков целиком
						 *
						 * @details Названия и значения декодируются во внутреннюю арену декодера,
						 *          а в output попадают ссылки на неё: аллокаций в установившемся
						 *          режиме нет. Прежнее содержимое output замещается.
						 *
						 * @note Полученные представления действительны до следующего вызова
						 *       decode() на этом же декодере - копируйте то, что нужно дольше
						 *
						 * @param block       блок заголовков (уже собранный из HEADERS + CONTINUATION)
						 * @param output      декодированные заголовки (ссылки в арену декодера)
						 * @param maxListSize лимит суммарного размера списка (защита от decompression bomb); 0 - без лимита
						 * @param error       код ошибки протокола (COMPRESSION_ERROR / ENHANCE_YOUR_CALM)
						 * @return            результат декодирования (OK/ERROR)
						 *
						 * \~english
						 * @brief Method of the decoding of a single block of the headers entirely
						 * @details The names and the values are decoded into the internal arena of the decoder,
						 *          while into output the references to it get: there are no allocations in the settled
						 *          mode. The previous content of output is replaced.
						 * @note The obtained representations are valid until the next call of
						 *       decode() on this same decoder - copy that which is needed longer
						 * @param block       block of the headers (already assembled from HEADERS + CONTINUATION)
						 * @param output      decoded headers (the references into the arena of the decoder)
						 * @param maxListSize limit of the total size of the list (a protection from a decompression bomb); 0 - without a limit
						 * @param error       error code of the protocol (COMPRESSION_ERROR / ENHANCE_YOUR_CALM)
						 * @return            result of the decoding (OK/ERROR)
						 *
						 * \~
						 */
						status_t decode(string_view block, vector <field_view_t> & output, const uint64_t maxListSize, error_t & error) noexcept;
						/**
						 * \~russian
						 * @brief Метод проверки превышения лимита списка заголовков последним блоком
						 *
						 * @details Блок сверх лимита декодируется целиком - иначе динамическая таблица
						 *          рассинхронизируется с кодером пира и соединение придётся рвать, -
						 *          но заголовки наружу не отдаются. Вызывающий вправе отвергнуть
						 *          один поток, оставив соединение живым
						 *
						 * @return признак превышения лимита последним декодированным блоком
						 *
						 * \~english
						 * @brief Method of checking the exceeding of the limit of the list of the headers by the last block
						 * @details A block above the limit is decoded entirely - otherwise the dynamic table
						 *          would become desynchronized with the encoder of the peer and the connection would have to be broken, -
						 *          but the headers are not issued outside. The caller is entitled to reject
						 *          one stream, leaving the connection alive
						 * @return flag of the exceeding of the limit by the last decoded block
						 *
						 * \~
						 */
						bool overflowed() const noexcept;
					public:
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * @param maxTableSize максимальный размер динамической таблицы
						 *
						 * \~english
						 * @brief Constructor
						 * @param maxTableSize largest size of the dynamic table
						 *
						 * \~
						 */
						explicit Decoder(const uint32_t maxTableSize = proto::DEFAULT_HEADER_TABLE_SIZE) noexcept;
						/**
						 * \~russian
						 * @brief Деструктор
						 *
						 *
						 * \~english
						 * @brief Destructor
						 *
						 * \~
						 */
						~Decoder() noexcept = default;
				} decoder_t;

				/**
				 * \~russian
				 * @brief Класс кодера HPACK
				 *
				 * @details Хранит собственную динамическую таблицу. Индексация: полное совпадение
				 *          (имя+значение) в статической/динамической таблице кодируется как
				 *          Indexed Header Field (RFC 7541 §6.1); при совпадении только имени -
				 *          Literal с инкрементальной индексацией и ссылкой на имя (RFC 7541 §6.2.1);
				 *          иначе - новое имя + значение с добавлением в динамическую таблицу.
				 *          Декодер пира выполняет те же добавления, благодаря чему индексы
				 *          остаются синхронными.
				 *
				 * \~english
				 * @brief Class of the encoder of HPACK
				 * @details Stores its own dynamic table. The indexing: a full coincidence
				 *          (a name+a value) in the static/dynamic table is encoded as
				 *          an Indexed Header Field (RFC 7541 §6.1); at a coincidence by the name alone -
				 *          a Literal with an incremental indexing and a reference to the name (RFC 7541 §6.2.1);
				 *          otherwise - a new name + a value with an addition into the dynamic table.
				 *          The decoder of the peer performs the same additions, thanks to which the indices
				 *          remain synchronous.
				 *
				 * \~
				 */
				typedef class __AWH_SHARED_EXPORT__ Encoder {
					private:
						// Собственная динамическая таблица
						dynamic_table_t _table;
					private:
						// Значение размера таблицы для отправляемого update
						uint32_t _pendingSize;
						// Наименьший размер таблицы за серию изменений между блоками (RFC 7541 §4.2)
						uint32_t _pendingMinSize;
					private:
						// Размер закодированного списка заголовков текущего блока до сжатия (RFC 9113 §6.5.2)
						uint64_t _listSize;
					private:
						/**
						 * \~russian
						 * Кольцо хешей пар название-значение недавно встреченных заголовков
						 *
						 * @details Динамическая таблица невелика, и место в ней стоит отдавать
						 *          заголовкам, которые повторяются. Заголовок с разовым значением
						 *          (:path, etag, date) вытесняет из таблицы как раз повторяющиеся
						 *          user-agent, accept и referer - на следующем блоке их приходится
						 *          передавать литералами заново, и таблица входит в постоянное
						 *          вытеснение сама себя. Поэтому в таблицу заносится только то,
						 *          что уже встречалось в пределах кольца.
						 *          Последняя ячейка отведена под ограничитель перебора и в
						 *          ёмкость кольца не входит. Пустое кольцо означает, что
						 *          адаптивная индексация выключена
						 *
						 * \~english
						 * Ring of the hashes of the pairs a name-a value of the recently met headers
						 * @details The dynamic table is not big, and the place in it is worth giving away
						 *          to the headers which repeat. A header with a one-time value
						 *          (:path, etag, date) evicts from the table exactly the repeating
						 *          user-agent, accept and referer - on the next block they have to be
						 *          transmitted by the literals anew, and the table enters a permanent
						 *          eviction of itself. Therefore into the table only that is entered
						 *          which has already been met within the limits of the ring.
						 *          The last cell is allotted for the limiter of the enumeration and does not enter
						 *          into the capacity of the ring. An empty ring means that
						 *          the adaptive indexing is disabled
						 *
						 * \~
						 */
						vector <uint32_t> _history;
						// Позиция записи в кольце хешей
						size_t _historyIndex;
						// Признак заполненности кольца хешей целиком
						bool _historyWrapped;
					private:
						// Требуется отправить Dynamic Table Size Update в начале следующего блока
						bool _sizeUpdatePending;
						// Автоматически считать чувствительными authorization/cookie и им подобные
						bool _sensitiveHeuristic;
					private:
						/**
						 * \~russian
						 * @brief Метод поиска заголовка в статической + динамической таблицах
						 *
						 * @details Возвращает индекс полного совпадения (имя+значение) или, если его нет,
						 *          заполняет индекс совпадения только по имени. 0 - совпадения нет.
						 *
						 * @param name      название искомого заголовка
						 * @param value     значение искомого заголовка
						 * @param nameIndex индекс совпадения только по имени
						 * @return          индекс полного совпадения (имя+значение)
						 *
						 * \~english
						 * @brief Method of the search of a header in the static + the dynamic tables
						 * @details Returns the index of a full coincidence (a name+a value) or, if there is none,
						 *          fills in the index of a coincidence by the name alone. A 0 - there is no coincidence.
						 * @param name      name of the sought header
						 * @param value     value of the sought header
						 * @param nameIndex index of a coincidence by the name alone
						 * @return          index of a full coincidence (a name+a value)
						 *
						 * \~
						 */
						uint64_t lookup(string_view name, string_view value, uint64_t & nameIndex) const noexcept;
						/**
						 * \~russian
						 * @brief Метод поиска заголовка только по названию
						 *
						 * @details Для чувствительного заголовка значение в таблицы не попадает
						 *          и полное совпадение искать незачем: нужен только индекс
						 *          названия. Полный поиск обошёлся бы хешированием значения,
						 *          а у cookie и authorization значения самые длинные в блоке
						 *
						 * @param name название искомого заголовка
						 * @return     объединённый индекс совпадения по названию либо 0
						 *
						 * \~english
						 * @brief Method of the search of a header by the name alone
						 * @details For a sensitive header the value does not get into the tables
						 *          and there is no point in searching for a full coincidence: only the index
						 *          of the name is needed. A full search would cost a hashing of the value,
						 *          while at a cookie and an authorization the values are the longest in the block
						 * @param name name of the sought header
						 * @return     united index of a coincidence by the name or 0
						 *
						 * \~
						 */
						uint64_t lookupName(string_view name) const noexcept;
						/**
						 * \~russian
						 * @brief Метод принятия решения об индексации заголовка
						 *
						 * @details Заголовок заносится в динамическую таблицу, только если он уже
						 *          встречался в пределах кольца истории. Пока кольцо не заполнено,
						 *          индексируется всё: на старте соединения истории ещё нет.
						 *          Метод изменяет состояние кольца, поэтому вызывается ровно
						 *          один раз на кодируемый заголовок
						 *
						 * @note Решение обязано совпасть с выбранным представлением: декодер пира
						 *       добавляет запись в таблицу именно по нему, и расхождение
						 *       рассинхронизировало бы таблицы
						 *
						 * @param name  название заголовка
						 * @param value значение заголовка
						 * @return      признак необходимости занести заголовок в таблицу
						 *
						 * \~english
						 * @brief Method of the taking of the decision about the indexing of a header
						 * @details A header is entered into the dynamic table only if it has already
						 *          been met within the limits of the ring of the history. Until the ring is filled,
						 *          everything is indexed: at the start of a connection there is no history yet.
						 *          The method changes the state of the ring, therefore it is called exactly
						 *          once per encoded header
						 * @note The decision is obliged to coincide with the chosen representation: the decoder of the peer
						 *       adds a record into the table exactly by it, and a divergence
						 *       would desynchronize the tables
						 * @param name  name of the header
						 * @param value value of the header
						 * @return      flag of the necessity to enter the header into the table
						 *
						 * \~
						 */
						bool indexable(const string_view name, const string_view value) noexcept;
					public:
						/**
						 * \~russian
						 * @brief Метод начала кодирования блока заголовков
						 *
						 * @details Дописывает отложенный Dynamic Table Size Update (RFC 7541 §4.2),
						 *          который обязан идти в самом начале блока. Вызывается один раз
						 *          перед пофиледным кодированием блока.
						 *
						 * @param output выходной буфер блока заголовков
						 *
						 * \~english
						 * @brief Method of the beginning of the encoding of a block of the headers
						 * @details Appends the postponed Dynamic Table Size Update (RFC 7541 §4.2)
						 *          which is obliged to go at the very beginning of the block. It is called once
						 *          before the field-by-field encoding of the block.
						 * @param output output buffer of the block of the headers
						 *
						 * \~
						 */
						void begin(string & output) noexcept;
						/**
						 * \~russian
						 * @brief Метод получения размера закодированного списка заголовков до сжатия
						 *
						 * @details Считается по правилу RFC 9113 §6.5.2 (сумма длин имён и значений плюс
						 *          32 байта на заголовок) и сбрасывается методом begin() в начале блока.
						 *          Нужен для сверки с анонсированным пиром SETTINGS_MAX_HEADER_LIST_SIZE
						 *
						 * @return размер списка заголовков текущего блока до сжатия
						 *
						 * \~english
						 * @brief Method of getting the size of the encoded list of the headers before the compression
						 * @details It is counted by the rule of RFC 9113 §6.5.2 (the sum of the lengths of the names and of the values plus
						 *          32 octets per header) and is reset by the method begin() at the beginning of a block.
						 *          It is needed for a comparison with the SETTINGS_MAX_HEADER_LIST_SIZE announced by the peer
						 * @return size of the list of the headers of the current block before the compression
						 *
						 * \~
						 */
						uint64_t listSize() const noexcept;
					public:
						/**
						 * \~russian
						 * @brief Метод получения собственной динамической таблицы
						 *
						 * @return собственная динамическая таблица
						 *
						 * \~english
						 * @brief Method of getting one's own dynamic table
						 * @return one's own dynamic table
						 *
						 * \~
						 */
						dynamic_table_t & table() noexcept;
						/**
						 * \~russian
						 * @brief Метод изменения максимального размера своей динамической таблицы (RFC 7541 §4.2)
						 *
						 * @note Вызывается при получении SETTINGS_HEADER_TABLE_SIZE пира: наш кодер обязан
						 *       не превышать таблицу, которую готов держать декодер пира. Применяется сразу
						 *       (с вытеснением), а в начало следующего блока ставится Dynamic Table Size Update,
						 *       чтобы декодер пира остался синхронным.
						 *
						 * @param size новый максимальный размер таблицы
						 *
						 * \~english
						 * @brief Method of changing the largest size of one's own dynamic table (RFC 7541 §4.2)
						 * @note It is called at the receipt of a SETTINGS_HEADER_TABLE_SIZE of the peer: our encoder is obliged
						 *       not to exceed the table which the decoder of the peer is ready to hold. It is applied at once
						 *       (with an eviction), while into the beginning of the next block a Dynamic Table Size Update is put,
						 *       so that the decoder of the peer would remain synchronous.
						 * @param size new largest size of the table
						 *
						 * \~
						 */
						void setMaxTableSize(const uint32_t size) noexcept;
						/**
						 * \~russian
						 * @brief Метод управления автоматическим определением чувствительных заголовков
						 *
						 * @details Включено по умолчанию: authorization/proxy-authorization/cookie/set-cookie
						 *          кодируются как Literal Never Indexed и не попадают в динамическую таблицу
						 *          (защита от CRIME-подобных атак). Выключение заметно улучшает сжатие
						 *          cookie-тяжёлого трафика, но снимает эту защиту; явный флаг sensitive
						 *          у отдельного заголовка продолжает действовать в любом режиме.
						 *
						 * @param mode режим автоматического определения
						 *
						 * \~english
						 * @brief Method of the control of the automatic determination of the sensitive headers
						 * @details It is enabled by default: authorization/proxy-authorization/cookie/set-cookie
						 *          are encoded as a Literal Never Indexed and do not get into the dynamic table
						 *          (a protection from the CRIME-like attacks). A disabling noticeably improves the compression
						 *          of the cookie-heavy traffic but removes this protection; an explicit flag sensitive
						 *          at a separate header continues to be in force in any mode.
						 * @param mode mode of the automatic determination
						 *
						 * \~
						 */
						void sensitiveHeuristic(const bool mode) noexcept;
						/**
						 * \~russian
						 * @brief Метод управления адаптивной индексацией заголовков
						 *
						 * @details Включено по умолчанию: в динамическую таблицу заносятся только
						 *          заголовки, уже встречавшиеся в пределах кольца истории. Это
						 *          бережёт место в таблице от заголовков с разовым значением
						 *          (:path, etag, date), которые иначе вытесняют повторяющиеся
						 *          и заставляют передавать их литералами заново.
						 *          Выключение возвращает индексацию всего подряд: осмысленно,
						 *          когда набор заголовков заведомо мал и повторяется целиком,
						 *          либо когда динамическая таблица заведомо велика
						 *
						 * @note Режим влияет только на выбор представления при кодировании
						 *       и совместимости не затрагивает: декодер пира следует
						 *       за представлением, каким бы оно ни было
						 *
						 * @param mode режим адаптивной индексации
						 *
						 * \~english
						 * @brief Method of the control of the adaptive indexing of the headers
						 * @details It is enabled by default: into the dynamic table only the
						 *          headers already met within the limits of the ring of the history are entered. This
						 *          saves the place in the table from the headers with a one-time value
						 *          (:path, etag, date) which otherwise evict the repeating ones
						 *          and force to transmit them by the literals anew.
						 *          A disabling returns the indexing of everything in a row: it is meaningful
						 *          when the collection of the headers is knowingly small and repeats entirely,
						 *          or when the dynamic table is knowingly big
						 * @note The mode influences only the choice of the representation at the encoding
						 *       and does not affect the compatibility: the decoder of the peer follows
						 *       the representation, whatever it would be
						 * @param mode mode of the adaptive indexing
						 *
						 * \~
						 */
						void adaptiveIndexing(const bool mode) noexcept;
					public:
						/**
						 * \~russian
						 * @brief Метод кодирования списка заголовков
						 *
						 * @param fields     заголовки (псевдо-заголовки :method/:path/... должны идти первыми)
						 * @param output     выходной буфер блока заголовков
						 * @param useHuffman применять Huffman-кодирование к строкам
						 *
						 * \~english
						 * @brief Method of the encoding of a list of the headers
						 * @param fields     headers (the pseudo headers :method/:path/... are obliged to go first)
						 * @param output     output buffer of the block of the headers
						 * @param useHuffman to apply the Huffman encoding to the strings
						 *
						 * \~
						 */
						void encode(const vector <field_t> & fields, string & output, const bool useHuffman = true) noexcept;
						/**
						 * \~russian
						 * @brief Метод кодирования списка декодированных заголовков (перекодирование)
						 *
						 * @details Позволяет переслать разобранный блок без промежуточных копий строк
						 *          (прокси-сценарий). Признак sensitive сохраняется: заголовок,
						 *          пришедший как Literal Never Indexed, таким же и уходит (RFC 7541 §7.1.3).
						 *
						 * @param fields     декодированные заголовки
						 * @param output     выходной буфер блока заголовков
						 * @param useHuffman применять Huffman-кодирование к строкам
						 *
						 * \~english
						 * @brief Method of the encoding of a list of the decoded headers (a re-encoding)
						 * @details It allows to forward a parsed block without the intermediate copies of the strings
						 *          (a proxy scenario). The flag sensitive is preserved: a header
						 *          which has come as a Literal Never Indexed goes away as the same one (RFC 7541 §7.1.3).
						 * @param fields     decoded headers
						 * @param output     output buffer of the block of the headers
						 * @param useHuffman to apply the Huffman encoding to the strings
						 *
						 * \~
						 */
						void encode(const vector <field_view_t> & fields, string & output, const bool useHuffman = true) noexcept;
						/**
						 * \~russian
						 * @brief Метод кодирования одного заголовка (zero-copy, без владения строками)
						 *
						 * @note Псевдо-заголовки :method/:path/... должны кодироваться первыми,
						 *       названия заголовков - строго в нижнем регистре (RFC 9113 §8.2.1).
						 *
						 * @param name       название заголовка
						 * @param value      значение заголовка
						 * @param output     выходной буфер блока заголовков
						 * @param sensitive  чувствительное значение (Literal Never Indexed, RFC 7541 §7.1.3)
						 * @param useHuffman применять Huffman-кодирование к строкам
						 *
						 * \~english
						 * @brief Method of the encoding of a single header (zero-copy, without an ownership of the strings)
						 * @note The pseudo headers :method/:path/... are obliged to be encoded first,
						 *       the names of the headers - strictly in the lower case (RFC 9113 §8.2.1).
						 * @param name       name of the header
						 * @param value      value of the header
						 * @param output     output buffer of the block of the headers
						 * @param sensitive  sensitive value (a Literal Never Indexed, RFC 7541 §7.1.3)
						 * @param useHuffman to apply the Huffman encoding to the strings
						 *
						 * \~
						 */
						void encode(string_view name, string_view value, string & output, const bool sensitive = false, const bool useHuffman = true) noexcept;
					public:
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * @param maxTableSize максимальный размер динамической таблицы
						 *
						 * \~english
						 * @brief Constructor
						 * @param maxTableSize largest size of the dynamic table
						 *
						 * \~
						 */
						explicit Encoder(const uint32_t maxTableSize = proto::DEFAULT_HEADER_TABLE_SIZE) noexcept;
						/**
						 * \~russian
						 * @brief Деструктор
						 *
						 *
						 * \~english
						 * @brief Destructor
						 *
						 * \~
						 */
						~Encoder() noexcept = default;
				} encoder_t;
			};
		};
	};
};

#endif // __AWH_HTTP_PARSER_HTTP2_HPACK__
