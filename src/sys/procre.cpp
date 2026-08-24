/**
 * @file procre.cpp
 * @date 2026-01-26
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
 * @brief Реализация модуля резольвера процессов — сопоставление сетевого соединения с владеющим им процессом через
 *        нативные механизмы операционной системы (procfs, sysctl, системные API)
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstdlib>
#include <cstring>
#include <iostream>

/**
 * Системный заголовочный файл
 */
#include <sys/types.h>

/**
 * Для операционной системы не являющейся MS Windows
 *
 * @note Заголовок netinet/in.h принадлежит POSIX и у MS Windows отсутствует.
 *       Соответствующие ему объявления приходят там из winsock2.h, подключаемого
 *       ниже через единую точку sys/win32.hpp
 *
 */
#if !_WIN32 && !_WIN64
	/**
	 * Системный заголовочный файл
	 */
	#include <netinet/in.h>
#endif

/**
 * Для операционной системы Linux
 */
#ifdef __linux__
	/**
	 * Стандартный заголовочный файл
	 */
	#include <map>

	/**
	 * Системные заголовочные файлы
	 */
	#include <dirent.h>
	#include <unistd.h>
	#include <sys/file.h>
	#include <sys/stat.h>
/**
 * Для операционной системы FreeBSD
 */
#elif __FreeBSD__
	/**
	 * Системные заголовочные файлы
	 */
	#include <libutil.h>
	#include <sys/un.h>
	#include <sys/user.h>
	#include <sys/sysctl.h>
	#include <sys/socket.h>
/**
 * Для операционной системы macOS
 */
#elif __APPLE__ || __MACH__
	/**
	 * Системный заголовочный файл
	 */
	#include <libproc.h>
/**
 * Для операционной системы NetBSD или OpenBSD
 */
#elif __NetBSD__ || __OpenBSD__
	/**
	 * Стандартные заголовочные файлы
	 */
	#include <memory>
	#include <vector>
	#include <fstream>
	#include <sstream>

	/**
	 * Системные заголовочные файлы
	 */
	#include <arpa/inet.h>

	/**
	 * Для операционной системы OpenBSD
	 *
	 * @note Название процесса берётся у ядра запросом: файловой системы процессов
	 *       у OpenBSD нет вовсе, и читать название неоткуда
	 *
	 */
	#if __OpenBSD__
		#include <sys/sysctl.h>
	#endif
/**
 * Реализация под Sun Solaris
 */
#elif __sun__
	/**
	 * Стандартные заголовочные файлы
	 */
	#include <fstream>
	#include <sstream>

	/**
	 * Системные заголовочные файлы
	 */
	#include <fcntl.h>
	#include <dirent.h>
	#include <unistd.h>
	#include <procfs.h>
	#include <sys/stat.h>
	#include <sys/socket.h>
	#include <sys/un.h>
	#include <netinet/in.h>
/**
 * Для операционной системы Windows
 */
#elif _WIN32 || _WIN64
	/**
	 * Подключаем единую точку подключения системных заголовков MS Windows
	 */
	#include <sys/win32.hpp>

	/**
	 * Системные заголовочные файлы
	 */
	#include <psapi.h>
	#include <iphlpapi.h>
	#include <cstdint>
#endif

/**
 * Подключаем заголовочный файл проекта
 */
#include <encoding/ascii.hpp>
#include <sys/procre.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Для операционной системы NetBSD или OpenBSD
 */
#if __NetBSD__ || __OpenBSD__
	/**
	 * @brief Инкапсулируем функции Process Resolver в пространство имён
	 *
	 */
	namespace procre {
		/**
		 * @brief Функция извлечения данных из файла
		 *
		 * @param filename путь к файлу для извлечения
		 * @param log      объект для работы с логами
		 * @return         содержимое файла
		 *
		 */
		static string read(const string & filename, const awh::log_t * log) noexcept {
			// Переменная результата
			string result = "";
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Устанавливаем путь к файлу для чтения
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
					// Записываем ошибку в лог
					log->debug("%s", __PRETTY_FUNCTION__, make_tuple(filename), awh::log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					log->print("%s", awh::log_t::flag_t::CRITICAL, error.what());
				#endif
			}
			// Возвращаем результат
			return result;
		}
	};
#endif

/**
 * @brief Конструктор
 *
 */
awh::Process_Resolver::Ports::Ports() noexcept : src(0), dst(0) {}

/**
 * @brief Конструктор
 *
 */
awh::Process_Resolver::Addresses::Addresses() noexcept : src(nullptr), dst(nullptr) {}

/**
 * @brief Конструктор
 *
 */
awh::Process_Resolver::Info::Info() noexcept :
 family(event::family_t::NONE),
 protocol(event::protocol_t::NONE) {}

/**
 * @brief Метод запуска процесса сканирования активных процессов и получения информации о них
 *
 */
