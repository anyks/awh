/**
 * @file: writer.cpp
 * @date: 2026-08-09
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация записи текста настроек INI — проверка построения имён, ограждение
 *        значений кавычками и управляющими последовательностями, запись разделов,
 *        свойств и примечаний, наборы настроек сложившихся наречий записи
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>
#include <type_traits>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <encoding/ascii.hpp>
#include <codec/ini/writer.hpp>

/**
 * Снимаем на время реализации макросы, чьи имена заняты
 * членами перечислений AWH (возвращает их macro_pop.hpp в конце файла)
 */
#include <sys/macro_push.hpp>

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
	 * Пространство имён контейнера INI
	 */
	using namespace awh::codec::ini;

	/**
	 * @brief Метод проверки значения на нужду в ограждении кавычками
	 *
	 * @details Ограждения требует значение, которое разбор без кавычек прочитал бы
	 * иначе: несущее пробельную обвязку, знак примечания либо кавычку
	 *
	 * @param value          проверяемое значение свойства
	 * @param comments       знаки, которые читающий признаёт началом примечания
	 * @param inlineComments признак признания примечания в конце строки читающим
	 * @return               результат проверки
	 *
	 */
	static bool quotable(const string_view value, const marker_t comments, const bool inlineComments) noexcept {
		/**
		 * Если значение пусто
		 *
		 * @note Пустое значение ограждения не требует: запись «имя =» разбирается
		 *       обратно в пустое значение всяким наречием
		 */
		if(value.empty())
			// Выводим отрицательный результат проверки значения
			return false;
		/**
		 * Если значение несёт пробельную обвязку
		 */
		if(ascii::isSpace(value.front()) || ascii::isSpace(value.back()))
			// Выводим положительный результат проверки значения
			return true;
		/**
		 * Выполняем перебор всех знаков значения
		 */
		for(size_t i = 0; i < value.length(); i++){
			/**
			 * Если знаком является кавычка либо знак начала примечания
			 */
			if((value[i] == '"') || (inlineComments && commented(value[i], comments)))
				// Выводим положительный результат проверки значения
				return true;
		}
		// Выводим отрицательный результат проверки значения
		return false;
	}
	/**
	 * @brief Метод проверки знака на нужду в записи управляющей последовательностью
	 *
	 * @param letter проверяемый знак значения
	 * @return       результат проверки
	 *
	 */
	static bool escapable(const char letter) noexcept {
		// Выводим результат проверки знака
		return ((letter == '\n') || (letter == '\r') || (letter == '\t') || (letter == '\\') || (letter == '"'));
	}
	/**
	 * @brief Метод записи знака управляющей последовательностью
	 *
	 * @param letter записываемый знак значения
	 * @param result текст, к которому дописывается управляющая последовательность
	 *
	 */
	static void escaped(const char letter, string & result) noexcept {
		// Выполняем добавление обратной косой черты к тексту
		result.push_back('\\');
		/**
		 * Определяем записываемый знак значения
		 */
		switch(letter){
			// Если знаком является перевод строки
			case '\n':
				// Выполняем добавление обозначения знака к тексту
				result.push_back('n');
			break;
			// Если знаком является возврат каретки
			case '\r':
				// Выполняем добавление обозначения знака к тексту
				result.push_back('r');
			break;
			// Если знаком является горизонтальная табуляция
			case '\t':
				// Выполняем добавление обозначения знака к тексту
				result.push_back('t');
			break;
			// Если знак записывается сам собою за обратной косой чертой
			default:
				// Выполняем добавление знака к тексту
				result.push_back(letter);
		}
	}
};

/**
 * @brief Конструктор
 *
 */
awh::codec::ini::Writer::Settings::Settings() noexcept :
 marker(';'), separator('='), delimiter('.'), quoting(quoting_t::AUTO),
 subsections(subsection_t::NONE), newline(newline_t::LF), inlineComments(false), comments(marker_t::BOTH),
 spaces(true), escapes(false), indent(false), separated(true) {}
