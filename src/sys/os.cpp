/**
 * @file: os.cpp
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

/**
 * Операционной системой является Linux
 */
#if __linux__
	/**
	 * Стандартные модули
	 */
	#include <queue>
	#include <linux/sysctl.h>
/**
 * Если операционной системой является FreeBSD, NetBSD и OpenBSD
 */
#elif __FreeBSD__ || __NetBSD__ || __OpenBSD__
	/**
	 * Стандартные модули
	 */
	#define _WANT_KINFO_PROC
	#include <sys/param.h>
	#include <sys/user.h>
#endif

/**
 * Для операционной системы не являющейся MS Windows
 */
#if !_WIN32 && !_WIN64
	/**
	 * Стандартные модули
	 */
	#include <pwd.h>
	#include <grp.h>
	#include <sys/resource.h>
#endif

/**
 * Если операционной системой является MacOS X, FreeBSD, NetBSD и OpenBSD
 */
#if __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
	/**
	 * Стандартные модули
	 */
	#include <sys/sysctl.h>
#endif

/**
 * Если операционной системой является MacOS X и Unix
 */
#if __unix__ || __unix || unix || (__APPLE__ && __MACH__)
	/**
	 * Стандартные модули
	 */
	#include <unistd.h>
	#include <sys/resource.h>

	/**
	 * Для операционной системы MacOS X
	 */
	#if __APPLE__ || __MACH__
		#include <mach/mach.h>
	/**
	 * Для операционной системы Sun Solaris
	 */
	#elif (_AIX || __TOS__AIX__) || (__sun__ || __sun || sun && (__SVR4 || __svr4__))
		/**
		 * Стандартные модули
		 */
		#include <fcntl.h>
		#include <procfs.h>
	/**
	 * Подключаем заголовки для Linux
	 */
	#elif __linux__ || __linux || linux || __gnu_linux__
		/**
		 * Стандартные модули
		 */
		#include <stdio.h>
	#endif
#endif

/**
 * Подключаем заголовочный файл
 */
#include <sys/os.hpp>

/**
 * Для операционной системы MS Windows
 */
#if _WIN32 && _WIN64
	/**
	 * Стандартные модули
	 */
	#include <wchar.h>
	#include <psapi.h>
#endif

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Для операционной системы не являющейся MS Windows
 */
#if !_WIN32 && !_WIN64
	/**
	 * @brief Функция получения строкового типа метаданных
	 *
	 * @param buffer буфер бинарных данных
	 * @param result результат работф функции
	 */
	static void metadata(const vector <char> & buffer, string & result) noexcept {
		// Если буфер данных передан
		if(!buffer.empty())
			// Выполняем формирование строки
			result.assign(buffer.begin(), buffer.end());
	}
	/**
	 * @brief Шаблон метода чтения метаданных из бинарного контейнера
	 *
	 * @tparam T Тип данных выводимого результата
	 */
	template <typename T>
	/**
	 * @brief Функция получения бинарного буфера метаданных
	 *
	 * @param buffer буфер бинарных данных
	 * @param result результат работф функции
	 */
	static void metadata(const vector <char> & buffer, vector <T> & result) noexcept {
		// Если буфер данных передан
		if(!buffer.empty()){
			// Выделяем память для результирующего буфера данных
			result.resize(buffer.size() / sizeof(T));
			// Выполняем копирование полученного буфера данных
			::memcpy(reinterpret_cast <char *> (result.data()), reinterpret_cast <const char *> (buffer.data()), buffer.size());
		}
	}
	/**
	 * @brief Шаблон метода чтения метаданных из бинарного контейнера
	 *
	 * @tparam T Тип данных выводимого результата
	 */
	template <typename T>
	/**
	 * @brief Функция получения основных типов метаданных
	 *
	 * @param buffer буфер бинарных данных
	 * @param result результат работф функции
	 */
	static void metadata(const vector <char> & buffer, T & result) noexcept {
		// Если данные являются основными
		if(!buffer.empty())
			// Выполняем копирование полученных данных
			::memcpy(&result, buffer.data(), sizeof(result));
	}
	/**
	 * @brief Метод извлечения настроек ядра операционной системы
	 *
	 * @param name   название записи для получения настроек
	 * @param buffer бинарный буфер с извлечёнными значениями
	 */
	static void sysctl(const string & name, vector <char> & buffer) noexcept {
		// Если название параметра и тип извлекаемого значения переданы
		if(!name.empty()){
			// Выполняем очистку буфера данных
			buffer.clear();
			/**
			 * Если мы работаем в MacOS X, FreeBSD, NetBSD или OpenBSD
			 */
			#if __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
				// Получаем размер буфера
				size_t length = 0;
				// Если размеры удачно получены
				if(::sysctlbyname(name.c_str(), nullptr, &length, nullptr, 0) == 0){
					// Выделяем в буфере нужное количество памяти
					buffer.resize(length, 0);
					// Запрашиваем искомые данные
					if(::sysctlbyname(name.c_str(), buffer.data(), &length, nullptr, 0) < 0)
						// Выполняем очистку буфера данных
						buffer.clear();
				}
			/**
			 * Если это Linux
			 */
			#elif __linux__
				// Объект работы с операционной системой
				awh::os_t os;
				// Создаём комманду запуска
				string command = "sysctl";
				// Добавляем разделитель
				command.append(1, ' ');
				// Добавляем название параметра
				command.append(name);
				// Добавляем разделитель
				command.append(1, ' ');
				// Добавляем усечение строки
				command.append("| cut -d \"=\" -f2 | xargs");
				// Выполняем получение значения команды
				const string & result = os.exec(command, false);
				// Если результат получен
				if(!result.empty()){
					// Очередь собранных данных
					std::queue <std::pair <string, bool>> data;
					// Выполняем перебор всего полученного результата
					for(auto & item : result){
						// Если символ является пробелом
						if(::isspace(item) || (item == '\t') || (item == '\n') || (item == '\r') || (item == '\f') || (item == '\v')){
							// Если очередь уже не пустая
							if(!data.empty()){
								// Если запись является числом
								if(data.back().second){
									// Выполняем создание блока данных
									std::pair <string, bool> record = std::make_pair("", true);
									// Выполняем добавление записи в очередь
									data.push(::move(record));
								// Если запись является строкой, добавляем полученный символ в запись
								} else data.back().first.append(1, ' ');
							}
						// Если символ является числом
						} else if(std::isdigit(item) || ((item == '-') && (data.empty() || data.back().first.empty()))) {
							// Если данных в очереди ещё нет
							if(data.empty()){
								// Выполняем создание блока данных
								std::pair <string, bool> record = std::make_pair(string(1, item), true);
								// Выполняем добавление записи в очередь
								data.push(::move(record));
							// Если данные в очереди уже есть, добавляем полученный символ в запись
							} else data.back().first.append(1, item);
						// Если символ является простым символом
						} else if(item != 0) {
							// Если данных в очереди ещё нет
							if(data.empty()){
								// Выполняем создание блока данных
								std::pair <string, bool> record = std::make_pair(string(1, item), false);
								// Выполняем добавление записи в очередь
								data.push(::move(record));
							// Если данные в очереди уже есть
							} else {
								// Помечаем что запись не является числом
								data.back().second = false;
								// Добавляем полученный символ в запись
								data.back().first.append(1, item);
							}
						}
					}
					/**
					 * Выполняем перебор всей очереди собранных данных
					 */
					while(!data.empty()){
						// Если запись существует
						if(!data.front().first.empty()){
							// Если запись является числом
							if(data.front().second){
								/**
								 * Выполняем отлов ошибок
								 */
								try {
									// Выполняем получение числа
									const uint64_t value1 = ::stoull(data.front().first);
									// Пытаемся уменьшить число
									if(static_cast <uint64_t> (static_cast <uint32_t> (value1)) == value1){
										// Выполняем преобразование в unsigned int 32
										const uint32_t value2 = static_cast <uint32_t> (value1);
										// Выполняем добавление новой записи в буфер
										buffer.insert(buffer.end(), reinterpret_cast <const char *> (&value2), reinterpret_cast <const char *> (&value2) + sizeof(value2));
									// Выполняем добавление новой записи в буфер
									} else buffer.insert(buffer.end(), reinterpret_cast <const char *> (&value1), reinterpret_cast <const char *> (&value1) + sizeof(value1));
								/**
								 * Если возникает ошибка
								 */
								} catch(const exception &) {
									// Выполняем получение числа
									const uint64_t value1 = 0;
									// Пытаемся уменьшить число
									if(static_cast <uint64_t> (static_cast <uint32_t> (value1)) == value1){
										// Выполняем преобразование в unsigned int 32
										const uint32_t value2 = static_cast <uint32_t> (value1);
										// Выполняем добавление новой записи в буфер
										buffer.insert(buffer.end(), reinterpret_cast <const char *> (&value2), reinterpret_cast <const char *> (&value2) + sizeof(value2));
									// Выполняем добавление новой записи в буфер
									} else buffer.insert(buffer.end(), reinterpret_cast <const char *> (&value1), reinterpret_cast <const char *> (&value1) + sizeof(value1));
								}
							// Если запись является текстом
							} else {
								// Если последний символ является пробелом, удаляем его
								if(::isspace(data.front().first.back()))
									// Выполняем удаление последний символ
									data.front().first.pop_back();
								// Выполняем добавление новой записи в буфер
								buffer.insert(buffer.end(), data.front().first.begin(), data.front().first.end());
							}
						}
						// Удаляем используемую запись
						data.pop();
					}
				}
			#endif
		}
	}
	/**
	 * @brief Метод установки настроек ядра операционной системы
	 *
	 * @param name   название записи для установки настроек
	 * @param buffer буфер бинарных данных записи для установки настроек
	 * @param size   размер буфера данных
	 * @return       результат выполнения установки
	 */
	static bool sysctl(const string & name, const void * buffer, const size_t size) noexcept {
		// Если название параметра передано
		if(!name.empty() && (buffer != nullptr) && (size > 0)){
			/**
			 * Если мы работаем в MacOS X, FreeBSD, NetBSD или OpenBSD
			 */
			#if __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
				// Устанавливаем новые параметры настройки ядра
				return (::sysctlbyname(name.c_str(), nullptr, 0, const_cast <uint8_t *> (reinterpret_cast <const uint8_t *> (buffer)), size) == 0);
			/**
			 * Операционной системой является Linux
			 */
			#elif __linux__
				// Объект работы с операционной системой
				awh::os_t os;
				// Создаём комманду запуска
				string command = "sysctl -w";
				// Добавляем разделитель
				command.append(1, ' ');
				// Добавляем название параметра
				command.append(name);
				// Добавляем разделитель
				command.append(1, '=');
				// Добавляем параметр для установки
				command.append(reinterpret_cast <const char *> (buffer), size);
				// Выполняем установку параметров ядра
				return !os.exec(command, false).empty();
			#endif
		}
		// Сообщаем, что ничего не установленно
		return false;
	}
