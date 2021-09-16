/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_FS__
#define __AWH_FS__

/**
 * Стандартная библиотека
 */
#include <string>
#include <fstream>
#include <codecvt>
#include <sstream>
#include <unistd.h>
#include <functional>
#include <stdlib.h>
#include <dirent.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
// Если это clang v10 или выше
#if defined(__AWH_EXPERIMENTAL__)
	#include <filesystem>
#endif

// Если - это Windows
#if defined(_WIN32) || defined(_WIN64)
	#include <conio.h>
	#include <direct.h>
// Если - это Unix
#else
	#include <grp.h>
	#include <pwd.h>
	#include <sys/mman.h>
#endif

/**
 * Наши модули
 */
#include <fmk.hpp>

// Устанавливаем область видимости
using namespace std;

/*
 * awh пространство имён
 */
namespace awh {
	/**
	 * FS Класс модуля работы с каталогами
	 */
	typedef struct FS {
		/**
		 * Типы файловой системы
		 */
		enum class type_t : u_short {NONE, DIR, FILE, SOCKET};
		/**
		 * file Функция извлечения названия и расширения файла
		 * @param filename адрес файла для извлечения его параметров
		 */
		static const pair <string, string> file(const string & filename) noexcept {
			// Результат работы функции
			pair <string, string> result;
			// Если файл передан
			if(!filename.empty()){
				// Позиция разделителя каталога
				size_t pos = 0;
				// Выполняем поиск разделителя каталога
				if((pos = filename.rfind(FS_SEPARATOR)) != string::npos){
					// Извлекаем имя файла
					const string & name = filename.substr(pos + 1);
					// Ищем расширение файла
					if((pos = name.rfind(".")) != string::npos){
						// Устанавливаем имя файла
						result.first = name.substr(0, pos);
						// Устанавливаем расширение файла
						result.second = name.substr(pos + 1);
					// Устанавливаем только имя файла
					} else result.first = move(name);
				}
			}
			// Выводим результат
			return result;
		}
		/**
		 * rmdir Функция удаления каталога и всего содержимого
		 * @param path путь до каталога
		 * @param fmk  объект фреймворка для работы
		 * @return     количество дочерних элементов
		 */
		static const int rmdir(const string & path, const fmk_t * fmk) noexcept {
			// Результат работы функции
			int result = -1;
			// Если путь передан
			if(!path.empty() && (fmk != nullptr)){
				// Если каталог существует
				if(isdir(path)){
					// Открываем указанный каталог
					DIR * dir = opendir(path.c_str());
					// Если каталог открыт
					if(dir != nullptr){
						// Устанавливаем количество дочерних элементов
						result = 0;
						// Создаем указатель на содержимое каталога
						struct dirent * ptr = nullptr;
						// Выполняем чтение содержимого каталога
						while(!result && (ptr = readdir(dir))){
							// Количество найденных элементов
							int res = -1;
							// Создаем структуру буфера статистики
							struct stat statbuf;
							// Пропускаем названия текущие "." и внешние "..", так как идет рекурсия
							if(!strcmp(ptr->d_name, ".") || !strcmp(ptr->d_name, "..")) continue;
							// Получаем адрес каталога
							const string & dirname = fmk->format("%s%s%s", path.c_str(), FS_SEPARATOR, ptr->d_name);
							// Если статистика извлечена
							if(!stat(dirname.c_str(), &statbuf)){
								// Если дочерний элемент является дирректорией
								if(S_ISDIR(statbuf.st_mode)) res = rmdir(dirname, fmk);
								// Если дочерний элемент является файлом то удаляем его
								else res = ::unlink(dirname.c_str());
							}
							// Запоминаем количество дочерних элементов
							result = res;
						}
						// Закрываем открытый каталог
						closedir(dir);
					}
					// Удаляем последний каталог
					if(!result) result = ::rmdir(path.c_str());
				// Выводим сообщение об ошибке
				} else fmk->log("the path name: \"%s\" is not found", fmk_t::log_t::CRITICAL, nullptr, path.c_str());
			}
			// Выводим результат
			return result;
		}
// Если - это не Windows
#if !defined(_WIN32) && !defined(_WIN64)
		/**
		 * mkdir Функция рекурсивного создания каталогов
		 * @param path адрес каталогов
		 */
		static void mkdir(const string & path) noexcept {
			// Если путь передан
			if(!path.empty()){
				// Буфер с названием каталога
				char tmp[256];
				// Указатель на сепаратор
				char * p = nullptr;
				// Получаем сепаратор
				const char sep = FS_SEPARATOR[0];
				// Копируем переданный адрес в буфер
				snprintf(tmp, sizeof(tmp), "%s", path.c_str());
				// Определяем размер адреса
				size_t len = strlen(tmp);
				// Если последний символ является сепаратором тогда удаляем его
				if(tmp[len - 1] == sep) tmp[len - 1] = 0;
				// Переходим по всем символам
				for(p = tmp + 1; * p; p++){
					// Если найден сепаратор
					if(* p == sep){
						// Сбрасываем указатель
						* p = 0;
						// Создаем каталог
						::mkdir(tmp, S_IRWXU);
						// Запоминаем сепаратор
						* p = sep;
					}
				}
				// Создаем последний каталог
				::mkdir(tmp, S_IRWXU);
			}
		}
		/**
		 * getuid Функция вывода идентификатора пользователя
		 * @param  name имя пользователя
		 * @return      полученный идентификатор пользователя
		 */
		static const uid_t getuid(const string & name) noexcept {
			// Получаем идентификатор имени пользователя
			struct passwd * pwd = getpwnam(name.c_str());
			// Если идентификатор пользователя не найден
			if(pwd == nullptr){
				// Выводим сообщение об ошибке
				printf("failed to get userId from username [%s]\n", name.c_str());
				// Сообщаем что ничего не найдено
				return 0;
			}
			// Выводим идентификатор пользователя
			return pwd->pw_uid;
		}
		/**
		 * getgid Функция вывода идентификатора группы пользователя
		 * @param  name название группы пользователя
		 * @return      полученный идентификатор группы пользователя
		 */
		static const gid_t getgid(const string & name) noexcept {
			// Получаем идентификатор группы пользователя
			struct group * grp = getgrnam(name.c_str());
			// Если идентификатор группы не найден
			if(grp == nullptr){
				// Выводим сообщение об ошибке
				printf("failed to get groupId from groupname [%s]\n", name.c_str());
				// Сообщаем что ничего не найдено
				return 0;
			}
			// Выводим идентификатор группы пользователя
			return grp->gr_gid;
		}
		/**
		 * chown Функция установки владельца на каталог
		 * @param path  путь к файлу или каталогу для установки владельца
		 * @param user  данные пользователя
		 * @param group идентификатор группы
		 */
		static void chown(const string & path, const string & user, const string & group) noexcept {
			// Если путь передан
			if(!path.empty() && !user.empty() && !group.empty() && isfile(path)){
				// Идентификатор пользователя
				uid_t uid = getuid(user);
				// Идентификатор группы
				gid_t gid = getgid(group);
				// Устанавливаем права на каталог
				if(uid && gid) ::chown(path.c_str(), uid, gid);
			}
		}
		/**
		 * makedir Функция создания каталога для хранения логов
		 * @param  path  адрес для каталога
		 * @param  user  данные пользователя
		 * @param  group идентификатор группы
		 * @return       результат создания каталога
		 */
		static const bool makedir(const string & path, const string & user, const string & group) noexcept {
			// Проверяем существует ли нужный нам каталог
			if(!isdir(path)){
				// Создаем каталог
				mkdir(path);
				// Устанавливаем права на каталог
				chown(path, user, group);
				// Сообщаем что все удачно
				return true;
			}
			// Сообщаем что каталог и так существует
			return false;
		}
#else
		/**
		 * mkdir Функция рекурсивного создания каталогов
		 * @param path адрес каталогов
		 */
		static void mkdir(const string & path) noexcept {
			// Если путь передан
			if(!path.empty()){
				// Буфер с названием каталога
				char tmp[256];
				// Указатель на сепаратор
				char * p = nullptr;
				// Получаем сепаратор
				const char sep = FS_SEPARATOR[0];
				// Копируем переданный адрес в буфер
				snprintf(tmp, sizeof(tmp), "%s", path.c_str());
				// Определяем размер адреса
				size_t len = strlen(tmp);
				// Если последний символ является сепаратором тогда удаляем его
				if(tmp[len - 1] == sep) tmp[len - 1] = 0;
				// Переходим по всем символам
				for(p = tmp + 1; * p; p++){
					// Если найден сепаратор
					if(* p == sep){
						// Сбрасываем указатель
						* p = 0;
						// Создаем каталог
						_mkdir(tmp);
						// Запоминаем сепаратор
						* p = sep;
					}
				}
				// Создаем последний каталог
				_mkdir(tmp);
			}
		}
		/**
		 * makedir Функция создания каталога для хранения логов
		 * @param path адрес для каталога
		 * @return     результат создания каталога
		 */
		static const bool makedir(const string & path) noexcept {
			// Проверяем существует ли нужный нам каталог
			if(!isdir(path)){
				// Создаем каталог
				mkdir(path);
				// Сообщаем что все удачно
				return true;
			}
			// Сообщаем что каталог и так существует
			return false;
		}
#endif
		/**
		 * istype Функция определяющая тип файловой системы по адресу
		 * @param  name адрес дирректории
		 * @return      тип файловой системы
		 */
		static const type_t istype(const string & name) noexcept {
			// Результат работы функции
			type_t result = type_t::NONE;
			// Если адрес дирректории передан
			if(!name.empty()){
				// Структура проверка статистики
				struct stat info;
				// Если тип определён
				if(stat(name.c_str(), &info) == 0){
					// Если это каталог
					if((info.st_mode & S_IFDIR) != 0) result = type_t::DIR;
					// Если это файл
					else if((info.st_mode & S_IFMT) != 0) result = type_t::FILE;
// Если - это не Windows
#if !defined(_WIN32) && !defined(_WIN64)
					// Если это сокет
					else if((info.st_mode & S_IFSOCK) != 0) result = type_t::SOCKET;
#endif
				}
			}
			// Выводим результат
			return result;
		}
		/**
		 * isdir Функция проверяющий существование дирректории
		 * @param  name адрес дирректории
		 * @return      результат проверки
		 */
		static const bool isdir(const string & name) noexcept {
			// Выводим результат
			return (istype(name) == type_t::DIR);
		}
		/**
		 * isfile Функция проверяющий существование файла
		 * @param  name адрес файла
		 * @return      результат проверки
		 */
		static const bool isfile(const string & name) noexcept {
			// Выводим результат
			return (istype(name) == type_t::FILE);
		}
		/**
		 * issock Функция проверки существования сокета
		 * @param  name адрес сокета
		 * @return      результат проверки
		 */
		static const bool issock(const string & name) noexcept {
			// Выводим результат
			return (istype(name) == type_t::SOCKET);
		}
		/**
		 * fsize Функция подсчёта размера файла
		 * @param filename адрес файла для проверки
		 * @return         размер файла в файловой системе
		 */
		static const uintmax_t fsize(const string & filename) noexcept {
			// Результат работы функции
			uintmax_t result = 0;
			// Если адрес файла передан верный
			if(!filename.empty() && isfile(filename)){
				// Открываем файл на чтение
				ifstream file(filename, ios::in);
				// Если файл открыт
				if(file.is_open()){
					// Перемещаем указатель в конец файла
					file.seekg(0, file.end);
					// Определяем размер файла
					result = file.tellg();
					// Возвращаем указатель обратно
					file.seekg(0, file.beg);
					// Закрываем файл
					file.close();
				}
			}
			// Выводим результат
			return result;
		}
// Если это clang v10 или выше
#if defined(__AWH_EXPERIMENTAL__)
		/**
		 * fcount Функция подсчёта количество файлов в каталоге
		 * @param path путь для подсчёта
		 * @param ext  расширение файла по которому идет фильтрация
		 * @return     количество файлов в каталоге
		 */
		static const uintmax_t fcount(const string & path, const string & ext) noexcept {
			// Результат работы функции
			uintmax_t result = 0;
			// Если адрес каталога и расширение файлов переданы
			if(!path.empty() && !ext.empty() && isdir(path)){
				// Устанавливаем область видимости
				namespace fs = std::__fs::filesystem;
				// Устанавливаем путь поиска
				fs::path dirpath = path;
				// Выполняем рекурсивный переход по всем подкаталогам
				for(auto && entry : fs::recursive_directory_iterator(dirpath, fs::directory_options::skip_permission_denied)){
					// Возможная ошибка
					error_code ignore_error;
					// Запрашиваем файл из каталога и считаем его размер
					if(fs::is_regular_file(fs::symlink_status(entry, ignore_error))){
						// Получаем путь файла
						fs::path fsp = entry.path();
						// Если расширение файла соответствует
						if(string(fsp.extension().c_str()).rfind(ext) != string::npos) result++;
					}
				}
			}
			// Выводим результат
			return result;
		}
		/**
		 * dsize Функция подсчёта размера каталога
		 * @param path путь для подсчёта
		 * @param ext  расширение файла по которому идет фильтрация
		 * @return     размер каталога в байтах
		 */
		static const uintmax_t dsize(const string & path, const string & ext) noexcept {
			// Результат работы функции
			uintmax_t result = 0;
			// Если адрес каталога и расширение файлов переданы
			if(!path.empty() && !ext.empty() && isdir(path)){
				// Устанавливаем область видимости
				namespace fs = std::__fs::filesystem;
				// Устанавливаем путь поиска
				fs::path dirpath = path;
				// Выполняем рекурсивный переход по всем подкаталогам
				for(auto && entry : fs::recursive_directory_iterator(dirpath, fs::directory_options::skip_permission_denied)){
					// Возможная ошибка
					error_code ignore_error;
					// Запрашиваем файл из каталога и считаем его размер
					if(fs::is_regular_file(fs::symlink_status(entry, ignore_error))){
						// Получаем путь файла
						fs::path fsp = entry.path();
						// Если расширение файла соответствует
						if(string(fsp.extension().c_str()).rfind(ext) != string::npos) result += fs::file_size(entry);
					}
				}
			}
			// Выводим результат
			return result;
		}
// Если это gcc
#else
		/**
		 * fcount Функция подсчёта количество файлов в каталоге
		 * @param path путь для подсчёта
		 * @param ext  расширение файла по которому идет фильтрация
		 * @param fmk  объект фреймворка для работы
		 * @return     количество файлов в каталоге
		 */
		static const uintmax_t fcount(const string & path, const string & ext, const fmk_t * fmk) noexcept {
			// Результат работы функции
			uintmax_t result = 0;
			// Если адрес каталога и расширение файлов переданы
			if(!path.empty() && !ext.empty() && (fmk != nullptr)){
				// Если каталог существует
				if(isdir(path)){
					// Открываем указанный каталог
					DIR * dir = opendir(path.c_str());
					// Если каталог открыт
					if(dir != nullptr){
						// Структура проверка статистики
						struct stat info;
						// Создаем указатель на содержимое каталога
						struct dirent * ptr = nullptr;
						// Выполняем чтение содержимого каталога
						while((ptr = readdir(dir))){
							// Пропускаем названия текущие "." и внешние "..", так как идет рекурсия
							if(!strcmp(ptr->d_name, ".") || !strcmp(ptr->d_name, "..")) continue;
							// Получаем адрес в виде строки
							const string & address = fmk->format("%s%s%s", path.c_str(), FS_SEPARATOR, ptr->d_name);
							// Если статистика извлечена
							if(!stat(address.c_str(), &info)){
								// Если дочерний элемент является дирректорией
								if(S_ISDIR(info.st_mode)) result += fcount(address, ext, fmk);
								// Если дочерний элемент является файлом то удаляем его
								else {
									// Получаем расширение файла
									const string & extension = fmk->format(".%s", ext.c_str());
									// Получаем длину адреса
									const size_t length = extension.length();
									// Если расширение файла найдено
									if(address.substr(address.length() - length, length).compare(extension) == 0){
										// Получаем количество файлов в каталоге
										result++;
									}
								}
							}
						}
						// Закрываем открытый каталог
						closedir(dir);
					}
				// Выводим сообщение об ошибке
				} else fmk->log("the path name: \"%s\" is not found", fmk_t::log_t::CRITICAL, nullptr, path.c_str());
			}
			// Выводим результат
			return result;
		}
		/**
		 * dsize Функция подсчёта размера каталога
		 * @param path путь для подсчёта
		 * @param ext  расширение файла по которому идет фильтрация
		 * @param fmk  объект фреймворка для работы
		 * @return     размер каталога в байтах
		 */
		static const uintmax_t dsize(const string & path, const string & ext, const fmk_t * fmk) noexcept {
			// Результат работы функции
			uintmax_t result = 0;
			// Если адрес каталога и расширение файлов переданы
			if(!path.empty() && !ext.empty() && (fmk != nullptr)){
				// Если каталог существует
				if(isdir(path)){
					// Открываем указанный каталог
					DIR * dir = opendir(path.c_str());
					// Если каталог открыт
					if(dir != nullptr){
						// Структура проверка статистики
						struct stat info;
						// Создаем указатель на содержимое каталога
						struct dirent * ptr = nullptr;
						// Выполняем чтение содержимого каталога
						while((ptr = readdir(dir))){
							// Пропускаем названия текущие "." и внешние "..", так как идет рекурсия
							if(!strcmp(ptr->d_name, ".") || !strcmp(ptr->d_name, "..")) continue;
							// Получаем адрес в виде строки
							const string & address = fmk->format("%s%s%s", path.c_str(), FS_SEPARATOR, ptr->d_name);
							// Если статистика извлечена
							if(!stat(address.c_str(), &info)){
								// Если дочерний элемент является дирректорией
								if(S_ISDIR(info.st_mode)) result += dsize(address, ext, fmk);
								// Если дочерний элемент является файлом то удаляем его
								else {
									// Получаем расширение файла
									const string & extension = fmk->format(".%s", ext.c_str());
									// Получаем длину адреса
									const size_t length = extension.length();
									// Если расширение файла найдено
									if(address.substr(address.length() - length, length).compare(extension) == 0){
										// Получаем размер файла
										result += fsize(address);
									}
								}
							}
						}
						// Закрываем открытый каталог
						closedir(dir);
					}
				// Выводим сообщение об ошибке
				} else fmk->log("the path name: \"%s\" is not found", fmk_t::log_t::CRITICAL, nullptr, path.c_str());
			}
			// Выводим результат
			return result;
		}
#endif
		/**
		 * realPath Метод извлечения реального адреса
		 * @param path путь который нужно определить
		 * @return     полный путь
		 */
		static const string realPath(const string & path) noexcept {
			// Результат работы функции
			string result = "";
			// Если путь передан
			if(!path.empty()){
				// Если - это Windows
				#if defined(_WIN32) || defined(_WIN64)
					// Создаём буфер для полного адреса
					char buffer[_MAX_PATH];
					// Заполняем буфер нулями
					memset(buffer, 0, sizeof(buffer));
					// Выполняем извлечение адресов из переменных окружений
					ExpandEnvironmentStrings(path.c_str(), buffer, ARRAYSIZE(buffer));
					// Устанавливаем результат
					result = buffer;
					// Заполняем буфер нулями
					memset(buffer, 0, sizeof(buffer));
					// Если адрес существует
					if(_fullpath(buffer, result.c_str(), _MAX_PATH) != nullptr) result = buffer;
				// Если - это Unix
				#else
					// Получаем полный адрес
					const char * realPath = realpath(path.c_str(), nullptr);
					// Если адрес существует
					if(realPath != nullptr) result = realPath;
					// Иначе выводим путь так, как он есть
					else result = path;
				#endif
			}
			// Выводим результат
			return result;
		}
		/**
		 * rfile Функция рекурсивного получения всех строк файла
		 * @param filename адрес файла для чтения
		 * @param fmk      объект фреймворка для работы
		 * @param callback функция обратного вызова
		 */
		static void rfile(const string & filename, const fmk_t * fmk, function <void (const string &, const uintmax_t)> callback) noexcept {
			// Если - это Windows
			#if defined(_WIN32) || defined(_WIN64)
				// Вызываем метод рекурсивного получения всех строк файла, старым способом
				rfile2(filename, fmk, callback);
			// Если - это Unix
			#else
				// Если адрес файла передан
				if(!filename.empty() && (fmk != nullptr)){
					// Если файл существует
					if(isfile(filename)){
						// Файловый дескриптор файла
						int fd = -1;
						// Структура статистики файла
						struct stat info;
						// Если файл не открыт
						if((fd = open(filename.c_str(), O_RDONLY)) < 0)
							// Выводим сообщение об ошибке
							fmk->log("the file name: \"%s\" is broken", fmk_t::log_t::CRITICAL, nullptr, filename.c_str());
						// Если файл открыт удачно
						else if(fstat(fd, &info) < 0)
							// Выводим сообщение об ошибке
							fmk->log("the file name: \"%s\" is unknown size", fmk_t::log_t::CRITICAL, nullptr, filename.c_str());
						// Иначе продолжаем
						else {
							// Буфер входящих данных
							void * buffer = nullptr;
							// Выполняем отображение файла в памяти
							if((buffer = mmap(0, info.st_size, PROT_READ, MAP_SHARED, fd, 0)) == MAP_FAILED)
								// Выводим сообщение что прочитать файл не удалось
								fmk->log("the file name: \"%s\" is not read", fmk_t::log_t::CRITICAL, nullptr, filename.c_str());
							// Если файл прочитан удачно
							else if(buffer != nullptr) {
								// Значение текущей и предыдущей буквы
								char letter = 0, old = 0;
								// Смещение в буфере и длина полученной строки
								size_t offset = 0, length = 0;
								// Получаем размер файла
								const uintmax_t size = info.st_size;
								// Переходим по всему буферу
								for(uintmax_t i = 0; i < size; i++){
									// Получаем значение текущей буквы
									letter = reinterpret_cast <char *> (buffer)[i];
									// Если текущая буква является переносом строк
									if((i > 0) && ((letter == '\n') || (i == (size - 1)))){
										// Если предыдущая буква была возвратом каретки, уменьшаем длину строки
										length = ((old == '\r' ? i - 1 : i) - offset);
										// Если это конец файла, корректируем размер последнего байта
										if(length == 0) length = 1;
										// Если длина слова получена, выводим полученную строку
										callback(string((char *) buffer + offset, length), size);
										// Выполняем смещение
										offset = (i + 1);
									}
									// Запоминаем предыдущую букву
									old = letter;
								}
								// Если данные не все прочитаны, выводим как есть
								if((offset == 0) && (size > 0)) callback(realPath(string((char *) buffer, size)), size);
							}
						}
						// Если файл открыт, закрываем его
						if(fd > -1) close(fd);
					// Выводим сообщение об ошибке
					} else fmk->log("the file name: \"%s\" is not found", fmk_t::log_t::CRITICAL, nullptr, filename.c_str());
				}
			#endif
		}
		/**
		 * rfile2 Функция рекурсивного получения всех строк файла (стандартным способом)
		 * @param filename адрес файла для чтения
		 * @param fmk      объект фреймворка для работы
		 * @param callback функция обратного вызова
		 */
		static void rfile2(const string & filename, const fmk_t * fmk, function <void (const string &, const uintmax_t)> callback) noexcept {
			// Если адрес файла передан
			if(!filename.empty() && (fmk != nullptr)){
				// Если файл существует
				if(isfile(filename)){
					// Открываем файл на чтение
					ifstream file(filename, ios::in);
					// Если файл открыт
					if(file.is_open()){
						// Перемещаем указатель в конец файла
						file.seekg(0, file.end);
						// Определяем размер файла
						const size_t size = file.tellg();
						// Возвращаем указатель обратно
						file.seekg(0, file.beg);
						// Устанавливаем размер буфера
						vector <char> buffer(size);
						// Выполняем чтение данных из файла
						file.read(buffer.data(), size);
						// Устанавливаем буфер
						if(!buffer.empty()){
							// Значение текущей и предыдущей буквы
							char letter = 0, old = 0;
							// Смещение в буфере и длина полученной строки
							size_t offset = 0, length = 0;
							// Получаем данные буфера
							const char * data = buffer.data();
							// Получаем размер файла
							const uintmax_t size = buffer.size();
							// Переходим по всему буферу
							for(uintmax_t i = 0; i < size; i++){
								// Получаем значение текущей буквы
								letter = reinterpret_cast <const char *> (data)[i];
								// Если текущая буква является переносом строк
								if((i > 0) && ((letter == '\n') || (i == (size - 1)))){
									// Если предыдущая буква была возвратом каретки, уменьшаем длину строки
									length = ((old == '\r' ? i - 1 : i) - offset);
									// Если это конец файла, корректируем размер последнего байта
									if(length == 0) length = 1;
									// Если длина слова получена, выводим полученную строку
									callback(string((char *) data + offset, length), size);
									// Выполняем смещение
									offset = (i + 1);
								}
								// Запоминаем предыдущую букву
								old = letter;
							}
							// Если данные не все прочитаны, выводим как есть
							if((offset == 0) && (size > 0)) callback(realPath(string((char *) data, size)), size);
							// Очищаем буфер данных
							buffer.clear();
							// Освобождаем выделенную память
							vector <char> ().swap(buffer);
						}
						// Закрываем файл
						file.close();
					}
				// Выводим сообщение об ошибке
				} else fmk->log("the file name: \"%s\" is not found", fmk_t::log_t::CRITICAL, nullptr, filename.c_str());
			}
		}
// Если это clang v10 или выше
#if defined(__ANYKS_EXPERIMENTAL__)
		/**
		 * rdir Функция рекурсивного получения файлов во всех подкаталогах
		 * @param path     путь до каталога
		 * @param ext      расширение файла по которому идет фильтрация
		 * @param fmk      объект фреймворка для работы
		 * @param callback функция обратного вызова
		 */
		static void rdir(const string & path, const string & ext, const fmk_t * fmk, function <void (const string &, const uintmax_t)> callback) noexcept {
			// Если адрес каталога и расширение файлов переданы
			if(!path.empty() && !ext.empty() && (fmk != nullptr)){
				// Если каталог существует
				if(isdir(path)){
					// Устанавливаем область видимости
					namespace fs = std::__fs::filesystem;
					// Устанавливаем путь поиска
					fs::path dirpath = path;
					// Получаем полный размер каталога
					const uintmax_t size = dsize(path, ext);
					// Если размер каталога получен
					if(size > 0){
						// Выполняем рекурсивный переход по всем подкаталогам
						for(auto && entry : fs::recursive_directory_iterator(dirpath, fs::directory_options::skip_permission_denied)){
							// Возможная ошибка
							error_code ignore_error;
							// Запрашиваем файл из каталога и считаем его размер
							if(fs::is_regular_file(fs::symlink_status(entry, ignore_error))){
								// Получаем путь файла
								fs::path fsp = entry.path();
								// Если расширение файла соответствует
								if(string(fsp.extension().c_str()).rfind(ext) != string::npos){
									// Выводим полный путь файла
									callback(realPath(fsp), size);
								}
							}
						}
					// Сообщаем что каталог пустой
					} else callback("", 0);
				// Выводим сообщение об ошибке
				} else fmk->log("the path name: \"%s\" is not found", fmk_t::log_t::CRITICAL, nullptr, path.c_str());
			}
		}
// Если это gcc
#else
		/**
		 * rdir Функция рекурсивного получения файлов во всех подкаталогах
		 * @param path     путь до каталога
		 * @param ext      расширение файла по которому идет фильтрация
		 * @param fmk      объект фреймворка для работы
		 * @param callback функция обратного вызова
		 */
		static void rdir(const string & path, const string & ext, const fmk_t * fmk, function <void (const string &, const uintmax_t)> callback) noexcept {
			// Если адрес каталога и расширение файлов переданы
			if(!path.empty() && !ext.empty() && (fmk != nullptr)){
				// Если каталог существует
				if(isdir(path)){
					// Получаем полный размер каталога
					const uintmax_t sizeDir = dsize(path, ext, fmk);
					// Если размер каталога получен
					if(sizeDir > 0){
						/**
						 * readFn Прототип функции запроса файлов в каталоге
						 * @param путь до каталога
						 * @param расширение файла по которому идет фильтрация
						 */
						function <void (const string &, const string &)> readFn;
						/**
						 * readFn Функция запроса файлов в каталоге
						 * @param path путь до каталога
						 * @param ext  расширение файла по которому идет фильтрация
						 */
						readFn = [&readFn, sizeDir, fmk, &callback](const string & path, const string & ext){
							// Открываем указанный каталог
							DIR * dir = opendir(path.c_str());
							// Если каталог открыт
							if(dir != nullptr){
								// Структура проверка статистики
								struct stat info;
								// Создаем указатель на содержимое каталога
								struct dirent * ptr = nullptr;
								// Выполняем чтение содержимого каталога
								while((ptr = readdir(dir))){
									// Пропускаем названия текущие "." и внешние "..", так как идет рекурсия
									if(!strcmp(ptr->d_name, ".") || !strcmp(ptr->d_name, "..")) continue;
									// Получаем адрес в виде строки
									const string & address = fmk->format("%s%s%s", path.c_str(), FS_SEPARATOR, ptr->d_name);
									// Если статистика извлечена
									if(!stat(address.c_str(), &info)){
										// Если дочерний элемент является дирректорией
										if(S_ISDIR(info.st_mode)) readFn(address, ext);
										// Если дочерний элемент является файлом то удаляем его
										else {
											// Получаем расширение файла
											const string & extension = fmk->format(".%s", ext.c_str());
											// Получаем длину адреса
											const size_t length = extension.length();
											// Если расширение файла найдено
											if(address.substr(address.length() - length, length).compare(extension) == 0){
												// Выводим полный путь файла
												callback(realPath(address), sizeDir);
											}
										}
									}
								}
								// Закрываем открытый каталог
								closedir(dir);
							}
						};
						// Запрашиваем данные первого каталога
						readFn(path, ext);
					// Сообщаем что каталог пустой
					} else callback("", 0);
				// Выводим сообщение об ошибке
				} else fmk->log("the path name: \"%s\" is not found", fmk_t::log_t::CRITICAL, nullptr, path.c_str());
			}
		}
#endif
		/**
		 * rfdir Функция рекурсивного чтения файлов во всех подкаталогах
		 * @param path     путь до каталога
		 * @param ext      расширение файла по которому идет фильтрация
		 * @param fmk      объект фреймворка для работы
		 * @param callback функция обратного вызова
		 */
		static void rfdir(const string & path, const string & ext, const fmk_t * fmk, function <void (const string &, const string &, const uintmax_t, const uintmax_t)> callback) noexcept {
			// Если адрес каталога и расширение файлов переданы
			if(!path.empty() && !ext.empty() && (fmk != nullptr)){
				// Если каталог существует
				if(isdir(path)){
					// Переходим по всему списку файлов в каталоге
					rdir(path, ext, fmk, [&](const string & filename, const uintmax_t dirSize){
						// Выполняем считывание всех строк текста
						rfile2(filename, fmk, [&](const string & str, const uintmax_t fileSize){
							// Если текст получен
							if(!str.empty()) callback(str, filename, fileSize, dirSize);
						});
					});
				// Выводим сообщение об ошибке
				} else fmk->log("the path name: \"%s\" is not found", fmk_t::log_t::CRITICAL, nullptr, path.c_str());
			}
		}
	} fs_t;
};

#endif // __AWH_FS__