/**
 * @brief Метод получения настроек наречия MS Windows
 *
 * @return настройки записи наречия MS Windows
 *
 */
awh::codec::ini::Writer::Settings awh::codec::ini::Writer::Settings::windows() noexcept {
	// Собираемые настройки записи текста настроек
	Settings result;
	// Устанавливаем знак начала примечания
	result.marker = ';';
	// Устанавливаем знаки, признаваемые читающим началом примечания
	result.comments = marker_t::SEMICOLON;
	/**
	 * Устанавливаем обращение с ограждением значения кавычками
	 *
	 * @note Кавычки этим наречием при чтении не снимаются, и записывать их значило
	 *       бы отдать читающему значение вместе с ними
	 */
	result.quoting = quoting_t::NEVER;
	// Устанавливаем вид знака конца строки собираемого текста
	result.newline = newline_t::CRLF;
	// Устанавливаем запрет записи пробелов вокруг разделителя
	result.spaces = false;
	// Выводим собранные настройки записи
	return result;
}
/**
 * @brief Метод получения настроек наречия configparser языка Python
 *
 * @return настройки записи наречия configparser
 *
 */
awh::codec::ini::Writer::Settings awh::codec::ini::Writer::Settings::python() noexcept {
	// Собираемые настройки записи текста настроек
	Settings result;
	// Устанавливаем знак начала примечания
	result.marker = '#';
	// Устанавливаем знак разделителя имени и значения
	result.separator = '=';
	// Устанавливаем обращение с ограждением значения кавычками
	result.quoting = quoting_t::NEVER;
	// Выводим собранные настройки записи
	return result;
}
/**
 * @brief Метод получения настроек наречия описания служб systemd
 *
 * @return настройки записи наречия systemd
 *
 */
awh::codec::ini::Writer::Settings awh::codec::ini::Writer::Settings::systemd() noexcept {
	// Собираемые настройки записи текста настроек
	Settings result;
	// Устанавливаем знак начала примечания
	result.marker = '#';
	// Устанавливаем обращение с ограждением значения кавычками
	result.quoting = quoting_t::AUTO;
	// Устанавливаем запись управляющих последовательностей в значении
	result.escapes = true;
	// Устанавливаем запрет записи пробелов вокруг разделителя
	result.spaces = false;
	// Выводим собранные настройки записи
	return result;
}
/**
 * @brief Метод получения настроек наречия настроек Git
 *
 * @return настройки записи наречия Git
 *
 */
awh::codec::ini::Writer::Settings awh::codec::ini::Writer::Settings::git() noexcept {
	// Собираемые настройки записи текста настроек
	Settings result;
	// Устанавливаем знак начала примечания
	result.marker = '#';
	// Устанавливаем обращение с ограждением значения кавычками
	result.quoting = quoting_t::AUTO;
	// Устанавливаем построение имени подраздела кавычками
	result.subsections = subsection_t::QUOTED;
	// Устанавливаем признание примечания в конце строки читающим
	result.inlineComments = true;
	// Устанавливаем запись управляющих последовательностей в значении
	result.escapes = true;
	// Устанавливаем запись отступа перед свойствами раздела
	result.indent = true;
	// Выводим собранные настройки записи
	return result;
}
/**
 * @brief Метод проверки имени раздела или свойства
 *
 * @param name    проверяемое имя раздела или свойства
 * @param section признак проверки имени раздела
 * @return        результат выполнения операции
 *
 */
