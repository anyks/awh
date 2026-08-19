/**
 * @file value.cpp
 * @date 2026-08-19
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
 * @brief Файл реализации владеющего значения бинарного контейнера ABC
 *
 * \~english
 * @brief Implementation file of the owning value of the ABC binary container
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл модуля
 */
#include <codec/abc/value.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <atomic>
#include <cstring>
#include <limits>
#include <utility>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Пространство имён работ, доступных лишь этому файлу
 *
 */
namespace {
	/**
	 * @brief Предел роста вместимого обращением по номеру
	 *
	 * @details Предел ставится единожды при заведении приложения, а читается всяким
	 *          потоком, оттого согласование порядка обращений здесь и не нужно
	 *
	 */
	std::atomic <size_t> Limit(0x10000);
};

/**
 * @brief Метод извлечения предела роста вместимого
 *
 * @return предел роста вместимого
 *
 */
size_t awh::codec::abc::Value::limit() noexcept {
	// Выводим предел роста вместимого
	return Limit.load(std::memory_order_relaxed);
}
/**
 * @brief Метод установки предела роста вместимого
 *
 * @param value устанавливаемый предел, ноль - без предела
 *
 */
void awh::codec::abc::Value::limit(const size_t value) noexcept {
	// Выполняем установку предела роста вместимого
	Limit.store(value, std::memory_order_relaxed);
}
/**
 * @brief Метод извлечения ссылки на отсутствующее значение
 *
 * @return ссылка на отсутствующее значение
 *
 */
const awh::codec::abc::Value & awh::codec::abc::Value::undefined() noexcept {
	// Отсутствующее значение, общее на весь процесс
	static const Value result;
	// Выводим ссылку на отсутствующее значение
	return result;
}
/**
 * @brief Метод извлечения ссылки на отбросное значение
 *
 * @return ссылка на отбросное значение
 *
 */
awh::codec::abc::Value & awh::codec::abc::Value::scrap() noexcept {
	// Отбросное значение, общее на весь процесс
	static Value result;
	// Выполняем очистку отбросного значения
	result.clear();
	// Выводим ссылку на отбросное значение
	return result;
}
/**
 * @brief Метод разбора звена пути на номер значения
 *
 * @param segment разбираемое звено пути
 * @param result  разобранный номер значения
 * @return        признак того, что звено является номером
 *
 */
bool awh::codec::abc::Value::indexed(const string_view segment, size_t & result) noexcept {
	// Выполняем сброс разобранного номера значения
	result = 0;
	// Если звено пути пусто
	if(segment.empty())
		// Сообщаем, что звено номером не является
		return false;
	/**
	 * Если запись номера имеет ведущий нуль, номером она не является, а является
	 * именем поля. Правило это взято у RFC 6901: без него `01` и `1` означали бы
	 * одно и то же, и путь перестал бы задавать значение однозначно
	 */
	if((segment.size() > 1) && (segment.front() == '0'))
		// Сообщаем, что звено номером не является
		return false;
	/**
	 * Выполняем перебор всех знаков звена пути
	 */
	for(const char letter : segment){
		// Если знак цифрой не является
		if((letter < '0') || (letter > '9')){
			// Выполняем сброс разобранного номера значения
			result = 0;
			// Сообщаем, что звено номером не является
			return false;
		}
		// Выполняем получение разряда номера значения
		const size_t digit = static_cast <size_t> (letter - '0');
		/**
		 * Если накопление разряда переполнило бы номер, отвергаем запись целиком.
		 * Приведение её к пределу выдало бы значение с иным номером, а вместимого
		 * такой длины не бывает вовсе
		 */
		if(result > ((numeric_limits <size_t>::max() - digit) / 10)){
			// Выполняем сброс разобранного номера значения
			result = 0;
			// Сообщаем, что звено номером не является
			return false;
		}
		// Выполняем накопление разряда номера значения
		result = ((result * 10) + digit);
	}
	// Сообщаем, что звено является номером
	return true;
}
/**
 * @brief Конструктор
 *
 */
awh::codec::abc::Value::Value() noexcept :
 _kind(kind_t::NONE), _type(type_t::UNDEFINED), _exponent(0), _negative(false) {
	// Выполняем сброс числа значения
	this->_number.natural = 0;
}
/**
 * @brief Конструктор
 *
 * @param kind вид узла заводимого значения
 *
 */
awh::codec::abc::Value::Value(const kind_t kind) noexcept :
 _kind(kind), _type(type_t::UNDEFINED), _exponent(0), _negative(false) {
	// Выполняем сброс числа значения
	this->_number.natural = 0;
	/**
	 * Определяем вид узла заводимого значения
	 */
	switch(static_cast <uint8_t> (kind)){
		// Если заводится пустое значение
		case static_cast <uint8_t> (kind_t::NUL): this->_type = type_t::NUL; break;
		// Если заводится логическое значение
		case static_cast <uint8_t> (kind_t::BOOL): this->_type = type_t::BOOL; break;
		// Если заводится число
		case static_cast <uint8_t> (kind_t::NUMBER): this->_type = type_t::UINT64; break;
		// Если заводится строка
		case static_cast <uint8_t> (kind_t::STRING): this->_type = type_t::STRING; break;
		// Если заводятся двоичные данные
		case static_cast <uint8_t> (kind_t::BLOB): this->_type = type_t::BLOB; break;
		// Если заводится отметка времени
		case static_cast <uint8_t> (kind_t::TIME): this->_type = type_t::TIME; break;
		// Если заводится опознаватель
		case static_cast <uint8_t> (kind_t::UUID): this->_type = type_t::UUID; break;
		// Если заводится массив
		case static_cast <uint8_t> (kind_t::ARRAY): this->_type = type_t::ARRAY; break;
		// Если заводится отображение
		case static_cast <uint8_t> (kind_t::MAP): this->_type = type_t::MAP; break;
	}
}
/**
 * @brief Конструктор
 *
 * @param value заводимое логическое значение
 *
 */
awh::codec::abc::Value::Value(const bool value) noexcept :
 _kind(kind_t::BOOL), _type(type_t::BOOL), _exponent(0), _negative(false) {
	// Выполняем установку логического значения
	this->_number.flag = value;
}
/**
 * @brief Конструктор
 *
 * @param value заводимое целое со знаком
 *
 */
awh::codec::abc::Value::Value(const int64_t value) noexcept :
 _kind(kind_t::NUMBER), _type(type_t::INT64), _exponent(0), _negative(false) {
	// Выполняем установку целого со знаком
	this->_number.integer = value;
}
/**
 * @brief Конструктор
 *
 * @param value заводимое целое без знака
 *
 */
awh::codec::abc::Value::Value(const uint64_t value) noexcept :
 _kind(kind_t::NUMBER), _type(type_t::UINT64), _exponent(0), _negative(false) {
	// Выполняем установку целого без знака
	this->_number.natural = value;
}
/**
 * @brief Конструктор
 *
 * @param value заводимое дробное число
 *
 */
