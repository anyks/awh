/**
 * @file args.cpp
 * @date 2026-09-02
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
 * \~russian
 * @brief Исходный файл модуля параметров запуска приложения
 *
 * \~english
 * @brief Source file of the module of the parameters of the launch of an application
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы модуля
 */
#include <args/args.hpp>

/**
 * Если операционной системой является MS Windows
 */
#if _WIN32 || _WIN64
	/**
	 * Подключаем заголовочный файл, объявляющий набор переменных окружения
	 *
	 * @details Объявлять его самому у MS Windows нельзя: `_environ` там не
	 *          переменная, а макрос к внутренности библиотеки времени выполнения
	 */
	#include <stdlib.h>
#endif

/**
 * Если операционной системой является macOS
 */
#if __APPLE__
	/**
	 * Подключаем заголовочный файл доступа к набору переменных окружения
	 *
	 * @details Обращение к «environ» напрямую под macOS дозволено лишь исполняемому
	 *          файлу: у библиотеки собиратель отвечает отказом связывания, и набор
	 *          берётся ходом «_NSGetEnviron»
	 */
	#include <crt_externs.h>
#endif

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Используем пространство имён параметров запуска приложения
 */
using namespace awh::args;

/**
 * Если операционной системой не является ни macOS, ни MS Windows
 *
 * @warning Объявление это обязано стоять ПЕРЕД помощником, что его зовёт: прежде
 *          оно стояло ЗА ним, и держалось лишь на том, что набор переменных
 *          окружения объявлен заголовками системы. У MS Windows объявления нет
 *          вовсе - `_environ` там разворачивается заголовком `stdlib.h`
 */
#if !__APPLE__ && !_WIN32 && !_WIN64
	/**
	 * Объявляем набор переменных окружения
	 */
	extern "C" char ** environ;
#endif

/**
 * @brief Внутренние помощники параметров запуска приложения
 *
 */
namespace {
	/**
	 * @brief Метод извлечения набора переменных окружения
	 *
	 * @return набор переменных окружения либо nullptr при его отсутствии
	 *
	 */
	char ** environment() noexcept {
		/**
		 * Если операционной системой является macOS
		 */
		#if __APPLE__
			// Выводим набор переменных окружения ходом системы
			return (* ::_NSGetEnviron());
		/**
		 * Если операционной системой является MS Windows
		 */
		#elif _WIN32 || _WIN64
			/**
			 * Выводим набор переменных окружения времени выполнения
			 *
			 * @warning Квалификатор пространства имён здесь НЕДОПУСТИМ: у MinGW
			 *          `_environ` есть макрос, разворачивающийся в вызов вида
			 *          `(* __p__environ())`, и запись `::_environ` даёт
			 *          `::(* __p__environ())` - отказ сборки. Замерено на стенде
			 */
			return _environ;
		/**
		 * Для всех остальных операционных систем
		 */
		#else
			// Выводим набор переменных окружения
			return ::environ;
		#endif
	}
}

/**
 * @brief Метод перевода имени параметра в путь оси хранения
 *
 * @param key имя параметра с разделителем звеньев
 * @return    путь звеньями оси хранения
 *
 */
string awh::args::Args::route(const string_view key) const noexcept {
	// Путь звеньями оси хранения
	string result(key);
	// Выполняем перебор всех знаков имени параметра
	for(size_t i = 0; i < result.length(); i++){
		// Если знаком является разделитель звеньев пути
		if(result.at(i) == this->_settings.delimiter)
			// Выполняем замену его разделителем оси хранения
			result.replace(i, 1, 1, '/');
	}
	// Выводим путь звеньями оси хранения
	return result;
}

/**
 * @brief Метод выведения значения из его записи
 *
 * @param text запись значения
 * @return     выведенное значение оси хранения
 *
 */
