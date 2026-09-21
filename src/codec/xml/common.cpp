/**
 * @file common.cpp
 * @date 2026-08-01
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
 * @brief Реализация общих определений контейнера XML — описания кодов ошибок разбора,
 *        названия кодировок исходного текста и их определение, сличение имён с учётом пространств имён
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <limits>

#include <encoding/ascii.hpp>
#include <num/lexical/lexical.hpp>
#include <codec/numeric.hpp>
#include <codec/xml/common.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Внутренние служебные объекты
 *
 */
namespace {
	/**
	 * Пространство имён библиотеки
	 */
	using namespace awh;

	/**
	 * @brief Посредник приведения вида ЯЗЫКА к тождественному разрядному виду
	 *
	 * @details Общее место кодеков порождено разрядными обозначениями - `int8_t`…`uint64_t`, -
	 * и телами располагает только по ним. Порождение же здесь ведётся видами ЯЗЫКА, ибо
	 * лишь их перечень одинаков на всякой системе. Посредник этот и сводит одно с другим:
	 * всякому виду языка он отвечает разрядным видом ТОЙ ЖЕ ширины и ТОЙ ЖЕ знаковости, а
	 * такой вид есть то же самое представление, и перенос через него не теряет ни разряда
	 *
	 * @warning Обозначения `int8_t`…`uint64_t` суть ПСЕВДОНИМЫ, и какому виду языка они
	 *          отвечают, решает система. У macOS ARM64 `int64_t` есть `long long`, у Linux
	 *          x86-64 - `long`: перечень разрядный даёт на двух системах РАЗНЫЕ наборы
	 *          видов, и потребитель, писавший `long long`, у Linux не связывался вовсе.
	 *          Замерено 07.09.2026 составом библиотеки на обеих системах: сборка молчит,
	 *          дефект виден только `nm` с `c++filt`
	 *
	 * @note Тела в общем месте для этого не требуется: посредник не порождает нового
	 *       обращения, а ведёт вызов к уже порождённому. Общее место остаётся заботою
	 *       того, кто зовёт `awh::codec::numeric` НАПРЯМУЮ, минуя кодек
	 */
	template <typename T, typename = void>
	struct fixed_t {
		// Виду неразрядному отвечает он сам
		typedef T type;
	};
	/**
	 * @brief Посредник приведения для видов целочисленных
	 *
	 * @note Признак `bool` исключён намеренно: ширина его системою не задана, и разрядного
	 *       вида, ему тождественного, не существует вовсе
	 */
	template <typename T>
	struct fixed_t <T, typename std::enable_if <std::is_integral <T>::value && !std::is_same <T, bool>::value>::type> {
		// Разрядный вид той же ширины и той же знаковости
		typedef typename std::conditional <std::is_signed <T>::value,
			typename std::conditional <sizeof(T) == 1, int8_t,
				typename std::conditional <sizeof(T) == 2, int16_t,
					typename std::conditional <sizeof(T) == 4, int32_t, int64_t>::type>::type>::type,
			typename std::conditional <sizeof(T) == 1, uint8_t,
				typename std::conditional <sizeof(T) == 2, uint16_t,
					typename std::conditional <sizeof(T) == 4, uint32_t, uint64_t>::type>::type>::type
		>::type type;
	};

	/**
	 * @brief Метод сличения последовательностей знаков без учёта регистра
	 *
	 * @details Сличение ведётся по правилам US-ASCII: прочие знаки сличаются как есть.
	 * Названия кодировок договором ограничены знаками US-ASCII, и привлекать сюда
	 * правила местности незачем
	 *
	 * @param first  первая последовательность знаков для сличения
	 * @param second вторая последовательность знаков для сличения
	 * @return       результат сличения
	 *
	 */
	static bool compare(const string_view first, const string_view second) noexcept {
		/**
		 * Если длины последовательностей знаков не совпадают
		 */
		if(first.length() != second.length())
			// Выводим отрицательный результат сличения
			return false;
		/**
		 * Выполняем перебор всех знаков последовательности
		 */
		for(size_t i = 0; i < first.length(); i++){
			/**
			 * Если очередные знаки не совпадают
			 */
			if(!ascii::equals(first[i], second[i]))
				// Выводим отрицательный результат сличения
				return false;
		}
		// Выводим положительный результат сличения
		return true;
	}
	/**
	 * @brief Метод отбрасывания пробельной обвязки содержимого разметки
	 *
	 * @details Пробельными считаются знаки, признанные таковыми договором XML: пробел,
	 * знак табуляции, возврат каретки и перевод строки. Обвязка в содержимом появляется
	 * сама собой при записи с отступами, и отбрасывать её приходится всегда
	 *
	 * @param text обрабатываемое содержимое
	 * @return     содержимое без пробельной обвязки по краям
	 *
	 */
	string_view trim(const string_view text) noexcept {
		// Начало значащей части содержимого
		size_t begin = 0;
		// Конец значащей части содержимого
		size_t end = text.length();
		/**
		 * Выполняем отбрасывание пробельных знаков в начале содержимого
		 */
		while((begin < end) && ((text[begin] == ' ') || (text[begin] == '\t') || (text[begin] == '\r') || (text[begin] == '\n')))
			// Выполняем переход к следующему знаку содержимого
			begin++;
		/**
		 * Выполняем отбрасывание пробельных знаков в конце содержимого
		 */
		while((end > begin) && ((text[end - 1] == ' ') || (text[end - 1] == '\t') || (text[end - 1] == '\r') || (text[end - 1] == '\n')))
			// Выполняем переход к предыдущему знаку содержимого
			end--;
		// Выводим содержимое без пробельной обвязки
		return text.substr(begin, end - begin);
	}
};