awh::codec::abc::Value::Value(const double value) noexcept :
 _kind(kind_t::NUMBER), _type(type_t::DOUBLE), _exponent(0), _negative(false) {
	// Выполняем установку дробного числа
	this->_number.real = value;
}
/**
 * @brief Конструктор
 *
 * @param value заводимая строка
 *
 */
awh::codec::abc::Value::Value(const string & value) noexcept :
 _kind(kind_t::STRING), _type(type_t::STRING), _text(value), _exponent(0), _negative(false) {
	// Выполняем сброс числа значения
	this->_number.natural = 0;
}
/**
 * @brief Конструктор
 *
 * @param value заводимая строка
 *
 */
awh::codec::abc::Value::Value(const char * value) noexcept :
 _kind(kind_t::STRING), _type(type_t::STRING), _exponent(0), _negative(false) {
	// Выполняем сброс числа значения
	this->_number.natural = 0;
	// Если заводимая строка существует
	if(value != nullptr)
		// Выполняем установку заводимой строки
		this->_text.assign(value);
}
/**
 * @brief Конструктор
 *
 * @param value переносимое значение дерева документа
 *
 */
awh::codec::abc::Value::Value(const Document::value_t & value) noexcept :
 _kind(kind_t::NONE), _type(type_t::UNDEFINED), _exponent(0), _negative(false) {
	// Выполняем сброс числа значения
	this->_number.natural = 0;
	// Выполняем перенесение значения из дерева документа
	this->absorb(value);
}
/**
 * @brief Конструктор копирования
 *
 * @param value копируемое значение
 *
 */
awh::codec::abc::Value::Value(const Value & value) noexcept :
 _kind(value._kind), _type(value._type), _number(value._number), _text(value._text),
 _exponent(value._exponent), _negative(value._negative), _keys(value._keys), _items(value._items) {}
/**
 * @brief Конструктор переноса
 *
 * @param value переносимое значение
 *
 */
awh::codec::abc::Value::Value(Value && value) noexcept :
 _kind(value._kind), _type(value._type), _number(value._number), _text(std::move(value._text)),
 _exponent(value._exponent), _negative(value._negative), _keys(std::move(value._keys)),
 _items(std::move(value._items)) {
	// Выполняем сброс вида узла перенесённого значения
	value._kind = kind_t::NONE;
	// Выполняем сброс вида перенесённого значения
	value._type = type_t::UNDEFINED;
}
/**
 * @brief Оператор присваивания копированием
 *
 * @param value копируемое значение
 * @return      ссылка на присвоенное значение
 *
 */
awh::codec::abc::Value & awh::codec::abc::Value::operator = (const Value & value) noexcept {
	// Если присваивается то же самое значение
	if(this == &value)
		// Выводим ссылку на присвоенное значение
		return (* this);
	// Выполняем очистку присваиваемого значения
	this->clear();
	// Выполняем установку вида узла значения
	this->_kind = value._kind;
	// Выполняем установку вида значения
	this->_type = value._type;
	// Выполняем установку числа значения
	this->_number = value._number;
	// Выполняем установку содержимого значения
	this->_text = value._text;
	// Выполняем установку десятичного порядка величины
	this->_exponent = value._exponent;
	// Выполняем установку признака отрицательности величины
	this->_negative = value._negative;
	// Выполняем установку имён полей отображения
	this->_keys = value._keys;
	// Выполняем установку значений вместимого
	this->_items = value._items;
	// Выводим ссылку на присвоенное значение
	return (* this);
}
/**
 * @brief Оператор присваивания переносом
 *
 * @param value переносимое значение
 * @return      ссылка на присвоенное значение
 *
 */
awh::codec::abc::Value & awh::codec::abc::Value::operator = (Value && value) noexcept {
	// Если присваивается то же самое значение
	if(this == &value)
		// Выводим ссылку на присвоенное значение
		return (* this);
	// Выполняем очистку присваиваемого значения
	this->clear();
	// Выполняем установку вида узла значения
	this->_kind = value._kind;
	// Выполняем установку вида значения
	this->_type = value._type;
	// Выполняем установку числа значения
	this->_number = value._number;
	// Выполняем перенесение содержимого значения
	this->_text = std::move(value._text);
	// Выполняем установку десятичного порядка величины
	this->_exponent = value._exponent;
	// Выполняем установку признака отрицательности величины
	this->_negative = value._negative;
	// Выполняем перенесение имён полей отображения
	this->_keys = std::move(value._keys);
	// Выполняем перенесение значений вместимого
	this->_items = std::move(value._items);
	// Выполняем сброс вида узла перенесённого значения
	value._kind = kind_t::NONE;
	// Выполняем сброс вида перенесённого значения
	value._type = type_t::UNDEFINED;
	// Выводим ссылку на присвоенное значение
	return (* this);
}
/**
 * @brief Деструктор
 *
 */
awh::codec::abc::Value::~Value() noexcept {
	// Выполняем очистку значения без возвратности
	this->clear();
}
/**
 * @brief Метод очистки значения
 *
 */
void awh::codec::abc::Value::clear() noexcept {
	/**
	 * Если детей у значения нет, разбирать нечего
	 */
	if(!this->_items.empty() || !this->_keys.empty()){
		// Вместилище значений, ожидающих разрушения
		vector <Value> pending;
		// Выполняем перенесение значений вместимого в ожидающие разрушения
		for(Value & item : this->_items)
			// Выполняем перенесение очередного значения вместимого
			pending.push_back(std::move(item));
		// Выполняем перенесение имён полей отображения в ожидающие разрушения
		for(Value & key : this->_keys)
			// Выполняем перенесение очередного имени поля отображения
			pending.push_back(std::move(key));
		// Выполняем очистку значений вместимого
		this->_items.clear();
		// Выполняем очистку имён полей отображения
		this->_keys.clear();
		/**
		 * Выполняем разрушение всех ожидающих значений.
		 *
		 * Разрушение ведётся обходом, а не возвратностью: дерево, собранное вручную,
		 * предела глубины не имеет вовсе, и возвратное разрушение сорвало бы стек уже
		 * на десятках тысяч уровней
		 */
		while(!pending.empty()){
			// Выполняем снятие очередного значения с вместилища ожидающих
			Value node = std::move(pending.back());
			// Выполняем удаление снятого значения из вместилища ожидающих
			pending.pop_back();
			// Выполняем перенесение детей снятого значения в ожидающие разрушения
			for(Value & item : node._items)
				// Выполняем перенесение очередного значения вместимого
				pending.push_back(std::move(item));
			// Выполняем перенесение имён полей снятого значения в ожидающие разрушения
			for(Value & key : node._keys)
				// Выполняем перенесение очередного имени поля отображения
				pending.push_back(std::move(key));
			// Выполняем очистку значений вместимого снятого значения
			node._items.clear();
			// Выполняем очистку имён полей снятого значения
			node._keys.clear();
		}
	}
	// Выполняем сброс вида узла значения
	this->_kind = kind_t::NONE;
	// Выполняем сброс вида значения
	this->_type = type_t::UNDEFINED;
	// Выполняем сброс числа значения
	this->_number.natural = 0;
	// Выполняем очистку содержимого значения
	this->_text.clear();
	// Выполняем сброс десятичного порядка величины
	this->_exponent = 0;
	// Выполняем сброс признака отрицательности величины
	this->_negative = false;
}
/**
 * @brief Метод проверки действительности значения
 *
 * @return признак действительности значения
 *
 */
