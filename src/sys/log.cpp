/**
 * @file log.cpp
 * @date 2026-09-13
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * @brief Реализация модуля логирования — форматирование сообщений по уровням важности,
 *        асинхронная доставка в приёмники вывода (консоль, файл, SysLog, функция обратного вызова),
 *        ротация файлов и удаление устаревших архивов
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
	/**
	 * Подключаем единую точку подключения системных заголовков MS Windows
	 *
	 * @note Подключается она прежде заголовков проекта и прочих заголовков MS Windows:
	 *       те самостоятельными не являются, а заголовок sys/os.hpp заводит макросом
	 *       имя u_char, какое системный _bsd_types.h объявляет типом через typedef
	 *
	 */
	#include <sys/macro/win32.hpp>
#endif

/**
 * Стандартные заголовочные файлы
 */
#include <fstream>
#include <cstring>
#include <cstdarg>
#include <iostream>
#include <algorithm>

/**
 * Системные заголовочные файлы
 */
#include <zlib.h>
#include <fcntl.h>
#include <sys/stat.h>

/**
 * Для операционной системы не являющейся MS Windows
 *
 * @note Заголовки unistd.h и sys/file.h принадлежат POSIX и у MS Windows отсутствуют.
 *       Работа с файлами ведётся там средствами самой системы через sys/macro/win32.hpp
 *
 */
#if !_WIN32 && !_WIN64
	/**
	 * Системные заголовочные файлы
	 */
	#include <unistd.h>
	#include <sys/file.h>
#endif

/**
 * Для операционной системы не являющейся MS Windows
 */
#if !_WIN32 && !_WIN64
	/**
	 * Системные заголовочные файлы для работы с syslog и обходом каталогов
	 */
	#include <sys/dirent.hpp>
	#include <syslog.h>
#endif

/**
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
	/**
	 * Если перенос строки лога не установлен
	 */
	#ifndef AWH_STRING_BREAK
		// Формируем перенос строк лога
		#define AWH_STRING_BREAK "\r\n"
	#endif
	/**
	 * Если переносы строки лога не установлены
	 */
	#ifndef AWH_STRING_BREAKS
		// Формируем переносы строк лога
		#define AWH_STRING_BREAKS AWH_STRING_BREAK"" AWH_STRING_BREAK
	#endif
/**
 * Для операционной системы не являющейся MS Windows
 */
#else
	/**
	 * Если перенос строки лога не установлен
	 */
	#ifndef AWH_STRING_BREAK
		// Формируем перенос строк лога
		#define AWH_STRING_BREAK "\n"
	#endif
	/**
	 * Если переносы строки лога не установлены
	 */
	#ifndef AWH_STRING_BREAKS
		// Формируем переносы строк лога
		#define AWH_STRING_BREAKS AWH_STRING_BREAK"" AWH_STRING_BREAK
	#endif
#endif


/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/os.hpp>
#include <sys/fmk.hpp>
#include <sys/locker.hpp>
#include <sys/chrono.hpp>
#include <sys/screen.hpp>
#include <sys/macro/lib.hpp>
#include <sys/log.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Используем пространство имён placeholders
 */
using namespace placeholders;

namespace awh {
/**
 * @brief Пространство имён работы с логами
 *
 */
namespace log {
/**
 * @brief Пространство имён внутреннего устройства модуля логирования
 *
 * @details Полезная нагрузка, приёмники вывода и состояние модуля наружу не выставляются:
 *          заголовочный файл несёт только договор, а всё устройство заведено здесь.
 *
 */
namespace {
	/**
	 * @brief Класс полезной нагрузки
	 *
	 * @details Полезная нагрузка формируется в момент вызова логирования и содержит
	 *          текст сообщения, дату формирования и флаг типа сообщения.
	 *
	 */
	typedef class Payload {
		public:
			// Флаг полезной нагрузки
			flag_t flag;
			// Текст полезной нагрузки
			string text;
			// Дата формирования сообщения (фиксируется в момент вызова)
			string date;
		public:
			/**
			 * @brief Оператор перемещающего присваивания параметров полезной нагрузки
			 *
			 * @param payload объект полезной нагрузки для перемещения
			 * @return        текущий объект полезной нагрузки
			 *
			 */
			Payload & operator = (Payload && payload) noexcept;
			/**
			 * @brief Оператор присваивания присваивания параметров полезной нагрузки
			 *
			 * @param payload объект полезной нагрузки для копирования
			 * @return        текущий объект полезной нагрузки
			 *
			 */
			Payload & operator = (const Payload & payload) noexcept;
		public:
			/**
			 * @brief Оператор сравнения
			 *
			 * @param payload объект полезной нагрузки для сравнения
			 * @return        результат сравнения
			 *
			 */
			bool operator == (const Payload & payload) noexcept;
		public:
			/**
			 * @brief Конструктор перемещения
			 *
			 * @param payload объект полезной нагрузки для перемещения
			 *
			 */
			explicit Payload(Payload && payload) noexcept;
			/**
			 * @brief Конструктор копирования
			 *
			 * @param payload объект полезной нагрузки для копирования
			 *
			 */
			explicit Payload(const Payload & payload) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Payload() noexcept;
		public:
			/**
			 * @brief Деструктор
			 *
			 */
			~Payload() noexcept = default;
	} payload_t;
	/**
	 * @brief Базовый абстрактный приёмник вывода логов
	 *
	 * @details Приёмник вывода логов может быть реализован в виде консольного вывода,
	 *          записи в файл, отправки в SysLog или передачи в функцию обратного вызова.
	 *
	 * @note Обратного указателя на владеющий объект приёмник больше не несёт:
	 *       состояние модуля единственно на процесс и берётся напрямую.
	 *
	 */
	class Sink {
		public:
			/**
			 * @brief Метод записи полезной нагрузки в приёмник
			 *
			 * @param payload объект полезной нагрузки
			 *
			 */
			virtual void write(const payload_t & payload) const noexcept = 0;
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			Sink() noexcept = default;
			/**
			 * @brief Деструктор
			 *
			 */
			virtual ~Sink() noexcept = default;
	};
	/**
	 * @brief Приёмник вывода логов в консоль
	 *
	 */
	class ConsoleSink : public Sink {
		public:
			/**
			 * @brief Метод записи полезной нагрузки в консоль
			 *
			 * @param payload объект полезной нагрузки
			 *
			 */
			void write(const payload_t & payload) const noexcept override;
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			ConsoleSink() noexcept = default;
			/**
			 * @brief Деструктор
			 *
			 */
			virtual ~ConsoleSink() noexcept = default;
	};
	/**
	 * @brief Приёмник вывода логов в файл
	 *
	 */
	class FileSink : public Sink {
		private:
			// Идентификатор процесса, владеющего дескриптором
			mutable pid_t _pid;
		private:
			// Постоянный дескриптор записи (на POSIX - файловый дескриптор, на Windows - HANDLE)
			mutable intptr_t _fd;
		private:
			// Путь к файлу, который сейчас открыт
			mutable string _opened;
			// Текущий размер открытого файла лога
			mutable uintmax_t _size;
		private:
			/**
			 * @brief Метод (пере)открытия постоянного дескриптора записи
			 *
			 */
			void reopen() const noexcept;
			/**
			 * @brief Метод выполнения ротации файла лога
			 *
			 */
			void rotate() const noexcept;
			/**
			 * @brief Метод удаления устаревших архивов логов (retention)
			 *
			 */
			void retention() const noexcept;
		private:
			/**
			 * @brief Метод формирования уникального имени архива логов
			 *
			 * @return путь к файлу архива, гарантированно не конфликтующий с существующими
			 *
			 */
			string nextArchive() const noexcept;
		public:
			/**
			 * @brief Метод записи полезной нагрузки в файл
			 *
			 * @param payload объект полезной нагрузки
			 *
			 */
			void write(const payload_t & payload) const noexcept override;
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			FileSink() noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~FileSink() noexcept;
	};
	/**
	 * @brief Приёмник отправки логов в SysLog
	 *
	 */
	class SyslogSink : public Sink {
		public:
			/**
			 * @brief Метод отправки полезной нагрузки в SysLog
			 *
			 * @param payload объект полезной нагрузки
			 *
			 */
			void write(const payload_t & payload) const noexcept override;
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			SyslogSink() noexcept = default;
			/**
			 * @brief Деструктор
			 *
			 */
			virtual ~SyslogSink() noexcept = default;
	};
	/**
	 * @brief Приёмник передачи логов в функцию обратного вызова
	 *
	 */
	class CallbackSink : public Sink {
		public:
			/**
			 * @brief Метод передачи полезной нагрузки в функцию обратного вызова
			 *
			 * @param payload объект полезной нагрузки
			 *
			 */
			void write(const payload_t & payload) const noexcept override;
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			CallbackSink() noexcept = default;
			/**
			 * @brief Деструктор
			 *
			 */
			virtual ~CallbackSink() noexcept = default;
	};
}
/**
 * @brief Пространство имён состояния модуля логирования
 *
 */
namespace {
	/**
	 * @brief Класс состояния модуля логирования
	 *
	 * @details Состояние заведено единственным на процесс: настройка журнала в пределах
	 *          приложения одна, отчего передавать её объектом больше не требуется.
	 *
	 */
	class State {
		public:
			// Флаг асинхронного режима работы
			bool _async;
		public:
			// Уровень логирования
			level_t _level;
		public:
			// Флаг формирования разделителя
			separator_t _sep;
		public:
			// Название сервиса для вывода лога
			string _name;
			// Формат даты и времени для вывода лога
			string _format;
			// Адрес файла для сохранения логов
			string _filename;
		public:
			// Максимальный размер файла лога
			size_t _maxSize;
			// Размер сообщения для формирования разделителя
			size_t _sepSize;
			// Максимальный размер очереди асинхронного вывода (0 - без ограничения)
			size_t _maxQueue;
			// Максимальное количество хранимых архивов логов (0 - без ограничения)
			size_t _maxFiles;
		public:
			// Объект работы с датой и временем
			awh::chrono_t _chrono;
		public:
			// Политика поведения при переполнении очереди асинхронного вывода
			overflow_t _overflow;
		public:
			// Список доступных флагов
			unordered_set <mode_t> _mode;
		public:
			// Идентификатор процесса, владеющего асинхронным потоком
			mutable atomic <pid_t> _pid;
		public:
			// Счётчик для сброса накопленных логов
			mutable atomic_uint8_t _counter;
		public:
			// Объект работы с дочерними потоками
			mutable awh::screen_t <payload_t> _screen;
		public:
			// Мютекс для блокировки потока
			mutable awh::lock_state_t <std::mutex> _mtx;
		public:
			// Набор приёмников вывода логов, построенный по текущему списку режимов
			mutable vector <unique_ptr <Sink>> _sinks;
		public:
			// Функция обратного вызова которая срабатывает при появлении лога
			function <void (const flag_t, string_view)> _callback;
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			State() noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~State() noexcept;
	};
	/**
	 * @brief Функция получения состояния модуля логирования
	 *
	 * @details Состояние заводится при первом обращении: так снимается зависимость
	 *          от порядка построения статических объектов приложения.
	 *
	 * @return состояние модуля логирования
	 *
	 */
	State & state() noexcept {
		// Выполняем создание состояния модуля логирования
		static State instance;
		// Возвращаем созданное состояние
		return instance;
	}
}
}
}

