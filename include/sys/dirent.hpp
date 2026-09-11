/**
 * @file: dirent.hpp
 * @date: 2026-09-12
 *
 * @brief Обход каталогов приёмами POSIX для оснастки MSVC
 *
 * @author Forman <info@anyks.com>
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_DIRENT_BASE__
#define __AWH_DIRENT_BASE__

/**
 * \~russian
 * @brief Обход каталогов приёмами POSIX
 *
 * @details Заголовка `dirent.h` у оснастки MSVC нет вовсе: он принадлежит наречиям POSIX,
 *          а MinGW несёт его своей частью. Библиотека же обходит каталоги именно этими
 *          приёмами - их около полусотни обращений в двух файлах, - и переписывать их на
 *          средства системы значило бы завести второе устройство обхода ради одной оснастки
 *
 * @note Приёмы заведены обоими рядами, узким и широким: узкий держит пути в UTF-8, как
 *       того требует остальная библиотека, а широкий отдаёт названия так, как их хранит
 *       сама система. Узкий ряд опирается на широкий, а не на средства кодовой страницы:
 *       иначе названия со знаками вне страницы процесса приходили бы искажёнными
 *
 * @warning Ячейка названия принадлежит ИМЕННО ЭТОМУ каталогу, а не потоку исполнения:
 *          читая два каталога вперемежку, вызывающий обязан получать два разных названия.
 *          Общая на поток ячейка отдавала бы название второго на месте первого
 *
 * \~english
 * @brief Directory traversal by the POSIX means
 * @details MSVC has no `dirent.h` header at all: it belongs to the POSIX dialects, while MinGW
 *          carries it as a part of itself. The library traverses directories precisely by those
 *          means - about fifty calls in two files - and rewriting them onto the system facilities
 *          would mean introducing a second traversal design for the sake of one toolchain
 *
 * \~
 */
#if defined(_MSC_VER)

/**
 * Подключаем единую точку подключения системных заголовков MS Windows
 */
#include <sys/win32.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <cstring>

/**
 * @brief Запись широкого каталога
 *
 */
struct _wdirent {
	// Длина названия записи в знаках
	size_t d_namlen;
	// Название записи каталога
	wchar_t d_name[MAX_PATH];
};

/**
 * @brief Описатель широкого каталога
 *
 * @note Признак первой записи обязателен: система отдаёт первую запись уже самим
 *       открытием поиска, и без признака она пропускалась бы
 */
struct _WDIR {
	// Описатель поиска системы
	HANDLE handle;
	// Признак того, что первая запись ещё не выдана
	bool first;
	// Образец поиска, нужный для перемотки каталога к началу
	std::wstring pattern;
	// Запись, отдаваемая вызывающему
	struct _wdirent entry;
};

/**
 * @brief Запись узкого каталога
 *
 */
struct dirent {
	// Длина названия записи в октетах
	size_t d_namlen;
	// Название записи каталога в UTF-8 (четыре октета на знак в худшем случае)
	char d_name[MAX_PATH * 4];
};

/**
 * @brief Описатель узкого каталога
 *
 */
struct DIR {
	// Описатель широкого каталога, на какой опирается узкий
	struct _WDIR * wide;
	// Запись, отдаваемая вызывающему
	struct dirent entry;
};

/**
 * @brief Функция открытия широкого каталога
 *
 * @param path путь к каталогу
 * @return     описатель открытого каталога либо пустое значение
 *
 */
