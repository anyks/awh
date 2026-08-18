/**
 * @file value.cpp
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
 * \~russian
 * @brief Исходный файл владеющего значения YAML
 *
 * \~english
 * @brief Source file of the owning value of YAML
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем стандартные заголовочные файлы
 */
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <limits>
#include <fstream>
#include <type_traits>

/**
 * Подключаем заголовочные файлы модуля
 */
#include <codec/yaml/value.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Используем пространство имён контейнера YAML
 */
using namespace awh::codec::yaml;

/**
 * @brief Внутренние помощники владеющего значения
 *
 */
namespace {
	/**
	 * @brief Шаблонная функция приведения дробного числа к затребованному виду
	 *
	 * @details Приведение это перенято у дерева документа дословно, а тем самым и у
	 *          контейнера JSON: расхождение владеющего значения с деревом, из какого оно
	 *          снято, дало бы разный итог одного и того же извлечения
	 *
	 * @tparam T     вид, к какому приводится число
	 * @param  value приводимое дробное число
	 * @return       приведённое число
	 *
	 */
	template <typename T>
	static T convert(const double value) noexcept {
		/**
		 * Если затребован дробный вид
		 */
		if(std::is_floating_point <T>::value)
			// Выводим приведённое число как оно есть
			return static_cast <T> (value);
		/**
		 * Если число не является числом вовсе
		 *
		 * @note Приведение `NaN` к целому есть неопределённое поведение при любом пределе:
		 *       выдаётся ноль, и выдаётся он успехом, - ровно так решено у дерева документа
		 */
		if(::isnan(value))
			// Выводим нулевое число
			return static_cast <T> (0);
		/**
		 * Если целая часть числа лежит ниже предела затребованного вида
		 */
		if(value <= static_cast <double> (std::numeric_limits <T>::lowest()))
			// Выводим нижний предел затребованного вида
			return std::numeric_limits <T>::lowest();
		/**
		 * Если целая часть числа лежит выше предела затребованного вида
		 */
		if(value >= static_cast <double> (std::numeric_limits <T>::max()))
			// Выводим верхний предел затребованного вида
			return std::numeric_limits <T>::max();
		// Выводим приведённое число
		return static_cast <T> (value);
	}
	/**
	 * @brief Функция проверки записи на числовую
	 *
	 * @details Проверкой этой решается, чем является часть пути: номером значения
	 *          перечня либо именем поля отображения. Отображение, поле числовое несущее,
	 *          разыскивается именем - вид вместилища решает раньше вида записи
	 *
	 * @param text проверяемая запись
	 * @return     признак числовой записи
	 *
	 */
	static bool numbering(const string & text) noexcept {
		// Выводим признак числовой записи
		return (!text.empty() && (text.find_first_not_of("0123456789") == string::npos));
	}
	/**
	 * @brief Функция получения ссылки на неопределённое значение
	 *
	 * @details Обращение постоянное, ничего не разыскавшее, обязано выдать ссылку, а
	 *          не завести значение: значение это одно на весь кодек и живёт оно всю
	 *          работу приложения
	 *
	 * @return ссылка на неопределённое значение
	 *
	 */
	static const awh::codec::yaml::Value & missing() noexcept {
		// Неопределённое значение, обращением неудачным выдаваемое
		static const awh::codec::yaml::Value result;
		// Выводим ссылку на неопределённое значение
		return result;
	}
	/**
	 * @brief Функция получения ссылки на пустую запись
	 *
	 * @return ссылка на пустую запись
	 *
	 */
	static const string & nothing() noexcept {
		// Пустая запись, обращением неудачным выдаваемая
		static const string result;
		// Выводим ссылку на пустую запись
		return result;
	}
}

/**
 * @brief Метод опознания вида числа по тексту значения
 *
 */
void awh::codec::yaml::Value::recognize() noexcept {
	// Выполняем опознание вида значения по записи его
	this->_type = narrow(this->_text, this->_schema, this->_number);
}
/**
 * @brief Метод проверки определённости значения
 *
 * @return признак определённости значения
 *
 */
bool awh::codec::yaml::Value::valid() const noexcept {
	// Выводим признак определённости значения
	return (this->_kind != kind_t::NONE);
}
/**
 * @brief Метод извлечения вида значения
 *
 * @return вид хранимого значения
 *
 */
awh::codec::yaml::kind_t awh::codec::yaml::Value::kind() const noexcept {
	// Выводим вид хранимого значения
	return this->_kind;
}
/**
 * @brief Метод извлечения вида хранения значения
 *
 * @return вид хранения значения
 *
 */
awh::codec::yaml::type_t awh::codec::yaml::Value::type() const noexcept {
	// Выводим вид хранения значения
	return this->_type;
}
/**
 * @brief Метод проверки вида хранения значения
 *
 * @param type сличаемый вид хранения
 * @return     признак совпадения вида
 *
 */
bool awh::codec::yaml::Value::is(const type_t type) const noexcept {
	// Выводим признак совпадения вида хранения значения
	return ((static_cast <uint32_t> (this->_type) & static_cast <uint32_t> (type)) > 0);
}
/**
 * @brief Метод извлечения количества значений вместилища
 *
 * @return количество значений вместилища
 *
 */
size_t awh::codec::yaml::Value::size() const noexcept {
	// Выводим количество значений вместилища
	return this->_items.size();
}
/**
 * @brief Метод проверки вместилища на пустоту
 *
 * @return признак пустоты вместилища
 *
 */
bool awh::codec::yaml::Value::empty() const noexcept {
	// Выводим признак пустоты вместилища
	return this->_items.empty();
}
/**
 * @brief Метод очистки значения
 *
 */
void awh::codec::yaml::Value::clear() noexcept {
	// Выполняем сброс вида хранимого значения
	this->_kind = kind_t::NONE;
	// Выполняем сброс вида хранения значения
	this->_type = type_t::UNDEFINED;
	// Выполняем сброс оформления записи значения
	this->_style = style_t::PLAIN;
	/**
	 * Выполняем сброс правила усечения переводов строк
	 *
	 * @note Сохраняющее правило взято умолчанием намеренно: у значения владеющего запись
	 *       есть само содержимое, и правило это единственное, при каком содержимое
	 *       возвращается тем же, сколько бы переводов строк ни стояло в конце. Дерево
	 *       документа перезаписывает блочные значения тем же правилом и по той же причине
	 */
	this->_chomp = chomp_t::KEEP;
	// Выполняем сброс построения вместилища
	this->_layout = layout_t::BLOCK;
	// Выполняем сброс разобранного числа
	this->_number = numeric_t();
	// Выполняем очистку содержимого значения
	this->_text.clear();
	// Выполняем очистку якоря значения
	this->_anchor.clear();
	// Выполняем очистку метки значения
	this->_tag.clear();
	// Выполняем очистку имён полей отображения
	this->_names.clear();
	// Выполняем очистку значений вместилища
	this->_items.clear();
}
/**
 * @brief Метод извлечения содержимого значения
 *
 * @return содержимое значения
 *
 */
const string & awh::codec::yaml::Value::text() const noexcept {
	// Выводим содержимое значения
	return this->_text;
}
/**
 * @brief Метод извлечения имени поля отображения по номеру
 *
 * @param index номер поля отображения
 * @return      имя поля отображения
 *
 */
const string & awh::codec::yaml::Value::key(const size_t index) const noexcept {
	/**
	 * Если поля отображения с таким номером нет
	 */
	if(index >= this->_names.size())
		// Выводим пустую запись
		return ::nothing();
	// Выводим имя поля отображения
	return this->_names.at(index);
}
/**
 * @brief Метод извлечения оформления записи значения
 *
 * @return оформление записи значения
 *
 */
awh::codec::yaml::style_t awh::codec::yaml::Value::style() const noexcept {
	// Выводим оформление записи значения
	return this->_style;
}
/**
 * @brief Метод установки оформления записи значения
 *
 * @param style устанавливаемое оформление записи
 *
 */
void awh::codec::yaml::Value::style(const style_t style) noexcept {
	// Выполняем установку оформления записи значения
	this->_style = style;
}
/**
 * @brief Метод извлечения правила усечения переводов строк
 *
 * @return правило усечения переводов строк
 *
 */
awh::codec::yaml::chomp_t awh::codec::yaml::Value::chomp() const noexcept {
	// Выводим правило усечения переводов строк
	return this->_chomp;
}
/**
 * @brief Метод установки правила усечения переводов строк
 *
 * @param chomp устанавливаемое правило усечения переводов строк
 *
 */
void awh::codec::yaml::Value::chomp(const chomp_t chomp) noexcept {
	// Выполняем установку правила усечения переводов строк
	this->_chomp = chomp;
}
/**
 * @brief Метод извлечения построения вместилища
 *
 * @return построение вместилища
 *
 */
awh::codec::yaml::layout_t awh::codec::yaml::Value::layout() const noexcept {
	// Выводим построение вместилища
	return this->_layout;
}
/**
 * @brief Метод установки построения вместилища
 *
 * @param layout устанавливаемое построение вместилища
 *
 */
void awh::codec::yaml::Value::layout(const layout_t layout) noexcept {
	// Выполняем установку построения вместилища
	this->_layout = layout;
}
/**
 * @brief Метод извлечения якоря значения
 *
 * @return якорь значения
 *
 */
const string & awh::codec::yaml::Value::anchor() const noexcept {
	// Выводим якорь значения
	return this->_anchor;
}
/**
 * @brief Метод установки якоря значения
 *
 * @param anchor устанавливаемый якорь значения
 *
 */
void awh::codec::yaml::Value::anchor(const string & anchor) noexcept {
	// Выполняем установку якоря значения
	this->_anchor = anchor;
}
/**
 * @brief Метод извлечения метки значения
 *
 * @return метка значения
 *
 */
const string & awh::codec::yaml::Value::tag() const noexcept {
	// Выводим метку значения
	return this->_tag;
}
/**
 * @brief Метод установки метки значения
 *
 * @param tag устанавливаемая метка значения
 *
 */
