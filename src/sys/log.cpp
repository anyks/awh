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
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Оператор [=] перемещения параметров полезной нагрузки
 * @param payload объект полезной нагрузки для перемещения
 * @return        текущий объект полезной нагрузки
 */
awh::Log::Payload & awh::Log::Payload::operator = (payload_t && payload) noexcept {
	// Выполняем установку флага
	this->flag = payload.flag;
	// Выполняем перемещение текста
	this->text = ::move(payload.text);
	// Выводим текущий объект
	return (* this);
}
/**
 * Оператор [=] присванивания параметров полезной нагрузки
 * @param payload объект полезной нагрузки для копирования
 * @return        текущий объект полезной нагрузки
 */
awh::Log::Payload & awh::Log::Payload::operator = (const payload_t & payload) noexcept {
	// Выполняем установку флага
	this->flag = payload.flag;
	// Выполняем копирование текста
	this->text = payload.text;
	// Выводим текущий объект
	return (* this);
}
/**
 * Оператор сравнения
 * @param payload объект полезной нагрузки для сравнения
 * @return        результат сравнения
 */
bool awh::Log::Payload::operator == (const payload_t & payload) noexcept {
	// Выполняем проверку полезной нагрузки
	return (
		(this->flag == payload.flag) &&
		(this->text.compare(payload.text) == 0)
	);
}
/**
 * Payload Конструктор перемещения
 * @param payload объект полезной нагрузки для перемещения
 */
awh::Log::Payload::Payload(payload_t && payload) noexcept {
	// Выполняем установку флага
	this->flag = payload.flag;
	// Выполняем перемещение текста
	this->text = ::move(payload.text);
}
/**
 * Payload Конструктор копирования
 * @param payload объект полезной нагрузки для копирования
 */
awh::Log::Payload::Payload(const payload_t & payload) noexcept {
	// Выполняем установку флага
	this->flag = payload.flag;
	// Выполняем копирование текста
	this->text = payload.text;
}
/**
 * Payload Конструктор
 */
awh::Log::Payload::Payload() noexcept : flag(flag_t::NONE), text{""} {}
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
				// Создаём объект потока
				stringstream date;
				// Определяем количество секунд
				const time_t seconds = time(nullptr);
				// Получаем структуру локального времени
				tm * tm = localtime(&seconds);
				// Выполняем извлечение даты
				date << put_time(tm, "_%m-%d-%Y_%H-%M-%S");
				/**
				 * Выполняем работу для Windows
				 */
				#if defined(_WIN32) || defined(_WIN64)
					// Буфер данных для чтения
					vector <char> buffer(size, 0);
					// Выполняем чтение из файла в буфер данные
					ReadFile(file, static_cast <LPVOID> (buffer.data()), static_cast <DWORD> (buffer.size()), 0, nullptr);
					// Если данные строки получены
					if(!buffer.empty()){
						// Получаем компоненты адреса
						const auto & cmp = this->components(this->_filename);
						// Открываем файл на сжатие
						gzFile gz = ::gzopen_w(this->_fmk->convert(this->_fmk->format("%s%s%s.gz", cmp.first.c_str(), cmp.second.c_str(), date.str().c_str())).c_str(), "wb9h");
						// Если файл открыт удачно
						if(gz != nullptr){
							// Выполняем сжатие файла
							::gzwrite(gz, buffer.data(), buffer.size());
							// Закрываем сжатый файл
							::gzclose(gz);
						// Если произошла ошибка
						} else {
							// Создаём буфер сообщения ошибки
							wchar_t message[256] = {0};
							// Выполняем формирование текста ошибки
							FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, WSAGetLastError(), 0, message, 256, 0);
							// Выводим текст полученной ошибки
							::fprintf(stderr, "Log rotate: %s\n", this->_fmk->convert(message).c_str());
						}
					}
				/**
				 * Выполняем работу для Unix
				 */
				#else
					// Открываем файл на чтение
					ifstream file(this->_filename, ios::in);
					// Если файл открыт
					if(file.is_open()){
						// Буфер данных для чтения
						vector <char> buffer(size, 0);
						// Выполняем чтение данных из файла
						file.read(buffer.data(), buffer.size());
						// Если данные строки получены
						if(!buffer.empty()){
							// Получаем компоненты адреса
							const auto & cmp = this->components(this->_filename);
							// Открываем файл на сжатие
							gzFile gz = ::gzopen(this->_fmk->format("%s%s%s.gz", cmp.first.c_str(), cmp.second.c_str(), date.str().c_str()).c_str(), "wb9h");
							// Если файл открыт удачно
							if(gz != nullptr){
								// Выполняем сжатие файла
								::gzwrite(gz, buffer.data(), buffer.size());
								// Закрываем сжатый файл
								::gzclose(gz);
							// Если произошла ошибка
							} else ::fprintf(stderr, "Log rotate: %s\n", ::strerror(errno));
						}
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
			if(size >= this->_maxSize)
				// Удаляем исходный файл логов
				_wunlink(filename.c_str());
		#endif
}
/**
 * cleaner Метод очистки строки от символов форматирования
 * @param text текст для очистки
 * @return     ощиченный текста
 */
