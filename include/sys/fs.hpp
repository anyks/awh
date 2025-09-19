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
 * Для операционной системы OS Windows
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
 * Для операционной системы OS Windows
 */
#if _WIN32 || _WIN64
	/**
	 * Подключаем стандартные модули
	 */
	#include <conio.h>
	#include <direct.h>
/**
 * Для операционной системы не являющейся OS Windows
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
			pair <string, string> components(const string & addr, const bool actual = true, const bool before = false) const noexcept;
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
			 * Для операционной системы не являющейся OS Windows
			 */
			#if !_WIN32 && !_WIN64
				/**
				 * @brief Метод установки владельца на файл или каталог
				 *
				 * @param path  путь к файлу или каталогу для установки владельца
				 * @param user  данные пользователя
				 * @param group идентификатор группы
				 * @return      результат работы функции
				 */
				bool chown(const string & path, const string & user, const string & group) const noexcept;
			/**
			 * Для операционной системы OS Windows
			 */
			#else
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
			 * @brief Метод чтения данных из файла
			 *
			 * @param filename адрес файла для чтения
			 * @return         бинарный буфер с прочитанными данными
			 */
			vector <char> read(const string & filename) const noexcept;
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
			FS(const fmk_t * fmk, const log_t * log) noexcept : _pid(::getpid()), _fmk(fmk), _log(log) {}
			/**
			 * @brief деструктор
			 *
			 */
			~FS() noexcept {}
	} fs_t;
};

#endif // __AWH_FS__
