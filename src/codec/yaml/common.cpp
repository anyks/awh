/**
 * @file common.cpp
 * @date 2026-08-17
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
 * @brief Общие объявления контейнера YAML — описания кодов отказов, названия видов и
 *        событий, разрешение вида скалярного значения по его записи и выбор вида записи
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cmath>
#include <cerrno>
#include <cstdlib>
#include <limits>

/**
 * Подключаем заголовочные файлы модуля
 */
#include <codec/yaml/common.hpp>
#include <codec/yaml/encoding.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Используем пространство имён контейнера YAML
 */
using namespace awh::codec::yaml;

/**
 * @brief Внутренние помощники разрешения видов скалярных значений
 *
 */
namespace {
	/**
	 * @brief Функция сличения записи с одним из перечисленных написаний
	 *
	 * @details Сличение ведётся дословно, без приведения к одному регистру: описание
	 *          дозволяет ровно три написания каждого слова - строчными, с прописной буквы
	 *          и прописными, - а `nULL` не признаёт пустым значением
	 *
	 * @param text     сличаемая запись значения
	 * @param variants перечень допустимых написаний, оканчивающийся пустым указателем
	 * @return         признак совпадения записи с одним из написаний
	 *
	 */
	static bool matches(const string_view text, const char * const * variants) noexcept {
		/**
		 * Выполняем перебор всех допустимых написаний
		 */
		for(const char * const * variant = variants; * variant != nullptr; variant++){
			/**
			 * Если запись совпадает с очередным написанием
			 */
			if(text.compare(* variant) == 0)
				// Выводим признак совпадения записи
				return true;
		}
		// Выводим признак несовпадения записи
		return false;
	}
	/**
	 * @brief Функция проверки того, что все знаки записи суть десятичные цифры
	 *
	 * @param text  проверяемая запись числа
	 * @param score признак дозволенности знака подчёркивания между цифрами
	 * @return      признак того, что запись состоит из одних цифр
	 *
	 */
	static bool digits(const string_view text, const bool score) noexcept {
		/**
		 * Если запись пуста
		 */
		if(text.empty())
			// Выводим признак несоответствия записи
			return false;
		// Количество встреченных цифр
		size_t count = 0;
		/**
		 * Выполняем перебор всех знаков записи числа
		 */
		for(const char letter : text){
			/**
			 * Если знак является десятичной цифрой
			 */
			if((letter >= '0') && (letter <= '9')){
				// Выполняем учёт встреченной цифры
				count++;
				// Выполняем переход к следующему знаку записи
				continue;
			}
			/**
			 * Если знак является дозволенным знаком подчёркивания
			 *
			 * @note Наречие 1.1 дозволяет разделять разряды знаком подчёркивания, наречие
			 *       1.2 не дозволяет вовсе, и оттого дозволенность задаётся доводом
			 */
			if(score && (letter == '_'))
				// Выполняем переход к следующему знаку записи
				continue;
			// Выводим признак несоответствия записи
			return false;
		}
		// Выводим признак того, что запись несёт хотя бы одну цифру
		return (count > 0);
	}
	/**
	 * @brief Функция проверки записи числа в заданной системе счисления
	 *
	 * @param text  проверяемая запись числа без обозначения системы счисления
	 * @param radix основание системы счисления
	 * @param score признак дозволенности знака подчёркивания между цифрами
	 * @return      признак соответствия записи системе счисления
	 *
	 */
	static bool based(const string_view text, const uint8_t radix, const bool score) noexcept {
		/**
		 * Если запись пуста
		 */
		if(text.empty())
			// Выводим признак несоответствия записи
			return false;
		// Количество встреченных цифр
		size_t count = 0;
		/**
		 * Выполняем перебор всех знаков записи числа
		 */
		for(const char letter : text){
			// Величина, отвечающая знаку записи
			uint8_t value = 0;
			/**
			 * Если знак является десятичной цифрой
			 */
			if((letter >= '0') && (letter <= '9'))
				// Получаем величину, отвечающую цифре
				value = static_cast <uint8_t> (letter - '0');
			/**
			 * Если знак является строчной буквой шестнадцатеричной записи
			 */
			else if((letter >= 'a') && (letter <= 'f'))
				// Получаем величину, отвечающую букве
				value = static_cast <uint8_t> ((letter - 'a') + 10);
			/**
			 * Если знак является прописной буквой шестнадцатеричной записи
			 */
			else if((letter >= 'A') && (letter <= 'F'))
				// Получаем величину, отвечающую букве
				value = static_cast <uint8_t> ((letter - 'A') + 10);
			/**
			 * Если знак является дозволенным знаком подчёркивания
			 */
			else if(score && (letter == '_'))
				// Выполняем переход к следующему знаку записи
				continue;
			/**
			 * Если знак цифрой не является вовсе
			 */
			else return false;
			/**
			 * Если величина знака выходит за основание системы счисления
			 */
			if(value >= radix)
				// Выводим признак несоответствия записи
				return false;
			// Выполняем учёт встреченной цифры
			count++;
		}
		// Выводим признак того, что запись несёт хотя бы одну цифру
		return (count > 0);
	}
	/**
	 * @brief Функция проверки записи дробного числа
	 *
	 * @details Проверяется лишь построение записи, преобразование не выполняется:
	 *          дозволяются целая часть, дробная часть и порядок, причём хотя бы одна цифра
	 *          обязана стоять до порядка
	 *
	 * @param text  проверяемая запись числа без знака
	 * @param score признак дозволенности знака подчёркивания между цифрами
	 * @return      признак соответствия записи дробному числу
	 *
	 */
	static bool floating(const string_view text, const bool score) noexcept {
		/**
		 * Если запись пуста
		 */
		if(text.empty())
			// Выводим признак несоответствия записи
			return false;
		// Количество встреченных цифр целой и дробной частей
		size_t count = 0;
		// Признак того, что точка уже встречена
		bool point = false;
		// Признак того, что буква порядка уже встречена
		bool power = false;
		// Количество встреченных цифр порядка
		size_t digits = 0;
		// Признак того, что знак порядка уже встречен
		bool signed_ = false;
		/**
		 * Выполняем перебор всех знаков записи числа
		 */
		for(size_t i = 0; i < text.size(); i++){
			// Получаем очередной знак записи числа
			const char letter = text.at(i);
			/**
			 * Если знак является десятичной цифрой
			 */
			if((letter >= '0') && (letter <= '9')){
				/**
				 * Если цифра принадлежит порядку
				 */
				if(power)
					// Выполняем учёт цифры порядка
					digits++;
				// Выполняем учёт цифры целой либо дробной части
				else count++;
				// Выполняем переход к следующему знаку записи
				continue;
			}
			/**
			 * Если знак является дозволенным знаком подчёркивания
			 */
			if(score && (letter == '_') && !power)
				// Выполняем переход к следующему знаку записи
				continue;
			/**
			 * Если знак является точкой, отделяющей дробную часть
			 */
			if((letter == '.') && !point && !power){
				// Запоминаем встреченную точку
				point = true;
				// Выполняем переход к следующему знаку записи
				continue;
			}
			/**
			 * Если знак является буквой порядка
			 */
			if(((letter == 'e') || (letter == 'E')) && !power && (count > 0)){
				// Запоминаем встреченную букву порядка
				power = true;
				// Выполняем переход к следующему знаку записи
				continue;
			}
			/**
			 * Если знак является знаком порядка, стоящим сразу за буквою его
			 */
			if(((letter == '-') || (letter == '+')) && power && !signed_ && (digits == 0)){
				// Запоминаем встреченный знак порядка
				signed_ = true;
				// Выполняем переход к следующему знаку записи
				continue;
			}
			// Выводим признак несоответствия записи
			return false;
		}
		/**
		 * Если запись не несёт ни одной цифры целой либо дробной части
		 */
		if(count == 0)
			// Выводим признак несоответствия записи
			return false;
		/**
		 * Если запись несёт букву порядка, а цифр порядка не несёт
		 */
		if(power && (digits == 0))
			// Выводим признак несоответствия записи
			return false;
		// Выводим признак того, что запись отвечает дробному числу
		return (point || power);
	}
	/**
	 * @brief Функция выбора самого узкого вида, целое число вмещающего
	 *
	 * @param negative  признак записи числа со знаком минус
	 * @param collected разобранное целое число без знака
	 * @param result    разобранное число всеми видами его
	 * @return          самый узкий вид, число вмещающий
	 *
	 */
	static type_t fitted(const bool negative, const uint64_t collected, numeric_t & result) noexcept {
		/**
		 * Если запись несёт знак минус
		 */
		if(negative){
			/**
			 * Если число за предел целого со знаком вышло
			 */
			if(collected > (static_cast <uint64_t> (numeric_limits <int64_t>::max()) + 1ull)){
				// Запоминаем разобранное число дробным приближением его
				result.real = -static_cast <double> (collected);
				// Выводим вид числа, ни в один родной вид не вместимого
				return type_t::EXTENDED;
			}
			// Запоминаем разобранное число целым со знаком
			result.integer = ((collected == (static_cast <uint64_t> (numeric_limits <int64_t>::max()) + 1ull)) ?
			 numeric_limits <int64_t>::min() : -static_cast <int64_t> (collected));
			// Запоминаем разобранное число дробным видом
			result.real = static_cast <double> (result.integer);
			// Запоминаем разобранное число целым без знака
			result.natural = static_cast <uint64_t> (result.integer);
			/**
			 * Если число вмещается в один байт со знаком
			 */
			if(result.integer >= numeric_limits <int8_t>::lowest())
				// Выводим вид целого числа одного байта
				return type_t::INT8;
			/**
			 * Если число вмещается в два байта со знаком
			 */
			if(result.integer >= numeric_limits <int16_t>::lowest())
				// Выводим вид целого числа двух байтов
				return type_t::INT16;
			/**
			 * Если число вмещается в четыре байта со знаком
			 */
			if(result.integer >= numeric_limits <int32_t>::lowest())
				// Выводим вид целого числа четырёх байтов
				return type_t::INT32;
			// Выводим вид целого числа восьми байтов
			return type_t::INT64;
		}
		// Запоминаем разобранное число целым без знака
		result.natural = collected;
		// Запоминаем разобранное число целым со знаком
		result.integer = static_cast <int64_t> (collected);
		// Запоминаем разобранное число дробным видом
		result.real = static_cast <double> (collected);
		/**
		 * Если число вмещается в один байт без знака
		 */
		if(collected <= numeric_limits <uint8_t>::max())
			// Выводим вид целого числа одного байта
			return type_t::UINT8;
		/**
		 * Если число вмещается в два байта без знака
		 */
		if(collected <= numeric_limits <uint16_t>::max())
			// Выводим вид целого числа двух байтов
			return type_t::UINT16;
		/**
		 * Если число вмещается в четыре байта без знака
		 */
		if(collected <= numeric_limits <uint32_t>::max())
			// Выводим вид целого числа четырёх байтов
			return type_t::UINT32;
		// Выводим вид целого числа восьми байтов
		return type_t::UINT64;
	}
	/**
	 * @brief Функция проверки записи числа в шестидесятиричной записи наречия 1.1
	 *
	 * @details Записью этой пользуются для времени и углов: `190:20:30` есть число
	 *          685230, а `12:30` - число 750. Наречие 1.2 её не знает вовсе, и там это
	 *          обыкновенная строка
	 *
	 * @param text проверяемая запись числа без знака
	 * @return     признак соответствия записи шестидесятиричному числу
	 *
	 */
	static bool sexagesimal(const string_view text) noexcept {
		/**
		 * Если запись не несёт разделителя частей
		 */
		if(text.find(':') == string_view::npos)
			// Выводим признак несоответствия записи
			return false;
		// Смещение начала очередной части записи
		size_t offset = 0;
		// Количество разобранных частей записи
		size_t parts = 0;
		/**
		 * Выполняем перебор всех частей записи числа
		 */
		while(offset <= text.size()){
			// Разыскиваем разделитель очередной части записи
			const size_t position = text.find(':', offset);
			// Получаем очередную часть записи числа
			const string_view part = text.substr(offset, ((position == string_view::npos) ? string_view::npos : (position - offset)));
			/**
			 * Если часть записи не является числом
			 *
			 * @note Последняя часть вправе быть дробной: запись `12:30.5` описанием
			 *       дозволена, и знаменует она 750.5
			 */
			if(!digits(part, true) && !((position == string_view::npos) && floating(part, true)))
				// Выводим признак несоответствия записи
				return false;
			// Выполняем учёт разобранной части записи
			parts++;
			/**
			 * Если части записи исчерпаны
			 */
			if(position == string_view::npos)
				// Выходим из перебора частей записи
				break;
			// Выполняем переход к следующей части записи
			offset = (position + 1);
		}
		// Выводим признак того, что запись несёт хотя бы две части
		return (parts > 1);
	}
}

