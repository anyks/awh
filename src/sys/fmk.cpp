/**
 * @file: fmk.cpp
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
#include <sys/fmk.hpp>

/**
 * Устанавливаем шаблон функции
 */
template <typename T>
/**
 * decimalPlaces Функция определения количества знаков после запятой
 * @param number число в котором нужно определить количество знаков
 * @return       количество знаков после запятой
 */
enable_if_t <(is_floating_point <T>::value), size_t> decimalPlaces(T number) noexcept {
	// Устанавливаем делитель
	T factor = 10;
	// Количество знаков после запятой
	size_t count = 0;
	// Убираем знак числа
	number = abs(number);
	// Выполняем округление в меньшую сторону
	auto c = (number - floor(number));
	// Выполняем умножение на 10 до тех пор, пока не переберём все числа
	while((c > 0) && (count < numeric_limits <T>::max_digits10)){
		// Получаем число
		c = (number * factor);
		// Выполняем округление в меньшую сторону
		c = (c - floor(c));
		// Увеличиваем множитель
		factor *= 10;
		// Считаем количество чисел
		count++;
	}
	// Выводим результат
	return count;
}
/**
 * Устанавливаем шаблон функции
 */
template <typename T>
/**
 * split Метод разделения строк на составляющие
 * @param str   строка для поиска
 * @param delim разделитель
 * @param v     результирующий вектор
 */
void split(const wstring & str, const wstring & delim, T & v) noexcept {
	/**
	 * trimFn Метод удаления пробелов вначале и конце текста
	 * @param text текст для удаления пробелов
	 * @return     результат работы функции
	 */
	function <const wstring (const wstring &)> trimFn = [](const wstring & text) noexcept {
		// Получаем временный текст
		wstring tmp = text;
		// Выполняем удаление пробелов по краям
		tmp.erase(tmp.begin(), find_if_not(tmp.begin(), tmp.end(), [](wchar_t c){ return iswspace(c); }));
		tmp.erase(find_if_not(tmp.rbegin(), tmp.rend(), [](wchar_t c){ return iswspace(c); }).base(), tmp.end());
		// Выводим результат
		return tmp;
	};
	// Очищаем словарь
	v.clear();
	// Получаем счётчики перебора
	size_t i = 0, j = str.find(delim);
	const size_t len = delim.length();
	// Выполняем разбиение строк
	while(j != wstring::npos){
		v.insert(v.end(), trimFn(str.substr(i, j - i)));
		i = ++j + (len - 1);
		j = str.find(delim, j);
		if(j == wstring::npos) v.insert(v.end(), trimFn(str.substr(i, str.length())));
	}
	// Если слово передано а вектор пустой, тогда создаем вектори из 1-го элемента
	if(!str.empty() && v.empty()) v.insert(v.end(), trimFn(str));
}
/**
 * trim Метод удаления пробелов вначале и конце текста
 * @param text текст для удаления пробелов
 * @return     результат работы функции
 */
string awh::Framework::trim(const string & text) const noexcept {
	// Получаем временный текст
	string tmp = text;
	// Выполняем удаление пробелов по краям
	tmp.erase(tmp.begin(), find_if_not(tmp.begin(), tmp.end(), [](char c){ return isspace(c); }));
	tmp.erase(find_if_not(tmp.rbegin(), tmp.rend(), [](char c){ return isspace(c); }).base(), tmp.end());
	// Выводим результат
	return tmp;
}
/**
 * convert Метод конвертирования строки utf-8 в строку
 * @param str строка utf-8 для конвертирования
 * @return    обычная строка
 */
string awh::Framework::convert(const wstring & str) const noexcept {
	// Результат работы функции
	string result = "";
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если строка передана
		if(!str.empty()){
			// Если используется BOOST
			#ifdef USE_BOOST_CONVERT
				// Объявляем конвертер
				using boost::locale::conv::utf_to_utf;
				// Выполняем конвертирование в utf-8 строку
				result = utf_to_utf <char> (str.c_str(), str.c_str() + str.size());
			// Если нужно использовать стандартную библиотеку
			#else
				// Устанавливаем тип для конвертера UTF-8
				using convert_type = codecvt_utf8 <wchar_t, 0x10ffff, little_endian>;
				// Объявляем конвертер
				wstring_convert <convert_type, wchar_t> conv;
				// wstring_convert <codecvt_utf8 <wchar_t>> conv;
				// Выполняем конвертирование в utf-8 строку
				result = conv.to_bytes(str);
			#endif
		}
	// Если возникает ошибка
	} catch(const range_error & error) {
		/* Пропускаем возникшую ошибку */
	}
	// Выводим результат
	return result;
}
/**
 * toLower Метод перевода русских букв в нижний регистр
 * @param str строка для перевода
 * @return    строка в нижнем регистре
 */
string awh::Framework::toLower(const string & str) const noexcept {
	// Результат работы функции
	string result = str;
	// Если строка передана
	if(!str.empty()){
		// Получаем временную строку
		wstring tmp = this->convert(result);
		// Если конвертация прошла успешно
		if(!tmp.empty()){
			// Выполняем приведение к нижнему регистру
			transform(tmp.begin(), tmp.end(), tmp.begin(), [](wchar_t c){
				// Приводим к нижнему регистру каждую букву
				return towlower(c);
			});
			// Конвертируем обратно
			result = this->convert(tmp);
		}
	}
	// Выводим результат
	return result;
}
/**
 * toUpper Метод перевода русских букв в верхний регистр
 * @param str строка для перевода
 * @return    строка в верхнем регистре
 */
string awh::Framework::toUpper(const string & str) const noexcept {
	// Результат работы функции
	string result = str;
	// Если строка передана
	if(!str.empty()){
		// Получаем временную строку
		wstring tmp = this->convert(result);
		// Если конвертация прошла успешно
		if(!tmp.empty()){
			// Выполняем приведение к верхнему регистру
			transform(tmp.begin(), tmp.end(), tmp.begin(), [](wchar_t c){
				// Приводим к верхнему регистру каждую букву
				return towupper(c);
			});
			// Конвертируем обратно
			result = this->convert(tmp);
		}
	}
	// Выводим результат
	return result;
}
/**
 * smartUpper Метод умного перевода символов в верхний регистр
 * @param str строка для перевода
 * @return    строка в верхнем регистре
 */
string awh::Framework::smartUpper(const string & str) const noexcept {
	// Результат работы функции
	string result = str;
	// Если строка передана
	if(!str.empty()){
		// Получаем временную строку
		const wstring & tmp = this->convert(result);
		// Если конвертация прошла успешно
		if(!tmp.empty())
			// Выполняем перевод символов в верхний регистр
			result = this->convert(this->smartUpper(tmp));
	}
	// Выводим результат
	return result;
}
/**
 * decToHex Метод конвертации 10-го числа в 16-е
 * @param number число для конвертации
 * @return       результат конвертации
 */
string awh::Framework::decToHex(const size_t number) const noexcept {
	// Результат работы функции
	string result = "0";
	// Если число передано
	if(number > 0){
		// Создаём поток для конвертации
		stringstream stream;
		// Записываем число в поток
		stream << hex << number;
		// Получаем результат в верхнем регистре
		result = this->toUpper(stream.str());
	}
	// Выводим результат
	return result;
}
/**
 * hexToDec Метод конвертации 16-го числа в 10-е
 * @param number число для конвертации
 * @return       результат конвертации
 */