/**
 * @brief Метод проверки совпадения имени
 *
 * @param uri   обозначение пространства имён для сличения
 * @param local местное имя для сличения
 * @return      результат проверки
 *
 */
bool awh::codec::xml::Name::is(const string_view uri, const string_view local) const noexcept {
	// Выполняем сличение обозначения пространства имён и местного имени
	return ((this->uri.compare(uri) == 0) && (this->local.compare(local) == 0));
}
/**
 * @brief Оператор сравнения
 *
 * @param name имя для сравнения
 * @return     результат сравнения
 *
 */
bool awh::codec::xml::Name::operator == (const Name & name) const noexcept {
	// Выполняем сличение по обозначению пространства имён и местному имени
	return this->is(name.uri, name.local);
}
/**
 * @brief Оператор сравнения
 *
 * @param name имя для сравнения
 * @return     результат сравнения
 *
 */
bool awh::codec::xml::Name::operator != (const Name & name) const noexcept {
	// Выводим обратный результат сличения имён
	return !this->is(name.uri, name.local);
}
/**
 * @brief Метод получения описания кода ошибки разбора
 *
 * @param error код ошибки разбора
 * @return      описание кода ошибки на английском языке
 *
 */
const char * awh::codec::xml::message(const error_t error) noexcept {
	/**
	 * Определяем код отказа разбора
	 *
	 * @note Сличение ведётся по САМОМУ перечню, а не по приведённому к байту значению,
	 *       и ветви `default` здесь нет нарочно: так собиратель сам сообщает о коде,
	 *       описания не получившем, - предупреждением `-Wswitch` с именем этого кода.
	 *       Приведение к байту, стоявшее здесь прежде, защиту эту снимало вовсе, и коды,
	 *       дописанные в перечень, оставались без описания молча. Проверено щупом:
	 *       дописанный код вызывает предупреждение по имени
	 *
	 * @warning Возвращать приведение нельзя: сторожем тут выступает собиратель, а не
	 *          проверка. Проверка описаний перечень перебрать не может - о том, что код
	 *          объявлен, ей узнать неоткуда
	 */
	switch(error){
		// Если ошибок не обнаружено
		case error_t::NONE:
			// Выводим описание кода ошибки
			return "no error";
		// Если произошла внутренняя ошибка разбора
		case error_t::INTERNAL:
			// Выводим описание кода ошибки
			return "internal parser error";
		// Если знак недопустим в разметке
		case error_t::INVALID_CHARACTER:
			// Выводим описание кода ошибки
			return "character is not allowed in XML";
		// Если последовательность байтов не отвечает объявленной кодировке
		case error_t::INVALID_ENCODING:
			// Выводим описание кода ошибки
			return "invalid byte sequence for the declared encoding";
		// Если объявленная кодировка не поддерживается
		case error_t::UNSUPPORTED_ENCODING:
			// Выводим описание кода ошибки
			return "unsupported encoding";
		// Если объявление разметки построено ошибочно
		case error_t::INVALID_DECLARATION:
			// Выводим описание кода ошибки
			return "malformed XML declaration";
		// Если объявленное издание разметки не поддерживается
		case error_t::UNSUPPORTED_VERSION:
			// Выводим описание кода ошибки
			return "unsupported XML version";
		// Если имя содержит недопустимые знаки либо пусто
		case error_t::INVALID_NAME:
			// Выводим описание кода ошибки
			return "invalid name";
		// Если длина имени превышает допустимую
		case error_t::NAME_TOO_LONG:
			// Выводим описание кода ошибки
			return "name is too long";
		// Если метка построена ошибочно
		case error_t::INVALID_TAG:
			// Выводим описание кода ошибки
			return "malformed tag";
		// Если метка не закрыта до конца текста
		case error_t::UNCLOSED_TAG:
			// Выводим описание кода ошибки
			return "unclosed tag";
		// Если имя закрывающей метки не совпадает с открывающей
		case error_t::MISMATCHED_TAG:
			// Выводим описание кода ошибки
			return "end tag does not match start tag";
		// Если обнаружена закрывающая метка без соответствующей открывающей
		case error_t::UNEXPECTED_CLOSE_TAG:
			// Выводим описание кода ошибки
			return "end tag without matching start tag";
		// Если в тексте более одного корневого узла
		case error_t::MULTIPLE_ROOTS:
			// Выводим описание кода ошибки
			return "more than one root element";
		// Если в тексте отсутствует корневой узел
		case error_t::MISSING_ROOT:
			// Выводим описание кода ошибки
			return "document has no root element";
		// Если текстовое содержимое обнаружено вне корневого узла
		case error_t::CONTENT_OUTSIDE_ROOT:
			// Выводим описание кода ошибки
			return "character data outside of root element";
		// Если превышена допустимая глубина вложенности узлов
		case error_t::DEPTH_EXCEEDED:
			// Выводим описание кода ошибки
			return "maximum element nesting depth exceeded";
		// Если атрибут построен ошибочно
		case error_t::INVALID_ATTRIBUTE:
			// Выводим описание кода ошибки
			return "malformed attribute";
		// Если атрибут с таким именем в узле уже объявлен
		case error_t::DUPLICATE_ATTRIBUTE:
			// Выводим описание кода ошибки
			return "duplicate attribute name";
		// Если значение атрибута не заключено в кавычки
		case error_t::UNQUOTED_ATTRIBUTE:
			// Выводим описание кода ошибки
			return "attribute value is not quoted";
		// Если ссылка на сущность построена ошибочно
		case error_t::INVALID_REFERENCE:
			// Выводим описание кода ошибки
			return "malformed entity reference";
		// Если обнаружена ссылка на необъявленную сущность
		case error_t::UNKNOWN_ENTITY:
			// Выводим описание кода ошибки
			return "reference to undeclared entity";
		// Если встречена ссылка на внешнюю либо неразбираемую сущность там, где она запрещена
		case error_t::EXTERNAL_ENTITY:
			// Выводим описание кода ошибки
			return "reference to an external or unparsed entity is not allowed here";
		// Если сущность ссылается сама на себя
		case error_t::RECURSIVE_ENTITY:
			// Выводим описание кода ошибки
			return "recursive entity reference";
		// Если превышена допустимая глубина вложенности сущностей
		case error_t::ENTITY_DEPTH_EXCEEDED:
			// Выводим описание кода ошибки
			return "maximum entity nesting depth exceeded";
		// Если превышен допустимый объём подстановки сущностей
		case error_t::ENTITY_LIMIT_EXCEEDED:
			// Выводим описание кода ошибки
			return "maximum entity expansion size exceeded";
		// Если превышено допустимое количество объявленных сущностей
		case error_t::ENTITY_COUNT_EXCEEDED:
			// Выводим описание кода ошибки
			return "too many entity declarations";
		// Если числовая ссылка построена ошибочно либо указывает на недопустимое кодовое значение
		case error_t::INVALID_CHAR_REFERENCE:
			// Выводим описание кода ошибки
			return "malformed or out-of-range character reference";
		// Если примечание построено ошибочно
		case error_t::INVALID_COMMENT:
			// Выводим описание кода ошибки
			return "malformed comment";
		// Если раздел дословного текста построен ошибочно
		case error_t::INVALID_CDATA:
			// Выводим описание кода ошибки
			return "malformed CDATA section";
		// Если указание обработчику построено ошибочно
		case error_t::INVALID_PROCESSING:
			// Выводим описание кода ошибки
			return "malformed processing instruction";
		// Если имя указания обработчику отведено договором
		case error_t::RESERVED_PROCESSING:
			// Выводим описание кода ошибки
			return "processing instruction target is reserved";
		// Если описание типа документа построено ошибочно
		case error_t::INVALID_DOCTYPE:
			// Выводим описание кода ошибки
			return "malformed document type declaration";
		// Если описание типа документа расположено не на своём месте
		case error_t::DOCTYPE_MISPLACED:
			// Выводим описание кода ошибки
			return "document type declaration is misplaced";
		// Если префикс пространства имён построен ошибочно
		case error_t::INVALID_PREFIX:
			// Выводим описание кода ошибки
			return "malformed namespace prefix";
		// Если префикс не связан ни с одним пространством имён
		case error_t::UNBOUND_PREFIX:
			// Выводим описание кода ошибки
			return "namespace prefix is not bound";
		// Если выполнена попытка переопределить отведённый договором префикс
		case error_t::RESERVED_PREFIX:
			// Выводим описание кода ошибки
			return "reserved namespace prefix cannot be redefined";
		// Если объявлению пространства имён дано недопустимое значение
		case error_t::INVALID_NAMESPACE:
			// Выводим описание кода ошибки
			return "invalid namespace declaration";
		// Если превышен предел, заданный настройками разбора
		case error_t::OVERFLOW_LIMIT:
			// Выводим описание кода ошибки
			return "configured parser limit exceeded";
		// Если разбираемый текст не помещается в разрядность хранилища
		case error_t::STORAGE_EXHAUSTED:
			// Выводим описание кода ошибки
			return "the text does not fit the width of the parser storage";
		// Если количество атрибутов у узла превышает допустимое
		case error_t::TOO_MANY_ATTRIBUTES:
			// Выводим описание кода ошибки
			return "too many attributes in the element";
		// Если построение разметки пересекает границу подставленной сущности
		case error_t::ENTITY_BOUNDARY:
			// Выводим описание кода ошибки
			return "logical structure crosses entity boundary";
		// Если файл разметки открыть не удалось
		case error_t::FILE_NOT_OPENED:
			// Выводим описание кода ошибки
			return "cannot open the markup file";
		// Если подача продолжена после объявленного конца текста
		case error_t::TEXT_ALREADY_ENDED:
			// Выводим описание кода ошибки
			return "feeding continued after the text was declared complete";
		// Если текст разметки записать в файл не удалось
		case error_t::FILE_NOT_WRITTEN:
			// Выводим описание кода ошибки
			return "cannot write the markup file";
		// Если файл разметки прочитать не удалось
		case error_t::FILE_NOT_READ:
			// Выводим описание кода ошибки
			return "cannot read the markup file";
		// Если переданный узел дерева непригоден
		case error_t::INVALID_NODE:
			// Выводим описание кода ошибки
			return "the passed tree node is unfit";
		// Если кодировка сменена посреди подачи текста
		case error_t::ENCODING_ALREADY_CHOSEN:
			// Выводим описание кода ошибки
			return "the encoding cannot be changed in the middle of the feed";
	}
	// Выводим описание неизвестного кода ошибки
	return "unknown error";
}
/**
 * @brief Метод получения названия кодировки
 *
 * @param encoding кодировка исходного текста
 * @return         общепринятое название кодировки
 *
 */
