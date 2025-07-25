/**
 * @file: os.hpp
 * @date: 2024-04-02
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

#ifndef __AWH_OPERATING_SYSTEM__
#define __AWH_OPERATING_SYSTEM__

/**
 * Стандартные модули
 */
#include <memory>
#include <vector>
#include <string>
#include <cstring>
#include <unistd.h>
#include <stdint.h>

// Если используется BOOST
#ifdef USE_BOOST_CONVERT
	#include <boost/locale/encoding_utf.hpp>
// Если нужно использовать стандартную библиотеку
#else
	#include <codecvt>
#endif

/**
 * Для операционной системы OS Windows
 */
#if _WIN32 || _WIN64
	/**
	 * Подключаем основные заголовочные файлы
	 */
	#include <windows.h>
	#include <process.h>
	#include <processthreadsapi.h>
	/**
	 * Заменяем переменную AWH ERROR
	 */
	#define AWH_ERROR() (WSAGetLastError())
	/**
	 * Заменяем типы данных
	 */
	#define uid_t uint16_t       // unsigned short
	#define gid_t uint16_t       // unsigned short
	#define u_char unsigned char // unsigned char
	/**
	 * Заменяем вызов функции
	 */
	#define getpid _getpid
	#define getppid GetCurrentProcessId
	/**
	 * Файловый разделитель Windows
	 */
	#define FS_SEPARATOR "\\"
	/**
	 * Устанавливаем кодировку UTF-8
	 */
	#pragma execution_character_set("utf-8")
/**
 * Для операционной системы не являющейся OS Windows
 */
#else
	/**
	 * Подключаем основные заголовочные файлы
	 */
	#include <cerrno>
	#include <sys/types.h>
	/**
	 * Заменяем переменную AWH ERROR
	 */
	#define AWH_ERROR() (errno)
	/**
	 * Файловый разделитель UNIX-подобных систем
	 */
	#define FS_SEPARATOR "/"
#endif

/**
 * Разрешаем сборку под Windows
 */
