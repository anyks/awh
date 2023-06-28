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
	// Открываем файл на чтение
	ifstream file(this->_filename, ios::in);
	// Если файл открыт
	if(file.is_open()){
		// Перемещаем указатель в конец файла
		file.seekg(0, file.end);
		// Определяем размер файла
		const size_t size = file.tellg();
		// Возвращаем указатель обратно
		file.seekg(0, file.beg);
		// Закрываем файл
		file.close();
		// Если размер файла лога, превышает максимально-установленный
		if(size >= this->_maxSize){
			// Создаем буфер для хранения даты
			char date[80];
			// Заполняем буфер нулями
			memset(date, 0, sizeof(date));
			// Определяем количество секунд
			const time_t seconds = time(nullptr);
			// Получаем структуру локального времени
			struct tm * timeinfo = localtime(&seconds);
			// Создаем формат полученного времени
			const string format = "_%m-%d-%Y_%H-%M-%S";
			// Копируем в буфер полученную дату и время
			strftime(date, sizeof(date), format.c_str(), timeinfo);
			// Открываем файл на чтение
			ifstream file(this->_filename, ios::in);
			// Если файл открыт
			if(file.is_open()){
				// Прочитанная строка из файла
				string filedata = "";
				// Открываем файл на сжатие
				gzFile gz = gzopen(this->_fmk->format("%s%s.gz", this->_filename.c_str(), date).c_str(), "wb9h");
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
		}
	}
}
/**
 * _print1 Метод вывода текстовой информации в консоль или файл
 * @param format формат строки вывода
 * @param flag   флаг типа логирования
 * @param buffer буфер данных для логирования
 */
void awh::Log::_print1(const string format, flag_t flag, const vector <char> buffer) const noexcept {
	// Если формат строки вывода и буфер данных для логирования переданы
	if(!format.empty() && !buffer.empty()){
		// Создаем буфер для хранения даты
		char date[80];
		// Флаг конца строки
		bool isEnd = false;
		// Данные для записи
		string logData = "";
		// Заполняем буфер нулями
		memset(date, 0, sizeof(date));
		// Определяем количество секунд
		const time_t seconds = time(nullptr);
		// Получаем структуру локального времени
		struct tm * timeinfo = localtime(&seconds);
		// Копируем в буфер полученную дату и время
		strftime(date, sizeof(date), this->_format.c_str(), timeinfo);
		// Если размер буфера меньше 3-х байт
		if(buffer.size() < 3){
			// Создаём строку для проверки
			const string str = buffer.data();
			// Проверяем является ли это переводом строки
			isEnd = ((str.compare("\r\n") == 0) || (str.compare("\n") == 0));
		}
		// Выполняем блокировку потока
		const lock_guard <mutex> lock(this->_mtx);
		// Если файл для вывода лога указан
		if(this->_fileMode && !this->_filename.empty()){
			// Открываем файл на запись
			ofstream file(this->_filename, ios::out | ios::app);
			// Если файл открыт
			if(file.is_open()){
				// Определяем тип сообщения
				switch(static_cast <uint8_t> (flag)){
					// Выводим сообщение так-как оно есть
					case static_cast <uint8_t> (flag_t::NONE):
						// Формируем текстовый вид лога
						logData = this->_fmk->format("%s%s", buffer.data(), (!isEnd ? "\r\n\r\n" : ""));
					break;
					// Выводим информационное сообщение
					case static_cast <uint8_t> (flag_t::INFO):
						// Формируем текстовый вид лога
						logData = this->_fmk->format("Info %s %s : %s%s", date, this->_name.c_str(), buffer.data(), (!isEnd ? "\r\n\r\n" : ""));
					break;
					// Выводим сообщение об ошибке
					case static_cast <uint8_t> (flag_t::CRITICAL):
						// Формируем текстовый вид лога
						logData = this->_fmk->format("Error %s %s : %s%s", date, this->_name.c_str(), buffer.data(), (!isEnd ? "\r\n\r\n" : ""));
					break;
					// Выводим сообщение предупреждения
					case static_cast <uint8_t> (flag_t::WARNING):
						// Формируем текстовый вид лога
						logData = this->_fmk->format("Warning %s %s : %s%s", date, this->_name.c_str(), buffer.data(), (!isEnd ? "\r\n\r\n" : ""));
					break;
				}
				// Выполняем запись в файл
				file.write(logData.data(), logData.size());
				// Закрываем файл
				file.close();
				// Выполняем ротацию логов
				this->rotate();
			}
		}
		// Определяем тип сообщения
		switch(static_cast <uint8_t> (flag)){
			// Выводим сообщение так-как оно есть
			case static_cast <uint8_t> (flag_t::NONE):
				// Формируем текстовый вид лога
				logData = this->_fmk->format("%s%s", buffer.data(), (!isEnd ? "\r\n\r\n" : ""));
			break;
			// Выводим информационное сообщение
			case static_cast <uint8_t> (flag_t::INFO):
				// Формируем текстовый вид лога
				logData = this->_fmk->format("\x1B[32m\x1B[1mInfo\x1B[0m \x1B[32m%s %s :\x1B[0m %s%s", date, this->_name.c_str(), buffer.data(), (!isEnd ? "\r\n\r\n" : ""));
			break;
			// Выводим сообщение об ошибке
			case static_cast <uint8_t> (flag_t::CRITICAL):
				// Формируем текстовый вид лога
				logData = this->_fmk->format("\x1B[31m\x1B[1mError\x1B[0m \x1B[31m%s %s :\x1B[0m %s%s", date, this->_name.c_str(), buffer.data(), (!isEnd ? "\r\n\r\n" : ""));
			break;
			// Выводим сообщение предупреждения
			case static_cast <uint8_t> (flag_t::WARNING):
				// Формируем текстовый вид лога
				logData = this->_fmk->format("\x1B[33m\x1B[1mWarning\x1B[0m \x1B[33m%s %s :\x1B[0m %s%s", date, this->_name.c_str(), buffer.data(), (!isEnd ? "\r\n\r\n" : ""));
			break;
		}
		// Если вывод сообщения в консоль разрешён
		if(this->_consoleMode){
			// Если тип сообщение не является пустым
			if(flag != flag_t::NONE) cout << "*************** START ***************" << endl << endl;
			// Выводим сообщение в консоль
			cout << logData;
			// Если тип сообщение не является пустым
			if(flag != flag_t::NONE) cout << "---------------- END ----------------" << endl << endl;
		}
		// Если функция подписки на логи установлена, выводим результат
		if(this->_fn != nullptr)
			// Выводим сообщение лога всем подписавшимся
			this->_fn(flag, string(buffer.begin(), buffer.end()));
	}
}
/**
 * _print2 Метод вывода текстовой информации в консоль или файл
 * @param format формат строки вывода
 * @param flag   флаг типа логирования
 * @param items  список аргументов для замены
 */