const char * awh::codec::xml::name(const encoding_t encoding) noexcept {
	/**
	 * Определяем кодировку исходного текста
	 */
	switch(encoding){
		// Если кодировкой является UTF-8
		case encoding_t::UTF8:
			// Выводим название кодировки
			return "UTF-8";
		// Если кодировкой является UTF-16 с обратным порядком байтов
		case encoding_t::UTF16LE:
			// Выводим название кодировки
			return "UTF-16LE";
		// Если кодировкой является UTF-16 с прямым порядком байтов
		case encoding_t::UTF16BE:
			// Выводим название кодировки
			return "UTF-16BE";
		// Если кодировкой является ISO-8859-1
		case encoding_t::LATIN1:
			// Выводим название кодировки
			return "ISO-8859-1";
		// Если кодировкой является US-ASCII
		case encoding_t::ASCII:
			// Выводим название кодировки
			return "US-ASCII";
		/**
		 * Если кодировка взята таблицею у общего модуля кодировок
		 *
		 * @note Имени самой кодировки вид этот не несёт: какая именно взята, хранится
		 *       таблицею у разбора, а сюда приходит один лишь вид
		 */
		case encoding_t::SINGLE:
			// Выводим обозначение вида кодировки
			return "single-byte";
		/**
		 * Если разбор дошёл до видов, обработки здесь не требующих
		 *
		 * @warning Перечислены они НАМЕРЕННО вместо `default`: приведение к `default`
		 *          глушит `-Wswitch`, и новый член перечня пройдёт молча
		 */
		case encoding_t::NONE:
			/**
			 * Выводим название неопределённой кодировки
			 *
			 * @note Прежде здесь выдавалась строка ПУСТАЯ - и ею же отвечала кодировка,
			 *       перечню не принадлежащая: два разных случая были неотличимы, а
			 *       звучащий, вписывающий название в журнал, получал дыру в сообщении.
			 *       Кодеки документа и таблицы зовут эти случаи «none» и «unknown»
			 */
			return "none";
	}
	// Выводим название неизвестной кодировки
	return "unknown";
}
/**
 * @brief Метод определения кодировки по её названию
 *
 * @param text название кодировки в любом регистре
 * @return     определённая кодировка исходного текста
 *
 */
