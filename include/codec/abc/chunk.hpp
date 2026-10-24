/**
 * @file chunk.hpp
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
 * @brief Заголовочный файл кадра бинарного контейнера ABC
 *
 * \~english
 * @brief Header file of a chunk of the ABC binary container
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
#include <vector>
#include <cstdint>
#include <cstddef>

/**
 * Подключаем заголовочный файл модуля
 */
#include "common.hpp"

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../compressor/block.hpp"
#include "../../cryptography/crypto.hpp"

/**
 * Подавляем системные макросы, занявшие имена членов перечислений ниже:
 * DELETE и ERROR у MS Windows, CS и PRIVATE у Sun Solaris, CS5 у termios.
 * Имена снимаются лишь на время объявлений - возврат в конце файла
 */
#include "../../sys/macro/suppress.hpp"

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
			 * @brief Длина заголовка кадра в октетах
			 *
			 * @details **Раскладка заголовка кадра по октетам.** Числа уложены от МЛАДШЕГО
			 *          октета к старшему, места запаса заполнены нулями:
			 *
			 * @li `0` (1) - метод сжатия содержимого, величина `compressor::method_t`;
			 *
			 * @li `1` (1) - разряды кадра: `0x01` содержимое зашифровано, `0x02` кадр обращён
			 *     в мусор (`CHUNK_WASTE`), прочие разряды при снятии отвергаются;
			 *
			 * @li `2` (2) - запас;
			 *
			 * @li `4` (4) - длина УЛОЖЕННОГО содержимого, то есть сжатого и зашифрованного;
			 *
			 * @li `8` (4) - длина ИСХОДНОГО содержимого, до сжатия;
			 *
			 * @li `12` (8) - порядковый номер кадра;
			 *
			 * @li `20` (4) - поколение записи кадра;
			 *
			 * @li `24` (8) - контрольная сумма кадра (`CHUNK_DIGEST`)
			 *
			 * @details **Выработка контрольной суммы.** Сумма берётся с заголовка, приведённого
			 *          к виду укладки: разряд мусора снят, восемь октетов самой суммы обнулены. Зерно её
			 *          `0x41424348554E4B01` - своё, отличное от зерна заголовка контейнера. Сначала
			 *          сумма снимается с тридцати двух октетов заголовка, затем ею же, как зерном,
			 *          кроется содержимое кадра. Пустое содержимое второго шага не имеет.
			 *
			 * @note Раскладка эта записана здесь 01.09.2026, а прежде жила ЛИШЬ В РЕАЛИЗАЦИИ:
			 *       наружу были открыты два смещения из семи - `CHUNK_FLAGS` и `CHUNK_DIGEST`, - и
			 *       открыты они были ради правки на месте, а не ради чтения кадра чужой стороной
			 *
			 * @warning Записана она СНЯТИЕМ С РЕАЛИЗАЦИИ, а не наоборот. Опорою для сличения
			 *          служить не может: судья, писанный по такой записи, повторил бы реализацию вместе
			 *          с её ошибками. Опорою она станет, когда переживёт правку реализации, ею не
			 *          подтверждённую
			 *
			 * \~english
			 * @brief Length of the header of a chunk in octets
			 *
			 * @details **The layout of the header of a chunk by octets.** The numbers are laid from the
			 *          least significant octet to the most significant one, and the places of the reserve are
			 *          filled with zeros:
			 *
			 * @li `0` (1) - the method of the compression of the content, the value of `compressor::method_t`;
			 *
			 * @li `1` (1) - the bits of the chunk: `0x01` the content is encrypted, `0x02` the chunk is turned
			 *     into a waste (`CHUNK_WASTE`), the other bits are refused at the taking;
			 *
			 * @li `2` (2) - a reserve;
			 *
			 * @li `4` (4) - the length of the LAID content, that is, of the compressed and encrypted one;
			 *
			 * @li `8` (4) - the length of the ORIGINAL content, before the compression;
			 *
			 * @li `12` (8) - the ordinal number of the chunk;
			 *
			 * @li `20` (4) - the generation of the record of the chunk;
			 *
			 * @li `24` (8) - the checksum of the chunk (`CHUNK_DIGEST`)
			 *
			 * @details **The production of the checksum.** The sum is taken from the header brought to the
			 *          shape of the laying: the bit of the waste is cleared, and the eight octets of the sum
			 *          itself are zeroed. Its seed is `0x41424348554E4B01` - its own, distinct from the seed of
			 *          the header of the container. First the sum is taken from the thirty-two octets of the
			 *          header, then that same sum, as a seed, covers the content of the chunk. An empty content
			 *          has no second step.
			 *
			 * @note This layout was written here on 2026-09-01, while before it lived ONLY IN THE
			 *       IMPLEMENTATION: two offsets out of seven were opened outwards - `CHUNK_FLAGS` and
			 *       `CHUNK_DIGEST` - and opened they were for the sake of an editing on the spot, not for the
			 *       reading of a chunk by a foreign side
			 *
			 * @warning It was written by a REMOVAL FROM THE IMPLEMENTATION, not the other way about. It
			 *          cannot serve as a support for a comparison: a judge written by such a record would
			 *          repeat the implementation together with its mistakes. It will become a support once it
			 *          outlives an editing of the implementation which does not confirm it
			 *
			 * \~
			 */
			constexpr size_t CHUNK_HEADER = 32;

			/**
			 * \~russian
			 * @brief Смещение разрядов свойств в заголовке кадра
			 *
			 * @details Смещение это открыто наружу ради пометки кадра мусором: правка
			 *          одного октета дешевле перезаписи кадра, какая обязала бы шифровать
			 *          содержимое наново, ничего в нём не меняя.
			 *
			 * \~english
			 * @brief Offset of the bits of the properties in the header of a chunk
			 *
			 * @details This offset is opened outwards for the sake of the marking of a chunk as a waste: an editing
			 *          of one octet is cheaper than a rewriting of the chunk which would oblige one to encrypt
			 *          the content anew without changing anything in it.
			 *
			 * \~
			 */
			constexpr size_t CHUNK_FLAGS = 1;

			/**
			 * \~russian
			 * @brief Разряд объявления кадра мусором
			 *
			 * \~english
			 * @brief Bit of the declaration of a chunk as a waste
			 *
			 * \~
			 */
			constexpr uint8_t CHUNK_WASTE = 0x02;

			/**
			 * \~russian
			 * @brief Смещение контрольной суммы в заголовке кадра
			 *
			 * @details Сумма кроет заголовок кадра вместе с содержимым его, а САМА в неё не
			 *          входит. Разряд мусора при выработке снимается: он учётный, метится правкой
			 *          одного октета уже уложенного кадра, и сумма ему следовать не обязана.
			 *
			 * @details Заведена она затем, что кадр иначе ничем не заверен: заголовок
			 *          контейнера свою сумму несёт, а кадр нёс лишь объявленные длины, и порча октета
			 *          внутри кадра проходила МОЛЧА. Развёртка 25.08.2026 дала 24 молчаливо неверных
			 *          чтения из 530 при порче одного лишь кадра оглавления, а он ведёт выборку по
			 *          всему контейнеру.
			 *
			 * \~english
			 * @brief Offset of the checksum in the header of a chunk
			 *
			 * @details The sum covers the header of the chunk together with its content while ITSELF is not
			 *          included in it. The bit of the waste is cleared upon the production: it is an accounting
			 *          one, marked by an editing of one octet of a chunk already laid, and the sum is not
			 *          obliged to follow it.
			 *
			 * @details It has been introduced because otherwise a chunk is certified by nothing: the header of
			 *          the container carries its own sum, while a chunk carried only the declared lengths, and a
			 *          corruption of an octet inside a chunk passed SILENTLY. The sweep of 2026-08-25 gave 24
			 *          silently wrong readings out of 530 upon the corruption of the index chunk alone, while
			 *          that chunk leads a selection over the whole container.
			 *
			 * \~
			 */
			constexpr size_t CHUNK_DIGEST = 24;

			/**
			 * \~russian
			 * @brief Виды содержимого кадра
			 *
			 * @details Вид содержимого выбирает метод сжатия: у знакового текста он один, у
			 *          сырых октетов иной, у однородных чисел третий. Подбор этот стоит того лишь при
			 *          однородном кадре: в кадре вперемешку всякий метод покажет среднее, и выбирать
			 *          будет не из чего.
			 *
			 * @note Имя `TEXT` занято макросом `winnt.h` у MS Windows, и переименовано оно НЕ
			 *       БЫЛО - в отличие от пары `single_t::NO`/`YES`, прежде звавшейся `FALSE`/`TRUE`.
			 *       Разница у макросов существенная: `TEXT(quote)` ФУНКЦИОНАЛЬНЫЙ и подставляется
			 *       лишь тогда, когда имя стоит перед открывающей скобкой, тогда как `FALSE` и
			 *       `TRUE` были объектными и подставлялись ВЕЗДЕ, никакой квалификацией не
			 *       спасаясь
			 *
			 * @note Замерено на Windows 11 ARM64 (MinGW/clangarm64) 03.09.2026: перечисление
			 *       с членом `TEXT = 1` собирается и печатает единицу, подстановки нет. Замер
			 *       прогнан отдельным щупом, а не выведен из свойств макроса
			 *
			 * @warning Мина эта, однако, замедленного действия: подстановка сработает, окажись
			 *          имя перед открывающей скобкой - в макросе, в шаблоне, где угодно. Пара снятия
			 *          макросов в начале файла бережёт лишь СВОЙ текст, а не место вызова у
			 *          потребителя. Всплывёт такой случай - переименовывать, и довод к тому уже здесь
			 *
			 * \~english
			 * @brief Kinds of the content of a chunk
			 *
			 * @details The kind of the content chooses the method of the compression: for a character text it is one, for
			 *          raw octets another, for homogeneous numbers a third one. This selection is worth it only with
			 *          a homogeneous chunk: in a chunk of a mixture every method will show an average, and there will be
			 *          nothing to choose from.
			 *
			 * @note The name `TEXT` is occupied by a macro of `winnt.h` at MS Windows, and renamed it was
			 *       NOT - unlike the pair `single_t::NO`/`YES`, which used to be called `FALSE`/`TRUE`. The
			 *       difference between the macros is essential: `TEXT(quote)` is FUNCTION-LIKE and expands
			 *       only when the name stands before an opening parenthesis, while `FALSE` and `TRUE` were
			 *       OBJECT-LIKE and expanded EVERYWHERE, with no qualification to escape them
			 *
			 * @note Measured on Windows 11 ARM64 (MinGW/clangarm64) on 2026-09-03: an enumeration with a
			 *       member `TEXT = 1` builds and prints a unit, there is no substitution. The measurement was
			 *       run by a separate probe, and not inferred from the properties of the macro
			 *
			 * @warning The mine is, however, one of a slow fuse: the expansion will trigger should the name
			 *          ever stand before an opening parenthesis - in a macro, in a template, anywhere. The pair
			 *          of the macro removals at the head of the file guards only ITS OWN text, not the place of
			 *          the call at a consumer. Should such a case appear - rename, and the argument to it is
			 *          already here
			 *
			 * \~
			 */
			enum class payload_t : uint8_t {
				MIXED   = 0x00, // Содержимое вперемешку
				TEXT    = 0x01, // Знаковый текст
				BINARY  = 0x02, // Сырые октеты
				NUMERIC = 0x03  // Однородные числа
			};

			/**
			 * \~russian
			 * @brief Снятые сведения о кадре
			 *
			 * \~english
			 * @brief Taken information about a chunk
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Chunk {
				/**
				 * \~russian
				 * Признак того, что кадр обращён правкой в мусор
				 *
				 * @note Мусорный кадр при подрядном чтении пропускается: он остаётся на
				 *       носителе лишь до уборки, а записей более не несёт
				 *
				 * \~english
				 * Flag that a chunk has been turned into a waste by an editing
				 *
				 * @note A waste chunk is skipped at a consecutive reading: it remains on
				 *       the medium only until a compaction and carries records no more
				 *
				 * \~
				 */
				bool waste;
				// Признак того, что содержимое кадра зашифровано
				bool encrypted;
				// Длина уложенного содержимого кадра в октетах
				uint32_t length;
				// Длина исходного содержимого кадра в октетах
				uint32_t origin;
				// Порядковый номер кадра
				uint64_t number;
				// Поколение записи кадра
				uint32_t generation;
				// Метод сжатия содержимого кадра
				compressor::method_t method;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * \~
				 */
				explicit Chunk() noexcept;
			} chunk_t;

			/**
			 * \~russian
			 * @brief Функция выработки контрольной суммы кадра
			 *
			 * @details Сумма кроет заголовок кадра вместе с содержимым его, а САМА в неё не
			 *          входит. Разряд мусора при выработке снимается: он учётный, метится правкой
			 *          одного октета уже уложенного кадра, и сумма ему следовать не обязана.
			 *
			 * @note Работа открыта наружу ради тех, кто правит кадр НА МЕСТЕ: всякая правка
			 *       содержимого обязана обновить сумму, иначе снятие кадра ответит отказом
			 *
			 * @param buffer буфер кадра целиком, от заголовка его
			 * @param size   размер кадра в октетах
			 * @return       выработанная контрольная сумма кадра
			 *
			 * \~english
			 * @brief Function of the production of the checksum of a chunk
			 *
			 * @details The sum covers the header of the chunk together with its content while ITSELF is not
			 *          included in it. The bit of the waste is cleared upon the production: it is an accounting
			 *          one, marked by an editing of one octet of a chunk already laid, and the sum is not
			 *          obliged to follow it.
			 *
			 * @note The work is opened outwards for those who edit a chunk IN PLACE: every editing of the
			 *       content is obliged to update the sum, otherwise the taking of the chunk answers with a
			 *       refusal
			 *
			 * @param buffer buffer of the whole chunk, from its header
			 * @param size   size of the chunk in octets
			 * @return       produced checksum of the chunk
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ uint64_t digest(const void * buffer, const size_t size) noexcept;
			/**
			 * \~russian
			 * @brief Функция обёртки уложенной записи кадром
			 *
			 * @details Заголовок кадра ставится ВПЕРЕДИ поданной записи, а сама она остаётся
			 *          как есть. Служит записи подписи владельца: тело контейнера обходится кадрами
			 *          подряд, и всё, что лежит внутри обхода, обязано быть кадром. Без обёртки место
			 *          записи подписи приходилось бы затирать кадром-заглушкой при следующей фиксации,
			 *          а между затиранием и записью головного заголовка оставалось окно, где обрыв
			 *          губил подпись прежнего поколения.
			 *
			 * @note Кадр метится МУСОРОМ: содержимое его записью контейнера не является, и
			 *       подрядное чтение обязано его пропустить
			 *
			 * @note Отказом отвечается запись, в кадр не вмещающаяся: длина кадра объявлена
			 *       четырьмя октетами, и запись длиннее легла бы в них усечённой МОЛЧА
			 *
			 * @param result     буфер уложенной записи, обёртываемой кадром
			 * @param number     порядковый номер кадра
			 * @param generation поколение записи кадра
			 * @return           признак успешной обёртки записи кадром
			 *
			 * \~english
			 * @brief Function of the wrapping of a laid record by a chunk
			 *
			 * @details The header of the chunk is put IN FRONT of the submitted record while the record
			 *          itself is left as is. It serves the record of the signature of the owner: the body of a container
			 *          is walked by the chunks in a row, and everything lying inside the walk is obliged to be a chunk.
			 *
			 * @note The chunk is marked as a WASTE: its content is not a record of the container, and a
			 *       consecutive reading is obliged to skip it
			 *
			 * @note A record which does not fit into a chunk is answered with a REFUSAL: the length of a chunk
			 *       is declared by four octets, and a longer record would lie in them truncated SILENTLY
			 *
			 * @param result     buffer of the laid record being wrapped by a chunk
			 * @param number     ordinal number of the chunk
			 * @param generation generation of the record of the chunk
			 * @return           true if the record has been wrapped by a chunk
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool envelope(vector <uint8_t> & result, const uint64_t number, const uint32_t generation) noexcept;

			/**
			 * \~russian
			 * @brief Класс укладки и снятия кадра
			 *
			 * @note **Щуп НУЖНОСТИ (заслон снимается), 08.09.2026.** Обесточены порознь все
			 *       28 заслонов части, под сторожем 24. Молчали ЧЕТЫРЕ:
			 *       - воронка отказа укладчика, доносящая причину журналу
			 *       (`ThePackerFunnelReportsItsCauseToTheJournal`) - четвёртая в своём роду,
			 *       закреплена порознь от близнецов;
			 *       - сумма кадра короче заголовка (`TheDigestOfAShortChunkIsEmpty`) - работа
			 *       отвечает ЧИСЛОМ, а не отказом, и обесточенный заслон валит прогон;
			 *       - ДВА недостижимы чёрным ящиком: пределы поля `uint32` у длины кадра и у
			 *       длины содержимого требуют четырёх гигабайт поданного. Сужены, не закрыты.
			 *       Шесть мест части краснят падением набора, что нужность их устанавливает
			 *       крепче всякого покраснения
			 *
			 * @note **О местах отказа, каких проверкою не навести.** Щуп ПУТИ 05.09.2026
			 *       (заслон принуждается срабатывать всегда) прошёл пять мест кадра, каким щуп
			 *       ПРИЧИН сторожа не нашёл, - и не промолчал ни разу: поверка длины краснит
			 *       ВОСЕМЬДЕСЯТ проверок, отказы шифрования и сжатия - от четырёх до пяти. То
			 *       есть заслоны живы и хожены, а молчание щупа причин означало лишь, что причины
			 *       там никто не спрашивал. Мёртвым кодом их звать нельзя
			 *
			 * @warning **Прежняя редакция этой записи была НЕВЕРНА.** Она числила условие
			 *          недостижимым, поминая среди недостижимого «недостающее снаряжение», - а
			 *          снаряжение кодеку НЕ ПРИНАДЛЕЖИТ: и разжиматель, и модуль шифрования отдаёт
			 *          зовущий, отдать их волен когда угодно, в том числе ПОСЛЕ укладки, а метод
			 *          сжатия кадр несёт в себе. Оттого сжатое волен получить тот, кто сжатия не
			 *          знает вовсе, и заказать шифрование волен тот, кто модуля не отдал. Разлад
			 *          оснастки достижим ПРЯМО, и за 05.09.2026 им закрыто шесть мест ABC:
			 *          `ChunkFixture.UnpackingCompressedWithoutTheCompressorIsRefused`,
			 *          `IndexFixture.UnpackFailureCarriesThePackerCause` (два устья),
			 *          `EditorFixture.PendingUnpackFailureCarriesThePackerCause`,
			 *          `EditorFixture.OpeningCompressedWithoutTheCompressorCarriesThePackerCause`,
			 *          `EditorFixture.PackFailureCarriesThePackerCause` и
			 *          `ContainerFixture.IndexPackFailureCarriesThePackerCause`.
			 *          Недостижимо ценою прогона остаётся ОДНО: длина сверх поля кадра, требующая
			 *          записи шире четырёх гигабайт
			 *
			 * @note Ошибка прежней редакции поучительна: «недостающее снаряжение» было
			 *       названо недостижимым по РАССУЖДЕНИЮ - набор оснастку по ходу работы не менял
			 *       ни разу, и молчание набора приняли за свойство кодека. Свойство же
			 *       принадлежало набору
			 *
			 * @details Кадр самоограничен: он несёт свой метод сжатия, длину и порядковый
			 *          номер. Оттого кадры сжимаются и шифруются поодиночке, а не сплошным потоком, и
			 *          читать до конца ради одного кадра не приходится.
			 *
			 * @details **Сжатие и шифрование ведутся отданными извне работами.** Модуль
			 *          сжатия и модуль шифрования заводятся потребителем и настраиваются им же: у них
			 *          свои зависимости, и кодек их за потребителя не заводит.
			 *
			 * @note Всякий кадр шифруется своим вызовом, а не общим потоком. Вызов этот
			 *       заводит свой вектор инициализации: общий поток на все кадры повторил бы его при
			 *       перезаписи кадра, а повтор вектора при том же ключе - это взлом, и по работе
			 *       программы он не виден вовсе
			 *
			 * \~english
			 * @brief Class of the laying and the taking of a chunk
			 *
			 * @note **The NEEDNESS audit (a guard is removed), 2026-09-08.** All 28 guards of the part were
			 *       de-energized apart, under the watch 24. FOUR stood silent:
			 *       - the funnel of a refusal of the packer, reporting its cause to the journal
			 *       (`ThePackerFunnelReportsItsCauseToTheJournal`) - the fourth of its kind, fixed apart from
			 *       its twins;
			 *       - the checksum of a chunk shorter than the header (`TheDigestOfAShortChunkIsEmpty`) - the
			 *       work answers with a NUMBER, not with a refusal, and a de-energized guard brings the run
			 *       down;
			 *       - TWO are unreachable for a black box: the limits of the `uint32` field at the length of a
			 *       chunk and at the length of the content require four gigabytes fed. Narrowed, not closed.
			 *       Six sites of the part turn red by a crash of the fixture, which settles their needness
			 *       harder than any reddening
			 *
			 * @note **Of the failure sites that a check cannot reach.** The PATH probe of 2026-09-05 (a guard
			 *       is forced to fire always) went over five sites of a chunk to which the probe of the CAUSES
			 *       found no guard, - and it did not keep silent even once: the verification of the length
			 *       turns EIGHTY checks red, the refusals of the encryption and of the compression - from four
			 *       to five. That is, the guards are alive and travelled, and the silence of the probe of the
			 *       causes meant only that no one asked a cause there. To call them dead code is impossible
			 *
			 * @warning **The former edition of this record was WRONG.** It listed the condition as
			 *          unreachable, naming among the unreachable "a missing gear", - yet the gear is NOT OWNED
			 *          by the codec: both the unpacker and the module of the encryption are handed by the
			 *          caller, and handed them he may at any moment, in particular AFTER the laying, while the
			 *          method of the compression a chunk carries in itself. Therefore the compressed may be
			 *          obtained by one who knows no compression at all, and the encryption may be ordered by
			 *          one who did not hand the module. A disorder of the equipment is reachable DIRECTLY, and
			 *          by 2026-09-05 six sites of ABC are closed by it:
			 *          `ChunkFixture.UnpackingCompressedWithoutTheCompressorIsRefused`,
			 *          `IndexFixture.UnpackFailureCarriesThePackerCause` (two mouths),
			 *          `EditorFixture.PendingUnpackFailureCarriesThePackerCause`,
			 *          `EditorFixture.OpeningCompressedWithoutTheCompressorCarriesThePackerCause`,
			 *          `EditorFixture.PackFailureCarriesThePackerCause` and
			 *          `ContainerFixture.IndexPackFailureCarriesThePackerCause`. Unreachable at the price of a
			 *          run there remains ONE: a length beyond the field of a chunk, requiring a record wider
			 *          than four gigabytes
			 *
			 * @note The mistake of the former edition is instructive: "the missing gear" was named
			 *       unreachable by REASONING - the fixture changed its equipment not once over the course of
			 *       the work, and the silence of the fixture was taken for a property of the codec. The
			 *       property belonged to the fixture
			 *
			 * @details A chunk is self-delimited: it carries its own method of the compression, length and ordinal
			 *          number. Therefore the chunks are compressed and encrypted one by one rather than by a solid stream, and
			 *          one does not have to read to the end for the sake of one chunk.
			 *
			 * @details **The compression and the encryption are conducted by the works given from outside.** The module
			 *          of the compression and the module of the encryption are created by the consumer and are configured by it as well: they have
			 *          their own dependencies, and the codec does not create them for the consumer.
			 *
			 * @note Every chunk is encrypted by its own call rather than by a common stream. This call
			 *       creates its own initialization vector: a common stream for all the chunks would repeat it at
			 *       a rewriting of a chunk, while a repetition of the vector with the same key is a break, and by the work
			 *       of the program it is not visible at all
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Packer {
				public:
					/**
					 * \~russian
					 * @brief Настройки укладки кадра
					 *
					 * \~english
					 * @brief Settings of the laying of a chunk
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Settings {
						// Признак шифрования содержимого кадра
						bool encrypt;
						/**
						 * \~russian
						 * Длина, ниже которой сжатие не выполняется вовсе
						 *
						 * @note Сжатие мелкого кадра не окупается: заголовок метода и словарь
						 *       съедают больше, чем сберегают
						 *
						 * \~english
						 * Length below which the compression is not performed at all
						 *
						 * @note The compression of a small chunk does not pay off: the header of the method and the dictionary
						 *       eat up more than they save
						 *
						 * \~
						 */
						size_t threshold;
						/**
						 * \~russian
						 * Вид хэш-суммы выработки ключа шифрования
						 *
						 * @note Вид шифра и вид хэш-суммы ставит потребитель, а не кодек: чем
						 *       шифровать своё содержимое, ведомо тому, кто контейнер завёл
						 *
						 * \~english
						 * Kind of the hash sum of the derivation of the key of the encryption
						 *
						 * @note The kind of the cipher and the kind of the hash sum are set by the consumer rather than by the codec: by what
						 *       to encrypt its own content is known to the one who created the container
						 *
						 * \~
						 */
						crypto_t::hash_t hash;
						// Размер шифрования содержимого кадра
						crypto_t::cipher_t cipher;
						// Метод сжатия содержимого вперемешку
						compressor::method_t mixed;
						// Метод сжатия знакового текста
						compressor::method_t text;
						// Метод сжатия сырых октетов
						compressor::method_t binary;
						// Метод сжатия однородных чисел
						compressor::method_t numeric;
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * \~english
						 * @brief Constructor
						 *
						 * \~
						 */
						explicit Settings() noexcept;
					} settings_t;
				private:
					// Код отказа укладки либо снятия кадра
					error_t _error;
				private:
					// Настройки укладки кадра
					settings_t _settings;
				private:
					// Модуль сжатия, отданный потребителем
					const compressor::block_t * _compressor;
				private:
					// Модуль шифрования, отданный потребителем
					const crypto_t * _crypto;
				private:
					/**
					 * \~russian
					 * @brief Метод объявления отказа укладки либо снятия кадра
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
					 * @details The report goes from here, from the single place of the declaration of a refusal: the
					 *          work answers with a refusal by a multitude of paths, and a record in each of them would
					 *          diverge from the others. A refusal ACCEPTED from the lower layer does not come here -
					 *          that layer has reported it itself, and a second report would only double the records.
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
					 * @brief Метод подбора метода сжатия под вид содержимого
					 *
					 * @param kind вид содержимого кадра
					 * @return     метод сжатия содержимого
					 *
					 * \~english
					 * @brief Method of the selection of the method of the compression for the kind of the content
					 *
					 * @param kind kind of the content of the chunk
					 * @return     method of the compression of the content
					 *
					 * \~
					 */
					compressor::method_t suggest(const payload_t kind) const noexcept;
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
					 * @brief Метод укладки кадра
					 *
					 * @details Сжатие, не давшее выигрыша, отбрасывается: кадр ложится сырым, а
					 *          методом его объявляется отсутствие сжатия. Иначе мелкое содержимое росло
					 *          бы от заголовков сжатия, а разбор о том не узнал бы вовсе.
					 *
					 * @param buffer     буфер укладываемого содержимого
					 * @param size       размер укладываемого содержимого в октетах
					 * @param kind       вид укладываемого содержимого
					 * @param number     порядковый номер кадра
					 * @param generation поколение записи кадра
					 * @param result     буфер, куда следует уложить кадр
					 * @return           признак успешности укладки
					 *
					 * \~english
					 * @brief Method of the laying of a chunk
					 *
					 * @details A compression which has not given a gain is discarded: the chunk is laid raw, while
					 *          its method is declared to be the absence of the compression. Otherwise a small content would grow
					 *          from the headers of the compression, and the parsing would not learn of it at all.
					 *
					 * @param buffer     buffer of the content being laid
					 * @param size       size of the content being laid in octets
					 * @param kind       kind of the content being laid
					 * @param number     ordinal number of the chunk
					 * @param generation generation of the record of the chunk
					 * @param result     buffer the chunk should be laid into
					 * @return           sign of the success of the laying
					 *
					 * \~
					 */
					bool pack(const void * buffer, const size_t size, const payload_t kind, const uint64_t number, const uint32_t generation, vector <uint8_t> & result) noexcept;
					/**
					 * \~russian
					 * @brief Метод снятия кадра
					 *
					 * @details Кадр, поданный не целиком, смещения не сдвигает: недостающие
					 *          октеты дожидаются следующей подачи.
					 *
					 * @param buffer буфер поданных октетов
					 * @param size   размер поданных октетов
					 * @param offset смещение, с какого следует снимать кадр
					 * @param result буфер, куда следует положить содержимое кадра
					 * @param chunk  снятые сведения о кадре
					 * @return       признак успешно снятого кадра
					 *
					 * \~english
					 * @brief Method of the taking of a chunk
					 *
					 * @details A chunk submitted not in full does not shift the offset: the missing
					 *          octets wait for the next submission.
					 *
					 * @param buffer buffer of the submitted octets
					 * @param size   size of the submitted octets
					 * @param offset offset the chunk should be taken from
					 * @param result buffer the content of the chunk should be placed into
					 * @param chunk  taken information about the chunk
					 * @return       sign of a successfully taken chunk
					 *
					 * \~
					 */
					bool unpack(const void * buffer, const size_t size, size_t & offset, vector <uint8_t> & result, chunk_t & chunk) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения кода отказа укладки либо снятия кадра
					 *
					 * @return код отказа
					 *
					 * \~english
					 * @brief Method of the extraction of the error code of the laying or of the taking of a chunk
					 *
					 * @return error code
					 *
					 * \~
					 */
					error_t error() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения настроек укладки кадра
					 *
					 * @return настройки укладки кадра
					 *
					 * \~english
					 * @brief Method of the extraction of the settings of the laying of a chunk
					 *
					 * @return settings of the laying of the chunk
					 *
					 * \~
					 */
					const settings_t & settings() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки настроек укладки кадра
					 *
					 * @param settings устанавливаемые настройки укладки кадра
					 *
					 * \~english
					 * @brief Method of the setting of the settings of the laying of a chunk
					 *
					 * @param settings settings of the laying of the chunk being set
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
					explicit Packer() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					~Packer() noexcept = default;
			} packer_t;
		};
	};
};

/**
 * Возвращаем системные макросы потребителю библиотеки:
 * имена, подавленные в начале файла, снова принадлежат ему
 */
#include "../../sys/macro/restore.hpp"
#include "../../sys/macro/global.hpp"
