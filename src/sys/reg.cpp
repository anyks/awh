/**
 * @file: re.cpp
 * @date: 2023-12-14
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
#include <sys/reg.hpp>

/**
 * error Метод извлечения текста ошибки регулярного выражения
 * @return текст ошибки регулярного выражения
 */
const string & awh::RegExp::error() const noexcept {
	// Выполняем извлечение текста ошибки регулярного выражения
	return this->_error;
}
/**
 * free Метод очистки объекта регулярного выражения
 * @param exp объект регулярного выражения
 */
void awh::RegExp::free(const exp_t & exp) const noexcept {
	// Если объект регулярного выражения ещё не очищен
	if(exp.mode){
		// Зануляем объект регулярного выражения
		const_cast <exp_t &> (exp).mode = !exp.mode;
		// Выполняем удаление скомпилированного регулярного выражения
		pcre2_regfree(&const_cast <exp_t &> (exp).reg);
	}
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
	// Если данные переданы верные
	if(!text.empty() && exp.mode){
		// Создаём объект матчинга
		regmatch_t match[1];
		// Выполняем разбор регулярного выражения
		const int error = pcre2_regexec(&exp.reg, text.c_str(), 1, match, REG_NOTEMPTY);
		// Если возникла ошибка
		if(!(result = (error == 0))){
			// Создаём буфер данных для извлечения данных ошибки
			char buffer[256];
			// Выполняем заполнение нулями буфер данных
			::memset(buffer, '\0', sizeof(buffer));
			// Выполняем извлечение текста ошибки
			const size_t size = pcre2_regerror(error, &exp.reg, buffer, sizeof(buffer) - 1);
			// Если текст ошибки получен
			if(size > 0)
				// Выполняем установку кода ошибки
				const_cast <regexp_t *> (this)->_error.assign(buffer, size);
		// Если ошибок не получено
		} else result = (match[0].rm_eo > 0);
	}
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
	// Если данные переданы верные
	if(!text.empty() && exp.mode){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Получаем строку текста для поиска
			const char * str = text.c_str();
			// Создаём объект матчинга
			unique_ptr <regmatch_t []> match(new regmatch_t [exp.reg.re_nsub + 1]);
			// Выполняем разбор регулярного выражения
			const int error = pcre2_regexec(&exp.reg, str, exp.reg.re_nsub + 1, match.get(), REG_NOTEMPTY);
			// Если возникла ошибка
			if(error > 0){
				// Создаём буфер данных для извлечения данных ошибки
				char buffer[256];
				// Выполняем заполнение нулями буфер данных
				::memset(buffer, '\0', sizeof(buffer));
				// Выполняем извлечение текста ошибки
				const size_t size = pcre2_regerror(error, &exp.reg, buffer, sizeof(buffer) - 1);
				// Если текст ошибки получен
				if(size > 0)
					// Выполняем установку кода ошибки
					const_cast <regexp_t *> (this)->_error.assign(buffer, size);
			// Если ошибок не получено
			} else {
				// Выполняем создание результата
				result.resize(exp.reg.re_nsub + 1);
				// Выполняем перебор всех полученных вариантов
				for(uint8_t i = 0; i < static_cast <uint8_t> (exp.reg.re_nsub + 1); i++){
					// Если результат получен
					if(match[i].rm_eo > 0)
						// Добавляем полученный результат в список результатов
						result.at(i).assign(str + match[i].rm_so, match[i].rm_eo - match[i].rm_so);
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const bad_alloc &) {
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		}
	}
	// Выводим результат
	return result;
}
/**
 * build Метод сборки регулярного выражения
 * @param pattern регулярное выражение для сборки
 * @param options список опций для сборки регулярного выражения
 * @return        результат собранного регулярного выражения
 */
awh::RegExp::exp_t awh::RegExp::build(const string & pattern, const vector <option_t> & options) const noexcept {
	// Результат работы функции
	exp_t result;
	// Если регулярное выражение передано
	if(!pattern.empty()){
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
						option |= REG_UTF;
					break;
					// Если передан флаг запреда вывода сопоставления
					case static_cast <uint8_t> (option_t::NOSUB):
						// Выполняем установку флага
						option |= REG_NOSUB;
					break;
					// Если передан флаг точки соответствующей чему угодно, включая NL
					case static_cast <uint8_t> (option_t::DOTALL):
						// Выполняем установку флага
						option |= REG_DOTALL;
					break;
					// Если передан флаг инвертирования жадности кванторов
					case static_cast <uint8_t> (option_t::UNGREEDY):
						// Выполняем установку флага
						option |= REG_UNGREEDY;
					break;
					// Если нужно блокировать пустые строки
					case static_cast <uint8_t> (option_t::NOTEMPTY):
						// Выполняем установку флага
						option |= REG_NOTEMPTY;
					break;
					// Если передан флаг работы без учёта регистра
					case static_cast <uint8_t> (option_t::CASELESS):
						// Выполняем установку флага
						option |= REG_ICASE;
					break;
					// Если передан флаг то (^ и $) будут соответствовать новым строкам в тексте
					case static_cast <uint8_t> (option_t::MULTILINE):
						// Выполняем установку флага
						option |= REG_NEWLINE;
					break;
				}
			}
		}
		// Выполняем компиляцию регулярного выражения
		const int error = pcre2_regcomp(&result.reg, pattern.c_str(), option);
		// Если возникла ошибка компиляции
		if(!(result.mode = (error == 0))){
			// Создаём буфер данных для извлечения данных ошибки
			char buffer[256];
			// Выполняем заполнение нулями буфер данных
			::memset(buffer, '\0', sizeof(buffer));
			// Выполняем извлечение текста ошибки
			const size_t size = pcre2_regerror(error, &result.reg, buffer, sizeof(buffer) - 1);
			// Если текст ошибки получен
			if(size > 0)
				// Выполняем установку кода ошибки
				const_cast <regexp_t *> (this)->_error.assign(buffer, size);
			// Выполняем удаление скомпилированного регулярного выражения
			pcre2_regfree(&result.reg);
		}
	}
	// Выводим результат
	return result;
}