/**
 * @brief Функция получения текстового описания кода отказа
 *
 * @param error код отказа разбора
 * @return      текстовое описание кода отказа
 *
 */
const char * awh::codec::yaml::message(const error_t error) noexcept {
	/**
	 * Определяем код отказа разбора
	 */
	switch(static_cast <uint8_t> (error)){
		// Если ошибок не обнаружено
		case static_cast <uint8_t> (error_t::NONE): return "ошибок не обнаружено";
		// Если произошла внутренняя ошибка разбора
		case static_cast <uint8_t> (error_t::INTERNAL): return "внутренняя ошибка разбора";
		// Если текст оборвался посреди значения
		case static_cast <uint8_t> (error_t::UNEXPECTED_EOF): return "текст оборвался посреди значения";
		// Если знак недопустим в этом месте текста
		case static_cast <uint8_t> (error_t::INVALID_CHARACTER): return "знак недопустим в этом месте текста";
		// Если последовательность байтов не отвечает объявленной кодировке
		case static_cast <uint8_t> (error_t::INVALID_ENCODING): return "последовательность байтов не отвечает объявленной кодировке";
		// Если объявленная кодировка не поддерживается
		case static_cast <uint8_t> (error_t::UNSUPPORTED_ENCODING): return "объявленная кодировка не поддерживается";
		// Если отступ не отвечает ни одному из открытых уровней
		case static_cast <uint8_t> (error_t::INVALID_INDENTATION): return "отступ не отвечает ни одному из открытых уровней";
		// Если отступ содержит знак горизонтальной подачи
		case static_cast <uint8_t> (error_t::TAB_IN_INDENTATION): return "отступ содержит знак горизонтальной подачи, описанием запрещённый";
		// Если скалярное значение не закрыто оградой
		case static_cast <uint8_t> (error_t::UNTERMINATED_SCALAR): return "скалярное значение не закрыто оградой до конца текста";
		// Если отменяющая последовательность не опознана
		case static_cast <uint8_t> (error_t::INVALID_ESCAPE): return "отменяющая последовательность не опознана";
		// Если запись знака Юникода содержит недопустимые знаки
		case static_cast <uint8_t> (error_t::INVALID_UNICODE): return "запись знака Юникода содержит недопустимые знаки";
		// Если суррогат не образует пары
		case static_cast <uint8_t> (error_t::UNPAIRED_SURROGATE): return "суррогат не образует пары";
		// Если заголовок блочного значения построен ошибочно
		case static_cast <uint8_t> (error_t::INVALID_BLOCK_HEADER): return "заголовок блочного значения построен ошибочно";
		// Если запись числа не отвечает действующей схеме
		case static_cast <uint8_t> (error_t::INVALID_NUMBER): return "запись числа не отвечает действующей схеме";
		// Если число не представимо затребованным видом
		case static_cast <uint8_t> (error_t::NUMBER_OUT_OF_RANGE): return "число не представимо затребованным видом";
		// Если содержимое метки двоичного значения записано ошибочно
		case static_cast <uint8_t> (error_t::INVALID_BINARY): return "содержимое метки !!binary не отвечает записи base64";
		// Если отметка времени построена ошибочно
		case static_cast <uint8_t> (error_t::INVALID_STAMP): return "ошибочное построение отметки времени";
		// Если ожидалось значение
		case static_cast <uint8_t> (error_t::EXPECTED_VALUE): return "ожидалось значение";
		// Если ожидалось имя пары отображения
		case static_cast <uint8_t> (error_t::EXPECTED_KEY): return "ожидалось имя пары отображения";
		// Если ожидалось двоеточие после имени пары
		case static_cast <uint8_t> (error_t::EXPECTED_COLON): return "ожидалось двоеточие после имени пары";
		// Если ожидалась запятая либо закрывающая скобка
		case static_cast <uint8_t> (error_t::EXPECTED_COMMA): return "ожидалась запятая либо закрывающая скобка поточного построения";
		// Если поточное построение не закрыто скобкой
		case static_cast <uint8_t> (error_t::UNCLOSED_FLOW): return "поточное построение не закрыто скобкой";
		// Если перечень и отображение смешаны на одном уровне
		case static_cast <uint8_t> (error_t::MIXED_COLLECTION): return "перечень и отображение смешаны на одном уровне";
		// Если имя пары отображения объявлено повторно
		case static_cast <uint8_t> (error_t::DUPLICATE_KEY): return "имя пары отображения объявлено повторно";
		// Если составное имя пары встречено при запрещённых составных именах
		case static_cast <uint8_t> (error_t::COMPLEX_KEY): return "составное имя пары при запрещённых составных именах";
		// Если ссылка указывает на метку, ещё не объявленную
		case static_cast <uint8_t> (error_t::UNKNOWN_ALIAS): return "ссылка указывает на метку, ещё не объявленную";
		// Если метка с таким именем уже объявлена
		case static_cast <uint8_t> (error_t::DUPLICATE_ANCHOR): return "метка с таким именем уже объявлена";
		// Если ссылка указывает сама на себя
		case static_cast <uint8_t> (error_t::RECURSIVE_ALIAS): return "ссылка указывает сама на себя через цепочку меток";
		// Если раскрытие ссылок порождает больше узлов, чем дозволено
		case static_cast <uint8_t> (error_t::EXPANSION_EXCEEDED): return "раскрытие ссылок порождает больше узлов, чем дозволено";
		// Если метка типа построена ошибочно
		case static_cast <uint8_t> (error_t::INVALID_TAG): return "метка типа построена ошибочно";
		// Если сокращение метки типа не объявлено директивой
		case static_cast <uint8_t> (error_t::UNKNOWN_TAG_HANDLE): return "сокращение метки типа не объявлено директивой %TAG";
		// Если содержимое не отвечает виду, заданному меткой типа
		case static_cast <uint8_t> (error_t::TAG_MISMATCH): return "содержимое не отвечает виду, заданному меткой типа";
		// Если директива построена ошибочно
		case static_cast <uint8_t> (error_t::INVALID_DIRECTIVE): return "директива построена ошибочно";
		// Если объявленное наречие не поддерживается
		case static_cast <uint8_t> (error_t::UNSUPPORTED_VERSION): return "наречие, объявленное директивой %YAML, не поддерживается";
		// Если начало нового документа встречено посреди значения
		case static_cast <uint8_t> (error_t::UNEXPECTED_DOCUMENT): return "начало нового документа посреди значения";
		// Если за окончанием документа стоят знаки
		case static_cast <uint8_t> (error_t::TRAILING_CHARACTERS): return "знаки за окончанием документа";
		// Если глубина вложенности превышает допустимую
		case static_cast <uint8_t> (error_t::DEPTH_EXCEEDED): return "глубина вложенности превышает допустимую";
		// Если длина скалярного значения превышает допустимую
		case static_cast <uint8_t> (error_t::SCALAR_TOO_LONG): return "длина скалярного значения превышает допустимую";
		// Если длина записи числа превышает допустимую
		case static_cast <uint8_t> (error_t::NUMBER_TOO_LONG): return "длина записи числа превышает допустимую";
		// Если длина имени метки превышает допустимую
		case static_cast <uint8_t> (error_t::ANCHOR_TOO_LONG): return "длина имени метки превышает допустимую";
		// Если количество узлов документа превышает допустимое
		case static_cast <uint8_t> (error_t::TOO_MANY_NODES): return "количество узлов документа превышает допустимое";
		// Если текст пуст, а документ затребован
		case static_cast <uint8_t> (error_t::EMPTY_TEXT): return "текст пуст, а документ затребован";
		// Если превышен предел, заданный настройками разбора
		case static_cast <uint8_t> (error_t::OVERFLOW_LIMIT): return "превышен предел, заданный настройками разбора";
		// Если настройки записи противоречат толкованию читающего
		case static_cast <uint8_t> (error_t::CONFLICTING_SETTINGS): return "настройки записи противоречат толкованию читающего";
	}
	// Выводим описание неизвестного кода отказа
	return "неизвестный код отказа";
}
/**
 * @brief Функция получения названия вида узла
 *
 * @param kind вид узла документа
 * @return     название вида узла
 *
 */
