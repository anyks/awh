/**
 * @file global.hpp
 * @date 2025-10-25
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
 * \~russian
 * @brief Заголовочный файл глобальных макросов сборки — определение атрибутов экспорта и импорта символов
 *        динамической библиотеки для MS Windows и остальных операционных систем, а также режима статической сборки
 *
 * \~english
 * @brief Header file of the global build macros — the definition of the export and import attributes of the symbols
 *        of a dynamic library for MS Windows and for the other operating systems, as well as of the static build mode
 *
 * \~
 *
 * @copyright Copyright © 2025
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_GLOBAL__
#define __AWH_GLOBAL__

/**
 * \~russian
 * @brief Совместимость с нативным MSVC
 *
 * @details Фреймворк везде полагает, что «Windows» - это MinGW: у того есть слой POSIX
 *          (unistd.h, ssize_t, pid_t) и синтаксис упаковки GCC. У нативного MSVC их нет
 *          вовсе, и слой этот вводится здесь, в базовом заголовке, единожды - вместо
 *          россыпи проверок `_MSC_VER` по десяткам мест. У MinGW и POSIX условие ложно,
 *          и ничего не меняется.
 *
 *          `ssize_t` берётся из `SSIZE_T` (BaseTsd.h), `pid_t` - `int` (его отдаёт
 *          `_getpid`), `getpid` приводится к `_getpid`.
 *
 * \~english
 * @brief Compatibility with the native MSVC
 *
 * \~
 */
#if defined(_MSC_VER)
	#include <BaseTsd.h>
	#include <process.h>
	#ifndef __AWH_MSVC_SSIZE_T__
		#define __AWH_MSVC_SSIZE_T__
		typedef SSIZE_T ssize_t;
	#endif
	#ifndef __AWH_MSVC_PID_T__
		#define __AWH_MSVC_PID_T__
		typedef int pid_t;
	#endif
	#ifndef getpid
		#define getpid _getpid
	#endif
#endif

/**
 * \~russian
 * @brief Приведение типов, проверяемое в отладочной сборке
 *
 * @details Отладочная сборка приводит тип с проверкой, выпускная - без неё. Так
 *          и задумано, и заменять приведение на проверяемое в обеих сборках не
 *          следует: отлаживаться нужно в отладочной сборке, а выпускная - это
 *          уже отлаженное, и от неё требуется наибольшая скорость. Приведение,
 *          уехавшее в выпуск неверным, есть ошибка разработчика, не поймавшего
 *          его в отладке, и проверка в выпуске её не исправит - лишь скроет.
 *
 * \~english
 * @brief Type cast checked in a debug build
 *
 * @details A debug build casts the type with a check, a release one without it. That is
 *          how it is meant, and the cast should not be replaced by a checked one in both builds:
 *          debugging is to be done in a debug build, while a release one is
 *          already debugged, and the greatest speed is required of it. A cast
 *          that went into the release wrong is a mistake of the developer who did not catch
 *          it in debugging, and a check in the release will not correct it — it will only hide it.
 *
 * \~
 */
#if DEBUG_MODE
	// Безопасное приведение типов с проверкой
	#define awh_cast dynamic_cast
/**
 * Если режим отладки не включён
 */
#else
	// Безопасное приведение типов без проверки
	#define awh_cast static_cast
#endif

/**
 * \~russian
 * Для операционной системы MS Windows
 *
 * @note Проверка ведётся через defined, а не значением: заголовок <windows.h> у mingw-w64
 *       определяет WIN32 пустым макросом, отчего условие вида «|| WIN32 ||» теряет операнд
 *       и сборка отвечает отказом «operator '||' has no right operand»
 *
 * \~english
 * For the MS Windows operating system
 *
 * @note The check is driven by defined rather than by the value: the <windows.h> header of mingw-w64
 *       defines WIN32 as an empty macro, which is why a condition of the «|| WIN32 ||» form loses an operand
 *       and the build answers with the failure «operator '||' has no right operand»
 *
 * \~
 */
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	// Экспортируем символы динамической библиотеки с видимостью по умолчанию
	#define __AWH_DECL_EXPORT__ __declspec(dllexport)
	// Импортируем символы динамической библиотеки с видимостью по умолчанию
	#define __AWH_DECL_IMPORT__ __declspec(dllimport)
/**
 * Для операционной системы не являющейся MS Windows
 */
#else
	// Экспортируем символы динамической библиотеки с видимостью по умолчанию
	#define __AWH_DECL_EXPORT__ __attribute__((visibility("default")))
	// Импортируем символы динамической библиотеки с видимостью по умолчанию
	#define __AWH_DECL_IMPORT__ __attribute__((visibility("default")))
#endif

/**
 * Если активирован экспорт динамической библиотеки
 */
#if __AWH_SHARED_LIBRARY_EXPORT__
	// Экспортируем символы динамической библиотеки
	#define __AWH_SHARED_EXPORT__ __AWH_DECL_EXPORT__