bool awh::codec::ini::Writer::verify(const string_view name, const bool section) noexcept {
	/**
	 * Если имя раздела или свойства пусто
	 */
	if(name.empty()){
		// Запоминаем код ошибки записи
		this->_error = (section ? error_t::EMPTY_SECTION : error_t::EMPTY_KEY);
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	/**
	 * Если имя раздела или свойства несёт пробельную обвязку
	 *
	 * @note Обвязка эта при чтении отбрасывается, и записанное имя разошлось бы с
	 *       прочитанным: отвергнуть её лучше, чем молча потерять
	 */
	if(ascii::isSpace(name.front()) || ascii::isSpace(name.back())){
		// Запоминаем код ошибки записи
		this->_error = (section ? error_t::INVALID_SECTION : error_t::INVALID_KEY);
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	/**
	 * Выполняем перебор всех знаков имени
	 */
	for(size_t i = 0; i < name.length(); i++){
		// Получаем признак недопустимости очередного знака имени
		bool invalid = ((name[i] == '\n') || (name[i] == '\r') || (name[i] == this->_settings.marker));
		/**
		 * Если проверяется имя раздела
		 */
		if(section)
			// Дополняем признак недопустимости знаками квадратных скобок и кавычки
			invalid = (invalid || (name[i] == '[') || (name[i] == ']') || (name[i] == '"'));
		/**
		 * Если проверяется имя свойства
		 */
		else invalid = (invalid || (name[i] == this->_settings.separator) || (name[i] == '[') || (name[i] == ']'));
		/**
		 * Если очередной знак имени недопустим
		 */
		if(invalid){
			// Запоминаем код ошибки записи
			this->_error = (section ? error_t::INVALID_SECTION : error_t::INVALID_KEY);
			// Выводим отрицательный результат выполнения операции
			return false;
		}
	}
	// Выводим положительный результат выполнения операции
	return true;
}
/**
 * @brief Метод записи значения свойства
 *
 * @param value записываемое значение свойства
 * @return      результат выполнения операции
 *
 */
bool awh::codec::ini::Writer::escape(const string_view value) noexcept {
	// Получаем признак нужды в ограждении значения кавычками
	const bool needed = ::quotable(value, this->_settings.comments, this->_settings.inlineComments);
	// Получаем признак ограждения значения кавычками
	const bool quoted = ((this->_settings.quoting == quoting_t::ALWAYS) || ((this->_settings.quoting == quoting_t::AUTO) && needed));
	/**
	 * Выполняем перебор всех знаков значения свойства
	 */
	for(size_t i = 0; i < value.length(); i++){
		/**
		 * Если знак записи управляющей последовательностью требует
		 */
		if(::escapable(value[i])){
			/**
			 * Если запись управляющих последовательностей настройками запрещена
			 */
			if(!this->_settings.escapes){
				/**
				 * Если знаком является знак конца строки
				 *
				 * @note Знак конца строки в значении без управляющей последовательности
				 *       записать нечем: ограждение кавычками его не спасает, поскольку
				 *       строка на нём обрывается, и разбор прочитал бы значение иначе
				 */
				if((value[i] == '\n') || (value[i] == '\r')){
					// Запоминаем код ошибки записи
					this->_error = error_t::INVALID_CHARACTER;
					// Выводим отрицательный результат выполнения операции
					return false;
				}
				/**
				 * Если знаком является кавычка при ограждении значения кавычками
				 */
				if((value[i] == '"') && quoted){
					// Запоминаем код ошибки записи
					this->_error = error_t::INVALID_CHARACTER;
					// Выводим отрицательный результат выполнения операции
					return false;
				}
			}
		}
	}
	/**
	 * Если значение ограждения кавычками требует, но настройками оно запрещено
	 */
	if(needed && !quoted && !this->_settings.escapes){
		// Запоминаем код ошибки записи
		this->_error = error_t::INVALID_CHARACTER;
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	/**
	 * Если значение ограждается кавычками
	 */
	if(quoted)
		// Выполняем запись открывающей кавычки значения
		this->_text.push_back('"');
	/**
	 * Выполняем перебор всех знаков значения свойства
	 */
	for(size_t i = 0; i < value.length(); i++){
		/**
		 * Если знак записывается управляющей последовательностью
		 */
		if(this->_settings.escapes && ::escapable(value[i])){
			/**
			 * Если знаком является кавычка вне ограждения кавычками
			 *
			 * @note Кавычка вне ограждения значения не портит: разбор снимает лишь
			 *       ту, что открывает значение, а прочие оставляет как есть. Записывать
			 *       её управляющей последовательностью там незачем
			 */
			if((value[i] == '"') && !quoted){
				// Выполняем запись знака значения
				this->_text.push_back(value[i]);
				// Выполняем переход к следующему знаку значения
				continue;
			}
			// Выполняем запись знака управляющей последовательностью
			::escaped(value[i], this->_text);
			// Выполняем переход к следующему знаку значения
			continue;
		}
		// Выполняем запись знака значения
		this->_text.push_back(value[i]);
	}
	/**
	 * Если значение ограждается кавычками
	 */
	if(quoted)
		// Выполняем запись закрывающей кавычки значения
		this->_text.push_back('"');
	// Выводим положительный результат выполнения операции
	return true;
}
/**
 * @brief Метод записи знака конца строки
 *
 */
void awh::codec::ini::Writer::newline() noexcept {
	// Выполняем запись знака конца строки
	this->_text.append(awh::codec::ini::newline(this->_settings.newline));
}
/**
 * @brief Метод получения текущих настроек записи
 *
 * @return текущие настройки записи текста настроек
 *
 */
const awh::codec::ini::Writer::settings_t & awh::codec::ini::Writer::settings() const noexcept {
	// Выводим текущие настройки записи текста настроек
	return this->_settings;
}
/**
 * @brief Метод установки настроек записи
 *
 * @param settings настройки записи текста настроек
 *
 */
void awh::codec::ini::Writer::settings(const settings_t & settings) noexcept {
	// Запоминаем настройки записи текста настроек
	this->_settings = settings;
}
/**
 * @brief Метод записи объявления раздела
 *
 * @param section    имя записываемого раздела
 * @param subsection имя записываемого подраздела
 * @return           результат выполнения операции
 *
 */
bool awh::codec::ini::Writer::section(const string_view section, const string_view subsection) noexcept {
	/**
	 * Если предыдущая операция записи завершилась ошибкой
	 */
	if(this->_error != error_t::NONE)
		// Выводим отрицательный результат выполнения операции
		return false;
	/**
	 * Если проверку имени раздела выполнить не удалось
	 */
	if(!this->verify(section, true))
		// Выводим отрицательный результат выполнения операции
		return false;
	/**
	 * Если имя подраздела передано, а построение его настройками не задано
	 */
	if(!subsection.empty() && (this->_settings.subsections == subsection_t::NONE)){
		// Запоминаем код ошибки записи
		this->_error = error_t::INVALID_SUBSECTION;
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	/**
	 * Если пустая строка перед объявлением раздела настройками задана
	 */
	if(this->_settings.separated && this->_sectioned)
		// Выполняем запись пустой строки перед объявлением раздела
		this->newline();
	// Выполняем запись открывающей квадратной скобки объявления
	this->_text.push_back('[');
	// Выполняем запись имени раздела
	this->_text.append(section);
	/**
	 * Если имя подраздела передано
	 */
	if(!subsection.empty()){
		/**
		 * Определяем построение имени подраздела
		 */
		switch(static_cast <uint8_t> (this->_settings.subsections)){
			// Если подраздел отделяется знаком-разделителем
			case static_cast <uint8_t> (subsection_t::DELIMITED): {
				/**
				 * Выполняем перебор всех знаков имени подраздела
				 */
				for(size_t i = 0; i < subsection.length(); i++){
					/**
					 * Если знак имени подраздела недопустим
					 */
					if((subsection[i] == ']') || (subsection[i] == '\n') || (subsection[i] == '\r')){
						// Запоминаем код ошибки записи
						this->_error = error_t::INVALID_SUBSECTION;
						// Выводим отрицательный результат выполнения операции
						return false;
					}
				}
				// Выполняем запись знака-разделителя имени подраздела
				this->_text.push_back(this->_settings.delimiter);
				// Выполняем запись имени подраздела
				this->_text.append(subsection);
			} break;
			// Если подраздел заключается в кавычки
			case static_cast <uint8_t> (subsection_t::QUOTED): {
				// Выполняем запись разделителя имени раздела и имени подраздела
				this->_text.push_back(' ');
				// Выполняем запись открывающей кавычки имени подраздела
				this->_text.push_back('"');
				/**
				 * Выполняем перебор всех знаков имени подраздела
				 */
				for(size_t i = 0; i < subsection.length(); i++){
					/**
					 * Если знак имени подраздела недопустим
					 */
					if((subsection[i] == '\n') || (subsection[i] == '\r')){
						// Запоминаем код ошибки записи
						this->_error = error_t::INVALID_SUBSECTION;
						// Выводим отрицательный результат выполнения операции
						return false;
					}
					/**
					 * Если знак имени подраздела ограждения требует
					 */
					if((subsection[i] == '"') || (subsection[i] == '\\'))
						// Выполняем запись обратной косой черты перед знаком
						this->_text.push_back('\\');
					// Выполняем запись знака имени подраздела
					this->_text.push_back(subsection[i]);
				}
				// Выполняем запись закрывающей кавычки имени подраздела
				this->_text.push_back('"');
			} break;
		}
	}
	// Выполняем запись закрывающей квадратной скобки объявления
	this->_text.push_back(']');
	// Выполняем запись знака конца строки
	this->newline();
	// Запоминаем признак того, что раздел текста настроек объявлен
	this->_sectioned = true;
	// Выводим положительный результат выполнения операции
	return true;
}
/**
 * @brief Метод записи свойства со значением
 *
 * @param key   имя записываемого свойства
 * @param value значение записываемого свойства
 * @return      результат выполнения операции
 *
 */
/**
 * @brief Метод записи свойства добавлением к перечню значений
 *
 * @param key    имя записываемого свойства
 * @param value  записываемое значение свойства
 * @param append признак добавления значения к перечню
 * @return       результат выполнения операции
 *
 */
bool awh::codec::ini::Writer::property(const string_view key, const string_view value, const bool append) noexcept {
	/**
	 * Если добавление значения к перечню не запрошено
	 */
	if(!append)
		// Выполняем запись свойства со значением обычной записью
		return this->property(key, value);
	/**
	 * Если предыдущая операция записи завершилась ошибкой
	 */
	if(this->_error != error_t::NONE)
		// Выводим отрицательный результат выполнения операции
		return false;
	/**
	 * Если проверку имени свойства выполнить не удалось
	 *
	 * @note Проверяется имя без скобок: сами скобки в имени недопустимы, и запись
	 *       их к имени добавляет уже этот метод
	 */
	if(!this->verify(key, false))
		// Выводим отрицательный результат выполнения операции
		return false;
	/**
	 * Если запись отступа перед свойствами раздела настройками задана
	 */
	if(this->_settings.indent && this->_sectioned)
		// Выполняем запись отступа перед именем свойства
		this->_text.push_back('\t');
	// Выполняем запись имени свойства
	this->_text.append(key);
	// Выполняем запись скобок добавления к перечню значений
	this->_text.append("[]");
	/**
	 * Если запись пробелов вокруг разделителя настройками задана
	 */
	if(this->_settings.spaces)
		// Выполняем запись пробела перед разделителем
		this->_text.push_back(' ');
	// Выполняем запись разделителя имени свойства и его значения
	this->_text.push_back(this->_settings.separator);
	/**
	 * Если запись пробелов вокруг разделителя настройками задана
	 */
	if(this->_settings.spaces)
		// Выполняем запись пробела за разделителем
		this->_text.push_back(' ');
	/**
	 * Если запись значения свойства выполнить не удалось
	 */
	if(!this->escape(value))
		// Выводим отрицательный результат выполнения операции
		return false;
	// Выполняем запись знака конца строки
	this->newline();
	// Выводим положительный результат выполнения операции
	return true;
}
bool awh::codec::ini::Writer::property(const string_view key, const string_view value) noexcept {
	/**
	 * Если предыдущая операция записи завершилась ошибкой
	 */
	if(this->_error != error_t::NONE)
		// Выводим отрицательный результат выполнения операции
		return false;
	/**
	 * Если проверку имени свойства выполнить не удалось
	 */
	if(!this->verify(key, false))
		// Выводим отрицательный результат выполнения операции
		return false;
	/**
	 * Если запись отступа перед свойствами раздела настройками задана
	 */
	if(this->_settings.indent && this->_sectioned)
		// Выполняем запись отступа перед именем свойства
		this->_text.push_back('\t');
	// Выполняем запись имени свойства
	this->_text.append(key);
	/**
	 * Если запись пробелов вокруг разделителя настройками задана
	 */
	if(this->_settings.spaces)
		// Выполняем запись пробела перед разделителем
		this->_text.push_back(' ');
	// Выполняем запись разделителя имени свойства и его значения
	this->_text.push_back(this->_settings.separator);
	/**
	 * Если запись пробелов вокруг разделителя настройками задана
	 */
	if(this->_settings.spaces)
		// Выполняем запись пробела за разделителем
		this->_text.push_back(' ');
	/**
	 * Если запись значения свойства выполнить не удалось
	 */
	if(!this->escape(value))
		// Выводим отрицательный результат выполнения операции
		return false;
	// Выполняем запись знака конца строки
	this->newline();
	// Выводим положительный результат выполнения операции
	return true;
}
/**
 * @brief Метод записи свойства без разделителя и значения
 *
 * @param key имя записываемого свойства
 * @return    результат выполнения операции
 *
 */
bool awh::codec::ini::Writer::property(const string_view key) noexcept {
	/**
	 * Если предыдущая операция записи завершилась ошибкой
	 */
	if(this->_error != error_t::NONE)
		// Выводим отрицательный результат выполнения операции
		return false;
	/**
	 * Если проверку имени свойства выполнить не удалось
	 */
	if(!this->verify(key, false))
		// Выводим отрицательный результат выполнения операции
		return false;
	/**
	 * Если запись отступа перед свойствами раздела настройками задана
	 */
	if(this->_settings.indent && this->_sectioned)
		// Выполняем запись отступа перед именем свойства
		this->_text.push_back('\t');
	// Выполняем запись имени свойства
	this->_text.append(key);
	// Выполняем запись знака конца строки
	this->newline();
	// Выводим положительный результат выполнения операции
	return true;
}
/**
 * @brief Метод записи примечания
 *
 * @param text содержимое записываемого примечания
 * @return     результат выполнения операции
 *
 */
bool awh::codec::ini::Writer::comment(const string_view text) noexcept {
	/**
	 * Если предыдущая операция записи завершилась ошибкой
	 */
	if(this->_error != error_t::NONE)
		// Выводим отрицательный результат выполнения операции
		return false;
	// Положение начала очередной строки примечания
	size_t offset = 0;
	/**
	 * Выполняем перебор всех строк примечания
	 */
	while(true){
		// Получаем положение конца очередной строки примечания
		size_t position = text.find('\n', offset);
		/**
		 * Если знак конца строки примечания не обнаружен
		 */
		if(position == string_view::npos)
			// Запоминаем положение конца примечания
			position = text.length();
		// Получаем содержимое очередной строки примечания
		string_view line = text.substr(offset, position - offset);
		/**
		 * Если строка примечания оканчивается возвратом каретки
		 */
		if(!line.empty() && (line.back() == '\r'))
			// Выполняем отбрасывание возврата каретки
			line = line.substr(0, line.length() - 1);
		// Выполняем запись знака начала примечания
		this->_text.push_back(this->_settings.marker);
		/**
		 * Если содержимое строки примечания не пусто
		 */
		if(!line.empty()){
			// Выполняем запись пробела за знаком начала примечания
			this->_text.push_back(' ');
			// Выполняем запись содержимого строки примечания
			this->_text.append(line);
		}
		// Выполняем запись знака конца строки
		this->newline();
		/**
		 * Если примечание записано целиком
		 */
		if(position >= text.length())
			// Выполняем прекращение записи примечания
			break;
		// Выполняем переход к следующей строке примечания
		offset = (position + 1);
	}
	// Выводим положительный результат выполнения операции
	return true;
}
/**
 * @brief Метод дописывания примечания к последней записанной строке
 *
 * @param text содержимое дописываемого примечания
 * @return     результат выполнения операции
 *
 */
bool awh::codec::ini::Writer::trailing(const string_view text) noexcept {
	/**
	 * Если предыдущая операция записи завершилась ошибкой
	 */
	if(this->_error != error_t::NONE)
		// Выводим отрицательный результат выполнения операции
		return false;
	// Получаем последовательность знаков конца строки
	const string_view newline = awh::codec::ini::newline(this->_settings.newline);
	/**
	 * Если записанного текста для дописывания примечания недостаточно
	 */
	if(this->_text.length() < newline.length()){
		// Запоминаем код ошибки записи
		this->_error = error_t::INTERNAL;
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	/**
	 * Если записанный текст знаком конца строки не оканчивается
	 */
	if(this->_text.compare(this->_text.length() - newline.length(), newline.length(), newline) != 0){
		// Запоминаем код ошибки записи
		this->_error = error_t::INTERNAL;
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	/**
	 * Выполняем снятие знака конца строки с записанного текста
	 *
	 * @note Примечание дописывается к строке, которая уже закрыта: снять знак её
	 *       конца и поставить его заново дешевле, чем откладывать закрытие всякой
	 *       строки в ожидании примечания, которого чаще всего не будет
	 */
	this->_text.resize(this->_text.length() - newline.length());
	// Выполняем запись пробела перед знаком начала примечания
	this->_text.push_back(' ');
	// Выполняем запись знака начала примечания
	this->_text.push_back(this->_settings.marker);
	/**
	 * Если содержимое примечания не пусто
	 */
	if(!text.empty()){
		// Выполняем запись пробела за знаком начала примечания
		this->_text.push_back(' ');
		// Выполняем запись содержимого примечания
		this->_text.append(text);
	}
	// Выполняем запись знака конца строки
	this->newline();
	// Выводим положительный результат выполнения операции
	return true;
}
/**
 * @brief Метод записи пустой строки
 *
 * @return результат выполнения операции
 *
 */
bool awh::codec::ini::Writer::blank() noexcept {
	/**
	 * Если предыдущая операция записи завершилась ошибкой
	 */
	if(this->_error != error_t::NONE)
		// Выводим отрицательный результат выполнения операции
		return false;
	// Выполняем запись знака конца строки
	this->newline();
	// Выводим положительный результат выполнения операции
	return true;
}
/**
 * @brief Метод получения кода ошибки записи
 *
 * @return код ошибки последней операции записи
 *
 */
awh::codec::ini::error_t awh::codec::ini::Writer::error() const noexcept {
	// Выводим код ошибки последней операции записи
	return this->_error;
}
/**
 * @brief Метод получения собранного текста настроек
 *
 * @return собранный текст настроек
 *
 */
const string & awh::codec::ini::Writer::text() const noexcept {
	// Выводим собранный текст настроек
	return this->_text;
}
/**
 * @brief Метод сброса записи в исходное состояние
 *
 */
void awh::codec::ini::Writer::clear() noexcept {
	// Выполняем сброс кода ошибки записи
	this->_error = error_t::NONE;
	// Выполняем сброс признака объявления раздела
	this->_sectioned = false;
	// Выполняем очистку собранного текста настроек
	this->_text.clear();
}
/**
 * @brief Конструктор
 *
 */
awh::codec::ini::Writer::Writer() noexcept : _error(error_t::NONE), _sectioned(false) {}
/**
 * @brief Конструктор
 *
 * @param settings настройки записи текста настроек
 *
 */
awh::codec::ini::Writer::Writer(const settings_t & settings) noexcept :
 _error(error_t::NONE), _sectioned(false), _settings(settings) {}
/**
 * @brief Деструктор
 *
 */
awh::codec::ini::Writer::~Writer() noexcept {
	// Выполняем очистку собранного текста настроек
	this->_text.clear();
}
/**
 * @brief Шаблон типа записываемого числа
 *
 * @tparam T тип записываемого числа
 *
 */
template <typename T>
/**
 * @brief Метод записи свойства с числовым значением
 *
 * @param key   имя записываемого свойства
 * @param value значение записываемого свойства
 * @return      результат выполнения операции
 *
 */
bool awh::codec::ini::Writer::number(const string_view key, const T value) noexcept {
	// Хранилище записи числового значения свойства
	char buffer[64];
	// Длина записи числового значения свойства
	int32_t length = 0;
	/**
	 * Если записывается логическое значение
	 *
	 * @note Сличение ведётся прежде целых чисел намеренно: логический тип языком
	 *       причислен к целым, и без этого истина записалась бы единицей
	 */
	if constexpr(is_same <T, bool>::value)
		// Выполняем запись свойства с логическим значением
		return this->property(key, (value ? "true" : "false"));
	/**
	 * Если записывается число с плавающей точкой
	 */
	else if constexpr(is_floating_point <T>::value)
		/**
		 * Выполняем запись числа с плавающей точкой
		 *
		 * @note Точность записи взята наибольшей из тех, что переживают обратный
		 *       разбор без потери разрядов: запись покороче теряла бы младшие
		 *       разряды при первом же обороте «запись - чтение»
		 */
		length = ::snprintf(buffer, sizeof(buffer), "%.17g", static_cast <double> (value));
	/**
	 * Если записывается целое число без знака
	 */
	else if constexpr(is_unsigned <T>::value)
		// Выполняем запись целого числа без знака
		length = ::snprintf(buffer, sizeof(buffer), "%llu", static_cast <unsigned long long> (value));
	/**
	 * Если записывается целое число со знаком
	 */
	else length = ::snprintf(buffer, sizeof(buffer), "%lld", static_cast <long long> (value));
	/**
	 * Если запись числового значения выполнить не удалось
	 */
	if((length <= 0) || (static_cast <size_t> (length) >= sizeof(buffer))){
		// Запоминаем код ошибки записи
		this->_error = error_t::INTERNAL;
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	// Выполняем запись свойства с числовым значением
	return this->property(key, string_view(buffer, static_cast <size_t> (length)));
}

/**
 * Выполняем порождение метода записи свойства с числовым значением для всех поддерживаемых типов
 */
template __AWH_SHARED_EXPORT__ bool awh::codec::ini::Writer::number <bool> (const string_view, const bool) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::ini::Writer::number <int8_t> (const string_view, const int8_t) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::ini::Writer::number <uint8_t> (const string_view, const uint8_t) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::ini::Writer::number <int16_t> (const string_view, const int16_t) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::ini::Writer::number <uint16_t> (const string_view, const uint16_t) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::ini::Writer::number <int32_t> (const string_view, const int32_t) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::ini::Writer::number <uint32_t> (const string_view, const uint32_t) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::ini::Writer::number <int64_t> (const string_view, const int64_t) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::ini::Writer::number <uint64_t> (const string_view, const uint64_t) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::ini::Writer::number <float> (const string_view, const float) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::ini::Writer::number <double> (const string_view, const double) noexcept;

/**
 * Возвращаем макросы, снятые в начале файла
 */
#include <sys/macro_pop.hpp>