#include "global.hpp"

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * OS Класс работы с операционной системой
	 */
	typedef class AWHSHARED_EXPORT OS {
		public:
			/**
			 * type_t Названия поддерживаемых операционных систем
			 */
			enum class type_t : uint8_t {
				NONE    = 0x00, // Операционная система не определена
				UNIX    = 0x01, // Операционная система Unix
				LINUX   = 0x02, // Операционная система Linux
				WIND32  = 0x03, // Операционная система Windows 32bit
				WIND64  = 0x04, // Операционная система Windows 64bit
				MACOSX  = 0x05, // Операционная система MacOS X
				SOLARIS = 0x06, // Операционная система Sun Solaris
				FREEBSD = 0x07, // Операционная система FreeBSD
				NETBSD  = 0x08, // Операционная система NetBSD
				OPENBSD = 0x09  // Операционная система OpenBSD
			};
		private:
			/**
			 * metadata Функция получения строкового типа метаданных
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
			 * @tparam Шаблон метода чтения метаданных из бинарного контейнера
			 */
			template <typename T>
			/**
			 * metadata Функция получения бинарного буфера метаданных
			 * @param buffer буфер бинарных данных
			 * @param result результат работф функции
			 */
			static void metadata(const vector <char> & buffer, vector <T> & result) noexcept {
				// Если буфер данных передан
				if(!buffer.empty()){
					// Выделяем память для результирующего буфера данных
					result.resize(buffer.size() / sizeof(T));
					// Выполняем копирование полученного буфера данных
					::memcpy(result.data(), buffer.data(), buffer.size());
				}
			}
			/**
			 * @tparam Шаблон метода чтения метаданных из бинарного контейнера
			 */
			template <typename T>
			/**
			 * metadata Функция получения основных типов метаданных
			 * @param buffer буфер бинарных данных
			 * @param result результат работф функции
			 */
			static void metadata(const vector <char> & buffer, T & result) noexcept {
				// Если данные являются основными
				if(!buffer.empty())
					// Выполняем копирование полученных данных
					::memcpy(&result, buffer.data(), sizeof(result));
			}
		public:
			/**
			 * boost Метод применение сетевой оптимизации операционной системы
			 * @return результат работы
			 */
			void boost() const noexcept;
		public:
			/**
			 * type Метод определения операционной системы
			 * @return название операционной системы
			 */
			type_t type() const noexcept;
		public:
			/**
			 * enableCoreDumps Метод активации создания дампа ядра
			 * @return результат установки лимитов дампов ядра
			 */
			bool enableCoreDumps() const noexcept;
		public:
			/**
			 * uid Метод вывода идентификатора пользователя
			 * @param name имя пользователя
			 * @return     полученный идентификатор пользователя
			 */
			uid_t uid(const string & name) const noexcept;
			/**
			 * gid Метод вывода идентификатора группы пользователя
			 * @param name название группы пользователя
			 * @return     полученный идентификатор группы пользователя
			 */
			gid_t gid(const string & name) const noexcept;
		private:
			/**
			 * congestionControl Метод определения алгоритма работы сети
			 * @param str строка с выводмом доступных алгоритмов из sysctl
			 * @return    выбранная строка с названием алгоритма
			 */
			string congestionControl(const string & str) const noexcept;
		public:
			/**
			 * limitFDs Метод установки количество разрешенных файловых дескрипторов
			 * @param cur текущий лимит на количество открытых файловых дискриптеров
			 * @param max максимальный лимит на количество открытых файловых дискриптеров
			 * @return    результат выполнения операции
			 */
			bool limitFDs(const uint32_t cur, const uint32_t max) const noexcept;
		public:
			/**
			 * chown Метод запуска приложения от имени указанного пользователя
			 * @param uid идентификатор пользователя
			 * @param gid идентификатор группы пользователя
			 * @return    результат выполнения операции
			 */
			bool chown(const uid_t uid, const gid_t gid = 0) const noexcept;
			/**
			 * chown Метод запуска приложения от имени указанного пользователя
			 * @param user  название пользователя
			 * @param group название группы пользователя
			 * @return      результат выполнения операции
			 */
			bool chown(const string & user, const string & group = "") const noexcept;
		private:
			/**
			 * sysctl Метод извлечения настроек ядра операционной системы
			 * @param name   название записи для получения настроек
			 * @param buffer бинарный буфер с извлечёнными значениями
			 */
			void sysctl(const string & name, vector <char> & buffer) const noexcept;
		public:
			/**
			 * @tparam Шаблон метода извлечения настроек ядра операционной системы
			 */
			template <typename T>
			/**
			 * sysctl Метод извлечения настроек ядра операционной системы
			 * @param name название записи для получения настроек
			 * @return     полученное значение записи
			 */
			T sysctl(const string & name) const noexcept {
				// Результат работы функции
				T result;
				// Если данные являются основными
				if(is_integral <T>::value || is_floating_point <T>::value || is_array <T>::value){
					// Буфер результата по умолчанию
					uint8_t buffer[sizeof(T)];
					// Заполняем нулями буфер данных
					::memset(buffer, 0, sizeof(T));
					// Выполняем установку результата по умолчанию
					::memcpy(&result, reinterpret_cast <T *> (buffer), sizeof(T));
				}
				// Если название записи передано правильно
				if(!name.empty()){
					// Создаём буфер данных для извлечения данных
					vector <char> buffer;
					// Выполняем извлечение данных записи
					this->sysctl(name, buffer);
					// Если данные буфера были извлечены удачно
					if(!buffer.empty()){
						// Если данные являются основными
						if(is_class <T>::value || is_integral <T>::value || is_floating_point <T>::value)
							// Выполняем получение данных
							this->metadata(buffer, result);
					}
				}
				// Выводим результат
				return result;
			}
		private:
			/**
			 * sysctl Метод установки настроек ядра операционной системы
			 * @param name   название записи для установки настроек
			 * @param buffer буфер бинарных данных записи для установки настроек
			 * @param size   размер буфера данных
			 * @return       результат выполнения установки
			 */
			bool sysctl(const string & name, const void * buffer, const size_t size) const noexcept;
		public:
			/**
			 * @tparam Шаблон метода установки настроек ядра операционной системы
			 */
			template <typename T>
			/**
			 * sysctl Метод установки настроек ядра операционной системы
			 * @param name  название записи для установки настроек
			 * @param value значение записи для установки настроек
			 * @return      результат выполнения установки
			 */
			bool sysctl(const string & name, const T value) const noexcept {
				// Если название записи для установки настроек передано
				if(!name.empty() && (is_integral <T>::value || is_floating_point <T>::value)){
					/**
					 * Если это Linux
					 */
					#if __linux__
						// Выполняем преобразование числа в строку
						const string param = std::to_string(value);
						// Выполняем установку буфера бинарных данных
						return this->sysctl(name, param.c_str(), param.size());
					/**
					 * Если это другая операционная система
					 */
					#else
						// Буфер результата по умолчанию
						vector <uint8_t> buffer(sizeof(value), 0);
						// Выполняем установку результата по умолчанию
						::memcpy(buffer.data(), &value, sizeof(value));
						// Выполняем установку буфера бинарных данных
						return this->sysctl(name, buffer.data(), buffer.size());
					#endif
				}
				// Сообщаем, что ничего не установленно
				return false;
			}
			/**
			 * @tparam Шаблон метода установки настроек ядра операционной системы
			 */
			template <typename T>
			/**
			 * sysctl Метод установки настроек ядра операционной системы
			 * @param name  название записи для установки настроек
			 * @param items значение записи для установки настроек
			 * @return      результат выполнения установки
			 */
			bool sysctl(const string & name, const vector <T> & items) const noexcept {
				// Если название записи для установки настроек передано
				if(!name.empty() && (is_integral <T>::value || is_floating_point <T>::value)){
					/**
					 * Если это Linux
					 */
					#if __linux__
						// Выполняем преобразование числа в строку
						string param = "";
						// Выполняем перебор всего списка параметров
						for(auto & item : items){
							// Если строка уже сформированна
							if(!param.empty())
								// Выполняем добавление пробела
								param.append(1, ' ');
							// Добавляем полученное значение в список
							param.append(std::to_string(item));
						}
						// Выполняем установку буфера бинарных данных
						return this->sysctl(name, param.c_str(), param.size());
					/**
					 * Если это другая операционная система
					 */
					#else
						// Смещение в бинарном буфере
						size_t offset = 0;
						// Буфер результата по умолчанию
						vector <uint8_t> buffer(items.size() * sizeof(T), 0);
						// Выполняем перебор всего списка параметров
						for(auto & item : items){
							// Выполняем установку результата по умолчанию
							::memcpy(buffer.data() + offset, &item, sizeof(item));
							// Выполняем увеличение смещения в буфере
							offset += sizeof(item);
						}
						// Выполняем установку буфера бинарных данных
						return this->sysctl(name, buffer.data(), buffer.size());
					#endif
				}
				// Сообщаем, что ничего не установленно
				return false;
			}
			/**
			 * sysctl Метод установки настроек ядра операционной системы
			 * @param name  название записи для установки настроек
			 * @param value значение записи для установки настроек
			 * @return      результат выполнения установки
			 */
			bool sysctl(const string & name, const string & value) const noexcept {
				// Если название записи для установки настроек передано
				if(!name.empty())
					// Выполняем установку буфера бинарных данных
					return this->sysctl(name, value.c_str(), value.size());
				// Сообщаем, что ничего не установленно
				return false;
			}
		public:
			/**
			 * exec Метод запуска внешнего приложения
			 * @param cmd       команда запуска
			 * @param multiline данные должны вернутся многострочные
			 */
			string exec(const string & cmd, const bool multiline = true) const noexcept;
	} os_t;
};

#endif // __AWH_OPERATING_SYSTEM__