size_t awh::Framework::hexToDec(const string & number) const noexcept {
	// Результат работы функции
	size_t result = 0;
	// Если 16-е число передано
	if(!number.empty()){
		// Создаём поток для конвертации
		stringstream stream;
		// Записываем число в поток
		stream << hex << number;
		// Получаем результат
		stream >> result;
		// Выполняем конвертацию числа
		// result = stoull(number, nullptr, 16);
	}
	// Выводим результат
	return result;
}
/**
 * noexp Метод перевода числа в безэкспоненциальную форму
 * @param number число для перевода
 * @param step   размер шага после запятой
 * @return       число в безэкспоненциальной форме
 */
string awh::Framework::noexp(const double number, const double step) const noexcept {
	// Результат работы функции
	string result = "";
	// Если размер шага и число переданы
	if((number > 0.0) && (step > 0.0)){
		// Создаём поток для конвертации числа
		stringstream stream;
		// Получаем размер шага
		const u_short size = (u_short) abs(log10(step));
		// Записываем число в поток
		stream << fixed << setprecision(size) << number;
		// Получаем из потока строку
		stream >> result;
		// Выполняем конвертацию числа
		wstring number = this->convert(result);
		// Если конвертация прошла успешно
		if(!number.empty()){
			// Переходим по всему числу
			for(auto it = number.begin(); it != number.end();){
				// Если это первый символ
				if(it == number.begin() && ((* it) == L'-')) ++it;
				// Проверяем является ли символ числом
				else if((this->numsSymbols.arabs.count(* it) > 0) || ((* it) == L'.')) ++it;
				// Иначе удаляем символ
				else it = number.erase(it);
			}
			// Запоминаем полученный результат
			result = this->convert(number);
		}
	}
	// Выводим результат
	return result;
}
/**
 * noexp Метод перевода числа в безэкспоненциальную форму
 * @param number  число для перевода
 * @param onlyNum выводить только числа
 * @return        число в безэкспоненциальной форме
 */
string awh::Framework::noexp(const double number, const bool onlyNum) const noexcept {
	// Результат работы функции
	string result = "";
	// Создаём поток для конвертации числа
	stringstream stream;
	// Получаем количество знаков после запятой
	const size_t count = decimalPlaces <double> (number);
	// Записываем число в поток
	stream << fixed << setprecision(count) << number;
	// Получаем из потока строку
	stream >> result;
	// Если количество цифр после запятой больше нуля
	if((count > 0) && (result.size() > 2)){
		// Устанавливаем значение последнего символа
		char last = '$';
		// Выполняем перебор всех символов
		for(auto it = (result.end() - 2); it != (result.begin() - 1);){
			// Если символ не является последним
			if(it != (result.end() - 2)){
				// Если символы совпадают, удаляем текущий
				if((* it) == last) result.erase(it);
				// Если символы не совпадат, выходим
				else break;
			}
			// Запоминаем текущее значение символа
			last = (* it);
			// Уменьшаем значение итератора
			it--;
		}
	}
	// Если нужно выводить только числа
	if(onlyNum){
		// Выполняем конвертацию числа
		wstring number = this->convert(result);
		// Если конвертация прошла успешно
		if(!number.empty()){
			// Переходим по всему числу
			for(auto it = number.begin(); it != number.end();){
				// Если это первый символ
				if(it == number.begin() && ((* it) == L'-')) ++it;
				// Проверяем является ли символ числом
				else if((this->numsSymbols.arabs.count(* it) > 0) || ((* it) == L'.')) ++it;
				// Иначе удаляем символ
				else it = number.erase(it);
			}
			// Запоминаем полученный результат
			result = this->convert(number);
		}
	}
	// Выводим результат
	return result;
}
/**
 * format Метод реализации функции формирования форматированной строки
 * @param format формат строки вывода
 * @param args   передаваемые аргументы
 * @return       сформированная строка
 */
string awh::Framework::format(const char * format, ...) const noexcept {
	// Результат работы функции
	string result = "";
	// Если формат передан
	if(format != nullptr){
		// Создаем список аргументов
		va_list args;
		// Создаем буфер
		vector <char> buffer(BUFFER_SIZE);
		// Заполняем буфер нулями
		memset(buffer.data(), 0, buffer.size());
		// Устанавливаем начальный список аргументов
		va_start(args, format);
		// Выполняем запись в буфер
		const size_t size = vsprintf(buffer.data(), format, args);
		// Завершаем список аргументов
		va_end(args);
		// Если размер не нулевой
		if(size > 0) result.assign(buffer.data(), size);
	}
	// Выводим результат
	return result;
}
/**
 * format Метод реализации функции формирования форматированной строки
 * @param  format формат строки вывода
 * @param  items  список аргументов строки
 * @return        сформированная строка
 */
string awh::Framework::format(const string & format, const vector <string> & items) const noexcept {
	// Результат работы функции
	string result = format;
	// Если данные переданы
	if(!format.empty() && !items.empty()){
		/**
		 * replaceFn Функция заменты подстроки в строке
		 * @param str  строка в которой нужно произвести замену
		 * @param from строка которую нужно заменить
		 * @param to   строка на которую нужно заменить
		 */
		auto replaceFn = [](string & str, const string & from, const string & to) noexcept {
			// Если строка пустая, выходим
			if(from.empty() || to.empty()) return;
			// Позиция подстроки в строке
			size_t pos = 0;
			// Выполняем поиск подстроки в стркое
			while((pos = str.find(from, pos)) != string::npos){
				// Заменяем подстроку в строке
				str.replace(pos, from.length(), to);
				// Увеличиваем позицию для поиска в строке
				pos += to.length();
			}
		};
		// Индекс в массиве
		u_short index = 1;
		// Исправляем возврат каретки
		replaceFn(result, "\\r", "\r");
		// Исправляем перенос строки
		replaceFn(result, "\\n", "\n");
		// Исправляем табуляцию
		replaceFn(result, "\\t", "\t");
		// Перебираем весь список аргументов
		for(auto & item : items){
			// Выполняем замену индекса аргумента на указанный аргумент
			replaceFn(result, "$" + to_string(index), item);
			// Увеличиваем значение индекса аргумента
			index++;
		}
	}
	// Выводим результат
	return result;
}
/**
 * toLower Метод перевода русских букв в нижний регистр
 * @param str строка для перевода
 * @return    строка в нижнем регистре
 */
char awh::Framework::toLower(const char letter) const noexcept {
	// Результат работы функции
	char result = 0;
	// Если строка передана
	if(letter > 0){
		// Выполняем конвертирование в utf-8 строку
		const wstring & tmp = this->convert(string{1, letter});
		// Если конвертация прошла успешно
		if(!tmp.empty()){
			// Строка для конвертации
			wstring str = L"";
			// Получаем первый символ строки
			const wchar_t c = tmp.front();
			// Формируем новую строку
			str.assign(1, towlower(c));
			// Выполняем конвертирование в utf-8 строку
			result = this->convert(str).front();
		}
	}
	// Выводим результат
	return result;
}
/**
 * toUpper Метод перевода русских букв в верхний регистр
 * @param str строка для перевода
 * @return    строка в верхнем регистре
 */
char awh::Framework::toUpper(const char letter) const noexcept {
	// Результат работы функции
	char result = 0;
	// Если строка передана
	if(letter > 0){
		// Выполняем конвертирование в utf-8 строку
		const wstring & tmp = this->convert(string{1, letter});
		// Если конвертация прошла успешно
		if(!tmp.empty()){
			// Строка для конвертации
			wstring str = L"";
			// Получаем первый символ строки
			const wchar_t c = tmp.front();
			// Формируем новую строку
			str.assign(1, towupper(c));
			// Выполняем конвертирование в utf-8 строку
			result = this->convert(str).front();
		}
	}
	// Выводим результат
	return result;
}
/**
 * toLower Метод перевода русских букв в нижний регистр
 * @param str строка для перевода
 * @return    строка в нижнем регистре
 */
