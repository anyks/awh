/**
 * @file: common.cpp
 * @date: 2026-08-01
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация общих определений контейнера XML — описания кодов ошибок разбора,
 *        названия кодировок исходного текста и их определение, сличение имён с учётом пространств имён
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/ascii.hpp>
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
	 * Определяем код ошибки разбора
	 */
	switch(static_cast <uint8_t> (error)){
		// Если ошибок не обнаружено
		case static_cast <uint8_t> (error_t::NONE):
			// Выводим описание кода ошибки
			return "no error";
		// Если произошла внутренняя ошибка разбора
		case static_cast <uint8_t> (error_t::INTERNAL):
			// Выводим описание кода ошибки
			return "internal parser error";
		// Если текст оборвался посреди разметки
		case static_cast <uint8_t> (error_t::UNEXPECTED_EOF):
			// Выводим описание кода ошибки
			return "unexpected end of input";
		// Если знак недопустим в разметке
		case static_cast <uint8_t> (error_t::INVALID_CHARACTER):
			// Выводим описание кода ошибки
			return "character is not allowed in XML";
		// Если последовательность байтов не отвечает объявленной кодировке
		case static_cast <uint8_t> (error_t::INVALID_ENCODING):
			// Выводим описание кода ошибки
			return "malformed byte sequence for the declared encoding";
		// Если объявленная кодировка не поддерживается
		case static_cast <uint8_t> (error_t::UNSUPPORTED_ENCODING):
			// Выводим описание кода ошибки
			return "unsupported character encoding";
		// Если объявление разметки построено ошибочно
		case static_cast <uint8_t> (error_t::INVALID_DECLARATION):
			// Выводим описание кода ошибки
			return "malformed XML declaration";
		// Если объявленное издание разметки не поддерживается
		case static_cast <uint8_t> (error_t::UNSUPPORTED_VERSION):
			// Выводим описание кода ошибки
			return "unsupported XML version";
		// Если имя содержит недопустимые знаки либо пусто
		case static_cast <uint8_t> (error_t::INVALID_NAME):
			// Выводим описание кода ошибки
			return "invalid name";
		// Если длина имени превышает допустимую
		case static_cast <uint8_t> (error_t::NAME_TOO_LONG):
			// Выводим описание кода ошибки
			return "name is too long";
		// Если метка построена ошибочно
		case static_cast <uint8_t> (error_t::INVALID_TAG):
			// Выводим описание кода ошибки
			return "malformed tag";
		// Если метка не закрыта до конца текста
		case static_cast <uint8_t> (error_t::UNCLOSED_TAG):
			// Выводим описание кода ошибки
			return "unclosed tag";
		// Если имя закрывающей метки не совпадает с открывающей
		case static_cast <uint8_t> (error_t::MISMATCHED_TAG):
			// Выводим описание кода ошибки
			return "end tag does not match start tag";
		// Если обнаружена закрывающая метка без соответствующей открывающей
		case static_cast <uint8_t> (error_t::UNEXPECTED_CLOSE_TAG):
			// Выводим описание кода ошибки
			return "end tag without matching start tag";
		// Если в тексте более одного корневого узла
		case static_cast <uint8_t> (error_t::MULTIPLE_ROOTS):
			// Выводим описание кода ошибки
			return "more than one root element";
		// Если в тексте отсутствует корневой узел
		case static_cast <uint8_t> (error_t::MISSING_ROOT):
			// Выводим описание кода ошибки
			return "document has no root element";
		// Если текстовое содержимое обнаружено вне корневого узла
		case static_cast <uint8_t> (error_t::CONTENT_OUTSIDE_ROOT):
			// Выводим описание кода ошибки
			return "character data outside of root element";
		// Если превышена допустимая глубина вложенности узлов
		case static_cast <uint8_t> (error_t::DEPTH_EXCEEDED):
			// Выводим описание кода ошибки
			return "maximum element nesting depth exceeded";
		// Если атрибут построен ошибочно
		case static_cast <uint8_t> (error_t::INVALID_ATTRIBUTE):
			// Выводим описание кода ошибки
			return "malformed attribute";
		// Если атрибут с таким именем в узле уже объявлен
		case static_cast <uint8_t> (error_t::DUPLICATE_ATTRIBUTE):
			// Выводим описание кода ошибки
			return "duplicate attribute name";
		// Если значение атрибута не заключено в кавычки
		case static_cast <uint8_t> (error_t::UNQUOTED_ATTRIBUTE):
			// Выводим описание кода ошибки
			return "attribute value is not quoted";
		// Если ссылка на сущность построена ошибочно
		case static_cast <uint8_t> (error_t::INVALID_REFERENCE):
			// Выводим описание кода ошибки
			return "malformed entity reference";
		// Если обнаружена ссылка на необъявленную сущность
		case static_cast <uint8_t> (error_t::UNKNOWN_ENTITY):
			// Выводим описание кода ошибки
			return "reference to undeclared entity";
		// Если обнаружена ссылка на внешнюю сущность
		case static_cast <uint8_t> (error_t::EXTERNAL_ENTITY):
			// Выводим описание кода ошибки
			return "external entity references are not allowed";
		// Если сущность ссылается сама на себя
		case static_cast <uint8_t> (error_t::RECURSIVE_ENTITY):
			// Выводим описание кода ошибки
			return "recursive entity reference";
		// Если превышена допустимая глубина вложенности сущностей
		case static_cast <uint8_t> (error_t::ENTITY_DEPTH_EXCEEDED):
			// Выводим описание кода ошибки
			return "maximum entity nesting depth exceeded";
		// Если превышен допустимый объём подстановки сущностей
		case static_cast <uint8_t> (error_t::ENTITY_LIMIT_EXCEEDED):
			// Выводим описание кода ошибки
			return "maximum entity expansion size exceeded";
		// Если превышено допустимое количество объявленных сущностей
		case static_cast <uint8_t> (error_t::ENTITY_COUNT_EXCEEDED):
			// Выводим описание кода ошибки
			return "too many entity declarations";
		// Если числовая ссылка указывает на недопустимое кодовое значение
		case static_cast <uint8_t> (error_t::INVALID_CHAR_REFERENCE):
			// Выводим описание кода ошибки
			return "character reference points to an invalid code point";
		// Если примечание построено ошибочно
		case static_cast <uint8_t> (error_t::INVALID_COMMENT):
			// Выводим описание кода ошибки
			return "malformed comment";
		// Если раздел дословного текста построен ошибочно
		case static_cast <uint8_t> (error_t::INVALID_CDATA):
			// Выводим описание кода ошибки
			return "malformed CDATA section";
		// Если указание обработчику построено ошибочно
		case static_cast <uint8_t> (error_t::INVALID_PROCESSING):
			// Выводим описание кода ошибки
			return "malformed processing instruction";
		// Если имя указания обработчику отведено договором
		case static_cast <uint8_t> (error_t::RESERVED_PROCESSING):
			// Выводим описание кода ошибки
			return "processing instruction target is reserved";
		// Если описание типа документа построено ошибочно
		case static_cast <uint8_t> (error_t::INVALID_DOCTYPE):
			// Выводим описание кода ошибки
			return "malformed document type declaration";
		// Если описание типа документа расположено не на своём месте
		case static_cast <uint8_t> (error_t::DOCTYPE_MISPLACED):
			// Выводим описание кода ошибки
			return "document type declaration is misplaced";
		// Если префикс пространства имён построен ошибочно
		case static_cast <uint8_t> (error_t::INVALID_PREFIX):
			// Выводим описание кода ошибки
			return "malformed namespace prefix";
		// Если префикс не связан ни с одним пространством имён
		case static_cast <uint8_t> (error_t::UNBOUND_PREFIX):
			// Выводим описание кода ошибки
			return "namespace prefix is not bound";
		// Если выполнена попытка переопределить отведённый договором префикс
		case static_cast <uint8_t> (error_t::RESERVED_PREFIX):
			// Выводим описание кода ошибки
			return "reserved namespace prefix cannot be redefined";
		// Если объявлению пространства имён дано недопустимое значение
		case static_cast <uint8_t> (error_t::INVALID_NAMESPACE):
			// Выводим описание кода ошибки
			return "invalid namespace declaration";
		// Если превышен предел, заданный настройками разбора
		case static_cast <uint8_t> (error_t::OVERFLOW_LIMIT):
			// Выводим описание кода ошибки
			return "configured parser limit exceeded";
		// Если узел разметки пересекает границу подставленной сущности
		case static_cast <uint8_t> (error_t::ENTITY_BOUNDARY):
			// Выводим описание кода ошибки
			return "element boundary crosses entity boundary";
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
	switch(static_cast <uint8_t> (encoding)){
		// Если кодировкой является UTF-8
		case static_cast <uint8_t> (encoding_t::UTF8):
			// Выводим название кодировки
			return "UTF-8";
		// Если кодировкой является UTF-16 с обратным порядком байтов
		case static_cast <uint8_t> (encoding_t::UTF16LE):
			// Выводим название кодировки
			return "UTF-16LE";
		// Если кодировкой является UTF-16 с прямым порядком байтов
		case static_cast <uint8_t> (encoding_t::UTF16BE):
			// Выводим название кодировки
			return "UTF-16BE";
		// Если кодировкой является ISO-8859-1
		case static_cast <uint8_t> (encoding_t::LATIN1):
			// Выводим название кодировки
			return "ISO-8859-1";
		// Если кодировкой является US-ASCII
		case static_cast <uint8_t> (encoding_t::ASCII):
			// Выводим название кодировки
			return "US-ASCII";
	}
	// Выводим название неопределённой кодировки
	return "";
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
