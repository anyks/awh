/**
 * @file lexer.cpp
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
 * @brief Исходный файл разборщика параметров запуска и текстовых потоков
 *
 * \~english
 * @brief Source file of the parser of the parameters of the launch and of the text streams
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы модуля
 */
#include <args/lexer.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Используем пространство имён параметров запуска приложения
 */
using namespace awh::args;

/**
 * @brief Внутренние помощники разборщика параметров
 *
 */
namespace {
	/**
	 * @brief Метод извещения об отказе разбора
	 *
	 * @param failure  отзыв извещения об отказе разбора
	 * @param error    код ошибки разбора
	 * @param location положение лексемы в поданном наборе
	 * @return         признак продолжения разбора
	 *
	 * @warning Виды здесь названы ПОЛНЫМ именем намеренно, и это не многословие: у
	 *          систем GNU заголовок «errno.h» объявляет свой «error_t» в глобальном
	 *          пространстве, и краткое имя в свободной функции разрешается в него, а
	 *          не в наш. Замерено срывом сборки на Debian 12.15 с g++ 12.2: довод
	 *          принимался видом «int», а внутри класса то же имя разрешалось верно -
	 *          оттого срыв виден лишь у свободных функций и лишь у систем GNU
	 *
	 */
	bool notify(const Lexer::failure_t & failure, const awh::args::error_t error, const awh::args::location_t & location) noexcept {
		// Если отзыв извещения об отказе установлен
		if(failure != nullptr)
			// Выполняем извещение об отказе разбора
			return failure(error, location);
		// Сообщаем, что разбор следует продолжить
		return true;
	}
}

/**
 * @brief Метод определения схожести слова с числом
 *
 * @param word слово для проверки
 * @return     результат проверки
 *
 */
bool awh::args::Lexer::numeric(const string_view word) const noexcept {
	// Если слово короче двух знаков, числом оно быть не может
	if(word.length() < 2)
		// Сообщаем, что слово числом не является
		return false;
	// Если слово начинается не с тире
	if(word.front() != '-')
		// Сообщаем, что слово числом не является
		return false;
	// Получаем остаток слова без ведущего тире
	const string_view rest = word.substr(1);
	// Если остаток начинается с точки, а иных знаков за нею нет
	if((rest.front() == '.') && (rest.length() < 2))
		// Сообщаем, что слово числом не является
		return false;
	// Если первый знак остатка не цифра и не точка
	if(!this->_fmk->is(rest.front(), fmk_t::check_t::NUMBER) && (rest.front() != '.'))
		// Сообщаем, что слово числом не является
		return false;
	// Выполняем перебор остальных знаков остатка
	for(size_t i = 1; i < rest.length(); i++){
		// Получаем знак остатка слова
		const char letter = rest.at(i);
		// Если знак цифрой не является
		if(!this->_fmk->is(letter, fmk_t::check_t::NUMBER)){
			// Пропускаем знаки, дозволенные записи числа
			switch(letter){
				// Разделитель целой и дробной части
				case '.':
				// Признак порядка числа
				case 'e':
				case 'E':
				// Знак порядка числа
				case '+':
				case '-':
					// Продолжаем перебор знаков остатка
				break;
				// Для всех остальных знаков
				default:
					// Сообщаем, что слово числом не является
					return false;
			}
		}
	}
	// Сообщаем, что слово видом схоже с числом
	return true;
}

/**
 * @brief Метод разреза текста на слова
 *
 * @param text    текст для разреза
 * @param result  контейнер собранных слов
 * @param failure отзыв извещения об отказе разбора
 * @return        результат разреза
 *
 */