const char * awh::codec::yaml::name(const kind_t kind) noexcept {
	/**
	 * Определяем вид узла документа
	 */
	switch(static_cast <uint8_t> (kind)){
		// Если узел не определён
		case static_cast <uint8_t> (kind_t::NONE): return "none";
		// Если узел является пустым значением
		case static_cast <uint8_t> (kind_t::NUL): return "null";
		// Если узел является логическим значением
		case static_cast <uint8_t> (kind_t::BOOL): return "bool";
		// Если узел является числом
		case static_cast <uint8_t> (kind_t::NUMBER): return "number";
		// Если узел является строкой
		case static_cast <uint8_t> (kind_t::STRING): return "string";
		// Если узел является двоичным содержимым
		case static_cast <uint8_t> (kind_t::BINARY): return "binary";
		// Если узел является отметкой времени
		case static_cast <uint8_t> (kind_t::STAMP): return "stamp";
		// Если узел является перечнем значений
		case static_cast <uint8_t> (kind_t::SEQUENCE): return "sequence";
		// Если узел является отображением пар
		case static_cast <uint8_t> (kind_t::MAPPING): return "mapping";
	}
	// Выводим название неизвестного вида узла
	return "unknown";
}
/**
 * @brief Функция получения названия вида значения
 *
 * @param type вид значения документа
 * @return     название вида значения
 *
 */
