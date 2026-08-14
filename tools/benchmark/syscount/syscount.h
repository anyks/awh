/**
 * @file syscount.h
 * @date 2026-07-26
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
 * @brief Заголовочный файл двоичного контракта счётчика системных вызовов — состав счётчиков
 *        и способ их получения измеряемым процессом
 *
 * @details Счётчик живёт в подставной библиотеке, внедряемой загрузчиком в
 *          измеряемый процесс, а читают его и набор бенчмарков AWH, и эталонные
 *          стенды конкурентов. Поэтому состав счётчиков описан отдельным
 *          заголовочным файлом на чистом C без зависимостей: он подключается и
 *          подставной библиотекой, и потребителем, и расхождение между ними
 *          становится ошибкой сборки, а не молчаливой порчей показателей
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_BENCHMARK_SYSCOUNT__
#define __AWH_BENCHMARK_SYSCOUNT__

/**
 * Стандартные заголовочные файлы
 */
#include <stdint.h>
#include <stddef.h>

/**
 * Если заголовочный файл подключается из C++
 */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Версия двоичного контракта счётчика
 *
 * @details Потребитель обязан сверить её со своей: подставная библиотека
 *          собирается отдельно от набора бенчмарков и может оказаться собранной
 *          из другой редакции исходного текста
 *
 */
#define AWH_SYSCOUNT_ABI_VERSION 1

/**
 * @brief Название функции получения состояния счётчика
 *
 * @details Потребитель отыскивает её через `dlsym`: отсутствие символа означает,
 *          что подставная библиотека не внедрена и показатели снять неоткуда
 *
 */
#define AWH_SYSCOUNT_ENTRY_POINT "awh_syscount_state"

/**
 * @brief Название переменной окружения выключения путей
 *
 * @details Принимает список названий путей через запятую. Выключение пути не
 *          является исправлением: оно нужно, чтобы измерить стоимость пути
 *          разностью двух прогонов
 *
 */
#define AWH_SYSCOUNT_DISABLE_VARIABLE "AWH_SYSCOUNT_DISABLE"

/**
 * @brief Разновидности учитываемых вызовов
 *
 * @details Порядок значений входит в двоичный контракт: потребитель обращается
 *          к счётчикам по этим индексам. Обращения ко времени стоят последними и
 *          в сумму системных вызовов не входят: на части платформ они
 *          обслуживаются без перехода в ядро, и складывать их с настоящими
 *          вызовами значило бы измерять разные вещи одним числом
 *
 */
typedef enum {
	AWH_SYSCOUNT_SOCKET = 0,     // Создание сокета
	AWH_SYSCOUNT_CLOSE,          // Закрытие файлового дескриптора
	AWH_SYSCOUNT_CONNECT,        // Подключение сокета
	AWH_SYSCOUNT_ACCEPT,         // Принятие входящего подключения
	AWH_SYSCOUNT_BIND,           // Привязка сокета к адресу
	AWH_SYSCOUNT_LISTEN,         // Перевод сокета в режим прослушивания
	AWH_SYSCOUNT_SHUTDOWN,       // Полузакрытие сокета
	AWH_SYSCOUNT_SETSOCKOPT,     // Установка параметра сокета
	AWH_SYSCOUNT_GETSOCKOPT,     // Чтение параметра сокета
	AWH_SYSCOUNT_GETSOCKNAME,    // Чтение локального адреса сокета
	AWH_SYSCOUNT_GETPEERNAME,    // Чтение адреса собеседника
	AWH_SYSCOUNT_FCNTL,          // Управление файловым дескриптором
	AWH_SYSCOUNT_IOCTL,          // Управление устройством
	AWH_SYSCOUNT_SYSCTL,         // Запрос параметров ядра
	AWH_SYSCOUNT_GETIFADDRS,     // Перечисление сетевых интерфейсов
	AWH_SYSCOUNT_POLL,           // Ожидание готовности событий
	AWH_SYSCOUNT_READ,           // Чтение файлового дескриптора
	AWH_SYSCOUNT_WRITE,          // Запись файлового дескриптора
	AWH_SYSCOUNT_RECV,           // Приём из сокета
	AWH_SYSCOUNT_SEND,           // Передача в сокет
	AWH_SYSCOUNT_RECVFROM,       // Приём дейтаграммы с адресом источника
	AWH_SYSCOUNT_SENDTO,         // Передача дейтаграммы по адресу
	AWH_SYSCOUNT_RECVMSG,        // Приём составного сообщения
	AWH_SYSCOUNT_SENDMSG,        // Передача составного сообщения
	AWH_SYSCOUNT_READV,          // Чтение в набор буферов
	AWH_SYSCOUNT_WRITEV,         // Запись из набора буферов
	AWH_SYSCOUNT_URING,          // Обращение к кольцам io_uring
	AWH_SYSCOUNT_SYSCALLS,       // Граница системных вызовов - служебное значение
	AWH_SYSCOUNT_CLOCK = AWH_SYSCOUNT_SYSCALLS, // Обращение к текущему времени
	AWH_SYSCOUNT_MAX             // Количество разновидностей - служебное значение
} awh_syscount_kind_t;