bool awh::codec::abc::Value::valid() const noexcept {
	// Выводим признак действительности значения
	return (this->_type != type_t::UNDEFINED);
}
/**
 * @brief Метод извлечения вида узла значения
 *
 * @return вид узла значения
 *
 */
awh::codec::abc::kind_t awh::codec::abc::Value::kind() const noexcept {
	// Выводим вид узла значения
	return this->_kind;
}
/**
 * @brief Метод извлечения вида значения
 *
 * @return вид значения
 *
 */
awh::codec::abc::type_t awh::codec::abc::Value::type() const noexcept {
	// Выводим вид значения
	return this->_type;
}
/**
 * @brief Метод проверки принадлежности значения к виду
 *
 * @param type вид значения, сборный либо точный
 * @return     признак принадлежности значения к виду
 *
 */
bool awh::codec::abc::Value::is(const type_t type) const noexcept {
	// Выводим признак принадлежности значения к виду
	return ((static_cast <uint32_t> (this->_type) & static_cast <uint32_t> (type)) != 0);
}
/**
 * @brief Метод извлечения количества значений вместимого
 *
 * @return количество значений вместимого
 *
 */
size_t awh::codec::abc::Value::size() const noexcept {
	// Выводим количество значений вместимого
	return this->_items.size();
}
/**
 * @brief Метод проверки пустоты значения
 *
 * @return признак пустоты значения
 *
 */
bool awh::codec::abc::Value::empty() const noexcept {
	// Если значение является вместимым
	if(this->is(type_t::CONTAINER))
		// Выводим признак пустоты вместимого
		return this->_items.empty();
	// Если значение хранится отрезком октетов
	if(this->is(type_t::SEGMENT) || (this->_type == type_t::UUID))
		// Выводим признак пустоты содержимого
		return this->_text.empty();
	// Выводим признак недействительности значения
	return !this->valid();
}
/**
 * @brief Метод извлечения содержимого значения
 *
 * @return содержимое значения
 *
 */
const string & awh::codec::abc::Value::text() const noexcept {
	// Выводим содержимое значения
	return this->_text;
}
/**
 * @brief Метод извлечения десятичного порядка величины
 *
 * @return десятичный порядок величины
 *
 */
int64_t awh::codec::abc::Value::exponent() const noexcept {
	// Выводим десятичный порядок величины
	return this->_exponent;
}
/**
 * @brief Метод проверки того, что величина меньше нуля
 *
 * @return признак того, что величина меньше нуля
 *
 */
bool awh::codec::abc::Value::negative() const noexcept {
	// Выводим признак того, что величина меньше нуля
	return this->_negative;
}
/**
 * @brief Метод извлечения имени поля отображения по его номеру
 *
 * @param index номер пары отображения
 * @return      имя поля отображения
 *
 */
const awh::codec::abc::Value & awh::codec::abc::Value::key(const size_t index) const noexcept {
	// Если затребованного имени поля отображения нет
	if(index >= this->_keys.size())
		// Выводим ссылку на отсутствующее значение
		return Value::undefined();
	// Выводим имя затребованного поля отображения
	return this->_keys.at(index);
}
/**
 * @brief Метод проверки наличия поля отображения по имени
 *
 * @param name имя поля отображения
 * @return     признак наличия поля отображения
 *
 */
bool awh::codec::abc::Value::contains(const string & name) const noexcept {
	// Если значение отображением не является
	if(this->_type != type_t::MAP)
		// Сообщаем, что поля отображения нет
		return false;
	/**
	 * Выполняем перебор всех имён полей отображения
	 */
	for(const Value & key : this->_keys){
		// Если имя поля отображения совпало с затребованным
		if((key._type == type_t::STRING) && (key._text.compare(name) == 0))
			// Сообщаем, что поле отображения найдено
			return true;
	}
	// Сообщаем, что поля отображения нет
	return false;
}
/**
 * @brief Оператор извлечения значения поля отображения по имени
 *
 * @param name имя поля отображения
 * @return     ссылка на значение поля отображения
 *
 */
const awh::codec::abc::Value & awh::codec::abc::Value::operator [] (const string & name) const noexcept {
	// Если значение отображением не является
	if(this->_type != type_t::MAP)
		// Выводим ссылку на отсутствующее значение
		return Value::undefined();
	/**
	 * Выполняем перебор всех имён полей отображения
	 */
	for(size_t i = 0; i < this->_keys.size(); i++){
		// Выполняем получение очередного имени поля отображения
		const Value & key = this->_keys.at(i);
		// Если имя поля отображения совпало с затребованным
		if((key._type == type_t::STRING) && (key._text.compare(name) == 0))
			// Выводим значение затребованного поля отображения
			return this->_items.at(i);
	}
	// Выводим ссылку на отсутствующее значение
	return Value::undefined();
}
/**
 * @brief Оператор заведения значения поля отображения по имени
 *
 * @param name имя поля отображения
 * @return     ссылка на значение поля отображения
 *
 */
awh::codec::abc::Value & awh::codec::abc::Value::operator [] (const string & name) noexcept {
	// Если значение вместимым иного вида является
	if((this->_type != type_t::MAP) && this->valid() && (this->_type != type_t::NUL))
		// Выводим ссылку на отбросное значение
		return Value::scrap();
	// Если значение отображением ещё не является
	if(this->_type != type_t::MAP){
		// Выполняем установку вида узла отображения
		this->_kind = kind_t::MAP;
		// Выполняем установку вида отображения
		this->_type = type_t::MAP;
	}
	/**
	 * Выполняем перебор всех имён полей отображения
	 */
	for(size_t i = 0; i < this->_keys.size(); i++){
		// Выполняем получение очередного имени поля отображения
		const Value & key = this->_keys.at(i);
		// Если имя поля отображения совпало с затребованным
		if((key._type == type_t::STRING) && (key._text.compare(name) == 0))
			// Выводим значение затребованного поля отображения
			return this->_items.at(i);
	}
	// Выполняем заведение имени затребованного поля отображения
	this->_keys.push_back(Value(name));
	// Выполняем заведение значения затребованного поля отображения
	this->_items.push_back(Value());
	// Выводим значение заведённого поля отображения
	return this->_items.back();
}
/**
 * @brief Оператор извлечения значения вместимого по номеру
 *
 * @param index номер значения вместимого
 * @return      ссылка на значение вместимого
 *
 */
const awh::codec::abc::Value & awh::codec::abc::Value::operator [] (const size_t index) const noexcept {
	// Если затребованного значения вместимого нет
	if(index >= this->_items.size())
		// Выводим ссылку на отсутствующее значение
		return Value::undefined();
	// Выводим затребованное значение вместимого
	return this->_items.at(index);
}
/**
 * @brief Оператор заведения значения вместимого по номеру
 *
 * @param index номер значения вместимого
 * @return      ссылка на значение вместимого
 *
 */
