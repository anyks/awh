/**
 * @file hpack.hpp
 * @brief HPACK — сжатие заголовков HTTP/2 (RFC 7541).
 *
 * HPACK — это отдельный самодостаточный кодек. Состоит из:
 *   1. целочисленного кодирования с префиксом переменной длины (RFC 7541 §5.1);
 *   2. строкового кодирования (литерал или Huffman, §5.2);
 *   3. статической таблицы (61 запись, §2.3.1, Appendix A);
 *   4. динамической таблицы с вытеснением по размеру (§2.3.2);
 *   5. Huffman-кодирования по фиксированной таблице (Appendix B).
 *
 * Это главный источник уязвимостей (decompression bomb), поэтому декодер обязан
 * жёстко ограничивать суммарный размер распакованного списка заголовков.
 *
 * Статус: реализованы статическая таблица, целочисленный/строковый кодек, Huffman,
 * динамическая таблица с вытеснением, декодер и кодер с индексацией.
 */

#ifndef AWH_EXPERIENCE_H2_HPACK_HPP
#define AWH_EXPERIENCE_H2_HPACK_HPP

#include "h2.hpp"

#include <deque>
#include <string>
#include <vector>
#include <cstdint>
#include <string_view>

namespace awh {
	namespace http2 {
		namespace hpack {
			/**
			 * @brief Пара заголовка. name/value — владеющие копии (после декодирования
			 *        могут ссылаться на динамическую таблицу, поэтому храним как string).
			 */
			struct field_t {
				std::string name;
				std::string value;
				/// Чувствительное значение (RFC 7541 §7.1.3): кодировать как Literal Never Indexed
				/// и не заносить в динамическую таблицу (защита от CRIME-подобных атак). Кодер
				/// дополнительно автоматически считает чувствительными authorization/cookie.
				bool        sensitive = false;
			};

			/**
			 * @brief Запись статической таблицы (zero-copy: ссылки на статические литералы).
			 */
			struct static_entry_t {
				std::string_view name;
				std::string_view value;
			};

			/**
			 * @brief Получить запись статической таблицы по индексу 1..61 (RFC 7541 Appendix A).
			 *
			 * @param index индекс (1-based); 0 или > 61 — невалиден
			 * @return указатель на запись либо nullptr
			 */
			const static_entry_t * staticTable(size_t index) noexcept;

			/// Число записей в статической таблице.
			static constexpr size_t STATIC_TABLE_SIZE = 61;

			// ───────────────────── Целочисленный кодек (RFC 7541 §5.1) ─────────────────────

			/**
			 * @brief Декодировать целое с префиксом переменной длины.
			 *
			 * @param data      входной буфер
			 * @param size      доступно байт
			 * @param prefixBits размер префикса в битах (1..8)
			 * @param value     [out] декодированное значение
			 * @param consumed  [out] сколько байт прочитано
			 * @return OK / INCOMPLETE (мало данных) / ERROR (переполнение)
			 */
			status_t decodeInteger(const uint8_t * data, size_t size, uint8_t prefixBits, uint64_t & value, size_t & consumed) noexcept;

			/**
			 * @brief Закодировать целое с префиксом переменной длины.
			 *
			 * Старшие (8 - prefixBits) бит первого байта берутся из prefixValue.
			 */
			void encodeInteger(std::string & out, uint64_t value, uint8_t prefixBits, uint8_t prefixValue) noexcept;

			// ───────────────────── Huffman (RFC 7541 Appendix B) ─────────────────────
			// TODO(этап 2): таблицу кодов скопировать из nghttp2_hd_huffman_data.c (MIT).

			/**
			 * @brief Декодировать Huffman-строку в out.
			 * @return false при некорректной последовательности (COMPRESSION_ERROR).
			 */
			bool huffmanDecode(const uint8_t * data, size_t size, std::string & out) noexcept;

			/**
			 * @brief Закодировать строку Huffman'ом в out.
			 */
			void huffmanEncode(std::string_view in, std::string & out) noexcept;

			/**
			 * @brief Длина строки в байтах после Huffman-кодирования (для выбора литерал/Huffman).
			 */
			size_t huffmanLength(std::string_view in) noexcept;

			// ───────────────────── Динамическая таблица (RFC 7541 §2.3.2) ─────────────────────

			/**
			 * @brief Динамическая таблица HPACK с вытеснением по размеру (FIFO).
			 *
			 * Размер записи = len(name) + len(value) + 32 (RFC 7541 §4.1).
			 */
			class DynamicTable {
				public:
					explicit DynamicTable(uint32_t maxSize = proto::DEFAULT_HEADER_TABLE_SIZE) noexcept;

