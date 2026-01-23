/**
 * @file: fs.cpp
 * @date: 2026-01-23
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
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
	/**
	 * Подключаем стандартные модули
	 */
	#include <objbase.h>
	#include <shlobj.h>
	#include <tchar.h>
	#include <strsafe.h>
#endif

/**
 * Стандартные модули
 */
#include <fstream>
#include <codecvt>
#include <sstream>
#include <cstdlib>
#include <fcntl.h>
#include <dirent.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>

/**
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
	/**
	 * Подключаем стандартные модули
	 */
	#include <sddl.h>
	#include <conio.h>
	#include <aclapi.h>
	#include <direct.h>
/**
 * Для операционной системы не являющейся MS Windows
 */
#else
	/**
	 * Подключаем стандартные модули
	 */
	#include <pwd.h>
	#include <unistd.h>
	#include <sys/mman.h>
#endif

/**
 * Если операционной системой является MacOS X
 */
#if __APPLE__ || __MACH__
	/**
	 * Подключаем стандартные модули
	 */
	#include <TargetConditionals.h>
	/**
	 * Если целевая платформа является MacOS X
	 */
	#if TARGET_OS_MAC && !TARGET_OS_IPHONE
		/**
		 * Включаем поддержку Objective-C автоматического управления памятью
		 */
		#define __AWH_USE_MACOS_ALIAS_RESOLUTION__ 1
		// ← #import допустим в .cpp при -x objective-c++
		#import <Foundation/Foundation.h>
	#endif
#endif

/**
 * Подключаем заголовочный файл
 */
#include <sys/fs.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Инкапсулируем статические типы данных в пространство имён
 */
namespace {
	/**
	 * Подписываемся на пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Функция получения полного пути файла или каталога
	 *
	 * @param input входная строка пути
	 * @param log   объект работы с логами
	 * @return      полная строка пути
	 */
	static string fullpath(const string_view input, const log_t * log) noexcept {
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Если входная строка пуста, возвращаем текущий рабочий каталог
			if(input.empty()){
				// Временный буфер для текущего рабочего каталога
				char cwd[PATH_MAX];
				// Получаем текущий рабочий каталог
				return (::getcwd(cwd, sizeof(cwd)) ? cwd : AWH_FS_SEPARATOR);
			}
			// 1. Обрабатываем вход: превращаем в абсолютный путь как строку
			string path = "";
			// Если путь уже абсолютный
			if(input.front() == AWH_FS_SEPARATOR[0])
				// Уже абсолютный
				path = input;
			// Если путь начинается с '~'
			else if(input.front() == '~') {
				// Выполняем поиск следующего слэша ~/ или ~
				const size_t pos = input.find(AWH_FS_SEPARATOR[0], 1);
				// Получаем домашний каталог пользователя
				const char * home = ::getenv("HOME");
				// Если переменная окружения не установлена
				if(home == nullptr){
					// Получаем информацию о пользователе из системы
					struct passwd * pw = ::getpwuid(::getuid());
					// Если информация о пользователе получена
					if(pw != nullptr)
						// Устанавливаем домашний каталог пользователя
						home = pw->pw_dir;
				}
				// Добавляем домашний каталог пользователя в путь
				path.append(home != nullptr ? home : AWH_FS_SEPARATOR);
				// Если найден слэш после тильды
				if(pos != string::npos)
					// Добавляем оставшуюся часть пути
					path.append(input.substr(pos));
			// Если путь относительный
			} else {
				// Временный буфер для текущего рабочего каталога
				char cwd[PATH_MAX];
				// Получаем текущий рабочий каталог
				if(!::getcwd(cwd, sizeof(cwd))){
					// Устанавливаем корневой каталог как текущий
					cwd[0] = '/';
					// Завершаем строку
					cwd[1] = '\0';
				}
				// Добавляем текущий рабочий каталог в путь
				path.append(cwd);
				// Добавляем слэш разделителя файловой системы
				path.append(AWH_FS_SEPARATOR);
				// Добавляем оставшуюся часть пути
				path.append(input);
			}
			// Компонент пути
			string part = "";
			// Составные части пути
			vector <string> parts;
			// 2. Теперь у нас есть строка, начинающаяся с '/', — нормализуем её
			for(char letter : path){
				// Если встретили слэш разделителя файловой системы
				if(letter == AWH_FS_SEPARATOR[0]){
					// Если компонент не пустой
					if(!part.empty()){
						// Обрабатываем компонент пути
						if(part == ".."){
							// Если есть из чего удалять, удаляем последний компонент
							if(!parts.empty())
								// Удаляем последний компонент пути
								parts.pop_back();
						// Если компонент не текущий каталог
						} else if(part != ".")
							// Добавляем компонент в составные части пути
							parts.push_back(part);
						// Очищаем компонент пути
						part.clear();
					}
				// Иначе накапливаем символ в компонент пути
				} else part.append(1, letter);
			}
			// Последний компонент
			if(!part.empty()){
				// Обрабатываем компонент пути
				if(part == ".."){
					// Если есть из чего удалять, удаляем последний компонент
					if(!parts.empty())
						// Удаляем последний компонент пути
						parts.pop_back();
				// Если компонент не текущий каталог
				} else if(part != ".")
					// Добавляем компонент в составные части пути
					parts.push_back(part);
			}
			// 3. Собираем результат — строго один слэш в начале
			string result = AWH_FS_SEPARATOR;
			// Проходим по всем частям пути
			for(size_t i = 0; i < parts.size(); ++i){
				// Если это не первая часть пути
				if(i > 0)
					// Добавляем слэш разделителя файловой системы
					result.append(AWH_FS_SEPARATOR);
				// Добавляем часть пути
				result.append(parts[i]);
			}
			// 4. Возвращаем результат
			return result;
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(input), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
		// Выводим результат
		return AWH_FS_SEPARATOR;
	}
};

/**
 * @brief Метод определяющая тип файловой системы по адресу
 *
 * @param addr адрес дирректории
 * @return     тип файловой системы
 */
