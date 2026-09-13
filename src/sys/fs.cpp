/**
 * @file fs.cpp
 * @date 2026-01-23
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
 * @brief Реализация модуля работы с файловой системой — чтение и запись файлов, обход каталогов, получение атрибутов,
 *        создание и удаление объектов ФС с нативной поддержкой macOS, Windows, Linux, FreeBSD, NetBSD, OpenBSD,
 *        Solaris и OpenIndiana
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
	/**
	 * Подключаем единую точку подключения системных заголовков MS Windows
	 *
	 * @note Подключается она прежде прочих заголовков MS Windows: те самостоятельными
	 *       не являются и требуют, чтобы базовые типы были объявлены до них
	 *
	 * @warning Порядок этот обязателен, а не опрятен: `shlobj.h` тянет за собой ветхий
	 *          `winsock.h`, и подключённый следом `winsock2.h` объявляет `sockaddr`
	 *          вторично. Оснастка MinGW такое сносила молча, MSVC отвечает отказом
	 *          «переопределение типа struct» - и указывает при этом на заголовок
	 *          системы, а не на место, где порядок нарушен
	 */
	#include <sys/macro/win32.hpp>

	/**
	 * Системные заголовочные файлы
	 */
	#include <tchar.h>
	#include <shlobj.h>
	#include <objbase.h>
	#include <strsafe.h>
#endif

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <climits>
#include <cstring>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <fcntl.h>
#include <sys/dirent.hpp>
/**
 * Заголовок замков файлов принадлежит наречиям POSIX: у оснастки MSVC его нет вовсе,
 * а приёмов его библиотека здесь не зовёт - подключение остаётся лишь для тех систем,
 * где он есть
 */
#if !defined(_MSC_VER)
	#include <sys/file.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>

/**
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
	/**
	 * Системные заголовочные файлы
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
	 * Системные заголовочные файлы
	 */
	#include <pwd.h>
	#include <unistd.h>
	#include <sys/mman.h>
#endif

/**
 * Если операционной системой является macOS
 */
#if __APPLE__ || __MACH__
	/**
	 * Системный заголовочный файл
	 */
	#include <TargetConditionals.h>

	/**
	 * Если целевая платформа является настольной macOS и разбор alias-файлов не отключён
	 *
	 * @note Признак TARGET_OS_MAC равен единице на всех платформах Apple, а не на
	 *       одной macOS, поэтому настольную систему отличает именно отсутствие
	 *       TARGET_OS_IPHONE: он покрывает iOS, tvOS, watchOS и visionOS. Alias
	 *       есть только в настольной файловой системе, и разбирать его больше негде
	 *
	 * @note Разбор отключается параметром сборки CMAKE_NO_MACOS_ALIAS. Отключать
	 *       его есть смысл там, где процесс не работает с Foundation больше ни для
	 *       чего: только этот разбор и тянет в процесс связку libobjc,
	 *       CoreFoundation и Foundation, а с ней 4.2 МБ резидентных страниц. Для
	 *       настольного приложения экономия мнимая - Foundation там загружен и без
	 *       нас, - а для служб и утилит настоящая
	 */
	#if (TARGET_OS_MAC && !TARGET_OS_IPHONE) && !defined(AWH_NO_MACOS_ALIAS)
		/**
		 * Включаем поддержку Objective-C автоматического управления памятью
		 */
		#define __AWH_USE_MACOS_ALIAS_RESOLUTION__ 1
		// ← #import допустим в .cpp при -x objective-c++
		#import <Foundation/Foundation.h>
	#endif
#endif

/**
 * Подключаем заголовочный файл проекта
 */
#include <sys/fs.hpp>
#include <sys/fmk.hpp>
#include <sys/log.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Инкапсулируем статические типы данных в основное пространство имён
 *
 */
namespace awh {
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		/**
		 * @brief Класс для автоматического управления каталогом Windows (RAII)
		 *
		 */
		class HandleDir {
			private:
				/**
				 * @brief Место обхода каталога
				 *
				 */
				typedef struct Place {
					// Объект дескриптора вложенного каталога
					awh::dir::_WDIR * handle;
					// Адрес вложенного каталога
					string address;
					/**
					 * @brief Конструктор
					 *
					 * @param handle  объект дескриптора вложенного каталога
					 * @param address адрес вложенного каталога
					 *
					 */
					explicit Place(awh::dir::_WDIR * handle, string_view address) noexcept :
					 handle(handle), address{address} {}
				} place_t;
			private:
				// Признак того, что незавершённую запись надлежит отдать повторно
				bool _repeat;
				// Признак незавершённого обхода
				bool _active;
			private:
				// Число долей незавершённой записи, отданных прежде
				size_t _delivered;
			private:
				// Вид незавершённой записи обхода
				uint8_t _pendingType;
			private:
				/**
				 * @brief Незавершённая запись обхода
				 *
				 * @note Отклик у видов `walkdir`, отдающих СОДЕРЖИМОЕ файла, вправе остановить
				 *       обход посреди файла. Запись эта тогда прочитана не до конца, а место её
				 *       в каталоге уже пройдено, - и без памяти о ней остаток файла был бы утерян
				 *       молча. Здесь и хранится адрес такой записи, вид её и число уже отданных
				 *       долей: продолжение отдаёт её повторно, а доли, отданные прежде, пропускает
				 */
				string _pending;
				// Адрес каталога, которому объект служит
				string _address;
			private:
				// Объект дескриптора
				awh::dir::_WDIR * _handle;
			private:
				/**
				 * @brief Места обхода вложенных каталогов
				 *
				 * @note Корень обхода здесь НЕ лежит: он живёт в `_handle` и по исчерпании не
				 *       закрывается, а перематывается. Тем и окупается внешний объект каталога -
				 *       повторный обзор того же каталога идёт мимо повторного открытия
				 */
				vector <place_t> _places;
			public:
				/**
				 * @brief Метод забвения незавершённой записи обхода
				 *
				 */
				void done() noexcept;
				/**
				 * @brief Метод перемотки каталога к началу
				 *
				 */
				void rewind() noexcept;
				/**
				 * @brief Метод сброса состояния обхода
				 *
				 * @note Закрываются и вложенные каталоги, и сам корень: объект после сброса
				 *       чист и готов служить иному адресу
				 *
				 */
				void reset() noexcept;
			public:
				/**
				 * @brief Метод проверки пустоты стопки мест обхода
				 *
				 * @return признак того, что обход идёт по самому корню
				 *
				 */
				bool empty() const noexcept;
				/**
				 * @brief Метод проверки валидности
				 *
				 * @return валидность каталога
				 *
				 */
				bool valid() const noexcept;
				/**
				 * @brief Метод проверки необходимости повторной выдачи записи
				 *
				 * @return признак того, что незавершённую запись надлежит отдать повторно
				 *
				 */
				bool repeat() const noexcept;
			public:
				/**
				 * @brief Метод извлечения числа отданных прежде долей
				 *
				 * @return число долей незавершённой записи, отданных прежде
				 *
				 */
				size_t delivered() const noexcept;
				/**
				 * @brief Метод извлечения вида незавершённой записи обхода
				 *
				 * @return вид незавершённой записи
				 *
				 */
				uint8_t pendingType() const noexcept;
			public:
				/**
				 * @brief Метод установки объекта дескриптора
				 *
				 * @param handle объект дескриптора
				 *
				 */
				void set(awh::dir::_WDIR * handle) noexcept;
			public:
				/**
				 * @brief Метод проверки незавершённости обхода
				 *
				 * @return признак того, что обход начат и до конца не доведён
				 *
				 */
				bool active() const noexcept;
				/**
				 * @brief Метод установки признака незавершённости обхода
				 *
				 * @param active признак незавершённости обхода
				 *
				 */
				void active(const bool active) noexcept;
			public:
				/**
				 * @brief Метод извлечения адреса каталога, которому объект служит
				 *
				 * @return адрес каталога
				 *
				 */
				const string & address() const noexcept;
				/**
				 * @brief Метод установки адреса каталога, которому объект служит
				 *
				 * @param address адрес каталога
				 *
				 */
				void address(string_view address) noexcept;
			public:
				/**
				 * @brief Метод извлечения дескриптора верхнего места обхода
				 *
				 * @return объект дескриптора вложенного каталога
				 *
				 */
				awh::dir::_WDIR * top() const noexcept;
				/**
				 * @brief Метод извлечения адреса верхнего места обхода
				 *
				 * @return адрес вложенного каталога
				 *
				 */
				const string & topAddress() const noexcept;
			public:
				/**
				 * @brief Метод снятия верхнего места обхода
				 *
				 */
				void pop() noexcept;
				/**
				 * @brief Метод добавления места обхода
				 *
				 * @param handle  объект дескриптора вложенного каталога
				 * @param address адрес вложенного каталога
				 *
				 */
				void push(awh::dir::_WDIR * handle, string_view address) noexcept;
			public:
				/**
				 * @brief Метод извлечения адреса незавершённой записи обхода
				 *
				 * @return адрес незавершённой записи
				 *
				 */
				const string & pending() const noexcept;
				/**
				 * @brief Метод запоминания незавершённой записи обхода
				 *
				 * @param address   адрес незавершённой записи
				 * @param type      вид незавершённой записи
				 * @param delivered число долей записи, отданных прежде
				 *
				 */
				void pending(string_view address, const uint8_t type, const size_t delivered) noexcept;
			public:
				/**
				 * @brief Оператор приведения к типу
				 *
				 * @return объект дескриптора
				 *
				 */
				operator awh::dir::_WDIR * () const noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 */
				explicit HandleDir() noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param handle объект дескриптора
				 *
				 */
				explicit HandleDir(awh::dir::_WDIR * handle) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~HandleDir() noexcept;
		};
		/**
		 * @brief Метод забвения незавершённой записи обхода
		 *
		 */
		void HandleDir::done() noexcept {
			// Сбрасываем число отданных прежде долей
			this->_delivered = 0;
			// Сбрасываем вид незавершённой записи
			this->_pendingType = 0;
			// Снимаем признак повторной выдачи записи
			this->_repeat = false;
			// Выполняем очистку адреса незавершённой записи
			this->_pending.clear();
		}
		/**
		 * @brief Метод перемотки каталога к началу
		 *
		 */
		void HandleDir::rewind() noexcept {
			// Если каталог валиден
			if(this->valid())
				// Выполняем перемотку каталога к началу
				awh::dir::_wrewinddir(this->_handle);
		}
		/**
		 * @brief Метод сброса состояния обхода
		 *
		 * @note Закрываются и вложенные каталоги, и сам корень:
		 *       объект после сброса чист и готов служить иному адресу
		 *
		 */
		void HandleDir::reset() noexcept {
			/**
			 * Выполняем закрытие всех вложенных каталогов
			 */
			for(auto & place : this->_places){
				// Если вложенный каталог валиден
				if(place.handle != nullptr)
					// Закрываем вложенный каталог
					awh::dir::_wclosedir(place.handle);
			}
			// Выполняем очистку стопки мест обхода
			this->_places.clear();
			// Если каталог валиден
			if(this->valid()){
				// Закрываем каталог
				awh::dir::_wclosedir(this->_handle);
				// Сбрасываем объект дескриптора
				this->_handle = nullptr;
			}
			// Сбрасываем признак незавершённости обхода
			this->_active = false;
			// Выполняем очистку адреса каталога
			this->_address.clear();
			// Выполняем забвение незавершённой записи обхода
			this->done();
		}
		/**
		 * @brief Метод проверки пустоты стопки мест обхода
		 *
		 * @return признак того, что обход идёт по самому корню
		 *
		 */
		bool HandleDir::empty() const noexcept {
			// Возвращаем признак пустоты стопки мест обхода
			return this->_places.empty();
		}
		/**
		 * @brief Метод проверки валидности
		 *
		 * @return валидность каталога
		 *
		 */
		bool HandleDir::valid() const noexcept {
			// Возвращаем результат проверки валидности каталога
			return (this->_handle != nullptr);
		}
		/**
		 * @brief Метод проверки необходимости повторной выдачи записи
		 *
		 * @return признак того, что незавершённую запись надлежит отдать повторно
		 *
		 */
		bool HandleDir::repeat() const noexcept {
			// Возвращаем признак повторной выдачи записи
			return this->_repeat;
		}
		/**
		 * @brief Метод извлечения числа отданных прежде долей
		 *
		 * @return число долей незавершённой записи, отданных прежде
		 *
		 */
		size_t HandleDir::delivered() const noexcept {
			// Возвращаем число отданных прежде долей
			return this->_delivered;
		}
		/**
		 * @brief Метод извлечения вида незавершённой записи обхода
		 *
		 * @return вид незавершённой записи
		 *
		 */
		uint8_t HandleDir::pendingType() const noexcept {
			// Возвращаем вид незавершённой записи
			return this->_pendingType;
		}
		/**
		 * @brief Метод установки объекта дескриптора
		 *
		 * @param handle объект дескриптора
		 *
		 */
		void HandleDir::set(awh::dir::_WDIR * handle) noexcept {
			// Если установка ещё не выполнена
			if(!this->valid())
				// Выполняем установку
				this->_handle = handle;
		}
		/**
		 * @brief Метод проверки незавершённости обхода
		 *
		 * @return признак того, что обход начат и до конца не доведён
		 *
		 */
		bool HandleDir::active() const noexcept {
			// Возвращаем признак незавершённости обхода
			return this->_active;
		}
		/**
		 * @brief Метод установки признака незавершённости обхода
		 *
		 * @param active признак незавершённости обхода
		 *
		 */
		void HandleDir::active(const bool active) noexcept {
			// Выполняем установку признака незавершённости обхода
			this->_active = active;
		}
		/**
		 * @brief Метод извлечения адреса каталога, которому объект служит
		 *
		 * @return адрес каталога
		 *
		 */
		const string & HandleDir::address() const noexcept {
			// Возвращаем адрес каталога
			return this->_address;
		}
		/**
		 * @brief Метод установки адреса каталога, которому объект служит
		 *
		 * @param address адрес каталога
		 *
		 */
		void HandleDir::address(string_view address) noexcept {
			// Выполняем установку адреса каталога
			this->_address = address;
		}
		/**
		 * @brief Метод извлечения дескриптора верхнего места обхода
		 *
		 * @return объект дескриптора вложенного каталога
		 *
		 */
		awh::dir::_WDIR * HandleDir::top() const noexcept {
			// Возвращаем дескриптор верхнего места обхода
			return (!this->_places.empty() ? this->_places.back().handle : nullptr);
		}
		/**
		 * @brief Метод извлечения адреса верхнего места обхода
		 *
		 * @return адрес вложенного каталога
		 *
		 */
		const string & HandleDir::topAddress() const noexcept {
			// Возвращаем адрес верхнего места обхода
			return (!this->_places.empty() ? this->_places.back().address : this->_address);
		}
		/**
		 * @brief Метод снятия верхнего места обхода
		 *
		 */
		void HandleDir::pop() noexcept {
			// Если стопка мест обхода не пуста
			if(!this->_places.empty()){
				// Если вложенный каталог валиден
				if(this->_places.back().handle != nullptr)
					// Закрываем вложенный каталог
					awh::dir::_wclosedir(this->_places.back().handle);
				// Выполняем снятие верхнего места обхода
				this->_places.pop_back();
			}
		}
		/**
		 * @brief Метод добавления места обхода
		 *
		 * @param handle  объект дескриптора вложенного каталога
		 * @param address адрес вложенного каталога
		 *
		 */
		void HandleDir::push(awh::dir::_WDIR * handle, string_view address) noexcept {
			// Если вложенный каталог валиден
			if(handle != nullptr)
				// Выполняем добавление места обхода
				this->_places.emplace_back(handle, address);
		}
		/**
		 * @brief Метод извлечения адреса незавершённой записи обхода
		 *
		 * @return адрес незавершённой записи
		 *
		 */
		const string & HandleDir::pending() const noexcept {
			// Возвращаем адрес незавершённой записи
			return this->_pending;
		}
		/**
		 * @brief Метод запоминания незавершённой записи обхода
		 *
		 * @param address   адрес незавершённой записи
		 * @param type      вид незавершённой записи
		 * @param delivered число долей записи, отданных прежде
		 *
		 */
		void HandleDir::pending(string_view address, const uint8_t type, const size_t delivered) noexcept {
			// Отмечаем запись подлежащей повторной выдаче
			this->_repeat = true;
			// Запоминаем адрес незавершённой записи
			this->_pending = address;
			// Запоминаем вид незавершённой записи
			this->_pendingType = type;
			// Запоминаем число отданных прежде долей
			this->_delivered = delivered;
		}
		/**
		 * @brief Оператор приведения к типу
		 *
		 * @return объект дескриптора
		 *
		 */
		HandleDir::operator awh::dir::_WDIR * () const noexcept {
			// Возвращаем объект дескриптора
			return this->_handle;
		}
		/**
		 * @brief Конструктор
		 *
		 */
		HandleDir::HandleDir() noexcept :
		 _repeat(false), _active(false),
		 _delivered(0), _pendingType(0),
		 _pending{""}, _address{""},
		 _handle(nullptr) {}
		/**
		 * @brief Конструктор
		 *
		 * @param handle объект дескриптора
		 *
		 */
		HandleDir::HandleDir(awh::dir::_WDIR * handle) noexcept :
		 _repeat(false), _active(false),
		 _delivered(0), _pendingType(0),
		 _pending{""}, _address{""},
		 _handle(handle) {}
		/**
		 * @brief Деструктор
		 *
		 */
		HandleDir::~HandleDir() noexcept {
			// Выполняем сброс состояния обхода вместе с закрытием каталогов
			this->reset();
		}

