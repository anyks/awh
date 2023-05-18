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
				this->_log->print("failed to get userId from username [%s]", log_t::flag_t::WARNING, name.c_str());
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
				this->_log->print("failed to get groupId from groupname [%s]", log_t::flag_t::WARNING, name.c_str());
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
 * @param name адрес дирректории
 * @return     результат проверки
 */
bool awh::FS::isDir(const string & name) const noexcept {
	// Выводим результат
	return (this->type(name) == type_t::DIR);
}
/**
 * isFile Метод проверяющий существование файла
 * @param name адрес файла
 * @return     результат проверки
 */
bool awh::FS::isFile(const string & name) const noexcept {
	// Выводим результат
	return (this->type(name) == type_t::FILE);
}
/**
 * isSock Метод проверки существования сокета
 * @param name адрес сокета
 * @return     результат проверки
 */
bool awh::FS::isSock(const string & name) const noexcept {
	// Выводим результат
	return (this->type(name) == type_t::SOCK);
}
/**
 * istype Метод определяющая тип файловой системы по адресу
 * @param name адрес дирректории
 * @return     тип файловой системы
 */
awh::FS::type_t awh::FS::type(const string & name) const noexcept {
	// Результат работы функции
	type_t result = type_t::NONE;
	// Если адрес дирректории передан
	if(!name.empty()){
		// Структура проверка статистики
		struct stat info;
		// Если тип определён
		if(stat(name.c_str(), &info) == 0){
			// Если это каталог
			if(S_ISDIR(info.st_mode)) result = type_t::DIR;
			// Если это устройство
			else if(S_ISCHR(info.st_mode)) result = type_t::CHR;
			// Если это блок устройства
			else if(S_ISBLK(info.st_mode)) result = type_t::BLK;
			// Если это файл
			else if(S_ISREG(info.st_mode)) result = type_t::FILE;
			// Если это устройство ввода-вывода
			else if(S_ISFIFO(info.st_mode)) result = type_t::FIFO;
			/**
			 * Если - это не Windows
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
				// Если это символьная ссылка
				else if(S_ISLNK(info.st_mode)) result = type_t::LNK;
				// Если это сокет
				else if(S_ISSOCK(info.st_mode)) result = type_t::SOCK;
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
						if(!strcmp(ptr->d_name, ".") || !strcmp(ptr->d_name, "..")) continue;
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
				if(!strcmp(ptr->d_name, ".") || !strcmp(ptr->d_name, "..")) continue;
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
	} else this->_log->print("the path name: \"%s\" is not dir", log_t::flag_t::WARNING, path.c_str());
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
				if(!strcmp(ptr->d_name, ".") || !strcmp(ptr->d_name, "..")) continue;
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
		if(!result) result = ::rmdir(path.c_str());
	// Выводим сообщение об ошибке
	} else this->_log->print("the path name: \"%s\" is not dir", log_t::flag_t::WARNING, path.c_str());
	// Выводим результат
	return result;
}
/**
 * realPath Метод извлечения реального адреса
 * @param path путь который нужно определить
 * @return     полный путь
 */
string awh::FS::realPath(const string & path) const noexcept {
	// Результат работы функции
	string result = "";
	// Если путь передан
	if(!path.empty()){
		/**
		 * Выполняем работу для Windows
		 */
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
			if(_fullpath(buffer, result.c_str(), _MAX_PATH) != nullptr)
				// Получаем полный адрес пути
				result = buffer;
		/**
		 * Выполняем работу для Unix
		 */
		#else
			// Получаем полный адрес
			const char * realPath = realpath(path.c_str(), nullptr);
			// Если адрес существует
			if(realPath != nullptr)
				// Получаем полный адрес пути
				result = realPath;
			// Иначе выводим путь так, как он есть
			else result = std::forward <const string> (path);
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * makePath Метод рекурсивного создания пути
 * @param path полный путь для создания
 */
void awh::FS::makePath(const string & path) const noexcept {
	// Если путь передан
	if(!path.empty()){
		// Буфер с названием каталога
		char buffer[256];
		// Указатель на сепаратор
		char * p = nullptr;
		// Получаем сепаратор
		const char sep = FS_SEPARATOR[0];
		// Копируем переданный адрес в буфер
		snprintf(buffer, sizeof(buffer), "%s", path.c_str());
		// Определяем размер адреса
		const size_t length = strlen(buffer);
		// Если последний символ является сепаратором тогда удаляем его
		if(buffer[length - 1] == sep)
			// Устанавливаем конец строки
			buffer[length - 1] = 0;
		// Если - это не Windows
		#if !defined(_WIN32) && !defined(_WIN64)
			// Переходим по всем символам
			for(p = buffer + 1; * p; p++){
				// Если найден сепаратор
				if(* p == sep){
					// Сбрасываем указатель
					* p = 0;
					// Создаем каталог
					::mkdir(buffer, S_IRWXU);
					// Запоминаем сепаратор
					* p = sep;
				}
			}
			// Создаем последний каталог
			::mkdir(buffer, S_IRWXU);
		#else
			// Переходим по всем символам
			for(p = buffer + 1; * p; p++){
				// Если найден сепаратор
				if(* p == sep){
					// Сбрасываем указатель
					* p = 0;
					// Создаем каталог
					_mkdir(buffer);
					// Запоминаем сепаратор
					* p = sep;
				}
			}
			// Создаем последний каталог
			_mkdir(buffer);
		#endif
	}
}
/**
 * Выполняем работу для Unix
 */
#if !defined(_WIN32) && !defined(_WIN64)
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
			// Устанавливаем права на каталог
			this->chown(path, user, group);
		}
		// Сообщаем что каталог и так существует
		return result;
	}
#endif
/**
 * fileComponents Метод извлечения названия и расширения файла
 * @param filename адрес файла для извлечения его параметров
 */
pair <string, string> awh::FS::fileComponents(const string & filename) const noexcept {
	// Результат работы функции
	pair <string, string> result;
	// Если файл передан
	if(!filename.empty()){
		// Позиция разделителя каталога
		size_t pos = 0;
		// Выполняем поиск разделителя каталога
		if((pos = filename.rfind(FS_SEPARATOR)) != string::npos){
			// Если переданный адрес является каталогом
			if(this->type(filename) == type_t::DIR)
				// Выполняем вывод названия каталога
				result.first = std::forward <const string> (filename.substr(pos + 1));
			// Если переданный адрес не является каталогом
			else {
				// Извлекаем имя файла
				const string & name = filename.substr(pos + 1);
				// Ищем расширение файла
				if((pos = name.find(".")) != string::npos){
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
				this->_log->print("the file name: \"%s\" is broken", log_t::flag_t::WARNING, filename.c_str());
			// Если файл открыт удачно
			else if(fstat(fd, &info) < 0)
				// Выводим сообщение об ошибке
				this->_log->print("the file name: \"%s\" is unknown size", log_t::flag_t::WARNING, filename.c_str());
			// Иначе продолжаем
			else {
				// Буфер входящих данных
				void * buffer = nullptr;
				// Выполняем отображение файла в памяти
				if((buffer = ::mmap(0, info.st_size, PROT_READ, MAP_SHARED, fd, 0)) == MAP_FAILED)
					// Выводим сообщение что прочитать файл не удалось
					this->_log->print("the file name: \"%s\" is not read", log_t::flag_t::WARNING, filename.c_str());
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
			}
			// Если файл открыт, закрываем его
			if(fd > -1) ::close(fd);
		// Выводим сообщение об ошибке
		} else this->_log->print("the file name: \"%s\" is not found", log_t::flag_t::WARNING, filename.c_str());
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
	} else this->_log->print("the file name: \"%s\" is not found", log_t::flag_t::WARNING, filename.c_str());
}
/**
 * readDir Метод рекурсивного получения файлов во всех подкаталогах
 * @param path     путь до каталога
 * @param ext      расширение файла по которому идет фильтрация
 * @param rec      флаг рекурсивного перебора каталогов
 * @param callback функция обратного вызова
 */
void awh::FS::readDir(const string & path, const string & ext, const bool rec, function <void (const string &)> callback) const noexcept {
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
		 * @param path путь до каталога
		 * @param ext  расширение файла по которому идет фильтрация
		 * @param rec  флаг рекурсивного перебора каталогов
		 */
		readFn = [&](const string & path, const string & ext, const bool rec) noexcept -> void {
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
					if(!strcmp(ptr->d_name, ".") || !strcmp(ptr->d_name, "..")) continue;
					// Получаем адрес в виде строки
					const string & address = this->_fmk->format("%s%s%s", path.c_str(), FS_SEPARATOR, ptr->d_name);
					// Если статистика извлечена
					if(!stat(address.c_str(), &info)){
						// Если дочерний элемент является дирректорией
						if(S_ISDIR(info.st_mode)){
							// Продолжаем обработку следующих каталогов
							if(rec) readFn(address, ext, rec);
							// Выводим данные каталога как он есть
							else callback(this->realPath(address));
						// Если дочерний элемент является файлом и расширение файла указано то выводим его
						} else if(!ext.empty()) {
							// Получаем расширение файла
							const string & extension = this->_fmk->format(".%s", ext.c_str());
							// Получаем длину адреса
							const size_t length = extension.length();
							// Если расширение файла найдено
							if(this->_fmk->compare(address.substr(address.length() - length, length), extension))
								// Выводим полный путь файла
								callback(this->realPath(address));
						// Если дочерний элемент является файлом то выводим его
						} else callback(this->realPath(address));
					}
				}
				// Закрываем открытый каталог
				::closedir(dir);
			}
		};
		// Запрашиваем данные первого каталога
		readFn(path, ext, rec);
	// Выводим сообщение об ошибке
	} else this->_log->print("the path name: \"%s\" is not found", log_t::flag_t::WARNING, path.c_str());
}
/**
 * readPath Метод рекурсивного чтения файлов во всех подкаталогах
 * @param path     путь до каталога
 * @param ext      расширение файла по которому идет фильтрация
 * @param rec      флаг рекурсивного перебора каталогов
 * @param callback функция обратного вызова
 */
void awh::FS::readPath(const string & path, const string & ext, const bool rec, function <void (const string &, const string &)> callback) const noexcept {
	// Если адрес каталога и расширение файлов переданы
	if(!path.empty() && this->isDir(path)){
		// Переходим по всему списку файлов в каталоге
		this->readDir(path, ext, rec, [&](const string & filename) noexcept -> void {
			// Выполняем считывание всех строк текста
			this->readFile2(filename, [&](const string & text) noexcept -> void {
				// Если текст получен
				if(!text.empty())
					// Выводим функцию обратного вызова
					callback(text, filename);
			});
		});
	// Выводим сообщение об ошибке
	} else this->_log->print("the path name: \"%s\" is not found", log_t::flag_t::WARNING, path.c_str());
}