awh::codec::abc::Value & awh::codec::abc::Value::operator [] (const size_t index) noexcept {
	// Если затребованное значение вместимого уже заведено
	if(index < this->_items.size())
		// Выводим затребованное значение вместимого
		return this->_items.at(index);
	// Если значение вместимым иного вида является
	if((this->_type != type_t::ARRAY) && this->valid() && (this->_type != type_t::NUL))
		// Выводим ссылку на отбросное значение
		return Value::scrap();
	// Выполняем получение предела роста вместимого
	const size_t limit = Value::limit();
	/**
	 * Если рост вместимого превышает предел, заводить его нельзя.
	 *
	 * Номер приходит извне - из настроек, из запроса, - и вместимое затребованной
	 * длины съело бы память целиком. Отказ отбросным значением честнее, нежели
	 * нехватка памяти посреди работы, отказа не выдающей вовсе
	 */
	if((limit > 0) && ((index + 1) > limit))
		// Выводим ссылку на отбросное значение
		return Value::scrap();
	// Если значение перечнем ещё не является
	if(this->_type != type_t::ARRAY){
		// Выполняем установку вида узла перечня
		this->_kind = kind_t::ARRAY;
		// Выполняем установку вида перечня
		this->_type = type_t::ARRAY;
	}
	// Выполняем заведение недостающих значений вместимого
	this->_items.resize(index + 1);
	// Выводим затребованное значение вместимого
	return this->_items.at(index);
}
/**
 * @brief Метод добавления значения в конец перечня
 *
 * @param value добавляемое значение
 * @return      признак успешности добавления
 *
 */
bool awh::codec::abc::Value::push(const Value & value) noexcept {
	// Если значение вместимым иного вида является
	if((this->_type != type_t::ARRAY) && this->valid() && (this->_type != type_t::NUL))
		// Сообщаем, что добавление отвечено отказом
		return false;
	// Если значение перечнем ещё не является
	if(this->_type != type_t::ARRAY){
		// Выполняем установку вида узла перечня
		this->_kind = kind_t::ARRAY;
		// Выполняем установку вида перечня
		this->_type = type_t::ARRAY;
	}
	// Выполняем добавление значения в конец перечня
	this->_items.push_back(value);
	// Сообщаем, что добавление успешно
	return true;
}
/**
 * @brief Метод добавления поля в отображение
 *
 * @param name  имя поля отображения
 * @param value добавляемое значение
 * @return      признак успешности добавления
 *
 */
bool awh::codec::abc::Value::insert(const string & name, const Value & value) noexcept {
	// Если значение вместимым иного вида является
	if((this->_type != type_t::MAP) && this->valid() && (this->_type != type_t::NUL))
		// Сообщаем, что добавление отвечено отказом
		return false;
	// Если значение отображением ещё не является
	if(this->_type != type_t::MAP){
		// Выполняем установку вида узла отображения
		this->_kind = kind_t::MAP;
		// Выполняем установку вида отображения
		this->_type = type_t::MAP;
	}
	/**
	 * Выполняем перебор всех имён полей отображения
	 */
	for(size_t i = 0; i < this->_keys.size(); i++){
		// Выполняем получение очередного имени поля отображения
		const Value & key = this->_keys.at(i);
		// Если имя поля отображения совпало с затребованным
		if((key._type == type_t::STRING) && (key._text.compare(name) == 0)){
			/**
			 * Выполняем перезапись занятого имени на прежнем его месте.
			 *
			 * Перенос поля в конец менял бы порядок записи, а порядок этот у ABC есть
			 * часть её: строгий вид записи ведёт имена по возрастанию, и перенос сбил
			 * бы подпись
			 */
			this->_items.at(i) = value;
			// Сообщаем, что добавление успешно
			return true;
		}
	}
	// Выполняем заведение имени затребованного поля отображения
	this->_keys.push_back(Value(name));
	// Выполняем заведение значения затребованного поля отображения
	this->_items.push_back(value);
	// Сообщаем, что добавление успешно
	return true;
}
/**
 * @brief Метод удаления поля отображения по имени
 *
 * @param name имя поля отображения
 * @return     признак успешности удаления
 *
 */
bool awh::codec::abc::Value::erase(const string & name) noexcept {
	// Если значение отображением не является
	if(this->_type != type_t::MAP)
		// Сообщаем, что удаление отвечено отказом
		return false;
	/**
	 * Выполняем перебор всех имён полей отображения
	 */
	for(size_t i = 0; i < this->_keys.size(); i++){
		// Выполняем получение очередного имени поля отображения
		const Value & key = this->_keys.at(i);
		// Если имя поля отображения совпало с затребованным
		if((key._type == type_t::STRING) && (key._text.compare(name) == 0)){
			// Выполняем удаление имени поля отображения
			this->_keys.erase(this->_keys.begin() + static_cast <ptrdiff_t> (i));
			// Выполняем удаление значения поля отображения
			this->_items.erase(this->_items.begin() + static_cast <ptrdiff_t> (i));
			// Сообщаем, что удаление успешно
			return true;
		}
	}
	// Сообщаем, что удаление отвечено отказом
	return false;
}
/**
 * @brief Метод удаления значения вместимого по номеру
 *
 * @param index номер значения вместимого
 * @return      признак успешности удаления
 *
 */
bool awh::codec::abc::Value::erase(const size_t index) noexcept {
	// Если затребованного значения вместимого нет
	if(index >= this->_items.size())
		// Сообщаем, что удаление отвечено отказом
		return false;
	// Выполняем удаление значения вместимого
	this->_items.erase(this->_items.begin() + static_cast <ptrdiff_t> (index));
	// Если значение является отображением
	if((this->_type == type_t::MAP) && (index < this->_keys.size()))
		// Выполняем удаление имени поля отображения
		this->_keys.erase(this->_keys.begin() + static_cast <ptrdiff_t> (index));
	// Сообщаем, что удаление успешно
	return true;
}
/**
 * @brief Метод извлечения значения по пути
 *
 * @param path путь к значению
 * @return     ссылка на значение либо ссылка на отсутствующее значение
 *
 */
