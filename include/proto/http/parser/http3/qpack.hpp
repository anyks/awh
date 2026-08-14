/**
 * @file: qpack.hpp
 * @date: 2026-07-27
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл кодека QPACK (RFC 9204) — сжатие полей HTTP/3: статическая таблица,
 *        динамическая таблица с абсолютной индексацией, потоки инструкций кодера и декодера
 *
 * \~english
 * @brief Header file of the codec of QPACK (RFC 9204) — the compression of the fields of HTTP/3: the static table,
 *        the dynamic table with an absolute indexing, the streams of the instructions of the encoder and of the decoder
 *
 * \~
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_HTTP_PARSER_HTTP3_QPACK__
#define __AWH_HTTP_PARSER_HTTP3_QPACK__

/**
 * Стандартные заголовочные файлы
 */
#include <deque>
#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <unordered_map>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "h3.hpp"
#include "../http2/hpack.hpp"
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
		 * @brief Пространство имён внутренних слоёв протокола HTTP/3
		 *
		 * \~english
		 * @brief Namespace of the internal layers of the HTTP/3 protocol
		 *
		 * \~
		 */
		namespace h3 {
			/**
			 * \~russian
			 * @brief Пространство имён QPACK - сжатие полей HTTP/3 (RFC 9204)
			 *
			 * @details QPACK решает ту же задачу, что и HPACK, но в условиях, где потоки
			 *          доставляются независимо друг от друга и могут прийти не в том порядке,
			 *          в котором отправлены. HPACK в таких условиях неприменим: его динамическая
			 *          таблица меняется прямо внутри блока заголовков, поэтому блок обязан
			 *          разбираться строго в порядке отправки, иначе таблицы разъедутся.
			 *
			 *          QPACK разделяет эти два потока управления:
			 *          1. таблица меняется инструкциями в отдельном однонаправленном потоке
			 *             кодера, доставляемом транспортом по порядку (RFC 9204 §4.3);
			 *          2. секция полей ссылается на записи абсолютными номерами и несёт
			 *             в префиксе Required Insert Count - номер вставки, до которой
			 *             таблица обязана быть заполнена, чтобы секция разобралась.
			 *
			 *          Секция, пришедшая раньше нужных вставок, не является ошибкой: поток
			 *          блокируется до их прихода. Число одновременно заблокированных потоков
			 *          ограничено параметром SETTINGS_QPACK_BLOCKED_STREAMS, и превышение
			 *          этой границы кодером - уже ошибка соединения.
			 *
			 *          Целочисленное кодирование с префиксом и таблица Huffman взяты из HPACK
			 *          без изменений (RFC 9204 §4.1.1), поэтому переиспользуются напрямую
			 *          из hpack.hpp: расхождение реализаций здесь было бы источником
			 *          несовместимости на ровном месте.
			 *
			 * @note Индексы статической таблицы QPACK нумеруются с нуля, а не с единицы,
			 *       как в HPACK: нулевой индекс здесь не служит признаком отсутствия записи
			 *
			 * \~english
			 * @brief Namespace of QPACK - the compression of the fields of HTTP/3 (RFC 9204)
			 * @details QPACK solves the same task as HPACK, but in the conditions where the streams
			 *          are delivered independently of each other and may come not in that order
			 *          in which they have been sent. HPACK is inapplicable in such conditions: its dynamic
			 *          table changes right inside a block of the headers, therefore the block is obliged
			 *          to be parsed strictly in the order of the sending, otherwise the tables would diverge.
			 *          QPACK separates these two flows of the control:
			 *          1. the table changes by the instructions in a separate unidirectional stream
			 *             of the encoder delivered by the transport in the order (RFC 9204 §4.3);
			 *          2. a section of the fields refers to the records by the absolute numbers and carries
			 *             in the prefix the Required Insert Count - the number of the insertion up to which
			 *             the table is obliged to be filled for the section to be parsed.
			 *          A section which has come earlier than the needed insertions is not an error: the stream
			 *          is blocked until their arrival. The number of the simultaneously blocked streams
			 *          is limited by the parameter SETTINGS_QPACK_BLOCKED_STREAMS, and an exceeding
			 *          of this boundary by the encoder is already an error of the connection.
			 *          The integer encoding with a prefix and the table of Huffman are taken from HPACK
			 *          without changes (RFC 9204 §4.1.1), therefore they are reused directly
			 *          from hpack.hpp: a divergence of the implementations here would be a source
			 *          of an incompatibility out of the blue.
			 * @note The indices of the static table of QPACK are numbered from a zero rather than from a unit,
			 *       as in HPACK: a zero index here does not serve as a flag of an absence of a record
			 *
			 * \~
			 */
			namespace qpack {
				/**
				 * \~russian
				 * @brief Количество записей в статической таблице (RFC 9204 Appendix A)
				 *
				 * \~english
				 * @brief Number of the records in the static table (RFC 9204 Appendix A)
				 *
				 * \~
				 */
				static constexpr size_t STATIC_TABLE_SIZE = 99;
				/**
				 * \~russian
				 * @brief Накладные расходы на запись динамической таблицы в октетах (RFC 9204 §3.2.1)
				 *
				 * \~english
				 * @brief Overhead per record of the dynamic table in octets (RFC 9204 §3.2.1)
				 *
				 * \~
				 */
				static constexpr uint64_t ENTRY_OVERHEAD = 32;

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
					// Название поля
					string_view name;
					// Значение поля
					string_view value;
				} static_entry_t;

				/**
				 * \~russian
				 * @brief Функция получения записи статической таблицы по индексу 0..98 (RFC 9204 Appendix A)
				 *
				 * @param index индекс записи (0-based); >= 99 - невалиден
				 * @return      указатель на запись либо nullptr
				 *
				 * \~english
				 * @brief Function of getting a record of the static table by an index 0..98 (RFC 9204 Appendix A)
				 * @param index index of the record (0-based); >= 99 is invalid
				 * @return      pointer to the record or nullptr
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ const static_entry_t * staticTable(const size_t index) noexcept;

				/**
				 * \~russian
				 * @brief Тип пары декодированного поля (невладеющая)
				 *
				 * @details Совпадает с типом HPACK намеренно: шлюз, транслирующий сообщение
				 *          между HTTP/2 и HTTP/3, передаёт разобранный список полей из одного
				 *          кодека в другой без промежуточного преобразования
				 *
				 * \~english
				 * @brief Type of a pair of a decoded field (non-owning)
				 * @details It coincides with the type of HPACK deliberately: a gateway translating a message
				 *          between HTTP/2 and HTTP/3 passes a parsed list of the fields from one
				 *          codec into the other without an intermediate conversion
				 *
				 * \~
				 */
				using field_view_t = h2::hpack::field_view_t;
				/**
				 * \~russian
				 * @brief Тип пары поля (владеющая копия)
				 *
				 * \~english
				 * @brief Type of a pair of a field (an owning copy)
				 *
				 * \~
				 */
				using field_t = h2::hpack::field_t;

				/**
				 * \~russian
				 * @brief Класс динамической таблицы QPACK с абсолютной индексацией (RFC 9204 §3.2)
				 *
				 * @details Размер записи = len(name) + len(value) + 32 (RFC 9204 §3.2.1), как в HPACK.
				 *          Отличие от HPACK - в нумерации: запись получает сквозной абсолютный номер
				 *          в момент вставки и сохраняет его до вытеснения. Номера живых записей
				 *          всегда образуют непрерывный отрезок, поэтому позиция записи в очереди
				 *          получается вычитанием, а сама очередь остаётся дешёвой
				 *
				 * \~english
				 * @brief Class of the dynamic table of QPACK with an absolute indexing (RFC 9204 §3.2)
				 * @details The size of a record = len(name) + len(value) + 32 (RFC 9204 §3.2.1), as in HPACK.
				 *          The difference from HPACK is in the numbering: a record gets a through absolute number
				 *          at the moment of the insertion and preserves it until the eviction. The numbers of the living records
				 *          always form a continuous segment, therefore the position of a record in the queue
				 *          is obtained by a subtraction, while the queue itself remains cheap
				 *
				 * \~
				 */
				typedef class __AWH_SHARED_EXPORT__ DynamicTable {
					private:
						// Список записей таблицы ([_retained] - самая старая живая запись)
						deque <field_t> _entries;
					private:
						/**
						 * \~russian
						 * Количество вытесненных записей, удерживаемых в начале списка
						 *
						 * @details Вытеснение отложено: декодер отдаёт наружу представления прямо
						 *          в записи таблицы, не копируя их строки, и запись, вытесненная
						 *          инструкцией потока кодера, обязана дожить до момента, когда
						 *          выданные представления перестанут быть нужны. Удерживаемые
						 *          записи из индексов уже сняты и по ним не находятся, а физически
						 *          снимаются вызовом release() в начале следующего разбора
						 *
						 * \~english
						 * Number of the evicted records held in the beginning of the list
						 * @details The eviction is postponed: the decoder issues outside the representations right
						 *          into the records of the table without copying their strings, and a record evicted
						 *          by an instruction of the stream of the encoder is obliged to live to the moment when
						 *          the issued representations cease to be needed. The held
						 *          records are already removed from the indices and are not found by them, while physically
						 *          they are removed by the call of release() at the beginning of the next parsing
						 *
						 * \~
						 */
						size_t _retained;
						/**
						 * \~russian
						 * Признак того, что наружу выданы представления в записи таблицы
						 *
						 * @details Удержание включается только на время жизни выданных
						 *          представлений. Инструкции потока кодера представлений не
						 *          выдают - заблокированные секции разбираются уже после
						 *          возврата из него, - поэтому вытеснение там снимает записи
						 *          сразу. Без этого признака поток из одних вставок удерживал
						 *          бы все вытесненные записи разом: на таблице в 4 КБ это
						 *          восемь мегабайт, то есть та же decompression bomb
						 *
						 * \~english
						 * Flag of the representations into the records of the table having been issued outside
						 * @details The holding is enabled only for the lifetime of the issued
						 *          representations. The instructions of the stream of the encoder do not issue
						 *          representations - the blocked sections are parsed already after
						 *          a return from it, - therefore the eviction there removes the records
						 *          at once. Without this flag a stream out of the insertions alone would hold
						 *          all the evicted records at once: on a table of 4 KB this is
						 *          eight megabytes, that is the same decompression bomb
						 *
						 * \~
						 */
						bool _holding;
					private:
						// Текущий суммарный размер таблицы
						uint64_t _size;
						// Текущая ёмкость таблицы
						uint64_t _capacity;
					private:
						/**
						 * \~russian
						 * Общее количество когда-либо вставленных записей
						 *
						 * @details Абсолютный номер следующей вставки. Абсолютный номер самой
						 *          старой живой записи равен (_inserts - _entries.size())
						 *
						 * \~english
						 * Total number of the ever inserted records
						 * @details The absolute number of the next insertion. The absolute number of the very
						 *          oldest living record is equal to (_inserts - _entries.size())
						 *
						 * \~
						 */
						uint64_t _inserts;
						// Количество вытесненных записей (абсолютный номер первой живой записи)
						uint64_t _dropped;
					private:
						/**
						 * \~russian
						 * Индекс записей по хешу пары название-значение (хеш -> абсолютный номер)
						 *
						 * @details Сопровождается только у кодера: декодер обращается к таблице
						 *          по готовому номеру и поиском не пользуется. Ключом служит хеш
						 *          пары, а не одного названия: полей с одним названием и разными
						 *          значениями в таблице бывают десятки, и по хешу названия они
						 *          все попадали бы в одно ведро, вырождая поиск в перебор
						 *
						 * \~english
						 * Index of the records by the hash of a pair a name-a value (a hash -> an absolute number)
						 * @details It is maintained only at the encoder: the decoder addresses the table
						 *          by a ready number and does not make use of a search. The key is the hash
						 *          of the pair rather than of the name alone: the fields with one name and different
						 *          values happen in the table by dozens, and by the hash of the name they
						 *          would all get into one bucket, degenerating the search into an enumeration
						 *
						 * \~
						 */
						unordered_multimap <size_t, uint64_t> _index;
						/**
						 * Индекс самой свежей записи с каждым названием (хеш названия -> абсолютный номер)
						 *
						 */
						unordered_map <size_t, uint64_t> _names;
					private:
						// Признак сопровождения индекса записей
						bool _indexing;
					private:
						/**
						 * \~russian
						 * @brief Метод вытеснения записей с самых старых
						 *
						 * @details Вытеснение идёт, пока в таблице не освободится место под
						 *          запись указанного размера. Нулевой размер приводит таблицу
						 *          в соответствие с текущей ёмкостью
						 *
						 * @param room требуемое свободное место в октетах
						 *
						 * \~english
						 * @brief Method of the eviction of the records from the very oldest ones
						 * @details The eviction goes on until in the table a place is freed for
						 *          a record of the indicated size. A zero size brings the table
						 *          into a correspondence with the current capacity
						 * @param room required free place in octets
						 *
						 * \~
						 */
						void evict(const uint64_t room) noexcept;
					public:
						/**
						 * \~russian
						 * @brief Метод получения количества живых записей таблицы
						 *
						 * @return количество живых записей
						 *
						 * \~english
						 * @brief Method of getting the number of the living records of the table
						 * @return number of the living records
						 *
						 * \~
						 */
						size_t count() const noexcept;
						/**
						 * \~russian
						 * @brief Метод снятия удерживаемых вытесненных записей
						 *
						 * @details Вызывается в начале разбора: с этого момента представления,
						 *          выданные наружу прошлым разбором, недействительны
						 *
						 * \~english
						 * @brief Method of the removal of the held evicted records
						 * @details It is called at the beginning of a parsing: from this moment the representations
						 *          issued outside by the past parsing are invalid
						 *
						 * \~
						 */
						void release() noexcept;
						/**
						 * \~russian
						 * @brief Метод установки признака выданных наружу представлений
						 *
						 * @param holding признак выданных наружу представлений
						 *
						 * \~english
						 * @brief Method of setting the flag of the representations issued outside
						 * @param holding flag of the representations issued outside
						 *
						 * \~
						 */
						void holding(const bool holding) noexcept;
						/**
						 * \~russian
						 * @brief Метод получения количества удерживаемых вытесненных записей
						 *
						 * @return количество удерживаемых записей
						 *
						 * \~english
						 * @brief Method of getting the number of the held evicted records
						 * @return number of the held records
						 *
						 * \~
						 */
						size_t retained() const noexcept;
						/**
						 * \~russian
						 * @brief Метод получения текущего суммарного размера таблицы
						 *
						 * @return суммарный размер живых записей с накладными расходами
						 *
						 * \~english
						 * @brief Method of getting the current total size of the table
						 * @return total size of the living records with the overhead
						 *
						 * \~
						 */
						uint64_t size() const noexcept;
						/**
						 * \~russian
						 * @brief Метод получения текущей ёмкости таблицы
						 *
						 * @return текущая ёмкость таблицы
						 *
						 * \~english
						 * @brief Method of getting the current capacity of the table
						 * @return current capacity of the table
						 *
						 * \~
						 */
						uint64_t capacity() const noexcept;
						/**
						 * \~russian
						 * @brief Метод получения общего количества вставок
						 *
						 * @details Одновременно является абсолютным номером следующей вставки
						 *          и значением Insert Count таблицы (RFC 9204 §2.1.4)
						 *
						 * @return общее количество вставленных записей
						 *
						 * \~english
						 * @brief Method of getting the total number of the insertions
						 * @details It is simultaneously the absolute number of the next insertion
						 *          and the value of the Insert Count of the table (RFC 9204 §2.1.4)
						 * @return total number of the inserted records
						 *
						 * \~
						 */
						uint64_t inserts() const noexcept;
						/**
						 * \~russian
						 * @brief Метод получения абсолютного номера самой старой живой записи
						 *
						 * @return количество вытесненных записей
						 *
						 * \~english
						 * @brief Method of getting the absolute number of the very oldest living record
						 * @return number of the evicted records
						 *
						 * \~
						 */
						uint64_t dropped() const noexcept;
					public:
						/**
						 * \~russian
						 * @brief Метод изменения ёмкости таблицы (RFC 9204 §3.2.3)
						 *
						 * @note Уменьшение ёмкости вытесняет записи с самых старых. Попытка
						 *       вытеснить запись, на которую ещё ссылаются, отвергается
						 *       вызывающим кодом до вызова этого метода
						 *
						 * @param capacity новая ёмкость таблицы
						 *
						 * \~english
						 * @brief Method of changing the capacity of the table (RFC 9204 §3.2.3)
						 * @note A decrease of the capacity evicts the records from the very oldest ones. An attempt
						 *       to evict a record to which references still lead is rejected
						 *       by the calling code before the call of this method
						 * @param capacity new capacity of the table
						 *
						 * \~
						 */
						void setCapacity(const uint64_t capacity) noexcept;
						/**
						 * \~russian
						 * @brief Метод управления сопровождением индекса записей
						 *
						 * @param mode режим сопровождения индекса
						 *
						 * \~english
						 * @brief Method of the control of the maintenance of the index of the records
						 * @param mode mode of the maintenance of the index
						 *
						 * \~
						 */
						void indexing(const bool mode) noexcept;
					public:
						/**
						 * \~russian
						 * @brief Метод вычисления размера записи (RFC 9204 §3.2.1)
						 *
						 * @param name  название поля
						 * @param value значение поля
						 * @return      размер записи с накладными расходами
						 *
						 * \~english
						 * @brief Method of the calculation of the size of a record (RFC 9204 §3.2.1)
						 * @param name  name of the field
						 * @param value value of the field
						 * @return      size of the record with the overhead
						 *
						 * \~
						 */
						static uint64_t entrySize(string_view name, string_view value) noexcept;
						/**
						 * \~russian
						 * @brief Метод проверки возможности вставки записи без вытеснения занятых
						 *
						 * @details Записи вытесняются с самой старой, поэтому вставка допустима,
						 *          если освобождения места хватит до первой записи с номером
						 *          не меньше границы удержания
						 *
						 * @param name  название поля
						 * @param value значение поля
						 * @param hold  наименьший абсолютный номер записи, которую нельзя вытеснять
						 * @return       признак возможности вставки
						 *
						 * \~english
						 * @brief Method of checking the possibility of an insertion of a record without an eviction of the occupied ones
						 * @details The records are evicted from the very oldest one, therefore an insertion is admissible,
						 *          if the freeing of the place suffices up to the first record with a number
						 *          not less than the boundary of the holding
						 * @param name  name of the field
						 * @param value value of the field
						 * @param hold  smallest absolute number of a record which cannot be evicted
						 * @return       flag of the possibility of the insertion
						 *
						 * \~
						 */
						bool insertable(string_view name, string_view value, const uint64_t hold) const noexcept;
						/**
						 * \~russian
						 * @brief Метод вставки записи в таблицу
						 *
						 * @note Вытесняет самые старые записи, пока новая не уложится в ёмкость.
						 *       Запись, не помещающаяся в ёмкость целиком, не вставляется вовсе
						 *       и таблицу не трогает: здесь QPACK намеренно расходится с HPACK,
						 *       где такая вставка опустошает таблицу и ошибкой не является
						 *       (RFC 7541 §4.4). В QPACK за размером следит кодер, а попытка -
						 *       ошибка QPACK_ENCODER_STREAM_ERROR (RFC 9204 §3.2.2), и поднимает
						 *       её вызывающий по отрицательному результату
						 *
						 * @param name  название поля
						 * @param value значение поля
						 * @return      признак успешной вставки
						 *
						 * \~english
						 * @brief Method of the insertion of a record into the table
						 * @note It evicts the very oldest records until the new one fits into the capacity.
						 *       A record not fitting into the capacity entirely is not inserted at all
						 *       and does not touch the table: here QPACK deliberately diverges from HPACK
						 *       where such an insertion empties the table and is not an error
						 *       (RFC 7541 §4.4). In QPACK the size is watched by the encoder, while an attempt is
						 *       an error QPACK_ENCODER_STREAM_ERROR (RFC 9204 §3.2.2), and it is raised
						 *       by the caller by the negative result
						 * @param name  name of the field
						 * @param value value of the field
						 * @return      flag of a successful insertion
						 *
						 * \~
						 */
						bool add(string_view name, string_view value) noexcept;
						/**
						 * \~russian
						 * @brief Метод получения записи по абсолютному номеру
						 *
						 * @param absolute абсолютный номер записи
						 * @return          указатель на запись либо nullptr, если запись вытеснена либо ещё не вставлена
						 *
						 * \~english
						 * @brief Method of getting a record by an absolute number
						 * @param absolute absolute number of the record
						 * @return          pointer to the record or nullptr, if the record is evicted or not yet inserted
						 *
						 * \~
						 */
						const field_t * at(const uint64_t absolute) const noexcept;
					public:
						/**
						 * \~russian
						 * @brief Метод поиска записи по названию и значению
						 *
						 * @param name      название искомого поля
						 * @param value     значение искомого поля
						 * @param absolute  абсолютный номер полного совпадения
						 * @param nameOnly  абсолютный номер совпадения только по названию
						 * @return          признак найденного полного совпадения
						 *
						 * \~english
						 * @brief Method of the search of a record by the name and the value
						 * @param name      name of the sought field
						 * @param value     value of the sought field
						 * @param absolute  absolute number of a full coincidence
						 * @param nameOnly  absolute number of a coincidence by the name alone
						 * @return          flag of a found full coincidence
						 *
						 * \~
						 */
						bool find(string_view name, string_view value, uint64_t & absolute, uint64_t & nameOnly) const noexcept;
						/**
						 * \~russian
						 * @brief Метод поиска записи только по названию
						 *
						 * @param name     название искомого поля
						 * @param absolute абсолютный номер совпадения
						 * @return         признак найденного совпадения
						 *
						 * \~english
						 * @brief Method of the search of a record by the name alone
						 * @param name     name of the sought field
						 * @param absolute absolute number of the coincidence
						 * @return         flag of a found coincidence
						 *
						 * \~
						 */
						bool findName(string_view name, uint64_t & absolute) const noexcept;
					public:
						/**
						 * \~russian
						 * @brief Метод очистки таблицы
						 *
						 * @note Сбрасывает и счётчик вставок: применяется при сбросе соединения,
						 *       а не при изменении ёмкости
						 *
						 * \~english
						 * @brief Method of the clearing of the table
						 * @note It also resets the counter of the insertions: it is applied at a reset of the connection
						 *       rather than at a change of the capacity
						 *
						 * \~
						 */
						void clear() noexcept;
					public:
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * @param capacity начальная ёмкость таблицы
						 *
						 * \~english
						 * @brief Constructor
						 * @param capacity initial capacity of the table
						 *
						 * \~
						 */
						explicit DynamicTable(const uint64_t capacity = 0) noexcept;
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
				 * @brief Пространство имён функций поиска в статической таблице (RFC 9204 Appendix A)
				 *
				 * \~english
				 * @brief Namespace of the functions of the search in the static table (RFC 9204 Appendix A)
				 *
				 * \~
				 */
				namespace stat {
					/**
					 * \~russian
					 * @brief Функция поиска записи статической таблицы
					 *
					 * @param name     название искомого поля
					 * @param value    значение искомого поля
					 * @param index    индекс полного совпадения
					 * @param nameOnly индекс совпадения только по названию
					 * @return         признак найденного полного совпадения
					 *
					 * \~english
					 * @brief Function of the search of a record of the static table
					 * @param name     name of the sought field
					 * @param value    value of the sought field
					 * @param index    index of a full coincidence
					 * @param nameOnly index of a coincidence by the name alone
					 * @return         flag of a found full coincidence
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ bool find(string_view name, string_view value, size_t & index, size_t & nameOnly) noexcept;
				};

				/**
				 * \~russian
				 * @brief Класс декодера QPACK
				 *
				 * @details Держит копию динамической таблицы кодера пира и восстанавливает её
				 *          по инструкциям потока кодера. Секции полей разбираются независимо
				 *          друг от друга и в любом порядке; секция, требующая ещё не пришедших
				 *          вставок, отвергается признаком BLOCKED и подлежит повторному разбору
				 *          после обработки очередной порции инструкций кодера.
				 *
				 *          Инструкции для обратного потока декодера копятся во внутреннем буфере
				 *          и забираются методами pending()/consumePending(): собственного канала
				 *          записи у кодека нет - его предоставляет парсер сессии
				 *
				 * \~english
				 * @brief Class of the decoder of QPACK
				 * @details It holds a copy of the dynamic table of the encoder of the peer and restores it
				 *          by the instructions of the stream of the encoder. The sections of the fields are parsed independently
				 *          of each other and in any order; a section requiring not yet arrived
				 *          insertions is rejected by the flag BLOCKED and is subject to a repeated parsing
				 *          after the processing of the next portion of the instructions of the encoder.
				 *          The instructions for the reverse stream of the decoder accumulate in an internal buffer
				 *          and are taken by the methods pending()/consumePending(): the codec has no channel
				 *          of the writing of its own - it is provided by the parser of the session
				 *
				 * \~
				 */
				typedef class __AWH_SHARED_EXPORT__ Decoder {
					private:
						/**
						 * \~russian
						 * @brief Структура среза декодированного поля
						 *
						 * @details Строки не копируются: представление ведёт прямо туда, откуда
						 *          пришло, - в статическую таблицу, в запись динамической либо
						 *          в арену декодированных литералов. Записи статической таблицы
						 *          живут всё время работы программы, вытеснение записей
						 *          динамической отложено до начала следующего разбора, а арена
						 *          отводится на секцию сразу по оценке сверху и за время разбора
						 *          не перевыделяется - поэтому все три представления переживают
						 *          разбор секции целиком
						 *
						 * \~english
						 * @brief Structure of a slice of a decoded field
						 * @details The strings are not copied: a representation leads right there whence
						 *          it has come, - into the static table, into a record of the dynamic one or
						 *          into the arena of the decoded literals. The records of the static table
						 *          live the whole time of the work of the program, the eviction of the records
						 *          of the dynamic one is postponed to the beginning of the next parsing, while the arena
						 *          is allotted for a section at once by an estimation from above and for the time of the parsing
						 *          is not reallocated - therefore all the three representations survive
						 *          the parsing of the section entirely
						 *
						 * \~
						 */
						typedef struct Slice {
							// Название поля
							string_view name;
							// Значение поля
							string_view value;
							// Признак чувствительного значения
							bool sensitive;
						} slice_t;
					private:
						// Динамическая таблица кодера пира
						dynamic_table_t _table;
					private:
						/**
						 * \~russian
						 * Арена декодированных строк текущей секции
						 *
						 * @details Размер буфера равен его ёмкости, а занятая часть отслеживается
						 *          отдельным счётчиком: дописывание методами строки обходится
						 *          вызовом через границу динамической библиотеки на каждое поле,
						 *          тогда как поля секции короткие и вызов стоит дороже самого
						 *          копирования. Ёмкость переиспользуется между секциями
						 *
						 * \~english
						 * Arena of the decoded strings of the current section
						 * @details The size of the buffer is equal to its capacity, while the occupied part is tracked
						 *          by a separate counter: an appending by the methods of a string costs
						 *          a call across the boundary of a dynamic library per field,
						 *          whereas the fields of a section are short and a call costs more than the copying
						 *          itself. The capacity is reused between the sections
						 *
						 * \~
						 */
						string _arena;
						// Занятая часть арены декодированных строк
						size_t _arenaLength;
						// Буфер декодирования значения поля (ёмкость переиспользуется)
						string _scratch;
						/**
						 * \~russian
						 * Буфер декодирования названия поля (ёмкость переиспользуется)
						 *
						 * @details Отдельный от буфера значения намеренно: инструкция вставки
						 *          со ссылкой на название берёт его из динамической таблицы,
						 *          а сама вставка способна вытеснить ту самую запись. Название
						 *          обязано пережить вставку, поэтому копируется сюда
						 *
						 * \~english
						 * Buffer of the decoding of the name of a field (the capacity is reused)
						 * @details It is separate from the buffer of the value deliberately: an instruction of an insertion
						 *          with a reference to a name takes it from the dynamic table,
						 *          while the insertion itself is capable of evicting that very record. The name
						 *          is obliged to survive the insertion, therefore it is copied here
						 *
						 * \~
						 */
						string _name;
						// Срезы декодированных полей текущей секции (ёмкость переиспользуется)
						vector <slice_t> _slices;
					private:
						// Буфер инструкций для потока декодера
						string _output;
						// Количество уже выданных наружу октетов буфера инструкций
						size_t _consumed;
					private:
						// Верхняя граница ёмкости таблицы, анонсированная нами в SETTINGS
						uint64_t _maxCapacity;
						// Число потоков, которым мы разрешили ожидать пополнения таблицы
						uint64_t _maxBlocked;
					private:
						/**
						 * \~russian
						 * Количество вставок, о котором уже извещён кодер пира
						 *
						 * @details Кодер ведёт это число под названием Known Received Count и
						 *          опирается на него, решая, какие записи можно вытеснять.
						 *          Извещение идёт двумя путями - подтверждением секции и
						 *          приращением счётчика вставок, - и оба обязаны быть учтены
						 *          здесь согласованно: иначе приращение наложилось бы на уже
						 *          подтверждённое секцией и увело счётчик кодера за число вставок
						 *
						 * \~english
						 * Number of the insertions about which the encoder of the peer is already notified
						 * @details The encoder conducts this number under the name Known Received Count and
						 *          leans on it, deciding which records may be evicted.
						 *          The notification goes by two ways - by a confirmation of a section and
						 *          by an increment of the counter of the insertions, - and both are obliged to be accounted
						 *          here concordantly: otherwise the increment would superimpose on the already
						 *          confirmed by the section and would lead the counter of the encoder beyond the number of the insertions
						 *
						 * \~
						 */
						uint64_t _acked;
					private:
						// Потоки, ожидающие пополнения таблицы (идентификатор потока -> требуемое число вставок)
						unordered_map <uint64_t, uint64_t> _blocked;
					private:
						// Последняя секция превысила лимит списка полей (декодирована, но не отдана)
						bool _overflow;
					private:
						/**
						 * \~russian
						 * @brief Метод декодирования строки, закодированной литералом либо Huffman
						 *
						 * @param data       входной буфер
						 * @param size       доступно байт
						 * @param prefixBits размер префикса длины в битах
						 * @param output     выходной буфер строки
						 * @param consumed   количество прочитанных байт
						 * @return           результат декодирования
						 *
						 * \~english
						 * @brief Method of the decoding of a string encoded by a literal or by a Huffman one
						 * @param data       input buffer
						 * @param size       octets available
						 * @param prefixBits size of the prefix of the length in bits
						 * @param output     output buffer of the string
						 * @param consumed   number of the read octets
						 * @return           result of the decoding
						 *
						 * \~
						 */
						status_t decodeString(const uint8_t * data, const size_t size, const uint8_t prefixBits, string & output, size_t & consumed) noexcept;
						/**
						 * \~russian
						 * @brief Метод декодирования строки представления прямо в арену
						 *
						 * @details Отличается от разбора в отдельный буфер только назначением, но
						 *          именно оно и стоит дорого: декодированное поле всё равно ложится
						 *          в арену, и разбор в промежуточный буфер означал бы лишнее
						 *          копирование каждой строки секции
						 *
						 * @param data       входной буфер
						 * @param size       доступно байт
						 * @param prefixBits размер префикса длины в битах
						 * @param consumed   количество прочитанных байт
						 * @return           результат декодирования
						 *
						 * \~english
						 * @brief Method of the decoding of a string of a representation right into the arena
						 * @details It differs from a parsing into a separate buffer only by the destination, but
						 *          exactly it costs dearly: a decoded field lies down into the arena
						 *          all the same, and a parsing into an intermediate buffer would mean a superfluous
						 *          copying of every string of the section
						 * @param data       input buffer
						 * @param size       octets available
						 * @param prefixBits size of the prefix of the length in bits
						 * @param consumed   number of the read octets
						 * @return           result of the decoding
						 *
						 * \~
						 */
						status_t decodeString(const uint8_t * data, const size_t size, const uint8_t prefixBits, size_t & consumed) noexcept;
						/**
						 * \~russian
						 * @brief Метод разрешения абсолютного номера записи в пару полей
						 *
						 * @param absolute абсолютный номер записи динамической таблицы
						 * @param entry    найденная запись
						 * @return         признак успешного разрешения
						 *
						 * \~english
						 * @brief Method of the resolution of an absolute number of a record into a pair of the fields
						 * @param absolute absolute number of the record of the dynamic table
						 * @param entry    found record
						 * @return         flag of a successful resolution
						 *
						 * \~
						 */
						bool resolve(const uint64_t absolute, const field_t *& entry) const noexcept;
						/**
						 * \~russian
						 * @brief Метод выделения места в арене декодированных строк
						 *
						 * @param size требуемое количество октетов
						 * @return     указатель на выделенное место
						 *
						 * \~english
						 * @brief Method of the allocation of a place in the arena of the decoded strings
						 * @param size required number of the octets
						 * @return     pointer to the allocated place
						 *
						 * \~
						 */
						char * reserve(const size_t size) noexcept;
						/**
						 * \~russian
						 * @brief Метод дописывания строки в арену декодированных строк
						 *
						 * @param value дописываемая строка
						 * @return      смещение дописанной строки в арене
						 *
						 * \~english
						 * @brief Method of the appending of a string into the arena of the decoded strings
						 * @param value string being appended
						 * @return      displacement of the appended string in the arena
						 *
						 * \~
						 */
						size_t append(string_view value) noexcept;
						/**
						 * \~russian
						 * @brief Метод учёта декодированного поля, уже лежащего в арене
						 *
						 * @param nameOffset  смещение названия поля в арене
						 * @param nameLength  длина названия поля
						 * @param valueOffset смещение значения поля в арене
						 * @param valueLength длина значения поля
						 * @param sensitive   признак чувствительного значения
						 * @param listSize    накопленный размер списка полей
						 * @param maxListSize лимит размера списка полей
						 *
						 * \~english
						 * @brief Method of the account of a decoded field already lying in the arena
						 * @param nameOffset  displacement of the name of the field in the arena
						 * @param nameLength  length of the name of the field
						 * @param valueOffset displacement of the value of the field in the arena
						 * @param valueLength length of the value of the field
						 * @param sensitive   flag of a sensitive value
						 * @param listSize    accumulated size of the list of the fields
						 * @param maxListSize limit of the size of the list of the fields
						 *
						 * \~
						 */
						void emit(string_view name, string_view value, const bool sensitive, const size_t mark, uint64_t & listSize, const uint64_t maxListSize) noexcept;
					public:
						/**
						 * \~russian
						 * @brief Метод получения динамической таблицы кодера пира
						 *
						 * @return динамическая таблица кодера пира
						 *
						 * \~english
						 * @brief Method of getting the dynamic table of the encoder of the peer
						 * @return dynamic table of the encoder of the peer
						 *
						 * \~
						 */
						dynamic_table_t & table() noexcept;
						/**
						 * \~russian
						 * @brief Метод установки верхней границы ёмкости таблицы
						 *
						 * @note Должна равняться анонсированному нами SETTINGS_QPACK_MAX_TABLE_CAPACITY.
						 *       Превышение кодером пира трактуется как QPACK_ENCODER_STREAM_ERROR
						 *
						 * @param capacity верхняя граница ёмкости таблицы
						 *
						 * \~english
						 * @brief Method of setting the upper boundary of the capacity of the table
						 * @note It is obliged to be equal to the SETTINGS_QPACK_MAX_TABLE_CAPACITY announced by us.
						 *       An exceeding by the encoder of the peer is treated as a QPACK_ENCODER_STREAM_ERROR
						 * @param capacity upper boundary of the capacity of the table
						 *
						 * \~
						 */
						void maxCapacity(const uint64_t capacity) noexcept;
						/**
						 * \~russian
						 * @brief Метод установки числа потоков, которым разрешено ожидать пополнения таблицы
						 *
						 * @note Должно равняться анонсированному нами SETTINGS_QPACK_BLOCKED_STREAMS.
						 *       Превышение кодером пира трактуется как QPACK_DECOMPRESSION_FAILED
						 *
						 * @param count число потоков
						 *
						 * \~english
						 * @brief Method of setting the number of the streams which are allowed to wait for a replenishment of the table
						 * @note It is obliged to be equal to the SETTINGS_QPACK_BLOCKED_STREAMS announced by us.
						 *       An exceeding by the encoder of the peer is treated as a QPACK_DECOMPRESSION_FAILED
						 * @param count number of the streams
						 *
						 * \~
						 */
						void maxBlocked(const uint64_t count) noexcept;
						/**
						 * \~russian
						 * @brief Метод получения количества потоков, ожидающих пополнения таблицы
						 *
						 * @return количество заблокированных потоков
						 *
						 * \~english
						 * @brief Method of getting the number of the streams waiting for a replenishment of the table
						 * @return number of the blocked streams
						 *
						 * \~
						 */
						size_t blocked() const noexcept;
					public:
						/**
						 * \~russian
						 * @brief Метод обработки инструкций потока кодера (RFC 9204 §4.3)
						 *
						 * @details Инструкции обрабатываются по одной, пока во входном буфере есть
						 *          целая инструкция. Неполный остаток буфера не является ошибкой:
						 *          вызывающий обязан сохранить его и подать снова вместе с продолжением
						 *
						 * @param data     входной буфер потока кодера
						 * @param consumed количество разобранных октетов
						 * @param error    код ошибки протокола
						 * @return         результат обработки (OK/ERROR)
						 *
						 * \~english
						 * @brief Method of the processing of the instructions of the stream of the encoder (RFC 9204 §4.3)
						 * @details The instructions are processed one by one while in the input buffer there is
						 *          a whole instruction. An incomplete remainder of the buffer is not an error:
						 *          the caller is obliged to preserve it and to supply it again together with the continuation
						 * @param data     input buffer of the stream of the encoder
						 * @param consumed number of the parsed octets
						 * @param error    error code of the protocol
						 * @return         result of the processing (OK/ERROR)
						 *
						 * \~
						 */
						status_t decodeEncoderStream(string_view data, size_t & consumed, error_t & error) noexcept;
						/**
						 * \~russian
						 * @brief Метод декодирования секции полей целиком
						 *
						 * @details Названия и значения декодируются во внутреннюю арену декодера,
						 *          а в output попадают ссылки на неё: аллокаций в установившемся
						 *          режиме нет. Прежнее содержимое output замещается
						 *
						 * @note Полученные представления действительны до следующего вызова
						 *       decode() на этом же декодере - копируйте то, что нужно дольше
						 *
						 * @param sid         идентификатор потока, которому принадлежит секция
						 * @param section     секция полей целиком (нагрузка кадра HEADERS)
						 * @param output      декодированные поля (ссылки в арену декодера)
						 * @param maxListSize лимит суммарного размера списка полей; 0 - без лимита
						 * @param error       код ошибки протокола
						 * @return            результат декодирования (OK/BLOCKED/ERROR)
						 *
						 * \~english
						 * @brief Method of the decoding of a section of the fields entirely
						 * @details The names and the values are decoded into the internal arena of the decoder,
						 *          while into output the references to it get: there are no allocations in the settled
						 *          mode. The previous content of output is replaced
						 * @note The obtained representations are valid until the next call of
						 *       decode() on this same decoder - copy that which is needed longer
						 * @param sid         identifier of the stream to which the section belongs
						 * @param section     section of the fields entirely (the payload of a HEADERS frame)
						 * @param output      decoded fields (the references into the arena of the decoder)
						 * @param maxListSize limit of the total size of the list of the fields; 0 - without a limit
						 * @param error       error code of the protocol
						 * @return            result of the decoding (OK/BLOCKED/ERROR)
						 *
						 * \~
						 */
						status_t decode(const uint64_t sid, string_view section, vector <field_view_t> & output, const uint64_t maxListSize, error_t & error) noexcept;
						/**
						 * \~russian
						 * @brief Метод проверки превышения лимита списка полей последней секцией
						 *
						 * @details Секция сверх лимита разбирается целиком - иначе динамическая
						 *          таблица рассинхронизировалась бы с кодером пира, - но поля
						 *          наружу не отдаются. Вызывающий вправе отвергнуть один поток,
						 *          оставив соединение живым
						 *
						 * @return признак превышения лимита последней декодированной секцией
						 *
						 * \~english
						 * @brief Method of checking the exceeding of the limit of the list of the fields by the last section
						 * @details A section above the limit is parsed entirely - otherwise the dynamic
						 *          table would become desynchronized with the encoder of the peer, - but the fields
						 *          are not issued outside. The caller is entitled to reject one stream,
						 *          leaving the connection alive
						 * @return flag of the exceeding of the limit by the last decoded section
						 *
						 * \~
						 */
						bool overflowed() const noexcept;
						/**
						 * \~russian
						 * @brief Метод отмены потока (RFC 9204 §4.4.2)
						 *
						 * @details Извещает кодер пира, что секции этого потока разобраны не будут
						 *          и ссылки на записи таблицы можно считать снятыми
						 *
						 * @param sid идентификатор отменяемого потока
						 *
						 * \~english
						 * @brief Method of the cancellation of a stream (RFC 9204 §4.4.2)
						 * @details It notifies the encoder of the peer that the sections of this stream will not be parsed
						 *          and the references to the records of the table may be considered removed
						 * @param sid identifier of the stream being cancelled
						 *
						 * \~
						 */
						void cancel(const uint64_t sid) noexcept;
					public:
						/**
						 * \~russian
						 * @brief Метод получения накопленных инструкций потока декодера
						 *
						 * @return представление накопленных инструкций
						 *
						 * \~english
						 * @brief Method of getting the accumulated instructions of the stream of the decoder
						 * @return representation of the accumulated instructions
						 *
						 * \~
						 */
						string_view pending() const noexcept;
						/**
						 * \~russian
						 * @brief Метод отметки инструкций потока декодера как отправленных
						 *
						 * @param size количество отправленных октетов
						 *
						 * \~english
						 * @brief Method of marking the instructions of the stream of the decoder as sent
						 * @param size number of the sent octets
						 *
						 * \~
						 */
						void consumePending(const size_t size) noexcept;
					public:
						/**
						 * \~russian
						 * @brief Метод сброса состояния декодера
						 *
						 * \~english
						 * @brief Method of the reset of the state of the decoder
						 *
						 * \~
						 */
						void clear() noexcept;
					public:
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * @param maxCapacity верхняя граница ёмкости динамической таблицы
						 * @param maxBlocked  число потоков, которым разрешено ожидать пополнения таблицы
						 *
						 * \~english
						 * @brief Constructor
						 * @param maxCapacity upper boundary of the capacity of the dynamic table
						 * @param maxBlocked  number of the streams which are allowed to wait for a replenishment of the table
						 *
						 * \~
						 */
						explicit Decoder(const uint64_t maxCapacity = proto::QPACK_TABLE_CAPACITY, const uint64_t maxBlocked = proto::QPACK_BLOCKED_STREAMS) noexcept;
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
				 * @brief Класс кодера QPACK
				 *
				 * @details Держит собственную динамическую таблицу и следит за тем, какая её часть
				 *          заведомо известна декодеру пира. Вытеснить запись, на которую ещё
				 *          ссылается неподтверждённая секция, нельзя: декодер пира разобрал бы
				 *          такую ссылку неверно, поэтому вытеснение ограничено снизу самой старой
				 *          записью, удерживаемой ссылками.
				 *
				 *          Ссылка на запись, которую декодер пира ещё не получил, блокирует поток
				 *          до её прихода. Это законно и полезно - иначе кодер не мог бы ссылаться
				 *          на только что вставленное, - но число одновременно заблокированных
				 *          потоков ограничено анонсом пира, и кодер обязан его соблюдать
				 *
				 * \~english
				 * @brief Class of the encoder of QPACK
				 * @details It holds its own dynamic table and watches which part of it
				 *          is knowingly known to the decoder of the peer. To evict a record to which
				 *          an unconfirmed section still refers is impossible: the decoder of the peer would parse
				 *          such a reference incorrectly, therefore the eviction is limited from below by the very oldest
				 *          record held by the references.
				 *          A reference to a record which the decoder of the peer has not yet obtained blocks the stream
				 *          until its arrival. This is lawful and useful - otherwise the encoder could not refer
				 *          to the just inserted, - but the number of the simultaneously blocked
				 *          streams is limited by the announcement of the peer, and the encoder is obliged to observe it
				 *
				 * \~
				 */
				typedef class __AWH_SHARED_EXPORT__ Encoder {
					private:
						/**
						 * \~russian
						 * @brief Признак отсутствия записи учёта секции в пуле
						 *
						 * \~english
						 * @brief Flag of an absence of a record of the account of a section in the pool
						 *
						 * \~
						 */
						static constexpr uint32_t NO_SECTION = static_cast <uint32_t> (~0u);
					private:
						/**
						 * \~russian
						 * @brief Структура учёта одной отправленной секции полей
						 *
						 * \~english
						 * @brief Structure of the account of a single sent section of the fields
						 *
						 * \~
						 */
						typedef struct Section {
							// Требуемое число вставок для разбора секции
							uint64_t required;
							/**
							 * \~russian
							 * Наименьший абсолютный номер записи, на которую ссылается секция
							 *
							 * @details Поимённого учёта ссылок не ведётся. Вытеснение ограничено
							 *          снизу самой старой удерживаемой записью, а она равна
							 *          наименьшему из посекционных минимумов - больше ни для чего
							 *          номера ссылок не нужны. Хранить их значило бы заводить
							 *          список на каждую секцию и счётчик на каждую запись,
							 *          то есть обращаться к аллокатору и на то, и на другое,
							 *          ради ответа, который умещается в одно число
							 *
							 * \~english
							 * Smallest absolute number of a record to which the section refers
							 * @details A name-by-name account of the references is not conducted. The eviction is limited
							 *          from below by the very oldest held record, and it is equal
							 *          to the smallest of the per-section minimums - for nothing else
							 *          the numbers of the references are needed. To store them would mean to start
							 *          a list per section and a counter per record,
							 *          that is to address the allocator both for the one and for the other,
							 *          for the sake of an answer which fits into a single number
							 *
							 * \~
							 */
							uint64_t minimal;
							// Позиция следующей секции потока в пуле, либо NO_SECTION
							uint32_t next;
						} section_t;
						/**
						 * \~russian
						 * @brief Структура неподтверждённых секций одного потока
						 *
						 * @details Секции потока подтверждаются в порядке отправки, поэтому
						 *          связный список с обоими концами наружу - ровно та структура,
						 *          которая нужна: подтверждение снимает голову, кодирование
						 *          дописывает хвост, а откат снимает хвост
						 *
						 * \~english
						 * @brief Structure of the unconfirmed sections of a single stream
						 * @details The sections of a stream are confirmed in the order of the sending, therefore
						 *          a linked list with both ends outside is exactly that structure
						 *          which is needed: a confirmation removes the head, an encoding
						 *          appends the tail, while a rollback removes the tail
						 *
						 * \~
						 */
						typedef struct Stream {
							// Позиция самой старой неподтверждённой секции потока в пуле
							uint32_t head;
							// Позиция самой свежей неподтверждённой секции потока в пуле
							uint32_t tail;
						} stream_t;
					private:
						// Собственная динамическая таблица
						dynamic_table_t _table;
					private:
						// Буфер инструкций для потока кодера
						string _output;
						// Количество уже выданных наружу октетов буфера инструкций
						size_t _consumed;
						// Буфер сборки представлений полей секции (ёмкость переиспользуется)
						string _scratch;
						/**
						 * \~russian
						 * Представления кодируемых полей (ёмкость переиспользуется)
						 *
						 * @details Кодирование владеющего списка полей и списка представлений
						 *          отличается только источником строк, поэтому владеющий список
						 *          приводится к представлениям, а сам алгоритм существует
						 *          в единственном экземпляре
						 *
						 * \~english
						 * Representations of the fields being encoded (the capacity is reused)
						 * @details The encoding of an owning list of the fields and of a list of the representations
						 *          differs only by the source of the strings, therefore an owning list
						 *          is brought to the representations, while the algorithm itself exists
						 *          in a single copy
						 *
						 * \~
						 */
						vector <field_view_t> _views;
					private:
						/**
						 * \~russian
						 * Пул записей учёта отправленных секций (место переиспользуется)
						 *
						 * @details Секции заводятся и снимаются с учёта на каждый запрос, поэтому
						 *          владеющий контейнер на поток обходился бы обращением
						 *          к аллокатору на отправку и ещё одним на подтверждение.
						 *          Записи пула вместо этого связаны позициями и переиспользуются
						 *          через список свободных: в установившемся режиме учёт секций
						 *          не стоит ни одного обращения к аллокатору
						 *
						 * @note Пул растёт до пика числа одновременно неподтверждённых секций
						 *       и сам не сокращается: место освобождает clear()
						 *
						 * \~english
						 * Pool of the records of the account of the sent sections (the place is reused)
						 * @details The sections are started and removed from the account per request, therefore
						 *          an owning container per stream would cost an address
						 *          to the allocator per sending and one more per confirmation.
						 *          The records of the pool are instead linked by the positions and are reused
						 *          through a list of the free ones: in the settled mode the account of the sections
						 *          costs not a single address to the allocator
						 * @note The pool grows up to the peak of the number of the simultaneously unconfirmed sections
						 *       and does not shrink by itself: the place is freed by clear()
						 *
						 * \~
						 */
						vector <section_t> _pool;
						// Позиция головы списка свободных записей пула, либо NO_SECTION
						uint32_t _spare;
					private:
						// Верхняя граница ёмкости таблицы, анонсированная пиром
						uint64_t _maxCapacity;
						// Число потоков, которым пир разрешил ожидать пополнения таблицы
						uint64_t _maxBlocked;
					private:
						/**
						 * \~russian
						 * Количество вставок, заведомо полученных декодером пира
						 *
						 * @details Известно из подтверждений секций и приращений счётчика вставок,
						 *          приходящих потоком декодера (RFC 9204 §2.1.4). Ссылка на запись
						 *          с номером не меньше этого числа заблокирует поток до её прихода
						 *
						 * \~english
						 * Number of the insertions knowingly obtained by the decoder of the peer
						 * @details It is known from the confirmations of the sections and from the increments of the counter of the insertions
						 *          coming by the stream of the decoder (RFC 9204 §2.1.4). A reference to a record
						 *          with a number not less than this number will block the stream until its arrival
						 *
						 * \~
						 */
						uint64_t _known;
					private:
						// Отправленные, но не подтверждённые секции по потокам
						unordered_map <uint64_t, stream_t> _sections;
					private:
						// Размер закодированного списка полей текущей секции до сжатия
						uint64_t _listSize;
					private:
						/**
						 * \~russian
						 * Кольцо хешей пар название-значение недавно встреченных полей
						 *
						 * @details Место в динамической таблице стоит отдавать полям, которые
						 *          повторяются. Поле с разовым значением иначе вытесняет как раз
						 *          повторяющиеся, и таблица входит в постоянное вытеснение сама
						 *          себя. Пустое кольцо означает, что адаптивная индексация выключена
						 *
						 * \~english
						 * Ring of the hashes of the pairs a name-a value of the recently met fields
						 * @details The place in the dynamic table is worth giving away to the fields which
						 *          repeat. A field with a one-time value otherwise evicts exactly
						 *          the repeating ones, and the table enters a permanent eviction of
						 *          itself. An empty ring means that the adaptive indexing is disabled
						 *
						 * \~
						 */
						vector <uint32_t> _history;
						// Позиция записи в кольце хешей
						size_t _historyIndex;
						// Признак заполненности кольца хешей целиком
						bool _historyWrapped;
					private:
						// Автоматически считать чувствительными authorization/cookie и им подобные
						bool _sensitiveHeuristic;
					private:
						/**
						 * \~russian
						 * @brief Метод занятия записи учёта секции в пуле
						 *
						 * @return позиция занятой записи в пуле
						 *
						 * \~english
						 * @brief Method of the occupation of a record of the account of a section in the pool
						 * @return position of the occupied record in the pool
						 *
						 * \~
						 */
						uint32_t acquire() noexcept;
						/**
						 * \~russian
						 * @brief Метод возврата записи учёта секции в пул
						 *
						 * @param position позиция возвращаемой записи в пуле
						 *
						 * \~english
						 * @brief Method of the return of a record of the account of a section into the pool
						 * @param position position of the record being returned in the pool
						 *
						 * \~
						 */
						void release(const uint32_t position) noexcept;
					private:
						/**
						 * \~russian
						 * @brief Метод получения наименьшего абсолютного номера удерживаемой записи
						 *
						 * @details Вытеснять записи ниже этой границы безопасно: на них не ссылается
						 *          ни одна неподтверждённая секция
						 *
						 * @return наименьший абсолютный номер удерживаемой ссылками записи
						 *
						 * \~english
						 * @brief Method of getting the smallest absolute number of a held record
						 * @details To evict the records below this boundary is safe: not a single unconfirmed section
						 *          refers to them
						 * @return smallest absolute number of a record held by the references
						 *
						 * \~
						 */
						uint64_t hold() const noexcept;
						/**
						 * \~russian
						 * @brief Метод подсчёта потоков, заблокированных ссылками на неполученные записи
						 *
						 * @return количество заблокированных потоков
						 *
						 * \~english
						 * @brief Method of the counting of the streams blocked by the references to the not obtained records
						 * @return number of the blocked streams
						 *
						 * \~
						 */
						size_t blocking() const noexcept;
						/**
						 * \~russian
						 * @brief Метод принятия решения об индексации поля
						 *
						 * @note Метод изменяет состояние кольца истории, поэтому вызывается
						 *       ровно один раз на кодируемое поле
						 *
						 * @param name  название поля
						 * @param value значение поля
						 * @return      признак необходимости занести поле в таблицу
						 *
						 * \~english
						 * @brief Method of the taking of the decision about the indexing of a field
						 * @note The method changes the state of the ring of the history, therefore it is called
						 *       exactly once per encoded field
						 * @param name  name of the field
						 * @param value value of the field
						 * @return      flag of the necessity to enter the field into the table
						 *
						 * \~
						 */
						bool indexable(string_view name, string_view value) noexcept;
						/**
						 * \~russian
						 * @brief Метод записи строки литералом либо Huffman
						 *
						 * @param output      выходной буфер
						 * @param input       записываемая строка
						 * @param prefixBits  размер префикса длины в битах
						 * @param prefixValue значение старших бит первого байта
						 * @param useHuffman  применять Huffman-кодирование
						 *
						 * \~english
						 * @brief Method of the writing of a string by a literal or by a Huffman one
						 * @param output      output buffer
						 * @param input       string being written
						 * @param prefixBits  size of the prefix of the length in bits
						 * @param prefixValue value of the higher bits of the first octet
						 * @param useHuffman  to apply the Huffman encoding
						 *
						 * \~
						 */
						void encodeString(string & output, string_view input, const uint8_t prefixBits, const uint8_t prefixValue, const bool useHuffman) noexcept;
						/**
						 * \~russian
						 * @brief Метод записи инструкции пополнения динамической таблицы
						 *
						 * @param name       название поля
						 * @param value      значение поля
						 * @param nameStatic индекс названия в статической таблице либо STATIC_TABLE_SIZE
						 * @param nameEntry  абсолютный номер названия в динамической таблице либо UINT64_MAX
						 * @param useHuffman применять Huffman-кодирование к строкам
						 *
						 * \~english
						 * @brief Method of the writing of an instruction of a replenishment of the dynamic table
						 * @param name       name of the field
						 * @param value      value of the field
						 * @param nameStatic index of the name in the static table or STATIC_TABLE_SIZE
						 * @param nameEntry  absolute number of the name in the dynamic table or UINT64_MAX
						 * @param useHuffman to apply the Huffman encoding to the strings
						 *
						 * \~
						 */
						void insertInstruction(string_view name, string_view value, const size_t nameStatic, const uint64_t nameEntry, const bool useHuffman) noexcept;
						/**
						 * \~russian
						 * @brief Метод кодирования секции полей из представлений
						 *
						 * @param sid        идентификатор потока, которому принадлежит секция
						 * @param fields     кодируемые поля
						 * @param output     выходной буфер секции полей
						 * @param useHuffman применять Huffman-кодирование к строкам
						 *
						 * \~english
						 * @brief Method of the encoding of a section of the fields out of the representations
						 * @param sid        identifier of the stream to which the section belongs
						 * @param fields     fields being encoded
						 * @param output     output buffer of the section of the fields
						 * @param useHuffman to apply the Huffman encoding to the strings
						 *
						 * \~
						 */
						void encodeSection(const uint64_t sid, const vector <field_view_t> & fields, string & output, const bool useHuffman) noexcept;
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
						 * @brief Метод установки верхней границы ёмкости таблицы, анонсированной пиром
						 *
						 * @note Вызывается при получении SETTINGS_QPACK_MAX_TABLE_CAPACITY пира.
						 *       В поток кодера ставится инструкция изменения ёмкости, чтобы декодер
						 *       пира выделил таблицу того же размера
						 *
						 * @param capacity верхняя граница ёмкости таблицы
						 *
						 * \~english
						 * @brief Method of setting the upper boundary of the capacity of the table announced by the peer
						 * @note It is called at the receipt of a SETTINGS_QPACK_MAX_TABLE_CAPACITY of the peer.
						 *       Into the stream of the encoder an instruction of a change of the capacity is put, so that the decoder
						 *       of the peer would allot a table of the same size
						 * @param capacity upper boundary of the capacity of the table
						 *
						 * \~
						 */
						void maxCapacity(const uint64_t capacity) noexcept;
						/**
						 * \~russian
						 * @brief Метод установки числа потоков, которым пир разрешил ожидать пополнения таблицы
						 *
						 * @param count число потоков
						 *
						 * \~english
						 * @brief Method of setting the number of the streams which the peer has allowed to wait for a replenishment of the table
						 * @param count number of the streams
						 *
						 * \~
						 */
						void maxBlocked(const uint64_t count) noexcept;
						/**
						 * \~russian
						 * @brief Метод получения размера закодированного списка полей до сжатия
						 *
						 * @details Считается по правилу RFC 9114 §4.2.2 (сумма длин названий и значений
						 *          плюс 32 октета на поле) и сбрасывается в начале каждой секции.
						 *          Нужен для сверки с анонсированным пиром SETTINGS_MAX_FIELD_SECTION_SIZE
						 *
						 * @return размер списка полей последней секции до сжатия
						 *
						 * \~english
						 * @brief Method of getting the size of the encoded list of the fields before the compression
						 * @details It is counted by the rule of RFC 9114 §4.2.2 (the sum of the lengths of the names and of the values
						 *          plus 32 octets per field) and is reset at the beginning of every section.
						 *          It is needed for a comparison with the SETTINGS_MAX_FIELD_SECTION_SIZE announced by the peer
						 * @return size of the list of the fields of the last section before the compression
						 *
						 * \~
						 */
						uint64_t listSize() const noexcept;
					public:
						/**
						 * \~russian
						 * @brief Метод управления автоматическим определением чувствительных полей
						 *
						 * @details Включено по умолчанию: authorization/proxy-authorization/cookie/set-cookie
						 *          кодируются с запретом индексации и не попадают в динамическую таблицу.
						 *          Выключение улучшает сжатие cookie-тяжёлого трафика ценой снятия
						 *          защиты от атак класса CRIME
						 *
						 * @param mode режим автоматического определения
						 *
						 * \~english
						 * @brief Method of the control of the automatic determination of the sensitive fields
						 * @details It is enabled by default: authorization/proxy-authorization/cookie/set-cookie
						 *          are encoded with a prohibition of the indexing and do not get into the dynamic table.
						 *          A disabling improves the compression of the cookie-heavy traffic at the price of the removal
						 *          of the protection from the attacks of the class CRIME
						 * @param mode mode of the automatic determination
						 *
						 * \~
						 */
						void sensitiveHeuristic(const bool mode) noexcept;
						/**
						 * \~russian
						 * @brief Метод управления адаптивной индексацией полей
						 *
						 * @details Включено по умолчанию: в динамическую таблицу заносятся только
						 *          поля, уже встречавшиеся в пределах кольца истории
						 *
						 * @param mode режим адаптивной индексации
						 *
						 * \~english
						 * @brief Method of the control of the adaptive indexing of the fields
						 * @details It is enabled by default: into the dynamic table only the
						 *          fields already met within the limits of the ring of the history are entered
						 * @param mode mode of the adaptive indexing
						 *
						 * \~
						 */
						void adaptiveIndexing(const bool mode) noexcept;
					public:
						/**
						 * \~russian
						 * @brief Метод обработки инструкций потока декодера (RFC 9204 §4.4)
						 *
						 * @param data     входной буфер потока декодера
						 * @param consumed количество разобранных октетов
						 * @param error    код ошибки протокола
						 * @return         результат обработки (OK/ERROR)
						 *
						 * \~english
						 * @brief Method of the processing of the instructions of the stream of the decoder (RFC 9204 §4.4)
						 * @param data     input buffer of the stream of the decoder
						 * @param consumed number of the parsed octets
						 * @param error    error code of the protocol
						 * @return         result of the processing (OK/ERROR)
						 *
						 * \~
						 */
						status_t decodeDecoderStream(string_view data, size_t & consumed, error_t & error) noexcept;
						/**
						 * \~russian
						 * @brief Метод кодирования секции полей
						 *
						 * @details Инструкции пополнения таблицы дописываются в буфер потока кодера
						 *          и обязаны быть отправлены пиру; порядок между ними и секцией
						 *          значения не имеет - за согласованность отвечает Required Insert
						 *          Count в префиксе секции
						 *
						 * @param sid        идентификатор потока, которому принадлежит секция
						 * @param fields     поля (псевдо-заголовки должны идти первыми)
						 * @param output     выходной буфер секции полей
						 * @param useHuffman применять Huffman-кодирование к строкам
						 *
						 * \~english
						 * @brief Method of the encoding of a section of the fields
						 * @details The instructions of a replenishment of the table are appended into the buffer of the stream of the encoder
						 *          and are obliged to be sent to the peer; the order between them and the section
						 *          has no meaning - for the concordance the Required Insert
						 *          Count in the prefix of the section answers
						 * @param sid        identifier of the stream to which the section belongs
						 * @param fields     fields (the pseudo headers are obliged to go first)
						 * @param output     output buffer of the section of the fields
						 * @param useHuffman to apply the Huffman encoding to the strings
						 *
						 * \~
						 */
						void encode(const uint64_t sid, const vector <field_t> & fields, std::string & output, const bool useHuffman = true) noexcept;
						/**
						 * \~russian
						 * @brief Метод кодирования секции полей из декодированного списка (перекодирование)
						 *
						 * @details Позволяет переслать разобранную секцию без промежуточных копий строк
						 *          (сценарий шлюза). Признак чувствительности сохраняется
						 *
						 * @param sid        идентификатор потока, которому принадлежит секция
						 * @param fields     декодированные поля
						 * @param output     выходной буфер секции полей
						 * @param useHuffman применять Huffman-кодирование к строкам
						 *
						 * \~english
						 * @brief Method of the encoding of a section of the fields out of a decoded list (a re-encoding)
						 * @details It allows to forward a parsed section without the intermediate copies of the strings
						 *          (a scenario of a gateway). The flag of the sensitivity is preserved
						 * @param sid        identifier of the stream to which the section belongs
						 * @param fields     decoded fields
						 * @param output     output buffer of the section of the fields
						 * @param useHuffman to apply the Huffman encoding to the strings
						 *
						 * \~
						 */
						void encode(const uint64_t sid, const vector <field_view_t> & fields, std::string & output, const bool useHuffman = true) noexcept;
						/**
						 * \~russian
						 * @brief Метод снятия ссылок отменённого потока
						 *
						 * @details Вызывается, когда поток сброшен и подтверждения его секций
						 *          уже не придут: удерживаемые ими записи освобождаются
						 *
						 * @param sid идентификатор отменяемого потока
						 *
						 * \~english
						 * @brief Method of the removal of the references of a cancelled stream
						 * @details It is called when a stream is reset and the confirmations of its sections
						 *          will not come any more: the records held by them are freed
						 * @param sid identifier of the stream being cancelled
						 *
						 * \~
						 */
						void cancel(const uint64_t sid) noexcept;
						/**
						 * \~russian
						 * @brief Метод отката последней закодированной секции потока
						 *
						 * @details Вызывается, когда закодированная секция так и не была отправлена:
						 *          её ссылки снимаются, а прежние секции потока остаются на учёте.
						 *          Отменять весь поток здесь нельзя - подтверждения уже отправленных
						 *          секций придут и не найдут записи, а это ошибка соединения
						 *
						 * @param sid идентификатор потока
						 *
						 * \~english
						 * @brief Method of the rollback of the last encoded section of a stream
						 * @details It is called when an encoded section has never been sent:
						 *          its references are removed, while the previous sections of the stream remain on the account.
						 *          To cancel the whole stream here is impossible - the confirmations of the already sent
						 *          sections will come and will not find the records, and this is an error of the connection
						 * @param sid identifier of the stream
						 *
						 * \~
						 */
						void rollback(const uint64_t sid) noexcept;
					public:
						/**
						 * \~russian
						 * @brief Метод получения накопленных инструкций потока кодера
						 *
						 * @return представление накопленных инструкций
						 *
						 * \~english
						 * @brief Method of getting the accumulated instructions of the stream of the encoder
						 * @return representation of the accumulated instructions
						 *
						 * \~
						 */
						string_view pending() const noexcept;
						/**
						 * \~russian
						 * @brief Метод отметки инструкций потока кодера как отправленных
						 *
						 * @param size количество отправленных октетов
						 *
						 * \~english
						 * @brief Method of marking the instructions of the stream of the encoder as sent
						 * @param size number of the sent octets
						 *
						 * \~
						 */
						void consumePending(const size_t size) noexcept;
					public:
						/**
						 * \~russian
						 * @brief Метод сброса состояния кодера
						 *
						 * \~english
						 * @brief Method of the reset of the state of the encoder
						 *
						 * \~
						 */
						void clear() noexcept;
					public:
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * @param maxCapacity верхняя граница ёмкости динамической таблицы
						 * @param maxBlocked  число потоков, которым разрешено ожидать пополнения таблицы
						 *
						 * \~english
						 * @brief Constructor
						 * @param maxCapacity upper boundary of the capacity of the dynamic table
						 * @param maxBlocked  number of the streams which are allowed to wait for a replenishment of the table
						 *
						 * \~
						 */
						explicit Encoder(const uint64_t maxCapacity = 0, const uint64_t maxBlocked = 0) noexcept;
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
		}
	};
};

#endif // __AWH_HTTP_PARSER_HTTP3_QPACK__