awh::codec::abc::value_t awh::args::Args::derive(const string_view text) const noexcept {
	// Если вывод вида значения отключён настройками
	if(!this->_settings.typed)
		// Выводим запись значения последовательностью знаков как есть
		return codec::abc::value_t(string(text));
	// Если запись значения пуста вовсе
	if(text.empty())
		// Выводим пустую последовательность знаков
		return codec::abc::value_t(string(text));
	// Если запись значения является истиной
	if(this->_fmk->compare(text, "true") || this->_fmk->compare(text, "yes") || this->_fmk->compare(text, "on"))
		// Выводим логическое значение истиной
		return codec::abc::value_t(true);
	// Если запись значения является ложью
	if(this->_fmk->compare(text, "false") || this->_fmk->compare(text, "no") || this->_fmk->compare(text, "off"))
		// Выводим логическое значение ложью
		return codec::abc::value_t(false);
	// Если запись значения является пустым значением
	if(this->_fmk->compare(text, "null") || this->_fmk->compare(text, "nil"))
		// Выводим пустое значение
		return codec::abc::value_t(codec::abc::kind_t::NUL);
	// Если запись значения является числом
	if(this->_fmk->is(text, fmk_t::check_t::NUMBER)){
		/**
		 * Если запись числа начата нулём, а знаков в ней больше одного, числом она
		 * НЕ БЕРЁТСЯ: записи вида «007» и «0755» встречаются номерами и правами
		 * доступа, и перевод их в число молча срезал бы ведущие нули
		 */
		if((text.front() != '0') || (text.length() < 2)){
			// Если запись числа начата знаком минуса
			if(text.front() == '-')
				// Выводим число видом целого со знаком
				return codec::abc::value_t(this->_fmk->atoi <int64_t> (text));
			// Выводим число видом целого без знака
			return codec::abc::value_t(this->_fmk->atoi <uint64_t> (text));
		}
	// Если запись значения является числом дробным
	} else if(this->_fmk->is(text, fmk_t::check_t::DECIMAL))
		// Выводим число видом дробного двойной точности
		return codec::abc::value_t(this->_fmk->atoi <double> (text));
	// Выводим запись значения последовательностью знаков как есть
	return codec::abc::value_t(string(text));
}

/**
 * @brief Метод укладки значения в дерево настроек
 *
 * @param path   путь звеньями оси хранения
 * @param value  значение для укладки
 * @param source источник значения
 * @return       результат укладки
 *
 */
bool awh::args::Args::lay(const string & path, codec::abc::value_t && value, const source_t source) noexcept {
	// Если путь укладки пуст вовсе
	if(path.empty())
		// Выходим из метода, укладывать значение некуда
		return false;
	// Выполняем поиск источника уже уложенного значения
	auto i = this->_origins.find(path);
	// Если значение по этому пути уже уложено
	if(i != this->_origins.end()){
		/**
		 * Если уложенное значение взято из источника СТАРШЕГО, поверх него значение
		 * НЕ ЛОЖИТСЯ: иначе разбор файла настроек, поданный после набора запуска,
		 * молча отменял бы поданное из набора
		 */
		if(static_cast <uint8_t> (i->second) > static_cast <uint8_t> (source))
			// Выходим из метода, укладка старшего источника сохранена
			return true;
		// Если значение подано повторно тем же самым источником
		if((i->second == source) && this->_settings.multiple){
			// Получаем ссылку на уже уложенное значение
			codec::abc::value_t & current = this->_root.place(path);
			// Если уложенное значение массивом ещё не является
			if(!current.is(codec::abc::type_t::ARRAY)){
				// Создаём массив собираемых значений параметра
				codec::abc::value_t array(codec::abc::kind_t::ARRAY);
				// Добавляем в массив уже уложенное значение
				if(!array.push(current))
					// Выходим из метода, укладка отвечена отказом
					return false;
				// Заменяем уложенное значение собранным массивом
				current = ::move(array);
			}
			// Добавляем в массив вновь поданное значение
			return current.push(value);
		}
	}
	// Выполняем укладку значения по пути оси хранения
	this->_root.place(path) = ::move(value);
	// Запоминаем источник уложенного значения
	this->_origins[path] = source;
	// Сообщаем, что укладка значения выполнена
	return true;
}

/**
 * @brief Метод очистки собранных параметров запуска
 *
 */
void awh::args::Args::clear() noexcept {
	// Выполняем очистку дерева собранных значений настроек
	this->_root = codec::abc::value_t(codec::abc::kind_t::MAP);
	// Выполняем очистку источников собранных значений настроек
	this->_origins.clear();
	// Выполняем очистку позиционных доводов набора запуска
	this->_operands.clear();
	// Выполняем очистку отказов последнего разбора
	this->_errors.clear();
}

