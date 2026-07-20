/**
 * @file: os.hpp
 * @date: 2025-10-25
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
	 * Системные заголовочные файлы
	 */
	#include <windows.h>
	#include <sddl.h>
	#include <process.h>
	#include <processthreadsapi.h>

	/**
	 * Заменяем переменную AWH ERROR
	 */
	#define AWH_ERROR() (::WSAGetLastError())

	/**
	 * Файловый разделитель Windows
	 */
	#define AWH_FS_SEPARATOR "\\"

	/**
	 * Устанавливаем тип данных uid_t
	 */
	#ifndef uid_t
		#define uid_t uint16_t
	#endif

	/**
	 * Устанавливаем тип данных gid_t
	 */
	#ifndef gid_t
		#define gid_t uint16_t
	#endif

	/**
	 * Устанавливаем тип данных u_char
	 */
	#ifndef u_char
		#define u_char unsigned char
	#endif

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
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Класс работы с операционной системой
	 *
	 */
	typedef class __AWH_SHARED_EXPORT__ Operating_System {
		private:
			// Объект логера
			const log_t * _log;
		public:
			/**
			 * @brief Режимы извлечения потребления памяти
			 *
			 */
			enum class rss_t : uint8_t {
				CURRENT = 0x00, // Текущее потребление памяти
				MAXIMUM = 0x01  // Максимальное потребление памяти
			};
			/**
			 * @brief Архитектура процессора на котором запущено приложение
			 *
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
			 * @brief Семейство поддерживаемых операционных систем
			 *
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
			 * @brief isAdmin Метод проверпи запущено ли приложение под суперпользователем
			 *
			 * @return результат проверки
			 */
			bool isAdmin() const noexcept;
		public:
			/**
			 * @brief Метод определения операционной системы
			 *
			 * @return семейство операционных систем
			 */
			family_t family() const noexcept;
		public:
			/**
			 * @brief Метод определение архитектуры процессора
			 *
			 * @return архитектура процессора
			 */
			cpu_t architecture() const noexcept;
		public:
			/**
			 * @brief Метод определения текущего расхода памяти
			 *
			 * @param mode режим потребления памяти
			 * @return     размер расхода памяти
			 */
			size_t rss(const rss_t mode) const noexcept;
		public:
			/**
			 * @brief Метод вывода статистики расхода памяти
			 *
			 */
			void printStatsMemory() const noexcept;
		public:
			/**
			 * @brief Метод очистки выделенной памяти
			 *
			 */
			void releaseFreeMemory() const noexcept;
		public:
			/**
			 * @brief Метод резервирования нужного размера памяти для всего приложения
			 *
			 * @param size размер резервированной памяти
			 * @return     результат выполнения операции
			 */
			bool warmup(const size_t size) const noexcept;
		public:
			/**
			 * @brief Метод блокировки возвращения оперативной памяти системе
			 *
			 * @param mode флаг активации/деактивации
			 * @return     результат выполнения операции
			 */
			bool disableReturnMemory(const bool mode) const noexcept;
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#if !_WIN32 && !_WIN64
		public:
			/**
			 * @brief Метод получения идентификатора текущего пользователя
			 *
			 * @return идентификатор текущего пользователя
			 */
			uid_t user() const noexcept;
			/**
			 * @brief Метод получения группы текущего пользователя
			 *
			 * @return идентификатор группы текущего пользователя
			 */
			gid_t group() const noexcept;
			/**
			 * @brief Метод получения списка групп текущего пользователя
			 *
			 * @return список групп текущего пользователя
			 */
			vector <gid_t> groups() const noexcept;
		public:
			/**
			 * @brief Метод получения имени пользователя по его идентификатору
			 *
			 * @param uid идентификатор пользователя
			 * @return    имя запрашиваемого пользователя
			 */
			string user(const uid_t uid) const noexcept;
			/**
			 * @brief Метод получения группы пользователя по её идентификатору
			 *
			 * @param gid идентификатор группы пользователя
			 * @return    название группы пользователя
			 */
			string group(const gid_t gid) const noexcept;
			/**
			 * @brief Метод получения идентификатора группы пользователя
			 *
			 * @param name название группы пользователя
			 * @return     идентификатор группы пользователя
			 */
			gid_t group(string_view name) const noexcept;
		public:
			/**
			 * @brief Метод вывода идентификатора пользователя
			 *
			 * @param name имя пользователя
			 * @return     полученный идентификатор пользователя
			 */
			uid_t uid(string_view name) const noexcept;
			/**
			 * @brief Метод вывода идентификатора группы пользователя
			 *
			 * @param name имя пользователя
			 * @return     полученный идентификатор группы пользователя
			 */
			gid_t gid(string_view name) const noexcept;
		public:
			/**
			 * @brief Получение списка групп пользователя
			 *
			 * @param user имя пользователя чьи группы следует получить
			 * @return     список групп пользователя
			 */
			vector <gid_t> groups(string_view user) const noexcept;
		public:
			/**
			 * @brief Метод запуска приложения от имени указанного пользователя
			 *
			 * @param uid идентификатор пользователя
			 * @return    результат выполнения операции
			 */
			bool chown(const uid_t uid) const noexcept;
			/**
			 * @brief Метод запуска приложения от имени указанного пользователя
			 *
			 * @param uid идентификатор пользователя
			 * @param gid идентификатор группы пользователя
			 * @return    результат выполнения операции
			 */
			bool chown(const uid_t uid, const gid_t gid) const noexcept;
			/**
			 * @brief Метод запуска приложения от имени указанного пользователя
			 *
			 * @param user  название пользователя
			 * @param group название группы пользователя
			 * @return      результат выполнения операции
			 */
			bool chown(string_view user, string_view group = "") const noexcept;
	/**
	 * Для операционной системы MS Windows
	 */
	#else
		public:
			/**
			 * @brief Метод получения идентификатора текущего пользователя
			 *
			 * @return идентификатор текущего пользователя
			 */
			wstring user() const noexcept;
			/**
			 * @brief Метод получения списка групп текущего пользователя
			 *
			 * @return список групп текущего пользователя
			 */
			vector <wstring> groups() const noexcept;
		public:
			/**
			 * @brief Метод получения названия пользователя/группы по идентификатору
			 *
			 * @param sid идентификатор пользователя/группы
			 * @return    имя запрашиваемого пользователя/группы
			 */
			string account(wstring_view sid) const noexcept;
			/**
			 * @brief Метод вывода идентификатора пользователя/группы
			 *
			 * @param name название пользователя/группы
			 * @return     полученный идентификатор пользователя/группы
			 */
			wstring account(string_view name) const noexcept;
		public:
			/**
			 * @brief Получение списка групп пользователя
			 *
			 * @param user имя пользователя чьи группы следует получить
			 * @return     список групп пользователя
			 */
			vector <wstring> groups(string_view user) const noexcept;
	#endif
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#if !_WIN32 && !_WIN64
		public:
			/**
			 * @brief Шаблон метода извлечения настроек ядра операционной системы
			 *
			 * @tparam T Тип данных выводимого результата
			 */
			template <typename T>
			/**
			 * @brief Метод извлечения настроек ядра операционной системы
			 *
			 * @param name название записи для получения настроек
			 * @return     полученное значение записи
			 */
			T sysctl(string_view name) const noexcept;
		public:
			/**
			 * @brief Шаблон метода установки настроек ядра операционной системы
			 *
			 * @tparam T Тип данных для установки
			 */
			template <typename T>
			/**
			 * @brief Метод установки настроек ядра операционной системы
			 *
			 * @param name  название записи для установки настроек
			 * @param value значение записи для установки настроек
			 * @return      результат выполнения установки
			 */
			bool sysctl(string_view name, const T value) const noexcept;
			/**
			 * @brief Метод установки настроек ядра операционной системы
			 *
			 * @param name  название записи для установки настроек
			 * @param value значение записи для установки настроек
			 * @return      результат выполнения установки
			 */
			bool sysctl(string_view name, string_view value) const noexcept;
			/**
			 * @brief Метод установки настроек ядра операционной системы
			 *
			 * @param name  название записи для установки настроек
			 * @param value значение записи для установки настроек
			 * @return      результат выполнения установки
			 */
			bool sysctl(string_view name, const char * value) const noexcept;
			/**
			 * @brief Шаблон метода установки настроек ядра операционной системы
			 *
			 * @tparam T Тип данных списка для установки
			 */
			template <typename T>
			/**
			 * @brief Метод установки настроек ядра операционной системы
			 *
			 * @param name  название записи для установки настроек
			 * @param items значение записи для установки настроек
			 * @return      результат выполнения установки
			 */
			bool sysctl(string_view name, const vector <T> & items) const noexcept;
			/**
			 * @brief Метод установки настроек ядра операционной системы
			 *
			 * @param name  название записи для установки настроек
			 * @param items значение записи для установки настроек
			 * @return      результат выполнения установки
			 */
			bool sysctl(string_view name, const vector <string> & items) const noexcept;
			/**
			 * @brief Метод установки настроек ядра операционной системы
			 *
			 * @param name  название записи для установки настроек
			 * @param items значение записи для установки настроек
			 * @return      результат выполнения установки
			 */
			bool sysctl(string_view name, const vector <const char *> & items) const noexcept;
	#endif
		public:
			/**
			 * @brief Метод запуска внешнего приложения
			 *
			 * @param cmd       команда запуска
			 * @param multiline данные должны вернутся многострочные
			 */
			string exec(string_view cmd, const bool multiline = true) const noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param log объект для работы с логами
			 */
			explicit Operating_System(const log_t * log) noexcept : _log(log) {}
	} os_t;
};

#endif // __AWH_OPERATING_SYSTEM__
