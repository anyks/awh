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

#ifndef __FS_DANUBE__
#define __FS_DANUBE__

/**
 * Стандартная библиотека
 */
#include <string>
#include <fstream>
#include <codecvt>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <functional>
#include <dirent.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
// Если это clang v10 или выше
#if defined(__DANUBE_EXPERIMENTAL__)
	#include <filesystem>
#endif

// Если - это Windows
#if defined(_WIN32) || defined(_WIN64)
	#include <conio.h>
	#include <direct.h>
// Если - это Unix
#else
	#include <grp.h>
	#include <pwd.h>
	#include <sys/mman.h>
#endif

/**
 * Наши модули
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>

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
				LNK  = 0x04, // Символьная ссылка
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
			 * @param name адрес дирректории
			 * @return     результат проверки
			 */
			bool isDir(const string & name) const noexcept;
			/**
			 * isFile Метод проверяющий существование файла
			 * @param name адрес файла
			 * @return     результат проверки
			 */
			bool isFile(const string & name) const noexcept;
			/**
			 * isSock Метод проверки существования сокета
			 * @param name адрес сокета
			 * @return     результат проверки
			 */
			bool isSock(const string & name) const noexcept;
		public:
			/**
			 * istype Метод определяющая тип файловой системы по адресу
			 * @param name адрес дирректории
			 * @return     тип файловой системы
			 */
			type_t type(const string & name) const noexcept;
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
			 * @param path путь который нужно определить
			 * @return     полный путь
			 */
			string realPath(const string & path) const noexcept;
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
			 * fileComponents Метод извлечения названия и расширения файла
			 * @param filename адрес файла для извлечения его параметров
			 */
			pair <string, string> fileComponents(const string & filename) const noexcept;
		public:
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
		public:
			/**
			 * chown Метод установки владельца на каталог
			 * @param path  путь к файлу или каталогу для установки владельца
			 * @param user  данные пользователя
			 * @param group идентификатор группы
			 */
			void chown(const string & path, const string & user, const string & group) const noexcept;
		public:
			/**
			 * readFile Метод рекурсивного получения всех строк файла
			 * @param filename адрес файла для чтения
			 * @param callback функция обратного вызова
			 */
			void readFile(const string & filename, function <void (const string &, const uintmax_t)> callback) const noexcept;
			/**
			 * readFile2 Метод рекурсивного получения всех строк файла (стандартным способом)
			 * @param filename адрес файла для чтения
			 * @param callback функция обратного вызова
			 */
			void readFile2(const string & filename, function <void (const string &, const uintmax_t)> callback) const noexcept;
		public:
			/**
			 * readDir Метод рекурсивного получения файлов во всех подкаталогах
			 * @param path     путь до каталога
			 * @param ext      расширение файла по которому идет фильтрация
			 * @param rec      флаг рекурсивного перебора каталогов
			 * @param callback функция обратного вызова
			 */
			void readDir(const string & path, const string & ext, const bool rec, function <void (const string &, const uintmax_t)> callback) const noexcept;
			/**
			 * readPath Метод рекурсивного чтения файлов во всех подкаталогах
			 * @param path     путь до каталога
			 * @param ext      расширение файла по которому идет фильтрация
			 * @param rec      флаг рекурсивного перебора каталогов
			 * @param callback функция обратного вызова
			 */
			void readPath(const string & path, const string & ext, const bool rec, function <void (const string &, const string &, const uintmax_t, const uintmax_t)> callback) const noexcept;
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

#endif // __FS_DANUBE__
