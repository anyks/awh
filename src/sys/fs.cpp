/**
 * @file: fs.cpp
 * @date: 2024-02-25
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
#include <sys/fs.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * @brief Пространство имён внутренних помощников модуля
 *
 */
namespace {
	/**
	 * @brief Функция разбора буфера данных на строки
	 *
	 * @note Прежний разбор был ошибочен: последняя строка из одного символа читалась с лишним
	 *       байтом за пределами буфера, пустая строка отдавалась символом переноса "\n", а первая
	 *       пустая строка файла терялась вовсе. Строки делятся по "\n", завершающий "\r" каждой
	 *       строки отбрасывается, пустые строки сохраняются, а перенос в самом конце файла
	 *       лишней пустой строки не порождает
	 *
	 * @param data     буфер данных для разбора
	 * @param size     размер буфера данных
	 * @param callback функция обратного вызова
	 */
	static void lines(const char * data, const size_t size, const function <void (const string &)> & callback){
		// Если данные переданы
		if((data != nullptr) && (size > 0) && (callback != nullptr)){
			// Начало текущей строки и конец строки
			size_t start = 0, end = 0;
			/**
			 * Выполняем перебор всего буфера данных
			 */
			for(size_t i = 0; i < size; i++){
				// Если текущая буква является переносом строки
				if(data[i] == '\n'){
					// Запоминаем конец строки
					end = i;
					// Если строка завершается возвратом каретки
					if((end > start) && (data[end - 1] == '\r'))
						// Отбрасываем возврат каретки
						end--;
					// Выводим полученную строку
					callback(string(data + start, end - start));
					// Запоминаем начало следующей строки
					start = (i + 1);
				}
			}
			// Если осталась последняя строка без переноса
			if(start < size){
				// Запоминаем конец строки
				end = size;
				// Если строка завершается возвратом каретки
				if((end > start) && (data[end - 1] == '\r'))
					// Отбрасываем возврат каретки
					end--;
				// Выводим полученную строку
				callback(string(data + start, end - start));
			}
		}
	}
	/**
	 * @brief Функция получения размера страницы памяти
	 *
	 * @return размер страницы памяти в байтах
	 */
	static size_t pagesize() noexcept {
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Сведения о системе
			SYSTEM_INFO info;
			// Выполняем получение сведений о системе
			::GetSystemInfo(&info);
			// Выводим размер страницы памяти
			return static_cast <size_t> (info.dwPageSize);
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#else
			// Получаем размер страницы памяти
			const long result = ::sysconf(_SC_PAGE_SIZE);
			// Выводим размер страницы памяти
			return (result > 0 ? static_cast <size_t> (result) : 4096);
		#endif
	}
};

/**
 * @brief Метод проверяющий существование дирректории
 *
 * @param addr адрес дирректории
 * @return     результат проверки
 */
bool awh::FS::isDir(const string & addr) const noexcept {
	// Выводим результат
	return (this->type(addr) == type_t::DIR);
}
/**
 * @brief Метод проверяющий существование файла
 *
 * @param addr адрес файла
 * @return     результат проверки
 */
bool awh::FS::isFile(const string & addr) const noexcept {
	// Выводим результат
	return (this->type(addr) == type_t::FILE);
}
/**
 * @brief Метод проверки существования сокета
 *
 * @param addr адрес сокета
 * @return     результат проверки
 */
bool awh::FS::isSock(const string & addr) const noexcept {
	// Выводим результат
	return (this->type(addr) == type_t::SOCK);
}
/**
 * @brief Метод проверки существования сокета
 *
 * @param addr адрес сокета
 * @return     результат проверки
 */
