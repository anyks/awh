/**
 * @file fs.hpp
 * @date 2026-01-22
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
 * @brief Заголовочный файл модуля работы с файловой системой — класс Filesystem для чтения и записи файлов,
 *        обхода каталогов, получения атрибутов, создания и удаления объектов ФС с нативной поддержкой macOS, Windows,
 *        Linux, FreeBSD, NetBSD, OpenBSD, Solaris и OpenIndiana
 *
 * \~english
 * @brief Header file of the filesystem module — the Filesystem class for reading and writing files,
 *        walking directories, getting attributes, creating and removing filesystem objects with native support of macOS, Windows,
 *        Linux, FreeBSD, NetBSD, OpenBSD, Solaris and OpenIndiana
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_FS__
#define __AWH_FS__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <functional>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "os.hpp"
#include "fmk.hpp"
#include "log.hpp"

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
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Класс для автоматического управления каталогом (RAII)
	 *
	 * \~english
	 * @brief Class for Automatic Directory Inspection (RAII)
	 *
	 * \~
	 */
	class HandleDir;
	/**
	 * \~russian
	 * @brief Класс для автоматического управления файлом (RAII)
	 *
	 * \~english
	 * @brief Class for Automatic File Management (RAII)
	 *
	 * \~
	 */
	class HandleFile;

	/**
	 * \~russian
	 * @brief Создаём тип данных объекта каталога
	 *
	 * \~english
	 * @brief Create a directory object data type
	 *
	 * \~
	 */
	using handle_dir_t = HandleDir;
	/**
	 * \~russian
	 * @brief Создаём тип данных объекта файла
	 *
	 * \~english
	 * @brief Create a file object data type
	 *
	 * \~
	 */
	using handle_file_t = HandleFile;

	/**
	 * \~russian
	 * @brief Класс модуля работы с файловой системой
	 *
	 * \~english
	 * @brief Class of the filesystem module
	 *
	 * \~
	 */
	typedef class __AWH_SHARED_EXPORT__ Filesystem {
		public:
			/**
			 * \~russian
			 * @brief Типы смещений в файле
			 *
			 * \~english
			 * @brief Types of the offsets in a file
			 *
			 * \~
			 */
			enum class seek_t : uint8_t {
				BEGIN   = 0x00, // Смещение от начала файла
				CURRENT = 0x01, // Смещение от текущей позиции
				END     = 0x02  // Смещение от конца файла
			};
			/**
			 * \~russian
			 * @brief Тип файловой системы
			 *
			 * \~english
			 * @brief Type of the filesystem object
			 *
			 * \~
			 */
			enum class type_t : uint8_t {
				NONE = 0x00, // Не установлено
				DIR  = 0x01, // Каталог
				CHR  = 0x02, // Устройство
				BLK  = 0x03, // Блок устройства
				FILE = 0x04, // Физический файл
				FIFO = 0x05, // Очередь ввода-вывода
				SOCK = 0x06, // Unix-сокет
				LINK = 0x07  // Символьная ссылка
			};
		public:
			/**
			 * \~russian
			 * @brief Тип прав доступа к файлу или каталогу
			 *
			 * \~english
			 * @brief Type of the access rights to a file or a directory
			 *
			 * \~
			 */
			using components_t = std::pair <string, string>;
		private:
			// Объект работы с операционной системой
			os_t _os;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		public:
			/**
			 * \~russian
			 * @brief Метод создания символьной ссылки
			 *
			 * @param first  адрес на который нужно сделать ссылку
			 * @param second адрес где должна быть создана ссылка
			 *
			 * \~english
			 * @brief Method of creating a symbolic link
			 *
			 * @param first  address the link should be made to
			 * @param second address where the link should be created
			 *
			 * \~
			 */
			void symlink(string_view first, string_view second) const noexcept;
			/**
			 * \~russian
			 * @brief Метод создания жёстких ссылок
			 *
			 * @param first  адрес на который нужно сделать ссылку
			 * @param second адрес где должна быть создана ссылка
			 *
			 * \~english
			 * @brief Method of creating hard links
			 *
			 * @param first  address the link should be made to
			 * @param second address where the link should be created
			 *
			 * \~
			 */
			void hardlink(string_view first, string_view second) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод удаления адреса файловой системы
			 *
			 * @param addr    полный адрес для удаления
			 * @param resolve флаг резолвинга символьных ссылок
			 * @return        результат удаления
			 *
			 * \~english
			 * @brief Method of removing a filesystem address
			 *
			 * @param addr    full address to remove
			 * @param resolve flag of resolving the symbolic links
			 * @return        result of the removal
			 *
			 * \~
			 */
			bool unlink(string_view addr, const bool resolve = false) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод, определяющий тип файловой системы по адресу
			 *
			 * @param addr        адрес директории или файла
			 * @param detectLinks флаг детектирования символьных ссылок (на горячих путях можно отключить)
			 * @return            тип файловой системы
			 *
			 * \~english
			 * @brief Method determining the type of the filesystem object by the address
			 *
			 * @param addr        address of the directory or of the file
			 * @param detectLinks flag of detecting the symbolic links (on hot paths it may be switched off)
			 * @return            type of the filesystem object
			 *
			 * \~
			 */
			type_t type(string_view addr, const bool detectLinks = true) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод извлечения реального адреса
			 *
			 * @param addr    адрес который нужно определить
			 * @param resolve флаг резолвинга символьных ссылок
			 * @return        полный путь
			 *
			 * \~english
			 * @brief Method of getting the real address
			 *
			 * @param addr    address that needs to be determined
			 * @param resolve flag of resolving the symbolic links
			 * @return        full path
			 *
			 * \~
			 */
			string fullpath(string_view addr, const bool resolve = false) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения прав доступа к файлу или каталогу
			 *
			 * @param addr путь к файлу или каталогу
			 * @return     запрашиваемые метаданные
			 *
			 * \~english
			 * @brief Method of getting the access rights to a file or a directory
			 *
			 * @param addr path to the file or to the directory
			 * @return     the requested metadata
			 *
			 * \~
			 */
			uint32_t chmod(string_view addr) const noexcept;
			/**
			 * \~russian
			 * @brief Метод изменения прав доступа к файлу или каталогу
			 *
			 * @param addr путь к файлу или каталогу
			 * @param mode метаданные для установки
			 * @return     результат работы функции
			 *
			 * \~english
			 * @brief Method of changing the access rights to a file or a directory
			 *
			 * @param addr path to the file or to the directory
			 * @param mode metadata to set
			 * @return     result of the work of the function
			 *
			 * \~
			 */
			bool chmod(string_view addr, const uint32_t mode) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки владельца на файл или каталог
			 *
			 * @param addr  путь к файлу или каталогу для установки владельца
			 * @param user  имя пользователя
			 * @param group название группы пользователя
			 * @return      результат работы функции
			 *
			 * \~english
			 * @brief Method of setting the owner of a file or a directory
			 *
			 * @param addr  path to the file or to the directory to set the owner of
			 * @param user  name of the user
			 * @param group name of the group of the user
			 * @return      result of the work of the function
			 *
			 * \~
			 */
			bool chown(string_view addr, string_view user, string_view group = "") const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод рекурсивного создания пути
			 *
			 * @param addr адрес для создания каталога
			 * @return     результат создания каталога
			 *
			 * \~english
			 * @brief Method of the recursive creation of a path
			 *
			 * @param addr address to create the directory at
			 * @return     result of the creation of the directory
			 *
			 * \~
			 */
			bool mkdir(string_view addr) const noexcept;
			/**
			 * \~russian
			 * @brief Метод создания каталога с указанием владельца
			 *
			 * @param addr  адрес для создания каталога
			 * @param user  имя пользователя
			 * @param group название группы пользователя
			 * @return      результат создания каталога
			 *
			 * \~english
			 * @brief Method of creating a directory with the owner specified
			 *
			 * @param addr  address to create the directory at
			 * @param user  name of the user
			 * @param group name of the group of the user
			 * @return      result of the creation of the directory
			 *
			 * \~
			 */
			bool mkdir(string_view addr, string_view user, string_view group) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод подмены целевого файла временным
			 *
			 * @details Запись в файл ведётся через временный файл с последующей подменою: так
			 *          отказ посреди записи оставляет прежнее содержимое целым, а не наполовину выписанным.
			 *          Подмена эта у POSIX выполняется `rename()`, но у MS Windows тот же
			 *          вызов существующий файл НЕ ЗАМЕНЯЕТ - он отвечает отказом, и запись во второй раз
			 *          по одному и тому же пути не удаётся вовсе.
			 *
			 * @warning Беда эта была настоящей и найдена не рассуждением: набор проверок
			 *          кодеков, впервые собравшись под MinGW 07.09.2026, дал четыре отказа в
			 *          трёх разных кодеках, и у всех четырёх падала ВТОРАЯ работа с файлом по
			 *          тому же пути, а первая проходила. Доказано щупом на стенде MinGW:
			 *          `rename()` на существующий файл отвечает `-1`, на отсутствующий - нулём
			 *
			 * @note Способ замены у MS Windows взят тот же, каким пользуется модуль
			 *       криптографии: `MoveFileEx` с признаками `MOVEFILE_REPLACE_EXISTING` и
			 *       `MOVEFILE_WRITE_THROUGH`. Первый дозволяет замену на месте, второй велит
			 *       дождаться, пока запись ляжет на устройство, - иначе подмена считается
			 *       свершённой прежде времени. Зовётся узкий вид (`MoveFileExA`), а не широкий:
			 *       пути ходят здесь `std::string`, и широкий их не примет
			 *
			 * @note Тело живёт в `src/codec/replace.cpp`, а не здесь, - по общему правилу
			 *       дерева о чистых заголовочных файлах. Порядок этот важен не только видом:
			 *       `windows.h` приносит макросы `ERROR`, `DELETE`, `TEXT`, и, стой включение
			 *       в заголовке, они расходились бы по всякому кодеку, его включившему. В
			 *       исходнике же они заперты одной единицей трансляции. Указал на это владелец
			 *       кодеков INI, YAML и TOML
			 *
			 * @note Снос целевого файла перед `rename()` был бы способом более простым, но
			 *       НЕВЕРНЫМ: между сносом и переименованием целевого файла не существует
			 *       вовсе, и отказ в этот миг оставляет потребителя без прежнего содержимого -
			 *       ровно то, ради чего временный файл и заводится
			 *
			 * @param temporary адрес временного файла записи
			 * @param filename  адрес целевого файла записи
			 * @return          признак успешной подмены
			 *
			 * \~english
			 * @brief Method of the replacement of a target file by a temporary one
			 *
			 * @details The writing into a file is performed through a temporary file with a subsequent
			 *          replacement: thus a refusal in the middle of the writing leaves the previous content whole.
			 *          At POSIX this replacement is performed by `rename()`, but at MS Windows the same call
			 *          does NOT replace an existing file - it answers with a refusal.
			 *
			 * @warning This problem was real and was not found by reasoning: a set of codec checks,
			 *          first assembled under MinGW on 09/07/2026, gave four failures in three different codecs,
			 *          and for all four the SECOND work with a file along the same path failed, while the first one passed.
			 *          Proven by a probe at the MinGW stand: `rename()` responds to an existing file with `-1`, to a missing file - zero
			 *
			 * @note The replacement method for MS Windows is the same as that used by the cryptography module:
			 *       `MoveFileEx` with the attributes `MOVEFILE_REPLACE_EXISTING` and `MOVEFILE_WRITE_THROUGH`.
			 *       The first allows replacement on the spot, the second tells you to wait until the recording
			 *       is transferred to the device, otherwise the replacement is considered completed ahead of time.
			 *       The name is the narrow view (`MoveFileExA`), not the wide one: the paths go here `std::string`,
			 *       and the wide one will not accept them
			 *
			 * @note The body lives in `src/codec/replace.cpp`, and not here - according to the general tree rule
			 *       about pure header files. This order is important not only in appearance:
			 *       `windows.h` brings the macros `ERROR`, `DELETE`, `TEXT`, and if included in the header,
			 *       they would diverge for any codec that included it. In the source code,
			 *       they are locked in one translation unit. The owner of the INI, YAML and TOML codecs pointed this out
			 *
			 * @note Demolishing the target file before `rename()` would be a simpler method, but it is WRONG:
			 *       between demolition and renaming, the target file does not exist at all,
			 *       and failure at that moment leaves the user without the previous contents - exactly what the temporary file is created for
			 *
			 * @param temporary address of the temporary file of the writing
			 * @param filename  address of the target file of the writing
			 * @return          sign of the successful replacement
			 *
			 * \~
			 */
			bool replaceAddress(string_view temporary, string_view filename) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод извлечения названия и расширения файла
			 *
			 * @param addr    путь к файлу для извлечения его параметров
			 * @param resolve флаг резолвинга символьных ссылок
			 * @param before  флаг определения первой точки расширения слева
			 *
			 * \~english
			 * @brief Method of getting the name and the extension of a file
			 *
			 * @param addr    path to the file to get its parameters of
			 * @param resolve flag of resolving the symbolic links
			 * @param before  flag of determining the first dot of the extension from the left
			 *
			 * \~
			 */
			components_t components(string_view addr, const bool resolve = false, const bool before = false) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод подсчёта размера файла/каталога
			 *
			 * @param addr    адрес для подсчёта размера
			 * @param ext     расширение файла если требуется фильтрация
			 * @param recurse флаг рекурсивного перебора каталогов
			 * @return        общий размер файла/каталога
			 *
			 * \~english
			 * @brief Method of counting the size of a file/directory
			 *
			 * @param addr    address to count the size of
			 * @param ext     extension of the file if filtering is required
			 * @param recurse flag of the recursive walk of the directories
			 * @return        total size of the file/directory
			 *
			 * \~
			 */
			uintmax_t size(string_view addr, string_view ext = "", const bool recurse = true) const noexcept;
			/**
			 * \~russian
			 * @brief Метод подсчёта количества файлов в каталоге
			 *
			 * @param addr    адрес для подсчёта количества файлов
			 * @param ext     расширение файла если требуется фильтрация
			 * @param recurse флаг рекурсивного перебора каталогов
			 * @return        количество файлов в каталоге
			 *
			 * \~english
			 * @brief Method of counting the number of files in a directory
			 *
			 * @param addr    address to count the number of files at
			 * @param ext     extension of the file if filtering is required
			 * @param recurse flag of the recursive walk of the directories
			 * @return        number of files in the directory
			 *
			 * \~
			 */
			uintmax_t count(string_view addr, string_view ext = "", const bool recurse = true) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Шаблон метода добавления в файл бинарных данных
			 *
			 * @tparam T тип буфера данных
			 *
			 * \~english
			 * @brief Template of the method of appending binary data to a file
			 *
			 * @tparam T type of the data buffer
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод добавления в файл бинарных данных
			 *
			 * @param filename путь к файлу в который необходимо выполнить запись
			 * @param buffer   бинарный буфер который необходимо записать в файл
			 * @param handle   внешний объект файла, если необходима поддержка пакетной обработки
			 *
			 * \~english
			 * @brief Method of appending binary data to a file
			 *
			 * @param filename path to the file the writing should be performed into
			 * @param buffer   binary buffer that needs to be written into the file
			 * @param handle   external file object if batch processing support is required
			 *
			 * \~
			 */
			void append(string_view filename, const T & buffer, handle_file_t * handle = nullptr) const noexcept;
			/**
			 * \~russian
			 * @brief Метод добавления в файл бинарных данных
			 *
			 * @param filename путь к файлу в который необходимо выполнить запись
			 * @param buffer   бинарный буфер который необходимо записать в файл
			 * @param handle   внешний объект файла, если необходима поддержка пакетной обработки
			 *
			 * \~english
			 * @brief Method of appending binary data to a file
			 *
			 * @param filename path to the file the writing should be performed into
			 * @param buffer   binary buffer that needs to be written into the file
			 * @param handle   external file object if batch processing support is required
			 *
			 * \~
			 */
			void append(string_view filename, const char * buffer, handle_file_t * handle = nullptr) const noexcept;
			/**
			 * \~russian
			 * @brief Метод добавления в файл бинарных данных
			 *
			 * @param filename путь к файлу в который необходимо выполнить запись
			 * @param buffer   бинарный буфер который необходимо записать в файл
			 * @param handle   внешний объект файла, если необходима поддержка пакетной обработки
			 *
			 * \~english
			 * @brief Method of appending binary data to a file
			 *
			 * @param filename path to the file the writing should be performed into
			 * @param buffer   binary buffer that needs to be written into the file
			 * @param handle   external file object if batch processing support is required
			 *
			 * \~
			 */
			void append(string_view filename, const wchar_t * buffer, handle_file_t * handle = nullptr) const noexcept;
			/**
			 * \~russian
			 * @brief Метод добавления в файл бинарных данных
			 *
			 * @param filename путь к файлу в который необходимо выполнить запись
			 * @param buffer   бинарный буфер который необходимо записать в файл
			 * @param size     размер бинарного буфера для записи в файл
			 * @param handle   внешний объект файла, если необходима поддержка пакетной обработки
			 *
			 * \~english
			 * @brief Method of appending binary data to a file
			 *
			 * @param filename path to the file the writing should be performed into
			 * @param buffer   binary buffer that needs to be written into the file
			 * @param size     size of the binary buffer to write into the file
			 * @param handle   external file object if batch processing support is required
			 *
			 * \~
			 */
			void append(string_view filename, const void * buffer, const size_t size, handle_file_t * handle = nullptr) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Шаблон метода чтения данных из файла
			 *
			 * @tparam T тип возвращаемого результата
			 *
			 * \~english
			 * @brief Template of the method of reading data from a file
			 *
			 * @tparam T type of the returned result
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод чтения данных из файла
			 *
			 * @param filename путь к файлу для чтения
			 * @param seek     тип смещения в файле
			 * @param offset   смещение в файле
			 * @param handle   внешний объект файла, если необходима поддержка пакетной обработки
			 * @return         бинарный буфер с прочитанными данными
			 *
			 * \~english
			 * @brief Method of reading data from a file
			 *
			 * @param filename path to the file to read
			 * @param seek     type of the offset in the file
			 * @param offset   offset in the file
			 * @param handle   external file object if batch processing support is required
			 * @return         binary buffer with the read data
			 *
			 * \~
			 */
			auto read(string_view filename, const seek_t seek = seek_t::BEGIN, const size_t offset = 0, handle_file_t * handle = nullptr) const noexcept -> T;
			/**
			 * \~russian
			 * @brief Шаблон метода чтения данных из файла
			 *
			 * @tparam T тип возвращаемого результата
			 *
			 * \~english
			 * @brief Template of the method of reading data from a file
			 *
			 * @tparam T type of the returned result
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод чтения данных из файла
			 *
			 * @param filename путь к файлу для чтения
			 * @param result   контейнер куда следует положить результат
			 * @param seek     тип смещения в файле
			 * @param offset   смещение в файле
			 * @param handle   внешний объект файла, если необходима поддержка пакетной обработки
			 *
			 * \~english
			 * @brief Method of reading data from a file
			 *
			 * @param filename path to the file to read
			 * @param result   container the result should be put into
			 * @param seek     type of the offset in the file
			 * @param offset   offset in the file
			 * @param handle   external file object if batch processing support is required
			 *
			 * \~
			 */
			void read(string_view filename, T & result, const seek_t seek = seek_t::BEGIN, const size_t offset = 0, handle_file_t * handle = nullptr) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод рекурсивного чтения больших файлов блоками с обратным вызовом
			 *
			 * @param filename путь к файлу для чтения
			 * @param size     размер блока для чтения
			 * @param callback функция обратного вызова для обработки прочитанных данных (возвращает true для продолжения чтения и false для остановки)
			 * @param offset   смещение в файле с которого следует начать чтение
			 * @param handle   внешний объект файла, если необходима поддержка пакетной обработки
			 *
			 * \~english
			 * @brief Method of the recursive reading of large files in blocks with a callback
			 *
			 * @param filename path to the file to read
			 * @param size     size of the block to read
			 * @param callback callback function for handling the read data (returns true to continue the reading and false to stop)
			 * @param offset   offset in the file the reading should start from
			 * @param handle   external file object if batch processing support is required
			 *
			 * \~
			 */
			void read(string_view filename, const size_t size, const function <bool (const void * buffer, const size_t size, const size_t offset, const size_t left)> & callback, const size_t offset = 0, handle_file_t * handle = nullptr) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Шаблон метода записи в файл бинарных данных
			 *
			 * @tparam T тип буфера данных
			 *
			 * \~english
			 * @brief Template of the method of writing binary data into a file
			 *
			 * @tparam T type of the data buffer
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод записи в файл бинарных данных
			 *
			 * @param filename путь к файлу в который необходимо выполнить запись
			 * @param buffer   бинарный буфер который необходимо записать в файл
			 * @param seek     тип смещения в файле
			 * @param offset   смещение в файле
			 * @param handle   внешний объект файла, если необходима поддержка пакетной обработки
			 *
			 * \~english
			 * @brief Method of writing binary data into a file
			 *
			 * @param filename path to the file the writing should be performed into
			 * @param buffer   binary buffer that needs to be written into the file
			 * @param seek     type of the offset in the file
			 * @param offset   offset in the file
			 * @param handle   external file object if batch processing support is required
			 *
			 * \~
			 */
			void write(string_view filename, const T & buffer, const seek_t seek = seek_t::BEGIN, const size_t offset = 0, handle_file_t * handle = nullptr) const noexcept;
			/**
			 * \~russian
			 * @brief Метод записи в файл бинарных данных
			 *
			 * @param filename путь к файлу в который необходимо выполнить запись
			 * @param buffer   бинарный буфер который необходимо записать в файл
			 * @param seek     тип смещения в файле
			 * @param offset   смещение в файле
			 * @param handle   внешний объект файла, если необходима поддержка пакетной обработки
			 *
			 * \~english
			 * @brief Method of writing binary data into a file
			 *
			 * @param filename path to the file the writing should be performed into
			 * @param buffer   binary buffer that needs to be written into the file
			 * @param seek     type of the offset in the file
			 * @param offset   offset in the file
			 * @param handle   external file object if batch processing support is required
			 *
			 * \~
			 */
			void write(string_view filename, const char * buffer, const seek_t seek = seek_t::BEGIN, const size_t offset = 0, handle_file_t * handle = nullptr) const noexcept;
			/**
			 * \~russian
			 * @brief Метод записи в файл бинарных данных
			 *
			 * @param filename путь к файлу в который необходимо выполнить запись
			 * @param buffer   бинарный буфер который необходимо записать в файл
			 * @param seek     тип смещения в файле
			 * @param offset   смещение в файле
			 * @param handle   внешний объект файла, если необходима поддержка пакетной обработки
			 *
			 * \~english
			 * @brief Method of writing binary data into a file
			 *
			 * @param filename path to the file the writing should be performed into
			 * @param buffer   binary buffer that needs to be written into the file
			 * @param seek     type of the offset in the file
			 * @param offset   offset in the file
			 * @param handle   external file object if batch processing support is required
			 *
			 * \~
			 */
			void write(string_view filename, const wchar_t * buffer, const seek_t seek = seek_t::BEGIN, const size_t offset = 0, handle_file_t * handle = nullptr) const noexcept;
			/**
			 * \~russian
			 * @brief Метод записи в файл бинарных данных
			 *
			 * @param filename путь к файлу в который необходимо выполнить запись
			 * @param buffer   бинарный буфер который необходимо записать в файл
			 * @param size     размер бинарного буфера для записи в файл
			 * @param seek     тип смещения в файле
			 * @param offset   смещение в файле
			 * @param handle   внешний объект файла, если необходима поддержка пакетной обработки
			 *
			 * \~english
			 * @brief Method of writing binary data into a file
			 *
			 * @param filename path to the file the writing should be performed into
			 * @param buffer   binary buffer that needs to be written into the file
			 * @param size     size of the binary buffer to write into the file
			 * @param seek     type of the offset in the file
			 * @param offset   offset in the file
			 * @param handle   external file object if batch processing support is required
			 *
			 * \~
			 */
			void write(string_view filename, const void * buffer, const size_t size, const seek_t seek = seek_t::BEGIN, const size_t offset = 0, handle_file_t * handle = nullptr) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод рекурсивного получения всех строк файла
			 *
			 * @param filename путь к файлу для чтения
			 * @param callback функция обратного вызова
			 * @param seek     тип смещения в файле
			 * @param offset   смещение в файле
			 * @param handle   внешний объект файла, если необходима поддержка пакетной обработки
			 *
			 * \~english
			 * @brief Method of the recursive getting of all the lines of a file
			 *
			 * @param filename path to the file to read
			 * @param callback callback function
			 * @param seek     type of the offset in the file
			 * @param offset   offset in the file
			 * @param handle   external file object if batch processing support is required
			 *
			 * \~
			 */
			void readfile(string_view filename, const function <void (string_view)> & callback, const seek_t seek = seek_t::BEGIN, const size_t offset = 0, handle_file_t * handle = nullptr) const noexcept;
			/**
			 * \~russian
			 * @brief Метод рекурсивного получения буфера данных из больших файлов
			 *
			 * @param filename путь к файлу для чтения
			 * @param size     размер буфера для чтения файла
			 * @param callback функция обратного вызова
			 * @param seek     тип смещения в файле
			 * @param offset   смещение в файле
			 * @param handle   внешний объект файла, если необходима поддержка пакетной обработки
			 *
			 * \~english
			 * @brief Method of the recursive getting of a data buffer from large files
			 *
			 * @param filename path to the file to read
			 * @param size     size of the buffer to read the file with
			 * @param callback callback function
			 * @param seek     type of the offset in the file
			 * @param offset   offset in the file
			 * @param handle   external file object if batch processing support is required
			 *
			 * \~
			 */
			void readfile(string_view filename, const size_t size, const function <void (const void *, const size_t)> & callback, const seek_t seek = seek_t::BEGIN, const size_t offset = 0, handle_file_t * handle = nullptr) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод рекурсивного получения файлов во всех подкаталогах
			 *
			 * @param path     путь до каталога
			 * @param ext      расширение файла по которому идет фильтрация
			 * @param recurse  флаг рекурсивного перебора каталогов
			 * @param callback функция обратного вызова
			 * @param resolve  флаг резолвинга символьных ссылок
			 * @param handle   внешний объект каталога, если необходима поддержка пакетной обработки
			 *
			 * \~english
			 * @brief Method of the recursive getting of the files in all the subdirectories
			 *
			 * @param path     path to the directory
			 * @param ext      extension of the file the filtering is driven by
			 * @param recurse  flag of the recursive walk of the directories
			 * @param callback callback function
			 * @param resolve  flag of resolving the symbolic links
			 * @param handle   external directory object if batch processing support is required
			 *
			 * \~
			 */
			void readdir(string_view path, string_view ext, const bool recurse, const function <void (const type_t, string_view)> & callback, const bool resolve = true, handle_dir_t * handle = nullptr) const noexcept;
			/**
			 * \~russian
			 * @brief Метод рекурсивного чтения файлов во всех подкаталогах построчно
			 *
			 * @param path     путь до каталога
			 * @param ext      расширение файла по которому идет фильтрация
			 * @param recurse  флаг рекурсивного перебора каталогов
			 * @param callback функция обратного вызова
			 * @param resolve  флаг резолвинга символьных ссылок
			 * @param handle   внешний объект каталога, если необходима поддержка пакетной обработки
			 *
			 * \~english
			 * @brief Method of the recursive reading of the files in all the subdirectories line by line
			 *
			 * @param path     path to the directory
			 * @param ext      extension of the file the filtering is driven by
			 * @param recurse  flag of the recursive walk of the directories
			 * @param callback callback function
			 * @param resolve  flag of resolving the symbolic links
			 * @param handle   external directory object if batch processing support is required
			 *
			 * \~
			 */
			void readdir(string_view path, string_view ext, const bool recurse, const function <void (const type_t, string_view, string_view)> & callback, const bool resolve = true, handle_dir_t * handle = nullptr) const noexcept;
			/**
			 * \~russian
			 * @brief Метод рекурсивного чтения файлов во всех подкаталогах бинарными блоками
			 *
			 * @param path     путь до каталога
			 * @param ext      расширение файла по которому идет фильтрация
			 * @param size     размер буфера для чтения файла
			 * @param recurse  флаг рекурсивного перебора каталогов
			 * @param callback функция обратного вызова
			 * @param resolve  флаг резолвинга символьных ссылок
			 * @param handle   внешний объект каталога, если необходима поддержка пакетной обработки
			 *
			 * \~english
			 * @brief Method of the recursive reading of the files in all the subdirectories in binary blocks
			 *
			 * @param path     path to the directory
			 * @param ext      extension of the file the filtering is driven by
			 * @param size     size of the buffer to read the file with
			 * @param recurse  flag of the recursive walk of the directories
			 * @param callback callback function
			 * @param resolve  flag of resolving the symbolic links
			 * @param handle   external directory object if batch processing support is required
			 *
			 * \~
			 */
			void readdir(string_view path, string_view ext, const size_t size, const bool recurse, const function <void (const type_t, string_view, const void *, const size_t)> & callback, const bool resolve = true, handle_dir_t * handle = nullptr) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 *
			 * \~english
			 * @brief Constructor
			 * @param fmk framework object
			 * @param log object for working with logs
			 *
			 * \~
			 */
			explicit Filesystem(const fmk_t * fmk, const log_t * log) noexcept;
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
			~Filesystem() noexcept {}
	} fs_t;
};

#endif // __AWH_FS__