/**
 * Для операционной системы MS Windows
 */
#else
	/**
	 * @brief Функция конвертация строки из wstring в string
	 *
	 * @param text текст для конвертации
	 * @return     результат проверки
	 */
	static string convert(const wstring & text) noexcept {
		// Результат работы функции
		string result = "";
		// Если текст для конвертации передан
		if(!text.empty()){
			// Получаем размер результирующего буфера данных в кодировке UTF-8
			const int32_t size = ::WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast <int32_t> (text.size()), 0, 0, nullptr, nullptr);
			// Если размер буфера данных получен
			if(size > 0){
				// Выделяем данные для результирующего буфера данных
				result.resize(static_cast <size_t> (size), 0);
				// Если конвертация буфера текстовых данных в UTF-8 не выполнена
				if(!::WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast <int32_t> (text.size()), result.data(), static_cast <int32_t> (result.size()), nullptr, nullptr)){
					// Выполняем удаление результирующего буфера данных
					result.clear();
					// Выполняем удаление выделенной памяти
					string().swap(result);
				}
			}
		}
		// Выводим результат
		return result;
	}
	/**
	 * @brief Функция конвертация строки из string в wstring
	 *
	 * @param text текст для конвертации
	 * @return     результат проверки
	 */
	static wstring convert(const string & text) noexcept {
		// Результат работы функции
		wstring result = L"";
		// Если текст для конвертации передан
		if(!text.empty()){
			// Получаем размер результирующего буфера данных в кодировке UTF-8
			const int32_t size = ::MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast <int32_t> (text.size()), 0, 0);
			// Если размер буфера данных получен
			if(size > 0){
				// Выделяем данные для результирующего буфера данных
				result.resize(static_cast <size_t> (size), 0);
				// Если конвертация буфера текстовых данных в UTF-8 не выполнена
				if(!::MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast <int32_t> (text.size()), result.data(), static_cast <int32_t> (result.size()))){
					// Выполняем удаление результирующего буфера данных
					result.clear();
					// Выполняем удаление выделенной памяти
					wstring().swap(result);
				}
			}
		}
		// Выводим результат
		return result;
	}
#endif
/**
 * @brief isAdmin Метод проверпи запущено ли приложение под суперпользователем
 *
 * @return результат проверки
 */
bool awh::OS::isAdmin() const noexcept {
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#if !_WIN32 && !_WIN64
		// Выводим результат проверки
		return (::geteuid() == 0);
	/**
	 * Для операционной системы MS Windows
	 */
	#else
		// Флаг проверки на администратора
		BOOL isAdmin = FALSE;
		// Группа администраторов
		PSID adminGroup = nullptr;
		// Инициализируем SID для группы администраторов
		SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
		// Если пользователь не принадлежит группе администраторов
		if(!::AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup))
			// Выходим из функции проверки
			return false;
		// Проверяем, входит ли текущий процесс в эту группу
		if(!::CheckTokenMembership(nullptr, adminGroup, &isAdmin))
			// Устанавливаем флаг, что пользователь не является администратором
			isAdmin = FALSE;
		// Очищаем объект группы
		::FreeSid(adminGroup);
		// Выводим результат проверки
		return (isAdmin != FALSE);
	#endif
}
/**
 * @brief Метод определения операционной системы
 *
 * @return название операционной системы
 */
awh::OS::family_t awh::OS::family() const noexcept {
	/**
	 * Операционной системой является Windows 32bit
	 */
	#ifdef _WIN32
		// Заполняем структуру
		return family_t::WIND32;
	/**
	 * Операционной системой является Windows 64bit
	 */
	#elif _WIN64
		// Заполняем структуру
		return family_t::WIND64;
	/**
	 * Операционной системой является MacOS X
	 */
	#elif __APPLE__ || __MACH__
		// Заполняем структуру
		return family_t::MACOSX;
	/**
	 * Операционной системой является Linux
	 */
	#elif __linux__
		// Заполняем структуру
		return family_t::LINUX;
	/**
	 * Операционной системой является FreeBSD
	 */
	#elif __FreeBSD__
		// Заполняем структуру
		return family_t::FREEBSD;
	/**
	 * Операционной системой является NetBSD
	 */
	#elif __NetBSD__
		// Заполняем структуру
		return family_t::NETBSD;
	/**
	 * Операционной системой является OpenBSD
	 */
	#elif __OpenBSD__
		// Заполняем структуру
		return family_t::OPENBSD;
	/**
	 * Реализация под Sun Solaris
	 */
	#elif __sun__
		// Заполняем структуру
		return family_t::SOLARIS;
	/**
	 * Операционной системой является Unix
	 */
	#elif __unix || __unix__
		// Заполняем структуру
		return family_t::UNIX;
	/**
	 * Операционной системой не распознана
	 */
	#else
		// Выводим результат
		return family_t::NONE;
	#endif
}
/**
 * @brief Метод определения текущего расхода памяти
 *
 * @param mode режим потребления памяти
 * @return     размер расхода памяти
 */