const char * awh::codec::yaml::name(const type_t type) noexcept {
	/**
	 * Определяем вид значения документа
	 */
	switch(static_cast <uint32_t> (type)){
		// Если значения нет вовсе
		case static_cast <uint32_t> (type_t::UNDEFINED): return "undefined";
		// Если значение является пустым
		case static_cast <uint32_t> (type_t::NUL): return "null";
		// Если значение является логическим
		case static_cast <uint32_t> (type_t::BOOL): return "bool";
		// Если значение является строкой
		case static_cast <uint32_t> (type_t::STRING): return "string";
		// Если значение является перечнем
		case static_cast <uint32_t> (type_t::SEQUENCE): return "sequence";
		// Если значение является отображением
		case static_cast <uint32_t> (type_t::MAPPING): return "mapping";
		// Если значение является целым со знаком шириною в один байт
		case static_cast <uint32_t> (type_t::INT8): return "int8";
		// Если значение является целым со знаком шириною в два байта
		case static_cast <uint32_t> (type_t::INT16): return "int16";
		// Если значение является целым со знаком шириною в четыре байта
		case static_cast <uint32_t> (type_t::INT32): return "int32";
		// Если значение является целым со знаком шириною в восемь байтов
		case static_cast <uint32_t> (type_t::INT64): return "int64";
		// Если значение является целым без знака шириною в один байт
		case static_cast <uint32_t> (type_t::UINT8): return "uint8";
		// Если значение является целым без знака шириною в два байта
		case static_cast <uint32_t> (type_t::UINT16): return "uint16";
		// Если значение является целым без знака шириною в четыре байта
		case static_cast <uint32_t> (type_t::UINT32): return "uint32";
		// Если значение является целым без знака шириною в восемь байтов
		case static_cast <uint32_t> (type_t::UINT64): return "uint64";
		// Если значение является дробным одинарной точности
		case static_cast <uint32_t> (type_t::FLOAT): return "float";
		// Если значение является дробным двойной точности
		case static_cast <uint32_t> (type_t::DOUBLE): return "double";
		// Если значение является числом, не вместимым ни в один родной вид
		case static_cast <uint32_t> (type_t::EXTENDED): return "extended";
		// Если значение является двоичным содержимым
		case static_cast <uint32_t> (type_t::BINARY): return "binary";
		// Если значение является отметкой времени
		case static_cast <uint32_t> (type_t::STAMP): return "stamp";
	}
	// Выводим название неизвестного вида значения
	return "unknown";
}
/**
 * @brief Функция получения названия события чтения
 *
 * @param event вид события чтения
 * @return      название события чтения
 *
 */
const char * awh::codec::yaml::name(const event_t event) noexcept {
	/**
	 * Определяем вид события чтения
	 */
	switch(static_cast <uint8_t> (event)){
		// Если событие не определено
		case static_cast <uint8_t> (event_t::NONE): return "NONE";
		// Если получено начало потока документов
		case static_cast <uint8_t> (event_t::STREAM_START): return "STREAM_START";
		// Если получен конец потока документов
		case static_cast <uint8_t> (event_t::STREAM_END): return "STREAM_END";
		// Если получено начало документа
		case static_cast <uint8_t> (event_t::DOCUMENT_START): return "DOCUMENT_START";
		// Если получен конец документа
		case static_cast <uint8_t> (event_t::DOCUMENT_END): return "DOCUMENT_END";
		// Если получено начало отображения
		case static_cast <uint8_t> (event_t::MAPPING_START): return "MAPPING_START";
		// Если получен конец отображения
		case static_cast <uint8_t> (event_t::MAPPING_END): return "MAPPING_END";
		// Если получено начало перечня
		case static_cast <uint8_t> (event_t::SEQUENCE_START): return "SEQUENCE_START";
		// Если получен конец перечня
		case static_cast <uint8_t> (event_t::SEQUENCE_END): return "SEQUENCE_END";
		// Если получено скалярное значение
		case static_cast <uint8_t> (event_t::SCALAR): return "SCALAR";
		// Если получена ссылка на метку
		case static_cast <uint8_t> (event_t::ALIAS): return "ALIAS";
		// Если получено примечание
		case static_cast <uint8_t> (event_t::COMMENT): return "COMMENT";
		// Если получена пустая строка
		case static_cast <uint8_t> (event_t::BLANK): return "BLANK";
		// Если текст разобран до конца
		case static_cast <uint8_t> (event_t::FINISH): return "FINISH";
	}
	// Выводим название неизвестного события чтения
	return "UNKNOWN";
}
/**
 * @brief Функция получения вида узла по виду значения
 *
 * @param type вид значения документа
 * @return     вид узла документа
 *
 */
