/**
 * @file syslog.cpp
 * @date 2026-09-07
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
 * @brief Реализация общей части сценариев замеров контейнера SysLog — мер прогона, поверок
 *        пригодности эталона и самих эталонных записей
 *
 * @copyright Copyright © 2026
 *
 */
#include "syslog.hpp"

/**
 * Подключаем заголовочные файлы бенчмарков
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>

/**
 * @brief Функция формирования сведений о прогоне сценария
 *
 * @param output итоги прогона сценария
 * @return       сведения о прогоне для вывода
 *
 */
std::string awh::benchmark::journal::details(const outcome_t & output) noexcept {
	// Буфер собираемых сведений о прогоне
	char buffer[256];
	// Выполняем сборку сведений о прогоне сценария
	::snprintf(
		buffer, sizeof(buffer), "записей: %zu, октетов: %zu, времени: %.3f с, выделений: %zu",
		output.operations, output.bytes, output.seconds, output.allocations
	);
	// Выводим собранные сведения о прогоне сценария
	return std::string(buffer);
}

/**
 * @brief Функция извлечения пропускной способности разбора
 *
 * @param output итоги прогона сценария
 * @return       пропускная способность в мегабайтах в секунду
 *
 */
double awh::benchmark::journal::perSecond(const outcome_t & output) noexcept {
	// Если время прогона не измерено
	if(output.seconds <= 0.0)
		// Выводим отсутствие пропускной способности
		return 0.0;
	// Выводим пропускную способность разбора в мегабайтах в секунду
	return ((static_cast <double> (output.bytes) / 1048576.0) / output.seconds);
}

/**
 * @brief Функция извлечения количества разобранных записей в секунду
 *
 * @param output итоги прогона сценария
 * @return       количество разобранных записей в секунду
 *
 */
double awh::benchmark::journal::perEvents(const outcome_t & output) noexcept {
	// Если время прогона не измерено
	if(output.seconds <= 0.0)
		// Выводим отсутствие разобранных записей
		return 0.0;
	// Выводим количество разобранных записей в секунду
	return (static_cast <double> (output.operations) / output.seconds);
}

/**
 * @brief Функция извлечения количества выделений памяти на одну запись
 *
 * @param output итоги прогона сценария
 * @return       количество выделений памяти на одну запись
 *
 */
double awh::benchmark::journal::perRecord(const outcome_t & output) noexcept {
	// Если операций прогоном не выполнено
	if(output.operations == 0)
		// Выводим отсутствие выделений памяти
		return 0.0;
	// Выводим количество выделений памяти на одну запись
	return (static_cast <double> (output.allocations) / static_cast <double> (output.operations));
}

/**
 * @brief Функция проверки работоспособности учёта выделений памяти
 *
 * @param output итоги прогона сценария
 * @param result заполняемый результат измерения
 * @return       признак работоспособности учёта
 *
 */
bool awh::benchmark::journal::counted(const outcome_t & output, awh::benchmark::result_t & result) noexcept {
	// Если операций прогоном не выполнено
	if(output.operations == 0){
		// Помечаем измерение недействительным
		result.invalid = true;
		// Устанавливаем причину недействительности измерения
		result.reason = "сценарий не выполнил ни одной операции";
		// Выводим неработоспособность учёта выделений памяти
		return false;
	}
	/**
	 * Если учёт выделений памяти молчит
	 *
	 * @note Показатель «не более» при молчащем счётчике отчитывается «уложился»
	 *       всегда: нулевой расход меньше всякого порога. У MinGW оператор из
	 *       libstdc++-6.dll замены не видит вовсе, и без этой поверки сценарий там
	 *       отчитывался бы успехом при нулевом учёте
	 */
	if(output.allocations == 0){
		// Помечаем измерение недействительным
		result.invalid = true;
		// Устанавливаем причину недействительности измерения
		result.reason = "учёт выделений памяти не работает: замена оператора не видна";
		// Выводим неработоспособность учёта выделений памяти
		return false;
	}
	// Выводим работоспособность учёта выделений памяти
	return true;
}

/**
 * @brief Функция поверки того, что измеряемая работа кругами состоялась
 *
 * @param output итоги прогона сценария
 * @param result заполняемый результат измерения
 * @return       признак того, что работа кругами состоялась
 *
 */
bool awh::benchmark::journal::worked(const outcome_t & output, awh::benchmark::result_t & result) noexcept {
	// Если итог работы числа кругов не достиг
	if(output.produced < output.operations){
		// Помечаем измерение недействительным
		result.invalid = true;
		// Устанавливаем причину недействительности измерения
		result.reason = "измеряемая работа отказом хотя бы одного круга завершилась";
		// Выводим отсутствие состоявшейся работы
		return false;
	}
	// Выводим признак состоявшейся работы
	return true;
}

/**
 * @brief Функция извлечения задержки обработки одной записи
 *
 * @param output итоги прогона сценария
 * @return       задержка обработки одной записи в микросекундах
 *
 */
double awh::benchmark::journal::perLatency(const outcome_t & output) noexcept {
	// Если операций прогоном не выполнено
	if(output.operations == 0)
		// Выводим отсутствие задержки обработки
		return 0.0;
	// Выводим задержку обработки одной записи в микросекундах
	return ((output.seconds * 1000000.0) / static_cast <double> (output.operations));
}

/**
 * @brief Функция получения контрольной суммы прогонов
 *
 * @return ссылка на контрольную сумму прогонов
 *
 */
volatile uint64_t & awh::benchmark::journal::checksum() noexcept {
	// Контрольная сумма прогонов
	static volatile uint64_t result = 0;
	// Выводим контрольную сумму прогонов
	return result;
}

/**
 * @brief Функция получения эталонной записи устаревшего описания
 *
 * @return эталонная запись устаревшего описания
 *
 */
