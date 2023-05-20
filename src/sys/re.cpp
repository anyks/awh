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
 * free Метод очистки объекта регулярного выражения
 * @param exp объект регулярного выражения
 */
void awh::RegExp::free(const exp_t & exp) const noexcept {
	// Если резулярное выражение собранно
	if(exp.ctx != nullptr){
		// Выполняем удаление контекста регулярного выражения
		pcre_free(exp.ctx);
		// Выполняем обнуление контекста регулярного выражения
		const_cast <exp_t *> (&exp)->ctx = nullptr;
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
	if(!text.empty() && (exp.ctx != nullptr)){
		// Массив позиций в тексте
		int ovector[30];
		// Выполняем разбор регулярного выражения не более 30-ти вариантов
		result = (pcre_exec(exp.ctx, nullptr, text.c_str(), text.length(), 0, 0, ovector, 30) > 0);
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
	// Выводим результат
	return result;
}
/**
 * build Метод сборки регулярного выражения
 * @param expression регулярное выражение для сборки
 * @param options    список опций для сборки регулярного выражения
 * @return           результат собранного регулярного выражения
 */
awh::RegExp::exp_t awh::RegExp::build(const string & expression, const vector <option_t> & options) noexcept {
	// Результат работы функции
	exp_t result;
	// Если регулярное выражение передано
	if(!expression.empty()){
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
	}
	// Выводим результат
	return result;
}