kind_t awh::codec::yaml::kind(const type_t type) noexcept {
	/**
	 * Если значение является числом любого вида
	 */
	if((static_cast <uint32_t> (type) & static_cast <uint32_t> (type_t::NUMBER)) != 0)
		// Выводим вид узла числа
		return kind_t::NUMBER;
	/**
	 * Определяем вид значения документа
	 */
	switch(static_cast <uint32_t> (type)){
		// Если значение является пустым
		case static_cast <uint32_t> (type_t::NUL): return kind_t::NUL;
		// Если значение является логическим
		case static_cast <uint32_t> (type_t::BOOL): return kind_t::BOOL;
		// Если значение является строкой
		case static_cast <uint32_t> (type_t::STRING): return kind_t::STRING;
		// Если значение является двоичным содержимым
		case static_cast <uint32_t> (type_t::BINARY): return kind_t::BINARY;
		// Если значение является отметкой времени
		case static_cast <uint32_t> (type_t::STAMP): return kind_t::STAMP;
		// Если значение является перечнем
		case static_cast <uint32_t> (type_t::SEQUENCE): return kind_t::SEQUENCE;
		// Если значение является отображением
		case static_cast <uint32_t> (type_t::MAPPING): return kind_t::MAPPING;
	}
	// Выводим вид неопределённого узла
	return kind_t::NONE;
}
/**
 * @brief Функция разрешения вида скалярного значения по его записи
 *
 * @param text   разрешаемая запись значения
 * @param schema действующая схема разрешения
 * @return       вид значения, отвечающий записи
 *
 */
type_t awh::codec::yaml::resolve(const string_view text, const schema_t schema) noexcept {
	/**
	 * Если действует схема, признающая одни лишь строки
	 */
	if(schema == schema_t::FAILSAFE)
		// Выводим вид строкового значения
		return type_t::STRING;
	/**
	 * Если запись пуста
	 *
	 * @note Пустая запись есть пустое значение: пара `ключ:` без значения знаменует
	 *       именно его, и в этом YAML расходится с JSON, где пустоты нет вовсе
	 */
	if(text.empty())
		// Выводим вид пустого значения
		return type_t::NUL;
	/**
	 * Если действует схема строгого соответствия правилам JSON
	 */
	if(schema == schema_t::JSON){
		/**
		 * Если запись является пустым значением
		 */
		if(text.compare("null") == 0)
			// Выводим вид пустого значения
			return type_t::NUL;
		/**
		 * Если запись является логическим значением
		 */
		if((text.compare("true") == 0) || (text.compare("false") == 0))
			// Выводим вид логического значения
			return type_t::BOOL;
		// Получаем запись числа без знака
		const string_view number = ((text.front() == '-') ? text.substr(1) : text);
		/**
		 * Если запись является целым либо дробным числом
		 *
		 * @note Правила JSON строже прочих: ведущий плюс запрещён, ведущий нуль запрещён,
		 *       знак подчёркивания запрещён тоже
		 */
		if(!number.empty() && (digits(number, false) || floating(number, false))){
			/**
			 * Если запись несёт ведущий нуль перед иной цифрой
			 */
			if((number.size() > 1) && (number.front() == '0') && (number.at(1) >= '0') && (number.at(1) <= '9'))
				// Выводим вид строкового значения
				return type_t::STRING;
			// Выводим сборный вид числа
			return type_t::NUMBER;
		}
		// Выводим вид строкового значения
		return type_t::STRING;
	}
	/**
	 * Перечень написаний пустого значения ядровой схемы
	 */
	static const char * const NULLS[] = {"~", "null", "Null", "NULL", nullptr};
	/**
	 * Если запись является пустым значением
	 */
	if(matches(text, NULLS))
		// Выводим вид пустого значения
		return type_t::NUL;
	/**
	 * Перечень написаний логического значения ядровой схемы
	 */
	static const char * const BOOLEANS[] = {"true", "True", "TRUE", "false", "False", "FALSE", nullptr};
	/**
	 * Если запись является логическим значением
	 */
	if(matches(text, BOOLEANS))
		// Выводим вид логического значения
		return type_t::BOOL;
	/**
	 * Если действует схема наречия 1.1
	 */
	if(schema == schema_t::LEGACY){
		/**
		 * Перечень написаний логического значения наречия 1.1
		 *
		 * @note Написания эти и породили беду, известную под именем норвежской: страна
		 *       NO, записанная без ограды, обращается в ложь
		 */
		static const char * const LEGACY_BOOLEANS[] = {
			"y", "Y", "yes", "Yes", "YES", "n", "N", "no", "No", "NO",
			"on", "On", "ON", "off", "Off", "OFF", nullptr
		};
		/**
		 * Если запись является логическим значением наречия 1.1
		 */
		if(matches(text, LEGACY_BOOLEANS))
			// Выводим вид логического значения
			return type_t::BOOL;
	}
	// Признак дозволенности знака подчёркивания между разрядами
	const bool score = (schema == schema_t::LEGACY);
	// Получаем запись числа без знака
	const string_view number = (((text.front() == '-') || (text.front() == '+')) ? text.substr(1) : text);
	/**
	 * Если запись без знака пуста
	 */
	if(number.empty())
		// Выводим вид строкового значения
		return type_t::STRING;
	/**
	 * Перечень написаний бесконечности
	 */
	static const char * const INFINITIES[] = {".inf", ".Inf", ".INF", nullptr};
	/**
	 * Если запись является бесконечностью
	 */
	if(matches(number, INFINITIES))
		// Выводим вид дробного числа двойной точности
		return type_t::DOUBLE;
	/**
	 * Перечень написаний нечисловой величины
	 *
	 * @note Величина эта знака не имеет: запись `-.nan` описанием не предусмотрена, и
	 *       оттого сличается она с записью целиком, а не с записью без знака
	 */
	static const char * const NOT_NUMBERS[] = {".nan", ".NaN", ".NAN", nullptr};
	/**
	 * Если запись является нечисловой величиной
	 */
	if(matches(text, NOT_NUMBERS))
		// Выводим вид дробного числа двойной точности
		return type_t::DOUBLE;
	/**
	 * Если запись является числом шестнадцатеричной системы счисления
	 */
	if((number.size() > 2) && (number.at(0) == '0') && ((number.at(1) == 'x') || (number.at(1) == 'X')) && based(number.substr(2), 16, score))
		// Выводим сборный вид числа
		return type_t::NUMBER;
	/**
	 * Если запись является числом восьмеричной системы счисления наречия 1.2
	 */
	if((number.size() > 2) && (number.at(0) == '0') && (number.at(1) == 'o') && based(number.substr(2), 8, score))
		// Выводим сборный вид числа
		return type_t::NUMBER;
	/**
	 * Если действует схема наречия 1.1
	 */
	if(schema == schema_t::LEGACY){
		/**
		 * Если запись является числом двоичной системы счисления
		 */
		if((number.size() > 2) && (number.at(0) == '0') && ((number.at(1) == 'b') || (number.at(1) == 'B')) && based(number.substr(2), 2, score))
			// Выводим сборный вид числа
			return type_t::NUMBER;
		/**
		 * Если запись является числом восьмеричной системы счисления наречия 1.1
		 *
		 * @note Ведущий нуль знаменует здесь восьмеричную запись, и `0777` есть 511.
		 *       Наречие 1.2 такую запись числом не признаёт вовсе - там это строка
		 */
		if((number.size() > 1) && (number.at(0) == '0') && based(number.substr(1), 8, score))
			// Выводим сборный вид числа
			return type_t::NUMBER;
		/**
		 * Если запись является числом шестидесятиричной записи
		 */
		if(sexagesimal(number))
			// Выводим сборный вид числа
			return type_t::NUMBER;
	}
	/**
	 * Если запись является целым десятичным числом
	 */
	if(digits(number, score))
		// Выводим сборный вид числа
		return type_t::NUMBER;
	/**
	 * Если запись является дробным числом
	 */
	if(floating(number, score))
		// Выводим сборный вид числа
		return type_t::NUMBER;
	// Выводим вид строкового значения
	return type_t::STRING;
}
/**
 * @brief Функция проверки записи имени метки либо ссылки
 *
 * @param text проверяемое имя метки
 * @return     признак допустимости имени метки
 *
 */