/**
 * @brief Структура счётчика одной разновидности вызовов
 *
 */
typedef struct {
	// Количество выполненных вызовов
	uint64_t calls;
	// Суммарное время, проведённое в вызовах, в наносекундах
	uint64_t nanoseconds;
} awh_syscount_entry_t;

/**
 * @brief Структура состояния счётчика системных вызовов
 *
 * @details Читается и изменяется измеряемым процессом напрямую: перехватчик и
 *          потребитель живут в одном адресном пространстве, и посредник между
 *          ними только добавил бы искажений в измеряемый горячий путь
 *
 */
typedef struct {
	// Версия двоичного контракта, с которой собрана подставная библиотека
	uint32_t version;
	// Размер структуры состояния в октетах
	uint32_t size;
	/**
	 * Признак активности учёта. Устанавливается потребителем непосредственно
	 * вокруг замера: перехватчик видит вызовы всего процесса целиком, включая
	 * подготовку сценария и вывод результатов
	 */
	volatile int32_t enabled;
	// Количество выключенных путей
	int32_t disabled;
	/**
	 * Признак того, что учётом управляет измеряемый процесс. Устанавливается
	 * потребителем при первом обращении и отменяет вывод итогов при завершении:
	 * потребитель открывает и закрывает окно замера сам, и в счётчиках к концу
	 * работы остаётся содержимое последнего окна, а не всего прогона - выводить
	 * его как итог означало бы выдавать показатели одного сценария за общие
	 */
	volatile int32_t managed;
	// Суммарное количество изменений подписки, переданных ядру
	uint64_t changes;
	// Наибольшее количество изменений подписки, переданных ядру за один вызов
	uint64_t peak;
	// Счётчики по разновидностям вызовов
	awh_syscount_entry_t entries[AWH_SYSCOUNT_MAX];
} awh_syscount_t;

/**
 * @brief Прототип функции получения состояния счётчика системных вызовов
 *
 * @details Отыскивается потребителем по названию из `AWH_SYSCOUNT_ENTRY_POINT`.
 *          Возвращается указатель на состояние, живущее в подставной библиотеке
 *
 * @return указатель на состояние счётчика
 *
 */
typedef awh_syscount_t * (* awh_syscount_state_t) (void);

/**
 * @brief Функция получения названия разновидности вызовов
 *
 * @details Реализована в самом заголовочном файле, чтобы названия не расходились
 *          между подставной библиотекой и потребителем
 *
 * @param kind разновидность вызовов
 * @return     название разновидности либо пустая строка для служебных значений
 *
 */
static inline const char * awh_syscount_name(const awh_syscount_kind_t kind){
	/**
	 * Определяем разновидность учитываемых вызовов
	 */
	switch(kind){
		case AWH_SYSCOUNT_SOCKET:      return "socket";
		case AWH_SYSCOUNT_CLOSE:       return "close";
		case AWH_SYSCOUNT_CONNECT:     return "connect";
		case AWH_SYSCOUNT_ACCEPT:      return "accept";
		case AWH_SYSCOUNT_BIND:        return "bind";
		case AWH_SYSCOUNT_LISTEN:      return "listen";
		case AWH_SYSCOUNT_SHUTDOWN:    return "shutdown";
		case AWH_SYSCOUNT_SETSOCKOPT:  return "setsockopt";
		case AWH_SYSCOUNT_GETSOCKOPT:  return "getsockopt";
		case AWH_SYSCOUNT_GETSOCKNAME: return "getsockname";
		case AWH_SYSCOUNT_GETPEERNAME: return "getpeername";
		case AWH_SYSCOUNT_FCNTL:       return "fcntl";
		case AWH_SYSCOUNT_IOCTL:       return "ioctl";
		case AWH_SYSCOUNT_SYSCTL:      return "sysctl";
		case AWH_SYSCOUNT_GETIFADDRS:  return "getifaddrs";
		case AWH_SYSCOUNT_POLL:        return "poll";
		case AWH_SYSCOUNT_READ:        return "read";
		case AWH_SYSCOUNT_WRITE:       return "write";
		case AWH_SYSCOUNT_RECV:        return "recv";
		case AWH_SYSCOUNT_SEND:        return "send";
		case AWH_SYSCOUNT_RECVFROM:    return "recvfrom";
		case AWH_SYSCOUNT_SENDTO:      return "sendto";
		case AWH_SYSCOUNT_RECVMSG:     return "recvmsg";
		case AWH_SYSCOUNT_SENDMSG:     return "sendmsg";
		case AWH_SYSCOUNT_READV:       return "readv";
		case AWH_SYSCOUNT_WRITEV:      return "writev";
		case AWH_SYSCOUNT_URING:       return "io_uring";
		case AWH_SYSCOUNT_CLOCK:       return "clock_gettime";
		default:                       return "";
	}
}

/**
 * Если заголовочный файл подключается из C++
 */
#ifdef __cplusplus
};
#endif

#endif // __AWH_BENCHMARK_SYSCOUNT__