void awh::codec::yaml::Value::tag(const string & tag) noexcept {
	// Выполняем установку метки значения
	this->_tag = tag;
}
/**
 * @brief Метод проверки наличия поля отображения с указанным именем
 *
 * @param name разыскиваемое имя поля отображения
 * @return     признак наличия поля отображения
 *
 */
bool awh::codec::yaml::Value::contains(const string & name) const noexcept {
	/**
	 * Если значение отображением не является
	 */
	if(this->_kind != kind_t::MAPPING)
		// Выводим отсутствие поля отображения
		return false;
	/**
	 * Выполняем перебор имён полей отображения
	 */
	for(auto & item : this->_names){
		/**
		 * Если имя поля отображения совпадает с разыскиваемым
		 */
		if(item.compare(name) == 0)
			// Выводим наличие поля отображения
			return true;
	}
	// Выводим отсутствие поля отображения
	return false;
}
/**
 * @brief Метод обращения к полю отображения по имени
 *
 * @param name имя поля отображения
 * @return     ссылка на значение поля отображения
 *
 */
const awh::codec::yaml::Value & awh::codec::yaml::Value::operator [] (const string & name) const noexcept {
	/**
	 * Если значение отображением не является
	 */
	if(this->_kind != kind_t::MAPPING)
		// Выводим неопределённое значение
		return ::missing();
	/**
	 * Выполняем перебор имён полей отображения
	 */
	for(size_t i = 0; i < this->_names.size(); i++){
		/**
		 * Если имя поля отображения совпадает с разыскиваемым
		 */
		if(this->_names.at(i).compare(name) == 0)
			// Выводим значение разысканного поля отображения
			return this->_items.at(i);
	}
	// Выводим неопределённое значение
	return ::missing();
}
/**
 * @brief Метод обращения к полю отображения по имени с заведением недостающего
 *
 * @param name имя поля отображения
 * @return     ссылка на значение поля отображения
 *
 */
awh::codec::yaml::Value & awh::codec::yaml::Value::operator [] (const string & name) noexcept {
	/**
	 * Если значение отображением не является
	 *
	 * @note Значение перерождается отображением, а простое содержимое его теряется.
	 *       Решено это так же, как решает розыск с заведением: обращение изменяемое есть
	 *       заявление о том, что здесь стоит вместилище, и спорить с ним нечем
	 */
	if(this->_kind != kind_t::MAPPING){
		// Выполняем очистку прежнего значения
		this->clear();
		// Назначаем значению вид отображения пар
		this->_kind = kind_t::MAPPING;
		// Назначаем значению вид хранения отображения пар
		this->_type = type_t::MAPPING;
	}
	/**
	 * Выполняем перебор имён полей отображения
	 */
	for(size_t i = 0; i < this->_names.size(); i++){
		/**
		 * Если имя поля отображения совпадает с разыскиваемым
		 */
		if(this->_names.at(i).compare(name) == 0)
			// Выводим значение разысканного поля отображения
			return this->_items.at(i);
	}
	// Выполняем добавление имени заводимого поля отображения
	this->_names.push_back(name);
	// Выполняем добавление значения заводимого поля отображения
	this->_items.emplace_back();
	// Выводим значение заведённого поля отображения
	return this->_items.back();
}
/**
 * @brief Метод обращения к значению вместилища по номеру
 *
 * @param index номер значения во вместилище
 * @return      ссылка на значение вместилища
 *
 */
const awh::codec::yaml::Value & awh::codec::yaml::Value::operator [] (const size_t index) const noexcept {
	/**
	 * Если значения с таким номером вместилище не несёт
	 */
	if(index >= this->_items.size())
		// Выводим неопределённое значение
		return ::missing();
	// Выводим значение вместилища
	return this->_items.at(index);
}
/**
 * @brief Метод обращения к значению вместилища по номеру с заведением недостающего
 *
 * @param index номер значения во вместилище
 * @return      ссылка на значение вместилища
 *
 */
awh::codec::yaml::Value & awh::codec::yaml::Value::operator [] (const size_t index) noexcept {
	/**
	 * Если значение вместилищем не является
	 */
	if((this->_kind != kind_t::SEQUENCE) && (this->_kind != kind_t::MAPPING)){
		// Выполняем очистку прежнего значения
		this->clear();
		// Назначаем значению вид перечня значений
		this->_kind = kind_t::SEQUENCE;
		// Назначаем значению вид хранения перечня значений
		this->_type = type_t::SEQUENCE;
	}
	/**
	 * Выполняем рост вместилища до затребованного номера
	 */
	while(index >= this->_items.size()){
		/**
		 * Если вместилище является отображением пар
		 *
		 * @note Имя заводимому полю берётся номером его: отображение без имени поля
		 *       записано быть не может, а рост по номеру заведение поля означает
		 */
		if(this->_kind == kind_t::MAPPING)
			// Выполняем добавление имени заводимого поля отображения
			this->_names.push_back(std::to_string(this->_items.size()));
		// Выполняем добавление значения вместилища
		this->_items.emplace_back();
	}
	// Выводим значение вместилища
	return this->_items.at(index);
}
/**
 * @brief Метод обращения к значению по пути
 *
 * @param path путь к разыскиваемому значению
 * @return     ссылка на разысканное значение
 *
 */
const awh::codec::yaml::Value & awh::codec::yaml::Value::at(const string & path) const noexcept {
	// Получаем путь к значению без ведущего разделителя частей
	const string route((!path.empty() && (path.front() == '/')) ? path.substr(1) : path);
	/**
	 * Если путь к значению пуст вовсе
	 */
	if(route.empty())
		// Выводим текущее значение
		return (* this);
	// Получаем ссылку на значение, путём разыскиваемое
	const Value * result = this;
	// Начало очередной части пути
	size_t start = 0;
	/**
	 * Выполняем перебор частей пути
	 */
	while(start <= route.size()){
		// Разыскиваем конец очередной части пути
		const size_t separator = route.find('/', start);
		// Получаем очередную часть пути
		const string part(route.substr(start, ((separator == string::npos) ? string::npos : (separator - start))));
		/**
		 * Если вместилище является перечнем значений, а часть пути числом
		 */
		if((result->_kind == kind_t::SEQUENCE) && ::numbering(part))
			// Выполняем переход к значению перечня по номеру его
			result = &((* result)[static_cast <size_t> (::strtoull(part.c_str(), nullptr, 10))]);
		// Выполняем переход к полю отображения по имени его
		else result = &((* result)[part]);
		/**
		 * Если разыскать значение не удалось
		 */
		if(!result->valid())
			// Выводим неопределённое значение
			return ::missing();
		/**
		 * Если части пути закончились
		 */
		if(separator == string::npos)
			// Выходим из перебора частей пути
			break;
		// Выполняем переход к следующей части пути
		start = (separator + 1);
	}
	// Выводим разысканное значение
	return (* result);
}
/**
 * @brief Метод обращения к значению по пути с заведением недостающего
 *
 * @param path путь к разыскиваемому значению
 * @return     ссылка на разысканное либо заведённое значение
 *
 */
awh::codec::yaml::Value & awh::codec::yaml::Value::place(const string & path) noexcept {
	// Получаем путь к значению без ведущего разделителя частей
	const string route((!path.empty() && (path.front() == '/')) ? path.substr(1) : path);
	/**
	 * Если путь к значению пуст вовсе
	 */
	if(route.empty())
		// Выводим текущее значение
		return (* this);
	// Получаем ссылку на значение, путём разыскиваемое
	Value * result = this;
	// Начало очередной части пути
	size_t start = 0;
	/**
	 * Выполняем перебор частей пути
	 */
	while(start <= route.size()){
		// Разыскиваем конец очередной части пути
		const size_t separator = route.find('/', start);
		// Получаем очередную часть пути
		const string part(route.substr(start, ((separator == string::npos) ? string::npos : (separator - start))));
		// Получаем признак того, что часть пути является числом
		const bool numeric = ::numbering(part);
		/**
		 * Если значение вместилищем не является вовсе
		 *
		 * @note Вид заводимого вместилища решает запись части пути: часть числовая
		 *       заводит перечень значений, а всякая иная - отображение пар
		 */
		if((result->_kind != kind_t::SEQUENCE) && (result->_kind != kind_t::MAPPING)){
			// Выполняем очистку прежнего значения
			result->clear();
			// Назначаем значению вид заводимого вместилища
			result->_kind = (numeric ? kind_t::SEQUENCE : kind_t::MAPPING);
			// Назначаем значению вид хранения заводимого вместилища
			result->_type = (numeric ? type_t::SEQUENCE : type_t::MAPPING);
		}
		/**
		 * Если вместилище является перечнем значений, а часть пути числом
		 */
		if((result->_kind == kind_t::SEQUENCE) && numeric)
			// Выполняем переход к значению перечня по номеру его
			result = &((* result)[static_cast <size_t> (::strtoull(part.c_str(), nullptr, 10))]);
		// Выполняем переход к полю отображения по имени его
		else result = &((* result)[part]);
		/**
		 * Если части пути закончились
		 */
		if(separator == string::npos)
			// Выходим из перебора частей пути
			break;
		// Выполняем переход к следующей части пути
		start = (separator + 1);
	}
	// Выводим разысканное либо заведённое значение
	return (* result);
}
/**
 * @brief Метод добавления значения в конец перечня значений
 *
 * @param value добавляемое значение
 * @return      признак успешности добавления
 *
 */
bool awh::codec::yaml::Value::push(const Value & value) noexcept {
	/**
	 * Если значение неопределённым является
	 */
	if(this->_kind == kind_t::NONE){
		// Назначаем значению вид перечня значений
		this->_kind = kind_t::SEQUENCE;
		// Назначаем значению вид хранения перечня значений
		this->_type = type_t::SEQUENCE;
	}
	/**
	 * Если значение перечнем значений не является
	 *
	 * @note Перерождения здесь нет намеренно: добавление в значение простое есть
	 *       промах потребителя, и отказ его обнаруживает, тогда как перерождение
	 *       потеряло бы содержимое молча
	 */
	if(this->_kind != kind_t::SEQUENCE)
		// Выводим признак неудачного добавления
		return false;
	// Выполняем добавление значения в конец перечня
	this->_items.push_back(value);
	// Выводим признак успешного добавления
	return true;
}
/**
 * @brief Метод установки поля отображения
 *
 * @param name  имя поля отображения
 * @param value устанавливаемое значение поля
 * @return      признак успешности установки
 *
 */