bool awh::FS::isLink(const string & addr) const noexcept {
	// Выводим результат
	return (this->type(addr, false) == type_t::LINK);
}
/**
 * @brief Метод определяющая тип файловой системы по адресу
 *
 * @param addr   адрес дирректории
 * @param actual флаг формирования актуальных адресов
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
			 * Для операционной системы MS Windows
			 */
			#if _WIN32 || _WIN64
				// Структура проверка статистики
				struct _stat info;
				// Выполняем извлечение актуального значения адреса
				const string & address = this->realPath(addr, actual);
				// Выполняем извлечение данных статистики
				const int32_t status = (!address.empty() ? ::_wstat(this->_fmk->convert(address).c_str(), &info) : -1);
			/**
			 * Для операционной системы не являющейся MS Windows
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
				 * Для операционной системы не являющейся MS Windows
				 */
				#if !_WIN32 && !_WIN64
					// Если это сокет
					else if(S_ISSOCK(info.st_mode))
						// Получаем тип файловой системы
						result = type_t::SOCK;
				/**
				 * Для операционной системы MS Windows
				 */
				#else
					// Создаём объект работы с файлом
					HANDLE file = CreateFileW(this->_fmk->convert(addr).c_str(), GENERIC_READ, (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
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
						if(::FSPathMakeRef(reinterpret_cast <const UInt8 *> (addr.c_str()), &link, nullptr) == 0){
							// Выполняем проверку является ли адрес ярлыком
							if(::FSResolveAliasFile(&link, TRUE, &isFolder, &wasAliased) == 0){
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
				 * Для операционной системы не являющейся MS Windows
				 */
				#if !_WIN32 && !_WIN64
					// Если тип определён
					if(::lstat(addr.c_str(), &info) == 0){
						// Если это символьная ссылка
						if(S_ISLNK(info.st_mode))
							// Получаем тип файловой системы
							result = type_t::LINK;
					}
				/**
				 * Для операционной системы MS Windows
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
							// Выполняем очистку объекта проверки файла (прежде здесь дважды освобождался объект ярлыка)
							ppf->Release();
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
		} catch(const ios_base::failure & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(addr, actual), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(addr, actual), log_t::flag_t::CRITICAL, error.what());
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
/**
 * @brief Метод извлечения реального адреса
 *
 * @param path   путь который нужно определить
 * @param actual флаг формирования актуальных адресов
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
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Создаём буфер для полного адреса
			wchar_t buffer[_MAX_PATH];
			// Заполняем буфер нулями
			::memset(buffer, 0, sizeof(buffer));
			// Выполняем извлечение адресов из переменных окружений
			::ExpandEnvironmentStringsW(this->_fmk->convert(path).c_str(), buffer, ARRAYSIZE(buffer));
			// Устанавливаем результат
			result = this->_fmk->convert(buffer);
			// Заполняем буфер нулями
			::memset(buffer, 0, sizeof(buffer));
			// Если адрес существует
			if(::_wfullpath(buffer, this->_fmk->convert(result).c_str(), _MAX_PATH) != nullptr){
				// Получаем полный адрес пути
				result = this->_fmk->convert(buffer);
				// Если адрес пути получен
				if(actual && !result.empty()){
					// Создаём объект проверки наличия ярлыка
					IShellLinkW * psl = nullptr;
					// Выполняем инициализацию результата
					HRESULT hres = ::CoInitialize(nullptr);
					// Выполняем инициализацию объекта для проверки ярлыков
					hres = ::CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW, reinterpret_cast <LPVOID *> (&psl));
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
										hres = ::StringCbCopyW(achPath, _countof(achPath), szGotPath);
										// Если полный адрес ярлыка извлечён
										if(SUCCEEDED(hres)){
											// Определяем размер полученных данных
											const int32_t size = ::WideCharToMultiByte(CP_UTF8, 0, achPath, -1, 0, 0, 0, 0);
											/**
											 * Размер включает завершающий ноль. Прежде ноль оставался в строке адреса,
											 * а пустой путь ярлыка (так разбирается, например, пустой файл) подменял
											 * адрес строкой из одного нуля, и существующий файл получал тип NONE
											 */
											if(size > 1){
												// Выполняем выделение памяти для результирующего буфера
												result.resize(static_cast <size_t> (size), 0);
												// Выполняем извлечение полного адреса ярлыка
												::WideCharToMultiByte(CP_UTF8, 0, achPath, -1, result.data(), size, 0, 0);
												// Удаляем завершающий ноль
												result.resize(static_cast <size_t> (size - 1));
											}
										}
									}
								}
							}
							// Выполняем очистку объекта проверки файла (прежде здесь дважды освобождался объект ярлыка)
							ppf->Release();
						}
						// Выполняем очистку объекта провверки файла
						psl->Release();
					}
					// Выполняем очистку объекта результата
					::CoUninitialize();
				}
			}
			// Выполняем перекодирование адреса
			return result;
		/**
		 * Для операционной системы не являющейся MS Windows
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
						// Если процесс является родительским
						if(this->_pid == static_cast <pid_t> (::getpid())){
							// Создаём объект файловой системы
							FSRef link;
							// Создаём флаги принадлежности адреса
							Boolean isFolder = false, wasAliased = false;
							// Выполняем чтение указанного каталога
							if(::FSPathMakeRef(reinterpret_cast <const UInt8 *> (result.c_str()), &link, nullptr) == 0){
								// Выполняем проверку является ли адрес ярлыком
								if(::FSResolveAliasFile(&link, TRUE, &isFolder, &wasAliased) == 0){
									// Если адрес является ярлыком
									if(static_cast <bool> (wasAliased)){
										// Создаём буфер данных адреса
										UInt8 buffer[1025];
										// Выполняем извлечение полного адреса файла
										if(::FSRefMakePath(&link, buffer, 1024) == 0)
											// Получаем полный адрес файла
											return this->_fmk->format("%s%s", buffer, (isFolder ? "/" : ""));
									}
								}
							}
						}
					#endif
					// Выводим полученный адрес
					return result;
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
				/**
				 * Адрес, какого в файловой системе нет, прежде отдавался как передан (относительным
				 * и ненормализованным). Теперь, как и в AWH 5, он отдаётся полным нормализованным
				 * адресом - лексическим разбором без разрешения ссылок (решение согласовано)
				 */
				if(!path.empty())
					// Выполняем лексическую нормализацию адреса
					return this->realPath(path, false);
			// Если актуальный путь выводить не нужно
			} else {
				// Если путь передан пустой или конеь адреса не указан
				if(path.empty() || (path.front() != AWH_FS_SEPARATOR[0])){
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
				/**
				 * Адрес нормализуется разбором на составные части: прежний разбор не сворачивал
				 * ".." в последней части адреса ("/a/b/.." оставался как есть, ядро же разрешает
				 * его в "/a"), а при текущем каталоге "/" давал двойной разделитель в начале
				 */
				// Составные части адреса
				vector <string> parts;
				// Название каталога для перебора адреса
				string folder = "";
				/**
				 * @brief Функция разбора адреса на составные части
				 *
				 * @param text адрес для разбора
				 */
				auto splitFn = [&parts, &folder](const string & text) -> void {
					/**
					 * Выполняем перебор всех символов адреса с завершающим разделителем
					 */
					for(size_t i = 0; i <= text.size(); i++){
						// Если достигнут разделитель или конец адреса
						if((i == text.size()) || (text[i] == AWH_FS_SEPARATOR[0])){
							// Если указан переход на уровень вверх
							if(folder.compare("..") == 0){
								// Если есть из чего удалять
								if(!parts.empty())
									// Удаляем последний каталог
									parts.pop_back();
							// Если мы получили название каталога, а не псевдоним текущего
							} else if(!folder.empty() && (folder.compare(".") != 0))
								// Добавляем название каталога
								parts.push_back(folder);
							// Очищаем название каталога
							folder.clear();
						// Выполняем сборку названия каталога
						} else folder.append(1, text[i]);
					}
				};
				// Выполняем разбор адреса текущего каталога
				splitFn(result);
				// Выполняем разбор переданного адреса
				splitFn(path);
				// Начинаем адрес с корня
				result = AWH_FS_SEPARATOR;
				/**
				 * Выполняем сборку адреса из составных частей
				 */
				for(size_t i = 0; i < parts.size(); i++){
					// Если это не первая часть адреса
					if(i > 0)
						// Добавляем разделитель адреса
						result.append(AWH_FS_SEPARATOR);
					// Добавляем название каталога
					result.append(parts[i]);
				}
			}
		#endif
	/**
	 * Если возникает ошибка
	 */
	} catch(const ios_base::failure & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(path, actual), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(path, actual), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод удаления полного пути
 *
 * @param path   полный путь для удаления
 * @param actual флаг формирования актуальных адресов
 * @return       количество дочерних элементов
 */
int32_t awh::FS::delPath(const string & path, const bool actual) const noexcept {
	// Результат работы функции
	int32_t result = -1;
	// Если адрес передан
	if(!path.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем извлечение актуального значения адреса
			const string & address = this->realPath(path, actual);
			// Если адрес получен правильный
			if(!address.empty()){
				/**
				 * Определяем тип пути
				 */
				/**
				 * Тип определяем без перехода по символьной ссылке: при актуальном адресе
				 * ссылки уже разрешены, а при неактуальном ссылка на каталог должна быть
				 * удалена как ссылка, без захода внутрь каталога на который она указывает
				 */
				switch(static_cast <uint8_t> (this->type(address, false))){
					// Если переданный путь является каталогом
					case static_cast <uint8_t> (type_t::DIR): {
						/**
						 * Для операционной системы MS Windows
						 */
						#if _WIN32 || _WIN64
							// Открываем указанный каталог
							_WDIR * dir = ::_wopendir(this->_fmk->convert(address).c_str());
						/**
						 * Для операционной системы не являющейся MS Windows
						 */
						#else
							// Открываем указанный каталог
							DIR * dir = ::opendir(address.c_str());
						#endif
							// Если каталог открыт
							if(dir != nullptr){
								// Устанавливаем количество дочерних элементов
								result = 0;
								/**
								 * Для операционной системы MS Windows
								 */
								#if _WIN32 || _WIN64
									// Структура проверка статистики
									struct _stat info;
									// Создаем указатель на содержимое каталога
									struct _wdirent * ptr = nullptr;
									/**
									 * Выполняем чтение содержимого каталога
									 */
									while(!result && (ptr = ::_wreaddir(dir))){
								/**
								 * Для операционной системы не являющейся MS Windows
								 */
								#else
									// Структура проверка статистики
									struct stat info;
									// Создаем указатель на содержимое каталога
									struct dirent * ptr = nullptr;
									/**
									 * Выполняем чтение содержимого каталога
									 */
									while(!result && (ptr = ::readdir(dir))){
								#endif
										// Количество найденных элементов
										int32_t count = -1;
										/**
										 * Для операционной системы MS Windows
										 */
										#if _WIN32 || _WIN64
											// Пропускаем названия текущие "." и внешние "..", так как идет рекурсия
											if(!::wcscmp(ptr->d_name, L".") || !::wcscmp(ptr->d_name, L".."))
												// Выполняем пропуск каталога
												continue;
											/**
											 * Дочерний адрес строится из разрешённого адреса, а не из переданного: иначе
											 * путь с ".." после символьной ссылки лексически указывает на иной каталог,
											 * чем его разрешает ядро, и удаление уходит в чужое дерево
											 */
											const string & child = this->_fmk->format("%s%s%s", address.c_str(), AWH_FS_SEPARATOR, this->_fmk->convert(wstring(ptr->d_name)).c_str());
										/**
										 * Для операционной системы не являющейся MS Windows
										 */
										#else
											// Пропускаем названия текущие "." и внешние "..", так как идет рекурсия
											if(!::strcmp(ptr->d_name, ".") || !::strcmp(ptr->d_name, ".."))
												// Выполняем пропуск каталога
												continue;
											// Получаем адрес дочернего элемента из разрешённого адреса
											const string & child = this->_fmk->format("%s%s%s", address.c_str(), AWH_FS_SEPARATOR, ptr->d_name);
										#endif
										/**
										 * Для операционной системы MS Windows
										 */
										#if _WIN32 || _WIN64
											// Если путь является символьной ссылкой, удаляем саму ссылку не заходя в неё
											if(this->isLink(child))
												// Выполняем удаление символьной ссылки
												count = ::_wunlink(this->_fmk->convert(child).c_str());
											// Если статистика извлечена
											else if(!::_wstat(this->_fmk->convert(child).c_str(), &info)){
												// Если дочерний элемент является дирректорией
												if(S_ISDIR(info.st_mode))
													// Выполняем удаление подкаталогов без разрешения ссылок
													count = this->delPath(child, false);
												// Если дочерний элемент является файлом то удаляем его
												else count = ::_wunlink(this->_fmk->convert(child).c_str());
											}
										/**
										 * Для операционной системы не являющейся MS Windows
										 */
										#else
											/**
											 * Статистику извлекаем через lstat, не переходя по символьным ссылкам:
											 * иначе ссылка на чужой каталог принимается за подкаталог и удаляется
											 * содержимое каталога на который она указывает
											 */
											if(!::lstat(child.c_str(), &info)){
												// Если дочерний элемент является символьной ссылкой
												if(S_ISLNK(info.st_mode))
													// Выполняем удаление самой символьной ссылки
													count = ::unlink(child.c_str());
												// Если дочерний элемент является дирректорией
												else if(S_ISDIR(info.st_mode))
													// Выполняем удаление подкаталогов без разрешения ссылок
													count = this->delPath(child, false);
												// Если дочерний элемент является файлом то удаляем его
												else count = ::unlink(child.c_str());
											}
										#endif
										// Запоминаем количество дочерних элементов
										result = count;
								}
								/**
								 * Для операционной системы MS Windows
								 */
								#if _WIN32 || _WIN64
									// Закрываем открытый каталог
									::_wclosedir(dir);
								/**
								 * Для операционной системы не являющейся MS Windows
								 */
								#else
									// Закрываем открытый каталог
									::closedir(dir);
								#endif
							}
							// Удаляем последний каталог
							if(!result){
								/**
								 * Для операционной системы MS Windows
								 */
								#if _WIN32 || _WIN64
									// Получаем количество дочерних элементов
									result = ::_wrmdir(this->_fmk->convert(address).c_str());
								/**
								 * Для операционной системы не являющейся MS Windows
								 */
								#else
									// Получаем количество дочерних элементов
									result = ::rmdir(address.c_str());
								#endif
							}
					} break;
					// Если переданный путь является файлом
					case static_cast <uint8_t> (type_t::FILE):
					// Если переданный путь является ссылкой
					case static_cast <uint8_t> (type_t::LINK): {
						/**
						 * Для операционной системы MS Windows
						 */
						#if _WIN32 || _WIN64
							// Выполняем удаление переданного пути
							result = ::_wunlink(this->_fmk->convert(address).c_str());
						/**
						 * Для операционной системы не являющейся MS Windows
						 */
						#else
							// Выполняем удаление переданного пути
							result = ::unlink(address.c_str());
						#endif
					} break;
				}
			}
			// Если путь является символьной ссылкой
			if(actual && this->isLink(path)){
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					// Выполняем извлечение актуального значения адреса
					const string & address = this->realPath(path, false);
					// Если адрес получен правильный
					if(!address.empty())
						// Выполняем удаление переданного пути
						result = ::_wunlink(this->_fmk->convert(address).c_str());
				/**
				 * Для операционной системы не являющейся MS Windows
				 */
				#else
					// Выполняем удаление переданного пути
					result = ::unlink(path.c_str());
				#endif
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const ios_base::failure & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(path), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(path), log_t::flag_t::CRITICAL, error.what());
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
/**
 * @brief Метод создания символьной ссылки
 *
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
			 * Для операционной системы не являющейся MS Windows
			 */
			#if !_WIN32 && !_WIN64
				// Выполняем создание символьной ссылки
				::symlink(this->realPath(addr1).c_str(), this->realPath(addr2).c_str());
			/**
			 * Для операционной системы MS Windows
			 */
			#else
				// Получаем полный адрес пути
				const string & filename = this->realPath(addr1);
				// Если файл передан
				if(!filename.empty()){
					// Выполняем инициализацию результата
					HRESULT hres = ::CoInitialize(nullptr);
					// Создаём объект проверки наличия ярлыка
					IShellLinkW * psl = nullptr;
					// Выполняем инициализацию объекта для проверки ярлыков
					hres = ::CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW, reinterpret_cast <LPVOID *> (&psl));
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
								hres = ppf->Save(this->_fmk->convert(symlink).c_str(), TRUE);
							}
							// Выполняем очистку объекта проверки файла (прежде здесь дважды освобождался объект ярлыка)
							ppf->Release();
						}
						// Выполняем очистку объекта провверки файла
						psl->Release();
					}
					// Выполняем очистку объекта результата
					::CoUninitialize();
				}
			#endif
		/**
		 * Если возникает ошибка
		 */
		} catch(const ios_base::failure & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(addr1, addr2), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(addr1, addr2), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод создания жёстких ссылок
 *
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
			 * Для операционной системы не являющейся MS Windows
			 */
			#if !_WIN32 && !_WIN64
				// Если адрес на который нужно создать ссылку существует
				if(this->type(addr1) != type_t::NONE)
					// Выполняем создание символьной ссылки
					::link(this->realPath(addr1).c_str(), this->realPath(addr2).c_str());
			/**
			 * Для операционной системы MS Windows
			 */
			#else
				// Если адрес на который нужно создать ссылку существует
				if(this->type(addr1) != type_t::NONE){
					/**
					 * Жёсткие ссылки NTFS поддерживает, и заводятся они без особых прав.
					 * Ярлык заводится лишь тогда, когда жёсткую ссылку система отвергла
					 * (иная файловая система, каталог вместо файла)
					 */
					if(!::CreateHardLinkW(this->_fmk->convert(this->realPath(addr2)).c_str(), this->_fmk->convert(this->realPath(addr1)).c_str(), nullptr))
						// Выполняем создание обычный ярлык
						this->symLink(addr1, addr2);
				}
			#endif
		/**
		 * Если возникает ошибка
		 */
		} catch(const ios_base::failure & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(addr1, addr2), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(addr1, addr2), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод подмены целевого файла временным
 *
 * @param temporary адрес временного файла записи
 * @param filename  адрес целевого файла записи
 * @return          результат подмены
 */
bool awh::FS::replaceAddress(const string & temporary, const string & filename) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если адреса переданы
	if(!temporary.empty() && !filename.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * Для операционной системы MS Windows
			 */
			#if _WIN32 || _WIN64
				/**
				 * Выполняем подмену целевого файла
				 *
				 * @note rename() у MS Windows существующий файл не заменяет, а отвечает отказом.
				 *       Признак MOVEFILE_WRITE_THROUGH велит дождаться, пока подмена ляжет на носитель
				 */
				if(!(result = (::MoveFileExW(this->_fmk->convert(temporary).c_str(), this->_fmk->convert(filename).c_str(), (MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) != 0)))
					// Выводим в лог сообщение
					this->_log->print("Replacing \"%s\" with \"%s\" failed", log_t::flag_t::WARNING, filename.c_str(), temporary.c_str());
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#else
				// Выполняем подмену целевого файла
				if(!(result = (::rename(temporary.c_str(), filename.c_str()) == 0)))
					// Выводим в лог сообщение
					this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(temporary, filename), log_t::flag_t::CRITICAL, error.what());
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
/**
 * @brief Метод рекурсивного создания пути
 *
 * @param path полный путь для создания
 */
void awh::FS::makePath(const string & path) const noexcept {
	// Если путь передан
	if(!path.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем извлечение актуального значения адреса
			const string & address = this->realPath(path);
			// Если адрес получен правильный
			if(!address.empty()){
				// Указатель на сепаратор
				char * p = nullptr;
				// Получаем сепаратор
				const char sep = AWH_FS_SEPARATOR[0];
				// Создаём буфер входящих данных
				std::unique_ptr <char []> buffer(new char [address.size() + 1]);
				// Копируем переданный адрес в буфер
				::snprintf(buffer.get(), address.size() + 1, "%s", address.c_str());
				// Если последний символ является сепаратором тогда удаляем его
				if(buffer.get()[address.size() - 1] == sep)
					// Устанавливаем конец строки
					buffer.get()[address.size() - 1] = 0;
				// Переходим по всем символам
				for(p = buffer.get() + 1; * p; p++){
					// Если найден сепаратор
					if(* p == sep){
						// Сбрасываем указатель
						(* p) = 0;
						/**
						 * Для операционной системы не являющейся MS Windows
						 */
						#if !_WIN32 && !_WIN64
							// Создаем каталог
							::mkdir(buffer.get(), S_IRWXU);
						/**
						 * Для операционной системы MS Windows
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
				 * Для операционной системы не являющейся MS Windows
				 */
				#if !_WIN32 && !_WIN64
					// Создаем последний каталог
					::mkdir(buffer.get(), S_IRWXU);
				/**
				 * Для операционной системы MS Windows
				 */
				#else
					// Создаем последний каталог
					_wmkdir(this->_fmk->convert(string(buffer.get())).c_str());
				#endif
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const bad_alloc &) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(path), log_t::flag_t::CRITICAL, "Memory allocation error");
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
			#endif
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		/**
		 * Если возникает ошибка
		 */
		} catch(const ios_base::failure & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(path), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(path), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод создания каталога для хранения логов
 *
 * @param path  адрес для каталога
 * @param user  данные пользователя
 * @param group идентификатор группы
 * @return      результат создания каталога
 */
bool awh::FS::makeDir(const string & path, [[maybe_unused]] const string & user, [[maybe_unused]] const string & group) const noexcept {
	// Результат работы функции
	bool result = false;
	// Проверяем существует ли нужный нам каталог
	if((result = (this->type(path) == type_t::NONE))){
		// Создаем каталог
		this->makePath(path);
		/**
		 * Успех сообщается лишь тогда, когда каталог действительно создан: прежде отказ
		 * создания (нет прав, путь занят) отвечался успехом. Отказ установки владельца
		 * исхода не меняет, как не менял и прежде
		 */
		if((result = (this->type(path) == type_t::DIR))){
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#if !_WIN32 && !_WIN64
				// Устанавливаем права на каталог
				this->chown(path, user, group);
			#endif
		}
	}
	// Выводим результат создания каталога (ложь, если каталог уже существовал)
	return result;
}
/**
 * @brief Метод извлечения названия и расширения файла
 *
 * @param addr   адрес файла для извлечения его параметров
 * @param actual флаг формирования актуальных адресов
 * @param before флаг определения первой точки расширения слева
 */
std::pair <string, string> awh::FS::components(const string & addr, const bool actual, const bool before) const noexcept {
	// Результат работы функции
	std::pair <string, string> result;
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
			const uint8_t offset = (filename.back() == AWH_FS_SEPARATOR[0] ? 2 : 1);
			// Выполняем поиск разделителя каталога
			pos = filename.rfind(AWH_FS_SEPARATOR, filename.length() - static_cast <size_t> (offset));
			/**
			 * Начало названия в адресе. Адрес без разделителя (актуальный адрес
			 * несуществующего файла отдаётся как передан, например "file.txt")
			 * прежде давал пустой результат, теперь он весь считается названием
			 */
			const size_t start = (pos != string::npos ? (pos + 1) : 0);
			// Если переданный адрес является каталогом
			if(this->type(filename) == type_t::DIR)
				// Выполняем вывод названия каталога
				result.first = filename.substr(start, filename.length() - (start + static_cast <size_t> (offset) - 1));
			// Если переданный адрес не является каталогом
			else {
				// Извлекаем имя файла
				const string & name = filename.substr(start);
				// Ищем расширение файла
				if((pos = (before ? name.find('.') : name.rfind('.'))) != string::npos){
					// Устанавливаем имя файла
					result.first = name.substr(0, pos);
					// Устанавливаем расширение файла
					result.second = name.substr(pos + 1);
				// Устанавливаем только имя файла
				} else result.first = name;
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const ios_base::failure & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(addr, actual, before), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(addr, actual, before), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод получения прав доступа к файлу или каталогу
 *
 * @param path полный путь к файлу или каталогу
 * @return     запрашиваемые метаданные
 */
mode_t awh::FS::chmod(const string & path) const noexcept {
	// Результат работы функции
	mode_t result = 0;
	// Если путь к файлу или каталогу передан
	if(!path.empty() && (this->type(path) != type_t::NONE)){
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Выполняем извлечение актуального значения адреса
			const string & address = this->realPath(path);
			// Если адрес получен правильный
			if(!address.empty())
				// Извлекаем все атрибуты файла
				return static_cast <mode_t> (::GetFileAttributesW(this->_fmk->convert(address).c_str()));
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#else
			// Создаём объект информационных данных файла или каталога
			struct stat info;
			// Выполняем чтение информационных данных файла
			if(!(result = (::stat(path.c_str(), &info) == 0)) && (errno != 0))
				// Выводим в лог сообщение
				this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
			// Если информационные данные считаны удачно
			else result = (info.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод изменения прав доступа к файлу или каталогу
 *
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
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Выполняем извлечение актуального значения адреса
			const string & address = this->realPath(path);
			// Если адрес получен правильный
			if(!address.empty())
				// Выполняем установку атрибутов файла
				return ::SetFileAttributesW(this->_fmk->convert(address).c_str(), static_cast <DWORD> (mode));
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#else
			// Выполняем установку метаданных файла
			if(!(result = (::chmod(path.c_str(), mode) == 0)) && (errno != 0))
				// Выводим в лог сообщение
				this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки владельца на файл или каталог
 *
 * @param path  путь к файлу или каталогу для установки владельца
 * @param user  данные пользователя
 * @param group идентификатор группы
 * @return      результат работы функции
 */
bool awh::FS::chown(const string & path, const string & user, const string & group) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если путь передан
	if(!path.empty() && !user.empty() && (this->type(path) != type_t::NONE)){
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#if !_WIN32 && !_WIN64
			// Если группа пользователя передана
			if(!group.empty()){
				// Идентификатор пользователя
				const uid_t uid = this->_os.uid(user);
				// Идентификатор группы
				const gid_t gid = this->_os.gid(group);
				// Устанавливаем права на каталог
				if((result = (uid && gid))){
					// Выполняем установку владельца
					if(!(result = (::chown(path.c_str(), uid, gid) == 0)) && (errno != 0))
						// Выводим в лог сообщение
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
				}
			}
		/**
		 * Для операционной системы MS Windows
		 */
		#else
			// Тип SID-а
			SID_NAME_USE sidType;
			// Размер SID-а пользователя/группы и домена пользователя
			DWORD sidSize = 0, domainSize = 0;
			// Получаем адрес файла
			const wstring fileName = this->_fmk->convert(path);
			// Получаем имя пользователя
			const wstring userName = this->_fmk->convert(user);
			// Первый вызов — получаем размеры буферов
			::LookupAccountNameW(nullptr, userName.c_str(), nullptr, &sidSize, nullptr, &domainSize, &sidType);
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
					this->_log->debug(L"%s", __PRETTY_FUNCTION__, std::make_tuple(path, user), log_t::flag_t::CRITICAL, message);
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print(L"%s", log_t::flag_t::CRITICAL, message);
				#endif
				// Выводим результат
				return result;
			}
			// Инициализируем доменное имя пользователя
			wstring domain(domainSize, L'\0');
			// Выделяем память под SID и домен
			PSID pSid = (PSID) ::LocalAlloc(LPTR, sidSize);
			// Извлекаем SID пользователя и его доменное имя
			if(!::LookupAccountNameW(nullptr, userName.c_str(), pSid, &sidSize, &domain[0], &domainSize, &sidType)){
				// Освобождаем ресурсы
				::LocalFree(pSid);
				// Выводим пустой результат
				return result;
			}
			// Объект параметров доступа
			EXPLICIT_ACCESSW ea = {0};
			// Зануляем объект параметров доступа
			::ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
			// Устанавливаем новые права
			ea.grfAccessMode = SET_ACCESS;
			// Устанавливаем идентификатор пользователя
			ea.Trustee.ptstrName = (LPWSTR) pSid;
			// Устанавливаем права доступа для SID-пользователя
			ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
			// Устанавливаем тип инициатора пользователя
			ea.Trustee.TrusteeType = TRUSTEE_IS_USER;
			// Наследование для подпапок и файлов
			ea.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
			// Права: чтение + запись + выполнение (если файл исполняемый)
			ea.grfAccessPermissions = (GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE);
			// Дескриптор системы безопасности
			PSECURITY_DESCRIPTOR sd = nullptr;
			// Старые и новые параметры безопасности
			PACL pOldDACL = nullptr, pNewDACL = nullptr;
			// Получаем текущий DACL файла
			if(::GetNamedSecurityInfoW(fileName.c_str(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, &pOldDACL, nullptr, &sd) != ERROR_SUCCESS){
				// Создаём буфер сообщения ошибки
				wchar_t message[256] = {0};
				// Выполняем формирование текста ошибки
				::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 256, 0);
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug(L"%s", __PRETTY_FUNCTION__, std::make_tuple(path, user), log_t::flag_t::CRITICAL, message);
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print(L"%s", log_t::flag_t::CRITICAL, message);
				#endif
				// Освобождаем ресурсы
				::LocalFree(pSid);
				// Выводим пустой результат
				return result;
			}
			// Создаем новый DACL с добавленной записью
			if(::SetEntriesInAclW(1, &ea, pOldDACL, &pNewDACL) != ERROR_SUCCESS){
				// Создаём буфер сообщения ошибки
				wchar_t message[256] = {0};
				// Выполняем формирование текста ошибки
				::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 256, 0);
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug(L"%s", __PRETTY_FUNCTION__, std::make_tuple(path, user), log_t::flag_t::CRITICAL, message);
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print(L"%s", log_t::flag_t::CRITICAL, message);
				#endif
				// Освобождаем дескриптор системы безопасности
				::LocalFree(sd);
				// Освобождаем ресурсы
				::LocalFree(pSid);
				// Выводим пустой результат
				return result;
			}
			// Применяем новый DACL к файлу
			if(!(result = (::SetNamedSecurityInfoW((LPWSTR) fileName.c_str(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, pNewDACL, nullptr) == ERROR_SUCCESS))){
				// Создаём буфер сообщения ошибки
				wchar_t message[256] = {0};
				// Выполняем формирование текста ошибки
				::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 256, 0);
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug(L"%s", __PRETTY_FUNCTION__, std::make_tuple(path, user), log_t::flag_t::CRITICAL, message);
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print(L"%s", log_t::flag_t::CRITICAL, message);
				#endif
			}
			// Освобождаем дескриптор системы безопасности
			::LocalFree(sd);
			// Освобождаем ресурсы
			::LocalFree(pSid);
			// Очищаем новый объект параметров безопасности
			::LocalFree(pNewDACL);
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * Для операционной системы MS Windows
 */
#if _WIN32 && _WIN64
	/**
	 * @brief Метод установки позиции в файле
	 *
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
 * @brief Метод подсчёта размера файла/каталога
 *
 * @param path полный путь для подсчёта размера
 * @param ext  расширение файла если требуется фильтрация
 * @param rec  флаг рекурсивного перебора каталогов
 * @return     общий размер файла/каталога
 */
uintmax_t awh::FS::size(const string & path, const string & ext, const bool rec) const noexcept {
	// Результат работы функции
	uintmax_t result = 0;
	// Если путь для подсчёта передан
	if(!path.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем извлечение актуального значения адреса
			const string & address = this->realPath(path);
			// Если адрес получен правильный
			if(!address.empty()){
				/**
				 * Определяем тип переданного пути
				 */
				switch(static_cast <uint8_t> (this->type(address))){
					// Если полный путь является файлом
					case static_cast <uint8_t> (type_t::FILE): {
						/**
						 * Для операционной системы MS Windows
						 */
						#if _WIN32 || _WIN64
							// Создаём объект работы с файлом
							HANDLE file = ::CreateFileW(this->_fmk->convert(address).c_str(), GENERIC_READ, (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
							// Если открыть файл открыт нормально
							if(file != INVALID_HANDLE_VALUE){
								// Размер файла (GetFileSize без старшей части обрезал файлы больше 4 Гб)
								LARGE_INTEGER length;
								// Если размер файла получен
								if(::GetFileSizeEx(file, &length))
									// Получаем размер файла
									result = static_cast <uintmax_t> (length.QuadPart);
								// Выполняем закрытие файла
								::CloseHandle(file);
							}
						/**
						 * Для операционной системы не являющейся MS Windows
						 */
						#else
							// Структура проверка статистики
							struct stat info;
							// Выполняем извлечение данных статистики
							const int32_t status = ::stat(address.c_str(), &info);
							// Если тип определён
							if(status == 0)
								// Выводим размер файла
								result = static_cast <uintmax_t> (info.st_size);
							// Если прочитать файла не вышло
							else {
								// Открываем файл на чтение
								ifstream file(address, ios::in | ios::binary);
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
						 * Для операционной системы MS Windows
						 */
						#if _WIN32 || _WIN64
							// Открываем указанный каталог
							_WDIR * dir = ::_wopendir(this->_fmk->convert(address).c_str());
						/**
						 * Для операционной системы не являющейся MS Windows
						 */
						#else
							// Открываем указанный каталог
							DIR * dir = ::opendir(address.c_str());
						#endif
							// Если каталог открыт
							if(dir != nullptr){
								/**
								 * Для операционной системы MS Windows
								 */
								#if _WIN32 || _WIN64
									// Структура проверка статистики
									struct _stat info;
									// Создаем указатель на содержимое каталога
									struct _wdirent * ptr = nullptr;
									/**
									 * Выполняем чтение содержимого каталога
									 */
									while((ptr = ::_wreaddir(dir))){
								/**
								 * Для операционной системы не являющейся MS Windows
								 */
								#else
									// Структура проверка статистики
									struct stat info;
									// Создаем указатель на содержимое каталога
									struct dirent * ptr = nullptr;
									/**
									 * Выполняем чтение содержимого каталога
									 */
									while((ptr = ::readdir(dir))){
								#endif
										/**
										 * Для операционной системы MS Windows
										 */
										#if _WIN32 || _WIN64
											// Пропускаем названия текущие "." и внешние "..", так как идет рекурсия
											if(!::wcscmp(ptr->d_name, L".") || !::wcscmp(ptr->d_name, L".."))
												// Выполняем пропуск каталога
												continue;
											// Получаем адрес в виде строки
											const string & address = this->_fmk->format("%s%s%s", path.c_str(), AWH_FS_SEPARATOR, this->_fmk->convert(wstring(ptr->d_name)).c_str());
										/**
										 * Для операционной системы не являющейся MS Windows
										 */
										#else
											// Пропускаем названия текущие "." и внешние "..", так как идет рекурсия
											if(!::strcmp(ptr->d_name, ".") || !::strcmp(ptr->d_name, ".."))
												// Выполняем пропуск каталога
												continue;
											// Получаем адрес в виде строки
											const string & address = this->_fmk->format("%s%s%s", path.c_str(), AWH_FS_SEPARATOR, ptr->d_name);
										#endif
										/**
										 * Для операционной системы MS Windows
										 */
										#if _WIN32 || _WIN64
											// Если статистика извлечена
											if(!::_wstat(this->_fmk->convert(address).c_str(), &info)){
										/**
										 * Для операционной системы не являющейся MS Windows
										 */
										#else
											// Если статистика извлечена
											if(!::stat(address.c_str(), &info)){
										#endif
												// Если дочерний элемент является дирректорией
												if(S_ISDIR(info.st_mode))
													/**
													 * В каталоги по символьным ссылкам не заходим: ссылка на
													 * родительский каталог зацикливает рекурсию без конца
													 */
													result += ((rec && !this->isLink(address)) ? this->size(address, ext) : 0);
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
								 * Для операционной системы MS Windows
								 */
								#if _WIN32 || _WIN64
									// Закрываем открытый каталог
									::_wclosedir(dir);
								/**
								 * Для операционной системы не являющейся MS Windows
								 */
								#else
									// Закрываем открытый каталог
									::closedir(dir);
								#endif
							}
					} break;
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const ios_base::failure & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(path, ext, rec), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(path, ext, rec), log_t::flag_t::CRITICAL, error.what());
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
/**
 * @brief Метод подсчёта количество файлов в каталоге
 *
 * @param path путь для подсчёта
 * @param ext  расширение файла если требуется фильтрация
 * @param rec  флаг рекурсивного перебора каталогов
 * @return     количество файлов в каталоге
 */
uintmax_t awh::FS::count(const string & path, const string & ext, const bool rec) const noexcept {
	// Результат работы функции
	uintmax_t result = 0;
	// Если адрес каталога и расширение файлов переданы
	if(!path.empty() && this->isDir(path)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем извлечение актуального значения адреса
			const string & address = this->realPath(path);
			// Если адрес получен правильный
			if(!address.empty()){
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					// Открываем указанный каталог
					_WDIR * dir = ::_wopendir(this->_fmk->convert(address).c_str());
				/**
				 * Для операционной системы не являющейся MS Windows
				 */
				#else
					// Открываем указанный каталог
					DIR * dir = ::opendir(address.c_str());
				#endif
					// Если каталог открыт
					if(dir != nullptr){
						/**
						 * Для операционной системы MS Windows
						 */
						#if _WIN32 || _WIN64
							// Структура проверка статистики
							struct _stat info;
							// Создаем указатель на содержимое каталога
							struct _wdirent * ptr = nullptr;
							/**
							 * Выполняем чтение содержимого каталога
							 */
							while((ptr = ::_wreaddir(dir))){
						/**
						 * Для операционной системы не являющейся MS Windows
						 */
						#else
							// Структура проверка статистики
							struct stat info;
							// Создаем указатель на содержимое каталога
							struct dirent * ptr = nullptr;
							/**
							 * Выполняем чтение содержимого каталога
							 */
							while((ptr = ::readdir(dir))){
						#endif
								/**
								 * Для операционной системы MS Windows
								 */
								#if _WIN32 || _WIN64
									// Пропускаем названия текущие "." и внешние "..", так как идет рекурсия
									if(!::wcscmp(ptr->d_name, L".") || !::wcscmp(ptr->d_name, L".."))
										// Выполняем пропуск каталога
										continue;
									// Получаем адрес в виде строки
									const string & address = this->_fmk->format("%s%s%s", path.c_str(), AWH_FS_SEPARATOR, this->_fmk->convert(wstring(ptr->d_name)).c_str());
								/**
								 * Для операционной системы не являющейся MS Windows
								 */
								#else
									// Пропускаем названия текущие "." и внешние "..", так как идет рекурсия
									if(!::strcmp(ptr->d_name, ".") || !::strcmp(ptr->d_name, ".."))
										// Выполняем пропуск каталога
										continue;
									// Получаем адрес в виде строки
									const string & address = this->_fmk->format("%s%s%s", path.c_str(), AWH_FS_SEPARATOR, ptr->d_name);
								#endif
								/**
								 * Для операционной системы MS Windows
								 */
								#if _WIN32 || _WIN64
									// Если статистика извлечена
									if(!::_wstat(this->_fmk->convert(address).c_str(), &info)){
								/**
								 * Для операционной системы не являющейся MS Windows
								 */
								#else
									// Если статистика извлечена
									if(!::stat(address.c_str(), &info)){
								#endif
										// Если дочерний элемент является дирректорией
										if(S_ISDIR(info.st_mode))
											/**
											 * В каталоги по символьным ссылкам не заходим: ссылка на
											 * родительский каталог зацикливает рекурсию без конца
											 */
											result += ((rec && !this->isLink(address)) ? this->count(address, ext) : 0);
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
						 * Для операционной системы MS Windows
						 */
						#if _WIN32 || _WIN64
							// Закрываем открытый каталог
							::_wclosedir(dir);
						/**
						 * Для операционной системы не являющейся MS Windows
						 */
						#else
							// Закрываем открытый каталог
							::closedir(dir);
						#endif
					}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const ios_base::failure & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(path, ext, rec), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(path, ext, rec), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	// Выводим сообщение об ошибке
	} else this->_log->print("Path name: \"%s\" is not dir", log_t::flag_t::WARNING, path.c_str());
	// Выводим результат
	return result;
}
/**
 * @brief Метод усечения файла до заданной длины
 *
 * @param filename адрес файла который необходимо усечь
 * @param length   длина, до какой усекается файл
 * @return         результат усечения
 */
bool awh::FS::truncate(const string & filename, const uint64_t length) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если адрес файла передан
	if(!filename.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем извлечение актуального значения адреса
			const string & address = this->realPath(filename);
			// Если адрес получен правильный
			if(!address.empty()){
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					// Выполняем открытие файла на запись, отсутствующий файл заводится
					HANDLE file = ::CreateFileW(this->_fmk->convert(address).c_str(), GENERIC_WRITE, (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
					// Если файл открыт нормально
					if(file != INVALID_HANDLE_VALUE){
						// Создаём объект большого числа
						LARGE_INTEGER li;
						// Устанавливаем длину, до какой усекается файл
						li.QuadPart = static_cast <LONGLONG> (length);
						// Выполняем перенос позиции и усечение файла по ней
						result = ((::SetFilePointerEx(file, li, nullptr, FILE_BEGIN) != FALSE) && (::SetEndOfFile(file) != FALSE));
						// Выполняем закрытие файла
						::CloseHandle(file);
					}
					// Если усечение не выполнено
					if(!result)
						// Выводим в лог сообщение
						this->_log->print("Filename: \"%s\" is not truncated", log_t::flag_t::WARNING, address.c_str());
				/**
				 * Для операционной системы не являющейся MS Windows
				 */
				#else
					// Выполняем открытие файла на запись, отсутствующий файл заводится
					const int32_t fd = ::open(address.c_str(), O_WRONLY | O_CREAT, 0666);
					// Если файл не открыт
					if(fd < 0)
						// Выводим в лог сообщение
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
					// Если файл открыт удачно
					else {
						// Выполняем усечение файла до заданной длины
						if(!(result = (::ftruncate(fd, static_cast <off_t> (length)) == 0)))
							// Выводим в лог сообщение
							this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
						// Закрываем файловый дескриптор
						::close(fd);
					}
				#endif
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(filename, length), log_t::flag_t::CRITICAL, error.what());
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
/**
 * @brief Метод сброса записанного из ядра на носитель
 *
 * @param filename адрес файла который необходимо сбросить на носитель
 * @param durable  флаг доведения записанного до носителя, а не до накопителя
 * @return         результат сброса
 */
bool awh::FS::flush(const string & filename, [[maybe_unused]] const bool durable) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если адрес файла передан и он существует
	if(!filename.empty() && (this->type(filename) != type_t::NONE)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем извлечение актуального значения адреса
			const string & address = this->realPath(filename);
			// Если адрес получен правильный
			if(!address.empty()){
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					// Выполняем открытие файла на запись, иначе сброс система отвергает
					HANDLE file = ::CreateFileW(this->_fmk->convert(address).c_str(), GENERIC_WRITE, (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
					// Если файл открыт нормально
					if(file != INVALID_HANDLE_VALUE){
						// Выполняем сброс данных и сведений о файле (признак durable здесь ничего не меняет)
						if(!(result = (::FlushFileBuffers(file) != FALSE)))
							// Носитель, сброса не держащий, отказом не считается
							result = (::GetLastError() == ERROR_INVALID_FUNCTION);
						// Выполняем закрытие файла
						::CloseHandle(file);
					}
				/**
				 * Для операционной системы не являющейся MS Windows
				 */
				#else
					/**
					 * Файл открывается на чтение: сброс относится к самому файлу, а не к описателю,
					 * и так сбрасывается и файл без права записи, и каталог (после переименования)
					 */
					const int32_t fd = ::open(address.c_str(), O_RDONLY);
					// Если файл не открыт
					if(fd < 0)
						// Выводим в лог сообщение
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
					// Если файл открыт удачно
					else {
						/**
						 * Если операционной системой является MacOS X
						 */
						#if __APPLE__ || __MACH__
							/**
							 * Выполняем сброс записанного из ядра на носитель
							 *
							 * @note fsync под MacOS X выносит данные лишь в накопитель, не опустошая
							 *       его кэша, до носителя данные доводит только F_FULLFSYNC
							 */
							const int32_t status = (durable ? ((::fcntl(fd, F_FULLFSYNC, 0) == -1) ? ::fsync(fd) : 0) : ::fsync(fd));
						/**
						 * Если поддерживается синхронизированный ввод-вывод
						 */
						#elif defined(_POSIX_SYNCHRONIZED_IO) && (_POSIX_SYNCHRONIZED_IO > 0)
							// Выполняем сброс записанного из ядра на носитель
							const int32_t status = (durable ? ::fsync(fd) : ::fdatasync(fd));
						/**
						 * Для остальных операционных систем
						 */
						#else
							// Выполняем сброс записанного из ядра на носитель
							const int32_t status = ::fsync(fd);
						#endif
						// Если сброс отвечен отказом
						if(!(result = (status == 0))){
							// Носитель, сброса не держащий (канал, устройство), отказом не считается
							if(!(result = (errno == EINVAL)))
								// Выводим в лог сообщение
								this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
						}
						// Закрываем файловый дескриптор
						::close(fd);
					}
				#endif
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(filename, durable), log_t::flag_t::CRITICAL, error.what());
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
/**
 * @brief Метод чтения данных из файла
 *
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
			// Выполняем извлечение актуального значения адреса
			const string & address = this->realPath(filename);
			// Если адрес получен правильный
			if(!address.empty()){
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					// Создаём объект работы с файлом
					HANDLE file = ::CreateFileW(this->_fmk->convert(address).c_str(), GENERIC_READ, (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
					// Если открыть файл открыт нормально
					if(file != INVALID_HANDLE_VALUE){
						// Размер файла
						LARGE_INTEGER length;
						// Если размер файла получен
						if(::GetFileSizeEx(file, &length) && (length.QuadPart > 0)){
							// Количество прочитанных байт
							DWORD bytes = 0;
							// Устанавливаем размер буфера
							result.resize(static_cast <size_t> (length.QuadPart), 0);
							/**
							 * Выполняем чтение из файла в буфер данные
							 *
							 * @note Счётчик прочитанного обязан быть передан: без структуры
							 *       OVERLAPPED система пустого указателя на него не допускает
							 */
							if(!::ReadFile(file, static_cast <LPVOID> (result.data()), static_cast <DWORD> (result.size()), &bytes, nullptr))
								// Отказавшее чтение оставляет результат пустым
								result.clear();
							// Иначе устанавливаем фактически прочитанный размер
							else result.resize(static_cast <size_t> (bytes));
						}
						// Выполняем закрытие файла
						::CloseHandle(file);
					}
				/**
				 * Для операционной системы не являющейся MS Windows
				 */
				#else
					// Файловый дескриптор файла
					int32_t fd = -1;
					// Структура статистики файла
					struct stat info;
					// Если файл не открыт
					if((fd = ::open(address.c_str(), O_RDONLY)) < 0)
						// Выводим сообщение об ошибке
						this->_log->print("Filename: \"%s\" is broken", log_t::flag_t::WARNING, address.c_str());
					// Если файл открыт удачно
					else if(::fstat(fd, &info) < 0)
						// Выводим сообщение об ошибке
						this->_log->print("Filename: \"%s\" is unknown size", log_t::flag_t::WARNING, address.c_str());
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
							this->_log->print("Filename: \"%s\" is not read", log_t::flag_t::WARNING, address.c_str());
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
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const ios_base::failure & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(filename), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(filename), log_t::flag_t::CRITICAL, error.what());
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
/**
 * @brief Метод чтения данных из файла со смещением
 *
 * @param filename адрес файла для чтения
 * @param seek     тип смещения в файле
 * @param offset   смещение в файле
 * @return         бинарный буфер с прочитанными данными
 */
vector <char> awh::FS::read(const string & filename, const seek_t seek, const size_t offset) const noexcept {
	// Результат работы функции
	vector <char> result;
	// Если адрес файла передан и он существует
	if(!filename.empty() && this->isFile(filename)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем извлечение актуального значения адреса
			const string & address = this->realPath(filename);
			// Если адрес получен правильный
			if(!address.empty()){
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					// Создаём объект работы с файлом
					HANDLE file = ::CreateFileW(this->_fmk->convert(address).c_str(), GENERIC_READ, (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
					// Если открыть файл открыт нормально
					if(file != INVALID_HANDLE_VALUE){
						// Размер файла
						LARGE_INTEGER length;
						// Если размер файла получен
						if(::GetFileSizeEx(file, &length)){
							// Получаем размер файла
							const uint64_t size = static_cast <uint64_t> (length.QuadPart);
							// Позиция начала чтения (смещение от конца отсчитывается назад)
							const uint64_t position = (seek == seek_t::END ? (offset < size ? (size - offset) : 0) : static_cast <uint64_t> (offset));
							// Если позиция лежит внутри файла
							if(position < size){
								// Количество прочитанных байт
								DWORD bytes = 0;
								// Структура позиции чтения
								OVERLAPPED overlapped = {};
								// Устанавливаем младшую часть позиции
								overlapped.Offset = static_cast <DWORD> (position & 0xFFFFFFFF);
								// Устанавливаем старшую часть позиции
								overlapped.OffsetHigh = static_cast <DWORD> (position >> 32);
								// Выделяем память для результата
								result.resize(static_cast <size_t> (size - position), 0);
								// Выполняем чтение из файла в буфер данные
								if(!::ReadFile(file, static_cast <LPVOID> (result.data()), static_cast <DWORD> (result.size()), &bytes, &overlapped))
									// Отказавшее чтение оставляет результат пустым
									result.clear();
								// Иначе устанавливаем фактически прочитанный размер
								else result.resize(static_cast <size_t> (bytes));
							}
						}
						// Выполняем закрытие файла
						::CloseHandle(file);
					}
				/**
				 * Для операционной системы не являющейся MS Windows
				 */
				#else
					// Структура статистики файла
					struct stat info;
					// Выполняем открытие файла на чтение
					const int32_t fd = ::open(address.c_str(), O_RDONLY);
					// Если файл не открыт
					if(fd < 0)
						// Выводим сообщение об ошибке
						this->_log->print("Filename: \"%s\" is broken", log_t::flag_t::WARNING, address.c_str());
					// Если размер файла не получен
					else if(::fstat(fd, &info) < 0)
						// Выводим сообщение об ошибке
						this->_log->print("Filename: \"%s\" is unknown size", log_t::flag_t::WARNING, address.c_str());
					// Иначе продолжаем
					else if(info.st_size > 0) {
						// Получаем размер файла
						const uint64_t size = static_cast <uint64_t> (info.st_size);
						/**
						 * Позиция начала чтения. Для вновь открытого описателя текущая позиция есть
						 * начало файла, а смещение от конца отсчитывается назад, как и в AWH 5
						 */
						const uint64_t position = (seek == seek_t::END ? (offset < size ? (size - offset) : 0) : static_cast <uint64_t> (offset));
						// Если позиция лежит внутри файла (иначе результат пустой, без огромного выделения памяти)
						if(position < size){
							// Выделяем память для результата
							result.resize(static_cast <size_t> (size - position), 0);
							// Количество прочитанных байт
							size_t bytes = 0;
							/**
							 * Выполняем чтение, пока буфер не заполнен
							 */
							while(bytes < result.size()){
								// Выполняем чтение очередной части файла
								const ssize_t count = ::pread(fd, result.data() + bytes, result.size() - bytes, static_cast <off_t> (position + bytes));
								// Если чтение прервано сигналом
								if((count < 0) && (errno == EINTR))
									// Повторяем чтение
									continue;
								// Если файл закончился или чтение отказало
								if(count <= 0)
									// Выходим из цикла
									break;
								// Увеличиваем количество прочитанных байт
								bytes += static_cast <size_t> (count);
							}
							// Устанавливаем фактически прочитанный размер
							result.resize(bytes);
						}
					}
					// Если файл открыт
					if(fd > -1)
						// Закрываем файловый дескриптор
						::close(fd);
				#endif
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(filename, offset), log_t::flag_t::CRITICAL, error.what());
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
/**
 * @brief Метод чтения файла блоками с обратным вызовом
 *
 * @param filename адрес файла для чтения
 * @param size     размер блока для чтения
 * @param callback функция обратного вызова (буфер, размер, смещение, остаток), ложь останавливает чтение
 * @param offset   смещение в файле с которого следует начать чтение
 */
void awh::FS::read(const string & filename, const size_t size, function <bool (const void *, const size_t, const size_t, const size_t)> callback, const size_t offset) const noexcept {
	// Если адрес файла передан и он существует
	if(!filename.empty() && (callback != nullptr) && this->isFile(filename)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем извлечение актуального значения адреса
			const string & address = this->realPath(filename);
			// Если адрес получен правильный
			if(!address.empty()){
				// Размер блока чтения (нулевой размер заменяется размером страницы памяти)
				const size_t chunk = (size > 0 ? size : ::pagesize());
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					// Создаём объект работы с файлом
					HANDLE file = ::CreateFileW(this->_fmk->convert(address).c_str(), GENERIC_READ, (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
					// Если открыть файл открыт нормально
					if(file != INVALID_HANDLE_VALUE){
						// Размер файла
						LARGE_INTEGER length;
						// Если размер файла получен
						if(::GetFileSizeEx(file, &length) && (length.QuadPart > 0)){
							// Получаем общий размер файла
							const uint64_t total = static_cast <uint64_t> (length.QuadPart);
							// Выделяем буфер блока
							vector <char> buffer(static_cast <size_t> (std::min(static_cast <uint64_t> (chunk), total)), 0);
							/**
							 * Выполняем чтение файла блоками
							 */
							for(uint64_t position = offset; position < total;){
								// Количество прочитанных байт
								DWORD bytes = 0;
								// Структура позиции чтения
								OVERLAPPED overlapped = {};
								// Устанавливаем младшую часть позиции
								overlapped.Offset = static_cast <DWORD> (position & 0xFFFFFFFF);
								// Устанавливаем старшую часть позиции
								overlapped.OffsetHigh = static_cast <DWORD> (position >> 32);
								// Выполняем чтение очередного блока
								if(!::ReadFile(file, static_cast <LPVOID> (buffer.data()), static_cast <DWORD> (std::min(static_cast <uint64_t> (buffer.size()), total - position)), &bytes, &overlapped) || (bytes == 0))
									// Выходим из цикла
									break;
								// Увеличиваем позицию чтения
								position += static_cast <uint64_t> (bytes);
								// Выводим прочитанный блок, ложь останавливает чтение
								if(!callback(buffer.data(), static_cast <size_t> (bytes), static_cast <size_t> (position - bytes), static_cast <size_t> (total - position)))
									// Выходим из цикла
									break;
							}
						}
						// Выполняем закрытие файла
						::CloseHandle(file);
					}
				/**
				 * Для операционной системы не являющейся MS Windows
				 */
				#else
					// Структура статистики файла
					struct stat info;
					// Выполняем открытие файла на чтение
					const int32_t fd = ::open(address.c_str(), O_RDONLY);
					// Если файл не открыт
					if(fd < 0)
						// Выводим сообщение об ошибке
						this->_log->print("Filename: \"%s\" is broken", log_t::flag_t::WARNING, address.c_str());
					// Если размер файла не получен
					else if(::fstat(fd, &info) < 0)
						// Выводим сообщение об ошибке
						this->_log->print("Filename: \"%s\" is unknown size", log_t::flag_t::WARNING, address.c_str());
					// Иначе продолжаем
					else if(info.st_size > 0) {
						// Получаем общий размер файла
						const uint64_t total = static_cast <uint64_t> (info.st_size);
						// Выделяем буфер блока
						vector <char> buffer(static_cast <size_t> (std::min(static_cast <uint64_t> (chunk), total)), 0);
						/**
						 * Выполняем чтение файла блоками
						 */
						for(uint64_t position = offset; position < total;){
							// Выполняем чтение очередного блока
							const ssize_t bytes = ::pread(fd, buffer.data(), static_cast <size_t> (std::min(static_cast <uint64_t> (buffer.size()), total - position)), static_cast <off_t> (position));
							// Если чтение прервано сигналом
							if((bytes < 0) && (errno == EINTR))
								// Повторяем чтение
								continue;
							// Если файл закончился или чтение отказало
							if(bytes <= 0)
								// Выходим из цикла
								break;
							// Увеличиваем позицию чтения
							position += static_cast <uint64_t> (bytes);
							// Выводим прочитанный блок, ложь останавливает чтение
							if(!callback(buffer.data(), static_cast <size_t> (bytes), static_cast <size_t> (position - bytes), static_cast <size_t> (total - position)))
								// Выходим из цикла
								break;
						}
					}
					// Если файл открыт
					if(fd > -1)
						// Закрываем файловый дескриптор
						::close(fd);
				#endif
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(filename, size, offset), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод записи в файл бинарных данных
 *
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
			// Выполняем извлечение актуального значения адреса
			const string & address = this->realPath(filename);
			// Если адрес получен правильный
			if(!address.empty()){
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					/**
					 * Выполняем открытие файла на запись
					 *
					 * @note Файл открывается с усечением (CREATE_ALWAYS), как и у POSIX: прежнее
					 *       открытие OPEN_ALWAYS оставляло хвост прежнего содержимого длиннее нового
					 */
					HANDLE file = ::CreateFileW(this->_fmk->convert(address).c_str(), GENERIC_WRITE, (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
					// Если открыть файл открыт нормально
					if(file != INVALID_HANDLE_VALUE){
						// Количество записанных байт
						DWORD written = 0;
						// Выполняем запись данных в файл
						::WriteFile(file, static_cast <LPCVOID> (buffer), static_cast <DWORD> (size), &written, nullptr);
						// Выполняем закрытие файла
						::CloseHandle(file);
					}
				/**
				 * Для операционной системы не являющейся MS Windows
				 */
				#else
					// Файловый поток для записи
					ofstream file(address, ios::binary);
					// Если файл открыт на запись
					if(file.is_open()){
						// Выполняем запись данных в файл
						file.write(buffer, size);
						// Закрываем файл
						file.close();
					}
				#endif
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const ios_base::failure & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(filename, buffer, size), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(filename, buffer, size), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод записи в файл бинарных данных по смещению
 *
 * @param filename адрес файла в который необходимо выполнить запись
 * @param buffer   бинарный буфер который необходимо записать в файл
 * @param size     размер бинарного буфера для записи в файл
 * @param seek     тип смещения в файле
 * @param offset   смещение в файле
 * @return         результат записи (легли ли данные в файл целиком)
 */
bool awh::FS::write(const string & filename, const char * buffer, const size_t size, const seek_t seek, const size_t offset) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если параметры для записи переданы
	if(!filename.empty() && (buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем извлечение актуального значения адреса
			const string & address = this->realPath(filename);
			// Если адрес получен правильный
			if(!address.empty()){
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					// Выполняем открытие файла на запись без усечения, отсутствующий файл заводится
					HANDLE file = ::CreateFileW(this->_fmk->convert(address).c_str(), GENERIC_WRITE, (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
					// Если открыть файл открыт нормально
					if(file != INVALID_HANDLE_VALUE){
						// Создаём объект большого числа
						LARGE_INTEGER li;
						// Устанавливаем смещение в файле
						li.QuadPart = static_cast <LONGLONG> (offset);
						// Выполняем перенос позиции записи (смещение от конца отсчитывается вперёд, как в AWH 5)
						if(::SetFilePointerEx(file, li, nullptr, (seek == seek_t::END ? FILE_END : FILE_BEGIN)) != FALSE){
							// Количество записанных байт
							size_t bytes = 0;
							/**
							 * Выполняем запись, пока все данные не легли в файл
							 */
							while(bytes < size){
								// Количество записанных байт за один вызов
								DWORD written = 0;
								// Выполняем запись очередной части данных
								if(!::WriteFile(file, static_cast <LPCVOID> (buffer + bytes), static_cast <DWORD> (std::min(size - bytes, static_cast <size_t> (0x7FFFFFFF))), &written, nullptr) || (written == 0))
									// Выходим из цикла
									break;
								// Увеличиваем количество записанных байт
								bytes += static_cast <size_t> (written);
							}
							// Запоминаем результат записи
							result = (bytes == size);
						}
						// Выполняем закрытие файла
						::CloseHandle(file);
					}
					// Если запись не выполнена
					if(!result)
						// Выводим в лог сообщение
						this->_log->print("Filename: \"%s\" is not written", log_t::flag_t::WARNING, address.c_str());
				/**
				 * Для операционной системы не являющейся MS Windows
				 */
				#else
					// Выполняем открытие файла на запись без усечения, отсутствующий файл заводится
					const int32_t fd = ::open(address.c_str(), O_WRONLY | O_CREAT, 0666);
					// Если файл не открыт
					if(fd < 0)
						// Выводим в лог сообщение
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
					// Если файл открыт удачно
					else {
						// Выполняем перенос позиции записи (смещение от конца отсчитывается вперёд, как в AWH 5)
						if(::lseek(fd, static_cast <off_t> (offset), (seek == seek_t::END ? SEEK_END : SEEK_SET)) < 0)
							// Выводим в лог сообщение
							this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
						// Если позиция установлена
						else {
							// Количество записанных байт
							size_t bytes = 0;
							/**
							 * Выполняем запись, пока все данные не легли в файл
							 */
							while(bytes < size){
								// Выполняем запись очередной части данных
								const ssize_t count = ::write(fd, buffer + bytes, size - bytes);
								// Если запись прервана сигналом
								if((count < 0) && (errno == EINTR))
									// Повторяем запись
									continue;
								// Если запись отказала
								if(count <= 0){
									// Выводим в лог сообщение
									this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
									// Выходим из цикла
									break;
								}
								// Увеличиваем количество записанных байт
								bytes += static_cast <size_t> (count);
							}
							// Запоминаем результат записи
							result = (bytes == size);
						}
						// Закрываем файловый дескриптор
						::close(fd);
					}
				#endif
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(filename, size, offset), log_t::flag_t::CRITICAL, error.what());
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
/**
 * @brief Метод добавления в файл бинарных данных
 *
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
			// Выполняем извлечение актуального значения адреса
			const string & address = this->realPath(filename);
			// Если адрес получен правильный
			if(!address.empty()){
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					// Выполняем открытие файла на добавление
					HANDLE file = ::CreateFileW(this->_fmk->convert(address).c_str(), FILE_APPEND_DATA, (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
					// Если открыть файл открыт нормально
					if(file != INVALID_HANDLE_VALUE){
						// Количество записанных байт
						DWORD written = 0;
						// Выполняем добавление данных в файл
						::WriteFile(file, static_cast <LPCVOID> (buffer), static_cast <DWORD> (size), &written, nullptr);
						// Выполняем закрытие файла
						::CloseHandle(file);
					}
				/**
				 * Для операционной системы не являющейся MS Windows
				 */
				#else
					// Файловый поток для добавления
					ofstream file(address, (ios::binary | ios::app));
					// Если файл открыт на добавление
					if(file.is_open()){
						// Выполняем добавление данных в файл
						file.write(buffer, size);
						// Закрываем файл
						file.close();
					}
				#endif
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const ios_base::failure & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(filename, buffer, size), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(filename, buffer, size), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод рекурсивного получения всех строк файла
 *
 * @param filename адрес файла для чтения
 * @param callback функция обратного вызова
 */
void awh::FS::readFile(const string & filename, function <void (const string &)> callback) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Вызываем метод рекурсивного получения всех строк файла, старым способом
			this->readFile2(filename, callback);
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#else
			// Если адрес файла передан
			if(!filename.empty() && this->isFile(filename)){
				// Выполняем извлечение актуального значения адреса
				const string & address = this->realPath(filename);
				// Если адрес получен правильный
				if(!address.empty()){
					// Файловый дескриптор файла
					int32_t fd = -1;
					// Структура статистики файла
					struct stat info;
					// Если файл не открыт
					if((fd = ::open(address.c_str(), O_RDONLY)) < 0)
						// Выводим сообщение об ошибке
						this->_log->print("Filename: \"%s\" is broken", log_t::flag_t::WARNING, address.c_str());
					// Если файл открыт удачно
					else if(::fstat(fd, &info) < 0)
						// Выводим сообщение об ошибке
						this->_log->print("Filename: \"%s\" is unknown size", log_t::flag_t::WARNING, address.c_str());
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
							this->_log->print("Filename: \"%s\" is not read", log_t::flag_t::WARNING, address.c_str());
						// Если файл прочитан удачно
						else if(buffer != nullptr)
							// Выполняем разбор отображённого файла на строки
							::lines(reinterpret_cast <const char *> (buffer), static_cast <size_t> (info.st_size), callback);
						// Если отображение файла в памяти выполнено
						if(buffer != MAP_FAILED)
							// Выполняем удаление сопоставления для указанного диапазона адресов
							::munmap(buffer, (length + offset - paOffset));
					}
					// Если файл открыт
					if(fd > -1)
						// Закрываем файловый дескриптор
						::close(fd);
				}
			// Выводим сообщение об ошибке
			} else this->_log->print("Filename: \"%s\" is not found", log_t::flag_t::WARNING, filename.c_str());
		#endif
	/**
	 * Если возникает ошибка
	 */
	} catch(const ios_base::failure & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(filename), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(filename), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод рекурсивного получения всех строк файла (стандартным способом)
 *
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
			 * @brief Функция чтения данных из бинарного буфера
			 *
			 * @param buffer буфер откуда производится чтение
			 */
			auto readFn = [&callback](vector <char> & buffer) noexcept -> void {
				// Устанавливаем буфер
				if(!buffer.empty()){
					// Выполняем разбор буфера на строки
					::lines(buffer.data(), buffer.size(), callback);
					// Очищаем буфер данных
					buffer.clear();
					// Освобождаем выделенную память
					vector <char> ().swap(buffer);
				}
			};
			// Выполняем извлечение актуального значения адреса
			const string & address = this->realPath(filename);
			// Если адрес получен правильный
			if(!address.empty()){
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					// Создаём объект работы с файлом
					HANDLE file = ::CreateFileW(this->_fmk->convert(address).c_str(), GENERIC_READ, (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
					// Если открыть файл открыт нормально
					if(file != INVALID_HANDLE_VALUE){
						// Размер файла
						LARGE_INTEGER length;
						// Если размер файла получен
						if(::GetFileSizeEx(file, &length) && (length.QuadPart > 0)){
							// Количество прочитанных байт
							DWORD bytes = 0;
							// Устанавливаем размер буфера
							vector <char> buffer(static_cast <size_t> (length.QuadPart));
							// Выполняем чтение из файла в буфер данные
							if(::ReadFile(file, static_cast <LPVOID> (buffer.data()), static_cast <DWORD> (buffer.size()), &bytes, nullptr)){
								// Устанавливаем фактически прочитанный размер
								buffer.resize(static_cast <size_t> (bytes));
								// Выполняем чтение данных из буфера
								readFn(buffer);
							}
						}
						// Выполняем закрытие файла
						::CloseHandle(file);
					}
				/**
				 * Для операционной системы не являющейся MS Windows
				 */
				#else
					// Структура проверка статистики
					struct stat info;
					// Общий размер файла
					uintmax_t size = 0;
					// Если тип определён
					if(::stat(address.c_str(), &info) == 0)
						// Получаем размер файла
						size = static_cast <uintmax_t> (info.st_size);
					// Открываем файл на чтение
					ifstream file(address, ios::in | ios::binary);
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
						// Устанавливаем фактически прочитанный размер
						buffer.resize(static_cast <size_t> (file.gcount()));
						// Выполняем чтение данных из буфера
						readFn(buffer);
						// Закрываем файл
						file.close();
					}
				#endif
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const ios_base::failure & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(filename), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(filename), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	// Выводим сообщение об ошибке
	} else this->_log->print("Filename: \"%s\" is not found", log_t::flag_t::WARNING, filename.c_str());
}
/**
 * @brief Метод рекурсивного получения всех строк файла (построчным методом)
 *
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
			// Выполняем извлечение актуального значения адреса
			const string & address = this->realPath(filename);
			// Если адрес получен правильный
			if(!address.empty()){
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					// Открываем файл на чтение
					ifstream file(this->_fmk->convert(address).c_str(), ios::in | ios::binary);
					// Если файл открыт
					if(file.is_open()){
						// Результат полученный из потока
						string result = "";
						/**
						 * Выполняем чтение данных из потока
						 */
						while(::getline(file, result))
							// Выводим полученный результат
							std::apply(callback, std::make_tuple(result));
						// Закрываем файл
						file.close();
					}
				/**
				 * Для операционной системы не являющейся MS Windows
				 */
				#else
					// Открываем файл на чтение
					ifstream file(address, ios::in | ios::binary);
					// Если файл открыт
					if(file.is_open()){
						// Результат полученный из потока
						string result = "";
						/**
						 * Выполняем чтение данных из потока
						 */
						while(getline(file, result))
							// Выводим полученный результат
							std::apply(callback, std::make_tuple(result));
						// Закрываем файл
						file.close();
					}
				#endif
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const ios_base::failure & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(filename), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(filename), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	// Выводим сообщение об ошибке
	} else this->_log->print("Filename: \"%s\" is not found", log_t::flag_t::WARNING, filename.c_str());
}
/**
 * @brief Метод получения содержимого файла блоками
 *
 * @param filename адрес файла для чтения
 * @param size     размер блока для чтения (ноль - размер страницы памяти)
 * @param callback функция обратного вызова
 */
void awh::FS::readFile(const string & filename, const size_t size, function <void (const void *, const size_t)> callback) const noexcept {
	// Если функция обратного вызова передана
	if(callback != nullptr){
		// Выполняем чтение файла блоками до самого конца
		this->read(filename, size, [&callback](const void * buffer, const size_t size, const size_t, const size_t) noexcept -> bool {
			// Выводим прочитанный блок
			std::apply(callback, std::make_tuple(buffer, size));
			// Продолжаем чтение
			return true;
		});
	}
}
/**
 * @brief Метод обхода файлов во всех подкаталогах с досрочной остановкой
 *
 * @param path     путь до каталога
 * @param ext      расширение файла по которому идет фильтрация
 * @param rec      флаг рекурсивного перебора каталогов
 * @param callback функция обратного вызова, ложь останавливает обход
 * @param actual   флаг формирования актуальных адресов
 * @return         результат обхода (довершён ли обход до конца)
 */
bool awh::FS::walkDir(const string & path, const string & ext, const bool rec, function <bool (const string &)> callback, const bool actual) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если адрес каталога и расширение файлов переданы
	if(!path.empty() && this->isDir(path)){
		/**
		 * @brief Прототип функции запроса файлов в каталоге
		 *
		 * @param путь до каталога
		 * @param расширение файла по которому идет фильтрация
		 * @param флаг рекурсивного перебора каталогов
		 * @return признак продолжения обхода
		 */
		function <bool (const string &, const string &, const bool)> readFn;
		// Глубина вложенности обхода
		size_t depth = 0;
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#if !_WIN32 && !_WIN64
			// Цепочка открытых каталогов (устройство и индексный узел) для обнаружения петель
			vector <pair <dev_t, ino_t>> chain;
		#endif
		/**
		 * @brief Функция запроса файлов в каталоге
		 *
		 * @param path путь до каталога
		 * @param ext  расширение файла по которому идет фильтрация
		 * @param rec  флаг рекурсивного перебора каталогов
		 * @return     признак продолжения обхода
		 */
		readFn = [&](const string & path, const string & ext, const bool rec) noexcept -> bool {
			// Признак продолжения обхода
			bool next = true;
			/**
			 * Выполняем перехват ошибок
			 */
			try {
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					// Открываем указанный каталог
					_WDIR * dir = ::_wopendir(this->_fmk->convert(path).c_str());
				/**
				 * Для операционной системы не являющейся MS Windows
				 */
				#else
					// Открываем указанный каталог
					DIR * dir = ::opendir(path.c_str());
				#endif
					// Если каталог не открыт, то корень считается отказом, а подкаталог пропускается как прежде
					if(dir == nullptr)
						// Выводим признак продолжения обхода
						return (depth > 0);
					// Если каталог открыт
					else {
						/**
						 * Для операционной системы MS Windows
						 */
						#if _WIN32 || _WIN64
							// Структура проверка статистики
							struct _stat info;
							// Создаем указатель на содержимое каталога
							struct _wdirent * ptr = nullptr;
							/**
							 * Выполняем чтение содержимого каталога
							 */
							while(next && (ptr = ::_wreaddir(dir))){
						/**
						 * Для операционной системы не являющейся MS Windows
						 */
						#else
							// Структура проверка статистики
							struct stat info;
							// Создаем указатель на содержимое каталога
							struct dirent * ptr = nullptr;
							/**
							 * Выполняем чтение содержимого каталога
							 */
							while(next && (ptr = ::readdir(dir))){
						#endif
								/**
								 * Для операционной системы MS Windows
								 */
								#if _WIN32 || _WIN64
									// Пропускаем названия текущие "." и внешние "..", так как идет рекурсия
									if(!::wcscmp(ptr->d_name, L".") || !::wcscmp(ptr->d_name, L".."))
										// Выполняем пропуск каталога
										continue;
									// Получаем адрес в виде строки
									const string & address = this->_fmk->format("%s%s%s", path.c_str(), AWH_FS_SEPARATOR, this->_fmk->convert(wstring(ptr->d_name)).c_str());
								/**
								 * Для операционной системы не являющейся MS Windows
								 */
								#else
									// Пропускаем названия текущие "." и внешние "..", так как идет рекурсия
									if(!::strcmp(ptr->d_name, ".") || !::strcmp(ptr->d_name, ".."))
										// Выполняем пропуск каталога
										continue;
									// Получаем адрес в виде строки
									const string & address = this->_fmk->format("%s%s%s", path.c_str(), AWH_FS_SEPARATOR, ptr->d_name);
								#endif
								/**
								 * Для операционной системы MS Windows
								 */
								#if _WIN32 || _WIN64
									// Если статистика извлечена
									if(!::_wstat(this->_fmk->convert(address).c_str(), &info)){
								/**
								 * Для операционной системы не являющейся MS Windows
								 */
								#else
									// Если статистика извлечена
									if(!::stat(address.c_str(), &info)){
								#endif
										// Если дочерний элемент является дирректорией
										if(S_ISDIR(info.st_mode)){
											// Продолжаем обработку следующих каталогов
											if(rec){
												/**
												 * Для операционной системы не являющейся MS Windows
												 */
												#if !_WIN32 && !_WIN64
													/**
													 * Символьные ссылки на каталоги обходим как прежде, но каталог уже
													 * открытый выше по цепочке рекурсии пропускаем: ссылка на родителя
													 * иначе зацикливает обход без конца
													 */
													bool loop = false;
													// Выполняем перебор всей цепочки открытых каталогов
													for(auto & item : chain){
														// Если каталог уже открыт выше по цепочке
														if((item.first == info.st_dev) && (item.second == info.st_ino)){
															// Запоминаем что обнаружена петля
															loop = true;
															// Выходим из цикла
															break;
														}
													}
													// Если петля обнаружена, пропускаем каталог
													if(loop)
														// Выполняем пропуск каталога
														continue;
													// Добавляем каталог в цепочку
													chain.emplace_back(info.st_dev, info.st_ino);
												#endif
												// Увеличиваем глубину вложенности
												depth++;
												// Выполняем функцию обратного вызова
												next = readFn(address, ext, rec);
												// Уменьшаем глубину вложенности
												depth--;
												/**
												 * Для операционной системы не являющейся MS Windows
												 */
												#if !_WIN32 && !_WIN64
													// Удаляем каталог из цепочки
													chain.pop_back();
												#endif
											}
											// Выводим данные каталога как он есть
											else next = std::apply(callback, std::make_tuple(this->realPath(address, actual)));
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
													next = std::apply(callback, std::make_tuple(this->realPath(address, actual)));
											}
										// Если дочерний элемент является файлом то выводим его
										} else next = std::apply(callback, std::make_tuple(this->realPath(address, actual)));
									// Если статистика не извлечена
									} else {
										/**
										 * Если операционной системой является MacOS X
										 */
										#if __APPLE__ || __MACH__
											// Если адрес является ссылкой
											if(this->isLink(address)){
												// Если дочерний элемент является файлом и расширение файла указано то выводим его
												if(!ext.empty()){
													// Получаем расширение файла
													const string & extension = this->_fmk->format(".%s", ext.c_str());
													// Получаем длину адреса
													const size_t length = extension.length();
													// Если расширение не выше полного адреса
													if(address.length() > length){
														// Если расширение файла найдено
														if(this->_fmk->compare(address.substr(address.length() - length, length), extension))
															// Выводим полный путь файла
															next = std::apply(callback, std::make_tuple(this->realPath(address, actual)));
													}
												// Если дочерний элемент является файлом то выводим его
												} else next = std::apply(callback, std::make_tuple(this->realPath(address, actual)));
											}
										#endif
									}
						}
						/**
						 * Для операционной системы MS Windows
						 */
						#if _WIN32 || _WIN64
							// Закрываем открытый каталог
							::_wclosedir(dir);
						/**
						 * Для операционной системы не являющейся MS Windows
						 */
						#else
							// Закрываем открытый каталог
							::closedir(dir);
						#endif
					}
			/**
			 * Если возникает ошибка
			 */
			} catch(const ios_base::failure & error) {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(path, ext, rec), log_t::flag_t::CRITICAL, error.what());
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
				// Обход прерван ошибкой
				next = false;
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(path, ext, rec), log_t::flag_t::CRITICAL, error.what());
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
				// Обход прерван ошибкой
				next = false;
			}
			// Выводим признак продолжения обхода
			return next;
		};
		// Выполняем извлечение актуального значения адреса
		const string & address = this->realPath(path);
		// Если адрес получен правильный
		if(!address.empty()){
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#if !_WIN32 && !_WIN64
				// Структура проверка статистики
				struct stat info;
				// Если статистика корневого каталога извлечена
				if(!::stat(address.c_str(), &info))
					// Добавляем корневой каталог в цепочку
					chain.emplace_back(info.st_dev, info.st_ino);
			#endif
			// Запрашиваем данные первого каталога
			result = readFn(address, ext, rec);
		}
	// Выводим сообщение об ошибке
	} else this->_log->print("Path name: \"%s\" is not found", log_t::flag_t::WARNING, path.c_str());
	// Выводим результат
	return result;
}
/**
 * @brief Метод рекурсивного получения файлов во всех подкаталогах
 *
 * @param path     путь до каталога
 * @param ext      расширение файла по которому идет фильтрация
 * @param rec      флаг рекурсивного перебора каталогов
 * @param callback функция обратного вызова
 * @param actual   флаг формирования актуальных адресов
 */
void awh::FS::readDir(const string & path, const string & ext, const bool rec, function <void (const string &)> callback, const bool actual) const noexcept {
	// Выполняем обход каталога без досрочной остановки
	this->walkDir(path, ext, rec, [&callback](const string & filename) noexcept -> bool {
		// Выводим полученный адрес
		std::apply(callback, std::make_tuple(filename));
		// Продолжаем обход
		return true;
	}, actual);
}
/**
 * @brief Метод рекурсивного чтения файлов во всех подкаталогах
 *
 * @param path     путь до каталога
 * @param ext      расширение файла по которому идет фильтрация
 * @param rec      флаг рекурсивного перебора каталогов
 * @param callback функция обратного вызова
 * @param actual   флаг формирования актуальных адресов
 */
void awh::FS::readPath(const string & path, const string & ext, const bool rec, function <void (const string &, const string &)> callback, const bool actual) const noexcept {
	// Если адрес каталога и расширение файлов переданы
	if(!path.empty() && this->isDir(path)){
		// Выполняем извлечение актуального значения адреса
		const string & address = this->realPath(path);
		// Если адрес получен правильный
		if(!address.empty())
			// Переходим по всему списку файлов в каталоге
			this->readDir(address, ext, rec, [&](const string & filename) noexcept -> void {
				// Выполняем считывание всех строк текста
				this->readFile(filename, [&](const string & text) noexcept -> void {
					// Если текст получен
					if(!text.empty())
						// Выводим функцию обратного вызова
						std::apply(callback, std::make_tuple(text, filename));
				});
			}, actual);
	// Выводим сообщение об ошибке
	} else this->_log->print("Path name: \"%s\" is not found", log_t::flag_t::WARNING, path.c_str());
}
/**
 * @brief конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::FS::FS(const fmk_t * fmk, const log_t * log) noexcept : _pid(::getpid()), _fmk(fmk), _log(log) {}