bool awh::codec::yaml::anchored(const string_view text) noexcept {
	/**
	 * Если имя метки пусто
	 */
	if(text.empty())
		// Выводим признак недопустимости имени метки
		return false;
	/**
	 * Если длина имени метки превышает допустимую
	 */
	if(text.size() > MAX_ANCHOR)
		// Выводим признак недопустимости имени метки
		return false;
	/**
	 * Выполняем перебор всех знаков имени метки
	 */
	for(const char letter : text){
		/**
		 * Определяем очередной знак имени метки
		 */
		switch(letter){
			// Если знак является пробельным либо открывающим поточное построение
			case ' ':
			case '\t':
			case '\r':
			case '\n':
			case '[':
			case ']':
			case '{':
			case '}':
			case ',':
				// Выводим признак недопустимости имени метки
				return false;
		}
	}
	// Выводим признак допустимости имени метки
	return true;
}
/**
 * @brief Функция выбора вида записи скалярного значения
 *
 * @param text   записываемое значение
 * @param schema действующая схема разрешения
 * @param key    признак того, что значение записывается именем пары
 * @return       наименее навязчивый допустимый вид записи
 *
 */
style_t awh::codec::yaml::quoting(const string_view text, const schema_t schema, const bool key) noexcept {
	/**
	 * Если значение пусто
	 *
	 * @note Пустое значение, записанное без ограды, прочтётся пустым значением, а не
	 *       пустой строкой: разница эта значаща, и оттого пустая строка ограду получает
	 */
	if(text.empty())
		// Выводим вид записи одинарной оградой
		return style_t::SINGLE;
	/**
	 * Выполняем перебор всех знаков записываемого значения в поиске неотменимых
	 *
	 * @details Перебор этот идёт прежде разрешения вида, а не за ним: знак управляющий
	 *          записать без отмены нельзя вовсе, и двойная ограда ему нужна независимо от
	 *          того, чем значение разрешается. Нашёл это ворошитель сличением перезаписи:
	 *          значение с переводом строки получало ограду одинарную и растекалось по
	 *          строкам, обратным чтением уже не читаясь
	 */
	for(size_t i = 0; i < text.size();){
		// Знак Юникода, очередной последовательностью записанный
		uint32_t code = 0;
		// Получаем длину последовательности очередного знака
		const size_t length = decode(text, i, code);
		/**
		 * Если последовательность знака построена ошибочно
		 *
		 * @note Записать такое можно лишь отменяющей последовательностью побайтно, и ограда
		 *       ему нужна двойная
		 */
		if(length == 0)
			// Выводим вид записи двойной оградой
			return style_t::DOUBLE;
		/**
		 * Если знак печатным не является
		 *
		 * @note Описанием текст печатными знаками и ограничен, и знак иной записывается
		 *       лишь отменяющей последовательностью. Проверка идёт по знаку Юникода, а не
		 *       по байту: знак `U+0081` записан байтами `C2 81`, из коих управляющим не
		 *       является ни один. Нашёл это ворошитель сличением перезаписи
		 */
		if(!printable(code))
			// Выводим вид записи двойной оградой
			return style_t::DOUBLE;
		/**
		 * Если знак является переводом строки либо горизонтальной подачей
		 *
		 * @note Печатными они числятся - описание дозволяет их прямо, - однако без ограды
		 *       передать их нельзя: перевод строки оборвал бы запись, а подача съелась бы
		 *       снятием отступа
		 */
		if((code == 0x09) || (code == 0x0A) || (code == 0x0D))
			// Выводим вид записи двойной оградой
			return style_t::DOUBLE;
		// Выполняем переход к следующему знаку записи
		i += length;
	}
	/**
	 * Если значение разрешается видом, строкою не являющимся
	 *
	 * @note Проверка эта - оборотная сторона разрешения: запись, которую чтение разрешит
	 *       числом либо логическим значением, обязана получить ограду, иначе строка `12`
	 *       вернётся числом
	 */
	if(resolve(text, schema) != type_t::STRING)
		// Выводим вид записи одинарной оградой
		return style_t::SINGLE;
	/**
	 * Выполняем перебор всех знаков записываемого значения
	 */
	for(size_t i = 0; i < text.size(); i++){
		// Получаем очередной знак записываемого значения
		const char letter = text.at(i);
		/**
		 * Определяем очередной знак записываемого значения
		 */
		switch(letter){
			/**
			 * Если знак открывает поточное построение
			 *
			 * @note Знаки эти значащи лишь внутри поточного построения, а в блочном
			 *       безобидны. Ограду они получают и там, и там: запись вправе собрать
			 *       перечень поточным построением, и значение, годное блочному построению,
			 *       но негодное поточному, обратилось бы в ловушку при смене вида
			 */
			case '[':
			case ']':
			case '{':
			case '}':
			case ',':
				// Выводим вид записи одинарной оградой
				return style_t::SINGLE;
			/**
			 * Если знак является указателем, значащим лишь первым знаком записи
			 *
			 * @note Знаки эти открывают метку, ссылку, метку типа, блочное значение,
			 *       директиву и примечание - но лишь стоя первыми. Внутри записи они
			 *       обыкновенны: `100%` есть строка, а `b#c` есть строка `b#c`, ибо
			 *       примечание открывается знаком решётки лишь за пробельным знаком
			 */
			case '&':
			case '*':
			case '!':
			case '|':
			case '>':
			case '%':
			case '@':
			case '`':
			case '\'':
			case '"':
			case '#': {
				/**
				 * Если знак стоит первым знаком записи
				 */
				if(i == 0)
					// Выводим вид записи одинарной оградой
					return style_t::SINGLE;
			} break;
			/**
			 * Если знак является двоеточием
			 *
			 * @note Двоеточие отделяет имя пары от значения лишь тогда, когда за ним стоит
			 *       пробельный знак либо конец записи. Имя пары строже: там двоеточие
			 *       недопустимо вовсе, ибо чтение оборвало бы имя на нём
			 */
			case ':': {
				/**
				 * Если значение записывается именем пары
				 */
				if(key)
					// Выводим вид записи одинарной оградой
					return style_t::SINGLE;
				/**
				 * Если за двоеточием стоит пробельный знак либо конец записи
				 */
				if(((i + 1) >= text.size()) || (text.at(i + 1) == ' ') || (text.at(i + 1) == '\t'))
					// Выводим вид записи одинарной оградой
					return style_t::SINGLE;
			} break;
			/**
			 * Если знак является знаком примечания
			 *
			 * @note Знак этот открывает примечание лишь тогда, когда перед ним стоит
			 *       пробельный знак. Первым знаком записи он тоже недопустим, но случай
			 *       этот разобран выше
			 */
			case ' ': {
				/**
				 * Если за пробелом стоит знак примечания
				 */
				if(((i + 1) < text.size()) && (text.at(i + 1) == '#'))
					// Выводим вид записи одинарной оградой
					return style_t::SINGLE;
			} break;
		}
	}
	/**
	 * Если значение начинается либо оканчивается пробельным знаком
	 *
	 * @note Обвязка пробелами при чтении без ограды снимается, и значение вернулось бы
	 *       иным
	 */
	if((text.front() == ' ') || (text.back() == ' '))
		// Выводим вид записи одинарной оградой
		return style_t::SINGLE;
	/**
	 * Если значение начинается со знака, открывающего иное построение
	 */
	switch(text.front()){
		// Если знак открывает объявление перечня, вопрос составного имени либо разделитель
		case '-':
		case '?':
		case ':': {
			/**
			 * Если за знаком стоит пробельный знак либо знак этот единственный
			 */
			if((text.size() == 1) || (text.at(1) == ' ') || (text.at(1) == '\t'))
				// Выводим вид записи одинарной оградой
				return style_t::SINGLE;
		} break;
	}
	// Выводим вид записи без ограды
	return style_t::PLAIN;
}
/**
 * @brief Функция разбора записи числа к самому узкому вмещающему виду
 *
 * @param text   разбираемая запись числа
 * @param schema действующая схема разрешения
 * @param result разобранное число
 * @return       вид разобранного числа, `UNDEFINED` - запись числом не является
 *
 */