wchar_t awh::Framework::toLower(const wchar_t letter) const noexcept {
	// Результат работы функции
	wchar_t result = 0;
	// Если строка передана
	if(letter > 0) result = towlower(letter);
	// Выводим результат
	return result;
}
/**
 * toUpper Метод перевода русских букв в верхний регистр
 * @param str строка для перевода
 * @return    строка в верхнем регистре
 */
wchar_t awh::Framework::toUpper(const wchar_t letter) const noexcept {
	// Результат работы функции
	wchar_t result = 0;
	// Если строка передана
	if(letter > 0) result = towupper(letter);
	// Выводим результат
	return result;
}
/**
 * trim Метод удаления пробелов вначале и конце текста
 * @param text текст для удаления пробелов
 * @return     результат работы функции
 */
wstring awh::Framework::trim(const wstring & text) const noexcept {
	// Получаем временный текст
	wstring tmp = text;
	// Выполняем удаление пробелов по краям
	tmp.erase(tmp.begin(), find_if_not(tmp.begin(), tmp.end(), [this](wchar_t c){ return this->isSpace(c); }));
	tmp.erase(find_if_not(tmp.rbegin(), tmp.rend(), [this](wchar_t c){ return this->isSpace(c); }).base(), tmp.end());
	// Выводим результат
	return tmp;
}
/**
 * convert Метод конвертирования строки в строку utf-8
 * @param str строка для конвертирования
 * @return    строка в utf-8
 */
wstring awh::Framework::convert(const string & str) const noexcept {
		// Результат работы функции
	wstring result = L"";
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если строка передана
		if(!str.empty()){
			// Если используется BOOST
			#ifdef USE_BOOST_CONVERT
				// Объявляем конвертер
				using boost::locale::conv::utf_to_utf;
				// Выполняем конвертирование в utf-8 строку
				result = utf_to_utf <wchar_t> (str.c_str(), str.c_str() + str.size());
			// Если нужно использовать стандартную библиотеку
			#else
				// Объявляем конвертер
				// wstring_convert <codecvt_utf8 <wchar_t>> conv;
				wstring_convert <codecvt_utf8_utf16 <wchar_t, 0x10ffff, little_endian>> conv;
				// Выполняем конвертирование в utf-8 строку
				result = conv.from_bytes(str);
			#endif
		}
	// Если возникает ошибка
	} catch(const range_error & error) {
		/* Пропускаем возникшую ошибку */
	}
	// Выводим результат
	return result;
}
/**
 * toLower Метод перевода русских букв в нижний регистр
 * @param str строка для перевода
 * @return    строка в нижнем регистре
 */
wstring awh::Framework::toLower(const wstring & str) const noexcept {
	// Результат работы функции
	wstring result = L"";
	// Если строка передана
	if(!str.empty()){
		// Переходим по всем буквам слова и формируем новую строку
		for(auto & c : str) result.append(1, towlower(c));
	}
	// Выводим результат
	return result;
}
/**
 * toUpper Метод перевода русских букв в верхний регистр
 * @param str строка для перевода
 * @return    строка в верхнем регистре
 */
wstring awh::Framework::toUpper(const wstring & str) const noexcept {
	// Результат работы функции
	wstring result = L"";
	// Если строка передана
	if(!str.empty()){
		// Переходим по всем буквам слова и формируем новую строку
		for(auto & c : str) result.append(1, towupper(c));
	}
	// Выводим результат
	return result;
}
/**
 * smartUpper Метод умного перевода символов в верхний регистр
 * @param str строка для перевода
 * @return    строка в верхнем регистре
 */
wstring awh::Framework::smartUpper(const wstring & str) const noexcept {
	// Результат работы функции
	wstring result = L"";
	// Если строка передана
	if(!str.empty()){
		// Флаг детекции символа
		bool mode = true;
		// Переходим по всем буквам слова и формируем новую строку
		for(auto & c : str){
			// Если флаг установлен
			if(mode) result.append(1, towupper(c));
			// Иначе добавляем символ как есть
			else result.append(1, c);
			// Если найден спецсимвол, устанавливаем флаг детекции
			mode = ((c == L'-') || (c == L'_') || this->isSpace(c));
		}
	}
	// Выводим результат
	return result;
}
/**
 * arabic2Roman Метод перевода арабских чисел в римские
 * @param number арабское число от 1 до 4999
 * @return       римское число
 */
wstring awh::Framework::arabic2Roman(const u_int number) const noexcept {
		// Результат работы функции
	wstring result = L"";
	// Если число передано верное
	if((number >= 1) && (number <= 4999)){
		// Копируем полученное число
		u_int n = number;
		// Вычисляем до тысяч
		result.append(this->numsSymbols.m[floor(n / 1000)]);
		// Уменьшаем диапазон
		n %= 1000;
		// Вычисляем до сотен
		result.append(this->numsSymbols.c[floor(n / 100)]);
		// Вычисляем до сотен
		n %= 100;
		// Вычисляем до десятых
		result.append(this->numsSymbols.x[floor(n / 10)]);
		// Вычисляем до сотен
		n %= 10;
		// Формируем окончательный результат
		result.append(this->numsSymbols.i[n]);
	}
	// Выводим результат
	return result;
}
/**
 * arabic2Roman Метод перевода арабских чисел в римские
 * @param word арабское число от 1 до 4999
 * @return     римское число
 */
wstring awh::Framework::arabic2Roman(const wstring & word) const noexcept {
	// Результат работы функции
	wstring result = L"";
	// Если слово передано
	if(!word.empty()){
		// Преобразуем слово в число
		const u_int number = stoi(word);
		// Выполняем расчет
		result.assign(this->arabic2Roman(number));
	}
	// Выводим результат
	return result;
}
/**
 * replace Метод замены в тексте слово на другое слово
 * @param text текст в котором нужно произвести замену
 * @param word слово для поиска
 * @param alt  слово на которое нужно произвести замену
 * @return     результирующий текст
 */
wstring awh::Framework::replace(const wstring & text, const wstring & word, const wstring & alt) const noexcept {
	// Результат работы функции
	wstring result = move(text);
	// Если текст передан и искомое слово не равно слову для замены
	if(!result.empty() && !word.empty() && (word.compare(alt) != 0)){
		// Позиция искомого текста
		size_t pos = 0;
		// Определяем текст на который нужно произвести замену
		const wstring & alternative = (!alt.empty() ? alt : L"");
		// Выполняем поиск всех слов
		while((pos = result.find(word, pos)) != wstring::npos){
			// Выполняем замену текста
			result.replace(pos, word.length(), alternative);
			// Смещаем позицию на единицу
			pos++;
		}
	}
	// Выводим результат
	return result;
}
/**
 * roman2Arabic Метод перевода римских цифр в арабские
 * @param word римское число
 * @return     арабское число
 */
