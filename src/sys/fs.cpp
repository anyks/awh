/**
 * @file: fs.cpp
 * @date: 2023-02-13
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2023
 */

// Подключаем заголовочный файл
#include <sys/fs.hpp>

/**
 * uid Метод вывода идентификатора пользователя
 * @param name имя пользователя
 * @return     полученный идентификатор пользователя
 */
uid_t awh::FS::uid(const string & name) const noexcept {
	// Результат работы функции
	uid_t result = 0;
	/**
	 * Выполняем работу для Unix
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Если имя пользователя передано
		if(!name.empty()){
			// Получаем идентификатор имени пользователя
			struct passwd * pwd = getpwnam(name.c_str());
			// Если идентификатор пользователя не найден
			if(pwd == nullptr){
				// Выводим сообщение об ошибке
				this->_log->print("Failed to get userId from username [%s]", log_t::flag_t::WARNING, name.c_str());
				// Сообщаем что ничего не найдено
				return result;
			}
			// Выводим идентификатор пользователя
			result = pwd->pw_uid;
		}
	#endif
	// Выводим результат
	return result;
}
/**
 * gid Метод вывода идентификатора группы пользователя
 * @param name название группы пользователя
 * @return     полученный идентификатор группы пользователя
 */
gid_t awh::FS::gid(const string & name) const noexcept {
	// Результат работы функции
	gid_t result = 0;
	/**
	 * Выполняем работу для Unix
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Если имя пользователя передано
		if(!name.empty()){
			// Получаем идентификатор группы пользователя
			struct group * grp = getgrnam(name.c_str());
			// Если идентификатор группы не найден
			if(grp == nullptr){
				// Выводим сообщение об ошибке
				this->_log->print("Failed to get groupId from groupname [%s]", log_t::flag_t::WARNING, name.c_str());
				// Сообщаем что ничего не найдено
				return result;
			}
			// Выводим идентификатор группы пользователя
			result = grp->gr_gid;
		}
	#endif
	// Выводим результат
	return result;
}
/**
 * isDir Метод проверяющий существование дирректории
 * @param addr адрес дирректории
 * @return     результат проверки
 */
bool awh::FS::isDir(const string & addr) const noexcept {
	// Выводим результат
	return (this->type(addr) == type_t::DIR);
}
/**
 * isFile Метод проверяющий существование файла
 * @param addr адрес файла
 * @return     результат проверки
 */
bool awh::FS::isFile(const string & addr) const noexcept {
	// Выводим результат
	return (this->type(addr) == type_t::FILE);
}
/**
 * isSock Метод проверки существования сокета
 * @param addr адрес сокета
 * @return     результат проверки
 */
bool awh::FS::isSock(const string & addr) const noexcept {
	// Выводим результат
	return (this->type(addr) == type_t::SOCK);
}
/**
 * isLink Метод проверки существования сокета
 * @param addr адрес сокета
 * @return     результат проверки
 */
bool awh::FS::isLink(const string & addr) const noexcept {
	// Выводим результат
	return (this->type(addr, false) == type_t::LINK);
}
/**
 * istype Метод определяющая тип файловой системы по адресу
 * @param addr   адрес дирректории
 * @param actual флаг проверки актуальных файлов
 * @return       тип файловой системы
 */
awh::FS::type_t awh::FS::type(const string & addr, const bool actual) const noexcept {
	// Результат работы функции
	type_t result = type_t::NONE;
	// Если адрес дирректории передан
	if(!addr.empty()){
		// Структура проверка статистики
		struct stat info;
		// Если тип определён
		if(::stat(addr.c_str(), &info) == 0){
			// Если это каталог
			if(S_ISDIR(info.st_mode))
				// Получаем тип файловой системы
				result = type_t::DIR;
			// Если это устройство
			else if(S_ISCHR(info.st_mode))
				// Получаем тип файловой системы
				result = type_t::CHR;
			// Если это блок устройства
			else if(S_ISBLK(info.st_mode))
				// Получаем тип файловой системы
				result = type_t::BLK;
			// Если это файл
			else if(S_ISREG(info.st_mode))
				// Получаем тип файловой системы
				result = type_t::FILE;
			// Если это устройство ввода-вывода
			else if(S_ISFIFO(info.st_mode))
				// Получаем тип файловой системы
				result = type_t::FIFO;
			/**
			 * Выполняем работу для Unix
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
				// Если это сокет
				else if(S_ISSOCK(info.st_mode))
					// Получаем тип файловой системы
					result = type_t::SOCK;
			#endif
			/**
			 * Если операционной системой является MacOS X
			 */
			#if __APPLE__ || __MACH__
				// Если детектировать актуальные файлы не нужно и его тип определённо установлен
				if(!actual && (result != type_t::NONE)){
					// Создаём объект файловой системы
					FSRef link;
					// Создаём флаги принадлежности адреса
					Boolean isFolder = false, wasAliased = false;
					// Выполняем чтение указанного каталога
					if(FSPathMakeRef(reinterpret_cast <const UInt8 *> (addr.c_str()), &link, nullptr) == 0){
						// Выполняем проверку является ли адрес ярлыком
						if(FSResolveAliasFile(&link, true, &isFolder, &wasAliased) == 0){
							// Если адрес является ярлыком
							if(static_cast <bool> (wasAliased))
								// Получаем тип файловой системы
								result = type_t::LINK;
						}
					}
				}
			#endif
		}
		// Если детектировать актуальные файлы не нужно и адрес не детектирован как ссылка
		if(!actual && (result != type_t::LINK)){
			/**
			 * Выполняем работу для Unix
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
				// Если тип определён
				if(::lstat(addr.c_str(), &info) == 0){
					// Если это символьная ссылка
					if(S_ISLNK(info.st_mode))
						// Получаем тип файловой системы
						result = type_t::LINK;
				}
			/**
			 * Выполняем работу для Windows
			 */
			#else
				// Создаём объект проверки наличия ярлыка
				IShellLink * psl = nullptr;
				// Выполняем инициализацию результата
				HRESULT hres = CoInitialize(nullptr);
				// Выполняем инициализацию объекта для проверки ярлыков
				hres = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLink, reinterpret_cast <LPVOID *> (&psl));
				// Если инициализация выполнена
				if(SUCCEEDED(hres)){
					// Создаём объект проверки файла
					IPersistFile * ppf = nullptr;
					// Выполняем инициализацию объекта для проверки файла
					hres = psl->QueryInterface(IID_IPersistFile, reinterpret_cast <void **> (&ppf));
					// Если объект для проверки файла инициализирован
					if(SUCCEEDED(hres)){
						/**
						 * Если мы работаем с юникодом
						 */
						#ifdef _UNICODE
							// Выполняем загрузку переданного адреса
							hres = ppf->Load(reinterpret_cast <LPCTSTR> (addr.c_str()), STGM_READ);
						/**
						 * Если мы работаем с кодировкой CP1251
						 */
						#else
							// Создаём буфер для кодирования символов
							WCHAR wsz[MAX_PATH] = {0};
							// Выполняем перекодирование в UTF-8
							MultiByteToWideChar(CP_ACP, 0, reinterpret_cast <LPCTSTR> (addr.c_str()), -1, wsz, MAX_PATH);
							// Выполняем загрузку переданного адреса
							hres = ppf->Load(wsz, STGM_READ);
						#endif
						// Если переданный адрес является ярлыком
						if(SUCCEEDED(hres))
							// Получаем тип файловой системы
							result = type_t::LINK;
						// Выполняем очистку объекта провверки файла
						psl->Release();
					}
					// Выполняем очистку объекта провверки файла
					psl->Release();
				}
				// Выполняем очистку объекта результата
				CoUninitialize();
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * size Метод подсчёта размера файла/каталога
 * @param path полный путь для подсчёта размера
 * @param ext  расширение файла если требуется фильтрация
 * @return     общий размер файла/каталога
 */
uintmax_t awh::FS::size(const string & path, const string & ext) const noexcept {
	// Результат работы функции
	uintmax_t result = 0;
	// Если путь для подсчёта передан
	if(!path.empty()){
		// Определяем тип переданного пути
		switch(static_cast <uint8_t> (this->type(path))){
			// Если полный путь является файлом
			case static_cast <uint8_t> (type_t::FILE): {
				// Открываем файл на чтение
				ifstream file(path, ios::in);
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
			} break;
			// Если полный путь является каталогом
			case static_cast <uint8_t> (type_t::DIR): {
				// Открываем указанный каталог
				DIR * dir = ::opendir(path.c_str());
				// Если каталог открыт
				if(dir != nullptr){
					// Структура проверка статистики
					struct stat info;
					// Создаем указатель на содержимое каталога
					struct dirent * ptr = nullptr;
					// Выполняем чтение содержимого каталога
					while((ptr = ::readdir(dir))){
						// Пропускаем названия текущие "." и внешние "..", так как идет рекурсия
						if(!::strcmp(ptr->d_name, ".") || !::strcmp(ptr->d_name, ".."))
							// Выполняем пропуск каталога
							continue;
						// Получаем адрес в виде строки
						const string & address = this->_fmk->format("%s%s%s", path.c_str(), FS_SEPARATOR, ptr->d_name);
						// Если статистика извлечена
						if(!stat(address.c_str(), &info)){
							// Если дочерний элемент является дирректорией
							if(S_ISDIR(info.st_mode))
								// Выполняем подсчёт размера каталога
								result += this->size(address, ext);
							// Если дочерний элемент является файлом
							else if(!ext.empty()) {
								// Получаем расширение файла
								const string & extension = this->_fmk->format(".%s", ext.c_str());
								// Получаем длину адреса
								const size_t length = extension.length();
								// Если расширение файла найдено
								if(this->_fmk->compare(address.substr(address.length() - length), extension))
									// Получаем размер файла
									result += this->size(address);
							// Получаем размер файла
							} else result += this->size(address);
						}
					}
					// Закрываем открытый каталог
					::closedir(dir);
				}
			} break;
		}
	}
	// Выводим результат
	return result;
}
/**
 * count Метод подсчёта количество файлов в каталоге
 * @param path путь для подсчёта
 * @param ext  расширение файла если требуется фильтрация
 * @return     количество файлов в каталоге
 */
uintmax_t awh::FS::count(const string & path, const string & ext) const noexcept {
	// Результат работы функции
	uintmax_t result = 0;
	// Если адрес каталога и расширение файлов переданы
	if(!path.empty() && this->isDir(path)){
		// Открываем указанный каталог
		DIR * dir = ::opendir(path.c_str());
		// Если каталог открыт
		if(dir != nullptr){
			// Структура проверка статистики
			struct stat info;
			// Создаем указатель на содержимое каталога
			struct dirent * ptr = nullptr;
			// Выполняем чтение содержимого каталога
			while((ptr = ::readdir(dir))){
				// Пропускаем названия текущие "." и внешние "..", так как идет рекурсия
				if(!::strcmp(ptr->d_name, ".") || !::strcmp(ptr->d_name, ".."))
					// Выполняем пропуск каталога
					continue;
				// Получаем адрес в виде строки
				const string & address = this->_fmk->format("%s%s%s", path.c_str(), FS_SEPARATOR, ptr->d_name);
				// Если статистика извлечена
				if(!stat(address.c_str(), &info)){
					// Если дочерний элемент является дирректорией
					if(S_ISDIR(info.st_mode))
						// Выполняем подсчитываем количество файлов в каталоге
						result += this->count(address, ext);
					// Если дочерний элемент является файлом
					else if(!ext.empty()) {
						// Получаем расширение файла
						const string & extension = this->_fmk->format(".%s", ext.c_str());
						// Получаем длину адреса
						const size_t length = extension.length();
						// Если расширение файла найдено
						if(this->_fmk->compare(address.substr(address.length() - length, length), extension))
							// Получаем количество файлов в каталоге
							result++;
					// Получаем количество файлов в каталоге
					} else result++;
				}
			}
			// Закрываем открытый каталог
			::closedir(dir);
		}
	// Выводим сообщение об ошибке
	} else this->_log->print("Path name: \"%s\" is not dir", log_t::flag_t::WARNING, path.c_str());
	// Выводим результат
	return result;
}
/**
 * delPath Метод удаления полного пути
 * @param path полный путь для удаления
 * @return     количество дочерних элементов
 */
int awh::FS::delPath(const string & path) const noexcept {
	// Результат работы функции
	int result = -1;
	// Если адрес каталога и расширение файлов переданы
	if(!path.empty() && this->isDir(path)){
		// Открываем указанный каталог
		DIR * dir = ::opendir(path.c_str());
		// Если каталог открыт
		if(dir != nullptr){
			// Устанавливаем количество дочерних элементов
			result = 0;
			// Создаем указатель на содержимое каталога
			struct dirent * ptr = nullptr;
			// Выполняем чтение содержимого каталога
			while(!result && (ptr = ::readdir(dir))){
				// Количество найденных элементов
				int res = -1;
				// Создаем структуру буфера статистики
				struct stat buffer;
				// Пропускаем названия текущие "." и внешние "..", так как идет рекурсия
				if(!::strcmp(ptr->d_name, ".") || !::strcmp(ptr->d_name, ".."))
					// Выполняем пропуск каталога
					continue;
				// Получаем адрес каталога
				const string & dirname = this->_fmk->format("%s%s%s", path.c_str(), FS_SEPARATOR, ptr->d_name);
				// Если статистика извлечена
				if(!stat(dirname.c_str(), &buffer)){
					// Если дочерний элемент является дирректорией
					if(S_ISDIR(buffer.st_mode))
						// Выполняем удаление подкаталогов
						res = this->delPath(dirname);
					// Если дочерний элемент является файлом то удаляем его
					else res = ::unlink(dirname.c_str());
				}
				// Запоминаем количество дочерних элементов
				result = res;
			}
			// Закрываем открытый каталог
			::closedir(dir);
		}
		// Удаляем последний каталог
		if(!result)
			// Получаем количество дочерних элементов
			result = ::rmdir(path.c_str());
	// Выводим сообщение об ошибке
	} else this->_log->print("Path name: \"%s\" is not dir", log_t::flag_t::WARNING, path.c_str());
	// Выводим результат
	return result;
}
/**
 * realPath Метод извлечения реального адреса
 * @param path   путь который нужно определить
 * @param actual флаг проверки актуальных файлов
 * @return       полный путь
 */
string awh::FS::realPath(const string & path, const bool actual) const noexcept {
	// Результат работы функции
	string result = "";
	/**
	 * Выполняем работу для Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Создаём буфер для полного адреса
		char buffer[_MAX_PATH];
		// Заполняем буфер нулями
		::memset(buffer, 0, sizeof(buffer));
		// Выполняем извлечение адресов из переменных окружений
		ExpandEnvironmentStrings(path.c_str(), buffer, ARRAYSIZE(buffer));
		// Устанавливаем результат
		result = buffer;
		// Заполняем буфер нулями
		::memset(buffer, 0, sizeof(buffer));
		// Если адрес существует
		if(_fullpath(buffer, result.c_str(), _MAX_PATH) != nullptr){
			// Получаем полный адрес пути
			result = buffer;
			// Если адрес пути получен
			if(actual && !result.empty()){
				// Создаём объект проверки наличия ярлыка
				IShellLink * psl = nullptr;
				// Выполняем инициализацию результата
				HRESULT hres = CoInitialize(nullptr);
				// Выполняем инициализацию объекта для проверки ярлыков
				hres = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLink, reinterpret_cast <LPVOID *> (&psl));
				// Если инициализация выполнена
				if(SUCCEEDED(hres)){
					// Создаём объект проверки файла
					IPersistFile * ppf = nullptr;
					// Выполняем инициализацию объекта для проверки файла
					hres = psl->QueryInterface(IID_IPersistFile, reinterpret_cast <void **> (&ppf));
					// Если объект для проверки файла инициализирован
					if(SUCCEEDED(hres)){
						/**
						 * Если мы работаем с юникодом
						 */
						#ifdef _UNICODE
							// Выполняем загрузку переданного адреса
							hres = ppf->Load(reinterpret_cast <LPCTSTR> (result.c_str()), STGM_READ);
						/**
						 * Если мы работаем с кодировкой CP1251
						 */
						#else
							// Создаём буфер для кодирования символов
							WCHAR wsz[MAX_PATH] = {0};
							// Выполняем перекодирование в UTF-8
							MultiByteToWideChar(CP_ACP, 0, reinterpret_cast <LPCTSTR> (result.c_str()), -1, wsz, MAX_PATH);
							// Выполняем загрузку переданного адреса
							hres = ppf->Load(wsz, STGM_READ);
						#endif
						// Если переданный адрес является ярлыком
						if(SUCCEEDED(hres)){
							// Выполняем резолвинг ярлыка
							hres = psl->Resolve(nullptr, 0);
							// Если резолвинг ярлыка удачно выполнен
							if(SUCCEEDED(hres)){
								// Создаём буфер символов для получения каталога ярлыка
								TCHAR szGotPath[MAX_PATH] = {0};
								// Выполняем каталога где находится ярлыка
								hres = psl->GetPath(szGotPath, _countof(szGotPath), nullptr, SLGP_RAWPATH);
								// Если каталог где находится ярлык получен
								if(SUCCEEDED(hres)){
									// Создаём буфер для извлечения полного адреса ярлыка
									TCHAR achPath[MAX_PATH] = {0};
									// Выполняем извлечение полного адреса ярлыка
									hres = StringCbCopy(achPath, _countof(achPath), szGotPath);
									// Если полный адрес ярлыка извлечён
									if(SUCCEEDED(hres))
										// Выполняем установку полного адреса ярлыка
										result = reinterpret_cast <char *> (achPath);
								}
							}
						}
						// Выполняем очистку объекта провверки файла
						psl->Release();
					}
					// Выполняем очистку объекта провверки файла
					psl->Release();
				}
				// Выполняем очистку объекта результата
				CoUninitialize();
			}
		}
	/**
	 * Выполняем работу для Unix
	 */
	#else
		// Если нужно вывести актуальнй путь адреса
		if(actual){
			// Устанавливаем переданный путь адреса
			result = path;
			// Создаём буфер данных для получения адреса
			char buffer[PATH_MAX];
			// Если адрес существует
			if(::realpath(result.c_str(), buffer) != nullptr){
				// Получаем полный адрес пути
				result = buffer;
				/**
				 * Если операционной системой является MacOS X
				 */
				#if __APPLE__ || __MACH__
					// Создаём объект файловой системы
					FSRef link;
					// Создаём флаги принадлежности адреса
					Boolean isFolder = false, wasAliased = false;
					// Выполняем чтение указанного каталога
					if(FSPathMakeRef(reinterpret_cast <const UInt8 *> (result.c_str()), &link, nullptr) == 0){
						// Выполняем проверку является ли адрес ярлыком
						if(FSResolveAliasFile(&link, true, &isFolder, &wasAliased) == 0){
							// Если адрес является ярлыком
							if(static_cast <bool> (wasAliased)){
								// Создаём буфер данных адреса
								UInt8 buffer[1025];
								// Выполняем извлечение полного адреса файла
								if(FSRefMakePath(&link, buffer, 1024) == 0)
									// Получаем полный адрес файла
									return this->_fmk->format("%s%s", buffer, (isFolder ? "/" : ""));
							}
						}
					}
				#endif
			// Если результат не получен
			} else if(this->isLink(result)) {
				// Выполняем зануление буфера данных
				::memset(buffer, 0, sizeof(buffer));
				// Получаем длину полученного адреса
				const ssize_t length = ::readlink(result.c_str(), buffer, sizeof(buffer) - 1);
				// Если длина адреса получена
				if(length != -1){
					// Выполняем установку конца строки
					buffer[length] = '\0';
					// Выводим полученный результат
					return buffer;
				}
			}
		// Если актуальный путь выводить не нужно
		} else {
			// Если путь передан пустой или конеь адреса не указан
			if(path.empty() || (path.front() != FS_SEPARATOR[0])){
				// Создаём буфер данных для получения адреса
				char buffer[PATH_MAX];
				// Выполняем получение адреса текущего каталога
				if(::getcwd(buffer, sizeof(buffer)) == nullptr)
					// Выводим результат как он был передан
					return path;
				// Получаем размер полученного адреса
				size_t length = ::strlen(buffer);
				// Выполняем выделение памяти для результирующего адреса
				result.resize(length + 1, 0);
				// Выполняем получение полного адреса до текущего каталога
				::memcpy(result.data(), buffer, length);
				// Выполняем установку конца строки
				result.back() = '\0';
			}
			// Название кталога для перебора адреса
			string folder = "";
			// начало и конец диапазона строки
			size_t begin = 0, end = 0;
			// Выполняем перебор всего переданного адреса
			while((end = path.find(FS_SEPARATOR, end)) != string::npos){
				// Получаем название текущего каталога
				folder = path.substr(begin, end - begin);
				// Если название каталога мы получили
				if(!folder.empty()){
					// Если указан переход на уровень вверх
					if(folder.compare("..") == 0){
						// Выполняем поиск предыдущего каталога
						size_t pos = result.rfind(FS_SEPARATOR);
						// Если разделитель найден
						if(pos != string::npos)
							// Выполняем удаление предыдущего каталога
							result.erase(pos);
					// Если мы получили название каталога, а не псевдоним текущего
					} else if(folder.compare(".") != 0) {
						// Добавляем разделитель адреса
						result.append(FS_SEPARATOR);
						// Добавляем название каталога
						result.append(folder);
					}
				}
				// Запоминаем начало смещения
				begin = (end + 1);
				// Увеличиваем конец смещения
				end++;
			}
			// Получаем последний элемент адреса
			folder = path.substr(begin, end - begin);
			// Если последний элемент не является адресом текущего каталога
			if(folder.compare(".") != 0){
				// Добавляем разделитель адреса
				result.append(FS_SEPARATOR);
				// Добавляем название последнего элемента адреса
				result.append(folder);
			}
			// Если последний символ является разделителем и адрес не состоит из одного символа
			if((result.size() > 1) && (result.back() == FS_SEPARATOR[0]))
				// Выполняем удаление последнего символа
				result.pop_back();
		}
	#endif
	// Выводим результат
	return result;
}
/**
 * symLink Метод создания символьной ссылки
 * @param addr1 адрес на который нужно сделать ссылку
 * @param addr2 адрес где должна быть создана ссылка
 */