/**
 * Если активирован импорт динамической библиотеки
 */
#elif __AWH_SHARED_LIBRARY_IMPORT__
	// Импортируем символы динамической библиотеки
	#define __AWH_SHARED_EXPORT__ __AWH_DECL_IMPORT__
/**
 * Если мы работаем со статической библиотекой
 */
#else
	// Статическая сборка, экспортировать символы не требуется
	#define __AWH_SHARED_EXPORT__
#endif

/**
 * \~russian
 * Признак того, что size_t и ssize_t являются самостоятельными типами
 *
 * @details Шаблоны создаются явно, для каждого типа порознь. Обыкновенно size_t совпадает
 *          с uint64_t, а ssize_t с int64_t, и создание их отдельно оказалось бы повторным
 *          - сборка на это отвечает отказом. Но есть системы, где типы эти самостоятельны,
 *          и там создание их необходимо, иначе не найдётся тела.
 *
 *          Проверено опытом на девяти системах через std::is_same:
 *
 *          - самостоятельны: macOS, OpenBSD
 *          - совпадают: FreeBSD, NetBSD, Solaris, OpenIndiana, Linux, Windows
 *
 * @note Перечень выделен в отдельный признак затем, что прежде он переписывался в каждом
 *       условии порознь - тридцать три места в пяти файлах - и довод его не был записан
 *       нигде
 *
 * @warning В прежнем перечне стоял Linux, записанный как `__Linux__`. Признака такого не
 *          объявляет ни один компилятор - они дают `__linux__`, - оттого условие там было
 *          всегда ложно, и лишь этим сборка Linux и держалась: типы там совпадают, и
 *          создание их отдельно её бы сломало. Написание это исправлять **нельзя**,
 *          Linux следует убрать из перечня вовсе, что здесь и сделано
 *
 * \~english
 * Indication that size_t and ssize_t are types of their own
 *
 * @details The templates are instantiated explicitly, for every type separately. Ordinarily size_t coincides
 *          with uint64_t, and ssize_t with int64_t, and instantiating them separately would turn out to be a repetition
 *          — the build answers to that with a failure. But there are systems where those types are of their own,
 *          and there instantiating them is necessary, otherwise no body will be found.
 *
 *          Checked by experience on nine systems through std::is_same:
 *          - of their own: macOS, OpenBSD
 *          - coincide: FreeBSD, NetBSD, Solaris, OpenIndiana, Linux, Windows
 *
 * @note The list is set apart as a separate indication because before it was rewritten in every
 *       condition separately — thirty-three places in five files — and its ground was written down
 *       nowhere
 *
 * @warning The former list held Linux, written as `__Linux__`. No compiler declares such an
 *          indication — they give `__linux__` — which is why the condition there was
 *          always false, and it is by that alone that the Linux build held: the types coincide there, and
 *          instantiating them separately would have broken it. That spelling **must not** be corrected,
 *          Linux should be removed from the list altogether, which is what is done here
 *
 * \~
 */
#if __APPLE__ || __MACH__ || __OpenBSD__
	#define __AWH_DISTINCT_SIZE_TYPES__ 1
#endif

