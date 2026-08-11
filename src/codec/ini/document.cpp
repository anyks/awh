/**
 * @file: document.cpp
 * @date: 2026-08-10
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация дерева настроек INI — сборка записей разобранного текста с сохранением
 *        его оформления, поиск по разделам и свойствам, подстановка обращений к значениям,
 *        правка на месте и обратная запись собранного дерева в текст
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <encoding/ascii.hpp>
#include <codec/ini/document.hpp>

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
 * @brief Конструктор
 *
 */
awh::codec::ini::Document::Settings::Settings() noexcept :
 references(reference_t::NONE), maxDepth(MAX_REFERENCE_DEPTH), maxExpansion(MAX_EXPANSION) {}
/**
 * @brief Метод получения содержимого отрезка хранилища знаков
 *
 * @param span отрезок общего хранилища знаков
 * @return     содержимое отрезка хранилища знаков
 *
 */
string_view awh::codec::ini::Document::get(const span_t & span) const noexcept {
	/**
	 * Если отрезок хранилища за его пределы выходит
	 */
	if((static_cast <size_t> (span.offset) + static_cast <size_t> (span.length)) > this->_store.length())
		// Выводим пустую последовательность знаков
		return string_view();
	// Выводим содержимое отрезка хранилища знаков
	return string_view(this->_store.data() + span.offset, span.length);
}
/**
 * @brief Метод добавления содержимого к хранилищу знаков
 *
 * @param text добавляемое к хранилищу содержимое
 * @return     отрезок хранилища с добавленным содержимым
 *
 */
awh::codec::ini::span_t awh::codec::ini::Document::add(const string_view text) noexcept {
	/**
	 * Если хранилище знаков предел разрядности отрезка превышает
	 *
	 * @note Отрезок хранит место указателями в четыре октета, и текст свыше четырёх
	 *       гигабайт обрезал бы их молча: лучше отдать пустой отрезок и отказ, чем
	 *       выдавать после этого чужое содержимое
	 */
	if((this->_store.length() + text.length()) > static_cast <size_t> (NO_RECORD)){
		// Запоминаем код ошибки правки
		this->_error = error_t::OVERFLOW_LIMIT;
		// Выводим пустой отрезок хранилища знаков
		return span_t();
	}
	// Собираемый отрезок хранилища знаков
	span_t result(static_cast <uint32_t> (this->_store.length()), static_cast <uint32_t> (text.length()));
	// Выполняем добавление содержимого к хранилищу знаков
	this->_store.append(text);
	// Выводим собранный отрезок хранилища знаков
	return result;
}
/**
 * @brief Метод приведения имени к виду для сличения
 *
 * @param name приводимое имя раздела или свойства
 * @return     имя, приведённое к виду для сличения
 *
 */
string awh::codec::ini::Document::fold(const string_view name, const bool section) const noexcept {
	// Приводимое имя раздела или свойства
	string result(name);
	/**
	 * Если регистр имён при сличении не учитывается
	 */
	if(!this->_settings.reader.sensitive && !(section && this->_settings.reader.sensitiveSections)){
		/**
		 * Выполняем перебор всех знаков имени
		 */
		for(size_t i = 0; i < result.length(); i++)
			// Выполняем приведение очередного знака имени к нижнему регистру
			result[i] = ascii::toLower(result[i]);
	}
	// Выводим приведённое имя раздела или свойства
	return result;
}
/**
 * @brief Метод получения вида последней записи, идущей в выдачу
 *
 * @details Записи, снятые пометкой удаления, при записи текста пропускаются, и
 * последней видимой является ближайшая к концу запись, пометки не несущая
 *
 * @return вид последней записи, идущей в выдачу
 *
 */
awh::codec::ini::kind_t awh::codec::ini::Document::kind() const noexcept {
	/**
	 * Выполняем перебор записей разобранного текста с конца
	 */
	for(size_t i = this->_records.size(); i > 0; i--){
		/**
		 * Если очередная запись пометки удаления не несёт
		 */
		if(this->_records.at(i - 1).kind != kind_t::NONE)
			// Выводим вид последней записи, идущей в выдачу
			return this->_records.at(i - 1).kind;
	}
	// Выводим вид отсутствующей записи
	return kind_t::NONE;
}
/**
 * @brief Метод проверки имени раздела или свойства
 *
 * @param name    проверяемое имя раздела или свойства
 * @param section признак проверки имени раздела
 * @param primary признак проверки имени самого раздела, а не подраздела
 * @return        результат выполнения операции
 *
 */