void awh::FS::symLink(const string & addr1, const string & addr2) const noexcept {
	// Если адреса переданы
	if(!addr1.empty() && !addr2.empty()){
		/**
		 * Выполняем работу для Unix
		 */
		#if !defined(_WIN32) && !defined(_WIN64)			
			// Выполняем создание символьной ссылки
			::symlink(this->realPath(addr1).c_str(), this->realPath(addr2).c_str());
		/**
		 * Выполняем работу для Windows
		 */
		#else
			// Получаем полный адрес пути
			const string & filename = this->realPath(addr1);
			// Если файл передан
			if(!filename.empty()){
				// Выполняем инициализацию результата
				HRESULT hres = CoInitialize(nullptr);
				// Создаём объект проверки наличия ярлыка
				IShellLink * psl = nullptr;
				// Выполняем инициализацию объекта для проверки ярлыков
				hres = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLink, reinterpret_cast <LPVOID *> (&psl));
				// Если инициализация выполнена
				if(SUCCEEDED(hres)){
					// Позиция разделителя каталога
					size_t pos = 0;
					// Создаём объект проверки файла
					IPersistFile * ppf = nullptr;
					// Выполняем инициализацию объекта для проверки файла
					hres = psl->QueryInterface(IID_IPersistFile, reinterpret_cast <void **> (&ppf));
					// Если объект для проверки файла инициализирован
					if(SUCCEEDED(hres)){
						// Определяем флаг обратного смещения
						const uint8_t offset = (filename.back() == '\\' ? 2 : 1);
						// Выполняем поиск разделителя каталога
						if((pos = filename.rfind("\\", filename.length() - static_cast <size_t> (offset))) != string::npos){
							// Создаём адрес ярлыка
							string symlink = "";
							// Описание создаваемого ярлыка
							string description = "";
							// Получаем адрес каталога где хранится файл
							const string & working = filename.substr(0, pos + 1);
							// Извлекаем имя файла
							const string & name = filename.substr(pos + 1, filename.length() - (pos + static_cast <size_t> (offset)));
							// Ищем расширение файла
							if((pos = name.find('.')) != string::npos)
								// Устанавливаем имя файла
								description = name.substr(0, pos);
							// Устанавливаем только имя файла
							else description = name;
							// Выполняем установку адреса ярлыка как он есть
							psl->SetPath(reinterpret_cast <LPCTSTR> (filename.c_str()));
							// Если рабочий каталог найден
							if(!working.empty())
								// Выполняем установку рабочего каталога
								psl->SetWorkingDirectory(reinterpret_cast <LPCTSTR> (working.c_str()));
							// Если название файла получено
							if(!description.empty())
								// Выполняем установку описания ярлыка
								psl->SetDescription(reinterpret_cast <LPCTSTR> (description.c_str()));
							// Если расширение ярлыка уже установлено
							if((addr2.length() > 4) && this->_fmk->compare(".lnk", addr2.substr(addr2.length() - 4)))
								// Выполняем установку адреса ярлыка как он есть
								symlink = this->realPath(addr2);
							// Выполняем установку полного пути адреса файла
							else symlink = this->_fmk->format("%s.lnk", this->realPath(addr2).c_str());
							/**
							 * Если мы работаем с юникодом
							 */
							#ifdef _UNICODE
								// Выполняем создание ярлыка в файловой системе
								hres = ppf->Save(reinterpret_cast <LPCTSTR> (symlink.c_str()), true); 
							/**
							 * Если мы работаем с кодировкой CP1251
							 */
							#else
								// Создаём буфер для кодирования символов
								WCHAR wsz[MAX_PATH] = {0};
								// Выполняем перекодирование в UTF-8
								MultiByteToWideChar(CP_ACP, 0, reinterpret_cast <LPCTSTR> (symlink.c_str()), -1, wsz, MAX_PATH);
								// Выполняем создание ярлыка в файловой системе
								hres = ppf->Save(wsz, true);
							#endif
						}
						// Выполняем очистку объекта провверки файла
						psl->Release();
					}
					// Выполняем очистку объекта провверки файла
					psl->Release();
				}
				// Выполняем очистку объекта результата
				CoUninitialize();
			}
		#endif
	}
}
/**
 * hardLink Метод создания жёстких ссылок
 * @param addr1 адрес на который нужно сделать ссылку
 * @param addr2 адрес где должна быть создана ссылка
 */
