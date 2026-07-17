/**
 * @file: fs.hpp
 * @date: 2026-01-22
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
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
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Класс модуля работы с файловой системой
	 *
	 */
	typedef class __AWH_SHARED_EXPORT__ Filesystem {
		public:
			/**
			 * @brief Типы смещений в файле
			 *
			 */
			enum class seek_t : uint8_t {
				BEGIN   = 0x00, // Смещение от начала файла
				CURRENT = 0x01, // Смещение от текущей позиции
				END     = 0x02  // Смещение от конца файла
			};
			/**
			 * @brief Тип файловой системы
			 *
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
			 * @brief Тип прав доступа к файлу или каталогу
			 *
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
			 * @brief Метод создания символьной ссылки
			 *
			 * @param first  адрес на который нужно сделать ссылку
			 * @param second адрес где должна быть создана ссылка
			 */
			void symlink(string_view first, string_view second) const noexcept;
			/**
			 * @brief Метод создания жёстких ссылок
			 *
			 * @param first  адрес на который нужно сделать ссылку
			 * @param second адрес где должна быть создана ссылка
			 */
			void hardlink(string_view first, string_view second) const noexcept;
		public:
			/**
			 * @brief Метод удаления адреса файловой системы
			 *
			 * @param addr    полный адрес для удаления
			 * @param resolve флаг резолвинга символьных ссылок
			 * @return        результат удаления
			 */
			bool unlink(string_view addr, const bool resolve = false) const noexcept;
		public:
			/**
			 * @brief Метод, определяющий тип файловой системы по адресу
			 *
			 * @param addr        адрес директории или файла
			 * @param detectLinks флаг детектирования символьных ссылок (на горячих путях можно отключить)
			 * @return            тип файловой системы
			 */
			type_t type(string_view addr, const bool detectLinks = true) const noexcept;
		public:
			/**
			 * @brief Метод извлечения реального адреса
			 *
			 * @param addr    адрес который нужно определить
			 * @param resolve флаг резолвинга символьных ссылок
			 * @return        полный путь
			 */
			string fullpath(string_view addr, const bool resolve = false) const noexcept;
		public:
			/**
			 * @brief Метод получения прав доступа к файлу или каталогу
			 *
			 * @param addr путь к файлу или каталогу
			 * @return     запрашиваемые метаданные
			 */
			uint32_t chmod(string_view addr) const noexcept;
			/**
			 * @brief Метод изменения прав доступа к файлу или каталогу
			 *
			 * @param addr путь к файлу или каталогу
			 * @param mode метаданные для установки
			 * @return     результат работы функции
			 */
			bool chmod(string_view addr, const uint32_t mode) const noexcept;
		public:
			/**
			 * @brief Метод установки владельца на файл или каталог
			 *
			 * @param addr  путь к файлу или каталогу для установки владельца
			 * @param user  имя пользователя
			 * @param group название группы пользователя
			 * @return      результат работы функции
			 */
			bool chown(string_view addr, string_view user, string_view group = "") const noexcept;
		public:
			/**
			 * @brief Метод рекурсивного создания пути
			 *
			 * @param addr адрес для создания каталога
			 * @return     результат создания каталога
			 */
			bool mkdir(string_view addr) const noexcept;
			/**
			 * @brief Метод создания каталога с указанием владельца
			 *
			 * @param addr  адрес для создания каталога
			 * @param user  имя пользователя
			 * @param group название группы пользователя
			 * @return      результат создания каталога
			 */
			bool mkdir(string_view addr, string_view user, string_view group) const noexcept;
		public:
			/**
			 * @brief Метод извлечения названия и расширения файла
			 *
			 * @param addr    путь к файлу для извлечения его параметров
			 * @param resolve флаг резолвинга символьных ссылок
			 * @param before  флаг определения первой точки расширения слева
			 */
			components_t components(string_view addr, const bool resolve = false, const bool before = false) const noexcept;
		public:
			/**
			 * @brief Метод подсчёта размера файла/каталога
			 *
			 * @param addr    адрес для подсчёта размера
			 * @param ext     расширение файла если требуется фильтрация
			 * @param recurse флаг рекурсивного перебора каталогов
			 * @return        общий размер файла/каталога
			 */
			uintmax_t size(string_view addr, string_view ext = "", const bool recurse = true) const noexcept;
			/**
			 * @brief Метод подсчёта количества файлов в каталоге
			 *
			 * @param addr    адрес для подсчёта количества файлов
			 * @param ext     расширение файла если требуется фильтрация
			 * @param recurse флаг рекурсивного перебора каталогов
			 * @return        количество файлов в каталоге
			 */
			uintmax_t count(string_view addr, string_view ext = "", const bool recurse = true) const noexcept;
		public:
			/**
			 * @brief Шаблон метода добавления в файл бинарных данных
			 *
			 * @tparam T тип буфера данных
			 */
			template <typename T>
			/**
			 * @brief Метод добавления в файл бинарных данных
			 *
			 * @param filename путь к файлу в который необходимо выполнить запись
			 * @param buffer   бинарный буфер который необходимо записать в файл
			 */
			void append(string_view filename, const T & buffer) const noexcept;
			/**
			 * @brief Метод добавления в файл бинарных данных
			 *
			 * @param filename путь к файлу в который необходимо выполнить запись
			 * @param buffer   бинарный буфер который необходимо записать в файл
			 */
			void append(string_view filename, const char * buffer) const noexcept;
			/**
			 * @brief Метод добавления в файл бинарных данных
			 *
			 * @param filename путь к файлу в который необходимо выполнить запись
			 * @param buffer   бинарный буфер который необходимо записать в файл
			 */
			void append(string_view filename, const wchar_t * buffer) const noexcept;
			/**
			 * @brief Метод добавления в файл бинарных данных
			 *
			 * @param filename путь к файлу в который необходимо выполнить запись
			 * @param buffer   бинарный буфер который необходимо записать в файл
			 * @param size     размер бинарного буфера для записи в файл
			 */
			void append(string_view filename, const void * buffer, const size_t size) const noexcept;
		public:
			/**
			 * @brief Шаблон метода чтения данных из файла
			 *
			 * @tparam T тип возвращаемого результата
			 */
			template <typename T>
			/**
			 * @brief Метод чтения данных из файла
			 *
			 * @param filename путь к файлу для чтения
			 * @param seek     тип смещения в файле
			 * @param offset   смещение в файле
			 * @return         бинарный буфер с прочитанными данными
			 */
			auto read(string_view filename, const seek_t seek = seek_t::BEGIN, const size_t offset = 0) const noexcept -> T;
			/**
			 * @brief Шаблон метода чтения данных из файла
			 *
			 * @tparam T тип возвращаемого результата
			 */
			template <typename T>
			/**
			 * @brief Метод чтения данных из файла
			 *
			 * @param filename путь к файлу для чтения
			 * @param result   контейнер куда следует положить результат
			 * @param seek     тип смещения в файле
			 * @param offset   смещение в файле
			 */
			void read(string_view filename, T & result, const seek_t seek = seek_t::BEGIN, const size_t offset = 0) const noexcept;
		public:
			/**
			 * @brief Метод рекурсивного чтения больших файлов блоками с обратным вызовом
			 *
			 * @param filename путь к файлу для чтения
			 * @param size     размер блока для чтения
			 * @param callback функция обратного вызова для обработки прочитанных данных (возвращает true для продолжения чтения и false для остановки)
			 * @param offset   смещение в файле с которого следует начать чтение
			 */
			void read(string_view filename, const size_t size, const function <bool (const void * buffer, const size_t size, const size_t offset, const size_t left)> & callback, const size_t offset = 0) const noexcept;
		public:
			/**
			 * @brief Шаблон метода записи в файл бинарных данных
			 *
			 * @tparam T тип буфера данных
			 */
			template <typename T>
			/**
			 * @brief Метод записи в файл бинарных данных
			 *
			 * @param filename путь к файлу в который необходимо выполнить запись
			 * @param buffer   бинарный буфер который необходимо записать в файл
			 * @param seek     тип смещения в файле
			 * @param offset   смещение в файле
			 */
			void write(string_view filename, const T & buffer, const seek_t seek = seek_t::BEGIN, const size_t offset = 0) const noexcept;
			/**
			 * @brief Метод записи в файл бинарных данных
			 *
			 * @param filename путь к файлу в который необходимо выполнить запись
			 * @param buffer   бинарный буфер который необходимо записать в файл
			 * @param seek     тип смещения в файле
			 * @param offset   смещение в файле
			 */
			void write(string_view filename, const char * buffer, const seek_t seek = seek_t::BEGIN, const size_t offset = 0) const noexcept;
			/**
			 * @brief Метод записи в файл бинарных данных
			 *
			 * @param filename путь к файлу в который необходимо выполнить запись
			 * @param buffer   бинарный буфер который необходимо записать в файл
			 * @param seek     тип смещения в файле
			 * @param offset   смещение в файле
			 */
			void write(string_view filename, const wchar_t * buffer, const seek_t seek = seek_t::BEGIN, const size_t offset = 0) const noexcept;
			/**
			 * @brief Метод записи в файл бинарных данных
			 *
			 * @param filename путь к файлу в который необходимо выполнить запись
			 * @param buffer   бинарный буфер который необходимо записать в файл
			 * @param size     размер бинарного буфера для записи в файл
			 * @param seek     тип смещения в файле
			 * @param offset   смещение в файле
			 */
			void write(string_view filename, const void * buffer, const size_t size, const seek_t seek = seek_t::BEGIN, const size_t offset = 0) const noexcept;
		public:
			/**
			 * @brief Метод рекурсивного получения всех строк файла
			 *
			 * @param filename путь к файлу для чтения
			 * @param callback функция обратного вызова
			 * @param seek     тип смещения в файле
			 * @param offset   смещение в файле
			 */
			void readfile(string_view filename, const function <void (string_view)> & callback, const seek_t seek = seek_t::BEGIN, const size_t offset = 0) const noexcept;
			/**
			 * @brief Метод рекурсивного получения буфера данных из больших файлов
			 *
			 * @param filename путь к файлу для чтения
			 * @param size     размер буфера для чтения файла
			 * @param callback функция обратного вызова
			 * @param seek     тип смещения в файле
			 * @param offset   смещение в файле
			 */
			void readfile(string_view filename, const size_t size, const function <void (const void *, const size_t)> & callback, const seek_t seek = seek_t::BEGIN, const size_t offset = 0) const noexcept;
		public:
			/**
			 * @brief Метод рекурсивного получения файлов во всех подкаталогах
			 *
			 * @param path     путь до каталога
			 * @param ext      расширение файла по которому идет фильтрация
			 * @param recurse  флаг рекурсивного перебора каталогов
			 * @param callback функция обратного вызова
			 * @param resolve  флаг резолвинга символьных ссылок
			 */
			void readdir(string_view path, string_view ext, const bool recurse, const function <void (const type_t, string_view)> & callback, const bool resolve = true) const noexcept;
			/**
			 * @brief Метод рекурсивного чтения файлов во всех подкаталогах построчно
			 *
			 * @param path     путь до каталога
			 * @param ext      расширение файла по которому идет фильтрация
			 * @param recurse  флаг рекурсивного перебора каталогов
			 * @param callback функция обратного вызова
			 * @param resolve  флаг резолвинга символьных ссылок
			 */
			void readdir(string_view path, string_view ext, const bool recurse, const function <void (const type_t, string_view, string_view)> & callback, const bool resolve = true) const noexcept;
			/**
			 * @brief Метод рекурсивного чтения файлов во всех подкаталогах бинарными блоками
			 *
			 * @param path     путь до каталога
			 * @param ext      расширение файла по которому идет фильтрация
			 * @param size     размер буфера для чтения файла
			 * @param recurse  флаг рекурсивного перебора каталогов
			 * @param callback функция обратного вызова
			 * @param resolve  флаг резолвинга символьных ссылок
			 */
			void readdir(string_view path, string_view ext, const size_t size, const bool recurse, const function <void (const type_t, string_view, const void *, const size_t)> & callback, const bool resolve = true) const noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			explicit Filesystem(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Filesystem() noexcept {}
	} fs_t;
};

#endif // __AWH_FS__
