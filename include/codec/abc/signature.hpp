/**
 * @file signature.hpp
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
 * @brief Заголовочный файл подписи владельца бинарного контейнера ABC
 *
 * \~english
 * @brief Header file of the signature of the owner of the ABC binary container
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_ABC_SIGNATURE__
#define __AWH_CODEC_ABC_SIGNATURE__

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
#include "common.hpp"
#include "header.hpp"

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../sys/log.hpp"
#include "../../cryptography/crypto.hpp"

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
			 * @brief Длина заголовка записи подписи в октетах
			 *
			 * @details **Раскладка записи подписи по октетам.** Обе длины уложены от МЛАДШЕГО
			 * октета к старшему:
			 *
			 * @li `0` (1) - вид подписи владельца, величина `crypto_t::signature_t`;
			 * @li `1` (1) - вид хэш-суммы, какой подпись выработана, `crypto_t::hash_t`;
			 * @li `2` (2) - длина октетов ПОДПИСИ;
			 * @li `4` (2) - длина КОРНЯ дерева свёрток, всегда `DIGEST_LENGTH`;
			 * @li `6` (2) - оставлено впрок, обязано быть нулевым.
			 *
			 * @warning За заголовком идёт СНАЧАЛА корень дерева свёрток, а ЗА НИМ октеты
			 * подписи - череда эта обратна череде длин их в заголовке. Обе длины кладутся двумя
			 * октетами, и запись шире `0xFFFF` не укладывается вовсе: сторож стоит у зовущего,
			 * а укладка оставляет буфер пустым
			 *
			 * @note Хвостовые октеты требуются нулевыми по тому же доводу, что и у строки
			 * оглавления (`ENTRY_LENGTH`): поле приходит с провода и обязано быть опознано, иначе
			 * занятые кем-то октеты проходят молча, а запись числится понятой целиком. Ценою тому
			 * - невозможность занять их подъёмом одной лишь младшей версии вида записи
			 *
			 * @warning Записана раскладка эта 01.09.2026 СНЯТИЕМ С РЕАЛИЗАЦИИ, а не наоборот, и
			 * опорою для сличения служить не может: судья, писанный по ней, повторил бы
			 * реализацию вместе с её ошибками
			 *
			 * \~english
			 * @brief Length of the header of the record of the signature in octets
			 *
			 * \~
			 */
			constexpr size_t SIGNATURE_HEADER = 8;

			/**
			 * \~russian
			 * @brief Длина свёртки в октетах
			 *
			 * @details Длина отвечает свёртке SHA-256: дерево свёрток считается ею, а не
			 * видом хэша подписи. Виды эти разные и разойтись им незачем - у Ed25519 и
			 * ГОСТ вид хэша подписи вовсе не задаётся, а дерево считать чем-то надо
			 *
			 * \~english
			 * @brief Length of a digest in octets
			 * @details The length corresponds to the SHA-256 digest: the tree of the digests is computed by it rather than
			 * by the kind of the hash of the signature. These kinds are different and there is no reason for them to diverge — at Ed25519 and
			 * GOST the kind of the hash of the signature is not set at all, while the tree has to be computed by something
			 *
			 * \~
			 */
			constexpr size_t DIGEST_LENGTH = 32;

			/**
			 * \~russian
			 * @brief Класс дерева свёрток по кадрам контейнера
			 *
			 * @details Дерево сводит свёртки всех кадров к одному корню, и подписывается
			 * корень, а не поток. Оттого ПОДПИСЫВАЕМОЕ всегда занимает тридцать два октета,
			 * сколь бы ни был велик контейнер, и поточная подача подписи не нужна вовсе -
			 * а у Ed25519 её и нет по устройству схемы
			 *
			 * @note Речь тут о ДЛИНЕ ПОДПИСЫВАЕМОГО, а не самой подписи: та зависит от схемы,
			 * длиною своею объявляется двумя октетами записи подписи (см. `SIGNATURE_HEADER`)
			 * и тридцати двум октетам не равна. Величины по схемам объявлены перечнем
			 * `crypto_t::signature_t` («cryptography/crypto.hpp») и здесь не повторяются:
			 * повторённое число расходится с исходным молча. Прежняя редакция этой записки
			 * читалась так, будто тридцать два октета занимает подпись, и поправлена 01.09.2026
			 *
			 * @details **Лист и узел считаются с разными приставками.** Без них узел
			 * можно было бы выдать за лист: подобрав кадр, чьи октеты равны паре свёрток,
			 * поддельщик получил бы тот же корень при иной череде кадров
			 *
			 * @details **Уклад свёрток по октетам.** Приставка стоит ПЕРВЫМ октетом того, что
			 * подаётся хэшу, и величины её таковы: `0x00` у листа, `0x01` у узла. Лист есть
			 * `SHA256(0x00 || октеты кадра)`, узел - `SHA256(0x01 || свёртка левой ветви ||
			 * свёртка правой ветви)`, обе свёртки по тридцать два октета. Вид хэша дерева
			 * закреплён SHA-256 и виду хэша ПОДПИСИ не следует - о том сказано у
			 * `DIGEST_LENGTH`. Октеты кадра берутся С НАДЕТЫМ заголовком кадра, но разряд
			 * мусора `CHUNK_WASTE` в признаках его перед выработкой снимается - довод тому
			 * изложен у `CHUNK_FLAGS`
			 *
			 * @note Уклад этот записан здесь 01.09.2026, а прежде жил ЛИШЬ В РЕАЛИЗАЦИИ, в
			 * безымянном пространстве имён `signature.cpp`. Заголовок объявлял, что приставки
			 * РАЗНЫЕ, но не какие они и где стоят, и поверщик на чужой стороне корня дерева
			 * повторить не мог ничем - а поверка подписи в том и состоит
			 *
			 * @warning Записан он СНЯТИЕМ С РЕАЛИЗАЦИИ, а не наоборот. Опорою для сличения
			 * служить не может: судья, писанный по такой записи, повторил бы реализацию вместе
			 * с её ошибками. Опорою он станет, когда переживёт правку реализации, им не
			 * подтверждённую
			 *
			 * @note Нечётная свёртка яруса поднимается на ярус выше без пары, а не
			 * сдваивается сама с собой: сдваивание позволяет двум разным чередам кадров
			 * дать один корень
			 *
			 * \~english
			 * @brief Class of the tree of the digests over the chunks of a container
			 * @details The tree reduces the digests of all the chunks to one root, and the root is signed
			 * rather than the stream. Therefore the signature always lies on thirty-two octets
			 * however large the container may be, and a streaming submission of the signature is not needed at all —
			 * while at Ed25519 there is none by the structure of the scheme
			 * @details **A leaf and a node are computed with different prefixes.** Without them a node
			 * could be passed off as a leaf: having picked a chunk whose octets equal a pair of digests,
			 * a forger would obtain the same root at a different series of the chunks
			 * @note An odd digest of a tier is raised to the tier above without a pair rather than
			 * being doubled with itself: a doubling allows two different series of the chunks
			 * to give one root
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Merkle {
				private:
					// Модуль шифрования, отданный потребителем
					const crypto_t * _crypto;
				private:
					// Свёртки кадров контейнера
					vector <vector <uint8_t>> _leaves;
				protected:
					// Объект работы с логами
					const log_t * _log;
				private:
					/**
					 * \~russian
					 * @brief Метод объявления отказа работы с деревом свёрток
					 *
					 * @param message текст объявляемого отказа
					 * @return        признак успешности, всегда ложь
					 *
					 * \~english
					 * @brief Method of the declaration of a failure
					 *
					 * @param message text of the failure being declared
					 * @return        flag of the success, always false
					 *
					 * \~
					 */
					bool fail(const char * message) const noexcept;
				public:
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
					 * @brief Метод внесения кадра свёрткой в дерево
					 *
					 * @details Кадр вносится тем видом, каким лёг на носитель: подписывается
					 * шифротекст, а не открытый текст. Иначе поверка обязала бы расшифровку,
					 * и подписанным оказалось бы не то, что лежит
					 *
					 * @param buffer буфер октетов вносимого кадра
					 * @param size   размер октетов вносимого кадра
					 * @return       признак успешного внесения
					 *
					 * \~english
					 * @brief Method of the adding of a chunk as a digest into the tree
					 * @details A chunk is added in the kind in which it lay onto the medium: the ciphertext is signed
					 * rather than the plaintext. Otherwise the checking would oblige a decryption,
					 * and it would turn out that not what lies is signed
					 * @param buffer buffer of the octets of the chunk being added
					 * @param size size of the octets of the chunk being added
					 * @return sign of a successful adding
					 *
					 * \~
					 */
					[[nodiscard]] bool add(const void * buffer, const size_t size) noexcept;
					/**
					 * \~russian
					 * @brief Метод сведения дерева к корню
					 *
					 * @param result буфер, куда следует положить корень дерева
					 * @return       признак успешно сведённого дерева
					 *
					 * \~english
					 * @brief Method of the reduction of the tree to the root
					 * @param result buffer the root of the tree should be placed into
					 * @return sign of a successfully reduced tree
					 *
					 * \~
					 */
					[[nodiscard]] bool root(vector <uint8_t> & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод сведения дерева к корню с приданной свёрткой
					 *
					 * @details Приданный кадр в дерево не оседает, а лишь участвует в
					 * сведении. Так берётся кадр оглавления: он подписывается наравне с
					 * телом, но при следующей фиксации ложится наново, и оседи он в дереве -
					 * дерево пришлось бы править задним числом
					 *
					 * @param result буфер, куда следует положить корень дерева
					 * @param buffer буфер октетов приданного кадра
					 * @param size   размер октетов приданного кадра
					 * @return       признак успешно сведённого дерева
					 *
					 * \~english
					 * @brief Method of the reduction of the tree to the root with an attached digest
					 * @details The attached chunk does not settle in the tree but only participates in
					 * the reduction. Thus the chunk of the index is taken: it is signed on a par with
					 * the body, but at the next commit it is laid anew, and were it to settle in the tree —
					 * the tree would have to be corrected after the fact
					 * @param result buffer the root of the tree should be placed into
					 * @param buffer buffer of the octets of the attached chunk
					 * @param size size of the octets of the attached chunk
					 * @return sign of a successfully reduced tree
					 *
					 * \~
					 */
					[[nodiscard]] bool root(vector <uint8_t> & result, const void * buffer, const size_t size) const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения количества внесённых кадров
					 *
					 * @return количество внесённых кадров
					 *
					 * \~english
					 * @brief Method of the extraction of the count of the added chunks
					 * @return count of the added chunks
					 *
					 * \~
					 */
					size_t leaves() const noexcept;
					/**
					 * \~russian
					 * @brief Метод очистки дерева свёрток
					 *
					 *
					 * \~english
					 * @brief Method of the clearing of the tree of the digests
					 *
					 * \~
					 */
					void clear() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param log объект для работы с логами
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * @param log object for working with logs
					 *
					 * \~
					 */
					explicit Merkle(const log_t * log) noexcept :
					 _crypto(nullptr), _log(log) {}
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
					~Merkle() noexcept {}
			} merkle_t;

			/**
			 * \~russian
			 * @brief Снятые сведения о подписи контейнера
			 *
			 * \~english
			 * @brief Taken information about the signature of a container
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Sign {
				// Вид подписи владельца контейнера
				crypto_t::signature_t kind;
				// Вид хэш-суммы, какой подпись выработана
				crypto_t::hash_t hash;
				// Корень дерева свёрток по кадрам контейнера
				vector <uint8_t> root;
				// Октеты подписи владельца контейнера
				vector <uint8_t> signature;
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
				Sign() noexcept :
				 kind(crypto_t::signature_t::NONE), hash(crypto_t::hash_t::NONE) {}
			} sign_t;

			/**
			 * \~russian
			 * @brief Функция подбора вида хэш-суммы под вид подписи
			 *
			 * @details У Ed25519 хэш-суммы нет вовсе, у ГОСТ она предписана самой схемой, и
			 * поданная отвергается. У RSA и ECDSA она обязательна. Оттого вид её ставится
			 * не потребителем напрямую, а подбором по виду ключа: поданный не тому виду
			 * обратился бы в отказ подписи посреди фиксации
			 *
			 * @param kind вид подписи владельца контейнера
			 * @param hash желаемый вид хэш-суммы
			 * @return     вид хэш-суммы, годный виду подписи
			 *
			 * \~english
			 * @brief Function of the selection of the kind of the hash for the kind of the signature
			 * @details At Ed25519 there is no hash at all, at GOST it is prescribed by the scheme itself, and
			 * a submitted one is rejected. At RSA and ECDSA it is obligatory. Therefore its kind is set
			 * not by the consumer directly but by a selection according to the kind of the key: one submitted to the wrong kind
			 * would turn into a refusal of the signature in the middle of a commit
			 * @param kind kind of the signature of the owner of the container
			 * @param hash desired kind of the hash
			 * @return kind of the hash fit for the kind of the signature
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ crypto_t::hash_t digest(const crypto_t::signature_t kind,
			 const crypto_t::hash_t hash) noexcept;
			/**
			 * \~russian
			 * @brief Функция укладки записи подписи контейнера
			 *
			 * @warning Отказ укладки объявляется ВЫДАЧЕЙ, и поверять его зовущий обязан, оттого
			 * выдача помечена `[[nodiscard]]`: половину эту набором не постеречь - понадобилась бы
			 * схема подписи шире 64 КиБ, каких сегодня нет вовсе, - и сторожем её ставится
			 * собиратель. Пропуск выдачи мимо был бы: запись,
			 * не уложенная, остаётся пустой, а обёртка кадром пустое содержимое принимает
			 * безропотно. Контейнер вышел бы объявленным подписанным, кадр подписи -
			 * безупречным по виду, а отказ всплыл бы лишь у читающего, да ещё и не тем кодом.
			 * Замерено щупом 03.09.2026: `internal parsing error` вместо обрыва подписи
			 *
			 * @param sign   укладываемая подпись контейнера
			 * @param result буфер, куда следует уложить запись подписи
			 * @return       признак успешно уложенной записи подписи
			 *
			 * \~english
			 * @brief Function of the laying of the record of the signature of a container
			 * @param sign signature of the container being laid
			 * @param result buffer the record of the signature should be laid into
			 * @return flag of the successfully laid record of the signature
			 *
			 * \~
			 */
			[[nodiscard]] __AWH_SHARED_EXPORT__ bool pack(const sign_t & sign, vector <uint8_t> & result) noexcept;
			/**
			 * \~russian
			 * @brief Функция снятия записи подписи контейнера
			 *
			 * @param buffer буфер поданных октетов
			 * @param size   размер поданных октетов
			 * @param sign   снятая подпись контейнера
			 * @param error  код отказа, если снять подпись не удалось
			 * @return       признак успешно снятой подписи
			 *
			 * \~english
			 * @brief Function of the taking of the record of the signature of a container
			 * @param buffer buffer of the submitted octets
			 * @param size size of the submitted octets
			 * @param sign taken signature of the container
			 * @param error error code if the signature could not be taken
			 * @return sign of a successfully taken signature
			 *
			 * \~
			 */
			[[nodiscard]] __AWH_SHARED_EXPORT__ bool unpack(const void * buffer, const size_t size,
			 sign_t & sign, error_t & error) noexcept;
			/**
			 * \~russian
			 * @brief Функция выработки отпечатка открытого ключа владельца
			 *
			 * @details Отпечаток усекается от начала до длины, отведённой заголовком.
			 * Усечение записано решением: тот, кто станет сличать его с полным отпечатком
			 * чужой работы, обязан знать, какая это половина
			 *
			 * @note Усечённый отпечаток есть опознание владельца, а не привязка к нему:
			 * стойкость к подбору пары падает вдвое от полной. Что контейнер именно от
			 * владельца, доказывает подпись, а не отпечаток
			 *
			 * @param crypto модуль шифрования, отданный потребителем
			 * @param name   имя ключа владельца контейнера
			 * @param result буфер, куда следует положить усечённый отпечаток
			 * @return       признак успешно выработанного отпечатка
			 *
			 * \~english
			 * @brief Function of the production of the fingerprint of the public key of the owner
			 * @details The fingerprint is truncated from the beginning to the length allotted by the header.
			 * The truncation is recorded as a decision: the one who will compare it with a full fingerprint
			 * of a foreign work is obliged to know which half it is
			 * @note A truncated fingerprint is an identification of the owner rather than a binding to it:
			 * the resistance to the picking of a pair falls by half from the full one. That the container is exactly from
			 * the owner is proven by the signature rather than by the fingerprint
			 * @param crypto module of the encryption given by the consumer
			 * @param name name of the key of the owner of the container
			 * @param result buffer the truncated fingerprint should be placed into
			 * @return sign of a successfully produced fingerprint
			 *
			 * \~
			 */
			[[nodiscard]] __AWH_SHARED_EXPORT__ bool fingerprint(const crypto_t & crypto, const string & name,
			 vector <uint8_t> & result) noexcept;
		};
	};
};

#endif // __AWH_CODEC_ABC_SIGNATURE__