void awh::FS::hardLink(const string & addr1, const string & addr2) const noexcept {
	// Если адреса переданы
	if(!addr1.empty() && !addr2.empty()){
		/**
		 * Выполняем работу для Unix
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Если адрес на который нужно создать ссылку существует
			if(this->type(addr1) != type_t::NONE)			
				// Выполняем создание символьной ссылки
				::link(this->realPath(addr1).c_str(), this->realPath(addr2).c_str());
		/**
		 * Выполняем работу для Windows
		 */
		#else
			// Блокируем неиспользуемые переменные
			(void) addr1;
			(void) addr2;
		#endif
	}
}
/**
 * makePath Метод рекурсивного создания пути
 * @param path полный путь для создания
 */
void awh::FS::makePath(const string & path) const noexcept {
	// Если путь передан
	if(!path.empty()){
		// Указатель на сепаратор
		char * p = nullptr;
		// Получаем сепаратор
		const char sep = FS_SEPARATOR[0];
		// Создаём буфер входящих данных
		unique_ptr <char []> buffer(new char [path.size() + 1]);
		// Копируем переданный адрес в буфер
		snprintf(buffer.get(), path.size() + 1, "%s", path.c_str());
		// Если последний символ является сепаратором тогда удаляем его
		if(buffer.get()[path.size() - 1] == sep)
			// Устанавливаем конец строки
			buffer.get()[path.size() - 1] = 0;
		// Переходим по всем символам
		for(p = buffer.get() + 1; * p; p++){
			// Если найден сепаратор
			if(* p == sep){
				// Сбрасываем указатель
				(* p) = 0;
				/**
				 * Выполняем работу для Unix
				 */
				#if !defined(_WIN32) && !defined(_WIN64)
					// Создаем каталог
					::mkdir(buffer.get(), S_IRWXU);
				/**
				 * Выполняем работу для MS Windows
				 */
				#else
					// Создаем каталог
					_mkdir(buffer.get());
				#endif
				// Запоминаем сепаратор
				(* p) = sep;
			}
		}
		/**
		 * Выполняем работу для Unix
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Создаем последний каталог
			::mkdir(buffer.get(), S_IRWXU);
		/**
		 * Выполняем работу для MS Windows
		 */
		#else
			// Создаем последний каталог
			_mkdir(buffer.get());
		#endif
	}
}
/**
 * makeDir Метод создания каталога для хранения логов
 * @param path  адрес для каталога
 * @param user  данные пользователя
 * @param group идентификатор группы
 * @return      результат создания каталога
 */