u_short awh::Framework::roman2Arabic(const wstring & word) const noexcept {
	// Результат работы функции
	u_short result = 0;
	// Если слово передано
	if(!word.empty()){
		// Символ поиска
		wchar_t c, o;
		// Вспомогательные переменные
		u_int i = 0, v = 0, n = 0;
		// Получаем длину слова
		const size_t length = word.length();
		// Если слово состоит всего из одной буквы
		if((length == 1) && (this->numsSymbols.roman.count(word.front()) < 1)) return result;
		// Если слово длиннее одной буквы
		else {
			// Переходим по всем буквам слова
			for(size_t i = 0, j = (length - 1); j > ((length / 2) - 1); i++, j--){
				// Проверяем является ли слово римским числом
				if(!(i == j ? (this->numsSymbols.roman.count(word.at(i)) > 0) :
					(this->numsSymbols.roman.count(word.at(i)) > 0) &&
					(this->numsSymbols.roman.count(word.at(j)) > 0)
				)) return result;
			}
		}
		// Преобразовываем цифру M
		if(word.front() == L'm'){
			for(n = 0; word[i] == L'm'; n++) i++;
			if(n > 4) return 0;
			v += n * 1000;
		}
		o = word[i];
		// Преобразовываем букву D и C
		if((o == L'd') || (o == L'c')){
			if((c = o) == L'd'){
				i++;
				v += 500;
			}
			o = word[i + 1];
			if((c == L'c') && (o == L'm')){
				i += 2;
				v += 900;
			} else if((c == L'c') && (o == L'd')) {
				i += 2;
				v += 400;
			} else {
				for(n = 0; word[i] == L'c'; n++) i++;
				if(n > 4) return 0;
				v += n * 100;
			}
		}
		o = word[i];
		// Преобразовываем букву L и X
		if((o == L'l') || (o == L'x')){
			if((c = o) == L'l'){
				i++;
				v += 50;
			}
			o = word[i + 1];
			if((c == L'x') && (o == L'c')){
				i += 2;
				v += 90;
			} else if((c == L'x') && (o == L'l')) {
				i += 2;
				v += 40;
			} else {
				for(n = 0; word[i] == L'x'; n++) i++;
				if(n > 4) return 0;
				v += n * 10;
			}
		}
		o = word[i];
		// Преобразовываем букву V и I
		if((o == L'v') || (o == L'i')){
			if((c = o) == L'v'){
				i++;
				v += 5;
			}
			o = word[i + 1];
			if((c == L'i') && (o == L'x')){
				i += 2;
				v += 9;
			} else if((c == L'i') && (o == L'v')){
				i += 2;
				v += 4;
			} else {
				for(n = 0; word[i] == L'i'; n++) i++;
				if(n > 4) return 0;
				v += n;
			}
		}
		// Формируем реузльтат
		result = (((word.length() == i) && (v >= 1) && (v <= 4999)) ? v : 0);
	}
	// Выводим результат
	return result;
}
/**
 * setCase Метод запоминания регистра слова
 * @param pos позиция для установки регистра
 * @param cur текущее значение регистра в бинарном виде
 * @return    позиция верхнего регистра в бинарном виде
 */
size_t awh::Framework::setCase(const size_t pos, const size_t cur) const noexcept {
	// Результат работы функции
	size_t result = cur;
	// Если позиция передана и длина слова тоже
	result += (1 << pos);
	// Выводим результат
	return result;
}
/**
 * countLetter Метод подсчета количества указанной буквы в слове
 * @param word   слово в котором нужно подсчитать букву
 * @param letter букву которую нужно подсчитать
 * @return       результат подсчёта
 */
size_t awh::Framework::countLetter(const wstring & word, const wchar_t letter) const noexcept {
	// Результат работы функции
	size_t result = 0;
	// Если слово и буква переданы
	if(!word.empty() && (letter > 0)){
		// Ищем нашу букву
		size_t pos = 0;
		// Выполняем подсчет количества указанных букв в слове
		while((pos = word.find(letter, pos)) != wstring::npos){
			// Считаем количество букв
			result++;
			// Увеличиваем позицию
			pos++;
		}
	}
	// Выводим результат
	return result;
}
/**
 * isUrl Метод проверки соответствия слова url адресу
 * @param word слово для проверки
 * @return     результат проверки
 */
bool awh::Framework::isUrl(const wstring & word) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если слово передано
	if(!word.empty()){
		// Выполняем парсинг nwt адреса
		auto resUri = this->nwt.parse(word);
		// Если ссылка найдена
		result = ((resUri.type != nwt_t::types_t::NONE) && (resUri.type != nwt_t::types_t::WRONG));
	}
	// Выводим результат
	return result;
}
/**
 * isUpper Метод проверки символ на верхний регистр
 * @param letter буква для проверки
 * @return       результат проверки
 */
bool awh::Framework::isUpper(const wchar_t letter) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если слово передано
	if(letter > 0){
		// Если код символа не изменился, значит регистр верхний
		result = (wint_t(letter) == towupper(letter));
	}
	// Выводим результат
	return result;
}
/**
 * isSpace Метод проверки является ли буква, пробелом
 * @param letter буква для проверки
 * @return       результат проверки
 */
bool awh::Framework::isSpace(const wchar_t letter) const noexcept {
	// Получаем код символа
	const u_short lid = letter;
	// Выводим результат
	return ((lid == 32) || (lid == 160) || (lid == 173) || (lid == 9));
}
/**
 * isLatian Метод проверки является ли строка латиницей
 * @param str строка для проверки
 * @return    результат проверки
 */
bool awh::Framework::isLatian(const wstring & str) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если строка передана
	if(!str.empty()){
		// Длина слова
		const size_t length = str.length();
		// Переводим слово в нижний регистр
		const wstring & tmp = this->toLower(str);
		// Если длина слова больше 1-го символа
		if(length > 1){
			/**
			 * checkFn Функция проверки на валидность символа
			 * @param text  текст для проверки
			 * @param index индекс буквы в слове
			 * @return      результат проверки
			 */
			auto checkFn = [this](const wstring & text, const size_t index) noexcept {
				// Результат работы функции
				bool result = false;
				// Получаем текущую букву
				const wchar_t letter = text.at(index);
				// Если буква не первая и не последняя
				if((index > 0) && (index < (text.length() - 1))){
					// Получаем предыдущую букву
					const wchar_t first = text.at(index - 1);
					// Получаем следующую букву
					const wchar_t second = text.at(index + 1);
					// Если это дефис
					result = ((letter == L'-') && (first != L'-') && (second != L'-'));
					// Если проверка не пройдена, проверяем на апостроф
					if(!result){
						// Выполняем проверку на апостроф
						result = (
							(letter == L'\'') && (((first != L'\'') && (second != L'\'')) ||
							((this->latian.count(first) > 0) && (this->latian.count(second) > 0)))
						);
					}
					// Если результат не получен
					if(!result) result = (this->latian.count(letter) > 0);
				// Выводим проверку как она есть
				} else result = (this->latian.count(letter) > 0);
				// Выводим результат
				return result;
			};
			// Переходим по всем буквам слова
			for(size_t i = 0, j = (length - 1); j > ((length / 2) - 1); i++, j--){
				// Проверяем является ли слово латинским
				result = (i == j ? checkFn(tmp, i) : checkFn(tmp, i) && checkFn(tmp, j));
				// Если слово не соответствует тогда выходим
				if(!result) break;
			}
		// Если символ всего один, проверяем его так
		} else result = (this->latian.count(tmp.front()) > 0);
	}
	// Выводим результат
	return result;
}
/**
 * checkLatian Метод проверки наличия латинских символов в строке
 * @param str строка для проверки
 * @return    результат проверки
 */
