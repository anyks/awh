/**
 * @file: log.cpp
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

// Подключаем заголовочный файл
#include <sys/log.hpp>

/**
 * rotate Метод выполнения ротации логов
 */
void awh::Log::rotate() const noexcept {
	/**
	 * Выполняем работу для Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Размер файла лога
		uintmax_t size = 0;
		// Получаем адрес файла для записи
		const wstring & filename = this->_fmk->convert(this->_filename);
		// Создаём объект работы с файлом
		HANDLE file = CreateFileW(filename.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		// Если открыть файл открыт нормально
		if(file != INVALID_HANDLE_VALUE){
			// Получаем размер файла лога
			size = static_cast <uintmax_t> (GetFileSize(file, nullptr));
	/**
	 * Выполняем работу для Unix
	 */
	#else
		// Структура проверка статистики
		struct stat info;
		// Если тип определён
		if(::stat(this->_filename.c_str(), &info) == 0){
			// Выводим размер файла
			const uintmax_t size = static_cast <uintmax_t> (info.st_size);
	#endif
			// Если размер файла лога, превышает максимально-установленный
			if(size >= this->_maxSize){
				// Создаем буфер для хранения даты
				char date[80];
				// Заполняем буфер нулями
				::memset(date, 0, sizeof(date));
				// Определяем количество секунд
				const time_t seconds = ::time(nullptr);
				// Получаем структуру локального времени
				struct tm * timeinfo = ::localtime(&seconds);
				// Создаем формат полученного времени
				const string format = "_%m-%d-%Y_%H-%M-%S";
				// Копируем в буфер полученную дату и время
				::strftime(date, sizeof(date), format.c_str(), timeinfo);
				/**
				 * Выполняем работу для Windows
				 */
				#if defined(_WIN32) || defined(_WIN64)
					// Прочитанная строка из файла
					string filedata(size, 0);
					// Открываем файл на сжатие
					gzFile gz = gzopen_w(this->_fmk->convert(this->_fmk->format("%s.gz", this->_filename.c_str())).c_str(), "wb9h");
					// Выполняем чтение из файла в буфер данные
					ReadFile(file, static_cast <LPVOID> (filedata.data()), static_cast <DWORD> (filedata.size()), 0, nullptr);
					// Если данные строки получены
					if(!filedata.empty())
						// Выполняем сжатие файла
						gzwrite(gz, filedata.data(), filedata.size());
					// Закрываем сжатый файл
					gzclose(gz);
				/**
				 * Выполняем работу для Unix
				 */
				#else
					// Открываем файл на чтение
					ifstream file(this->_filename, ios::in);
					// Если файл открыт
					if(file.is_open()){
						// Прочитанная строка из файла
						string filedata = "";
						// Открываем файл на сжатие
						gzFile gz = gzopen(this->_fmk->format("%s.gz", this->_filename.c_str()).c_str(), "wb9h");
						// Считываем до тех пор пока все удачно
						while(file.good()){
							// Считываем строку из файла
							getline(file, filedata);
							// Если данные строки получены
							if(!filedata.empty())
								// Выполняем сжатие файла
								gzwrite(gz, filedata.data(), filedata.size());
						}
						// Закрываем сжатый файл
						gzclose(gz);
						// Закрываем файл
						file.close();
						// Удаляем исходный файл логов
						::unlink(this->_filename.c_str());
					}
				#endif
			}
		}
		/**
		 * Выполняем работу для Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Выполняем закрытие файла
			CloseHandle(file);
			// Если размер файла лога, превышает максимально-установленный
			if(size >= this->_maxFileSize)
				// Удаляем исходный файл логов
				_wunlink(filename.c_str());
		#endif
}
/**
 * receiving Метод получения данных
 * @param payload объект полезной нагрузки
 */
void awh::Log::receiving(const payload_t & payload) const noexcept {
	// Создаем буфер для хранения даты
	char date[80];
	// Флаг конца строки
	bool isEnd = false;
	// Заполняем буфер нулями
	::memset(date, 0, sizeof(date));
	// Определяем количество секунд
	const time_t seconds = ::time(nullptr);
	// Получаем структуру локального времени
	struct tm * timeinfo = ::localtime(&seconds);
	// Копируем в буфер полученную дату и время
	::strftime(date, sizeof(date), this->_format.c_str(), timeinfo);
	// Если размер буфера меньше 3-х байт
	if(payload.data.length() < 3)
		// Проверяем является ли это переводом строки
		isEnd = ((payload.data.compare("\r\n") == 0) || (payload.data.compare("\n") == 0));
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(this->_mtx);
	// Если файл для вывода лога указан
	if((this->_mode.find(mode_t::FILE) != this->_mode.end()) && !this->_filename.empty()){
		/**
		 * Выполняем работу для Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Выполняем открытие файла на запись
			HANDLE file = CreateFileW(this->_fmk->convert(this->_filename).c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
			// Если открыть файл открыт нормально
			if(file != INVALID_HANDLE_VALUE){
				// Определяем тип сообщения
				switch(static_cast <uint8_t> (payload.flag)){
					// Выводим сообщение так-как оно есть
					case static_cast <uint8_t> (flag_t::NONE):
						// Формируем текстовый вид лога
						logData = this->_fmk->format("%s%s", payload.data.c_str(), (!isEnd ? "\r\n\r\n" : ""));
					break;
					// Выводим информационное сообщение
					case static_cast <uint8_t> (flag_t::INFO):
						// Формируем текстовый вид лога
						logData = this->_fmk->format("Info %s %s : %s%s", date, this->_name.c_str(), payload.data.c_str(), (!isEnd ? "\r\n\r\n" : ""));
					break;
					// Выводим сообщение об ошибке
					case static_cast <uint8_t> (flag_t::CRITICAL):
						// Формируем текстовый вид лога
						logData = this->_fmk->format("Error %s %s : %s%s", date, this->_name.c_str(), payload.data.c_str(), (!isEnd ? "\r\n\r\n" : ""));
					break;
					// Выводим сообщение предупреждения
					case static_cast <uint8_t> (flag_t::WARNING):
						// Формируем текстовый вид лога
						logData = this->_fmk->format("Warning %s %s : %s%s", date, this->_name.c_str(), payload.data.c_str(), (!isEnd ? "\r\n\r\n" : ""));
					break;
				}
				// Выполняем запись в файл
				WriteFile(file, static_cast <LPCVOID> (logData.data()), static_cast <DWORD> (logData.size()), 0, nullptr);
			}
			// Выполняем закрытие файла
			CloseHandle(file);
			// Выполняем ротацию логов
			this->rotate();
		/**
		 * Выполняем работу для Unix
		 */
		#else
			// Открываем файл на запись
			ofstream file(this->_filename, ios::out | ios::app);
			// Если файл открыт
			if(file.is_open()){
				// Определяем тип сообщения
				switch(static_cast <uint8_t> (payload.flag)){
					// Выводим сообщение так-как оно есть
					case static_cast <uint8_t> (flag_t::NONE):
						// Формируем текстовый вид лога
						file << this->_fmk->format("%s%s", payload.data.c_str(), (!isEnd ? "\r\n\r\n" : ""));
					break;
					// Выводим информационное сообщение
					case static_cast <uint8_t> (flag_t::INFO):
						// Формируем текстовый вид лога
						file << this->_fmk->format("Info %s %s : %s%s", date, this->_name.c_str(), payload.data.c_str(), (!isEnd ? "\r\n\r\n" : ""));
					break;
					// Выводим сообщение об ошибке
					case static_cast <uint8_t> (flag_t::CRITICAL):
						// Формируем текстовый вид лога
						file << this->_fmk->format("Error %s %s : %s%s", date, this->_name.c_str(), payload.data.c_str(), (!isEnd ? "\r\n\r\n" : ""));
					break;
					// Выводим сообщение предупреждения
					case static_cast <uint8_t> (flag_t::WARNING):
						// Формируем текстовый вид лога
						file << this->_fmk->format("Warning %s %s : %s%s", date, this->_name.c_str(), payload.data.c_str(), (!isEnd ? "\r\n\r\n" : ""));
					break;
				}
				// Закрываем файл
				file.close();
				// Выполняем ротацию логов
				this->rotate();
			}
		#endif
	}
	// Если вывод сообщения в консоль разрешён
	if(this->_mode.find(mode_t::CONSOLE) != this->_mode.end()){
		// Если тип сообщение не является пустым
		if(payload.flag != flag_t::NONE)
			// Выводим обозначение начала вывода лога
			cout << "*************** START ***************" << endl << endl;
		// Определяем тип сообщения
		switch(static_cast <uint8_t> (payload.flag)){
			// Выводим сообщение так-как оно есть
			case static_cast <uint8_t> (flag_t::NONE):
				// Формируем текстовый вид лога
				cout << this->_fmk->format("%s%s", payload.data.c_str(), (!isEnd ? "\r\n\r\n" : ""));
			break;
			// Выводим информационное сообщение
			case static_cast <uint8_t> (flag_t::INFO):
				// Формируем текстовый вид лога
				cout << this->_fmk->format("\x1B[32m\x1B[1mInfo\x1B[0m \x1B[32m%s %s :\x1B[0m %s%s", date, this->_name.c_str(), payload.data.c_str(), (!isEnd ? "\r\n\r\n" : ""));
			break;
			// Выводим сообщение об ошибке
			case static_cast <uint8_t> (flag_t::CRITICAL):
				// Формируем текстовый вид лога
				cout << this->_fmk->format("\x1B[31m\x1B[1mError\x1B[0m \x1B[31m%s %s :\x1B[0m %s%s", date, this->_name.c_str(), payload.data.c_str(), (!isEnd ? "\r\n\r\n" : ""));
			break;
			// Выводим сообщение предупреждения
			case static_cast <uint8_t> (flag_t::WARNING):
				// Формируем текстовый вид лога
				cout << this->_fmk->format("\x1B[33m\x1B[1mWarning\x1B[0m \x1B[33m%s %s :\x1B[0m %s%s", date, this->_name.c_str(), payload.data.c_str(), (!isEnd ? "\r\n\r\n" : ""));
			break;
		}
		// Если тип сообщение не является пустым
		if(payload.flag != flag_t::NONE)
			// Выводим обозначение конца вывода лога
			cout << "---------------- END ----------------" << endl << endl;
	}
	// Если функция подписки на логи установлена, выводим результат
	if((this->_mode.find(mode_t::DEFERRED) != this->_mode.end()) && (this->_fn != nullptr))
		// Выводим сообщение лога всем подписавшимся
		this->_fn(payload.flag, payload.data);
}
/**
 * print Метод вывода текстовой информации в консоль или файл
 * @param format формат строки вывода
 * @param flag   флаг типа логирования
 */
void awh::Log::print(const string & format, flag_t flag, ...) const noexcept {
	// Если формат передан
	if(!format.empty()){
		// Если уровень логирования соответствует
		if((this->_level == level_t::ALL) ||
		  ((this->_level == level_t::INFO) && (flag == flag_t::INFO)) ||
		  ((this->_level == level_t::WARNING) && (flag == flag_t::WARNING)) ||
		  ((this->_level == level_t::CRITICAL) && (flag == flag_t::CRITICAL)) ||
		  ((this->_level == level_t::INFO_WARNING) && ((flag == flag_t::INFO) || (flag == flag_t::WARNING))) ||
		  ((this->_level == level_t::INFO_CRITICAL) && ((flag == flag_t::INFO) || (flag == flag_t::CRITICAL))) ||
		  ((this->_level == level_t::WARNING_CRITICAL) && ((flag == flag_t::WARNING) || (flag == flag_t::CRITICAL)))){
			// Создаем список аргументов
			va_list args;
			// Запускаем инициализацию списка аргументов
			va_start(args, flag);
			// Буфер данных для логирования
			vector <char> buffer(1024);
			// Выполняем перебор всех аргументов
			while(true){
				// Создаем список аргументов
				va_list args2;
				// Копируем список аргументов
				va_copy(args2, args);
				// Выполняем запись в буфер данных
				size_t res = vsnprintf(buffer.data(), buffer.size(), format.c_str(), args2);
				// Если результат получен
				if((res >= 0) && (res < buffer.size())){
					// Завершаем список аргументов
					va_end(args);
					// Завершаем список локальных аргументов
					va_end(args2);
					// Если результат не получен
					if(res == 0)
						// Выполняем сброс результата
						buffer.clear();
					// Выводим результат
					else buffer.assign(buffer.begin(), buffer.begin() + res);
					// Выходим из цикла
					break;
				}
				// Размер буфера данных
				size_t size = 0;
				// Если данные не получены, увеличиваем буфер в два раза
				if(res < 0)
					// Увеличиваем размер буфера в два раза
					size = (buffer.size() * 2);
				// Увеличиваем размер буфера на один байт
				else size = (res + 1);
				// Очищаем буфер данных
				buffer.clear();
				// Выделяем память для буфера
				buffer.resize(size);
				// Завершаем список локальных аргументов
				va_end(args2);
			}
			// Завершаем список аргументов
			va_end(args);
			// Если буфер данных для логирования сформирован
			if(!buffer.empty()){
				// Создаём объект полезной нагрузки
				payload_t payload;
				// Устанавливаем флаг логирования
				payload.flag = flag;
				// Устанавливаем даныне сообщения
				payload.data.assign(buffer.begin(), buffer.end());
				// Если асинхронный режим работы активирован
				if(this->_async){
					// Получаем идентификатор текущего процесса
					const pid_t pid = getpid();
					// Если идентификатор процесса является дочерним
					if(pid != this->_pid){
						// Если процесс ещё не инициализирован и дочерний поток уже создан
						if((this->_child != nullptr) && (this->_initialized.count(pid) < 1)){
							// Выполняем удаление оюъекта дочернего потока
							delete this->_child;
							// Выполняем зануление дочернего потока
							this->_child = nullptr;
							// Выполняем очистку списка инициализированных процессов
							this->_initialized.clear();
						}
					}
					// Если дочерний поток не создан
					if(this->_child == nullptr){
						// Выполняем создание дочернего потока
						this->_child = new child_t <payload_t> ();
						// Выполняем установку функцию обратного вызова
						this->_child->on(std::bind(&log_t::receiving, this, _1));
						// Выполняем инициализацию текущего процесса
						this->_initialized.emplace(pid);
					}
					// Выполняем отправку сообщения дочернему потоку
					this->_child->send(std::move(payload));
				// Выполняем вывод полученного лога
				} else this->receiving(std::move(payload));
			}
		}
	}
}
/**
 * print Метод вывода текстовой информации в консоль или файл
 * @param format формат строки вывода
 * @param flag   флаг типа логирования
 * @param items  список аргументов для замены
 */
void awh::Log::print(const string & format, flag_t flag, const vector <string> & items) const noexcept {
	// Если формат передан
	if(!format.empty() && !items.empty()){
		// Если уровень логирования соответствует
		if((this->_level == level_t::ALL) ||
		  ((this->_level == level_t::INFO) && (flag == flag_t::INFO)) ||
		  ((this->_level == level_t::WARNING) && (flag == flag_t::WARNING)) ||
		  ((this->_level == level_t::CRITICAL) && (flag == flag_t::CRITICAL)) ||
		  ((this->_level == level_t::INFO_WARNING) && ((flag == flag_t::INFO) || (flag == flag_t::WARNING))) ||
		  ((this->_level == level_t::INFO_CRITICAL) && ((flag == flag_t::INFO) || (flag == flag_t::CRITICAL))) ||
		  ((this->_level == level_t::WARNING_CRITICAL) && ((flag == flag_t::WARNING) || (flag == flag_t::CRITICAL)))){
		  	// Создаём объект полезной нагрузки
			payload_t payload;
			// Устанавливаем флаг логирования
			payload.flag = flag;
			// Устанавливаем даныне сообщения
			payload.data = this->_fmk->format(format, items);
			// Если асинхронный режим работы активирован
			if(this->_async){
				// Получаем идентификатор текущего процесса
				const pid_t pid = getpid();
				// Если идентификатор процесса является дочерним
				if(pid != this->_pid){
					// Если процесс ещё не инициализирован и дочерний поток уже создан
					if((this->_child != nullptr) && (this->_initialized.count(pid) < 1)){
						// Выполняем удаление оюъекта дочернего потока
						delete this->_child;
						// Выполняем зануление дочернего потока
						this->_child = nullptr;
						// Выполняем очистку списка инициализированных процессов
						this->_initialized.clear();
					}
				}
				// Если дочерний поток не создан
				if(this->_child == nullptr){
					// Выполняем создание дочернего потока
					this->_child = new child_t <payload_t> ();
					// Выполняем установку функцию обратного вызова
					this->_child->on(std::bind(&log_t::receiving, this, _1));
					// Выполняем инициализацию текущего процесса
					this->_initialized.emplace(pid);
				}
				// Выполняем отправку сообщения дочернему потоку
				this->_child->send(std::move(payload));
			// Выполняем вывод полученного лога
			} else this->receiving(std::move(payload));
		}
	}
}
/**
 * mode Метод добавления режимов вывода логов
 * @param mode список режимов вывода логов
 */
void awh::Log::mode(const set <mode_t> & mode) noexcept {
	// Выполняем установку списка режимов вывода логов
	this->_mode = mode;
}
/**
 * async Метод установки флага асинхронного режима работы
 * @param mode флаг асинхронного режима работы
 */
void awh::Log::async(const bool mode) noexcept {
	// Устанавливаем флаг асинхронного режима работы
	this->_async = mode;
}
/**
 * name Метод установки название сервиса для вывода лога
 * @param name название сервиса для вывода лога
 */
void awh::Log::name(const string & name) noexcept {
	// Устанавливаем название сервиса для вывода лога
	this->_name = name;
}
/**
 * maxSize Метод установки максимального размера файла логов
 * @param size максимальный размер файла логов
 */
void awh::Log::maxSize(const float size) noexcept {
	// Устанавливаем максимальный размер файла логов
	this->_maxSize = size;
}
/**
 * level Метод установки уровня логирования
 * @param level уровень логирования для установки
 */
void awh::Log::level(const level_t level) noexcept {
	// Выполняем установку уровень логирования
	this->_level = level;
}
/**
 * format Метод установки формата даты и времени для вывода лога
 * @param format формат даты и времени для вывода лога
 */
void awh::Log::format(const string & format) noexcept {
	// Устанавливаем формат даты и времени для вывода лога
	this->_format = format;
}
/**
 * filename Метод установки файла для сохранения логов
 * @param filename адрес файла для сохранения логов
 */
void awh::Log::filename(const string & filename) noexcept {
	// Устанавливаем адрес файла для сохранения логов
	this->_filename = filename;
}
/**
 * subscribe Метод подписки на события логов
 * @param callback функция обратного вызова
 */
void awh::Log::subscribe(function <void (const flag_t, const string &)> callback) noexcept {
	// Устанавливаем функцию подписки на получение лога
	this->_fn = callback;
}
/**
 * Log Конструктор
 * @param fmk      объект фреймворка
 * @param filename адрес файла для сохранения логов
 */
awh::Log::Log(const fmk_t * fmk, const string & filename) noexcept :
 _pid(0), _async(false), _maxSize(MAX_SIZE_LOGFILE),
 _level(level_t::ALL), _name(AWH_SHORT_NAME), _format(DATE_FORMAT),
 _filename(filename), _child(nullptr), _fn(nullptr), _fmk(fmk) {
	// Запоминаем идентификатор родительского объекта
	this->_pid = getpid();
	// Выполняем разрешение на вывод всех видов логов
	this->_mode = {mode_t::FILE, mode_t::CONSOLE, mode_t::DEFERRED};
}
/**
 * ~Log Деструктор
 */
awh::Log::~Log() noexcept {
	// Если объект работы с дочерним потоком создан, удаляем
	if(this->_child != nullptr)
		// Удаляем объект работы с дочерним потоком
		delete this->_child;
}
