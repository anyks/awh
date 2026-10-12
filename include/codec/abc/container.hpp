/**
 * @file container.hpp
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
 * @brief Заголовочный файл сборки бинарного контейнера ABC целиком
 *
 * \~english
 * @brief Header file of the assembling of the whole ABC binary container
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
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "chunk.hpp"
#include "index.hpp"
#include "signature.hpp"
#include "value.hpp"
#include "common.hpp"
#include "header.hpp"
#include "writer.hpp"
#include <sys/macro/global.hpp>

/**
 * Подключаем заголовочные файлы проекта
 */

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
			 * @brief Класс сборки контейнера целиком
			 *
			 * @note **О местах отказа, каких проверкою не навести.** Сплошной щуп подменою
			 *       причин прошёл все места отказа сборки порознь 05.09.2026. Шесть мест
			 *       `SIGNING_FAILED` суть ПОСЛЕДНЯЯ РУКА, и разобрано это по коду, а не выведено
			 *       из молчания щупа:
			 *       - четыре из них - отказ дерева свёрток, а дерево отказывает лишь втроём (нет
			 *       модуля шифрования, нет буфера, нулевая длина), здесь же модуль отдан и кадр
			 *       непуст всегда; дорога стережётся двумя проверками порознь -
			 *       `SignatureFixture.MerkleRefusesWithoutItsGear` для внесения кадра и
			 *       `SignatureFixture.MerkleRootWithAnAppendedChunkRefusesWithoutItsGear` для
			 *       сведения с приданным кадром. Прежде звалась одна первая, и довод выходил
			 *       шире правды: трёхдоводного `root` без оснастки не звал никто, что и
			 *       вскрыл щуп нужности 07.09.2026;
			 *       - укладка записи подписи отказывает лишь при подписи свыше `0xFFFF` октетов
			 *       либо корне свыше того же, а ED25519 даёт шестьдесят четыре;
			 *       - обёртка записи кадром отказывает лишь при записи свыше четырёх гигабайт.
			 *       Недостижимо здесь УСЛОВИЕ, а не путь, и это ЗАМЕРЕНО 06.09.2026, а не выведено:
			 *       щуп пути принудил каждый из шести заслонов срабатывать всегда, и всякий покрасил
			 *       от восьми до одиннадцати проверок. Дорога хожена всяким подписанным контейнером
			 *
			 * @note Седьмое место той же причины ОТ НИХ ОТЛИЧНО и слепым не является:
			 *       отказывает сам подписыватель, и достижимо это прямо - имя владельца ключа
			 *       может не иметь вовсе. Закреплено
			 *       `ContainerFixture.SignerRefusalNamesTheSigningCause`. Слипание разных бед под
			 *       одним кодом причины различие это прятало
			 *
			 *
			 * @details Контейнер есть заголовок опознания и череда кадров за ним. Записи
			 *          копятся в памяти и укладываются кадром по достижении порога: сжимать и
			 *          шифровать каждую запись поодиночке не окупается, а кадром - окупается.
			 *
			 * @details **Заголовок кладётся последним, хотя лежит первым.** Длина тела,
			 *          число записей и разряды свойств известны лишь по завершении сборки, и ставить
			 *          их наперёд значило бы возвращаться к заголовку правкой.
			 *
			 * @note Смена вида содержимого укладывает накопленное кадром. Иначе кадр вышел
			 *       бы вперемешку, и подбор метода сжатия под вид содержимого потерял бы смысл
			 *
			 * @note **Отказ укладки накопленного не сбрасывает.** Место на носителе может
			 *       появиться, ключ может быть выставлен, и следующая попытка пройдёт по тем же
			 *       данным. Сброс же данных отказом не лечится ничем
			 *
			 * @warning **Целость содержимого кадров заверяется лишь ПОДПИСЬЮ либо
			 *          ШИФРОВАНИЕМ.** Заголовок опознания несёт контрольную сумму, а кадр — лишь
			 *          объявленные длины: они ловят порчу полей, но не порчу содержимого.
			 *          Развёртка 25.08.2026, порча всякого октета контейнера тремя способами
			 *          (0x01, 0x80, 0xFF), содержимое сличалось с истиной:
			 *
			 * @warning
			 *          уклад кадров        | прогонов | молча неверно
			 *          как есть            |   11 880 |  10 392   (содержимое 10 368, оглавление 24)
			 *          сжатие              |    2 301 |     803   (содержимое   800, оглавление  3)
			 *          шифрование          |    3 057 |       0
			 *          подпись (поверка)   |    2 496 |       0
			 *
			 * @warning Сжатие ловит порчу лишь ОТЧАСТИ — 424 из 1224 порч содержимого, —
			 *          ибо разжатие принимает испорченный поток чаще, чем кажется. Заслоном оно
			 *          не считается
			 *
			 * @warning Порча кадра ОГЛАВЛЕНИЯ выдаётся молча в 24 случаях из 530: строки
			 *          его ничем не заверены, и испорченная строка ведёт выборку на чужие октеты в
			 *          границах тела. Заслон здесь один — подпись либо шифрование
			 *
			 * @warning **Замком работа НЕ защищена: один объект — один поток.** Замок держит
			 *          лишь `Editor` — ему он нужен ради фиксации по сроку своим потоком, — и
			 *          равняться по нему нельзя. Замер 25.08.2026, один `Fetcher` на четыре потока:
			 *          тринадцать донесений TSan и девятнадцать неверно прочитанных записей из
			 *          четырёхсот, молча. Свой объект у всякого потока над ОБЩИМ источником чтения:
			 *          ноль донесений, ноль расхождений — источник читается, а не правится, и делится
			 *          свободно
			 *
			 * @note **О заслонах контейнера, ни разу не сработавших.** Сплошной щуп нужности
			 *       07.09.2026 обесточил порознь все 52 заслона `container`, сохраняя условие
			 *       вычисляемым. Под сторожем 33 места, молчат 18, и молчание разобрано чтением:
			 *       - **укладка накопленного** (`flush`) о трёх местах, **дерево свёрток**
			 *       (`merkle.add`, `merkle.root`), **сборка записи подписи** (`pack`,
			 *       `envelope`) - последние руки: красит их лишь отказ сжатия или шифрования,
			 *       одного вида с прочими по кодеку;
			 *       - **шесть заслонов свободной поверки** затенены соседями, называющими ТУ ЖЕ
			 *       причину, - разобрано при самой `verify` ниже, там же и довод, отчего
			 *       снимать их нельзя;
			 *       - **ужатие буфера подачи** (`_offset > 0`) заслоном НЕ является вовсе: при
			 *       нулевом смещении тело безвредно, и без него буфер лишь растёт, оставаясь
			 *       верным. Щуп обходит всякое `if(`, и не всякое молчащее место есть заслон;
			 *       - **неснятый заголовок** в `complete()` затенён арифметикой: длина тела при
			 *       неснятом заголовке нулевая, и сличение ниже само отвечает ложью;
			 *       - **снятие кадра** (`unpack`) прогон ПОДВЕСИЛО, и это ответ: заслон несёт
			 *       выход из цикла выборки - отказавшая `unpack` смещения не двигает, и без
			 *       него цикл вечен. Судить по повисшему прогону нельзя, разобрано чтением
			 *
			 * \~english
			 * @brief Class of the assembling of the whole container
			 *
			 * @note **Of the failure sites that a check cannot reach.** The full audit by a substitution of the
			 *       causes went over every assembly failure site apart on 2026-09-05. Six sites of `SIGNING_FAILED`
			 *       are the LAST HAND, and this was resolved by the code, not inferred from the silence of the
			 *       probe:
			 *       - four of them are the refusal of the digest tree, while the tree refuses only in three ways
			 *       (no encryption module, no buffer, a zero length), here the module is given and a chunk is never
			 *       empty; the road is guarded by two checks apart - `SignatureFixture.MerkleRefusesWithoutItsGear`
			 *       for the entering of a chunk and
			 *       `SignatureFixture.MerkleRootWithAnAppendedChunkRefusesWithoutItsGear` for the reduction with an
			 *       endowed chunk. Before, one was the first-named, and the argument came out wider than the truth:
			 *       no one called a three-argument `root` without the gear, which the necessity probe of 2026-09-07
			 *       revealed;
			 *       - the laying of a signature record refuses only at a signature above `0xFFFF` octets or a root
			 *       above the same, while ED25519 gives sixty-four;
			 *       - the wrapping of a record by a chunk refuses only at a record above four gigabytes. It is the
			 *       CONDITION that is unreachable here, not the path, and this was MEASURED on 2026-09-06, not
			 *       inferred: a path probe forced each of the six guards to fire always, and each turned from eight
			 *       to eleven checks red. The road is travelled by every signed container
			 *
			 * @note The seventh site of the same cause DIFFERS FROM THEM and is not blind: the signer itself
			 *       refuses, and this is reachable directly - the name of the key owner may be absent entirely. Fixed
			 *       by `ContainerFixture.SignerRefusalNamesTheSigningCause`. The merging of different troubles under
			 *       one cause code hid this difference
			 *
			 * @details A container is an identifying header and a series of chunks after it. The records
			 *          accumulate in the memory and are laid by a chunk upon the reaching of a threshold: to compress and
			 *          to encrypt every record one by one does not pay off, while by a chunk it does.
			 *
			 * @details **The header is laid last although it lies first.** The length of the body,
			 *          the count of the records and the bits of the properties are known only upon the completion of the assembling, and to set
			 *          them beforehand would mean to return to the header by an editing.
			 *
			 * @note A change of the kind of the content lays the accumulated by a chunk. Otherwise the chunk would come out
			 *       of a mixture, and the selection of the method of the compression for the kind of the content would lose its sense
			 *
			 * @note **A failure of the laying does not discard the accumulated.** The space on the medium may
			 *       appear, the key may be set, and the next attempt will pass over the same data. While a discarding
			 *       of the data is not cured by anything
			 *
			 * @warning **The integrity of the content of the chunks is certified only by the SIGNATURE or
			 *          the ENCRYPTION.** The identifying header carries a checksum, while a chunk carries only the
			 *          declared lengths: they catch a corruption of the fields, but not a corruption of the content.
			 *          A sweep of 2026-08-25 over every octet of a container corrupted in three ways (0x01, 0x80,
			 *          0xFF), the content was compared with the truth:
			 *
			 * @warning
			 *          laying of chunks     |   runs    |  silently wrong
			 *          as is                 |  11 880   |  10 392   (content 10 368, index 24)
			 *          compression           |   2 301   |     803   (content   800, index   3)
			 *          encryption            |   3 057   |       0
			 *          signature (checking)  |   2 496   |       0
			 *
			 * @warning The compression catches a corruption only PARTLY - 424 of 1224 corruptions of the content,
			 *          - for an uncompressing accepts a corrupted stream more often than it seems. It is not counted
			 *          as a guard
			 *
			 * @warning A corruption of the INDEX chunk passes silently in 24 cases of 530: its lines are certified
			 *          by nothing, and a corrupted line leads the fetching to foreign octets within the body. There
			 *          is one guard here - a signature or an encryption
			 *
			 * @warning **The work is NOT protected by a lock: one object — one thread.** Only `Editor`
			 *          holds a lock - it needs one for the timed fix by its own thread - and one must not judge
			 *          the others by it. A measurement of 2026-08-25, one `Fetcher` over four threads: thirteen
			 *          reports of TSan and nineteen records of four hundred read wrongly, silently. An own
			 *          object per thread over a SHARED source of the reading: zero reports, zero divergences -
			 *          the source is read, not edited, and is shared freely
			 *
			 * @note **Of the guards of the container, never once triggered.** The full audit of necessity of
			 *       2026-09-07 de-energized all 52 `container` guards apart, keeping the condition computable.
			 *       Under the watch 33 sites, 18 stand silent, and the silence is resolved by reading:
			 *       - **the laying of the accumulated** (`flush`) of three sites, **the digest tree**
			 *       (`merkle.add`, `merkle.root`), **the assembling of a signature record** (`pack`, `envelope`) -
			 *       the last hands: only a failure of the compression or the encryption turns them red, of one kind
			 *       with the others across the codec;
			 *       - **six guards of a free check** are shadowed by neighbours naming the SAME cause - resolved at
			 *       the very `verify` below, there too the argument why they must not be removed;
			 *       - **the shrinking of the submission buffer** (`_offset > 0`) is NOT a guard at all: at a zero
			 *       offset the body is harmless, and without it the buffer only grows, staying correct. The probe
			 *       goes over every `if(`, and not every silent site is a guard;
			 *       - **an unclaimed header** in `complete()` is shadowed by arithmetic: the length of the body at
			 *       an unclaimed header is zero, and the comparison below answers with a falsehood itself;
			 *       - **the taking off of a chunk** (`unpack`) HUNG the run, and that is the answer: the guard
			 *       carries the exit from the fetching loop - a failed `unpack` does not move the offsets, and
			 *       without it the loop is eternal. One cannot judge by a hung run; resolved by reading
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Assembler {
				public:
					/**
					 * \~russian
					 * @brief Настройки сборки контейнера
					 *
					 * \~english
					 * @brief Settings of the assembling of a container
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Settings {
						/**
						 * \~russian
						 * Порог накопления записей, по достижении которого копится кадр
						 *
						 * @note Порог считается по несжатым записям: сколько выйдет после
						 *       сжатия, известно лишь после него, а решать надо до
						 *
						 * \~english
						 * Threshold of the accumulation of the records upon the reaching of which a chunk is laid
						 *
						 * @note The threshold is counted over the uncompressed records: how much will come out after
						 *       the compression is known only after it, while the decision must be made before
						 *
						 * \~
						 */
						size_t block;
						// Признак строгого вида записи собираемых значений
						bool canonical;
						// Признак того, что тело несёт череду документов, а не один
						bool stream;
						/**
						 * \~russian
						 * Признак ведения оглавления собираемого контейнера
						 *
						 * @note Оглавление стоит шестнадцати октетов на запись и окупается
						 *       выборкой по номеру. Контейнеру, читаемому подряд от начала, оно не
						 *       нужно вовсе, оттого и отключаемо
						 *
						 * \~english
						 * Flag of the leading of the index of an assembled container
						 *
						 * @note The index costs sixteen octets per record and pays off by
						 *       the fetching by a number. A container read consecutively from the beginning does not need it
						 *       at all, therefore it is switchable off
						 *
						 * \~
						 */
						bool indexed;
						// Вид содержимого контейнера, ставимый потребителем
						uint32_t content;
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
					// Настройки сборки контейнера
					settings_t _settings;
				private:
					// Заголовок опознания собираемого контейнера
					header_t _header;
				private:
					// Модуль укладки кадра
					packer_t _packer;
				private:
					// Код отказа сборки контейнера
					error_t _error;
				private:
					// Вид содержимого накопленных записей
					payload_t _kind;
				private:
					// Накопленные записи, ещё не уложенные кадром
					vector <uint8_t> _pending;
				private:
					// Уложенные кадры собираемого контейнера
					vector <uint8_t> _body;
				private:
					// Оглавление собираемого контейнера
					index_t _index;
				private:
					/**
					 * \~russian
					 * Строки оглавления накопленных записей
					 *
					 * @note Смещение кадра в теле известно лишь по укладке его, оттого строки
					 *       накопленных записей ведутся отдельно и дополняются им при укладке
					 *
					 * \~english
					 * Rows of the index of the accumulated records
					 *
					 * @note The offset of a chunk in the body is known only upon its laying, therefore the rows
					 *       of the accumulated records are led separately and are supplemented by it at the laying
					 *
					 * \~
					 */
					vector <entry_t> _marks;
				private:
					// Количество уложенных записей
					uint64_t _records;
				private:
					// Порядковый номер следующего кадра
					uint64_t _number;
				private:
					// Признак того, что хоть один кадр вышел сжатым
					bool _compressed;
				private:
					// Дерево свёрток по кадрам собираемого контейнера
					merkle_t _merkle;
				private:
					// Модуль шифрования, отданный для подписи контейнера
					const crypto_t * _signer;
				private:
					// Имя ключа владельца собираемого контейнера
					string _name;
				private:
					// Желаемый вид хэш-суммы подписи владельца
					crypto_t::hash_t _hash;
				private:
					/**
					 * \~russian
					 * @brief Метод объявления отказа сборки контейнера
					 *
					 * @details Донесение идёт отсюда, из единственного места объявления отказа:
					 *          работа отвечает отказом множеством путей, и запись в каждом из них
					 *          разошлась бы с прочими. Отказ, ПРИНЯТЫЙ от нижнего слоя, сюда не идёт -
					 *          тот слой донёс о нём сам, и второе донесение лишь двоило бы записи.
					 *
					 * @param error объявляемый код отказа
					 * @return      признак успешности, всегда ложь
					 *
					 * \~english
					 * @brief Method of the declaration of a failure
					 *
					 * @details The report comes from here, from the single place of the declaration of a failure: the
					 *          work answers with a failure by many roads, and a record at each of them would diverge from the
					 *          others. A failure ACCEPTED from a lower layer does not come here - that layer has reported it
					 *          itself, and a second report would only double the records.
					 *
					 * @param error code of the failure being declared
					 * @return      flag of the success, always false
					 *
					 * \~
					 */
					bool fail(const error_t error) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод установки модуля сжатия
					 *
					 * @param value устанавливаемый модуль сжатия, ноль - снятие модуля
					 *
					 * \~english
					 * @brief Method of the setting of the module of the compression
					 *
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
					 *
					 * @param value module of the encryption being set, zero — removal of the module
					 *
					 * \~
					 */
					void crypto(const crypto_t * value) noexcept;
					/**
					 * \~russian
					 * @brief Метод объявления подписи собираемого контейнера
					 *
					 * @details Подписывается корень дерева свёрток по кадрам, а не поток
					 *          октетов: корень всегда в тридцать два октета, сколь бы ни был велик
					 *          контейнер, и поточная подача подписи не нужна вовсе - а у Ed25519 её и
					 *          нет по устройству схемы.
					 *
					 * @details Вид хэш-суммы подбирается по виду ключа, а поданный служит
					 *          лишь пожеланием: у Ed25519 и ГОСТ хэш-сумма не задаётся вовсе, и
					 *          поданная им обратилась бы в отказ подписи посреди сборки.
					 *
					 * @param crypto модуль шифрования, ноль - снятие подписи
					 * @param name   имя ключа владельца контейнера
					 * @param hash   желаемый вид хэш-суммы подписи
					 *
					 * \~english
					 * @brief Method of the declaration of the signature of an assembled container
					 *
					 * @details The root of the tree of the digests over the chunks is signed rather than the stream
					 *          of the octets: the root is always thirty-two octets however large
					 *          the container may be, and a streaming submission of the signature is not needed at all — while at Ed25519 there is
					 *          none by the structure of the scheme.
					 *
					 * @details The kind of the hash is selected by the kind of the key, while a submitted one serves
					 *          only as a wish: at Ed25519 and GOST the hash is not set at all, and
					 *          one submitted to them would turn into a refusal of the signature in the middle of the assembling.
					 *
					 * @param crypto module of the encryption, zero — removal of the signature
					 * @param name name of the key of the owner of the container
					 * @param hash desired kind of the hash of the signature
					 *
					 * \~
					 */
					void sign(const crypto_t * crypto, const string & name,
					 const crypto_t::hash_t hash = crypto_t::hash_t::SHA256) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод установки признака владельца контейнера
					 *
					 * @param buffer буфер устанавливаемого признака владельца
					 * @param size   размер устанавливаемого признака владельца
					 *
					 * \~english
					 * @brief Method of the setting of the sign of the owner of a container
					 *
					 * @param buffer buffer of the sign of the owner being set
					 * @param size size of the sign of the owner being set
					 *
					 * \~
					 */
					void owner(const void * buffer, const size_t size) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки отпечатка открытого ключа владельца
					 *
					 * @param buffer буфер устанавливаемого отпечатка ключа
					 * @param size   размер устанавливаемого отпечатка ключа
					 *
					 * \~english
					 * @brief Method of the setting of the fingerprint of the public key of the owner
					 *
					 * @param buffer buffer of the fingerprint of the key being set
					 * @param size size of the fingerprint of the key being set
					 *
					 * \~
					 */
					void fingerprint(const void * buffer, const size_t size) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод внесения значения записью контейнера
					 *
					 * @param value вносимое значение
					 * @param kind  вид содержимого вносимой записи
					 * @return      признак успешности внесения
					 *
					 * \~english
					 * @brief Method of the adding of a value as a record of a container
					 *
					 * @param value value being added
					 * @param kind kind of the content of the record being added
					 * @return sign of the success of the adding
					 *
					 * \~
					 */
					[[nodiscard]] bool append(const value_t & value, const payload_t kind = payload_t::MIXED) noexcept;
					/**
					 * \~russian
					 * @brief Метод внесения готовой записи контейнера
					 *
					 * @details Запись подаётся собранной наперёд: контейнеру довольно знать
					 *          её длину, а разбирать её ради внесения незачем.
					 *
					 * @param buffer буфер вносимой записи
					 * @param size   размер вносимой записи
					 * @param kind   вид содержимого вносимой записи
					 * @return       признак успешности внесения
					 *
					 * \~english
					 * @brief Method of the adding of a ready record of a container
					 *
					 * @details The record is submitted assembled beforehand: it is enough for the container to know
					 *          its length, and there is no point in parsing it for the sake of the adding.
					 *
					 * @param buffer buffer of the record being added
					 * @param size size of the record being added
					 * @param kind kind of the content of the record being added
					 * @return sign of the success of the adding
					 *
					 * \~
					 */
					[[nodiscard]] bool append(const void * buffer, const size_t size, const payload_t kind = payload_t::MIXED) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод укладки накопленных записей кадром
					 *
					 * @return признак успешности укладки
					 *
					 * \~english
					 * @brief Method of the laying of the accumulated records by a chunk
					 *
					 * @return sign of the success of the laying
					 *
					 * \~
					 */
					[[nodiscard]] bool flush() noexcept;
					/**
					 * \~russian
					 * @brief Метод завершения сборки контейнера
					 *
					 * @param result буфер, куда следует уложить собранный контейнер
					 * @return       признак успешности сборки
					 *
					 * \~english
					 * @brief Method of the completion of the assembling of a container
					 *
					 * @param result buffer the assembled container should be laid into
					 * @return sign of the success of the assembling
					 *
					 * \~
					 */
					[[nodiscard]] bool complete(vector <uint8_t> & result) noexcept;
					/**
					 * \~russian
					 * @brief Метод сброса состояния сборки контейнера
					 *
					 *
					 * \~english
					 * @brief Method of the reset of the state of the assembling of a container
					 *
					 * \~
					 */
					void reset() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения количества уложенных записей
					 *
					 * @return количество уложенных записей
					 *
					 * \~english
					 * @brief Method of the extraction of the count of the laid records
					 *
					 * @return count of the laid records
					 *
					 * \~
					 */
					uint64_t records() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения размера накопленных записей
					 *
					 * @return размер накопленных записей в октетах
					 *
					 * \~english
					 * @brief Method of the extraction of the size of the accumulated records
					 *
					 * @return size of the accumulated records in octets
					 *
					 * \~
					 */
					size_t pending() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения оглавления собираемого контейнера
					 *
					 * @return оглавление собираемого контейнера
					 *
					 * \~english
					 * @brief Method of the extraction of the index of an assembled container
					 *
					 * @return index of the assembled container
					 *
					 * \~
					 */
					const index_t & index() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения кода отказа сборки контейнера
					 *
					 * @return код отказа
					 *
					 * \~english
					 * @brief Method of the extraction of the error code of the assembling of a container
					 *
					 * @return error code
					 *
					 * \~
					 */
					error_t error() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения модуля укладки кадра
					 *
					 * @return модуль укладки кадра
					 *
					 * \~english
					 * @brief Method of the extraction of the module of the laying of a chunk
					 *
					 * @return module of the laying of a chunk
					 *
					 * \~
					 */
					packer_t & packer() noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения настроек сборки контейнера
					 *
					 * @return настройки сборки контейнера
					 *
					 * \~english
					 * @brief Method of the extraction of the settings of the assembling of a container
					 *
					 * @return settings of the assembling of a container
					 *
					 * \~
					 */
					const settings_t & settings() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки настроек сборки контейнера
					 *
					 * @param settings устанавливаемые настройки сборки контейнера
					 *
					 * \~english
					 * @brief Method of the setting of the settings of the assembling of a container
					 *
					 * @param settings settings of the assembling of a container being set
					 *
					 * \~
					 */
					void settings(const settings_t & settings) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Assembler() noexcept;
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
					~Assembler() noexcept {}
			} assembler_t;

			/**
			 * \~russian
			 * @brief Класс поточного снятия контейнера
			 *
			 * @details Снимающий ведает заголовком и чередой кадров, но не записями в них:
			 *          содержимое кадра он выдаёт наружу целиком, а разбор его ведёт разбиратель
			 *          записи. Смешение этих слоёв обязало бы держать разбиратель ради одного
			 *          пересчёта кадров.
			 *
			 * @note Октеты подаются как придут: кадр, поданный не целиком, дожидается
			 *       недостающих октетов, а смещение при этом не сдвигается
			 *
			 * @warning **Замком работа НЕ защищена: один объект — один поток.** Замок держит
			 *          лишь `Editor` — ему он нужен ради фиксации по сроку своим потоком, — и
			 *          равняться по нему нельзя. Замер 25.08.2026, один `Fetcher` на четыре потока:
			 *          тринадцать донесений TSan и девятнадцать неверно прочитанных записей из
			 *          четырёхсот, молча. Свой объект у всякого потока над ОБЩИМ источником чтения:
			 *          ноль донесений, ноль расхождений — источник читается, а не правится, и делится
			 *          свободно
			 *
			 * \~english
			 * @brief Class of the streaming taking of a container
			 *
			 * @details The taker deals with the header and the series of the chunks but not with the records in them:
			 *          it issues the content of a chunk outwards as a whole, while its parsing is conducted by the parser
			 *          of a record. A mixing of these layers would oblige one to hold a parser for the sake of a mere
			 *          counting of the chunks.
			 *
			 * @note The octets are submitted as they come: a chunk submitted not in full waits for
			 *       the missing octets, while the offset is not shifted at that
			 *
			 * @warning **The work is NOT protected by a lock: one object — one thread.** Only `Editor`
			 *          holds a lock - it needs one for the timed fix by its own thread - and one must not judge
			 *          the others by it. A measurement of 2026-08-25, one `Fetcher` over four threads: thirteen
			 *          reports of TSan and nineteen records of four hundred read wrongly, silently. An own
			 *          object per thread over a SHARED source of the reading: zero reports, zero divergences -
			 *          the source is read, not edited, and is shared freely
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Loader {
				private:
					// Снятый заголовок опознания контейнера
					header_t _header;
				private:
					// Модуль снятия кадра
					packer_t _packer;
				private:
					// Код отказа снятия контейнера
					error_t _error;
				private:
					// Признак снятого заголовка опознания контейнера
					bool _ready;
				private:
					// Смещение разбора в буфере поданных октетов
					size_t _offset;
				private:
					/**
					 * \~russian
					 * Количество октетов, отброшенных из буфера подачи
					 *
					 * @note Буфер ужимается по выдаче кадра, оттого смещение разбора в нём
					 *       не годится счётом вычитанного тела - для того ведётся отброшенное
					 *
					 * \~english
					 * Count of the octets discarded from the buffer of the submission
					 *
					 * @note The buffer is compacted upon the issuing of a chunk, therefore the offset of the parsing in it
					 *       is not fit as the count of the read body — for that the discarded is counted
					 *
					 * \~
					 */
					size_t _origin;
				private:
					// Буфер поданных октетов
					vector <uint8_t> _buffer;
				private:
					/**
					 * \~russian
					 * @brief Метод объявления отказа снятия контейнера
					 *
					 * @details Донесение идёт отсюда, из единственного места объявления отказа:
					 *          работа отвечает отказом множеством путей, и запись в каждом из них
					 *          разошлась бы с прочими. Отказ, ПРИНЯТЫЙ от нижнего слоя, сюда не идёт -
					 *          тот слой донёс о нём сам, и второе донесение лишь двоило бы записи.
					 *
					 * @param error объявляемый код отказа
					 * @return      признак успешности, всегда ложь
					 *
					 * \~english
					 * @brief Method of the declaration of a failure
					 *
					 * @details The report comes from here, from the single place of the declaration of a failure: the
					 *          work answers with a failure by many roads, and a record at each of them would diverge from the
					 *          others. A failure ACCEPTED from a lower layer does not come here - that layer has reported it
					 *          itself, and a second report would only double the records.
					 *
					 * @param error code of the failure being declared
					 * @return      flag of the success, always false
					 *
					 * \~
					 */
					bool fail(const error_t error) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод установки модуля сжатия
					 *
					 * @param value устанавливаемый модуль сжатия, ноль - снятие модуля
					 *
					 * \~english
					 * @brief Method of the setting of the module of the compression
					 *
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
					 *
					 * @param value module of the encryption being set, zero — removal of the module
					 *
					 * \~
					 */
					void crypto(const crypto_t * value) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод подачи октетов контейнера
					 *
					 * @param buffer буфер подаваемых октетов
					 * @param size   размер подаваемых октетов
					 * @return       признак успешности подачи
					 *
					 * \~english
					 * @brief Method of the submission of the octets of a container
					 *
					 * @param buffer buffer of the octets being submitted
					 * @param size size of the octets being submitted
					 * @return sign of the success of the submission
					 *
					 * \~
					 */
					[[nodiscard]] bool feed(const void * buffer, const size_t size) noexcept;
					/**
					 * \~russian
					 * @brief Метод выдачи содержимого очередного кадра
					 *
					 * @note Отсутствие кадра НЕ означает отказа: кадр вправе быть ещё не подан
					 *       целиком. Отличаются два эти положения кодом отказа - `TRUNCATED_HEADER` и
					 *       `TRUNCATED_CHUNK` значат «подай ещё октетов», а не «всё пропало»; при
					 *       `NONE` кадры кончились. Снятие продолжается подачей и новым вызовом,
					 *       сбрасывать сниматель при этом НЕ нужно
					 *
					 * @note Конец тела опознаётся `complete()`, а не концом кадров: за телом
					 *       лежат оглавление, запись подписи и хвостовой заголовок
					 *
					 * @param result буфер, куда следует положить содержимое кадра
					 * @param chunk  снятые сведения о кадре
					 * @return       признак выданного кадра
					 *
					 * \~english
					 * @brief Method of the issuing of the content of the next chunk
					 *
					 * @note The absence of a chunk does NOT mean a refusal: the chunk may be not yet
					 *       submitted in full. The codes `TRUNCATED_HEADER` and `TRUNCATED_CHUNK` mean
					 *       "submit more octets" rather than "all is lost"; at `NONE` the chunks are over
					 *
					 * @note The end of the body is recognised by `complete()` rather than by the end of the chunks
					 *
					 * @param result buffer the content of the chunk should be placed into
					 * @param chunk taken information about the chunk
					 * @return sign of an issued chunk
					 *
					 * \~
					 */
					[[nodiscard]] bool next(vector <uint8_t> & result, chunk_t & chunk) noexcept;
					/**
					 * \~russian
					 * @brief Метод сброса состояния снятия контейнера
					 *
					 *
					 * \~english
					 * @brief Method of the reset of the state of the taking of a container
					 *
					 * \~
					 */
					void reset() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод проверки снятости заголовка опознания контейнера
					 *
					 * @return признак снятого заголовка
					 *
					 * \~english
					 * @brief Method of the checking of the taking of the identifying header of a container
					 *
					 * @return sign of a taken header
					 *
					 * \~
					 */
					[[nodiscard]] bool ready() const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки исчерпанности тела контейнера
					 *
					 * @details Заголовок объявляет длину тела наперёд, и по ней усечённый
					 *          контейнер отличается от целого. Без этой поверки подрядный съём выдаёт
					 *          кадры до тех, каких недостаёт, и молчит: оборвавшаяся подача с виду
					 *          ничем не отличается от завершённой.
					 *
					 * @note Поверяется тело: оглавление, запись подписи и хвостовой заголовок
					 *       лежат за ним, и подрядному съёму они не выдаются
					 *
					 * @return признак того, что тело контейнера подано целиком
					 *
					 * \~english
					 * @brief Method of the checking of the exhaustion of the body of a container
					 *
					 * @details The header declares the length of the body in advance, and by it a truncated
					 *          container differs from a whole one. Without this check a consecutive fetching issues the
					 *          chunks up to the missing ones and is silent: a broken submission in appearance differs from a
					 *          completed one by nothing.
					 *
					 * @note The body is checked: the index, the record of the signature and the tail header
					 *       lie beyond it
					 *
					 * @return sign that the body of the container has been submitted in full
					 *
					 * \~
					 */
					[[nodiscard]] bool complete() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения снятого заголовка опознания контейнера
					 *
					 * @return снятый заголовок опознания
					 *
					 * \~english
					 * @brief Method of the extraction of the taken identifying header of a container
					 *
					 * @return taken identifying header
					 *
					 * \~
					 */
					const header_t & header() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения кода отказа снятия контейнера
					 *
					 * @return код отказа
					 *
					 * \~english
					 * @brief Method of the extraction of the error code of the taking of a container
					 *
					 * @return error code
					 *
					 * \~
					 */
					error_t error() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения модуля снятия кадра
					 *
					 * @return модуль снятия кадра
					 *
					 * \~english
					 * @brief Method of the extraction of the module of the taking of a chunk
					 *
					 * @return module of the taking of a chunk
					 *
					 * \~
					 */
					packer_t & packer() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Loader() noexcept;
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
					~Loader() noexcept {}
			} loader_t;

			/**
			 * \~russian
			 * @brief Функция поверки подписи владельца контейнера
			 *
			 * @details Поверка идёт по октетам, как они лежат: кадры сводятся в дерево тем
			 *          видом, каким записаны, а сходится ли корень с подписанным, решает подпись.
			 *          **Ключа расшифровки поверка не требует** - подписан шифротекст, и открывать
			 *          его ради поверки незачем.
			 *
			 * @note **Шесть заслонов поверки затенены соседями, называющими ТУ ЖЕ причину.**
			 *       Установлено сплошным щупом нужности 07.09.2026: обесточение любого из них
			 *       не красит ни одной проверки, хотя `VerifyRefusesBrokenLayout` берёт все
			 *       четыре вида порчи и утверждает причину каждого отказа. Затенение разобрано
			 *       чтением, и заслоны оставлены НАМЕРЕННО:
			 *
			 * - пустые октеты (`INTERNAL`): без заслона `nullptr` уходит в `header.unpack`,
			 *   и та отвечает отказом сама - причина совпадает, а проверка устоит;
			 * - место подписи за октетами (`TRUNCATED_SIGNATURE`): без заслона перебор
			 *   кадров доходит до конца тела, и то же имя отказа приходит от сличения
			 *   `signature + CHUNK_HEADER > size` ниже по течению;
			 * - недостача октетов на заголовок кадра (`TRUNCATED_CHUNK`): затенение здесь
			 *   ДОКАЗУЕМО арифметикой - при `offset + CHUNK_HEADER > signature` условие
			 *   соседа `offset + CHUNK_HEADER + length > signature` истинно ВСЕГДА, ибо
			 *   длина неотрицательна, а переполнения нет: она собирается из четырёх
			 *   октетов и не превосходит `2^32 - 1`;
			 * - отказ дерева свёрток (`SIGNING_FAILED`, дважды): заслоны последней руки,
			 *   одного вида с прочими девятнадцатью по кодеку; красит их лишь отказ
			 *   самого шифрования.
			 *
			 * @warning Затенённость НЕ означает лишности, и снимать их нельзя. Заслон на
			 *          недостаче октетов стережёт ПАМЯТЬ, а не имя отказа: длина кадра собирается
			 *          чтением `octets[offset + 4 + i]`, и при месте подписи, близком к концу
			 *          поданных октетов, эти октеты лежат ЗА БУФЕРОМ. Сосед назовёт ту же причину,
			 *          но лишь ПОСЛЕ чтения за концом. Оттого зрячим этот заслон делает не
			 *          утверждение причины, а прогон под санитайзером (`CMAKE_BUILD_SANITIZE`),
			 *          и до такого прогона его молчание доказательством лишности не считать
			 *
			 * @param crypto модуль шифрования, отданный потребителем
			 * @param name   имя ключа владельца контейнера
			 * @param buffer буфер поданных октетов контейнера
			 * @param size   размер поданных октетов контейнера
			 * @param error  код отказа, если поверка не удалась
			 * @return       признак сошедшейся подписи владельца
			 *
			 * \~english
			 * @brief Function of the checking of the signature of the owner of a container
			 *
			 * @details The checking goes over the octets as they lie: the chunks are reduced into the tree in the kind
			 *          in which they are written, while whether the root agrees with the signed one is decided by the signature.
			 *          **The checking does not require the key of the decryption** — the ciphertext is signed, and to open
			 *          it for the sake of the checking is pointless.
			 *
			 * @note **Six guards of the checking are shadowed by neighbours naming the SAME cause.** Established by
			 *       the full necessity probe of 2026-09-07: de-energizing any of them turns not a single check red,
			 *       although `VerifyRefusesBrokenLayout` takes all four kinds of corruption and asserts the cause of
			 *       each refusal. The shadowing is resolved by reading, and the guards are left DELIBERATELY:
			 *       - empty octets (`INTERNAL`): without a guard `nullptr` goes into `header.unpack`, and that
			 *       answers with a refusal itself - the cause coincides, and the check will stand;
			 *       - the signature place beyond the octets (`TRUNCATED_SIGNATURE`): without a guard the enumeration
			 *       of the chunks reaches the end of the body, and the same name of a refusal comes from the
			 *       comparison `signature + CHUNK_HEADER > size` downstream;
			 *       - the shortage of octets for a chunk header (`TRUNCATED_CHUNK`): the shadowing here is PROVABLE
			 *       by arithmetic - at `offset + CHUNK_HEADER > signature` the neighbour's condition
			 *       `offset + CHUNK_HEADER + length > signature` is true ALWAYS, for the length is non-negative and
			 *       there is no overflow: it is assembled from four octets and does not exceed `2^32 - 1`;
			 *       - the refusal of the digest tree (`SIGNING_FAILED`, twice): the guards of the last hand, of one
			 *       kind with the other nineteen across the codec; only a failure of the encryption itself turns
			 *       them red
			 *
			 * @warning A shadowing does NOT mean superfluity, and they must not be removed. The guard on the
			 *          shortage of octets guards MEMORY, not the name of a refusal: the length of a chunk is
			 *          assembled by reading `octets[offset + 4 + i]`, and at a signature place close to the end of
			 *          the submitted octets these octets lie BEYOND THE BUFFER. A neighbour will name the same cause,
			 *          but only AFTER a reading past the end. Therefore it is not the assertion of a cause but a run
			 *          under the sanitizer (`CMAKE_BUILD_SANITIZE`) that makes this guard sighted, and before such a
			 *          run its silence is not to be counted as proof of superfluity
			 *
			 * @param crypto module of the encryption given by the consumer
			 * @param name name of the key of the owner of the container
			 * @param buffer buffer of the submitted octets of the container
			 * @param size size of the submitted octets of the container
			 * @param error error code if the checking has failed
			 * @return sign of an agreed signature of the owner
			 *
			 * \~
			 */
			[[nodiscard]] __AWH_SHARED_EXPORT__ bool verify(const crypto_t & crypto, const string & name,
			 const void * buffer, const size_t size, error_t & error) noexcept;
		};
	};
};