namespace awh {
/**
 * @brief Пространство имён работы с логами
 *
 */
namespace log {
/**
 * @brief Пространство имён предварительных объявлений служебных средств
 *
 * @details Объявления нужны приёмникам вывода, которые зовут построение строки лога
 *          до того, как она описана ниже по файлу.
 *
 */
namespace {
	/**
	 * @brief Функция построения набора приёмников вывода логов
	 *
	 * @details Состояние принимается доводом затем, что построение выполняется в том числе
	 *          из конструктора состояния, когда обращаться к нему через state() ещё нельзя:
	 *          объект на тот миг только строится.
	 *
	 * @param self состояние модуля логирования
	 *
	 */
	void rebuild(State & self) noexcept;
	/**
	 * @brief Функция проверки разрешения на вывод лога
	 *
	 * @param flag флаг типа логирования
	 * @return     результат проверки
	 *
	 */
	bool allowed(const flag_t flag) noexcept;
	/**
	 * @brief Функция очистки текста от символов форматирования
	 *
	 * @param text текст для очистки
	 * @return     очищенный текст
	 *
	 */
	string & cleaner(string & text) noexcept;
	/**
	 * @brief Функция передачи полезной нагрузки в приёмники вывода
	 *
	 * @param payload объект полезной нагрузки
	 *
	 */
	void dispatch(payload_t && payload) noexcept;
	/**
	 * @brief Функция получения полезной нагрузки из очереди асинхронного вывода
	 *
	 * @param payload объект полезной нагрузки
	 *
	 */
	void receiving(const payload_t & payload) noexcept;
	/**
	 * @brief Функция разбора адреса файла лога на составляющие
	 *
	 * @param filename адрес файла лога
	 * @return         каталог и название файла
	 *
	 */
	pair <string, string> components(string_view filename) noexcept;
	/**
	 * @brief Функция построения текста лога
	 *
	 * @param payload объект полезной нагрузки
	 * @param colored флаг цветового форматирования
	 * @return        построенный текст лога
	 *
	 */
	string compose(const payload_t & payload, const bool colored) noexcept;
}
}
}