bool awh::codec::yaml::Value::insert(const string & name, const Value & value) noexcept {
	/**
	 * Если значение неопределённым является
	 */
	if(this->_kind == kind_t::NONE){
		// Назначаем значению вид отображения пар
		this->_kind = kind_t::MAPPING;
		// Назначаем значению вид хранения отображения пар
		this->_type = type_t::MAPPING;
	}
	/**
	 * Если значение отображением пар не является
	 */
	if(this->_kind != kind_t::MAPPING)
		// Выводим признак неудачной установки
		return false;
	/**
	 * Выполняем перебор имён полей отображения
	 */
	for(size_t i = 0; i < this->_names.size(); i++){
		/**
		 * Если имя поля отображения совпадает с устанавливаемым
		 *
		 * @note Поле перезаписывается на своём месте: порядок полей отображения есть
		 *       порядок их записи в текст, и перестановка поля при перезаписи его
		 *       переменила бы текст там, где потребитель менял лишь значение
		 */
		if(this->_names.at(i).compare(name) == 0){
			// Выполняем перезапись значения поля отображения
			this->_items.at(i) = value;
			// Выводим признак успешной установки
			return true;
		}
	}
	// Выполняем добавление имени заводимого поля отображения
	this->_names.push_back(name);
	// Выполняем добавление значения заводимого поля отображения
	this->_items.push_back(value);
	// Выводим признак успешной установки
	return true;
}
/**
 * @brief Метод добавления поля отображения рядом с одноимённым
 *
 * @param name  имя поля отображения
 * @param value добавляемое значение поля
 * @return      признак успешности добавления
 *
 */
bool awh::codec::yaml::Value::append(const string & name, const Value & value) noexcept {
	/**
	 * Если значение неопределённым является
	 */
	if(this->_kind == kind_t::NONE){
		// Назначаем значению вид отображения пар
		this->_kind = kind_t::MAPPING;
		// Назначаем значению вид хранения отображения пар
		this->_type = type_t::MAPPING;
	}
	/**
	 * Если значение отображением пар не является
	 */
	if(this->_kind != kind_t::MAPPING)
		// Выводим признак неудачного добавления
		return false;
	/**
	 * Выполняем добавление имени заводимого поля отображения
	 *
	 * @note Розыска по имени здесь нет вовсе - тем метод и отличается от установки поля:
	 *       разбор с настройкой `duplicate_t::KEEP` удерживает все вхождения
	 *       повторяющегося имени, и воспроизвести такое отображение установкой нельзя
	 */
	this->_names.push_back(name);
	// Выполняем добавление значения заводимого поля отображения
	this->_items.push_back(value);
	// Выводим признак успешного добавления
	return true;
}
/**
 * @brief Метод снятия поля отображения по имени
 *
 * @param name имя снимаемого поля отображения
 * @return     признак успешности снятия
 *
 */
bool awh::codec::yaml::Value::erase(const string & name) noexcept {
	/**
	 * Если значение отображением пар не является
	 */
	if(this->_kind != kind_t::MAPPING)
		// Выводим признак неудачного снятия
		return false;
	/**
	 * Выполняем перебор имён полей отображения
	 */
	for(size_t i = 0; i < this->_names.size(); i++){
		/**
		 * Если имя поля отображения совпадает со снимаемым
		 */
		if(this->_names.at(i).compare(name) == 0){
			// Выполняем снятие имени поля отображения
			this->_names.erase(std::next(this->_names.begin(), static_cast <ptrdiff_t> (i)));
			// Выполняем снятие значения поля отображения
			this->_items.erase(std::next(this->_items.begin(), static_cast <ptrdiff_t> (i)));
			// Выводим признак успешного снятия
			return true;
		}
	}
	// Выводим признак неудачного снятия
	return false;
}
/**
 * @brief Метод снятия значения вместилища по номеру
 *
 * @param index номер снимаемого значения вместилища
 * @return      признак успешности снятия
 *
 */
bool awh::codec::yaml::Value::erase(const size_t index) noexcept {
	/**
	 * Если значения с таким номером вместилище не несёт
	 */
	if(index >= this->_items.size())
		// Выводим признак неудачного снятия
		return false;
	/**
	 * Если вместилище является отображением пар
	 */
	if(this->_kind == kind_t::MAPPING)
		// Выполняем снятие имени поля отображения
		this->_names.erase(std::next(this->_names.begin(), static_cast <ptrdiff_t> (index)));
	// Выполняем снятие значения вместилища
	this->_items.erase(std::next(this->_items.begin(), static_cast <ptrdiff_t> (index)));
	// Выводим признак успешного снятия
	return true;
}
/**
 * @brief Шаблонный метод извлечения числа затребованным видом
 *
 * @tparam T      затребованный вид числа
 * @param  result переменная, куда помещается извлечённое значение
 * @return        признак успешности извлечения
 *
 */
template <typename T>
bool awh::codec::yaml::Value::extract(T & result) const noexcept {
	/**
	 * Если значение является целым числом
	 */
	if(static_cast <uint32_t> (this->_type) & static_cast <uint32_t> (type_t::SIGNED)){
		// Устанавливаем извлечённое значение приведением языка
		result = static_cast <T> (this->_number.integer);
		// Выводим признак успешного извлечения
		return true;
	}
	/**
	 * Если значение является целым числом без знака
	 */
	if(static_cast <uint32_t> (this->_type) & static_cast <uint32_t> (type_t::UNSIGNED)){
		// Устанавливаем извлечённое значение приведением языка
		result = static_cast <T> (this->_number.natural);
		// Выводим признак успешного извлечения
		return true;
	}
	/**
	 * Если значение является дробным числом либо числом, ни в один родной вид не вместимым
	 */
	if(static_cast <uint32_t> (this->_type) & (static_cast <uint32_t> (type_t::REAL) | static_cast <uint32_t> (type_t::EXTENDED))){
		// Устанавливаем извлечённое значение приведением дробного
		result = ::convert <T> (this->_number.real);
		// Выводим признак успешного извлечения
		return true;
	}
	// Выводим признак неудачного извлечения
	return false;
}
/**
 * @brief Метод извлечения логического значения
 *
 * @param result переменная, куда помещается извлечённое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::yaml::Value::value(bool & result) const noexcept {
	/**
	 * Если значение логическим не является
	 */
	if(this->_kind != kind_t::BOOL)
		// Выводим признак неудачного извлечения
		return false;
	// Устанавливаем извлечённое логическое значение
	result = (this->_number.integer != 0);
	// Выводим признак успешного извлечения
	return true;
}
/**
 * @brief Метод извлечения целого числа шириною в один байт
 *
 * @param result переменная, куда помещается извлечённое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::yaml::Value::value(int8_t & result) const noexcept {
	// Выводим извлечённое число
	return this->extract <int8_t> (result);
}
/**
 * @brief Метод извлечения целого числа шириною в два байта
 *
 * @param result переменная, куда помещается извлечённое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::yaml::Value::value(int16_t & result) const noexcept {
	// Выводим извлечённое число
	return this->extract <int16_t> (result);
}
/**
 * @brief Метод извлечения целого числа шириною в четыре байта
 *
 * @param result переменная, куда помещается извлечённое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::yaml::Value::value(int32_t & result) const noexcept {
	// Выводим извлечённое число
	return this->extract <int32_t> (result);
}
/**
 * @brief Метод извлечения целого числа шириною в восемь байтов
 *
 * @param result переменная, куда помещается извлечённое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::yaml::Value::value(int64_t & result) const noexcept {
	// Выводим извлечённое число
	return this->extract <int64_t> (result);
}
/**
 * @brief Метод извлечения целого числа без знака шириною в один байт
 *
 * @param result переменная, куда помещается извлечённое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::yaml::Value::value(uint8_t & result) const noexcept {
	// Выводим извлечённое число
	return this->extract <uint8_t> (result);
}
/**
 * @brief Метод извлечения целого числа без знака шириною в два байта
 *
 * @param result переменная, куда помещается извлечённое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::yaml::Value::value(uint16_t & result) const noexcept {
	// Выводим извлечённое число
	return this->extract <uint16_t> (result);
}
/**
 * @brief Метод извлечения целого числа без знака шириною в четыре байта
 *
 * @param result переменная, куда помещается извлечённое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::yaml::Value::value(uint32_t & result) const noexcept {
	// Выводим извлечённое число
	return this->extract <uint32_t> (result);
}
/**
 * @brief Метод извлечения целого числа без знака шириною в восемь байтов
 *
 * @param result переменная, куда помещается извлечённое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::yaml::Value::value(uint64_t & result) const noexcept {
	// Выводим извлечённое число
	return this->extract <uint64_t> (result);
}
/**
 * @brief Метод извлечения дробного числа одинарной точности
 *
 * @param result переменная, куда помещается извлечённое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::yaml::Value::value(float & result) const noexcept {
	// Выводим извлечённое число
	return this->extract <float> (result);
}
/**
 * @brief Метод извлечения дробного числа двойной точности
 *
 * @param result переменная, куда помещается извлечённое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::yaml::Value::value(double & result) const noexcept {
	// Выводим извлечённое число
	return this->extract <double> (result);
}
/**
 * @brief Метод извлечения строкового значения
 *
 * @param result переменная, куда помещается извлечённое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::yaml::Value::value(string & result) const noexcept {
	/**
	 * Если значение вместилищем является
	 *
	 * @note Вместилище строкою не выдаётся: выдавать его пришлось бы сборкой текста, а
	 *       сборка есть иная работа, и зовётся она иначе. Решено это дословно так же,
	 *       как решено у дерева документа
	 */
	if((this->_kind == kind_t::MAPPING) || (this->_kind == kind_t::SEQUENCE) || (this->_kind == kind_t::NONE))
		// Выводим признак неудачного извлечения
		return false;
	// Устанавливаем извлечённое строковое значение записью его
	result.assign(this->_text);
	// Выводим признак успешного извлечения
	return true;
}
/**
 * @brief Метод снятия значения со ссылки на узел документа
 *
 * @param value ссылка на узел документа
 *
 */
