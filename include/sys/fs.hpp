/**
 * @file: fs.hpp
 * @date: 2023-02-13
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2023
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
 * Стандартная библиотека
 */
#include <string>
#include <fstream>
#include <codecvt>
#include <sstream>
#include <cstdlib>
#include <functional>
#include <unistd.h>
#include <dirent.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>

/**
 * Если это clang v10 или выше
 */
#if defined(__DANUBE_EXPERIMENTAL__)
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
	#include <grp.h>
	#include <pwd.h>
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
#include <sys/fmk.hpp>
#include <sys/log.hpp>

/**
 * Для операционной системы MS Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	#include <tchar.h>
	#include <strsafe.h>
#endif

// Устанавливаем область видимости
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * FS Класс модуля работы с файловой системой
	 */
	typedef class FS {
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
			// Создаём объект фреймворка
			const fmk_t * _fmk;
			// Создаём объект работы с логами
			const log_t * _log;
		public:
			/**
			 * uid Метод вывода идентификатора пользователя
			 * @param name имя пользователя
			 * @return     полученный идентификатор пользователя
			 */
			uid_t uid(const string & name) const noexcept;
			/**
			 * gid Метод вывода идентификатора группы пользователя
			 * @param name название группы пользователя
			 * @return     полученный идентификатор группы пользователя
			 */
			gid_t gid(const string & name) const noexcept;
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
			 * istype Метод определяющая тип файловой системы по адресу
			 * @param addr   адрес дирректории
			 * @param actual флаг проверки актуальных файлов
			 * @return       тип файловой системы
			 */
			type_t type(const string & addr, const bool actual = true) const noexcept;
		public:
			/**
			 * size Метод подсчёта размера файла/каталога
			 * @param path полный путь для подсчёта размера
			 * @param ext  расширение файла если требуется фильтрация
			 * @return     общий размер файла/каталога
			 */
			uintmax_t size(const string & path, const string & ext = "") const noexcept;
			/**
			 * count Метод подсчёта количество файлов в каталоге
			 * @param path путь для подсчёта
			 * @param ext  расширение файла если требуется фильтрация
			 * @return     количество файлов в каталоге
			 */
			uintmax_t count(const string & path, const string & ext = "") const noexcept;
		public:
			/**
			 * delPath Метод удаления полного пути
			 * @param path полный путь для удаления
			 * @return     количество дочерних элементов
			 */
			int delPath(const string & path) const noexcept;
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
			 * @param before флаг определения первой точки расширения слева
			 */
			pair <string, string> components(const string & addr, const bool before = false) const noexcept;
		public:
			/**
			 * Выполняем работу для Unix
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
				/**
				 * chmod Метод получения метаданных файла или каталога
				 * @param path полный путь к файлу или каталогу
				 * @return     запрашиваемые метаданные
				 */
				mode_t chmod(const string & path) const noexcept;
				/**
				 * chmod Метод изменения метаданных файла или каталога
				 * @param path полный путь к файлу или каталогу
				 * @param mode метаданные для установки
				 * @return     результат работы функции
				 */
				bool chmod(const string & path, const mode_t mode) const noexcept;
				/**
				 * chown Метод установки владельца на каталог
				 * @param path  путь к файлу или каталогу для установки владельца
				 * @param user  данные пользователя
				 * @param group идентификатор группы
				 * @return      результат работы функции
				 */
				bool chown(const string & path, const string & user, const string & group) const noexcept;
			#endif
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
		public:
			/**
			 * readDir Метод рекурсивного получения файлов во всех подкаталогах
			 * @param path     путь до каталога
			 * @param ext      расширение файла по которому идет фильтрация
			 * @param recurs   флаг рекурсивного перебора каталогов
			 * @param callback функция обратного вызова
			 * @param actual   флаг проверки актуальных файлов
			 */
			void readDir(const string & path, const string & ext, const bool recurs, function <void (const string &)> callback, const bool actual = true) const noexcept;
			/**
			 * readPath Метод рекурсивного чтения файлов во всех подкаталогах
			 * @param path     путь до каталога
			 * @param ext      расширение файла по которому идет фильтрация
			 * @param recurs   флаг рекурсивного перебора каталогов
			 * @param callback функция обратного вызова
			 * @param actual   флаг проверки актуальных файлов
			 */
			void readPath(const string & path, const string & ext, const bool recurs, function <void (const string &, const string &)> callback, const bool actual = true) const noexcept;
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