bool awh::FS::makeDir(const string & path, const string & user, const string & group) const noexcept {
	// Результат работы функции
	bool result = false;
	// Проверяем существует ли нужный нам каталог
	if((result = (this->type(path) == type_t::NONE))){
		// Создаем каталог
		this->makePath(path);
		/**
		 * Выполняем работу для Unix
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Устанавливаем права на каталог
			this->chown(path, user, group);
		/**
		 * Выполняем работу для MS Windows
		 */
		#else
			// Зануляем неиспользуемые переменные
			(void) user;
			(void) group;
		#endif
	}
	// Сообщаем что каталог и так существует
	return result;
}
/**
 * components Метод извлечения названия и расширения файла
 * @param addr   адрес файла для извлечения его параметров
 * @param actual флаг проверки актуальных файлов
 * @param before флаг определения первой точки расширения слева
 */
pair <string, string> awh::FS::components(const string & addr, const bool actual, const bool before) const noexcept {
	// Результат работы функции
	pair <string, string> result;
	// Получаем полный адрес пути
	const string & filename = this->realPath(addr, actual);
	// Если файл передан
	if(!filename.empty()){
		// Позиция разделителя каталога
		size_t pos = 0;
		// Определяем флаг обратного смещения
		const uint8_t offset = (filename.back() == FS_SEPARATOR[0] ? 2 : 1);
		// Выполняем поиск разделителя каталога
		if((pos = filename.rfind(FS_SEPARATOR, filename.length() - static_cast <size_t> (offset))) != string::npos){
			// Если переданный адрес является каталогом
			if(this->type(filename) == type_t::DIR)
				// Выполняем вывод названия каталога
				result.first = filename.substr(pos + 1, filename.length() - (pos + static_cast <size_t> (offset)));
			// Если переданный адрес не является каталогом
			else {
				// Извлекаем имя файла
				const string & name = filename.substr(pos + 1);
				// Ищем расширение файла
				if((pos = (before ? name.find('.') : name.rfind('.'))) != string::npos){
					// Устанавливаем имя файла
					result.first = name.substr(0, pos);
					// Устанавливаем расширение файла
					result.second = name.substr(pos + 1);
				// Устанавливаем только имя файла
				} else result.first = std::forward <const string> (name);
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * Выполняем работу для Unix
 */
#if !defined(_WIN32) && !defined(_WIN64)
	/**
	 * chmod Метод получения метаданных файла или каталога
	 * @param path полный путь к файлу или каталогу
	 * @return     запрашиваемые метаданные
	 */
	mode_t awh::FS::chmod(const string & path) const noexcept {
		// Результат работы функции
		mode_t result = 0;
		// Если путь к файлу или каталогу передан
		if(!path.empty() && (this->type(path) != type_t::NONE)){
			// Создаём объект информационных данных файла или каталога
			struct stat info;
			// Выполняем чтение информационных данных файла
			if(!(result = (stat(path.c_str(), &info) == 0)) && (errno != 0))
				// Выводим в лог сообщение
				this->_log->print("%s", log_t::flag_t::WARNING, strerror(errno));
			// Если информационные данные считаны удачно
			else result = (info.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));
		}
		// Выводим результат
		return result;
	}
	/**
	 * chmod Метод изменения метаданных файла или каталога
	 * @param path полный путь к файлу или каталогу
	 * @param mode метаданные для установки
	 * @return     результат работы функции
	 */
	bool awh::FS::chmod(const string & path, const mode_t mode) const noexcept {
		// Результат работы функции
		bool result = false;
		// Если путь к файлу или каталогу передан
		if(!path.empty() && (this->type(path) != type_t::NONE)){
			// Выполняем установку метаданных файла
			if(!(result = (::chmod(path.c_str(), mode) == 0)) && (errno != 0))
				// Выводим в лог сообщение
				this->_log->print("%s", log_t::flag_t::WARNING, strerror(errno));
		}
		// Выводим результат
		return result;
	}
	/**
	 * chown Метод установки владельца на каталог
	 * @param path  путь к файлу или каталогу для установки владельца
	 * @param user  данные пользователя
	 * @param group идентификатор группы
	 * @return      результат работы функции
	 */
	bool awh::FS::chown(const string & path, const string & user, const string & group) const noexcept {
		// Результат работы функции
		bool result = false;
		// Если путь передан
		if(!path.empty() && !user.empty() && !group.empty() && this->isFile(path)){
			// Идентификатор пользователя
			const uid_t uid = this->uid(user);
			// Идентификатор группы
			const gid_t gid = this->gid(group);
			// Устанавливаем права на каталог
			if((result = (uid && gid))){
				// Выполняем установку владельца
				if(!(result = (::chown(path.c_str(), uid, gid) == 0)) && (errno != 0))
					// Выводим в лог сообщение
					this->_log->print("%s", log_t::flag_t::WARNING, strerror(errno));
			}
		}
		// Выводим результат
		return result;
	}
#endif
/**
 * readFile Метод рекурсивного получения всех строк файла
 * @param filename адрес файла для чтения
 * @param callback функция обратного вызова
 */
void awh::FS::readFile(const string & filename, function <void (const string &)> callback) const noexcept {
	/**
	 * Выполняем работу для Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Вызываем метод рекурсивного получения всех строк файла, старым способом
		this->readFile2(filename, callback);
	/**
	 * Выполняем работу для Unix
	 */
	#else
		// Если адрес файла передан
		if(!filename.empty() && this->isFile(filename)){
			// Файловый дескриптор файла
			int fd = -1;
			// Структура статистики файла
			struct stat info;
			// Если файл не открыт
			if((fd = ::open(filename.c_str(), O_RDONLY)) < 0)
				// Выводим сообщение об ошибке
				this->_log->print("Filename: \"%s\" is broken", log_t::flag_t::WARNING, filename.c_str());
			// Если файл открыт удачно
			else if(fstat(fd, &info) < 0)
				// Выводим сообщение об ошибке
				this->_log->print("Filename: \"%s\" is unknown size", log_t::flag_t::WARNING, filename.c_str());
			// Иначе продолжаем
			else {
				// Создаём смещение в тексте
				off_t offset = 0;
				// Инициализируем смещение в памяти
				off_t paOffset = (offset & ~(sysconf(_SC_PAGE_SIZE) - 1));
				// Если смещение в файше превышает размер файла
				if(offset >= info.st_size){
					// Выводим сообщение об ошибке
					this->_log->print("File offset position of %zu bytes cannot exceed the file size of %zu bytes", log_t::flag_t::CRITICAL, offset, info.st_size);
					// Выходим из функции
					return;
				}
				// Получаем размер данных в файле
				const size_t length = (info.st_size - offset);
				// Выполняем отображение файла в памяти
				void * buffer = ::mmap(nullptr, (length + offset - paOffset), PROT_READ, MAP_PRIVATE, fd, paOffset);
				// Если произошла ошибка чтения данных файла
				if(buffer == MAP_FAILED)
					// Выводим сообщение что прочитать файл не удалось
					this->_log->print("Filename: \"%s\" is not read", log_t::flag_t::WARNING, filename.c_str());
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
							if(length == 0)
								// Устанавливаем размер символа в 1 байт
								length = 1;
							// Если мы получили последний символ и он не является переносом строки
							if((i == (size - 1)) && (letter != '\n'))
								// Выполняем компенсацию размера строки
								length++;
							// Если длина слова получена, выводим полученную строку
							callback(string((char *) buffer + offset, length));
							// Выполняем смещение
							offset = (i + 1);
						}
						// Запоминаем предыдущую букву
						old = letter;
					}
					// Если данные не все прочитаны, выводим как есть
					if((offset == 0) && (size > 0))
						// Выводим полученную строку
						callback(string((char *) buffer, size));
				}
				// Выполняем удаление сопоставления для указанного диапазона адресов
				::munmap(buffer, (length + offset - paOffset));
			}
			// Если файл открыт, закрываем его
			if(fd > -1) ::close(fd);
		// Выводим сообщение об ошибке
		} else this->_log->print("Filename: \"%s\" is not found", log_t::flag_t::WARNING, filename.c_str());
	#endif
}
/**
 * readFile2 Метод рекурсивного получения всех строк файла (стандартным способом)
 * @param filename адрес файла для чтения
 * @param callback функция обратного вызова
 */
