/**
 * @file: os.hpp
 * @date: 2024-04-02
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

#ifndef __AWH_OPERATING_SYSTEM__
#define __AWH_OPERATING_SYSTEM__

/**
 * Стандартные модули
 */
#include <memory>
#include <vector>
#include <string>
#include <cstring>

/**
 * Для оснастки MSVC, не несущей слоя POSIX
 */
#if defined(_MSC_VER)
	/**
	 * Подключаем стандартные модули
	 */
	#include <io.h>
	#include <process.h>
/**
 * Для оснастки, несущей слой POSIX
 */
#else
	/**
	 * Подключаем стандартные модули
	 */
	#include <unistd.h>
#endif

/**
 * Системный заголовок
 */
#include <stdint.h>

// Если используется BOOST
#ifdef USE_BOOST_CONVERT
	#include <boost/locale/encoding_utf.hpp>
// Если нужно использовать стандартную библиотеку
#else
	#include <codecvt>
#endif

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
	 * Подключаем основные заголовочные файлы
	 *
	 * Порядок обязателен: winsock2.h идёт строго до windows.h, иначе windows.h
	 * втягивает устаревший winsock.h, и объявления сокетов сталкиваются -
	 * оснастка MSVC отвечает на это переопределением типов sockaddr и fd_set
	 */
	#include <winsock2.h>
	#include <ws2tcpip.h>
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
	 * Для оснастки MSVC, не несущей словаря POSIX
	 *
	 * Заголовок sys/stat.h у неё есть, но типа mode_t и признаков вида файла
	 * S_ISDIR и подобных он не даёт: их несёт слой POSIX, которого оснастка не
	 * имеет. MinGW отдаёт их вместе с оснасткой, потому слой нужен только MSVC.
	 *
	 * Числа взяты у MinGW, чтобы обе оснастки Windows вели себя одинаково:
	 * mode_t там шестнадцатиразрядный, а вид блочного устройства равен 0x6000
	 */
	#if defined(_MSC_VER)
		/**
		 * Подключаем заголовок метаданных файла
		 */
		#include <sys/stat.h>
		/**
		 * Своего mode_t у MSVC нет, потому тип заводим под охраной признака
		 */
		#if !defined(__AWH_MSVC_MODE_T__)
			/**
			 * Признак не даёт объявить тип повторно: заголовок подключают многие
			 */
			#define __AWH_MSVC_MODE_T__
			// mode_t берётся меры MinGW: unsigned short
			typedef unsigned short mode_t;
		#endif
		/**
		 * Вида блочного устройства в заголовке оснастки нет, а разбор его спрашивает
		 */
		#ifndef _S_IFBLK
			/**
			 * Число то же, что у MinGW: обе оснастки Windows узнают вид одинаково
			 */
			#define _S_IFBLK 0x6000
		#endif
		/**
		 * Признак каталога объявляем, если его ещё нет
		 */
		#if !defined(S_ISDIR)
			/**
			 * Каталог узнаём по разряду _S_IFDIR в поле вида _S_IFMT
			 */
			#define S_ISDIR(mode) (((mode) & _S_IFMT) == _S_IFDIR)
		#endif
		/**
		 * Признак обычного файла объявляем, если его ещё нет
		 */
		#if !defined(S_ISREG)
			/**
			 * Обычный файл узнаём по разряду _S_IFREG в поле вида _S_IFMT
			 */
			#define S_ISREG(mode) (((mode) & _S_IFMT) == _S_IFREG)
		#endif
		/**
		 * Признак символьного устройства объявляем, если его ещё нет
		 */
		#if !defined(S_ISCHR)
			/**
			 * Символьное устройство узнаём по разряду _S_IFCHR в поле вида _S_IFMT
			 */
			#define S_ISCHR(mode) (((mode) & _S_IFMT) == _S_IFCHR)
		#endif
		/**
		 * Признак блочного устройства объявляем, если его ещё нет
		 */
		#if !defined(S_ISBLK)
			/**
			 * Блочное устройство узнаём по разряду _S_IFBLK в поле вида _S_IFMT
			 */
			#define S_ISBLK(mode) (((mode) & _S_IFMT) == _S_IFBLK)
		#endif
		/**
		 * Признак именованного канала объявляем, если его ещё нет
		 */
		#if !defined(S_ISFIFO)
			/**
			 * Именованный канал узнаём по разряду _S_IFIFO в поле вида _S_IFMT
			 */
			#define S_ISFIFO(mode) (((mode) & _S_IFMT) == _S_IFIFO)
		#endif
	#endif
/**
 * Для операционной системы не являющейся MS Windows
 */