bool awh::args::Lexer::split(const string_view text, vector <string> & result, const failure_t & failure) const noexcept {
	// Выполняем очистку контейнера собранных слов
	result.clear();
	// Собираемое слово текста
	string word = "";
	// Признак начатого слова
	bool started = false;
	// Знак незакрытой кавычки, нуль при её отсутствии
	char quote = 0;
	// Выполняем перебор всех знаков поданного текста
	for(size_t i = 0; i < text.length(); i++){
		// Получаем знак поданного текста
		const char letter = text.at(i);
		// Если знаком является обратная косая
		if(letter == '\\'){
			// Если обратная косая стоит последним знаком текста
			if((i + 1) >= text.length()){
				// Выполняем извещение об отказе разбора
				if(!notify(failure, error_t::DANGLING, location_t(result.size(), i)))
					// Выходим из метода, разбор остановлен
					return false;
				// Продолжаем разбор текста дальше
				continue;
			}
			// Отмечаем, что слово текста начато
			started = true;
			// Добавляем к слову знак, следующий за обратной косой
			word.append(1, text.at(++i));
			// Продолжаем разбор текста дальше
			continue;
		}
		// Если кавычка не открыта вовсе
		if(quote == 0){
			// Если знаком является кавычка одинарная либо двойная
			if((letter == '"') || (letter == '\'')){
				// Запоминаем знак открытой кавычки
				quote = letter;
				// Отмечаем, что слово текста начато
				started = true;
				// Продолжаем разбор текста дальше
				continue;
			}
			// Если знаком является разделитель слов
			if(this->_fmk->is(letter, fmk_t::check_t::SPACE)){
				// Если слово текста начато
				if(started)
					// Добавляем собранное слово в контейнер
					result.push_back(::move(word));
				// Выполняем очистку собираемого слова
				word.clear();
				// Отмечаем, что слово текста не начато
				started = false;
				// Продолжаем разбор текста дальше
				continue;
			}
		// Если открытая кавычка закрыта тем же самым знаком
		} else if(letter == quote) {
			// Снимаем признак открытой кавычки
			quote = 0;
			// Продолжаем разбор текста дальше
			continue;
		}
		// Отмечаем, что слово текста начато
		started = true;
		// Добавляем знак к собираемому слову
		word.append(1, letter);
	}
	// Если кавычка осталась незакрытой
	if(quote != 0){
		// Выполняем извещение об отказе разбора
		if(!notify(failure, error_t::UNPAIRED, location_t(result.size(), text.length())))
			// Выходим из метода, разбор остановлен
			return false;
	}
	// Если слово текста осталось начатым
	if(started)
		// Добавляем собранное слово в контейнер
		result.push_back(::move(word));
	// Сообщаем, что разрез текста выполнен
	return true;
}

/**
 * @brief Метод разбора набора доводов запуска
 *
 * @param items    набор доводов запуска
 * @param callback отзыв выдачи разобранной лексемы
 * @param failure  отзыв извещения об отказе разбора
 * @return         результат разбора
 *
 */
