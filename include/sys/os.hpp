/**
 * @file: os.hpp
 * @date: 2025-10-25
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл модуля работы с операционной системой —
 *        класс Operating_System для определения семейства и версии ОС, получения информации о процессоре и памяти,
 *        управления лимитами процесса, привилегиями и системными параметрами
 *
 * \~english
 * @brief Header file of the module for working with the operating system —
 *        the Operating_System class for determining the family and the version of the OS, getting information about the processor and the memory,
 *        managing the limits of the process, the privileges and the system parameters
 *
 * \~
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_OPERATING_SYSTEM__
#define __AWH_OPERATING_SYSTEM__

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <string>
#include <cstdint>
#include <unistd.h>

/**
 * Активируем поддержку юникода
 */
#ifndef UNICODE
	#define UNICODE
#endif

/**
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
	/**
	 * \~russian
	 * Заменяем переменную AWH ERROR
	 *
	 * @note Заголовков MS Windows здесь нет намеренно: телу макроса объявление в точке
	 *       определения не требуется, оно нужно лишь в точке применения, то есть в файле
	 *       реализации. Подключайся здесь sys/win32.hpp, снятия макросов протекали бы
	 *       в единицу трансляции потребителя библиотеки
	 *
	 * \~english
	 * Replace the AWH ERROR variable
	 * @note The MS Windows headers are absent here deliberately: the body of the macro requires no declaration at the point
	 *       of definition, it needs it only at the point of use, that is, in the implementation
	 *       file. Were sys/win32.hpp included here, the removals of the macros would leak
	 *       into the translation unit of the consumer of the library
	 *
	 * \~
	 */
	#define AWH_ERROR() (::WSAGetLastError())

	/**
	 * Файловый разделитель Windows
	 */
	#define AWH_FS_SEPARATOR "\\"

	/**
	 * \~russian
	 * Устанавливаем типы данных uid_t и gid_t
	 *
	 * @details Типов этих у MS Windows нет вовсе - разделения пользователей и групп по
	 *          числам там не заведено, - и восполняются они здесь
	 *
	 * @note Заводятся они объявлениями типов, а не макросами. Прежде здесь стояли
	 *       макросы под проверкой «#ifndef», и проверка та ловушкой была по существу:
	 *       препроцессор объявлений типов не видит, а макрос, заведённый поверх чужого
	 *       объявления, обращает его в бессмыслицу. Так и вышло у соседнего u_char -
	 *       пояснение ниже. Здесь беды не случилось лишь оттого, что MinGW типов этих
	 *       не объявляет, но опираться на это незачем: объявление типа ловушки не несёт
	 *
	 * \~english
	 * Set the uid_t and gid_t data types
	 * @details MS Windows has no such types at all — the separation of users and groups by
	 *          numbers is not introduced there — and they are made up for here
	 * @note They are introduced by type declarations rather than by macros. Before, macros
	 *       stood here under an «#ifndef» check, and that check was a trap in essence:
	 *       the preprocessor does not see type declarations, and a macro introduced over someone else's
	 *       declaration turns it into nonsense. That is exactly what happened with the neighbouring u_char —
	 *       the explanation is below. No trouble happened here only because MinGW does not declare those
	 *       types, but there is no point in relying on that: a type declaration carries no trap
	 *
	 * \~
	 */
	typedef uint16_t uid_t;
	typedef uint16_t gid_t;

	/**
	 * \~russian
	 * Подключаем заголовочный файл типов происхождения BSD
	 *
	 * @details Заголовок этот объявляет u_char, u_short, u_int и u_long - типы, какими
	 *          библиотека пользуется повсюду. У MS Windows их тянет за собой winsock2.h,
	 *          а его здесь нет намеренно (пояснение выше, у AWH_ERROR), оттого он и
	 *          подключается прямо
	 *
	 * @warning Прежде тип u_char заводился здесь макросом под проверкой «#ifndef
	 *          u_char», и проверка эта была негодной по существу: система объявляет его
	 *          **типом**, а препроцессор типов не видит. Проверка проходила, макрос
	 *          заводился, и всякое последующее подключение winsock2.h обращало
	 *          системное объявление «typedef unsigned char u_char;» в «typedef unsigned
	 *          char unsigned char;». Сборка отвечала отказом «duplicate 'unsigned'», и
	 *          выстреливало это лишь при определённом порядке подключений - оттого
	 *          дефект и дожил до появления файла, подключившего заголовки в ином порядке
	 *
	 * @note Подключение это безопасно: заголовок несёт одни объявления типов, макросов
	 *       не заводит и защищён от повторного включения
	 *
	 * \~english
	 * Include the header file of the types of BSD origin
	 * @details That header declares u_char, u_short, u_int and u_long — the types the
	 *          library uses everywhere. On MS Windows they are dragged in by winsock2.h,
	 *          and it is absent here deliberately (the explanation is above, at AWH_ERROR), which is why it
	 *          is included directly
	 * @warning Before, the u_char type was introduced here by a macro under an «#ifndef
	 *          u_char» check, and that check was unfit in essence: the system declares it
	 *          as a **type**, while the preprocessor does not see types. The check passed, the macro
	 *          was introduced, and every subsequent include of winsock2.h turned the
	 *          system declaration «typedef unsigned char u_char;» into «typedef unsigned
	 *          char unsigned char;». The build answered with the failure «duplicate 'unsigned'», and
	 *          it fired only at a certain order of includes — that is why the defect lived
	 *          until a file appeared that included the headers in a different order
	 * @note That include is safe: the header carries type declarations alone, introduces no macros
	 *       and is guarded against repeated inclusion
	 *
	 * \~
	 */
	#include <_bsd_types.h>

	/**
	 * Устанавливаем функцию getpid
	 */
	#ifndef getpid
		#define getpid _getpid
	#endif

	/**
	 * Устанавливаем функцию getppid
	 */
	#ifndef getppid
		#define getppid GetCurrentProcessId
	#endif
