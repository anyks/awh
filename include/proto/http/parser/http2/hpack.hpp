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
 * @brief Заголовочный файл кодека HPACK (RFC 7541) — статическая и динамическая таблицы заголовков,
 *        кодирование и декодирование целых с префиксом, Хаффман-кодирование,
 *        а также кодер и декодер блоков заголовков с защитой от decompression bomb
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_HTTP_PARSER_HTTP2_HPACK__
#define __AWH_HTTP_PARSER_HTTP2_HPACK__

/**
 * Стандартные заголовочные файлы
 */
#include <deque>
#include <string>
#include <vector>
#include <cstdint>
#include <string_view>
#include <unordered_map>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "h2.hpp"
#include "../../../../sys/global.hpp"

/**
 * @brief основное пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Пространство имён HTTP-протокола
	 *
	 */
	namespace http {
		/**
		 * @brief Пространство имён внутренних слоёв протокола HTTP/2
		 *
		 */
		namespace h2 {
			/**
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
			 */
			namespace hpack {
				/**
				 * @brief Количество записей в статической таблице (RFC 7541 Appendix A)
				 *
				 */
				static constexpr size_t STATIC_TABLE_SIZE = 61;

				/**
				 * @brief Структура записи статической таблицы (zero-copy: ссылки на статические литералы)
				 *
				 */
				typedef struct Static_Entry {
					// Название заголовка
					string_view name;
					// Значение заголовка
					string_view value;
				} static_entry_t;

				/**
				 * @brief Функция получения записи статической таблицы по индексу 1..61 (RFC 7541 Appendix A)
				 *
				 * @param index индекс записи (1-based); 0 или > 61 - невалиден
				 * @return      указатель на запись либо nullptr
				 *
				 */
				__AWH_SHARED_EXPORT__ const static_entry_t * staticTable(const size_t index) noexcept;

				/**
				 * @brief Структура пары декодированного заголовка (невладеющая)
				 *
				 * @details Название и значение указывают во внутреннюю арену декодера и
				 *          действительны до следующего вызова decode() на том же декодере
				 *          (а также до его reset/уничтожения). Если заголовок нужен дольше -
				 *          копируйте его значение.
				 *
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
						 * @brief Конструктор
						 *
						 */
						explicit Field_View() noexcept;
				} field_view_t;

				/**
				 * @brief Класс пары заголовка
				 *
				 * @details Название и значение - владеющие копии: используется на стороне
				 *          кодирования, где данные принадлежат вызывающему коду.
				 *
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
						 * @brief Конструктор
						 *
						 */
						explicit Field() noexcept;
						/**
						 * @brief Конструктор
						 *
						 * @param name  название заголовка
						 * @param value значение заголовка
						 *
						 */
						explicit Field(string name, string value) noexcept;
						/**
						 * @brief Конструктор
						 *
						 * @param name      название заголовка
						 * @param value     значение заголовка
						 * @param sensitive флаг чувствительного значения
						 *
						 */
						explicit Field(string name, string value, const bool sensitive) noexcept;
				} field_t;

				/**
				 * @brief Пространство имён функций Хаффман-кодирования (RFC 7541 Appendix B)
				 *
				 */
				namespace huffman {
					/**
					 * @brief Функция вычисления длины строки в байтах после Huffman-кодирования
					 *
					 * @note Используется для выбора способа кодирования (литерал/Huffman)
					 *
					 * @param input строка для вычисления
					 * @return      длина строки после кодирования
					 *
					 */
					__AWH_SHARED_EXPORT__ size_t length(string_view input) noexcept;
					/**
					 * @brief Функция кодирования строки Huffman'ом (RFC 7541 Appendix B)
					 *
					 * @param input  кодируемая строка
					 * @param output выходной буфер закодированной строки
					 *
					 */
					__AWH_SHARED_EXPORT__ void encode(string_view input, string & output) noexcept;
					/**
					 * @brief Функция декодирования Huffman-строки (RFC 7541 Appendix B)
					 *
					 * @note Содержимое выходного буфера замещается, а не дополняется
					 *
					 * @param data   входной буфер
					 * @param size   доступно байт
					 * @param output выходной буфер декодированной строки
					 * @return       результат декодирования (false - некорректная последовательность, COMPRESSION_ERROR)
					 *
					 */
					__AWH_SHARED_EXPORT__ bool decode(const uint8_t * data, const size_t size, string & output) noexcept;
				};

				/**
				 * @brief Пространство имён функций кодирования/декодирования целых с префиксом переменной длины (RFC 7541 §5.1)
				 *
				 */
				namespace prefixed {
					/**
					 * @brief Функция кодирования целого с префиксом переменной длины (RFC 7541 §5.1)
					 *
					 * @note Старшие (8 - prefixBits) бит первого байта берутся из prefixValue
					 *
					 * @param output      выходной буфер
					 * @param value       кодируемое значение
					 * @param prefixBits  размер префикса в битах (1..8)
					 * @param prefixValue значение старших бит первого байта
					 *
					 */
					__AWH_SHARED_EXPORT__ void encode(string & output, const uint64_t value, const uint8_t prefixBits, const uint8_t prefixValue) noexcept;
					/**
					 * @brief Функция декодирования целого с префиксом переменной длины (RFC 7541 §5.1)
					 *
					 * @param data       входной буфер
					 * @param size       доступно байт
					 * @param prefixBits размер префикса в битах (1..8)
					 * @param value      декодированное значение
					 * @param consumed   количество прочитанных байт
					 * @return           результат декодирования (OK / INCOMPLETE - мало данных / ERROR - переполнение)
					 *
					 */
					__AWH_SHARED_EXPORT__ status_t decode(const uint8_t * data, const size_t size, const uint8_t prefixBits, uint64_t & value, size_t & consumed) noexcept;
				};

				/**
				 * @brief Класс динамической таблицы HPACK с вытеснением по размеру FIFO (RFC 7541 §2.3.2)
				 *
				 * @details Размер записи = len(name) + len(value) + 32 (RFC 7541 §4.1)
				 *
				 */
				typedef class __AWH_SHARED_EXPORT__ DynamicTable {
					private:
						// Список записей таблицы ([0] - самая свежая запись)
						deque <field_t> _entries;
					private:
						// Текущий суммарный размер таблицы
						uint32_t _size;
						// Лимит размера таблицы
						uint32_t _maxSize;
					private:
						/**
						 * Общее количество когда-либо добавленных записей
						 *
						 * @details Добавление записи сдвигает индексы всех остальных, поэтому
						 *          в индексе хранится не позиция, а сквозной номер добавления.
						 *          Живые записи всегда занимают непрерывный отрезок номеров,
						 *          и позиция получается вычитанием: index = _inserts - seq + 1
						 *
						 */
						uint64_t _inserts;
						/**
						 * Индекс записей по хешу названия заголовка (хеш -> номер добавления)
						 *
						 * @details Без индекса поиск заголовка в таблице линейный, а выполняется он
						 *          для каждого заголовка каждого блока: на таблице по умолчанию это
						 *          две трети стоимости кодирования. Ключом служит именно хеш, а не
						 *          представление в строку записи: таблица копируется целиком при
						 *          сбросе соединения, и представления указывали бы в строки
						 *          копии-источника. Совпадение хешей проверяется сравнением названий
						 *
						 */
						unordered_multimap <size_t, uint64_t> _index;
					private:
						/**
						 * Признак сопровождения индекса записей
						 *
						 * @details Индекс нужен только кодеру: декодер обращается к таблице
						 *          по готовому номеру и поиском по названию не пользуется,
						 *          а сопровождение индекса стоит выделения памяти на запись
						 *
						 */
						bool _indexing;
					private:
						/**
						 * @brief Метод вытеснения записей с конца, пока размер не уложится в лимит
						 *
						 */
						void evict() noexcept;
					public:
						/**
						 * @brief Метод получения количества записей таблицы
						 *
						 * @return количество записей таблицы
						 *
						 */
						size_t count() const noexcept;
						/**
						 * @brief Метод получения текущего суммарного размера таблицы
						 *
						 * @return текущий суммарный размер таблицы
						 *
						 */
						uint32_t size() const noexcept;
						/**
						 * @brief Метод получения лимита размера таблицы
						 *
						 * @return лимит размера таблицы
						 *
						 */
						uint32_t maxSize() const noexcept;
					public:
						/**
						 * @brief Метод изменения максимального размера таблицы (Dynamic Table Size Update)
						 *
						 * @note Лишние записи вытесняются сразу
						 *
						 * @param maxSize новый максимальный размер таблицы
						 *
						 */
						void setMaxSize(const uint32_t maxSize) noexcept;
						/**
						 * @brief Метод доступа к записи по индексу (1-based внутри динамической части)
						 *
						 * @param index индекс записи
						 * @return      указатель на запись либо nullptr
						 *
						 */
						const field_t * at(const size_t index) const noexcept;
						/**
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
						 */
						uint64_t find(string_view name, string_view value, uint64_t & nameIndex) const noexcept;
						/**
						 * @brief Метод добавления записи в начало таблицы (с вытеснением старых при нехватке места)
						 *
						 * @param name  название заголовка
						 * @param value значение заголовка
						 *
						 */
						void add(string_view name, string_view value) noexcept;
					public:
						/**
						 * @brief Конструктор
						 *
						 * @param maxSize  максимальный размер таблицы
						 * @param indexing сопровождать индекс записей для поиска по названию
						 *
						 */
						explicit DynamicTable(const uint32_t maxSize = proto::DEFAULT_HEADER_TABLE_SIZE, const bool indexing = false) noexcept;
						/**
						 * @brief Деструктор
						 *
						 */
						~DynamicTable() noexcept = default;
				} dynamic_table_t;

				/**
				 * @brief Класс декодера HPACK
				 *
				 * @details Хранит динамическую таблицу пира
				 *
				 */
				typedef class __AWH_SHARED_EXPORT__ Decoder {
					private:
						/**
						 * @brief Структура среза декодированного заголовка в арене
						 *
						 * @details Во время разбора арена дописывается и может быть перевыделена,
						 *          поэтому позиции хранятся смещениями, а string_view собираются
						 *          один раз по завершении разбора всего блока.
						 *
						 */
						typedef struct Slice {
							// Смещение названия заголовка в арене
							size_t nameOffset;
							// Длина названия заголовка
							size_t nameLength;
							// Смещение значения заголовка в арене
							size_t valueOffset;
							// Длина значения заголовка
							size_t valueLength;
							// Признак чувствительного значения (Literal Never Indexed)
							bool sensitive;
						} slice_t;
					private:
						// Динамическая таблица пира
						dynamic_table_t _table;
					private:
						// Арена декодированных строк текущего блока (ёмкость переиспользуется)
						string _arena;
						// Буфер Huffman-декодирования одной строки (ёмкость переиспользуется)
						string _scratch;
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
						 * @brief Метод получения динамической таблицы пира
						 *
						 * @return динамическая таблица пира
						 *
						 */
						dynamic_table_t & table() noexcept;
						/**
						 * @brief Метод установки верхней границы для Dynamic Table Size Update (RFC 7541 §6.3)
						 *
						 * @note Должна равняться объявленному нами SETTINGS_HEADER_TABLE_SIZE.
						 *       Превышение пиром трактуется как COMPRESSION_ERROR.
						 *
						 * @param size верхняя граница размера таблицы
						 *
						 */
						void setProtocolMaxSize(const uint32_t size) noexcept;
					public:
						/**
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
						 */
						status_t decode(string_view block, vector <field_view_t> & output, const uint64_t maxListSize, error_t & error) noexcept;
						/**
						 * @brief Метод проверки превышения лимита списка заголовков последним блоком
						 *
						 * @details Блок сверх лимита декодируется целиком - иначе динамическая таблица
						 *          рассинхронизируется с кодером пира и соединение придётся рвать, -
						 *          но заголовки наружу не отдаются. Вызывающий вправе отвергнуть
						 *          один поток, оставив соединение живым
						 *
						 * @return признак превышения лимита последним декодированным блоком
						 *
						 */
						bool overflowed() const noexcept;
					public:
						/**
						 * @brief Конструктор
						 *
						 * @param maxTableSize максимальный размер динамической таблицы
						 *
						 */
						explicit Decoder(const uint32_t maxTableSize = proto::DEFAULT_HEADER_TABLE_SIZE) noexcept;
						/**
						 * @brief Деструктор
						 *
						 */
						~Decoder() noexcept = default;
				} decoder_t;

				/**
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
						// Требуется отправить Dynamic Table Size Update в начале следующего блока
						bool _sizeUpdatePending;
						// Автоматически считать чувствительными authorization/cookie и им подобные
						bool _sensitiveHeuristic;
					private:
						/**
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
						 */
						uint64_t lookup(string_view name, string_view value, uint64_t & nameIndex) const noexcept;
					public:
						/**
						 * @brief Метод начала кодирования блока заголовков
						 *
						 * @details Дописывает отложенный Dynamic Table Size Update (RFC 7541 §4.2),
						 *          который обязан идти в самом начале блока. Вызывается один раз
						 *          перед пофиледным кодированием блока.
						 *
						 * @param output выходной буфер блока заголовков
						 *
						 */
						void begin(string & output) noexcept;
						/**
						 * @brief Метод получения размера закодированного списка заголовков до сжатия
						 *
						 * @details Считается по правилу RFC 9113 §6.5.2 (сумма длин имён и значений плюс
						 *          32 байта на заголовок) и сбрасывается методом begin() в начале блока.
						 *          Нужен для сверки с анонсированным пиром SETTINGS_MAX_HEADER_LIST_SIZE
						 *
						 * @return размер списка заголовков текущего блока до сжатия
						 *
						 */
						uint64_t listSize() const noexcept;
					public:
						/**
						 * @brief Метод получения собственной динамической таблицы
						 *
						 * @return собственная динамическая таблица
						 *
						 */
						dynamic_table_t & table() noexcept;
						/**
						 * @brief Метод изменения максимального размера своей динамической таблицы (RFC 7541 §4.2)
						 *
						 * @note Вызывается при получении SETTINGS_HEADER_TABLE_SIZE пира: наш кодер обязан
						 *       не превышать таблицу, которую готов держать декодер пира. Применяется сразу
						 *       (с вытеснением), а в начало следующего блока ставится Dynamic Table Size Update,
						 *       чтобы декодер пира остался синхронным.
						 *
						 * @param size новый максимальный размер таблицы
						 *
						 */
						void setMaxTableSize(const uint32_t size) noexcept;
						/**
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
						 */
						void sensitiveHeuristic(const bool mode) noexcept;
					public:
						/**
						 * @brief Метод кодирования списка заголовков
						 *
						 * @param fields     заголовки (псевдо-заголовки :method/:path/... должны идти первыми)
						 * @param output     выходной буфер блока заголовков
						 * @param useHuffman применять Huffman-кодирование к строкам
						 *
						 */
						void encode(const vector <field_t> & fields, string & output, const bool useHuffman = true) noexcept;
						/**
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
						 */
						void encode(const vector <field_view_t> & fields, string & output, const bool useHuffman = true) noexcept;
						/**
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
						 */
						void encode(string_view name, string_view value, string & output, const bool sensitive = false, const bool useHuffman = true) noexcept;
					public:
						/**
						 * @brief Конструктор
						 *
						 * @param maxTableSize максимальный размер динамической таблицы
						 *
						 */
						explicit Encoder(const uint32_t maxTableSize = proto::DEFAULT_HEADER_TABLE_SIZE) noexcept;
						/**
						 * @brief Деструктор
						 *
						 */
						~Encoder() noexcept = default;
				} encoder_t;
			};
		};
	};
};

#endif // __AWH_HTTP_PARSER_HTTP2_HPACK__