awh::FileSystem::type_t awh::FileSystem::type(string_view addr) const noexcept {
	// Результат работы функции
	type_t result = type_t::NONE;
	// Если адрес дирректории передан
	if(!std::empty(addr)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * Для операционной системы MS Windows
			 */
			#if _WIN32 || _WIN64
				// Структура проверка статистики
				struct _stat info{};
				// Выполняем извлечение актуального значения адреса
				const wstring & address = this->_fmk->convert(this->fullpath(addr));
				// Выполняем извлечение данных статистики
				const int32_t status = (!address.empty() ? ::_wstat(address.c_str(), &info) : -1);
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#else
				// Структура проверка статистики
				struct stat info{};
				// Выполняем извлечение данных статистики
				const int32_t status = ::stat(addr.data(), &info);
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
					HANDLE file = ::CreateFileW(address.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
					// Если открыть файл открыт нормально
					if(file != INVALID_HANDLE_VALUE){
						// Если файл является сокетом
						if(::GetFileType(file) == FILE_TYPE_PIPE)
							// Получаем тип файловой системы
							result = type_t::SOCK;
						// Выполняем закрытие файла
						::CloseHandle(file);
					}
				#endif
				/**
				 * Если операционной системой является MacOS X
				 */
				#if __APPLE__ || __MACH__
					/**
					 * Если целевая платформа является MacOS X
					 */
					#ifdef __AWH_USE_MACOS_ALIAS_RESOLUTION__
						// Если детектировать актуальные файлы не нужно и его тип определённо установлен
						if(result != type_t::NONE){
							/**
							 * Выполняем проверку является ли файл alias-файлом
							 */
							/*
							@autoreleasepool {
								// Преобразуем путь в NSString
								NSString * path = [NSString stringWithUTF8String:addr.data()];
								// Если путь не существует
								if(!path || ![[NSFileManager defaultManager] fileExistsAtPath:path])
									// Выводим результат по умолчанию
									return result;
								// Создаём объект URL из пути
								NSURL * url = [NSURL fileURLWithPath:path];
								// Если объект URL не создан
								if(!url)
									// Выводим результат по умолчанию
									return result;
								// Объект для хранения информации о ресурсе
								NSNumber * isAliasNumber = nil;
								// Ошибки получения ресурса
								NSError * error = nil;
								// Проверяем, является ли файл alias-файлом
								BOOL success = [url getResourceValue:&isAliasNumber forKey:NSURLIsAliasFileKey error:&error];
								// Если файл является alias-файлом
								if(success && isAliasNumber && [isAliasNumber boolValue])
									// Получаем тип файловой системы
									result = type_t::LINK;
							}
							*/
						}
					#endif
				#endif
			}
			// Если детектировать актуальные файлы не нужно и адрес не детектирован как ссылка
			if(result != type_t::LINK){
				/**
				 * Для операционной системы не являющейся MS Windows
				 */
				#if !_WIN32 && !_WIN64
					// Если тип определён
					if(::lstat(addr.data(), &info) == 0){
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
							hres = ppf->Load(address.c_str(), STGM_READ);
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
					::CoUninitialize();
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(addr), log_t::flag_t::CRITICAL, error.what());
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(addr), log_t::flag_t::CRITICAL, error.what());
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
 * @param first  адрес на который нужно сделать ссылку
 * @param second адрес где должна быть создана ссылка
 */
void awh::FileSystem::symlink(string_view first, string_view second) const noexcept {
	// Если адреса переданы
	if(!std::empty(first) && !std::empty(second)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#if !_WIN32 && !_WIN64
				// Выполняем создание символьной ссылки
				::symlink(this->fullpath(first, true).c_str(), this->fullpath(second, true).c_str());
			/**
			 * Для операционной системы MS Windows
			 */
			#else
				// Получаем полный адрес пути
				const string & filename = this->fullpath(first, true);
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
								string name = ::move(filename.substr(pos + 1, filename.length() - (pos + static_cast <size_t> (offset))));
								// Ищем расширение файла
								if((pos = name.find('.')) != string::npos)
									// Устанавливаем имя файла
									description = ::move(name.substr(0, pos));
								// Устанавливаем только имя файла
								else description = ::move(name);
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
								if((second.size() > 4) && this->_fmk->compare(".lnk", second.substr(second.size() - 4)))
									// Выполняем установку адреса ярлыка как он есть
									symlink = ::move(this->fullpath(second, true));
								// Выполняем установку полного пути адреса файла
								else symlink = ::move(this->_fmk->format("%s.lnk", this->fullpath(second, true).c_str()));
								// Выполняем создание ярлыка в файловой системе
								hres = ppf->Save(this->_fmk->convert(symlink).c_str(), TRUE);
							}
							// Выполняем очистку объекта провверки файла
							psl->Release();
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(first, second), log_t::flag_t::CRITICAL, error.what());
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(first, second), log_t::flag_t::CRITICAL, error.what());
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
 * @param first  адрес на который нужно сделать ссылку
 * @param second адрес где должна быть создана ссылка
 */
void awh::FileSystem::hardlink(string_view first, string_view second) const noexcept {
	// Если адреса переданы
	if(!std::empty(first) && !std::empty(second)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#if !_WIN32 && !_WIN64
				// Если адрес на который нужно создать ссылку существует
				if(this->type(first) != type_t::NONE)
					// Выполняем создание символьной ссылки
					::link(this->fullpath(first, true).c_str(), this->fullpath(second, true).c_str());
			/**
			 * Для операционной системы MS Windows
			 */
			#else
				// Если адрес на который нужно создать ссылку существует
				if(this->type(first) != type_t::NONE)
					// Выполняем создание обычный ярлык
					this->symlink(first, second);
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(first, second), log_t::flag_t::CRITICAL, error.what());
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(first, second), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод удаления адреса файловой системы
 *
 * @param addr    полный адрес для удаления
 * @param resolve флаг резолвинга символьных ссылок
 * @return        количество дочерних элементов
 */
int32_t awh::FileSystem::unlink(string_view addr, const bool resolve) const noexcept {
	// Результат работы функции
	int32_t result = -1;
	// Если адрес передан
	if(!std::empty(addr)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем извлечение актуального значения адреса
			const string & address = this->fullpath(addr, resolve);
			// Если адрес получен правильный
			if(!address.empty()){
				/**
				 * Определяем тип пути
				 */
				switch(static_cast <uint8_t> (this->type(address))){
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
									struct _stat info{};
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
									struct stat info{};
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
											// Получаем адрес в виде строки
											const string & address = this->_fmk->format("%s%s%s", addr.data(), AWH_FS_SEPARATOR, this->_fmk->convert(ptr->d_name).c_str());
										/**
										 * Для операционной системы не являющейся MS Windows
										 */
										#else
											// Пропускаем названия текущие "." и внешние "..", так как идет рекурсия
											if(!::strcmp(ptr->d_name, ".") || !::strcmp(ptr->d_name, ".."))
												// Выполняем пропуск каталога
												continue;
											// Получаем адрес каталога
											const string & address = this->_fmk->format("%s%s%s", addr.data(), AWH_FS_SEPARATOR, ptr->d_name);
										#endif
										/**
										 * Для операционной системы MS Windows
										 */
										#if _WIN32 || _WIN64
											// Конвертируем адрес в формат wstring
											const wstring & path = this->_fmk->convert(address);
											// Если статистика извлечена
											if(!::_wstat(path.c_str(), &info)){
												// Если дочерний элемент является дирректорией
												if(S_ISDIR(info.st_mode))
													// Выполняем удаление подкаталогов
													count = this->unlink(address, resolve);
												// Если дочерний элемент является файлом то удаляем его
												else count = ::_wunlink(path.c_str());
											// Если путь является символьной ссылкой
											} else if(this->type(address) == type_t::LINK)
												// Выполняем удаление символьной ссылки
												count = ::_wunlink(path.c_str());
										/**
										 * Для операционной системы не являющейся MS Windows
										 */
										#else
											// Если статистика извлечена
											if(!::stat(address.c_str(), &info)){
												// Если дочерний элемент является дирректорией
												if(S_ISDIR(info.st_mode))
													// Выполняем удаление подкаталогов
													count = this->unlink(address, resolve);
												// Если дочерний элемент является файлом то удаляем его
												else count = ::unlink(address.c_str());
											// Если путь является символьной ссылкой
											} else if(this->type(address) == type_t::LINK)
												// Выполняем удаление символьной ссылки
												count = ::unlink(address.c_str());
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
			if(resolve && (this->type(addr) == type_t::LINK)){
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					// Выполняем извлечение актуального значения адреса
					const string & address = this->fullpath(addr);
					// Если адрес получен правильный
					if(!address.empty())
						// Выполняем удаление переданного пути
						result = ::_wunlink(this->_fmk->convert(address).c_str());
				/**
				 * Для операционной системы не являющейся MS Windows
				 */
				#else
					// Выполняем удаление переданного пути
					result = ::unlink(addr.data());
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(addr), log_t::flag_t::CRITICAL, error.what());
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(addr), log_t::flag_t::CRITICAL, error.what());
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
 * @param addr    адрес который нужно определить
 * @param resolve флаг резолвинга символьных ссылок
 * @return        полный путь
 */
string awh::FileSystem::fullpath(string_view addr, const bool resolve) const noexcept {
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
			::ExpandEnvironmentStringsW(this->_fmk->convert(addr.data()).c_str(), buffer, ARRAYSIZE(buffer));
			// Устанавливаем результат
			result = ::move(this->_fmk->convert(buffer));
			// Заполняем буфер нулями
			::memset(buffer, 0, sizeof(buffer));
			// Если адрес существует
			if(::_wfullpath(buffer, this->_fmk->convert(result).c_str(), _MAX_PATH) != nullptr){
				// Получаем полный адрес пути
				result = ::move(this->_fmk->convert(buffer));
				// Если адрес пути получен
				if(resolve && !result.empty()){
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
											// Если размер извлекаемых данных получен
											if(size > 0){
												// Выполняем выделение памяти для результирующего буфера
												result.resize(static_cast <size_t> (size), 0);
												// Выполняем извлечение полного адреса ярлыка
												::WideCharToMultiByte(CP_UTF8, 0, achPath, -1, result.data(), result.size(), 0, 0);
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
					::CoUninitialize();
				}
			}
			// Выполняем перекодирование адреса
			return result;
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#else
			// Если нужно выполнять резолвинг символьных ссылок
			if(resolve && !std::empty(addr)){
				// Устанавливаем переданный путь адреса
				result = addr.data();
				// Создаём буфер данных для получения адреса
				char buffer[PATH_MAX];
				// Если адрес существует
				if(::realpath(result.c_str(), buffer) != nullptr){
					// Получаем полный адрес пути
					result = buffer;
					/**
					 * Если целевая платформа является MacOS X
					 */
					#ifdef __AWH_USE_MACOS_ALIAS_RESOLUTION__
						/**
						 * Выполняем проверку является ли файл alias-файлом
						 */
						/*
						@autoreleasepool {
							// Преобразуем путь в NSString
							NSString * nsPath = [NSString stringWithUTF8String:buffer];
							// Если путь не существует
							if(!nsPath || ![[NSFileManager defaultManager] fileExistsAtPath:nsPath])
								// Файл не существует — не трогаем
								return result;
							// Создаём объект URL из пути
							NSURL * url = [NSURL fileURLWithPath:nsPath];
							// Если объект URL не создан
							if(!url)
								// Выводим результат как он есть
								return result;
							// Переменная для хранения ошибок
							NSError * error = nil;
							// Объект для хранения информации о ресурсе
							NSNumber * isAliasNumber = nil;
							// Проверяем, является ли файл alias-файлом
							BOOL success = [url getResourceValue:&isAliasNumber forKey:NSURLIsAliasFileKey error:&error];
							// Если возникла ошибка при проверке
							if(error)
								// Ошибка — возвращаем как есть
								return result;
							// Если файл является alias-файлом
							if(success && isAliasNumber && [isAliasNumber boolValue]){
								// Получаем bookmark-данные из alias-файла
								NSData * bookmarkData = [NSURL bookmarkDataWithContentsOfURL:url error:&error];
								// Если bookmark-данные получены
								if(bookmarkData){
									// Выполняем резолвинг alias-файла
									NSURL * resolvedURL = [NSURL URLByResolvingBookmarkData:bookmarkData options:0 relativeToURL:nil bookmarkDataIsStale:nil error:&error];
									// Если резолвинг выполнен
									if(resolvedURL){
										// Флаг определения каталога
										BOOL isDir = NO;
										// Проверяем является ли разрешённый путь каталогом
										[[NSFileManager defaultManager] fileExistsAtPath:[resolvedURL path] isDirectory:&isDir];
										// Получаем результат резолвинга
										result = [[resolvedURL path] UTF8String];
										// Если это каталог — добавляем завершающий слеш
										if(isDir && !result.empty() && (result.back() != '/'))
											// Добавляем завершающий слеш
											result.append(1, '/');
										// Выводим полученный результат
										return result;
									}
								}
							}
						}
						*/
					#endif
				// Если результат не получен и является ссылкой
				} else if(this->type(result) == type_t::LINK) {
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
			// Если актуальный путь выводить не нужно, просто возвращаем полный путь
			} else return ::fullpath(addr, this->_log);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(addr, resolve), log_t::flag_t::CRITICAL, error.what());
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(addr, resolve), log_t::flag_t::CRITICAL, error.what());
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
 * @param addr адрес файла или каталога
 * @return     запрашиваемые метаданные
 */
uint32_t awh::FileSystem::chmod(string_view addr) const noexcept {
	// Результат работы функции
	uint32_t result = 0;
	// Если путь к файлу или каталогу передан
	if(!std::empty(addr) && (this->type(addr) != type_t::NONE)){
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Выполняем извлечение актуального значения адреса
			const string & address = this->fullpath(addr, true);
			// Если адрес получен правильный
			if(!address.empty())
				// Извлекаем все атрибуты файла
				return static_cast <uint32_t> (::GetFileAttributesW(this->_fmk->convert(address).c_str()));
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#else
			// Создаём объект информационных данных файла или каталога
			struct stat info{};
			// Выполняем чтение информационных данных файла
			if(!(result = (::stat(addr.data(), &info) == 0)) && (errno != 0)){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(addr), log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим в лог сообщение
					this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
				#endif
			// Если информационные данные считаны удачно
			} else result = static_cast <uint32_t> (info.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод изменения прав доступа к файлу или каталогу
 *
 * @param addr адрес файла или каталога
 * @param mode метаданные для установки
 * @return     результат работы функции
 */
bool awh::FileSystem::chmod(string_view addr, const uint32_t mode) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если путь к файлу или каталогу передан
	if(!std::empty(addr) && (this->type(addr) != type_t::NONE)){
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Выполняем извлечение актуального значения адреса
			const string & address = this->fullpath(addr, true);
			// Если адрес получен правильный
			if(!address.empty())
				// Выполняем установку атрибутов файла
				return ::SetFileAttributesW(this->_fmk->convert(address).c_str(), static_cast <DWORD> (mode));
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#else
			// Выполняем установку метаданных файла
			if(!(result = (::chmod(addr.data(), static_cast <mode_t> (mode)) == 0)) && (errno != 0)){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(addr, mode), log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим в лог сообщение
					this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
				#endif
			}
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки владельца на файл или каталог
 *
 * @param addr  адрес файла или каталога для установки владельца
 * @param user  имя пользователя
 * @param group название группы пользователя
 * @return      результат работы функции
 */
bool awh::FileSystem::chown(string_view addr, string_view user, [[maybe_unused]] string_view group) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если путь передан
	if(!std::empty(addr) && !std::empty(user) && (this->type(addr) != type_t::NONE)){
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#if !_WIN32 && !_WIN64
			// Если группа пользователя передана
			if(!std::empty(group)){
				// Идентификатор пользователя
				const uid_t uid = this->_os.uid(user.data());
				// Идентификатор группы
				const gid_t gid = this->_os.gid(group.data());
				// Устанавливаем права на каталог
				if((result = (uid && gid))){
					// Выполняем установку владельца
					if(!(result = (::chown(addr.data(), uid, gid) == 0)) && (errno != 0)){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(addr, user, group), log_t::flag_t::CRITICAL, ::strerror(errno));
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим в лог сообщение
							this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
						#endif
					}
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
			wstring fileName = ::move(this->_fmk->convert(addr.data()));
			// Получаем имя пользователя
			wstring userName = ::move(this->_fmk->convert(user.data()));
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
					this->_log->debug(L"%s", __PRETTY_FUNCTION__, std::make_tuple(addr, user), log_t::flag_t::CRITICAL, message);
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
					this->_log->debug(L"%s", __PRETTY_FUNCTION__, std::make_tuple(addr, user), log_t::flag_t::CRITICAL, message);
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
			if(!(result = (::SetNamedSecurityInfoW(reinterpret_cast <LPWSTR> (fileName.data()), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, pNewDACL, nullptr) == ERROR_SUCCESS))){
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
 * @brief Метод рекурсивного создания пути
 *
 * @param addr адрес для создания каталога
 */
void awh::FileSystem::mkdir(string_view addr) const noexcept {
	// Если путь передан
	if(!std::empty(addr)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем извлечение актуального значения адреса
			const string & address = this->fullpath(addr, true);
			// Если адрес получен правильный
			if(!address.empty()){
				// Создаём буфер данных для получения адреса
				char buffer[PATH_MAX];
				// Копируем переданный адрес в буфер
				::snprintf(buffer, address.size() + 1, "%s", address.c_str());
				// Если последний символ является сепаратором тогда удаляем его
				if(buffer[address.size() - 1] == AWH_FS_SEPARATOR[0])
					// Устанавливаем конец строки
					buffer[address.size() - 1] = 0;
				// Указатель на сепаратор
				char * i = nullptr;
				// Переходим по всем символам
				for(i = buffer + 1; * i; i++){
					// Если найден сепаратор
					if((* i) == AWH_FS_SEPARATOR[0]){
						// Сбрасываем указатель
						(* i) = 0;
						/**
						 * Для операционной системы не являющейся MS Windows
						 */
						#if !_WIN32 && !_WIN64
							// Создаем каталог
							::mkdir(buffer, S_IRWXU);
						/**
						 * Для операционной системы MS Windows
						 */
						#else
							// Создаем каталог
							::_wmkdir(this->_fmk->convert(buffer).c_str());
						#endif
						// Запоминаем сепаратор
						(* i) = AWH_FS_SEPARATOR[0];
					// Если это последний символ в строке
					} else if(* (i + 1) == 0) {
						/**
						 * Для операционной системы не являющейся MS Windows
						 */
						#if !_WIN32 && !_WIN64
							// Создаем каталог
							::mkdir(buffer, S_IRWXU);
						/**
						 * Для операционной системы MS Windows
						 */
						#else
							// Создаем каталог
							::_wmkdir(this->_fmk->convert(buffer).c_str());
						#endif
					}
				}
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
				this->_log->debug("Memory allocation error", __PRETTY_FUNCTION__, std::make_tuple(addr), log_t::flag_t::CRITICAL);
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("Memory allocation error", log_t::flag_t::CRITICAL);
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(addr), log_t::flag_t::CRITICAL, error.what());
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(addr), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод создания каталога с указанием владельца
 *
 * @param addr  адрес для создания каталога
 * @param user  имя пользователя
 * @param group название группы пользователя
 * @return      результат создания каталога
 */
bool awh::FileSystem::mkdir(string_view addr, string_view user, string_view group) const noexcept {
	// Результат работы функции
	bool result = false;
	// Проверяем существует ли нужный нам каталог
	if((result = (this->type(addr) == type_t::NONE))){
		// Создаем каталог
		this->mkdir(addr);
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#if !_WIN32 && !_WIN64
			// Устанавливаем права на каталог
			this->chown(addr, user, group);
		#endif
	}
	// Сообщаем что каталог и так существует
	return result;
}
/**
 * @brief Метод извлечения названия и расширения файла
 *
 * @param addr    адрес файла для извлечения его параметров
 * @param resolve флаг резолвинга символьных ссылок
 * @param before  флаг определения первой точки расширения слева
 */
awh::FileSystem::components_t awh::FileSystem::components(string_view addr, const bool resolve, const bool before) const noexcept {
	// Результат работы функции
	components_t result;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Получаем полный адрес пути
		const string & filename = this->fullpath(addr, resolve);
		// Если файл передан
		if(!filename.empty()){
			// Позиция разделителя каталога
			size_t pos = 0;
			// Определяем флаг обратного смещения
			const uint8_t offset = (filename.back() == AWH_FS_SEPARATOR[0] ? 2 : 1);
			// Выполняем поиск разделителя каталога
			if((pos = filename.rfind(AWH_FS_SEPARATOR, filename.length() - static_cast <size_t> (offset))) != string::npos){
				// Если переданный адрес является каталогом
				if(this->type(filename) == type_t::DIR)
					// Выполняем вывод названия каталога
					result.first = ::move(filename.substr(pos + 1, filename.length() - (pos + static_cast <size_t> (offset))));
				// Если переданный адрес не является каталогом
				else {
					// Извлекаем имя файла
					string name = ::move(filename.substr(pos + 1));
					// Ищем расширение файла
					if((pos = (before ? name.find('.') : name.rfind('.'))) != string::npos){
						// Устанавливаем имя файла
						result.first = ::move(name.substr(0, pos));
						// Устанавливаем расширение файла
						result.second = ::move(name.substr(pos + 1));
					// Устанавливаем только имя файла
					} else result.first = ::move(name);
				}
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(addr, resolve, before), log_t::flag_t::CRITICAL, error.what());
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(addr, resolve, before), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод подсчёта размера файла/каталога
 *
 * @param addr    адрес для подсчёта размера
 * @param ext     расширение файла если требуется фильтрация
 * @param recurse флаг рекурсивного перебора каталогов
 * @return        общий размер файла/каталога
 */
uintmax_t awh::FileSystem::size(string_view addr, string_view ext, const bool recurse) const noexcept {
	// Результат работы функции
	uintmax_t result = 0;
	// Если путь для подсчёта передан
	if(!std::empty(addr)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем извлечение актуального значения адреса
			const string & address = this->fullpath(addr, true);
			// Если адрес получен правильный
			if(!address.empty()){
				// Получаем обёртку полученного пути
				string_view path = address;
				/**
				 * Определяем тип переданного пути
				 */
				switch(static_cast <uint8_t> (this->type(path))){
					// Если полный путь является файлом
					case static_cast <uint8_t> (type_t::FILE): {
						/**
						 * Для операционной системы MS Windows
						 */
						#if _WIN32 || _WIN64
							// Создаём объект работы с файлом
							HANDLE file = ::CreateFileW(this->_fmk->convert(path.data()).c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
							// Если открыть файл открыт нормально
							if(file != INVALID_HANDLE_VALUE){
								// Получаем размер файла
								result = static_cast <uintmax_t> (::GetFileSize(file, nullptr));
								// Выполняем закрытие файла
								::CloseHandle(file);
							}
						/**
						 * Для операционной системы не являющейся MS Windows
						 */
						#else
							// Структура проверка статистики
							struct stat info{};
							// Выполняем извлечение данных статистики
							return static_cast <uintmax_t> (::stat(path.data(), &info));
						#endif
					} break;
					// Если полный путь является каталогом
					case static_cast <uint8_t> (type_t::DIR): {
						/**
						 * Для операционной системы MS Windows
						 */
						#if _WIN32 || _WIN64
							// Открываем указанный каталог
							_WDIR * dir = ::_wopendir(this->_fmk->convert(path.data()).c_str());
						/**
						 * Для операционной системы не являющейся MS Windows
						 */
						#else
							// Открываем указанный каталог
							DIR * dir = ::opendir(path.data());
						#endif
							// Если каталог открыт
							if(dir != nullptr){
								/**
								 * Для операционной системы MS Windows
								 */
								#if _WIN32 || _WIN64
									// Структура проверка статистики
									struct _stat info{};
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
									struct stat info{};
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
											// Получаем полный путь в виде строки
											const string & address = this->_fmk->format("%s%s%s", path.data(), AWH_FS_SEPARATOR, this->_fmk->convert(ptr->d_name).c_str());
										/**
										 * Для операционной системы не являющейся MS Windows
										 */
										#else
											// Пропускаем названия текущие "." и внешние "..", так как идет рекурсия
											if(!::strcmp(ptr->d_name, ".") || !::strcmp(ptr->d_name, ".."))
												// Выполняем пропуск каталога
												continue;
											// Получаем полный путь в виде строки
											const string & address = this->_fmk->format("%s%s%s", path.data(), AWH_FS_SEPARATOR, ptr->d_name);
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
												// Получаем обёртку полученного пути
												string_view path = address;
												// Если дочерний элемент является дирректорией
												if(S_ISDIR(info.st_mode))
													// Выполняем подсчёт размера каталога
													result += (recurse ? this->size(path, ext, recurse) : 0);
												// Если дочерний элемент является файлом
												else if(!std::empty(ext)) {
													// Получаем расширение файла
													const string & extension = this->_fmk->format(".%s", ext.data());
													// Если расширение не выше полного адреса
													if(path.size() > extension.size()){
														// Если расширение файла найдено
														if(this->_fmk->compare(path.substr(path.size() - extension.size()).data(), extension))
															// Получаем размер файла
															result += this->size(path, ext, recurse);
													}
												// Получаем размер файла
												} else result += this->size(path, ext, recurse);
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(addr, ext, recurse), log_t::flag_t::CRITICAL, error.what());
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(addr, ext, recurse), log_t::flag_t::CRITICAL, error.what());
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
 * @param addr    адрес для подсчёта количества файлов
 * @param ext     расширение файла если требуется фильтрация
 * @param recurse флаг рекурсивного перебора каталогов
 * @return        количество файлов в каталоге
 */
uintmax_t awh::FileSystem::count(string_view addr, string_view ext, const bool recurse) const noexcept {
	// Результат работы функции
	uintmax_t result = 0;
	// Если адрес каталога и расширение файлов переданы
	if(!std::empty(addr) && (this->type(addr) == type_t::DIR)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем извлечение актуального значения адреса
			const string & address = this->fullpath(addr, true);
			// Если адрес получен правильный
			if(!address.empty()){
				// Получаем обёртку полученного пути
				string_view path = address;
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					// Открываем указанный каталог
					_WDIR * dir = ::_wopendir(this->_fmk->convert(path.data()).c_str());
				/**
				 * Для операционной системы не являющейся MS Windows
				 */
				#else
					// Открываем указанный каталог
					DIR * dir = ::opendir(path.data());
				#endif
					// Если каталог открыт
					if(dir != nullptr){
						/**
						 * Для операционной системы MS Windows
						 */
						#if _WIN32 || _WIN64
							// Структура проверка статистики
							struct _stat info{};
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
							struct stat info{};
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
									const string & address = this->_fmk->format("%s%s%s", path.data(), AWH_FS_SEPARATOR, this->_fmk->convert(ptr->d_name).c_str());
								/**
								 * Для операционной системы не являющейся MS Windows
								 */
								#else
									// Пропускаем названия текущие "." и внешние "..", так как идет рекурсия
									if(!::strcmp(ptr->d_name, ".") || !::strcmp(ptr->d_name, ".."))
										// Выполняем пропуск каталога
										continue;
									// Получаем адрес в виде строки
									const string & address = this->_fmk->format("%s%s%s", path.data(), AWH_FS_SEPARATOR, ptr->d_name);
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
										// Получаем обёртку полученного пути
										string_view path = address;
										// Если дочерний элемент является дирректорией
										if(S_ISDIR(info.st_mode))
											// Выполняем подсчитываем количество файлов в каталоге
											result += (recurse ? this->count(path, ext, recurse) : 0);
										// Если дочерний элемент является файлом
										else if(!std::empty(ext)) {
											// Получаем расширение файла
											const string & extension = this->_fmk->format(".%s", ext.data());
											// Если расширение не выше полного адреса
											if(path.size() > extension.size()){
												// Если расширение файла найдено
												if(this->_fmk->compare(path.substr(path.size() - extension.size()).data(), extension))
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(addr, ext, recurse), log_t::flag_t::CRITICAL, error.what());
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(addr, ext, recurse), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	// Если переданный адрес не является каталогом
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Address name: \"%s\" is not dir", __PRETTY_FUNCTION__, std::make_tuple(addr, ext, recurse), log_t::flag_t::CRITICAL, addr.data());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Address name: \"%s\" is not dir", log_t::flag_t::WARNING, addr.data());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Шаблон метода добавления в файл бинарных данных
 *
 * @tparam T тип буфера данных
 */
template <typename T>
/**
 * @brief Метод добавления в файл бинарных данных
 *
 * @param filename адрес файла в который необходимо выполнить запись
 * @param buffer   бинарный буфер который необходимо записать в файл
 */
void awh::FileSystem::append(string_view filename, const T & buffer) const noexcept {
	// Если буфер данных передан
	if(!std::empty(filename))
		// Выполняем добавление в файл бинарных данных
		this->append(filename, buffer.data(), buffer.size());
}
/**
 * @brief Явный специализированный шаблон метода добавления в текстовый файл
 *
 */
template void awh::FileSystem::append(string_view, const string &) const noexcept;
/**
 * @brief Явный специализированный шаблон метода добавления в текстовый файл из буфера символов
 *
 */
template void awh::FileSystem::append(string_view, const vector <char> &) const noexcept;
/**
 * @brief Явный специализированный шаблон метода добавления в файл бинарных данных
 *
 */
template void awh::FileSystem::append(string_view, const vector <uint8_t> &) const noexcept;
/**
 * @brief Метод добавления в файл бинарных данных
 *
 * @param filename адрес файла в который необходимо выполнить запись
 * @param buffer   бинарный буфер который необходимо записать в файл
 * @param size     размер бинарного буфера для записи в файл
 */
void awh::FileSystem::append(string_view filename, const void * buffer, const size_t size) const noexcept {
	// Если параметры для записи переданы
	if(!filename.empty() && (buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем извлечение актуального значения адреса
			const string & address = this->fullpath(filename, true);
			// Если адрес получен правильный
			if(!address.empty()){
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					// Выполняем открытие файла на добавление
					HANDLE file = ::CreateFileW(this->_fmk->convert(address).c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
					// Если открыть файл открыт нормально
					if(file != INVALID_HANDLE_VALUE){
						// Выполняем добавление данных в файл
						::WriteFile(file, static_cast <LPCVOID> (buffer), static_cast <DWORD> (size), 0, nullptr);
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
						file.write(reinterpret_cast <const char *> (buffer), size);
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
 * @brief Шаблон метода чтения данных из файла
 *
 * @tparam T тип возвращаемого результата
 */
template <typename T>
/**
 * @brief Метод чтения данных из файла
 *
 * @param filename адрес файла для чтения
 * @param seek     тип смещения в файле
 * @param offset   смещение в файле
 * @return         бинарный буфер с прочитанными данными
 */
auto awh::FileSystem::read(string_view filename, const seek_t seek, const size_t offset) const noexcept -> T {
	// Результат работы функции
	T result;
	// Если буфер данных передан
	if(!std::empty(filename))
		// Выполняем чтение данных из файла
		this->read(filename, seek, result, offset);
	// Выводим результат
	return result;
}
/**
 * @brief Явный специализированный шаблон метода чтения данных из файла в строку
 *
 */
template string awh::FileSystem::read(string_view, const seek_t, const size_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода чтения данных из файла в буфер символов
 *
 */
template vector <char> awh::FileSystem::read(string_view, const seek_t, const size_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода чтения данных из файла в буфер бинарных данных
 *
 */
template vector <uint8_t> awh::FileSystem::read(string_view, const seek_t, const size_t) const noexcept;
/**
 * @brief Шаблон метода чтения данных из файла
 *
 * @tparam T тип возвращаемого результата
 */
template <typename T>
/**
 * @brief Метод чтения данных из файла
 *
 * @param filename адрес файла для чтения
 * @param seek     тип смещения в файле
 * @param result   контейнер куда следует положить результат
 * @param offset   смещение в файле
 */
void awh::FileSystem::read(string_view filename, const seek_t seek, T & result, const size_t offset) const noexcept {
	// Если адрес файла передан и он существует
	if(!std::empty(filename) && (this->type(filename) == type_t::FILE)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем извлечение актуального значения адреса
			const string & address = this->fullpath(filename, true);
			// Если адрес получен правильный
			if(!address.empty()){
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					// Создаём объект работы с файлом
					HANDLE file = ::CreateFileW(this->_fmk->convert(address).c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
					// Если открыть файл открыт нормально
					if(file != INVALID_HANDLE_VALUE){
						// Создаём объект большого числа
						LARGE_INTEGER li;
						// Устанавливаем начальное значение позиции
						li.QuadPart = static_cast <LONGLONG> (offset);
						/**
						 * Определяем тип смещения в файле события
						 */
						switch(static_cast <uint8_t> (seek)){
							// Если смещение от начала файла
							case static_cast <uint8_t> (seek_t::BEGIN):
								// Выполняем установку позиции в файле
								li.LowPart = ::SetFilePointer(file, li.LowPart, &li.HighPart, FILE_BEGIN);
							break;
							// Если смещение от текущей позиции в файле
							case static_cast <uint8_t> (seek_t::CURRENT):
								// Выполняем установку позиции в файле
								li.LowPart = ::SetFilePointer(file, li.LowPart, &li.HighPart, FILE_CURRENT);
							break;
							// Если смещение от конца файла
							case static_cast <uint8_t> (seek_t::END):
								// Выполняем установку позиции в файле
								li.LowPart = ::SetFilePointer(file, li.LowPart, &li.HighPart, FILE_END);
							break;
							// Если тип смещения не определён
							default: li.LowPart = 0;
						}
						// Если мы получили ошибку установки позиции
						if((li.LowPart == INVALID_SET_FILE_POINTER) && (::GetLastError() != NO_ERROR))
							// Сбрасываем значение установленной позиции
							li.QuadPart = -1;
						// Если позиция установлена успешно
						if(li.QuadPart > -1){
							// Определяем размер читаемых данных
							size_t size = (static_cast <size_t> (::GetFileSize(file, nullptr)) - offset);
							// Если объект результата пустой
							if(result.empty())
								// Устанавливаем размер буфера
								result.resize(size);
							// Если объект результата уже задан — читаем min(размер буфера, size)
							else size = ::min(size, result.size());
							// Выполняем чтение из файла в буфер данные
							if(!::ReadFile(file, static_cast <LPVOID> (&result[0]), static_cast <DWORD> (size), 0, nullptr)){
								// Создаём буфер сообщения ошибки
								wchar_t message[256] = {0};
								// Выполняем формирование текста ошибки
								::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 256, 0);
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug(L"%s", __PRETTY_FUNCTION__, std::make_tuple(filename, static_cast <uint16_t> (seek), result.size(), offset), log_t::flag_t::CRITICAL, message);
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print(L"%s", log_t::flag_t::CRITICAL, message);
								#endif
							}
							// Выполняем закрытие файла
							::CloseHandle(file);
						}
					}
				/**
				 * Для операционной системы не являющейся MS Windows
				 */
				#else
					// Файловый дескриптор файла
					int32_t fd = -1;
					// Структура статистики файла
					struct stat info{};
					// Если файл не открыт
					if((fd = ::open(address.c_str(), O_RDONLY)) < 0){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(filename, static_cast <uint16_t> (seek), result.size(), offset), log_t::flag_t::CRITICAL, ::strerror(errno));
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
						#endif
					// Если файл открыт удачно
					} else if(::fstat(fd, &info) < 0) {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(filename, static_cast <uint16_t> (seek), result.size(), offset), log_t::flag_t::CRITICAL, ::strerror(errno));
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
						#endif
					// Если размер файла изменился
					} else if(static_cast <size_t> (info.st_size) > offset) {
						// Позиция в файле
						off_t position = 0;
						/**
						 * Определяем тип смещения в файле события
						 */
						switch(static_cast <uint8_t> (seek)){
							// Если смещение от начала файла
							case static_cast <uint8_t> (seek_t::BEGIN):
								// Выполняем расчёт смещения в файле
								position = static_cast <off_t> (offset);
							break;
							// Если смещение от конца файла
							case static_cast <uint8_t> (seek_t::END):
								// Выполняем расчёт смещения в файле
								position = (static_cast <off_t> (info.st_size) - static_cast <off_t> (offset));
							break;
						}
						// Проверяем границы
						if(position < 0)
							// Устанавливаем позицию в начало файла
							position = 0;
						// Если позиция выше размера файла
						if(position >= static_cast <off_t> (info.st_size)){
							// Закрываем файловый дескриптор
							::close(fd);
							// Выходим из метода
							return;
						}
						// Определяем размер читаемых данных
						size_t size = (static_cast <size_t> (info.st_size) - static_cast <size_t> (position));
						// Если объект результата пустой
						if(result.empty())
							// Устанавливаем размер буфера
							result.resize(size);
						// Если объект результата уже задан — читаем min(размер буфера, size)
						else size = ::min(size, result.size());
						// Читаем данные из файла в буфер
						if(::pread(fd, &result[0], size, position) != static_cast <ssize_t> (size)){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(filename, static_cast <uint16_t> (seek), result.size(), offset), log_t::flag_t::CRITICAL, ::strerror(errno));
							/**
							* Если режим отладки не включён
							*/
							#else
								// Выводим сообщение что прочитать файл не удалось
								this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
							#endif
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
		} catch(const ios_base::failure & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(filename, static_cast <uint16_t> (seek), result.size(), offset), log_t::flag_t::CRITICAL, error.what());
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(filename, static_cast <uint16_t> (seek), result.size(), offset), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Явный специализированный шаблон метода чтения данных из файла в строку
 *
 */
template void awh::FileSystem::read(string_view, const seek_t, string &, const size_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода чтения данных из файла в буфер символов
 *
 */
template void awh::FileSystem::read(string_view, const seek_t, vector <char> &, const size_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода чтения данных из файла в буфер бинарных данных
 *
 */
template void awh::FileSystem::read(string_view, const seek_t, vector <uint8_t> &, const size_t) const noexcept;
/**
 * @brief Шаблон метода записи в файл бинарных данных
 *
 * @tparam T тип буфера данных
 */
template <typename T>
/**
 * @brief Метод записи в файл бинарных данных
 *
 * @param filename адрес файла в который необходимо выполнить запись
 * @param buffer   бинарный буфер который необходимо записать в файл
 * @param seek     тип смещения в файле
 * @param offset   смещение в файле
 */
void awh::FileSystem::write(string_view filename, const T & buffer, const seek_t seek, const size_t offset) const noexcept {
	// Выполняем запись в файл бинарных данных
	this->write(filename, buffer.data(), buffer.size(), seek, offset);
}
/**
 * @brief Явный специализированный шаблон метода записи в файл бинарных данных из строки
 *
 */
template void awh::FileSystem::write(string_view, const string &, const seek_t, const size_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода записи в файл бинарных данных из буфера символов
 *
 */
template void awh::FileSystem::write(string_view, const vector <char> &, const seek_t, const size_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода записи в файл бинарных данных из буфера бинарных данных
 *
 */
template void awh::FileSystem::write(string_view, const vector <uint8_t> &, const seek_t, const size_t) const noexcept;
/**
 * @brief Метод записи в файл бинарных данных
 *
 * @param filename адрес файла в который необходимо выполнить запись
 * @param buffer   бинарный буфер который необходимо записать в файл
 * @param size     размер бинарного буфера для записи в файл
 * @param seek     тип смещения в файле
 * @param offset   смещение в файле
 */
void awh::FileSystem::write(string_view filename, const void * buffer, const size_t size, const seek_t seek, const size_t offset) const noexcept {
	// Если параметры для записи переданы
	if(!std::empty(filename) && (buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем извлечение актуального значения адреса
			const string & address = this->fullpath(filename, true);
			// Если адрес получен правильный
			if(!address.empty()){
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					// Выполняем открытие файла на запись
					HANDLE file = ::CreateFileW(this->_fmk->convert(address).c_str(), GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
					// Если открыть файл открыт нормально
					if(file != INVALID_HANDLE_VALUE){
						// Создаём объект большого числа
						LARGE_INTEGER li;
						// Устанавливаем начальное значение позиции
						li.QuadPart = static_cast <LONGLONG> (offset);
						/**
						 * Определяем тип смещения в файле события
						 */
						switch(static_cast <uint8_t> (seek)){
							// Если смещение от начала файла
							case static_cast <uint8_t> (seek_t::BEGIN):
								// Выполняем установку позиции в файле
								li.LowPart = ::SetFilePointer(file, li.LowPart, &li.HighPart, FILE_BEGIN);
							break;
							// Если смещение от текущей позиции в файле
							case static_cast <uint8_t> (seek_t::CURRENT):
								// Выполняем установку позиции в файле
								li.LowPart = ::SetFilePointer(file, li.LowPart, &li.HighPart, FILE_CURRENT);
							break;
							// Если смещение от конца файла
							case static_cast <uint8_t> (seek_t::END):
								// Выполняем установку позиции в файле
								li.LowPart = ::SetFilePointer(file, li.LowPart, &li.HighPart, FILE_END);
							break;
							// Если тип смещения не определён
							default: li.LowPart = 0;
						}
						// Если мы получили ошибку установки позиции
						if((li.LowPart == INVALID_SET_FILE_POINTER) && (::GetLastError() != NO_ERROR))
							// Сбрасываем значение установленной позиции
							li.QuadPart = -1;
						// Если позиция установлена успешно
						if(li.QuadPart > -1)
							// Выполняем запись данных в файл
							::WriteFile(file, static_cast <LPCVOID> (buffer), static_cast <DWORD> (size), 0, nullptr);
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
						/**
						 * Определяем тип смещения в файле события
						 */
						switch(static_cast <uint8_t> (seek)){
							// Если смещение от начала файла
							case static_cast <uint8_t> (seek_t::BEGIN):
								// Устанавливаем позицию записи
								file.seekp(offset, file.beg);
							break;
							// Если смещение от текущей позиции в файле
							case static_cast <uint8_t> (seek_t::CURRENT):
								// Устанавливаем позицию записи
								file.seekp(offset, file.cur);
							break;
							// Если смещение от конца файла
							case static_cast <uint8_t> (seek_t::END):
								// Устанавливаем позицию записи
								file.seekp(offset, file.end);
							break;
						}
						// Выполняем запись данных в файл
						file.write(reinterpret_cast <const char *> (buffer), size);
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(filename, buffer, size, static_cast <uint16_t> (seek), offset), log_t::flag_t::CRITICAL, error.what());
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(filename, buffer, size, static_cast <uint16_t> (seek), offset), log_t::flag_t::CRITICAL, error.what());
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
 * @param seek     тип смещения в файле
 * @param callback функция обратного вызова
 * @param offset   смещение в файле
 */
void awh::FileSystem::readfile(string_view filename, const seek_t seek, const function <void (const string &)> & callback, const size_t offset) const noexcept {

}
/**
 * @brief Метод рекурсивного получения файлов во всех подкаталогах
 *
 * @param path     путь до каталога
 * @param ext      расширение файла по которому идет фильтрация
 * @param recurse  флаг рекурсивного перебора каталогов
 * @param callback функция обратного вызова
 * @param resolve  флаг резолвинга символьных ссылок
 */
void awh::FileSystem::readdir(string_view path, string_view ext, const bool recurse, const function <void (const string &)> & callback, const bool resolve) const noexcept {

}
/**
 * @brief Метод рекурсивного чтения файлов во всех подкаталогах
 *
 * @param path     путь до каталога
 * @param ext      расширение файла по которому идет фильтрация
 * @param recurse  флаг рекурсивного перебора каталогов
 * @param callback функция обратного вызова
 * @param resolve  флаг резолвинга символьных ссылок
 */
void awh::FileSystem::readdir(string_view path, string_view ext, const bool recurse, const function <void (const string &, const string &)> & callback, const bool resolve) const noexcept {
	// Если адрес каталога и расширение файлов переданы
	if(!std::empty(path) && (this->type(path) == type_t::DIR)){
		// Выполняем извлечение актуального значения адреса
		const string & address = this->fullpath(path, resolve);
		// Если адрес получен правильный
		if(!address.empty())
			// Переходим по всему списку файлов в каталоге
			this->readdir(address, ext, recurse, [&](const string & filename) noexcept -> void {
				// Выполняем считывание всех строк текста
				this->readfile(filename, seek_t::BEGIN, [&](const string & text) noexcept -> void {
					// Если текст получен
					if(!text.empty())
						// Выводим функцию обратного вызова
						callback(text, filename);
				});
			}, resolve);
	// Если переданный адрес не является каталогом
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Address: \"%s\" is not found", __PRETTY_FUNCTION__, std::make_tuple(path, ext, recurse, resolve), log_t::flag_t::CRITICAL, path.data());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Address: \"%s\" is not found", log_t::flag_t::WARNING, path.data());
		#endif
	}
}
