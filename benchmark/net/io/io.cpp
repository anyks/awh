/**
 * @file: io.cpp
 * @date: 2026-07-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Общее окружение бенчмарков сетевого движка — объекты фреймворка и логирования,
 *        выделение свободного порта петлевого интерфейса и формирование сведений о замере
 *
 * @copyright: Copyright © 2026
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

/**
 * Подключаем системные заголовочные файлы
 */
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <netinet/in.h>

/**
 * Если сборка производится под операционную систему macOS
 */
#if __APPLE__
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
	// Объект сведений о потреблении ресурсов процессом
	struct rusage usage{};
	// Если сведения о потреблении ресурсов не получены
	if(::getrusage(RUSAGE_SELF, &usage) != 0)
		// Выводим нулевой объём занятой памяти
		return 0;
	/**
	 * Если сборка производится под операционную систему macOS
	 */
	#if __APPLE__
		// Выводим пиковый объём занятой памяти как есть: macOS сообщает его в октетах
		return static_cast <size_t> (usage.ru_maxrss);
	/**
	 * Если сборка производится под все остальные операционные системы
	 */
	#else
		// Выводим пиковый объём занятой памяти: остальные системы сообщают его в кибибайтах
		return (static_cast <size_t> (usage.ru_maxrss) * 1024);
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
	 * Если сборка производится под операционную систему macOS
	 */
	#if __APPLE__
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
		#ifdef TASK_VM_INFO_REV1_COUNT
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
	// Выполняем создание временного сокета
	const int32_t fd = ::socket(AF_INET, SOCK_STREAM, 0);
	// Если временный сокет не создан
	if(fd < 0)
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
	// Выполняем закрытие временного сокета
	::close(fd);
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
/**
 * @brief Функция получения объекта фреймворка сценариев
 *
 * @return объект фреймворка
 *
 */
const awh::fmk_t * awh::benchmark::io::framework() noexcept {
	// Объект фреймворка сценариев
	static awh::fmk_t result;
	// Выводим объект фреймворка
	return &result;
}
/**
 * @brief Функция получения объекта логирования сценариев
 *
 * @return объект логирования
 *
 */
const awh::log_t * awh::benchmark::io::logger() noexcept {
	// Объект логирования сценариев
	static awh::log_t result(framework());
	// Отключаем логирование на время прогона сценариев
	const_cast <awh::log_t &> (result).level(awh::log_t::level_t::NONE);
	// Выводим объект логирования
	return &result;
}