void awh::codec::yaml::Value::absorb(const Document::value_t & value) noexcept {
	// Выполняем очистку прежнего значения
	this->clear();
	/**
	 * Если ссылка на узел документа недействительна
	 */
	if(!value.valid())
		// Выходим из снятия значения
		return;
	/**
	 * Выполняем снятие схемы, над разбором действовавшей
	 *
	 * @details Схема снимается вместе со значением: запись `on` логическою является лишь
	 *          под наречием 1.1, и перезапись схемою иною переменила бы смысл значения, а
	 *          не только написание его
	 *
	 * @note Нашёл это ворошитель круговым ходом снятого значения на тексте с директивой
	 *       `%YAML 1.1`
	 */
	this->_schema = value.schema();
	// Выполняем снятие вида значения
	this->_kind = value.kind();
	// Выполняем снятие вида хранения значения
	this->_type = value.type();
	// Выполняем снятие оформления записи значения
	this->_style = value.style();
	// Выполняем снятие содержимого значения
	this->_text.assign(value.text());
	/**
	 * Выполняем назначение правила усечения переводов строк по содержимому
	 *
	 * @details Правило берётся тем, при каком содержимое возвращается тем же: сохраняющее,
	 *          коль скоро содержимое переводом строки оканчивается, и отсекающее, коль
	 *          скоро не оканчивается. Записи `|-` перевода строки в конце не имеют вовсе, и
	 *          правило сохраняющее приписало бы им лишний
	 */
	this->_chomp = ((!this->_text.empty() && (this->_text.back() == '\n')) ? chomp_t::KEEP : chomp_t::STRIP);
	// Выполняем снятие якоря значения
	this->_anchor.assign(value.anchor());
	// Выполняем снятие метки значения
	this->_tag.assign(value.tag());
	/**
	 * Если значение вместилищем является
	 */
	if((this->_kind == kind_t::MAPPING) || (this->_kind == kind_t::SEQUENCE)){
		// Получаем признак того, что вместилище является отображением пар
		const bool mapping = (this->_kind == kind_t::MAPPING);
		/**
		 * Выполняем перебор значений вместилища
		 */
		for(Document::value_t item = value.begin(); item.valid(); item = item.next()){
			/**
			 * Если вместилище является отображением пар
			 */
			if(mapping)
				// Выполняем снятие имени поля отображения
				this->_names.emplace_back(item.name());
			// Выполняем заведение значения вместилища
			this->_items.emplace_back();
			// Выполняем снятие значения вместилища
			this->_items.back().absorb(item);
		}
		// Выходим из снятия значения
		return;
	}
	/**
	 * Если значение числом является
	 *
	 * @details Число **снимается с узла**, а не разбирается заново из записи его. Разбор
	 *          заново шёл бы схемою своей, а узел разобран схемою документа, и записи
	 *          вроде `0777` расходились бы: наречие 1.1 читает её восьмеричной, давая 511,
	 *          а схема ядровая - десятичной, давая 777
	 *
	 * @note Нашёл это ворошитель сличением извлечения с деревом. Прежде здесь стоял разбор
	 *       записи, и довод при нём стоял такой: «узел хранит число полем закрытым, а
	 *       запись известна и разбор её дёшев». Довод был верен ценою, но неверен по сути -
	 *       дёшев путь, ведущий не туда
	 */
	if(this->_kind == kind_t::NUMBER){
		/**
		 * Если число целым со знаком является
		 */
		if(this->is(type_t::SIGNED))
			// Выполняем снятие целого числа со знаком
			value.value(this->_number.integer);
		/**
		 * Если число целым без знака является
		 */
		else if(this->is(type_t::UNSIGNED))
			// Выполняем снятие целого числа без знака
			value.value(this->_number.natural);
		// Выполняем снятие дробного числа
		else value.value(this->_number.real);
	}
	/**
	 * Если значение логическим является
	 */
	else if(this->_kind == kind_t::BOOL) {
		// Извлекаемое логическое значение
		bool result = false;
		/**
		 * Если извлечь логическое значение удалось
		 */
		if(value.value(result))
			// Выполняем сохранение извлечённого логического значения
			this->_number.integer = (result ? 1 : 0);
	}
}
/**
 * @brief Метод записи значения в поток записи
 *
 * @param writer поток записи, куда ложится значение
 *
 */
void awh::codec::yaml::Value::compose(writer_t & writer) const noexcept {
	/**
	 * Если значение якорь несёт
	 */
	if(!this->_anchor.empty())
		// Выполняем запись якоря, значению предпосылаемого
		writer.anchor(this->_anchor);
	/**
	 * Если значение метку несёт
	 */
	if(!this->_tag.empty())
		// Выполняем запись метки, значению предпосылаемой
		writer.tag(this->_tag);
	/**
	 * Определяем вид записываемого значения
	 */
	switch(static_cast <uint8_t> (this->_kind)){
		/**
		 * Если значение является отображением пар
		 */
		case static_cast <uint8_t> (kind_t::MAPPING): {
			/**
			 * Если открыть отображение пар не удалось
			 */
			if(!writer.mapping(this->_layout))
				// Выходим из записи значения
				return;
			/**
			 * Выполняем перебор полей отображения
			 */
			for(size_t i = 0; i < this->_items.size(); i++){
				// Выполняем запись имени поля отображения
				writer.key(this->key(i));
				// Выполняем запись значения поля отображения
				this->_items.at(i).compose(writer);
			}
			// Выполняем закрытие записанного отображения пар
			writer.close();
		} break;
		/**
		 * Если значение является перечнем значений
		 */
		case static_cast <uint8_t> (kind_t::SEQUENCE): {
			/**
			 * Если открыть перечень значений не удалось
			 */
			if(!writer.sequence(this->_layout))
				// Выходим из записи значения
				return;
			/**
			 * Выполняем перебор значений перечня
			 */
			for(auto & item : this->_items)
				// Выполняем запись значения перечня
				item.compose(writer);
			// Выполняем закрытие записанного перечня значений
			writer.close();
		} break;
		/**
		 * Если значение неопределённым является
		 *
		 * @note Значение неопределённое записывается пустотою: обращение изменяемое
		 *       заводит поле именно им, и запись такого поля обязана дать `null`, а не
		 *       пропасть из текста вовсе
		 */
		case static_cast <uint8_t> (kind_t::NONE):
		/**
		 * Если значение пустым является
		 */
		case static_cast <uint8_t> (kind_t::NUL): {
			/**
			 * Если запись пустоты дана самим значением
			 */
			if(!this->_text.empty()){
				// Выполняем запись пустоты записью её
				writer.raw(this->_text);
				// Выходим из записи значения
				return;
			}
			/**
			 * Если пустота стоит корнем документа, свойств не неся
			 *
			 * @details Пустота записывается пустотою же: строка `- ` есть запись перечня с
			 *          пустым значением, а `value: !` - пара с пустым значением, меткою
			 *          помеченным. Записью описания её выражать нельзя - метка, значению
			 *          предпосланная, отменяет разрешение схемы, и `! null` вернулось бы
			 *          строкою `null` вместо пустоты
			 *
			 * @note Словом `null` пустота не выражается нигде, даже корнем: схема защитная
			 *       слова того не знает, и значение возвращалось бы обратно строкою.
			 *       Присутствие документа в потоке держит черта его начала. Правило это
			 *       взято у дерева документа дословно - разойдись они, и перезапись снятого
			 *       значения разошлась бы с перезаписью дерева, из какого оно снято
			 *
			 * @note Нашёл это ворошитель круговым ходом снятого значения на записи
			 *       `value: !`
			 */
			writer.raw(string());
		} break;
		/**
		 * Если значение строкою является
		 */
		case static_cast <uint8_t> (kind_t::STRING): {
			/**
			 * Если значение записано блочным
			 */
			if((this->_style == style_t::LITERAL) || (this->_style == style_t::FOLDED)){
				/**
				 * Если записать блочное значение удалось
				 */
				if(writer.block(this->_text, this->_style, this->_chomp))
					// Выходим из записи значения
					return;
				// Выполняем запись строкового значения оградою двойною
				writer.value(this->_text, style_t::DOUBLE);
				// Выходим из записи значения
				return;
			}
			/**
			 * Если значение записано без ограды
			 *
			 * @note Ограда решается наново: строка `12`, без ограды записанная, прочлась бы
			 *       обратно числом, и перезапись переменила бы вид значения
			 */
			if(this->_style == style_t::PLAIN)
				// Выполняем запись строкового значения оградою, содержимым решаемой
				writer.value(this->_text);
			// Выполняем запись строкового значения оградою указанной
			else writer.value(this->_text, this->_style);
		} break;
		/**
		 * Если значение иного вида является
		 */
		case static_cast <uint8_t> (kind_t::NUMBER): {
			/**
			 * Выполняем запись числа записью описания
			 *
			 * @details Запись эта берётся описанием, а не удержанным написанием: двоичное
			 *          `0b1010` да восьмеричное `0777` числами читает одно наречие 1.1, и
			 *          значение, снятое с текста того наречия, возвращалось бы строкою
			 *          всюду, где читают по 1.2. Оформления владеющее значение не
			 *          удерживает - так решено договором о нём
			 *
			 * @note Нашёл это ворошитель круговым ходом снятого значения на тексте с
			 *       директивой `%YAML 1.1` и записью `0b1010`
			 */
			/**
			 * Если число ни в один родной вид не вмещается
			 *
			 * @details Такое число хранится записью своей, а выдаётся дробным приближением:
			 *          запись `12345678901234567890123456789` описанием вернулась бы
			 *          приближением `1.2345678901234568e+28`, и круговой ход потерял бы
			 *          число. Наречия записи такой не касаются - двоичных да восьмеричных
			 *          записей той длины не бывает, - а перезапись несёт директиву наречия
			 *          своего, и запись читается обратно тою же схемой
			 *
			 * @note Указал на это ведущий кодеков JSON и XML: у него дословно хранится
			 *       ровно тот же вид, и по тому же доводу
			 */
			if(this->is(type_t::EXTENDED) && !this->_text.empty())
				// Выполняем запись числа дословно, как оно записью дано
				writer.raw(this->_text);
			/**
			 * Если число целым со знаком является
			 */
			else if(this->is(type_t::SIGNED))
				// Выполняем запись целого числа со знаком
				writer.value(this->_number.integer);
			/**
			 * Если число целым без знака является
			 */
			else if(this->is(type_t::UNSIGNED))
				// Выполняем запись целого числа без знака
				writer.value(this->_number.natural);
			// Выполняем запись дробного числа
			else writer.value(this->_number.real);
		} break;
		/**
		 * Если значение логическим является
		 */
		case static_cast <uint8_t> (kind_t::BOOL): {
			/**
			 * Выполняем запись логического значения записью описания
			 *
			 * @details Запись эта берётся описанием, а не удержанным написанием: слова
			 *          `yes` да `on` логическими читает одно наречие 1.1, и значение,
			 *          снятое с текста того наречия, возвращалось бы строкою всюду, где
			 *          читают по 1.2. Оформления владеющее значение не удерживает, а
			 *          `true` с `false` читаются логическими у всех схем, кроме защитной
			 *
			 * @note Нашёл это ворошитель круговым ходом снятого значения на тексте с
			 *       директивой `%YAML 1.1` и записью `yes`
			 */
			writer.value(this->_number.integer != 0);
		} break;
		/**
		 * Если значение иного вида является
		 */
		default:
			// Выполняем запись значения дословно, как оно записью дано
			writer.raw(this->_text);
	}
}
/**
 * @brief Метод разбора текста YAML во владеющее значение
 *
 * @param text разбираемый текст YAML
 * @return     признак успешности разбора
 *
 */
