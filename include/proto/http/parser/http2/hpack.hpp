/**
 * @file: hpack.hpp
 * @date: 2026-07-19
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
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
#include <utility>
#include <string_view>

/**
 * Подключаем наши заголовочные файлы
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
			 */
			namespace hpack {
				/**
				 * @brief Количество записей в статической таблице (RFC 7541 Appendix A)
				 *
				 */
				static constexpr size_t STATIC_TABLE_SIZE = 61;

				/**
				 * @brief Структура пары заголовка
				 *
				 * @details Название и значение - владеющие копии: после декодирования они могут
				 *          ссылаться на динамическую таблицу, поэтому хранятся как string
				 */
				typedef struct Field {
					// Название заголовка
					string name;
					// Значение заголовка
					string value;
					// Чувствительное значение (RFC 7541 §7.1.3): кодировать как Literal Never Indexed
					// и не заносить в динамическую таблицу (защита от CRIME-подобных атак). Кодер
					// дополнительно автоматически считает чувствительными authorization/cookie
					bool sensitive;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Field() noexcept :
					 name{""}, value{""}, sensitive(false) {}
					/**
					 * @brief Конструктор
					 *
					 * @param name  название заголовка
					 * @param value значение заголовка
					 */
					explicit Field(string name, string value) noexcept :
					 name(std::move(name)), value(std::move(value)), sensitive(false) {}
					/**
					 * @brief Конструктор
					 *
					 * @param name      название заголовка
					 * @param value     значение заголовка
					 * @param sensitive флаг чувствительного значения
					 */
					explicit Field(string name, string value, const bool sensitive) noexcept :
					 name(std::move(name)), value(std::move(value)), sensitive(sensitive) {}
				} field_t;
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
				 */
				const static_entry_t * staticTable(const size_t index) noexcept;
				/**
				 * @brief Функция декодирования целого с префиксом переменной длины (RFC 7541 §5.1)
				 *
				 * @param data       входной буфер
				 * @param size       доступно байт
				 * @param prefixBits размер префикса в битах (1..8)
				 * @param value      декодированное значение
				 * @param consumed   количество прочитанных байт
				 * @return           результат декодирования (OK / INCOMPLETE - мало данных / ERROR - переполнение)
				 */
				status_t decodeInteger(const uint8_t * data, const size_t size, const uint8_t prefixBits, uint64_t & value, size_t & consumed) noexcept;
				/**
				 * @brief Функция кодирования целого с префиксом переменной длины (RFC 7541 §5.1)
				 *
				 * @note Старшие (8 - prefixBits) бит первого байта берутся из prefixValue
				 *
				 * @param out         выходной буфер
				 * @param value       кодируемое значение
				 * @param prefixBits  размер префикса в битах (1..8)
				 * @param prefixValue значение старших бит первого байта
				 */
				void encodeInteger(string & out, const uint64_t value, const uint8_t prefixBits, const uint8_t prefixValue) noexcept;
				/**
				 * @brief Функция декодирования Huffman-строки (RFC 7541 Appendix B)
				 *
				 * @param data входной буфер
				 * @param size доступно байт
				 * @param out  выходной буфер декодированной строки
				 * @return     результат декодирования (false - некорректная последовательность, COMPRESSION_ERROR)
				 */
				bool huffmanDecode(const uint8_t * data, const size_t size, string & out) noexcept;
				/**
				 * @brief Функция кодирования строки Huffman'ом (RFC 7541 Appendix B)
				 *
				 * @param in  кодируемая строка
				 * @param out выходной буфер закодированной строки
				 */
				void huffmanEncode(string_view in, string & out) noexcept;
				/**
				 * @brief Функция вычисления длины строки в байтах после Huffman-кодирования
				 *
				 * @note Используется для выбора способа кодирования (литерал/Huffman)
				 *
				 * @param in строка для вычисления
				 * @return   длина строки после кодирования
				 */
				size_t huffmanLength(string_view in) noexcept;
				/**
				 * @brief Класс динамической таблицы HPACK с вытеснением по размеру FIFO (RFC 7541 §2.3.2)
				 *
				 * @details Размер записи = len(name) + len(value) + 32 (RFC 7541 §4.1)
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
						 * @brief Метод вытеснения записей с конца, пока размер не уложится в лимит
						 *
						 */
						void evict() noexcept;
					public:
						/**
						 * @brief Метод добавления записи в начало таблицы (с вытеснением старых при нехватке места)
						 *
						 * @param name  название заголовка
						 * @param value значение заголовка
						 */
						void add(string_view name, string_view value) noexcept;
						/**
						 * @brief Метод доступа к записи по индексу (1-based внутри динамической части)
						 *
						 * @param index индекс записи
						 * @return      указатель на запись либо nullptr
						 */
						const field_t * at(const size_t index) const noexcept;
						/**
						 * @brief Метод изменения максимального размера таблицы (Dynamic Table Size Update)
						 *
						 * @note Лишние записи вытесняются сразу
						 *
						 * @param maxSize новый максимальный размер таблицы
						 */
						void setMaxSize(const uint32_t maxSize) noexcept;
					public:
						/**
						 * @brief Метод получения количества записей таблицы
						 *
						 * @return количество записей таблицы
						 */
						size_t count() const noexcept;
						/**
						 * @brief Метод получения текущего суммарного размера таблицы
						 *
						 * @return текущий суммарный размер таблицы
						 */
						uint32_t size() const noexcept;
						/**
						 * @brief Метод получения лимита размера таблицы
						 *
						 * @return лимит размера таблицы
						 */
						uint32_t maxSize() const noexcept;
					public:
						/**
						 * @brief Конструктор
						 *
						 * @param maxSize максимальный размер таблицы
						 */
						explicit DynamicTable(const uint32_t maxSize = proto::DEFAULT_HEADER_TABLE_SIZE) noexcept;
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
				 */
				typedef class __AWH_SHARED_EXPORT__ Decoder {
					private:
						// Динамическая таблица пира
						dynamic_table_t _table;
					private:
						// Максимум размера таблицы, разрешённый нашим SETTINGS_HEADER_TABLE_SIZE
						uint32_t _protocolMaxSize;
					public:
						/**
						 * @brief Метод декодирования одного блока заголовков целиком
						 *
						 * @param block       блок заголовков (уже собранный из HEADERS + CONTINUATION)
						 * @param out         декодированные заголовки
						 * @param maxListSize лимит суммарного размера списка (защита от decompression bomb); 0 - без лимита
						 * @param err         код ошибки протокола (COMPRESSION_ERROR / ENHANCE_YOUR_CALM)
						 * @return            результат декодирования (OK/ERROR)
						 */
						status_t decode(string_view block, vector <field_t> & out, const uint64_t maxListSize, error_t & err) noexcept;
					public:
						/**
						 * @brief Метод получения динамической таблицы пира
						 *
						 * @return динамическая таблица пира
						 */
						dynamic_table_t & table() noexcept;
						/**
						 * @brief Метод установки верхней границы для Dynamic Table Size Update (RFC 7541 §6.3)
						 *
						 * @note Должна равняться объявленному нами SETTINGS_HEADER_TABLE_SIZE.
						 *       Превышение пиром трактуется как COMPRESSION_ERROR
						 *
						 * @param size верхняя граница размера таблицы
						 */
						void setProtocolMaxSize(const uint32_t size) noexcept;
					public:
						/**
						 * @brief Конструктор
						 *
						 * @param maxTableSize максимальный размер динамической таблицы
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
				 */
				typedef class __AWH_SHARED_EXPORT__ Encoder {
					private:
						// Собственная динамическая таблица
						dynamic_table_t _table;
					private:
						// Требуется отправить Dynamic Table Size Update в начале следующего блока
						bool _sizeUpdatePending;
						// Значение размера таблицы для отправляемого update
						uint32_t _pendingSize;
					private:
						/**
						 * @brief Метод поиска заголовка в статической + динамической таблицах
						 *
						 * @details Возвращает индекс полного совпадения (имя+значение) или, если его нет,
						 *          заполняет индекс совпадения только по имени. 0 - совпадения нет
						 *
						 * @param name      название искомого заголовка
						 * @param value     значение искомого заголовка
						 * @param nameIndex индекс совпадения только по имени
						 * @return          индекс полного совпадения (имя+значение)
						 */
						uint64_t lookup(string_view name, string_view value, uint64_t & nameIndex) const noexcept;
					public:
						/**
						 * @brief Метод начала кодирования блока заголовков
						 *
						 * @details Дописывает отложенный Dynamic Table Size Update (RFC 7541 §4.2),
						 *          который обязан идти в самом начале блока. Вызывается один раз
						 *          перед пофиледным кодированием блока
						 *
						 * @param out выходной буфер блока заголовков
						 */
						void begin(string & out) noexcept;
						/**
						 * @brief Метод кодирования одного заголовка (zero-copy, без владения строками)
						 *
						 * @note Псевдо-заголовки :method/:path/... должны кодироваться первыми,
						 *       названия заголовков - строго в нижнем регистре (RFC 9113 §8.2.1)
						 *
						 * @param name       название заголовка
						 * @param value      значение заголовка
						 * @param out        выходной буфер блока заголовков
						 * @param sensitive  чувствительное значение (Literal Never Indexed, RFC 7541 §7.1.3)
						 * @param useHuffman применять Huffman-кодирование к строкам
						 */
						void encode(string_view name, string_view value, string & out, const bool sensitive = false, const bool useHuffman = true) noexcept;
						/**
						 * @brief Метод кодирования списка заголовков
						 *
						 * @param fields     заголовки (псевдо-заголовки :method/:path/... должны идти первыми)
						 * @param out        выходной буфер блока заголовков
						 * @param useHuffman применять Huffman-кодирование к строкам
						 */
						void encode(const vector <field_t> & fields, string & out, const bool useHuffman = true) noexcept;
					public:
						/**
						 * @brief Метод получения собственной динамической таблицы
						 *
						 * @return собственная динамическая таблица
						 */
						dynamic_table_t & table() noexcept;
						/**
						 * @brief Метод изменения максимального размера своей динамической таблицы (RFC 7541 §4.2)
						 *
						 * @note Вызывается при получении SETTINGS_HEADER_TABLE_SIZE пира: наш кодер обязан
						 *       не превышать таблицу, которую готов держать декодер пира. Применяется сразу
						 *       (с вытеснением), а в начало следующего блока ставится Dynamic Table Size Update,
						 *       чтобы декодер пира остался синхронным
						 *
						 * @param size новый максимальный размер таблицы
						 */
						void setMaxTableSize(const uint32_t size) noexcept;
					public:
						/**
						 * @brief Конструктор
						 *
						 * @param maxTableSize максимальный размер динамической таблицы
						 */
						explicit Encoder(const uint32_t maxTableSize = proto::DEFAULT_HEADER_TABLE_SIZE) noexcept;
						/**
						 * @brief Деструктор
						 *
						 */
						~Encoder() noexcept = default;
				} encoder_t;
			}
		}
	};
};

#endif // __AWH_HTTP_PARSER_HTTP2_HPACK__