size_t awh::OS::rss(const rss_t mode) const noexcept {
	// Результат работы функции
	size_t result = 0;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Определяем режим потребления памяти
		 */
		switch(static_cast <uint8_t> (mode)){
			// Если необходимо получить текущее потребление памяти
			case static_cast <uint8_t> (rss_t::CURRENT): {
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					// Создаём объект информации о памяти
					PROCESS_MEMORY_COUNTERS info;
					// Выполняем извлечение данных текущего процесса
					if(!::GetProcessMemoryInfo(::GetCurrentProcess(), &info, sizeof(info))){
						// Создаём буфер сообщения ошибки
						wchar_t message[256] = {0};
						// Выполняем формирование текста ошибки
						::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 256, 0);
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::convert(message).c_str());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							::fprintf(stderr, "ERROR! %s\n\n", ::convert(message).c_str());
						#endif
					// Выполняем извлечение размера пика потребляемой памяти
					} else result = static_cast <size_t> (info.WorkingSetSize);
				/**
				 * Для операционной системы MacOS X
				 */
				#elif __APPLE__ || __MACH__
					// Создаём объект информации о памяти
					struct mach_task_basic_info info;
					// Устанавливаем количество извлекаемой информации
					mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
					// Выполняем извлечение информации о текущем потреблении памяти
					if(::task_info(::mach_task_self(), MACH_TASK_BASIC_INFO, reinterpret_cast <task_info_t> (&info), &count) != KERN_SUCCESS){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, "Unable to access to determine memory consumption");
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							::fprintf(stderr, "ERROR! %s\n\n", "Unable to access to determine memory consumption");
						#endif
						// Выводим пустой результат
						return result;
					}
					// Выполняем извлечение размера пика потребляемой памяти
					return static_cast <size_t> (info.resident_size);
				/**
				 * Для операционной системы FreeBSD, NetBSD, OpenBSD
				 */
				#elif __FreeBSD__ || __NetBSD__ || __OpenBSD__
					// Создаём объект информации о памяти
					struct kinfo_proc info;
					// Получаем размер объекта информации
					size_t size = sizeof(info);
					// Устанавливаем флаги для необходимых нам процессов
					int32_t mib[] = {CTL_KERN, KERN_PROC, KERN_PROC_PID, static_cast <int32_t> (::getpid())};
					// Выполняем извлечение данных о потреблении памяти
					if(::sysctl(mib, 4, &info, &size, nullptr, 0) != 0){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::strerror(errno));
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							::fprintf(stderr, "ERROR! %s\n\n", ::strerror(errno));
						#endif
						// Выводим пустой результат
						return result;
					}
					// RSS в страницах; переводим в байты
					return (static_cast <size_t> (info.ki_rssize) * ::getpagesize());
				/**
				 * Для операционной системы Linux
				 */
				#elif __linux__ || __linux || linux || __gnu_linux__
					// Размер потребления памяти
					long rss = 0L;
					// Создаём указатель файла
					FILE * file = nullptr;
					// Открываем файловый дескриптор для чтения памяти
					if((file = ::fopen( "/proc/self/statm", "r" )) == nullptr){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::strerror(errno));
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							::fprintf(stderr, "ERROR! %s\n\n", ::strerror(errno));
						#endif
						// Выводим пустой результат
						return result;
					}
					// Выполняем чтение данных потребления памяти
					if(::fscanf(file, "%*s%ld", &rss) != 1){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::strerror(errno));
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							::fprintf(stderr, "ERROR! %s\n\n", ::strerror(errno));
						#endif
						// Выполняем закрытие файлового дескриптора
						::fclose(file);
						// Выводим пустой результат
						return result;
					}
					// Выполняем закрытие файлового дескриптора
					::fclose(file);
					// Выполняем извлечение размера пика потребляемой памяти
					result = (static_cast <size_t> (rss) * static_cast <size_t> (::sysconf(_SC_PAGESIZE)));
				/**
				 * Для операционной системы Sun Solaris
				 */
				#elif (_AIX || __TOS__AIX__) || (__sun__ || __sun || sun && (__SVR4 || __svr4__))
					// Создаём файловый дескриптор для чтения файла
					int32_t sock = -1;
					// Создаём объект информации о памяти
					struct psinfo psinfo;
					// Открываем файловый дескриптор для чтения памяти
					if((sock = ::open("/proc/self/psinfo", O_RDONLY)) == -1){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::strerror(errno));
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							::fprintf(stderr, "ERROR! %s\n\n", ::strerror(errno));
						#endif
						// Выводим пустой результат
						return result;
					}
					// Выполняем чтение данных потребления памяти
					if(::read(sock, &psinfo, sizeof(psinfo) ) != sizeof(psinfo)){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::strerror(errno));
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							::fprintf(stderr, "ERROR! %s\n\n", ::strerror(errno));
						#endif
						// Выполняем закрытие файлового дескриптора
						::close(sock);
						// Выводим пустой результат
						return result;
					}
					// Выполняем закрытие файлового дескриптора
					::close(sock);
					// Выполняем извлечение размера пика потребляемой памяти
					result = static_cast <size_t> (psinfo.pr_rssize * 1024L);
				#endif
			} break;
			// Если необходимо получить максимальное потребление памяти
			case static_cast <uint8_t> (rss_t::MAXIMUM): {
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					// Создаём объект информации о памяти
					PROCESS_MEMORY_COUNTERS info;
					// Выполняем извлечение данных текущего процесса
					if(!::GetProcessMemoryInfo(::GetCurrentProcess(), &info, sizeof(info))){
						// Создаём буфер сообщения ошибки
						wchar_t message[256] = {0};
						// Выполняем формирование текста ошибки
						::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 256, 0);
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::convert(message).c_str());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							::fprintf(stderr, "ERROR! %s\n\n", ::convert(message).c_str());
						#endif
					// Выполняем извлечение размера пика потребляемой памяти
					} else result = static_cast <size_t> (info.PeakWorkingSetSize);
				/**
				 * Для операционной системы Sun Solaris
				 */
				#elif (_AIX || __TOS__AIX__) || (__sun__ || __sun || sun && (__SVR4 || __svr4__))
					// Создаём файловый дескриптор для чтения файла
					int32_t sock = -1;
					// Создаём объект информации о памяти
					struct psinfo psinfo;
					// Открываем файловый дескриптор для чтения памяти
					if((sock = ::open("/proc/self/psinfo", O_RDONLY)) == -1){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::strerror(errno));
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							::fprintf(stderr, "ERROR! %s\n\n", ::strerror(errno));
						#endif
						// Выводим пустой результат
						return result;
					}
					// Выполняем чтение данных потребления памяти
					if(::read(sock, &psinfo, sizeof(psinfo) ) != sizeof(psinfo)){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::strerror(errno));
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							::fprintf(stderr, "ERROR! %s\n\n", ::strerror(errno));
						#endif
						// Выполняем закрытие файлового дескриптора
						::close(sock);
						// Выводим пустой результат
						return result;
					}
					// Выполняем закрытие файлового дескриптора
					::close(sock);
					// Выполняем извлечение размера пика потребляемой памяти
					result = static_cast <size_t> (psinfo.pr_rssize * 1024L);
				/**
				 * Если операционной системой является MacOS X, FreeBSD, NetBSD, OpenBSD и Linux
				 */
				#elif __unix__ || __unix || unix || (__APPLE__ && __MACH__)
					// Создаём объект информации о памяти
					struct rusage rusage;
					// Если получить данные памяти не вышло
					if(::getrusage(RUSAGE_SELF, &rusage) != 0){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::strerror(errno));
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							::fprintf(stderr, "ERROR! %s\n\n", ::strerror(errno));
						#endif
					// Если данные получили удачно
					} else {
						/**
						 * Реализация под MacOS X
						 */
						#if __APPLE__ && __MACH__
							// Выполняем извлечение размера пика потребляемой памяти
							result = static_cast <size_t> (rusage.ru_maxrss);
						/**
						 * Реализация под Linux и FreeBSD
						 */
						#else
							// Выполняем извлечение размера пика потребляемой памяти
							result = static_cast <size_t> (rusage.ru_maxrss * 1024L);
						#endif
					}
				#endif
			} break;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			::fprintf(stderr, "ERROR! %s\n\n", error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * Для операционной системы не являющейся MS Windows
 */
#if !_WIN32 && !_WIN64
	/**
	 * @brief Метод получения идентификатора текущего пользвоателя
	 *
	 * @return идентификатор текущего пользователя
	 */
	uid_t awh::OS::user() const noexcept {
		// Выводим идентификатор текущего пользователя
		return ::geteuid();
	}
	/**
	 * @brief Метод получения группы текущего пользователя
	 *
	 * @return идентификатор группы текущего пользователя
	 */
	gid_t awh::OS::group() const noexcept {
		// Выводим идентификатор группы пользователя
		return ::getegid();
	}
	/**
	 * @brief Метод получения списка групп текущего пользователя
	 *
	 * @return список групп текущего пользователя
	 */
	vector <gid_t> awh::OS::groups() const noexcept {
		// Результат работы функции
		vector <gid_t> result;
		// Буфер данных для извлечения данных
		char buffer[1024];
		// Объект параметров пользователя
		struct passwd pwd;
		// Извлечённые данные пользователя
		struct passwd * data = nullptr;
		// Выполняем извлечение данных пользователя
		if((::getpwuid_r(::geteuid(), &pwd, buffer, sizeof(buffer), &data) == 0) && (data != nullptr)){
			/**
			 * Для операционной системы MacOS X, FreeBSD или Sun Solaris
			 */
			#if __APPLE__ || __MACH__ || __FreeBSD__ || __sun__
				// Добавляем текущую группу пользователя в список
				result.push_back(pwd.pw_gid);
				// Активируем перебор групп
				::setgrent();
				// Активная группа пользователя
				struct group * grp = nullptr;
				/**
				 * Выполняем перебор всех групп пользователя
				 */
				while((grp = ::getgrent()) != nullptr){
					// Получаем текущую группу пользователя
					if(grp->gr_mem){
						// Теперь перебираем все группы пользователей котоыре есть
						for(char ** member = grp->gr_mem; (* member != nullptr); ++member){
							// Если пользователи совпадают
							if(::strcmp(pwd.pw_name, * member) == 0){
								// Формируем список групп пользователя
								result.push_back(grp->gr_gid);
								// Выходим из цикла
								break;
							}
						}
					}
				}
				// Деактивируем перебор групп
				::endgrent();
			/**
			 * Для остальных операционных систем
			 */
			#else
				// Количество групп пользователя
				int32_t count = 0;
				// Первый вызов: нам необходимо определить количетсво групп пользователя
				::getgrouplist(pwd.pw_name, pwd.pw_gid, nullptr, &count);
				// Увеличиваем список групп пользователя
				result.resize(static_cast <size_t> (count));
				// Второй вызов: пробуем получить список групп пользователя
				if(::getgrouplist(pwd.pw_name, pwd.pw_gid, reinterpret_cast <gid_t *> (result.data()), &count) == -1){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::strerror(errno));
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! %s\n\n", ::strerror(errno));
					#endif
					// Очищаем список групп пользователя
					result.clear();
				}
			#endif
		// Если данные пользователя не извлечены
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::strerror(errno));
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! %s\n\n", ::strerror(errno));
			#endif
		}
		// Выводим результат
		return result;
	}
	/**
	 * @brief Метод получения имени пользователя по его идентификатору
	 *
	 * @param uid идентификатор пользователя
	 * @return    имя запрашиваемого пользователя
	 */
	string awh::OS::user(const uid_t uid) const noexcept {
		// Буфер данных для извлечения данных
		char buffer[1024];
		// Объект параметров пользователя
		struct passwd pwd;
		// Извлечённые данные пользователя
		struct passwd * data = nullptr;
		// Выполняем извлечение данных пользователя
		if((::getpwuid_r(uid, &pwd, buffer, sizeof(buffer), &data) == 0) && (data != nullptr))
			// Выводим имя пользователя
			return string(pwd.pw_name);
		// Если данные пользователя не извлечены
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::strerror(errno));
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! %s\n\n", ::strerror(errno));
			#endif
		}
		// Выводим результат
		return "";
	}
	/**
	 * @brief Метод получения группы пользователя по её идентификатору
	 *
	 * @param gid идентификатор группы пользователя
	 * @return    название группы пользователя
	 */
	string awh::OS::group(const gid_t gid) const noexcept {
		// Буфер данных для извлечения данных
		char buffer[1024];
		// Объект параметров группы пользователя
		struct group grp;
		// Извлечённые данные группы пользователя
		struct group * data = nullptr;
		// Выполняем извлечение данных группы пользователя
		if((::getgrgid_r(gid, &grp, buffer, sizeof(buffer), &data) == 0) && (data != nullptr))
			// Выводим имя пользователя
			return string(grp.gr_name);
		// Если данные пользователя не извлечены
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::strerror(errno));
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! %s\n\n", ::strerror(errno));
			#endif
		}
		// Выводим результат
		return "";
	}
	/**
	 * @brief Метод получения идентификатора группы пользователя
	 *
	 * @param name название группы пользователя
	 * @return     идентификатор группы пользователя
	 */
	gid_t awh::OS::group(const string & name) const noexcept {
		// Если название группы пользователя передано
		if(!name.empty()){
			// Буфер данных для извлечения данных
			char buffer[1024];
			// Объект параметров группы пользователя
			struct group grp;
			// Извлечённые данные группы пользователя
			struct group * data = nullptr;
			// Выполняем извлечение данных группы пользователя
			if((::getgrnam_r(name.c_str(), &grp, buffer, sizeof(buffer), &data) == 0) && (data != nullptr))
				// Выводим идентификатор группы пользователя
				return grp.gr_gid;
			// Если данные группы пользователя не извлечены
			else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::strerror(errno));
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! %s\n\n", ::strerror(errno));
				#endif
			}
		}
		// Выводим результат
		return 0;
	}
	/**
	 * @brief Метод вывода идентификатора пользователя
	 *
	 * @param name имя пользователя
	 * @return     полученный идентификатор пользователя
	 */
	uid_t awh::OS::uid(const string & name) const noexcept {
		// Если имя пользователя передано
		if(!name.empty()){
			// Буфер данных для извлечения данных
			char buffer[1024];
			// Объект параметров пользователя
			struct passwd pwd;
			// Извлечённые данные пользователя
			struct passwd * data = nullptr;
			// Выполняем извлечение данных пользователя
			if((::getpwnam_r(name.c_str(), &pwd, buffer, sizeof(buffer), &data) == 0) && (data != nullptr))
				// Выводим идентификатор пользователя
				return pwd.pw_uid;
			// Если данные пользователя не извлечены
			else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::strerror(errno));
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! %s\n\n", ::strerror(errno));
				#endif
			}
		}
		// Выводим результат
		return 0;
	}
	/**
	 * @brief Метод вывода идентификатора группы пользователя
	 *
	 * @param name имя пользователя
	 * @return     полученный идентификатор группы пользователя
	 */
	gid_t awh::OS::gid(const string & name) const noexcept {
		// Если имя пользователя передано
		if(!name.empty()){
			// Буфер данных для извлечения данных
			char buffer[1024];
			// Объект параметров пользователя
			struct passwd pwd;
			// Извлечённые данные пользователя
			struct passwd * data = nullptr;
			// Выполняем извлечение данных пользователя
			if((::getpwnam_r(name.c_str(), &pwd, buffer, sizeof(buffer), &data) == 0) && (data != nullptr))
				// Выводим идентификатор группы пользователя
				return pwd.pw_gid;
			// Если данные пользователя не извлечены
			else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::strerror(errno));
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! %s\n\n", ::strerror(errno));
				#endif
			}
		}
		// Выводим результат
		return 0;
	}
	/**
	 * @brief Получение списка групп пользователя
	 *
	 * @param user имя пользователя чьи группы следует получить
	 * @return     список групп пользователя
	 */
	vector <gid_t> awh::OS::groups(const string & user) const noexcept {
		// Результат работы функции
		vector <gid_t> result;
		// Если имя пользователя передано
		if(!user.empty()){
			// Буфер данных для извлечения данных
			char buffer[1024];
			// Объект параметров пользователя
			struct passwd pwd;
			// Извлечённые данные пользователя
			struct passwd * data = nullptr;
			// Выполняем извлечение данных пользователя
			if((::getpwnam_r(user.c_str(), &pwd, buffer, sizeof(buffer), &data) == 0) && (data != nullptr)){
				/**
				 * Для операционной системы MacOS X, FreeBSD или Sun Solaris
				 */
				#if __APPLE__ || __MACH__ || __FreeBSD__ || __sun__
					// Добавляем текущую группу пользователя в список
					result.push_back(pwd.pw_gid);
					// Активируем перебор групп
					::setgrent();
					// Активная группа пользователя
					struct group * grp = nullptr;
					/**
					 * Выполняем перебор всех групп пользователя
					 */
					while((grp = ::getgrent()) != nullptr){
						// Получаем текущую группу пользователя
						if(grp->gr_mem){
							// Теперь перебираем все группы пользователей котоыре есть
							for(char ** member = grp->gr_mem; (* member != nullptr); ++member){
								// Если пользователи совпадают
								if(user.compare(* member) == 0){
									// Формируем список групп пользователя
									result.push_back(grp->gr_gid);
									// Выходим из цикла
									break;
								}
							}
						}
					}
					// Деактивируем перебор групп
					::endgrent();
				/**
				 * Для остальных операционных систем
				 */
				#else
					// Количество групп пользователя
					int32_t count = 0;
					// Первый вызов: нам необходимо определить количетсво групп пользователя
					::getgrouplist(user.c_str(), pwd.pw_gid, nullptr, &count);
					// Увеличиваем список групп пользователя
					result.resize(static_cast <size_t> (count));
					// Второй вызов: пробуем получить список групп пользователя
					if(::getgrouplist(user.c_str(), pwd.pw_gid, reinterpret_cast <gid_t *> (result.data()), &count) == -1){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::strerror(errno));
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							::fprintf(stderr, "ERROR! %s\n\n", ::strerror(errno));
						#endif
						// Очищаем список групп пользователя
						result.clear();
					}
				#endif
			// Если данные пользователя не извлечены
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::strerror(errno));
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! %s\n\n", ::strerror(errno));
				#endif
			}
		}
		// Выводим результат
		return result;
	}
	/**
	 * @brief Метод запуска приложения от имени указанного пользователя
	 *
	 * @param gid идентификатор группы пользователя
	 * @return    результат выполнения операции
	 */
	bool awh::OS::chown(const uid_t uid) const noexcept {
		// Если идентификатор пользователя успешно установлен
		if(::setuid(uid) == 0)
			// Выводим положительный результат
			return true;
		// Если идентификатор пользователя не установлен
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::strerror(errno));
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! %s\n\n", ::strerror(errno));
			#endif
		}
		// Выводим результат
		return false;
	}
	/**
	 * @brief Метод запуска приложения от имени указанного пользователя
	 *
	 * @param uid идентификатор пользователя
	 * @param gid идентификатор группы пользователя
	 * @return    результат выполнения операции
	 */
	bool awh::OS::chown(const uid_t uid, const gid_t gid) const noexcept {
		// Если идентификатор пользователя успешно установлен
		if(::setuid(uid) == 0){
			// Если идентификатор групы пользователя успешно установлен
			if(::setgid(gid) == 0)
				// Выводим положительный результат
				return true;
			// Если идентификатор группы пользователя не установлен
			else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::strerror(errno));
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! %s\n\n", ::strerror(errno));
				#endif
			}
			// Выводим отрицательный результат
			return false;
		// Если идентификатор пользователя не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::strerror(errno));
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! %s\n\n", ::strerror(errno));
			#endif
		}
		// Выводим результат
		return false;
	}
	/**
	 * @brief Метод запуска приложения от имени указанного пользователя
	 *
	 * @param user  название пользователя
	 * @param group название группы пользователя
	 * @return      результат выполнения операции
	 */
	bool awh::OS::chown(const string & user, const string & group) const noexcept {
		// Если имя пользователя и название группы пользователя переданы
		if(!user.empty() && !group.empty()){
			// Буфер данных для извлечения данных
			char buffer[1024];
			// Объект параметров пользователя
			struct passwd pwd;
			// Извлечённые данные пользователя
			struct passwd * data = nullptr;
			// Выполняем извлечение данных пользователя
			if((::getpwnam_r(user.c_str(), &pwd, buffer, sizeof(buffer), &data) == 0) && (data != nullptr)){
				// Объект параметров группы пользователя
				struct group grp;
				// Извлечённые данные группы пользователя
				struct group * data = nullptr;
				// Выполняем извлечение данных группы пользователя
				if((::getgrnam_r(group.c_str(), &grp, buffer, sizeof(buffer), &data) == 0) && (data != nullptr))
					// Выводим установку права доступа
					return this->chown(pwd.pw_uid, grp.gr_gid);
				// Если данные группы пользователя не извлечены
				else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::strerror(errno));
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! %s\n\n", ::strerror(errno));
					#endif
				}
			// Если данные пользователя не извлечены
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::strerror(errno));
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! %s\n\n", ::strerror(errno));
				#endif
			}
		}
		// Выводим результат
		return false;
	}