/**
 * @brief Метод разбора набора доводов запуска
 *
 * @param count число доводов набора запуска
 * @param items набор доводов запуска
 * @return      результат разбора
 *
 */
bool awh::args::Args::parse(const int32_t count, const char * items[]) noexcept {
	// Если набор доводов запуска не подан вовсе
	if((count <= 0) || (items == nullptr))
		// Выходим из метода, разбирать нечего
		return false;
	// Контейнер доводов набора запуска
	vector <string> result;
	// Выполняем резервирование памяти под доводы набора запуска
	result.reserve(static_cast <size_t> (count));
	/**
	 * Выполняем перебор всех доводов набора запуска, ПРОПУСКАЯ первый: им приходит
	 * путь к исполняемому файлу, а не параметр приложения
	 */
	for(int32_t i = 1; i < count; i++){
		// Если довод набора запуска подан
		if(items[i] != nullptr)
			// Добавляем довод в контейнер доводов набора запуска
			result.emplace_back(items[i]);
	}
	// Выполняем разбор собранного набора доводов запуска
	return this->parse(result);
}

/**
 * @brief Метод разбора набора доводов запуска широкими знаками
 *
 * @param count число доводов набора запуска
 * @param items набор доводов запуска
 * @return      результат разбора
 *
 */
bool awh::args::Args::parse(const int32_t count, const wchar_t * items[]) noexcept {
	// Если набор доводов запуска не подан вовсе
	if((count <= 0) || (items == nullptr))
		// Выходим из метода, разбирать нечего
		return false;
	// Контейнер доводов набора запуска
	vector <string> result;
	// Выполняем резервирование памяти под доводы набора запуска
	result.reserve(static_cast <size_t> (count));
	/**
	 * Выполняем перебор всех доводов набора запуска, ПРОПУСКАЯ первый: им приходит
	 * путь к исполняемому файлу, а не параметр приложения
	 */
	for(int32_t i = 1; i < count; i++){
		// Если довод набора запуска подан
		if(items[i] != nullptr)
			// Добавляем довод, переведённый из широких знаков, в контейнер
			result.emplace_back(this->_fmk->convert(items[i]));
	}
	// Выполняем разбор собранного набора доводов запуска
	return this->parse(result);
}

/**
 * @brief Метод разбора набора доводов запуска
 *
 * @param items набор доводов запуска
 * @return      результат разбора
 *
 */
bool awh::args::Args::parse(const vector <string> & items) noexcept {
	// Выполняем очистку отказов последнего разбора
	this->_errors.clear();
	// Признак успешности укладки разобранного
	bool result = true;
	// Выполняем разбор набора доводов запуска
	const bool parsed = this->_lexer.parse(items, [this, &result](const lexeme_t & lexeme) noexcept -> bool {
		// Определяем вид разобранной лексемы
		switch(static_cast <uint8_t> (lexeme.type)){
			// Если лексемой является именованный параметр
			case static_cast <uint8_t> (token_t::PARAM): {
				// Выполняем перевод имени параметра в путь оси хранения
				const string & path = this->route(lexeme.key);
				/**
				 * Значение, не поданное вовсе, ложится ИСТИНОЙ: запись «--verbose»
				 * есть взведённый признак, а не пустая последовательность знаков
				 */
				if(!lexeme.assigned)
					// Выполняем укладку взведённого признака
					result = (this->lay(path, codec::abc::value_t(true), source_t::CLI) && result);
				// Выполняем укладку выведенного значения параметра
				else result = (this->lay(path, this->derive(lexeme.value), source_t::CLI) && result);
			} break;
			// Если лексемой является позиционный довод
			case static_cast <uint8_t> (token_t::OPERAND):
				// Добавляем позиционный довод в контейнер собранных
				this->_operands.emplace_back(lexeme.value);
			break;
		}
		// Сообщаем, что разбор следует продолжить
		return true;
	}, [this](const error_t error, const location_t & location) noexcept -> bool {
		// Выполняем запоминание отказа разбора вместе с его положением
		this->_errors.emplace_back(error, location);
		// Выводим в лог сообщение об отказе разбора набора запуска
		this->_log->print("Args: %s at argument %zu", log_t::flag_t::WARNING, args::message(error), location.index);
		// Сообщаем, что разбор следует продолжить
		return true;
	});
	// Выводим результат разбора набора доводов запуска
	return (parsed && result && this->_errors.empty());
}

