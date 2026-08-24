/**
 * @file log.cpp
 * @date 2025-10-25
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
 * @copyright Copyright © 2025
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
	#include <sys/win32.hpp>
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
 *       Работа с файлами ведётся там средствами самой системы через sys/win32.hpp
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
	#include <dirent.h>
	#include <syslog.h>
#endif

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/os.hpp>
#include <sys/log.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Используем пространство имён placeholders
 */
using namespace placeholders;

/**
 * @brief Оператор перемещающего присваивания параметров полезной нагрузки
 *
 * @param payload объект полезной нагрузки для перемещения
 * @return        текущий объект полезной нагрузки
 *
 */
awh::Logging::Payload & awh::Logging::Payload::operator = (payload_t && payload) noexcept {
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
awh::Logging::Payload & awh::Logging::Payload::operator = (const payload_t & payload) noexcept {
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
bool awh::Logging::Payload::operator == (const payload_t & payload) noexcept {
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
awh::Logging::Payload::Payload(payload_t && payload) noexcept {
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
awh::Logging::Payload::Payload(const payload_t & payload) noexcept {
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
awh::Logging::Payload::Payload() noexcept : flag(flag_t::NONE), text{""}, date{""} {}

/**
 * @brief Конструктор
 *
 * @param log объект логирования
 *
 */
awh::Logging::Sink::Sink(const Logging * log) noexcept : _log(log) {}

/**
 * @brief Метод записи полезной нагрузки в консоль
 *
 * @param payload объект полезной нагрузки
 *
 */
void awh::Logging::ConsoleSink::write(const payload_t & payload) const noexcept {
	// Получаем указатель на владеющий объект логирования
	const Logging * self = this->_log;
	// Если тип сообщения не является пустым
	if(payload.flag != flag_t::NONE){
		/**
		 * Определяем флаг формирования разделителя
		 */
		switch(static_cast <uint8_t> (self->_sep)){
			// Если разделитель нужно отобразить с учётом размера текста
			case static_cast <uint8_t> (separator_t::SMART): {
				// Если размер текста соответствует размеру лога
				if(payload.text.length() >= self->_sepSize)
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
	cout << self->compose(payload, true);
	// Если тип сообщения не является пустым
	if(payload.flag != flag_t::NONE){
		/**
		 * Определяем флаг формирования разделителя
		 */
		switch(static_cast <uint8_t> (self->_sep)){
			// Если разделитель нужно отобразить с учётом размера текста
			case static_cast <uint8_t> (separator_t::SMART): {
				// Если размер текста соответствует размеру лога
				if(payload.text.length() >= self->_sepSize)
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
	self->_counter.fetch_add(1, std::memory_order_relaxed);
	// Если мы прошли полный круг счётчика
	if(self->_counter.load(std::memory_order_acquire) == 0)
		// Выполняем сброс накопленных логов
		cout << flush;
}
/**
 * @brief Конструктор
 *
 * @param log объект логирования
 *
 */
awh::Logging::ConsoleSink::ConsoleSink(const Logging * log) noexcept : Sink(log) {}

/**
 * @brief Метод (пере)открытия постоянного дескриптора записи
 *
 */
void awh::Logging::FileSink::reopen() const noexcept {
	// Получаем указатель на владеющий объект логирования
	const Logging * self = this->_log;
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
		HANDLE handle = ::CreateFileW(self->_fmk->convert(self->_filename).c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
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
		this->_fd = ::open(self->_filename.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
		// Структура для получения статистики файла
		struct stat info;
		// Получаем текущий размер файла лога единоразово при открытии
		this->_size = (((this->_fd != -1) && (::fstat(static_cast <int32_t> (this->_fd), &info) == 0)) ? static_cast <uintmax_t> (info.st_size) : 0);
	#endif
	// Запоминаем идентификатор текущего процесса
	this->_pid = ::getpid();
	// Запоминаем путь открытого файла
	this->_opened = self->_filename;
}
/**
 * @brief Метод выполнения ротации файла лога
 *
 */
void awh::Logging::FileSink::rotate() const noexcept {
	// Получаем указатель на владеющий объект логирования
	const Logging * self = this->_log;
	// Формируем уникальное имя архива (без коллизий в пределах одной секунды)
	const string archive = this->nextArchive();
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		// Получаем путь к исходному файлу лога
		const wstring & filename = self->_fmk->convert(self->_filename);
		// Открываем исходный файл лога на чтение
		HANDLE file = ::CreateFileW(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		// Если файл открыт нормально
		if(file != INVALID_HANDLE_VALUE){
			// Флаг успешности сжатия
			bool success = false;
			// Открываем файл архива на сжатие
			gzFile gz = ::gzopen_w(self->_fmk->convert(archive).c_str(), "wb9h");
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
				::fprintf(stderr, "ERROR! Logging rotate: %s\n\n", self->_fmk->convert(message).c_str());
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
			ifstream file(self->_filename, ios::in | ios::binary);
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
			::unlink(self->_filename.c_str());
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
void awh::Logging::FileSink::retention() const noexcept {
	// Получаем указатель на владеющий объект логирования
	const Logging * self = this->_log;
	// Если ограничение на количество архивов не установлено, выходим
	if(self->_maxFiles == 0)
		// Выходим из метода
		return;
	// Получаем компоненты адреса файла лога
	const auto & cmp = self->components(self->_filename);
	// Определяем каталог хранения архивов
	const string dir = (cmp.first.empty() ? string{"./"} : cmp.first);
	// Базовое имя файла лога без расширения
	const string & base = cmp.second;
	// Если базовое имя файла не определено, выходим
	if(base.empty())
		// Выходим из метода
		return;
	// Список найденных архивов (путь, время модификации)
	vector <std::pair <string, uintmax_t>> archives;
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		// Формируем маску поиска архивов
		const wstring & mask = self->_fmk->convert(self->_fmk->format("%s%s*.gz", dir.c_str(), base.c_str()));
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
				const string name = self->_fmk->convert(data.cFileName);
				// Пропускаем файлы, не являющиеся архивами лога (имя должно иметь вид <base>_...gz)
				if((name.length() <= (base.length() + 3)) || (name.compare(0, base.length(), base) != 0) ||
				   (name[base.length()] != '_') || (name.compare(name.length() - 3, 3, ".gz") != 0))
					// Переходим к следующему файлу
					continue;
				// Формируем полный путь к архиву
				const string & full = self->_fmk->format("%s%s", dir.c_str(), name.c_str());
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
		DIR * directory = ::opendir(dir.c_str());
		// Если каталог открыт
		if(directory != nullptr){
			// Объект записи каталога
			struct dirent * entry = nullptr;
			/**
			 * Перебираем все записи каталога
			 */
			while((entry = ::readdir(directory)) != nullptr){
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
			::closedir(directory);
		}
	#endif
	// Если количество архивов превышает установленный лимит
	if(archives.size() > self->_maxFiles){
		// Выполняем сортировку архивов по времени модификации (от старых к новым)
		std::sort(archives.begin(), archives.end(), [](const auto & a, const auto & b) noexcept -> bool {
			// Сравниваем время модификации
			return (a.second < b.second);
		});
		// Вычисляем количество архивов для удаления
		const size_t count = (archives.size() - self->_maxFiles);
		/**
		 * Удаляем самые старые архивы сверх установленного лимита
		 */
		for(size_t i = 0; i < count; i++){
			/**
			 * Для операционной системы MS Windows
			 */
			#if _WIN32 || _WIN64
				// Удаляем устаревший архив
				::_wunlink(self->_fmk->convert(archives.at(i).first).c_str());
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
string awh::Logging::FileSink::nextArchive() const noexcept {
	// Получаем указатель на владеющий объект логирования
	const Logging * self = this->_log;
	// Получаем компоненты адреса файла лога
	const auto & cmp = self->components(self->_filename);
	// Выполняем извлечение даты для имени архива
	const string & date = self->_chrono.format("_%m-%d-%Y_%H-%M-%S");
	// Лямбда проверки существования файла
	auto exists = [self](const string & path) noexcept -> bool {
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Проверяем существование файла по его атрибутам
			return (::GetFileAttributesW(self->_fmk->convert(path).c_str()) != INVALID_FILE_ATTRIBUTES);
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#else
			// Подавляем предупреждение о неиспользуемом параметре захвата
			(void) self;
			// Структура для получения статистики файла
			struct stat info{};
			// Проверяем существование файла
			return (::stat(path.c_str(), &info) == 0);
		#endif
	};
	// Формируем базовое имя архива
	string path = self->_fmk->format("%s%s%s.gz", cmp.first.c_str(), cmp.second.c_str(), date.c_str());
	// Если архив с таким именем уже существует (несколько ротаций в течение одной секунды)
	if(exists(path)){
		/**
		 * Подбираем свободное имя с порядковым индексом
		 */
		for(uint32_t i = 1; i > 0; i++){
			// Формируем имя архива с порядковым индексом
			const string candidate = self->_fmk->format("%s%s%s_%u.gz", cmp.first.c_str(), cmp.second.c_str(), date.c_str(), i);
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
void awh::Logging::FileSink::write(const payload_t & payload) const noexcept {
	// Получаем указатель на владеющий объект логирования
	const Logging * self = this->_log;
	// Если файл для вывода лога не указан, выходим
	if(self->_filename.empty())
		// Выходим из метода
		return;
	// Формируем запись с очищенным от управляющих символов текстом
	payload_t record(payload);
	// Выполняем очистку текста от символов форматирования
	self->cleaner(record.text);
	// Формируем строку лога без символов цветового форматирования
	const string line = self->compose(record, false);
	// Получаем идентификатор текущего процесса
	const pid_t pid = ::getpid();
	/**
	 * (Пере)открываем дескриптор если: он ещё не открыт, изменился путь файла,
	 * или мы оказались в дочернем процессе после fork (унаследованный дескриптор).
	 */
	if((this->_fd == -1) || (this->_opened != self->_filename) || (this->_pid != pid))
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
	if(this->_size >= self->_maxSize){
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
 * @param log объект логирования
 *
 */
awh::Logging::FileSink::FileSink(const Logging * log) noexcept :
 Sink(log), _pid(0), _fd(-1), _opened{""}, _size(0) {}
/**
 * @brief Деструктор
 *
 */
awh::Logging::FileSink::~FileSink() noexcept {
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
void awh::Logging::SyslogSink::write(const payload_t & payload) const noexcept {
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#if !_WIN32 && !_WIN64
		// Получаем указатель на владеющий объект логирования
		const Logging * self = this->_log;
		// Открываем SysLog для нашего приложения
		::openlog(!self->_name.empty() ? self->_name.c_str() : AWH_SHORT_NAME, LOG_PID, LOG_USER);
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
 * @brief Конструктор
 *
 * @param log объект логирования
 *
 */
awh::Logging::SyslogSink::SyslogSink(const Logging * log) noexcept : Sink(log) {}

/**
 * @brief Метод передачи полезной нагрузки в функцию обратного вызова
 *
 * @param payload объект полезной нагрузки
 *
 */
void awh::Logging::CallbackSink::write(const payload_t & payload) const noexcept {
	// Получаем указатель на владеющий объект логирования
	const Logging * self = this->_log;
	// Если функция подписки на логи установлена, рассылаем сообщение подписчику
	if(self->_callback != nullptr)
		// Рассылаем сообщение лога подписчику
		self->_callback(payload.flag, payload.text);
}
/**
 * @brief Конструктор
 *
 * @param log объект логирования
 *
 */
awh::Logging::CallbackSink::CallbackSink(const Logging * log) noexcept : Sink(log) {}

/**
 * @brief Метод перестроения набора приёмников по текущему списку режимов
 *
 */
void awh::Logging::rebuild() noexcept {
	// Очищаем текущий набор приёмников
	this->_sinks.clear();
	// Если разрешён вывод логов в функцию обратного вызова
	if(this->_mode.find(mode_t::DEFERRED) != this->_mode.end())
		// Добавляем приёмник функции обратного вызова
		this->_sinks.push_back(std::make_unique <CallbackSink> (this));
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#if !_WIN32 && !_WIN64
		// Если разрешена отправка логов в SysLog
		if(this->_mode.find(mode_t::SYSLOG) != this->_mode.end())
			// Добавляем приёмник SysLog
			this->_sinks.push_back(std::make_unique <SyslogSink> (this));
	#endif
	// Если разрешён вывод логов в консоль
	if(this->_mode.find(mode_t::CONSOLE) != this->_mode.end())
		// Добавляем приёмник консоли
		this->_sinks.push_back(std::make_unique <ConsoleSink> (this));
	// Если разрешён вывод логов в файл
	if(this->_mode.find(mode_t::FILE) != this->_mode.end())
		// Добавляем приёмник файла
		this->_sinks.push_back(std::make_unique <FileSink> (this));
}

/**
 * @brief Метод проверки разрешён ли вывод лога для указанного флага
 *
 * @param flag флаг типа логирования
 * @return     результат проверки соответствия уровню логирования
 *
 */
bool awh::Logging::allowed(const flag_t flag) const noexcept {
	// Выполняем проверку соответствия флага установленному уровню логирования
	return (
		(this->_level == level_t::ALL) ||
		((this->_level == level_t::INFO) && (flag == flag_t::INFO)) ||
		((this->_level == level_t::WARNING) && (flag == flag_t::WARNING)) ||
		((this->_level == level_t::CRITICAL) && (flag == flag_t::CRITICAL)) ||
		((this->_level == level_t::INFO_WARNING) && ((flag == flag_t::INFO) || (flag == flag_t::WARNING))) ||
		((this->_level == level_t::INFO_CRITICAL) && ((flag == flag_t::INFO) || (flag == flag_t::CRITICAL))) ||
		((this->_level == level_t::WARNING_CRITICAL) && ((flag == flag_t::WARNING) || (flag == flag_t::CRITICAL)))
	);
}
/**
 * @brief Метод очистки строки от символов форматирования
 *
 * @param text текст для очистки
 * @return     очищенный текст
 *
 */
string & awh::Logging::cleaner(string & text) const noexcept {
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
			} else if(!this->_fmk->is(letter, fmk_t::check_t::NUMBER) && (letter != ';')) {
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
 * @brief Метод маршрутизации полезной нагрузки в приёмники (синхронно или асинхронно)
 *
 * @param payload объект полезной нагрузки
 *
 */
void awh::Logging::dispatch(payload_t && payload) const noexcept {
	// Если асинхронный режим работы не активирован, выводим сообщение синхронно
	if(!this->_async){
		// Выполняем синхронный вывод полученного лога
		this->receiving(payload);
		// Выходим из метода
		return;
	}
	// Получаем идентификатор текущего процесса
	const pid_t pid = ::getpid();
	/**
	 * Быстрая проверка без блокировки: в типовом случае (тот же процесс и живой поток)
	 * управление жизненным циклом скрина не требуется.
	 */
	if((pid != this->_pid.load(std::memory_order_acquire)) || !static_cast <bool> (this->_screen)){
		// Выполняем блокировку потока на время управления жизненным циклом скрина
		const locker_t <> lock(this->_mtx);
		// Если идентификатор процесса сменился (например, после fork)
		if(pid != this->_pid.load(std::memory_order_acquire)){
			// Запоминаем идентификатор текущего процесса
			this->_pid.store(pid, std::memory_order_release);
			/**
			 * Останавливаем унаследованный скрин. Так-как Screen самостоятельно
			 * обнаруживает смену процесса, join() унаследованного потока не выполняется.
			 */
			this->_screen.stop();
		}
		// Если дочерний поток не создан
		if(!static_cast <bool> (this->_screen)){
			// Выполняем установку функции обратного вызова
			this->_screen = static_cast <function <void (const payload_t &)>> (std::bind(&log_t::receiving, this, _1));
			// Применяем ограничение размера очереди асинхронного вывода
			this->_screen.capacity(this->_maxQueue);
			// Применяем политику поведения при переполнении очереди
			this->_screen.overflow(static_cast <screen_t <payload_t>::overflow_t> (this->_overflow));
			// Запускаем работу скрина
			this->_screen.start();
		}
	}
	// Выполняем отправку сообщения дочернему потоку
	this->_screen = ::move(payload);
}
/**
 * @brief Метод получения данных
 *
 * @param payload объект полезной нагрузки
 *
 */
void awh::Logging::receiving(const payload_t & payload) const noexcept {
	// Выполняем блокировку потока
	const locker_t <> lock(this->_mtx);
	/**
	 * Выполняем перебор всех установленных приёмников вывода логов
	 */
	for(const auto & sink : this->_sinks){
		// Если приёмник создан, выполняем запись полезной нагрузки
		if(sink != nullptr)
			// Записываем полезную нагрузку в приёмник
			sink->write(payload);
	}
}
/**
 * @brief Метод извлечения компонента адреса файла
 *
 * @param filename адрес где находится файл
 * @return         параметры компонента (адрес, название файла без расширения)
 *
 */
std::pair <string, string> awh::Logging::components(string_view filename) const noexcept {
	// Переменная результата
	std::pair <string, string> result;
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
			result.second.append(this->_name.empty() ? AWH_SHORT_NAME : this->_name);
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод формирования итоговой строки лога
 *
 * @param payload объект полезной нагрузки
 * @param colored нужно ли добавлять символы цветового форматирования
 * @return        сформированная строка лога
 *
 */
string awh::Logging::compose(const payload_t & payload, const bool colored) const noexcept {
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
			return this->_fmk->format("%s%s", payload.text.c_str(), tail);
		// Печатаем информационное сообщение
		case static_cast <uint8_t> (flag_t::INFO):
			// Формируем текстовый вид лога
			return (
				colored ?
				this->_fmk->format("\x1B[32m\x1B[1mInfo\x1B[0m \x1B[32m%s %s :\x1B[0m %s%s", payload.date.c_str(), this->_name.c_str(), payload.text.c_str(), tail) :
				this->_fmk->format("Info %s %s : %s%s", payload.date.c_str(), this->_name.c_str(), payload.text.c_str(), tail)
			);
		// Записываем ошибку в лог
		case static_cast <uint8_t> (flag_t::CRITICAL):
			// Формируем текстовый вид лога
			return (
				colored ?
				this->_fmk->format("\x1B[31m\x1B[1mError\x1B[0m \x1B[31m%s %s :\x1B[0m %s%s", payload.date.c_str(), this->_name.c_str(), payload.text.c_str(), tail) :
				this->_fmk->format("Error %s %s : %s%s", payload.date.c_str(), this->_name.c_str(), payload.text.c_str(), tail)
			);
		// Записываем в лог сообщение предупреждения
		case static_cast <uint8_t> (flag_t::WARNING):
			// Формируем текстовый вид лога
			return (
				colored ?
				this->_fmk->format("\x1B[33m\x1B[1mWarning\x1B[0m \x1B[33m%s %s :\x1B[0m %s%s", payload.date.c_str(), this->_name.c_str(), payload.text.c_str(), tail) :
				this->_fmk->format("Warning %s %s : %s%s", payload.date.c_str(), this->_name.c_str(), payload.text.c_str(), tail)
			);
	}
	// Возвращаем пустой результат
	return "";
}
/**
 * @brief Метод вывода текстовой информации в консоль или файл
 *
 * @param format формат строки вывода
 * @param flag   флаг типа логирования
 *
 */
void awh::Logging::print(string_view format, flag_t flag, ...) const noexcept {
	// Если формат передан и уровень логирования соответствует
	if(!format.empty() && this->allowed(flag)){
		// Создаём текст для логирования
		const string text{format};
		// Буфер данных для логирования
		vector <char> buffer(1024);
		// Результирующая строка логирования
		string result;
		// Создаём список аргументов
		va_list args;
		// Запускаем инициализацию списка аргументов
		va_start(args, flag);
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
		// Завершаем список аргументов
		va_end(args);
		// Если результирующая строка сформирована
		if(!result.empty()){
			// Создаём объект полезной нагрузки
			payload_t payload;
			// Устанавливаем флаг логирования
			payload.flag = flag;
			// Устанавливаем данные сообщения
			payload.text = ::move(result);
			// Фиксируем дату формирования сообщения в момент вызова
			payload.date = this->_chrono.format(this->_format);
			// Выполняем маршрутизацию полезной нагрузки в приёмники
			this->dispatch(::move(payload));
		}
	}
}
/**
 * @brief Метод вывода текстовой информации в консоль или файл
 *
 * @param format формат строки вывода
 * @param flag   флаг типа логирования
 *
 */
void awh::Logging::print(wstring_view format, flag_t flag, ...) const noexcept {
	// Если формат передан и уровень логирования соответствует
	if(!format.empty() && this->allowed(flag)){
		// Создаём текст для логирования
		const wstring text{format};
		// Буфер данных для логирования
		vector <wchar_t> buffer(1024);
		// Результирующая строка логирования
		wstring result;
		// Создаём список аргументов
		va_list args;
		// Запускаем инициализацию списка аргументов
		va_start(args, flag);
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
		// Завершаем список аргументов
		va_end(args);
		// Если результирующая строка сформирована
		if(!result.empty()){
			// Создаём объект полезной нагрузки
			payload_t payload;
			// Устанавливаем флаг логирования
			payload.flag = flag;
			// Устанавливаем данные сообщения
			payload.text = this->_fmk->convert(result);
			// Фиксируем дату формирования сообщения в момент вызова
			payload.date = this->_chrono.format(this->_format);
			// Выполняем маршрутизацию полезной нагрузки в приёмники
			this->dispatch(::move(payload));
		}
	}
}
/**
 * @brief Метод вывода текстовой информации в консоль или файл
 *
 * @param format формат строки вывода
 * @param flag   флаг типа логирования
 * @param args   список аргументов для замены
 *
 */
void awh::Logging::print(string_view format, flag_t flag, const vector <string> & args) const noexcept {
	// Если формат передан, список аргументов не пустой и уровень логирования соответствует
	if(!format.empty() && !args.empty() && this->allowed(flag)){
		// Создаём объект полезной нагрузки
		payload_t payload;
		// Устанавливаем флаг логирования
		payload.flag = flag;
		// Устанавливаем данные сообщения
		payload.text = this->_fmk->format(format, args);
		// Фиксируем дату формирования сообщения в момент вызова
		payload.date = this->_chrono.format(this->_format);
		// Выполняем маршрутизацию полезной нагрузки в приёмники
		this->dispatch(::move(payload));
	}
}
/**
 * @brief Метод вывода текстовой информации в консоль или файл
 *
 * @param format формат строки вывода
 * @param flag   флаг типа логирования
 * @param args   список аргументов для замены
 *
 */
void awh::Logging::print(wstring_view format, flag_t flag, const vector <wstring> & args) const noexcept {
	// Если формат передан, список аргументов не пустой и уровень логирования соответствует
	if(!format.empty() && !args.empty() && this->allowed(flag)){
		// Создаём объект полезной нагрузки
		payload_t payload;
		// Устанавливаем флаг логирования
		payload.flag = flag;
		// Устанавливаем данные сообщения
		payload.text = this->_fmk->convert(this->_fmk->format(format, args));
		// Фиксируем дату формирования сообщения в момент вызова
		payload.date = this->_chrono.format(this->_format);
		// Выполняем маршрутизацию полезной нагрузки в приёмники
		this->dispatch(::move(payload));
	}
}
/**
 * @brief Метод установки безопасности работы потоков
 *
 * @param mode флаг режима безопасности потоков
 *
 */
void awh::Logging::threadSafety(const bool mode) noexcept {
	// Устанавливаем режим безопасности потоков
	this->_mtx.enabled = mode;
}
/**
 * @brief Метод извлечения установленного формата лога
 *
 * @return формат лога для извлечения
 *
 */
const string & awh::Logging::format() const noexcept {
	// Возвращаем установленный формат
	return this->_format;
}
/**
 * @brief Метод установки формата даты и времени для вывода лога
 *
 * @param format формат даты и времени для вывода лога
 *
 */
void awh::Logging::format(string_view format) noexcept {
	// Устанавливаем формат даты и времени для вывода лога
	this->_format = format;
}
/**
 * @brief Метод получения установленных режимов вывода логов
 *
 * @return список режимов вывода логов
 *
 */
const unordered_set <awh::Logging::mode_t> & awh::Logging::mode() const noexcept {
	// Возвращаем список режимов вывода логов
	return this->_mode;
}
/**
 * @brief Метод добавления режимов вывода логов
 *
 * @param mode список режимов вывода логов
 *
 */
void awh::Logging::mode(const unordered_set <mode_t> & mode) noexcept {
	// Выполняем блокировку потока
	const locker_t <> lock(this->_mtx);
	// Выполняем установку списка режимов вывода логов
	this->_mode = mode;
	// Выполняем перестроение набора приёмников
	this->rebuild();
}
/**
 * @brief Метод установки название сервиса для вывода лога
 *
 * @param name название сервиса для вывода лога
 *
 */
void awh::Logging::name(string_view name) noexcept {
	// Устанавливаем название сервиса для вывода лога
	this->_name = name;
}
/**
 * @brief Метод установки флага асинхронного режима работы
 *
 * @param mode флаг асинхронного режима работы
 *
 */
void awh::Logging::async(const bool mode) noexcept {
	// Устанавливаем флаг асинхронного режима работы
	this->_async = mode;
}
/**
 * @brief Метод установки максимального размера файла логов
 *
 * @param size максимальный размер файла логов
 *
 */
void awh::Logging::maxSize(const float size) noexcept {
	// Устанавливаем максимальный размер файла логов
	this->_maxSize = size;
}
/**
 * @brief Метод установки размера текста для формирования разделителя
 *
 * @param size размер текста для формирования разделителя
 *
 */
void awh::Logging::sepSize(const size_t size) noexcept {
	// Устанавливаем размер текста для формирования разделителя
	this->_sepSize = size;
}
/**
 * @brief Метод установки уровня логирования
 *
 * @param level уровень логирования для установки
 *
 */
void awh::Logging::level(const level_t level) noexcept {
	// Выполняем установку уровень логирования
	this->_level = level;
}
/**
 * @brief Метод установки максимального размера очереди асинхронного вывода
 *
 * @param size максимальный размер очереди (0 - без ограничения)
 *
 */
void awh::Logging::maxQueue(const size_t size) noexcept {
	// Устанавливаем максимальный размер очереди асинхронного вывода
	this->_maxQueue = size;
	// Если дочерний поток уже запущен, применяем ограничение немедленно
	if(static_cast <bool> (this->_screen))
		// Применяем ограничение размера очереди
		this->_screen.capacity(size);
}
/**
 * @brief Метод установки максимального количества хранимых архивов логов
 *
 * @param count максимальное количество архивов (0 - без ограничения)
 *
 */
void awh::Logging::maxFiles(const size_t count) noexcept {
	// Устанавливаем максимальное количество хранимых архивов логов
	this->_maxFiles = count;
}
/**
 * @brief Метод установки файла для сохранения логов
 *
 * @param filename путь к файлу для сохранения логов
 *
 */
void awh::Logging::filename(string_view filename) noexcept {
	// Выполняем блокировку потока
	const locker_t <> lock(this->_mtx);
	// Устанавливаем путь к файлу для сохранения логов
	this->_filename = filename;
}
/**
 * @brief Метод установки разделителя сообщений логирования
 *
 * @param sep разделитель для установки
 *
 */
void awh::Logging::separator(const separator_t sep) noexcept {
	// Устанавливаем разделитель сообщений логирования
	this->_sep = sep;
}
/**
 * @brief Метод установки политики поведения при переполнении очереди асинхронного вывода
 *
 * @param overflow политика поведения при переполнении очереди
 *
 */
void awh::Logging::overflow(const overflow_t overflow) noexcept {
	// Устанавливаем политику поведения при переполнении очереди
	this->_overflow = overflow;
	// Если дочерний поток уже запущен, применяем политику немедленно
	if(static_cast <bool> (this->_screen))
		// Применяем политику переполнения очереди
		this->_screen.overflow(static_cast <screen_t <payload_t>::overflow_t> (overflow));
}
/**
 * @brief Метод подписки на события логов
 *
 * @param callback функция обратного вызова
 *
 */
void awh::Logging::subscribe(function <void (const flag_t, string_view)> callback) noexcept {
	// Устанавливаем функцию подписки на получение лога
	this->_callback = ::move(callback);
}
/**
 * @brief Конструктор
 *
 * @param fmk      объект фреймворка
 * @param filename путь к файлу для сохранения логов
 *
 */
awh::Logging::Logging(const fmk_t * fmk, string_view filename) noexcept :
 _async(false), _level(level_t::ALL), _sep(separator_t::ALWAYS),
 _name{AWH_SHORT_NAME}, _format{DATE_FORMAT}, _filename{filename},
 _maxSize(MAX_SIZE_LOGFILE), _sepSize(0x400), _maxQueue(0), _maxFiles(0),
 _chrono(fmk, this), _overflow(overflow_t::DROP_OLD), _pid(0),
 _counter{1}, _screen(Screen <payload_t>::health_t::DEAD), _callback(nullptr), _fmk(fmk) {
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
	this->rebuild();
}
/**
 * @brief Деструктор
 *
 */
awh::Logging::~Logging() noexcept {
	// Если объект работы с дочерним потоком создан, удаляем
	if(static_cast <bool> (this->_screen))
		// Останавливаем работу скрина
		this->_screen.stop();
}
