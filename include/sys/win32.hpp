/**
 * @file win32.hpp
 * @date 2026-08-05
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
 * @brief Заголовочный файл единой точки подключения системных заголовков MS Windows —
 *        закрепляет обязательный порядок подключения и урезает состав заголовков
 *
 * \~english
 * @brief Header file of the single point of inclusion of the MS Windows system headers —
 *        it fixes the mandatory order of inclusion and trims the composition of the headers
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_WIN32__
#define __AWH_WIN32__

/**
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64

	/**
	 * \~russian
	 * @brief Наименьшая поддерживаемая версия операционной системы — Windows 10
	 *
	 * @details Версия задаётся до подключения системных заголовков: от неё зависит состав
	 *          объявлений. Windows 10 выбрана затем, что кластеру нужны объекты задания
	 *          с завершением по закрытию, ожидание объектов из системного пула потоков и
	 *          именованные каналы с отказом удалённым клиентам.
	 *
	 * \~english
	 * @brief Lowest supported version of the operating system — Windows 10
	 *
	 * @details The version is set before the inclusion of the system headers: the composition of the
	 *          declarations depends on it. Windows 10 is chosen because the cluster needs job objects
	 *          with termination on close, the waiting for objects from the system thread pool and
	 *          named pipes with a refusal to remote clients.
	 *
	 * \~
	 */
	#ifndef _WIN32_WINNT
		#define _WIN32_WINNT 0x0A00
	#endif

	/**
	 * \~russian
	 * @brief Урезание состава заголовка windows.h
	 *
	 * @details Снимает подключение подсистем, каких библиотеке не требуется — графики,
	 *          звука, буфера обмена, оболочки. Заодно убирает часть засорения
	 *          пространства имён макросами.
	 *
	 * \~english
	 * @brief Trimming of the composition of the windows.h header
	 *
	 * @details It removes the inclusion of the subsystems the library does not need — graphics,
	 *          sound, clipboard, shell. At the same time it removes part of the pollution
	 *          of the namespace by macros.
	 *
	 * \~
	 */
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif

	/**
	 * \~russian
	 * @brief Запрет макросов min и max
	 *
	 * @details Без него заголовки MS Windows заводят min и max макросами, и обращения
	 *          вида std::min<T>(a, b) перестают разбираться.
	 *
	 * \~english
	 * @brief Prohibition of the min and max macros
	 *
	 * @details Without it the MS Windows headers introduce min and max as macros, and references
	 *          of the std::min<T>(a, b) form stop being parsed.
	 *
	 * \~
	 */
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif

	/**
	 * \~russian
	 * @brief Подсистема графики GDI не отключается
	 *
	 * @details Выключатель NOGDI напрашивается: заголовок wingdi.h заводит макросы ERROR,
	 *          ALTERNATE и TRANSPARENT, и все три сталкиваются с именами членов
	 *          перечислений AWH. Пользоваться им, однако, нельзя - проверено опытом:
	 *          модуль fmk обращается к шрифту консоли через CONSOLE_FONT_INFOEX,
	 *          SetCurrentConsoleFontEx и FF_DONTCARE, а последняя есть постоянная GDI,
	 *          и с выключателем сборка отвечает отказом.
	 *
	 *          Столкновения снимает не выключатель, а пара sys/push.hpp и
	 *          sys/pop.hpp в самих заголовках AWH: она срабатывает при любом
	 *          порядке подключения, тогда как выключатель действует лишь когда
	 *          windows.h подключается впервые.
	 *
	 * @note Выключателем NO_STRICT, отменяющим макрос STRICT, пользоваться тоже не
	 *       следует: он превращает раздельные типы дескрипторов в общий void *, то есть
	 *       ослабляет проверку типов
	 *
	 * \~english
	 * @brief The GDI graphics subsystem is not switched off
	 *
	 * @details The NOGDI switch suggests itself: the wingdi.h header introduces the macros ERROR,
	 *          ALTERNATE and TRANSPARENT, and all three collide with the names of the members
	 *          of the AWH enumerations. Using it, however, is impossible — checked by experience:
	 *          the fmk module refers to the console font through CONSOLE_FONT_INFOEX,
	 *          SetCurrentConsoleFontEx and FF_DONTCARE, and the last one is a GDI constant,
	 *          and with the switch the build answers with a failure.
	 *
	 *          The collisions are removed not by the switch but by the pair sys/push.hpp and
	 *          sys/pop.hpp in the AWH headers themselves: it works at any
	 *          order of inclusion, whereas the switch takes effect only when
	 *          windows.h is included for the first time.
	 *
	 * @note The NO_STRICT switch, which cancels the STRICT macro, should not be used
	 *       either: it turns the separate handle types into a common void *, that is,
	 *       it weakens the type checking
	 *
	 * \~
	 */

	/**
	 * \~russian
	 * Системные заголовочные файлы
	 *
	 * @note Порядок обязателен: winsock2.h подключается строго до windows.h, иначе
	 *       windows.h втянет устаревший winsock.h и объявления сокетов столкнутся.
	 *       Заголовок ws2def.h самостоятельным не является и требует, чтобы базовые
	 *       типы (ULONG и прочие) были объявлены прежде него
	 *
	 * \~english
	 * System header files
	 *
	 * @note The order is mandatory: winsock2.h is included strictly before windows.h, otherwise
	 *       windows.h will drag in the outdated winsock.h and the socket declarations will collide.
	 *       The ws2def.h header is not self-sufficient and requires the basic
	 *       types (ULONG and others) to be declared before it
	 *
	 * \~
	 */
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <ws2def.h>
	#include <mswsock.h>
	#include <windows.h>

	/**
	 * \~russian
	 * @brief О снятии макросов MS Windows
	 *
	 * @details Снятия здесь нет намеренно, хотя напрашивается: системные заголовки
	 *          заводят макросами обычные английские слова - DELETE, ERROR, STRICT,
	 *          NO_ERROR, TEXT и прочие, - и те сталкиваются с именами членов
	 *          перечислений AWH.
	 *
	 *          Снимать их здесь и не нужно, и вредно. Не нужно потому, что заголовок
	 *          этот подключается лишь из файлов реализации, а заголовки AWH защищают
	 *          свои объявления сами - парой sys/push.hpp и sys/pop.hpp,
	 *          снимающей имена на время объявлений и возвращающей их следом.
	 *
	 *          Вредно потому, что файлы реализации макросами этими пользуются по делу:
	 *          модуль procre сличает итог вызовов MS Windows с NO_ERROR. Снятие
	 *          отняло бы у них имена, какими они вправе пользоваться - ровно то, в чём
	 *          упрекаем сами заголовки MS Windows.
	 *
	 * @note Проверено опытом: сборка с постоянным снятием отвечала отказом
	 *       "'NO_ERROR' was not declared in this scope" в четырёх местах src/sys/procre.cpp
	 *
	 * \~english
	 * @brief On the removal of the MS Windows macros
	 *
	 * @details There is no removal here deliberately, although it suggests itself: the system headers
	 *          introduce ordinary English words as macros — DELETE, ERROR, STRICT,
	 *          NO_ERROR, TEXT and others — and those collide with the names of the members
	 *          of the AWH enumerations.
	 *
	 *          Removing them here is both unnecessary and harmful. Unnecessary because this header
	 *          is included only from the implementation files, while the AWH headers protect
	 *          their declarations themselves — by the pair sys/push.hpp and sys/pop.hpp,
	 *          which removes the names for the duration of the declarations and brings them back afterwards.
	 *
	 *          Harmful because the implementation files use those macros for a reason:
	 *          the procre module compares the outcome of the MS Windows calls against NO_ERROR. A removal
	 *          would take away from them the names they are entitled to use — exactly what we
	 *          reproach the MS Windows headers themselves for.
	 *
	 * @note Checked by experience: a build with a permanent removal answered with the failure
	 *       "'NO_ERROR' was not declared in this scope" in four places of src/sys/procre.cpp
	 *
	 * \~
	 */

#endif

#endif // __AWH_WIN32__