static inline struct _WDIR * _wopendir(const wchar_t * path) noexcept {
	// Если путь не передан
	if((path == nullptr) || (path[0] == L'\0')){
		// Сообщаем о неверном доводе
		::SetLastError(ERROR_INVALID_PARAMETER);
		// Выходим из функции
		return nullptr;
	}
	// Создаём описатель каталога
	struct _WDIR * result = new (std::nothrow) struct _WDIR();
	// Если памяти не хватило
	if(result == nullptr)
		// Выходим из функции
		return nullptr;
	// Собираем образец поиска: системе нужен путь со звёздочкой
	result->pattern = path;
	// Если путь не оканчивается разделителем
	if((result->pattern.back() != L'\\') && (result->pattern.back() != L'/'))
		// Добавляем разделитель
		result->pattern.append(1, L'\\');
	// Добавляем образец, под какой подходит всякая запись
	result->pattern.append(L"*");
	// Данные первой найденной записи
	WIN32_FIND_DATAW data{};
	// Выполняем открытие поиска
	result->handle = ::FindFirstFileW(result->pattern.c_str(), &data);
	// Если открыть поиск не удалось
	if(result->handle == INVALID_HANDLE_VALUE){
		// Удаляем описатель каталога
		delete result;
		// Выходим из функции
		return nullptr;
	}
	// Запоминаем первую запись: она выдана уже самим открытием
	::wcsncpy_s(result->entry.d_name, data.cFileName, _TRUNCATE);
	// Запоминаем длину названия записи
	result->entry.d_namlen = ::wcslen(result->entry.d_name);
	// Отмечаем, что первая запись ещё не выдана вызывающему
	result->first = true;
	// Выводим описатель каталога
	return result;
}
/**
 * @brief Функция чтения записи широкого каталога
 *
 * @param dir описатель открытого каталога
 * @return    запись каталога либо пустое значение по исчерпании
 *
 */
static inline struct _wdirent * _wreaddir(struct _WDIR * dir) noexcept {
	// Если описатель каталога не передан
	if(dir == nullptr)
		// Выходим из функции
		return nullptr;
	// Если первая запись ещё не выдана
	if(dir->first){
		// Отмечаем первую запись выданной
		dir->first = false;
		// Выводим запись, добытую открытием
		return &dir->entry;
	}
	// Данные очередной найденной записи
	WIN32_FIND_DATAW data{};
	// Если очередной записи нет
	if(!::FindNextFileW(dir->handle, &data))
		// Выходим из функции
		return nullptr;
	// Запоминаем название записи
	::wcsncpy_s(dir->entry.d_name, data.cFileName, _TRUNCATE);
	// Запоминаем длину названия записи
	dir->entry.d_namlen = ::wcslen(dir->entry.d_name);
	// Выводим запись каталога
	return &dir->entry;
}
/**
 * @brief Функция перемотки широкого каталога к началу
 *
 * @param dir описатель открытого каталога
 *
 */
static inline void _wrewinddir(struct _WDIR * dir) noexcept {
	// Если описатель каталога не передан
	if(dir == nullptr)
		// Выходим из функции
		return;
	// Закрываем прежний поиск
	if(dir->handle != INVALID_HANDLE_VALUE)
		// Выполняем закрытие поиска
		::FindClose(dir->handle);
	// Данные первой найденной записи
	WIN32_FIND_DATAW data{};
	// Открываем поиск заново тем же образцом
	dir->handle = ::FindFirstFileW(dir->pattern.c_str(), &data);
	// Если открыть поиск не удалось
	if(dir->handle == INVALID_HANDLE_VALUE){
		// Отмечаем, что выдавать нечего
		dir->first = false;
		// Выходим из функции
		return;
	}
	// Запоминаем первую запись
	::wcsncpy_s(dir->entry.d_name, data.cFileName, _TRUNCATE);
	// Запоминаем длину названия записи
	dir->entry.d_namlen = ::wcslen(dir->entry.d_name);
	// Отмечаем, что первая запись ещё не выдана
	dir->first = true;
}
/**
 * @brief Функция закрытия широкого каталога
 *
 * @param dir описатель открытого каталога
 * @return    ноль при успехе
 *
 */