bool awh::codec::yaml::Value::parse(const string & text) noexcept {
	// Выводим признак успешности разбора настройками обычными
	return this->parse(text, document_t::settings_t());
}
/**
 * @brief Метод разбора текста YAML указанными настройками
 *
 * @param text     разбираемый текст YAML
 * @param settings настройки разбора текста
 * @return         признак успешности разбора
 *
 */
bool awh::codec::yaml::Value::parse(const string & text, const Document::settings_t & settings) noexcept {
	// Выполняем очистку прежнего значения
	this->clear();
	// Выполняем заведение дерева документа
	document_t document(settings);
	/**
	 * Если разобрать текст документа не удалось
	 */
	if(!document.parse(text))
		// Выводим признак неудачного разбора
		return false;
	// Выполняем снятие дерева документа собственной памятью
	this->absorb(document.root());
	// Выводим признак успешного разбора
	return true;
}
/**
 * @brief Метод разбора текста YAML из файла
 *
 * @param filename адрес разбираемого файла
 * @return         признак успешности разбора
 *
 */
bool awh::codec::yaml::Value::load(const string & filename) noexcept {
	// Выводим признак успешности разбора настройками обычными
	return this->load(filename, document_t::settings_t());
}
/**
 * @brief Метод разбора текста YAML из файла указанными настройками
 *
 * @param filename адрес разбираемого файла
 * @param settings настройки разбора текста
 * @return         признак успешности разбора
 *
 */
bool awh::codec::yaml::Value::load(const string & filename, const Document::settings_t & settings) noexcept {
	// Выполняем очистку прежнего значения
	this->clear();
	// Выполняем заведение дерева документа
	document_t document(settings);
	/**
	 * Если разобрать текст документа из файла не удалось
	 */
	if(!document.load(filename))
		// Выводим признак неудачного разбора
		return false;
	// Выполняем снятие дерева документа собственной памятью
	this->absorb(document.root());
	// Выводим признак успешного разбора
	return true;
}
/**
 * @brief Метод перезаписи значения в текст YAML с указанными настройками
 *
 * @param settings настройки записи текста
 * @return         текст YAML
 *
 */
string awh::codec::yaml::Value::dump(const writer_t::settings_t & settings) const noexcept {
	/**
	 * Если значение неопределённым является
	 *
	 * @details Значение неопределённое есть отсутствие значения, а не пустота: текст
	 *          `%YAML 1.1` несёт наречие и ни одного документа, и снятое с него значение
	 *          неопределённо. Записать его словом `null` значило бы выдумать документ,
	 *          какого не было
	 *
	 * @note Поле неопределённое внутри вместилища записывается именно словом `null` -
	 *       иного описание не знает, - и противоречия здесь нет: поле там есть, а
	 *       документа здесь нет вовсе
	 *
	 * @note Нашёл это ворошитель круговым ходом снятого значения на тексте из одного
	 *       наречия
	 */
	if(!this->valid())
		// Выводим пустой текст
		return string();
	// Выполняем заведение потока записи
	writer_t writer;
	// Выполняем установку настроек записи текста
	writer.settings(settings);
	// Выполняем открытие записываемого документа
	writer.document();
	// Выполняем запись значения в поток записи
	this->compose(writer);
	// Выполняем завершение записи документа
	writer.finish();
	// Выводим записанный текст YAML
	return writer.take();
}
/**
 * @brief Метод перезаписи значения в текст YAML
 *
 * @return текст YAML
 *
 */
string awh::codec::yaml::Value::dump() const noexcept {
	// Настройки записи собираемого текста
	writer_t::settings_t settings;
	/**
	 * Устанавливаем схему разрешения видов скалярных значений
	 *
	 * @note Берётся схема, значением удержанная: запись `on` логическою является лишь под
	 *       наречием 1.1, и решать ограду иною схемою значило бы выдать текст, читаемый
	 *       иначе, чем он записан
	 */
	settings.schema = this->_schema;
	/**
	 * Устанавливаем запись директивы наречия
	 *
	 * @note Директива записывается лишь под схемою наречия 1.1: без неё чтение вернулось
	 *       бы к схеме ядровой, и записи вроде `on` вернулись бы строками
	 */
	settings.version = (this->_schema == schema_t::LEGACY);
	// Выводим записанный текст YAML
	return this->dump(settings);
}
/**
 * @brief Метод записи значения в файл
 *
 * @param filename адрес записываемого файла
 * @return         признак успешности записи
 *
 */
bool awh::codec::yaml::Value::save(const string & filename) const noexcept {
	// Выполняем открытие записываемого файла
	ofstream file(filename, ios::binary | ios::trunc);
	/**
	 * Если открыть записываемый файл не удалось
	 */
	if(!file.is_open())
		// Выводим признак неудачной записи
		return false;
	// Получаем текст YAML, значением записанный
	const string text = this->dump();
	// Выполняем запись текста YAML в файл
	file.write(text.data(), static_cast <streamsize> (text.size()));
	// Выводим признак успешности записи
	return file.good();
}
/**
 * @brief Метод сличения значений
 *
 * @param value сличаемое значение
 * @return      признак совпадения значений
 *
 */