/**
 * \~russian
 * Признак того, что ядро само разводит подключения между процессами кластера
 *
 * @details Кластер держит по слушающему сокету на процесс, все они встают на один и тот
 *          же порт, а ядро раздаёт им входящие подключения. Так поступают лишь три
 *          системы, и признак этот выделен в отдельный макрос, чтобы перечень их лежал в
 *          одном месте, а не переписывался в каждом условии порознь.
 *
 *          Проверено опытом на стендах: три сокета встают на один порт, следом идут
 *          тридцать подключений, и считается, скольким сокетам они достались.
 *
 *          - Linux - разводит с версии 3.9
 *          - FreeBSD - разводит **лишь** через `SO_REUSEPORT_LB`, заведённую в 12.0;
 *            обычная `SO_REUSEPORT` там отдаёт все подключения последнему сокету
 *          - Solaris - разводит обычной `SO_REUSEPORT`, как и Linux
 *
 * @note Прочие системы кластера такого не несут, и включать его им во вред: подключения
 *       достанутся одному процессу, а прочие простоят впустую. macOS, NetBSD и OpenBSD
 *       настройку **объявляют**, привязку нескольких сокетов допускают, но не разводят
 *       ничего - все подключения идут последнему привязавшемуся. OpenIndiana же не
 *       объявляет её вовсе
 *
 * @warning Solaris и OpenIndiana здесь **расходятся**, хотя обе объявляют `__sun`, и
 *          различать их приходится по `__illumos__` - его объявляет лишь вторая. Это
 *          первое расхождение между ними, встреченное в работе: по SCTP они шли вместе
 *
 * @note MS Windows в перечень не входит намеренно и войти не может, но причина здесь
 *       не та, что у macOS, NetBSD и OpenBSD. Тем разводящей настройки тоже недостаёт,
 *       однако у них есть второй путь - слушающий сокет заводится единожды и достаётся
 *       дочерним процессам наследованием через `fork`, а ядро разводит подключения
 *       между теми, кто принимает на **одном и том же** сокете. У MS Windows нет ни
 *       того, ни другого: `SO_REUSEPORT` не объявлена Winsock вовсе, а прямого
 *       соответствия `fork` не существует
 *
 *       Проверено опытом на стенде Windows 10 x86-64, MinGW64: два слушателя с
 *       `SO_REUSEADDR` привязываются к одному порту без отказа, но все десять пробных
 *       подключений принял один из них, второй не получил ни одного. Иначе говоря,
 *       привязка проходит, а разведения не происходит, и опереться на неё нельзя
 *
 *       Путь для MS Windows - тот же по сути, что у macOS и OpenBSD: воркеры принимают
 *       на одном сокете. Достаётся он им не наследованием, а передачей через
 *       `WSADuplicateSocket` служебным сообщением по каналу кластера. Работа эта
 *       относится к `unit::Server` и ждёт готовности движка ввода-вывода, см.
 *       src/net/backend/win/README.md
 *
 * \~english
 * Indication that the kernel itself distributes the connections between the processes of the cluster
 *
 * @details The cluster keeps one listening socket per process, all of them stand on one and the same
 *          port, and the kernel hands the incoming connections out to them. Only three systems
 *          behave that way, and this indication is set apart as a separate macro so that their list lies in
 *          one place rather than being rewritten in every condition separately.
 *
 *          Checked by experience on the stands: three sockets stand on one port, then go
 *          thirty connections, and it is counted how many sockets got them.
 *
 *          - Linux — distributes since version 3.9
 *          - FreeBSD — distributes **only** through `SO_REUSEPORT_LB`, introduced in 12.0;
 *            the ordinary `SO_REUSEPORT` there gives all the connections to the last socket
 *          - Solaris — distributes by the ordinary `SO_REUSEPORT`, like Linux
 *
 * @note The other systems carry no such cluster, and enabling it for them is to their harm: the connections
 *       will go to one process, while the others will stand idle in vain. macOS, NetBSD and OpenBSD
 *       **declare** the setting, admit the binding of several sockets, but distribute
 *       nothing — all the connections go to the last one that bound. OpenIndiana, on the other hand, does not
 *       declare it at all
 *
 * @warning Solaris and OpenIndiana here **diverge**, although both declare `__sun`, and
 *          they have to be told apart by `__illumos__` — only the second one declares it. This is
 *          the first divergence between them met in the work: on SCTP they went together
 *
 * @note MS Windows is not in the list deliberately and cannot enter it, but the reason here
 *       is not the one of macOS, NetBSD and OpenBSD. Those also lack the distributing setting,
 *       however they have a second way — the listening socket is created once and is passed to
 *       the child processes by inheritance through `fork`, while the kernel distributes the connections
 *       between those accepting on **one and the same** socket. MS Windows has neither
 *       of the two: `SO_REUSEPORT` is not declared by Winsock at all, and a direct
 *       counterpart of `fork` does not exist
 *
 *       Checked by experience on the Windows 10 x86-64 stand, MinGW64: two listeners with
 *       `SO_REUSEADDR` bind to one port without a failure, but all ten trial
 *       connections were accepted by one of them, the second got not a single one. In other words,
 *       the binding goes through, while no distribution happens, and it cannot be relied upon
 *
 *       The way for MS Windows is in essence the same as for macOS and OpenBSD: the workers accept
 *       on one socket. They get it not by inheritance but by a hand-over through
 *       `WSADuplicateSocket` as a service message over the cluster channel. That work
 *       belongs to `unit::Server` and awaits the readiness of the input-output engine, see
 *       src/net/backend/win/README.md
 *
 * \~
 */
#if __linux__ || (__sun && !__illumos__)
	// Разводящая настройка заведена в Linux 3.9 и Solaris 11.4
	#define __AWH_CLUSTER_BALANCE__ 1
/**
 * \~russian
 * Число версии FreeBSD объявляет не компилятор, а заголовок: без его подключения
 * проверка версии молча не срабатывает, и разводящая настройка теряется
 *
 * \~english
 * The version number of FreeBSD is declared not by the compiler but by the header: without its inclusion
 * the version check silently fails, and the distributing setting is lost
 *
 * \~
 */
#elif __FreeBSD__
	/**
	 * Число версии FreeBSD объявляет не компилятор, а заголовок: без его подключения
	 * проверка версии молча не срабатывает, и разводящая настройка теряется
	 */
	#include <sys/param.h>
	/**
	 * Разводящая настройка заведена в FreeBSD 12.0 и выше
	 */
	#if __FreeBSD_version >= 1200000
		// Разводящая настройка заведена в FreeBSD 12.0 и выше
		#define __AWH_CLUSTER_BALANCE__ 1
	#endif
#endif

#endif // __AWH_GLOBAL__