bool awh::args::Lexer::parse(const vector <string> & items, const callback_t & callback, const failure_t & failure) const noexcept {
	// Если отзыв выдачи разобранной лексемы не установлен
	if(callback == nullptr)
		// Выходим из метода, выдавать разобранное некому
		return false;
	// Число выданных лексем разбора
	size_t count = 0;
	// Признак пройденного конца именованных параметров
	bool terminated = false;
	// Выполняем перебор всего набора доводов запуска
	for(size_t i = 0; i < items.size(); i++){
		// Получаем довод набора запуска
		const string & item = items.at(i);
		// Если число лексем превысило предел разбора
		if((this->_settings.maxTokens > 0) && (count >= this->_settings.maxTokens)){
			// Выполняем извещение об отказе разбора
			if(!notify(failure, error_t::MANY_TOKENS, location_t(i, 0)))
				// Выходим из метода, разбор остановлен
				return false;
			// Выходим из метода, набор разобран не весь
			return false;
		}
		// Создаём лексему разбора
		lexeme_t lexeme;
		// Устанавливаем положение лексемы в поданном наборе
		lexeme.location = location_t(i, 0);
		// Если конец именованных параметров уже пройден либо довод именем не начат
		if(terminated || (item.empty()) || (item.front() != '-')){
			// Устанавливаем вид лексемы позиционным доводом
			lexeme.type = token_t::OPERAND;
			// Устанавливаем содержимое позиционного довода
			lexeme.value = item;
			// Увеличиваем число выданных лексем разбора
			count++;
			// Выполняем выдачу разобранной лексемы
			if(!callback(lexeme))
				// Выходим из метода, разбор остановлен
				return false;
			// Продолжаем разбор набора дальше
			continue;
		}
		// Если довод видом схож с числом, значением он не является
		if(this->_settings.negative && this->numeric(item)){
			// Устанавливаем вид лексемы позиционным доводом
			lexeme.type = token_t::OPERAND;
			// Устанавливаем содержимое позиционного довода
			lexeme.value = item;
			// Увеличиваем число выданных лексем разбора
			count++;
			// Выполняем выдачу разобранной лексемы
			if(!callback(lexeme))
				// Выходим из метода, разбор остановлен
				return false;
			// Продолжаем разбор набора дальше
			continue;
		}
		// Получаем число ведущих тире у довода набора
		const size_t dashes = ((item.length() > 1) && (item.at(1) == '-') ? 2 : 1);
		// Если довод состоит из одних лишь тире
		if(item.length() == dashes){
			// Если доводом является признак конца именованных параметров
			if((dashes == 2) && this->_settings.terminus){
				// Отмечаем, что конец именованных параметров пройден
				terminated = true;
				// Устанавливаем вид лексемы признаком конца параметров
				lexeme.type = token_t::TERMINUS;
				// Увеличиваем число выданных лексем разбора
				count++;
				// Выполняем выдачу разобранной лексемы
				if(!callback(lexeme))
					// Выходим из метода, разбор остановлен
					return false;
				// Продолжаем разбор набора дальше
				continue;
			}
			// Выполняем извещение об отказе разбора
			if(!notify(failure, error_t::EMPTY_KEY, location_t(i, 0)))
				// Выходим из метода, разбор остановлен
				return false;
			// Продолжаем разбор набора дальше
			continue;
		}
		// Получаем запись довода без ведущих тире
		const string_view body(item.data() + dashes, item.length() - dashes);
		// Выполняем поиск разделителя имени со значением
		const size_t pos = body.find('=');
		// Получаем имя параметра из записи довода
		lexeme.key = ((pos != string_view::npos) ? body.substr(0, pos) : body);
		// Если имя параметра пусто вовсе
		if(lexeme.key.empty()){
			// Выполняем извещение об отказе разбора
			if(!notify(failure, error_t::EMPTY_KEY, location_t(i, dashes)))
				// Выходим из метода, разбор остановлен
				return false;
			// Продолжаем разбор набора дальше
			continue;
		}
		// Если длина имени параметра превысила предел разбора
		if((this->_settings.maxKey > 0) && (lexeme.key.length() > this->_settings.maxKey)){
			// Выполняем извещение об отказе разбора
			if(!notify(failure, error_t::LONG_KEY, location_t(i, dashes)))
				// Выходим из метода, разбор остановлен
				return false;
			// Продолжаем разбор набора дальше
			continue;
		}
		// Устанавливаем вид лексемы именованным параметром
		lexeme.type = token_t::PARAM;
		// Если разделитель имени со значением найден
		if(pos != string_view::npos){
			// Отмечаем, что значение параметру подано
			lexeme.assigned = true;
			// Устанавливаем значение параметра из записи довода
			lexeme.value = body.substr(pos + 1);
		// Если значение подано следующим доводом набора
		} else if((i + 1) < items.size()) {
			// Получаем довод, следующий за именем параметра
			const string & next = items.at(i + 1);
			// Если следующий довод значением быть может
			if(next.empty() || (next.front() != '-') || (this->_settings.negative && this->numeric(next))){
				// Отмечаем, что значение параметру подано
				lexeme.assigned = true;
				// Устанавливаем значение параметра следующим доводом
				lexeme.value = next;
				// Пропускаем довод, взятый значением
				i++;
			}
		}
		// Если длина значения параметра превысила предел разбора
		if(lexeme.assigned && (this->_settings.maxValue > 0) && (lexeme.value.length() > this->_settings.maxValue)){
			// Выполняем извещение об отказе разбора
			if(!notify(failure, error_t::LONG_VALUE, lexeme.location))
				// Выходим из метода, разбор остановлен
				return false;
			// Продолжаем разбор набора дальше
			continue;
		}
		// Увеличиваем число выданных лексем разбора
		count++;
		// Выполняем выдачу разобранной лексемы
		if(!callback(lexeme))
			// Выходим из метода, разбор остановлен
			return false;
	}
	// Сообщаем, что разбор набора выполнен
	return true;
}

/**
 * @brief Метод разбора текстового потока
 *
 * @param text     текст для разбора
 * @param callback отзыв выдачи разобранной лексемы
 * @param failure  отзыв извещения об отказе разбора
 * @return         результат разбора
 *
 */
bool awh::args::Lexer::parse(const string_view text, const callback_t & callback, const failure_t & failure) const noexcept {
	// Контейнер слов, собранных разрезом текста
	vector <string> items;
	// Выполняем разрез поданного текста на слова
	if(!this->split(text, items, failure))
		// Выходим из метода, разрез текста остановлен
		return false;
	// Выполняем разбор собранных слов тем же разборщиком
	return this->parse(items, callback, failure);
}

/**
 * @brief Метод извлечения настроек разбора параметров
 *
 * @return настройки разбора параметров
 *
 */
auto awh::args::Lexer::settings() const noexcept -> const settings_t & {
	// Выводим настройки разбора параметров
	return this->_settings;
}

/**
 * @brief Метод установки настроек разбора параметров
 *
 * @param settings настройки разбора параметров
 *
 */
void awh::args::Lexer::settings(const settings_t & settings) noexcept {
	// Устанавливаем настройки разбора параметров
	this->_settings = settings;
}
