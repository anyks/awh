/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <log.hpp>

/**
 * rotate Метод выполнения ротации логов
 */
void awh::Log::rotate() const noexcept {
	// Открываем файл на чтение
	ifstream file(this->logFile, ios::in);
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
		if(size >= this->maxFileSize){
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
			ifstream file(this->logFile, ios::in);
			// Если файл открыт
			if(file.is_open()){
				// Прочитанная строка из файла
				string filedata = "";
				// Открываем файл на сжатие
				gzFile gz = gzopen(this->fmk->format("%s.gz", this->logFile.c_str()).c_str(), "wb9h");
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
				::unlink(this->logFile.c_str());
			}
		}
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
		// Создаем список аргументов
		va_list args;
		// Устанавливаем начальный список аргументов
		va_start(args, flag);
		// Создаем буфер
		vector <char> buffer(BUFFER_CHUNK);
		// Заполняем буфер нулями
		memset(buffer.data(), 0, buffer.size());
		// Выполняем запись в буфер
		const size_t size = vsprintf(buffer.data(), format.c_str(), args);
		// Завершаем список аргументов
		va_end(args);
		// Если строка получена
		if(size > 0){
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
			strftime(date, sizeof(date), this->logFormat.c_str(), timeinfo);
			// Если размер буфера меньше 3-х байт
			if(size < 3){
				// Создаём строку для проверки
				const string str = buffer.data();
				// Проверяем является ли это переводом строки
				isEnd = ((str.compare("\r\n") == 0) || (str.compare("\n") == 0));
			}
			// Если файл для вывода лога указан
			if(this->fileMode && !this->logFile.empty()){
				// Открываем файл на запись
				ofstream file(this->logFile, ios::out | ios::app);
				// Если файл открыт
				if(file.is_open()){
					// Определяем тип сообщения
					switch((u_short) flag){
						// Выводим сообщение так-как оно есть
						case (u_short) flag_t::NONE: logData = this->fmk->format("%s%s", buffer.data(), (!isEnd ? "\r\n\r\n" : "")); break;
						// Выводим информационное сообщение
						case (u_short) flag_t::INFO: logData = this->fmk->format("Info %s %s : %s%s", date, this->logName.c_str(), buffer.data(), (!isEnd ? "\r\n\r\n" : "")); break;
						// Выводим сообщение об ошибке
						case (u_short) flag_t::CRITICAL: logData = this->fmk->format("Error %s %s : %s%s", date, this->logName.c_str(), buffer.data(), (!isEnd ? "\r\n\r\n" : "")); break;
						// Выводим сообщение предупреждения
						case (u_short) flag_t::WARNING: logData = this->fmk->format("Warning %s %s : %s%s", date, this->logName.c_str(), buffer.data(), (!isEnd ? "\r\n\r\n" : "")); break;
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
			switch((u_short) flag){
				// Выводим сообщение так-как оно есть
				case (u_short) flag_t::NONE: logData = this->fmk->format("%s%s", buffer.data(), (!isEnd ? "\r\n\r\n" : "")); break;
				// Выводим информационное сообщение
				case (u_short) flag_t::INFO: logData = this->fmk->format("\x1B[32m\x1B[1mInfo\x1B[0m \x1B[32m%s %s :\x1B[0m %s%s", date, this->logName.c_str(), buffer.data(), (!isEnd ? "\r\n\r\n" : "")); break;
				// Выводим сообщение об ошибке
				case (u_short) flag_t::CRITICAL: logData = this->fmk->format("\x1B[31m\x1B[1mError\x1B[0m \x1B[31m%s %s :\x1B[0m %s%s", date, this->logName.c_str(), buffer.data(), (!isEnd ? "\r\n\r\n" : "")); break;
				// Выводим сообщение предупреждения
				case (u_short) flag_t::WARNING: logData = this->fmk->format("\x1B[33m\x1B[1mWarning\x1B[0m \x1B[33m%s %s :\x1B[0m %s%s", date, this->logName.c_str(), buffer.data(), (!isEnd ? "\r\n\r\n" : "")); break;
			}
			// Если вывод сообщения в консоль разрешён
			if(this->consoleMode){
				// Если тип сообщение не является пустым
				if(flag != flag_t::NONE) cout << "*************** START ***************" << endl << endl;
				// Выводим сообщение в консоль
				cout << logData;
				// Если тип сообщение не является пустым
				if(flag != flag_t::NONE) cout << "---------------- END ----------------" << endl << endl;
			}
			// Если функция подписки на логи установлена, выводим результат
			if(this->subscribeFn != nullptr) this->subscribeFn(flag, logData);
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
		strftime(date, sizeof(date), this->logFormat.c_str(), timeinfo);
		// Создаём строку для проверки
		const string & str = this->fmk->format(format, items);
		// Если размер буфера меньше 3-х байт
		if(items.size() < 3)
			// Проверяем является ли это переводом строки
			isEnd = ((str.compare("\r\n") == 0) || (str.compare("\n") == 0));
		// Если файл для вывода лога указан
		if(this->fileMode && !this->logFile.empty()){
			// Открываем файл на запись
			ofstream file(this->logFile, ios::out | ios::app);
			// Если файл открыт
			if(file.is_open()){
				// Определяем тип сообщения
				switch((u_short) flag){
					// Выводим сообщение так-как оно есть
					case (u_short) flag_t::NONE: logData = this->fmk->format("%s%s", str.c_str(), (!isEnd ? "\n\n" : "")); break;
					// Выводим информационное сообщение
					case (u_short) flag_t::INFO: logData = this->fmk->format("Info %s %s : %s%s", date, this->logName.c_str(), str.c_str(), (!isEnd ? "\n\n" : "")); break;
					// Выводим сообщение об ошибке
					case (u_short) flag_t::CRITICAL: logData = this->fmk->format("Error %s %s : %s%s", date, this->logName.c_str(), str.c_str(), (!isEnd ? "\n\n" : "")); break;
					// Выводим сообщение предупреждения
					case (u_short) flag_t::WARNING: logData = this->fmk->format("Warning %s %s : %s%s", date, this->logName.c_str(), str.c_str(), (!isEnd ? "\n\n" : "")); break;
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
		switch((u_short) flag){
			// Выводим сообщение так-как оно есть
			case (u_short) flag_t::NONE: logData = this->fmk->format("%s%s", str.c_str(), (!isEnd ? "\r\n\r\n" : "")); break;
			// Выводим информационное сообщение
			case (u_short) flag_t::INFO: logData = this->fmk->format("\x1B[32m\x1B[1mInfo\x1B[0m \x1B[32m%s %s :\x1B[0m %s%s", date, this->logName.c_str(), str.c_str(), (!isEnd ? "\r\n\r\n" : "")); break;
			// Выводим сообщение об ошибке
			case (u_short) flag_t::CRITICAL: logData = this->fmk->format("\x1B[31m\x1B[1mError\x1B[0m \x1B[31m%s %s :\x1B[0m %s%s", date, this->logName.c_str(), str.c_str(), (!isEnd ? "\r\n\r\n" : "")); break;
			// Выводим сообщение предупреждения
			case (u_short) flag_t::WARNING: logData = this->fmk->format("\x1B[33m\x1B[1mWarning\x1B[0m \x1B[33m%s %s :\x1B[0m %s%s", date, this->logName.c_str(), str.c_str(), (!isEnd ? "\r\n\r\n" : "")); break;
		}
		// Если вывод сообщения в консоль разрешён
		if(this->consoleMode){
			// Если тип сообщение не является пустым
			if(flag != flag_t::NONE) cout << "*************** START ***************" << endl << endl;
			// Выводим сообщение в консоль
			cout << logData;
			// Если тип сообщение не является пустым
			if(flag != flag_t::NONE) cout << "---------------- END ----------------" << endl << endl;
		}
		// Если функция подписки на логи установлена, выводим результат
		if(this->subscribeFn != nullptr) this->subscribeFn(flag, logData);
	}
}
/**
 * allowFile Метод установки разрешения на вывод лога в файл
 * @param mode флаг разрешения на вывод лога в файл
 */
void awh::Log::allowFile(const bool mode) noexcept {
	// Устанавливаем разрешение на вывод лога в файл
	this->fileMode = mode;
}
/**
 * allowConsole Метод установки разрешения на вывод лога в консоль
 * @param mode флаг разрешения на вывод лога в консоль
 */
void awh::Log::allowConsole(const bool mode) noexcept {
	// Устанавливаем разрешение на вывод лога в консоль
	this->consoleMode = mode;
}
/**
 * setLogName Метод установки название сервиса для вывода лога
 * @param name название сервиса для вывода лога
 */
void awh::Log::setLogName(const string & name) noexcept {
	// Устанавливаем название сервиса для вывода лога
	this->logName = name;
}
/**
 * setLogMaxFileSize Метод установки максимального размера файла логов
 * @param size максимальный размер файла логов
 */
void awh::Log::setLogMaxFileSize(const float size) noexcept {
	// Устанавливаем максимальный размер файла логов
	this->maxFileSize = size;
}
/**
 * setLogFormat Метод установки формата даты и времени для вывода лога
 * @param format формат даты и времени для вывода лога
 */
void awh::Log::setLogFormat(const string & format) noexcept {
	// Устанавливаем формат даты и времени для вывода лога
	this->logFormat = format;
}
/**
 * setLogFilename Метод установки файла для сохранения логов
 * @param filename адрес файла для сохранения логов
 */
void awh::Log::setLogFilename(const string & filename) noexcept {
	// Устанавливаем адрес файла для сохранения логов
	this->logFile = filename;
}
/**
 * subscribe Метод подписки на события логов
 * @param callback функция обратного вызова
 */
void awh::Log::subscribe(function <void (const flag_t, const string &)> callback) noexcept {
	// Устанавливаем функцию подписки на получение лога
	this->subscribeFn = callback;
}
