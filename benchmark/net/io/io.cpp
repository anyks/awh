/**
 * @file io.cpp
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
 * @brief Общее окружение бенчмарков сетевого движка — объекты фреймворка и логирования,
 *        выделение свободного порта петлевого интерфейса и формирование сведений о замере
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>

/**
 * Подключаем заголовочный файл бенчмарков сетевого движка
 */
#include "io.hpp"
#include <sys/log.hpp>

/**
 * Подключаем системные заголовочные файлы
 *
 * @note У MS Windows заголовков этих нет вовсе, а средства сокетов приходят единой
 *       точкой входа: порядок включения у них свой, и нарушение его даёт отказы о
 *       переопределении, далёкие от места причины
 */
#if defined(_WIN32) || defined(_WIN64)
	#include <sys/macro/win32.hpp>
	// Сведения о памяти процесса берутся отдельной библиотекой системы
	#include <psapi.h>
#else
	#include <unistd.h>
	#include <sys/time.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <sys/resource.h>
	#include <netinet/in.h>
	/**
	 * Диапазон эфемерных портов у систем BSD и Solaris добывается через sysctl
	 * по имени, а у Linux читается из /proc - там этого заголовка не нужно
	 */
	#if !defined(__linux__)
		#include <sys/sysctl.h>
	#endif
#endif

/**
 * Если сборка производится под операционную систему macOS
 */
#if defined(__APPLE__)
	#include <mach/mach.h>
	#include <mach/task_info.h>
#endif

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Функция формирования сведений о прогоне сценария
 *
 * @param output итоги прогона сценария
 * @return       сведения о прогоне для вывода
 *
 */
string awh::benchmark::io::details(const outcome_t & output) noexcept {
	// Буфер формирования сведений о прогоне
	char buffer[512];
	// Вычисляем среднее время выполнения одной операции в микросекундах
	const double microseconds = ((output.operations > 0)
	 ? ((output.seconds * 1e6) / static_cast <double> (output.operations)) : 0.0);
	// Выполняем формирование сведений о прогоне
	int32_t offset = ::snprintf(
		buffer, sizeof(buffer),
		"операций: %zu, время: %.3f с, на операцию: %.2f мкс, выделений: %zu (%.1f на операцию), память процесса: %.1f МБ (своей %.1f МБ)",
		output.operations, output.seconds, microseconds, output.allocations,
		perOperation(output), (static_cast <double> (output.footprint) / 1048576.0),
		(static_cast <double> (output.occupancy) / 1048576.0)
	);
	// Если сводка по системным вызовам снята
	if(!output.calls.empty() && (offset < static_cast <int32_t> (sizeof(buffer))))
		// Дополняем сведения о прогоне сводкой по системным вызовам
		offset += ::snprintf(
			buffer + offset, (sizeof(buffer) - static_cast <size_t> (offset)),
			", %s", output.calls.c_str()
		);
	// Если ядру передавались изменения подписки
	if((output.changes > 0) && (offset < static_cast <int32_t> (sizeof(buffer))))
		// Дополняем сведения о прогоне сведениями об изменениях подписки
		::snprintf(
			buffer + offset, (sizeof(buffer) - static_cast <size_t> (offset)),
			", изменений подписки: %.2f на операцию, наибольший пакет: %zu",
			perChange(output), output.batch
		);
	// Выводим сведения о прогоне
	return string(buffer);
}
/**
 * @brief Функция извлечения количества операций в секунду
 *
 * @param output итоги прогона сценария
 * @return       количество операций в секунду
 *
 */
double awh::benchmark::io::perSecond(const outcome_t & output) noexcept {
	// Если время прогона не измерено
	if(output.seconds <= 0.0)
		// Выводим нулевое количество операций в секунду
		return 0.0;
	// Выводим количество операций в секунду
	return (static_cast <double> (output.operations) / output.seconds);
}
/**
 * @brief Функция проверки того, что сценарий выполнил хоть одну операцию
 *
 * @param result результат измерения, в который вносится признак
 * @param output итоги прогона сценария
 *
 */
void awh::benchmark::io::validate(awh::benchmark::result_t & result, const outcome_t & output) noexcept {
	// Если сценарий не выполнил ни одной операции
	if(output.operations == 0){
		// Отмечаем измерение как недействительное
		result.invalid = true;
		// Устанавливаем причину недействительности измерения
		result.reason = "сценарий не выполнил ни одной операции - показателю не по чему считаться";
	}
}
/**
 * @brief Функция обеспечения запаса описателей файлов под прогон
 *
 * @param required потребное количество описателей
 * @param reason   причина невозможности прогона (заполняется при отказе)
 * @return         признак достаточности предела описателей
 *
 */