namespace awh {
/**
 * @brief Пространство имён работы с логами
 *
 */
namespace log {
namespace {
/**
 * @brief Устройство полезной нагрузки, приёмников и служебных средств
 *
 */
/**
 * @brief Оператор перемещающего присваивания параметров полезной нагрузки
 *
 * @param payload объект полезной нагрузки для перемещения
 * @return        текущий объект полезной нагрузки
 *
 */
Payload & Payload::operator = (payload_t && payload) noexcept {
	// Выполняем установку флага
	this->flag = payload.flag;
	// Выполняем перемещение текста
	this->text = ::move(payload.text);
	// Выполняем перемещение даты
	this->date = ::move(payload.date);
	// Возвращаем текущий объект
	return (* this);
}
/**
 * @brief Оператор присваивания присваивания параметров полезной нагрузки
 *
 * @param payload объект полезной нагрузки для копирования
 * @return        текущий объект полезной нагрузки
 *
 */
Payload & Payload::operator = (const payload_t & payload) noexcept {
	// Выполняем установку флага
	this->flag = payload.flag;
	// Выполняем копирование текста
	this->text = payload.text;
	// Выполняем копирование даты
	this->date = payload.date;
	// Возвращаем текущий объект
	return (* this);
}
/**
 * @brief Оператор сравнения
 *
 * @param payload объект полезной нагрузки для сравнения
 * @return        результат сравнения
 *
 */
bool Payload::operator == (const payload_t & payload) noexcept {
	// Выполняем проверку полезной нагрузки
	return (
		(this->flag == payload.flag) &&
		(this->text.compare(payload.text) == 0)
	);
}
/**
 * @brief Конструктор перемещения
 *
 * @param payload объект полезной нагрузки для перемещения
 *
 */
Payload::Payload(payload_t && payload) noexcept {
	// Выполняем установку флага
	this->flag = payload.flag;
	// Выполняем перемещение текста
	this->text = ::move(payload.text);
	// Выполняем перемещение даты
	this->date = ::move(payload.date);
}
/**
 * @brief Конструктор копирования
 *
 * @param payload объект полезной нагрузки для копирования
 *
 */
Payload::Payload(const payload_t & payload) noexcept {
	// Выполняем установку флага
	this->flag = payload.flag;
	// Выполняем копирование текста
	this->text = payload.text;
	// Выполняем копирование даты
	this->date = payload.date;
}
/**
 * @brief Конструктор
 *
 */
Payload::Payload() noexcept : flag(flag_t::NONE), text{""}, date{""} {}


/**
 * @brief Метод записи полезной нагрузки в консоль
 *
 * @param payload объект полезной нагрузки
 *
 */
void ConsoleSink::write(const payload_t & payload) const noexcept {
	// Если тип сообщения не является пустым
	if(payload.flag != flag_t::NONE){
		/**
		 * Определяем флаг формирования разделителя
		 */
		switch(static_cast <uint8_t> (state()._sep)){
			// Если разделитель нужно отобразить с учётом размера текста
			case static_cast <uint8_t> (separator_t::SMART): {
				// Если размер текста соответствует размеру лога
				if(payload.text.length() >= state()._sepSize)
					// Возвращаем обозначение начала вывода лога
					cout << "*************** START ***************" << endl << endl;
			} break;
			// Если разделитель нужно отобразить всегда
			case static_cast <uint8_t> (separator_t::ALWAYS):
				// Возвращаем обозначение начала вывода лога
				cout << "*************** START ***************" << endl << endl;
			break;
		}
	}
	// Выводим сформированное сообщение лога с символами цветового форматирования
	cout << compose(payload, true);
	// Если тип сообщения не является пустым
	if(payload.flag != flag_t::NONE){
		/**
		 * Определяем флаг формирования разделителя
		 */
		switch(static_cast <uint8_t> (state()._sep)){
			// Если разделитель нужно отобразить с учётом размера текста
			case static_cast <uint8_t> (separator_t::SMART): {
				// Если размер текста соответствует размеру лога
				if(payload.text.length() >= state()._sepSize)
					// Возвращаем обозначение конца вывода лога
					cout << "---------------- END ----------------" << endl << endl;
			} break;
			// Если разделитель нужно отобразить всегда
			case static_cast <uint8_t> (separator_t::ALWAYS):
				// Возвращаем обозначение конца вывода лога
				cout << "---------------- END ----------------" << endl << endl;
			break;
		}
	}
	// Увеличиваем счётчик для принудительного сброса накопленных логов
	state()._counter.fetch_add(1, std::memory_order_relaxed);
	// Если мы прошли полный круг счётчика
	if(state()._counter.load(std::memory_order_acquire) == 0)
		// Выполняем сброс накопленных логов
		cout << flush;
}

/**
 * @brief Метод (пере)открытия постоянного дескриптора записи
 *
 */
void FileSink::reopen() const noexcept {
	// Если дескриптор ранее был открыт, закрываем его
	if(this->_fd != -1){
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Закрываем дескриптор файла
			::CloseHandle(reinterpret_cast <HANDLE> (this->_fd));
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#else
			// Закрываем файловый дескриптор
			::close(static_cast <int32_t> (this->_fd));
		#endif
		// Сбрасываем дескриптор
		this->_fd = -1;
	}
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		// Открываем файл лога на дозапись (FILE_APPEND_DATA обеспечивает атомарную дозапись)
		HANDLE handle = ::CreateFileW(awh::fmk::convert(state()._filename).c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		// Если файл открыт нормально
		if(handle != INVALID_HANDLE_VALUE){
			// Запоминаем дескриптор файла
			this->_fd = reinterpret_cast <intptr_t> (handle);
			// Структура для получения размера файла
			LARGE_INTEGER size;
			// Получаем текущий размер файла лога единоразово при открытии
			this->_size = (::GetFileSizeEx(handle, &size) ? static_cast <uintmax_t> (size.QuadPart) : 0);
		// Если открыть файл не удалось
		} else {
			// Сбрасываем дескриптор
			this->_fd = -1;
			// Обнуляем накопленный размер файла
			this->_size = 0;
		}
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#else
		// Открываем файл лога на дозапись (O_APPEND гарантирует атомарную дозапись между процессами)
		this->_fd = ::open(state()._filename.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
		// Структура для получения статистики файла
		struct stat info;
		// Получаем текущий размер файла лога единоразово при открытии
		this->_size = (((this->_fd != -1) && (::fstat(static_cast <int32_t> (this->_fd), &info) == 0)) ? static_cast <uintmax_t> (info.st_size) : 0);
	#endif
	// Запоминаем идентификатор текущего процесса
	this->_pid = ::getpid();
	// Запоминаем путь открытого файла
	this->_opened = state()._filename;
}
/**
 * @brief Метод выполнения ротации файла лога
 *
 */
void FileSink::rotate() const noexcept {
	// Формируем уникальное имя архива (без коллизий в пределах одной секунды)
	const string archive = this->nextArchive();
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		// Получаем путь к исходному файлу лога
		const wstring & filename = awh::fmk::convert(state()._filename);
		// Открываем исходный файл лога на чтение
		HANDLE file = ::CreateFileW(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		// Если файл открыт нормально
		if(file != INVALID_HANDLE_VALUE){
			// Флаг успешности сжатия
			bool success = false;
			// Открываем файл архива на сжатие
			gzFile gz = ::gzopen_w(awh::fmk::convert(archive).c_str(), "wb9h");
			// Если файл архива открыт удачно
			if(gz != nullptr){
				// Буфер потокового чтения данных (64 Кб)
				vector <char> buffer(0x10000);
				// Количество прочитанных байт
				DWORD bytes = 0;
				/**
				 * Выполняем потоковое чтение и сжатие файла порциями
				 */
				while(::ReadFile(file, static_cast <LPVOID> (buffer.data()), static_cast <DWORD> (buffer.size()), &bytes, nullptr) && (bytes > 0))
					// Выполняем сжатие порции данных
					::gzwrite(gz, buffer.data(), bytes);
				// Закрываем сжатый файл
				::gzclose(gz);
				// Устанавливаем флаг успешности сжатия
				success = true;
			// Если произошла ошибка сжатия
			} else {
				// Создаём буфер сообщения ошибки
				wchar_t message[0xFF] = {0};
				// Выполняем формирование текста ошибки
				::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 0xFF, 0);
				// Возвращаем текст полученной ошибки
				::fprintf(stderr, "ERROR! Logging rotate: %s\n\n", awh::fmk::convert(message).c_str());
			}
			// Выполняем закрытие исходного файла
			::CloseHandle(file);
			// Удаляем исходный файл логов только после успешного сжатия (во избежание потери данных)
			if(success)
				// Удаляем исходный файл логов
				::_wunlink(filename.c_str());
		}
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#else
		// Открываем файл архива на сжатие
		gzFile gz = ::gzopen(archive.c_str(), "wb9h");
		// Если файл архива открыт удачно
		if(gz != nullptr){
			// Открываем исходный файл лога на чтение
			ifstream file(state()._filename, ios::in | ios::binary);
			// Если файл открыт
			if(file.is_open()){
				// Буфер потокового чтения данных (64 Кб)
				vector <char> buffer(0x10000);
				/**
				 * Выполняем потоковое чтение и сжатие файла порциями
				 */
				while(file){
					// Выполняем чтение очередной порции данных
					file.read(buffer.data(), static_cast <streamsize> (buffer.size()));
					// Получаем количество прочитанных байт
					const streamsize bytes = file.gcount();
					// Если данные прочитаны, записываем их в архив
					if(bytes > 0)
						// Выполняем сжатие порции данных
						::gzwrite(gz, buffer.data(), static_cast <uint32_t> (bytes));
				}
				// Закрываем исходный файл
				file.close();
			}
			// Закрываем сжатый файл
			::gzclose(gz);
			// Удаляем исходный файл логов только после успешного сжатия
			::unlink(state()._filename.c_str());
		// Если произошла ошибка сжатия, исходный файл не удаляем (во избежание потери данных)
		} else ::fprintf(stderr, "ERROR! Logging rotate: %s\n\n", ::strerror(errno));
	#endif
	// Выполняем удаление устаревших архивов логов
	this->retention();
}
/**
 * @brief Метод удаления устаревших архивов логов (retention)
 *
 */
void FileSink::retention() const noexcept {
	// Если ограничение на количество архивов не установлено, выходим
	if(state()._maxFiles == 0)
		// Выходим из метода
		return;
	// Получаем компоненты адреса файла лога
	const auto & cmp = components(state()._filename);
	// Определяем каталог хранения архивов
	const string dir = (cmp.first.empty() ? string{"./"} : cmp.first);
	// Базовое имя файла лога без расширения
	const string & base = cmp.second;
	// Если базовое имя файла не определено, выходим
	if(base.empty())
		// Выходим из метода
		return;
	// Список найденных архивов (путь, время модификации)
	vector <pair <string, uintmax_t>> archives;
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		// Формируем маску поиска архивов
		const wstring & mask = awh::fmk::convert(awh::fmk::format("%s%s*.gz", dir.c_str(), base.c_str()));
		// Структура данных результата поиска
		WIN32_FIND_DATAW data;
		// Выполняем поиск первого файла по маске
		HANDLE find = ::FindFirstFileW(mask.c_str(), &data);
		// Если поиск выполнен успешно
		if(find != INVALID_HANDLE_VALUE){
			/**
			 * Перебираем все найденные файлы
			 */
			do {
				// Пропускаем каталоги
				if(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					// Переходим к следующему файлу
					continue;
				// Получаем имя найденного файла
				const string name = awh::fmk::convert(data.cFileName);
				// Пропускаем файлы, не являющиеся архивами лога (имя должно иметь вид <base>_...gz)
				if((name.length() <= (base.length() + 3)) || (name.compare(0, base.length(), base) != 0) ||
				   (name[base.length()] != '_') || (name.compare(name.length() - 3, 3, ".gz") != 0))
					// Переходим к следующему файлу
					continue;
				// Формируем полный путь к архиву
				const string & full = awh::fmk::format("%s%s", dir.c_str(), name.c_str());
				// Формируем время модификации файла
				const uintmax_t mtime = ((static_cast <uintmax_t> (data.ftLastWriteTime.dwHighDateTime) << 32) | data.ftLastWriteTime.dwLowDateTime);
				// Добавляем архив в список
				archives.emplace_back(full, mtime);
			/**
			 * Продолжаем поиск до тех пор пока есть файлы
			 */
			} while(::FindNextFileW(find, &data));
			// Закрываем дескриптор поиска
			::FindClose(find);
		}
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#else
		// Открываем каталог хранения архивов
		awh::dir::DIR * directory = awh::dir::opendir(dir.c_str());
		// Если каталог открыт
		if(directory != nullptr){
			// Объект записи каталога
			awh::dir::dirent * entry = nullptr;
			/**
			 * Перебираем все записи каталога
			 */
			while((entry = awh::dir::readdir(directory)) != nullptr){
				// Получаем имя файла
				const string name = entry->d_name;
				// Проверяем что имя начинается с базового имени лога и оканчивается на .gz
				if((name.length() > (base.length() + 3)) && (name.compare(0, base.length(), base) == 0) &&
				   (name[base.length()] == '_') && (name.compare(name.length() - 3, 3, ".gz") == 0)){
					// Структура для получения статистики файла
					struct stat info{};
					// Формируем полный путь к архиву
					const string full = (dir + name);
					// Получаем время модификации архива
					if(::stat(full.c_str(), &info) == 0)
						// Добавляем архив в список
						archives.emplace_back(full, static_cast <uintmax_t> (info.st_mtime));
				}
			}
			// Закрываем каталог
			awh::dir::closedir(directory);
		}
	#endif
	// Если количество архивов превышает установленный лимит
	if(archives.size() > state()._maxFiles){
		// Выполняем сортировку архивов по времени модификации (от старых к новым)
		std::sort(archives.begin(), archives.end(), [](const auto & a, const auto & b) noexcept -> bool {
			// Сравниваем время модификации
			return (a.second < b.second);
		});
		// Вычисляем количество архивов для удаления
		const size_t count = (archives.size() - state()._maxFiles);
		/**
		 * Удаляем самые старые архивы сверх установленного лимита
		 */
		for(size_t i = 0; i < count; i++){
			/**
			 * Для операционной системы MS Windows
			 */
			#if _WIN32 || _WIN64
				// Удаляем устаревший архив
				::_wunlink(awh::fmk::convert(archives.at(i).first).c_str());
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#else
				// Удаляем устаревший архив
				::unlink(archives.at(i).first.c_str());
			#endif
		}
	}
}
/**
 * @brief Метод формирования уникального имени архива логов
 *
 * @return путь к файлу архива, гарантированно не конфликтующий с существующими
 *
 */
string FileSink::nextArchive() const noexcept {
	// Получаем компоненты адреса файла лога
	const auto & cmp = components(state()._filename);
	// Выполняем извлечение даты для имени архива
	const string & date = state()._chrono.format("_%m-%d-%Y_%H-%M-%S");
	// Лямбда проверки существования файла
	auto exists = [](const string & path) noexcept -> bool {
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Проверяем существование файла по его атрибутам
			return (::GetFileAttributesW(awh::fmk::convert(path).c_str()) != INVALID_FILE_ATTRIBUTES);
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#else
			// Подавляем предупреждение о неиспользуемом параметре захвата
			// Структура для получения статистики файла
			struct stat info{};
			// Проверяем существование файла
			return (::stat(path.c_str(), &info) == 0);
		#endif
	};
	// Формируем базовое имя архива
	string path = awh::fmk::format("%s%s%s.gz", cmp.first.c_str(), cmp.second.c_str(), date.c_str());
	// Если архив с таким именем уже существует (несколько ротаций в течение одной секунды)
	if(exists(path)){
		/**
		 * Подбираем свободное имя с порядковым индексом
		 */
		for(uint32_t i = 1; i > 0; i++){
			// Формируем имя архива с порядковым индексом
			const string candidate = awh::fmk::format("%s%s%s_%u.gz", cmp.first.c_str(), cmp.second.c_str(), date.c_str(), i);
			// Если файла с таким именем нет, используем его
			if(!exists(candidate)){
				// Запоминаем найденное свободное имя
				path = candidate;
				// Выходим из цикла
				break;
			}
		}
	}
	// Возвращаем сформированное имя архива
	return path;
}
/**
 * @brief Метод записи полезной нагрузки в файл
 *
 * @param payload объект полезной нагрузки
 *
 */
void FileSink::write(const payload_t & payload) const noexcept {
	// Если файл для вывода лога не указан, выходим
	if(state()._filename.empty())
		// Выходим из метода
		return;
	// Формируем запись с очищенным от управляющих символов текстом
	payload_t record(payload);
	// Выполняем очистку текста от символов форматирования
	cleaner(record.text);
	// Формируем строку лога без символов цветового форматирования
	const string line = compose(record, false);
	// Получаем идентификатор текущего процесса
	const pid_t pid = ::getpid();
	/**
	 * (Пере)открываем дескриптор если: он ещё не открыт, изменился путь файла,
	 * или мы оказались в дочернем процессе после fork (унаследованный дескриптор).
	 */
	if((this->_fd == -1) || (this->_opened != state()._filename) || (this->_pid != pid))
		// Выполняем (пере)открытие постоянного дескриптора записи
		this->reopen();
	// Если дескриптор открыть не удалось, выходим
	if(this->_fd == -1)
		// Выходим из метода
		return;
	// Указатель на данные для записи
	const char * data = line.data();
	// Количество оставшихся для записи байт
	size_t remaining = line.size();
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		/**
		 * Выполняем запись строки лога с учётом возможной частичной записи
		 */
		while(remaining > 0){
			// Количество записанных байт
			DWORD written = 0;
			// Выполняем запись очередной порции данных в файл
			if(!::WriteFile(reinterpret_cast <HANDLE> (this->_fd), static_cast <LPCVOID> (data), static_cast <DWORD> (remaining), &written, nullptr) || (written == 0)){
				// Закрываем дескриптор, чтобы переоткрыть его при следующей записи
				::CloseHandle(reinterpret_cast <HANDLE> (this->_fd));
				// Сбрасываем дескриптор
				this->_fd = -1;
				// Прерываем цикл записи
				break;
			}
			// Смещаем указатель на записанное количество байт
			data += written;
			// Уменьшаем количество оставшихся для записи байт
			remaining -= static_cast <size_t> (written);
		}
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#else
		/**
		 * Выполняем запись строки лога с учётом возможной частичной записи
		 */
		while(remaining > 0){
			// Выполняем запись очередной порции данных в файл
			const ssize_t written = ::write(static_cast <int32_t> (this->_fd), data, remaining);
			// Если запись завершилась ошибкой
			if(written <= 0){
				// Если запись прервана сигналом, повторяем попытку
				if((written < 0) && (errno == EINTR))
					// Повторяем попытку записи
					continue;
				// Закрываем дескриптор, чтобы переоткрыть его при следующей записи
				::close(static_cast <int32_t> (this->_fd));
				// Сбрасываем файловый дескриптор
				this->_fd = -1;
				// Прерываем цикл записи
				break;
			}
			// Смещаем указатель на записанное количество байт
			data += written;
			// Уменьшаем количество оставшихся для записи байт
			remaining -= static_cast <size_t> (written);
		}
	#endif
	// Если в процессе записи произошла ошибка, выходим
	if(this->_fd == -1)
		// Выходим из метода
		return;
	// Увеличиваем накопленный размер файла лога
	this->_size += line.size();
	// Если накопленный размер файла превышает максимально-установленный
	if(this->_size >= state()._maxSize){
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Закрываем текущий дескриптор перед ротацией
			::CloseHandle(reinterpret_cast <HANDLE> (this->_fd));
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#else
			// Закрываем текущий дескриптор перед ротацией
			::close(static_cast <int32_t> (this->_fd));
		#endif
		// Сбрасываем дескриптор
		this->_fd = -1;
		// Выполняем ротацию файла лога
		this->rotate();
		// Заново открываем дескриптор записи (исходный файл удалён ротацией)
		this->reopen();
	}
}
/**
 * @brief Конструктор
 *
 */
FileSink::FileSink() noexcept :
 Sink(), _pid(0), _fd(-1), _opened{""}, _size(0) {}
/**
 * @brief Деструктор
 *
 */
FileSink::~FileSink() noexcept {
	// Если дескриптор открыт, закрываем его
	if(this->_fd != -1){
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Закрываем дескриптор файла
			::CloseHandle(reinterpret_cast <HANDLE> (this->_fd));
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#else
			// Закрываем файловый дескриптор
			::close(static_cast <int32_t> (this->_fd));
		#endif
		// Сбрасываем дескриптор
		this->_fd = -1;
	}
}

/**
 * @brief Метод отправки полезной нагрузки в SysLog
 *
 * @param payload объект полезной нагрузки
 *
 */
void SyslogSink::write(const payload_t & payload) const noexcept {
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#if !_WIN32 && !_WIN64
		// Открываем SysLog для нашего приложения
		::openlog(!state()._name.empty() ? state()._name.c_str() : AWH_SHORT_NAME, LOG_PID, LOG_USER);
		// Уровень сообщения SysLog
		int32_t priority = LOG_NOTICE;
		/**
		 * Определяем тип сообщения
		 */
		switch(static_cast <uint8_t> (payload.flag)){
			// Записываем в лог сообщение так-как оно есть
			case static_cast <uint8_t> (flag_t::NONE):
				// Устанавливаем уровень уведомления
				priority = LOG_NOTICE;
			break;
			// Печатаем информационное сообщение
			case static_cast <uint8_t> (flag_t::INFO):
				// Устанавливаем информационный уровень
				priority = LOG_INFO;
			break;
			// Записываем ошибку в лог
			case static_cast <uint8_t> (flag_t::CRITICAL):
				// Устанавливаем уровень ошибки
				priority = LOG_ERR;
			break;
			// Записываем в лог сообщение предупреждения
			case static_cast <uint8_t> (flag_t::WARNING):
				// Устанавливаем уровень предупреждения
				priority = LOG_WARNING;
			break;
		}
		// Выполняем отправку сообщения
		::syslog(priority, "%s", payload.text.c_str());
		// Закрываем SysLog
		::closelog();
	/**
	 * Для операционной системы MS Windows
	 */
	#else
		// Подавляем предупреждение о неиспользуемом параметре
		(void) payload;
	#endif
}

/**
 * @brief Метод передачи полезной нагрузки в функцию обратного вызова
 *
 * @param payload объект полезной нагрузки
 *
 */
void CallbackSink::write(const payload_t & payload) const noexcept {
	// Если функция подписки на логи установлена, рассылаем сообщение подписчику
	if(state()._callback != nullptr)
		// Рассылаем сообщение лога подписчику
		state()._callback(payload.flag, payload.text);
}

/**
 * @brief Функция перестроения набора приёмников по текущему списку режимов
 *
 */
void rebuild(State & self) noexcept {
	// Очищаем текущий набор приёмников
	self._sinks.clear();
	// Если разрешён вывод логов в функцию обратного вызова
	if(self._mode.find(mode_t::DEFERRED) != self._mode.end())
		// Добавляем приёмник функции обратного вызова
		self._sinks.push_back(std::make_unique <CallbackSink> ());
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#if !_WIN32 && !_WIN64
		// Если разрешена отправка логов в SysLog
		if(self._mode.find(mode_t::SYSLOG) != self._mode.end())
			// Добавляем приёмник SysLog
			self._sinks.push_back(std::make_unique <SyslogSink> ());
	#endif
	// Если разрешён вывод логов в консоль
	if(self._mode.find(mode_t::CONSOLE) != self._mode.end())
		// Добавляем приёмник консоли
		self._sinks.push_back(std::make_unique <ConsoleSink> ());
	// Если разрешён вывод логов в файл
	if(self._mode.find(mode_t::FILE) != self._mode.end())
		// Добавляем приёмник файла
		self._sinks.push_back(std::make_unique <FileSink> ());
}

/**
 * @brief Функция проверки разрешён ли вывод лога для указанного флага
 *
 * @param flag флаг типа логирования
 * @return     результат проверки соответствия уровню логирования
 *
 */
bool allowed(const flag_t flag) noexcept {
	// Выполняем проверку соответствия флага установленному уровню логирования
	return (
		(state()._level == level_t::ALL) ||
		((state()._level == level_t::INFO) && (flag == flag_t::INFO)) ||
		((state()._level == level_t::WARNING) && (flag == flag_t::WARNING)) ||
		((state()._level == level_t::CRITICAL) && (flag == flag_t::CRITICAL)) ||
		((state()._level == level_t::INFO_WARNING) && ((flag == flag_t::INFO) || (flag == flag_t::WARNING))) ||
		((state()._level == level_t::INFO_CRITICAL) && ((flag == flag_t::INFO) || (flag == flag_t::CRITICAL))) ||
		((state()._level == level_t::WARNING_CRITICAL) && ((flag == flag_t::WARNING) || (flag == flag_t::CRITICAL)))
	);
}
/**
 * @brief Функция очистки строки от символов форматирования
 *
 * @param text текст для очистки
 * @return     очищенный текст
 *
 */
string & cleaner(string & text) noexcept {
	// Позиция найденного элемента
	size_t pos = 0;
	/**
	 * Выполняем поиск символов экранирования
	 */
	while((pos = text.find("\x1B[", pos)) != string::npos){
		// Флаг обнаружения завершения блока экранирования
		bool found = false;
		/**
		 * Выполняем поиск завершения блока экранирования (начиная с первого байта параметров)
		 */
		for(size_t i = (pos + 2); i < text.length(); i++){
			// Выполняем получение текущего символа
			const char letter = text[i];
			// Если мы получили символ завершения блока
			if(letter == 'm'){
				// Выполняем удаление всей последовательности экранирования
				text.erase(pos, (i + 1) - pos);
				// Устанавливаем флаг обнаружения завершения
				found = true;
				// Выходим из цикла
				break;
			// Если символ не является числом и не является разделителем параметров
			} else if(!awh::fmk::is(letter, awh::fmk::check_t::NUMBER) && (letter != ';')) {
				// Удаляем некорректную (незавершённую) последовательность экранирования
				text.erase(pos, i - pos);
				// Устанавливаем флаг обнаружения завершения
				found = true;
				// Выходим из цикла
				break;
			}
		}
		// Если завершение последовательности не найдено (обрыв в конце строки), прекращаем разбор
		if(!found)
			// Выходим из цикла во избежание зацикливания
			break;
	}
	// Возвращаем результат
	return text;
}
/**
 * @brief Функция маршрутизации полезной нагрузки в приёмники (синхронно или асинхронно)
 *
 * @param payload объект полезной нагрузки
 *
 */
void dispatch(payload_t && payload) noexcept {
	// Если асинхронный режим работы не активирован, выводим сообщение синхронно
	if(!state()._async){
		// Выполняем синхронный вывод полученного лога
		receiving(payload);
		// Выходим из метода
		return;
	}
	// Получаем идентификатор текущего процесса
	const pid_t pid = ::getpid();
	/**
	 * Быстрая проверка без блокировки: в типовом случае (тот же процесс и живой поток)
	 * управление жизненным циклом скрина не требуется.
	 */
	if((pid != state()._pid.load(std::memory_order_acquire)) || !static_cast <bool> (state()._screen)){
		// Выполняем блокировку потока на время управления жизненным циклом скрина
		const locker_t <> lock(state()._mtx);
		// Если идентификатор процесса сменился (например, после fork)
		if(pid != state()._pid.load(std::memory_order_acquire)){
			// Запоминаем идентификатор текущего процесса
			state()._pid.store(pid, std::memory_order_release);
			/**
			 * Останавливаем унаследованный скрин. Так-как Screen самостоятельно
			 * обнаруживает смену процесса, join() унаследованного потока не выполняется.
			 */
			state()._screen.stop();
		}
		// Если дочерний поток не создан
		if(!static_cast <bool> (state()._screen)){
			// Выполняем установку функции обратного вызова
			state()._screen = static_cast <function <void (const payload_t &)>> (std::bind(&receiving, _1));
			// Применяем ограничение размера очереди асинхронного вывода
			state()._screen.capacity(state()._maxQueue);
			// Применяем политику поведения при переполнении очереди
			state()._screen.overflow(static_cast <screen_t <payload_t>::overflow_t> (state()._overflow));
			// Запускаем работу скрина
			state()._screen.start();
		}
	}
	// Выполняем отправку сообщения дочернему потоку
	state()._screen = ::move(payload);
}
/**
 * @brief Функция получения данных
 *
 * @param payload объект полезной нагрузки
 *
 */
void receiving(const payload_t & payload) noexcept {
	// Выполняем блокировку потока
	const locker_t <> lock(state()._mtx);
	/**
	 * Выполняем перебор всех установленных приёмников вывода логов
	 */
	for(const auto & sink : state()._sinks){
		// Если приёмник создан, выполняем запись полезной нагрузки
		if(sink != nullptr)
			// Записываем полезную нагрузку в приёмник
			sink->write(payload);
	}
}
/**
 * @brief Функция извлечения компонента адреса файла
 *
 * @param filename адрес где находится файл
 * @return         параметры компонента (адрес, название файла без расширения)
 *
 */
pair <string, string> components(string_view filename) noexcept {
	// Переменная результата
	pair <string, string> result;
	// Если адрес передан
	if(!filename.empty()){
		// Позиция разделителя каталога и расширения файла
		size_t pos1 = 0, pos2 = 0;
		// Выполняем поиск разделителя каталога
		if((pos1 = filename.rfind(AWH_FS_SEPARATOR, filename.length() - 1)) != string::npos){
			// Устанавливаем путь к каталогу где хранится файл (включая разделитель)
			result.first = filename.substr(0, pos1 + 1);
			// Если расширение файла найдено
			if((pos2 = filename.find('.', pos1 + 1)) != string::npos)
				// Устанавливаем название файла без расширения
				result.second = filename.substr(pos1 + 1, pos2 - (pos1 + 1));
			// Если расширение не найдено, используем имя файла целиком
			else result.second = filename.substr(pos1 + 1);
		// Если разделитель каталога не найден
		} else {
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#if !_WIN32 && !_WIN64
				// Устанавливаем путь к текущему каталогу
				result.first.append("./");
			#endif
			// Если расширение файла найдено
			if((pos2 = filename.find('.')) != string::npos)
				// Устанавливаем название файла без расширения
				result.second = filename.substr(0, pos2);
			// Если расширение не найдено, используем имя файла целиком
			else result.second = filename;
		}
		/**
		 * Если название файла извлечь не удалось (например, скрытый файл вида ".log"
		 * или путь оканчивается разделителем), подставляем имя-заглушку, чтобы
		 * формирование имени архива и его поиск при retention оставались согласованными.
		 */
		if(result.second.empty())
			// Добавляем имя-заглушку для формирования имени архива (в случае если имя файла не указано, например, при указании каталога)
			result.second.append(state()._name.empty() ? AWH_SHORT_NAME : state()._name);
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Функция формирования итоговой строки лога
 *
 * @param payload объект полезной нагрузки
 * @param colored нужно ли добавлять символы цветового форматирования
 * @return        сформированная строка лога
 *
 */
string compose(const payload_t & payload, const bool colored) noexcept {
	// Флаг конца строки
	bool isEnd = false;
	// Если размер буфера меньше 3-х байт
	if(payload.text.length() < 3)
		// Проверяем является ли это переводом строки
		isEnd = ((payload.text.compare(AWH_STRING_BREAK) == 0) || (payload.text.compare(AWH_STRING_BREAKS) == 0));
	// Определяем хвост сообщения (перенос строки добавляем только если его ещё нет)
	const char * tail = (!isEnd ? AWH_STRING_BREAKS : "");
	/**
	 * Определяем тип сообщения
	 */
	switch(static_cast <uint8_t> (payload.flag)){
		// Записываем в лог сообщение так-как оно есть
		case static_cast <uint8_t> (flag_t::NONE):
			// Формируем текстовый вид лога
			return awh::fmk::format("%s%s", payload.text.c_str(), tail);
		// Печатаем информационное сообщение
		case static_cast <uint8_t> (flag_t::INFO):
			// Формируем текстовый вид лога
			return (
				colored ?
				awh::fmk::format("\x1B[32m\x1B[1mInfo\x1B[0m \x1B[32m%s %s :\x1B[0m %s%s", payload.date.c_str(), state()._name.c_str(), payload.text.c_str(), tail) :
				awh::fmk::format("Info %s %s : %s%s", payload.date.c_str(), state()._name.c_str(), payload.text.c_str(), tail)
			);
		// Записываем ошибку в лог
		case static_cast <uint8_t> (flag_t::CRITICAL):
			// Формируем текстовый вид лога
			return (
				colored ?
				awh::fmk::format("\x1B[31m\x1B[1mError\x1B[0m \x1B[31m%s %s :\x1B[0m %s%s", payload.date.c_str(), state()._name.c_str(), payload.text.c_str(), tail) :
				awh::fmk::format("Error %s %s : %s%s", payload.date.c_str(), state()._name.c_str(), payload.text.c_str(), tail)
			);
		// Записываем в лог сообщение предупреждения
		case static_cast <uint8_t> (flag_t::WARNING):
			// Формируем текстовый вид лога
			return (
				colored ?
				awh::fmk::format("\x1B[33m\x1B[1mWarning\x1B[0m \x1B[33m%s %s :\x1B[0m %s%s", payload.date.c_str(), state()._name.c_str(), payload.text.c_str(), tail) :
				awh::fmk::format("Warning %s %s : %s%s", payload.date.c_str(), state()._name.c_str(), payload.text.c_str(), tail)
			);
	}
	// Возвращаем пустой результат
	return "";
}
}
}
}

namespace awh {
/**
 * @brief Пространство имён работы с логами
 *
 */
namespace log {
/**
 * @brief Пространство имён построения состояния модуля логирования
 *
 */
namespace {
	/**
	 * @brief Конструктор
	 *
	 */
	State::State() noexcept :
	 _async(false), _level(level_t::ALL), _sep(separator_t::ALWAYS),
	 _name{AWH_SHORT_NAME}, _format{DATE_FORMAT}, _filename{""},
	 _maxSize(MAX_SIZE_LOGFILE), _sepSize(0x400), _maxQueue(0), _maxFiles(0),
	 _overflow(overflow_t::DROP_OLD), _pid(0),
	 _counter{1}, _screen(awh::Screen <payload_t>::health_t::DEAD), _callback(nullptr) {
		// Запоминаем идентификатор родительского процесса
		this->_pid = ::getpid();
		/**
		 * Деактивируем мьютекс по умолчанию (основа фреймворка - однопоточный event-loop + fork,
		 * потокобезопасность включается разработчиком явно через threadSafety(true))
		 */
		this->_mtx.enabled = false;
		// Выполняем разрешение на вывод всех видов логов
		this->_mode = {mode_t::FILE, mode_t::CONSOLE, mode_t::DEFERRED};
		// Выполняем построение набора приёмников вывода логов
		rebuild(* this);
	}
	/**
	 * @brief Деструктор
	 *
	 */
	State::~State() noexcept {
		// Если объект работы с дочерним потоком создан, удаляем
		if(static_cast <bool> (this->_screen))
			// Останавливаем работу скрина
			this->_screen.stop();
	}
	/**
	 * @brief Функция вывода записи с уже заведённым списком аргументов
	 *
	 * @details Тело это общее у print() и debug(): доводы «...» дальше по вызову не
	 *          передаются никак, кроме как `va_list`, - оттого вывод и разрезан надвое.
	 *          Прежде разреза не было, а debug() был шаблоном и передавал доводы прямою
	 *          пачкою `args...`; шаблон же вынуждал держать сборку записи в заголовочном файле
	 *
	 * @param format формат строки вывода
	 * @param flag   флаг типа логирования
	 * @param args   заведённый список аргументов формирования записи
	 *
	 */
	void emit(string_view format, awh::log::flag_t flag, va_list args) noexcept {
		// Если формат передан и уровень логирования соответствует
		if(!format.empty() && allowed(flag)){
			// Создаём текст для логирования
			const string text{format};
			// Буфер данных для логирования
			vector <char> buffer(1024);
			// Результирующая строка логирования
			string result;
			/**
			 * Выполняем формирование строки лога с учётом списка аргументов
			 */
			for(;;){
				// Создаем список аргументов
				va_list args2;
				// Копируем список аргументов
				va_copy(args2, args);
				// Выполняем запись в буфер данных
				const int32_t res = ::vsnprintf(&buffer[0], buffer.size(), text.c_str(), args2);
				// Завершаем список локальных аргументов
				va_end(args2);
				// Если произошла ошибка форматирования, прекращаем разбор
				if(res < 0)
					// Выходим из цикла
					break;
				// Если строка полностью поместилась в буфер
				if(static_cast <size_t> (res) < buffer.size()){
					// Копируем сформированную строку
					result.assign(buffer.data(), static_cast <size_t> (res));
					// Выходим из цикла
					break;
				}
				// Увеличиваем буфер под требуемый размер (vsnprintf вернул необходимую длину)
				buffer.resize(static_cast <size_t> (res) + 1);
			}
			// Если результирующая строка сформирована
			if(!result.empty()){
				// Создаём объект полезной нагрузки
				payload_t payload;
				// Устанавливаем флаг логирования
				payload.flag = flag;
				// Устанавливаем данные сообщения
				payload.text = ::move(result);
				// Фиксируем дату формирования сообщения в момент вызова
				payload.date = state()._chrono.format(state()._format);
				// Выполняем маршрутизацию полезной нагрузки в приёмники
				dispatch(::move(payload));
			}
		}
	}

	/**
	 * @brief Функция вывода широкой записи с уже заведённым списком аргументов
	 *
	 * @param format формат строки вывода
	 * @param flag   флаг типа логирования
	 * @param args   заведённый список аргументов формирования записи
	 *
	 */
	void emit(wstring_view format, awh::log::flag_t flag, va_list args) noexcept {
		// Если формат передан и уровень логирования соответствует
		if(!format.empty() && allowed(flag)){
			// Создаём текст для логирования
			const wstring text{format};
			// Буфер данных для логирования
			vector <wchar_t> buffer(1024);
			// Результирующая строка логирования
			wstring result;
			/**
			 * Выполняем формирование строки лога с учётом списка аргументов
			 */
			for(;;){
				// Создаем список аргументов
				va_list args2;
				// Копируем список аргументов
				va_copy(args2, args);
				// Выполняем запись в буфер данных
				const int32_t res = ::vswprintf(&buffer[0], buffer.size(), text.c_str(), args2);
				// Завершаем список локальных аргументов
				va_end(args2);
				// Если строка успешно сформирована и поместилась в буфер
				if((res >= 0) && (static_cast <size_t> (res) < buffer.size())){
					// Копируем сформированную строку
					result.assign(buffer.data(), static_cast <size_t> (res));
					// Выходим из цикла
					break;
				}
				/**
				 * Функция vswprintf не возвращает требуемую длину буфера, поэтому при
				 * нехватке места увеличиваем буфер вдвое. Предохранитель ограничивает
				 * максимальный размер во избежание бесконечного цикла при ошибке.
				 */
				if(buffer.size() >= 0x100000)
					// Выходим из цикла (предохранитель)
					break;
				// Увеличиваем размер буфера в два раза
				buffer.resize(buffer.size() * 2);
			}
			// Если результирующая строка сформирована
			if(!result.empty()){
				// Создаём объект полезной нагрузки
				payload_t payload;
				// Устанавливаем флаг логирования
				payload.flag = flag;
				// Устанавливаем данные сообщения
				payload.text = awh::fmk::convert(result);
				// Фиксируем дату формирования сообщения в момент вызова
				payload.date = state()._chrono.format(state()._format);
				// Выполняем маршрутизацию полезной нагрузки в приёмники
				dispatch(::move(payload));
			}
		}
	}

	/**
	 * @brief Внутренние средства сборки отладочной записи
	 *
	 */
	/**
	 * @brief Функция сведения доводов метода к строке
	 *
	 * @param params доводы, переданные в метод
	 * @return       сведённая строка доводов, пустая при отсутствии доводов
	 *
	 */
	string serialization(std::initializer_list <awh::log::arg_t> params) noexcept {
		// Строка сведённых доводов
		string result;
		// Если доводы не поданы вовсе, сводить нечего
		if(params.size() == 0)
			// Выводим строку пустую: сборщик по ней узнаёт об их отсутствии
			return result;
		// Выполняем добавление открывающей скобки
		result.append(1, '(');
		// Признак того, что очередной довод первый
		bool first = true;
		/**
		 * Выполняем перебор всех поданных доводов
		 */
		for(const awh::log::arg_t & param : params){
			// Если довод не первый, отделяем его от предыдущего
			if(!first)
				// Выполняем добавление разделителя доводов
				result.append(", ");
			// Снимаем признак первого довода
			first = false;
			// Выполняем укладку очередного довода
			param.lay(result);
		}
		// Выполняем добавление закрывающей скобки
		result.append(1, ')');
		// Выводим строку сведённых доводов
		return result;
	}
	/**
	 * @brief Функция сборки отладочной записи
	 *
	 * @details Сборка эта прежде стояла в заголовочном файле, в телах самих шаблонов
	 *          debug(), и разбиралась заново каждою единицей трансляции, хотя от видов
	 *          доводов не зависит ни единою своей частью
	 *
	 * @param format формат строки вывода
	 * @param method название вызываемого метода
	 * @param params доводы, переданные в метод
	 * @return       собранная отладочная запись
	 *
	 */
	string assemble(string_view format, string_view method, std::initializer_list <awh::log::arg_t> params) noexcept {
		// Сведённые доводы метода
		const string & arguments = serialization(params);
		// Собираемая отладочная запись
		string result = AWH_STRING_BREAKS"\x1B[1mCalled function:\x1B[0m" AWH_STRING_BREAK;
		// Добавляем название вызываемого метода
		result.append(method);
		// Добавляем перенос строки
		result.append(AWH_STRING_BREAKS);
		/**
		 * Если доводы метода поданы
		 */
		if(!arguments.empty()){
			// Добавляем заголовок доводов метода
			result.append("\x1B[1mArguments function:\x1B[0m" AWH_STRING_BREAK);
			// Добавляем сами доводы метода
			result.append(arguments);
			// Добавляем перенос строки
			result.append(AWH_STRING_BREAKS);
			// Добавляем заголовок самого сообщения
			result.append("\x1B[1mMessage:\x1B[0m" AWH_STRING_BREAK);
		}
		// Добавляем формат сообщения
		result.append(format);
		// Выводим собранную отладочную запись
		return result;
	}
}
}
}


/**
 * @brief Функция вывода текстовой информации в консоль или файл
 *
 * @param format формат строки вывода
 * @param flag   флаг типа логирования
 *
 */
/**
 * @brief Заведение пустого довода
 *
 */
awh::log::Argument::Argument() noexcept : _kind(kind_t::NONE), _signed(0) {}
/**
 * @brief Заведение логического довода
 *
 * @param value укладываемое значение довода
 *
 */
awh::log::Argument::Argument(const bool value) noexcept : _kind(kind_t::BOOLEAN), _boolean(value) {}
/**
 * @brief Заведение символьного довода
 *
 * @param value укладываемое значение довода
 *
 */
awh::log::Argument::Argument(const char value) noexcept : _kind(kind_t::SYMBOL), _symbol(value) {}
/**
 * @brief Заведения знаковых числовых доводов
 *
 * @param value укладываемое значение довода
 *
 */
awh::log::Argument::Argument(const signed char value) noexcept : _kind(kind_t::SIGNED), _signed(static_cast <int64_t> (value)) {}
awh::log::Argument::Argument(const short value) noexcept : _kind(kind_t::SIGNED), _signed(static_cast <int64_t> (value)) {}
awh::log::Argument::Argument(const int value) noexcept : _kind(kind_t::SIGNED), _signed(static_cast <int64_t> (value)) {}
awh::log::Argument::Argument(const long value) noexcept : _kind(kind_t::SIGNED), _signed(static_cast <int64_t> (value)) {}
awh::log::Argument::Argument(const long long value) noexcept : _kind(kind_t::SIGNED), _signed(static_cast <int64_t> (value)) {}
/**
 * @brief Заведения беззнаковых числовых доводов
 *
 * @param value укладываемое значение довода
 *
 */
awh::log::Argument::Argument(const unsigned char value) noexcept : _kind(kind_t::UNSIGNED), _unsigned(static_cast <uint64_t> (value)) {}
awh::log::Argument::Argument(const unsigned short value) noexcept : _kind(kind_t::UNSIGNED), _unsigned(static_cast <uint64_t> (value)) {}
awh::log::Argument::Argument(const unsigned int value) noexcept : _kind(kind_t::UNSIGNED), _unsigned(static_cast <uint64_t> (value)) {}
awh::log::Argument::Argument(const unsigned long value) noexcept : _kind(kind_t::UNSIGNED), _unsigned(static_cast <uint64_t> (value)) {}
awh::log::Argument::Argument(const unsigned long long value) noexcept : _kind(kind_t::UNSIGNED), _unsigned(static_cast <uint64_t> (value)) {}
/**
 * @brief Заведения дробных числовых доводов
 *
 * @param value укладываемое значение довода
 *
 */
awh::log::Argument::Argument(const float value) noexcept : _kind(kind_t::REAL), _real(static_cast <double> (value)) {}
awh::log::Argument::Argument(const double value) noexcept : _kind(kind_t::REAL), _real(value) {}
awh::log::Argument::Argument(const long double value) noexcept : _kind(kind_t::REAL), _real(static_cast <double> (value)) {}
/**
 * @brief Заведения узких строковых доводов
 *
 * @details Строка держится обзором, а не копией: довод живёт до конца полного выражения
 *          вызова, и записи журнала того довольно
 *
 * @param value укладываемое значение довода
 *
 */
awh::log::Argument::Argument(const char * value) noexcept : _kind(kind_t::TEXT), _text(value != nullptr ? string_view(value) : string_view()) {}
awh::log::Argument::Argument(char * value) noexcept : _kind(kind_t::TEXT), _text(value != nullptr ? string_view(value) : string_view()) {}
awh::log::Argument::Argument(const string & value) noexcept : _kind(kind_t::TEXT), _text(value) {}
awh::log::Argument::Argument(string_view value) noexcept : _kind(kind_t::TEXT), _text(value) {}
/**
 * @brief Заведения широких строковых доводов
 *
 * @param value укладываемое значение довода
 *
 */
awh::log::Argument::Argument(const wchar_t * value) noexcept : _kind(kind_t::WTEXT), _wtext(value != nullptr ? wstring_view(value) : wstring_view()) {}
awh::log::Argument::Argument(wchar_t * value) noexcept : _kind(kind_t::WTEXT), _wtext(value != nullptr ? wstring_view(value) : wstring_view()) {}
awh::log::Argument::Argument(const wstring & value) noexcept : _kind(kind_t::WTEXT), _wtext(value) {}
awh::log::Argument::Argument(wstring_view value) noexcept : _kind(kind_t::WTEXT), _wtext(value) {}
/**
 * @brief Заведения указательных доводов
 *
 * @param value укладываемое значение довода
 *
 */
awh::log::Argument::Argument(const void * value) noexcept : _kind(kind_t::POINTER), _pointer(value) {}
awh::log::Argument::Argument(std::nullptr_t) noexcept : _kind(kind_t::POINTER), _pointer(nullptr) {}
/**
 * @brief Метод укладки довода в строку доводов
 *
 * @details Вид укладки всякого довода взят не по вкусу, а по прежнему поведению: доводы
 *          укладывались строковым потоком, и вид записи журнала обязан был остаться тем же.
 *          Оттого логическое укладывается числом, а не словом, указатель пустой - нулём,
 *          а дробное видом «%g» с шестью значащими
 *
 * @warning Дробному сюда просится `fmk::noexp`, и это было бы ошибкой: он для того и
 *          заведён, чтобы порядок РАЗВОРАЧИВАТЬ, - `1e-308` вышел бы тремя сотнями нулей,
 *          а бесконечность с не-числом обратились бы нулём. Сличением с прежней укладкой
 *          поймано на 22 значениях из 30
 *
 * @note Одно расхождение с прежней укладкой оставлено НАМЕРЕННО: довод `uint8_t` поток
 *       выводил ЗНАКОМ, а не числом, - байт со значением 200 уходил в журнал непечатным
 *       знаком. Ровно оттого по дереву и стоят две с половиною тысячи приведений к
 *       `uint16_t`: обход этот заводился вручную у всякого места. Двадцать два места
 *       обойдены им НЕ были и печатали мусор; ныне байт укладывается числом
 *
 * @param result строка доводов, куда ведётся укладка
 *
 */
void awh::log::Argument::lay(string & result) const noexcept {
	// Место под запись числового довода
	char buffer[64];
	/**
	 * Выполняем укладку довода по его виду
	 */
	switch(static_cast <uint8_t> (this->_kind)){
		// Если довод логический
		case static_cast <uint8_t> (kind_t::BOOLEAN):
			// Выполняем укладку логического довода числом
			result.append(1, (this->_boolean ? '1' : '0'));
		break;
		// Если довод символьный
		case static_cast <uint8_t> (kind_t::SYMBOL):
			// Выполняем укладку символьного довода
			result.append(1, this->_symbol);
		break;
		// Если довод числовой со знаком
		case static_cast <uint8_t> (kind_t::SIGNED):
			// Выполняем укладку знакового довода
			result.append(std::to_string(this->_signed));
		break;
		// Если довод числовой без знака
		case static_cast <uint8_t> (kind_t::UNSIGNED):
			// Выполняем укладку беззнакового довода
			result.append(std::to_string(this->_unsigned));
		break;
		// Если довод числовой дробный
		case static_cast <uint8_t> (kind_t::REAL): {
			// Выполняем запись дробного довода
			const int32_t length = ::snprintf(buffer, sizeof(buffer), "%g", this->_real);
			// Если запись состоялась
			if(length > 0)
				// Выполняем укладку дробного довода
				result.append(buffer, static_cast <size_t> (length));
		} break;
		// Если довод строковый узкий
		case static_cast <uint8_t> (kind_t::TEXT):
			// Выполняем укладку узкой строки
			result.append(this->_text);
		break;
		// Если довод строковый широкий
		case static_cast <uint8_t> (kind_t::WTEXT):
			// Выполняем укладку широкой строки, обратив её узкою рамкою
			result.append(awh::fmk::convert(wstring(this->_wtext)));
		break;
		// Если довод указателем
		case static_cast <uint8_t> (kind_t::POINTER): {
			/**
			 * Если указатель пустым является
			 */
			if(this->_pointer == nullptr){
				// Выполняем укладку пустого указателя нулём
				result.append(1, '0');
				// Выходим из разбора
				break;
			}
			// Выполняем запись адреса указателя
			const int32_t length = ::snprintf(buffer, sizeof(buffer), "0x%llx", static_cast <unsigned long long> (reinterpret_cast <uintptr_t> (this->_pointer)));
			// Если запись состоялась
			if(length > 0)
				// Выполняем укладку адреса указателя
				result.append(buffer, static_cast <size_t> (length));
		} break;
	}
}


/**
 * @brief Функция вывода отладочной информации в консоль или файл
 *
 * @param format формат строки вывода
 * @param method название вызываемого метода
 * @param params доводы, переданные в метод
 * @param flag   флаг типа логирования
 *
 */
void awh::log::debug(string_view format, string_view method, std::initializer_list <arg_t> params, flag_t flag, ...) noexcept {
	// Если формат строки вывода не передан, выводить нечего
	if(format.empty())
		// Выходим из функции
		return;
	// Список аргументов формирования записи
	va_list args;
	// Запускаем инициализацию списка аргументов
	va_start(args, flag);
	/**
	 * Если название вызываемого метода не передано
	 */
	if(method.empty())
		// Пишем запись без отладочной обвязки
		emit(format, flag, args);
	/**
	 * Если название вызываемого метода передано
	 */
	else {
		// Выполняем сборку отладочной записи
		const string & record = assemble(format, method, params);
		// Пишем собранную отладочную запись
		emit(string_view(record), flag, args);
	}
	// Завершаем список аргументов
	va_end(args);
}
/**
 * @brief Функция вывода отладочной информации в консоль или файл
 *
 * @param format формат строки вывода
 * @param method название вызываемого метода
 * @param params доводы, переданные в метод
 * @param flag   флаг типа логирования
 *
 */
void awh::log::debug(wstring_view format, string_view method, std::initializer_list <arg_t> params, flag_t flag, ...) noexcept {
	// Если формат строки вывода не передан, выводить нечего
	if(format.empty())
		// Выходим из функции
		return;
	// Список аргументов формирования записи
	va_list args;
	// Запускаем инициализацию списка аргументов
	va_start(args, flag);
	/**
	 * Если название вызываемого метода не передано
	 */
	if(method.empty())
		// Пишем запись без отладочной обвязки
		emit(format, flag, args);
	/**
	 * Если название вызываемого метода передано
	 */
	else {
		// Выполняем сборку записи, обратив формат узкою записью рамкою
		const string & record = assemble(string_view(awh::fmk::convert(wstring(format))), method, params);
		// Пишем собранную отладочную запись широким выводом
		emit(wstring_view(awh::fmk::convert(record)), flag, args);
	}
	// Завершаем список аргументов
	va_end(args);
}
/**
 * @brief Функция вывода отладочной информации в консоль или файл
 *
 * @param format формат строки вывода
 * @param method название вызываемого метода
 * @param params доводы, переданные в метод
 * @param flag   флаг типа логирования
 * @param args   список аргументов для подстановки
 *
 */
void awh::log::debug(string_view format, string_view method, std::initializer_list <arg_t> params, flag_t flag, const vector <string> & args) noexcept {
	// Если формат строки вывода не передан, выводить нечего
	if(format.empty())
		// Выходим из функции
		return;
	/**
	 * Если название вызываемого метода не передано
	 */
	if(method.empty()){
		// Пишем запись без отладочной обвязки
		print(format, flag, args);
		// Выходим из функции
		return;
	}
	// Пишем собранную отладочную запись
	print(string_view(assemble(format, method, params)), flag, args);
}
/**
 * @brief Функция вывода отладочной информации в консоль или файл
 *
 * @param format формат строки вывода
 * @param method название вызываемого метода
 * @param params доводы, переданные в метод
 * @param flag   флаг типа логирования
 * @param args   список аргументов для подстановки
 *
 */
void awh::log::debug(wstring_view format, string_view method, std::initializer_list <arg_t> params, flag_t flag, const vector <wstring> & args) noexcept {
	// Если формат строки вывода не передан, выводить нечего
	if(format.empty())
		// Выходим из функции
		return;
	/**
	 * Если название вызываемого метода не передано
	 */
	if(method.empty()){
		// Пишем запись без отладочной обвязки
		print(format, flag, args);
		// Выходим из функции
		return;
	}
	// Выполняем сборку записи, обратив формат узкою записью рамкою
	const string & record = assemble(string_view(awh::fmk::convert(wstring(format))), method, params);
	// Пишем собранную отладочную запись широким выводом: широки и доводы подстановки
	print(wstring_view(awh::fmk::convert(record)), flag, args);
}

void awh::log::print(string_view format, flag_t flag, ...) noexcept {
	// Список аргументов формирования записи
	va_list args;
	// Запускаем инициализацию списка аргументов
	va_start(args, flag);
	// Выполняем вывод записи общим телом
	emit(format, flag, args);
	// Завершаем список аргументов
	va_end(args);
}
/**
 * @brief Функция вывода текстовой информации в консоль или файл
 *
 * @param format формат строки вывода
 * @param flag   флаг типа логирования
 *
 */
void awh::log::print(wstring_view format, flag_t flag, ...) noexcept {
	// Список аргументов формирования записи
	va_list args;
	// Запускаем инициализацию списка аргументов
	va_start(args, flag);
	// Выполняем вывод записи общим телом
	emit(format, flag, args);
	// Завершаем список аргументов
	va_end(args);
}
/**
 * @brief Функция вывода текстовой информации в консоль или файл
 *
 * @param format формат строки вывода
 * @param flag   флаг типа логирования
 * @param args   список аргументов для замены
 *
 */
void awh::log::print(string_view format, flag_t flag, const vector <string> & args) noexcept {
	// Если формат передан, список аргументов не пустой и уровень логирования соответствует
	if(!format.empty() && !args.empty() && allowed(flag)){
		// Создаём объект полезной нагрузки
		payload_t payload;
		// Устанавливаем флаг логирования
		payload.flag = flag;
		// Устанавливаем данные сообщения
		payload.text = awh::fmk::format(format, args);
		// Фиксируем дату формирования сообщения в момент вызова
		payload.date = state()._chrono.format(state()._format);
		// Выполняем маршрутизацию полезной нагрузки в приёмники
		dispatch(::move(payload));
	}
}
/**
 * @brief Функция вывода текстовой информации в консоль или файл
 *
 * @param format формат строки вывода
 * @param flag   флаг типа логирования
 * @param args   список аргументов для замены
 *
 */
void awh::log::print(wstring_view format, flag_t flag, const vector <wstring> & args) noexcept {
	// Если формат передан, список аргументов не пустой и уровень логирования соответствует
	if(!format.empty() && !args.empty() && allowed(flag)){
		// Создаём объект полезной нагрузки
		payload_t payload;
		// Устанавливаем флаг логирования
		payload.flag = flag;
		// Устанавливаем данные сообщения
		payload.text = awh::fmk::convert(awh::fmk::format(format, args));
		// Фиксируем дату формирования сообщения в момент вызова
		payload.date = state()._chrono.format(state()._format);
		// Выполняем маршрутизацию полезной нагрузки в приёмники
		dispatch(::move(payload));
	}
}
/**
 * @brief Функция установки безопасности работы потоков
 *
 * @param mode флаг режима безопасности потоков
 *
 */
void awh::log::threadSafety(const bool mode) noexcept {
	// Устанавливаем режим безопасности потоков
	state()._mtx.enabled = mode;
}
/**
 * @brief Функция извлечения установленного формата лога
 *
 * @return формат лога для извлечения
 *
 */
const string & awh::log::format() noexcept {
	// Возвращаем установленный формат
	return state()._format;
}
/**
 * @brief Функция установки формата даты и времени для вывода лога
 *
 * @param format формат даты и времени для вывода лога
 *
 */
void awh::log::format(string_view format) noexcept {
	// Устанавливаем формат даты и времени для вывода лога
	state()._format = format;
}
/**
 * @brief Функция получения установленных режимов вывода логов
 *
 * @return список режимов вывода логов
 *
 */
const unordered_set <awh::log::mode_t> & awh::log::mode() noexcept {
	// Возвращаем список режимов вывода логов
	return state()._mode;
}
/**
 * @brief Функция добавления режимов вывода логов
 *
 * @param mode список режимов вывода логов
 *
 */
void awh::log::mode(const unordered_set <mode_t> & mode) noexcept {
	// Выполняем блокировку потока
	const locker_t <> lock(state()._mtx);
	// Выполняем установку списка режимов вывода логов
	state()._mode = mode;
	// Выполняем перестроение набора приёмников
	rebuild(state());
}
/**
 * @brief Функция установки название сервиса для вывода лога
 *
 * @param name название сервиса для вывода лога
 *
 */
void awh::log::name(string_view name) noexcept {
	// Устанавливаем название сервиса для вывода лога
	state()._name = name;
}
/**
 * @brief Функция установки флага асинхронного режима работы
 *
 * @param mode флаг асинхронного режима работы
 *
 */
void awh::log::async(const bool mode) noexcept {
	// Устанавливаем флаг асинхронного режима работы
	state()._async = mode;
}
/**
 * @brief Функция установки максимального размера файла логов
 *
 * @param size максимальный размер файла логов
 *
 */
void awh::log::maxSize(const float size) noexcept {
	// Устанавливаем максимальный размер файла логов
	state()._maxSize = static_cast <size_t> (size);
}
/**
 * @brief Функция установки размера текста для формирования разделителя
 *
 * @param size размер текста для формирования разделителя
 *
 */
void awh::log::sepSize(const size_t size) noexcept {
	// Устанавливаем размер текста для формирования разделителя
	state()._sepSize = size;
}
/**
 * @brief Функция установки уровня логирования
 *
 * @param level уровень логирования для установки
 *
 */
void awh::log::level(const level_t level) noexcept {
	// Выполняем установку уровень логирования
	state()._level = level;
}
/**
 * @brief Функция установки максимального размера очереди асинхронного вывода
 *
 * @param size максимальный размер очереди (0 - без ограничения)
 *
 */
void awh::log::maxQueue(const size_t size) noexcept {
	// Устанавливаем максимальный размер очереди асинхронного вывода
	state()._maxQueue = size;
	// Если дочерний поток уже запущен, применяем ограничение немедленно
	if(static_cast <bool> (state()._screen))
		// Применяем ограничение размера очереди
		state()._screen.capacity(size);
}
/**
 * @brief Функция установки максимального количества хранимых архивов логов
 *
 * @param count максимальное количество архивов (0 - без ограничения)
 *
 */
void awh::log::maxFiles(const size_t count) noexcept {
	// Устанавливаем максимальное количество хранимых архивов логов
	state()._maxFiles = count;
}
/**
 * @brief Функция установки файла для сохранения логов
 *
 * @param filename путь к файлу для сохранения логов
 *
 */
void awh::log::filename(string_view filename) noexcept {
	// Выполняем блокировку потока
	const locker_t <> lock(state()._mtx);
	// Устанавливаем путь к файлу для сохранения логов
	state()._filename = filename;
}
/**
 * @brief Функция установки разделителя сообщений логирования
 *
 * @param sep разделитель для установки
 *
 */
void awh::log::separator(const separator_t sep) noexcept {
	// Устанавливаем разделитель сообщений логирования
	state()._sep = sep;
}
/**
 * @brief Функция установки политики поведения при переполнении очереди асинхронного вывода
 *
 * @param overflow политика поведения при переполнении очереди
 *
 */
void awh::log::overflow(const overflow_t overflow) noexcept {
	// Устанавливаем политику поведения при переполнении очереди
	state()._overflow = overflow;
	// Если дочерний поток уже запущен, применяем политику немедленно
	if(static_cast <bool> (state()._screen))
		// Применяем политику переполнения очереди
		state()._screen.overflow(static_cast <screen_t <payload_t>::overflow_t> (overflow));
}
/**
 * @brief Функция подписки на события логов
 *
 * @param callback функция обратного вызова
 *
 */
void awh::log::subscribe(function <void (const flag_t, string_view)> callback) noexcept {
	// Устанавливаем функцию подписки на получение лога
	state()._callback = ::move(callback);
}
