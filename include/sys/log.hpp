/**
 * @file: log.hpp
 * @date: 2021-12-19
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2021
 */

#ifndef __AWH_LOG__
#define __AWH_LOG__

/**
 * Стандартная библиотека
 */
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <cstring>
#include <cstdarg>
#include <functional>
#include <sys/types.h>
#include <zlib.h>

// Если - это Windows
#if defined(_WIN32) || defined(_WIN64)
	#include <time.h>
// Если - это Unix
#else
	#include <ctime>
#endif

/**
 * Наши модули
 */
#include <sys/fmk.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Log Класс работы с логами
	 */
	typedef class Log {
		public:
			/**
			 * flag_t Флаги логирования
			 */
			enum class flag_t : uint8_t {NONE, INFO, WARNING, CRITICAL};
		private:
			// Флаг разрешения вывода логов в файл
			bool fileMode = true;
			// Флаг разрешения вывода логов в консоль
			bool consoleMode = true;
		private:
			// Максимальный размер файла лога
			size_t maxFileSize = MAX_SIZE_LOGFILE;
		private:
			// Адрес файла для сохранения логов
			string logFile = "";
			// Формат даты и времени для вывода лога
			string logFormat = DATE_FORMAT;
			// Название сервиса для вывода лога
			string logName = AWH_SHORT_NAME;
		private:
			// Функция обратного вызова которая срабатывает, при появлении лога
			function <void (const flag_t, const string &)> subscribeFn = nullptr;
		private:
			// Создаём объект фреймворка
			const fmk_t * fmk = nullptr;
		private:
			/**
			 * rotate Метод выполнения ротации логов
			 */
			void rotate() const noexcept;
		public:
			/**
			 * print Метод вывода текстовой информации в консоль или файл
			 * @param format формат строки вывода
			 * @param flag   флаг типа логирования
			 */
			void print(const string & format, flag_t flag, ...) const noexcept;
			/**
			 * print Метод вывода текстовой информации в консоль или файл
			 * @param format формат строки вывода
			 * @param flag   флаг типа логирования
			 * @param items  список аргументов для замены
			 */
			void print(const string & format, flag_t flag, const vector <string> & items) const noexcept;
		public:
			/**
			 * allowFile Метод установки разрешения на вывод лога в файл
			 * @param mode флаг разрешения на вывод лога в файл
			 */
			void allowFile(const bool mode) noexcept;
			/**
			 * allowConsole Метод установки разрешения на вывод лога в консоль
			 * @param mode флаг разрешения на вывод лога в консоль
			 */
			void allowConsole(const bool mode) noexcept;
		public:
			/**
			 * setLogName Метод установки название сервиса для вывода лога
			 * @param name название сервиса для вывода лога
			 */
			void setLogName(const string & name) noexcept;
			/**
			 * setLogMaxFileSize Метод установки максимального размера файла логов
			 * @param size максимальный размер файла логов
			 */
			void setLogMaxFileSize(const float size) noexcept;
			/**
			 * setLogFormat Метод установки формата даты и времени для вывода лога
			 * @param format формат даты и времени для вывода лога
			 */
			void setLogFormat(const string & format) noexcept;
			/**
			 * setLogFilename Метод установки файла для сохранения логов
			 * @param filename адрес файла для сохранения логов
			 */
			void setLogFilename(const string & filename) noexcept;
		public:
			/**
			 * subscribe Метод подписки на события логов
			 * @param callback функция обратного вызова
			 */
			void subscribe(function <void (const flag_t, const string &)> callback) noexcept;
		public:
			/**
			 * Log Конструктор
			 * @param fmk      объект фреймворка
			 * @param filename адрес файла для сохранения логов
			 */
			Log(const fmk_t * fmk, const string & filename = "") noexcept : fmk(fmk), logFile(filename) {}
			/**
			 * ~Log Деструктор
			 */
			~Log() noexcept {}
	} log_t;
};

#endif // __AWH_LOG__