static inline int _wclosedir(struct _WDIR * dir) noexcept {
	// Если описатель каталога не передан
	if(dir == nullptr)
		// Выходим из функции
		return -1;
	// Если поиск открыт
	if(dir->handle != INVALID_HANDLE_VALUE)
		// Выполняем закрытие поиска
		::FindClose(dir->handle);
	// Удаляем описатель каталога
	delete dir;
	// Сообщаем об успехе
	return 0;
}
/**
 * @brief Функция открытия каталога
 *
 * @param path путь к каталогу в UTF-8
 * @return     описатель открытого каталога либо пустое значение
 *
 */
static inline DIR * opendir(const char * path) noexcept {
	// Если путь не передан
	if((path == nullptr) || (path[0] == '\0')){
		// Сообщаем о неверном доводе
		::SetLastError(ERROR_INVALID_PARAMETER);
		// Выходим из функции
		return nullptr;
	}
	// Получаем длину пути в широких знаках
	const int length = ::MultiByteToWideChar(CP_UTF8, 0, path, -1, nullptr, 0);
	// Если перевод не удался
	if(length <= 0)
		// Выходим из функции
		return nullptr;
	// Собираем путь в широких знаках
	std::wstring wide(static_cast <size_t> (length - 1), L'\0');
	// Выполняем перевод пути
	::MultiByteToWideChar(CP_UTF8, 0, path, -1, &wide[0], length);
	// Открываем широкий каталог
	struct _WDIR * handle = ::_wopendir(wide.c_str());
	// Если открыть каталог не удалось
	if(handle == nullptr)
		// Выходим из функции
		return nullptr;
	// Создаём описатель узкого каталога
	DIR * result = new (std::nothrow) DIR();
	// Если памяти не хватило
	if(result == nullptr){
		// Закрываем широкий каталог
		::_wclosedir(handle);
		// Выходим из функции
		return nullptr;
	}
	// Запоминаем описатель широкого каталога
	result->wide = handle;
	// Выводим описатель каталога
	return result;
}
/**
 * @brief Функция чтения записи каталога
 *
 * @param dir описатель открытого каталога
 * @return    запись каталога либо пустое значение по исчерпании
 *
 */
static inline struct dirent * readdir(DIR * dir) noexcept {
	// Если описатель каталога не передан
	if((dir == nullptr) || (dir->wide == nullptr))
		// Выходим из функции
		return nullptr;
	// Читаем запись широкого каталога
	struct _wdirent * source = ::_wreaddir(dir->wide);
	// Если записей больше нет
	if(source == nullptr)
		// Выходим из функции
		return nullptr;
	// Переводим название записи в UTF-8
	const int length = ::WideCharToMultiByte(CP_UTF8, 0, source->d_name, -1, dir->entry.d_name, static_cast <int> (sizeof(dir->entry.d_name)), nullptr, nullptr);
	// Если перевод не удался
	if(length <= 0)
		// Выходим из функции
		return nullptr;
	// Запоминаем длину названия записи без завершающего нуля
	dir->entry.d_namlen = static_cast <size_t> (length - 1);
	// Выводим запись каталога
	return &dir->entry;
}
/**
 * @brief Функция перемотки каталога к началу
 *
 * @param dir описатель открытого каталога
 *
 */
static inline void rewinddir(DIR * dir) noexcept {
	// Если описатель каталога передан
	if((dir != nullptr) && (dir->wide != nullptr))
		// Выполняем перемотку широкого каталога
		::_wrewinddir(dir->wide);
}
/**
 * @brief Функция закрытия каталога
 *
 * @param dir описатель открытого каталога
 * @return    ноль при успехе
 *
 */
static inline int closedir(DIR * dir) noexcept {
	// Если описатель каталога не передан
	if(dir == nullptr)
		// Выходим из функции
		return -1;
	// Закрываем широкий каталог
	if(dir->wide != nullptr)
		// Выполняем закрытие широкого каталога
		::_wclosedir(dir->wide);
	// Удаляем описатель каталога
	delete dir;
	// Сообщаем об успехе
	return 0;
}

#endif // _MSC_VER

#endif // __AWH_DIRENT_BASE__