		/**
		 * @brief Класс для автоматического управления файлом Windows (RAII)
		 *
		 */
		class HandleFile {
			private:
				// Объект дескриптора
				HANDLE _handle;
			public:
				/**
				 * @brief Метод проверки валидности
				 *
				 * @return валидность дескриптора
				 *
				 */
				bool valid() const noexcept;
			public:
				/**
				 * @brief Метод установки объекта дескриптора
				 *
				 * @param handle объект дескриптора
				 *
				 */
				void set(HANDLE handle) noexcept;
			public:
				/**
				 * @brief Оператор приведения к типу
				 *
				 * @return объект дескриптора
				 *
				 */
				operator HANDLE() const noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 */
				explicit HandleFile() noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param handle объект дескриптора
				 *
				 */
				explicit HandleFile(HANDLE handle) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~HandleFile() noexcept;
		};
		/**
		 * @brief Метод проверки валидности
		 *
		 * @return валидность дескриптора
		 *
		 */
		bool HandleFile::valid() const noexcept {
			// Возвращаем результат проверки валидности дескриптора
			return (
				(this->_handle != nullptr) &&
				(this->_handle != INVALID_HANDLE_VALUE)
			);
		}
		/**
		 * @brief Метод установки объекта дескриптора
		 *
		 * @param handle объект дескриптора
		 *
		 */
		void HandleFile::set(HANDLE handle) noexcept {
			// Если установка ещё не выполнена
			if(!this->valid())
				// Выполняем установку
				this->_handle = handle;
		}
		/**
		 * @brief Оператор приведения к типу
		 *
		 * @return объект дескриптора
		 *
		 */
		HandleFile::operator HANDLE() const noexcept {
			// Возвращаем объект дескриптора
			return this->_handle;
		}
		/**
		 * @brief Конструктор
		 *
		 */
		HandleFile::HandleFile() noexcept : _handle(INVALID_HANDLE_VALUE) {}
		/**
		 * @brief Конструктор
		 *
		 * @param handle объект дескриптора
		 *
		 */
		HandleFile::HandleFile(HANDLE handle) noexcept : _handle(handle) {}
		/**
		 * @brief Деструктор
		 *
		 */
		HandleFile::~HandleFile() noexcept {
			// Если дескриптор валиден
			if(this->valid())
				// Закрываем дескриптор
				::CloseHandle(this->_handle);
		}
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#else
		/**
		 * @brief Класс для автоматического управления каталогом POSIX (RAII)
		 *
		 */
		class HandleDir {
			private:
				/**
				 * @brief Место обхода каталога
				 *
				 */
				typedef struct Place {
					// Объект дескриптора вложенного каталога
					awh::dir::DIR * handle;
					// Адрес вложенного каталога
					string address;
					/**
					 * @brief Конструктор
					 *
					 * @param handle  объект дескриптора вложенного каталога
					 * @param address адрес вложенного каталога
					 *
					 */
					explicit Place(awh::dir::DIR * handle, string_view address) noexcept :
					 handle(handle), address{address} {}
				} place_t;
			private:
				// Признак того, что незавершённую запись надлежит отдать повторно
				bool _repeat;
				// Признак незавершённого обхода
				bool _active;
			private:
				// Число долей незавершённой записи, отданных прежде
				size_t _delivered;
			private:
				// Вид незавершённой записи обхода
				uint8_t _pendingType;
			private:
				/**
				 * @brief Незавершённая запись обхода
				 *
				 * @note Отклик у видов `walkdir`, отдающих СОДЕРЖИМОЕ файла, вправе остановить
				 *       обход посреди файла. Запись эта тогда прочитана не до конца, а место её
				 *       в каталоге уже пройдено, - и без памяти о ней остаток файла был бы утерян
				 *       молча. Здесь и хранится адрес такой записи, вид её и число уже отданных
				 *       долей: продолжение отдаёт её повторно, а доли, отданные прежде, пропускает
				 */
				string _pending;
				// Адрес каталога, которому объект служит
				string _address;
			private:
				// Объект дескриптора
				awh::dir::DIR * _handle;
			private:
				/**
				 * @brief Места обхода вложенных каталогов
				 *
				 * @note Корень обхода здесь НЕ лежит: он живёт в `_handle` и по исчерпании не
				 *       закрывается, а перематывается. Тем и окупается внешний объект каталога -
				 *       повторный обзор того же каталога идёт мимо повторного открытия
				 */
				vector <place_t> _places;
			public:
				/**
				 * @brief Метод забвения незавершённой записи обхода
				 *
				 */
				void done() noexcept;
				/**
				 * @brief Метод перемотки каталога к началу
				 *
				 */
				void rewind() noexcept;
				/**
				 * @brief Метод сброса состояния обхода
				 *
				 * @note Закрываются и вложенные каталоги, и сам корень: объект после сброса
				 *       чист и готов служить иному адресу
				 *
				 */
				void reset() noexcept;
			public:
				/**
				 * @brief Метод проверки пустоты стопки мест обхода
				 *
				 * @return признак того, что обход идёт по самому корню
				 *
				 */
				bool empty() const noexcept;
				/**
				 * @brief Метод проверки валидности
				 *
				 * @return валидность каталога
				 *
				 */
				bool valid() const noexcept;
				/**
				 * @brief Метод проверки необходимости повторной выдачи записи
				 *
				 * @return признак того, что незавершённую запись надлежит отдать повторно
				 *
				 */
				bool repeat() const noexcept;
			public:
				/**
				 * @brief Метод извлечения числа отданных прежде долей
				 *
				 * @return число долей незавершённой записи, отданных прежде
				 *
				 */
				size_t delivered() const noexcept;
				/**
				 * @brief Метод извлечения вида незавершённой записи обхода
				 *
				 * @return вид незавершённой записи
				 *
				 */
				uint8_t pendingType() const noexcept;
			public:
				/**
				 * @brief Метод установки объекта дескриптора
				 *
				 * @param handle объект дескриптора
				 *
				 */
				void set(awh::dir::DIR * handle) noexcept;
			public:
				/**
				 * @brief Метод проверки незавершённости обхода
				 *
				 * @return признак того, что обход начат и до конца не доведён
				 *
				 */
				bool active() const noexcept;
				/**
				 * @brief Метод установки признака незавершённости обхода
				 *
				 * @param active признак незавершённости обхода
				 *
				 */
				void active(const bool active) noexcept;
			public:
				/**
				 * @brief Метод извлечения адреса каталога, которому объект служит
				 *
				 * @return адрес каталога
				 *
				 */
				const string & address() const noexcept;
				/**
				 * @brief Метод установки адреса каталога, которому объект служит
				 *
				 * @param address адрес каталога
				 *
				 */
				void address(string_view address) noexcept;
			public:
				/**
				 * @brief Метод извлечения дескриптора верхнего места обхода
				 *
				 * @return объект дескриптора вложенного каталога
				 *
				 */
				awh::dir::DIR * top() const noexcept;
				/**
				 * @brief Метод извлечения адреса верхнего места обхода
				 *
				 * @return адрес вложенного каталога
				 *
				 */
				const string & topAddress() const noexcept;
			public:
				/**
				 * @brief Метод снятия верхнего места обхода
				 *
				 */
				void pop() noexcept;
				/**
				 * @brief Метод добавления места обхода
				 *
				 * @param handle  объект дескриптора вложенного каталога
				 * @param address адрес вложенного каталога
				 *
				 */
				void push(awh::dir::DIR * handle, string_view address) noexcept;
			public:
				/**
				 * @brief Метод извлечения адреса незавершённой записи обхода
				 *
				 * @return адрес незавершённой записи
				 *
				 */
				const string & pending() const noexcept;
				/**
				 * @brief Метод запоминания незавершённой записи обхода
				 *
				 * @param address   адрес незавершённой записи
				 * @param type      вид незавершённой записи
				 * @param delivered число долей записи, отданных прежде
				 *
				 */
				void pending(string_view address, const uint8_t type, const size_t delivered) noexcept;
			public:
				/**
				 * @brief Оператор приведения к типу
				 *
				 * @return объект дескриптора
				 *
				 */
				operator awh::dir::DIR * () const noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 */
				explicit HandleDir() noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param dir объект каталога
				 *
				 */
				explicit HandleDir(awh::dir::DIR * dir) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~HandleDir() noexcept;
		};
		/**
		 * @brief Метод забвения незавершённой записи обхода
		 *
		 */
		void HandleDir::done() noexcept {
			// Сбрасываем число отданных прежде долей
			this->_delivered = 0;
			// Сбрасываем вид незавершённой записи
			this->_pendingType = 0;
			// Снимаем признак повторной выдачи записи
			this->_repeat = false;
			// Выполняем очистку адреса незавершённой записи
			this->_pending.clear();
		}
		/**
		 * @brief Метод перемотки каталога к началу
		 *
		 */
		void HandleDir::rewind() noexcept {
			// Если каталог валиден
			if(this->valid())
				// Выполняем перемотку каталога к началу
				awh::dir::rewinddir(this->_handle);
		}
		/**
		 * @brief Метод сброса состояния обхода
		 *
		 * @note Закрываются и вложенные каталоги, и сам корень:
		 *       объект после сброса чист и готов служить иному адресу
		 *
		 */
		void HandleDir::reset() noexcept {
			/**
			 * Выполняем закрытие всех вложенных каталогов
			 */
			for(auto & place : this->_places){
				// Если вложенный каталог валиден
				if(place.handle != nullptr)
					// Закрываем вложенный каталог
					awh::dir::closedir(place.handle);
			}
			// Выполняем очистку стопки мест обхода
			this->_places.clear();
			// Если каталог валиден
			if(this->valid()){
				// Закрываем каталог
				awh::dir::closedir(this->_handle);
				// Сбрасываем объект дескриптора
				this->_handle = nullptr;
			}
			// Сбрасываем признак незавершённости обхода
			this->_active = false;
			// Выполняем очистку адреса каталога
			this->_address.clear();
			// Выполняем забвение незавершённой записи обхода
			this->done();
		}
		/**
		 * @brief Метод проверки пустоты стопки мест обхода
		 *
		 * @return признак того, что обход идёт по самому корню
		 *
		 */
		bool HandleDir::empty() const noexcept {
			// Возвращаем признак пустоты стопки мест обхода
			return this->_places.empty();
		}
		/**
		 * @brief Метод проверки валидности
		 *
		 * @return валидность каталога
		 *
		 */
		bool HandleDir::valid() const noexcept {
			// Возвращаем результат проверки валидности каталога
			return (this->_handle != nullptr);
		}
		/**
		 * @brief Метод проверки необходимости повторной выдачи записи
		 *
		 * @return признак того, что незавершённую запись надлежит отдать повторно
		 *
		 */
		bool HandleDir::repeat() const noexcept {
			// Возвращаем признак повторной выдачи записи
			return this->_repeat;
		}
		/**
		 * @brief Метод извлечения числа отданных прежде долей
		 *
		 * @return число долей незавершённой записи, отданных прежде
		 *
		 */
		size_t HandleDir::delivered() const noexcept {
			// Возвращаем число отданных прежде долей
			return this->_delivered;
		}
		/**
		 * @brief Метод извлечения вида незавершённой записи обхода
		 *
		 * @return вид незавершённой записи
		 *
		 */
		uint8_t HandleDir::pendingType() const noexcept {
			// Возвращаем вид незавершённой записи
			return this->_pendingType;
		}
		/**
		 * @brief Метод установки объекта дескриптора
		 *
		 * @param handle объект дескриптора
		 *
		 */
		void HandleDir::set(awh::dir::DIR * handle) noexcept {
			// Если установка ещё не выполнена
			if(!this->valid())
				// Выполняем установку
				this->_handle = handle;
		}
		/**
		 * @brief Метод проверки незавершённости обхода
		 *
		 * @return признак того, что обход начат и до конца не доведён
		 *
		 */
		bool HandleDir::active() const noexcept {
			// Возвращаем признак незавершённости обхода
			return this->_active;
		}
		/**
		 * @brief Метод установки признака незавершённости обхода
		 *
		 * @param active признак незавершённости обхода
		 *
		 */
		void HandleDir::active(const bool active) noexcept {
			// Выполняем установку признака незавершённости обхода
			this->_active = active;
		}
		/**
		 * @brief Метод извлечения адреса каталога, которому объект служит
		 *
		 * @return адрес каталога
		 *
		 */
		const string & HandleDir::address() const noexcept {
			// Возвращаем адрес каталога
			return this->_address;
		}
		/**
		 * @brief Метод установки адреса каталога, которому объект служит
		 *
		 * @param address адрес каталога
		 *
		 */
		void HandleDir::address(string_view address) noexcept {
			// Выполняем установку адреса каталога
			this->_address = address;
		}
		/**
		 * @brief Метод извлечения дескриптора верхнего места обхода
		 *
		 * @return объект дескриптора вложенного каталога
		 *
		 */
		awh::dir::DIR * HandleDir::top() const noexcept {
			// Возвращаем дескриптор верхнего места обхода
			return (!this->_places.empty() ? this->_places.back().handle : nullptr);
		}
		/**
		 * @brief Метод извлечения адреса верхнего места обхода
		 *
		 * @return адрес вложенного каталога
		 *
		 */
		const string & HandleDir::topAddress() const noexcept {
			// Возвращаем адрес верхнего места обхода
			return (!this->_places.empty() ? this->_places.back().address : this->_address);
		}
		/**
		 * @brief Метод снятия верхнего места обхода
		 *
		 */
		void HandleDir::pop() noexcept {
			// Если стопка мест обхода не пуста
			if(!this->_places.empty()){
				// Если вложенный каталог валиден
				if(this->_places.back().handle != nullptr)
					// Закрываем вложенный каталог
					awh::dir::closedir(this->_places.back().handle);
				// Выполняем снятие верхнего места обхода
				this->_places.pop_back();
			}
		}
		/**
		 * @brief Метод добавления места обхода
		 *
		 * @param handle  объект дескриптора вложенного каталога
		 * @param address адрес вложенного каталога
		 *
		 */
		void HandleDir::push(awh::dir::DIR * handle, string_view address) noexcept {
			// Если вложенный каталог валиден
			if(handle != nullptr)
				// Выполняем добавление места обхода
				this->_places.emplace_back(handle, address);
		}
		/**
		 * @brief Метод извлечения адреса незавершённой записи обхода
		 *
		 * @return адрес незавершённой записи
		 *
		 */
		const string & HandleDir::pending() const noexcept {
			// Возвращаем адрес незавершённой записи
			return this->_pending;
		}
		/**
		 * @brief Метод запоминания незавершённой записи обхода
		 *
		 * @param address   адрес незавершённой записи
		 * @param type      вид незавершённой записи
		 * @param delivered число долей записи, отданных прежде
		 *
		 */
		void HandleDir::pending(string_view address, const uint8_t type, const size_t delivered) noexcept {
			// Отмечаем запись подлежащей повторной выдаче
			this->_repeat = true;
			// Запоминаем адрес незавершённой записи
			this->_pending = address;
			// Запоминаем вид незавершённой записи
			this->_pendingType = type;
			// Запоминаем число отданных прежде долей
			this->_delivered = delivered;
		}
		/**
		 * @brief Оператор приведения к типу
		 *
		 * @return объект дескриптора
		 *
		 */
		HandleDir::operator awh::dir::DIR * () const noexcept {
			// Возвращаем объект дескриптора
			return this->_handle;
		}
		/**
		 * @brief Конструктор
		 *
		 */
		HandleDir::HandleDir() noexcept :
		 _repeat(false), _active(false),
		 _delivered(0), _pendingType(0),
		 _pending{""}, _address{""},
		 _handle(nullptr) {}
		/**
		 * @brief Конструктор
		 *
		 * @param handle объект дескриптора
		 *
		 */
		HandleDir::HandleDir(awh::dir::DIR * handle) noexcept :
		 _repeat(false), _active(false),
		 _delivered(0), _pendingType(0),
		 _pending{""}, _address{""},
		 _handle(handle) {}
		/**
		 * @brief Деструктор
		 *
		 */
		HandleDir::~HandleDir() noexcept {
			// Выполняем сброс состояния обхода вместе с закрытием каталогов
			this->reset();
		}

		/**
		 * @brief Класс для автоматического управления файлом POSIX (RAII)
		 *
		 */
		class HandleFile {
			private:
				// Файловый дескриптор
				int32_t _fd;
			public:
				/**
				 * @brief Метод проверки валидности
				 *
				 * @return валидность файлового дескриптора
				 *
				 */
				bool valid() const noexcept;
			public:
				/**
				 * @brief Метод установки файлового дескриптора
				 *
				 * @param fd файловый дескриптор
				 *
				 */
				void set(const int32_t fd) noexcept;
			public:
				/**
				 * @brief Оператор приведения к типу
				 *
				 * @return файловый дескриптор
				 *
				 */
				operator int32_t () const noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 */
				explicit HandleFile() noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param fd файловый дескриптор
				 *
				 */
				explicit HandleFile(const int32_t fd) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~HandleFile() noexcept;
		};
		/**
		 * @brief Метод проверки валидности
		 *
		 * @return валидность файлового дескриптора
		 *
		 */
		bool HandleFile::valid() const noexcept {
			// Возвращаем результат проверки валидности файлового дескриптора
			return (this->_fd > -1);
		}
		/**
		 * @brief Метод установки файлового дескриптора
		 *
		 * @param fd файловый дескриптор
		 *
		 */
		void HandleFile::set(const int32_t fd) noexcept {
			// Если установка ещё не выполнена
			if(!this->valid())
				// Выполняем установку
				this->_fd = fd;
		}
		/**
		 * @brief Оператор приведения к типу
		 *
		 * @return файловый дескриптор
		 *
		 */
		HandleFile::operator int32_t () const noexcept {
			// Возвращаем файловый дескриптор
			return this->_fd;
		}
		/**
		 * @brief Конструктор
		 *
		 */
		HandleFile::HandleFile() noexcept : _fd(-1) {}
		/**
		 * @brief Конструктор
		 *
		 * @param fd файловый дескриптор
		 *
		 */
		HandleFile::HandleFile(const int32_t fd) noexcept : _fd(fd) {}
		/**
		 * @brief Деструктор
		 *
		 */
		HandleFile::~HandleFile() noexcept {
			// Если файловый дескриптор валиден
			if(this->valid())
				// Закрываем файловый дескриптор
				::close(this->_fd);
		}
	#endif

	/**
	 * @brief Оператор сноса объекта каталога
	 *
	 * @note Тело живёт здесь, а не в заголовке, намеренно: сноситель по умолчанию требует
	 *       полного вида в том месте, где объект гибнет, - то есть у потребителя. Здесь вид
	 *       полон, а наружу выходит лишь объявление, и системное API платформы остаётся скрыто
	 *
	 * @param handle объект каталога для сноса
	 *
	 */
	void HandleDirDeleter::operator () (HandleDir * handle) const noexcept {
		// Выполняем снос объекта каталога
		delete handle;
	}
	/**
	 * @brief Оператор сноса объекта файла
	 *
	 * @note Тело живёт здесь, а не в заголовке, намеренно: сноситель по умолчанию требует
	 *       полного вида в том месте, где объект гибнет, - то есть у потребителя. Здесь вид
	 *       полон, а наружу выходит лишь объявление, и системное API платформы остаётся скрыто
	 *
	 * @param handle объект файла для сноса
	 *
	 */
	void HandleFileDeleter::operator () (HandleFile * handle) const noexcept {
		// Выполняем снос объекта файла
		delete handle;
	}
};

/**
 * @brief Инкапсулируем статические типы данных в пространство имён
 *
 */