bool awh::codec::yaml::Value::operator == (const Value & value) const noexcept {
	/**
	 * Если виды значений расходятся
	 */
	if(this->_kind != value._kind)
		// Выводим признак несовпадения значений
		return false;
	/**
	 * Если значение вместилищем является
	 */
	if((this->_kind == kind_t::MAPPING) || (this->_kind == kind_t::SEQUENCE)){
		/**
		 * Если количества значений вместилищ расходятся
		 */
		if(this->_items.size() != value._items.size())
			// Выводим признак несовпадения значений
			return false;
		/**
		 * Если вместилище является отображением пар
		 *
		 * @details Порядок полей сличению не подлежит: описание порядка полей отображения
		 *          не предписывает вовсе, и два текста, одни и те же поля разным порядком
		 *          записавшие, суть один и тот же документ. Записи порядок значим и там
		 *          сохраняется, а сличению он не указ
		 *
		 * @note Решено это владельцем, и решено одинаково у всех кодеков: эталоном взят
		 *       порядок, каким его видит JavaScript
		 */
		if(this->_kind == kind_t::MAPPING){
			// Признаки того, что поле сличаемого отображения уже сличено
			vector <bool> matched(value._items.size(), false);
			/**
			 * Выполняем перебор полей отображения
			 */
			for(size_t i = 0; i < this->_items.size(); i++){
				// Признак того, что поле разыскано у сличаемого отображения
				bool found = false;
				/**
				 * Выполняем розыск поля у сличаемого отображения
				 *
				 * @note Розыск ведётся среди полей, ещё не сличённых: отображение, снятое
				 *       с текста, повторные имена нести может, и одно поле сличаемого
				 *       не должно закрывать собою два поля сличающего
				 */
				for(size_t j = 0; j < value._items.size(); j++){
					/**
					 * Если поле сличаемого отображения уже сличено
					 */
					if(matched.at(j))
						// Выполняем переход к следующему полю сличаемого отображения
						continue;
					/**
					 * Если имена полей отображений расходятся
					 */
					if(this->_names.at(i).compare(value._names.at(j)) != 0)
						// Выполняем переход к следующему полю сличаемого отображения
						continue;
					/**
					 * Если значения полей отображений расходятся
					 */
					if(!(this->_items.at(i) == value._items.at(j)))
						// Выполняем переход к следующему полю сличаемого отображения
						continue;
					// Выполняем установку признака сличённого поля
					matched.at(j) = true;
					// Выполняем установку признака разысканного поля
					found = true;
					// Выходим из розыска поля
					break;
				}
				/**
				 * Если поле у сличаемого отображения не разыскано
				 */
				if(!found)
					// Выводим признак несовпадения значений
					return false;
			}
			// Выводим признак совпадения значений
			return true;
		}
		/**
		 * Выполняем перебор значений перечня
		 *
		 * @note Порядок значений перечня сличается: перечень порядком своим и определён,
		 *       и перестановка значений его есть иной перечень
		 */
		for(size_t i = 0; i < this->_items.size(); i++){
			/**
			 * Если значения перечней расходятся
			 */
			if(!(this->_items.at(i) == value._items.at(i)))
				// Выводим признак несовпадения значений
				return false;
		}
		// Выводим признак совпадения значений
		return true;
	}
	/**
	 * Если значение пустым либо неопределённым является
	 *
	 * @note Записи их сличению не подлежат: пустота, записанная пустотою же, и пустота,
	 *       записанная словом `null`, суть одно и то же значение. Сличение записей
	 *       развело бы дерево, снятое с текста, с деревом, собранным из значений языка
	 */
	if((this->_kind == kind_t::NUL) || (this->_kind == kind_t::NONE))
		// Выводим признак совпадения значений
		return true;
	/**
	 * Если значение логическим является
	 *
	 * @note Сличается само значение, а не запись его: `true` и `True` разнятся лишь
	 *       написанием
	 */
	if(this->_kind == kind_t::BOOL)
		// Выводим признак совпадения логических значений
		return ((this->_number.integer != 0) == (value._number.integer != 0));
	/**
	 * Если значение числом является
	 *
	 * @note Сличается само число, а не запись его: `0x1F` и `31` суть одно число,
	 *       записанное двумя способами. Целое сличается с целым, дробное с дробным -
	 *       иначе `1` совпало бы с `1.0`, а виды их разнятся
	 */
	if(this->_kind == kind_t::NUMBER){
		/**
		 * Если оба числа целыми со знаком являются
		 */
		if(this->is(type_t::SIGNED) && value.is(type_t::SIGNED))
			// Выводим признак совпадения целых чисел
			return (this->_number.integer == value._number.integer);
		/**
		 * Если оба числа целыми без знака являются
		 */
		if(this->is(type_t::UNSIGNED) && value.is(type_t::UNSIGNED))
			// Выводим признак совпадения целых чисел
			return (this->_number.natural == value._number.natural);
		/**
		 * Если одно число целым со знаком является, а другое целым без знака
		 *
		 * @details Знаковость есть свойство записи, а не числа: `-0` со знаком читается, а
		 *          `0` без него, число же у них одно. Владеющее значение оформления не
		 *          удерживает, и запись `-0` возвращается круговым ходом записью `0` -
		 *          сличение по знаковости развело бы их
		 *
		 * @note Отрицательное со знаком целому без знака не равно никогда: сличение идёт
		 *       лишь тогда, когда знаковое неотрицательно. Нашёл это ворошитель круговым
		 *       ходом снятого значения на записи `-0`
		 */
		if(this->is(type_t::INT) && value.is(type_t::INT)){
			// Получаем целое со знаком того из чисел, какое знаковым является
			const int64_t integer = (this->is(type_t::SIGNED) ? this->_number.integer : value._number.integer);
			// Получаем целое без знака того из чисел, какое беззнаковым является
			const uint64_t natural = (this->is(type_t::SIGNED) ? value._number.natural : this->_number.natural);
			// Выводим признак совпадения целых чисел разной знаковости
			return ((integer >= 0) && (static_cast <uint64_t> (integer) == natural));
		}
		/**
		 * Если оба числа дробными являются
		 */
		if(this->is(static_cast <type_t> (static_cast <uint32_t> (type_t::REAL) | static_cast <uint32_t> (type_t::EXTENDED))) &&
		   value.is(static_cast <type_t> (static_cast <uint32_t> (type_t::REAL) | static_cast <uint32_t> (type_t::EXTENDED)))){
			/**
			 * Если оба числа нечислами являются
			 *
			 * @details Нечисло не равно даже самому себе - таково правило языка, - и
			 *          сличение обычное объявляло бы неравными два значения `.nan`. Тем
			 *          всякий документ, нечисло несущий, переставал бы равняться сам себе, а
			 *          сличение обязано быть возвратным прежде всего прочего
			 *
			 * @note Нашёл это ворошитель круговым ходом снятого значения на записи `.nan`
			 */
			if(::isnan(this->_number.real) || ::isnan(value._number.real))
				// Выводим признак совпадения нечисел
				return (::isnan(this->_number.real) && ::isnan(value._number.real));
			// Выводим признак совпадения дробных чисел
			return (this->_number.real == value._number.real);
		}
		// Выводим признак совпадения записей чисел
		return (this->_text.compare(value._text) == 0);
	}
	// Выводим признак совпадения записей значений
	return (this->_text.compare(value._text) == 0);
}
/**
 * @brief Метод сличения значений на несовпадение
 *
 * @param value сличаемое значение
 * @return      признак несовпадения значений
 *
 */
bool awh::codec::yaml::Value::operator != (const Value & value) const noexcept {
	// Выводим признак несовпадения значений
	return !((* this) == value);
}
/**
 * @brief Оператор присваивания копированием
 *
 * @param value присваиваемое значение
 * @return      ссылка на текущее значение
 *
 */
awh::codec::yaml::Value & awh::codec::yaml::Value::operator = (const Value & value) noexcept {
	/**
	 * Если присваивается значение само себе
	 */
	if(this == &value)
		// Выводим ссылку на текущее значение
		return (* this);
	// Выполняем копирование вида хранимого значения
	this->_kind = value._kind;
	// Выполняем копирование вида хранения значения
	this->_type = value._type;
	// Выполняем копирование схемы опознания числа
	this->_schema = value._schema;
	// Выполняем копирование разобранного числа
	this->_number = value._number;
	// Выполняем копирование оформления записи значения
	this->_style = value._style;
	// Выполняем копирование правила усечения переводов строк
	this->_chomp = value._chomp;
	// Выполняем копирование построения вместилища
	this->_layout = value._layout;
	// Выполняем копирование содержимого значения
	this->_text = value._text;
	// Выполняем копирование якоря значения
	this->_anchor = value._anchor;
	// Выполняем копирование метки значения
	this->_tag = value._tag;
	// Выполняем копирование имён полей отображения
	this->_names = value._names;
	// Выполняем копирование значений вместилища
	this->_items = value._items;
	// Выводим ссылку на текущее значение
	return (* this);
}
/**
 * @brief Оператор присваивания переносом
 *
 * @param value переносимое значение
 * @return      ссылка на текущее значение
 *
 */
awh::codec::yaml::Value & awh::codec::yaml::Value::operator = (Value && value) noexcept {
	/**
	 * Если присваивается значение само себе
	 */
	if(this == &value)
		// Выводим ссылку на текущее значение
		return (* this);
	// Выполняем перенос вида хранимого значения
	this->_kind = value._kind;
	// Выполняем перенос вида хранения значения
	this->_type = value._type;
	// Выполняем перенос схемы опознания числа
	this->_schema = value._schema;
	// Выполняем перенос разобранного числа
	this->_number = value._number;
	// Выполняем перенос оформления записи значения
	this->_style = value._style;
	// Выполняем перенос правила усечения переводов строк
	this->_chomp = value._chomp;
	// Выполняем перенос построения вместилища
	this->_layout = value._layout;
	// Выполняем перенос содержимого значения
	this->_text = std::move(value._text);
	// Выполняем перенос якоря значения
	this->_anchor = std::move(value._anchor);
	// Выполняем перенос метки значения
	this->_tag = std::move(value._tag);
	// Выполняем перенос имён полей отображения
	this->_names = std::move(value._names);
	// Выполняем перенос значений вместилища
	this->_items = std::move(value._items);
	// Выполняем сброс перенесённого значения
	value.clear();
	// Выводим ссылку на текущее значение
	return (* this);
}
/**
 * @brief Конструктор
 *
 */
awh::codec::yaml::Value::Value() noexcept :
 _kind(kind_t::NONE), _type(type_t::UNDEFINED), _schema(schema_t::CORE),
 _style(style_t::PLAIN), _chomp(chomp_t::KEEP), _layout(layout_t::BLOCK) {}
/**
 * @brief Конструктор вместилища указанного вида
 *
 * @param kind вид заводимого значения
 *
 */
awh::codec::yaml::Value::Value(const kind_t kind) noexcept : Value() {
	// Выполняем установку вида заводимого значения
	this->_kind = kind;
	/**
	 * Определяем вид заводимого значения
	 */
	switch(static_cast <uint8_t> (kind)){
		// Если заводится отображение пар
		case static_cast <uint8_t> (kind_t::MAPPING): this->_type = type_t::MAPPING; break;
		// Если заводится перечень значений
		case static_cast <uint8_t> (kind_t::SEQUENCE): this->_type = type_t::SEQUENCE; break;
		// Если заводится пустое значение
		case static_cast <uint8_t> (kind_t::NUL): this->_type = type_t::NUL; break;
		// Если заводится логическое значение
		case static_cast <uint8_t> (kind_t::BOOL): this->_type = type_t::BOOL; break;
		// Если заводится строковое значение
		case static_cast <uint8_t> (kind_t::STRING): this->_type = type_t::STRING; break;
	}
}
/**
 * @brief Конструктор логического значения
 *
 * @param value заводимое значение
 *
 */
awh::codec::yaml::Value::Value(const bool value) noexcept : Value() {
	// Выполняем установку вида заводимого значения
	this->_kind = kind_t::BOOL;
	// Выполняем установку вида хранения заводимого значения
	this->_type = type_t::BOOL;
	// Выполняем установку записи заводимого значения
	this->_text.assign(value ? "true" : "false");
	// Выполняем установку разобранного логического значения
	this->_number.integer = (value ? 1 : 0);
}
/**
 * @brief Конструктор целого значения со знаком
 *
 * @param value заводимое значение
 *
 */
awh::codec::yaml::Value::Value(const int64_t value) noexcept : Value() {
	// Выполняем установку вида заводимого значения
	this->_kind = kind_t::NUMBER;
	// Выполняем установку записи заводимого значения
	this->_text.assign(std::to_string(value));
	// Выполняем опознание вида числа по записи его
	this->recognize();
}
/**
 * @brief Конструктор целого значения без знака
 *
 * @param value заводимое значение
 *
 */
awh::codec::yaml::Value::Value(const uint64_t value) noexcept : Value() {
	// Выполняем установку вида заводимого значения
	this->_kind = kind_t::NUMBER;
	// Выполняем установку записи заводимого значения
	this->_text.assign(std::to_string(value));
	// Выполняем опознание вида числа по записи его
	this->recognize();
}
/**
 * @brief Конструктор дробного значения
 *
 * @param value заводимое значение
 *
 */
