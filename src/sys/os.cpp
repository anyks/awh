/**
 * @file: os.cpp
 * @date: 2025-10-25
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
 * Стандартные заголовочные файлы
 */
#include <memory>
#include <cerrno>
#include <cstring>
#include <cstddef>
#include <cstdlib>
#include <iostream>

/**
 * Если используется аллокатор TcMalloc
 */
#if __AWH_USE_TCMALLOC__
	/**
	 * Заголовочный файл аллокатора TcMalloc
	 */
	#include <gperftools/malloc_extension.h>
/**
 * Если используется аллокатор Glibc
 */
#elif __GLIBC__
	/**
	 * Заголовочный файл аллокатора Glibc
	 */
	#include <malloc.h>
#endif

/**
 * Операционной системой является Linux
 */
#if __linux__
	/**
	 * Стандартный заголовочный файл
	 */
	#include <queue>

	/**
	 * Системный заголовочный файл
	 */
	#include <linux/sysctl.h>
/**
 * Если операционной системой является FreeBSD, NetBSD и OpenBSD
 */
#elif __FreeBSD__ || __NetBSD__ || __OpenBSD__
	/**
	 * Системные заголовочные файлы
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
	 * Системные заголовочные файлы
	 */
	#include <pwd.h>
	#include <grp.h>
	#include <sys/resource.h>
#endif

/**
 * Если операционной системой является macOS, FreeBSD, NetBSD и OpenBSD
 */
#if __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
	/**
	 * Системный заголовочный файл
	 */
	#include <sys/sysctl.h>
#endif

/**
 * Если операционной системой является macOS и Unix
 */
#if __unix__ || __unix || unix || (__APPLE__ && __MACH__)
	/**
	 * Системные заголовочные файлы
	 */
	#include <unistd.h>
	#include <sys/mman.h>
	#include <sys/resource.h>

	/**
	 * Для операционной системы macOS
	 */
	#if __APPLE__ || __MACH__
		/**
		 * Системные заголовочные файлы
		 */
		#include <mach/mach.h>
		#include <malloc/malloc.h>
	/**
	 * Для операционной системы Sun Solaris
	 */
	#elif (_AIX || __TOS__AIX__) || (__sun__ || __sun || sun && (__SVR4 || __svr4__))
		/**
		 * Системные заголовочные файлы
		 */
		#include <fcntl.h>
		#include <procfs.h>
	/**
	 * Подключаем заголовки для Linux
	 */
	#elif __linux__ || __linux || linux || __gnu_linux__
		/**
		 * Системный заголовочный файл
		 */
		#include <stdio.h>
	#endif
#endif

/**
 * Подключаем заголовочный файл проекта
 */
#include <sys/os.hpp>

/**
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
	/**
	 * Системные заголовочные файлы
	 */
	#include <wchar.h>
	#include <psapi.h>

	/**
	 * Если активирован компилятор MinGW
	 */
	#if __MINGW32__ || __MINGW64__
		/**
		 * Заголовочный файл аллокатора
		 */
		#include <malloc.h>
	/**
	 * Если активирован компилятор MS Visual Studio
	 */
	#else
		/**
		 * Заголовочный файл контроллера памяти
		 */
		#include <process.h>
	#endif
#endif