/**
 * @brief Метод разбора текстового потока
 *
 * @param text текст для разбора
 * @return     результат разбора
 *
 */
bool awh::args::Args::text(const string_view text) noexcept {
	// Выполняем очистку отказов последнего разбора
	this->_errors.clear();
	// Признак успешности укладки разобранного
	bool result = true;
	// Выполняем разбор поданного текстового потока
	const bool parsed = this->_lexer.parse(text, [this, &result](const lexeme_t & lexeme) noexcept -> bool {
		// Определяем вид разобранной лексемы
		switch(static_cast <uint8_t> (lexeme.type)){
			// Если лексемой является именованный параметр
			case static_cast <uint8_t> (token_t::PARAM): {
				// Выполняем перевод имени параметра в путь оси хранения
				const string & path = this->route(lexeme.key);
				// Если значение параметру не подано вовсе
				if(!lexeme.assigned)
					// Выполняем укладку взведённого признака
					result = (this->lay(path, codec::abc::value_t(true), source_t::TEXT) && result);
				// Выполняем укладку выведенного значения параметра
				else result = (this->lay(path, this->derive(lexeme.value), source_t::TEXT) && result);
			} break;
			// Если лексемой является позиционный довод
			case static_cast <uint8_t> (token_t::OPERAND):
				// Добавляем позиционный довод в контейнер собранных
				this->_operands.emplace_back(lexeme.value);
			break;
		}
		// Сообщаем, что разбор следует продолжить
		return true;
	}, [this](const error_t error, const location_t & location) noexcept -> bool {
		// Выполняем запоминание отказа разбора вместе с его положением
		this->_errors.emplace_back(error, location);
		// Выводим в лог сообщение об отказе разбора текстового потока
		this->_log->print("Args: %s at word %zu", log_t::flag_t::WARNING, args::message(error), location.index);
		// Сообщаем, что разбор следует продолжить
		return true;
	});
	// Выводим результат разбора текстового потока
	return (parsed && result && this->_errors.empty());
}

/**
 * @brief Метод сбора переменных окружения
 *
 * @return результат сбора
 *
 */
bool awh::args::Args::env() noexcept {
	// Если начало имён переменных окружения не установлено
	if(this->_prefix.empty()){
		// Выводим в лог сообщение об отсутствии начала имён переменных
		this->_log->print("Args: environment prefix is not set", log_t::flag_t::WARNING);
		// Выходим из метода, отбирать переменные не по чему
		return false;
	}
	// Получаем набор переменных окружения
	char ** items = environment();
	// Если набор переменных окружения не получен
	if(items == nullptr)
		// Выходим из метода, собирать нечего
		return false;
	// Получаем начало имён переменных окружения в верхнем регистре
	const string & prefix = this->_fmk->transform(this->_prefix, fmk_t::transform_t::UPPER_CASE);
	// Признак успешности укладки собранного
	bool result = true;
	// Выполняем перебор всего набора переменных окружения
	for(size_t i = 0; items[i] != nullptr; i++){
		// Получаем запись очередной переменной окружения
		const string_view record(items[i]);
		// Выполняем поиск разделителя имени переменной со значением
		const size_t pos = record.find('=');
		// Если разделитель имени со значением не найден
		if(pos == string_view::npos)
			// Продолжаем перебор переменных окружения дальше
			continue;
		// Получаем имя переменной окружения
		const string_view name(record.substr(0, pos));
		/**
		 * Отбираем переменные, имя которых начато отведённым приложению началом
		 * ВМЕСТЕ с подчёркиванием: без него начало «APP» брало бы и «APPLICATION»
		 */
		if((name.length() <= (prefix.length() + 1)) || (name.compare(0, prefix.length(), prefix) != 0) || (name.at(prefix.length()) != '_'))
			// Продолжаем перебор переменных окружения дальше
			continue;
		// Получаем остаток имени переменной без начала и подчёркивания за ним
		string path(name.substr(prefix.length() + 1));
		// Выполняем перевод остатка имени переменной в нижний регистр
		this->_fmk->transform(path, fmk_t::transform_t::LOWER_CASE);
		// Выполняем перебор всех знаков остатка имени переменной
		for(size_t j = 0; j < path.length(); j++){
			// Если знаком является подчёркивание
			if(path.at(j) == '_')
				// Выполняем замену его разделителем оси хранения
				path.replace(j, 1, 1, '/');
		}
		// Выполняем укладку выведенного значения переменной окружения
		result = (this->lay(path, this->derive(record.substr(pos + 1)), source_t::ENV) && result);
	}
	// Выводим результат сбора переменных окружения
	return result;
}

