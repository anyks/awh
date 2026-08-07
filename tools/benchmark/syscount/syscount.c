/**
 * @file: syscount.c
 * @date: 2026-07-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Подставная библиотека счёта системных вызовов — перехватывает обращения измеряемого
 *        процесса к ядру, считает их количество и время и умеет выключать отдельные пути,
 *        чтобы измерить их стоимость разностью двух прогонов
 *
 * @details Инструмент отвечает на вопрос, на который профилировщик выборок не
 *          отвечает: сколько именно вызовов приходится на одну операцию и что
 *          изменится, если убрать вот этот путь. Счётчик вызовов детерминирован,
 *          в отличие от времени, поэтому именно он годится в пороги набора
 *          бенчмарков: время на незанятой машине шумит на единицы процентов, а на
 *          рабочей - на десятки, и порог по времени приходится задавать с
 *          многократным запасом, тогда как порог по количеству вызовов ловит
 *          лишнюю работу в тот же день, когда она появилась
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>

/**
 * Системные заголовочные файлы
 */
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <ifaddrs.h>
#include <sys/types.h>
#include <sys/socket.h>
/**
 * Заголовочный файл запроса параметров ядра
 *
 * @note Вызов этот принадлежит BSD. Linux его объявлял, но признал устаревшим и
 *       убрал вовсе - параметры ядра читаются там через `/proc` и netlink, - и
 *       заголовка у него нет. Перехватывать на Linux оттого нечего, а безусловное
 *       подключение стоило бы сборке отказа ещё до первой строки самого счётчика
 *
 */
#if !__linux__
	#include <sys/sysctl.h>
#endif

/**
 * Заголовочные файлы механизма подмены через LD_PRELOAD и очереди опроса Linux
 *
 * @note Разыскание подлинных функций нужно лишь этому способу подмены: у macOS
 *       подлинная функция остаётся доступной по своему имени
 */
#if __linux__
	#include <dlfcn.h>
	#include <sys/epoll.h>
#endif
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <netinet/in.h>

/**
 * Если операционной системой является macOS
 */
#if __APPLE__
	#include <sys/event.h>
#endif

/**
 * Подключаем заголовочный файл двоичного контракта счётчика
 */
#include "syscount.h"

/**
 * @brief Признак поддержки перехвата на текущей платформе
 *
 * @details Способов подмены два, и они не похожи один на другой.
 *
 *          macOS подменяет разделом `__interpose` загрузчика dyld: в объект кладётся
 *          таблица пар «подмена, подменяемое», и загрузчик переписывает переходы во
 *          всех прочих объектах. Подлинная функция при этом остаётся доступной по
 *          своему имени, и разыскивать её не требуется.
 *
 *          Linux подменяет совпадением имени при загрузке через `LD_PRELOAD`:
 *          подменяющая функция **носит то же имя**, что и подменяемая, и получает
 *          перед нею предпочтение. Подлинная функция по имени становится недоступна -
 *          вместо неё звалась бы сама подмена, - и добывается она разысканием в
 *          следующем объекте загрузки (`RTLD_NEXT`)
 *
 * @note Прочие системы остаются без перехвата. BSD подменяют тем же способом, что и
 *       Linux, и завести их будет нетрудно - но проверять это следует замером на них
 *       самих, а не рассуждением по сходству
 *
 */
#if __APPLE__ || __linux__
	#define AWH_SYSCOUNT_SUPPORTED 1
#else
	#define AWH_SYSCOUNT_SUPPORTED 0
#endif

/**
 * @brief Разновидности выключаемых путей
 *
 * @details Выключение пути не является его исправлением: подлинный вызов
 *          подменяется отказом, и это меняет поведение измеряемого процесса.
 *          Приём годится ровно для того, чтобы измерить стоимость пути
 *          разностью двух прогонов, и результат такого прогона нельзя
 *          приводить как показатель движка
 *
 */
enum {
	AWH_SYSCOUNT_DISABLE_UDP_PROBE   = 0x01, // Пробное подключение дейтаграммным сокетом ради своего адреса
	AWH_SYSCOUNT_DISABLE_ROUTE       = 0x02, // Обращения к маршрутной таблице и перечисление интерфейсов
	AWH_SYSCOUNT_DISABLE_READ_DRAIN  = 0x04  // Повторное чтение из сокета после короткого
};

/**
 * @brief Состояние счётчика системных вызовов
 *
 */
static awh_syscount_t __awh_syscount__ = {
	.version = AWH_SYSCOUNT_ABI_VERSION,
	.size = sizeof(awh_syscount_t),
	/**
	 * Учёт выключен до окончания загрузки образа и включается конструктором
	 * подставной библиотеки. Включать его статическим значением нельзя: подмена
	 * начинает действовать с момента применения раздела `__interpose`, то есть
	 * раньше, чем загрузчик успевает подготовить окружение времени выполнения, и
	 * обращения самого загрузчика к перехваченным функциям приводят к падению
	 * процесса. Дальше учёт остаётся включённым: эталонные стенды конкурентов о
	 * счётчике не знают и включать его не умеют, а показатели с них нужны для
	 * сравнения ровно так же, как со своего набора. Набор бенчмарков AWH, наоборот,
	 * обнуляет счётчики перед замером и выключает учёт после - тогда в счётчиках
	 * остаётся только окно замера, а подготовка сценария в них не попадает
	 */
	.enabled = 0,
	.disabled = 0,
	.managed = 0,
	.changes = 0,
	.peak = 0
};