bool awh::Framework::checkLatian(const wstring & str) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если строка передана
	if(!str.empty()){
		// Длина слова
		const size_t length = str.length();
		// Если длина слова больше 1-го символа
		if(length > 1){
			// Переходим по всем буквам слова
			for(size_t i = 0, j = (length - 1); j > ((length / 2) - 1); i++, j--){
				// Проверяем является ли слово латинским
				result = (i == j ? (this->latian.count(str.at(i)) > 0) : (this->latian.count(str.at(i)) > 0) || (this->latian.count(str.at(j)) > 0));
				// Если найдена хотя бы одна латинская буква тогда выходим
				if(result) break;
			}
		// Если символ всего один, проверяем его так
		} else result = (this->latian.count(str.front()) > 0);
	}
	// Выводим результат
	return result;
}
/**
 * isNumber Метод проверки является ли слово числом
 * @param word слово для проверки
 * @return     результат проверки
 */
bool awh::Framework::isNumber(const string & word) const noexcept {
	// Выполняем проверку
	return this->isNumber(this->convert(word));
}
/**
 * isNumber Метод проверки является ли слово числом
 * @param word слово для проверки
 * @return     результат проверки
 */
bool awh::Framework::isNumber(const wstring & word) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если слово передана
	if(!word.empty()){
		// Длина слова
		const size_t length = word.length();
		// Если длина слова больше 1-го символа
		if(length > 1){
			// Начальная позиция поиска
			const u_short pos = ((word.front() == L'-') || (word.front() == L'+') ? 1 : 0);
			// Переходим по всем буквам слова
			for(size_t i = pos, j = (length - 1); j > ((length / 2) - 1); i++, j--){
				// Проверяем является ли слово арабским числом
				result = !(
					(i == j) ? (this->numsSymbols.arabs.count(word.at(i)) < 1) :
					(this->numsSymbols.arabs.count(word.at(i)) < 1) ||
					(this->numsSymbols.arabs.count(word.at(j)) < 1)
				);
				// Если слово не соответствует тогда выходим
				if(!result) break;
			}
		// Если символ всего один, проверяем его так
		} else result = (this->numsSymbols.arabs.count(word.front()) > 0);
	}
	// Выводим результат
	return result;
}
/**
 * isANumber Метод проверки является ли косвенно слово числом
 * @param word слово для проверки
 * @return     результат проверки
 */