awh::codec::xml::encoding_t awh::codec::xml::encoding(const string_view text) noexcept {
	/**
	 * Если название кодировки не передано
	 */
	if(text.empty())
		// Выводим неопределённую кодировку
		return encoding_t::NONE;
	/**
	 * Если кодировкой является UTF-8
	 */
	if(::compare(text, "UTF-8") || ::compare(text, "UTF8"))
		// Выводим определённую кодировку
		return encoding_t::UTF8;
	/**
	 * Если кодировкой является UTF-16 с обратным порядком байтов
	 */
	if(::compare(text, "UTF-16LE") || ::compare(text, "UTF16LE"))
		// Выводим определённую кодировку
		return encoding_t::UTF16LE;
	/**
	 * Если кодировкой является UTF-16 с прямым порядком байтов
	 */
	if(::compare(text, "UTF-16BE") || ::compare(text, "UTF16BE"))
		// Выводим определённую кодировку
		return encoding_t::UTF16BE;
	/**
	 * Если кодировка объявлена как UTF-16 без указания порядка байтов
	 *
	 * @note Порядок байтов в таком случае определяется меткой в начале текста,
	 *       поэтому объявление разбирается в кодировку с прямым порядком лишь
	 *       как основание по умолчанию договора
	 */
	if(::compare(text, "UTF-16") || ::compare(text, "UTF16"))
		// Выводим определённую кодировку
		return encoding_t::UTF16BE;
	/**
	 * Если кодировкой является ISO-8859-1
	 */
	if(::compare(text, "ISO-8859-1") || ::compare(text, "ISO8859-1") || ::compare(text, "LATIN1") || ::compare(text, "L1"))
		// Выводим определённую кодировку
		return encoding_t::LATIN1;
	/**
	 * Если кодировкой является US-ASCII
	 */
	if(::compare(text, "US-ASCII") || ::compare(text, "ASCII") || ::compare(text, "ANSI_X3.4-1968"))
		// Выводим определённую кодировку
		return encoding_t::ASCII;
	// Выводим неопределённую кодировку
	return encoding_t::NONE;
}
/**
 * @brief Метод разбора целого числа со знаком из содержимого разметки
 *
 * @param text   разбираемое содержимое
 * @param result ссылка на результат разбора
 * @return       признак успешного разбора
 *
 */