const awh::codec::abc::Value & awh::codec::abc::Value::at(const string & path) const noexcept {
	// Обходимое значение документа
	const Value * current = this;
	// Смещение начала разбираемого звена пути
	size_t offset = 0;
	/**
	 * Выполняем обход всех звеньев пути
	 */
	while(offset <= path.size()){
		// Выполняем поиск конца разбираемого звена пути
		const size_t bound = path.find('/', offset);
		// Выполняем получение длины разбираемого звена пути
		const size_t length = ((bound == string::npos) ? (path.size() - offset) : (bound - offset));
		// Выполняем получение разбираемого звена пути
		const string_view segment(path.data() + offset, length);
		// Если звено пути не пусто
		if(!segment.empty()){
			// Разобранный номер значения вместимого
			size_t index = 0;
			// Если звено пути является номером значения
			if(Value::indexed(segment, index)){
				// Если затребованного значения вместимого нет
				if(index >= current->_items.size())
					// Выводим ссылку на отсутствующее значение
					return Value::undefined();
				// Выполняем переход к затребованному значению вместимого
				current = &current->_items.at(index);
			// Если звено пути является именем поля отображения
			} else {
				// Выполняем получение значения затребованного поля отображения
				const Value & value = (* current)[string(segment)];
				// Если затребованного поля отображения нет
				if(!value.valid())
					// Выводим ссылку на отсутствующее значение
					return Value::undefined();
				// Выполняем переход к значению затребованного поля
				current = &value;
			}
		}
		// Если звенья пути исчерпаны
		if(bound == string::npos)
			// Выходим из обхода звеньев пути
			break;
		// Выполняем переход к следующему звену пути
		offset = (bound + 1);
	}
	// Выводим ссылку на найденное значение
	return (* current);
}
/**
 * @brief Метод заведения значения по пути
 *
 * @param path путь к значению
 * @return     ссылка на заведённое значение
 *
 */
awh::codec::abc::Value & awh::codec::abc::Value::place(const string & path) noexcept {
	// Заводимое значение документа
	Value * current = this;
	// Смещение начала разбираемого звена пути
	size_t offset = 0;
	/**
	 * Выполняем обход всех звеньев пути
	 */
	while(offset <= path.size()){
		// Выполняем поиск конца разбираемого звена пути
		const size_t bound = path.find('/', offset);
		// Выполняем получение длины разбираемого звена пути
		const size_t length = ((bound == string::npos) ? (path.size() - offset) : (bound - offset));
		// Выполняем получение разбираемого звена пути
		const string_view segment(path.data() + offset, length);
		// Если звено пути не пусто
		if(!segment.empty()){
			// Разобранный номер значения вместимого
			size_t index = 0;
			// Если звено пути является номером значения
			if(Value::indexed(segment, index))
				// Выполняем заведение затребованного значения вместимого
				current = &(* current)[index];
			// Выполняем заведение затребованного поля отображения
			else current = &(* current)[string(segment)];
		}
		// Если звенья пути исчерпаны
		if(bound == string::npos)
			// Выходим из обхода звеньев пути
			break;
		// Выполняем переход к следующему звену пути
		offset = (bound + 1);
	}
	// Выводим ссылку на заведённое значение
	return (* current);
}
/**
 * @brief Метод извлечения логического значения
 *
 * @param result извлекаемое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::abc::Value::value(bool & result) const noexcept {
	// Выполняем сброс извлекаемого значения
	result = false;
	// Если значение логическим не является
	if(this->_type != type_t::BOOL)
		// Сообщаем, что извлечение отвечено отказом
		return false;
	// Выполняем установку извлекаемого значения
	result = this->_number.flag;
	// Сообщаем, что извлечение успешно
	return true;
}
/**
 * @brief Метод извлечения числа видом целого со знаком
 *
 * @param result извлекаемое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::abc::Value::value(int64_t & result) const noexcept {
	// Выполняем сброс извлекаемого значения
	result = 0;
	// Если значение является отметкой времени
	if(this->_type == type_t::TIME){
		// Выполняем установку извлекаемого значения
		result = this->_number.integer;
		// Сообщаем, что извлечение успешно
		return true;
	}
	// Если значение является целым со знаком
	if(this->is(type_t::SIGNED)){
		// Выполняем установку извлекаемого значения
		result = this->_number.integer;
		// Сообщаем, что извлечение успешно
		return true;
	}
	// Если значение является целым без знака
	if(this->is(type_t::UNSIGNED)){
		// Если число видом со знаком не представимо
		if(this->_number.natural > static_cast <uint64_t> (numeric_limits <int64_t>::max()))
			// Сообщаем, что извлечение отвечено отказом
			return false;
		// Выполняем установку извлекаемого значения
		result = static_cast <int64_t> (this->_number.natural);
		// Сообщаем, что извлечение успешно
		return true;
	}
	// Если значение является дробным
	if(this->is(type_t::REAL)){
		// Выполняем получение дробного значения
		const double real = this->_number.real;
		// Если дробное значение видом целого со знаком не представимо
		if((real < -9223372036854775808.0) || (real > 9223372036854774784.0) ||
		   (real != static_cast <double> (static_cast <int64_t> (real))))
			// Сообщаем, что извлечение отвечено отказом
			return false;
		// Выполняем установку извлекаемого значения
		result = static_cast <int64_t> (real);
		// Сообщаем, что извлечение успешно
		return true;
	}
	// Сообщаем, что извлечение отвечено отказом
	return false;
}
/**
 * @brief Метод извлечения числа видом целого без знака
 *
 * @param result извлекаемое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::abc::Value::value(uint64_t & result) const noexcept {
	// Выполняем сброс извлекаемого значения
	result = 0;
	// Если значение является целым без знака
	if(this->is(type_t::UNSIGNED)){
		// Выполняем установку извлекаемого значения
		result = this->_number.natural;
		// Сообщаем, что извлечение успешно
		return true;
	}
	// Если значение является целым со знаком либо отметкой времени
	if(this->is(type_t::SIGNED) || (this->_type == type_t::TIME)){
		// Если число меньше нуля, видом без знака оно не представимо
		if(this->_number.integer < 0)
			// Сообщаем, что извлечение отвечено отказом
			return false;
		// Выполняем установку извлекаемого значения
		result = static_cast <uint64_t> (this->_number.integer);
		// Сообщаем, что извлечение успешно
		return true;
	}
	// Если значение является дробным
	if(this->is(type_t::REAL)){
		// Выполняем получение дробного значения
		const double real = this->_number.real;
		// Если дробное значение видом целого без знака не представимо
		if((real < 0.0) || (real > 18446744073709549568.0) ||
		   (real != static_cast <double> (static_cast <uint64_t> (real))))
			// Сообщаем, что извлечение отвечено отказом
			return false;
		// Выполняем установку извлекаемого значения
		result = static_cast <uint64_t> (real);
		// Сообщаем, что извлечение успешно
		return true;
	}
	// Сообщаем, что извлечение отвечено отказом
	return false;
}
/**
 * @brief Метод извлечения числа видом дробного
 *
 * @param result извлекаемое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::abc::Value::value(double & result) const noexcept {
	// Выполняем сброс извлекаемого значения
	result = 0.0;
	// Если значение является дробным
	if(this->is(type_t::REAL)){
		// Выполняем установку извлекаемого значения
		result = this->_number.real;
		// Сообщаем, что извлечение успешно
		return true;
	}
	// Если значение является целым со знаком
	if(this->is(type_t::SIGNED)){
		// Выполняем установку извлекаемого значения
		result = static_cast <double> (this->_number.integer);
		// Сообщаем, что извлечение успешно
		return true;
	}
	// Если значение является целым без знака
	if(this->is(type_t::UNSIGNED)){
		// Выполняем установку извлекаемого значения
		result = static_cast <double> (this->_number.natural);
		// Сообщаем, что извлечение успешно
		return true;
	}
	// Сообщаем, что извлечение отвечено отказом
	return false;
}
/**
 * @brief Метод извлечения содержимого значения строкой
 *
 * @param result извлекаемое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::abc::Value::value(string & result) const noexcept {
	// Выполняем очистку извлекаемого значения
	result.clear();
	// Если значение отрезком октетов не хранится
	if(!this->is(type_t::SEGMENT) && (this->_type != type_t::UUID))
		// Сообщаем, что извлечение отвечено отказом
		return false;
	// Выполняем установку извлекаемого значения
	result = this->_text;
	// Сообщаем, что извлечение успешно
	return true;
}
/**
 * @brief Метод перенесения значения из дерева документа
 *
 * @param value переносимое значение дерева документа
 *
 */
