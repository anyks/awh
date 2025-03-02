/**
 * @file: fs.hpp
 * @date: 2024-02-25
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
 * Для операционной системы MS Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	#include <windows.h>
	#include <objbase.h>
	#include <shlobj.h>
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
#include <dirent.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>

/**
 * Если это clang v10 или выше
 */
#if defined(__AWH_EXPERIMENTAL__)
	#include <filesystem>
#endif

/**
 * Для операционной системы MS Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	#include <conio.h>
	#include <direct.h>
/**
 * Выполняем работу для Unix
 */
#else
	#include <sys/mman.h>
#endif

/**
 * Если операционной системой является MacOS X
 */
#if __APPLE__ || __MACH__
	#include <Carbon/Carbon.h>
#endif

/**
 * Наши модули
 */
#include <sys/os.hpp>
#include <sys/fmk.hpp>
#include <sys/log.hpp>

/**
 * Для операционной системы MS Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	#include <tchar.h>
	#include <strsafe.h>
#endif

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * FS Класс модуля работы с файловой системой
	 */
	typedef class AWHSHARED_EXPORT FS {
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
			 * message Метод получения текста описания ошибки
			 * @param code код ошибки для получения сообщения
			 * @return     текст сообщения описания кода ошибки
			 */
			string message(const int32_t code = 0) const noexcept;
		public:
			/**
			 * isDir Метод проверяющий существование дирректории
			 * @param addr адрес дирректории
			 * @return     результат проверки
			 */
			bool isDir(const string & addr) const noexcept;
			/**
			 * isFile Метод проверяющий существование файла
			 * @param addr адрес файла
			 * @return     результат проверки
			 */
			bool isFile(const string & addr) const noexcept;
			/**
			 * isSock Метод проверки существования сокета
			 * @param addr адрес сокета
			 * @return     результат проверки
			 */
			bool isSock(const string & addr) const noexcept;
			/**
			 * isLink Метод проверки существования сокета
			 * @param addr адрес сокета
			 * @return     результат проверки
			 */
			bool isLink(const string & addr) const noexcept;
		public:
			/**
			 * type Метод определяющая тип файловой системы по адресу
			 * @param addr   адрес дирректории
			 * @param actual флаг проверки актуальных файлов
			 * @return       тип файловой системы
			 */
			type_t type(const string & addr, const bool actual = true) const noexcept;
		public:
			/**
			 * delPath Метод удаления полного пути
			 * @param path полный путь для удаления
			 * @return     количество дочерних элементов
			 */
			int32_t delPath(const string & path) const noexcept;
		public:
			/**
			 * realPath Метод извлечения реального адреса
			 * @param path   путь который нужно определить
			 * @param actual флаг проверки актуальных файлов
			 * @return       полный путь
			 */
			string realPath(const string & path, const bool actual = true) const noexcept;
		public:
			/**
			 * symLink Метод создания символьной ссылки
			 * @param addr1 адрес на который нужно сделать ссылку
			 * @param addr2 адрес где должна быть создана ссылка
			 */
			void symLink(const string & addr1, const string & addr2) const noexcept;
			/**
			 * hardLink Метод создания жёстких ссылок
			 * @param addr1 адрес на который нужно сделать ссылку
			 * @param addr2 адрес где должна быть создана ссылка
			 */
			void hardLink(const string & addr1, const string & addr2) const noexcept;
		public:
			/**
			 * makePath Метод рекурсивного создания пути
			 * @param path полный путь для создания
			 */
			void makePath(const string & path) const noexcept;
			/**
			 * makeDir Метод создания каталога для хранения логов
			 * @param path  адрес для каталога
			 * @param user  данные пользователя
			 * @param group идентификатор группы
			 * @return      результат создания каталога
			 */
			bool makeDir(const string & path, const string & user, const string & group) const noexcept;
		public:
			/**
			 * components Метод извлечения названия и расширения файла
			 * @param addr   адрес файла для извлечения его параметров
			 * @param actual флаг проверки актуальных файлов
			 * @param before флаг определения первой точки расширения слева
			 */
			pair <string, string> components(const string & addr, const bool actual = true, const bool before = false) const noexcept;
		public:
			/**
			 * chmod Метод получения прав доступа к файлу или каталогу
			 * @param path полный путь к файлу или каталогу
			 * @return     запрашиваемые метаданные
			 */
			mode_t chmod(const string & path) const noexcept;
			/**
			 * chmod Метод изменения прав доступа к файлу или каталогу
			 * @param path полный путь к файлу или каталогу
			 * @param mode метаданные для установки
			 * @return     результат работы функции
			 */
			bool chmod(const string & path, const mode_t mode) const noexcept;
		public:
			/**
			 * Выполняем работу для Unix
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
				/**
				 * chown Метод установки владельца на файл или каталог
				 * @param path  путь к файлу или каталогу для установки владельца
				 * @param user  данные пользователя
				 * @param group идентификатор группы
				 * @return      результат работы функции
				 */
				bool chown(const string & path, const string & user, const string & group) const noexcept;
			/**
			 * Выполняем работу для Windows
			 */
			#else
				/**
				 * seek Метод установки позиции в файле
				 * @param file     объект открытого файла
				 * @param distance дистанцию на которую нужно переместить позицию
				 * @param position текущая позиция в файле
				 * @return         перенос позиции в файле
				 */
				int64_t seek(HANDLE file, const int64_t distance, const DWORD position) const noexcept;
			#endif
		public:
			/**
			 * size Метод подсчёта размера файла/каталога
			 * @param path полный путь для подсчёта размера
			 * @param ext  расширение файла если требуется фильтрация
			 * @param rec  флаг рекурсивного перебора каталогов
			 * @return     общий размер файла/каталога
			 */
			uintmax_t size(const string & path, const string & ext = "", const bool rec = true) const noexcept;
			/**
			 * count Метод подсчёта количество файлов в каталоге
			 * @param path путь для подсчёта
			 * @param ext  расширение файла если требуется фильтрация
			 * @param rec  флаг рекурсивного перебора каталогов
			 * @return     количество файлов в каталоге
			 */
			uintmax_t count(const string & path, const string & ext = "", const bool rec = true) const noexcept;
		public:
			/**
			 * read Метод чтения данных из файла
			 * @param filename адрес файла для чтения
			 * @return         бинарный буфер с прочитанными данными
			 */
			vector <char> read(const string & filename) const noexcept;
		public:
			/**
			 * write Метод записи в файл бинарных данных
			 * @param filename адрес файла в который необходимо выполнить запись
			 * @param buffer   бинарный буфер который необходимо записать в файл
			 * @param size     размер бинарного буфера для записи в файл
			 */
			void write(const string & filename, const char * buffer, const size_t size) const noexcept;
			/**
			 * append Метод добавления в файл бинарных данных
			 * @param filename адрес файла в который необходимо выполнить запись
			 * @param buffer   бинарный буфер который необходимо записать в файл
			 * @param size     размер бинарного буфера для записи в файл
			 */
			void append(const string & filename, const char * buffer, const size_t size) const noexcept;
		public:
			/**
			 * readFile Метод рекурсивного получения всех строк файла
			 * @param filename адрес файла для чтения
			 * @param callback функция обратного вызова
			 */
			void readFile(const string & filename, function <void (const string &)> callback) const noexcept;
			/**
			 * readFile2 Метод рекурсивного получения всех строк файла (стандартным способом)
			 * @param filename адрес файла для чтения
			 * @param callback функция обратного вызова
			 */
			void readFile2(const string & filename, function <void (const string &)> callback) const noexcept;
			/**
			 * readFile3 Метод рекурсивного получения всех строк файла (построчным методом)
			 * @param filename адрес файла для чтения
			 * @param callback функция обратного вызова
			 */
			void readFile3(const string & filename, function <void (const string &)> callback) const noexcept;
		public:
			/**
			 * readDir Метод рекурсивного получения файлов во всех подкаталогах
			 * @param path     путь до каталога
			 * @param ext      расширение файла по которому идет фильтрация
			 * @param rec      флаг рекурсивного перебора каталогов
			 * @param callback функция обратного вызова
			 * @param actual   флаг проверки актуальных файлов
			 */
			void readDir(const string & path, const string & ext, const bool rec, function <void (const string &)> callback, const bool actual = true) const noexcept;
			/**
			 * readPath Метод рекурсивного чтения файлов во всех подкаталогах
			 * @param path     путь до каталога
			 * @param ext      расширение файла по которому идет фильтрация
			 * @param rec      флаг рекурсивного перебора каталогов
			 * @param callback функция обратного вызова
			 * @param actual   флаг проверки актуальных файлов
			 */
			void readPath(const string & path, const string & ext, const bool rec, function <void (const string &, const string &)> callback, const bool actual = true) const noexcept;
		public:
			/**
			 * FS конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			FS(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log) {}
			/**
			 * ~FS деструктор
			 */
			~FS() noexcept {}
	} fs_t;
};

#endif // __AWH_FS__
