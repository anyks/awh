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
#ifndef __AWH_CODEC_ABC_CHUNK__
#define __AWH_CODEC_ABC_CHUNK__

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
 * Подключаем заголовочные файлы проекта
 */
#include "../../sys/log.hpp"
#include "../../compressor/block.hpp"
#include "../../cryptography/crypto.hpp"

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
			 * @brief Длина заголовка кадра в октетах
			 *
			 * \~english
			 * @brief Length of the header of a chunk in octets
			 *
			 * \~
			 */
			constexpr size_t CHUNK_HEADER = 32;

			/**
			 * \~russian
			 * @brief Смещение разрядов свойств в заголовке кадра
			 *
			 * @details Смещение это открыто наружу ради пометки кадра мусором: правка
			 * одного октета дешевле перезаписи кадра, какая обязала бы шифровать
			 * содержимое наново, ничего в нём не меняя
			 *
			 * \~english
			 * @brief Offset of the bits of the properties in the header of a chunk
			 * @details This offset is opened outwards for the sake of the marking of a chunk as a waste: an editing
			 * of one octet is cheaper than a rewriting of the chunk which would oblige one to encrypt
			 * the content anew without changing anything in it
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
			 * входит. Разряд мусора при выработке снимается: он учётный, метится правкой
			 * одного октета уже уложенного кадра, и сумма ему следовать не обязана
			 *
			 * @details Заведена она затем, что кадр иначе ничем не заверен: заголовок
			 * контейнера свою сумму несёт, а кадр нёс лишь объявленные длины, и порча октета
			 * внутри кадра проходила МОЛЧА. Развёртка 25.08.2026 дала 24 молчаливо неверных
			 * чтения из 530 при порче одного лишь кадра оглавления, а он ведёт выборку по
			 * всему контейнеру
			 *
			 * \~english
			 * @brief Offset of the checksum in the header of a chunk
			 * @details The sum covers the header of the chunk together with its content while ITSELF is not
			 * included in it. The bit of the waste is cleared upon the production: it is an accounting one
			 * @details It has been introduced because otherwise a chunk is certified by nothing: a corruption
			 * of an octet inside a chunk passed SILENTLY — 24 silently wrong readings of 530 by the sweep
			 * of 25.08.2026 over the index chunk alone
			 *
			 * \~
			 */
			constexpr size_t CHUNK_DIGEST = 24;

			/**
			 * \~russian
			 * @brief Виды содержимого кадра
			 *
			 * @details Вид содержимого выбирает метод сжатия: у знакового текста он один, у
			 * сырых октетов иной, у однородных чисел третий. Подбор этот стоит того лишь при
			 * однородном кадре: в кадре вперемешку всякий метод покажет среднее, и выбирать
			 * будет не из чего
			 *
			 * \~english
			 * @brief Kinds of the content of a chunk
			 * @details The kind of the content chooses the method of the compression: for a character text it is one, for
			 * raw octets another, for homogeneous numbers a third one. This selection is worth it only with
			 * a homogeneous chunk: in a chunk of a mixture every method will show an average, and there will be
			 * nothing to choose from
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
				// Метод сжатия содержимого кадра
				compressor::method_t method;
				// Длина уложенного содержимого кадра в октетах
				uint32_t length;
				// Длина исходного содержимого кадра в октетах
				uint32_t origin;
				// Порядковый номер кадра
				uint64_t number;
				// Поколение записи кадра
				uint32_t generation;
				// Признак того, что содержимое кадра зашифровано
				bool encrypted;
				/**
				 * \~russian
				 * Признак того, что кадр обращён правкой в мусор
				 *
				 * @note Мусорный кадр при подрядном чтении пропускается: он остаётся на
				 * носителе лишь до уборки, а записей более не несёт
				 *
				 * \~english
				 * Flag that a chunk has been turned into a waste by an editing
				 * @note A waste chunk is skipped at a consecutive reading: it remains on
				 * the medium only until a compaction and carries records no more
				 *
				 * \~
				 */
				bool waste;
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
				Chunk() noexcept :
				 method(compressor::method_t::NONE), length(0), origin(0),
				 number(0), generation(0), encrypted(false), waste(false) {}
			} chunk_t;

			/**
			 * \~russian
			 * @brief Класс укладки и снятия кадра
			 *
			 * @details Кадр самоограничен: он несёт свой метод сжатия, длину и порядковый
			 * номер. Оттого кадры сжимаются и шифруются поодиночке, а не сплошным потоком, и
			 * читать до конца ради одного кадра не приходится
			 *
			 * @details **Сжатие и шифрование ведутся отданными извне работами.** Модуль
			 * сжатия и модуль шифрования заводятся потребителем и настраиваются им же: у них
			 * свои зависимости, и кодек их за потребителя не заводит
			 *
			 * @note Всякий кадр шифруется своим вызовом, а не общим потоком. Вызов этот
			 * заводит свой вектор инициализации: общий поток на все кадры повторил бы его при
			 * перезаписи кадра, а повтор вектора при том же ключе - это взлом, и по работе
			 * программы он не виден вовсе
			 *
			 * \~english
			 * @brief Class of the laying and the taking of a chunk
			 * @details A chunk is self-delimited: it carries its own method of the compression, length and ordinal
			 * number. Therefore the chunks are compressed and encrypted one by one rather than by a solid stream, and
			 * one does not have to read to the end for the sake of one chunk
			 * @details **The compression and the encryption are conducted by the works given from outside.** The module
			 * of the compression and the module of the encryption are created by the consumer and are configured by it as well: they have
			 * their own dependencies, and the codec does not create them for the consumer
			 * @note Every chunk is encrypted by its own call rather than by a common stream. This call
			 * creates its own initialization vector: a common stream for all the chunks would repeat it at
			 * a rewriting of a chunk, while a repetition of the vector with the same key is a break, and by the work
			 * of the program it is not visible at all
			 *
			 * \~
			 */
			/**
			 * \~russian
			 * @brief Функция выработки контрольной суммы кадра
			 *
			 * @details Сумма кроет заголовок кадра вместе с содержимым его, а САМА в неё не
			 * входит. Разряд мусора при выработке снимается: он учётный, метится правкой
			 * одного октета уже уложенного кадра, и сумма ему следовать не обязана
			 *
			 * @note Работа открыта наружу ради тех, кто правит кадр НА МЕСТЕ: всякая правка
			 * содержимого обязана обновить сумму, иначе снятие кадра ответит отказом
			 *
			 * @param buffer буфер кадра целиком, от заголовка его
			 * @param size   размер кадра в октетах
			 * @return       выработанная контрольная сумма кадра
			 *
			 * \~english
			 * @brief Function of the production of the checksum of a chunk
			 * @details The sum covers the header of the chunk together with its content while ITSELF is not
			 * included in it. The bit of the waste is cleared upon the production
			 * @note The work is opened outwards for those who edit a chunk IN PLACE
			 * @param buffer buffer of the whole chunk, from its header
			 * @param size size of the chunk in octets
			 * @return produced checksum of the chunk
			 *
			 * \~
			 */
			[[nodiscard]] __AWH_SHARED_EXPORT__ uint64_t digest(const void * buffer, const size_t size) noexcept;

			/**
			 * \~russian
			 * @brief Функция обёртки уложенной записи кадром
			 *
			 * @details Заголовок кадра ставится ВПЕРЕДИ поданной записи, а сама она остаётся
			 * как есть. Служит записи подписи владельца: тело контейнера обходится кадрами
			 * подряд, и всё, что лежит внутри обхода, обязано быть кадром. Без обёртки место
			 * записи подписи приходилось бы затирать кадром-заглушкой при следующей фиксации,
			 * а между затиранием и записью головного заголовка оставалось окно, где обрыв
			 * губил подпись прежнего поколения
			 *
			 * @note Кадр метится МУСОРОМ: содержимое его записью контейнера не является, и
			 * подрядное чтение обязано его пропустить
			 *
			 * @note Отказом отвечается запись, в кадр не вмещающаяся: длина кадра объявлена
			 * четырьмя октетами, и запись длиннее легла бы в них усечённой МОЛЧА
			 *
			 * @param result     буфер уложенной записи, обёртываемой кадром
			 * @param number     порядковый номер кадра
			 * @param generation поколение записи кадра
			 * @return           признак успешной обёртки записи кадром
			 *
			 * \~english
			 * @brief Function of the wrapping of a laid record by a chunk
			 * @details The header of the chunk is put IN FRONT of the submitted record while the record
			 * itself is left as is. It serves the record of the signature of the owner: the body of a container
			 * is walked by the chunks in a row, and everything lying inside the walk is obliged to be a chunk
			 * @note The chunk is marked as a WASTE: its content is not a record of the container
			 * @param result buffer of the laid record being wrapped by a chunk
			 * @param number ordinal number of the chunk
			 * @param generation generation of the record of the chunk
			 * @return true if the record has been wrapped by a chunk
			 *
			 * \~
			 */
			[[nodiscard]] __AWH_SHARED_EXPORT__ bool envelope(vector <uint8_t> & result, const uint64_t number, const uint32_t generation) noexcept;

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
						 * Длина, ниже которой сжатие не выполняется вовсе
						 *
						 * @note Сжатие мелкого кадра не окупается: заголовок метода и словарь
						 * съедают больше, чем сберегают
						 *
						 * \~english
						 * Length below which the compression is not performed at all
						 * @note The compression of a small chunk does not pay off: the header of the method and the dictionary
						 * eat up more than they save
						 *
						 * \~
						 */
						size_t threshold;
						/**
						 * \~russian
						 * Вид хэш-суммы выработки ключа шифрования
						 *
						 * @note Вид шифра и вид хэш-суммы ставит потребитель, а не кодек: чем
						 * шифровать своё содержимое, ведомо тому, кто контейнер завёл
						 *
						 * \~english
						 * Kind of the hash sum of the derivation of the key of the encryption
						 * @note The kind of the cipher and the kind of the hash sum are set by the consumer rather than by the codec: by what
						 * to encrypt its own content is known to the one who created the container
						 *
						 * \~
						 */
						crypto_t::hash_t hash;
						// Размер шифрования содержимого кадра
						crypto_t::cipher_t cipher;
						// Признак шифрования содержимого кадра
						bool encrypt;
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
					// Настройки укладки кадра
					settings_t _settings;
				private:
					// Код отказа укладки либо снятия кадра
					error_t _error;
				private:
					// Модуль сжатия, отданный потребителем
					const compressor::block_t * _compressor;
				private:
					// Модуль шифрования, отданный потребителем
					const crypto_t * _crypto;
				protected:
					// Объект работы с логами
					const log_t * _log;
				private:
					/**
					 * \~russian
					 * @brief Метод объявления отказа укладки либо снятия кадра
					 *
					 * @details Донесение идёт отсюда, из единственного места объявления отказа:
					 * работа отвечает отказом множеством путей, и запись в каждом из них
					 * разошлась бы с прочими. Отказ, ПРИНЯТЫЙ от нижнего слоя, сюда не идёт -
					 * тот слой донёс о нём сам, и второе донесение лишь двоило бы записи
					 *
					 * @param error объявляемый код отказа
					 * @return      признак успешности, всегда ложь
					 *
					 * \~english
					 * @brief Method of the declaration of a failure
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
					 * @param kind kind of the content of the chunk
					 * @return method of the compression of the content
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
				public:
					/**
					 * \~russian
					 * @brief Метод укладки кадра
					 *
					 * @details Сжатие, не давшее выигрыша, отбрасывается: кадр ложится сырым, а
					 * методом его объявляется отсутствие сжатия. Иначе мелкое содержимое росло
					 * бы от заголовков сжатия, а разбор о том не узнал бы вовсе
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
					 * @details A compression which has not given a gain is discarded: the chunk is laid raw, while
					 * its method is declared to be the absence of the compression. Otherwise a small content would grow
					 * from the headers of the compression, and the parsing would not learn of it at all
					 * @param buffer buffer of the content being laid
					 * @param size size of the content being laid in octets
					 * @param kind kind of the content being laid
					 * @param number ordinal number of the chunk
					 * @param generation generation of the record of the chunk
					 * @param result buffer the chunk should be laid into
					 * @return sign of the success of the laying
					 *
					 * \~
					 */
					[[nodiscard]] bool pack(const void * buffer, const size_t size, const payload_t kind,
					 const uint64_t number, const uint32_t generation, vector <uint8_t> & result) noexcept;
					/**
					 * \~russian
					 * @brief Метод снятия кадра
					 *
					 * @details Кадр, поданный не целиком, смещения не сдвигает: недостающие
					 * октеты дожидаются следующей подачи
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
					 * @details A chunk submitted not in full does not shift the offset: the missing
					 * octets wait for the next submission
					 * @param buffer buffer of the submitted octets
					 * @param size size of the submitted octets
					 * @param offset offset the chunk should be taken from
					 * @param result buffer the content of the chunk should be placed into
					 * @param chunk taken information about the chunk
					 * @return sign of a successfully taken chunk
					 *
					 * \~
					 */
					[[nodiscard]] bool unpack(const void * buffer, const size_t size, size_t & offset,
					 vector <uint8_t> & result, chunk_t & chunk) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения кода отказа укладки либо снятия кадра
					 *
					 * @return код отказа
					 *
					 * \~english
					 * @brief Method of the extraction of the error code of the laying or of the taking of a chunk
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
					 * @param log объект для работы с логами
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * @param log object for working with logs
					 *
					 * \~
					 */
					explicit Packer(const log_t * log) noexcept;
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
					~Packer() noexcept {}
			} packer_t;
		};
	};
};

/**
 * Возвращаем снятые ранее макросы
 */
#include "../../sys/macro_pop.hpp"

#endif // __AWH_CODEC_ABC_CHUNK__