void awh::codec::abc::Value::absorb(const Document::value_t & value) noexcept {
	// Выполняем очистку переносимого значения
	this->clear();
	// Если переносимое значение недействительно
	if(!value.valid())
		// Выходим из работы
		return;
	// Работа перенесения одного значения дерева документа
	typedef struct Task {
		// Переносимое значение дерева документа
		Document::value_t source;
		// Значение, куда следует перенести
		Value * target;
	} task_t;
	// Вместилище работ перенесения
	vector <task_t> pending;
	// Выполняем заведение работы перенесения корня
	pending.push_back(task_t{value, this});
	/**
	 * Выполняем все работы перенесения без возвратности.
	 *
	 * Указания на заводимых детей действительны оттого, что вместилища их заводятся
	 * сразу нужной длины и более не растут: перевыделение памяти обратило бы их в
	 * указания на снесённое
	 */
	while(!pending.empty()){
		// Выполняем снятие очередной работы перенесения
		const task_t task = pending.back();
		// Выполняем удаление снятой работы перенесения
		pending.pop_back();
		// Выполняем получение вида переносимого значения
		const type_t type = task.source.type();
		// Выполняем установку вида переносимого значения
		task.target->_type = type;
		// Выполняем установку вида узла переносимого значения
		task.target->_kind = abc::kind(type);
		/**
		 * Определяем вид переносимого значения
		 */
		switch(static_cast <uint32_t> (type)){
			/**
			 * Если значение является вместимым
			 */
			case static_cast <uint32_t> (type_t::ARRAY):
			case static_cast <uint32_t> (type_t::MAP): {
				// Выполняем получение количества значений вместимого
				const size_t count = task.source.size();
				// Выполняем заведение значений вместимого
				task.target->_items.resize(count);
				// Если вместимое является отображением
				if(type == type_t::MAP)
					// Выполняем заведение имён полей отображения
					task.target->_keys.resize(count);
				/**
				 * Выполняем заведение работ перенесения всех детей вместимого
				 */
				for(size_t i = 0; i < count; i++){
					// Выполняем заведение работы перенесения значения вместимого
					pending.push_back(task_t{task.source.at(i), &task.target->_items.at(i)});
					// Если вместимое является отображением
					if(type == type_t::MAP)
						// Выполняем заведение работы перенесения имени поля
						pending.push_back(task_t{task.source.key(i), &task.target->_keys.at(i)});
				}
			} break;
			/**
			 * Если значение является логическим
			 */
			case static_cast <uint32_t> (type_t::BOOL): {
				// Извлекаемое логическое значение
				bool flag = false;
				// Выполняем извлечение логического значения
				(void) task.source.value(flag);
				// Выполняем установку логического значения
				task.target->_number.flag = flag;
			} break;
			/**
			 * Если значение хранится отрезком октетов
			 */
			case static_cast <uint32_t> (type_t::STRING):
			case static_cast <uint32_t> (type_t::BLOB):
			case static_cast <uint32_t> (type_t::UUID): {
				// Выполняем перенесение содержимого значения
				task.target->_text.assign(task.source.data());
			} break;
			/**
			 * Если значение является числом неограниченной ширины
			 */
			case static_cast <uint32_t> (type_t::EXTENDED):
			case static_cast <uint32_t> (type_t::DECIMAL): {
				// Выполняем перенесение октетов величины
				task.target->_text.assign(task.source.data());
				// Выполняем установку десятичного порядка величины
				task.target->_exponent = task.source.exponent();
				// Выполняем установку признака отрицательности величины
				task.target->_negative = task.source.negative();
			} break;
			// Если значение является пустым
			case static_cast <uint32_t> (type_t::NUL): break;
			/**
			 * Если значение является числом родного вида либо отметкой времени
			 */
			default: {
				// Если значение является дробным
				if((static_cast <uint32_t> (type) & static_cast <uint32_t> (type_t::REAL)) != 0){
					// Извлекаемое дробное значение
					double real = 0.0;
					// Выполняем извлечение дробного значения
					(void) task.source.value(real);
					// Выполняем установку дробного значения
					task.target->_number.real = real;
				// Если значение является целым со знаком либо отметкой времени
				} else if(((static_cast <uint32_t> (type) & static_cast <uint32_t> (type_t::SIGNED)) != 0) ||
				          (type == type_t::TIME)) {
					// Извлекаемое целое со знаком
					int64_t integer = 0;
					// Выполняем извлечение целого со знаком
					(void) task.source.value(integer);
					// Выполняем установку целого со знаком
					task.target->_number.integer = integer;
				// Если значение является целым без знака
				} else {
					// Извлекаемое целое без знака
					uint64_t natural = 0;
					// Выполняем извлечение целого без знака
					(void) task.source.value(natural);
					// Выполняем установку целого без знака
					task.target->_number.natural = natural;
				}
			} break;
		}
	}
}
/**
 * @brief Метод укладки значения в собираемую запись
 *
 * @param writer сборщик бинарной записи
 * @return       признак успешности укладки
 *
 */