/**
 * Используем стандартное пространство имён
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
			// Выполняем копирование только выровненного по размеру типа количества байт (защита от выхода за границы)
			::memcpy(reinterpret_cast <char *> (result.data()), reinterpret_cast <const char *> (&buffer[0]), result.size() * sizeof(T));
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
		if(!buffer.empty()){
			// Определяем безопасное количество копируемых байт (не больше размера буфера и размера результата)
			const size_t length = (buffer.size() < sizeof(result) ? buffer.size() : sizeof(result));
			// Выполняем копирование полученных данных без выхода за границы буфера
			::memcpy(&result, &buffer[0], length);
		}
	}
	/**
	 * Тип элемента списка групп пользователя для функции getgrouplist (на macOS прототип использует int32_t, на остальных системах gid_t)
	 */
	#if __APPLE__ || __MACH__
		/**
		 * @brief Тип элемента списка групп пользователя для функции getgrouplist на macOS
		 *
		 */
		typedef int32_t grouplist_t;
	/**
	 * Для операционной системы не являющейся macOS
	 */
	#else
		/**
		 * @brief Тип элемента списка групп пользователя для функции getgrouplist для операционной системы не являющейся macOS
		 *
		 */
		typedef gid_t grouplist_t;
	#endif
	/**
	 * @brief Функция извлечения данных пользователя по его идентификатору с динамическим буфером
	 *
	 * @param uid    идентификатор пользователя
	 * @param pwd    объект параметров пользователя
	 * @param buffer буфер данных для извлечения
	 * @param data   указатель на извлечённые данные пользователя
	 * @return       результат извлечения данных
	 */
	static bool getUserById(const uid_t uid, struct passwd & pwd, vector <char> & buffer, struct passwd ** data) noexcept {
		// Получаем рекомендованный системой размер буфера данных
		const long size = ::sysconf(_SC_GETPW_R_SIZE_MAX);
		// Выделяем память для буфера данных (с запасным значением, если система не вернула размер)
		buffer.resize((size > 0) ? static_cast <size_t> (size) : 1024);
		// Результат извлечения данных
		int32_t result = 0;
		/**
		 * Выполняем извлечение данных пользователя, увеличивая буфер при его нехватке
		 */
		while((result = ::getpwuid_r(uid, &pwd, &buffer[0], buffer.size(), data)) == ERANGE)
			// Увеличиваем размер буфера данных вдвое
			buffer.resize(buffer.size() * 2);
		// Возвращаем результат извлечения данных
		return ((result == 0) && (* data != nullptr));
	}
	/**
	 * @brief Функция извлечения данных пользователя по его имени с динамическим буфером
	 *
	 * @param name   имя пользователя
	 * @param pwd    объект параметров пользователя
	 * @param buffer буфер данных для извлечения
	 * @param data   указатель на извлечённые данные пользователя
	 * @return       результат извлечения данных
	 */
	static bool getUserByName(const char * name, struct passwd & pwd, vector <char> & buffer, struct passwd ** data) noexcept {
		// Получаем рекомендованный системой размер буфера данных
		const long size = ::sysconf(_SC_GETPW_R_SIZE_MAX);
		// Выделяем память для буфера данных (с запасным значением, если система не вернула размер)
		buffer.resize((size > 0) ? static_cast <size_t> (size) : 1024);
		// Результат извлечения данных
		int32_t result = 0;
		/**
		 * Выполняем извлечение данных пользователя, увеличивая буфер при его нехватке
		 */
		while((result = ::getpwnam_r(name, &pwd, &buffer[0], buffer.size(), data)) == ERANGE)
			// Увеличиваем размер буфера данных вдвое
			buffer.resize(buffer.size() * 2);
		// Возвращаем результат извлечения данных
		return ((result == 0) && (* data != nullptr));
	}
	/**
	 * @brief Функция извлечения данных группы по её идентификатору с динамическим буфером
	 *
	 * @param gid    идентификатор группы пользователя
	 * @param grp    объект параметров группы пользователя
	 * @param buffer буфер данных для извлечения
	 * @param data   указатель на извлечённые данные группы пользователя
	 * @return       результат извлечения данных
	 */
	static bool getGroupById(const gid_t gid, struct group & grp, vector <char> & buffer, struct group ** data) noexcept {
		// Получаем рекомендованный системой размер буфера данных
		const long size = ::sysconf(_SC_GETGR_R_SIZE_MAX);
		// Выделяем память для буфера данных (с запасным значением, если система не вернула размер)
		buffer.resize((size > 0) ? static_cast <size_t> (size) : 1024);
		// Результат извлечения данных
		int32_t result = 0;
		/**
		 * Выполняем извлечение данных группы, увеличивая буфер при его нехватке
		 */
		while((result = ::getgrgid_r(gid, &grp, &buffer[0], buffer.size(), data)) == ERANGE)
			// Увеличиваем размер буфера данных вдвое
			buffer.resize(buffer.size() * 2);
		// Возвращаем результат извлечения данных
		return ((result == 0) && (* data != nullptr));
	}
	/**
	 * @brief Функция извлечения данных группы по её имени с динамическим буфером
	 *
	 * @param name   название группы пользователя
	 * @param grp    объект параметров группы пользователя
	 * @param buffer буфер данных для извлечения
	 * @param data   указатель на извлечённые данные группы пользователя
	 * @return       результат извлечения данных
	 */
	static bool getGroupByName(const char * name, struct group & grp, vector <char> & buffer, struct group ** data) noexcept {
		// Получаем рекомендованный системой размер буфера данных
		const long size = ::sysconf(_SC_GETGR_R_SIZE_MAX);
		// Выделяем память для буфера данных (с запасным значением, если система не вернула размер)
		buffer.resize((size > 0) ? static_cast <size_t> (size) : 1024);
		// Результат извлечения данных
		int32_t result = 0;
		/**
		 * Выполняем извлечение данных группы, увеличивая буфер при его нехватке
		 */
		while((result = ::getgrnam_r(name, &grp, &buffer[0], buffer.size(), data)) == ERANGE)
			// Увеличиваем размер буфера данных вдвое
			buffer.resize(buffer.size() * 2);
		// Возвращаем результат извлечения данных
		return ((result == 0) && (* data != nullptr));
	}
	/**
	 * @brief Метод извлечения настроек ядра операционной системы
	 *
	 * @param name   название записи для получения настроек
	 * @param buffer бинарный буфер с извлечёнными значениями
	 */
	static void sysctl(string_view name, vector <char> & buffer) noexcept {
		// Если название параметра и тип извлекаемого значения переданы
		if(!name.empty()){
			// Выполняем очистку буфера данных
			buffer.clear();
			/**
			 * Если мы работаем в macOS, FreeBSD, NetBSD или OpenBSD
			 */
			#if __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
				// Получаем размер буфера
				size_t length = 0;
				// Если размеры удачно получены
				if(::sysctlbyname(name.data(), nullptr, &length, nullptr, 0) == 0){
					// Выделяем в буфере нужное количество памяти
					buffer.resize(length, 0);
					// Запрашиваем искомые данные
					if(::sysctlbyname(name.data(), &buffer[0], &length, nullptr, 0) < 0)
						// Выполняем очистку буфера данных
						buffer.clear();
				}
			/**
			 * Если это Linux
			 */
			#elif __linux__
				// Формируем путь к параметру ядра в виртуальной файловой системе /proc/sys
				string path = "/proc/sys/";
				/**
				 * Преобразуем точечную нотацию параметра в путь файловой системы (net.core.somaxconn -> net/core/somaxconn)
				 */
				for(const char letter : name)
					// Заменяем разделитель точки на разделитель каталогов
					path.append(1, (letter == '.') ? '/' : letter);
				// Открываем файл параметра ядра на чтение
				FILE * file = ::fopen(path.c_str(), "rb");
				// Если файл параметра ядра удачно открыт
				if(file != nullptr){
					// Прочитанное значение параметра ядра
					string result;
					// Буфер для чтения данных
					char chunk[256];
					// Количество прочитанных байт
					size_t bytes = 0;
					/**
					 * Считываем содержимое файла параметра ядра целиком
					 */
					while((bytes = ::fread(chunk, sizeof(char), sizeof(chunk), file)) > 0)
						// Добавляем прочитанный блок данных к результату
						result.append(chunk, bytes);
					// Закрываем файл параметра ядра
					::fclose(file);
					// Если результат получен
					if(!result.empty()){
						// Очередь собранных данных
						std::queue <std::pair <string, bool>> data;
						/**
						 * Выполняем перебор всего полученного результата
						 */
						for(const char item : result){
							// Если символ является пробельным
							if(::isspace(static_cast <uint8_t> (item))){
								// Если очередь уже не пустая
								if(!data.empty()){
									// Если запись является числом
									if(data.back().second){
										// Если текущая запись уже содержит данные, начинаем новую запись
										if(!data.back().first.empty())
											// Выполняем добавление новой пустой числовой записи в очередь
											data.push(std::make_pair(string(""), true));
									// Если запись является строкой, добавляем разделитель в запись
									} else data.back().first.append(1, ' ');
								}
							// Если символ является числом
							} else if(std::isdigit(static_cast <uint8_t> (item)) || ((item == '-') && (data.empty() || data.back().first.empty()))) {
								// Если данных в очереди ещё нет
								if(data.empty())
									// Выполняем создание блока данных
									data.push(std::make_pair(string(1, item), true));
								// Если данные в очереди уже есть, добавляем полученный символ в запись
								else data.back().first.append(1, item);
							// Если символ является простым символом
							} else {
								// Если данных в очереди ещё нет
								if(data.empty())
									// Выполняем создание блока данных
									data.push(std::make_pair(string(1, item), false));
								// Если данные в очереди уже есть
								else {
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
											// Выполняем преобразование в unsigned uint32_t
											const uint32_t value2 = static_cast <uint32_t> (value1);
											// Выполняем добавление новой записи в буфер
											buffer.insert(buffer.end(), reinterpret_cast <const char *> (&value2), reinterpret_cast <const char *> (&value2) + sizeof(value2));
										// Выполняем добавление новой записи в буфер
										} else buffer.insert(buffer.end(), reinterpret_cast <const char *> (&value1), reinterpret_cast <const char *> (&value1) + sizeof(value1));
									/**
									 * Если возникает ошибка преобразования числа
									 */
									} catch(const exception &) {
										// Значение по умолчанию при ошибке преобразования
										const uint32_t value = 0;
										// Выполняем добавление нулевой записи в буфер
										buffer.insert(buffer.end(), reinterpret_cast <const char *> (&value), reinterpret_cast <const char *> (&value) + sizeof(value));
									}
								// Если запись является текстом
								} else {
									// Если последний символ является пробельным, удаляем его
									if(::isspace(static_cast <uint8_t> (data.front().first.back())))
										// Выполняем удаление последнего символа
										data.front().first.pop_back();
									// Выполняем добавление новой записи в буфер
									buffer.insert(buffer.end(), data.front().first.begin(), data.front().first.end());
								}
							}
							// Удаляем используемую запись
							data.pop();
						}
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
	static bool sysctl(string_view name, const void * buffer, const size_t size) noexcept {
		// Если название параметра передано
		if(!name.empty() && (buffer != nullptr) && (size > 0)){
			/**
			 * Если мы работаем в macOS, FreeBSD, NetBSD или OpenBSD
			 */
			#if __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
				// Устанавливаем новые параметры настройки ядра
				return (::sysctlbyname(name.data(), nullptr, 0, const_cast <uint8_t *> (reinterpret_cast <const uint8_t *> (buffer)), size) == 0);
			/**
			 * Операционной системой является Linux
			 */
			#elif __linux__
				// Формируем путь к параметру ядра в виртуальной файловой системе /proc/sys
				string path = "/proc/sys/";
				/**
				 * Преобразуем точечную нотацию параметра в путь файловой системы (net.core.somaxconn -> net/core/somaxconn)
				 */
				for(const char letter : name)
					// Заменяем разделитель точки на разделитель каталогов
					path.append(1, (letter == '.') ? '/' : letter);
				// Открываем файл параметра ядра на запись
				FILE * file = ::fopen(path.c_str(), "wb");
				// Если файл параметра ядра удачно открыт
				if(file != nullptr){
					// Выполняем запись значения параметра ядра
					const size_t bytes = ::fwrite(buffer, sizeof(char), size, file);
					// Закрываем файл параметра ядра
					::fclose(file);
					// Сообщаем результат записи значения параметра ядра
					return (bytes == size);
				}
				// Сообщаем, что значение параметра ядра не установлено
				return false;
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
	static string convert(wstring_view text) noexcept {
		// Переменная результата
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
		// Возвращаем результат
		return result;
	}
	/**
	 * @brief Функция конвертация строки из string в wstring
	 *
	 * @param text текст для конвертации
	 * @return     результат проверки
	 */
	static wstring convert(string_view text) noexcept {
		// Переменная результата
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
		// Возвращаем результат
		return result;
	}
#endif
/**
 * @brief isAdmin Метод проверпи запущено ли приложение под суперпользователем
 *
 * @return результат проверки
 */
bool awh::Operating_System::isAdmin() const noexcept {
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#if !_WIN32 && !_WIN64
		// Возвращаем результат проверки
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
		// Возвращаем результат проверки
		return (isAdmin != FALSE);
	#endif
}
/**
 * @brief Метод определения операционной системы
 *
 * @return семейство операционных систем
 */
awh::Operating_System::family_t awh::Operating_System::family() const noexcept {
	/**
	 * Операционной системой является Windows 32bit
	 */
	#ifdef _WIN32
		// Возвращаем флаг операционной системы
		return family_t::WIND32;
	/**
	 * Операционной системой является Windows 64bit
	 */
	#elif _WIN64
		// Возвращаем флаг операционной системы
		return family_t::WIND64;
	/**
	 * Операционной системой является macOS
	 */
	#elif __APPLE__ || __MACH__
		// Возвращаем флаг операционной системы
		return family_t::MACOSX;
	/**
	 * Операционной системой является Linux
	 */
	#elif __linux__
		// Возвращаем флаг операционной системы
		return family_t::LINUX;
	/**
	 * Операционной системой является FreeBSD
	 */
	#elif __FreeBSD__
		// Возвращаем флаг операционной системы
		return family_t::FREEBSD;
	/**
	 * Операционной системой является NetBSD
	 */
	#elif __NetBSD__
		// Возвращаем флаг операционной системы
		return family_t::NETBSD;
	/**
	 * Операционной системой является OpenBSD
	 */
	#elif __OpenBSD__
		// Возвращаем флаг операционной системы
		return family_t::OPENBSD;
	/**
	 * Реализация под Sun Solaris
	 */
	#elif __sun__
		/**
		 * Если операционной системой является OpenSolaris
		 */
		#ifdef __illumos__
			// Возвращаем флаг операционной системы
			return family_t::ILLUMOS;
		#else
			// Возвращаем флаг операционной системы
			return family_t::SOLARIS;
		#endif
	/**
	 * Операционной системой является Unix
	 */
	#elif __unix || __unix__
		// Возвращаем флаг операционной системы
		return family_t::UNIX;
	/**
	 * Операционной системой не распознана
	 */
	#else
		// Возвращаем флаг операционной системы
		return family_t::NONE;
	#endif
}
/**
 * @brief Метод определение архитектуры процессора
 *
 * @return архитектура процессора
 */
awh::Operating_System::cpu_t awh::Operating_System::architecture() const noexcept {
	/**
	 * Если процессор принадлежит к x86_64
	 */
	#if __x86_64__ || _M_X64
		// Возвращаем определённую архитектуру процессора
		return cpu_t::AMD64;
	/**
	 * Если процессор принадлежит к ARM64
	 */
	#elif __aarch64__ || _M_ARM64
		// Возвращаем определённую архитектуру процессора
		return cpu_t::ARM64;
	/**
	 * Если процессор принадлежит к ARM
	 */
	#elif __arm__ || _M_ARM
		// Возвращаем определённую архитектуру процессора
		return cpu_t::ARM;
	/**
	 * Если процессор принадлежит к x86
	 */
	#elif __i386__ || _M_IX86
		// Возвращаем определённую архитектуру процессора
		return cpu_t::X86;
	/**
	 * Если процессор принадлежит к PowerPC
	 */
	#elif __powerpc__ || __ppc__
		// Возвращаем определённую архитектуру процессора
		return cpu_t::PPC;
	/**
	 * Если процессор принадлежит к MIPS
	 */
	#elif __mips__
		// Возвращаем определённую архитектуру процессора
		return cpu_t::MIPS;
	/**
	 * Для остальных типов процессоров
	 */
	#else
		// Возвращаем определённую архитектуру процессора
		return cpu_t::UNKNOWN;
	#endif
}
/**
 * @brief Метод определения текущего расхода памяти
 *
 * @param mode режим потребления памяти
 * @return     размер расхода памяти
 */
size_t awh::Operating_System::rss(const rss_t mode) const noexcept {
	// Переменная результата
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
				 * Если используется аллокатор TcMalloc
				 */
				#if __AWH_USE_TCMALLOC__
					// Выполняем получение занятой памяти выделенной аллокатором
					MallocExtension::instance()->GetNumericProperty("generic.current_allocated_bytes", &result);
				/**
				 * Если используются стандартные аллокаторы
				 */
				#else
					/**
					 * Для операционной системы MS Windows
					 */
					#if _WIN32 || _WIN64
						// Создаём объект информации о памяти
						PROCESS_MEMORY_COUNTERS info;
						// Выполняем извлечение данных текущего процесса
						if(!::GetProcessMemoryInfo(::GetCurrentProcess(), &info, sizeof(info))){
							// Создаём буфер сообщения ошибки
							wchar_t message[0xFF] = {0};
							// Выполняем формирование текста ошибки
							::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 0xFF, 0);
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::convert(message).c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::CRITICAL, ::convert(message).c_str());
							#endif
						// Выполняем извлечение размера пика потребляемой памяти
						} else result = static_cast <size_t> (info.WorkingSetSize);
					/**
					 * Для операционной системы macOS
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
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, "Unable to access to determine memory consumption");
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::CRITICAL, "Unable to access to determine memory consumption");
							#endif
							// Возвращаем пустой результат
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
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
							#endif
							// Возвращаем пустой результат
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
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
							#endif
							// Возвращаем пустой результат
							return result;
						}
						// Выполняем чтение данных потребления памяти
						if(::fscanf(file, "%*s%ld", &rss) != 1){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
							#endif
							// Выполняем закрытие файлового дескриптора
							::fclose(file);
							// Возвращаем пустой результат
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
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
							#endif
							// Возвращаем пустой результат
							return result;
						}
						// Выполняем чтение данных потребления памяти
						if(::read(sock, &psinfo, sizeof(psinfo) ) != sizeof(psinfo)){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
							#endif
							// Выполняем закрытие файлового дескриптора
							::close(sock);
							// Возвращаем пустой результат
							return result;
						}
						// Выполняем закрытие файлового дескриптора
						::close(sock);
						// Выполняем извлечение размера пика потребляемой памяти
						result = static_cast <size_t> (psinfo.pr_rssize * 1024L);
					#endif
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
						wchar_t message[0xFF] = {0};
						// Выполняем формирование текста ошибки
						::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 0xFF, 0);
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::convert(message).c_str());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::CRITICAL, ::convert(message).c_str());
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
							// Записываем ошибку в лог
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
						#endif
						// Возвращаем пустой результат
						return result;
					}
					// Выполняем чтение данных потребления памяти
					if(::read(sock, &psinfo, sizeof(psinfo) ) != sizeof(psinfo)){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
						#endif
						// Выполняем закрытие файлового дескриптора
						::close(sock);
						// Возвращаем пустой результат
						return result;
					}
					// Выполняем закрытие файлового дескриптора
					::close(sock);
					// Выполняем извлечение размера пика потребляемой памяти
					result = static_cast <size_t> (psinfo.pr_rssize * 1024L);
				/**
				 * Если операционной системой является macOS, FreeBSD, NetBSD, OpenBSD и Linux
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
							// Записываем ошибку в лог
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
						#endif
					// Если данные получили удачно
					} else {
						/**
						 * Реализация под macOS
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
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод вывода статистики расхода памяти
 *
 */
void awh::Operating_System::printStatsMemory() const noexcept {
	/**
	 * Если используется аллокатор TcMalloc
	 */
	#if __AWH_USE_TCMALLOC__
		// Печатаем разделители
		cout << "*************** START ***************" << endl << endl << flush;
		// Возвращаем статус занятой памяти
		MallocExtension::instance()->PrintStats();
		// Печатаем разделители
		cout << endl << "---------------- END ----------------" << endl << endl << flush;
	/**
	 * Если аллокатор TcMalloc не используется
	 */
	#else
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, "Memory operations only work in the release build");
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory operations only work in the release build");
		#endif
	#endif
}
/**
 * @brief Метод очистки выделенной памяти
 *
 */
void awh::Operating_System::releaseFreeMemory() const noexcept {
	/**
	 * Если используется аллокатор TcMalloc
	 */
	#if __AWH_USE_TCMALLOC__
		// Выполняем сброс памяти
		MallocExtension::instance()->ReleaseFreeMemory();
	/**
	 * Если используется аллокатор Glibc
	 */
	#elif __GLIBC__
		// Выполняем сброс памяти
		::malloc_trim(0);
	/**
	 * Операционной системой является macOS
	 */
	#elif __APPLE__ || __MACH__
		/**
		 * macOS: нет malloc_trim, но можно использовать malloc_zone.
		 * В новых версиях достаточно madvise, но явного API нет.
		 * Альтернатива: malloc_zone_pressure_relief (доступна в macOS 11+)
		 */
		#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 110000
			// Выполняем сброс памяти
			::malloc_zone_pressure_relief(nullptr, 0);
		#endif
	/**
	 * Если операционной системой является FreeBSD, NetBSD и OpenBSD
	 */
	#elif __FreeBSD__ || __NetBSD__ || __OpenBSD__
		/**
		 * BSD: на FreeBSD стандартный аллокатор — jemalloc, принудительный возврат памяти системе выполняется через mallctl
		 */
		#if __FreeBSD__ && JEMALLOC
			// Объявляем функцию управления аллокатором jemalloc
			extern "C" int32_t mallctl(const char *, void *, size_t *, void *, size_t);
			/**
			 * Принудительно очищаем все арены аллокатора (MALLCTL_ARENAS_ALL == 4096), возвращая память системе
			 */
			::mallctl("arena.4096.purge", nullptr, nullptr, nullptr, 0);
		#endif
	/**
	 * Для операционной системы MS Windows
	 */
	#elif _WIN32 || _WIN64
		/**
		 * Если активирован компилятор MS Visual Studio
		 */
		#if _MSC_VER
			// Выполняем сброс памяти
			::_heapmin();
		/**
		 * Если активирован компилятор MinGW
		 */
		#elif __MINGW32__ || __MINGW64__
			/**
			 * MinGW: нет стандартного trim, но можно попробовать
			 */
			#if _UCRT || __MSVCRT_VERSION__
				// Экспортируем нужную нам функцию
				extern "C" int32_t _heap_trim(size_t);
				// Выполняем сброс памяти
				_heap_trim(0);
			#endif
		#endif
	#endif
}
/**
 * @brief Метод резервирования нужного размера памяти для всего приложения
 *
 * @param size размер резервированной памяти
 * @return     результат выполнения операции
 */
bool awh::Operating_System::warmup(const size_t size) const noexcept {
	// Если размер резервированной памяти передан
	if(size > 0){
		// Выделяем память
		void * p = ::malloc(size);
		// Если память не выделена
		if(p == nullptr)
			// Возвращаем отрицательный результат
			return false;
		/**
		 * Если операционной системой является Linux
		 */
		#ifdef __linux__
			/**
			 * Закрепляем память в RAM (предотвращает swap)
			 */
			::madvise(p, size, MADV_WILLNEED);
		#endif
		// Освобождаем — tcmalloc/jemalloc сохранят регион в пуле
		::free(p);
		// Возвращаем положительный результат
		return true;
	}
	// Возвращаем отрицательный результат
	return false;
}
/**
 * @brief Метод блокировки возвращения оперативной памяти системе
 *
 * @param mode флаг активации/деактивации
 * @return     результат выполнения операции
 */
bool awh::Operating_System::disableReturnMemory([[maybe_unused]] const bool mode) const noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Если используется аллокатор TcMalloc
	 */
	#if __AWH_USE_TCMALLOC__
		// Если необходимо блокировать возвращение оперативной памяти системе
		if(mode){
			// Запрещаем возвращение памяти
			MallocExtension::instance()->SetNumericProperty("tcmalloc.release_rate", 0.);
			// Отключаем фоновую очистку
			MallocExtension::instance()->SetNumericProperty("tcmalloc.background_release_rate", 0.);
			// Отключить автоматическое возвращение памяти
			MallocExtension::instance()->SetNumericProperty("tcmalloc.decommit_time_ms", 1000000000);
			{
				// Значение установленное времени
				size_t value = 0;
				// Извлекаем установленный параметр
				MallocExtension::instance()->GetNumericProperty("tcmalloc.decommit_time_ms", &value);
				// Если параметр установлен корректно
				if((result = (value == 1000000000))){
					// Значение установленного множителя
					double rate = 0.;
					// Выполняем извлечение установленного параметра
					MallocExtension::instance()->GetNumericProperty("tcmalloc.release_rate", &rate);
					// Если множитель установлен верно
					if((result = (rate == 0.))){
						// Выполняем извлечение установленного параметра
						MallocExtension::instance()->GetNumericProperty("tcmalloc.background_release_rate", &rate);
						// Выполняем формирование результата
						result = (rate == 0.);
					}
				}
			}
		// Если необходимо разблокировать возвращение памяти системе
		} else {
			// Разрешаем возвращение памяти
			MallocExtension::instance()->SetNumericProperty("tcmalloc.release_rate", 1.);
			// Включаем фоновую очистку
			MallocExtension::instance()->SetNumericProperty("tcmalloc.background_release_rate", 1.);
			// Включаем автоматическое возвращение памяти
			MallocExtension::instance()->SetNumericProperty("tcmalloc.decommit_time_ms", 10000);
			{
				// Значение установленное времени
				size_t value = 0;
				// Извлекаем установленный параметр
				MallocExtension::instance()->GetNumericProperty("tcmalloc.decommit_time_ms", &value);
				// Если параметр установлен корректно
				if((result = (value == 10000))){
					// Значение установленного множителя
					double rate = 0.;
					// Выполняем извлечение установленного параметра
					MallocExtension::instance()->GetNumericProperty("tcmalloc.release_rate", &rate);
					// Если множитель установлен верно
					if((result = (rate == 1.))){
						// Выполняем извлечение установленного параметра
						MallocExtension::instance()->GetNumericProperty("tcmalloc.background_release_rate", &rate);
						// Выполняем формирование результата
						result = (rate == 1.);
					}
				}
			}
		}
	/**
	 * Если аллокатор TcMalloc не используется
	 */
	#else
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(mode), log_t::flag_t::CRITICAL, "Memory operations only work in the release build");
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory operations only work in the release build");
		#endif
	#endif
	// Возвращаем результат
	return result;
}
/**
 * Для операционной системы не являющейся MS Windows
 */