string & awh::Log::cleaner(string & text) const noexcept {
	// Позиция найденного элемента
	size_t pos = 0;
	// Значение текущего символа
	char letter = 0;
	// Выполняем поиск символов экранирования
	while((pos = text.find("\x1B[", pos)) != string::npos){
		// Выполняем поиск завершения блока экранирования
		for(size_t i = (pos + 3); i < text.length(); i++){
			// Выполняем получение текущего символа
			letter = text.at(i);
			// Если мы получили символ завершения блока
			if(letter == 'm'){
				// Выполняем удаление
				text.erase(pos, (i + 1) - pos);
				// Выходим из цикла
				break;
			// Если символ не является числом, выходим
			} else if(!this->_fmk->is(letter, fmk_t::check_t::NUMBER)) {
				// Выполняем удаление
				text.erase(pos, i - pos);
				// Выходим из функции
				return text;
			}
		}
	}
	// Выводим полученный результат
	return text;
}
/**
 * receiving Метод получения данных
 * @param payload объект полезной нагрузки
 */
void awh::Log::receiving(const payload_t & payload) const noexcept {
	// Флаг конца строки
	bool isEnd = false;
	// Создаём объект потока
	stringstream date;
	// Определяем количество секунд
	const time_t seconds = time(nullptr);
	// Получаем структуру локального времени
	tm * tm = localtime(&seconds);
	// Выполняем извлечение даты
	date << put_time(tm, this->_format.c_str());
	// Если размер буфера меньше 3-х байт
	if(payload.text.length() < 3)
		// Проверяем является ли это переводом строки
		isEnd = ((payload.text.compare(AWH_STRING_BREAK) == 0) || (payload.text.compare(AWH_STRING_BREAK) == 0));
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Если функция подписки на логи установлена, выводим результат
	if((this->_mode.find(mode_t::DEFERRED) != this->_mode.end()) && (this->_fn != nullptr))
		// Выводим сообщение лога всем подписавшимся
		this->_fn(payload.flag, payload.text);
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
				cout << this->_fmk->format("%s%s", payload.text.c_str(), (!isEnd ? AWH_STRING_BREAKS : ""));
			break;
			// Выводим информационное сообщение
			case static_cast <uint8_t> (flag_t::INFO):
				// Формируем текстовый вид лога
				cout << this->_fmk->format("\x1B[32m\x1B[1mInfo\x1B[0m \x1B[32m%s %s :\x1B[0m %s%s", date.str().c_str(), this->_name.c_str(), payload.text.c_str(), (!isEnd ? AWH_STRING_BREAKS : ""));
			break;
			// Выводим сообщение об ошибке
			case static_cast <uint8_t> (flag_t::CRITICAL):
				// Формируем текстовый вид лога
				cout << this->_fmk->format("\x1B[31m\x1B[1mError\x1B[0m \x1B[31m%s %s :\x1B[0m %s%s", date.str().c_str(), this->_name.c_str(), payload.text.c_str(), (!isEnd ? AWH_STRING_BREAKS : ""));
			break;
			// Выводим сообщение предупреждения
			case static_cast <uint8_t> (flag_t::WARNING):
				// Формируем текстовый вид лога
				cout << this->_fmk->format("\x1B[33m\x1B[1mWarning\x1B[0m \x1B[33m%s %s :\x1B[0m %s%s", date.str().c_str(), this->_name.c_str(), payload.text.c_str(), (!isEnd ? AWH_STRING_BREAKS : ""));
			break;
		}
		// Если тип сообщение не является пустым
		if(payload.flag != flag_t::NONE)
			// Выводим обозначение конца вывода лога
			cout << "---------------- END ----------------" << endl << endl;
	}
	// Если файл для вывода лога указан
	if((this->_mode.find(mode_t::FILE) != this->_mode.end()) && !this->_filename.empty()){
		// Выполняем очистку от символов форматирования
		this->cleaner(const_cast <string &> (payload.text));
		/**
		 * Выполняем работу для Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Выполняем открытие файла на запись
			HANDLE file = CreateFileW(this->_fmk->convert(this->_filename).c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
			// Если открыть файл открыт нормально
			if(file != INVALID_HANDLE_VALUE){
				// Текст логирования для записи в файл лога
				string logData = "";
				// Определяем тип сообщения
				switch(static_cast <uint8_t> (payload.flag)){
					// Выводим сообщение так-как оно есть
					case static_cast <uint8_t> (flag_t::NONE):
						// Формируем текстовый вид лога
						logData = this->_fmk->format("%s%s", payload.text.c_str(), (!isEnd ? AWH_STRING_BREAKS : ""));
					break;
					// Выводим информационное сообщение
					case static_cast <uint8_t> (flag_t::INFO):
						// Формируем текстовый вид лога
						logData = this->_fmk->format("Info %s %s : %s%s", date.str().c_str(), this->_name.c_str(), payload.text.c_str(), (!isEnd ? AWH_STRING_BREAKS : ""));
					break;
					// Выводим сообщение об ошибке
					case static_cast <uint8_t> (flag_t::CRITICAL):
						// Формируем текстовый вид лога
						logData = this->_fmk->format("Error %s %s : %s%s", date.str().c_str(), this->_name.c_str(), payload.text.c_str(), (!isEnd ? AWH_STRING_BREAKS : ""));
					break;
					// Выводим сообщение предупреждения
					case static_cast <uint8_t> (flag_t::WARNING):
						// Формируем текстовый вид лога
						logData = this->_fmk->format("Warning %s %s : %s%s", date.str().c_str(), this->_name.c_str(), payload.text.c_str(), (!isEnd ? AWH_STRING_BREAKS : ""));
					break;
				}
				// Выполняем запись в файл
				WriteFile(file, static_cast <LPCVOID> (logData.data()), static_cast <DWORD> (logData.size()), 0, nullptr);
				// Выполняем закрытие файла
				CloseHandle(file);
			}
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
						file << this->_fmk->format("%s%s", payload.text.c_str(), (!isEnd ? AWH_STRING_BREAKS : ""));
					break;
					// Выводим информационное сообщение
					case static_cast <uint8_t> (flag_t::INFO):
						// Формируем текстовый вид лога
						file << this->_fmk->format("Info %s %s : %s%s", date.str().c_str(), this->_name.c_str(), payload.text.c_str(), (!isEnd ? AWH_STRING_BREAKS : ""));
					break;
					// Выводим сообщение об ошибке
					case static_cast <uint8_t> (flag_t::CRITICAL):
						// Формируем текстовый вид лога
						file << this->_fmk->format("Error %s %s : %s%s", date.str().c_str(), this->_name.c_str(), payload.text.c_str(), (!isEnd ? AWH_STRING_BREAKS : ""));
					break;
					// Выводим сообщение предупреждения
					case static_cast <uint8_t> (flag_t::WARNING):
						// Формируем текстовый вид лога
						file << this->_fmk->format("Warning %s %s : %s%s", date.str().c_str(), this->_name.c_str(), payload.text.c_str(), (!isEnd ? AWH_STRING_BREAKS : ""));
					break;
				}
				// Закрываем файл
				file.close();
				// Выполняем ротацию логов
				this->rotate();
			}
		#endif
	}
}
/**
 * components Метод извлечения компонента адреса файла
 * @param filename адрес где находится файл
 * @return         параметры компонента (адрес, название файла без расширения)
 */
pair <string, string> awh::Log::components(const string & filename) const noexcept {
	// Результат работы функции
	pair <string, string> result;
	// Если адрес передан
	if(!filename.empty()){
		// Позиция разделителя каталога
		size_t pos1 = 0, pos2 = 0;
		// Выполняем поиск разделителя каталога
		if((pos1 = filename.rfind(FS_SEPARATOR, filename.length() - 1)) != string::npos){
			// Ищем расширение файла
			if((pos2 = filename.find('.', pos1 + 1)) != string::npos){
				// Устанавливаем адрес файла где хранится файл
				result.first = filename.substr(0, pos1 + 1);
				// Устанавливаем название файла без расширения
				result.second = filename.substr(pos1 + 1, pos2 - (pos1 + 1));
			}
		// Если разделитель каталога не найден
		} else {
			// Ищем расширение файла
			if((pos2 = filename.find('.')) != string::npos){
				/**
				 * Выполняем работу для Unix
				 */
				#if !defined(_WIN32) && !defined(_WIN64)
					// Устанавливаем адрес файла где хранится файл
					result.first.append("./");
				#endif
				// Устанавливаем название файла без расширения
				result.second = filename.substr(0, pos2);
			}
		}
	}
	// Выводим результат
	return result;
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
			for(;;){
				// Создаем список аргументов
				va_list args2;
				// Копируем список аргументов
				va_copy(args2, args);
				// Выполняем запись в буфер данных
				size_t res = ::vsnprintf(buffer.data(), buffer.size(), format.c_str(), args2);
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
				payload.text.assign(buffer.begin(), buffer.end());
				// Если асинхронный режим работы активирован
				if(this->_async){
					// Получаем идентификатор текущего процесса
					const pid_t pid = ::getpid();
					// Если идентификатор процесса является дочерним
					if(pid != this->_pid){
						// Если процесс ещё не инициализирован и дочерний поток уже создан
						if(static_cast <bool> (this->_screen) && (this->_initialized.count(pid) < 1)){
							// Выполняем остановку скрина
							this->_screen.stop();
							// Выполняем очистку списка инициализированных процессов
							this->_initialized.clear();
						}
					}
					// Если дочерний поток не создан
					if(!static_cast <bool> (this->_screen)){
						// Выполняем установку функцию обратного вызова
						this->_screen = static_cast <function <void (const payload_t &)>> (bind(&log_t::receiving, this, _1));
						// Выполняем инициализацию текущего процесса
						this->_initialized.emplace(pid);
						// Запускаем работу скрина
						this->_screen.start();
					}
					// Выполняем отправку сообщения дочернему потоку
					this->_screen = ::move(payload);
				// Выполняем вывод полученного лога
				} else this->receiving(payload);
			}
		}
	}
}
/**
 * print Метод вывода текстовой информации в консоль или файл
 * @param format формат строки вывода
 * @param flag   флаг типа логирования
 * @param args   список аргументов для замены
 */