bool awh::codec::abc::Value::compose(writer_t & writer) const noexcept {
	/**
	 * @brief Функция укладки одиночного значения в собираемую запись
	 *
	 * @param node   укладываемое значение
	 * @param writer сборщик бинарной записи
	 * @return       признак успешности укладки
	 *
	 */
	auto single = [](const Value & node, writer_t & writer) noexcept -> bool {
		/**
		 * Определяем вид укладываемого значения
		 */
		switch(static_cast <uint32_t> (node._type)){
			/**
			 * Если значения нет вовсе, укладывается пустое значение.
			 *
			 * Изменяемое обращение заводит значение вида `NONE`, и потребитель вправе
			 * его не заполнить. Отказ здесь означал бы, что заведённое обращением поле
			 * делает запись несобираемой вовсе
			 */
			case static_cast <uint32_t> (type_t::UNDEFINED):
			case static_cast <uint32_t> (type_t::NUL): return writer.nul();
			// Если значение является логическим
			case static_cast <uint32_t> (type_t::BOOL): return writer.boolean(node._number.flag);
			// Если значение является строкой
			case static_cast <uint32_t> (type_t::STRING): return writer.text(node._text);
			// Если значение является двоичными данными
			case static_cast <uint32_t> (type_t::BLOB): return writer.blob(node._text.data(), node._text.size());
			// Если значение является опознавателем
			case static_cast <uint32_t> (type_t::UUID): return writer.uuid(node._text.data(), node._text.size());
			// Если значение является отметкой времени
			case static_cast <uint32_t> (type_t::TIME): return writer.timestamp(node._number.integer);
			/**
			 * Если значение является числом неограниченной ширины
			 */
			case static_cast <uint32_t> (type_t::EXTENDED):
			case static_cast <uint32_t> (type_t::DECIMAL):
				// Выполняем укладку числа неограниченной ширины
				return writer.decimal(node._text.data(), node._text.size(), node._negative, node._exponent);
		}
		// Если значение является дробным
		if((static_cast <uint32_t> (node._type) & static_cast <uint32_t> (type_t::REAL)) != 0)
			// Выполняем укладку дробного значения
			return writer.number(node._number.real);
		// Если значение является целым со знаком
		if((static_cast <uint32_t> (node._type) & static_cast <uint32_t> (type_t::SIGNED)) != 0)
			// Выполняем укладку целого со знаком
			return writer.number(node._number.integer);
		// Если значение является целым без знака
		if((static_cast <uint32_t> (node._type) & static_cast <uint32_t> (type_t::UNSIGNED)) != 0)
			// Выполняем укладку целого без знака
			return writer.number(node._number.natural);
		// Сообщаем, что укладка отвечена отказом
		return false;
	};
	/**
	 * @brief Функция укладки начала вместимого в собираемую запись
	 *
	 * @param node   укладываемое вместимое
	 * @param writer сборщик бинарной записи
	 * @return       признак успешности укладки
	 *
	 */
	auto open = [](const Value & node, writer_t & writer) noexcept -> bool {
		// Если вместимое является отображением
		if(node._type == type_t::MAP){
			// Если имён полей у отображения не столько же, сколько значений
			if(node._keys.size() != node._items.size())
				// Сообщаем, что укладка отвечена отказом
				return false;
			// Выполняем укладку начала отображения
			return writer.mapBegin(static_cast <uint64_t> (node._items.size()));
		}
		// Выполняем укладку начала массива
		return writer.arrayBegin(static_cast <uint64_t> (node._items.size()));
	};
	// Если значение вместимым не является
	if(!this->is(type_t::CONTAINER))
		// Выполняем укладку одиночного значения
		return single((* this), writer);
	// Если укладка начала вместимого отвечена отказом
	if(!open((* this), writer))
		// Сообщаем, что укладка отвечена отказом
		return false;
	// Вместилище обходимых вместимых вместе с номером очередного ребёнка
	vector <pair <const Value *, size_t>> stack;
	// Выполняем заведение обхода корня укладываемого значения
	stack.push_back(make_pair(this, static_cast <size_t> (0)));
	/**
	 * Выполняем укладку всех вместимых без возвратности
	 */
	while(!stack.empty()){
		// Выполняем получение обходимого вместимого
		const Value * node = stack.back().first;
		// Если дети обходимого вместимого исчерпаны
		if(stack.back().second >= node->_items.size()){
			// Выполняем снятие обходимого вместимого с вместилища
			stack.pop_back();
			// Если закрытие вместимого отвечено отказом
			if(!((node->_type == type_t::MAP) ? writer.mapEnd() : writer.arrayEnd()))
				// Сообщаем, что укладка отвечена отказом
				return false;
			// Продолжаем укладку вместимых
			continue;
		}
		// Выполняем получение номера очередного ребёнка вместимого
		const size_t index = stack.back().second++;
		// Если вместимое является отображением
		if(node->_type == type_t::MAP){
			// Если укладка имени поля отображения отвечена отказом
			if(!single(node->_keys.at(index), writer))
				// Сообщаем, что укладка отвечена отказом
				return false;
		}
		// Выполняем получение очередного значения вместимого
		const Value & child = node->_items.at(index);
		// Если значение вместимого вместимым не является
		if(!child.is(type_t::CONTAINER)){
			// Если укладка одиночного значения отвечена отказом
			if(!single(child, writer))
				// Сообщаем, что укладка отвечена отказом
				return false;
			// Продолжаем укладку вместимых
			continue;
		}
		// Если укладка начала вложенного вместимого отвечена отказом
		if(!open(child, writer))
			// Сообщаем, что укладка отвечена отказом
			return false;
		// Выполняем заведение обхода вложенного вместимого
		stack.push_back(make_pair(&child, static_cast <size_t> (0)));
	}
	// Сообщаем, что укладка успешна
	return true;
}
/**
 * @brief Метод разбора записи во владеющее значение
 *
 * @param buffer буфер разбираемой записи
 * @param size   размер разбираемой записи в октетах
 * @return       признак успешности разбора
 *
 */
bool awh::codec::abc::Value::parse(const void * buffer, const size_t size) noexcept {
	// Выполняем очистку разбираемого значения
	this->clear();
	// Дерево разбираемого документа
	document_t document;
	// Если разбор записи в дерево документа отвечен отказом
	if(!document.parse(buffer, size))
		// Сообщаем, что разбор отвечен отказом
		return false;
	// Выполняем перенесение корня дерева документа
	this->absorb(document.root());
	// Сообщаем, что разбор успешен
	return true;
}
/**
 * @brief Метод сборки записи из владеющего значения
 *
 * @return собранная запись
 *
 */
vector <uint8_t> awh::codec::abc::Value::dump() const noexcept {
	// Сборщик бинарной записи
	writer_t writer;
	// Если укладка значения в собираемую запись отвечена отказом
	if(!this->compose(writer))
		// Выводим пустую запись
		return vector <uint8_t> ();
	// Выводим собранную запись
	return writer.record();
}
/**
 * @brief Оператор сличения значений
 *
 * @param value сличаемое значение
 * @return      признак равенства значений
 *
 */