/**
 * @brief Глубина вложенности перехваченных вызовов
 *
 * @details Часть библиотечных функций реализована через другие перехватываемые:
 *          `send` вызывает `sendto`, `recv` вызывает `recvfrom`, `getifaddrs`
 *          выполняет два `sysctl`. Без учёта вложенности один вызов попадал бы в
 *          счётчики дважды, а его время складывалось бы с временем вложенного, и
 *          сумма по счётчикам превышала бы время прогона. Учитывается только
 *          самый внешний вызов
 *
 * @note Переменная намеренно не является потоко-локальной. Хранилище потока в
 *       подставной библиотеке недоступно до окончания загрузки образа, а подмена
 *       действует с более раннего момента, поэтому обращение к нему приводит к
 *       падению процесса. Сценарии набора односоставны по потокам - обе стороны
 *       обмена обслуживаются одним циклом событий в одном потоке, - поэтому общий
 *       счётчик вложенности им достаточен. При измерении многопоточной нагрузки
 *       счётчики могут разойтись с действительностью на величину состязания, но
 *       не приведут к отказу
 *
 */
static int32_t __awh_depth__ = 0;

/**
 * @brief Признак короткого чтения по номеру файлового дескриптора
 *
 * @details Нужен единственному выключаемому пути - повторному чтению после
 *          короткого. Разрядность массива выбрана по практическому предельному
 *          числу дескрипторов процесса: дескрипторы за его границей учёту не
 *          подлежат, и путь для них просто не выключается
 *
 */
static uint8_t __awh_shortened__[65536] = {0};

/**
 * @brief Функция получения текущего времени в наносекундах
 *
 * @details Используется частная функция macOS вместо `clock_gettime`: последняя
 *          сама подлежит перехвату, и обращение к ней из перехватчика считалось
 *          бы за обращение измеряемого процесса
 *
 * @return текущее время в наносекундах
 *
 */
static inline uint64_t __awh_nanostamp__(void){
	/**
	 * Если операционной системой является macOS
	 */
	#if __APPLE__
		// Выводим текущее время монотонных часов
		return clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);
	/**
	 * Если это другая операционная система
	 */
	#else
		// Объект структуры для хранения времени
		struct timespec ts;
		// Получаем текущее время монотонных часов
		clock_gettime(CLOCK_MONOTONIC, &ts);
		// Выводим текущее время в наносекундах
		return (((uint64_t) ts.tv_sec * 1000000000ULL) + (uint64_t) ts.tv_nsec);
	#endif
}

/**
 * @brief Функция получения состояния счётчика системных вызовов
 *
 * @details Отыскивается измеряемым процессом через `dlsym` по названию из
 *          `AWH_SYSCOUNT_ENTRY_POINT`
 *
 * @return указатель на состояние счётчика
 *
 */
__attribute__((visibility("default")))
awh_syscount_t * awh_syscount_state(void){
	// Выводим состояние счётчика системных вызовов
	return &__awh_syscount__;
}

/**
 * @brief Функция вывода итогов счёта при завершении процесса
 *
 * @details Нужна эталонным стендам конкурентов: они о счётчике не знают и читать
 *          его не умеют, поэтому итоги выводятся в поток ошибок как есть
 *
 */
static void __awh_summary__(void){
	// Прекращаем учёт: вывод итогов сам выполняется системными вызовами
	__awh_syscount__.enabled = 0;
	/**
	 * Если пути выключались, предупреждение выводится всегда и раньше всего
	 * остального: прогон с выключенным путём приводить как показатель нельзя, и
	 * узнать об этом читатель обязан независимо от того, кто управлял учётом
	 */
	if(__awh_syscount__.disabled != 0)
		// Предупреждаем о недостоверности показателей прогона
		fprintf(stderr, "\nВНИМАНИЕ: пути выключены, показатели прогона недостоверны\n");
	// Если учётом управлял измеряемый процесс
	if(__awh_syscount__.managed)
		// Выходим из функции: в счётчиках осталось окно последнего замера
		return;
	// Суммарное количество системных вызовов
	uint64_t total = 0;
	/**
	 * Перебираем разновидности системных вызовов
	 */
	for(int32_t i = 0; i < AWH_SYSCOUNT_SYSCALLS; i++)
		// Суммируем количество выполненных вызовов
		total += __awh_syscount__.entries[i].calls;
	// Если вызовы не учитывались
	if((total == 0) && (__awh_syscount__.entries[AWH_SYSCOUNT_CLOCK].calls == 0))
		// Выходим из функции
		return;
	// Выводим заголовок итогов счёта
	fprintf(stderr, "\n=== системные вызовы ===\n");
	/**
	 * Перебираем все разновидности учитываемых вызовов
	 */
	for(int32_t i = 0; i < AWH_SYSCOUNT_MAX; i++){
		// Если вызовы этой разновидности не выполнялись
		if(__awh_syscount__.entries[i].calls == 0)
			// Переходим к следующей разновидности
			continue;
		// Выводим счётчик разновидности вызовов
		fprintf(
			stderr, "%-14s %10llu %10.1f мс %8.3f мкс/вызов\n",
			awh_syscount_name((awh_syscount_kind_t) i),
			(unsigned long long) __awh_syscount__.entries[i].calls,
			(double) __awh_syscount__.entries[i].nanoseconds / 1e6,
			((double) __awh_syscount__.entries[i].nanoseconds / 1e3) / (double) __awh_syscount__.entries[i].calls
		);
	}
	// Выводим суммарное количество системных вызовов
	fprintf(stderr, "%-14s %10llu\n", "всего", (unsigned long long) total);
	// Если изменения подписки передавались ядру
	if(__awh_syscount__.changes > 0)
		// Выводим сведения об изменениях подписки
		fprintf(
			stderr, "изменений подписки: %llu, наибольший пакет: %llu\n",
			(unsigned long long) __awh_syscount__.changes,
			(unsigned long long) __awh_syscount__.peak
		);
}