awh::codec::yaml::Value::Value(const double value) noexcept : Value() {
	// Выполняем установку вида заводимого значения
	this->_kind = kind_t::NUMBER;
	/**
	 * Если число числом не является вовсе
	 *
	 * @note Записи эти берутся описанием наречия, а не выдачей библиотеки языка:
	 *       `to_string` выдал бы `nan` и `inf`, а описание требует `.nan` и `.inf`
	 */
	if(::isnan(value))
		// Выполняем установку записи заводимого значения
		this->_text.assign(".nan");
	/**
	 * Если число является бесконечностью
	 */
	else if(::isinf(value))
		// Выполняем установку записи заводимого значения
		this->_text.assign((value < 0.) ? "-.inf" : ".inf");
	/**
	 * Если число записывается обычным порядком
	 */
	else {
		// Собираемая запись дробного числа
		char buffer[64];
		/**
		 * Собираем запись дробного числа
		 *
		 * @note Сборка эта перенята у потока записи дословно: разрядов берётся
		 *       семнадцать, ибо столько нужно, чтобы всякое число двойной точности
		 *       прочлось обратно тем же самым числом. Выдача `to_string` здесь негодна -
		 *       она обрезает число шестью разрядами после точки
		 */
		const int length = ::snprintf(buffer, sizeof(buffer), "%.17g", value);
		/**
		 * Если собрать запись дробного числа удалось
		 */
		if((length > 0) && (static_cast <size_t> (length) < sizeof(buffer)))
			// Выполняем установку записи заводимого значения
			this->_text.assign(buffer, static_cast <size_t> (length));
		/**
		 * Если запись числа дробной части не несёт
		 *
		 * @note Запись `1` схемою прочлась бы целым числом, а записывалось дробное: точка
		 *       с нулём сохраняет вид числа при обратном чтении
		 */
		if(this->_text.find_first_of(".eE") == string::npos)
			// Выполняем добавление дробной части записи числа
			this->_text.append(".0");
	}
	// Выполняем опознание вида числа по записи его
	this->recognize();
}
/**
 * @brief Конструктор строкового значения
 *
 * @param value заводимое значение
 * @param style оформление записи значения
 *
 */
awh::codec::yaml::Value::Value(const string & value, const style_t style) noexcept : Value() {
	// Выполняем установку вида заводимого значения
	this->_kind = kind_t::STRING;
	// Выполняем установку вида хранения заводимого значения
	this->_type = type_t::STRING;
	// Выполняем установку оформления записи значения
	this->_style = style;
	// Выполняем установку записи заводимого значения
	this->_text.assign(value);
	// Выполняем назначение правила усечения переводов строк по содержимому
	this->_chomp = ((!this->_text.empty() && (this->_text.back() == '\n')) ? chomp_t::KEEP : chomp_t::STRIP);
}
/**
 * @brief Конструктор строкового значения
 *
 * @param value заводимое значение
 * @param style оформление записи значения
 *
 */
awh::codec::yaml::Value::Value(const char * value, const style_t style) noexcept :
 Value(string((value != nullptr) ? value : ""), style) {}
/**
 * @brief Конструктор снятия значения со ссылки на узел документа
 *
 * @param value ссылка на узел документа
 *
 */
awh::codec::yaml::Value::Value(const Document::value_t & value) noexcept : Value() {
	// Выполняем снятие значения со ссылки на узел документа
	this->absorb(value);
}
/**
 * @brief Конструктор копирования
 *
 * @param value копируемое значение
 *
 */
awh::codec::yaml::Value::Value(const Value & value) noexcept : Value() {
	// Выполняем копирование значения
	(* this) = value;
}
/**
 * @brief Конструктор переноса
 *
 * @param value переносимое значение
 *
 */
awh::codec::yaml::Value::Value(Value && value) noexcept : Value() {
	// Выполняем перенос значения
	(* this) = std::move(value);
}
/**
 * @brief Метод получения вместилища, сборкой открытого
 *
 * @return ссылка на открытое вместилище
 *
 */
awh::codec::yaml::Value & awh::codec::yaml::Builder::opened() noexcept {
	// Получаем ссылку на вместилище, сборкой открытое
	Value * result = &this->_root;
	/**
	 * Выполняем перебор номеров пути к открытому вместилищу
	 */
	for(auto & index : this->_path)
		// Выполняем переход к вложенному вместилищу
		result = &((* result)[index]);
	// Выводим ссылку на открытое вместилище
	return (* result);
}
/**
 * @brief Метод занесения собранного значения во вместилище
 *
 * @param value заносимое значение
 * @return      номер занесённого значения во вместилище
 *
 */
size_t awh::codec::yaml::Builder::deposit(Value && value) noexcept {
	/**
	 * Если значению предпослан якорь
	 */
	if(!this->_anchor.empty()){
		// Выполняем установку якоря, значению предпосланного
		value.anchor(this->_anchor);
		// Выполняем сброс якоря, значению предпосланного
		this->_anchor.clear();
	}
	/**
	 * Если значению предпослана метка
	 */
	if(!this->_tag.empty()){
		// Выполняем установку метки, значению предпосланной
		value.tag(this->_tag);
		// Выполняем сброс метки, значению предпосланной
		this->_tag.clear();
	}
	// Получаем ссылку на вместилище, сборкой открытое
	Value & owner = this->opened();
	/**
	 * Если вместилище является отображением пар
	 */
	if(owner.kind() == kind_t::MAPPING){
		// Получаем имя заносимого поля отображения
		const string name(this->_key);
		// Получаем признак того, что поле кладётся рядом с одноимённым
		const bool appended = this->_appended;
		// Выполняем сброс имени поля отображения
		this->_keyed = false;
		// Выполняем сброс признака добавления поля рядом с одноимённым
		this->_appended = false;
		// Выполняем очистку имени поля отображения
		this->_key.clear();
		/**
		 * Если поле кладётся рядом с одноимённым
		 *
		 * @note Номер поля берётся последним, а не розыском по имени: розыск отдал бы
		 *       вхождение первое, и вместилище, следом открытое, собиралось бы не туда.
		 *       Ловушка эта на скалярах не видна вовсе - вылезает лишь тогда, когда за
		 *       добавлением открывается отображение либо перечень
		 */
		if(appended){
			// Выполняем добавление поля отображения рядом с одноимённым
			owner.append(name, value);
			// Выводим номер добавленного поля отображения
			return (owner.size() > 0 ? (owner.size() - 1) : 0);
		}
		// Выполняем занесение поля отображения
		owner.insert(name, value);
		/**
		 * Выполняем розыск номера занесённого поля отображения
		 */
		for(size_t i = 0; i < owner.size(); i++){
			/**
			 * Если имя поля отображения совпадает с занесённым
			 */
			if(owner.key(i).compare(name) == 0)
				// Выводим номер занесённого поля отображения
				return i;
		}
		// Выводим номер занесённого поля отображения
		return (owner.size() > 0 ? (owner.size() - 1) : 0);
	}
	// Выполняем занесение значения в конец перечня
	owner.push(value);
	// Выводим номер занесённого значения перечня
	return (owner.size() > 0 ? (owner.size() - 1) : 0);
}
/**
 * @brief Метод открытия вместилища указанного вида
 *
 * @param value открываемое вместилище
 * @return      признак успешности открытия
 *
 */
bool awh::codec::yaml::Builder::expand(Value && value) noexcept {
	/**
	 * Если сборка завершена
	 */
	if(this->_done)
		// Выводим признак неудачного открытия
		return false;
	/**
	 * Если вместилище открывается корнем собираемого значения
	 */
	if(this->_path.empty() && (this->_root.kind() != kind_t::MAPPING) && (this->_root.kind() != kind_t::SEQUENCE)){
		/**
		 * Если корню предпослан якорь
		 */
		if(!this->_anchor.empty()){
			// Выполняем установку якоря, корню предпосланного
			value.anchor(this->_anchor);
			// Выполняем сброс якоря, корню предпосланного
			this->_anchor.clear();
		}
		/**
		 * Если корню предпослана метка
		 */
		if(!this->_tag.empty()){
			// Выполняем установку метки, корню предпосланной
			value.tag(this->_tag);
			// Выполняем сброс метки, корню предпосланной
			this->_tag.clear();
		}
		// Выполняем установку корня собираемого значения
		this->_root = std::move(value);
		// Выводим признак успешного открытия
		return true;
	}
	// Получаем ссылку на вместилище, сборкой открытое
	Value & owner = this->opened();
	/**
	 * Если вместилище открытое отображением пар является, а имя поля не назначено
	 *
	 * @note Отказ здесь обнаруживает промах потребителя: поле отображения без имени
	 *       записано быть не может, а имя, номером подставленное, спрятало бы промах
	 *       и выдало бы текст, потребителем не задуманный
	 */
	if((owner.kind() == kind_t::MAPPING) && !this->_keyed)
		// Выводим признак неудачного занесения
		return false;
	// Выполняем занесение открываемого вместилища
	const size_t index = this->deposit(std::move(value));
	// Выполняем переход внутрь открытого вместилища
	this->_path.push_back(index);
	// Выводим признак успешного открытия
	return true;
}
/**
 * @brief Метод открытия отображения пар
 *
 * @param layout построение открываемого отображения
 * @return       признак успешности открытия
 *
 */
bool awh::codec::yaml::Builder::mapping(const layout_t layout) noexcept {
	// Выполняем заведение открываемого отображения пар
	Value value(kind_t::MAPPING);
	// Выполняем установку построения открываемого отображения
	value.layout(layout);
	// Выводим признак успешности открытия отображения пар
	return this->expand(std::move(value));
}
/**
 * @brief Метод открытия перечня значений
 *
 * @param layout построение открываемого перечня
 * @return       признак успешности открытия
 *
 */