					/// Добавить запись в начало, вытеснив старые при нехватке места.
					void add(std::string_view name, std::string_view value) noexcept;
					/// Доступ по индексу (1-based внутри динамической части).
					const field_t * at(size_t index) const noexcept;
					/// Изменить максимальный размер (Dynamic Table Size Update), вытесняя лишнее.
					void setMaxSize(uint32_t maxSize) noexcept;

					size_t   count() const noexcept { return _entries.size(); }
					uint32_t size() const noexcept { return _size; }
					uint32_t maxSize() const noexcept { return _maxSize; }
				private:
					/// Вытеснять записи с конца, пока размер не уложится в лимит.
					void evict() noexcept;
				private:
					std::deque <field_t> _entries; // [0] — самая свежая запись
					uint32_t _size    = 0;          // текущий суммарный размер
					uint32_t _maxSize = 0;          // лимит размера
			};

			/**
			 * @brief Декодер HPACK. Хранит динамическую таблицу пира.
			 */
			class Decoder {
				public:
					explicit Decoder(uint32_t maxTableSize = proto::DEFAULT_HEADER_TABLE_SIZE) noexcept
					 : _table(maxTableSize), _protocolMaxSize(maxTableSize) {}

					/**
					 * @brief Декодировать один блок заголовков целиком.
					 *
					 * @param block       фрагмент блока (уже собранный из HEADERS+CONTINUATION)
					 * @param out         [out] декодированные заголовки
					 * @param maxListSize лимит суммарного размера (защита от bomb); 0 — без лимита
					 * @param err         [out] код ошибки при ERROR (COMPRESSION_ERROR / ENHANCE_YOUR_CALM)
					 * @return OK / ERROR
					 */
					status_t decode(std::string_view block, std::vector <field_t> & out, uint64_t maxListSize, error_t & err) noexcept;

					DynamicTable & table() noexcept { return _table; }

					/**
					 * @brief Верхняя граница для Dynamic Table Size Update (RFC 7541 §6.3).
					 *        Должна равняться объявленному нами SETTINGS_HEADER_TABLE_SIZE.
					 *        Превышение пиром трактуется как COMPRESSION_ERROR.
					 */
					void setProtocolMaxSize(uint32_t size) noexcept { _protocolMaxSize = size; }
				private:
					DynamicTable _table;
					uint32_t     _protocolMaxSize; // максимум, разрешённый нашим SETTINGS
			};

			/**
			 * @brief Кодер HPACK. Хранит свою динамическую таблицу.
			 *
			 * Индексация: полное совпадение (имя+значение) в статической/динамической
			 * таблице кодируется как Indexed Header Field (§6.1); при совпадении только
			 * имени — Literal с инкрементальной индексацией и ссылкой на имя (§6.2.1);
			 * иначе — новое имя + значение с добавлением в динамическую таблицу. Декодер
			 * пира выполняет те же добавления, благодаря чему индексы остаются синхронными.
			 */
			class Encoder {
				public:
					explicit Encoder(uint32_t maxTableSize = proto::DEFAULT_HEADER_TABLE_SIZE) noexcept
					 : _table(maxTableSize) {}

					/**
					 * @brief Закодировать список заголовков в out.
					 *
					 * @param fields    заголовки (псевдо-заголовки :method/:path/... должны идти первыми)
					 * @param out       выходной буфер блока
					 * @param useHuffman применять Huffman-кодирование к строкам
					 */
					void encode(const std::vector <field_t> & fields, std::string & out, bool useHuffman = true) noexcept;

					DynamicTable & table() noexcept { return _table; }

					/**
					 * @brief Изменить максимальный размер своей динамической таблицы (RFC 7541 §4.2).
					 *
					 * Вызывается при получении SETTINGS_HEADER_TABLE_SIZE пира: наш кодер обязан
					 * не превышать таблицу, которую готов держать декодер пира. Применяется сразу
					 * (с вытеснением), а в начало следующего блока ставится Dynamic Table Size Update,
					 * чтобы декодер пира остался синхронным.
					 */
					void setMaxTableSize(uint32_t size) noexcept;
				private:
					DynamicTable _table;
					bool         _sizeUpdatePending = false; // нужно отправить Dynamic Table Size Update
					uint32_t     _pendingSize       = 0;     // значение для этого update
			};
		}
	}
}

#endif // AWH_EXPERIENCE_H2_HPACK_HPP
