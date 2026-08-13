/**
 * @file: fs.hpp
 * @date: 2026-01-22
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
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
 * @copyright: Copyright © 2026
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
			 * @brief Метод извлечения названия и расширения файла
			 *
			 * @param addr    путь к файлу для извлечения его параметров
			 * @param resolve флаг резолвинга символьных ссылок
			 * @param before  флаг определения первой точки расширения слева
			 *
			 * \~english
			 * @brief Method of getting the name and the extension of a file
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
			 *
			 * \~english
			 * @brief Method of appending binary data to a file
			 * @param filename path to the file the writing should be performed into
			 * @param buffer   binary buffer that needs to be written into the file
			 *
			 * \~
			 */
			void append(string_view filename, const T & buffer) const noexcept;
			/**
			 * \~russian
			 * @brief Метод добавления в файл бинарных данных
			 *
			 * @param filename путь к файлу в который необходимо выполнить запись
			 * @param buffer   бинарный буфер который необходимо записать в файл
			 *
			 * \~english
			 * @brief Method of appending binary data to a file
			 * @param filename path to the file the writing should be performed into
			 * @param buffer   binary buffer that needs to be written into the file
			 *
			 * \~
			 */
			void append(string_view filename, const char * buffer) const noexcept;
			/**
			 * \~russian
			 * @brief Метод добавления в файл бинарных данных
			 *
			 * @param filename путь к файлу в который необходимо выполнить запись
			 * @param buffer   бинарный буфер который необходимо записать в файл
			 *
			 * \~english
			 * @brief Method of appending binary data to a file
			 * @param filename path to the file the writing should be performed into
			 * @param buffer   binary buffer that needs to be written into the file
			 *
			 * \~
			 */
			void append(string_view filename, const wchar_t * buffer) const noexcept;
			/**
			 * \~russian
			 * @brief Метод добавления в файл бинарных данных
			 *
			 * @param filename путь к файлу в который необходимо выполнить запись
			 * @param buffer   бинарный буфер который необходимо записать в файл
			 * @param size     размер бинарного буфера для записи в файл
			 *
			 * \~english
			 * @brief Method of appending binary data to a file
			 * @param filename path to the file the writing should be performed into
			 * @param buffer   binary buffer that needs to be written into the file
			 * @param size     size of the binary buffer to write into the file
			 *
			 * \~
			 */
			void append(string_view filename, const void * buffer, const size_t size) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Шаблон метода чтения данных из файла
			 *
			 * @tparam T тип возвращаемого результата
			 *
			 * \~english
			 * @brief Template of the method of reading data from a file
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
			 * @return         бинарный буфер с прочитанными данными
			 *
			 * \~english
			 * @brief Method of reading data from a file
			 * @param filename path to the file to read
			 * @param seek     type of the offset in the file
			 * @param offset   offset in the file
			 * @return         binary buffer with the read data
			 *
			 * \~
			 */
			auto read(string_view filename, const seek_t seek = seek_t::BEGIN, const size_t offset = 0) const noexcept -> T;
			/**
			 * \~russian
			 * @brief Шаблон метода чтения данных из файла
			 *
			 * @tparam T тип возвращаемого результата
			 *
			 * \~english
			 * @brief Template of the method of reading data from a file
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
			 *
			 * \~english
			 * @brief Method of reading data from a file
			 * @param filename path to the file to read
			 * @param result   container the result should be put into
			 * @param seek     type of the offset in the file
			 * @param offset   offset in the file
			 *
			 * \~
			 */
			void read(string_view filename, T & result, const seek_t seek = seek_t::BEGIN, const size_t offset = 0) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод рекурсивного чтения больших файлов блоками с обратным вызовом
			 *
			 * @param filename путь к файлу для чтения
			 * @param size     размер блока для чтения
			 * @param callback функция обратного вызова для обработки прочитанных данных (возвращает true для продолжения чтения и false для остановки)
			 * @param offset   смещение в файле с которого следует начать чтение
			 *
			 * \~english
			 * @brief Method of the recursive reading of large files in blocks with a callback
			 * @param filename path to the file to read
			 * @param size     size of the block to read
			 * @param callback callback function for handling the read data (returns true to continue the reading and false to stop)
			 * @param offset   offset in the file the reading should start from
			 *
			 * \~
			 */
			void read(string_view filename, const size_t size, const function <bool (const void * buffer, const size_t size, const size_t offset, const size_t left)> & callback, const size_t offset = 0) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Шаблон метода записи в файл бинарных данных
			 *
			 * @tparam T тип буфера данных
			 *
			 * \~english
			 * @brief Template of the method of writing binary data into a file
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
			 *
			 * \~english
			 * @brief Method of writing binary data into a file
			 * @param filename path to the file the writing should be performed into
			 * @param buffer   binary buffer that needs to be written into the file
			 * @param seek     type of the offset in the file
			 * @param offset   offset in the file
			 *
			 * \~
			 */
			void write(string_view filename, const T & buffer, const seek_t seek = seek_t::BEGIN, const size_t offset = 0) const noexcept;
			/**
			 * \~russian
			 * @brief Метод записи в файл бинарных данных
			 *
			 * @param filename путь к файлу в который необходимо выполнить запись
			 * @param buffer   бинарный буфер который необходимо записать в файл
			 * @param seek     тип смещения в файле
			 * @param offset   смещение в файле
			 *
			 * \~english
			 * @brief Method of writing binary data into a file
			 * @param filename path to the file the writing should be performed into
			 * @param buffer   binary buffer that needs to be written into the file
			 * @param seek     type of the offset in the file
			 * @param offset   offset in the file
			 *
			 * \~
			 */
			void write(string_view filename, const char * buffer, const seek_t seek = seek_t::BEGIN, const size_t offset = 0) const noexcept;
			/**
			 * \~russian
			 * @brief Метод записи в файл бинарных данных
			 *
			 * @param filename путь к файлу в который необходимо выполнить запись
			 * @param buffer   бинарный буфер который необходимо записать в файл
			 * @param seek     тип смещения в файле
			 * @param offset   смещение в файле
			 *
			 * \~english
			 * @brief Method of writing binary data into a file
			 * @param filename path to the file the writing should be performed into
			 * @param buffer   binary buffer that needs to be written into the file
			 * @param seek     type of the offset in the file
			 * @param offset   offset in the file
			 *
			 * \~
			 */
			void write(string_view filename, const wchar_t * buffer, const seek_t seek = seek_t::BEGIN, const size_t offset = 0) const noexcept;
			/**
			 * \~russian
			 * @brief Метод записи в файл бинарных данных
			 *
			 * @param filename путь к файлу в который необходимо выполнить запись
			 * @param buffer   бинарный буфер который необходимо записать в файл
			 * @param size     размер бинарного буфера для записи в файл
			 * @param seek     тип смещения в файле
			 * @param offset   смещение в файле
			 *
			 * \~english
			 * @brief Method of writing binary data into a file
			 * @param filename path to the file the writing should be performed into
			 * @param buffer   binary buffer that needs to be written into the file
			 * @param size     size of the binary buffer to write into the file
			 * @param seek     type of the offset in the file
			 * @param offset   offset in the file
			 *
			 * \~
			 */
			void write(string_view filename, const void * buffer, const size_t size, const seek_t seek = seek_t::BEGIN, const size_t offset = 0) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод рекурсивного получения всех строк файла
			 *
			 * @param filename путь к файлу для чтения
			 * @param callback функция обратного вызова
			 * @param seek     тип смещения в файле
			 * @param offset   смещение в файле
			 *
			 * \~english
			 * @brief Method of the recursive getting of all the lines of a file
			 * @param filename path to the file to read
			 * @param callback callback function
			 * @param seek     type of the offset in the file
			 * @param offset   offset in the file
			 *
			 * \~
			 */
			void readfile(string_view filename, const function <void (string_view)> & callback, const seek_t seek = seek_t::BEGIN, const size_t offset = 0) const noexcept;
			/**
			 * \~russian
			 * @brief Метод рекурсивного получения буфера данных из больших файлов
			 *
			 * @param filename путь к файлу для чтения
			 * @param size     размер буфера для чтения файла
			 * @param callback функция обратного вызова
			 * @param seek     тип смещения в файле
			 * @param offset   смещение в файле
			 *
			 * \~english
			 * @brief Method of the recursive getting of a data buffer from large files
			 * @param filename path to the file to read
			 * @param size     size of the buffer to read the file with
			 * @param callback callback function
			 * @param seek     type of the offset in the file
			 * @param offset   offset in the file
			 *
			 * \~
			 */
			void readfile(string_view filename, const size_t size, const function <void (const void *, const size_t)> & callback, const seek_t seek = seek_t::BEGIN, const size_t offset = 0) const noexcept;
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
			 *
			 * \~english
			 * @brief Method of the recursive getting of the files in all the subdirectories
			 * @param path     path to the directory
			 * @param ext      extension of the file the filtering is driven by
			 * @param recurse  flag of the recursive walk of the directories
			 * @param callback callback function
			 * @param resolve  flag of resolving the symbolic links
			 *
			 * \~
			 */
			void readdir(string_view path, string_view ext, const bool recurse, const function <void (const type_t, string_view)> & callback, const bool resolve = true) const noexcept;
			/**
			 * \~russian
			 * @brief Метод рекурсивного чтения файлов во всех подкаталогах построчно
			 *
			 * @param path     путь до каталога
			 * @param ext      расширение файла по которому идет фильтрация
			 * @param recurse  флаг рекурсивного перебора каталогов
			 * @param callback функция обратного вызова
			 * @param resolve  флаг резолвинга символьных ссылок
			 *
			 * \~english
			 * @brief Method of the recursive reading of the files in all the subdirectories line by line
			 * @param path     path to the directory
			 * @param ext      extension of the file the filtering is driven by
			 * @param recurse  flag of the recursive walk of the directories
			 * @param callback callback function
			 * @param resolve  flag of resolving the symbolic links
			 *
			 * \~
			 */
			void readdir(string_view path, string_view ext, const bool recurse, const function <void (const type_t, string_view, string_view)> & callback, const bool resolve = true) const noexcept;
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
			 *
			 * \~english
			 * @brief Method of the recursive reading of the files in all the subdirectories in binary blocks
			 * @param path     path to the directory
			 * @param ext      extension of the file the filtering is driven by
			 * @param size     size of the buffer to read the file with
			 * @param recurse  flag of the recursive walk of the directories
			 * @param callback callback function
			 * @param resolve  flag of resolving the symbolic links
			 *
			 * \~
			 */
			void readdir(string_view path, string_view ext, const size_t size, const bool recurse, const function <void (const type_t, string_view, const void *, const size_t)> & callback, const bool resolve = true) const noexcept;
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
