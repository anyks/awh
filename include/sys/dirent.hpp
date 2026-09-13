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

/**
 * Если сборка выполняется оснасткою Visual Studio
 */
#if _MSC_VER
	/**
	 * Стандартные заголовочные файлы
	 */
	#include <string>
	#include <cstring>
	/**
	 * Подключаем единую точку подключения системных заголовков MS Windows
	 */
	#include "macro/win32.hpp"
/**
 * Если сборка выполняется не оснасткою Visual Studio
 */
#else
	/**
	 * Подключаем заголовок обхода каталогов системы
	 */
	#include <dirent.h>
#endif

/**
 * \~russian
 * @brief Пространство имён обхода каталогов
 *
 * @details Пространство заведено затем, что устройство ниже заводит имена POSIX -
 *          `DIR`, `dirent`, `opendir` и прочие, - и заводись они в пространстве
 *          глобальном, всякий подключивший библиотеку получал бы их себе. Имена эти
 *          общеупотребительны, и пересечение с ними у стороннего кода дело времени.
 *
 * @note У систем POSIX имена эти приходят из `dirent.h` и живут в пространстве
 *       глобальном; сюда они вносятся объявлениями `using`, а не обёртками - обёртка
 *       завела бы лишний вызов там, где нужно одно лишь имя.
 *
 * @warning Имена широкого ряда сохранены со знаком подчёркивания впереди - `_WDIR`,
 *          `_wopendir` и прочие: ровно так их зовёт `dirent.h` у MinGW, и объявление
 *          `using` иначе их не подхватило бы. В пространстве имён знак этот законен:
 *          стандарт закрепляет за собою `_x` лишь в пространстве глобальном.
 *
 * \~english
 * @brief Namespace of the traversal of the directories
 *
 * @details The namespace is started because the design below introduces the POSIX names -
 *          the `DIR`, the `dirent`, the `opendir` and the others, - and were they introduced in the
 *          global namespace, everyone who included the library would get them for themselves. These names
 *          are in common use, and an intersection with them at a foreign code is a matter of time.
 *
 * @note At the POSIX systems these names come from the `dirent.h` and live in the global
 *       namespace; they are brought here by the `using` declarations, and not by the wrappers - a wrapper
 *       would introduce an extra call where only a name is needed.
 *
 * @warning The names of the wide row are kept with the underscore in front - the `_WDIR`,
 *          the `_wopendir` and the others: exactly so they are called by the `dirent.h` at the MinGW, and the
 *          `using` declaration would not pick them up otherwise. In a namespace this sign is legal:
 *          the standard reserves the `_x` for itself only in the global namespace.
 *
 * \~
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * dir пространство имён обхода каталогов
	 */
	namespace dir {
		/**
		 * Если сборка выполняется оснасткою Visual Studio
		 *
		 * @details Заголовка `dirent.h` у неё нет вовсе: он принадлежит наречиям POSIX, а
		 *          MinGW несёт его своей частью. Оттого устройство обхода заводится здесь
		 */
		#if _MSC_VER
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
			 * @brief Структура описателя широкого каталога
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
				wstring pattern;
				// Запись, отдаваемая вызывающему
				struct _wdirent entry;
			};

			/**
			 * @brief Структура записи узкого каталога
			 *
			 */
			struct dirent {
				// Длина названия записи в октетах
				size_t d_namlen;
				// Название записи каталога в UTF-8 (четыре октета на знак в худшем случае)
				char d_name[MAX_PATH * 4];
			};

			/**
			 * @brief Структура описателя узкого каталога
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
			inline struct _WDIR * _wopendir(const wchar_t * path) noexcept {
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
			inline struct _wdirent * _wreaddir(struct _WDIR * dir) noexcept {
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
			inline void _wrewinddir(struct _WDIR * dir) noexcept {
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
			inline int32_t _wclosedir(struct _WDIR * dir) noexcept {
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
			inline DIR * opendir(const char * path) noexcept {
				// Если путь не передан
				if((path == nullptr) || (path[0] == '\0')){
					// Сообщаем о неверном доводе
					::SetLastError(ERROR_INVALID_PARAMETER);
					// Выходим из функции
					return nullptr;
				}
				// Получаем длину пути в широких знаках
				const int32_t length = ::MultiByteToWideChar(CP_UTF8, 0, path, -1, nullptr, 0);
				// Если перевод не удался
				if(length <= 0)
					// Выходим из функции
					return nullptr;
				// Собираем путь в широких знаках
				std::wstring wide(static_cast <size_t> (length - 1), L'\0');
				// Выполняем перевод пути
				::MultiByteToWideChar(CP_UTF8, 0, path, -1, &wide[0], length);
				// Открываем широкий каталог
				struct _WDIR * handle = _wopendir(wide.c_str());
				// Если открыть каталог не удалось
				if(handle == nullptr)
					// Выходим из функции
					return nullptr;
				// Создаём описатель узкого каталога
				DIR * result = new (std::nothrow) DIR();
				// Если памяти не хватило
				if(result == nullptr){
					// Закрываем широкий каталог
					_wclosedir(handle);
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
			inline struct dirent * readdir(DIR * dir) noexcept {
				// Если описатель каталога не передан
				if((dir == nullptr) || (dir->wide == nullptr))
					// Выходим из функции
					return nullptr;
				// Читаем запись широкого каталога
				struct _wdirent * source = _wreaddir(dir->wide);
				// Если записей больше нет
				if(source == nullptr)
					// Выходим из функции
					return nullptr;
				// Переводим название записи в UTF-8
				const int32_t length = ::WideCharToMultiByte(CP_UTF8, 0, source->d_name, -1, dir->entry.d_name, static_cast <int32_t> (sizeof(dir->entry.d_name)), nullptr, nullptr);
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
			inline void rewinddir(DIR * dir) noexcept {
				// Если описатель каталога передан
				if((dir != nullptr) && (dir->wide != nullptr))
					// Выполняем перемотку широкого каталога
					_wrewinddir(dir->wide);
			}
			/**
			 * @brief Функция закрытия каталога
			 *
			 * @param dir описатель открытого каталога
			 * @return    ноль при успехе
			 *
			 */
			inline int32_t closedir(DIR * dir) noexcept {
				// Если описатель каталога не передан
				if(dir == nullptr)
					// Выходим из функции
					return -1;
				// Закрываем широкий каталог
				if(dir->wide != nullptr)
					// Выполняем закрытие широкого каталога
					_wclosedir(dir->wide);
				// Удаляем описатель каталога
				delete dir;
				// Сообщаем об успехе
				return 0;
			}
		/**
		 * Если сборка выполняется не оснасткою Visual Studio
		 */
		#else
			/**
			 * Вносим имена узкого ряда из заголовка системы
			 */
			using ::DIR;
			using ::dirent;
			using ::opendir;
			using ::readdir;
			using ::closedir;
			using ::rewinddir;
			/**
			 * Если сборка выполняется под MS Windows
			 *
			 * @details Широкий ряд несёт один лишь MinGW: прочим системам POSIX он не
			 *          нужен вовсе, названия у них и без того приходят октетами
			 */
			#if _WIN32 || _WIN64
				/**
				 * Вносим имена широкого ряда из заголовка системы
				 */
				using ::_WDIR;
				using ::_wdirent;
				using ::_wopendir;
				using ::_wreaddir;
				using ::_wclosedir;
				using ::_wrewinddir;
			#endif
		#endif
	}
}

#endif // __AWH_DIRENT_BASE__