#else
	/**
	 * Подключаем основные заголовочные файлы
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
 * Разрешаем сборку под Windows
 */
#include "global.hpp"

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
	 * @brief Класс работы с операционной системой
	 *
	 */
	typedef class AWH_SHARED_EXPORT OS {
		public:
			/**
			 * Режимы извлечения потребления памяти
			 */
			enum class rss_t : uint8_t {
				CURRENT = 0x00, // Текущее потребление памяти
				MAXIMUM = 0x01  // Максимальное потребление памяти
			};
			/**
			 * Семейстов поддерживаемых операционных систем
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
				DFLYBSD = 0x09, // Операционная система DragonFly BSD
				SOLARIS = 0x0A, // Операционная система Sun Solaris
				ILLUMOS = 0x0B  // Операционная система (OpenIndiana, SmartOS, OmniOS)
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
			 * @return название операционной системы
			 */
			family_t family() const noexcept;
		public:
			/**
			 * @brief Метод определения текущего расхода памяти
			 *
			 * @param mode режим потребления памяти
			 * @return     размер расхода памяти
			 */
			size_t rss(const rss_t mode) const noexcept;
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#if !_WIN32 && !_WIN64
		public:
			/**
			 * @brief Метод получения идентификатора текущего пользвоателя
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
			gid_t group(const string & name) const noexcept;
		public:
			/**
			 * @brief Метод вывода идентификатора пользователя
			 *
			 * @param name имя пользователя
			 * @return     полученный идентификатор пользователя
			 */
			uid_t uid(const string & name) const noexcept;
			/**
			 * @brief Метод вывода идентификатора группы пользователя
			 *
			 * @param name имя пользователя
			 * @return     полученный идентификатор группы пользователя
			 */
			gid_t gid(const string & name) const noexcept;
		public:
			/**
			 * @brief Получение списка групп пользователя
			 *
			 * @param user имя пользователя чьи группы следует получить
			 * @return     список групп пользователя
			 */
			vector <gid_t> groups(const string & user) const noexcept;
		public:
			/**
			 * @brief Метод запуска приложения от имени указанного пользователя
			 *
			 * @param gid идентификатор группы пользователя
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
			bool chown(const string & user, const string & group = "") const noexcept;
	/**
	 * Для операционной системы MS Windows
	 */
	#else
		public:
			/**
			 * @brief Метод получения идентификатора текущего пользвоателя
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
			string account(const wstring & sid) const noexcept;
			/**
			 * @brief Метод вывода идентификатора пользователя/группы
			 *
			 * @param name название пользователя/группы
			 * @return     полученный идентификатор пользователя/группы
			 */
			wstring account(const string & name) const noexcept;
		public:
			/**
			 * @brief Получение списка групп пользователя
			 *
			 * @param user имя пользователя чьи группы следует получить
			 * @return     список групп пользователя
			 */
			vector <wstring> groups(const string & user) const noexcept;
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
			T sysctl(const string & name) const noexcept;
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
			bool sysctl(const string & name, const T value) const noexcept;
			/**
			 * @brief Метод установки настроек ядра операционной системы
			 *
			 * @param name  название записи для установки настроек
			 * @param value значение записи для установки настроек
			 * @return      результат выполнения установки
			 */
			bool sysctl(const string & name, const string & value) const noexcept;
			/**
			 * @brief Метод установки настроек ядра операционной системы
			 *
			 * @param name  название записи для установки настроек
			 * @param value значение записи для установки настроек
			 * @return      результат выполнения установки
			 */
			bool sysctl(const string & name, const char * value) const noexcept;
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
			bool sysctl(const string & name, const vector <T> & items) const noexcept;
			/**
			 * @brief Метод установки настроек ядра операционной системы
			 *
			 * @param name  название записи для установки настроек
			 * @param items значение записи для установки настроек
			 * @return      результат выполнения установки
			 */
			bool sysctl(const string & name, const vector <string> & items) const noexcept;
			/**
			 * @brief Метод установки настроек ядра операционной системы
			 *
			 * @param name  название записи для установки настроек
			 * @param items значение записи для установки настроек
			 * @return      результат выполнения установки
			 */
			bool sysctl(const string & name, const vector <const char *> & items) const noexcept;
	#endif
		public:
			/**
			 * @brief Метод запуска внешнего приложения
			 *
			 * @param cmd       команда запуска
			 * @param multiline данные должны вернутся многострочные
			 */
			string exec(const string & cmd, const bool multiline = true) const noexcept;
	} os_t;
};

#endif // __AWH_OPERATING_SYSTEM__
