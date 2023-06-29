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
#include <ctime>
#include <queue>
#include <thread>
#include <atomic>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <cstring>
#include <cstdarg>
#include <functional>
#include <condition_variable>
#include <zlib.h>

/**
 * Если операционной системой не является Windows
 */
#if !defined(_WIN32) && !defined(_WIN64)
	#include <unistd.h>
#endif

/**
 * Наши модули
 */
#include <sys/fmk.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;
using namespace std::placeholders;

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
			enum class flag_t : uint8_t {
				NONE     = 0x00, // Флаг не установлен
				INFO     = 0x01, // Информационное сообщение
				WARNING  = 0x02, // Предупреждающее сообщение
				CRITICAL = 0x03  // Критическое сообщение
			};
			/**
			 * level_t Уровни логирования
			 */
			enum class level_t : uint8_t {
				NONE             = 0x00, // Логирование отключено
				ALL              = 0x07, // Разрешено выводить все виды логов
				INFO             = 0x01, // Разрешено выводить только информационные логи
				WARNING          = 0x02, // Разрешено выводить только логи предупреждения
				CRITICAL         = 0x03, // Разрешено выводить только критические логи
				INFO_WARNING     = 0x04, // Разрешено выводить логи информационные и предупреждения
				INFO_CRITICAL    = 0x05, // Разрешено выводить логи информационные и критические
				WARNING_CRITICAL = 0x06  // Разрешено выводить логи предупреждения и критические
			};
		private:
			/**
			 * Payload Структура полезной нагрузки
			 */
			typedef struct Payload {
				flag_t flag; // Флаг полезной нагрузки
				string data; // Данные полезной нагрузки
				/**
				 * Payload Конструктор
				 */
				Payload() noexcept : flag(flag_t::NONE), data("") {}
			} payload_t;
		private:
			// Идентификатор процесса
			pid_t _pid;
		private:
			// Флаг остановки работы дочернего потока
			mutable bool _stop;
			// Флаг разрешения вывода логов в файл
			bool _fileMode;
			// Флаг разрешения вывода логов в консоль
			bool _consoleMode;
		private:
			// Максимальный размер файла лога
			size_t _maxSize;
		private:
			// Уровень логирования
			level_t _level;
		private:
			// Мютекс для блокировки потока
			mutable mutex _mtx1, _mtx2;
			// Условная переменная, ожидания поступления данных
			mutable condition_variable _cv;
			// Очередь полезной нагрузки
			mutable queue <payload_t> _payload;
		private:
			// Пул рабочих дочерних потоков
			mutable map <pid_t, std::thread> _thr;
		private:
			// Название сервиса для вывода лога
			string _name;
			// Формат даты и времени для вывода лога
			string _format;
			// Адрес файла для сохранения логов
			string _filename;
		private:
			// Функция обратного вызова которая срабатывает, при появлении лога
			function <void (const flag_t, const string &)> _fn;
		private:
			// Создаём объект фреймворка
			const fmk_t * _fmk;
		private:
			/**
			 * receiving Метод получения данных
			 */
			void receiving() const noexcept;
		private:
			/**
			 * rotate Метод выполнения ротации логов
			 */
			void rotate() const noexcept;
		private:
			/**
			 * checkInputData Метод проверки на существование данных
			 * @return результат проверки
			 */
			bool checkInputData() const noexcept;
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
			 * name Метод установки название сервиса для вывода лога
			 * @param name название сервиса для вывода лога
			 */
			void name(const string & name) noexcept;
			/**
			 * maxSize Метод установки максимального размера файла логов
			 * @param size максимальный размер файла логов
			 */
			void maxSize(const float size) noexcept;
			/**
			 * level Метод установки уровня логирования
			 * @param level уровень логирования для установки
			 */
			void level(const level_t level) noexcept;
			/**
			 * format Метод установки формата даты и времени для вывода лога
			 * @param format формат даты и времени для вывода лога
			 */
			void format(const string & format) noexcept;
			/**
			 * filename Метод установки файла для сохранения логов
			 * @param filename адрес файла для сохранения логов
			 */
			void filename(const string & filename) noexcept;
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
			Log(const fmk_t * fmk, const string & filename = "") noexcept;
			/**
			 * ~Log Деструктор
			 */
			~Log() noexcept;
	} log_t;
};

#endif // __AWH_LOG__