void awh::Process_Resolver::scanning() noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Для операционной системы Linux
		 */
		#if __linux__
			// Сначала читаем информацию о всех сокетах из /proc/net/
			map <ino_t, info_t> socketsInfo;
			/**
			 * @brief Функция извлечения информации о сокете из файловой системы /proc/net/
			 *
			 * @param filename путь к файлу для извлечения
			 * @param proto    протокол сокета
			 * @param family   семейство адресов сокета
			 *
			 */
			auto parse = [&socketsInfo](const char * filename, const event::protocol_t proto, const event::family_t family) noexcept -> void {
				// Открываем файл для чтения
				FILE * fp = ::fopen(filename, "r");
				// Если файл не открыт
				if(fp == nullptr)
					// Выходим из функции
					return;
				// Буфер для чтения строк из файла
				char buffer[0x200];
				// Если в файле нет строк для чтения
				if(!::fgets(buffer, sizeof(buffer), fp)){
					// Закрываем файл
					::fclose(fp);
					// Выходим из функции
					return;
				}
				/**
				 * Читаем строки из файла и извлекаем информацию о сокете
				 */
				while(::fgets(buffer, sizeof(buffer), fp)){
					// Индексный дескриптор сокета
					ino_t inode = 0;
					// Объект для хранения информации о сокете
					info_t info{};
					// Устанавливаем семейство адресов сокета
					info.family = family;
					// Устанавливаем протокол сокета
					info.protocol = proto;
					/**
					 * Определяем семейство IP-адресов сокета
					 */
					switch(static_cast <uint8_t> (family)){
						// Для семейства IPv4
						case static_cast <uint8_t> (event::family_t::IPV4): {
							// Буфер для хранения IP-адреса назначения
							uint32_t target = 0;
							// Буфер для хранения IP-адреса источника
							uint32_t source = 0;
							// Буферы для хранения портов (для предотвращения переполнения стека при sscanf %x)
							uint32_t srcPort = 0, dstPort = 0;
							// Извлекаем информацию о сокете из строки
							if(::sscanf(buffer, "%*d: %x:%x %x:%x %*x %*x:%*x %*x:%*x %*x %*d %*d %lu", &source, &srcPort, &target, &dstPort, &inode) == 5){
								// Устанавливаем порты
								info.ports.src = static_cast <uint16_t> (srcPort);
								info.ports.dst = static_cast <uint16_t> (dstPort);
								// Выполняем инициализацию объекта IP-адреса источника процесса
								info.addresses.src = make_unique <net::addr_net_ipv4_t> ();
								// Выполняем инициализацию объекта IP-адреса назначения процесса
								info.addresses.dst = make_unique <net::addr_net_ipv4_t> ();
								// Устанавливаем IP-адрес источника процесса
								awh_cast <net::addr_net_ipv4_t *> (info.addresses.src.get())->address = source;
								// Устанавливаем IP-адрес назначения процесса
								awh_cast <net::addr_net_ipv4_t *> (info.addresses.dst.get())->address = target;
								// Сохраняем информацию о сокете в общем контейнере
								socketsInfo[inode] = ::move(info);
							}
						} break;
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6): {
							// Буфер для хранения IP-адреса назначения
							uint32_t target[4] = {0};
							// Буфер для хранения IP-адреса источника
							uint32_t source[4] = {0};
							// Буферы для хранения портов (для предотвращения переполнения стека при sscanf %x)
							uint32_t srcPort = 0, dstPort = 0;
							// Извлекаем информацию о сокете из строки
							if(::sscanf(buffer, "%*d: %8x%8x%8x%8x:%x %8x%8x%8x%8x:%x %*x %*x:%*x %*x:%*x %*x %*d %*d %lu",
							   &source[0], &source[1], &source[2], &source[3], &srcPort,
							   &target[0], &target[1], &target[2], &target[3], &dstPort, &inode) == 11){
								// Устанавливаем порты
								info.ports.src = static_cast <uint16_t> (srcPort);
								info.ports.dst = static_cast <uint16_t> (dstPort);
								// Выполняем инициализацию объекта IP-адреса источника процесса
								info.addresses.src = make_unique <net::addr_net_ipv6_t> ();
								// Выполняем инициализацию объекта IP-адреса назначения процесса
								info.addresses.dst = make_unique <net::addr_net_ipv6_t> ();
								// Устанавливаем IP-адрес источника процесса
								::memcpy(&awh_cast <net::addr_net_ipv6_t *> (info.addresses.src.get())->address[0], source, 16);
								// Устанавливаем IP-адрес назначения процесса
								::memcpy(&awh_cast <net::addr_net_ipv6_t *> (info.addresses.dst.get())->address[0], target, 16);
								// Сохраняем информацию о сокете в общем контейнере
								socketsInfo[inode] = ::move(info);
							}
						} break;
					}
				}
				// Закрываем файл
				::fclose(fp);
			};
			// Выполняем извлечение информации о сокете из файла /proc/net/tcp для протокола TCP и семейства IPv4
			parse("/proc/net/tcp", event::protocol_t::TCP, event::family_t::IPV4);
			// Выполняем извлечение информации о сокете из файла /proc/net/udp для протокола UDP и семейства IPv4
			parse("/proc/net/udp", event::protocol_t::UDP, event::family_t::IPV4);
			// Выполняем извлечение информации о сокете из файла /proc/net/raw для протокола RAW и семейства IPv4
			parse("/proc/net/raw", event::protocol_t::RAW, event::family_t::IPV4);
			// Выполняем извлечение информации о сокете из файла /proc/net/icmp для протокола ICMP и семейства IPv4
			parse("/proc/net/icmp", event::protocol_t::ICMP, event::family_t::IPV4);
			// Выполняем извлечение информации о сокете из файла /proc/net/igmp для протокола IGMP и семейства IPv4
			parse("/proc/net/igmp", event::protocol_t::IGMP, event::family_t::IPV4);
			// Выполняем извлечение информации о сокете из файла /proc/net/tcp6 для протокола TCP и семейства IPv6
			parse("/proc/net/tcp6", event::protocol_t::TCP, event::family_t::IPV6);
			// Выполняем извлечение информации о сокете из файла /proc/net/udp6 для протокола UDP и семейства IPv6
			parse("/proc/net/udp6", event::protocol_t::UDP, event::family_t::IPV6);
			// Выполняем извлечение информации о сокете из файла /proc/net/raw6 для протокола RAW и семейства IPv6
			parse("/proc/net/raw6", event::protocol_t::RAW, event::family_t::IPV6);
			// Выполняем извлечение информации о сокете из файла /proc/net/icmp6 для протокола ICMP и семейства IPv6
			parse("/proc/net/icmp6", event::protocol_t::ICMP, event::family_t::IPV6);
			// Выполняем извлечение информации о сокете из файла /proc/net/igmp6 для протокола IGMP и семейства IPv6
			parse("/proc/net/igmp6", event::protocol_t::IGMP, event::family_t::IPV6);
			/**
			 * Открываем файл для чтения информации о сокете из файловой системы /proc/net/unix для протокола NONE и семейства UDS
			 */
			if(FILE * fp = ::fopen("/proc/net/unix", "r")){
				// Буфер для чтения строк из файла
				char buffer[512];
				// Если в файле есть строки для чтения
				if(::fgets(buffer, sizeof(buffer), fp)){
					/**
					 * Читаем строки из файла и извлекаем информацию о сокете
					 */
					while(::fgets(buffer, sizeof(buffer), fp)){
						// Индексный дескриптор сокета
						ino_t inode = 0;
						// Буфер для хранения пути сокета
						char path[0x100] = {0};
						// Извлекаем информацию о сокете из строки
						const int32_t num = ::sscanf(buffer, "%*p: %*x %*x %*x %*x %*x %lu %255s", &inode, path);
						// Если информация о сокете извлечена успешно
						if((num >= 1) && (inode != 0)){
							// Объект для хранения информации о сокете
							info_t info{};
							// Устанавливаем семейство адресов сокета
							info.family = event::family_t::UDS;
							// Выполняем инициализацию объекта UNIX-адреса источника процесса
							info.addresses.src = make_unique <net::addr_fs_t> ();
							// Выполняем инициализацию объекта UNIX-адреса назначения процесса
							info.addresses.dst = make_unique <net::addr_fs_t> ();
							// Если путь сокета извлечён успешно
							if(num == 2)
								// Устанавливаем UNIX-адрес источника процесса
								awh_cast <net::addr_fs_t *> (info.addresses.src.get())->address = path;
							// Сохраняем информацию о сокете в общем контейнере
							socketsInfo[inode] = ::move(info);
						}
					}
				}
				// Закрываем файл
				::fclose(fp);
			}
			// Теперь читаем информацию о всех процессах из /proc и сопоставляем её с информацией о сокете
			DIR * dir = ::opendir("/proc");
			// Если каталог открыт удачно
			if(dir != nullptr){
				// Буфер для хранения информации о сокете
				struct dirent * entry = nullptr;
				/**
				 * Читаем записи из каталога и извлекаем информацию о процессе
				 */
				while((entry = ::readdir(dir)) != nullptr){
					// Если запись является каталогом и её имя начинается с цифры
					if((entry->d_type == DT_DIR) && awh::ascii::isDigit(entry->d_name[0])){
						// Получаем идентификатор процесса из имени каталога
						pid_t pid = ::atoi(entry->d_name);
						// Буфер для хранения названия приложения которому принадлежит процесс
						char path[0x100];
						// Выполняем заполнение буфера для получения названия приложения которому принадлежит процесс
						::snprintf(path, sizeof(path), "/proc/%d/fd", pid);
						// Открываем каталог для чтения информации о файловых дескрипторах процесса
						DIR * dir = ::opendir(path);
						// Если каталог открыт удачно
						if(dir != nullptr){
							// Буфер для хранения информации о сокете
							struct dirent * entry = nullptr;
							/**
							 * Читаем записи из каталога и извлекаем информацию о сокете
							 */
							while((entry = ::readdir(dir)) != nullptr){
								// Если запись является символической ссылкой или её тип неизвестен
								if((entry->d_type == DT_LNK) || (entry->d_type == DT_UNKNOWN)){
									// Буфер для хранения пути к файловому дескриптору процесса
									char path[0x200];
									// Выполняем заполнение буфера для получения пути к файловому дескриптору процесса
									::snprintf(path, sizeof(path), "/proc/%d/fd/%s", pid, entry->d_name);
									// Буфер для хранения информации о сокете
									char target[0x200];
									// Читаем символическую ссылку для получения информации о сокете
									const ssize_t length = ::readlink(path, target, sizeof(target) - 1);
									// Если информация о сокете извлечена успешно
									if(length > 0){
										// Завершаем строку нулевым символом
										target[length] = '\0';
										// Если путь к сокету начинается с "socket:["
										if(::strncmp(target, "socket:[", 8) == 0){
											// Извлекаем индексный дескриптор сокета из пути
											ino_t inode = ::strtoull(target + 8, nullptr, 10);
											// Ищем информацию о сокете по индексному дескриптору
											auto i = socketsInfo.find(inode);
											// Если информация о сокете найдена
											if(i != socketsInfo.end()){
												// Если функция обратного вызова установлена
												if(this->_callback != nullptr)
													// Выполняем функцию обратного вызова, передавая информацию о сокете по константной ссылке (без лишнего копирования и аллокаций)
													this->_callback(pid, i->second);
											}
										}
									}
								}
							}
							// Закрываем каталог
							::closedir(dir);
						}
					}
				}
				// Закрываем каталог
				::closedir(dir);
			}
		/**
		 * Реализация под Sun Solaris
		 */
		#elif __sun__
			// Читаем информацию о всех процессах из /proc и сопоставляем её с информацией о сокете
			DIR * dir = ::opendir("/proc");
			// Если каталог открыт удачно
			if(dir != nullptr){
				// Буфер для хранения информации о сокете
				struct dirent * entry = nullptr;
				/**
				 * Читаем записи из каталога и извлекаем информацию о процессе
				 */
				while((entry = ::readdir(dir)) != nullptr){
					// Если запись является каталогом и её имя начинается с цифры
					if((entry->d_name[0] >= '0') && (entry->d_name[0] <= '9')){
						// Получаем идентификатор процесса из имени каталога
						pid_t pid = ::atoi(entry->d_name);
						// Буфер для хранения названия приложения которому принадлежит процесс
						char path[0x100];
						// Выполняем заполнение буфера для получения названия приложения которому принадлежит процесс
						::snprintf(path, sizeof(path), "/proc/%d/fd", pid);
						// Открываем каталог для чтения информации о файловых дескрипторах процесса
						DIR * dir = ::opendir(path);
						// Если каталог открыт удачно
						if(dir != nullptr){
							// Буфер для хранения информации о сокете
							struct dirent * entry = nullptr;
							/**
							 * Читаем записи из каталога и извлекаем информацию о сокете
							 */
							while((entry = ::readdir(dir)) != nullptr){
								// Если запись является каталогом и её имя начинается с цифры
								if((entry->d_name[0] >= '0') && (entry->d_name[0] <= '9')){
									// Буфер для хранения пути к файловому дескриптору процесса
									char path[0x200];
									// Выполняем заполнение буфера для получения пути к файловому дескриптору процесса
									::snprintf(path, sizeof(path), "/proc/%d/fd/%s", pid, entry->d_name);
									// Объект для хранения информации о сокете
									struct stat st{0};
									// Если путь к сокету существует и является сокетом
									if((::stat(path, &st) == 0) && S_ISSOCK(st.st_mode)){
										// Открываем файловый дескриптор для получения информации о сокете
										int32_t fd = ::open(path, O_RDONLY);
										// Если файловый дескриптор открыт удачно
										if(fd >= 0){
											// Объект подключения для получения информации о сокете
											struct sockaddr_storage source{0}, target{0};
											// Получаем разммер объекта подключения адреса источника
											socklen_t sourceLength = sizeof(source);
											// Получаем разммер объекта подключения адреса назначения
											socklen_t targetLength = sizeof(target);
											// Если информация о сокете получена успешно
											if(::getsockname(fd, reinterpret_cast <struct sockaddr *> (&source), &sourceLength) == 0){
												// Если функция обратного вызова установлена
												if(this->_callback != nullptr){
													// Объект для хранения информации о сокете
													info_t info{};
													// Получаем информацию о сокете для адреса назначения
													::getpeername(fd, reinterpret_cast <struct sockaddr *> (&target), &targetLength);
													/**
													 * Определяем семейство IP-адресов сокета
													 */
													switch(source.ss_family){
														// Для семейства UDS
														case AF_UNIX: {
															// Устанавливаем семейство адресов сокета
															info.family = event::family_t::UDS;
															// Выполняем инициализацию объекта UNIX-адреса источника процесса
															info.addresses.src = make_unique <net::addr_fs_t>();
															// Устанавливаем UNIX-адрес источника процесса
															awh_cast <net::addr_fs_t *> (info.addresses.src.get())->address = reinterpret_cast <struct sockaddr_un *> (&source)->sun_path;
															// Если путь к сокету назначения извлечён успешно
															if((targetLength > 0) && (target.ss_family == AF_UNIX)){
																// Выполняем инициализацию объекта UNIX-адреса назначения процесса
																info.addresses.dst = make_unique <net::addr_fs_t> ();
																// Устанавливаем UNIX-адрес назначения процесса
																awh_cast <net::addr_fs_t *> (info.addresses.dst.get())->address = reinterpret_cast <struct sockaddr_un *> (&target)->sun_path;
															}
														} break;
														// Для семейства IPv4
														case AF_INET: {
															// Устанавливаем семейство адресов сокета
															info.family = event::family_t::IPV4;
															// Получаем объект подключения для локального адреса
															auto local = reinterpret_cast <struct sockaddr_in *> (&source);
															// Получаем объект подключения для удалённого адреса
															auto remote = reinterpret_cast <struct sockaddr_in *> (&target);
															// Устанавливаем порт локального адреса
															info.ports.src = ntohs(local->sin_port);
															// Если адрес назначения извлечён успешно
															if((targetLength > 0) && (target.ss_family == AF_INET))
																// Устанавливаем порт удалённого адреса
																info.ports.dst = ntohs(remote->sin_port);
															// Выполняем инициализацию объекта IP-адреса источника процесса
															info.addresses.src = make_unique <net::addr_net_ipv4_t> ();
															// Устанавливаем IP-адрес источника процесса
															awh_cast <net::addr_net_ipv4_t *> (info.addresses.src.get())->address = local->sin_addr.s_addr;
															// Если адрес назначения извлечён успешно
															if((targetLength > 0) && (target.ss_family == AF_INET)){
																// Выполняем инициализацию объекта IP-адреса назначения процесса
																info.addresses.dst = make_unique <net::addr_net_ipv4_t >();
																// Устанавливаем IP-адрес назначения процесса
																awh_cast <net::addr_net_ipv4_t *> (info.addresses.dst.get())->address = remote->sin_addr.s_addr;
															}
														} break;
														// Для семейства IPv6
														case AF_INET6: {
															// Устанавливаем семейство адресов сокета
															info.family = event::family_t::IPV6;
															// Получаем объект подключения для локального адреса
															auto local = reinterpret_cast <struct sockaddr_in6 *> (&source);
															// Получаем объект подключения для удалённого адреса
															auto remote = reinterpret_cast <struct sockaddr_in6 *> (&target);
															// Устанавливаем порт локального адреса
															info.ports.src = ntohs(local->sin6_port);
															// Если адрес назначения извлечён успешно
															if((targetLength > 0) && (target.ss_family == AF_INET6))
																// Устанавливаем порт удалённого адреса
																info.ports.dst = ntohs(remote->sin6_port);
															// Выполняем инициализацию объекта IP-адреса источника процесса
															info.addresses.src = make_unique <net::addr_net_ipv6_t> ();
															// Устанавливаем IP-адрес источника процесса
															::memcpy(&awh_cast <net::addr_net_ipv6_t *> (info.addresses.src.get())->address[0], &local->sin6_addr, 16);
															// Если адрес назначения извлечён успешно
															if((targetLength > 0) && (target.ss_family == AF_INET6)){
																// Выполняем инициализацию объекта IP-адреса назначения процесса
																info.addresses.dst = make_unique <net::addr_net_ipv6_t> ();
																// Устанавливаем IP-адрес назначения процесса
																::memcpy(&awh_cast <net::addr_net_ipv6_t *> (info.addresses.dst.get())->address[0], &remote->sin6_addr, 16);
															}
														} break;
													}
													// Тип сокета для определения протокола сокета
													int32_t type = 0;
													// Получаем размер типа сокета
													socklen_t length = sizeof(type);
													// Если тип сокета получен успешно
													if(::getsockopt(fd, SOL_SOCKET, SO_TYPE, &type, &length) == 0){
														/**
														 * Определяем протокол сокета
														 */
														switch(type){
															// Если тип сокета является TCP-протоколом
															case SOCK_STREAM:
																// Устанавливаем протокол сокета
																info.protocol = event::protocol_t::TCP;
															break;
															// Если тип сокета является UDP-протоколом
															case SOCK_DGRAM:
																// Устанавливаем протокол сокета
																info.protocol = event::protocol_t::UDP;
															break;
															// Если тип сокета является RAW-протоколом
															case SOCK_RAW:
																// Устанавливаем протокол сокета
																info.protocol = event::protocol_t::RAW;
															break;
															// Если протокол сокета не определён
															default : info.protocol = event::protocol_t::NONE;
														}
													}
													// Выполняем функцию обратного вызова
													this->_callback(pid, info);
												}
											}
											// Закрываем файловый дескриптор
											::close(fd);
										}
									}
								}
							}
							// Закрываем каталог
							::closedir(dir);
						}
					}
				}
				// Закрываем каталог
				::closedir(dir);
			}
		/**
		 * Для операционной системы macOS
		 */
		#elif __APPLE__ || __MACH__
			// Узнаём требуемый размер буфера под список идентификаторов процессов (функция возвращает размер данных в байтах)
			int32_t listSize = ::proc_listpids(PROC_ALL_PIDS, 0, nullptr, 0);
			// Если размер буфера для списка идентификаторов процессов получен
			if(listSize > 0)
				// Увеличиваем размер буфера на 16 дополнительных процессов (каждый процесс идентифицируется с помощью PID, который является целым числом)
				listSize += static_cast <int32_t> (16 * sizeof(pid_t));
			// Выделяем память под список идентификаторов процессов
			auto pids = make_unique <pid_t []> ((listSize > 0 ? listSize : 0) / sizeof(pid_t));
			// Получаем список идентификаторов процессов (функция возвращает размер заполненных данных в байтах)
			const int32_t bytes = ((listSize > 0) ? ::proc_listpids(PROC_ALL_PIDS, 0, pids.get(), listSize) : -1);
			// Если список идентификаторов процессов получен
			if(bytes < 0){
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
			// Если список идентификаторов процессов получен
			} else if(bytes > 0) {
				// Объект информации о процессе
				info_t info{};
				// Идентификатор процесса
				pid_t pid = 0;
				// Общее и актуальное количество файловых дескрипторов процесса
				int32_t fds = 0, actual = 0;
				// Текущая ёмкость буфера файловых дескрипторов (в байтах)
				int32_t fdsCapacity = 0;
				// Вычисляем количество идентификаторов процессов (функция возвращает размер данных в байтах, а не количество PID)
				const int32_t count = (bytes / static_cast <int32_t> (sizeof(pid_t)));
				// Динамический буфер списка файловых дескрипторов процесса (переиспользуется между процессами, растёт по мере необходимости)
				unique_ptr <struct proc_fdinfo []> fdinfo = nullptr;
				/**
				 * Переходим по всему списку идентификаторов процессов
				 */
				for(int32_t i = 0; i < count; ++i){
					// Получаем идентификатор процесса
					pid = pids[i];
					// Если идентификатор процесса не получен
					if(pid == 0)
						// Продолжем выполнение цикла
						continue;
					// Узнаём сколько файловых дескрипторов открыто у процесса
					fds = ::proc_pidinfo(pid, PROC_PIDLISTFDS, 0, nullptr, 0);
					// Если количество файловых дескрипторов не получено
					if(fds <= 0)
						// Продолжем выполнение цикла
						continue;
					// Если требуемый размер буфера превышает текущую ёмкость
					if(fds > fdsCapacity){
						// Запоминаем новую ёмкость буфера
						fdsCapacity = fds;
						// Выделяем память под список файловых дескрипторов процесса
						fdinfo = make_unique <struct proc_fdinfo []> (fdsCapacity / sizeof(struct proc_fdinfo));
					}
					// Получаем список актуальных файловых дескрипторов процесса (ограничиваем фактическим размером буфера во избежание переполнения)
					actual = ::proc_pidinfo(pid, PROC_PIDLISTFDS, 0, fdinfo.get(), fdsCapacity);
					// Если список актуальных файловых дескрипторов получен
					if(actual > 0){
						// Вычисляем количество файловых дескрипторов процесса
						actual = (actual / sizeof(struct proc_fdinfo));
						/**
						 * Переходим по всему списку файловых дескрипторов процесса
						 */
						for(int32_t j = 0; j < actual; ++j){
							// Если файловый дескриптор является сокетом
							if(fdinfo[j].proc_fdtype == PROX_FDTYPE_SOCKET){
								// Объект информации о сокете
								struct socket_fdinfo si{};
								// Получаем информацию о сокете
								if(::proc_pidfdinfo(pid, fdinfo[j].proc_fd, PROC_PIDFDSOCKETINFO, &si, sizeof(si)) > 0){
									// Сбрасываем объект информации о процессе
									info = info_t();
									/**
									 * Определяем протокол сокета
									 */
									switch(si.psi.soi_protocol){
										// Если протокол сокета является IP-протоколом
										case IPPROTO_IP:
											// Устанавливаем семейство протокола сокета
											info.family = event::family_t::IPV4;
										break;
										// Если протокол сокета является RAW-протоколом
										case IPPROTO_RAW:
											// Устанавливаем протокол сокета
											info.protocol = event::protocol_t::RAW;
										break;
										// Если протокол сокета является TCP-протоколом
										case IPPROTO_TCP:
											// Устанавливаем протокол сокета
											info.protocol = event::protocol_t::TCP;
										break;
										// Если протокол сокета является UDP-протоколом
										case IPPROTO_UDP:
											// Устанавливаем протокол сокета
											info.protocol = event::protocol_t::UDP;
										break;
										// Если протокол сокета является ICMP-протоколом
										case IPPROTO_ICMP:
											// Устанавливаем протокол сокета
											info.protocol = event::protocol_t::ICMP;
										break;
										// Если протокол сокета является IGMP-протоколом
										case IPPROTO_IGMP:
											// Устанавливаем протокол сокета
											info.protocol = event::protocol_t::IGMP;
										break;
										// Если протокол сокета является SCTP-протоколом
										case IPPROTO_SCTP:
											// Устанавливаем протокол сокета
											info.protocol = event::protocol_t::SCTP;
										break;
										// Если протокол сокета является IPv6-протоколом
										case IPPROTO_IPV6:
											// Устанавливаем семейство протокола сокета
											info.family = event::family_t::IPV6;
										break;
										// Если протокол сокета является ICMPv6-протоколом
										case IPPROTO_ICMPV6:
											// Устанавливаем протокол сокета
											info.protocol = event::protocol_t::ICMP;
										break;
										// Если протокол сокета не определён
										default : info.protocol = event::protocol_t::NONE;
									}
									/**
									 * Определяем тип сокета
									 */
									switch(si.psi.soi_kind){
										// Если сокет является интернет-сокетом
										case SOCKINFO_IN: {
											// Если семейство протокола сокета не определено
											if((si.psi.soi_proto.pri_in.insi_vflag == 0) &&
											(si.psi.soi_protocol != IPPROTO_IP) &&
											(si.psi.soi_protocol != IPPROTO_IPV6))
												// Сбрасываем семейство протокола сокета
												info.family = event::family_t::NONE;
											// Если семейство протокола сокета определено
											else {
												// Если сокет является IPv4-сокетом
												if(si.psi.soi_proto.pri_in.insi_vflag & INI_IPV4)
													// Устанавливаем семейство протокола сокета
													info.family = event::family_t::IPV4;
												// Если сокет является IPv6-сокетом
												if(si.psi.soi_proto.pri_in.insi_vflag & INI_IPV6)
													// Устанавливаем семейство протокола сокета
													info.family = event::family_t::IPV6;
											}
											// Устанавливаем исходный порт сокета
											info.ports.src = ntohs(si.psi.soi_proto.pri_in.insi_lport);
											// Устанавливаем целевой порт сокета
											info.ports.dst = ntohs(si.psi.soi_proto.pri_in.insi_fport);
											/**
											 * Определяем семейство IP-адресов сокета
											 */
											switch(static_cast <uint8_t> (info.family)){
												// Для семейства IPv4
												case static_cast <uint8_t> (event::family_t::IPV4): {
													// Выполняем инициализацию объекта IP-адреса источника процесса
													info.addresses.src = make_unique <net::addr_net_ipv4_t> ();
													// Выполняем инициализацию объекта IP-адреса назначения процесса
													info.addresses.dst = make_unique <net::addr_net_ipv4_t> ();
													// Устанавливаем IP-адрес источника процесса
													awh_cast <net::addr_net_ipv4_t *> (info.addresses.src.get())->address = si.psi.soi_proto.pri_in.insi_laddr.ina_46.i46a_addr4.s_addr;
													// Устанавливаем IP-адрес назначения процесса
													awh_cast <net::addr_net_ipv4_t *> (info.addresses.dst.get())->address = si.psi.soi_proto.pri_in.insi_faddr.ina_46.i46a_addr4.s_addr;
												} break;
												// Для семейства IPv6
												case static_cast <uint8_t> (event::family_t::IPV6): {
													// Выполняем инициализацию объекта IP-адреса источника процесса
													info.addresses.src = make_unique <net::addr_net_ipv6_t> ();
													// Выполняем инициализацию объекта IP-адреса назначения процесса
													info.addresses.dst = make_unique <net::addr_net_ipv6_t> ();
													// Устанавливаем IP-адрес источника процесса
													::memcpy(&awh_cast <net::addr_net_ipv6_t *> (info.addresses.src.get())->address[0], &si.psi.soi_proto.pri_in.insi_laddr.ina_6, 16);
													// Устанавливаем IP-адрес назначения процесса
													::memcpy(&awh_cast <net::addr_net_ipv6_t *> (info.addresses.dst.get())->address[0], &si.psi.soi_proto.pri_in.insi_faddr.ina_6, 16);
												} break;
											}
										} break;
										// Если сокет является TCP-сокетом
										case SOCKINFO_TCP: {
											// Если семейство протокола сокета не определено
											if((si.psi.soi_proto.pri_tcp.tcpsi_ini.insi_vflag == 0) &&
											   (si.psi.soi_protocol != IPPROTO_IP) &&
											   (si.psi.soi_protocol != IPPROTO_IPV6))
												// Сбрасываем семейство протокола сокета
												info.family = event::family_t::NONE;
											// Если семейство протокола сокета определено
											else {
												// Если сокет является IPv4-сокетом
												if(si.psi.soi_proto.pri_tcp.tcpsi_ini.insi_vflag & INI_IPV4)
													// Устанавливаем семейство протокола сокета
													info.family = event::family_t::IPV4;
												// Если сокет является IPv6-сокетом
												if(si.psi.soi_proto.pri_tcp.tcpsi_ini.insi_vflag & INI_IPV6)
													// Устанавливаем семейство протокола сокета
													info.family = event::family_t::IPV6;
											}
											// Устанавливаем исходный порт сокета
											info.ports.src = ntohs(si.psi.soi_proto.pri_tcp.tcpsi_ini.insi_lport);
											// Устанавливаем целевой порт сокета
											info.ports.dst = ntohs(si.psi.soi_proto.pri_tcp.tcpsi_ini.insi_fport);
											/**
											 * Определяем семейство IP-адресов сокета
											 */
											switch(static_cast <uint8_t> (info.family)){
												// Для семейства IPv4
												case static_cast <uint8_t> (event::family_t::IPV4): {
													// Выполняем инициализацию объекта IP-адреса источника процесса
													info.addresses.src = make_unique <net::addr_net_ipv4_t> ();
													// Выполняем инициализацию объекта IP-адреса назначения процесса
													info.addresses.dst = make_unique <net::addr_net_ipv4_t> ();
													// Устанавливаем IP-адрес источника процесса
													awh_cast <net::addr_net_ipv4_t *> (info.addresses.src.get())->address = si.psi.soi_proto.pri_tcp.tcpsi_ini.insi_laddr.ina_46.i46a_addr4.s_addr;
													// Устанавливаем IP-адрес назначения процесса
													awh_cast <net::addr_net_ipv4_t *> (info.addresses.dst.get())->address = si.psi.soi_proto.pri_tcp.tcpsi_ini.insi_faddr.ina_46.i46a_addr4.s_addr;
												} break;
												// Для семейства IPv6
												case static_cast <uint8_t> (event::family_t::IPV6): {
													// Выполняем инициализацию объекта IP-адреса источника процесса
													info.addresses.src = make_unique <net::addr_net_ipv6_t> ();
													// Выполняем инициализацию объекта IP-адреса назначения процесса
													info.addresses.dst = make_unique <net::addr_net_ipv6_t> ();
													// Устанавливаем IP-адрес источника процесса
													::memcpy(&awh_cast <net::addr_net_ipv6_t *> (info.addresses.src.get())->address[0], &si.psi.soi_proto.pri_tcp.tcpsi_ini.insi_laddr.ina_6, 16);
													// Устанавливаем IP-адрес назначения процесса
													::memcpy(&awh_cast <net::addr_net_ipv6_t *> (info.addresses.dst.get())->address[0], &si.psi.soi_proto.pri_tcp.tcpsi_ini.insi_faddr.ina_6, 16);
												} break;
											}
										} break;
										// Если сокет является UNIX-сокетом
										case SOCKINFO_UN: {
											// Устанавливаем семейство протокола сокета
											info.family = event::family_t::UDS;
											// Устанавливаем исходный порт сокета
											info.ports.src = 0;
											// Устанавливаем целевой порт сокета
											info.ports.dst = 0;
											// Выполняем инициализацию объекта UNIX-адреса источника процесса
											info.addresses.src = make_unique <net::addr_fs_t> ();
											// Выполняем инициализацию объекта UNIX-адреса назначения процесса
											info.addresses.dst = make_unique <net::addr_fs_t> ();
											// Устанавливаем UNIX-адрес источника процесса
											awh_cast <net::addr_fs_t *> (info.addresses.src.get())->address = si.psi.soi_proto.pri_un.unsi_addr.ua_sun.sun_path;
											// Устанавливаем UNIX-адрес назначения процесса
											awh_cast <net::addr_fs_t *> (info.addresses.dst.get())->address = si.psi.soi_proto.pri_un.unsi_caddr.ua_sun.sun_path;
										} break;
									}
									// Если функция обратного вызова установлена
									if(this->_callback != nullptr)
										// Выполняем функцию обратного вызова
										this->_callback(pid, info);
								}
							}
						}
					}
				}
			}
		/**
		 * Для операционной системы FreeBSD
		 */
		#elif __FreeBSD__
			// Массив MIB для получения списка процессов
			int32_t mib[3] = {
				CTL_KERN,      // Получаем информацию о ядре
				KERN_PROC,     // Получаем информацию о процессах
				KERN_PROC_PROC // Получаем список всех процессов
			};
			// Количество данных для kinfo_proc
			size_t length = 0;
			// Узнаем размер необходимых данных для kinfo_proc
			if((::sysctl(mib, 3, nullptr, &length, nullptr, 0) == 0) && (length > 0)){
				// Добавляем запас на случай появления новых процессов между двумя вызовами sysctl
				length += ((length / 8) + sizeof(struct kinfo_proc));
				// Выделяем память под процессы
				auto procs = make_unique <struct kinfo_proc []> (length / sizeof(struct kinfo_proc));
				// Получаем список всех процессов
				if(::sysctl(mib, 3, procs.get(), &length, nullptr, 0) == 0){
					// Объект для хранения информации о сокете
					info_t info{};
					// Вычисляем количество процессов
					const size_t count = (length / sizeof(struct kinfo_proc));
					/**
					 * Переходим по всему списку процессов
					 */
					for(size_t i = 0; i < count; ++i){
						// Размер данных для файловых дескрипторов процесса
						size_t length = 0;
						// Получаем идентификатор процесса
						pid_t pid = procs[i].ki_pid;
						// Запрашиваем файловые дескрипторы для конкретного процесса
						int32_t fmib[4] = {
							CTL_KERN,           // Получаем информацию о ядре
							KERN_PROC,          // Получаем информацию о процессах
							KERN_PROC_FILEDESC, // Получаем информацию о файловых дескрипторах процесса
							pid                 // Получаем информацию о конкретном процессе
						};
						// Узнаем размер необходимых данных для файловых дескрипторов процесса
						if((::sysctl(fmib, 4, nullptr, &length, nullptr, 0) == 0) && (length > 0)){
							// Добавляем запас на случай открытия новых дескрипторов между двумя вызовами sysctl
							length += (length / 8);
							// Выделяем память под файловые дескрипторы процесса
							auto files = make_unique <char []> (length);
							// Получаем список файловых дескрипторов процесса
							if(::sysctl(fmib, 4, files.get(), &length, nullptr, 0) == 0){
								// Буфер для обхода списка файловых дескрипторов процесса
								char * ptr = files.get();
								// Вычисляем конец списка файловых дескрипторов процесса
								char * end = (ptr + length);
								/**
								 * Проходим по всем файлам процесса
								 */
								while(ptr < end){
									// Получаем информацию о файле процесса
									struct kinfo_file * kf = reinterpret_cast <struct kinfo_file *> (ptr);
									// Если размер объекта kinfo_file не установлен
									if(kf->kf_structsize == 0)
										// Прекращаем обработку файлов процесса
										break;
									// Если файл оказался сокетом
									if((kf->kf_type == KF_TYPE_SOCKET) && (kf->kf_sock_domain >= 0)){
										// Сбрасываем объект информации о процессе
										info = info_t();
										/**
										 * Определяем протокол сокета
										 */
										switch(kf->kf_sock_protocol){
											// Если протокол сокета является IP-протоколом
											case IPPROTO_IP:
												// Устанавливаем семейство протокола сокета
												info.family = event::family_t::IPV4;
											break;
											// Если протокол сокета является RAW-протоколом
											case IPPROTO_RAW:
												// Устанавливаем протокол сокета
												info.protocol = event::protocol_t::RAW;
											break;
											// Если протокол сокета является TCP-протоколом
											case IPPROTO_TCP:
												// Устанавливаем протокол сокета
												info.protocol = event::protocol_t::TCP;
											break;
											// Если протокол сокета является UDP-протоколом
											case IPPROTO_UDP:
												// Устанавливаем протокол сокета
												info.protocol = event::protocol_t::UDP;
											break;
											// Если протокол сокета является ICMP-протоколом
											case IPPROTO_ICMP:
												// Устанавливаем протокол сокета
												info.protocol = event::protocol_t::ICMP;
											break;
											// Если протокол сокета является IGMP-протоколом
											case IPPROTO_IGMP:
												// Устанавливаем протокол сокета
												info.protocol = event::protocol_t::IGMP;
											break;
											// Если протокол сокета является SCTP-протоколом
											case IPPROTO_SCTP:
												// Устанавливаем протокол сокета
												info.protocol = event::protocol_t::SCTP;
											break;
											// Если протокол сокета является IPv6-протоколом
											case IPPROTO_IPV6:
												// Устанавливаем протокол сокета
												info.family = event::family_t::IPV6;
											break;
											// Если протокол сокета является ICMPv6-протоколом
											case IPPROTO_ICMPV6:
												// Устанавливаем протокол сокета
												info.protocol = event::protocol_t::ICMP;
											break;
											// Если протокол сокета не определён
											default: info.protocol = event::protocol_t::NONE;
										}
										/**
										 * Определяем семейство адресов сокета
										 */
										switch(kf->kf_sock_domain){
											// Для семейства IPv4
											case AF_INET:
												// Устанавливаем семейство протокола сокета
												info.family = event::family_t::IPV4;
											break;
											// Для семейства IPv6
											case AF_INET6:
												// Устанавливаем семейство протокола сокета
												info.family = event::family_t::IPV6;
											break;
											// Для семейства UNIX-сокетов
											case AF_UNIX:
												// Устанавливаем семейство протокола сокета
												info.family = event::family_t::UDS;
											break;
											// Если семейство сокета не определено
											default: info.family = event::family_t::NONE;
										}
										// Получаем объект подключения для локального адреса
										auto source = reinterpret_cast <struct sockaddr *> (&kf->kf_sa_local);
										// Получаем объект подключения для удалённого адреса
										auto destination = reinterpret_cast <struct sockaddr *> (&kf->kf_sa_peer);
										/**
										 * Определяем семейство IP-адресов сокета
										 */
										switch(static_cast <uint8_t> (info.family)){
											// Для семейства UDS
											case static_cast <uint8_t> (event::family_t::UDS): {
												// Если семейство адресов сокета принадлежит к семейству UNIX-сокетов
												if(source->sa_family == AF_UNIX){
													// Устанавливаем исходный порт сокета
													info.ports.src = 0;
													// Устанавливаем целевой порт сокета
													info.ports.dst = 0;
													// Выполняем инициализацию объекта UNIX-адреса источника процесса
													info.addresses.src = make_unique <net::addr_fs_t> ();
													// Выполняем инициализацию объекта UNIX-адреса назначения процесса
													info.addresses.dst = make_unique <net::addr_fs_t> ();
													// Получаем объект подключения для локального адреса
													auto local = reinterpret_cast <struct sockaddr_un *> (source);
													// Получаем объект подключения для удалённого адреса
													auto remote = reinterpret_cast <struct sockaddr_un *> (destination);
													// Устанавливаем UNIX-адрес источника процесса
													awh_cast <net::addr_fs_t *> (info.addresses.src.get())->address = local->sun_path;
													// Если адрес назначения существует (сокет соединён)
													if(destination->sa_family == AF_UNIX)
														// Устанавливаем UNIX-адрес назначения процесса
														awh_cast <net::addr_fs_t *> (info.addresses.dst.get())->address = remote->sun_path;
												}
											} break;
											// Для семейства IPv4
											case static_cast <uint8_t> (event::family_t::IPV4): {
												// Если семейство адресов сокета принадлежит к семейству IPv4
												if(source->sa_family == AF_INET){
													// Выполняем инициализацию объекта IP-адреса источника процесса
													info.addresses.src = make_unique <net::addr_net_ipv4_t> ();
													// Выполняем инициализацию объекта IP-адреса назначения процесса
													info.addresses.dst = make_unique <net::addr_net_ipv4_t> ();
													// Получаем объект подключения для локального адреса
													auto local = reinterpret_cast <struct sockaddr_in *> (source);
													// Получаем объект подключения для удалённого адреса
													auto remote = reinterpret_cast <struct sockaddr_in *> (destination);
													// Определяем, соединён ли сокет (есть ли валидный адрес назначения)
													const bool connected = (destination->sa_family == AF_INET);
													// Устанавливаем порт локального адреса
													info.ports.src = ntohs(local->sin_port);
													// Устанавливаем порт удалённого адреса
													info.ports.dst = (connected ? ntohs(remote->sin_port) : 0);
													// Устанавливаем IP-адрес источника процесса
													awh_cast <net::addr_net_ipv4_t *> (info.addresses.src.get())->address = local->sin_addr.s_addr;
													// Если адрес назначения существует
													if(connected)
														// Устанавливаем IP-адрес назначения процесса
														awh_cast <net::addr_net_ipv4_t *> (info.addresses.dst.get())->address = remote->sin_addr.s_addr;
												}
											} break;
											// Для семейства IPv6
											case static_cast <uint8_t> (event::family_t::IPV6): {
												// Если семейство адресов сокета принадлежит к семейству IPv6
												if(source->sa_family == AF_INET6){
													// Выполняем инициализацию объекта IP-адреса источника процесса
													info.addresses.src = make_unique <net::addr_net_ipv6_t> ();
													// Выполняем инициализацию объекта IP-адреса назначения процесса
													info.addresses.dst = make_unique <net::addr_net_ipv6_t> ();
													// Получаем объект подключения для локального адреса
													auto local = reinterpret_cast <struct sockaddr_in6 *> (source);
													// Получаем объект подключения для удалённого адреса
													auto remote = reinterpret_cast <struct sockaddr_in6 *> (destination);
													// Определяем, соединён ли сокет (есть ли валидный адрес назначения)
													const bool connected = (destination->sa_family == AF_INET6);
													// Устанавливаем порт локального адреса
													info.ports.src = ntohs(local->sin6_port);
													// Устанавливаем порт удалённого адреса
													info.ports.dst = (connected ? ntohs(remote->sin6_port) : 0);
													// Устанавливаем IP-адрес источника процесса
													::memcpy(&awh_cast <net::addr_net_ipv6_t *> (info.addresses.src.get())->address[0], &local->sin6_addr, 16);
													// Если адрес назначения существует
													if(connected)
														// Устанавливаем IP-адрес назначения процесса
														::memcpy(&awh_cast <net::addr_net_ipv6_t *> (info.addresses.dst.get())->address[0], &remote->sin6_addr, 16);
												}
											} break;
										}
										// Если функция обратного вызова установлена
										if(this->_callback != nullptr)
											// Выполняем функцию обратного вызова
											this->_callback(pid, info);
									}
									// Переходим к следующему файлу процесса
									ptr += kf->kf_structsize;
								}
							}
						}
					}
				}
			}
		/**
		 * Для операционной системы NetBSD или OpenBSD
		 */
		#elif __NetBSD__ || __OpenBSD__
			// Если функция обратного вызова установлена
			if(this->_callback != nullptr){
				/**
				 * В NetBSD и OpenBSD системные структуры для сокетов часто закрыты или требуют libkvm и прав root
				 * Надежный способ из пространства пользователя — парсинг вывода штатных системных утилит (sockstat/fstat)
				 */
				/**
				 * Для операционной системы NetBSD
				 */
				#if __NetBSD__
					// Открываем процесс для чтения вывода команды sockstat
					FILE * fp = ::popen("sockstat -n 2>/dev/null", "r");
					// Если процесс открыт успешно
					if(fp != nullptr){
						// Буфер для чтения строк вывода команды
						char buffer[0x400];
						/**
						 * Читаем заголовок вывода команды
						 */
						if(::fgets(buffer, sizeof(buffer), fp)){
							/**
							 * Читаем строки вывода команды
							 */
							while(::fgets(buffer, sizeof(buffer), fp)){
								// Парсим строку вывода команды
								istringstream iss(buffer);
								// Переменные для хранения полей вывода команды
								string user = "", cmd = "", pid_str = "", fd_str = "", proto = "", local = "", foreign = "";
								/**
								 * Если строка вывода команды соответствует формату:
								 * FORMAT: USER COMMAND PID FD PROTO LOCAL FOREIGN
								 */
								if(iss >> user >> cmd >> pid_str >> fd_str >> proto >> local){
									// Объект для хранения информации о сокете
									info_t info{};
									// Получаем идентификатор процесса
									pid_t pid = ::atoi(pid_str.c_str());
									// Читаем удалённый адрес, если он есть
									iss >> foreign;
									// Если строка содержит информацию о сетевом сокете
									if((proto == "tcp") || (proto == "udp")){
										// Устанавливаем протокол сокета
										info.protocol = ((proto == "tcp") ? event::protocol_t::TCP : event::protocol_t::UDP);
										/**
										 * @brief Функция для парсинга адреса и порта из строки формата "IP.PORT" или "*.*"
										 *
										 * @param addr     строка с адресом и портом
										 * @param isSource флаг, указывающий, является ли адрес источником (true) или назначением (false)
										 *
										 */
										auto parse = [&info](const string & addr, const bool isSource) noexcept -> void {
											// Если адрес является универсальным или пустым, пропускаем его
											if((addr == "*.*") || addr.empty())
												// Выход из функции, так как нет конкретного адреса и порта для обработки
												return;
											// Находим позицию последней точки, которая разделяет IP-адрес и порт
											auto pos = addr.find_last_of('.');
											// Если позиция разделителя найдена и она не является первой символом, продолжаем парсинг
											if((pos != std::string::npos) && (pos > 0)){
												// Извлекаем IP-адрес из строки, используя позицию разделителя
												string ip = addr.substr(0, pos);
												// Извлекаем порт из строки, используя позицию разделителя, и конвертируем его в число
												uint16_t port = static_cast <uint16_t> (::atoi(addr.substr(pos + 1).c_str()));
												// Если флаг установлен на источник
												if(isSource)
													// Устанавливаем порт источника
													info.ports.src = port;
												// Устанавливаем порт назначения
												else info.ports.dst = port;
												// Если IP-адрес принадлежит к семейству IPv6
												if(ip.find(':') != std::string::npos){
													// Устанавливаем семейство протокола сокета
													info.family = event::family_t::IPV6;
													// Выполняем инициализацию объекта IP-адреса (IPv6)
													auto a = make_unique <net::addr_net_ipv6_t> ();
													// Устанавливаем IP-адрес в созданный объект IPv6-адреса
													::inet_pton(AF_INET6, ip.c_str(), &a->address);
													// Если флаг установлен на источник
													if(isSource)
														// Устанавливаем IP-адрес источника
														info.addresses.src = ::move(a);
													// Устанавливаем IP-адрес назначения
													else info.addresses.dst = ::move(a);
												// Если IP-адрес принадлежит к семейству IPv4
												} else {
													// Устанавливаем семейство протокола сокета
													info.family = event::family_t::IPV4;
													// Выполняем инициализацию объекта IP-адреса (IPv4)
													auto a = make_unique <net::addr_net_ipv4_t> ();
													// Устанавливаем IP-адрес в созданный объект IPv4-адреса
													::inet_pton(AF_INET, ip.c_str(), &a->address);
													// Если флаг установлен на источник
													if(isSource)
														// Устанавливаем IP-адрес источника
														info.addresses.src = ::move(a);
													// Устанавливаем IP-адрес назначения
													else info.addresses.dst = ::move(a);
												}
											}
										};
										// Парсим локальный адрес и порт
										parse(local, true);
										// Парсим удалённый адрес и порт
										parse(foreign, false);
										// Если семейство протокола сокета определено
										if(info.family != event::family_t::NONE)
											// Выполняем функцию обратного вызова
											this->_callback(pid, info);
									// Если строка содержит информацию о UNIX-сокете
									} else if((local == "local") || (proto == "local")) {
										// Устанавливаем семейство протокола сокета
										info.family = event::family_t::UDS;
										// Устанавливаем протокол сокета
										info.protocol = event::protocol_t::NONE;
										// Выполняем инициализацию объекта файлового адреса источника процесса
										info.addresses.src = make_unique <net::addr_fs_t> ();
										// Переменная для хранения оставшейся части строки после удалённого адреса
										string rest = "";
										// Инициализируем путь к сокету значением удалённого адреса (который может содержать путь к сокету)
										string path = foreign;
										// Читаем оставшуюся часть строки, которая может содержать путь к сокету
										std::getline(iss, rest);
										// Если оставшаяся часть строки не пуста, добавляем её к пути
										if(!rest.empty())
											// Добавляем оставшуюся часть строки к пути, так как она может содержать дополнительные компоненты пути к сокету
											path += rest;
										// Если путь к сокету не пустой
										if(!path.empty()){
											// Устанавливаем файловый адрес источника процесса
											awh_cast <net::addr_fs_t *> (info.addresses.src.get())->address = path;
											// Выполняем функцию обратного вызова
											this->_callback(pid, info);
										}
									}
								}
							}
						}
						// Закрываем процесс
						::pclose(fp);
					}
				/**
				 * Для операционной системы OpenBSD
				 */
				#elif __OpenBSD__
					// Открываем процесс для чтения вывода команды fstat
					FILE * fp = ::popen("fstat -n 2>/dev/null", "r");
					// Если процесс открыт успешно
					if(fp != nullptr){
						// Буфер для чтения строк вывода команды
						char buffer[0x400];
						/**
						 * Читаем заголовок вывода команды
						 */
						if(::fgets(buffer, sizeof(buffer), fp)){
							/**
							 * Читаем строки вывода команды
							 */
							while(::fgets(buffer, sizeof(buffer), fp)){
								// Парсим строку вывода команды
								istringstream iss(buffer);
								// Переменные для хранения полей вывода команды
								string user = "", cmd = "", pid_str = "", fd_str = "", mount = "", mode = "", proto = "", sz_dv = "", local = "", arrow = "", foreign = "";
								/**
								 * Если строка вывода команды соответствует формату:
								 * FORMAT: USER CMD PID FD MOUNT INUM MODE SZ|DV R/W [LOCAL] [<-> FOREIGN]
								 */
								if(iss >> user >> cmd >> pid_str >> fd_str >> mount){
									// Объект для хранения информации о сокете
									info_t info{};
									// Получаем идентификатор процесса
									pid_t pid = ::atoi(pid_str.c_str());
									// Если строка содержит информацию о сетевом сокете
									if(mount == "internet"){
										// Читаем остальные поля строки вывода команды
										iss >> mode >> proto >> sz_dv >> local;
										// Устанавливаем протокол сокета
										info.protocol = ((proto == "tcp") ? event::protocol_t::TCP :
														((proto == "udp") ? event::protocol_t::UDP : event::protocol_t::NONE));
										// Если строка содержит информацию о удалённом адресе
										if(iss >> arrow){
											// Если строка содержит символ "<->", это означает, что за ним следует удалённый адрес
											if(arrow == "<->")
												// Читаем удалённый адрес
												iss >> foreign;
											// Если строка не содержит символ "<->", это означает, что удалённый адрес отсутствует, и мы должны использовать значение после "local" в качестве удалённого адреса
											else foreign = arrow;
										}
										/**
										 * @brief Функция для парсинга адреса и порта из строки формата "IP:PORT" или "*:*"
										 *
										 * @param addr     строка с адресом и портом
										 * @param isSource флаг, указывающий, является ли адрес источником (true) или назначением (false)
										 *
										 */
										auto parse = [&info](const string & addr, const bool isSource) noexcept -> void {
											// Если адрес является универсальным или пустым, пропускаем его
											if((addr == "*:*") || addr.empty() || (addr.find("*:") == 0))
												// Выход из функции, так как нет конкретного адреса и порта для обработки
												return;
											// Находим позицию последнего двоеточия, которая разделяет IP-адрес и порт
											auto pos = addr.find_last_of(':');
											// Если позиция разделителя найдена и она не является первой символом, продолжаем парсинг
											if((pos != std::string::npos) && (pos > 0)){
												// Извлекаем IP-адрес из строки, используя позицию разделителя
												string ip = addr.substr(0, pos);
												// Извлекаем порт из строки, используя позицию разделителя, и конвертируем его в число
												uint16_t port = static_cast <uint16_t> (::atoi(addr.substr(pos + 1).c_str()));
												// Если IP-адрес заключён в квадратные скобки (что может указывать на IPv6-адрес), удаляем их
												if((ip.front() == '[') && (ip.back() == ']'))
													// Удаляем квадратные скобки из IP-адреса
													ip = ip.substr(1, ip.size() - 2);
												// Если флаг установлен на источник
												if(isSource)
													// Устанавливаем порт источника
													info.ports.src = port;
												// Устанавливаем порт назначения
												else info.ports.dst = port;
												// Если IP-адрес принадлежит к семейству IPv6
												if(ip.find(':') != string::npos){
													// Устанавливаем семейство протокола сокета
													info.family = event::family_t::IPV6;
													// Выполняем инициализацию объекта IP-адреса (IPv6)
													auto a = make_unique <net::addr_net_ipv6_t> ();
													// Устанавливаем IP-адрес в созданный объект IPv6-адреса
													::inet_pton(AF_INET6, ip.c_str(), &a->address);
													// Если флаг установлен на источник
													if(isSource)
														// Устанавливаем IP-адрес источника
														info.addresses.src = ::move(a);
													// Устанавливаем IP-адрес назначения
													else info.addresses.dst = ::move(a);
												// Если IP-адрес принадлежит к семейству IPv4
												} else {
													// Устанавливаем семейство протокола сокета
													info.family = event::family_t::IPV4;
													// Выполняем инициализацию объекта IP-адреса (IPv4)
													auto a = make_unique <net::addr_net_ipv4_t> ();
													// Устанавливаем IP-адрес в созданный объект IPv4-адреса
													::inet_pton(AF_INET, ip.c_str(), &a->address);
													// Если флаг установлен на источник
													if(isSource)
														// Устанавливаем IP-адрес источника
														info.addresses.src = ::move(a);
													// Устанавливаем IP-адрес назначения
													else info.addresses.dst = ::move(a);
												}
											}
										};
										// Парсим локальный адрес и порт
										parse(local, true);
										// Парсим удалённый адрес и порт
										parse(foreign, false);
										// Если семейство протокола сокета определено
										if(info.family != event::family_t::NONE)
											// Выполняем функцию обратного вызова
											this->_callback(pid, info);
									// Если строка содержит информацию о UNIX-сокете
									} else if(mount == "unix") {
										// Читаем остальные поля строки вывода команды
										iss >> mode >> sz_dv >> local;
										// Инициализируем путь к сокету
										string path = "";
										// Список партиций строки, который будет использоваться для поиска пути к сокету
										vector <string> parts;
										// Добавляем локальный адрес в список партиций, так как он может содержать путь к сокету
										parts.push_back(local);
										// Инициализируем строковое значение для чтения оставшейся части строки, которая может содержать путь к сокету
										string part = "";
										/**
										 * Читаем оставшиеся части строки, которые могут содержать путь к сокету
										 */
										while(iss >> part)
											// Добавляем прочитанную часть строки в список партиций, так как она может содержать дополнительные компоненты пути к сокету
											parts.push_back(part);
										/**
										 * Пытаемся найти путь к сокету, который обычно не начинается с "0x" (указывающего на адрес в шестнадцатеричном формате) и не является типом сокета ("dgram" или "stream")
										 */
										for(auto i = parts.rbegin(); i != parts.rend(); ++i){
											// Если текущая часть строки не начинается с "0x" и не является типом сокета "dgram" или "stream", предполагаем, что это путь к сокету
											if(((*i).find("0x") != 0) && ((* i) != "dgram") && ((* i) != "stream")){
												// Устанавливаем путь к сокету
												path = * i;
												// Прекращаем поиск пути к сокету, так как мы уже нашли его
												break;
											}
										}
										// Если путь к сокету получен успешно
										if(!path.empty()){
											// Устанавливаем семейство протокола сокета
											info.family = event::family_t::UDS;
											// Устанавливаем протокол сокета
											info.protocol = event::protocol_t::NONE;
											// Выполняем инициализацию объекта файлового адреса источника процесса
											info.addresses.src = make_unique <net::addr_fs_t> ();
											// Устанавливаем файловый адрес источника процесса
											awh_cast <net::addr_fs_t *> (info.addresses.src.get())->address = path;
											// Выполняем функцию обратного вызова
											this->_callback(pid, info);
										}
									}
								}
							}
						}
						// Закрываем процесс
						::pclose(fp);
					}
				#endif
			}
		/**
		 * Для операционной системы MS Windows
		 */
		#elif _WIN32 || _WIN64
			// Если функция обратного вызова установлена
			if(this->_callback != nullptr){
				// Размер данных для таблицы TCP-соединений
				ULONG size = 0;
				// Если мы успешно получили размер данных для таблицы TCP-соединений IPv4
				if(::GetExtendedTcpTable(nullptr, &size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0) == ERROR_INSUFFICIENT_BUFFER){
					// Выделяем память под таблицу TCP-соединений IPv4
					auto table = make_unique <uint8_t []> (size);
					// Если мы успешно получили данные для таблицы TCP-соединений IPv4
					if(::GetExtendedTcpTable(table.get(), &size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0) == NO_ERROR){
						// Получаем объект таблицы TCP-соединений IPv4
						auto * pTcpTable = reinterpret_cast <PMIB_TCPTABLE_OWNER_PID> (table.get());
						/**
						 * Проходим по всем записям в таблице TCP-соединений IPv4
						 */
						for(DWORD i = 0; i < pTcpTable->dwNumEntries; i++){
							// Объект для хранения информации о сокете
							info_t info{};
							// Устанавливаем семейство протокола сокета
							info.family = event::family_t::IPV4;
							// Устанавливаем протокол сокета
							info.protocol = event::protocol_t::TCP;
							// Устанавливаем исходный порт сокета
							info.ports.src = ntohs(static_cast <uint16_t> (pTcpTable->table[i].dwLocalPort));
							// Устанавливаем целевой порт сокета
							info.ports.dst = ntohs(static_cast <uint16_t> (pTcpTable->table[i].dwRemotePort));
							// Выполняем инициализацию объекта IP-адреса источника процесса
							info.addresses.src = make_unique <net::addr_net_ipv4_t> ();
							// Выполняем инициализацию объекта IP-адреса назначения процесса
							info.addresses.dst = make_unique <net::addr_net_ipv4_t> ();
							// Устанавливаем IP-адрес источника процесса
							awh_cast <net::addr_net_ipv4_t *> (info.addresses.src.get())->address = pTcpTable->table[i].dwLocalAddr;
							// Устанавливаем IP-адрес назначения процесса
							awh_cast <net::addr_net_ipv4_t *> (info.addresses.dst.get())->address = pTcpTable->table[i].dwRemoteAddr;
							// Выполняем функцию обратного вызова для текущей записи в таблице TCP-соединений IPv4
							this->_callback(pTcpTable->table[i].dwOwningPid, info);
						}
					}
				}
				// Сбрасываем размер данных для таблицы TCP-соединений IPv6
				size = 0;
				// Если мы успешно получили размер данных для таблицы TCP-соединений IPv6
				if(::GetExtendedTcpTable(nullptr, &size, FALSE, AF_INET6, TCP_TABLE_OWNER_PID_ALL, 0) == ERROR_INSUFFICIENT_BUFFER){
					// Выделяем память под таблицу TCP-соединений IPv6
					auto table = make_unique <uint8_t []> (size);
					// Если мы успешно получили данные для таблицы TCP-соединений IPv6
					if(::GetExtendedTcpTable(table.get(), &size, FALSE, AF_INET6, TCP_TABLE_OWNER_PID_ALL, 0) == NO_ERROR){
						// Получаем объект таблицы TCP-соединений IPv6
						auto * pTcp6Table = reinterpret_cast <PMIB_TCP6TABLE_OWNER_PID> (table.get());
						/**
						 * Проходим по всем записям в таблице TCP-соединений IPv6
						 */
						for(DWORD i = 0; i < pTcp6Table->dwNumEntries; i++){
							// Объект для хранения информации о сокете
							info_t info{};
							// Устанавливаем семейство протокола сокета
							info.family = event::family_t::IPV6;
							// Устанавливаем протокол сокета
							info.protocol = event::protocol_t::TCP;
							// Устанавливаем исходный порт сокета
							info.ports.src = ntohs(static_cast <uint16_t> (pTcp6Table->table[i].dwLocalPort));
							// Устанавливаем целевой порт сокета
							info.ports.dst = ntohs(static_cast <uint16_t> (pTcp6Table->table[i].dwRemotePort));
							// Выполняем инициализацию объекта IP-адреса источника процесса
							info.addresses.src = make_unique <net::addr_net_ipv6_t>();
							// Выполняем инициализацию объекта IP-адреса назначения процесса
							info.addresses.dst = make_unique <net::addr_net_ipv6_t>();
							// Устанавливаем IP-адрес источника процесса
							::memcpy(&awh_cast <net::addr_net_ipv6_t *> (info.addresses.src.get())->address[0], pTcp6Table->table[i].ucLocalAddr, 16);
							// Устанавливаем IP-адрес назначения процесса
							::memcpy(&awh_cast <net::addr_net_ipv6_t *> (info.addresses.dst.get())->address[0], pTcp6Table->table[i].ucRemoteAddr, 16);
							// Выполняем функцию обратного вызова для текущей записи в таблице TCP-соединений IPv6
							this->_callback(pTcp6Table->table[i].dwOwningPid, info);
						}
					}
				}
				// Сбрасываем размер данных для таблицы UDP-соединений
				size = 0;
				// Если мы успешно получили размер данных для таблицы UDP-соединений IPv4
				if(::GetExtendedUdpTable(nullptr, &size, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0) == ERROR_INSUFFICIENT_BUFFER){
					// Выделяем память под таблицу UDP-соединений IPv4
					auto table = make_unique <uint8_t []> (size);
					// Если мы успешно получили данные для таблицы UDP-соединений IPv4
					if(::GetExtendedUdpTable(table.get(), &size, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0) == NO_ERROR){
						// Получаем объект таблицы UDP-соединений IPv4
						auto * pUdpTable = reinterpret_cast <PMIB_UDPTABLE_OWNER_PID> (table.get());
						/**
						 * Проходим по всем записям в таблице UDP-соединений IPv4
						 */
						for(DWORD i = 0; i < pUdpTable->dwNumEntries; i++){
							// Объект для хранения информации о сокете
							info_t info{};
							// Устанавливаем семейство протокола сокета
							info.family = event::family_t::IPV4;
							// Устанавливаем протокол сокета
							info.protocol = event::protocol_t::UDP;
							// Устанавливаем целевой порт сокета (для UDP он всегда 0, так как UDP не устанавливает соединение)
							info.ports.dst = 0;
							// Устанавливаем исходный порт сокета
							info.ports.src = ntohs(static_cast <uint16_t> (pUdpTable->table[i].dwLocalPort));
							// Выполняем инициализацию объекта IP-адреса источника процесса
							info.addresses.src = make_unique <net::addr_net_ipv4_t>();
							// Выполняем инициализацию объекта IP-адреса назначения процесса
							info.addresses.dst = make_unique <net::addr_net_ipv4_t>();
							// Устанавливаем IP-адрес источника процесса
							awh_cast <net::addr_net_ipv4_t *> (info.addresses.src.get())->address = pUdpTable->table[i].dwLocalAddr;
							// Выполняем функцию обратного вызова для текущей записи в таблице UDP-соединений IPv4
							this->_callback(pUdpTable->table[i].dwOwningPid, info);
						}
					}
				}
				// Сбрасываем размер данных для таблицы UDP-соединений IPv6
				size = 0;
				// Если мы успешно получили размер данных для таблицы UDP-соединений IPv6
				if(::GetExtendedUdpTable(nullptr, &size, FALSE, AF_INET6, UDP_TABLE_OWNER_PID, 0) == ERROR_INSUFFICIENT_BUFFER){
					// Выделяем память под таблицу UDP-соединений IPv6
					auto table = make_unique <uint8_t []> (size);
					// Если мы успешно получили данные для таблицы UDP-соединений IPv6
					if(::GetExtendedUdpTable(table.get(), &size, FALSE, AF_INET6, UDP_TABLE_OWNER_PID, 0) == NO_ERROR){
						// Получаем объект таблицы UDP-соединений IPv6
						auto * pUdp6Table = reinterpret_cast <PMIB_UDP6TABLE_OWNER_PID> (table.get());
						/**
						 * Проходим по всем записям в таблице UDP-соединений IPv6
						 */
						for(DWORD i = 0; i < pUdp6Table->dwNumEntries; i++){
							// Объект для хранения информации о сокете
							info_t info{};
							// Устанавливаем семейство протокола сокета
							info.family = event::family_t::IPV6;
							// Устанавливаем протокол сокета
							info.protocol = event::protocol_t::UDP;
							// Устанавливаем целевой порт сокета (для UDP он всегда 0, так как UDP не устанавливает соединение)
							info.ports.dst = 0;
							// Устанавливаем исходный порт сокета
							info.ports.src = ntohs(static_cast <uint16_t> (pUdp6Table->table[i].dwLocalPort));
							// Выполняем инициализацию объекта IP-адреса источника процесса
							info.addresses.src = make_unique <net::addr_net_ipv6_t>();
							// Выполняем инициализацию объекта IP-адреса назначения процесса
							info.addresses.dst = make_unique <net::addr_net_ipv6_t>();
							// Устанавливаем IP-адрес источника процесса
							::memcpy(&awh_cast<net::addr_net_ipv6_t*>(info.addresses.src.get())->address[0], pUdp6Table->table[i].ucLocalAddr, 16);
							// Выполняем функцию обратного вызова для текущей записи в таблице UDP-соединений IPv6
							this->_callback(pUdp6Table->table[i].dwOwningPid, info);
						}
					}
				}
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
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", awh::log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод получения названия приложения по идентификатору процесса
 *
 * @param pid идентификатор процесса
 * @return    название приложения которому принадлежит процесс
 *
 */
string awh::Process_Resolver::name(const pid_t pid) const noexcept {
	// Переменная результата
	string result = "";
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
			::snprintf(buffer, sizeof(buffer), "/proc/%d/comm", pid);
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
			// Выполняем получение данных процесса
			struct kinfo_proc * proc = ::kinfo_getproc(pid);
			// Если данные процесса получены
			if(proc != nullptr){
				// Выполняем получение названия процесса
				result = proc->ki_comm;
				// Очищаем ранее созданный объект с данными процесса
				::free(proc);
			}
		/**
		 * Для операционной системы macOS
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
		 * Для операционной системы OpenBSD
		 *
		 * @details Название процесса берётся у ядра запросом, а не из файловой системы
		 *          процессов: OpenBSD её убрала целиком - каталога `/proc` там нет
		 *          вовсе, - и чтение оттуда возвращало пустое название всегда. Ядро
		 *          же держит название в записи о процессе и отдаёт его запросом
		 *
		 */
		#elif __OpenBSD__
			// Запись о процессе, получаемая у ядра
			struct kinfo_proc process;
			// Зануляем запись о процессе
			::memset(&process, 0, sizeof(process));
			// Размер записи о процессе
			size_t length = sizeof(process);
			// Идентификатор параметра записи о запрошенном процессе
			int32_t mib[6] = {
				CTL_KERN,           // Ядро системы
				KERN_PROC,          // Записи о процессах
				KERN_PROC_PID,      // Отбор по идентификатору процесса
				static_cast <int32_t> (pid),
				static_cast <int32_t> (sizeof(process)),
				1                   // Число запрашиваемых записей
			};
			// Если запись о процессе получена и название в ней есть
			if((::sysctl(mib, 6, &process, &length, nullptr, 0) == 0) && (length > 0) && (process.p_comm[0] != '\0'))
				// Устанавливаем название процесса
				result.assign(process.p_comm, ::strnlen(process.p_comm, sizeof(process.p_comm)));
		/**
		 * Для операционной системы NetBSD
		 */
		#elif __NetBSD__
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
				// Выполняем извлечение данных в буфер (резервируем место под нуль-терминатор, readlink его не добавляет)
				const int32_t size = static_cast <int32_t> (::readlink(ss.str().c_str(), buffer, sizeof(buffer) - 1));
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
					if(!result.empty() && (result.rbegin()[0] == '\n'))
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
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(pid), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", awh::log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод установки функции обратного вызова для получения информации о процессе
 *
 * @param callback функция обратного вызова
 *
 */
void awh::Process_Resolver::on(function <void (const pid_t, const info_t &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback = ::move(callback);
}
/**
 * @brief Конструктор
 *
 * @param log объект для работы с логами
 *
 */
awh::Process_Resolver::Process_Resolver(const log_t * log) noexcept : _callback(nullptr), _log(log) {}
/**
 * @brief Деструктор
 *
 */
awh::Process_Resolver::~Process_Resolver() noexcept {}
