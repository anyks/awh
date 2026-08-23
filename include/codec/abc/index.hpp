/**
 * @file index.hpp
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
 * @brief Заголовочный файл оглавления бинарного контейнера ABC
 *
 * \~english
 * @brief Header file of the index of the ABC binary container
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_ABC_INDEX__
#define __AWH_CODEC_ABC_INDEX__

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <cstdint>
#include <cstddef>
#include <functional>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "chunk.hpp"
#include "common.hpp"
#include "header.hpp"

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../sys/log.hpp"

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
			 * @brief Длина строки оглавления в октетах
			 *
			 * \~english
			 * @brief Length of a row of the index in octets
			 *
			 * \~
			 */
			constexpr size_t ENTRY_LENGTH = 24;

			/**
			 * \~russian
			 * @brief Строка оглавления контейнера
			 *
			 * @details Строка указывает не на октеты носителя, а на кадр и место в его
			 * содержимом. Указывать на носитель нельзя: содержимое кадра сжато и
			 * зашифровано, и места записи в нём попросту нет до снятия кадра
			 *
			 * \~english
			 * @brief Row of the index of a container
			 * @details A row points not to the octets of the medium but to a chunk and to a place in its
			 * content. It is impossible to point to the medium: the content of a chunk is compressed and
			 * encrypted, and the place of the record simply does not exist in it until the taking of the chunk
			 *
			 * \~
			 */
			enum class mark_t : uint32_t {
				NONE    = 0x00000000, // Свойств строки не объявлено
				ERASED  = 0x00000001  // Запись снесена правкой контейнера
			};

			/**
			 * \~russian
			 * @brief Строка оглавления контейнера
			 *
			 * \~english
			 * @brief Row of the index of a container
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Entry {
				// Смещение кадра от начала тела контейнера
				uint64_t chunk;
				// Смещение записи в содержимом кадра
				uint32_t offset;
				// Длина записи в октетах
				uint32_t length;
				/**
				 * \~russian
				 * Разряды свойств строки оглавления
				 *
				 * @note Снос записи ведётся разрядом, а не изъятием строки: номера записей
				 * при изъятии сдвинулись бы, и всякая ссылка на них извне обратилась бы в
				 * ссылку на соседа
				 *
				 * \~english
				 * Bits of the properties of a row of the index
				 * @note The erasure of a record is led by a bit rather than by a removal of the row: the numbers of the records
				 * would shift at a removal, and every reference to them from outside would turn into
				 * a reference to a neighbour
				 *
				 * \~
				 */
				uint32_t marks;
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
				Entry() noexcept : chunk(0), offset(0), length(0), marks(static_cast <uint32_t> (mark_t::NONE)) {}
				/**
				 * \~russian
				 * @brief Метод проверки объявленного свойства строки оглавления
				 *
				 * @param mark проверяемое свойство строки
				 * @return     признак объявленности свойства
				 *
				 * \~english
				 * @brief Method of the checking of a declared property of a row of the index
				 * @param mark property of the row being checked
				 * @return sign of the declaration of the property
				 *
				 * \~
				 */
				[[nodiscard]] bool is(const mark_t mark) const noexcept;
				/**
				 * \~russian
				 * @brief Метод объявления свойства строки оглавления
				 *
				 * @param mark  объявляемое свойство строки
				 * @param value устанавливаемое значение свойства
				 *
				 * \~english
				 * @brief Method of the declaration of a property of a row of the index
				 * @param mark property of the row being declared
				 * @param value value of the property being set
				 *
				 * \~
				 */
				void set(const mark_t mark, const bool value) noexcept;
			} entry_t;

			/**
			 * \~russian
			 * @brief Класс оглавления контейнера
			 *
			 * @details Оглавление даёт добраться до одной записи, не разбирая всего
			 * контейнера: по номеру записи оно указывает кадр и место в нём. Без него до
			 * последней записи пришлось бы снять все кадры до неё
			 *
			 * @note Оглавление лежит за телом контейнера, а не перед ним: места записей
			 * известны лишь по укладке кадров, и вести оглавление впереди значило бы
			 * возвращаться к нему правкой на всякую запись
			 *
			 * \~english
			 * @brief Class of the index of a container
			 * @details The index allows one to reach one record without parsing the whole
			 * container: by the number of the record it points to a chunk and to a place in it. Without it in order to reach
			 * the last record one would have to take all the chunks before it
			 * @note The index lies after the body of a container rather than before it: the places of the records
			 * are known only upon the laying of the chunks, and to lead the index in front would mean
			 * to return to it by an editing at every record
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Index {
				private:
					// Строки оглавления контейнера
					vector <entry_t> _entries;
				protected:
					// Объект работы с логами
					const log_t * _log;
				public:
					/**
					 * \~russian
					 * @brief Метод внесения строки оглавления
					 *
					 * @param entry вносимая строка оглавления
					 *
					 * \~english
					 * @brief Method of the adding of a row of the index
					 * @param entry row of the index being added
					 *
					 * \~
					 */
					void add(const entry_t & entry) noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения строк оглавления
					 *
					 * @return строки оглавления контейнера
					 *
					 * \~english
					 * @brief Method of the extraction of the rows of the index
					 * @return rows of the index of the container
					 *
					 * \~
					 */
					const vector <entry_t> & entries() const noexcept;
					/**
					 * \~russian
					 * @brief Метод правки строки оглавления
					 *
					 * @param number номер правимой строки оглавления
					 * @param entry  устанавливаемая строка оглавления
					 * @return       признак успешной правки
					 *
					 * \~english
					 * @brief Method of the editing of a row of the index
					 * @param number number of the row of the index being edited
					 * @param entry row of the index being set
					 * @return sign of a successful editing
					 *
					 * \~
					 */
					[[nodiscard]] bool replace(const uint64_t number, const entry_t & entry) noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения количества строк оглавления
					 *
					 * @return количество строк оглавления
					 *
					 * \~english
					 * @brief Method of the extraction of the count of the rows of the index
					 * @return count of the rows of the index
					 *
					 * \~
					 */
					size_t size() const noexcept;
					/**
					 * \~russian
					 * @brief Метод очистки оглавления контейнера
					 *
					 *
					 * \~english
					 * @brief Method of the clearing of the index of a container
					 *
					 * \~
					 */
					void clear() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод укладки оглавления в октеты
					 *
					 * @param result буфер, куда следует уложить оглавление
					 *
					 * \~english
					 * @brief Method of the laying of an index into the octets
					 * @param result buffer the index should be laid into
					 *
					 * \~
					 */
					void pack(vector <uint8_t> & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод снятия оглавления с октетов
					 *
					 * @param buffer буфер поданных октетов
					 * @param size   размер поданных октетов
					 * @param error  код отказа, если снять оглавление не удалось
					 * @return       признак успешно снятого оглавления
					 *
					 * \~english
					 * @brief Method of the taking of an index from the octets
					 * @param buffer buffer of the submitted octets
					 * @param size size of the submitted octets
					 * @param error error code if the index could not be taken
					 * @return sign of a successfully taken index
					 *
					 * \~
					 */
					[[nodiscard]] bool unpack(const void * buffer, const size_t size, error_t & error) noexcept;
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
					explicit Index(const log_t * log) noexcept : _log(log) {}
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
					~Index() noexcept {}
			} index_t;

			/**
			 * \~russian
			 * @brief Класс выборки записи контейнера по номеру
			 *
			 * @details Выборщик держит в памяти оглавление и один снятый кадр, а не
			 * контейнер целиком: тем большой контейнер и читается на машине, в память
			 * какой он не влезает
			 *
			 * @details **Октеты подаются отданной извне работой.** Откуда они берутся -
			 * из файла, из сети либо из памяти - выборщика не касается, и заводить чтение
			 * файла он не станет
			 *
			 * @note Последний снятый кадр удерживается: соседние записи чаще всего лежат
			 * в одном кадре, и снимать его наново на всякую запись значило бы платить
			 * расшифровкой и разжатием за каждую из них
			 *
			 * \~english
			 * @brief Class of the fetching of a record of a container by its number
			 * @details The fetcher holds in the memory the index and one taken chunk rather than
			 * the whole container: by that a large container is read on a machine into the memory of which
			 * it does not fit
			 * @details **The octets are submitted by a work given from outside.** Where they are taken from —
			 * from a file, from the network or from the memory — does not concern the fetcher, and it will not create
			 * a reading of a file
			 * @note The last taken chunk is retained: the neighbouring records most often lie
			 * in one chunk, and to take it anew at every record would mean to pay
			 * by a decryption and a decompression for each of them
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Fetcher {
				public:
					/**
					 * \~russian
					 * @brief Работа чтения октетов контейнера
					 *
					 * @details Работа обязана вычитать ровно затребованное: недочитанное
					 * значит отказ, а не конец. Конец же виден по заголовку опознания
					 *
					 * \~english
					 * @brief Work of the reading of the octets of a container
					 * @details The work is obliged to read exactly what has been requested: an underread
					 * means a failure rather than an end. While the end is visible by the identifying header
					 *
					 * \~
					 */
					typedef function <bool (const uint64_t, const size_t, vector <uint8_t> &)> source_t;
				private:
					// Снятый заголовок опознания контейнера
					header_t _header;
				private:
					// Снятое оглавление контейнера
					index_t _index;
				private:
					// Модуль снятия кадра
					packer_t _packer;
				private:
					// Код отказа выборки записи
					error_t _error;
				private:
					// Признак открытого контейнера
					bool _opened;
				private:
					// Признак удержания снятого кадра
					bool _cached;
				private:
					// Смещение удерживаемого кадра от начала тела контейнера
					uint64_t _origin;
				private:
					// Содержимое удерживаемого кадра
					vector <uint8_t> _chunk;
				private:
					// Работа чтения октетов контейнера
					source_t _source;
				protected:
					// Объект работы с логами
					const log_t * _log;
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
				public:
					/**
					 * \~russian
					 * @brief Метод открытия контейнера отданной работой чтения
					 *
					 * @param source устанавливаемая работа чтения октетов контейнера
					 * @return       признак успешно открытого контейнера
					 *
					 * \~english
					 * @brief Method of the opening of a container by a given work of the reading
					 * @param source work of the reading of the octets of the container being set
					 * @return sign of a successfully opened container
					 *
					 * \~
					 */
					[[nodiscard]] bool open(source_t source) noexcept;
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
					/**
					 * \~russian
					 * @brief Метод сброса состояния выборки записей
					 *
					 *
					 * \~english
					 * @brief Method of the reset of the state of the fetching of the records
					 *
					 * \~
					 */
					void reset() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения количества записей контейнера
					 *
					 * @return количество записей контейнера
					 *
					 * \~english
					 * @brief Method of the extraction of the count of the records of a container
					 * @return count of the records of the container
					 *
					 * \~
					 */
					uint64_t records() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения снятого заголовка опознания контейнера
					 *
					 * @return снятый заголовок опознания
					 *
					 * \~english
					 * @brief Method of the extraction of the taken identifying header of a container
					 * @return taken identifying header
					 *
					 * \~
					 */
					const header_t & header() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения снятого оглавления контейнера
					 *
					 * @return снятое оглавление контейнера
					 *
					 * \~english
					 * @brief Method of the extraction of the taken index of a container
					 * @return taken index of the container
					 *
					 * \~
					 */
					const index_t & index() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения кода отказа выборки записи
					 *
					 * @return код отказа
					 *
					 * \~english
					 * @brief Method of the extraction of the error code of the fetching of a record
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
					 * @param log объект для работы с логами
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Fetcher(const log_t * log) noexcept;
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
					~Fetcher() noexcept {}
			} fetcher_t;
		};
	};
};

/**
 * Возвращаем снятые ранее макросы
 */
#include "../../sys/macro_pop.hpp"

#endif // __AWH_CODEC_ABC_INDEX__
