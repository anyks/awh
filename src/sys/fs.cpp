/**
 * @file: fs.cpp
 * @date: 2024-02-25
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2024
 */

// Подключаем заголовочный файл
#include <sys/fs.hpp>

/**
 * message Метод получения текста описания ошибки
 * @param code код ошибки для получения сообщения
 * @return     текст сообщения описания кода ошибки
 */
string awh::FS::message(const int32_t code) const noexcept {
	// Если код ошибки не передан
	if(code == 0)
		// Выполняем получение кода ошибки
		const_cast <int32_t &> (code) = AWH_ERROR();
	/**
	 * Методы только для OS Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Создаём буфер сообщения ошибки
		wchar_t message[256] = {0};
		// Выполняем формирование текста ошибки
		FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, code, 0, message, 256, 0);
		// Выводим текст полученной ошибки
		return this->_fmk->convert(message);
	/**
	 * Для всех остальных операционных систем
	 */
	#else
		// Выводим текст полученной ошибки
		return ::strerror(code);
	#endif
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
 * type Метод определяющая тип файловой системы по адресу
 * @param addr   адрес дирректории
 * @param actual флаг проверки актуальных файлов
 * @return       тип файловой системы
 */
awh::FS::type_t awh::FS::type(const string & addr, const bool actual) const noexcept {
	// Результат работы функции
	type_t result = type_t::NONE;
	// Если адрес дирректории передан
	if(!addr.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * Выполняем работу для Windows
			 */
			#if defined(_WIN32) || defined(_WIN64)
				// Структура проверка статистики
				struct _stat info;
				// Выполняем извлечение данных статистики
				const int32_t status = _wstat(this->_fmk->convert(addr).c_str(), &info);
			/**
			 * Выполняем работу для Unix
			 */
			#else
				// Структура проверка статистики
				struct stat info;
				// Выполняем извлечение данных статистики
				const int32_t status = ::stat(addr.c_str(), &info);
			#endif
			// Если тип определён
			if(status == 0){
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
				/**
				 * Выполняем работу для Windows
				 */
				#else
					// Создаём объект работы с файлом
					HANDLE file = CreateFileW(this->_fmk->convert(addr).c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
					// Если открыть файл открыт нормально
					if(file != INVALID_HANDLE_VALUE){
						// Если файл является сокетом
						if(GetFileType(file) == FILE_TYPE_PIPE)
							// Получаем тип файловой системы
							result = type_t::SOCK;
						// Выполняем закрытие файла
						CloseHandle(file);
					}
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
					IShellLinkW * psl = nullptr;
					// Выполняем инициализацию результата
					HRESULT hres = CoInitialize(nullptr);
					// Выполняем инициализацию объекта для проверки ярлыков
					hres = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW, reinterpret_cast <LPVOID *> (&psl));
					// Если инициализация выполнена
					if(SUCCEEDED(hres)){
						// Создаём объект проверки файла
						IPersistFile * ppf = nullptr;
						// Выполняем инициализацию объекта для проверки файла
						hres = psl->QueryInterface(IID_IPersistFile, reinterpret_cast <void **> (&ppf));
						// Если объект для проверки файла инициализирован
						if(SUCCEEDED(hres)){
							// Выполняем загрузку переданного адреса
							hres = ppf->Load(this->_fmk->convert(addr).c_str(), STGM_READ);
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
		/**
		 * Если возникает ошибка
		 */
		} catch (const std::ios_base::failure & error) {
			// Выводим сообщение инициализации метода класса скрипта торговой платформы
			this->_log->print("%s for address %s", log_t::flag_t::CRITICAL, error.what(), addr.c_str());
		/**
		 * Если возникает ошибка
		 */
		} catch(const std::exception & error) {
			// Выводим сообщение инициализации метода класса скрипта торговой платформы
			this->_log->print("%s for address %s", log_t::flag_t::CRITICAL, error.what(), addr.c_str());
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
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Определяем тип переданного пути
			switch(static_cast <uint8_t> (this->type(path))){
				// Если полный путь является файлом
				case static_cast <uint8_t> (type_t::FILE): {
					/**
					 * Выполняем работу для Windows
					 */
					#if defined(_WIN32) || defined(_WIN64)
						// Создаём объект работы с файлом
						HANDLE file = CreateFileW(this->_fmk->convert(path).c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
						// Если открыть файл открыт нормально
						if(file != INVALID_HANDLE_VALUE){
							// Получаем размер файла
							result = static_cast <uintmax_t> (GetFileSize(file, nullptr));
							// Выполняем закрытие файла
							CloseHandle(file);
						}
					/**
					 * Выполняем работу для Unix
					 */
					#else
						// Структура проверка статистики
						struct stat info;
						// Выполняем извлечение данных статистики
						const int32_t status = ::stat(path.c_str(), &info);
						// Если тип определён
						if(status == 0)
							// Выводим размер файла
							result = static_cast <uintmax_t> (info.st_size);
						// Если прочитать файла не вышло
						else {
							// Открываем файл на чтение
							ifstream file(path, ios::in | ios::binary);
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
					#endif
				} break;
				// Если полный путь является каталогом
				case static_cast <uint8_t> (type_t::DIR): {
					/**
					 * Выполняем работу для Windows
					 */
					#if defined(_WIN32) || defined(_WIN64)
						// Открываем указанный каталог
						_WDIR * dir = _wopendir(this->_fmk->convert(path).c_str());
					/**
					 * Выполняем работу для Unix
					 */
					#else
						// Открываем указанный каталог
						DIR * dir = ::opendir(path.c_str());
					#endif
						// Если каталог открыт
						if(dir != nullptr){
							/**
							 * Выполняем работу для Windows
							 */
							#if defined(_WIN32) || defined(_WIN64)
								// Структура проверка статистики
								struct _stat info;
								// Создаем указатель на содержимое каталога
								struct _wdirent * ptr = nullptr;
								// Выполняем чтение содержимого каталога
								while((ptr = _wreaddir(dir))){
							/**
							 * Выполняем работу для Unix
							 */
							#else
								// Структура проверка статистики
								struct stat info;
								// Создаем указатель на содержимое каталога
								struct dirent * ptr = nullptr;
								// Выполняем чтение содержимого каталога
								while((ptr = ::readdir(dir))){
							#endif
									/**
									 * Выполняем работу для Windows
									 */
									#if defined(_WIN32) || defined(_WIN64)
										// Пропускаем названия текущие "." и внешние "..", так как идет рекурсия
										if(!::wcscmp(ptr->d_name, L".") || !::wcscmp(ptr->d_name, L".."))
											// Выполняем пропуск каталога
											continue;
										// Получаем адрес в виде строки
										const string & address = this->_fmk->format("%s%s%s", path.c_str(), FS_SEPARATOR, this->_fmk->convert(wstring(ptr->d_name)).c_str());
									/**
									 * Выполняем работу для Unix
									 */
									#else
										// Пропускаем названия текущие "." и внешние "..", так как идет рекурсия
										if(!::strcmp(ptr->d_name, ".") || !::strcmp(ptr->d_name, ".."))
											// Выполняем пропуск каталога
											continue;
										// Получаем адрес в виде строки
										const string & address = this->_fmk->format("%s%s%s", path.c_str(), FS_SEPARATOR, ptr->d_name);
									#endif
									/**
									 * Выполняем работу для Windows
									 */
									#if defined(_WIN32) || defined(_WIN64)
										// Если статистика извлечена
										if(!_wstat(this->_fmk->convert(address).c_str(), &info)){
									/**
									 * Выполняем работу для Unix
									 */
									#else
										// Если статистика извлечена
										if(!::stat(address.c_str(), &info)){
									#endif
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
												// Если расширение не выше полного адреса
												if(address.length() > length){
													// Если расширение файла найдено
													if(this->_fmk->compare(address.substr(address.length() - length), extension))
														// Получаем размер файла
														result += this->size(address);
												}
											// Получаем размер файла
											} else result += this->size(address);
									}
							}
							/**
							 * Выполняем работу для Windows
							 */
							#if defined(_WIN32) || defined(_WIN64)
								// Закрываем открытый каталог
								_wclosedir(dir);
							/**
							 * Выполняем работу для Unix
							 */
							#else
								// Закрываем открытый каталог
								::closedir(dir);
							#endif
						}
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch (const std::ios_base::failure & error) {
			// Выводим сообщение инициализации метода класса скрипта торговой платформы
			this->_log->print("%s for path %s", log_t::flag_t::CRITICAL, error.what(), path.c_str());
		/**
		 * Если возникает ошибка
		 */
		} catch(const std::exception & error) {
			// Выводим сообщение инициализации метода класса скрипта торговой платформы
			this->_log->print("%s for path %s", log_t::flag_t::CRITICAL, error.what(), path.c_str());
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
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * Выполняем работу для Windows
			 */
			#if defined(_WIN32) || defined(_WIN64)
				// Открываем указанный каталог
				_WDIR * dir = _wopendir(this->_fmk->convert(path).c_str());
			/**
			 * Выполняем работу для Unix
			 */
			#else
				// Открываем указанный каталог
				DIR * dir = ::opendir(path.c_str());
			#endif
				// Если каталог открыт
				if(dir != nullptr){
					/**
					 * Выполняем работу для Windows
					 */
					#if defined(_WIN32) || defined(_WIN64)
						// Структура проверка статистики
						struct _stat info;
						// Создаем указатель на содержимое каталога
						struct _wdirent * ptr = nullptr;
						// Выполняем чтение содержимого каталога
						while((ptr = _wreaddir(dir))){
					/**
					 * Выполняем работу для Unix
					 */
					#else
						// Структура проверка статистики
						struct stat info;
						// Создаем указатель на содержимое каталога
						struct dirent * ptr = nullptr;
						// Выполняем чтение содержимого каталога
						while((ptr = ::readdir(dir))){
					#endif
							/**
							 * Выполняем работу для Windows
							 */
							#if defined(_WIN32) || defined(_WIN64)
								// Пропускаем названия текущие "." и внешние "..", так как идет рекурсия
								if(!::wcscmp(ptr->d_name, L".") || !::wcscmp(ptr->d_name, L".."))
									// Выполняем пропуск каталога
									continue;
								// Получаем адрес в виде строки
								const string & address = this->_fmk->format("%s%s%s", path.c_str(), FS_SEPARATOR, this->_fmk->convert(wstring(ptr->d_name)).c_str());
							/**
							 * Выполняем работу для Unix
							 */
							#else
								// Пропускаем названия текущие "." и внешние "..", так как идет рекурсия
								if(!::strcmp(ptr->d_name, ".") || !::strcmp(ptr->d_name, ".."))
									// Выполняем пропуск каталога
									continue;
								// Получаем адрес в виде строки
								const string & address = this->_fmk->format("%s%s%s", path.c_str(), FS_SEPARATOR, ptr->d_name);
							#endif
							/**
							 * Выполняем работу для Windows
							 */
							#if defined(_WIN32) || defined(_WIN64)
								// Если статистика извлечена
								if(!_wstat(this->_fmk->convert(address).c_str(), &info)){
							/**
							 * Выполняем работу для Unix
							 */
							#else
								// Если статистика извлечена
								if(!::stat(address.c_str(), &info)){
							#endif
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
										// Если расширение не выше полного адреса
										if(address.length() > length){
											// Если расширение файла найдено
											if(this->_fmk->compare(address.substr(address.length() - length, length), extension))
												// Получаем количество файлов в каталоге
												result++;
										}
									// Получаем количество файлов в каталоге
									} else result++;
							}
					}
					/**
					 * Выполняем работу для Windows
					 */
					#if defined(_WIN32) || defined(_WIN64)
						// Закрываем открытый каталог
						_wclosedir(dir);
					/**
					 * Выполняем работу для Unix
					 */
					#else
						// Закрываем открытый каталог
						::closedir(dir);
					#endif
				}
		/**
		 * Если возникает ошибка
		 */
		} catch (const std::ios_base::failure & error) {
			// Выводим сообщение инициализации метода класса скрипта торговой платформы
			this->_log->print("%s for path %s", log_t::flag_t::CRITICAL, error.what(), path.c_str());
		/**
		 * Если возникает ошибка
		 */
		} catch(const std::exception & error) {
			// Выводим сообщение инициализации метода класса скрипта торговой платформы
			this->_log->print("%s for path %s", log_t::flag_t::CRITICAL, error.what(), path.c_str());
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
int32_t awh::FS::delPath(const string & path) const noexcept {
	// Результат работы функции
	int32_t result = -1;
	// Если адрес передан
	if(!path.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Определяем тип пути
			switch(static_cast <uint8_t> (this->type(path))){
				// Если переданный путь является каталогом
				case static_cast <uint8_t> (type_t::DIR): {
					/**
					 * Выполняем работу для Windows
					 */
					#if defined(_WIN32) || defined(_WIN64)
						// Открываем указанный каталог
						_WDIR * dir = _wopendir(this->_fmk->convert(path).c_str());
					/**
					 * Выполняем работу для Unix
					 */
					#else
						// Открываем указанный каталог
						DIR * dir = ::opendir(path.c_str());
					#endif
						// Если каталог открыт
						if(dir != nullptr){
							// Устанавливаем количество дочерних элементов
							result = 0;
							/**
							 * Выполняем работу для Windows
							 */
							#if defined(_WIN32) || defined(_WIN64)
								// Структура проверка статистики
								struct _stat info;
								// Создаем указатель на содержимое каталога
								struct _wdirent * ptr = nullptr;
								// Выполняем чтение содержимого каталога
								while(!result && (ptr = _wreaddir(dir))){
							/**
							 * Выполняем работу для Unix
							 */
							#else
								// Структура проверка статистики
								struct stat info;
								// Создаем указатель на содержимое каталога
								struct dirent * ptr = nullptr;
								// Выполняем чтение содержимого каталога
								while(!result && (ptr = ::readdir(dir))){
							#endif
									// Количество найденных элементов
									int32_t count = -1;
									/**
									 * Выполняем работу для Windows
									 */
									#if defined(_WIN32) || defined(_WIN64)
										// Пропускаем названия текущие "." и внешние "..", так как идет рекурсия
										if(!::wcscmp(ptr->d_name, L".") || !::wcscmp(ptr->d_name, L".."))
											// Выполняем пропуск каталога
											continue;
										// Получаем адрес в виде строки
										const string & address = this->_fmk->format("%s%s%s", path.c_str(), FS_SEPARATOR, this->_fmk->convert(wstring(ptr->d_name)).c_str());
									/**
									 * Выполняем работу для Unix
									 */
									#else
										// Пропускаем названия текущие "." и внешние "..", так как идет рекурсия
										if(!::strcmp(ptr->d_name, ".") || !::strcmp(ptr->d_name, ".."))
											// Выполняем пропуск каталога
											continue;
										// Получаем адрес каталога
										const string & address = this->_fmk->format("%s%s%s", path.c_str(), FS_SEPARATOR, ptr->d_name);
									#endif
									/**
									 * Выполняем работу для Windows
									 */
									#if defined(_WIN32) || defined(_WIN64)
										// Если статистика извлечена
										if(!_wstat(this->_fmk->convert(address).c_str(), &info)){
											// Если дочерний элемент является дирректорией
											if(S_ISDIR(info.st_mode))
												// Выполняем удаление подкаталогов
												count = this->delPath(address);
											// Если дочерний элемент является файлом то удаляем его
											else count = _wunlink(this->_fmk->convert(address).c_str());
										// Если путь является символьной ссылкой
										} else if(this->isLink(address))
											// Выполняем удаление символьной ссылки
											count = _wunlink(this->_fmk->convert(address).c_str());
									/**
									 * Выполняем работу для Unix
									 */
									#else
										// Если статистика извлечена
										if(!::stat(address.c_str(), &info)){
											// Если дочерний элемент является дирректорией
											if(S_ISDIR(info.st_mode))
												// Выполняем удаление подкаталогов
												count = this->delPath(address);
											// Если дочерний элемент является файлом то удаляем его
											else count = ::unlink(address.c_str());
										// Если путь является символьной ссылкой
										} else if(this->isLink(address))
											// Выполняем удаление символьной ссылки
											count = ::unlink(address.c_str());
									#endif
									// Запоминаем количество дочерних элементов
									result = count;
							}
							/**
							 * Выполняем работу для Windows
							 */
							#if defined(_WIN32) || defined(_WIN64)
								// Закрываем открытый каталог
								_wclosedir(dir);
							/**
							 * Выполняем работу для Unix
							 */
							#else
								// Закрываем открытый каталог
								::closedir(dir);
							#endif
						}
						// Удаляем последний каталог
						if(!result){
							/**
							 * Выполняем работу для Windows
							 */
							#if defined(_WIN32) || defined(_WIN64)
								// Получаем количество дочерних элементов
								result = _wrmdir(this->_fmk->convert(path).c_str());
							/**
							 * Выполняем работу для Unix
							 */
							#else
								// Получаем количество дочерних элементов
								result = ::rmdir(path.c_str());
							#endif
						}
				} break;
				// Если переданный путь является файлом
				case static_cast <uint8_t> (type_t::FILE): {
					/**
					 * Выполняем работу для Windows
					 */
					#if defined(_WIN32) || defined(_WIN64)
						// Выполняем удаление переданного пути
						result = _wunlink(this->_fmk->convert(path).c_str());
					/**
					 * Выполняем работу для Unix
					 */
					#else
						// Выполняем удаление переданного пути
						result = ::unlink(path.c_str());
					#endif
				} break;
			}
			// Если путь является символьной ссылкой
			if(this->isLink(path)){
				/**
				 * Выполняем работу для Windows
				 */
				#if defined(_WIN32) || defined(_WIN64)
					// Выполняем удаление переданного пути
					result = _wunlink(this->_fmk->convert(path).c_str());
				/**
				 * Выполняем работу для Unix
				 */
				#else
					// Выполняем удаление переданного пути
					result = ::unlink(path.c_str());
				#endif
			}
		/**
		 * Если возникает ошибка
		 */
		} catch (const std::ios_base::failure & error) {
			// Выводим сообщение инициализации метода класса скрипта торговой платформы
			this->_log->print("%s for path %s", log_t::flag_t::CRITICAL, error.what(), path.c_str());
		/**
		 * Если возникает ошибка
		 */
		} catch(const std::exception & error) {
			// Выводим сообщение инициализации метода класса скрипта торговой платформы
			this->_log->print("%s for path %s", log_t::flag_t::CRITICAL, error.what(), path.c_str());
		}
	}
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
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Выполняем работу для Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Создаём буфер для полного адреса
			wchar_t buffer[_MAX_PATH];
			// Заполняем буфер нулями
			::memset(buffer, 0, sizeof(buffer));
			// Выполняем извлечение адресов из переменных окружений
			ExpandEnvironmentStringsW(this->_fmk->convert(path).c_str(), buffer, ARRAYSIZE(buffer));
			// Устанавливаем результат
			result = this->_fmk->convert(buffer);
			// Заполняем буфер нулями
			::memset(buffer, 0, sizeof(buffer));
			// Если адрес существует
			if(_wfullpath(buffer, this->_fmk->convert(result).c_str(), _MAX_PATH) != nullptr){
				// Получаем полный адрес пути
				result = this->_fmk->convert(buffer);
				// Если адрес пути получен
				if(actual && !result.empty()){
					// Создаём объект проверки наличия ярлыка
					IShellLinkW * psl = nullptr;
					// Выполняем инициализацию результата
					HRESULT hres = CoInitialize(nullptr);
					// Выполняем инициализацию объекта для проверки ярлыков
					hres = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW, reinterpret_cast <LPVOID *> (&psl));
					// Если инициализация выполнена
					if(SUCCEEDED(hres)){
						// Создаём объект проверки файла
						IPersistFile * ppf = nullptr;
						// Выполняем инициализацию объекта для проверки файла
						hres = psl->QueryInterface(IID_IPersistFile, reinterpret_cast <void **> (&ppf));
						// Если объект для проверки файла инициализирован
						if(SUCCEEDED(hres)){
							// Выполняем загрузку переданного адреса
							hres = ppf->Load(this->_fmk->convert(result).c_str(), STGM_READ);
							// Если переданный адрес является ярлыком
							if(SUCCEEDED(hres)){
								// Выполняем резолвинг ярлыка
								hres = psl->Resolve(nullptr, 0);
								// Если резолвинг ярлыка удачно выполнен
								if(SUCCEEDED(hres)){
									// Создаём буфер символов для получения каталога ярлыка
									WCHAR szGotPath[MAX_PATH] = {0};
									// Выполняем получение каталога где находится ярлыка
									hres = psl->GetPath(szGotPath, _countof(szGotPath), nullptr, SLGP_RAWPATH);
									// Если каталог где находится ярлык получен
									if(SUCCEEDED(hres)){
										// Создаём буфер для извлечения полного адреса ярлыка
										WCHAR achPath[MAX_PATH] = {0};
										// Выполняем извлечение полного адреса ярлыка
										hres = StringCbCopyW(achPath, _countof(achPath), szGotPath);
										// Если полный адрес ярлыка извлечён
										if(SUCCEEDED(hres)){
											// Определяем размер полученных данных
											const int32_t size = WideCharToMultiByte(CP_UTF8, 0, achPath, -1, 0, 0, 0, 0);
											// Если размер извлекаемых данных получен
											if(size > 0){
												// Выполняем выделение памяти для результирующего буфера
												result.resize(static_cast <size_t> (size), 0);
												// Выполняем извлечение полного адреса ярлыка
												WideCharToMultiByte(CP_UTF8, 0, achPath, -1, result.data(), result.size(), 0, 0);
											}
										}
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
			// Выполняем перекодирование адреса
			return result;
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
					const size_t length = ::strlen(buffer);
					// Выполняем выделение памяти для результирующего адреса
					result.resize(length, 0);
					// Выполняем получение полного адреса до текущего каталога
					::memcpy(result.data(), buffer, length);
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
	/**
	 * Если возникает ошибка
	 */
	} catch (const std::ios_base::failure & error) {
		// Выводим сообщение инициализации метода класса скрипта торговой платформы
		this->_log->print("%s for path %s", log_t::flag_t::CRITICAL, error.what(), path.c_str());
	/**
	 * Если возникает ошибка
	 */
	} catch(const std::exception & error) {
		// Выводим сообщение инициализации метода класса скрипта торговой платформы
		this->_log->print("%s for path %s", log_t::flag_t::CRITICAL, error.what(), path.c_str());
	}
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
		 * Выполняем перехват ошибок
		 */
		try {
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
					IShellLinkW * psl = nullptr;
					// Выполняем инициализацию объекта для проверки ярлыков
					hres = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW, reinterpret_cast <LPVOID *> (&psl));
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
								psl->SetPath(this->_fmk->convert(filename).c_str());
								// Если рабочий каталог найден
								if(!working.empty())
									// Выполняем установку рабочего каталога
									psl->SetWorkingDirectory(this->_fmk->convert(working).c_str());
								// Если название файла получено
								if(!description.empty())
									// Выполняем установку описания ярлыка
									psl->SetDescription(this->_fmk->convert(description).c_str());
								// Если расширение ярлыка уже установлено
								if((addr2.length() > 4) && this->_fmk->compare(".lnk", addr2.substr(addr2.length() - 4)))
									// Выполняем установку адреса ярлыка как он есть
									symlink = this->realPath(addr2);
								// Выполняем установку полного пути адреса файла
								else symlink = this->_fmk->format("%s.lnk", this->realPath(addr2).c_str());
								// Выполняем создание ярлыка в файловой системе
								hres = ppf->Save(this->_fmk->convert(symlink).c_str(), true);
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
		/**
		 * Если возникает ошибка
		 */
		} catch (const std::ios_base::failure & error) {
			// Выводим сообщение инициализации метода класса скрипта торговой платформы
			this->_log->print("%s for address %s", log_t::flag_t::CRITICAL, error.what(), addr1.c_str());
		/**
		 * Если возникает ошибка
		 */
		} catch(const std::exception & error) {
			// Выводим сообщение инициализации метода класса скрипта торговой платформы
			this->_log->print("%s for address %s", log_t::flag_t::CRITICAL, error.what(), addr1.c_str());
		}
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
		 * Выполняем перехват ошибок
		 */
		try {
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
				// Если адрес на который нужно создать ссылку существует
				if(this->type(addr1) != type_t::NONE)
					// Выполняем создание обычный ярлык
					this->symLink(addr1, addr2);
			#endif
		/**
		 * Если возникает ошибка
		 */
		} catch (const std::ios_base::failure & error) {
			// Выводим сообщение инициализации метода класса скрипта торговой платформы
			this->_log->print("%s for address %s", log_t::flag_t::CRITICAL, error.what(), addr1.c_str());
		/**
		 * Если возникает ошибка
		 */
		} catch(const std::exception & error) {
			// Выводим сообщение инициализации метода класса скрипта торговой платформы
			this->_log->print("%s for address %s", log_t::flag_t::CRITICAL, error.what(), addr1.c_str());
		}
	}
}
/**
 * makePath Метод рекурсивного создания пути
 * @param path полный путь для создания
 */
void awh::FS::makePath(const string & path) const noexcept {
	// Если путь передан
	if(!path.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Указатель на сепаратор
			char * p = nullptr;
			// Получаем сепаратор
			const char sep = FS_SEPARATOR[0];
			// Создаём буфер входящих данных
			unique_ptr <char []> buffer(new char [path.size() + 1]);
			// Копируем переданный адрес в буфер
			::snprintf(buffer.get(), path.size() + 1, "%s", path.c_str());
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
						_wmkdir(this->_fmk->convert(string(buffer.get())).c_str());
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
				_wmkdir(this->_fmk->convert(string(buffer.get())).c_str());
			#endif
		/**
		 * Если возникает ошибка
		 */
		} catch(const std::bad_alloc &) {
			// Выводим сообщение в лог
			this->_log->print("FS: %s", log_t::flag_t::CRITICAL, "memory allocation error");
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		/**
		 * Если возникает ошибка
		 */
		} catch (const std::ios_base::failure & error) {
			// Выводим сообщение инициализации метода класса скрипта торговой платформы
			this->_log->print("%s for path %s", log_t::flag_t::CRITICAL, error.what(), path.c_str());
		/**
		 * Если возникает ошибка
		 */
		} catch(const std::exception & error) {
			// Выводим сообщение инициализации метода класса скрипта торговой платформы
			this->_log->print("%s for path %s", log_t::flag_t::CRITICAL, error.what(), path.c_str());
		}
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
	/**
	 * Выполняем перехват ошибок
	 */
	try {
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
	/**
	 * Если возникает ошибка
	 */
	} catch (const std::ios_base::failure & error) {
		// Выводим сообщение инициализации метода класса скрипта торговой платформы
		this->_log->print("%s for address %s", log_t::flag_t::CRITICAL, error.what(), addr.c_str());
	/**
	 * Если возникает ошибка
	 */
	} catch(const std::exception & error) {
		// Выводим сообщение инициализации метода класса скрипта торговой платформы
		this->_log->print("%s for address %s", log_t::flag_t::CRITICAL, error.what(), addr.c_str());
	}
	// Выводим результат
	return result;
}
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
		/**
		 * Выполняем работу для Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Извлекаем все атрибуты файла
			return static_cast <mode_t> (GetFileAttributesW(this->_fmk->convert(path).c_str()));
		/**
		 * Выполняем работу для Unix
		 */
		#else
			// Создаём объект информационных данных файла или каталога
			struct stat info;
			// Выполняем чтение информационных данных файла
			if(!(result = (stat(path.c_str(), &info) == 0)) && (AWH_ERROR() != 0))
				// Выводим в лог сообщение
				this->_log->print("%s", log_t::flag_t::WARNING, this->message(AWH_ERROR()).c_str());
			// Если информационные данные считаны удачно
			else result = (info.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));
		#endif
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
		/**
		 * Выполняем работу для Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Выполняем установку атрибутов файла
			return SetFileAttributesW(this->_fmk->convert(path).c_str(), static_cast <DWORD> (mode));
		/**
		 * Выполняем работу для Unix
		 */
		#else
			// Выполняем установку метаданных файла
			if(!(result = (::chmod(path.c_str(), mode) == 0)) && (AWH_ERROR() != 0))
				// Выводим в лог сообщение
				this->_log->print("%s", log_t::flag_t::WARNING, this->message(AWH_ERROR()).c_str());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * Выполняем работу для Unix
 */
#if !defined(_WIN32) && !defined(_WIN64)
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
		if(!path.empty() && !user.empty() && !group.empty() && (this->type(path) != type_t::NONE)){
			// Идентификатор пользователя
			const uid_t uid = this->_os.uid(user);
			// Идентификатор группы
			const gid_t gid = this->_os.gid(group);
			// Устанавливаем права на каталог
			if((result = (uid && gid))){
				// Выполняем установку владельца
				if(!(result = (::chown(path.c_str(), uid, gid) == 0)) && (AWH_ERROR() != 0))
					// Выводим в лог сообщение
					this->_log->print("%s", log_t::flag_t::WARNING, this->message(AWH_ERROR()).c_str());
			}
		}
		// Выводим результат
		return result;
	}
/**
 * Выполняем работу для Windows
 */
#else
	/**
	 * seek Метод установки позиции в файле
	 * @param file     объект открытого файла
	 * @param distance дистанцию на которую нужно переместить позицию
	 * @param position текущая позиция в файле
	 * @return         перенос позиции в файле
	 */
	int64_t awh::FS::seek(HANDLE file, const int64_t distance, const DWORD position) const noexcept {
		// Создаём объект большого числа
		LARGE_INTEGER li;
		// Устанавливаем начальное значение позиции
		li.QuadPart = distance;
		// Выполняем установку позиции в файле
		li.LowPart = SetFilePointer(file, li.LowPart, &li.HighPart, position);
		// Если мы получили ошибку установки позиции
		if((li.LowPart == INVALID_SET_FILE_POINTER) && (GetLastError() != NO_ERROR))
			// Сбрасываем значение установленной позиции
			li.QuadPart = -1;
		// Выводим значение установленной позиции
		return li.QuadPart;
	}
#endif
/**
 * read Метод чтения данных из файла
 * @param filename адрес файла для чтения
 * @return         бинарный буфер с прочитанными данными
 */
vector <char> awh::FS::read(const string & filename) const noexcept {
	// Результат работы функции
	vector <char> result;
	// Если адрес файла передан и он существует
	if(!filename.empty() && this->isFile(filename)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * Выполняем работу для Windows
			 */
			#if defined(_WIN32) || defined(_WIN64)
				// Создаём объект работы с файлом
				HANDLE file = CreateFileW(this->_fmk->convert(filename).c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
				// Если открыть файл открыт нормально
				if(file != INVALID_HANDLE_VALUE){
					// Устанавливаем размер буфера
					result.resize(static_cast <uintmax_t> (GetFileSize(file, nullptr)), 0);
					// Выполняем чтение из файла в буфер данные
					ReadFile(file, static_cast <LPVOID> (result.data()), static_cast <DWORD> (result.size()), 0, nullptr);
					// Выполняем закрытие файла
					CloseHandle(file);
				}
			/**
			 * Выполняем работу для Unix
			 */
			#else
				// Файловый дескриптор файла
				int32_t fd = -1;
				// Структура статистики файла
				struct stat info;
				// Если файл не открыт
				if((fd = ::open(filename.c_str(), O_RDONLY)) < 0)
					// Выводим сообщение об ошибке
					this->_log->print("Filename: \"%s\" is broken", log_t::flag_t::WARNING, filename.c_str());
				// Если файл открыт удачно
				else if(::fstat(fd, &info) < 0)
					// Выводим сообщение об ошибке
					this->_log->print("Filename: \"%s\" is unknown size", log_t::flag_t::WARNING, filename.c_str());
				// Иначе продолжаем
				else if(info.st_size > 0) {
					// Создаём смещение в тексте
					off_t offset = 0;
					// Инициализируем смещение в памяти
					off_t paOffset = (offset & ~(sysconf(_SC_PAGE_SIZE) - 1));
					// Получаем размер данных в файле
					const size_t size = (info.st_size - offset);
					// Выполняем отображение файла в памяти
					void * buffer = ::mmap(nullptr, (size + offset - paOffset), PROT_READ, MAP_PRIVATE, fd, paOffset);
					// Если произошла ошибка чтения данных файла
					if(buffer == MAP_FAILED)
						// Выводим сообщение что прочитать файл не удалось
						this->_log->print("Filename: \"%s\" is not read", log_t::flag_t::WARNING, filename.c_str());
					// Если файл прочитан удачно
					else if(buffer != nullptr)
						// Выполняем выделение памяти для результирующего буфера
						result.assign(reinterpret_cast <char *> (buffer), reinterpret_cast <char *> (buffer) + info.st_size);
					// Выполняем удаление сопоставления для указанного диапазона адресов
					::munmap(buffer, (size + offset - paOffset));
				}
				// Если файл открыт
				if(fd > -1)
					// Закрываем файловый дескриптор
					::close(fd);
			#endif
		/**
		 * Если возникает ошибка
		 */
		} catch (const std::ios_base::failure & error) {
			// Выводим сообщение инициализации метода класса скрипта торговой платформы
			this->_log->print("%s for filename %s", log_t::flag_t::CRITICAL, error.what(), filename.c_str());
		/**
		 * Если возникает ошибка
		 */
		} catch(const std::exception & error) {
			// Выводим сообщение инициализации метода класса скрипта торговой платформы
			this->_log->print("%s for filename %s", log_t::flag_t::CRITICAL, error.what(), filename.c_str());
		}
	}
	// Выводим результат
	return result;
}
/**
 * write Метод записи в файл бинарных данных
 * @param filename адрес файла в который необходимо выполнить запись
 * @param buffer   бинарный буфер который необходимо записать в файл
 * @param size     размер бинарного буфера для записи в файл
 */
void awh::FS::write(const string & filename, const char * buffer, const size_t size) const noexcept {
	// Если параметры для записи переданы
	if(!filename.empty() && (buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * Выполняем работу для Windows
			 */
			#if defined(_WIN32) || defined(_WIN64)
				// Выполняем открытие файла на запись
				HANDLE file = CreateFileW(this->_fmk->convert(filename).c_str(), GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
				// Если открыть файл открыт нормально
				if(file != INVALID_HANDLE_VALUE){
					// Выполняем запись данных в файл
					WriteFile(file, static_cast <LPCVOID> (buffer), static_cast <DWORD> (size), 0, nullptr);
					// Выполняем закрытие файла
					CloseHandle(file);
				}
			/**
			 * Выполняем работу для Unix
			 */
			#else
				// Файловый поток для записи
				ofstream file(filename, ios::binary);
				// Если файл открыт на запись
				if(file.is_open()){
					// Выполняем запись данных в файл
					file.write(buffer, size);
					// Закрываем файл
					file.close();
				}
			#endif
		/**
		 * Если возникает ошибка
		 */
		} catch (const std::ios_base::failure & error) {
			// Выводим сообщение инициализации метода класса скрипта торговой платформы
			this->_log->print("%s for filename %s", log_t::flag_t::CRITICAL, error.what(), filename.c_str());
		/**
		 * Если возникает ошибка
		 */
		} catch(const std::exception & error) {
			// Выводим сообщение инициализации метода класса скрипта торговой платформы
			this->_log->print("%s for filename %s", log_t::flag_t::CRITICAL, error.what(), filename.c_str());
		}
	}
}
/**
 * append Метод добавления в файл бинарных данных
 * @param filename адрес файла в который необходимо выполнить запись
 * @param buffer   бинарный буфер который необходимо записать в файл
 * @param size     размер бинарного буфера для записи в файл
 */
void awh::FS::append(const string & filename, const char * buffer, const size_t size) const noexcept {
	// Если параметры для записи переданы
	if(!filename.empty() && (buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * Выполняем работу для Windows
			 */
			#if defined(_WIN32) || defined(_WIN64)
				// Выполняем открытие файла на добавление
				HANDLE file = CreateFileW(this->_fmk->convert(filename).c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
				// Если открыть файл открыт нормально
				if(file != INVALID_HANDLE_VALUE){
					// Выполняем добавление данных в файл
					WriteFile(file, static_cast <LPCVOID> (buffer), static_cast <DWORD> (size), 0, nullptr);
					// Выполняем закрытие файла
					CloseHandle(file);
				}
			/**
			 * Выполняем работу для Unix
			 */
			#else
				// Файловый поток для добавления
				ofstream file(filename, (ios::binary | ios::app));
				// Если файл открыт на добавление
				if(file.is_open()){
					// Выполняем добавление данных в файл
					file.write(buffer, size);
					// Закрываем файл
					file.close();
				}
			#endif
		/**
		 * Если возникает ошибка
		 */
		} catch (const std::ios_base::failure & error) {
			// Выводим сообщение инициализации метода класса скрипта торговой платформы
			this->_log->print("%s for filename %s", log_t::flag_t::CRITICAL, error.what(), filename.c_str());
		/**
		 * Если возникает ошибка
		 */
		} catch(const std::exception & error) {
			// Выводим сообщение инициализации метода класса скрипта торговой платформы
			this->_log->print("%s for filename %s", log_t::flag_t::CRITICAL, error.what(), filename.c_str());
		}
	}
}
/**
 * readFile Метод рекурсивного получения всех строк файла
 * @param filename адрес файла для чтения
 * @param callback функция обратного вызова
 */
void awh::FS::readFile(const string & filename, function <void (const string &)> callback) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
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
				int32_t fd = -1;
				// Структура статистики файла
				struct stat info;
				// Если файл не открыт
				if((fd = ::open(filename.c_str(), O_RDONLY)) < 0)
					// Выводим сообщение об ошибке
					this->_log->print("Filename: \"%s\" is broken", log_t::flag_t::WARNING, filename.c_str());
				// Если файл открыт удачно
				else if(::fstat(fd, &info) < 0)
					// Выводим сообщение об ошибке
					this->_log->print("Filename: \"%s\" is unknown size", log_t::flag_t::WARNING, filename.c_str());
				// Иначе продолжаем
				else if(info.st_size > 0) {
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
								callback(string(reinterpret_cast <char *> (buffer) + offset, length));
								// Выполняем смещение
								offset = (i + 1);
							}
							// Запоминаем предыдущую букву
							old = letter;
						}
						// Если данные не все прочитаны, выводим как есть
						if((offset == 0) && (size > 0))
							// Выводим полученную строку
							callback(string(reinterpret_cast <char *> (buffer), size));
					}
					// Выполняем удаление сопоставления для указанного диапазона адресов
					::munmap(buffer, (length + offset - paOffset));
				}
				// Если файл открыт
				if(fd > -1)
					// Закрываем файловый дескриптор
					::close(fd);
			// Выводим сообщение об ошибке
			} else this->_log->print("Filename: \"%s\" is not found", log_t::flag_t::WARNING, filename.c_str());
		#endif
	/**
	 * Если возникает ошибка
	 */
	} catch (const std::ios_base::failure & error) {
		// Выводим сообщение инициализации метода класса скрипта торговой платформы
		this->_log->print("%s for filename %s", log_t::flag_t::CRITICAL, error.what(), filename.c_str());
	/**
	 * Если возникает ошибка
	 */
	} catch(const std::exception & error) {
		// Выводим сообщение инициализации метода класса скрипта торговой платформы
		this->_log->print("%s for filename %s", log_t::flag_t::CRITICAL, error.what(), filename.c_str());
	}
}
/**
 * readFile2 Метод рекурсивного получения всех строк файла (стандартным способом)
 * @param filename адрес файла для чтения
 * @param callback функция обратного вызова
 */
void awh::FS::readFile2(const string & filename, function <void (const string &)> callback) const noexcept {
	// Если адрес файла передан
	if(!filename.empty() && this->isFile(filename)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * readFn Функция чтения данных из бинарного буфера
			 * @param buffer буфер откуда производится чтение
			 */
			auto readFn = [&callback](vector <char> & buffer) noexcept -> void {
				// Устанавливаем буфер
				if(!buffer.empty()){
					// Значение текущей и предыдущей буквы
					char letter = 0, old = 0;
					// Смещение в буфере и длина полученной строки
					size_t offset = 0, length = 0;
					// Получаем данные буфера
					const char * data = buffer.data();
					// Получаем размер файла
					const uintmax_t size = static_cast <uintmax_t> (buffer.size());
					// Переходим по всему буферу
					for(uintmax_t i = 0; i < size; i++){
						// Получаем значение текущей буквы
						letter = data[i];
						// Если текущая буква является переносом строк
						if((i > 0) && ((letter == '\n') || (i == (size - 1)))){
							// Если предыдущая буква была возвратом каретки, уменьшаем длину строки
							length = ((old == '\r' ? i - 1 : i) - offset);
							// Если это конец файла
							if(length == 0)
								// Корректируем размер последнего байта
								length = 1;
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
			};
			/**
			 * Выполняем работу для Windows
			 */
			#if defined(_WIN32) || defined(_WIN64)
				// Создаём объект работы с файлом
				HANDLE file = CreateFileW(this->_fmk->convert(filename).c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
				// Если открыть файл открыт нормально
				if(file != INVALID_HANDLE_VALUE){
					// Устанавливаем размер буфера
					vector <char> buffer(static_cast <uintmax_t> (GetFileSize(file, nullptr)));
					// Выполняем чтение из файла в буфер данные
					ReadFile(file, static_cast <LPVOID> (buffer.data()), static_cast <DWORD> (buffer.size()), 0, nullptr);
					// Выполняем чтение данных из буфера
					readFn(buffer);
					// Выполняем закрытие файла
					CloseHandle(file);
				}
			/**
			 * Выполняем работу для Unix
			 */
			#else
				// Структура проверка статистики
				struct stat info;
				// Общий размер файла
				uintmax_t size = 0;
				// Если тип определён
				if(::stat(filename.c_str(), &info) == 0)
					// Получаем размер файла
					size = static_cast <uintmax_t> (info.st_size);
				// Открываем файл на чтение
				ifstream file(filename, ios::in | ios::binary);
				// Если файл открыт
				if(file.is_open()){
					// Если арзмер файла не получен
					if(size == 0){
						// Перемещаем указатель в конец файла
						file.seekg(0, file.end);
						// Определяем размер файла
						size = file.tellg();
						// Возвращаем указатель обратно
						file.seekg(0, file.beg);
					}
					// Устанавливаем размер буфера
					vector <char> buffer(size, 0);
					// Выполняем чтение данных из файла
					file.read(buffer.data(), size);
					// Выполняем чтение данных из буфера
					readFn(buffer);
					// Закрываем файл
					file.close();
				}
			#endif
		/**
		 * Если возникает ошибка
		 */
		} catch (const std::ios_base::failure & error) {
			// Выводим сообщение инициализации метода класса скрипта торговой платформы
			this->_log->print("%s for filename %s", log_t::flag_t::CRITICAL, error.what(), filename.c_str());
		/**
		 * Если возникает ошибка
		 */
		} catch(const std::exception & error) {
			// Выводим сообщение инициализации метода класса скрипта торговой платформы
			this->_log->print("%s for filename %s", log_t::flag_t::CRITICAL, error.what(), filename.c_str());
		}
	// Выводим сообщение об ошибке
	} else this->_log->print("Filename: \"%s\" is not found", log_t::flag_t::WARNING, filename.c_str());
}
/**
 * readFile3 Метод рекурсивного получения всех строк файла (построчным методом)
 * @param filename адрес файла для чтения
 * @param callback функция обратного вызова
 */
void awh::FS::readFile3(const string & filename, function <void (const string &)> callback) const noexcept {
	// Если адрес файла передан
	if(!filename.empty() && this->isFile(filename)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * Выполняем работу для Windows
			 */
			#if defined(_WIN32) || defined(_WIN64)
				// Открываем файл на чтение
				ifstream file(this->_fmk->convert(filename).c_str(), ios::in | ios::binary);
				// Если файл открыт
				if(file.is_open()){
					// Результат полученный из потока
					string result = "";
					// Выполняем чтение данных из потока
					while(std::getline(file, result))
						// Выводим полученный результат
						callback(result);
					// Закрываем файл
					file.close();
				}
			/**
			 * Выполняем работу для Unix
			 */
			#else
				// Открываем файл на чтение
				ifstream file(filename, ios::in | ios::binary);
				// Если файл открыт
				if(file.is_open()){
					// Результат полученный из потока
					string result = "";
					// Выполняем чтение данных из потока
					while(std::getline(file, result))
						// Выводим полученный результат
						callback(result);
					// Закрываем файл
					file.close();
				}
			#endif
		/**
		 * Если возникает ошибка
		 */
		} catch (const std::ios_base::failure & error) {
			// Выводим сообщение инициализации метода класса скрипта торговой платформы
			this->_log->print("%s for filename %s", log_t::flag_t::CRITICAL, error.what(), filename.c_str());
		/**
		 * Если возникает ошибка
		 */
		} catch(const std::exception & error) {
			// Выводим сообщение инициализации метода класса скрипта торговой платформы
			this->_log->print("%s for filename %s", log_t::flag_t::CRITICAL, error.what(), filename.c_str());
		}
	// Выводим сообщение об ошибке
	} else this->_log->print("Filename: \"%s\" is not found", log_t::flag_t::WARNING, filename.c_str());
}
/**
 * readDir Метод рекурсивного получения файлов во всех подкаталогах
 * @param path     путь до каталога
 * @param ext      расширение файла по которому идет фильтрация
 * @param rec      флаг рекурсивного перебора каталогов
 * @param callback функция обратного вызова
 * @param actual   флаг проверки актуальных файлов
 */
void awh::FS::readDir(const string & path, const string & ext, const bool rec, function <void (const string &)> callback, const bool actual) const noexcept {
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
			/**
			 * Выполняем перехват ошибок
			 */
			try {
				/**
				 * Выполняем работу для Windows
				 */
				#if defined(_WIN32) || defined(_WIN64)
					// Открываем указанный каталог
					_WDIR * dir = _wopendir(this->_fmk->convert(path).c_str());
				/**
				 * Выполняем работу для Unix
				 */
				#else
					// Открываем указанный каталог
					DIR * dir = ::opendir(path.c_str());
				#endif
					// Если каталог открыт
					if(dir != nullptr){
						/**
						 * Выполняем работу для Windows
						 */
						#if defined(_WIN32) || defined(_WIN64)
							// Структура проверка статистики
							struct _stat info;
							// Создаем указатель на содержимое каталога
							struct _wdirent * ptr = nullptr;
							// Выполняем чтение содержимого каталога
							while((ptr = _wreaddir(dir))){
						/**
						 * Выполняем работу для Unix
						 */
						#else
							// Структура проверка статистики
							struct stat info;
							// Создаем указатель на содержимое каталога
							struct dirent * ptr = nullptr;
							// Выполняем чтение содержимого каталога
							while((ptr = ::readdir(dir))){
						#endif
								/**
								 * Выполняем работу для Windows
								 */
								#if defined(_WIN32) || defined(_WIN64)
									// Пропускаем названия текущие "." и внешние "..", так как идет рекурсия
									if(!::wcscmp(ptr->d_name, L".") || !::wcscmp(ptr->d_name, L".."))
										// Выполняем пропуск каталога
										continue;
									// Получаем адрес в виде строки
									const string & address = this->_fmk->format("%s%s%s", path.c_str(), FS_SEPARATOR, this->_fmk->convert(wstring(ptr->d_name)).c_str());
								/**
								 * Выполняем работу для Unix
								 */
								#else
									// Пропускаем названия текущие "." и внешние "..", так как идет рекурсия
									if(!::strcmp(ptr->d_name, ".") || !::strcmp(ptr->d_name, ".."))
										// Выполняем пропуск каталога
										continue;
									// Получаем адрес в виде строки
									const string & address = this->_fmk->format("%s%s%s", path.c_str(), FS_SEPARATOR, ptr->d_name);
								#endif
								/**
								 * Выполняем работу для Windows
								 */
								#if defined(_WIN32) || defined(_WIN64)
									// Если статистика извлечена
									if(!_wstat(this->_fmk->convert(address).c_str(), &info)){
								/**
								 * Выполняем работу для Unix
								 */
								#else
									// Если статистика извлечена
									if(!::stat(address.c_str(), &info)){
								#endif
										// Если дочерний элемент является дирректорией
										if(S_ISDIR(info.st_mode)){
											// Продолжаем обработку следующих каталогов
											if(rec)
												// Выполняем функцию обратного вызова
												readFn(address, ext, rec);
											// Выводим данные каталога как он есть
											else callback(this->realPath(address, actual));
										// Если дочерний элемент является файлом и расширение файла указано то выводим его
										} else if(!ext.empty()) {
											// Получаем расширение файла
											const string & extension = this->_fmk->format(".%s", ext.c_str());
											// Получаем длину адреса
											const size_t length = extension.length();
											// Если расширение не выше полного адреса
											if(address.length() > length){
												// Если расширение файла найдено
												if(this->_fmk->compare(address.substr(address.length() - length, length), extension))
													// Выводим полный путь файла
													callback(this->realPath(address, actual));
											}
										// Если дочерний элемент является файлом то выводим его
										} else callback(this->realPath(address, actual));
								}
						}
						/**
						 * Выполняем работу для Windows
						 */
						#if defined(_WIN32) || defined(_WIN64)
							// Закрываем открытый каталог
							_wclosedir(dir);
						/**
						 * Выполняем работу для Unix
						 */
						#else
							// Закрываем открытый каталог
							::closedir(dir);
						#endif
					}
			/**
			 * Если возникает ошибка
			 */
			} catch (const std::ios_base::failure & error) {
				// Выводим сообщение инициализации метода класса скрипта торговой платформы
				this->_log->print("%s for path %s", log_t::flag_t::CRITICAL, error.what(), path.c_str());
			/**
			 * Если возникает ошибка
			 */
			} catch(const std::exception & error) {
				// Выводим сообщение инициализации метода класса скрипта торговой платформы
				this->_log->print("%s for path %s", log_t::flag_t::CRITICAL, error.what(), path.c_str());
			}
		};
		// Запрашиваем данные первого каталога
		readFn(path, ext, rec);
	// Выводим сообщение об ошибке
	} else this->_log->print("Path name: \"%s\" is not found", log_t::flag_t::WARNING, path.c_str());
}
/**
 * readPath Метод рекурсивного чтения файлов во всех подкаталогах
 * @param path     путь до каталога
 * @param ext      расширение файла по которому идет фильтрация
 * @param rec      флаг рекурсивного перебора каталогов
 * @param callback функция обратного вызова
 * @param actual   флаг проверки актуальных файлов
 */
void awh::FS::readPath(const string & path, const string & ext, const bool rec, function <void (const string &, const string &)> callback, const bool actual) const noexcept {
	// Если адрес каталога и расширение файлов переданы
	if(!path.empty() && this->isDir(path)){
		// Переходим по всему списку файлов в каталоге
		this->readDir(path, ext, rec, [&](const string & filename) noexcept -> void {
			// Выполняем считывание всех строк текста
			this->readFile(filename, [&](const string & text) noexcept -> void {
				// Если текст получен
				if(!text.empty())
					// Выводим функцию обратного вызова
					callback(text, filename);
			});
		}, actual);
	// Выводим сообщение об ошибке
	} else this->_log->print("Path name: \"%s\" is not found", log_t::flag_t::WARNING, path.c_str());
}