bool awh::benchmark::io::descriptors(const size_t required, std::string & reason) noexcept {
	/**
	 * Если сборка производится под операционную систему MS Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Предела описателей вида POSIX система не ведёт, прогон не ограничен
		(void) required;
		(void) reason;
		// Выводим признак достаточности предела описателей
		return true;
	/**
	 * Если сборка производится под все остальные операционные системы
	 */
	#else
		// Действующий предел описателей файлов
		struct rlimit limit {};
		/**
		 * Если предел описателей файлов извлечь не удалось, прогону не мешаем
		 *
		 * @note Молчаливый пропуск здесь был бы хуже отказа: предел неизвестен, а
		 *       значит неизвестно и то, что прогон невозможен
		 */
		if(::getrlimit(RLIMIT_NOFILE, &limit) != 0)
			// Выводим признак достаточности предела описателей
			return true;
		// Если мягкого предела не хватает, а жёсткий выше него
		if((static_cast <size_t> (limit.rlim_cur) < required) && (limit.rlim_cur < limit.rlim_max)){
			// Поднимаемый предел описателей файлов
			struct rlimit raised = limit;
			// Поднимаем мягкий предел до потребного, но не выше жёсткого
			raised.rlim_cur = ((limit.rlim_max == RLIM_INFINITY) ? static_cast <rlim_t> (required) :
			 ((static_cast <size_t> (limit.rlim_max) < required) ? limit.rlim_max : static_cast <rlim_t> (required)));
			// Если поднять предел описателей файлов удалось
			if(::setrlimit(RLIMIT_NOFILE, &raised) == 0)
				// Запоминаем поднятый предел описателей файлов
				limit = raised;
		}
		// Если предела описателей файлов достаточно
		if(static_cast <size_t> (limit.rlim_cur) >= required)
			// Выводим признак достаточности предела описателей
			return true;
		// Место под собираемую причину невозможности прогона
		char buffer[192] = {0};
		// Собираем причину невозможности прогона
		::snprintf(
			buffer, sizeof(buffer),
			"предел описателей файлов %zu при потребных %zu (жёсткий предел %zu) - прогон невозможен по окружению",
			static_cast <size_t> (limit.rlim_cur), required, static_cast <size_t> (limit.rlim_max)
		);
		// Устанавливаем причину невозможности прогона
		reason.assign(buffer);
		// Выводим признак недостаточности предела описателей
		return false;
	#endif
}
/**
 * @brief Функция оценки достаточности диапазона эфемерных портов
 *
 * @details Заведена отдельно от добычи границ: добываются они у каждой системы
 *          по-своему, а судятся одинаково, и повторять суждение в каждой ветви
 *          значило бы держать один и тот же расчёт в трёх местах
 *
 * @param first    нижняя граница диапазона
 * @param last     верхняя граница диапазона
 * @param required потребное количество исходящих портов
 * @param reason   причина невозможности прогона (заполняется при отказе)
 * @return         признак достаточности диапазона
 *
 */
static bool enough(const size_t first, const size_t last, const size_t required, std::string & reason) noexcept {
	// Количество портов, отведённых системой под исходящие подключения
	const size_t available = ((last - first) + 1);
	// Если портов диапазона достаточно
	if(available >= required)
		// Выводим признак достаточности диапазона
		return true;
	// Место под собираемую причину невозможности прогона
	char buffer[224] = {0};
	// Собираем причину невозможности прогона
	::snprintf(
		buffer, sizeof(buffer),
		"эфемерных портов %zu (диапазон %zu-%zu) при потребных %zu - прогон невозможен по окружению",
		available, first, last, required
	);
	// Устанавливаем причину невозможности прогона
	reason.assign(buffer);
	// Выводим признак недостаточности диапазона
	return false;
}
/**
 * @brief Функция проверки запаса эфемерных портов под прогон
 *
 * @param required потребное количество исходящих портов
 * @param reason   причина невозможности прогона (заполняется при отказе)
 * @return         признак достаточности диапазона эфемерных портов
 *
 */