type_t awh::codec::yaml::narrow(const string_view text, const schema_t schema, numeric_t & result) noexcept {
	/**
	 * Если запись числом не является вовсе
	 *
	 * @note Разрешение стоит прежде разбора нарочно: своды правил у них разойтись не
	 *       вправе, и разбор берётся лишь за то, что разрешение числом признало
	 */
	if(!(static_cast <uint32_t> (resolve(text, schema)) & static_cast <uint32_t> (type_t::NUMBER)))
		// Выводим признак того, что запись числом не является
		return type_t::UNDEFINED;
	// Признак записи числа со знаком минус
	const bool negative = (!text.empty() && (text.front() == '-'));
	// Получаем запись числа без знака
	const string_view number = ((!text.empty() && ((text.front() == '-') || (text.front() == '+'))) ? text.substr(1) : text);
	/**
	 * Перечень написаний бесконечности
	 */
	static const char * const INFINITIES[] = {".inf", ".Inf", ".INF", nullptr};
	/**
	 * Если запись является бесконечностью
	 */
	if(matches(number, INFINITIES)){
		// Запоминаем разобранную бесконечность
		result.real = (negative ? -numeric_limits <double>::infinity() : numeric_limits <double>::infinity());
		// Выводим вид дробного числа двойной точности
		return type_t::DOUBLE;
	}
	/**
	 * Перечень написаний нечисловой величины
	 */
	static const char * const NOT_NUMBERS[] = {".nan", ".NaN", ".NAN", nullptr};
	/**
	 * Если запись является нечисловой величиной
	 */
	if(matches(text, NOT_NUMBERS)){
		// Запоминаем разобранную нечисловую величину
		result.real = numeric_limits <double>::quiet_NaN();
		// Выводим вид дробного числа двойной точности
		return type_t::DOUBLE;
	}
	// Собираемая запись числа без знаков подчёркивания
	string plain;
	// Выполняем упреждающее выделение памяти под собираемую запись
	plain.reserve(number.size());
	/**
	 * Выполняем перебор всех знаков записи числа
	 */
	for(const char letter : number){
		/**
		 * Если знак является знаком подчёркивания
		 *
		 * @note Знак этот наречие 1.1 дозволяет между разрядами для удобочитаемости, и
		 *       числом он не является: снимается он прежде разбора
		 */
		if(letter != '_')
			// Выполняем добавление знака к собираемой записи
			plain.push_back(letter);
	}
	// Основание системы счисления записи числа
	uint8_t radix = 10;
	// Смещение начала разрядов записи числа
	size_t offset = 0;
	/**
	 * Если запись открывается указателем системы счисления
	 */
	if((plain.size() > 2) && (plain.at(0) == '0')){
		/**
		 * Определяем указатель системы счисления записи
		 */
		switch(plain.at(1)){
			// Если запись является шестнадцатеричной
			case 'x':
			case 'X': radix = 16; offset = 2; break;
			// Если запись является восьмеричной наречия 1.2
			case 'o': radix = 8; offset = 2; break;
			/**
			 * Если запись является двоичной
			 *
			 * @note Двоичную запись знает лишь наречие 1.1, и разрешение до разбора её
			 *       не допустит, коли действует иная схема
			 */
			case 'b':
			case 'B': radix = 2; offset = 2; break;
		}
	}
	/**
	 * Если запись является восьмеричной наречия 1.1
	 *
	 * @note Ведущий нуль знаменует здесь восьмеричную запись, и `0777` есть 511. Наречие
	 *       1.2 такую запись числом не признаёт вовсе, и разрешение сюда не пропустит
	 */
	if((radix == 10) && (schema == schema_t::LEGACY) && (plain.size() > 1) && (plain.at(0) == '0') &&
	   (plain.find_first_not_of("01234567", 1) == string::npos)){
		// Запоминаем восьмеричное основание системы счисления
		radix = 8;
		// Запоминаем смещение начала разрядов записи
		offset = 1;
	}
	/**
	 * Если запись является шестидесятиричной
	 *
	 * @details Запись эту знает лишь наречие 1.1: `12:30` есть 750, а `1:00:00` есть
	 *          3600. Собирается число по разрядам, ибо разбор языка её не знает вовсе
	 */
	if((radix == 10) && (schema == schema_t::LEGACY) && (plain.find(':') != string::npos)){
		// Собираемое число шестидесятиричной записи
		double collected = 0.;
		// Признак того, что запись несёт дробную часть
		bool fractional = false;
		// Смещение начала очередного разряда записи
		size_t position = 0;
		/**
		 * Выполняем перебор всех разрядов шестидесятиричной записи
		 */
		while(position <= plain.size()){
			// Разыскиваем разделитель очередного разряда записи
			const size_t separator = plain.find(':', position);
			// Получаем очередной разряд шестидесятиричной записи
			const string part(plain, position, (((separator == string::npos) ? plain.size() : separator) - position));
			/**
			 * Если разряд несёт дробную часть
			 */
			if(part.find('.') != string::npos)
				// Запоминаем признак дробной части записи
				fractional = true;
			// Выполняем накопление числа очередным разрядом
			collected = ((collected * 60.) + ::strtod(part.c_str(), nullptr));
			/**
			 * Если разделителей в записи больше нет
			 */
			if(separator == string::npos)
				// Выходим из перебора разрядов записи
				break;
			// Выполняем переход к следующему разряду записи
			position = (separator + 1);
		}
		// Запоминаем разобранное число дробным видом
		result.real = (negative ? -collected : collected);
		/**
		 * Если запись дробной части не несёт
		 */
		if(!fractional){
			/**
			 * Если собранное число целым видом уже не представимо
			 *
			 * @details Разряды шестидесятиричной записи накапливаются дробным видом, и число
			 *          их описанием не ограничено вовсе: запись из десятка разрядов выходит за
			 *          предел целого. Приведение такого числа к целому есть неопределённое
			 *          поведение, и оттого оно выдаётся дробным видом. Нашёл это ворошитель
			 *          под UBSan
			 */
			if(collected >= static_cast <double> (numeric_limits <uint64_t>::max()))
				// Выводим вид дробного числа двойной точности
				return type_t::DOUBLE;
			// Выводим самый узкий вид, собранное число вмещающий
			return fitted(negative, static_cast <uint64_t> (collected), result);
		}
		// Выводим вид дробного числа двойной точности
		return type_t::DOUBLE;
	}
	/**
	 * Если запись является дробной
	 */
	if((radix == 10) && (plain.find_first_of(".eE") != string::npos)){
		// Выполняем сброс признака ошибки разбора записи
		errno = 0;
		// Запоминаем разобранное дробное число
		result.real = ::strtod(plain.c_str(), nullptr);
		/**
		 * Если запись несёт знак минус
		 */
		if(negative)
			// Выполняем смену знака разобранного числа
			result.real = -result.real;
		/**
		 * Если число за предел дробного вида вышло
		 *
		 * @details Запись `1e400` двойной точностью не представима, и разбор отдавал её
		 *          бесконечностью: число терялось, а от записи `.inf` отличить его было
		 *          уже нечем. Отдаём такую запись видом, ни в один родной вид не
		 *          вместимым, - тем она хранится записью своей, как и целое, за предел
		 *          вышедшее. Правило это перенято у кодека JSON дословно
		 *
		 * @note Сличение это ловит и потерю значимости: запись `1e-400` даёт нуль тем же
		 *       признаком, и нулём она тоже не является. Числа же поднормальные - вроде
		 *       `1e-320` - представимы, пусть и точностью урезанной: признак выхода за
		 *       предел стоит и у них, и оттого сличается сам исход, а не один признак
		 */
		if((errno == ERANGE) && (::isinf(result.real) || (result.real == 0.)))
			// Выводим вид числа, ни в один родной вид не вместимого
			return type_t::EXTENDED;
		// Выводим вид дробного числа двойной точности
		return type_t::DOUBLE;
	}
	// Выполняем сброс признака ошибки разбора записи
	errno = 0;
	// Выполняем разбор записи целого числа без знака
	const uint64_t collected = ::strtoull((plain.c_str() + offset), nullptr, radix);
	/**
	 * Если число за предел целого вида вышло
	 *
	 * @note Число, ни в один родной вид не вместившееся, хранится записью своей и
	 *       выдаётся дробным приближением: отбросить его значило бы потерять содержимое,
	 *       записанное текстом верно
	 */
	if(errno == ERANGE){
		// Запоминаем разобранное число дробным приближением его
		result.real = ::strtod(plain.c_str() + offset, nullptr);
		/**
		 * Если запись несёт знак минус
		 */
		if(negative)
			// Выполняем смену знака разобранного числа
			result.real = -result.real;
		// Выводим вид числа, ни в один родной вид не вместимого
		return type_t::EXTENDED;
	}
	// Выводим самый узкий вид, разобранное число вмещающий
	return fitted(negative, collected, result);
}