void awh::FS::readFile2(const string & filename, function <void (const string &)> callback) const noexcept {
	// Если адрес файла передан
	if(!filename.empty() && this->isFile(filename)){
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
					letter = data[i];
					// Если текущая буква является переносом строк
					if((i > 0) && ((letter == '\n') || (i == (size - 1)))){
						// Если предыдущая буква была возвратом каретки, уменьшаем длину строки
						length = ((old == '\r' ? i - 1 : i) - offset);
						// Если это конец файла, корректируем размер последнего байта
						if(length == 0) length = 1;
						// Если мы получили последний символ и он не является переносом строки
						if((i == (size - 1)) && (letter != '\n'))
							// Выполняем компенсацию размера строки
							length++;
						// Если длина слова получена, выводим полученную строку
						callback(string(data + offset, length));
						// Выполняем смещение
						offset = (i + 1);
					}
					// Запоминаем предыдущую букву
					old = letter;
				}
				// Если данные не все прочитаны, выводим как есть
				if((offset == 0) && (size > 0))
					// Выводим полученную строку
					callback(string(data, size));
				// Очищаем буфер данных
				buffer.clear();
				// Освобождаем выделенную память
				vector <char> ().swap(buffer);
			}
			// Закрываем файл
			file.close();
		}
	// Выводим сообщение об ошибке
	} else this->_log->print("Filename: \"%s\" is not found", log_t::flag_t::WARNING, filename.c_str());
}
/**
 * readDir Метод рекурсивного получения файлов во всех подкаталогах
 * @param path     путь до каталога
 * @param ext      расширение файла по которому идет фильтрация
 * @param recurs   флаг рекурсивного перебора каталогов
 * @param callback функция обратного вызова
 * @param actual   флаг проверки актуальных файлов
 */