/**
 * Для операционной системы MS Windows
 */
#else
	/**
	 * @brief Метод получения идентификатора текущего пользвоателя
	 *
	 * @return идентификатор текущего пользователя
	 */
	wstring awh::OS::user() const noexcept {
		// Результат работы функции
		wstring result = L"";
		// Токен текущего процесса
		HANDLE token = nullptr;
		// Открываем токен текущего процесса
		if(!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &token)){
			// Если мы получили ошибку
			if(::GetLastError() != 0){
				// Создаём буфер сообщения ошибки
				wchar_t message[256] = {0};
				// Выполняем формирование текста ошибки
				::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 256, 0);
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::convert(message).c_str());
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! %s\n\n", ::convert(message).c_str());
				#endif
			}
			// Выводим результат
			return result;
		}
		// Размер буфера данных пользователя
		DWORD size = 0;
		// Токен пользователя
		PTOKEN_USER tokenUser = nullptr;
		// Сначала получаем размер буфера
		::GetTokenInformation(token, TokenUser, nullptr, 0, &size);
		// Если размер буфера мы не определили
		if(::GetLastError() != ERROR_INSUFFICIENT_BUFFER){
			// Если мы получили ошибку
			if(::GetLastError() != 0){
				// Создаём буфер сообщения ошибки
				wchar_t message[256] = {0};
				// Выполняем формирование текста ошибки
				::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 256, 0);
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::convert(message).c_str());
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! %s\n\n", ::convert(message).c_str());
				#endif
			}
			// Закрываем токен
			::CloseHandle(token);
			// Выводим результат
			return result;
		}
		// Выделяем память под токен пользователя
		tokenUser = (PTOKEN_USER) ::LocalAlloc(LPTR, size);
		// Если память не может быть выделена
		if(tokenUser == nullptr) {
			// Если мы получили ошибку
			if(::GetLastError() != 0){
				// Создаём буфер сообщения ошибки
				wchar_t message[256] = {0};
				// Выполняем формирование текста ошибки
				::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 256, 0);
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::convert(message).c_str());
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! %s\n\n", ::convert(message).c_str());
				#endif
			}
			// Закрываем токен
			::CloseHandle(token);
			// Выводим результат
			return result;
		}
		// Получаем информацию о пользователе
		if(::GetTokenInformation(token, TokenUser, tokenUser, size, &size)){
			// Итоговое имя пользователя
			LPWSTR username = nullptr;
			// Если имя пользователя мы извлекли успешно
			if(::ConvertSidToStringSidW(tokenUser->User.Sid, &username)){
				// Если результат мы получили
				if((username != nullptr) && (username[0] != L'\0'))
					// Запоминаем итоговое имя пользователя
					result = username;
				// Освобождаем память, выделенную ConvertSidToStringSid
				::LocalFree(username);
			}
		}
		// Освобождаем память выделенную для токена пользователя
		::LocalFree(tokenUser);
		// Закрываем токен процесса
		::CloseHandle(token);
		// Выводим результат
		return result;
	}
	/**
	 * @brief Метод получения списка групп текущего пользователя
	 *
	 * @return список групп текущего пользователя
	 */
	vector <wstring> awh::OS::groups() const noexcept {
		// Результат работы функции
		vector <wstring> result;
		// Токен текущего процесса
		HANDLE token = nullptr;
		// Открываем токен текущего процесса
		if(!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &token)){
			// Если мы получили ошибку
			if(::GetLastError() != 0){
				// Создаём буфер сообщения ошибки
				wchar_t message[256] = {0};
				// Выполняем формирование текста ошибки
				::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 256, 0);
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::convert(message).c_str());
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! %s\n\n", ::convert(message).c_str());
				#endif
			}
			// Выводим результат
			return result;
		}
		// Размер буфера данных пользователя
		DWORD size = 0;
		// Токен группы пользователя
		PTOKEN_GROUPS tokenGroups = nullptr;
		// Сначала получаем размер буфера
		::GetTokenInformation(token, TokenGroups, nullptr, 0, &size);
		// Если размер буфера мы не определили
		if(::GetLastError() != ERROR_INSUFFICIENT_BUFFER){
			// Если мы получили ошибку
			if(::GetLastError() != 0){
				// Создаём буфер сообщения ошибки
				wchar_t message[256] = {0};
				// Выполняем формирование текста ошибки
				::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 256, 0);
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::convert(message).c_str());
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! %s\n\n", ::convert(message).c_str());
				#endif
			}
			// Закрываем токен
			::CloseHandle(token);
			// Выводим результат
			return result;
		}
		// Выделяем память под токен группы
		tokenGroups = (PTOKEN_GROUPS) ::LocalAlloc(LPTR, size);
		// Если память не может быть выделена
		if(tokenGroups == nullptr){
			// Если мы получили ошибку
			if(::GetLastError() != 0){
				// Создаём буфер сообщения ошибки
				wchar_t message[256] = {0};
				// Выполняем формирование текста ошибки
				::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 256, 0);
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::convert(message).c_str());
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! %s\n\n", ::convert(message).c_str());
				#endif
			}
			// Закрываем токен
			::CloseHandle(token);
			// Выводим результат
			return result;
		}
		// Сначала получаем размер буфера
		if(::GetTokenInformation(token, TokenGroups, tokenGroups, size, &size)){
			// Выполняем перебор всех групп пользователя
			for(DWORD i = 0; i < tokenGroups->GroupCount; ++i){
				// Итоговое название группы пользователя
				LPWSTR usergroup = nullptr;
				// Если название группы пользователя мы извлекли успешно
				if(::ConvertSidToStringSidW(tokenGroups->Groups[i].Sid, &usergroup)){
					// Если результат мы получили
					if((usergroup != nullptr) && (usergroup[0] != L'\0'))
						// Добавляем полученное название группы пользователя в список групп
						result.push_back(usergroup);
					// Освобождаем память, выделенную ConvertSidToStringSid
					::LocalFree(usergroup);
				}
			}
		}
		// Освобождаем память выделенную для токена группы пользователя
		::LocalFree(tokenGroups);
		// Закрываем токен процесса
		::CloseHandle(token);
		// Выводим результат
		return result;
	}
	/**
	 * @brief Метод получения названия пользователя/группы по идентификатору
	 *
	 * @param sid идентификатор пользователя/группы
	 * @return    имя запрашиваемого пользователя/группы
	 */
	string awh::OS::account(const wstring & sid) const noexcept {
		// Результат работы функции
		string result = "";
		// Если идентификатор пользователя передан
		if(!sid.empty()){
			// Объект идентификатора пользователя
			PSID pSid = nullptr;
			// Выполняем идентификатор пользователя
			if(::ConvertStringSidToSidW(sid.c_str(), &pSid)){
				// Тип SID-а
				SID_NAME_USE sidType;
				// Размер имени пользователя и домена пользователя
				DWORD nameSize = 0, domainSize = 0;
				// Сначала вызываем для получения размеров буферов
				::LookupAccountSidW(nullptr, pSid, nullptr, &nameSize, nullptr, &domainSize, &sidType);
				// Если мы получиши ошибку извлечения размеров буфера
				if(::GetLastError() != ERROR_INSUFFICIENT_BUFFER){
					// Создаём буфер сообщения ошибки
					wchar_t message[256] = {0};
					// Выполняем формирование текста ошибки
					::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 256, 0);
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::convert(message).c_str());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! %s\n\n", ::convert(message).c_str());
					#endif
					// Выводим результат
					return result;
				}
				// Инициализируем имя пользователя
				wstring name(nameSize, L'\0');
				// Инициализируем доменное имя пользователя
				wstring domain(domainSize, L'\0');
				// Извлекаем имя пользователя и его доменное имя
				if(::LookupAccountSidW(nullptr, pSid, &name[0], &nameSize, &domain[0], &domainSize, &sidType)){
					// Если доменное имя установлено
					if(!domain.empty() && (domain[0] != '\0')){
						/**
						 * Формат: "DOMAIN\Username" или просто "Username" для локальных учетных записей
						 */
						result = (::convert(domain) + "\\" + ::convert(name));
					// Если доменное имя не получено
					} else result = ::convert(name);
				}
				// Освобождаем память выделенную под идентификатор пользователя
				::LocalFree(pSid);
			// Если конвертация не выполнена
			} else {
				// Если мы получили ошибку
				if(::GetLastError() != 0){
					// Создаём буфер сообщения ошибки
					wchar_t message[256] = {0};
					// Выполняем формирование текста ошибки
					::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 256, 0);
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::convert(message).c_str());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! %s\n\n", ::convert(message).c_str());
					#endif
				}
			}
		}
		// Выводим результат
		return result;
	}
	/**
	 * @brief Метод вывода идентификатора пользователя/группы
	 *
	 * @param name название пользователя/группы
	 * @return     полученный идентификатор пользователя/группы
	 */
	wstring awh::OS::account(const string & name) const noexcept {
		// Результат работы функции
		wstring result = L"";
		// Если имя пользователя/группы передано
		if(!name.empty()){
			// Тип SID-а
			SID_NAME_USE sidType;
			// Размер SID-а пользователя/группы и домена пользователя
			DWORD sidSize = 0, domainSize = 0;
			// Выполняем конвертирование название пользователя/группы
			wstring account = ::convert(name), actualDomain = L"";
			// Выполняем поиск разделителя
			const size_t pos = account.find(L"\\");
			// Если позиция разделителя доменного имя найдена
			if(pos != wstring::npos){
				// Извлекаем доменное имя
				actualDomain = account.substr(0, pos);
				// Удаляем из аккаунта доменное имя
				account.erase(0, pos + 1);
			}
			// Первый вызов — получаем размеры буферов
			::LookupAccountNameW(nullptr, account.c_str(), nullptr, &sidSize, nullptr, &domainSize, &sidType);
			// Если мы получиши ошибку извлечения размеров буфера
			if(::GetLastError() != ERROR_INSUFFICIENT_BUFFER){
				// Создаём буфер сообщения ошибки
				wchar_t message[256] = {0};
				// Выполняем формирование текста ошибки
				::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 256, 0);
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::convert(message).c_str());
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! %s\n\n", ::convert(message).c_str());
				#endif
				// Выводим результат
				return result;
			}
			// Инициализируем доменное имя пользователя
			wstring domain(domainSize, L'\0');
			// Выделяем память под SID и домен
			PSID pSid = (PSID) ::LocalAlloc(LPTR, sidSize);
			// Извлекаем SID пользователя/группы и его доменное имя
			if(::LookupAccountNameW(nullptr, account.c_str(), pSid, &sidSize, &domain[0], &domainSize, &sidType)){
				// Строка SID идентификатора пользователя/доменного имени
				LPWSTR sid = nullptr;
				// Выполняем извлечение SID идентификатор пользователя/доменного имени
				if(::ConvertSidToStringSidW(pSid, &sid) && ((domain.compare(actualDomain) == 0) || actualDomain.empty())){
					// Если результат мы получили
					if((sid != nullptr) && (sid[0] != L'\0'))
						// Выполняем получение SID-а
						result.assign(sid, sid + wcslen(sid));
					// Освобождаем строку, выделенную ConvertSidToStringSid
					::LocalFree(sid);
				}
			}
			// Освобождаем ресурсы
			::LocalFree(pSid);
		}
		// Выводим результат
		return result;
	}
	/**
	 * @brief Получение списка групп пользователя
	 *
	 * @param user имя пользователя чьи группы следует получить
	 * @return     список групп пользователя
	 */
	vector <wstring> awh::OS::groups(const string & user) const noexcept {
		// Результат работы функции
		vector <wstring> result;
		// Если имя пользователя передано
		if(!user.empty()){
			// Тип SID-а
			SID_NAME_USE sidType;
			// Размер SID-а пользователя и домена пользователя
			DWORD sidSize = 0, domainSize = 0;
			// Выполняем конвертирование название пользователя/группы
			const wstring & account = ::convert(user);
			// Первый вызов — получаем размеры буферов
			::LookupAccountNameW(nullptr, account.c_str(), nullptr, &sidSize, nullptr, &domainSize, &sidType);
			// Если мы получиши ошибку извлечения размеров буфера
			if(::GetLastError() != ERROR_INSUFFICIENT_BUFFER){
				// Создаём буфер сообщения ошибки
				wchar_t message[256] = {0};
				// Выполняем формирование текста ошибки
				::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 256, 0);
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::convert(message).c_str());
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! %s\n\n", ::convert(message).c_str());
				#endif
				// Выводим результат
				return result;
			}
			// Инициализируем доменное имя пользователя
			wstring domain(domainSize, L'\0');
			// Выделяем память под SID и домен
			PSID pSid = (PSID) ::LocalAlloc(LPTR, sidSize);
			// Извлекаем SID пользователя и его доменное имя
			if(!::LookupAccountNameW(nullptr, account.c_str(), pSid, &sidSize, &domain[0], &domainSize, &sidType)){
				// Освобождаем ресурсы
				::LocalFree(pSid);
				// Выводим пустой результат
				return result;
			}
			/**
			 * Открываем процесс и получаем его токен
			 * ⚠️ Это даст группы ДЛЯ ТЕКУЩЕГО ПРОЦЕССА.
			 * Для получения групп ЛЮБОГО пользователя нужно использовать LogonUser + OpenProcessToken.
			 */
			// Токен текущего процесса
			HANDLE token = nullptr;
			// Открываем токен текущего процесса
			if(!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &token)){
				// Если мы получили ошибку
				if(::GetLastError() != 0){
					// Создаём буфер сообщения ошибки
					wchar_t message[256] = {0};
					// Выполняем формирование текста ошибки
					::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 256, 0);
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::convert(message).c_str());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! %s\n\n", ::convert(message).c_str());
					#endif
				}
				// Освобождаем ресурсы
				::LocalFree(pSid);
				// Выводим пустой результат
				return result;
			}
			// Размер буфера данных
			DWORD size = 0;
			// Сначала получаем размер буфера
			::GetTokenInformation(token, TokenGroups, nullptr, 0, &size);
			// Если размер буфера мы не определили
			if(::GetLastError() != ERROR_INSUFFICIENT_BUFFER){
				// Если мы получили ошибку
				if(::GetLastError() != 0){
					// Создаём буфер сообщения ошибки
					wchar_t message[256] = {0};
					// Выполняем формирование текста ошибки
					::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 256, 0);
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::convert(message).c_str());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! %s\n\n", ::convert(message).c_str());
					#endif
				}
				// Закрываем токен
				::CloseHandle(token);
				// Освобождаем ресурсы
				::LocalFree(pSid);
				// Выводим результат
				return result;
			}
			// Выделяем память под токен группы
			PTOKEN_GROUPS tokenGroups = (PTOKEN_GROUPS) ::LocalAlloc(LPTR, size);
			// Если память не может быть выделена
			if(tokenGroups == nullptr){
				// Если мы получили ошибку
				if(::GetLastError() != 0){
					// Создаём буфер сообщения ошибки
					wchar_t message[256] = {0};
					// Выполняем формирование текста ошибки
					::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 256, 0);
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, ::convert(message).c_str());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! %s\n\n", ::convert(message).c_str());
					#endif
				}
				// Закрываем токен
				::CloseHandle(token);
				// Освобождаем ресурсы
				::LocalFree(pSid);
				// Выводим результат
				return result;
			}
			// Сначала получаем размер буфера
			if(::GetTokenInformation(token, TokenGroups, tokenGroups, size, &size)){
				// Выполняем перебор всех групп пользователя
				for(DWORD i = 0; i < tokenGroups->GroupCount; ++i){
					// Итоговое название группы пользователя
					LPWSTR usergroup = nullptr;
					// Если название группы пользователя мы извлекли успешно
					if(::ConvertSidToStringSidW(tokenGroups->Groups[i].Sid, &usergroup)){
						// Если результат мы получили верный
						if((usergroup != nullptr) && (usergroup[0] != L'\0'))
							// Добавляем полученное название группы пользователя в список групп
							result.push_back(usergroup);
						// Освобождаем память, выделенную ConvertSidToStringSid
						::LocalFree(usergroup);
					}
				}
			}
			// Освобождаем память выделенную для токена группы
			::LocalFree(tokenGroups);
			// Закрываем токен
			::CloseHandle(token);
			// Освобождаем ресурсы
			::LocalFree(pSid);
		}
		// Выводим результат
		return result;
	}