/**
 * @brief Метод установки значения по умолчанию
 *
 * @param key   имя параметра с разделителем звеньев
 * @param value запись значения по умолчанию
 * @return      результат установки
 *
 */
bool awh::args::Args::fallback(const string_view key, const string_view value) noexcept {
	// Выполняем укладку значения по умолчанию
	return this->lay(this->route(key), this->derive(value), source_t::DEFAULT);
}

/**
 * @brief Метод слияния дерева значений с деревом настроек
 *
 * @param value  сливаемое дерево значений
 * @param path   путь звеньями оси хранения, уже пройденный слиянием
 * @param source источник сливаемых значений
 * @return       результат слияния
 *
 */
bool awh::args::Args::merge(const codec::abc::value_t & value, const string & path, const source_t source) noexcept {
	// Если сливаемое значение недействительно вовсе
	if(!value.valid())
		// Выходим из метода, сливать нечего
		return false;
	// Если сливаемое значение является отображением
	if(value.is(codec::abc::type_t::MAP)){
		// Признак успешности слияния полей отображения
		bool result = true;
		// Выполняем перебор всех полей отображения
		for(size_t i = 0; i < value.size(); i++){
			// Извлекаемое имя поля отображения
			string name = "";
			// Если имя поля отображения знаками не выражается
			if(!value.key(i).value(name))
				// Продолжаем перебор полей отображения дальше
				continue;
			/**
			 * Выполняем слияние поля отображения вглубь: отображения сливаются
			 * звено за звеном, а вместимые и одиночные значения ложатся целиком
			 */
			result = (this->merge(value[i], (path.empty() ? name : this->_fmk->format("%s/%s", path.c_str(), name.c_str())), source) && result);
		}
		// Выводим результат слияния полей отображения
		return result;
	}
	// Выполняем укладку сливаемого значения целиком
	return this->lay(path, codec::abc::value_t(value), source);
}

/**
 * @brief Метод разбора записи настроек кодеком
 *
 * @param text запись настроек для разбора
 * @return     результат разбора
 *
 */
bool awh::args::Args::config(const string_view text) noexcept {
	// Собираемое дерево значений записи настроек
	codec::abc::value_t value;
	// Выполняем разбор записи настроек кодеком
	if(!this->_bridge.decode(text, value)){
		// Выполняем запоминание отказа разбора записи настроек
		this->_errors.emplace_back(error_t::CODEC, location_t());
		// Выходим из метода, разбор отвечен отказом
		return false;
	}
	// Выполняем слияние разобранного дерева с деревом настроек
	return this->merge(value, "", source_t::FILE);
}

/**
 * @brief Метод чтения файла настроек
 *
 * @param filename путь к файлу настроек
 * @return         результат чтения
 *
 */
bool awh::args::Args::filename(const string & filename) noexcept {
	// Если файла настроек нет вовсе
	if(this->_fs.type(filename) != fs_t::type_t::FILE){
		// Выполняем запоминание отказа чтения файла настроек
		this->_errors.emplace_back(error_t::FILESYSTEM, location_t());
		// Выводим в лог сообщение об отсутствии файла настроек
		this->_log->print("Args: %s \"%s\"", log_t::flag_t::WARNING, args::message(error_t::FILESYSTEM), filename.c_str());
		// Выходим из метода, читать нечего
		return false;
	}
	// Прочитанная запись файла настроек
	string text = "";
	// Выполняем чтение файла настроек целиком
	this->_fs.read(filename, text);
	// Если запись файла настроек пуста вовсе
	if(text.empty()){
		// Выполняем запоминание отказа чтения файла настроек
		this->_errors.emplace_back(error_t::FILESYSTEM, location_t());
		// Выходим из метода, читать нечего
		return false;
	}
	// Выполняем разбор прочитанной записи настроек
	return this->config(text);
}