bool awh::benchmark::io::ports(const size_t required, std::string & reason) noexcept {
	/**
	 * Если сборка производится под операционную систему MS Windows
	 *
	 * @note Диапазон там правится реестром и настройкой netsh, а не общедоступным
	 *       чтением: сведений о нём прогон не добывает и прогону не мешает
	 */
	#if defined(_WIN32) || defined(_WIN64)
		(void) required;
		(void) reason;
		// Выводим признак достаточности диапазона
		return true;
	/**
	 * Если операционной системой является Linux
	 */
	#elif defined(__linux__)
		// Границы диапазона эфемерных портов
		size_t first = 0, last = 0;
		// Открываем настройку диапазона эфемерных портов
		FILE * file = ::fopen("/proc/sys/net/ipv4/ip_local_port_range", "r");
		// Если настройку прочитать не удалось, прогону не мешаем
		if(file == nullptr)
			// Выводим признак достаточности диапазона
			return true;
		// Выполняем чтение границ диапазона
		const bool readed = (::fscanf(file, "%zu %zu", &first, &last) == 2);
		// Закрываем настройку диапазона эфемерных портов
		::fclose(file);
		// Если границы прочитать не удалось, прогону не мешаем
		if(!readed)
			// Выводим признак достаточности диапазона
			return true;
		// Выполняем проверку достаточности прочитанного диапазона
		return ::enough(first, last, required, reason);
	/**
	 * Если операционной системой является BSD либо Solaris
	 */
	#else
		// Границы диапазона эфемерных портов
		int32_t first = 0, last = 0;
		// Размеры извлекаемых значений
		size_t length = sizeof(first);
		/**
		 * Если нижнюю границу диапазона получить не удалось, прогону не мешаем
		 *
		 * @note Настройки эти есть не у всех систем: у Solaris диапазон правится
		 *       через ndd, и sysctl о нём не знает вовсе
		 */
		if(::sysctlbyname("net.inet.ip.portrange.first", &first, &length, nullptr, 0) != 0)
			// Выводим признак достаточности диапазона
			return true;
		// Восстанавливаем размер извлекаемого значения
		length = sizeof(last);
		// Если верхнюю границу диапазона получить не удалось, прогону не мешаем
		if(::sysctlbyname("net.inet.ip.portrange.last", &last, &length, nullptr, 0) != 0)
			// Выводим признак достаточности диапазона
			return true;
		// Если границы получены бессмысленными, прогону не мешаем
		if((first <= 0) || (last <= 0) || (last < first))
			// Выводим признак достаточности диапазона
			return true;
		// Выполняем проверку достаточности полученного диапазона
		return ::enough(static_cast <size_t> (first), static_cast <size_t> (last), required, reason);
	#endif
}
/**
 * @brief Функция извлечения пропускной способности в мебибайтах в секунду
 *
 * @param output итоги прогона сценария
 * @return       пропускная способность
 *
 */
double awh::benchmark::io::megabytes(const outcome_t & output) noexcept {
	// Если время прогона не измерено
	if(output.seconds <= 0.0)
		// Выводим нулевую пропускную способность
		return 0.0;
	// Выводим пропускную способность в мебибайтах в секунду
	return ((static_cast <double> (output.bytes) / 1048576.0) / output.seconds);
}
/**
 * @brief Функция извлечения количества выделений памяти на одну операцию
 *
 * @param output итоги прогона сценария
 * @return       количество выделений памяти на одну операцию
 *
 */
double awh::benchmark::io::perOperation(const outcome_t & output) noexcept {
	// Если операции не выполнялись
	if(output.operations == 0)
		// Выводим нулевое количество выделений памяти
		return 0.0;
	// Выводим количество выделений памяти на одну операцию
	return (static_cast <double> (output.allocations) / static_cast <double> (output.operations));
}
/**
 * @brief Функция извлечения количества системных вызовов на одну операцию
 *
 * @param output итоги прогона сценария
 * @return       количество системных вызовов на одну операцию
 *
 */
double awh::benchmark::io::perSyscall(const outcome_t & output) noexcept {
	// Если операции не выполнялись
	if(output.operations == 0)
		// Выводим нулевое количество системных вызовов
		return 0.0;
	// Выводим количество системных вызовов на одну операцию
	return (static_cast <double> (output.syscalls) / static_cast <double> (output.operations));
}
/**
 * @brief Функция извлечения количества изменений подписки на одну операцию
 *
 * @param output итоги прогона сценария
 * @return       количество изменений подписки на одну операцию
 *
 */
double awh::benchmark::io::perChange(const outcome_t & output) noexcept {
	// Если операции не выполнялись
	if(output.operations == 0)
		// Выводим нулевое количество изменений подписки
		return 0.0;
	// Выводим количество изменений подписки на одну операцию
	return (static_cast <double> (output.changes) / static_cast <double> (output.operations));
}
/**
 * @brief Функция снятия показателей окружения по итогам замера
 *
 * @param output итоги прогона сценария
 *
 */