bool awh::codec::xml::integer(const string_view text, int64_t & result) noexcept {
	// Получаем содержимое без пробельной обвязки
	const string_view value = ::trim(text);
	/**
	 * Если разбирать нечего
	 */
	if(value.empty())
		// Выводим признак неудачного разбора
		return false;
	// Содержимое, разбору отдаваемое
	string_view digits = value;
	/**
	 * Если содержимое записано с ведущим знаком плюса
	 *
	 * @details Договор XSD (W3C XML Schema Part 2, §3.3.13) даёт `xs:integer` лексику
	 *          `[\-+]?[0-9]+`, где плюс дозволен наравне с минусом, а разбор `fromChars`
	 *          следует правилам языка и плюса не принимает вовсе. Оттого плюс снимается
	 *          здесь, и разбору достаётся остаток
	 *
	 * @note Довод общий с соседним входом `real`, приведённым к лексике XSD прежде: мера
	 *       строгости у входов одного кодека расходиться не должна. Замер 16.09.2026:
	 *       `+5` отвергалось целым, будучи принятым дробным
	 */
	if(digits.front() == '+')
		// Выполняем снятие ведущего знака плюса
		digits.remove_prefix(1);
	/**
	 * Если за снятым знаком не осталось ничего
	 */
	if(digits.empty())
		// Выводим признак неудачного разбора
		return false;
	// Разобранное значение, выходной переменной ещё не отданное
	int64_t number = 0;
	// Выполняем разбор целого числа со знаком
	const lexical_t::result_t <char> res = lexical_t::fromChars(digits.data(), digits.data() + digits.length(), number);
	/**
	 * Выводим признак успешного приведения, если число разобрано целиком
	 *
	 * @note Остаток за числом отвергается намеренно: «52abc» числом не является,
	 *       и приведение такого поля к 52 скрыло бы ошибку в содержимом
	 *
	 * @note Разбор ведётся в СВОЮ переменную, а выходная трогается лишь по успеху:
	 *       прежде разбор писал прямо в неё, и отвергнутое «1e3» оставляло у звучащего
	 *       единицу вместо нетронутого значения - ошибка обращения оборачивалась
	 *       правдоподобным числом
	 */
	if(static_cast <bool> (res) && (res.ptr == (digits.data() + digits.length()))){
		// Запоминаем разобранное значение
		result = number;
		// Выводим признак успешного приведения
		return true;
	}
	// Выводим признак неудачного приведения
	return false;
}
/**
 * @brief Метод разбора целого числа без знака из содержимого разметки
 *
 * @param text   разбираемое содержимое
 * @param result ссылка на результат разбора
 * @return       признак успешного разбора
 *
 */
