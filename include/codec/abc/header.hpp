/**
 * @file header.hpp
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
 * @brief Заголовочный файл заголовка опознания бинарного контейнера ABC
 *
 * \~english
 * @brief Header file of the identifying header of the ABC binary container
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_ABC_HEADER__
#define __AWH_CODEC_ABC_HEADER__

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <cstdint>
#include <cstddef>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"

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
			 * @brief Длина заголовка опознания в октетах
			 *
			 * @details Длина постоянна намеренно: заголовок читается одним коротким чтением
			 * прежде всего прочего, и переменная длина потребовала бы читать его дважды
			 *
			 * \~english
			 * @brief Length of the identifying header in octets
			 * @details The length is constant deliberately: the header is read by one short reading
			 * before everything else, and a variable length would require reading it twice
			 *
			 * \~
			 */
			constexpr size_t HEADER_LENGTH = 96;

			/**
			 * \~russian
			 * @brief Длина признака владельца контейнера в октетах
			 *
			 * \~english
			 * @brief Length of the sign of the owner of a container in octets
			 *
			 * \~
			 */
			constexpr size_t OWNER_LENGTH = 16;

			/**
			 * \~russian
			 * @brief Длина отпечатка открытого ключа в октетах
			 *
			 * @details Отпечаток есть усечённая свёртка канонической записи открытого ключа.
			 * Усечение задаётся здесь, а не модулем подписи: сколько его отрезать, решает вид
			 * записи контейнера, а не устройство ключа
			 *
			 * \~english
			 * @brief Length of the fingerprint of a public key in octets
			 * @details The fingerprint is a truncated digest of the canonical record of a public key.
			 * The truncation is set here rather than by the module of the signature: how much of it to cut off is decided by the kind
			 * of the record of the container rather than by the structure of the key
			 *
			 * \~
			 */
			constexpr size_t FINGERPRINT_LENGTH = 16;

			/**
			 * \~russian
			 * @brief Старшая версия поддерживаемого вида записи
			 *
			 * \~english
			 * @brief Major version of the supported kind of the record
			 *
			 * \~
			 */
			constexpr uint8_t VERSION_MAJOR = 1;

			/**
			 * \~russian
			 * @brief Младшая версия поддерживаемого вида записи
			 *
			 * \~english
			 * @brief Minor version of the supported kind of the record
			 *
			 * \~
			 */
			constexpr uint8_t VERSION_MINOR = 0;

			/**
			 * \~russian
			 * @brief Разряды свойств контейнера
			 *
			 * @details Разряды эти лежат в заголовке открыто и читаются прежде тела: по ним
			 * потребитель узнаёт, что с телом делать, ещё не имея ни ключа, ни самого тела
			 *
			 * \~english
			 * @brief Bits of the properties of a container
			 * @details These bits lie in the header openly and are read before the body: by them
			 * the consumer learns what to do with the body while having neither the key nor the body itself
			 *
			 * @note Признак `CANONICAL` есть ОБЪЯВЛЕНИЕ собирателя, а не поверенная истина:
			 * заголовок читается прежде тела, и поверить его наперёд нельзя. Поверяется он
			 * разбором самих записей - настройкой `Reader::Settings::canonical`, какую
			 * потребитель ставит по этому признаку: `reader.settings().canonical =
			 * fetcher.header().is(flag_t::CANONICAL)`
			 *
			 * \~
			 */
			enum class flag_t : uint16_t {
				NONE       = 0x0000, // Свойств не объявлено
				CANONICAL  = 0x0001, // Тело собрано строгим видом записи
				COMPRESSED = 0x0002, // Тело сжато
				ENCRYPTED  = 0x0004, // Тело зашифровано
				SIGNED     = 0x0008, // Контейнер подписан владельцем
				PAGED      = 0x0010, // Тело уложено страницами, а не поточной чередой
				STREAM     = 0x0020  // Тело несёт череду документов, а не один
			};

			/**
			 * \~russian
			 * @brief Заголовок опознания контейнера
			 *
			 * @details Заголовок отвечает на вопрос «тот ли это контейнер либо подмена» до
			 * загрузки тела, без ключа расшифровки и без разбора записи. Оттого он постоянной
			 * длины, не сжат, не зашифрован и несёт свою контрольную сумму
			 *
			 * @details **Признак владельца и вид содержимого кодеком не толкуются.** Их
			 * ставит потребитель, а контейнер лишь хранит и выдаёт: что они означают, ведомо
			 * тому, кто контейнер завёл
			 *
			 * @note Контрольная сумма отделяет подмену от усечения: обрезанный файл честно
			 * скажет «оборван», а не «чужой». Подделку она не ловит и ловить не должна - на
			 * то есть подпись владельца, а сумма стережёт порчу
			 *
			 * \~english
			 * @brief Identifying header of a container
			 * @details The header answers the question "is this the right container or a substitution" before
			 * the loading of the body, without the key of the decryption and without the parsing of the record. Therefore it is of a constant
			 * length, is not compressed, is not encrypted and carries its own checksum
			 * @details **The sign of the owner and the kind of the content are not interpreted by the codec.** They
			 * are set by the consumer, while the container only stores and issues them: what they mean is known
			 * to the one who created the container
			 * @note The checksum separates a substitution from a truncation: a cut file will honestly
			 * say "truncated" rather than "foreign". It does not catch a forgery and must not — for
			 * that there is the signature of the owner, while the sum guards against a corruption
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Header {
				// Старшая версия вида записи контейнера
				uint8_t version;
				// Младшая версия вида записи контейнера
				uint8_t revision;
				// Разряды свойств контейнера
				uint16_t flags;
				// Вид содержимого контейнера, ставимый потребителем
				uint32_t content;
				// Длина тела контейнера в октетах
				uint64_t length;
				// Количество записей в теле контейнера
				uint64_t records;
				// Смещение оглавления от начала контейнера, ноль - оглавления нет
				uint64_t index;
				// Смещение подписи от начала контейнера, ноль - подписи нет
				uint64_t signature;
				/**
				 * \~russian
				 * Поколение записи контейнера
				 *
				 * @note Поколение растёт на всякую фиксацию правок. По нему и отличают
				 * головной заголовок от хвостового, когда перезапись головного оборвалась
				 *
				 * \~english
				 * Generation of the record of a container
				 * @note The generation grows at every commit of the edits. By it the head header is distinguished
				 * from the tail one when the rewriting of the head one has been interrupted
				 *
				 * \~
				 */
				uint64_t generation;
				// Признак владельца контейнера, ставимый потребителем
				uint8_t owner[OWNER_LENGTH];
				// Отпечаток открытого ключа владельца контейнера
				uint8_t fingerprint[FINGERPRINT_LENGTH];
				/**
				 * \~russian
				 * @brief Метод проверки объявленного свойства контейнера
				 *
				 * @param flag проверяемое свойство контейнера
				 * @return     признак объявленности свойства
				 *
				 * \~english
				 * @brief Method of the checking of a declared property of a container
				 * @param flag property of the container being checked
				 * @return sign of the declaration of the property
				 *
				 * \~
				 */
				[[nodiscard]] bool is(const flag_t flag) const noexcept;
				/**
				 * \~russian
				 * @brief Метод объявления свойства контейнера
				 *
				 * @param flag  объявляемое свойство контейнера
				 * @param value устанавливаемое значение свойства
				 *
				 * \~english
				 * @brief Method of the declaration of a property of a container
				 * @param flag property of the container being declared
				 * @param value value of the property being set
				 *
				 * \~
				 */
				void set(const flag_t flag, const bool value) noexcept;
				/**
				 * \~russian
				 * @brief Метод укладки заголовка в октеты
				 *
				 * @details Контрольная сумма считается по уложенным октетам и кладётся в
				 * последние восемь: считать её раньше значило бы считать по полям, а не по
				 * тому, что ляжет на носитель
				 *
				 * @param result буфер, куда следует уложить заголовок
				 *
				 * \~english
				 * @brief Method of the laying of a header into the octets
				 * @details The checksum is computed over the laid octets and is placed into
				 * the last eight: to compute it earlier would mean to compute it over the fields rather than over
				 * that which will lie on the medium
				 * @param result buffer the header should be laid into
				 *
				 * \~
				 */
				void pack(vector <uint8_t> & result) const noexcept;
				/**
				 * \~russian
				 * @brief Метод снятия заголовка с октетов
				 *
				 * @param buffer буфер поданных октетов
				 * @param size   размер поданных октетов
				 * @param error  код отказа, если снять заголовок не удалось
				 * @return       признак успешно снятого заголовка
				 *
				 * \~english
				 * @brief Method of the taking of a header from the octets
				 * @param buffer buffer of the submitted octets
				 * @param size size of the submitted octets
				 * @param error error code if the header could not be taken
				 * @return sign of a successfully taken header
				 *
				 * \~
				 */
				[[nodiscard]] bool unpack(const void * buffer, const size_t size, error_t & error) noexcept;
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
				Header() noexcept;
			} header_t;

			/**
			 * \~russian
			 * @brief Функция быстрой проверки поданных октетов на признак контейнера
			 *
			 * @details Проверка эта служит опознанию до загрузки: она читает лишь длину
			 * заголовка и отвечает, наш ли это контейнер, не разбирая полей его
			 *
			 * @param buffer буфер поданных октетов
			 * @param size   размер поданных октетов
			 * @return       признак того, что октеты начинают контейнер
			 *
			 * \~english
			 * @brief Function of the quick checking of the submitted octets for the sign of a container
			 * @details This check serves the identification before the loading: it reads only the length
			 * of the header and answers whether this is our container without parsing its fields
			 * @param buffer buffer of the submitted octets
			 * @param size size of the submitted octets
			 * @return sign that the octets begin a container
			 *
			 * \~
			 */
			[[nodiscard]] __AWH_SHARED_EXPORT__ bool probe(const void * buffer, const size_t size) noexcept;
		};
	};
};

/**
 * Возвращаем снятые ранее макросы
 */
#include "../../sys/macro_pop.hpp"

#endif // __AWH_CODEC_ABC_HEADER__