/**
 * Для операционной системы не являющейся MS Windows
 */
#else
	/**
	 * Системные заголовочные файлы
	 */
	#include <sys/types.h>

	/**
	 * Заменяем переменную AWH ERROR
	 */
	#define AWH_ERROR() (errno)

	/**
	 * Файловый разделитель UNIX-подобных систем
	 */
	#define AWH_FS_SEPARATOR "/"
#endif

/**
 * Подключаем заголовочный файл проекта
 */
#include "log.hpp"

/**
 * Разрешаем сборку под Windows
 */
#include "global.hpp"

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
	 * @brief Класс работы с операционной системой
	 *
	 * \~english
	 * @brief Class for working with the operating system
	 *
	 * \~
	 */
	typedef class __AWH_SHARED_EXPORT__ Operating_System {
		private:
			// Объект логера
			const log_t * _log;
		public:
			/**
			 * \~russian
			 * @brief Режимы извлечения потребления памяти
			 *
			 * \~english
			 * @brief Modes of getting the memory consumption
			 *
			 * \~
			 */
			enum class rss_t : uint8_t {
				CURRENT = 0x00, // Текущее потребление памяти
				MAXIMUM = 0x01  // Максимальное потребление памяти
			};
			/**
			 * \~russian
			 * @brief Архитектура процессора на котором запущено приложение
			 *
			 * \~english
			 * @brief Architecture of the processor the application is running on
			 *
			 * \~
			 */
			enum class cpu_t : uint8_t {
				NONE    = 0x00, // Архитектура процессора не установлена
				X86     = 0x01, // Архитектура процессора принадлежит к i386
				ARM     = 0x02, // Архитектура процессора принадлежит к ARM32
				PPC     = 0x03, // Архитектура процессора принадлежит к PowerPC
				MIPS    = 0x04, // Архитектура процессора принадлежит к MIPS
				ARM64   = 0x05, // Архитектура процессора принадлежит к ARM64
				AMD64   = 0x06, // Архитектура процессора принадлежит к AMD64
				UNKNOWN = 0x07  // Архитектура процессора не определён
			};
			/**
			 * \~russian
			 * @brief Семейство поддерживаемых операционных систем
			 *
			 * \~english
			 * @brief Family of the supported operating systems
			 *
			 * \~
			 */
			enum class family_t : uint8_t {
				NONE    = 0x00, // Операционная система не определена
				UNIX    = 0x01, // Операционная система Unix
				LINUX   = 0x02, // Операционная система Linux
				WIND32  = 0x03, // Операционная система Windows 32bit
				WIND64  = 0x04, // Операционная система Windows 64bit
				MACOSX  = 0x05, // Операционная система macOS
				NETBSD  = 0x06, // Операционная система NetBSD
				OPENBSD = 0x07, // Операционная система OpenBSD
				FREEBSD = 0x08, // Операционная система FreeBSD
				SOLARIS = 0x09, // Операционная система Sun Solaris
				ILLUMOS = 0x0A  // Операционная система (OpenIndiana, SmartOS, OmniOS)
			};
		public:
			/**
			 * \~russian
			 * @brief isAdmin Метод проверпи запущено ли приложение под суперпользователем
			 *
			 * @return результат проверки
			 *
			 * \~english
			 * @brief isAdmin Method of checking whether the application is running under the superuser
			 * @return result of the check
			 *
			 * \~
			 */
			bool isAdmin() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод определения операционной системы
			 *
			 * @return семейство операционных систем
			 *
			 * \~english
			 * @brief Method of determining the operating system
			 * @return family of the operating systems
			 *
			 * \~
			 */
			family_t family() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод определение архитектуры процессора
			 *
			 * @return архитектура процессора
			 *
			 * \~english
			 * @brief Method of determining the architecture of the processor
			 * @return architecture of the processor
			 *
			 * \~
			 */
			cpu_t architecture() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод определения текущего расхода памяти
			 *
			 * @param mode режим потребления памяти
			 * @return     размер расхода памяти
			 *
			 * \~english
			 * @brief Method of determining the current memory consumption
			 * @param mode mode of the memory consumption
			 * @return     size of the memory consumption
			 *
			 * \~
			 */
			size_t rss(const rss_t mode) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод вывода статистики расхода памяти
			 *
			 * \~english
			 * @brief Method of reporting the statistics of the memory consumption
			 *
			 * \~
			 */
			void printStatsMemory() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод очистки выделенной памяти
			 *
			 * \~english
			 * @brief Method of releasing the allocated memory
			 *
			 * \~
			 */
			void releaseFreeMemory() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод резервирования нужного размера памяти для всего приложения
			 *
			 * @param size размер резервированной памяти
			 * @return     результат выполнения операции
			 *
			 * \~english
			 * @brief Method of reserving the required size of memory for the whole application
			 * @param size size of the reserved memory
			 * @return     result of performing the operation
			 *
			 * \~
			 */
			bool warmup(const size_t size) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод блокировки возвращения оперативной памяти системе
			 *
			 * @param mode флаг активации/деактивации
			 * @return     результат выполнения операции
			 *
			 * \~english
			 * @brief Method of blocking the return of the main memory to the system
			 * @param mode flag of enabling/disabling
			 * @return     result of performing the operation
			 *
			 * \~
			 */
			bool disableReturnMemory(const bool mode) const noexcept;
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#if !_WIN32 && !_WIN64
		public:
			/**
			 * \~russian
			 * @brief Метод получения идентификатора текущего пользователя
			 *
			 * @return идентификатор текущего пользователя
			 *
			 * \~english
			 * @brief Method of getting the identifier of the current user
			 * @return identifier of the current user
			 *
			 * \~
			 */
			uid_t user() const noexcept;
			/**
			 * \~russian
			 * @brief Метод получения группы текущего пользователя
			 *
			 * @return идентификатор группы текущего пользователя
			 *
			 * \~english
			 * @brief Method of getting the group of the current user
			 * @return identifier of the group of the current user
			 *
			 * \~
			 */
			gid_t group() const noexcept;
			/**
			 * \~russian
			 * @brief Метод получения списка групп текущего пользователя
			 *
			 * @return список групп текущего пользователя
			 *
			 * \~english
			 * @brief Method of getting the list of groups of the current user
			 * @return list of groups of the current user
			 *
			 * \~
			 */
			vector <gid_t> groups() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения имени пользователя по его идентификатору
			 *
			 * @param uid идентификатор пользователя
			 * @return    имя запрашиваемого пользователя
			 *
			 * \~english
			 * @brief Method of getting the name of a user by their identifier
			 * @param uid identifier of the user
			 * @return    name of the requested user
			 *
			 * \~
			 */
			string user(const uid_t uid) const noexcept;
			/**
			 * \~russian
			 * @brief Метод получения группы пользователя по её идентификатору
			 *
			 * @param gid идентификатор группы пользователя
			 * @return    название группы пользователя
			 *
			 * \~english
			 * @brief Method of getting the group of a user by its identifier
			 * @param gid identifier of the group of the user
			 * @return    name of the group of the user
			 *
			 * \~
			 */
			string group(const gid_t gid) const noexcept;
			/**
			 * \~russian
			 * @brief Метод получения идентификатора группы пользователя
			 *
			 * @param name название группы пользователя
			 * @return     идентификатор группы пользователя
			 *
			 * \~english
			 * @brief Method of getting the identifier of the group of a user
			 * @param name name of the group of the user
			 * @return     identifier of the group of the user
			 *
			 * \~
			 */
			gid_t group(string_view name) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод вывода идентификатора пользователя
			 *
			 * @param name имя пользователя
			 * @return     полученный идентификатор пользователя
			 *
			 * \~english
			 * @brief Method of yielding the identifier of a user
			 * @param name name of the user
			 * @return     the obtained identifier of the user
			 *
			 * \~
			 */
			uid_t uid(string_view name) const noexcept;
			/**
			 * \~russian
			 * @brief Метод вывода идентификатора группы пользователя
			 *
			 * @param name имя пользователя
			 * @return     полученный идентификатор группы пользователя
			 *
			 * \~english
			 * @brief Method of yielding the identifier of the group of a user
			 * @param name name of the user
			 * @return     the obtained identifier of the group of the user
			 *
			 * \~
			 */
			gid_t gid(string_view name) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Получение списка групп пользователя
			 *
			 * @param user имя пользователя чьи группы следует получить
			 * @return     список групп пользователя
			 *
			 * \~english
			 * @brief Getting the list of groups of a user
			 * @param user name of the user whose groups should be obtained
			 * @return     list of groups of the user
			 *
			 * \~
			 */
			vector <gid_t> groups(string_view user) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод запуска приложения от имени указанного пользователя
			 *
			 * @param uid идентификатор пользователя
			 * @return    результат выполнения операции
			 *
			 * \~english
			 * @brief Method of running the application on behalf of the specified user
			 * @param uid identifier of the user
			 * @return    result of performing the operation
			 *
			 * \~
			 */
			bool chown(const uid_t uid) const noexcept;
			/**
			 * \~russian
			 * @brief Метод запуска приложения от имени указанного пользователя
			 *
			 * @param uid идентификатор пользователя
			 * @param gid идентификатор группы пользователя
			 * @return    результат выполнения операции
			 *
			 * \~english
			 * @brief Method of running the application on behalf of the specified user
			 * @param uid identifier of the user
			 * @param gid identifier of the group of the user
			 * @return    result of performing the operation
			 *
			 * \~
			 */
			bool chown(const uid_t uid, const gid_t gid) const noexcept;
			/**
			 * \~russian
			 * @brief Метод запуска приложения от имени указанного пользователя
			 *
			 * @param user  название пользователя
			 * @param group название группы пользователя
			 * @return      результат выполнения операции
			 *
			 * \~english
			 * @brief Method of running the application on behalf of the specified user
			 * @param user  name of the user
			 * @param group name of the group of the user
			 * @return      result of performing the operation
			 *
			 * \~
			 */
			bool chown(string_view user, string_view group = "") const noexcept;
	/**
	 * Для операционной системы MS Windows
	 */
	#else
		public:
			/**
			 * \~russian
			 * @brief Метод получения идентификатора текущего пользователя
			 *
			 * @return идентификатор текущего пользователя
			 *
			 * \~english
			 * @brief Method of getting the identifier of the current user
			 * @return identifier of the current user
			 *
			 * \~
			 */
			wstring user() const noexcept;
			/**
			 * \~russian
			 * @brief Метод получения списка групп текущего пользователя
			 *
			 * @return список групп текущего пользователя
			 *
			 * \~english
			 * @brief Method of getting the list of groups of the current user
			 * @return list of groups of the current user
			 *
			 * \~
			 */
			vector <wstring> groups() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения названия пользователя/группы по идентификатору
			 *
			 * @param sid идентификатор пользователя/группы
			 * @return    имя запрашиваемого пользователя/группы
			 *
			 * \~english
			 * @brief Method of getting the name of a user/group by its identifier
			 * @param sid identifier of the user/group
			 * @return    name of the requested user/group
			 *
			 * \~
			 */
			string account(wstring_view sid) const noexcept;
			/**
			 * \~russian
			 * @brief Метод вывода идентификатора пользователя/группы
			 *
			 * @param name название пользователя/группы
			 * @return     полученный идентификатор пользователя/группы
			 *
			 * \~english
			 * @brief Method of yielding the identifier of a user/group
			 * @param name name of the user/group
			 * @return     the obtained identifier of the user/group
			 *
			 * \~
			 */
			wstring account(string_view name) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Получение списка групп пользователя
			 *
			 * @param user имя пользователя чьи группы следует получить
			 * @return     список групп пользователя
			 *
			 * \~english
			 * @brief Getting the list of groups of a user
			 * @param user name of the user whose groups should be obtained
			 * @return     list of groups of the user
			 *
			 * \~
			 */
			vector <wstring> groups(string_view user) const noexcept;
	#endif
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#if !_WIN32 && !_WIN64
		public:
			/**
			 * \~russian
			 * @brief Шаблон метода извлечения настроек ядра операционной системы
			 *
			 * @tparam T Тип данных выводимого результата
			 *
			 * \~english
			 * @brief Template of the method of getting the settings of the kernel of the operating system
			 * @tparam T Data type of the yielded result
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод извлечения настроек ядра операционной системы
			 *
			 * @param name название записи для получения настроек
			 * @return     полученное значение записи
			 *
			 * \~english
			 * @brief Method of getting the settings of the kernel of the operating system
			 * @param name name of the record to get the settings of
			 * @return     the obtained value of the record
			 *
			 * \~
			 */
			T sysctl(string_view name) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Шаблон метода установки настроек ядра операционной системы
			 *
			 * @tparam T Тип данных для установки
			 *
			 * \~english
			 * @brief Template of the method of setting the settings of the kernel of the operating system
			 * @tparam T Data type to set
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод установки настроек ядра операционной системы
			 *
			 * @param name  название записи для установки настроек
			 * @param value значение записи для установки настроек
			 * @return      результат выполнения установки
			 *
			 * \~english
			 * @brief Method of setting the settings of the kernel of the operating system
			 * @param name  name of the record to set the settings of
			 * @param value value of the record to set the settings of
			 * @return      result of performing the setting
			 *
			 * \~
			 */
			bool sysctl(string_view name, const T value) const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки настроек ядра операционной системы
			 *
			 * @param name  название записи для установки настроек
			 * @param value значение записи для установки настроек
			 * @return      результат выполнения установки
			 *
			 * \~english
			 * @brief Method of setting the settings of the kernel of the operating system
			 * @param name  name of the record to set the settings of
			 * @param value value of the record to set the settings of
			 * @return      result of performing the setting
			 *
			 * \~
			 */
			bool sysctl(string_view name, string_view value) const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки настроек ядра операционной системы
			 *
			 * @param name  название записи для установки настроек
			 * @param value значение записи для установки настроек
			 * @return      результат выполнения установки
			 *
			 * \~english
			 * @brief Method of setting the settings of the kernel of the operating system
			 * @param name  name of the record to set the settings of
			 * @param value value of the record to set the settings of
			 * @return      result of performing the setting
			 *
			 * \~
			 */
			bool sysctl(string_view name, const char * value) const noexcept;
			/**
			 * \~russian
			 * @brief Шаблон метода установки настроек ядра операционной системы
			 *
			 * @tparam T Тип данных списка для установки
			 *
			 * \~english
			 * @brief Template of the method of setting the settings of the kernel of the operating system
			 * @tparam T Data type of the list to set
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод установки настроек ядра операционной системы
			 *
			 * @param name  название записи для установки настроек
			 * @param items значение записи для установки настроек
			 * @return      результат выполнения установки
			 *
			 * \~english
			 * @brief Method of setting the settings of the kernel of the operating system
			 * @param name  name of the record to set the settings of
			 * @param items value of the record to set the settings of
			 * @return      result of performing the setting
			 *
			 * \~
			 */
			bool sysctl(string_view name, const vector <T> & items) const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки настроек ядра операционной системы
			 *
			 * @param name  название записи для установки настроек
			 * @param items значение записи для установки настроек
			 * @return      результат выполнения установки
			 *
			 * \~english
			 * @brief Method of setting the settings of the kernel of the operating system
			 * @param name  name of the record to set the settings of
			 * @param items value of the record to set the settings of
			 * @return      result of performing the setting
			 *
			 * \~
			 */
			bool sysctl(string_view name, const vector <string> & items) const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки настроек ядра операционной системы
			 *
			 * @param name  название записи для установки настроек
			 * @param items значение записи для установки настроек
			 * @return      результат выполнения установки
			 *
			 * \~english
			 * @brief Method of setting the settings of the kernel of the operating system
			 * @param name  name of the record to set the settings of
			 * @param items value of the record to set the settings of
			 * @return      result of performing the setting
			 *
			 * \~
			 */
			bool sysctl(string_view name, const vector <const char *> & items) const noexcept;
	#endif
		public:
			/**
			 * \~russian
			 * @brief Метод запуска внешнего приложения
			 *
			 * @param cmd       команда запуска
			 * @param multiline данные должны вернутся многострочные
			 *
			 * \~english
			 * @brief Method of running an external application
			 * @param cmd       command of the run
			 * @param multiline the data must come back multiline
			 *
			 * \~
			 */
			string exec(string_view cmd, const bool multiline = true) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param log объект для работы с логами
			 *
			 * \~english
			 * @brief Constructor
			 * @param log object for working with logs
			 *
			 * \~
			 */
			explicit Operating_System(const log_t * log) noexcept : _log(log) {}
	} os_t;
};

#endif // __AWH_OPERATING_SYSTEM__
