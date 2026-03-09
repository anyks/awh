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
#include <netinet/in.h>

/**
 * Для операционной системы Linux
 */
#ifdef __linux__
	#include <map>
	#include <dirent.h>
	#include <unistd.h>
	#include <sys/file.h>
	#include <sys/stat.h>
/**
 * Для операционной системы FreeBSD
 */
#elif __FreeBSD__
	#include <libutil.h>
	#include <sys/un.h>
	#include <sys/user.h>
	#include <sys/sysctl.h>
	#include <sys/socket.h>
/**
 * Для операционной системы MacOS X
 */
#elif __APPLE__ || __MACH__
	#include <libproc.h>
/**
 * Для операционной системы NetBSD или OpenBSD
 */
#elif __NetBSD__ || __OpenBSD__
	#include <memory>
	#include <vector>
	#include <fstream>
	#include <sstream>
	#include <arpa/inet.h>
/**
 * Реализация под Sun Solaris
 */
#elif __sun__
	#include <fstream>
	#include <sstream>
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
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <iphlpapi.h>
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
			 * @param filename адрес файла для извлечения
			 * @param proto    протокол сокета
			 * @param family   семейство адресов сокета
			 */
			auto parse = [&socketsInfo](const char* filename, const event::protocol_t proto, const event::family_t family) noexcept -> void {
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
					// Состояние сокета
					int32_t state = 0;
					// Объект для хранения информации о сокете
					info_t info{};
					// Устанавливаем семейство адресов сокета
					info.family = family;
					// Устанавливаем протокол сокета
					info.protocol = proto;
					/**
					 * Определяем семейстов IP-адресов сокета
					 */
					switch(static_cast <uint8_t> (family)){
						// Для семейства IPv4
						case static_cast <uint8_t> (event::family_t::IPV4): {
							// Буфер для хранения IP-адреса назначения
							uint32_t target = 0;
							// Буфер для хранения IP-адреса источника
							uint32_t source = 0;
							// Извлекаем информацию о сокете из строки
							if(::sscanf(buffer, "%*d: %x:%x %x:%x %x %*x:%*x %*x:%*x %*x %*d %*d %lu", &source, &info.ports.src, &target, &info.ports.dst, &state, &inode) == 6){
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
							// Извлекаем информацию о сокете из строки
							if(::sscanf(buffer, "%*d: %8x%8x%8x%8x:%x %8x%8x%8x%8x:%x %x %*x:%*x %*x:%*x %*x %*d %*d %lu",
							   &source[0], &source[1], &source[2], &source[3], &info.ports.src,
							   &target[0], &target[1], &target[2], &target[3], &info.ports.dst, &state, &inode) == 12){
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
			// Выполняем извлечение информации о сокете из файла /proc/net/tcp6 для протокола TCP и семейства IPv6
			parse("/proc/net/tcp6", event::protocol_t::TCP, event::family_t::IPV6);
			// Выполняем извлечение информации о сокете из файла /proc/net/udp6 для протокола UDP и семейства IPv6
			parse("/proc/net/udp6", event::protocol_t::UDP, event::family_t::IPV6);
			// Выполняем извлечение информации о сокете из файла /proc/net/raw6 для протокола RAW и семейства IPv6
			parse("/proc/net/raw6", event::protocol_t::RAW, event::family_t::IPV6);
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
					if((entry->d_type == DT_DIR) && ::isdigit(entry->d_name[0])){
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
												if(this->_callback != nullptr){
													// Объект для хранения информации о сокете
													info_t info{};
													// Устанавливаем порты процесса
													info.ports = i->second.ports;
													// Устанавливаем семейство адресов процесса
													info.family = i->second.family;
													// Устанавливаем протокол процесса
													info.protocol = i->second.protocol;
													/**
													 * Определяем семейстов IP-адресов сокета
													 */
													switch(static_cast <uint8_t> (info.family)){
														// Для семейства UDS
														case static_cast <uint8_t> (event::family_t::UDS): {
															// Выполняем инициализацию объекта UNIX-адреса источника процесса
															info.addresses.src = make_unique <net::addr_fs_t> ();
															// Выполняем инициализацию объекта UNIX-адреса назначения процесса
															info.addresses.dst = make_unique <net::addr_fs_t> ();
															// Устанавливаем UNIX-адрес источника процесса
															awh_cast <net::addr_fs_t *> (info.addresses.src.get())->address = awh_cast <net::addr_fs_t *> (i->second.addresses.src.get())->address;
															// Устанавливаем UNIX-адрес назначения процесса
															awh_cast <net::addr_fs_t *> (info.addresses.dst.get())->address = awh_cast <net::addr_fs_t *> (i->second.addresses.dst.get())->address;
														} break;
														// Для семейства IPv4
														case static_cast <uint8_t> (event::family_t::IPV4): {
															// Выполняем инициализацию объекта IP-адреса источника процесса
															info.addresses.src = make_unique <net::addr_net_ipv4_t> ();
															// Выполняем инициализацию объекта IP-адреса назначения процесса
															info.addresses.dst = make_unique <net::addr_net_ipv4_t> ();
															// Устанавливаем IP-адрес источника процесса
															awh_cast <net::addr_net_ipv4_t *> (info.addresses.src.get())->address = awh_cast <net::addr_net_ipv4_t *> (i->second.addresses.src.get())->address;
															// Устанавливаем IP-адрес назначения процесса
															awh_cast <net::addr_net_ipv4_t *> (info.addresses.dst.get())->address = awh_cast <net::addr_net_ipv4_t *> (i->second.addresses.dst.get())->address;
														} break;
														// Для семейства IPv6
														case static_cast <uint8_t> (event::family_t::IPV6): {
															// Выполняем инициализацию объекта IP-адреса источника процесса
															info.addresses.src = make_unique <net::addr_net_ipv6_t> ();
															// Выполняем инициализацию объекта IP-адреса назначения процесса
															info.addresses.dst = make_unique <net::addr_net_ipv6_t> ();
															// Устанавливаем IP-адрес источника процесса
															::memcpy(&awh_cast <net::addr_net_ipv6_t *> (info.addresses.src.get())->address[0], &awh_cast <net::addr_net_ipv6_t *> (i->second.addresses.src.get())->address[0], 16);
															// Устанавливаем IP-адрес назначения процесса
															::memcpy(&awh_cast <net::addr_net_ipv6_t *> (info.addresses.dst.get())->address[0], &awh_cast <net::addr_net_ipv6_t *> (i->second.addresses.dst.get())->address[0], 16);
														} break;
													}
													// Выполняем функцию обратного вызова
													this->_callback(pid, info);
												}
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
			DIR* proc_dir = ::opendir("/proc");
			if (proc_dir != nullptr) {
				struct dirent* proc_entry;
				while ((proc_entry = ::readdir(proc_dir)) != nullptr) {
					if (proc_entry->d_name[0] >= '0' && proc_entry->d_name[0] <= '9') {
						pid_t pid = ::atoi(proc_entry->d_name);
						
						char fd_path[256];
						::snprintf(fd_path, sizeof(fd_path), "/proc/%d/fd", pid);
						
						DIR* fd_dir = ::opendir(fd_path);
						if (fd_dir != nullptr) {
							struct dirent* fd_entry;
							while ((fd_entry = ::readdir(fd_dir)) != nullptr) {
								if (fd_entry->d_name[0] >= '0' && fd_entry->d_name[0] <= '9') {
									char link_path[512];
									::snprintf(link_path, sizeof(link_path), "/proc/%d/fd/%s", pid, fd_entry->d_name);
									
									struct stat st;
									if (::stat(link_path, &st) == 0 && S_ISSOCK(st.st_mode)) {
										int fd = ::open(link_path, O_RDONLY);
										if (fd >= 0) {
											struct sockaddr_storage addr_local, addr_remote;
											socklen_t len_local = sizeof(addr_local);
											socklen_t len_remote = sizeof(addr_remote);
											
											if (::getsockname(fd, reinterpret_cast<struct sockaddr*>(&addr_local), &len_local) == 0) {
												info_t info = info_t();
												::getpeername(fd, reinterpret_cast<struct sockaddr*>(&addr_remote), &len_remote);
												
												int family = addr_local.ss_family;
												if (family == AF_INET) {
													info.family = event::family_t::IPV4;
													auto sin_local = reinterpret_cast<struct sockaddr_in*>(&addr_local);
													auto sin_remote = reinterpret_cast<struct sockaddr_in*>(&addr_remote);
													
													info.ports.src = ntohs(sin_local->sin_port);
													if (len_remote > 0 && addr_remote.ss_family == AF_INET) {
														info.ports.dst = ntohs(sin_remote->sin_port);
													}
													
													info.addresses.src = std::make_unique<net::addr_net_ipv4_t>();
													awh_cast<net::addr_net_ipv4_t*>(info.addresses.src.get())->address = sin_local->sin_addr.s_addr;
													if (len_remote > 0 && addr_remote.ss_family == AF_INET) {
														info.addresses.dst = std::make_unique<net::addr_net_ipv4_t>();
														awh_cast<net::addr_net_ipv4_t*>(info.addresses.dst.get())->address = sin_remote->sin_addr.s_addr;
													}
												} else if (family == AF_INET6) {
													info.family = event::family_t::IPV6;
													auto sin6_local = reinterpret_cast<struct sockaddr_in6*>(&addr_local);
													auto sin6_remote = reinterpret_cast<struct sockaddr_in6*>(&addr_remote);
													
													info.ports.src = ntohs(sin6_local->sin6_port);
													if (len_remote > 0 && addr_remote.ss_family == AF_INET6) {
														info.ports.dst = ntohs(sin6_remote->sin6_port);
													}
													
													info.addresses.src = std::make_unique<net::addr_net_ipv6_t>();
													::memcpy(&awh_cast<net::addr_net_ipv6_t*>(info.addresses.src.get())->address[0], &sin6_local->sin6_addr, 16);
													if (len_remote > 0 && addr_remote.ss_family == AF_INET6) {
														info.addresses.dst = std::make_unique<net::addr_net_ipv6_t>();
														::memcpy(&awh_cast<net::addr_net_ipv6_t*>(info.addresses.dst.get())->address[0], &sin6_remote->sin6_addr, 16);
													}
												} else if (family == AF_UNIX) {
													info.family = event::family_t::UDS;
													auto sun_local = reinterpret_cast<struct sockaddr_un*>(&addr_local);
													auto sun_remote = reinterpret_cast<struct sockaddr_un*>(&addr_remote);
													
													info.addresses.src = std::make_unique<net::addr_fs_t>();
													awh_cast<net::addr_fs_t*>(info.addresses.src.get())->address = sun_local->sun_path;
													
													if (len_remote > 0 && addr_remote.ss_family == AF_UNIX) {
														info.addresses.dst = std::make_unique<net::addr_fs_t>();
														awh_cast<net::addr_fs_t*>(info.addresses.dst.get())->address = sun_remote->sun_path;
													}
												}
												
												int type = 0;
												socklen_t type_len = sizeof(type);
												if (::getsockopt(fd, SOL_SOCKET, SO_TYPE, &type, &type_len) == 0) {
													if (type == SOCK_STREAM) info.protocol = event::protocol_t::TCP;
													else if (type == SOCK_DGRAM) info.protocol = event::protocol_t::UDP;
													else if (type == SOCK_RAW) info.protocol = event::protocol_t::RAW;
												}
												
												if (this->_callback != nullptr) {
													this->_callback(pid, info);
												}
											}
											::close(fd);
										}
									}
								}
							}
							::closedir(fd_dir);
						}
					}
				}
				::closedir(proc_dir);
			}
		/**
		 * Для операционной системы MacOS X
		 */
		#elif __APPLE__ || __MACH__
			// Список идентификаторов процессов
			pid_t pids[0x1000];
			// Получаем список идентификаторов процессов
			const int32_t count = ::proc_listpids(PROC_ALL_PIDS, 0, pids, sizeof(pid_t) * 0x1000);
			// Если список идентификаторов процессов получен
			if(count < 0){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
			// Если список идентификаторов процессов получен
			} else if(count > 0) {
				// Объект информации о процессе
				info_t info;
				// Идентификатор процесса
				pid_t pid = 0;
				// Общее и актуальное количество файловых дескрипторов процесса
				int32_t fds = 0, actual = 0;
				// Список файловых дескрипторов процесса
				struct proc_fdinfo fdinfo[0x1000];
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
					// Получаем список актуальных файловых дескрипторов процесса
					actual = ::proc_pidinfo(pid, PROC_PIDLISTFDS, 0, fdinfo, fds * sizeof(struct proc_fdinfo));
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
								struct socket_fdinfo si{0};
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
											 * Определяем семейстов IP-адресов сокета
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
											 * Определяем семейстов IP-адресов сокета
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
											info.ports.src = ntohs(0);
											// Устанавливаем целевой порт сокета
											info.ports.dst = ntohs(0);
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
			int mib[3] = { CTL_KERN, KERN_PROC, KERN_PROC_PROC };
			size_t len = 0;
			// Узнаем размер необходимых данных для kinfo_proc
			if (::sysctl(mib, 3, nullptr, &len, nullptr, 0) == 0 && len > 0) {
				// Выделяем память под процессы
				auto procs = std::make_unique<struct kinfo_proc[]>(len / sizeof(struct kinfo_proc));
				// Получаем список всех процессов
				if (::sysctl(mib, 3, procs.get(), &len, nullptr, 0) == 0) {
					const size_t count = len / sizeof(struct kinfo_proc);
					info_t info;
					
					for (size_t i = 0; i < count; ++i) {
						pid_t pid = procs[i].ki_pid;
						
						// Запрашиваем файловые дескрипторы для конкретного процесса
						int fmib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_FILEDESC, pid };
						size_t flen = 0;
						
						if (::sysctl(fmib, 4, nullptr, &flen, nullptr, 0) == 0 && flen > 0) {
							auto files = std::make_unique<char[]>(flen);
							if (::sysctl(fmib, 4, files.get(), &flen, nullptr, 0) == 0) {
								// Проходим по всем файлам процесса
								char* ptr = files.get();
								char* end = ptr + flen;
								
								while (ptr < end) {
									struct kinfo_file* kf = reinterpret_cast<struct kinfo_file*>(ptr);
									if (kf->kf_structsize == 0) break;
									
									// Если файл оказался сокетом
									if (kf->kf_type == KF_TYPE_SOCKET && kf->kf_sock_domain >= 0) {
										info = info_t();
										
										// Определяем протокол сокета
										switch (kf->kf_sock_protocol) {
											case IPPROTO_IP:     info.family = event::family_t::IPV4; break;
											case IPPROTO_RAW:    info.protocol = event::protocol_t::RAW; break;
											case IPPROTO_TCP:    info.protocol = event::protocol_t::TCP; break;
											case IPPROTO_UDP:    info.protocol = event::protocol_t::UDP; break;
											case IPPROTO_ICMP:   info.protocol = event::protocol_t::ICMP; break;
											case IPPROTO_IGMP:   info.protocol = event::protocol_t::IGMP; break;
											case IPPROTO_SCTP:   info.protocol = event::protocol_t::SCTP; break;
											case IPPROTO_IPV6:   info.family = event::family_t::IPV6; break;
											case IPPROTO_ICMPV6: info.protocol = event::protocol_t::ICMP; break;
											default:             info.protocol = event::protocol_t::NONE;
										}

										// Определяем семейство
										if (kf->kf_sock_domain == AF_INET) {
											info.family = event::family_t::IPV4;
										} else if (kf->kf_sock_domain == AF_INET6) {
											info.family = event::family_t::IPV6;
										} else if (kf->kf_sock_domain == AF_UNIX) {
											info.family = event::family_t::UDS;
										}

										// Обработка адресов
										auto src_sa = reinterpret_cast<struct sockaddr*>(&kf->kf_sa_local);
										auto dst_sa = reinterpret_cast<struct sockaddr*>(&kf->kf_sa_peer);
										
										if (info.family == event::family_t::IPV4 && src_sa->sa_family == AF_INET) {
											auto src_in = reinterpret_cast<struct sockaddr_in*>(src_sa);
											auto dst_in = reinterpret_cast<struct sockaddr_in*>(dst_sa);
											
											info.ports.src = ntohs(src_in->sin_port);
											info.ports.dst = dst_in ? ntohs(dst_in->sin_port) : 0;
											
											info.addresses.src = std::make_unique<net::addr_net_ipv4_t>();
											info.addresses.dst = std::make_unique<net::addr_net_ipv4_t>();
											
											awh_cast<net::addr_net_ipv4_t*>(info.addresses.src.get())->address = src_in->sin_addr.s_addr;
											if (dst_in) {
												awh_cast<net::addr_net_ipv4_t*>(info.addresses.dst.get())->address = dst_in->sin_addr.s_addr;
											}
										} else if (info.family == event::family_t::IPV6 && src_sa->sa_family == AF_INET6) {
											auto src_in6 = reinterpret_cast<struct sockaddr_in6*>(src_sa);
											auto dst_in6 = reinterpret_cast<struct sockaddr_in6*>(dst_sa);
											
											info.ports.src = ntohs(src_in6->sin6_port);
											info.ports.dst = dst_in6 ? ntohs(dst_in6->sin6_port) : 0;
											
											info.addresses.src = std::make_unique<net::addr_net_ipv6_t>();
											info.addresses.dst = std::make_unique<net::addr_net_ipv6_t>();
											
											::memcpy(&awh_cast<net::addr_net_ipv6_t*>(info.addresses.src.get())->address[0], &src_in6->sin6_addr, 16);
											if (dst_in6) {
												::memcpy(&awh_cast<net::addr_net_ipv6_t*>(info.addresses.dst.get())->address[0], &dst_in6->sin6_addr, 16);
											}
										} else if (info.family == event::family_t::UDS && src_sa->sa_family == AF_UNIX) {
											auto src_un = reinterpret_cast<struct sockaddr_un*>(src_sa);
											auto dst_un = reinterpret_cast<struct sockaddr_un*>(dst_sa);
											
											info.addresses.src = std::make_unique<net::addr_fs_t>();
											info.addresses.dst = std::make_unique<net::addr_fs_t>();
											
											awh_cast<net::addr_fs_t*>(info.addresses.src.get())->address = src_un->sun_path;
											if (dst_un) {
												awh_cast<net::addr_fs_t*>(info.addresses.dst.get())->address = dst_un->sun_path;
											}
										}

										// Вызываем callback
										if (this->_callback != nullptr) {
											this->_callback(pid, info);
										}
									}
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
			if (this->_callback != nullptr) {
				// В NetBSD и OpenBSD системные структуры для сокетов часто закрыты или требуют libkvm и прав root.
				// Надежный способ из пространства пользователя — парсинг вывода штатных системных утилит (sockstat/fstat)
				
				/**
				 * Для операционной системы NetBSD
				 */
				#if defined(__NetBSD__)
					FILE* fp = ::popen("sockstat -n 2>/dev/null", "r");
					if (fp) {
						char line[1024];
						if (::fgets(line, sizeof(line), fp)) {
							while (::fgets(line, sizeof(line), fp)) {
								std::istringstream iss(line);
								std::string user, cmd, pid_str, fd_str, proto, local, foreign;
								// FORMAT: USER COMMAND PID FD PROTO LOCAL FOREIGN
								if (iss >> user >> cmd >> pid_str >> fd_str >> proto >> local) {
									pid_t pid = ::atoi(pid_str.c_str());
									info_t info = info_t();
									iss >> foreign;
									
									if (proto == "tcp" || proto == "udp") {
										info.protocol = (proto == "tcp") ? event::protocol_t::TCP : event::protocol_t::UDP;
										
										auto parse_addr = [&info](const std::string& addr_port, bool is_src) {
											if (addr_port == "*.*" || addr_port.empty()) return;
											auto pos = addr_port.find_last_of('.');
											if (pos != std::string::npos && pos > 0) {
												std::string ip = addr_port.substr(0, pos);
												uint16_t port = static_cast<uint16_t>(::atoi(addr_port.substr(pos + 1).c_str()));
												
												if (is_src) info.ports.src = port;
												else info.ports.dst = port;
												
												if (ip.find(':') != std::string::npos) {
													info.family = event::family_t::IPV6;
													auto a = std::make_unique<net::addr_net_ipv6_t>();
													::inet_pton(AF_INET6, ip.c_str(), &a->address);
													if (is_src) info.addresses.src = std::move(a);
													else info.addresses.dst = std::move(a);
												} else {
													info.family = event::family_t::IPV4;
													auto a = std::make_unique<net::addr_net_ipv4_t>();
													::inet_pton(AF_INET, ip.c_str(), &a->address);
													if (is_src) info.addresses.src = std::move(a);
													else info.addresses.dst = std::move(a);
												}
											}
										};
										
										parse_addr(local, true);
										parse_addr(foreign, false);
										if (info.family != event::family_t::NONE) {
											this->_callback(pid, info);
										}
										
									} else if (local == "local" || proto == "local") {
										info.family = event::family_t::UDS;
										info.protocol = event::protocol_t::NONE;
										info.addresses.src = std::make_unique<net::addr_fs_t>();
										
										std::string path = foreign;
										std::string rest;
										std::getline(iss, rest);
										if (!rest.empty()) path += rest;
										
										if (!path.empty()) {
											awh_cast<net::addr_fs_t*>(info.addresses.src.get())->address = path;
											this->_callback(pid, info);
										}
									}
								}
							}
						}
						::pclose(fp);
					}
				/**
				 * Для операционной системы OpenBSD
				 */
				#elif defined(__OpenBSD__)
					FILE* fp = ::popen("fstat -n 2>/dev/null", "r");
					if (fp) {
						char line[1024];
						if (::fgets(line, sizeof(line), fp)) {
							while (::fgets(line, sizeof(line), fp)) {
								std::istringstream iss(line);
								std::string user, cmd, pid_str, fd_str, mount, mode, proto, sz_dv, local, arrow, foreign;
								
								// FORMAT: USER CMD PID FD MOUNT INUM MODE SZ|DV R/W [LOCAL] [<-> FOREIGN]
								if (iss >> user >> cmd >> pid_str >> fd_str >> mount) {
									pid_t pid = ::atoi(pid_str.c_str());
									info_t info = info_t();
									
									if (mount == "internet") {
										iss >> mode >> proto >> sz_dv >> local;
										
										info.protocol = (proto == "tcp") ? event::protocol_t::TCP : 
														(proto == "udp") ? event::protocol_t::UDP : event::protocol_t::NONE;
										
										if (iss >> arrow) {
											if (arrow == "<->") iss >> foreign;
											else foreign = arrow;
										}
										
										auto parse_addr = [&info](const std::string& addr_port, bool is_src) {
											if (addr_port == "*:*" || addr_port.empty() || addr_port.find("*:") == 0) return;
											auto pos = addr_port.find_last_of(':'); 
											if (pos != std::string::npos && pos > 0) {
												std::string ip = addr_port.substr(0, pos);
												uint16_t port = static_cast<uint16_t>(::atoi(addr_port.substr(pos + 1).c_str()));
												
												if (ip.front() == '[' && ip.back() == ']') ip = ip.substr(1, ip.size() - 2);
												
												if (is_src) info.ports.src = port;
												else info.ports.dst = port;
												
												if (ip.find(':') != std::string::npos) {
													info.family = event::family_t::IPV6;
													auto a = std::make_unique<net::addr_net_ipv6_t>();
													::inet_pton(AF_INET6, ip.c_str(), &a->address);
													if (is_src) info.addresses.src = std::move(a);
													else info.addresses.dst = std::move(a);
												} else {
													info.family = event::family_t::IPV4;
													auto a = std::make_unique<net::addr_net_ipv4_t>();
													::inet_pton(AF_INET, ip.c_str(), &a->address);
													if (is_src) info.addresses.src = std::move(a);
													else info.addresses.dst = std::move(a);
												}
											}
										};
										
										parse_addr(local, true);
										parse_addr(foreign, false);
										if (info.family != event::family_t::NONE) {
											this->_callback(pid, info);
										}
										
									} else if (mount == "unix") {
										iss >> mode >> sz_dv >> local;
										std::string path;
										std::vector<std::string> parts;
										parts.push_back(local);
										std::string part;
										while (iss >> part) parts.push_back(part);
										
										for (auto it = parts.rbegin(); it != parts.rend(); ++it) {
											if ((*it).find("0x") != 0 && *it != "dgram" && *it != "stream") {
												path = *it;
												break;
											}
										}
										
										if (!path.empty()) {
											info.family = event::family_t::UDS;
											info.protocol = event::protocol_t::NONE;
											info.addresses.src = std::make_unique<net::addr_fs_t>();
											awh_cast<net::addr_fs_t*>(info.addresses.src.get())->address = path;
											this->_callback(pid, info);
										}
									}
								}
							}
						}
						::pclose(fp);
					}
				#endif
			}
		/**
		 * Для операционной системы MS Windows
		 */
		#elif _WIN32 || _WIN64
			if (this->_callback != nullptr) {
				ULONG size = 0;
				// TCP IPv4
				if (::GetExtendedTcpTable(nullptr, &size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0) == ERROR_INSUFFICIENT_BUFFER) {
					auto table = std::make_unique<uint8_t[]>(size);
					if (::GetExtendedTcpTable(table.get(), &size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0) == NO_ERROR) {
						auto* pTcpTable = reinterpret_cast<PMIB_TCPTABLE_OWNER_PID>(table.get());
						for (DWORD i = 0; i < pTcpTable->dwNumEntries; i++) {
							info_t info = info_t();
							info.family = event::family_t::IPV4;
							info.protocol = event::protocol_t::TCP;
							info.ports.src = ntohs(static_cast<uint16_t>(pTcpTable->table[i].dwLocalPort));
							info.ports.dst = ntohs(static_cast<uint16_t>(pTcpTable->table[i].dwRemotePort));
							
							info.addresses.src = std::make_unique<net::addr_net_ipv4_t>();
							info.addresses.dst = std::make_unique<net::addr_net_ipv4_t>();
							awh_cast<net::addr_net_ipv4_t*>(info.addresses.src.get())->address = pTcpTable->table[i].dwLocalAddr;
							awh_cast<net::addr_net_ipv4_t*>(info.addresses.dst.get())->address = pTcpTable->table[i].dwRemoteAddr;
							
							this->_callback(pTcpTable->table[i].dwOwningPid, info);
						}
					}
				}
				
				// TCP IPv6
				size = 0;
				if (::GetExtendedTcpTable(nullptr, &size, FALSE, AF_INET6, TCP_TABLE_OWNER_PID_ALL, 0) == ERROR_INSUFFICIENT_BUFFER) {
					auto table = std::make_unique<uint8_t[]>(size);
					if (::GetExtendedTcpTable(table.get(), &size, FALSE, AF_INET6, TCP_TABLE_OWNER_PID_ALL, 0) == NO_ERROR) {
						auto* pTcp6Table = reinterpret_cast<PMIB_TCP6TABLE_OWNER_PID>(table.get());
						for (DWORD i = 0; i < pTcp6Table->dwNumEntries; i++) {
							info_t info = info_t();
							info.family = event::family_t::IPV6;
							info.protocol = event::protocol_t::TCP;
							info.ports.src = ntohs(static_cast<uint16_t>(pTcp6Table->table[i].dwLocalPort));
							info.ports.dst = ntohs(static_cast<uint16_t>(pTcp6Table->table[i].dwRemotePort));
							
							info.addresses.src = std::make_unique<net::addr_net_ipv6_t>();
							info.addresses.dst = std::make_unique<net::addr_net_ipv6_t>();
							::memcpy(&awh_cast<net::addr_net_ipv6_t*>(info.addresses.src.get())->address[0], pTcp6Table->table[i].ucLocalAddr, 16);
							::memcpy(&awh_cast<net::addr_net_ipv6_t*>(info.addresses.dst.get())->address[0], pTcp6Table->table[i].ucRemoteAddr, 16);
							
							this->_callback(pTcp6Table->table[i].dwOwningPid, info);
						}
					}
				}
				
				// UDP IPv4
				size = 0;
				if (::GetExtendedUdpTable(nullptr, &size, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0) == ERROR_INSUFFICIENT_BUFFER) {
					auto table = std::make_unique<uint8_t[]>(size);
					if (::GetExtendedUdpTable(table.get(), &size, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0) == NO_ERROR) {
						auto* pUdpTable = reinterpret_cast<PMIB_UDPTABLE_OWNER_PID>(table.get());
						for (DWORD i = 0; i < pUdpTable->dwNumEntries; i++) {
							info_t info = info_t();
							info.family = event::family_t::IPV4;
							info.protocol = event::protocol_t::UDP;
							info.ports.src = ntohs(static_cast<uint16_t>(pUdpTable->table[i].dwLocalPort));
							info.ports.dst = 0;
							
							info.addresses.src = std::make_unique<net::addr_net_ipv4_t>();
							info.addresses.dst = std::make_unique<net::addr_net_ipv4_t>();
							awh_cast<net::addr_net_ipv4_t*>(info.addresses.src.get())->address = pUdpTable->table[i].dwLocalAddr;
							
							this->_callback(pUdpTable->table[i].dwOwningPid, info);
						}
					}
				}
				
				// UDP IPv6
				size = 0;
				if (::GetExtendedUdpTable(nullptr, &size, FALSE, AF_INET6, UDP_TABLE_OWNER_PID, 0) == ERROR_INSUFFICIENT_BUFFER) {
					auto table = std::make_unique<uint8_t[]>(size);
					if (::GetExtendedUdpTable(table.get(), &size, FALSE, AF_INET6, UDP_TABLE_OWNER_PID, 0) == NO_ERROR) {
						auto* pUdp6Table = reinterpret_cast<PMIB_UDP6TABLE_OWNER_PID>(table.get());
						for (DWORD i = 0; i < pUdp6Table->dwNumEntries; i++) {
							info_t info = info_t();
							info.family = event::family_t::IPV6;
							info.protocol = event::protocol_t::UDP;
							info.ports.src = ntohs(static_cast<uint16_t>(pUdp6Table->table[i].dwLocalPort));
							info.ports.dst = 0;
							
							info.addresses.src = std::make_unique<net::addr_net_ipv6_t>();
							info.addresses.dst = std::make_unique<net::addr_net_ipv6_t>();
							::memcpy(&awh_cast<net::addr_net_ipv6_t*>(info.addresses.src.get())->address[0], pUdp6Table->table[i].ucLocalAddr, 16);
							
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод получения названия приложения по идентификатору процесса
 *
 * @param pid идентификатор процесса
 * @return    название приложения которому принадлежит процесс
 */
string awh::Process_Resolver::name(const pid_t pid) const noexcept {
	// Результат работы функции
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
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки функции обратного вызова для получения информации о процессе
 *
 * @param callback функция обратного вызова
 */
void awh::Process_Resolver::on(function <void (const pid_t, const info_t &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback = ::move(callback);
}
/**
 * @brief Конструктор
 *
 * @param log объект для работы с логами
 */
awh::Process_Resolver::Process_Resolver(const log_t * log) noexcept : _callback(nullptr), _log(log) {}
/**
 * @brief Деструктор
 *
 */
awh::Process_Resolver::~Process_Resolver() noexcept {}