void awh::FS::readDir(const string & path, const string & ext, const bool recurs, function <void (const string &)> callback, const bool actual) const noexcept {
	// Если адрес каталога и расширение файлов переданы
	if(!path.empty() && this->isDir(path)){
		/**
		 * readFn Прототип функции запроса файлов в каталоге
		 * @param путь до каталога
		 * @param расширение файла по которому идет фильтрация
		 * @param флаг рекурсивного перебора каталогов
		 */
		function <void (const string &, const string &, const bool)> readFn;
		/**
		 * readFn Функция запроса файлов в каталоге
		 * @param path   путь до каталога
		 * @param ext    расширение файла по которому идет фильтрация
		 * @param recurs флаг рекурсивного перебора каталогов
		 */
		readFn = [&](const string & path, const string & ext, const bool recurs) noexcept -> void {
			// Открываем указанный каталог
			DIR * dir = ::opendir(path.c_str());
			// Если каталог открыт
			if(dir != nullptr){
				// Структура проверка статистики
				struct stat info;
				// Создаем указатель на содержимое каталога
				struct dirent * ptr = nullptr;
				// Выполняем чтение содержимого каталога
				while((ptr = ::readdir(dir))){
					// Пропускаем названия текущие "." и внешние "..", так как идет рекурсия
					if(!::strcmp(ptr->d_name, ".") || !::strcmp(ptr->d_name, ".."))
						// Выполняем пропуск каталога
						continue;
					// Получаем адрес в виде строки
					const string & address = this->_fmk->format("%s%s%s", path.c_str(), FS_SEPARATOR, ptr->d_name);
					// Если статистика извлечена
					if(!stat(address.c_str(), &info)){
						// Если дочерний элемент является дирректорией
						if(S_ISDIR(info.st_mode)){
							// Продолжаем обработку следующих каталогов
							if(recurs)
								// Выполняем функцию обратного вызова
								readFn(address, ext, recurs);
							// Выводим данные каталога как он есть
							else callback(this->realPath(address, actual));
						// Если дочерний элемент является файлом и расширение файла указано то выводим его
						} else if(!ext.empty()) {
							// Получаем расширение файла
							const string & extension = this->_fmk->format(".%s", ext.c_str());
							// Получаем длину адреса
							const size_t length = extension.length();
							// Если расширение файла найдено
							if(this->_fmk->compare(address.substr(address.length() - length, length), extension))
								// Выводим полный путь файла
								callback(this->realPath(address, actual));
						// Если дочерний элемент является файлом то выводим его
						} else callback(this->realPath(address, actual));
					}
				}
				// Закрываем открытый каталог
				::closedir(dir);
			}
		};
		// Запрашиваем данные первого каталога
		readFn(path, ext, recurs);
	// Выводим сообщение об ошибке
	} else this->_log->print("Path name: \"%s\" is not found", log_t::flag_t::WARNING, path.c_str());
}
/**
 * readPath Метод рекурсивного чтения файлов во всех подкаталогах
 * @param path     путь до каталога
 * @param ext      расширение файла по которому идет фильтрация
 * @param recurs   флаг рекурсивного перебора каталогов
 * @param callback функция обратного вызова
 * @param actual   флаг проверки актуальных файлов
 */
void awh::FS::readPath(const string & path, const string & ext, const bool recurs, function <void (const string &, const string &)> callback, const bool actual) const noexcept {
	// Если адрес каталога и расширение файлов переданы
	if(!path.empty() && this->isDir(path)){
		// Переходим по всему списку файлов в каталоге
		this->readDir(path, ext, recurs, [&](const string & filename) noexcept -> void {
			// Выполняем считывание всех строк текста
			this->readFile2(filename, [&](const string & text) noexcept -> void {
				// Если текст получен
				if(!text.empty())
					// Выводим функцию обратного вызова
					callback(text, filename);
			});
		}, actual);
	// Выводим сообщение об ошибке
	} else this->_log->print("Path name: \"%s\" is not found", log_t::flag_t::WARNING, path.c_str());
}
