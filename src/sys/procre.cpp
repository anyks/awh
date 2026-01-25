/**
 * @file: procre.cpp
 * @date: 2026-01-26
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

/**
 * Стандартные модули
 */
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sys/types.h>

/**
 * Для операционной системы Linux
 */
#ifdef __linux__
	#include <sys/file.h>
	#include <sys/stat.h>
/**
 * Для операционной системы FreeBSD
 */
#elif __FreeBSD__
	#include <libutil.h>
	#include <sys/user.h>
/**
 * Для операционной системы MacOS X
 */
#elif __APPLE__ || __MACH__
	#include <libproc.h>
/**
 * Для операционной системы NetBSD или OpenBSD
 */
#elif __NetBSD__ || __OpenBSD__
	#include <fstream>
	#include <sstream>
/**
 * Реализация под Sun Solaris
 */
#elif __sun__
	#include <fstream>
	#include <sstream>
	#include <procfs.h>
/**
 * Для операционной системы Windows
 */
#elif _WIN32 || _WIN64
	#include <windows.h>
	#include <psapi.h>
	#include <cstdint>
#endif

/**
 * Подключаем заголовочный файл
 */
#include <sys/procre.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Для операционной системы NetBSD или OpenBSD
 */
#if __NetBSD__ || __OpenBSD__
	/**
	 * Инкапсулируем функции Process Resolver в пространство имён
	 */
	namespace procre {
		/**
		 * @brief Функция извлечения данных записи
		 *
		 * @param filename адрес файла для извлечения
		 * @param log      объект для работы с логами
		 * @return         содержимое файла
		 */
		static string read(const string & filename, const awh::log_t * log) noexcept {
			// Результат работы функции
			string result = "";
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Устанавливаем адрес файла для чтения
				ifstream file(filename.c_str());
				// Если файл открыт удачно
				if(file.is_open()){
					// Переходим в конец файла
					file.seekg(0, ios::end);
					// Определяем размер файла
					const streampos pos = file.tellg();
					// Если размер файла получен
					if(pos != static_cast <streampos> (-1))
						// Выделяем память для буфера данных
						result.reserve(static_cast <size_t> (pos));
					// Переходим в начало файла
					file.seekg(0, ios::beg);
					// Выполняем заполнение данными буфер памяти
					result.assign(istreambuf_iterator <char> (file), istreambuf_iterator <char> ());
					// Закрываем файл
					file.close();
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Выполняем сброс работы функции
				result = "";
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(filename), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			}
			// Выводим результат
			return result;
		}
	};
#endif

/**
 * @brief Метод получения названия приложения по идентификатору процесса
 *
 * @param pid идентификатор процесса
 * @return    название приложения которому принадлежит процесс
 */
string awh::ProcessResolver::name(const pid_t pid) const noexcept {
	// Результат работы функции
	string result = "";
	// Если идентификатор процесса передан
	if(pid > 0){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * Для операционной системы Linux
			 */
			#if __linux__
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
				// Если файл существует и это обычный файл
				if((status == 0) && S_ISREG(info.st_mode)){
					// Выполняем открытие файла
					FILE * file = ::fopen(buffer, "r");
					// Если файл открыт удачно
					if(file != nullptr){
						// Чтение данных из файла.
						// Внимание: sizeof(buffer) == 1024, читаем до sizeof(buffer) - 1, чтобы гарантировать нуль-терминатор
						const size_t size = ::fread(buffer, sizeof(char), sizeof(buffer) - 1, file);
						// Если данные из файла прочитаны удачно
						if(size > 0){
							// Если последний символ это перенос строки
							if(buffer[size - 1] == '\n')
								// Выполняем формирование результата (до переноса)
								result.assign(buffer, buffer + (size - 1));
							// Выполняем формирование результата
							else result.assign(buffer, buffer + size);
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
			 * Для операционной системы NetBSD или OpenBSD
			 */
			#elif __NetBSD__ || __OpenBSD__
				// Строковый поток названия файла
				stringstream ss;
				// Формируем название файла
				ss << "/proc/" << pid << "/comm";
				// Выполняем извлечение данных файла
				result = ::procre::read(ss.str(), this->_log);
				// Если файл прочитан удачно
				if(!result.empty()){
					// Если последний символ является переносом строки
					if(result.rbegin()[0] == '\n')
						// Выполняем удаление последнего символа
						result.pop_back();
				// Если не может быть прочитан
				} else {
					// Выполняем очистку потока
					ss.str("");
					// Формируем название файла
					ss << "/proc/" << pid << "/exe";
					// Создаём буфер строки
					char buffer[1024];
					// Заполняем нулями буфер данных
					::memset(buffer, 0, sizeof(buffer));
					// Выполняем извлечение данных в буфер
					const int32_t size = static_cast <int32_t> (::readlink(ss.str().c_str(), buffer, sizeof(buffer)));
					// Выполняем чтение данных в бинарный буфер
					if(size > 0) {
						// Устанавливаем последний символ
						buffer[size] = '\0';
						// Если мы нашли разделитель
						if(const char * p = ::strrchr(buffer, '/'))
							// Выполняем удаление разделителя
							result = (p + 1);
						// Выполняем получение названия приложения
						else result = buffer;
						// Если последний символ является переносом строки
						if(result.rbegin()[0] == '\n')
							// Выполняем удаление последнего символа
							result.pop_back();
					}
				}
			/**
			 * Реализация под Sun Solaris
			 */
			#elif __sun__
				// Строковый поток названия файла
				stringstream ss;
				// Формируем название файла
				ss << "/proc/" << pid << "/psinfo";
				// Выполняем чтение файла
				ifstream file(ss.str());
				// Если файл прочитан удачно
				if(file.is_open()){
					// Создаём объект информационных данных процесса
					psinfo_t info;
					// Выполняем чтение структуры данных процесса
					file.read(reinterpret_cast <char *> (&info), sizeof(info));
					// Закрываем файл
					file.close();
					// Выполняем извлечение названия процесса
					result = info.pr_fname;
				}
			/**
			 * Для операционной системы MS Windows
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
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(pid), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