/**
 * @brief Метод выдачи дерева настроек записью кодека
 *
 * @param result собранная запись настроек
 * @return       результат выдачи
 *
 */
bool awh::args::Args::dump(string & result) noexcept {
	// Выполняем перевод дерева настроек в запись кодека
	if(!this->_bridge.encode(this->_root, result)){
		// Выполняем запоминание отказа выдачи записи настроек
		this->_errors.emplace_back(error_t::UNSUPPORTED, location_t());
		// Выходим из метода, выдача отвечена отказом
		return false;
	}
	// Сообщаем, что выдача записи настроек выполнена
	return true;
}

/**
 * @brief Метод записи дерева настроек в файл
 *
 * @param filename путь к файлу настроек
 * @return         результат записи
 *
 */
bool awh::args::Args::save(const string & filename) noexcept {
	// Собираемая запись настроек
	string text = "";
	// Выполняем выдачу дерева настроек записью кодека
	if(!this->dump(text))
		// Выходим из метода, выдача отвечена отказом
		return false;
	// Выполняем запись собранной записи настроек в файл
	this->_fs.write(filename, text.data(), text.size());
	// Сообщаем, что запись дерева настроек выполнена
	return true;
}

/**
 * @brief Метод извлечения моста между контейнером ABC и текстовыми кодеками
 *
 * @return мост между контейнером ABC и текстовыми кодеками
 *
 */
awh::codec::bridge_t & awh::args::Args::bridge() noexcept {
	// Выводим мост между контейнером ABC и текстовыми кодеками
	return this->_bridge;
}

/**
 * @brief Метод проверки наличия параметра
 *
 * @param key имя параметра с разделителем звеньев
 * @return    результат проверки
 *
 */
bool awh::args::Args::has(const string_view key) const noexcept {
	// Выводим признак наличия значения по пути оси хранения
	return this->_root.at(this->route(key)).valid();
}

/**
 * @brief Метод извлечения источника значения параметра
 *
 * @param key имя параметра с разделителем звеньев
 * @return    источник значения параметра
 *
 */
source_t awh::args::Args::source(const string_view key) const noexcept {
	// Выполняем поиск источника уложенного значения
	auto i = this->_origins.find(this->route(key));
	// Если источник уложенного значения найден
	if(i != this->_origins.end())
		// Выводим источник уложенного значения
		return i->second;
	// Выводим неопределённый источник значения
	return source_t::NONE;
}

/**
 * @brief Метод извлечения числа значений вместимого параметра
 *
 * @param key имя параметра с разделителем звеньев
 * @return    число значений вместимого
 *
 */
size_t awh::args::Args::size(const string_view key) const noexcept {
	// Выводим число значений вместимого по пути оси хранения
	return this->_root.at(this->route(key)).size();
}

/**
 * @brief Шаблон метода извлечения значения параметра
 *
 * @tparam T тип извлекаемого значения
 * @param key имя параметра с разделителем звеньев
 * @return    извлечённое значение параметра
 *
 */