void awh::Log::_print2(const string format, flag_t flag, const vector <string> items) const noexcept {
	// Если формат передан
	if(!format.empty() && !items.empty()){
		// Создаем буфер для хранения даты
		char date[80];
		// Флаг конца строки
		bool isEnd = false;
		// Данные для записи
		string logData = "";
		// Заполняем буфер нулями
		memset(date, 0, sizeof(date));
		// Определяем количество секунд
		const time_t seconds = time(nullptr);
		// Получаем структуру локального времени
		struct tm * timeinfo = localtime(&seconds);
		// Копируем в буфер полученную дату и время
		strftime(date, sizeof(date), this->_format.c_str(), timeinfo);
		// Создаём строку для проверки
		const string & str = this->_fmk->format(format, items);
		// Если размер буфера меньше 3-х байт
		if(items.size() < 3)
			// Проверяем является ли это переводом строки
			isEnd = ((str.compare("\r\n") == 0) || (str.compare("\n") == 0));
		// Выполняем блокировку потока
		const lock_guard <mutex> lock(this->_mtx);
		// Если файл для вывода лога указан
		if(this->_fileMode && !this->_filename.empty()){
			// Открываем файл на запись
			ofstream file(this->_filename, ios::out | ios::app);
			// Если файл открыт
			if(file.is_open()){
				// Определяем тип сообщения
				switch(static_cast <uint8_t> (flag)){
					// Выводим сообщение так-как оно есть
					case static_cast <uint8_t> (flag_t::NONE):
						// Формируем текстовый вид лога
						logData = this->_fmk->format("%s%s", str.c_str(), (!isEnd ? "\n\n" : ""));
					break;
					// Выводим информационное сообщение
					case static_cast <uint8_t> (flag_t::INFO):
						// Формируем текстовый вид лога
						logData = this->_fmk->format("Info %s %s : %s%s", date, this->_name.c_str(), str.c_str(), (!isEnd ? "\n\n" : ""));
					break;
					// Выводим сообщение об ошибке
					case static_cast <uint8_t> (flag_t::CRITICAL):
						// Формируем текстовый вид лога
						logData = this->_fmk->format("Error %s %s : %s%s", date, this->_name.c_str(), str.c_str(), (!isEnd ? "\n\n" : ""));
					break;
					// Выводим сообщение предупреждения
					case static_cast <uint8_t> (flag_t::WARNING):
						// Формируем текстовый вид лога
						logData = this->_fmk->format("Warning %s %s : %s%s", date, this->_name.c_str(), str.c_str(), (!isEnd ? "\n\n" : ""));
					break;
				}
				// Выполняем запись в файл
				file.write(logData.data(), logData.size());
				// Закрываем файл
				file.close();
				// Выполняем ротацию логов
				this->rotate();
			}
		}
		// Определяем тип сообщения
		switch(static_cast <uint8_t> (flag)){
			// Выводим сообщение так-как оно есть
			case static_cast <uint8_t> (flag_t::NONE):
				// Формируем текстовый вид лога
				logData = this->_fmk->format("%s%s", str.c_str(), (!isEnd ? "\r\n\r\n" : ""));
			break;
			// Выводим информационное сообщение
			case static_cast <uint8_t> (flag_t::INFO):
				// Формируем текстовый вид лога
				logData = this->_fmk->format("\x1B[32m\x1B[1mInfo\x1B[0m \x1B[32m%s %s :\x1B[0m %s%s", date, this->_name.c_str(), str.c_str(), (!isEnd ? "\r\n\r\n" : ""));
			break;
			// Выводим сообщение об ошибке
			case static_cast <uint8_t> (flag_t::CRITICAL):
				// Формируем текстовый вид лога
				logData = this->_fmk->format("\x1B[31m\x1B[1mError\x1B[0m \x1B[31m%s %s :\x1B[0m %s%s", date, this->_name.c_str(), str.c_str(), (!isEnd ? "\r\n\r\n" : ""));
			break;
			// Выводим сообщение предупреждения
			case static_cast <uint8_t> (flag_t::WARNING):
				// Формируем текстовый вид лога
				logData = this->_fmk->format("\x1B[33m\x1B[1mWarning\x1B[0m \x1B[33m%s %s :\x1B[0m %s%s", date, this->_name.c_str(), str.c_str(), (!isEnd ? "\r\n\r\n" : ""));
			break;
		}
		// Если вывод сообщения в консоль разрешён
		if(this->_consoleMode){
			// Если тип сообщение не является пустым
			if(flag != flag_t::NONE) cout << "*************** START ***************" << endl << endl;
			// Выводим сообщение в консоль
			cout << logData;
			// Если тип сообщение не является пустым
			if(flag != flag_t::NONE) cout << "---------------- END ----------------" << endl << endl;
		}
		// Если функция подписки на логи установлена, выводим результат
		if(this->_fn != nullptr) this->_fn(flag, str);
	}
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
				// Выполняем поиск пула потоков принадлежащего процессу
				auto it = this->_thr.find(getpid());
				// Если пул потоков был найден
				if(it != this->_thr.end())
					// Выполняем добавление записи в пул потоков
					it->second->push(std::bind(&log_t::_print1, this, _1, _2, _3), format, flag, std::move(buffer));
				// Если пул потоков не найден
				else {
					// Выполняем создание пула потоков
					auto ret = this->_thr.emplace(getpid(), unique_ptr <thr_t> (new thr_t(2)));
					// Выполняем инициализацию пула потоков
					ret.first->second->init();
					// Выполняем добавление записи в пул потоков
					ret.first->second->push(std::bind(&log_t::_print1, this, _1, _2, _3), format, flag, std::move(buffer));
				}	
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
		  	// Выполняем поиск пула потоков принадлежащего процессу
			auto it = this->_thr.find(getpid());
			// Если пул потоков был найден
			if(it != this->_thr.end())
				// Выполняем добавление записи в пул потоков
				it->second->push(std::bind(&log_t::_print2, this, _1, _2, _3), format, flag, items);
			// Если пул потоков не найден
			else {
				// Выполняем создание пула потоков
				auto ret = this->_thr.emplace(getpid(), unique_ptr <thr_t> (new thr_t(2)));
				// Выполняем инициализацию пула потоков
				ret.first->second->init();
				// Выполняем добавление записи в пул потоков
				ret.first->second->push(std::bind(&log_t::_print2, this, _1, _2, _3), format, flag, items);
			}	
		}
	}
}
/**
 * allowFile Метод установки разрешения на вывод лога в файл
 * @param mode флаг разрешения на вывод лога в файл
 */
void awh::Log::allowFile(const bool mode) noexcept {
	// Устанавливаем разрешение на вывод лога в файл
	this->_fileMode = mode;
}
/**
 * allowConsole Метод установки разрешения на вывод лога в консоль
 * @param mode флаг разрешения на вывод лога в консоль
 */
void awh::Log::allowConsole(const bool mode) noexcept {
	// Устанавливаем разрешение на вывод лога в консоль
	this->_consoleMode = mode;
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
 * ~Log Деструктор
 */
awh::Log::~Log() noexcept {
	// Выполняем поиск пула потоков принадлежащего процессу
	auto it = this->_thr.find(getpid());
	// Если пул потоков был найден
	if(it != this->_thr.end())
		// Выполняем ожидание завершения работы потоков
		it->second->wait();
}