bool awh::codec::xml::integer(const string_view text, uint64_t & result) noexcept {
	// Получаем содержимое без пробельной обвязки
	const string_view value = ::trim(text);
	/**
	 * Если разбирать нечего
	 */
	if(value.empty())
		// Выводим признак неудачного разбора
		return false;
	/**
	 * Если содержимое записано со знаком минуса
	 *
	 * @note Число отрицательное в тип без знака не приводится: запрошенный тип и есть
	 *       указание на ожидаемую запись. Договор XSD (§3.3.20) дозволяет `-0` и у
	 *       `xs:nonNegativeInteger`, но запись эта для беззнакового типа бессмысленна,
	 *       и отвергается она намеренно
	 */
	if(value.front() == '-')
		// Выводим признак неудачного разбора
		return false;
	// Содержимое, разбору отдаваемое
	string_view digits = value;
	/**
	 * Если содержимое записано с ведущим знаком плюса
	 *
	 * @note Плюс договором XSD дозволен, а `fromChars` его не принимает - оттого он
	 *       снимается здесь. Прежде плюс отвергался наравне с минусом, и `+5`
	 *       беззнаковым не читалось, будучи по договору законным
	 */
	if(digits.front() == '+')
		// Выполняем снятие ведущего знака плюса
		digits.remove_prefix(1);
	/**
	 * Если за снятым знаком не осталось ничего
	 */
	if(digits.empty())
		// Выводим признак неудачного разбора
		return false;
	// Разобранное значение, выходной переменной ещё не отданное
	uint64_t number = 0;
	// Выполняем разбор целого числа без знака
	const lexical_t::result_t <char> res = lexical_t::fromChars(digits.data(), digits.data() + digits.length(), number);
	// Выводим признак успешного разбора, если число разобрано целиком
	/**
	 * Выводим признак успешного приведения, если число разобрано целиком
	 *
	 * @note Остаток за числом отвергается намеренно: «52abc» числом не является,
	 *       и приведение такого поля к 52 скрыло бы ошибку в содержимом
	 *
	 * @note Разбор ведётся в СВОЮ переменную, а выходная трогается лишь по успеху:
	 *       прежде разбор писал прямо в неё, и отвергнутое «1e3» оставляло у звучащего
	 *       единицу вместо нетронутого значения - ошибка обращения оборачивалась
	 *       правдоподобным числом
	 */
	if(static_cast <bool> (res) && (res.ptr == (digits.data() + digits.length()))){
		// Запоминаем разобранное значение
		result = number;
		// Выводим признак успешного приведения
		return true;
	}
	// Выводим признак неудачного приведения
	return false;
}
/**
 * @brief Метод разбора числа с плавающей точкой из содержимого разметки
 *
 * @param text   разбираемое содержимое
 * @param result ссылка на результат разбора
 * @return       признак успешного разбора
 *
 */