#if !_WIN32 && !_WIN64
	/**
	 * @brief Метод получения идентификатора текущего пользователя
	 *
	 * @return идентификатор текущего пользователя
	 */
	uid_t awh::Operating_System::user() const noexcept {
		// Возвращаем идентификатор текущего пользователя
		return ::geteuid();
	}
	/**
	 * @brief Метод получения группы текущего пользователя
	 *
	 * @return идентификатор группы текущего пользователя
	 */
	gid_t awh::Operating_System::group() const noexcept {
		// Возвращаем идентификатор группы пользователя
		return ::getegid();
	}
	/**
	 * @brief Метод получения списка групп текущего пользователя
	 *
	 * @return список групп текущего пользователя
	 */
	vector <gid_t> awh::Operating_System::groups() const noexcept {
		// Переменная результата
		vector <gid_t> result;
		// Буфер данных для извлечения данных
		vector <char> buffer;
		// Объект параметров пользователя
		struct passwd pwd{};
		// Извлечённые данные пользователя
		struct passwd * data = nullptr;
		// Выполняем извлечение данных пользователя
		if(::getUserById(::geteuid(), pwd, buffer, &data)){
			// Количество групп пользователя
			int32_t count = 0;
			// Первый вызов: нам необходимо определить количество групп пользователя
			::getgrouplist(pwd.pw_name, pwd.pw_gid, nullptr, &count);
			// Если количество групп пользователя получено
			if(count > 0){
				// Выделяем память для списка групп пользователя
				result.resize(static_cast <size_t> (count));
				// Второй вызов: пробуем получить список групп пользователя
				if(::getgrouplist(pwd.pw_name, pwd.pw_gid, reinterpret_cast <grouplist_t *> (result.data()), &count) == -1){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
					#endif
					// Очищаем список групп пользователя
					result.clear();
				// Корректируем размер списка под фактическое количество групп
				} else result.resize(static_cast <size_t> (count));
			}
		// Если данные пользователя не извлечены
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
			#endif
		}
		// Возвращаем результат
		return result;
	}
	/**
	 * @brief Метод получения имени пользователя по его идентификатору
	 *
	 * @param uid идентификатор пользователя
	 * @return    имя запрашиваемого пользователя
	 */
	string awh::Operating_System::user(const uid_t uid) const noexcept {
		// Буфер данных для извлечения данных
		vector <char> buffer;
		// Объект параметров пользователя
		struct passwd pwd{};
		// Извлечённые данные пользователя
		struct passwd * data = nullptr;
		// Выполняем извлечение данных пользователя
		if(::getUserById(uid, pwd, buffer, &data))
			// Печатаем имя пользователя
			return string(pwd.pw_name);
		// Если данные пользователя не извлечены
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(uid), log_t::flag_t::CRITICAL, ::strerror(errno));
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
			#endif
		}
		// Возвращаем результат
		return "";
	}
	/**
	 * @brief Метод получения группы пользователя по её идентификатору
	 *
	 * @param gid идентификатор группы пользователя
	 * @return    название группы пользователя
	 */
	string awh::Operating_System::group(const gid_t gid) const noexcept {
		// Буфер данных для извлечения данных
		vector <char> buffer;
		// Объект параметров группы пользователя
		struct group grp{};
		// Извлечённые данные группы пользователя
		struct group * data = nullptr;
		// Выполняем извлечение данных группы пользователя
		if(::getGroupById(gid, grp, buffer, &data))
			// Печатаем имя пользователя
			return string(grp.gr_name);
		// Если данные пользователя не извлечены
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(gid), log_t::flag_t::CRITICAL, ::strerror(errno));
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
			#endif
		}
		// Возвращаем результат
		return "";
	}
	/**
	 * @brief Метод получения идентификатора группы пользователя
	 *
	 * @param name название группы пользователя
	 * @return     идентификатор группы пользователя
	 */
	gid_t awh::Operating_System::group(string_view name) const noexcept {
		// Если название группы пользователя передано
		if(!name.empty()){
			// Буфер данных для извлечения данных
			vector <char> buffer;
			// Объект параметров группы пользователя
			struct group grp{};
			// Извлечённые данные группы пользователя
			struct group * data = nullptr;
			// Формируем нуль-терминированное название группы пользователя
			const string groupname(name);
			// Выполняем извлечение данных группы пользователя
			if(::getGroupByName(groupname.c_str(), grp, buffer, &data))
				// Возвращаем идентификатор группы пользователя
				return grp.gr_gid;
			// Если данные группы пользователя не извлечены
			else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
			}
		}
		// Возвращаем результат
		return 0;
	}
	/**
	 * @brief Метод вывода идентификатора пользователя
	 *
	 * @param name имя пользователя
	 * @return     полученный идентификатор пользователя
	 */
	uid_t awh::Operating_System::uid(string_view name) const noexcept {
		// Если имя пользователя передано
		if(!name.empty()){
			// Буфер данных для извлечения данных
			vector <char> buffer;
			// Объект параметров пользователя
			struct passwd pwd{};
			// Извлечённые данные пользователя
			struct passwd * data = nullptr;
			// Формируем нуль-терминированное имя пользователя
			const string username(name);
			// Выполняем извлечение данных пользователя
			if(::getUserByName(username.c_str(), pwd, buffer, &data))
				// Возвращаем идентификатор пользователя
				return pwd.pw_uid;
			// Если данные пользователя не извлечены
			else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
			}
		}
		// Возвращаем результат
		return 0;
	}
	/**
	 * @brief Метод вывода идентификатора группы пользователя
	 *
	 * @param name имя пользователя
	 * @return     полученный идентификатор группы пользователя
	 */
	gid_t awh::Operating_System::gid(string_view name) const noexcept {
		// Если имя пользователя передано
		if(!name.empty()){
			// Буфер данных для извлечения данных
			vector <char> buffer;
			// Объект параметров пользователя
			struct passwd pwd{};
			// Извлечённые данные пользователя
			struct passwd * data = nullptr;
			// Формируем нуль-терминированное имя пользователя
			const string username(name);
			// Выполняем извлечение данных пользователя
			if(::getUserByName(username.c_str(), pwd, buffer, &data))
				// Возвращаем идентификатор группы пользователя
				return pwd.pw_gid;
			// Если данные пользователя не извлечены
			else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
			}
		}
		// Возвращаем результат
		return 0;
	}
	/**
	 * @brief Получение списка групп пользователя
	 *
	 * @param user имя пользователя чьи группы следует получить
	 * @return     список групп пользователя
	 */
	vector <gid_t> awh::Operating_System::groups(string_view user) const noexcept {
		// Переменная результата
		vector <gid_t> result;
		// Если имя пользователя передано
		if(!user.empty()){
			// Буфер данных для извлечения данных
			vector <char> buffer;
			// Объект параметров пользователя
			struct passwd pwd{};
			// Извлечённые данные пользователя
			struct passwd * data = nullptr;
			// Формируем нуль-терминированное имя пользователя
			const string username(user);
			// Выполняем извлечение данных пользователя
			if(::getUserByName(username.c_str(), pwd, buffer, &data)){
				// Количество групп пользователя
				int32_t count = 0;
				// Первый вызов: нам необходимо определить количество групп пользователя
				::getgrouplist(pwd.pw_name, pwd.pw_gid, nullptr, &count);
				// Если количество групп пользователя получено
				if(count > 0){
					// Выделяем память для списка групп пользователя
					result.resize(static_cast <size_t> (count));
					// Второй вызов: пробуем получить список групп пользователя
					if(::getgrouplist(pwd.pw_name, pwd.pw_gid, reinterpret_cast <grouplist_t *> (result.data()), &count) == -1){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(user), log_t::flag_t::CRITICAL, ::strerror(errno));
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
						#endif
						// Очищаем список групп пользователя
						result.clear();
					// Корректируем размер списка под фактическое количество групп
					} else result.resize(static_cast <size_t> (count));
				}
			// Если данные пользователя не извлечены
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(user), log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
			}
		}
		// Возвращаем результат
		return result;
	}
	/**
	 * @brief Метод запуска приложения от имени указанного пользователя
	 *
	 * @param uid идентификатор пользователя
	 * @return    результат выполнения операции
	 */
	bool awh::Operating_System::chown(const uid_t uid) const noexcept {
		// Если идентификатор пользователя успешно установлен
		if(::setuid(uid) == 0)
			// Возвращаем положительный результат
			return true;
		// Если идентификатор пользователя не установлен
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(uid), log_t::flag_t::CRITICAL, ::strerror(errno));
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
			#endif
		}
		// Возвращаем результат
		return false;
	}
	/**
	 * @brief Метод запуска приложения от имени указанного пользователя
	 *
	 * @param uid идентификатор пользователя
	 * @param gid идентификатор группы пользователя
	 * @return    результат выполнения операции
	 */
	bool awh::Operating_System::chown(const uid_t uid, const gid_t gid) const noexcept {
		// Если идентификатор пользователя успешно установлен
		if(::setuid(uid) == 0){
			// Если идентификатор групы пользователя успешно установлен
			if(::setgid(gid) == 0)
				// Возвращаем положительный результат
				return true;
			// Если идентификатор группы пользователя не установлен
			else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(uid, gid), log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
			}
			// Возвращаем отрицательный результат
			return false;
		// Если идентификатор пользователя не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(uid, gid), log_t::flag_t::CRITICAL, ::strerror(errno));
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
			#endif
		}
		// Возвращаем результат
		return false;
	}
	/**
	 * @brief Метод запуска приложения от имени указанного пользователя
	 *
	 * @param user  название пользователя
	 * @param group название группы пользователя
	 * @return      результат выполнения операции
	 */
	bool awh::Operating_System::chown(string_view user, string_view group) const noexcept {
		// Если имя пользователя и название группы пользователя переданы
		if(!user.empty() && !group.empty()){
			// Буфер данных для извлечения данных пользователя
			vector <char> buffer;
			// Объект параметров пользователя
			struct passwd pwd{};
			// Извлечённые данные пользователя
			struct passwd * data = nullptr;
			// Формируем нуль-терминированное имя пользователя
			const string username(user);
			// Выполняем извлечение данных пользователя
			if(::getUserByName(username.c_str(), pwd, buffer, &data)){
				// Сохраняем идентификатор пользователя до повторного использования буфера
				const uid_t uid = pwd.pw_uid;
				// Буфер данных для извлечения данных группы пользователя
				vector <char> bufferGroup;
				// Объект параметров группы пользователя
				struct group grp{};
				// Извлечённые данные группы пользователя
				struct group * dataGroup = nullptr;
				// Формируем нуль-терминированное название группы пользователя
				const string groupname(group);
				// Выполняем извлечение данных группы пользователя
				if(::getGroupByName(groupname.c_str(), grp, bufferGroup, &dataGroup))
					// Возвращаем установку права доступа
					return this->chown(uid, grp.gr_gid);
				// Если данные группы пользователя не извлечены
				else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(user, group), log_t::flag_t::CRITICAL, ::strerror(errno));
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
					#endif
				}
			// Если данные пользователя не извлечены
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(user, group), log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
			}
		}
		// Возвращаем результат
		return false;
	}
