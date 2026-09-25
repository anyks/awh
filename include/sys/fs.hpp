/**
 * @file: fs.hpp
 * @date: 2024-02-25
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

#ifndef __AWH_FS__
#define __AWH_FS__

/**
 * Наши модули
 */
#include "os.hpp"
#include "fmk.hpp"
#include "log.hpp"

/**
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
	/**
	 * Подключаем стандартные модули
	 */
	#include <objbase.h>
	#include <shlobj.h>
	#include <tchar.h>
	#include <strsafe.h>
#endif

/**
 * Стандартные модули
 */
#include <string>
#include <fstream>
#include <codecvt>
#include <sstream>
#include <cstdlib>
#include <functional>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>

/**
 * Если это clang v10 или выше
 */
#if __AWH_EXPERIMENTAL__
	/**
	 * Подключаем стандартные модули
	 */
	#include <filesystem>
#endif

/**
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
	/**
	 * Подключаем стандартные модули
	 */
	#include <sddl.h>
	#include <conio.h>
	#include <aclapi.h>
	#include <direct.h>
/**
 * Для операционной системы не являющейся MS Windows
 */
#else
	/**
	 * Подключаем стандартные модули
	 */
	#include <sys/mman.h>
#endif

/**
 * Если операционной системой является MacOS X
 */
#if __APPLE__ || __MACH__
	/**
	 * Подключаем стандартные модули
	 */
	#include <Carbon/Carbon.h>
#endif