#endif
/**
 * Для операционной системы не являющейся MS Windows
 */
#if !_WIN32 && !_WIN64
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
	T awh::OS::sysctl(const string & name) const noexcept {
		// Результат работы функции
		T result;
		// Если данные являются основными
		if(is_integral <T>::value || is_floating_point <T>::value || is_array <T>::value){
			// Буфер результата по умолчанию
			uint8_t buffer[sizeof(T)];
			// Заполняем нулями буфер данных
			::memset(buffer, 0, sizeof(T));
			// Выполняем установку результата по умолчанию
			::memcpy(&result, reinterpret_cast <T *> (buffer), sizeof(T));
		}
		// Если название записи передано правильно
		if(!name.empty()){
			// Создаём буфер данных для извлечения данных
			vector <char> buffer;
			// Выполняем извлечение данных записи
			::sysctl(name, buffer);
			// Если данные буфера были извлечены удачно
			if(!buffer.empty()){
				// Если данные являются основными
				if(is_class <T>::value || is_integral <T>::value || is_floating_point <T>::value)
					// Выполняем получение данных
					::metadata(buffer, result);
			}
		}
		// Выводим результат
		return result;
	}
	/**
	 * Объявляем прототипы для извлечения значений настроек ядра операционной системы
	 */
	template int8_t awh::OS::sysctl <int8_t> (const string &) const noexcept;
	template uint8_t awh::OS::sysctl <uint8_t> (const string &) const noexcept;
	template int16_t awh::OS::sysctl <int16_t> (const string &) const noexcept;
	template uint16_t awh::OS::sysctl <uint16_t> (const string &) const noexcept;
	template int32_t awh::OS::sysctl <int32_t> (const string &) const noexcept;
	template uint32_t awh::OS::sysctl <uint32_t> (const string &) const noexcept;
	template int64_t awh::OS::sysctl <int64_t> (const string &) const noexcept;
	template uint64_t awh::OS::sysctl <uint64_t> (const string &) const noexcept;
	template float awh::OS::sysctl <float> (const string &) const noexcept;
	template double awh::OS::sysctl <double> (const string &) const noexcept;
	template string awh::OS::sysctl <string> (const string &) const noexcept;
	/**
	 * Если операционной системой является MacOS X или Linux
	 */
	#if __APPLE__ || __MACH__ || __Linux__
		template size_t awh::OS::sysctl <size_t> (const string &) const noexcept;
		template ssize_t awh::OS::sysctl <ssize_t> (const string &) const noexcept;
	#endif
	/**
	 * Объявляем прототипы для извлечения списка значений настроек ядра операционной системы
	 */
	template vector <int8_t> awh::OS::sysctl <vector <int8_t>> (const string &) const noexcept;
	template vector <uint8_t> awh::OS::sysctl <vector <uint8_t>> (const string &) const noexcept;
	template vector <int16_t> awh::OS::sysctl <vector <int16_t>> (const string &) const noexcept;
	template vector <uint16_t> awh::OS::sysctl <vector <uint16_t>> (const string &) const noexcept;
	template vector <int32_t> awh::OS::sysctl <vector <int32_t>> (const string &) const noexcept;
	template vector <uint32_t> awh::OS::sysctl <vector <uint32_t>> (const string &) const noexcept;
	template vector <int64_t> awh::OS::sysctl <vector <int64_t>> (const string &) const noexcept;
	template vector <uint64_t> awh::OS::sysctl <vector <uint64_t>> (const string &) const noexcept;
	template vector <float> awh::OS::sysctl <vector <float>> (const string &) const noexcept;
	template vector <double> awh::OS::sysctl <vector <double>> (const string &) const noexcept;
	template vector <string> awh::OS::sysctl <vector <string>> (const string &) const noexcept;
	/**
	 * Если операционной системой является MacOS X или Linux
	 */
	#if __APPLE__ || __MACH__ || __Linux__
		template vector <size_t> awh::OS::sysctl <vector <size_t>> (const string &) const noexcept;
		template vector <ssize_t> awh::OS::sysctl <vector <ssize_t>> (const string &) const noexcept;
	#endif
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
	bool awh::OS::sysctl(const string & name, const T value) const noexcept {
		// Если название записи для установки настроек передано
		if(!name.empty() && (is_integral <T>::value || is_floating_point <T>::value)){
			/**
			 * Если это Linux
			 */
			#if __linux__
				// Выполняем преобразование числа в строку
				const string param = std::to_string(value);
				// Выполняем установку буфера бинарных данных
				return ::sysctl(name, param.c_str(), param.size());
			/**
			 * Если это другая операционная система
			 */
			#else
				// Буфер результата по умолчанию
				vector <uint8_t> buffer(sizeof(value), 0);
				// Выполняем установку результата по умолчанию
				::memcpy(buffer.data(), &value, sizeof(value));
				// Выполняем установку буфера бинарных данных
				return ::sysctl(name, buffer.data(), buffer.size());
			#endif
		}
		// Сообщаем, что ничего не установленно
		return false;
	}
	/**
	 * Объявляем прототипы для установки значений настроек ядра операционной системы
	 */
	template bool awh::OS::sysctl <int8_t> (const string &, const int8_t) const noexcept;
	template bool awh::OS::sysctl <uint8_t> (const string &, const uint8_t) const noexcept;
	template bool awh::OS::sysctl <int16_t> (const string &, const int16_t) const noexcept;
	template bool awh::OS::sysctl <uint16_t> (const string &, const uint16_t) const noexcept;
	template bool awh::OS::sysctl <int32_t> (const string &, const int32_t) const noexcept;
	template bool awh::OS::sysctl <uint32_t> (const string &, const uint32_t) const noexcept;
	template bool awh::OS::sysctl <int64_t> (const string &, const int64_t) const noexcept;
	template bool awh::OS::sysctl <uint64_t> (const string &, const uint64_t) const noexcept;
	template bool awh::OS::sysctl <float> (const string &, const float) const noexcept;
	template bool awh::OS::sysctl <double> (const string &, const double) const noexcept;
	/**
	 * Если операционной системой является MacOS X или Linux
	 */
	#if __APPLE__ || __MACH__ || __Linux__
		template bool awh::OS::sysctl <size_t> (const string &, const size_t) const noexcept;
		template bool awh::OS::sysctl <ssize_t> (const string &, const ssize_t) const noexcept;
	#endif
	/**
	 * @brief Метод установки настроек ядра операционной системы
	 *
	 * @param name  название записи для установки настроек
	 * @param value значение записи для установки настроек
	 * @return      результат выполнения установки
	 */
	bool awh::OS::sysctl(const string & name, const string & value) const noexcept {
		// Если название записи для установки настроек передано
		if(!name.empty())
			// Выполняем установку буфера бинарных данных
			return ::sysctl(name, value.c_str(), value.size());
		// Сообщаем, что ничего не установленно
		return false;
	}
	/**
	 * @brief Метод установки настроек ядра операционной системы
	 *
	 * @param name  название записи для установки настроек
	 * @param value значение записи для установки настроек
	 * @return      результат выполнения установки
	 */
	bool awh::OS::sysctl(const string & name, const char * value) const noexcept {
		// Если название записи для установки настроек передано
		if(!name.empty())
			// Выполняем установку буфера бинарных данных
			return ::sysctl(name, value, ::strlen(value));
		// Сообщаем, что ничего не установленно
		return false;
	}
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
	bool awh::OS::sysctl(const string & name, const vector <T> & items) const noexcept {
		// Если название записи для установки настроек передано
		if(!name.empty() && (is_integral <T>::value || is_floating_point <T>::value)){
			/**
			 * Если это Linux
			 */
			#if __linux__
				// Выполняем преобразование числа в строку
				string param = "";
				// Выполняем перебор всего списка параметров
				for(auto & item : items){
					// Если строка уже сформированна
					if(!param.empty())
						// Выполняем добавление пробела
						param.append(1, ' ');
					// Добавляем полученное значение в список
					param.append(std::to_string(item));
				}
				// Выполняем установку буфера бинарных данных
				return ::sysctl(name, param.c_str(), param.size());
			/**
			 * Если это другая операционная система
			 */
			#else
				// Смещение в бинарном буфере
				size_t offset = 0;
				// Буфер результата по умолчанию
				vector <uint8_t> buffer(items.size() * sizeof(T), 0);
				// Выполняем перебор всего списка параметров
				for(auto & item : items){
					// Выполняем установку результата по умолчанию
					::memcpy(buffer.data() + offset, &item, sizeof(item));
					// Выполняем увеличение смещения в буфере
					offset += sizeof(item);
				}
				// Выполняем установку буфера бинарных данных
				return ::sysctl(name, buffer.data(), buffer.size());
			#endif
		}
		// Сообщаем, что ничего не установленно
		return false;
	}
	/**
	 * Объявляем прототипы для установки списка значений настроек ядра операционной системы
	 */
	template bool awh::OS::sysctl <int8_t> (const string &, const vector <int8_t> &) const noexcept;
	template bool awh::OS::sysctl <uint8_t> (const string &, const vector <uint8_t> &) const noexcept;
	template bool awh::OS::sysctl <int16_t> (const string &, const vector <int16_t> &) const noexcept;
	template bool awh::OS::sysctl <uint16_t> (const string &, const vector <uint16_t> &) const noexcept;
	template bool awh::OS::sysctl <int32_t> (const string &, const vector <int32_t> &) const noexcept;
	template bool awh::OS::sysctl <uint32_t> (const string &, const vector <uint32_t> &) const noexcept;
	template bool awh::OS::sysctl <int64_t> (const string &, const vector <int64_t> &) const noexcept;
	template bool awh::OS::sysctl <uint64_t> (const string &, const vector <uint64_t> &) const noexcept;
	template bool awh::OS::sysctl <float> (const string &, const vector <float> &) const noexcept;
	template bool awh::OS::sysctl <double> (const string &, const vector <double> &) const noexcept;
	/**
	 * Если операционной системой является MacOS X или Linux
	 */
	#if __APPLE__ || __MACH__ || __Linux__
		template bool awh::OS::sysctl <size_t> (const string &, const vector <size_t> &) const noexcept;
		template bool awh::OS::sysctl <ssize_t> (const string &, const vector <ssize_t> &) const noexcept;
	#endif
	/**
	 * @brief Метод установки настроек ядра операционной системы
	 *
	 * @param name  название записи для установки настроек
	 * @param items значение записи для установки настроек
	 * @return      результат выполнения установки
	 */
	bool awh::OS::sysctl(const string & name, const vector <string> & items) const noexcept {
		// Если название записи для установки настроек передано
		if(!name.empty()){
			// Выполняем преобразование числа в строку
			string param = "";
			// Выполняем перебор всего списка параметров
			for(auto & item : items){
				// Если строка уже сформированна
				if(!param.empty())
					// Выполняем добавление пробела
					param.append(1, ' ');
				// Добавляем полученное значение в список
				param.append(item);
			}
			// Выполняем установку буфера бинарных данных
			return ::sysctl(name, param.c_str(), param.size());
		}
		// Сообщаем, что ничего не установленно
		return false;
	}
	/**
	 * @brief Метод установки настроек ядра операционной системы
	 *
	 * @param name  название записи для установки настроек
	 * @param items значение записи для установки настроек
	 * @return      результат выполнения установки
	 */
	bool awh::OS::sysctl(const string & name, const vector <const char *> & items) const noexcept {
		// Если название записи для установки настроек передано
		if(!name.empty()){
			// Выполняем преобразование числа в строку
			string param = "";
			// Выполняем перебор всего списка параметров
			for(auto & item : items){
				// Если строка уже сформированна
				if(!param.empty())
					// Выполняем добавление пробела
					param.append(1, ' ');
				// Добавляем полученное значение в список
				param.append(item);
			}
			// Выполняем установку буфера бинарных данных
			return ::sysctl(name, param.c_str(), param.size());
		}
		// Сообщаем, что ничего не установленно
		return false;
	}