bool awh::codec::xml::real(const string_view text, double & result) noexcept {
	// Получаем содержимое без пробельной обвязки
	const string_view value = ::trim(text);
	/**
	 * Если разбирать нечего
	 */
	if(value.empty())
		// Выводим признак неудачного разбора
		return false;
	/**
	 * Если содержимое является особым значением договора XSD
	 *
	 * @details Договор XSD (W3C XML Schema Part 2, §3.2.5) допускает ровно четыре
	 *          особых написания: `INF`, `+INF`, `-INF` и `NaN`, и РЕГИСТР В НИХ
	 *          ЗНАЧИМ - иных написаний лексика не содержит вовсе
	 *
	 * @warning Сличение ведётся ТОЧНОЕ и ДО общего разбора намеренно: разбор числа
	 *          `fromChars` следует правилам языка, а не XSD, и принимает `inf`, `nan`,
	 *          `infinity` и `INFINITY` в любом написании. Замер 16.09.2026: все они
	 *          проходили, и содержимое, договору XSD не отвечающее, ложилось в дерево
	 *          числом. Соседний вход `boolean` при этом держался строго - `True` и
	 *          `TRUE` отвергал, - и мера строгости у двух входов одного кодека
	 *          расходилась
	 */
	if((value.compare("INF") == 0) || (value.compare("+INF") == 0)){
		// Запоминаем положительную бесконечность
		result = ::std::numeric_limits <double>::infinity();
		// Выводим признак успешного приведения
		return true;
	/**
	 * Если содержимое является отрицательной бесконечностью
	 */
	} else if(value.compare("-INF") == 0) {
		// Запоминаем отрицательную бесконечность
		result = -::std::numeric_limits <double>::infinity();
		// Выводим признак успешного приведения
		return true;
	/**
	 * Если содержимое является нечислом
	 */
	} else if(value.compare("NaN") == 0) {
		// Запоминаем нечисло
		result = ::std::numeric_limits <double>::quiet_NaN();
		// Выводим признак успешного приведения
		return true;
	}
	/**
	 * Содержимое, знак числа несущее
	 *
	 * @note Договор XSD допускает ведущий `+` наравне с `-`, а разбор `fromChars`
	 *       плюса не принимает вовсе. Оттого плюс снимается здесь, и разбору
	 *       достаётся остаток. Замер 16.09.2026: `+1.5`, `+0.5e-3` и `+INF`
	 *       отвергались, будучи по договору XSD вполне законными
	 */
	string_view digits = value;
	/**
	 * Если содержимое начинается со знака числа
	 */
	if((digits.front() == '+') || (digits.front() == '-'))
		// Снимаем знак числа с начала содержимого
		digits.remove_prefix(1);
	/**
	 * Если за знаком числа не стоит ни цифры, ни точки
	 *
	 * @warning Застава эта и отсекает особые написания, договором XSD не признанные:
	 *          `inf`, `nan`, `infinity` и всякое иное, что `fromChars` принял бы по
	 *          правилам языка. Законные же особые значения разобраны ВЫШЕ точным
	 *          сличением и сюда не доходят
	 */
	if(digits.empty() || (!awh::ascii::isDigit(digits.front()) && (digits.front() != '.')))
		// Выводим признак неудачного разбора
		return false;
	// Содержимое, разбору подлежащее: без ведущего плюса, знак минуса сохраняя
	const string_view parsed = ((value.front() == '+') ? digits : value);
	// Разобранное значение, выходной переменной ещё не отданное
	double number = 0;
	// Выполняем разбор числа с плавающей точкой
	const lexical_t::result_t <char> res = lexical_t::fromChars(parsed.data(), parsed.data() + parsed.length(), number);
	// Выводим признак успешного разбора, если число разобрано целиком
	/**
	 * Выводим признак успешного приведения, если число разобрано целиком
	 *
	 * @note Остаток за числом отвергается намеренно: «52abc» числом не является,
	 *       и приведение такого поля к 52 скрыло бы ошибку в содержимом
	 *
	 * @note Разбор ведётся в СВОЮ переменную, а выходная трогается лишь по успеху:
	 *       прежде разбор писал прямо в неё, и отвергнутое «1e3» оставляло у звучащего
	 *       единицу вместо нетронутого значения - ошибка обращения оборачивалась
	 *       правдоподобным числом
	 */
	if(static_cast <bool> (res) && (res.ptr == (parsed.data() + parsed.length()))){
		// Запоминаем разобранное значение
		result = number;
		// Выводим признак успешного приведения
		return true;
	}
	// Выводим признак неудачного приведения
	return false;
}
/**
 * @brief Метод разбора логического значения из содержимого разметки
 *
 * @param text   разбираемое содержимое
 * @param result ссылка на результат разбора
 * @return       признак успешного разбора
 *
 */
bool awh::codec::xml::boolean(const string_view text, bool & result) noexcept {
	// Получаем содержимое без пробельной обвязки
	const string_view value = ::trim(text);
	/**
	 * Если содержимым является истина
	 *
	 * @note Регистр записи учитывается намеренно: договор XSD иных написаний
	 *       логического значения не допускает, а «True» из чужого ответа лучше
	 *       отвергнуть явно, чем принять за истину и разойтись с отправителем
	 */
	if((value.compare("true") == 0) || (value.compare("1") == 0)){
		// Запоминаем разобранное логическое значение
		result = true;
		// Выводим признак успешного разбора
		return true;
	}
	/**
	 * Если содержимым является ложь
	 */
	if((value.compare("false") == 0) || (value.compare("0") == 0)){
		// Запоминаем разобранное логическое значение
		result = false;
		// Выводим признак успешного разбора
		return true;
	}
	// Выводим признак неудачного разбора
	return false;
}
/**
 * @brief Шаблонный метод извлечения числа из содержимого разметки затребованным видом
 *
 * @details Разбор ведётся общим для всех кодеков рамки местом: договор извлечения един,
 * и держаться он обязан в одном написании. Прежде разбор был написан у разметки дважды -
 * у дерева документа и у самостоятельного значения, - и написания расходились по четырём
 * правилам разом: пробельная обвязка, записи `nan` и `inf`, показатель степени при
 * извлечении целым и запись, в разрядность не вмещающаяся. Сведены они решением
 * от 01.09.2026
 *
 * @tparam T      затребованный вид числа
 * @param  text   разбираемое содержимое
 * @param  result ссылка на результат разбора
 * @return        признак успешного разбора
 *
 */