template <typename T>
T awh::args::Args::get(const string_view key) const noexcept {
	// Получаем значение по пути оси хранения
	const codec::abc::value_t & value = this->_root.at(this->route(key));
	// Если извлекается последовательность знаков
	if constexpr(is_same <T, string>::value){
		// Извлекаемая последовательность знаков
		string result = "";
		// Выполняем извлечение последовательности знаков
		if(value.value(result))
			// Выводим извлечённую последовательность знаков
			return result;
		// Выводим пустую последовательность знаков
		return string("");
	// Если извлекается логическое значение
	} else if constexpr(is_same <T, bool>::value) {
		// Извлекаемое логическое значение
		bool result = false;
		// Выполняем извлечение логического значения
		if(value.value(result))
			// Выводим извлечённое логическое значение
			return result;
		/**
		 * Если значение уложено последовательностью знаков, оно приводится к
		 * логическому: вывод вида значения отключается настройкою, и потребитель
		 * от этого извлечения лишаться не должен
		 */
		string text = "";
		// Выполняем извлечение последовательности знаков
		if(value.value(text))
			// Выводим признак совпадения записи с истиной
			return (this->_fmk->compare(text, "true") || this->_fmk->compare(text, "yes") || this->_fmk->compare(text, "on") || this->_fmk->compare(text, "1"));
		// Выводим логическое значение ложью
		return false;
	// Если извлекается число
	} else {
		// Извлекаемое число дробное
		double real = 0.;
		// Если значение уложено дробным числом
		if(value.is(codec::abc::type_t::REAL) && value.value(real))
			// Выводим извлечённое число дробное
			return static_cast <T> (real);
		// Если извлекается число дробное
		if constexpr(is_floating_point <T>::value) {
			// Извлекаемое число целое со знаком
			int64_t number = 0;
			// Если значение уложено целым числом
			if(value.is(codec::abc::type_t::INT) && value.value(number))
				// Выводим извлечённое число целое
				return static_cast <T> (number);
		// Если извлекается число целое без знака
		} else if constexpr(is_unsigned <T>::value) {
			// Извлекаемое число целое без знака
			uint64_t number = 0;
			// Если значение уложено целым числом без знака
			if(value.value(number))
				// Выводим извлечённое число целое без знака
				return static_cast <T> (number);
		// Если извлекается число целое со знаком
		} else {
			// Извлекаемое число целое со знаком
			int64_t number = 0;
			// Если значение уложено целым числом со знаком
			if(value.value(number))
				// Выводим извлечённое число целое со знаком
				return static_cast <T> (number);
		}
		// Извлекаемая последовательность знаков
		string text = "";
		/**
		 * Если значение уложено последовательностью знаков, оно приводится к числу:
		 * вывод вида значения отключается настройкою, и потребитель от этого
		 * извлечения лишаться не должен
		 */
		if(value.value(text))
			// Выводим число, разобранное из последовательности знаков
			return this->_fmk->atoi <T> (text);
		// Выводим число нулём
		return static_cast <T> (0);
	}
}

/**
 * @brief Шаблон метода извлечения значений вместимого параметра
 *
 * @tparam T тип извлекаемых значений
 * @param key имя параметра с разделителем звеньев
 * @return    извлечённые значения вместимого
 *
 */
template <typename T>
vector <T> awh::args::Args::arr(const string_view key) const noexcept {
	// Контейнер извлечённых значений вместимого
	vector <T> result;
	// Выполняем перевод имени параметра в путь оси хранения
	const string & path = this->route(key);
	// Получаем значение по пути оси хранения
	const codec::abc::value_t & value = this->_root.at(path);
	// Если значение вместимым не является вовсе
	if(!value.is(codec::abc::type_t::ARRAY)){
		/**
		 * Если значение уложено одиночным, оно выдаётся вместимым из него одного:
		 * параметр, поданный единожды, и параметр, поданный дважды, потребителю
		 * вместимого различаться не должны
		 */
		if(value.valid())
			// Добавляем одиночное значение в контейнер извлечённых
			result.push_back(this->get <T> (key));
		// Выводим контейнер извлечённых значений вместимого
		return result;
	}
	// Выполняем резервирование памяти под извлекаемые значения
	result.reserve(value.size());
	// Выполняем перебор всех значений вместимого
	for(size_t i = 0; i < value.size(); i++)
		// Добавляем извлечённое значение вместимого в контейнер
		result.push_back(this->get <T> (this->_fmk->format("%s%c%zu", string(key).c_str(), this->_settings.delimiter, i)));
	// Выводим контейнер извлечённых значений вместимого
	return result;
}

/**
 * @brief Метод извлечения позиционных доводов набора запуска
 *
 * @return позиционные доводы в порядке их встречи
 *
 */
const vector <string> & awh::args::Args::operands() const noexcept {
	// Выводим позиционные доводы набора запуска
	return this->_operands;
}

/**
 * @brief Метод извлечения дерева собранных значений настроек
 *
 * @return дерево собранных значений настроек
 *
 */
const awh::codec::abc::value_t & awh::args::Args::root() const noexcept {
	// Выводим дерево собранных значений настроек
	return this->_root;
}

/**
 * @brief Метод извлечения отказов последнего разбора
 *
 * @return отказы, случившиеся при последнем разборе
 *
 */
const vector <pair <error_t, location_t>> & awh::args::Args::errors() const noexcept {
	// Выводим отказы последнего разбора
	return this->_errors;
}