bool awh::codec::yaml::Builder::sequence(const layout_t layout) noexcept {
	// Выполняем заведение открываемого перечня значений
	Value value(kind_t::SEQUENCE);
	// Выполняем установку построения открываемого перечня
	value.layout(layout);
	// Выводим признак успешности открытия перечня значений
	return this->expand(std::move(value));
}
/**
 * @brief Метод закрытия открытого вместилища
 *
 * @return признак успешности закрытия
 *
 */
bool awh::codec::yaml::Builder::close() noexcept {
	/**
	 * Если сборка завершена
	 */
	if(this->_done)
		// Выводим признак неудачного закрытия
		return false;
	/**
	 * Если закрывается вместилище вложенное
	 */
	if(!this->_path.empty()){
		// Выполняем выход из закрываемого вместилища
		this->_path.pop_back();
		// Выводим признак успешного закрытия
		return true;
	}
	/**
	 * Если корень собираемого значения вместилищем является
	 */
	if((this->_root.kind() == kind_t::MAPPING) || (this->_root.kind() == kind_t::SEQUENCE)){
		// Выполняем завершение сборки
		this->_done = true;
		// Выводим признак успешного закрытия
		return true;
	}
	// Выводим признак неудачного закрытия
	return false;
}
/**
 * @brief Метод назначения имени поля отображения
 *
 * @param name имя назначаемого поля отображения
 * @return     признак успешности назначения
 *
 */
bool awh::codec::yaml::Builder::key(const string & name) noexcept {
	/**
	 * Если сборка завершена
	 */
	if(this->_done)
		// Выводим признак неудачного назначения
		return false;
	/**
	 * Если имя поля отображения назначено уже
	 *
	 * @note Отказ здесь обнаруживает промах потребителя: имя, назначенное дважды подряд,
	 *       означает потерянное значение, и молчаливая замена первого имени вторым
	 *       спрятала бы потерю
	 */
	if(this->_keyed)
		// Выводим признак неудачного назначения
		return false;
	// Выполняем установку имени поля отображения
	this->_key.assign(name);
	// Выполняем установку признака назначенного имени поля
	this->_keyed = true;
	/**
	 * Выполняем сброс признака добавления поля рядом с одноимённым
	 *
	 * @note Признак принадлежит имени, а не сборке: без сброса одно добавление сделало
	 *       бы добавлениями все поля до конца сборки, и перезапись поля перестала бы
	 *       работать после первого же повтора
	 */
	this->_appended = false;
	// Выводим признак успешного назначения
	return true;
}
/**
 * @brief Метод назначения имени поля, рядом с одноимённым кладомого
 *
 * @param name имя назначаемого поля отображения
 * @return     признак успешности назначения
 *
 */
bool awh::codec::yaml::Builder::append(const string & name) noexcept {
	/**
	 * Если назначить имя поля отображения не удалось
	 */
	if(!this->key(name))
		// Выводим признак неудачного назначения
		return false;
	// Выполняем установку признака добавления поля рядом с одноимённым
	this->_appended = true;
	// Выводим признак успешного назначения
	return true;
}
/**
 * @brief Метод назначения якоря, значению предпосылаемого
 *
 * @param name имя назначаемого якоря
 * @return     признак успешности назначения
 *
 */
bool awh::codec::yaml::Builder::anchor(const string & name) noexcept {
	/**
	 * Если сборка завершена
	 */
	if(this->_done)
		// Выводим признак неудачного назначения
		return false;
	// Выполняем установку якоря, значению предпосылаемого
	this->_anchor.assign(name);
	// Выводим признак успешного назначения
	return true;
}
/**
 * @brief Метод назначения метки, значению предпосылаемой
 *
 * @param name имя назначаемой метки
 * @return     признак успешности назначения
 *
 */
bool awh::codec::yaml::Builder::tag(const string & name) noexcept {
	/**
	 * Если сборка завершена
	 */
	if(this->_done)
		// Выводим признак неудачного назначения
		return false;
	// Выполняем установку метки, значению предпосылаемой
	this->_tag.assign(name);
	// Выводим признак успешного назначения
	return true;
}
/**
 * @brief Метод записи готового значения
 *
 * @param value записываемое значение
 * @return      признак успешности записи
 *
 */
bool awh::codec::yaml::Builder::value(const Value & value) noexcept {
	/**
	 * Если сборка завершена
	 */
	if(this->_done)
		// Выводим признак неудачной записи
		return false;
	/**
	 * Если значение записывается корнем собираемого
	 *
	 * @note Значение простое корнем сборку и завершает: вместилища, куда заносить
	 *       следующее, у неё нет вовсе
	 */
	if(this->_path.empty() && (this->_root.kind() != kind_t::MAPPING) && (this->_root.kind() != kind_t::SEQUENCE)){
		// Выполняем заведение записываемого значения
		Value result(value);
		/**
		 * Если корню предпослан якорь
		 */
		if(!this->_anchor.empty()){
			// Выполняем установку якоря, корню предпосланного
			result.anchor(this->_anchor);
			// Выполняем сброс якоря, корню предпосланного
			this->_anchor.clear();
		}
		/**
		 * Если корню предпослана метка
		 */
		if(!this->_tag.empty()){
			// Выполняем установку метки, корню предпосланной
			result.tag(this->_tag);
			// Выполняем сброс метки, корню предпосланной
			this->_tag.clear();
		}
		// Выполняем установку корня собираемого значения
		this->_root = std::move(result);
		// Выполняем завершение сборки
		this->_done = true;
		// Выводим признак успешной записи
		return true;
	}
	// Получаем ссылку на вместилище, сборкой открытое
	Value & owner = this->opened();
	/**
	 * Если вместилище открытое отображением пар является, а имя поля не назначено
	 *
	 * @note Отказ здесь обнаруживает промах потребителя: поле отображения без имени
	 *       записано быть не может, а имя, номером подставленное, спрятало бы промах
	 *       и выдало бы текст, потребителем не задуманный
	 */
	if((owner.kind() == kind_t::MAPPING) && !this->_keyed)
		// Выводим признак неудачного занесения
		return false;
	// Выполняем занесение записываемого значения
	this->deposit(Value(value));
	// Выводим признак успешной записи
	return true;
}
/**
 * @brief Метод записи пустого значения
 *
 * @return признак успешности записи
 *
 */
bool awh::codec::yaml::Builder::null() noexcept {
	// Выводим признак успешности записи пустого значения
	return this->value(Value(kind_t::NUL));
}
/**
 * @brief Метод записи логического значения
 *
 * @param value записываемое значение
 * @return      признак успешности записи
 *
 */
bool awh::codec::yaml::Builder::value(const bool value) noexcept {
	// Выводим признак успешности записи логического значения
	return this->value(Value(value));
}
/**
 * @brief Метод записи целого числа со знаком
 *
 * @param value записываемое значение
 * @return      признак успешности записи
 *
 */
bool awh::codec::yaml::Builder::value(const int64_t value) noexcept {
	// Выводим признак успешности записи целого числа
	return this->value(Value(value));
}
/**
 * @brief Метод записи целого числа без знака
 *
 * @param value записываемое значение
 * @return      признак успешности записи
 *
 */
bool awh::codec::yaml::Builder::value(const uint64_t value) noexcept {
	// Выводим признак успешности записи целого числа
	return this->value(Value(value));
}
/**
 * @brief Метод записи дробного числа
 *
 * @param value записываемое значение
 * @return      признак успешности записи
 *
 */
bool awh::codec::yaml::Builder::value(const double value) noexcept {
	// Выводим признак успешности записи дробного числа
	return this->value(Value(value));
}
/**
 * @brief Метод записи строкового значения
 *
 * @param value записываемое значение
 * @param style оформление записи значения
 * @return      признак успешности записи
 *
 */
bool awh::codec::yaml::Builder::value(const string & value, const style_t style) noexcept {
	// Выводим признак успешности записи строкового значения
	return this->value(Value(value, style));
}
/**
 * @brief Метод записи строкового значения
 *
 * @param value записываемое значение
 * @param style оформление записи значения
 * @return      признак успешности записи
 *
 */
bool awh::codec::yaml::Builder::value(const char * value, const style_t style) noexcept {
	// Выводим признак успешности записи строкового значения
	return this->value(Value(value, style));
}
/**
 * @brief Метод извлечения глубины открытых вместилищ
 *
 * @return количество вместилищ, сборкой открытых и не закрытых
 *
 */
size_t awh::codec::yaml::Builder::depth() const noexcept {
	/**
	 * Если сборка завершена либо корень вместилищем не является
	 */
	if(this->_done || ((this->_root.kind() != kind_t::MAPPING) && (this->_root.kind() != kind_t::SEQUENCE)))
		// Выводим отсутствие открытых вместилищ
		return 0;
	// Выводим количество открытых вместилищ вместе с корневым
	return (this->_path.size() + 1);
}
/**
 * @brief Метод сброса сборки
 *
 */
void awh::codec::yaml::Builder::reset() noexcept {
	// Выполняем очистку собираемого значения
	this->_root.clear();
	// Выполняем очистку пути к открытому вместилищу
	this->_path.clear();
	// Выполняем очистку имени поля отображения
	this->_key.clear();
	// Выполняем очистку якоря, значению предпосылаемого
	this->_anchor.clear();
	// Выполняем очистку метки, значению предпосылаемой
	this->_tag.clear();
	// Выполняем сброс признака назначенного имени поля
	this->_keyed = false;
	/**
	 * Выполняем сброс признака добавления поля рядом с одноимённым
	 *
	 * @note Без сброса признак пережил бы изъятие собранного значения и лёг бы на сборку
	 *       следующую
	 */
	this->_appended = false;
	// Выполняем сброс признака завершённой сборки
	this->_done = false;
}
/**
 * @brief Метод изъятия собранного значения
 *
 * @return собранное значение
 *
 */
awh::codec::yaml::Value awh::codec::yaml::Builder::finish() noexcept {
	// Получаем собранное значение
	Value result(std::move(this->_root));
	// Выполняем сброс сборки
	this->reset();
	// Выводим собранное значение
	return result;
}
/**
 * @brief Конструктор
 *
 */
awh::codec::yaml::Builder::Builder() noexcept : _keyed(false), _appended(false), _done(false) {}