namespace {
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Функция получения размера страницы памяти
	 *
	 * @details Заводится своя затем, что getpagesize принадлежит POSIX и у MS Windows
	 *          отсутствует. Там же величина эта берётся из сведений о системе полем
	 *          dwPageSize, а сами сведения запрашиваются единожды - меняться при работе
	 *          они не могут.
	 *
	 * @return размер страницы памяти в байтах
	 *
	 */
	static size_t __awh_pagesize__() noexcept {
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Сведения о системе, запрашиваемые единожды
			static SYSTEM_INFO info = [](){
				// Создаём объект сведений о системе
				SYSTEM_INFO result;
				// Выполняем получение сведений о системе
				::GetSystemInfo(&result);
				// Возвращаем полученные сведения
				return result;
			}();
			// Возвращаем размер страницы памяти
			return static_cast <size_t> (info.dwPageSize);
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#else
			// Возвращаем размер страницы памяти
			return static_cast <size_t> (::getpagesize());
		#endif
	}

	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		/**
		 * @brief Шаблон класса для автоматического управления COM-интерфейсами (RAII)
		 *
		 * @tparam T тип интерфейса
		 *
		 */
		template <typename T>
		/**
		 * @brief Класс для автоматического управления COM-интерфейсами (RAII)
		 *
		 */
		class ComGuard {
			private:
				// Указатель на COM-интерфейс
				T * _ptr;
			public:
				/**
				 * @brief Метод получения указателя на интерфейс
				 *
				 * @return указатель на интерфейс
				 *
				 */
				T * get() const noexcept {
					// Возвращаем указатель на интерфейс
					return this->_ptr;
				}
			public:
				/**
				 * @brief Метод сброса указателя на интерфейс
				 *
				 * @param ptr новый указатель на интерфейс
				 *
				 */
				void reset(T * ptr = nullptr) noexcept {
					// Если указатель на интерфейс установлен
					if(this->_ptr != nullptr)
						// Выполняем освобождение интерфейса
						this->_ptr->Release();
					// Устанавливаем новый указатель на интерфейс
					this->_ptr = ptr;
				}
			public:
				/**
				 * @brief Опретор получение адреса указателя (для CoCreateInstance и т.п.)
				 *
				 * @return адрес указателя
				 *
				 */
				T ** operator & () noexcept {
					// Возвращаем адрес указателя
					return &this->_ptr;
				}
				/**
				 * @brief Оператор доступа к членам интерфейса
				 *
				 * @return указатель на интерфейс
				 *
				 */
				T * operator -> () const noexcept {
					// Возвращаем указатель на интерфейс
					return this->_ptr;
				}
			public:
				/**
				 * @brief Оператор приведения к типу bool
				 *
				 * @return валидность указателя
				 *
				 */
				explicit operator bool() const noexcept {
					// Возвращаем результат проверки валидности указателя
					return (this->_ptr != nullptr);
				}
			public:
				/**
				 * @brief Конструктор
				 *
				 */
				ComGuard() noexcept : _ptr(nullptr) {}
				/**
				 * @brief Конструктор
				 *
				 * @param ptr указатель на COM-интерфейс
				 *
				 */
				ComGuard(T * ptr) noexcept : _ptr(ptr) {}
				/**
				 * @brief Деструктор
				 *
				 */
				~ComGuard() noexcept {
					// Выполняем сброс указателя
					this->reset();
				}
		};
		/**
		 * @brief Псевдоним класса управления COM-интерфейсами
		 *
		 * @details Заводится псевдонимом шаблона, а не через typedef: объявить шаблон
		 *          через typedef язык не позволяет вовсе, и прежняя запись
		 *          "template <typename T> typedef class ComGuard { ... } com_guard_t;"
		 *          сборкой не разбиралась. Обнаружилось это первой сборкой под MinGW64 —
		 *          прежде блок этот компилятором не читался ни разу.
		 *
		 * @tparam T тип интерфейса
		 *
		 */
		template <typename T>
		/**
		 * @brief Создаём тип данных COM-интефрейса
		 *
		 */
		using com_guard_t = ComGuard <T>;
	/**
	 * Помощник этот заведён для систем POSIX и только для них
	 *
	 * @warning Ограждения у определения НЕ БЫЛО, тогда как единственный его зов лежит
	 *          в ветви `#else` от `#if _WIN32 || _WIN64` (обращение `fullpath` класса).
	 *          Под MS Windows помощник оттого транслировался и не звался ни разу, а
	 *          собиратель показывал `unused function` на строке ОПРЕДЕЛЕНИЯ - то есть
	 *          на месте, где стало видно, а не на месте, где чего-то не хватает
	 *
	 * @note Мёртвым кодом он не является и переносу не подлежит: полный путь - предмет
	 *       модуля файловой системы, и здесь ему и место. Не хватало лишь ограждения,
	 *       отвечающего ограждению его зова
	 *
	 */
	#else
		/**
		 * @brief Функция получения полного пути файла или каталога
		 *
		 * @param input входная строка пути
		 * @return      полная строка пути
		 *
		 */
		static string fullpath(const string_view input) noexcept {
			/**
			 * Выполняем перехват ошибок
			 */
			try {
				// Если входная строка пуста, возвращаем текущий рабочий каталог
				if(input.empty()){
					// Временный буфер для текущего рабочего каталога
					char cwd[PATH_MAX];
					// Получаем текущий рабочий каталог
					return (::getcwd(cwd, sizeof(cwd)) ? cwd : AWH_FS_SEPARATOR);
				}
				// 1. Обрабатываем вход: превращаем в абсолютный путь как строку
				string path = "";
				// Если путь уже абсолютный
				if(input.front() == AWH_FS_SEPARATOR[0])
					// Уже абсолютный
					path = input;
				// Если путь начинается с '~'
				else if(input.front() == '~') {
					// Выполняем поиск следующего слэша ~/ или ~
					const size_t pos = input.find(AWH_FS_SEPARATOR[0], 1);
					// Получаем домашний каталог пользователя
					const char * home = ::getenv("HOME");
					// Если переменная окружения не установлена
					if(home == nullptr){
						/**
						 * Для операционной системы MS Windows
						 *
						 * @note Учётных записей в смысле POSIX там нет, как нет и getpwuid
						 *       с getuid. Домашний каталог задаётся переменной окружения
						 *       USERPROFILE, а при её отсутствии складывается из HOMEDRIVE
						 *       и HOMEPATH - так поступают и сами средства MS Windows
						 *
						 */
						#if _WIN32 || _WIN64
							// Буфер для сборки домашнего каталога из двух переменных окружения
							static string profile = "";
							// Получаем домашний каталог пользователя
							home = ::getenv("USERPROFILE");
							// Если переменная окружения не установлена
							if(home == nullptr){
								// Получаем букву диска домашнего каталога
								const char * drive = ::getenv("HOMEDRIVE");
								// Получаем путь домашнего каталога
								const char * tail = ::getenv("HOMEPATH");
								// Если обе переменные окружения установлены
								if((drive != nullptr) && (tail != nullptr)){
									// Выполняем сборку домашнего каталога
									profile.assign(drive).append(tail);
									// Устанавливаем домашний каталог пользователя
									home = profile.c_str();
								}
							}
						/**
						 * Для операционной системы не являющейся MS Windows
						 */
						#else
							// Получаем информацию о пользователе из системы
							struct passwd * pw = ::getpwuid(::getuid());
							// Если информация о пользователе получена
							if(pw != nullptr)
								// Устанавливаем домашний каталог пользователя
								home = pw->pw_dir;
						#endif
					}
					// Добавляем домашний каталог пользователя в путь
					path.append(home != nullptr ? home : AWH_FS_SEPARATOR);
					// Если найден слэш после тильды
					if(pos != string::npos)
						// Добавляем оставшуюся часть пути
						path.append(input.substr(pos));
				// Если путь относительный
				} else {
					// Временный буфер для текущего рабочего каталога
					char cwd[PATH_MAX];
					// Получаем текущий рабочий каталог
					if(!::getcwd(cwd, sizeof(cwd))){
						// Устанавливаем корневой каталог как текущий
						cwd[0] = '/';
						// Завершаем строку
						cwd[1] = '\0';
					}
					// Добавляем текущий рабочий каталог в путь
					path.append(cwd);
					// Добавляем слэш разделителя файловой системы
					path.append(AWH_FS_SEPARATOR);
					// Добавляем оставшуюся часть пути
					path.append(input);
				}
				// Компонент пути
				string part = "";
				// Составные части пути
				vector <string> parts;
				/**
				 * 2. Теперь у нас есть строка, начинающаяся с '/', — нормализуем её
				 */
				for(char letter : path){
					// Если встретили слэш разделителя файловой системы
					if(letter == AWH_FS_SEPARATOR[0]){
						// Если компонент не пустой
						if(!part.empty()){
							// Обрабатываем компонент пути
							if(part == ".."){
								// Если есть из чего удалять, удаляем последний компонент
								if(!parts.empty())
									// Удаляем последний компонент пути
									parts.pop_back();
							// Если компонент не текущий каталог
							} else if(part != ".")
								// Добавляем компонент в составные части пути
								parts.push_back(part);
							// Очищаем компонент пути
							part.clear();
						}
					// Иначе накапливаем символ в компонент пути
					} else part.append(1, letter);
				}
				// Последний компонент
				if(!part.empty()){
					// Обрабатываем компонент пути
					if(part == ".."){
						// Если есть из чего удалять, удаляем последний компонент
						if(!parts.empty())
							// Удаляем последний компонент пути
							parts.pop_back();
					// Если компонент не текущий каталог
					} else if(part != ".")
						// Добавляем компонент в составные части пути
						parts.push_back(part);
				}
				// 3. Собираем результат — строго один слэш в начале
				string result = AWH_FS_SEPARATOR;
				/**
				 * Проходим по всем частям пути
				 */
				for(size_t i = 0; i < parts.size(); ++i){
					// Если это не первая часть пути
					if(i > 0)
						// Добавляем слэш разделителя файловой системы
						result.append(AWH_FS_SEPARATOR);
					// Добавляем часть пути
					result.append(parts[i]);
				}
				// 4. Возвращаем результат
				return result;
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					log::debug("%s", __PRETTY_FUNCTION__, {input}, log::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					log::print("%s", log::flag_t::CRITICAL, error.what());
				#endif
			}
			// Возвращаем результат
			return AWH_FS_SEPARATOR;
		}
	#endif
};

/**
 * @brief Метод создания символьной ссылки
 *
 * @param first  адрес на который нужно сделать ссылку
 * @param second адрес где должна быть создана ссылка
 *
 */
void awh::Filesystem::symlink(string_view first, string_view second) const noexcept {
	// Если адреса переданы
	if(!first.empty() && !second.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#if !_WIN32 && !_WIN64
				// Выполняем создание символьной ссылки
				::symlink(this->fullpath(first, true).c_str(), this->fullpath(second, true).c_str());
			/**
			 * Для операционной системы MS Windows
			 */
			#else
				// Получаем полный адрес пути
				const string & filename = this->fullpath(first, true);
				// Если файл передан
				if(!filename.empty()){
					// Выполняем инициализацию результата
					HRESULT hres = ::CoInitialize(nullptr);
					// Создаём объект проверки наличия ярлыка
					com_guard_t <IShellLinkW> psl;
					// Выполняем инициализацию объекта для проверки ярлыков
					hres = ::CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW, reinterpret_cast <LPVOID *> (&psl));
					// Если инициализация выполнена
					if(SUCCEEDED(hres) && psl){
						// Позиция разделителя каталога
						size_t pos = 0;
						// Создаём объект проверки файла
						com_guard_t <IPersistFile> ppf;
						// Выполняем инициализацию объекта для проверки файла
						hres = psl->QueryInterface(IID_IPersistFile, reinterpret_cast <void **> (&ppf));
						// Если объект для проверки файла инициализирован
						if(SUCCEEDED(hres) && ppf){
							// Определяем флаг обратного смещения
							const uint8_t offset = (filename.back() == '\\' ? 2 : 1);
							// Выполняем поиск разделителя каталога
							if((pos = filename.rfind("\\", filename.length() - static_cast <size_t> (offset))) != string::npos){
								// Создаём адрес ярлыка
								string symlink = "";
								// Описание создаваемого ярлыка
								string description = "";
								// Получаем адрес каталога где хранится файл
								const string & working = filename.substr(0, pos + 1);
								// Извлекаем имя файла
								string name = filename.substr(pos + 1, filename.length() - (pos + static_cast <size_t> (offset)));
								// Ищем расширение файла
								if((pos = name.find('.')) != string::npos)
									// Устанавливаем имя файла
									description = name.substr(0, pos);
								// Устанавливаем только имя файла
								else description = ::move(name);
								// Выполняем установку адреса ярлыка как он есть
								psl->SetPath(fmk::convert(filename).c_str());
								// Если рабочий каталог найден
								if(!working.empty())
									// Выполняем установку рабочего каталога
									psl->SetWorkingDirectory(fmk::convert(working).c_str());
								// Если название файла получено
								if(!description.empty())
									// Выполняем установку описания ярлыка
									psl->SetDescription(fmk::convert(description).c_str());
								// Если расширение ярлыка уже установлено
								if((second.size() > 4) && fmk::compare(".lnk", second.substr(second.size() - 4)))
									// Выполняем установку адреса ярлыка как он есть
									symlink = this->fullpath(second, true);
								// Выполняем установку полного пути адреса файла
								else symlink = fmk::format("%s.lnk", this->fullpath(second, true).c_str());
								// Выполняем создание ярлыка в файловой системе
								hres = ppf->Save(fmk::convert(symlink).c_str(), TRUE);
							}
						}
					}
					// Выполняем очистку объекта результата
					::CoUninitialize();
				}
			#endif
		/**
		 * Если возникает ошибка
		 */
		} catch(const ios_base::failure & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {first, second}, log::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, error.what());
			#endif
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {first, second}, log::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод создания жёстких ссылок
 *
 * @param first  адрес на который нужно сделать ссылку
 * @param second адрес где должна быть создана ссылка
 *
 */
void awh::Filesystem::hardlink(string_view first, string_view second) const noexcept {
	// Если адреса переданы
	if(!first.empty() && !second.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#if !_WIN32 && !_WIN64
				// Если адрес на который нужно создать ссылку существует
				if(this->type(first) != type_t::NONE)
					// Выполняем создание символьной ссылки
					::link(this->fullpath(first, true).c_str(), this->fullpath(second, true).c_str());
			/**
			 * Для операционной системы MS Windows
			 */
			/**
			 * Для операционной системы MS Windows
			 *
			 * @details Жёсткая ссылка у MS Windows своя и настоящая: CreateHardLinkW
			 *          заводит на файловой системе NTFS вторую запись каталога, ведущую
			 *          к тем же данным, - ровно то же, что делает link у POSIX. Особых
			 *          полномочий она не требует, в отличие от ссылки символьной.
			 *
			 * @note Ярлык оболочки остаётся здесь запасным ходом, и лишь им: жёсткая
			 *       ссылка невозможна поверх FAT и exFAT, а равно между разными томами -
			 *       обе записи обязаны лежать на одном. Там, где система отвечает
			 *       отказом, заводится ярлык - тем сохраняется прежнее поведение вызова
			 *
			 */
			#else
				// Если адрес на который нужно создать ссылку существует
				if(this->type(first) != type_t::NONE){
					// Получаем полный адрес файла, на который ведёт ссылка
					const wstring & target = fmk::convert(this->fullpath(first, true));
					// Получаем полный адрес создаваемой ссылки
					const wstring & filename = fmk::convert(this->fullpath(second, true));
					// Выполняем создание жёсткой ссылки средствами системы
					if(!::CreateHardLinkW(filename.c_str(), target.c_str(), nullptr))
						// Если система жёсткую ссылку завести не смогла - заводим ярлык
						this->symlink(first, second);
				}
			#endif
		/**
		 * Если возникает ошибка
		 */
		} catch(const ios_base::failure & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {first, second}, log::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, error.what());
			#endif
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {first, second}, log::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод удаления адреса файловой системы
 *
 * @param addr    полный адрес для удаления
 * @param resolve флаг резолвинга символьных ссылок
 * @return        результат удаления
 *
 */
bool awh::Filesystem::unlink(string_view addr, const bool resolve) const noexcept {
	// Переменная результата
	bool result = false;
	// Если адрес передан
	if(!addr.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем извлечение актуального значения адреса
			const string & address = this->fullpath(addr, resolve);
			// Если адрес получен правильный
			if(!address.empty()){
				/**
				 * Определяем тип пути
				 */
				switch(static_cast <uint8_t> (this->type(address))){
					// Если переданный путь является каталогом
					case static_cast <uint8_t> (type_t::DIR): {
						/**
						 * Для операционной системы MS Windows
						 */
						#if _WIN32 || _WIN64
							// Открываем указанный каталог
							HandleDir dir(awh::dir::_wopendir(fmk::convert(address).c_str()));
						/**
						 * Для операционной системы не являющейся MS Windows
						 */
						#else
							// Открываем указанный каталог
							HandleDir dir(awh::dir::opendir(address.c_str()));
						#endif
						// Если каталог открыт
						if((result = dir.valid())){
							/**
							 * Для операционной системы MS Windows
							 */
							#if _WIN32 || _WIN64
								// Структура проверка статистики
								struct _stat info{};
								// Создаем указатель на содержимое каталога
								awh::dir::_wdirent * ptr = nullptr;
								/**
								 * Выполняем чтение содержимого каталога
								 */
								while((ptr = awh::dir::_wreaddir(dir))){
							/**
							 * Для операционной системы не являющейся MS Windows
							 */
							#else
								// Структура проверка статистики
								struct stat info{};
								// Создаем указатель на содержимое каталога
								awh::dir::dirent * ptr = nullptr;
								/**
								 * Выполняем чтение содержимого каталога
								 */
								while((ptr = awh::dir::readdir(dir))){
							#endif
									/**
									 * Для операционной системы MS Windows
									 */
									#if _WIN32 || _WIN64
										// Пропускаем названия текущие "." и внешние "..", так как идет рекурсия
										if(!::wcscmp(ptr->d_name, L".") || !::wcscmp(ptr->d_name, L".."))
											// Выполняем пропуск каталога
											continue;
										// Получаем полный путь дочернего элемента из разрешённого адреса каталога
										const string & child = fmk::format("%s%s%s", address.c_str(), AWH_FS_SEPARATOR, fmk::convert(ptr->d_name).c_str());
									/**
									 * Для операционной системы не являющейся MS Windows
									 */
									#else
										// Пропускаем названия текущие "." и внешние "..", так как идет рекурсия
										if(!::strcmp(ptr->d_name, ".") || !::strcmp(ptr->d_name, ".."))
											// Выполняем пропуск каталога
											continue;
										// Получаем полный путь дочернего элемента из разрешённого адреса каталога
										const string & child = fmk::format("%s%s%s", address.c_str(), AWH_FS_SEPARATOR, ptr->d_name);
									#endif
									/**
									 * Для операционной системы MS Windows
									 */
									#if _WIN32 || _WIN64
										// Конвертируем адрес в формат wstring
										const wstring & path = fmk::convert(child);
										// Если статистика извлечена
										if(!::_wstat(path.c_str(), &info)){
											// Если дочерний элемент является директорией
											if(S_ISDIR(info.st_mode))
												// Выполняем удаление подкаталогов
												result = this->unlink(child, resolve);
											// Если дочерний элемент является файлом то удаляем его
											else result = (::_wunlink(path.c_str()) == 0);
										// Если путь является символьной ссылкой
										} else if(this->type(child) == type_t::LINK)
											// Выполняем удаление символьной ссылки
											result = (::_wunlink(path.c_str()) == 0);
									/**
									 * Для операционной системы не являющейся MS Windows
									 */
									#else
										// Если статистика извлечена
										if(!::stat(child.c_str(), &info)){
											// Если дочерний элемент является директорией
											if(S_ISDIR(info.st_mode))
												// Выполняем удаление подкаталогов
												result = this->unlink(child, resolve);
											// Если дочерний элемент является файлом то удаляем его
											else result = (::unlink(child.c_str()) == 0);
										// Если путь является символьной ссылкой
										} else if(this->type(child) == type_t::LINK)
											// Выполняем удаление символьной ссылки
											result = (::unlink(child.c_str()) == 0);
									#endif
									// Если удаление не выполнено
									if(!result)
										// Прекращаем выполнение удаления
										break;
							}
						}
						// Удаляем последний каталог
						if(result){
							/**
							 * Для операционной системы MS Windows
							 */
							#if _WIN32 || _WIN64
								// Получаем количество дочерних элементов
								result = (::_wrmdir(fmk::convert(address).c_str()) == 0);
							/**
							 * Для операционной системы не являющейся MS Windows
							 */
							#else
								// Получаем количество дочерних элементов
								result = (::rmdir(address.c_str()) == 0);
							#endif
						}
					} break;
					// Если переданный путь является файлом
					case static_cast <uint8_t> (type_t::FILE):
					// Если переданный путь является ссылкой
					case static_cast <uint8_t> (type_t::LINK): {
						/**
						 * Для операционной системы MS Windows
						 */
						#if _WIN32 || _WIN64
							// Выполняем удаление переданного пути
							result = (::_wunlink(fmk::convert(address).c_str()) == 0);
						/**
						 * Для операционной системы не являющейся MS Windows
						 */
						#else
							// Выполняем удаление переданного пути
							result = (::unlink(address.c_str()) == 0);
						#endif
					} break;
				}
				// Если путь является символьной ссылкой
				if(resolve && (this->type(addr) == type_t::LINK)){
					/**
					 * Для операционной системы MS Windows
					 */
					#if _WIN32 || _WIN64
						// Выполняем извлечение актуального значения адреса
						const string & address = this->fullpath(addr);
						// Если адрес получен правильный
						if(!address.empty())
							// Выполняем удаление переданного пути
							result = (::_wunlink(fmk::convert(address).c_str()) == 0);
					/**
					 * Для операционной системы не являющейся MS Windows
					 */
					#else
						// Выполняем удаление переданного пути
						result = (::unlink(addr.data()) == 0);
					#endif
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const ios_base::failure & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {addr}, log::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, error.what());
			#endif
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {addr}, log::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод, определяющий тип файловой системы по адресу
 *
 * @param addr        адрес директории или файла
 * @param detectLinks флаг детектирования символьных ссылок (на горячих путях можно отключить)
 * @return            тип файловой системы
 *
 */
awh::Filesystem::type_t awh::Filesystem::type(string_view addr, const bool detectLinks) const noexcept {
	// Переменная результата
	type_t result = type_t::NONE;
	// Если адрес директории или файла передан
	if(!addr.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * Для операционной системы MS Windows
			 */
			#if _WIN32 || _WIN64
				// Структура проверка статистики
				struct _stat info{};
				// Выполняем извлечение актуального значения адреса
				const wstring & address = fmk::convert(this->fullpath(addr));
				// Выполняем извлечение данных статистики
				const int32_t status = (!address.empty() ? ::_wstat(address.c_str(), &info) : -1);
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#else
				// Структура проверка статистики
				struct stat info{};
				// Выполняем извлечение данных статистики
				const int32_t status = ::stat(addr.data(), &info);
			#endif
			// Если тип определён
			if(status == 0){
				// Если это каталог
				if(S_ISDIR(info.st_mode))
					// Получаем тип файловой системы
					result = type_t::DIR;
				// Если это устройство
				else if(S_ISCHR(info.st_mode))
					// Получаем тип файловой системы
					result = type_t::CHR;
				// Если это блок устройства
				else if(S_ISBLK(info.st_mode))
					// Получаем тип файловой системы
					result = type_t::BLK;
				// Если это файл
				else if(S_ISREG(info.st_mode))
					// Получаем тип файловой системы
					result = type_t::FILE;
				// Если это устройство ввода-вывода
				else if(S_ISFIFO(info.st_mode))
					// Получаем тип файловой системы
					result = type_t::FIFO;
				/**
				 * Для операционной системы не являющейся MS Windows
				 */
				#if !_WIN32 && !_WIN64
					// Если это сокет
					else if(S_ISSOCK(info.st_mode))
						// Получаем тип файловой системы
						result = type_t::SOCK;
				/**
				 * Для операционной системы MS Windows
				 */
				#else
					// Создаём объект работы с файлом
					/**
					 * @note Дозволяется и запись, и удаление, а не одно лишь чтение: файл вправе
					 *       держать открытым кто-то ещё - движок наблюдения за файловой системой
					 *       держит его именно так, - и обращение с одним лишь дозволением чтения
					 *       отвечало бы отказом ERROR_SHARING_VIOLATION. Отказ этот молчаливый:
					 *       дозапись уходила бы мимо файла, а размер выдавался бы нулевым
					 */
					HANDLE file = ::CreateFileW(address.c_str(), GENERIC_READ, (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
					// Если файл открыт нормально
					if(file != INVALID_HANDLE_VALUE){
						// Если файл является сокетом
						if(::GetFileType(file) == FILE_TYPE_PIPE)
							// Получаем тип файловой системы
							result = type_t::SOCK;
						// Выполняем закрытие файла
						::CloseHandle(file);
					}
				#endif
				/**
				 * Если операционной системой является macOS
				 */
				#if __APPLE__ || __MACH__
					/**
					 * Если целевая платформа является macOS
					 */
					#ifdef __AWH_USE_MACOS_ALIAS_RESOLUTION__
						// Alias-файлы всегда являются обычными файлами — проверяем только их и только если детект включён
						if(detectLinks && (result == type_t::FILE)){
							/**
							 * Выполняем проверку является ли файл alias-файлом
							 */
							@autoreleasepool {
								// Преобразуем путь в NSString
								NSString * path = [NSString stringWithUTF8String:addr.data()];
								// Если путь не существует
								if(!path || ![[NSFileManager defaultManager] fileExistsAtPath:path])
									// Возвращаем значение по умолчанию
									return result;
								// Создаём объект URL из пути
								NSURL * url = [NSURL fileURLWithPath:path];
								// Если объект URL не создан
								if(!url)
									// Возвращаем значение по умолчанию
									return result;
								// Объект для хранения информации о ресурсе
								NSNumber * isAliasNumber = nil;
								// Ошибки получения ресурса
								NSError * error = nil;
								// Проверяем, является ли файл alias-файлом
								BOOL success = [url getResourceValue:&isAliasNumber forKey:NSURLIsAliasFileKey error:&error];
								// Если файл является alias-файлом
								if(success && isAliasNumber && [isAliasNumber boolValue])
									// Получаем тип файловой системы
									result = type_t::LINK;
							}
						}
					#endif
				#endif
			}
			// Если детектирование ссылок включено и адрес ещё не детектирован как ссылка
			if(detectLinks && (result != type_t::LINK)){
				/**
				 * Для операционной системы не являющейся MS Windows
				 */
				#if !_WIN32 && !_WIN64
					// Если тип определён
					if(::lstat(addr.data(), &info) == 0){
						// Если это символьная ссылка
						if(S_ISLNK(info.st_mode))
							// Получаем тип файловой системы
							result = type_t::LINK;
					}
				/**
				 * Для операционной системы MS Windows
				 */
				#else
					// Ярлыки Windows всегда имеют расширение .lnk — иначе дорогой COM-вызов не требуется
					if((address.size() > 4) && (::_wcsicmp(address.c_str() + (address.size() - 4), L".lnk") == 0)){
						// Выполняем инициализацию результата
						HRESULT hres = ::CoInitialize(nullptr);
						// Создаём объект проверки наличия ярлыка
						com_guard_t <IShellLinkW> psl;
						// Выполняем инициализацию объекта для проверки ярлыков
						hres = ::CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW, reinterpret_cast <LPVOID *> (&psl));
						// Если инициализация выполнена
						if(SUCCEEDED(hres) && psl){
							// Создаём объект проверки файла
							com_guard_t <IPersistFile> ppf;
							// Выполняем инициализацию объекта для проверки файла
							hres = psl->QueryInterface(IID_IPersistFile, reinterpret_cast <void **> (&ppf));
							// Если объект для проверки файла инициализирован
							if(SUCCEEDED(hres) && ppf){
								// Выполняем загрузку переданного адреса
								hres = ppf->Load(address.c_str(), STGM_READ);
								// Если переданный адрес является ярлыком
								if(SUCCEEDED(hres))
									// Получаем тип файловой системы
									result = type_t::LINK;
							}
						}
						// Выполняем очистку объекта результата
						::CoUninitialize();
					}
				#endif
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const ios_base::failure & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {addr, detectLinks}, log::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, error.what());
			#endif
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {addr, detectLinks}, log::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод извлечения реального адреса
 *
 * @param addr    адрес который нужно определить
 * @param resolve флаг резолвинга символьных ссылок
 * @return        полный путь
 *
 */
string awh::Filesystem::fullpath(string_view addr, const bool resolve) const noexcept {
	// Переменная результата
	string result = "";
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Создаём буфер для полного адреса
			wchar_t buffer[_MAX_PATH];
			// Заполняем буфер нулями
			::memset(buffer, 0, sizeof(buffer));
			// Выполняем извлечение адресов из переменных окружений
			::ExpandEnvironmentStringsW(fmk::convert(addr.data()).c_str(), buffer, ARRAYSIZE(buffer));
			// Устанавливаем результат
			result = fmk::convert(buffer);
			// Заполняем буфер нулями
			::memset(buffer, 0, sizeof(buffer));
			// Если адрес существует
			if(::_wfullpath(buffer, fmk::convert(result).c_str(), _MAX_PATH) != nullptr){
				// Получаем полный адрес пути
				result = fmk::convert(buffer);
				// Если адрес пути получен
				if(resolve && !result.empty()){
					// Создаём объект проверки наличия ярлыка
					com_guard_t <IShellLinkW> psl;
					// Выполняем инициализацию результата
					HRESULT hres = ::CoInitialize(nullptr);
					// Выполняем инициализацию объекта для проверки ярлыков
					hres = ::CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW, reinterpret_cast <LPVOID *> (&psl));
					// Если инициализация выполнена
					if(SUCCEEDED(hres)){
						// Создаём объект проверки файла
						com_guard_t <IPersistFile> ppf;
						// Выполняем инициализацию объекта для проверки файла
						hres = psl->QueryInterface(IID_IPersistFile, reinterpret_cast <void **> (&ppf));
						// Если объект для проверки файла инициализирован
						if(SUCCEEDED(hres)){
							// Выполняем загрузку переданного адреса
							hres = ppf->Load(fmk::convert(result).c_str(), STGM_READ);
							// Если переданный адрес является ярлыком
							if(SUCCEEDED(hres)){
								// Выполняем резолвинг ярлыка
								hres = psl->Resolve(nullptr, 0);
								// Если резолвинг ярлыка удачно выполнен
								if(SUCCEEDED(hres)){
									// Создаём буфер символов для получения каталога ярлыка
									WCHAR szGotPath[MAX_PATH] = {0};
									// Выполняем получение каталога где находится ярлыка
									hres = psl->GetPath(szGotPath, _countof(szGotPath), nullptr, SLGP_RAWPATH);
									// Если каталог где находится ярлык получен
									if(SUCCEEDED(hres)){
										// Создаём буфер для извлечения полного адреса ярлыка
										WCHAR achPath[MAX_PATH] = {0};
										// Выполняем извлечение полного адреса ярлыка
										hres = ::StringCbCopyW(achPath, _countof(achPath), szGotPath);
										// Если полный адрес ярлыка извлечён
										if(SUCCEEDED(hres)){
											// Определяем размер полученных данных
											const int32_t size = ::WideCharToMultiByte(CP_UTF8, 0, achPath, -1, 0, 0, 0, 0);
											// Если размер извлекаемых данных получен
											if(size > 0){
												// Выполняем выделение памяти для результирующего буфера
												result.resize(static_cast <size_t> (size), 0);
												// Выполняем извлечение полного адреса ярлыка
												::WideCharToMultiByte(CP_UTF8, 0, achPath, -1, result.data(), static_cast <int32_t> (result.size()), 0, 0);
											}
										}
									}
								}
							}
						}
					}
					// Выполняем очистку объекта результата
					::CoUninitialize();
				}
			}
			// Выполняем перекодирование адреса
			return result;
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#else
			// Если нужно выполнять резолвинг символьных ссылок
			if(resolve && !addr.empty()){
				// Устанавливаем переданный путь адреса
				result = addr.data();
				// Создаём буфер данных для получения адреса
				char buffer[PATH_MAX];
				// Если адрес существует
				if(::realpath(result.c_str(), buffer) != nullptr){
					// Получаем полный адрес пути
					result = buffer;
					/**
					 * Если целевая платформа является macOS
					 */
					#ifdef __AWH_USE_MACOS_ALIAS_RESOLUTION__
						/**
						 * Выполняем проверку является ли файл alias-файлом
						 */
						@autoreleasepool {
							// Преобразуем путь в NSString
							NSString * nsPath = [NSString stringWithUTF8String:buffer];
							// Если путь не существует
							if(!nsPath || ![[NSFileManager defaultManager] fileExistsAtPath:nsPath])
								// Файл не существует — не трогаем
								return result;
							// Создаём объект URL из пути
							NSURL * url = [NSURL fileURLWithPath:nsPath];
							// Если объект URL не создан
							if(!url)
								// Возвращаем результат как он есть
								return result;
							// Переменная для хранения ошибок
							NSError * error = nil;
							// Объект для хранения информации о ресурсе
							NSNumber * isAliasNumber = nil;
							// Проверяем, является ли файл alias-файлом
							BOOL success = [url getResourceValue:&isAliasNumber forKey:NSURLIsAliasFileKey error:&error];
							// Если возникла ошибка при проверке
							if(error)
								// Ошибка — возвращаем как есть
								return result;
							// Если файл является alias-файлом
							if(success && isAliasNumber && [isAliasNumber boolValue]){
								// Получаем bookmark-данные из alias-файла
								NSData * bookmarkData = [NSURL bookmarkDataWithContentsOfURL:url error:&error];
								// Если bookmark-данные получены
								if(bookmarkData){
									// Выполняем резолвинг alias-файла
									NSURL * resolvedURL = [NSURL URLByResolvingBookmarkData:bookmarkData options:0 relativeToURL:nil bookmarkDataIsStale:nil error:&error];
									// Если резолвинг выполнен
									if(resolvedURL){
										// Флаг определения каталога
										BOOL isDir = NO;
										// Проверяем является ли разрешённый путь каталогом
										[[NSFileManager defaultManager] fileExistsAtPath:[resolvedURL path] isDirectory:&isDir];
										// Получаем результат резолвинга
										result = [[resolvedURL path] UTF8String];
										// Если это каталог — добавляем завершающий слеш
										if(isDir && !result.empty() && (result.back() != '/'))
											// Добавляем завершающий слеш
											result.append(1, '/');
										// Возвращаем результат
										return result;
									}
								}
							}
						}
					#endif
					// Возвращаем результат
					return result;
				// Если идентификатор обнулился после переполнения счётчика и является ссылкой
				} else if(this->type(result) == type_t::LINK) {
					// Получаем длину полученного адреса
					const ssize_t length = ::readlink(result.c_str(), buffer, sizeof(buffer) - 1);
					// Если длина адреса получена
					if(length != -1){
						// Выполняем установку конца строки
						buffer[length] = '\0';
						// Возвращаем результат
						return buffer;
					}
				}
			}
			// Формируем полный путь как он есть
			return ::fullpath(addr);
		#endif
	/**
	 * Если возникает ошибка
	 */
	} catch(const ios_base::failure & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {addr, resolve}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {addr, resolve}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод получения прав доступа к файлу или каталогу
 *
 * @param addr путь к файлу или каталогу
 * @return     запрашиваемые метаданные
 *
 */
uint32_t awh::Filesystem::chmod(string_view addr) const noexcept {
	// Переменная результата
	uint32_t result = 0;
	// Если путь к файлу или каталогу передан
	if(!addr.empty() && (this->type(addr) != type_t::NONE)){
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Выполняем извлечение актуального значения адреса
			const string & address = this->fullpath(addr, true);
			// Если адрес получен правильный
			if(!address.empty())
				// Извлекаем все атрибуты файла
				return static_cast <uint32_t> (::GetFileAttributesW(fmk::convert(address).c_str()));
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#else
			// Создаём объект информационных данных файла или каталога
			struct stat info{};
			// Выполняем чтение информационных данных файла
			if(!(result = (::stat(addr.data(), &info) == 0)) && (errno != 0)){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					log::debug("%s", __PRETTY_FUNCTION__, {addr}, log::flag_t::CRITICAL, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем в лог сообщение
					log::print("%s", log::flag_t::CRITICAL, ::strerror(errno));
				#endif
			// Если информационные данные считаны удачно
			} else result = static_cast <uint32_t> (info.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод изменения прав доступа к файлу или каталогу
 *
 * @param addr путь к файлу или каталогу
 * @param mode метаданные для установки
 * @return     результат работы функции
 *
 */
bool awh::Filesystem::chmod(string_view addr, const uint32_t mode) const noexcept {
	// Переменная результата
	bool result = false;
	// Если путь к файлу или каталогу передан
	if(!addr.empty() && (this->type(addr) != type_t::NONE)){
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Выполняем извлечение актуального значения адреса
			const string & address = this->fullpath(addr, true);
			// Если адрес получен правильный
			if(!address.empty())
				// Выполняем установку атрибутов файла
				return ::SetFileAttributesW(fmk::convert(address).c_str(), static_cast <DWORD> (mode));
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#else
			// Выполняем установку метаданных файла
			if(!(result = (::chmod(addr.data(), static_cast <mode_t> (mode)) == 0)) && (errno != 0)){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					log::debug("%s", __PRETTY_FUNCTION__, {addr, mode}, log::flag_t::CRITICAL, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем в лог сообщение
					log::print("%s", log::flag_t::CRITICAL, ::strerror(errno));
				#endif
			}
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод установки владельца на файл или каталог
 *
 * @param addr  путь к файлу или каталогу для установки владельца
 * @param user  имя пользователя
 * @param group название группы пользователя
 * @return      результат работы функции
 *
 */
bool awh::Filesystem::chown(string_view addr, string_view user, [[maybe_unused]] string_view group) const noexcept {
	// Переменная результата
	bool result = false;
	// Если путь передан
	if(!addr.empty() && !user.empty() && (this->type(addr) != type_t::NONE)){
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#if !_WIN32 && !_WIN64
			// Если группа пользователя передана
			if(!group.empty()){
				// Идентификатор пользователя
				const uid_t uid = this->_os.uid(user.data());
				// Идентификатор группы
				const gid_t gid = this->_os.group(group.data());
				// Устанавливаем права на каталог
				if((result = (uid && gid))){
					// Выполняем установку владельца
					if(!(result = (::chown(addr.data(), uid, gid) == 0)) && (errno != 0)){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							log::debug("%s", __PRETTY_FUNCTION__, {addr, user, group}, log::flag_t::CRITICAL, ::strerror(errno));
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем в лог сообщение
							log::print("%s", log::flag_t::CRITICAL, ::strerror(errno));
						#endif
					}
				}
			}
		/**
		 * Для операционной системы MS Windows
		 */
		#else
			// Тип SID-а
			SID_NAME_USE sidType;
			// Размер SID-а пользователя/группы и домена пользователя
			DWORD sidSize = 0, domainSize = 0;
			// Получаем путь к файлу
			wstring fileName = fmk::convert(addr.data());
			// Получаем имя пользователя
			wstring userName = fmk::convert(user.data());
			// Первый вызов — получаем размеры буферов
			::LookupAccountNameW(nullptr, userName.c_str(), nullptr, &sidSize, nullptr, &domainSize, &sidType);
			// Если мы получиши ошибку извлечения размеров буфера
			if(::GetLastError() != ERROR_INSUFFICIENT_BUFFER){
				// Создаём буфер сообщения ошибки
				wchar_t message[0xFF] = {0};
				// Выполняем формирование текста ошибки
				::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 0xFF, 0);
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					log::debug(L"%s", __PRETTY_FUNCTION__, {addr, user}, log::flag_t::CRITICAL, message);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					log::print(L"%s", log::flag_t::CRITICAL, message);
				#endif
				// Возвращаем результат
				return result;
			}
			// Инициализируем доменное имя пользователя
			wstring domain(domainSize, L'\0');
			// Выделяем память под SID и домен
			PSID pSid = (PSID) ::LocalAlloc(LPTR, sidSize);
			// Извлекаем SID пользователя и его доменное имя
			if(!::LookupAccountNameW(nullptr, userName.c_str(), pSid, &sidSize, &domain[0], &domainSize, &sidType)){
				// Освобождаем ресурсы
				::LocalFree(pSid);
				// Возвращаем пустой результат
				return result;
			}
			// Объект параметров доступа
			EXPLICIT_ACCESSW ea = {};
			// Зануляем объект параметров доступа
			::ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
			// Устанавливаем новые права
			ea.grfAccessMode = SET_ACCESS;
			// Устанавливаем идентификатор пользователя
			ea.Trustee.ptstrName = (LPWSTR) pSid;
			// Устанавливаем права доступа для SID-пользователя
			ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
			// Устанавливаем тип инициатора пользователя
			ea.Trustee.TrusteeType = TRUSTEE_IS_USER;
			// Наследование для подпапок и файлов
			ea.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
			// Права: чтение + запись + выполнение (если файл исполняемый)
			ea.grfAccessPermissions = (GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE);
			// Дескриптор системы безопасности
			PSECURITY_DESCRIPTOR sd = nullptr;
			// Старые и новые параметры безопасности
			PACL pOldDACL = nullptr, pNewDACL = nullptr;
			// Получаем текущий DACL файла
			if(::GetNamedSecurityInfoW(fileName.c_str(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, &pOldDACL, nullptr, &sd) != ERROR_SUCCESS){
				// Создаём буфер сообщения ошибки
				wchar_t message[0xFF] = {0};
				// Выполняем формирование текста ошибки
				::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 0xFF, 0);
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					log::debug(L"%s", __PRETTY_FUNCTION__, {addr, user}, log::flag_t::CRITICAL, message);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					log::print(L"%s", log::flag_t::CRITICAL, message);
				#endif
				// Освобождаем ресурсы
				::LocalFree(pSid);
				// Возвращаем пустой результат
				return result;
			}
			// Создаем новый DACL с добавленной записью
			if(::SetEntriesInAclW(1, &ea, pOldDACL, &pNewDACL) != ERROR_SUCCESS){
				// Создаём буфер сообщения ошибки
				wchar_t message[0xFF] = {0};
				// Выполняем формирование текста ошибки
				::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 0xFF, 0);
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					log::debug(L"%s", __PRETTY_FUNCTION__, {addr, user}, log::flag_t::CRITICAL, message);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					log::print(L"%s", log::flag_t::CRITICAL, message);
				#endif
				// Освобождаем дескриптор системы безопасности
				::LocalFree(sd);
				// Освобождаем ресурсы
				::LocalFree(pSid);
				// Возвращаем пустой результат
				return result;
			}
			// Применяем новый DACL к файлу
			if(!(result = (::SetNamedSecurityInfoW(reinterpret_cast <LPWSTR> (fileName.data()), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, pNewDACL, nullptr) == ERROR_SUCCESS))){
				// Создаём буфер сообщения ошибки
				wchar_t message[0xFF] = {0};
				// Выполняем формирование текста ошибки
				::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 0xFF, 0);
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					log::debug(L"%s", __PRETTY_FUNCTION__, {addr, user}, log::flag_t::CRITICAL, message);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					log::print(L"%s", log::flag_t::CRITICAL, message);
				#endif
			}
			// Освобождаем дескриптор системы безопасности
			::LocalFree(sd);
			// Освобождаем ресурсы
			::LocalFree(pSid);
			// Очищаем новый объект параметров безопасности
			::LocalFree(pNewDACL);
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод рекурсивного создания пути
 *
 * @param addr адрес для создания каталога
 * @return     результат создания каталога
 *
 */
bool awh::Filesystem::mkdir(string_view addr) const noexcept {
	// Переменная результата
	bool result = false;
	// Если путь передан
	if(!addr.empty()){
		// Проверяем существует ли нужный нам каталог
		if((result = (this->type(addr) == type_t::NONE))){
			/**
			 * Выполняем перехват ошибок
			 */
			try {
				// Выполняем извлечение актуального значения адреса
				const string & address = this->fullpath(addr);
				// Если адрес получен правильный
				if((result = !address.empty())){
					// Мутабельная копия полного адреса (без фиксированного буфера и snprintf для портируемости)
					string buffer(address);
					// Если последний символ является сепаратором тогда удаляем его
					if(!buffer.empty() && (buffer.back() == AWH_FS_SEPARATOR[0]))
						// Удаляем завершающий сепаратор
						buffer.pop_back();
					/**
					 * Переходим по всем символам адреса, создавая каталоги по мере прохождения
					 */
					for(size_t i = 1; i < buffer.length(); ++i){
						// Если найден сепаратор
						if(buffer[i] == AWH_FS_SEPARATOR[0]){
							// Временно обрываем строку нулём, чтобы получить промежуточный путь
							buffer[i] = '\0';
							/**
							 * Для операционной системы не являющейся MS Windows
							 */
							#if !_WIN32 && !_WIN64
								// Создаем каталог
								result = (::mkdir(buffer.c_str(), S_IRWXU) == 0);
							/**
							 * Для операционной системы MS Windows
							 */
							#else
								// Если это дисковой сепаратор
								if((i == 2) && (buffer[1] == ':')){
									// Запоминаем сепаратор
									buffer[i] = AWH_FS_SEPARATOR[0];
									// Продолжаем перебор адреса
									continue;
								}
								// Создаем каталог
								result = (::_wmkdir(fmk::convert(buffer.c_str()).c_str()) == 0);
							#endif
							// Если каталог уже существует
							if(!result && (errno == EEXIST))
								// Восстанавливаем флаг
								result = true;
							// Запоминаем сепаратор
							buffer[i] = AWH_FS_SEPARATOR[0];
						// Если это последний символ в строке
						} else if((i + 1) == buffer.length()) {
							/**
							 * Для операционной системы не являющейся MS Windows
							 */
							#if !_WIN32 && !_WIN64
								// Создаем каталог
								result = (::mkdir(buffer.c_str(), S_IRWXU) == 0);
							/**
							 * Для операционной системы MS Windows
							 */
							#else
								// Если это дисковой сепаратор
								if((buffer.length() == 2) && (buffer[1] == ':'))
									// Выходим из цикла
									break;
								// Создаем каталог
								result = (::_wmkdir(fmk::convert(buffer.c_str()).c_str()) == 0);
							#endif
							// Если каталог уже существует
							if(!result && (errno == EEXIST))
								// Восстанавливаем флаг
								result = true;
						}
						// Если создание каталога не удалось
						if(!result)
							// Прекращаем выполнение цикла
							break;
					}
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const bad_alloc &) {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					log::debug("Memory allocation error", __PRETTY_FUNCTION__, {addr}, log::flag_t::CRITICAL);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					log::print("Memory allocation error", log::flag_t::CRITICAL);
				#endif
				// Выходим из приложения
				::_exit(EXIT_FAILURE);
			/**
			 * Если возникает ошибка
			 */
			} catch(const ios_base::failure & error) {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					log::debug("%s", __PRETTY_FUNCTION__, {addr}, log::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					log::print("%s", log::flag_t::CRITICAL, error.what());
				#endif
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					log::debug("%s", __PRETTY_FUNCTION__, {addr}, log::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					log::print("%s", log::flag_t::CRITICAL, error.what());
				#endif
			}
		}
	}
	// Сообщаем что каталог и так существует
	return result;
}
/**
 * @brief Метод создания каталога с указанием владельца
 *
 * @param addr  адрес для создания каталога
 * @param user  имя пользователя
 * @param group название группы пользователя
 * @return      результат создания каталога
 *
 */
bool awh::Filesystem::mkdir(string_view addr, string_view user, string_view group) const noexcept {
	// Переменная результата
	bool result = false;
	// Проверяем существует ли нужный нам каталог
	if((result = (this->type(addr) == type_t::NONE))){
		// Создаем каталог
		if((result = this->mkdir(addr)))
			/**
			 * @warning Охват «не MS Windows» здесь стоял с той поры, когда у метода
			 *          chown тела под MS Windows ещё не было. Тело появилось - оно
			 *          ставит владельца через LookupAccountNameW и SetNamedSecurityInfoW,
			 *          - а охват остался, и каталог создавался там БЕЗ владельца,
			 *          причём молча: метод отвечал истиной, будто права поставлены.
			 *          Название группы под MS Windows не применяется, о чём сказано
			 *          у самого chown, но владелец обязан ставиться на всех системах
			 */
			// Устанавливаем права на каталог
			result = this->chown(addr, user, group);
	}
	// Сообщаем что каталог и так существует
	return result;
}
/**
 * @brief Метод подмены целевого файла временным
 *
 * @param temporary адрес временного файла записи
 * @param filename  адрес целевого файла записи
 * @return          признак успешной подмены
 *
 */
bool awh::Filesystem::replaceAddress(string_view temporary, string_view filename) const noexcept {
	/**
	 * Для операционной системы, MS Windows не являющейся
	 */
	#if !_WIN32 && !_WIN64
		// Выполняем подмену целевого файла временным
		return (::rename(string(temporary).c_str(), string(filename).c_str()) == 0);
	/**
	 * Для операционной системы MS Windows
	 */
	#else
		/**
		 * Выполняем подмену целевого файла временным с заменою на месте
		 *
		 * @note Зовётся узкий вид, а не широкий: пути ходят здесь `std::string`
		 */
		return (::MoveFileExW(fmk::convert(temporary).c_str(), fmk::convert(filename).c_str(), (MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) != 0);
	#endif
}
/**
 * @brief Метод извлечения названия и расширения файла
 *
 * @param addr    путь к файлу для извлечения его параметров
 * @param resolve флаг резолвинга символьных ссылок
 * @param before  флаг определения первой точки расширения слева
 *
 */
awh::Filesystem::components_t awh::Filesystem::components(string_view addr, const bool resolve, const bool before) const noexcept {
	// Переменная результата
	components_t result;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Получаем полный адрес пути
		const string & filename = this->fullpath(addr, resolve);
		// Если файл передан
		if(!filename.empty()){
			// Позиция разделителя каталога
			size_t pos = 0;
			// Определяем флаг обратного смещения
			const uint8_t offset = (filename.back() == AWH_FS_SEPARATOR[0] ? 2 : 1);
			// Выполняем поиск разделителя каталога
			if((pos = filename.rfind(AWH_FS_SEPARATOR, filename.length() - static_cast <size_t> (offset))) != string::npos){
				// Если переданный адрес является каталогом
				if(this->type(filename) == type_t::DIR)
					// Выполняем вывод названия каталога
					result.first = filename.substr(pos + 1, filename.length() - (pos + static_cast <size_t> (offset)));
				// Если переданный адрес не является каталогом
				else {
					// Извлекаем имя файла
					string name = filename.substr(pos + 1);
					// Ищем расширение файла
					if((pos = (before ? name.find('.') : name.rfind('.'))) != string::npos){
						// Устанавливаем имя файла
						result.first = name.substr(0, pos);
						// Устанавливаем расширение файла
						result.second = name.substr(pos + 1);
					// Устанавливаем только имя файла
					} else result.first = ::move(name);
				}
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const ios_base::failure & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {addr, resolve, before}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {addr, resolve, before}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод подсчёта размера файла/каталога
 *
 * @param addr    адрес для подсчёта размера
 * @param ext     расширение файла если требуется фильтрация
 * @param recurse флаг рекурсивного перебора каталогов
 * @return        общий размер файла/каталога
 *
 */
uintmax_t awh::Filesystem::size(string_view addr, string_view ext, const bool recurse) const noexcept {
	// Переменная результата
	uintmax_t result = 0;
	// Если путь для подсчёта передан
	if(!addr.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем извлечение актуального значения адреса
			const string & address = this->fullpath(addr, true);
			// Если адрес получен правильный
			if(!address.empty()){
				// Получаем обёртку полученного пути
				string_view path = address;
				/**
				 * Определяем тип переданного пути
				 */
				switch(static_cast <uint8_t> (this->type(path, false))){
					// Если полный путь является файлом
					case static_cast <uint8_t> (type_t::FILE): {
						/**
						 * Для операционной системы MS Windows
						 */
						#if _WIN32 || _WIN64
							// Создаём объект работы с файлом
							/**
							 * @note Дозволяется и запись, и удаление, а не одно лишь чтение: файл вправе
							 *       держать открытым кто-то ещё - движок наблюдения за файловой системой
							 *       держит его именно так, - и обращение с одним лишь дозволением чтения
							 *       отвечало бы отказом ERROR_SHARING_VIOLATION. Отказ этот молчаливый:
							 *       дозапись уходила бы мимо файла, а размер выдавался бы нулевым
							 */
							HANDLE file = ::CreateFileW(fmk::convert(path.data()).c_str(), GENERIC_READ, (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
							// Если файл открыт нормально
							if(file != INVALID_HANDLE_VALUE){
								// Объект для хранения размера файла
								LARGE_INTEGER size;
								// Получаем размер файла
								if(!::GetFileSizeEx(file, &size)){
									// Создаём буфер сообщения ошибки
									wchar_t message[0xFF] = {0};
									// Выполняем формирование текста ошибки
									::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 0xFF, 0);
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										log::debug(L"%s", __PRETTY_FUNCTION__, {addr, ext, recurse}, log::flag_t::CRITICAL, message);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										log::print(L"%s", log::flag_t::CRITICAL, message);
									#endif
									// Выполняем закрытие файла
									::CloseHandle(file);
									// Возвращаем нулевой размер файла
									return result;
								}
								// Выполняем закрытие файла
								::CloseHandle(file);
								// Формируем полный размер файла
								result = static_cast <uintmax_t> (size.QuadPart);
							}
						/**
						 * Для операционной системы не являющейся MS Windows
						 */
						#else
							// Структура проверка статистики
							struct stat info{};
							// Если статистика извлечена
							if(::stat(path.data(), &info) == 0)
								// Выполняем извлечение данных статистики
								result = static_cast <uintmax_t> (info.st_size);
						#endif
					} break;
					// Если полный путь является каталогом
					case static_cast <uint8_t> (type_t::DIR): {
						/**
						 * Для операционной системы MS Windows
						 */
						#if _WIN32 || _WIN64
							// Открываем указанный каталог
							HandleDir dir(awh::dir::_wopendir(fmk::convert(path.data()).c_str()));
						/**
						 * Для операционной системы не являющейся MS Windows
						 */
						#else
							// Открываем указанный каталог
							HandleDir dir(awh::dir::opendir(path.data()));
						#endif
						// Если каталог открыт
						if(dir.valid()){
							/**
							 * Для операционной системы MS Windows
							 */
							#if _WIN32 || _WIN64
								// Создаем указатель на содержимое каталога
								awh::dir::_wdirent * ptr = nullptr;
								/**
								 * Выполняем чтение содержимого каталога
								 */
								while((ptr = awh::dir::_wreaddir(dir))){
							/**
							 * Для операционной системы не являющейся MS Windows
							 */
							#else
								// Создаем указатель на содержимое каталога
								awh::dir::dirent * ptr = nullptr;
								/**
								 * Выполняем чтение содержимого каталога
								 */
								while((ptr = awh::dir::readdir(dir))){
							#endif
									/**
									 * Для операционной системы MS Windows
									 */
									#if _WIN32 || _WIN64
										// Пропускаем названия текущие "." и внешние "..", так как идет рекурсия
										if(!::wcscmp(ptr->d_name, L".") || !::wcscmp(ptr->d_name, L".."))
											// Выполняем пропуск каталога
											continue;
										// Получаем полный путь в виде строки
										const string & address = fmk::format("%s%s%s", path.data(), AWH_FS_SEPARATOR, fmk::convert(ptr->d_name).c_str());
									/**
									 * Для операционной системы не являющейся MS Windows
									 */
									#else
										// Пропускаем названия текущие "." и внешние "..", так как идет рекурсия
										if(!::strcmp(ptr->d_name, ".") || !::strcmp(ptr->d_name, ".."))
											// Выполняем пропуск каталога
											continue;
										// Получаем полный путь в виде строки
										const string & address = fmk::format("%s%s%s", path.data(), AWH_FS_SEPARATOR, ptr->d_name);
									#endif
									/**
									 * Определяем тип переданного пути
									 */
									switch(static_cast <uint8_t> (this->type(address))){
										// Если полный путь является каталогом
										case static_cast <uint8_t> (type_t::DIR):
											// Выполняем подсчёт размера каталога
											result += (recurse ? this->size(address, ext, recurse) : 0);
										break;
										// Если полный путь является файлом
										case static_cast <uint8_t> (type_t::FILE): {
											// Флаг необходимости учёта файла
											bool allowed = true;
											// Если расширение файла передано
											if(!ext.empty()){
												// Получаем обёртку полученного пути
												string_view path = address;
												// Получаем расширение файла
												const string & extension = fmk::format(".%s", ext.data());
												// Файл учитывается только если его расширение совпадает с фильтром
												allowed = ((path.size() > extension.size()) && fmk::compare(path.substr(path.size() - extension.size()).data(), extension));
											}
											// Если файл нужно учесть — получаем его размер напрямую, без повторного резолвинга
											if(allowed){
												/**
												 * Для операционной системы MS Windows
												 */
												#if _WIN32 || _WIN64
													// Структура проверка статистики
													struct _stat info{};
													// Если статистика извлечена
													if(!::_wstat(fmk::convert(address).c_str(), &info))
												/**
												 * Для операционной системы не являющейся MS Windows
												 */
												#else
													// Структура проверка статистики
													struct stat info{};
													// Если статистика извлечена
													if(!::stat(address.c_str(), &info))
												#endif
														// Выполняем извлечение данных статистики
														result += static_cast <uintmax_t> (info.st_size);
											}
										} break;
										// Если путь принадлежит к другому типу
										default: {
											/**
											 * Для операционной системы MS Windows
											 */
											#if _WIN32 || _WIN64
												// Структура проверка статистики
												struct _stat info{};
												// Если статистика извлечена
												if(!::_wstat(fmk::convert(address).c_str(), &info))
											/**
											 * Для операционной системы не являющейся MS Windows
											 */
											#else
												// Структура проверка статистики
												struct stat info{};
												// Если статистика извлечена
												if(!::stat(address.c_str(), &info))
											#endif
													// Выполняем извлечение данных статистики
													result += static_cast <uintmax_t> (info.st_size);
										}
									}
								}
						}
					} break;
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const ios_base::failure & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {addr, ext, recurse}, log::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, error.what());
			#endif
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {addr, ext, recurse}, log::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод подсчёта количества файлов в каталоге
 *
 * @param addr    адрес для подсчёта количества файлов
 * @param ext     расширение файла если требуется фильтрация
 * @param recurse флаг рекурсивного перебора каталогов
 * @return        количество файлов в каталоге
 *
 */
uintmax_t awh::Filesystem::count(string_view addr, string_view ext, const bool recurse) const noexcept {
	// Переменная результата
	uintmax_t result = 0;
	// Если адрес каталога и расширение файлов переданы
	if(!addr.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем извлечение актуального значения адреса
			const string & address = this->fullpath(addr, true);
			// Если адрес получен правильный
			if(!address.empty() && (this->type(address, false) == type_t::DIR)){
				// Получаем обёртку полученного пути
				string_view path = address;
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					// Открываем указанный каталог
					HandleDir dir(awh::dir::_wopendir(fmk::convert(path.data()).c_str()));
				/**
				 * Для операционной системы не являющейся MS Windows
				 */
				#else
					// Открываем указанный каталог
					HandleDir dir(awh::dir::opendir(path.data()));
				#endif
					// Если каталог открыт
					if(dir.valid()){
						/**
						 * Для операционной системы MS Windows
						 */
						#if _WIN32 || _WIN64
							// Создаем указатель на содержимое каталога
							awh::dir::_wdirent * ptr = nullptr;
							/**
							 * Выполняем чтение содержимого каталога
							 */
							while((ptr = awh::dir::_wreaddir(dir))){
						/**
						 * Для операционной системы не являющейся MS Windows
						 */
						#else
							// Создаем указатель на содержимое каталога
							awh::dir::dirent * ptr = nullptr;
							/**
							 * Выполняем чтение содержимого каталога
							 */
							while((ptr = awh::dir::readdir(dir))){
						#endif
								/**
								 * Для операционной системы MS Windows
								 */
								#if _WIN32 || _WIN64
									// Пропускаем названия текущие "." и внешние "..", так как идет рекурсия
									if(!::wcscmp(ptr->d_name, L".") || !::wcscmp(ptr->d_name, L".."))
										// Выполняем пропуск каталога
										continue;
									// Получаем адрес в виде строки
									const string & address = fmk::format("%s%s%s", path.data(), AWH_FS_SEPARATOR, fmk::convert(ptr->d_name).c_str());
								/**
								 * Для операционной системы не являющейся MS Windows
								 */
								#else
									// Пропускаем названия текущие "." и внешние "..", так как идет рекурсия
									if(!::strcmp(ptr->d_name, ".") || !::strcmp(ptr->d_name, ".."))
										// Выполняем пропуск каталога
										continue;
									// Получаем адрес в виде строки
									const string & address = fmk::format("%s%s%s", path.data(), AWH_FS_SEPARATOR, ptr->d_name);
								#endif
								/**
								 * Определяем тип переданного пути
								 */
								switch(static_cast <uint8_t> (this->type(address))){
									// Если полный путь является каталогом
									case static_cast <uint8_t> (type_t::DIR):
										// Подсчитываем количество файлов в каталоге
										result += (recurse ? this->count(address, ext, recurse) : 0);
									break;
									// Если путь принадлежит к другому типу
									default: {
										// Если расширение файла передано
										if(!ext.empty()){
											// Получаем обёртку полученного пути
											string_view path = address;
											// Получаем расширение файла
											const string & extension = fmk::format(".%s", ext.data());
											// Если расширение не выше полного адреса
											if(path.size() > extension.size()){
												// Если расширение файла найдено
												if(fmk::compare(path.substr(path.size() - extension.size()).data(), extension))
													// Получаем количество файлов в каталоге
													result++;
											}
										// Если расширение файла не передано, то просто получаем количество файлов в каталоге
										} else result++;
									}
								}
							}
					}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const ios_base::failure & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {addr, ext, recurse}, log::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, error.what());
			#endif
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {addr, ext, recurse}, log::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, error.what());
			#endif
		}
	// Если переданный адрес не является каталогом
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			log::debug("Address name: \"%s\" is not dir", __PRETTY_FUNCTION__, {addr, ext, recurse}, log::flag_t::WARNING, addr.data());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("Address name: \"%s\" is not dir", log::flag_t::WARNING, addr.data());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод создания объекта каталога
 *
 * @return умный указатель на объект каталога
 *
 */
awh::handle_dir_t awh::Filesystem::handleDir() const noexcept {
	// Выводим созданный объект каталога
	return handle_dir_t(new HandleDir());
}
/**
 * @brief Метод создания объекта файла
 *
 * @return умный указатель на объект файла
 *
 */
awh::handle_file_t awh::Filesystem::handleFile() const noexcept {
	// Выводим созданный объект файла
	return handle_file_t(new HandleFile());
}
/**
 * @brief Метод усечения файла до заданной длины
 *
 * @param filename путь к файлу который необходимо усечь
 * @param length   длина, до какой усекается файл
 * @param handle   внешний объект файла, если необходима поддержка пакетной обработки
 *
 * @return         признак того, что усечение выполнено
 *
 */
bool awh::Filesystem::truncate(string_view filename, const uint64_t length, const handle_file_t & handle) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если адрес файла передан
	if(!filename.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * @brief Свой объект файла
			 *
			 * @note Заводится он всегда, но в дело идёт ЛИШЬ когда внешний объект не передан.
			 *       Так путь работы выходит один на оба случая, а закрытие описателя остаётся
			 *       делом самого объекта
			 */
			HandleFile local;
			// Объект файла, которым идёт работа
			HandleFile & file = (handle != nullptr ? (* handle) : local);
			/**
			 * Выполняем извлечение актуального значения адреса
			 *
			 * @note Извлекается он ЛИШЬ когда объект файла ещё не заведён: адрес идёт только
			 *       в открытие, а при уже открытом описателе не нужен вовсе
			 */
			const string address = (!file.valid() ? this->fullpath(filename, true) : string());
			// Если объект файла заведён либо адрес получен правильный
			if(file.valid() || !address.empty()){
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					// Если объект файла ещё не заведён
					if(!file.valid())
						/**
						 * @brief Открываем файл на чтение и запись, заводя отсутствующий
						 *
						 * @note Признак `OPEN_ALWAYS` заводит отсутствующий файл и оставляет
						 *       существующий как есть: усечение до нуля обязано и заводить
						 *       пустой файл, а `CREATE_ALWAYS` усекал бы и то, что усекать
						 *       не велено
						 */
						file.set(::CreateFileW(fmk::convert(address).c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
					// Если файл открыт нормально
					if(file.valid()){
						// Создаём объект большого числа
						LARGE_INTEGER li;
						// Устанавливаем длину, до какой усекается файл
						li.QuadPart = static_cast <LONGLONG> (length);
						/**
						 * Выполняем установку позиции и усечение файла по ней
						 *
						 * @note Разделения на усечение и наращивание у MS Windows нет:
						 *       `SetEndOfFile` кладёт конец файла туда, где стоит позиция,
						 *       в обе стороны
						 */
						result = ((::SetFilePointerEx(file, li, nullptr, FILE_BEGIN) != FALSE) && (::SetEndOfFile(file) != FALSE));
						// Если усечение отвечено отказом
						if(!result){
							// Создаём буфер сообщения ошибки
							wchar_t message[0xFF] = {0};
							// Выполняем формирование текста ошибки
							::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::GetLastError(), 0, message, 0xFF, 0);
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log::debug(L"%s", __PRETTY_FUNCTION__, {filename, length}, log::flag_t::CRITICAL, message);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								log::print(L"%s", log::flag_t::CRITICAL, message);
							#endif
						}
					/**
					 * Если открыть файл не удалось
					 */
					} else {
						// Создаём буфер сообщения ошибки
						wchar_t message[0xFF] = {0};
						// Выполняем формирование текста ошибки
						::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::GetLastError(), 0, message, 0xFF, 0);
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							log::debug(L"%s", __PRETTY_FUNCTION__, {filename, length}, log::flag_t::CRITICAL, message);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log::print(L"%s", log::flag_t::CRITICAL, message);
						#endif
					}
				/**
				 * Для операционной системы не являющейся MS Windows
				 */
				#else
					// Если объект файла ещё не заведён
					if(!file.valid())
						/**
						 * @brief Открываем файл на запись, заводя отсутствующий
						 *
						 * @note Ход этот равен `OPEN_ALWAYS` у ветви MS Windows: отсутствующий
						 *       файл заводится, существующий открывается КАК ЕСТЬ. Признак
						 *       `O_TRUNC` здесь негоден - он усекал бы и тогда, когда велено
						 *       усечь до длины ненулевой
						 */
						file.set(::open(address.c_str(), O_WRONLY | O_CREAT, 0644));
					// Если файл открыт нормально
					if(file.valid()){
						// Выполняем усечение файла до заданной длины
						result = (::ftruncate(file, static_cast <off_t> (length)) == 0);
						// Если усечение отвечено отказом
						if(!result){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log::debug("%s", __PRETTY_FUNCTION__, {filename, length}, log::flag_t::CRITICAL, ::strerror(errno));
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								log::print("%s", log::flag_t::CRITICAL, ::strerror(errno));
							#endif
						}
					/**
					 * Если открыть файл не удалось
					 */
					} else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							log::debug("%s", __PRETTY_FUNCTION__, {filename, length}, log::flag_t::CRITICAL, ::strerror(errno));
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log::print("%s", log::flag_t::CRITICAL, ::strerror(errno));
						#endif
					}
				#endif
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {filename, length}, log::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод сброса записанного из ядра на носитель
 *
 * @param filename путь к файлу который необходимо сбросить на носитель
 * @param durable  признак доведения записанного до носителя, а не до накопителя
 * @param handle   внешний объект файла, если необходима поддержка пакетной обработки
 * @return         признак того, что сброс выполнен
 *
 */
bool awh::Filesystem::flush(string_view filename, const bool durable, const handle_file_t & handle) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если адрес файла передан
	if(!filename.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * @brief Свой объект файла
			 *
			 * @note Заводится он всегда, но в дело идёт ЛИШЬ когда внешний объект не передан.
			 *       Сброс своего объекта смысла почти не имеет - записанное чужим описателем
			 *       он не доведёт, - но отказом это не считается: файл открывается заново и
			 *       сбрасывается как есть
			 */
			HandleFile local;
			// Объект файла, которым идёт работа
			HandleFile & file = (handle != nullptr ? (* handle) : local);
			/**
			 * Выполняем извлечение актуального значения адреса
			 *
			 * @note Извлекается он ЛИШЬ когда объект файла ещё не заведён: адрес идёт только
			 *       в открытие, а при уже открытом описателе не нужен вовсе
			 */
			const string address = (!file.valid() ? this->fullpath(filename, true) : "");
			// Если объект файла заведён либо адрес получен правильный
			if(file.valid() || !address.empty()){
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					// Если объект файла ещё не заведён
					if(!file.valid())
						/**
						 * @brief Открываем файл на запись
						 *
						 * @note Сброс требует права записи: описатель, открытый на одно лишь
						 *       чтение, `FlushFileBuffers` отвергает отказом ERROR_ACCESS_DENIED
						 */
						file.set(::CreateFileW(fmk::convert(address).c_str(), GENERIC_WRITE, (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
					// Если файл открыт нормально
					if(file.valid()){
						/**
						 * Выполняем сброс записанного из ядра на носитель
						 *
						 * @note Разделения на данные и сведения о файле у MS Windows нет:
						 *       `FlushFileBuffers` сбрасывает и то, и другое, оттого признак
						 *       `durable` здесь ничего не меняет
						 */
						result = (::FlushFileBuffers(file) != FALSE);
						// Если сброс отвечен отказом
						if(!result){
							/**
							 * Если отказ настоящий
							 *
							 * @note Так отвечают канал и устройство посимвольное: сбрасывать
							 *       у них нечего, и запись от того мимо файла не ушла
							 */
							if(!(result = (::GetLastError() == ERROR_INVALID_FUNCTION))){
								// Создаём буфер сообщения ошибки
								wchar_t message[0xFF] = {0};
								// Выполняем формирование текста ошибки
								::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::GetLastError(), 0, message, 0xFF, 0);
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									log::debug(L"%s", __PRETTY_FUNCTION__, {filename, durable}, log::flag_t::CRITICAL, message);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									log::print(L"%s", log::flag_t::CRITICAL, message);
								#endif
							}
						}
					/**
					 * Если открыть файл не удалось
					 */
					} else {
						// Создаём буфер сообщения ошибки
						wchar_t message[0xFF] = {0};
						// Выполняем формирование текста ошибки
						::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::GetLastError(), 0, message, 0xFF, 0);
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							log::debug(L"%s", __PRETTY_FUNCTION__, {filename, durable}, log::flag_t::CRITICAL, message);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log::print(L"%s", log::flag_t::CRITICAL, message);
						#endif
					}
				/**
				 * Для операционной системы не являющейся MS Windows
				 */
				#else
					// Если объект файла ещё не заведён
					if(!file.valid())
						/**
						 * @brief Открываем файл на запись
						 *
						 * @note Сброс требует права записи: описатель, открытый на одно лишь
						 *       чтение, отвечает отказом `EBADF` у ряда систем
						 */
						file.set(::open(address.c_str(), O_WRONLY));
					// Если файл открыт нормально
					if(file.valid()){
						/**
						 * Если целевая платформа является macOS
						 *
						 * @details `fsync` там выносит записанное из ядра в НАКОПИТЕЛЬ, но опустошить
						 *          вместилище самого накопителя не велит, и обрыв питания записанное
						 *          теряет. Доводит до пластины лишь `F_FULLFSYNC`, и заведён он ровно для этого.
						 *
						 * @note Отказ `F_FULLFSYNC` откатывается к `fsync`, а не объявляется отказом:
						 *       управление им держит не всякая файловая система
						 */
						#ifdef __APPLE__
							// Выполняем сброс записанного из ядра на носитель
							const int32_t flushed = (durable ?
								((::fcntl(file, F_FULLFSYNC, 0) == -1) ? ::fsync(file) : 0) :
								::fsync(file)
							);
						/**
						 * Если целевая платформа держит сброс одних лишь данных
						 *
						 * @note `fdatasync` не сбрасывает сведений о файле, если размер его не менялся,
						 *       и тем обходится дешевле. Долговечности он не обещает, оттого стоит
						 *       ЛИШЬ под снятым признаком `durable`
						 */
						#elif _POSIX_SYNCHRONIZED_IO && (_POSIX_SYNCHRONIZED_IO > 0)
							// Выполняем сброс записанного из ядра на носитель
							const int32_t flushed = (durable ? ::fsync(file) : ::fdatasync(file));
						/**
						 * Для прочих операционных систем
						 */
						#else
							// Выполняем сброс записанного из ядра на носитель
							const int32_t flushed = ::fsync(file);
						#endif
						// Выполняем проверку исхода сброса
						result = (flushed == 0);
						// Если сброс отвечен отказом
						if(!result){
							/**
							 * Если отказ настоящий
							 *
							 * @note Так отвечают канал и устройство посимвольное: сбрасывать
							 *       у них нечего, и запись от того мимо файла не ушла
							 */
							if(!(result = (errno == EINVAL))){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									log::debug("%s", __PRETTY_FUNCTION__, {filename, durable}, log::flag_t::CRITICAL, ::strerror(errno));
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									log::print("%s", log::flag_t::CRITICAL, ::strerror(errno));
								#endif
							}
						}
					/**
					 * Если открыть файл не удалось
					 */
					} else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							log::debug("%s", __PRETTY_FUNCTION__, {filename, durable}, log::flag_t::CRITICAL, ::strerror(errno));
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log::print("%s", log::flag_t::CRITICAL, ::strerror(errno));
						#endif
					}
				#endif
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {filename, durable}, log::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Шаблон метода добавления в файл бинарных данных
 *
 * @tparam T тип буфера данных
 *
 */
template <typename T>
/**
 * @brief Метод добавления в файл бинарных данных
 *
 * @param filename путь к файлу в который необходимо выполнить запись
 * @param buffer   бинарный буфер который необходимо записать в файл
 * @param handle   внешний объект файла, если необходима поддержка пакетной обработки
 * @return         признак того, легли ли данные в файл
 *
 */
bool awh::Filesystem::append(string_view filename, const T & buffer, const handle_file_t & handle) const noexcept {
	// Если буфер данных передан
	if(!filename.empty()){
		// Если тип буфера является строкой
		if constexpr (is_same_v <T, string>)
			// Выводим признак того, легли ли данные в файл
			return this->append(filename, buffer.data(), buffer.size(), handle);
		// Если тип буфера является строкой символов
		else if constexpr (is_same_v <T, wstring>) {
			// Выполняем конвертацию строки
			const string & data = fmk::convert(buffer);
			// Выводим признак того, легли ли данные в файл
			return this->append(filename, data.c_str(), data.size(), handle);
		// Если тип буфера является вектором символов
		} else if constexpr (is_same_v <T, vector <char>>)
			// Выводим признак того, легли ли данные в файл
			return this->append(filename, buffer.data(), buffer.size(), handle);
		// Если тип буфера является вектором байтов
		else if constexpr (is_same_v <T, vector <uint8_t>>)
			// Выводим признак того, легли ли данные в файл
			return this->append(filename, buffer.data(), buffer.size(), handle);
	}
	/**
	 * Выводим отказ
	 *
	 * @note Сюда управление доходит ЛИШЬ когда доводы работу не прошли: адрес пуст
	 *       либо буфер пуст. Записи не было вовсе, и отвечать успехом здесь нельзя
	 */
	return false;
}
/**
 * @brief Явный специализированный шаблон метода добавления строки в текстовый файл
 *
 */
template bool awh::Filesystem::append(string_view, const string &, const handle_file_t &) const noexcept;
/**
 * @brief Явный специализированный шаблон метода добавления строки wide символов в текстовый файл
 *
 */
template bool awh::Filesystem::append(string_view, const wstring &, const handle_file_t &) const noexcept;
/**
 * @brief Явный специализированный шаблон метода добавления буфера символов в текстовый файл
 *
 */
template bool awh::Filesystem::append(string_view, const vector <char> &, const handle_file_t &) const noexcept;
/**
 * @brief Явный специализированный шаблон метода добавления буфера байтов в файл бинарных данных
 *
 */
template bool awh::Filesystem::append(string_view, const vector <uint8_t> &, const handle_file_t &) const noexcept;
/**
 * @brief Метод добавления в файл бинарных данных
 *
 * @param filename путь к файлу в который необходимо выполнить запись
 * @param buffer   бинарный буфер который необходимо записать в файл
 * @param handle   внешний объект файла, если необходима поддержка пакетной обработки
 * @return         признак того, легли ли данные в файл
 *
 */
bool awh::Filesystem::append(string_view filename, const char * buffer, const handle_file_t & handle) const noexcept {
	// Если буфер данных передан
	if(!filename.empty() && (buffer != nullptr) && ((* buffer) != '\0'))
		// Выводим признак того, легли ли данные в файл
		return this->append(filename, buffer, ::strlen(buffer), handle);
	/**
	 * Выводим отказ
	 *
	 * @note Сюда управление доходит ЛИШЬ когда доводы работу не прошли: адрес пуст
	 *       либо буфер пуст. Записи не было вовсе, и отвечать успехом здесь нельзя
	 */
	return false;
}
/**
 * @brief Метод добавления в файл бинарных данных
 *
 * @param filename путь к файлу в который необходимо выполнить запись
 * @param buffer   бинарный буфер который необходимо записать в файл
 * @param handle   внешний объект файла, если необходима поддержка пакетной обработки
 * @return         признак того, легли ли данные в файл
 *
 */
bool awh::Filesystem::append(string_view filename, const wchar_t * buffer, const handle_file_t & handle) const noexcept {
	// Если буфер данных передан
	if(!filename.empty() && (buffer != nullptr) && ((* buffer) != L'\0')){
		// Выполняем конвертацию строки
		const string & data = fmk::convert(buffer);
		// Выводим признак того, легли ли данные в файл
		return this->append(filename, data.c_str(), data.size(), handle);
	}
	/**
	 * Выводим отказ
	 *
	 * @note Сюда управление доходит ЛИШЬ когда доводы работу не прошли: адрес пуст
	 *       либо буфер пуст. Записи не было вовсе, и отвечать успехом здесь нельзя
	 */
	return false;
}
/**
 * @brief Метод добавления в файл бинарных данных
 *
 * @param filename путь к файлу в который необходимо выполнить запись
 * @param buffer   бинарный буфер который необходимо записать в файл
 * @param size     размер бинарного буфера для записи в файл
 * @param handle   внешний объект файла, если необходима поддержка пакетной обработки
 * @return         признак того, легли ли данные в файл
 *
 */
bool awh::Filesystem::append(string_view filename, const void * buffer, const size_t size, const handle_file_t & handle) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если параметры для записи переданы
	if(!filename.empty() && (buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * @brief Свой объект файла
			 *
			 * @note Заводится он всегда, но в дело идёт ЛИШЬ когда внешний объект не передан.
			 *       Так путь работы выходит один на оба случая, а закрытие описателя остаётся
			 *       делом самого объекта: свой гибнет с концом области видимости, внешний -
			 *       вместе с умным указателем у того, кто его завёл
			 */
			HandleFile local;
			// Объект файла, которым идёт работа
			HandleFile & file = (handle != nullptr ? (* handle) : local);
			/**
			 * Выполняем извлечение актуального значения адреса
			 *
			 * @note Извлекается он ЛИШЬ когда объект файла ещё не заведён: адрес идёт только
			 *       в открытие, а при уже открытом описателе не нужен вовсе. Разбор пути стоит
			 *       перехода в ядро (`realpath` под POSIX, `GetFullPathName` под MS Windows),
			 *       и при потоковой работе мелкими долями плата эта ложилась бы на всякую долю
			 */
			const string address = (!file.valid() ? this->fullpath(filename, true) : "");
			// Если объект файла заведён либо адрес получен правильный
			if(file.valid() || (!address.empty())){
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					// Если объект файла ещё не заведён
					if(!file.valid())
						/**
						 * @brief Инициализируем новый объект файла
						 *
						 * @note Дозволяется и запись, и удаление, а не одно лишь чтение: файл вправе
						 *       держать открытым кто-то ещё - движок наблюдения за файловой системой
						 *       держит его именно так, - и обращение с одним лишь дозволением чтения
						 *       отвечало бы отказом ERROR_SHARING_VIOLATION. Отказ этот молчаливый:
						 *       дозапись уходила бы мимо файла, а размер выдавался бы нулевым
						 */
						file.set(::CreateFileW(fmk::convert(address).c_str(), FILE_APPEND_DATA, (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
					// Если файл открыт нормально
					if(file.valid()){
						// Число октетов, легших в файл
						DWORD written = 0;
						/**
						 * Выполняем добавление данных в файл
						 *
						 * @note Успехом считается ЛИШЬ запись всего буфера: `WriteFile` вправе
						 *       лечь частью, и частичная запись это тот же отказ - хвост буфера
						 *       до файла не дошёл
						 */
						result = ((::WriteFile(file, static_cast <LPCVOID> (buffer), static_cast <DWORD> (size), &written, nullptr) != FALSE) && (static_cast <size_t> (written) == size));
					}
					/**
					 * Если открыть файл не удалось
					 *
					 * @note Отказ этот обязан быть слышен: дозапись, ушедшая мимо файла,
					 *       ничем себя иначе не выдаёт - вызывающий об отказе не узнаёт
					 *       вовсе, а файл остаётся прежним
					 */
					else {
						// Создаём буфер сообщения ошибки
						wchar_t message[0xFF] = {0};
						// Выполняем формирование текста ошибки
						::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::GetLastError(), 0, message, 0xFF, 0);
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							log::debug(L"%s", __PRETTY_FUNCTION__, {filename, buffer, size}, log::flag_t::CRITICAL, message);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log::print(L"%s", log::flag_t::CRITICAL, message);
						#endif
					}
				/**
				 * Для операционной системы не являющейся MS Windows
				 */
				#else
					// Если объект файла ещё не заведён
					if(!file.valid())
						/**
						 * Выполняем открытие файла на дозапись
						 *
						 * @note Ход этот равен `FILE_APPEND_DATA` с `OPEN_ALWAYS` у ветви Windows: отсутствующий
						 *       файл заводится, существующий открывается как есть, а всякая запись ложится в конец
						 *       файла ядром, минуя текущую позицию. Свойство это принадлежит самому описателю,
						 *       оттого при пакетной обработке внешним объектом файла дозапись остаётся дозаписью
						 *       и после любого стороннего смещения позиции
						 */
						file.set(::open(address.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644));
					/**
					 * Если файл открыть не удалось
					 *
					 * @note Отказ этот обязан быть слышен: дозапись, ушедшая мимо файла,
					 *       ничем себя иначе не выдаёт - вызывающий об отказе не узнаёт
					 *       вовсе, а файл остаётся прежним
					 */
					if(!file.valid()){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							log::debug("%s", __PRETTY_FUNCTION__, {filename, buffer, size}, log::flag_t::CRITICAL, ::strerror(errno));
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log::print("%s", log::flag_t::CRITICAL, ::strerror(errno));
						#endif
					// Если файл открыт нормально
					} else {
						/**
						 * Если добавление данных в файл отвечено отказом
						 *
						 * @note Сличение идёт с полным размером буфера, а не с нулём: запись
						 *       вправе лечь частью, и частичная запись это тот же отказ - хвост
						 *       буфера до файла не дошёл
						 */
						if(!(result = (::write(file, buffer, size) == static_cast <ssize_t> (size)))){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log::debug("%s", __PRETTY_FUNCTION__, {filename, buffer, size}, log::flag_t::CRITICAL, ::strerror(errno));
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								log::print("%s", log::flag_t::CRITICAL, ::strerror(errno));
							#endif
						}
					}
				#endif
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const ios_base::failure & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {filename, buffer, size}, log::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, error.what());
			#endif
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {filename, buffer, size}, log::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Шаблон метода чтения данных из файла
 *
 * @tparam T тип возвращаемого результата
 *
 */
template <typename T>
/**
 * @brief Метод чтения данных из файла
 *
 * @param filename путь к файлу для чтения
 * @param seek     тип смещения в файле
 * @param offset   смещение в файле
 * @param handle   внешний объект файла, если необходима поддержка пакетной обработки
 * @return         бинарный буфер с прочитанными данными
 *
 */
auto awh::Filesystem::read(string_view filename, const seek_t seek, const size_t offset, const handle_file_t & handle) const noexcept -> T {
	// Переменная результата
	T result;
	// Если буфер данных передан
	if(!filename.empty())
		// Выполняем чтение данных из файла
		this->read(filename, result, seek, offset, handle);
	// Возвращаем результат
	return result;
}
/**
 * @brief Явный специализированный шаблон метода чтения данных из файла в строку
 *
 */
template string awh::Filesystem::read(string_view, const seek_t, const size_t, const handle_file_t &) const noexcept;
/**
 * @brief Явный специализированный шаблон метода чтения данных из файла в буфер символов
 *
 */
template vector <char> awh::Filesystem::read(string_view, const seek_t, const size_t, const handle_file_t &) const noexcept;
/**
 * @brief Явный специализированный шаблон метода чтения данных из файла в буфер бинарных данных
 *
 */
template vector <uint8_t> awh::Filesystem::read(string_view, const seek_t, const size_t, const handle_file_t &) const noexcept;
/**
 * @brief Шаблон метода чтения данных из файла
 *
 * @tparam T тип возвращаемого результата
 *
 */
template <typename T>
/**
 * @brief Метод чтения данных из файла
 *
 * @param filename путь к файлу для чтения
 * @param result   контейнер куда следует положить результат
 * @param seek     тип смещения в файле
 * @param offset   смещение в файле
 * @param handle   внешний объект файла, если необходима поддержка пакетной обработки
 *
 */
void awh::Filesystem::read(string_view filename, T & result, const seek_t seek, const size_t offset, const handle_file_t & handle) const noexcept {
	// Если путь к файлу указан и он существует
	if(!filename.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * @brief Свой объект файла
			 *
			 * @note Заводится он всегда, но в дело идёт ЛИШЬ когда внешний объект не передан.
			 *       Так путь работы выходит один на оба случая, а закрытие описателя остаётся
			 *       делом самого объекта: свой гибнет с концом области видимости, внешний -
			 *       вместе с умным указателем у того, кто его завёл
			 */
			HandleFile local;
			// Объект файла, которым идёт работа
			HandleFile & file = (handle != nullptr ? (* handle) : local);
			/**
			 * Выполняем извлечение актуального значения адреса
			 *
			 * @note Извлекается он ЛИШЬ когда объект файла ещё не заведён: адрес идёт только
			 *       в открытие, а при уже открытом описателе не нужен вовсе. Разбор пути стоит
			 *       перехода в ядро (`realpath` под POSIX, `GetFullPathName` под MS Windows),
			 *       и при потоковой работе мелкими долями плата эта ложилась бы на всякую долю
			 */
			const string address = (!file.valid() ? this->fullpath(filename, true) : "");
			// Если объект файла заведён либо адрес получен правильный
			if(file.valid() || (!address.empty() && (this->type(address) == type_t::FILE))){
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					// Если объект файла ещё не заведён
					if(!file.valid())
						/**
						 * @brief Инициализируем новый объект файла
						 *
						 * @note Дозволяется и запись, и удаление, а не одно лишь чтение: файл вправе
						 *       держать открытым кто-то ещё - движок наблюдения за файловой системой
						 *       держит его именно так, - и обращение с одним лишь дозволением чтения
						 *       отвечало бы отказом ERROR_SHARING_VIOLATION. Отказ этот молчаливый:
						 *       дозапись уходила бы мимо файла, а размер выдавался бы нулевым
						 */
						file.set(::CreateFileW(fmk::convert(address).c_str(), GENERIC_READ, (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
					// Если файл открыт нормально
					if(file.valid()){
						// Создаём объект большого числа
						LARGE_INTEGER li;
						// Устанавливаем начальное значение позиции
						li.QuadPart = static_cast <LONGLONG> (offset);
						/**
						 * Определяем тип смещения в файле события
						 */
						switch(static_cast <uint8_t> (seek)){
							// Если смещение от начала файла
							case static_cast <uint8_t> (seek_t::BEGIN):
								// Выполняем установку позиции в файле
								li.LowPart = ::SetFilePointer(file, li.LowPart, &li.HighPart, FILE_BEGIN);
							break;
							// Если смещение от текущей позиции в файле
							case static_cast <uint8_t> (seek_t::CURRENT):
								// Выполняем установку позиции в файле
								li.LowPart = ::SetFilePointer(file, li.LowPart, &li.HighPart, FILE_CURRENT);
							break;
							// Если смещение от конца файла
							case static_cast <uint8_t> (seek_t::END):
								// Выполняем установку позиции в файле
								li.LowPart = ::SetFilePointer(file, li.LowPart, &li.HighPart, FILE_END);
							break;
							// Если тип смещения не определён
							default: li.LowPart = 0;
						}
						// Если мы получили ошибку установки позиции
						if((li.LowPart == INVALID_SET_FILE_POINTER) && (::GetLastError() != NO_ERROR))
							// Сбрасываем значение установленной позиции
							li.QuadPart = -1;
						// Если позиция установлена успешно
						if(li.QuadPart > -1){
							// Объект для хранения полного размера файла
							LARGE_INTEGER fileSize;
							// Если размер файла получить не удалось либо смещение находится за пределами файла
							if(!::GetFileSizeEx(file, &fileSize) || (offset >= static_cast <size_t> (fileSize.QuadPart)))
								// Выходим из метода (дескриптор будет закрыт автоматически)
								return;
							// Определяем размер читаемых данных
							size_t size = (static_cast <size_t> (fileSize.QuadPart) - offset);
							// Если объект результата пустой
							if(result.empty())
								// Устанавливаем размер буфера
								result.resize(size);
							// Если объект результата уже задан — читаем min(размер буфера, size)
							else size = ::min(size, result.size());
							// Выполняем чтение из файла в буфер данные
							if(!::ReadFile(file, static_cast <LPVOID> (&result[0]), static_cast <DWORD> (size), 0, nullptr)){
								/**
								 * Выполняем очистку буфера результата
								 *
								 * @note Отказ чтения обязан оставлять ПУСТО, а не размеченный нулями
								 *       буфер: вид возврата у работы пуст, и отличить прочитанное от
								 *       неудавшегося вызывающий может лишь по пустоте результата
								 */
								result.clear();
								// Создаём буфер сообщения ошибки
								wchar_t message[0xFF] = {0};
								// Выполняем формирование текста ошибки
								::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 0xFF, 0);
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									log::debug(L"%s", __PRETTY_FUNCTION__, {filename, static_cast <uint16_t> (seek), result.size(), offset}, log::flag_t::CRITICAL, message);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									log::print(L"%s", log::flag_t::CRITICAL, message);
								#endif
							}
						}
					}
				/**
				 * Для операционной системы не являющейся MS Windows
				 */
				#else
					// Структура статистики файла
					struct stat info{};
					// Если объект файла ещё не заведён
					if(!file.valid())
						// Создаём объект файлового дескриптора
						file.set(::open(address.c_str(), O_RDONLY));
					// Если файл не открыт
					if(!file.valid()){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							log::debug("%s", __PRETTY_FUNCTION__, {filename, static_cast <uint16_t> (seek), result.size(), offset}, log::flag_t::CRITICAL, ::strerror(errno));
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log::print("%s", log::flag_t::CRITICAL, ::strerror(errno));
						#endif
					// Если файл открыт удачно
					} else if(::fstat(file, &info) < 0) {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							log::debug("%s", __PRETTY_FUNCTION__, {filename, static_cast <uint16_t> (seek), result.size(), offset}, log::flag_t::CRITICAL, ::strerror(errno));
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log::print("%s", log::flag_t::CRITICAL, ::strerror(errno));
						#endif
					// Если размер файла изменился
					} else if(static_cast <size_t> (info.st_size) > offset) {
						// Позиция в файле
						off_t position = 0;
						/**
						 * Определяем тип смещения в файле события
						 */
						switch(static_cast <uint8_t> (seek)){
							// Если смещение от начала файла
							case static_cast <uint8_t> (seek_t::BEGIN):
								// Выполняем расчёт смещения в файле
								position = static_cast <off_t> (offset);
							break;
							// Если смещение от конца файла
							case static_cast <uint8_t> (seek_t::END):
								// Выполняем расчёт смещения в файле
								position = (static_cast <off_t> (info.st_size) - static_cast <off_t> (offset));
							break;
						}
						// Проверяем границы
						if(position < 0)
							// Устанавливаем позицию в начало файла
							position = 0;
						// Если позиция выше размера файла
						if(position >= static_cast <off_t> (info.st_size))
							// Выходим из метода
							return;
						// Определяем размер читаемых данных
						size_t size = (static_cast <size_t> (info.st_size) - static_cast <size_t> (position));
						// Если объект результата пустой
						if(result.empty())
							// Устанавливаем размер буфера
							result.resize(size);
						// Если объект результата уже задан — читаем min(размер буфера, size)
						else size = ::min(size, result.size());
						// Читаем данные из файла в буфер
						if(::pread(file, &result[0], size, position) != static_cast <ssize_t> (size)){
							/**
							 * Выполняем очистку буфера результата
							 *
							 * @note Отказ чтения обязан оставлять ПУСТО, а не размеченный нулями
							 *       буфер: вид возврата у работы пуст, и отличить прочитанное от
							 *       неудавшегося вызывающий может лишь по пустоте результата
							 */
							result.clear();
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log::debug("%s", __PRETTY_FUNCTION__, {filename, static_cast <uint16_t> (seek), result.size(), offset}, log::flag_t::CRITICAL, ::strerror(errno));
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем в лог сообщение что прочитать файл не удалось
								log::print("%s", log::flag_t::CRITICAL, ::strerror(errno));
							#endif
						}
					}
				#endif
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const ios_base::failure & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {filename, static_cast <uint16_t> (seek), offset}, log::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, error.what());
			#endif
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {filename, static_cast <uint16_t> (seek), offset}, log::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Явный специализированный шаблон метода чтения данных из файла в строку
 *
 */
template void awh::Filesystem::read(string_view, string &, const seek_t, const size_t, const handle_file_t &) const noexcept;
/**
 * @brief Явный специализированный шаблон метода чтения данных из файла в буфер символов
 *
 */
template void awh::Filesystem::read(string_view, vector <char> &, const seek_t, const size_t, const handle_file_t &) const noexcept;
/**
 * @brief Явный специализированный шаблон метода чтения данных из файла в буфер бинарных данных
 *
 */
template void awh::Filesystem::read(string_view, vector <uint8_t> &, const seek_t, const size_t, const handle_file_t &) const noexcept;
/**
 * @brief Метод рекурсивного чтения больших файлов блоками с обратным вызовом
 *
 * @param filename путь к файлу для чтения
 * @param size     размер блока для чтения
 * @param callback функция обратного вызова для обработки прочитанных данных (возвращает true для продолжения чтения и false для остановки)
 * @param offset   смещение в файле с которого следует начать чтение
 * @param handle   внешний объект файла, если необходима поддержка пакетной обработки
 *
 */
void awh::Filesystem::read(string_view filename, const size_t size, const function <bool (const void * buffer, const size_t size, const size_t offset, const size_t left)> & callback, const size_t offset, const handle_file_t & handle) const noexcept {
	// Если буфер данных передан
	if(!filename.empty() && (size > 0) && (callback != nullptr)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * @brief Свой объект файла
			 *
			 * @note Заводится он всегда, но в дело идёт ЛИШЬ когда внешний объект не передан.
			 *       Так путь работы выходит один на оба случая, а закрытие описателя остаётся
			 *       делом самого объекта: свой гибнет с концом области видимости, внешний -
			 *       вместе с умным указателем у того, кто его завёл
			 */
			HandleFile local;
			// Объект файла, которым идёт работа
			HandleFile & file = (handle != nullptr ? (* handle) : local);
			/**
			 * Выполняем извлечение актуального значения адреса
			 *
			 * @note Извлекается он ЛИШЬ когда объект файла ещё не заведён: адрес идёт только
			 *       в открытие, а при уже открытом описателе не нужен вовсе. Разбор пути стоит
			 *       перехода в ядро (`realpath` под POSIX, `GetFullPathName` под MS Windows),
			 *       и при потоковой работе мелкими долями плата эта ложилась бы на всякую долю
			 */
			const string address = (!file.valid() ? this->fullpath(filename, true) : "");
			// Если объект файла заведён либо адрес получен правильный
			if(file.valid() || (!address.empty() && (this->type(address) == type_t::FILE))){
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					// Если объект файла ещё не заведён
					if(!file.valid())
						/**
						 * @brief Инициализируем новый объект файла
						 *
						 * @note Дозволяется и запись, и удаление, а не одно лишь чтение: файл вправе
						 *       держать открытым кто-то ещё - движок наблюдения за файловой системой
						 *       держит его именно так, - и обращение с одним лишь дозволением чтения
						 *       отвечало бы отказом ERROR_SHARING_VIOLATION. Отказ этот молчаливый:
						 *       дозапись уходила бы мимо файла, а размер выдавался бы нулевым
						 */
						file.set(::CreateFileW(fmk::convert(address).c_str(), GENERIC_READ, (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
					// Если файл открыт нормально
					if(file.valid()){
						// Объект для хранения полного размера файла
						LARGE_INTEGER fileSize;
						// Если размер файла получить не удалось
						if(!::GetFileSizeEx(file, &fileSize)){
							// Создаём буфер сообщения ошибки
							wchar_t message[0xFF] = {0};
							// Выполняем формирование текста ошибки
							::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::GetLastError(), 0, message, 0xFF, 0);
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log::debug(L"%s", __PRETTY_FUNCTION__, {filename, size, offset}, log::flag_t::CRITICAL, message);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								log::print(L"%s", log::flag_t::CRITICAL, message);
							#endif
							// Выходим из метода (дескриптор будет закрыт автоматически)
							return;
						}
						// Если файл не пустой (для пустого файла проекция создать невозможно)
						if(fileSize.QuadPart > 0){
							// Создаём объект проекции файла в память в режиме только для чтения
							HandleFile mapping(::CreateFileMappingW(file, nullptr, PAGE_READONLY, 0, 0, nullptr));
							// Если создать проекцию файла не удалось
							if(!mapping.valid()){
								// Создаём буфер сообщения ошибки
								wchar_t message[0xFF] = {0};
								// Выполняем формирование текста ошибки
								::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::GetLastError(), 0, message, 0xFF, 0);
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									log::debug(L"%s", __PRETTY_FUNCTION__, {filename, size, offset}, log::flag_t::CRITICAL, message);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									log::print(L"%s", log::flag_t::CRITICAL, message);
								#endif
								// Выходим из метода (дескрипторы будут закрыты автоматически)
								return;
							}
							// Отображаем весь файл в адресное пространство процесса (zero-copy)
							const void * addr = ::MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
							// Если отобразить проекцию файла не удалось
							if(addr == nullptr){
								// Создаём буфер сообщения ошибки
								wchar_t message[0xFF] = {0};
								// Выполняем формирование текста ошибки
								::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::GetLastError(), 0, message, 0xFF, 0);
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									log::debug(L"%s", __PRETTY_FUNCTION__, {filename, size, offset}, log::flag_t::CRITICAL, message);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									log::print(L"%s", log::flag_t::CRITICAL, message);
								#endif
								// Выходим из метода (дескрипторы будут закрыты автоматически)
								return;
							}
							// Общий размер файла
							const size_t total = static_cast <size_t> (fileSize.QuadPart);
							/**
							 * Перебираем файл блоками, передавая указатели напрямую из проекции
							 */
							for(size_t position = offset; position < total; position += size){
								// Определяем размер текущего блока
								const size_t bytes = ::min(size, (total - position));
								// Определяем размер оставшихся данных после текущего блока
								const size_t left = (total - position - bytes);
								// Передаём указатель прямо из проекции файла без копирования данных
								if(!callback(static_cast <const uint8_t *> (addr) + position, bytes, position, left))
									// Прерываем чтение файла по запросу вызывающей стороны
									break;
							}
							// Освобождаем проекцию файла из адресного пространства процесса
							::UnmapViewOfFile(addr);
						}
					}
				/**
				 * Для операционной системы не являющейся MS Windows
				 */
				#else
					// Структура статистики файла
					struct stat info{};
					// Если объект файла ещё не заведён
					if(!file.valid())
						// Создаём объект файлового дескриптора
						file.set(::open(address.c_str(), O_RDONLY));
					// Если файл не открыт
					if(!file.valid()){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							log::debug("%s", __PRETTY_FUNCTION__, {filename, size, offset}, log::flag_t::CRITICAL, ::strerror(errno));
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log::print("%s", log::flag_t::CRITICAL, ::strerror(errno));
						#endif
					// Если получить статистику файла не удалось
					} else if(::fstat(file, &info) < 0) {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							log::debug("%s", __PRETTY_FUNCTION__, {filename, size, offset}, log::flag_t::CRITICAL, ::strerror(errno));
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log::print("%s", log::flag_t::CRITICAL, ::strerror(errno));
						#endif
					// Если файл не пустой (пустой файл проецировать в память нельзя)
					} else if(info.st_size > 0) {
						// Общий размер файла
						const size_t total = static_cast <size_t> (info.st_size);
						// Отображаем весь файл в адресное пространство процесса (zero-copy)
						void * addr = ::mmap(nullptr, total, PROT_READ, MAP_PRIVATE, file, 0);
						// Если отобразить проекцию файла не удалось
						if(addr == MAP_FAILED){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log::debug("%s", __PRETTY_FUNCTION__, {filename, size, offset}, log::flag_t::CRITICAL, ::strerror(errno));
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								log::print("%s", log::flag_t::CRITICAL, ::strerror(errno));
							#endif
						// Если проекция файла создана удачно
						} else {
							/**
							 * Подсказываем ядру о последовательном характере чтения данных
							 */
							#if defined(MADV_SEQUENTIAL)
								// Активируем упреждающее чтение страниц проекции файла
								::madvise(addr, total, MADV_SEQUENTIAL);
							#endif
							/**
							 * Перебираем файл блоками, передавая указатели напрямую из проекции
							 */
							for(size_t position = offset; position < total; position += size){
								// Определяем размер текущего блока
								const size_t bytes = ::min(size, (total - position));
								// Определяем размер оставшихся данных после текущего блока
								const size_t left = (total - position - bytes);
								// Передаём указатель прямо из проекции файла без копирования данных
								if(!callback(static_cast <const uint8_t *> (addr) + position, bytes, position, left))
									// Прерываем чтение файла по запросу вызывающей стороны
									break;
							}
							// Освобождаем проекцию файла из адресного пространства процесса
							::munmap(addr, total);
						}
					}
				#endif
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const ios_base::failure & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {filename, size, offset}, log::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, error.what());
			#endif
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {filename, size, offset}, log::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Шаблон метода записи в файл бинарных данных
 *
 * @tparam T тип буфера данных
 *
 */
template <typename T>
/**
 * @brief Метод записи в файл бинарных данных
 *
 * @param filename путь к файлу в который необходимо выполнить запись
 * @param buffer   бинарный буфер который необходимо записать в файл
 * @param seek     тип смещения в файле
 * @param offset   смещение в файле
 * @param handle   внешний объект файла, если необходима поддержка пакетной обработки
 * @return         признак того, легли ли данные в файл
 *
 */
bool awh::Filesystem::write(string_view filename, const T & buffer, const seek_t seek, const size_t offset, const handle_file_t & handle) const noexcept {
	// Если буфер данных передан
	if(!filename.empty()){
		// Если тип буфера является строкой
		if constexpr (is_same_v <T, string>)
			// Выводим признак того, легли ли данные в файл
			return this->write(filename, buffer.data(), buffer.size(), seek, offset, handle);
		// Если тип буфера является строкой символов
		else if constexpr (is_same_v <T, wstring>) {
			// Выполняем конвертацию строки
			const string & data = fmk::convert(buffer);
			// Выводим признак того, легли ли данные в файл
			return this->write(filename, data.c_str(), data.size(), seek, offset, handle);
		// Если тип буфера является вектором символов
		} else if constexpr (is_same_v <T, vector <char>>)
			// Выводим признак того, легли ли данные в файл
			return this->write(filename, buffer.data(), buffer.size(), seek, offset, handle);
		// Если тип буфера является вектором байтов
		else if constexpr (is_same_v <T, vector <uint8_t>>)
			// Выводим признак того, легли ли данные в файл
			return this->write(filename, buffer.data(), buffer.size(), seek, offset, handle);
	}
	/**
	 * Выводим отказ
	 *
	 * @note Сюда управление доходит ЛИШЬ когда доводы работу не прошли: адрес пуст
	 *       либо буфер пуст. Записи не было вовсе, и отвечать успехом здесь нельзя
	 */
	return false;
}
/**
 * @brief Явный специализированный шаблон метода записи в файл бинарных данных из строки
 *
 */
template bool awh::Filesystem::write(string_view, const string &, const seek_t, const size_t, const handle_file_t &) const noexcept;
/**
 * @brief Явный специализированный шаблон метода записи в файл бинарных данных из строки wide символов
 *
 */
template bool awh::Filesystem::write(string_view, const wstring &, const seek_t, const size_t, const handle_file_t &) const noexcept;
/**
 * @brief Явный специализированный шаблон метода записи в файл бинарных данных из буфера символов
 *
 */
template bool awh::Filesystem::write(string_view, const vector <char> &, const seek_t, const size_t, const handle_file_t &) const noexcept;
/**
 * @brief Явный специализированный шаблон метода записи в файл бинарных данных из буфера бинарных данных
 *
 */
template bool awh::Filesystem::write(string_view, const vector <uint8_t> &, const seek_t, const size_t, const handle_file_t &) const noexcept;
/**
 * @brief Метод записи в файл бинарных данных
 *
 * @param filename путь к файлу в который необходимо выполнить запись
 * @param buffer   бинарный буфер который необходимо записать в файл
 * @param seek     тип смещения в файле
 * @param offset   смещение в файле
 * @param handle   внешний объект файла, если необходима поддержка пакетной обработки
 * @return         признак того, легли ли данные в файл
 *
 */
bool awh::Filesystem::write(string_view filename, const char * buffer, const seek_t seek, const size_t offset, const handle_file_t & handle) const noexcept {
	// Если буфер данных передан
	if(!filename.empty() && (buffer != nullptr) && ((* buffer) != '\0'))
		// Выводим признак того, легли ли данные в файл
		return this->write(filename, buffer, ::strlen(buffer), seek, offset, handle);
	/**
	 * Выводим отказ
	 *
	 * @note Сюда управление доходит ЛИШЬ когда доводы работу не прошли: адрес пуст
	 *       либо буфер пуст. Записи не было вовсе, и отвечать успехом здесь нельзя
	 */
	return false;
}
/**
 * @brief Метод записи в файл бинарных данных
 *
 * @param filename путь к файлу в который необходимо выполнить запись
 * @param buffer   бинарный буфер который необходимо записать в файл
 * @param seek     тип смещения в файле
 * @param offset   смещение в файле
 * @param handle   внешний объект файла, если необходима поддержка пакетной обработки
 * @return         признак того, легли ли данные в файл
 *
 */
bool awh::Filesystem::write(string_view filename, const wchar_t * buffer, const seek_t seek, const size_t offset, const handle_file_t & handle) const noexcept {
	// Если буфер данных передан
	if(!filename.empty() && (buffer != nullptr) && ((* buffer) != L'\0')){
		// Выполняем конвертацию строки
		const string & data = fmk::convert(buffer);
		// Выводим признак того, легли ли данные в файл
		return this->write(filename, data.c_str(), data.size(), seek, offset, handle);
	}
	/**
	 * Выводим отказ
	 *
	 * @note Сюда управление доходит ЛИШЬ когда доводы работу не прошли: адрес пуст
	 *       либо буфер пуст. Записи не было вовсе, и отвечать успехом здесь нельзя
	 */
	return false;
}
/**
 * @brief Метод записи в файл бинарных данных
 *
 * @param filename путь к файлу в который необходимо выполнить запись
 * @param buffer   бинарный буфер который необходимо записать в файл
 * @param size     размер бинарного буфера для записи в файл
 * @param seek     тип смещения в файле
 * @param offset   смещение в файле
 * @param handle   внешний объект файла, если необходима поддержка пакетной обработки
 * @return         признак того, легли ли данные в файл
 *
 */
bool awh::Filesystem::write(string_view filename, const void * buffer, const size_t size, const seek_t seek, const size_t offset, const handle_file_t & handle) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если параметры для записи переданы
	if(!filename.empty() && (buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * @brief Свой объект файла
			 *
			 * @note Заводится он всегда, но в дело идёт ЛИШЬ когда внешний объект не передан.
			 *       Так путь работы выходит один на оба случая, а закрытие описателя остаётся
			 *       делом самого объекта: свой гибнет с концом области видимости, внешний -
			 *       вместе с умным указателем у того, кто его завёл
			 */
			HandleFile local;
			// Объект файла, которым идёт работа
			HandleFile & file = (handle != nullptr ? (* handle) : local);
			/**
			 * Выполняем извлечение актуального значения адреса
			 *
			 * @note Извлекается он ЛИШЬ когда объект файла ещё не заведён: адрес идёт только
			 *       в открытие, а при уже открытом описателе не нужен вовсе. Разбор пути стоит
			 *       перехода в ядро (`realpath` под POSIX, `GetFullPathName` под MS Windows),
			 *       и при потоковой работе мелкими долями плата эта ложилась бы на всякую долю
			 */
			const string address = (!file.valid() ? this->fullpath(filename, true) : "");
			// Если объект файла заведён либо адрес получен правильный
			if(file.valid() || (!address.empty())){
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					// Если объект файла ещё не заведён
					if(!file.valid())
						/**
						 * @brief Инициализируем новый объект файла
						 *
						 * @note Дозволяется и запись, и удаление, а не одно лишь чтение: файл вправе
						 *       держать открытым кто-то ещё - движок наблюдения за файловой системой
						 *       держит его именно так, - и обращение с одним лишь дозволением чтения
						 *       отвечало бы отказом ERROR_SHARING_VIOLATION. Отказ этот молчаливый:
						 *       дозапись уходила бы мимо файла, а размер выдавался бы нулевым
						 */
						/**
						 * @brief Открываем файл на чтение и запись
						 *
						 * @note Право чтения испрашивается ВМЕСТЕ с правом записи, хотя запись
						 *       сама по себе им не пользуется. Ход этот равен `O_RDWR` у ветви
						 *       POSIX и заведён ради одинаковости поведения: объект файла, заведённый
						 *       записью, обязан годиться и чтению на всякой системе. Испроси мы одно
						 *       лишь `GENERIC_WRITE`, чтение тем же объектом отвечало бы отказом
						 *       ERROR_ACCESS_DENIED под MS Windows и проходило бы под POSIX
						 */
						file.set(::CreateFileW(fmk::convert(address).c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
					// Если файл открыт нормально
					if(file.valid()){
						// Создаём объект большого числа
						LARGE_INTEGER li;
						// Устанавливаем начальное значение позиции
						li.QuadPart = static_cast <LONGLONG> (offset);
						/**
						 * Определяем тип смещения в файле события
						 */
						switch(static_cast <uint8_t> (seek)){
							// Если смещение от начала файла
							case static_cast <uint8_t> (seek_t::BEGIN):
								// Выполняем установку позиции в файле
								li.LowPart = ::SetFilePointer(file, li.LowPart, &li.HighPart, FILE_BEGIN);
							break;
							// Если смещение от текущей позиции в файле
							case static_cast <uint8_t> (seek_t::CURRENT):
								// Выполняем установку позиции в файле
								li.LowPart = ::SetFilePointer(file, li.LowPart, &li.HighPart, FILE_CURRENT);
							break;
							// Если смещение от конца файла
							case static_cast <uint8_t> (seek_t::END):
								// Выполняем установку позиции в файле
								li.LowPart = ::SetFilePointer(file, li.LowPart, &li.HighPart, FILE_END);
							break;
							// Если тип смещения не определён
							default: li.LowPart = 0;
						}
						// Если мы получили ошибку установки позиции
						if((li.LowPart == INVALID_SET_FILE_POINTER) && (::GetLastError() != NO_ERROR))
							// Сбрасываем значение установленной позиции
							li.QuadPart = -1;
						// Если позиция установлена успешно
						if(li.QuadPart > -1){
							// Число октетов, легших в файл
							DWORD written = 0;
							/**
							 * Выполняем запись данных в файл
							 *
							 * @note Успехом считается ЛИШЬ запись всего буфера: `WriteFile` вправе
							 *       лечь частью, и частичная запись это тот же отказ - хвост буфера
							 *       до файла не дошёл
							 */
							result = ((::WriteFile(file, static_cast <LPCVOID> (buffer), static_cast <DWORD> (size), &written, nullptr) != FALSE) && (static_cast <size_t> (written) == size));
						}
					/**
					 * Если открыть файл не удалось
					 *
					 * @note Отказ этот обязан быть слышен: запись, ушедшая мимо файла,
					 *       ничем себя иначе не выдаёт - вызывающий об отказе не узнаёт
					 *       вовсе, а файл остаётся прежним
					 */
					} else {
						// Создаём буфер сообщения ошибки
						wchar_t message[0xFF] = {0};
						// Выполняем формирование текста ошибки
						::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::GetLastError(), 0, message, 0xFF, 0);
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							log::debug(L"%s", __PRETTY_FUNCTION__, {filename, buffer, size, static_cast <uint16_t> (seek), offset}, log::flag_t::CRITICAL, message);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log::print(L"%s", log::flag_t::CRITICAL, message);
						#endif
					}
				/**
				 * Для операционной системы не являющейся MS Windows
				 */
				#else
					// Если объект файла ещё не заведён
					if(!file.valid())
						/**
						 * Выполняем открытие файла на запись, существующий НЕ усекая
						 *
						 * @note Ход этот равен `OPEN_ALWAYS` у ветви Windows: отсутствующий файл заводится,
						 *       существующий открывается как есть. Поток `ofstream` здесь не годится - вид
						 *       `out` усекает файл, а вид `in | out` отсутствующего не заводит вовсе
						 */
						file.set(::open(address.c_str(), O_RDWR | O_CREAT, 0644));
					// Если файл не открыт
					if(!file.valid()){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							log::debug("%s", __PRETTY_FUNCTION__, {filename, buffer, size, static_cast <uint16_t> (seek), offset}, log::flag_t::CRITICAL, ::strerror(errno));
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log::print("%s", log::flag_t::CRITICAL, ::strerror(errno));
						#endif
					// Если файл открыт нормально
					} else {
						/**
						 * Определяем тип смещения в файле события
						 */
						switch(static_cast <uint8_t> (seek)){
							// Если смещение от начала файла
							case static_cast <uint8_t> (seek_t::BEGIN):
								// Выполняем установку позиции записи
								::lseek(file, static_cast <off_t> (offset), SEEK_SET);
							break;
							// Если смещение от текущей позиции в файле
							case static_cast <uint8_t> (seek_t::CURRENT):
								// Выполняем установку позиции записи
								::lseek(file, static_cast <off_t> (offset), SEEK_CUR);
							break;
							// Если смещение от конца файла
							case static_cast <uint8_t> (seek_t::END):
								// Выполняем установку позиции записи
								::lseek(file, static_cast <off_t> (offset), SEEK_END);
							break;
						}
						/**
						 * Если запись данных в файл отвечена отказом
						 *
						 * @note Сличение идёт с полным размером буфера, а не с нулём: запись
						 *       вправе лечь частью, и частичная запись это тот же отказ - хвост
						 *       буфера до файла не дошёл
						 */
						if(!(result = (::write(file, buffer, size) == static_cast <ssize_t> (size)))){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log::debug("%s", __PRETTY_FUNCTION__, {filename, buffer, size, static_cast <uint16_t> (seek), offset}, log::flag_t::CRITICAL, ::strerror(errno));
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								log::print("%s", log::flag_t::CRITICAL, ::strerror(errno));
							#endif
						}
					}
				#endif
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const ios_base::failure & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {filename, buffer, size, static_cast <uint16_t> (seek), offset}, log::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, error.what());
			#endif
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {filename, buffer, size, static_cast <uint16_t> (seek), offset}, log::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод рекурсивного получения всех строк файла
 *
 * @param filename путь к файлу для чтения
 * @param callback функция обратного вызова
 * @param seek     тип смещения в файле
 * @param offset   смещение в файле
 * @param handle   внешний объект файла, если необходима поддержка пакетной обработки
 *
 */
void awh::Filesystem::readfile(string_view filename, const function <void (string_view)> & callback, const seek_t seek, const size_t offset, const handle_file_t & handle) const noexcept {
	// Если параметры для записи переданы
	if(!filename.empty() && (callback != nullptr)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * @brief Свой объект файла
			 *
			 * @note Заводится он всегда, но в дело идёт ЛИШЬ когда внешний объект не передан.
			 *       Так путь работы выходит один на оба случая, а закрытие описателя остаётся
			 *       делом самого объекта: свой гибнет с концом области видимости, внешний -
			 *       вместе с умным указателем у того, кто его завёл
			 */
			HandleFile local;
			// Объект файла, которым идёт работа
			HandleFile & file = (handle != nullptr ? (* handle) : local);
			/**
			 * Выполняем извлечение актуального значения адреса
			 *
			 * @note Извлекается он ЛИШЬ когда объект файла ещё не заведён: адрес идёт только
			 *       в открытие, а при уже открытом описателе не нужен вовсе. Разбор пути стоит
			 *       перехода в ядро (`realpath` под POSIX, `GetFullPathName` под MS Windows),
			 *       и при потоковой работе мелкими долями плата эта ложилась бы на всякую долю
			 */
			const string address = (!file.valid() ? this->fullpath(filename, true) : "");
			// Если объект файла заведён либо адрес получен правильный
			if(file.valid() || (!address.empty())){
				// Локальный буфер для хранения незавершённой строки
				string remainder = "";
				/**
				 * @brief Функция обработки прочитанных данных
				 *
				 * @param data указатель на данные
				 * @param size размер данных
				 *
				 */
				auto processFn = [&](const char * data, size_t size) noexcept -> void {
					// Если размер данных равен нулю
					if(size == 0)
						// Выходим из функции обработки
						return;
					// Добавляем прочитанные данные к остатку
					remainder.append(data, size);
					// Индекс начала текущей необработанной строки
					size_t start = 0;
					// Длина накопленного буфера
					const size_t length = remainder.length();
					/**
					 * Выполняем обработку остатка на наличие полных строк за один проход
					 */
					for(size_t pos = 0; pos < length; ++pos){
						// Если символ является символом новой строки
						if(remainder[pos] == '\n'){
							// Определяем конец строки без завершающего перевода
							size_t end = pos;
							// Если перед символом новой строки стоит возврат каретки — отбрасываем его
							if((end > start) && (remainder[end - 1] == '\r'))
								// Уменьшаем границу строки
								--end;
							// Вызываем функцию обратного вызова с найденной строкой
							callback(string_view(remainder.c_str() + start, end - start));
							// Сдвигаем начало следующей необработанной строки
							start = pos + 1;
						}
					}
					// Удаляем обработанный префикс, оставляя незавершённый хвост (один сдвиг вместо копий в цикле)
					if(start > 0)
						// Удаляем обработанную часть буфера
						remainder.erase(0, start);
				};
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					// Если объект файла ещё не заведён
					if(!file.valid())
						/**
						 * @brief Инициализируем новый объект файла
						 *
						 * @note Дозволяется и запись, и удаление, а не одно лишь чтение: файл вправе
						 *       держать открытым кто-то ещё - движок наблюдения за файловой системой
						 *       держит его именно так, - и обращение с одним лишь дозволением чтения
						 *       отвечало бы отказом ERROR_SHARING_VIOLATION. Отказ этот молчаливый:
						 *       дозапись уходила бы мимо файла, а размер выдавался бы нулевым
						 */
						file.set(::CreateFileW(fmk::convert(address).c_str(), GENERIC_READ, (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
					// Если файл открыт нормально
					if(file.valid()){
						// Создаём объект большого числа
						LARGE_INTEGER li;
						// Устанавливаем начальное значение позиции
						li.QuadPart = static_cast <LONGLONG> (offset);
						/**
						 * Определяем тип смещения в файле события
						 */
						switch(static_cast <uint8_t> (seek)){
							// Если смещение от начала файла
							case static_cast <uint8_t> (seek_t::BEGIN):
								// Выполняем установку позиции в файле
								li.LowPart = ::SetFilePointer(file, li.LowPart, &li.HighPart, FILE_BEGIN);
							break;
							// Если смещение от текущей позиции в файле
							case static_cast <uint8_t> (seek_t::CURRENT):
								// Выполняем установку позиции в файле
								li.LowPart = ::SetFilePointer(file, li.LowPart, &li.HighPart, FILE_CURRENT);
							break;
							// Если смещение от конца файла
							case static_cast <uint8_t> (seek_t::END):
								// Выполняем установку позиции в файле
								li.LowPart = ::SetFilePointer(file, li.LowPart, &li.HighPart, FILE_END);
							break;
							// Если тип смещения не определён
							default: li.LowPart = 0;
						}
						// Если мы получили ошибку установки позиции
						if((li.LowPart == INVALID_SET_FILE_POINTER) && (::GetLastError() != NO_ERROR))
							// Сбрасываем значение установленной позиции
							li.QuadPart = -1;
						// Если позиция установлена успешно
						if(li.QuadPart > -1){
							// Размер файла
							LARGE_INTEGER length;
							// Получаем размер файла
							if(!::GetFileSizeEx(file, &length)){
								// Создаём буфер сообщения ошибки
								wchar_t message[0xFF] = {0};
								// Выполняем формирование текста ошибки
								::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 0xFF, 0);
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									log::debug(L"%s", __PRETTY_FUNCTION__, {filename, static_cast <uint16_t> (seek), offset}, log::flag_t::CRITICAL, message);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									log::print(L"%s", log::flag_t::CRITICAL, message);
								#endif
								// Выходим из метода
								return;
							}
							// Выполняем создание буфера для чтения файла
							vector <char> buffer(::min(static_cast <size_t> (length.QuadPart), ::__awh_pagesize__()), 0);
							// Количество прочитанных байт
							DWORD bytes = 0;
							/**
							 * Читаем файл по частям до тех пор, пока не достигнем конца файла
							 */
							while(li.QuadPart < length.QuadPart){
								// Создаём объект перекрытого ввода-вывода
								OVERLAPPED overlapped = {};
								// Устанавливаем смещение для чтения
								overlapped.Offset = li.LowPart;
								// Устанавливаем старшее смещение для чтения
								overlapped.OffsetHigh = li.HighPart;
								// Выполняем чтение части файла в буфер
								if(!::ReadFile(file, &buffer[0], static_cast <DWORD> (::min <ULONGLONG> (static_cast <ULONGLONG> (buffer.size()),  static_cast <ULONGLONG> (length.QuadPart - li.QuadPart))), &bytes, &overlapped)){
									// Создаём буфер сообщения ошибки
									wchar_t message[0xFF] = {0};
									// Выполняем формирование текста ошибки
									::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 0xFF, 0);
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										log::debug(L"%s", __PRETTY_FUNCTION__, {filename, static_cast <uint16_t> (seek), offset}, log::flag_t::CRITICAL, message);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										log::print(L"%s", log::flag_t::CRITICAL, message);
									#endif
									// Выходим из метода
									return;
								}
								// Если прочитано 0 байт — выходим из цикла
								if(bytes == 0)
									// Замыкаем цикл чтения файла
									break;
								// Выполняем обработку прочитанного буфера
								processFn(&buffer[0], static_cast <size_t> (bytes));
								// Обновляем позицию в файле
								li.QuadPart += static_cast <LONGLONG> (bytes);
							}
						}
					}
				/**
				 * Для операционной системы не являющейся MS Windows
				 */
				#else
					// Структура статистики файла
					struct stat info{};
					// Если объект файла ещё не заведён
					if(!file.valid())
						// Выполняем открытие файла на чтение
						file.set(::open(address.c_str(), O_RDONLY));
					// Если файл не открыт
					if(!file.valid()){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							log::debug("%s", __PRETTY_FUNCTION__, {filename, static_cast <uint16_t> (seek), offset}, log::flag_t::CRITICAL, ::strerror(errno));
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log::print("%s", log::flag_t::CRITICAL, ::strerror(errno));
						#endif
					// Если файл открыт удачно
					} else if(::fstat(file, &info) < 0) {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							log::debug("%s", __PRETTY_FUNCTION__, {filename, static_cast <uint16_t> (seek), offset}, log::flag_t::CRITICAL, ::strerror(errno));
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log::print("%s", log::flag_t::CRITICAL, ::strerror(errno));
						#endif
					// Если размер файла изменился
					} else if(static_cast <size_t> (info.st_size) > offset) {
						// Позиция в файле
						off_t position = 0;
						/**
						 * Определяем тип смещения в файле события
						 */
						switch(static_cast <uint8_t> (seek)){
							// Если смещение от начала файла
							case static_cast <uint8_t> (seek_t::BEGIN):
								// Выполняем расчёт смещения в файле
								position = static_cast <off_t> (offset);
							break;
							// Если смещение от конца файла
							case static_cast <uint8_t> (seek_t::END):
								// Выполняем расчёт смещения в файле
								position = (static_cast <off_t> (info.st_size) - static_cast <off_t> (offset));
							break;
						}
						// Проверяем границы
						if(position < 0)
							// Устанавливаем позицию в начало файла
							position = 0;
						// Если позиция выше размера файла
						if(position >= static_cast <off_t> (info.st_size))
							// Выходим из метода
							return;
						// Определяем размер читаемых данных
						const off_t length = (static_cast <off_t> (info.st_size) - static_cast <off_t> (position));
						// Выполняем создание буфера для чтения файла
						vector <char> buffer(static_cast <size_t> (::min <off_t> (length, static_cast <off_t> (::__awh_pagesize__()))), 0);
						// Количество прочитанных байт
						ssize_t bytes = 0;
						/**
						 * Читаем файл по частям до тех пор, пока не достигнем конца файла
						 */
						while(position < length){
							// Читаем часть файла в буфер
							bytes = ::pread(file, &buffer[0], static_cast <size_t> (::min <off_t> (static_cast <off_t> (buffer.size()), length - position)), position);
							// Если прочитать часть файла не удалось
							if(bytes <= 0)
								// Выходим из цикла чтения файла
								break;
							// Выполняем обработку прочитанного буфера
							processFn(&buffer[0], static_cast <size_t> (bytes));
							// Обновляем позицию в файле
							position += static_cast <off_t> (bytes);
						}
					}
				#endif
				// Обработка последней строки (если нет \n в конце)
				if(!remainder.empty()){
					// Создаём представление строки
					string_view str(remainder.c_str(), remainder.size());
					// Если строка заканчивается символом возврата каретки
					if(!str.empty() && (str.back() == '\r'))
						// Удаляем символ возврата каретки из строки
						str = str.substr(0, str.size() - 1);
					// Вызываем функцию обратного вызова с найденной строкой
					callback(str);
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const ios_base::failure & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {filename, static_cast <uint16_t> (seek), offset}, log::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, error.what());
			#endif
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {filename, static_cast <uint16_t> (seek), offset}, log::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод рекурсивного получения буфера данных из больших файлов
 *
 * @param filename путь к файлу для чтения
 * @param size     размер буфера для чтения файла
 * @param callback функция обратного вызова
 * @param seek     тип смещения в файле
 * @param offset   смещение в файле
 * @param handle   внешний объект файла, если необходима поддержка пакетной обработки
 *
 */
void awh::Filesystem::readfile(string_view filename, const size_t size, const function <void (const void *, const size_t)> & callback, const seek_t seek, const size_t offset, const handle_file_t & handle) const noexcept {
	// Если параметры для записи переданы
	if(!filename.empty() && (callback != nullptr)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * @brief Свой объект файла
			 *
			 * @note Заводится он всегда, но в дело идёт ЛИШЬ когда внешний объект не передан.
			 *       Так путь работы выходит один на оба случая, а закрытие описателя остаётся
			 *       делом самого объекта: свой гибнет с концом области видимости, внешний -
			 *       вместе с умным указателем у того, кто его завёл
			 */
			HandleFile local;
			// Объект файла, которым идёт работа
			HandleFile & file = (handle != nullptr ? (* handle) : local);
			/**
			 * Выполняем извлечение актуального значения адреса
			 *
			 * @note Извлекается он ЛИШЬ когда объект файла ещё не заведён: адрес идёт только
			 *       в открытие, а при уже открытом описателе не нужен вовсе. Разбор пути стоит
			 *       перехода в ядро (`realpath` под POSIX, `GetFullPathName` под MS Windows),
			 *       и при потоковой работе мелкими долями плата эта ложилась бы на всякую долю
			 */
			const string address = (!file.valid() ? this->fullpath(filename, true) : "");
			// Если объект файла заведён либо адрес получен правильный
			if(file.valid() || (!address.empty())){
				// Определяем размер блока чтения (без модификации const-параметра)
				const size_t chunk = ((size == 0) ? ::__awh_pagesize__() : size);
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					// Если объект файла ещё не заведён
					if(!file.valid())
						/**
						 * @brief Инициализируем новый объект файла
						 *
						 * @note Дозволяется и запись, и удаление, а не одно лишь чтение: файл вправе
						 *       держать открытым кто-то ещё - движок наблюдения за файловой системой
						 *       держит его именно так, - и обращение с одним лишь дозволением чтения
						 *       отвечало бы отказом ERROR_SHARING_VIOLATION. Отказ этот молчаливый:
						 *       дозапись уходила бы мимо файла, а размер выдавался бы нулевым
						 */
						file.set(::CreateFileW(fmk::convert(address).c_str(), GENERIC_READ, (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
					// Если файл открыт нормально
					if(file.valid()){
						// Создаём объект большого числа
						LARGE_INTEGER li;
						// Устанавливаем начальное значение позиции
						li.QuadPart = static_cast <LONGLONG> (offset);
						/**
						 * Определяем тип смещения в файле события
						 */
						switch(static_cast <uint8_t> (seek)){
							// Если смещение от начала файла
							case static_cast <uint8_t> (seek_t::BEGIN):
								// Выполняем установку позиции в файле
								li.LowPart = ::SetFilePointer(file, li.LowPart, &li.HighPart, FILE_BEGIN);
							break;
							// Если смещение от текущей позиции в файле
							case static_cast <uint8_t> (seek_t::CURRENT):
								// Выполняем установку позиции в файле
								li.LowPart = ::SetFilePointer(file, li.LowPart, &li.HighPart, FILE_CURRENT);
							break;
							// Если смещение от конца файла
							case static_cast <uint8_t> (seek_t::END):
								// Выполняем установку позиции в файле
								li.LowPart = ::SetFilePointer(file, li.LowPart, &li.HighPart, FILE_END);
							break;
							// Если тип смещения не определён
							default: li.LowPart = 0;
						}
						// Если мы получили ошибку установки позиции
						if((li.LowPart == INVALID_SET_FILE_POINTER) && (::GetLastError() != NO_ERROR))
							// Сбрасываем значение установленной позиции
							li.QuadPart = -1;
						// Если позиция установлена успешно
						if(li.QuadPart > -1){
							// Размер файла
							LARGE_INTEGER length;
							// Получаем размер файла
							if(!::GetFileSizeEx(file, &length)){
								// Создаём буфер сообщения ошибки
								wchar_t message[0xFF] = {0};
								// Выполняем формирование текста ошибки
								::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 0xFF, 0);
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									log::debug(L"%s", __PRETTY_FUNCTION__, {filename, size, static_cast <uint16_t> (seek), offset}, log::flag_t::CRITICAL, message);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									log::print(L"%s", log::flag_t::CRITICAL, message);
								#endif
								// Выходим из метода
								return;
							}
							// Выполняем создание буфера для чтения файла
							vector <char> buffer(static_cast <size_t> (::min(length.QuadPart, static_cast <LONGLONG> (chunk))), 0);
							// Количество прочитанных байт
							DWORD bytes = 0;
							/**
							 * Читаем файл по частям до тех пор, пока не достигнем конца файла
							 */
							while(li.QuadPart < length.QuadPart){
								// Создаём объект перекрытого ввода-вывода
								OVERLAPPED overlapped = {};
								// Устанавливаем смещение для чтения
								overlapped.Offset = li.LowPart;
								// Устанавливаем старшее смещение для чтения
								overlapped.OffsetHigh = li.HighPart;
								// Выполняем чтение части файла в буфер
								if(!::ReadFile(file, &buffer[0], static_cast <DWORD> (::min <ULONGLONG> (static_cast <ULONGLONG> (buffer.size()),  static_cast <ULONGLONG> (length.QuadPart - li.QuadPart))), &bytes, &overlapped)){
									// Создаём буфер сообщения ошибки
									wchar_t message[0xFF] = {0};
									// Выполняем формирование текста ошибки
									::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 0xFF, 0);
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										log::debug(L"%s", __PRETTY_FUNCTION__, {filename, size, static_cast <uint16_t> (seek), offset}, log::flag_t::CRITICAL, message);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										log::print(L"%s", log::flag_t::CRITICAL, message);
									#endif
									// Выходим из метода
									return;
								}
								// Если прочитано 0 байт — выходим из цикла
								if(bytes == 0)
									// Замыкаем цикл чтения файла
									break;
								// Возвращаем функцию обратного вызова
								callback(&buffer[0], static_cast <size_t> (bytes));
								// Обновляем позицию в файле
								li.QuadPart += static_cast <LONGLONG> (bytes);
							}
						}
					}
				/**
				 * Для операционной системы не являющейся MS Windows
				 */
				#else
					// Структура статистики файла
					struct stat info{};
					// Если объект файла ещё не заведён
					if(!file.valid())
						// Выполняем открытие файла на чтение
						file.set(::open(address.c_str(), O_RDONLY));
					// Если файл не открыт
					if(!file.valid()){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							log::debug("%s", __PRETTY_FUNCTION__, {filename, size, static_cast <uint16_t> (seek), offset}, log::flag_t::CRITICAL, ::strerror(errno));
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log::print("%s", log::flag_t::CRITICAL, ::strerror(errno));
						#endif
					// Если файл открыт удачно
					} else if(::fstat(file, &info) < 0) {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							log::debug("%s", __PRETTY_FUNCTION__, {filename, size, static_cast <uint16_t> (seek), offset}, log::flag_t::CRITICAL, ::strerror(errno));
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log::print("%s", log::flag_t::CRITICAL, ::strerror(errno));
						#endif
					// Если размер файла изменился
					} else if(static_cast <size_t> (info.st_size) > offset) {
						// Позиция в файле
						off_t position = 0;
						/**
						 * Определяем тип смещения в файле события
						 */
						switch(static_cast <uint8_t> (seek)){
							// Если смещение от начала файла
							case static_cast <uint8_t> (seek_t::BEGIN):
								// Выполняем расчёт смещения в файле
								position = static_cast <off_t> (offset);
							break;
							// Если смещение от конца файла
							case static_cast <uint8_t> (seek_t::END):
								// Выполняем расчёт смещения в файле
								position = (static_cast <off_t> (info.st_size) - static_cast <off_t> (offset));
							break;
						}
						// Проверяем границы
						if(position < 0)
							// Устанавливаем позицию в начало файла
							position = 0;
						// Если позиция выше размера файла
						if(position >= static_cast <off_t> (info.st_size))
							// Выходим из метода
							return;
						// Определяем размер читаемых данных
						const off_t length = (static_cast <off_t> (info.st_size) - static_cast <off_t> (position));
						// Выполняем создание буфера для чтения файла
						vector <char> buffer(static_cast <size_t> (::min <off_t> (length, static_cast <off_t> (chunk))), 0);
						// Количество прочитанных байт
						ssize_t bytes = 0;
						/**
						 * Читаем файл по частям до тех пор, пока не достигнем конца файла
						 */
						while(position < length){
							// Читаем часть файла в буфер
							bytes = ::pread(file, &buffer[0], static_cast <size_t> (::min <off_t> (static_cast <off_t> (buffer.size()), length - position)), position);
							// Если прочитать часть файла не удалось
							if(bytes <= 0)
								// Выходим из цикла чтения файла
								break;
							// Возвращаем функцию обратного вызова
							callback(&buffer[0], static_cast <size_t> (bytes));
							// Обновляем позицию в файле
							position += static_cast <off_t> (bytes);
						}
					}
				#endif
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const ios_base::failure & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {filename, size, static_cast <uint16_t> (seek), offset}, log::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, error.what());
			#endif
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {filename, size, static_cast <uint16_t> (seek), offset}, log::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод обхода файлов во всех подкаталогах с остановкой и продолжением
 *
 * @param path     путь до каталога
 * @param ext      расширение файла по которому идет фильтрация
 * @param recurse  флаг рекурсивного перебора каталогов
 * @param callback функция обратного вызова, ложь останавливает обход
 * @param resolve  флаг резолвинга символьных ссылок
 * @param handle   внешний объект каталога, если необходима поддержка пакетной обработки
 * @return         признак того, что обход довершён до конца
 *
 */
bool awh::Filesystem::walkdir(string_view path, string_view ext, const bool recurse, const function <bool (const type_t, string_view)> & callback, const bool resolve, const handle_dir_t & handle) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если адрес каталога и функция обратного вызова переданы
	if(!path.empty() && (callback != nullptr)){
		// Выполняем извлечение актуального значения адреса
		const string & root = this->fullpath(path, resolve);
		// Если адрес получен правильный
		if(!root.empty() && (this->type(root) == type_t::DIR)){
			/**
			 * @brief Свой объект каталога
			 *
			 * @note Заводится он всегда, но в дело идёт ЛИШЬ когда внешний объект не передан.
			 *       Так путь обхода выходит один на оба случая: разница между пакетной работой
			 *       и разовой сводится к тому, кто переживёт вызов, а не к тому, как идёт обход
			 */
			HandleDir local;
			// Объект каталога, которым идёт обход
			HandleDir & dir = (handle != nullptr ? (* handle) : local);
			/**
			 * Выполняем перехват ошибок
			 */
			try {
				/**
				 * Если объект каталога нашему адресу ещё не служит
				 *
				 * @note Сюда же попадает и объект, служивший ИНОМУ адресу: прежний обход
				 *       ему уже не продолжить, и состояние его сбрасывается вместе с
				 *       закрытием всех открытых им каталогов
				 */
				if(!dir.valid() || (dir.address() != root)){
					// Выполняем сброс состояния прежнего обхода
					dir.reset();
					/**
					 * Для операционной системы MS Windows
					 */
					#if _WIN32 || _WIN64
						// Открываем корень обхода
						dir.set(awh::dir::_wopendir(fmk::convert(root).c_str()));
					/**
					 * Для операционной системы не являющейся MS Windows
					 */
					#else
						// Открываем корень обхода
						dir.set(awh::dir::opendir(root.c_str()));
					#endif
					// Запоминаем адрес, которому объект служит
					dir.address(root);
					// Отмечаем обход начатым
					dir.active(true);
				/**
				 * Если прежний обход был доведён до конца
				 *
				 * @note Вот ради чего внешний объект каталога и заводится: повторный обзор
				 *       того же каталога идёт перемоткой, мимо повторного открытия. Открытие
				 *       каталога стоит столько же, сколько открытие файла, - под MS Windows
				 *       это около 0.37 мс на вызов
				 */
				} else if(!dir.active()) {
					// Выполняем перемотку каталога к началу
					dir.rewind();
					// Отмечаем обход начатым
					dir.active(true);
				}
				// Если корень обхода открыть не удалось
				if(!dir.valid()){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						log::debug("Directory name: \"%s\" cannot be opened", __PRETTY_FUNCTION__, {path, ext, recurse, resolve}, log::flag_t::WARNING, root.c_str());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						log::print("Directory name: \"%s\" cannot be opened", log::flag_t::WARNING, root.c_str());
					#endif
					// Выполняем сброс состояния обхода
					dir.reset();
					// Выходим из функции
					return result;
				}
				// Отмечаем обход доведённым до конца
				result = true;
				/**
				 * Выполняем обход до исчерпания содержимого либо до остановки откликом
				 */
				while(result){
					/**
					 * Если прежний обход остановился, не довершив записи, отдаём её повторно
					 *
					 * @note Место записи в каталоге уже пройдено, и без повторной выдачи остаток
					 *       её был бы утерян молча. Ставится признак этот теми видами `walkdir`,
					 *       что отдают СОДЕРЖИМОЕ файла: они же и пропустят доли, отданные прежде.
					 *       Обход по одним именам его не ставит никогда - там запись отдана целиком
					 */
					if(dir.repeat()){
						// Получаем адрес незавершённой записи
						const string address = dir.pending();
						// Получаем вид незавершённой записи
						const type_t type = static_cast <type_t> (dir.pendingType());
						// Запоминаем число долей, отданных прежде
						const size_t delivered = dir.delivered();
						/**
						 * Отдаём незавершённую запись повторно
						 *
						 * @note Признак повторной выдачи здесь НЕ снимается: отклик тех видов
						 *       `walkdir`, что отдают содержимое, читает его сам, чтобы узнать,
						 *       сколько долей пропустить. Снимет его либо он же по довершении
						 *       файла, либо мы следом, если он того не сделал
						 */
						result = callback(type, address);
						/**
						 * Если отклик места остановки не подвинул, забываем запись сами
						 *
						 * @note Без этого обход по одним именам, коему подан объект каталога с
						 *       незавершённой записью от иного вида `walkdir`, отдавал бы её
						 *       вечно: снимать признак ему нечем, он содержимого не читает
						 */
						if(dir.repeat() && (dir.delivered() == delivered) && (dir.pending() == address))
							// Выполняем забвение незавершённой записи обхода
							dir.done();
						// Продолжаем обход
						continue;
					}
					// Получаем адрес каталога, по которому идёт обход
					const string base = (dir.empty() ? dir.address() : dir.topAddress());
					/**
					 * Для операционной системы MS Windows
					 */
					#if _WIN32 || _WIN64
						// Выполняем чтение содержимого каталога
						awh::dir::_wdirent * ptr = awh::dir::_wreaddir(dir.empty() ? static_cast <awh::dir::_WDIR *> (dir) : dir.top());
					/**
					 * Для операционной системы не являющейся MS Windows
					 */
					#else
						// Выполняем чтение содержимого каталога
						awh::dir::dirent * ptr = awh::dir::readdir(dir.empty() ? static_cast <awh::dir::DIR *> (dir) : dir.top());
					#endif
					// Если содержимое каталога исчерпано
					if(ptr == nullptr){
						// Если исчерпан сам корень обхода
						if(dir.empty()){
							// Отмечаем обход доведённым до конца
							dir.active(false);
							// Выходим из обхода
							break;
						}
						// Получаем адрес исчерпанного каталога
						const string address = dir.topAddress();
						/**
						 * Снимаем исчерпанный каталог со стопки мест обхода
						 *
						 * @note Снятие идёт ПРЕЖДЕ отклика намеренно: откажись отклик продолжать,
						 *       обход возобновится с родителя, а не с уже отданного каталога
						 */
						dir.pop();
						// Отдаём сам каталог ПОСЛЕ его содержимого
						result = callback(type_t::DIR, address);
						// Продолжаем обход
						continue;
					}
					/**
					 * Для операционной системы MS Windows
					 */
					#if _WIN32 || _WIN64
						// Пропускаем названия текущие "." и внешние "..", так как идет рекурсия
						if(!::wcscmp(ptr->d_name, L".") || !::wcscmp(ptr->d_name, L".."))
							// Выполняем пропуск каталога
							continue;
						// Получаем адрес в виде строки
						const string & address = fmk::format("%s%s%s", base.c_str(), AWH_FS_SEPARATOR, fmk::convert(ptr->d_name).c_str());
					/**
					 * Для операционной системы не являющейся MS Windows
					 */
					#else
						// Пропускаем названия текущие "." и внешние "..", так как идет рекурсия
						if(!::strcmp(ptr->d_name, ".") || !::strcmp(ptr->d_name, ".."))
							// Выполняем пропуск каталога
							continue;
						// Получаем адрес в виде строки
						const string & address = fmk::format("%s%s%s", base.c_str(), AWH_FS_SEPARATOR, ptr->d_name);
					#endif
					// Получаем тип переданного пути
					const type_t type = this->type(address);
					/**
					 * Определяем тип переданного пути
					 */
					switch(static_cast <uint8_t> (type)){
						// Если полный путь является каталогом
						case static_cast <uint8_t> (type_t::DIR): {
							// Если требуется рекурсивный перебор каталогов
							if(recurse){
								/**
								 * Для операционной системы MS Windows
								 */
								#if _WIN32 || _WIN64
									// Открываем вложенный каталог
									awh::dir::_WDIR * nested = awh::dir::_wopendir(fmk::convert(address).c_str());
								/**
								 * Для операционной системы не являющейся MS Windows
								 */
								#else
									// Открываем вложенный каталог
									awh::dir::DIR * nested = awh::dir::opendir(address.c_str());
								#endif
								// Если вложенный каталог открыт
								if(nested != nullptr){
									// Выполняем спуск во вложенный каталог
									dir.push(nested, address);
									// Продолжаем обход
									continue;
								}
							}
							// Возвращаем данные каталога как он есть
							result = callback(type, address);
						} break;
						// Если полный путь является ссылкой
						case static_cast <uint8_t> (type_t::LINK):
						// Если полный путь является файлом
						case static_cast <uint8_t> (type_t::FILE): {
							// Если расширение файла передано
							if(!ext.empty()){
								// Получаем расширение файла
								const string & extension = fmk::format(".%s", ext.data());
								// Если расширение не выше полного адреса
								if(address.size() > extension.length()){
									// Получаем хвост адреса длиною в расширение
									const string & part = address.substr(address.size() - extension.length(), extension.length());
									// Если расширение файла найдено
									if(fmk::compare(part, extension))
										// Возвращаем полный путь файла
										result = callback(type, address);
								}
							// Если расширение файла не передано, то просто выводим полный путь файла
							} else result = callback(type, address);
						} break;
						// Если путь принадлежит к другому типу
						default:
							// Возвращаем полный путь файла
							result = callback(type, address);
					}
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const ios_base::failure & error) {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					log::debug("%s", __PRETTY_FUNCTION__, {path, ext, recurse, resolve}, log::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					log::print("%s", log::flag_t::CRITICAL, error.what());
				#endif
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					log::debug("%s", __PRETTY_FUNCTION__, {path, ext, recurse, resolve}, log::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					log::print("%s", log::flag_t::CRITICAL, error.what());
				#endif
			}
		}
	// Если переданный адрес не является каталогом
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			log::debug("Path name: \"%s\" is not found", __PRETTY_FUNCTION__, {path, ext, recurse, resolve}, log::flag_t::WARNING, path.data());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("Path name: \"%s\" is not found", log::flag_t::WARNING, path.data());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод обхода файлов во всех подкаталогах построчно с остановкой и продолжением
 *
 * @param path     путь до каталога
 * @param ext      расширение файла по которому идет фильтрация
 * @param recurse  флаг рекурсивного перебора каталогов
 * @param callback функция обратного вызова, ложь останавливает обход
 * @param resolve  флаг резолвинга символьных ссылок
 * @param handle   внешний объект каталога, если необходима поддержка пакетной обработки
 * @return         признак того, что обход довершён до конца
 *
 */
bool awh::Filesystem::walkdir(string_view path, string_view ext, const bool recurse, const function <bool (const type_t, string_view, string_view)> & callback, const bool resolve, const handle_dir_t & handle) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если адрес каталога и функция обратного вызова переданы
	if(!path.empty() && (callback != nullptr)){
		// Выполняем извлечение актуального значения адреса
		const string & address = this->fullpath(path, resolve);
		// Если адрес получен правильный
		if(!address.empty() && (this->type(address) == type_t::DIR)){
			// Переходим по всему списку файлов в каталоге
			result = this->walkdir(address, ext, recurse, [&](const type_t type, string_view filename) noexcept -> bool {
				/**
				 * @brief Признак того, что обход велено продолжать
				 *
				 * @note Отклик чтения строк остановить чтение не может - вид его возврата пуст.
				 *       Оттого признак этот выдачу строк прекращает немедленно, а чтение самого
				 *       файла довершается вхолостую
				 */
				bool proceed = true;
				/**
				 * @brief Число долей этого файла, отданных прежде
				 *
				 * @details Обход, остановленный посреди файла, запоминает место остановки во внешнем
				 *          объекте каталога. Продолжение получает файл ПОВТОРНО и пропускает доли,
				 *          отданные прежде, - так остаток файла не теряется и не отдаётся дважды.
				 *
				 * @warning Без внешнего объекта каталога хранить место остановки негде: продолжения
				 *          не будет вовсе, и остановка посреди файла отбросит его остаток
				 *
				 * @note Пропуск идёт по СЧЁТУ долей, а не по смещению в файле: измени файл между
				 *       долями обхода, и счёт указал бы уже на иное место. Продолжение здесь, как
				 *       и продолжение по дереву, идёт по возможности
				 */
				const size_t skip = (((handle != nullptr) && handle->repeat() && (handle->pending() == filename)) ? handle->delivered() : 0);
				// Число долей этого файла, отданных к этому мигу
				size_t index = 0;
				/**
				 * Определяем тип переданного пути
				 */
				switch(static_cast <uint8_t> (type)){
					// Если полный путь является ссылкой
					case static_cast <uint8_t> (type_t::LINK): {
						// Получаем полный путь файла
						const string & address = this->fullpath(filename, true);
						// Если полный путь является файлом
						if(this->type(address) == type_t::FILE){
							// Если расширение файла передано
							if(!ext.empty()){
								// Получаем расширение файла
								const string & extension = fmk::format(".%s", ext.data());
								// Если расширение не выше полного адреса
								if(address.size() > extension.length()){
									// Получаем хвост адреса длиною в расширение
									const string_view part = filename.substr(filename.size() - extension.length(), extension.length());
									// Если расширение файла найдено
									if(fmk::compare(part, extension)){
										// Выполняем считывание всех строк текста
										this->readfile(address, [&](string_view text) noexcept -> void {
											// Если текст получен и обход велено продолжать
											if(proceed && !text.empty() && (index++ >= skip))
												// Возвращаем функцию обратного вызова
												proceed = callback(type, filename, text);
										}, seek_t::BEGIN);
									}
								}
							// Если расширение файла не передано
							} else {
								// Выполняем считывание всех строк текста
								this->readfile(address, [&](string_view text) noexcept -> void {
									// Если текст получен и обход велено продолжать
									if(proceed && !text.empty() && (index++ >= skip))
										// Возвращаем функцию обратного вызова
										proceed = callback(type, filename, text);
								}, seek_t::BEGIN);
							}
						}
					} break;
					// Если полный путь является файлом
					case static_cast <uint8_t> (type_t::FILE): {
						// Выполняем считывание всех строк текста
						this->readfile(filename, [&](string_view text) noexcept -> void {
							// Если текст получен и обход велено продолжать
							if(proceed && !text.empty() && (index++ >= skip))
								// Возвращаем функцию обратного вызова
								proceed = callback(type, filename, text);
						}, seek_t::BEGIN);
					} break;
				}
				/**
				 * Запоминаем место остановки внутри файла либо забываем прежнее
				 *
				 * @note Забвение обязательно и при успехе: файл, довершённый до конца, повторной
				 *       выдачи не требует, а признак, оставленный стоять, отдал бы его снова
				 */
				if(handle != nullptr){
					// Если обход остановлен посреди файла
					if(!proceed)
						// Запоминаем место остановки внутри файла
						handle->pending(filename, static_cast <uint8_t> (type), index);
					// Если файл довершён до конца
					else handle->done();
				}
				// Выводим признак того, что обход велено продолжать
				return proceed;
			}, resolve, handle);
		}
	// Если переданный адрес не является каталогом
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			log::debug("Address: \"%s\" is not found", __PRETTY_FUNCTION__, {path, ext, recurse, resolve}, log::flag_t::WARNING, path.data());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("Address: \"%s\" is not found", log::flag_t::WARNING, path.data());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод обхода файлов во всех подкаталогах бинарными блоками с остановкой и продолжением
 *
 * @param path     путь до каталога
 * @param ext      расширение файла по которому идет фильтрация
 * @param size     размер буфера для чтения файла
 * @param recurse  флаг рекурсивного перебора каталогов
 * @param callback функция обратного вызова, ложь останавливает обход
 * @param resolve  флаг резолвинга символьных ссылок
 * @param handle   внешний объект каталога, если необходима поддержка пакетной обработки
 * @return         признак того, что обход довершён до конца
 *
 */
bool awh::Filesystem::walkdir(string_view path, string_view ext, const size_t size, const bool recurse, const function <bool (const type_t, string_view, const void *, const size_t)> & callback, const bool resolve, const handle_dir_t & handle) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если адрес каталога и функция обратного вызова переданы
	if(!path.empty() && (callback != nullptr)){
		// Выполняем извлечение актуального значения адреса
		const string & address = this->fullpath(path, resolve);
		// Если адрес получен правильный
		if(!address.empty() && (this->type(address) == type_t::DIR)){
			// Переходим по всему списку файлов в каталоге (нулевой размер блока скорректирует readfile)
			result = this->walkdir(address, ext, recurse, [&](const type_t type, string_view filename) noexcept -> bool {
				/**
				 * @brief Признак того, что обход велено продолжать
				 *
				 * @note Отклик чтения блоков остановить чтение не может - вид его возврата пуст.
				 *       Оттого признак этот выдачу блоков прекращает немедленно, а чтение самого
				 *       файла довершается вхолостую
				 */
				bool proceed = true;
				/**
				 * @brief Число долей этого файла, отданных прежде
				 *
				 * @details Обход, остановленный посреди файла, запоминает место остановки во внешнем
				 *          объекте каталога. Продолжение получает файл ПОВТОРНО и пропускает доли,
				 *          отданные прежде, - так остаток файла не теряется и не отдаётся дважды.
				 *
				 * @warning Без внешнего объекта каталога хранить место остановки негде: продолжения
				 *          не будет вовсе, и остановка посреди файла отбросит его остаток
				 *
				 * @note Пропуск идёт по СЧЁТУ долей, а не по смещению в файле: измени файл между
				 *       долями обхода, и счёт указал бы уже на иное место. Продолжение здесь, как
				 *       и продолжение по дереву, идёт по возможности
				 */
				const size_t skip = (((handle != nullptr) && handle->repeat() && (handle->pending() == filename)) ? handle->delivered() : 0);
				// Число долей этого файла, отданных к этому мигу
				size_t index = 0;
				/**
				 * Определяем тип переданного пути
				 */
				switch(static_cast <uint8_t> (type)){
					// Если полный путь является ссылкой
					case static_cast <uint8_t> (type_t::LINK): {
						// Получаем полный путь файла
						const string & address = this->fullpath(filename, true);
						// Если полный путь является файлом
						if(this->type(address) == type_t::FILE){
							// Если расширение файла передано
							if(!ext.empty()){
								// Получаем расширение файла
								const string & extension = fmk::format(".%s", ext.data());
								// Если расширение не выше полного адреса
								if(address.size() > extension.length()){
									// Получаем хвост адреса длиною в расширение
									const string_view part = filename.substr(filename.size() - extension.length(), extension.length());
									// Если расширение файла найдено
									if(fmk::compare(part, extension)){
										// Выполняем считывание всех блоков данных
										this->readfile(address, size, [&](const void * buffer, const size_t size) noexcept -> void {
											// Если буфер данных получен и обход велено продолжать
											if(proceed && (buffer != nullptr) && (size > 0) && (index++ >= skip))
												// Возвращаем функцию обратного вызова
												proceed = callback(type, filename, buffer, size);
										}, seek_t::BEGIN);
									}
								}
							// Если расширение файла не передано
							} else {
								// Выполняем считывание всех блоков данных
								this->readfile(address, size, [&](const void * buffer, const size_t size) noexcept -> void {
									// Если буфер данных получен и обход велено продолжать
									if(proceed && (buffer != nullptr) && (size > 0) && (index++ >= skip))
										// Возвращаем функцию обратного вызова
										proceed = callback(type, filename, buffer, size);
								}, seek_t::BEGIN);
							}
						}
					} break;
					// Если полный путь является файлом
					case static_cast <uint8_t> (type_t::FILE): {
						// Выполняем считывание всех блоков данных
						this->readfile(filename, size, [&](const void * buffer, const size_t size) noexcept -> void {
							// Если буфер данных получен и обход велено продолжать
							if(proceed && (buffer != nullptr) && (size > 0) && (index++ >= skip))
								// Возвращаем функцию обратного вызова
								proceed = callback(type, filename, buffer, size);
						}, seek_t::BEGIN);
					} break;
				}
				/**
				 * Запоминаем место остановки внутри файла либо забываем прежнее
				 *
				 * @note Забвение обязательно и при успехе: файл, довершённый до конца, повторной
				 *       выдачи не требует, а признак, оставленный стоять, отдал бы его снова
				 */
				if(handle != nullptr){
					// Если обход остановлен посреди файла
					if(!proceed)
						// Запоминаем место остановки внутри файла
						handle->pending(filename, static_cast <uint8_t> (type), index);
					// Если файл довершён до конца
					else handle->done();
				}
				// Выводим признак того, что обход велено продолжать
				return proceed;
			}, resolve, handle);
		}
	// Если переданный адрес не является каталогом
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			log::debug("Address: \"%s\" is not found", __PRETTY_FUNCTION__, {path, ext, size, recurse, resolve}, log::flag_t::WARNING, path.data());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("Address: \"%s\" is not found", log::flag_t::WARNING, path.data());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод рекурсивного получения файлов во всех подкаталогах
 *
 * @param path     путь до каталога
 * @param ext      расширение файла по которому идет фильтрация
 * @param recurse  флаг рекурсивного перебора каталогов
 * @param callback функция обратного вызова
 * @param resolve  флаг резолвинга символьных ссылок
 * @param handle   внешний объект каталога, если необходима поддержка пакетной обработки
 *
 */
void awh::Filesystem::readdir(string_view path, string_view ext, const bool recurse, const function <void (const type_t, string_view)> & callback, const bool resolve, const handle_dir_t & handle) const noexcept {
	/**
	 * Выполняем обход, который остановить нельзя
	 *
	 * @note Работа эта есть тот же обход, у которого отклик всегда велит продолжать.
	 *       Держать для неё своё устройство незачем - разошлись бы они не видом, а
	 *       поведением, и всякая находка чинилась бы дважды
	 */
	this->walkdir(path, ext, recurse, [&callback](const type_t type, string_view address) noexcept -> bool {
		// Выполняем функцию обратного вызова
		callback(type, address);
		// Велим продолжать обход
		return true;
	}, resolve, handle);
}
/**
 * @brief Метод рекурсивного чтения файлов во всех подкаталогах построчно
 *
 * @param path     путь до каталога
 * @param ext      расширение файла по которому идет фильтрация
 * @param recurse  флаг рекурсивного перебора каталогов
 * @param callback функция обратного вызова
 * @param resolve  флаг резолвинга символьных ссылок
 * @param handle   внешний объект каталога, если необходима поддержка пакетной обработки
 *
 */
void awh::Filesystem::readdir(string_view path, string_view ext, const bool recurse, const function <void (const type_t, string_view, string_view)> & callback, const bool resolve, const handle_dir_t & handle) const noexcept {
	// Если адрес каталога и расширение файлов переданы
	if(!path.empty() && (callback != nullptr)){
		// Выполняем извлечение актуального значения адреса
		const string & address = this->fullpath(path, resolve);
		// Если адрес получен правильный
		if(!address.empty() && (this->type(address) == type_t::DIR)){
			// Переходим по всему списку файлов в каталоге
			this->readdir(address, ext, recurse, [&](const type_t type, string_view filename) noexcept -> void {
				/**
				 * Определяем тип переданного пути
				 */
				switch(static_cast <uint8_t> (type)){
					// Если полный путь является ссылкой
					case static_cast <uint8_t> (type_t::LINK): {
						// Получаем полный путь файла
						const string & address = this->fullpath(filename, true);
						// Если полный путь является файлом
						if(this->type(address) == type_t::FILE){
							// Если расширение файла передано
							if(!ext.empty()){
								// Получаем путь до файла в нижнем регистре
								string_view path = address;
								// Получаем расширение файла
								const string & extension = fmk::format(".%s", ext.data());
								// Если расширение не выше полного адреса
								if(path.size() > extension.length()){
									// Если расширение файла найдено
									if(fmk::compare(filename.substr(filename.size() - extension.length(), extension.length()).data(), extension)){
										// Выполняем считывание всех строк текста
										this->readfile(path, [&](string_view text) noexcept -> void {
											// Если текст получен
											if(!text.empty())
												// Возвращаем функцию обратного вызова
												callback(type, filename, text);
										}, seek_t::BEGIN);
									}
								}
							// Если расширение файла не передано
							} else {
								// Выполняем считывание всех строк текста
								this->readfile(address, [&](string_view text) noexcept -> void {
									// Если текст получен
									if(!text.empty())
										// Возвращаем функцию обратного вызова
										callback(type, filename, text);
								}, seek_t::BEGIN);
							}
						}
					} break;
					// Если полный путь является файлом
					case static_cast <uint8_t> (type_t::FILE): {
						// Выполняем считывание всех строк текста
						this->readfile(filename, [&](string_view text) noexcept -> void {
							// Если текст получен
							if(!text.empty())
								// Возвращаем функцию обратного вызова
								callback(type, filename, text);
						}, seek_t::BEGIN);
					} break;
				}
			}, resolve, handle);
		}
	// Если переданный адрес не является каталогом
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			log::debug("Address: \"%s\" is not found", __PRETTY_FUNCTION__, {path, ext, recurse, resolve}, log::flag_t::WARNING, path.data());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("Address: \"%s\" is not found", log::flag_t::WARNING, path.data());
		#endif
	}
}
/**
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
 */
void awh::Filesystem::readdir(string_view path, string_view ext, const size_t size, const bool recurse, const function <void (const type_t, string_view, const void *, const size_t)> & callback, const bool resolve, const handle_dir_t & handle) const noexcept {
	// Если адрес каталога и расширение файлов переданы
	if(!path.empty() && (callback != nullptr)){
		// Выполняем извлечение актуального значения адреса
		const string & address = this->fullpath(path, resolve);
		// Если адрес получен правильный
		if(!address.empty() && (this->type(address) == type_t::DIR)){
			// Переходим по всему списку файлов в каталоге (нулевой размер блока скорректирует readfile)
			this->readdir(address, ext, recurse, [&](const type_t type, string_view filename) noexcept -> void {
				/**
				 * Определяем тип переданного пути
				 */
				switch(static_cast <uint8_t> (type)){
					// Если полный путь является ссылкой
					case static_cast <uint8_t> (type_t::LINK): {
						// Получаем полный путь файла
						const string & address = this->fullpath(filename, true);
						// Если полный путь является файлом
						if(this->type(address) == type_t::FILE){
							// Если расширение файла передано
							if(!ext.empty()){
								// Получаем путь до файла в нижнем регистре
								string_view path = address;
								// Получаем расширение файла
								const string & extension = fmk::format(".%s", ext.data());
								// Если расширение не выше полного адреса
								if(path.size() > extension.length()){
									// Если расширение файла найдено
									if(fmk::compare(filename.substr(filename.size() - extension.length(), extension.length()).data(), extension)){
										// Выполняем считывание всех строк текста
										this->readfile(path, size, [&](const void * buffer, const size_t size) noexcept -> void {
											// Буфер данных получен успешно
											if((buffer != nullptr) && (size > 0))
												// Возвращаем функцию обратного вызова
												callback(type, filename, buffer, size);
										}, seek_t::BEGIN);
									}
								}
							// Если расширение файла не передано
							} else {
								// Выполняем считывание всех строк текста
								this->readfile(address, size, [&](const void * buffer, const size_t size) noexcept -> void {
									// Буфер данных получен успешно
									if((buffer != nullptr) && (size > 0))
										// Возвращаем функцию обратного вызова
										callback(type, filename, buffer, size);
								}, seek_t::BEGIN);
							}
						}
					} break;
					// Если полный путь является файлом
					case static_cast <uint8_t> (type_t::FILE): {
						// Выполняем считывание всех строк текста
						this->readfile(filename, size, [&](const void * buffer, const size_t size) noexcept -> void {
							// Буфер данных получен успешно
							if((buffer != nullptr) && (size > 0))
								// Возвращаем функцию обратного вызова
								callback(type, filename, buffer, size);
						}, seek_t::BEGIN);
					} break;
				}
			}, resolve, handle);
		}
	// Если переданный адрес не является каталогом
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			log::debug("Address: \"%s\" is not found", __PRETTY_FUNCTION__, {path, ext, size, recurse, resolve}, log::flag_t::WARNING, path.data());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("Address: \"%s\" is not found", log::flag_t::WARNING, path.data());
		#endif
	}
}
/**
 * @brief Конструктор
 *
 */
awh::Filesystem::Filesystem() noexcept : _os() {}