/**
 * @brief Метод извлечения начала имён переменных окружения
 *
 * @return начало имён переменных окружения
 *
 */
const string & awh::args::Args::prefix() const noexcept {
	// Выводим начало имён переменных окружения
	return this->_prefix;
}

/**
 * @brief Метод установки начала имён переменных окружения
 *
 * @param prefix начало имён переменных окружения
 *
 */
void awh::args::Args::prefix(const string_view prefix) noexcept {
	// Устанавливаем начало имён переменных окружения
	this->_prefix.assign(prefix);
}

/**
 * @brief Метод извлечения настроек сбора параметров запуска
 *
 * @return настройки сбора параметров запуска
 *
 */
const Args::settings_t & awh::args::Args::settings() const noexcept {
	// Выводим настройки сбора параметров запуска
	return this->_settings;
}

/**
 * @brief Метод установки настроек сбора параметров запуска
 *
 * @param settings настройки сбора параметров запуска
 *
 */
void awh::args::Args::settings(const settings_t & settings) noexcept {
	// Устанавливаем настройки сбора параметров запуска
	this->_settings = settings;
}

/**
 * @brief Метод извлечения настроек разбора параметров
 *
 * @return настройки разбора параметров
 *
 */
const Lexer::settings_t & awh::args::Args::lexing() const noexcept {
	// Выводим настройки разбора параметров
	return this->_lexer.settings();
}

/**
 * @brief Метод установки настроек разбора параметров
 *
 * @param settings настройки разбора параметров
 *
 */
void awh::args::Args::lexing(const lexer_t::settings_t & settings) noexcept {
	// Устанавливаем настройки разбора параметров
	this->_lexer.settings(settings);
}

/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
awh::args::Args::Args(const fmk_t * fmk, const log_t * log) noexcept :
 _prefix{""}, _lexer(fmk, log), _bridge(log), _fs(fmk, log),
 _root(codec::abc::kind_t::MAP), _fmk(fmk), _log(log) {
	// Выполняем установку объекта работы с логами дереву настроек
	this->_root.setLogger(log);
}

/**
 * Выполняем объявление извлечений всех поддержанных видов
 */
template bool awh::args::Args::get <bool> (const string_view) const noexcept;
template int8_t awh::args::Args::get <int8_t> (const string_view) const noexcept;
template uint8_t awh::args::Args::get <uint8_t> (const string_view) const noexcept;
template int16_t awh::args::Args::get <int16_t> (const string_view) const noexcept;
template uint16_t awh::args::Args::get <uint16_t> (const string_view) const noexcept;
template int32_t awh::args::Args::get <int32_t> (const string_view) const noexcept;
template uint32_t awh::args::Args::get <uint32_t> (const string_view) const noexcept;
template int64_t awh::args::Args::get <int64_t> (const string_view) const noexcept;
template uint64_t awh::args::Args::get <uint64_t> (const string_view) const noexcept;
template float awh::args::Args::get <float> (const string_view) const noexcept;
template double awh::args::Args::get <double> (const string_view) const noexcept;
template std::string awh::args::Args::get <std::string> (const string_view) const noexcept;

/**
 * Выполняем объявление извлечений вместимого всех поддержанных видов
 */
template vector <bool> awh::args::Args::arr <bool> (const string_view) const noexcept;
template vector <int8_t> awh::args::Args::arr <int8_t> (const string_view) const noexcept;
template vector <uint8_t> awh::args::Args::arr <uint8_t> (const string_view) const noexcept;
template vector <int16_t> awh::args::Args::arr <int16_t> (const string_view) const noexcept;
template vector <uint16_t> awh::args::Args::arr <uint16_t> (const string_view) const noexcept;
template vector <int32_t> awh::args::Args::arr <int32_t> (const string_view) const noexcept;
template vector <uint32_t> awh::args::Args::arr <uint32_t> (const string_view) const noexcept;
template vector <int64_t> awh::args::Args::arr <int64_t> (const string_view) const noexcept;
template vector <uint64_t> awh::args::Args::arr <uint64_t> (const string_view) const noexcept;
template vector <float> awh::args::Args::arr <float> (const string_view) const noexcept;
template vector <double> awh::args::Args::arr <double> (const string_view) const noexcept;
template vector <std::string> awh::args::Args::arr <std::string> (const string_view) const noexcept;
