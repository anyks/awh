/**
 * @file editor.hpp
 * @date 2026-08-19
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
 * @brief Заголовочный файл правки бинарного контейнера ABC на месте
 *
 * \~english
 * @brief Header file of the editing of the ABC binary container in place
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_ABC_EDITOR__
#define __AWH_CODEC_ABC_EDITOR__

/**
 * Стандартные заголовочные файлы
 */
#include <mutex>
#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <functional>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "chunk.hpp"
#include "index.hpp"
#include "schedule.hpp"
#include "signature.hpp"
#include "value.hpp"
#include "common.hpp"
#include "header.hpp"

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
			 * @brief Класс правки контейнера на месте
			 *
			 * @details Правка ведётся дописыванием, а не перезаписью: новая запись ложится
			 * в конец, а строка оглавления перенаправляется на неё. Прежние октеты
			 * остаются на носителе мусором до уборки. Перезапись же обязала бы двигать всё,
			 * что лежит за правимой записью, а контейнер тем и ценен, что велик
			 *
			 * @details **Правки копятся в памяти и ложатся на носитель фиксацией.** Иначе
			 * всякая правка стоила бы сжатия, шифрования и перезаписи оглавления, а
			 * фиксацией они платятся однажды за целую пачку
			 *
			 * @note **Отказ фиксации накопленного не сбрасывает.** Место на носителе может
			 * появиться, и следующая фиксация пройдёт по тем же данным
			 *
			 * @note Накопленное читается наравне с закреплённым: данные уже в памяти, и
			 * отказывать в них до фиксации значило бы обязать потребителя держать их
			 * вторым списком
			 *
			 * @note Головной заголовок правится последним, а перед ним ложится хвостовой:
			 * обрыв посреди фиксации оставит контейнер прежнего поколения, а не битым
			 *
			 * \~english
			 * @brief Class of the editing of a container in place
			 * @details The editing is led by an appending rather than by a rewriting: a new record lies
			 * at the end, while the row of the index is redirected to it. The former octets
			 * remain on the medium as a waste until a compaction. A rewriting would oblige one to move everything
			 * that lies after the record being edited, while a container is valuable exactly by being large
			 * @details **The edits accumulate in the memory and lie on the medium by a commit.** Otherwise
			 * every edit would cost a compression, an encryption and a rewriting of the index, while by
			 * a commit they are paid once for a whole batch
			 * @note **A failure of the commit does not discard the accumulated.** The space on the medium may
			 * appear, and the next commit will pass over the same data
			 * @note The accumulated is read on a par with the committed: the data is already in the memory, and
			 * to refuse it before the commit would mean to oblige the consumer to hold it
			 * as a second list
			 * @note The head header is edited last, and before it the tail one is laid:
			 * an interruption in the middle of a commit will leave the container of the previous generation rather than broken
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Editor {
				public:
					/**
					 * \~russian
					 * @brief Работа чтения октетов контейнера
					 *
					 * \~english
					 * @brief Work of the reading of the octets of a container
					 *
					 * \~
					 */
					typedef function <bool (const uint64_t, const size_t, vector <uint8_t> &)> source_t;
					/**
					 * \~russian
					 * @brief Работа записи октетов контейнера
					 *
					 * \~english
					 * @brief Work of the writing of the octets of a container
					 *
					 * \~
					 */
					typedef function <bool (const uint64_t, const void *, const size_t)> sink_t;
					/**
					 * \~russian
					 * @brief Способы фиксации накопленных правок
					 *
					 * @details Способ по сроку здесь не объявлен намеренно: срок отбивается
					 * не кодеком, а либо юнитом таймера сетевого движка, либо своим потоком,
					 * и подаётся сюда обычной ручной фиксацией
					 *
					 * \~english
					 * @brief Modes of the commit of the accumulated edits
					 * @details The mode by a deadline is not declared here deliberately: the deadline is beaten out
					 * not by the codec but either by the unit of the timer of the network engine or by an own thread,
					 * and is submitted here by an ordinary manual commit
					 *
					 * \~
					 */
					enum class mode_t : uint8_t {
						MANUAL   = 0x00, // Фиксация лишь по требованию потребителя
						SIZE     = 0x01, // Фиксация по размеру накопленных правок
						RECORDS  = 0x02, // Фиксация по количеству накопленных правок
						DEADLINE = 0x03, // Фиксация по сроку, поверяемому при обращении
						THREAD   = 0x04  // Фиксация по сроку, отбиваемому своим потоком
					};
					/**
					 * \~russian
					 * @brief Настройки правки контейнера
					 *
					 * \~english
					 * @brief Settings of the editing of a container
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Settings {
						// Способ фиксации накопленных правок
						mode_t mode;
						// Порог накопления записей, по достижении какого копится кадр
						size_t block;
						/**
						 * \~russian
						 * Порог самочинной фиксации: октеты при способе по размеру и
						 * количество правок при способе по количеству
						 *
						 * \~english
						 * Threshold of the self-willed commit: octets at the mode by a size and
						 * count of the edits at the mode by a count
						 *
						 * \~
						 */
						size_t limit;
						/**
						 * \~russian
						 * Срок самочинной фиксации в миллисекундах
						 *
						 * @note Срок этот берётся лишь способами фиксации по сроку. Где
						 * поднят сетевой движок, срок надлежит отбивать юнитом таймера его, а
						 * сюда приводить обычной ручной фиксацией: свой поток был бы вторым
						 * сроком при одном уже отбиваемом
						 *
						 * \~english
						 * Deadline of the self-willed commit in milliseconds
						 * @note This deadline is taken only by the modes of the commit by a deadline. Where
						 * the network engine is raised, the deadline should be beaten out by the unit of its timer, while
						 * here it should be led by an ordinary manual commit: an own thread would be a second
						 * deadline while one is already being beaten out
						 *
						 * \~
						 */
						uint32_t delay;
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
					 * @brief Накопленная правка оглавления
					 *
					 * \~english
					 * @brief Accumulated edit of the index
					 *
					 * \~
					 */
					typedef struct Edit {
						// Строка оглавления накопленной записи
						entry_t entry;
						// Номер правимой строки оглавления
						uint64_t number;
						// Признак того, что запись вносится, а не правится
						bool added;
						/**
						 * \~russian
						 * Номер уложенного кадра, несущего запись, либо предел размера,
						 * если запись ещё копится и кадром не уложена
						 *
						 * \~english
						 * Number of the laid chunk carrying the record, or the limit of the size
						 * if the record is still accumulating and is not laid by a chunk
						 *
						 * \~
						 */
						size_t batch;
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
						Edit() noexcept : number(0), added(true), batch(0) {}
					} edit_t;
				private:
					/**
					 * \~russian
					 * Замок состояния правки контейнера
					 *
					 * @note Замок возвратный намеренно: самочинная фиксация зовётся изнутри
					 * внесения записи, и невозвратный замок затянул бы сам себя
					 *
					 * @note Замок этот берётся всяким обращением, а не одним лишь способом
					 * фиксации своим потоком: замок, берущийся выборочно, разошёлся бы с
					 * настройкой, смененной на ходу, и разошёлся бы молча
					 *
					 * \~english
					 * Lock of the state of the editing of a container
					 * @note The lock is recursive deliberately: the self-willed commit is called from within
					 * the adding of a record, and a non-recursive lock would deadlock itself
					 * @note This lock is taken by every appeal rather than by the mode of the commit
					 * by an own thread alone: a lock taken selectively would diverge from
					 * a setting changed on the fly, and would diverge silently
					 *
					 * \~
					 */
					mutable recursive_mutex _mtx;
				private:
					// Отбой срока самочинной фиксации
					schedule_t _schedule;
				private:
					// Дерево свёрток по кадрам правимого контейнера
					merkle_t _merkle;
				private:
					// Модуль шифрования, отданный для подписи контейнера
					const crypto_t * _signer;
				private:
					// Имя ключа владельца правимого контейнера
					string _name;
				private:
					// Желаемый вид хэш-суммы подписи владельца
					crypto_t::hash_t _hash;
				private:
					// Заголовок опознания правимого контейнера
					header_t _header;
				private:
					// Оглавление правимого контейнера
					index_t _index;
				private:
					// Модуль укладки и снятия кадра
					packer_t _packer;
				private:
					// Настройки правки контейнера
					settings_t _settings;
				private:
					// Код отказа правки контейнера
					error_t _error;
				private:
					// Признак открытого контейнера
					bool _opened;
				private:
					// Полная длина правимого контейнера на носителе
					uint64_t _length;
				private:
					// Количество октетов, обращённых правкой в мусор
					uint64_t _garbage;
				private:
					// Порядковый номер следующего кадра
					uint64_t _number;
				private:
					// Вид содержимого накопленных записей
					payload_t _kind;
				private:
					// Накопленные записи, ещё не уложенные кадром
					vector <uint8_t> _pending;
				private:
					// Накопленные правки оглавления
					vector <edit_t> _marks;
				private:
					/**
					 * \~russian
					 * Уложенные кадры, ещё не записанные на носитель
					 *
					 * @note Кадр укладывается по достижении порога либо по смене вида
					 * содержимого, а не при фиксации: тем накопление большой пачки не
					 * оборачивается укладкой её целиком единым мигом
					 *
					 * \~english
					 * Laid chunks not yet written onto the medium
					 * @note A chunk is laid upon the reaching of the threshold or upon a change of the kind
					 * of the content rather than at the commit: by that an accumulation of a large batch does not
					 * turn into a laying of the whole of it at a single moment
					 *
					 * \~
					 */
					vector <vector <uint8_t>> _batches;
				private:
					// Признак наличия хвостового заголовка опознания контейнера
					bool _tailed;
				private:
					// Признак наличия правок, ещё не закреплённых на носителе
					bool _dirty;
				private:
					// Смещение удерживаемого кадра от начала тела контейнера
					uint64_t _origin;
				private:
					// Признак удержания снятого кадра
					bool _cached;
				private:
					// Содержимое удерживаемого кадра
					vector <uint8_t> _chunk;
				private:
					// Работа чтения октетов контейнера
					source_t _source;
				private:
					// Работа записи октетов контейнера
					sink_t _sink;
				private:
					/**
					 * \~russian
					 * @brief Метод снятия кадра контейнера с носителя
					 *
					 * @param origin смещение кадра от начала тела контейнера
					 * @return       признак успешно снятого кадра
					 *
					 * \~english
					 * @brief Method of the taking of a chunk of a container from the medium
					 * @param origin offset of the chunk from the beginning of the body of the container
					 * @return sign of a successfully taken chunk
					 *
					 * \~
					 */
					[[nodiscard]] bool fetch(const uint64_t origin) noexcept;
					/**
					 * \~russian
					 * @brief Метод сбора свёрток по кадрам тела контейнера
					 *
					 * @details Сбор идёт по октетам, как они лежат, и ключа расшифровки не
					 * требует: подписан шифротекст. Идёт он единожды при объявлении подписи,
					 * а далее дерево ведётся дописыванием - иначе всякая фиксация обязала бы
					 * перечитывать контейнер целиком
					 *
					 * @return признак успешного сбора свёрток
					 *
					 * \~english
					 * @brief Method of the gathering of the digests over the chunks of the body of a container
					 * @details The gathering goes over the octets as they lie and does not require the key of the decryption:
					 * the ciphertext is signed. It goes once at the declaration of the signature,
					 * while further the tree is led by an appending — otherwise every commit would oblige one
					 * to reread the whole container
					 * @return sign of a successful gathering of the digests
					 *
					 * \~
					 */
					[[nodiscard]] bool harvest() noexcept;
					/**
					 * \~russian
					 * @brief Метод укладки накопленных записей кадром в память
					 *
					 * @return признак успешности укладки
					 *
					 * \~english
					 * @brief Method of the laying of the accumulated records by a chunk into the memory
					 * @return sign of the success of the laying
					 *
					 * \~
					 */
					[[nodiscard]] bool pack() noexcept;
					/**
					 * \~russian
					 * @brief Метод накопления записи правкой контейнера
					 *
					 * @param buffer буфер накопляемой записи
					 * @param size   размер накопляемой записи
					 * @param kind   вид содержимого накопляемой записи
					 * @param added  признак того, что запись вносится, а не правится
					 * @param number номер правимой строки оглавления
					 * @return       признак успешности накопления
					 *
					 * \~english
					 * @brief Method of the accumulation of a record as an edit of a container
					 * @param buffer buffer of the record being accumulated
					 * @param size size of the record being accumulated
					 * @param kind kind of the content of the record being accumulated
					 * @param added sign that the record is being added rather than edited
					 * @param number number of the row of the index being edited
					 * @return sign of the success of the accumulation
					 *
					 * \~
					 */
					[[nodiscard]] bool add(const void * buffer, const size_t size, const payload_t kind,
					 const bool added, const uint64_t number) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод установки модуля сжатия
					 *
					 * @param value устанавливаемый модуль сжатия, ноль - снятие модуля
					 *
					 * \~english
					 * @brief Method of the setting of the module of the compression
					 * @param value module of the compression being set, zero — removal of the module
					 *
					 * \~
					 */
					void compressor(const compressor::block_t * value) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки модуля шифрования
					 *
					 * @param value устанавливаемый модуль шифрования, ноль - снятие модуля
					 *
					 * \~english
					 * @brief Method of the setting of the module of the encryption
					 * @param value module of the encryption being set, zero — removal of the module
					 *
					 * \~
					 */
					void crypto(const crypto_t * value) noexcept;
					/**
					 * \~russian
					 * @brief Метод объявления подписи правимого контейнера
					 *
					 * @details Подпись объявляется по открытии контейнера, а не до него: сбор
					 * свёрток по кадрам тела требует чтения, а читать до открытия нечего
					 *
					 * @note Всякая фиксация кладёт свою подпись: поколение сменилось, а
					 * подпись прежнего поколения на новое тело не сходится и сходиться не
					 * должна
					 *
					 * @param crypto модуль шифрования, ноль - снятие подписи
					 * @param name   имя ключа владельца контейнера
					 * @param hash   желаемый вид хэш-суммы подписи
					 * @return       признак успешно объявленной подписи
					 *
					 * \~english
					 * @brief Method of the declaration of the signature of an edited container
					 * @details The signature is declared after the opening of the container rather than before it: the gathering
					 * of the digests over the chunks of the body requires a reading, while there is nothing to read before the opening
					 * @note Every commit lays its own signature: the generation has changed, while
					 * the signature of the previous generation does not agree with the new body and must not
					 * agree
					 * @param crypto module of the encryption, zero — removal of the signature
					 * @param name name of the key of the owner of the container
					 * @param hash desired kind of the hash of the signature
					 * @return sign of a successfully declared signature
					 *
					 * \~
					 */
					[[nodiscard]] bool sign(const crypto_t * crypto, const string & name,
					 const crypto_t::hash_t hash = crypto_t::hash_t::SHA256) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод открытия контейнера отданными работами чтения и записи
					 *
					 * @details Заголовок берётся головной, а при негодности его - хвостовой:
					 * негодный головной значит обрыв посреди правки, и контейнер откатывается
					 * к поколению, какое было до неё
					 *
					 * @param source устанавливаемая работа чтения октетов контейнера
					 * @param sink   устанавливаемая работа записи октетов контейнера
					 * @param length полная длина контейнера на носителе
					 * @return       признак успешно открытого контейнера
					 *
					 * \~english
					 * @brief Method of the opening of a container by the given works of the reading and of the writing
					 * @details The header is taken as the head one, and at its unfitness the tail one:
					 * an unfit head one means an interruption in the middle of an editing, and the container is rolled back
					 * to the generation which was before it
					 * @param source work of the reading of the octets of the container being set
					 * @param sink work of the writing of the octets of the container being set
					 * @param length full length of the container on the medium
					 * @return sign of a successfully opened container
					 *
					 * \~
					 */
					[[nodiscard]] bool open(source_t source, sink_t sink, const uint64_t length) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод внесения записи в конец контейнера
					 *
					 * @param buffer буфер вносимой записи
					 * @param size   размер вносимой записи
					 * @param kind   вид содержимого вносимой записи
					 * @return       признак успешности внесения
					 *
					 * \~english
					 * @brief Method of the adding of a record to the end of a container
					 * @param buffer buffer of the record being added
					 * @param size size of the record being added
					 * @param kind kind of the content of the record being added
					 * @return sign of the success of the adding
					 *
					 * \~
					 */
					[[nodiscard]] bool append(const void * buffer, const size_t size, const payload_t kind = payload_t::MIXED) noexcept;
					/**
					 * \~russian
					 * @brief Метод правки записи контейнера по номеру
					 *
					 * @param number порядковый номер правимой записи
					 * @param buffer буфер новой записи
					 * @param size   размер новой записи
					 * @param kind   вид содержимого новой записи
					 * @return       признак успешности правки
					 *
					 * \~english
					 * @brief Method of the editing of a record of a container by its number
					 * @param number ordinal number of the record being edited
					 * @param buffer buffer of the new record
					 * @param size size of the new record
					 * @param kind kind of the content of the new record
					 * @return sign of the success of the editing
					 *
					 * \~
					 */
					[[nodiscard]] bool replace(const uint64_t number, const void * buffer, const size_t size,
					 const payload_t kind = payload_t::MIXED) noexcept;
					/**
					 * \~russian
					 * @brief Метод сноса записи контейнера по номеру
					 *
					 * @details Строка снесённой записи остаётся в оглавлении помеченной, а не
					 * изымается: изъятие сдвинуло бы номера соседей, и всякая ссылка на них
					 * извне обратилась бы в ссылку на чужую запись
					 *
					 * @param number порядковый номер сносимой записи
					 * @return       признак успешности сноса
					 *
					 * \~english
					 * @brief Method of the erasure of a record of a container by its number
					 * @details The row of an erased record remains in the index marked rather than
					 * being removed: a removal would shift the numbers of the neighbours, and every reference to them
					 * from outside would turn into a reference to a foreign record
					 * @param number ordinal number of the record being erased
					 * @return sign of the success of the erasure
					 *
					 * \~
					 */
					[[nodiscard]] bool erase(const uint64_t number) noexcept;
					/**
					 * \~russian
					 * @brief Метод выборки записи контейнера по номеру
					 *
					 * @param number порядковый номер выбираемой записи
					 * @param result буфер, куда следует положить выбранную запись
					 * @return       признак успешно выбранной записи
					 *
					 * \~english
					 * @brief Method of the fetching of a record of a container by its number
					 * @param number ordinal number of the record being fetched
					 * @param result buffer the fetched record should be placed into
					 * @return sign of a successfully fetched record
					 *
					 * \~
					 */
					[[nodiscard]] bool record(const uint64_t number, vector <uint8_t> & result) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод фиксации накопленных правок на носителе
					 *
					 * @return признак успешности фиксации
					 *
					 * \~english
					 * @brief Method of the commit of the accumulated edits onto the medium
					 * @return sign of the success of the commit
					 *
					 * \~
					 */
					[[nodiscard]] bool commit() noexcept;
					/**
					 * \~russian
					 * @brief Метод уборки мусора перестройкой контейнера
					 *
					 * @details Уборка пишет контейнер начисто на отданный носитель, беря лишь
					 * живые записи: правка дописыванием мусор копит, а вернуть место может
					 * лишь перестройка. Правка на месте того не сделает - мусор лежит между
					 * живыми записями, и вычеркнуть его значило бы двигать всё, что за ним
					 *
					 * @details **Строки снесённых записей уборкой сохраняются пустыми.**
					 * Изъятие их сдвинуло бы номера соседей, а номера эти живут и вне
					 * контейнера. Место они занимают лишь строкой оглавления, а не записью
					 *
					 * @note Незакреплённые правки уборка сперва закрепляет: убирать по
					 * оглавлению, разошедшемуся с памятью, значило бы терять накопленное
					 *
					 * @note Вид содержимого записи оглавлением не хранится, оттого уборке он
					 * подаётся снаружи: хранить его строкой значило бы платить за него
					 * всякой записью ради одной лишь уборки
					 *
					 * @param target работа записи октетов убираемого контейнера
					 * @param kind   вид содержимого записей убираемого контейнера
					 * @param length полная длина убранного контейнера
					 * @return       признак успешности уборки
					 *
					 * \~english
					 * @brief Method of the compaction of the waste by a rebuilding of a container
					 * @details The compaction writes a container anew onto a given medium taking only
					 * the live records: an editing by an appending accumulates a waste, while the space can be returned
					 * only by a rebuilding. An editing in place will not do that — the waste lies between
					 * the live records, and to cross it out would mean to move everything that is behind it
					 * @details **The rows of the erased records are preserved empty by the compaction.**
					 * Their removal would shift the numbers of the neighbours, while these numbers live outside
					 * the container as well. They occupy the space only by a row of the index rather than by a record
					 * @note The uncommitted edits are committed by the compaction first: to compact by
					 * an index which has diverged from the memory would mean to lose the accumulated
					 * @note The kind of the content of a record is not stored by the index, therefore it is submitted
					 * to the compaction from outside: to store it by a row would mean to pay for it
					 * at every record for the sake of the compaction alone
					 * @param target work of the writing of the octets of the container being compacted
					 * @param kind kind of the content of the records of the container being compacted
					 * @param length full length of the compacted container
					 * @return sign of the success of the compaction
					 *
					 * \~
					 */
					[[nodiscard]] bool compact(sink_t target, const payload_t kind, uint64_t & length) noexcept;
					/**
					 * \~russian
					 * @brief Метод сброса состояния правки контейнера
					 *
					 *
					 * \~english
					 * @brief Method of the reset of the state of the editing of a container
					 *
					 * \~
					 */
					void reset() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения количества записей контейнера
					 *
					 * @return количество записей контейнера, снесённые в счёт входят
					 *
					 * \~english
					 * @brief Method of the extraction of the count of the records of a container
					 * @return count of the records of the container, the erased ones are included in the count
					 *
					 * \~
					 */
					uint64_t records() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения количества октетов, обращённых в мусор
					 *
					 * @return количество октетов мусора на носителе
					 *
					 * \~english
					 * @brief Method of the extraction of the count of the octets turned into a waste
					 * @return count of the octets of the waste on the medium
					 *
					 * \~
					 */
					uint64_t garbage() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения размера накопленных правок
					 *
					 * @return размер накопленных записей в октетах
					 *
					 * \~english
					 * @brief Method of the extraction of the size of the accumulated edits
					 * @return size of the accumulated records in octets
					 *
					 * \~
					 */
					size_t pending() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения полной длины контейнера на носителе
					 *
					 * @return полная длина контейнера в октетах
					 *
					 * \~english
					 * @brief Method of the extraction of the full length of a container on the medium
					 * @return full length of the container in octets
					 *
					 * \~
					 */
					uint64_t length() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения заголовка опознания правимого контейнера
					 *
					 * @return заголовок опознания контейнера
					 *
					 * \~english
					 * @brief Method of the extraction of the identifying header of an edited container
					 * @return identifying header of the container
					 *
					 * \~
					 */
					const header_t & header() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения оглавления правимого контейнера
					 *
					 * @return оглавление контейнера
					 *
					 * \~english
					 * @brief Method of the extraction of the index of an edited container
					 * @return index of the container
					 *
					 * \~
					 */
					const index_t & index() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения кода отказа правки контейнера
					 *
					 * @return код отказа
					 *
					 * \~english
					 * @brief Method of the extraction of the error code of the editing of a container
					 * @return error code
					 *
					 * \~
					 */
					error_t error() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения модуля укладки и снятия кадра
					 *
					 * @return модуль укладки и снятия кадра
					 *
					 * \~english
					 * @brief Method of the extraction of the module of the laying and of the taking of a chunk
					 * @return module of the laying and of the taking of a chunk
					 *
					 * \~
					 */
					packer_t & packer() noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения настроек правки контейнера
					 *
					 * @return настройки правки контейнера
					 *
					 * \~english
					 * @brief Method of the extraction of the settings of the editing of a container
					 * @return settings of the editing of a container
					 *
					 * \~
					 */
					const settings_t & settings() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки настроек правки контейнера
					 *
					 * @param settings устанавливаемые настройки правки контейнера
					 *
					 * \~english
					 * @brief Method of the setting of the settings of the editing of a container
					 * @param settings settings of the editing of a container being set
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
					Editor() noexcept;
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
					~Editor() noexcept;
			} editor_t;
		};
	};
};

/**
 * Возвращаем снятые ранее макросы
 */
#include "../../sys/macro_pop.hpp"

#endif // __AWH_CODEC_ABC_EDITOR__
