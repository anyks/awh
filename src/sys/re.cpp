/**
 * @file: re.cpp
 * @date: 2023-05-20
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2023
 */

// Подключаем заголовочный файл
#include <sys/re.hpp>

/**
 * Методы только для OS Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	/**
	 * convert Метод конвертирования строки utf-8 в строку
	 * @param str строка utf-8 для конвертирования
	 * @return    обычная строка
	 */
	string awh::RegExp::convert(const wstring & str) const noexcept {
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
	 * convert Метод конвертирования строки в строку utf-8
	 * @param str строка для конвертирования
	 * @return    строка в utf-8
	 */
	wstring awh::RegExp::convert(const string & str) const noexcept {
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
#endif
/**
 * free Метод очистки объекта регулярного выражения
 * @param exp объект регулярного выражения
 */
void awh::RegExp::free(const exp_t & exp) const noexcept {
	/**
	 * Для операционных систем кроме OS Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Если резулярное выражение собранно
		if(exp.ctx != nullptr){
			// Выполняем удаление контекста регулярного выражения
			pcre_free(exp.ctx);
			// Выполняем обнуление контекста регулярного выражения
			const_cast <exp_t *> (&exp)->ctx = nullptr;
		}
	#endif
}
/**
 * test Метод проверки регулярного выражения
 * @param text текст для обработки
 * @param exp  объект регулярного выражения
 * @return     результат проверки регулярного выражения
 */
bool awh::RegExp::test(const string & text, const exp_t & exp) const noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Методы только для OS Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Если данные переданы верные
		if(!text.empty()){
			/**
			 * Выполняем обработку ошибки
			 */
			try {
				// Если мы выполняем работу с UTF-8
				if(exp.utf8){
					// Результат работы регулярного выражения
					wsmatch match;
					// Выполняем конвертирования текста
					const wstring & enter = this->convert(text);
					// Выполняем разбор регулярного выражения
					result = regex_match(enter, match, exp.wreg, regex_constants::match_default);
				// Если мы выполняем работу без поддержки UTF-8
				} else {
					// Результат работы регулярного выражения
					smatch match;
					// Выполняем разбор регулярного выражения
					result = regex_match(text, match, exp.reg, regex_constants::match_default);
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {}
		}
	/**
	 * Для всех остальных операционных систем
	 */
	#else
		// Если данные переданы верные
		if(!text.empty() && (exp.ctx != nullptr)){
			// Массив позиций в тексте
			int ovector[30];
			// Выполняем разбор регулярного выражения не более 30-ти вариантов
			result = (pcre_exec(exp.ctx, nullptr, text.c_str(), text.length(), 0, 0, ovector, 30) > 0);
		}
	#endif
	// Выводим результат
	return result;
}
/**
 * exec Метод запуска регулярного выражения
 * @param text текст для обработки
 * @param exp  объект регулярного выражения
 * @return     результат обработки регулярного выражения
 */
vector <string> awh::RegExp::exec(const string & text, const exp_t & exp) const noexcept {
	// Результат работы функции
	vector <string> result;
	/**
	 * Методы только для OS Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Если данные переданы верные
		if(!text.empty()){
			/**
			 * Выполняем обработку ошибки
			 */
			try {
				// Если мы выполняем работу с UTF-8
				if(exp.utf8){
					// Результат работы регулярного выражения
					wsmatch match;
					// Выполняем конвертирования текста
					const wstring & enter = this->convert(text);
					// Выполняем поиск в тексте по регулярному выражению
					if(regex_match(enter, match, exp.wreg, regex_constants::match_default)){
						// Выполняем перебор всех полученных результатов
						for(auto & item : match)
							// Добавляем полученный результат в список результатов
							result.push_back(this->convert(item));
					}
				// Если мы выполняем работу без поддержки UTF-8
				} else {
					// Результат работы регулярного выражения
					smatch match;
					// Выполняем поиск в тексте по регулярному выражению
					if(regex_match(text, match, exp.reg, regex_constants::match_default)){
						// Выполняем перебор всех полученных результатов
						for(auto & item : match)
							// Добавляем полученный результат в список результатов
							result.push_back(item);
					}
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {}
		}
	/**
	 * Для всех остальных операционных систем
	 */
	#else
		// Если данные переданы верные
		if(!text.empty() && (exp.ctx != nullptr)){
			// Массив позиций в тексте
			int ovector[30];
			// Выполняем разбор регулярного выражения не более 30-ти вариантов
			const int count = pcre_exec(exp.ctx, nullptr, text.c_str(), text.length(), 0, 0, ovector, 30);
			// Если в текст не соответствует регулярному выражению
			if(count > 0){
				// Индекс полученной записи
				uint8_t index = 0;
				// Выполняем выделение памяти для списка результатов
				result.resize(count);
				// Выполняем перебор всех полученных результатов
				for(uint8_t i = 0; i < (2 * count); i += 2)
					// Получаем название переменной
					result[index++] = (ovector[i] < 0 ? "" : text.substr(ovector[i], ovector[i + 1] - ovector[i]));
			}
		}
	#endif
	// Выводим результат
	return result;
}
/**
 * build Метод сборки регулярного выражения
 * @param expression регулярное выражение для сборки
 * @param options    список опций для сборки регулярного выражения
 * @return           результат собранного регулярного выражения
 */
awh::RegExp::exp_t awh::RegExp::build(const string & expression, const vector <option_t> & options) const noexcept {
	// Результат работы функции
	exp_t result;
	// Если регулярное выражение передано
	if(!expression.empty()){
		/**
		 * Методы только для OS Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Результат работы функции
			wregex::flag_type option = static_cast <wregex::flag_type> (0);
			// Если опции переданы
			if(!options.empty()){
				// Выполняем перебор всех переданных опций
				for(auto & item : options){
					// Определяем тип переданной опции
					switch(static_cast <uint8_t> (item)){
						// Если передан флаг запуска в режиме UTF-8
						case static_cast <uint8_t> (option_t::UTF8):
							// Выполняем установку флага работы с UTF-8
							result.utf8 = true;
						break;
						// Если передан флаг дополнительных функций
						case static_cast <uint8_t> (option_t::EXTRA):
							// Устанавливаем флаг
							option |= wregex::optimize;
						break;
						// Если передан флаг инвертирования жадности кванторов
						case static_cast <uint8_t> (option_t::UNGREEDY):
							// Устанавливаем флаг
							option |= wregex::nosubs;
						break;
						// Если передан флаг работы без учёта регистра
						case static_cast <uint8_t> (option_t::CASELESS):
							// Выполняем установку флага
							option |= wregex::icase;
						break;
						// Если передан флаг игнорирования пробелов и # комментариев
						case static_cast <uint8_t> (option_t::EXTENDED):
							// Устанавливаем флаг
							option |= wregex::collate;
						break;
					}
				}
			}
			// Устанавливаем флаг
			option |= wregex::ECMAScript;
			// Если мы выполняем работу с UTF-8
			if(result.utf8)
				// Выполняем сборку регулярного выражения
				result.wreg = wregex(this->convert(expression), option);
			// Иначе выполняем сборку регулярных выражений без поддержки UTF-8
			else result.reg = regex(expression, option);
		/**
		 * Для всех остальных операционных систем
		 */
		#else
			// Список основных опций
			int option = 0;
			// Если опции переданы
			if(!options.empty()){
				// Выполняем перебор всех переданных опций
				for(auto & item : options){
					// Определяем тип переданной опции
					switch(static_cast <uint8_t> (item)){
						// Если передан флаг запуска в режиме UTF-8
						case static_cast <uint8_t> (option_t::UTF8):
							// Выполняем установку флага
							option |= PCRE_UTF8;
						break;
						// Если передан флаг дополнительных функций
						case static_cast <uint8_t> (option_t::EXTRA):
							// Выполняем установку флага
							option |= PCRE_EXTRA;
						break;
						// Если передан флаг точки соответствующей чему угодно, включая NL
						case static_cast <uint8_t> (option_t::DOTALL):
							// Выполняем установку флага
							option |= PCRE_DOTALL;
						break;
						// Если передан флаг инвертирования жадности кванторов
						case static_cast <uint8_t> (option_t::UNGREEDY):
							// Выполняем установку флага
							option |= PCRE_UNGREEDY;
						break;
						// Если передан флаг привязки шаблона якоря
						case static_cast <uint8_t> (option_t::ANCHORED):
							// Выполняем установку флага
							option |= PCRE_ANCHORED;
						break;
						// Если передан флаг работы без учёта регистра
						case static_cast <uint8_t> (option_t::CASELESS):
							// Выполняем установку флага
							option |= PCRE_CASELESS;
						break;
						// Если передан флаг игнорирования пробелов и # комментариев
						case static_cast <uint8_t> (option_t::EXTENDED):
							// Выполняем установку флага
							option |= PCRE_EXTENDED;
						break;
						// Если передан флаг то (^ и $) будут соответствовать новым строкам в тексте
						case static_cast <uint8_t> (option_t::MULTILINE):
							// Выполняем установку флага
							option |= PCRE_MULTILINE;
						break;
						// Если передан флаг то (\R) будет соответствовать всем окончаниям строк Unicode
						case static_cast <uint8_t> (option_t::BSR_UNICODE):
							// Выполняем установку флага
							option |= PCRE_BSR_UNICODE;
						break;
						// Если передан флаг то будет установлен CR в качестве последовательности новой строки
						case static_cast <uint8_t> (option_t::NEWLINE_CR):
							// Выполняем установку флага
							option |= PCRE_NEWLINE_CR;
						break;
						// Если передан флаг то будет установлен LF в качестве последовательности новой строки
						case static_cast <uint8_t> (option_t::NEWLINE_LF):
							// Выполняем установку флага
							option |= PCRE_NEWLINE_LF;
						break;
						// Если передан флаг то будет установлен CRLF в качестве последовательности новой строки
						case static_cast <uint8_t> (option_t::NEWLINE_CRLF):
							// Выполняем установку флага
							option |= PCRE_NEWLINE_CRLF;
						break;
						// Если передан флаг то (\R) будет соответствовать только CR, LF или CRLF
						case static_cast <uint8_t> (option_t::BSR_ANYCRLF):
							// Выполняем установку флага
							option |= PCRE_BSR_ANYCRLF;
						break;
						// Если передан флаг то будет скомпилированны автоматические выноски
						case static_cast <uint8_t> (option_t::AUTO_CALLOUT):
							// Выполняем установку флага
							option |= PCRE_AUTO_CALLOUT;
						break;
						// Если передан флаг не проверять шаблон на допустимость UTF-8
						case static_cast <uint8_t> (option_t::NO_UTF8_CHECK):
							// Выполняем установку флага
							option |= PCRE_NO_UTF8_CHECK;
						break;
						// Если передан флаг отключения пронумерованных скобок захвата (доступны именованные)
						case static_cast <uint8_t> (option_t::NO_AUTO_CAPTURE):
							// Выполняем установку флага
							option |= PCRE_NO_AUTO_CAPTURE;
						break;
					}
				}
			}
			// Выполняем компиляцию регулярного выражения
			result.ctx = pcre_compile(expression.c_str(), option, &result.error, &result.offset, nullptr);
		#endif
	}
	// Выводим результат
	return result;
}