template <typename T>
bool awh::codec::xml::numeric(const string_view text, T & result) noexcept {
	// Разрядный вид, тождественный виду языка приёмника
	typedef typename fixed_t <T>::type fixed;
	/**
	 * Если вид приёмника разрядному виду тождествен сам
	 */
	if constexpr(std::is_same <T, fixed>::value)
		// Выводим признак успешности извлечения числа общим для кодеков местом
		return awh::codec::numeric <T> (text, result);
	/**
	 * Если вид приёмника есть иной вид языка той же ширины
	 */
	else {
		// Приёмник разрядного вида для извлечения числа
		fixed value = fixed();
		// Выполняем извлечение числа общим для кодеков местом
		const bool ok = awh::codec::numeric <fixed> (text, value);
		/**
		 * Если извлечение числа удалось
		 *
		 * @note Приёмник ставится ЛИШЬ при успехе: общее место при отказе приёмника не
		 *       трогает, и перенос безусловный портил бы прежнее его содержимое
		 */
		if(ok)
			// Выполняем перенос извлечённого числа приёмнику вида языка
			result = static_cast <T> (value);
		// Выводим признак успешности извлечения числа
		return ok;
	}
}
/**
 * Выполняем явное создание тел разбора числа по всем поддерживаемым видам ЯЗЫКА
 *
 * @warning Перечень ведётся видами языка, а не разрядными обозначениями: последние суть
 *          псевдонимы, и на разных системах ложатся на разные виды - подробности при
 *          посреднике `fixed_t` в начале исходника
 *
 * @note Вид `char` не порождается НАМЕРЕННО, и довод тот же, что у записи: `char` есть
 *       вид знака, а не числа, и разбор текста в него потребителю ни к чему. Виды
 *       `signed char` и `unsigned char` порождаются - они берутся числами осознанно
 */
template __AWH_SHARED_EXPORT__ bool awh::codec::xml::numeric <bool> (const string_view, bool &) noexcept;
/**
 * Порождение видом `char` заведено 08.09.2026 по находке на стендах Sun
 *
 * @warning Вид `char` НЕ СОВПАДАЕТ ни с `signed char`, ни с `unsigned char` - это третий,
 *          самостоятельный вид языка. Обыкновенно `int8_t` есть псевдоним `signed char`, и
 *          порождение без `char` беды не давало; у Solaris же и OpenIndiana `int8_t` есть
 *          `char`, и всякий звучащий, подающий `int8_t`, не связывался вовсе
 *
 * @note Замерено на стенде: `is_same <int8_t, char>` истинно, `is_same <int8_t, signed char>`
 *       ложно. Отказ связывания: `Writer::value <char>` не найден
 *
 * @note Правило отсюда общее: порождать надлежит ВИДАМИ ЯЗЫКА и всеми четырнадцатью, а не
 *       разрядными псевдонимами и не тринадцатью - какому виду отвечает псевдоним, решает
 *       система, и решает по-разному
 *
 */
template __AWH_SHARED_EXPORT__ bool awh::codec::xml::numeric <char> (const string_view, char &) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::xml::numeric <signed char> (const string_view, signed char &) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::xml::numeric <unsigned char> (const string_view, unsigned char &) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::xml::numeric <short> (const string_view, short &) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::xml::numeric <unsigned short> (const string_view, unsigned short &) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::xml::numeric <int> (const string_view, int &) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::xml::numeric <unsigned int> (const string_view, unsigned int &) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::xml::numeric <long> (const string_view, long &) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::xml::numeric <unsigned long> (const string_view, unsigned long &) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::xml::numeric <long long> (const string_view, long long &) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::xml::numeric <unsigned long long> (const string_view, unsigned long long &) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::xml::numeric <float> (const string_view, float &) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::xml::numeric <double> (const string_view, double &) noexcept;