bool awh::Framework::isANumber(const wstring & word) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если слово передано
	if(!word.empty()){
		// Проверяем является ли слово числом
		result = this->isNumber(word);
		// Если не является то проверяем дальше
		if(!result){
			// Длина слова
			const size_t length = word.length();
			// Проверяем являются ли первая и последняя буква слова, числом
			result = (this->isNumber(wstring(1, word.front())) || this->isNumber(wstring(1, word.back())));
			// Если оба варианта не сработали
			if(!result && (length > 2)){
				// Первое слово
				wstring first = L"";
				// Переходим по всему списку
				for(size_t i = 1, j = length - 2; j > ((length / 2) - 1); i++, j--){
					// Получаем первое слово
					first.assign(1, word.at(i));
					// Проверяем является ли слово арабским числом
					result = (i == j ? this->isNumber(first) : this->isNumber(first) || this->isNumber(wstring(1, word[j])));
					// Если хоть один символ является числом, выходим
					if(result) break;
				}
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * isDecimal Метод проверки является ли слово дробным числом
 * @param word слово для проверки
 * @return     результат проверки
 */
bool awh::Framework::isDecimal(const string & word) const noexcept {
	// Выводим результат проверки
	return this->isDecimal(this->convert(word));
}
/**
 * isDecimal Метод проверки является ли слово дробным числом
 * @param word слово для проверки
 * @return     результат проверки
 */
bool awh::Framework::isDecimal(const wstring & word) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если слово передана
	if(!word.empty()){
		// Длина слова
		const size_t length = word.length();
		// Если длина слова больше 1-го символа
		if(length > 1){
			// Текущая буква
			wchar_t letter = 0;
			// Начальная позиция поиска
			const u_short pos = ((word.front() == L'-') || (word.front() == L'+') ? 1 : 0);
			// Переходим по всем символам слова
			for(size_t i = pos; i < length; i++){
				// Если позиция не первая
				if(i > pos){
					// Получаем текущую букву
					letter = word.at(i);
					// Если плавающая точка найдена
					if((letter == L'.') || (letter == L',')){
						// Проверяем правые и левую части
						result = (
							this->isNumber(word.substr(pos, i - pos)) &&
							this->isNumber(word.substr(i + 1))
						);
						// Выходим из цикла
						break;
					}
				}
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * getZones Метод извлечения списка пользовательских зон интернета
 * @return список доменных зон
 */
const std::set <wstring> & awh::Framework::getZones() const noexcept {
	// Выводим список доменных зон интернета
	return this->nwt.getZones();
}
/**
 * urls Метод извлечения координат url адресов в строке
 * @param text текст для извлечения url адресов
 * @return     список координат с url адресами
 */
std::map <size_t, size_t> awh::Framework::urls(const wstring & text) const noexcept {
	// Результат работы функции
	map <size_t, size_t> result;
	// Если текст передан
	if(!text.empty()){
		// Позиция найденного nwt адреса
		size_t pos = 0;
		// Выполням поиск ссылок в тексте
		while(pos < text.length()){
			// Выполняем парсинг nwt адреса
			auto resUri = this->nwt.parse(text.substr(pos));
			// Если ссылка найдена
			if(resUri.type != nwt_t::types_t::NONE){
				// Получаем данные слова
				const wstring & word = resUri.uri;
				// Если это не предупреждение
				if(resUri.type != nwt_t::types_t::WRONG){
					// Если позиция найдена
					if((pos = text.find(word, pos)) != wstring::npos){
						// Если в списке результатов найдены пустные значения, очищаем список
						if(result.count(wstring::npos) > 0) result.clear();
						// Добавляем в список нашу ссылку
						result.insert({pos, pos + word.length()});
					// Если ссылка не найдена в тексте, выходим
					} else break;
				}
				// Сдвигаем значение позиции
				pos += word.length();
			// Если uri адрес больше не найден то выходим
			} else break;
		}
	}
	// Выводим результат
	return result;
}
/**
 * split Метод разделения строк на составляющие
 * @param str   строка для поиска
 * @param delim разделитель
 * @param v     результирующий вектор
 */
void awh::Framework::split(const wstring & str, const wstring & delim, set <wstring> & v) const noexcept {
	// Выполняем сплит строки
	::split(str, delim, v);
}
/**
 * split Метод разделения строк на составляющие
 * @param str   строка для поиска
 * @param delim разделитель
 * @param v     результирующий вектор
 */
void awh::Framework::split(const wstring & str, const wstring & delim, list <wstring> & v) const noexcept {
	// Выполняем сплит строки
	::split(str, delim, v);
}
/**
 * split Метод разделения строк на составляющие
 * @param str   строка для поиска
 * @param delim разделитель
 * @param v     результирующий вектор
 */
void awh::Framework::split(const wstring & str, const wstring & delim, vector <wstring> & v) const noexcept {
	// Выполняем сплит строки
	::split(str, delim, v);
}
/**
 * split Метод разделения строк на составляющие
 * @param str   строка для поиска
 * @param delim разделитель
 * @param v     результирующий вектор
 */
void awh::Framework::split(const string & str, const string & delim, set <wstring> & v) const noexcept {
	// Выполняем сплит строки
	::split(this->convert(str), this->convert(delim), v);
}
/**
 * split Метод разделения строк на составляющие
 * @param str   строка для поиска
 * @param delim разделитель
 * @param v     результирующий вектор
 */
void awh::Framework::split(const string & str, const string & delim, list <wstring> & v) const noexcept {
	// Выполняем сплит строки
	::split(this->convert(str), this->convert(delim), v);
}
/**
 * split Метод разделения строк на составляющие
 * @param str   строка для поиска
 * @param delim разделитель
 * @param v     результирующий вектор
 */
void awh::Framework::split(const string & str, const string & delim, vector <wstring> & v) const noexcept {
	// Выполняем сплит строки
	::split(this->convert(str), this->convert(delim), v);
}
/**
 * setZone Метод установки пользовательской зоны
 * @param zone пользовательская зона
 */
void awh::Framework::setZone(const string & zone) noexcept {
	// Если зона передана, устанавливаем её
	if(!zone.empty()) this->nwt.setZone(this->convert(zone));
}
/**
 * setZone Метод установки пользовательской зоны
 * @param zone пользовательская зона
 */
void awh::Framework::setZone(const wstring & zone) noexcept {
	// Если зона передана, устанавливаем её
	if(!zone.empty()) this->nwt.setZone(zone);
}
/**
 * setZones Метод установки списка пользовательских зон
 * @param zones список доменных зон интернета
 */
void awh::Framework::setZones(const std::set <string> & zones) noexcept {
	// Устанавливаем список доменных зон
	if(!zones.empty()){
		// Переходим по всему списку доменных зон
		for(auto & zone : zones) this->setZone(zone);
	}
}
/**
 * setZones Метод установки списка пользовательских зон
 * @param zones список доменных зон интернета
 */
void awh::Framework::setZones(const std::set <wstring> & zones) noexcept {
	// Устанавливаем список доменных зон
	if(!zones.empty()) this->nwt.setZones(zones);
}
/**
 * setLocale Метод установки локали
 * @param locale локализация приложения
 */
void awh::Framework::setLocale(const string & locale) noexcept {
	// Устанавливаем локаль
	if(!locale.empty()){
		// Создаём новую локаль
		std::locale loc(locale.c_str());
		// Устанавливапм локализацию приложения
		::setlocale(LC_CTYPE, locale.c_str());
		::setlocale(LC_COLLATE, locale.c_str());
		// Устанавливаем локаль системы
		this->locale = std::locale::global(loc);
		/**
		 * Устанавливаем типы данных для Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Устанавливаем кодировку
			SetConsoleOutputCP(65001);
		#endif
	}
}
/**
 * unixTimestamp Метод получения штампа времени в миллисекундах
 * @return штамп времени в миллисекундах
 */
time_t awh::Framework::unixTimestamp() const noexcept {
	// Получаем штамп времени в миллисекундах
	chrono::milliseconds ms = chrono::duration_cast <chrono::milliseconds> (chrono::system_clock::now().time_since_epoch());
	// Выводим результат
	return ms.count();
}
/**
 * timeToStr Метод преобразования UnixTimestamp в строку
 * @param date   дата в UnixTimestamp
 * @param format формат даты
 * @return       строка содержащая дату
 */
string awh::Framework::timeToStr(const time_t date, const string & format) const noexcept {
	// Буфер с данными
	char buf[255];
	// Создаем структуру времени
	struct tm * tm = localtime(&date);
	// Зануляем буфер
	memset(buf, 0, sizeof(buf));
	// Выполняем парсинг даты
	strftime(buf, sizeof(buf), format.c_str(), tm);
	// Выводим результат
	return string(buf);
}
/**
 * strToTime Метод перевода строки в UnixTimestamp
 * @param date   строка даты
 * @param format формат даты
 * @return       дата в UnixTimestamp
 */
time_t awh::Framework::strToTime(const string & date, const string & format) const noexcept {
	// Результат работы функции
	time_t result = 0;
	// Если данные переданы
	if(!date.empty() && !format.empty()){
		// Создаем структуру времени
		struct tm tm;
		// Зануляем структуру
		memset(&tm, 0, sizeof(struct tm));
		// Выполняем парсинг даты
		this->strpTime(date, format, &tm);
		// Выводим результат
		result = mktime(&tm);
	}
	// Выводим результат
	return result;
}
/**
 * timeToAbbr Метод перевода времени в аббревиатуру
 * @param  date дата в UnixTimestamp
 * @return      строка содержащая аббревиатуру даты
 */
string awh::Framework::timeToAbbr(const time_t date) const noexcept {
	// Результат работы функции
	string result = "0 msec.";
	// Если число передано
	if(date > 0){
		// Если число больше года
		if(date >= 29030400000) result = this->format("%.1f year.", (date / double(29030400000)));
		// Если число больше месяца
		else if(date >= 2419200000) result = this->format("%.1f month.", (date / double(2419200000)));
		// Если число больше недели
		else if(date >= 604800000) result = this->format("%.1f week.", (date / double(604800000)));
		// Если число больше дня
		else if(date >= 86400000) result = this->format("%.1f day.", (date / double(86400000)));
		// Если число больше часа
		else if(date >= 3600000) result = this->format("%.1f hour.", (date / double(3600000)));
		// Если число больше минуты
		else if(date >= 60000) result = this->format("%.1f min.", (date / double(60000)));
		// Если число ольше секунды
		else if(date >= 1000) result = this->format("%.1f sec.", (date / double(1000)));
		// Иначе выводим как есть
		else result = this->format("%u msec.", date);
	}
	// Выводим результат
	return result;
}
/**
 * strpTime Функция получения Unix TimeStamp из строки
 * @param str    строка с датой
 * @param format форматы даты
 * @param tm     объект даты
 * @return       результат работы
 */
string awh::Framework::strpTime(const string & str, const string & format, struct tm * tm) const noexcept {
	// Результат работы функции
	string result = "";
	// Если данные переданы
	if(!str.empty() && !format.empty() && (tm != nullptr)){
		/*
		 * Разве стандартная библиотека C++ не хороша? std::get_time определен таким образом,
		 * что его параметры формата были точно такие же, как у strptime.
		 * Конечно, мы должны сначала создать строковый поток и наполнить его текущей локалью C,
		 * и мы также должны убедиться, что мы возвращаем правильные вещи, если это не удаётся или если это удаётся,
		 * но это все ещё намного проще. Чем любая из версий в любой из стандартных библиотек C.
		 */
		// Создаём строковый поток
		istringstream input(str.c_str());
		// Устанавливаем текущую локаль
		input.imbue(this->locale);
		// Извлекаем время локали
		input >> get_time(tm, format.c_str());
		// Если время получено
		if(!input.fail()){
			// Получаем указатель на строку
			const char * s = str.c_str();
			// Если всё удачно, выводим время
			result = (char *)(s + input.tellg());
		}
	}
	// Выводим результат
	return result;
}
/**
 * os Метод определения операционной системы
 * @return название операционной системы
 */
awh::Framework::os_t awh::Framework::os() const noexcept {
	// Результат
	os_t result = os_t::NONE;
	// Определяем операционную систему
	#ifdef _WIN32
		// Заполняем структуру
		result = os_t::WIND32;
	#elif _WIN64
		// Заполняем структуру
		result = os_t::WIND64;
	#elif __APPLE__ || __MACH__
		// Заполняем структуру
		result = os_t::MACOSX;
	#elif __linux__
		// Заполняем структуру
		result = os_t::LINUX;
	#elif __FreeBSD__
		// Заполняем структуру
		result = os_t::FREEBSD;
	#elif __unix || __unix__
		// Заполняем структуру
		result = os_t::UNIX;
	#endif
	// Выводим результат
	return result;
}
/**
 * icon Метод получения иконки
 * @param end флаг завершения работы
 * @return    иконка напутствия работы
 */
string awh::Framework::icon(const bool end) const noexcept {
	// Список иконок для начала работы
	const vector <string> iconBegin = {
		"🎲","🎰","🏓","🎱","🥚","⚽️",
		"🏀","🏈","⚾️","🥎","🏐","🪙",
		"🎾","🏑","🧲","🏹","🧱","🏋‍♀️",
		"⛹‍♀️","🤽‍♀️","🥁","🕯","🎳","🎮",
		"🙏","🤪","🙄","😏","😊","☺️",
		"😉","🤔","😋","😤","🤥","🧐",
		"🤓","😇","🙃","🤫","🤭","🙂",
		"🤗","🤩","😌","😎","🤡","🤠",
		"🌟","🧠","👀","👁","🏦","🛸",
		"🎬","❤️","📈","🛒","🛎","🤹‍♀️",
		"☝️","🎈","🧚","🕊","✨","⚡️",
		"🌏", "🔥","🪁","🎻","🎲","🎪",
		"🚦","🇷🇺","📺","🏸","🚀","⏳",
		"⏳","♨️","📉","💤","📊","🏳️"
	};
	// Список иконок для конца работы
	const vector <string> iconEnd = {
		"🍾","🎉","🎊","🎈","🎁","🥳",
		"🤩","😍","🥰","🤝","🙌","👐",
		"👌","✌️","🤟","🐝","🎖","🥇",
		"🥈","🥉","🏅","💳","🧨","🚬",
		"🏆","🎯","💎","🔮","🎗","🏵",
		"💪","👍","🪄","💍","⏰","🧮",
		"👸","🤴","🥷","💖","💘","🛍",
		"💝","🧸","💸","🧟‍♂️","💞","👩‍💻",
		"🎀","👅","💋","🚨","🦾","🦠",
		"💩","👾","👼","💥","💫","🌞",
		"🍫","🎂","💯","📰","❤️‍🔥","🎣",
		"🏁","🧾","💶","💷","💴","💵"
	};
	// рандомизация генератора случайных чисел
	srand(time(nullptr));
	// Получаем иконку
	return (!end ? iconBegin[rand() % iconBegin.size()] : iconEnd[rand() % iconEnd.size()]);
}
/**
 * md5 Метод получения md5 хэша из строки
 * @param text текст для перевода в строку
 * @return     хэш md5
 */
string awh::Framework::md5(const string & text) const noexcept {
	// Результат работы функции
	string result = "";
	// Если текст передан
	if(!text.empty()){
		// Массив полученных значений
		u_char digest[16];
		// Создаем контекст
		MD5_CTX ctx;
		// Выполняем инициализацию контекста
		MD5_Init(&ctx);
		// Выполняем расчет суммы
		MD5_Update(&ctx, text.c_str(), text.length());
		// Копируем полученные данные
		MD5_Final(digest, &ctx);
		// Строка md5
		char buffer[33];
		// Заполняем массив нулями
		memset(buffer, 0, 33);
		// Заполняем строку данными md5
		for(u_short i = 0; i < 16; i++) sprintf(&buffer[i * 2], "%02x", u_int(digest[i]));
		// Выводим результат
		result = buffer;
	}
	// Выводим результат
	return result;
}
/**
 * sha1 Метод получения sha1 хэша из строки
 * @param text текст для перевода в строку
 * @return     хэш sha1
 */
string awh::Framework::sha1(const string & text) const noexcept {
	// Результат работы функции
	string result = "";
	// Если текст передан
	if(!text.empty()){
		// Массив полученных значений
		u_char digest[20];
		// Создаем контекст
		SHA_CTX ctx;
		// Выполняем инициализацию контекста
		SHA1_Init(&ctx);
		// Выполняем расчет суммы
		SHA1_Update(&ctx, text.c_str(), text.length());
		// Копируем полученные данные
		SHA1_Final(digest, &ctx);
		// Строка sha1
		char buffer[41];
		// Заполняем массив нулями
		memset(buffer, 0, 41);
		// Заполняем строку данными sha1
		for(u_short i = 0; i < 20; i++) sprintf(&buffer[i * 2], "%02x", u_int(digest[i]));
		// Выводим результат
		result = buffer;
	}
	// Выводим результат
	return result;
}
/**
 * sha256 Метод получения sha256 хэша из строки
 * @param text текст для перевода в строку
 * @return     хэш sha256
 */
string awh::Framework::sha256(const string & text) const noexcept {
	// Результат работы функции
	string result = "";
	// Если текст передан
	if(!text.empty()){
		// Массив полученных значений
		u_char digest[32];
		// Создаем контекст
		SHA256_CTX ctx;
		// Выполняем инициализацию контекста
		SHA256_Init(&ctx);
		// Выполняем расчет суммы
		SHA256_Update(&ctx, text.c_str(), text.length());
		// Копируем полученные данные
		SHA256_Final(digest, &ctx);
		// Строка sha256
		char buffer[65];
		// Заполняем массив нулями
		memset(buffer, 0, 65);
		// Заполняем строку данными sha256
		for(u_short i = 0; i < 32; i++) sprintf(&buffer[i * 2], "%02x", u_int(digest[i]));
		// Выводим результат
		result = buffer;
	}
	// Выводим результат
	return result;
}
/**
 * sha512 Метод получения sha512 хэша из строки
 * @param text текст для перевода в строку
 * @return     хэш sha512
 */
string awh::Framework::sha512(const string & text) const noexcept {
	// Результат работы функции
	string result = "";
	// Если текст передан
	if(!text.empty()){
		// Массив полученных значений
		u_char digest[64];
		// Создаем контекст
		SHA512_CTX ctx;
		// Выполняем инициализацию контекста
		SHA512_Init(&ctx);
		// Выполняем расчет суммы
		SHA512_Update(&ctx, text.c_str(), text.length());
		// Копируем полученные данные
		SHA512_Final(digest, &ctx);
		// Строка sha512
		char buffer[129];
		// Заполняем массив нулями
		memset(buffer, 0, 129);
		// Заполняем строку данными sha512
		for(u_short i = 0; i < 64; i++) sprintf(&buffer[i * 2], "%02x", u_int(digest[i]));
		// Выводим результат
		result = buffer;
	}
	// Выводим результат
	return result;
}
/**
 * hmacsha1 Метод получения подписи hmac sha1
 * @param key  ключ для подписи
 * @param text текст для получения подписи
 * @return     хэш sha1
 */
string awh::Framework::hmacsha1(const string & key, const string & text) const noexcept {
	// Результат работы функции
	string result = "";
	// Если текст передан
	if(!key.empty() && !text.empty()){
		// Строка sha1
		char buffer[41];
		// Заполняем массив нулями
		memset(buffer, 0, 41);
		// Выполняем получение подписи
		const u_char * digest = HMAC(EVP_sha1(), key.c_str(), key.size(), (u_char *) text.c_str(), text.size(), nullptr, nullptr);
		// Заполняем строку данными sha1
		for(u_short i = 0; i < 20; i++) sprintf(&buffer[i * 2], "%02x", u_int(digest[i]));
		// Выводим результат
		result = buffer;
	}
	// Выводим результат
	return result;
}
/**
 * hmacsha256 Метод получения подписи hmac sha256
 * @param key  ключ для подписи
 * @param text текст для получения подписи
 * @return     хэш sha256
 */
string awh::Framework::hmacsha256(const string & key, const string & text) const noexcept {
	// Результат работы функции
	string result = "";
	// Если текст передан
	if(!key.empty() && !text.empty()){
		// Строка sha256
		char buffer[65];
		// Заполняем массив нулями
		memset(buffer, 0, 65);
		// Выполняем получение подписи
		const u_char * digest = HMAC(EVP_sha256(), key.c_str(), key.size(), (u_char *) text.c_str(), text.size(), nullptr, nullptr);
		// Заполняем строку данными sha256
		for(u_short i = 0; i < 32; i++) sprintf(&buffer[i * 2], "%02x", u_int(digest[i]));
		// Выводим результат
		result = buffer;
	}
	// Выводим результат
	return result;
}
/**
 * hmacsha512 Метод получения подписи hmac sha512
 * @param key  ключ для подписи
 * @param text текст для получения подписи
 * @return     хэш sha512
 */
string awh::Framework::hmacsha512(const string & key, const string & text) const noexcept {
	// Результат работы функции
	string result = "";
	// Если текст передан
	if(!key.empty() && !text.empty()){
		// Строка sha512
		char buffer[129];
		// Заполняем массив нулями
		memset(buffer, 0, 129);
		// Выполняем получение подписи
		const u_char * digest = HMAC(EVP_sha512(), key.c_str(), key.size(), (u_char *) text.c_str(), text.size(), nullptr, nullptr);
		// Заполняем строку данными sha512
		for(u_short i = 0; i < 64; i++) sprintf(&buffer[i * 2], "%02x", u_int(digest[i]));
		// Выводим результат
		result = buffer;
	}
	// Выводим результат
	return result;
}
/**
 * bytes Метод получения размера в байтах из строки
 * @param str строка обозначения размерности
 * @return    размер в байтах
 */
size_t awh::Framework::bytes(const string & str) const noexcept {
	// Результат работы регулярного выражения
	smatch match;
	// Размер количество байт
	size_t size = 0;
	// Устанавливаем правило регулярного выражения
	regex e("([\\d\\.\\,]+)(B|KB|MB|GB)", regex::ECMAScript);
	// Выполняем размерности данных
	regex_search(str, match, e);
	// Если данные найдены
	if(!match.empty()){
		// Размерность скорости
		float dimension = 1.0f;
		// Получаем значение размерности
		float value = stof(match[1].str());
		// Запоминаем параметры
		const string & param = match[2].str();
		// Проверяем являются ли переданные данные байтами (8, 16, 32, 64, 128, 256, 512, 1024 ...)
		bool isbite = !fmod(value / 8, 2);
		// Если это байты
		if(param.compare("B") == 0) dimension = 1;
		// Если это размерность в киллобитах
		else if(param.compare("KB") == 0) dimension = (isbite ? 1000 : 1024);
		// Если это размерность в мегабитах
		else if(param.compare("MB") == 0) dimension = (isbite ? 1000000 : 1048576);
		// Если это размерность в гигабитах
		else if(param.compare("GB") == 0) dimension = (isbite ? 1000000000 : 1073741824);
		// Размер буфера по умолчанию
		size = (long) value;
		// Если размерность установлена тогда расчитываем количество байт
		if(value > -1) size = (value * dimension);
	}
	// Выводим результат
	return size;
}
/**
 * seconds Метод получения размера в секундах из строки
 * @param str строка обозначения размерности
 * @return    размер в секундах
 */
time_t awh::Framework::seconds(const string & str) const noexcept {
	// Результат работы регулярного выражения
	smatch match;
	// Количество секунд
	time_t seconds = 0;
	// Устанавливаем правило регулярного выражения
	regex e("([\\d\\.\\,]+)(s|m|h|d|M|y)", regex::ECMAScript);
	// Выполняем поиск времени
	regex_search(str, match, e);
	// Если данные найдены
	if(!match.empty()){
		// Размерность времени
		float dimension = 1.0f;
		// Получаем значение размерности
		float value = stof(match[1].str());
		// Запоминаем параметры
		const string & param = match[2].str();
		// Если это секунды
		if(param.compare("s") == 0) dimension = 1;
		// Если это размерность в минутах
		else if(param.compare("m") == 0) dimension = 60;
		// Если это размерность в часах
		else if(param.compare("h") == 0) dimension = 3600;
		// Если это размерность в днях
		else if(param.compare("d") == 0) dimension = 86400;
		// Если это размерность в месяцах
		else if(param.compare("М") == 0) dimension = 2592000;
		// Если это размерность в годах
		else if(param.compare("y") == 0) dimension = 31104000;
		// Размер буфера по умолчанию
		seconds = time_t(value);
		// Если время установлено тогда расчитываем количество секунд
		if(value > -1) seconds = (value * dimension);
	}
	// Выводим результат
	return seconds;
}
/**
 * sizeBuffer Метод получения размера буфера в байтах
 * @param str пропускная способность сети (bps, kbps, Mbps, Gbps)
 * @return    размер буфера в байтах
 */
long awh::Framework::sizeBuffer(const string & str) const noexcept {
	/*
	* Help - http://www.securitylab.ru/analytics/243414.php
	*
	* 0.04 - Пропускная способность сети 40 милисекунд
	* 100 - Скорость в мегабитах (Мб) на пользователя
	* 8 - Количество бит в байте
	* 1024000 - количество байт в мегабайте
	* (2 * 0.04) * ((100 * 1024000) / 8)  = 1000 байт
	*
	*/
	// Результат работы регулярного выражения
	smatch match;
	// Размер буфера в байтах
	long size = -1;
	// Устанавливаем правило регулярного выражения
	regex e("([\\d\\.\\,]+)(bps|kbps|Mbps|Gbps)", regex::ECMAScript);
	// Выполняем поиск скорости
	regex_search(str, match, e);
	// Если данные найдены
	if(!match.empty()){
		// Запоминаем параметры
		string param = match[2].str();
		// Размерность скорости
		float dimension = 1.0f;
		// Получаем значение скорости
		float speed = stof(match[1].str());
		// Проверяем являются ли переданные данные байтами (8, 16, 32, 64, 128, 256, 512, 1024 ...)
		bool isbite = !fmod(speed / 8, 2);
		// Если это байты
		if(param.compare("bps") == 0) dimension = 1;
		// Если это размерность в киллобитах
		else if(param.compare("kbps") == 0) dimension = (isbite ? 1000 : 1024);
		// Если это размерность в мегабитах
		else if(param.compare("Mbps") == 0) dimension = (isbite ? 1000000 : 1048576);
		// Если это размерность в гигабитах
		else if(param.compare("Gbps") == 0) dimension = (isbite ? 1000000000 : 1073741824);
		// Размер буфера по умолчанию
		size = (long) speed;
		// Если скорость установлена тогда расчитываем размер буфера
		if(speed > -1) size = (2.0f * 0.04f) * ((speed * dimension) / 8.0f);
	}
	// Выводим результат
	return size;
}
/**
 * rate Метод порверки на сколько процентов (A > B) или (A < B)
 * @param a первое число
 * @param b второе число
 * @return  результат расчёта
 */
float awh::Framework::rate(const float a, const float b) const noexcept {
	// Выводим разницу в процентах
	return ((a > b ? ((a - b) / b * 100) : ((b - a) / b * 100) * -1));
}
/**
 * Framework Конструктор
 */
awh::Framework::Framework() noexcept {
	// Устанавливаем локализацию системы
	this->setLocale();
}
/**
 * Framework Конструктор
 * @param locale локализация приложения
 */
awh::Framework::Framework(const string & locale) noexcept {
	// Устанавливаем локализацию системы
	this->setLocale(locale);
}