const std::string & awh::benchmark::journal::legacy() noexcept {
	// Эталонная запись устаревшего описания
	static const std::string result =
		"<45>Oct 22 12:34:56 freebsd-log.dmz.example.org syslog-ng[8763]: "
		"[notice]syslog-ng starting up; version='4.7.1', cfg-fingerprint='a3f21c', "
		"module-path='/usr/local/lib/syslog-ng', persist-file='/var/db/syslog-ng.persist'";
	// Выводим эталонную запись устаревшего описания
	return result;
}

/**
 * @brief Функция получения эталонной записи нынешнего описания
 *
 * @return эталонная запись нынешнего описания
 *
 */
const std::string & awh::benchmark::journal::modern() noexcept {
	// Эталонная запись нынешнего описания
	static const std::string result =
		"<165>1 2023-04-11T23:29:33.003Z mymachine.example.com evntslog 1093 ID47 "
		"[exampleSDID@32473 iut=\"3\" eventSource=\"Application\" eventID=\"1011\"] "
		"An application event log entry with a reasonably long free-form message body";
	// Выводим эталонную запись нынешнего описания
	return result;
}

/**
 * @brief Функция получения эталонной записи со многими блоками данных
 *
 * @return эталонная запись со многими блоками данных
 *
 */
const std::string & awh::benchmark::journal::structured() noexcept {
	// Эталонная запись со многими блоками структурированных данных
	static const std::string result =
		"<10>1 2023-12-25T15:29:22.000003-07:00 srv-demo-2022.demo.pgr auditd 1093 GNRL_EV "
		"[origin@23668 software=\"auditd\" swVersion=\"3.1.2\" enterpriseId=\"23668\"]"
		"[meta@23668 sequenceId=\"98211\" sysUpTime=\"884512\" language=\"en-US\"]"
		"[event@23668 type=\"SYSCALL\" result=\"success\" exit=\"0\" syscall=\"openat\"]"
		"[subject@23668 uid=\"1000\" auid=\"1000\" ses=\"7\" comm=\"sshd\"]"
		"[object@23668 path=\"/var/log/auth.log\" mode=\"0640\" dev=\"08:01\"]"
		"[network@23668 saddr=\"10.0.0.17\" sport=\"48122\" daddr=\"10.0.0.1\" dport=\"22\"]"
		"[escaped@23668 quoted=\"a\\\"b\" bracket=\"c\\]d\" slash=\"e\\\\f\"]"
		"[policy@23668 rule=\"watch-auth\" action=\"always\" key=\"identity\"] "
		"Audit record for a successful file access by a privileged process";
	// Выводим эталонную запись со многими блоками структурированных данных
	return result;
}

/**
 * @brief Функция получения эталонной записи наименьшей длины
 *
 * @return эталонная запись наименьшей длины
 *
 */
const std::string & awh::benchmark::journal::minimal() noexcept {
	// Эталонная запись наименьшей длины
	static const std::string result = "<13>Oct 22 12:34:56 host app: ok";
	// Выводим эталонную запись наименьшей длины
	return result;
}

/**
 * @brief Функция получения эталонного потока записей
 *
 * @return эталонный поток записей
 *
 */
const std::string & awh::benchmark::journal::stream() noexcept {
	// Эталонный поток записей
	static const std::string result = []() noexcept -> std::string {
		// Собираемый поток записей
		std::string output;
		// Выделяем память под собираемый поток записей
		output.reserve(0x8000);
		/**
		 * Выполняем сборку потока записей обоих описаний вперемешку
		 *
		 * @note Описания чередуются намеренно: сборщик журналов принимает поток от
		 *       устройств разных поколений разом, и самоопределение описания работает
		 *       у него на всякой записи, а не единожды на весь поток
		 */
		for(size_t i = 0; i < 32; i++){
			// Добавляем запись устаревшего описания в поток
			output.append(awh::benchmark::journal::legacy());
			// Отделяем запись от следующей переводом строки
			output.append(1, '\n');
			// Добавляем запись нынешнего описания в поток
			output.append(awh::benchmark::journal::modern());
			// Отделяем запись от следующей переводом строки
			output.append(1, '\n');
			// Добавляем запись наименьшей длины в поток
			output.append(awh::benchmark::journal::minimal());
			// Отделяем запись от следующей переводом строки
			output.append(1, '\n');
			// Добавляем запись без приставки приоритета в поток
			output.append("Oct 22 10:52:01 scapegoat.dmz.example.org sched[222]: That's All Folks!");
			// Отделяем запись от следующей переводом строки
			output.append(1, '\n');
		}
		// Выводим собранный поток записей
		return output;
	}();
	// Выводим эталонный поток записей
	return result;
}

/**
 * @brief Функция получения количества записей эталонного потока
 *
 * @return количество записей эталонного потока
 *
 */
size_t awh::benchmark::journal::records() noexcept {
	// Количество записей эталонного потока
	static const size_t result = []() noexcept -> size_t {
		// Получаем эталонный поток записей
		const std::string & text = awh::benchmark::journal::stream();
		// Количество записей эталонного потока
		size_t output = 0;
		/**
		 * Выполняем перебор всех знаков эталонного потока
		 *
		 * @note Записи считаются по знакам конца строки самого потока, а не постоянной
		 *       у сценария: постоянная разошлась бы с потоком при первой же правке
		 *       эталона, и показатель числа записей в секунду соврал бы молча
		 */
		for(size_t i = 0; i < text.size(); i++){
			// Если знак потока концом строки является
			if(text[i] == '\n')
				// Наращиваем количество записей эталонного потока
				output++;
		}
		// Выводим количество записей эталонного потока
		return output;
	}();
	// Выводим количество записей эталонного потока
	return result;
}
