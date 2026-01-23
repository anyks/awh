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
				struct _stat info;
				// Выполняем извлечение актуального значения адреса
				const wstring & address = this->_fmk->convert(this->fullpath(addr));
				// Выполняем извлечение данных статистики
				const int32_t status = (!address.empty() ? ::_wstat(address.c_str(), &info) : -1);
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#else
				// Структура проверка статистики
				struct stat info;
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
								if((second.size() > 4) && this->_fmk->compare(".lnk", second.substr(second.size() - 4)))
									// Выполняем установку адреса ярлыка как он есть
									symlink = this->fullpath(second, true);
								// Выполняем установку полного пути адреса файла
								else symlink = this->_fmk->format("%s.lnk", this->fullpath(second, true).c_str());
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

}
/**
 * @brief Метод получения прав доступа к файлу или каталогу
 *
 * @param addr адрес файла или каталога
 * @return     запрашиваемые метаданные
 */
uint16_t awh::FileSystem::chmod(string_view addr) const noexcept {
}
/**
 * @brief Метод изменения прав доступа к файлу или каталогу
 *
 * @param addr адрес файла или каталога
 * @param mode метаданные для установки
 * @return     результат работы функции
 */
bool awh::FileSystem::chmod(string_view addr, const uint16_t mode) const noexcept {

}
/**
 * @brief Метод установки владельца на файл или каталог
 *
 * @param addr  адрес файла или каталога для установки владельца
 * @param user  имя пользователя
 * @param group название группы пользователя
 * @return      результат работы функции
 */
bool awh::FileSystem::chown(string_view addr, string_view user, string_view group) const noexcept {

}
/**
 * @brief Метод рекурсивного создания пути
 *
 * @param addr адрес для создания каталога
 */
void awh::FileSystem::mkdir(string_view addr) const noexcept {

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

}
/**
 * @brief Метод извлечения названия и расширения файла
 *
 * @param addr    адрес файла для извлечения его параметров
 * @param resolve флаг резолвинга символьных ссылок
 * @param before  флаг определения первой точки расширения слева
 */
awh::FileSystem::components_t awh::FileSystem::components(string_view addr, const bool resolve, const bool before) const noexcept {

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

}
/**
 * @brief Метод установки позиции в файле
 *
 * @param file     объект открытого файла
 * @param distance дистанцию на которую нужно переместить позицию
 * @param position текущая позиция в файле
 * @return         перенос позиции в файле
 */
ssize_t awh::FileSystem::seek(string_view filename, const seek_t seek) const noexcept {

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

}
/**
 * @brief Метод добавления в файл бинарных данных
 *
 * @param filename адрес файла в который необходимо выполнить запись
 * @param buffer   бинарный буфер который необходимо записать в файл
 * @param size     размер бинарного буфера для записи в файл
 */
void awh::FileSystem::append(string_view filename, const void * buffer, const size_t size) const noexcept {

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
 * @param result   контейнер куда следует положить результат
 * @param offset   смещение в файле
 */
void awh::FileSystem::read(string_view filename, const seek_t seek, T & result, const size_t offset) const noexcept {

}
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

}
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

}