void awh::Log::print(const string & format, flag_t flag, const vector <string> & args) const noexcept {
	// Если формат передан
	if(!format.empty() && !args.empty()){
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
			payload.text = this->_fmk->format(format, args);
			// Если асинхронный режим работы активирован
			if(this->_async){
				// Получаем идентификатор текущего процесса
				const pid_t pid = ::getpid();
				// Если идентификатор процесса является дочерним
				if(pid != this->_pid){
					// Если процесс ещё не инициализирован, а скрин уже запущен
					if(static_cast <bool> (this->_screen) && (this->_initialized.count(pid) < 1)){
						// Выполняем остановку скрина
						this->_screen.stop();
						// Выполняем очистку списка инициализированных процессов
						this->_initialized.clear();
					}
				}
				// Если дочерний поток не создан
				if(!static_cast <bool> (this->_screen)){
					// Выполняем установку функцию обратного вызова
					this->_screen = static_cast <function <void (const payload_t &)>> (bind(&log_t::receiving, this, _1));
					// Выполняем инициализацию текущего процесса
					this->_initialized.emplace(pid);
					// Запускаем работу скрина
					this->_screen.start();
				}
				// Выполняем отправку сообщения дочернему потоку
				this->_screen = ::move(payload);
			// Выполняем вывод полученного лога
			} else this->receiving(payload);
		}
	}
}
/**
 * mode Метод получения установленных режимов вывода логов
 * @return список режимов вывода логов
 */
const set <awh::Log::mode_t> & awh::Log::mode() const noexcept {
	// Выводим список режимов вывода логов
	return this->_mode;
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
 * format Метод извлечения установленного формата лога
 * @return формат лога для извлечения
 */
const string & awh::Log::format() const noexcept {
	// Выводим установленный формат
	return this->_format;
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
 _level(level_t::ALL), _name{AWH_SHORT_NAME}, _format{DATE_FORMAT},
 _filename{filename}, _screen(Screen <payload_t>::health_t::DEAD), _fn(nullptr), _fmk(fmk) {
	// Запоминаем идентификатор родительского объекта
	this->_pid = ::getpid();
	// Выполняем разрешение на вывод всех видов логов
	this->_mode = {mode_t::FILE, mode_t::CONSOLE, mode_t::DEFERRED};
}
/**
 * ~Log Деструктор
 */
awh::Log::~Log() noexcept {
	// Если объект работы с дочерним потоком создан, удаляем
	if(static_cast <bool> (this->_screen))
		// Останавливаем работу скрина
		this->_screen.stop();
}
