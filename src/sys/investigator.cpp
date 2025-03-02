/**
 * @file: investigator.cpp
 * @date: 2024-11-18
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
 * Подключаем заголовочный файл
 */
#include <sys/investigator.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * inquiry Метод проведения дознания
 * @param pid идентификатор процесса
 * @return    название приложения которому принадлежит процесс
 */
string awh::Investigator::inquiry(const pid_t pid) const noexcept {
	// Результат работы функции
	string result = "";
	// Если идентификатор процесса передан
	if(pid > 0){
		/**
		 * Для операционной системы Linux
		 */
		#ifdef __linux__
			// Создаём буфер строки
			char buffer[1024];
			// Заполняем нулями буфер данных
			::memset(buffer, 0, sizeof(buffer));
			// Заполняем адрес процесса
			::sprintf(buffer, "/proc/%d/comm", pid);
			// Структура проверка статистики
			struct stat info;
			// Выполняем извлечение данных статистики
			const int32_t status = ::stat(buffer, &info);
			// Если файл существует
			if((status == 0) && S_ISREG(info.st_mode)){
				// Выполняем открытие файла
				FILE * file = ::fopen(buffer, "r");
				// Если файл открыт удачно
				if(file != nullptr){
					// Размер данных полученных из файла
					const size_t size = ::fread(buffer, sizeof(char), 1024, file);
					// Если данные из файла прочитан удачно
					if(size > 0){
						// Если последний символ это перенос строки
						if(buffer[size - 1] == '\n')
							// Формируем завершающий ноль
							buffer[size - 1] = '\0';
						// Выполняем формирование результата
						result.assign(buffer, buffer + size);
					}
					// Выполняем закрытие файла
					::fclose(file);
				}
			}
		/**
		 * Для операционной системы FreeBSD
		 */
		#elif __FreeBSD__
			// Выполняем получение данные процесса
			struct kinfo_proc * proc = ::kinfo_getproc(pid);
			// Если данные процесса получены
			if(proc != nullptr){
				// Выполняем получение названия процесса
				result = proc->ki_comm;
				// Очищаем ранее созданный объект с данными процесса
				::free(proc);
			}
		/**
		 * Для операционной системы MacOS X
		 */
		#elif __APPLE__ || __MACH__
			// Создаём буфер строки
			char buffer[512];
			// Заполняем нулями буфер данных
			::memset(buffer, 0, sizeof(buffer));
			// Получаем название приложения
			ssize_t size = ::proc_name(pid, buffer, sizeof(buffer));
			// Если название приложения получено
			if(size > 0)
				// Выполняем формирование результата
				result.assign(buffer, buffer + size);
		/**
		 * Для операционной системы Windows
		 */
		#elif _WIN32 || _WIN64
			// Выполняем получение данных процесса
			HANDLE hpc = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
			// Если процесс открыт удачно
			if(hpc != nullptr){
				// Создаём буфер строки
				char buffer[MAX_PATH];
				// Заполняем нулями буфер данных
				::memset(buffer, 0, sizeof(buffer));
				// Извлекаем данные процесса
				::GetProcessImageFileNameA(hpc, buffer, MAX_PATH);
				// Выполняем получение результата
				result = buffer;
				// Выполняем закрытие процесса
				::CloseHandle(hpc);
				// Выполняем поиск каталога
				const size_t pos = result.rfind('\\');
				// Если каталог найден
				if(pos != string::npos)
					// Выполняем удаление лишних символов
					result.erase(result.begin(), result.begin() + (pos + 1));
			}
		#endif
	}
	// Выводим результат
	return result;
}