bool awh::codec::ini::Document::acceptable(const string_view name, const bool section, const bool primary) noexcept {
	/**
	 * Если имя раздела или свойства пусто
	 */
	if(name.empty()){
		// Запоминаем код ошибки правки
		this->_error = (section ? error_t::EMPTY_SECTION : error_t::EMPTY_KEY);
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	/**
	 * Если длина имени предел настроек превышает
	 */
	if(name.length() > this->_settings.reader.maxName){
		// Запоминаем код ошибки правки
		this->_error = error_t::NAME_TOO_LONG;
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
		// Запоминаем код ошибки правки
		this->_error = (section ? error_t::INVALID_SECTION : error_t::INVALID_KEY);
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	/**
	 * Выполняем перебор всех знаков имени
	 */
	for(size_t i = 0; i < name.length(); i++){
		/**
		 * Получаем признак недопустимости очередного знака имени
		 *
		 * @note Знак примечания недопустим лишь первым знаком имени свойства: там он
		 *       обращает всю строку в примечание. Внутри имени он значащим не является
		 *       и запрету не подлежит - иначе прочитанное имя правкой не принималось бы
		 */
		bool invalid = ((name[i] == '\n') || (name[i] == '\r') || (name[i] == '[') || (name[i] == ']') ||
		                (!section && (i == 0) && commented(name[i], this->_settings.reader.comments)));
		/**
		 * Если проверяется имя подраздела, заключаемого в кавычки
		 *
		 * @note Внутри кавычек значащи лишь знаки конца строки: кавычку и обратную
		 *       косую черту запись ограждает, а квадратная скобка там знак
		 *       обыкновенный. Запрет их отвергал бы правкой то, что читается
		 */
		if(section && !primary && (this->_settings.reader.subsections == subsection_t::QUOTED))
			// Получаем признак недопустимости очередного знака имени подраздела
			invalid = ((name[i] == '\n') || (name[i] == '\r'));
		/**
		 * Если проверяется имя раздела
		 */
		else if(section){
			// Дополняем признак недопустимости знаком кавычки
			invalid = (invalid || (name[i] == '"'));
			/**
			 * Если проверяется имя самого раздела, а подраздел отделяется разделителем
			 *
			 * @note Имени подраздела знак этот не заказан: читающий режет имя раздела по
			 *       первому его появлению, и всё, что за ним, именем подраздела и служит
			 */
			if(primary && (this->_settings.reader.subsections == subsection_t::DELIMITED))
				// Дополняем признак недопустимости знаком-разделителем имени подраздела
				invalid = (invalid || (name[i] == this->_settings.reader.delimiter));
		}
		/**
		 * Если проверяется имя свойства
		 */
		else invalid = (invalid || separated(name[i], this->_settings.reader.separators));
		/**
		 * Если очередной знак имени недопустим
		 */
		if(invalid){
			// Запоминаем код ошибки правки
			this->_error = (section ? error_t::INVALID_SECTION : error_t::INVALID_KEY);
			// Выводим отрицательный результат выполнения операции
			return false;
		}
	}
	// Выводим положительный результат выполнения операции
	return true;
}
/**
 * @brief Метод сборки ключа указателя разделов
 *
 * @param section    имя раздела
 * @param subsection имя подраздела
 * @return           ключ указателя разделов
 *
 */
string awh::codec::ini::Document::label(const string_view section, const string_view subsection) const noexcept {
	// Собираемый ключ указателя разделов
	string result = this->fold(section, true);
	/**
	 * Выполняем добавление разделителя имён к собираемому ключу
	 *
	 * @note Имена соединяются через нулевой байт намеренно: разделитель этот в
	 *       именах не встречается, и раздел «a» с подразделом «b» не сольётся с
	 *       разделом «a.b», объявленным иначе
	 */
	result.push_back('\0');
	/**
	 * Выполняем добавление имени подраздела к собираемому ключу
	 *
	 * @note Имя подраздела в сличении регистр учитывает всегда: таково обращение
	 *       настроек Git, где подраздел несёт то имя ветви, то обозначение
	 *       источника, а в них регистр значащ
	 */
	result.append(subsection);
	// Выводим собранный ключ указателя разделов
	return result;
}
/**
 * @brief Метод сборки ключа указателя свойств
 *
 * @param section порядковый номер раздела
 * @param key     имя свойства
 * @return        ключ указателя свойств
 *
 */
string awh::codec::ini::Document::label(const uint32_t section, const string_view key) const noexcept {
	// Собираемый ключ указателя свойств
	string result = to_string(section);
	// Выполняем добавление разделителя к собираемому ключу
	result.push_back('\0');
	// Выполняем добавление имени свойства к собираемому ключу
	result.append(this->fold(key));
	// Выводим собранный ключ указателя свойств
	return result;
}
/**
 * @brief Метод дозаписи имени к ключу с приведением регистра
 *
 * @param buffer буфер, куда дозаписывается имя
 * @param name   дозаписываемое имя
 * @param lower  признак приведения имени к нижнему регистру
 *
 */
static void appended(string & buffer, const string_view name, const bool lower) noexcept {
	/**
	 * Если приведение имени к нижнему регистру не требуется
	 */
	if(!lower){
		// Выполняем дозапись имени к ключу как есть
		buffer.append(name);
		// Выходим из функции
		return;
	}
	// Запоминаем длину ключа до дозаписи имени
	const size_t offset = buffer.length();
	// Выполняем дозапись имени к ключу
	buffer.append(name);
	/**
	 * Выполняем перебор дозаписанных знаков имени
	 *
	 * @note Приведение выполняется поверх дозаписанного, а не отдельной строкой:
	 *       дозапись переносит имя блоком, что заметно дешевле переноса знака за
	 *       знаком, а приведение проходит по нему уже на месте
	 */
	for(size_t i = offset; i < buffer.length(); i++)
		// Выполняем приведение очередного знака имени к нижнему регистру
		buffer[i] = awh::ascii::toLower(buffer[i]);
}
/**
 * @brief Метод сборки ключа указателя разделов в переданный буфер
 *
 * @param buffer     буфер, куда собирается ключ
 * @param section    имя раздела
 * @param subsection имя подраздела
 *
 */
void awh::codec::ini::Document::labelled(string & buffer, const string_view section, const string_view subsection) const noexcept {
	// Выполняем очистку переданного буфера
	buffer.clear();
	// Выполняем дозапись имени раздела с приведением регистра
	::appended(buffer, section, (!this->_settings.reader.sensitive && !this->_settings.reader.sensitiveSections));
	/**
	 * Выполняем добавление разделителя имён к собираемому ключу
	 *
	 * @note Имена соединяются через нулевой байт намеренно: разделитель этот в
	 *       именах не встречается, и раздел «a» с подразделом «b» не сольётся с
	 *       разделом «a.b», объявленным иначе
	 */
	buffer.push_back('\0');
	/**
	 * Выполняем добавление имени подраздела к собираемому ключу
	 *
	 * @note Имя подраздела в сличении регистр учитывает всегда: таково обращение
	 *       настроек Git, где подраздел несёт то имя ветви, то обозначение
	 *       источника, а в них регистр значащ
	 */
	buffer.append(subsection);
}
/**
 * @brief Метод сборки ключа указателя свойств в переданный буфер
 *
 * @param buffer  буфер, куда собирается ключ
 * @param section порядковый номер раздела
 * @param key     имя свойства
 *
 */
void awh::codec::ini::Document::labelled(string & buffer, const uint32_t section, const string_view key) const noexcept {
	// Выполняем очистку переданного буфера
	buffer.clear();
	// Запись порядкового номера раздела десятичными разрядами
	char digits[16];
	// Положение начала записи порядкового номера раздела
	size_t position = sizeof(digits);
	// Записываемый порядковый номер раздела
	uint32_t number = section;
	/**
	 * Выполняем запись порядкового номера раздела с младшего разряда
	 *
	 * @note Запись выполняется своими средствами, а не std::to_string: та строит
	 *       строку, которую пришлось бы переносить в буфер ещё раз
	 */
	do {
		// Записываем очередной разряд порядкового номера раздела
		digits[--position] = static_cast <char> ('0' + (number % 10));
		// Выполняем переход к следующему разряду порядкового номера раздела
		number /= 10;
	// Выполняем запись до исчерпания разрядов порядкового номера раздела
	} while(number > 0);
	// Выполняем дозапись порядкового номера раздела к собираемому ключу
	buffer.append(digits + position, sizeof(digits) - position);
	// Выполняем добавление разделителя к собираемому ключу
	buffer.push_back('\0');
	// Выполняем дозапись имени свойства с приведением регистра
	::appended(buffer, key, !this->_settings.reader.sensitive);
}
/**
 * @brief Метод поиска раздела по имени с переиспользуемым буфером
 *
 * @param buffer     буфер сборки ключа указателя разделов
 * @param section    имя искомого раздела
 * @param subsection имя искомого подраздела
 * @param result     порядковый номер найденного раздела
 * @return           признак того, что раздел найден
 *
 */
bool awh::codec::ini::Document::search(string & buffer, const string_view section, const string_view subsection, uint32_t & result) const noexcept {
	/**
	 * Если искомым является раздел без имени
	 */
	if(section.empty() && subsection.empty()){
		// Запоминаем порядковый номер раздела без имени
		result = 0;
		// Выводим признак того, что раздел найден
		return !this->_sections.empty();
	}
	// Выполняем сборку ключа указателя разделов
	this->labelled(buffer, section, subsection);
	// Выполняем поиск раздела в указателе разделов
	const auto i = this->_index.find(buffer);
	/**
	 * Если раздел в указателе не обнаружен
	 */
	if(i == this->_index.end())
		// Выводим признак того, что раздел не найден
		return false;
	// Запоминаем порядковый номер найденного раздела
	result = i->second;
	// Выводим признак того, что раздел найден
	return true;
}
/**
 * @brief Метод поиска объявлений свойства по имени
 *
 * @param key        имя искомого свойства
 * @param section    имя раздела
 * @param subsection имя подраздела
 * @return           перечень объявлений свойства либо пустой указатель
 *
 */
const vector <uint32_t> * awh::codec::ini::Document::locate(const string_view key, const string_view section, const string_view subsection) const noexcept {
	// Буфер сборки ключей указателей
	string buffer;
	// Порядковый номер найденного раздела
	uint32_t index = 0;
	/**
	 * Если раздел с таким именем не обнаружен
	 */
	if(!this->search(buffer, section, subsection, index))
		// Выводим пустой указатель
		return nullptr;
	// Выполняем сборку ключа указателя свойств
	this->labelled(buffer, index, key);
	// Выполняем поиск свойства в указателе свойств
	const auto i = this->_properties.find(buffer);
	/**
	 * Если свойство с таким именем не обнаружено либо объявлений у него не осталось
	 */
	if((i == this->_properties.end()) || i->second.empty())
		// Выводим пустой указатель
		return nullptr;
	// Выводим перечень объявлений найденного свойства
	return &i->second;
}
/**
 * @brief Метод поиска раздела по имени
 *
 * @param section    имя искомого раздела
 * @param subsection имя искомого подраздела
 * @param result     порядковый номер найденного раздела
 * @return           признак того, что раздел найден
 *
 */
bool awh::codec::ini::Document::search(const string_view section, const string_view subsection, uint32_t & result) const noexcept {
	/**
	 * Если искомым является раздел без имени
	 */
	if(section.empty() && subsection.empty()){
		// Запоминаем порядковый номер раздела без имени
		result = 0;
		// Выводим признак того, что раздел найден
		return !this->_sections.empty();
	}
	// Выполняем поиск раздела в указателе разделов
	const auto i = this->_index.find(this->label(section, subsection));
	/**
	 * Если раздел в указателе не обнаружен
	 */
	if(i == this->_index.end())
		// Выводим признак того, что раздел не найден
		return false;
	// Запоминаем порядковый номер найденного раздела
	result = i->second;
	// Выводим признак того, что раздел найден
	return true;
}
/**
 * @brief Метод перестроения указателей поиска
 *
 */
void awh::codec::ini::Document::reindex() noexcept {
	// Выполняем очистку указателя разделов
	this->_index.clear();
	// Выполняем очистку указателя свойств
	this->_properties.clear();
	// Выполняем очистку порядка первых объявлений свойств
	this->_order.clear();
	// Выполняем очистку последних записей разделов
	this->_last.clear();
	// Выполняем выделение памяти под порядок объявлений всех разделов
	this->_order.resize(this->_sections.size());
	// Выполняем выделение памяти под последние записи всех разделов
	this->_last.resize(this->_sections.size(), NO_RECORD);
	/**
	 * Выполняем перебор всех разделов разобранного текста
	 */
	for(size_t i = 0; i < this->_sections.size(); i++){
		/**
		 * Если раздел в тексте настроек не объявлен
		 */
		if(!this->_sections.at(i).declared)
			// Выполняем переход к следующему разделу
			continue;
		// Выполняем добавление раздела к указателю разделов
		this->_index.emplace(this->label(this->get(this->_sections.at(i).name), this->get(this->_sections.at(i).subsection)), static_cast <uint32_t> (i));
	}
	/**
	 * Выполняем перебор всех записей разобранного текста
	 */
	for(size_t i = 0; i < this->_records.size(); i++){
		/**
		 * Если раздел, которому запись принадлежит, в перечне разделов существует
		 */
		if(this->_records.at(i).section < this->_last.size())
			// Запоминаем запись последней записью своего раздела
			this->_last.at(this->_records.at(i).section) = static_cast <uint32_t> (i);
		/**
		 * Если запись свойством не является
		 */
		if(this->_records.at(i).kind != kind_t::PROPERTY)
			// Выполняем переход к следующей записи
			continue;
		// Получаем перечень объявлений свойства в указателе свойств
		vector <uint32_t> & records = this->_properties[this->label(this->_records.at(i).section, this->get(this->_records.at(i).key))];
		/**
		 * Если свойство объявляется в разделе впервые
		 *
		 * @note Порядок первых объявлений собирается здесь же, попутно: обход
		 *       записей ради него отдельным проходом ничего бы не дал, а признак
		 *       первого объявления виден по пустоте перечня
		 */
		if(records.empty() && (this->_records.at(i).section < this->_order.size()))
			// Выполняем добавление записи к порядку первых объявлений раздела
			this->_order.at(this->_records.at(i).section).push_back(static_cast <uint32_t> (i));
		// Выполняем добавление свойства к указателю свойств
		records.push_back(static_cast <uint32_t> (i));
	}
}
/**
 * @brief Метод разрешения обращений внутри значения свойства
 *
 * @param value   разрешаемое значение свойства
 * @param section порядковый номер раздела, которому значение принадлежит
 * @param stack   перечень свойств, разрешаемых в настоящее время
 * @param budget  остаток допустимого объёма подстановки в байтах
 * @param result  значение с разрешёнными обращениями
 * @return        результат выполнения операции
 *
 */
bool awh::codec::ini::Document::expand(const string_view value, const uint32_t section, vector <string> & stack, uint64_t & budget, string & result) noexcept {
	/**
	 * Если глубина вложенности обращений предел настроек превышает
	 */
	if(static_cast <uint64_t> (stack.size()) > static_cast <uint64_t> (this->_settings.maxDepth)){
		// Запоминаем код ошибки разбора
		this->_error = error_t::REFERENCE_DEPTH;
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	/**
	 * Выполняем перебор всех знаков разрешаемого значения
	 */
	for(size_t i = 0; i < value.length(); i++){
		// Знак, которым начинается обращение к значению
		const char letter = ((this->_settings.references == reference_t::SHELL) ? '$' : '%');
		// Знак, которым открывается имя в обращении к значению
		const char opening = ((this->_settings.references == reference_t::SHELL) ? '{' : '(');
		/**
		 * Если очередной знак обращением к значению не является
		 */
		if(value[i] != letter){
			/**
			 * Если допустимый объём подстановки исчерпан
			 */
			if((budget--) == 0){
				// Запоминаем код ошибки разбора
				this->_error = error_t::EXPANSION_EXCEEDED;
				// Выводим отрицательный результат выполнения операции
				return false;
			}
			// Выполняем добавление знака к разрешённому значению
			result.push_back(value[i]);
			// Выполняем переход к следующему знаку значения
			continue;
		}
		/**
		 * Если знак обращения удвоен
		 *
		 * @note Удвоение служит записи самого знака: без него значение с денежным
		 *       знаком либо с долей в сотых записать было бы нечем
		 */
		if(((i + 1) < value.length()) && (value[i + 1] == letter)){
			/**
			 * Если остаток допустимого объёма подстановки исчерпан
			 */
			if((budget--) == 0){
				// Запоминаем код ошибки разбора
				this->_error = error_t::EXPANSION_EXCEEDED;
				// Выводим отрицательный результат выполнения операции
				return false;
			}
			// Выполняем добавление знака к разрешённому значению
			result.push_back(value[i]);
			// Выполняем переход к знаку за удвоенным
			i++;
			// Выполняем переход к следующему знаку значения
			continue;
		}
		/**
		 * Если за знаком обращения имя не открывается
		 */
		if(((i + 1) >= value.length()) || (value[i + 1] != opening)){
			/**
			 * Если остаток допустимого объёма подстановки исчерпан
			 *
			 * @note Остаток убывает на всяком знаке разрешённого значения, а не только
			 *       на подставленном: предел задан объёму подстановки целиком, и знак,
			 *       мимо него проходящий, обращал бы предел в необязательный
			 */
			if((budget--) == 0){
				// Запоминаем код ошибки разбора
				this->_error = error_t::EXPANSION_EXCEEDED;
				// Выводим отрицательный результат выполнения операции
				return false;
			}
			// Выполняем добавление знака к разрешённому значению
			result.push_back(value[i]);
			// Выполняем переход к следующему знаку значения
			continue;
		}
		// Получаем положение конца имени в обращении к значению
		const size_t position = value.find(((this->_settings.references == reference_t::SHELL) ? '}' : ')'), i + 2);
		/**
		 * Если конец имени в обращении к значению не обнаружен
		 */
		if(position == string_view::npos){
			// Запоминаем код ошибки разбора
			this->_error = error_t::UNKNOWN_REFERENCE;
			// Выводим отрицательный результат выполнения операции
			return false;
		}
		// Получаем длину знаков, закрывающих обращение к значению
		const size_t closing = ((this->_settings.references == reference_t::SHELL) ? 1 : 2);
		/**
		 * Если обращение по образцу configparser не закрыто признаком подстановки
		 */
		if((closing == 2) && (((position + 1) >= value.length()) || (value[position + 1] != 's'))){
			// Запоминаем код ошибки разбора
			this->_error = error_t::UNKNOWN_REFERENCE;
			// Выводим отрицательный результат выполнения операции
			return false;
		}
		// Получаем имя, к которому обращено значение
		string_view name = value.substr(i + 2, position - (i + 2));
		// Порядковый номер раздела, в котором ищется значение
		uint32_t target = section;
		/**
		 * Если обращение несёт имя раздела
		 *
		 * @note Запись «раздел:имя» принята разбором языка Python для обращения к
		 *       значению чужого раздела; без имени раздела значение ищется в своём
		 */
		const size_t separator = name.find(':');
		/**
		 * Если разделитель имени раздела и имени свойства обнаружен
		 */
		if(separator != string_view::npos){
			// Получаем имя раздела, к значению которого обращено
			const string_view label = name.substr(0, separator);
			/**
			 * Если раздел с таким именем не обнаружен
			 */
			if(!this->search(label, "", target)){
				// Признак обнаружения раздела разбором имени на раздел с подразделом
				bool found = false;
				/**
				 * Выполняем перебор знаков имени раздела, к значению которого обращено
				 *
				 * @note Обращение к значению чужого подраздела записывается тем же
				 *       знаком-разделителем, каким подраздел отделяется в объявлении
				 *       раздела: «${remote.origin:url}». Места разреза перебираются слева
				 *       направо: знак этот вправе стоять и в имени раздела, и в имени
				 *       подраздела, а какое из них где кончается, знает лишь указатель
				 */
				for(size_t i = 1; ((i + 1) < label.length()) && !found; i++){
					/**
					 * Если очередной знак знаком-разделителем не является
					 */
					if(label[i] != this->_settings.reader.delimiter)
						// Выполняем переход к следующему знаку имени раздела
						continue;
					// Выполняем поиск раздела с подразделом по очередному разрезу имени
					found = this->search(label.substr(0, i), label.substr(i + 1), target);
				}
				/**
				 * Если раздел не обнаружен и с разбором имени на раздел с подразделом
				 */
				if(!found){
					// Запоминаем код ошибки разбора
					this->_error = error_t::UNKNOWN_REFERENCE;
					// Выводим отрицательный результат выполнения операции
					return false;
				}
			}
			// Получаем имя свойства без имени раздела
			name = name.substr(separator + 1);
		}
		// Выполняем поиск свойства, к которому обращено значение
		const auto j = this->_properties.find(this->label(target, name));
		/**
		 * Если свойство, к которому обращено значение, не обнаружено
		 */
		if((j == this->_properties.end()) || j->second.empty()){
			// Запоминаем код ошибки разбора
			this->_error = error_t::UNKNOWN_REFERENCE;
			// Выводим отрицательный результат выполнения операции
			return false;
		}
		// Собираем обозначение свойства, к которому обращено значение
		string label = this->label(target, name);
		/**
		 * Выполняем перебор всех свойств, разрешаемых в настоящее время
		 */
		for(auto & item : stack){
			/**
			 * Если свойство, к которому обращено значение, уже разрешается
			 */
			if(item.compare(label) == 0){
				// Запоминаем код ошибки разбора
				this->_error = error_t::RECURSIVE_REFERENCE;
				// Выводим отрицательный результат выполнения операции
				return false;
			}
		}
		// Выполняем добавление свойства к перечню разрешаемых
		stack.push_back(std::move(label));
		/**
		 * Получаем порядковый номер записи, к значению которой обращено
		 *
		 * @note Объявление выбирается по тому же правилу, что и выдача значения:
		 *       иначе обращение к свойству давало бы не то значение, какое выдаёт
		 *       поиск по его имени, - при обращении с повторами по последнему
		 *       объявлению «${x}» разрешалось бы первым, а «get(x)» отдавал последнее
		 */
		const uint32_t source = ((this->_settings.reader.duplicates == duplicate_t::LAST) ? j->second.back() : j->second.front());
		/**
		 * Если разрешение значения, к которому обращено, выполнить не удалось
		 */
		if(!this->expand(this->get(this->_records.at(source).value), target, stack, budget, result))
			// Выводим отрицательный результат выполнения операции
			return false;
		// Выполняем изъятие свойства из перечня разрешаемых
		stack.pop_back();
		// Выполняем переход к знаку за обращением к значению
		i = (position + closing - 1);
	}
	// Выводим результат выполнения операции
	return true;
}
/**
 * @brief Метод подстановки обращений к значениям других свойств
 *
 * @param strict признак прекращения подстановки неразрешимым обращением
 * @return       результат выполнения операции
 *
 */
bool awh::codec::ini::Document::resolve(const bool strict) noexcept {
	/**
	 * Если подстановка обращений настройками не задана
	 */
	if(this->_settings.references == reference_t::NONE)
		// Выводим положительный результат выполнения операции
		return true;
	// Выполняем сброс признака неразрешимого обращения
	this->_dangling = false;
	// Остаток допустимого объёма подстановки в байтах
	uint64_t budget = this->_settings.maxExpansion;
	/**
	 * Перечень разрешённых значений свойств
	 *
	 * @note Разрешённые значения собираются отдельно и переносятся в хранилище
	 *       разом: разрешение читает значения из того же хранилища, и дописывание
	 *       в него по ходу обесценивало бы читаемое
	 */
	vector <pair <uint32_t, string>> resolved;
	/**
	 * Выполняем перебор всех записей разобранного текста
	 */
	for(size_t i = 0; i < this->_records.size(); i++){
		/**
		 * Если запись свойством не является
		 */
		if(this->_records.at(i).kind != kind_t::PROPERTY)
			// Выполняем переход к следующей записи
			continue;
		/**
		 * Получаем значение очередного свойства до подстановки
		 *
		 * @note Берётся именно исходное значение, а не разрешённое прежде: подстановка
		 *       выполняется заново при всякой правке дерева, и разрешение разрешённого
		 *       подставляло бы дважды там, где подставленное само несёт знак обращения
		 */
		const string_view value = this->get(this->_records.at(i).raw);
		// Знак, которым начинается обращение к значению
		const char letter = ((this->_settings.references == reference_t::SHELL) ? '$' : '%');
		/**
		 * Если значение свойства обращений не несёт
		 */
		if(value.find(letter) == string_view::npos)
			// Выполняем переход к следующей записи
			continue;
		// Запоминаем, что дерево несёт обращения к значениям
		this->_referenced = true;
		// Разрешённое значение очередного свойства
		string result;
		// Перечень свойств, разрешаемых в настоящее время
		vector <string> stack;
		// Выполняем добавление разрешаемого свойства к перечню
		stack.push_back(this->label(this->_records.at(i).section, this->get(this->_records.at(i).key)));
		/**
		 * Если разрешение значения свойства выполнить не удалось
		 */
		if(!this->expand(value, this->_records.at(i).section, stack, budget, result)){
			/**
			 * Если неразрешимое обращение подстановку прекращает
			 */
			if(strict)
				// Выводим отрицательный результат выполнения операции
				return false;
			/**
			 * Выполняем сброс кода ошибки неразрешимого обращения
			 *
			 * @details Пересчёт после правки снисходителен: правка вправе оставить дерево
			 * в состоянии, где обращение разрешить не по чему - источник удалён либо ещё
			 * не заведён, - и объявлять правку неудавшейся за это нельзя. Значение
			 * остаётся таким, каким записано, и разрешится само, едва источник появится
			 */
			this->_error = error_t::NONE;
			// Запоминаем, что дерево несёт неразрешимое обращение
			this->_dangling = true;
			// Запоминаем значение свойства в виде, в каком оно записано
			result.assign(value);
		}
		// Выполняем добавление разрешённого значения к перечню
		resolved.emplace_back(static_cast <uint32_t> (i), std::move(result));
	}
	/**
	 * Выполняем перебор всех разрешённых значений свойств
	 */
	for(auto & item : resolved){
		// Выполняем перенос разрешённого значения в хранилище знаков
		this->_records.at(item.first).value = this->add(item.second);
		/**
		 * Если хранилище знаков предел разрядности отрезка превысило
		 */
		if(this->_error == error_t::OVERFLOW_LIMIT)
			// Выводим отрицательный результат выполнения операции
			return false;
	}
	// Выводим положительный результат выполнения операции
	return true;
}
/**
 * @brief Метод пересчёта подстановки обращений после правки дерева
 *
 * @return результат выполнения операции
 *
 */
bool awh::codec::ini::Document::substitute() noexcept {
	/**
	 * Если дерево обращений к значениям не несёт
	 *
	 * @note Пересчёт обходится перебором всех записей, и дереву без обращений платить
	 *       за него незачем: подставлять там нечего
	 */
	if(!this->_referenced || (this->_settings.references == reference_t::NONE))
		// Выводим положительный результат выполнения операции
		return true;
	// Запоминаем, что подстановка обращений устарела
	this->_stale = true;
	// Выводим положительный результат выполнения операции
	return true;
}
/**
 * @brief Метод пересчёта устаревшей подстановки перед чтением значения
 *
 */
void awh::codec::ini::Document::refresh() const noexcept {
	/**
	 * Если подстановка обращений не устарела
	 */
	if(!this->_stale)
		// Выходим из пересчёта подстановки обращений
		return;
	// Выполняем сброс признака устаревшей подстановки
	this->_stale = false;
	/**
	 * Выполняем снисходительный пересчёт подстановки обращений
	 *
	 * @note Постоянность чтения пересчётом не нарушается: он приводит разрешённые
	 *       значения в соответствие с записанными, смысла дерева не меняя, - и ради
	 *       этого одного здесь снимается постоянность
	 */
	const_cast <Document *> (this)->resolve(false);
}
/**
 * @brief Метод получения текущих настроек дерева
 *
 * @return текущие настройки дерева настроек
 *
 */
const awh::codec::ini::Document::settings_t & awh::codec::ini::Document::settings() const noexcept {
	// Выводим текущие настройки дерева настроек
	return this->_settings;
}
/**
 * @brief Метод установки настроек дерева
 *
 * @param settings настройки дерева настроек
 *
 */
void awh::codec::ini::Document::settings(const settings_t & settings) noexcept {
	// Запоминаем настройки дерева настроек
	this->_settings = settings;
	/**
	 * Выполняем перестроение указателей поиска
	 *
	 * @note Указатели собраны по свёртке имён, а свёртка зависит от учёта регистра:
	 *       смена настроек без перестроения обращала бы поиск в неверный - имена в
	 *       указателе свёрнуты по-старому, а искомое сворачивается по-новому
	 *
	 * @warning Подстановка обращений заново не выполняется: разрешены они однажды,
	 *          при разборе, и смена настроек значений уже не меняет
	 */
	this->reindex();
}
/**
 * @brief Метод разбора текста настроек
 *
 * @param text разбираемый текст настроек
 * @return     результат выполнения операции
 *
 */
bool awh::codec::ini::Document::parse(const string_view text) noexcept {
	// Выполняем освобождение дерева настроек
	this->clear();
	// Получаем настройки разбора текста настроек
	reader_t::settings_t settings = this->_settings.reader;
	/**
	 * Устанавливаем выдачу примечаний отдельным событием
	 *
	 * @note Примечания и пустые строки читаются всегда: дерево держит оформление
	 *       текста, и отказ от них лишил бы его возможности переписать файл
	 *       настроек, не обеднив его
	 */
	settings.emitComments = true;
	// Устанавливаем выдачу пустых строк отдельным событием
	settings.emitBlanks = true;
	// Объект потокового чтения текста настроек
	reader_t reader(settings);
	// Выполняем заведение раздела без имени
	this->_sections.emplace_back();
	// Порядковый номер раздела, которому принадлежат разбираемые записи
	uint32_t current = 0;
	/**
	 * Если передачу разбираемого текста выполнить не удалось
	 */
	if(!reader.feed(text)){
		// Запоминаем код ошибки разбора
		this->_error = reader.error();
		// Запоминаем положение обнаруженной ошибки
		this->_errorLocation = reader.errorLocation();
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	/**
	 * Выполняем перебор всех событий разбора
	 */
	while(reader.next()){
		// Собираемая запись разобранного текста
		record_t record;
		/**
		 * Определяем вид текущего события разбора
		 */
		switch(static_cast <uint8_t> (reader.event())){
			// Если событием является объявление раздела
			case static_cast <uint8_t> (event_t::SECTION): {
				// Собираем ключ указателя разделов
				const string label = this->label(reader.section().section, reader.section().subsection);
				// Выполняем поиск раздела в указателе разделов
				const auto i = this->_index.find(label);
				/**
				 * Если раздел с таким именем уже объявлен
				 *
				 * @note Повторное объявление раздела заводит не новый раздел, а
				 *       продолжение прежнего: свойства обоих объявлений принадлежат
				 *       одному разделу, и такова работа всех наречий
				 */
				if(i != this->_index.end())
					// Запоминаем порядковый номер найденного раздела
					current = i->second;
				/**
				 * Если раздел объявляется впервые
				 */
				else {
					// Собираемый раздел разобранного текста
					section_t section;
					// Запоминаем место имени раздела в хранилище знаков
					section.name = this->add(reader.section().section);
					// Запоминаем место имени подраздела в хранилище знаков
					section.subsection = this->add(reader.section().subsection);
					// Запоминаем признак объявления раздела в тексте настроек
					section.declared = true;
					// Запоминаем порядковый номер объявленного раздела
					current = static_cast <uint32_t> (this->_sections.size());
					// Выполняем добавление раздела к перечню разделов
					this->_sections.push_back(section);
					// Выполняем добавление раздела к указателю разделов
					this->_index.emplace(label, current);
				}
				// Запоминаем вид собираемой записи
				record.kind = kind_t::SECTION;
			} break;
			// Если событием является свойство со значением
			case static_cast <uint8_t> (event_t::PROPERTY): {
				// Запоминаем вид собираемой записи
				record.kind = kind_t::PROPERTY;
				// Запоминаем место имени свойства в хранилище знаков
				record.key = this->add(reader.key());
				// Запоминаем место значения свойства в хранилище знаков
				record.value = this->add(reader.text());
				/**
				 * Если хранилище знаков предел разрядности отрезка превысило
				 */
				if(this->_error == error_t::OVERFLOW_LIMIT)
					// Выводим отрицательный результат выполнения операции
					return false;
				// Запоминаем место значения свойства до подстановки обращений
				record.raw = record.value;
				// Запоминаем признак заключения значения в кавычки
				record.quoted = reader.property().quoted;
				// Запоминаем признак свойства без разделителя и значения
				record.valueless = reader.property().valueless;
				// Запоминаем признак свойства, добавляющего значение к перечню
				record.append = reader.property().append;
			} break;
			// Если событием является примечание
			case static_cast <uint8_t> (event_t::COMMENT): {
				// Запоминаем вид собираемой записи
				record.kind = kind_t::COMMENT;
				/**
				 * Запоминаем место содержимого примечания в хранилище знаков
				 *
				 * @note Содержимое примечания хранится на месте значения свойства:
				 *       заводить ради него отдельное поле незачем, а запись эта
				 *       свойством не является и значения не несёт
				 */
				record.value = this->add(reader.text());
				/**
				 * Если хранилище знаков предел разрядности отрезка превысило
				 */
				if(this->_error == error_t::OVERFLOW_LIMIT)
					// Выводим отрицательный результат выполнения операции
					return false;
				// Запоминаем знак, которым примечание начато
				record.marker = reader.comment().marker;
				// Запоминаем расположение примечания в тексте настроек
				record.placement = reader.comment().placement;
			} break;
			// Если событием является пустая строка
			case static_cast <uint8_t> (event_t::BLANK):
				// Запоминаем вид собираемой записи
				record.kind = kind_t::BLANK;
			break;
			// Если событие разбора обработки не требует
			default: continue;
		}
		// Запоминаем порядковый номер раздела, которому запись принадлежит
		record.section = current;
		// Выполняем добавление записи к перечню записей
		this->_records.push_back(record);
	}
	/**
	 * Если разбор прекращён ошибкой
	 */
	if(reader.state() == state_t::FAILED){
		// Получаем код ошибки разбора
		const error_t error = reader.error();
		// Получаем положение обнаруженной ошибки
		const location_t location = reader.errorLocation();
		/**
		 * Выполняем освобождение дерева настроек
		 *
		 * @note Дерево при отказе разбора освобождается намеренно: половина
		 *       разобранного текста настроек хуже его отсутствия, поскольку
		 *       читается без признака своей неполноты
		 */
		this->clear();
		// Запоминаем код ошибки разбора
		this->_error = error;
		// Запоминаем положение обнаруженной ошибки
		this->_errorLocation = location;
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	// Выполняем перестроение указателей поиска
	this->reindex();
	/**
	 * Если подстановку обращений к значениям выполнить не удалось
	 */
	if(!this->resolve()){
		// Получаем код ошибки подстановки обращений
		const error_t error = this->_error;
		// Выполняем освобождение дерева настроек
		this->clear();
		// Запоминаем код ошибки подстановки обращений
		this->_error = error;
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	// Выводим положительный результат выполнения операции
	return true;
}
/**
 * @brief Метод разбора текста настроек с заданными настройками
 *
 * @param text     разбираемый текст настроек
 * @param settings настройки дерева настроек
 * @return         результат выполнения операции
 *
 */
bool awh::codec::ini::Document::parse(const string_view text, const settings_t & settings) noexcept {
	// Запоминаем настройки дерева настроек
	this->_settings = settings;
	// Выполняем разбор текста настроек
	return this->parse(text);
}
/**
 * @brief Метод получения кода ошибки разбора
 *
 * @return код ошибки последней операции разбора
 *
 */
awh::codec::ini::error_t awh::codec::ini::Document::error() const noexcept {
	// Выводим код ошибки последней операции разбора
	return this->_error;
}
/**
 * @brief Метод получения места обнаружения ошибки
 *
 * @return положение обнаруженной ошибки в исходном тексте
 *
 */
const awh::codec::ini::location_t & awh::codec::ini::Document::errorLocation() const noexcept {
	// Выводим положение обнаруженной ошибки в исходном тексте
	return this->_errorLocation;
}
/**
 * @brief Метод получения перечня объявленных разделов
 *
 * @return перечень объявленных разделов текста настроек
 *
 */
vector <awh::codec::ini::name_t> awh::codec::ini::Document::sections() const noexcept {
	// Собираемый перечень объявленных разделов
	vector <name_t> result;
	/**
	 * Выполняем перебор всех разделов разобранного текста
	 */
	for(size_t i = 0; i < this->_sections.size(); i++){
		/**
		 * Если раздел в тексте настроек не объявлен
		 */
		if(!this->_sections.at(i).declared)
			// Выполняем переход к следующему разделу
			continue;
		// Собираемое имя объявленного раздела
		name_t name;
		// Запоминаем имя объявленного раздела
		name.section = this->get(this->_sections.at(i).name);
		// Запоминаем имя объявленного подраздела
		name.subsection = this->get(this->_sections.at(i).subsection);
		// Выполняем добавление имени к собираемому перечню
		result.push_back(name);
	}
	// Выводим собранный перечень объявленных разделов
	return result;
}
/**
 * @brief Метод проверки наличия раздела
 *
 * @param section    имя искомого раздела
 * @param subsection имя искомого подраздела
 * @return           результат проверки
 *
 */
bool awh::codec::ini::Document::section(const string_view section, const string_view subsection) const noexcept {
	// Порядковый номер найденного раздела
	uint32_t index = 0;
	/**
	 * Если искомым является раздел без имени
	 */
	if(section.empty() && subsection.empty())
		// Выводим отрицательный результат проверки наличия раздела
		return false;
	// Выводим результат поиска раздела по имени
	return this->search(section, subsection, index);
}
/**
 * @brief Метод получения перечня имён свойств раздела
 *
 * @param section    имя раздела
 * @param subsection имя подраздела
 * @return           перечень имён свойств раздела
 *
 */
vector <string_view> awh::codec::ini::Document::keys(const string_view section, const string_view subsection) const noexcept {
	// Собираемый перечень имён свойств раздела
	vector <string_view> result;
	// Порядковый номер найденного раздела
	uint32_t index = 0;
	/**
	 * Если раздел с таким именем не обнаружен
	 */
	if(!this->search(section, subsection, index))
		// Выводим собранный перечень имён свойств
		return result;
	/**
	 * Если порядок первых объявлений раздела не собран
	 */
	if(index >= this->_order.size())
		// Выводим собранный перечень имён свойств
		return result;
	// Выполняем выделение памяти под собираемый перечень имён
	result.reserve(this->_order.at(index).size());
	/**
	 * Выполняем перебор первых объявлений свойств раздела
	 *
	 * @note Перечень собран указателями при перестроении и повторов не несёт:
	 *       свойство, объявленное несколько раз, лежит в нём однажды и в том
	 *       месте, где объявлено впервые
	 */
	for(const uint32_t item : this->_order.at(index)){
		/**
		 * Если запись свойства удалена
		 */
		if(this->_records.at(item).kind != kind_t::PROPERTY)
			// Выполняем переход к следующему объявлению
			continue;
		// Выполняем добавление имени свойства к собираемому перечню
		result.push_back(this->get(this->_records.at(item).key));
	}
	// Выводим собранный перечень имён свойств раздела
	return result;
}
/**
 * @brief Метод проверки наличия свойства
 *
 * @param key        имя искомого свойства
 * @param section    имя раздела
 * @param subsection имя подраздела
 * @return           результат проверки
 *
 */
bool awh::codec::ini::Document::has(const string_view key, const string_view section, const string_view subsection) const noexcept {
	// Выводим результат проверки наличия свойства
	return (this->locate(key, section, subsection) != nullptr);
}
/**
 * @brief Метод получения значения свойства
 *
 * @param key        имя искомого свойства
 * @param section    имя раздела
 * @param subsection имя подраздела
 * @return           значение найденного свойства либо пустая последовательность
 *
 */
string_view awh::codec::ini::Document::get(const string_view key, const string_view section, const string_view subsection) const noexcept {
	// Выполняем пересчёт подстановки обращений, устаревшей после правки
	this->refresh();
	// Выполняем поиск объявлений свойства
	const vector <uint32_t> * records = this->locate(key, section, subsection);
	/**
	 * Если свойство с таким именем не обнаружено
	 */
	if(records == nullptr)
		// Выводим пустую последовательность знаков
		return string_view();
	/**
	 * Выводим значение последнего объявления свойства
	 *
	 * @note Выбор объявления задан настройкой обращения с повторами: наречия
	 *       расходятся здесь между собою, и держаться одного было бы неверно
	 */
	if(this->_settings.reader.duplicates == duplicate_t::LAST)
		// Выводим значение последнего объявления свойства
		return this->get(this->_records.at(records->back()).value);
	// Выводим значение первого объявления свойства
	return this->get(this->_records.at(records->front()).value);
}
/**
 * @brief Метод получения перечня значений свойства
 *
 * @param key        имя искомого свойства
 * @param section    имя раздела
 * @param subsection имя подраздела
 * @return           перечень значений найденного свойства
 *
 */
vector <string_view> awh::codec::ini::Document::values(const string_view key, const string_view section, const string_view subsection) const noexcept {
	// Выполняем пересчёт подстановки обращений, устаревшей после правки
	this->refresh();
	// Собираемый перечень значений свойства
	vector <string_view> result;
	// Порядковый номер найденного раздела
	uint32_t index = 0;
	/**
	 * Если раздел с таким именем не обнаружен
	 */
	if(!this->search(section, subsection, index))
		// Выводим собранный перечень значений свойства
		return result;
	// Выполняем поиск свойства в указателе свойств
	const auto i = this->_properties.find(this->label(index, key));
	/**
	 * Если свойство с таким именем не обнаружено
	 */
	if(i == this->_properties.end())
		// Выводим собранный перечень значений свойства
		return result;
	/**
	 * Выполняем перебор всех объявлений свойства
	 */
	for(auto & item : i->second)
		// Выполняем добавление значения объявления к собираемому перечню
		result.push_back(this->get(this->_records.at(item).value));
	// Выводим собранный перечень значений свойства
	return result;
}
/**
 * @brief Метод объявления раздела
 *
 * @param section    имя объявляемого раздела
 * @param subsection имя объявляемого подраздела
 * @return           результат выполнения операции
 *
 */
bool awh::codec::ini::Document::create(const string_view section, const string_view subsection) noexcept {
	/**
	 * Выполняем сброс кода ошибки последней операции
	 *
	 * @note Код держится от последней операции, а не от последней неудачной: иначе
	 *       отказ, случившийся когда-то прежде, приписывался бы удачной правке
	 */
	this->_error = error_t::NONE;
	/**
	 * Если имя объявляемого раздела недопустимо
	 */
	if(!this->acceptable(section, true))
		// Выводим отрицательный результат выполнения операции
		return false;
	/**
	 * Если имя подраздела задано
	 */
	if(!subsection.empty()){
		/**
		 * Если построение имени подраздела настройками не задано
		 *
		 * @note Записать такой подраздел нечем: наречие, подразделов не признающее,
		 *       имени для него не имеет, и запись собранного дерева отказала бы
		 */
		if(this->_settings.reader.subsections == subsection_t::NONE){
			// Запоминаем код ошибки правки
			this->_error = error_t::INVALID_SUBSECTION;
			// Выводим отрицательный результат выполнения операции
			return false;
		}
		/**
		 * Если имя подраздела недопустимо
		 */
		if(!this->acceptable(subsection, true, false))
			// Выводим отрицательный результат выполнения операции
			return false;
		/**
		 * Если подраздел отделяется знаком-разделителем
		 */
		if(this->_settings.reader.subsections == subsection_t::DELIMITED){
			/**
			 * Глубина вложенности объявляемого подраздела
			 *
			 * @note Один разделитель отделяет подраздел от раздела, прочие стоят внутри
			 *       его имени: разбор считает их все и потому обязан насчитать столько же
			 */
			uint32_t depth = 1;
			/**
			 * Выполняем перебор знаков имени подраздела
			 */
			for(size_t i = 0; i < subsection.length(); i++){
				/**
				 * Если очередным знаком является знак-разделитель
				 */
				if(subsection[i] == this->_settings.reader.delimiter)
					// Выполняем увеличение глубины вложенности подраздела
					depth++;
			}
			/**
			 * Если глубина вложенности подразделов предел настроек превышает
			 */
			if(depth >= this->_settings.reader.maxDepth){
				// Запоминаем код ошибки правки
				this->_error = error_t::DEPTH_EXCEEDED;
				// Выводим отрицательный результат выполнения операции
				return false;
			}
		}
	}
	// Порядковый номер найденного раздела
	uint32_t index = 0;
	/**
	 * Если раздел с таким именем уже объявлен
	 */
	if(this->search(section, subsection, index))
		// Выводим положительный результат выполнения операции
		return true;
	/**
	 * Если перечень разделов пуст
	 */
	if(this->_sections.empty())
		// Выполняем заведение раздела без имени
		this->_sections.emplace_back();
	// Собираемый раздел разобранного текста
	section_t record;
	// Запоминаем место имени раздела в хранилище знаков
	record.name = this->add(section);
	// Запоминаем место имени подраздела в хранилище знаков
	record.subsection = this->add(subsection);
	/**
	 * Если хранилище знаков предел разрядности отрезка превысило
	 */
	if(this->_error == error_t::OVERFLOW_LIMIT)
		// Выводим отрицательный результат выполнения операции
		return false;
	// Запоминаем признак объявления раздела в тексте настроек
	record.declared = true;
	// Запоминаем порядковый номер объявляемого раздела
	index = static_cast <uint32_t> (this->_sections.size());
	// Выполняем добавление раздела к перечню разделов
	this->_sections.push_back(record);
	// Собираемая запись объявления раздела
	record_t line;
	// Запоминаем вид собираемой записи
	line.kind = kind_t::SECTION;
	// Запоминаем порядковый номер раздела, которому запись принадлежит
	line.section = index;
	/**
	 * Если записи разобранного текста уже есть
	 *
	 * @note Пустая строка перед объявлением раздела ставится ради читаемости:
	 *       разделы, слипшиеся друг с другом, человек читает с трудом
	 */
	if(!this->_records.empty() && (this->kind() != kind_t::BLANK)){
		// Собираемая запись пустой строки
		record_t blank;
		// Запоминаем вид собираемой записи
		blank.kind = kind_t::BLANK;
		// Запоминаем порядковый номер раздела, которому запись принадлежит
		blank.section = index;
		// Выполняем добавление записи к перечню записей
		this->_records.push_back(blank);
	}
	// Выполняем добавление записи к перечню записей
	this->_records.push_back(line);
	/**
	 * Выполняем приращение указателей поиска
	 *
	 * @note Записи раздела встают в конец перечня, и порядковые номера собранных
	 *       ранее записей от того не сдвигаются: перестраивать указатели, на них
	 *       ссылающиеся, незачем
	 */
	// Выполняем выделение памяти под порядок объявлений заведённых разделов
	this->_order.resize(this->_sections.size());
	// Выполняем выделение памяти под последние записи заведённых разделов
	this->_last.resize(this->_sections.size(), NO_RECORD);
	// Запоминаем последнюю запись объявленного раздела
	this->_last.at(index) = static_cast <uint32_t> (this->_records.size() - 1);
	// Выполняем добавление раздела к указателю разделов
	this->_index.emplace(this->label(section, subsection), index);
	// Выводим положительный результат выполнения операции
	return true;
}
/**
 * @brief Метод установки значения свойства
 *
 * @param key        имя устанавливаемого свойства
 * @param value      устанавливаемое значение свойства
 * @param section    имя раздела
 * @param subsection имя подраздела
 * @return           результат выполнения операции
 *
 */
bool awh::codec::ini::Document::set(const string_view key, const string_view value, const string_view section, const string_view subsection) noexcept {
	/**
	 * Выполняем сброс кода ошибки последней операции
	 *
	 * @note Код держится от последней операции, а не от последней неудачной: иначе
	 *       отказ, случившийся когда-то прежде, приписывался бы удачной правке
	 */
	this->_error = error_t::NONE;
	/**
	 * Если имя устанавливаемого свойства недопустимо
	 */
	if(!this->acceptable(key, false))
		// Выводим отрицательный результат выполнения операции
		return false;
	// Порядковый номер найденного раздела
	uint32_t index = 0;
	/**
	 * Если значение устанавливается разделу без имени, а дерево пусто
	 *
	 * @note Раздел без имени объявления не требует и потому через объявление
	 *       заведён быть не может: сборка дерева с нуля вызовами установки значения
	 *       без этого была бы невозможна там, где имя раздела не задано
	 */
	if(section.empty() && subsection.empty() && this->_sections.empty()){
		// Выполняем заведение раздела без имени
		this->_sections.emplace_back();
		// Выполняем выделение памяти под порядок объявлений раздела без имени
		this->_order.resize(this->_sections.size());
		// Выполняем выделение памяти под последнюю запись раздела без имени
		this->_last.resize(this->_sections.size(), NO_RECORD);
	}
	/**
	 * Если раздел с таким именем не обнаружен
	 */
	if(!this->search(section, subsection, index)){
		/**
		 * Если объявление раздела выполнить не удалось
		 */
		if(!this->create(section, subsection))
			// Выводим отрицательный результат выполнения операции
			return false;
		/**
		 * Если объявленный раздел обнаружить не удалось
		 */
		if(!this->search(section, subsection, index))
			// Выводим отрицательный результат выполнения операции
			return false;
	}
	// Выполняем поиск свойства в указателе свойств
	const auto i = this->_properties.find(this->label(index, key));
	/**
	 * Если свойство с таким именем уже объявлено
	 */
	if((i != this->_properties.end()) && !i->second.empty()){
		/**
		 * Получаем порядковый номер правимой записи
		 *
		 * @note Правится то объявление, которое выдал бы поиск значения: прочие
		 *       остаются нетронутыми, поскольку задают перечень значений
		 */
		const uint32_t target = ((this->_settings.reader.duplicates == duplicate_t::LAST) ? i->second.back() : i->second.front());
		// Запоминаем место нового значения свойства в хранилище знаков
		this->_records.at(target).value = this->add(value);
		/**
		 * Если хранилище знаков предел разрядности отрезка превысило
		 */
		if(this->_error == error_t::OVERFLOW_LIMIT)
			// Выводим отрицательный результат выполнения операции
			return false;
		/**
		 * Запоминаем место нового значения свойства до подстановки обращений
		 *
		 * @note Место до подстановки держится ради обратной записи: в файл настроек
		 *       уходит именно оно, и обращение попадает туда как записано. Само же
		 *       значение разрешается заново по завершении правки - вместе со всеми
		 *       прочими, на которые правка могла повлиять
		 */
		this->_records.at(target).raw = this->_records.at(target).value;
		/**
		 * Снимаем признак свойства, записанного без разделителя и значения
		 *
		 * @note Свойству дано значение, и записывать его без разделителя нельзя:
		 *       значение при обратной записи было бы потеряно
		 */
		this->_records.at(target).valueless = false;
		/**
		 * Снимаем признак свойства, записанного добавлением к перечню значений
		 *
		 * @note Установка значения заменяет прежнее, а не добавляет к перечню:
		 *       записать его добавлением значило бы при обратном чтении получить
		 *       перечень там, где его не задавали
		 */
		this->_records.at(target).append = false;
		// Снимаем признак значения, заключённого в кавычки
		this->_records.at(target).quoted = false;
		/**
		 * Если установленное значение несёт обращение к другому значению
		 *
		 * @note Признак взводится здесь, а не одной лишь подстановкой: обращение,
		 *       заведённое правкой, подстановка иначе не разрешила бы вовсе - дерево
		 *       считалось бы обращений не несущим
		 */
		if((this->_settings.references != reference_t::NONE) &&
		   (value.find((this->_settings.references == reference_t::SHELL) ? '$' : '%') != string_view::npos))
			// Запоминаем, что дерево несёт обращения к значениям
			this->_referenced = true;
		// Выводим результат пересчёта подстановки обращений
		return this->substitute();
	}
	/**
	 * Положение вставки записи в перечень записей
	 *
	 * @note Место вставки берётся из указателя последних записей: перебор всех
	 *       записей ради него обращал бы сборку дерева вызовами этого метода в
	 *       квадратичную
	 */
	size_t position = ((index < this->_last.size()) && (this->_last.at(index) != NO_RECORD) ? static_cast <size_t> (this->_last.at(index) + 1) : this->_records.size());
	/**
	 * Выполняем отступление от пустых строк в конце раздела
	 *
	 * @note Пустая строка в конце раздела отделяет его от следующего, и запись
	 *       свойства за ней оторвала бы свойство от своего раздела
	 */
	while((position > 0) && (this->_records.at(position - 1).kind == kind_t::BLANK) && (this->_records.at(position - 1).section == index))
		// Выполняем перенос места вставки перед пустой строкой
		position--;
	// Собираемая запись свойства со значением
	record_t record;
	// Запоминаем вид собираемой записи
	record.kind = kind_t::PROPERTY;
	// Запоминаем порядковый номер раздела, которому запись принадлежит
	record.section = index;
	// Запоминаем место имени свойства в хранилище знаков
	record.key = this->add(key);
	// Запоминаем место значения свойства в хранилище знаков
	record.value = this->add(value);
	/**
	 * Если хранилище знаков предел разрядности отрезка превысило
	 */
	if(this->_error == error_t::OVERFLOW_LIMIT)
		// Выводим отрицательный результат выполнения операции
		return false;
	// Запоминаем место значения свойства до подстановки обращений
	record.raw = record.value;
	/**
	 * Если запись встаёт в конец перечня записей
	 *
	 * @note Порядковые номера собранных ранее записей при этом не сдвигаются, и
	 *       указатели поиска достаточно нарастить. Ради этого случая всё и
	 *       затевалось: сборка дерева вызовами этого метода идёт именно так
	 */
	if((position == this->_records.size()) && (index < this->_last.size()) && (index < this->_order.size())){
		// Выполняем добавление записи к перечню записей
		this->_records.push_back(record);
		// Запоминаем порядковый номер добавленной записи
		const uint32_t target = static_cast <uint32_t> (this->_records.size() - 1);
		// Запоминаем запись последней записью своего раздела
		this->_last.at(index) = target;
		// Получаем перечень объявлений свойства в указателе свойств
		vector <uint32_t> & records = this->_properties[this->label(index, key)];
		/**
		 * Если свойство объявляется в разделе впервые
		 */
		if(records.empty() && (index < this->_order.size()))
			// Выполняем добавление записи к порядку первых объявлений раздела
			this->_order.at(index).push_back(target);
		// Выполняем добавление свойства к указателю свойств
		records.push_back(target);
		/**
		 * Если установленное значение несёт обращение к другому значению
		 *
		 * @note Признак взводится здесь, а не одной лишь подстановкой: обращение,
		 *       заведённое правкой, подстановка иначе не разрешила бы вовсе - дерево
		 *       считалось бы обращений не несущим
		 */
		if((this->_settings.references != reference_t::NONE) &&
		   (value.find((this->_settings.references == reference_t::SHELL) ? '$' : '%') != string_view::npos))
			// Запоминаем, что дерево несёт обращения к значениям
			this->_referenced = true;
		// Выводим результат пересчёта подстановки обращений
		return this->substitute();
	}
	// Выполняем вставку записи в перечень записей
	this->_records.insert(this->_records.begin() + static_cast <ptrdiff_t> (position), record);
	// Выполняем перестроение указателей поиска
	this->reindex();
	/**
	 * Если установленное значение несёт обращение к другому значению
	 *
	 * @note Признак взводится здесь, а не одной лишь подстановкой: обращение,
	 *       заведённое правкой, подстановка иначе не разрешила бы вовсе - дерево
	 *       считалось бы обращений не несущим
	 */
	if((this->_settings.references != reference_t::NONE) &&
	   (value.find((this->_settings.references == reference_t::SHELL) ? '$' : '%') != string_view::npos))
		// Запоминаем, что дерево несёт обращения к значениям
		this->_referenced = true;
	// Выводим результат пересчёта подстановки обращений
	return this->substitute();
}
/**
 * @brief Метод удаления свойства
 *
 * @param key        имя удаляемого свойства
 * @param section    имя раздела
 * @param subsection имя подраздела
 * @return           результат выполнения операции
 *
 */
bool awh::codec::ini::Document::erase(const string_view key, const string_view section, const string_view subsection) noexcept {
	/**
	 * Выполняем сброс кода ошибки последней операции
	 *
	 * @note Код держится от последней операции, а не от последней неудачной: иначе
	 *       отказ, случившийся когда-то прежде, приписывался бы удачной правке
	 */
	this->_error = error_t::NONE;
	// Порядковый номер найденного раздела
	uint32_t index = 0;
	/**
	 * Если раздел с таким именем не обнаружен
	 */
	if(!this->search(section, subsection, index))
		// Выводим отрицательный результат выполнения операции
		return false;
	// Выполняем поиск свойства в указателе свойств
	const auto i = this->_properties.find(this->label(index, key));
	/**
	 * Если свойство с таким именем не обнаружено
	 */
	if((i == this->_properties.end()) || i->second.empty())
		// Выводим отрицательный результат выполнения операции
		return false;
	/**
	 * Выполняем перебор всех объявлений свойства
	 */
	for(auto & item : i->second){
		// Выполняем удаление записи объявления свойства
		this->_records.at(item).kind = kind_t::NONE;
		/**
		 * Если за записью свойства следует примечание конца её строки
		 *
		 * @note Примечание это описывает удалённое свойство, и оставлять его в
		 *       тексте незачем: без своей строки оно повисло бы в чужой
		 */
		if(((item + 1) < this->_records.size()) && (this->_records.at(item + 1).kind == kind_t::COMMENT) &&
		   (this->_records.at(item + 1).placement == placement_t::TAIL))
			// Выполняем удаление записи примечания
			this->_records.at(item + 1).kind = kind_t::NONE;
	}
	/**
	 * Выполняем изъятие свойства из указателей поиска
	 *
	 * @note Удалённые записи из перечня не изымаются, а лишь помечаются, и
	 *       порядковые номера прочих записей от того не сдвигаются: перестраивать
	 *       указатели целиком незачем
	 */
	if(index < this->_order.size()){
		// Получаем перечень первых объявлений свойств раздела
		vector <uint32_t> & order = this->_order.at(index);
		// Получаем порядковый номер записи первого объявления свойства
		const uint32_t target = i->second.front();
		/**
		 * Выполняем перебор всех первых объявлений свойств раздела
		 */
		for(auto j = order.begin(); j != order.end(); ++j){
			/**
			 * Если объявление удаляемому свойству принадлежит
			 */
			if((* j) == target){
				// Выполняем изъятие объявления из порядка объявлений раздела
				order.erase(j);
				// Выходим из перебора объявлений свойств раздела
				break;
			}
		}
	}
	// Выполняем изъятие свойства из указателя свойств
	this->_properties.erase(i);
	/**
	 * Выводим результат пересчёта подстановки обращений
	 *
	 * @note Удалённое свойство могло быть источником обращения, и разрешённые прежде
	 *       значения без пересчёта остались бы подставленными по нему
	 */
	return this->substitute();
}
/**
 * @brief Метод удаления раздела
 *
 * @param section    имя удаляемого раздела
 * @param subsection имя удаляемого подраздела
 * @return           результат выполнения операции
 *
 */
bool awh::codec::ini::Document::remove(const string_view section, const string_view subsection) noexcept {
	/**
	 * Выполняем сброс кода ошибки последней операции
	 *
	 * @note Код держится от последней операции, а не от последней неудачной: иначе
	 *       отказ, случившийся когда-то прежде, приписывался бы удачной правке
	 */
	this->_error = error_t::NONE;
	/**
	 * Если имя удаляемого раздела пусто
	 */
	if(section.empty())
		// Выводим отрицательный результат выполнения операции
		return false;
	// Порядковый номер найденного раздела
	uint32_t index = 0;
	/**
	 * Если раздел с таким именем не обнаружен
	 */
	if(!this->search(section, subsection, index))
		// Выводим отрицательный результат выполнения операции
		return false;
	/**
	 * Выполняем перебор всех записей разобранного текста
	 */
	for(size_t i = 0; i < this->_records.size(); i++){
		/**
		 * Если запись удаляемому разделу принадлежит
		 */
		if(this->_records.at(i).section == index)
			// Выполняем удаление записи разобранного текста
			this->_records.at(i).kind = kind_t::NONE;
	}
	/**
	 * Снимаем признак объявления раздела в тексте настроек
	 *
	 * @note Сам раздел из перечня не изымается: на его порядковый номер ссылаются
	 *       записи, и сдвиг номеров обесценил бы их. Необъявленный раздел ни в
	 *       перечень разделов, ни в выдачу поиска не идёт
	 */
	this->_sections.at(index).declared = false;
	// Выполняем перестроение указателей поиска
	this->reindex();
	/**
	 * Выводим результат пересчёта подстановки обращений
	 *
	 * @note Удалённый раздел мог нести источники обращений, и разрешённые прежде
	 *       значения без пересчёта остались бы подставленными по ним
	 */
	return this->substitute();
}
/**
 * @brief Метод получения количества объявленных разделов
 *
 * @return количество объявленных разделов текста настроек
 *
 */
size_t awh::codec::ini::Document::size() const noexcept {
	// Количество объявленных разделов текста настроек
	size_t result = 0;
	/**
	 * Выполняем перебор всех разделов разобранного текста
	 */
	for(size_t i = 0; i < this->_sections.size(); i++){
		/**
		 * Если раздел в тексте настроек объявлен
		 */
		if(this->_sections.at(i).declared)
			// Выполняем увеличение количества объявленных разделов
			result++;
	}
	// Выводим количество объявленных разделов текста настроек
	return result;
}
/**
 * @brief Метод проверки дерева на отсутствие записей
 *
 * @return результат проверки
 *
 */
bool awh::codec::ini::Document::empty() const noexcept {
	/**
	 * Выполняем перебор всех записей разобранного текста
	 */
	for(size_t i = 0; i < this->_records.size(); i++){
		/**
		 * Если запись удалённой не является
		 */
		if(this->_records.at(i).kind != kind_t::NONE)
			// Выводим отрицательный результат проверки
			return false;
	}
	// Выводим положительный результат проверки
	return true;
}
/**
 * @brief Метод освобождения дерева настроек
 *
 */
void awh::codec::ini::Document::clear() noexcept {
	// Выполняем сброс кода ошибки разбора
	this->_error = error_t::NONE;
	// Выполняем сброс признака наличия обращений к значениям
	this->_referenced = false;
	// Выполняем сброс признака устаревшей подстановки обращений
	this->_stale = false;
	// Выполняем сброс признака неразрешимого обращения
	this->_dangling = false;
	// Выполняем сброс положения обнаруженной ошибки
	this->_errorLocation = location_t();
	// Выполняем очистку хранилища знаков
	this->_store.clear();
	// Выполняем очистку перечня записей разобранного текста
	this->_records.clear();
	// Выполняем очистку перечня разделов разобранного текста
	this->_sections.clear();
	// Выполняем очистку указателя разделов
	this->_index.clear();
	// Выполняем очистку указателя свойств
	this->_properties.clear();
	// Выполняем очистку порядка первых объявлений свойств
	this->_order.clear();
	// Выполняем очистку последних записей разделов
	this->_last.clear();
}
/**
 * @brief Метод записи дерева обратно в текст настроек
 *
 * @param settings настройки записи текста настроек
 * @return         собранный текст настроек
 *
 */
/**
 * @brief Метод получения настроек записи, отвечающих настройкам разбора
 *
 * @return настройки записи текста настроек
 *
 */
awh::codec::ini::Writer::Settings awh::codec::ini::Document::writing() const noexcept {
	// Собираемые настройки записи текста настроек
	writer_t::settings_t result;
	// Устанавливаем построение имени подраздела
	result.subsections = this->_settings.reader.subsections;
	// Устанавливаем знак, отделяющий имя подраздела при построении разделителем
	result.delimiter = this->_settings.reader.delimiter;
	// Устанавливаем признание примечания в конце строки читающим
	result.inlineComments = this->_settings.reader.inlineComments;
	// Устанавливаем знаки, признаваемые читающим началом примечания
	result.comments = this->_settings.reader.comments;
	// Устанавливаем запись управляющих последовательностей в значении
	result.escapes = this->_settings.reader.escapes;
	// Устанавливаем склеивание строк, продолженных обратной косой чертой, читающим
	result.continuations = this->_settings.reader.continuations;
	// Устанавливаем запись многострочного значения продолжением отступом
	result.indents = this->_settings.reader.indents;
	// Устанавливаем наибольшую допустимую длину имени раздела или свойства
	result.maxName = this->_settings.reader.maxName;
	/**
	 * Если знак решётки читающим признаётся, а точка с запятой нет
	 *
	 * @note Знак примечания берётся тот, который читающий признаёт: примечание,
	 *       записанное чужим знаком, досталось бы ему частью значения
	 */
	if(this->_settings.reader.comments == marker_t::HASH)
		// Устанавливаем знак начала примечания
		result.marker = '#';
	/**
	 * Если разделителем имени и значения читающим признаётся лишь двоеточие
	 */
	if(this->_settings.reader.separators == separator_t::COLON)
		// Устанавливаем знак, разделяющий имя свойства и его значение
		result.separator = ':';
	/**
	 * Устанавливаем обращение с ограждением значения кавычками
	 *
	 * @note Ограждение назначается лишь тогда, когда читающий кавычки снимает.
	 *       Наречию, считающему кавычки частью значения, ограждение противопоказано
	 *       вдвойне: записанные кавычки достались бы ему данными, а значение, уже
	 *       несущее кавычку, записать без управляющих последовательностей нечем
	 */
	result.quoting = ((this->_settings.reader.quotes == quote_t::STRIP) ? quoting_t::AUTO : quoting_t::NEVER);
	// Выводим собранные настройки записи
	return result;
}
/**
 * @brief Метод записи дерева обратно в текст настроек
 *
 * @return собранный текст настроек
 *
 */
string awh::codec::ini::Document::text() const noexcept {
	// Выполняем запись дерева настройками, отвечающими настройкам разбора
	return this->text(this->writing());
}
string awh::codec::ini::Document::text(const writer_t::settings_t & settings) const noexcept {
	// Выполняем сброс кода ошибки последней операции
	this->_error = error_t::NONE;
	// Выполняем пересчёт подстановки обращений, устаревшей после правки
	this->refresh();
	/**
	 * Если дерево несёт неразрешимое обращение
	 *
	 * @details Правка вправе удалить источник обращения, и чтение такое значение выдаёт
	 * в записанном виде. Записать же дерево целиком нельзя: текст с обращением в пустоту
	 * разбор отвергает по неизвестному обращению, и выдать его значило бы отдать
	 * заведомо негодный файл настроек тому, кто отказа не спросил
	 */
	if(this->_dangling){
		// Запоминаем код ошибки записи
		this->_error = error_t::UNKNOWN_REFERENCE;
		// Выводим пустой текст настроек
		return string();
	}
	// Получаем настройки записи текста настроек
	writer_t::settings_t options = settings;
	/**
	 * Снимаем расстановку пустых строк перед объявлениями разделов
	 *
	 * @note Расстановка эта взята из исходного текста и хранится записями пустых
	 *       строк: добавлять к ней свою значило бы наращивать пустые строки при
	 *       каждом обороте «чтение - запись»
	 */
	options.separated = false;
	// Объект записи текста настроек
	writer_t writer(options);
	// Признак успешной записи очередной записи дерева
	bool result = true;
	/**
	 * Выполняем перебор всех записей разобранного текста
	 */
	for(size_t i = 0; i < this->_records.size(); i++){
		/**
		 * Определяем вид очередной записи разобранного текста
		 */
		switch(static_cast <uint8_t> (this->_records.at(i).kind)){
			// Если записью является объявление раздела
			case static_cast <uint8_t> (kind_t::SECTION):
				// Выполняем запись объявления раздела
				result = writer.section(this->get(this->_sections.at(this->_records.at(i).section).name), this->get(this->_sections.at(this->_records.at(i).section).subsection));
			break;
			// Если записью является свойство со значением
			case static_cast <uint8_t> (kind_t::PROPERTY): {
				/**
				 * Если свойство записано без разделителя и значения
				 */
				if(this->_records.at(i).valueless)
					// Выполняем запись свойства без разделителя и значения
					result = writer.property(this->get(this->_records.at(i).key));
				else {
					/**
					 * Если значение было заключено в кавычки, а запись ставит их по надобности
					 *
					 * @note Кавычки берутся из исходного текста наравне со знаком примечания:
					 *       человек, писавший файл руками, поставил их сам, и снимать их при
					 *       перезаписи значило бы править то, о чём не просили
					 */
					if(this->_records.at(i).quoted && (writer.settings().quoting == quoting_t::AUTO)){
						// Получаем настройки записи текста настроек
						writer_t::settings_t current = writer.settings();
						// Устанавливаем ограждение значения кавычками
						current.quoting = quoting_t::ALWAYS;
						// Выполняем установку настроек записи текста настроек
						writer.settings(current);
						// Выполняем запись свойства со значением
						result = writer.property(this->get(this->_records.at(i).key), this->get(this->_records.at(i).raw), this->_records.at(i).append);
						// Возвращаем ограждение значения кавычками по надобности
						current.quoting = quoting_t::AUTO;
						// Выполняем установку настроек записи текста настроек
						writer.settings(current);
					// Выполняем запись свойства со значением
					} else result = writer.property(this->get(this->_records.at(i).key), this->get(this->_records.at(i).raw), this->_records.at(i).append);
				}
			} break;
			// Если записью является примечание
			case static_cast <uint8_t> (kind_t::COMMENT): {
				/**
				 * Если знак примечания записи с настройками записи расходится
				 *
				 * @note Знак берётся из исходного текста: человек, писавший файл
				 *       руками, выбрал его сам, и подменять его при перезаписи
				 *       значило бы править то, о чём не просили
				 */
				if(writer.settings().marker != this->_records.at(i).marker){
					// Получаем настройки записи текста настроек
					writer_t::settings_t current = writer.settings();
					// Запоминаем знак начала примечания записи
					current.marker = this->_records.at(i).marker;
					// Выполняем установку настроек записи текста настроек
					writer.settings(current);
				}
				/**
				 * Если примечание занимает строку целиком
				 */
				if(this->_records.at(i).placement == placement_t::OWN)
					// Выполняем запись примечания отдельной строкой
					result = writer.comment(this->get(this->_records.at(i).value));
				// Выполняем дописывание примечания к последней записанной строке
				else result = writer.trailing(this->get(this->_records.at(i).value));
			} break;
			// Если записью является пустая строка
			case static_cast <uint8_t> (kind_t::BLANK):
				// Выполняем запись пустой строки
				result = writer.blank();
			break;
		}
		/**
		 * Если запись очередной записи дерева выполнить не удалось
		 *
		 * @note Отказ писателя означает, что записать дерево без потерь нельзя:
		 *       значение несёт знак, огородить который настройки записи не
		 *       позволяют. Выдавать при этом обрубок текста опаснее, чем не выдать
		 *       ничего, - обрубок прочитается без нареканий и молча подменит данные
		 */
		if(!result){
			// Запоминаем код ошибки записи текста настроек
			this->_error = writer.error();
			// Выводим пустой текст настроек
			return string();
		}
	}
	// Выводим собранный текст настроек
	return writer.text();
}
/**
 * @brief Конструктор
 *
 */
awh::codec::ini::Document::Document() noexcept : _error(error_t::NONE), _referenced(false), _stale(false), _dangling(false) {}
/**
 * @brief Конструктор
 *
 * @param settings настройки дерева настроек
 *
 */
awh::codec::ini::Document::Document(const settings_t & settings) noexcept : _error(error_t::NONE), _referenced(false), _stale(false), _dangling(false), _settings(settings) {}
/**
 * @brief Деструктор
 *
 */
awh::codec::ini::Document::~Document() noexcept {
	// Выполняем освобождение дерева настроек
	this->clear();
}
/**
 * @brief Шаблон типа числа результата разбора
 *
 * @tparam T тип числа результата разбора
 *
 */
template <typename T>
/**
 * @brief Метод получения значения свойства числом
 *
 * @param result     ссылка на результат разбора
 * @param key        имя искомого свойства
 * @param section    имя раздела
 * @param subsection имя подраздела
 * @return           признак успешного разбора
 *
 */
bool awh::codec::ini::Document::value(T & result, const string_view key, const string_view section, const string_view subsection) const noexcept {
	// Выполняем поиск объявлений свойства
	const vector <uint32_t> * records = this->locate(key, section, subsection);
	/**
	 * Если свойство с таким именем не обнаружено
	 *
	 * @note Поиск выполняется однажды: прежде проверка наличия и получение значения
	 *       повторяли одну и ту же работу дважды, а стоила она половину обращения
	 */
	if(records == nullptr)
		// Выводим признак неудачного разбора
		return false;
	/**
	 * Получаем значение свойства по настройке обращения с повторами
	 */
	const string_view value = ((this->_settings.reader.duplicates == duplicate_t::LAST) ?
		this->get(this->_records.at(records->back()).value) :
		this->get(this->_records.at(records->front()).value)
	);
	// Выполняем разбор значения свойства числом
	return numeric(value, result);
}

/**
 * Выполняем порождение метода получения значения свойства числом для всех поддерживаемых типов
 */
template __AWH_SHARED_EXPORT__ bool awh::codec::ini::Document::value <bool> (bool &, const string_view, const string_view, const string_view) const noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::ini::Document::value <int8_t> (int8_t &, const string_view, const string_view, const string_view) const noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::ini::Document::value <uint8_t> (uint8_t &, const string_view, const string_view, const string_view) const noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::ini::Document::value <int16_t> (int16_t &, const string_view, const string_view, const string_view) const noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::ini::Document::value <uint16_t> (uint16_t &, const string_view, const string_view, const string_view) const noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::ini::Document::value <int32_t> (int32_t &, const string_view, const string_view, const string_view) const noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::ini::Document::value <uint32_t> (uint32_t &, const string_view, const string_view, const string_view) const noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::ini::Document::value <int64_t> (int64_t &, const string_view, const string_view, const string_view) const noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::ini::Document::value <uint64_t> (uint64_t &, const string_view, const string_view, const string_view) const noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::ini::Document::value <float> (float &, const string_view, const string_view, const string_view) const noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::ini::Document::value <double> (double &, const string_view, const string_view, const string_view) const noexcept;

/**
 * Возвращаем макросы, снятые в начале файла
 */
#include <sys/macro_pop.hpp>