/**
 * @brief Функция разбора переменной окружения выключения путей
 *
 */
__attribute__((constructor))
static void __awh_initialize__(void){
	// Регистрируем вывод итогов счёта при завершении процесса
	atexit(&__awh_summary__);
	// Включаем учёт вызовов: окружение времени выполнения к этому моменту готово
	__awh_syscount__.enabled = 1;
	// Получаем список выключаемых путей
	const char * value = getenv(AWH_SYSCOUNT_DISABLE_VARIABLE);
	// Если список выключаемых путей не задан
	if((value == NULL) || (value[0] == '\0'))
		// Выходим из функции
		return;
	// Если требуется выключить пробное подключение дейтаграммным сокетом
	if(strstr(value, "udp-probe") != NULL)
		// Выключаем пробное подключение дейтаграммным сокетом
		__awh_syscount__.disabled |= AWH_SYSCOUNT_DISABLE_UDP_PROBE;
	// Если требуется выключить обращения к маршрутной таблице
	if(strstr(value, "route-sysctl") != NULL)
		// Выключаем обращения к маршрутной таблице
		__awh_syscount__.disabled |= AWH_SYSCOUNT_DISABLE_ROUTE;
	// Если требуется выключить повторное чтение после короткого
	if(strstr(value, "read-drain") != NULL)
		// Выключаем повторное чтение после короткого
		__awh_syscount__.disabled |= AWH_SYSCOUNT_DISABLE_READ_DRAIN;
}

/**
 * Если перехват на текущей платформе поддерживается
 */
#if AWH_SYSCOUNT_SUPPORTED

/**
 * @brief Макрос привязки подменяющей функции к подменяемой
 *
 * @details Загрузчик dyld читает раздел `__DATA,__interpose` как массив пар
 *          «подмена, подменяемое» и переписывает переходы во всех остальных
 *          объектах загрузки. Подлинная функция при этом остаётся доступной по
 *          своему имени, поэтому получать её через `dlsym` не требуется
 *
 */
