/**
 * @file writer.hpp
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
 * @brief Заголовочный файл сборки записи бинарного контейнера ABC
 *
 * \~english
 * @brief Header file of the assembling of a record of the ABC binary container
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_ABC_WRITER__
#define __AWH_CODEC_ABC_WRITER__

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <string>
#include <cstdint>
#include <cstddef>
#include <string_view>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"
#include "encoding.hpp"

/**
 * Снимаем на время объявлений макросы, чьи имена заняты
 */
#include "../../sys/macro_push.hpp"

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
		 * \~english
		 * @brief ABC binary container namespace
		 *
		 * \~
		 */
		namespace abc {
			/**
			 * \~russian
			 * @brief Класс сборки бинарной записи
			 *
			 * @details Сборка ведёт учёт вместимых сама: значение сверх объявленной длины,
			 * незакрытое вместимое и вместимое именем поля отвергаются на месте, а не
			 * оставляются на разбор
			 *
			 * @details **Строгий вид записи.** Настройка эта обязывает объявлять длины
			 * вместимых и вести имена полей по возрастанию. Нужна она там, где запись
			 * подписывается: одинаковое содержимое обязано давать одинаковые октеты, иначе
			 * подпись при пересборке перестаёт совпадать
			 *
			 * \~english
			 * @brief Class of the assembling of a binary record
			 * @details The assembling keeps the account of the containers itself: a value beyond the declared length,
			 * an unclosed container and a container as the name of a field are rejected on the spot rather
			 * than left to the parsing
			 * @details **Strict kind of the record.** This setting obliges to declare the lengths
			 * of the containers and to lead the names of the fields in the ascending order. It is needed where a record
			 * is signed: an identical content must give identical octets, otherwise
			 * the signature ceases to coincide at a reassembling
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Writer {
				public:
					/**
					 * \~russian
					 * @brief Настройки сборки записи
					 *
					 * \~english
					 * @brief Settings of the assembling of a record
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Settings {
						/**
						 * \~russian
						 * Признак строгого вида записи
						 *
						 * @note Строгий вид отвергает неопределённую длину вместимого и
						 * обязывает вести имена полей отображения по возрастанию их записи
						 *
						 * \~english
						 * Flag of the strict kind of the record
						 * @note The strict kind rejects an indefinite length of a container and
						 * obliges to lead the names of the fields of a mapping in the ascending order of their record
						 *
						 * \~
						 */
						bool canonical;
						// Признак проверки строк на соответствие кодировке UTF-8
						bool validate;
						// Признак отказа на повторяющееся имя поля отображения
						bool duplicates;
						// Наибольшая допустимая глубина вложенности, ноль - предел модуля
						uint32_t maxDepth;
						/**
						 * \~russian
						 * Порог укладки содержимого ссылкой в октетах, ноль - копировать всегда
						 *
						 * @note Настройка эта по умолчанию снята: содержимое, уложенное ссылкой,
						 * обязано пережить выдачу записи, и обязанность эта ложится на потребителя.
						 * Впрягать её всем без спроса нельзя
						 *
						 * \~english
						 * Threshold of the laying of the content by a reference in octets, zero — to copy always
						 * @note This setting is removed by default: the content laid by a reference
						 * is obliged to outlive the issuance of the record, and this obligation lies upon the consumer.
						 * It cannot be harnessed to everyone without asking
						 *
						 * \~
						 */
						size_t reference;
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
						Settings() noexcept;
					} settings_t;
				private:
					/**
					 * \~russian
					 * @brief Звено стека вместимых
					 *
					 * \~english
					 * @brief Link of the stack of the containers
					 *
					 * \~
					 */
					typedef struct Frame {
						// Признак того, что вместимое является отображением
						bool mapping;
						// Признак неопределённой длины вместимого
						bool indefinite;
						// Признак того, что следующим ожидается имя поля
						bool expectKey;
						// Признак того, что имя поля в этом вместимом уже было
						bool marked;
						// Количество оставшихся значений вместимого
						uint64_t remain;
						// Отрезок записи имени поля, уложенного последним
						span_t key;
						/**
						 * \~russian
						 * Вид значения, собираемого кусками, либо UNDEFINED у вместимого
						 *
						 * @note Значение, собираемое кусками, ведётся тем же стеком, что и
						 * вместимые: закрывается оно тем же концом, а куски его не суть
						 * значения вместившего
						 *
						 * \~english
						 * Kind of the value assembled by the chunks, or UNDEFINED at a container
						 * @note A value assembled by the chunks is led by the same stack as the
						 * containers: it is closed by the same end, while its chunks are not
						 * the values of the container
						 *
						 * \~
						 */
						type_t segment;
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
						Frame() noexcept :
						 mapping(false), indefinite(false), expectKey(false), marked(false), remain(0),
						 segment(type_t::UNDEFINED) {}
					} frame_t;
					/**
					 * \~russian
					 * @brief Врезка чужого содержимого в собираемую запись
					 *
					 * @details Врезка хранит место в буфере собираемой записи, куда содержимое
					 * это встаёт, но октетов его не держит: они остаются у потребителя, и
					 * запись выдаётся кусками, не склеиваясь в один буфер
					 *
					 * \~english
					 * @brief Insertion of a foreign content into an assembled record
					 * @details The insertion holds the place in the buffer of the assembled record where
					 * this content stands, but does not hold its octets: they remain at the consumer, and
					 * the record is issued by the pieces without being glued into one buffer
					 *
					 * \~
					 */
					typedef struct Cut {
						// Смещение врезки в буфере собираемой записи
						size_t offset;
						// Буфер октетов врезки
						const void * buffer;
						// Размер октетов врезки
						size_t size;
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * @param offset смещение врезки в буфере собираемой записи
						 * @param buffer буфер октетов врезки
						 * @param size   размер октетов врезки
						 *
						 * \~english
						 * @brief Constructor
						 * @param offset offset of the insertion in the buffer of the assembled record
						 * @param buffer buffer of the octets of the insertion
						 * @param size size of the octets of the insertion
						 *
						 * \~
						 */
						Cut(const size_t offset, const void * buffer, const size_t size) noexcept :
						 offset(offset), buffer(buffer), size(size) {}
					} cut_t;
				private:
					// Настройки сборки записи
					settings_t _settings;
				private:
					// Код отказа сборки
					error_t _error;
				private:
					// Буфер собираемой записи
					/**
					 * \~russian
					 * Буфер собираемой записи
					 *
					 * @note Буфер переменен у постоянного вида: выдача цельной записи вправе
					 * вклеить в него содержимое, уложенное ссылкой, и потребителю от того
					 * ничего не меняется
					 *
					 * \~english
					 * Buffer of the assembled record
					 * @note The buffer is mutable at a constant kind: the issuance of a whole record
					 * has the right to glue into it the content laid by a reference, and for the consumer
					 * nothing changes from that
					 *
					 * \~
					 */
					mutable vector <uint8_t> _record;
				private:
					// Врезки чужого содержимого в собираемую запись
					mutable vector <cut_t> _cuts;
				private:
					/**
					 * \~russian
					 * Стек вместимых сборки
					 *
					 * @note Стек переменен у постоянного вида: вклейка содержимого, уложенного
					 * ссылкой, сдвигает отрезки имён полей, отсчитанные от начала буфера
					 *
					 * \~english
					 * Stack of the containers of the assembling
					 * @note The stack is mutable at a constant kind: the gluing of the content laid
					 * by a reference shifts the segments of the names of the fields counted from the beginning of the buffer
					 *
					 * \~
					 */
					mutable vector <frame_t> _stack;
				private:
					// Признак того, что сборка отвечена отказом
					bool _failed;
				private:
					// Количество собранных документов
					uint32_t _documents;
				private:
					/**
					 * \~russian
					 * @brief Метод объявления отказа сборки
					 *
					 * @param error код отказа сборки
					 * @return      признак успешности сборки, всегда ложь
					 *
					 * \~english
					 * @brief Method of the declaration of a refusal of the assembling
					 * @param error error code of the assembling
					 * @return sign of the success of the assembling, always false
					 *
					 * \~
					 */
					[[nodiscard]] bool fail(const error_t error) noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки места, куда укладывается значение
					 *
					 * @param container признак того, что укладывается вместимое
					 * @return          признак успешности сборки
					 *
					 * \~english
					 * @brief Method of the checking of the place a value is laid into
					 * @param container sign that a container is being laid
					 * @return sign of the success of the assembling
					 *
					 * \~
					 */
					[[nodiscard]] bool prepare(const bool container) noexcept;
					/**
					 * \~russian
					 * @brief Метод учёта уложенного значения
					 *
					 * @param start смещение начала записи уложенного значения
					 * @return      признак успешности сборки
					 *
					 * \~english
					 * @brief Method of the account of a laid value
					 * @param start offset of the beginning of the record of the laid value
					 * @return sign of the success of the assembling
					 *
					 * \~
					 */
					[[nodiscard]] bool account(const size_t start) noexcept;
					/**
					 * \~russian
					 * @brief Метод укладки начала вместимого
					 *
					 * @param mapping    признак того, что вместимое является отображением
					 * @param count      количество значений вместимого
					 * @param indefinite признак неопределённой длины вместимого
					 * @return           признак успешности сборки
					 *
					 * \~english
					 * @brief Method of the laying of the beginning of a container
					 * @param mapping sign that the container is a mapping
					 * @param count number of the values of the container
					 * @param indefinite sign of an indefinite length of the container
					 * @return sign of the success of the assembling
					 *
					 * \~
					 */
					[[nodiscard]] bool open(const bool mapping, const uint64_t count, const bool indefinite) noexcept;
					/**
					 * \~russian
					 * @brief Метод укладки конца вместимого
					 *
					 * @param mapping признак того, что вместимое является отображением
					 * @return        признак успешности сборки
					 *
					 * \~english
					 * @brief Method of the laying of the end of a container
					 * @param mapping sign that the container is a mapping
					 * @return sign of the success of the assembling
					 *
					 * \~
					 */
					[[nodiscard]] bool close(const bool mapping) noexcept;
					/**
					 * \~russian
					 * @brief Метод начала значения, собираемого кусками
					 *
					 * @param string признак того, что собирается строка, а не двоичные данные
					 * @return       признак успешности укладки
					 *
					 * \~english
					 * @brief Method of the beginning of a value assembled by the chunks
					 * @param string sign that a string is being assembled rather than binary data
					 * @return sign of the success of the laying
					 *
					 * \~
					 */
					[[nodiscard]] bool segment(const bool string) noexcept;
					/**
					 * \~russian
					 * @brief Метод конца значения, собираемого кусками
					 *
					 * @param string признак того, что собирается строка, а не двоичные данные
					 * @return       признак успешности укладки
					 *
					 * \~english
					 * @brief Method of the end of a value assembled by the chunks
					 * @param string sign that a string is being assembled rather than binary data
					 * @return sign of the success of the laying
					 *
					 * \~
					 */
					[[nodiscard]] bool segmentEnd(const bool string) noexcept;
					/**
					 * \~russian
					 * @brief Метод вклейки содержимого, уложенного ссылкой
					 *
					 * @details Врезки вклеиваются в буфер собираемой записи, а отрезки имён
					 * полей, уложенных ранее, сдвигаются на вклеенное: смещения их отсчитаны от
					 * начала буфера, и вклейка сдвинула бы их молча
					 *
					 * \~english
					 * @brief Method of the gluing of the content laid by a reference
					 * @details The insertions are glued into the buffer of the assembled record, while the segments of the names
					 * of the fields laid earlier are shifted by the glued: their offsets are counted from
					 * the beginning of the buffer, and the gluing would shift them silently
					 *
					 * \~
					 */
					void flatten() const noexcept;
					/**
					 * \~russian
					 * @brief Метод укладки октетов содержимого значения
					 *
					 * @details Содержимое, чей размер порог укладки ссылкой превысил, ложится
					 * врезкой, а прочее копируется в буфер собираемой записи
					 *
					 * @param buffer буфер укладываемых октетов
					 * @param size   размер укладываемых октетов
					 * @param copy   признак того, что копировать содержимое обязательно
					 *
					 * \~english
					 * @brief Method of the laying of the octets of the content of a value
					 * @details The content whose size has exceeded the threshold of the laying by a reference is laid
					 * by an insertion, while the rest is copied into the buffer of the assembled record
					 * @param buffer buffer of the octets being laid
					 * @param size size of the octets being laid
					 * @param copy sign that the copying of the content is obligatory
					 *
					 * \~
					 */
					void content(const void * buffer, const size_t size, const bool copy) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод укладки пустого значения
					 *
					 * @return признак успешности сборки
					 *
					 * \~english
					 * @brief Method of the laying of an empty value
					 * @return sign of the success of the assembling
					 *
					 * \~
					 */
					[[nodiscard]] bool nul() noexcept;
					/**
					 * \~russian
					 * @brief Метод укладки логического значения
					 *
					 * @param value укладываемое значение
					 * @return      признак успешности сборки
					 *
					 * \~english
					 * @brief Method of the laying of a logical value
					 * @param value value being laid
					 * @return sign of the success of the assembling
					 *
					 * \~
					 */
					[[nodiscard]] bool boolean(const bool value) noexcept;
					/**
					 * \~russian
					 * @brief Метод укладки целого числа со знаком
					 *
					 * @param value укладываемое значение
					 * @return      признак успешности сборки
					 *
					 * \~english
					 * @brief Method of the laying of an integer with a sign
					 * @param value value being laid
					 * @return sign of the success of the assembling
					 *
					 * \~
					 */
					[[nodiscard]] bool number(const int64_t value) noexcept;
					/**
					 * \~russian
					 * @brief Метод укладки целого числа без знака
					 *
					 * @param value укладываемое значение
					 * @return      признак успешности сборки
					 *
					 * \~english
					 * @brief Method of the laying of an integer without a sign
					 * @param value value being laid
					 * @return sign of the success of the assembling
					 *
					 * \~
					 */
					[[nodiscard]] bool number(const uint64_t value) noexcept;
					/**
					 * \~russian
					 * @brief Метод укладки дробного числа двойной точности
					 *
					 * @param value укладываемое значение
					 * @return      признак успешности сборки
					 *
					 * \~english
					 * @brief Method of the laying of a fractional number of a double precision
					 * @param value value being laid
					 * @return sign of the success of the assembling
					 *
					 * \~
					 */
					[[nodiscard]] bool number(const double value) noexcept;
					/**
					 * \~russian
					 * @brief Метод укладки дробного числа одинарной точности
					 *
					 * @param value укладываемое значение
					 * @return      признак успешности сборки
					 *
					 * \~english
					 * @brief Method of the laying of a fractional number of a single precision
					 * @param value value being laid
					 * @return sign of the success of the assembling
					 *
					 * \~
					 */
					[[nodiscard]] bool number(const float value) noexcept;
					/**
					 * \~russian
					 * @brief Метод укладки строки
					 *
					 * @param value укладываемое значение
					 * @return      признак успешности сборки
					 *
					 * \~english
					 * @brief Method of the laying of a string
					 * @param value value being laid
					 * @return sign of the success of the assembling
					 *
					 * \~
					 */
					[[nodiscard]] bool text(const string_view value) noexcept;
					/**
					 * \~russian
					 * @brief Метод укладки двоичных данных
					 *
					 * @param buffer буфер укладываемых данных
					 * @param size   размер укладываемых данных в октетах
					 * @return       признак успешности сборки
					 *
					 * \~english
					 * @brief Method of the laying of binary data
					 * @param buffer buffer of the data being laid
					 * @param size size of the data being laid in octets
					 * @return sign of the success of the assembling
					 *
					 * \~
					 */
					[[nodiscard]] bool blob(const void * buffer, const size_t size) noexcept;
					/**
					 * \~russian
					 * @brief Метод укладки отметки времени
					 *
					 * @param value укладываемое значение
					 * @return      признак успешности сборки
					 *
					 * \~english
					 * @brief Method of the laying of a time stamp
					 * @param value value being laid
					 * @return sign of the success of the assembling
					 *
					 * \~
					 */
					[[nodiscard]] bool timestamp(const int64_t value) noexcept;
					/**
					 * \~russian
					 * @brief Метод укладки опознавателя
					 *
					 * @param buffer буфер укладываемого опознавателя
					 * @param size   размер укладываемого опознавателя в октетах
					 * @return       признак успешности сборки
					 *
					 * \~english
					 * @brief Method of the laying of an identifier
					 * @param buffer buffer of the identifier being laid
					 * @param size size of the identifier being laid in octets
					 * @return sign of the success of the assembling
					 *
					 * \~
					 */
					[[nodiscard]] bool uuid(const void * buffer, const size_t size) noexcept;
					/**
					 * \~russian
					 * @brief Метод укладки целого числа неограниченной ширины
					 *
					 * @details Октеты величины укладываются от младшего к старшему, и старший
					 * октет нулевым быть не вправе: иначе одно и то же число получило бы
					 * несколько записей, и строгий вид стал бы невозможен
					 *
					 * @param buffer   буфер октетов величины
					 * @param size     размер октетов величины
					 * @param negative признак того, что число меньше нуля
					 * @return         признак успешности сборки
					 *
					 * \~english
					 * @brief Method of the laying of an integer of an unlimited width
					 * @details The octets of the magnitude are laid from the low one to the high one, and the high
					 * octet has no right to be a zero one: otherwise one and the same number would receive
					 * several records, and the strict kind would become impossible
					 * @param buffer buffer of the octets of the magnitude
					 * @param size size of the octets of the magnitude
					 * @param negative sign that the number is less than zero
					 * @return sign of the success of the assembling
					 *
					 * \~
					 */
					[[nodiscard]] bool bignum(const void * buffer, const size_t size, const bool negative) noexcept;
					/**
					 * \~russian
					 * @brief Метод укладки десятичного числа
					 *
					 * @note Порядок, равный нулю, укладывается целым неограниченной ширины, а
					 *       не десятичным числом: значение у них одно и то же, а строгий вид
					 *       записи обязывает одному значению отвечать одной записи
					 *
					 * @param buffer   буфер октетов величины
					 * @param size     размер октетов величины
					 * @param negative признак того, что число меньше нуля
					 * @param exponent десятичный порядок величины
					 * @return         признак успешности сборки
					 *
					 * \~english
					 * @brief Method of the laying of a decimal number
					 * @note An exponent equal to zero is laid by an integer of an unlimited width rather
					 *       than by a decimal number: their value is one and the same, while the strict kind
					 *       of the record obliges one record to correspond to one value
					 * @param buffer buffer of the octets of the magnitude
					 * @param size size of the octets of the magnitude
					 * @param negative sign that the number is less than zero
					 * @param exponent decimal exponent of the magnitude
					 * @return sign of the success of the assembling
					 *
					 * \~
					 */
					[[nodiscard]] bool decimal(const void * buffer, const size_t size, const bool negative, const int64_t exponent) noexcept;
					/**
					 * \~russian
					 * @brief Метод укладки открытого расширения
					 *
					 * @details Расширение это заводится потребителем, а не модулем: номер подвида
					 * толкует он сам, а запись несёт его вместе с октетами. Так к записи прибавляется
					 * свой вид значения, и разбору чужой записи он не мешает ничем - разбиратель
					 * выдаёт расширение отдельным событием, не пытаясь его толковать
					 *
					 * @note Номера подвидов модулем не заняты вовсе и потребителю принадлежат
					 * целиком: соглашение о них есть дело того, кто запись пишет и читает
					 *
					 * @param subtype номер подвида расширения, заведённый потребителем
					 * @param buffer  буфер октетов расширения
					 * @param size    размер октетов расширения
					 * @return        признак успешности сборки
					 *
					 * \~english
					 * @brief Method of the laying of an open extension
					 * @details This extension is instituted by the consumer rather than by the module: the number of the subtype
					 * is interpreted by it itself, while the record carries it together with the octets. Thus its own kind
					 * of a value is added to the record, and it does not hinder the parsing of a foreign record in any way — the parser
					 * issues the extension by a separate event without attempting to interpret it
					 * @note The numbers of the subtypes are not occupied by the module at all and belong to the consumer
					 * entirely: the agreement about them is the affair of the one who writes and reads the record
					 * @param subtype number of the subtype of the extension instituted by the consumer
					 * @param buffer buffer of the octets of the extension
					 * @param size size of the octets of the extension
					 * @return sign of the success of the assembling
					 *
					 * \~
					 */
					[[nodiscard]] bool custom(const uint64_t subtype, const void * buffer, const size_t size) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод укладки начала массива объявленной длины
					 *
					 * @param count количество значений массива
					 * @return      признак успешности сборки
					 *
					 * \~english
					 * @brief Method of the laying of the beginning of an array of a declared length
					 * @param count number of the values of the array
					 * @return sign of the success of the assembling
					 *
					 * \~
					 */
					/**
					 * \~russian
					 * @brief Метод начала строки, собираемой кусками
					 *
					 * @details Строка эта кладётся, когда длина её наперёд неизвестна: куски
					 * её ложатся один за другим обыкновенными строками, а концом объявляется
					 * их завершение. Так большой текст уходит в запись потоком, не собираясь
					 * в памяти целиком
					 *
					 * @note Строгий вид записи неопределённую длину отвергает: длина обязана
					 * быть объявлена, иначе запись одного и того же значения выйдет разной
					 *
					 * @return признак успешности укладки
					 *
					 * \~english
					 * @brief Method of the beginning of a string assembled by the chunks
					 * @details This string is laid when its length is not known beforehand: its chunks
					 * lie one after another as ordinary strings, while their completion is declared by the end.
					 * Thus a large text goes into the record by a stream without being assembled
					 * in the memory as a whole
					 * @note The strict kind of the record rejects an indefinite length: the length is obliged
					 * to be declared, otherwise the record of one and the same value will come out different
					 * @return sign of the success of the laying
					 *
					 * \~
					 */
					[[nodiscard]] bool textBegin() noexcept;
					/**
					 * \~russian
					 * @brief Метод конца строки, собираемой кусками
					 *
					 * @return признак успешности укладки
					 *
					 * \~english
					 * @brief Method of the end of a string assembled by the chunks
					 * @return sign of the success of the laying
					 *
					 * \~
					 */
					[[nodiscard]] bool textEnd() noexcept;
					/**
					 * \~russian
					 * @brief Метод начала двоичных данных, собираемых кусками
					 *
					 * @return признак успешности укладки
					 *
					 * \~english
					 * @brief Method of the beginning of binary data assembled by the chunks
					 * @return sign of the success of the laying
					 *
					 * \~
					 */
					[[nodiscard]] bool blobBegin() noexcept;
					/**
					 * \~russian
					 * @brief Метод конца двоичных данных, собираемых кусками
					 *
					 * @return признак успешности укладки
					 *
					 * \~english
					 * @brief Method of the end of binary data assembled by the chunks
					 * @return sign of the success of the laying
					 *
					 * \~
					 */
					[[nodiscard]] bool blobEnd() noexcept;
					[[nodiscard]] bool arrayBegin(const uint64_t count) noexcept;
					/**
					 * \~russian
					 * @brief Метод укладки начала массива неопределённой длины
					 *
					 * @return признак успешности сборки
					 *
					 * \~english
					 * @brief Method of the laying of the beginning of an array of an indefinite length
					 * @return sign of the success of the assembling
					 *
					 * \~
					 */
					[[nodiscard]] bool arrayBegin() noexcept;
					/**
					 * \~russian
					 * @brief Метод укладки конца массива
					 *
					 * @return признак успешности сборки
					 *
					 * \~english
					 * @brief Method of the laying of the end of an array
					 * @return sign of the success of the assembling
					 *
					 * \~
					 */
					[[nodiscard]] bool arrayEnd() noexcept;
					/**
					 * \~russian
					 * @brief Метод укладки начала отображения объявленной длины
					 *
					 * @param count количество пар отображения
					 * @return      признак успешности сборки
					 *
					 * \~english
					 * @brief Method of the laying of the beginning of a mapping of a declared length
					 * @param count number of the pairs of the mapping
					 * @return sign of the success of the assembling
					 *
					 * \~
					 */
					[[nodiscard]] bool mapBegin(const uint64_t count) noexcept;
					/**
					 * \~russian
					 * @brief Метод укладки начала отображения неопределённой длины
					 *
					 * @return признак успешности сборки
					 *
					 * \~english
					 * @brief Method of the laying of the beginning of a mapping of an indefinite length
					 * @return sign of the success of the assembling
					 *
					 * \~
					 */
					[[nodiscard]] bool mapBegin() noexcept;
					/**
					 * \~russian
					 * @brief Метод укладки конца отображения
					 *
					 * @return признак успешности сборки
					 *
					 * \~english
					 * @brief Method of the laying of the end of a mapping
					 * @return sign of the success of the assembling
					 *
					 * \~
					 */
					[[nodiscard]] bool mapEnd() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод сброса состояния сборки
					 *
					 *
					 * \~english
					 * @brief Method of the reset of the state of the assembling
					 *
					 * \~
					 */
					void reset() noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки завершённости собранной записи
					 *
					 * @details Запись завершена, когда все вместимые закрыты и уложен хотя бы
					 * один документ
					 *
					 * @return признак завершённости собранной записи
					 *
					 * \~english
					 * @brief Method of the checking of the completeness of an assembled record
					 * @details A record is completed when all the containers are closed and at least
					 * one document is laid
					 * @return sign of the completeness of the assembled record
					 *
					 * \~
					 */
					[[nodiscard]] bool complete() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения собранной записи
					 *
					 * @return собранная запись
					 *
					 * \~english
					 * @brief Method of the extraction of an assembled record
					 * @return assembled record
					 *
					 * \~
					 */
					const vector <uint8_t> & record() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения собранной записи кусками
					 *
					 * @details Куски эти подаются средству записи россыпью - скажем, вызовом
					 * writev: содержимое, уложенное ссылкой, в буфер записи не копируется вовсе,
					 * и крупные значения уходят наружу, не быв удвоены в памяти
					 *
					 * @note Куски ссылаются на память сборки и на память потребителя: живут они
					 * не дольше и той, и другой
					 *
					 * @return куски собранной записи по порядку
					 *
					 * \~english
					 * @brief Method of the extraction of an assembled record by the pieces
					 * @details These pieces are submitted to the means of the writing scattered — say, by the call of
					 * writev: the content laid by a reference is not copied into the buffer of the record at all,
					 * and the large values go outside without having been doubled in the memory
					 * @note The pieces refer to the memory of the assembling and to the memory of the consumer: they live
					 * no longer than both the one and the other
					 * @return pieces of the assembled record in order
					 *
					 * \~
					 */
					[[nodiscard]] vector <piece_t> pieces() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения длины собранной записи
					 *
					 * @note Длина эта считает и содержимое, уложенное ссылкой, оттого размеру
					 * буфера собираемой записи она равна не всегда
					 *
					 * @return длина собранной записи в октетах
					 *
					 * \~english
					 * @brief Method of the extraction of the length of an assembled record
					 * @note This length counts the content laid by a reference as well, whereby it is not always
					 * equal to the size of the buffer of the assembled record
					 * @return length of the assembled record in octets
					 *
					 * \~
					 */
					size_t length() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения кода отказа сборки
					 *
					 * @return код отказа сборки
					 *
					 * \~english
					 * @brief Method of the extraction of the error code of the assembling
					 * @return error code of the assembling
					 *
					 * \~
					 */
					error_t error() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения глубины вложенности сборки
					 *
					 * @return глубина вложенности сборки
					 *
					 * \~english
					 * @brief Method of the extraction of the depth of the nesting of the assembling
					 * @return depth of the nesting of the assembling
					 *
					 * \~
					 */
					uint32_t depth() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения настроек сборки записи
					 *
					 * @return настройки сборки записи
					 *
					 * \~english
					 * @brief Method of the extraction of the settings of the assembling of a record
					 * @return settings of the assembling of a record
					 *
					 * \~
					 */
					const settings_t & settings() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки настроек сборки записи
					 *
					 * @param settings устанавливаемые настройки сборки записи
					 *
					 * \~english
					 * @brief Method of the setting of the settings of the assembling of a record
					 * @param settings settings of the assembling of a record being set
					 *
					 * \~
					 */
					void settings(const settings_t & settings) noexcept;
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
					Writer() noexcept;
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
					~Writer() noexcept {}
			} writer_t;
		};
	};
};

/**
 * Возвращаем снятые ранее макросы
 */
#include "../../sys/macro_pop.hpp"

#endif // __AWH_CODEC_ABC_WRITER__