void awh::benchmark::io::collect(outcome_t & output) noexcept {
	// Получаем статистику выделений памяти
	awh::benchmark::allocations(output.allocations, output.allocated);
	// Если учёт системных вызовов доступен
	if(awh::benchmark::syscall::available()){
		// Получаем суммарное количество выполненных системных вызовов
		output.syscalls = awh::benchmark::syscall::total();
		// Получаем количество изменений подписки, переданных ядру
		output.changes = awh::benchmark::syscall::changes();
		// Получаем наибольшее количество изменений подписки за один вызов
		output.batch = awh::benchmark::syscall::peak();
		// Формируем сводку по системным вызовам
		output.calls = awh::benchmark::syscall::summary(output.operations);
	}
	// Получаем пиковый объём занятой процессом памяти
	output.footprint = footprint();
	// Получаем пиковый собственный объём памяти процесса
	output.occupancy = occupancy();
}
/**
 * @brief Функция получения пикового объёма занятой процессом памяти
 *
 * @return пиковый объём занятой процессом памяти в октетах
 *
 */
size_t awh::benchmark::io::footprint() noexcept {
	/**
	 * Если операционной системой является MS Windows
	 *
	 * @note Учёта ресурсов в понятиях POSIX у этой системы нет вовсе, а пиковый объём
	 *       она сообщает своим приёмом сведений о памяти процесса
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Объект сведений о памяти процесса
		PROCESS_MEMORY_COUNTERS counters{};
		// Если сведения о памяти процесса не получены
		if(!::GetProcessMemoryInfo(::GetCurrentProcess(), &counters, sizeof(counters)))
			// Выводим нулевой объём занятой памяти
			return 0;
		// Выводим пиковый объём рабочего набора процесса
		return static_cast <size_t> (counters.PeakWorkingSetSize);
	#else
	// Объект сведений о потреблении ресурсов процессом
	struct rusage usage{};
	// Если сведения о потреблении ресурсов не получены
	if(::getrusage(RUSAGE_SELF, &usage) != 0)
		// Выводим нулевой объём занятой памяти
		return 0;
	/**
	 * Если сборка производится под операционную систему macOS
	 */
	#if defined(__APPLE__)
		// Выводим пиковый объём занятой памяти как есть: macOS сообщает его в октетах
		return static_cast <size_t> (usage.ru_maxrss);
	/**
	 * Если сборка производится под все остальные операционные системы
	 */
	#else
		// Выводим пиковый объём занятой памяти: остальные системы сообщают его в кибибайтах
		return (static_cast <size_t> (usage.ru_maxrss) * 1024);
	#endif
	#endif
}
/**
 * @brief Функция получения пикового собственного объёма памяти процесса
 *
 * @return пиковый собственный объём памяти процесса в октетах
 *
 */
size_t awh::benchmark::io::occupancy() noexcept {
	/**
	 * Если операционной системой является MS Windows
	 *
	 * @note Собственным объёмом здесь считается закрытый объём процесса - то, что не
	 *       делится с прочими. Он и есть ближайшее соответствие тому, что прочие системы
	 *       сообщают отдельной величиной
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Объект расширенных сведений о памяти процесса
		PROCESS_MEMORY_COUNTERS_EX counters{};
		// Если сведения о памяти процесса не получены
		if(!::GetProcessMemoryInfo(::GetCurrentProcess(), reinterpret_cast <PROCESS_MEMORY_COUNTERS *> (&counters), sizeof(counters)))
			// Выводим нулевой объём занятой памяти
			return 0;
		// Выводим закрытый объём памяти процесса
		return static_cast <size_t> (counters.PrivateUsage);
	/**
	 * Если сборка производится под операционную систему macOS
	 */
	#elif defined(__APPLE__)
		// Объект сведений о виртуальной памяти задачи
		task_vm_info_data_t info{};
		// Размер объекта сведений в машинных словах
		mach_msg_type_number_t count = TASK_VM_INFO_COUNT;
		// Если сведения о виртуальной памяти задачи не получены
		if(::task_info(::mach_task_self(), TASK_VM_INFO, reinterpret_cast <task_info_t> (&info), &count) != KERN_SUCCESS)
			// Выводим нулевой объём занятой памяти
			return 0;
		/**
		 * Если ядро сообщило пиковое значение собственного объёма
		 */
		#if defined(TASK_VM_INFO_REV1_COUNT)
			// Если сведения содержат пиковое значение собственного объёма
			if(count >= TASK_VM_INFO_REV1_COUNT)
				// Выводим пиковый собственный объём памяти процесса
				return static_cast <size_t> (info.ledger_phys_footprint_peak);
		#endif
		// Выводим текущий собственный объём памяти процесса
		return static_cast <size_t> (info.phys_footprint);
	/**
	 * Если сборка производится под все остальные операционные системы
	 */
	#else
		// Объект чтения сведений о состоянии процесса
		FILE * file = ::fopen("/proc/self/status", "r");
		// Если сведения о состоянии процесса недоступны
		if(file == nullptr)
			// Выводим нулевой объём занятой памяти
			return 0;
		// Буфер чтения строки сведений
		char buffer[256];
		// Пиковый собственный объём памяти процесса
		size_t result = 0;
		/**
		 * Читаем сведения о состоянии процесса построчно
		 */
		while(::fgets(buffer, sizeof(buffer), file) != nullptr){
			// Значение пикового объёма занятой памяти в кибибайтах
			size_t value = 0;
			// Если строка содержит пиковый объём занятой памяти
			if(::sscanf(buffer, "VmHWM: %zu kB", &value) == 1){
				// Запоминаем пиковый собственный объём памяти процесса
				result = (value * 1024);
				// Прекращаем чтение сведений
				break;
			}
		}
		// Выполняем закрытие сведений о состоянии процесса
		::fclose(file);
		// Выводим пиковый собственный объём памяти процесса
		return result;
	#endif
}
/**
 * @brief Функция получения свободного порта петлевого интерфейса
 *
 * @return номер свободного порта
 *
 */