/**
 * @brief пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief Класс модуля работы с файловой системой
	 *
	 */
	typedef class AWH_SHARED_EXPORT FS {
		public:
			/**
			 * Типы файловой системы
			 */
			enum class type_t : uint8_t {
				NONE = 0x00, // Не установлено
				DIR  = 0x01, // Каталог
				CHR  = 0x02, // Устройство
				BLK  = 0x03, // Блок устройства
				LINK = 0x04, // Символьная ссылка
				FILE = 0x05, // Файл
				FIFO = 0x06, // Очередь ввода-вывода
				SOCK = 0x07  // Сокет
			};
			/**
			 * Типы смещений в файле
			 */
			enum class seek_t : uint8_t {
				BEGIN   = 0x00, // Смещение от начала файла
				CURRENT = 0x01, // Смещение от текущей позиции
				END     = 0x02  // Смещение от конца файла
			};
		private:
			// Объект работы с операционной системой
			os_t _os;
		private:
			// Идентификатор родительского процесса
			pid_t _pid;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		public:
			/**
			 * @brief Метод проверяющий существование дирректории
			 *
			 * @param addr адрес дирректории
			 * @return     результат проверки
			 */
			bool isDir(const string & addr) const noexcept;
			/**
			 * @brief Метод проверяющий существование файла
			 *
			 * @param addr адрес файла
			 * @return     результат проверки
			 */
			bool isFile(const string & addr) const noexcept;
			/**
			 * @brief Метод проверки существования сокета
			 *
			 * @param addr адрес сокета
			 * @return     результат проверки
			 */
			bool isSock(const string & addr) const noexcept;
			/**
			 * @brief Метод проверки существования сокета
			 *
			 * @param addr адрес сокета
			 * @return     результат проверки
			 */
			bool isLink(const string & addr) const noexcept;
		public:
			/**
			 * @brief Метод определяющая тип файловой системы по адресу
			 *
			 * @param addr   адрес дирректории
			 * @param actual флаг формирования актуальных адресов
			 * @return       тип файловой системы
			 */
			type_t type(const string & addr, const bool actual = true) const noexcept;
		public:
			/**
			 * @brief Метод извлечения реального адреса
			 *
			 * @param path   путь который нужно определить
			 * @param actual флаг формирования актуальных адресов
			 * @return       полный путь
			 */
			string realPath(const string & path, const bool actual = true) const noexcept;
		public:
			/**
			 * @brief Метод удаления полного пути
			 *
			 * @param path   полный путь для удаления
			 * @param actual флаг формирования актуальных адресов
			 * @return       количество дочерних элементов
			 */
			int32_t delPath(const string & path, const bool actual = true) const noexcept;
		public:
			/**
			 * @brief Метод создания символьной ссылки
			 *
			 * @param addr1 адрес на который нужно сделать ссылку
			 * @param addr2 адрес где должна быть создана ссылка
			 */
			void symLink(const string & addr1, const string & addr2) const noexcept;
			/**
			 * @brief Метод создания жёстких ссылок
			 *
			 * @param addr1 адрес на который нужно сделать ссылку
			 * @param addr2 адрес где должна быть создана ссылка
			 */
			void hardLink(const string & addr1, const string & addr2) const noexcept;
		public:
			/**
			 * @brief Метод подмены целевого файла временным
			 *
			 * @note Запись через временный файл с последующей подменой оставляет прежнее
			 *       содержимое целым при отказе посреди записи. У POSIX подмена выполняется
			 *       rename(), а у MS Windows тот же вызов существующий файл не заменяет,
			 *       оттого там зовётся MoveFileExW с признаком замены
			 *
			 * @param temporary адрес временного файла записи
			 * @param filename  адрес целевого файла записи
			 * @return          результат подмены
			 */
			bool replaceAddress(const string & temporary, const string & filename) const noexcept;
		public:
			/**
			 * @brief Метод рекурсивного создания пути
			 *
			 * @param path полный путь для создания
			 */
			void makePath(const string & path) const noexcept;
			/**
			 * @brief Метод создания каталога для хранения логов
			 *
			 * @param path  адрес для каталога
			 * @param user  данные пользователя
			 * @param group идентификатор группы
			 * @return      результат создания каталога
			 */
			bool makeDir(const string & path, const string & user, const string & group) const noexcept;
		public:
			/**
			 * @brief Метод извлечения названия и расширения файла
			 *
			 * @param addr   адрес файла для извлечения его параметров
			 * @param actual флаг формирования актуальных адресов
			 * @param before флаг определения первой точки расширения слева
			 */
			std::pair <string, string> components(const string & addr, const bool actual = true, const bool before = false) const noexcept;
		public:
			/**
			 * @brief Метод получения прав доступа к файлу или каталогу
			 *
			 * @param path полный путь к файлу или каталогу
			 * @return     запрашиваемые метаданные
			 */
			mode_t chmod(const string & path) const noexcept;
			/**
			 * @brief Метод изменения прав доступа к файлу или каталогу
			 *
			 * @param path полный путь к файлу или каталогу
			 * @param mode метаданные для установки
			 * @return     результат работы функции
			 */
			bool chmod(const string & path, const mode_t mode) const noexcept;
		public:
			/**
			 * @brief Метод установки владельца на файл или каталог
			 *
			 * @param path  путь к файлу или каталогу для установки владельца
			 * @param user  данные пользователя
			 * @param group идентификатор группы
			 * @return      результат работы функции
			 */
			bool chown(const string & path, const string & user, const string & group = "") const noexcept;
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 && _WIN64
		public:
			/**
			 * @brief Метод установки позиции в файле
			 *
			 * @param file     объект открытого файла
			 * @param distance дистанцию на которую нужно переместить позицию
			 * @param position текущая позиция в файле
			 * @return         перенос позиции в файле
			 */
			int64_t seek(HANDLE file, const int64_t distance, const DWORD position) const noexcept;
	#endif
		public:
			/**
			 * @brief Метод подсчёта размера файла/каталога
			 *
			 * @param path полный путь для подсчёта размера
			 * @param ext  расширение файла если требуется фильтрация
			 * @param rec  флаг рекурсивного перебора каталогов
			 * @return     общий размер файла/каталога
			 */
			uintmax_t size(const string & path, const string & ext = "", const bool rec = true) const noexcept;
			/**
			 * @brief Метод подсчёта количество файлов в каталоге
			 *
			 * @param path путь для подсчёта
			 * @param ext  расширение файла если требуется фильтрация
			 * @param rec  флаг рекурсивного перебора каталогов
			 * @return     количество файлов в каталоге
			 */
			uintmax_t count(const string & path, const string & ext = "", const bool rec = true) const noexcept;
		public:
			/**
			 * @brief Метод усечения файла до заданной длины
			 *
			 * @note Отсутствующий файл заводится пустым, файл короче заданной длины
			 *       наращивается нулями - так велит работа самих систем
			 *
			 * @param filename адрес файла который необходимо усечь
			 * @param length   длина, до какой усекается файл
			 * @return         результат усечения
			 */
			bool truncate(const string & filename, const uint64_t length = 0) const noexcept;
			/**
			 * @brief Метод сброса записанного из ядра на носитель
			 *
			 * @note Под MacOS X fsync не опустошает кэш самого накопителя, оттого при
			 *       durable зовётся F_FULLFSYNC. Отказ EINVAL (канал, устройство) отказом
			 *       сброса не считается
			 *
			 * @param filename адрес файла который необходимо сбросить на носитель
			 * @param durable  флаг доведения записанного до носителя, а не до накопителя
			 * @return         результат сброса
			 */
			bool flush(const string & filename, const bool durable = true) const noexcept;
		public:
			/**
			 * @brief Метод чтения данных из файла
			 *
			 * @param filename адрес файла для чтения
			 * @return         бинарный буфер с прочитанными данными
			 */
			vector <char> read(const string & filename) const noexcept;
			/**
			 * @brief Метод чтения данных из файла со смещением
			 *
			 * @note Смещение за пределами файла даёт пустой результат, отказ чтения тоже.
			 *       При END смещение отсчитывается от конца файла НАЗАД (как в AWH 5)
			 *
			 * @param filename адрес файла для чтения
			 * @param seek     тип смещения в файле (CURRENT для нового описателя равен BEGIN)
			 * @param offset   смещение в файле
			 * @return         бинарный буфер с прочитанными данными
			 */
			vector <char> read(const string & filename, const seek_t seek, const size_t offset) const noexcept;
			/**
			 * @brief Метод чтения файла блоками с обратным вызовом
			 *
			 * @param filename адрес файла для чтения
			 * @param size     размер блока для чтения
			 * @param callback функция обратного вызова (буфер, размер, смещение, остаток), ложь останавливает чтение
			 * @param offset   смещение в файле с которого следует начать чтение
			 */
			void read(const string & filename, const size_t size, function <bool (const void *, const size_t, const size_t, const size_t)> callback, const size_t offset = 0) const noexcept;
		public:
			/**
			 * @brief Метод записи в файл бинарных данных
			 *
			 * @param filename адрес файла в который необходимо выполнить запись
			 * @param buffer   бинарный буфер который необходимо записать в файл
			 * @param size     размер бинарного буфера для записи в файл
			 */
			void write(const string & filename, const char * buffer, const size_t size) const noexcept;
			/**
			 * @brief Метод записи в файл бинарных данных по смещению
			 *
			 * @note В отличие от записи без смещения файл НЕ усекается: данные ложатся
			 *       поверх прежних начиная с заданного места, отсутствующий файл заводится.
			 *       При END смещение отсчитывается от конца файла ВПЕРЁД, как у lseek (как в AWH 5)
			 *
			 * @param filename адрес файла в который необходимо выполнить запись
			 * @param buffer   бинарный буфер который необходимо записать в файл
			 * @param size     размер бинарного буфера для записи в файл
			 * @param seek     тип смещения в файле (CURRENT для нового описателя равен BEGIN)
			 * @param offset   смещение в файле
			 * @return         результат записи (легли ли данные в файл целиком)
			 */
			bool write(const string & filename, const char * buffer, const size_t size, const seek_t seek, const size_t offset = 0) const noexcept;
			/**
			 * @brief Метод добавления в файл бинарных данных
			 *
			 * @param filename адрес файла в который необходимо выполнить запись
			 * @param buffer   бинарный буфер который необходимо записать в файл
			 * @param size     размер бинарного буфера для записи в файл
			 */
			void append(const string & filename, const char * buffer, const size_t size) const noexcept;
		public:
			/**
			 * @brief Метод рекурсивного получения всех строк файла
			 *
			 * @param filename адрес файла для чтения
			 * @param callback функция обратного вызова
			 */
			void readFile(const string & filename, function <void (const string &)> callback) const noexcept;
			/**
			 * @brief Метод рекурсивного получения всех строк файла (стандартным способом)
			 *
			 * @param filename адрес файла для чтения
			 * @param callback функция обратного вызова
			 */
			void readFile2(const string & filename, function <void (const string &)> callback) const noexcept;
			/**
			 * @brief Метод рекурсивного получения всех строк файла (построчным методом)
			 *
			 * @param filename адрес файла для чтения
			 * @param callback функция обратного вызова
			 */
			void readFile3(const string & filename, function <void (const string &)> callback) const noexcept;
			/**
			 * @brief Метод получения содержимого файла блоками
			 *
			 * @param filename адрес файла для чтения
			 * @param size     размер блока для чтения (ноль - размер страницы памяти)
			 * @param callback функция обратного вызова
			 */
			void readFile(const string & filename, const size_t size, function <void (const void *, const size_t)> callback) const noexcept;
		public:
			/**
			 * @brief Метод рекурсивного получения файлов во всех подкаталогах
			 *
			 * @param path     путь до каталога
			 * @param ext      расширение файла по которому идет фильтрация
			 * @param rec      флаг рекурсивного перебора каталогов
			 * @param callback функция обратного вызова
			 * @param actual   флаг формирования актуальных адресов
			 */
			void readDir(const string & path, const string & ext, const bool rec, function <void (const string &)> callback, const bool actual = true) const noexcept;
			/**
			 * @brief Метод обхода файлов во всех подкаталогах с досрочной остановкой
			 *
			 * @note Отличается от readDir лишь тем, что функция обратного вызова вправе
			 *       остановить обход, вернув ложь
			 *
			 * @param path     путь до каталога
			 * @param ext      расширение файла по которому идет фильтрация
			 * @param rec      флаг рекурсивного перебора каталогов
			 * @param callback функция обратного вызова, ложь останавливает обход
			 * @param actual   флаг формирования актуальных адресов
			 * @return         результат обхода (довершён ли обход до конца)
			 */
			bool walkDir(const string & path, const string & ext, const bool rec, function <bool (const string &)> callback, const bool actual = true) const noexcept;
			/**
			 * @brief Метод рекурсивного чтения файлов во всех подкаталогах
			 *
			 * @param path     путь до каталога
			 * @param ext      расширение файла по которому идет фильтрация
			 * @param rec      флаг рекурсивного перебора каталогов
			 * @param callback функция обратного вызова
			 * @param actual   флаг формирования актуальных адресов
			 */
			void readPath(const string & path, const string & ext, const bool rec, function <void (const string &, const string &)> callback, const bool actual = true) const noexcept;
		public:
			/**
			 * @brief конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			FS(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief деструктор
			 *
			 */
			~FS() noexcept {}
	} fs_t;
};

#endif // __AWH_FS__