#endif
/**
 * @brief Метод запуска внешнего приложения
 *
 * @param cmd       команда запуска
 * @param multiline данные должны вернутся многострочные
 */
string awh::OS::exec(const string & cmd, const bool multiline) const noexcept {
	// Полученный результат
	string result = "";
	// Если комманда запуска приложения передана правильно
	if(!cmd.empty()){
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#if !_WIN32 && !_WIN64
			// Создаем буфер для чтения результата
			char buffer[128];
			// Создаем пайп для чтения результата работы OS
			FILE * stream = ::popen(cmd.c_str(), "r");
			// Если пайп открыт
			if(stream != nullptr){
				/**
				 * Считываем до тех пор пока все не прочитаем
				 */
				while(::fgets(buffer, sizeof(buffer), stream) != nullptr){
					// Добавляем полученный результат
					result.append(buffer);
					// Если это не мультилайн
					if(!multiline)
						// Выходим из цикла
						break;
				}
				// Закрываем пайп
				::pclose(stream);
			}
		/**
		 * Для операционной системы MS Windows
		 */
		#else
			// Создаем буфер для чтения результата
			wchar_t buffer[128];
			// Создаем пайп для чтения результата работы OS
			FILE * stream = ::_wpopen(::convert(cmd).c_str(), L"rt");
			// Если пайп открыт
			if(stream){
				/**
				 * Считываем до тех пор пока все не прочитаем
				 */
				while(::fgetws(buffer, sizeof(buffer), stream) != nullptr){
					// Выполняем конвертирование в utf-8 строку
					result.append(::convert(buffer));
					// Если это не мультилайн
					if(!multiline)
						// Выходим из цикла
						break;
				}
				// Закрываем пайп
				::_pclose(stream);
			}
		#endif
	}
	// Выводим результат
	return result;
}
