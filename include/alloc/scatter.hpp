/**
 * @file scatter.hpp
 * @date 2026-09-15
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
 * @brief Заголовочный файл рассеяния ключевого материала по образу
 *
 * @section scatter_decisions Намеренные решения
 *
 * @details <b>Задача.</b> Зашитый в образ ключ находят НЕ разбором кода, а поиском
 *          участка высокой неопределённости: тридцать два случайных байта видны в
 *          образе пятном и находятся сканером за секунды, без понимания логики. Оттого
 *          материал в образе не лежит готовым и не лежит кучкой - он рассеян по носителю
 *          и выводится из него на запуске.
 *
 *          <b>Рамка, которую нельзя терять.</b> Приложение выводит ключ САМО, без
 *          человека, - значит всё нужное лежит внутри, и у хозяина машины оно есть.
 *          Покупается стоимость поиска, а не невозможность: рассеяние закрывает дорогу
 *          сканеру пятна и разбору по частям, но не закрывает дорогу тому, кто снимет
 *          собранный материал точкой останова на входе в вывод ключа. Против последнего
 *          средства здесь нет и не обещано.
 *
 *          <b>Две стороны одного договора считают ОДНУ раскладку из ОДНОГО зерна.</b>
 *          Раскладчик (`lay`) работает при сборке на закрытой машине: секрет и зерно на
 *          входе, носитель на выходе. Сборщик (`gather`) работает на запуске в образе:
 *          носитель и то же зерно на входе, собранный материал в приёмнике тайн на
 *          выходе. Обе стороны выводят перестановку мест и гамму из зерна одним и тем же
 *          воспроизводимым потоком, оттого поток обязан давать один ряд на всякой
 *          системе - он и даёт: `splitmix64` есть чистая арифметика над
 *          шестидесятичетырёхразрядным словом, без обращения к платформе.
 *
 *          <b>Гамма наложением обязательна, а не украшение.</b> Ляг в носитель открытый
 *          байт секрета, пусть и рассеянный, - места секрета выдали бы себя разбором по
 *          частям: тридцать два особых байта среди рабочих данных. Оттого в носитель
 *          ложится `секрет XOR гамма`, а гамма выводится из зерна, какого в образе нет
 *          вовсе. Это и значит «ключ не хранится, а выводится»: в образе лежит наложение,
 *          снять его нечем, не имея зерна.
 *
 *          <b>Носитель равномерен по всей длине.</b> Места, не занятые секретом,
 *          заполняются байтами того же потока, а не нулём и не случаем со стороны. Так
 *          места секрета неотличимы от заполняющих по распределению, и пятна в образе
 *          нет вовсе. Заполни мы прочее нулём - секрет проступил бы островками среди
 *          пустоты; возьми случай со стороны - раскладка перестала бы быть
 *          воспроизводимой, а воспроизводимость нужна: одно зерно даёт одну сборку.
 *
 *          <b>Ёмкость носителя объявляет раскладчик, и она НЕ меньше длины секрета.</b>
 *          Запас сверх длины - место, куда лечь заполняющим байтам и куда рассеять
 *          секрет пошире. Чем шире носитель, тем реже места секрета, тем дороже их
 *          собрать, не зная зерна, - но тем толще образ. Мера отдана зовущему.
 *
 *          <b>Собранное ложится в приёмник тайн ПОБАЙТНО, минуя кучу.</b> Один временный
 *          сбор на стеке или в куче обнулил бы весь труд: собранный материал лежал бы
 *          открытым в незащищённой памяти. Сборщик льёт байт за байтом прямо в
 *          `vessel_t`, и промежуточного сбора не возникает вовсе.
 *
 *          <b>Привязка к контрольной сумме кода ведётся якорем.</b> `anchor` подмешивает
 *          в зерно контрольную сумму собственного кода и версию ключа: патч образа
 *          меняет сумму, а с ней зерно и весь поток, и секрет собирается неверным -
 *          защита неотделима от расшифровки. Механизм здесь; саму сумму и участок образа,
 *          по какому она берётся, подаёт зовущий - участок у macOS и MS Windows подпись
 *          правит ПОСЛЕ сборки, и выбирать его надо со стендами каждой системы.
 *
 *          <b>Чего рассеяние НЕ делает.</b> Оно не выводит ключ из собранного материала -
 *          вывод (KDF, растяжение пароля) есть договор криптографической библиотеки, и
 *          рассеяние оставляет ему собранный материал в приёмнике тайн, не более.
 *
 * \~english
 * @brief Header file of scattering the key material across the image
 *
 * @section scatter_decisions Intentional decisions
 *
 * @details <b>The task.</b> A key embedded in an image is found NOT by code analysis
 *          but by searching for a region of high entropy: thirty-two random bytes are
 *          visible in the image as a blob and are found by a scanner in seconds, without
 *          understanding the logic. Hence the material does not lie in the image ready
 *          or in a pile - it is scattered across the carrier and derived from it at
 *          startup.
 *
 *          <b>A framework that must not be lost.</b> The application derives the key
 *          ITSELF, without a human, - so everything needed is inside, and the machine
 *          owner has it. What is bought is the cost of search, not impossibility:
 *          scattering blocks the blob scanner and piece-by-piece analysis, but does not
 *          block someone who snapshots the assembled material at a breakpoint on entry
 *          to the key derivation. Against the latter there is no remedy here and none
 *          is promised.
 *
 *          <b>Both sides of one contract compute ONE layout from ONE seed.</b> The
 *          layer (`lay`) runs at build time on a closed machine: secret and seed in,
 *          carrier out. The gatherer (`gather`) runs at startup in the image: carrier
 *          and the same seed in, assembled material in the secret vessel out. Both
 *          sides derive the permutation of places and the gamma from the seed with the
 *          same reproducible stream, hence the stream must give one sequence on every
 *          system - and it does: `splitmix64` is pure arithmetic over a sixty-four-bit
 *          word, with no platform access.
 *
 *          <b>Gamma overlay is mandatory, not decoration.</b> Should an open byte of
 *          the secret lie in the carrier, even scattered, - the secret's places would
 *          betray themselves by piece-by-piece analysis: thirty-two distinctive bytes
 *          among working data. Hence the carrier stores `secret XOR gamma`, and the
 *          gamma is derived from the seed, which is not in the image at all. This is
 *          what "the key is not stored but derived" means: the image holds an overlay,
 *          and it cannot be removed without the seed.
 *
 *          <b>The carrier is uniform across its full length.</b> Places not occupied
 *          by the secret are filled with bytes of the same stream, not zeros and not
 *          randomness from outside. This way the secret places are indistinguishable
 *          from the fill by distribution, and there is no blob in the image. Had we
 *          filled the rest with zeros - the secret would show as islands among emptiness;
 *          had we taken randomness from outside - the layout would stop being
 *          reproducible, and reproducibility is needed: one seed gives one build.
 *
 *          <b>The carrier capacity is declared by the layer, and it is NOT less than
 *          the secret length.</b> The excess beyond the length is a place for fill bytes
 *          and for scattering the secret wider. The wider the carrier, the rarer the
 *          secret places, the more expensive to gather them without the seed, - but the
 *          thicker the image. The trade-off is given to the caller.
 *
 *          <b>The assembled material goes into the secret vessel BYTE BY BYTE, bypassing
 *          the heap.</b> A single temporary assembly on the stack or in the heap would
 *          negate the whole effort: the assembled material would lie open in unprotected
 *          memory. The gatherer pours byte by byte straight into `vessel_t`, and no
 *          intermediate assembly arises.
 *
 *          <b>Binding to the code checksum is done by an anchor.</b> `anchor` mixes
 *          into the seed the checksum of its own code and the key version: an image
 *          patch changes the checksum, and with it the seed and the whole stream, and
 *          the secret is assembled incorrectly - the protection is inseparable from
 *          decryption. The mechanism is here; the checksum itself and the image region
 *          it is taken from are supplied by the caller - on macOS and MS Windows the
 *          signature fixes the region AFTER build, and it must be chosen with the
 *          stands of each system.
 *
 *          <b>What scattering does NOT do.</b> It does not derive the key from the
 *          assembled material - derivation (KDF, password stretching) is the contract
 *          of the cryptographic library, and scattering leaves the assembled material
 *          in the secret vessel, nothing more.
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 * \~
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
#include <cstddef>
#include <cstdint>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "vessel.hpp"
#include "../sys/macro/global.hpp"

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
	 * \~russian
	 * @brief Пространство имён распределителя памяти
	 *
	 * \~english
	 * @brief Namespace of the memory allocator
	 *
	 * \~
	 */
	namespace alloc {
		/**
		 * \~russian
		 * @brief Пространство имён рассеяния ключевого материала по образу
		 *
		 * \~english
		 * @brief Namespace of scattering the key material across the image
		 *
		 * \~
		 */
		namespace scatter {
			/**
			 * \~russian
			 * @brief Функция подмешивания якоря в зерно раскладки
			 *
			 * @note Привязывает раскладку к якорю - контрольной сумме собственного кода
			 *       и версии ключа. Обе стороны договора зовут его ОДИНАКОВО до `lay` и
			 *       `gather`, подавая одно зерно, один якорь, одну версию: иначе поток
			 *       разойдётся и секрет не соберётся
			 *
			 * @note Патч образа - снятие проверки, вставка точки останова - меняет
			 *       контрольную сумму, а с ней зерно и весь поток: секрет собирается
			 *       неверным. Защита становится неотделима от расшифровки - не
			 *       «приложение обнаружило взлом», а «контейнер не открылся»
			 *
			 * @note Саму контрольную сумму и участок образа, по какому она берётся,
			 *       подаёт зовущий: участок у macOS и MS Windows подпись правит ПОСЛЕ
			 *       сборки, и выбирать его надо со стендами каждой системы
			 *
			 * @param seed     зерно раскладки из закрытого контура
			 * @param checksum контрольная сумма кода либо иной якорь, либо нуль
			 * @param version  версия ключа, разводящая контуры сборок
			 * @return         зерно, привязанное к якорю и версии
			 *
			 * \~english
			 * @brief Function of blending an anchor into the layout seed
			 *
			 * @note Binds the layout to an anchor - the checksum of its own code and
			 *       the key version. Both sides of the contract call it IDENTICALLY
			 *       before `lay` and `gather`, passing one seed, one anchor, one
			 *       version: otherwise the stream diverges and the secret is not
			 *       assembled
			 *
			 * @note An image patch - removing a check, inserting a breakpoint -
			 *       changes the checksum, and with it the seed and the whole stream:
			 *       the secret is assembled incorrectly. The protection becomes
			 *       inseparable from decryption - not "the application detected a
			 *       breach" but "the container did not open"
			 *
			 * @note The checksum itself and the image region it is taken from are
			 *       supplied by the caller: on macOS and MS Windows the signature
			 *       fixes the region AFTER build, and it must be chosen with the
			 *       stands of each system
			 *
			 * @param seed     layout seed from the closed loop
			 * @param checksum code checksum or another anchor, or zero
			 * @param version  key version separating build loops
			 * @return         seed bound to the anchor and the version
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ uint64_t anchor(const uint64_t seed, const uint64_t checksum, const uint64_t version = 0) noexcept;
			/**
			 * \~russian
			 * @brief Функция раскладки секрета по носителю (время сборки)
			 *
			 * @note Работает при сборке на закрытой машине: секрет наружу образа не
			 *       уходит, в образ уходит лишь носитель. В приложении не зовётся
			 *
			 * @note Ёмкость носителя обязана быть НЕ меньше длины секрета: иначе
			 *       части секрета некуда лечь, и функция отвечает пустым носителем
			 *
			 * @param secret   адрес секрета
			 * @param length   длина секрета в байтах
			 * @param seed     зерно раскладки из закрытого контура
			 * @param capacity ёмкость носителя в байтах, не меньше длины секрета
			 * @return         носитель с рассеянным секретом либо пустой при отказе
			 *
			 * \~english
			 * @brief Function of laying the secret out across the carrier (build time)
			 *
			 * @note Works at build time on a closed machine: the secret does not
			 *       leave the image, only the carrier goes into the image. Not called
			 *       in the application
			 *
			 * @note The carrier capacity must be NOT less than the secret length:
			 *       otherwise there is nowhere for the secret parts to lie, and the
			 *       function answers with an empty carrier
			 *
			 * @param secret   address of the secret
			 * @param length   length of the secret in bytes
			 * @param seed     layout seed from the closed loop
			 * @param capacity capacity of the carrier in bytes, not less than the length
			 * @return         carrier with the scattered secret, or empty on refusal
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ std::vector <uint8_t> lay(const uint8_t * secret, const size_t length, const uint64_t seed, const size_t capacity) noexcept;
			/**
			 * \~russian
			 * @brief Функция вплетения секрета в предоставленную рабочую таблицу (время сборки)
			 *
			 * @note В отличие от `lay`, носитель СВОЙ не заводит и прочих мест не
			 *       заполняет: на местах секрета правит поданную таблицу, прочие её места
			 *       - настоящие рабочие данные - оставляет нетронутыми. Так секрет
			 *       сливается с осмысленными данными, а не образует свой участок высокой
			 *       неопределённости, какой сканер нашёл бы целиком
			 *
			 * @note Таблица обязана годиться носителем, и это ответственность зовущего:
			 *       её правка на местах секрета допустима (места отданы под секрет и
			 *       рабочего значения там больше нет), а собственная неопределённость её
			 *       достаточна, чтобы вкрапления секрета в ней не выделялись. Таблица из
			 *       нулей либо связного текста носителем не годится
			 *
			 * @param table  адрес рабочей таблицы, правимой на местах секрета
			 * @param size   размер рабочей таблицы в байтах, не меньше длины секрета
			 * @param secret адрес секрета
			 * @param length длина секрета в байтах
			 * @param seed   зерно раскладки из закрытого контура
			 * @return       признак выполнения операции
			 *
			 * \~english
			 * @brief Function of weaving the secret into a provided working table (build time)
			 *
			 * @note Unlike `lay`, it does not create its own carrier and does not fill
			 *       other places: it edits the provided table at the secret places and
			 *       leaves the rest - real working data - untouched. This way the
			 *       secret blends with meaningful data instead of forming its own
			 *       region of high entropy that a scanner would find whole
			 *
			 * @note The table must be suitable as a carrier, and this is the caller's
			 *       responsibility: editing it at the secret places is allowed (those
			 *       places are given to the secret and no working value remains there),
			 *       and its own entropy is sufficient that the secret's grains do not
			 *       stand out. A table of zeros or coherent text is not suitable as a
			 *       carrier
			 *
			 * @param table  address of the working table edited at the secret places
			 * @param size   size of the working table in bytes, not less than the length
			 * @param secret address of the secret
			 * @param length length of the secret in bytes
			 * @param seed   layout seed from the closed loop
			 * @return       flag of the operation performed
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool weave(uint8_t * table, const size_t size, const uint8_t * secret, const size_t length, const uint64_t seed) noexcept;
			/**
			 * \~russian
			 * @brief Функция порождения исходного текста с носителем (время сборки)
			 *
			 * @note Работает при сборке: отдаёт готовый к включению текст на C++ с
			 *       носителем в виде массива. Ни секрета, ни зерна текст не несёт -
			 *       только носитель, длину секрета и ёмкость
			 *
			 * @param carrier носитель, полученный от `lay`
			 * @param length  длина секрета в байтах
			 * @param name    имя порождаемого массива носителя
			 * @return        исходный текст на C++ либо пустая строка при отказе
			 *
			 * \~english
			 * @brief Function of emitting the source text with the carrier (build time)
			 *
			 * @note Works at build time: returns ready-to-include C++ source text with
			 *       the carrier as an array. The text carries neither the secret nor
			 *       the seed - only the carrier, the secret length, and the capacity
			 *
			 * @param carrier carrier obtained from `lay`
			 * @param length  length of the secret in bytes
			 * @param name    name of the emitted carrier array
			 * @return        C++ source text, or an empty string on refusal
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ std::string emit(const std::vector <uint8_t> & carrier, const size_t length, const std::string & name) noexcept;
			/**
			 * \~russian
			 * @brief Функция сборки секрета из носителя в приёмник тайн (запуск)
			 *
			 * @note Работает на запуске в образе: снимает наложение и льёт секрет
			 *       ПОБАЙТНО прямо в приёмник, минуя кучу. Промежуточного сбора нет
			 *
			 * @note Зерно обязано совпадать с тем, каким велась раскладка: чужое
			 *       зерно соберёт из носителя мусор, а не секрет
			 *
			 * @note Приёмник заводится ЗДЕСЬ на длину секрета: прежнее его содержимое
			 *       снимается
			 *
			 * @param carrier  адрес носителя в образе
			 * @param capacity ёмкость носителя в байтах
			 * @param seed     то же зерно, каким велась раскладка
			 * @param length   длина секрета в байтах
			 * @param vessel   приёмник, куда ложится собранный секрет
			 * @return         признак выполнения операции
			 *
			 * \~english
			 * @brief Function of gathering the secret from the carrier into the vessel (run time)
			 *
			 * @note Works at startup in the image: removes the overlay and pours the
			 *       secret BYTE BY BYTE straight into the vessel, bypassing the heap.
			 *       There is no intermediate assembly
			 *
			 * @note The seed must match the one used for the layout: a foreign seed
			 *       will gather garbage from the carrier, not the secret
			 *
			 * @note The vessel is created HERE for the secret length: its previous
			 *       content is removed
			 *
			 * @param carrier  address of the carrier in the image
			 * @param capacity capacity of the carrier in bytes
			 * @param seed     the same seed the layout was made with
			 * @param length   length of the secret in bytes
			 * @param vessel   vessel the gathered secret is put into
			 * @return         flag of the operation performed
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool gather(const uint8_t * carrier, const size_t capacity, const uint64_t seed, const size_t length, vessel_t & vessel) noexcept;
		}
	};
};
