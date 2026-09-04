/**
 * @file storage.hpp
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
 * @brief Заголовочный файл хранения бинарного контейнера ABC в файле
 *
 * \~english
 * @brief Header file of the storing of the ABC binary container in a file
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_ABC_STORAGE__
#define __AWH_CODEC_ABC_STORAGE__

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>
#include <vector>
#include <string>
#include <cstdint>
#include <cstddef>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"
#include "editor.hpp"
#include "container.hpp"

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../sys/log.hpp"

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
			 * @brief Класс хранения контейнера в файле
			 *
			 * @details Правка контейнера на месте ведётся работами чтения и записи октетов,
			 * какие потребитель подаёт правке сам. Хранилище это их и подаёт, избавляя
			 * потребителя от писания их всякий раз заново
			 *
			 * @details **Кодек файлов не открывает сам.** Носитель у контейнера бывает
			 * всякий - файл, память, сеть, - и правка знает о нём лишь две работы. Оттого
			 * хранилище стоит отдельным классом, а не полем правки: потребителю, чей
			 * носитель не файл, оно не нужно вовсе, а собранная библиотека его не несёт
			 *
			 * \~english
			 * @brief Class of the storing of a container in a file
			 * @details The editing of a container in place is conducted by the works of the reading and of the writing
			 * of the octets which the consumer submits to the editing itself. This storage submits them, relieving
			 * the consumer of writing them anew every time
			 * @details **The codec does not open the files itself.** The medium of a container may be
			 * any — a file, a memory, a network, — and the editing knows only two works about it. Whereby
			 * the storage stands as a separate class rather than as a field of the editing: a consumer whose
			 * medium is not a file does not need it at all
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Storage {
				public:
					/**
					 * \~russian
					 * @brief Способы сброса записанного на носитель
					 *
					 * @details Разница способов видна ЛИШЬ у систем Apple. Там `fsync` выносит
					 * записанное из ядра в НАКОПИТЕЛЬ, но опустошить вместилище самого накопителя
					 * не велит, и обрыв питания записанное теряет; доводит до пластины только
					 * `F_FULLFSYNC`. У прочих систем `fsync` доводит сам, и способы там равны
					 *
					 * @note Цена полного сброса высока и замерена: 200 кругов по 4 КиБ на macOS
					 * ARM64 (02.09.2026) дали 43 мкс на `fsync` против 4604 мкс на `F_FULLFSYNC` -
					 * в сто шесть раз дороже, сиречь 4.6 мс на всякую фиксацию правки
					 *
					 * \~english
					 * @brief Ways of the flushing of the written to the medium
					 * @details The difference of the ways is visible ONLY at the systems of Apple
					 *
					 * \~
					 */
					enum class sync_t : uint8_t {
						FULL  = 0x00, // Полный сброс, доводящий записанное до пластины
						PLAIN = 0x01  // Сброс из ядра накопителю, обрыва питания не переживающий
					};
				private:
					// Название файла контейнера
					string _filename;
				private:
					/**
					 * \~russian
					 * Способ сброса записанного на носитель
					 *
					 * @note Умолчанием взят ПОЛНЫЙ сброс: обещание «правки ложатся на носитель
					 * фиксацией» без него не исполняется у систем Apple, а умолчание обязано
					 * держать обещание, а не быстроту. Кому тысяча фиксаций в секунду дороже
					 * переживания обрыва питания - тот снимет его сам
					 *
					 * \~english
					 * Way of the flushing of the written to the medium
					 *
					 * \~
					 */
					sync_t _sync;
				private:
					// Поток файла контейнера
					FILE * _stream;
				private:
					// Полная длина контейнера на носителе
					uint64_t _length;
				private:
					// Код отказа работы с файлом контейнера
					error_t _error;
				private:
					/**
					 * \~russian
					 * @brief Метод перевода потока файла на заданное смещение
					 *
					 * @param offset смещение в файле контейнера
					 * @return       признак успешности перевода
					 *
					 * \~english
					 * @brief Method of the moving of the stream of the file to a given offset
					 * @param offset offset in the file of the container
					 * @return sign of the success of the moving
					 *
					 * \~
					 */
					[[nodiscard]] bool seek(const uint64_t offset) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод открытия существующего файла контейнера
					 *
					 * @details Файл открывается на чтение вместе с записью: правка контейнера
					 * ведётся на месте, и одного чтения ей мало
					 *
					 * @param filename название открываемого файла контейнера
					 * @return         признак успешно открытого файла
					 *
					 * \~english
					 * @brief Method of the opening of an existing file of a container
					 * @details The file is opened for the reading together with the writing: the editing of a container
					 * is conducted in place, and the reading alone is not enough for it
					 * @param filename name of the file of the container being opened
					 * @return sign of a successfully opened file
					 *
					 * \~
					 */
					[[nodiscard]] bool open(const string & filename) noexcept;
					/**
					 * \~russian
					 * @brief Метод заведения нового файла контейнера
					 *
					 * @note Файл, стоявший под этим названием прежде, усекается до пустого:
					 * заведение нового контейнера поверх старого есть замена его целиком
					 *
					 * @param filename название заводимого файла контейнера
					 * @return         признак успешно заведённого файла
					 *
					 * \~english
					 * @brief Method of the creation of a new file of a container
					 * @note A file which stood under this name before is truncated to an empty one:
					 * the creation of a new container over an old one is its replacement as a whole
					 * @param filename name of the file of the container being created
					 * @return sign of a successfully created file
					 *
					 * \~
					 */
					[[nodiscard]] bool create(const string & filename) noexcept;
					/**
					 * \~russian
					 * @brief Метод закрытия файла контейнера
					 *
					 *
					 * \~english
					 * @brief Method of the closing of the file of a container
					 *
					 * \~
					 */
					void close() noexcept;
					/**
					 * \~russian
					 * @brief Метод сброса записанного на носитель
					 *
					 * @return признак успешности сброса
					 *
					 * \~english
					 * @brief Method of the flushing of the written to the medium
					 * @return sign of the success of the flushing
					 *
					 * \~
					 */
					[[nodiscard]] bool flush() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод установки способа сброса записанного на носитель
					 *
					 * @details Полный сброс доводит записанное до пластины и переживает обрыв
					 * ПИТАНИЯ; обычный выносит его лишь из ядра накопителю и переживает обрыв
					 * ПРОГРАММЫ. Разница видна лишь у систем Apple, у прочих способы равны
					 *
					 * @warning Снятие полного сброса ОСЛАБЛЯЕТ обещание правки: контейнер,
					 * фиксация какого застигнута обрывом питания, откатится к прежнему поколению
					 * не всегда - хвостовой заголовок может не успеть лечь на пластину. Плата за
					 * обещание замерена: 4.6 мс на фиксацию против 43 мкс
					 *
					 * @param value устанавливаемый способ сброса
					 *
					 * \~english
					 * @brief Method of the setting of the way of the flushing of the written to the medium
					 * @param value way of the flushing being set
					 *
					 * \~
					 */
					void sync(const sync_t value) noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения способа сброса записанного на носитель
					 *
					 * @return способ сброса записанного на носитель
					 *
					 * \~english
					 * @brief Method of the extraction of the way of the flushing of the written to the medium
					 * @return way of the flushing of the written to the medium
					 *
					 * \~
					 */
					[[nodiscard]] sync_t sync() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод выдачи работы чтения октетов контейнера
					 *
					 * @return работа чтения октетов контейнера
					 *
					 * \~english
					 * @brief Method of the issuing of the work of the reading of the octets of a container
					 * @return work of the reading of the octets of the container
					 *
					 * \~
					 */
					editor_t::source_t source() noexcept;
					/**
					 * \~russian
					 * @brief Метод выдачи работы записи октетов контейнера
					 *
					 * @return работа записи октетов контейнера
					 *
					 * \~english
					 * @brief Method of the issuing of the work of the writing of the octets of a container
					 * @return work of the writing of the octets of the container
					 *
					 * \~
					 */
					editor_t::sink_t sink() noexcept;
					/**
					 * \~russian
					 * @brief Метод открытия контейнера файла правкой
					 *
					 * @details Работы чтения и записи ссылаются на хранилище: жить оно обязано
					 * дольше правки, ибо правка зовёт их и при фиксации, и при уборке мусора
					 *
					 * @param editor правка, какой открывается контейнер
					 * @return       признак успешно открытого контейнера
					 *
					 * \~english
					 * @brief Method of the opening of the container of the file by an editing
					 * @details The works of the reading and of the writing refer to the storage: it is obliged to live
					 * longer than the editing, for the editing calls them both at a commit and at a compaction
					 * @param editor editing the container is opened by
					 * @return sign of a successfully opened container
					 *
					 * \~
					 */
					[[nodiscard]] bool bind(editor_t & editor) noexcept;
					/**
					 * \~russian
					 * @brief Метод открытия контейнера файла выборкой записей
					 *
					 * @details Близнец связывания с правкой, и заведён он по той же надобности:
					 * выборка сама открывается ОДНИМ источником, и полной длины контейнера ей знать
					 * неоткуда, - а по ней поверяется, умещается ли кадр оглавления в контейнер.
					 * Хранилище длину ведёт полем и подаёт её выборке само, оттого сторож работает
					 * без участия зовущего
					 *
					 * @note Открывать выборку можно и напрямую, `open(storage.source())`, - но тогда
					 * длина остаётся неведомой, сторож снимается, и последней преградой становится
					 * сам источник
					 *
					 * @param fetcher выборка, какой открывается контейнер
					 * @return        признак успешно открытого контейнера
					 *
					 * \~english
					 * @brief Method of the opening of the container of the file by a fetcher of the records
					 * @param fetcher fetcher the container is opened by
					 * @return sign of a successfully opened container
					 *
					 * \~
					 */
					[[nodiscard]] bool bind(fetcher_t & fetcher) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод записи собранного контейнера в файл
					 *
					 * @details Сборка контейнера выдаёт октеты его целиком, и работа эта кладёт
					 * их в заведённый файл
					 *
					 * @param filename название заводимого файла контейнера
					 * @param buffer   буфер октетов собранного контейнера
					 * @param size     размер октетов собранного контейнера
					 * @return         признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of an assembled container into a file
					 * @details The assembling of a container issues its octets as a whole, and this work places
					 * them into a created file
					 * @param filename name of the file of the container being created
					 * @param buffer buffer of the octets of the assembled container
					 * @param size size of the octets of the assembled container
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					[[nodiscard]] bool store(const string & filename, const void * buffer, const size_t size) noexcept;
					/**
					 * \~russian
					 * @brief Метод чтения октетов файла целиком
					 *
					 * @details Работа эта есть БЛИЗНЕЦ записи `store`: та кладёт октеты в файл, эта
					 * берёт их оттуда обратно. Держится пара затем, что потребителю, которому нужна
					 * ГОЛАЯ запись в файле, а не вместилище с заголовком да кадрами, обе половины
					 * дороги нужны, и вторая её половина отсутствовала - положить запись было чем,
					 * а взять обратно нечем, и потребителю оставалось звать `fopen` самому
					 *
					 * @note Работа эта вместилища НЕ РАЗБИРАЕТ и заголовка его не ждёт: отдаёт она
					 *       октеты как есть. Разбор вместилища ведут `bind` да `load`, и путать их
					 *       с нею не следует - у них своё устройство и свои отказы
					 *
					 * @warning Предел длины здесь обязателен и по умолчанию НЕ БЕСКОНЕЧЕН: чтение
					 *          целиком берёт столько памяти, сколько весит файл, и поданное имя
					 *          приходит извне. Файл сверх предела отвергается, а не читается
					 *
					 * @note Предел, равный НУЛЮ, означает «без предела» - согласно тому же
					 *       уговору, каким живут `maxString` и `maxBlob` в настройках разбора.
					 *       Уговор этот у кодека единый, и разойдись он здесь - потребитель,
					 *       подавший нуль в значении «ни октета», получил бы обратное
					 *
					 * @param filename название читаемого файла
					 * @param result   вычитанные октеты файла
					 * @param limit    наибольшая допустимая длина файла в октетах, ноль - без предела
					 * @return         признак успешности чтения
					 *
					 * \~english
					 * @brief Method of the reading of the octets of a file as a whole
					 * @details This work is the TWIN of the writing `store`
					 * @param filename name of the file being read
					 * @param result read octets of the file
					 * @param limit largest admissible length of the file in octets
					 * @return sign of the success of the reading
					 *
					 * \~
					 */
					[[nodiscard]] bool fetch(const string & filename, vector <uint8_t> & result,
					 const uint64_t limit = 0x4000000) noexcept;
					/**
					 * \~russian
					 * @brief Метод подачи файла контейнера снимателю
					 *
					 * @details Файл подаётся кусками, а не целиком: снятие ведётся потоком, и
					 * держать контейнер в памяти целиком ему незачем
					 *
					 * @param loader сниматель, какому подаётся файл контейнера
					 * @param block  размер подаваемого куска в октетах
					 * @return       признак успешности подачи
					 *
					 * \~english
					 * @brief Method of the submission of the file of a container to a loader
					 * @details The file is submitted by the chunks rather than as a whole: the taking is conducted by a stream,
					 * and there is no need for it to hold the container in the memory as a whole
					 * @param loader loader the file of the container is submitted to
					 * @param block size of the submitted chunk in octets
					 * @return sign of the success of the submission
					 *
					 * \~
					 */
					[[nodiscard]] bool load(loader_t & loader, const size_t block = 65536) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод проверки открытости файла контейнера
					 *
					 * @return признак открытого файла контейнера
					 *
					 * \~english
					 * @brief Method of the checking of the openness of the file of a container
					 * @return sign of an opened file of the container
					 *
					 * \~
					 */
					[[nodiscard]] bool opened() const noexcept;
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
					 * @brief Метод извлечения названия файла контейнера
					 *
					 * @return название файла контейнера
					 *
					 * \~english
					 * @brief Method of the extraction of the name of the file of a container
					 * @return name of the file of the container
					 *
					 * \~
					 */
					const string & filename() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения кода отказа работы с файлом контейнера
					 *
					 * @return код отказа работы с файлом контейнера
					 *
					 * \~english
					 * @brief Method of the extraction of the error code of the work with the file of a container
					 * @return error code of the work with the file of the container
					 *
					 * \~
					 */
					error_t error() const noexcept;
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
					explicit Storage(const log_t * log) noexcept;
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
					~Storage() noexcept;
				protected:
					// Объект работы с логами
					const log_t * _log;
				private:
					/**
					 * \~russian
					 * @brief Метод объявления отказа работы с носителем
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
				private:
					/**
					 * \~russian
					 * @brief Оператор копирования отсутствует намеренно
					 *
					 * @note Хранилище владеет потоком файла: копия его закрыла бы поток дважды
					 *
					 * \~english
					 * @brief The copy operator is absent deliberately
					 * @note The storage owns the stream of the file: its copy would close the stream twice
					 *
					 * \~
					 */
					Storage(const Storage &) = delete;
					Storage & operator = (const Storage &) = delete;
			} storage_t;
		};
	};
};

#endif // __AWH_CODEC_ABC_STORAGE__