#if __APPLE__
	#define AWH_SYSCOUNT_BIND(name) \
		__attribute__((used)) static struct { const void * replacement; const void * replacee; } \
		__awh_interpose_##name __attribute__((section("__DATA,__interpose"))) = \
		{ (const void *) (uintptr_t) &__awh_##name##__, (const void *) (uintptr_t) &name };
	/**
	 * @brief Имя подменяющей функции
	 *
	 * @note Загрузчик берёт подмену из таблицы, а не по имени, поэтому имя ей годится
	 *       любое собственное - и оно намеренно отлично от подменяемого
	 *
	 */
	#define AWH_SYSCOUNT_HOOK(name) __awh_##name##__
	/**
	 * @brief Обращение к подлинной функции
	 *
	 * @note Подлинная функция остаётся доступной по своему имени: таблица переписывает
	 *       переходы прочих объектов, а не саму подставную библиотеку
	 *
	 */
	#define AWH_SYSCOUNT_REAL(name) name
	// Признак необходимости разыскания подлинных функций
	#define AWH_SYSCOUNT_RESOLVE 0
#else
	/**
	 * @brief Макрос привязки подменяющей функции к подменяемой
	 *
	 * @details Привязки как отдельного действия здесь нет вовсе: подменяющая функция
	 *          носит имя подменяемой, и предпочтение ей отдаёт сам компоновщик времени
	 *          загрузки. Макрос заводит указатель на подлинную функцию - её и
	 *          разыскивает конструктор библиотеки
	 *
	 */
	#define AWH_SYSCOUNT_BIND(name) /* привязка выполняется совпадением имени */
	/**
	 * @brief Имя подменяющей функции
	 *
	 * @note Имя обязано в точности совпадать с подменяемым, иначе подмены не выйдет
	 *       вовсе, и видимость обязана быть открытой: библиотека собирается со
	 *       скрытой по умолчанию, а скрытый символ подменять нечего
	 *
	 */
	#define AWH_SYSCOUNT_HOOK(name) __attribute__((visibility("default"))) name
	/**
	 * @brief Обращение к подлинной функции
	 *
	 * @note Зовётся через указатель: по имени зовётся сама подмена, и обращение к нему
	 *       ушло бы в бесконечное самоповторение
	 *
	 */
	#define AWH_SYSCOUNT_REAL(name) __awh_real_##name
	// Признак необходимости разыскания подлинных функций
	#define AWH_SYSCOUNT_RESOLVE 1
#endif

/**
 * @brief Макрос объявления указателя на подлинную функцию
 *
 * @details Заводится лишь там, где подлинная функция по имени недоступна. У macOS
 *          макрос пуст: обращение к подлинной идёт прямо по имени
 *
 */
#if AWH_SYSCOUNT_RESOLVE
	#define AWH_SYSCOUNT_DECLARE(type, name, parameters) static type (* __awh_real_##name) parameters = NULL;
	/**
	 * @brief Макрос разыскания подлинной функции в следующем объекте загрузки
	 *
	 * @note Разыскание выполняется конструктором, до первого перехваченного обращения:
	 *       разыскивать при первом обращении значило бы звать `dlsym` с горячего пути,
	 *       а он и сам обращается к перехватываемым функциям
	 *
	 */
	#define AWH_SYSCOUNT_RESOLVE_ONE(name) \
		* (void **) (& __awh_real_##name) = dlsym(RTLD_NEXT, #name);
	/**
	 * @brief Макрос страховки на случай обращения прежде разыскания
	 *
	 * @note Конструкторы прочих объектов загрузки вправе обратиться к перехваченной
	 *       функции раньше, чем отработает наш, и указатель окажется пуст. Сличение
	 *       это стоит одного обращения к памяти и бережёт от падения на пустом
	 *       указателе - расход неотличим от нуля рядом с самим системным вызовом
	 *
	 */
	#define AWH_SYSCOUNT_ENSURE(name) \
		if(__awh_real_##name == NULL) AWH_SYSCOUNT_RESOLVE_ONE(name)
#else
	#define AWH_SYSCOUNT_DECLARE(type, name, parameters)
	#define AWH_SYSCOUNT_RESOLVE_ONE(name)
	#define AWH_SYSCOUNT_ENSURE(name)
#endif

/**
 * @brief Макрос объявления подменяющей функции с учётом вызова
 *
 * @details Вне окна замера и внутри уже учтённого вызова обращение передаётся
 *          подлинной функции без всякого учёта: расход перехватчика на горячем
 *          пути должен быть неотличим от нуля, когда учёт выключен
 *
 */
#define AWH_SYSCOUNT_WRAP(kind, type, name, parameters, arguments) \
	AWH_SYSCOUNT_DECLARE(type, name, parameters) \
	type AWH_SYSCOUNT_HOOK(name) parameters { \
		AWH_SYSCOUNT_ENSURE(name) \
		if(!__awh_syscount__.enabled || (__awh_depth__ > 0)) \
			return AWH_SYSCOUNT_REAL(name) arguments; \
		__awh_depth__++; \
		const uint64_t start = __awh_nanostamp__(); \
		type result = AWH_SYSCOUNT_REAL(name) arguments; \
		__awh_syscount__.entries[kind].nanoseconds += (__awh_nanostamp__() - start); \
		__awh_syscount__.entries[kind].calls++; \
		__awh_depth__--; \
		return result; \
	} \
	AWH_SYSCOUNT_BIND(name)

/**
 * @brief Подменяющая функция создания сокета
 *
 * @details Учитывается отдельно от остальных, потому что через неё выключается
 *          пробное подключение дейтаграммным сокетом: отказ в создании сокета
 *          прекращает опрос своего адреса в самом его начале, до подключения и
 *          перечисления интерфейсов
 *
 * @param domain   семейство адресов
 * @param type     тип сокета
 * @param protocol протокол
 * @return         файловый дескриптор сокета либо признак ошибки
 *
 */
AWH_SYSCOUNT_DECLARE(int32_t, socket, (int32_t domain, int32_t type, int32_t protocol))
int32_t AWH_SYSCOUNT_HOOK(socket)(int32_t domain, int32_t type, int32_t protocol){
	// Разыскиваем подлинную функцию, если конструктор ещё не отработал
	AWH_SYSCOUNT_ENSURE(socket)
	// Если требуется выключить пробное подключение дейтаграммным сокетом
	if((__awh_syscount__.disabled & AWH_SYSCOUNT_DISABLE_UDP_PROBE)
	 && ((domain == AF_INET) || (domain == AF_INET6))
	 && (type == SOCK_DGRAM) && (protocol == IPPROTO_IP)){
		// Устанавливаем признак отказа в доступе
		errno = EPERM;
		// Выводим признак ошибки создания сокета
		return -1;
	}
	// Если учёт вызовов не ведётся
	if(!__awh_syscount__.enabled || (__awh_depth__ > 0))
		// Выполняем создание сокета
		return AWH_SYSCOUNT_REAL(socket)(domain, type, protocol);
	// Увеличиваем глубину вложенности перехваченных вызовов
	__awh_depth__++;
	// Запоминаем время начала вызова
	const uint64_t start = __awh_nanostamp__();
	// Выполняем создание сокета
	const int32_t result = AWH_SYSCOUNT_REAL(socket)(domain, type, protocol);
	// Суммируем время, проведённое в вызове
	__awh_syscount__.entries[AWH_SYSCOUNT_SOCKET].nanoseconds += (__awh_nanostamp__() - start);
	// Считаем выполненный вызов
	__awh_syscount__.entries[AWH_SYSCOUNT_SOCKET].calls++;
	// Уменьшаем глубину вложенности перехваченных вызовов
	__awh_depth__--;
	// Выводим файловый дескриптор сокета
	return result;
}
AWH_SYSCOUNT_BIND(socket)

/**
 * Перехват запроса параметров ядра
 *
 * @note Вызова этого у Linux нет вовсе - перехватывать там нечего
 */
#if !__linux__
/**
 * @brief Подменяющая функция запроса параметров ядра
 *
 * @details Учитывается отдельно от остальных, потому что через неё выключаются
 *          обращения к маршрутной таблице: и выгрузка таблицы соседей, и
 *          перечисление интерфейсов внутри `getifaddrs` идут одним и тем же
 *          семейством запросов
 *
 * @param name      массив параметров запроса
 * @param length    длина массива параметров
 * @param output    буфер результата
 * @param outlength размер буфера результата
 * @param input     буфер устанавливаемого значения
 * @param inlength  размер буфера устанавливаемого значения
 * @return          результат выполнения запроса
 *
 */
AWH_SYSCOUNT_DECLARE(int32_t, sysctl, (int32_t * name, u_int length, void * output, size_t * outlength, void * input, size_t inlength))
int32_t AWH_SYSCOUNT_HOOK(sysctl)(int32_t * name, u_int length, void * output, size_t * outlength, void * input, size_t inlength){
	// Если требуется выключить обращения к маршрутной таблице
	if((__awh_syscount__.disabled & AWH_SYSCOUNT_DISABLE_ROUTE)
	 && (length >= 2) && (name != NULL) && (name[1] == PF_ROUTE)){
		// Устанавливаем признак отказа в доступе
		errno = EPERM;
		// Выводим признак ошибки выполнения запроса
		return -1;
	}
	// Если учёт вызовов не ведётся
	if(!__awh_syscount__.enabled || (__awh_depth__ > 0))
		// Выполняем запрос параметров ядра
		return AWH_SYSCOUNT_REAL(sysctl)(name, length, output, outlength, input, inlength);
	// Увеличиваем глубину вложенности перехваченных вызовов
	__awh_depth__++;
	// Запоминаем время начала вызова
	const uint64_t start = __awh_nanostamp__();
	// Выполняем запрос параметров ядра
	const int32_t result = AWH_SYSCOUNT_REAL(sysctl)(name, length, output, outlength, input, inlength);
	// Суммируем время, проведённое в вызове
	__awh_syscount__.entries[AWH_SYSCOUNT_SYSCTL].nanoseconds += (__awh_nanostamp__() - start);
	// Считаем выполненный вызов
	__awh_syscount__.entries[AWH_SYSCOUNT_SYSCTL].calls++;
	// Уменьшаем глубину вложенности перехваченных вызовов
	__awh_depth__--;
	// Выводим результат выполнения запроса
	return result;
}
AWH_SYSCOUNT_BIND(sysctl)
#endif // !__linux__

/**
 * @brief Подменяющая функция приёма данных из сокета
 *
 * @details Учитывается отдельно от остальных, потому что через неё выключается
 *          повторное чтение после короткого: подмена запоминает по дескриптору
 *          факт короткого чтения и на следующем обращении отвечает отказом, не
 *          обращаясь к ядру. Это в точности повторяет поведение движка, который
 *          выходил бы из цикла чтения по объёму доступных данных вместо пробного
 *          обращения
 *
 * @param fd     файловый дескриптор сокета
 * @param buffer буфер для приёма данных
 * @param length размер буфера
 * @param flags  флаги приёма
 * @return       количество принятых октетов либо признак ошибки
 *
 */
AWH_SYSCOUNT_DECLARE(ssize_t, recv, (int32_t fd, void * buffer, size_t length, int32_t flags))
ssize_t AWH_SYSCOUNT_HOOK(recv)(int32_t fd, void * buffer, size_t length, int32_t flags){
	// Разыскиваем подлинную функцию, если конструктор ещё не отработал
	AWH_SYSCOUNT_ENSURE(recv)
	// Признак учёта дескриптора в таблице коротких чтений
	const int32_t tracked = ((fd >= 0) && (fd < (int32_t) (sizeof(__awh_shortened__) / sizeof(__awh_shortened__[0]))));
	// Если требуется выключить повторное чтение после короткого
	if((__awh_syscount__.disabled & AWH_SYSCOUNT_DISABLE_READ_DRAIN) && tracked && __awh_shortened__[fd]){
		// Сбрасываем признак короткого чтения дескриптора
		__awh_shortened__[fd] = 0;
		// Устанавливаем признак отсутствия данных
		errno = EAGAIN;
		// Выводим признак ошибки приёма данных
		return -1;
	}
	// Количество принятых октетов
	ssize_t result = 0;
	// Если учёт вызовов не ведётся
	if(!__awh_syscount__.enabled || (__awh_depth__ > 0))
		// Выполняем приём данных из сокета
		result = AWH_SYSCOUNT_REAL(recv)(fd, buffer, length, flags);
	// Если учёт вызовов ведётся
	else {
		// Увеличиваем глубину вложенности перехваченных вызовов
		__awh_depth__++;
		// Запоминаем время начала вызова
		const uint64_t start = __awh_nanostamp__();
		// Выполняем приём данных из сокета
		result = AWH_SYSCOUNT_REAL(recv)(fd, buffer, length, flags);
		// Суммируем время, проведённое в вызове
		__awh_syscount__.entries[AWH_SYSCOUNT_RECV].nanoseconds += (__awh_nanostamp__() - start);
		// Считаем выполненный вызов
		__awh_syscount__.entries[AWH_SYSCOUNT_RECV].calls++;
		// Уменьшаем глубину вложенности перехваченных вызовов
		__awh_depth__--;
	}
	// Если дескриптор подлежит учёту в таблице коротких чтений
	if(tracked)
		// Запоминаем, было ли чтение короче запрошенного
		__awh_shortened__[fd] = (uint8_t) ((result > 0) && ((size_t) result < length));
	// Выводим количество принятых октетов
	return result;
}
AWH_SYSCOUNT_BIND(recv)

/**
 * Перехват ожидания готовности событий у kqueue
 *
 * @note Вызов этот принадлежит BSD и macOS. Правки подписок уходят у него тем же
 *       вызовом, что и ожидание, оттого оба и учитываются одной разновидностью
 */
#if __APPLE__
/**
 * @brief Подменяющая функция ожидания готовности событий
 *
 * @details Учитывается отдельно от остальных, потому что кроме самого вызова
 *          учитывает размер переданного ядру пакета изменений подписки. Именно
 *          он показывает, во что обходится отложенная регистрация событий: пакет
 *          в сто тысяч записей означает и линейный поиск по нему, и мебибайты
 *          памяти под него
 *
 * @param fd        файловый дескриптор очереди событий
 * @param changes   массив изменений подписки
 * @param nchanges  количество изменений подписки
 * @param events    массив готовых событий
 * @param nevents   размер массива готовых событий
 * @param timeout   предельное время ожидания
 * @return          количество готовых событий либо признак ошибки
 *
 */
AWH_SYSCOUNT_DECLARE(int32_t, kevent, (int32_t fd, const struct kevent * changes, int32_t nchanges, struct kevent * events, int32_t nevents, const struct timespec * timeout))
int32_t AWH_SYSCOUNT_HOOK(kevent)(int32_t fd, const struct kevent * changes, int32_t nchanges, struct kevent * events, int32_t nevents, const struct timespec * timeout){
	// Если учёт вызовов не ведётся
	if(!__awh_syscount__.enabled || (__awh_depth__ > 0))
		// Выполняем ожидание готовности событий
		return AWH_SYSCOUNT_REAL(kevent)(fd, changes, nchanges, events, nevents, timeout);
	// Увеличиваем глубину вложенности перехваченных вызовов
	__awh_depth__++;
	// Если ядру передаются изменения подписки
	if(nchanges > 0){
		// Суммируем количество переданных изменений подписки
		__awh_syscount__.changes += (uint64_t) nchanges;
		// Если пакет изменений оказался наибольшим
		if((uint64_t) nchanges > __awh_syscount__.peak)
			// Запоминаем наибольший размер пакета изменений
			__awh_syscount__.peak = (uint64_t) nchanges;
	}
	// Запоминаем время начала вызова
	const uint64_t start = __awh_nanostamp__();
	// Выполняем ожидание готовности событий
	const int32_t result = AWH_SYSCOUNT_REAL(kevent)(fd, changes, nchanges, events, nevents, timeout);
	// Суммируем время, проведённое в вызове
	__awh_syscount__.entries[AWH_SYSCOUNT_POLL].nanoseconds += (__awh_nanostamp__() - start);
	// Считаем выполненный вызов
	__awh_syscount__.entries[AWH_SYSCOUNT_POLL].calls++;
	// Уменьшаем глубину вложенности перехваченных вызовов
	__awh_depth__--;
	// Выводим количество готовых событий
	return result;
}
AWH_SYSCOUNT_BIND(kevent)
#endif // __APPLE__

/**
 * Перехват очереди опроса Linux
 *
 * @note Учитываются обе её стороны - ожидание и правка подписок - одной
 *       разновидностью. У kqueue они и вовсе выполняются одним вызовом, и сведение
 *       их вместе здесь тому и служит: показатель обращений к очереди опроса
 *       остаётся сопоставим между платформами, а не считает разное разным числом
 */
#if __linux__
	/**
	 * @brief Подменяющая функция ожидания готовности событий
	 *
	 * @param fd      дескриптор очереди опроса
	 * @param events  массив, в который складываются полученные события
	 * @param count   ёмкость массива событий
	 * @param timeout срок ожидания в миллисекундах
	 * @return        количество полученных событий
	 *
	 */
	AWH_SYSCOUNT_WRAP(AWH_SYSCOUNT_POLL, int32_t, epoll_wait,
	 (int32_t fd, struct epoll_event * events, int32_t count, int32_t timeout), (fd, events, count, timeout))

	/**
	 * @brief Подменяющая функция правки подписки в очереди опроса
	 *
	 * @param fd    дескриптор очереди опроса
	 * @param op    выполняемое действие
	 * @param sock  дескриптор, подписка которого правится
	 * @param event набор признаков ожидания
	 * @return      результат правки
	 *
	 */
	AWH_SYSCOUNT_WRAP(AWH_SYSCOUNT_POLL, int32_t, epoll_ctl,
	 (int32_t fd, int32_t op, int32_t sock, struct epoll_event * event), (fd, op, sock, event))

	/**
	 * @brief Подменяющая функция ожидания готовности событий с маской сигналов
	 *
	 * @note Заводится наравне с `epoll_wait`: часть библиотек зовёт эту разновидность
	 *       даже без маски сигналов, и без её перехвата их ожидания не были бы видны
	 *       вовсе - показания вышли бы заниженными молча
	 *
	 * @param fd      дескриптор очереди опроса
	 * @param events  массив, в который складываются полученные события
	 * @param count   ёмкость массива событий
	 * @param timeout срок ожидания в миллисекундах
	 * @param mask    маска блокируемых на время ожидания сигналов
	 * @return        количество полученных событий
	 *
	 */
	AWH_SYSCOUNT_WRAP(AWH_SYSCOUNT_POLL, int32_t, epoll_pwait,
	 (int32_t fd, struct epoll_event * events, int32_t count, int32_t timeout, const sigset_t * mask),
	 (fd, events, count, timeout, mask))

	/**
	 * @brief Подменяющая функция принятия входящего подключения с набором признаков
	 *
	 * @note Разновидность эта принадлежит Linux и заводит принятый сокет сразу с
	 *       нужными признаками, без отдельных обращений. Перехватывать её обязательно:
	 *       без этого приём подключений у пользующихся ею библиотек не виден вовсе
	 *
	 * @param fd     дескриптор слушающего сокета
	 * @param addr   адрес принятого подключения
	 * @param length размер структуры адреса
	 * @param flags  набор признаков заводимого сокета
	 * @return       дескриптор принятого подключения
	 *
	 */
	AWH_SYSCOUNT_WRAP(AWH_SYSCOUNT_ACCEPT, int32_t, accept4,
	 (int32_t fd, struct sockaddr * addr, socklen_t * length, int32_t flags), (fd, addr, length, flags))
#endif // __linux__

/**
 * @brief Подменяющая функция управления файловым дескриптором
 *
 * @details Объявлена с переменным числом параметров, поэтому обёрткой из макроса
 *          не описывается. Третий параметр передаётся дальше как указатель:
 *          на всех поддерживаемых платформах целое и указатель занимают в
 *          соглашении о вызове одно и то же место
 *
 * @param fd      файловый дескриптор
 * @param command выполняемая команда
 * @return        результат выполнения команды
 *
 */
AWH_SYSCOUNT_DECLARE(int32_t, fcntl, (int32_t fd, int32_t command, ...))
int32_t AWH_SYSCOUNT_HOOK(fcntl)(int32_t fd, int32_t command, ...){
	// Разыскиваем подлинную функцию, если конструктор ещё не отработал
	AWH_SYSCOUNT_ENSURE(fcntl)
	// Список параметров переменной длины
	va_list arguments;
	// Открываем список параметров переменной длины
	va_start(arguments, command);
	// Извлекаем параметр команды
	void * argument = va_arg(arguments, void *);
	// Закрываем список параметров переменной длины
	va_end(arguments);
	// Если учёт вызовов не ведётся
	if(!__awh_syscount__.enabled || (__awh_depth__ > 0))
		// Выполняем управление файловым дескриптором
		return AWH_SYSCOUNT_REAL(fcntl)(fd, command, argument);
	// Увеличиваем глубину вложенности перехваченных вызовов
	__awh_depth__++;
	// Запоминаем время начала вызова
	const uint64_t start = __awh_nanostamp__();
	// Выполняем управление файловым дескриптором
	const int32_t result = AWH_SYSCOUNT_REAL(fcntl)(fd, command, argument);
	// Суммируем время, проведённое в вызове
	__awh_syscount__.entries[AWH_SYSCOUNT_FCNTL].nanoseconds += (__awh_nanostamp__() - start);
	// Считаем выполненный вызов
	__awh_syscount__.entries[AWH_SYSCOUNT_FCNTL].calls++;
	// Уменьшаем глубину вложенности перехваченных вызовов
	__awh_depth__--;
	// Выводим результат выполнения команды
	return result;
}
AWH_SYSCOUNT_BIND(fcntl)

/**
 * @brief Подменяющая функция управления устройством
 *
 * @param fd      файловый дескриптор
 * @param request выполняемый запрос
 * @return        результат выполнения запроса
 *
 */
AWH_SYSCOUNT_DECLARE(int32_t, ioctl, (int32_t fd, unsigned long request, ...))
int32_t AWH_SYSCOUNT_HOOK(ioctl)(int32_t fd, unsigned long request, ...){
	// Разыскиваем подлинную функцию, если конструктор ещё не отработал
	AWH_SYSCOUNT_ENSURE(ioctl)
	// Список параметров переменной длины
	va_list arguments;
	// Открываем список параметров переменной длины
	va_start(arguments, request);
	// Извлекаем параметр запроса
	void * argument = va_arg(arguments, void *);
	// Закрываем список параметров переменной длины
	va_end(arguments);
	// Если учёт вызовов не ведётся
	if(!__awh_syscount__.enabled || (__awh_depth__ > 0))
		// Выполняем управление устройством
		return AWH_SYSCOUNT_REAL(ioctl)(fd, request, argument);
	// Увеличиваем глубину вложенности перехваченных вызовов
	__awh_depth__++;
	// Запоминаем время начала вызова
	const uint64_t start = __awh_nanostamp__();
	// Выполняем управление устройством
	const int32_t result = AWH_SYSCOUNT_REAL(ioctl)(fd, request, argument);
	// Суммируем время, проведённое в вызове
	__awh_syscount__.entries[AWH_SYSCOUNT_IOCTL].nanoseconds += (__awh_nanostamp__() - start);
	// Считаем выполненный вызов
	__awh_syscount__.entries[AWH_SYSCOUNT_IOCTL].calls++;
	// Уменьшаем глубину вложенности перехваченных вызовов
	__awh_depth__--;
	// Выводим результат выполнения запроса
	return result;
}
AWH_SYSCOUNT_BIND(ioctl)

/**
 * Описываем остальные перехватываемые вызовы обёрткой из макроса
 */
AWH_SYSCOUNT_WRAP(AWH_SYSCOUNT_CLOSE, int32_t, close, (int32_t fd), (fd))
AWH_SYSCOUNT_WRAP(AWH_SYSCOUNT_CONNECT, int32_t, connect, (int32_t fd, const struct sockaddr * addr, socklen_t length), (fd, addr, length))
AWH_SYSCOUNT_WRAP(AWH_SYSCOUNT_ACCEPT, int32_t, accept, (int32_t fd, struct sockaddr * addr, socklen_t * length), (fd, addr, length))
AWH_SYSCOUNT_WRAP(AWH_SYSCOUNT_BIND, int32_t, bind, (int32_t fd, const struct sockaddr * addr, socklen_t length), (fd, addr, length))
AWH_SYSCOUNT_WRAP(AWH_SYSCOUNT_LISTEN, int32_t, listen, (int32_t fd, int32_t backlog), (fd, backlog))
AWH_SYSCOUNT_WRAP(AWH_SYSCOUNT_SHUTDOWN, int32_t, shutdown, (int32_t fd, int32_t how), (fd, how))
AWH_SYSCOUNT_WRAP(AWH_SYSCOUNT_SETSOCKOPT, int32_t, setsockopt, (int32_t fd, int32_t level, int32_t option, const void * value, socklen_t length), (fd, level, option, value, length))
AWH_SYSCOUNT_WRAP(AWH_SYSCOUNT_GETSOCKOPT, int32_t, getsockopt, (int32_t fd, int32_t level, int32_t option, void * value, socklen_t * length), (fd, level, option, value, length))
AWH_SYSCOUNT_WRAP(AWH_SYSCOUNT_GETSOCKNAME, int32_t, getsockname, (int32_t fd, struct sockaddr * addr, socklen_t * length), (fd, addr, length))
AWH_SYSCOUNT_WRAP(AWH_SYSCOUNT_GETPEERNAME, int32_t, getpeername, (int32_t fd, struct sockaddr * addr, socklen_t * length), (fd, addr, length))
AWH_SYSCOUNT_WRAP(AWH_SYSCOUNT_GETIFADDRS, int32_t, getifaddrs, (struct ifaddrs ** addr), (addr))
AWH_SYSCOUNT_WRAP(AWH_SYSCOUNT_READ, ssize_t, read, (int32_t fd, void * buffer, size_t length), (fd, buffer, length))
AWH_SYSCOUNT_WRAP(AWH_SYSCOUNT_WRITE, ssize_t, write, (int32_t fd, const void * buffer, size_t length), (fd, buffer, length))
AWH_SYSCOUNT_WRAP(AWH_SYSCOUNT_SEND, ssize_t, send, (int32_t fd, const void * buffer, size_t length, int32_t flags), (fd, buffer, length, flags))
AWH_SYSCOUNT_WRAP(AWH_SYSCOUNT_RECVFROM, ssize_t, recvfrom, (int32_t fd, void * buffer, size_t length, int32_t flags, struct sockaddr * addr, socklen_t * addrlength), (fd, buffer, length, flags, addr, addrlength))
AWH_SYSCOUNT_WRAP(AWH_SYSCOUNT_SENDTO, ssize_t, sendto, (int32_t fd, const void * buffer, size_t length, int32_t flags, const struct sockaddr * addr, socklen_t addrlength), (fd, buffer, length, flags, addr, addrlength))
AWH_SYSCOUNT_WRAP(AWH_SYSCOUNT_RECVMSG, ssize_t, recvmsg, (int32_t fd, struct msghdr * message, int32_t flags), (fd, message, flags))
AWH_SYSCOUNT_WRAP(AWH_SYSCOUNT_SENDMSG, ssize_t, sendmsg, (int32_t fd, const struct msghdr * message, int32_t flags), (fd, message, flags))
AWH_SYSCOUNT_WRAP(AWH_SYSCOUNT_READV, ssize_t, readv, (int32_t fd, const struct iovec * vector, int32_t count), (fd, vector, count))
AWH_SYSCOUNT_WRAP(AWH_SYSCOUNT_WRITEV, ssize_t, writev, (int32_t fd, const struct iovec * vector, int32_t count), (fd, vector, count))
AWH_SYSCOUNT_WRAP(AWH_SYSCOUNT_CLOCK, int32_t, clock_gettime, (clockid_t id, struct timespec * ts), (id, ts))

/**
 * @brief Функция разыскания подлинных функций в следующем объекте загрузки
 *
 * @details Выполняется конструктором, до первого перехваченного обращения:
 *          разыскивать при обращении значило бы звать `dlsym` с горячего пути, а он
 *          и сам обращается к перехватываемым функциям
 *
 * @note Приоритет задан меньшим, чем у разбора окружения, чтобы указатели были
 *       заполнены прежде, чем учёт вообще включится
 *
 */
#if AWH_SYSCOUNT_RESOLVE
__attribute__((constructor(101)))
static void __awh_resolve__(void){
	// Разыскиваем подлинные функции работы с сокетами
	AWH_SYSCOUNT_RESOLVE_ONE(socket)
	AWH_SYSCOUNT_RESOLVE_ONE(close)
	AWH_SYSCOUNT_RESOLVE_ONE(connect)
	AWH_SYSCOUNT_RESOLVE_ONE(accept)
	AWH_SYSCOUNT_RESOLVE_ONE(bind)
	AWH_SYSCOUNT_RESOLVE_ONE(listen)
	AWH_SYSCOUNT_RESOLVE_ONE(shutdown)
	AWH_SYSCOUNT_RESOLVE_ONE(setsockopt)
	AWH_SYSCOUNT_RESOLVE_ONE(getsockopt)
	AWH_SYSCOUNT_RESOLVE_ONE(getsockname)
	AWH_SYSCOUNT_RESOLVE_ONE(getpeername)
	// Разыскиваем подлинные функции управления дескрипторами
	AWH_SYSCOUNT_RESOLVE_ONE(fcntl)
	AWH_SYSCOUNT_RESOLVE_ONE(ioctl)
	AWH_SYSCOUNT_RESOLVE_ONE(getifaddrs)
	// Разыскиваем подлинные функции обмена данными
	AWH_SYSCOUNT_RESOLVE_ONE(read)
	AWH_SYSCOUNT_RESOLVE_ONE(write)
	AWH_SYSCOUNT_RESOLVE_ONE(recv)
	AWH_SYSCOUNT_RESOLVE_ONE(send)
	AWH_SYSCOUNT_RESOLVE_ONE(recvfrom)
	AWH_SYSCOUNT_RESOLVE_ONE(sendto)
	AWH_SYSCOUNT_RESOLVE_ONE(recvmsg)
	AWH_SYSCOUNT_RESOLVE_ONE(sendmsg)
	AWH_SYSCOUNT_RESOLVE_ONE(readv)
	AWH_SYSCOUNT_RESOLVE_ONE(writev)
	// Разыскиваем подлинные функции очереди опроса
	AWH_SYSCOUNT_RESOLVE_ONE(epoll_wait)
	AWH_SYSCOUNT_RESOLVE_ONE(epoll_ctl)
	AWH_SYSCOUNT_RESOLVE_ONE(epoll_pwait)
	AWH_SYSCOUNT_RESOLVE_ONE(accept4)
	// Разыскиваем подлинную функцию обращения к часам
	AWH_SYSCOUNT_RESOLVE_ONE(clock_gettime)
}
#endif

#endif // AWH_SYSCOUNT_SUPPORTED