uint16_t awh::benchmark::io::port() noexcept {
	/**
	 * Если операционной системой является MS Windows
	 *
	 * @note Средства сокетов у этой системы требуют подъёма прежде первого же обращения,
	 *       и без него создание сокета отвечает отказом. Порт тогда взялся бы запасной -
	 *       один и тот же у всех сценариев разом, - и сценарии столкнулись бы на нём,
	 *       меряя не движок, а занятость порта. Подъём ведётся единожды за работу
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Признак поднятых средств сокетов
		static const bool winsock = []() noexcept -> bool {
			// Сведения о поднятых средствах сокетов
			WSADATA data;
			// Выводим признак успешного подъёма средств сокетов
			return (::WSAStartup(MAKEWORD(2, 2), &data) == 0);
		}();
		// Отмечаем признак поднятых средств использованным
		static_cast <void> (winsock);
	#endif
	// Выполняем создание временного сокета
	const awh::net::socket_t fd = ::socket(AF_INET, SOCK_STREAM, 0);
	// Если временный сокет не создан
	if(fd == awh::net::invalid_socket_t)
		// Выводим порт из динамического диапазона
		return 45000;
	// Параметры привязки временного сокета
	struct sockaddr_in addr{};
	// Устанавливаем семейство адреса
	addr.sin_family = AF_INET;
	// Устанавливаем адрес петлевого интерфейса
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	// Запрашиваем у системы любой свободный порт
	addr.sin_port = 0;
	// Номер выделенного порта
	uint16_t result = 45000;
	// Если привязка временного сокета выполнена
	if(::bind(fd, reinterpret_cast <struct sockaddr *> (&addr), sizeof(addr)) == 0){
		// Размер структуры параметров сокета
		socklen_t length = sizeof(addr);
		// Если параметры привязки сокета получены
		if(::getsockname(fd, reinterpret_cast <struct sockaddr *> (&addr), &length) == 0)
			// Извлекаем номер выделенного системой порта
			result = ntohs(addr.sin_port);
	}
	/**
	 * Выполняем закрытие временного сокета
	 *
	 * @note У MS Windows сокет закрывается СВОИМ приёмом: общий приём описателей сокету
	 *       отвечает отказом и оставляет его живым, а занятый им порт - занятым
	 */
	#if defined(_WIN32) || defined(_WIN64)
		::closesocket(fd);
	#else
		::close(fd);
	#endif
	// Выводим номер выделенного порта
	return result;
}
/**
 * @brief Функция получения набора опций события сценариев
 *
 * @return набор опций события
 *
 */
uint16_t awh::benchmark::io::options() noexcept {
	// Выводим набор опций события
	return (
		awh::event::options::NO_SIGILL |
		awh::event::options::NO_SIGPIPE |
		awh::event::options::REUSE_ADDR |
		awh::event::options::NO_IO_BLOCK |
		awh::event::options::CLOSE_ON_EXEC |
		awh::event::options::TCP_NO_DELAY
	);
}