bool awh::codec::abc::Value::operator == (const Value & value) const noexcept {
	/**
	 * @brief Функция сличения одиночных значений
	 *
	 * @param first  первое сличаемое значение
	 * @param second второе сличаемое значение
	 * @return       признак равенства значений
	 *
	 */
	auto single = [](const Value & first, const Value & second) noexcept -> bool {
		/**
		 * Если сличаются числа родного вида, сличаем их значением, а не видом хранения.
		 *
		 * Вид хранения при пересборке записи сужается: `UINT64` со значением `42`
		 * укладывается наименьшей записью и разбирается обратно уже как `UINT8`.
		 * Сличение по виду хранения объявляло бы такие значения разными, хотя число
		 * у них одно и то же
		 */
		if(first.is(type_t::INT) && second.is(type_t::INT)){
			// Если оба числа являются целыми без знака
			if(first.is(type_t::UNSIGNED) && second.is(type_t::UNSIGNED))
				// Выводим признак равенства целых без знака
				return (first._number.natural == second._number.natural);
			// Если оба числа являются целыми со знаком
			if(first.is(type_t::SIGNED) && second.is(type_t::SIGNED))
				// Выводим признак равенства целых со знаком
				return (first._number.integer == second._number.integer);
			// Выполняем получение числа со знаком из сличаемых
			const int64_t integer = (first.is(type_t::SIGNED) ? first._number.integer : second._number.integer);
			// Выполняем получение числа без знака из сличаемых
			const uint64_t natural = (first.is(type_t::SIGNED) ? second._number.natural : first._number.natural);
			// Если число со знаком меньше нуля, равным беззнаковому оно быть не может
			if(integer < 0)
				// Сообщаем, что значения неравны
				return false;
			// Выводим признак равенства чисел разной знаковости
			return (static_cast <uint64_t> (integer) == natural);
		}
		// Если сличаются числа, из которых хотя бы одно является дробным
		if((first.is(type_t::INT) || first.is(type_t::REAL)) && (second.is(type_t::INT) || second.is(type_t::REAL))){
			// Извлекаемое первое дробное значение
			double left = 0.0;
			// Извлекаемое второе дробное значение
			double right = 0.0;
			// Если извлечение сличаемых значений отвечено отказом
			if(!first.value(left) || !second.value(right))
				// Сообщаем, что значения неравны
				return false;
			/**
			 * Если оба значения суть нечисла, сличаем их записью.
			 *
			 * Нечисло само себе не равно по правилу арифметики, но правило это о
			 * вычислениях, а не о тождестве записи: значение обязано быть равно самому
			 * себе, иначе круговой ход записи недоказуем вовсе
			 */
			if((left != left) && (right != right)){
				// Запись первого сличаемого нечисла
				uint64_t bits = 0;
				// Запись второго сличаемого нечисла
				uint64_t other = 0;
				// Выполняем получение записи первого сличаемого нечисла
				::memcpy(& bits, & left, sizeof(bits));
				// Выполняем получение записи второго сличаемого нечисла
				::memcpy(& other, & right, sizeof(other));
				// Выводим признак равенства записей сличаемых нечисел
				return (bits == other);
			}
			// Выводим признак равенства дробных значений
			return (left == right);
		}
		/**
		 * Если сличаются числа неограниченной ширины, сличаем их величиной вместе со
		 * знаком и порядком: целое и десятичное с нулевым порядком есть одно и то же
		 * число, а видом хранения они расходятся
		 */
		if(((first._type == type_t::EXTENDED) || (first._type == type_t::DECIMAL)) &&
		   ((second._type == type_t::EXTENDED) || (second._type == type_t::DECIMAL)))
			// Выводим признак равенства величин вместе со знаком и порядком
			return ((first._negative == second._negative) && (first._exponent == second._exponent) &&
			        (first._text.compare(second._text) == 0));
		// Если виды сличаемых значений расходятся
		if(first._type != second._type)
			// Сообщаем, что значения неравны
			return false;
		/**
		 * Определяем вид сличаемых значений
		 */
		switch(static_cast <uint32_t> (first._type)){
			// Если значения отсутствуют либо пусты
			case static_cast <uint32_t> (type_t::UNDEFINED):
			case static_cast <uint32_t> (type_t::NUL): return true;
			// Если значения являются логическими
			case static_cast <uint32_t> (type_t::BOOL): return (first._number.flag == second._number.flag);
			// Если значения хранятся отрезком октетов
			case static_cast <uint32_t> (type_t::STRING):
			case static_cast <uint32_t> (type_t::BLOB):
			case static_cast <uint32_t> (type_t::UUID): return (first._text.compare(second._text) == 0);
			// Если значения являются отметками времени
			case static_cast <uint32_t> (type_t::TIME): return (first._number.integer == second._number.integer);
		}
		// Сообщаем, что значения неравны
		return false;
	};
	// Вместилище пар значений, ожидающих сличения
	vector <pair <const Value *, const Value *>> pending;
	// Выполняем заведение сличения корней значений
	pending.push_back(make_pair(this, &value));
	/**
	 * Выполняем сличение всех пар значений без возвратности
	 */
	while(!pending.empty()){
		// Выполняем снятие очередной пары значений
		const pair <const Value *, const Value *> couple = pending.back();
		// Выполняем удаление снятой пары значений
		pending.pop_back();
		// Выполняем получение первого сличаемого значения
		const Value & first = (* couple.first);
		// Выполняем получение второго сличаемого значения
		const Value & second = (* couple.second);
		// Если вместимость сличаемых значений расходится
		if(first.is(type_t::CONTAINER) != second.is(type_t::CONTAINER))
			// Сообщаем, что значения неравны
			return false;
		// Если сличаемые значения вместимыми не являются
		if(!first.is(type_t::CONTAINER)){
			// Если одиночные значения расходятся
			if(!single(first, second))
				// Сообщаем, что значения неравны
				return false;
			// Продолжаем сличение пар значений
			continue;
		}
		// Если виды сличаемых вместимых расходятся
		if(first._type != second._type)
			// Сообщаем, что значения неравны
			return false;
		// Если количества значений вместимых расходятся
		if(first._items.size() != second._items.size())
			// Сообщаем, что значения неравны
			return false;
		// Если сличаемые значения являются перечнями
		if(first._type == type_t::ARRAY){
			/**
			 * Выполняем заведение сличения всех значений перечней по порядку
			 */
			for(size_t i = 0; i < first._items.size(); i++)
				// Выполняем заведение сличения очередной пары значений
				pending.push_back(make_pair(&first._items.at(i), &second._items.at(i)));
			// Продолжаем сличение пар значений
			continue;
		}
		/**
		 * Выполняем сличение отображений без учёта порядка полей.
		 *
		 * Порядок полей отображения принадлежит записи, а не значению: два отображения
		 * с одними и теми же полями равны, как бы поля ни были уложены. Эталоном здесь
		 * взято поведение JavaScript
		 */
		for(size_t i = 0; i < first._keys.size(); i++){
			// Признак того, что поле отображения найдено
			bool found = false;
			/**
			 * Выполняем поиск имени поля отображения среди имён второго значения
			 */
			for(size_t j = 0; j < second._keys.size(); j++){
				// Если имена полей отображений расходятся
				if(!single(first._keys.at(i), second._keys.at(j)))
					// Продолжаем поиск имени поля отображения
					continue;
				// Выполняем заведение сличения значений найденных полей
				pending.push_back(make_pair(&first._items.at(i), &second._items.at(j)));
				// Выполняем установку признака того, что поле найдено
				found = true;
				// Выходим из поиска имени поля отображения
				break;
			}
			// Если поле отображения не найдено
			if(!found)
				// Сообщаем, что значения неравны
				return false;
		}
	}
	// Сообщаем, что значения равны
	return true;
}
/**
 * @brief Оператор сличения значений на неравенство
 *
 * @param value сличаемое значение
 * @return      признак неравенства значений
 *
 */
bool awh::codec::abc::Value::operator != (const Value & value) const noexcept {
	// Выводим признак неравенства значений
	return !((* this) == value);
}