/**
 * Для операционной системы MS Windows
 */
#else
	/**
	 * @brief Метод получения идентификатора текущего пользователя
	 *
	 * @return идентификатор текущего пользователя
	 */
	wstring awh::Operating_System::user() const noexcept {
		// Переменная результата
		wstring result = L"";
		// Токен текущего процесса
		HANDLE token = nullptr;
		// Открываем токен текущего процесса
		if(!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &token)){
			// Если мы получили ошибку
			if(::GetLastError() != 0){
				// Создаём буфер сообщения ошибки
				wchar_t message[0xFF] = {0};
				// Выполняем формирование текста ошибки
				::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 0xFF, 0);
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::convert(message).c_str());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::convert(message).c_str());
				#endif
			}
			// Возвращаем результат
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
				wchar_t message[0xFF] = {0};
				// Выполняем формирование текста ошибки
				::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 0xFF, 0);
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::convert(message).c_str());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::convert(message).c_str());
				#endif
			}
			// Закрываем токен
			::CloseHandle(token);
			// Возвращаем результат
			return result;
		}
		// Выделяем память под токен пользователя
		tokenUser = (PTOKEN_USER) ::LocalAlloc(LPTR, size);
		// Если память не может быть выделена
		if(tokenUser == nullptr) {
			// Если мы получили ошибку
			if(::GetLastError() != 0){
				// Создаём буфер сообщения ошибки
				wchar_t message[0xFF] = {0};
				// Выполняем формирование текста ошибки
				::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 0xFF, 0);
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::convert(message).c_str());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::convert(message).c_str());
				#endif
			}
			// Закрываем токен
			::CloseHandle(token);
			// Возвращаем результат
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
		// Возвращаем результат
		return result;
	}
	/**
	 * @brief Метод получения списка групп текущего пользователя
	 *
	 * @return список групп текущего пользователя
	 */
	vector <wstring> awh::Operating_System::groups() const noexcept {
		// Переменная результата
		vector <wstring> result;
		// Токен текущего процесса
		HANDLE token = nullptr;
		// Открываем токен текущего процесса
		if(!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &token)){
			// Если мы получили ошибку
			if(::GetLastError() != 0){
				// Создаём буфер сообщения ошибки
				wchar_t message[0xFF] = {0};
				// Выполняем формирование текста ошибки
				::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 0xFF, 0);
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::convert(message).c_str());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::convert(message).c_str());
				#endif
			}
			// Возвращаем результат
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
				wchar_t message[0xFF] = {0};
				// Выполняем формирование текста ошибки
				::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 0xFF, 0);
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::convert(message).c_str());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::convert(message).c_str());
				#endif
			}
			// Закрываем токен
			::CloseHandle(token);
			// Возвращаем результат
			return result;
		}
		// Выделяем память под токен группы
		tokenGroups = (PTOKEN_GROUPS) ::LocalAlloc(LPTR, size);
		// Если память не может быть выделена
		if(tokenGroups == nullptr){
			// Если мы получили ошибку
			if(::GetLastError() != 0){
				// Создаём буфер сообщения ошибки
				wchar_t message[0xFF] = {0};
				// Выполняем формирование текста ошибки
				::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 0xFF, 0);
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::convert(message).c_str());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::convert(message).c_str());
				#endif
			}
			// Закрываем токен
			::CloseHandle(token);
			// Возвращаем результат
			return result;
		}
		// Сначала получаем размер буфера
		if(::GetTokenInformation(token, TokenGroups, tokenGroups, size, &size)){
			/**
			 * Выполняем перебор всех групп пользователя
			 */
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
		// Возвращаем результат
		return result;
	}
	/**
	 * @brief Метод получения названия пользователя/группы по идентификатору
	 *
	 * @param sid идентификатор пользователя/группы
	 * @return    имя запрашиваемого пользователя/группы
	 */
	string awh::Operating_System::account(wstring_view sid) const noexcept {
		// Переменная результата
		string result = "";
		// Если идентификатор пользователя передан
		if(!sid.empty()){
			// Объект идентификатора пользователя
			PSID pSid = nullptr;
			// Выполняем идентификатор пользователя
			if(::ConvertStringSidToSidW(sid.data(), &pSid)){
				// Тип SID-а
				SID_NAME_USE sidType;
				// Размер имени пользователя и домена пользователя
				DWORD nameSize = 0, domainSize = 0;
				// Сначала вызываем для получения размеров буферов
				::LookupAccountSidW(nullptr, pSid, nullptr, &nameSize, nullptr, &domainSize, &sidType);
				// Если мы получиши ошибку извлечения размеров буфера
				if(::GetLastError() != ERROR_INSUFFICIENT_BUFFER){
					// Создаём буфер сообщения ошибки
					wchar_t message[0xFF] = {0};
					// Выполняем формирование текста ошибки
					::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 0xFF, 0);
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(::convert(sid)), log_t::flag_t::CRITICAL, ::convert(message).c_str());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::convert(message).c_str());
					#endif
					// Возвращаем результат
					return result;
				}
				// Инициализируем имя пользователя
				wstring name(nameSize, L'\0');
				// Инициализируем доменное имя пользователя
				wstring domain(domainSize, L'\0');
				// Извлекаем имя пользователя и его доменное имя
				if(::LookupAccountSidW(nullptr, pSid, &name[0], &nameSize, &domain[0], &domainSize, &sidType)){
					// Усекаем буферы до фактической длины (второй вызов возвращает длину без нуль-терминатора)
					name.resize(nameSize);
					// Усекаем доменное имя до фактической длины
					domain.resize(domainSize);
					// Если доменное имя установлено
					if(!domain.empty() && (domain[0] != '\0')){
						/**
						 * Формат: "DOMAIN\Username" или просто "Username" для локальных учетных записей
						 */
						result = ::move(::convert(domain) + "\\" + ::convert(name));
					// Если доменное имя не получено
					} else result = ::move(::convert(name));
				}
				// Освобождаем память выделенную под идентификатор пользователя
				::LocalFree(pSid);
			// Если конвертация не выполнена
			} else {
				// Если мы получили ошибку
				if(::GetLastError() != 0){
					// Создаём буфер сообщения ошибки
					wchar_t message[0xFF] = {0};
					// Выполняем формирование текста ошибки
					::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 0xFF, 0);
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(::convert(sid)), log_t::flag_t::CRITICAL, ::convert(message).c_str());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::convert(message).c_str());
					#endif
				}
			}
		}
		// Возвращаем результат
		return result;
	}
	/**
	 * @brief Метод вывода идентификатора пользователя/группы
	 *
	 * @param name название пользователя/группы
	 * @return     полученный идентификатор пользователя/группы
	 */
	wstring awh::Operating_System::account(string_view name) const noexcept {
		// Переменная результата
		wstring result = L"";
		// Если имя пользователя/группы передано
		if(!name.empty()){
			// Тип SID-а
			SID_NAME_USE sidType;
			// Размер SID-а пользователя/группы и домена пользователя
			DWORD sidSize = 0, domainSize = 0;
			// Выполняем конвертирование название пользователя/группы
			wstring account = ::move(::convert(name)), actualDomain = L"";
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
				wchar_t message[0xFF] = {0};
				// Выполняем формирование текста ошибки
				::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 0xFF, 0);
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::CRITICAL, ::convert(message).c_str());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::convert(message).c_str());
				#endif
				// Возвращаем результат
				return result;
			}
			// Инициализируем доменное имя пользователя
			wstring domain(domainSize, L'\0');
			// Выделяем память под SID и домен
			PSID pSid = (PSID) ::LocalAlloc(LPTR, sidSize);
			// Извлекаем SID пользователя/группы и его доменное имя
			if(::LookupAccountNameW(nullptr, account.c_str(), pSid, &sidSize, &domain[0], &domainSize, &sidType)){
				// Усекаем доменное имя до фактической длины (второй вызов возвращает длину без нуль-терминатора)
				domain.resize(domainSize);
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
		// Возвращаем результат
		return result;
	}
	/**
	 * @brief Получение списка групп пользователя
	 *
	 * @param user имя пользователя чьи группы следует получить
	 * @return     список групп пользователя
	 */
	vector <wstring> awh::Operating_System::groups(string_view user) const noexcept {
		// Переменная результата
		vector <wstring> result;
		// Если имя пользователя передано
		if(!user.empty()){
			// Тип SID-а
			SID_NAME_USE sidType;
			// Размер SID-а пользователя и домена пользователя
			DWORD sidSize = 0, domainSize = 0;
			// Выполняем конвертирование название пользователя/группы
			const wstring account = ::move(::convert(user));
			// Первый вызов — получаем размеры буферов
			::LookupAccountNameW(nullptr, account.c_str(), nullptr, &sidSize, nullptr, &domainSize, &sidType);
			// Если мы получиши ошибку извлечения размеров буфера
			if(::GetLastError() != ERROR_INSUFFICIENT_BUFFER){
				// Создаём буфер сообщения ошибки
				wchar_t message[0xFF] = {0};
				// Выполняем формирование текста ошибки
				::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 0xFF, 0);
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(user), log_t::flag_t::CRITICAL, ::convert(message).c_str());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::convert(message).c_str());
				#endif
				// Возвращаем результат
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
				// Возвращаем пустой результат
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
					wchar_t message[0xFF] = {0};
					// Выполняем формирование текста ошибки
					::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 0xFF, 0);
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(user), log_t::flag_t::CRITICAL, ::convert(message).c_str());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::convert(message).c_str());
					#endif
				}
				// Освобождаем ресурсы
				::LocalFree(pSid);
				// Возвращаем пустой результат
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
					wchar_t message[0xFF] = {0};
					// Выполняем формирование текста ошибки
					::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 0xFF, 0);
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(user), log_t::flag_t::CRITICAL, ::convert(message).c_str());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::convert(message).c_str());
					#endif
				}
				// Закрываем токен
				::CloseHandle(token);
				// Освобождаем ресурсы
				::LocalFree(pSid);
				// Возвращаем результат
				return result;
			}
			// Выделяем память под токен группы
			PTOKEN_GROUPS tokenGroups = (PTOKEN_GROUPS) ::LocalAlloc(LPTR, size);
			// Если память не может быть выделена
			if(tokenGroups == nullptr){
				// Если мы получили ошибку
				if(::GetLastError() != 0){
					// Создаём буфер сообщения ошибки
					wchar_t message[0xFF] = {0};
					// Выполняем формирование текста ошибки
					::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 0xFF, 0);
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(user), log_t::flag_t::CRITICAL, ::convert(message).c_str());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::convert(message).c_str());
					#endif
				}
				// Закрываем токен
				::CloseHandle(token);
				// Освобождаем ресурсы
				::LocalFree(pSid);
				// Возвращаем результат
				return result;
			}
			// Сначала получаем размер буфера
			if(::GetTokenInformation(token, TokenGroups, tokenGroups, size, &size)){
				/**
				 * Выполняем перебор всех групп пользователя
				 */
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
		// Возвращаем результат
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
	T awh::Operating_System::sysctl(string_view name) const noexcept {
		// Переменная результата (для скалярных типов выполняется нулевая инициализация)
		T result{};
		// Если название записи передано правильно
		if(!name.empty()){
			// Создаём буфер данных для извлечения данных
			vector <char> buffer;
			// Выполняем извлечение данных записи
			::sysctl(name, buffer);
			// Если данные буфера были извлечены удачно
			if(!buffer.empty())
				// Выполняем получение данных в соответствии с типом результата
				::metadata(buffer, result);
		}
		// Возвращаем результат
		return result;
	}
	/**
	 * Объявляем прототипы для извлечения значений настроек ядра операционной системы
	 */
	template int8_t awh::Operating_System::sysctl <int8_t> (string_view) const noexcept;
	template uint8_t awh::Operating_System::sysctl <uint8_t> (string_view) const noexcept;
	template int16_t awh::Operating_System::sysctl <int16_t> (string_view) const noexcept;
	template uint16_t awh::Operating_System::sysctl <uint16_t> (string_view) const noexcept;
	template int32_t awh::Operating_System::sysctl <int32_t> (string_view) const noexcept;
	template uint32_t awh::Operating_System::sysctl <uint32_t> (string_view) const noexcept;
	template int64_t awh::Operating_System::sysctl <int64_t> (string_view) const noexcept;
	template uint64_t awh::Operating_System::sysctl <uint64_t> (string_view) const noexcept;
	template float awh::Operating_System::sysctl <float> (string_view) const noexcept;
	template double awh::Operating_System::sysctl <double> (string_view) const noexcept;
	template string awh::Operating_System::sysctl <string> (string_view) const noexcept;
	/**
	 * Если операционной системой является macOS или Linux
	 */
	#if __APPLE__ || __MACH__ || __linux__
		template size_t awh::Operating_System::sysctl <size_t> (string_view) const noexcept;
		template ssize_t awh::Operating_System::sysctl <ssize_t> (string_view) const noexcept;
	#endif
	/**
	 * Объявляем прототипы для извлечения списка значений настроек ядра операционной системы
	 */
	template vector <int8_t> awh::Operating_System::sysctl <vector <int8_t>> (string_view) const noexcept;
	template vector <uint8_t> awh::Operating_System::sysctl <vector <uint8_t>> (string_view) const noexcept;
	template vector <int16_t> awh::Operating_System::sysctl <vector <int16_t>> (string_view) const noexcept;
	template vector <uint16_t> awh::Operating_System::sysctl <vector <uint16_t>> (string_view) const noexcept;
	template vector <int32_t> awh::Operating_System::sysctl <vector <int32_t>> (string_view) const noexcept;
	template vector <uint32_t> awh::Operating_System::sysctl <vector <uint32_t>> (string_view) const noexcept;
	template vector <int64_t> awh::Operating_System::sysctl <vector <int64_t>> (string_view) const noexcept;
	template vector <uint64_t> awh::Operating_System::sysctl <vector <uint64_t>> (string_view) const noexcept;
	template vector <float> awh::Operating_System::sysctl <vector <float>> (string_view) const noexcept;
	template vector <double> awh::Operating_System::sysctl <vector <double>> (string_view) const noexcept;
	template vector <string> awh::Operating_System::sysctl <vector <string>> (string_view) const noexcept;
	/**
	 * Если операционной системой является macOS или Linux
	 */
	#if __APPLE__ || __MACH__ || __linux__
		template vector <size_t> awh::Operating_System::sysctl <vector <size_t>> (string_view) const noexcept;
		template vector <ssize_t> awh::Operating_System::sysctl <vector <ssize_t>> (string_view) const noexcept;
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
	bool awh::Operating_System::sysctl(string_view name, const T value) const noexcept {
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
				::memcpy(&buffer[0], &value, sizeof(value));
				// Выполняем установку буфера бинарных данных
				return ::sysctl(name, &buffer[0], buffer.size());
			#endif
		}
		// Сообщаем, что ничего не установленно
		return false;
	}
	/**
	 * Объявляем прототипы для установки значений настроек ядра операционной системы
	 */
	template bool awh::Operating_System::sysctl <int8_t> (string_view, const int8_t) const noexcept;
	template bool awh::Operating_System::sysctl <uint8_t> (string_view, const uint8_t) const noexcept;
	template bool awh::Operating_System::sysctl <int16_t> (string_view, const int16_t) const noexcept;
	template bool awh::Operating_System::sysctl <uint16_t> (string_view, const uint16_t) const noexcept;
	template bool awh::Operating_System::sysctl <int32_t> (string_view, const int32_t) const noexcept;
	template bool awh::Operating_System::sysctl <uint32_t> (string_view, const uint32_t) const noexcept;
	template bool awh::Operating_System::sysctl <int64_t> (string_view, const int64_t) const noexcept;
	template bool awh::Operating_System::sysctl <uint64_t> (string_view, const uint64_t) const noexcept;
	template bool awh::Operating_System::sysctl <float> (string_view, const float) const noexcept;
	template bool awh::Operating_System::sysctl <double> (string_view, const double) const noexcept;
	/**
	 * Если операционной системой является macOS или Linux
	 */
	#if __APPLE__ || __MACH__ || __linux__
		template bool awh::Operating_System::sysctl <size_t> (string_view, const size_t) const noexcept;
		template bool awh::Operating_System::sysctl <ssize_t> (string_view, const ssize_t) const noexcept;
	#endif
	/**
	 * @brief Метод установки настроек ядра операционной системы
	 *
	 * @param name  название записи для установки настроек
	 * @param value значение записи для установки настроек
	 * @return      результат выполнения установки
	 */
	bool awh::Operating_System::sysctl(string_view name, string_view value) const noexcept {
		// Если название записи для установки настроек передано
		if(!name.empty())
			// Выполняем установку буфера бинарных данных
			return ::sysctl(name, value.data(), value.size());
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
	bool awh::Operating_System::sysctl(string_view name, const char * value) const noexcept {
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
	bool awh::Operating_System::sysctl(string_view name, const vector <T> & items) const noexcept {
		// Если название записи для установки настроек передано
		if(!name.empty() && (is_integral <T>::value || is_floating_point <T>::value)){
			/**
			 * Если это Linux
			 */
			#if __linux__
				// Выполняем преобразование числа в строку
				string param = "";
				/**
				 * Выполняем перебор всего списка параметров
				 */
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
				/**
				 * Выполняем перебор всего списка параметров
				 */
				for(auto & item : items){
					// Выполняем установку результата по умолчанию
					::memcpy(&buffer[0] + offset, &item, sizeof(item));
					// Выполняем увеличение смещения в буфере
					offset += sizeof(item);
				}
				// Выполняем установку буфера бинарных данных
				return ::sysctl(name, &buffer[0], buffer.size());
			#endif
		}
		// Сообщаем, что ничего не установленно
		return false;
	}
	/**
	 * Объявляем прототипы для установки списка значений настроек ядра операционной системы
	 */
	template bool awh::Operating_System::sysctl <int8_t> (string_view, const vector <int8_t> &) const noexcept;
	template bool awh::Operating_System::sysctl <uint8_t> (string_view, const vector <uint8_t> &) const noexcept;
	template bool awh::Operating_System::sysctl <int16_t> (string_view, const vector <int16_t> &) const noexcept;
	template bool awh::Operating_System::sysctl <uint16_t> (string_view, const vector <uint16_t> &) const noexcept;
	template bool awh::Operating_System::sysctl <int32_t> (string_view, const vector <int32_t> &) const noexcept;
	template bool awh::Operating_System::sysctl <uint32_t> (string_view, const vector <uint32_t> &) const noexcept;
	template bool awh::Operating_System::sysctl <int64_t> (string_view, const vector <int64_t> &) const noexcept;
	template bool awh::Operating_System::sysctl <uint64_t> (string_view, const vector <uint64_t> &) const noexcept;
	template bool awh::Operating_System::sysctl <float> (string_view, const vector <float> &) const noexcept;
	template bool awh::Operating_System::sysctl <double> (string_view, const vector <double> &) const noexcept;
	/**
	 * Если операционной системой является macOS или Linux
	 */
	#if __APPLE__ || __MACH__ || __linux__
		template bool awh::Operating_System::sysctl <size_t> (string_view, const vector <size_t> &) const noexcept;
		template bool awh::Operating_System::sysctl <ssize_t> (string_view, const vector <ssize_t> &) const noexcept;
	#endif
	/**
	 * @brief Метод установки настроек ядра операционной системы
	 *
	 * @param name  название записи для установки настроек
	 * @param items значение записи для установки настроек
	 * @return      результат выполнения установки
	 */
	bool awh::Operating_System::sysctl(string_view name, const vector <string> & items) const noexcept {
		// Если название записи для установки настроек передано
		if(!name.empty()){
			// Выполняем преобразование числа в строку
			string param = "";
			/**
			 * Выполняем перебор всего списка параметров
			 */
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
	bool awh::Operating_System::sysctl(string_view name, const vector <const char *> & items) const noexcept {
		// Если название записи для установки настроек передано
		if(!name.empty()){
			// Выполняем преобразование числа в строку
			string param = "";
			/**
			 * Выполняем перебор всего списка параметров
			 */
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
string awh::Operating_System::exec(string_view cmd, const bool multiline) const noexcept {
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
			// Создаем пайп для чтения результата работы Operating_System
			FILE * stream = ::popen(cmd.data(), "r");
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
			// Создаем пайп для чтения результата работы Operating_System
			FILE * stream = ::_wpopen(::convert(cmd).c_str(), L"rt");
			// Если пайп открыт
			if(stream){
				/**
				 * Считываем до тех пор пока все не прочитаем
				 */
				while(::fgetws(buffer, sizeof(buffer) / sizeof(buffer[0]), stream) != nullptr){
					// Выполняем конвертирование в utf-8 строку
					result.append(::move(::convert(buffer)));
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
	// Возвращаем результат
	return result;
}
